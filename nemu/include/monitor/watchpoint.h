#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"
#include "cpu/reg.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	char expr_string[32];
	int value;

	/* TODO: Add more members if necessary */

} WP;


void new_wp(char* ch);
void check_wp(int* state);
void delete_wp(int number);
void print_wp();
#endif
