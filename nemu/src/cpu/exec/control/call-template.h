#include "cpu/exec/template-start.h"

#define instr call

static void do_execute() {
	uint32_t shift = (-1) << (DATA_BYTE * 8 - 1);
	shift <<= 1;
	shift = ~shift;
	cpu.eip &= shift;
	cpu.esp -= 4;
	swaddr_write(cpu.esp, 4, cpu.eip + DATA_BYTE + 1);
	if(op_src->type == OP_TYPE_IMM)
		cpu.eip += op_src->val;
	else
		cpu.eip = (op_src->val & shift) - (DATA_BYTE + 1);
    print_asm("call $0x%x", cpu.eip + DATA_BYTE + 1);
}


make_instr_helper(i)
make_instr_helper(rm)

#include "cpu/exec/template-end.h"