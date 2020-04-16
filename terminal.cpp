#include "terminal.h"

#include <cassert>

namespace gib::terminal {

void Terminal::enable_raw_mode() {
  tcgetattr(fd_, &saved_termios_);
  termios new_termios{saved_termios_};
  new_termios.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(fd_, TCSAFLUSH, &new_termios);
}

uint8_t Terminal::read() {
  uint8_t byte;
  while (true) {
    int result = ::read(fd_, &byte, 1);
    if (result > 0) {
      return byte;
    }
  }
}

void Terminal::write(uint8_t byte) { assert(::write(fd_, &byte, 1) == 1); }

void Terminal::write(const std::string_view &text) {
  assert(::write(fd_, text.data(), text.size()) == text.size());
}

void Terminal::disable_raw_mode() {
  tcsetattr(fd_, TCSAFLUSH, &saved_termios_);
}

}  // namespace gib::terminal
