#pragma once
#include <string>
#include <vector>
#include <stdexcept>

namespace utils {

static const std::string BASE64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

inline std::string base64Encode(const std::vector<uchar>& data) {
    std::string encoded;
    int i = 0;
    unsigned char char3[3], char4[4];
    size_t len = data.size();
    size_t idx = 0;

    while (len--) {
        char3[i++] = data[idx++];
        if (i == 3) {
            char4[0] = (char3[0] & 0xfc) >> 2;
            char4[1] = ((char3[0] & 0x03) << 4) + ((char3[1] & 0xf0) >> 4);
            char4[2] = ((char3[1] & 0x0f) << 2) + ((char3[2] & 0xc0) >> 6);
            char4[3] = char3[2] & 0x3f;
            for (int j = 0; j < 4; j++) encoded += BASE64_CHARS[char4[j]];
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 3; j++) char3[j] = '\0';
        char4[0] = (char3[0] & 0xfc) >> 2;
        char4[1] = ((char3[0] & 0x03) << 4) + ((char3[1] & 0xf0) >> 4);
        char4[2] = ((char3[1] & 0x0f) << 2) + ((char3[2] & 0xc0) >> 6);
        for (int j = 0; j < i + 1; j++) encoded += BASE64_CHARS[char4[j]];
        while (i++ < 3) encoded += '=';
    }
    return encoded;
}

inline std::vector<uchar> base64Decode(const std::string& encoded) {
    size_t len = encoded.size();
    int i = 0;
    unsigned char char4[4], char3[3];
    std::vector<uchar> decoded;

    for (size_t idx = 0; idx < len; idx++) {
        char c = encoded[idx];
        if (c == '=') break;
        size_t pos = BASE64_CHARS.find(c);
        if (pos == std::string::npos) continue;
        char4[i++] = static_cast<unsigned char>(pos);
        if (i == 4) {
            char3[0] = (char4[0] << 2) + ((char4[1] & 0x30) >> 4);
            char3[1] = ((char4[1] & 0x0f) << 4) + ((char4[2] & 0x3c) >> 2);
            char3[2] = ((char4[2] & 0x3) << 6) + char4[3];
            for (int j = 0; j < 3; j++) decoded.push_back(char3[j]);
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 4; j++) char4[j] = 0;
        char3[0] = (char4[0] << 2) + ((char4[1] & 0x30) >> 4);
        char3[1] = ((char4[1] & 0x0f) << 4) + ((char4[2] & 0x3c) >> 2);
        for (int j = 0; j < i - 1; j++) decoded.push_back(char3[j]);
    }
    return decoded;
}

} // namespace utils
