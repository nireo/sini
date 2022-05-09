#ifndef _SINI_H
#define _SINI_H

#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace sini {
enum tok_kind_t {
  T_NONE,
  T_IDENTIFER,
  T_NUMBER,
  T_STRING,
  T_PUNCT, // a punctuator i.e. '[' '('
  T_EOF,
};

enum status_t {
  STATUS_OK,
  STATUS_UNTERMINATED_STRING,
  STATUS_EXPECTED_IDENTIFIER,
  STATUS_BAD_PUNCTUATOR,
  STATUS_ERR,
};

struct tok_t {
  template <typename T> tok_t(tok_kind_t ty, T data) : type_(ty), data_(data){};
  tok_t() {
    type_ = T_NONE;
    data_ = std::monostate{};
  };
  tok_kind_t type_ = T_NONE;
  std::variant<std::monostate, std::string, int64_t, char> data_{
      std::monostate{}}; // type safe union
};

using tok_list_t = std::vector<tok_t>;
struct section_t {
  section_t() {}
  tok_t &operator[](const std::string &s) { return keys_[s]; }
  const tok_t &operator[](const std::string &s) const { return keys_.at(s); }
  std::unordered_map<std::string, tok_t>
      keys_; // just use tokens, since it contains the values.
};

// NumberAsString tells the class whether you want numbers represented as
// std::string or int64_t
class ini_t {
public:
  [[nodiscard]] status_t parse_file(const std::string &filename) noexcept {
    std::ifstream f(filename);

    // This way we don't have to rely on the string class' automatic
    // reallocation.
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

  template <typename T>
  [[nodiscard]] std::optional<T> get_value(const std::string &sect,
                                           const std::string &vname) {
    try {
      return std::get<T>(sections[sect][vname]);
    } catch (const std::bad_variant_access &) {
      return std::nullopt;
    }
  }

  template <typename T>
  void bind_value(const std::string &sect, const std::string &vname,
                  T &tobind) {
    try {
      auto val = std::get<T>(sections[sect][vname]);
      tobind = val;
    } catch (const std::bad_variant_access &) {
      return; // don't do anything
    }
  }

  void output_to_stream(std::basic_ostream<char> &stream) const {
    for (const auto &[name, section_data] : sections) {
      stream << '[' << name << ']' << std::endl;
      for (const auto &[k, v] : section_data.keys_) {
        stream << k << " = ";
        if (v.type_ == T_NUMBER) {
          stream << std::get<int64_t>(v.data_);
        } else {
          stream << std::get<std::string>(v.data_);
        }
        stream << '\n';
      }
      stream << '\n';
    }
  }

  void write_number(const std::string &section, const std::string &name,
                    int64_t value) {
    sections[section][name] = tok_t(T_NUMBER, value);
  }
  void write_str(const std::string &section, const std::string &name,
                 const std::string &value) {
    sections[section][name] = tok_t(T_STRING, value);
  }

  [[nodiscard]] bool is_number(const std::string &section,
                               const std::string &name) const {
    return sections.at(section).keys_.at(name).type_ == T_NUMBER;
  };
  [[nodiscard]] bool is_str(const std::string &section,
                            const std::string &name) const {
    return sections.at(section).keys_.at(name).type_ == T_STRING;
  };

private:
  std::vector<tok_t> parse_tokens(std::string_view p) noexcept {
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
        result_vector.push_back(tok_t(T_NUMBER, std::stoll(number_string)));
        continue;
      }
      if (std::isalpha(p[index])) {
        std::string identifier_string = std::string(1, p[index]);
        ++index;
        while (index < sz && std::isalnum(p[index])) {
          identifier_string += p[index];
          ++index;
        }
        result_vector.push_back(
            tok_t(T_IDENTIFER, std::move(identifier_string)));
        continue;
      }
      if (p[index] == '[' || p[index] == ']' || p[index] == '=') {
        result_vector.push_back(tok_t(T_PUNCT, p[index]));
        ++index;
        continue;
      }
      if (p[index] == '"') {
        ++index;
        std::string string_lit;
        while (index < sz && p[index] != '"') {
          string_lit += p[index];
          ++index;
        }
        if (p[index] != '"') {
          break;
        }
        ++index;
        result_vector.push_back(tok_t(T_STRING, string_lit));
        continue;
      }
      break;
    }
    result_vector.push_back(tok_t(T_EOF, std::monostate{}));
    return result_vector;
  }

  static bool match_punct(char c, const tok_list_t &tokens,
                          size_t &pos) noexcept {
    if (tokens[pos].type_ != T_PUNCT) {
      return false;
    }

    return c == std::get<char>(tokens[pos].data_);
  }

  status_t parse_keyval(const tok_list_t &tokens, size_t &pos) noexcept {
    // we know that the current token is identifier, so that the variant
    // contains a string value.
    auto key = std::move(std::get<std::string>(tokens[pos].data_));
    ++pos;

    if (!match_punct('=', tokens, pos)) {
      return STATUS_BAD_PUNCTUATOR;
    }
    ++pos;
    auto value = tokens[pos];
    sections[curr_sec][key] = value;
    return STATUS_OK;
  }

  status_t parse_sect(const tok_list_t &tokens, size_t &pos) noexcept {
    // in the loop we don't check whether the punctuator is [, ] or =
    // so we need to check here.
    if (!match_punct('[', tokens, pos)) {
      return STATUS_BAD_PUNCTUATOR;
    }
    ++pos;

    // the section name is an identifier, so we need to set the new identifier.
    if (tokens[pos].type_ != T_IDENTIFER) {
      return STATUS_EXPECTED_IDENTIFIER;
    }
    curr_sec = std::get<std::string>(tokens[pos].data_);
    sections[curr_sec] = section_t{};
    ++pos;
    if (!match_punct(']', tokens, pos)) {
      return STATUS_BAD_PUNCTUATOR;
    }

    return STATUS_OK;
  }
  std::unordered_map<std::string, section_t> sections;
  std::string curr_sec;
};

}; // namespace sini

#endif
