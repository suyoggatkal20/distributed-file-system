#ifndef BASE64_H
#define BASE64_H

#include <string>
#include <cstdint>

class Base64
{
public:
    // Encode a uint64_t value into a Base64 string
    static std::string encode(uint64_t input);

    // Decode a Base64 string back into a uint64_t value
    static uint64_t decode(const std::string &encoded);
};

#endif // BASE64_H
