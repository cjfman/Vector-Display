#ifndef SCREEN_CONTROLLER_HH
#define SCREEN_CONTROLLER_HH

#include "command_parser.h"

typedef struct BeamState {
	int x;
	int y;
	int a; // Active
} BeamState;

typedef struct ScreenState {
	int x_width;
	int y_width;
	int x_offset;
	int y_offset;
	unsigned long speed; 	   // Time to cross the width of the screen in us
	unsigned long last_update; // Time of last update
	BeamState beam;
} ScreenState;

typedef enum ScreenMotionType {
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
	int length; // Should this be a float?
} LineMotion;

int nextBeamState(const int elapsed, const ScreenMotion* cmd, const ScreenState* screen, BeamState* beam);
void screen_init(ScreenState* screen);
int screen_push_point(RingMemPool* pool, const PointCmd* cmd);
int screen_push_line(RingMemPool* pool, const LineCmd* cmd);
void screen_set_scale(ScreenState* screen, const ScaleCmd* cmd);
void update_screen(long time, ScreenState* screen, RingMemPool* pool);

#endif // SCREEN_CONTROLLER_HH
