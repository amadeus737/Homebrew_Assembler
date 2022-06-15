#pragma once

// Define symbols for interpreting lines (can be multiple characters, if desired)
constexpr const char* DIRECTIVE_KEYS = ".";
constexpr const char* SYMBOL_KEYS = "@";
constexpr const char* LABEL_KEYS = "[]:";

// Define symbols for number types (only one character allowed, only one of these three can be blank)
constexpr const char* BIN_KEY = "%";
constexpr const char* HEX_KEY = "$";
constexpr const char* DEC_KEY = "";

// Define strings for origin and export directives (do not incude DIRECTIVE_KEYS defined above...
// this will be added automatically)
constexpr const char* ARCH_STR = "arch";
constexpr const char* INCLUDE_STR = "include";
constexpr const char* INSERT_STR = "insert";
constexpr const char* ORIGIN_STR = "org";
constexpr const char* EXPORT_STR = "export";
constexpr const char* BYTE_STR = "byte";
constexpr const char* ASCII_STR = "ascii";