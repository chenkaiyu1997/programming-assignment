#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ, NEQ,
	NUM, NUM16,
	REG, REF, REV,
	AND, OR, VAR
	/* TODO: Add more token types */
};

static struct rule {
	char *regex;
	int token_type;
	int precedence;
	int associate; // 0 -> left 1 -> right
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE, 99, 0},				
	{"==", EQ, 3, 0}, 					
	{"!=", NEQ, 3, 0}, 				
	{"&&", AND, 5, 0},
	{"\\|\\|", OR, 4, 0},

	{"\\+", '+', 10, 0},
	{"-", '-', 10, 0},
	{"\\(", '(', 99, 0},
	{"\\)", ')', 99, 0},
	{"\\*", '*', 20, 0},
	{"\\/", '/', 20, 0},

	{"0x[0-9a-fA-F]+", NUM16, 99, 0},
	{"[0-9]+", NUM, 99, 0},
	{"\\$[a-zA-Z]+",REG, 99, 0},
	{"[a-zA-Z_]+[a-zA-Z0-9_]*", VAR, 99, 0},

	{"\\!", '!', 30, 1},
	{"\\*", REF, 30, 1},
	{"\\-", REV, 30, 1},

};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret != 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
	int precedence;
	int associate; // 0 -> left 1 -> right
} Token;

Token tokens[32];
int nr_token;

bool is_op(char ch) {
	return ch == '+' || ch == '-' || ch == '*' || ch == '/';
}

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array ``tokens''. For certain 
				 * types of tokens, some extra actions should be performed.
				 */
				tokens[nr_token].type = rules[i].token_type;
				tokens[nr_token].precedence = rules[i].precedence;
				tokens[nr_token].associate = rules[i].associate;

				if (substr_len > 31) { //One byte for \0
					printf("Ahh... Too long.");
					return false;
				}
				switch(rules[i].token_type) {
					case NUM:
					case NUM16:
					case VAR:
					case REG:
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token].str[substr_len] = '\0';
						break;
					case NOTYPE:
						nr_token --; //No record
						break;
					case '-':
						if (nr_token == 0 || is_op(tokens[nr_token - 1].type)){
							tokens[nr_token].precedence = 30;
							tokens[nr_token].associate = 1;
							tokens[nr_token].type = REV;
						}
						break;
					case '*':
						if (nr_token == 0 || is_op(tokens[nr_token - 1].type)){
							tokens[nr_token].precedence = 30;
							tokens[nr_token].associate = 1;
							tokens[nr_token].type = REF;
						}
						break;
					default: ;
				}
				nr_token ++;
				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}
	return true; 
}

int get_var(char*);//用于查找变量

int eval(int p, int q, bool *success);
uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
	return eval(0, nr_token - 1, success);
}


int parse_num(char *s) {
	int result = 0;
	for (; *s; s++) {
		result = result * 10 + (*s - '0');
	}
	return result;
}

int parse_num16(char *s) {
	int result = 0;
	s += 2;
	for (; *s; s++) {
		if (*s >= 'a')
			*s -= 32;
		result = result * 16 + ((*s >= 'A') ? (*s - 'A' + 10) : (*s - '0'));
	}
	return result;
}

int get_reg(char *s, bool *success) {
	s++;
	if(strcmp(s, "eip") == 0) 
		return cpu.eip;
	int i;
	for(i = 0; i < 8; i++)
		if(strcmp(regsl[i], s) == 0)
			return reg_l(i);

	for(i = 0; i < 8; ++i)
		if(strcmp(regsw[i], s) == 0)
			return reg_w(i);

	for(i = 0; i < 8; ++i)
		if(strcmp(regsb[i], s) == 0)
			return reg_b(i);

	*success = false;
	return 0;
}

bool check_parentheses(int p, int q, bool *success) {
	if(tokens[p].type != '(' || tokens[q].type != ')')
		return false;
	int cnt = 0;
	for(; p <= q; p++) {
		if (tokens[p].type == '(')
			cnt ++;
		else if (tokens[p].type == ')')
			cnt --;
		else if (cnt < 0) {
			*success = false;
			return false;
		}
		else if (cnt <= 0) // If something outside of parentheses
			return false;
	}
	if (cnt != 0)
		*success = false;
	return cnt == 0;
}

int find_dominant_pos(int p, int q, bool* success) {
	int ans = -1;
	int cnt = 0;
	for(; p <= q; p++) {
		if (tokens[p].type == '(')
			cnt ++;
		else if (tokens[p].type == ')')
			cnt --;
		else if (cnt == 0 && (ans == -1 || tokens[p].precedence <= tokens[ans].precedence)) {
			if (ans == -1 || tokens[p].precedence < tokens[ans].precedence) 
				ans = p;
			else if (tokens[p].associate == 0)
				ans = p;
		}
	}
	if(ans == -1)
		*success = false;
	return ans;
}

int eval(int p, int q, bool *success) {
	if (*success == false)
		return 0;
	if (p > q) {
		*success = false;
		return 0;
	}
	else if (p == q) {
		if (tokens[p].type == NUM) {
			return parse_num(tokens[p].str);
		}
		if (tokens[p].type == NUM16) {
			return parse_num16(tokens[p].str);
		}
		if (tokens[p].type == REG) {
			return get_reg(tokens[p].str, success);
		}
		if(tokens[p].type == VAR){
			int result = get_var(tokens[p].str);
			if(result == -1){
				*success = false;
				printf("can not find variable : %s\n", tokens[p].str);
				return 1;
			}
			else return result;
		}
		*success = false;
		return 0;
	}
	else if(check_parentheses(p, q, success)) {
		return eval(p + 1, q - 1, success);
	}
	else if(tokens[p].type == REV) {
		return -eval(p + 1, q, success);
	} 
	else if(tokens[p].type == REF) {
		return (int)swaddr_read(eval(p + 1, q, success), 4);
	}
	else if(tokens[p].type == '!') {
		return !eval(p + 1, q, success);
	}
	else {
		int op = find_dominant_pos(p, q, success);
		int val1 = eval(p, op - 1, success);
		int val2 = eval(op + 1, q, success);

		switch(tokens[op].type) {
			case '+':
				return val1 + val2;
			case '-':
				return val1 - val2;
			case '*':
				return val1 * val2;
			case '/':
				return val1 / val2;
			case AND:
				return val1 && val2;
			case OR:
				return val1 || val2;
			case EQ:
				return val1 == val2;
			case NEQ:
				return val1 != val2;
			default:
				printf("No such type!!");
				*success = false;
				return 0;
		}
	}
}

