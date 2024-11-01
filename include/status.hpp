#pragma once
#include <exception>
#include <string>
#include <iostream>


enum class Status: int
{
    success = 0,
    elf_header__open_failed,
    elf_header__function_not_found,
    elf_header__section_not_found,
    elf_runner__fork_failed,
    elf_runner__wait_failed,
    elf_runner__child_died,
    elf_runner__child_finished,
};


class CriticalException : public std::exception { 
public: 
    CriticalException(Status msg) 
        : status(msg) 
    {} 

    const char* what() const throw() 
    { 
        std::string output = std::to_string(static_cast<int>(status));
        std::cout << static_cast<int>(status);
        return "";
    } 

private: 
    Status status;
}; 
