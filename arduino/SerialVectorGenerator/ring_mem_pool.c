#include "common.h"
#include "ring_mem_pool.h"

// Initialize ring
void ring_init(RingMemPool* ring, void* memory, int size) {
	memset(ring, '\0', sizeof(RingMemPool));
	ring->size   = size;
	ring->memory = memory;
}

// Check how much contiguous memory is available
int ring_remaining(const RingMemPool* ring) {
	if (ring->head == ring->tail) {
		// The head and tail are pointing to the same location
		// Either all the memory is available, or none of it is
		return (ring->used == 0) ? ring->size : 0;
	}
	if (ring->head < ring->tail) {
		// Head trails tail. Only the memory from
		// the head to the tail is available
		return ring->tail - ring->head;
	}

	// Head leads tail. There is memory available after the head
	// and befor the tail. Return the larger one
	int after = ring->size - ring->head;
	return (after > ring->tail) ? after : ring->tail;
}

// Get an entry onto the ring
void* ring_get(RingMemPool* ring, int size) {
	// Don't do anything for zero size
	if (size == 0) {
		ring->last_err = RING_OK;
		return NULL;
	}

	// Check remaining memory
	if (ring_remaining(ring) < size + sizeof(RingEntryHdr)) {
		ring->last_err = RING_OUT_OF_MEM;
		return NULL;
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
	ring->last_hdr = (RingEntryHdr*) &ring->memory[ring->head];
	ring->last_hdr->idx  = ring->head;
	ring->last_hdr->size = size;
	ring->head  += sizeof(RingEntryHdr);
	ring->used  += sizeof(RingEntryHdr);
	ring->count += 1

	// Get pointer to entry
	void* start = &ring->memory[ring->head];
	ring->head += size;
	ring->used += size;
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
	RingEntryHdr* hdr = &ring->memory[ring->tail];
	ring->used -= hdr->size + sizeof(RingEntryHdr);
	ring->tail += hdr->size + sizeof(RingEntryHdr);
	ring->count--;

	// Wrap tail if it has passed the wrap point
	if (ring->tail >= ring->wrap_point) {
		ring->tail = 0;
		ring->wrap_point = 0;
	}

	// Clear last header
	if (ring->count == 0) {
		ring->last_hdr = 0;
	}

	ring->last_err = RING_OK;
	return hdr->size;
}

// Resize the last entry
void* ring_resize(RingMemPool* ring, int size) {
	// Check the header
	if (!ring->last_hdr) {
		ring->last_err = RING_ERR;
		return NULL;
	}

	// Don't shrink the last entry
	if (size < ring->last_hdr->size) {
		ring->last_err = RING_OK;
		return &ring->memory[ring->last_hdr->idx + sizeof(RingEntryHdr)];
	}

	// Check for additional memory after the head
	int missing = size - ring->last_hdr->size;
	if (ring->head > ring->tail) {
		// Head is leading tail
		// Check for more memory at the end of the ring
		if (ring->size - ring->head < missing) {
			// Ran out of space
			ring->last_err = RING_OUT_OF_MEM;
			return NULL;
		}
	}
	else if (ring->head < ring->tail) {
		// Head is training tail
		// Check for memory between them
		if (ring->tail - ring->head < missing) {
			// Ran out of space
			ring->last_err = RING_OUT_OF_MEM;
			return NULL;
		}
	}
	else if (ring->head == ring->tail) {
		// Either out of memory or have a serious error
		ring->last_err = (ring->count) ? RING_OUT_OF_MEM : RING_CRITICAL;
		return NULL;
	}

	// Move head and return pointer to last data entry
	ring->head += missing;
	ring->last_hdr->size += missing;
	return &ring->memory[ring->last_hdr->idx + sizeof(RingEntryHdr)];
}
