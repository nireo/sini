#ifndef _SINI_H
#define _SINI_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace sini {

enum tok_kind_t {
  T_NONE,
  T_IDENTIFER,
  T_NUMBER,
  T_STRING,
  T_PUNCT, // a punctuator i.e. '[' '('
};

enum status_t {
  STATUS_OK,
  STATUS_ERR,
};

struct tok_t {
  tok_t &operator=(const tok_t&) = delete;
  tok_t(const tok_t&) = delete;

  tok_kind_t type_ = T_NONE;
  std::variant<std::monostate, std::string, int64_t> data{
      std::monostate{}}; // type safe union
};

struct section_t {
  // TODO: maybe implement a more lightweight map.
  std::unordered_map<std::string, std::unique_ptr<section_t>> subsections_;
  std::unordered_map<std::string, tok_t>
      keys_; // just use tokens, since it contains the values.
};

class ini_t {
public:
  [[nodiscard]] status_t parse_file(std::string_view filename);

private:
  std::unique_ptr<section_t> root_sect_;
};

}; // namespace sini

#endif
