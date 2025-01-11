# Debugger (temporary name)
*Sick of IDA's annoying GUI? me 2! meet `Debugger`*

Debugger is a combination of a static and dynamic debugger for vim lovers
## Features
### Loading Elf binaries and viewing specific functions
![image](https://github.com/user-attachments/assets/8f5969f5-4abb-4977-90f1-53cb2ea7f77f)

### Viewing all avaliable symbols
![image](https://github.com/user-attachments/assets/5e6b6eb5-f232-49cb-a50b-15c401919066)

### View runtime information
![image](https://github.com/user-attachments/assets/af37ac9c-7fe8-4605-9703-21279acef949)

### Or viewing every function that ran including runtime argumets
![image](https://github.com/user-attachments/assets/3ca83d21-9d83-4f3c-8582-fd50ad800019)

## How to build test binary
```console
g++ -no-pie main.cpp -oprogram
```
## How to build debugger
```console
mkdir build && cd build
cmake .. && cmake --build .
./Debugger ../test/program main
```
