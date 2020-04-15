#include <iostream>

#include "gibline.h"

int main(int argc, char *argv[]) {
    for (;;) {
        auto cmd = gib::line("dummy>");
        std::cout << cmd;
    }
}
