// The SoundEngine subsystem: everything SoundEngine() reaches that nothing
// outside it reaches, and so nothing outside it needs.
//
// Moved out of SMB.cpp by tools/split.py. These are methods of SMBEngine like every other
// routine of the game and are declared in SMBEngine.hpp; the file they are written in is the
// only thing that is different, and tools/layers.py is what keeps it meaning something.
//
#include "SMB.hpp"

// Inputs: x, y = square 1 control register contents to dump
// Outputs: none
void SMBEngine::Dump_Squ1_Regs()
{
    writeData(SND_SQUARE1_REG + 1, y); // dump the contents of X and Y into square 1's control regs
    writeData(SND_SQUARE1_REG, x);
}

// Inputs: x, y = square 2 control register contents to dump
// Outputs: none
void SMBEngine::Dump_Sq2_Regs()
{
    writeData(SND_SQUARE2_REG, x); // dump the contents of X and Y into square 2's control regs
    writeData(SND_SQUARE2_REG + 1, y);
}

// Inputs: none (reads EventMusicBuffer/AreaMusicBuffer memory)
// Outputs: a = selected control value (0x04/0x08/0x28); x = 0x82; y = 0x7f (fixed reg contents for
// the caller to dump)
void SMBEngine::LoadControlRegs()
{
    // check secondary buffer for win castle music
    a = M(EventMusicBuffer) & EndOfCastleMusic;
    if (a != 0)
    {
        a = 0x04;    // this value is only used for win castle music
        goto AllMus; // unconditional branch
    } // NotECstlM
    a = M(AreaMusicBuffer) & 0b01111101; // check primary buffer for water music
    if (a != 0)
    {
        a = 0x08; // this is the default value for all other music
        if (a != 0)
        {
            goto AllMus;
        }
    } // WaterMus: this value is used for water music and all other event music
    a = 0x28;

AllMus: // load contents of other sound regs for square 2
    x = 0x82;
    y = 0x7f;
}

// Inputs: y = envelope table offset (reads EventMusicBuffer/AreaMusicBuffer memory to pick the table)
// Outputs: a = envelope byte loaded from the selected table
void SMBEngine::LoadEnvelopeData()
{
    // check secondary buffer for win castle music
    a = M(EventMusicBuffer) & EndOfCastleMusic;
    if (a != 0)
    {
        a = M(EndOfCastleMusicEnvData + y); // load data from offset for win castle music
        return;

        //------------------------------------------------------------------------
    } // LoadUsualEnvData
    // check primary buffer for water music
    a = M(AreaMusicBuffer) & 0b01111101;
    if (a != 0)
    {
        a = M(AreaMusicEnvData + y); // load default data from offset for all other music
        return;

        //------------------------------------------------------------------------
    } // LoadWaterEventMusEnvData
    a = M(WaterEventMusEnvData + y); // load data from offset for water music and all other event music
}

// Inputs: a = raw length/note byte
// Outputs: x = original a on entry (saved copy for the caller); a, y = see ProcessLengthData
void SMBEngine::AlternateLengthHandler()
{
    x = a; // save a copy of original byte into X
    // turn xx00000x into 00000xxx, with d0 of the original as the MSB here
    a = (uint8_t)(((a & 0x01) << 2) | ((a >> 6) & 0x03));

    ProcessLengthData();
}

// Inputs: a = raw length code (low 3 bits used); also reads zero-page 0xf0 and NoteLengthTblAdder
// Outputs: a = length value from MusicLengthLookupTbl_data; y = index used for the lookup
void SMBEngine::ProcessLengthData()
{
    const uint8_t MusicLengthLookupTbl_data[] = {0x05, 0x0a, 0x14, 0x28, 0x50, 0x1e, 0x3c, 0x02, 0x04, 0x08, 0x10, 0x20,
                                                 0x40, 0x18, 0x30, 0x0c, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x12, 0x24, 0x08,
                                                 0x36, 0x03, 0x09, 0x06, 0x12, 0x1b, 0x24, 0x0c, 0x24, 0x02, 0x06, 0x04,
                                                 0x0c, 0x12, 0x18, 0x08, 0x12, 0x01, 0x03, 0x02, 0x06, 0x09, 0x0c, 0x04};

    a &= 0b00000111;            // clear all but the three LSBs
    a += M(0xf0);               // add offset loaded from first header byte
    a += M(NoteLengthTblAdder); // add extra if time running out music
    y = a;
    a = MusicLengthLookupTbl_data[y]; // load length
}

