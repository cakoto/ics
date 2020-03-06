#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <math.h>

// this should be enough
static char buf[65536];
static int pos = 0; // position

static inline uint32_t choose(uint32_t n){
    srandom(time(NULL));
    return n == 0 ? random() : random() % n;
}

void gen_num();
void gen(char op);
void gen_rand_op();

static inline void gen_rand_expr() {

    switch (choose(3)) {
        case 0:
            gen_num();
            break;
        case 1:
            gen('(');
            gen_rand_expr();
            gen(')');
            break;
        default:
            gen_rand_expr();
            gen_rand_op();
            gen_rand_expr();
            break;

    }

    if (pos >= 65536) assert(0);
    buf[0] = '\0';
}

void gen(char op) {
    buf[pos++] = op;
}

void gen_num() {
    uint32_t temp = choose(0);
    int nDigits = (int)floor(log10(temp)) + 1;
    snprintf(buf+pos, nDigits, "%d", temp);
    pos += nDigits;
}

void gen_rand_op() {
    switch (choose(4)) {
        case 0: buf[pos++] = '+';break;
        case 1: buf[pos++] = '-';break;
        case 2: buf[pos++] = '*';break;
        default: buf[pos++] = '/';break; // division zero is a small probability event
    }
}

static char code_buf[65536];
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
