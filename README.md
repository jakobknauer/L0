# Compile

```shell
cmake -B build -D CMAKE_CXX_COMPILER=clang++
make -C build
```

# Run
 
```shell
# Generate textual IR representation from L0 files
build/src/l0/main/main <file1.l0 file2.l0 ...>

# Compile and link to executable
clang <file1.ll file2.ll ...> -o <output_file>
```


# Example
```shell
cd examples

../build/src/l0/main/main "faculty.l0" "math.l0" "print.l0" "read.l0" "string.l0"
clang *.ll -o faculty

# Run
./faculty
```