// Inputs: a = frequency table index (forwarded to Dump_Freq_Regs); x, y = square 1 control register
// contents (forwarded to Dump_Squ1_Regs)
// Outputs: none
void SMBEngine::PlaySqu1Sfx()
{
    Dump_Squ1_Regs(); // do sub to set ctrl regs for square 1, then set frequency regs
    SetFreq_Squ1();
}

// Inputs: a = frequency table index (forwarded to Dump_Freq_Regs)
// Outputs: none
void SMBEngine::SetFreq_Squ1()
{
    x = 0x00; // set frequency reg offset for square 1 sound channel
    Dump_Freq_Regs();
}

// Inputs: a = frequency table index; x = sound register channel offset (0x00 square 1, 0x04
// square 2, 0x08 triangle)
// Outputs: none (a and y are left as scratch)
void SMBEngine::Dump_Freq_Regs()
{
    const uint8_t FreqRegLookupTbl_data[] = {
        0x00, 0x88,
        0x00, 0x2f,
        0x00, 0x00, 0x02, 0xa6, 0x02, 0x80, 0x02, 0x5c, 0x02, 0x3a, 0x02, 0x1a, 0x01, 0xdf, 0x01, 0xc4, 0x01,
        0xab, 0x01, 0x93, 0x01, 0x7c, 0x01, 0x67, 0x01, 0x53, 0x01, 0x40, 0x01, 0x2e, 0x01, 0x1d, 0x01, 0x0d, 0x00, 0xfe, 0x00, 0xef,
        0x00, 0xe2, 0x00, 0xd5, 0x00, 0xc9, 0x00, 0xbe, 0x00, 0xb3, 0x00, 0xa9, 0x00, 0xa0, 0x00, 0x97, 0x00, 0x8e, 0x00, 0x86, 0x00,
        0x77, 0x00, 0x7e, 0x00, 0x71, 0x00, 0x54, 0x00, 0x64, 0x00, 0x5f, 0x00, 0x59, 0x00, 0x50, 0x00, 0x47, 0x00, 0x43, 0x00, 0x3b,
        0x00, 0x35, 0x00, 0x2a, 0x00, 0x23, 0x04, 0x75, 0x03, 0x57, 0x02, 0xf9, 0x02, 0xcf, 0x01, 0xfc, 0x00, 0x6a};

    y = a;
    a = FreqRegLookupTbl_data[1 + y]; // use previous contents of A for sound reg offset
    if (a != 0)
    {                                       // if zero, then do not load
        writeData(SND_REGISTER + 2 + x, a); // first byte goes into LSB of frequency divider
        // second byte goes into 3 MSB plus extra bit for
        a = FreqRegLookupTbl_data[y] | 0b00001000; // length counter
        writeData(SND_REGISTER + 3 + x, a);
    } // NoTone
}

// Inputs: a = frequency table index (forwarded to Dump_Freq_Regs); x, y = square 2 control register
// contents (forwarded to Dump_Sq2_Regs)
// Outputs: none
void SMBEngine::PlaySqu2Sfx()
{
    Dump_Sq2_Regs(); // do sub to set ctrl regs for square 2, then set frequency regs
    SetFreq_Squ2();
}

// Inputs: a = frequency table index (forwarded to Dump_Freq_Regs)
// Outputs: none
void SMBEngine::SetFreq_Squ2()
{
    x = 0x04;         // set frequency reg offset for square 2 sound channel
    Dump_Freq_Regs(); // unconditional branch
}

// Inputs: a = frequency table index (forwarded to Dump_Freq_Regs)
// Outputs: none
void SMBEngine::SetFreq_Tri()
{
    x = 0x08;         // set frequency reg offset for triangle sound channel
    Dump_Freq_Regs(); // unconditional branch
}

// Inputs: a, x, y = noise register values to load directly
// Outputs: none
void SMBEngine::PlayBeat()
{
    writeData(SND_NOISE_REG, a); // load beat data into noise regs
    writeData(SND_NOISE_REG + 2, x);
    writeData(SND_NOISE_REG + 3, y);

    // ExitMusicHandler
}

