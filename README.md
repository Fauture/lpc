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
- Original `.o`/`.pak` execution (`src/original_vm.cpp`): a partial action-bytecode
  VM that can run global initializers and user functions from real `magic_Linux32`
  `.o` files.
- Efun index table extracted from IDA (`src/efun_table.h`): includes both efun
  names and return-type bytes. `call_in_efun` is connected to the implemented
  builtins, so `write/printf/sprintf` etc. can produce real output during `.pak`
  startup.
- `call_other` object loading with a global object registry: loaded `.o` objects are
  cached and reused, so later `call_other` calls can see the same object state.
- `create()` call and a real `while` main-loop/timer stub: if the object defines
  `heart_beat`, the VM runs it repeatedly with a timed interval.
- LZSS decompression (`src/lzss.cpp`) and `.pak` unpacking (`src/pak_loader.cpp`)
  for the `lib_aaa32.pak` / `etc.pak` archives used by the original driver.
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
lpcvm run lib_aaa32.pak
lpcvm server examples\server.c 4000
lpcvm disasm script.o
lpcvm unpack lib_aaa32.pak outdir
```

`run` accepts an LPC `.c` source file, a compiled `.o` file, or a `.pak` archive.
For `.pak`, it extracts the archive, executes `start_aaa.o` through the original
action-bytecode VM, loads the pak's real `.o` objects, prints the startup log, and
then starts a real TCP service on port `8001`.
`server` starts a simple telnet-like LPC server (Winsock TCP).
`disasm` prints the parsed original `.o` structure and action nodes.  
`unpack` decompresses a `.pak` archive into a directory.

Try the live server:

```bat
build\Release\lpcvm.exe server examples\server.c 4000
```

Then connect with telnet/PowerShell to `127.0.0.1:4000` and send:
`hello`, `who`, `time`, `dbdemo`, `help`.

## Layout

- `src/value.*` – LPC values (int/float/string/array/mapping/object/closure)
- `src/lexer.*` – tokenizer
- `src/ast.*` – AST nodes
- `src/parser.*` – LPC parser
- `src/interpreter.*` – tree-walking interpreter / VM
- `src/builtins.*` – builtin efuns
- `src/o_loader.*` – original `.o` binary parser
