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
        screen_init(&this->screen);
    }

    char pool_mem[1<<10];
    RingMemPool pool;
    ScreenState screen;
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
    EXPECT_EQ(5,  motion->y1);
    EXPECT_EQ(3,  motion->x2);
    EXPECT_EQ(17, motion->y2);
    EXPECT_EQ(13, motion->length);
}

TEST_F(ScreenControllerTest, updateScreenPoint) {
    // Defaults are 100 width, 0 offset, 100ms speed, 1ms hold
    PointCmd cmd = {
        {}, // base
        54, // x
        81, // y
    };
    screen_push_point(&this->pool, &cmd);
    update_screen(0, &this->screen, &this->pool);
    EXPECT_EQ(1, this->screen.beam.a);
    EXPECT_EQ(54, this->screen.beam.x);
    EXPECT_EQ(81, this->screen.beam.y);

    // Change screen bounds
    screen.x_width  = 50;
    screen.y_width  = 86;
    screen.x_offset = 20;
    screen.y_offset = 0;
    cmd.x = 25;
    cmd.y = 9;
    screen_push_point(&this->pool, &cmd);
    update_screen(1000, &this->screen, &this->pool);
    EXPECT_EQ(1, this->screen.beam.a);
    EXPECT_EQ(25, this->screen.beam.x);
    EXPECT_EQ(9, this->screen.beam.y);

    // Progress time by 500us
    update_screen(1500, &this->screen, &this->pool);
    EXPECT_EQ(1, this->screen.beam.a);
    EXPECT_EQ(25, this->screen.beam.x);
    EXPECT_EQ(9, this->screen.beam.y);

    // Progress time by 999us
    update_screen(1999, &this->screen, &this->pool);
    EXPECT_EQ(1, this->screen.beam.a);
    EXPECT_EQ(25, this->screen.beam.x);
    EXPECT_EQ(9, this->screen.beam.y);

    // Progress time by 1us
    update_screen(2000, &this->screen, &this->pool);
    EXPECT_EQ(0, this->screen.beam.a);

    // Progress time by > 1us
    update_screen(2000, &this->screen, &this->pool);
    EXPECT_EQ(0, this->screen.beam.a);
}

TEST_F(ScreenControllerTest, updateScreenBounds) {
    // Change screen bounds
    screen.x_width  = 50;
    screen.y_width  = 86;
    screen.x_offset = 20;
    screen.y_offset = 9;

    // X is below bounds
    PointCmd cmd = {
        {},  // base
        -70, // x
        10,  // y
    };
    screen_push_point(&this->pool, &cmd);
    update_screen(0, &this->screen, &this->pool);
    EXPECT_EQ(1,   this->screen.beam.a);
    EXPECT_EQ(-20, this->screen.beam.x) << "X was below bounds";
    EXPECT_EQ(10,  this->screen.beam.y);
    ring_pop(&this->pool);

    // X is above bounds
    cmd.x = 110;
    cmd.y = -1;
    screen_push_point(&this->pool, &cmd);
    update_screen(0, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(30, this->screen.beam.x) << "X was above bounds";
    EXPECT_EQ(-1, this->screen.beam.y);
    ring_pop(&this->pool);

    // Y is below bounds
    cmd.x = 0;
    cmd.y = -89;
    screen_push_point(&this->pool, &cmd);
    update_screen(0, &this->screen, &this->pool);
    EXPECT_EQ(1,   this->screen.beam.a);
    EXPECT_EQ(0, this->screen.beam.x) << "X was below bounds";
    EXPECT_EQ(-9,  this->screen.beam.y);
    ring_pop(&this->pool);

    // Y is above bounds
    cmd.x = -1;
    cmd.y = 100;
    screen_push_point(&this->pool, &cmd);
    update_screen(0, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(-1, this->screen.beam.x) << "X was above bounds";
    EXPECT_EQ(77, this->screen.beam.y);
    ring_pop(&this->pool);
}

/*
TEST_F(ScreenControllerTest, updateScreenLine) {
    LineCmd cmd {
        {}, // base
        0,  // x1
        0,  // y1
        30, // x2
        40, // y2
    };
    screen_push_line(&this->pool, &cmd);
    update_screen(0, &this->screen, &this->pool);
}
*/
