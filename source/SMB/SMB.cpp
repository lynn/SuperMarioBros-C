// This file was generated from docs/smbdis.asm by the tool in codegen, and has
// since been corrected by hand where the translation was not faithful to the
// console. Regenerating it discards those corrections: see the note in
// codegen/CMakeLists.txt before you do.
//
#include "SMB.hpp"

void SMBEngine::code(int mode)
{
    const uint8_t VRAM_Buffer_Offset_data[] = {
        LOBYTE(VRAM_Buffer1_Offset), LOBYTE(VRAM_Buffer2_Offset)
    };

    const uint8_t VRAM_AddrTable_High_data[] = {
        HIBYTE(VRAM_Buffer1), HIBYTE(WaterPaletteData), HIBYTE(GroundPaletteData),
        HIBYTE(UndergroundPaletteData), HIBYTE(CastlePaletteData), HIBYTE(VRAM_Buffer1_Offset),
        HIBYTE(VRAM_Buffer2), HIBYTE(VRAM_Buffer2), HIBYTE(BowserPaletteData),
        HIBYTE(DaySnowPaletteData), HIBYTE(NightSnowPaletteData), HIBYTE(MushroomPaletteData),
        HIBYTE(MarioThanksMessage), HIBYTE(LuigiThanksMessage), HIBYTE(MushroomRetainerSaved),
        HIBYTE(PrincessSaved1), HIBYTE(PrincessSaved2), HIBYTE(WorldSelectMessage1),
        HIBYTE(WorldSelectMessage2)
    };

    const uint8_t VRAM_AddrTable_Low_data[] = {
        LOBYTE(VRAM_Buffer1), LOBYTE(WaterPaletteData), LOBYTE(GroundPaletteData),
        LOBYTE(UndergroundPaletteData), LOBYTE(CastlePaletteData), LOBYTE(VRAM_Buffer1_Offset),
        LOBYTE(VRAM_Buffer2), LOBYTE(VRAM_Buffer2), LOBYTE(BowserPaletteData),
        LOBYTE(DaySnowPaletteData), LOBYTE(NightSnowPaletteData), LOBYTE(MushroomPaletteData),
        LOBYTE(MarioThanksMessage), LOBYTE(LuigiThanksMessage), LOBYTE(MushroomRetainerSaved),
        LOBYTE(PrincessSaved1), LOBYTE(PrincessSaved2), LOBYTE(WorldSelectMessage1),
        LOBYTE(WorldSelectMessage2)
    };

    bool shiftedBit = false;
    bool carry = false;

    switch (mode)
    {
    case 0:
        loadConstantData();
        goto Start;
    case 1:
        goto NonMaskableInterrupt;
    default:
        bad_jump();
        return;
    }

Start:
    /* sei */ // pretty standard 6502 type init here
    /* cld */
    // init PPU control register 1
    writeData(PPU_CTRL_REG1, 0b00010000);
    // reset stack pointer
    s = 0xff;

    // wait two frames
    do { a = readData(PPU_STATUS); } while ((a & 0x80) == 0);
    do { a = readData(PPU_STATUS); } while ((a & 0x80) == 0);

    y = ColdBootOffset; // load default cold boot pointer
    x = 0x05; // this is where we check for a warm boot

    do // WBootCheck: check each score digit in the top score
    {
        if (M(TopScoreDisplay + x) >= 10)
            goto ColdBoot; // if not, give up and proceed with cold boot
        --x;
    } while ((x & 0x80) == 0);
    // second checkpoint, check to see if
    // another location has a specific value
    if (M(WarmBootValidation) != 0xa5)
        goto ColdBoot;
    y = WarmBootOffset; // if passed both, load warm boot pointer

ColdBoot: // clear memory using pointer in Y
    InitializeMemory();
    writeData(SND_DELTA_REG + 1, 0); // reset delta counter load register
    writeData(OperMode, 0); // reset primary mode of operation
    // set warm boot flag
    writeData(WarmBootValidation, 0xa5);
    writeData(PseudoRandomBitReg, 0xa5); // set seed for pseudorandom register
    writeData(SND_MASTERCTRL_REG, 0b00001111); // enable all sound channels except dmc
    a = 0b00000110;
    writeData(PPU_CTRL_REG2, 0b00000110); // turn off clipping for OAM and background
    MoveAllSpritesOffscreen();
    InitializeNameTables(); // initialize both name tables
    ++M(DisableScreenFlag); // set flag to disable screen output
    a = M(Mirror_PPU_CTRL_REG1) | 0b10000000; // enable NMIs
    WritePPUReg1();

    return; // EndlessLoop: endless loop, need I say more?

NonMaskableInterrupt:
    // disable NMIs in mirror reg
    a = M(Mirror_PPU_CTRL_REG1) & 0b01111111; // save all other bits
    writeData(Mirror_PPU_CTRL_REG1, a);
    a &= 0b01111110; // alter name table address to be $2800
    writeData(PPU_CTRL_REG1, a); // (essentially $2000) but save other bits
    // disable OAM and background display by default
    a = M(Mirror_PPU_CTRL_REG2) & 0b11100110;
    // get screen disable flag
    if (M(DisableScreenFlag) == 0)
    { // if set, used bits as-is
        // otherwise reenable bits and save them
        a = M(Mirror_PPU_CTRL_REG2) | 0b00011110;
    } // ScreenOff: save bits for later but not in register at the moment
    writeData(Mirror_PPU_CTRL_REG2, a);
    a &= 0b11100111; // disable screen for now
    writeData(PPU_CTRL_REG2, a);
    x = readData(PPU_STATUS); // reset flip-flop and reset scroll registers to zero
    a = 0x00;
    InitScroll();
    writeData(PPU_SPR_ADDR, a); // reset spr-ram address register
    // perform spr-ram DMA access on $0200-$02ff
    writeData(SPR_DMA, 0x02);
    x = M(VRAM_Buffer_AddrCtrl); // load control for pointer to buffer contents
    // set indirect at $00 to pointer
    writeData(0x00, VRAM_AddrTable_Low_data[x]);
    writeData(0x01, VRAM_AddrTable_High_data[x]);
    UpdateScreen(); // update screen with buffer contents
    y = 0x00;
    // check for usage of $0341
    if (M(VRAM_Buffer_AddrCtrl) == 0x06)
    {
        y = 0x01; // get offset based on usage
    } // InitBuffer
    x = VRAM_Buffer_Offset_data[y];
    // clear buffer header at last location
    writeData(VRAM_Buffer1_Offset + x, 0x00);
    writeData(VRAM_Buffer1 + x, 0x00);
    writeData(VRAM_Buffer_AddrCtrl, 0x00); // reinit address control to $0301
    // copy mirror of $2001 to register
    writeData(PPU_CTRL_REG2, M(Mirror_PPU_CTRL_REG2));
    SoundEngine(); // play sound
    ReadJoypads(); // read joypads
    PauseRoutine(); // handle pause
    UpdateTopScore();
    if ((M(GamePauseStatus) & 0x01) == 0) // check for pause status
    {
        // if master timer control not set, decrement
        if (M(TimerControl) != 0)
        { // all frame and interval timers
            --M(TimerControl);
            if (M(TimerControl) != 0)
                goto NoDecTimers;
        } // DecTimers: load end offset for end of frame timers
        x = 0x14;
        --M(IntervalTimerControl); // decrement interval timer control,
        if ((M(IntervalTimerControl) & 0x80) == 0)
            goto DecTimersLoop; // if not expired, only frame timers will decrement
        a = 0x14;
        writeData(IntervalTimerControl, 0x14); // if control for interval timers expired,
        x = 0x23; // interval timers will decrement along with frame timers

DecTimersLoop: // check current timer
        if (M(Timers + x) != 0)
        { // if current timer expired, branch to skip,
            --M(Timers + x); // otherwise decrement the current timer
        } // SkipExpTimer: move onto next timer
        --x;
        if ((x & 0x80) == 0)
            goto DecTimersLoop; // do this until all timers are dealt with

NoDecTimers: // increment frame counter
        ++M(FrameCounter);
    } // PauseSkip
    x = 0x00;
    y = 0x07;
    // get first memory location of LSFR bytes
    a = M(PseudoRandomBitReg) & 0b00000010; // mask out all but d1
    writeData(0x00, a); // save here
    // get second memory location
    a = M(PseudoRandomBitReg + 1) & 0b00000010; // mask out all but d1
    a ^= M(0x00); // perform exclusive-OR on d1 from first and second bytes
    carry = a != 0; // set if one or the other is set, clear if neither or both are

RotPRandomBit: // shift the fed-in bit into d7, and the bit that falls out into the next byte
    shiftedBit = (M(PseudoRandomBitReg + x) & 0x01) != 0;
    writeData(PseudoRandomBitReg + x, (uint8_t)((M(PseudoRandomBitReg + x) >> 1) | (carry ? 0x80 : 0x00)));
    carry = shiftedBit;
    ++x; // increment to next byte
    --y; // decrement for loop
    if (y != 0)
        goto RotPRandomBit;
    // check for flag here
    if (M(Sprite0HitDetectFlag) != 0)
    {

        do // Sprite0Clr: wait for sprite 0 flag to clear, which will
        {
            a = readData(PPU_STATUS);
            a &= 0b01000000; // not happen until vblank has ended
        } while (a != 0);
        // if in pause mode, do not bother with sprites at all
        a = M(GamePauseStatus) >> 1;
        if ((M(GamePauseStatus) & 0x01) != 0)
            goto Sprite0Hit;
        MoveSpritesOffscreen();
        SpriteShuffler();

Sprite0Hit: // do sprite #0 hit detection
        a = readData(PPU_STATUS);
        a &= 0b01000000;
        if (a == 0)
            goto Sprite0Hit;
        y = 0x14; // small delay, to wait until we hit horizontal blank time

        do // HBlankDelay
        {
            --y;
        } while (y != 0);
    } // SkipSprite0: set scroll registers from variables
    writeData(PPU_SCROLL_REG, M(HorizontalScroll));
    writeData(PPU_SCROLL_REG, M(VerticalScroll));
    a = M(Mirror_PPU_CTRL_REG1); // load saved mirror of $2000
    pha();
    writeData(PPU_CTRL_REG1, a);
    // if in pause mode, do not perform operation mode stuff
    a = M(GamePauseStatus) >> 1;
    if ((M(GamePauseStatus) & 0x01) == 0)
    {
        OperModeExecutionTree(); // otherwise do one of many, many possible subroutines
    } // SkipMainOper: reset flip-flop
    a = readData(PPU_STATUS);
    pla();
    a |= 0b10000000; // reactivate NMIs
    writeData(PPU_CTRL_REG1, a);
    return; // we are done until the next frame!
}

