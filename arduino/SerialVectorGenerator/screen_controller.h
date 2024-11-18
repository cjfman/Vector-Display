#ifndef SCREEN_CONTROLLER_HH
#define SCREEN_CONTROLLER_HH

#include "command_parser.h"

typedef struct ScreenState {
	int x_scale;
	int y_scale;
	int x_offset;
	int y_offset;
	long speed; // us to cross the entire screen
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


int nextScreenState(const int elapsed_time, const ScreenMotion* cmd, const ScreenState* screen, BeamState* beam);


#endif // SCREEN_CONTROLLER_HH
