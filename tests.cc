#include <iostream>
#include "sini.h"

int main() {
  auto ini_file = sini::ini_t();
  auto status = ini_file.parse_file("./test.ini");
  std::cout << "Status: " << status << '\n';

  ini_file.output_to_stream(std::cout);

  return 0;
}
