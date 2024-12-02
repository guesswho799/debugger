#include "disassembler.hpp"
#include "status.hpp"

#include <algorithm>


Disassembler::Disassembler() :
	_handle(_get_handler())
{
    cs_option(_handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_INTEL);
}

Disassembler::~Disassembler()
{
    cs_close(&_handle);
}

csh Disassembler::_get_handler()
{
    csh handle;
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) throw Status::disassemble__open_failed;
    return handle;
}

std::vector<Disassembler::Line> Disassembler::disassemble(uint8_t* input_buffer, size_t input_buffer_size, int base_address)
{
    std::vector<Disassembler::Line> result;
    cs_insn *insn;
    ssize_t count = cs_disasm(_handle, input_buffer, input_buffer_size, base_address, 0, &insn);
    if (count < 0) throw Status::disassemble__parse_failed;
    int pc = 0;
    for (int i = 0; i < count; i++)
    {
        std::vector<int> opcodes;
        opcodes.insert(opcodes.end(), input_buffer+pc, input_buffer+pc+insn[i].size);
        result.emplace_back(opcodes, insn[i].mnemonic, insn[i].op_str, insn[i].address, is_jump(insn[i].mnemonic));
        pc += insn[i].size;
    }
    cs_free(insn, count);
    return result;
}

bool Disassembler::is_jump(std::string instruction)
{
    std::vector<std::string> jump_values{"jmp", "je", "jne", "jg", "jl", "jge", "jle"};
    return std::find(jump_values.begin(), jump_values.end(), instruction) != jump_values.end();
}

