#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include "sim.h"

#define MAXLINE 256

extern char *tracefile;

char buf[MAXLINE];

int trace_count;

addr_t *trace_buf;

int curr_idx;

//extern int memsize;

extern int debug;

extern struct frame *coremap;

/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {

    int max = -1;
    int victom = -1;
    for (int i = 0; i < memsize; i++) {
        if (coremap[i].next_ref > max) {
            max = coremap[i].next_ref;
            victom = i;
        }
    }
    
    return victom;


}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {

    int frame_idx = p->frame >> PAGE_SHIFT;
    char *mem_ptr = &physmem[frame_idx*SIMPAGESIZE];
    addr_t *vaddr_ptr = (addr_t *)(mem_ptr + sizeof(int));

    //printf("%d\n", (unsigned int)(*vaddr_ptr));

    int i = curr_idx + 1;
    while(i < trace_count && (trace_buf[i] != (*vaddr_ptr))) {
        i++;
    }
    // if we don't find the next reference, next reference will be set to the trace count
    coremap[frame_idx].next_ref = i - curr_idx;
    //printf("%d %d\n", p->frame, i);
    curr_idx++;
    return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {

    if (!tracefile) {
        perror("Error: tracefile does not exist.");
        exit(1);
    }

    FILE *tfp = fopen(tracefile, "r");

    if (!tfp) {
        perror("Error: could not read from tracefile.");
        exit(1);
    }

    char type;
    addr_t vaddr;

    while(fgets(buf, MAXLINE, tfp)) {
        if(buf[0] != '=') {
            trace_count++;
        } else {
            continue;
        }
    }

    rewind(tfp);

    trace_buf = (addr_t *)malloc(trace_count * sizeof(addr_t));
    int i = 0;
    while(fgets(buf, MAXLINE, tfp)) {
        if(buf[0] != '=') {
            sscanf(buf, "%c %lx", &type, &vaddr);
            trace_buf[i] = vaddr;
            i++;
        } else {
            continue;
        }
    }

    curr_idx = -1;

    fclose(tfp);

}
