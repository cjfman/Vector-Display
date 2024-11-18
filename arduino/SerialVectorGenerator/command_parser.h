#ifndef CONTROL_PARSER_HH
#define CONTROL_PARSER_HH

#include "ring_mem_pool.h"

#define CMD_BUF_SIZE 256
#define CMD_MAX_NUM_ARGS 10
#define CMD_MAX_TOKEN 16

#define CMD_OK 0
#define CMD_ERROR_OTHER -1
#define CMD_ERR_BUF_OVERRUN -2
#define CMD_ERR_BAD_CMD -3
#define CMD_ERR_CMD_TOO_LONG -4
#define CMD_ERR_CMD_NOOP -5
#define CMD_ERR_TOO_MANY_ARGS -6
#define CMD_ERR_WRONG_NUM_ARGS -7
#define CMD_ERR_PARSE -8
#define CMD_ERR_TOKEN -9

typedef enum CommandType {
	Cmd_Scale,
    Cmd_Point,
	Cmd_Line,
    Cmd_Noop,
	Cmd_Num,
} CommandType;

// Command formats
// Scale: Set the scale for the dimentions
//        scale x_scale y_scale x_offset y_offset
//        x_scale: Number that represents distance between center and edge of x-dimention
//        y_scale: Number that represents distance between center and edge of y-dimention
//        x_offset: Sets zero point for x-dimention. 0 is center of the screen. -x_scale is left edge
//        y_offset: Sets zero point for y-dimention. 0 is center of the screen. -y_scale is bottom edge
//  
// Set: Set the position on the screen
//      set x y
// Line: Draw a line on the sreen
//       line x1 y1 x2 y2 ms
//       x1: Start position x-dimention
//       y1: Start position y-dimention
//       x2: End position x-dimention
//       y2: End position y-dimention
//       ms: Time in milliseconds to get to that position

typedef enum CommandParseState {
    Start,
    Idle,
    TokenStart,
    Token,
} CommandParseState;

typedef struct Command {
    char* buf;
    CommandType type;
    int numargs;
    char* args[CMD_MAX_NUM_ARGS];
} Command;

typedef struct ScaleCmd {
	Command base;
	unsigned x_scale;
	unsigned y_scale;
	int x_offset;
	int y_offset;
} ScaleCmd;

typedef struct PointCmd {
	Command base;
	int x;
	int y;
} PointCmd;

typedef struct LineCmd {
	Command base;
	int x1;
	int y1;
	int x2;
	int y2;
} LineCmd;

typedef union CmdUnion {
	Command  base;
	ScaleCmd scale;
	PointCmd point;
	LineCmd  line;
} CommandUnion;

void clearCache(void);
int buildCmd(char* new_cmd, int len);
int commandSize(void);
int noopCommand(void);
int commandComplete(void);
int cmdBufLen(void);
int getCmd(char* buf, int buf_len);
int cmdParse(CommandUnion* cmd_pool, char* buf, int len);
const char* cmdErrToText(int errcode);

#endif // CONTROL_PARSER_HH
