#include "gibline.h"

#include <iostream>
#include <vector>

namespace gib {

class Engine {
 public:
  Engine() = default;
  Engine(const Engine &other) = delete;
  void push_history(std::string value) { history.push_back(value); }

 private:
  std::vector<std::string> history;
};

Engine &engine() {
  static Engine state;
  return state;
}

std::string line(std::string_view prompt) {
  auto &state = engine();
  std::string response;
  std::cout << prompt;
  getline(std::cin, response);
  state.push_history(response);
  return response;
}

}  // namespace gib
