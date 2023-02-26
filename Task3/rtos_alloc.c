#include <stddef.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include "rtos-alloc.h"

#define ALIGNMENT 4
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) / ALIGNMENT * ALIGNMENT)

#define HEADER_SIZE sizeof(void*)

typedef struct chunk_header {
    size_t c_size;
    struct chunk_header *c_left;
    struct chunk_header *c_right;
} chunk_header;

static chunk_header *free_tree = NULL;

static chunk_header *insert_free_chunk(chunk_header *tree, chunk_header *chunk);
static chunk_header *remove_free_chunk(chunk_header *tree, chunk_header *chunk);
static chunk_header *find_best_fit_chunk(chunk_header *tree, size_t size);

static chunk_header *allocated_list = NULL;
static chunk_header *free_list = NULL;

static chunk_header *find_free_chunk(size_t size);

static chunk_header *split_chunk(chunk_header *chunk, size_t size);

static void combine_chunks(chunk_header *chunk);

/*
 * Allocate memory of size `size` using best-fit algorithm.
 * Returns a pointer to the allocated memory or NULL if failed.
 */
void *rtos_malloc(size_t size) {
    // Align the requested size and add space for the chunk header
    size_t adjusted_size = ALIGN(size + sizeof(chunk_header));

    // Find the best-fit free chunk
    chunk_header *p = find_free_chunk(adjusted_size);

    // If no free chunk is big enough, allocate a new chunk
    if (!p) {
        // Calculate the size of the new chunk
        size_t page_size = 4096;
        size_t num_pages = (adjusted_size + sizeof(chunk_header) + ALIGNMENT + page_size - 1) / page_size;
        size_t total_size = num_pages * page_size;
        total_size += sizeof(chunk_header);
        total_size += ALIGNMENT;
        total_size += HEADER_SIZE;

        // Allocate the new chunk
        void *new_chunk = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (new_chunk == MAP_FAILED) {
            return NULL;
        }

        // Split the new chunk into an allocated chunk and a free chunk
        chunk_header *new_header = (chunk_header *) new_chunk;
        new_header->c_size = total_size;
        p = split_chunk(new_header, adjusted_size);
    }

    // Add the allocated chunk to the allocated list
    void *result = (void *) (p + 1);
    chunk_header *new_chunk = (chunk_header *) result - 1;
    new_chunk->c_left = allocated_list;
    allocated_list = new_chunk;

    return result;
}

/*
 * Reallocate memory pointed to by `ptr` to new size `size`.
 * Returns a pointer to the reallocated memory or NULL if failed.
 */
void *rtos_realloc(void *ptr, size_t size) {
    if (!ptr) {
        return rtos_malloc(size);
    }
    chunk_header *p = ((chunk_header *) ptr) - 1;
    size_t old_size = p->c_size - sizeof(chunk_header);
    size_t adjusted_size = ALIGN(size + sizeof(chunk_header));
    if (size <= old_size) {
        // Split the chunk into an allocated chunk and a free chunk, if necessary
        void *result = (void *) (p + 1);
        if (split_chunk(p, adjusted_size)) {
            // Add the allocated chunk to the allocated list
            chunk_header *new_chunk = (chunk_header *) result - 1;
            new_chunk->c_left = allocated_list;
            allocated_list = new_chunk;
        }
        return ptr;
    }

    // Allocate a new chunk
    void *new_chunk = rtos_malloc(size);
    if (!new_chunk) {
        return NULL;
    }

    // Copy the data from the old chunk to the new chunk
    memcpy(new_chunk, ptr, old_size);

    // Free the old chunk
    rtos_free(ptr);

    return new_chunk;
}

/*
 * Free memory pointed to by `ptr`.
 */
void rtos_free(void *ptr) {
    if (!ptr) {
        return;
    }
    chunk_header *p = ((chunk_header *) ptr) - 1;

    // Remove the allocated chunk from the list
    if (allocated_list == p) {
        allocated_list = allocated_list->c_left;
    } else {
        for (chunk_header *chunk = allocated_list; chunk != NULL; chunk = chunk->c_left) {
            if (chunk->c_left == p) {
                chunk->c_left = p->c_left;
                break;
            }
        }
    }

    // Add the freed chunk to the tree
    free_tree = insert_free_chunk(free_tree, p);

    combine_chunks(p);
}

/*
 * Returns the size of the memory block pointed to by `ptr`.
 * Returns 0 if `ptr` is NULL.
 */
size_t rtos_alloc_size(void *ptr) {
    if (!ptr) {
        return 0;
    }
    chunk_header *p = ((chunk_header *) ptr) - 1;
    return p->c_size - sizeof(chunk_header);
}

/*
 * Checks if the given pointer `ptr` was allocated by the allocator.
 * Returns true if `ptr` is a valid allocated pointer, false otherwise.
 */
