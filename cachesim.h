#ifndef CACHESIM_H
#define CACHESIM_H

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

typedef enum{
    I_LOAD,
    D_LOAD,
    D_STORE,
    D_MODIFY,
    INVALID
} FLAG; 

typedef struct{
    int hit; // 0 = false, 1 = true;
    int miss;
    int eviction;
} cache_result;

typedef struct{
    FLAG flag;
    long long unsigned int addr;
    int size;
} cache_line;

typedef struct {
    int S; // num sets (2^ set idx bits)
    int s; // set index bits
    int B; // block size (2^block bits)
    int b; // block bits (offset)
    int E; // Associativity
} cache_params;

typedef struct{
    int v_bit; // valid bit
    int tag; // cache tag
    clock_t tstamp; // for lru eviction
} cache_block;

cache_params init_cache_params(int idx_bits, int associativity, int block_bits){ // s E b
    int num_sets = 1 << idx_bits;
    int block_size = 1 << block_bits;
    cache_params params = {num_sets, idx_bits, block_size, block_bits, associativity};
    return params;
}

cache_line init_cache_line(char* line){
     // remove leading whitespace if needed
    if (isspace((unsigned char) line[0]))
    {
        int i = 0;
        while (line[i] != '\0') 
        {
            line[i] = line[i + 1];
            i++;
        }
    }

    char flag;
    long long unsigned int address;
    int size;

    sscanf(line, "%c %llx,%d", &flag, &address, &size);
    
    FLAG fl;
    switch (flag){
        case 'I':
            fl = I_LOAD;
            break;
        case 'L':
            fl = D_LOAD;
            break;
        case 'S':
            fl = D_STORE;
            break;
        case 'M':
            fl = D_MODIFY;
            break;
        default:
            fl = INVALID;
            break;
    }

    cache_line curr_line = {fl, address, size};
    return curr_line;
}

cache_block* init_cache(int size){
    cache_block* cache = (cache_block*) malloc(size*sizeof(cache_block));

    for (int i = 0; i < size; i++){
        cache[i].tag = 0;
        cache[i].v_bit = 0;
        cache[i].tstamp = 0;
    }

    return cache;
}

void free_cache(cache_block* cache){
    free(cache);
}

cache_result simulate_cache_read_write(cache_block* cache, cache_params params, cache_line line){
    int addr =          line.addr;
    int block_offset =  params.b;
    int set_index =     params.s;
    int associtivity =  params.E;

    cache_result result = {0, 0, 0};

    set_index = (addr >> block_offset) & ((1 << set_index) - 1); // remove offset bits and then mask out all bits exvept set index bits
    int tag = addr >> (set_index + block_offset); // shift to get only tag bits (addr - set_idx - offset = tag)

    cache_block* set = cache + (set_index * associtivity); // simulate finding cache set 
    for (int block_num = 0; block_num < associtivity; block_num ++){ // iterate through cache set
        if (set[block_num].v_bit == 1 && set[block_num].tag == tag){ // if found return a cache hit
            result.hit = 1;
            set[block_num].tstamp = clock();
            return result;
        }else if (set[block_num].v_bit == 0){ // miss
            set[block_num].tag = tag;
            set[block_num].v_bit = 1;
            set[block_num].tstamp = clock();
            result.miss =  1;
            return result;
        }
    }

    // cache miss and cache is full so we do an eviction
    // find LRU item in lru list and remove, then find it in the cache 
    clock_t lru_tstamp = set[0].tstamp;
    int lru_idx = 0;

    for (int i = 0; i < associtivity; i++){
        if (set[i].tstamp < lru_tstamp){
            lru_tstamp = set[i].tstamp;
            lru_idx = i;
        }
    }

    set[lru_idx].tstamp = clock();
    set[lru_idx].tag = tag;

    result.miss = 1;
    result.eviction = 1;
    return result;

}

void eval_cache(int* hits, int* miss, int* evict, cache_result result){
    *hits += result.hit;
    *miss += result.miss;
    *evict += result.eviction;
}

void verbose(int v, cache_line line, cache_result result){
    if (!v) return;

    if (line.flag == I_LOAD){
        printf("I ");
    }else if (line.flag == D_LOAD){
        printf("L ");
    }else if (line.flag == D_STORE){
        printf("S ");
    }else{
        printf("M ");
    }

    printf("%llx, %d ", line.addr, line.size);

    
    if (result.miss == 1)
        printf("miss ");
    if (result.eviction == 1)
        printf("eviction ");
    if (result.hit == 1)
        printf("hit ");

    printf("\n");
        
}


#endif