#pragma once

#include <ftxui/component/component.hpp>
#include <capstone/capstone.h>
#include <inttypes.h>
#include <vector>
#include "status.hpp"


namespace Disassembler
{
    struct Line
    {
        std::vector<int> opcodes;
        std::string instruction;
        std::string arguments;
        uint64_t address;
        bool is_jump;
    };

    inline bool is_jump(std::string instruction)
    {
        std::vector<std::string> jump_values{"jmp", "je", "jne", "jg", "jl", "jge", "jle"};
	return std::find(jump_values.begin(), jump_values.end(), instruction) != jump_values.end();
    }

    inline std::vector<Line> disassemble_raw(uint8_t *input_buffer, size_t input_buffer_size, int base_address)
    {
	std::vector<Line> result;
	csh handle;
	cs_insn *insn;
	if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) throw Status::disassembler__open_failed;
	cs_option(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_INTEL);
	ssize_t count = cs_disasm(handle, input_buffer, input_buffer_size, base_address, 0, &insn);
	if (count < 0) throw Status::disassembler__parse_failed;
	int pc = 0;
	for (int i = 0; i < count; i++)
	{
	    std::vector<int> opcodes;
	    opcodes.insert(opcodes.end(), input_buffer+pc, input_buffer+pc+insn[i].size);
	    result.emplace_back(opcodes, insn[i].mnemonic, insn[i].op_str, insn[i].address, is_jump(insn[i].mnemonic));
	    pc += insn[i].size;
	}
	cs_free(insn, count);
	cs_close(&handle);
	return result;
    }
}

