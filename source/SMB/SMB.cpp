// This file was generated from docs/smbdis.asm by the tool in codegen, and has
// since been corrected by hand where the translation was not faithful to the
// console. Regenerating it discards those corrections: see the note in
// codegen/CMakeLists.txt before you do.
//
#include "SMB.hpp"

// Inputs: none (mode is a C++ parameter, not a 6502 register)
// Outputs: none
void SMBEngine::code(int mode)
{
    switch (mode)
    {
    case 0:
        loadConstantData();
        Start();
        return;
    case 1:
        NonMaskableInterrupt();
        return;
    default:
        bad_jump();
        return;
    }
}

// Inputs: none
// Outputs: none (delegates final register state to WritePPUReg1)
void SMBEngine::Start()
{
    /* sei */ // pretty standard 6502 type init here
    /* cld */
    // init PPU control register 1
    ppu->writeRegister(PPU_CTRL_REG1, 0b00010000);

    // wait two frames
    while ((ppu->readRegister(PPU_STATUS) & 0x80) == 0) {}
    while ((ppu->readRegister(PPU_STATUS) & 0x80) == 0) {}

    // Detect warm boot: normal score digits and WarmBootValidation present
    bool isWarmBoot = M(TopScoreDisplay + 5) < 10 && M(TopScoreDisplay + 4) < 10 && M(TopScoreDisplay + 3) < 10 &&
                      M(TopScoreDisplay + 2) < 10 && M(TopScoreDisplay + 1) < 10 && M(TopScoreDisplay + 0) < 10 &&
                      warmBootValidation_ == 0xa5;

    // On a warm boot, preserve high-score data etc. around 0x7d7 until 0x7fe.
    // On a cold boot, clear all of RAM.
    const uint16_t clearUntil = isWarmBoot ? 0x07d6 : 0x07fe;

    InitializeMemory(clearUntil);
    apu->writeRegister(SND_DELTA_REG + 1, 0); // reset delta counter load register
    operMode_ = TitleScreenModeValue;         // reset primary mode of operation
    // set warm boot flag
    warmBootValidation_ = 0xa5;
    pseudoRandomBitReg_ = 0xa5;                         // set seed for pseudorandom register
    apu->writeRegister(SND_MASTERCTRL_REG, 0b00001111); // enable all sound channels except dmc
    ppu->writeRegister(PPU_CTRL_REG2, 0b00000110);      // turn off clipping for OAM and background
    MoveAllSpritesOffscreen();
    InitializeNameTables();                        // initialize both name tables
    ++disableScreenFlag_;                          // set flag to disable screen output
    WritePPUReg1(mirrorPpuCtrlReg1_ | 0b10000000); // enable NMIs
}

// Inputs: none
// Outputs: none
void SMBEngine::NonMaskableInterrupt()
{
    const uint8_t VRAM_AddrTable_High_data[] = {
        HIBYTE(VRAM_Buffer1),       HIBYTE(WaterPaletteData),    HIBYTE(GroundPaletteData),     HIBYTE(UndergroundPaletteData),
        HIBYTE(CastlePaletteData),  HIBYTE(VRAM_Buffer1_Offset), HIBYTE(VRAM_Buffer2),          HIBYTE(VRAM_Buffer2),
        HIBYTE(BowserPaletteData),  HIBYTE(DaySnowPaletteData),  HIBYTE(NightSnowPaletteData),  HIBYTE(MushroomPaletteData),
        HIBYTE(MarioThanksMessage), HIBYTE(LuigiThanksMessage),  HIBYTE(MushroomRetainerSaved), HIBYTE(PrincessSaved1),
        HIBYTE(PrincessSaved2),     HIBYTE(WorldSelectMessage1), HIBYTE(WorldSelectMessage2)};

    const uint8_t VRAM_AddrTable_Low_data[] = {
        LOBYTE(VRAM_Buffer1),       LOBYTE(WaterPaletteData),    LOBYTE(GroundPaletteData),     LOBYTE(UndergroundPaletteData),
        LOBYTE(CastlePaletteData),  LOBYTE(VRAM_Buffer1_Offset), LOBYTE(VRAM_Buffer2),          LOBYTE(VRAM_Buffer2),
        LOBYTE(BowserPaletteData),  LOBYTE(DaySnowPaletteData),  LOBYTE(NightSnowPaletteData),  LOBYTE(MushroomPaletteData),
        LOBYTE(MarioThanksMessage), LOBYTE(LuigiThanksMessage),  LOBYTE(MushroomRetainerSaved), LOBYTE(PrincessSaved1),
        LOBYTE(PrincessSaved2),     LOBYTE(WorldSelectMessage1), LOBYTE(WorldSelectMessage2)};

    // disable NMIs in mirror reg
    uint8_t ctrlReg1 = mirrorPpuCtrlReg1_ & 0b01111111; // save all other bits
    mirrorPpuCtrlReg1_ = ctrlReg1;
    // alter name table address to be $2800 (essentially $2000) but save other bits
    ppu->writeRegister(PPU_CTRL_REG1, ctrlReg1 & 0b01111110);
    // disable OAM and background display by default
    uint8_t ctrlReg2 = mirrorPpuCtrlReg2_ & 0b11100110;
    // get screen disable flag
    if (disableScreenFlag_ == 0)
    { // if set, used bits as-is
        // otherwise reenable bits and save them
        ctrlReg2 = mirrorPpuCtrlReg2_ | 0b00011110;
    } // ScreenOff: save bits for later but not in register at the moment
    mirrorPpuCtrlReg2_ = ctrlReg2;
    ppu->writeRegister(PPU_CTRL_REG2, ctrlReg2 & 0b11100111); // disable screen for now
    ppu->readRegister(PPU_STATUS);                            // reset flip-flop and reset scroll registers to zero
    InitScroll(0);                                            // scroll to 0,0
    ppu->writeRegister(PPU_SPR_ADDR, 0x00);                   // reset spr-ram address register
    // perform spr-ram DMA access on $0200-$02ff
    ppu->writeDMA(2);
    uint8_t bufferCtrl = vRAM_Buffer_AddrCtrl_; // load control for pointer to buffer contents
    // pointer to the buffer contents
    UpdateScreen((uint16_t)((VRAM_AddrTable_High_data[bufferCtrl] << 8) | VRAM_AddrTable_Low_data[bufferCtrl]));
    // check for usage of $0341
    uint8_t bufferOffset = (vRAM_Buffer_AddrCtrl_ == 6) ? 0x40 : 0; // get offset based on usage
    // clear buffer header at last location
    ram[0x0300 + bufferOffset] = 0;
    ram[VRAM_Buffer1 + bufferOffset] = 0x00;
    vRAM_Buffer_AddrCtrl_ = 0; // reinit address control to $0301
    // copy mirror of $2001 to register
    ppu->writeRegister(PPU_CTRL_REG2, mirrorPpuCtrlReg2_);
    SoundEngine();                      // play sound
    ReadJoypads();                      // read joypads
    PauseRoutine();                     // handle pause
    TopScoreCheck(5);                   // UpdateTopScore: start with mario's score
    TopScoreCheck(11);                  // now do luigi's score
    if ((gamePauseStatus_ & 0x01) == 0) // check for pause status
    {
        // if master timer control not set, decrement
        bool decTimers = true;
        if (timerControl_ != 0)
        { // all frame and interval timers
            --timerControl_;
            decTimers = (timerControl_ == 0);
        }
        if (decTimers)
        {                            // DecTimers: load end offset for end of frame timers
            --intervalTimerControl_; // decrement interval timer control,
            // if the interval timer control has not expired, only the frame timers
            // decrement; if it has, it is reloaded and the interval timers decrement too
            const bool intervalExpired = (intervalTimerControl_ & 0x80) != 0;
            if (intervalExpired)
            {
                intervalTimerControl_ = 20; // the 21 frame rule!
            }
            uint8_t timerIndex = intervalExpired ? 35 : 20;

            do // DecTimersLoop: check current timer
            {
                if (M(Timers + timerIndex) != 0)
                {                             // if current timer expired, branch to skip,
                    --M(Timers + timerIndex); // otherwise decrement the current timer
                } // SkipExpTimer: move onto next timer
                --timerIndex;
            } while ((timerIndex & 0x80) == 0); // do this until all timers are dealt with
        }

        // NoDecTimers: increment frame counter
        ++frameCounter_;
    } // PauseSkip

    // Advance PRNG
    const uint8_t firstBit = pseudoRandomBitReg_ & 2;
    const uint8_t secondBit = M(PseudoRandomBitReg + 1) & 2;
    bool carry = (secondBit ^ firstBit) != 0;

    for (int i = 0; i < 7; i++)
    {
        const bool shiftedBit = (M(PseudoRandomBitReg + i) & 0x01) != 0;
        ram[PseudoRandomBitReg + i] = (uint8_t)((M(PseudoRandomBitReg + i) >> 1) | (carry ? 0x80 : 0x00));
        carry = shiftedBit;
    }

    // check for flag here
    if (sprite0HitDetectFlag_ != 0)
    {
        // Sprite0Clr: wait for sprite 0 flag to clear, which will
        // not happen until vblank has ended
        while ((ppu->readRegister(PPU_STATUS) & 0b01000000) != 0) {}
        // if in pause mode, do not bother with sprites at all
        if ((gamePauseStatus_ & 0x01) == 0)
        {
            MoveSpritesOffscreen();
            SpriteShuffler();
        }

        // Sprite0Hit: do sprite #0 hit detection
        while ((ppu->readRegister(PPU_STATUS) & 0b01000000) == 0) {}

        // small delay, to wait until we hit horizontal blank time
        uint8_t hblankDelay = 0x14;
        do // HBlankDelay
        {
            --hblankDelay;
        } while (hblankDelay != 0);
    } // SkipSprite0: set scroll registers from variables
    ppu->writeRegister(PPU_SCROLL_REG, horizontalScroll_);
    ppu->writeRegister(PPU_SCROLL_REG, verticalScroll_);
    uint8_t savedCtrlReg1 = mirrorPpuCtrlReg1_; // load saved mirror of $2000
    ppu->writeRegister(PPU_CTRL_REG1, savedCtrlReg1);
    // if in pause mode, do not perform operation mode stuff
    if ((gamePauseStatus_ & 0x01) == 0)
    {
        OperModeExecutionTree(); // otherwise do one of many, many possible subroutines
    } // SkipMainOper: reset flip-flop
    ppu->readRegister(PPU_STATUS);                                 // reset flip-flop
    ppu->writeRegister(PPU_CTRL_REG1, savedCtrlReg1 | 0b10000000); // reactivate NMIs
    // we are done until the next frame!
}

