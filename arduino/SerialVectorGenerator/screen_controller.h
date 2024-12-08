#ifndef SCREEN_CONTROLLER_HH
#define SCREEN_CONTROLLER_HH

#include <stdbool.h>

#include "command_parser.h"

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
	int x1;
	int y1;
	int x2;
	int y2;
	float length; // Should this be a float?
} LineMotion;

typedef struct ScreenState {
	int x_width;
	int y_width;
	int x_offset;
	int y_offset;
	int hold_time;     // Time to hold a point
	float speed;       // Points moved in a microsecond
	long motion_start; // Time when current motion started
	BeamState beam;
	int motion_active;
} ScreenState;

bool nextBeamState(const int elapsed, const ScreenMotion* cmd, ScreenState* screen);
void screen_init(ScreenState* screen);
PointMotion* screen_push_point(RingMemPool* pool, const PointCmd* cmd);
LineMotion* screen_push_line(RingMemPool* pool, const LineCmd* cmd);
bool update_screen(long time, ScreenState* screen, RingMemPool* pool);
uint16_t position_to_binary(int pos, int scale, unsigned bits, bool dipole);

#endif // SCREEN_CONTROLLER_HH
