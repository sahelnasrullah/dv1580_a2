#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// gcc -E memory_manager.c -o memory_manager.i // Preprocessor
// gcc -S memory_manager.c -o memory_manager.s // Compiler
// gcc -c memory_manager.c -o memory_manager.o // Assembly

// Define structure memory block
typedef struct Memory_Block 
{
    size_t size;          // Size
    int free;             // 1 means free, 0 means it's in use
    struct Memory_Block* next; // Pointer to the next block
} Memory_Block;

void* memory_pool = NULL; // // 
Memory_Block* free_memory_list = NULL;   // Pointer to free memory blocks
size_t memory_left = 0;                  // Memory left

// Initialize with size
void mem_init(size_t size) 
{
    if (size == 0) 
    {
        printf("Error: Can't initialize memory pool with size 0\n");
        return;
    }

    // Allocate memory
    memory_pool = malloc(size * sizeof(Memory_Block));
    if (memory_pool == NULL) 
    {
        printf("Failed to allocate memory pool (mem_init)\n");
        return;
    }

    // set first block in the pool
    free_memory_list = (Memory_Block*) memory_pool;
    free_memory_list->size = size;
    free_memory_list->free = 1; 
    free_memory_list->next = NULL;
    
    memory_left = free_memory_list->size;
}

// Allocate memory from the pool
void* mem_alloc(size_t size) 
{
    Memory_Block* current = free_memory_list;

    if (size == 0) 
    {
        return NULL;
    }
    if (memory_left < size) 
    {
        printf("Not enough memory left (asked for more than available)\n");
        return NULL;
    }

    // traverse to find block large enough
    while (current != NULL) 
    {
        if (current->free && current->size >= size) 
        {
                // Split block if it's big enough
                Memory_Block* new_block = (Memory_Block*) ((char*)current + sizeof(Memory_Block) + size);
                new_block->size = current->size - size;
                new_block->free = 1; 
                new_block->next = current->next;

                current->size = size;
                current->next = new_block;
                
            current->free = 0; 
            memory_left -= size; 

            return (void*)(current + 1); // Return pointer
        }
        current = current->next;
    }

    printf("Not enough memory (couldn't find a suitable block)\n");
    return NULL;
}

// Free block
void mem_free(void* block) {
    if (block == NULL) 
    {
        return;
    }

    Memory_Block* mem_block = (Memory_Block*)block - 1;
    mem_block->free = 1;
    memory_left += mem_block->size + sizeof(Memory_Block); // add available memory 

    // Merge free blocks
    Memory_Block* current = free_memory_list;
    while (current != NULL) 
    {
        if (current->free && current->next && current->next->free) 
        {
            current->size += sizeof(Memory_Block) + current->next->size; // merge with the next block
            current->next = current->next->next; 
        }
        current = current->next;
    }
}

// Resize allocated block
void* mem_resize(void* block, size_t new_size) 
{
    if (block == NULL) {
        return mem_alloc(new_size);
    }

    Memory_Block* mem_block = (Memory_Block*)block - 1;

    if (mem_block->size >= new_size) 
    {
        return block; // No need to resize
    }

    // Allocate new memory and copy over the data
    void* new_block = mem_alloc(new_size);
    if (new_block != NULL) 
    {
        memcpy(new_block, block, mem_block->size); // Copy old data to new block
        mem_free(block);
    }

    return new_block;
}

// Free everything
void mem_deinit() 
{
    free(memory_pool);
    memory_pool = NULL; 
    free_memory_list = NULL;
    memory_left = 0;
}