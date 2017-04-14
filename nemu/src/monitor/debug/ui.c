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
	int i;
	printf("dumping register file\n");
	for (i = R_EAX; i <= R_EDI; i++) {
		printf("%%%s: 0x%08x\n", regsl[i], cpu.gpr[i]._32);
	}
	printf("%%eip: 0x%08x\n", cpu.eip);
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
	if (success == 0) {
		printf("%d\n", value);
	}
	else {
		printf("Expression error!");
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
	{ "p", "p [expr] prints the result of the expr", cmd_p}

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
