#include <string.h>

#include <string>

#include "gtest/gtest.h"

extern "C" {
#include "command_parser.h"
#include "ring_mem_pool.h"
#include "screen_controller.h"
}

class ScreenControllerTest: public testing::Test {
protected:
    void SetUp() {
        ring_init(&this->pool, this->pool_mem, sizeof(this->pool_mem));
    }

    char pool_mem[1<<10];
    RingMemPool pool;
};

TEST_F(ScreenControllerTest, point) {
    // Create point command and push it to the screen
    PointCmd cmd = {
        {},  // base
        45,  // x
        -98, // y
    };
    screen_push_point(&this->pool, &cmd);

    // Read motion from pool
    PointMotion* motion = (PointMotion*)ring_peek(&this->pool);
    ASSERT_EQ(RING_OK, this->pool.last_err);
    ASSERT_EQ(sizeof(PointMotion), (unsigned)ring_pop(&this->pool));
    ASSERT_EQ(SM_Point, motion->base.type);
    EXPECT_EQ(45,  motion->x);
    EXPECT_EQ(-98, motion->y);
}

TEST_F(ScreenControllerTest, lineFromOrigin) {
    // Create line command and push it to the screen
    // 4-5-6 right triangle
    LineCmd cmd = {
        {}, // base
        0,  // x1
        0,  // y1
        3,  // x2
        4,  // y2
    };
    ASSERT_TRUE(screen_push_line(&this->pool, &cmd));

    // Read motion from pool
    LineMotion* motion = (LineMotion*)ring_peek(&this->pool);
    ASSERT_EQ(RING_OK, this->pool.last_err);
    ASSERT_EQ(sizeof(LineMotion), (unsigned)ring_pop(&this->pool));
    ASSERT_EQ(SM_Line, motion->base.type);
    EXPECT_EQ(0, motion->x1);
    EXPECT_EQ(0, motion->y1);
    EXPECT_EQ(3, motion->x2);
    EXPECT_EQ(4, motion->y2);
    EXPECT_EQ(5, motion->length);
}

TEST_F(ScreenControllerTest, lineShifted) {
    // Create line command and push it to the screen
    // 5-11-12 right triangle
    LineCmd cmd = {
        {}, // base
        -2, // x1
        5,  // y1
        3,  // x2
        17, // y2
    };
    ASSERT_TRUE(screen_push_line(&this->pool, &cmd));

    // Read motion from pool
    LineMotion* motion = (LineMotion*)ring_peek(&this->pool);
    ASSERT_EQ(RING_OK, this->pool.last_err);
    ASSERT_EQ(sizeof(LineMotion), (unsigned)ring_pop(&this->pool));
    ASSERT_EQ(SM_Line, motion->base.type);
    EXPECT_EQ(-2, motion->x1);
    EXPECT_EQ(5, motion->y1);
    EXPECT_EQ(3, motion->x2);
    EXPECT_EQ(17, motion->y2);
    EXPECT_EQ(13, motion->length);
}
