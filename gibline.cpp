#include "gibline.h"
#include "terminal.h"

#include <iostream>
#include <vector>

namespace gib {

class Engine {
 public:
  Engine() : terminal_{STDIN_FILENO} {}
  Engine(const Engine &other) = delete;
  void push_history(std::string value) { history.push_back(value); }

  terminal::Terminal* terminal() { return &terminal_; }

 private:
  std::vector<std::string> history;
  terminal::Terminal terminal_;
};

Engine &engine() {
  static Engine state;
  return state;
}

std::string line(std::string_view prompt) {
  auto &state = engine();
  terminal::RawModeGuard{state.terminal()};
  std::string response;
  std::cout << prompt;
  getline(std::cin, response);
  state.push_history(response);
  return response;
}

}  // namespace gib
