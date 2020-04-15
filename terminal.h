#pragma once

#include <termios.h>
#include <unistd.h>

#include <cstdio>

namespace gib::terminal {

class Terminal {
 public:
  Terminal(int fd) : fd_{fd} {}
  Terminal(const Terminal &) = delete;

  bool is_tty() const { return isatty(fd_); }

  void enable_raw_mode();
  void disable_raw_mode();

 private:
  int fd_;
  struct termios saved_termios_;
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
