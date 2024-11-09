#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "command_parser.h"
#include "ring_mem_pool.h"

// cmd prefixes
const char* cmd_set[Cmd_Num] = {
	"scale",
	"point",
	"line",
	"noop",
};

// Ring buffer
char cmd_buf[CMD_BUF_SIZE];
int cmd_buf_len = 0;

void clearCache(void) {
    cmd_buf_len = 0;
}

// And a command to the buffer
int buildCmd(char* new_cmd, int len) {
    // Check size
    if (len + cmd_buf_len > CMD_BUF_SIZE - 1) {
        // Buffer overrun
        // Reset buffer
        clearCache();
        return CMD_ERR_CMD_TOO_LONG;
    }
    memcpy(&cmd_buf[cmd_buf_len], new_cmd, len);
    cmd_buf_len += len;
    return CMD_OK;
}

int lookForChar(char c) {
    int i;
    for (i = 0; i < cmd_buf_len; i++) {
        if (cmd_buf[i] == c) return i;
    }
    return -1;
}

// Shift the buffer pointer
void shiftBuf(int len) {
    // Don't do anything if the shift amount is too much
    if (len > cmd_buf_len) return;

    int i;
    for (i = 0; i < len; i++) {
        cmd_buf[i] = cmd_buf[i + len];
    }
    cmd_buf_len -= len;

}

// Trim line ends
int trimCrlf(void) {
    if (cmd_buf_len == 0) return 0;

    char c;
    int shift_len = 0;
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
int crlfPos(void) {
    if (!cmd_buf_len) return 0;

    int cr_pos = lookForChar('\r');
    int lf_pos = lookForChar('\n');

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
int commandSize(void) {
    int pos = crlfPos();
    return (pos != -1) ? pos : 0;
}

int noopCommand(void) {
    if (crlfPos() == -1) return 0;

    return trimCrlf();
}

int commandComplete(void) {
    return (crlfPos() != -1);
}

int cmdBufLen(void) {
    return cmd_buf_len;
}

// Get a command string from the command cache
// and copy it into the buffer
int getCmd(char* buf, int buf_len) {
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

// Decode a scale command
int cmdDecodeScale(ScaleCmd* cmd) {
	const Command* base = &cmd->base;
	if (base->numargs != 4) return CMD_ERR_WRONG_NUM_ARGS;
	cmd->x_scale  = atoi(base->args[0]);
	cmd->y_scale  = atoi(base->args[1]);
	cmd->x_offset = atoi(base->args[2]);
	cmd->y_offset = atoi(base->args[3]);
	return CMD_OK;
}

// Decode a point command
int cmdDecodePoint(PointCmd* cmd) {
	const Command* base = &cmd->base;
	if (base->numargs != 2) return CMD_ERR_WRONG_NUM_ARGS;
	cmd->x = atoi(base->args[0]);
	cmd->y = atoi(base->args[1]);
}

// Decode a line command
int cmdDecodeLine(LineCmd* cmd) {
	const Command* base = &cmd->base;
	if (base->numargs != 5) return CMD_ERR_WRONG_NUM_ARGS;
	cmd->x1 = atoi(base->args[0]);
	cmd->y1 = atoi(base->args[1]);
	cmd->x2 = atoi(base->args[2]);
	cmd->y2 = atoi(base->args[3]);
	cmd->ms = atoi(base->args[4]);
}

// Parse a command line
int cmdParse(RingMemPool* cmd_pool, char* buf, int len) {
	// Get a new command
	Command* cmd = ring_get(cmd_pool, sizeof(Command));
	if (!cmd) return CMD_ERROR_OTHER;


	// Command arguments are space separated
    int count = 0;
	char* cmd_start = buf;
    for (int i = 0; i < len; i++) {
        if (buf[i] == ' ') {
			// Trim off leading spaces
			if (count == 0) {
				cmd_start += 1;
				continue;
			}

			// Safety check
            if (count == CMD_MAX_NUM_ARGS) return CMD_ERR_TOO_MANY_ARGS;

			// Reached the end of an argument
            buf[i] = '\0'; // Replace with null char
            // Pointer to argument
            cmd->args[count++] = &buf[++i];
        }
    }
    if (!count) return CMD_ERR_WRONG_NUM_ARGS;

    buf[i] = '\0'; // Mark end of last arg
    cmd->buf = cmd_start;
    cmd->numargs = count;

    // Get cmd type
	int (*decode_fn)(Command* cmd) = NULL;
	int cmd_size = 0;
    if (strcmp(cmd_set[Cmd_Scale], buf) == 0) {
        cmd->type = Cmd_Scale;
		decode_fn = cmdDecodeScale;
		cmd_size  = sizeof(ScaleCmd);
    }
	else if (strcmp(cmd_set[Cmd_Point], buf) == 0) {
        cmd->type = Cmd_Point;
		decode_fn = cmdDecodePoint;
		cmd_size  = sizeof(PointCmd);
    }
	else if (strcmp(cmd_set[Cmd_Line], buf) == 0) {
        cmd->type = Cmd_Line;
		decode_fn = cmdDecodeLine;
		cmd_size  = sizeof(LineCmd);
    }
	else if (strcmp(cmd_set[Cmd_Noop], buf) == 0) {
        cmd->type = Cmd_Noop;
    }
    else {
        return CMD_ERR_BAD_CMD;
    }

	// Resize command and process it
	if (decode_fn) {
		cmd = ring_resize(cmd_pool, cmd_size);
		if (cmd == NULL) {
			// Failed to resize the command in the pool
			return CMD_ERROR_OTHER;
		}
		decode_fn(cmd);
	}

    return CMD_OK;
}

const char* cmdErrToText(int errcode) {
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
    case CMD_ERR_TOKEN:
        return "Token error";
    default:
        return "Unknown command error";
    }
}
