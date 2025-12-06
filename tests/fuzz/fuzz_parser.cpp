#include "feed/parser.hpp"
#include <cstdint>
#include <cstddef>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) return 0;
    
    std::string_view input(reinterpret_cast<const char*>(data), size);
    
    // Fuzz JSON parser
    auto result = hft::Parser::parse_json(input);
    
    // Fuzz binary parser
    hft::Parser::parse_binary(data, size);
    
    return 0;
}
