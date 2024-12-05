#include <SPI.h>

extern "C" {
#include "command_parser.h"
#include "ring_mem_pool.h"
#include "screen_controller.h"
}

#define BAUD 115200
#define DAC_SYNC 12
#define DAC_LDAC 8
#define DAC_CLR  7
#define DAC_RSET 4
#define DAC_CLK_SPEED 5000000 // 5MHz / 200ns

char motion_mem[100];
RingMemPool motion_pool = {0};
ScreenState main_screen = {0};

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

static inline void dac_reset(void) {
    // Reset
    digitalWrite(DAC_RSET, LOW);
    delay(1);
    digitalWrite(DAC_RSET, HIGH);
    // Clear
    digitalWrite(DAC_CLR, LOW);
    delay(1);
    digitalWrite(DAC_CLR, HIGH);
}

void dac_write(uint16_t x, uint16_t y) {
    // Write data
    SPI.beginTransaction(SPISettings(DAC_CLK_SPEED, MSBFIRST, SPI_MODE1));
    digitalWrite(DAC_SYNC, LOW);
    SPI.transfer(x, 16);
    SPI.transfer(y, 16);
    digitalWrite(DAC_SYNC, HIGH);
    SPI.endTransaction();

    // Load data
    digitalWrite(DAC_LDAC, LOW);
    delayMicroseconds(1);
    digitalWrite(DAC_LDAC, HIGH);
}

void update_dac(const ScreenState* screen) {
    static uint16_t x = 0;
    static uint16_t y = 0;
    if (screen->beam.x == x && screen->beam.y == y) {
        // Nothing to do
        return;
    }
    x = screen->beam.x;
    y = screen->beam.y;
    dac_write(x, y);
}

void setup() {
    // Setup DAC control logic pins
    // The are active low
    digitalWrite(DAC_SYNC, HIGH);
    digitalWrite(DAC_LDAC, HIGH);
    digitalWrite(DAC_CLR,  HIGH);
    digitalWrite(DAC_RSET, HIGH);
    pinMode(DAC_SYNC, OUTPUT);
    pinMode(DAC_LDAC, OUTPUT);
    pinMode(DAC_CLR,  OUTPUT);
    pinMode(DAC_RSET, OUTPUT);
    dac_reset();

    // Set up serial
    Serial.begin(BAUD);
    Serial.print("Vector Generator Command Terminal\n");

    // Initialize memory
    screen_init(&main_screen);
    ring_init(&motion_pool, motion_mem, sizeof(motion_mem));

    // Print prompt
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
        main_screen.x_width = cmd.scale.x_width;
        main_screen.y_width = cmd.scale.y_width;
        break;
    case Cmd_Speed:
        main_screen.hold_time = cmd.speed.hold_time;
        main_screen.speed     = cmd.speed.speed;
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
    update_screen(micros(), &main_screen, &motion_pool);
    update_dac(&main_screen);
}
