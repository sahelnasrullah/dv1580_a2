#include "memory_manager.h"
#include "linked_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

// Synchronization primitives for thread safety.
pthread_rwlock_t list_rwlock = PTHREAD_RWLOCK_INITIALIZER; // Read-Write lock for read-heavy functions.
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;    // Mutex lock for write-heavy functions.


void list_init(Node** head, size_t size) {
    *head = NULL;
    mem_init(size);
}

void list_insert(Node** head, uint16_t data) {
    pthread_mutex_lock(&list_mutex); 

    Node* new_node = (Node*)mem_alloc(sizeof(Node));
    if (!new_node) {
        printf("Memory allocation failed\n");
        pthread_mutex_unlock(&list_mutex); 
        return;
    }

    new_node->data = data;
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
    } else {
        Node* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }

    pthread_mutex_unlock(&list_mutex); 
}

void list_insert_after(Node* prev_node, uint16_t data) {
    if (prev_node == NULL) {
        printf("Previous node cannot be NULL\n");
        return;
    }

    pthread_mutex_lock(&list_mutex);

    Node* new_node = (Node*)mem_alloc(sizeof(Node));
    if (!new_node) {
        printf("Memory allocation failed\n");
        pthread_mutex_unlock(&list_mutex); 
        return;
    }

    new_node->data = data;
    new_node->next = prev_node->next;
    prev_node->next = new_node;

    pthread_mutex_unlock(&list_mutex);
}

void list_insert_before(Node** head, Node* next_node, uint16_t data) {
    if (next_node == NULL) {
        printf("Next node cannot be NULL\n");
        return;
    }

    pthread_mutex_lock(&list_mutex); 

    Node* new_node = (Node*)mem_alloc(sizeof(Node));
    if (!new_node) {
        printf("Memory allocation failed\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }
    new_node->data = data;

    if (*head == next_node) {
        new_node->next = *head;
        *head = new_node;
    } else {
        Node* current = *head;
        while (current != NULL && current->next != next_node) {
            current = current->next;
        }

        if (current == NULL) {
            printf("The specified next node is not in the list\n");
            mem_free(new_node);
            pthread_mutex_unlock(&list_mutex);
            return;
        }

        new_node->next = next_node;
        current->next = new_node;
    }

    pthread_mutex_unlock(&list_mutex); 
}

void list_delete(Node** head, uint16_t data) {
    pthread_mutex_lock(&list_mutex); 

    if (*head == NULL) {
        printf("List is empty\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }

    Node* current = *head;
    Node* previous = NULL;

    while (current != NULL && current->data != data) {
        previous = current;
        current = current->next;
    }

    if (current == NULL) {
        printf("Data not found in the list\n");
        pthread_mutex_unlock(&list_mutex); 
        return;
    }

    if (previous == NULL) {
        *head = current->next;
    } else {
        previous->next = current->next;
    }

    mem_free(current);

    pthread_mutex_unlock(&list_mutex); 
}

Node* list_search(Node** head, uint16_t data) {
    pthread_rwlock_rdlock(&list_rwlock);

    Node* current = *head;
    while (current != NULL) {
        if (current->data == data) {
            pthread_rwlock_unlock(&list_rwlock);
            return current;
        }
        current = current->next;
    }

    pthread_rwlock_unlock(&list_rwlock);
    return NULL;
}

void list_display(Node** head) {
    pthread_rwlock_rdlock(&list_rwlock);

    Node* current = *head;
    printf("[");
    while (current != NULL) {
        printf("%u", current->data);
        if (current->next != NULL) {
            printf(", ");
        }
        current = current->next;
    }
    printf("]");

    pthread_rwlock_unlock(&list_rwlock);
}

void list_display_range(Node** head, Node* start_node, Node* end_node) {
    pthread_rwlock_rdlock(&list_rwlock);

    Node* current = start_node ? start_node : *head;
    printf("[");
    while (current != NULL && (end_node == NULL || current != end_node->next)) {
        printf("%u", current->data);
        if (current->next != NULL && current != end_node) {
            printf(", ");
        }
        current = current->next;
    }
    printf("]");

    pthread_rwlock_unlock(&list_rwlock); 
}

int list_count_nodes(Node** head) {
    pthread_rwlock_rdlock(&list_rwlock);

    int count = 0;
    Node* current = *head;
    while (current != NULL) {
        count++;
        current = current->next;
    }

    pthread_rwlock_unlock(&list_rwlock);
    return count;
}

void list_cleanup(Node** head) {
    pthread_mutex_lock(&list_mutex);

    Node* current = *head;
    while (current != NULL) {
        Node* next_node = current->next;
        mem_free(current);
        current = next_node;
    }
    *head = NULL;
    mem_deinit();

    pthread_mutex_unlock(&list_mutex);
}