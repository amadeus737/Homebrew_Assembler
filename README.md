# Homebrew_Assembler

This purpose of this project is to develop a retargetable assembler using C++. The goal is to make this assembler configurable so that it useful to a wide array of different homebrew CPU builds. 


## Usage

End-users can configure this assembler to their specific architecture and desired syntax by modifying the following files:

**(1) "Config.h":**<br>
This file defines the syntax used by the assembler. All of the constant expressions in the code block below are modifiable by the end-user (within the limitations stated in the code block comments) and the values entered here determine how the assembler will parse the assembly file. For example, in most assemblers, directives like ***_.org_*** are specific with the ***"."*** directive symbol. By default, this assembler uses the same convention but there is nothing stopping an end-user from having the same directive as ***_*org_*** instead, for example.<br>

As specified in the code block comments below, these user-specified parameters are subject to some limitations. For example, the **DIRECTIVE_KEYS**, **SYMBOL_KEYS**, and **LABEL_KEYS** are all allowed multi-character assignment while **BIN_KEY**, **HEX_KEY**, and **DEC_KEY** only allow single character assignment. These fields are named with **__KEY_** and **__KEYS_** subscripts to hopefully make that a little more obvious. Furthermore, the second block of constant expression definitions (for binary, hexadecimal, and decimal keys) only allows for one of the three assignments to be blank. These limitations are imposed by the way the assembler is structured and the way in which it is parsed. Failure to follow these instructions will result to unexpected behavior. This approach was chosen because it greatly simplifies parsing numerical quantities. But hey - source code is here for you if you'd like the assembler to operate in a different way.

```c
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
constexpr const char* ORIGIN_STR = "org";
constexpr const char* EXPORT_STR = "export";
```

_[Readme in progress...]_
