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
  // Put a single character at the current cursor position.
  void put(uint8_t character) { state_.push_back(character); }

  // Delete a single character from current cursor position back.
  void backspace() {
    if (!state_.empty()) {
      state_.pop_back();
    }
  }

  // Clear a word back from the cursor position.
  void clear_word() {
    while (!state_.empty()) {
      if (state_.back() != ' ') {
        break;
      }
      state_.pop_back();
    }
    while (!state_.empty()) {
      if (state_.back() == ' ') {
        break;
      }
      state_.pop_back();
    }
  }

  // Clear back from the cursor position to beginning of the line.
  void clear_to_start() {
    while (!state_.empty()) {
      state_.pop_back();
    }
  }

  // Get current line contents.
  const std::string &line() const { return state_; }

  // Set line contents, and put cursor at the end of the line.
  void reset(const std::string &&to) { state_ = std::move(to); }

 private:
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
  term->write(cursor.line());
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
      case 27:  // control
        mod = term->read();
        if (mod != 91) {
          break;
        }
        value = term->read();
        if (value == 65) {  // up arrow
          cursor.reset(state.history_back());
        } else if (value == 66) {  // down arrow
          cursor.reset(state.history_forward());
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
