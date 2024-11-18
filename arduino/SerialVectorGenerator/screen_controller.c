#include <Math.h>

#include "screen_controller.h"

#define SCREEN_MAX_VALUE 128
#define SCREEN_MIN_VALUE 0
#define SCREEN_WIDTH (SCREEN_MAX_VALUE - SCREEN_MIN_VALUE)

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

void calcLine(int elapsed_time_ms, const LineMotion* motion, const ScreenState* screen, BeamState* beam) {
	// Dert: Distance = Rate * time
	// Rate = screen->speed / SCREEN_WIDTH
	long completed_nom   = (long)screen->speed * elapsed_time_ms;
	long completed_denom = (long)SCREEN_WIDTH  * motion->length;
	int x = (motion->x2 - motion->x1) * completed_nom / completed_denom;
	int y = (motion->y2 - motion->y1) * completed_nom / completed_denom;
	calcPointHelper(x, y, screen, beam);
}

int nextScreenState(int elapsed_time_ms, const ScreenMotion* motion, const ScreenState* screen, BeamState* beam) {
	switch (motion->type) {
	case SM_Point:
		calcPoint(motion, screen, beam);
		break;
	case SM_Line:
		calcLine(elapsed_time_ms, motion, screen, beam);
		break;
	default:
		beam->x = 0;
		beam->y = 0;
		beam->a = 0;
	}
	return 1;
}
