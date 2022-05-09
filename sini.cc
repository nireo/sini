#include "sini.h"
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace sini {

static std::string current_section_ = "global";
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
      while (index < sz && std::isalnum(p[index])) {
        identifier_string += p[index];
        ++index;
      }

      tok_t t{};
      t.data = std::move(identifier_string);
      t.type_ = T_IDENTIFER;
      result_vector.push_back(t);

      continue;
    }

    if (p[index] == '[' || p[index] == ']' || p[index] == '=') {
      tok_t t{};
      t.data = p[index];
      t.type_ = T_PUNCT;
      result_vector.push_back(t);

      ++index;
      continue;
    }

    if (p[index] == '"') {
      tok_t t{};
      t.type_ = T_STRING;
      ++index;

      std::string string_lit;
      while (index < sz && p[index] != '"') {
        string_lit += p[index];
        ++index;
      }
      if (p[index] != '"') {
        // unterminated string.
        break;
      }
      ++index;
      t.data = std::move(string_lit);
      result_vector.push_back(t);

      continue;
    }

    std::cout << "unrecognized token" << p[index] << '\n';
    break; // unrecognized token.
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

static bool match_punct(char c, const tok_list_t &tokens, size_t &pos) {
  if (tokens[pos].type_ != T_PUNCT) {
    return false;
  }

  return c == std::get<char>(tokens[pos].data);
}

status_t ini_t::parse_keyval(const tok_list_t &tokens, size_t &pos) noexcept {
  // we know that the current token is identifier, so that the variant contains
  // a string value.
  auto key = std::move(std::get<std::string>(tokens[pos].data));
  ++pos;

  if (!match_punct('=', tokens, pos)) {
    std::cout << "test3\n";
    return STATUS_BAD_PUNCTUATOR;
  }
  ++pos;
  auto value = tokens[pos];

  sections[current_section_][key] = value;
  return STATUS_OK;
}

status_t ini_t::parse_sect(const tok_list_t &tokens, size_t &pos) noexcept {
  // in the loop we don't check whether the punctuator is [, ] or =
  // so we need to check here.
  if (!match_punct('[', tokens, pos)) {
    std::cout << "test1\n";
    return STATUS_BAD_PUNCTUATOR;
  }
  ++pos;

  // the section name is an identifier, so we need to set the new identifier.
  if (tokens[pos].type_ != T_IDENTIFER) {
    return STATUS_EXPECTED_IDENTIFIER;
  }
  current_section_ = std::get<std::string>(tokens[pos].data);
  sections[current_section_] = section_t{};

  ++pos;
  if (!match_punct(']', tokens, pos)) {
    std::cout << "test2 " << tokens[pos].type_ << '\n';
    return STATUS_BAD_PUNCTUATOR;
  }

  return STATUS_OK;
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

  std::cout << file_output << '\n';

  auto tokens = parse_tokens(file_output);
  size_t pos = 0;

  while (tokens[pos].type_ != T_EOF) {
    // we can either start a section, or assign key-value pairs.
    if (tokens[pos].type_ == T_PUNCT) {
      // we found a section definition
      auto status = parse_sect(tokens, pos);
      if (status != STATUS_OK) {
        return status;
      }
      ++pos;

      continue;
    } else if (tokens[pos].type_ == T_IDENTIFER) {
      // we found a key-value pair.
      auto status = parse_keyval(tokens, pos);
      if (status != STATUS_OK) {
        return status;
      }
      ++pos;

      continue;
    }

    std::cerr << "invalid token" << tokens[pos].type_ << '\n';
    break;
  }

  std::cout << "got " << sections.size() << " sections.\n";

  return STATUS_OK;
}

void ini_t::output_to_stream(std::basic_ostream<char> &stream) const {
  for (const auto &[name, section_data] : sections) {
    stream << '[' << name << ']' << std::endl;
    for (const auto &[k, v] : section_data.keys_) {
      stream << k << " = ";
      if (v.type_ == T_NUMBER) {
        stream << std::get<int64_t>(v.data);
      } else {
        stream << std::get<std::string>(v.data);
      }
      stream << '\n';
    }
    stream << '\n';
  }
}

void ini_t::write_number(const std::string &section, const std::string &name,
                         int64_t value) {
  tok_t t{
      .type_ = T_NUMBER,
      .data = value,
  };

  sections[section][name] = std::move(t);
}

void ini_t::write_str(const std::string &section, const std::string &name,
                      const std::string &value) {
  tok_t t{
      .type_ = T_STRING,
      .data = value,
  };
  sections[section][name] = std::move(t);
}
} // namespace sini
