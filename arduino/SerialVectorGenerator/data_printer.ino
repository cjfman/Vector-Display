#include <string.h>

#include "command_parser.h"
#include "screen_controller.h"

static inline String printScaleCmd(const ScaleCmd* cmd) {
    Serial.print("scale");
    Serial.print(" x_width: ");
    Serial.print(cmd->x_width);
    Serial.print(" y_width: ");
    Serial.print(cmd->y_width);
    Serial.print(" x_centered: ");
    Serial.print(cmd->x_centered);
    Serial.print(" y_centered: ");
    Serial.print(cmd->y_centered);
}

static inline void printPointCmd(const PointCmd* cmd) {
    Serial.print("point x: ");
    Serial.print(cmd->x);
    Serial.print(" y: ");
    Serial.print(cmd->y);
}

static inline void printLineCmd(const LineCmd* cmd) {
    Serial.print("line");
    Serial.print(" x1: ");
    Serial.print(cmd->x1);
    Serial.print(" y1: ");
    Serial.print(cmd->y1);
    Serial.print(" x2: ");
    Serial.print(cmd->x2);
    Serial.print(" y2: ");
    Serial.print(cmd->y2);
}

static inline void printSpeedCmd(const SpeedCmd* cmd) {
    Serial.print("speed");
    Serial.print(" holdtime: ");
    Serial.print(cmd->hold_time);
    Serial.print(" speed: ");
    Serial.print(cmd->speed);
}

static inline void printSequenceCmd(const SequenceCmd* cmd) {
    Serial.print("sequence ");
    Serial.print(cmd->base.args[0]);
}

static inline void printSetCmd(const SetCmd* cmd) {
    Serial.print((cmd->set) ? "set " : "unset ");
    Serial.print(cmd->base.args[0]);
}

void printCommand(const Command* cmd) {
    switch (cmd->type) {
    case Cmd_Scale:
        printScaleCmd((const ScaleCmd*) cmd);
        break;
    case Cmd_Point:
        printPointCmd((const PointCmd*) cmd);
        break;
    case Cmd_Line:
        printLineCmd((const LineCmd*) cmd);
        break;
    case Cmd_Speed:
    case Cmd_Hold:
        printSpeedCmd((const SpeedCmd*) cmd);
        break;
    case Cmd_Sequence:
        printSequenceCmd((const SequenceCmd*) cmd);
        break;
    case Cmd_Set:
    case Cmd_Unset:
        printSetCmd((const SetCmd*) cmd);
        break;
    case Cmd_Noop:
        Serial.print("noop");
        break;
    default:
        Serial.print("Unknown command motion type ");
        Serial.print(cmd->type);
        break;
    }
}

static inline void printPointMotion(const PointMotion* motion) {
    Serial.print("PointMotion x: ");
    Serial.print(motion->x);
    Serial.print(" y: ");
    Serial.print(motion->y);
}

static inline String printLineMotion(const LineMotion* motion) {
    Serial.write("LineMotion ");
    Serial.print(" x1: ");
    Serial.print(motion->mx1 / 1000);
    Serial.print(" y1: ");
    Serial.print(motion->my1 / 1000);
    Serial.print(" x2: ");
    Serial.print(motion->mx2 / 1000);
    Serial.print(" y2: ");
    Serial.print(motion->my2 / 1000);
    Serial.print(" dx: ");
    Serial.print(motion->dx);
    Serial.print(" dy: ");
    Serial.print(motion->dy);
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
        Serial.write((String("Unknown screen motion type ") + motion->type).c_str());
        break;
    }
}

void printBeamState(const BeamState* state) {
    Serial.write((String("BeamState ")
        + " x: "      + state->x
        + " y: "      + state->y
        + " active: " + state->a
    ).c_str());
}
