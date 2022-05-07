#ifndef _SINI_H
#define _SINI_H

#include <cstdint>
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
  STATUS_ERR,
};

struct tok_t {
  tok_kind_t type_ = T_NONE;
  std::variant<std::monostate, std::string, int64_t, char> data{
      std::monostate{}}; // type safe union
};

using tok_list_t = std::vector<tok_t>;
struct section_t {
  section_t() {}

  section_t &operator=(const section_t &) = delete;
  section_t(const section_t &) = delete;

  tok_t &operator[](const std::string &s) { return keys_[s]; }

  const tok_t &operator[](const std::string &s) const { return keys_.at(s); }

  std::unordered_map<std::string, tok_t>
      keys_; // just use tokens, since it contains the values.
};

class ini_t {
public:
  [[nodiscard]] status_t parse_file(const std::string &filename);

  template <typename T>
  std::optional<T> get_value(const std::string &sect, const std::string &vname);

  template <typename T>
  void bind_value(const std::string &sect, const std::string &vname, T &tobind);
  void output_to_stream(std::basic_ofstream<char> &stream) const;

  void write_number(const std::string &section, const std::string &name,
                    int64_t);
  void write_str(const std::string &section, const std::string &name,
                    const std::string &value);

private:
  [[nodiscard]] status_t parse_keyval(const tok_list_t &tokens,
                                      size_t &pos) noexcept;
  [[nodiscard]] status_t parse_sect(const tok_list_t &tokens,
                                    size_t &pos) noexcept;

  std::unordered_map<std::string, section_t> sections;
};

}; // namespace sini

#endif
