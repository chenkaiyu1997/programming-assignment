#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_list[NR_WP];
static WP *head, *free_;

void new_wp(char *s) {
	WP* tmp = free_;
	strcpy(tmp->expr_string, s);
	bool success = true;
	tmp->value = expr(s, &success);
	if (!success) {
		printf("Expression error\n");
		return;
	}
	free_ = free_ -> next;
	tmp -> next = head;
	head = tmp;
	return;
}

void delete_wp(int number) {
	WP *tmp;
	WP *last = head;
	for(tmp = head; tmp; tmp = tmp -> next) {
		if (tmp -> NO == number)
			break;
		last = tmp;
	}
	if (tmp == NULL) {
		printf("No watchpoint %d\n", number);
	}
	if(last == tmp) {
		head = last -> next;
	}
	last -> next = tmp -> next;
	tmp -> next = free_;
	free_ = tmp;
	printf("watchpoint %d delete successfully\n", number);
	return;
}

void check_wp(int* nemu_state) {
	WP* iter;
	for(iter = head; iter; iter = iter -> next) {
		bool success = true;
		int new_value = expr(iter -> expr_string, &success);
		if (new_value != iter -> value) {
			*nemu_state = 0;
			printf("%8x:\twatchpoint %d hit: the value of %s changed from %d to %d\n", cpu.eip, iter->NO, iter->expr_string, iter->value, new_value);
			iter -> value = new_value;
		}
	}
}

void print_wp() {
	if (head == NULL) {
		printf("No watchpoint!");
		return;
	}
	WP* iter;
	for(iter = head; iter; iter = iter -> next) {
		printf("watchpoint %d\texpr: %s\tvalue : %d\n", iter->NO, iter->expr_string, iter->value);
	}
	return;
}

void init_wp_list() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_list[i].NO = i;
		wp_list[i].next = &wp_list[i + 1];
	}
	wp_list[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_list;
}

/* TODO: Implement the functionality of watchpoint */


