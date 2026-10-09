# Building

Before you can build for either DOS or Windows, you would need to setup DJGPP and\or MSVC environments.

Build instructions assume **unix**-like OS, if you're on **windows** consider using compatability layers or DJGPP environment method.

## DOS target

To build DOS executables you would need either full [DJGPP environment](https://archive.org/details/DJGPP_Development_System_August_1998) running on DOS or DJGPP [cross-compiler](https://github.com/andrewwutw/build-djgpp). If you're planning doing instruction matching use DJGPP environment, otherwise prefer cross-compilation.

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

## Windows target

To build Windows executables you would need [MSVC++ 6.0 Professional](https://archive.org/download/X04-58170) and [DirectX 6 SDK](https://archive.org/details/directx6sdk).

### Preparing environment

Create MSVC environment root directory (e.g. `~/cross/msvc/98`):
```sh
mkdir ~/cross/msvc/98
```
This directory will hold all necessary binaries and header files for building.

#### Getting MSVC

1. Download and unpack [MSVC6](https://archive.org/download/X04-58170).
2. (Optional) Convert `image/VCP600ENU1.mdf` to iso:
```sh
mdf2iso VCP600ENU1.mdf
```
3. Mount `VCP600ENU1.iso`.
4. Copy `common` and `vc98` directories to your MSVC environment root.
5. Unmount `VCP600ENU1.iso`.

#### Getting DirectX

1. Download [DirectX 6 SDK](https://archive.org/details/directx6sdk).
2. Extract SFX archive to your MSVC environment root:
```sh
unrar x DIRECTX6_SDK.EXE ~/cross/msvc/98
```

### Compiling

Instructions below assume your MSVC root directory is set up and looks like this:
```
common/
DIRX6SDK.98/
vc98/
```

1. Clone or [download](https://github.com/adm244/ags2_decomp/archive/refs/heads/main.zip) this repository:
```sh
git clone https://github.com/adm244/ags2_decomp.git
```
2. Navigate into AGS version directory you wish to compile (e.g. `ags203`):
```batch
cd ags2_decomp/ags203
```
3. Run GNU make to create build directory structure (needed only once):
```sh
make prepare
```
4. Run GNU make to compile. Point `MSVC_DIR` to your MSVC root directory (e.g. `~/cross/msvc/98`):
```sh
make MSVC_DIR=~/cross/msvc/98
```
If build was successfull you will find compiled binaries in `bin` directory (e.g. `ags203/bin`).

To remove all executables and object files:
```sh
make clean
```
OR to also remove compiled dependencies:
```sh
make distclean
```

#### Wine

By default, build system relies on `wine` to run Windows programs. But you can change it by passing `WINE` variable to `make`:
```sh
make WINE=<runner> MSVC_DIR=~/cross/msvc/98
```

You can also bypass `wine` and run binaries directly:
```sh
make WINE= MSVC_DIR=~/cross/msvc/98
```
