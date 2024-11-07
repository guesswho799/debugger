# Debugger (temporary name)
*Sick of IDA's annoying GUI? me 2! meet `Debugger`*

Debugger is a combination of a static and dynamic debugger for vim lovers
## Features
### Loading Elf binaries and viewing specific functions
![image](https://github.com/user-attachments/assets/8e4adb90-5768-4d26-bbc9-8625722604e6)

### Viewing all avaliable symbols
![image](https://github.com/user-attachments/assets/45acc530-4e45-43d2-a78c-ab1cba173c95)

### Running the binary and 'coloring' in flows
![image](https://github.com/user-attachments/assets/1392686d-5a38-4379-a38a-4bc895c6bf05)

## How to build test binary
```console
cd test && g++ -no-pie -g -O0 main.cpp -oprogram
```
## How to build debugger
```console
mkdir build && cd build
cmake .. && cmake --build .
./Debugger ../test/program main
```
