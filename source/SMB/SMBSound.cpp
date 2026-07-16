// The SoundEngine subsystem: everything SoundEngine() reaches that nothing
// outside it reaches, and so nothing outside it needs.
//
// Moved out of SMB.cpp by tools/split.py. These are methods of SMBEngine like every other
// routine of the game and are declared in SMBEngine.hpp; the file they are written in is the
// only thing that is different, and tools/layers.py is what keeps it meaning something.
//
#include "SMB.hpp"

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
    // check secondary buffer for win castle music
    if ((M(EventMusicBuffer) & EndOfCastleMusic) != 0)
    {
        return M(EndOfCastleMusicEnvData + offset); // load data from offset for win castle music
    }
    // check primary buffer for water music
    if ((M(AreaMusicBuffer) & 0b01111101) != 0)
    {
        return M(AreaMusicEnvData + offset); // load default data from offset for all other music
    }
    return M(WaterEventMusEnvData + offset); // data for water music and all other event music
}

// Inputs: rawByte = raw length/note byte. Returns the note length.
uint8_t SMBEngine::AlternateLengthHandler(uint8_t rawByte)
{
    // turn xx00000x into 00000xxx, with d0 of the original as the MSB here
    uint8_t lengthCode = ((rawByte & 0x01) << 2) | ((rawByte >> 6) & 0x03);
    return ProcessLengthData(lengthCode);
}

