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
    uint8_t size;
} RingEntryHdr;

typedef struct RingMemPool {
    volatile uint16_t size;
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t wrap_point;
    volatile int8_t   last_err;
    void* memory;
} RingMemPool;

void ring_init(RingMemPool* ring, void* memory, uint16_t size);
void ring_reset(RingMemPool* ring);
uint16_t ring_remaining(const RingMemPool* ring);
void* ring_get(RingMemPool* ring, uint8_t size);
void* ring_peek(const RingMemPool* ring);
uint8_t ring_pop(RingMemPool* ring);

#endif // RING_MEM_POOL_H
