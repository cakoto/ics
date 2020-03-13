#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include <stdlib.h>

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;


void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i+1;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

// shallow copy
void shallow_copy(WP* target,WP* prototype) {
    target->next = prototype->next;
    target->NO = prototype->NO;
}

// Create a new node
WP* new_wp(char* expr,int val) {
    if(head == NULL) {
        // add info
        head = (WP*)malloc(sizeof(WP));
        shallow_copy(head, free_);
        free_ = free_->next;
        head->next = NULL;
        strcpy(head->expr, expr);
        head->val = val;
        wp_info(1, head);

    } else {
        // overflow
        if (free_->next == NULL) {
            assert(0);
        }

        WP* p = head;
        while(p->next != NULL) {
            p = p->next;
        }
        // add info
        p->next = free_;
        p = p->next;
        strcpy(p->expr, expr);
        p->val = val;
        free_ = free_->next;
        p->next = NULL;
        wp_info(1, p);

    }
    return head;
}

// Free node
void free_wp(int NO) {
    if (head == NULL) {
        printf("Watch point List is empty!\n");
        assert(0);
    }
    WP* p = head;       // Find the node which want to find.
    WP* prev = p;       // Store the previous node.
    WP* temp = free_;   // temp variable

    // Find the node
    while (p->NO != NO) {
        if (p->next == NULL) {
            printf("Don't find the watch point!\n");
            return;
        }
        prev = p;
        p = p->next;
    }

    if (head == p) {
        wp_info(2, head);
        if(head->next == NULL) {
            head = NULL;
            printf("Watch point list has been deleted over!\n");
            return;
        }
        head = head->next;
    } else {
        prev->next = p->next;
        wp_info(2, prev);
    }

    prev = free_;
    // insert sort
    while (p->NO > temp->NO) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == free_) {
        p->next = free_;
        memset(p->expr,'\0',32);
        p->val = 0;
        free_ = p;
    } else {
        p->next = temp;
        memset(p->expr,'\0',32);
        p->val = 0;
        prev->next = p;
    }
    // init

}

// Print watch point information
void wp_info(int pattern, WP* wp) {
    if (pattern == 1) {
        printf("Add NO.%d watch point\t%s\t%d\n",wp->NO, wp->expr, wp->val);
    } else if (pattern == 2) {
        printf("Successfully Delete the No.%d watch point.\n", wp->NO);
    } else {
        WP *temp = head;
        if (temp == NULL) {
            printf("Watch point list is empty.\n");
            return;
        }
        printf("Num\tExpr\tAddress\t\n");
        while (temp != NULL) {
            printf("%d:\t%s\t%d\t0x%08x\n", temp->NO, temp->expr, temp->val, temp->val);
            temp = temp->next;
        }
    }
}

// Check watch points update status
bool is_wp_update() {
    WP* temp = head;
    bool success = false;
    bool flag = false;

    while (temp != NULL) {
        uint32_t res = expr(temp->expr,&success);
        if(success == true) {
            if(res != temp->val) {
                flag = true;
                printf("NO.%d watchpoint\texpression:%s\n",temp->NO,temp->expr);
                printf("Old value:\t0x%08x\t%d\n",temp->val,temp->val);
                printf("New value:\t0x%08x\t%d\n",res,res);
                temp->val = res;
            }
        } else {
            printf("Invalid expression!\n");
            return false;
        }
        temp = temp->next;
    }
    return flag;
}
