# Homebrew_Assembler

This purpose of this project is to develop a retargetable assembler using C++. The goal is to make this assembler configurable so that it useful to a wide array of different homebrew CPU builds. 

## Usage and Basic Syntax

This assembler has been made retargetable for several reasons, but one of the main motivations is to support multiple syntax styles. This decision was motivated by my own personal experience learning assembly. In my undergrad days I learned to code using x86-style assembly, but most of the exposure I had to assembly was via 6502-style coding that I did in my spare tinkering time. There are elements of both styles that I really like and I'd like to have a compiler that allows intermixing of the two. Extending that idea a bit further, I felt it might be useful to support any user-specified syntax style. Furthermore, an assembler is really only useful for the hardware that its built for. That was another reason behind the decision to make it retargetable: homebrew CPU designs are extremely varied and in order for this be useful to a wide array (possibly including future builds I may do), a retargetable, configurable assembler like this is really the only way to go.

**Rules**<br>
At present, the assembler is just over 1,000 lines of code. So you must understand that this is not meant to be a robust interpreter that can understand what you're trying to write if you do things just slightly wrong. There are strict rules that the user must follow in order to guarantee expected behavior. 

**(1) One instruction per line**<br>
This one is pretty standard among assemblers, but might be foreign to those coming from high-level languages. I didn't want to make a complicated recursive parser that allows freeform typing so instead the user must limit their code to one instruction per line.

**(2) Whitespace separation between labels, opcodes, and any operands**<br>
Again, pretty standard. The assembler needs to be able to identify what part of the line corresponds to the actual instruction and what part corresponds to arguments that configure that instruction. Note that whitespace includes tab, space, and commas.

**(3) Declarations and definitions in logical order**<br>
Like most languages, this assembler requires that any symbols or labels are defined before they are used. For example, if you intend to use a command like "mov a, Y", where Y is the hexadecimal value 21, you must explicitly define Y before the mov command like so: @Y = $21.

**(4) ASCII space character is replaced with forward slash**<br>
Because the tokenizer in this assembler relies on splitting tokens based on whitespace (comma, tab, or space), any spaces in a string (Ex: "Hello world!") will result in more than one token being created for that string. For that reason, I enforce using a forward slash instead so that it is easily understood by the interpreter as a space. I might remove this limitation in the future, but for now that's where we are. Note that this means forward slashes inside a string will not be processed as expected.

**(5) Lines that start with ';' are treated as comments**<br>
Again, using the semicolon to define a comment line is pretty standard for assembly. This assembler does not support multi-line comment symbols. If you have a comment that needs to go over multiple lines, mark each line with a semicolon at the beginning.'

**(6) Make sure to use the correct case**<br>
This is in-line with the idea of allowing the users to define their own syntax. If you want to support both upper- and lower- case versions of your syntax, you can easily do that via alias definitions.

**(7) Follow convention with specifying arguments**<br>
The assembler only supports a few argument types at present, including: decimal, binary, hexadecimal, and ASCII. Decimal numbers are specified via standard number entry (i.e., no symbols precede the numerical value - ex: 10). Binary values are specified via the '%' operator (ex: %1010). Hexadecimal uses the '$' operator (ex: $0A). Finally, ASCII is specified with open and close quotes (ex: "Hello world!"). Note that single quote character entry is not supported.

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
