#include "Base64.h"
#include <stdexcept>

// Base64 encoding table
const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Encode a uint64_t value into a Base64 string
std::string Base64::encode(uint64_t input)
{
    unsigned char bytes[8];

    // Break down the uint64_t into bytes
    for (int i = 0; i < 8; ++i)
    {
        bytes[7 - i] = (input >> (i * 8)) & 0xFF;
    }

    std::string encoded;
    int val = 0, valb = -6;

    for (int i = 0; i < 8; ++i)
    {
        val = (val << 8) + bytes[i];
        valb += 8;
        while (valb >= 0)
        {
            encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    while (encoded.size() % 4)
    {
        encoded.push_back('=');
    }

    return encoded;
}

// Decode a Base64 string back into a uint64_t value
uint64_t Base64::decode(const std::string &encoded)
{
    if (encoded.size() != 12)
    { // Check for valid 8-byte Base64 encoding size (should be 12 chars with padding)
        throw std::invalid_argument("Invalid Base64 string length for uint64_t decoding");
    }

    uint64_t result = 0;
    int val = 0, valb = -8;

    for (char c : encoded)
    {
        if (c == '=')
            break;
        auto pos = base64_chars.find(c);
        if (pos == std::string::npos)
            throw std::invalid_argument("Invalid character in Base64 string");

        val = (val << 6) + pos;
        valb += 6;

        if (valb >= 0)
        {
            result = (result << 8) | ((val >> valb) & 0xFF);
            valb -= 8;
        }
    }

    return result;
}
