#include <stdint.h>
#include <stdio.h>
#include "alloc.h"

int initialized = 0;
const size_t memsize = 256;

// Byte buffer for allocations.
uint8_t heap[memsize];

// Control block structure.
struct ctrl {
    size_t       size;
    struct ctrl* next;
};

const size_t blocksize = sizeof (struct ctrl);

// Pointer to the control block of the first
// free region of memory. This is NULL until
// the first allocation is made.
struct ctrl* free_list_start = NULL;

// Magic value to identify allocated blocks.
// There is nothing special about this value, other
// than the fact that it appears  as "DEAD" in  the
// hex dump, indicating that it is not reachable in
// the free list.
struct ctrl* ALLOCATED = (struct ctrl*) 0xADDE;

void init() {
    // Create control block that spans all memory.
    struct ctrl* initial = (struct ctrl*) heap;
    initial->size = memsize - blocksize;
    initial->next = NULL;

    // Point the free list head to the block.
    free_list_start = initial;

    initialized = 1;
}

// Get the first byte following an allocation.
void *byteafter(struct ctrl *blk) {
    return (void*) blk + blocksize + blk->size;
}

void *alloc(size_t nbytes) {
    if (!initialized) {
        init();
    }

    // We only allocate multiples of 8 bytes.
    nbytes = (7 + nbytes) & (~7);

    if (nbytes == 0) {
        return (void *) 0;
    }

    // Look for a candidate block.
    struct ctrl **blk = &free_list_start;
    while (*blk != NULL && (*blk)->size < nbytes) {
        blk = &(*blk)->next;
    }

    if (*blk == NULL) {
        // There does not exist a big enough block.
        return (void *) 0;
    } else if ((*blk)->size > nbytes + blocksize) {
        // Strict inequality guarantees space for 
        // non-zero allocation in the new block.
        void *ptr = *blk + 1;

        // Split memory with new block.
        struct ctrl* split = ptr + nbytes;
        split->size = (*blk)->size - nbytes - blocksize;
        split->next = (*blk)->next;

        // Update the block to reflect new size.
        (*blk)->size = nbytes;
        (*blk)->next = ALLOCATED;

        // Remove this block from the free list.
        *blk = split;

        return ptr;
    } else {
        // Block is big enough, but not big enough
        // to accomodate another allocation.
        void *ptr = *blk + 1;
        struct ctrl* next = (*blk)->next;
        (*blk)->next = ALLOCATED;
        *blk = next;
        return ptr;
    }

}

void free(void *ptr) {
    // Compute the ctrl block associated with ptr.
    struct ctrl *blk = ((struct ctrl *) ptr) - 1;

    // Check for NULL-free and double free. (NOOP)
    if (ptr == NULL || blk->next != ALLOCATED) {
        return;
    }

    // Remove allocated-mark.
    blk->next = NULL;

    // Search for adjacent free blocks to merge.
    struct ctrl **adj = &free_list_start;
    while (*adj != NULL) {
        if (byteafter(*adj) == blk) {
            // Left-adjacent is free.
            (*adj)->size += blocksize + blk->size;
            blk = *adj;
            *adj = (*adj)->next;
        } else if (byteafter(blk) == *adj) {
            // Right-adjacent is free.
            blk->size += blocksize + (*adj)->size;
            *adj = (*adj)->next;
        } else {
            // Keep looking.
            adj = &(*adj)->next;
        }
    }

    // Put the new (merged) control block in the free list.
    blk->next = free_list_start;
    free_list_start = blk;
}

void hexdump() {
    for (int i = 0; i < memsize; i++) {
        if (i % 8 == 0) {
            printf("%p | ", &heap[i]);
        }

        // Print one byte as two hexadecimal digits
        printf("%02X ", heap[i]);

        // On the end of the line (every 8 bytes)
        if ((i + 1) % 8 == 0) {
            // Mark if these 8 bytes is the allocated tab
            if (ALLOCATED == *(struct ctrl **) &heap[i-7]) {
                printf("| A");
            } else {
                // Decimal value (size_t, i64, u64)
                printf("| %u",   *(uint32_t *) &heap[i-7]);
            }
            // New line every 8 bytes
            printf("\n");
        }
    }

    printf("%zu bytes dumped.\n", memsize);

    // Print the free list.
    printf("FREE LIST:\n");
    struct ctrl **blk = &free_list_start;
    while (*blk != NULL) {
        printf("%p %zu\n", *blk, (*blk)->size);
        blk = &(*blk)->next;
    }
}

