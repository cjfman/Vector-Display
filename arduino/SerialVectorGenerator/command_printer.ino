#include <String.h>

#include "command_parser.h"

static inline String printScale(const ScaleCmd* cmd) {
	return String("scale x_width: ") + cmd->x_width
		+ " y_width: "  + cmd->y_width
		+ " x_offset: " + cmd->x_offset
		+ " y_offset: " + cmd->y_offset;
}

static inline String printPoint(const PointCmd* cmd) {
	return String("point x: ") + cmd->x + " y: " + cmd->y;
}

static inline String printLine(const LineCmd* cmd) {
	return String("line")
    + " x1: " + cmd->x1
		+ " y1: " + cmd->y1
		+ " x2: " + cmd->x2
		+ " y2: " + cmd->y2;
}

String commandToString(const Command* cmd) {
	switch (cmd->type) {
	case Cmd_Scale:
		return printScale((const ScaleCmd*) cmd);
	case Cmd_Point:
		return printPoint((const PointCmd*) cmd);
	case Cmd_Line:
		return printLine((const LineCmd*) cmd);
	default:
		break;
	}
	return String();
}
