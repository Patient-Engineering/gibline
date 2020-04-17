#include "gibline.h"

#include <cstdint>
#include <iostream>
#include <vector>

#include "terminal.h"

namespace gib {
namespace {

namespace ansi {
const char *PULL_LEFT = "\x1b[1000D";
const char *CLEAR = "\x1b[0K";
}  // namespace ansi

class Cursor {
 public:
  // Creates a new cursor
  Cursor() : position_(), state_() {}

  // Put a single character at the current cursor position.
  void put(char character) {
    state_.insert(position_, std::string() + character);
    position_++;
  }

  // Delete a single character from current cursor position back.
  void backspace() {
    if (!at_beginning()) {
      state_.erase(position_ - 1, 1);
      position_--;
    }
  }

  bool at_beginning() const { return position_ == 0; }

  bool at_end() const { return position_ == state_.size(); }

  char current() const { return state_[position_ - 1]; }

  void home() { position_ = 0; }

  void end() { position_ = state_.size(); }

  // Move forward a word from the cursor position.
  void word_forward() {
    while (!at_end()) {
      if (current() != ' ') {
        break;
      }
      right();
    }
    while (!at_beginning()) {
      if (current() == ' ') {
        break;
      }
      right();
    }
  }

  // Move back a word from the cursor position.
  void word_back() {
    while (!at_beginning()) {
      if (current() != ' ') {
        break;
      }
      left();
    }
    while (!at_beginning()) {
      if (current() == ' ') {
        break;
      }
      left();
    }
  }

  // Clear a word back from the cursor position.
  void clear_word() {
    while (!at_beginning()) {
      if (current() != ' ') {
        break;
      }
      backspace();
    }
    while (!at_beginning()) {
      if (current() == ' ') {
        break;
      }
      backspace();
    }
  }

  // Clear back from the cursor position to beginning of the line.
  void clear_to_start() {
    while (!at_beginning()) {
      backspace();
    }
  }

  size_t position() const { return position_; }

  void left() {
    if (!at_beginning()) {
      position_--;
    }
  }

  void right() {
    if (!at_end()) {
      position_++;
    }
  }

  // Get current line contents.
  const std::string &line() const { return state_; }

  // Set line contents, and put cursor at the end of the line.
  void reset(const std::string &&to) { state_ = std::move(to); }

 private:
  size_t position_;
  std::string state_;
};

class Engine {
 public:
  Engine() : terminal_{STDIN_FILENO}, history_offset_(0) {}
  Engine(const Engine &other) = delete;

  // Record a new history entry and put it at the end.
  void push_history(std::string value) {
    history_.push_back(value);
    history_offset_ = history_.size();
  }

  // Move history one entry back and return selected item.
  std::string history_back() {
    if (history_.empty()) {
      return "";
    }
    if (--history_offset_ < 0) {
      history_offset_ = 0;
    }
    return history_[history_offset_];
  }

  // Move history one entry forward and return selected item.
  std::string history_forward() {
    if (++history_offset_ > history_.size()) {
      history_offset_ = history_.size();
    }
    if (history_offset_ == history_.size()) {
      // "Current" history entry is empty.
      return "";
    }
    return history_[history_offset_];
  }

  terminal::Terminal *terminal() { return &terminal_; }

 private:
  int history_offset_;
  std::vector<std::string> history_;
  terminal::Terminal terminal_;
};

// Get engine singleton.
Engine &engine() {
  static Engine state;
  return state;
}

// Clear the current line, then print prompt and the line contents.
void redraw(const Cursor &cursor, std::string_view prompt,
            terminal::Terminal *term) {
  const auto &tinfo = term->terminfo();
  term->write(tinfo.query_string("carriage_return"));
  term->write(tinfo.query_string("clr_eol"));
  term->write(prompt);
  for (size_t i = 0; i < cursor.line().size(); i++) {
    if (i == cursor.position()) {
      term->write("_");
    } else {
      term->write(cursor.line()[i]);
    }
  }
}

}  // namespace

// Read a single line and return it with the ending newline.
std::string line(std::string_view prompt) {
  auto &state = engine();
  auto *term = state.terminal();
  terminal::RawModeGuard rawmode{term};

  int history_offset = 0;
  Cursor cursor;
  bool done = false;
  while (!done) {
    redraw(cursor, prompt, term);
    uint8_t next = term->read();
    uint8_t mod;
    uint8_t value;
    switch (next) {
      case '\n':
      case 4:  // C-D
        term->write("\n");
        done = true;
        break;
      case 127:  // backspace
        cursor.backspace();
        break;
      case 21:  // C-U
        cursor.clear_to_start();
        break;
      case 23:  // C-W
        cursor.clear_word();
        break;
      case 98:  // M-B
        cursor.word_back();
        break;
      case 27:  // control
        mod = term->read();
        if (mod == 91) {
          value = term->read();
          if (value == 65) {  // up arrow
            cursor.reset(state.history_back());
          } else if (value == 66) {  // down arrow
            cursor.reset(state.history_forward());
          } else if (value == 67) {  // right arrow
            cursor.right();
          } else if (value == 68) {  // left arrow
            cursor.left();
          }
        } else if (mod == 'f') {  // M-f
          cursor.word_forward();
        } else if (mod == 'b') {  // M-b
          cursor.word_back();
        } else if (mod == 'a') {  // M-a
          cursor.home();
        } else if (mod == 'e') {  // M-e
          cursor.end();
        }
        break;
      default:
        cursor.put(next);
        break;
    }
  }

  state.push_history(cursor.line());
  return cursor.line() + '\n';
}

}  // namespace gib