// Inputs: none
// Outputs: none
void SMBEngine::PauseRoutine()
{
    const uint8_t operMode = operMode_; // are we in victory mode?
    if (operMode != VictoryModeValue)
    {
        if (operMode != GameModeValue)
        {
            return; // if not, leave
        }
        // if we are in game mode, are we running game engine?
        if (operMode_Task_ != 3)
        {
            return; // if not, leave
        }
    } // ChkPauseTimer: check if pause timer is still counting down
    if (gamePauseTimer_ != 0)
    {
        --gamePauseTimer_; // if so, decrement and leave
        return;

        //------------------------------------------------------------------------
    } // ChkStart: check to see if start is pressed
    uint8_t pauseStatus;
    if ((savedJoypadBits_[0] & Start_Button) != 0) // on controller 1
    {
        // check to see if timer flag is set
        if ((gamePauseStatus_ & 0b10000000) != 0) // and if so, do not reset timer (residual,
        {
            return; // joypad reading routine makes this unnecessary)
        }
        // set pause timer
        gamePauseTimer_ = 43;
        // set pause sfx queue for next pause mode
        pauseSoundQueue_ = gamePauseStatus_ + 1;
        pauseStatus = (gamePauseStatus_ ^ 0b00000001) | 0b10000000; // invert d0 and set d7
    }
    else
    {                                                // ClrPauseTimer: clear timer flag if timer is at zero and start button
        pauseStatus = gamePauseStatus_ & 0b01111111; // is not pressed
    }

    // SetPause
    gamePauseStatus_ = pauseStatus;

    // ExitPause
}

// Inputs: none
// Outputs: none
void SMBEngine::SpriteShuffler()
{
    // the original loaded AreaType here into Y, which is likely residual code: the value is
    // overwritten before it is ever read
    const uint8_t presetOffset = 0x28; // preset value which will put it at sprite #10

    uint8_t sprOffsetIndex = 14; // start at the end of OAM data offsets

    do // ShuffleLoop: check for offset value against
    {
        const uint8_t sprOffset = M(SprDataOffset + sprOffsetIndex);
        if (sprOffset >= presetOffset)
        { // if less, skip this part
            // get current offset to preset value we want to add
            const uint8_t shuffleAmt = M(SprShuffleAmt + sprShuffleAmtOffset_);
            // get shuffle amount, add to current sprite offset
            uint8_t shuffled = sprOffset + shuffleAmt;
            if (shuffled < shuffleAmt)
            {                             // if the add wrapped past $ff,
                shuffled += presetOffset; // add the preset value $28 to the offset
            } // StrSprOffset: store new offset here or old one if branched to here
            ram[SprDataOffset + sprOffsetIndex] = shuffled;
        } // NextSprOffset: move backwards to next one
        --sprOffsetIndex;
    } while ((sprOffsetIndex & 0x80) == 0);

    const uint8_t nextShuffleAmtOffset = sprShuffleAmtOffset_ + 1; // load offset
    // SetAmtOffset: if offset + 1 not 3, store; otherwise, init to 0
    sprShuffleAmtOffset_ = nextShuffleAmtOffset == 3 ? 0 : nextShuffleAmtOffset;

    uint8_t miscOffset = 8; // load offsets for values and storage
    uint8_t sprDataIndex = 2;

    do // SetMiscOffset: load one of three OAM data offsets
    {
        // store the first one unmodified, add eight to the second, and eight more to the third
        const uint8_t base = M(SprDataOffset + 5 + sprDataIndex);
        ram[Misc_SprDataOffset - 2 + miscOffset] = base;
        ram[Misc_SprDataOffset - 1 + miscOffset] = base + 8;
        ram[Misc_SprDataOffset + miscOffset] = base + 16;
        miscOffset -= 3;
        --sprDataIndex;
    } while ((sprDataIndex & 0x80) == 0); // do this until all misc spr offsets are loaded
}

// start both players at the first area
// Inputs: worldNumber = world number to save
// Outputs: none (the original left x = 0; callers that need it set it themselves)
void SMBEngine::GoContinue(uint8_t worldNumber)
{
    worldNumber_ = worldNumber;
    offScr_WorldNumber_ = worldNumber; // of the previously saved world number
    areaNumber_ = 0;                   // note that on power-up using this function
    offScr_AreaNumber_ = 0;            // will make no difference
}

// Inputs: none
// Outputs: none
void SMBEngine::DrawMushroomIcon()
{
    const uint8_t MushroomIconData_data[] = {0x07, 0x22, 0x49, 0x83, 0xce, 0x24, 0x24, 0x00};

    uint8_t iconByte = 7; // read eight bytes to be read by transfer routine

    do // IconDataRead: note that the default position is set for a
    {
        ram[VRAM_Buffer1 - 1 + iconByte] = MushroomIconData_data[iconByte]; // 1-player game
        --iconByte;
    } while ((iconByte & 0x80) == 0);

    if (numberOfPlayers_ != 0)
    {
        // otherwise, load blank tile in 1-player position
        ram[VRAM_Buffer1 + 3] = 0x24;
        ram[VRAM_Buffer1 + 5] = 0xce;
    } // ExitIcon
}

// Inputs: none (reads DemoAction/DemoActionTimer memory)
// Outputs: none (the bool return communicates "demo over", like a status flag)
bool SMBEngine::DemoEngine()
{
    const uint8_t DemoTimingData_data[] = {0x9b, 0x10, 0x18, 0x05, 0x2c, 0x20, 0x24, 0x15, 0x5a, 0x10, 0x20,
                                           0x28, 0x30, 0x20, 0x10, 0x80, 0x20, 0x30, 0x30, 0x01, 0xff, 0x00};

    const uint8_t DemoActionData_data[] = {0x01, 0x80, 0x02, 0x81, 0x41, 0x80, 0x01, 0x42, 0xc2, 0x02, 0x80,
                                           0x41, 0xc1, 0x41, 0xc1, 0x01, 0xc1, 0x01, 0x02, 0x80, 0x00};

    uint8_t action = demoAction_; // load current demo action
    // load current action timer
    if (demoActionTimer_ == 0)
    { // if timer still counting down, skip
        ++action;
        ++demoAction_;                                             // if expired, increment action, X, and
        const uint8_t nextTimer = DemoTimingData_data[action - 1]; // get next timer
        demoActionTimer_ = nextTimer;                              // store as current timer
        if (nextTimer == 0)
        {
            return true; // if timer already at zero, the demo is over
        }
    } // DoAction: get and perform action (current or next)
    savedJoypadBits_[0] = DemoActionData_data[action - 1];
    --demoActionTimer_; // decrement action timer

    return false; // DemoOver: demo still going
}

// Inputs: value = value to write to PPU_CTRL_REG1 (and its mirror)
// Outputs: none
void SMBEngine::WritePPUReg1(uint8_t value)
{
    ppu->writeRegister(PPU_CTRL_REG1, value); // write contents of A to PPU register 1
    mirrorPpuCtrlReg1_ = value;               // and its mirror
}

