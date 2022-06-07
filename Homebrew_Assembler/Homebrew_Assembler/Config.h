#ifndef CONFIG_H
#define CONFIG_H
#endif

#include <string>

// Define symbols for interpreting lines (only one character is allowed!)
constexpr const char* DIRECTIVE_SYM = ".";
constexpr const char* SYMBOL_SYM = "@";

// Multiple characters allowed!
constexpr const char* LABEL_SYM = "[]:";

// Define symbols for number types (only one can be blank)
constexpr const char* BIN_SYM = "%";
constexpr const char* HEX_SYM = "$";
constexpr const char* DEC_SYM = "";

// Define strings for 
constexpr const char* ORIGIN_STR = "org";
constexpr const char* EXPORT_STR = "export";