bool rtos_allocated(void *ptr) {
    for (chunk_header *p = allocated_list; p != NULL; p = p->c_left) {
        if ((void *) (p + 1) == ptr) {
            return true;
        }
    }
    return false;
}

/*
 * Returns the total size of memory allocated by the allocator.
 */
size_t rtos_total_allocated(void) {
    size_t total = 0;

    // Include the size of each allocated chunk's user data in the total
    for (chunk_header *p = allocated_list; p != NULL; p = p->c_left) {
        total += p->c_size - sizeof(chunk_header);
    }
    return total;
}

/*
* Find the best-fit free chunk with size >= size.
* Returns a pointer to the best-fit chunk or NULL if none found.
*/
static chunk_header *find_free_chunk(size_t size) {
    chunk_header *best = find_best_fit_chunk(free_tree, size);
    if (best != NULL) {
        free_tree = remove_free_chunk(free_tree, best);
    }
    return best;
}


/*
*Split a chunk into two smaller chunks, one of size size and the other with remaining size.
*Returns a pointer to the new chunk.
*/
static chunk_header *split_chunk(chunk_header *chunk, size_t size) {
    // Align the requested size and the remaining size before calculating them
    size_t aligned_size = ALIGN(size);
    size_t remaining_size = ALIGN(chunk->c_size) - aligned_size - sizeof(chunk_header);

    if (remaining_size < sizeof(chunk_header)) {
        aligned_size += remaining_size;
        remaining_size = 0;
    }

    // Update the size of the original chunk
    chunk->c_size = aligned_size;
    chunk->c_left = chunk->c_right = NULL;

    if (remaining_size > 0) {
        // Create a new chunk from the remaining space
        chunk_header *new_chunk = (chunk_header *) ((char *) chunk + sizeof(chunk_header) + aligned_size);
        new_chunk->c_size = remaining_size;
        new_chunk->c_left = new_chunk->c_right = NULL;

        // If the new chunk is nonempty, insert it into the tree
        free_tree = insert_free_chunk(free_tree, new_chunk);
        combine_chunks(new_chunk);
    }

    return chunk;
}

/*
* Combine a chunk with its adjacent free chunks, if any.
*/
static void combine_chunks(chunk_header *chunk) {
    while (chunk->c_left != NULL && (char *) chunk + sizeof(chunk_header) + chunk->c_size == (char *) chunk->c_left) {
        chunk_header *left = chunk->c_left;
        chunk = split_chunk(chunk, chunk->c_size + left->c_size + sizeof(chunk_header));
        free_tree = remove_free_chunk(free_tree, left);
    }
    while (chunk->c_right != NULL && (char *) chunk + sizeof(chunk_header) + chunk->c_size == (char *) chunk->c_right) {
        chunk_header *right = chunk->c_right;
        chunk = split_chunk(chunk, chunk->c_size + right->c_size + sizeof(chunk_header));
        free_tree = remove_free_chunk(free_tree, right);
    }
}

static chunk_header *find_best_fit_chunk(chunk_header *tree, size_t size) {
    chunk_header *best = NULL;

    while (tree != NULL) {
        size_t aligned_size = ALIGN(tree->c_size);
        if (aligned_size >= size && (best == NULL || aligned_size < ALIGN(best->c_size))) {
            best = tree;
        }
        if (aligned_size >= size) {
            tree = tree->c_left;
        } else {
            tree = tree->c_right;
        }
    }

    return best;
}

/*
 * Insert a free chunk into the free tree.
 */
static chunk_header *insert_free_chunk(chunk_header *tree, chunk_header *chunk) {
    if (tree == NULL) {
        return chunk;
    }
    if (chunk->c_size <= tree->c_size) {
        tree->c_left = insert_free_chunk(tree->c_left, chunk);
    } else {
        tree->c_right = insert_free_chunk(tree->c_right, chunk);
    }
    return tree;
}

/*

Remove a free chunk from the free tree.
*/
static chunk_header *remove_free_chunk(chunk_header *tree, chunk_header *chunk) {
    if (tree == NULL) {
        return NULL;
    }
    if (chunk == tree) {
        if (tree->c_left == NULL) {
            return tree->c_right;
        }
        if (tree->c_right == NULL) {
            return tree->c_left;
        }
        chunk_header *min_right = tree->c_right;
        while (min_right->c_left != NULL) {
            min_right = min_right->c_left;
        }
        min_right->c_left = tree->c_left;
        return tree->c_right;
    }
    if (chunk->c_size <= tree->c_size) {
        tree->c_left = remove_free_chunk(tree->c_left, chunk);
    } else {
        tree->c_right = remove_free_chunk(tree->c_right, chunk);
    }
    return tree;
}