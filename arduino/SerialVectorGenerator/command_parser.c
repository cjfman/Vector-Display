#include <ctype.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "command_parser.h"
#include "ring_mem_pool.h"

// cmd prefixes
const char* cmd_set[Cmd_NUM] = {
    [Cmd_Scale]    = "scale",
    [Cmd_Point]    = "point",
    [Cmd_Line]     = "line",
    [Cmd_Speed]    = "speed",
    [Cmd_Hold]     = "hold",
    [Cmd_Sequence] = "sequence",
    [Cmd_Set]      = "set",
    [Cmd_Unset]    = "unset",
    [Cmd_Noop]     = "noop",
};

// Ring buffer
static char cmd_buf[CMD_BUF_SIZE + 1];
static uint8_t cmd_buf_len = 0;

void clearCache(void) {
    cmd_buf_len = 0;
}

// And a command to the buffer
err_t buildCmd(const char* new_cmd, uint8_t len) {
    // Check size
    if (len + cmd_buf_len > CMD_BUF_SIZE - 1) {
        // Buffer overrun
        // Reset buffer
        clearCache();
        return CMD_ERR_CMD_TOO_LONG;
    }
//    memcpy(&cmd_buf[cmd_buf_len], new_cmd, len);
//    cmd_buf_len += len;
    for (uint8_t i = 0; i < len; i++) {
        char c = new_cmd[i];
        if (isprint(c) || c == '\r' || c == '\n') {
            cmd_buf[cmd_buf_len++] = c;
        }
        else if (c == '\b' && cmd_buf_len > 0) {
            // Backspace
            cmd_buf_len--;
        }
    }
    return CMD_OK;
}

static int16_t lookForChar(char c) {
    int i;
    for (i = 0; i < cmd_buf_len; i++) {
        if (cmd_buf[i] == c) return i;
    }
    return -1;
}

// Shift the buffer pointer
static void shiftBuf(uint8_t len) {
    // Don't do anything if the shift amount is too much
    if (len > cmd_buf_len) return;

    uint8_t i;
    for (i = 0; i < len; i++) {
        cmd_buf[i] = cmd_buf[i + len];
    }
    cmd_buf_len -= len;

}

// Trim line ends
static uint8_t trimCrlf(void) {
    if (cmd_buf_len == 0) return 0;

    char c;
    uint8_t shift_len = 0;
    // Check first byte
    if (cmd_buf_len >= 1) {
         c = cmd_buf[0];
        if (c == '\r' || c == '\n') shift_len++;
    }
    // Check second byte
    if (cmd_buf_len >= 2) {
         c = cmd_buf[1];
        if (c == '\r' || c == '\n') shift_len++;
    }

    if (shift_len) shiftBuf(shift_len);

    return shift_len;
}

// Find the end of the next command
static int16_t crlfPos(void) {
    if (!cmd_buf_len) return 0;

    int16_t cr_pos = lookForChar('\r');
    int16_t lf_pos = lookForChar('\n');

    // Must be at least one of them for a complete command
    if (cr_pos == -1 && lf_pos == -1) return -1;

    // There is a \r but not an \n
    if (cr_pos != -1 && lf_pos == -1) return cr_pos;

    // There is an \n but not an \r
    if (cr_pos == 1 && lf_pos != -1) return lf_pos;

    // Return the first one
    return (cr_pos < lf_pos) ? cr_pos : lf_pos;
}

// The size of the command
uint8_t commandSize(void) {
    int16_t pos = crlfPos();
    return (pos != -1) ? pos : 0;
}

uint8_t noopCommand(void) {
    if (crlfPos() == -1) return 0;

    return trimCrlf();
}

bool commandComplete(void) {
    return (cmd_buf_len > 0 && crlfPos() != -1);
}

uint8_t cmdBufLen(void) {
    return cmd_buf_len;
}

// Get a command string from the command cache
// and copy it into the buffer
err_t getCmd(char* buf, uint8_t buf_len) {
    // Check for a complete command
    if (crlfPos() == -1) return 0;

    // Get the command string
    int cmd_len = commandSize();
    if (cmd_len == 0) {
        // This was a noop command
        trimCrlf();
        return CMD_ERR_CMD_NOOP;
    }

    // Buffer size safety check
    if (buf_len < cmd_len) {
        return CMD_ERR_BUF_OVERRUN;
    }

    // Copy command
    memcpy(buf, cmd_buf, cmd_len);
    shiftBuf(cmd_len);
    trimCrlf();

    return 0;
}

// Command decoder functer type
typedef err_t (*DecodeFn)(Command *);

// Decode a scale command
static err_t cmdDecodeScale(ScaleCmd* cmd) {
    const Command* base = &cmd->base;
    if (base->numargs != 4) return CMD_ERR_WRONG_NUM_ARGS;
    cmd->x_width    = atoi(base->args[0]);
    cmd->y_width    = atoi(base->args[1]);
    cmd->x_centered = !!atoi(base->args[2]);
    cmd->y_centered = !!atoi(base->args[3]);
    return CMD_OK;
}

// Decode a point command
static err_t cmdDecodePoint(PointCmd* cmd) {
    const Command* base = &cmd->base;
    if (base->numargs != 2) return CMD_ERR_WRONG_NUM_ARGS;
    cmd->x = atoi(base->args[0]);
    cmd->y = atoi(base->args[1]);
    return CMD_OK;
}

// Decode a line command
static err_t cmdDecodeLine(LineCmd* cmd) {
    const Command* base = &cmd->base;
    if (base->numargs != 4) return CMD_ERR_WRONG_NUM_ARGS;
    cmd->x1 = atoi(base->args[0]);
    cmd->y1 = atoi(base->args[1]);
    cmd->x2 = atoi(base->args[2]);
    cmd->y2 = atoi(base->args[3]);
    return CMD_OK;
}

