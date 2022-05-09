#include "sini.h"
#include <fstream> // for ofstream.

int main() {
  auto ini_file = sini::ini_t();

  // write data.
  ini_file.write_number("number_sect", "a", 512);
  ini_file.write_str("number_sect", "a", "b");

  std::ofstream file_stream("output.ini"); // create new file stream.
  ini_file.output_to_stream(file_stream); // write to a custom stream.
  file_stream.close(); // close file
}
