#ifndef SCREEN_CONTROLLER_HH
#define SCREEN_CONTROLLER_HH

#include <inttypes.h>
#include <stdbool.h>

#include "command_parser.h"

#define SEQ_LEN 16


typedef struct BeamState {
    int16_t x;
    int16_t y;
    int a; // Active
} BeamState;

typedef enum ScreenMotionType {
    SM_None = 0,
    SM_Point,
    SM_Line,
} ScreenMotionType;

typedef struct ScreenMotion {
    ScreenMotionType type;
} ScreenMotion;

typedef struct PointMotion {
    ScreenMotion base;
    int16_t x;
    int16_t y;
} PointMotion;

typedef struct LineMotion {
    ScreenMotion base;
#ifndef AVR
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
#endif
    int32_t mx1;
    int32_t my1;
    int32_t mx2;
    int32_t my2;
    int16_t dx; // Millipoints per microsecond
    int16_t dy; // Millipoints per microsecond
} LineMotion;

typedef struct ScreenState {
    uint8_t x_size_pow; // Size is a power of 2
    uint8_t y_size_pow; // Size is a power of 2
    bool x_centered;
    bool y_centered;
    uint16_t hold_time;    // Time to hold a point
    uint16_t speed;        // Millipoints moved in a microsecond
    uint32_t motion_start; // Time when current motion started
    BeamState beam;
    bool motion_active;
    bool sequence_enabled;
    int8_t sequence_size;
    int8_t sequence_idx;
    ScreenMotion* sequence[SEQ_LEN];
} ScreenState;

void screen_init(ScreenState* screen);
PointMotion* screen_push_point(RingMemPool* pool, const PointCmd* cmd);
LineMotion* screen_push_line(RingMemPool* pool, const LineCmd* cmd, uint16_t speed);
bool update_screen(uint32_t time, ScreenState* screen, RingMemPool* pool);
bool sequence_start(ScreenState* screen);
bool sequence_end(ScreenState* screen);
bool sequence_clear(ScreenState* screen);
bool add_to_sequence(ScreenState* screen, ScreenMotion* motion);

#endif // SCREEN_CONTROLLER_HH
