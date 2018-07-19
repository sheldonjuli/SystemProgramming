#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

typedef struct stack_element {
    unsigned int frame;
    struct stack_element *next;
} stack_e;

stack_e *head;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int lru_evict() {
    
    stack_e *curr = head;
    int evict_frame = -1;
    while(curr->next->next != NULL) {
        curr = curr->next;
    }
    evict_frame = curr->next->frame >> PAGE_SHIFT;
    free(curr->next);
    curr->next = NULL;
    return evict_frame;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {

    stack_e *curr = head;
    while (curr != NULL && curr->next != NULL) {
        if (curr != NULL) {
            if (curr->next->frame == p->frame) {
                stack_e *temp = curr->next;
                curr->next = curr->next->next;
                free(temp);
                break;
            }
            curr = curr->next;
        }
    }

    // add to the head
    stack_e *new_element = (stack_e *)malloc(sizeof(stack_e));
    new_element->frame = p->frame;
    new_element->next = head;
    head = new_element;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
    head = NULL;
}
