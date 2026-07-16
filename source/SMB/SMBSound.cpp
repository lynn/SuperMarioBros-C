// The SoundEngine subsystem: everything SoundEngine() reaches that nothing
// outside it reaches, and so nothing outside it needs.
//
// Moved out of SMB.cpp by tools/split.py. These are methods of SMBEngine like every other
// routine of the game and are declared in SMBEngine.hpp; the file they are written in is the
// only thing that is different, and tools/layers.py is what keeps it meaning something.
//
#include "SMB.hpp"

struct Song
{
    uint8_t lengthByteOffset;
    const uint8_t *data;
    uint8_t bassOffset;
    uint8_t harmonyOffset;
    uint8_t percussionOffset;
};

const uint8_t Star_CloudMData_data[73] = {
    // square 2
    0x84, 0x2c, 0x2c, 0x2c, 0x82, 0x04, 0x2c, 0x04, 0x85, 0x2c, 0x84, 0x2c, 0x2c, 0x2a, 0x2a, 0x2a, 0x82, 0x04, 0x2a, 0x04, 0x85, 0x2a,
    0x84, 0x2a, 0x2a, 0x00,

    // square 1
    0x1f, 0x1f, 0x1f, 0x98, 0x1f, 0x1f, 0x98, 0x9e, 0x98, 0x1f, 0x1d, 0x1d, 0x1d, 0x94, 0x1d, 0x1d, 0x94, 0x9c, 0x94, 0x1d,

    // triangle
    0x86, 0x18, 0x85, 0x26, 0x30, 0x84, 0x04, 0x26, 0x30, 0x86, 0x14, 0x85, 0x22, 0x2c, 0x84, 0x04, 0x22, 0x2c,

    // noise
    0x21, 0xd0, 0xc4, 0xd0, 0x31, 0xd0, 0xc4, 0xd0, 0x00};

const uint8_t GroundM_P1Data_data[] = {
    // square 2
    0x85, 0x2c, 0x22, 0x1c, 0x84, 0x26, 0x2a, 0x82, 0x28, 0x26, 0x04, 0x87, 0x22, 0x34, 0x3a, 0x82, 0x40, 0x04, 0x36, 0x84, 0x3a, 0x34,
    0x82, 0x2c, 0x30, 0x85, 0x2a, 0x00,

    // square 1
    0x5d, 0x55, 0x4d, 0x15, 0x19, 0x96, 0x15, 0xd5, 0xe3, 0xeb, 0x2d, 0xa6, 0x2b, 0x27, 0x9c, 0x9e, 0x59,

    // triangle
    0x85, 0x22, 0x1c, 0x14, 0x84, 0x1e, 0x22, 0x82, 0x20, 0x1e, 0x04, 0x87, 0x1c, 0x2c, 0x34, 0x82, 0x36, 0x04, 0x30, 0x34, 0x04, 0x2c,
    0x04, 0x26, 0x2a, 0x85, 0x22,

    // square 1 of part 2 A...
    0x84, 0x04, 0x82, 0x3a, 0x38, 0x36, 0x32, 0x04, 0x34, 0x04, 0x24, 0x26, 0x2c, 0x04, 0x26, 0x2c, 0x30, 0x00, 0x05, 0xb4, 0xb2, 0xb0,
    0x2b, 0xac, 0x84, 0x9c, 0x9e, 0xa2, 0x84, 0x94, 0x9c, 0x9e, 0x85, 0x14, 0x22, 0x84, 0x2c, 0x85, 0x1e, 0x82, 0x2c, 0x84, 0x2c, 0x1e,

    0x84, 0x04, 0x82, 0x3a, 0x38, 0x36, 0x32, 0x04, 0x34, 0x04, 0x64, 0x04, 0x64, 0x86, 0x64, 0x00, 0x05, 0xb4, 0xb2, 0xb0, 0x2b, 0xac,
    0x84, 0x37, 0xb6, 0xb6, 0x45, 0x85, 0x14, 0x1c, 0x82, 0x22, 0x84, 0x2c, 0x4e, 0x82, 0x4e, 0x84, 0x4e, 0x22,

    0x84, 0x04, 0x85, 0x32, 0x85, 0x30, 0x86, 0x2c, 0x04, 0x00, 0x05, 0xa4, 0x05, 0x9e, 0x05, 0x9d, 0x85, 0x84, 0x14, 0x85, 0x24, 0x28,
    0x2c, 0x82, 0x22, 0x84, 0x22, 0x14, 0x21, 0xd0, 0xc4, 0xd0, 0x31, 0xd0, 0xc4, 0xd0, 0x00};

const uint8_t *SilenceData_data = GroundM_P1Data_data + 27; // the first 0x00
const uint8_t *GroundM_P2AData_data = GroundM_P1Data_data + 72;
const uint8_t *GroundM_P2BData_data = GroundM_P2AData_data + 44;
const uint8_t *GroundM_P2CData_data = GroundM_P2BData_data + 40;

const uint8_t GroundM_P3AData_data[] = {0x82, 0x2c, 0x84, 0x2c, 0x2c, 0x82, 0x2c, 0x30, 0x04, 0x34, 0x2c, 0x04, 0x26, 0x86, 0x22,
                                        0x00, 0xa4, 0x25, 0x25, 0xa4, 0x29, 0xa2, 0x1d, 0x9c, 0x95,

                                        0x82, 0x2c, 0x2c, 0x04, 0x2c, 0x04, 0x2c, 0x30, 0x85, 0x34, 0x04, 0x04, 0x00, 0xa4, 0x25,
                                        0x25, 0xa4, 0xa8, 0x63, 0x04, 0x85, 0x0e, 0x1a, 0x84, 0x24, 0x85, 0x22, 0x14, 0x84, 0x0c,

                                        0x82, 0x34, 0x84, 0x34, 0x34, 0x82, 0x2c, 0x84, 0x34, 0x86, 0x3a, 0x04, 0x00, 0xa0, 0x21,
                                        0x21, 0xa0, 0x21, 0x2b, 0x05, 0xa3, 0x82, 0x18, 0x84, 0x18, 0x18, 0x82, 0x18, 0x18, 0x04,
                                        0x86, 0x3a, 0x22, 0x31, 0x90, 0x31, 0x90, 0x31, 0x71, 0x31, 0x90, 0x90, 0x90, 0x00};

const uint8_t *GroundM_P3BData_data = GroundM_P3AData_data + 25;
const uint8_t *GroundMLdInData_data = GroundM_P3AData_data + 25 + 30;

const uint8_t GroundM_P4AData_data[] = {
    0x82, 0x34, 0x84, 0x2c, 0x85, 0x22, 0x84, 0x24, 0x82, 0x26, 0x36, 0x04, 0x36, 0x86, 0x26, 0x00, 0xac, 0x27, 0x5d, 0x1d,
    0x9e, 0x2d, 0xac, 0x9f, 0x85, 0x14, 0x82, 0x20, 0x84, 0x22, 0x2c, 0x1e, 0x1e, 0x82, 0x2c, 0x2c, 0x1e, 0x04,

    0x87, 0x2a, 0x40, 0x40, 0x40, 0x3a, 0x36, 0x82, 0x34, 0x2c, 0x04, 0x26, 0x86, 0x22, 0x00, 0xe3, 0xf7, 0xf7, 0xf7, 0xf5,
    0xf1, 0xac, 0x27, 0x9e, 0x9d, 0x85, 0x18, 0x82, 0x1e, 0x84, 0x22, 0x2a, 0x22, 0x22, 0x82, 0x2c, 0x2c, 0x22, 0x04,

    0x86, 0x04,

    0x82, 0x2a, 0x36, 0x04, 0x36, 0x87, 0x36, 0x34, 0x30, 0x86, 0x2c, 0x04, 0x00, 0x00, 0x68, 0x6a, 0x6c, 0x45, 0xa2, 0x31,
    0xb0, 0xf1, 0xed, 0xeb, 0xa2, 0x1d, 0x9c, 0x95, 0x86, 0x04, 0x85, 0x22, 0x82, 0x22, 0x87, 0x22, 0x26, 0x2a, 0x84, 0x2c,
    0x22, 0x86, 0x14, 0x51, 0x90, 0x31, 0x11, 0x00};

const uint8_t *GroundM_P4BData_data = GroundM_P4AData_data + 38;
const uint8_t *DeathMusData_data = GroundM_P4AData_data + 38 + 39;
const uint8_t *GroundM_P4CData_data = GroundM_P4AData_data + 38 + 39 + 2;

const uint8_t CastleMusData_data[161] = {
    0x80, 0x22, 0x28, 0x22, 0x26, 0x22, 0x24, 0x22, 0x26, 0x22, 0x28, 0x22, 0x2a, 0x22, 0x28, 0x22, 0x26, 0x22, 0x28, 0x22, 0x26,
    0x22, 0x24, 0x22, 0x26, 0x22, 0x28, 0x22, 0x2a, 0x22, 0x28, 0x22, 0x26, 0x20, 0x26, 0x20, 0x24, 0x20, 0x26, 0x20, 0x28, 0x20,
    0x26, 0x20, 0x28, 0x20, 0x26, 0x20, 0x24, 0x20, 0x26, 0x20, 0x24, 0x20, 0x26, 0x20, 0x28, 0x20, 0x26, 0x20, 0x28, 0x20, 0x26,
    0x20, 0x24, 0x28, 0x30, 0x28, 0x32, 0x28, 0x30, 0x28, 0x2e, 0x28, 0x30, 0x28, 0x2e, 0x28, 0x2c, 0x28, 0x2e, 0x28, 0x30, 0x28,
    0x32, 0x28, 0x30, 0x28, 0x2e, 0x28, 0x30, 0x28, 0x2e, 0x28, 0x2c, 0x28, 0x2e, 0x00, 0x04, 0x70, 0x6e, 0x6c, 0x6e, 0x70, 0x72,
    0x70, 0x6e, 0x70, 0x6e, 0x6c, 0x6e, 0x70, 0x72, 0x70, 0x6e, 0x6e, 0x6c, 0x6e, 0x70, 0x6e, 0x70, 0x6e, 0x6c, 0x6e, 0x6c, 0x6e,
    0x70, 0x6e, 0x70, 0x6e, 0x6c, 0x76, 0x78, 0x76, 0x74, 0x76, 0x74, 0x72, 0x74, 0x76, 0x78, 0x76, 0x74, 0x76, 0x74, 0x72, 0x74,
    0x84, 0x1a, 0x83, 0x18, 0x20, 0x84, 0x1e, 0x83, 0x1c, 0x28, 0x26, 0x1c, 0x1a, 0x1c};

