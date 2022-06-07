# Homebrew_Assembler

This purpose of this project is to develop a retargetable assembler using C++. The goal is to make this assembler configurable so that it useful to a wide array of different homebrew CPU builds. 


## Usage

End-users can configure this assembler to their specific architecture and desired syntax by modifying the following files:

**(1) "Config.h":**<br>
This file defines the syntax used by the assembler. All of the constant expressions in the code block below are modifiable by the end-user (within the limitations stated in the code block comments) and the values entered here determine how the assembler will parse the assembly file. For example, in most assemblers, directives like ***_.org_*** are specific with the ***"."*** directive symbol. By default, this assembler uses the same convention but there is nothing stopping an end-user from having the same directive as ***_*org_*** instead, for example.<br>

As specified in the code block comments below, these user-specified parameters are subject to some limitations. For example, the **DIRECTIVE_SIM** and **SYMBOL_SIM** both are limited to one character assignment while **LABEL_SYM** allows multiple characters. This limitation is imposed by the way it is parsed and failure to follow these instructions will result in unexpected behavior.

```c
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
```

_[Readme in progress...]_
