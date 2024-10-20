#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

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
    md_pool = malloc(size);
    
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
    
    // Initial validation checks
    if (size == 0) 
    {
        size = sizeof(Memory_Block); // Allocate minimum usable size
    }

    // Track total allocated memory across all threads
    static size_t total_allocated = 0;
    
    // Check if this allocation would exceed total memory or available memory
    if (total_allocated + size > memory_pool || memory_left < size) 
    {
        pthread_mutex_unlock(&memory_mutex);
        return NULL;
    }

    // Check metadata pool availability
    if ((char*)md_pool_left >= ((char*)md_pool + 1000 * sizeof(Memory_Block))) 
    {
        printf("No more metadata blocks available\n");
        pthread_mutex_unlock(&memory_mutex);
        return NULL;
    }

    Memory_Block* current = free_memory_list;
    size_t current_offset = 0;
    void* result = NULL;
    bool found_suitable_block = false;

    // Verify total available contiguous memory
    size_t largest_free_block = 0;
    Memory_Block* temp = free_memory_list;
    while (temp != NULL) {
        if (temp->free && temp->size > largest_free_block) {
            largest_free_block = temp->size;
        }
        temp = temp->next;
    }

    // If no block is large enough, fail fast
    if (largest_free_block < size) {
        pthread_mutex_unlock(&memory_mutex);
        return NULL;
    }

    // Search for suitable block
    while (current != NULL) 
    {
        // Check if we're still within memory pool bounds
        if (current_offset >= memory_pool) 
        {
            pthread_mutex_unlock(&memory_mutex);
            return NULL;
        }

        if (current->free && current->size >= size) 
        {
            found_suitable_block = true;

            // Verify this allocation won't exceed memory pool
            if (current_offset + size > memory_pool) 
            {
                pthread_mutex_unlock(&memory_mutex);
                return NULL;
            }

            // Split block if it's significantly larger
            if (current->size > size + sizeof(Memory_Block)) 
            {
                // Get new metadata block
                Memory_Block* new_md = md_pool_left;
                md_pool_left = (Memory_Block*)((char*)new_md + sizeof(Memory_Block));

                // Ensure metadata pointers are valid
                if (!new_md || !current)
                {
                    pthread_mutex_unlock(&memory_mutex);
                    return NULL;
                }

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
            total_allocated += size;  // Update total allocated memory
            result = (char*)memory_pool + current_offset;
            break;
        }

        // Move to next block
        current_offset += current->size;
        current = current->next;
    }

    // If no suitable block found, return NULL
    if (!found_suitable_block) 
    {
        pthread_mutex_unlock(&memory_mutex);
        return NULL;
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
                    // Ensure the iter and next pointers are valid before merging
                    if (!iter || !iter->next)
                    {
                        printf("Error: Invalid memory block pointer during merge in mem_free.\n");
                        pthread_mutex_unlock(&memory_mutex);
                        return;
                    }

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
    Memory_Block* prev = NULL;
    size_t offset = (char*)block - (char*)memory_pool;
    size_t current_offset = 0;
    void* result = NULL;

    // Find the corresponding metadata block
    while (current != NULL) 
    {
        if (current_offset == offset) 
        {
            // Ensure current and prev pointers are valid
            if (!current || (prev && !prev->next))
            {
                printf("Error: Invalid memory block pointer in mem_resize.\n");
                pthread_mutex_unlock(&memory_mutex);
                return NULL;
            }

            // Current block is big enough
            if (current->size >= new_size) 
            {
                // Split block if it's significantly larger
                if (current->size > new_size + sizeof(Memory_Block)) 
                {
                    Memory_Block* new_block = (Memory_Block*)((char*)current + new_size);

                    // Ensure new_block pointer is valid
                    if (!new_block)
                    {
                        printf("Error: Invalid new_block pointer in mem_resize.\n");
                        pthread_mutex_unlock(&memory_mutex);
                        return NULL;
                    }

                    new_block->size = current->size - new_size;
                    new_block->free = 1;
                    new_block->next = current->next;
                    
                    current->size = new_size;
                    current->next = new_block;
                }
                result = block;
                break;
            }
            
            // Check if merging with the next block is possible and sufficient
            if (current->next && current->next->free && 
                (current->size + current->next->size >= new_size)) 
            {
                // Merge with next block
                current->size += current->next->size;
                current->next = current->next->next;
                
                // If the merged block is significantly larger, split it
                if (current->size > new_size + sizeof(Memory_Block)) 
                {
                    Memory_Block* new_block = (Memory_Block*)((char*)current + new_size);

                    // Ensure new_block pointer is valid
                    if (!new_block)
                    {
                        printf("Error: Invalid new_block pointer after merging in mem_resize.\n");
                        pthread_mutex_unlock(&memory_mutex);
                        return NULL;
                    }

                    new_block->size = current->size - new_size;
                    new_block->free = 1;
                    new_block->next = current->next;
                    
                    current->size = new_size;
                    current->next = new_block;
                }
                result = block;
                break;
            }

            // If we can't resize in place, allocate a new block and copy
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
        prev = current;
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
