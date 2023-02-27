#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>

#define ALIGNMENT 16   // Must be power of 2
#define GET_PAD(x) ((ALIGNMENT - 1) - (((x) - 1) & (ALIGNMENT - 1)))

#define PADDED_SIZE(x) ((x) + GET_PAD(x))

#define PTR_OFFSET(p, offset) ((void*)((char *)(p) + (offset)))

struct block *head = NULL;

struct block {
    struct block *next;
    int size;
    int in_use;
};

int padded_size_of_block = PADDED_SIZE(sizeof(struct block));

void split_space(struct block *current_node, int requested_size) {
    struct block *new_node = PTR_OFFSET(current_node, requested_size + padded_size_of_block);
    new_node->size = current_node->size - requested_size - padded_size_of_block;
    new_node->in_use = 0;
    new_node->next = current_node->next;
    current_node->next = new_node;
    current_node->size = requested_size;
    current_node->in_use = 1;
}

void myfree(void *p) {
    struct block *accessed_node = p - padded_size_of_block;
    accessed_node->in_use = 0;
}

void *myalloc(int size) {
    int padded_node_size = PADDED_SIZE(size);
    int required_space = padded_node_size + padded_size_of_block + 16;
    if (head == NULL) {
        head = mmap(NULL, 1024, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
        head->next = NULL;
        head->size = 1024 - padded_size_of_block;
        head->in_use = 0;
    }

    struct block *current_node = head;
    while (current_node != NULL) {
        if (current_node->size >= padded_node_size && !current_node->in_use) {
            if (current_node->size >= required_space) {
                split_space(current_node, padded_node_size);
            }
            current_node->in_use = 1;
            int padded_block_size = padded_size_of_block;
            return PTR_OFFSET(current_node, padded_block_size);
        }
        current_node = current_node->next;
    }
    return NULL;
}

void print_data(void)
{
    struct block *b = head;

    if (b == NULL) {
        printf("[empty]\n");
        return;
    }

    while (b != NULL) {
        // Uncomment the following line if you want to see the pointer values
        //printf("[%p:%d,%s]", b, b->size, b->in_use? "used": "free");
        printf("[%d,%s]", b->size, b->in_use? "used": "free");
        if (b->next != NULL) {
            printf(" -> ");
        }

        b = b->next;
    }

    printf("\n");
}

int main(void) {
    void *p;

    myalloc(10);     print_data();
    p = myalloc(20); print_data();
    myalloc(30);     print_data();
    myfree(p);       print_data();
    myalloc(40);     print_data();
    myalloc(10);     print_data();
}