// Inputs: startOffset = starting low-byte offset within page $07xx to begin clearing at (the outer page
// counter always starts at 7 internally, but the first page's clear range is controlled by the
// incoming startOffset since the inner loop decrements it without resetting it first)
// Outputs: none
// Clear RAM from $0000 up to and including clearUntil, leaving the stack region
// $0160-$01ff intact (as the original 6502 init does).
void SMBEngine::InitializeMemory(uint16_t clearUntil)
{
    for (uint16_t addr = 0; addr <= clearUntil; addr++)
    {
        if (addr < 0x0160 || addr > 0x01ff) { ram[addr] = 0; }
    }

    joypadOverride_ = 0;
    abButtons_ = 0;
    previousAbButtons_ = 0;
    upDownButtons_ = 0;
    leftRightButtons_ = 0;

    if (clearUntil >= GameEngineSubroutine) { gameEngineSubroutine_ = 0; }
    if (clearUntil >= ObjectOffset) { objectOffset_ = 0; }
    if (clearUntil >= FrameCounter) { frameCounter_ = 0; }
    if (clearUntil >= SavedJoypad1Bits) { savedJoypadBits_[0] = 0; }
    if (clearUntil >= SavedJoypad2Bits) { savedJoypadBits_[1] = 0; }
    if (clearUntil >= Mirror_PPU_CTRL_REG1) { mirrorPpuCtrlReg1_ = 0; }
    if (clearUntil >= Mirror_PPU_CTRL_REG2) { mirrorPpuCtrlReg2_ = 0; }

    pauseSoundQueue_ = 0;
    square1SoundQueue_ = 0;
    square2SoundQueue_ = 0;
    noiseSoundQueue_ = 0;
    areaMusicQueue_ = 0;
    eventMusicQueue_ = 0;
    square1SoundBuffer_ = 0;
    square2SoundBuffer_ = 0;
    noiseSoundBuffer_ = 0;
    areaMusicBuffer_ = 0;
    musicOffset_Noise_ = 0;
    musicOffset_Triangle_ = 0;
    musicOffset_Square1_ = 0;
    musicOffset_Square2_ = 0;

    if (clearUntil >= 0x07b0) { musicOffset_Noise_ = 0; }
    if (clearUntil >= 0x07b1) { eventMusicBuffer_ = 0; }
    if (clearUntil >= 0x07b2) { pauseSoundBuffer_ = 0; }
    if (clearUntil >= 0x07b3) { squ2_NoteLenBuffer_ = 0; }
    if (clearUntil >= 0x07b4) { squ2_NoteLenCounter_ = 0; }
    if (clearUntil >= 0x07b5) { squ2_EnvelopeDataCtrl_ = 0; }
    if (clearUntil >= 0x07b6) { squ1_NoteLenCounter_ = 0; }
    if (clearUntil >= 0x07b7) { squ1_EnvelopeDataCtrl_ = 0; }
    if (clearUntil >= 0x07b8) { tri_NoteLenBuffer_ = 0; }
    if (clearUntil >= 0x07b9) { tri_NoteLenCounter_ = 0; }
    if (clearUntil >= 0x07ba) { noise_BeatLenCounter_ = 0; }
    if (clearUntil >= 0x07bb) { squ1_SfxLenCounter_ = 0; }
    if (clearUntil >= 0x07bd) { squ2_SfxLenCounter_ = 0; }
    if (clearUntil >= 0x07be) { sfx_SecondaryCounter_ = 0; }
    if (clearUntil >= 0x07bf) { noise_SfxLenCounter_ = 0; }
}

// Inputs: none
// Outputs: none (delegates to SprInitLoop)
void SMBEngine::MoveAllSpritesOffscreen()
{
    SprInitLoop(0); // this routine moves all sprites off the screen
}

// Inputs: none
// Outputs: none (delegates to SprInitLoop)
void SMBEngine::MoveSpritesOffscreen()
{
    SprInitLoop(4); // this routine moves all but sprite 0 off the screen
}

// Inputs: yOffset = starting OAM Y-coordinate offset (0 for all sprites, 4 to skip sprite 0)
// Outputs: none
void SMBEngine::SprInitLoop(uint8_t yOffset)
{
    uint8_t offset = yOffset;

    do // SprInitLoop: write 248 into OAM data's Y coordinate
    {
        ram[Sprite_Y_Position + offset] = 0xf8;
        offset += 4; // which will move it off the screen
    } while (offset != 0);
}

// Inputs: none
// Outputs: none
void SMBEngine::SetupIntermediate()
{
    uint8_t savedBgColorCtrl = backgroundColorCtrl_; // save current background color control
    uint8_t savedPlayerStatus = playerStatus_;       // and player status to stack
    // set background color to black
    playerStatus_ = 0;        // and player status to not fiery
    backgroundColorCtrl_ = 2; // this is the ONLY time bg color ctrl is set to less than 4
    GetPlayerColors();
    playerStatus_ = savedPlayerStatus;       // we only execute this routine for the intermediate
    backgroundColorCtrl_ = savedBgColorCtrl; // lives display, and once done, restore bg color ctrl and
                                             // player status; then move onto next task
    ++screenRoutineTask_;                    // IncSubtask
}

// Inputs: none
// Outputs: none (delegates to GetPlayerColors)
void SMBEngine::GetBackgroundColor()
{
    const uint8_t BGColorCtrl_Addr_data[] = {0x00, 0x09, 0x0a, 0x04};

    const uint8_t bgColorCtrl = backgroundColorCtrl_; // check background color control
    if (bgColorCtrl != 0)
    { // if not set, increment task and fetch palette
        // put appropriate palette into vram
        vRAM_Buffer_AddrCtrl_ = BGColorCtrl_Addr_data[bgColorCtrl - 4]; // note that if set to 5-7, $0301 will not be read
    } // NoBGColor: increment to next subtask and plod on through
    ++screenRoutineTask_;

    GetPlayerColors();
}

// Inputs: none
// Outputs: none
void SMBEngine::WriteTopStatusLine()
{
    // a = 0; // select main status bar
    WriteGameText(TextNumber_TopStatusBarLine); // output it
    ++screenRoutineTask_;                       // IncSubtask
}

// Inputs: none
// Outputs: none
void SMBEngine::WriteBottomStatusLine()
{
    GetSBNybbles(); // write player's score and coin tally to screen
    const uint8_t bufferOffset = vRAM_Buffer1_Offset_;
    // write address for world-area number on screen
    ram[VRAM_Buffer1 + bufferOffset] = 0x20;
    ram[VRAM_Buffer1 + 1 + bufferOffset] = 0x73;
    // write length for it
    ram[VRAM_Buffer1 + 2 + bufferOffset] = 0x03;
    // first the world number, incremented for proper number display
    ram[VRAM_Buffer1 + 3 + bufferOffset] = worldNumber_ + 1;
    // next the dash
    ram[VRAM_Buffer1 + 4 + bufferOffset] = 0x28;
    // next the level number, likewise incremented
    ram[VRAM_Buffer1 + 5 + bufferOffset] = levelNumber_ + 1;
    // put null terminator on
    ram[VRAM_Buffer1 + 6 + bufferOffset] = 0x00;
    // move the buffer offset up by 6 bytes
    vRAM_Buffer1_Offset_ = bufferOffset + 6;
    ++screenRoutineTask_; // IncSubtask
}

// Inputs: none
// Outputs: none
void SMBEngine::InitializeNameTables()
{
    ppu->readRegister(PPU_STATUS); // reset flip-flop
    // load mirror of ppu reg $2000, set sprites for first 4k and background for second 4k,
    // then clear the rest of the lower nybble, leaving the higher alone
    WritePPUReg1((mirrorPpuCtrlReg1_ | 0b00010000) & 0b11110000);
    WriteNTAddr(0x24); // set vram address to start of name table 1

    WriteNTAddr(0x20); // and then set it to name table 0
}

// Inputs: highByte = high byte of the name table VRAM address to select
// Outputs: none (delegates to InitNTLoop)
void SMBEngine::WriteNTAddr(uint8_t highByte)
{
    ppu->writeRegister(PPU_ADDRESS, highByte);
    ppu->writeRegister(PPU_ADDRESS, 0x00);

    InitNTLoop(0x24, 0x04, 0xc0); // clear name table with blank tile #24
}

// count out exactly 768 tiles
// Inputs: a = tile value to fill the name table with; x, y = loop counters controlling the total
// tile count (see the comment above)
// Outputs: none (delegates final register state through the attribute-table clear and InitScroll)
void SMBEngine::InitNTLoop(uint8_t tile, uint8_t xCount, uint8_t yCount)
{
    uint8_t outerCount = xCount;
    // deliberately not reset per pass: it counts down 0xc0 the first time and wraps to a full
    // 256 on each of the remaining three, for exactly 768 tiles
    uint8_t innerCount = yCount;

    do // InitNTLoop
    {
        do
        {
            ppu->writeRegister(PPU_DATA, tile);
            --innerCount;
        } while (innerCount != 0);
        --outerCount;
    } while (outerCount != 0);

    // the loops above leave X at zero, which is the value everything below is filled with
    vRAM_Buffer1_Offset_ = 0; // init vram buffer 1 offset
    vRAM_Buffer1_ = 0;        // init vram buffer 1

    uint8_t attributeCount = 64; // now to clear the attribute table (with zero this time)
    do                           // InitATLoop
    {
        ppu->writeRegister(PPU_DATA, 0x00);
        --attributeCount;
    } while (attributeCount != 0);
    horizontalScroll_ = 0; // reset scroll variables
    verticalScroll_ = 0;
    InitScroll(0); // initialize scroll registers to zero
}

// Inputs: none
// Outputs: none
void SMBEngine::ReadJoypads()
{
    // reset and clear strobe of joypad ports
    controller1->writeByte(1);
    controller1->writeByte(0);
    ReadPortBits(0); // start with joypad 1's port

    ReadPortBits(1); // increment for joypad 2's port
}

