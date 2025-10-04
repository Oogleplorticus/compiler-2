# IR Specification 0.1.0

The IR is an SSA (Single static assignment) based IR that uses block arguments as an alternative to phi nodes.

# Structure

## Blocks

Blocks contain instructions.

Blocks may only be jumped to from blocks within the same function.

Blocks can have parameters and take arguments as to allow more complex data flow (Replacement for phi nodes).

## Functions

Functions contain blocks and may be called from any block, even those in other functions.

Functions can have parameters and outputs. Think of an output as a return value.

## Variables

Variables have a numerical ID that is scoped within their containing block. There may be as many variables as needed within any one block.

As per the requirements of SSA, each variable can only be assigned once.
Variables also have a type and size.

## Instructions

There is only one true instruction; calling a function. All instructions and directives are modeled as a function, e.g. an add function.

The functions called can be user defined (as decribed earlier), or defined as part of the specification.

These functions may not neccessarily result in a call, many of the specification functions will be inlined and should be treated as functions in a more abtract sense.

# Variables

## Static and dynamic

Variables can be either static or dynamic. static variables are global, whilst dynamic variables are scoped within their blocks.

The IDs are shared between them, with static variable IDs growing downwards from max, and dynamic variable IDs growing upwards from zero.

## Types

Variables have types to ensure better safety, and to allow more confidence in riskier optimisations.

Variables can have no type, however this is not reccomended as it means losing the advantages above.

There are 4 base types + a pointer type (though pointers are closer to a special case).
3 of the 4 base types can have a variable length, this must be a power of 2.
This variable length may also be set to the word size of the build target, think `usize`.

|Type|ID|Examples|
|-|-|-|
|Signed int|1|`i32` `i64`|
|Unsigned int|2|`u32` `u64`|
|Float|3|`f32` `f64`|
|Boolean|4|`bool`|
|Pointer|N/A|`ptr<i32>`|

# Instructions

It is important to remember that every instruction is like a function call.

Every instruction is represented by an instruction ID. Specification defined instructions/functions grow downwards from max, whilst user defined functions grow upwards from zero. This minimises the chance of ID collisions.

Instruction IDs are `u32`s. Despite being unsigned, the specification defined instructions will be described as negative for convinience.

## Instruction descriptions

Remember directives have the same format as instructions

|ID|Name|Description|
|-|-|-|
|-1|Declare|Declares a variable of a specific type|
|-2|Move|Copies data into a variable from a constant or variable|
|-3|Load|Loads the data at a pointer to a variable|
|-4|Store|Stores the data in a variable to a pointer|
|||
|-128|Add|Adds two numbers|
|-129|Subtract|Subtracts a number from another|
|-130|Multiply|Multiplies two numbers|
|-131|Divide|Divides a number by another and outputs the quotient|
|-132|Remainder|Divides a number by another and outputs the remainder|
|||
|-256|Print|Outputs an array of characters to the appropriate text output|

## Instruction parameters

|ID|Name|Parameters (in order)|
|-|-|-|
|-1|Declare|type:`i8`, typeExp`u8`, ID:`u32`|
|-2|Move|destinationID:`u32`, sourceID:`u32`|
|||
|-256|Print|source:`ptr<u8>`, length:`usize`

# Bytecode Format

### Endianness

The bytecode format is little endian.

### Types

Types are represented by two bytes. one for the type itself (`i8`), and another for the size (`u8`).

The first byte contains either a positive or negative type ID. If the type ID is negative, it is a pointer to that type, if positive, the type itself.

A type ID of 0 is a void pointer, useful for situations where you are pointing to multiple types, or another pointer.

The size byte does not represent the exact size, but instead the size can be found raising two to the power of the size value. This keeps sizes limited to powers of two.
A size byte of all 1s represents the word size of the target system, think `usize`.

In the case of booleans, the size byte still exists, it is just ignored.

### Variables

Variables are represented by a numerical ID `u32`. Variable 0 is a special case as it can be used to discard instruction outputs, and signify a constant input.

When using `%0` to represent a constant input, it will be followed by a type identifier and then the relevant data for the type. In the case the type is `isize` or similar, a further 1 byte will be included to represent the length of the data in the bytecode.

## Header

The header contains the version and data tables, including metadata.

### Magic number

The magic number for the file format is `0x78 0x70 0x62 0xC0` (little endian)

### Version

The version is stored as 3 `u32`s in the order of: major, minor and patch.

### Flags

TODO

### Section table

The section table stores the location of each file section as their offset from the start in bytes. This offset is stored as a `u64`.

Since the sections have a set order, only their offsets are stored, and in the order they appear in the file.

|Section order|
|-|
|Section table (Offset not stored since it never changes)|
|Function table|
|Static variables|
|Program logic|

### Function table

The function table starts with a `u32` representing the number of functions.

Each individual function entry in the function table contains, in the order described:
- A `u32` representing the function ID
- A `u64` representing the length of the function identifier.
- An array of `u8`s representing the function identifier.

## Static variables

The static variable section starts with a `u32` representing the number of static variables.

Each static variable entry starts with its variable ID (`u32`) and type (one `i8` and one `u8`). After that a data count (`u64`) and count instances of the type.

## Program Logic

### Functions

Function definitions start with the function ID (`u32`) and block count (`u64`). After that, two `u32`s, representing the output then input count respectively. Then finally the types of said outputs and inputs in order.

### Blocks

Blocks start with their instruction count (`u64`) and argument count (`u32`). After that the types of said arguments in order. The argument's variable IDs are based on their order. e.g. the 3rd argument is referred to by %3 (variable 0 is a special case).

Blocks are identified by their order within the function. e.g. the 3rd block in the function is block 2 (0 indexed).

The first block inherits the functions input arguments as its own.