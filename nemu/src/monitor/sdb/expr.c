/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <memory/vaddr.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, 

  /* TODO: Add more token types */
  TK_DECIMAL,
  TK_HEX,
  TK_POS,
  TK_NEG,
  TK_REF,
  TK_DEREF,
  TK_EQ,
  TK_NEQ,
  TK_AND,
  TK_OR,
  TK_REG_NAME,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {"\\+", '+'},         // plus
  {"\\-", '-'},         // minus
  {"\\*", '*'},         // mul
  {"\\/", '/'},         // div
  {"\\(", '('},         // lp
  {"\\)", ')'},         // rp
  {" +", TK_NOTYPE},    // spaces
  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ},
  {"&&", TK_AND},
  {"\\|\\|", TK_OR},
  {"0x[a-fA-F0-9]+", TK_HEX},    // spaces
  {"[0-9]+", TK_DECIMAL},    // spaces
  {"\\$(\\$0|ra|sp|gp|tp|t[0-6]|s[0-9]|s1[01]|a[0-7])", TK_REG_NAME},
};

/*
  运算符优先级（由高至低）：
  * 1. 正负号、取地址、解引用：+, -, &, *
  * 2. 乘除：* /
  * 3. 加减：+ -
  * 4. 判等：==, !=
  * 5. 逻辑与、或：&&, ||
  * ...
*/
#define NR_PRIOR 5 // 目前有5种
#define NR_REGEX ARRLEN(rules)

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

        // Log("matched str is %s\n", e + position);
        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;
          case '+':
          case '-':
          case '*':
          case '/':
          case '(':
          case ')':
          case TK_EQ:
          case TK_NEQ:
          case TK_AND:
          case TK_OR:
          case TK_DECIMAL:
          case TK_HEX:
          case TK_REG_NAME:
            assert(nr_token < 32);
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token ++;
            break;
          default:
            assert(0);
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

bool check_parentheses(int p, int q);

long int eval(int p, int q) {
  Log("eval: p = %d, q = %d", p, q);
  if (p > q) {
    /* Bad expression */
    assert(0);
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    // printf("the type is %d\n", tokens[p].type);
    // printf("the value is %s\n", tokens[p].str);
    switch (tokens[p].type) {
      case TK_DECIMAL:
        return atoi(tokens[p].str);
      case TK_HEX:
        return strtol(tokens[p].str, NULL, 16);
      case TK_REG_NAME:
        bool success = false;
        int reg_val = isa_reg_str2val(tokens[p].str+1, &success);
        if (success) {
          return reg_val;
        } else {
          Log("reg name %s error.", tokens[p].str);
          return 0;
        }
      default:
        assert(0);
    }
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else { // 判断主运算符，分治
    // 先根据优先级记录运算符的位置，假定每一个优先级的运算符不超过10个
    int op_priority[NR_PRIOR][10] = {0};
    int op_count[NR_PRIOR] = {0}; // 每种运算符的个数
    int paren_count = 0;
    int main_op = 0;
    for (int i = p; i <= q; i++) {
      if (tokens[i].type == '(') {
        paren_count ++;
      }
      else if (tokens[i].type == ')') {
        paren_count --;
      }
      else if (paren_count == 0) {
        switch (tokens[i].type) {
          case TK_POS:
          case TK_NEG:
          case TK_REF:
          case TK_DEREF:
            op_priority[0][op_count[0] ++] = i;
            break;
          case '*':
          case '/':
            op_priority[1][op_count[1] ++] = i;
            break;
          case '+':
          case '-':
            op_priority[2][op_count[2] ++] = i;
            break;
          case TK_EQ:
          case TK_NEQ:
            op_priority[3][op_count[3] ++] = i;
            break;
          case TK_AND:
          case TK_OR:
            op_priority[4][op_count[4] ++] = i;
            break;
          case TK_DECIMAL:
          case TK_HEX:
            break;
          default:
            assert(0);
        }
      }
    }
    int highest_prior = -1;
    for (int i = NR_PRIOR-1; i >= 0; i --) {
      if (op_count[i] > 0) {
        if (i > 0) { // 左结合
          main_op = op_priority[i][op_count[i] - 1];
        } else { // 右结合
          main_op = op_priority[i][0];
        }
        highest_prior = i;
        break;
      }
    }

    assert(highest_prior >= 0);

    if (highest_prior == 0) {
      int the_val = eval(main_op + 1, q);
      switch (tokens[main_op].type) {
        case TK_POS:
          return the_val;
        case TK_NEG:
          return -the_val;
        case TK_DEREF:
          return vaddr_read(the_val, 4);
        // case TK_REF:
        //   return &the_val;
        default: assert(0);
      }
    }

    int val1 = eval(p, main_op - 1);
    int val2 = eval(main_op + 1, q);

    switch (tokens[main_op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      case TK_EQ: return val1 == val2;
      case TK_NEQ: return val1 != val2;
      case TK_AND: return val1 && val2;
      case TK_OR: return val1 || val2;
      default: assert(0);
    }
  }
}

bool check_parentheses(int p, int q) {
  if (tokens[p].type != '(' || tokens[q].type != ')') {
    return false;
  }

  int i;
  int count = 0;

  for (i = p; i <= q; i ++) {
    if (tokens[i].type == '(') {
      count ++;
    }
    else if (tokens[i].type == ')') {
      count --;
    }
  }
  assert(count == 0);
  return count == 0;
}


bool require_unary_op(int tk_type) {
  switch (tk_type) {
    case '+':
    case '-':
    case '*':
    case '/':
    case '(':
      return true;
    default:
      return false;
  }
}

long int expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  for (int i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '-' && (i == 0 || require_unary_op(tokens[i - 1].type))) {
        tokens[i].type = TK_NEG;
    }
    if (tokens[i].type == '*' && (i == 0 || require_unary_op(tokens[i - 1].type))) {
        tokens[i].type = TK_DEREF;
    }
  }

  /* TODO: Insert codes to evaluate the expression. */
  int p = 0;
  int q = nr_token - 1;

  Log("Total %d tokens.", nr_token);
  return eval(p, q);
}
