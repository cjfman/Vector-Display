#include <string.h>

#include "common.h"
#include "ring_mem_pool.h"

// Initialize ring
void ring_init(RingMemPool* ring, void* memory, unsigned size) {
	memset(ring, '\0', sizeof(RingMemPool));
	ring->size   = size;
	ring->memory = memory;
}

// Check how much contiguous memory is available
// Only a writer may call this
unsigned ring_remaining(const RingMemPool* ring) {
	if (ring->head == ring->tail) {
		// The head and tail are pointing to the same location
		// Either all the memory is available, or none of it is
		// If there's a wrap point, all data is being used
		return (ring->wrap_point) ? ring->size : 0;
	}

	// Copy values to ensure nothing else changes them
	int tail       = ring->tail;
	int wrap_point = ring->wrap_point;

	// Synchronicity note
	// If the tail is trailing the head, even if it moves it
	// won't pass the head, there for the memory from the head
	// to the end of the ring will valid free memory
	// If the tail wraps while this function is being called
	// the space between the head and old tail will still
	// be valid free memory
	
	if (wrap_point) {
		// Head trails tail. Only the memory from
		// the head to the tail is available
		return tail - ring->head;
	}

	// Head leads tail. There is memory available after the head
	// and before the tail. Return the larger one
	int after = ring->size - ring->head;
	return (after > tail) ? after : tail;
}

// Get an entry onto the ring
void* ring_get(RingMemPool* ring, unsigned size) {
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
	int data_count = 0;
	RingEntryHdr* hdr = (RingEntryHdr*) &ring->memory[ring->head];
	hdr->idx = ring->head;
	hdr->size = size;
	data_count += sizeof(RingEntryHdr);

	// Get pointer to entry
	void* start = &ring->memory[ring->head];
	data_count += size;

	// Update head and metadata
	ring->head  += data_count;
	ring->last_err = RING_OK;

	return start;
}

// Clear an entry from the ring
unsigned ring_pop(RingMemPool* ring) {
	// Do nothing if there's nothing to pop
	if (ring->head == ring->tail) {
		return 0; // Nothing to pop
	}

	// Get header and move tail
	RingEntryHdr* hdr = &ring->memory[ring->tail];
	int size = hdr->size + sizeof(RingEntryHdr);

	// Wrap tail if it has passed the wrap point
	if (ring->tail + size >= ring->wrap_point) {
		ring->tail = 0;
		ring->wrap_point = 0;
	}
	else {
		ring->tail += size;
	}

	return hdr->size;
}
