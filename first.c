??/????????????????#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int blocks = 0;
long sets = 0;

typedef struct Queue {
  int front;
  int last;
  int size;
  unsigned long int * tags;
} Queue;

int isFull(Queue q) {
  return q.size == blocks;
}

int insert(Queue * q, unsigned long int tagBit, int index) {
  if (!isFull(q[index])) {
    if (q[index].last == blocks-1)
      q[index].last = -1;
    q[index].tags[(q[index].last)+1] = tagBit;
    q[index].last = (q[index].last)+1;
    q[index].size = (q[index].size)+1;
    return 1;
  }
  return 0;
}

void removeFirst(Queue * q, unsigned long index) {
  if (q[index].size) {
    if (q[index].front == blocks-1)
      q[index].front = 0;
    else
      q[index].front = (q[index].front)+1;
    q[index].size = (q[index].size)-1;
  }
}

unsigned long int removeLRU(Queue * q, unsigned long index) {
  unsigned long int removed = q[index].tags[q[index].front];
  if (q[index].size) {
    if (q[index].front == blocks-1)
      q[index].front = 0;
    else
      q[index].front = (q[index].front)+1;
    q[index].size = (q[index].size)-1;
  }
  return removed;
}

void insertLRU(Queue * q, unsigned long int tagBit, int index) {
  Queue que = q[index];
  unsigned long int RU;
  if (que.tags[que.front] == tagBit) {
    RU = removeLRU(q, index);
    insert(q, RU, index);
  }
}

void printHash(Queue * q, int index) {
  int i = q[index].front;
  if (q[index].size) {
    while ( i != q[index].last ) {
      printf("%lx\n", q[index].tags[i]);
      i++;
      if ( i == blocks )
	i = 0;
    }
    printf("%lx\n", q[index].tags[i]);
  }
  else
    printf("q[%d] size = 0\n", index);
}

//Set queue
Queue setQueue(Queue q) {
  q.front = 0;
  q.last = -1;
  q.size = 0;
  q.tags = malloc(sizeof(unsigned long int)*blocks);
  return q;
}

//Create cache; set global variable blocks
Queue * initCache(long cacheSize, long blockSize, long assoc) {
  Queue * cache; int i;
  if (assoc == 0) { 
    cache = malloc(sizeof(Queue));
    blocks = cacheSize / blockSize;
    sets = 1;
    cache[0] = setQueue(cache[0]);
    //printf("sets:1\n");
  }
  else {
    sets = cacheSize / (blockSize * assoc); //printf("sets:%ld\n", sets);
    cache = malloc(sizeof(Queue)*sets);
    blocks = assoc;
    for (i = 0; i < sets; i++) 
      cache[i] = setQueue(cache[i]);
  }  
  return cache;
}

//Find associativity
long findAssoc(char * assoc) {
  if (!strcmp(assoc,"direct")) 
    return 1;
  else if (!strcmp(assoc, "assoc")) 
    return 0;
  long ac;
  sscanf(assoc, "assoc:%ld", &ac);
  return ac;
}

int myLog(long x) {
  int i = 0;
  while (x != 1) {
    x = x >> 1;
    i++;
  }
  return i;
}

unsigned long int getTag(unsigned long int address, long blockSize, long sets) {
  int numBlockBits = myLog(blockSize);
  int numIndexBits = myLog(sets);
  //printf("Tag bits:%d\n", 48-numBlockBits+numIndexBits);
  return address >> (numBlockBits+numIndexBits);
}

unsigned long int getIndex(unsigned long int address, long blockSize, long sets)
{//9 - 512 3 - 8 2871
  int numBB = myLog(blockSize); //int numIB = myLog(sets);
  //printf("block bits=%d index bits=%d", numBB, numIB);
  unsigned long int mask; 
  unsigned long int a = 1 << myLog(sets);
  /*
  printf("1 << myLog(sets))=%lx\n", a);
  printf("(1 << myLog(sets))-1=%lx\n", a-1); 
  printf("(a-1) << tagBits=%lx\n", (a-1) << numBB);
  */
  mask = (a-1) << numBB;
  //printf("mask=%lx\n", mask);
  unsigned long int isolated = address & mask;
  //printf("isolated >> numBB =%lx\n", isolated >> numBB);
  return isolated >> numBB;
}

int searchTag(Queue * q, unsigned long index, unsigned long int tag) {
  int i = q[index].front;
  if (q[index].size) {
    //printf("q[%ld].last = %d size = %d\n", index, q[index].last, q[index].size);
    //printf("%d != %d\n", i, q[index].last);
    while (i != q[index].last) {
      //printf("q:%lx == tag:%lx\n", q[index].tags[i], tag);
      if (q[index].tags[i] == tag)
	return 1;
      if (i == blocks-1)
	i = 0;
      else
	i++;
    }
    if (q[index].tags[q[index].last] == tag) //q[index].tags[0] == tag
      return 1;
  }
  return 0;
} 

