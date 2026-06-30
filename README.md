# AGS 2.x Decompilation

A matching decompilation project of Adventure Game Studio (AGS) 2.x. \
The goal is to decompile all publicly released AGS 2.x versions for software preservation and educational purposes.

This project focuses **only** on runtime engine and does **not** include editor components (`roomedit` for DOS and later `agsedit` for Windows).

The code in this repository is the result of reverse-engineering, source reconstruction and instruction matching processes. Source reconstruction is based on AC 1.14 [roommake](https://archive.org/details/rmakesrc) and AGS [legacy](https://github.com/adventuregamestudio/ags/tree/legacy) sources with main focus on producing source code as close to original as possible.

## Status

Version 2.00 is **complete** and demo game is fully playable. Next milestone would be 2.05 since it's the first version that can be tested against a "real" game: LassiQuest.

For more information refer to [STATUS](./STATUS.md).

## Building

To build DOS executables you would need either full [DJGPP environment](https://archive.org/details/DJGPP_Development_System_August_1998) running on DOS or DJGPP [cross-compiler](https://github.com/andrewwutw/build-djgpp). If you're planning doing instruction matching use DJGPP environment, otherwise prefer cross-compilation.

Build instructions assume **unix**-like OS, if you're on **windows** consider using compatability layers or DJGPP environment method.

### Cross-compilation

> [!NOTE]
> Cross-compilation tested **ONLY** with GCC 4.9.4.

1. Download and unpack DJGPP [cross-compiler](https://github.com/andrewwutw/build-djgpp/releases/tag/v2.6).
2. Clone or [download](https://github.com/adm244/ags2_decomp/archive/refs/heads/main.zip) this repository:
```sh
git clone https://github.com/adm244/ags2_decomp.git
```
3. Navigate into AGS version directory you wish to compile (e.g. `ags200`):
```sh
cd ags2_decomp/ags200
```
4. Run GNU make to create build directory structure (needed only once):
```sh
make prepare
```
5. Run GNU make to compile. Point `DJGPP_DIR` to cross-compiler root directory (where `setenv` is located) (e.g. `~/cross/4.9.4/djgpp`):
```sh
make DJGPP_DIR=~/cross/4.9.4/djgpp
```

If build was successfull you will find compiled binaries in `bin` directory (e.g. `ags200/bin`).

To remove all executables and object files:
```sh
make clean
```
OR to also remove compiled dependencies:
```sh
make distclean
```

### DJGPP Environment

This method allows to use the same compiler version used to produce original binary, which is required for instruction matching. If you don't care about it -- prefer cross-compilation.

Although you can use any DJGPP version, these instructions assume you're using [this](https://archive.org/details/DJGPP_Development_System_August_1998) CD ISO from 1998 that match closely original environment.

#### Preparing environment

1. Create a directory that will contain DJGPP environment (e.g. `~/djgpp-env`) and navigate to it.
2. Create `DJGPP` directory and unpack `v2/djdev201.zip` into it.
3. Unpack following packages into `~/djgpp-env/DJGPP` (in this order):
> [!WARNING]
> Beware, some packages may contain directory names in UPPERCASE.
```sh
v2gnu/bnu27b.zip    # GNU binutils 2.7 (do NOT use 2.8.1)
v2gnu/fil316b.zip   # GNU fileutils 3.16
v2gnu/gcc281b.zip   # GNU gcc 2.8.1
v2gnu/gdb416b.zip   # GNU debugger 4.16 (optional)
v2gnu/gpp281b.zip   # GNU g++ 2.8.1 (unpack with replace)
v2gnu/lgp2811b.zip  # GNU libg++ 2.8.1.1
v2gnu/mak3761b.zip  # GNU make 3.76.1
v2misc/mlp107b.zip  # ML's djp 1.07 (optional)
# if you need DPMI unpack this also:
v2misc/csdpmi3b.zip # CS's DPMI 3 (names in UPPERCASE)
```
4. Make sure `DJGPP/djgpp.env` contains `+LFN=n` to disable LFN support.

At this point you should have a complete DJGPP environment in `~/djgpp-env` ready for compilation step.

5. Clone or [download](https://github.com/adm244/ags2_decomp/archive/refs/heads/main.zip) this repository into `~/djgpp-env`:
```sh
git clone https://github.com/adm244/ags2_decomp.git
```
6. Rename `ags2_decomp` to `agsdec` (for 8.3 compliance, optional):
```sh
mv ags2_decomp/ agsdec/
```

If you're planning to use DOSBOX ([DOSBOX-X](https://dosbox-x.com/) is recommended) create `dosbox.conf` in DJGPP environment directory with following content:
```batch
[autoexec]
@SET PATH=C:\DJGPP\BIN;%PATH%
@SET DJGPP=C:\DJGPP\DJGPP.ENV
```
> [!NOTE]
> You can also mount any host directory with `MOUNT <DRIVE> <PATH_ON_HOST>` command in case you want to keep repo files separately.

7. Launch DOSBOX from `~/djgpp-env`.

#### Compiling

Instructions below assume you've mounted DJGPP environment directory as C drive and contents of this repo is located at `C:\AGSDEC`.

1. Navigate into AGS version directory you wish to compile (e.g. `AGS200`):
```batch
cd AGSDEC\AGS200
```
2. Run GNU make to create build directory structure (needed only once):
```sh
make prepare
```
3. Run GNU make to compile:
```sh
make
```

If build was successfull you will find compiled binaries in `BIN` directory (e.g. `AGS200\BIN`).

## Instruction matching

This process ensures that reconstructed source code, when compiled with a specific compiler version, produces identical to original binary assembly instructions. Matching is done against each function across all object files manually with the help of a simple diffing script.

Keep in mind this does **not** gurantee that output is byte-to-byte identical, since jump instructions may have different targets and global variables are placed at different addresses. Thus, a behavior matching process should follow this step to ensure identical behavior.

Instruction matching should be done on **host** system since `diff_symbol.sh` script requires relatively modern versions of `bash` and `binutils`, you would also need `cat` (part of textutils), `grep`, `awk`, `diff` and `less` installed.

Make sure you've successfully compiled `ac.exe` and `obj` directory contains object files.

Run following command to do matching of function `update_stuff` from `AC.O`:
```sh
make diff OBJECT=AC.O SYMBOL=update_stuff
```

In some cases there might be more then one function with a similar name, you can specify symbol index to select one of them:
```sh
make diff OBJECT=AC.O SYMBOL=run_animation SYMBOL_INDEX=1
```

Refer to `orig/ac_symbols.txt` file for the list of all functions that can be matched. This list is obtained through reverse-engineering and exporting tagged functions from Ghidra.

For information regarding instruction matching and GCC2 specific patterns refer to [GCC2_NOTES](./docs/GCC2_NOTES.md).
