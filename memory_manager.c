#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define structure for memory block metadata
typedef struct Memory_Block 
{
    size_t size;          // Size of the allocated memory block (in the memory pool)
    int free;             // 1 means free, 0 means it's in use
    struct Memory_Block* next; // Pointer to the next block
} Memory_Block;

void* memory_pool = NULL;        // Pool for actual data
void* block_pool = NULL;         // Pool for Memory_Block metadata
Memory_Block* free_memory_list = NULL;  // Pointer to free memory blocks
size_t memory_left = 0;          // Memory left in the memory pool
size_t block_pool_left = 0;      // Blocks left in the block pool

// Initialize with total size of memory pool
void mem_init(size_t size) 
{
    if (size == 0) 
    {
        printf("Error: Can't initialize memory pool with size 0\n");
        return;
    }

    // Allocate memory pools
    memory_pool = malloc(size);
    block_pool = malloc(1000 * sizeof(Memory_Block));  // Block pool for 1000 Memory_Block structures
    
    if (memory_pool == NULL || block_pool == NULL) 
    {
        printf("Failed to allocate memory pool or block pool (mem_init)\n");
        return;
    }

    // Set the first block from the memory pool
    free_memory_list = (Memory_Block*) block_pool;  // Use block pool for metadata
    free_memory_list->size = size; // Entire memory pool is free initially
    free_memory_list->free = 1;
    free_memory_list->next = NULL;

    memory_left = size;
    block_pool_left = 1000;  // Start with space for 1000 blocks in the block pool
}

// Helper function to allocate a new block from block pool
Memory_Block* get_new_block() 
{
    if (block_pool_left == 0) 
    {
        printf("No more blocks available in the block pool\n");
        return NULL;
    }

    // Allocate from the block pool
    Memory_Block* new_block = (Memory_Block*)((char*)block_pool + (1000 - block_pool_left) * sizeof(Memory_Block));
    block_pool_left--;

    return new_block;
}

// Allocate memory from the pool
void* mem_alloc(size_t size) 
{
    if (size == 0) 
    {
        return NULL;
    }

    if (memory_left < size) 
    {
        printf("Not enough memory left (asked for more than available)\n");
        return NULL;
    }

    Memory_Block* current = free_memory_list;

    // Traverse to find a block large enough
    while (current != NULL) 
    {
        if (current->free && current->size >= size) 
        {
            // Allocate new block from block pool
            Memory_Block* new_block = get_new_block();
            if (new_block == NULL) 
            {
                printf("Failed to allocate new block from block pool\n");
                return NULL;
            }

            // Split block if it's big enough
            if (current->size > size + sizeof(Memory_Block)) 
            {
                new_block->size = current->size - size - sizeof(Memory_Block);
                new_block->free = 1;
                new_block->next = current->next;
                current->next = new_block;
            }

            current->size = size;
            current->free = 0;

            memory_left -= size;  // Update remaining memory pool size

            return (void*)((char*)memory_pool + ((char*)current - (char*)block_pool));
        }
        current = current->next;
    }

    printf("Not enough memory (couldn't find a suitable block)\n");
    return NULL;
}

// Free allocated memory
void mem_free(void* block) 
{
    if (block == NULL) 
    {
        return;
    }

    // Get the memory block metadata associated with the block
    Memory_Block* mem_block = (Memory_Block*)block - 1;
    mem_block->free = 1;
    memory_left += mem_block->size;  // Return memory to the pool

    // Merge adjacent free blocks
    Memory_Block* current = free_memory_list;
    while (current != NULL) 
    {
        if (current->free && current->next && current->next->free) 
        {
            // Merge adjacent free blocks
            current->size += sizeof(Memory_Block) + current->next->size;
            current->next = current->next->next;
        }
        current = current->next;
    }
}

// Resize allocated memory block
void* mem_resize(void* block, size_t new_size) 
{
    if (block == NULL) 
    {
        return mem_alloc(new_size);
    }

    Memory_Block* mem_block = (Memory_Block*)block - 1;

    if (mem_block->size >= new_size) 
    {
        return block;  // No need to resize if the block is already large enough
    }

    // Allocate new memory and copy data
    void* new_block = mem_alloc(new_size);
    if (new_block != NULL) 
    {
        memcpy(new_block, block, mem_block->size);
        mem_free(block);  // Free the old block
    }

    return new_block;
}

// Free everything
void mem_deinit() 
{
    free(memory_pool);
    free(block_pool);
    memory_pool = NULL;
    block_pool = NULL;
    free_memory_list = NULL;
    memory_left = 0;
    block_pool_left = 0;
}
