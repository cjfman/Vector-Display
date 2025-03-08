#include <math.h>
#include <SPI.h>

bool PROMPT = true;
bool DEBUG  = false;

extern "C" {
#include "command_parser.h"
#include "ring_mem_pool.h"
#include "screen_controller.h"
#include "utils.h"
}

#define BAUD 115200
#define DAC_SYNC 9
#define DAC_LDAC 8
#define DAC_CLR  7
#define DAC_RSET 4
//#define DAC_CLK_SPEED 5000000 // 5MHz / 200ns
#define DAC_CLK_SPEED 1000000 // 1MHz

char motion_mem[256];
RingMemPool motion_pool = {0};
ScreenState main_screen = {0};

void newline() {
    Serial.write("\n");
}

void debugPrint(String msg) {
    newline();
    Serial.write(msg.c_str());
    newline();
}

void printPrompt() {
    Serial.write("> ");
}

void printErrorCode(err_t errcode) {
    newline();
    Serial.write("NAK: ");
    Serial.write(cmdErrToText(errcode));
    Serial.write("\n");
}

void printError(char* msg) {
    Serial.write("NAK: ");
    Serial.write(msg);
    Serial.write("\n");
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

static inline void dac_write(uint16_t val) {
    // Write data
    SPI.beginTransaction(SPISettings(DAC_CLK_SPEED, MSBFIRST, SPI_MODE1));
    digitalWrite(DAC_SYNC, LOW);
    SPI.transfer16(val);
    digitalWrite(DAC_SYNC, HIGH);
    SPI.endTransaction();

    // Load data
    digitalWrite(DAC_LDAC, LOW);
    delayMicroseconds(1);
    digitalWrite(DAC_LDAC, HIGH);
}

static inline void dac_write2(uint16_t x, uint16_t y) {
    // Write data
    // X
    SPI.beginTransaction(SPISettings(DAC_CLK_SPEED, MSBFIRST, SPI_MODE1));
    digitalWrite(DAC_SYNC, LOW);
    SPI.transfer16(y);
    SPI.transfer16(x);
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
    uint16_t new_x = position_to_binary(screen->beam.x, screen->x_size_pow, DAC_BIT_WIDTH, true);
    uint16_t new_y = position_to_binary(screen->beam.y, screen->y_size_pow, DAC_BIT_WIDTH, true);
    if (new_x == x && new_y == y) {
        // Nothing to do
        return;
    }
    x = new_x;
    y = new_y;
    if (DEBUG) {
        Serial.print("Updating screen:");
        Serial.print(" x = ");
        Serial.print(screen->beam.x);
        Serial.print(" -> 0x");
        Serial.print(x, HEX);
        Serial.print(" y = ");
        Serial.print(screen->beam.y);
        Serial.print(" -> 0y");
        Serial.print(y, HEX);
        Serial.write("\n");
    }
    dac_write2(x, y);
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

    // Set up comms
    SPI.begin();
    Serial.begin(BAUD);
    Serial.write("Vector Generator Command Terminal\n");

    // Initialize memory
    screen_init(&main_screen);
    ring_init(&motion_pool, motion_mem, sizeof(motion_mem));
    main_screen.x_size_pow = 11;
    main_screen.y_size_pow = 11;
    main_screen.x_centered = true;
    main_screen.y_centered = true;

    // Print prompt
    if (PROMPT) printPrompt();
}

void checkForCommand(void) {
    char cmd_buf[CMD_BUF_SIZE];
    memset(cmd_buf, '\0', CMD_BUF_SIZE);

    int read_len = Serial.available();
    if (read_len == 0) return;

    // Read bytes
    int total = Serial.readBytes(cmd_buf, read_len);
    if (total != read_len) {
        if (PROMPT) {
            Serial.write("NAK: Unknown error\n");
            newline();
            printPrompt();
        }
        else {
            Serial.write("NAK\n");
        }
        return;
    }

    // Build command
    err_t errcode = buildCmd(cmd_buf, read_len);
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
        if (PROMPT) {
            printPrompt();
        }
        return;
    }

    // Load command
    errcode = getCmd(cmd_buf, CMD_BUF_SIZE);
    if (errcode) {
        if (PROMPT) {
            printErrorCode(errcode);
            printPrompt();
        }
        return;
    }

    // Parse command
    CommandUnion cmd;
    errcode = cmdParse(&cmd, cmd_buf, CMD_BUF_SIZE);
    if (errcode) {
        if (PROMPT) {
            printErrorCode(errcode);
            printPrompt();
        }
        return;
    }

    // Run command
    bool success = false;
    ScreenMotion* motion = nullptr;
    switch (cmd.base.type) {
    case Cmd_Point:
        motion = (ScreenMotion*)screen_push_point(&motion_pool, (PointCmd*)&cmd);
        break;
    case Cmd_Line:
        motion = (ScreenMotion*)screen_push_line(&motion_pool, (LineCmd*)&cmd, main_screen.speed);
        break;
    case Cmd_Scale:
        main_screen.x_size_pow = log2ceil(cmd.scale.x_width);
        main_screen.y_size_pow = log2ceil(cmd.scale.y_width);
        main_screen.x_centered = cmd.scale.x_centered;
        main_screen.y_centered = cmd.scale.y_centered;
        success = true;
        break;
    case Cmd_Speed:
        if (cmd.speed.hold_time > 0) {
            main_screen.hold_time = cmd.speed.hold_time;
            success = true;
        }
        if (cmd.speed.speed > 0) {
            main_screen.speed = cmd.speed.speed * 1000;
            success = true;
        }
        break;
    case Cmd_Sequence:
        if (cmd.sequence.start) {
            success = sequence_start(&main_screen);
            if (success) ring_reset(&motion_pool);
        }
        else if (cmd.sequence.end) {
            success = sequence_end(&main_screen);
        }
        else if (cmd.sequence.clear) {
            success = sequence_clear(&main_screen);
            if (success) ring_reset(&motion_pool);
        }
        break;
    case Cmd_Set:
    case Cmd_Unset:
        if (strcmp(cmd.set.name, "debug") == 0) {
            DEBUG = cmd.set.set;
            success = true;
        }
        else if (strcmp(cmd.set.name, "prompt") == 0) {
            PROMPT = cmd.set.set;
            success = true;
        }
        break;
    case Cmd_Noop:
        success = true;
        break;
    default:
        break;
    }

    if (motion != NULL && main_screen.sequence_enabled) {
        success = add_to_sequence(&main_screen, motion);
    }

    if (PROMPT) {
        printCommand((Command*)&cmd);
        Serial.print("\n");
        if (motion != NULL) {
            Serial.print("New Motion: 0x");
            Serial.print((uint16_t)motion, HEX);
            if (DEBUG) {
                Serial.write(" ");
                serialPrintMotion(motion);
            }
            Serial.print("\n");
        }
        else {
            Serial.print((success) ? "OK\n" : "FAILED\n");
        }
        printPrompt();
    }
    else {
        Serial.print((success || motion) ? "ACK\n" : "NAK\n");
    }
}