// Inputs: lengthCode = raw length code (low 3 bits used); also reads zero-page 0xf0 and
// NoteLengthTblAdder. Returns the length value from MusicLengthLookupTbl_data.
uint8_t SMBEngine::ProcessLengthData(uint8_t lengthCode)
{
    const uint8_t MusicLengthLookupTbl_data[] = {0x05, 0x0a, 0x14, 0x28, 0x50, 0x1e, 0x3c, 0x02, 0x04, 0x08, 0x10, 0x20,
                                                 0x40, 0x18, 0x30, 0x0c, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x12, 0x24, 0x08,
                                                 0x36, 0x03, 0x09, 0x06, 0x12, 0x1b, 0x24, 0x0c, 0x24, 0x02, 0x06, 0x04,
                                                 0x0c, 0x12, 0x18, 0x08, 0x12, 0x01, 0x03, 0x02, 0x06, 0x09, 0x0c, 0x04};

    uint8_t index = (lengthCode & 0b00000111) // clear all but the three LSBs
                    + M(0xf0)                 // add offset loaded from first header byte
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
    const uint8_t FreqRegLookupTbl_data[] = {
        0x00, 0x88,
        0x00, 0x2f,
        0x00, 0x00, 0x02, 0xa6, 0x02, 0x80, 0x02, 0x5c, 0x02, 0x3a, 0x02, 0x1a, 0x01, 0xdf, 0x01, 0xc4, 0x01,
        0xab, 0x01, 0x93, 0x01, 0x7c, 0x01, 0x67, 0x01, 0x53, 0x01, 0x40, 0x01, 0x2e, 0x01, 0x1d, 0x01, 0x0d, 0x00, 0xfe, 0x00, 0xef,
        0x00, 0xe2, 0x00, 0xd5, 0x00, 0xc9, 0x00, 0xbe, 0x00, 0xb3, 0x00, 0xa9, 0x00, 0xa0, 0x00, 0x97, 0x00, 0x8e, 0x00, 0x86, 0x00,
        0x77, 0x00, 0x7e, 0x00, 0x71, 0x00, 0x54, 0x00, 0x64, 0x00, 0x5f, 0x00, 0x59, 0x00, 0x50, 0x00, 0x47, 0x00, 0x43, 0x00, 0x3b,
        0x00, 0x35, 0x00, 0x2a, 0x00, 0x23, 0x04, 0x75, 0x03, 0x57, 0x02, 0xf9, 0x02, 0xcf, 0x01, 0xfc, 0x00, 0x6a};

    uint8_t lsb = FreqRegLookupTbl_data[1 + freqIndex];
    if (lsb == 0)
    {
        return 0; // if zero, then do not load
    }
    writeData(SND_REGISTER + 2 + channelOffset, lsb); // first byte goes into LSB of frequency divider
    // second byte goes into 3 MSB plus extra bit for length counter
    uint8_t toneHi = FreqRegLookupTbl_data[freqIndex] | 0b00001000;
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
void SMBEngine::FPS2nd(uint8_t ctrlByte)
{
    DmpJpFPS(ctrlByte, 0xbc);
}

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
    uint8_t tone = 0x90; // SmSpc: this creates spaces in the sound, giving it its distinct noise
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
    // is there a sound playing on square 2?
    if (M(Square2SoundBuffer) != 0)
    {
        goto HandleSquare1Music;
    }
    // check for death music or d4 set on secondary buffer
    a = M(EventMusicBuffer) & 0b10010001; // note that regs for death music or d4 are loaded by default
    if (a != 0)
    {
        goto HandleSquare1Music;
    }
    y = M(Squ2_EnvelopeDataCtrl); // check for contents saved from LoadControlRegs
    if (y != 0)                   // (y is the envelope offset LoadEnvelopeData reads, pre-decrement)
    {
        --M(Squ2_EnvelopeDataCtrl); // decrement unless already zero
    } // NoDecEnv1: do a load of envelope data to replace default
    a = LoadEnvelopeData(y);
    writeData(SND_SQUARE2_REG, a); // based on offset set by first load unless playing
    writeData(SND_SQUARE2_REG + 1, 0x7f); // death music or d4 set on secondary buffer

HandleSquare1Music:
    y = M(MusicOffset_Square1); // is there a nonzero offset here?
    if (y == 0)
    {
        goto HandleTriangleMusic; // if not, skip ahead to the triangle channel
    }
    --M(Squ1_NoteLenCounter); // decrement square 1 note length
    if (M(Squ1_NoteLenCounter) == 0)
    { // is it time for more data?

    FetchSqu1MusicData:
        y = M(MusicOffset_Square1); // increment square 1 music offset and fetch data
        ++M(MusicOffset_Square1);
        a = M(W(MusicData) + y);
        if (a == 0)
        {                                         // if nonzero, then skip this part
            writeData(SND_SQUARE1_REG, 0x83);     // store some data into control regs for square 1
            a = 0x94;                             // and fetch another byte of data, used to give
            writeData(SND_SQUARE1_REG + 1, 0x94); // death music its unique sound
            writeData(AltRegContentFlag, 0x94);
            goto FetchSqu1MusicData; // unconditional branch
        } // Squ1NoteHandler
        uint8_t rawByte = a;                            // the fetched music byte
        a = AlternateLengthHandler(rawByte);
        writeData(Squ1_NoteLenCounter, a);              // save note length in square 1 note counter
        if (M(Square1SoundBuffer) != 0)                 // is there a sound playing on square 1?
        {
            goto HandleTriangleMusic;
        }
        uint8_t note = rawByte & 0b00111110;            // change saved data to appropriate note format
        uint8_t tone = SetFreq_Squ1(note);              // play the note (0 if a rest)
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
    // is there a sound playing on square 1?
    if (M(Square1SoundBuffer) != 0)
    {
        goto HandleTriangleMusic;
    }
    // check for death music or d4 set on secondary buffer
    a = M(EventMusicBuffer) & 0b10010001;
    if (a == 0)
    {
        y = M(Squ1_EnvelopeDataCtrl); // check saved envelope offset
        if (y != 0)
        {
            --M(Squ1_EnvelopeDataCtrl); // decrement unless already zero
        } // NoDecEnv2: do a load of envelope data
        a = LoadEnvelopeData(y);
        writeData(SND_SQUARE1_REG, a); // based on offset set by first load
    } // DeathMAltReg: check for alternate control reg data
    a = M(AltRegContentFlag);
    if (a == 0)
    {
        a = 0x7f; // load this value if zero, the alternate value
    } // DoAltLoad: if nonzero, and let's move on
    writeData(SND_SQUARE1_REG + 1, a);

HandleTriangleMusic:
    a = M(MusicOffset_Triangle);
    --M(Tri_NoteLenCounter); // decrement triangle note length
    if (M(Tri_NoteLenCounter) != 0)
    {
        goto HandleNoiseMusic; // is it time for more data?
    }
    y = M(MusicOffset_Triangle); // increment square 1 music offset and fetch data
    ++M(MusicOffset_Triangle);
    a = M(W(MusicData) + y);
    if (a == 0)
    {
        goto LoadTriCtrlReg; // if zero, skip all this and move on to noise
    }
    if ((a & 0x80) != 0)
    {                                      // if non-negative, data is note
        a = ProcessLengthData(a);          // otherwise, it is length data
        writeData(Tri_NoteLenBuffer, a);   // save contents of A
        writeData(SND_TRIANGLE_REG, 0x1f); // load some default data for triangle control reg
        y = M(MusicOffset_Triangle);       // fetch another byte
        ++M(MusicOffset_Triangle);
        a = M(W(MusicData) + y);
        if (a == 0)
        {
            goto LoadTriCtrlReg; // check once more for nonzero data
        }
    } // TriNoteHandler
    SetFreq_Tri(a);
    x = M(Tri_NoteLenBuffer); // save length in triangle note counter
    writeData(Tri_NoteLenCounter, x);
    a = M(EventMusicBuffer) & 0b01101110; // check for death music or d4 set on secondary buffer
    if (a == 0)
    { // if playing any other secondary, skip primary buffer check
        // check primary buffer for water or castle level music
        a = M(AreaMusicBuffer) & 0b00001010;
        if (a == 0)
        {
            goto HandleNoiseMusic; // if playing any other primary, or death or d4, go on to noise routine
        }
    } // NotDOrD4: if playing water or castle music or any secondary
    a = x;
    if (a < 0x12)
    {
        // check for win castle music again if not playing a long note
        a = M(EventMusicBuffer) & EndOfCastleMusic;
        if (a != 0)
        {
            a = 0x0f; // load value $0f if playing the win castle music and playing a short
            if (a != 0)
            {
                goto LoadTriCtrlReg; // note, load value $1f if playing water or castle level music or any
            }
        } // MediN: secondary besides death and d4 except win castle or win castle and playing
        a = 0x1f;
        if (a != 0)
        {
            goto LoadTriCtrlReg; // a short note, and load value $ff if playing a long note on water, castle
        }
    } // LongN: or any secondary (including win castle) except death and d4
    a = 0xff;

LoadTriCtrlReg:
    writeData(SND_TRIANGLE_REG, a); // save final contents of A into control reg for triangle

HandleNoiseMusic:
    // check if playing underground or castle music
    a = M(AreaMusicBuffer) & 0b11110011;
    if (a == 0)
    {
        return; // if so, skip the noise routine
    }
    --M(Noise_BeatLenCounter); // decrement noise beat length
    if (M(Noise_BeatLenCounter) != 0)
    {
        return; // is it time for more data?
    }

FetchNoiseBeatData:
    y = M(MusicOffset_Noise); // increment noise beat offset and fetch data
    ++M(MusicOffset_Noise);
    a = M(W(MusicData) + y); // get noise beat data, if nonzero, branch to handle
    if (a == 0)
    {
        a = M(NoiseDataLoopbackOfs);     // if data is zero, reload original noise beat offset
        writeData(MusicOffset_Noise, a); // and loopback next time around
        goto FetchNoiseBeatData;         // unconditional branch
    } // NoiseBeatHandler
    x = a; // save the raw beat byte for the note format below
    a = AlternateLengthHandler(x);
    writeData(Noise_BeatLenCounter, a); // store length in noise beat counter
    a = x;
    a &= 0b00111110; // reload data and erase length bits
    if (a == 0)
    {
        goto SilentBeat; // if no beat data, silence
    }
    if (a != 0x30)
    { // noise accordingly
        if (a != 0x20)
        {
            a &= 0b00010000;
            if (a == 0)
            {
                goto SilentBeat;
            }
            a = 0x1c; // short beat data
            x = 0x03;
            y = 0x18;
            PlayBeat(a, x, y);
            return;
        } // StrongBeat
        a = 0x1c; // strong beat data
        x = 0x0c;
        y = 0x18;
        PlayBeat(a, x, y);
        return;
    } // LongBeat
    a = 0x1c; // long beat data
    x = 0x03;
    y = 0x58;
    PlayBeat(a, x, y);
    return;

SilentBeat:
    a = 0x10; // silence
    PlayBeat(a, x, y);
}

// Inputs: none
// Outputs: none
void SMBEngine::NoiseSfxHandler()
{
    const uint8_t BrickShatterEnvData_data[] = {0x15, 0x16, 0x16, 0x17, 0x17, 0x18, 0x19, 0x19,
                                                0x1a, 0x1a, 0x1c, 0x1d, 0x1d, 0x1e, 0x1e, 0x1f};

    const uint8_t BrickShatterFreqData_data[] = {0x01, 0x0e, 0x0e, 0x0d, 0x0b, 0x06, 0x0c, 0x0f,
                                                 0x0a, 0x09, 0x03, 0x0d, 0x08, 0x0d, 0x06, 0x0c};

    y = M(NoiseSoundQueue); // check for sfx in queue
    if (y != 0)
    {
        writeData(NoiseSoundBuffer, y); // if found, put in buffer
        if ((y & Sfx_BrickShatter) != 0)
        {
            goto PlayBrickShatter; // brick shatter
        }
        if ((y & Sfx_BowserFlame) != 0)
        {
            goto PlayBowserFlame; // bowser flame
        }
    } // CheckNoiseBuffer
    a = M(NoiseSoundBuffer); // check for sfx in buffer
    if (a != 0)
    { // if not found, exit sub
        if ((a & Sfx_BrickShatter) != 0)
        {
            goto ContinueBrickShatter; // brick shatter
        }
        if ((a & Sfx_BowserFlame) != 0)
        {
            goto ContinueBowserFlame; // bowser flame
        }
    } // ExNH
    return;

PlayBowserFlame:
    a = 0x40; // load length of bowser flame sound
    writeData(Noise_SfxLenCounter, 0x40);

ContinueBowserFlame:
    a = M(Noise_SfxLenCounter) >> 1;
    y = a;
    x = 0x0f; // load reg contents of bowser flame sound
    a = M(BowserFlameEnvData - 1 + y);
    if (a != 0)
    {
        goto PlayNoiseSfx; // unconditional branch here
    }

PlayBrickShatter:
    a = 0x20; // load length of brick shatter sound
    writeData(Noise_SfxLenCounter, 0x20);
    goto ContinueBrickShatter;

ContinueBrickShatter:
    a = M(Noise_SfxLenCounter) >> 1; // divide by 2 and check for bit set to use offset
    if ((M(Noise_SfxLenCounter) & 0x01) == 0)
    {
        goto DecrementSfx3Length;
    }

    y = a;
    x = BrickShatterFreqData_data[y]; // load reg contents of brick shatter sound
    a = BrickShatterEnvData_data[y];

PlayNoiseSfx:
    writeData(SND_NOISE_REG, a); // play the sfx
    writeData(SND_NOISE_REG + 2, x);
    a = 0x18;
    writeData(SND_NOISE_REG + 3, 0x18);

DecrementSfx3Length:
    --M(Noise_SfxLenCounter); // decrement length of sfx
    if (M(Noise_SfxLenCounter) == 0)
    {
        // if done, stop playing the sfx
        writeData(SND_NOISE_REG, 0xf0);
        a = 0x00;
        writeData(NoiseSoundBuffer, 0x00);
    } // ExSfx3
}

// Inputs: none
// Outputs: none
void SMBEngine::SoundEngine()
{
    a = M(OperMode); // are we in title screen mode?
    if (a == 0)
    {
        writeData(SND_MASTERCTRL_REG, a); // if so, disable sound and leave
        return;

        //------------------------------------------------------------------------
    } // SndOn
    writeData(JOYPAD_PORT2, 0xff);       // disable irqs and set frame counter mode???
    writeData(SND_MASTERCTRL_REG, 0x0f); // enable first four channels
    // is sound already in pause mode?
    if (M(PauseModeFlag) == 0)
    {
        // if not, check pause sfx queue
        if (M(PauseSoundQueue) != 0x01)
        {
            goto RunSoundSubroutines; // if queue is empty, skip pause mode routine
        }
    } // InPause: check pause sfx buffer
    if (M(PauseSoundBuffer) == 0)
    {
        a = M(PauseSoundQueue); // check pause queue
        if (a == 0)
        {
            goto SkipSoundSubroutines;
        }
        writeData(PauseSoundBuffer, a); // if queue full, store in buffer and activate
        writeData(PauseModeFlag, a);    // pause mode to interrupt game sounds
        // disable sound and clear sfx buffers
        writeData(SND_MASTERCTRL_REG, 0x00);
        writeData(Square1SoundBuffer, 0x00);
        writeData(Square2SoundBuffer, 0x00);
        writeData(NoiseSoundBuffer, 0x00);
        writeData(SND_MASTERCTRL_REG, 0x0f); // enable sound again
        a = 0x2a;                            // store length of sound in pause counter
        writeData(Squ1_SfxLenCounter, 0x2a);

    PTone1F: // play first tone
        a = 0x44;
        goto PTRegC; // unconditional branch
    } // ContPau: check pause length left
    a = M(Squ1_SfxLenCounter);
    if (a != 0x24)
    {
        if (a == 0x1e)
        {
            goto PTone1F;
        }
        if (a != 0x18)
        {
            goto DecPauC; // only load regs during times, otherwise skip
        }
    } // PTone2F: store reg contents and play the pause sfx
    a = 0x64;

PTRegC:
    x = 0x84;
    y = 0x7f;
    PlaySqu1Sfx(a, x, y);

DecPauC: // decrement pause sfx counter
    --M(Squ1_SfxLenCounter);
    if (M(Squ1_SfxLenCounter) != 0)
    {
        goto SkipSoundSubroutines;
    }
    // disable sound if in pause mode and
    writeData(SND_MASTERCTRL_REG, 0x00); // not currently playing the pause sfx
    // if no longer playing pause sfx, check to see
    if (M(PauseSoundBuffer) == 0x02)
    {
        a = 0x00; // clear pause mode to allow game sounds again
        writeData(PauseModeFlag, 0x00);
    } // SkipPIn: clear pause sfx buffer
    a = 0x00;
    writeData(PauseSoundBuffer, 0x00);
    if (a == 0)
    {
        goto SkipSoundSubroutines;
    }

RunSoundSubroutines:
    Square1SfxHandler(); // play sfx on square channel 1
    Square2SfxHandler(); //  ''  ''  '' square channel 2
    NoiseSfxHandler();   //  ''  ''  '' noise channel
    MusicHandler();      // play music on all channels
    a = 0x00;            // clear the music queues
    writeData(AreaMusicQueue, 0x00);
    writeData(EventMusicQueue, 0x00);

SkipSoundSubroutines:
    // clear the sound effects queues
    writeData(Square1SoundQueue, 0x00);
    writeData(Square2SoundQueue, 0x00);
    writeData(NoiseSoundQueue, 0x00);
    writeData(PauseSoundQueue, 0x00);
    y = M(DAC_Counter);                  // load some sort of counter
    a = M(AreaMusicBuffer) & 0b00000011; // check for specific music
    if (a != 0)
    {
        ++M(DAC_Counter); // increment and check counter
        if (y < 0x30)
        {
            goto StrWave; // if not there yet, just store it
        }
    } // NoIncDAC
    a = y;
    if (a == 0)
    {
        goto StrWave; // if we are at zero, do not decrement
    }
    --M(DAC_Counter); // decrement counter

StrWave: // store into DMC load register (??)
    writeData(SND_DELTA_REG + 1, y);
    // we are done here
}

// Inputs: none
// Outputs: none
void SMBEngine::MusicHandler()
{
    bool shiftedBit = false;

    a = M(EventMusicQueue); // check event music queue
    if (a != 0)
    {
        goto LoadEventMusic;
    }
    a = M(AreaMusicQueue); // check area music queue
    if (a != 0)
    {
        goto LoadAreaMusic;
    }
    // check both buffers
    a = M(EventMusicBuffer) | M(AreaMusicBuffer);
    if (a != 0)
    {
        goto HandleSquare2Music; // if we have music, start with square 2 channel
    }
    return; // no music, then leave

LoadEventMusic:
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
    if (a != TimeRunningOutMusic)
    {
        goto FindEventMusicHeader;
    }
    x = 0x08; // load offset to be added to length byte of header
    writeData(NoteLengthTblAdder, 0x08);
    goto FindEventMusicHeader; // unconditional branch
LoadAreaMusic:
    if (a == 0x04)
    { // no, do not stop square 1 sfx
        StopSquare1Sfx();
    } // NoStop1: start counter used only by ground level music
    y = 0x10;

GMLoopB:
    writeData(GroundMusicHeaderOfs, y);

HandleAreaMusicLoopB:
    y = 0x00; // clear event music buffer
    writeData(EventMusicBuffer, 0x00);
    writeData(AreaMusicBuffer, a); // copy area music queue contents to buffer
    if (a == 0x01)
    {
        ++M(GroundMusicHeaderOfs);   // increment but only if playing ground level music
        y = M(GroundMusicHeaderOfs); // is it time to loopback ground level music?
        if (y != 0x32)
        {
            goto LoadHeader; // branch ahead with alternate offset
        }
        y = 0x11;
        goto GMLoopB; // unconditional branch
    } // FindAreaMusicHeader
    y = 0x08;                             // load Y for offset of area music
    writeData(MusicOffset_Square2, 0x08); // residual instruction here

FindEventMusicHeader:
    ++y; // increment Y pointer based on previously loaded queue contents
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // bit shift and increment until we find a set bit for music
    if (!shiftedBit)
    {
        goto FindEventMusicHeader;
    }

LoadHeader:
    // load offset for header
    y = M(MusicHeaderOffsetData + y);
    // now load the header
    writeData(NoteLenLookupTblOfs, M(MusicHeaderData + y));
    writeData(MusicDataLow, M(MusicHeaderData + 1 + y));
    writeData(MusicDataHigh, M(MusicHeaderData + 2 + y));
    writeData(MusicOffset_Triangle, M(MusicHeaderData + 3 + y));
    writeData(MusicOffset_Square1, M(MusicHeaderData + 4 + y));
    a = M(MusicHeaderData + 5 + y);
    writeData(MusicOffset_Noise, a);
    writeData(NoiseDataLoopbackOfs, a);
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
    goto HandleSquare2Music;

HandleSquare2Music:
    --M(Squ2_NoteLenCounter); // decrement square 2 note length
    if (M(Squ2_NoteLenCounter) != 0)
    {
        MiscSqu2MusicTasks();
        return;
    }
    y = M(MusicOffset_Square2); // increment square 2 music offset and fetch data
    ++M(MusicOffset_Square2);
    a = M(W(MusicData) + y);
    if (a != 0)
    { // if zero, the data is a null terminator
        if ((a & 0x80) == 0)
        {
            goto Squ2NoteHandler; // if non-negative, data is a note
        }
        if (a != 0)
        {
            goto Squ2LengthHandler; // otherwise it is length data
        }
    } // EndOfMusicData
    a = M(EventMusicBuffer); // check secondary buffer for time running out music
    if (a == TimeRunningOutMusic)
    {
        a = M(AreaMusicBuffer_Alt); // load previously saved contents of primary buffer
        if (a != 0)
        {
            goto MusicLoopBack; // and start playing the song again if there is one
        }
    } // NotTRO: check for victory music (the only secondary that loops)
    a &= VictoryMusic;
    if (a != 0)
    {
        goto LoadEventMusic;
    }
    // check primary buffer for any music except pipe intro
    a = M(AreaMusicBuffer) & 0b01011111;
    if (a != 0)
    {
        goto MusicLoopBack; // if any area music except pipe intro, music loops
    }
    // clear primary and secondary buffers and initialize
    writeData(AreaMusicBuffer, 0x00); // control regs of square and triangle channels
    writeData(EventMusicBuffer, 0x00);
    writeData(SND_TRIANGLE_REG, 0x00);
    a = 0x90;
    writeData(SND_SQUARE1_REG, 0x90);
    writeData(SND_SQUARE2_REG, 0x90);
    return;

MusicLoopBack:
    goto HandleAreaMusicLoopB;

Squ2LengthHandler:
    a = ProcessLengthData(a); // store length of note
    writeData(Squ2_NoteLenBuffer, a);
    y = M(MusicOffset_Square2); // fetch another byte (MUST NOT BE LENGTH BYTE!)
    ++M(MusicOffset_Square2);
    a = M(W(MusicData) + y);

Squ2NoteHandler:
    // is there a sound playing on this channel?
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
}
