// RingMemPool
// A ring buffer like memory pool that can store entries of arbitrary size
// Although it is written in a lock free way, this is mostly academic as
// AVR 8-bit micro controllers doesn't provide atomic types beyond a byte

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
	volatile int size;
	volatile int head;
	volatile int tail;
	volatile int wrap_point;
	volatile int last_err;
	void* memory;
} RingMemPool;

void ring_init(RingMemPool* ring, void* memory, int size);
int ring_remaining(const RingMemPool* ring);
void* ring_get(RingMemPool* ring, int size);
void* ring_peek(const RingMemPool* ring);
int ring_pop(RingMemPool* ring);

#endif // RING_MEM_POOL_H
