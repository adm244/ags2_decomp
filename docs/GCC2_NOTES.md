# GCC 2 instruction macthing notes

These notes describe GCC 2 compiler and standard libs behavior. Might be specific to DJGPP.

## General notes

* If assembly code does the same thing as C code but doesn't match target exactly then check your data types.

* If `fopen` is called with mode `t` then read functions like `fgets` will swallow `\r` symbol (convert windows new line style to unix).

* If padding NOPs are different, but code is the same, check jump destinations and control-flow.

* Marking function `inline` will force compiler to save `ebx` register.

* Local `static` and global variables affect code generation for function that's using them.

* 2-bytes NO-OP `mov esi,esi` has side-effects (changes flags), `lea esi,[esi]` has the same size and no side-effects. Apparently GCC 2.x at some point used unsafe version and later fixed it.

* Compiler will align instructions to 4-byte boundary if it's a jump target. With that said, sometimes compiler misaligns code as a result of alignment calculation bug or something. In this case NOP instructions might not match.

* Stack alignment must be done according to Ghidra's analysis, but keep in mind that Ghidra offsets addresses by 4 bytes. First match stack size, then focus on variables order.

* Stack size does not always correspond to local variables count. Compiler might reuse stack space if variable goes out of scope. This might hint to which scope variable belongs to.

## Continue and Break

```c
for (...) { // <-- 3) and only then here
    if (...) {
        ...
        continue; // <-- 1) this will jump
    }
    ...
    break; // <-- will jump at the end
} // <-- 2) here first
```

## Compound statement

```c
int var = <...>;
if (var!='[') { <...> }
```
```asm
cmp DWORD PTR [ebp-0xc],0x5b
jne <...>
```

VS

```c
if ((var=<...>)!='[') { <...> }
```
```asm
mov eax,DWORD PTR [ebp-0xc]
cmp eax,0x5b
jne <...>
```

## Pointer vs Array types

Pointer and array types are treated differently by the compiler.

```asm
mov edx,0x244               ; <-- array type (int p[n])
mov edx,DWORD PTR ds:0x244  ; <-- pointer type (int *p)
```

This also affects instructions order when accessing member types.

## Signedness

```asm
movzx eax,dx  ; <-- dx is unsigned
movsx eax,dx  ; <-- dx is signed
```

## Empty if-else statements

Empty `if` case (`cmp, j<cond>, jmp`):

```c
if (...) ; // <-- generates empty if case
else ...;
```

## Extra jump instructions

* Infinite `for` and `while` loops are different:

```c
for (;;) { /*<...>*/ } // <-- doesn't generate extra jumps

while (1) { /*<...>*/ }     // <--\
                            //     |-- both generate extra jumps
do { /*<...>*/ } while (1); // <--/
```

* Unreachable `jmp` instruction:

```c
for\while (...) {
    if (...) {
        ...
        continue; // <-- generates first jump
    } // <-- generates second (unreachable) jump
    else {
        ...
    }
}
```

```c
for(;;) {
    if (...) goto cleanup;
    return; // <-- generates first jump
} // <-- generates second (unreachable) jump
cleanup:
    return;
```

* Using a `goto` also generates extra jumps (although sometimes it doesn't).

* There might be the case when compiler decides to generate extra jumps even for an empty function. It can be solved by either moving function implementation below **all** its call sites or moving it to a different compilation unit. This looks like a bug in the compiler code generation logic.
