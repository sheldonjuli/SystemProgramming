#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

char *clock_array;

int clock_idx;

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {

    int evict_idx = -1;
    while (evict_idx < 0) {

        if (clock_array[clock_idx] == 1) {
            clock_array[clock_idx] = 0;
        } else {
            evict_idx = clock_idx;
        }

        clock_idx = (clock_idx + 1) % memsize;
    }

    return evict_idx;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
    
    clock_array[p->frame >> PAGE_SHIFT] = 1;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
    clock_idx = 0;
    clock_array = (char *) calloc(memsize, sizeof(char));
}
