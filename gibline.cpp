#include "gibline.h"

#include <iostream>

namespace gib {

std::string line(std::string_view prompt) {
  std::string response;
  std::cout << prompt;
  getline(std::cin, response);
  return response;
}

}  // namespace gib
