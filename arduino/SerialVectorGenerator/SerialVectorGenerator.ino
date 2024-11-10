extern "C" {
    #include "command_parser.h"
    #include "ring_mem_pool.h"
}
#define BAUD 115200

char cmd_mem[1<<10];
RingMemPool cmd_pool;

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
    ring_init(&cmd_pool, cmd_mem, sizeof(cmd_mem));
    printPrompt();
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
    errcode = cmdParse(&cmd_pool, cmd_buf, CMD_BUF_SIZE);
    if (errcode) {
        printErrorCode(errcode);
        printPrompt();
        return;
    }

    Serial.print(commandToString(last_entry(&cmd_pool)) + "\n");
    printPrompt();
}

void loop() {
    runOnce();
    delay(1);
}