void loop() {
    // Check for command, then update the screen

    uint32_t now = micros();
    bool active = update_screen(now, &main_screen, &motion_pool);

    // Handle debug
    static int32_t debug_start = -1;
    static BeamState beam_state;
    static const ScreenMotion* last_motion = nullptr;
    if (DEBUG) {
        bool printed = false;
        uint32_t after = micros();
        if (debug_start < 0) debug_start = now;
        const ScreenMotion* next_motion = ring_peek(&motion_pool);
        if (memcmp(&beam_state, &main_screen.beam, sizeof(BeamState)) != 0) {
            Serial.print("\n");
            Serial.print("Screen update time ");
            Serial.print(after - now);
            Serial.print("us\n");
            /*
            Serial.print("Time: ");
            Serial.print(now / 1000.0);
            Serial.print("ms | ");
            //*/
            printBeamState(&main_screen.beam);
            Serial.print("\n");
            memcpy(&beam_state, &main_screen.beam, sizeof(BeamState));
            printed = true;
        }
        /*
        if (last_motion != next_motion && next_motion && PROMPT) {
            Serial.write((String("\nNext motion 0x") + String((size_t)next_motion, HEX) + ": ").c_str());
            serialPrintMotion(next_motion);
            Serial.write("\n");
            last_motion = next_motion;
            printed = true;
        }
        //*/

        // Stop debug after 10 seconds
        if (now - debug_start > 10000000l) {
            debug_start = -1;
            DEBUG = false;
        }
        if (printed) {
            if (PROMPT) {
                printPrompt();
            }
            else {
                Serial.write("\n");
            }
        }
    }

    // Update the screen
    update_dac(&main_screen);

    // Handle command check
    if (!active || !DEBUG) {
        checkForCommand();
    }
}
