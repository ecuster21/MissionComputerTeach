#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

int main()
{
  // Step 1: Create a 32-byte frame. Every byte starts as 0.
  std::vector<uint8_t> frame(32, 0);

  // Step 2: Fill the first five bytes.
  // TODO: frame[0] should be 0xEB
  frame[0] = 0xEB;
  // TODO: frame[1] should be 0x90
  frame[1] = 0x90;
  // TODO: frame[2] should be 0x0C
  frame[2] = 0x0C;
  // TODO: frame[3] should be 0x01
  frame[3] = 0x01;
  // TODO: frame[4] should be 0x02
  frame[4] = 0x02;

  // Step 3: Print frame_type/source_id/destination_id in hexadecimal.
  // Hint: uint8_t may print as a character, so cast it to int before printing.
  std::cout << std::hex << std::setfill('0');

  // TODO: print frame[2] as frame_type
  std::cout << "Frame Type: 0x" << std::setw(2) << static_cast<int>(frame[2]) << std::endl;
  // TODO: print frame[3] as source_id
  std::cout << "Source ID: 0x" << std::setw(2) << static_cast<int>(frame[3]) << std::endl;
  // TODO: print frame[4] as destination_id
  std::cout << "Destination ID: 0x" << std::setw(2) << static_cast<int>(frame[4]) << std::endl;

  return 0;
}