// Inputs: port = joypad port offset (0 = port 1, 1 = port 2)
// Outputs: none
void SMBEngine::ReadPortBits(uint8_t port)
{
    uint8_t accumulated = 0;
    uint8_t bitsLeft = 8;

    do // PortLoop: preserve accumulated bits across the port read
    {
        const uint8_t portValue = readData(JOYPAD_PORT + port); // read current bit on joypad port
        // OR-ing d1 into d0 is necessary on the old famicom systems in japan
        const bool shiftedBit = (((portValue >> 1) | portValue) & 0x01) != 0; // this is the bit the port read
        accumulated = (uint8_t)((accumulated << 1) | (shiftedBit ? 1 : 0));   // and shift it in
        --bitsLeft;
    } while (bitsLeft != 0); // count down bits left

    savedJoypadBits_[port] = accumulated; // save controller status here always
    // check for select or start: if neither the saved state nor the current state
    // have any of these two set, branch
    if ((accumulated & 0b00110000 & M(JoypadBitMask + port)) != 0)
    {
        // otherwise store without select or start bits and leave
        savedJoypadBits_[port] = accumulated & 0b11001111;
        return;

        //------------------------------------------------------------------------
    } // Save8Bits
    ram[JoypadBitMask + port] = accumulated; // save with all bits in another place and leave
}

// store contents of A into scroll registers
// Inputs: value = value to write to both scroll registers
// Outputs: none
void SMBEngine::InitScroll(uint8_t value)
{
    ppu->writeRegister(PPU_SCROLL_REG, value);
    ppu->writeRegister(PPU_SCROLL_REG, value); // and end whatever subroutine led us here
}

// Inputs: scoreOffset = offset into PlayerScoreDisplay for the score to compare (e.g. 5 for Mario,
// 11 for Luigi)
// Outputs: none
void SMBEngine::TopScoreCheck(uint8_t scoreOffset)
{
    uint8_t playerDigit = scoreOffset;
    uint8_t topDigit = 5; // start with the lowest digit
    bool borrow = false;

    do // GetScoreDiff: subtract each player digit from each high score digit
    {
        // from lowest to highest, if any top score digit exceeds any player digit, the borrow
        // stays set until a subsequent subtraction clears it (player digit is higher than top)
        const int diff = M(PlayerScoreDisplay + playerDigit) - M(TopScoreDisplay + topDigit) - (borrow ? 1 : 0);
        borrow = diff < 0;
        --playerDigit;
        --topDigit;
    } while ((topDigit & 0x80) == 0);

    if (!borrow)
    {                  // if the whole score still borrowed, no new high score
        ++playerDigit; // increment X and Y once to the start of the score
        ++topDigit;

        do // CopyScore: store player's score digits into high score memory area
        {
            ram[TopScoreDisplay + topDigit] = M(PlayerScoreDisplay + playerDigit);
            ++playerDigit;
            ++topDigit;
        } while (topDigit < 6);
    } // NoTopSc
}

// Inputs: none
// Outputs: none
void SMBEngine::InitializeGame()
{
    // clear all memory as in initialization procedure, but this time, clear only as far as $076f
    InitializeMemory(0x076f);

    uint8_t soundByte = 0x1f;

    do // ClrSndLoop: clear out memory used
    {
        ram[SoundMemory + soundByte] = 0;
        --soundByte; // by the sound engines
    } while ((soundByte & 0x80) == 0);
    demoTimer_ = 24; // set demo timer
    LoadAreaPointer();

    InitializeArea();
}

// Inputs: none
// Outputs: none
void SMBEngine::InitializeArea()
{
    // clear all memory again, only as far as $074b; this is only necessary if branching from
    InitializeMemory(0x074b);

    uint8_t timerIndex = 0x21;

    do // ClrTimersLoop: clear out memory between
    {
        ram[Timers + timerIndex] = 0;
        --timerIndex; // $0780 and $07a1
    } while ((timerIndex & 0x80) == 0);

    // if AltEntranceControl not set, use halfway page, if any found;
    // otherwise use saved entry page number here
    const uint8_t startPage = altEntranceControl_ != 0 ? entrancePage_ : halfwayPage_;
    // StartPage: set as value here
    screenLeft_PageLoc_ = startPage;
    currentPageLoc_ = startPage;  // also set as current page
    backloadingFlag_ = startPage; // set flag here if halfway page or saved entry page number found

    // get pixel coordinates for screen borders
    const uint8_t oddPage = GetScreenPosition() & 0b00000001;
    // if on odd numbered page, use $2480 as start of rendering, otherwise use $2080;
    // this address is used later as the name table address for rendering of game area
    // SetInitNTHigh: store name table address
    currentNTAddr_High_ = oddPage != 0 ? 0x24 : 0x20;
    currentNTAddr_Low_ = 0x80;
    // store LSB of page number in high nybble of block buffer column position
    blockBufferColumnPos_ = oddPage << 4;
    --areaObjectLength_; // set area object lengths for all empty
    --M(AreaObjectLength + 1);
    --M(AreaObjectLength + 2);
    // set value for renderer to update 12 column sets
    columnSets_ = 11;   // 12 column sets = 24 metatile columns = 1 1/2 screens
    GetAreaDataAddrs(); // get enemy and level addresses and load header
    // check to see if primary hard mode has been activated
    bool setSecHard = true; // if so, activate the secondary no matter where we're at
    if (primaryHardMode_ == 0)
    {
        const uint8_t worldNumber = worldNumber_; // otherwise check world number
        if (worldNumber < World5) { setSecHard = false; }
        else if (worldNumber == World5)
        {
            // otherwise, world 5, so check level number
            setSecHard = (levelNumber_ >= Level3);
        } // if not equal to, then world > 5, thus activate
    }
    if (setSecHard)
    {
        // SetSecHard: set secondary hard mode flag for areas 5-3 and beyond
        ++secondaryHardMode_;
    }

    // CheckHalfway
    if (halfwayPage_ != 0)
    {
        // if halfway page set, overwrite start position from header
        playerEntranceCtrl_ = 2;
    } // DoneInitArea: silence music
    areaMusicQueue_ = Silence;
    disableScreenFlag_ = 1; // disable screen output
    ++operMode_Task_;       // increment one of the modes
}

// Inputs: none
// Outputs: none
void SMBEngine::SetupGameOver()
{
    // reset screen routine task control for title screen, game,
    screenRoutineTask_ = 0;           // and game over modes
    sprite0HitDetectFlag_ = 0;        // disable sprite 0 check
    eventMusicQueue_ = GameOverMusic; // put game over music in secondary queue
    ++disableScreenFlag_;             // disable screen output
    ++operMode_Task_;                 // set secondary mode to 1
}

