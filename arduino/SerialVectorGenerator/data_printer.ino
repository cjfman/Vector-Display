#include <string.h>

#include "command_parser.h"
#include "screen_controller.h"

static inline String printScaleCmd(const ScaleCmd* cmd) {
    return String("scale x_width: ") + cmd->x_width
        + " y_width: "  + cmd->y_width
        + " x_centered: " + cmd->x_centered
        + " y_centered: " + cmd->y_centered;
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

static inline String printSpeedCmd(const SpeedCmd* cmd) {
    return String("speed")
        + " holdtime: " + cmd->hold_time
        + " speed: " + cmd->speed;
}

static inline String printSequenceCmd(const SequenceCmd* cmd) {
    return String("sequence ") + cmd->base.args[0];
}

static inline String printSetCmd(const SetCmd* cmd) {
    if (cmd->set) {
        return String("set ") + cmd->base.args[0];
    }
    else {
        return String("unset ") + cmd->base.args[0];
    }
}

String commandToString(const Command* cmd) {
    switch (cmd->type) {
    case Cmd_Scale:
        return printScaleCmd((const ScaleCmd*) cmd);
    case Cmd_Point:
        return printPointCmd((const PointCmd*) cmd);
    case Cmd_Line:
        return printLineCmd((const LineCmd*) cmd);
    case Cmd_Speed:
    case Cmd_Hold:
        return printSpeedCmd((const SpeedCmd*) cmd);
    case Cmd_Sequence:
        return printSequenceCmd((const SequenceCmd*) cmd);
    case Cmd_Set:
    case Cmd_Unset:
        return printSetCmd((const SetCmd*) cmd);
    case Cmd_Noop:
        return "noop";
    default:
        return String("Unknown command motion type ") + cmd->type;
    }
}

static inline void printPointMotion(const PointMotion* motion) {
    Serial.write((String("PointMotion x: ") + motion->x + " y: " + motion->y).c_str());
}

static inline String printLineMotion(const LineMotion* motion) {
    Serial.write("LineMotion ");
    Serial.write((String(" x1: ") + motion->mx1 / 1000).c_str());
    Serial.write((String(" y1: ") + motion->my1 / 1000).c_str());
    Serial.write((String(" x2: ") + motion->mx2 / 1000).c_str());
    Serial.write((String(" y2: ") + motion->my2 / 1000).c_str());
    Serial.write((String(" dx: ") + motion->dx).c_str());
    Serial.write((String(" dy: ") + motion->dy).c_str());
}

void serialPrintMotion(const ScreenMotion* motion) {
    switch (motion->type) {
    case SM_Point:
        printPointMotion((const PointMotion*) motion);
        break;
    case SM_Line:
        printLineMotion((const LineMotion*) motion);
        break;
    default:
        Serial.print((String("Unknown screen motion type ") + motion->type).c_str());
        break;
    }
}

String beamStateToString(const BeamState* state) {
    return String("BeamState ")
        + " x: " + state->x
        + " y: " + state->y
        + " active: " + state->a;
}
