# About

L0 is statically typed, compiled programming language that I started as a hobby project to get into compilers and the LLVM toolchain.
L0 features and characteristics:
    - Strong and static typing
    - Structures
    - Type inference for local variables
    - Immutability by default
    - Functions as first-class citizens, higher-order functions, anonymous functions, closures
    - Pointer semantics and arithmetic
    - Manual memory management

## Examples

This is a Hello World program in L0:

```
fn main () -> ()
{
    print("Hello, World!");
};
```

You can find more examples in [the examples directory](examples).

## Build

To build the project itself, run

```shell
cmake -B build -D CMAKE_CXX_COMPILER=clang++
make -C build
```

## Run
 
```shell
# Generate textual IR representation from L0 files
build/src/l0/main/l0c <file1.l0 file2.l0 ...>

# Compile and link to executable
clang <file1.ll file2.ll ...> -o <output_file>

# E.g. to build and run the faculty example:
cd examples
../build/src/l0/main/l0c "faculty.l0" "math.l0" "print.l0" "read.l0" "string.l0"
clang *.ll -o faculty
./faculty
```
