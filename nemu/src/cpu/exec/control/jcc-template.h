#include "cpu/exec/template-start.h"

#define update_eip()\
	cpu.eip += (int32_t)op_src->val;\
	if(DATA_BYTE == 2) cpu.eip &= 0xffff;

#if DATA_BYTE == 1
#define CODE_LEN 2
#endif
#if DATA_BYTE == 2
#define CODE_LEN 4
#endif
#if DATA_BYTE == 4
#define CODE_LEN 6
#endif

#define instr je

static void do_execute() {
	if(cpu.ZF == 1) update_eip();
	print_asm("je $0x%x", cpu.eip + CODE_LEN);
}

make_instr_helper(i)

#undef instr

#undef CODE_LEN
#include "cpu/exec/template-end.h"