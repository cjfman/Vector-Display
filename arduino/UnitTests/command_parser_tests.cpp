#include <string.h>

#include <string>

#include "gtest/gtest.h"

extern "C" {
#include "command_parser.h"
}

class CommandParserTest: public testing::Test {
protected:
    void SetUp() {
        clearCache();
        memset(this->cmd_buf, '\0', sizeof(this->cmd_buf));
    }
    void build_command(const char* cmd_str, unsigned size) {
        ASSERT_EQ(CMD_OK, buildCmd((std::string(cmd_str) + "\r\n").data(), size));
        ASSERT_TRUE(commandComplete());
        ASSERT_EQ(CMD_OK, getCmd(this->cmd_buf, CMD_BUF_SIZE));
        ASSERT_EQ(std::string(cmd_str), std::string(this->cmd_buf));
    }

    char cmd_buf[CMD_BUF_SIZE];
};

TEST_F(CommandParserTest, scale) {
    // Send and parse command
    const char cmd_str[] = "scale 44 87 1 0";
    this->build_command(cmd_str, sizeof(cmd_str));
    CommandUnion cmd;
    ASSERT_EQ(CMD_OK, cmdParse(&cmd, this->cmd_buf, CMD_BUF_SIZE))
        << "Base command: " << cmd.base.buf << "; Num args: " << cmd.base.numargs;

    // Check parsed command
    ASSERT_EQ(Cmd_Scale, cmd.base.type);
    ScaleCmd* scale_cmd = (ScaleCmd*)&cmd;
    EXPECT_EQ(44, scale_cmd->x_width);
    EXPECT_EQ(87, scale_cmd->y_width);
    EXPECT_TRUE(scale_cmd->x_centered);
    EXPECT_FALSE(scale_cmd->y_centered);
}

TEST_F(CommandParserTest, point) {
    // Send and parse command
    const char cmd_str[] = "point 42 -5";
    this->build_command(cmd_str, sizeof(cmd_str));
    CommandUnion cmd;
    ASSERT_EQ(CMD_OK, cmdParse(&cmd, this->cmd_buf, CMD_BUF_SIZE))
        << "Base command: " << cmd.base.buf << "; Num args: " << cmd.base.numargs;

    // Check parsed command
    ASSERT_EQ(Cmd_Point, cmd.base.type);
    PointCmd* point_cmd = (PointCmd*)&cmd;
    EXPECT_EQ(42, point_cmd->x);
    EXPECT_EQ(-5, point_cmd->y);
}

TEST_F(CommandParserTest, line) {
    // Send and parse command
    const char cmd_str[] = "line -2 0 452 87";
    this->build_command(cmd_str, sizeof(cmd_str));
    CommandUnion cmd;
    ASSERT_EQ(CMD_OK, cmdParse(&cmd, this->cmd_buf, CMD_BUF_SIZE))
        << "Base command: " << cmd.base.buf << "; Num args: " << cmd.base.numargs;

    // Check parsed command
    ASSERT_EQ(Cmd_Line, cmd.base.type);
    LineCmd* line_cmd = (LineCmd*)&cmd;
    EXPECT_EQ(-2,  line_cmd->x1);
    EXPECT_EQ(0,   line_cmd->y1);
    EXPECT_EQ(452, line_cmd->x2);
    EXPECT_EQ(87,  line_cmd->y2);
}
