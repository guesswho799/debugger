# Debugger (temporary name)
*Sick of IDA's annoying GUI? me 2! meet `Debugger`*

Debugger is a combination of a static and dynamic debugger for vim lovers
## Features
### Loading Elf binaries and viewing specific functions
![image](https://github.com/user-attachments/assets/b9ed3cb4-a13f-4513-94e4-d05990bc6566)

### Viewing all avaliable symbols
![image](https://github.com/user-attachments/assets/45acc530-4e45-43d2-a78c-ab1cba173c95)

### Running the binary and 'coloring' in flows
![image](https://github.com/user-attachments/assets/1f9dbfce-132e-41de-84db-1754d8b6ad93)

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