//<cache size> <associativity> <cache policy> <block size> <trace file>
int main(int argc, char ** argv) {
  //test queues 
  /*
  blocks = 6;
  Queue * test = malloc(sizeof(Queue)*10); int z;
  for (z = 0; z < 10; z++) {
    test[z] = setQueue(test[z]);
    printHash(test, z);
  }
  insert(test, 1, 0);
  insert(test, 2, 0);
  insert(test, 3, 0);
  //removeFirst(test, 0);
  insert(test, 4, 0);
  insert(test, 5, 0);
  insert(test, 6, 0);
  removeFirst(test, 0); removeFirst(test, 0);
  insert(test, 1, 0);
  printHash(test, 0); 
  printf("Front:%d test[0].tags[front]:%lx\n", test[0].front, test[0].tags[test[0].front]);
  printf("last:%lx\n", test[0].tags[test[0].last]);
  */
  
  //set up
  char *ptr; int writes, reads, hit, miss; writes = reads = hit = miss = 0;
  long cache_size = strtol(argv[1], &ptr, 10);
  long block_size = strtol(argv[4], &ptr, 10);
  long assoc = findAssoc(argv[2]); 
  Queue * q = initCache(cache_size, block_size, assoc);
  //unsigned long int evicted;
  //printf("blocks:%d\n", blocks);
  
  //read 
  FILE * fp = fopen(argv[5], "r");
  char line[50];
  while (fgets(line, 50, fp)) {
    //printf("%s\n", line);
    if (!strcmp(line, "#eof\n")) {
      //printf("BREAK\n");
      break;
    }
    char op[2]; unsigned long int address;
    sscanf(line, "%*s %s %lx", op, &address);
    unsigned long index = getIndex(address, block_size, sets);
    unsigned long int tag = getTag(address, block_size, sets);
    //printf("tag:%lx\n", tag); 
    int found = searchTag(q, index, tag);
    //printf("Index:%ld Tag:%lx Found = %d\n", index, tag, found);
    if (op[0] == 'R') {
      if (!found) {
	reads++;
	miss++;
	if (isFull(q[index])) 
	  removeFirst(q, index);
	insert(q, tag, index);
      }
      else {
	hit++;
	if (!strcmp(argv[3], "lru"))
	  insertLRU(q, tag, index);
	//evicted = removeLRU(q, index);
      }
      //printf("    [READ]reads:%d writes:%d hits:%d misses:%d\n", reads, writes, hit, miss);
    }
    else {
      writes++;
      if (!found) {
	miss++;
	reads++;
	if (isFull(q[index])) 
	  removeFirst(q, index);
	insert(q, tag, index);
      }
      else {
	hit++;
	if (!strcmp(argv[3], "lru"))
	  insertLRU(q, tag, index);
      }
      //printf("    [WRITE]reads:%d writes:%d hits:%d misses:%d\n", reads, writes, hit, miss);
    }
    //printf("tag: %lx index: %d\n", tag, (int)index);
  }
  
  //printf("writes:%d reads:%d hits:%d misses:%d\n", writes, reads, hit, miss);
  free(q);
  
  printf("no-prefetch\n"); printf("Memory reads: %d\n", reads);
  printf("Memory writes: %d\n", writes); printf("Cache hits: %d\n", hit);
  printf("Cache misses: %d\n", miss);
  //printf("\nIndex:%lx\n", getIndex(3213816984, 16, 2));
  //printf("Tag:%lx\n", getTag(3213816984, 16, 2));
  fclose(fp);
  
  fp = fopen(argv[5], "r"); //unsigned long int bs = block_size;
  


  //prefetch 
  //initialize cache
  Queue * q2 = initCache(cache_size, block_size, assoc);
 

  //set variables to 0
  reads = writes = hit = miss = 0;
  


  //look through file
  while (fgets(line, 50, fp)) {
    if (!strcmp(line, "#eof\n")) 
      break;
    char op[2]; unsigned long int address;
    sscanf(line, "%*s %s %lx", op, &address);
    


   //find the index and the tag from the current line
    unsigned long index = getIndex(address, block_size, sets);
    unsigned long int tag = getTag(address, block_size, sets);
    


    //determine if operation is a read or a write
    int found = searchTag(q2, index, tag);
    if (op[0] == 'R') {


      //operation is a read; was the address found in the cache?
      if (!found) {

  

        //was not found increment read and miss
        reads++;
        miss++;
        if (isFull(q2[index]))
	  removeFirst(q2, index);
        insert(q2, tag, index);


       //since it was a miss I want to prefetch; make sure the next adj address is in the cache
	unsigned long int adj_block = address+block_size;
	unsigned long index_adj = getIndex(adj_block, block_size, sets);
	unsigned long int tag_adj = getTag(adj_block, block_size, sets);
	found = searchTag(q2, index_adj, tag_adj);
	if (!found) {
	  reads++;
	  if (isFull(q2[index_adj]))
	    removeFirst(q2, index_adj);
	  insert(q2, tag_adj, index_adj);
	}
      }
      else {
        hit++;
	if (!strcmp(argv[3], "lru"))
          insertLRU(q2, tag, index);
      }
    }
    else {
      writes++;
      if (!found) {
        miss++;
        reads++;
        if (isFull(q2[index])) 
	  removeFirst(q2, index);
        insert(q2, tag, index);



	unsigned long int adj_block = address+block_size;
        unsigned long index_adj = getIndex(adj_block, block_size, sets);
        unsigned long int tag_adj = getTag(adj_block, block_size, sets);


        found = searchTag(q2, index_adj, tag_adj);
        if (!found) {
          reads++;
          if (isFull(q2[index_adj]))
            removeFirst(q2, index_adj);
          insert(q2, tag_adj, index_adj);
	}
      }
      else {
        hit++;
	if (!strcmp(argv[3], "lru"))
          insertLRU(q2, tag, index);
      }
    }
  }
  printf("with-prefetch\n"); printf("Memory reads: %d\n", reads);
  printf("Memory writes: %d\n", writes); printf("Cache hits: %d\n", hit);
  printf("Cache misses: %d\n", miss);
  free(q2);
  fclose(fp);
  
  return 0;
} 
