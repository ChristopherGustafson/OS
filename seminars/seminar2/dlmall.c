#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>

struct head {
    uint16_t bfree;
    uint16_t bsize;
    uint16_t free;
    uint16_t size;
    struct head *next;
    struct head *prev;
};

struct taken{
    uint16_t bfree;
    uint16_t bsize;
    uint16_t free;
    uint16_t size;
};

#define TRUE 1
#define FALSE 0

#define HEAD (sizeof(struct head)) 

#define MIN(size) (((size)>(8))?(size):(8))

#define LIMIT(size) (MIN(0) + HEAD + size)

#define MAGIC(memory) ((struct head*)memory - 1)

#define HIDE(block) (void*)((struct head* )block + 1)

#define ALIGN 8

#define ARENA (64*1024)

struct head *flist;
struct head *arena = NULL;


int printFlist() {
    int count = 0;
    int sum = 0;
    struct head *iter = flist;
    while(iter != NULL){
        //printf("Size of element %d in free list: %d\n", count, iter->size);
        sum += iter->size;
        count++;
        iter = iter->next;
    }
    printf("Number of elements in the free list: %d\n", count);
    printf("Avarage block size: %d\n", sum/count);
    return count;
}

//Return pointer to the block after (higher address) the given block in memory
struct head *after(struct head *block) {
    return (struct head*)((char*)block + HEAD + block->size);
}

//Return pointer to the block before (lower address) the given block in memory
struct head *before(struct head *block){
    return (struct head*)((char*)block - HEAD - block->bsize);
}


struct head *split(struct head *block, int size){
    int remSize = block->size - size - HEAD;
    block->size =  size;

    struct head *splt = after(block);
    splt->bsize = block->size;
    splt->bfree = block->free;
    splt->size = remSize;
    splt->free = TRUE;

    struct head *aft = after(splt);
    aft->bsize = splt->size;
    aft->bfree = splt->free;

    return splt;
}


struct head *new(){

    if(arena != NULL) {
        //printf("One arena already allocated \n");
        return NULL;
    }

    struct head *new = mmap(NULL, ARENA, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(new == MAP_FAILED) {
        printf("mmap failed: error %d\n", errno);
        return NULL;
    }

    // make room for head and sentinel (blocking memory merge in end of arena)
    uint16_t size = ARENA - 2*HEAD;

    new->bfree = FALSE;
    new->bsize = 0;
    new->free = TRUE;
    new->size = size;

    flist = new;

    struct head *sentinel = after(new);

    sentinel->bfree = TRUE;
    sentinel->bsize = size;
    sentinel->free = FALSE;
    sentinel->size = 0;

    return new;
}

void detatch(struct head *block) {

    if(block->next != NULL){
        block->next->prev = block->prev;
    }
    
    if(block->prev != NULL){
        block->prev->next = block->next;
    }
    else{
        flist = block->next;
    }

}

//Insert block at the first position in the free list, set flist pointer to this block
void insert(struct head *block) {
    block->next = flist;
    block->prev = NULL;
    if(flist != NULL){
        flist->prev = block;
    }
    flist = block; 

    //printf("inserted block into free list with size %d\n", block->size);
}

//Adjust the request to be more or equal to MIN and aligned to ALIGN
int adjust(size_t request){
    int size = MIN(request);
    while(size%ALIGN != 0){
        size++;
    }
    return size;
}

struct head *find(int size){
    struct head *current = flist;
    while(current->size < size){
        current = current->next;
        if(current == NULL){
            return NULL;
        }
    }

    if(current->size >= LIMIT(size)){
        struct head *splt = split(current, size);
        insert(splt);
        current->free = FALSE;
        after(current)->bfree = FALSE;
        detatch(current);
    }
    else{
        current->free = FALSE;
        after(current)->bfree = FALSE;
        detatch(current);
    }

    return current;
}

struct head *merge(struct head *block){

    struct head *aft = after(block);

    if(block->bfree) {
        //Unlink the block before
        struct head *bef = before(block);

        detatch(bef);

        // calculate and set the total size of merged blocks
        int size = bef->size + HEAD + block->size;
        bef->size = size;
        //update the block after the merged blocks
        aft->bsize = size;
        aft->bfree = TRUE;
        //Continue with the merged block
        block = bef;
    }



    if(aft->free) {
        //Unlink the block
        detatch(aft);
        //Calculate and set total size of merged blocks
        int size = block->size + HEAD + aft->size;
        block->size = size;
        //Update the block after the merged block
        struct head *aft2 = after(aft);
        aft2->bsize = size;
        aft2->bfree = TRUE;
    }
    return block;
}

void *dalloc(size_t request){

    if(arena == NULL){
        arena = new();
    }
    
    if(flist == NULL){
        flist = arena;
    }

    if(request <= 0){
        return NULL;
    }

    int size = adjust(request);
    struct head *taken = find(size);

    if(taken == NULL){
        return NULL;
    }
    else{
        return HIDE(taken);
    }
}

void dfree(void *memory) {

    if(memory != NULL) {

        struct head *block = MAGIC(memory);

        block = merge(block);

        struct head *aft = after(block);
        block->free = TRUE;
        aft->bfree = TRUE;

        insert(block);
    }
    return;
}
