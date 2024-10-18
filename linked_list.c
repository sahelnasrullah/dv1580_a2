#include "memory_manager.h"
#include "linked_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>



// Initialize list w pointer & initialize memory pool if not already
void list_init(Node** head, size_t node_size) {
    if (memory_pool == NULL) {  
        size_t poolSize = node_size * 100;
        mem_init(node_size);
        printf("Memory pool initialized in list_init\n");
    }
    *head = NULL;  // Initialize the head of the list to NULL
}

// Insert a new node
void list_insert(Node** head, int data) {
    // Allocate memory for a new node
    Node* new_node = (Node*)mem_alloc(sizeof(Node));
    if (new_node == NULL) {
        printf("Failed to allocate memory for new node (list_insert)\n");
        return;
    }

    new_node->data = data;
    new_node->next = NULL;

    if (*head == NULL) 
    {
        *head = new_node;
    } 
    else 
    {
        Node* current = *head;
        while (current->next != NULL) 
        {
            current = current->next;
        }
        current->next = new_node;  // Link the new node at the end of the list
    }
}

// Insert new node after a given node
void list_insert_after(Node* prev_node, int data) {
    if (prev_node == NULL) {  // Ensure the previous node is not NULL
        return;
    }

    // Allocate memory new node
    Node* new_node = (Node*)mem_alloc(sizeof(Node));
    if (new_node == NULL) {
        printf("Failed to allocate memory for new node (list_insert_after)\n");
        return;
    }

    new_node->data = data;
    new_node->next = prev_node->next;
    prev_node->next = new_node;
}

// Insert node before given node in list
void list_insert_before(Node** head, Node* next_node, int data) {
    if (*head == NULL || next_node == NULL) {
        return;
    }

    if (*head == next_node) { 
        Node* new_node = (Node*)mem_alloc(sizeof(Node));
        if (new_node == NULL) {
            printf("Failed to allocate memory for new node (list_insert_before)\n");
            return;
        }
        new_node->data = data;
        new_node->next = *head;  // Link new node to current head
        *head = new_node;  // Update pointer
        return;
    }

    // Traverse, find the node before the next_node
    Node* current = *head;
    while (current != NULL && current->next != next_node) {
        current = current->next;
    }

    if (current != NULL) {
        Node* new_node = (Node*)mem_alloc(sizeof(Node));
        if (new_node == NULL) {
            printf("Failed to allocate memory for new node (list_insert_before)\n");
            return;
        }

        new_node->data = data;
        new_node->next = next_node;
        current->next = new_node;
    }
}

// Delete node
void list_delete(Node** head, int data) {
    if (*head == NULL) {
        return;
    }

    Node* current = *head;
    Node* prev = NULL;

    // Delete head
    if (current != NULL && current->data == data) {
        *head = current->next;  // Update the head pointer to the next node
        mem_free(current);  // Free the memory of the deleted node
        return;
    }

    // Find data to delete
    while (current != NULL && current->data != data) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) { 
        return;
    }

    prev->next = current->next;  // Remove node from lsit
    mem_free(current);  // Free memory of deleted node
}

// Search by value
Node* list_search(Node** head, int data) {
    Node* current = *head;

    while (current != NULL) {
        if (current->data == data) {
            return current;
        }

        current = current->next;
    }
    return NULL;
}

// Display list
void list_display(Node** head) {
    if (*head == NULL) {
        printf("[]\n");
        return;
    }

    Node* current = *head;
    printf("[");

    // Traverse & print each node data
    while (current != NULL) {
        printf("%d", current->data);
        
        if (current->next != NULL) {
            printf(", ");
        }
        current = current->next;
    }
    printf("]\n");
}

// Display nodes from start to end
void list_display_range(Node** head, Node* start_node, Node* end_node) {
    if (*head == NULL) {
        printf("[]");
        return;
    }

    // If start_node is not provided, start from the head
    Node* current = *head;
    if (start_node != NULL) {
    current = start_node;
}
    printf("[");
    
    // Traverse list & print each node
    while (current != NULL) {
        printf("%d", current->data);
        
        if (current == end_node) {
            break;
        }
        
        if (current->next != NULL) {
            printf(", ");
        }
        
        current = current->next;
    }
    
    printf("]");
}

// Count number of nodes
int list_count_nodes(Node** head) {
    int node_counter = 0;
    Node* current = *head;

    while (current != NULL) {
        node_counter++;
        current = current->next;
    }
    return node_counter;
}

// Clean and free all nodes
void list_cleanup(Node** head) {
    Node* current = *head;

    // free each node
    while (current != NULL) {
        Node* next = current->next;
        mem_free(current);
        current = next;
    }
    *head = NULL;  // Pointer set to NULL
}