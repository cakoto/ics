#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;                       // the number of node
  struct watchpoint *next;
  char expr[32];
  uint32_t val;
} WP;

WP* new_wp(char* expr,int val);
void free_wp(int NO);
void wp_info(int pattern, WP* wp);
bool is_wp_update();

#endif