const uint8_t GameOverMusData_data[45] = {0x82, 0x2c, 0x04, 0x04, 0x22, 0x04, 0x04, 0x84, 0x1c, 0x87, 0x26, 0x2a, 0x26, 0x84, 0x24,
                                          0x28, 0x24, 0x80, 0x22, 0x00, 0x9c, 0x05, 0x94, 0x05, 0x0d, 0x9f, 0x1e, 0x9c, 0x98, 0x9d,
                                          0x82, 0x22, 0x04, 0x04, 0x1c, 0x04, 0x04, 0x84, 0x14, 0x86, 0x1e, 0x80, 0x16, 0x80, 0x14};

const uint8_t TimeRunOutMusData_data[62] = {0x81, 0x1c, 0x30, 0x04, 0x30, 0x30, 0x04, 0x1e, 0x32, 0x04, 0x32, 0x32, 0x04, 0x20, 0x34, 0x04,
                                            0x34, 0x34, 0x04, 0x36, 0x04, 0x84, 0x36, 0x00, 0x46, 0xa4, 0x64, 0xa4, 0x48, 0xa6, 0x66, 0xa6,
                                            0x4a, 0xa8, 0x68, 0xa8, 0x6a, 0x44, 0x2b, 0x81, 0x2a, 0x42, 0x04, 0x42, 0x42, 0x04, 0x2c, 0x64,
                                            0x04, 0x64, 0x64, 0x04, 0x2e, 0x46, 0x04, 0x46, 0x46, 0x04, 0x22, 0x04, 0x84, 0x22};

const uint8_t WinLevelMusData_data[97] = {
    0x87, 0x04, 0x06, 0x0c, 0x14, 0x1c, 0x22, 0x86, 0x2c, 0x22, 0x87, 0x04, 0x60, 0x0e, 0x14, 0x1a, 0x24, 0x86, 0x2c, 0x24,
    0x87, 0x04, 0x08, 0x10, 0x18, 0x1e, 0x28, 0x86, 0x30, 0x30, 0x80, 0x64, 0x00, 0xcd, 0xd5, 0xdd, 0xe3, 0xed, 0xf5, 0xbb,
    0xb5, 0xcf, 0xd5, 0xdb, 0xe5, 0xed, 0xf3, 0xbd, 0xb3, 0xd1, 0xd9, 0xdf, 0xe9, 0xf1, 0xf7, 0xbf, 0xff, 0xff, 0xff, 0x34,
    0x00, // unused byte
    0x86, 0x04, 0x87, 0x14, 0x1c, 0x22, 0x86, 0x34, 0x84, 0x2c, 0x04, 0x04, 0x04, 0x87, 0x14, 0x1a, 0x24, 0x86, 0x32, 0x84,
    0x2c, 0x04, 0x86, 0x04, 0x87, 0x18, 0x1e, 0x28, 0x86, 0x36, 0x87, 0x30, 0x30, 0x30, 0x80, 0x2c};

const uint8_t UndergroundMusData_data[65] = {
    0x82, 0x14, 0x2c, 0x62, 0x26, 0x10, 0x28, 0x80, 0x04, 0x82, 0x14, 0x2c, 0x62, 0x26, 0x10, 0x28, 0x80, 0x04, 0x82, 0x08, 0x1e, 0x5e,
    0x18, 0x60, 0x1a, 0x80, 0x04, 0x82, 0x08, 0x1e, 0x5e, 0x18, 0x60, 0x1a, 0x86, 0x04, 0x83, 0x1a, 0x18, 0x16, 0x84, 0x14, 0x1a, 0x18,
    0x0e, 0x0c, 0x16, 0x83, 0x14, 0x20, 0x1e, 0x1c, 0x28, 0x26, 0x87, 0x24, 0x1a, 0x12, 0x10, 0x62, 0x0e, 0x80, 0x04, 0x04, 0x00};

const uint8_t WaterMusData_data[255] = {
    0x82, 0x18, 0x1c, 0x20, 0x22, 0x26, 0x28, 0x81, 0x2a, 0x2a, 0x2a, 0x04, 0x2a, 0x04, 0x83, 0x2a, 0x82, 0x22, 0x86, 0x34, 0x32, 0x34,
    0x81, 0x04, 0x22, 0x26, 0x2a, 0x2c, 0x30, 0x86, 0x34, 0x83, 0x32, 0x82, 0x36, 0x84, 0x34, 0x85, 0x04, 0x81, 0x22, 0x86, 0x30, 0x2e,
    0x30, 0x81, 0x04, 0x22, 0x26, 0x2a, 0x2c, 0x2e, 0x86, 0x30, 0x83, 0x22, 0x82, 0x36, 0x84, 0x34, 0x85, 0x04, 0x81, 0x22, 0x86, 0x3a,
    0x3a, 0x3a, 0x82, 0x3a, 0x81, 0x40, 0x82, 0x04, 0x81, 0x3a, 0x86, 0x36, 0x36, 0x36, 0x82, 0x36, 0x81, 0x3a, 0x82, 0x04, 0x81, 0x36,
    0x86, 0x34, 0x82, 0x26, 0x2a, 0x36, 0x81, 0x34, 0x34, 0x85, 0x34, 0x81, 0x2a, 0x86, 0x2c, 0x00, 0x84, 0x90, 0xb0, 0x84, 0x50, 0x50,
    0xb0, 0x00, 0x98, 0x96, 0x94, 0x92, 0x94, 0x96, 0x58, 0x58, 0x58, 0x44, 0x5c, 0x44, 0x9f, 0xa3, 0xa1, 0xa3, 0x85, 0xa3, 0xe0, 0xa6,
    0x23, 0xc4, 0x9f, 0x9d, 0x9f, 0x85, 0x9f, 0xd2, 0xa6, 0x23, 0xc4, 0xb5, 0xb1, 0xaf, 0x85, 0xb1, 0xaf, 0xad, 0x85, 0x95, 0x9e, 0xa2,
    0xaa, 0x6a, 0x6a, 0x6b, 0x5e, 0x9d, 0x84, 0x04, 0x04, 0x82, 0x22, 0x86, 0x22, 0x82, 0x14, 0x22, 0x2c, 0x12, 0x22, 0x2a, 0x14, 0x22,
    0x2c, 0x1c, 0x22, 0x2c, 0x14, 0x22, 0x2c, 0x12, 0x22, 0x2a, 0x14, 0x22, 0x2c, 0x1c, 0x22, 0x2c, 0x18, 0x22, 0x2a, 0x16, 0x20, 0x28,
    0x18, 0x22, 0x2a, 0x12, 0x22, 0x2a, 0x18, 0x22, 0x2a, 0x12, 0x22, 0x2a, 0x14, 0x22, 0x2c, 0x0c, 0x22, 0x2c, 0x14, 0x22, 0x34, 0x12,
    0x22, 0x30, 0x10, 0x22, 0x2e, 0x16, 0x22, 0x34, 0x18, 0x26, 0x36, 0x16, 0x26, 0x36, 0x14, 0x26, 0x36, 0x12, 0x22, 0x36, 0x5c, 0x22,
    0x34, 0x0c, 0x22, 0x22, 0x81, 0x1e, 0x1e, 0x85, 0x1e, 0x81, 0x12, 0x86, 0x14};

const uint8_t EndOfCastleMusData_data[119] = {
    0x81, 0x2c, 0x22, 0x1c, 0x2c, 0x22, 0x1c, 0x85, 0x2c, 0x04, 0x81, 0x2e, 0x24, 0x1e, 0x2e, 0x24, 0x1e, 0x85, 0x2e, 0x04,
    0x81, 0x32, 0x28, 0x22, 0x32, 0x28, 0x22, 0x85, 0x32, 0x87, 0x36, 0x36, 0x36, 0x84, 0x3a, 0x00, 0x5c, 0x54, 0x4c, 0x5c,
    0x54, 0x4c, 0x5c, 0x1c, 0x1c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5e, 0x56, 0x4e, 0x5e, 0x56, 0x4e, 0x5e, 0x1e, 0x1e, 0x5e, 0x5e,
    0x5e, 0x5e, 0x62, 0x5a, 0x50, 0x62, 0x5a, 0x50, 0x62, 0x22, 0x22, 0x62, 0xe7, 0xe7, 0xe7, 0x2b, 0x86, 0x14, 0x81, 0x14,
    0x80, 0x14, 0x14, 0x81, 0x14, 0x14, 0x14, 0x14, 0x86, 0x16, 0x81, 0x16, 0x80, 0x16, 0x16, 0x81, 0x16, 0x16, 0x16, 0x16,
    0x81, 0x28, 0x22, 0x1a, 0x28, 0x22, 0x1a, 0x28, 0x80, 0x28, 0x28, 0x81, 0x28, 0x87, 0x2c, 0x2c, 0x2c, 0x84, 0x30};

