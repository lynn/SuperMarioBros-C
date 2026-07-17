/**
 * @file
 * @brief names the addresses of the NES RAM, as the disassembly does.
 */
#ifndef RAMMAP_HPP
#define RAMMAP_HPP

#include <cstddef>
#include <cstdint>

/**
 * A name the disassembly gives an address in RAM.
 */
struct RamLabel
{
    const char* name;
    uint16_t address;
};

/**
 * What the disassembly calls the addresses in RAM, for the RAM view to label
 * them with. An address can have more than one name: the disassembly names both
 * an array and its first element in places, and gives some addresses a second
 * name for a second purpose.
 *
 * The names are the disassembly's, so that the view and the source cross-
 * reference. Only the addresses that are named are here, and the ones that are
 * name the start of whatever is at them rather than every byte of it.
 */
extern const RamLabel ramMap[];

/**
 * How many names ramMap holds.
 */
extern const std::size_t ramMapLength;

/**
 * A run of RAM that is all one thing, for the RAM view to colour.
 */
struct RamRegion
{
    const char* name;
    uint16_t first;  /**< the first address in the region */
    uint16_t last;   /**< the last address in it, itself part of the region */
    uint32_t color;  /**< what to draw it in, as 0xrrggbb */
};

/**
 * The runs of RAM that are each all one thing. The regions do not overlap, and
 * they cover only the parts of RAM that are worth telling apart at a glance:
 * the rest of it is a scattering of individual variables, which ramMap names.
 */
extern const RamRegion ramRegions[];

/**
 * How many regions ramRegions holds.
 */
extern const std::size_t ramRegionsLength;

/**
 * The region an address falls in, or null if it falls outside all of them.
 */
const RamRegion* findRamRegion(uint16_t address);

#endif // RAMMAP_HPP
