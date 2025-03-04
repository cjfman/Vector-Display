#include <SPI.h>

bool PROMPT = true;
bool DEBUG  = false;

extern "C" {
#include "command_parser.h"
#include "ring_mem_pool.h"
#include "screen_controller.h"
}

#define BAUD 115200
#define DAC_SYNC 9
#define DAC_LDAC 8
#define DAC_CLR  7
#define DAC_RSET 4
//#define DAC_CLK_SPEED 5000000 // 5MHz / 200ns
#define DAC_CLK_SPEED 1000000 // 1MHz

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

void dac_write(uint16_t val) {
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

void dac_write2(uint16_t x, uint16_t y) {
    // Write data
    // X
    SPI.beginTransaction(SPISettings(DAC_CLK_SPEED, MSBFIRST, SPI_MODE1));
    digitalWrite(DAC_SYNC, LOW);
    SPI.transfer16(x);
    SPI.transfer16(y);
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
    uint16_t new_x = position_to_binary(screen->beam.x, screen->x_width - screen->x_offset, 16, true);
    uint16_t new_y = position_to_binary(screen->beam.y, screen->y_width - screen->y_offset, 16, true);
    if (new_x == x && new_y == y) {
        // Nothing to do
        return;
    }
    x = new_x;
    y = new_y;
    if (DEBUG) {
        String msg = String("Updating screen:")
        + " x = " + screen->beam.x + " -> 0x" + String(x, HEX)
        + " y = " + screen->beam.y + " -> 0x" + String(y, HEX);
        Serial.write(msg.c_str());
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
    Serial.print("Vector Generator Command Terminal\n");

    // Initialize memory
    screen_init(&main_screen);
    ring_init(&motion_pool, motion_mem, sizeof(motion_mem));
    main_screen.x_width  = 200;
    main_screen.y_width  = 200;
    main_screen.x_offset = 100;
    main_screen.y_offset = 100;

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
            Serial.print("NAK: Unknown error\n");
            newline();
            printPrompt();
        }
        return;
    }

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
        if (PROMPT) {
            Serial.print("\nNoop\n");
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
        motion = (ScreenMotion*)screen_push_line(&motion_pool, (LineCmd*)&cmd);
        break;
    case Cmd_Scale:
        main_screen.x_width  = cmd.scale.x_width;
        main_screen.y_width  = cmd.scale.y_width;
        main_screen.x_offset = cmd.scale.x_offset;
        main_screen.y_offset = cmd.scale.y_offset;
        success = true;
        break;
    case Cmd_Speed:
        if (cmd.speed.hold_time > 0) {
            main_screen.hold_time = cmd.speed.hold_time;
            success = true;
        }
        if (cmd.speed.speed > 0) {
            main_screen.speed = cmd.speed.speed;
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
        Serial.print(commandToString((Command*)&cmd) + "\n");
        Serial.print((success) ? "OK" :
                     (motion)  ? (String("New Motion: 0x") + String((size_t)(motion), HEX) + " " + motionToString(motion)).c_str() :
                                 "FAILED");
        Serial.print("\n");
        printPrompt();
    }
}

void loop() {
    // Check for command, then update the screen

    long now = micros();
    bool active = update_screen(now, &main_screen, &motion_pool);

    // Handle debug
    static long debug_start = -1;
    static const ScreenMotion* last_motion = nullptr;
    static BeamState beam_state;
    const ScreenMotion* next_motion = ring_peek(&motion_pool);
    if (DEBUG) {
        if (debug_start < 0) debug_start = now;
        String msg;
        if (memcmp(&beam_state, &main_screen.beam, sizeof(BeamState)) != 0) {
            msg = String("Time: ") + (now / 1000.0) + "ms | " + beamStateToString(&main_screen.beam) + "\n";
            Serial.write(msg.c_str());
            Serial.write("\n");
            memcpy(&beam_state, &main_screen.beam, sizeof(BeamState));
        }
        if (last_motion != next_motion && next_motion && PROMPT) {
            msg = String("\nNext motion 0x") + String((size_t)next_motion, HEX) + ": " + motionToString(next_motion) + "\n";
            Serial.write(msg.c_str());
            Serial.write("\n");
            last_motion = next_motion;
        }

        // Stop debug after 10 seconds
        if (now - debug_start > 10000000l) {
            debug_start = -1;
            DEBUG = false;
            printPrompt();
        }
    }

    // Update the screen
    update_dac(&main_screen);

    // Handle command check
    if (!active || !DEBUG) {
        checkForCommand();
    }
}
