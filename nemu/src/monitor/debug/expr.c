#include "nemu.h"
#include <stdlib.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_DEC,NEG,DEREF

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"==", TK_EQ},        // equal
  {"\\*", '*'},         // multiply
  {"/", '/'},           // division
  {"\\+", '+'},         // plus
  {"-", '-'},           // subtraction
  {"\\(", '('},         // l_bracket
  {"\\)", ')'},         // r_bracket
  {"[0-9]+", TK_DEC}    // decimal

};

static struct node {
    int token_type;
    int priority;
} nodes[] = {
    /*quote from C Operator Precedence*/
    { "+", 4},
    { "-", 4},
    { "*", 3},
    { "/", 3},
    { NEG, 2}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

bool check_unary_operator(int pos) {
    /* Just need to judge front token
     * is right parenthesis or number.
     * Using for negative number and indirect referencing judgement.
     */
    if (pos == 0) {
        return true;
    } else {
        if(tokens[pos-1].type == TK_DEC || tokens[pos-1].type == ')'){
            return false;
        }
    }
    return true;
}

/* Discern token */
static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
            case '+':
                tokens[nr_token++].type = rules[i].token_type;
                break;

            case '-':
                if(check_unary_operator(nr_token)){
                    tokens[nr_token++].type = NEG;
                } else {
                    tokens[nr_token++].type = rules[i].token_type;
                }
                break;
            case '*':
                if(check_unary_operator(nr_token)) {
                    tokens[nr_token++].type = DEREF;
                } else {
                    tokens[nr_token++].type = rules[i].token_type;
                }
                break;

            case '/':
                tokens[nr_token++].type = rules[i].token_type;
                break;
            case '(':
                tokens[nr_token++].type = rules[i].token_type;
                break;
            case ')':
                tokens[nr_token++].type = rules[i].token_type;
                break;

            case TK_NOTYPE: break;
            case TK_DEC:
                assert(substr_len <= 31);

                strncpy(tokens[nr_token].str, substr_start, substr_len);    //copy string
                tokens[nr_token].str[substr_len] = '\0';
                tokens[nr_token++].type = TK_DEC;
                break;

            default:
                printf("Wrong token!\n");
                return false;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int st, int ed) {
    /*
     * Judge the expression is surrounded by a matched pair of parentheses.
     * If that is true, just throw awat the parentheses.
     */
    if(tokens[st].type != '(' || tokens[ed].type != ')') {
        return false;
    }
    int flag = 0;

    for (int i = st; i < ed; ++i) {
        if(tokens[i].type == '(') flag++;
        if(tokens[i].type == ')') flag--;
        if(flag == 0 && i < ed) {
            return false;
        }
        if(tokens[ed].type == ')') flag--;
    }
    return flag == 0;
}

bool check_op(int pos){
    return !(tokens[pos].type == '(' ||
             tokens[pos].type == ')' ||
             tokens[pos].type == TK_DEC||
             tokens[pos].type == TK_NOTYPE);
}

int search_main_op(int st, int ed, int *main_op) {
    int op_prior = -1;
    for (int i = st; i < ed; ++i) {
        if(tokens[i].type == '(') {     //Assume the expression is right.
            int flag = 1;
            while(1) {
                if(tokens[i].type == '(') flag++,i++;
                if(tokens[i].type == ')') flag--,i++;
                if(flag == 0) break;
            }
        }
        if(check_op(i)) {
            int j = 0;
            for (j = 0; j < 5; ++j) {
                if(nodes[j].token_type == tokens[i].type) break;
            }
            if(nodes[j].priority > op_prior)
                op_prior = nodes[j].priority,main_op = i;
        }
    }
}

uint32_t eval(int st, int ed, bool *success) {      // start/end position of the expr
    if(st > ed) {                                   // bad expression
        return *success = false;
    } else if(st == ed){
        /* Single token.
         * For now this token should be a number.
         * Return the value of the number.
         */

        return strtol(tokens[st].str, NULL, 10);;
    } else if(check_parentheses(st, ed) == true){
        return eval(st+1, ed-1, success);
    } else {
        int main_op = 0;
        search_main_op(st,ed,&main_op);
        int op_type = tokens[main_op].type;
        int val1 = 0x3f3f3f3f;  //inf
        int val2 = 0x3f3f3f3f;  //inf
        if (op_type != NEG && op_type != DEREF)
            val1 = eval(st, main_op-1, success);
        val2 = eval(main_op+1, ed, success);

        switch (op_type) {
            case '+':
                return val1 + val2;
            case '-':
                return val1 - val2;
            case '*':
                return val1 * val2;
            case '/':
                return val1 / val2;
            /*case DEREF:*/
                //TODO
            case NEG:
                return -val2;
            default: assert(0);
        }
    }
}


uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */


  return 0;
}