const uint8_t VictoryMusData_data[56] = {0x83, 0x04, 0x84, 0x0c, 0x83, 0x62, 0x10, 0x84, 0x12, 0x83, 0x1c, 0x22, 0x1e, 0x22,
                                         0x26, 0x18, 0x1e, 0x04, 0x1c, 0x00, 0xe3, 0xe1, 0xe3, 0x1d, 0xde, 0xe0, 0x23, 0xec,
                                         0x75, 0x74, 0xf0, 0xf4, 0xf6, 0xea, 0x31, 0x2d, 0x83, 0x12, 0x14, 0x04, 0x18, 0x1a,
                                         0x1c, 0x14, 0x26, 0x22, 0x1e, 0x1c, 0x18, 0x1e, 0x22, 0x0c, 0x14, 0xff, 0xff, 0xff};

const Song TimeRunningOutHdr = Song{0x08, TimeRunOutMusData_data, 0x27, 0x18};
const Song Star_CloudHdr = Song{0x20, Star_CloudMData_data, 0x2e, 0x1a, 0x40};
const Song EndOfLevelMusHdr = Song{0x20, WinLevelMusData_data, 0x3d, 0x21};
const Song ResidualHeaderData = Song{0x20, nullptr, 0x3f, 0x1d};
const Song UndergroundMusHdr = Song{0x18, UndergroundMusData_data, 0x00, 0x00};
const Song SilenceHdr = Song{0x08, SilenceData_data, 0x00, 0x00, /*overrun*/ 0xa4};
const Song CastleMusHdr = Song{0x00, CastleMusData_data, 0x93, 0x62};
const Song VictoryMusHdr = Song{0x10, VictoryMusData_data, 0x24, 0x14};
const Song GameOverMusHdr = Song{0x18, GameOverMusData_data, 0x1e, 0x14};
const Song WaterMusHdr = Song{0x08, WaterMusData_data, 0xa0, 0x70, 0x68};
const Song WinCastleMusHdr = Song{0x08, EndOfCastleMusData_data, 0x4c, 0x24, /*overrun*/ 0x18};
const Song GroundLevelPart1Hdr = Song{0x18, GroundM_P1Data_data, 0x2d, 0x1c, 0xb8};
const Song GroundLevelPart2AHdr = Song{0x18, GroundM_P2AData_data, 0x20, 0x12, 0x70};
const Song GroundLevelPart2BHdr = Song{0x18, GroundM_P2BData_data, 0x1b, 0x10, 0x44};
const Song GroundLevelPart2CHdr = Song{0x18, GroundM_P2CData_data, 0x11, 0x0a, 0x1c};
const Song GroundLevelPart3AHdr = Song{0x18, GroundM_P3AData_data, 0x2d, 0x10, 0x58};
const Song GroundLevelPart3BHdr = Song{0x18, GroundM_P3BData_data, 0x14, 0x0d, 0x3f};
const Song GroundLevelLeadInHdr = Song{0x18, GroundMLdInData_data, 0x15, 0x0d, 0x21};
const Song GroundLevelPart4AHdr = Song{0x18, GroundM_P4AData_data, 0x18, 0x10, 0x7a};
const Song GroundLevelPart4BHdr = Song{0x18, GroundM_P4BData_data, 0x19, 0x0f, 0x54};
const Song GroundLevelPart4CHdr = Song{0x18, GroundM_P4CData_data, 0x1e, 0x12, 0x2b};
const Song DeathMusHdr = Song{0x18, DeathMusData_data, 0x1e, 0x0f, 0x2d};

const Song songs[] = {
    DeathMusHdr,          GameOverMusHdr,       VictoryMusHdr,        WinCastleMusHdr,      GameOverMusHdr,       EndOfLevelMusHdr,
    TimeRunningOutHdr,    SilenceHdr,           GroundLevelPart1Hdr,  WaterMusHdr,          UndergroundMusHdr,    CastleMusHdr,
    Star_CloudHdr,        GroundLevelLeadInHdr, Star_CloudHdr,        SilenceHdr,

    GroundLevelLeadInHdr, GroundLevelPart1Hdr,  GroundLevelPart1Hdr,  GroundLevelPart2AHdr, GroundLevelPart2BHdr, GroundLevelPart2AHdr,
    GroundLevelPart2CHdr, GroundLevelPart2AHdr, GroundLevelPart2BHdr, GroundLevelPart2AHdr, GroundLevelPart2CHdr, GroundLevelPart3AHdr,
    GroundLevelPart3BHdr, GroundLevelPart3AHdr, GroundLevelLeadInHdr, GroundLevelPart1Hdr,  GroundLevelPart1Hdr,  GroundLevelPart4AHdr,
    GroundLevelPart4BHdr, GroundLevelPart4AHdr, GroundLevelPart4CHdr, GroundLevelPart4AHdr, GroundLevelPart4BHdr, GroundLevelPart4AHdr,
    GroundLevelPart4CHdr, GroundLevelPart3AHdr, GroundLevelPart3BHdr, GroundLevelPart3AHdr, GroundLevelLeadInHdr, GroundLevelPart4AHdr,
    GroundLevelPart4BHdr, GroundLevelPart4AHdr, GroundLevelPart4CHdr};

// Dump the control-register contents into square 1's control regs.
void SMBEngine::Dump_Squ1_Regs(uint8_t ctrlByte, uint8_t sweepByte)
{
    writeData(SND_SQUARE1_REG + 1, sweepByte);
    writeData(SND_SQUARE1_REG, ctrlByte);
}

// Dump the control-register contents into square 2's control regs.
void SMBEngine::Dump_Sq2_Regs(uint8_t ctrlByte, uint8_t sweepByte)
{
    writeData(SND_SQUARE2_REG, ctrlByte);
    writeData(SND_SQUARE2_REG + 1, sweepByte);
}

// Pick the square 2 control value from the active music (reads Event/AreaMusicBuffer).
// The caller dumps this with the fixed reg contents x = 0x82, y = 0x7f.
uint8_t SMBEngine::LoadControlRegs()
{
    // check secondary buffer for win castle music
    if ((M(EventMusicBuffer) & EndOfCastleMusic) != 0)
    {
        return 0x04; // this value is only used for win castle music
    }
    // check primary buffer for water music
    if ((M(AreaMusicBuffer) & 0b01111101) != 0)
    {
        return 0x08; // this is the default value for all other music
    }
    return 0x28; // this value is used for water music and all other event music
}

// Load an envelope byte at the given table offset, picking the table from the active music.
uint8_t SMBEngine::LoadEnvelopeData(uint8_t offset)
{
    // EndOfCastleMusicEnvData
    //
    const uint8_t EndOfCastleMusicEnvData_data[] = {0x98, 0x99, 0x9a, 0x9b};

    // AreaMusicEnvData
    //
    const uint8_t AreaMusicEnvData_data[] = {0x90, 0x94, 0x94, 0x95, 0x95, 0x96, 0x97, 0x98};

    // WaterEventMusEnvData
    //
    const uint8_t WaterEventMusEnvData_data[] = {0x90, 0x91, 0x92, 0x92, 0x93, 0x93, 0x93, 0x94, 0x94, 0x94, 0x94, 0x94, 0x94, 0x95,
                                                 0x95, 0x95, 0x95, 0x95, 0x95, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96,
                                                 0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x96, 0x95, 0x95, 0x94, 0x93};

    // check secondary buffer for win castle music
    if ((M(EventMusicBuffer) & EndOfCastleMusic) != 0)
    {
        return EndOfCastleMusicEnvData_data[offset]; // load data from offset for win castle music
    }
    // check primary buffer for water music
    if ((M(AreaMusicBuffer) & 0b01111101) != 0)
    {
        return AreaMusicEnvData_data[offset]; // load default data from offset for all other music
    }
    return WaterEventMusEnvData_data[offset]; // data for water music and all other event music
}

// Inputs: rawByte = raw length/note byte. Returns the note length.
uint8_t SMBEngine::AlternateLengthHandler(uint8_t rawByte)
{
    // turn xx00000x into 00000xxx, with d0 of the original as the MSB here
    uint8_t lengthCode = ((rawByte & 0x01) << 2) | ((rawByte >> 6) & 0x03);
    return ProcessLengthData(lengthCode);
}

// Inputs: lengthCode = raw length code (low 3 bits used); also reads NoteLenLookupTblOfs and
// NoteLengthTblAdder. Returns the length value from MusicLengthLookupTbl_data.
uint8_t SMBEngine::ProcessLengthData(uint8_t lengthCode)
{
    const uint8_t MusicLengthLookupTbl_data[] = {0x05, 0x0a, 0x14, 0x28, 0x50, 0x1e, 0x3c, 0x02, 0x04, 0x08, 0x10, 0x20,
                                                 0x40, 0x18, 0x30, 0x0c, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x12, 0x24, 0x08,
                                                 0x36, 0x03, 0x09, 0x06, 0x12, 0x1b, 0x24, 0x0c, 0x24, 0x02, 0x06, 0x04,
                                                 0x0c, 0x12, 0x18, 0x08, 0x12, 0x01, 0x03, 0x02, 0x06, 0x09, 0x0c, 0x04};

    uint8_t index = (lengthCode & 0b00000111) // clear all but the three LSBs
                    + M(NoteLenLookupTblOfs)  // add offset loaded from first header byte
                    + M(NoteLengthTblAdder);  // add extra if time running out music
    return MusicLengthLookupTbl_data[index];
}

