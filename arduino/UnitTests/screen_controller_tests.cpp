#include <string.h>

#include <string>

#include "gtest/gtest.h"

extern "C" {
#include "command_parser.h"
#include "ring_mem_pool.h"
#include "screen_controller.h"
#include "utils.h"
}

class ScreenControllerTest: public testing::Test {
protected:
    void SetUp() {
        ring_init(&this->pool, this->pool_mem, sizeof(this->pool_mem));
        screen_init(&this->screen);
        // Assumes default values of
        // hold_time = 1000
        this->screen.x_size_pow = 10;
        this->screen.y_size_pow = 10;
        this->screen.speed = 10000;
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
    ASSERT_TRUE(screen_push_line(&this->pool, &cmd, this->screen.speed));

    // Read motion from pool
    LineMotion* motion = (LineMotion*)ring_peek(&this->pool);
    ASSERT_EQ(RING_OK, this->pool.last_err);
    ASSERT_EQ(sizeof(LineMotion), (unsigned)ring_pop(&this->pool));
    ASSERT_EQ(SM_Line, motion->base.type);
    EXPECT_EQ(0, motion->x1);
    EXPECT_EQ(0, motion->y1);
    EXPECT_EQ(3, motion->x2);
    EXPECT_EQ(4, motion->y2);
    EXPECT_EQ(0,    motion->mx1);
    EXPECT_EQ(0,    motion->my1);
    EXPECT_EQ(3000, motion->mx2);
    EXPECT_EQ(4000, motion->my2);
    EXPECT_EQ(6000, motion->dx);
    EXPECT_EQ(8000, motion->dy);
}

TEST_F(ScreenControllerTest, longLineFromOrigin) {
    // Create line command and push it to the screen
    // 4-5-6 right triangle
    LineCmd cmd = {
        {},  // base
        0,   // x1
        0,   // y1
        300, // x2
        400, // y2
    };
    ASSERT_TRUE(screen_push_line(&this->pool, &cmd, this->screen.speed));

    // Read motion from pool
    LineMotion* motion = (LineMotion*)ring_peek(&this->pool);
    ASSERT_EQ(RING_OK, this->pool.last_err);
    ASSERT_EQ(sizeof(LineMotion), (unsigned)ring_pop(&this->pool));
    ASSERT_EQ(SM_Line, motion->base.type);
    EXPECT_EQ(0,   motion->x1);
    EXPECT_EQ(0,   motion->y1);
    EXPECT_EQ(300, motion->x2);
    EXPECT_EQ(400, motion->y2);
    EXPECT_EQ(0,      motion->mx1);
    EXPECT_EQ(0,      motion->my1);
    EXPECT_EQ(300000, motion->mx2);
    EXPECT_EQ(400000, motion->my2);
    EXPECT_EQ(6000, motion->dx);
    EXPECT_EQ(8000, motion->dy);
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
    ASSERT_TRUE(screen_push_line(&this->pool, &cmd, this->screen.speed));

    // Read motion from pool
    LineMotion* motion = (LineMotion*)ring_peek(&this->pool);
    ASSERT_EQ(RING_OK, this->pool.last_err);
    ASSERT_EQ(sizeof(LineMotion), (unsigned)ring_pop(&this->pool));
    ASSERT_EQ(SM_Line, motion->base.type);
    EXPECT_EQ(-3, motion->x1);
    EXPECT_EQ(-4, motion->y1);
    EXPECT_EQ(3,  motion->x2);
    EXPECT_EQ(4,  motion->y2);
//    EXPECT_EQ(10, motion->length);
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
    ASSERT_TRUE(screen_push_line(&this->pool, &cmd, this->screen.speed));

    // Read motion from pool
    LineMotion* motion = (LineMotion*)ring_peek(&this->pool);
    ASSERT_EQ(RING_OK, this->pool.last_err);
    ASSERT_EQ(sizeof(LineMotion), (unsigned)ring_pop(&this->pool));
    ASSERT_EQ(SM_Line, motion->base.type);
    EXPECT_EQ(-2, motion->x1);
    EXPECT_EQ(5,  motion->y1);
    EXPECT_EQ(3,  motion->x2);
    EXPECT_EQ(17, motion->y2);
//    EXPECT_EQ(13, motion->length);
}

TEST_F(ScreenControllerTest, updateScreenPoint) {
    // Defaults are 10 size power, origin in bottom left corner, 100ms speed, 1ms hold
    PointCmd cmd = {
        {}, // base
        54, // x
        81, // y
    };
    screen_push_point(&this->pool, &cmd);
    update_screen(0, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(54, this->screen.beam.x);
    EXPECT_EQ(81, this->screen.beam.y);

    // Change screen bounds
    screen.x_size_pow = 8;
    screen.y_size_pow = 8;
    screen.x_centered = true;
    screen.y_centered = true;
    cmd.x = 25;
    cmd.y = 9;
    screen_push_point(&this->pool, &cmd);
    update_screen(1000, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(25, this->screen.beam.x);
    EXPECT_EQ(9,  this->screen.beam.y);

    // Progress time by 500us
    update_screen(1500, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(25, this->screen.beam.x);
    EXPECT_EQ(9,  this->screen.beam.y);

    // Progress time by 999us
    update_screen(1999, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(25, this->screen.beam.x);
    EXPECT_EQ(9,  this->screen.beam.y);

    // Progress time by 1us
    update_screen(2000, &this->screen, &this->pool);
    EXPECT_EQ(0, this->screen.beam.a);

    // Progress time by > 1us
    update_screen(2000, &this->screen, &this->pool);
    EXPECT_EQ(0, this->screen.beam.a);
}

TEST_F(ScreenControllerTest, updateScreenBounds) {
    // Change screen bounds
    screen.x_size_pow = 6;
    screen.y_size_pow = 5;
    screen.x_centered = true;
    screen.y_centered = true;

    // X is below bounds
    PointCmd cmd = {
        {},  // base
        -70, // x
        10,  // y
    };
    screen_push_point(&this->pool, &cmd);
    update_screen(0, &this->screen, &this->pool);
    EXPECT_EQ(1,   this->screen.beam.a);
    EXPECT_EQ(-32, this->screen.beam.x) << "X was below bounds";
    EXPECT_EQ(10,  this->screen.beam.y);
    ring_pop(&this->pool);

    // X is above bounds
    cmd.x = 110;
    cmd.y = -1;
    screen_push_point(&this->pool, &cmd);
    update_screen(0, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(32, this->screen.beam.x) << "X was above bounds";
    EXPECT_EQ(-1, this->screen.beam.y);
    ring_pop(&this->pool);

    // Y is below bounds
    cmd.x = 0;
    cmd.y = -89;
    screen_push_point(&this->pool, &cmd);
    update_screen(0, &this->screen, &this->pool);
    EXPECT_EQ(1,   this->screen.beam.a);
    EXPECT_EQ(0, this->screen.beam.x) << "X was below bounds";
    EXPECT_EQ(-16,  this->screen.beam.y);
    ring_pop(&this->pool);

    // Y is above bounds
    cmd.x = -1;
    cmd.y = 100;
    screen_push_point(&this->pool, &cmd);
    update_screen(0, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(-1, this->screen.beam.x) << "X was above bounds";
    EXPECT_EQ(16, this->screen.beam.y);
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
    screen_push_line(&this->pool, &cmd, this->screen.speed);

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

TEST_F(ScreenControllerTest, updateScreenLongLineFromOrigin) {
    this->screen.x_size_pow = 11;
    this->screen.y_size_pow = 11;
    this->screen.x_centered = true;
    this->screen.y_centered = true;
    this->screen.speed      = 10;
    LineCmd cmd = {
        {},  // base
        0,   // x1
        0,   // y1
        300, // x2
        400, // y2
    };
    // Line length of 50
    screen_push_line(&this->pool, &cmd, this->screen.speed);

    // t = 0us; 0% of line
    update_screen(0, &this->screen, &this->pool);
    EXPECT_EQ(1, this->screen.beam.a);
    EXPECT_EQ(0, this->screen.beam.x);
    EXPECT_EQ(0, this->screen.beam.y);

    // t = 10ms; 20% of line
    update_screen(10000, &this->screen, &this->pool);
    EXPECT_EQ(1,  this->screen.beam.a);
    EXPECT_EQ(60, this->screen.beam.x);
    EXPECT_EQ(80, this->screen.beam.y);

    // t = 20ms; 40% of line
    update_screen(20000, &this->screen, &this->pool);
    EXPECT_EQ(1,   this->screen.beam.a);
    EXPECT_EQ(120, this->screen.beam.x);
    EXPECT_EQ(160, this->screen.beam.y);

    // t = 30ms; 60% of line
    update_screen(30000, &this->screen, &this->pool);
    EXPECT_EQ(1,   this->screen.beam.a);
    EXPECT_EQ(180, this->screen.beam.x);
    EXPECT_EQ(240, this->screen.beam.y);

    // t = 40ms; 80% of line
    update_screen(40000, &this->screen, &this->pool);
    EXPECT_EQ(1,   this->screen.beam.a);
    EXPECT_EQ(240, this->screen.beam.x);
    EXPECT_EQ(320, this->screen.beam.y);

    // t = 50ms; 100% of line
    update_screen(50000, &this->screen, &this->pool);
    EXPECT_EQ(1,   this->screen.beam.a);
    EXPECT_EQ(300, this->screen.beam.x);
    EXPECT_EQ(400, this->screen.beam.y);

    // t > 50ms; >100% of line
    update_screen(60000, &this->screen, &this->pool);
    EXPECT_EQ(0,   this->screen.beam.a);
    EXPECT_EQ(300, this->screen.beam.x);
    EXPECT_EQ(400, this->screen.beam.y);
}

TEST_F(ScreenControllerTest, updateScreenLineCornerToCorner) {
    this->screen.x_size_pow = 9;
    this->screen.y_size_pow = 9;
    this->screen.x_centered = true;
    this->screen.y_centered = true;
    LineCmd cmd = {
        {},  // base
        -30, // x1
        -40, // y1
        30,  // x2
        40,  // y2
    };
    // Line length of 50
    screen_push_line(&this->pool, &cmd, this->screen.speed);

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

TEST_F(ScreenControllerTest, updateScreenLongLineCornerToCorner) {
    this->screen.x_size_pow = 11;
    this->screen.y_size_pow = 11;
    this->screen.x_centered = true;
    this->screen.y_centered = true;
    this->screen.speed      = 10;
    LineCmd cmd = {
        {},   // base
        -300, // x1
        -400, // y1
        300,  // x2
        400,  // y2
    };
    // Line length of 50
    screen_push_line(&this->pool, &cmd, this->screen.speed);

    // t = 0ms; 0% of line
    update_screen(0, &this->screen, &this->pool);
    EXPECT_EQ(1,   this->screen.beam.a);
    EXPECT_EQ(-300, this->screen.beam.x);
    EXPECT_EQ(-400, this->screen.beam.y);

    // t = 20ms; 20% of line
    update_screen(20000, &this->screen, &this->pool);
    EXPECT_EQ(1,    this->screen.beam.a);
    EXPECT_EQ(-180, this->screen.beam.x);
    EXPECT_EQ(-240, this->screen.beam.y);

    // t = 40ms; 40% of line
    update_screen(40000, &this->screen, &this->pool);
    EXPECT_EQ(1,    this->screen.beam.a);
    EXPECT_EQ(-60, this->screen.beam.x);
    EXPECT_EQ(-80, this->screen.beam.y);

    // t = 60ms; 60% of line
    update_screen(60000, &this->screen, &this->pool);
    EXPECT_EQ(1,   this->screen.beam.a);
    EXPECT_EQ(60, this->screen.beam.x);
    EXPECT_EQ(80, this->screen.beam.y);

    // t = 80ms; 80% of line
    update_screen(80000, &this->screen, &this->pool);
    EXPECT_EQ(1,   this->screen.beam.a);
    EXPECT_EQ(180, this->screen.beam.x);
    EXPECT_EQ(240, this->screen.beam.y);

    // t = 100ms; 100% of line
    update_screen(100000, &this->screen, &this->pool);
    EXPECT_EQ(1,   this->screen.beam.a);
    EXPECT_EQ(300, this->screen.beam.x);
    EXPECT_EQ(400, this->screen.beam.y);

    // t > 100ms; >100% of line
    update_screen(110000, &this->screen, &this->pool);
    EXPECT_EQ(0,   this->screen.beam.a);
    EXPECT_EQ(300, this->screen.beam.x);
    EXPECT_EQ(400, this->screen.beam.y);
}

TEST_F(ScreenControllerTest, emtpySequence) {
    // Before start is called
    ASSERT_TRUE(sequence_clear(&this->screen)) << "Reset should always work";
    ASSERT_TRUE(sequence_clear(&this->screen)) << "Reset should always work";
    ASSERT_FALSE(sequence_end(&this->screen)) << "End should fail unless start has been called";

    // Start and end
    ASSERT_TRUE(sequence_start(&this->screen));
    EXPECT_TRUE(this->screen.sequence_enabled);
    ASSERT_FALSE(sequence_start(&this->screen)) << "Start should fail when called for a second time before end is called";
    ASSERT_TRUE(sequence_end(&this->screen));
    EXPECT_TRUE(this->screen.sequence_enabled);

    // Clear
    ASSERT_FALSE(sequence_end(&this->screen)) << "End should fail when called for a second time until start has been called again";
    ASSERT_FALSE(sequence_start(&this->screen)) << "Start should fail when called for a second time before end is called";
    EXPECT_TRUE(this->screen.sequence_enabled);
    ASSERT_TRUE(sequence_clear(&this->screen)) << "Reset should always work";
    EXPECT_FALSE(this->screen.sequence_enabled);

    // Mid sequence clear
    ASSERT_TRUE(sequence_start(&this->screen));
    EXPECT_TRUE(this->screen.sequence_enabled);
    ASSERT_FALSE(sequence_start(&this->screen)) << "Start should fail when called for a second time before end is called";
    ASSERT_TRUE(sequence_clear(&this->screen)) << "Reset should always work";
    EXPECT_FALSE(this->screen.sequence_enabled);
    ASSERT_TRUE(sequence_clear(&this->screen)) << "Reset should always work";
    ASSERT_FALSE(sequence_end(&this->screen)) << "End should fail unless start has been called";
    ASSERT_TRUE(sequence_start(&this->screen));
    EXPECT_TRUE(this->screen.sequence_enabled);
    ASSERT_TRUE(sequence_end(&this->screen));
    EXPECT_TRUE(this->screen.sequence_enabled);
}

TEST_F(ScreenControllerTest, basicSequence) {
    ASSERT_TRUE(sequence_start(&this->screen));
    ASSERT_TRUE(this->screen.sequence_enabled);

    // Motion one
    ScreenMotion motion1;
    add_to_sequence(&this->screen, &motion1);
    ASSERT_EQ(1, this->screen.sequence_size);
    EXPECT_EQ(-1, this->screen.sequence_idx);
    EXPECT_EQ(&motion1, this->screen.sequence[0]);

    // Motion two
    ScreenMotion motion2;
    add_to_sequence(&this->screen, &motion2);
    ASSERT_EQ(2, this->screen.sequence_size);
    EXPECT_EQ(-1, this->screen.sequence_idx);
    EXPECT_EQ(&motion2, this->screen.sequence[1]);

    // Motion three
    ScreenMotion motion3;
    add_to_sequence(&this->screen, &motion3);
    ASSERT_EQ(3, this->screen.sequence_size);
    EXPECT_EQ(-1, this->screen.sequence_idx);
    EXPECT_EQ(&motion3, this->screen.sequence[2]);

    // End and clear
    ASSERT_TRUE(sequence_end(&this->screen));
    EXPECT_TRUE(this->screen.sequence_enabled);
    EXPECT_EQ(0, this->screen.sequence_idx);
    ASSERT_TRUE(sequence_clear(&this->screen));
    EXPECT_FALSE(this->screen.sequence_enabled);
    EXPECT_EQ(-1, this->screen.sequence_idx);
}

TEST(ScreenController, unsignedPositionTo16Bits) {
    // Power 4
    EXPECT_EQ(0x0000u, position_to_binary(0,  4,  16, false));
    EXPECT_EQ(0x0fffu, position_to_binary(1,  4,  16, false));
    EXPECT_EQ(0x4fffu, position_to_binary(5,  4,  16, false));
    EXPECT_EQ(0x8fffu, position_to_binary(9,  4,  16, false));
    EXPECT_EQ(0x9fffu, position_to_binary(10, 4,  16, false));

    // Power 7
    EXPECT_EQ(0x0000u, position_to_binary(0,   7, 16, false));
    EXPECT_EQ(0x13ffu, position_to_binary(10,  7, 16, false));
    EXPECT_EQ(0x63ffu, position_to_binary(50,  7, 16, false));
    EXPECT_EQ(0xc5ffu, position_to_binary(99,  7, 16, false));
    EXPECT_EQ(0xc7ffu, position_to_binary(100, 7, 16, false));

    // Power 10
    EXPECT_EQ(0x0000u, position_to_binary(0,    10, 16, false));
    EXPECT_EQ(0x18ffu, position_to_binary(100,  10, 16, false));
    EXPECT_EQ(0x7cffu, position_to_binary(500,  10, 16, false));
    EXPECT_EQ(0xf9bfu, position_to_binary(999,  10, 16, false));
    EXPECT_EQ(0xf9ffu, position_to_binary(1000, 10, 16, false));
}

TEST(ScreenController, signedPositionTo16Bits) {
    // Power 4
    EXPECT_EQ(0x0000u, position_to_binary(0,  4,  16, true));
    EXPECT_EQ(0x07ffu, position_to_binary(1,  4,  16, true));
    EXPECT_EQ(0x27ffu, position_to_binary(5,  4,  16, true));
    EXPECT_EQ(0x47ffu, position_to_binary(9,  4,  16, true));
    EXPECT_EQ(0x67ffu, position_to_binary(13, 4,  16, true));

    // Power 7
    EXPECT_EQ(0x0000u, position_to_binary(0,   7, 16, true));
    EXPECT_EQ(0x09ffu, position_to_binary(10,  7, 16, true));
    EXPECT_EQ(0x31ffu, position_to_binary(50,  7, 16, true));
    EXPECT_EQ(0x62ffu, position_to_binary(99,  7, 16, true));
    EXPECT_EQ(0x63ffu, position_to_binary(100, 7, 16, true));

    // Scale 10
    EXPECT_EQ(0x0000u, position_to_binary(0,    10, 16, true));
    EXPECT_EQ(0x0c7fu, position_to_binary(100,  10, 16, true));
    EXPECT_EQ(0x3e7fu, position_to_binary(500,  10, 16, true));
    EXPECT_EQ(0x7cdfu, position_to_binary(999,  10, 16, true));
    EXPECT_EQ(0x7d1fu, position_to_binary(1001, 10, 16, true));
}

/*
TEST(ScreenController, unsignedPositionTo12Bits) {
    // Scale 10
    EXPECT_EQ(0x0000u, position_to_binary(0,  10,  12, false));
    EXPECT_EQ(0x0199u, position_to_binary(1,  10,  12, false));
    EXPECT_EQ(0x07ffu, position_to_binary(5,  10,  12, false));
    EXPECT_EQ(0x0e65u, position_to_binary(9,  10,  12, false));
    EXPECT_EQ(0x0fffu, position_to_binary(10, 10,  12, false));

    // Scale 100
    EXPECT_EQ(0x0000u, position_to_binary(0,   100, 12, false));
    EXPECT_EQ(0x0199u, position_to_binary(10,  100, 12, false));
    EXPECT_EQ(0x07ffu, position_to_binary(50,  100, 12, false));
    EXPECT_EQ(0x0fd6u, position_to_binary(99,  100, 12, false));
    EXPECT_EQ(0x0fffu, position_to_binary(100, 100, 12, false));

    // Scale 1000
    EXPECT_EQ(0x0000u, position_to_binary(0,    1000, 12, false));
    EXPECT_EQ(0x0199u, position_to_binary(100,  1000, 12, false));
    EXPECT_EQ(0x07ffu, position_to_binary(500,  1000, 12, false));
    EXPECT_EQ(0x0ffau, position_to_binary(999,  1000, 12, false));
    EXPECT_EQ(0x0fffu, position_to_binary(1000, 1000, 12, false));
}

TEST(ScreenController, signedPositionTo12Bits) {
    // Scale 10
    EXPECT_EQ(0x0000u, position_to_binary(0,  10,  12, true));
    EXPECT_EQ(0x00ccu, position_to_binary(1,  10,  12, true));
    EXPECT_EQ(0x03ffu, position_to_binary(5,  10,  12, true));
    EXPECT_EQ(0x0732u, position_to_binary(9,  10,  12, true));
    EXPECT_EQ(0x07ffu, position_to_binary(10, 10,  12, true));

    // Scale 100
    EXPECT_EQ(0x0000u, position_to_binary(0,   100, 12, true));
    EXPECT_EQ(0x00ccu, position_to_binary(10,  100, 12, true));
    EXPECT_EQ(0x03ffu, position_to_binary(50,  100, 12, true));
    EXPECT_EQ(0x07eau, position_to_binary(99,  100, 12, true));
    EXPECT_EQ(0x07ffu, position_to_binary(100, 100, 12, true));

    // Scale 1000
    EXPECT_EQ(0x0000u, position_to_binary(0,    1000, 12, true));
    EXPECT_EQ(0x00ccu, position_to_binary(100,  1000, 12, true));
    EXPECT_EQ(0x03ffu, position_to_binary(500,  1000, 12, true));
    EXPECT_EQ(0x07fcu, position_to_binary(999,  1000, 12, true));
    EXPECT_EQ(0x07ffu, position_to_binary(1000, 1000, 12, true));
}
*/
