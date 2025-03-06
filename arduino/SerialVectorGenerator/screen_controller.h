#ifndef SCREEN_CONTROLLER_HH
#define SCREEN_CONTROLLER_HH

#include <inttypes.h>
#include <stdbool.h>

#include "command_parser.h"

#define SEQ_LEN 16

#ifndef max
#define max(a,b)                \
   ({ __typeof__ (a) _a = (a);  \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })      \

#endif // max

#ifndef min
#define min(a,b)                \
   ({ __typeof__ (a) _a = (a);  \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })      \

#endif // mix

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
    int32_t dx; // Points per microsecond
    int32_t dy; // Points per microsecond
} LineMotion;

typedef struct ScreenState {
    int16_t x_width;
    int16_t y_width;
    int16_t x_offset;
    int16_t y_offset;
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
bool add_to_sequence(ScreenState* screen, const ScreenMotion* motion);

// Scale position to a bit width
static inline uint16_t position_to_binary(int pos, int scale, unsigned bits, bool dipole) {
    bits = min(16, bits);
    if (dipole) {
        bits--;
    }
    else {
        pos = max(0, pos);
    }

    long max_value = (1l << bits) - 1;
    return (uint16_t)((max_value * pos / scale) & 0xFFFF);
}

#endif // SCREEN_CONTROLLER_HH
