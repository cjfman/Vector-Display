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
    EXPECT_EQ(128,    pool.size);
    EXPECT_EQ(0,      pool.head);
    EXPECT_EQ(0,      pool.tail);
    EXPECT_EQ(0,      pool.wrap_point);
    EXPECT_EQ(buf,     pool.memory);
    EXPECT_EQ(128,    ring_remaining(&pool));
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
    EXPECT_EQ(sizeof(buf) - sizeof(msg) - sizeof(RingEntryHdr), (unsigned)ring_remaining(&pool));
    strcpy(mem_in, msg);

    // Verify memory
    char* mem_out = (char*)ring_peek(&pool);
    ASSERT_EQ(RING_OK, pool.last_err);
    ASSERT_NOT_NULL(mem_out);
    ASSERT_EQ(mem_in, mem_out) << "Expected memory address to be the same";
    ASSERT_EQ(0, strncmp(msg, mem_out, sizeof(msg))) << "Message was corrupted";

    // Pop memory
    EXPECT_EQ(sizeof(msg), (unsigned)ring_pop(&pool));
    ASSERT_EQ(RING_OK, pool.last_err);
    EXPECT_EQ(pool.head, pool.tail);
    EXPECT_EQ(sizeof(buf), (unsigned)ring_remaining(&pool));
    EXPECT_EQ(0, ring_pop(&pool));
}

TEST(RingMemoryPool, getAndPopMany) {
    // Init pool
    char buf[128];
    RingMemPool pool;
    ring_init(&pool, buf, sizeof(buf));
    ASSERT_EQ(RING_OK, pool.last_err);

    // Do this few enough times to ensure no wrapping around the end of the buffer happens
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
        EXPECT_EQ(0, ring_pop(&pool));
    }
}

TEST(RingMemoryPool, getAndPopManyWrap) {
    // Init pool
    char buf[128];
    RingMemPool pool;
    ring_init(&pool, buf, sizeof(buf));
    ASSERT_EQ(RING_OK, pool.last_err);

    // Do this enough times to ensure wrapping around the end of the buffer
    const char msg[] = "0123456789";
    for (int i = 0; i < 10; i++) {
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
        EXPECT_EQ(sizeof(msg), ring_pop(&pool))       << "Iteration " << i;
        ASSERT_EQ(RING_OK, pool.last_err)             << "Iteration " << i;
        EXPECT_EQ(pool.head, pool.tail)               << "Iteration " << i;
        EXPECT_EQ(sizeof(buf), ring_remaining(&pool)) << "Iteration " << i;
        EXPECT_EQ(0, ring_pop(&pool))                 << "Iteration " << i;
    }
}

TEST(RingMemoryPool, getManyThanPopMany) {
    // Init pool
    char buf[128];
    RingMemPool pool;
    ring_init(&pool, buf, sizeof(buf));
    ASSERT_EQ(RING_OK, pool.last_err);

    // Do this enough times to ensure no wrapping around the end of the buffer happens
    for (int i = 0; i < 5; i++) {
        // Get memory and set it
        char* mem_in = (char*)ring_get(&pool, sizeof(i));
        ASSERT_EQ(RING_OK, pool.last_err);
        ASSERT_NOT_NULL(mem_in) << "Iteration " << i << ". Memory wasn't allocated";
        memcpy(mem_in, (char*)&i, sizeof(i));
    }

    // Verify and pop memory
    for (int i = 0; i < 5; i++) {
        // Verify memory
        char* mem_out = (char*)ring_peek(&pool);
        ASSERT_NOT_NULL(mem_out);
        ASSERT_EQ(i, *(int*)mem_out) << "Iteration " << i << ". Message was corrupted";

        // Pop
        EXPECT_EQ(sizeof(i), ring_pop(&pool));
        ASSERT_EQ(RING_OK, pool.last_err);
    }

    // Final state
    EXPECT_EQ(pool.head, pool.tail);
    EXPECT_EQ(sizeof(buf), ring_remaining(&pool));
    EXPECT_EQ(0, ring_pop(&pool));
}

TEST(RingMemoryPool, useAllMemory) {
    // Init pool
    char buf[128];
    RingMemPool pool;
    ring_init(&pool, buf, sizeof(buf));
    ASSERT_EQ(RING_OK, pool.last_err);

    // Do this enough times to ensure no wrapping around the end of the buffer happens
    int i;
    for (i = 0; pool.last_err == RING_OK; i++) {
        // Get memory and set it
        char* mem_in = (char*)ring_get(&pool, sizeof(i));
        if (mem_in) memcpy(mem_in, (char*)&i, sizeof(i));
    }
    ASSERT_EQ(RING_OUT_OF_MEM, pool.last_err);

    // Verify and pop memory
    int j;
    for (j = 0; ring_remaining(&pool) < (int)sizeof(buf) && j < i; j++) {
        // Verify memory
        char* mem_out = (char*)ring_peek(&pool);
        if (mem_out) {
            ASSERT_EQ(j, *(int*)mem_out) << "Iteration " << j << ". Message was corrupted";

            // Pop
            EXPECT_EQ(sizeof(j), ring_pop(&pool));
            ASSERT_EQ(RING_OK, pool.last_err);
        }
    }

    // Final state
    EXPECT_EQ(i - 1, j) << "Pushes didn't match pops";
    EXPECT_EQ(pool.head, pool.tail);
    EXPECT_EQ(sizeof(buf), ring_remaining(&pool));
    EXPECT_EQ(0, ring_pop(&pool));
}
