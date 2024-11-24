extern "C" {
#include "command_parser.h"
#include "ring_mem_pool.h"
#include "screen_controller.h"
}

#define BAUD 115200

char motion_mem[1<<10];
RingMemPool motion_pool = {0};

void newline() {
    Serial.print("\n");
}

void debugPrint(String msg) {
    newline();
    Serial.print(msg);
    newline();
}

void printPrompt() {
    Serial.print("> ");
}

void printErrorCode(int errcode) {
    newline();
    Serial.print("NAK: ");
    Serial.print(cmdErrToText(errcode));
    Serial.write('\n');
}

void printError(char* msg) {
    Serial.print("NAK: ");
    Serial.print(msg);
    Serial.write('\n');
}

String intToString(int i) {
    char s[10];
    snprintf(s, 10, "%d", i);
    return String(s);
}

void setup() {
    Serial.begin(BAUD);
    Serial.print("Vector Generator Command Terminal\n");
    screen_init();
    printPrompt();
    ring_init(&motion_pool, motion_mem, sizeof(motion_mem));
}

void runOnce(void) {
    char cmd_buf[CMD_BUF_SIZE];
    memset(cmd_buf, '\0', CMD_BUF_SIZE);

    int read_len = Serial.available();
    if (read_len == 0) return;

    // Read bytes
    int total = Serial.readBytes(cmd_buf, read_len);
    if (total != read_len) {
        Serial.print("NAK: Unknown error\n");
        newline();
        printPrompt();
        return;
    }
    String num = intToString(read_len);

    // Build command
    int errcode = buildCmd(cmd_buf, read_len);
    if (errcode) {
        printErrorCode(errcode);
        printPrompt();
        return;
    }

    // Check for full command
    if (!commandComplete()) {
        return;
    }

    // Check for noop command
    if (noopCommand()) {
        Serial.print("\nNoop\n");
        printPrompt();
        return;
    }

    // Load command
    errcode = getCmd(cmd_buf, CMD_BUF_SIZE);
    if (errcode) {
        printErrorCode(errcode);
        printPrompt();
        return;
    }

    // Parse command
    CommandUnion cmd;
    errcode = cmdParse(&cmd, cmd_buf, CMD_BUF_SIZE);
    if (errcode) {
        printErrorCode(errcode);
        printPrompt();
        return;
    }

    // Run command
    int success = 1;
    switch (cmd.base.type) {
    case Cmd_Point:
        success = screen_push_point(&motion_pool, (PointCmd*)&cmd);
        break;
    case Cmd_Line:
        success = screen_push_line(&motion_pool, (LineCmd*)&cmd);
        break;
    case Cmd_Scale:
        screen_set_scale((ScaleCmd*)&cmd);
        break;
    case Cmd_Noop:
        break;
    }

    Serial.print(commandToString((Command*)&cmd) + "\n");
    Serial.print((success) ? "OK" : "FAILED");
    printPrompt();
}

void loop() {
    runOnce();
    update_screen(micros(), &motion_pool);
    delay(1);
}
