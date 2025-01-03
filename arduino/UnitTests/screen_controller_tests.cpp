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
        // Assumes default values of
        // x_width   = 100
        // y_width   = 100
        // hold_time = 1000
        this->screen.speed = 10;
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

TEST_F(ScreenControllerTest, lineThroughOrigin) {
    // Create line command and push it to the screen
    // 4-5-6 right triangle
    LineCmd cmd = {
        {}, // base
        -3, // x1
        -4, // y1
        3,  // x2
        4,  // y2
    };
    ASSERT_TRUE(screen_push_line(&this->pool, &cmd));

    // Read motion from pool
    LineMotion* motion = (LineMotion*)ring_peek(&this->pool);
    ASSERT_EQ(RING_OK, this->pool.last_err);
    ASSERT_EQ(sizeof(LineMotion), (unsigned)ring_pop(&this->pool));
    ASSERT_EQ(SM_Line, motion->base.type);
    EXPECT_EQ(-3, motion->x1);
    EXPECT_EQ(-4, motion->y1);
    EXPECT_EQ(3,  motion->x2);
    EXPECT_EQ(4,  motion->y2);
    EXPECT_EQ(10, motion->length);
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

TEST_F(ScreenControllerTest, updateScreenLineFromOrigin) {
    LineCmd cmd = {
        {}, // base
        0,  // x1
        0,  // y1
        30, // x2
        40, // y2
    };
    // Line length of 50
    screen_push_line(&this->pool, &cmd);

    // t = 0us; 0% of line
    update_screen(0, &this->screen, &this->pool);
    EXPECT_EQ(1, this->screen.beam.a);
    EXPECT_EQ(0, this->screen.beam.x);
    EXPECT_EQ(0, this->screen.beam.y);

    // t = 1us; 20% of line
    update_screen(1, &this->screen, &this->pool);
    EXPECT_EQ(1, this->screen.beam.a);
    EXPECT_EQ(6, this->screen.beam.x);
    EXPECT_EQ(8, this->screen.beam.y);

    // t = 2us; 40% of line
    update_screen(2, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(12, this->screen.beam.x);
    EXPECT_EQ(16, this->screen.beam.y);

    // t = 3us; 60% of line
    update_screen(3, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(18, this->screen.beam.x);
    EXPECT_EQ(24, this->screen.beam.y);

    // t = 4us; 80% of line
    update_screen(4, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(24, this->screen.beam.x);
    EXPECT_EQ(32, this->screen.beam.y);

    // t = 5us; 100% of line
    update_screen(5, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(30, this->screen.beam.x);
    EXPECT_EQ(40, this->screen.beam.y);

    // t > 5us; >100% of line
    update_screen(6, &this->screen, &this->pool);
    EXPECT_EQ(0,  this->screen.beam.a);
}

TEST_F(ScreenControllerTest, updateScreenLineCornerToCorner) {
    this->screen.x_width  = 200;
    this->screen.y_width  = 200;
    this->screen.x_offset = 100;
    this->screen.y_offset = 100;
    LineCmd cmd = {
        {},  // base
        -30, // x1
        -40, // y1
        30,  // x2
        40,  // y2
    };
    // Line length of 50
    screen_push_line(&this->pool, &cmd);

    // t = 0us; 0% of line
    update_screen(0, &this->screen, &this->pool);
    EXPECT_EQ(1,   this->screen.beam.a);
    EXPECT_EQ(-30, this->screen.beam.x);
    EXPECT_EQ(-40, this->screen.beam.y);

    // t = 1us; 20% of line
    update_screen(2, &this->screen, &this->pool);
    EXPECT_EQ(1,   this->screen.beam.a);
    EXPECT_EQ(-18, this->screen.beam.x);
    EXPECT_EQ(-24, this->screen.beam.y);

    // t = 2us; 40% of line
    update_screen(4, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(-6, this->screen.beam.x);
    EXPECT_EQ(-8, this->screen.beam.y);

    // t = 3us; 60% of line
    update_screen(6, &this->screen, &this->pool);
    EXPECT_EQ(1, this->screen.beam.a);
    EXPECT_EQ(6, this->screen.beam.x);
    EXPECT_EQ(8, this->screen.beam.y);

    // t = 4us; 80% of line
    update_screen(8, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(18, this->screen.beam.x);
    EXPECT_EQ(24, this->screen.beam.y);

    // t = 5us; 100% of line
    update_screen(10, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(30, this->screen.beam.x);
    EXPECT_EQ(40, this->screen.beam.y);

    // t > 5us; >100% of line
    update_screen(11, &this->screen, &this->pool);
    EXPECT_EQ(0,  this->screen.beam.a);
}

TEST(ScreenController, unsignedPositionTo16Bits) {
    // Scale 10
    EXPECT_EQ(0x0000, position_to_binary(0,  10,  16, false));
    EXPECT_EQ(0x1999, position_to_binary(1,  10,  16, false));
    EXPECT_EQ(0x7fff, position_to_binary(5,  10,  16, false));
    EXPECT_EQ(0xe665, position_to_binary(9,  10,  16, false));
    EXPECT_EQ(0xffff, position_to_binary(10, 10,  16, false));

    // Scale 100
    EXPECT_EQ(0x0000, position_to_binary(0,   100, 16, false));
    EXPECT_EQ(0x1999, position_to_binary(10,  100, 16, false));
    EXPECT_EQ(0x7fff, position_to_binary(50,  100, 16, false));
    EXPECT_EQ(0xfd6f, position_to_binary(99,  100, 16, false));
    EXPECT_EQ(0xffff, position_to_binary(100, 100, 16, false));

    // Scale 1000
    EXPECT_EQ(0x0000, position_to_binary(0,    1000, 16, false));
    EXPECT_EQ(0x1999, position_to_binary(100,  1000, 16, false));
    EXPECT_EQ(0x7fff, position_to_binary(500,  1000, 16, false));
    EXPECT_EQ(0xffbd, position_to_binary(999,  1000, 16, false));
    EXPECT_EQ(0xffff, position_to_binary(1000, 1000, 16, false));
}

TEST(ScreenController, signedPositionTo16Bits) {
    // Scale 10
    EXPECT_EQ(0x0000, position_to_binary(0,  10,  16, true));
    EXPECT_EQ(0x0ccc, position_to_binary(1,  10,  16, true));
    EXPECT_EQ(0x3fff, position_to_binary(5,  10,  16, true));
    EXPECT_EQ(0x7332, position_to_binary(9,  10,  16, true));
    EXPECT_EQ(0x7fff, position_to_binary(10, 10,  16, true));

    // Scale 100
    EXPECT_EQ(0x0000, position_to_binary(0,   100, 16, true));
    EXPECT_EQ(0x0ccc, position_to_binary(10,  100, 16, true));
    EXPECT_EQ(0x3fff, position_to_binary(50,  100, 16, true));
    EXPECT_EQ(0x7eb7, position_to_binary(99,  100, 16, true));
    EXPECT_EQ(0x7fff, position_to_binary(100, 100, 16, true));

    // Scale 1000
    EXPECT_EQ(0x0000, position_to_binary(0,    1000, 16, true));
    EXPECT_EQ(0x0ccc, position_to_binary(100,  1000, 16, true));
    EXPECT_EQ(0x3fff, position_to_binary(500,  1000, 16, true));
    EXPECT_EQ(0x7fde, position_to_binary(999,  1000, 16, true));
    EXPECT_EQ(0x7fff, position_to_binary(1000, 1000, 16, true));
}

TEST(ScreenController, unsignedPositionTo12Bits) {
    // Scale 10
    EXPECT_EQ(0x0000, position_to_binary(0,  10,  12, false));
    EXPECT_EQ(0x0199, position_to_binary(1,  10,  12, false));
    EXPECT_EQ(0x07ff, position_to_binary(5,  10,  12, false));
    EXPECT_EQ(0x0e65, position_to_binary(9,  10,  12, false));
    EXPECT_EQ(0x0fff, position_to_binary(10, 10,  12, false));

    // Scale 100
    EXPECT_EQ(0x0000, position_to_binary(0,   100, 12, false));
    EXPECT_EQ(0x0199, position_to_binary(10,  100, 12, false));
    EXPECT_EQ(0x07ff, position_to_binary(50,  100, 12, false));
    EXPECT_EQ(0x0fd6, position_to_binary(99,  100, 12, false));
    EXPECT_EQ(0x0fff, position_to_binary(100, 100, 12, false));

    // Scale 1000
    EXPECT_EQ(0x0000, position_to_binary(0,    1000, 12, false));
    EXPECT_EQ(0x0199, position_to_binary(100,  1000, 12, false));
    EXPECT_EQ(0x07ff, position_to_binary(500,  1000, 12, false));
    EXPECT_EQ(0x0ffa, position_to_binary(999,  1000, 12, false));
    EXPECT_EQ(0x0fff, position_to_binary(1000, 1000, 12, false));
}

TEST(ScreenController, signedPositionTo12Bits) {
    // Scale 10
    EXPECT_EQ(0x0000, position_to_binary(0,  10,  12, true));
    EXPECT_EQ(0x00cc, position_to_binary(1,  10,  12, true));
    EXPECT_EQ(0x03ff, position_to_binary(5,  10,  12, true));
    EXPECT_EQ(0x0732, position_to_binary(9,  10,  12, true));
    EXPECT_EQ(0x07ff, position_to_binary(10, 10,  12, true));

    // Scale 100
    EXPECT_EQ(0x0000, position_to_binary(0,   100, 12, true));
    EXPECT_EQ(0x00cc, position_to_binary(10,  100, 12, true));
    EXPECT_EQ(0x03ff, position_to_binary(50,  100, 12, true));
    EXPECT_EQ(0x07ea, position_to_binary(99,  100, 12, true));
    EXPECT_EQ(0x07ff, position_to_binary(100, 100, 12, true));

    // Scale 1000
    EXPECT_EQ(0x0000, position_to_binary(0,    1000, 12, true));
    EXPECT_EQ(0x00cc, position_to_binary(100,  1000, 12, true));
    EXPECT_EQ(0x03ff, position_to_binary(500,  1000, 12, true));
    EXPECT_EQ(0x07fc, position_to_binary(999,  1000, 12, true));
    EXPECT_EQ(0x07ff, position_to_binary(1000, 1000, 12, true));
}
