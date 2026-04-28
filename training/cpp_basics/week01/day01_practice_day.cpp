#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>

int main(){
    std::vector<uint8_t> frame(32, 0);
    frame[0] = 0xEB;
    frame[1] = 0x90;
    frame[2] = 0x0C;
    frame[3] = 0x01;
    frame[4] = 0x02;

    std::cout << std::hex
              << std::setfill('0')
              << "Frame Type: 0x" << std::setw(2) << static_cast<int>(frame[2]) << std::endl
              << "Source ID: 0x" << std::setw(2) << static_cast<int>(frame[3]) << std::endl
              << "Destination ID: 0x" << std::setw(2) << static_cast<int>(frame[4]) << std::endl;

    return 0;



}