#ifndef MEMORYACCESS_HPP
#define MEMORYACCESS_HPP

#include <cstdint>

/**
 * Wraps a memory location or a register so the decompiled code can read and
 * write it with the same operators the 6502 instructions used.
 */
class MemoryAccess
{
public:
    /**
     * Construct a MemoryAccess to a location.
     */
    MemoryAccess(uint8_t* value);

    /**
     * Construct a MemoryAccess to a constant value.
     */
    MemoryAccess(uint8_t constant);

    MemoryAccess& operator = (uint8_t value);
    MemoryAccess& operator = (const MemoryAccess& rhs);
    MemoryAccess& operator += (uint8_t value);
    MemoryAccess& operator -= (uint8_t value);
    MemoryAccess& operator ++ ();
    MemoryAccess& operator -- ();
    MemoryAccess& operator ++ (int unused);
    MemoryAccess& operator -- (int unused);
    MemoryAccess& operator &= (uint8_t value);
    MemoryAccess& operator |= (uint8_t value);
    MemoryAccess& operator ^= (uint8_t value);
    MemoryAccess& operator <<= (int shift);
    MemoryAccess& operator >>= (int shift);
    operator uint8_t();

private:
    uint8_t* value;
    uint8_t constant;
};

#endif // MEMORYACCESS_HPP