// Set ctrl regs for square 1, then set its frequency regs.
uint8_t SMBEngine::PlaySqu1Sfx(uint8_t freqIndex, uint8_t ctrlByte, uint8_t sweepByte)
{
    Dump_Squ1_Regs(ctrlByte, sweepByte);
    return SetFreq_Squ1(freqIndex);
}

uint8_t SMBEngine::SetFreq_Squ1(uint8_t freqIndex)
{
    // set frequency reg offset for square 1 sound channel
    return Dump_Freq_Regs(freqIndex, 0x00);
}

// Inputs: freqIndex = frequency table index; channelOffset = sound register channel offset
// (0x00 square 1, 0x04 square 2, 0x08 triangle)
// Returns: the tone's high frequency byte (with length-counter bit), or 0 if the note is a rest.
uint8_t SMBEngine::Dump_Freq_Regs(uint8_t freqIndex, uint8_t channelOffset)
{
    const uint16_t FreqRegLookupTbl_data[] = {
        // These are NES APU clock "divider" values, which can be
        // converted to a frequency using this formula:
        //
        //    Hz = 1789773 / (16 * (t+1))
        //
        0x0088, // 00 = G#4-30¢
        0x002f, // 02 =  D6-14¢
        0x0000, // 04 = silence
        0x02a6, // 06 =  E2-01¢
        0x0280, // 08 =  F2-01¢
        0x025c, // 0a = F#2-01¢
        0x023a, // 0c =  G2-01¢
        0x021a, // 0e = G#2-01¢
        0x01df, // 10 = A#2+00¢
        0x01c4, // 12 =  B2+00¢
        0x01ab, // 14 =  C3-02¢
        0x0193, // 16 = C#3-02¢
        0x017c, // 18 =  D3+00¢
        0x0167, // 1a = D#3-02¢
        0x0153, // 1c =  E3-03¢
        0x0140, // 1e =  F3-04¢
        0x012e, // 20 = F#3-04¢
        0x011d, // 22 =  G3-04¢
        0x010d, // 24 = G#3-04¢
        0x00fe, // 26 =  A3-05¢
        0x00ef, // 28 = A#3+00¢
        0x00e2, // 2a =  B3-04¢
        0x00d5, // 2c =  C4-02¢
        0x00c9, // 2e = C#4-02¢
        0x00be, // 30 =  D4-05¢
        0x00b3, // 32 = D#4-02¢
        0x00a9, // 34 =  E4-03¢
        0x00a0, // 36 =  F4-09¢
        0x0097, // 38 = F#4-10¢
        0x008e, // 3a =  G4-04¢
        0x0086, // 3c = G#4-04¢
        0x0077, // 3e = A#4+00¢
        0x007e, // 40 =  A4+02¢
        0x0071, // 42 =  B4-11¢
        0x0054, // 44 =  E5-03¢
        0x0064, // 46 = C#5-02¢
        0x005f, // 48 =  D5-14¢
        0x0059, // 4a = D#5-02¢
        0x0050, // 4c =  F5-20¢
        0x0047, // 4e =  G5-16¢
        0x0043, // 50 = G#5-17¢
        0x003b, // 52 = A#5+00¢
        0x0035, // 54 =  C6-18¢
        0x002a, // 56 =  E6-24¢
        0x0023, // 58 =  G6-16¢
        0x0475, // 5a =  G1-01¢
        0x0357, // 5c =  C2-02¢
        0x02f9, // 5e =  D2+00¢
        0x02cf, // 60 = D#2-02¢
        0x01fc, // 62 =  A2-02¢
        0x006a, // 64 =  C5-02¢
    };

    uint16_t divider = FreqRegLookupTbl_data[freqIndex / 2];
    if (divider == 0)
    {
        return 0; // if zero, then do not load
    }
    writeData(SND_REGISTER + 2 + channelOffset, divider & 0xff);
    uint8_t toneHi = (divider >> 8) | 0b00001000; // length counter bit
    writeData(SND_REGISTER + 3 + channelOffset, toneHi);
    return toneHi;
}

// Set ctrl regs for square 2, then set its frequency regs.
uint8_t SMBEngine::PlaySqu2Sfx(uint8_t freqIndex, uint8_t ctrlByte, uint8_t sweepByte)
{
    Dump_Sq2_Regs(ctrlByte, sweepByte);
    return SetFreq_Squ2(freqIndex);
}

uint8_t SMBEngine::SetFreq_Squ2(uint8_t freqIndex)
{
    // set frequency reg offset for square 2 sound channel
    return Dump_Freq_Regs(freqIndex, 0x04);
}

uint8_t SMBEngine::SetFreq_Tri(uint8_t freqIndex)
{
    // set frequency reg offset for triangle sound channel
    return Dump_Freq_Regs(freqIndex, 0x08);
}

// Load beat data directly into the noise regs.
void SMBEngine::PlayBeat(uint8_t noiseCtrl, uint8_t noiseLow, uint8_t noiseHigh)
{
    writeData(SND_NOISE_REG, noiseCtrl);
    writeData(SND_NOISE_REG + 2, noiseLow);
    writeData(SND_NOISE_REG + 3, noiseHigh);

    // ExitMusicHandler
}

// Inputs: ctrlByte, sweepByte = square 1 control register contents (forwarded to Dump_Squ1_Regs)
void SMBEngine::DmpJpFPS(uint8_t ctrlByte, uint8_t sweepByte)
{
    Dump_Squ1_Regs(ctrlByte, sweepByte);
    DecJpFPS(); // unconditional branch outta here
}

// unconditional branch, however we got here
// Inputs: none
// Outputs: none
void SMBEngine::DecJpFPS() { BranchToDecLength1(); }

// Inputs: none
// Outputs: none
void SMBEngine::BranchToDecLength1()
{
    DecrementSfx1Length(); // unconditional branch (regardless of how we got here)
}

// Inputs: none (reads/writes Squ1_SfxLenCounter memory)
// Outputs: none
void SMBEngine::DecrementSfx1Length()
{
    --M(Squ1_SfxLenCounter); // decrement length of sfx
    if (M(Squ1_SfxLenCounter) != 0)
    {
        return;
    }
    StopSquare1Sfx();
}

void SMBEngine::StopSquare1Sfx()
{
    // if end of sfx reached, clear buffer
    writeData(0xf1, 0x00); // and stop making the sfx
    writeData(SND_MASTERCTRL_REG, 0x0e);
    writeData(SND_MASTERCTRL_REG, 0x0f);
}

// Inputs: length = value to store to Squ2_SfxLenCounter; ctrlByte = square 2 control register
// partial contents (forwarded to PlaySqu2Sfx)
void SMBEngine::CGrab_TTickRegL(uint8_t length, uint8_t ctrlByte)
{
    writeData(Squ2_SfxLenCounter, length);
    // load the rest of reg contents of coin grab and timer tick sound
    PlaySqu2Sfx(0x42, ctrlByte, 0x7f);
    ContinueCGrabTTick();
}

// Inputs: none (reads Squ2_SfxLenCounter memory)
// Outputs: none
void SMBEngine::ContinueCGrabTTick()
{
    if (M(Squ2_SfxLenCounter) == 0x30) // check for time to play second tone yet
    {
        writeData(SND_SQUARE2_REG + 2, 0x54); // if so, load the tone directly into the reg
    } // N2Tone
    DecrementSfx2Length(); // unconditional branch, however we got here
}

// Inputs: freqIndex = frequency table index (forwarded to Dump_Freq_Regs); ctrlByte, sweepByte =
// square 2 control register contents (forwarded to Dump_Sq2_Regs)
void SMBEngine::LoadSqu2Regs(uint8_t freqIndex, uint8_t ctrlByte, uint8_t sweepByte)
{
    PlaySqu2Sfx(freqIndex, ctrlByte, sweepByte);
    DecrementSfx2Length();
}

// Inputs: none (reads/writes Squ2_SfxLenCounter memory)
// Outputs: none
void SMBEngine::DecrementSfx2Length()
{
    --M(Squ2_SfxLenCounter); // decrement length of sfx
    if (M(Squ2_SfxLenCounter) != 0)
    {
        return;
    }
    EmptySfx2Buffer();
}

void SMBEngine::EmptySfx2Buffer()
{
    writeData(Square2SoundBuffer, 0x00); // initialize square 2's sound effects buffer
    StopSquare2Sfx();
}

void SMBEngine::StopSquare2Sfx()
{
    // stop playing the sfx
    writeData(SND_MASTERCTRL_REG, 0x0d);
    writeData(SND_MASTERCTRL_REG, 0x0f);
}

// the flagpole slide sound shares part of third part
// Inputs: ctrlByte = square 1 control register partial contents (forwarded to DmpJpFPS)
void SMBEngine::FPS2nd(uint8_t ctrlByte) { DmpJpFPS(ctrlByte, 0xbc); }

// Inputs: freqIndex = frequency table index for the first part of mario's jumping sound.
void SMBEngine::JumpRegContents(uint8_t freqIndex)
{
    // note that small and big jump borrow each others' reg contents
    PlaySqu1Sfx(freqIndex, 0x82, 0xa7);
    writeData(Squ1_SfxLenCounter, 0x28); // store length of sfx for both jumping sounds
    ContinueSndJump();
}

// Inputs: none (reads Squ1_SfxLenCounter memory)
// Outputs: none
void SMBEngine::ContinueSndJump()
{
    uint8_t length = M(Squ1_SfxLenCounter); // jumping sounds seem to be composed of three parts
    if (length == 0x25)
    {
        DmpJpFPS(0x5f, 0xf6); // load second part
        return;
    } // N2Prt: check for third part
    if (length != 0x20)
    {
        DecJpFPS();
        return;
    }
    FPS2nd(0x48); // load third part
}

