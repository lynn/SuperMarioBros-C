#ifndef SMB_HPP
#define SMB_HPP

#include "SMBConstants.hpp"
#include "SMBEngine.hpp"

//---------------------------------------------------------------------
// Macros:
//---------------------------------------------------------------------

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
#define HIBYTE(v) (static_cast<uint8_t>((v >> 8) & 0xff))

/**
 * Low byte of a 16-bit integer.
 */
#define LOBYTE(v) (static_cast<uint8_t>(v & 0xff))

#endif // SMB_HPP
