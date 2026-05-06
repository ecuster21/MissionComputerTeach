#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstdint>
#include <string>

std::string format_frame_hex(const std::vector<uint8_t> & frame) {
    //使用循环遍历数组打印
    std::ostringstream stream;
    for(size_t i = 0; i < frame.size(); ++i) {
        if(i != 0) {
            stream << " "; //在每个字节之间添加空格
        }
        stream << std::hex << std::setw(2) << std::setfill('0') <<static_cast<int>(frame[i]);//将字节转换为两位十六进制数，并添加到流中
    }
    return stream.str(); //返回格式化后的字符串
}


int main(){
    std::vector<uint8_t> frame(32, 0);
    frame = {0xEB, 0x90, 0x0C, 0x01, 0x02} ;
    std::string result = format_frame_hex(frame);
    std::cout << "Formatted Frame: " << result << std::endl;
    return 0;
}