// the fireball sound shares reg contents with the bump sound
// Inputs: length = value to store to Squ1_SfxLenCounter; sweepByte = square 1 sweep reg contents
void SMBEngine::Fthrow(uint8_t length, uint8_t sweepByte)
{
    writeData(Squ1_SfxLenCounter, length);
    PlaySqu1Sfx(0x0c, 0x9e, sweepByte); // load offset for bump sound
    ContinueBumpThrow();
}

// Inputs: none (reads Squ1_SfxLenCounter memory)
// Outputs: none
void SMBEngine::ContinueBumpThrow()
{
    if (M(Squ1_SfxLenCounter) != 0x06) // check for second part of bump sound
    {
        DecJpFPS();
        return;
    }
    writeData(SND_SQUARE1_REG + 1, 0xbb); // load second part directly
    DecJpFPS();
}

// Inputs: none (reads Squ1_SfxLenCounter memory)
// Outputs: none
void SMBEngine::ContinueSwimStomp()
{
    const uint8_t SwimStompEnvelopeData_data[] = {0x9f, 0x9b, 0x98, 0x96, 0x95, 0x94, 0x92, 0x90, 0x90, 0x9a, 0x97, 0x95, 0x93, 0x92};

    uint8_t length = M(Squ1_SfxLenCounter); // look up reg contents in data section based on
    // length of sound left, used to control sound's envelope
    writeData(SND_SQUARE1_REG, SwimStompEnvelopeData_data[length - 1]);
    if (length != 0x06)
    {
        BranchToDecLength1();
        return;
    }
    // when the length counts down to a certain point, put this
    // directly into the LSB of square 1's frequency divider
    writeData(SND_SQUARE1_REG + 2, 0x9e);
    BranchToDecLength1();
}

// Inputs: none (reads Squ1_SfxLenCounter memory)
// Outputs: none
void SMBEngine::ContinueSmackEnemy()
{
    uint8_t tone = 0x90;               // SmSpc: this creates spaces in the sound, giving it its distinct noise
    if (M(Squ1_SfxLenCounter) == 0x08) // check about halfway through
    {
        // if we're at the about-halfway point, make the second tone
        writeData(SND_SQUARE1_REG + 2, 0xa0); // in the smack enemy sound
        tone = 0x9f;
    }

    writeData(SND_SQUARE1_REG, tone); // SmTick
    DecrementSfx1Length();
}

// Inputs: none (reads Squ1_SfxLenCounter memory)
// Outputs: none
void SMBEngine::ContinuePipeDownInj()
{
    // The regs are written to only during six specific times, during which d3 must be
    // set and d1-0 must be clear.
    if ((M(Squ1_SfxLenCounter) & 0b00001011) == 0b00001000)
    {
        PlaySqu1Sfx(0x44, 0x9a, 0x91); // and this is where it actually gets written in
    }
    DecrementSfx1Length();
}

// Inputs: none
// Outputs: none
void SMBEngine::Square1SfxHandler()
{
    uint8_t queued = M(Square1SoundQueue); // check for sfx in queue
    if (queued != 0)
    {
        writeData(Square1SoundBuffer, queued); // if found, put in buffer
        if ((queued & Sfx_SmallJump) != 0)
        {
            JumpRegContents(0x26); // branch here for small mario jumping sound
            return;
        }
        if ((queued & Sfx_BigJump) != 0)
        {
            JumpRegContents(0x18); // branch here for big mario jumping sound
            return;
        }
        if ((queued & Sfx_Bump) != 0)
        {
            Fthrow(0x0a, 0x93); // load length of sfx and reg contents for bump sound
            return;
        }
        if ((queued & Sfx_EnemyStomp) != 0)
        {
            writeData(Squ1_SfxLenCounter, 0x0e); // store length of swim/stomp sound
            PlaySqu1Sfx(0x26, 0x9e, 0x9c);       // store reg contents for swim/stomp sound
            ContinueSwimStomp();
            return;
        }
        if ((queued & Sfx_EnemySmack) != 0)
        {
            writeData(Squ1_SfxLenCounter, 0x0e); // store length of smack enemy sound
            // store reg contents for smack enemy sound
            uint8_t tone = PlaySqu1Sfx(0x28, 0x9f, 0xcb);
            if (tone != 0)
            {
                DecrementSfx1Length(); // unconditional branch
                return;
            }
            ContinueSmackEnemy();
            return;
        }
        if ((queued & Sfx_PipeDown_Injury) != 0)
        {
            writeData(Squ1_SfxLenCounter, 0x2f); // load length of pipedown sound
            ContinuePipeDownInj();
            return;
        }
        if ((queued & Sfx_Fireball) != 0)
        {
            Fthrow(0x05, 0x99); // load reg contents for fireball throw sound
            return;
        }
        if ((queued & Sfx_Flagpole) != 0)
        {
            writeData(Squ1_SfxLenCounter, 0x40); // store length of flagpole sound
            SetFreq_Squ1(0x62);                  // load part of reg contents for flagpole sound
            FPS2nd(0x99);                        // now load the rest
            return;
        }
    } // CheckSfx1Buffer
    uint8_t buffered = M(Square1SoundBuffer); // check for sfx in buffer (if not found, exit sub)
    if ((buffered & Sfx_SmallJump) != 0)
    {
        ContinueSndJump(); // small mario jump
    }
    else if ((buffered & Sfx_BigJump) != 0)
    {
        ContinueSndJump(); // big mario jump
    }
    else if ((buffered & Sfx_Bump) != 0)
    {
        ContinueBumpThrow(); // bump
    }
    else if ((buffered & Sfx_EnemyStomp) != 0)
    {
        ContinueSwimStomp(); // swim/stomp
    }
    else if ((buffered & Sfx_EnemySmack) != 0)
    {
        ContinueSmackEnemy(); // smack enemy
    }
    else if ((buffered & Sfx_PipeDown_Injury) != 0)
    {
        ContinuePipeDownInj(); // pipedown/injury
    }
    else if ((buffered & Sfx_Fireball) != 0)
    {
        ContinueBumpThrow(); // fireball throw
    }
    else if ((buffered & Sfx_Flagpole) != 0)
    {
        DecrementSfx1Length(); // slide flagpole
    } // ExS1H
}

// Inputs: none. Second part of the fireworks/gunfire sound (shares the PBFRegs endpoint).
void SMBEngine::ContinueBlast()
{
    if (M(Squ2_SfxLenCounter) != 0x18) // check for time to play second part
    {
        DecrementSfx2Length();
        return;
    }
    LoadSqu2Regs(0x18, 0x9f, 0x93); // load second part reg contents
}

// Inputs: none. Second part of the bowser defeat sound (shares the PBFRegs endpoint).
void SMBEngine::ContinueBowserFall()
{
    if (M(Squ2_SfxLenCounter) != 0x08) // check for almost near the end
    {
        DecrementSfx2Length();
        return;
    }
    LoadSqu2Regs(0x5a, 0x9f, 0xa4); // load the rest of reg contents for bowser defeat sound
}

// Inputs: none. Steps the power-up grab sound.
void SMBEngine::ContinuePowerUpGrab()
{
    const uint8_t PowerUpGrabFreqData_data[] = {0x4c, 0x52, 0x4c, 0x48, 0x3e, 0x36, 0x3e, 0x36, 0x30, 0x28, 0x4a, 0x50, 0x4a, 0x64, 0x3c,
                                                0x32, 0x3c, 0x32, 0x2c, 0x24, 0x3a, 0x64, 0x3a, 0x34, 0x2c, 0x22, 0x2c, 0x22, 0x1c, 0x14};

    uint8_t counter = M(Squ2_SfxLenCounter);
    if ((counter & 0x01) != 0)
    {
        DecrementSfx2Length(); // alter frequency every other frame
        return;
    }
    // load frequency reg based on length left over / 2; store reg contents of power-up grab sound
    uint8_t offset = counter >> 1;
    LoadSqu2Regs(PowerUpGrabFreqData_data[offset - 1], 0x5d, 0x7f);
}

// Inputs: none. Steps the 1-up sound, loading new tones only every eight frames.
void SMBEngine::ContinueExtraLife()
{
    const uint8_t ExtraLifeFreqData_data[] = {0x58, 0x02, 0x54, 0x56, 0x4e, 0x44};

    uint8_t counter = M(Squ2_SfxLenCounter);
    if ((counter & 0b00000111) != 0)
    {
        DecrementSfx2Length(); // JumpToDecLength2: not yet time for a new tone
        return;
    }
    uint8_t offset = counter >> 3;
    LoadSqu2Regs(ExtraLifeFreqData_data[offset - 1], 0x82, 0x7f); // load our reg contents
}

// Inputs: none. Steps the power-up reveal / vine grow sound off the secondary counter.
void SMBEngine::ContinueGrowItems()
{
    const uint8_t PUp_VGrow_FreqData_data[] = {0x14, 0x04, 0x22, 0x24, 0x16, 0x04, 0x24, 0x26, // used by both
                                               0x18, 0x04, 0x26, 0x28, 0x1a, 0x04, 0x28, 0x2a,
                                               0x1c, 0x04, 0x2a, 0x2c, 0x1e, 0x04, 0x2c, 0x2e, // used by vinegrow
                                               0x20, 0x04, 0x2e, 0x30, 0x22, 0x04, 0x30, 0x32};

    ++M(Sfx_SecondaryCounter);                     // increment secondary counter for both sounds
    uint8_t offset = M(Sfx_SecondaryCounter) >> 1; // this sound doesn't decrement the usual counter
    if (offset != M(Squ2_SfxLenCounter))
    {
        writeData(SND_SQUARE2_REG, 0x9d);              // load contents of other reg directly
        SetFreq_Squ2(PUp_VGrow_FreqData_data[offset]); // secondary counter / 2 as frequency offset
        return;
    }
    EmptySfx2Buffer(); // StopGrowItems: branch to stop playing sounds
}