// Inputs: none (loads AreaPointer into a itself before calling GetAreaType)
// Outputs: none
void SMBEngine::GetAreaDataAddrs()
{
    const uint8_t AreaDataAddrHigh_data[] = {
        HIBYTE(L_WaterArea1),       HIBYTE(L_WaterArea2),       HIBYTE(L_WaterArea3),       HIBYTE(L_GroundArea1),  HIBYTE(L_GroundArea2),
        HIBYTE(L_GroundArea3),      HIBYTE(L_GroundArea4),      HIBYTE(L_GroundArea5),      HIBYTE(L_GroundArea6),  HIBYTE(L_GroundArea7),
        HIBYTE(L_GroundArea8),      HIBYTE(L_GroundArea9),      HIBYTE(L_GroundArea10),     HIBYTE(L_GroundArea11), HIBYTE(L_GroundArea12),
        HIBYTE(L_GroundArea13),     HIBYTE(L_GroundArea14),     HIBYTE(L_GroundArea15),     HIBYTE(L_GroundArea16), HIBYTE(L_GroundArea17),
        HIBYTE(L_GroundArea18),     HIBYTE(L_GroundArea19),     HIBYTE(L_GroundArea20),     HIBYTE(L_GroundArea21), HIBYTE(L_GroundArea22),
        HIBYTE(L_UndergroundArea1), HIBYTE(L_UndergroundArea2), HIBYTE(L_UndergroundArea3), HIBYTE(L_CastleArea1),  HIBYTE(L_CastleArea2),
        HIBYTE(L_CastleArea3),      HIBYTE(L_CastleArea4),      HIBYTE(L_CastleArea5),      HIBYTE(L_CastleArea6)};

    const uint8_t AreaDataAddrLow_data[] = {
        LOBYTE(L_WaterArea1),       LOBYTE(L_WaterArea2),       LOBYTE(L_WaterArea3),       LOBYTE(L_GroundArea1),  LOBYTE(L_GroundArea2),
        LOBYTE(L_GroundArea3),      LOBYTE(L_GroundArea4),      LOBYTE(L_GroundArea5),      LOBYTE(L_GroundArea6),  LOBYTE(L_GroundArea7),
        LOBYTE(L_GroundArea8),      LOBYTE(L_GroundArea9),      LOBYTE(L_GroundArea10),     LOBYTE(L_GroundArea11), LOBYTE(L_GroundArea12),
        LOBYTE(L_GroundArea13),     LOBYTE(L_GroundArea14),     LOBYTE(L_GroundArea15),     LOBYTE(L_GroundArea16), LOBYTE(L_GroundArea17),
        LOBYTE(L_GroundArea18),     LOBYTE(L_GroundArea19),     LOBYTE(L_GroundArea20),     LOBYTE(L_GroundArea21), LOBYTE(L_GroundArea22),
        LOBYTE(L_UndergroundArea1), LOBYTE(L_UndergroundArea2), LOBYTE(L_UndergroundArea3), LOBYTE(L_CastleArea1),  LOBYTE(L_CastleArea2),
        LOBYTE(L_CastleArea3),      LOBYTE(L_CastleArea4),      LOBYTE(L_CastleArea5),      LOBYTE(L_CastleArea6)};

    const uint8_t AreaDataHOffsets_data[] = {0x00, 0x03, 0x19, 0x1c};

    const uint8_t EnemyDataAddrHigh_data[] = {
        HIBYTE(E_CastleArea1),      HIBYTE(E_CastleArea2),  HIBYTE(E_CastleArea3),  HIBYTE(E_CastleArea4),      HIBYTE(E_CastleArea5),
        HIBYTE(E_CastleArea6),      HIBYTE(E_GroundArea1),  HIBYTE(E_GroundArea2),  HIBYTE(E_GroundArea3),      HIBYTE(E_GroundArea4),
        HIBYTE(E_GroundArea5),      HIBYTE(E_GroundArea6),  HIBYTE(E_GroundArea7),  HIBYTE(E_GroundArea8),      HIBYTE(E_GroundArea9),
        HIBYTE(E_GroundArea10),     HIBYTE(E_GroundArea11), HIBYTE(E_GroundArea12), HIBYTE(E_GroundArea13),     HIBYTE(E_GroundArea14),
        HIBYTE(E_GroundArea15),     HIBYTE(E_GroundArea16), HIBYTE(E_GroundArea17), HIBYTE(E_GroundArea18),     HIBYTE(E_GroundArea19),
        HIBYTE(E_GroundArea20),     HIBYTE(E_GroundArea21), HIBYTE(E_GroundArea22), HIBYTE(E_UndergroundArea1), HIBYTE(E_UndergroundArea2),
        HIBYTE(E_UndergroundArea3), HIBYTE(E_WaterArea1),   HIBYTE(E_WaterArea2),   HIBYTE(E_WaterArea3)};

    const uint8_t EnemyDataAddrLow_data[] = {
        LOBYTE(E_CastleArea1),      LOBYTE(E_CastleArea2),  LOBYTE(E_CastleArea3),  LOBYTE(E_CastleArea4),      LOBYTE(E_CastleArea5),
        LOBYTE(E_CastleArea6),      LOBYTE(E_GroundArea1),  LOBYTE(E_GroundArea2),  LOBYTE(E_GroundArea3),      LOBYTE(E_GroundArea4),
        LOBYTE(E_GroundArea5),      LOBYTE(E_GroundArea6),  LOBYTE(E_GroundArea7),  LOBYTE(E_GroundArea8),      LOBYTE(E_GroundArea9),
        LOBYTE(E_GroundArea10),     LOBYTE(E_GroundArea11), LOBYTE(E_GroundArea12), LOBYTE(E_GroundArea13),     LOBYTE(E_GroundArea14),
        LOBYTE(E_GroundArea15),     LOBYTE(E_GroundArea16), LOBYTE(E_GroundArea17), LOBYTE(E_GroundArea18),     LOBYTE(E_GroundArea19),
        LOBYTE(E_GroundArea20),     LOBYTE(E_GroundArea21), LOBYTE(E_GroundArea22), LOBYTE(E_UndergroundArea1), LOBYTE(E_UndergroundArea2),
        LOBYTE(E_UndergroundArea3), LOBYTE(E_WaterArea1),   LOBYTE(E_WaterArea2),   LOBYTE(E_WaterArea3)};

    const uint8_t EnemyAddrHOffsets_data[] = {0x1f, 0x06, 0x1c, 0x00};

    GetAreaType(areaPointer_); // use 2 MSB of the area pointer

    // mask out all but 5 LSB, save as low offset
    uint8_t lOffset = areaPointer_ & 0b00011111;
    areaAddrsLOffset_ = lOffset;

    // load base value with 2 altered MSB, becomes offset for level data
    uint8_t enemyOffset = EnemyAddrHOffsets_data[areaType_] + lOffset;
    // use offset to load pointer
    enemyDataLow_ = EnemyDataAddrLow_data[enemyOffset];
    enemyDataHigh_ = EnemyDataAddrHigh_data[enemyOffset];

    // do the same thing but with different base value, using area type as offset
    uint8_t areaOffset = AreaDataHOffsets_data[areaType_] + lOffset;
    // use this offset to load another pointer
    areaDataLow_ = AreaDataAddrLow_data[areaOffset];
    areaDataHigh_ = AreaDataAddrHigh_data[areaOffset];

    uint8_t header0 = M(W(AreaData) + 0); // load first byte of header

    uint8_t foreScenery = header0 & 0b00000111; // save 3 LSB for foreground scenery or bg color control
    if (foreScenery >= 4)
    {
        backgroundColorCtrl_ = foreScenery; // if 4 or greater, save value here as bg color control
        foreScenery = 0;
    } // StoreFore: if less, save value here as foreground scenery
    foregroundScenery_ = foreScenery;

    // save player entrance control bits, shifted over to the LSBs
    playerEntranceCtrl_ = (header0 & 0b00111000) >> 3;

    // save 2 MSB for game timer setting, moved over to the LSBs
    gameTimerSetting_ = (header0 & 0b11000000) >> 6;

    uint8_t header1 = M(W(AreaData) + 1); // load second byte of header

    terrainControl_ = header1 & 0b00001111; // mask out all but lower nybble

    // save 2 MSB for background scenery type, shifted to the LSBs
    backgroundScenery_ = (header1 & 0b00110000) >> 4;

    uint8_t areaStyle = (header1 & 0b11000000) >> 6; // move the bits over to the LSBs
    if (areaStyle == 0b00000011)
    {                                   // and nullify other value
        cloudTypeOverride_ = areaStyle; // otherwise store value in other place
        areaStyle = 0;
    } // StoreStyle
    areaStyle_ = areaStyle;

    // increment area data address by 2 bytes
    uint32_t wide = ((areaDataHigh_ << 8) | areaDataLow_) + 2;
    areaDataLow_ = LOBYTE(wide);
    areaDataHigh_ = HIBYTE(wide);
}

// Inputs: objectOffset = object buffer offset forwarded to ImposeBlockGravity/ImposeGravitySprObj
// Outputs: none (residual: no callers in this port)
void SMBEngine::ResidualGravityCode(uint8_t objectOffset)
{
    // this part appears to be residual
    ImposeBlockGravity(0, objectOffset);
}

// Inputs: baseValue = base value (added to 13)
// Outputs: none (residual: no callers in this port; delegates to ResJmpM with x = baseValue+13,
// y = 0x1b)
void SMBEngine::ResidualMiscObjectCode(uint8_t baseValue)
{
    // miscellaneous objects; 0x1b is supposedly used once to set offset for block buffer data.
    // probably used in early stages to do misc to bg collision detection
    ResJmpM((uint8_t)(baseValue + 0x0d), 0x1b);
}

// Inputs: none
// Outputs: none
void SMBEngine::DisplayTimeUp()
{
    // if game timer not expired, increment task
    if (gameTimerExpiredFlag_ != 0)
    {                              // control 2 tasks forward, otherwise, stay here
        gameTimerExpiredFlag_ = 0; // reset timer expiration flag
        // a = 2; // output time-up screen to buffer
        OutputInter(TextNumber_TimeUp);
        return;
    } // NoTimeUp: increment control task 2 tasks forward
    ++screenRoutineTask_;
    ++screenRoutineTask_; // IncSubtask
}

// Inputs: a = text_number (passed as the C++ parameter, corresponds to register A in the original
// routine, forwarded to WriteGameText)
// Outputs: none
void SMBEngine::OutputInter(uint8_t text_number)
{
    WriteGameText(text_number);
    ResetScreenTimer();
    disableScreenFlag_ = 0; // reenable screen output
}

// Inputs: none
// Outputs: none
void SMBEngine::ResetSpritesAndScreenTimer()
{
    if (screenTimer_ != 0) // check if screen timer has expired
    {
        return;
    }

    MoveAllSpritesOffscreen(); // if so reset sprites now
    ResetScreenTimer();
}

// Inputs: none
// Outputs: none
void SMBEngine::ResetScreenTimer()
{
    screenTimer_ = 7;     // reset timer again
    ++screenRoutineTask_; // move onto next task
}