// Inputs: x, y = square 1 control register contents (forwarded to Dump_Squ1_Regs)
// Outputs: none
void SMBEngine::DmpJpFPS()
{
    Dump_Squ1_Regs();
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

// Inputs: none
// Outputs: x = 0x0f (scratch, mirrors the second writeData call)
void SMBEngine::StopSquare1Sfx()
{
    // if end of sfx reached, clear buffer
    writeData(0xf1, 0x00); // and stop making the sfx
    writeData(SND_MASTERCTRL_REG, 0x0e);
    x = 0x0f;
    writeData(SND_MASTERCTRL_REG, 0x0f);
}

// Inputs: a = value to store to Squ2_SfxLenCounter; x = square 2 control register partial contents
// (forwarded to PlaySqu2Sfx)
// Outputs: none
void SMBEngine::CGrab_TTickRegL()
{
    writeData(Squ2_SfxLenCounter, a);
    y = 0x7f; // load the rest of reg contents
    a = 0x42; // of coin grab and timer tick sound
    PlaySqu2Sfx();
    ContinueCGrabTTick();
}

// Inputs: none (reads Squ2_SfxLenCounter memory)
// Outputs: none
void SMBEngine::ContinueCGrabTTick()
{
    a = M(Squ2_SfxLenCounter); // check for time to play second tone yet
    if (a == 0x30)
    {
        a = 0x54; // if so, load the tone directly into the reg
        writeData(SND_SQUARE2_REG + 2, 0x54);
    } // N2Tone
    DecrementSfx2Length(); // unconditional branch, however we got here
}

// Inputs: a = frequency table index (forwarded to Dump_Freq_Regs); x, y = square 2 control register
// contents (forwarded to Dump_Sq2_Regs)
// Outputs: none
void SMBEngine::LoadSqu2Regs()
{
    PlaySqu2Sfx();
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

// Inputs: none
// Outputs: x = 0x00 (scratch, mirrors the writeData call)
void SMBEngine::EmptySfx2Buffer()
{
    x = 0x00; // initialize square 2's sound effects buffer
    writeData(Square2SoundBuffer, 0x00);
    StopSquare2Sfx();
}

// Inputs: none
// Outputs: x = 0x0f (scratch, mirrors the second writeData call)
void SMBEngine::StopSquare2Sfx()
{
    // stop playing the sfx
    writeData(SND_MASTERCTRL_REG, 0x0d);
    x = 0x0f;
    writeData(SND_MASTERCTRL_REG, 0x0f);
}

// the flagpole slide sound shares part of third part
// Inputs: x = square 1 control register partial contents (forwarded to DmpJpFPS/Dump_Squ1_Regs)
// Outputs: none
void SMBEngine::FPS2nd()
{
    y = 0xbc;
    DmpJpFPS();
}

// Inputs: none
// Outputs: none
void SMBEngine::JumpRegContents()
{
    x = 0x82; // note that small and big jump borrow each others' reg contents
    y = 0xa7; // anyway, this loads the first part of mario's jumping sound
    PlaySqu1Sfx();
    a = 0x28;                            // store length of sfx for both jumping sounds
    writeData(Squ1_SfxLenCounter, 0x28); // then continue on here
    ContinueSndJump();
}

// Inputs: none (reads Squ1_SfxLenCounter memory)
// Outputs: none
void SMBEngine::ContinueSndJump()
{
    a = M(Squ1_SfxLenCounter); // jumping sounds seem to be composed of three parts
    if (a == 0x25)
    {
        x = 0x5f; // load second part
        y = 0xf6;
        DmpJpFPS(); // unconditional branch
        return;
    } // N2Prt: check for third part
    if (a != 0x20)
    {
        DecJpFPS();
        return;
    }
    x = 0x48; // load third part
    FPS2nd();
}

// the fireball sound shares reg contents with the bump sound
// Inputs: a = value to store to Squ1_SfxLenCounter; x = square 1 control register partial contents
// Outputs: none
void SMBEngine::Fthrow()
{
    x = 0x9e;
    writeData(Squ1_SfxLenCounter, a);
    a = 0x0c; // load offset for bump sound
    PlaySqu1Sfx();
    ContinueBumpThrow();
}

// Inputs: none (reads Squ1_SfxLenCounter memory)
// Outputs: none
void SMBEngine::ContinueBumpThrow()
{
    a = M(Squ1_SfxLenCounter); // check for second part of bump sound
    if (a != 0x06)
    {
        DecJpFPS();
        return;
    }
    a = 0xbb; // load second part directly
    writeData(SND_SQUARE1_REG + 1, 0xbb);
    DecJpFPS();
}

// Inputs: none (reads Squ1_SfxLenCounter memory)
// Outputs: none
void SMBEngine::ContinueSwimStomp()
{
    const uint8_t SwimStompEnvelopeData_data[] = {0x9f, 0x9b, 0x98, 0x96, 0x95, 0x94, 0x92, 0x90, 0x90, 0x9a, 0x97, 0x95, 0x93, 0x92};

    y = M(Squ1_SfxLenCounter);             // look up reg contents in data section based on
    a = SwimStompEnvelopeData_data[y - 1]; // length of sound left, used to control sound's
    writeData(SND_SQUARE1_REG, a);         // envelope
    if (y != 0x06)
    {
        BranchToDecLength1();
        return;
    }
    a = 0x9e;                             // when the length counts down to a certain point, put this
    writeData(SND_SQUARE1_REG + 2, 0x9e); // directly into the LSB of square 1's frequency divider
    BranchToDecLength1();
}

// Inputs: none (reads Squ1_SfxLenCounter memory)
// Outputs: none
void SMBEngine::ContinueSmackEnemy()
{
    y = M(Squ1_SfxLenCounter); // check about halfway through
    if (y == 0x08)
    {
        // if we're at the about-halfway point, make the second tone
        writeData(SND_SQUARE1_REG + 2, 0xa0); // in the smack enemy sound
        a = 0x9f;
        if (a != 0)
        {
            goto SmTick;
        }
    } // SmSpc: this creates spaces in the sound, giving it its distinct noise
    a = 0x90;

SmTick:
    writeData(SND_SQUARE1_REG, a);
    DecrementSfx1Length();
}

// Inputs: none (reads Squ1_SfxLenCounter memory)
// Outputs: none
void SMBEngine::ContinuePipeDownInj()
{
    bool shiftedBit = false;

    // some bitwise logic, forces the regs
    a = M(Squ1_SfxLenCounter) >> 1; // to be written to only during six specific times
    if ((M(Squ1_SfxLenCounter) & 0x01) != 0)
    {
        goto NoPDwnL; // during which d3 must be set and d1-0 must be clear
    }
    shiftedBit = (a & 0x01) != 0;
    a >>= 1;
    if (shiftedBit)
    {
        goto NoPDwnL;
    }
    a &= 0b00000010;
    if (a == 0)
    {
        goto NoPDwnL;
    }
    y = 0x91; // and this is where it actually gets written in
    x = 0x9a;
    a = 0x44;
    PlaySqu1Sfx();

NoPDwnL:
    DecrementSfx1Length();
}

// Inputs: none
// Outputs: none
void SMBEngine::Square1SfxHandler()
{
    y = M(Square1SoundQueue); // check for sfx in queue
    if (y != 0)
    {
        writeData(Square1SoundBuffer, y); // if found, put in buffer
        if ((y & Sfx_SmallJump) != 0)
        {
            a = 0x26; // branch here for small mario jumping sound
            JumpRegContents();
            return;
        }
        if ((y & Sfx_BigJump) != 0)
        {
            a = 0x18; // branch here for big mario jumping sound
            JumpRegContents();
            return;
        }
        if ((y & Sfx_Bump) != 0)
        {
            a = 0x0a; // load length of sfx and reg contents for bump sound
            y = 0x93;
            Fthrow();
            return;
        }
        if ((y & Sfx_EnemyStomp) != 0)
        {
            // store length of swim/stomp sound
            writeData(Squ1_SfxLenCounter, 0x0e);
            y = 0x9c; // store reg contents for swim/stomp sound
            x = 0x9e;
            a = 0x26;
            PlaySqu1Sfx();
            ContinueSwimStomp();
            return;
        }
        if ((y & Sfx_EnemySmack) != 0)
        {
            // store length of smack enemy sound
            y = 0xcb;
            x = 0x9f;
            writeData(Squ1_SfxLenCounter, 0x0e);
            a = 0x28; // store reg contents for smack enemy sound
            PlaySqu1Sfx();
            if (a != 0)
            {
                DecrementSfx1Length(); // unconditional branch
                return;
            }
            ContinueSmackEnemy();
            return;
        }
        if ((y & Sfx_PipeDown_Injury) != 0)
        {
            a = 0x2f; // load length of pipedown sound
            writeData(Squ1_SfxLenCounter, 0x2f);
            ContinuePipeDownInj();
            return;
        }
        if ((y & Sfx_Fireball) != 0)
        {
            a = 0x05;
            y = 0x99; // load reg contents for fireball throw sound
            Fthrow();
            return;
        }
        if ((y & Sfx_Flagpole) != 0)
        {
            // store length of flagpole sound
            writeData(Squ1_SfxLenCounter, 0x40);
            a = 0x62; // load part of reg contents for flagpole sound
            SetFreq_Squ1();
            x = 0x99; // now load the rest
            FPS2nd();
            return;
        }
    } // CheckSfx1Buffer
    a = M(Square1SoundBuffer); // check for sfx in buffer
    if (a != 0)
    { // if not found, exit sub
        if ((a & Sfx_SmallJump) != 0)
        {
            ContinueSndJump(); // small mario jump
            return;
        }
        if ((a & Sfx_BigJump) != 0)
        {
            ContinueSndJump(); // big mario jump
            return;
        }
        if ((a & Sfx_Bump) != 0)
        {
            ContinueBumpThrow(); // bump
            return;
        }
        if ((a & Sfx_EnemyStomp) != 0)
        {
            ContinueSwimStomp(); // swim/stomp
            return;
        }
        if ((a & Sfx_EnemySmack) != 0)
        {
            ContinueSmackEnemy(); // smack enemy
            return;
        }
        if ((a & Sfx_PipeDown_Injury) != 0)
        {
            ContinuePipeDownInj(); // pipedown/injury
            return;
        }
        if ((a & Sfx_Fireball) != 0)
        {
            ContinueBumpThrow(); // fireball throw
            return;
        }
        if ((a & Sfx_Flagpole) != 0)
        {
            DecrementSfx1Length(); // slide flagpole
            return;
        }
    } // ExS1H
}

// Inputs: none
// Outputs: none
void SMBEngine::Square2SfxHandler()
{
    const uint8_t PUp_VGrow_FreqData_data[] = {0x14, 0x04, 0x22, 0x24, 0x16, 0x04, 0x24, 0x26, // used by both
                                               0x18, 0x04, 0x26, 0x28, 0x1a, 0x04, 0x28, 0x2a,
                                               0x1c, 0x04, 0x2a, 0x2c, 0x1e, 0x04, 0x2c, 0x2e, // used by vinegrow
                                               0x20, 0x04, 0x2e, 0x30, 0x22, 0x04, 0x30, 0x32};

    const uint8_t PowerUpGrabFreqData_data[] = {0x4c, 0x52, 0x4c, 0x48, 0x3e, 0x36, 0x3e, 0x36, 0x30, 0x28, 0x4a, 0x50, 0x4a, 0x64, 0x3c,
                                                0x32, 0x3c, 0x32, 0x2c, 0x24, 0x3a, 0x64, 0x3a, 0x34, 0x2c, 0x22, 0x2c, 0x22, 0x1c, 0x14};

    const uint8_t ExtraLifeFreqData_data[] = {0x58, 0x02, 0x54, 0x56, 0x4e, 0x44};

    bool shiftedBit = false;

    // special handling for the 1-up sound to keep it
    a = M(Square2SoundBuffer) & Sfx_ExtraLife; // from being interrupted by other sounds on square 2
    if (a != 0)
    {
        goto ContinueExtraLife;
    }
    y = M(Square2SoundQueue); // check for sfx in queue
    if (y != 0)
    {
        writeData(Square2SoundBuffer, y); // if found, put in buffer and check for the following
        if ((y & Sfx_BowserFall) != 0)
        {
            goto PlayBowserFall; // bowser fall
        }
        if ((y & Sfx_CoinGrab) != 0)
        {
            goto PlayCoinGrab; // coin grab
        }
        if ((y & Sfx_GrowPowerUp) != 0)
        {
            goto PlayGrowPowerUp; // power-up reveal
        }
        if ((y & Sfx_GrowVine) != 0)
        {
            goto PlayGrowVine; // vine grow
        }
        if ((y & Sfx_Blast) != 0)
        {
            goto PlayBlast; // fireworks/gunfire
        }
        if ((y & Sfx_TimerTick) != 0)
        {
            goto PlayTimerTick; // timer tick
        }
        if ((y & Sfx_PowerUpGrab) != 0)
        {
            goto PlayPowerUpGrab; // power-up grab
        }
        if ((y & Sfx_ExtraLife) != 0)
        {
            goto PlayExtraLife; // 1-up
        }
    } // CheckSfx2Buffer
    a = M(Square2SoundBuffer); // check for sfx in buffer
    if (a != 0)
    { // if not found, exit sub
        if ((a & Sfx_BowserFall) != 0)
        {
            goto ContinueBowserFall; // bowser fall
        }
        if ((a & Sfx_CoinGrab) != 0)
        {
            goto Cont_CGrab_TTick; // coin grab
        }
        if ((a & Sfx_GrowPowerUp) != 0)
        {
            goto ContinueGrowItems; // power-up reveal
        }
        if ((a & Sfx_GrowVine) != 0)
        {
            goto ContinueGrowItems; // vine grow
        }
        if ((a & Sfx_Blast) != 0)
        {
            goto ContinueBlast; // fireworks/gunfire
        }
        if ((a & Sfx_TimerTick) != 0)
        {
            goto Cont_CGrab_TTick; // timer tick
        }
        if ((a & Sfx_PowerUpGrab) != 0)
        {
            goto ContinuePowerUpGrab; // power-up grab
        }
        if ((a & Sfx_ExtraLife) != 0)
        {
            goto ContinueExtraLife; // 1-up
        }
    } // ExS2H
    return;

PlayCoinGrab:
    a = 0x35; // load length of coin grab sound
    x = 0x8d; // and part of reg contents
    CGrab_TTickRegL();
    return;

PlayTimerTick:
    a = 0x06; // load length of timer tick sound
    x = 0x98; // and part of reg contents
    CGrab_TTickRegL();
    return;

PlayBlast:
    // load length of fireworks/gunfire sound
    writeData(Squ2_SfxLenCounter, 0x20);
    y = 0x94; // load reg contents of fireworks/gunfire sound
    a = 0x5e;
    goto SBlasJ;
ContinueBlast:
    a = M(Squ2_SfxLenCounter); // check for time to play second part
    if (a != 0x18)
    {
        DecrementSfx2Length();
        return;
    }
    y = 0x93; // load second part reg contents then
    a = 0x18;
SBlasJ:
    goto BlstSJp;

PlayPowerUpGrab:
    a = 0x36; // load length of power-up grab sound
    writeData(Squ2_SfxLenCounter, 0x36);

ContinuePowerUpGrab:
    // load frequency reg based on length left over
    a = M(Squ2_SfxLenCounter) >> 1; // divide by 2
    if ((M(Squ2_SfxLenCounter) & 0x01) != 0)
    {
        DecrementSfx2Length(); // alter frequency every other frame
        return;
    }
    y = a;
    a = PowerUpGrabFreqData_data[y - 1]; // use length left over / 2 for frequency offset
    x = 0x5d;                            // store reg contents of power-up grab sound
    y = 0x7f;
    LoadSqu2Regs();
    return;

Cont_CGrab_TTick:
    ContinueCGrabTTick();
    return;

JumpToDecLength2:
    DecrementSfx2Length();
    return;

PlayBowserFall:
    // load length of bowser defeat sound
    writeData(Squ2_SfxLenCounter, 0x38);
    y = 0xc4; // load contents of reg for bowser defeat sound
    a = 0x18;
BlstSJp:
    goto PBFRegs;

ContinueBowserFall:
    a = M(Squ2_SfxLenCounter); // check for almost near the end
    if (a != 0x08)
    {
        DecrementSfx2Length();
        return;
    }
    y = 0xa4; // if so, load the rest of reg contents for bowser defeat sound
    a = 0x5a;

PBFRegs: // the fireworks/gunfire sound shares part of reg contents here
    x = 0x9f;
    LoadSqu2Regs();
    return;

PlayExtraLife:
    a = 0x30; // load length of 1-up sound
    writeData(Squ2_SfxLenCounter, 0x30);

ContinueExtraLife:
    a = M(Squ2_SfxLenCounter);
    x = 0x03; // load new tones only every eight frames

    do // DivLLoop
    {
        shiftedBit = (a & 0x01) != 0;
        a >>= 1;
        if (shiftedBit)
        {
            goto JumpToDecLength2; // if any bits set here, branch to dec the length
        }
        --x;
    } while (x != 0); // do this until all bits checked, if none set, continue
    y = a;
    a = ExtraLifeFreqData_data[y - 1]; // load our reg contents
    x = 0x82;
    y = 0x7f;
    LoadSqu2Regs(); // unconditional branch
    return;

PlayGrowPowerUp:
    a = 0x10; // load length of power-up reveal sound
    if (a == 0)
    {

    PlayGrowVine:
        a = 0x20; // load length of vine grow sound
    } // GrowItemRegs
    writeData(Squ2_SfxLenCounter, a);
    // load contents of reg for both sounds directly
    writeData(SND_SQUARE2_REG + 1, 0x7f);
    a = 0x00; // start secondary counter for both sounds
    writeData(Sfx_SecondaryCounter, 0x00);

ContinueGrowItems:
    ++M(Sfx_SecondaryCounter); // increment secondary counter for both sounds
    // this sound doesn't decrement the usual counter
    a = M(Sfx_SecondaryCounter) >> 1; // divide by 2 to get the offset
    y = a;
    if (y != M(Squ2_SfxLenCounter))
    { // if so, branch to jump, and stop playing sounds
        // load contents of other reg directly
        writeData(SND_SQUARE2_REG, 0x9d);
        a = PUp_VGrow_FreqData_data[y]; // use secondary counter / 2 as offset for frequency regs
        SetFreq_Squ2();
        return;

        //------------------------------------------------------------------------
    } // StopGrowItems
    EmptySfx2Buffer(); // branch to stop playing sounds
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
    LoadEnvelopeData();
    writeData(SND_SQUARE2_REG, a); // based on offset set by first load unless playing
    x = 0x7f;                      // death music or d4 set on secondary buffer
    writeData(SND_SQUARE2_REG + 1, 0x7f);

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
        AlternateLengthHandler();
        writeData(Squ1_NoteLenCounter, a); // save contents of A in square 1 note counter
        y = M(Square1SoundBuffer);         // is there a sound playing on square 1?
        if (y != 0)
        {
            goto HandleTriangleMusic;
        }
        a = x;
        a &= 0b00111110; // change saved data to appropriate note format
        SetFreq_Squ1();  // play the note
        if (a != 0)
        {
            LoadControlRegs();
        } // SkipCtrlL: save envelope offset
        writeData(Squ1_EnvelopeDataCtrl, a);
        Dump_Squ1_Regs();
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
        LoadEnvelopeData();
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
        ProcessLengthData();               // otherwise, it is length data
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
    SetFreq_Tri();
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
    AlternateLengthHandler();
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
            PlayBeat();
            return;
        } // StrongBeat
        a = 0x1c; // strong beat data
        x = 0x0c;
        y = 0x18;
        PlayBeat();
        return;
    } // LongBeat
    a = 0x1c; // long beat data
    x = 0x03;
    y = 0x58;
    PlayBeat();
    return;

SilentBeat:
    a = 0x10; // silence
    PlayBeat();
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
    PlaySqu1Sfx();

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
    ProcessLengthData(); // store length of note
    writeData(Squ2_NoteLenBuffer, a);
    y = M(MusicOffset_Square2); // fetch another byte (MUST NOT BE LENGTH BYTE!)
    ++M(MusicOffset_Square2);
    a = M(W(MusicData) + y);

Squ2NoteHandler:
    x = M(Square2SoundBuffer); // is there a sound playing on this channel?
    if (x == 0)
    {
        SetFreq_Squ2(); // no, then play the note
        if (a != 0)
        {                      // check to see if note is rest
            LoadControlRegs(); // if not, load control regs for square 2
        } // Rest: save contents of A
        writeData(Squ2_EnvelopeDataCtrl, a);
        Dump_Sq2_Regs(); // dump X and Y into square 2 control regs
    } // SkipFqL1: save length in square 2 note counter
    writeData(Squ2_NoteLenCounter, M(Squ2_NoteLenBuffer));
    MiscSqu2MusicTasks();
}
