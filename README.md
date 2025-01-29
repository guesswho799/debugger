# Debugger (temporary name)
*Sick of IDA's annoying GUI? me 2! meet `Debugger`*

Debugger is a combination of a static and dynamic debugger,<br>
It displays all information in tui format and is controlled only by simple keyboard shortcuts.
## Features
### Load Elf binaries and view specific functions
![image](https://github.com/user-attachments/assets/ca2c7847-c0ec-478d-89fc-5d5a7647e54a)

### View all avaliable symbols
![image](https://github.com/user-attachments/assets/35086e4b-dc63-4b17-8fb0-7845bc4dbabf)

### Filter dead code by tracing every function
![image](https://github.com/user-attachments/assets/79d1e720-ad1a-4b80-ab52-bf7085a37597)

### Found an interesting function? Debug it step by step
![image](https://github.com/user-attachments/assets/cd11029d-f9ea-4045-909f-6ee777d18e12)


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
