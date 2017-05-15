#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the ``readline'' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {
	int instr_num;
	sscanf(args, "%d", &instr_num);
	printf("excuting %d steps\n", instr_num);
	cpu_exec(instr_num);
	return 0;
}

static int cmd_info(char *args) {
	char ch = 0;
	sscanf(args, "%c", &ch);
	if(ch == 'r') {
		int i;
		printf("dumping register file\n");
		for (i = R_EAX; i <= R_EDI; i++) {
			printf("%%%s: 0x%08x\n", regsl[i], cpu.gpr[i]._32);
		}
		printf("eip\t0x%x\t%d\n",cpu.eip,cpu.eip);
		printf("eflags\t0x%8x\t%d\n",cpu.eflags, cpu.eflags);
		printf("CF\t%x\n",cpu.CF);
		printf("PF\t%x\n",cpu.PF);
		printf("ZF\t%x\n",cpu.ZF);
		printf("SF\t%x\n",cpu.SF);
		printf("IF\t%x\n",cpu.IF);
		printf("DF\t%x\n",cpu.DF);
		printf("OF\t%x\n",cpu.OF);
	}
	else if(ch == 'w') {
		print_wp();
	}
	else {
		printf("Command error!");
	}
	return 0;
}

static int cmd_x(char *args) {
	int i, len, pos;
	unsigned tmp = 0;
	sscanf(args, "%d 0%*c%x", &len, &pos);
	printf("dumping %d values from memory starting at 0x%08x\n", len, pos);
	len *= 4;
	for (i = 0; i < len; i++) {
		Assert(pos < HW_MEM_SIZE, "physical address(0x%08x) is out of bound", pos);
		tmp = (tmp << 8) + (*(unsigned char *)hwa_to_va(pos + i));
		if (i % 4 == 3) {
			printf("0x%08x\n", tmp);
			tmp = 0;
		}
	}
	return 0;
}

static int cmd_p(char *args) {
	bool success = true;
	int value = expr(args, &success);
	if (success == true) {
		printf("%d\n", value);
	}
	else {
		printf("Expression error!\n");
	}
	return 0;
}

static int cmd_d(char *args) {
	int num = -1;
	sscanf(args, "%d", &num);
	if(num < 0 || num >= 32) {
		printf("Number Error!");
		return 0;
	}
	delete_wp(num);
	return 0;
}

static int cmd_w(char *args) {
	new_wp(args);
	return 0;
}
bool get_fun(uint32_t, char*);
static int cmd_bt(char *args){
    if(args != NULL)
        printf("(there is no need to input any arguments)\n");
    uint32_t tmp = cpu.ebp;
    uint32_t addr = cpu.eip;
    char name[32];
    int i = 0, j;
    while(get_fun(addr, name)){
        name[31] = '\0';
        printf("#%02d  %08x in %s(",i++, addr, name);
        for(j = 2; j < 6; ++j){
            if(tmp + j * 4 > 0 && tmp + j * 4 < 0x8000000)
                printf("%d, ", swaddr_read(tmp + j*4, 4));
        }
        if(tmp + j * 4 > 0 && tmp + j * 4 < 0x8000000)
            printf("%d", swaddr_read(tmp + j * 4, 4));
        printf(")\n");
        addr = swaddr_read(tmp + 4, 4);
        tmp = swaddr_read(tmp, 4);
    }
    return 0;
}

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "si [num] means excute num steps", cmd_si},
	{ "info", "info r means print the register file", cmd_info},
	{ "x", "x [num] [pos] prints the num values start from pos in the memory", cmd_x},
	{ "p", "p [expr] prints the result of the expr", cmd_p},
	{ "w", "w [expr] creates a watchpoint", cmd_w},
	{ "d", "d [num] deletes watchpoint NO.[num]", cmd_d},
    { "bt", "print backtrace of all stack frames.", cmd_bt}

	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))


static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
