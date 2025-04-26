#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stddef.h>   // För size_t
#include <stdbool.h>  // För bool

#ifdef __cplusplus
extern "C" {
#endif


typedef struct Memory_Block {
    void *pnt;                   // Pekare till början av detta block
    size_t size;                // Storlek på blocket
    bool free;                  // Om blocket är ledigt eller inte
    struct Memory_Block* next;  // Nästa block i kedjan
} Memory_Block;


extern void* memory_pool;
extern Memory_Block* block_pool;
extern int memory_pool_size;


void mem_init(size_t size);


void *mem_alloc(size_t size);


void mem_free(void *block);


void *mem_resize(void *block, size_t size);


void mem_deinit();

#ifdef __cplusplus
}
#endif

#endif // MEMORY_MANAGER_H