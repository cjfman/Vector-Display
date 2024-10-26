#ifndef RING_MEM_POOL_H
#define RING_MEM_POOL_H

#define RING_OK 0
#define RING_ERR -1
#define RING_OUT_OF_MEM -2
#define RING_CRITICAL -3

typedef struct RingEntryHdr {
	int num;
	int size;
} RingEntryHdr;

typedef struct RingMemPool {
	int size;
	int used;
	int count;
	int head;
	int tail;
	int wrap_point;
	void* memory;
	int last_err;
} RingMemPool;

void ring_init(RingMemPool* ring, char* memory, int size);
void* ring_get(RingMemPool* ring, int size);
int ring_pop(RingMemPool* ring);

#endif // RING_MEM_POOL_H
