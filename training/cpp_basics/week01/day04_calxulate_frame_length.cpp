#include <iostream>
#include <cstddef>

constexpr std::size_t FRAME_HEADER_LENGTH = 2;
constexpr std::size_t FRAME_TYPE_LENGTH = 1;
constexpr std::size_t SOURCE_ID_LENGTH = 1;
constexpr std::size_t DESTINATION_ID_LENGTH = 1;
constexpr std::size_t TIMESTAMP_LENGTH = 1;
constexpr std::size_t CRC8_LENGTH = 1;

std::size_t minimum_frame_length()
{
    return FRAME_HEADER_LENGTH +
            FRAME_TYPE_LENGTH + 
            SOURCE_ID_LENGTH +
            DESTINATION_ID_LENGTH +
            TIMESTAMP_LENGTH +
            CRC8_LENGTH;
}

std::size_t data_area_length_for(std::size_t frame_length){
    return frame_length - minimum_frame_length();
}

std::size_t protocol_timestamp_offset_for(std::size_t frame_length){
    return frame_length - TIMESTAMP_LENGTH -CRC8_LENGTH;
}

int main(){
    std::size_t minimum  = minimum_frame_length();
    std::cout << "Minimum frame Length: " << minimum << " bytes." << std::endl;

    std::size_t frame_length = 32;
    std::size_t data = data_area_length_for(frame_length);
    std::cout << "Data area Length for frame Length: " << data << " bytes." << std::endl; 

    std::size_t timestamp_offset = protocol_timestamp_offset_for(frame_length);
    std::cout << "Protocol timestamp offset for frame Length: " << timestamp_offset << " bytes." << std::endl;
    return 0;
}