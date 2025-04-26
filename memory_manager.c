#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include "memory_manager.h"

pthread_mutex_t memory_mutex = PTHREAD_MUTEX_INITIALIZER;

void* memory_pool = NULL;   
Memory_Block* block_pool = NULL;      

int memory_pool_size = 0;      
int total_memory_allocated = 0;

void mem_init(size_t size) {
    pthread_mutex_lock(&memory_mutex);

    memory_pool = malloc(size);
    block_pool = malloc(sizeof(Memory_Block));

    if (memory_pool == NULL || block_pool == NULL) {
        printf("Error: Memory pool allocation failed\n");
        pthread_mutex_unlock(&memory_mutex);
        return;
    }

    block_pool->pnt = memory_pool;
    block_pool->size = size;
    block_pool->free = true;
    block_pool->next = NULL;

    memory_pool_size = size;
    total_memory_allocated = 0;

    pthread_mutex_unlock(&memory_mutex);
}

void* mem_alloc(size_t size) 
{
    if (size == 0) size = 1;

    pthread_mutex_lock(&memory_mutex);

    if (total_memory_allocated + size > memory_pool_size) {
        pthread_mutex_unlock(&memory_mutex);
        return NULL;
    }

    Memory_Block* current = block_pool;

    while (current != NULL)
    {
        if (current->free && current->size >= size)
        {
            if (current->size == size) {
                current->free = false;
                total_memory_allocated += size; 
                pthread_mutex_unlock(&memory_mutex);
                return current->pnt;
            }

            Memory_Block* new_block = malloc(sizeof(Memory_Block));
            if (new_block == NULL) {
                pthread_mutex_unlock(&memory_mutex);
                printf("No block allocated\n");
                return NULL;
            }

            new_block->pnt = (char*)current->pnt + size;
            new_block->size = current->size - size;
            new_block->free = true;
            new_block->next = current->next;

            current->size = size;
            current->free = false;
            current->next = new_block;

            total_memory_allocated += size; 

            pthread_mutex_unlock(&memory_mutex);
            return current->pnt;
        }

        current = current->next;
    }

    pthread_mutex_unlock(&memory_mutex);
    return NULL;
}

void mem_free(void* block) {
    if (!block) {
        printf("Nothing to free\n");
        return;
    }
    pthread_mutex_lock(&memory_mutex);

    Memory_Block * current = block_pool;
    while (current != NULL) 
    {
        if (current->pnt == block) {
            if (!current->free) {
                total_memory_allocated -= current->size;
            }
            current->free = true;

            Memory_Block* next_block = current->next;
            if (next_block != NULL && next_block->free) {
                current->size += next_block->size;
                current->next = next_block->next;
                free(next_block);
            }
            pthread_mutex_unlock(&memory_mutex);
            return;
        } 
        else {
            current = current->next;
        }
    }
    pthread_mutex_unlock(&memory_mutex);
}

void* mem_resize(void* ptr, size_t new_size) {
    if (ptr == NULL) {
        printf("Block is NULL");
        return NULL;
    }

    pthread_mutex_lock(&memory_mutex);
    Memory_Block* current = block_pool;
    while (current != NULL) {
        if (current->pnt == ptr) {
            if (current->size >= new_size) {
                printf("Block is large enough");
                pthread_mutex_unlock(&memory_mutex);
                return ptr;
            } else {
                void* pnt_new_block = mem_alloc(new_size);
                if (pnt_new_block == NULL) {
                    pthread_mutex_unlock(&memory_mutex);
                    return NULL;
                }
                memcpy(pnt_new_block, ptr, current->size);
                mem_free(ptr);
                pthread_mutex_unlock(&memory_mutex);
                return pnt_new_block;
            }
        }
        current = current->next;
    }
    pthread_mutex_unlock(&memory_mutex);
    return NULL;
}

void mem_deinit() {
    free(memory_pool);
    pthread_mutex_lock(&memory_mutex);
    memory_pool = NULL;

    Memory_Block* current = block_pool;
    while (current != NULL) {
        Memory_Block* next_block = current->next;
        free(current);
        current = next_block;
    }
    block_pool = NULL;
    pthread_mutex_unlock(&memory_mutex);
    pthread_mutex_destroy(&memory_mutex);
}