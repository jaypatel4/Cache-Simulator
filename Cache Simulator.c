#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
Used for creating the cache layout, eavh line is the hexadecimal value given to store by the computer
*/
typedef struct{
    int order;
    unsigned long long tag;
}line;

/*
Returns log base 2 of a value
Used only for checking arguments for proper formatting
*/
double Format(int n) {
    double check = (double)n;
    if(check >= 0) {
        check = log2(check);
    }
    return check;
}

/*
Search the specific set for a specific tag, return -1 (invalid index in an array) if not found or the index of where the tag is found
*/
long long findTag(line** cache, long long setID, long long tagID, int size) {
    for(long long i = 0; i<size; i++) {
        if(cache[setID][i].order > -1 && cache[setID][i].tag == tagID) {
            return i;
        }
    }
    return -1;
}

/*
Find the biggest int for the timer in the set, then return that number
*/
int findOrder(line** cache, long long setID, int size) {
    int x = -1;
    for(int i = 0; i < size; i++) {
        if(cache[setID][i].order > -1 && cache[setID][i].order > x) {
            x = cache[setID][i].order;
        }
    }
    return x;
}

/*
Uses replacement policy used by cache (either Last recently used or first in first out) to determine replacement order of lines in cache
*/
long long findRemoveOrder(line ** cache, long long setID, int size) {
    int x = cache[setID][0].order;
    long long index = 0;
    for(long long i = 0; i < size; i++) {
        if(cache[setID][i].order > -1 && cache[setID][i].order < x) {
            index = i;
            x = cache[setID][i].order;
        }
    }
    return index;
}

/*
Use findOrder to get the largest time interval (represents either the most recently used or first input) and increment
Only being used when replacement policy is LRU and the memory is accessed, or when a new element is added in FIFO replacement policy
*/
void updateOrder(line** cache, long long setID, long long tagID, int size) {
    int order = (findOrder(cache,setID,size)+1);
    cache[setID][tagID].order = order;
}

/*
Find whether there is an open line in the specific set
*/
long long insert(line** cache, long long setID, long long tagID, int size) {
    for(long long i = 0; i<size; i++) {
        if(cache[setID][i].order == -1) {
            cache[setID][i].tag = tagID;
            return i;
        }
    }
    return -1;
}


/*
When the cache set is full, find the proper line to replace and move the new tag into the line
*/
long long removeInsert(line** cache, long long setID, long long tagID, int size) {

    long long index = findRemoveOrder(cache,setID,size);
    cache[setID][index].tag = tagID;
    return index;
}

/*
Not used for the final results only for testing
*/
void printCache(line** cache, int sets, int size) {
    for (long long i = 0; i < sets; i++) {
        printf("Set: %llu  |  ", i);
        for(int j = 0; j < size; j++) {
            if(cache[i][j].order > -1) {
                printf("%llu    ",cache[i][j].tag);

            }
            else {
                printf("-    ");
            }
        }
        printf("\n");
    }
}

