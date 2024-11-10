#ifndef RING_MEM_POOL_H
#define RING_MEM_POOL_H

#define RING_OK 0
#define RING_ERR -1
#define RING_OUT_OF_MEM -2
#define RING_CRITICAL -3

typedef struct RingEntryHdr {
	int idx;
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
	RingEntryHdr* last_hdr;
	int last_err;
} RingMemPool;

void ring_init(RingMemPool* ring, void* memory, int size);
int ring_remaining(const RingMemPool* ring);
void* ring_get(RingMemPool* ring, int size);
int ring_pop(RingMemPool* ring);
void* ring_resize(RingMemPool* ring, int size);
void* last_entry(const RingMemPool* pool);

#endif // RING_MEM_POOL_H
