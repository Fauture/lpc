# LPC VM for Windows (C++17)

This project is a from-scratch Windows rewrite of the LPC virtual machine found in
the IDA-loaded `magic_Linux32x` binary. It is designed to be built with Visual
Studio 2022 / CMake.

## Features

- LPC source interpreter for the pseudo-LPC dialect recovered from the original
  game driver (`v2.1` style, plus a useful subset of `v37` style).
- Stack-based LPC virtual machine core (`src/interpreter.cpp`).
- Original compiled `.o` container parser and disassembler (`src/o_loader.cpp`):
  reads the binary format used by the original VM, including the header, string
  pool, identifier table, hidden functions, DLT function table, and code segments.
  `run` currently executes trivial original global segments; full original
  action-bytecode execution is still under development.
- Builtin efuns (stubs plus common helpers) so simple scripts can run.

## Build

Using Visual Studio 2022:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```

Or use the open folder / CMake integration in VS2022.

## Usage

```bat
lpcvm run script.c
lpcvm run script.o
lpcvm disasm script.o
```

`run` accepts either an LPC `.c` source file or a compiled `.o` file.  
`disasm` prints the parsed original `.o` structure and action nodes.

## Layout

- `src/value.*` – LPC values (int/float/string/array/mapping/object/closure)
- `src/lexer.*` – tokenizer
- `src/ast.*` – AST nodes
- `src/parser.*` – LPC parser
- `src/interpreter.*` – tree-walking interpreter / VM
- `src/builtins.*` – builtin efuns
- `src/o_loader.*` – original `.o` binary parser
