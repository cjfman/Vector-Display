#include <string.h>

#include "command_parser.h"
#include "screen_controller.h"

static inline String printScaleCmd(const ScaleCmd* cmd) {
	return String("scale x_width: ") + cmd->x_width
		+ " y_width: "  + cmd->y_width
		+ " x_offset: " + cmd->x_offset
		+ " y_offset: " + cmd->y_offset;
}

static inline String printPointCmd(const PointCmd* cmd) {
	return String("point x: ") + cmd->x + " y: " + cmd->y;
}

static inline String printLineCmd(const LineCmd* cmd) {
	return String("line")
        + " x1: " + cmd->x1
		+ " y1: " + cmd->y1
		+ " x2: " + cmd->x2
		+ " y2: " + cmd->y2;
}

String commandToString(const Command* cmd) {
	switch (cmd->type) {
	case Cmd_Scale:
		return printScaleCmd((const ScaleCmd*) cmd);
	case Cmd_Point:
		return printPointCmd((const PointCmd*) cmd);
	case Cmd_Line:
		return printLineCmd((const LineCmd*) cmd);
	}
	return String("Unknown command motion type ") + cmd->type;
}

static inline String printPointMotion(const PointMotion* motion) {
    return String("PointMotion x: ") + motion->x + " y: " + motion->y;
}

static inline String printLineMotion(const LineMotion* motion) {
	return String("LineMotion ")
        + " x1: "     + motion->x1
		+ " y1: "     + motion->y1
		+ " x2: "     + motion->x2
		+ " y2: "     + motion->y2;
		+ " length: " + (long)motion->length;
}

String motionToString(const ScreenMotion* motion) {
    switch (motion->type) {
    case SM_Point:
        return printPointMotion((const PointMotion*) motion);
    case SM_Line:
        return printLineMotion((const LineMotion*) motion);
    }
	return String("Unknown screen motion type ") + motion->type;
}
