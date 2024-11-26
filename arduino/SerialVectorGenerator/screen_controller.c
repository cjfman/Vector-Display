#include <math.h>
#include <string.h>

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

//#define SCREEN_MAX_VALUE 256l
//#define SCREEN_MIN_VALUE 0l
//#define SCREEN_WIDTH (SCREEN_MAX_VALUE - SCREEN_MIN_VALUE)
//
//static inline int abs_int(int val) {
//	return (val < 0) ? val * -1 : val;
//}
//
//// Check bounds of the position and max it out if needed
//static inline int get_abs_pos(int val, int scale, int offset) {
//	val += offset;
//	return (abs_int(val) < scale) ?   val : // Value is within bounds
//		   (val > 0)              ? scale : // Value is too large
//		                                0 ; // Value is too small
//
//}
//
//static void calcPointHelper(int x, int y, const ScreenState* screen, BeamState* beam) {
//	// Scale and offset position
//	beam->x = SCREEN_WIDTH * get_abs_pos(x, screen->x_width, screen->x_offset) / screen->x_width - SCREEN_MIN_VALUE;
//	beam->y = SCREEN_WIDTH * get_abs_pos(y, screen->y_width, screen->y_offset) / screen->y_width - SCREEN_MIN_VALUE;
//	beam->a = 1;
//}

static void calcPoint(const PointMotion* motion, BeamState* beam) {
	beam->x = motion->x;
	beam->y = motion->y;
	beam->a = 1;
}

void calcLine(int elapsed, const LineMotion* motion, const ScreenState* screen, BeamState* beam) {
	// Dert: Distance = Rate * time
	// Rate = screen->speed / SCREEN_WIDTH
	// Completed = distance / length
	// TODO Should these be floats?
	long completed_nom   = screen->speed * elapsed;
	long completed_denom = screen->x_width * motion->length;
	beam->x = (motion->x2 - motion->x1) * completed_nom / completed_denom;
	beam->y = (motion->y2 - motion->y1) * completed_nom / completed_denom;
	beam->a = 1;
}

int nextBeamState(int elapsed, const ScreenMotion* motion, const ScreenState* screen, BeamState* beam) {
	switch (motion->type) {
	case SM_Point:
		calcPoint((PointMotion*)motion, beam);
		break;
	case SM_Line:
		calcLine(elapsed, (LineMotion*)motion, screen, beam);
		break;
	default:
		beam->x = 0;
		beam->y = 0;
		beam->a = 0;
	}

	// Bounds check
	beam->x = max(min(beam->x, screen->x_width - screen->x_offset), -screen->x_offset);
	beam->y = max(min(beam->y, screen->y_width - screen->y_offset), -screen->y_offset);
	
	return 1;
}

void screen_init(ScreenState* screen) {
	memset(screen, '\0', sizeof(ScreenState));
	screen->x_width = 100;
	screen->y_width = 100;
	screen->speed   = 100000; // 100 ms
}

int screen_push_point(RingMemPool* pool, const PointCmd* cmd) {
	int success;
	// Allocate object from the pool
	PointMotion* motion = ring_get(pool, sizeof(PointMotion));
	if (!motion) {
		success = 0;
	}
	else {
		// Populate motion
		motion->base.type = SM_Point;
		motion->x = cmd->x;
		motion->y = cmd->y;
		success = 1;
	}
	return success;
}

int screen_push_line(RingMemPool* pool, const LineCmd* cmd) {
	int success;
	// Allocate object from the pool
	LineMotion* motion = ring_get(pool, sizeof(LineMotion));
	if (!motion) {
		success = 0;
	}
	else {
		// Populate motion
		motion->base.type = SM_Line;
		motion->x1 = cmd->x1;
		motion->y1 = cmd->y1;
		motion->x2 = cmd->x2;
		motion->y2 = cmd->y2;

		// Calculate length
		motion->length = sqrt(pow(cmd->x2 - cmd->x1, 2) + pow(cmd->y2 - cmd->y1, 2));
		success = 1;
	}
	return success;
}

void screen_set_scale(ScreenState* screen, const ScaleCmd* cmd) {
	screen->x_width = cmd->x_width;
	screen->y_width = cmd->y_width;
}

void update_screen(long time, ScreenState* screen, RingMemPool* pool) {
	long elapsed = time - screen->last_update;
	screen->last_update = time;

	// Get motion
	ScreenMotion* motion = ring_peek(pool);
	if (!motion) {
		return;
	}

	// Determine new beam position
	BeamState beam;
	nextBeamState(elapsed, motion, screen, &beam);
	screen->beam = beam;
	ring_pop(pool);

	// TODO Update screen hardware
}
