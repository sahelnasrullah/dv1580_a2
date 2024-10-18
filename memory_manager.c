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

void* memory_pool = NULL;        
void* md_pool = NULL;         
Memory_Block* free_memory_list = NULL;  // Pointer to free memory blocks
size_t memory_left = 0;          // Memory left in the memory pool
size_t md_pool_left = 0;      // Blocks left in the block pool

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
    md_pool = malloc(1000 * sizeof(Memory_Block));
    
    if (memory_pool == NULL || md_pool == NULL) 
    {
        printf("Failed to allocate memory pool or block pool\n");
        return;
    }

    // Initialize first metadata block to track entire memory pool
    free_memory_list = (Memory_Block*)md_pool;
    free_memory_list->size = size;
    free_memory_list->free = 1;
    free_memory_list->next = NULL;

    // Set initial state
    memory_left = size;
    md_pool_left = (Memory_Block*)((char*)md_pool + sizeof(Memory_Block)); // Point to next available metadata block
}

void* mem_alloc(size_t size) 
{
    if (size == 0 || memory_left < size) 
    {
        return NULL;
    }

    if (md_pool_left >= (Memory_Block*)((char*)md_pool + 1000 * sizeof(Memory_Block))) 
    {
        printf("No more metadata blocks available\n");
        return NULL;
    }

    Memory_Block* current = free_memory_list;
    size_t current_offset = 0;  // Track position in memory_pool

    while (current != NULL) 
    {
        if (current->free && current->size >= size) 
        {
            // Split block if it's significantly larger
            if (current->size > size + sizeof(Memory_Block)) 
            {
                // Get new metadata block from md_pool
                Memory_Block* new_md = md_pool_left;
                md_pool_left = (Memory_Block*)((char*)md_pool_left + sizeof(Memory_Block));

                // Setup new free block metadata
                new_md->size = current->size - size;
                new_md->free = 1;
                new_md->next = current->next;

                // Update current block
                current->size = size;
                current->free = 0;
                current->next = new_md;
            } 
            else 
            {
                // Use entire block if split wouldn't be worth it
                current->free = 0;
            }

            memory_left -= size;
            
            // Return memory location from memory_pool
            return (char*)memory_pool + current_offset;
        }

        // Move to next block, updating offset
        current_offset += current->size;
        current = current->next;
    }

    return NULL;
}

void mem_free(void* block) 
{
    if (block == NULL) 
    {
        return;
    }

    // Calculate position in memory_pool
    size_t block_offset = (char*)block - (char*)memory_pool;
    
    // Find corresponding metadata
    Memory_Block* current = free_memory_list;
    size_t current_offset = 0;

    while (current != NULL) 
    {
        if (current_offset == block_offset) 
        {
            current->free = 1;
            memory_left += current->size;

            // Merge adjacent free blocks
            Memory_Block* iter = free_memory_list;
            while (iter != NULL && iter->next != NULL) 
            {
                if (iter->free && iter->next->free) 
                {
                    // Combine blocks
                    iter->size += iter->next->size;
                    iter->next = iter->next->next;
                    
                    // Return metadata block to pool
                    md_pool_left = (Memory_Block*)((char*)md_pool_left - sizeof(Memory_Block));
                }
                iter = iter->next;
            }
            return;
        }
        current_offset += current->size;
        current = current->next;
    }
}

void* mem_resize(void* block, size_t new_size) 
{
    if (block == NULL) 
    {
        return mem_alloc(new_size);
    }

    // Find the metadata block for this memory
    Memory_Block* current = free_memory_list;
    size_t offset = (char*)block - (char*)memory_pool;
    size_t current_offset = 0;

    // Find the corresponding metadata block
    while (current != NULL) 
    {
        if (current_offset == offset) 
        {
            if (current->size >= new_size) 
            {
                return block; // Current block is big enough
            }

            // Need to allocate new block
            void* new_block = mem_alloc(new_size);
            if (new_block != NULL) 
            {
                memcpy(new_block, block, current->size);
                mem_free(block);
            }
            return new_block;
        }
        current_offset += current->size;
        current = current->next;
    }
    return NULL;
}

void mem_deinit() 
{
    free(memory_pool);
    free(md_pool);
    memory_pool = NULL;
    md_pool = NULL;
    free_memory_list = NULL;
    memory_left = 0;
    md_pool_left = NULL;
}