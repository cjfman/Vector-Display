#ifndef SCREEN_CONTROLLER_HH
#define SCREEN_CONTROLLER_HH

#include "command_parser.h"

typedef struct ScreenState {
	int x_scale;
	int y_scale;
	int x_offset;
	int y_offset;
	unsigned long speed; 	   // Time cross the entire screen
	unsigned long last_update; // Time of last update
} ScreenState;

typedef struct BeamState {
	int x;
	int y;
	int a; // Active
} BeamState;

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
	int length;
} LineMotion;

int nextBeamState(const int elapsed, const ScreenMotion* cmd, const ScreenState* screen, BeamState* beam);
void screen_init(void);
int screen_push_point(const PointCmd* cmd);
int screen_push_line(const LineCmd* cmd);
void screen_set_scale(const ScaleCmd* cmd);
void update_screen(long time);

#endif // SCREEN_CONTROLLER_HH
