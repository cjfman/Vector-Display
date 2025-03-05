#ifndef SCREEN_CONTROLLER_HH
#define SCREEN_CONTROLLER_HH

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
    int x;
    int y;
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
    int x;
    int y;
} PointMotion;

typedef struct LineMotion {
    ScreenMotion base;
#ifndef AVR
    long x1;
    long y1;
    long x2;
    long y2;
#endif
    long mx1;
    long my1;
    long mx2;
    long my2;
    long dx; // Points per microsecond
    long dy; // Points per microsecond
} LineMotion;

typedef struct ScreenState {
    int x_width;
    int y_width;
    int x_offset;
    int y_offset;
    int hold_time; // Time to hold a point
    long speed;        // Millipoints moved in a microsecond
    long motion_start; // Time when current motion started
    BeamState beam;
    int motion_active;
    bool sequence_enabled;
    int sequence_size;
    int sequence_idx;
    ScreenMotion* sequence[SEQ_LEN];
} ScreenState;

void screen_init(ScreenState* screen);
PointMotion* screen_push_point(RingMemPool* pool, const PointCmd* cmd);
LineMotion* screen_push_line(RingMemPool* pool, const LineCmd* cmd, long speed);
bool update_screen(long time, ScreenState* screen, RingMemPool* pool);
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