// Inputs: length = length of the reveal/grow sound. Loads the shared reg contents and steps it.
void SMBEngine::GrowItemRegs(uint8_t length)
{
    writeData(Squ2_SfxLenCounter, length);
    writeData(SND_SQUARE2_REG + 1, 0x7f);  // load contents of reg for both sounds directly
    writeData(Sfx_SecondaryCounter, 0x00); // start secondary counter for both sounds
    ContinueGrowItems();
}

// Inputs: none
// Outputs: none
void SMBEngine::Square2SfxHandler()
{
    // special handling for the 1-up sound to keep it from being interrupted by
    // other sounds on square 2
    if ((M(Square2SoundBuffer) & Sfx_ExtraLife) != 0)
    {
        ContinueExtraLife();
        return;
    }
    uint8_t queued = M(Square2SoundQueue); // check for sfx in queue
    if (queued != 0)
    {
        writeData(Square2SoundBuffer, queued); // if found, put in buffer and check for the following
        if ((queued & Sfx_BowserFall) != 0)
        {
            writeData(Squ2_SfxLenCounter, 0x38); // load length of bowser defeat sound
            LoadSqu2Regs(0x18, 0x9f, 0xc4);      // inlined PlayBowserFall: load reg contents
            return;
        }
        if ((queued & Sfx_CoinGrab) != 0)
        {
            CGrab_TTickRegL(0x35, 0x8d); // inlined PlayCoinGrab: length + part of reg contents
            return;
        }
        if ((queued & Sfx_GrowPowerUp) != 0)
        {
            GrowItemRegs(0x10); // power-up reveal
            return;
        }
        if ((queued & Sfx_GrowVine) != 0)
        {
            GrowItemRegs(0x20); // vine grow
            return;
        }
        if ((queued & Sfx_Blast) != 0)
        {
            writeData(Squ2_SfxLenCounter, 0x20); // load length of fireworks/gunfire sound
            LoadSqu2Regs(0x5e, 0x9f, 0x94);      // inlined PlayBlast: load reg contents
            return;
        }
        if ((queued & Sfx_TimerTick) != 0)
        {
            CGrab_TTickRegL(0x06, 0x98); // inlined PlayTimerTick: length + part of reg contents
            return;
        }
        if ((queued & Sfx_PowerUpGrab) != 0)
        {
            writeData(Squ2_SfxLenCounter, 0x36); // load length of power-up grab sound
            ContinuePowerUpGrab();
            return;
        }
        if ((queued & Sfx_ExtraLife) != 0)
        {
            writeData(Squ2_SfxLenCounter, 0x30); // load length of 1-up sound
            ContinueExtraLife();
            return;
        }
    } // CheckSfx2Buffer
    uint8_t buffered = M(Square2SoundBuffer); // check for sfx in buffer (if not found, exit sub)
    if ((buffered & Sfx_BowserFall) != 0)
    {
        ContinueBowserFall(); // bowser fall
    }
    else if ((buffered & Sfx_CoinGrab) != 0)
    {
        ContinueCGrabTTick(); // coin grab
    }
    else if ((buffered & Sfx_GrowPowerUp) != 0)
    {
        ContinueGrowItems(); // power-up reveal
    }
    else if ((buffered & Sfx_GrowVine) != 0)
    {
        ContinueGrowItems(); // vine grow
    }
    else if ((buffered & Sfx_Blast) != 0)
    {
        ContinueBlast(); // fireworks/gunfire
    }
    else if ((buffered & Sfx_TimerTick) != 0)
    {
        ContinueCGrabTTick(); // timer tick
    }
    else if ((buffered & Sfx_PowerUpGrab) != 0)
    {
        ContinuePowerUpGrab(); // power-up grab
    }
    else if ((buffered & Sfx_ExtraLife) != 0)
    {
        ContinueExtraLife(); // 1-up
    } // ExS2H
}

// Inputs: none
// Outputs: none
void SMBEngine::MiscSqu2MusicTasks()
{
    // load default envelope for square 2 unless a sound is playing on square 2, or death
    // music / d4 is set on the secondary buffer (those load their regs by default)
    if (M(Square2SoundBuffer) == 0 && (M(EventMusicBuffer) & 0b10010001) == 0)
    {
        uint8_t offset = M(Squ2_EnvelopeDataCtrl); // envelope offset saved from LoadControlRegs
        if (offset != 0)
        {
            --M(Squ2_EnvelopeDataCtrl); // NoDecEnv1: decrement unless already zero
        }
        writeData(SND_SQUARE2_REG, LoadEnvelopeData(offset)); // replace default envelope
        writeData(SND_SQUARE2_REG + 1, 0x7f);
    }
    HandleSquare1Music();
}

// Inputs: none
// Outputs: none
void SMBEngine::HandleSquare1Music()
{
    if (M(MusicOffset_Square1) == 0) // is there a nonzero offset here?
    {
        HandleTriangleMusic(); // if not, skip ahead to the triangle channel
        return;
    }
    --M(Squ1_NoteLenCounter);        // decrement square 1 note length
    if (M(Squ1_NoteLenCounter) == 0) // is it time for more data?
    {
        uint8_t rawByte;
        for (;;) // FetchSqu1MusicData
        {
            uint8_t offset = M(MusicOffset_Square1); // fetch data and advance the offset
            ++M(MusicOffset_Square1);
            rawByte = musicData[offset];
            if (rawByte != 0)
            {
                break; // if nonzero, then skip this part
            }
            // store some data into control regs for square 1 and fetch another byte, used
            // to give death music its unique sound
            writeData(SND_SQUARE1_REG, 0x83);
            writeData(SND_SQUARE1_REG + 1, 0x94);
            writeData(AltRegContentFlag, 0x94);
        } // Squ1NoteHandler
        writeData(Squ1_NoteLenCounter, AlternateLengthHandler(rawByte)); // save note length
        if (M(Square1SoundBuffer) != 0)                                  // is there a sound playing on square 1?
        {
            HandleTriangleMusic();
            return;
        }
        uint8_t note = rawByte & 0b00111110; // change saved data to appropriate note format
        uint8_t tone = SetFreq_Squ1(note);   // play the note (0 if a rest)
        // In the rest case the dump reuses the freq-reg scratch: channel offset 0x00 and the note.
        uint8_t ctrlByte = 0x00;
        uint8_t sweepByte = note;
        uint8_t envCtrl = tone;
        if (tone != 0)
        {
            envCtrl = LoadControlRegs(); // SkipCtrlL
            ctrlByte = 0x82;
            sweepByte = 0x7f;
        }
        writeData(Squ1_EnvelopeDataCtrl, envCtrl); // save envelope offset
        Dump_Squ1_Regs(ctrlByte, sweepByte);
    } // MiscSqu1MusicTasks
    if (M(Square1SoundBuffer) != 0) // is there a sound playing on square 1?
    {
        HandleTriangleMusic();
        return;
    }
    // check for death music or d4 set on secondary buffer
    if ((M(EventMusicBuffer) & 0b10010001) == 0)
    {
        uint8_t offset = M(Squ1_EnvelopeDataCtrl); // check saved envelope offset
        if (offset != 0)
        {
            --M(Squ1_EnvelopeDataCtrl); // NoDecEnv2: decrement unless already zero
        }
        writeData(SND_SQUARE1_REG, LoadEnvelopeData(offset)); // load envelope data
    } // DeathMAltReg: check for alternate control reg data
    uint8_t altReg = M(AltRegContentFlag);
    if (altReg == 0)
    {
        altReg = 0x7f; // load this value if zero, the alternate value if nonzero
    } // DoAltLoad
    writeData(SND_SQUARE1_REG + 1, altReg);
    HandleTriangleMusic();
}

// Inputs: none
// Outputs: none
void SMBEngine::HandleTriangleMusic()
{
    --M(Tri_NoteLenCounter);        // decrement triangle note length
    if (M(Tri_NoteLenCounter) != 0) // is it time for more data?
    {
        HandleNoiseMusic();
        return;
    }
    uint8_t offset = M(MusicOffset_Triangle); // increment music offset and fetch data
    ++M(MusicOffset_Triangle);
    uint8_t data = musicData[offset];
    if (data == 0)
    {
        writeData(SND_TRIANGLE_REG, data); // LoadTriCtrlReg: skip all this and move on to noise
        HandleNoiseMusic();
        return;
    }
    if ((data & 0x80) != 0) // if bit 7 set, data is length data
    {
        writeData(Tri_NoteLenBuffer, ProcessLengthData(data)); // save the length
        writeData(SND_TRIANGLE_REG, 0x1f);                     // load some default data for triangle control reg
        offset = M(MusicOffset_Triangle);                      // fetch another byte
        ++M(MusicOffset_Triangle);
        data = musicData[offset];
        if (data == 0)
        {
            writeData(SND_TRIANGLE_REG, data); // LoadTriCtrlReg: check once more for nonzero data
            HandleNoiseMusic();
            return;
        }
    } // TriNoteHandler
    SetFreq_Tri(data);
    uint8_t noteLength = M(Tri_NoteLenBuffer); // save length in triangle note counter
    writeData(Tri_NoteLenCounter, noteLength);
    // if playing any other primary, or death or d4, go on to the noise routine; otherwise
    // (water or castle music, or any secondary) pick a control value for the triangle channel
    if ((M(EventMusicBuffer) & 0b01101110) == 0 && (M(AreaMusicBuffer) & 0b00001010) == 0)
    {
        HandleNoiseMusic();
        return;
    } // NotDOrD4
    uint8_t ctrl = 0xff; // LongN: value for a long note
    if (noteLength < 0x12)
    {
        // $0f for win castle music, else $1f for water/castle level music or any secondary
        ctrl = (M(EventMusicBuffer) & EndOfCastleMusic) != 0 ? 0x0f : 0x1f;
    }
    writeData(SND_TRIANGLE_REG, ctrl); // LoadTriCtrlReg: save into control reg for triangle
    HandleNoiseMusic();
}

