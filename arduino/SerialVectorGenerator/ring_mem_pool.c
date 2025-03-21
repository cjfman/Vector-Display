#include <inttypes.h>
#include <string.h>

#include "common.h"
#include "ring_mem_pool.h"

// Initialize ring
void ring_init(RingMemPool* ring, void* memory, uint16_t size) {
    memset(ring, '\0', sizeof(RingMemPool));
    ring->size     = size;
    ring->memory   = memory;
    ring->last_err = RING_OK;
}

void ring_reset(RingMemPool* ring) {
    int16_t size     = ring->size;
    void* memory = ring->memory;
    memset(ring, '\0', sizeof(RingMemPool));
    ring->size     = size;
    ring->memory   = memory;
    ring->last_err = RING_OK;
}

// Check how much contiguous memory is available
// Only a writer may call this
uint16_t ring_remaining(const RingMemPool* ring) {
    // Error guard
    if (ring->last_err == RING_CRITICAL) return 0;

    if (ring->head == ring->tail) {
        // The head and tail are pointing to the same location
        // Either all the memory is available, or none of it is
        // If there's a wrap point, all data is being used
        return (!ring->wrap_point) ? ring->size : 0;
    }

    // Copy values to ensure nothing else changes them
    int16_t tail       = ring->tail;
    int16_t wrap_point = ring->wrap_point;

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
    int16_t after = ring->size - ring->head;
    return (after > tail) ? after : tail;
}

// Get an entry onto the ring
void* ring_get(RingMemPool* ring, uint8_t size) {
    // Error guard
    if (ring->last_err == RING_CRITICAL) return NULL;

    // Don't do anything for zero size
    if (size == 0) {
        ring->last_err = RING_OK;
        return NULL;
    }

    // Check remaining memory
    uint8_t full_size = size + sizeof(RingEntryHdr);
    if (ring_remaining(ring) <= full_size) {
        ring->last_err = RING_OUT_OF_MEM;
        return NULL;
    }

    // If tail trails or is at the head, there are two places to put the data
    // Head may need to wrap if the data won't fit at the end
    if (ring->size - ring->head < full_size) {
        if (ring->tail < ring->head) {
            // Wrap head
            ring->wrap_point = ring->head;
            ring->head = 0;
        }
        else if (ring->tail == ring->head) {
            // Move them both
            ring->head = 0;
            ring->tail = 0;
        }
    }

    // Construct a header
    uint16_t data_count = 0;
    RingEntryHdr* hdr = (RingEntryHdr*) &ring->memory[ring->head];
    hdr->size = size;
    data_count += sizeof(RingEntryHdr);

    // Get pointer to entry
    void* start = &ring->memory[ring->head];
    data_count += size;

    // Update head and metadata
    ring->head += data_count;
    if (ring->head >= ring->size) {
        // This should never happen
        ring->head = 0;
        ring->tail = 0;
        ring->last_err = RING_CRITICAL;
        return 0;
    }

    ring->count++;
    ring->last_err = RING_OK;
    return start + sizeof(RingEntryHdr);
}

void* ring_peek(const RingMemPool* ring) {
    // Error guard
    if (ring->last_err == RING_CRITICAL) return NULL;

    // Do nothing if there's nothing to pop
    if (ring->head == ring->tail) {
        return NULL; // Nothing to pop
    }

    return &ring->memory[ring->tail + sizeof(RingEntryHdr)];
}

// Clear an entry from the ring
uint8_t ring_pop(RingMemPool* ring) {
    // Error guard
    if (ring->last_err == RING_CRITICAL) return 0;

    // Do nothing if there's nothing to pop
    if (ring->head == ring->tail) {
        ring->last_err = RING_OK;
        return 0; // Nothing to pop
    }

    // Get header and move tail
    RingEntryHdr* hdr = &ring->memory[ring->tail];
    uint8_t size = hdr->size + sizeof(RingEntryHdr);

    // Wrap tail if it has passed the wrap point
    if (ring->wrap_point && ring->tail + size >= ring->wrap_point) {
        ring->tail = 0;
        ring->wrap_point = 0;
    }
    else {
        ring->tail += size;
        if (ring->tail >= ring->size) {
            // This should never happen
            ring->head = 0;
            ring->tail = 0;
            ring->last_err = RING_CRITICAL;
            return 0;
        }
    }

    ring->count--;
    ring->last_err = RING_OK;
    return hdr->size;
}
