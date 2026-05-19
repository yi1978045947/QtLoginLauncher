#pragma once

#include <string>

class CSafeBase64
{
public:
    CSafeBase64(void);
    ~CSafeBase64(void);
    
    static std::string Encode(const std::string &input);
    static std::string Decode(const std::string &input);
};
