#include "terminal.h"

namespace gib::terminal {

void Terminal::enable_raw_mode() {
  tcgetattr(fd_, &saved_termios_);
  struct termios new_termios {
    saved_termios_
  };
  new_termios.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(fd_, TCSAFLUSH, &new_termios);
}

void Terminal::disable_raw_mode() { tcsetattr(fd_, TCSAFLUSH, &saved_termios_); }

}  // namespace gib::terminal
