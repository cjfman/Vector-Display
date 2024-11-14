#ifndef SCREEN_CONTROLLER_HH
#define SCREEN_CONTROLLER_HH

#include "command_parser.h"

typedef struct ScreenState {
	int x_scale;
	int y_scale;
	int x_offset;
	int y_offset;
} ScreenState;

typedef struct BeamState {
	int x;
	int y;
	int a; // Active
} BeamState;

int nextScreenState(const int elapsed_time, const Command* cmd, const ScreenState* screen, BeamState* beam);



#endif // SCREEN_CONTROLLER_HH
