#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>


constexpr std::size_t FRAME_TYPE_OFFSET = 2;
constexpr std::size_t FRAME_SOURCE_ID_OFFSET = 3;
constexpr std::size_t FRAME_DESTINATION_ID_OFFSET = 4;

uint8_t frame_type(const std::vector<uint8_t>& frame){
    return frame[FRAME_TYPE_OFFSET];
}

uint8_t source_id(const std::vector<uint8_t>& frame){
    return frame[FRAME_SOURCE_ID_OFFSET];
}

uint8_t destination_id(const std::vector<uint8_t>& frame){
    return frame[FRAME_DESTINATION_ID_OFFSET];
}

int main()
{
    std::vector<uint8_t> frame(32,0);
    frame[FRAME_TYPE_OFFSET] = 0x0C;
    frame[FRAME_SOURCE_ID_OFFSET] = 0x01;
    frame[FRAME_DESTINATION_ID_OFFSET] = 0x02;
    uint8_t type = frame_type(frame);
    std::cout << "Frame Type: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(type) << std::endl;

    uint8_t source = source_id(frame);
    std::cout << "Source ID: 0x"
              << std::hex
              << std::setw(2)
              << std::setfill('0')
              << static_cast<int>(source)
              << std::endl;
    
    uint8_t destination = destination_id(frame);
    std::cout << "Destination ID: 0x"
              << std::hex
              << std::setw(2)
              << std::setfill('0')
              << static_cast<int>(destination)
              << std::endl;

    return 0;
}