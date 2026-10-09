#!/bin/bash

# Takes TARGET_BIN and BASE_OBJ and diffs function SYMBOL
# Used for binary matching

# Original COFF binary (uncompressed)
TARGET_BIN=$1
# file containing symbols for original binary (exported from Ghidra)
# in a form of <function name>;<start address>;<end address>
SYMBOLS_FILE=$2
# Recompiled COFF relocatable object file
BASE_OBJ=$3
# Unmangled function name
SYMBOL=$4
# Index of found symbol, in case there's multiple found
INDEX=$5

OBJDUMP=objdump
DISSYM_OPT=--disassemble

if [[ "${TOOLCHAIN:-}" == "msvc" ]]; then
    # llvm-objdump has more sane disassembly output
    OBJDUMP=llvm-objdump
    DISSYM_OPT=--disassemble-symbols
fi

# objdump common options
OPTIONS="--no-addresses \
         --no-show-raw-insn \
         --disassembler-options=intel \
         --section=.text"
# echo $OPTIONS

# disassemble TARGET_BIN
INPUT=$(cat "$SYMBOLS_FILE" | grep "^$SYMBOL;")
IFS=";" read -r NAME START END <<< "$INPUT"
# echo $NAME $START $END
$OBJDUMP $OPTIONS --start-address=$START --stop-address=$((END+1)) -d $TARGET_BIN > target.txt

# disassemble OBJ
MANGLED_SYMBOLS=($($OBJDUMP -t $BASE_OBJ | grep "$SYMBOL" | awk '{print $NF}'))
# echo "${MANGLED_SYMBOLS[@]}"
$OBJDUMP $OPTIONS $DISSYM_OPT="${MANGLED_SYMBOLS[$INDEX]}" $BASE_OBJ > base.txt

# diff TARGET_BIN and OBJ
diff -dy --color=always target.txt base.txt | less -RN
