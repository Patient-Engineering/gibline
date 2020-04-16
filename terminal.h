#pragma once

#include <termios.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <map>
#include <string_view>

namespace gib::terminal {

class TermInfo {
 public:
  static TermInfo from_env();
  static TermInfo from_file(const std::filesystem::path &path);

  using FlagMap = std::map<std::string_view, bool>;
  using NumberMap = std::map<std::string_view, int32_t>;
  using StringMap = std::map<std::string_view, std::string>;

  std::string_view query_string(const std::string_view query) const;

 private:
  TermInfo(const TermInfo &) = delete;
  TermInfo(FlagMap &&fm, NumberMap &&nm, StringMap &&sm)
      : flags_{std::move(fm)},
        numbers_{std::move(nm)},
        strings_{std::move(sm)} {}
  std::map<std::string_view, bool> flags_;
  std::map<std::string_view, int32_t> numbers_;
  std::map<std::string_view, std::string> strings_;
};

class Terminal {
 public:
  Terminal(int fd) : fd_{fd}, terminfo_{TermInfo::from_env()} {}
  Terminal(const Terminal &) = delete;

  bool is_tty() const { return isatty(fd_); }

  uint8_t read();
  void write(uint8_t byte);
  void write(const std::string_view &text);
  void enable_raw_mode();
  void disable_raw_mode();

  const TermInfo &terminfo() { return terminfo_; }

 private:
  int fd_;
  struct termios saved_termios_;
  TermInfo terminfo_;
};

class RawModeGuard {
 public:
  RawModeGuard(Terminal *term) : term_{term} {
    if (term_->is_tty()) {
      term->enable_raw_mode();
    }
  }

  ~RawModeGuard() {
    if (term_->is_tty()) {
      term_->disable_raw_mode();
    }
  }

 private:
  Terminal *term_;
};

}  // namespace gib::terminal
