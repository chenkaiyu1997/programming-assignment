#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ,
	NUM
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
	{" +",	NOTYPE, 0, 0},				
	{"\\+", '+', 10, 0},
	{"-", '-', 10, 0},
	{"\\*", '*', 20, 0},
	{"\\/", '/', 20, 0},
	{"==", EQ, 5, 0}, 					
	{"\\d+", NUM, 0, 0}

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
						strncpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token].str[substr_len] = '\0';
						break;
					case NOTYPE:
						nr_token --; //No record
						break;
					default: panic("please implement me");
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

bool check_parentheses(int p, int q) {
	if(tokens[p].type != '(' || tokens[q].type != ')')
		return false;
	int cnt = 0;
	for(p++; p < q; p++) {
		if (tokens[p].type == '(')
			cnt ++;
		else if (tokens[p].type == ')')
			cnt --;
		else if (cnt <= 0) // If something outside of parentheses
			return false;
	}
	return cnt == 0;
}

int find_dominant_pos(int p, int q) {
	int ans = -1;
	int cnt = 0;
	for(p; p <= q; p++) {
		if (tokens[p].type == '(')
			cnt ++;
		else if (tokens[p].type == ')')
			cnt --;
		else if (cnt == 0 && (ans == -1 || tokens[p].precedence >= tokens[ans].precedence)) {
			if (ans == -1 || tokens[p].precedence > tokens[ans].precedence) 
				ans = p;
			else if (tokens[p].associate == 1)
				ans = p;
		}
	}
	Assert(ans != -1 || tokens[ans].type == NUM, "dominate error!");
	return ans;
}

int eval(int p, int q, bool *success) {
	if (p > q) {
		*success = false;
		return 0;
	}
	else if (p == q) {
		if (tokens[p].type != NUM) {
			printf("Not Number ?!");
			*success = false;
			return 0;
		}
		return parse_num(tokens[p].str);
	}
	else if(check_parentheses(p, q)) {
		return eval(p + 1, q - 1, success);
	}
	else {
		int op = find_dominant_pos(p, q);
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
			default:
				printf("No such type.");
				*success = false;
				return 0;
		}
	}
}

