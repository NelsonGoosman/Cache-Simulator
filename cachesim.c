#include <getopt.h>
#include <stdlib.h>
#include <unistd.h> 
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include "cachesim.h"


#define ADDRESS_LENGTH 64  // 64-bit memory addressing
#define BUFFER_SIZE 256    // Line buffer for reading from file


/* 
 * this function provides a standard way for your cache
 * simulator to display its final statistics (i.e., hit and miss)
 */ 
void print_summary(int hits, int misses, int evictions)
{
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
}

/*
 * print usage info
 */
void print_usage(char* argv[])
{
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/trace01.dat\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/trace01.dat\n", argv[0]);
    exit(0);
}


int main(int argc, char* argv[])
{
    // hit miss and eviction stat counters
    int hit_count = 0, miss_count = 0, eviction_count = 0;
    
    // init command line variables
    int s = 0, E = 0, b = 0, verbose_flg = 0;
    int c; // getopt arg
    char* t = NULL; // tracefile


    while ((c=getopt(argc, argv, "s:E:b:t:vh")) != -1){
        switch (c){
            case 'h': 
                print_usage(argv);
                exit(0);
            case 's':
                s = atoi(optarg);
                break;
            case 'v':
                verbose_flg = 1;
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                t = optarg;
                break; 
            case '?':
                fprintf(stderr, "Error: invalid or missing argument detected. Exiting\n");
                exit(1);
            default:
                fprintf(stderr, "Error: invalid or missing argument detected. Exiting\n");
                exit(1);
        }
    } 

    if (s <= 0 || E <= 0 || b <= 0){
        fprintf(stderr, "Error: s, E, or b have been set to less than or equal to 0. Exiting\n");
        exit(1);
    }
    if (t == NULL){
        fprintf(stderr, "Error: T is null\n");
        exit(1);
    }

    
    FILE* tracefile = fopen(t, "r");
    if (tracefile == NULL){
        fprintf(stderr, "File not found or unable to open file");
        exit(1);
    }

    // set the cache parameter struct containing number of sets, set idx bits, block size, block bits and associativity
    cache_params cache_info = init_cache_params(s, E, b);

    // calculate size of memory to allocate
    const int max_size = cache_info.S * cache_info.E * cache_info.B;
    cache_block* cache = init_cache(max_size);
   

    char line_buffer[BUFFER_SIZE];

    while (fgets(line_buffer, BUFFER_SIZE, tracefile))
    {
        cache_line c_line = init_cache_line(line_buffer); //(cache line contains flag, address, size)
        cache_result result = {0, 0, 0};
       // 1. Look for item in cache 
       // 2. If found incriment hit
       // 3. If not found incriment miss
       // 4. if cache is full remove lru items untill it can fit new items. Incriment evection count

       switch (c_line.flag){
        // these all effectively do the same thing
            case I_LOAD:
            case D_LOAD:
            case D_STORE:
                result = simulate_cache_read_write(cache, cache_info, c_line);
                eval_cache(&hit_count, &miss_count, &eviction_count, result);
                verbose(verbose_flg, c_line, result);
                break;
            case D_MODIFY: // read and write here (call read write twice)
                result = simulate_cache_read_write(cache, cache_info, c_line);
                int h = result.hit, ev = result.eviction, m = result.miss;
                eval_cache(&hit_count, &miss_count, &eviction_count, result);
                //verbose(verbose_flg, c_line, result);
                result = simulate_cache_read_write(cache, cache_info, c_line);
                cache_result result_cpy = result;
                result.hit = result.hit | h ? 1 : 0;
                result.miss = result.miss | m ? 1 : 0;
                result.eviction = result.eviction | ev ? 1 : 0;
                eval_cache(&hit_count, &miss_count, &eviction_count, result_cpy);
                verbose(verbose_flg, c_line, result);
                break;
            case INVALID:
                fprintf(stderr, "Invalid or missing flag encountered. Exiting\n");
                fclose(tracefile);
                free_cache(cache);
                exit(1);
            default:
                fprintf(stderr, "Invalid or missing flag encountered. Exiting\n");
                fclose(tracefile);
                free_cache(cache);
                exit(1);
       }

    }


    // output cache hit and miss statistics
    print_summary(hit_count, miss_count, eviction_count);
    fclose(tracefile);
    free_cache(cache);
    return 0;
}
