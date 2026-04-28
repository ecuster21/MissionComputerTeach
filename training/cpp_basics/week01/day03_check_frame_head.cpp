#include <iomanip>
#include <iostream>
#include <vector>
#include <cstdint>

constexpr std::size_t FIRST_BYTE = 0;
constexpr std::size_t SECOND_BYTE = 1;

bool has_valid_header(const std::vector<uint8_t>& frame){
    if(frame.size() < 2){
        return false;
    }
    if(frame[FIRST_BYTE] == 0xEB && frame[SECOND_BYTE] == 0x90){
        return true;
    }
    return false;
}

void test_right_header(){
    std::vector<uint8_t> frame(32,0);
    frame[FIRST_BYTE] = 0xEB;
    frame[SECOND_BYTE] = 0x90;
    if(has_valid_header(frame)){
        std::cout << "Right header test passed." << std::endl;
    }else{
        std::cout << "Right header test failed." << std::endl;
    }
}
void test_wrong_header(){
    std::vector<uint8_t> frame(32,0);
    frame[FIRST_BYTE] = 0x00;
    frame[SECOND_BYTE] = 0x00;
    if(!has_valid_header(frame)){
        std::cout << "Wrong header test passed." << std::endl;
    }else{
        std::cout << "Wrong header test failed." << std::endl;
    }
}

void test_empty_frame(){
    std::vector<uint8_t> frame;
    frame.clear();
    if(!has_valid_header(frame)){
        std::cout << "Empty frame test passed." << std::endl;
    }else{
        std::cout << "Empty frame test failed." << std::endl;
    }
}


int main(){
    test_right_header();
    test_wrong_header();
    test_empty_frame();

    return 0;
}