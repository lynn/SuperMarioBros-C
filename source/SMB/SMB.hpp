#ifndef SMB_HPP
#define SMB_HPP

#include "SMBConstants.hpp"
#include "SMBEngine.hpp"

#include <cstdio>

//---------------------------------------------------------------------
// Macros:
//---------------------------------------------------------------------

/**
 * A jump engine landed on an index the table does not have.
 */
#define bad_jump() printf("bad jump: %d\n", __LINE__);

/**
 * Access a byte of emulated memory.
 */
#define M(addr) getMemory(addr)

/**
 * Read a word from emulated memory (in little-endian format).
 */
#define W(addr) getMemoryWord(addr)

/**
 * High/upper byte of a 16-bit integer.
 */
#define HIBYTE(v) (static_cast<uint8_t>(((v) >> 8) & 0xff))

/**
 * Low byte of a 16-bit integer.
 */
#define LOBYTE(v) (static_cast<uint8_t>((v) & 0xff))

#endif // SMB_HPP


enum class Mt : uint8_t {
    Blank = 0x00,
    Black = 0x01,
    BushLeft = 0x02,
    BushMiddle = 0x03,
    BushRight = 0x04,
    MountainLeft = 0x05,
    MountainTwoEyes = 0x06,
    MountainTop = 0x07,
    MountainRight = 0x08,
    MountainOneEye = 0x09,
    MountainSolid = 0x0a,
    BridgeGuardrail = 0x0b,
    Chain = 0x0c,
    TallTreetopTopHalf = 0x0d,
    ShortTreetop = 0x0e,
    TallTreetopBottomHalf = 0x0f,
    // Solid starts here
    PipeEntranceLeft = 0x10,
    PipeEntranceRight = 0x11,
    PipeTopLeft = 0x12,
    PipeTopRight = 0x13,
    PipeShaftLeft = 0x14,
    PipeShaftRight = 0x15,
    TreeLedgeLeftEdge = 0x16,
    TreeLedgeMiddle = 0x17,
    TreeLedgeRightEdge = 0x18,
    MushroomLeftEdge = 0x19,
    MushroomMiddle = 0x1a,
    MushroomRightEdge = 0x1b,
    SidewaysPipeEndTop = 0x1c,
    SidewaysPipeShaftTop = 0x1d,
    SidewaysPipeJointTop = 0x1e,
    SidewaysPipeEndBottom = 0x1f,
    SidewaysPipeShaftBottom = 0x20,
    SidewaysPipeJointBottom = 0x21,
    Seaplant = 0x22,
    BlankHitBlock = 0x23, // blank, used on bricks or blocks that are being hit
    // Climbable starts here
    FlagpoleBall = 0x24,
    FlagpoleShaft = 0x25,
    BlankVine = 0x26, // blank, used in conjunction with vines

    VerticalRope = 0x40,
    HorizontalRope = 0x41,
    LeftPulley = 0x42,
    RightPulley = 0x43,
    BlankBalanceRope = 0x44,
    CastleTop = 0x45,
    CastleWindowLeft = 0x46,
    CastleBrickWall = 0x47,
    CastleWindowRight = 0x48,
    CastleTopWithBrick = 0x49,
    EntranceTop = 0x4a,
    EntranceBottom = 0x4b,
    GreenLedgeStump = 0x4c,
    Fence = 0x4d,
    TreeTrunk = 0x4e,
    MushroomStumpTop = 0x4f,
    MushroomStumpBottom = 0x50,

    // Bumpable I guess?
    LinedBreakableBrick = 0x51,
    BreakableBrick = 0x52,
    BreakableBrickNotUsed = 0x53,
    CrackedRockTerrain = 0x54,
    LinedBrickPowerUp = 0x55,
    LinedBrickVine = 0x56,
    LinedBrickStar = 0x57,
    LinedBrickCoins = 0x58,
    LinedBrick1Up = 0x59,
    BrickPowerUp = 0x5a,
    BrickVine = 0x5b,
    BrickStar = 0x5c,
    BrickCoins = 0x5d,
    Brick1Up = 0x5e,
    HiddenBlock1Coin = 0x5f,
    HiddenBlock1Up = 0x60,

    // Solid starts here
    SolidBlock3D = 0x61,
    SolidBlockWhiteWall = 0x62,
    Bridge = 0x63,
    BulletBillCannonBarrel = 0x64,
    BulletBillCannonTop = 0x65,
    BulletBillCannonBottom = 0x66,
    BlankJumpspring = 0x67,
    HalfBrickJumpspring = 0x68,
    SolidBlockWaterLevel = 0x69, // solid block (water level, green rock)
    HalfBrick = 0x6a,
    WaterPipeTop = 0x6b,
    WaterPipeBottom = 0x6c,
    FlagBallResidual = 0x6d,

    CloudTopLeft = 0x80,
    CloudTopMiddle = 0x81,
    CloudTopRight = 0x82,
    CloudBottomLeft = 0x83,
    CloudBottomMiddle = 0x84,
    CloudBottomRight = 0x85,
    WaterLavaTop = 0x86,
    WaterLava = 0x87,
    CloudLevelTerrain = 0x88,
    BowsersBridge = 0x89,

    QuestionBlockWithCoin = 0xc0,
    QuestionBlockWithPowerUp = 0xc1,
    Coin = 0xc2,
    UnderwaterCoin = 0xc3,
    EmptyBlock = 0xc4,
    Axe = 0xc5,
};
