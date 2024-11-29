# Debugger (temporary name)
*Sick of IDA's annoying GUI? me 2! meet `Debugger`*

Debugger is a combination of a static and dynamic debugger for vim lovers
## Features
### Loading Elf binaries and viewing specific functions
![image](https://github.com/user-attachments/assets/deef2d3f-3ba1-49fe-9dbd-ad3fffd3f043)

### Viewing all avaliable symbols
![image](https://github.com/user-attachments/assets/cd470a33-a297-475d-b076-192a751c98fb)

### Running the binary and 'coloring' in flows
![image](https://github.com/user-attachments/assets/9abbd81b-7445-4238-8759-e5268e3755a7)

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
