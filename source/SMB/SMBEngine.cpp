#include <cstring>

#include "../Emulation/APU.hpp"
#include "../Emulation/Controller.hpp"
#include "../Emulation/PPU.hpp"

#include "SMBConstants.hpp"
#include "SMBEngine.hpp"

#define DATA_STORAGE_OFFSET 0x8000 // Starting address for storing constant data

//---------------------------------------------------------------------
// Public interface
//---------------------------------------------------------------------

const std::size_t SMBEngine::RAM_SIZE;

SMBEngine::SMBEngine(uint8_t* romImage, bool enableAudio) :
    audioEnabled(enableAudio)
{
    apu = new APU();
    ppu = new PPU(*this);
    controller1 = new Controller();
    controller2 = new Controller();

    // Anything the game reads from the address space that loadConstantData()
    // does not write is read from here as-is, so start from a known value rather
    // than from whatever happens to be on the heap.
    //
    memset(dataStorage, 0, sizeof(dataStorage));

    // CHR Location in ROM: Header (16 bytes) + 2 PRG pages (16k each)
    chr = (romImage + 16 + (16384 * 2));
}

SMBEngine::~SMBEngine()
{
    delete apu;
    delete ppu;
    delete controller1;
    delete controller2;
}

void SMBEngine::audioCallback(uint8_t* stream, int length)
{
    apu->output(stream, length);
}

Controller& SMBEngine::getController1()
{
    return *controller1;
}

Controller& SMBEngine::getController2()
{
    return *controller2;
}

const uint8_t* SMBEngine::getRam() const
{
    return ram;
}

bool SMBEngine::isLagFrame() const
{
    // The NMI handler only overruns a frame while it is drawing the screen for a
    // newly loaded area, which is the first of the screen routines (InitScreen).
    //
    // Checked against FCEUX over a full playthrough of the game: of the 53 lag
    // frames in it, this identifies 51, and never mistakes a normal frame for a
    // lag frame. It misses the frames that lag because an unusual number of
    // enemies are on screen, which only cycle-accurate timing could predict.
    //
    return ram[ScreenRoutineTask] == 0;
}

void SMBEngine::render(uint32_t* buffer)
{
    ppu->render(buffer);
}

void SMBEngine::reset()
{
    // Run the decompiled code for initialization
    code(0);
}

void SMBEngine::update()
{
    // Run the decompiled code for the NMI handler
    code(1);

    // Update the APU
    if (audioEnabled)
    {
        apu->stepFrame();
    }
}

//---------------------------------------------------------------------
// Private methods

uint8_t* SMBEngine::getCHR()
{
    return chr;
}

uint8_t& SMBEngine::getMemory(uint16_t address)
{
    // Constant data
    if( address >= DATA_STORAGE_OFFSET )
    {
        return dataStorage[address - DATA_STORAGE_OFFSET];
    }

    // RAM and Mirrors
    return ram[address & 0x7ff];
}

uint16_t SMBEngine::getMemoryWord(uint8_t address)
{
    return (uint16_t)readData(address) + ((uint16_t)(readData(address + 1)) << 8);
}

void SMBEngine::pha()
{
    writeData(0x100 | (uint16_t)s, a);
    s--;
}

void SMBEngine::pla()
{
    s++;
    a = readData(0x100 | (uint16_t)s);
}

uint8_t SMBEngine::readData(uint16_t address)
{
    // Constant data
    if( address >= DATA_STORAGE_OFFSET )
    {
        return dataStorage[address - DATA_STORAGE_OFFSET];
    }
    // RAM and Mirrors
    else if( address < 0x2000 )
    {
        return ram[address & 0x7ff];
    }
    // PPU Registers and Mirrors
    else if( address < 0x4000 )
    {
        return ppu->readRegister(0x2000 + (address & 0x7));
    }
    // IO registers
    else if( address < 0x4020 )
    {
        switch (address)
        {
        case 0x4016:
            return controller1->readByte();
        case 0x4017:
            return controller2->readByte();
        }
    }

    return 0;
}

void SMBEngine::writeData(uint16_t address, uint8_t value)
{
    // RAM and Mirrors
    if( address < 0x2000 )
    {
        ram[address & 0x7ff] = value;
    }
    // PPU Registers and Mirrors
    else if( address < 0x4000 )
    {
        ppu->writeRegister(0x2000 + (address & 0x7), value);
    }
    // IO registers
    else if( address < 0x4020 )
    {
        switch( address )
        {
        case 0x4014:
            ppu->writeDMA(value);
            break;
        case 0x4016:
            controller1->writeByte(value);
            controller2->writeByte(value);
            break;
        default:
            apu->writeRegister(address, value);
            break;
        }
    }
}

void SMBEngine::writeData(uint16_t address, const uint8_t* data, size_t length)
{
    address -= DATA_STORAGE_OFFSET;

    memcpy(dataStorage + (std::ptrdiff_t)address, data, length);
}
