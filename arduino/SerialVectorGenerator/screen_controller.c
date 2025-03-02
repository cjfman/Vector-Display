#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#define DEBUG false

#include "ring_mem_pool.h"
#include "screen_controller.h"

 #define max(a,b)               \
   ({ __typeof__ (a) _a = (a);  \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })      \

 #define min(a,b)               \
   ({ __typeof__ (a) _a = (a);  \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })      \

static inline bool calcPoint(int elapsed, const PointMotion* motion, const ScreenState* screen, BeamState* beam) {
    beam->x = motion->x;
    beam->y = motion->y;
    if (elapsed >= screen->hold_time) {
        // Motion is complete
        beam->a = 0;
        return false;
    }
    beam->a = 1;
    return true;
}

static inline bool calcLine(int elapsed, const LineMotion* motion, const ScreenState* screen, BeamState* beam) {
    // Dert: Distance = Rate * time
    float moved = screen->speed * elapsed;
    if (moved > motion->length) {
        // Motion is complete
        beam->x = motion->x2;
        beam->y = motion->y2;
        beam->a = 0;
        return false;
    }

    // Calculate movement in each dimention
    beam->x = (motion->x2 - motion->x1) * moved / motion->length + motion->x1;
    beam->y = (motion->y2 - motion->y1) * moved / motion->length + motion->y1;
    beam->a = 1;
    return true;
}

bool nextBeamState(int elapsed, const ScreenMotion* motion, ScreenState* screen) {
    BeamState beam;
    int active;
    switch (motion->type) {
    case SM_Point:
        active = calcPoint(elapsed, (PointMotion*)motion, screen, &beam);
        break;
    case SM_Line:
        active = calcLine(elapsed, (LineMotion*)motion, screen, &beam);
        break;
    default:
        active = 0;
        beam.x = 0;
        beam.y = 0;
        beam.a = 0;
    }

    // Bounds check
    beam.x = max(min(beam.x, screen->x_width - screen->x_offset), -screen->x_offset);
    beam.y = max(min(beam.y, screen->y_width - screen->y_offset), -screen->y_offset);
    screen->beam = beam;
    
    return active;
}

void screen_init(ScreenState* screen) {
    memset(screen, '\0', sizeof(ScreenState));
    screen->x_width   = 100;
    screen->y_width   = 100;
    screen->speed     = 0.01;  // points / microsecond
    screen->hold_time = 1000;  // 1 ms
    screen->sequence_enabled = false;
    screen->sequence_idx = -1;
}

PointMotion* screen_push_point(RingMemPool* pool, const PointCmd* cmd) {
    // Allocate object from the pool
    PointMotion* motion = ring_get(pool, sizeof(PointMotion));
    if (!motion) {
        return NULL;
    }

    // Populate motion
    motion->base.type = SM_Point;
    motion->x = cmd->x;
    motion->y = cmd->y;
    return motion;
}

LineMotion* screen_push_line(RingMemPool* pool, const LineCmd* cmd) {
    // Allocate object from the pool
    LineMotion* motion = ring_get(pool, sizeof(LineMotion));
    if (!motion) {
        return NULL;
    }
    // Populate motion
    motion->base.type = SM_Line;
    motion->x1 = cmd->x1;
    motion->y1 = cmd->y1;
    motion->x2 = cmd->x2;
    motion->y2 = cmd->y2;

    // Calculate length
    motion->length = sqrt(pow(cmd->x2 - cmd->x1, 2) + pow(cmd->y2 - cmd->y1, 2));
    return motion;
}

bool update_screen(long time, ScreenState* screen, RingMemPool* pool) {
    long elapsed = time - screen->motion_start;

    // Get the current motion
    ScreenMotion* motion = (!screen->sequence_enabled) ? ring_peek(pool)                        :
                           (screen->sequence_idx >= 0) ? screen->sequence[screen->sequence_idx] :
                                                         NULL                                   ;
    if (!motion) {
        return false;
    }

    // Check for active motion
    if (screen->motion_active) {
        // Get the next motion
        if (nextBeamState(elapsed, motion, screen)) {
            // Current motion is still active
            return true;
        }
        // Motion has completed
        if (!screen->sequence_enabled) {
            ring_pop(pool);
        }
        else {
            screen->sequence_idx = (screen->sequence_idx + 1) % screen->sequence_size;
        }
        screen->motion_active = 0;
    }

    // Get the next motion
    motion = ring_peek(pool);
    motion = (!screen->sequence_enabled) ? ring_peek(pool)                        :
             (screen->sequence_idx >= 0) ? screen->sequence[screen->sequence_idx] :
                                           NULL                                   ;
    if (!motion) {
        return false;
    }

    // Determine new beam position
    screen->motion_active = 1;
    screen->motion_start = time;
    nextBeamState(0, motion, screen);
    return true;
}

// Scale position to a bit width
uint16_t position_to_binary(int pos, int scale, unsigned bits, bool dipole) {
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

// Start loading a sequence
bool sequence_start(ScreenState* screen) {
    if (screen->sequence_enabled) {
        // Sequence start has already been called
        return false;
    }
    screen->sequence_enabled = true;
    return true;
}

// Stop loading a sequence
bool sequence_end(ScreenState* screen) {
    if (!screen->sequence_enabled || screen->sequence_idx >= 0) {
        // There is no sequence to end, or it's currently running
        return false;
    }
    screen->sequence_idx = 0;
    return true;
}

// Clear any loaded sequence
bool sequence_clear(ScreenState* screen) {
    screen->sequence_enabled = false;
    screen->sequence_size    = 0;
    screen->sequence_idx     = -1;
    return true;
}

// Add to a sequence
bool add_to_sequence(ScreenState* screen, const ScreenMotion* motion) {
    if (!screen->sequence_enabled
        || screen->sequence_size >= SEQ_LEN
        || screen->sequence_idx >= 0
    ) {
        return false;
    }
    screen->sequence[screen->sequence_size++] = motion;
    return true;
}