// Inputs: none
// Outputs: none
void SMBEngine::HandleNoiseMusic()
{
    // check if playing underground or castle music
    if ((M(AreaMusicBuffer) & 0b11110011) == 0)
    {
        return; // if so, skip the noise routine
    }
    --M(Noise_BeatLenCounter);        // decrement noise beat length
    if (M(Noise_BeatLenCounter) != 0) // is it time for more data?
    {
        return;
    }
    uint8_t rawByte;
    uint8_t offset;
    for (;;) // FetchNoiseBeatData
    {
        offset = M(MusicOffset_Noise); // increment noise beat offset and fetch data
        ++M(MusicOffset_Noise);
        rawByte = musicData[offset];
        if (rawByte != 0)
        {
            break; // if nonzero, branch to handle
        }
        // if data is zero, reload original noise beat offset and loopback next time around
        writeData(MusicOffset_Noise, M(NoiseDataLoopbackOfs));
    } // NoiseBeatHandler
    writeData(Noise_BeatLenCounter, AlternateLengthHandler(rawByte)); // store length
    uint8_t beat = rawByte & 0b00111110;                              // reload data and erase length bits
    // The silent beat writes 0x10 to the noise control reg (volume 0), leaving the raw byte
    // and the fetch offset in the period regs as the original left X and Y.
    if (beat == 0)
    {
        PlayBeat(0x10, rawByte, offset); // SilentBeat: if no beat data, silence
    }
    else if (beat == 0x30)
    {
        PlayBeat(0x1c, 0x03, 0x58); // long beat data
    }
    else if (beat == 0x20)
    {
        PlayBeat(0x1c, 0x0c, 0x18); // strong beat data
    }
    else if ((beat & 0b00010000) == 0)
    {
        PlayBeat(0x10, rawByte, offset); // SilentBeat
    }
    else
    {
        PlayBeat(0x1c, 0x03, 0x18); // short beat data
    }
}

// Inputs: noiseEnv, noiseFreq = reg contents to play. Plays the sfx then steps its length.
void SMBEngine::PlayNoiseSfx(uint8_t noiseEnv, uint8_t noiseFreq)
{
    writeData(SND_NOISE_REG, noiseEnv); // play the sfx
    writeData(SND_NOISE_REG + 2, noiseFreq);
    writeData(SND_NOISE_REG + 3, 0x18);
    DecrementSfx3Length();
}

// Inputs: none (reads/writes Noise_SfxLenCounter memory)
void SMBEngine::DecrementSfx3Length()
{
    --M(Noise_SfxLenCounter); // decrement length of sfx
    if (M(Noise_SfxLenCounter) == 0)
    {
        // if done, stop playing the sfx
        writeData(SND_NOISE_REG, 0xf0);
        writeData(NoiseSoundBuffer, 0x00);
    } // ExSfx3
}

// Inputs: none. Steps the brick shatter sound, playing a tone every other frame.
void SMBEngine::ContinueBrickShatter()
{
    const uint8_t BrickShatterEnvData_data[] = {0x15, 0x16, 0x16, 0x17, 0x17, 0x18, 0x19, 0x19,
                                                0x1a, 0x1a, 0x1c, 0x1d, 0x1d, 0x1e, 0x1e, 0x1f};

    const uint8_t BrickShatterFreqData_data[] = {0x01, 0x0e, 0x0e, 0x0d, 0x0b, 0x06, 0x0c, 0x0f,
                                                 0x0a, 0x09, 0x03, 0x0d, 0x08, 0x0d, 0x06, 0x0c};

    uint8_t counter = M(Noise_SfxLenCounter);
    if ((counter & 0x01) == 0) // divide by 2 and check for bit set to use offset
    {
        DecrementSfx3Length();
        return;
    }
    uint8_t offset = counter >> 1;
    // load reg contents of brick shatter sound
    PlayNoiseSfx(BrickShatterEnvData_data[offset], BrickShatterFreqData_data[offset]);
}

// Inputs: none. Loads the brick shatter length and steps it.
void SMBEngine::PlayBrickShatter()
{
    writeData(Noise_SfxLenCounter, 0x20); // load length of brick shatter sound
    ContinueBrickShatter();
}

// Inputs: none. Steps the bowser flame sound; when its envelope table runs out it hands
// off to the brick shatter sound (as the original's fall-through did).
void SMBEngine::ContinueBowserFlame()
{
    const uint8_t BowserFlameEnvData_data[] = {
        0x93, // stray byte read by `lda BowserFlameEnvData-1,y`
        0x15, 0x16, 0x16, 0x17, 0x17, 0x18, 0x19, 0x19, 0x1a, 0x1a, 0x1c, 0x1d, 0x1d, 0x1e, 0x1e, 0x1f, 0x1f, 0x1f, 0x1f, 0x1e, 0x1d, 0x1c,
        0x1e, 0x1f, 0x1f, 0x1e, 0x1d, 0x1c, 0x1a, 0x18, 0x16, 0x14,
        // Brick shatter env data repeated here
        0x15, 0x16, 0x16, 0x17, 0x17, 0x18, 0x19, 0x19, 0x1a, 0x1a, 0x1c, 0x1d, 0x1d, 0x1e, 0x1e, 0x1f, 0};

    uint8_t offset = M(Noise_SfxLenCounter) >> 1;
    uint8_t env = BowserFlameEnvData_data[offset];
    if (env != 0)
    {
        PlayNoiseSfx(env, 0x0f); // load reg contents of bowser flame sound
        return;
    }
    PlayBrickShatter();
}

// Inputs: none
// Outputs: none
void SMBEngine::NoiseSfxHandler()
{
    uint8_t queued = M(NoiseSoundQueue); // check for sfx in queue
    if (queued != 0)
    {
        writeData(NoiseSoundBuffer, queued); // if found, put in buffer
        if ((queued & Sfx_BrickShatter) != 0)
        {
            PlayBrickShatter(); // brick shatter
            return;
        }
        if ((queued & Sfx_BowserFlame) != 0)
        {
            writeData(Noise_SfxLenCounter, 0x40); // PlayBowserFlame: load length
            ContinueBowserFlame();
            return;
        }
    } // CheckNoiseBuffer
    uint8_t buffered = M(NoiseSoundBuffer); // check for sfx in buffer (if not found, exit sub)
    if ((buffered & Sfx_BrickShatter) != 0)
    {
        ContinueBrickShatter(); // brick shatter
    }
    else if ((buffered & Sfx_BowserFlame) != 0)
    {
        ContinueBowserFlame(); // bowser flame
    } // ExNH
}

// Inputs: none
// Outputs: none
void SMBEngine::SoundEngine()
{
    if (M(OperMode) == 0) // are we in title screen mode?
    {
        writeData(SND_MASTERCTRL_REG, 0x00); // if so, disable sound and leave
        return;
    } // SndOn
    writeData(JOYPAD_PORT2, 0xff);       // disable irqs and set frame counter mode???
    writeData(SND_MASTERCTRL_REG, 0x0f); // enable first four channels

    // run the game sound routines unless sound is in pause mode or a pause sfx is queued
    if (M(PauseModeFlag) == 0 && M(PauseSoundQueue) != 0x01)
    {
        Square1SfxHandler();             // play sfx on square channel 1
        Square2SfxHandler();             //  ''  ''  '' square channel 2
        NoiseSfxHandler();               //  ''  ''  '' noise channel
        MusicHandler();                  // play music on all channels
        writeData(AreaMusicQueue, 0x00); // clear the music queues
        writeData(EventMusicQueue, 0x00);
    }
    else // InPause: the pause routine owns square 1 this frame
    {
        bool decPauseCounter = true;
        if (M(PauseSoundBuffer) == 0) // check pause sfx buffer
        {
            uint8_t queued = M(PauseSoundQueue); // check pause queue
            if (queued == 0)
            {
                decPauseCounter = false; // nothing queued
            }
            else
            {
                // queue full: store in buffer and activate pause mode to interrupt game sounds
                writeData(PauseSoundBuffer, queued);
                writeData(PauseModeFlag, queued);
                writeData(SND_MASTERCTRL_REG, 0x00); // disable sound and clear sfx buffers
                writeData(Square1SoundBuffer, 0x00);
                writeData(Square2SoundBuffer, 0x00);
                writeData(NoiseSoundBuffer, 0x00);
                writeData(SND_MASTERCTRL_REG, 0x0f); // enable sound again
                writeData(Squ1_SfxLenCounter, 0x2a); // store length of sound in pause counter
                PlaySqu1Sfx(0x44, 0x84, 0x7f);       // PTone1F: play first tone
            }
        }
        else // ContPau: check pause length left
        {
            // only load regs during specific times, otherwise skip
            uint8_t length = M(Squ1_SfxLenCounter);
            if (length == 0x24 || length == 0x18)
            {
                PlaySqu1Sfx(0x64, 0x84, 0x7f); // PTone2F: store reg contents and play the pause sfx
            }
            else if (length == 0x1e)
            {
                PlaySqu1Sfx(0x44, 0x84, 0x7f); // PTone1F
            }
        }
        if (decPauseCounter) // DecPauC: decrement pause sfx counter
        {
            --M(Squ1_SfxLenCounter);
            if (M(Squ1_SfxLenCounter) == 0)
            {
                // disable sound if in pause mode and not currently playing the pause sfx
                writeData(SND_MASTERCTRL_REG, 0x00);
                // if no longer playing pause sfx, allow game sounds again
                if (M(PauseSoundBuffer) == 0x02)
                {
                    writeData(PauseModeFlag, 0x00); // clear pause mode
                } // SkipPIn
                writeData(PauseSoundBuffer, 0x00); // clear pause sfx buffer
            }
        }
    }

    // SkipSoundSubroutines: clear the sound effects queues
    writeData(Square1SoundQueue, 0x00);
    writeData(Square2SoundQueue, 0x00);
    writeData(NoiseSoundQueue, 0x00);
    writeData(PauseSoundQueue, 0x00);

    uint8_t counter = M(DAC_Counter); // load some sort of counter
    bool skipDecrement = false;
    if ((M(AreaMusicBuffer) & 0b00000011) != 0) // check for specific music
    {
        ++M(DAC_Counter); // increment and check counter
        if (counter < 0x30)
        {
            skipDecrement = true; // if not there yet, just store it
        }
    } // NoIncDAC
    if (!skipDecrement && counter != 0) // if we are at zero, do not decrement
    {
        --M(DAC_Counter); // decrement counter
    }
    writeData(SND_DELTA_REG + 1, counter); // StrWave: store into DMC load register (??)
}

