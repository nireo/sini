#include <iostream>
#include "sini.h"

int main() {
  auto ini_file = sini::ini_t();
  auto status = ini_file.parse_file("./test.ini");
  std::cout << "Status: " << status << '\n';

  ini_file.output_to_stream(std::cout);

  auto write_file = sini::ini_t();
  write_file.write_str("test_sect1", "hello", "world");
  write_file.write_str("test_sectr2", "hello", "world");

  write_file.output_to_stream(std::cout);

  return 0;
}
