#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#define DEBUG false

#include "ring_mem_pool.h"
#include "screen_controller.h"

static inline bool calcPoint(uint16_t elapsed, const PointMotion* motion, const ScreenState* screen, BeamState* beam) {
    beam->x = motion->x;
    beam->y = motion->y;
#ifdef AVR
    elapsed = elapsed >> 10; // Dividing by 1024 is close enough
#else
    elapsed /= 1000;
#endif
    if (elapsed >= screen->hold_time) {
        // Motion is complete
        beam->a = 0;
        return false;
    }
    beam->a = 1;
    return true;
}

static inline bool line_completed(int32_t end, int32_t pos, bool direction) {
    return ((direction && pos > end) || (!direction && (pos < end)));
}

static inline bool calcLine(uint16_t elapsed, const LineMotion* motion, const ScreenState* screen, BeamState* beam) {
    // Dert: Distance = Rate * time

    // Calculate movement in each dimention
    int32_t next_x = motion->mx1 + motion->dx * elapsed;
    int32_t next_y = motion->my1 + motion->dy * elapsed;
    beam->a = 1;
    if (line_completed(motion->mx2, next_x, (motion->dx > 0))
        || line_completed(motion->my2, next_y, (motion->dy > 0))
    ) {
        // Motion is complete
        next_x = motion->mx2;
        next_y = motion->my2;
        beam->a = 0;
    }
#ifdef AVR
    beam->x = next_x >> 10;
    beam->y = next_y >> 10;
#else
    beam->x = next_x / 1000;
    beam->y = next_y / 1000;
#endif
    return (beam->a > 0);
}

static inline bool nextBeamState(uint16_t elapsed, const ScreenMotion* motion, ScreenState* screen) {
    BeamState beam;
    bool active;
    switch (motion->type) {
    case SM_Point:
        active = calcPoint(elapsed, (PointMotion*)motion, screen, &beam);
        break;
    case SM_Line:
        active = calcLine(elapsed, (LineMotion*)motion, screen, &beam);
        break;
    default:
        active = false;
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
    screen->speed     = 10;  // millipoint / microsecond
    screen->hold_time = 1;  // 1 ms
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

LineMotion* screen_push_line(RingMemPool* pool, const LineCmd* cmd, uint16_t speed) {
    // Allocate object from the pool
    speed = max(2, speed);
    LineMotion* motion = ring_get(pool, sizeof(LineMotion));
    if (!motion) {
        return NULL;
    }
    // Calculate length in millipoints
    float length = sqrtf(powf(cmd->x2 - cmd->x1, 2) + powf(cmd->y2 - cmd->y1, 2)) * 1000;

    // Populate motion
    motion->base.type = SM_Line;
#ifndef AVR
    motion->x1 = cmd->x1;
    motion->y1 = cmd->y1;
    motion->x2 = cmd->x2;
    motion->y2 = cmd->y2;
#endif
    // Multiply by 1000 to get millipoints
    motion->mx1 = 1000l*cmd->x1;
    motion->my1 = 1000l*cmd->y1;
    motion->mx2 = 1000l*cmd->x2;
    motion->my2 = 1000l*cmd->y2;
    motion->dx  = (motion->mx2 - motion->mx1) * speed / length;
    motion->dy  = (motion->my2 - motion->my1) * speed / length;

    return motion;
}

bool update_screen(uint32_t time, ScreenState* screen, RingMemPool* pool) {
    int16_t elapsed = time - screen->motion_start;

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
