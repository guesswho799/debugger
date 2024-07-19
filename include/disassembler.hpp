#pragma once

#include <cstring>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <dis-asm.h>
#include <vector>

#define MAX_DISASSEMBLE_LINE (120)

namespace Disassebler
{
    static char global_line_buffer[MAX_DISASSEMBLE_LINE]{};
    static int current_index = 0;

    typedef struct {
    char *insn_buffer;
    bool reenter;
    } stream_state;

    struct Line
    {
        std::vector<int> opcodes;
        std::string disassemble;
        int address;
    };

    static int dis_fprintf(void *stream, const char *fmt, ...)
    {
        stream_state *ss = (stream_state *)stream;

        va_list arg;
        va_start(arg, fmt);
        if (!ss->reenter) {
            vasprintf(&ss->insn_buffer, fmt, arg);
            ss->reenter = true;
        } else {
            char *tmp;
            vasprintf(&tmp, fmt, arg);

            char *tmp2;
            asprintf(&tmp2, "%s%s", ss->insn_buffer, tmp);
            free(ss->insn_buffer);
            free(tmp);
            ss->insn_buffer = tmp2;
        }
        va_end(arg);

        return 0;
    }

    static int fprintf_styled(void *, enum disassembler_style, const char* fmt, ...)
    {
        va_list args;
        int size;

        va_start(args, fmt);
        size = vsprintf(global_line_buffer+current_index, fmt, args);
        current_index += size;
        va_end(args);

        return size;
    }

    inline std::vector<Line> disassemble_raw(uint8_t *input_buffer, size_t input_buffer_size, int base_address)
    {
        std::vector<Line> output{};
        stream_state ss = {};

        disassemble_info disasm_info = {};
        init_disassemble_info(&disasm_info, &ss, dis_fprintf, fprintf_styled);
        disasm_info.arch = bfd_arch_i386;
        disasm_info.mach = bfd_mach_x86_64;
        disasm_info.buffer = input_buffer;
        disasm_info.buffer_vma = 0;
        disasm_info.buffer_length = input_buffer_size;
        disassemble_init_for_target(&disasm_info);

        disassembler_ftype disasm;
        disasm = disassembler(bfd_arch_i386, false, bfd_mach_x86_64, NULL);

        size_t pc = 0;
        while (pc < input_buffer_size)
        {
            // Generate disassembled string in global_line_buffer
            size_t insn_size = disasm(pc, &disasm_info);

            // Generate line
            std::vector<int> opcodes{};
            opcodes.insert(opcodes.end(), input_buffer+pc, input_buffer+pc+insn_size);
            const Line line {
                .opcodes{opcodes},
                .disassemble = global_line_buffer,
                .address  = base_address + pc
            };
            pc += insn_size;

            output.push_back(line);

            // Clear disassembled string in global_line_buffer
            std::memset(global_line_buffer, 0, MAX_DISASSEMBLE_LINE);
            current_index = 0;
        }

        return output;
    }
}
