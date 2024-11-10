#include "screen_controller.h"

#define SCREEN_MAX_VALUE 128
#define SCREEN_MIN_VALUE 0

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

static calcPoint(const PointCmd* cmd, const ScreenState* screen, BeamState* beam) {
	// Scale and offset position
	long diff = SCREEN_MAX_VALUE - SCREEN_MIN_VALUE;
	beam->x = diff * get_abs_pos(cmd->x, screen->x_scale, screen->x_offset) / screen->x_scale - SCREEN_MIN_VALUE;
	beam->y = diff * get_abs_pos(cmd->y, screen->y_scale, screen->y_offset) / screen->y_scale - SCREEN_MIN_VALUE;
	beam->a = 1;
}

void nextScreenState(const int elapsed_time, const Command* cmd, const ScreenState* screen, BeamState* beam) {
	switch (cmd->type) {
	case Cmd_Point:
		calcPoint(cmd, screen, beam);
	default:
		beam->x = 0;
		beam->y = 0;
		beam->a = 0;
	}
}
