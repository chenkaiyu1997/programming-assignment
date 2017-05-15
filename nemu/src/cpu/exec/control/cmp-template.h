#include "cpu/exec/template-start.h"

#define instr cmp

static void do_execute () {
	DATA_TYPE result = op_src->val - op_src-> val;
	UPDATE_FLAGS(result) //Update PF ZF SF
	cpu.CF = (op_dest->val > op_src->val) ? 0 : 1;
	cpu.AF = (op_dest->val & 0x7) > (op_src->val & 0x7) ? 0 : 1;
	cpu.OF = (MSB(op_dest->val) != MSB(op_src->val) && MSB(result) != MSB(op_dest->val));
	print_asm_template2();
}

make_instr_helper(rm)
#if DATA_BYTE == 2 || DATA_BYTE == 4
make_instr_helper(r)
#endif

#include "cpu/exec/template-end.h"

#if DATA_BYTE == 2 || DATA_BYTE == 4
make_instr_helper(si2rm)
#endif
make_instr_helper(i2a)
make_instr_helper(i2r)
make_instr_helper(i2rm)
make_instr_helper(r2rm)
make_instr_helper(rm2r)