// Inputs: none
// Outputs: none
void SMBEngine::DisplayIntermediate()
{
    const uint8_t operMode = operMode_; // check primary mode of operation
    if (operMode != 0)                  // if in title screen mode, skip this
    {
        if (operMode == GameOverModeValue)
        { // GameOverInter: set screen timer
            screenTimer_ = 18;
            // a = 3; // output game over screen to buffer
            WriteGameText(TextNumber_GameOver);
            ++operMode_Task_; // inlined
            return;
        }
        //------------------------------------------------------------------------
        // otherwise check for mode of alternate entry
        if (altEntranceControl_ == 0) // and branch if found
        {
            // check if we are on castle level; on a castle level, or if the flag that skips
            // the intermediate lives display is clear, show the display
            if (areaType_ == 3 || disableIntermediate_ == 0)
            { // PlayerInter: put player in appropriate place for
                // DrawPlayer_Intermediate: IntermediatePlayerData_data staged
                // {0x58, 0x01, 0x00, 0x60, 0xff, 0x04} over $02-$07 to display the player as he
                // always appears on the world/lives display; they are constants, so they go
                // straight into the call. ($06 = 0xff was never read.) 0xb8 = offset for small
                // standing, 4 = sprite data offset, then flip control, sprite attributes,
                // horizontal and vertical position, and four rows of sprites.
                DrawPlayerLoop(0xb8, 0x04, 0x01, 0x00, 0x60, 0x58, 0x04);
                // get empty sprite attributes, set horizontal flip bit for bottom-right sprite
                ram[Sprite_Attributes + 32] = M(Sprite_Attributes + 36) | 0b01000000;
                // a = 1; // lives display, then output lives display to buffer
                OutputInter(TextNumber_WorldLivesDisplay);
                return;
            }
        }
    }

    // NoInter: set for specific task and leave
    screenRoutineTask_ = 8;
}

// Inputs: none
// Outputs: none
void SMBEngine::ClearBuffersDrawIcon()
{
    if (operMode_ != TitleScreenModeValue) // check game mode
    {                                      // if not title screen mode, leave
        ++operMode_Task_;
        return;
    }

    uint8_t bufferByte = 0; // otherwise, clear buffer space

    do // TScrClear
    {
        ram[VRAM_Buffer1 - 1 + bufferByte] = 0x00;
        ram[VRAM_Buffer1 - 1 + 0x100 + bufferByte] = 0x00;
        --bufferByte;
    } while (bufferByte != 0);
    DrawMushroomIcon(); // draw player select icon

    ++screenRoutineTask_; // IncSubtask
}

// Inputs: none
// Outputs: none
void SMBEngine::WriteTopScore()
{
    UpdateNumber(0xfa); // run display routine to display top score on title
    // move onto next mode
    ++operMode_Task_;
}

// Inputs: none
// Outputs: none
void SMBEngine::SetupVictoryMode()
{
    // get page location of right side of screen, increment to next page, and store here
    destinationPageLoc_ = screenRight_PageLoc_ + 1;
    eventMusicQueue_ = EndOfCastleMusic; // play win castle music
    ++operMode_Task_;                    // jump to set next major task in victory mode
}

// Inputs: none
// Outputs: none
void SMBEngine::PrimaryGameSetup()
{
    fetchNewGameTimerFlag_ = 1; // set flag to load game timer from header
    playerSize_ = 1;            // set player's size to small
    numberofLives_ = 2;         // give each player three lives
    offScr_NumberofLives_ = 2;
    SecondaryGameSetup();
}

// Inputs: none
// Outputs: none
void SMBEngine::SecondaryGameSetup()
{
    const uint8_t Sprite0Data_data[] = {0x18, 0xff, 0x23, 0x58};

    const uint8_t DefaultSprOffsets_data[] = {0x04, 0x30, 0x48, 0x60, 0x78, 0x90, 0xa8, 0xc0, 0xd8, 0xe8, 0x24, 0xf8, 0xfc, 0x28, 0x2c};

    disableScreenFlag_ = 0; // enable screen output

    uint8_t bufferByte = 0;

    do // ClearVRLoop: clear buffer at $0300-$03ff
    {
        ram[VRAM_Buffer1 - 1 + bufferByte] = 0x00;
        ++bufferByte;
    } while (bufferByte != 0);
    gameTimerExpiredFlag_ = 0;    // clear game timer exp flag
    disableIntermediate_ = 0;     // clear skip lives display flag
    backloadingFlag_ = 0;         // clear value here
    balPlatformAlignment_ = 0xff; // initialize balance platform assignment flag
    // put the LSB of the left side page location into d0 of the ppu register #1
    // mirror, to set the proper PPU name table
    mirrorPpuCtrlReg1_ = (mirrorPpuCtrlReg1_ & 0xfe) | (screenLeft_PageLoc_ & 0x01);
    GetAreaMusic(); // load proper music into queue
    // load sprite shuffle amounts to be used later
    ram[SprShuffleAmt + 2] = 0x38;
    ram[SprShuffleAmt + 1] = 0x48;
    sprShuffleAmt_ = 0x58;

    uint8_t sprOffsetIndex = 14; // load default OAM offsets into $06e4-$06f2

    do // ShufAmtLoop
    {
        ram[SprDataOffset + sprOffsetIndex] = DefaultSprOffsets_data[sprOffsetIndex];
        --sprOffsetIndex; // do this until they're all set
    } while ((sprOffsetIndex & 0x80) == 0);

    uint8_t sprite0Byte = 3; // set up sprite #0

    do // ISpr0Loop
    {
        ram[Sprite_Data + sprite0Byte] = Sprite0Data_data[sprite0Byte];
        --sprite0Byte;
    } while ((sprite0Byte & 0x80) == 0);
    DoNothing2();       // these don't do anything useful
    ram[0x06c9] = 0xff; // DoNothing1: residual, this value is not used anywhere
    DoNothing2();
    ++sprite0HitDetectFlag_; // set sprite #0 check flag
    ++operMode_Task_;        // increment to next task
}

// Inputs: none
// Outputs: none
void SMBEngine::DoNothing2() {}

// Inputs: none
// Outputs: none
void SMBEngine::InitScreen()
{
    MoveAllSpritesOffscreen(); // initialize all sprites including sprite #0
    InitializeNameTables();    // and erase both name and attribute tables
    if (operMode_ == TitleScreenModeValue)
    {                         // if mode still 0, do not load
        ++screenRoutineTask_; // IncSubtask
        return;
    }
    SetVRAMAddr_A(3); // into buffer pointer
}

// Inputs: none
// Outputs: none
void SMBEngine::GetAreaPalette()
{
    const uint8_t AreaPalette_data[] = {0x01, 0x02, 0x03, 0x04};

    // select appropriate palette to load based on area type
    SetVRAMAddr_A(AreaPalette_data[areaType_]);
}

// store offset into buffer control
// Inputs: addrCtrl = VRAM buffer address control offset to store
// Outputs: none
void SMBEngine::SetVRAMAddr_A(uint8_t addrCtrl)
{
    vRAM_Buffer_AddrCtrl_ = addrCtrl;
    ++screenRoutineTask_; // IncSubtask
}

// Inputs: none
// Outputs: none
void SMBEngine::GetAlternatePalette1()
{
    if (areaStyle_ == 1) // check for mushroom level style
    {
        vRAM_Buffer_AddrCtrl_ = 11; // if found, load appropriate palette
    } // NoAltPal: now onto the next task
    ++screenRoutineTask_; // IncSubtask
}

// Inputs: none
// Outputs: none
void SMBEngine::DrawTitleScreen()
{
    if (operMode_ != TitleScreenModeValue) // are we in title screen mode?
    {
        ++operMode_Task_; // inlined
        return;
    }
    // load address $1ec0 into the vram address register
    ppu->writeRegister(PPU_ADDRESS, HIBYTE(TitleScreenDataOffset));
    ppu->writeRegister(PPU_ADDRESS, LOBYTE(TitleScreenDataOffset));
    ppu->readRegister(PPU_DATA); // do one garbage read

    uint8_t pageHigh = 3; // the buffer starts at $0300
    uint8_t lowByte = 0;

    do // OutputTScr: get title screen from chr-rom
    {
        ram[(pageHigh << 8) + lowByte] = ppu->readRegister(PPU_DATA); // store 256 bytes into buffer
        ++lowByte;
        if (lowByte == 0)
        {               // if not past 256 bytes, do not increment
            ++pageHigh; // otherwise increment high byte of the buffer address
        } // ChkHiByte: check high byte?
    } while (pageHigh != 0x04 || lowByte < 0x3a);
    // set buffer transfer control to $0300,
    // inlined: goto SetVRAMAddr_B; // increment task and exit
    vRAM_Buffer_AddrCtrl_ = 5;
    ++screenRoutineTask_; // IncSubtask
}

