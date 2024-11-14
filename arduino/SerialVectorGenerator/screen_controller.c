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

static void calcPointHelper(int x, int y, const ScreenState* screen, BeamState* beam) {
	// Scale and offset position
	long diff = SCREEN_MAX_VALUE - SCREEN_MIN_VALUE;
	beam->x = diff * get_abs_pos(x, screen->x_scale, screen->x_offset) / screen->x_scale - SCREEN_MIN_VALUE;
	beam->y = diff * get_abs_pos(y, screen->y_scale, screen->y_offset) / screen->y_scale - SCREEN_MIN_VALUE;
	beam->a = 1;
}

static void calcPoint(const PointCmd* cmd, const ScreenState* screen, BeamState* beam) {
	calcPointHelper(cmd->x, cmd->y, screen, beam);
}

void calcLine(int elapsed_time_ms, const LineCmd* cmd, const ScreenState* screen, BeamState* beam) {
	float completed = (float)elapsed_time_ms / cmd->ms;
	completed = (completed > 1) ? completed : 1;
	int x = (cmd->x2 - cmd->x1) * completed + cmd->x1;
	int y = (cmd->y2 - cmd->y1) * completed + cmd->y1;
	calcPointHelper(x, y, screen, beam);
}

int nextScreenState(int elapsed_time_ms, const Command* cmd, const ScreenState* screen, BeamState* beam) {
	switch (cmd->type) {
	case Cmd_Point:
		calcPoint(cmd, screen, beam);
		break;
	case Cmd_Line:
		calcLine(elapsed_time_ms, cmd, screen, beam);
		break;
	default:
		beam->x = 0;
		beam->y = 0;
		beam->a = 0;
	}
	return 1;
}