int main(int argc, char* argv[])
{
    // Check for proper amount of arguments
    if(argc != 6) {
        printf("error");
        return 0;
    }

    // Check cache size formatting
    int cacheSize = atoi(argv[1]);
    if(cacheSize < 2 || ((Format(cacheSize) != floor(Format(cacheSize))) && cacheSize != 2)) {
        printf("error");
        return 0;
    }

    // Check block size formatting
    int blockSize = atoi(argv[4]);
    if(blockSize < 2 || ((Format(blockSize) != floor(Format(blockSize))) && blockSize != 2)) {
        printf("error");
        return 0;
    }

    // Check replacement policy formatting
    if((strcmp(argv[3],"fifo") != 0)  && (strcmp(argv[3],"lru") != 0) ) {
        printf("error");
        return 0;
    }

    // Check associativity formatting
    int assoc;
    if((strcmp(argv[2],"direct")!= 0) && (strcmp(argv[2], "assoc") != 0) ) {

        if(argv[2][0] != 'a' || argv[2][1] != 's' || argv[2][2] != 's' || argv[2][3] != 'o' || argv[2][4] != 'c' || argv[2][5] != ':' || (argv[2][6] >'9' || argv[2][6] < '0')) {
            printf("error");
            return 0;
        }
        else {
            sscanf(argv[2], "assoc:%d",&assoc);
            if(assoc < 2 || ((Format(assoc) != floor(Format(assoc))) && assoc != 2)) {
                printf("error");
                return 0;
            }
        }
    }

    FILE *fp = fopen(argv[5],"r");

    // Check for valid file
    if(fp == NULL) {
        printf("error");
        return 0;
    }


    // Initialize counters for results
    int cacheMiss = 0;
    int cacheHit = 0;
    int memRead = 0;
    int memWrite = 0;



    // Lines per set = lines
    int lines = 0;
    int sets = 0;
    int blockBits = 0;
    blockBits = log2(blockSize);
    int setBits = 0;

    // At this point we know we have proper formatting so we can determine the configurations of the cache
    if(strcmp(argv[2],"direct") == 0) {
        lines = 1;
        sets = (cacheSize / blockSize);
        setBits = log2(sets);
    }
    else if(strcmp(argv[2],"assoc") == 0) {
        sets = 1;
        lines = (cacheSize / blockSize);
    }
    else {
        sets = (cacheSize / (blockSize * assoc));
        lines = assoc;
        setBits = log2(sets);
    }

    // Malloc the array of lines in cache, each index is 1 set so each cache[i][j] corresponds to the jth line in the ith set
    line **cache = (line **)malloc(sets * sizeof(line *));
    for(int i = 0; i < sets; i++) {
        cache[i] = (line *)malloc(lines * sizeof(line));
    }

    // Initialize empty cache
    for(int i = 0; i < sets; i++) {
        for(int j = 0; j < lines; j++) {
            cache[i][j].order = -1;
        }
    }
    
    // Character array for scanning first value in each line of the input file, this information is generally useless to us until we encounter #eof
    char useless[40];
    useless[1] = '0';

    // A command given, either W (write) or R (read) from the cache
    char command;

    unsigned long long cacheAddress;
    unsigned long long compare = ((long long) pow(2.0, (double)setBits)-1);

    // 2nd value in character array being e means we have encountered #eof, we are done scanning inputs
    while(useless[1] != 'e') {
        fscanf(fp, "%s ",useless);
        if(useless[1] == 'e') {
            break;
        }
        // Scan input for the address in hexadecimal 0x%llx converts hex into decimal values
        fscanf(fp,"%c 0x%llx\n",&command, &cacheAddress);

        // Use bit manipulation to get the setID and tagID from the cache address
        unsigned long long setID = ((cacheAddress >> blockBits) & compare);
        unsigned long long tagID = (cacheAddress >> (blockBits + setBits));
        long long found = findTag(cache,setID,tagID,lines);
    
        // If the command is a write, either update usage based on replacement policy or insert if the cache does not have the cache address (write-through)
        if(command == 'W') {
            if(found > -1) {
                memWrite++;
                cacheHit++;
                if(strcmp(argv[3],"lru") == 0) {
                    updateOrder(cache,setID,found,lines);
                }
            }
            else if(found == -1) {
                cacheMiss++;
                memRead++;
                memWrite++;

                long long check = insert(cache,setID,tagID,lines);
                if(check == -1) {
                    check = removeInsert(cache,setID,tagID,lines);
                }

                updateOrder(cache,setID,check,lines);

            }
        }
        // If the command is a read either insert if the value is found or update based on the replacement policy
        else if(command == 'R') {
            if(found > -1) {
                cacheHit++;
                if(strcmp(argv[3],"lru") == 0) {
                    updateOrder(cache,setID,found,lines);
                }
            }
            // The difference between read and write lies in the cache usage count, memory and cache usage is incrimented differently for reads and writes
            else if(found == -1) {
                cacheMiss++;
                memRead++;
                long long check = insert(cache,setID,tagID,lines);
                if(check == -1) {
                    check = removeInsert(cache,setID,tagID,lines);
                }
                updateOrder(cache,setID,check,lines);
            }

        }
        
    }

    // Free all memory allocated for the cache
    for(int i = 0; i < sets; i++) {
        free(cache[i]);
    }
    free(cache);


    // Prints cache usage, in total the number of reads, writes, hits, misses
    printf("Memory reads: %d\n", memRead);
    printf("Memory writes: %d\n",memWrite);
    printf("Cache hits: %d\n",cacheHit);
    printf("Cache misses: %d\n",cacheMiss);
    
    fclose(fp);
}