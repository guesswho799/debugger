# Debugger (temporary name)
*Sick of IDA's annoying GUI? me 2! meet `Debugger`*

Debugger is a combination of a static and dynamic debugger,<br>
It displays all information in tui format and is controlled only by simple keyboard shortcuts.
## Features
### Load Elf binaries and view specific functions
![image](https://github.com/user-attachments/assets/9bffd902-7c3a-4930-a273-468b58ae7ef4)

### View all avaliable symbols
![image](https://github.com/user-attachments/assets/5e6b6eb5-f232-49cb-a50b-15c401919066)

### Find relevant function by tracing every function
![image](https://github.com/user-attachments/assets/f1389539-cd0b-414f-b5b9-d610aed227d6)

### Found your function? Debug it step by step
![image](https://github.com/user-attachments/assets/af37ac9c-7fe8-4605-9703-21279acef949)


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
## How to build with Docker
```console
sudo docker run --rm -it --entrypoint /bin/bash -v.:/home/ubuntu --network=host ubuntu
apt install cmake build-essential git
goto `How to build debugger`
```

### Dependencies
* Capstone for disassembling
* FTXUI for tui
