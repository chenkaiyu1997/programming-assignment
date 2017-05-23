#include "cpu/exec/template-start.h"

#define instr call

static void do_execute() {
    cpu.esp -= 4;
    if(op_src->type == OP_TYPE_IMM){
        swaddr_write(cpu.esp, 4, cpu.eip + DATA_BYTE + 1);
        cpu.eip += op_src->val;
        if(DATA_BYTE == 2) cpu.eip &= 0x0000ffff;
        print_asm("call $0x%x", cpu.eip + DATA_BYTE + 1);
    }
    else{
        swaddr_write(cpu.esp, 4, cpu.eip + 2);
        cpu.eip = (op_src->val) - 2;
        print_asm("call $0x%x", cpu.eip + 2);
    }
}


make_instr_helper(i)
make_instr_helper(rm)

#include "cpu/exec/template-end.h"