// Inputs: bufferAddr = address of the VRAM buffer to transfer
// Outputs: none (delegates to InitScroll with a = 0, ending the update loop)
void SMBEngine::UpdateScreen(uint16_t bufferAddr)
{
    for (;;) // WriteBufferToScreen
    {
        ppu->readRegister(PPU_STATUS); // reset flip-flop

        // Read a packet from the buffer in the
        // https://www.nesdev.org/wiki/Tile_compression#NES_Stripe_Image_RLE format.
        const uint8_t high = M(bufferAddr + 0);
        if (high == 0) { break; }
        const uint8_t low = M(bufferAddr + 1);
        const uint8_t count = M(bufferAddr + 2);
        uint8_t dataIndex = 3;

        ppu->writeRegister(PPU_ADDRESS, high);
        ppu->writeRegister(PPU_ADDRESS, low);

        // Set PPU direction (bit 0b100 of PPUCTRL)
        const bool down = (count & 0x80) != 0;
        const uint8_t ctrl = down ? mirrorPpuCtrlReg1_ | 0b100 : mirrorPpuCtrlReg1_ & ~0b100;
        WritePPUReg1(ctrl);

        bool singleByte = (count & 0x40) != 0;
        if (!singleByte) { --dataIndex; }

        for (uint8_t j = 0; j < (count & 0x3f); j++)
        {
            if (!singleByte) { ++dataIndex; }
            ppu->writeRegister(PPU_DATA, M(bufferAddr + dataIndex));
        }

        // advance to the next packet
        bufferAddr = (uint16_t)(bufferAddr + dataIndex + 1);

        // sets vram address to $3f00
        ppu->writeRegister(PPU_ADDRESS, 0x3f);
        ppu->writeRegister(PPU_ADDRESS, 0x00);

        // then reinitializes it for some reason
        ppu->writeRegister(PPU_ADDRESS, 0x00);
        ppu->writeRegister(PPU_ADDRESS, 0x00);
    }

    InitScroll(0);
}

// Inputs: none
// Outputs: none
void SMBEngine::PrintVictoryMessages()
{
    // The world 1-7 counter check can set the end timer without touching the
    // message counters at all; every other path runs IncMsgCounter first.
    bool setEndTimerDirectly = false;

    // load secondary message counter
    if (secondaryMsgCounter_ == 0) // if set, branch to increment message counters
    {
        uint8_t msgCounter = primaryMsgCounter_; // otherwise load primary message counter
        // decide whether we print a message (ThankPlayer) or just keep counting
        bool thankPlayer = false;
        if (msgCounter == 0)
        {
            thankPlayer = true; // if set to zero, branch to print first message
        }
        else if (msgCounter < 9) // residual code: the counter never actually reaches 9
        {
            if (worldNumber_ == World8) // check world number
            {                           // if not at world 8, skip to next part
                if (msgCounter >= 3)    // if not at 3 yet (world 8 only), keep counting
                {
                    msgCounter -= 1; // otherwise subtract one
                    thankPlayer = true;
                }
            }
            else
            {                                    // MRetainerMsg: check primary message counter
                thankPlayer = (msgCounter >= 2); // if not at 2 yet (world 1-7 only), keep counting
            }
        }

        if (thankPlayer)
        {
            // ThankPlayer: put primary message counter into Y
            uint8_t msgIndex = msgCounter;
            bool evalForMusic = false;
            if (msgIndex == 0)
            { // if counter nonzero, skip this part, do not print first message
                // otherwise get player currently on the screen
                if (currentPlayer_ != 0)
                {
                    ++msgIndex; // increment Y once for luigi; mario leaves it at zero
                }
                evalForMusic = true;
            }
            else
            { // SecondPartMsg: increment Y to do world 8's message
                ++msgIndex;
                if (worldNumber_ == World8)
                {
                    evalForMusic = true; // if at world 8, branch to next part
                }
                else
                {
                    --msgIndex; // otherwise decrement Y for world 1-7's message
                    if (msgIndex >= 4)
                    {
                        setEndTimerDirectly = true; // branch to set victory end timer
                    }
                    else
                    {
                        // if counter is at 3, keep counting; otherwise print
                        evalForMusic = (msgIndex < 3);
                    }
                }
            }

            if (evalForMusic)
            {
                // EvalForMusic: if counter not yet at 3 (world 8 only), branch
                if (msgIndex == 3)
                { // to print message only (note world 1-7 will only
                    // reach this code if counter = 0, and will always branch)
                    eventMusicQueue_ = VictoryMusic; // otherwise load victory music first (world 8 only)
                } // PrintMsg: put primary message counter in A
                // ($0c-$0d = first), ($0e = world 1-7's), ($0f-$12 = world 8's)
                // write message counter to vram address controller
                vRAM_Buffer_AddrCtrl_ = msgIndex + 12;
            }
        }
    }

    if (!setEndTimerDirectly)
    {
        // IncMsgCounter: add four to secondary message counter
        const uint32_t wide = ((primaryMsgCounter_ << 8) | secondaryMsgCounter_) + 4;
        secondaryMsgCounter_ = LOBYTE(wide);
        primaryMsgCounter_ = HIBYTE(wide);

        // SetEndTimer: if not reached value yet, branch to leave
        if (HIBYTE(wide) < 7) { return; }
    }
    worldEndTimer_ = 6; // otherwise set world end timer
    ++operMode_Task_;

    // ExitMsgs: leave
}

// Inputs: none
// Outputs: none
void SMBEngine::PlayerEndWorld()
{
    if (worldEndTimer_ != 0) // check to see if world end timer expired
    {
        return; // branch to leave if not
    }
    if (worldNumber_ < World8)     // check world number
    {                              // thus branch to read controller
        areaNumber_ = 0;           // otherwise initialize area number used as offset
        levelNumber_ = 0;          // and level number control to start at area 1
        operMode_Task_ = 0;        // initialize secondary mode of operation
        ++worldNumber_;            // increment world number to move onto the next world
        LoadAreaPointer();         // get area address offset for the next area
        ++fetchNewGameTimerFlag_;  // set flag to load game timer from header
        operMode_ = GameModeValue; // set mode of operation to game mode

        return; // EndExitOne: and leave

        //------------------------------------------------------------------------
    } // EndChkBButton
    // check to see if B button was pressed on either controller
    if (((savedJoypadBits_[0] | savedJoypadBits_[1]) & B_Button) != 0)
    { // branch to leave if not
        // otherwise set world selection flag
        worldSelectEnableFlag_ = 1;
        numberofLives_ = 0xff; // remove onscreen player's lives
        TerminateGame();       // do sub to continue other player or end game
    } // EndExitTwo: leave
}

// Inputs: none
// Outputs: none
void SMBEngine::RunGameOver()
{
    // reenable screen
    disableScreenFlag_ = 0;
    // check controller for start pressed
    if ((savedJoypadBits_[0] & Start_Button) != 0)
    {
        TerminateGame();
        return;
    }
    if (screenTimer_ != 0) // if not pressed, wait for
    {                      // screen timer to expire
        return;
    }
    TerminateGame();
}

// Inputs: none
// Outputs: none
void SMBEngine::TerminateGame()
{
    eventMusicQueue_ = Silence;              // silence music
    const bool endGame = TransposePlayers(); // check if other player can keep
    if (!endGame)
    {
        ContinueGame(); // going, and do so if possible
        return;
    }
    // otherwise put world number of current
    continueWorld_ = worldNumber_; // player into secret continue function variable
    operMode_Task_ = 0;            // reset all modes to title screen and
    screenTimer_ = 0;              // leave
    operMode_ = TitleScreenModeValue;
}

// Inputs: none
// Outputs: none
void SMBEngine::PlayerVictoryWalk()
{
    victoryWalkControl_ = 0; // set value here to not walk player by default
    // walk unless the player has reached the destination page and is far
    // enough across it
    const bool walk = player_PageLoc_ != destinationPageLoc_ || player_X_Position_ < 0x60;
    if (walk)
    { // PerformWalk: otherwise increment value and Y
        ++victoryWalkControl_;
    }

    // DontWalk: move player to the right or not
    AutoControlPlayer(walk ? 1 : 0);
    // check page location of left side of screen
    if (screenLeft_PageLoc_ != destinationPageLoc_)
    {                                                   // branch if equal to change modes if necessary
        const uint32_t wide = scrollFractional_ + 0x80; // do fixed point math on fractional part of scroll
        scrollFractional_ = LOBYTE(wide);               // save fractional movement amount
        // one pixel per frame, plus the carry out of the fraction, used as scroll amount
        ScrollScreen((uint8_t)(1 + HIBYTE(wide))); // do sub to scroll the screen
        UpdScrollVar();                            // do another sub to update screen and scroll variables
        ++victoryWalkControl_;                     // increment value to stay in this routine
    } // ExitVWalk: load value set here
    if (victoryWalkControl_ == 0)
    {
        ++operMode_Task_; // if zero, branch to change modes
    }
    // otherwise leave
}

