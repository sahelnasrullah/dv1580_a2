#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Define structure for memory block metadata
typedef struct Memory_Block 
{
    size_t size;          // Size of the allocated memory block (in the memory pool)
    int free;             // 1 means free, 0 means it's in use
    struct Memory_Block* next; // Pointer to the next block
} Memory_Block;

void* memory_pool = NULL;        
size_t memory_left = 0;          // Memory left in the memory pool

void* md_pool = NULL;         
Memory_Block* md_pool_left = NULL;      // Pointer to next available metadata block
Memory_Block* free_memory_list = NULL;  // Pointer to free memory blocks

pthread_mutex_t memory_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize with total size of memory pool
void mem_init(size_t size) 
{
    pthread_mutex_lock(&memory_mutex);
    
    if (size == 0) 
    {
        printf("Error: Can't initialize memory pool with size 0\n");
        pthread_mutex_unlock(&memory_mutex);
        return;
    }

    // Allocate memory pools
    memory_pool = malloc(size);
    md_pool = malloc(1000 * sizeof(Memory_Block));
    
    if (memory_pool == NULL || md_pool == NULL) 
    {
        printf("Failed to allocate memory pool or block pool\n");
        pthread_mutex_unlock(&memory_mutex);
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

    pthread_mutex_unlock(&memory_mutex);
}

void* mem_alloc(size_t size) 
{
    pthread_mutex_lock(&memory_mutex);
    
    if (size == 0 || memory_left < size) 
    {
        pthread_mutex_unlock(&memory_mutex);
        return NULL;
    }

    // Check if we've exceeded the metadata pool
    if ((char*)md_pool_left >= ((char*)md_pool + 1000 * sizeof(Memory_Block))) 
    {
        printf("No more metadata blocks available\n");
        pthread_mutex_unlock(&memory_mutex);
        return NULL;
    }

    Memory_Block* current = free_memory_list;
    size_t current_offset = 0;  // Track position in memory_pool
    void* result = NULL;

    while (current != NULL) 
    {
        if (current->free && current->size >= size) 
        {
            // Split block if it's significantly larger
            if (current->size > size + sizeof(Memory_Block)) 
            {
                // Get new metadata block from md_pool
                Memory_Block* new_md = md_pool_left;
                md_pool_left = (Memory_Block*)((char*)new_md + sizeof(Memory_Block));

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
            result = (char*)memory_pool + current_offset;
            break;
        }

        // Move to next block, updating offset
        current_offset += current->size;
        current = current->next;
    }

    pthread_mutex_unlock(&memory_mutex);
    return result;
}

void mem_free(void* block) 
{
    if (block == NULL) 
    {
        return;
    }

    pthread_mutex_lock(&memory_mutex);

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
                    
                    // Return metadata block to pool by moving pointer back
                    md_pool_left = (Memory_Block*)((char*)md_pool_left - sizeof(Memory_Block));
                }
                iter = iter->next;
            }
            break;
        }
        current_offset += current->size;
        current = current->next;
    }

    pthread_mutex_unlock(&memory_mutex);
}

void* mem_resize(void* block, size_t new_size) 
{
    pthread_mutex_lock(&memory_mutex);

    if (block == NULL) 
    {
        void* result = mem_alloc(new_size);
        pthread_mutex_unlock(&memory_mutex);
        return result;
    }

    // Find the metadata block for this memory
    Memory_Block* current = free_memory_list;
    size_t offset = (char*)block - (char*)memory_pool;
    size_t current_offset = 0;
    void* result = NULL;

    // Find the corresponding metadata block
    while (current != NULL) 
    {
        if (current_offset == offset) 
        {
            if (current->size >= new_size) 
            {
                result = block; // Current block is big enough
                break;
            }

            // Need to allocate new block
            void* new_block = mem_alloc(new_size);
            if (new_block != NULL) 
            {
                memcpy(new_block, block, current->size);
                mem_free(block);
                result = new_block;
            }
            break;
        }
        current_offset += current->size;
        current = current->next;
    }

    pthread_mutex_unlock(&memory_mutex);
    return result;
}

void mem_deinit() 
{
    pthread_mutex_lock(&memory_mutex);
    
    free(memory_pool);
    free(md_pool);
    memory_pool = NULL;
    md_pool = NULL;
    free_memory_list = NULL;
    memory_left = 0;
    md_pool_left = NULL;

    pthread_mutex_unlock(&memory_mutex);
    pthread_mutex_destroy(&memory_mutex);
}