//------------------------------------------------------------------------

void SMBEngine::PauseRoutine()
{
    a = M(OperMode); // are we in victory mode?
    if (a != VictoryModeValue)
    {
        if (a != GameModeValue)
            return; // if not, leave
        a = M(OperMode_Task); // if we are in game mode, are we running game engine?
        if (a != 0x03)
            return; // if not, leave
    } // ChkPauseTimer: check if pause timer is still counting down
    a = M(GamePauseTimer);
    if (a != 0)
    {
        --M(GamePauseTimer); // if so, decrement and leave
        return;

    //------------------------------------------------------------------------
    } // ChkStart: check to see if start is pressed
    a = M(SavedJoypad1Bits) & Start_Button; // on controller 1
    if (a != 0)
    {
        // check to see if timer flag is set
        a = M(GamePauseStatus) & 0b10000000; // and if so, do not reset timer (residual,
        if (a != 0)
            return; // joypad reading routine makes this unnecessary)
        // set pause timer
        writeData(GamePauseTimer, 0x2b);
        a = M(GamePauseStatus);
        y = a;
        ++y; // set pause sfx queue for next pause mode
        writeData(PauseSoundQueue, y);
        a ^= 0b00000001; // invert d0 and set d7
        a |= 0b10000000;
        goto SetPause; // unconditional branch
    } // ClrPauseTimer: clear timer flag if timer is at zero and start button
    a = M(GamePauseStatus) & 0b01111111; // is not pressed

SetPause:
    writeData(GamePauseStatus, a);

    return; // ExitPause
}

//------------------------------------------------------------------------

void SMBEngine::SpriteShuffler()
{
    y = M(AreaType); // load level type, likely residual code
    a = 0x28; // load preset value which will put it at
    writeData(0x00, 0x28); // sprite #10
    x = 0x0e; // start at the end of OAM data offsets

    do // ShuffleLoop: check for offset value against
    {
        a = M(SprDataOffset + x);
        if (a >= M(0x00))
        { // if less, skip this part
            y = M(SprShuffleAmtOffset); // get current offset to preset value we want to add
            a += M(SprShuffleAmt + y); // get shuffle amount, add to current sprite offset
            if (a < M(SprShuffleAmt + y))
            { // if the add wrapped past $ff, skip second add
                a += M(0x00); // otherwise add preset value $28 to offset
            } // StrSprOffset: store new offset here or old one if branched to here
            writeData(SprDataOffset + x, a);
        } // NextSprOffset: move backwards to next one
        --x;
    } while ((x & 0x80) == 0);
    x = M(SprShuffleAmtOffset); // load offset
    ++x;
    if (x == 0x03)
    { // if offset + 1 not 3, store
        x = 0x00; // otherwise, init to 0
    } // SetAmtOffset
    writeData(SprShuffleAmtOffset, x);
    x = 0x08; // load offsets for values and storage
    y = 0x02;

    do // SetMiscOffset: load one of three OAM data offsets
    {
        a = M(SprDataOffset + 5 + y);
        writeData(Misc_SprDataOffset - 2 + x, a); // store first one unmodified, but
        a += 0x08; // more to the third one
        writeData(Misc_SprDataOffset - 1 + x, a); // note that due to the way X is set up,
        a += 0x08;
        writeData(Misc_SprDataOffset + x, a);
        --x;
        --x;
        --x;
        --y;
    } while ((y & 0x80) == 0); // do this until all misc spr offsets are loaded
    return;
}

//------------------------------------------------------------------------