// Decode a speed command
static err_t cmdDecodeSpeed(SpeedCmd* cmd) {
    const Command* base = &cmd->base;
    if (base->numargs != 1) return CMD_ERR_WRONG_NUM_ARGS;
    cmd->speed = atof(base->args[0]);
    return CMD_OK;
}

// Decode a hold command
static err_t cmdDecodeHold(SpeedCmd* cmd) {
    const Command* base = &cmd->base;
    if (base->numargs != 1) return CMD_ERR_WRONG_NUM_ARGS;
    cmd->hold_time = atoi(base->args[0]);
    return CMD_OK;
}

// Decode a sequence command
static err_t cmdDecodeSequence(SequenceCmd* cmd) {
    const Command* base = &cmd->base;
    if (base->numargs != 1) return CMD_ERR_WRONG_NUM_ARGS;
    if (strcmp(base->args[0], "start") == 0) {
        cmd->start = true;
    }
    else if (strcmp(base->args[0], "end") == 0) {
        cmd->end = true;
    }
    else if (strcmp(base->args[0], "clear") == 0) {
        cmd->clear = true;
    }
    else {
        return CMD_ERR_BAD_ARG;
    }
    return CMD_OK;
}

// Decode Set/Unset command
static err_t cmdDecodeSet(SetCmd* cmd) {
    const Command* base = &cmd->base;
    if (base->numargs != 1) return CMD_ERR_WRONG_NUM_ARGS;
    cmd->name = base->args[0];
    return CMD_OK;
}

// Parse a command line
err_t cmdParse(CommandUnion* cmd, char* buf, uint8_t len) {
    // Command arguments are space separated
    memset(cmd, '\0', sizeof(CommandUnion));
    uint8_t count = 0;
    char* cmd_start = buf;
    uint8_t i;
    for (i = 0; i < len; i++) {
        if (buf[i] == ' ') {
            // Trim off leading spaces
            if (cmd_start[0] == ' ') {
                cmd_start += 1;
                continue;
            }

            // Safety check
            if (count == CMD_MAX_NUM_ARGS) return CMD_ERR_TOO_MANY_ARGS;

            // Reached the end of an argument
            buf[i] = '\0'; // Replace with null char
            // Pointer to argument
            cmd->base.args[count++] = &buf[++i];
        }
        if (buf[i] == '\0') break;
    }

    buf[i] = '\0'; // Mark end of last arg
    cmd->base.buf     = cmd_start;
    cmd->base.numargs = count;

    // Get cmd type
    err_t (*decode_fn)(Command* cmd) = NULL;
    if (strcmp(cmd_set[Cmd_Scale], cmd_start) == 0) {
        cmd->base.type = Cmd_Scale;
        decode_fn = (DecodeFn)cmdDecodeScale;
    }
    else if (strcmp(cmd_set[Cmd_Point], cmd_start) == 0) {
        cmd->base.type = Cmd_Point;
        decode_fn = (DecodeFn)cmdDecodePoint;
    }
    else if (strcmp(cmd_set[Cmd_Line], cmd_start) == 0) {
        cmd->base.type = Cmd_Line;
        decode_fn = (DecodeFn)cmdDecodeLine;
    }
    else if (strcmp(cmd_set[Cmd_Speed], cmd_start) == 0) {
        cmd->base.type = Cmd_Speed;
        cmd->speed.hold_time = 0;
        decode_fn = (DecodeFn)cmdDecodeSpeed;
    }
    else if (strcmp(cmd_set[Cmd_Hold], cmd_start) == 0) {
        cmd->base.type = Cmd_Speed; // This isn't a mistake
        cmd->speed.speed = 0;
        decode_fn = (DecodeFn)cmdDecodeHold;
    }
    else if (strcmp(cmd_set[Cmd_Sequence], cmd_start) == 0) {
        cmd->base.type = Cmd_Sequence;
        decode_fn = (DecodeFn)cmdDecodeSequence;
    }
    else if (strcmp(cmd_set[Cmd_Set], cmd_start) == 0) {
        cmd->base.type = Cmd_Set;
        cmd->set.set = true;
        decode_fn = (DecodeFn)cmdDecodeSet;
    }
    else if (strcmp(cmd_set[Cmd_Unset], cmd_start) == 0) {
        cmd->base.type = Cmd_Unset;
        cmd->set.set = false;
        decode_fn = (DecodeFn)cmdDecodeSet;
    }
    else if (strcmp(cmd_set[Cmd_Noop], cmd_start) == 0) {
        cmd->base.type = Cmd_Noop;
    }
    else {
        return CMD_ERR_BAD_CMD;
    }

    // Process command
    if (decode_fn) {
        return decode_fn((Command*)cmd);
    }

    return CMD_OK;
}

const char* cmdErrToText(err_t errcode) {
    switch (errcode) {
    case CMD_OK:
        return "No error";
    case CMD_ERROR_OTHER:
        return "Other command error";
    case CMD_ERR_BUF_OVERRUN:
        return "Buffer overrun";
    case CMD_ERR_BAD_CMD:
        return "Unknown command";
    case CMD_ERR_CMD_TOO_LONG:
        return "Command too long";
    case CMD_ERR_CMD_NOOP:
        return "Noop command not handled";
    case CMD_MAX_NUM_ARGS:
        return "Too marny arguments";
    case CMD_ERR_WRONG_NUM_ARGS:
        return "Wrong number of arguments";
    case CMD_ERR_PARSE:
        return "Parse error";
    case CMD_ERR_BAD_ARG:
        return "Bad argument";
    default:
        return "Unknown command error";
    }
}
