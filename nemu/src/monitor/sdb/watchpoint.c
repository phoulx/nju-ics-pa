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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[32];
  int old_val;
  int val;

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp() {
  if (free_ == NULL) {
    assert(0);
  }

  WP *wp = free_;
  free_ = free_->next;
  wp->next = head;
  head = wp;
  return wp;
}


void free_wp(WP *wp) {
  wp->next = free_;
  free_ = wp;
}

bool set_wp(char *args) {
  if (!args) {
    return false;
  }
  WP *wp = new_wp();
  strcpy(wp->expr, args);
  bool success = true;
  wp->val = expr(wp->expr, &success);
  if (success) {
    return true;
  }
  return false;
}

void del_wp(int n) {
  free_wp(&wp_pool[n]);
}

void watchpoint_check() {
  WP* curr = head;
  while (curr != NULL) {
    bool success = false;
    int new_val = expr(curr->expr, &success);
    if (curr->val != new_val) {
      Log("Watchpoint %d triggered (expr is %s):", curr->NO, curr->expr);
      printf("before: %d, after: %d\n", curr->old_val, new_val);
      curr->old_val = curr->val;
      curr->val = new_val;
      nemu_state.state = NEMU_STOP;
      break;
    }
  }
}

void watchpoints_display() {
  printf("%10s\t%10s\t%10s\n", "NO", "Expr", "Current Value");
  WP* curr = head;
  while (curr != NULL) {
    printf("%10d\t%10s\t%10d\n", curr->NO, curr->expr, curr->val);
    curr = curr->next;
  }
  printf("\n");
}