// Inputs: none
// Outputs: none
void SMBEngine::ScreenRoutines()
{
    switch (screenRoutineTask_) // run one of the following subroutines
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

// Inputs: none
// Outputs: none
void SMBEngine::AreaParserTaskControl()
{
    ++disableScreenFlag_; // turn off screen

    do // TaskLoop: render column set of current area
    {
        AreaParserTaskHandler();
        // check number of tasks
    } while (areaParserTaskNum_ != 0); // if tasks still not all done, do another one
    --columnSets_; // do we need to render more column sets?
    if ((columnSets_ & 0x80) != 0)
    {
        ++screenRoutineTask_; // if not, move on to the next task
    } // OutputCol: set vram buffer to output rendered column set
    vRAM_Buffer_AddrCtrl_ = 6; // on next NMI
}

// Inputs: none
// Outputs: none
void SMBEngine::GameOverMode()
{
    switch (operMode_Task_)
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

// reset game modes, disable
// Inputs: none
// Outputs: none
void SMBEngine::ResetTitle()
{
    operMode_ = TitleScreenModeValue; // sprite 0 check and disable
    operMode_Task_ = 0;               // screen output
    sprite0HitDetectFlag_ = 0;
    ++disableScreenFlag_;
}

// if timer for demo has expired, reset modes
// Inputs: joypadBits = joypad bits (SavedJoypad1Bits | SavedJoypad2Bits from the caller; tested for
// the A button bit before loading the secret continue world number)
// Outputs: none
void SMBEngine::ChkContinue(uint8_t joypadBits)
{
    if (demoTimer_ == 0)
    {
        ResetTitle();
        return;
    }
    if ((joypadBits & 0x80) != 0) // check to see if A button was also pushed
    {                             // if not, don't load continue function's world number
        // load previously saved world number for secret continue when pressing A + start
        GoContinue(continueWorld_);
    } // StartWorld1
    LoadAreaPointer();
    ++hidden1UpFlag_; // set 1-up box flag for both players
    ++offScr_Hidden1UpFlag_;
    ++fetchNewGameTimerFlag_; // set fetch new game timer flag
    ++operMode_;              // set next game mode
    // if world select flag is on, then primary
    primaryHardMode_ = worldSelectEnableFlag_; // hard mode must be on as well
    operMode_Task_ = 0;                        // set game mode here, and clear demo timer
    demoTimer_ = 0;

    uint8_t displayByte = 0x17;

    do // InitScores: clear player scores and coin displays
    {
        ram[ScoreAndCoinDisplay + displayByte] = 0;
        --displayByte;
    } while ((displayByte & 0x80) == 0);
}

// Inputs: none
// Outputs: none
void SMBEngine::VictoryModeSubroutines()
{
    switch (operMode_Task_)
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

// Inputs: none
// Outputs: none
void SMBEngine::BridgeCollapse()
{
    const uint8_t bowserOffset = bowserFront_Offset_; // get enemy offset for bowser
    // check enemy object identifier for bowser
    bool removeBridge = false;
    if (M(Enemy_ID + bowserOffset) == Bowser) // otherwise metatile removal not necessary
    {
        objectOffset_ = bowserOffset;                              // store as enemy offset here
        const uint8_t bowserState = M(Enemy_State + bowserOffset); // if bowser in normal state, skip all of this
        if (bowserState == 0) { removeBridge = true; }
        else
        {
            // if bowser's state has d6 clear, skip to silence music
            if ((bowserState & 0b01000000) != 0)
            {
                // check bowser's vertical coordinate
                if (M(Enemy_Y_Position + bowserOffset) < 0xe0)
                {
                    MoveD_Bowser(bowserOffset);
                    return;
                }
            }
        }
    }

    if (!removeBridge)
    {
        // SetM2: silence music
        eventMusicQueue_ = Silence;
        ++operMode_Task_; // move onto next secondary mode in autoctrl mode
        KillAllEnemies(); // jump to empty all enemy slots and then leave
        return;
    }

    // RemoveBridge
    --bowserFeetCounter_;        // decrement timer to control bowser's feet
    if (bowserFeetCounter_ == 0) // if not expired, skip all of this
    {
        bowserFeetCounter_ = 4; // otherwise, set timer now
        // invert bit to control bowser's feet
        bowserBodyControls_ = bowserBodyControls_ ^ 0x01;
        // get bridge collapse offset here and use as low byte of name table address;
        // the high byte is always $22
        const uint8_t nameTableLow = M(BridgeCollapseData + bridgeCollapseOffset_);

        const uint8_t vramOffset = vRAM_Buffer1_Offset_ + 1; // increment vram buffer offset
        // 12 = offset for tile data for sub to draw blank metatile
        RemBridge(0x0c, vramOffset, nameTableLow, 0x22); // do sub here to remove bowser's bridge metatiles
        MoveVOffset(vramOffset);                         // set new vram buffer offset
        // load the fireworks/gunfire sound into the square 2 sfx
        square2SoundQueue_ = Sfx_Blast; // queue while at the same time loading the brick
        // shatter sound into the noise sfx queue thus
        noiseSoundQueue_ = Sfx_BrickShatter; // producing the unique sound of the bridge collapsing
        ++bridgeCollapseOffset_;             // increment bridge collapse offset
        if (bridgeCollapseOffset_ == 15)     // the end, go ahead and skip this part
        {
            InitVStf(bowserOffset);                       // initialize whatever vertical speed bowser has
            ram[Enemy_State + bowserOffset] = 0b01000000; // set bowser's state to one of defeated states (d6 set)
            square2SoundQueue_ = Sfx_BowserFall;          // play bowser defeat sound
        }
    }

    // NoBFall: jump to code that draws bowser
    BowserGfxHandler(bowserOffset);
}

// this is the heart of the entire program,
// Inputs: none
// Outputs: none
void SMBEngine::OperModeExecutionTree()
{
    switch (operMode_)
    {
    case TitleScreenModeValue:
        TitleScreenMode();
        return;
    case GameModeValue:
        GameMode();
        return;
    case VictoryModeValue:
        VictoryMode();
        return;
    case GameOverModeValue:
        GameOverMode();
        return;
    default:
        bad_jump();
        return;
    } // most of what goes on starts here
}

// Inputs: none
// Outputs: none
void SMBEngine::TitleScreenMode()
{
    switch (operMode_Task_)
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

// Inputs: none
// Outputs: none
void SMBEngine::GameMenuRoutine()
{
    // check to see if either player pressed
    const uint8_t joypadBits = savedJoypadBits_[0] | savedJoypadBits_[1]; // only the start button (either joypad)
    if (joypadBits == Start_Button || joypadBits == A_Button + Start_Button)
    { // StartGame: if either start or A + start, execute here
        ChkContinue(joypadBits);
        return;
    }

    // B advances the world selection where select changes the number of players
    bool worldSelectPressed = false;

    // ChkSelect: check to see if the select button was pressed
    if (joypadBits != Select_Button)
    { // if so, branch reset demo timer
        // otherwise check demo timer
        if (demoTimer_ == 0)
        {                              // if demo timer not expired, branch to check world selection
            selectTimer_ = joypadBits; // set controller bits here if running demo
            if (DemoEngine())          // run through the demo actions
            {
                ResetTitle(); // demo over, thus branch
                return;
            }
            RunDemo(); // otherwise, run game engine for demo
            return;
        } // ChkWorldSel: check to see if world selection has been enabled
        if (worldSelectEnableFlag_ == 0)
        {
            NullJoypad();
            return;
        }
        if (joypadBits != B_Button)
        {
            NullJoypad();
            return;
        }
        worldSelectPressed = true; // if so, increment Y and execute same code as select
    } // SelectBLogic: if select or B pressed, check demo timer one last time
    if (demoTimer_ == 0)
    {
        ResetTitle(); // if demo timer expired, branch to reset title screen mode
        return;
    }
    // otherwise reset demo timer
    demoTimer_ = 24;
    // check select/B button timer
    if (selectTimer_ != 0)
    {
        NullJoypad(); // if not expired, branch
        return;
    }
    selectTimer_ = 16; // otherwise reset select button timer
    if (!worldSelectPressed)
    { // note this will not be run if world selection is disabled
        // if no, must have been the select button, therefore
        // change number of players and draw icon accordingly
        numberOfPlayers_ = numberOfPlayers_ ^ 0b00000001;
        DrawMushroomIcon();
        NullJoypad();
        return;
    } // IncWorldSel: increment world select number
    // mask out higher bits and store as current world select number
    const uint8_t worldSelectNumber = (worldSelectNumber_ + 1) & 0b00000111;
    worldSelectNumber_ = worldSelectNumber;
    GoContinue(worldSelectNumber);

    uint8_t templateByte = 0;

    do // UpdateShroom: write template for world select in vram buffer
    {
        // do this until all bytes are written
        ram[VRAM_Buffer1 - 1 + templateByte] = M(WSelectBufferTemplate + templateByte);
        ++templateByte;
    } while (((templateByte - 6) & 0x80) != 0);
    // get world number from variable and increment for proper display,
    // and put in blank byte before null terminator
    ram[VRAM_Buffer1 + 3] = worldNumber_ + 1;
    NullJoypad();
}

// clear joypad bits for player 1
// Inputs: none
// Outputs: none
void SMBEngine::NullJoypad()
{
    savedJoypadBits_[0] = 0;
    RunDemo();
}

// run game engine
// Inputs: none
// Outputs: none
void SMBEngine::RunDemo()
{
    GameCoreRoutine();
    // check to see if we're running lose life routine
    if (gameEngineSubroutine_ == Gs_PlayerLoseLife) { ResetTitle(); }
}

// Inputs: none
// Outputs: none
void SMBEngine::VictoryMode()
{
    VictoryModeSubroutines(); // run victory mode subroutines
    // get current task of victory mode
    if (operMode_Task_ != 0)
    {                           // if on bridge collapse, skip enemy processing
        objectOffset_ = 0;      // otherwise reset enemy object offset
        EnemiesAndLoopsCore(0); // and run enemy code
    } // AutoPlayer: get player's relative coordinates
    RelativePlayerPosition();
    PlayerGfxHandler(); // draw the player, then leave
}

// Inputs: none
// Outputs: none
void SMBEngine::GameMode()
{
    switch (operMode_Task_)
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
