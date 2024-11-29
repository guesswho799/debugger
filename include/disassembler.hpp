#pragma once

#include <capstone/capstone.h>
#include <inttypes.h>
#include <fstream>
#include <sstream>
#include <vector>

#define MAX_DISASSEMBLE_LINE (120)

namespace Disassebler
{
    struct Line
    {
        std::vector<int> opcodes;
        std::string disassemble;
        uint64_t address;
    };

    inline std::vector<Line> disassemble_raw(uint8_t *input_buffer, size_t input_buffer_size, int base_address)
    {
	std::vector<Line> result;
	csh handle;
	cs_insn *insn;
	if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) throw 1;
	cs_option(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_INTEL);
	ssize_t count = cs_disasm(handle, input_buffer, input_buffer_size, base_address, 0, &insn);
	if (count < 0) throw 2;
	int pc = 0;
	for (int i = 0; i < count; i++)
	{
            std::ostringstream instruction_as_string;
	    instruction_as_string << insn[i].mnemonic << " " << insn[i].op_str;
	    std::vector<int> opcodes;
	    opcodes.insert(opcodes.end(), input_buffer+pc, input_buffer+pc+insn[i].size);
	    result.emplace_back(opcodes, instruction_as_string.str(), insn[i].address);
	    pc += insn[i].size;
	}
	cs_free(insn, count);
	cs_close(&handle);
	return result;
    }
}

