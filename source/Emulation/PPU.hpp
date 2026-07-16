#ifndef PPU_HPP
#define PPU_HPP

#include <cstdint>

class SMBEngine;

/**
 * Everything the PPU changes as it runs, and so everything a save state needs of it.
 * Its CHR data is the ROM's and never changes, and its engine is not state either,
 * so neither of those is here.
 *
 * PPU inherits this rather than holding it so that saving and loading are a single
 * assignment each, and a field added here cannot be left out of them by mistake.
 */
struct PPUState
{
    uint8_t ppuCtrl; /**< $2000 */
    uint8_t ppuMask; /**< $2001 */
    uint8_t ppuStatus; /**< 2002 */
    uint8_t oamAddress; /**< $2003 */
    uint8_t ppuScrollX; /**< $2005 */
    uint8_t ppuScrollY; /**< $2005 */

    uint8_t palette[32]; /**< Palette data. */
    uint8_t nametable[2048]; /**< Background table. */
    uint8_t oam[256]; /**< Sprite memory. */

    // PPU Address control
    uint16_t currentAddress; /**< Address that will be accessed on the next PPU read/write. */
    bool writeToggle; /**< Toggles whether the low or high bit of the current address will be set on the next write to PPUADDR. */
    uint8_t vramBuffer; /**< Stores the last read byte from VRAM to delay reads by 1 byte. */
};

/**
 * Emulates the NES Picture Processing Unit.
 */
class PPU : private PPUState
{
public:
    PPU(SMBEngine& engine);

    /**
     * Take a copy of everything the PPU is holding, to give back to loadState().
     */
    PPUState saveState() const;

    /**
     * Put the PPU back as it was when the state was taken.
     */
    void loadState(const PPUState& state);

    uint8_t readRegister(uint16_t address);

    /**
     * Render to a frame buffer.
     */
    void render(uint32_t* buffer);

    /**
     * Render all four nametables, ignoring scroll, to a 512x480 buffer: the
     * tables of $2000, $2400, $2800 and $2c00, in that order, clockwise from
     * the top left. Super Mario Bros. mirrors vertically, so the left column is
     * one table of VRAM and the right column the other.
     *
     * This is the whole background the PPU could draw, of which render() shows
     * the 256x240 window the scroll registers select.
     */
    void renderNametables(uint32_t* buffer);

    /**
     * The scroll position render() draws from, in pixels into the 512x480
     * nametable picture. The window wraps around at its right edge.
     */
    void getScroll(int& x, int& y);

    void writeDMA(uint8_t page);

    void writeRegister(uint16_t address, uint8_t value);

private:
    SMBEngine& engine;

    uint8_t getAttributeTableValue(uint16_t nametableAddress);
    uint16_t getNametableIndex(uint16_t address);
    uint8_t readByte(uint16_t address);
    uint8_t readCHR(int index);
    uint8_t readDataRegister();
    void renderTile(uint32_t* buffer, int index, int xOffset, int yOffset, int width = 256, int height = 240);
    void writeAddressRegister(uint8_t value);
    void writeByte(uint16_t address, uint8_t value);
    void writeDataRegister(uint8_t value);
};

#endif // PPU_HPP
