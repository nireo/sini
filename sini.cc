#include "sini.h"
#include <cctype>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#define CT tokens[pos]

namespace sini {
static std::unique_ptr<section_t> curr_sec_{nullptr};

using tok_list_t = std::vector<tok_t>;
static std::vector<tok_t> parse_tokens(std::string_view p) noexcept {
  std::vector<tok_t> result_vector{};
  size_t index = 0;
  auto sz = p.size();

  while (index < sz) {
    if (std::isspace(p[index])) {
      ++index;
      continue;
    }

    if (std::isdigit(p[index])) {
      std::string number_string = std::string(1, p[index]);
      ++index;
      while (index < sz && std::isdigit(p[index])) {
        number_string += p[index];
        ++index;
      }

      tok_t t{};
      t.type_ = T_NUMBER;
      t.data = std::stoll(number_string);
      result_vector.push_back(t);

      continue;
    }

    if (std::isalpha(p[index])) {
      std::string identifier_string = std::string(1, p[index]);
      ++index;
      while (index < sz && std::isalpha(p[index])) {
        identifier_string += p[index];
        ++index;
      }

      tok_t t{};
      t.data = std::move(identifier_string);
      t.type_ = T_IDENTIFER;
      result_vector.push_back(t);

      continue;
    }

    if (p[index] == '[' || p[index] == ']' || p[index] == '.') {
      tok_t t{};
      t.data = p[index];
      t.type_ = T_PUNCT;
      result_vector.push_back(t);

      ++index;
      continue;
    }
  }

  result_vector.push_back(tok_t{.type_ = T_EOF});
  return result_vector;
}

template <typename T>
std::optional<T> ini_t::get_value(const std::string &sect,
                                  const std::string &vname) {
  try {
    return std::get<T>(sections[sect][vname]);
  } catch (const std::bad_variant_access &) {
    return std::nullopt;
  }
}

template <typename T>
void ini_t::bind_value(const std::string &sect, const std::string &vname,
                       T &to_bind) {
  try {
    auto val = std::get<T>(sections[sect][vname]);
    to_bind = val;
  } catch (const std::bad_variant_access &) {
    return; // don't do anything
  }
}

// parse_file fills the ini_t with data from a given file.
status_t ini_t::parse_file(const std::string &filename) {
  std::ifstream f(filename);

  // This way we don't have to rely on the string class' automatic reallocation.
  std::string file_output;
  f.seekg(0, std::ios::end);
  file_output.reserve(f.tellg());
  f.seekg(0, std::ios::beg);

  file_output.assign((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());

  auto tokens = parse_tokens(file_output);
  size_t pos = 0;

  curr_sec_ = std::make_unique<section_t>();

  while (tokens[pos].type_ != T_EOF) {
    // we can either start a section, or assign key-value pairs.
    if (CT.type_ == T_PUNCT) {
    }
  }

  return STATUS_OK;
}

} // namespace sini
