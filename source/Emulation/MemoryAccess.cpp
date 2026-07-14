#include "MemoryAccess.hpp"

MemoryAccess::MemoryAccess(uint8_t* value)
{
    this->value = value;
}

MemoryAccess::MemoryAccess(uint8_t constant)
{
    this->constant = constant;
    this->value = &constant;
}

MemoryAccess& MemoryAccess::operator = (uint8_t value)
{
    *(this->value) = value;
    return *this;
}

MemoryAccess& MemoryAccess::operator = (const MemoryAccess& rhs)
{
    return ((*this) = *(rhs.value));
}

MemoryAccess& MemoryAccess::operator += (uint8_t value)
{
    *(this->value) += value;
    return *this;
}

MemoryAccess& MemoryAccess::operator -= (uint8_t value)
{
    *(this->value) -= value;
    return *this;
}

MemoryAccess& MemoryAccess::operator ++ ()
{
    *(this->value) = *(this->value) + 1;
    return *this;
}

MemoryAccess& MemoryAccess::operator -- ()
{
    *(this->value) = *(this->value) - 1;
    return *this;
}

MemoryAccess& MemoryAccess::operator ++ (int unused)
{
    return ++(*this);
}

MemoryAccess& MemoryAccess::operator -- (int unused)
{
    return --(*this);
}

MemoryAccess& MemoryAccess::operator &= (uint8_t value)
{
    *(this->value) &= value;
    return *this;
}

MemoryAccess& MemoryAccess::operator |= (uint8_t value)
{
    *(this->value) |= value;
    return *this;
}

MemoryAccess& MemoryAccess::operator ^= (uint8_t value)
{
    *(this->value) ^= value;
    return *this;
}

MemoryAccess& MemoryAccess::operator <<= (int shift)
{
    for (int i = 0; i < shift; i++)
    {
        *(this->value) = (*(this->value) << 1) & 0xfe;
    }
    return *this;
}

MemoryAccess& MemoryAccess::operator >>= (int shift)
{
    for (int i = 0; i < shift; i++)
    {
        *(this->value) = (*(this->value) >> 1) & 0x7f;
    }
    return *this;
}

MemoryAccess::operator uint8_t()
{
    return *value;
}

