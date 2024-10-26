#include "ring_mem_pool.h"

// Initialize ring
void ring_init(RingMemPool* ring, char* memory, int size) {
	memset(ring, '\0', sizeof(RingMemPool));
	ring->size   = size;
	ring->memory = memory;
}

// Check how much contiguous memory is available
static int ring_remaining(RingMemPool* ring) {
	if (ring->head == ring->tail) {
		// The head and tail are pointing to the same location
		// Either all the memory is available, or not of it is
		return (ring->used == 0) ? ring->size : 0;
	}
	if (ring->head < ring->tail) {
		// Head trails tail, the memory from the head to the
		// tail is available
		return ring->tail - ring->head;
	}

	// Head leads tail, there is memory available after the head
	// and befor the tail. Return the larger one
	int after = ring->size - ring->head;
	return (after > ring->tail) ? after : ring->tail;
}

// Get an entry onto the ring
void* ring_get(RingMemPool* ring, int size) {
	// Check remaining memory
	if (ring_remaining(ring) < size + sizeof(RingEntryHdr)) {
		ring->last_err = RING_OUT_OF_MEM;
		return 0;
	}

	// If tail leads head, there are two places to put the data
	// Head may need to wrap if the data won't fit at the end
	if (ring->tail > ring->head
		&& ring->size - ring->head < size
	) {
		// Wrap head
		ring->wrap_point = ring->head;
		ring->head = 0;
	}

	// Construct a header
	RingEntryHdr* hdr = (RingEntryHdr*) &ring->memory[ring->head];
	ring->head += sizeof(hdr);
	ring->size += sizeof(hdr);
	hdr->num  = ring->count++;
	hdr->size = size;

	// Set memory
	void* start = &ring->memory[ring->head];
	ring->head += size;
	ring->used += size;
	ring->count++;
	ring->last_err = RING_OK;

	return start;
}

// Clear an entry from the ring
int ring_pop(RingMemPool* ring) {
	// Do nothing if there's nothing to pop
	if (ring->count == 0) {
		return 0; // Nothing to pop
	}

	// Get header and move tail
	RingEntryHdr* hdr = ring->memory;
	ring->used -= hdr->size;
	ring->tail += hdr->size;

	// Wrap tail if it has passed the wrap point
	if (ring->tail >= ring->wrap_point) {
		ring->tail = 0;
		ring->wrap_point = 0;
	}

	return hdr->size;
}
