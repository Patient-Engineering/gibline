#include "terminal.h"

#include <cassert>
#include <cstring>
#include <fstream>
#include <map>
#include <vector>

#include "termnames.h"

namespace gib::terminal {

namespace {
template <typename T>
T read_into(std::ifstream *stream) {
  T val;
  std::array<char, sizeof(val)> buf;
  stream->read(buf.data(), buf.size());
  // TODO endianness
  ::memcpy(&val, buf.data(), buf.size());
  return val;
}

std::string read_until_null(std::ifstream *stream) {
  std::string s;
  for (;;) {
    char c;
    stream->read(&c, 1);
    if (c == 0) break;
    s.append(&c, 1);
  }
  return s;
}
}  // namespace

TermInfo TermInfo::from_env() {
  std::string_view term{getenv("TERM")};
  if (term.empty()) {
    // fallback mode
    term = "dumb";
  }

  std::filesystem::path terminfo_base{"/usr/share/terminfo"};
  auto terminfo_path{terminfo_base / term.substr(0, 1) / term};

  return TermInfo::from_file(terminfo_path);
}

std::string_view TermInfo::query_string(const std::string_view query) const {
  return strings_.at(query);
}

TermInfo TermInfo::from_file(const std::filesystem::path &path) {
  std::ifstream terminfo_file{path, std::ios::in | std::ios::binary};
  enum class InfoType { Legacy, Extended };

  int16_t magic{read_into<int16_t>(&terminfo_file)};
  InfoType infotype;
  if (magic == 01036) {
    infotype = InfoType::Extended;
  } else if (magic == 0432) {
    infotype = InfoType::Legacy;
  } else {
    throw std::runtime_error("Unknown terminfo type");
  }

  auto names_size{read_into<int16_t>(&terminfo_file)};
  auto bools_size{read_into<int16_t>(&terminfo_file)};
  auto numbers_size{read_into<int16_t>(&terminfo_file)};
  auto offsets_size{read_into<int16_t>(&terminfo_file)};
  auto strings_size{read_into<int16_t>(&terminfo_file)};

  std::string names{read_until_null(&terminfo_file)};

  std::map<std::string_view, bool> bools;
  for (int i = 0; i < bools_size; i++) {
    int8_t flag{read_into<int8_t>(&terminfo_file)};

    if (i < names::bool_map.size()) {
      bools.insert({names::bool_map[i], flag != 0});
    }
  }

  // Must be aligned to 2 bytes
  if (terminfo_file.tellg() % sizeof(int16_t) != 0) {
    int8_t dummy{read_into<int8_t>(&terminfo_file)};
  }

  std::map<std::string_view, int32_t> numbers;
  for (int i = 0; i < numbers_size; i++) {
    if (infotype == InfoType::Legacy) {
      int16_t val = read_into<int16_t>(&terminfo_file);
      if (i < names::number_map.size()) {
        numbers.insert({names::number_map[i], val});
      }
    } else {
      int32_t val = read_into<int32_t>(&terminfo_file);
      if (i < names::number_map.size()) {
        numbers.insert({names::number_map[i], val});
      }
    }
  }

  std::vector<int16_t> offsets;
  for (int i = 0; i < offsets_size; i++) {
    auto val = read_into<int16_t>(&terminfo_file);
    offsets.push_back(val);
  }

  std::string strings_table;
  strings_table.resize(strings_size);
  terminfo_file.read(strings_table.data(), strings_table.size());
  std::string_view table_view = strings_table;

  std::map<std::string_view, std::string> strings;
  for (int i = 0; i < offsets.size() && i < names::string_map.size(); i++) {
    if (offsets[i] < 0) continue;

    auto slice = table_view.substr(offsets[i]);
    auto pos = slice.find_first_of('\x00');
    auto s = slice.substr(0, pos);
    strings.insert({names::string_map[i], std::string(s)});
  }

  return TermInfo{std::move(bools), std::move(numbers), std::move(strings)};
}

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
