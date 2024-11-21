// ring_mem_pool_tests.c

#include <string.h>

#include <string>

#include "gtest/gtest.h"

extern "C" {
#include "ring_mem_pool.h"
}

#define ASSERT_NOT_NULL(exp) ASSERT_NE((void*)NULL, (void*)exp)


TEST(RingMemoryPool, init) {
    char buf[128];
    RingMemPool pool;
    ring_init(&pool, buf, sizeof(buf));
    EXPECT_EQ(RING_OK, pool.last_err);
    EXPECT_EQ(128u,    pool.size);
    EXPECT_EQ(0u,      pool.head);
    EXPECT_EQ(0u,      pool.tail);
    EXPECT_EQ(0u,      pool.wrap_point);
    EXPECT_EQ(buf,     pool.memory);
    EXPECT_EQ(128u,    ring_remaining(&pool));
}

TEST(RingMemoryPool, getAndPopOne) {
    // Init pool
    char buf[128];
    RingMemPool pool;
    ring_init(&pool, buf, sizeof(buf));
    ASSERT_EQ(RING_OK, pool.last_err);

    // Get memory and set it
    const char msg[] = "0123456789";
    char* mem_in = (char*)ring_get(&pool, sizeof(msg));
    ASSERT_EQ(RING_OK, pool.last_err);
    ASSERT_NOT_NULL(mem_in) << "Memory wasn't allocated";
    EXPECT_EQ(sizeof(buf) - sizeof(msg) - sizeof(RingEntryHdr), ring_remaining(&pool));
    strcpy(mem_in, msg);

    // Verify memory
    char* mem_out = (char*)ring_peek(&pool);
    ASSERT_EQ(RING_OK, pool.last_err);
    ASSERT_NOT_NULL(mem_out);
    ASSERT_EQ(mem_in, mem_out) << "Expected memory address to be the same";
    ASSERT_EQ(0, strncmp(msg, mem_out, sizeof(msg))) << "Message was corrupted";

    // Pop memory
    EXPECT_EQ(sizeof(msg), ring_pop(&pool));
    ASSERT_EQ(RING_OK, pool.last_err);
    EXPECT_EQ(pool.head, pool.tail);
    EXPECT_EQ(sizeof(buf), ring_remaining(&pool));
    EXPECT_EQ(0u, ring_pop(&pool));
}

TEST(RingMemoryPool, getAndPopMany) {
    // Init pool
    char buf[128];
    RingMemPool pool;
    ring_init(&pool, buf, sizeof(buf));
    ASSERT_EQ(RING_OK, pool.last_err);

    // Do this enough times to ensure no wrapping around the end of the buffer happens
    const char msg[] = "0123456789";
    for (int i = 0; i < 5; i++) {
        // Get memory and set it
        char* mem_in = (char*)ring_get(&pool, sizeof(msg));
        ASSERT_EQ(RING_OK, pool.last_err);
        ASSERT_NOT_NULL(mem_in) << "Iteration " << i << ". Memory wasn't allocated";
        strcpy(mem_in, msg);

        // Verify memory
        char* mem_out = (char*)ring_peek(&pool);
        ASSERT_NOT_NULL(mem_out);
        ASSERT_EQ(mem_in, mem_out) << "Iteration " << i << ". Expected memory address to be the same";
        ASSERT_EQ(0, strncmp(msg, mem_out, sizeof(msg))) << "Iteration " << i << ". Message was corrupted";

        // Pop memory
        EXPECT_EQ(sizeof(msg), ring_pop(&pool));
        ASSERT_EQ(RING_OK, pool.last_err);
        EXPECT_EQ(pool.head, pool.tail);
        EXPECT_EQ(sizeof(buf), ring_remaining(&pool));
    }
}
