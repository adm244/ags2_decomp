# AGS 2.x Decompilation

A matching decompilation project of Adventure Game Studio (AGS) 2.x. \
The goal is to decompile all publicly released AGS 2.x versions for software preservation and educational purposes.

This project focuses **only** on runtime engine and does **not** include editor components (`roomedit` for DOS and later `agsedit` for Windows).

The code in this repository is the result of reverse-engineering, source reconstruction and instruction matching processes. Source reconstruction is based on AC 1.14 [roommake](https://archive.org/details/rmakesrc) and AGS [legacy](https://github.com/adventuregamestudio/ags/tree/legacy) sources with main focus on producing source code as close to original as possible.

## Status

Version 2.00 is **complete** and demo game is fully playable. Next milestone would be 2.05 since it's the first version that can be tested against a "real" game: LassiQuest.

For more information refer to [STATUS](./STATUS.md).

## Comparing versions

As more information is gained over time, previously finished versions can be altered slightly. So do **NOT** rely on commit history to get differences between them. Use `git diff --no-index` on source directories instead.

For example, to compare versions 2.00 and 2.01 execute this from project root directory:
```sh
git diff --no-index ags200/src ags201/src
```

## Building

This project is using GNU `make` as its build system. Starting with AGS 2.03, it can be built for two targets: **DOS** (DJGPP) and **Windows** (MSVC).

**Before** you can build, you need to setup the DJGPP environment or a cross-compiler for targeting DOS, and the MSVC environment for targeting Windows. Instructions are provided in [BUILD](./docs/BUILD.MD) documentation.

The instructions below assume that you have the DJGPP and MSVC environments set up.

To build a specific AGS version, first navigate to its folder (e.g. `ags203`) and run `make prepare`:
```sh
cd ags203/ && make prepare
```
This will create necessary folder structure (run `make prepare` only once).

To build using DJGPP environment just invoke `make`:
```sh
make
```

For DJGPP cross-compilation, pass `DJGPP_DIR` variable pointing to cross-compiler root folder (e.g. `~/cross/4.9.4/djgpp`):
```sh
make DJGPP_DIR=~/cross/4.9.4/djgpp
```

For MSVC compilation, pass `MSVC_DIR` variable pointing to MSVC root folder (e.g. `~/cross/msvc/98`):
```sh
make MSVC_DIR=~/cross/msvc/98
```

For more information refer to [BUILD](./docs/BUILD.MD).

## Instruction matching

This process ensures that reconstructed source code, when compiled with a specific compiler version, produces identical to original binary assembly instructions. Matching is done against each function across all object files manually with the help of a simple diffing script.

Keep in mind this does **not** gurantee that output is byte-to-byte identical, since jump instructions may have different targets and global variables are placed at different addresses. Thus, a behavior matching process should follow this step to ensure identical behavior. This project doesn't have a goal of perfecly aligning data and compilation units.

Instruction matching should be done on **host** system since `diff_symbol.sh` script requires relatively modern versions of `bash` and `binutils`, you would also need `cat` (part of textutils), `grep`, `awk`, `diff` and `less` installed.

Make sure you've successfully compiled `ac.exe` (or `acwin.exe`) and `obj` directory contains object files.

Run following command to do matching of function `update_stuff` from `AC.O` compiled for DOS:
```sh
make diff OBJECT=AC.O SYMBOL=update_stuff
```

In some cases there might be more then one function with a similar name, you can specify symbol index to select one of them:
```sh
make diff OBJECT=AC.O SYMBOL=run_animation SYMBOL_INDEX=1
```

For Windows version pass `TOOLCHAIN=msvc` variable into `make`:
```sh
make diff TOOLCHAIN=msvc OBJECT=ac.obj SYMBOL=update_stuff
```

GNU `objdump` has issues disassembling Windows COFF files, so for Windows version `llvm-objdump` is used.

Refer to `orig/ac_symbols.txt` (or `orig/acwin_symbols.txt`) file for the list of all functions that can be matched. This list is obtained through reverse-engineering and exporting tagged functions from Ghidra. Symbols file contains virtual memory addreses (VMAs). To convert to file offset, subtract section VMA and add section file offset.

For information regarding instruction matching and GCC2 specific patterns refer to [GCC2_NOTES](./docs/GCC2_NOTES.md).