// start both players at the first area
void SMBEngine::GoContinue()
{
    writeData(WorldNumber, a);
    writeData(OffScr_WorldNumber, a); // of the previously saved world number
    x = 0x00; // note that on power-up using this function
    writeData(AreaNumber, 0x00); // will make no difference
    writeData(OffScr_AreaNumber, 0x00);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawMushroomIcon()
{
    const uint8_t MushroomIconData_data[] = {
        0x07, 0x22, 0x49, 0x83, 0xce, 0x24, 0x24, 0x00
    };

    y = 0x07; // read eight bytes to be read by transfer routine

    do // IconDataRead: note that the default position is set for a
    {
        writeData(VRAM_Buffer1 - 1 + y, MushroomIconData_data[y]); // 1-player game
        --y;
    } while ((y & 0x80) == 0);
    a = M(NumberOfPlayers); // check number of players
    if (a != 0)
    { // if set to 1-player game, we're done
        // otherwise, load blank tile in 1-player position
        writeData(VRAM_Buffer1 + 3, 0x24);
        a = 0xce; // then load shroom icon tile in 2-player position
        writeData(VRAM_Buffer1 + 5, 0xce);
    } // ExitIcon
    return;
}

//------------------------------------------------------------------------

bool SMBEngine::DemoEngine()
{
    const uint8_t DemoTimingData_data[] = {
        0x9b, 0x10, 0x18, 0x05, 0x2c, 0x20, 0x24,
        0x15, 0x5a, 0x10, 0x20, 0x28, 0x30, 0x20, 0x10,
        0x80, 0x20, 0x30, 0x30, 0x01, 0xff, 0x00
    };

    const uint8_t DemoActionData_data[] = {
        0x01, 0x80, 0x02, 0x81, 0x41, 0x80, 0x01,
        0x42, 0xc2, 0x02, 0x80, 0x41, 0xc1, 0x41, 0xc1,
        0x01, 0xc1, 0x01, 0x02, 0x80, 0x00
    };

    bool demoOver = false;

    x = M(DemoAction); // load current demo action
    // load current action timer
    if (M(DemoActionTimer) == 0)
    { // if timer still counting down, skip
        ++x;
        ++M(DemoAction); // if expired, increment action, X, and
        demoOver = true; // demo over by default
        a = DemoTimingData_data[x - 1]; // get next timer
        writeData(DemoActionTimer, a); // store as current timer
        if (a == 0)
            return demoOver; // if timer already at zero, skip
    } // DoAction: get and perform action (current or next)
    a = DemoActionData_data[x - 1];
    writeData(SavedJoypad1Bits, a);
    --M(DemoActionTimer); // decrement action timer
    demoOver = false; // demo still going

    return demoOver; // DemoOver
}


//------------------------------------------------------------------------

void SMBEngine::WritePPUReg1()
{
    writeData(PPU_CTRL_REG1, a); // write contents of A to PPU register 1
    writeData(Mirror_PPU_CTRL_REG1, a); // and its mirror
    return;
}


//------------------------------------------------------------------------

void SMBEngine::InitializeMemory()
{
    x = 0x07; // set initial high byte to $0700-$07ff
    a = 0x00; // set initial low byte to start of page (at $00 of page)
    writeData(0x06, 0x00);

    do // InitPageLoop
    {
        writeData(0x07, x);

        do // InitByteLoop: check to see if we're on the stack ($0100-$01ff)
        {
            if (x == 0x01)
            { // if not, go ahead anyway
                if (y >= 0x60)
                    goto SkipByte; // if so, skip write
            } // InitByte: otherwise, initialize byte with current low byte in Y
            writeData(W(0x06) + y, a);

SkipByte:
            --y;
        } while (y != 0xff);
        --x; // go onto the next page
    } while ((x & 0x80) == 0); // do this until all pages of memory have been erased
    return;
}



//------------------------------------------------------------------------
























//------------------------------------------------------------------------








//------------------------------------------------------------------------














































































//------------------------------------------------------------------------









//------------------------------------------------------------------------

void SMBEngine::MoveAllSpritesOffscreen()
{
    y = 0x00; // this routine moves all sprites off the screen
    Skip_0();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveSpritesOffscreen()
{
    y = 0x04; // this routine moves all but sprite 0
    Skip_0();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Skip_0()
{
    a = 0xf8; // off the screen

    do // SprInitLoop: write 248 into OAM data's Y coordinate
    {
        writeData(Sprite_Y_Position + y, 0xf8);
        ++y; // which will move it off the screen
        ++y;
        ++y;
        ++y;
    } while (y != 0);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::SetupIntermediate()
{
        a = M(BackgroundColorCtrl); // save current background color control
        pha(); // and player status to stack
        a = M(PlayerStatus);
        pha();
        // set background color to black
        writeData(PlayerStatus, 0x00); // and player status to not fiery
        a = 0x02; // this is the ONLY time background color control
        writeData(BackgroundColorCtrl, 0x02); // is set to less than 4
        GetPlayerColors();
        pla(); // we only execute this routine for
        writeData(PlayerStatus, a); // the intermediate lives display
        pla(); // and once we're done, we return bg
        writeData(BackgroundColorCtrl, a); // color ctrl and player status from stack
        IncSubtask(); // then move onto the next task
        return;
}

//------------------------------------------------------------------------

void SMBEngine::GetBackgroundColor()
{
    const uint8_t BGColorCtrl_Addr_data[] = {
        0x00, 0x09, 0x0a, 0x04
    };

    y = M(BackgroundColorCtrl); // check background color control
    if (y != 0)
    { // if not set, increment task and fetch palette
        // put appropriate palette into vram
        writeData(VRAM_Buffer_AddrCtrl, BGColorCtrl_Addr_data[y - 4]); // note that if set to 5-7, $0301 will not be read
    } // NoBGColor: increment to next subtask and plod on through
    ++M(ScreenRoutineTask);

    GetPlayerColors();
    return;
}



//------------------------------------------------------------------------

void SMBEngine::WriteTopStatusLine()
{
    // a = 0x00; // select main status bar
    WriteGameText(TextNumber_TopStatusBarLine); // output it
    IncSubtask(); // onto the next task
    return;
}

//------------------------------------------------------------------------

void SMBEngine::WriteBottomStatusLine()
{
    GetSBNybbles(); // write player's score and coin tally to screen
    x = M(VRAM_Buffer1_Offset);
    // write address for world-area number on screen
    writeData(VRAM_Buffer1 + x, 0x20);
    writeData(VRAM_Buffer1 + 1 + x, 0x73);
    // write length for it
    writeData(VRAM_Buffer1 + 2 + x, 0x03);
    y = M(WorldNumber); // first the world number
    ++y;
    a = y;
    writeData(VRAM_Buffer1 + 3 + x, a);
    // next the dash
    writeData(VRAM_Buffer1 + 4 + x, 0x28);
    y = M(LevelNumber); // next the level number
    ++y; // increment for proper number display
    a = y;
    writeData(VRAM_Buffer1 + 5 + x, a);
    // put null terminator on
    writeData(VRAM_Buffer1 + 6 + x, 0x00);
    a = x; // move the buffer offset up by 6 bytes
    a += 0x06;
    writeData(VRAM_Buffer1_Offset, a);
    IncSubtask();
    return;
}

//------------------------------------------------------------------------

// move onto next task
void SMBEngine::IncSubtask()
{
    ++M(ScreenRoutineTask);
    return;
}












//------------------------------------------------------------------------

void SMBEngine::JumpEngine()
{
    a <<= 1; // shift bit from contents of A
    y = a;
    pla(); // pull saved return address from stack
    writeData(0x04, a); // save to indirect
    pla();
    writeData(0x05, a);
    ++y;
    a = M(W(0x04) + y); // load pointer from indirect
    writeData(0x06, a); // note that if an RTS is performed in next routine
    ++y; // it will return to the execution before the sub
    a = M(W(0x04) + y); // that called this routine
    writeData(0x07, a);
     // jump to the address we loaded

    InitializeNameTables();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitializeNameTables()
{
    a = readData(PPU_STATUS); // reset flip-flop
    // load mirror of ppu reg $2000
    a = M(Mirror_PPU_CTRL_REG1) | 0b00010000; // set sprites for first 4k and background for second 4k
    a &= 0b11110000; // clear rest of lower nybble, leave higher alone
    WritePPUReg1();
    a = 0x24; // set vram address to start of name table 1
    WriteNTAddr();
    a = 0x20; // and then set it to name table 0

    WriteNTAddr();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::WriteNTAddr()
{
    writeData(PPU_ADDRESS, a);
    writeData(PPU_ADDRESS, 0x00);
    x = 0x04; // clear name table with blank tile #24
    y = 0xc0;
    a = 0x24;

    InitNTLoop();
    return;
}

//------------------------------------------------------------------------

// count out exactly 768 tiles
void SMBEngine::InitNTLoop()
{
InitNTLoop:
    writeData(PPU_DATA, a);
    --y;
    if (y != 0)
        goto InitNTLoop;
    --x;
    if (x != 0)
        goto InitNTLoop;
    y = 64; // now to clear the attribute table (with zero this time)
    a = x;
    writeData(VRAM_Buffer1_Offset, a); // init vram buffer 1 offset
    writeData(VRAM_Buffer1, a); // init vram buffer 1

    do // InitATLoop
    {
        writeData(PPU_DATA, a);
        --y;
    } while (y != 0);
    writeData(HorizontalScroll, a); // reset scroll variables
    writeData(VerticalScroll, a);
    InitScroll(); // initialize scroll registers to zero
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ReadJoypads()
{
    // reset and clear strobe of joypad ports
    writeData(JOYPAD_PORT, 0x01);
    a = 0x00;
    x = 0x00; // start with joypad 1's port
    writeData(JOYPAD_PORT, 0x00);
    ReadPortBits();
    ++x; // increment for joypad 2's port

    ReadPortBits();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ReadPortBits()
{
    bool shiftedBit = false;

    y = 0x08;

    do // PortLoop: push previous bit onto stack
    {
        pha();
        a = readData(JOYPAD_PORT + x); // read current bit on joypad port
        writeData(0x00, a); // check d1 and d0 of port output
        a >>= 1; // this is necessary on the old
        a |= M(0x00); // famicom systems in japan
        shiftedBit = (a & 0x01) != 0; // this is the bit the port read
        pla(); // read bits from stack
        a = (uint8_t)((a << 1) | (shiftedBit ? 1 : 0)); // and shift it in
        --y;
    } while (y != 0); // count down bits left
    writeData(SavedJoypadBits + x, a); // save controller status here always
    pha();
    a &= 0b00110000; // check for select or start
    a &= M(JoypadBitMask + x); // if neither saved state nor current state
    if (a != 0)
    { // have any of these two set, branch
        pla();
        a &= 0b11001111; // otherwise store without select
        writeData(SavedJoypadBits + x, a); // or start bits and leave
        return;

    //------------------------------------------------------------------------
    } // Save8Bits
    pla();
    writeData(JoypadBitMask + x, a); // save with all bits in another place and leave
    return;
}

//------------------------------------------------------------------------

// store contents of A into scroll registers
void SMBEngine::InitScroll()
{
    writeData(PPU_SCROLL_REG, a);
    writeData(PPU_SCROLL_REG, a); // and end whatever subroutine led us here
    return;
}



//------------------------------------------------------------------------

void SMBEngine::UpdateTopScore()
{
    x = 0x05; // start with mario's score
    TopScoreCheck();
    x = 0x0b; // now do luigi's score

    TopScoreCheck();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::TopScoreCheck()
{
    bool borrow = false;

    y = 0x05; // start with the lowest digit
    borrow = false;

    do // GetScoreDiff: subtract each player digit from each high score digit
    {
        int diff = M(PlayerScoreDisplay + x) - M(TopScoreDisplay + y) - (borrow ? 1 : 0);
        a = (uint8_t)diff; // from lowest to highest, if any top score digit exceeds
        borrow = diff < 0; // any player digit, the borrow stays set until a subsequent
        --x; // subtraction clears it (player digit is higher than top)
        --y;
    } while ((y & 0x80) == 0);
    if (!borrow)
    { // if the whole score still borrowed, no new high score
        ++x; // increment X and Y once to the start of the score
        ++y;

        do // CopyScore: store player's score digits into high score memory area
        {
            a = M(PlayerScoreDisplay + x);
            writeData(TopScoreDisplay + y, a);
            ++x;
            ++y;
        } while (y < 0x06);
    } // NoTopSc
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitializeGame()
{
    y = 0x6f; // clear all memory as in initialization procedure,
    InitializeMemory(); // but this time, clear only as far as $076f
    y = 0x1f;

    do // ClrSndLoop: clear out memory used
    {
        writeData(SoundMemory + y, 0);
        --y; // by the sound engines
    } while ((y & 0x80) == 0);
    a = 0x18; // set demo timer
    writeData(DemoTimer, 0x18);
    LoadAreaPointer();

    InitializeArea();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitializeArea()
{
    y = 0x4b; // clear all memory again, only as far as $074b
    InitializeMemory(); // this is only necessary if branching from
    x = 0x21;

    do // ClrTimersLoop: clear out memory between
    {
        writeData(Timers + x, 0x00);
        --x; // $0780 and $07a1
    } while ((x & 0x80) == 0);
    a = M(HalfwayPage);
    y = M(AltEntranceControl); // if AltEntranceControl not set, use halfway page, if any found
    if (y != 0)
    {
        a = M(EntrancePage); // otherwise use saved entry page number here
    } // StartPage: set as value here
    writeData(ScreenLeft_PageLoc, a);
    writeData(CurrentPageLoc, a); // also set as current page
    writeData(BackloadingFlag, a); // set flag here if halfway page or saved entry page number found
    GetScreenPosition(); // get pixel coordinates for screen borders
    y = 0x20; // if on odd numbered page, use $2480 as start of rendering
    a &= 0b00000001; // otherwise use $2080, this address used later as name table
    if (a != 0)
    { // address for rendering of game area
        y = 0x24;
    } // SetInitNTHigh: store name table address
    writeData(CurrentNTAddr_High, y);
    y = 0x80;
    writeData(CurrentNTAddr_Low, 0x80);
    a <<= 1; // store LSB of page number in high nybble
    a <<= 1; // of block buffer column position
    a <<= 1;
    a <<= 1;
    writeData(BlockBufferColumnPos, a);
    --M(AreaObjectLength); // set area object lengths for all empty
    --M(AreaObjectLength + 1);
    --M(AreaObjectLength + 2);
    a = 0x0b; // set value for renderer to update 12 column sets
    writeData(ColumnSets, 0x0b); // 12 column sets = 24 metatile columns = 1 1/2 screens
    GetAreaDataAddrs(); // get enemy and level addresses and load header
    // check to see if primary hard mode has been activated
    if (M(PrimaryHardMode) != 0)
        goto SetSecHard; // if so, activate the secondary no matter where we're at
    a = M(WorldNumber); // otherwise check world number
    if (a < World5)
        goto CheckHalfway;
    if (a != World5)
        goto SetSecHard; // if not equal to, then world > 5, thus activate
    // otherwise, world 5, so check level number
    if (M(LevelNumber) < Level3)
        goto CheckHalfway;

SetSecHard: // set secondary hard mode flag for areas 5-3 and beyond
    ++M(SecondaryHardMode);

CheckHalfway:
    if (M(HalfwayPage) != 0)
    {
        a = 0x02; // if halfway page set, overwrite start position from header
        writeData(PlayerEntranceCtrl, 0x02);
    } // DoneInitArea: silence music
    writeData(AreaMusicQueue, Silence);
    a = 0x01; // disable screen output
    writeData(DisableScreenFlag, 0x01);
    ++M(OperMode_Task); // increment one of the modes
    return;
}

//------------------------------------------------------------------------

void SMBEngine::SetupGameOver()
{
    // reset screen routine task control for title screen, game,
    writeData(ScreenRoutineTask, 0x00); // and game over modes
    writeData(Sprite0HitDetectFlag, 0x00); // disable sprite 0 check
    a = GameOverMusic;
    writeData(EventMusicQueue, GameOverMusic); // put game over music in secondary queue
    ++M(DisableScreenFlag); // disable screen output
    ++M(OperMode_Task); // set secondary mode to 1
    return;
}

























//------------------------------------------------------------------------

void SMBEngine::GetAreaDataAddrs()
{
    const uint8_t AreaDataAddrHigh_data[] = {
        HIBYTE(L_WaterArea1), HIBYTE(L_WaterArea2), HIBYTE(L_WaterArea3), HIBYTE(L_GroundArea1), HIBYTE(L_GroundArea2), HIBYTE(L_GroundArea3),
        HIBYTE(L_GroundArea4), HIBYTE(L_GroundArea5), HIBYTE(L_GroundArea6), HIBYTE(L_GroundArea7), HIBYTE(L_GroundArea8), HIBYTE(L_GroundArea9),
        HIBYTE(L_GroundArea10), HIBYTE(L_GroundArea11), HIBYTE(L_GroundArea12), HIBYTE(L_GroundArea13), HIBYTE(L_GroundArea14), HIBYTE(L_GroundArea15),
        HIBYTE(L_GroundArea16), HIBYTE(L_GroundArea17), HIBYTE(L_GroundArea18), HIBYTE(L_GroundArea19), HIBYTE(L_GroundArea20), HIBYTE(L_GroundArea21),
        HIBYTE(L_GroundArea22), HIBYTE(L_UndergroundArea1), HIBYTE(L_UndergroundArea2), HIBYTE(L_UndergroundArea3), HIBYTE(L_CastleArea1),
        HIBYTE(L_CastleArea2), HIBYTE(L_CastleArea3), HIBYTE(L_CastleArea4), HIBYTE(L_CastleArea5), HIBYTE(L_CastleArea6)
    };

    const uint8_t AreaDataAddrLow_data[] = {
        LOBYTE(L_WaterArea1), LOBYTE(L_WaterArea2), LOBYTE(L_WaterArea3), LOBYTE(L_GroundArea1), LOBYTE(L_GroundArea2), LOBYTE(L_GroundArea3),
        LOBYTE(L_GroundArea4), LOBYTE(L_GroundArea5), LOBYTE(L_GroundArea6), LOBYTE(L_GroundArea7), LOBYTE(L_GroundArea8), LOBYTE(L_GroundArea9),
        LOBYTE(L_GroundArea10), LOBYTE(L_GroundArea11), LOBYTE(L_GroundArea12), LOBYTE(L_GroundArea13), LOBYTE(L_GroundArea14), LOBYTE(L_GroundArea15),
        LOBYTE(L_GroundArea16), LOBYTE(L_GroundArea17), LOBYTE(L_GroundArea18), LOBYTE(L_GroundArea19), LOBYTE(L_GroundArea20), LOBYTE(L_GroundArea21),
        LOBYTE(L_GroundArea22), LOBYTE(L_UndergroundArea1), LOBYTE(L_UndergroundArea2), LOBYTE(L_UndergroundArea3), LOBYTE(L_CastleArea1),
        LOBYTE(L_CastleArea2), LOBYTE(L_CastleArea3), LOBYTE(L_CastleArea4), LOBYTE(L_CastleArea5), LOBYTE(L_CastleArea6)
    };

    const uint8_t AreaDataHOffsets_data[] = {
        0x00, 0x03, 0x19, 0x1c
    };

    const uint8_t EnemyDataAddrHigh_data[] = {
        HIBYTE(E_CastleArea1), HIBYTE(E_CastleArea2), HIBYTE(E_CastleArea3), HIBYTE(E_CastleArea4), HIBYTE(E_CastleArea5), HIBYTE(E_CastleArea6),
        HIBYTE(E_GroundArea1), HIBYTE(E_GroundArea2), HIBYTE(E_GroundArea3), HIBYTE(E_GroundArea4), HIBYTE(E_GroundArea5), HIBYTE(E_GroundArea6),
        HIBYTE(E_GroundArea7), HIBYTE(E_GroundArea8), HIBYTE(E_GroundArea9), HIBYTE(E_GroundArea10), HIBYTE(E_GroundArea11), HIBYTE(E_GroundArea12),
        HIBYTE(E_GroundArea13), HIBYTE(E_GroundArea14), HIBYTE(E_GroundArea15), HIBYTE(E_GroundArea16), HIBYTE(E_GroundArea17), HIBYTE(E_GroundArea18),
        HIBYTE(E_GroundArea19), HIBYTE(E_GroundArea20), HIBYTE(E_GroundArea21), HIBYTE(E_GroundArea22), HIBYTE(E_UndergroundArea1),
        HIBYTE(E_UndergroundArea2), HIBYTE(E_UndergroundArea3), HIBYTE(E_WaterArea1), HIBYTE(E_WaterArea2), HIBYTE(E_WaterArea3)
    };

    const uint8_t EnemyDataAddrLow_data[] = {
        LOBYTE(E_CastleArea1), LOBYTE(E_CastleArea2), LOBYTE(E_CastleArea3), LOBYTE(E_CastleArea4), LOBYTE(E_CastleArea5), LOBYTE(E_CastleArea6),
        LOBYTE(E_GroundArea1), LOBYTE(E_GroundArea2), LOBYTE(E_GroundArea3), LOBYTE(E_GroundArea4), LOBYTE(E_GroundArea5), LOBYTE(E_GroundArea6),
        LOBYTE(E_GroundArea7), LOBYTE(E_GroundArea8), LOBYTE(E_GroundArea9), LOBYTE(E_GroundArea10), LOBYTE(E_GroundArea11), LOBYTE(E_GroundArea12),
        LOBYTE(E_GroundArea13), LOBYTE(E_GroundArea14), LOBYTE(E_GroundArea15), LOBYTE(E_GroundArea16), LOBYTE(E_GroundArea17), LOBYTE(E_GroundArea18),
        LOBYTE(E_GroundArea19), LOBYTE(E_GroundArea20), LOBYTE(E_GroundArea21), LOBYTE(E_GroundArea22), LOBYTE(E_UndergroundArea1),
        LOBYTE(E_UndergroundArea2), LOBYTE(E_UndergroundArea3), LOBYTE(E_WaterArea1), LOBYTE(E_WaterArea2), LOBYTE(E_WaterArea3)
    };

    const uint8_t EnemyAddrHOffsets_data[] = {
        0x1f, 0x06, 0x1c, 0x00
    };

    uint32_t wide = 0;

    a = M(AreaPointer); // use 2 MSB for Y
    GetAreaType();
    y = a;
    // mask out all but 5 LSB
    a = M(AreaPointer) & 0b00011111;
    writeData(AreaAddrsLOffset, a); // save as low offset
    a = EnemyAddrHOffsets_data[y]; // load base value with 2 altered MSB,
    a += M(AreaAddrsLOffset); // becomes offset for level data
    y = a;
    // use offset to load pointer
    writeData(EnemyDataLow, EnemyDataAddrLow_data[y]);
    writeData(EnemyDataHigh, EnemyDataAddrHigh_data[y]);
    y = M(AreaType); // use area type as offset
    a = AreaDataHOffsets_data[y]; // do the same thing but with different base value
    a += M(AreaAddrsLOffset);
    y = a;
    // use this offset to load another pointer
    writeData(AreaDataLow, AreaDataAddrLow_data[y]);
    writeData(AreaDataHigh, AreaDataAddrHigh_data[y]);
    y = 0x00; // load first byte of header
    a = M(W(AreaData) + 0x00);
    pha(); // save it to the stack for now
    a &= 0b00000111; // save 3 LSB for foreground scenery or bg color control
    if (a >= 0x04)
    {
        writeData(BackgroundColorCtrl, a); // if 4 or greater, save value here as bg color control
        a = 0x00;
    } // StoreFore: if less, save value here as foreground scenery
    writeData(ForegroundScenery, a);
    pla(); // pull byte from stack and push it back
    pha();
    a &= 0b00111000; // save player entrance control bits
    a >>= 1; // shift bits over to LSBs
    a >>= 1;
    a >>= 1;
    writeData(PlayerEntranceCtrl, a); // save value here as player entrance control
    pla(); // pull byte again but do not push it back
    a &= 0b11000000; // save 2 MSB for game timer setting
    a >>= 6; // and move them over to the LSBs
    writeData(GameTimerSetting, a); // save value here as game timer setting
    ++y;
    a = M(W(AreaData) + y); // load second byte of header
    pha(); // save to stack
    a &= 0b00001111; // mask out all but lower nybble
    writeData(TerrainControl, a);
    pla(); // pull and push byte to copy it to A
    pha();
    a &= 0b00110000; // save 2 MSB for background scenery type
    a >>= 1;
    a >>= 1; // shift bits to LSBs
    a >>= 1;
    a >>= 1;
    writeData(BackgroundScenery, a); // save as background scenery
    pla();
    a &= 0b11000000;
    a >>= 6; // move the bits over to the LSBs
    if (a == 0b00000011)
    { // and nullify other value
        writeData(CloudTypeOverride, a); // otherwise store value in other place
        a = 0x00;
    } // StoreStyle
    writeData(AreaStyle, a);
    // increment area data address by 2 bytes
    wide = ((M(AreaDataHigh) << 8) | M(AreaDataLow)) + 0x02;
    writeData(AreaDataLow, LOBYTE(wide));
    writeData(AreaDataHigh, HIBYTE(wide));
    a = HIBYTE(wide);
    return;
}























//------------------------------------------------------------------------

void SMBEngine::ResidualGravityCode()
{
    y = 0x00; // this part appears to be residual,
    Skip_6();
    return;
}





























//------------------------------------------------------------------------

void SMBEngine::ResidualMiscObjectCode()
{
    a = x;
    a += 0x0d; // miscellaneous objects
    x = a;
    y = 0x1b; // supposedly used once to set offset for block buffer data
    ResJmpM(); // probably used in early stages to do misc to bg collision detection
    return;
}













//------------------------------------------------------------------------

void SMBEngine::DrawPlayer_Intermediate()
{
    const uint8_t IntermediatePlayerData_data[] = {
        0x58, 0x01, 0x00, 0x60, 0xff, 0x04
    };

    x = 0x05; // store data into zero page memory

    do // PIntLoop: load data to display player as he always
    {
        writeData(0x02 + x, IntermediatePlayerData_data[x]); // appears on world/lives display
        --x;
    } while ((x & 0x80) == 0); // do this until all data is loaded
    x = 0xb8; // load offset for small standing
    y = 0x04; // load sprite data offset
    DrawPlayerLoop(); // draw player accordingly
    // get empty sprite attributes
    a = M(Sprite_Attributes + 36) | 0b01000000; // set horizontal flip bit for bottom-right sprite
    writeData(Sprite_Attributes + 32, a); // store and leave
    return;
}






















































//------------------------------------------------------------------------

void SMBEngine::DisplayTimeUp()
{
    a = M(GameTimerExpiredFlag); // if game timer not expired, increment task
    if (a != 0)
    { // control 2 tasks forward, otherwise, stay here
        writeData(GameTimerExpiredFlag, 0x00); // reset timer expiration flag
        // a = 0x02; // output time-up screen to buffer
        OutputInter(TextNumber_TimeUp);
        return;
    } // NoTimeUp: increment control task 2 tasks forward
    ++M(ScreenRoutineTask);
    IncSubtask();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::OutputInter(uint8_t text_number)
{
    WriteGameText(text_number);
    ResetScreenTimer();
    a = 0x00;
    writeData(DisableScreenFlag, 0x00); // reenable screen output
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ResetSpritesAndScreenTimer()
{
    a = M(ScreenTimer); // check if screen timer has expired
    if (a != 0)
        return;

    MoveAllSpritesOffscreen(); // if so reset sprites now
    ResetScreenTimer();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ResetScreenTimer()
{
    a = 0x07; // reset timer again
    writeData(ScreenTimer, 0x07);
    ++M(ScreenRoutineTask); // move onto next task
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DisplayIntermediate()
{
    a = M(OperMode); // check primary mode of operation
    if (a == 0)
        goto NoInter; // if in title screen mode, skip this
    if (a != GameOverModeValue)
    { // if so, proceed to display game over screen
        // otherwise check for mode of alternate entry
        if (M(AltEntranceControl) != 0)
            goto NoInter; // and branch if found
        y = M(AreaType); // check if we are on castle level
        if (y != 0x03)
        {
            // if this flag is set, skip intermediate lives display
            if (M(DisableIntermediate) != 0)
                goto NoInter; // and jump to specific task, otherwise
        } // PlayerInter: put player in appropriate place for
        DrawPlayer_Intermediate();
        // a = 0x01; // lives display, then output lives display to buffer
        OutputInter(TextNumber_WorldLivesDisplay);
        return;

    //------------------------------------------------------------------------
    } // GameOverInter: set screen timer
    writeData(ScreenTimer, 0x12);
    // a = 0x03; // output game over screen to buffer
    WriteGameText(TextNumber_GameOver);
    ++M(OperMode_Task); // inlined
    return;

NoInter: // set for specific task and leave
    a = 0x08;
    writeData(ScreenRoutineTask, 0x08);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ClearBuffersDrawIcon()
{
    a = M(OperMode); // check game mode
    if (a != 0)
    { // if not title screen mode, leave
        ++M(OperMode_Task);
        return;
    }
    x = 0x00; // otherwise, clear buffer space

    do // TScrClear
    {
        writeData(VRAM_Buffer1 - 1 + x, a);
        writeData(VRAM_Buffer1 - 1 + 0x100 + x, a);
        --x;
    } while (x != 0);
    DrawMushroomIcon(); // draw player select icon

    IncSubtask();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::WriteTopScore()
{
    a = 0xfa; // run display routine to display top score on title
    UpdateNumber();
    // move onto next mode
    ++M(OperMode_Task);
    return;
}



//------------------------------------------------------------------------





//------------------------------------------------------------------------


//------------------------------------------------------------------------



//------------------------------------------------------------------------

void SMBEngine::SetupVictoryMode()
{
    x = M(ScreenRight_PageLoc); // get page location of right side of screen
    ++x; // increment to next page
    writeData(DestinationPageLoc, x); // store here
    a = EndOfCastleMusic;
    writeData(EventMusicQueue, EndOfCastleMusic); // play win castle music
    ++M(OperMode_Task); // jump to set next major task in victory mode
    return;
}















//------------------------------------------------------------------------




































//------------------------------------------------------------------------

void SMBEngine::PrimaryGameSetup()
{
    writeData(FetchNewGameTimerFlag, 0x01); // set flag to load game timer from header
    writeData(PlayerSize, 0x01); // set player's size to small
    a = 0x02;
    writeData(NumberofLives, 0x02); // give each player three lives
    writeData(OffScr_NumberofLives, 0x02);
    SecondaryGameSetup();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::SecondaryGameSetup()
{
    const uint8_t Sprite0Data_data[] = {
        0x18, 0xff, 0x23, 0x58
    };

    const uint8_t DefaultSprOffsets_data[] = {
        0x04, 0x30, 0x48, 0x60, 0x78, 0x90, 0xa8, 0xc0,
        0xd8, 0xe8, 0x24, 0xf8, 0xfc, 0x28, 0x2c
    };

    writeData(DisableScreenFlag, 0x00); // enable screen output
    y = 0x00;

    do // ClearVRLoop: clear buffer at $0300-$03ff
    {
        writeData(VRAM_Buffer1 - 1 + y, 0x00);
        ++y;
    } while (y != 0);
    writeData(GameTimerExpiredFlag, 0x00); // clear game timer exp flag
    writeData(DisableIntermediate, 0x00); // clear skip lives display flag
    writeData(BackloadingFlag, 0x00); // clear value here
    writeData(BalPlatformAlignment, 0xff); // initialize balance platform assignment flag
    // put the LSB of the left side page location into d0 of the ppu register #1
    // mirror, to set the proper PPU name table
    a = (uint8_t)((M(Mirror_PPU_CTRL_REG1) & 0xfe) | (M(ScreenLeft_PageLoc) & 0x01));
    writeData(Mirror_PPU_CTRL_REG1, a);
    GetAreaMusic(); // load proper music into queue
    // load sprite shuffle amounts to be used later
    writeData(SprShuffleAmt + 2, 0x38);
    writeData(SprShuffleAmt + 1, 0x48);
    a = 0x58;
    writeData(SprShuffleAmt, 0x58);
    x = 0x0e; // load default OAM offsets into $06e4-$06f2

    do // ShufAmtLoop
    {
        writeData(SprDataOffset + x, DefaultSprOffsets_data[x]);
        --x; // do this until they're all set
    } while ((x & 0x80) == 0);
    y = 0x03; // set up sprite #0

    do // ISpr0Loop
    {
        a = Sprite0Data_data[y];
        writeData(Sprite_Data + y, a);
        --y;
    } while ((y & 0x80) == 0);
    DoNothing2(); // these don't do anything useful
    DoNothing1();
    ++M(Sprite0HitDetectFlag); // set sprite #0 check flag
    ++M(OperMode_Task); // increment to next task
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DoNothing1()
{
    a = 0xff; // this is residual code, this value is
    writeData(0x06c9, 0xff); // not used anywhere in the program
    DoNothing2();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DoNothing2()
{
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitScreen()
{
    MoveAllSpritesOffscreen(); // initialize all sprites including sprite #0
    InitializeNameTables(); // and erase both name and attribute tables
    a = M(OperMode);
    if (a == 0)
    { // if mode still 0, do not load
        IncSubtask();
        return;
    }
    x = 0x03; // into buffer pointer
    SetVRAMAddr_A();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetAreaPalette()
{
    const uint8_t AreaPalette_data[] = {
        0x01, 0x02, 0x03, 0x04
    };

    y = M(AreaType); // select appropriate palette to load
    x = AreaPalette_data[y]; // based on area type
    SetVRAMAddr_A();
    return;
}

//------------------------------------------------------------------------

// store offset into buffer control
void SMBEngine::SetVRAMAddr_A()
{
    writeData(VRAM_Buffer_AddrCtrl, x);
    IncSubtask();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetAlternatePalette1()
{
    a = M(AreaStyle); // check for mushroom level style
    if (a == 0x01)
    {
        a = 0x0b; // if found, load appropriate palette
        writeData(VRAM_Buffer_AddrCtrl, a);
    } // NoAltPal: now onto the next task
    IncSubtask();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawTitleScreen()
{
    a = M(OperMode); // are we in title screen mode?
    if (a != 0)
    {
        ++M(OperMode_Task); // inlined
        return;
    }
    a = HIBYTE(TitleScreenDataOffset); // load address $1ec0 into
    writeData(PPU_ADDRESS, a); // the vram address register
    a = LOBYTE(TitleScreenDataOffset);
    writeData(PPU_ADDRESS, a);
    // put address $0300 into
    writeData(0x01, 0x03); // the indirect at $00
    y = 0x00;
    writeData(0x00, 0x00);
    a = readData(PPU_DATA); // do one garbage read

OutputTScr: // get title screen from chr-rom
    a = readData(PPU_DATA);
    writeData(W(0x00) + y, a); // store 256 bytes into buffer
    ++y;
    if (y == 0)
    { // if not past 256 bytes, do not increment
        ++M(0x01); // otherwise increment high byte of indirect
    } // ChkHiByte: check high byte?
    if (M(0x01) != 0x04)
        goto OutputTScr; // if not, loop back and do another
    if (y < 0x3a)
        goto OutputTScr; // if not, loop back and do another
    a = 0x05; // set buffer transfer control to $0300,
    // inlined: goto SetVRAMAddr_B; // increment task and exit
    writeData(VRAM_Buffer_AddrCtrl, a);
    IncSubtask();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::UpdateScreen()
{
    bool shiftedBit = false;
    uint32_t wide = 0;

    goto UpdateScreen2;
    do // WriteBufferToScreen
    {
        writeData(PPU_ADDRESS, a); // store high byte of vram address
        ++y;
        // load next byte (second)
        writeData(PPU_ADDRESS, M(W(0x00) + y)); // store low byte of vram address
        ++y;
        a = M(W(0x00) + y); // load next byte (third)
        a <<= 1; // shift to left and save in stack
        pha();
        // load mirror of $2000,
        a = M(Mirror_PPU_CTRL_REG1) | 0b00000100; // set ppu to increment by 32 by default
        if ((M(W(0x00) + y) & 0x80) == 0)
        { // if d7 of third byte was clear, ppu will
            a &= 0b11111011; // only increment by 1
        } // SetupWrites: write to register
        WritePPUReg1();
        pla(); // pull from stack and shift to left again
        shiftedBit = (a & 0x80) != 0;
        a <<= 1;
        if (shiftedBit)
        { // if d6 of third byte was clear, do not repeat byte
            a |= 0b00000010; // otherwise set d1 and increment Y
            ++y;
        } // GetLength: shift back to the right to get proper length
        a >>= 1;
        shiftedBit = (a & 0x01) != 0;
        a >>= 1; // note that d1 was taken above
        x = a;

        do // OutputToVRAM: if d1 was set, repeat loading the same byte
        {
            if (!shiftedBit)
            {
                ++y; // otherwise increment Y to load next byte
            } // RepeatByte: load more data from buffer and write to vram
            writeData(PPU_DATA, M(W(0x00) + y));
            --x; // done writing?
        } while (x != 0);
        wide = ((M(0x01) << 8) | M(0x00)) + y + 1; // add end length plus one to the indirect at $00
        writeData(0x00, LOBYTE(wide)); // to allow this routine to read another set of updates
        writeData(0x01, HIBYTE(wide));
        a = HIBYTE(wide);
        // sets vram address to $3f00
        writeData(PPU_ADDRESS, 0x3f);
        a = 0x00;
        writeData(PPU_ADDRESS, 0x00);
        writeData(PPU_ADDRESS, 0x00); // then reinitializes it for some reason
        writeData(PPU_ADDRESS, 0x00);

UpdateScreen2: // reset flip-flop
        x = readData(PPU_STATUS);
        y = 0x00; // load first byte from indirect as a pointer
        a = M(W(0x00) + 0x00);
    } while (a != 0); // if byte is zero we have no further updates to make here

    InitScroll();
    return;
}





//------------------------------------------------------------------------
































//------------------------------------------------------------------------

    //------------------------------------------------------------------------

void SMBEngine::PrintVictoryMessages()
{
    uint32_t wide = 0;

    // load secondary message counter
    if (M(SecondaryMsgCounter) != 0)
        goto IncMsgCounter; // if set, branch to increment message counters
    a = M(PrimaryMsgCounter); // otherwise load primary message counter
    if (a == 0)
        goto ThankPlayer; // if set to zero, branch to print first message
    if (a >= 0x09)
        goto IncMsgCounter; // is residual code, counter never reaches 9)
    y = M(WorldNumber); // check world number
    if (y == World8)
    { // if not at world 8, skip to next part
        if (a < 0x03)
            goto IncMsgCounter; // if not at 3 yet (world 8 only), branch to increment
        a -= 0x01; // otherwise subtract one
        goto ThankPlayer; // and skip to next part
    } // MRetainerMsg: check primary message counter
    if (a < 0x02)
        goto IncMsgCounter; // if not at 2 yet (world 1-7 only), branch

ThankPlayer: // put primary message counter into Y
    y = a;
    if (y == 0)
    { // if counter nonzero, skip this part, do not print first message
        // otherwise get player currently on the screen
        if (M(CurrentPlayer) == 0)
            goto EvalForMusic; // if mario, branch
        ++y; // otherwise increment Y once for luigi and
        if (y != 0)
            goto EvalForMusic; // do an unconditional branch to the same place
    } // SecondPartMsg: increment Y to do world 8's message
    ++y;
    if (M(WorldNumber) == World8)
        goto EvalForMusic; // if at world 8, branch to next part
    --y; // otherwise decrement Y for world 1-7's message
    if (y < 0x04)
    { // branch to set victory end timer
        if (y >= 0x03)
            goto IncMsgCounter; // branch to keep counting

EvalForMusic: // if counter not yet at 3 (world 8 only), branch
        if (y == 0x03)
        { // to print message only (note world 1-7 will only
            a = VictoryMusic; // reach this code if counter = 0, and will always branch)
            writeData(EventMusicQueue, VictoryMusic); // otherwise load victory music first (world 8 only)
        } // PrintMsg: put primary message counter in A
        a = y;
        a += 0x0c; // ($0c-$0d = first), ($0e = world 1-7's), ($0f-$12 = world 8's)
        writeData(VRAM_Buffer_AddrCtrl, a); // write message counter to vram address controller

IncMsgCounter:
        wide = ((M(PrimaryMsgCounter) << 8) | M(SecondaryMsgCounter)) + 0x04; // add four to secondary message counter
        writeData(SecondaryMsgCounter, LOBYTE(wide));
        writeData(PrimaryMsgCounter, HIBYTE(wide));
        a = HIBYTE(wide);

        // SetEndTimer: if not reached value yet, branch to leave
        if (a < 0x07)
            return;
    }
    a = 0x06;
    writeData(WorldEndTimer, 0x06); // otherwise set world end timer
    ++M(OperMode_Task);

    return; // ExitMsgs: leave
}







//------------------------------------------------------------------------

void SMBEngine::PlayerEndWorld()
{
    a = M(WorldEndTimer); // check to see if world end timer expired
    if (a != 0)
        return; // branch to leave if not
    y = M(WorldNumber); // check world number
    if (y < World8)
    { // thus branch to read controller
        a = 0x00;
        writeData(AreaNumber, 0x00); // otherwise initialize area number used as offset
        writeData(LevelNumber, 0x00); // and level number control to start at area 1
        writeData(OperMode_Task, 0x00); // initialize secondary mode of operation
        ++M(WorldNumber); // increment world number to move onto the next world
        LoadAreaPointer(); // get area address offset for the next area
        ++M(FetchNewGameTimerFlag); // set flag to load game timer from header
        a = GameModeValue;
        writeData(OperMode, GameModeValue); // set mode of operation to game mode

        return; // EndExitOne: and leave

    //------------------------------------------------------------------------
    } // EndChkBButton
    a = M(SavedJoypad1Bits) | M(SavedJoypad2Bits); // check to see if B button was pressed on
    a &= B_Button; // either controller
    if (a != 0)
    { // branch to leave if not
        // otherwise set world selection flag
        writeData(WorldSelectEnableFlag, 0x01);
        a = 0xff; // remove onscreen player's lives
        writeData(NumberofLives, 0xff);
        TerminateGame(); // do sub to continue other player or end game
    } // EndExitTwo: leave
    return;
}


//------------------------------------------------------------------------

void SMBEngine::RunGameOver()
{
    // reenable screen
    writeData(DisableScreenFlag, 0x00);
    // check controller for start pressed
    a = M(SavedJoypad1Bits) & Start_Button;
    if (a != 0)
    {
        TerminateGame();
        return;
    }
    a = M(ScreenTimer); // if not pressed, wait for
    if (a != 0)
    { // screen timer to expire
        return;
    }
    TerminateGame();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::TerminateGame()
{
    bool endGame = false;

    a = Silence; // silence music
    writeData(EventMusicQueue, Silence);
    endGame = TransposePlayers(); // check if other player can keep
    if (!endGame)
    {
        ContinueGame(); // going, and do so if possible
        return;
    }
    // otherwise put world number of current
    writeData(ContinueWorld, M(WorldNumber)); // player into secret continue function variable
    a = 0x00; // residual ASL instruction
    writeData(OperMode_Task, 0x00); // reset all modes to title screen and
    writeData(ScreenTimer, 0x00); // leave
    writeData(OperMode, 0x00);
    return;
}

//------------------------------------------------------------------------
















//------------------------------------------------------------------------
























//------------------------------------------------------------------------

void SMBEngine::PlayerVictoryWalk()
{
    uint32_t wide = 0;

    y = 0x00; // set value here to not walk player by default
    writeData(VictoryWalkControl, 0x00);
    // get player's page location
    if (M(Player_PageLoc) == M(DestinationPageLoc))
    { // if page locations don't match, branch
        // otherwise get player's horizontal position
        if (M(Player_X_Position) >= 0x60)
            goto DontWalk; // if still on other page, branch ahead
    } // PerformWalk: otherwise increment value and Y
    ++M(VictoryWalkControl);
    y = 0x01; // note Y will be used to walk the player

DontWalk: // put contents of Y in A and
    a = y;
    AutoControlPlayer(); // use A to move player to the right or not
    // check page location of left side of screen
    if (M(ScreenLeft_PageLoc) != M(DestinationPageLoc))
    { // branch if equal to change modes if necessary
        wide = M(ScrollFractional) + 0x80; // do fixed point math on fractional part of scroll
        writeData(ScrollFractional, LOBYTE(wide)); // save fractional movement amount
        a = (uint8_t)(0x01 + HIBYTE(wide)); // one pixel per frame, plus the carry out of the fraction
        y = a; // use as scroll amount
        ScrollScreen(); // do sub to scroll the screen
        UpdScrollVar(); // do another sub to update screen and scroll variables
        ++M(VictoryWalkControl); // increment value to stay in this routine
    } // ExitVWalk: load value set here
    a = M(VictoryWalkControl);
    if (a == 0)
        ++M(OperMode_Task); // if zero, branch to change modes
    return; // otherwise leave
}

//------------------------------------------------------------------------

void SMBEngine::ScreenRoutines()
{
    a = M(ScreenRoutineTask); // run one of the following subroutines
    switch (a)
    {
    case 0:
        InitScreen();
        return;
    case 1:
        SetupIntermediate();
        return;
    case 2:
        WriteTopStatusLine();
        return;
    case 3:
        WriteBottomStatusLine();
        return;
    case 4:
        DisplayTimeUp();
        return;
    case 5:
        ResetSpritesAndScreenTimer();
        return;
    case 6:
        DisplayIntermediate();
        return;
    case 7:
        ResetSpritesAndScreenTimer();
        return;
    case 8:
        AreaParserTaskControl();
        return;
    case 9:
        GetAreaPalette();
        return;
    case 10:
        GetBackgroundColor();
        return;
    case 11:
        GetAlternatePalette1();
        return;
    case 12:
        DrawTitleScreen();
        return;
    case 13:
        ClearBuffersDrawIcon();
        return;
    case 14:
        WriteTopScore();
        return;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

void SMBEngine::AreaParserTaskControl()
{
    ++M(DisableScreenFlag); // turn off screen

    do // TaskLoop: render column set of current area
    {
        AreaParserTaskHandler();
        // check number of tasks
    } while (M(AreaParserTaskNum) != 0); // if tasks still not all done, do another one
    --M(ColumnSets); // do we need to render more column sets?
    if ((M(ColumnSets) & 0x80) != 0)
    {
        ++M(ScreenRoutineTask); // if not, move on to the next task
    } // OutputCol: set vram buffer to output rendered column set
    a = 0x06;
    writeData(VRAM_Buffer_AddrCtrl, 0x06); // on next NMI
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GameOverMode()
{
    switch (M(OperMode_Task))
    {
    case 0:
        SetupGameOver();
        return;
    case 1:
        ScreenRoutines();
        return;
    case 2:
        RunGameOver();
        return;
    default:
        bad_jump();
        return;
    }
}






//------------------------------------------------------------------------





















//------------------------------------------------------------------------














//------------------------------------------------------------------------

// reset game modes, disable
void SMBEngine::ResetTitle()
{
    a = 0x00;
    writeData(OperMode, 0x00); // sprite 0 check and disable
    writeData(OperMode_Task, 0x00); // screen output
    writeData(Sprite0HitDetectFlag, 0x00);
    ++M(DisableScreenFlag);
    return;
}

//------------------------------------------------------------------------

// if timer for demo has expired, reset modes
void SMBEngine::ChkContinue()
{
    bool shiftedBit = false;

    y = M(DemoTimer);
    if (y == 0)
    {
        ResetTitle();
        return;
    }
    shiftedBit = (a & 0x80) != 0;
    if (shiftedBit) // check to see if A button was also pushed
    { // if not, don't load continue function's world number
        a = M(ContinueWorld); // load previously saved world number for secret
        GoContinue(); // continue function when pressing A + start
    } // StartWorld1
    LoadAreaPointer();
    ++M(Hidden1UpFlag); // set 1-up box flag for both players
    ++M(OffScr_Hidden1UpFlag);
    ++M(FetchNewGameTimerFlag); // set fetch new game timer flag
    ++M(OperMode); // set next game mode
    // if world select flag is on, then primary
    writeData(PrimaryHardMode, M(WorldSelectEnableFlag)); // hard mode must be on as well
    writeData(OperMode_Task, 0x00); // set game mode here, and clear demo timer
    writeData(DemoTimer, 0x00);
    x = 0x17;
    a = 0x00;

    do // InitScores: clear player scores and coin displays
    {
        writeData(ScoreAndCoinDisplay + x, 0x00);
        --x;
    } while ((x & 0x80) == 0);
    return;
}



















//------------------------------------------------------------------------



//------------------------------------------------------------------------

void SMBEngine::VictoryModeSubroutines()
{
    switch (M(OperMode_Task))
    {
    case 0:
        BridgeCollapse();
        return;
    case 1:
        SetupVictoryMode();
        return;
    case 2:
        PlayerVictoryWalk();
        return;
    case 3:
        PrintVictoryMessages();
        return;
    case 4:
        PlayerEndWorld();
        return;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

void SMBEngine::BridgeCollapse()
{
    x = M(BowserFront_Offset); // get enemy offset for bowser
    // check enemy object identifier for bowser
    if (M(Enemy_ID + x) != Bowser)
        goto SetM2; // metatile removal not necessary
    writeData(ObjectOffset, x); // store as enemy offset here
    a = M(Enemy_State + x); // if bowser in normal state, skip all of this
    if (a != 0)
    {
        a &= 0b01000000; // if bowser's state has d6 clear, skip to silence music
        if (a == 0)
            goto SetM2;
        // check bowser's vertical coordinate
        if (M(Enemy_Y_Position + x) < 0xe0)
        {
            MoveD_Bowser();
            return;
        }

SetM2: // silence music
        a = Silence;
        writeData(EventMusicQueue, Silence);
        ++M(OperMode_Task); // move onto next secondary mode in autoctrl mode
        KillAllEnemies(); // jump to empty all enemy slots and then leave
        return;
    } // RemoveBridge
    --M(BowserFeetCounter); // decrement timer to control bowser's feet
    if (M(BowserFeetCounter) != 0)
        goto NoBFall; // if not expired, skip all of this
    writeData(BowserFeetCounter, 0x04); // otherwise, set timer now
    a = M(BowserBodyControls) ^ 0x01; // invert bit to control bowser's feet
    writeData(BowserBodyControls, a);
    // put high byte of name table address here for now
    writeData(0x05, 0x22);
    y = M(BridgeCollapseOffset); // get bridge collapse offset here
    // load low byte of name table address and store here
    writeData(0x04, M(BridgeCollapseData + y));
    y = M(VRAM_Buffer1_Offset); // increment vram buffer offset
    ++y;
    x = 0x0c; // set offset for tile data for sub to draw blank metatile
    RemBridge(); // do sub here to remove bowser's bridge metatiles
    x = M(ObjectOffset); // get enemy offset
    MoveVOffset(); // set new vram buffer offset
    // load the fireworks/gunfire sound into the square 2 sfx
    writeData(Square2SoundQueue, Sfx_Blast); // queue while at the same time loading the brick
    // shatter sound into the noise sfx queue thus
    writeData(NoiseSoundQueue, Sfx_BrickShatter); // producing the unique sound of the bridge collapsing
    ++M(BridgeCollapseOffset); // increment bridge collapse offset
    if (M(BridgeCollapseOffset) != 0x0f)
        goto NoBFall; // the end, go ahead and skip this part
    InitVStf(); // initialize whatever vertical speed bowser has
    writeData(Enemy_State + x, 0b01000000); // set bowser's state to one of defeated states (d6 set)
    a = Sfx_BowserFall;
    writeData(Square2SoundQueue, Sfx_BowserFall); // play bowser defeat sound

NoBFall: // jump to code that draws bowser
    BowserGfxHandler();
    return;
}






//------------------------------------------------------------------------



















//------------------------------------------------------------------------

void SMBEngine::OperModeExecutionTree()
{
    // this is the heart of the entire program,
    switch (M(OperMode))
    {
    case 0:
        TitleScreenMode();
        return;
    case 1:
        GameMode();
        return;
    case 2:
        VictoryMode();
        return;
    case 3:
        GameOverMode();
        return;
    default:
        bad_jump();
        return;
    } // most of what goes on starts here
}

//------------------------------------------------------------------------

void SMBEngine::TitleScreenMode()
{
    switch (M(OperMode_Task))
    {
    case 0:
        InitializeGame();
        return;
    case 1:
        ScreenRoutines();
        return;
    case 2:
        PrimaryGameSetup();
        return;
    case 3:
        GameMenuRoutine();
        return;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

void SMBEngine::GameMenuRoutine()
{
    bool demoOver = false;

    y = 0x00;
    // check to see if either player pressed
    a = M(SavedJoypad1Bits) | M(SavedJoypad2Bits); // only the start button (either joypad)
    if (a != Start_Button)
    {
        if (a != A_Button + Start_Button)
            goto ChkSelect; // if not, branch to check select button
    } // StartGame: if either start or A + start, execute here
    ChkContinue();
    return;

ChkSelect: // check to see if the select button was pressed
    if (a != Select_Button)
    { // if so, branch reset demo timer
        // otherwise check demo timer
        if (M(DemoTimer) == 0)
        { // if demo timer not expired, branch to check world selection
            writeData(SelectTimer, a); // set controller bits here if running demo
            demoOver = DemoEngine(); // run through the demo actions
            if (demoOver)
            {
                ResetTitle(); // demo over, thus branch
                return;
            }
            RunDemo(); // otherwise, run game engine for demo
            return;
        } // ChkWorldSel: check to see if world selection has been enabled
        x = M(WorldSelectEnableFlag);
        if (x == 0)
        {
            NullJoypad();
            return;
        }
        if (a != B_Button)
        {
            NullJoypad();
            return;
        }
        ++y; // if so, increment Y and execute same code as select
    } // SelectBLogic: if select or B pressed, check demo timer one last time
    if (M(DemoTimer) == 0)
    {
        ResetTitle(); // if demo timer expired, branch to reset title screen mode
        return;
    }
    // otherwise reset demo timer
    writeData(DemoTimer, 0x18);
    // check select/B button timer
    if (M(SelectTimer) != 0)
    {
        NullJoypad(); // if not expired, branch
        return;
    }
    a = 0x10; // otherwise reset select button timer
    writeData(SelectTimer, 0x10);
    if (y != 0x01)
    { // note this will not be run if world selection is disabled
        // if no, must have been the select button, therefore
        a = M(NumberOfPlayers) ^ 0b00000001; // change number of players and draw icon accordingly
        writeData(NumberOfPlayers, a);
        DrawMushroomIcon();
        NullJoypad();
        return;
    } // IncWorldSel: increment world select number
    x = M(WorldSelectNumber);
    ++x;
    a = x;
    a &= 0b00000111; // mask out higher bits
    writeData(WorldSelectNumber, a); // store as current world select number
    GoContinue();

    do // UpdateShroom: write template for world select in vram buffer
    {
        writeData(VRAM_Buffer1 - 1 + x, M(WSelectBufferTemplate + x)); // do this until all bytes are written
        ++x;
    } while (((x - 0x06) & 0x80) != 0);
    y = M(WorldNumber); // get world number from variable and increment for
    ++y; // proper display, and put in blank byte before
    writeData(VRAM_Buffer1 + 3, y); // null terminator
    NullJoypad();
    return;
}

//------------------------------------------------------------------------

// clear joypad bits for player 1
void SMBEngine::NullJoypad()
{
    a = 0x00;
    writeData(SavedJoypad1Bits, 0x00);
    RunDemo();
    return;
}

//------------------------------------------------------------------------

// run game engine
void SMBEngine::RunDemo()
{
    GameCoreRoutine();
    a = M(GameEngineSubroutine); // check to see if we're running lose life routine
    if (a != 0x06)
        return; // ExitMenu
    ResetTitle();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::VictoryMode()
{
    VictoryModeSubroutines(); // run victory mode subroutines
    // get current task of victory mode
    if (M(OperMode_Task) != 0)
    { // if on bridge collapse, skip enemy processing
        x = 0x00;
        writeData(ObjectOffset, 0x00); // otherwise reset enemy object offset
        EnemiesAndLoopsCore(); // and run enemy code
    } // AutoPlayer: get player's relative coordinates
    RelativePlayerPosition();
    PlayerGfxHandler(); // draw the player, then leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GameMode()
{
    switch (M(OperMode_Task))
    {
    case 0:
        InitializeArea();
        return;
    case 1:
        ScreenRoutines();
        return;
    case 2:
        SecondaryGameSetup();
        return;
    case 3:
        GameCoreRoutine();
        return;
    default:
        bad_jump();
        return;
    }
}