// Inputs: none
// Outputs: none
void SMBEngine::MusicHandler()
{
    // The music handler is a small state machine: loading a song header falls
    // through into playing square 2, and the end of a song can loop back to
    // either the header search or the area music setup.
    enum class Step
    {
        LoadEventMusic,
        LoadAreaMusic,
        HandleAreaMusicLoopB,
        FindEventMusicHeader,
        LoadHeader,
        HandleSquare2Music
    };
    Step step;

    a = M(EventMusicQueue); // check event music queue
    if (a != 0)
    {
        step = Step::LoadEventMusic;
    }
    else
    {
        a = M(AreaMusicQueue); // check area music queue
        if (a != 0)
        {
            step = Step::LoadAreaMusic;
        }
        else
        {
            a = M(EventMusicBuffer) | M(AreaMusicBuffer); // check both buffers
            if (a == 0)
            {
                return; // no music, then leave
            }
            step = Step::HandleSquare2Music; // if we have music, start with square 2 channel
        }
    }

    while (true)
    {
        switch (step)
        {
        case Step::LoadEventMusic:
            writeData(EventMusicBuffer, a); // copy event music queue contents to buffer
            if (a == DeathMusic)
            {                     // if not, jump elsewhere
                StopSquare1Sfx(); // stop sfx in square 1 and 2
                StopSquare2Sfx(); // but clear only square 1's sfx buffer
            } // NoStopSfx
            x = M(AreaMusicBuffer);
            writeData(AreaMusicBuffer_Alt, x); // save current area music buffer to be re-obtained later
            y = 0x00;
            writeData(NoteLengthTblAdder, 0x00); // default value for additional length byte offset
            writeData(AreaMusicBuffer, 0x00);    // clear area music buffer
            if (a == TimeRunningOutMusic)
            {
                x = 0x08; // load offset to be added to length byte of header
                writeData(NoteLengthTblAdder, 0x08);
            }
            step = Step::FindEventMusicHeader;
            break;

        case Step::LoadAreaMusic:
            if (a == 0x04)
            { // no, do not stop square 1 sfx
                StopSquare1Sfx();
            } // NoStop1: start counter used only by ground level music
            y = 0x10;
            // GMLoopB
            writeData(GroundMusicHeaderOfs, y);
            step = Step::HandleAreaMusicLoopB;
            break;

        case Step::HandleAreaMusicLoopB:
            y = 0x00; // clear event music buffer
            writeData(EventMusicBuffer, 0x00);
            writeData(AreaMusicBuffer, a); // copy area music queue contents to buffer
            if (a == 0x01)
            {
                ++M(GroundMusicHeaderOfs);   // increment but only if playing ground level music
                y = M(GroundMusicHeaderOfs); // is it time to loopback ground level music?
                if (y == 0x32)
                {
                    y = 0x11; // GMLoopB with alternate offset
                    writeData(GroundMusicHeaderOfs, y);
                    break; // stay in HandleAreaMusicLoopB
                }
                step = Step::LoadHeader;
                break;
            } // FindAreaMusicHeader
            y = 0x08;                             // load Y for offset of area music
            writeData(MusicOffset_Square2, 0x08); // residual instruction here
            step = Step::FindEventMusicHeader;
            break;

        case Step::FindEventMusicHeader:
            // increment Y pointer based on previously loaded queue contents,
            // bit shifting until we find a set bit for music
            {
                bool shiftedBit;
                do
                {
                    ++y;
                    shiftedBit = (a & 0x01) != 0;
                    a >>= 1;
                } while (!shiftedBit);
            }
            step = Step::LoadHeader;
            break;

        case Step::LoadHeader:
            // load offset for header
            {
                const Song song = songs[y - 1];
                // y = M(MusicHeaderOffsetData + y);
                // now load the header
                writeData(NoteLenLookupTblOfs, song.lengthByteOffset);
                musicData = song.data;
                writeData(MusicOffset_Triangle, song.bassOffset);
                writeData(MusicOffset_Square1, song.harmonyOffset);
                a = song.percussionOffset;
                writeData(MusicOffset_Noise, a);
                writeData(NoiseDataLoopbackOfs, a);
            }
            // initialize music note counters
            writeData(Squ2_NoteLenCounter, 0x01);
            writeData(Squ1_NoteLenCounter, 0x01);
            writeData(Tri_NoteLenCounter, 0x01);
            writeData(Noise_BeatLenCounter, 0x01);
            // initialize music data offset for square 2
            writeData(MusicOffset_Square2, 0x00);
            writeData(AltRegContentFlag, 0x00); // initialize alternate control reg data used by square 1
            // disable triangle channel and reenable it
            writeData(SND_MASTERCTRL_REG, 0x0b);
            a = 0x0f;
            writeData(SND_MASTERCTRL_REG, 0x0f);
            step = Step::HandleSquare2Music;
            break;

        case Step::HandleSquare2Music:
            --M(Squ2_NoteLenCounter); // decrement square 2 note length
            if (M(Squ2_NoteLenCounter) != 0)
            {
                MiscSqu2MusicTasks();
                return;
            }
            y = M(MusicOffset_Square2); // increment square 2 music offset and fetch data
            ++M(MusicOffset_Square2);
            a = musicData[y];
            if (a == 0) // if zero, the data is a null terminator
            {
                // EndOfMusicData
                a = M(EventMusicBuffer); // check secondary buffer for time running out music
                if (a == TimeRunningOutMusic)
                {
                    a = M(AreaMusicBuffer_Alt); // load previously saved contents of primary buffer
                    if (a != 0)
                    {
                        // MusicLoopBack: start playing the song again if there is one
                        step = Step::HandleAreaMusicLoopB;
                        break;
                    }
                } // NotTRO: check for victory music (the only secondary that loops)
                a &= VictoryMusic;
                if (a != 0)
                {
                    step = Step::LoadEventMusic;
                    break;
                }
                // check primary buffer for any music except pipe intro
                a = M(AreaMusicBuffer) & 0b01011111;
                if (a != 0)
                {
                    // MusicLoopBack: if any area music except pipe intro, music loops
                    step = Step::HandleAreaMusicLoopB;
                    break;
                }
                // clear primary and secondary buffers and initialize
                writeData(AreaMusicBuffer, 0x00); // control regs of square and triangle channels
                writeData(EventMusicBuffer, 0x00);
                writeData(SND_TRIANGLE_REG, 0x00);
                a = 0x90;
                writeData(SND_SQUARE1_REG, 0x90);
                writeData(SND_SQUARE2_REG, 0x90);
                return;
            }
            if ((a & 0x80) != 0)
            {
                // Squ2LengthHandler
                a = ProcessLengthData(a); // store length of note
                writeData(Squ2_NoteLenBuffer, a);
                y = M(MusicOffset_Square2); // fetch another byte (MUST NOT BE LENGTH BYTE!)
                ++M(MusicOffset_Square2);
                a = musicData[y];
            }
            // Squ2NoteHandler: is there a sound playing on this channel?
            if (M(Square2SoundBuffer) == 0)
            {
                uint8_t note = a;
                uint8_t tone = SetFreq_Squ2(note); // no, then play the note (0 if a rest)
                // In the rest case the dump reuses the freq-reg scratch: channel offset 0x04 and the note.
                uint8_t ctrlByte = 0x04;
                uint8_t sweepByte = note;
                uint8_t envCtrl = tone;
                if (tone != 0) // check to see if note is rest
                {
                    envCtrl = LoadControlRegs(); // if not, load control regs for square 2
                    ctrlByte = 0x82;
                    sweepByte = 0x7f;
                } // Rest: save contents of A
                writeData(Squ2_EnvelopeDataCtrl, envCtrl);
                Dump_Sq2_Regs(ctrlByte, sweepByte); // dump into square 2 control regs
            } // SkipFqL1: save length in square 2 note counter
            writeData(Squ2_NoteLenCounter, M(Squ2_NoteLenBuffer));
            MiscSqu2MusicTasks();
            return;
        }
    }
}
