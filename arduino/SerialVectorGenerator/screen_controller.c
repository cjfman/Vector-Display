#include <math.h>
#include <util/atomic.h>

#include "ring_mem_pool.h"
#include "screen_controller.h"

#define SCREEN_MAX_VALUE 128
#define SCREEN_MIN_VALUE 0
#define SCREEN_WIDTH (SCREEN_MAX_VALUE - SCREEN_MIN_VALUE)

ScreenState main_screen = {0};

static inline int abs_int(int val) {
	return (val < 0) ? val * -1 : val;
}

// Check bounds of the position and max it out if needed
static inline int get_abs_pos(int val, int scale, int offset) {
	val += offset;
	return (abs_int(val) < scale) ?   val : // Value is within bounds
		   (val > 0)              ? scale : // Value is too large
		                                0 ; // Value is too small

}

static void calcPointHelper(int x, int y, const ScreenState* screen, BeamState* beam) {
	// Scale and offset position
	beam->x = SCREEN_WIDTH * get_abs_pos(x, screen->x_scale, screen->x_offset) / screen->x_scale - SCREEN_MIN_VALUE;
	beam->y = SCREEN_WIDTH * get_abs_pos(y, screen->y_scale, screen->y_offset) / screen->y_scale - SCREEN_MIN_VALUE;
	beam->a = 1;
}

static void calcPoint(const PointMotion* motion, const ScreenState* screen, BeamState* beam) {
	calcPointHelper(motion->x, motion->y, screen, beam);
}

void calcLine(int elapsed, const LineMotion* motion, const ScreenState* screen, BeamState* beam) {
	// Dert: Distance = Rate * time
	// Rate = screen->speed / SCREEN_WIDTH
	// Completed = distance / length
	long completed_nom   = (long)screen->speed * elapsed;
	long completed_denom = (long)SCREEN_WIDTH  * motion->length;
	int x = (motion->x2 - motion->x1) * completed_nom / completed_denom;
	int y = (motion->y2 - motion->y1) * completed_nom / completed_denom;
	calcPointHelper(x, y, screen, beam);
}

int nextBeamState(int elapsed, const ScreenMotion* motion, const ScreenState* screen, BeamState* beam) {
	switch (motion->type) {
	case SM_Point:
		calcPoint(motion, screen, beam);
		break;
	case SM_Line:
		calcLine(elapsed, motion, screen, beam);
		break;
	default:
		beam->x = 0;
		beam->y = 0;
		beam->a = 0;
	}
	return 1;
}

void screen_init(void) {
	memset(&main_screen, '\0', sizeof(main_screen));
	main_screen.x_scale = 100;
	main_screen.y_scale = 100;
	main_screen.speed   = 100000; // 100 ms
}

int screen_push_point(RingMemPool* pool, const PointCmd* cmd) {
	int success;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Allocate object from the pool
		PointMotion* motion = ring_get(pool, sizeof(PointMotion));
		if (!motion) {
			success = 0;
		}
		else {
			// Populate motion
			motion->x = cmd->x;
			motion->y = cmd->y;
			success = 1;
		}
	}
	return success;
}

int screen_push_line(RingMemPool* pool, const LineCmd* cmd) {
	int success;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Allocate object from the pool
		LineMotion* motion = ring_get(pool, sizeof(LineMotion));
		if (!motion) {
			success = 0;
		}
		else {
			// Populate motion
			motion->x1 = cmd->x1;
			motion->y1 = cmd->y1;
			motion->x2 = cmd->x2;
			motion->y2 = cmd->y2;

			// Calculate length
			motion->length = sqrt(pow(cmd->x2 - cmd->x1, 2) + pow(cmd->y2 - cmd->y1, 2));
		}
	}
	return success;
}

void screen_set_scale(const ScaleCmd* cmd) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		main_screen.x_scale = cmd->x_scale;
		main_screen.y_scale = cmd->y_scale;
	}
}

void update_screen(long time, RingMemPool* pool) {
	long elapsed = time - main_screen.last_update;
	main_screen.last_update = time;

	// Get motion
	ScreenMotion* motion = ring_peek(pool);
	if (!motion) {
		return;
	}

	// Determine new beam position
	BeamState state;
	nextBeamState(elapsed, motion, &main_screen, &state);
	ring_pop(pool);

	// TODO Update screen hardware
}
