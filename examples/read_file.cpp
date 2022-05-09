#include <iostream>
#include "sini.h"


// This program reads a ini file and then prints it to stdout.
int main() {
  auto ini_file = sini::ini_t();
  ini_file.parse_file("file.ini");
  ini_file.output_to_stream(std::cout);

  return 0;
}
