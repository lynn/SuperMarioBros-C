// This file was generated from docs/smbdis.asm by the tool in codegen, and has
// since been corrected by hand where the translation was not faithful to the
// console. Regenerating it discards those corrections: see the note in
// codegen/CMakeLists.txt before you do.
//
#include "SMB.hpp"

void SMBEngine::code(int mode)
{
    // The bit an LSR/ASL shifts out, captured before the shift destroys it. Only
    // ever set immediately above the branch that reads it.
    bool shiftedBit = false;

    // Scratch for multi-byte (page:position:fraction) arithmetic.
    uint32_t wide = 0;

    // Borrow out of a multi-byte subtraction.
    bool borrow = false;

    // A carry the original threads from one piece of arithmetic into another,
    // having never cleared it in between. Each use names where it comes from.
    bool carry = false;

    // Set by FindEmptyMiscSlot when it gets as far as comparing the offset. False
    // only when the last slot was free straight away, in which case the original
    // leaves the carry it was called with -- clear. CoinBlock reads it.
    bool miscSlotSearched = false;

    // The carry the bloober's swim code happens to be holding when ChkNearPlayer
    // adds sixteen pixels to the bloober's vertical coordinate.
    bool blooberCarry = false;

    // Results these subroutines used to hand back in the carry flag.
    bool allEnemySlotsFull = false;
    bool bumpedBlockFound = false;
    bool climbMTileFound = false;
    bool coinMTileFound = false;
    bool collisionFound = false;
    bool demoOver = false;
    bool enemyRightOfPlayer = false;
    bool endGame = false;
    bool enemyYPosInRange = false;
    bool hammerSpawned = false;
    bool jumpspringFound = false;
    bool lrgObjJustStarted = false;
    bool playerVerticalOutOfRange = false;
    bool sidePipeShaftDrawn = false;
    bool solidMTileFound = false;

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
    writeData(0x00, M(VRAM_AddrTable_Low + x));
    writeData(0x01, M(VRAM_AddrTable_High + x));
    UpdateScreen(); // update screen with buffer contents
    y = 0x00;
    // check for usage of $0341
    if (M(VRAM_Buffer_AddrCtrl) == 0x06)
    {
        y = 0x01; // get offset based on usage
    } // InitBuffer
    x = M(VRAM_Buffer_Offset + y);
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
    y = 0x07; // read eight bytes to be read by transfer routine

    do // IconDataRead: note that the default position is set for a
    {
        writeData(VRAM_Buffer1 - 1 + y, M(MushroomIconData + y)); // 1-player game
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
    bool demoOver = false;

    x = M(DemoAction); // load current demo action
    // load current action timer
    if (M(DemoActionTimer) == 0)
    { // if timer still counting down, skip
        ++x;
        ++M(DemoAction); // if expired, increment action, X, and
        demoOver = true; // demo over by default
        a = M(DemoTimingData - 1 + x); // get next timer
        writeData(DemoActionTimer, a); // store as current timer
        if (a == 0)
            return demoOver; // if timer already at zero, skip
    } // DoAction: get and perform action (current or next)
    a = M(DemoActionData - 1 + x);
    writeData(SavedJoypad1Bits, a);
    --M(DemoActionTimer); // decrement action timer
    demoOver = false; // demo still going

    return demoOver; // DemoOver
}

//------------------------------------------------------------------------

void SMBEngine::ColorRotation()
{
    // get frame counter
    a = M(FrameCounter) & 0x07; // mask out all but three LSB
    if (a != 0)
        return; // branch if not set to zero to do this every eighth frame
    x = M(VRAM_Buffer1_Offset); // check vram buffer offset
    if (x >= 0x31)
        return; // if offset over 48 bytes, branch to leave
    y = a; // otherwise use frame counter's 3 LSB as offset here

    do // GetBlankPal: get blank palette for palette 3
    {
        writeData(VRAM_Buffer1 + x, M(BlankPalette + y)); // store it in the vram buffer
        ++x; // increment offsets
        ++y;
    } while (y < 0x08); // do this until all bytes are copied
    x = M(VRAM_Buffer1_Offset); // get current vram buffer offset
    writeData(0x00, 0x03); // set counter here
    a = M(AreaType); // get area type
    a <<= 1; // multiply by 4 to get proper offset
    a <<= 1;
    y = a; // save as offset here

    do // GetAreaPal: fetch palette to be written based on area type
    {
        writeData(VRAM_Buffer1 + 3 + x, M(Palette3Data + y)); // store it to overwrite blank palette in vram buffer
        ++y;
        ++x;
        --M(0x00); // decrement counter
    } while ((M(0x00) & 0x80) == 0); // do this until the palette is all copied
    x = M(VRAM_Buffer1_Offset); // get current vram buffer offset
    y = M(ColorRotateOffset); // get color cycling offset
    writeData(VRAM_Buffer1 + 4 + x, M(ColorRotatePalette + y)); // get and store current color in second slot of palette
    a = M(VRAM_Buffer1_Offset);
    a += 0x07;
    writeData(VRAM_Buffer1_Offset, a);
    ++M(ColorRotateOffset); // increment color cycling offset
    a = M(ColorRotateOffset);
    if (a < 0x06)
        return; // if so, branch to leave
    a = 0x00;
    writeData(ColorRotateOffset, 0x00); // otherwise, init to keep it in range

    return; // ExitColorRot: leave
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

void SMBEngine::GetAreaMusic()
{
    a = M(OperMode); // if in title screen mode, leave
    if (a != 0)
    {
        // check for specific alternate mode of entry
        if (M(AltEntranceControl) != 0x02)
        { // from area object data header
            y = 0x05; // select music for pipe intro scene by default
            a = M(PlayerEntranceCtrl); // check value from level header for certain values
            if (a == 0x06)
                goto StoreMusic; // load music for pipe intro scene if header
            if (a == 0x07)
                goto StoreMusic;
        } // ChkAreaType: load area type as offset for music bit
        y = M(AreaType);
        if (M(CloudTypeOverride) == 0)
            goto StoreMusic; // check for cloud type override
        y = 0x04; // select music for cloud type level if found

StoreMusic: // otherwise select appropriate music for level type
        a = M(MusicSelectData + y);
        writeData(AreaMusicQueue, a); // store in queue and leave
    } // ExitGetM
    return;
}

//------------------------------------------------------------------------

bool SMBEngine::TransposePlayers()
{
    bool endGame = false;

    endGame = true; // end the game by default
    a = M(NumberOfPlayers); // if only a 1 player game, leave
    if (a == 0)
        return endGame;
    a = M(OffScr_NumberofLives); // does offscreen player have any lives left?
    if ((a & 0x80) != 0)
        return endGame; // branch if not
    // invert bit to update
    a = M(CurrentPlayer) ^ 0b00000001; // which player is on the screen
    writeData(CurrentPlayer, a);
    x = 0x06;

    do // TransLoop: transpose the information
    {
        a = M(OnscreenPlayerInfo + x);
        pha(); // of the onscreen player
        // with that of the offscreen player
        writeData(OnscreenPlayerInfo + x, M(OffscreenPlayerInfo + x));
        pla();
        writeData(OffscreenPlayerInfo + x, a);
        --x;
    } while ((x & 0x80) == 0);
    endGame = false; // get the game going

    return endGame; // ExTrans
}

//------------------------------------------------------------------------

    //------------------------------------------------------------------------

void SMBEngine::IncAreaObjOffset()
{
        ++M(AreaDataOffset); // increment offset of level pointer
        ++M(AreaDataOffset);
        a = 0x00; // reset page select
        writeData(AreaObjectPageSel, 0x00);
        return;
}


//------------------------------------------------------------------------

bool SMBEngine::FindEmptyEnemySlot()
{
    bool allEnemySlotsFull = false;

    x = 0x00; // start at first enemy slot

EmptyChkLoop: // assume a slot is free by default
    allEnemySlotsFull = false;
    a = M(Enemy_Flag + x); // check enemy buffer for nonzero
    if (a != 0)
    { // if zero, leave
        ++x;
        if (x != 0x05)
            goto EmptyChkLoop;
        allEnemySlotsFull = true; // ExitEmptyChk: all values nonzero
    }
    return allEnemySlotsFull;
}

//------------------------------------------------------------------------

void SMBEngine::GetLrgObjAttrib()
{
    y = M(AreaObjOffsetBuffer + x); // get offset saved from area obj decoding routine
    // get first byte of level object
    a = M(W(AreaData) + y) & 0b00001111;
    writeData(0x07, a); // save row location
    ++y;
    // get next byte, save lower nybble (length or height)
    a = M(W(AreaData) + y) & 0b00001111; // as Y, then leave
    y = a;
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetAreaObjXPosition()
{
    a = M(CurrentColumnPos); // multiply current offset where we're at by 16
    a <<= 1; // to obtain horizontal pixel coordinate
    a <<= 1;
    a <<= 1;
    a <<= 1;
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetAreaObjYPosition()
{
    a = M(0x07); // multiply value by 16
    a <<= 1;
    a <<= 1; // this will give us the proper vertical pixel coordinate
    a <<= 1;
    a <<= 1;
    a += 32; // add 32 pixels for the status bar
    return;
}


//------------------------------------------------------------------------

void SMBEngine::FindAreaPointer()
{
    y = M(WorldNumber); // load offset from world variable
    a = M(WorldAddrOffsets + y);
    a += M(AreaNumber);
    y = a;
    a = M(AreaAddrOffsets + y); // from there we have our area pointer
    return;
}


//------------------------------------------------------------------------

void SMBEngine::MovePlayerYAxis()
{
    a += M(Player_Y_Position); // add contents of A to player position
    writeData(Player_Y_Position, a);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PlayerPhysicsSub()
{
    uint32_t wide = 0;

    // check player state
    if (M(Player_State) == 0x03)
    { // if not climbing, branch
        y = 0x00;
        // get controller bits for up/down
        a = M(Up_Down_Buttons) & M(Player_CollisionBits); // check against player's collision detection bits
        if (a == 0)
            goto ProcClimb; // if not pressing up or down, branch
        y = 0x01;
        a &= 0b00001000; // check for pressing up
        if (a != 0)
            goto ProcClimb;
        y = 0x02;

ProcClimb: // load value here
        writeData(Player_Y_MoveForce, M(Climb_Y_MForceData + y)); // store as vertical movement force
        a = 0x08; // load default animation timing
        x = M(Climb_Y_SpeedData + y); // load some other value here
        writeData(Player_Y_Speed, x); // store as vertical speed
        if ((x & 0x80) == 0)
        { // if climbing down, use default animation timing value
            a = 0x04; // otherwise divide timer setting by 2
        } // SetCAnim: store animation timer setting and leave
        writeData(PlayerAnimTimerSet, a);
        return;

    //------------------------------------------------------------------------
    } // CheckForJumping
    // if jumpspring animating,
    if (M(JumpspringAnimCtrl) != 0)
        goto NoJump; // skip ahead to something else
    // check for A button press
    a = M(A_B_Buttons) & A_Button;
    if (a == 0)
        goto NoJump; // if not, branch to something else
    a &= M(PreviousA_B_Buttons); // if button not pressed in previous frame, branch
    if (a != 0)
    {

NoJump: // otherwise, jump to something else
        goto X_Physics;
    } // ProcJumping
    // check player state
    if (M(Player_State) == 0)
        goto InitJS; // if on the ground, branch
    // if swimming flag not set, jump to do something else
    if (M(SwimmingFlag) == 0)
        goto NoJump; // to prevent midair jumping, otherwise continue
    // if jump/swim timer nonzero, branch
    if (M(JumpSwimTimer) != 0)
        goto InitJS;
    // check player's vertical speed
    if ((M(Player_Y_Speed) & 0x80) == 0)
        goto InitJS; // if player's vertical speed motionless or down, branch
    goto X_Physics; // if timer at zero and player still rising, do not swim

InitJS: // set jump/swim timer
    writeData(JumpSwimTimer, 0x20);
    y = 0x00; // initialize vertical force and dummy variable
    writeData(Player_YMF_Dummy, 0x00);
    writeData(Player_Y_MoveForce, 0x00);
    // get vertical high and low bytes of jump origin
    writeData(JumpOrigin_Y_HighPos, M(Player_Y_HighPos)); // and store them next to each other here
    writeData(JumpOrigin_Y_Position, M(Player_Y_Position));
    // set player state to jumping/swimming
    writeData(Player_State, 0x01);
    a = M(Player_XSpeedAbsolute); // check value related to walking/running speed
    if (a < 0x09)
        goto ChkWtr; // branch if below certain values, increment Y
    y = 0x01; // for each amount equal or exceeded
    if (a < 0x10)
        goto ChkWtr;
    y = 0x02;
    if (a < 0x19)
        goto ChkWtr;
    y = 0x03;
    if (a < 0x1c)
        goto ChkWtr; // note that for jumping, range is 0-4 for Y
    y = 0x04;

ChkWtr: // set value here (apparently always set to 1)
    writeData(DiffToHaltJump, 0x01);
    // if swimming flag disabled, branch
    if (M(SwimmingFlag) == 0)
        goto GetYPhy;
    y = 0x05; // otherwise set Y to 5, range is 5-6
    // if whirlpool flag not set, branch
    if (M(Whirlpool_Flag) == 0)
        goto GetYPhy;
    y = 0x06; // otherwise increment to 6

GetYPhy: // store appropriate jump/swim
    writeData(VerticalForce, M(JumpMForceData + y)); // data here
    writeData(VerticalForceDown, M(FallMForceData + y));
    writeData(Player_Y_MoveForce, M(InitMForceData + y));
    writeData(Player_Y_Speed, M(PlayerYSpdData + y));
    // if swimming flag disabled, branch
    if (M(SwimmingFlag) != 0)
    {
        // load swim/goomba stomp sound into
        writeData(Square1SoundQueue, Sfx_EnemyStomp); // square 1's sfx queue
        if (M(Player_Y_Position) >= 0x14)
            goto X_Physics; // if below a certain point, branch
        a = 0x00; // otherwise reset player's vertical speed
        writeData(Player_Y_Speed, 0x00); // and jump to something else to keep player
        goto X_Physics; // from swimming above water level
    } // PJumpSnd: load big mario's jump sound by default
    a = Sfx_BigJump;
    // is mario big?
    if (M(PlayerSize) != 0)
    {
        a = Sfx_SmallJump; // if not, load small mario's jump sound
    } // SJumpSnd: store appropriate jump sound in square 1 sfx queue
    writeData(Square1SoundQueue, a);

X_Physics:
    y = 0x00;
    writeData(0x00, 0x00); // init value here
    // if mario is on the ground, branch
    if (M(Player_State) != 0)
    {
        a = M(Player_XSpeedAbsolute); // check something that seems to be related
        if (a >= 0x19)
            goto GetXPhy; // if =>$19 branch here
        if (a < 0x19)
            goto ChkRFast; // if not branch elsewhere
    } // ProcPRun: if mario on the ground, increment Y
    y = 0x01;
    // check area type
    if (M(AreaType) == 0)
        goto ChkRFast; // if water type, branch
    y = 0x00; // decrement Y by default for non-water type area
    // get left/right controller bits
    if (M(Left_Right_Buttons) != M(Player_MovingDir))
        goto ChkRFast; // if controller bits <> moving direction, skip this part
    // check for b button pressed
    a = M(A_B_Buttons) & B_Button;
    if (a == 0)
    { // if pressed, skip ahead to set timer
        // check for running timer set
        if (M(RunningTimer) != 0)
            goto GetXPhy; // if set, branch

ChkRFast: // if running timer not set or level type is water,
        ++y;
        ++M(0x00); // increment Y again and temp variable in memory
        if (M(RunningSpeed) == 0)
        { // if running speed set here, branch
            if (M(Player_XSpeedAbsolute) < 0x21)
                goto GetXPhy; // if less than a certain amount, branch ahead
        } // FastXSp: if running speed set or speed => $21 increment $00
        ++M(0x00);
        goto GetXPhy; // and jump ahead
    } // SetRTmr: if b button pressed, set running timer
    a = 0x0a;
    writeData(RunningTimer, 0x0a);

GetXPhy: // get maximum speed to the left
    writeData(MaximumLeftSpeed, M(MaxLeftXSpdData + y));
    // check for specific routine running
    if (M(GameEngineSubroutine) == 0x07)
    { // if not running, skip and use old value of Y
        y = 0x03; // otherwise set Y to 3
    } // GetXPhy2: get maximum speed to the right
    writeData(MaximumRightSpeed, M(MaxRightXSpdData + y));
    y = M(0x00); // get other value in memory
    // get value using value in memory as offset
    writeData(FrictionAdderLow, M(FrictionData + y));
    writeData(FrictionAdderHigh, 0x00); // init something here
    a = M(PlayerFacingDir);
    if (a != M(Player_MovingDir))
    { // if the same, branch to leave
        wide = ((M(FrictionAdderHigh) << 8) | M(FrictionAdderLow)) << 1; // otherwise double the 16-bit friction adder
        writeData(FrictionAdderLow, LOBYTE(wide));
        writeData(FrictionAdderHigh, HIBYTE(wide));
    } // ExitPhy: and then leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetPlayerAnimSpeed()
{
    y = 0x00; // initialize offset in Y
    a = M(Player_XSpeedAbsolute); // check player's walking/running speed
    if (a < 0x1c)
    { // if greater than a certain amount, branch ahead
        y = 0x01; // otherwise increment Y
        if (a < 0x0e)
        { // if greater than this but not greater than first, skip increment
            y = 0x02; // otherwise increment Y again
        } // ChkSkid: get controller bits
        a = M(SavedJoypadBits) & 0b01111111; // mask out A button
        if (a == 0)
            goto SetAnimSpd; // if no other buttons pressed, branch ahead of all this
        a &= 0x03; // mask out all others except left and right
        if (a != M(Player_MovingDir))
            goto ProcSkid; // if left/right controller bits <> moving direction, branch
        a = 0x00; // otherwise set zero value here
    } // SetRunSpd: store zero or running speed here
    writeData(RunningSpeed, a);
    goto SetAnimSpd;

ProcSkid: // check player's walking/running speed
    if (M(Player_XSpeedAbsolute) >= 0x0b)
        goto SetAnimSpd; // if greater than this amount, branch
    writeData(Player_MovingDir, M(PlayerFacingDir)); // otherwise use facing direction to set moving direction
    a = 0x00;
    writeData(Player_X_Speed, 0x00); // nullify player's horizontal speed
    writeData(Player_X_MoveForce, 0x00); // and dummy variable for player

SetAnimSpd: // get animation timer setting using Y as offset
    a = M(PlayerAnimTmrData + y);
    writeData(PlayerAnimTimerSet, a);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ImposeFriction()
{
    bool shiftedBit = false;
    uint32_t wide = 0;

    a &= M(Player_CollisionBits); // perform AND between left/right controller bits and collision flag
    if (a == 0x00)
    { // if any bits set, branch to next part
        a = M(Player_X_Speed);
        if (a == 0)
            goto SetAbsSpd; // if player has no horizontal speed, branch ahead to last part
        if ((a & 0x80) == 0)
            goto RghtFrict; // if player moving to the right, branch to slow
        if ((a & 0x80) != 0)
            goto LeftFrict; // otherwise logic dictates player moving left, branch to slow
    } // JoypFrict: take the right controller bit
    shiftedBit = (a & 0x01) != 0;
    a >>= 1;
    if (!shiftedBit)
        goto RghtFrict; // if left button pressed, thus branch

LeftFrict: // load value set here
    // speed:force is one 16-bit quantity, and so is the friction adder
    wide = ((M(Player_X_Speed) << 8) | M(Player_X_MoveForce))
         + ((M(FrictionAdderHigh) << 8) | M(FrictionAdderLow)); // add to it another value set here
    writeData(Player_X_MoveForce, LOBYTE(wide)); // store here
    writeData(Player_X_Speed, HIBYTE(wide)); // set as new horizontal speed
    a = HIBYTE(wide);
    if (((a - M(MaximumRightSpeed)) & 0x80) != 0)
        goto XSpdSign; // if horizontal speed greater negatively, branch
    a = M(MaximumRightSpeed); // otherwise set preset value as horizontal speed
    writeData(Player_X_Speed, a); // thus slowing the player's left movement down
    goto SetAbsSpd; // skip to the end

RghtFrict: // load value set here
    // speed:force is one 16-bit quantity, and so is the friction adder
    wide = ((M(Player_X_Speed) << 8) | M(Player_X_MoveForce))
         - ((M(FrictionAdderHigh) << 8) | M(FrictionAdderLow)); // subtract from it another value set here
    writeData(Player_X_MoveForce, LOBYTE(wide)); // store here
    writeData(Player_X_Speed, HIBYTE(wide)); // set as new horizontal speed
    a = HIBYTE(wide);
    if (((a - M(MaximumLeftSpeed)) & 0x80) == 0)
        goto XSpdSign; // if horizontal speed greater positively, branch
    a = M(MaximumLeftSpeed); // otherwise set preset value as horizontal speed
    writeData(Player_X_Speed, a); // thus slowing the player's right movement down

XSpdSign: // if player not moving or moving to the right,
    if ((a & 0x80) == 0)
        goto SetAbsSpd; // branch and leave horizontal speed value unmodified
    a ^= 0xff;
    a += 0x01; // unsigned walking/running speed

SetAbsSpd: // store walking/running speed here and leave
    writeData(Player_XSpeedAbsolute, a);
    return;
}

//------------------------------------------------------------------------

bool SMBEngine::SpawnHammerObj()
{
    bool hammerSpawned = false;

    // get pseudorandom bits from
    a = M(PseudoRandomBitReg + 1) & 0b00000111; // second part of LSFR
    if (a == 0)
    { // if any bits are set, branch and use as offset
        a = M(PseudoRandomBitReg + 1) & 0b00001000; // get d3 from same part of LSFR
    } // SetMOfs: use either d3 or d2-d0 for offset here
    y = a;
    a = M(Misc_State + y); // if any values loaded in
    if (a != 0)
        goto NoHammer; // $2a-$32 where offset is then leave, no hammer
    x = M(HammerEnemyOfsData + y); // get offset of enemy slot to check using Y as offset
    a = M(Enemy_Flag + x); // check enemy buffer flag at offset
    if (a != 0)
        goto NoHammer; // if buffer flag set, branch to leave, no hammer
    x = M(ObjectOffset); // get original enemy object offset
    a = x;
    writeData(HammerEnemyOffset + y, a); // save here
    writeData(Misc_State + y, 0x90); // save hammer's state here
    a = 0x07;
    writeData(Misc_BoundBoxCtrl + y, 0x07); // set something else entirely, here
    hammerSpawned = true;
    return hammerSpawned;

//------------------------------------------------------------------------

NoHammer: // get original enemy object offset
    x = M(ObjectOffset);
    hammerSpawned = false;
    return hammerSpawned;
}

//------------------------------------------------------------------------

bool SMBEngine::FindEmptyMiscSlot()
{
    bool miscSlotSearched = false;

    y = 0x08; // start at end of misc objects buffer
    miscSlotSearched = false; // no compare done yet

    FMiscLoop: // get misc object state
    a = M(Misc_State + y);
    if (a != 0)
    { // branch if none found to use current offset
        --y; // decrement offset
        miscSlotSearched = true; // the offset never falls below five, so this sets the carry
        if (y != 0x05)
            goto FMiscLoop; // do this until all slots are checked
        y = 0x08; // if no empty slots found, use last slot
    } // UseMiscS: store offset of misc object buffer here (residual)
    writeData(JumpCoinMiscOffset, y);
    return miscSlotSearched;
}


//------------------------------------------------------------------------

bool SMBEngine::BlockBumpedChk()
{
    bool bumpedBlockFound = false;

    y = 0x0d; // start at end of metatile data

BumpChkLoop: // check to see if current metatile matches
    if (a != M(BrickQBlockMetatiles + y))
    { // metatile found in block buffer, branch if so
        --y; // otherwise move onto next metatile
        if ((y & 0x80) == 0)
            goto BumpChkLoop; // do this until all metatiles are checked
        bumpedBlockFound = false; // if none match
    }
    else
    { // MatchBump
        bumpedBlockFound = true;
    }
    return bumpedBlockFound;
}

//------------------------------------------------------------------------

void SMBEngine::SpawnBrickChunks()
{
    // set horizontal coordinate of block object
    writeData(Block_Orig_XPos + x, M(Block_X_Position + x)); // as original horizontal coordinate here
    writeData(Block_X_Speed + x, 0xf0); // set horizontal speed for brick chunk objects
    writeData(Block_X_Speed + 2 + x, 0xf0);
    writeData(Block_Y_Speed + x, 0xfa); // set vertical speed for one
    writeData(Block_Y_Speed + 2 + x, 0xfc); // set lower vertical speed for the other
    writeData(Block_Y_MoveForce + x, 0x00); // init fractional movement force for both
    writeData(Block_Y_MoveForce + 2 + x, 0x00);
    writeData(Block_PageLoc + 2 + x, M(Block_PageLoc + x)); // copy page location
    writeData(Block_X_Position + 2 + x, M(Block_X_Position + x)); // copy horizontal coordinate
    a = M(Block_Y_Position + x);
    a += 0x08; // and save as vertical coordinate for one of them
    writeData(Block_Y_Position + 2 + x, a);
    a = 0xfa;
    writeData(Block_Y_Speed + x, 0xfa); // set vertical speed...again??? (redundant)
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ExecGameLoopback()
{
    a = M(Player_PageLoc); // send player back four pages
    a -= 0x04;
    writeData(Player_PageLoc, a);
    a = M(CurrentPageLoc); // send current page back four pages
    a -= 0x04;
    writeData(CurrentPageLoc, a);
    a = M(ScreenLeft_PageLoc); // subtract four from page location
    a -= 0x04;
    writeData(ScreenLeft_PageLoc, a);
    a = M(ScreenRight_PageLoc); // do the same for the page location
    a -= 0x04;
    writeData(ScreenRight_PageLoc, a);
    a = M(AreaObjectPageLoc); // subtract four from page control
    a -= 0x04;
    writeData(AreaObjectPageLoc, a);
    // initialize page select for both
    writeData(EnemyObjectPageSel, 0x00); // area and enemy objects
    writeData(AreaObjectPageSel, 0x00);
    writeData(EnemyDataOffset, 0x00); // initialize enemy object data offset
    writeData(EnemyObjectPageLoc, 0x00); // and enemy object page control
    a = M(AreaDataOfsLoopback + y); // adjust area object offset based on
    writeData(AreaDataOffset, a); // which loop command we encountered
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PosPlatform()
{
    uint32_t wide = 0;

    // add or subtract pixels depending on offset, as one 16-bit page:coordinate
    wide = ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x))
         + ((M(PlatPosDataHigh + y) << 8) | M(PlatPosDataLow + y));
    writeData(Enemy_X_Position + x, LOBYTE(wide)); // store as new horizontal coordinate
    writeData(Enemy_PageLoc + x, HIBYTE(wide)); // store as new page location
    a = HIBYTE(wide);
    return; // and go back
}

//------------------------------------------------------------------------

void SMBEngine::ProcSwimmingB(bool blooberCarry)
{
    // get enemy's movement counter
    a = M(BlooperMoveCounter + x) & 0b00000010; // check for d1 set
    if (a == 0)
    { // branch if set
        a = M(FrameCounter) & 0b00000111; // get 3 LSB of frame counter
        pha(); // and save it to the stack
        if ((M(BlooperMoveCounter + x) & 0x01) == 0) // check d0 of the enemy's movement counter
        { // branch if set
            pla(); // pull 3 LSB of frame counter from the stack
            if (a != 0)
                return; // branch to leave, execute code only every eighth frame
            a = M(Enemy_Y_MoveForce + x);
            a += 0x01;
            writeData(Enemy_Y_MoveForce + x, a); // set movement force
            writeData(BlooperMoveSpeed + x, a); // set as movement speed
            if (a != 0x02)
                return; // if certain horizontal speed, branch to leave
            ++M(BlooperMoveCounter + x); // otherwise increment movement counter

            return; // BSwimE

        //------------------------------------------------------------------------
        } // SlowSwim
        pla(); // pull 3 LSB of frame counter from the stack
        if (a != 0)
            return; // branch to leave, execute code only every eighth frame
        a = M(Enemy_Y_MoveForce + x);
        a -= 0x01;
        writeData(Enemy_Y_MoveForce + x, a); // set movement force
        writeData(BlooperMoveSpeed + x, a); // set as movement speed
        if (a != 0)
            return; // if any speed, branch to leave
        ++M(BlooperMoveCounter + x); // otherwise increment movement counter
        a = 0x02;
        writeData(EnemyIntervalTimer + x, 0x02); // set enemy's timer

        return; // NoSSw: leave

    //------------------------------------------------------------------------
    } // ChkForFloatdown
    // get enemy timer
    if (M(EnemyIntervalTimer + x) != 0)
    { // branch if expired

Floatdown:
        // get frame counter
        a = M(FrameCounter) >> 1; // check for d0 set
        if ((M(FrameCounter) & 0x01) == 0)
        { // branch to leave on every other frame
            ++M(Enemy_Y_Position + x); // otherwise increment vertical coordinate
        } // NoFD: leave
        return;

    //------------------------------------------------------------------------
    } // ChkNearPlayer
    // get vertical coordinate
    a = (uint8_t)(M(Enemy_Y_Position + x) + 0x10 + (blooberCarry ? 1 : 0)); // add sixteen pixels, plus whatever carry the swim code left behind
    if (a < M(Player_Y_Position))
        goto Floatdown; // if modified vertical less than player's, branch
    a = 0x00;
    writeData(BlooperMoveCounter + x, 0x00); // otherwise nullify movement counter
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetFirebarPosition()
{
    pha(); // save high byte of spinstate to the stack
    a &= 0b00001111; // mask out low nybble
    if (a >= 0x09)
    { // if lower than $09, branch ahead
        a ^= 0b00001111; // otherwise get two's compliment to oscillate
        a += 0x01;
    } // GetHAdder: store result, modified or not, here
    writeData(0x01, a);
    y = M(0x00); // load number of firebar ball where we're at
    a = M(FirebarTblOffsets + y); // load offset to firebar position data
    a += M(0x01); // add oscillated high byte of spinstate
    y = a; // to offset here and use as new offset
    // get data here and store as horizontal adder
    writeData(0x01, M(FirebarPosLookupTbl + y));
    pla(); // pull whatever was in A from the stack
    pha(); // save it again because we still need it
    a += 0x08; // add eight this time, to get vertical adder
    a &= 0b00001111; // mask out high nybble
    if (a >= 0x09)
    {
        a ^= 0b00001111; // otherwise get two's compliment
        a += 0x01;
    } // GetVAdder: store result here
    writeData(0x02, a);
    y = M(0x00);
    a = M(FirebarTblOffsets + y); // load offset to firebar position data again
    a += M(0x02); // this time add value in $02 to offset here and use as offset
    y = a;
    // get data here and store as vertica adder
    writeData(0x02, M(FirebarPosLookupTbl + y));
    pla(); // pull out whatever was in A one last time
    a >>= 1; // divide by eight or shift three to the right
    a >>= 1;
    a >>= 1;
    y = a; // use as offset
    a = M(FirebarMirrorData + y); // load mirroring data here
    writeData(0x03, a); // store
    return;
}

//------------------------------------------------------------------------

void SMBEngine::FirebarSpin()
{
    uint32_t wide = 0;

    writeData(0x07, a); // save spinning speed here
    // check spinning direction
    if (M(FirebarSpinDirection + x) == 0)
    { // if moving counter-clockwise, branch to other part
        y = 0x18; // possibly residual ldy
        wide = ((M(FirebarSpinState_High + x) << 8) | M(FirebarSpinState_Low + x))
             + M(0x07); // add spinning speed to what would normally be the horizontal speed
        writeData(FirebarSpinState_Low + x, LOBYTE(wide));
        a = HIBYTE(wide); // what would normally be the vertical speed, never stored back
        return;

    //------------------------------------------------------------------------
    } // SpinCounterClockwise
    y = 0x08; // possibly residual ldy
    wide = ((M(FirebarSpinState_High + x) << 8) | M(FirebarSpinState_Low + x))
         - M(0x07); // subtract spinning speed from what would normally be the horizontal speed
    writeData(FirebarSpinState_Low + x, LOBYTE(wide));
    a = HIBYTE(wide); // what would normally be the vertical speed, never stored back
    return;
}

//------------------------------------------------------------------------

        //------------------------------------------------------------------------

void SMBEngine::SetupPlatformRope()
{
    uint32_t wide = 0;

            pha(); // save second/third copy to stack
            wide = M(Enemy_X_Position + y) + 0x08; // get horizontal coordinate, add eight pixels
            a = LOBYTE(wide);
            // if secondary hard mode flag set,
            if (M(SecondaryHardMode) == 0)
            { // use coordinate as-is
                wide = a + 0x10; // otherwise add sixteen more pixels, dropping the carry from the eight
                a = LOBYTE(wide);
            } // GetLRp: save modified horizontal coordinate to stack
            pha();
            a = (uint8_t)(M(Enemy_PageLoc + y) + HIBYTE(wide)); // add carry to page location
            writeData(0x02, a); // and save here
            pla(); // pull modified horizontal coordinate
            a &= 0b11110000; // from the stack, mask out low nybble
            a >>= 1; // and shift three bits to the right
            a >>= 1;
            a >>= 1;
            writeData(0x00, a); // store result here as part of name table low byte
            x = M(Enemy_Y_Position + y); // get vertical coordinate
            pla(); // get second/third copy of vertical speed from stack
            if ((a & 0x80) != 0)
            { // skip this part if moving downwards or not at all
                a = x;
                a += 0x08; // add eight to vertical coordinate and
                x = a; // save as X
            } // GetHRp: move vertical coordinate to A
            a = x;
            x = M(VRAM_Buffer1_Offset); // get vram buffer offset
            wide = a >> 6; // keep d7 and d6 of the vertical coordinate aside
            a = (uint8_t)((a << 2) | (a >> 7)); // rotate d7 round to d0
            pha(); // save modified vertical coordinate to stack
            a = (uint8_t)(wide | 0b00100000); // with d7 and d6 at the 2 LSB, set d5 to get
            writeData(0x01, a); // the appropriate high byte of name table address, then store
            // get saved page location from earlier
            a = M(0x02) & 0x01; // mask out all but LSB
            a <<= 1;
            a <<= 1; // shift twice to the left and save with the
            a |= M(0x01); // rest of the bits of the high byte, to get
            writeData(0x01, a); // the proper name table and the right place on it
            pla(); // get modified vertical coordinate from stack
            a &= 0b11100000; // mask out low nybble and LSB of high nybble
            a += M(0x00); // add to horizontal part saved here
            writeData(0x00, a); // save as name table low byte
            a = M(Enemy_Y_Position + y);
            if (a >= 0xe8)
            { // bottom of the screen, we're done, branch to leave
                a = M(0x00) & 0b10111111; // mask out d6 of low byte of name table address
                writeData(0x00, a);
            } // ExPRp: leave!
            return;
}


//------------------------------------------------------------------------

void SMBEngine::ChkInvisibleMTiles()
{
    if (a != 0x5f)
    { // branch to leave if found
    } // ExCInvT: leave with zero flag set if either found
    return;
}

//------------------------------------------------------------------------

bool SMBEngine::ChkJumpspringMetatiles()
{
    bool jumpspringFound = false;

    if (a != 0x67)
    { // branch to note the jumpspring if found
        jumpspringFound = false;
        if (a != 0x68)
            return jumpspringFound; // branch if not found
    } // JSFnd
    jumpspringFound = true;

    return jumpspringFound; // NoJSFnd: leave
}

//------------------------------------------------------------------------

void SMBEngine::HandlePipeEntry()
{
    // check saved controller bits from earlier
    a = M(Up_Down_Buttons) & 0b00000100; // for pressing down
    if (a == 0)
        return; // if not pressing down, branch to leave
    a = M(0x00);
    if (a != 0x11)
        return; // branch to leave if not found
    a = M(0x01);
    if (a != 0x10)
        return; // branch to leave if not found
    writeData(ChangeAreaTimer, 0x30); // set timer for change of area
    writeData(GameEngineSubroutine, 0x03); // set to run vertical pipe entry routine on next frame
    writeData(Square1SoundQueue, Sfx_PipeDown_Injury); // load pipedown/injury sound
    writeData(Player_SprAttrib, 0b00100000); // set background priority bit in player's attributes
    a = M(WarpZoneControl); // check warp zone control
    if (a == 0)
        return; // branch to leave if none found
    a &= 0b00000011; // mask out all but 2 LSB
    a <<= 1;
    a <<= 1; // multiply by four
    x = a; // save as offset to warp zone numbers (starts at left pipe)
    a = M(Player_X_Position); // get player's horizontal position
    if (a < 0x60)
        goto GetWNum; // if player at left, not near middle, use offset and skip ahead
    ++x; // otherwise increment for middle pipe
    if (a < 0xa0)
        goto GetWNum; // if player at middle, but not too far right, use offset and skip
    ++x; // otherwise increment for last pipe

GetWNum: // get warp zone numbers
    y = M(WarpZoneNumbers + x);
    --y; // decrement for use as world number
    writeData(WorldNumber, y); // store as world number and offset
    x = M(WorldAddrOffsets + y); // get offset to where this world's area offsets are
    // get area offset based on world offset
    writeData(AreaPointer, M(AreaAddrOffsets + x)); // store area offset here to be used to change areas
    writeData(EventMusicQueue, Silence); // silence music
    a = 0x00;
    writeData(EntrancePage, 0x00); // initialize starting page number
    writeData(AreaNumber, 0x00); // initialize area number used for area address offset
    writeData(LevelNumber, 0x00); // initialize level number used for world display
    writeData(AltEntranceControl, 0x00); // initialize mode of entry
    ++M(Hidden1UpFlag); // set flag for hidden 1-up blocks
    ++M(FetchNewGameTimerFlag); // set flag to load new game timer

    return; // ExPipeE: leave!!!
}

//------------------------------------------------------------------------

bool SMBEngine::CheckForCoinMTiles()
{
    bool coinMTileFound = false;

    if (a == 0xc2)
        goto CoinSd; // branch if found
    if (a == 0xc3)
        goto CoinSd; // branch if found
    coinMTileFound = false; // otherwise leave
    return coinMTileFound;

//------------------------------------------------------------------------

CoinSd:
    coinMTileFound = true;
    a = Sfx_CoinGrab;
    writeData(Square2SoundQueue, Sfx_CoinGrab); // load coin grab sound and leave
    return coinMTileFound;
}


//------------------------------------------------------------------------

    //------------------------------------------------------------------------

bool SMBEngine::SubtEnemyYPos()
{
    bool enemyYPosInRange = false;

        a = M(Enemy_Y_Position + x); // add 62 pixels to enemy object's
        a += 0x3e;
        enemyYPosInRange = a >= 0x44; // compare against a certain range
        return enemyYPosInRange; // and leave with the result for a conditional branch
}



//------------------------------------------------------------------------

void SMBEngine::SixSpriteStacker()
{
    x = 0x06; // do six sprites

    do // StkLp: store X or Y coordinate into OAM data
    {
        writeData(Sprite_Data + y, a);
        a += 0x08; // add eight pixels
        ++y;
        ++y; // move offset four bytes forward
        ++y;
        ++y;
        --x; // do another sprite
    } while (x != 0); // do this until all sprites are done
    y = M(0x02); // get saved OAM data offset and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawBubble()
{
    y = M(Player_Y_HighPos); // if player's vertical high position
    --y; // not within screen, skip all of this
    if (y != 0)
        return;
    // check air bubble's offscreen bits
    a = M(Bubble_OffscreenBits) & 0b00001000;
    if (a != 0)
        return; // if bit set, branch to leave
    y = M(Bubble_SprDataOffset + x); // get air bubble's OAM data offset
    // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, M(Bubble_Rel_XPos)); // store as X coordinate here
    // get relative vertical coordinate
    writeData(Sprite_Y_Position + y, M(Bubble_Rel_YPos)); // store as Y coordinate here
    writeData(Sprite_Tilenumber + y, 0x74); // put air bubble tile into OAM data
    a = 0x02;
    writeData(Sprite_Attributes + y, 0x02); // set attribute byte

    return; // ExDBub: leave
}

//------------------------------------------------------------------------

void SMBEngine::GetGfxOffsetAdder()
{
    a = M(PlayerSize); // get player's size
    if (a != 0)
    { // if player big, use current offset as-is
        a = y; // for big player
        a += 0x08; // for small player
        y = a;
    } // SzOfs: go back
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ChkForPlayerAttrib()
{
    y = M(Player_SprDataOffset); // get sprite data offset
    if (M(GameEngineSubroutine) != 0x0b)
    { // branch to change third and fourth row OAM attributes
        a = M(PlayerGfxOffset); // get graphics table offset
        if (a == 0x50)
            goto C_S_IGAtt; // if crouch offset, either standing offset,
        if (a == 0xb8)
            goto C_S_IGAtt; // go ahead and execute code to change
        if (a == 0xc0)
            goto C_S_IGAtt;
        if (a != 0xc8)
            return; // if none of these, branch to leave
    } // KilledAtt
    a = M(Sprite_Attributes + 16 + y) & 0b00111111; // mask out horizontal and vertical flip bits
    writeData(Sprite_Attributes + 16 + y, a); // for third row sprites and save
    a = M(Sprite_Attributes + 20 + y) & 0b00111111;
    a |= 0b01000000; // set horizontal flip bit for second
    writeData(Sprite_Attributes + 20 + y, a); // sprite in the third row

C_S_IGAtt:
    a = M(Sprite_Attributes + 24 + y) & 0b00111111; // mask out horizontal and vertical flip bits
    writeData(Sprite_Attributes + 24 + y, a); // for fourth row sprites and save
    a = M(Sprite_Attributes + 28 + y) & 0b00111111;
    a |= 0b01000000; // set horizontal flip bit for second
    writeData(Sprite_Attributes + 28 + y, a); // sprite in the fourth row

    return; // ExPlyrAt: leave
}


//------------------------------------------------------------------------

void SMBEngine::GetProperObjOffset()
{
    a = x; // move offset to A
    a += M(ObjOffsetData + y); // add amount of bytes to offset depending on setting in Y
    x = a; // put back in X and leave
    return;
}






//------------------------------------------------------------------------

void SMBEngine::PlayerLakituDiff()
{
    bool enemyRightOfPlayer = false;

    y = 0x00; // set Y for default value
    enemyRightOfPlayer = PlayerEnemyDiff(); // get horizontal difference between enemy and player
    if ((a & 0x80) != 0)
    { // branch if enemy is to the right of the player
        ++y; // increment Y for left of player
        a = M(0x00) ^ 0xff; // get two's compliment of low byte of horizontal difference
        a += 0x01; // store two's compliment as horizontal difference
        writeData(0x00, a);
    } // ChkLakDif: get low byte of horizontal difference
    if (M(0x00) < 0x3c)
        goto ChkPSpeed;
    // otherwise set maximum distance
    writeData(0x00, 0x3c);
    // check if lakitu is in our current enemy slot
    if (M(Enemy_ID + x) != Lakitu)
        goto ChkPSpeed; // if not, branch elsewhere
    a = y; // compare contents of Y, now in A
    if (a == M(LakituMoveDirection + x))
        goto ChkPSpeed; // if moving toward the player, branch, do not alter
    // if moving to the left beyond maximum distance,
    if (M(LakituMoveDirection + x) != 0)
    { // branch and alter without delay
        --M(LakituMoveSpeed + x); // decrement horizontal speed
        a = M(LakituMoveSpeed + x); // if horizontal speed not yet at zero, branch to leave
        if (a != 0)
            return;
    } // SetLMovD: set horizontal direction depending on horizontal
    a = y;
    writeData(LakituMoveDirection + x, a); // difference between enemy and player if necessary

ChkPSpeed:
    a = M(0x00) & 0b00111100; // mask out all but four bits in the middle
    a >>= 1; // divide masked difference by four
    a >>= 1;
    writeData(0x00, a); // store as new value
    y = 0x00; // init offset
    if (M(Player_X_Speed) == 0)
        goto SubDifAdj; // if player not moving horizontally, branch
    if (M(ScrollAmount) == 0)
        goto SubDifAdj; // if scroll speed not set, branch to same place
    y = 0x01; // otherwise increment offset
    if (M(Player_X_Speed) < 0x19)
        goto ChkSpinyO;
    if (M(ScrollAmount) < 0x02)
        goto ChkSpinyO; // to same place
    y = 0x02; // otherwise increment once more

ChkSpinyO: // check for spiny object
    if (M(Enemy_ID + x) == Spiny)
    { // branch if not found
        // if player not moving, skip this part
        if (M(Player_X_Speed) != 0)
            goto SubDifAdj;
    } // ChkEmySpd: check vertical speed
    if (M(Enemy_Y_Speed + x) != 0)
        goto SubDifAdj; // branch if nonzero
    y = 0x00; // otherwise reinit offset

SubDifAdj: // get one of three saved values from earlier
    a = M(0x0001 + y);
    y = M(0x00); // get saved horizontal difference

    do // SPixelLak: subtract one for each pixel of horizontal difference
    {
        a -= 0x01; // from one of three saved values
        --y;
    } while ((y & 0x80) == 0); // branch until all pixels are subtracted, to adjust difference

    return; // ExMoveLak: leave!!!
}


//------------------------------------------------------------------------

void SMBEngine::ChkForLandJumpSpring()
{
    bool jumpspringFound = false;

    jumpspringFound = ChkJumpspringMetatiles(); // do sub to check if player landed on jumpspring
    if (jumpspringFound)
    { // jumpspring not found, therefore leave
        writeData(VerticalForce, 0x70); // otherwise set vertical movement force for player
        writeData(JumpspringForce, 0xf9); // set default jumpspring force
        writeData(JumpspringTimer, 0x03); // set jumpspring timer to be used later
        a = 0x01;
        writeData(JumpspringAnimCtrl, 0x01); // set jumpspring animation control to start animating
    } // ExCJSp: and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawVine()
{
    writeData(0x00, y); // save offset here
    a = M(Enemy_Rel_YPos); // get relative vertical coordinate
    a += M(VineYPosAdder + y); // add value using offset in Y to get value
    x = M(VineObjOffset + y); // get offset to vine
    y = M(Enemy_SprDataOffset + x); // get sprite data offset
    writeData(0x02, y); // store sprite data offset here
    SixSpriteStacker(); // stack six sprites on top of each other vertically
    a = M(Enemy_Rel_XPos); // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, a); // store in first, third and fifth sprites
    writeData(Sprite_X_Position + 8 + y, a);
    writeData(Sprite_X_Position + 16 + y, a);
    a += 0x06; // add six pixels to second, fourth and sixth sprites
    writeData(Sprite_X_Position + 4 + y, a); // to give characteristic staggered vine shape to
    writeData(Sprite_X_Position + 12 + y, a); // our vertical stack of sprites
    writeData(Sprite_X_Position + 20 + y, a);
    // set bg priority and palette attribute bits
    writeData(Sprite_Attributes + y, 0b00100001); // set in first, third and fifth sprites
    writeData(Sprite_Attributes + 8 + y, 0b00100001);
    writeData(Sprite_Attributes + 16 + y, 0b00100001);
    a = 0b01100001; // additionally, set horizontal flip bit
    writeData(Sprite_Attributes + 4 + y, 0b01100001); // for second, fourth and sixth sprites
    writeData(Sprite_Attributes + 12 + y, 0b01100001);
    writeData(Sprite_Attributes + 20 + y, 0b01100001);
    x = 0x05; // set tiles for six sprites

    do // VineTL: set tile number for sprite
    {
        a = 0xe1;
        writeData(Sprite_Tilenumber + y, 0xe1);
        ++y; // move offset to next sprite data
        ++y;
        ++y;
        ++y;
        --x; // move onto next sprite
    } while ((x & 0x80) == 0); // loop until all sprites are done
    y = M(0x02); // get original offset
    // get offset to vine adding data
    if (M(0x00) == 0)
    { // if offset not zero, skip this part
        a = 0xe0;
        writeData(Sprite_Tilenumber + y, 0xe0); // set other tile number for top of vine
    } // SkpVTop: start with the first sprite again
    x = 0x00;

    do // ChkFTop: get original starting vertical coordinate
    {
        a = M(VineStart_Y_Position);
        a -= M(Sprite_Y_Position + y); // subtract top-most sprite's Y coordinate
        if (a >= 0x64)
        { // apart, skip this to leave sprite alone
            a = 0xf8;
            writeData(Sprite_Y_Position + y, 0xf8); // otherwise move sprite offscreen
        } // NextVSp: move offset to next OAM data
        ++y;
        ++y;
        ++y;
        ++y;
        ++x; // move onto next sprite
    } while (x != 0x06);
    y = M(0x00); // return offset set earlier
    return;
}



//------------------------------------------------------------------------

// use area object identifier bit as offset
void SMBEngine::AreaFrenzy()
{
    x = M(0x00);
    a = M(FrenzyIDData - 8 + x); // note that it starts at 8, thus weird address here
    y = 0x05;

FreCompLoop: // check regular slots of enemy object buffer
    --y;
    if ((y & 0x80) == 0)
    { // if all slots checked and enemy object not found, branch to store
        if (a != M(Enemy_ID + y))
            goto FreCompLoop;
        a = 0x00; // if enemy object already present, nullify queue and leave
    } // ExitAFrenzy: store enemy into frenzy queue
    writeData(EnemyFrenzyQueue, a);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::WaterPipe()
{
    GetLrgObjAttrib(); // get row and lower nybble
    y = M(AreaObjectLength + x); // get length (residual code, water pipe is 1 col thick)
    x = M(0x07); // get row
    writeData(MetatileBuffer + x, 0x6b); // draw something here and below it
    a = 0x6c;
    writeData(MetatileBuffer + 1 + x, 0x6c);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Jumpspring()
{
    bool allEnemySlotsFull = false;

    GetLrgObjAttrib();
    allEnemySlotsFull = FindEmptyEnemySlot(); // find empty space in enemy object buffer
    GetAreaObjXPosition(); // get horizontal coordinate for jumpspring
    writeData(Enemy_X_Position + x, a); // and store
    // store page location of jumpspring
    writeData(Enemy_PageLoc + x, M(CurrentPageLoc));
    GetAreaObjYPosition(); // get vertical coordinate for jumpspring
    writeData(Enemy_Y_Position + x, a); // and store
    writeData(Jumpspring_FixedYPos + x, a); // store as permanent coordinate here
    writeData(Enemy_ID + x, JumpspringObject); // write jumpspring object to enemy object buffer
    y = 0x01;
    writeData(Enemy_Y_HighPos + x, 0x01); // store vertical high byte
    ++M(Enemy_Flag + x); // set flag for enemy object buffer
    x = M(0x07);
    // draw metatiles in two rows where jumpspring is
    writeData(MetatileBuffer + x, 0x67);
    a = 0x68;
    writeData(MetatileBuffer + 1 + x, 0x68);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ClimbingSub()
{
    bool shiftedBit = false;
    uint32_t wide = 0;

    y = 0x00; // set default adder here
    // get player's vertical speed
    if ((M(Player_Y_Speed) & 0x80) != 0)
    { // if not moving upwards, branch
        y = 0xff; // otherwise set adder to $ff
    } // MoveOnVine: store adder here
    writeData(0x00, y);
    // highpos:position:dummy is one 24-bit quantity, and $00:speed the signed
    // 16-bit amount to move the player up or down by
    wide = (M(Player_Y_HighPos) << 16) | (M(Player_Y_Position) << 8) | M(Player_YMF_Dummy);
    wide += (M(0x00) << 16) | (M(Player_Y_Speed) << 8) | M(Player_Y_MoveForce);
    writeData(Player_YMF_Dummy, LOBYTE(wide)); // add movement force to dummy variable
    writeData(Player_Y_Position, HIBYTE(wide)); // and store to move player up or down
    writeData(Player_Y_HighPos, (uint8_t)(wide >> 16)); // and store
    a = (uint8_t)(wide >> 16);
    // compare left/right controller bits
    a = M(Left_Right_Buttons) & M(Player_CollisionBits); // to collision flag
    if (a != 0)
    { // if not set, skip to end
        y = M(ClimbSideTimer); // otherwise check timer
        if (y == 0)
        { // if timer not expired, branch to leave
            writeData(ClimbSideTimer, 0x18); // otherwise set timer now
            x = 0x00; // set default offset here
            y = M(PlayerFacingDir); // get facing direction
            shiftedBit = (a & 0x01) != 0;
            if (!shiftedBit) // check the right button controller bit
            { // if controller right pressed, branch ahead
                x = 0x02; // otherwise increment offset by 2 bytes
            } // ClimbFD: check to see if facing right
            --y;
            if (y != 0)
            { // if so, branch, do not increment
                ++x; // otherwise increment by 1 byte
            } // CSetFDir
            // add to or subtract from the player's 16-bit horizontal position, using the
            // 16-bit value here as the adder and X as offset
            wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position))
                 + ((M(ClimbAdderHigh + x) << 8) | M(ClimbAdderLow + x));
            writeData(Player_X_Position, LOBYTE(wide));
            writeData(Player_PageLoc, HIBYTE(wide));
            a = HIBYTE(wide);
            // get left/right controller bits again
            a = M(Left_Right_Buttons) ^ 0b00000011; // invert them and store them while player
            writeData(PlayerFacingDir, a); // is on vine to face player in opposite direction
        } // ExitCSub: then leave
        return;

    //------------------------------------------------------------------------
    } // InitCSTimer: initialize timer here
    writeData(ClimbSideTimer, a);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ProcessWhirlpools()
{
    uint32_t wide = 0;

    a = M(AreaType); // check for water type level
    if (a != 0)
        return; // branch to leave if not found
    writeData(Whirlpool_Flag, a); // otherwise initialize whirlpool flag
    a = M(TimerControl); // if master timer control set,
    if (a != 0)
        return; // branch to leave
    y = 0x04; // otherwise start with last whirlpool data

WhLoop: // get left extent of whirlpool
    wide = ((M(Whirlpool_PageLoc + y) << 8) | M(Whirlpool_LeftExtent + y))
         + M(Whirlpool_Length + y); // add length of whirlpool
    writeData(0x02, LOBYTE(wide)); // store result as right extent here
    a = M(Whirlpool_PageLoc + y); // get page location
    if (a == 0)
        goto NextWh; // if none or page 0, branch to get next data
    writeData(0x01, HIBYTE(wide)); // store result as page location of right extent here
    a = HIBYTE(wide);
    // the player and the left extent are each one 16-bit page:coordinate
    wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position))
         - ((M(Whirlpool_PageLoc + y) << 8) | M(Whirlpool_LeftExtent + y)); // subtract left extent
    a = HIBYTE(wide);
    if ((a & 0x80) != 0)
        goto NextWh; // if player too far left, branch to get next data
    // the right extent and the player are each one 16-bit page:coordinate
    wide = ((M(0x01) << 8) | M(0x02)) // otherwise get right extent
         - ((M(Player_PageLoc) << 8) | M(Player_X_Position)); // subtract player's horizontal coordinate
    a = HIBYTE(wide);
    if ((a & 0x80) != 0)
    { // if player within right extent, branch to whirlpool code

NextWh: // move onto next whirlpool data
        --y;
        if ((y & 0x80) == 0)
            goto WhLoop; // do this until all whirlpools are checked

        return; // ExitWh: leave

    //------------------------------------------------------------------------
    } // WhirlpoolActivate
    // get length of whirlpool
    a = M(Whirlpool_Length + y) >> 1; // divide by 2
    writeData(0x00, a); // save here
    // get left extent of whirlpool
    wide = ((M(Whirlpool_PageLoc + y) << 8) | M(Whirlpool_LeftExtent + y)) + M(0x00); // add length divided by 2
    writeData(0x01, LOBYTE(wide)); // save as center of whirlpool
    writeData(0x00, HIBYTE(wide)); // save as page location of whirlpool center
    a = HIBYTE(wide); // get page location
    // get frame counter
    a = M(FrameCounter) >> 1; // check d0 (to run on every other frame)
    if ((M(FrameCounter) & 0x01) == 0)
        goto WhPull; // if d0 not set, branch to last part of code
    // the center and the player are each one 16-bit page:coordinate
    wide = ((M(0x00) << 8) | M(0x01)) // get center
         - ((M(Player_PageLoc) << 8) | M(Player_X_Position)); // subtract player's horizontal coordinate
    a = HIBYTE(wide);
    if ((a & 0x80) != 0)
    { // if player to the left of center, branch
        wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) - 0x01; // otherwise slowly pull player left, towards the center
        writeData(Player_X_Position, LOBYTE(wide)); // set player's new horizontal coordinate
        a = HIBYTE(wide);
    } // LeftWh: get player's collision bits
    else // jump to set player's new page location
    {
        a = M(Player_CollisionBits) >> 1; // take d0
        if ((M(Player_CollisionBits) & 0x01) == 0)
            goto WhPull; // if d0 not set, branch
        wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + 0x01; // otherwise slowly pull player right, towards the center
        writeData(Player_X_Position, LOBYTE(wide)); // set player's new horizontal coordinate
        a = HIBYTE(wide);
    } // SetPWh: set player's new page location
    writeData(Player_PageLoc, a);

WhPull:
    writeData(0x00, 0x10); // set vertical movement force
    writeData(Whirlpool_Flag, 0x01); // set whirlpool flag to be used later
    writeData(0x02, 0x01); // also set maximum vertical speed
    a = 0x00;
    x = 0x00; // set X for player offset
    ImposeGravity(); // jump to put whirlpool effect on player vertically, do not return
    return;
}



//------------------------------------------------------------------------

void SMBEngine::InitRetainerObj()
{
    a = 0xb8; // set fixed vertical position for
    writeData(Enemy_Y_Position + x, 0xb8); // princess/mushroom retainer object
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitBulletBill()
{
    // set moving direction for left
    writeData(Enemy_MovingDir + x, 0x02);
    a = 0x09; // set bounding box control for $09
    writeData(Enemy_BoundBoxCtrl + x, 0x09);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitFireworks()
{
    uint32_t wide = 0;

    a = M(FrenzyEnemyTimer); // if timer not expired yet, branch to leave
    if (a == 0)
    {
        a = 0x20; // otherwise reset timer
        writeData(FrenzyEnemyTimer, 0x20);
        --M(FireworksCounter); // decrement for each explosion
        y = 0x06; // start at last slot

        do // StarFChk
        {
            --y;
            // check for presence of star flag object
        } while (M(Enemy_ID + y) != StarFlagObject); // routine goes into infinite loop = crash
        wide = ((M(Enemy_PageLoc + y) << 8) | M(Enemy_X_Position + y)) // get horizontal coordinate of star flag object, then
             - 0x30; // subtract 48 pixels from it
        a = LOBYTE(wide);
        pha(); // and save to the stack
        a = HIBYTE(wide);
        writeData(0x00, a); // the page location of the star flag object, less the borrow
        a = M(FireworksCounter); // get fireworks counter
        a += M(Enemy_State + y); // add state of star flag object (possibly not necessary)
        y = a; // use as offset
        pla(); // get saved horizontal coordinate of star flag - 48 pixels
        wide = ((M(0x00) << 8) | a) + M(FireworksXPosData + y); // add number based on offset of fireworks counter
        writeData(Enemy_X_Position + x, LOBYTE(wide)); // store as the fireworks object horizontal coordinate
        writeData(Enemy_PageLoc + x, HIBYTE(wide)); // the fireworks object
        a = HIBYTE(wide);
        // get vertical position using same offset
        writeData(Enemy_Y_Position + x, M(FireworksYPosData + y)); // and store as vertical coordinate for fireworks object
        writeData(Enemy_Y_HighPos + x, 0x01); // store in vertical high byte
        writeData(Enemy_Flag + x, 0x01); // and activate enemy buffer flag
        writeData(ExplosionGfxCounter + x, 0x00); // initialize explosion counter
        a = 0x08;
        writeData(ExplosionTimerCounter + x, 0x08); // set explosion timing counter
    } // ExitFWk
    return;
}

//------------------------------------------------------------------------

void SMBEngine::EndFrenzy()
{
    y = 0x05; // start at last slot

    do // LakituChk: check enemy identifiers
    {
        if (M(Enemy_ID + y) == Lakitu)
        {
            a = 0x01; // if found, set state
            writeData(Enemy_State + y, 0x01);
        } // NextFSlot: move onto the next slot
        --y;
    } while ((y & 0x80) == 0); // do this until all slots are checked
    a = 0x00;
    writeData(EnemyFrenzyBuffer, 0x00); // empty enemy frenzy buffer
    writeData(Enemy_Flag + x, 0x00); // disable enemy buffer flag for this object
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MovePiranhaPlant()
{
    bool enemyRightOfPlayer = false;

    // check enemy state
    if (M(Enemy_State + x) != 0)
        goto PutinPipe; // if set at all, branch to leave
    // check enemy's timer here
    if (M(EnemyFrameTimer + x) != 0)
        goto PutinPipe; // branch to end if not yet expired
    // check movement flag
    if (M(PiranhaPlant_MoveFlag + x) == 0)
    { // if moving, skip to part ahead
        // if currently rising, branch
        if ((M(PiranhaPlant_Y_Speed + x) & 0x80) == 0)
        { // to move enemy upwards out of pipe
            enemyRightOfPlayer = PlayerEnemyDiff(); // get horizontal difference between player and
            if ((a & 0x80) != 0)
            { // piranha plant, and branch if enemy to right of player
                // otherwise get saved horizontal difference
                a = M(0x00) ^ 0xff;
                a += 0x01;
                writeData(0x00, a); // save as new horizontal difference
            } // ChkPlayerNearPipe
            // get saved horizontal difference
            if (M(0x00) < 0x21)
                goto PutinPipe; // if player within a certain distance, branch to leave
        } // ReversePlantSpeed
        // get vertical speed
        a = M(PiranhaPlant_Y_Speed + x) ^ 0xff;
        a += 0x01;
        writeData(PiranhaPlant_Y_Speed + x, a); // save as new vertical speed
        ++M(PiranhaPlant_MoveFlag + x); // increment to set movement flag
    } // SetupToMovePPlant
    a = M(PiranhaPlantDownYPos + x); // get original vertical coordinate (lowest point)
    y = M(PiranhaPlant_Y_Speed + x); // get vertical speed
    if ((y & 0x80) != 0)
    { // branch if moving downwards
        a = M(PiranhaPlantUpYPos + x); // otherwise get other vertical coordinate (highest point)
    } // RiseFallPiranhaPlant
    writeData(0x00, a); // save vertical coordinate here
    // get frame counter
    a = M(FrameCounter) >> 1;
    if ((M(FrameCounter) & 0x01) == 0)
        goto PutinPipe; // branch to leave if d0 set (execute code every other frame)
    // get master timer control
    if (M(TimerControl) != 0)
        goto PutinPipe; // branch to leave if set (likely not necessary)
    a = M(Enemy_Y_Position + x); // get current vertical coordinate
    a += M(PiranhaPlant_Y_Speed + x); // add vertical speed to move up or down
    writeData(Enemy_Y_Position + x, a); // save as new vertical coordinate
    if (a != M(0x00))
        goto PutinPipe; // branch to leave if not yet reached
    writeData(PiranhaPlant_MoveFlag + x, 0x00); // otherwise clear movement flag
    a = 0x40;
    writeData(EnemyFrameTimer + x, 0x40); // set timer to delay piranha plant movement

PutinPipe:
    a = 0b00100000; // set background priority bit in sprite
    writeData(Enemy_SprAttrib + x, 0b00100000); // attributes to give illusion of being inside pipe
    return; // then leave
}




//------------------------------------------------------------------------

void SMBEngine::CastleObject()
{
    bool allEnemySlotsFull = false;
    bool lrgObjJustStarted = false;

    GetLrgObjAttrib(); // save lower nybble as starting row
    writeData(0x07, y); // if starting row is above $0a, game will crash!!!
    y = 0x04;
    lrgObjJustStarted = ChkLrgObjFixedLength(); // load length of castle if not already loaded
    a = x;
    pha(); // save obj buffer offset to stack
    y = M(AreaObjectLength + x); // use current length as offset for castle data
    x = M(0x07); // begin at starting row
    a = 0x0b;
    writeData(0x06, 0x0b); // load upper limit of number of rows to print

    do // CRendLoop: load current byte using offset
    {
        writeData(MetatileBuffer + x, M(CastleMetatiles + y));
        ++x; // store in buffer and increment buffer offset
        if (M(0x06) != 0)
        { // have we reached upper limit yet?
            ++y; // if not, increment column-wise
            ++y; // to byte in next row
            ++y;
            ++y;
            ++y;
            --M(0x06); // move closer to upper limit
        } // ChkCFloor: have we reached the row just before floor?
    } while (x != 0x0b); // if not, go back and do another row
    pla();
    x = a; // get obj buffer offset from before
    a = M(CurrentPageLoc);
    if (a == 0)
        return; // if we're at page 0, we do not need to do anything else
    a = M(AreaObjectLength + x); // check length
    if (a == 0x01)
        goto PlayerStop;
    y = M(0x07); // check starting row for tall castle ($00)
    if (y == 0)
    {
        if (a == 0x03)
            goto PlayerStop;
    } // NotTall: if not tall castle, check to see if we're at the third column
    if (a != 0x02)
        return; // if we aren't and the castle is tall, don't create flag yet
    GetAreaObjXPosition(); // otherwise, obtain and save horizontal pixel coordinate
    pha();
    allEnemySlotsFull = FindEmptyEnemySlot(); // find an empty place on the enemy object buffer
    pla();
    writeData(Enemy_X_Position + x, a); // then write horizontal coordinate for star flag
    writeData(Enemy_PageLoc + x, M(CurrentPageLoc)); // set page location for star flag
    writeData(Enemy_Y_HighPos + x, 0x01); // set vertical high byte
    writeData(Enemy_Flag + x, 0x01); // set flag for buffer
    writeData(Enemy_Y_Position + x, 0x90); // set vertical coordinate
    a = StarFlagObject; // set star flag value in buffer itself
    writeData(Enemy_ID + x, StarFlagObject);
    return;

//------------------------------------------------------------------------

PlayerStop: // put brick at floor to stop player at end of level
    y = 0x52;
    writeData(MetatileBuffer + 10, 0x52); // this is only done if we're on the second column

    return; // ExitCastle
}

//------------------------------------------------------------------------

void SMBEngine::GetPipeHeight()
{
    bool lrgObjJustStarted = false;

    y = 0x01; // check for length loaded, if not, load
    lrgObjJustStarted = ChkLrgObjFixedLength(); // pipe length of 2 (horizontal)
    GetLrgObjAttrib();
    a = y; // get saved lower nybble as height
    a &= 0x07; // save only the three lower bits as
    writeData(0x06, a); // vertical length, then load Y with
    y = M(AreaObjectLength + x); // length left over
    return;
}

//------------------------------------------------------------------------

bool SMBEngine::ChkLrgObjLength()
{
    bool lrgObjJustStarted = false;

    GetLrgObjAttrib(); // get row location and size (length if branched to from here)
    lrgObjJustStarted = ChkLrgObjFixedLength();
    return lrgObjJustStarted;
}

//------------------------------------------------------------------------

bool SMBEngine::ChkLrgObjFixedLength()
{
    bool lrgObjJustStarted = false;

    a = M(AreaObjectLength + x); // check for set length counter
    lrgObjJustStarted = false; // not just starting
    if ((a & 0x80) != 0)
    { // if counter not set, load it, otherwise leave alone
        a = y; // save length into length counter
        writeData(AreaObjectLength + x, a);
        lrgObjJustStarted = true; // just starting
    } // LenSet
    return lrgObjJustStarted;
}

//------------------------------------------------------------------------

void SMBEngine::Hole_Water()
{
    bool lrgObjJustStarted = false;

    lrgObjJustStarted = ChkLrgObjLength(); // get low nybble and save as length
    // render waves
    writeData(MetatileBuffer + 10, 0x86);
    x = 0x0b;
    y = 0x01; // now render the water underneath
    a = 0x87;
    RenderUnderPart();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::FlagBalls_Residual()
{
    GetLrgObjAttrib(); // get low nybble from object byte
    x = 0x02; // render flag balls on third row from top
    a = 0x6d; // of screen downwards based on low nybble
    RenderUnderPart();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::FlagpoleObject()
{
    uint32_t wide = 0;

    // render flagpole ball on top
    writeData(MetatileBuffer, 0x24);
    x = 0x01; // now render the flagpole shaft
    y = 0x08;
    a = 0x25;
    RenderUnderPart();
    a = 0x61; // render solid block at the bottom
    writeData(MetatileBuffer + 10, 0x61);
    GetAreaObjXPosition();
    wide = ((M(CurrentPageLoc) << 8) | a) - 0x08; // subtract eight pixels and use as horizontal
    writeData(Enemy_X_Position + 5, LOBYTE(wide)); // coordinate for the flag
    writeData(Enemy_PageLoc + 5, HIBYTE(wide)); // page location for the flag
    a = HIBYTE(wide);
    writeData(Enemy_Y_Position + 5, 0x30); // set vertical coordinate for flag
    writeData(FlagpoleFNum_Y_Pos, 0xb0); // set initial vertical coordinate for flagpole's floatey number
    a = FlagpoleFlagObject;
    writeData(Enemy_ID + 5, FlagpoleFlagObject); // set flag identifier, note that identifier and coordinates
    ++M(Enemy_Flag + 5); // use last space in enemy object buffer
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RowOfCoins()
{
    y = M(AreaType); // get area type
    a = M(CoinMetatileData + y); // load appropriate coin metatile
    GetRow();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::EmptyBlock()
{
    GetLrgObjAttrib(); // get row location
    x = M(0x07);
    a = 0xc4;
    ColObj();
    return;
}

//------------------------------------------------------------------------

// column length of 1
void SMBEngine::ColObj()
{
    y = 0x00;
    RenderUnderPart();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RowOfBricks()
{
    y = M(AreaType); // load area type obtained from area offset pointer
    // check for cloud type override
    if (M(CloudTypeOverride) != 0)
    {
        y = 0x04; // if cloud type, override area type
    } // DrawBricks: get appropriate metatile
    a = M(BrickMetatiles + y);
    GetRow(); // and go render it
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RowOfSolidBlocks()
{
    y = M(AreaType); // load area type obtained from area offset pointer
    a = M(SolidBlockMetatiles + y); // get metatile
    GetRow();
    return;
}

//------------------------------------------------------------------------

// store metatile here
void SMBEngine::GetRow()
{
    bool lrgObjJustStarted = false;

    pha();
    lrgObjJustStarted = ChkLrgObjLength(); // get row number, load length
    DrawRow();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawRow()
{
    x = M(0x07);
    y = 0x00; // set vertical height of 1
    pla();
    RenderUnderPart(); // render object
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ColumnOfBricks()
{
    y = M(AreaType); // load area type obtained from area offset
    a = M(BrickMetatiles + y); // get metatile (no cloud override as for row)
    GetRow2();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ColumnOfSolidBlocks()
{
    y = M(AreaType); // load area type obtained from area offset
    a = M(SolidBlockMetatiles + y); // get metatile
    GetRow2();
    return;
}

//------------------------------------------------------------------------

// save metatile to stack for now
void SMBEngine::GetRow2()
{
    pha();
    GetLrgObjAttrib(); // get length and row
    pla(); // restore metatile
    x = M(0x07); // get starting row
    RenderUnderPart(); // now render the column
    return;
}

//------------------------------------------------------------------------

void SMBEngine::BulletBillCannon()
{
    GetLrgObjAttrib(); // get row and length of bullet bill cannon
    x = M(0x07); // start at first row
    a = 0x64; // render bullet bill cannon
    writeData(MetatileBuffer + x, 0x64);
    ++x;
    --y; // done yet?
    if ((y & 0x80) != 0)
        goto SetupCannon;
    a = 0x65; // if not, render middle part
    writeData(MetatileBuffer + x, 0x65);
    ++x;
    --y; // done yet?
    if ((y & 0x80) != 0)
        goto SetupCannon;
    a = 0x66; // if not, render bottom until length expires
    RenderUnderPart();

SetupCannon: // get offset for data used by cannons and whirlpools
    x = M(Cannon_Offset);
    GetAreaObjYPosition(); // get proper vertical coordinate for cannon
    writeData(Cannon_Y_Position + x, a); // and store it here
    writeData(Cannon_PageLoc + x, M(CurrentPageLoc)); // store page number for cannon here
    GetAreaObjXPosition(); // get proper horizontal coordinate for cannon
    writeData(Cannon_X_Position + x, a); // and store it here
    ++x;
    if (x >= 0x06)
    { // if not yet reached sixth cannon, branch to save offset
        x = 0x00; // otherwise initialize it
    } // StrCOffset: save new offset and leave
    writeData(Cannon_Offset, x);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::StaircaseObject()
{
    bool lrgObjJustStarted = false;

    lrgObjJustStarted = ChkLrgObjLength(); // check and load length
    if (lrgObjJustStarted)
    { // if length already loaded, skip init part
        a = 0x09; // start past the end for the bottom
        writeData(StaircaseControl, 0x09); // of the staircase
    } // NextStair: move onto next step (or first if starting)
    --M(StaircaseControl);
    y = M(StaircaseControl);
    x = M(StaircaseRowData + y); // get starting row and height to render
    y = M(StaircaseHeightData + y);
    a = 0x61; // now render solid block staircase
    RenderUnderPart();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Hole_Empty()
{
    uint32_t wide = 0;
    bool lrgObjJustStarted = false;

    lrgObjJustStarted = ChkLrgObjLength(); // get lower nybble and save as length
    if (!lrgObjJustStarted)
        goto NoWhirlP; // skip this part if length already loaded
    // check for water type level
    if (M(AreaType) != 0)
        goto NoWhirlP; // if not water type, skip this part
    x = M(Whirlpool_Offset); // get offset for data used by cannons and whirlpools
    GetAreaObjXPosition(); // get proper vertical coordinate of where we're at
    wide = ((M(CurrentPageLoc) << 8) | a) - 0x10; // subtract 16 pixels
    writeData(Whirlpool_LeftExtent + x, LOBYTE(wide)); // store as left extent of whirlpool
    writeData(Whirlpool_PageLoc + x, HIBYTE(wide)); // save as page location of whirlpool
    a = HIBYTE(wide); // get page location of where we're at
    ++y;
    ++y; // increment length by 2
    a = y;
    a <<= 1; // multiply by 16 to get size of whirlpool
    a <<= 1; // note that whirlpool will always be
    a <<= 1; // two blocks bigger than actual size of hole
    a <<= 1; // and extend one block beyond each edge
    writeData(Whirlpool_Length + x, a); // save size of whirlpool here
    ++x;
    if (x >= 0x05)
    { // if not yet reached fifth whirlpool, branch to save offset
        x = 0x00; // otherwise initialize it
    } // StrWOffset: save new offset here
    writeData(Whirlpool_Offset, x);
    goto NoWhirlP;

NoWhirlP: // get appropriate metatile, then
    x = M(AreaType);
    a = M(HoleMetatiles + x); // render the hole proper
    x = 0x08;
    y = 0x0f; // start at ninth row and go to bottom, run RenderUnderPart
    RenderUnderPart();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RenderUnderPart()
{
RenderUnderPart:
    writeData(AreaObjectHeight, y); // store vertical length to render
    y = M(MetatileBuffer + x); // check current spot to see if there's something
    if (y == 0)
        goto DrawThisRow; // we need to keep, if nothing, go ahead
    if (y == 0x17)
        goto WaitOneRow; // if middle part (tree ledge), wait until next row
    if (y == 0x1a)
        goto WaitOneRow; // if middle part (mushroom ledge), wait until next row
    if (y == 0xc0)
        goto DrawThisRow; // if question block w/ coin, overwrite
    if (y >= 0xc0)
        goto WaitOneRow; // if any other metatile with palette 3, wait until next row
    if (y != 0x54)
        goto DrawThisRow; // if cracked rock terrain, overwrite
    if (a == 0x50)
        goto WaitOneRow; // if stem top of mushroom, wait until next row
    goto DrawThisRow;

DrawThisRow: // render contents of A from routine that called this
    writeData(MetatileBuffer + x, a);
    goto WaitOneRow;

WaitOneRow:
    ++x;
    if (x < 0x0d)
    {
        y = M(AreaObjectHeight); // decrement, and stop rendering if there is no more length
        --y;
        if ((y & 0x80) == 0)
            goto RenderUnderPart;
    } // ExitUPartR
    return;
}

//------------------------------------------------------------------------

void SMBEngine::AreaStyleObject()
{
    bool lrgObjJustStarted = false;

    // load level object style and jump to the right sub
    switch (M(AreaStyle))
    {
    case 0:
        goto TreeLedge; // also used for cloud type levels
    case 1:
        goto MushroomLedge;
    case 2:
        BulletBillCannon();
        return;
    default:
        bad_jump();
        return;
    }

TreeLedge:
    GetLrgObjAttrib(); // get row and length of green ledge
    a = M(AreaObjectLength + x); // check length counter for expiration
    if (a != 0)
    {
        if ((a & 0x80) == 0)
            goto MidTreeL;
        a = y;
        writeData(AreaObjectLength + x, a); // store lower nybble into buffer flag as length of ledge
        a = M(CurrentPageLoc) | M(CurrentColumnPos); // are we at the start of the level?
        if (a == 0)
            goto MidTreeL;
        a = 0x16; // render start of tree ledge
        goto NoUnder;

MidTreeL:
        x = M(0x07);
        // render middle of tree ledge
        writeData(MetatileBuffer + x, 0x17); // note that this is also used if ledge position is
        a = 0x4c; // at the start of level for continuous effect
        goto AllUnder; // now render the part underneath
    } // EndTreeL: render end of tree ledge
    a = 0x18;
    goto NoUnder;

MushroomLedge:
    lrgObjJustStarted = ChkLrgObjLength(); // get shroom dimensions
    writeData(0x06, y); // store length here for now
    if (lrgObjJustStarted)
    {
        // divide length by 2 and store elsewhere
        a = M(AreaObjectLength + x) >> 1;
        writeData(MushroomLedgeHalfLen + x, a);
        a = 0x19; // render start of mushroom
        goto NoUnder;
    } // EndMushL: if at the end, render end of mushroom
    a = 0x1b;
    y = M(AreaObjectLength + x);
    if (y == 0)
        goto NoUnder;
    // get divided length and store where length
    writeData(0x06, M(MushroomLedgeHalfLen + x)); // was stored originally
    x = M(0x07);
    a = 0x1a;
    writeData(MetatileBuffer + x, 0x1a); // render middle of mushroom
    if (y != M(0x06)) return; // are we smack dab in the center? if not, branch to leave
    ++x;
    writeData(MetatileBuffer + x, 0x4f); // render stem top of mushroom underneath the middle
    a = 0x50;
    goto AllUnder;

AllUnder:
    ++x;
    y = 0x0f; // set $0f to render all way down
    RenderUnderPart(); // now render the stem of mushroom
    return;

NoUnder: // load row of ledge
    x = M(0x07);
    y = 0x00; // set 0 for no bottom on this part
    RenderUnderPart();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PulleyRopeObject()
{
    bool lrgObjJustStarted = false;

    lrgObjJustStarted = ChkLrgObjLength(); // get length of pulley/rope object
    y = 0x00; // initialize metatile offset
    if (lrgObjJustStarted)
        goto RenderPul; // if starting, render left pulley
    y = 0x01;
    // if not at the end, render rope
    if (M(AreaObjectLength + x) != 0)
        goto RenderPul;
    y = 0x02; // otherwise render right pulley
    goto RenderPul;

RenderPul:
    a = M(PulleyRopeMetatiles + y);
    writeData(MetatileBuffer, a); // render at the top of the screen
    return;
}

//------------------------------------------------------------------------

void SMBEngine::VerticalPipe()
{
    uint32_t wide = 0;
    bool allEnemySlotsFull = false;

    GetPipeHeight();
    // check to see if value was nullified earlier
    if (M(0x00) != 0)
    { // (if d3, the usage control bit of second byte, was set)
        ++y;
        ++y;
        ++y;
        ++y; // add four if usage control bit was not set
    } // WarpPipe: save value in stack
    a = y;
    pha();
    a = M(AreaNumber) | M(WorldNumber); // if at world 1-1, do not add piranha plant ever
    if (a == 0)
        goto DrawPipe;
    y = M(AreaObjectLength + x); // if on second column of pipe, branch
    if (y == 0)
        goto DrawPipe; // (because we only need to do this once)
    allEnemySlotsFull = FindEmptyEnemySlot(); // check for an empty moving data buffer space
    if (allEnemySlotsFull)
        goto DrawPipe; // if not found, too many enemies, thus skip
    GetAreaObjXPosition(); // get horizontal pixel coordinate
    wide = ((M(CurrentPageLoc) << 8) | a) + 0x08; // add eight to put the piranha plant in the center
    writeData(Enemy_X_Position + x, LOBYTE(wide)); // store as enemy's horizontal coordinate
    writeData(Enemy_PageLoc + x, HIBYTE(wide)); // store as enemy's page coordinate
    a = HIBYTE(wide); // add carry to current page number
    a = 0x01;
    writeData(Enemy_Y_HighPos + x, 0x01);
    writeData(Enemy_Flag + x, 0x01); // activate enemy flag
    GetAreaObjYPosition(); // get piranha plant's vertical coordinate and store here
    writeData(Enemy_Y_Position + x, a);
    a = PiranhaPlant; // write piranha plant's value into buffer
    writeData(Enemy_ID + x, PiranhaPlant);
    InitPiranhaPlant();

DrawPipe: // get value saved earlier and use as Y
    pla();
    y = a;
    x = M(0x07); // get buffer offset
    // draw the appropriate pipe with the Y we loaded earlier
    writeData(MetatileBuffer + x, M(VerticalPipeData + y)); // render the top of the pipe
    ++x;
    a = M(VerticalPipeData + 2 + y); // render the rest of the pipe
    y = M(0x06); // subtract one from length and render the part underneath
    --y;
    RenderUnderPart();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::InitJumpGPTroopa()
{
    // set for movement to the left
    writeData(Enemy_MovingDir + x, 0x02);
    a = 0xf8; // set horizontal speed
    writeData(Enemy_X_Speed + x, 0xf8);
    TallBBox2();
    return;
}

//------------------------------------------------------------------------

// set specific value for bounding box control
void SMBEngine::TallBBox2()
{
    a = 0x03;
    SetBBox2();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::InitGoomba()
{
    InitNormalEnemy(); // set appropriate horizontal speed
    SmallBBox(); // set $09 as bounding box control, set other values
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitPodoboo()
{
    // set enemy position to below
    writeData(Enemy_Y_HighPos + x, 0x02); // the bottom of the screen
    writeData(Enemy_Y_Position + x, 0x02);
    writeData(EnemyIntervalTimer + x, 0x01); // set timer for enemy
    a = 0x00;
    writeData(Enemy_State + x, 0x00); // initialize enemy state, then jump to use
    SmallBBox(); // $09 as bounding box size and set other things
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitNormalEnemy()
{
    y = 0x01; // load offset of 1 by default
    // check for primary hard mode flag set
    if (M(PrimaryHardMode) == 0)
    {
        y = 0x00; // if not set, decrement offset
    } // GetESpd: get appropriate horizontal speed
    a = M(NormalXSpdData + y);
    SetESpd();
    return;
}

//------------------------------------------------------------------------

// store as speed for enemy object
void SMBEngine::SetESpd()
{
    writeData(Enemy_X_Speed + x, a);
    TallBBox(); // branch to set bounding box control and other data
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitRedKoopa()
{
    InitNormalEnemy(); // load appropriate horizontal speed
    a = 0x01; // set enemy state for red koopa troopa $03
    writeData(Enemy_State + x, 0x01);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitHammerBro()
{
    // init horizontal speed and timer used by hammer bro
    writeData(HammerThrowingTimer + x, 0x00); // apparently to time hammer throwing
    writeData(Enemy_X_Speed + x, 0x00);
    y = M(SecondaryHardMode); // get secondary hard mode flag
    writeData(EnemyIntervalTimer + x, M(HBroWalkingTimerData + y)); // set value as delay for hammer bro to walk left
    a = 0x0b; // set specific value for bounding box size control
    SetBBox();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitHorizFlySwimEnemy()
{
    a = 0x00; // initialize horizontal speed
    SetESpd();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitBloober()
{
    a = 0x00; // initialize horizontal speed
    writeData(BlooperMoveSpeed + x, 0x00);
    SmallBBox();
    return;
}

//------------------------------------------------------------------------

// set specific bounding box size control
void SMBEngine::SmallBBox()
{
    a = 0x09;
    SetBBox(); // unconditional branch
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitRedPTroopa()
{
    y = 0x30; // load central position adder for 48 pixels down
    a = M(Enemy_Y_Position + x); // set vertical coordinate into location to
    writeData(RedPTroopaOrigXPos + x, a); // be used as original vertical coordinate
    if ((a & 0x80) != 0)
    { // if vertical coordinate < $80
        y = 0xe0; // if => $80, load position adder for 32 pixels up
    } // GetCent: send central position adder to A
    a = y;
    a += M(Enemy_Y_Position + x); // add to current vertical coordinate
    writeData(RedPTroopaCenterYPos + x, a); // store as central vertical coordinate
    TallBBox();
    return;
}

//------------------------------------------------------------------------

// set specific bounding box size control
void SMBEngine::TallBBox()
{
    a = 0x03;
    SetBBox();
    return;
}

//------------------------------------------------------------------------

// set bounding box control here
void SMBEngine::SetBBox()
{
    writeData(Enemy_BoundBoxCtrl + x, a);
    a = 0x02; // set moving direction for left
    writeData(Enemy_MovingDir + x, 0x02);
    InitVStf();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::InitCheepCheep()
{
    SmallBBox(); // set vertical bounding box, speed, init others
    // check one portion of LSFR
    a = M(PseudoRandomBitReg + x) & 0b00010000; // get d4 from it
    writeData(CheepCheepMoveMFlag + x, a); // save as movement flag of some sort
    a = M(Enemy_Y_Position + x);
    writeData(CheepCheepOrigYPos + x, a); // save original vertical coordinate here
    return;
}

//------------------------------------------------------------------------

    //------------------------------------------------------------------------

void SMBEngine::EnemyLanding()
{
        InitVStf(); // do something here to vertical speed and something else
        a = M(Enemy_Y_Position + x) & 0b11110000; // save high nybble of vertical coordinate, and
        a |= 0b00001000; // set d3, then store, probably used to set enemy object
        writeData(Enemy_Y_Position + x, a); // neatly on whatever it's landing on
        return;
}

//------------------------------------------------------------------------

void SMBEngine::InitHoriPlatform()
{
    a = 0x00;
    writeData(XMoveSecondaryCounter + x, 0x00); // init one of the moving counters
    CommonPlatCode(); // jump ahead to execute more code
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitVertPlatform()
{
    y = 0x40; // set default value here
    a = M(Enemy_Y_Position + x); // check vertical position
    if ((a & 0x80) != 0)
    { // if above a certain point, skip this part
        a ^= 0xff;
        a += 0x01;
        y = 0xc0; // get alternate value to add to vertical position
    } // SetYO: save as top vertical position
    writeData(YPlatformTopYPos + x, a);
    a = y;
    a += M(Enemy_Y_Position + x); // to vertical position
    writeData(YPlatformCenterYPos + x, a); // save result as central vertical position
    CommonPlatCode();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::CommonPlatCode()
{
    InitVStf(); // do a sub to init certain other values
    SPBBox();
    return;
}

//------------------------------------------------------------------------

// set default bounding box size control
void SMBEngine::SPBBox()
{
    a = 0x05;
    y = M(AreaType);
    if (y == 0x03)
        goto CasPBB; // use default value if found
    y = M(SecondaryHardMode); // otherwise check for secondary hard mode flag
    if (y != 0)
        goto CasPBB; // if set, use default value
    a = 0x06; // use alternate value if not castle or secondary not set

CasPBB: // set bounding box size control here and leave
    writeData(Enemy_BoundBoxCtrl + x, a);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PlatLiftUp()
{
    // set movement amount here
    writeData(Enemy_Y_MoveForce + x, 0x10);
    a = 0xff; // set moving speed for platforms going up
    writeData(Enemy_Y_Speed + x, 0xff);
    CommonSmallLift(); // skip ahead to part we should be executing
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PlatLiftDown()
{
    // set movement amount here
    writeData(Enemy_Y_MoveForce + x, 0xf0);
    a = 0x00; // set moving speed for platforms going down
    writeData(Enemy_Y_Speed + x, 0x00);
    CommonSmallLift();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::CommonSmallLift()
{
    y = 0x01;
    PosPlatform(); // do a sub to add 12 pixels due to preset value
    a = 0x04;
    writeData(Enemy_BoundBoxCtrl + x, 0x04); // set bounding box control for small platforms
    return;
}

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
    y = M(BackgroundColorCtrl); // check background color control
    if (y != 0)
    { // if not set, increment task and fetch palette
        // put appropriate palette into vram
        writeData(VRAM_Buffer_AddrCtrl, M(BGColorCtrl_Addr - 4 + y)); // note that if set to 5-7, $0301 will not be read
    } // NoBGColor: increment to next subtask and plod on through
    ++M(ScreenRoutineTask);

    GetPlayerColors();
    return;
}



//------------------------------------------------------------------------

void SMBEngine::WriteTopStatusLine()
{
    a = 0x00; // select main status bar
    WriteGameText(); // output it
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

void SMBEngine::WriteGameText()
{
    pha(); // save text number to stack
    a <<= 1;
    y = a; // multiply by 2 and use as offset
    if (y < 0x04)
        goto LdGameText; // branch to use current offset as-is
    if (y >= 0x08)
    { // branch to check players
        y = 0x08; // otherwise warp zone, therefore set offset
    } // Chk2Players: check for number of players
    if (M(NumberOfPlayers) != 0)
        goto LdGameText; // if there are two, use current offset to also print name
    ++y; // otherwise increment offset by one to not print name

LdGameText: // get offset to message we want to print
    x = M(GameTextOffsets + y);
    y = 0x00;

    GameTextLoop();
    return;
}

//------------------------------------------------------------------------

// load message data
void SMBEngine::GameTextLoop()
{
GameTextLoop:
    bool shiftedBit = false;

    a = M(GameText + x);
    if (a != 0xff)
    { // branch to end text if found
        writeData(VRAM_Buffer1 + y, a); // otherwise write data to buffer
        ++x; // and increment increment
        ++y;
        if (y != 0)
            goto GameTextLoop; // do this for 256 bytes if no terminator found
    } // EndGameText: put null terminator at end
    writeData(VRAM_Buffer1 + y, 0x00);
    pla(); // pull original text number from stack
    x = a;
    if (a < 0x04)
    {
        --x; // are we printing the world/lives display?
        if (x == 0)
        { // if not, branch to check player's name
            a = M(NumberofLives); // otherwise, check number of lives
            a += 0x01;
            if (a >= 10)
            {
                a -= 10; // if so, subtract 10 and put a crown tile
                y = 0x9f; // next to the difference...strange things happen if
                writeData(VRAM_Buffer1 + 7, 0x9f); // the number of lives exceeds 19
            } // PutLives
            writeData(VRAM_Buffer1 + 8, a);
            y = M(WorldNumber); // write world and level numbers (incremented for display)
            ++y; // to the buffer in the spaces surrounding the dash
            writeData(VRAM_Buffer1 + 19, y);
            y = M(LevelNumber);
            ++y;
            writeData(VRAM_Buffer1 + 21, y); // we're done here
            return;

        //------------------------------------------------------------------------
        } // CheckPlayerName
        a = M(NumberOfPlayers); // check number of players
        if (a == 0)
            return; // if only 1 player, leave
        a = M(CurrentPlayer); // load current player
        --x; // check to see if current message number is for time up
        if (x != 0)
            goto ChkLuigi;
        y = M(OperMode); // check for game over mode
        if (y == GameOverModeValue)
            goto ChkLuigi;
        a ^= 0b00000001; // if not, must be time up, invert d0 to do other player

ChkLuigi:
        shiftedBit = (a & 0x01) != 0;
        a >>= 1;
        if (!shiftedBit)
            return; // if mario is current player, do not change the name
        y = 0x04;

        do // NameLoop: otherwise, replace "MARIO" with "LUIGI"
        {
            a = M(LuigiName + y);
            writeData(VRAM_Buffer1 + 3 + y, a);
            --y;
        } while ((y & 0x80) == 0); // do this until each letter is replaced

        return; // ExitChkName

    //------------------------------------------------------------------------
    } // PrintWarpZoneNumbers
    a -= 0x04; // subtract 4 and then shift to the left
    a <<= 1; // twice to get proper warp zone number
    a <<= 1; // offset
    x = a;
    y = 0x00;

    do // WarpNumLoop: print warp zone numbers into the
    {
        writeData(VRAM_Buffer1 + 27 + y, M(WarpZoneNumbers + x)); // placeholders from earlier
        ++x;
        ++y; // put a number in every fourth space
        ++y;
        ++y;
        ++y;
    } while (y < 0x0c);
    a = 0x2c; // load new buffer pointer at end of message
    SetVRAMOffset();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RenderAttributeTables()
{
    bool shiftedBit = false;

    // get low byte of next name table address
    a = M(CurrentNTAddr_Low) & 0b00011111; // to be written to, mask out all but 5 LSB,
    a -= 0x04;
    a &= 0b00011111; // mask out bits again and store
    writeData(0x01, a);
    a = M(CurrentNTAddr_High); // get high byte and branch if the subtraction above borrowed
    if ((M(CurrentNTAddr_Low) & 0b00011111) < 0x04)
    {
        a ^= 0b00000100; // otherwise invert d2
    } // SetATHigh: mask out all other bits
    a &= 0b00000100;
    a |= 0x23; // add $2300 to the high byte and store
    writeData(0x00, a);
    a = M(0x01); // get low byte - 4, divide by 4, add offset for
    shiftedBit = (a & 0x02) != 0; // the second shift carries d1 out
    a >>= 1; // attribute table and store
    a >>= 1;
    a = (uint8_t)(a + 0xc0 + (shiftedBit ? 1 : 0)); // we should now have the appropriate block of
    writeData(0x01, a); // attribute table in our temp address
    x = 0x00;
    y = M(VRAM_Buffer2_Offset); // get buffer offset

    do // AttribLoop
    {
        writeData(VRAM_Buffer2 + y, M(0x00)); // store high byte of attribute table address
        a = M(0x01);
        a += 0x08; // below the status bar, and store
        writeData(VRAM_Buffer2 + 1 + y, a);
        writeData(0x01, a); // also store in temp again
        // fetch current attribute table byte and store
        writeData(VRAM_Buffer2 + 3 + y, M(AttributeBuffer + x)); // in the buffer
        writeData(VRAM_Buffer2 + 2 + y, 0x01); // store length of 1 in buffer
        a = 0x00;
        writeData(AttributeBuffer + x, 0x00); // clear current byte in attribute buffer
        ++y; // increment buffer offset by 4 bytes
        ++y;
        ++y;
        ++y;
        ++x; // increment attribute offset and check to see
    } while (x < 0x07);
    writeData(VRAM_Buffer2 + y, a); // put null terminator at the end
    writeData(VRAM_Buffer2_Offset, y); // store offset in case we want to do any more

    SetVRAMCtrl();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::SetVRAMCtrl()
{
    a = 0x06;
    writeData(VRAM_Buffer_AddrCtrl, 0x06); // set buffer to $0341 and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RemoveCoin_Axe()
{
    y = 0x41; // set low byte so offset points to $0341
    a = 0x03; // load offset for default blank metatile
    x = M(AreaType); // check area type
    if (x == 0)
    { // if not water type, use offset
        a = 0x04; // otherwise load offset for blank metatile used in water
    } // WriteBlankMT: do a sub to write blank metatile to vram buffer
    PutBlockMetatile();
    a = 0x06;
    writeData(VRAM_Buffer_AddrCtrl, 0x06); // set vram address controller to $0341 and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ReplaceBlockMetatile()
{
    WriteBlockMetatile(); // write metatile to vram buffer to replace block object
    ++M(Block_ResidualCounter); // increment unused counter (residual code)
    --M(Block_RepFlag + x); // decrement flag (residual code)
    return; // leave
}

//------------------------------------------------------------------------

void SMBEngine::DestroyBlockMetatile()
{
    a = 0x00; // force blank metatile if branched/jumped to this point

    WriteBlockMetatile();
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

void SMBEngine::IncrementColumnPos()
{
    ++M(CurrentColumnPos); // increment column where we're at
    a = M(CurrentColumnPos) & 0b00001111; // mask out higher nybble
    if (a == 0)
    {
        writeData(CurrentColumnPos, a); // if no bits left set, wrap back to zero (0-f)
        ++M(CurrentPageLoc); // and increment page number where we're at
    } // NoColWrap: increment column offset where we're at
    ++M(BlockBufferColumnPos);
    a = M(BlockBufferColumnPos) & 0b00011111; // mask out all but 5 LSB (0-1f)
    writeData(BlockBufferColumnPos, a); // and save
    return;
}

//------------------------------------------------------------------------

void SMBEngine::AlterAreaAttributes()
{
    y = M(AreaObjOffsetBuffer + x); // load offset for level object data saved in buffer
    ++y; // load second byte
    a = M(W(AreaData) + y);
    pha(); // save in stack for now
    a &= 0b01000000;
    if (a == 0)
    { // branch if d6 is set
        pla();
        pha(); // pull and push offset to copy to A
        a &= 0b00001111; // mask out high nybble and store as
        writeData(TerrainControl, a); // new terrain height type bits
        pla();
        a &= 0b00110000; // pull and mask out all but d5 and d4
        a >>= 1; // move bits to lower nybble and store
        a >>= 1; // as new background scenery bits
        a >>= 1;
        a >>= 1;
        writeData(BackgroundScenery, a); // then leave
        return;

    //------------------------------------------------------------------------
    } // Alter2
    pla();
    a &= 0b00000111; // mask out all but 3 LSB
    if (a >= 0x04)
    { // and nullify foreground scenery bits
        writeData(BackgroundColorCtrl, a);
        a = 0x00;
    } // SetFore: otherwise set new foreground scenery bits
    writeData(ForegroundScenery, a);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ScrollLockObject_Warp()
{
    x = 0x04; // load value of 4 for game text routine as default
    // warp zone (4-3-2), then check world number
    if (M(WorldNumber) == 0)
        goto WarpNum;
    x = 0x05; // if world number > 1, increment for next warp zone (5)
    y = M(AreaType); // check area type
    --y;
    if (y != 0)
        goto WarpNum; // if ground area type, increment for last warp zone
    x = 0x06; // (8-7-6) and move on

WarpNum:
    a = x;
    writeData(WarpZoneControl, a); // store number here to be used by warp zone routine
    WriteGameText(); // print text and warp zone numbers
    a = PiranhaPlant;
    KillEnemies(); // load identifier for piranha plants and do sub

    ScrollLockObject();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ScrollLockObject()
{
    // invert scroll lock to turn it on
    a = M(ScrollLock) ^ 0b00000001;
    writeData(ScrollLock, a);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::IntroPipe()
{
    bool lrgObjJustStarted = false;
    bool sidePipeShaftDrawn = false;

    y = 0x03; // check if length set, if not set, set it
    lrgObjJustStarted = ChkLrgObjFixedLength();
    y = 0x0a; // set fixed value and render the sideways part
    sidePipeShaftDrawn = RenderSidewaysPipe();
    if (sidePipeShaftDrawn)
    { // if the shaft was not drawn, not time to draw vertical pipe part
        x = 0x06; // blank everything above the vertical pipe part

        do // VPipeSectLoop: all the way to the top of the screen
        {
            a = 0x00;
            writeData(MetatileBuffer + x, 0x00); // because otherwise it will look like exit pipe
            --x;
        } while ((x & 0x80) == 0);
        a = M(VerticalPipeData + y); // draw the end of the vertical pipe part
        writeData(MetatileBuffer + 7, a);
    } // NoBlankP
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ExitPipe()
{
    bool lrgObjJustStarted = false;
    bool sidePipeShaftDrawn = false;

    y = 0x03; // check if length set, if not set, set it
    lrgObjJustStarted = ChkLrgObjFixedLength();
    GetLrgObjAttrib(); // get vertical length, then plow on through RenderSidewaysPipe

    sidePipeShaftDrawn = RenderSidewaysPipe();
    return;
}

//------------------------------------------------------------------------

bool SMBEngine::RenderSidewaysPipe()
{
    bool sidePipeShaftDrawn = false;

    --y; // decrement twice to make room for shaft at bottom
    --y; // and store here for now as vertical length
    writeData(0x05, y);
    y = M(AreaObjectLength + x); // get length left over and store here
    writeData(0x06, y);
    x = M(0x05); // get vertical length plus one, use as buffer offset
    ++x;
    a = M(SidePipeShaftData + y); // check for value $00 based on horizontal offset
    if (a != 0x00)
    { // if found, do not draw the vertical pipe shaft
        x = 0x00;
        y = M(0x05); // init buffer offset and get vertical length
        RenderUnderPart(); // and render vertical shaft using tile number in A
        sidePipeShaftDrawn = true; // used by IntroPipe
    }
    else
    {
        sidePipeShaftDrawn = false;
    } // DrawSidePart: render side pipe part at the bottom
    y = M(0x06);
    writeData(MetatileBuffer + x, M(SidePipeTopPart + y)); // note that the pipe parts are stored
    a = M(SidePipeBottomPart + y); // backwards horizontally
    writeData(MetatileBuffer + 1 + x, a);
    return sidePipeShaftDrawn;
}

//------------------------------------------------------------------------

void SMBEngine::QuestionBlockRow_High()
{
    a = 0x03; // start on the fourth row
    Skip_1();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::QuestionBlockRow_Low()
{
    a = 0x07; // start on the eighth row
    Skip_1();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Skip_1()
{
    bool lrgObjJustStarted = false;

    pha(); // save whatever row to the stack for now
    lrgObjJustStarted = ChkLrgObjLength(); // get low nybble and save as length
    pla();
    x = a; // render question boxes with coins
    a = 0xc0;
    writeData(MetatileBuffer + x, 0xc0);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Bridge_High()
{
    a = 0x06; // start on the seventh row from top of screen
    Skip_2();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Bridge_Middle()
{
    a = 0x07; // start on the eighth row
    Skip_2();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Skip_2()
{
    Skip_3();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Bridge_Low()
{
    a = 0x09; // start on the tenth row
    Skip_3();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Skip_3()
{
    bool lrgObjJustStarted = false;

    pha(); // save whatever row to the stack for now
    lrgObjJustStarted = ChkLrgObjLength(); // get low nybble and save as length
    pla();
    x = a; // render bridge railing
    writeData(MetatileBuffer + x, 0x0b);
    ++x;
    y = 0x00; // now render the bridge itself
    a = 0x63;
    RenderUnderPart();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::EndlessRope()
{
    x = 0x00; // render rope from the top to the bottom of screen
    y = 0x0f;
    DrawRope();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::BalancePlatRope()
{
    a = x; // save object buffer offset for now
    pha();
    x = 0x01; // blank out all from second row to the bottom
    y = 0x0f; // with blank used for balance platform rope
    a = 0x44;
    RenderUnderPart();
    pla(); // get back object buffer offset
    x = a;
    GetLrgObjAttrib(); // get vertical length from lower nybble
    x = 0x01;

    DrawRope();
    return;
}

//------------------------------------------------------------------------

// render the actual rope
void SMBEngine::DrawRope()
{
    a = 0x40;
    RenderUnderPart();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::CastleBridgeObj()
{
    bool lrgObjJustStarted = false;

    y = 0x0c; // load length of 13 columns
    lrgObjJustStarted = ChkLrgObjFixedLength();
    ChainObj();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::AxeObj()
{
    a = 0x08; // load bowser's palette into sprite portion of palette
    writeData(VRAM_Buffer_AddrCtrl, 0x08);

    ChainObj();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ChainObj()
{
    y = M(0x00); // get value loaded earlier from decoder
    x = M(C_ObjectRow - 2 + y); // get appropriate row and metatile for object
    a = M(C_ObjectMetatile - 2 + y);
    ColObj();
    return;
}

//------------------------------------------------------------------------

// get appropriate metatile for brick (question block
void SMBEngine::DrawQBlk()
{
        a = M(BrickQBlockMetatiles + y);
        pha(); // if branched to here from question block routine)
        GetLrgObjAttrib(); // get row from location byte
        DrawRow(); // now render the object
        return;
}

//------------------------------------------------------------------------

void SMBEngine::LoadAreaPointer()
{
    FindAreaPointer(); // find it and store it here
    writeData(AreaPointer, a);

    GetAreaType();
    return;
}

//------------------------------------------------------------------------

// mask out all but d6 and d5
void SMBEngine::GetAreaType()
{
    a &= 0b01100000;
    a >>= 5; // make %0xx00000 into %000000xx
    writeData(AreaType, a); // save 2 MSB as area type
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetAreaDataAddrs()
{
    uint32_t wide = 0;

    a = M(AreaPointer); // use 2 MSB for Y
    GetAreaType();
    y = a;
    // mask out all but 5 LSB
    a = M(AreaPointer) & 0b00011111;
    writeData(AreaAddrsLOffset, a); // save as low offset
    a = M(EnemyAddrHOffsets + y); // load base value with 2 altered MSB,
    a += M(AreaAddrsLOffset); // becomes offset for level data
    y = a;
    // use offset to load pointer
    writeData(EnemyDataLow, M(EnemyDataAddrLow + y));
    writeData(EnemyDataHigh, M(EnemyDataAddrHigh + y));
    y = M(AreaType); // use area type as offset
    a = M(AreaDataHOffsets + y); // do the same thing but with different base value
    a += M(AreaAddrsLOffset);
    y = a;
    // use this offset to load another pointer
    writeData(AreaDataLow, M(AreaDataAddrLow + y));
    writeData(AreaDataHigh, M(AreaDataAddrHigh + y));
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

void SMBEngine::CyclePlayerPalette()
{
            a &= 0x03; // mask out all but d1-d0 (previously d3-d2)
            writeData(0x00, a); // store result here to use as palette bits
            // get player attributes
            a = M(Player_SprAttrib) & 0b11111100; // save any other bits but palette bits
            a |= M(0x00); // add palette bits
            writeData(Player_SprAttrib, a); // store as new player attributes
            return; // and leave
}

//------------------------------------------------------------------------

void SMBEngine::ResetPalStar()
{
        // get player attributes
        a = M(Player_SprAttrib) & 0b11111100; // mask out palette bits to force palette 0
        writeData(Player_SprAttrib, a); // store as new player attributes
        return; // and leave
}

//------------------------------------------------------------------------

void SMBEngine::CoinBlock()
{
    bool miscSlotSearched = false;

    miscSlotSearched = FindEmptyMiscSlot(); // set offset for empty or last misc object buffer slot
    // get page location of block object
    writeData(Misc_PageLoc + y, M(Block_PageLoc + x)); // store as page location of misc object
    // get horizontal coordinate of block object
    a = M(Block_X_Position + x) | 0x05; // add 5 pixels
    writeData(Misc_X_Position + y, a); // store as horizontal coordinate of misc object
    // get vertical coordinate of block object
    // the jump engine reaches CoinBlock with the carry clear, so the slot search
    // above only leaves it set if it got as far as its compare
    a = (uint8_t)(M(Block_Y_Position + x) - 0x10 - (miscSlotSearched ? 0 : 1)); // subtract 16 pixels
    writeData(Misc_Y_Position + y, a); // store as vertical coordinate of misc object
    JCoinC(); // jump to rest of code as applies to this misc object
    return;
}

//------------------------------------------------------------------------

void SMBEngine::SetupJumpCoin()
{
    bool shiftedBit = false;
    bool miscSlotSearched = false;

    miscSlotSearched = FindEmptyMiscSlot(); // set offset for empty or last misc object buffer slot
    // get page location saved earlier
    writeData(Misc_PageLoc + y, M(Block_PageLoc2 + x)); // and save as page location for misc object
    a = M(0x06); // get low byte of block buffer offset
    shiftedBit = (a & 0x10) != 0; // the fourth shift below carries d4 out
    a <<= 1;
    a <<= 1; // multiply by 16 to use lower nybble
    a <<= 1;
    a <<= 1;
    a |= 0x05; // add five pixels
    writeData(Misc_X_Position + y, a); // save as horizontal coordinate for misc object
    // get vertical high nybble offset from earlier
    a = (uint8_t)(M(0x02) + 0x20 + (shiftedBit ? 1 : 0)); // add 32 pixels for the status bar, plus the bit shifted out above
    writeData(Misc_Y_Position + y, a); // store as vertical coordinate

    JCoinC();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::JCoinC()
{
    writeData(Misc_Y_Speed + y, 0xfb); // set vertical speed
    a = 0x01;
    writeData(Misc_Y_HighPos + y, 0x01); // set vertical high byte
    writeData(Misc_State + y, 0x01); // set state for misc object
    writeData(Square2SoundQueue, 0x01); // load coin grab sound
    writeData(ObjectOffset, x); // store current control bit as misc object offset
    GiveOneCoin(); // update coin tally on the screen and coin amount variable
    ++M(CoinTallyFor1Ups); // increment coin tally used to activate 1-up block flag
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GiveOneCoin()
{
    a = 0x01; // set digit modifier to add 1 coin
    writeData(DigitModifier + 5, 0x01); // to the current player's coin tally
    x = M(CurrentPlayer); // get current player on the screen
    y = M(CoinTallyOffsets + x); // get offset for player's coin tally
    DigitsMathRoutine(); // update the coin tally
    ++M(CoinTally); // increment onscreen player's coin amount
    if (M(CoinTally) == 100)
    { // if not, skip all of this
        writeData(CoinTally, 0x00); // otherwise, reinitialize coin amount
        ++M(NumberofLives); // give the player an extra life
        a = Sfx_ExtraLife;
        writeData(Square2SoundQueue, Sfx_ExtraLife); // play 1-up sound
    } // CoinPoints
    a = 0x02; // set digit modifier to award
    writeData(DigitModifier + 4, 0x02); // 200 points to the player

    AddToScore();
    return;
}




//------------------------------------------------------------------------

void SMBEngine::SetupPowerUp()
{
    // load power-up identifier into
    writeData(Enemy_ID + 5, PowerUpObject); // special use slot of enemy object buffer
    // store page location of block object
    writeData(Enemy_PageLoc + 5, M(Block_PageLoc + x)); // as page location of power-up object
    // store horizontal coordinate of block object
    writeData(Enemy_X_Position + 5, M(Block_X_Position + x)); // as horizontal coordinate of power-up object
    writeData(Enemy_Y_HighPos + 5, 0x01); // set vertical high byte of power-up object
    a = M(Block_Y_Position + x); // get vertical coordinate of block object
    a -= 0x08; // subtract 8 pixels
    writeData(Enemy_Y_Position + 5, a); // and use as vertical coordinate of power-up object

    PwrUpJmp();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::MushFlowerBlock()
{
        a = 0x00; // load mushroom/fire flower into power-up type
        Skip_4();
        return;
}

//------------------------------------------------------------------------

void SMBEngine::StarBlock()
{
        a = 0x02; // load star into power-up type
    Skip_4();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Skip_4()
{
        Skip_5();
        return;
}

//------------------------------------------------------------------------

void SMBEngine::ExtraLifeMushBlock()
{
        a = 0x03; // load 1-up mushroom into power-up type
    Skip_5();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Skip_5()
{
        writeData(0x39, a); // store correct power-up type
        SetupPowerUp();
        return;
}

//------------------------------------------------------------------------

void SMBEngine::BrickShatter()
{
    CheckTopOfBlock(); // check to see if there's a coin directly above this block
    a = Sfx_BrickShatter;
    writeData(Block_RepFlag + x, Sfx_BrickShatter); // set flag for block object to immediately replace metatile
    writeData(NoiseSoundQueue, Sfx_BrickShatter); // load brick shatter sound
    SpawnBrickChunks(); // create brick chunk objects
    writeData(Player_Y_Speed, 0xfe); // set vertical speed for player
    a = 0x05;
    writeData(DigitModifier + 5, 0x05); // set digit modifier to give player 50 points
    AddToScore(); // do sub to update the score
    x = M(SprDataOffset_Ctrl); // load control bit and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::CheckTopOfBlock()
{
    x = M(SprDataOffset_Ctrl); // load control bit
    y = M(0x02); // get vertical high nybble offset used in block buffer
    if (y == 0)
        return; // branch to leave if set to zero, because we're at the top
    a = y; // otherwise set to A
    a -= 0x10; // subtract $10 to move up one row in the block buffer
    writeData(0x02, a); // store as new vertical high nybble offset
    y = a;
    a = M(W(0x06) + y); // get contents of block buffer in same column, one row up
    if (a != 0xc2)
        return; // if not, branch to leave
    a = 0x00;
    writeData(W(0x06) + y, 0x00); // otherwise put blank metatile where coin was
    RemoveCoin_Axe(); // write blank metatile to vram buffer
    x = M(SprDataOffset_Ctrl); // get control bit
    SetupJumpCoin(); // create jumping coin object and update coin variables

    return; // TopEx: leave!
}

//------------------------------------------------------------------------

void SMBEngine::BlockObjMT_Updater()
{
    x = 0x01; // set offset to start with second block object

    do // UpdateLoop: set offset here
    {
        writeData(ObjectOffset, x);
        a = M(VRAM_Buffer1); // if vram buffer already being used here,
        if (a != 0)
            goto NextBUpd; // branch to move onto next block object
        a = M(Block_RepFlag + x); // if flag for block object already clear,
        if (a == 0)
            goto NextBUpd; // branch to move onto next block object
        // get low byte of block buffer
        writeData(0x06, M(Block_BBuf_Low + x)); // store into block buffer address
        writeData(0x07, 0x05); // set high byte of block buffer address
        a = M(Block_Orig_YPos + x); // get original vertical coordinate of block object
        writeData(0x02, a); // store here and use as offset to block buffer
        y = a;
        a = M(Block_Metatile + x); // get metatile to be written
        writeData(W(0x06) + y, a); // write it to the block buffer
        ReplaceBlockMetatile(); // do sub to replace metatile where block object is
        a = 0x00;
        writeData(Block_RepFlag + x, 0x00); // clear block object flag

NextBUpd: // decrement block object offset
        --x;
    } while ((x & 0x80) == 0); // do this until both block objects are dealt with
    return; // then leave
}

//------------------------------------------------------------------------

void SMBEngine::MoveRedPTroopaDown()
{
    y = 0x00; // set Y to move downwards
    MoveRedPTroopa(); // skip to movement routine
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveRedPTroopa()
{
    ++x; // increment X for enemy offset
    writeData(0x00, 0x03); // set downward movement amount here
    writeData(0x01, 0x06); // set upward movement amount here
    writeData(0x02, 0x02); // set maximum speed here
    a = y; // set movement direction in A, and
    RedPTroopaGrav(); // jump to move this thing
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

void SMBEngine::ImposeGravityBlock()
{
    y = 0x01; // set offset for maximum speed
    Skip_6();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Skip_6()
{
    // set movement amount here
    writeData(0x00, 0x50);
    a = M(MaxSpdBlockData + y); // get maximum speed

    ImposeGravitySprObj();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::MovePlatformDown()
{
    a = 0x00; // save value to stack (if branching here, execute next
    Skip_7();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MovePlatformUp()
{
    a = 0x01; // save value to stack
    Skip_7();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Skip_7()
{
    pha();
    y = M(Enemy_ID + x); // get enemy object identifier
    ++x; // increment offset for enemy object
    a = 0x05; // load default value here
    if (y == 0x29)
    { // this code, thus unconditional branch here
        a = 0x09; // residual code
    } // SetDplSpd: save downward movement amount here
    writeData(0x00, a);
    // save upward movement amount here
    writeData(0x01, 0x0a);
    // save maximum vertical speed here
    writeData(0x02, 0x03);
    pla(); // get value from stack
    y = a; // use as Y, then move onto code shared by red koopa

    RedPTroopaGrav();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RedPTroopaGrav()
{
    ImposeGravity(); // do a sub to move object gradually
    x = M(ObjectOffset); // get enemy object offset and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::SetupLakitu()
{
        a = 0x00; // erase counter for lakitu's reappearance
        writeData(LakituReappearTimer, 0x00);
        InitHorizFlySwimEnemy(); // set $03 as bounding box, set other attributes
        TallBBox2(); // set $03 as bounding box again (not necessary) and leave
        return;
}

//------------------------------------------------------------------------

void SMBEngine::InitShortFirebar()
{
    uint32_t wide = 0;

    // initialize low byte of spin state
    writeData(FirebarSpinState_Low + x, 0x00);
    a = M(Enemy_ID + x); // subtract $1b from enemy identifier
    a -= 0x1b;
    y = a;
    // get spinning speed of firebar
    writeData(FirebarSpinSpeed + x, M(FirebarSpinSpdData + y));
    // get spinning direction of firebar
    writeData(FirebarSpinDirection + x, M(FirebarSpinDirData + y));
    a = M(Enemy_Y_Position + x);
    a += 0x04;
    writeData(Enemy_Y_Position + x, a);
    wide = ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x)) + 0x04;
    writeData(Enemy_X_Position + x, LOBYTE(wide));
    writeData(Enemy_PageLoc + x, HIBYTE(wide));
    a = HIBYTE(wide);
    TallBBox2(); // set bounding box control (not used) and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitBalPlatform()
{
    --M(Enemy_Y_Position + x); // raise vertical position by two pixels
    --M(Enemy_Y_Position + x);
    // if secondary hard mode flag not set,
    if (M(SecondaryHardMode) == 0)
    { // branch ahead
        y = 0x02; // otherwise set value here
        PosPlatform(); // do a sub to add or subtract pixels
    } // AlignP: set default value here for now
    y = 0xff;
    a = M(BalPlatformAlignment); // get current balance platform alignment
    writeData(Enemy_State + x, a); // set platform alignment to object state here
    if ((a & 0x80) != 0)
    { // if old alignment $ff, put $ff as alignment for negative
        a = x; // if old contents already $ff, put
        y = a; // object offset as alignment to make next positive
    } // SetBPA: store whatever value's in Y here
    writeData(BalPlatformAlignment, y);
    a = 0x00;
    writeData(Enemy_MovingDir + x, 0x00); // init moving direction
    y = 0x00; // init Y
    PosPlatform(); // do a sub to add 8 pixels, then run shared code here

    InitDropPlatform();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitDropPlatform()
{
    a = 0xff;
    writeData(PlatformCollisionFlag + x, 0xff); // set some value here
    CommonPlatCode(); // then jump ahead to execute more code
    return;
}

//------------------------------------------------------------------------

void SMBEngine::LargeLiftUp()
{
    PlatLiftUp(); // execute code for platforms going up
    LargeLiftBBox(); // overwrite bounding box for large platforms
    return;
}

//------------------------------------------------------------------------

void SMBEngine::LargeLiftDown()
{
    PlatLiftDown(); // execute code for platforms going down

    LargeLiftBBox();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::LargeLiftBBox()
{
    SPBBox(); // jump to overwrite bounding box size control
    return;
}

//------------------------------------------------------------------------

void SMBEngine::EndAreaPoints()
{
    y = 0x0b; // load offset for mario's score by default
    // check player on the screen
    if (M(CurrentPlayer) != 0)
    { // if mario, do not change
        y = 0x11; // otherwise load offset for luigi's score
    } // ELPGive: award 50 points per game timer interval
    DigitsMathRoutine();
    a = M(CurrentPlayer); // get player on the screen (or 500 points per
    a <<= 1; // fireworks explosion if branched here from there)
    a <<= 1; // shift to high nybble
    a <<= 1;
    a <<= 1;
    a |= 0b00000100; // add four to set nybble for game timer
    UpdateNumber(); // jump to print the new score and game timer
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawStarFlag()
{
    RelativeEnemyPosition(); // get relative coordinates of star flag
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    x = 0x03; // do four sprites

    do // DSFLoop: get relative vertical coordinate
    {
        a = M(Enemy_Rel_YPos);
        a += M(StarFlagYPosAdder + x); // add Y coordinate adder data
        writeData(Sprite_Y_Position + y, a); // store as Y coordinate
        // get tile number
        writeData(Sprite_Tilenumber + y, M(StarFlagTileData + x)); // store as tile number
        // set palette and background priority bits
        writeData(Sprite_Attributes + y, 0x22); // store as attributes
        a = M(Enemy_Rel_XPos); // get relative horizontal coordinate
        a += M(StarFlagXPosAdder + x); // add X coordinate adder data
        writeData(Sprite_X_Position + y, a); // store as X coordinate
        ++y;
        ++y; // increment OAM data offset four bytes
        ++y; // for next sprite
        ++y;
        --x; // move onto next sprite
    } while ((x & 0x80) == 0); // do this until all sprites are done
    x = M(ObjectOffset); // get enemy object offset and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DoOtherPlatform()
{
            y = M(Enemy_State + x); // get offset of other platform
            pla(); // get old vertical coordinate from stack
            a -= M(Enemy_Y_Position + x); // get difference of old vs. new coordinate
            a += M(Enemy_Y_Position + y); // add difference to vertical coordinate of other
            writeData(Enemy_Y_Position + y, a); // platform to move it in the opposite direction
            a = M(PlatformCollisionFlag + x); // if no collision, skip this part here
            if ((a & 0x80) == 0)
            {
                x = a; // put offset which collision occurred here
                PositionPlayerOnVPlat(); // and use it to position player accordingly
            } // DrawEraseRope
            y = M(ObjectOffset); // get enemy object offset
            // check to see if current platform is
            a = M(Enemy_Y_Speed + y) | M(Enemy_Y_MoveForce + y); // moving at all
            if (a == 0)
                goto ExitRp; // if not, skip all of this and branch to leave
            // get vram buffer offset
            if (M(VRAM_Buffer1_Offset) >= 0x20)
                goto ExitRp; // and skip this, branch to leave
            a = M(Enemy_Y_Speed + y);
            pha(); // save two copies of vertical speed to stack
            pha();
            SetupPlatformRope(); // do a sub to figure out where to put new bg tiles
            // write name table address to vram buffer
            writeData(VRAM_Buffer1 + x, M(0x01)); // first the high byte, then the low
            writeData(VRAM_Buffer1 + 1 + x, M(0x00));
            // set length for 2 bytes
            writeData(VRAM_Buffer1 + 2 + x, 0x02);
            // if platform moving upwards, branch
            if ((M(Enemy_Y_Speed + y) & 0x80) == 0)
            { // to do something else
                writeData(VRAM_Buffer1 + 3 + x, 0xa2); // otherwise put tile numbers for left
                a = 0xa3; // and right sides of rope in vram buffer
                writeData(VRAM_Buffer1 + 4 + x, 0xa3);
            } // EraseR1: put blank tiles in vram buffer
            else // jump to skip this part
            {
                a = 0x24;
                writeData(VRAM_Buffer1 + 3 + x, 0x24); // to erase rope
                writeData(VRAM_Buffer1 + 4 + x, 0x24);
            } // OtherRope
            // get offset of other platform from state
            y = M(Enemy_State + y); // use as Y here
            pla(); // pull second copy of vertical speed from stack
            a ^= 0xff; // invert bits to reverse speed
            SetupPlatformRope(); // do sub again to figure out where to put bg tiles
            // write name table address to vram buffer
            writeData(VRAM_Buffer1 + 5 + x, M(0x01)); // this time we're doing putting tiles for
            // the other platform
            writeData(VRAM_Buffer1 + 6 + x, M(0x00));
            writeData(VRAM_Buffer1 + 7 + x, 0x02); // set length again for 2 bytes
            pla(); // pull first copy of vertical speed from stack
            if ((a & 0x80) != 0)
            { // if moving upwards (note inversion earlier), skip this
                writeData(VRAM_Buffer1 + 8 + x, 0xa2); // otherwise put tile numbers for left
                a = 0xa3; // and right sides of rope in vram
                writeData(VRAM_Buffer1 + 9 + x, 0xa3); // transfer buffer
            } // EraseR2: put blank tiles in vram buffer
            else // jump to skip this part
            {
                a = 0x24;
                writeData(VRAM_Buffer1 + 8 + x, 0x24); // to erase rope
                writeData(VRAM_Buffer1 + 9 + x, 0x24);
            } // EndRp: put null terminator at the end
            writeData(VRAM_Buffer1 + 10 + x, 0x00);
            a = M(VRAM_Buffer1_Offset); // add ten bytes to the vram buffer offset
            a += 10;
            writeData(VRAM_Buffer1_Offset, a);

ExitRp: // get enemy object buffer offset and leave
            x = M(ObjectOffset);
            return;
}

//------------------------------------------------------------------------

void SMBEngine::StopPlatforms()
{
        InitVStf(); // initialize vertical speed and low byte
        writeData(Enemy_Y_Speed + y, a); // for both platforms and leave
        writeData(Enemy_Y_MoveForce + y, a);
        return;
}

//------------------------------------------------------------------------

void SMBEngine::ChkYPCollision()
{
    a = M(PlatformCollisionFlag + x); // if collision flag not set here, branch
    if ((a & 0x80) == 0)
    { // to leave
        PositionPlayerOnVPlat(); // otherwise position player appropriately
    } // ExYPl: leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PositionPlayerOnS_Plat()
{
    y = a; // use bounding box counter saved in collision flag
    a = M(Enemy_Y_Position + x); // for offset
    a += M(PlayerPosSPlatData - 1 + y); // coordinate
    Skip_8();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PositionPlayerOnVPlat()
{
    a = M(Enemy_Y_Position + x); // get vertical coordinate
    Skip_8();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Skip_8()
{
    uint32_t wide = 0;

    y = M(GameEngineSubroutine);
    if (y == 0x0b)
        return; // skip all of this
    y = M(Enemy_Y_HighPos + x);
    if (y != 0x01)
        return;
    wide = ((y << 8) | a) - 0x20; // subtract 32 pixels from the vertical coordinate for the player object's height
    writeData(Player_Y_Position, LOBYTE(wide)); // save as player's new vertical coordinate
    writeData(Player_Y_HighPos, HIBYTE(wide)); // and store as player's new vertical high byte
    a = HIBYTE(wide);
    a = 0x00;
    writeData(Player_Y_Speed, 0x00); // initialize vertical speed and low byte of force
    writeData(Player_Y_MoveForce, 0x00); // and then leave

    return; // ExPlPos
}



//------------------------------------------------------------------------

// load vertical high nybble offset for block buffer
void SMBEngine::ErACM()
{
    y = M(0x02);
    a = 0x00; // load blank metatile
    writeData(W(0x06) + y, 0x00); // store to remove old contents from block buffer
    RemoveCoin_Axe(); // update the screen accordingly
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ChkUnderEnemy()
{
    a = 0x00; // set flag in A for save vertical coordinate
    y = 0x15; // set Y to check the bottom middle (8,18) of enemy object
    BlockBufferChk_Enemy(); // hop to it!
    return;
}


//------------------------------------------------------------------------

void SMBEngine::BlockBufferChk_Enemy()
{
    pha(); // save contents of A to stack
    a = x;
    a += 0x01;
    x = a;
    pla(); // pull A from stack and jump elsewhere
    BBChk_E();
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

void SMBEngine::BlockBufferChk_FBall()
{
    y = 0x1a; // set offset for block buffer adder data
    a = x;
    a += 0x07; // add seven bytes to use
    x = a;

    ResJmpM();
    return;
}

//------------------------------------------------------------------------

// set A to return vertical coordinate
void SMBEngine::ResJmpM()
{
    a = 0x00;

    BBChk_E();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::BlockBufferColli_Feet()
{
    ++y; // if branched here, increment to next set of adders

    BlockBufferColli_Head();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::BlockBufferColli_Head()
{
    a = 0x00; // set flag to return vertical coordinate
    Skip_9();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::BlockBufferColli_Side()
{
    a = 0x01; // set flag to return horizontal coordinate
    Skip_9();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Skip_9()
{
    x = 0x00; // set offset for player object

    BlockBufferCollision();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::DrawEnemyObjRow()
{
    // load two tiles of enemy graphics
    writeData(0x00, M(EnemyGraphicsTable + x));
    a = M(EnemyGraphicsTable + 1 + x);

    DrawOneSpriteRow();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::DrawFireball()
{
    y = M(FBall_SprDataOffset + x); // get fireball's sprite data offset
    // get relative vertical coordinate
    writeData(Sprite_Y_Position + y, M(Fireball_Rel_YPos)); // store as sprite Y coordinate
    // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, M(Fireball_Rel_XPos)); // store as sprite X coordinate, then do shared code

    DrawFirebar();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::DrawPlayer_Intermediate()
{
    x = 0x05; // store data into zero page memory

    do // PIntLoop: load data to display player as he always
    {
        writeData(0x02 + x, M(IntermediatePlayerData + x)); // appears on world/lives display
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

void SMBEngine::RenderPlayerSub()
{
    writeData(0x07, a); // store number of rows of sprites to draw
    a = M(Player_Rel_XPos);
    writeData(Player_Pos_ForScroll, a); // store player's relative horizontal position
    writeData(0x05, a); // store it here also
    writeData(0x02, M(Player_Rel_YPos)); // store player's vertical position
    writeData(0x03, M(PlayerFacingDir)); // store player's facing direction
    writeData(0x04, M(Player_SprAttrib)); // store player's sprite attributes
    x = M(PlayerGfxOffset); // load graphics table offset
    y = M(Player_SprDataOffset); // get player's sprite data offset

    DrawPlayerLoop();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawPlayerLoop()
{
DrawPlayerLoop:
    // load player's left side
    writeData(0x00, M(PlayerGraphicsTable + x));
    a = M(PlayerGraphicsTable + 1 + x); // now load right side
    DrawOneSpriteRow();
    --M(0x07); // decrement rows of sprites to draw
    if (M(0x07) != 0)
        goto DrawPlayerLoop; // do this until all rows are drawn
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetCurrentAnimOffset()
{
    a = M(PlayerAnimCtrl); // get animation frame control
    GetOffsetFromAnimCtrl(); // jump to get proper offset to graphics table
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ThreeFrameExtent()
{
    a = 0x02; // load upper extent for frame control for climbing

    AnimationControl();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::AnimationControl()
{
    writeData(0x00, a); // store upper extent here
    GetCurrentAnimOffset(); // get proper offset to graphics table
    pha(); // save offset to stack
    // load animation frame timer
    if (M(PlayerAnimTimer) == 0)
    { // branch if not expired
        // get animation frame timer amount
        writeData(PlayerAnimTimer, M(PlayerAnimTimerSet)); // and set timer accordingly
        a = M(PlayerAnimCtrl);
        a += 0x01;
        if (a >= M(0x00))
        { // if frame control + 1 < upper extent, use as next
            a = 0x00; // otherwise initialize frame control
        } // SetAnimC: store as new animation frame control
        writeData(PlayerAnimCtrl, a);
    } // ExAnimC: get offset to graphics table from stack and leave
    pla();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetOffsetFromAnimCtrl()
{
        a <<= 1; // multiply animation frame control
        a <<= 1; // by eight to get proper amount
        a <<= 1; // to add to our offset
        a += M(PlayerGfxTblOffsets + y); // add to offset to graphics table
        return; // and return with result in A
}


//------------------------------------------------------------------------

void SMBEngine::RelativeBubblePosition()
{
    y = 0x01; // set for air bubble offsets
    GetProperObjOffset(); // modify X to get proper air bubble offset
    y = 0x03;
    RelWOfs(); // get the coordinates
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RelativeFireballPosition()
{
    y = 0x00; // set for fireball offsets
    GetProperObjOffset(); // modify X to get proper fireball offset
    y = 0x02;

    RelWOfs();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::RelativeMiscPosition()
{
    y = 0x02; // set for misc object offsets
    GetProperObjOffset(); // modify X to get proper misc object offset
    y = 0x06;
    RelWOfs(); // get the coordinates
    return;
}


//------------------------------------------------------------------------

void SMBEngine::RelativeBlockPosition()
{
    a = 0x09; // get coordinates of one block object
    y = 0x04; // relative to the screen
    VariableObjOfsRelPos();
    ++x; // adjust offset for other block object if any
    ++x;
    a = 0x09;
    ++y; // adjust other and get coordinates for other one

    VariableObjOfsRelPos();
    return;
}



//------------------------------------------------------------------------

void SMBEngine::GetFireballOffscreenBits()
{
    y = 0x00; // set for fireball offsets
    GetProperObjOffset(); // modify X to get proper fireball offset
    y = 0x02; // set other offset for fireball's offscreen bits
    GetOffScreenBitsSet(); // and get offscreen information about fireball
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetBubbleOffscreenBits()
{
    y = 0x01; // set for air bubble offsets
    GetProperObjOffset(); // modify X to get proper air bubble offset
    y = 0x03; // set other offset for airbubble's offscreen bits
    GetOffScreenBitsSet(); // and get offscreen information about air bubble
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetMiscOffscreenBits()
{
    y = 0x02; // set for misc object offsets
    GetProperObjOffset(); // modify X to get proper misc object offset
    y = 0x06; // set other offset for misc object's offscreen bits
    GetOffScreenBitsSet(); // and get offscreen information about misc object
    return;
}


//------------------------------------------------------------------------

void SMBEngine::GetBlockOffscreenBits()
{
    a = 0x09; // set A to add 9 bytes in order to get block obj offset
    y = 0x04; // set Y to put offscreen bits in Block_OffscreenBits

    SetOffscrBitsOffset();
    return;
}





//------------------------------------------------------------------------

void SMBEngine::HandleChangeSize()
{
    y = M(PlayerAnimCtrl); // get animation frame control
    a = M(FrameCounter) & 0b00000011; // get frame counter and execute this code every
    if (a == 0)
    { // fourth frame, otherwise branch ahead
        ++y; // increment frame control
        if (y >= 0x0a)
        { // if not there yet, skip ahead to use
            y = 0x00; // otherwise initialize both grow/shrink flag
            writeData(PlayerChangeSizeFlag, 0x00); // and animation frame control
        } // CSzNext: store proper frame control
        writeData(PlayerAnimCtrl, y);
    } // GorSLog: get player's size
    if (M(PlayerSize) == 0)
    { // if player small, skip ahead to next part
        a = M(ChangeSizeOffsetAdder + y); // get offset adder based on frame control as offset
        y = 0x0f; // load offset for player growing

    GetOffsetFromAnimCtrl();
    return;

    //------------------------------------------------------------------------
    } // ShrinkPlayer
    a = y; // add ten bytes to frame control as offset
    a += 0x0a; // this thing apparently uses two of the swimming frames
    x = a; // to draw the player shrinking
    y = 0x09; // load offset for small player swimming
    // get what would normally be offset adder
    if (M(ChangeSizeOffsetAdder + x) == 0)
    { // and branch to use offset if nonzero
        y = 0x01; // otherwise load offset for big player swimming
    } // ShrPlF: get offset to graphics table based on offset loaded
    a = M(PlayerGfxTblOffsets + y);
    return; // and leave
}

//------------------------------------------------------------------------

void SMBEngine::RenderAreaGraphics()
{
    // store LSB of where we're at
    a = M(CurrentColumnPos) & 0x01;
    writeData(0x05, a);
    y = M(VRAM_Buffer2_Offset); // store vram buffer offset
    writeData(0x00, y);
    // get current name table address we're supposed to render
    writeData(VRAM_Buffer2 + 1 + y, M(CurrentNTAddr_Low));
    writeData(VRAM_Buffer2 + y, M(CurrentNTAddr_High));
    // store length byte of 26 here with d7 set
    writeData(VRAM_Buffer2 + 2 + y, 0x9a); // to increment by 32 (in columns)
    a = 0x00; // init attribute row
    writeData(0x04, 0x00);
    x = 0x00;

    do // DrawMTLoop: store init value of 0 or incremented offset for buffer
    {
        writeData(0x01, x);
        // get first metatile number, and mask out all but 2 MSB
        a = M(MetatileBuffer + x) & 0b11000000;
        writeData(0x03, a); // store attribute table bits here
        a >>= 6; // note that metatile format is %xx000000 attribute table bits,
        y = a; // %00xxxxxx metatile number, so move the bits to d1-d0 and use as offset here
        // get address to graphics table from here
        writeData(0x06, M(MetatileGraphics_Low + y));
        writeData(0x07, M(MetatileGraphics_High + y));
        a = M(MetatileBuffer + x); // get metatile number again
        a <<= 1; // multiply by 4 and use as tile offset
        a <<= 1;
        writeData(0x02, a);
        // get current task number for level processing and
        a = M(AreaParserTaskNum) & 0b00000001; // mask out all but LSB, then invert LSB, multiply by 2
        a ^= 0b00000001; // to get the correct column position in the metatile,
        a <<= 1; // then add to the tile offset so we can draw either side
        a += M(0x02); // of the metatiles
        y = a;
        x = M(0x00); // use vram buffer offset from before as X
        writeData(VRAM_Buffer2 + 3 + x, M(W(0x06) + y)); // get first tile number (top left or top right) and store
        ++y;
        // now get the second (bottom left or bottom right) and store
        writeData(VRAM_Buffer2 + 4 + x, M(W(0x06) + y));
        y = M(0x04); // get current attribute row
        // get LSB of current column where we're at, and
        if (M(0x05) == 0)
        { // branch if set (clear = left attrib, set = right)
            // get current row we're rendering
            a = M(0x01) >> 1; // branch if LSB set (clear = top left, set = bottom left)
            if ((M(0x01) & 0x01) != 0)
                goto LLeft;
            M(0x03) >>= 6; // move the attribute bits into d1-d0, for upper left square
            goto SetAttrib;
        } // RightCheck: get LSB of current row we're rendering
        a = M(0x01) >> 1; // branch if set (clear = top right, set = bottom right)
        if ((M(0x01) & 0x01) == 0)
        {
            M(0x03) >>= 1; // shift attribute bits 4 to the right
            M(0x03) >>= 1; // thus in d3-d2, for upper right square
            M(0x03) >>= 1;
            M(0x03) >>= 1;
            goto SetAttrib;

LLeft: // shift attribute bits 2 to the right
            M(0x03) >>= 1;
            M(0x03) >>= 1; // thus in d5-d4 for lower left square
        } // NextMTRow: move onto next attribute row
        ++M(0x04);

SetAttrib: // get previously saved bits from before
        a = M(AttributeBuffer + y) | M(0x03); // if any, and put new bits, if any, onto
        writeData(AttributeBuffer + y, a); // the old, and store
        ++M(0x00); // increment vram buffer offset by 2
        ++M(0x00);
        x = M(0x01); // get current gfx buffer row, and check for
        ++x; // the bottom of the screen
    } while (x < 0x0d); // if not there yet, loop back
    y = M(0x00); // get current vram buffer offset, increment by 3
    ++y; // (for name table address and length bytes)
    ++y;
    ++y;
    writeData(VRAM_Buffer2 + y, 0x00); // put null terminator at end of data for name table
    writeData(VRAM_Buffer2_Offset, y); // store new buffer offset
    ++M(CurrentNTAddr_Low); // increment name table address low
    // check current low byte
    a = M(CurrentNTAddr_Low) & 0b00011111; // if no wraparound, just skip this part
    if (a == 0)
    {
        // if wraparound occurs, make sure low byte stays
        writeData(CurrentNTAddr_Low, 0x80); // just under the status bar
        // and then invert d2 of the name table address high
        a = M(CurrentNTAddr_High) ^ 0b00000100; // to move onto the next appropriate name table
        writeData(CurrentNTAddr_High, a);
    } // ExitDrawM: jump to set buffer to $0341 and leave
    SetVRAMCtrl();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::OnGroundStateSub()
{
    GetPlayerAnimSpeed(); // do a sub to set animation frame timing
    a = M(Left_Right_Buttons);
    if (a != 0)
    { // if left/right controller bits not set, skip instruction
        writeData(PlayerFacingDir, a); // otherwise set new facing direction
    } // GndMove: do a sub to impose friction on player's walk/run
    ImposeFriction();
    MovePlayerHorizontally(); // do another sub to move player horizontally
    writeData(Player_X_Scroll, a); // set returned value as player's movement speed for scroll
    return;
}


//------------------------------------------------------------------------

void SMBEngine::MovePlayerHorizontally()
{
    a = M(JumpspringAnimCtrl); // if jumpspring currently animating,
    if (a != 0)
        return; // branch to leave
    x = a; // otherwise set zero for offset to use player's stuff
    MoveObjectHorizontally();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::MovePlayerVertically()
{
    x = 0x00; // set X for player offset
    if (M(TimerControl) == 0)
    { // if master timer control set, branch ahead
        a = M(JumpspringAnimCtrl); // otherwise check to see if jumpspring is animating
        if (a != 0)
            return; // branch to leave if so
    } // NoJSChk: dump vertical force
    writeData(0x00, M(VerticalForce));
    a = 0x04; // set maximum vertical speed here
    ImposeGravitySprObj(); // then jump to move player vertically
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitFlyingCheepCheep()
{
    uint32_t wide = 0;

    a = M(FrenzyEnemyTimer); // if timer here not expired yet, branch to leave
    if (a != 0)
        return;
    SmallBBox(); // jump to set bounding box size $09 and init other values
    a = M(PseudoRandomBitReg + 1 + x) & 0b00000011; // set pseudorandom offset here
    y = a;
    // load timer with pseudorandom offset
    writeData(FrenzyEnemyTimer, M(FlyCCTimerData + y));
    y = 0x03; // load Y with default value
    a = M(SecondaryHardMode);
    if (a != 0)
    { // if secondary hard mode flag not set, do not increment Y
        y = 0x04; // otherwise, increment Y to allow as many as four onscreen
    } // MaxCC: store whatever pseudorandom bits are in Y
    writeData(0x00, y);
    if (x >= M(0x00))
        return; // if X => Y, branch to leave
    a = M(PseudoRandomBitReg + x) & 0b00000011; // get last two bits of LSFR, first part
    writeData(0x00, a); // and store in two places
    writeData(0x01, a);
    // set vertical speed for cheep-cheep
    writeData(Enemy_Y_Speed + x, 0xfb);
    a = 0x00; // load default value
    y = M(Player_X_Speed); // check player's horizontal speed
    if (y == 0)
        goto GSeed; // if player not moving left or right, skip this part
    a = 0x04;
    if (y < 0x19)
        goto GSeed; // do not change A
    a = 0x08; // otherwise, multiply A by 2

GSeed: // save to stack
    pha();
    a += M(0x00); // add to last two bits of LSFR we saved earlier
    writeData(0x00, a); // save it there
    a = M(PseudoRandomBitReg + 1 + x) & 0b00000011; // if neither of the last two bits of second LSFR set,
    if (a != 0)
    { // skip this part and save contents of $00
        a = M(PseudoRandomBitReg + 2 + x) & 0b00001111; // otherwise overwrite with lower nybble of
        writeData(0x00, a); // third LSFR part
    } // RSeed: get value from stack we saved earlier
    pla();
    a += M(0x01); // add to last two bits of LSFR we saved in other place
    y = a; // use as pseudorandom offset here
    // get horizontal speed using pseudorandom offset
    writeData(Enemy_X_Speed + x, M(FlyCCXSpeedData + y));
    // set to move towards the right
    writeData(Enemy_MovingDir + x, 0x01);
    // if player moving left or right, branch ahead of this part
    if (M(Player_X_Speed) != 0)
        goto D2XPos1;
    y = M(0x00); // get first LSFR or third LSFR lower nybble
    a = y; // and check for d1 set
    a &= 0b00000010;
    if (a == 0)
        goto D2XPos1; // if d1 not set, branch
    a = M(Enemy_X_Speed + x) ^ 0xff; // if d1 set, change horizontal speed
    a += 0x01; // direction
    writeData(Enemy_X_Speed + x, a);
    ++M(Enemy_MovingDir + x); // increment to move towards the left

D2XPos1: // get first LSFR or third LSFR lower nybble again
    a = y;
    a &= 0b00000010;
    if (a != 0)
    { // check for d1 set again, branch again if not set
        wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position))
             + M(FlyCCXPositionData + y); // if d1 set, add value obtained from pseudorandom offset
        writeData(Enemy_X_Position + x, LOBYTE(wide)); // and save as enemy's horizontal position
        a = HIBYTE(wide); // and jump past this part
    } // D2XPos2: get player's horizontal position
    else
    {
        wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position))
             - M(FlyCCXPositionData + y); // if d1 not set, subtract value obtained from pseudorandom
        writeData(Enemy_X_Position + x, LOBYTE(wide)); // offset and save as enemy's horizontal position
        a = HIBYTE(wide);
    } // FinCCSt: save as enemy's page location
    writeData(Enemy_PageLoc + x, a);
    writeData(Enemy_Flag + x, 0x01); // set enemy's buffer flag
    writeData(Enemy_Y_HighPos + x, 0x01); // set enemy's high vertical byte
    a = 0xf8;
    writeData(Enemy_Y_Position + x, 0xf8); // put enemy below the screen, and we are done
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveFlyGreenPTroopa()
{
    XMoveCntr_GreenPTroopa(); // do sub to increment primary and secondary counters
    MoveWithXMCntrs(); // do sub to move green paratroopa accordingly, and horizontally
    y = 0x01; // set Y to move green paratroopa down
    a = M(FrameCounter) & 0b00000011; // check frame counter 2 LSB for any bits set
    if (a == 0)
    { // branch to leave if set to move up/down every fourth frame
        a = M(FrameCounter) & 0b01000000; // check frame counter for d6 set
        if (a == 0)
        { // branch to move green paratroopa down if set
            y = 0xff; // otherwise set Y to move green paratroopa up
        } // YSway: store adder here
        writeData(0x00, y);
        a = M(Enemy_Y_Position + x);
        a += M(0x00); // to give green paratroopa a wavy flight
        writeData(Enemy_Y_Position + x, a);
    } // NoMGPT: leave!
    return;
}

//------------------------------------------------------------------------

void SMBEngine::XMoveCntr_GreenPTroopa()
{
    a = 0x13; // load preset maximum value for secondary counter
    XMoveCntr_Platform();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::XMoveCntr_Platform()
{
    writeData(0x01, a); // store value here
    a = M(FrameCounter) & 0b00000011; // branch to leave if not on
    if (a == 0)
    { // every fourth frame
        y = M(XMoveSecondaryCounter + x); // get secondary counter
        // get primary counter
        a = M(XMovePrimaryCounter + x) >> 1;
        if ((M(XMovePrimaryCounter + x) & 0x01) != 0)
            goto DecSeXM; // if d0 of primary counter set, branch elsewhere
        if (y == M(0x01))
            goto IncPXM; // if equal, branch ahead of this part
        ++M(XMoveSecondaryCounter + x); // increment secondary counter and leave
    } // NoIncXM
    return;

//------------------------------------------------------------------------

IncPXM: // increment primary counter and leave
    ++M(XMovePrimaryCounter + x);
    return;

//------------------------------------------------------------------------

DecSeXM: // put secondary counter in A
    a = y;
    if (a == 0)
        goto IncPXM; // if secondary counter at zero, branch back
    --M(XMoveSecondaryCounter + x); // otherwise decrement secondary counter and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveWithXMCntrs()
{
    a = M(XMoveSecondaryCounter + x); // save secondary counter to stack
    pha();
    y = 0x01; // set value here by default
    a = M(XMovePrimaryCounter + x) & 0b00000010; // if d1 of primary counter is
    if (a == 0)
    { // set, branch ahead of this part here
        a = M(XMoveSecondaryCounter + x) ^ 0xff; // otherwise change secondary
        a += 0x01;
        writeData(XMoveSecondaryCounter + x, a);
        y = 0x02; // load alternate value here
    } // XMRight: store as moving direction
    writeData(Enemy_MovingDir + x, y);
    MoveEnemyHorizontally();
    writeData(0x00, a); // save value obtained from sub here
    pla(); // get secondary counter from stack
    writeData(XMoveSecondaryCounter + x, a); // and return to original place
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RunStarFlagObj()
{
    // initialize enemy frenzy buffer
    writeData(EnemyFrenzyBuffer, 0x00);
    a = M(StarFlagTaskControl); // check star flag object task number here
    if (a >= 0x05)
        return;
    switch (a)
    {
    case 0:
        return;
    case 1:
        goto GameTimerFireworks;
    case 2:
        goto AwardGameTimerPoints;
    case 3:
        goto RaiseFlagSetoffFWorks;
    case 4:
        goto DelayToAreaEnd;
    default:
        bad_jump();
        return;
    } // otherwise jump to appropriate sub

GameTimerFireworks:
    y = 0x05; // set default state for star flag object
    a = M(GameTimerDisplay + 2); // get game timer's last digit
    if (a == 0x01)
        goto SetFWC; // if last digit of game timer set to 1, skip ahead
    y = 0x03; // otherwise load new value for state
    if (a == 0x03)
        goto SetFWC; // if last digit of game timer set to 3, skip ahead
    y = 0x00; // otherwise load one more potential value for state
    if (a == 0x06)
        goto SetFWC; // if last digit of game timer set to 6, skip ahead
    a = 0xff; // otherwise set value for no fireworks

SetFWC: // set fireworks counter here
    writeData(FireworksCounter, a);
    writeData(Enemy_State + x, y); // set whatever state we have in star flag object

    do // IncrementSFTask1
    {
        ++M(StarFlagTaskControl); // increment star flag object task number

        return; // StarFlagExit: leave

    //------------------------------------------------------------------------

AwardGameTimerPoints:
        // check all game timer digits for any intervals left
        a = M(GameTimerDisplay) | M(GameTimerDisplay + 1);
        a |= M(GameTimerDisplay + 2);
    } while (a == 0); // if no time left on game timer at all, branch to next task
    a = M(FrameCounter) & 0b00000100; // check frame counter for d2 set (skip ahead
    if (a != 0)
    { // for four frames every four frames) branch if not set
        a = Sfx_TimerTick;
        writeData(Square2SoundQueue, Sfx_TimerTick); // load timer tick sound
    } // NoTTick: set offset here to subtract from game timer's last digit
    y = 0x23;
    a = 0xff; // set adder here to $ff, or -1, to subtract one
    writeData(DigitModifier + 5, 0xff); // from the last digit of the game timer
    DigitsMathRoutine(); // subtract digit
    a = 0x05; // set now to add 50 points
    writeData(DigitModifier + 5, 0x05); // per game timer interval subtracted

    EndAreaPoints();
    return;

RaiseFlagSetoffFWorks:
    // check star flag's vertical position
    if (M(Enemy_Y_Position + x) >= 0x72)
    { // if star flag higher vertically, branch to other code
        --M(Enemy_Y_Position + x); // otherwise, raise star flag by one pixel
        DrawStarFlag(); // and skip this part here
        return;
    } // SetoffF: check fireworks counter
    a = M(FireworksCounter);
    if (a == 0)
        goto DrawFlagSetTimer; // if no fireworks left to go off, skip this part
    if ((a & 0x80) != 0)
        goto DrawFlagSetTimer; // if no fireworks set to go off, skip this part
    a = Fireworks;
    writeData(EnemyFrenzyBuffer, Fireworks); // otherwise set fireworks object in frenzy queue

    DrawStarFlag();
    return;

//------------------------------------------------------------------------

DrawFlagSetTimer:
    DrawStarFlag(); // do sub to draw star flag
    a = 0x06;
    writeData(EnemyIntervalTimer + x, 0x06); // set interval timer here

IncrementSFTask2:
    ++M(StarFlagTaskControl); // move onto next task
    return;

//------------------------------------------------------------------------

DelayToAreaEnd:
    DrawStarFlag(); // do sub to draw star flag
    a = M(EnemyIntervalTimer + x); // if interval timer set in previous task
    if (a == 0)
    { // not yet expired, branch to leave
        a = M(EventMusicBuffer); // if event music buffer empty,
        if (a == 0)
            goto IncrementSFTask2; // branch to increment task
    } // StarFlagExit2
    return; // otherwise leave
}

//------------------------------------------------------------------------

void SMBEngine::YMovingPlatform()
{
    // if platform moving up or down, skip ahead to
    a = M(Enemy_Y_Speed + x) | M(Enemy_Y_MoveForce + x); // check on other position
    if (a != 0)
        goto ChkYCenterPos;
    writeData(Enemy_YMF_Dummy + x, a); // initialize dummy variable
    if (M(Enemy_Y_Position + x) >= M(YPlatformTopYPos + x))
        goto ChkYCenterPos; // ahead of all this
    a = M(FrameCounter) & 0b00000111; // check for every eighth frame
    if (a == 0)
    {
        ++M(Enemy_Y_Position + x); // increase vertical position every eighth frame
    } // SkipIY: skip ahead to last part
    ChkYPCollision();
    return;

ChkYCenterPos:
    // if current vertical position < central position, branch
    if (M(Enemy_Y_Position + x) >= M(YPlatformCenterYPos + x))
    {
        MovePlatformUp(); // otherwise start slowing descent/moving upwards
        ChkYPCollision();
        return;
    } // YMDown: start slowing ascent/moving downwards
    MovePlatformDown();

    ChkYPCollision();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveLargeLiftPlat()
{
    MoveLiftPlatforms(); // execute common to all large and small lift platforms
    ChkYPCollision(); // branch to position player correctly
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveSmallPlatform()
{
    MoveLiftPlatforms(); // execute common to all large and small lift platforms
    ChkSmallPlatCollision(); // branch to position player correctly
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveLiftPlatforms()
{
    uint32_t wide = 0;

    a = M(TimerControl); // if master timer control set, skip all of this
    if (a != 0)
        return; // and branch to leave
    // position:dummy and speed:force are each one 16-bit quantity
    wide = ((M(Enemy_Y_Position + x) << 8) | M(Enemy_YMF_Dummy + x))
         + ((M(Enemy_Y_Speed + x) << 8) | M(Enemy_Y_MoveForce + x)); // move up or down
    writeData(Enemy_YMF_Dummy + x, LOBYTE(wide));
    writeData(Enemy_Y_Position + x, HIBYTE(wide)); // and then leave
    a = HIBYTE(wide);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ChkSmallPlatCollision()
{
    a = M(PlatformCollisionFlag + x); // get bounding box counter saved in collision flag
    if (a == 0)
        return; // if none found, leave player position alone
    PositionPlayerOnS_Plat(); // use to position player correctly

    return; // ExLiftP: then leave
}


//------------------------------------------------------------------------

void SMBEngine::SmallPlatformCollision()
{
    bool collisionFound = false;
    bool playerVerticalOutOfRange = false;

    a = M(TimerControl); // if master timer control set,
    if (a != 0)
        goto ExSPC; // branch to leave
    writeData(PlatformCollisionFlag + x, a); // otherwise initialize collision flag
    playerVerticalOutOfRange = CheckPlayerVertical(); // do a sub to see if player is below a certain point
    if (playerVerticalOutOfRange)
        goto ExSPC; // or entirely offscreen, and branch to leave if true
    a = 0x02;
    writeData(0x00, 0x02); // load counter here for 2 bounding boxes

    do // ChkSmallPlatLoop
    {
        x = M(ObjectOffset); // get enemy object offset
        GetEnemyBoundBoxOfs(); // get bounding box offset in Y
        a &= 0b00000010; // if d1 of offscreen lower nybble bits was set
        if (a != 0)
            goto ExSPC; // then branch to leave
        // check top of platform's bounding box for being
        if (M(BoundingBox_UL_YPos + y) >= 0x20)
        { // if so, branch, don't do collision detection
            collisionFound = PlayerCollisionCore(); // otherwise, perform player-to-platform collision detection
            if (collisionFound)
                goto ProcSPlatCollisions; // skip ahead if collision
        } // MoveBoundBox
        a = M(BoundingBox_UL_YPos + y); // move bounding box vertical coordinates
        a += 0x80;
        writeData(BoundingBox_UL_YPos + y, a);
        a = M(BoundingBox_DR_YPos + y);
        a += 0x80;
        writeData(BoundingBox_DR_YPos + y, a);
        --M(0x00); // decrement counter we set earlier
    } while (M(0x00) != 0); // loop back until both bounding boxes are checked

ExSPC: // get enemy object buffer offset, then leave
    x = M(ObjectOffset);
    return;

//------------------------------------------------------------------------

ProcSPlatCollisions:
    x = M(ObjectOffset); // return enemy object buffer offset to X, then continue
    ProcLPlatCollisions();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ProcLPlatCollisions()
{
    a = M(BoundingBox_DR_YPos + y); // get difference by subtracting the top
    a -= M(BoundingBox_UL_YPos); // of the platform's bounding box
    if (a >= 0x04)
        goto ChkForTopCollision; // branch, do not alter vertical speed of player
    // check to see if player's vertical speed is moving down
    if ((M(Player_Y_Speed) & 0x80) == 0)
        goto ChkForTopCollision; // if so, don't mess with it
    a = 0x01; // otherwise, set vertical
    writeData(Player_Y_Speed, 0x01); // speed of player to kill jump

ChkForTopCollision:
    a = M(BoundingBox_DR_YPos); // get difference by subtracting the top
    a -= M(BoundingBox_UL_YPos + y); // of the player's bounding box
    if (a >= 0x06)
        goto PlatformSideCollisions; // if difference not close enough, skip all of this
    if ((M(Player_Y_Speed) & 0x80) != 0)
        goto PlatformSideCollisions; // if player's vertical speed moving upwards, skip this
    a = M(0x00); // get saved bounding box counter from earlier
    y = M(Enemy_ID + x);
    if (y == 0x2b)
        goto SetCollisionFlag; // regardless of which one, branch to use bounding box counter
    if (y == 0x2c)
        goto SetCollisionFlag;
    a = x; // otherwise use enemy object buffer offset

SetCollisionFlag:
    x = M(ObjectOffset); // get enemy object buffer offset
    writeData(PlatformCollisionFlag + x, a); // save either bounding box counter or enemy offset here
    a = 0x00;
    writeData(Player_State, 0x00); // set player state to normal then leave
    return;

//------------------------------------------------------------------------

PlatformSideCollisions:
    // set value here to indicate possible horizontal
    writeData(0x00, 0x01); // collision on left side of platform
    a = M(BoundingBox_DR_XPos); // get difference by subtracting platform's left edge
    a -= M(BoundingBox_UL_XPos + y);
    if (a >= 0x08)
    {
        ++M(0x00); // otherwise increment value set here for right side collision
        // get difference by subtracting player's left edge
        // the original clears the carry rather than setting it here, so the
        // subtraction takes one pixel more than it means to
        a = (uint8_t)(M(BoundingBox_DR_XPos + y) - M(BoundingBox_UL_XPos) - 1); // from platform's right edge
        if (a >= 0x09)
            goto NoSideC; // and instead branch to leave (no collision)
    } // SideC: deal with horizontal collision
    ImpedePlayerMove();

NoSideC: // return with enemy object buffer offset
    x = M(ObjectOffset);
    return;
}

//------------------------------------------------------------------------

bool SMBEngine::CheckForSolidMTiles()
{
    bool solidMTileFound = false;

    GetMTileAttrib(); // find appropriate offset based on metatile's 2 MSB
    solidMTileFound = a >= M(SolidMTileUpperExt + x); // compare current metatile with solid metatiles
    return solidMTileFound;
}

//------------------------------------------------------------------------

bool SMBEngine::CheckForClimbMTiles()
{
    bool climbMTileFound = false;

    GetMTileAttrib(); // find appropriate offset based on metatile's 2 MSB
    climbMTileFound = a >= M(ClimbMTileUpperExt + x); // compare current metatile with climbable metatiles
    return climbMTileFound;
}

//------------------------------------------------------------------------

void SMBEngine::GetMTileAttrib()
{
    y = a; // save metatile value into Y
    a &= 0b11000000; // mask out all but 2 MSB
    a >>= 6; // shift d7-d6 down to d1-d0
    x = a; // use as offset for metatile data
    a = y; // get original metatile value back

    return; // ExEBG: leave
}

//------------------------------------------------------------------------

void SMBEngine::FireballBGCollision()
{
    // check fireball's vertical coordinate
    if (M(Fireball_Y_Position + x) < 0x18)
        goto ClearBounceFlag; // if within the status bar area of the screen, branch ahead
    BlockBufferChk_FBall(); // do fireball to background collision detection on bottom of it
    if (a == 0)
        goto ClearBounceFlag; // if nothing underneath fireball, branch
    ChkForNonSolids(); // check for non-solid metatiles
    if (a == 0x26 || a == 0xc2 || a == 0xc3
        || a == 0x5f || a == 0x60)
        goto ClearBounceFlag; // branch if any found
    // if fireball's vertical speed set to move upwards,
    if ((M(Fireball_Y_Speed + x) & 0x80) != 0)
        goto InitFireballExplode; // branch to set exploding bit in fireball's state
    // if bouncing flag already set,
    if (M(FireballBouncingFlag + x) != 0)
        goto InitFireballExplode; // branch to set exploding bit in fireball's state
    writeData(Fireball_Y_Speed + x, 0xfd); // otherwise set vertical speed to move upwards (give it bounce)
    writeData(FireballBouncingFlag + x, 0x01); // set bouncing flag
    a = M(Fireball_Y_Position + x) & 0xf8; // modify vertical coordinate to land it properly
    writeData(Fireball_Y_Position + x, a); // store as new vertical coordinate
    return; // leave

//------------------------------------------------------------------------

ClearBounceFlag:
    a = 0x00;
    writeData(FireballBouncingFlag + x, 0x00); // clear bouncing flag by default
    return; // leave

//------------------------------------------------------------------------

InitFireballExplode:
    writeData(Fireball_State + x, 0x80); // set exploding flag in fireball's state
    a = Sfx_Bump;
    writeData(Square1SoundQueue, Sfx_Bump); // load bump sound
    return; // leave
}



//------------------------------------------------------------------------

void SMBEngine::ProcessPlayerAction()
{
    a = M(Player_State); // get player's state
    if (a != 0x03)
    { // if climbing, branch here
        if (a != 0x02)
        { // if falling, branch here
            if (a == 0x01)
            { // if not jumping, branch here
                if (M(SwimmingFlag) != 0)
                    goto ActionSwimming; // if swimming flag set, branch elsewhere
                y = 0x06; // load offset for crouching
                // get crouching flag
                if (M(CrouchingFlag) != 0)
                    goto NonAnimatedActs; // if set, branch to get offset for graphics table
                y = 0x00; // otherwise load offset for jumping
                goto NonAnimatedActs; // go to get offset to graphics table
            } // ProcOnGroundActs
            y = 0x06; // load offset for crouching
            // get crouching flag
            if (M(CrouchingFlag) != 0)
                goto NonAnimatedActs; // if set, branch to get offset for graphics table
            y = 0x02; // load offset for standing
            // check player's horizontal speed
            a = M(Player_X_Speed) | M(Left_Right_Buttons); // and left/right controller bits
            if (a == 0)
                goto NonAnimatedActs; // if no speed or buttons pressed, use standing offset
            // load walking/running speed
            if (M(Player_XSpeedAbsolute) < 0x09)
                goto ActionWalkRun; // if less than a certain amount, branch, too slow to skid
            // otherwise check to see if moving direction
            a = M(Player_MovingDir) & M(PlayerFacingDir); // and facing direction are the same
            if (a != 0)
                goto ActionWalkRun; // if moving direction = facing direction, branch, don't skid
            y = 0x03; // otherwise increment to skid offset ($03)

NonAnimatedActs:
            GetGfxOffsetAdder(); // do a sub here to get offset adder for graphics table
            writeData(PlayerAnimCtrl, 0x00); // initialize animation frame control
            a = M(PlayerGfxTblOffsets + y); // load offset to graphics table using size as offset
            return;

        //------------------------------------------------------------------------
        } // ActionFalling
        y = 0x04; // load offset for walking/running
        GetGfxOffsetAdder(); // get offset to graphics table
        GetCurrentAnimOffset(); // execute instructions for falling state
        return;

ActionWalkRun:
        y = 0x04; // load offset for walking/running
        GetGfxOffsetAdder(); // get offset to graphics table
        goto FourFrameExtent; // execute instructions for normal state
    } // ActionClimbing
    y = 0x05; // load offset for climbing
    // check player's vertical speed
    if (M(Player_Y_Speed) == 0)
        goto NonAnimatedActs; // if no speed, branch, use offset as-is
    GetGfxOffsetAdder(); // otherwise get offset for graphics table
    ThreeFrameExtent(); // then skip ahead to more code
    return;

ActionSwimming:
    y = 0x01; // load offset for swimming
    GetGfxOffsetAdder();
    // check jump/swim timer
    a = M(JumpSwimTimer) | M(PlayerAnimCtrl); // and animation frame control
    if (a != 0)
        goto FourFrameExtent; // if any one of these set, branch ahead
    a = M(A_B_Buttons);
    a <<= 1; // check for A button pressed
    if ((M(A_B_Buttons) & 0x80) != 0)
        goto FourFrameExtent; // branch to same place if A button pressed

    GetCurrentAnimOffset();
    return;

FourFrameExtent:
    a = 0x03; // load upper extent for frame control
    AnimationControl(); // jump to get offset and animate player object
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PlayerMovementSubs()
{
    a = 0x00; // set A to init crouch flag by default
    // is player small?
    if (M(PlayerSize) == 0)
    { // if so, branch
        // check state of player
        if (M(Player_State) != 0)
            goto ProcMove; // if not on the ground, branch
        // load controller bits for up and down
        a = M(Up_Down_Buttons) & 0b00000100; // single out bit for down button
    } // SetCrouch: store value in crouch flag
    writeData(CrouchingFlag, a);

ProcMove: // run sub related to jumping and swimming
    PlayerPhysicsSub();
    a = M(PlayerChangeSizeFlag); // if growing/shrinking flag set,
    if (a == 0)
    { // branch to leave
        a = M(Player_State);
        if (a != 0x03)
        { // if climbing, branch ahead, leave timer unset
            y = 0x18;
            writeData(ClimbSideTimer, 0x18); // otherwise reset timer now
        } // MoveSubs
        switch (a)
        {
        case 0:
            OnGroundStateSub();
            return;
        case 1:
            goto JumpSwimSub;
        case 2:
            goto FallingSub;
        case 3:
            ClimbingSub();
            return;
        default:
            bad_jump();
            return;
        }
    } // NoMoveSub
    return;

//------------------------------------------------------------------------

FallingSub:
    writeData(VerticalForce, M(VerticalForceDown)); // dump vertical movement force for falling into main one
    goto LRAir; // movement force, then skip ahead to process left/right movement

JumpSwimSub:
    y = M(Player_Y_Speed); // if player's vertical speed zero
    if ((y & 0x80) != 0)
    { // or moving downwards, branch to falling
        a = M(A_B_Buttons) & A_Button; // check to see if A button is being pressed
        a &= M(PreviousA_B_Buttons); // and was pressed in previous frame
        if (a != 0)
            goto ProcSwim; // if so, branch elsewhere
        a = M(JumpOrigin_Y_Position); // get vertical position player jumped from
        a -= M(Player_Y_Position); // subtract current from original vertical coordinate
        if (a < M(DiffToHaltJump))
            goto ProcSwim; // or just starting to jump, if just starting, skip ahead
    } // DumpFall: otherwise dump falling into main fractional
    writeData(VerticalForce, M(VerticalForceDown));

ProcSwim: // if swimming flag not set,
    if (M(SwimmingFlag) == 0)
        goto LRAir; // branch ahead to last part
    GetPlayerAnimSpeed(); // do a sub to get animation frame timing
    if (M(Player_Y_Position) < 0x14)
    { // if not yet reached a certain position, branch ahead
        a = 0x18;
        writeData(VerticalForce, 0x18); // otherwise set fractional
    } // LRWater: check left/right controller bits (check for swimming)
    a = M(Left_Right_Buttons);
    if (a == 0)
        goto LRAir; // if not pressing any, skip
    writeData(PlayerFacingDir, a); // otherwise set facing direction accordingly

LRAir: // check left/right controller bits (check for jumping/falling)
    a = M(Left_Right_Buttons);
    if (a != 0)
    { // if not pressing any, skip
        ImposeFriction(); // otherwise process horizontal movement
    } // JSMove: do a sub to move player horizontally
    MovePlayerHorizontally();
    writeData(Player_X_Scroll, a); // set player's speed here, to be used for scroll later
    if (M(GameEngineSubroutine) == 0x0b)
    { // branch if not set to run
        a = 0x28;
        writeData(VerticalForce, 0x28); // otherwise set fractional
    } // ExitMov1: jump to move player vertically, then leave
    MovePlayerVertically();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DisplayTimeUp()
{
    a = M(GameTimerExpiredFlag); // if game timer not expired, increment task
    if (a != 0)
    { // control 2 tasks forward, otherwise, stay here
        writeData(GameTimerExpiredFlag, 0x00); // reset timer expiration flag
        a = 0x02; // output time-up screen to buffer
        OutputInter();
        return;
    } // NoTimeUp: increment control task 2 tasks forward
    ++M(ScreenRoutineTask);
    IncSubtask();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::OutputInter()
{
        WriteGameText();
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
        a = 0x01; // lives display, then output lives display to buffer
        OutputInter();
        return;

    //------------------------------------------------------------------------
    } // GameOverInter: set screen timer
    writeData(ScreenTimer, 0x12);
    a = 0x03; // output game over screen to buffer
    WriteGameText();
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

void SMBEngine::DonePlayerTask()
{
        writeData(TimerControl, 0x00); // initialize master timer control to continue timers
        a = 0x08;
        writeData(GameEngineSubroutine, 0x08); // set player control routine to run next frame
        return; // leave
}

//------------------------------------------------------------------------

    //------------------------------------------------------------------------

void SMBEngine::PlayerFireFlower()
{
        // check master timer control
        if (M(TimerControl) != 0xc0)
        { // branch if at moment, not before or after
            // get frame counter
            a = M(FrameCounter) >> 2; // divide by four to change every four frames

    CyclePlayerPalette();
    return;

        //------------------------------------------------------------------------
        } // ResetPalFireFlower
        DonePlayerTask(); // do sub to init timer control and run player control routine

    ResetPalStar();
    return;
}



//------------------------------------------------------------------------

// otherwise increment two bytes
void SMBEngine::Inc2B()
{
    ++M(EnemyDataOffset);
    ++M(EnemyDataOffset);
    a = 0x00; // init page select for enemy objects
    writeData(EnemyObjectPageSel, 0x00);
    x = M(ObjectOffset); // reload current offset in enemy buffers
    return; // and leave
}

//------------------------------------------------------------------------

    //------------------------------------------------------------------------

void SMBEngine::StopPlayerMove()
{
        ImpedePlayerMove(); // stop player's movement

        return; // ExCSM: leave
}

//------------------------------------------------------------------------

    //------------------------------------------------------------------------

void SMBEngine::HandleCoinMetatile()
{
        ErACM(); // do sub to erase coin metatile from block buffer
        ++M(CoinTallyFor1Ups); // increment coin tally used for 1-up blocks
        GiveOneCoin(); // update coin amount and tally on the screen
        return;
}

//------------------------------------------------------------------------

void SMBEngine::PutPlayerOnVine()
{
    // set player state to climbing
    writeData(Player_State, 0x03);
    // nullify player's horizontal speed
    writeData(Player_X_Speed, 0x00); // and fractional horizontal movement force
    writeData(Player_X_MoveForce, 0x00);
    a = M(Player_X_Position); // get player's horizontal coordinate
    a -= M(ScreenLeft_X_Pos); // subtract from left side horizontal coordinate
    if (a < 0x10)
    { // if 16 or more pixels difference, do not alter facing direction
        a = 0x02;
        writeData(PlayerFacingDir, 0x02); // otherwise force player to face left
    } // SetVXPl: get current facing direction, use as offset
    y = M(PlayerFacingDir);
    a = M(0x06); // get low byte of block buffer address
    a <<= 1;
    a <<= 1; // move low nybble to high
    a <<= 1;
    a <<= 1;
    a += M(ClimbXPosAdder - 1 + y); // add pixels depending on facing direction
    writeData(Player_X_Position, a); // store as player's horizontal coordinate
    a = M(0x06); // get low byte of block buffer address again
    if (a == 0)
    { // if not zero, branch
        a = M(ScreenRight_PageLoc); // load page location of right side of screen
        a += M(ClimbPLocAdder - 1 + y); // add depending on facing location
        writeData(Player_PageLoc, a); // store as player's page location
    } // ExPVne: finally, we're done!
    return;
}

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

void SMBEngine::WarpZoneObject()
{
    a = M(ScrollLock); // check for scroll lock flag
    if (a == 0)
        return; // branch if not set to leave
    // check to see if player's vertical coordinate has
    a = M(Player_Y_Position) & M(Player_Y_HighPos); // same bits set as in vertical high byte (why?)
    if (a != 0)
        return; // if so, branch to leave
    writeData(ScrollLock, a); // otherwise nullify scroll lock flag
    ++M(WarpZoneControl); // increment warp zone flag to make warp pipes for warp zone
    EraseEnemyObject(); // kill this object
    return;
}

//------------------------------------------------------------------------

void SMBEngine::VineObjectHandler()
{
    bool shiftedBit = false;

    if (x != 0x05)
        goto ExitVH; // if not in last slot, branch to leave
    y = M(VineFlagOffset);
    --y; // decrement vine flag in Y, use as offset
    if (M(VineHeight) == M(VineHeightData + y))
        goto RunVSubs; // branch ahead to skip this part
    // get frame counter
    a = M(FrameCounter) >> 1; // take d1
    shiftedBit = (a & 0x01) != 0;
    a >>= 1;
    if (!shiftedBit)
        goto RunVSubs; // if d1 not set (2 frames every 4) skip this part
    a = M(Enemy_Y_Position + 5);
    a -= 0x01; // subtract vertical position of vine
    writeData(Enemy_Y_Position + 5, a); // one pixel every frame it's time
    ++M(VineHeight); // increment vine height

RunVSubs: // if vine still very small,
    a = M(VineHeight);
    if (a < 0x08)
        goto ExitVH;
    RelativeEnemyPosition(); // get relative coordinates of vine,
    GetEnemyOffscreenBits(); // and any offscreen bits
    y = 0x00; // initialize offset used in draw vine sub

    do // VDrawLoop: draw vine
    {
        DrawVine();
        ++y; // increment offset
    } while (y != M(VineFlagOffset)); // do not yet match, loop back to draw more vine
    a = M(Enemy_OffscreenBits) & 0b00001100; // mask offscreen bits
    if (a != 0)
    { // if none of the saved offscreen bits set, skip ahead
        --y; // otherwise decrement Y to get proper offset again

        do // KillVine: get enemy object offset for this vine object
        {
            x = M(VineObjOffset + y);
            EraseEnemyObject(); // kill this vine object
            --y; // decrement Y
        } while ((y & 0x80) == 0); // if any vine objects left, loop back to kill it
        writeData(VineFlagOffset, a); // initialize vine flag/offset
        writeData(VineHeight, a); // initialize vine height
    } // WrCMTile: check vine height
    a = M(VineHeight);
    if (a < 0x20)
        goto ExitVH; // then branch ahead to leave
    x = 0x06; // set offset in X to last enemy slot
    a = 0x01; // set A to obtain horizontal in $04, but we don't care
    y = 0x1b; // set Y to offset to get block at ($04, $10) of coordinates
    BlockBufferCollision(); // do a sub to get block buffer address set, return contents
    y = M(0x02);
    if (y >= 0xd0)
        goto ExitVH; // current block buffer, branch to leave, do not write
    a = M(W(0x06) + y); // otherwise check contents of block buffer at
    if (a != 0)
        goto ExitVH; // current offset, if not empty, branch to leave
    a = 0x26;
    writeData(W(0x06) + y, 0x26); // otherwise, write climbing metatile to block buffer

ExitVH: // get enemy object offset and leave
    x = M(ObjectOffset);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitLakitu()
{
    // check to see if an enemy is already in
    if (M(EnemyFrenzyBuffer) == 0)
    { // the frenzy buffer, and branch to kill lakitu if so

    SetupLakitu();
    return;
    } // KillLakitu
    EraseEnemyObject();
    return;
}



//------------------------------------------------------------------------

void SMBEngine::FloateyNumbersRoutine()
{
    bool borrow = false;

    a = M(FloateyNum_Control + x); // load control for floatey number
    if (a == 0)
        return; // if zero, branch to leave
    if (a >= 0x0b)
    {
        a = 0x0b; // otherwise set to $0b, thus keeping
        writeData(FloateyNum_Control + x, 0x0b); // it in range
    } // ChkNumTimer: use as Y
    y = a;
    a = M(FloateyNum_Timer + x); // check value here
    if (a == 0)
    { // if nonzero, branch ahead
        writeData(FloateyNum_Control + x, a); // initialize floatey number control and leave
        return;

    //------------------------------------------------------------------------
    } // DecNumTimer: decrement value here
    --M(FloateyNum_Timer + x);
    if (a == 0x2b)
    {
        if (y == 0x0b)
        { // branch ahead if not found
            ++M(NumberofLives); // give player one extra life (1-up)
            a = Sfx_ExtraLife;
            writeData(Square2SoundQueue, Sfx_ExtraLife); // and play the 1-up sound
        } // LoadNumTiles: load point value here
        a = M(ScoreUpdateData + y) >> 4; // move high nybble to low
        x = a; // use as X offset, essentially the digit
        // load again and this time
        a = M(ScoreUpdateData + y) & 0b00001111; // mask out the high nybble
        writeData(DigitModifier + x, a); // store as amount to add to the digit
        AddToScore(); // update the score accordingly
    } // ChkTallEnemy: get OAM data offset for enemy object
    y = M(Enemy_SprDataOffset + x);
    a = M(Enemy_ID + x); // get enemy object identifier
    if (a == Spiny)
        goto FloateyPart; // branch if spiny
    if (a == PiranhaPlant)
        goto FloateyPart; // branch if piranha plant
    if (a == HammerBro)
        goto GetAltOffset; // branch elsewhere if hammer bro
    if (a == GreyCheepCheep)
        goto FloateyPart; // branch if cheep-cheep of either color
    if (a == RedCheepCheep)
        goto FloateyPart;
    if (a >= TallEnemy)
        goto GetAltOffset; // branch elsewhere if enemy object => $09
    if (M(Enemy_State + x) >= 0x02)
        goto FloateyPart; // $02 or greater, branch beyond this part

GetAltOffset: // load some kind of control bit
    x = M(SprDataOffset_Ctrl);
    y = M(Alt_SprDataOffset + x); // get alternate OAM data offset
    x = M(ObjectOffset); // get enemy object offset again

FloateyPart: // get vertical coordinate for
    a = M(FloateyNum_Y_Pos + x);
    borrow = a < 0x18; // the compare's borrow is still live at the subtract below
    if (a >= 0x18)
    { // status bar, branch
        --a;
        writeData(FloateyNum_Y_Pos + x, a); // otherwise subtract one and store as new
    } // SetupNumSpr: get vertical coordinate
    a = (uint8_t)(M(FloateyNum_Y_Pos + x) - 0x08 - (borrow ? 1 : 0)); // subtract eight (and the borrow) and dump into the
    DumpTwoSpr(); // left and right sprite's Y coordinates
    a = M(FloateyNum_X_Pos + x); // get horizontal coordinate
    writeData(Sprite_X_Position + y, a); // store into X coordinate of left sprite
    a += 0x08; // add eight pixels and store into X
    writeData(Sprite_X_Position + 4 + y, a); // coordinate of right sprite
    writeData(Sprite_Attributes + y, 0x02); // set palette control in attribute bytes
    writeData(Sprite_Attributes + 4 + y, 0x02); // of left and right sprites
    a = M(FloateyNum_Control + x);
    a <<= 1; // multiply our floatey number control by 2
    x = a; // and use as offset for look-up table
    writeData(Sprite_Tilenumber + y, M(FloateyNumTileData + x)); // display first half of number of points
    a = M(FloateyNumTileData + 1 + x);
    writeData(Sprite_Tilenumber + 4 + y, a); // display the second half
    x = M(ObjectOffset); // get enemy object offset and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawHammer()
{
    y = M(Misc_SprDataOffset + x); // get misc object OAM data offset
    if (M(TimerControl) == 0)
    { // if master timer control set, skip this part
        // otherwise get hammer's state
        a = M(Misc_State + x) & 0b01111111; // mask out d7
        if (a == 0x01)
            goto GetHPose; // if so, branch
    } // ForceHPose: reset offset here
    x = 0x00;
    if (x != 0)
    { // do unconditional branch to rendering part

GetHPose: // get frame counter
        a = M(FrameCounter) >> 2; // move d3-d2 to d1-d0
        a &= 0b00000011; // mask out all but d1-d0 (changes every four frames)
        x = a; // use as timing offset
    } // RenderH: get relative vertical coordinate
    a = M(Misc_Rel_YPos);
    a += M(FirstSprYPos + x); // add first sprite vertical adder based on offset
    writeData(Sprite_Y_Position + y, a); // store as sprite Y coordinate for first sprite
    a += M(SecondSprYPos + x); // add second sprite vertical adder based on offset
    writeData(Sprite_Y_Position + 4 + y, a); // store as sprite Y coordinate for second sprite
    a = M(Misc_Rel_XPos); // get relative horizontal coordinate
    a += M(FirstSprXPos + x); // add first sprite horizontal adder based on offset
    writeData(Sprite_X_Position + y, a); // store as sprite X coordinate for first sprite
    a += M(SecondSprXPos + x); // add second sprite horizontal adder based on offset
    writeData(Sprite_X_Position + 4 + y, a); // store as sprite X coordinate for second sprite
    writeData(Sprite_Tilenumber + y, M(FirstSprTilenum + x)); // get and store tile number of first sprite
    writeData(Sprite_Tilenumber + 4 + y, M(SecondSprTilenum + x)); // get and store tile number of second sprite
    a = M(HammerSprAttrib + x);
    writeData(Sprite_Attributes + y, a); // get and store attribute bytes for both
    writeData(Sprite_Attributes + 4 + y, a); // note in this case they use the same data
    x = M(ObjectOffset); // get misc object offset
    a = M(Misc_OffscreenBits) & 0b11111100; // check offscreen bits
    if (a != 0)
    { // if all bits clear, leave object alone
        writeData(Misc_State + x, 0x00); // otherwise nullify misc object state
        a = 0xf8;
        DumpTwoSpr(); // do sub to move hammer sprites offscreen
    } // NoHOffscr: leave
    return;
}






//------------------------------------------------------------------------

void SMBEngine::DrawSmallPlatform()
{
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    a = 0x5b; // load tile number for small platforms
    ++y; // increment offset for tile numbers
    DumpSixSpr(); // dump tile number into all six sprites
    ++y; // increment offset for attributes
    a = 0x02; // load palette controls
    DumpSixSpr(); // dump attributes into all six sprites
    --y; // decrement for original offset
    --y;
    a = M(Enemy_Rel_XPos); // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, a);
    writeData(Sprite_X_Position + 12 + y, a); // dump as X coordinate into first and fourth sprites
    a += 0x08; // add eight pixels
    writeData(Sprite_X_Position + 4 + y, a); // dump into second and fifth sprites
    writeData(Sprite_X_Position + 16 + y, a);
    a += 0x08; // add eight more pixels
    writeData(Sprite_X_Position + 8 + y, a); // dump into third and sixth sprites
    writeData(Sprite_X_Position + 20 + y, a);
    a = M(Enemy_Y_Position + x); // get vertical coordinate
    x = a;
    pha(); // save to stack
    if (x < 0x20)
    { // do not mess with it
        a = 0xf8; // otherwise move first three sprites offscreen
    } // TopSP: dump vertical coordinate into Y coordinates
    DumpThreeSpr();
    pla(); // pull from stack
    a += 0x80; // add 128 pixels
    x = a;
    if (x < 0x20)
    { // then do not change altered coordinate
        a = 0xf8; // otherwise move last three sprites offscreen
    } // BotSP: dump vertical coordinate + 128 pixels
    writeData(Sprite_Y_Position + 12 + y, a);
    writeData(Sprite_Y_Position + 16 + y, a); // into Y coordinates
    writeData(Sprite_Y_Position + 20 + y, a);
    a = M(Enemy_OffscreenBits); // get offscreen bits
    pha(); // save to stack
    a &= 0b00001000; // check d3
    if (a != 0)
    {
        a = 0xf8; // if d3 was set, move first and
        writeData(Sprite_Y_Position + y, 0xf8); // fourth sprites offscreen
        writeData(Sprite_Y_Position + 12 + y, 0xf8);
    } // SOfs: move out and back into stack
    pla();
    pha();
    a &= 0b00000100; // check d2
    if (a != 0)
    {
        a = 0xf8; // if d2 was set, move second and
        writeData(Sprite_Y_Position + 4 + y, 0xf8); // fifth sprites offscreen
        writeData(Sprite_Y_Position + 16 + y, 0xf8);
    } // SOfs2: get from stack
    pla();
    a &= 0b00000010; // check d1
    if (a != 0)
    {
        a = 0xf8; // if d1 was set, move third and
        writeData(Sprite_Y_Position + 8 + y, 0xf8); // sixth sprites offscreen
        writeData(Sprite_Y_Position + 20 + y, 0xf8);
    } // ExSPl: get enemy object offset and leave
    x = M(ObjectOffset);
    return;
}

//------------------------------------------------------------------------

        //------------------------------------------------------------------------

void SMBEngine::FindPlayerAction()
{
            ProcessPlayerAction(); // find proper offset to graphics table by player's actions
            PlayerGfxProcessing(); // draw player, then process for fireball throwing
            return;
}

//------------------------------------------------------------------------

void SMBEngine::PlayerGfxProcessing()
{
    writeData(PlayerGfxOffset, a); // store offset to graphics table here
    a = 0x04;
    RenderPlayerSub(); // draw player based on offset loaded
    ChkForPlayerAttrib(); // set horizontal flip bits as necessary
    if (M(FireballThrowingTimer) == 0)
    {
        PlayerOffscreenChk(); // if fireball throw timer not set, skip to the end
        return;
    }
    y = 0x00; // set value to initialize by default
    a = M(PlayerAnimTimer); // get animation frame timer
    if (a >= M(FireballThrowingTimer))
    {
        writeData(FireballThrowingTimer, 0x00); // initialize fireball throw timer
        PlayerOffscreenChk(); // if animation frame timer => fireball throw timer skip to end
        return;
    }
    writeData(FireballThrowingTimer, a); // otherwise store animation timer into fireball throw timer
    // load offset for throwing
    // get offset to graphics table
    writeData(PlayerGfxOffset, M(PlayerGfxTblOffsets + 0x07)); // store it for use later
    y = 0x04; // set to update four sprite rows by default
    a = M(Player_X_Speed) | M(Left_Right_Buttons); // check for horizontal speed or left/right button press
    if (a != 0)
    { // if no speed or button press, branch using set value in Y
        y = 0x03; // otherwise set to update only three sprite rows
    } // SUpdR: save in A for use
    a = y;
    RenderPlayerSub(); // in sub, draw player object again
    PlayerOffscreenChk();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PlayerOffscreenChk()
{
    bool shiftedBit = false;

    // get player's offscreen bits
    a = M(Player_OffscreenBits) >> 4; // move vertical bits to low nybble
    writeData(0x00, a); // store here
    x = 0x03; // check all four rows of player sprites
    a = M(Player_SprDataOffset); // get player's sprite data offset
    a += 0x18; // add 24 bytes to start at bottom row
    y = a; // set as offset here

    do // PROfsLoop: load offscreen Y coordinate just in case
    {
        a = 0xf8;
        shiftedBit = (M(0x00) & 0x01) != 0;
        M(0x00) >>= 1; // take the bit
        if (shiftedBit)
        { // if bit not set, skip, do not move sprites
            DumpTwoSpr(); // otherwise dump offscreen Y coordinate into sprite data
        } // NPROffscr
        a = y;
        a -= 0x08; // next row up
        y = a;
        --x; // decrement row counter
    } while ((x & 0x80) == 0); // do this until all sprite rows are checked
    return; // then we are done!
}

//------------------------------------------------------------------------

void SMBEngine::PlayerGfxHandler()
{
    // if player's injured invincibility timer
    if (M(InjuryTimer) != 0)
    { // not set, skip checkpoint and continue code
        a = M(FrameCounter) >> 1; // otherwise check frame counter and branch
        if ((M(FrameCounter) & 0x01) != 0)
            return; // to leave on every other frame (when d0 is set)
    } // CntPl: if executing specific game engine routine,
    if (M(GameEngineSubroutine) != 0x0b)
    {
        // if grow/shrink flag set
        if (M(PlayerChangeSizeFlag) == 0)
        { // then branch to some other code
            // if swimming flag set, branch to
            if (M(SwimmingFlag) == 0)
            {
                FindPlayerAction(); // different part, do not return
                return;
            }
            if (M(Player_State) == 0x00)
            {
                FindPlayerAction(); // branch and do not return
                return;
            }
            FindPlayerAction(); // otherwise jump and return
            a = M(FrameCounter) & 0b00000100; // check frame counter for d2 set (8 frames every
            if (a != 0)
                return; // eighth frame), and branch if set to leave
            x = a; // initialize X to zero
            y = M(Player_SprDataOffset); // get player sprite data offset
            if ((M(PlayerFacingDir) & 0x01) == 0) // get player's facing direction
            { // if player facing to the right, use current offset
                ++y;
                ++y; // otherwise move to next OAM data
                ++y;
                ++y;
            } // SwimKT: check player's size
            if (M(PlayerSize) != 0)
            { // if big, use first tile
                a = M(Sprite_Tilenumber + 24 + y); // check tile number of seventh/eighth sprite
                if (a == M(SwimTileRepOffset))
                    return; // if spr7/spr8 tile number = value, branch to leave
                ++x; // otherwise increment X for second tile
            } // BigKTS: overwrite tile number in sprite 7/8
            a = M(SwimKickTileNum + x);
            writeData(Sprite_Tilenumber + 24 + y, a); // to animate player's feet when swimming

            return; // ExPGH: then leave

        } // DoChangeSize
        HandleChangeSize(); // find proper offset to graphics table for grow/shrink
        PlayerGfxProcessing(); // draw player, then process for fireball throwing
        return;
    } // PlayerKilled
    y = 0x0e; // load offset for player killed
    a = M(PlayerGfxTblOffsets + 0x0e); // get offset to graphics table
    PlayerGfxProcessing();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::JumpspringHandler()
{
    GetEnemyOffscreenBits(); // get offscreen information
    // check master timer control
    if (M(TimerControl) != 0)
        goto DrawJSpr; // branch to last section if set
    a = M(JumpspringAnimCtrl); // check jumpspring frame control
    if (a == 0)
        goto DrawJSpr; // branch to last section if not set
    y = a;
    --y; // subtract one from frame control,
    a = y; // the only way a poor nmos 6502 can
    a &= 0b00000010; // mask out all but d1, original value still in Y
    if (a == 0)
    { // if set, branch to move player up
        ++M(Player_Y_Position);
        ++M(Player_Y_Position); // move player's vertical position down two pixels
    } // DownJSpr: move player's vertical position up two pixels
    else // skip to next part
    {
        --M(Player_Y_Position);
        --M(Player_Y_Position);
    } // PosJSpr: get permanent vertical position
    a = M(Jumpspring_FixedYPos + x);
    a += M(Jumpspring_Y_PosData + y); // add value using frame control as offset
    writeData(Enemy_Y_Position + x, a); // store as new vertical position
    if (y < 0x01)
        goto BounceJS; // if offset not yet at third frame ($01), skip to next part
    a = M(A_B_Buttons) & A_Button; // check saved controller bits for A button press
    if (a == 0)
        goto BounceJS; // skip to next part if A not pressed
    a &= M(PreviousA_B_Buttons); // check for A button pressed in previous frame
    if (a != 0)
        goto BounceJS; // skip to next part if so
    a = 0xf4;
    writeData(JumpspringForce, 0xf4); // otherwise write new jumpspring force here

BounceJS: // check frame control offset again
    if (y != 0x03)
        goto DrawJSpr; // skip to last part if not yet at fifth frame ($03)
    writeData(Player_Y_Speed, M(JumpspringForce)); // store jumpspring force as player's new vertical speed
    a = 0x00;
    writeData(JumpspringAnimCtrl, 0x00); // initialize jumpspring frame control

DrawJSpr: // get jumpspring's relative coordinates
    RelativeEnemyPosition();
    EnemyGfxHandler(); // draw jumpspring
    OffscreenBoundsCheck(); // check to see if we need to kill it
    a = M(JumpspringAnimCtrl); // if frame control at zero, don't bother
    if (a == 0)
        return; // trying to animate it, just leave
    a = M(JumpspringTimer);
    if (a != 0)
        return; // if jumpspring timer not expired yet, leave
    a = 0x04;
    writeData(JumpspringTimer, 0x04); // otherwise initialize jumpspring timer
    ++M(JumpspringAnimCtrl); // increment frame control to animate jumpspring

    return; // ExJSpring: leave
}

//------------------------------------------------------------------------

void SMBEngine::BlockObjectsCore()
{
    a = M(Block_State + x); // get state of block object
    if (a == 0)
        goto UpdSte; // if not set, branch to leave
    a &= 0x0f; // mask out high nybble
    pha(); // push to stack
    y = a; // put in Y for now
    a = x;
    a += 0x09; // add 9 bytes to offset (note two block objects are created
    x = a; // when using brick chunks, but only one offset for both)
    --y; // decrement Y to check for solid block state
    if (y != 0)
    { // branch if found, otherwise continue for brick chunks
        ImposeGravityBlock(); // do sub to impose gravity on one block object object
        MoveObjectHorizontally(); // do another sub to move horizontally
        a = x;
        a += 0x02;
        x = a;
        ImposeGravityBlock(); // do sub to impose gravity on other block object
        MoveObjectHorizontally(); // do another sub to move horizontally
        x = M(ObjectOffset); // get block object offset used for both
        RelativeBlockPosition(); // get relative coordinates
        GetBlockOffscreenBits(); // get offscreen information
        DrawBrickChunks(); // draw the brick chunks
        pla(); // get lower nybble of saved state
        y = M(Block_Y_HighPos + x); // check vertical high byte of block object
        if (y == 0)
            goto UpdSte; // if above the screen, branch to kill it
        pha(); // otherwise save state back into stack
        if (0xf0 < M(Block_Y_Position + 2 + x))
        { // to the bottom of the screen, and branch if not
            writeData(Block_Y_Position + 2 + x, 0xf0); // otherwise set offscreen coordinate
        } // ChkTop: get top block object's vertical coordinate
        a = M(Block_Y_Position + x);
        pla(); // pull block object state from stack
        if (M(Block_Y_Position + x) < 0xf0)
            goto UpdSte; // if not, branch to save state
        if (M(Block_Y_Position + x) >= 0xf0)
            goto KillBlock; // otherwise do unconditional branch to kill it
    } // BouncingBlockHandler
    ImposeGravityBlock(); // do sub to impose gravity on block object
    x = M(ObjectOffset); // get block object offset
    RelativeBlockPosition(); // get relative coordinates
    GetBlockOffscreenBits(); // get offscreen information
    DrawBlock(); // draw the block
    // get vertical coordinate
    a = M(Block_Y_Position + x) & 0x0f; // mask out high nybble
    pla(); // pull state from stack
    if ((M(Block_Y_Position + x) & 0x0f) >= 0x05)
        goto UpdSte; // if still above amount, not time to kill block yet, thus branch
    a = 0x01;
    writeData(Block_RepFlag + x, 0x01); // otherwise set flag to replace metatile

KillBlock: // if branched here, nullify object state
    a = 0x00;

UpdSte: // store contents of A in block object state
    writeData(Block_State + x, a);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RunRetainerObj()
{
    GetEnemyOffscreenBits();
    RelativeEnemyPosition();
    EnemyGfxHandler();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawPowerUp()
{
    y = M(Enemy_SprDataOffset + 5); // get power-up's sprite data offset
    a = M(Enemy_Rel_YPos); // get relative vertical coordinate
    a += 0x08; // add eight pixels
    writeData(0x02, a); // store result here
    // get relative horizontal coordinate
    writeData(0x05, M(Enemy_Rel_XPos)); // store here
    x = M(PowerUpType); // get power-up type
    // get attribute data for power-up type
    a = M(PowerUpAttributes + x) | M(Enemy_SprAttrib + 5); // add background priority bit if set
    writeData(0x04, a); // store attributes here
    a = x;
    pha(); // save power-up type to the stack
    a <<= 1;
    a <<= 1; // multiply by four to get proper offset
    x = a; // use as X
    a = 0x01;
    writeData(0x07, 0x01); // set counter here to draw two rows of sprite object
    writeData(0x03, 0x01); // init d1 of flip control

    do // PUpDrawLoop
    {
        // load left tile of power-up object
        writeData(0x00, M(PowerUpGfxTable + x));
        a = M(PowerUpGfxTable + 1 + x); // load right tile
        DrawOneSpriteRow(); // branch to draw one row of our power-up object
        --M(0x07); // decrement counter
    } while ((M(0x07) & 0x80) == 0); // branch until two rows are drawn
    y = M(Enemy_SprDataOffset + 5); // get sprite data offset again
    pla(); // pull saved power-up type from the stack
    if (a == 0)
        goto PUpOfs; // if regular mushroom, branch, do not change colors or flip
    if (a == 0x03)
        goto PUpOfs; // if 1-up mushroom, branch, do not change colors or flip
    writeData(0x00, a); // store power-up type here now
    // get frame counter
    a = M(FrameCounter) >> 1; // divide by 2 to change colors every two frames
    a &= 0b00000011; // mask out all but d1 and d0 (previously d2 and d1)
    a |= M(Enemy_SprAttrib + 5); // add background priority bit if any set
    writeData(Sprite_Attributes + y, a); // set as new palette bits for top left and
    writeData(Sprite_Attributes + 4 + y, a); // top right sprites for fire flower and star
    x = M(0x00);
    --x; // check power-up type for fire flower
    if (x != 0)
    { // if found, skip this part
        writeData(Sprite_Attributes + 8 + y, a); // otherwise set new palette bits  for bottom left
        writeData(Sprite_Attributes + 12 + y, a); // and bottom right sprites as well for star only
    } // FlipPUpRightSide
    a = M(Sprite_Attributes + 4 + y) | 0b01000000; // set horizontal flip bit for top right sprite
    writeData(Sprite_Attributes + 4 + y, a);
    a = M(Sprite_Attributes + 12 + y) | 0b01000000; // set horizontal flip bit for bottom right sprite
    writeData(Sprite_Attributes + 12 + y, a); // note these are only done for fire flower and star power-ups

PUpOfs: // jump to check to see if power-up is offscreen at all, then leave
    SprObjectOffscrChk();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::EnemyGfxHandler()
{
    // get enemy object vertical position
    writeData(0x02, M(Enemy_Y_Position + x));
    // get enemy object horizontal position
    writeData(0x05, M(Enemy_Rel_XPos)); // relative to screen
    writeData(0xeb, M(Enemy_SprDataOffset + x)); // get sprite data offset
    writeData(VerticalFlipFlag, 0x00); // initialize vertical flip flag by default
    writeData(0x03, M(Enemy_MovingDir + x)); // get enemy object moving direction
    writeData(0x04, M(Enemy_SprAttrib + x)); // get enemy object sprite attributes
    a = M(Enemy_ID + x);
    if (a != PiranhaPlant)
        goto CheckForRetainerObj; // if not, branch
    if ((M(PiranhaPlant_Y_Speed + x) & 0x80) != 0)
        goto CheckForRetainerObj; // if piranha plant moving upwards, branch
    y = M(EnemyFrameTimer + x);
    if (y == 0)
        goto CheckForRetainerObj; // if timer for movement expired, branch
    return; // if all conditions fail, leave

//------------------------------------------------------------------------

CheckForRetainerObj:
    a = M(Enemy_State + x); // store enemy state
    writeData(0xed, a);
    a &= 0b00011111; // nullify all but 5 LSB and use as Y
    y = a;
    a = M(Enemy_ID + x); // check for mushroom retainer/princess object
    if (a == RetainerObject)
    { // if not found, branch
        y = 0x00; // if found, nullify saved state in Y
        // set value that will not be used
        writeData(0x03, 0x01);
        a = 0x15; // set value $15 as code for mushroom retainer/princess object
    } // CheckForBulletBillCV
    if (a == BulletBill_CannonVar)
    { // if not found, branch again
        --M(0x02); // decrement saved vertical position
        a = 0x03;
        // get timer for enemy object
        if (M(EnemyFrameTimer + x) != 0)
        { // if expired, do not set priority bit
            a = 0b00100011; // otherwise do so
        } // SBBAt: set new sprite attributes
        writeData(0x04, a);
        y = 0x00; // nullify saved enemy state both in Y and in
        writeData(0xed, 0x00); // memory location here
        a = 0x08; // set specific value to unconditionally branch once
    } // CheckForJumpspring
    if (a == JumpspringObject)
    {
        y = 0x03; // set enemy state -2 MSB here for jumpspring object
        x = M(JumpspringAnimCtrl); // get current frame number for jumpspring object
        a = M(JumpspringFrameOffsets + x); // load data using frame number as offset
    } // CheckForPodoboo
    writeData(0xef, a); // store saved enemy object value here
    writeData(0xec, y); // and Y here (enemy state -2 MSB if not changed)
    x = M(ObjectOffset); // get enemy object offset
    if (a != 0x0c)
        goto CheckBowserGfxFlag; // branch if not found
    // if moving upwards, branch
    if ((M(Enemy_Y_Speed + x) & 0x80) != 0)
        goto CheckBowserGfxFlag;
    ++M(VerticalFlipFlag); // otherwise, set flag for vertical flip

CheckBowserGfxFlag:
    a = M(BowserGfxFlag); // if not drawing bowser at all, skip to something else
    if (a != 0)
    {
        y = 0x16; // if set to 1, draw bowser's front
        if (a != 0x01)
        {
            y = 0x17; // otherwise draw bowser's rear
        } // SBwsrGfxOfs
        writeData(0xef, y);
    } // CheckForGoomba
    y = M(0xef); // check value for goomba object
    if (y != Goomba)
        goto CheckBowserFront; // branch if not found
    a = M(Enemy_State + x);
    if (a >= 0x02)
    { // if not defeated, go ahead and animate
        x = 0x04; // if defeated, write new value here
        writeData(0xec, 0x04);
    } // GmbaAnim: check for d5 set in enemy object state
    a &= 0b00100000;
    a |= M(TimerControl); // or timer disable flag set
    if (a != 0)
        goto CheckBowserFront; // if either condition true, do not animate goomba
    a = M(FrameCounter) & 0b00001000; // check for every eighth frame
    if (a != 0)
        goto CheckBowserFront;
    a = M(0x03) ^ 0b00000011; // invert bits to flip horizontally every eight frames
    writeData(0x03, a); // leave alone otherwise

CheckBowserFront:
    // load sprite attribute using enemy object
    a = M(EnemyAttributeData + y) | M(0x04); // as offset, and add to bits already loaded
    writeData(0x04, a);
    // load value based on enemy object as offset
    x = M(EnemyGfxTableOffsets + y); // save as X
    y = M(0xec); // get previously saved value
    a = M(BowserGfxFlag);
    if (a != 0)
    { // if not drawing bowser object at all, skip all of this
        if (a == 0x01)
        { // if not drawing front part, branch to draw the rear part
            // check bowser's body control bits
            if ((M(BowserBodyControls) & 0x80) != 0)
            { // branch if d7 not set (control's bowser's mouth)
                x = 0xde; // otherwise load offset for second frame
            } // ChkFrontSte: check saved enemy state
            a = M(0xed) & 0b00100000; // if bowser not defeated, do not set flag
            if (a == 0)
                goto DrawBowser;

FlipBowserOver:
            writeData(VerticalFlipFlag, x); // set vertical flip flag to nonzero

DrawBowser:
            DrawEnemyObject(); // draw bowser's graphics now
            return;
        } // CheckBowserRear
        // check bowser's body control bits
        a = M(BowserBodyControls) & 0x01;
        if (a != 0)
        { // branch if d0 not set (control's bowser's feet)
            x = 0xe4; // otherwise load offset for second frame
        } // ChkRearSte: check saved enemy state
        a = M(0xed) & 0b00100000; // if bowser not defeated, do not set flag
        if (a == 0)
            goto DrawBowser;
        a = M(0x02); // subtract 16 pixels from
        a -= 0x10;
        writeData(0x02, a);
        goto FlipBowserOver; // jump to set vertical flip flag
    } // CheckForSpiny
    if (x == 0x24)
    { // if not found, branch
        if (y == 0x05)
        { // otherwise branch
            x = 0x30; // set to spiny egg offset
            writeData(0x03, 0x02); // set enemy direction to reverse sprites horizontally
            a = 0x05;
            writeData(0xec, 0x05); // set enemy state
        } // NotEgg: skip a big chunk of this if we found spiny but not in egg
        CheckForHammerBro();
        return;
    } // CheckForLakitu
    if (x == 0x90)
    { // branch if not loaded
        a = M(0xed) & 0b00100000; // check for d5 set in enemy state
        if (a != 0)
            goto NoLAFr; // branch if set
        if (M(FrenzyEnemyTimer) >= 0x10)
            goto NoLAFr; // branch if not
        x = 0x96; // if d6 not set and timer in range, load alt frame for lakitu

NoLAFr: // skip this next part if we found lakitu but alt frame not needed
        CheckDefeatedState();
        return;
    } // CheckUpsideDownShell
    // check for enemy object => $04
    if (M(0xef) >= 0x04)
        goto CheckRightSideUpShell; // branch if true
    if (y < 0x02)
        goto CheckRightSideUpShell; // branch if enemy state < $02
    x = 0x5a; // set for upside-down koopa shell by default
    if (M(0xef) != BuzzyBeetle)
        goto CheckRightSideUpShell;
    x = 0x7e; // set for upside-down buzzy beetle shell if found
    ++M(0x02); // increment vertical position by one pixel

CheckRightSideUpShell:
    // check for value set here
    if (M(0xec) != 0x04)
    {
        CheckForHammerBro(); // enemy state => $02 but not = $04, leave shell upside-down
        return;
    }
    x = 0x72; // set right-side up buzzy beetle shell by default
    ++M(0x02); // increment saved vertical position by one pixel
    y = M(0xef);
    if (y != BuzzyBeetle)
    { // branch if found
        x = 0x66; // change to right-side up koopa shell if not found
        ++M(0x02); // and increment saved vertical position again
    } // CheckForDefdGoomba
    if (y != Goomba)
    {
        CheckForHammerBro(); // failed buzzy beetle object test)
        return;
    }
    x = 0x54; // load for regular goomba
    // note that this only gets performed if enemy state => $02
    a = M(0xed) & 0b00100000; // check saved enemy state for d5 set
    if (a != 0)
    {
        CheckForHammerBro(); // branch if set
        return;
    }
    x = 0x8a; // load offset for defeated goomba
    --M(0x02); // set different value and decrement saved vertical position
    CheckForHammerBro();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::CheckForHammerBro()
{
    y = M(ObjectOffset);
    // check for hammer bro object
    if (M(0xef) == HammerBro)
    { // branch if not found
        a = M(0xed);
        if (a == 0)
            goto CheckToAnimateEnemy; // branch if not in normal enemy state
        a &= 0b00001000;
        if (a == 0)
        {
            CheckDefeatedState(); // if d3 not set, branch further away
            return;
        }
        x = 0xb4; // otherwise load offset for different frame
        if (x != 0)
            goto CheckToAnimateEnemy; // unconditional branch
    } // CheckForBloober
    if (x == 0x48)
        goto CheckToAnimateEnemy; // branch if found
    a = M(EnemyIntervalTimer + y);
    if (a >= 0x05)
    {
        CheckDefeatedState(); // branch if some timer is above a certain point
        return;
    }
    if (x != 0x3c)
        goto CheckToAnimateEnemy; // branch if not found this time
    if (a == 0x01)
    {
        CheckDefeatedState(); // branch if timer is set to certain point
        return;
    }
    ++M(0x02); // increment saved vertical coordinate three pixels
    ++M(0x02);
    ++M(0x02);
    goto CheckAnimationStop; // and do something else

CheckToAnimateEnemy:
    a = M(0xef); // check for specific enemy objects
    if (a == Goomba)
    {
        CheckDefeatedState(); // branch if goomba
        return;
    }
    if (a == 0x08)
    {
        CheckDefeatedState(); // branch if bullet bill (note both variants use $08 here)
        return;
    }
    if (a == Podoboo)
    {
        CheckDefeatedState(); // branch if podoboo
        return;
    }
    if (a >= 0x18)
    {
        CheckDefeatedState();
        return;
    }
    y = 0x00;
    if (a == 0x15)
    { // which uses different code here, branch if not found
        y = 0x01; // residual instruction
        // are we on world 8?
        if (M(WorldNumber) >= World8)
        {
            CheckDefeatedState(); // if so, leave the offset alone (use princess)
            return;
        }
        x = 0xa2; // otherwise, set for mushroom retainer object instead
        a = 0x03; // set alternate state here
        writeData(0xec, 0x03);
        if (a != 0)
        {
            CheckDefeatedState(); // unconditional branch
            return;
        }
    } // CheckForSecondFrame
    // load frame counter
    a = M(FrameCounter) & M(EnemyAnimTimingBMask + y); // mask it (partly residual, one byte not ever used)
    if (a != 0)
    {
        CheckDefeatedState(); // branch if timing is off
        return;
    }

CheckAnimationStop:
    // check saved enemy state
    a = M(0xed) & 0b10100000; // for d7 or d5, or check for timers stopped
    a |= M(TimerControl);
    if (a != 0)
    {
        CheckDefeatedState(); // if either condition true, branch
        return;
    }
    a = x;
    a += 0x06; // add $06 to current enemy offset
    x = a; // to animate various enemy objects
    CheckDefeatedState();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::CheckDefeatedState()
{
    // check saved enemy state
    a = M(0xed) & 0b00100000; // for d5 set
    if (a == 0)
    {
        DrawEnemyObject(); // branch if not set
        return;
    }
    if (M(0xef) < 0x04)
    {
        DrawEnemyObject(); // branch if less
        return;
    }
    writeData(VerticalFlipFlag, 0x01); // set vertical flip flag
    y = 0x00;
    writeData(0xec, 0x00); // init saved value here
    DrawEnemyObject();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawEnemyObject()
{
    y = M(0xeb); // load sprite data offset
    DrawEnemyObjRow(); // draw six tiles of data
    DrawEnemyObjRow(); // into sprite data
    DrawEnemyObjRow();
    x = M(ObjectOffset); // get enemy object offset
    y = M(Enemy_SprDataOffset + x); // get sprite data offset
    if (M(0xef) == 0x08)
    { // for bullet bill, branch if not found

SkipToOffScrChk:
        SprObjectOffscrChk(); // jump if found
        return;
    } // CheckForVerticalFlip
    // check if vertical flip flag is set here
    if (M(VerticalFlipFlag) != 0)
    { // branch if not
        // get attributes of first sprite we dealt with
        a = M(Sprite_Attributes + y) | 0b10000000; // set bit for vertical flip
        ++y;
        ++y; // increment two bytes so that we store the vertical flip
        DumpSixSpr(); // in attribute bytes of enemy obj sprite data
        --y;
        --y; // now go back to the Y coordinate offset
        a = y;
        x = a; // give offset to X
        a = M(0xef);
        if (a == HammerBro)
            goto FlipEnemyVertically;
        if (a == Lakitu)
            goto FlipEnemyVertically; // branch for hammer bro or lakitu
        if (a >= 0x15)
            goto FlipEnemyVertically; // also branch if enemy object => $15
        a = x;
        a += 0x08; // if not selected objects or => $15, set
        x = a; // offset in X for next row

FlipEnemyVertically:
        a = M(Sprite_Tilenumber + x); // load first or second row tiles
        pha(); // and save tiles to the stack
        a = M(Sprite_Tilenumber + 4 + x);
        pha();
        // exchange third row tiles
        writeData(Sprite_Tilenumber + x, M(Sprite_Tilenumber + 16 + y)); // with first or second row tiles
        writeData(Sprite_Tilenumber + 4 + x, M(Sprite_Tilenumber + 20 + y));
        pla(); // pull first or second row tiles from stack
        writeData(Sprite_Tilenumber + 20 + y, a); // and save in third row
        pla();
        writeData(Sprite_Tilenumber + 16 + y, a);
    } // CheckForESymmetry
    // are we drawing bowser at all?
    if (M(BowserGfxFlag) != 0)
        goto SkipToOffScrChk; // branch if so
    a = M(0xef);
    x = M(0xec); // get alternate enemy state
    if (a == 0x05)
    {
        SprObjectOffscrChk(); // jump if found
        return;
    } // ContES: check for bloober object
    if (a == Bloober)
        goto MirrorEnemyGfx;
    if (a == PiranhaPlant)
        goto MirrorEnemyGfx;
    if (a == Podoboo)
        goto MirrorEnemyGfx; // branch if either of three are found
    if (a == Spiny)
    { // branch closer if not found
        if (x != 0x05)
            goto CheckToMirrorLakitu; // branch if not an egg, otherwise
    } // ESRtnr: check for princess/mushroom retainer object
    if (a == 0x15)
    {
        a = 0x42; // set horizontal flip on bottom right sprite
        writeData(Sprite_Attributes + 20 + y, 0x42); // note that palette bits were already set earlier
    } // SpnySC: if alternate enemy state set to 1 or 0, branch
    if (x < 0x02)
        goto CheckToMirrorLakitu;

MirrorEnemyGfx:
    // if enemy object is bowser, skip all of this
    if (M(BowserGfxFlag) != 0)
        goto CheckToMirrorLakitu;
    // load attribute bits of first sprite
    a = M(Sprite_Attributes + y) & 0b10100011;
    writeData(Sprite_Attributes + y, a); // save vertical flip, priority, and palette bits
    writeData(Sprite_Attributes + 8 + y, a); // in left sprite column of enemy object OAM data
    writeData(Sprite_Attributes + 16 + y, a);
    a |= 0b01000000; // set horizontal flip
    if (x == 0x05)
    { // if alternate state not set to $05, branch
        a |= 0b10000000; // otherwise set vertical flip
    } // EggExc: set bits of right sprite column
    writeData(Sprite_Attributes + 4 + y, a);
    writeData(Sprite_Attributes + 12 + y, a); // of enemy object sprite data
    writeData(Sprite_Attributes + 20 + y, a);
    if (x != 0x04)
        goto CheckToMirrorLakitu; // branch if not $04
    // get second row left sprite attributes
    a = M(Sprite_Attributes + 8 + y) | 0b10000000;
    writeData(Sprite_Attributes + 8 + y, a); // store bits with vertical flip in
    writeData(Sprite_Attributes + 16 + y, a); // second and third row left sprites
    a |= 0b01000000;
    writeData(Sprite_Attributes + 12 + y, a); // store with horizontal and vertical flip in
    writeData(Sprite_Attributes + 20 + y, a); // second and third row right sprites

CheckToMirrorLakitu:
    // check for lakitu enemy object
    if (M(0xef) == Lakitu)
    { // branch if not found
        if (M(VerticalFlipFlag) == 0)
        { // branch if vertical flip flag not set
            // save vertical flip and palette bits
            a = M(Sprite_Attributes + 16 + y) & 0b10000001; // in third row left sprite
            writeData(Sprite_Attributes + 16 + y, a);
            // set horizontal flip and palette bits
            a = M(Sprite_Attributes + 20 + y) | 0b01000001; // in third row right sprite
            writeData(Sprite_Attributes + 20 + y, a);
            x = M(FrenzyEnemyTimer); // check timer
            if (x >= 0x10)
            {
                SprObjectOffscrChk(); // branch if timer has not reached a certain range
                return;
            }
            writeData(Sprite_Attributes + 12 + y, a); // otherwise set same for second row right sprite
            a &= 0b10000001;
            writeData(Sprite_Attributes + 8 + y, a); // preserve vertical flip and palette bits for left sprite
            if (x < 0x10)
            {
                SprObjectOffscrChk(); // unconditional branch
                return;
            }
        } // NVFLak: get first row left sprite attributes
        a = M(Sprite_Attributes + y) & 0b10000001;
        writeData(Sprite_Attributes + y, a); // save vertical flip and palette bits
        // get first row right sprite attributes
        a = M(Sprite_Attributes + 4 + y) | 0b01000001; // set horizontal flip and palette bits
        writeData(Sprite_Attributes + 4 + y, a); // note that vertical flip is left as-is
    } // CheckToMirrorJSpring
    // check for jumpspring object (any frame)
    if (M(0xef) < 0x18)
    {
        SprObjectOffscrChk(); // branch if not jumpspring object at all
        return;
    }
    writeData(Sprite_Attributes + 8 + y, 0x82); // set vertical flip and palette bits of
    writeData(Sprite_Attributes + 16 + y, 0x82); // second and third row left sprites
    a = 0b11000010;
    writeData(Sprite_Attributes + 12 + y, 0b11000010); // set, in addition to those, horizontal flip
    writeData(Sprite_Attributes + 20 + y, 0b11000010); // for second and third row right sprites
    SprObjectOffscrChk();
    return;
}



//------------------------------------------------------------------------

void SMBEngine::DrawBlock()
{
    // get relative vertical coordinate of block object
    writeData(0x02, M(Block_Rel_YPos)); // store here
    // get relative horizontal coordinate of block object
    writeData(0x05, M(Block_Rel_XPos)); // store here
    writeData(0x04, 0x03); // set attribute byte here
    a = 0x01;
    writeData(0x03, 0x01); // set horizontal flip bit here (will not be used)
    y = M(Block_SprDataOffset + x); // get sprite data offset
    x = 0x00; // reset X for use as offset to tile data

    do // DBlkLoop: get left tile number
    {
        writeData(0x00, M(DefaultBlockObjTiles + x)); // set here
        a = M(DefaultBlockObjTiles + 1 + x); // get right tile number
        DrawOneSpriteRow(); // do sub to write tile numbers to first row of sprites
    } while (x != 0x04); // and loop back until all four sprites are done
    x = M(ObjectOffset); // get block object offset
    y = M(Block_SprDataOffset + x); // get sprite data offset
    if (M(AreaType) != 0x01)
    { // if found, branch to next part
        a = 0x86;
        writeData(Sprite_Tilenumber + y, 0x86); // otherwise remove brick tiles with lines
        writeData(Sprite_Tilenumber + 4 + y, 0x86); // and replace then with lineless brick tiles
    } // ChkRep: check replacement metatile
    if (M(Block_Metatile + x) == 0xc4)
    { // branch ahead to use current graphics
        a = 0x87; // set A for used block tile
        ++y; // increment Y to write to tile bytes
        DumpFourSpr(); // do sub to dump into all four sprites
        --y; // return Y to original offset
        a = 0x03; // set palette bits
        x = M(AreaType);
        --x; // check for ground level type area again
        if (x != 0)
        { // if found, use current palette bits
            a = 0x01; // otherwise set to $01
        } // SetBFlip: put block object offset back in X
        x = M(ObjectOffset);
        writeData(Sprite_Attributes + y, a); // store attribute byte as-is in first sprite
        a |= 0b01000000;
        writeData(Sprite_Attributes + 4 + y, a); // set horizontal flip bit for second sprite
        a |= 0b10000000;
        writeData(Sprite_Attributes + 12 + y, a); // set both flip bits for fourth sprite
        a &= 0b10000011;
        writeData(Sprite_Attributes + 8 + y, a); // set vertical flip bit for third sprite
    } // BlkOffscr: get offscreen bits for block object
    a = M(Block_OffscreenBits);
    pha(); // save to stack
    a &= 0b00000100; // check to see if d2 in offscreen bits are set
    if (a != 0)
    { // if not set, branch, otherwise move sprites offscreen
        a = 0xf8; // move offscreen two OAMs
        writeData(Sprite_Y_Position + 4 + y, 0xf8); // on the right side
        writeData(Sprite_Y_Position + 12 + y, 0xf8);
    } // PullOfsB: pull offscreen bits from stack
    pla();
    ChkLeftCo();
    return;
}

//------------------------------------------------------------------------

// check to see if d3 in offscreen bits are set
void SMBEngine::ChkLeftCo()
{
    a &= 0b00001000;
    if (a == 0)
    { // if not set, branch, otherwise move sprites offscreen
        return;
    }
    MoveColOffscreen();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::DrawBrickChunks()
{
    uint32_t wide = 0;
    bool carry = false;

    // set palette bits here
    writeData(0x00, 0x02);
    a = 0x75; // set tile number for ball (something residual, likely)
    if (M(GameEngineSubroutine) != 0x05)
    { // use palette and tile number assigned
        // otherwise set different palette bits
        writeData(0x00, 0x03);
        a = 0x84; // and set tile number for brick chunks
    } // DChunks: get OAM data offset
    y = M(Block_SprDataOffset + x);
    ++y; // increment to start with tile bytes in OAM
    DumpFourSpr(); // do sub to dump tile number into all four sprites
    a = M(FrameCounter); // get frame counter
    a <<= 1;
    a <<= 1;
    a <<= 1; // move low nybble to high
    a <<= 1;
    a &= 0xc0; // get what was originally d3-d2 of low nybble
    a |= M(0x00); // add palette bits
    ++y; // increment offset for attribute bytes
    DumpFourSpr(); // do sub to dump attribute data into all four sprites
    --y;
    --y; // decrement offset to Y coordinate
    a = M(Block_Rel_YPos); // get first block object's relative vertical coordinate
    DumpTwoSpr(); // do sub to dump current Y coordinate into two sprites
    // get first block object's relative horizontal coordinate
    writeData(Sprite_X_Position + y, M(Block_Rel_XPos)); // save into X coordinate of first sprite
    a = M(Block_Orig_XPos + x); // get original horizontal coordinate
    a -= M(ScreenLeft_X_Pos); // subtract coordinate of left side from original coordinate
    writeData(0x00, a); // store result as relative horizontal coordinate of original
    carry = a >= M(Block_Rel_XPos); // the borrow this subtract leaves is read by the add below
    a -= M(Block_Rel_XPos); // get difference of relative positions of original - current
    wide = a + M(0x00) + (carry ? 1 : 0); // add original relative position to result
    a = (uint8_t)(LOBYTE(wide) + 0x06 + HIBYTE(wide)); // plus 6 pixels, and this add's own carry, to position second brick chunk correctly
    writeData(Sprite_X_Position + 4 + y, a); // save into X coordinate of second sprite
    a = M(Block_Rel_YPos + 1); // get second block object's relative vertical coordinate
    writeData(Sprite_Y_Position + 8 + y, a);
    writeData(Sprite_Y_Position + 12 + y, a); // dump into Y coordinates of third and fourth sprites
    // get second block object's relative horizontal coordinate
    writeData(Sprite_X_Position + 8 + y, M(Block_Rel_XPos + 1)); // save into X coordinate of third sprite
    a = M(0x00); // use original relative horizontal position
    carry = a >= M(Block_Rel_XPos + 1); // the borrow this subtract leaves is read by the add below
    a -= M(Block_Rel_XPos + 1); // get difference of relative positions of original - current
    wide = a + M(0x00) + (carry ? 1 : 0); // add original relative position to result
    a = (uint8_t)(LOBYTE(wide) + 0x06 + HIBYTE(wide)); // plus 6 pixels, and this add's own carry, to position fourth brick chunk correctly
    writeData(Sprite_X_Position + 12 + y, a); // save into X coordinate of fourth sprite
    a = M(Block_OffscreenBits); // get offscreen bits for block object
    ChkLeftCo(); // do sub to move left half of sprites offscreen if necessary
    if ((M(Block_OffscreenBits) & 0x80) != 0) // check d7 of the offscreen bits
    { // if d7 not set, branch to last part
        a = 0xf8;
        DumpTwoSpr(); // otherwise move top sprites offscreen
    } // ChnkOfs: if relative position on left side of screen,
    a = M(0x00);
    if ((a & 0x80) == 0)
        return; // go ahead and leave
    a = M(Sprite_X_Position + y); // otherwise compare left-side X coordinate
    if (a < M(Sprite_X_Position + 4 + y))
        return; // branch to leave if less
    a = 0xf8; // otherwise move right half of sprites offscreen
    writeData(Sprite_Y_Position + 4 + y, 0xf8);
    writeData(Sprite_Y_Position + 12 + y, 0xf8);

    return; // ExBCDr: leave
}


//------------------------------------------------------------------------

void SMBEngine::MoveFallingPlatform()
{
    y = 0x20; // set movement amount
    SetHiMax();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveLakitu()
{
    // check lakitu's enemy state
    a = M(Enemy_State + x) & 0b00100000; // for d5 set
    if (a != 0)
    { // if not set, continue with code
        MoveD_EnemyVertically(); // otherwise jump to move defeated lakitu downwards
        return;
    } // ChkLS: if lakitu's enemy state not set at all,
    if (M(Enemy_State + x) != 0)
    { // go ahead and continue with code
        writeData(LakituMoveDirection + x, 0x00); // otherwise initialize moving direction to move to left
        writeData(EnemyFrenzyBuffer, 0x00); // initialize frenzy buffer
        a = 0x10;
        if (a != 0)
            goto SetLSpd; // load horizontal speed and do unconditional branch
    } // Fr12S
    a = Spiny;
    writeData(EnemyFrenzyBuffer, Spiny); // set spiny identifier in frenzy buffer
    y = 0x02;

    do // LdLDa: load values
    {
        writeData(0x0001 + y, M(LakituDiffAdj + y)); // store in zero page
        --y;
    } while ((y & 0x80) == 0); // do this until all values are stired
    PlayerLakituDiff(); // execute sub to set speed and create spinys

SetLSpd: // set movement speed returned from sub
    writeData(LakituMoveSpeed + x, a);
    y = 0x01; // set moving direction to right by default
    a = M(LakituMoveDirection + x) & 0x01; // get LSB of moving direction
    if (a == 0)
    { // if set, branch to the end to use moving direction
        a = M(LakituMoveSpeed + x) ^ 0xff; // get two's compliment of moving speed
        a += 0x01;
        writeData(LakituMoveSpeed + x, a); // store as new moving speed
        y = 0x02; // increment moving direction to left
    } // SetLMov: store moving direction
    writeData(Enemy_MovingDir + x, y);
    MoveEnemyHorizontally(); // move lakitu horizontally
    return;
}

//------------------------------------------------------------------------

void SMBEngine::BalancePlatform()
{
    uint32_t wide = 0;

    // check high byte of vertical position
    if (M(Enemy_Y_HighPos + x) == 0x03)
    {
        EraseEnemyObject(); // if far below screen, kill the object
        return;
    } // DoBPl: get object's state (set to $ff or other platform offset)
    a = M(Enemy_State + x);
    if ((a & 0x80) != 0)
    { // if doing other balance platform, branch to leave
        return;

    //------------------------------------------------------------------------
    } // CheckBalPlatform
    y = a; // save offset from state as Y
    // get collision flag of platform
    writeData(0x00, M(PlatformCollisionFlag + x)); // store here
    // get moving direction
    if (M(Enemy_MovingDir + x) != 0)
    {
    } // ChkForFall
    else // if set, jump here
    {
        a = 0x2d; // check if platform is above a certain point
        if (0x2d >= M(Enemy_Y_Position + x))
        { // if not, branch elsewhere
            if (y == M(0x00))
                goto MakePlatformFall; // enemy state, branch to make platforms fall
            a = 0x2f; // otherwise add 2 pixels to vertical position
            writeData(Enemy_Y_Position + x, 0x2f); // of current platform and branch elsewhere
            StopPlatforms(); // to make platforms stop
            return;

MakePlatformFall:
        } // ChkOtherForFall
        else // make platforms fall
        {
            if (0x2d >= M(Enemy_Y_Position + y))
            { // if not, branch elsewhere
                if (x == M(0x00))
                    goto MakePlatformFall; // enemy state, branch to make platforms fall
                a = 0x2f; // otherwise add 2 pixels to vertical position
                writeData(Enemy_Y_Position + y, 0x2f); // of other platform and branch elsewhere
                StopPlatforms(); // jump to stop movement and do not return
                return;
            } // ChkToMoveBalPlat
            a = M(Enemy_Y_Position + x); // save vertical position to stack
            pha();
            a = M(PlatformCollisionFlag + x); // get collision flag
            if ((a & 0x80) != 0)
            { // branch if collision
                wide = ((M(Enemy_Y_Speed + x) << 8) | M(Enemy_Y_MoveForce + x))
                     + 0x05; // add $05 to contents of moveforce, whatever they be
                writeData(0x00, LOBYTE(wide)); // store here
                a = HIBYTE(wide); // the vertical speed, plus the carry
                if ((a & 0x80) != 0)
                    goto PlatDn; // branch if moving downwards
                if (a != 0)
                    goto PlatUp; // branch elsewhere if moving upwards
                a = M(0x00);
                if (a < 0x0b)
                    goto PlatSt; // if not enough, branch to stop movement
                if (a >= 0x0b)
                    goto PlatUp; // otherwise keep branch to move upwards
            } // ColFlg: if collision flag matches
            if (a == M(ObjectOffset))
                goto PlatDn; // current enemy object offset, branch

PlatUp: // do a sub to move upwards
            MovePlatformUp();
            DoOtherPlatform(); // jump ahead to remaining code
            return;

PlatSt: // do a sub to stop movement
            StopPlatforms();
            DoOtherPlatform(); // jump ahead to remaining code
            return;

PlatDn: // do a sub to move downwards
            MovePlatformDown();

    DoOtherPlatform();
    return;

        //------------------------------------------------------------------------
        } // InitPlatformFall
        a = y; // move offset of other platform from Y to X
        x = a;
        GetEnemyOffscreenBits(); // get offscreen bits
        a = 0x06;
        SetupFloateyNumber(); // award 1000 points to player
        writeData(FloateyNum_X_Pos + x, M(Player_Rel_XPos)); // put floatey number coordinates where player is
        writeData(FloateyNum_Y_Pos + x, M(Player_Y_Position));
        a = 0x01; // set moving direction as flag for
        writeData(Enemy_MovingDir + x, 0x01); // falling platforms

    StopPlatforms();
    return;

    //------------------------------------------------------------------------
    } // PlatformFall
    a = y; // save offset for other platform to stack
    pha();
    MoveFallingPlatform(); // make current platform fall
    pla();
    x = a; // pull offset from stack and save to X
    MoveFallingPlatform(); // make other platform fall
    x = M(ObjectOffset);
    a = M(PlatformCollisionFlag + x); // if player not standing on either platform,
    if ((a & 0x80) == 0)
    { // skip this part
        x = a; // transfer collision flag offset as offset to X
        PositionPlayerOnVPlat(); // and position player appropriately
    } // ExPF: get enemy object buffer offset and leave
    x = M(ObjectOffset);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PlayerHeadCollision()
{
    bool bumpedBlockFound = false;

    pha(); // store metatile number to stack
    a = 0x11; // load unbreakable block object state by default
    x = M(SprDataOffset_Ctrl); // load offset control bit here
    // check player's size
    if (M(PlayerSize) == 0)
    { // if small, branch
        a = 0x12; // otherwise load breakable block object state
    } // DBlockSte: store into block object buffer
    writeData(Block_State + x, a);
    DestroyBlockMetatile(); // store blank metatile in vram buffer to write to name table
    x = M(SprDataOffset_Ctrl); // load offset control bit
    a = M(0x02); // get vertical high nybble offset used in block buffer routine
    writeData(Block_Orig_YPos + x, a); // set as vertical coordinate for block object
    y = a;
    // get low byte of block buffer address used in same routine
    writeData(Block_BBuf_Low + x, M(0x06)); // save as offset here to be used later
    a = M(W(0x06) + y); // get contents of block buffer at old address at $06, $07
    bumpedBlockFound = BlockBumpedChk(); // do a sub to check which block player bumped head on
    writeData(0x00, a); // store metatile here
    y = M(PlayerSize); // check player's size
    if (y == 0)
    { // if small, use metatile itself as contents of A
        a = y; // otherwise init A (note: big = 0)
    } // ChkBrick: if no match was found in previous sub, skip ahead
    if (!bumpedBlockFound)
        goto PutMTileB;
    // otherwise load unbreakable state into block object buffer
    writeData(Block_State + x, 0x11); // note this applies to both player sizes
    a = 0xc4; // load empty block metatile into A for now
    y = M(0x00); // get metatile from before
    if (y != 0x58)
    { // if so, branch
        if (y != 0x5d)
            goto PutMTileB; // if not, branch ahead to store empty block metatile
    } // StartBTmr: check brick coin timer flag
    if (M(BrickCoinTimerFlag) == 0)
    { // if set, timer expired or counting down, thus branch
        a = 0x0b;
        writeData(BrickCoinTimer, 0x0b); // if not set, set brick coin timer
        ++M(BrickCoinTimerFlag); // and set flag linked to it
    } // ContBTmr: check brick coin timer
    if (M(BrickCoinTimer) == 0)
    { // if not yet expired, branch to use current metatile
        y = 0xc4; // otherwise use empty block metatile
    } // PutOldMT: put metatile into A
    a = y;

PutMTileB: // store whatever metatile be appropriate here
    writeData(Block_Metatile + x, a);
    InitBlock_XY_Pos(); // get block object horizontal coordinates saved
    y = M(0x02); // get vertical high nybble offset
    writeData(W(0x06) + y, 0x23); // write blank metatile $23 to block buffer
    writeData(BlockBounceTimer, 0x10); // set block bounce timer
    pla(); // pull original metatile from stack
    writeData(0x05, a); // and save here
    y = 0x00; // set default offset
    // is player crouching?
    if (M(CrouchingFlag) == 0)
    { // if so, branch to increment offset
        // is player big?
        if (M(PlayerSize) == 0)
            goto BigBP; // if so, branch to use default offset
    } // SmallBP: increment for small or big and crouching
    y = 0x01;

BigBP: // get player's vertical coordinate
    a = M(Player_Y_Position);
    a += M(BlockYPosAdderData + y); // add value determined by size
    a &= 0xf0; // mask out low nybble to get 16-pixel correspondence
    writeData(Block_Y_Position + x, a); // save as vertical coordinate for block object
    // get block object state
    if (M(Block_State + x) != 0x11)
    { // if set to value loaded for unbreakable, branch
        BrickShatter(); // execute code for breakable brick
    } // Unbreak: execute code for unbreakable brick or question block
    else // skip subroutine to do last part of code here
    {
        BumpBlock();
    } // InvOBit: invert control bit used by block objects
    a = M(SprDataOffset_Ctrl) ^ 0x01; // and floatey numbers
    writeData(SprDataOffset_Ctrl, a);
    return; // leave!
}

//------------------------------------------------------------------------

void SMBEngine::BumpBlock()
{
    bool bumpedBlockFound = false;

    CheckTopOfBlock(); // check to see if there's a coin directly above this block
    writeData(Square1SoundQueue, Sfx_Bump); // play bump sound
    writeData(Block_X_Speed + x, 0x00); // initialize horizontal speed for block object
    writeData(Block_Y_MoveForce + x, 0x00); // init fractional movement force
    writeData(Player_Y_Speed, 0x00); // init player's vertical speed
    writeData(Block_Y_Speed + x, 0xfe); // set vertical speed for block object
    a = M(0x05); // get original metatile from stack
    bumpedBlockFound = BlockBumpedChk(); // do a sub to check which block player bumped head on
    if (!bumpedBlockFound)
    { // if no match was found, branch to leave
        return;
    }
    a = y; // move block number to A
    if (a >= 0x09)
    { // branch to use current number
        a -= 0x05; // otherwise subtract 5 for second set to get proper number
    } // BlockCode: run appropriate subroutine depending on block number
    switch (a)
    {
    case 0:
        MushFlowerBlock();
        return;
    case 1:
        CoinBlock();
        return;
    case 2:
        CoinBlock();
        return;
    case 3:
        ExtraLifeMushBlock();
        return;
    case 4:
        MushFlowerBlock();
        return;
    case 5:
        VineBlock();
        return;
    case 6:
        StarBlock();
        return;
    case 7:
        CoinBlock();
        return;
    case 8:
        ExtraLifeMushBlock();
        return;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

void SMBEngine::VineBlock()
{
    x = 0x05; // load last slot for enemy object buffer
    y = M(SprDataOffset_Ctrl); // get control bit
    Setup_Vine(); // set up vine object
    return; // leave
}

//------------------------------------------------------------------------

void SMBEngine::PlayerBGCollision()
{
    bool climbMTileFound = false;
    bool coinMTileFound = false;
    bool jumpspringFound = false;
    bool solidMTileFound = false;

    a = M(DisableCollisionDet); // if collision detection disabled flag set,
    if (a != 0)
        return; // branch to leave
    a = M(GameEngineSubroutine);
    if (a == 0x0b)
        return; // branch to leave
    if (a < 0x04)
        return; // if running routines $00-$03 branch to leave
    a = 0x01; // load default player state for swimming
    y = M(SwimmingFlag); // if swimming flag set,
    if (y == 0)
    { // branch ahead to set default state
        a = M(Player_State); // if player in normal state,
        if (a != 0)
        { // branch to set default state for falling
            if (a != 0x03)
                goto ChkOnScr; // if in any other state besides climbing, skip to next part
        } // SetFallS: load default player state for falling
        a = 0x02;
    } // SetPSte: set whatever player state is appropriate
    writeData(Player_State, a);

ChkOnScr:
    a = M(Player_Y_HighPos);
    if (a != 0x01)
        return; // branch to leave if not
    writeData(Player_CollisionBits, 0xff); // initialize player's collision flag
    a = M(Player_Y_Position);
    if (a >= 0xcf)
    { // if not too close to the bottom of screen, continue

        return; // ExPBGCol: otherwise leave

    //------------------------------------------------------------------------
    } // ChkCollSize
    y = 0x02; // load default offset
    if (M(CrouchingFlag) != 0)
        goto GBBAdr; // if player crouching, skip ahead
    if (M(PlayerSize) != 0)
        goto GBBAdr; // if player small, skip ahead
    y = 0x01; // otherwise decrement offset for big player not crouching
    if (M(SwimmingFlag) != 0)
        goto GBBAdr; // if swimming flag set, skip ahead
    y = 0x00; // otherwise decrement offset

GBBAdr: // get value using offset
    a = M(BlockBufferAdderData + y);
    writeData(0xeb, a); // store value here
    y = a; // put value into Y, as offset for block buffer routine
    x = M(PlayerSize); // get player's size as offset
    if (M(CrouchingFlag) != 0)
    { // if player not crouching, branch ahead
        ++x; // otherwise increment size as offset
    } // HeadChk: get player's vertical coordinate
    if (M(Player_Y_Position) < M(PlayerBGUpperExtent + x))
        goto DoFootCheck; // if player is too high, skip this part
    BlockBufferColli_Head(); // do player-to-bg collision detection on top of
    if (a == 0)
        goto DoFootCheck; // player, and branch if nothing above player's head
    coinMTileFound = CheckForCoinMTiles(); // check to see if player touched coin with their head
    if (coinMTileFound)
        goto AwardTouchedCoin; // if so, branch to some other part of code
    // check player's vertical speed
    if ((M(Player_Y_Speed) & 0x80) == 0)
        goto DoFootCheck; // if player not moving upwards, branch elsewhere
    // check lower nybble of vertical coordinate returned
    if (M(0x04) < 0x04)
        goto DoFootCheck; // if low nybble < 4, branch
    solidMTileFound = CheckForSolidMTiles(); // check to see what player's head bumped on
    if (!solidMTileFound)
    { // if player collided with solid metatile, branch
        // otherwise check area type
        if (M(AreaType) == 0)
            goto NYSpd; // if water level, branch ahead
        // if block bounce timer not expired,
        if (M(BlockBounceTimer) != 0)
            goto NYSpd; // branch ahead, do not process collision
        PlayerHeadCollision(); // otherwise do a sub to process collision
        goto DoFootCheck; // jump ahead to skip these other parts here
    } // SolidOrClimb
    if (a == 0x26)
        goto NYSpd; // branch ahead and do not play sound
    a = Sfx_Bump;
    writeData(Square1SoundQueue, Sfx_Bump); // otherwise load bump sound

NYSpd: // set player's vertical speed to nullify
    a = 0x01;
    writeData(Player_Y_Speed, 0x01); // jump or swim

DoFootCheck:
    y = M(0xeb); // get block buffer adder offset
    if (M(Player_Y_Position) >= 0xcf)
        goto DoPlayerSideCheck; // if player is too far down on screen, skip all of this
    BlockBufferColli_Feet(); // do player-to-bg collision detection on bottom left of player
    coinMTileFound = CheckForCoinMTiles(); // check to see if player touched coin with their left foot
    if (coinMTileFound)
        goto AwardTouchedCoin; // if so, branch to some other part of code
    pha(); // save bottom left metatile to stack
    BlockBufferColli_Feet(); // do player-to-bg collision detection on bottom right of player
    writeData(0x00, a); // save bottom right metatile here
    pla();
    writeData(0x01, a); // pull bottom left metatile and save here
    if (a != 0)
        goto ChkFootMTile; // if anything here, skip this part
    a = M(0x00); // otherwise check for anything in bottom right metatile
    if (a == 0)
        goto DoPlayerSideCheck; // and skip ahead if not
    coinMTileFound = CheckForCoinMTiles(); // check to see if player touched coin with their right foot
    if (!coinMTileFound)
        goto ChkFootMTile; // if not, skip unconditional jump and continue code

AwardTouchedCoin:
    HandleCoinMetatile(); // follow the code to erase coin and award to player 1 coin
    return;

ChkFootMTile:
    climbMTileFound = CheckForClimbMTiles(); // check to see if player landed on climbable metatiles
    if (climbMTileFound)
        goto DoPlayerSideCheck; // if so, branch
    y = M(Player_Y_Speed); // check player's vertical speed
    if ((y & 0x80) != 0)
        goto DoPlayerSideCheck; // if player moving upwards, branch
    if (a == 0xc5)
    { // if player did not touch axe, skip ahead
    } // ContChk: do sub to check for hidden coin or 1-up blocks
    else // otherwise jump to set modes of operation
    {
        ChkInvisibleMTiles();
        if (a == 0x5f || a == 0x60)
            goto DoPlayerSideCheck; // if either found, branch
        // if jumpspring animating right now,
        if (M(JumpspringAnimCtrl) == 0)
        { // branch ahead
            y = M(0x04); // check lower nybble of vertical coordinate returned
            if (y >= 0x05)
            { // if lower nybble < 5, branch
                writeData(0x00, M(Player_MovingDir)); // use player's moving direction as temp variable
                ImpedePlayerMove(); // jump to impede player's movement in that direction
                return;
            } // LandPlyr: do sub to check for jumpspring metatiles and deal with it
            ChkForLandJumpSpring();
            a = 0xf0;
            a &= M(Player_Y_Position); // mask out lower nybble of player's vertical position
            writeData(Player_Y_Position, a); // and store as new vertical position to land player properly
            HandlePipeEntry(); // do sub to process potential pipe entry
            a = 0x00;
            writeData(Player_Y_Speed, 0x00); // initialize vertical speed and fractional
            writeData(Player_Y_MoveForce, 0x00); // movement force to stop player's vertical movement
            writeData(StompChainCounter, 0x00); // initialize enemy stomp counter
        } // InitSteP
        a = 0x00;
        writeData(Player_State, 0x00); // set player's state to normal

DoPlayerSideCheck:
        y = M(0xeb); // get block buffer adder offset
        ++y;
        ++y; // increment offset 2 bytes to use adders for side collisions
        a = 0x02; // set value here to be used as counter
        writeData(0x00, 0x02);

        do // SideCheckLoop
        {
            ++y; // move onto the next one
            writeData(0xeb, y); // store it
            a = M(Player_Y_Position);
            if (a < 0x20)
                goto BHalf; // if player is in status bar area, branch ahead to skip this part
            if (a >= 0xe4)
                return; // branch to leave if player is too far down
            BlockBufferColli_Side(); // do player-to-bg collision detection on one half of player
            if (a == 0)
                goto BHalf; // branch ahead if nothing found
            if (a == 0x1c)
                goto BHalf; // if collided with sideways pipe (top), branch ahead
            if (a == 0x6b)
                goto BHalf; // if collided with water pipe (top), branch ahead
            climbMTileFound = CheckForClimbMTiles(); // do sub to see if player bumped into anything climbable
            if (!climbMTileFound)
                goto CheckSideMTiles; // if not, branch to alternate section of code

BHalf: // load block adder offset
            y = M(0xeb);
            ++y; // increment it
            a = M(Player_Y_Position); // get player's vertical position
            if (a < 0x08)
                return; // if too high, branch to leave
            if (a >= 0xd0)
                return; // if too low, branch to leave
            BlockBufferColli_Side(); // do player-to-bg collision detection on other half of player
            if (a != 0)
                goto CheckSideMTiles; // if something found, branch
            --M(0x00); // otherwise decrement counter
        } while (M(0x00) != 0); // run code until both sides of player are checked

        return; // ExSCH: leave

    //------------------------------------------------------------------------

CheckSideMTiles:
        ChkInvisibleMTiles(); // check for hidden or coin 1-up blocks
        if (a == 0x5f || a == 0x60)
            return; // branch to leave if either found
        climbMTileFound = CheckForClimbMTiles(); // check for climbable metatiles
        if (climbMTileFound)
        { // if not found, skip and continue with code
            goto HandleClimbing; // otherwise jump to handle climbing
        } // ContSChk: check to see if player touched coin
        coinMTileFound = CheckForCoinMTiles();
        if (coinMTileFound)
        {
            HandleCoinMetatile(); // if so, execute code to erase coin and award to player 1 coin
            return;
        }
        jumpspringFound = ChkJumpspringMetatiles(); // check for jumpspring metatiles
        if (jumpspringFound)
        { // if not found, branch ahead to continue cude
            a = M(JumpspringAnimCtrl); // otherwise check jumpspring animation control
            if (a != 0)
                return; // branch to leave if set
            StopPlayerMove(); // otherwise jump to impede player's movement
            return;
        } // ChkPBtm: get player's state
        if (M(Player_State) != 0x00)
        {
            StopPlayerMove(); // if not, branch to impede player's movement
            return;
        }
        y = M(PlayerFacingDir); // get player's facing direction
        --y;
        if (y != 0)
        {
            StopPlayerMove(); // if facing left, branch to impede movement
            return;
        }
        if (a != 0x6c)
        { // if collided with sideways pipe (bottom), branch
            if (a != 0x1f)
            {
                StopPlayerMove(); // otherwise branch to impede player's movement
                return;
            }
        } // PipeDwnS: check player's attributes
        a = M(Player_SprAttrib);
        if (a == 0)
        { // if already set, branch, do not play sound again
            y = Sfx_PipeDown_Injury;
            writeData(Square1SoundQueue, Sfx_PipeDown_Injury); // otherwise load pipedown/injury sound
        } // PlyrPipe
        a |= 0b00100000;
        writeData(Player_SprAttrib, a); // set background priority bit in player attributes
        a = M(Player_X_Position) & 0b00001111; // get lower nybble of player's horizontal coordinate
        if (a != 0)
        { // if at zero, branch ahead to skip this part
            y = 0x00; // set default offset for timer setting data
            // load page location for left side of screen
            if (M(ScreenLeft_PageLoc) != 0)
            { // if at page zero, use default offset
                y = 0x01; // otherwise increment offset
            } // SetCATmr: set timer for change of area as appropriate
            writeData(ChangeAreaTimer, M(AreaChangeTimerData + y));
        } // ChkGERtn: get number of game engine routine running
        a = M(GameEngineSubroutine);
        if (a == 0x07)
            return; // if running player entrance routine or
        if (a != 0x08)
            return;
        a = 0x02;
        writeData(GameEngineSubroutine, 0x02); // otherwise set sideways pipe entry routine to run
        return; // and leave

    } // HandleAxeMetatile
    writeData(OperMode_Task, 0x00); // reset secondary mode
    writeData(OperMode, 0x02); // set primary mode to autoctrl mode
    a = 0x18;
    writeData(Player_X_Speed, 0x18); // set horizontal speed and continue to erase axe metatile

    ErACM();
    return;

HandleClimbing:
    y = M(0x04); // check low nybble of horizontal coordinate returned from
    if (y >= 0x06)
    { // makes actual physical part of vine or flagpole thinner
        if (y < 0x0a)
            goto ChkForFlagpole;
    } // ExHC: leave if too far left or too far right
    return;

//------------------------------------------------------------------------

ChkForFlagpole:
    if (a != 0x24)
    { // branch if flagpole ball found
        if (a != 0x25)
            goto VineCollision; // branch to alternate code if flagpole shaft not found
    } // FlagpoleCollision
    if (M(GameEngineSubroutine) == 0x05)
    {
        PutPlayerOnVine(); // if running, branch to end of climbing code
        return;
    }
    writeData(PlayerFacingDir, 0x01); // set player's facing direction to right
    ++M(ScrollLock); // set scroll lock flag
    if (M(GameEngineSubroutine) != 0x04)
    { // if running, branch to end of flagpole code here
        a = BulletBill_CannonVar; // load identifier for bullet bills (cannon variant)
        KillEnemies(); // get rid of them
        writeData(EventMusicQueue, Silence); // silence music
        writeData(FlagpoleSoundQueue, 0x40); // load flagpole sound into flagpole sound queue
        x = 0x04; // start at end of vertical coordinate data
        a = M(Player_Y_Position);
        writeData(FlagpoleCollisionYPos, a); // store player's vertical coordinate here to be used later

ChkFlagpoleYPosLoop:
        if (a < M(FlagpoleYPosData + x))
        { // if player's => current, branch to use current offset
            --x; // otherwise decrement offset to use
            if (x != 0)
                goto ChkFlagpoleYPosLoop; // do this until all data is checked (use last one if all checked)
        } // MtchF: store offset here to be used later
        writeData(FlagpoleScore, x);
    } // RunFR
    a = 0x04;
    writeData(GameEngineSubroutine, 0x04); // set value to run flagpole slide routine
    PutPlayerOnVine(); // jump to end of climbing code
    return;

VineCollision:
    if (a != 0x26)
    {
        PutPlayerOnVine();
        return;
    }
    // check player's vertical coordinate
    if (M(Player_Y_Position) >= 0x20)
    {
        PutPlayerOnVine(); // branch if not that far up
        return;
    }
    a = 0x01;
    writeData(GameEngineSubroutine, 0x01); // otherwise set to run autoclimb routine next frame
    PutPlayerOnVine();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ProcMoveRedPTroopa()
{
    a = M(Enemy_Y_Speed + x) | M(Enemy_Y_MoveForce + x); // check for any vertical force or speed
    if (a != 0)
        goto MoveRedPTUpOrDown; // branch if any found
    writeData(Enemy_YMF_Dummy + x, a); // initialize something here
    // check current vs. original vertical coordinate
    if (M(Enemy_Y_Position + x) >= M(RedPTroopaOrigXPos + x))
        goto MoveRedPTUpOrDown; // if current => original, skip ahead to more code
    // get frame counter
    a = M(FrameCounter) & 0b00000111; // mask out all but 3 LSB
    if (a == 0)
    { // if any bits set, branch to leave
        ++M(Enemy_Y_Position + x); // otherwise increment red paratroopa's vertical position
    } // NoIncPT: leave
    return;

//------------------------------------------------------------------------

MoveRedPTUpOrDown:
    // check current vs. central vertical coordinate
    if (M(Enemy_Y_Position + x) >= M(RedPTroopaCenterYPos + x))
    { // if current < central, jump to move downwards
        // otherwise jump to move upwards
        // inlined:
        y = 0x01; // set Y to move upwards
        MoveRedPTroopa();
        return;

    } // MovPTDwn: move downwards
    MoveRedPTroopaDown();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::JCoinGfxHandler()
{
    goto JCoinGfxHandler2;
    do // DrawFloateyNumber_Coin
    {
        if ((M(FrameCounter) & 0x01) == 0) // get frame counter divide by 2
        { // branch if d0 not set to raise number every other frame
            --M(Misc_Y_Position + x); // otherwise, decrement vertical coordinate
        } // NotRsNum: get vertical coordinate
        a = M(Misc_Y_Position + x);
        DumpTwoSpr(); // dump into both sprites
        a = M(Misc_Rel_XPos); // get relative horizontal coordinate
        writeData(Sprite_X_Position + y, a); // store as X coordinate for first sprite
        a += 0x08; // add eight pixels
        writeData(Sprite_X_Position + 4 + y, a); // store as X coordinate for second sprite
        writeData(Sprite_Attributes + y, 0x02); // store attribute byte in both sprites
        writeData(Sprite_Attributes + 4 + y, 0x02);
        writeData(Sprite_Tilenumber + y, 0xf7); // put tile numbers into both sprites
        a = 0xfb; // that resemble "200"
        writeData(Sprite_Tilenumber + 4 + y, 0xfb);
        return; // then jump to leave (why not an rts here instead?)

JCoinGfxHandler2:
        y = M(Misc_SprDataOffset + x); // get coin/floatey number's OAM data offset
        // get state of misc object
    } while (M(Misc_State + x) >= 0x02); // branch to draw floatey number
    a = M(Misc_Y_Position + x); // store vertical coordinate as
    writeData(Sprite_Y_Position + y, a); // Y coordinate for first sprite
    a += 0x08; // add eight pixels
    writeData(Sprite_Y_Position + 4 + y, a); // store as Y coordinate for second sprite
    a = M(Misc_Rel_XPos); // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, a);
    writeData(Sprite_X_Position + 4 + y, a); // store as X coordinate for first and second sprites
    // get frame counter
    a = M(FrameCounter) >> 1; // divide by 2 to alter every other frame
    a &= 0b00000011; // mask out d2-d1
    x = a; // use as graphical offset
    a = M(JumpingCoinTiles + x); // load tile number
    ++y; // increment OAM data offset to write tile numbers
    DumpTwoSpr(); // do sub to dump tile number into both sprites
    --y; // decrement to get old offset
    writeData(Sprite_Attributes + y, 0x02); // set attribute byte in first sprite
    a = 0x82;
    writeData(Sprite_Attributes + 4 + y, 0x82); // set attribute byte with vertical flip in second sprite
    x = M(ObjectOffset); // get misc object offset

    return; // ExJCGfx: leave
}








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
        writeData(SprDataOffset + x, M(DefaultSprOffsets + x));
        --x; // do this until they're all set
    } while ((x & 0x80) == 0);
    y = 0x03; // set up sprite #0

    do // ISpr0Loop
    {
        a = M(Sprite0Data + y);
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
    y = M(AreaType); // select appropriate palette to load
    x = M(AreaPalette + y); // based on area type
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

void SMBEngine::InitLongFirebar()
{
    DuplicateEnemyObj(); // create enemy object for long firebar

    InitShortFirebar();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitBowser()
{
    DuplicateEnemyObj(); // jump to create another bowser object
    writeData(BowserFront_Offset, x); // save offset of first here
    writeData(BowserBodyControls, 0x00); // initialize bowser's body controls
    writeData(BridgeCollapseOffset, 0x00); // and bridge collapse offset
    writeData(BowserOrigXPos, M(Enemy_X_Position + x)); // store original horizontal position here
    writeData(BowserFireBreathTimer, 0xdf); // store something here
    writeData(Enemy_MovingDir + x, 0xdf); // and in moving direction
    writeData(BowserFeetCounter, 0x20); // set bowser's feet timer and in enemy timer
    writeData(EnemyFrameTimer + x, 0x20);
    writeData(BowserHitPoints, 0x05); // give bowser 5 hit points
    a = 0x02;
    writeData(BowserMovementSpeed, 0x02); // set default movement speed here
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DuplicateEnemyObj()
{
    y = 0xff; // start at beginning of enemy slots

    do // FSLoop: increment one slot
    {
        ++y;
        // check enemy buffer flag for empty slot
    } while (M(Enemy_Flag + y) != 0); // if set, branch and keep checking
    writeData(DuplicateObj_Offset, y); // otherwise set offset here
    a = x; // transfer original enemy buffer offset
    a |= 0b10000000; // store with d7 set as flag in new enemy
    writeData(Enemy_Flag + y, a); // slot as well as enemy offset
    writeData(Enemy_PageLoc + y, M(Enemy_PageLoc + x)); // copy page location and horizontal coordinates
    // from original enemy to new enemy
    writeData(Enemy_X_Position + y, M(Enemy_X_Position + x));
    writeData(Enemy_Flag + x, 0x01); // set flag as normal for original enemy
    writeData(Enemy_Y_HighPos + y, 0x01); // set high vertical byte for new enemy
    a = M(Enemy_Y_Position + x);
    writeData(Enemy_Y_Position + y, a); // copy vertical coordinate from original to new
    return;
}

//------------------------------------------------------------------------

void SMBEngine::SetFlameTimer()
{
    y = M(BowserFlameTimerCtrl); // load counter as offset
    ++M(BowserFlameTimerCtrl); // increment
    // mask out all but 3 LSB
    a = M(BowserFlameTimerCtrl) & 0b00000111; // to keep in range of 0-7
    writeData(BowserFlameTimerCtrl, a);
    a = M(FlameTimerData + y); // load value to be used then leave
    return;
}

//------------------------------------------------------------------------

    //------------------------------------------------------------------------

void SMBEngine::ProcBowserFlame()
{
    bool shiftedBit = false;
    uint32_t wide = 0;

    // if master timer control flag set,
    if (M(TimerControl) != 0)
        goto SetGfxF; // skip all of this
    a = 0x40; // load default movement force
    if (M(SecondaryHardMode) != 0)
    { // if secondary hard mode flag not set, use default
        a = 0x60; // otherwise load alternate movement force to go faster
    } // SFlmX: store value here
    writeData(0x00, a);
    // pageloc:position:force is one 24-bit quantity
    wide = (M(Enemy_PageLoc + x) << 16) | (M(Enemy_X_Position + x) << 8) | M(Enemy_X_MoveForce + x);
    wide -= (0x01 << 8) | M(0x00); // subtract value from movement force and one pixel from the position
    writeData(Enemy_X_MoveForce + x, LOBYTE(wide)); // save new value
    writeData(Enemy_X_Position + x, HIBYTE(wide)); // to move to the left
    writeData(Enemy_PageLoc + x, (uint8_t)(wide >> 16));
    a = (uint8_t)(wide >> 16);
    y = M(BowserFlamePRandomOfs + x); // get some value here and use as offset
    a = M(Enemy_Y_Position + x); // load vertical coordinate
    if (a == M(FlameYPosData + y))
        goto SetGfxF; // if equal, branch and do not modify coordinate
    a += M(Enemy_Y_MoveForce + x); // otherwise add value here to coordinate and store
    writeData(Enemy_Y_Position + x, a); // as new vertical coordinate

SetGfxF: // get new relative coordinates
    RelativeEnemyPosition();
    a = M(Enemy_State + x); // if bowser's flame not in normal state,
    if (a != 0)
        return; // branch to leave
    // otherwise, continue
    writeData(0x00, 0x51); // write first tile number
    y = 0x02; // load attributes without vertical flip by default
    a = M(FrameCounter) & 0b00000010; // invert vertical flip bit every 2 frames
    if (a != 0)
    { // if d1 not set, write default value
        y = 0x82; // otherwise write value with vertical flip bit set
    } // FlmeAt: set bowser's flame sprite attributes here
    writeData(0x01, y);
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    x = 0x00;

    do // DrawFlameLoop
    {
        // get Y relative coordinate of current enemy object
        writeData(Sprite_Y_Position + y, M(Enemy_Rel_YPos)); // write into Y coordinate of OAM data
        writeData(Sprite_Tilenumber + y, M(0x00)); // write current tile number into OAM data
        ++M(0x00); // increment tile number to draw more bowser's flame
        writeData(Sprite_Attributes + y, M(0x01)); // write saved attributes into OAM data
        a = M(Enemy_Rel_XPos);
        writeData(Sprite_X_Position + y, a); // write X relative coordinate of current enemy object
        a += 0x08;
        writeData(Enemy_Rel_XPos, a); // then add eight to it and store
        ++y;
        ++y;
        ++y;
        ++y; // increment Y four times to move onto the next OAM
        ++x; // move onto the next OAM, and branch if three
    } while (x < 0x03);
    x = M(ObjectOffset); // reload original enemy offset
    GetEnemyOffscreenBits(); // get offscreen information
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    // get enemy object offscreen bits
    a = M(Enemy_OffscreenBits) >> 1; // take d0, and push the rest to the stack
    pha();
    if ((M(Enemy_OffscreenBits) & 0x01) != 0)
    { // branch if it was not set
        a = 0xf8; // otherwise move sprite offscreen, this part likely
        writeData(Sprite_Y_Position + 12 + y, 0xf8); // residual since flame is only made of three sprites
    } // M3FOfs: get bits from stack
    pla();
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // take d1, and push the bits back to the stack
    pha();
    if (shiftedBit)
    { // branch if it was not set again
        a = 0xf8; // otherwise move third sprite offscreen
        writeData(Sprite_Y_Position + 8 + y, 0xf8);
    } // M2FOfs: get bits from stack again
    pla();
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // take d2, and push the bits back to the stack again
    pha();
    if (shiftedBit)
    { // branch if it was not set yet again
        a = 0xf8; // otherwise move second sprite offscreen
        writeData(Sprite_Y_Position + 4 + y, 0xf8);
    } // M1FOfs: get bits from stack one last time
    pla();
    shiftedBit = (a & 0x01) != 0;
    if (shiftedBit) // and d3
    { // branch if it was not set one last time
        a = 0xf8;
        writeData(Sprite_Y_Position + y, 0xf8); // otherwise move first sprite offscreen
    } // ExFlmeD: leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RunFireworks()
{
    --M(ExplosionTimerCounter + x); // decrement explosion timing counter here
    if (M(ExplosionTimerCounter + x) == 0)
    { // if not expired, skip this part
        writeData(ExplosionTimerCounter + x, 0x08); // reset counter
        ++M(ExplosionGfxCounter + x); // increment explosion graphics counter
        if (M(ExplosionGfxCounter + x) >= 0x03)
            goto FireworksSoundScore; // if at a certain point, branch to kill this object
    } // SetupExpl: get relative coordinates of explosion
    RelativeEnemyPosition();
    // copy relative coordinates
    writeData(Fireball_Rel_YPos, M(Enemy_Rel_YPos)); // from the enemy object to the fireball object
    // first vertical, then horizontal
    writeData(Fireball_Rel_XPos, M(Enemy_Rel_XPos));
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    a = M(ExplosionGfxCounter + x); // get explosion graphics counter
    DrawExplosion_Fireworks(); // do a sub to draw the explosion then leave
    return;

//------------------------------------------------------------------------

FireworksSoundScore:
    // disable enemy buffer flag
    writeData(Enemy_Flag + x, 0x00);
    // play fireworks/gunfire sound
    writeData(Square2SoundQueue, Sfx_Blast);
    a = 0x05; // set part of score modifier for 500 points
    writeData(DigitModifier + 4, 0x05);
    EndAreaPoints(); // jump to award points accordingly then leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawExplosion_Fireball()
{
    y = M(Alt_SprDataOffset + x); // get OAM data offset of alternate sort for fireball's explosion
    a = M(Fireball_State + x); // load fireball state
    ++M(Fireball_State + x); // increment state for next frame
    a >>= 1; // divide by 2
    a &= 0b00000111; // mask out all but d3-d1
    if (a >= 0x03)
    { // branch if so, otherwise continue to draw explosion
        // moved
        a = 0x00; // clear fireball state to kill it
        writeData(Fireball_State + x, 0x00);
        return;
    }
    DrawExplosion_Fireworks();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::FlagpoleRoutine()
{
    uint32_t wide = 0;

    x = 0x05; // set enemy object offset
    writeData(ObjectOffset, 0x05); // to special use slot
    a = M(Enemy_ID + 0x05);
    if (a == FlagpoleFlagObject)
    { // branch to leave
        if (M(GameEngineSubroutine) != 0x04)
            goto SkipScore; // branch to near the end of code
        if (M(Player_State) != 0x03)
            goto SkipScore; // branch to near the end of code
        // check flagpole flag's vertical coordinate
        if (M(Enemy_Y_Position + 0x05) >= 0xaa)
            goto GiveFPScr; // branch to end the level
        // check player's vertical coordinate
        if (M(Player_Y_Position) >= 0xa2)
            goto GiveFPScr; // branch to end the level
        // position:dummy is one 16-bit quantity; the compare above left the carry clear
        wide = ((M(Enemy_Y_Position + 0x05) << 8) | M(Enemy_YMF_Dummy + 0x05)) + 0x01ff; // add movement amount to move flag down
        writeData(Enemy_YMF_Dummy + 0x05, LOBYTE(wide)); // save dummy variable
        writeData(Enemy_Y_Position + 0x05, HIBYTE(wide)); // store vertical coordinate
        wide = ((M(FlagpoleFNum_Y_Pos) << 8) | M(FlagpoleFNum_YMFDummy)) - 0x01ff; // subtract the same to move the floatey number up
        writeData(FlagpoleFNum_YMFDummy, LOBYTE(wide)); // save dummy variable
        writeData(FlagpoleFNum_Y_Pos, HIBYTE(wide)); // and store vertical coordinate here
        a = HIBYTE(wide);

SkipScore: // jump to skip ahead and draw flag and floatey number
        goto FPGfx;

GiveFPScr: // get score offset from earlier (when player touched flagpole)
        y = M(FlagpoleScore);
        // get amount to award player points
        x = M(FlagpoleScoreDigits + y); // get digit with which to award points
        writeData(DigitModifier + x, M(FlagpoleScoreMods + y)); // store in digit modifier
        AddToScore(); // do sub to award player points depending on height of collision
        a = 0x05;
        writeData(GameEngineSubroutine, 0x05); // set to run end-of-level subroutine on next frame

FPGfx: // get offscreen information
        GetEnemyOffscreenBits();
        RelativeEnemyPosition(); // get relative coordinates
        FlagpoleGfxHandler(); // draw flagpole flag and floatey number
    } // ExitFlagP
    return;
}



//------------------------------------------------------------------------

void SMBEngine::FlagpoleGfxHandler()
{
    uint32_t wide = 0;

    y = M(Enemy_SprDataOffset + x); // get sprite data offset for flagpole flag
    a = M(Enemy_Rel_XPos); // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, a); // store as X coordinate for first sprite
    a += 0x08; // add eight pixels and store
    writeData(Sprite_X_Position + 4 + y, a); // as X coordinate for second and third sprites
    writeData(Sprite_X_Position + 8 + y, a);
    wide = a + 0x0c; // add twelve more pixels and
    writeData(0x05, LOBYTE(wide)); // store here to be used later by floatey number
    a = M(Enemy_Y_Position + x); // get vertical coordinate
    DumpTwoSpr(); // and do sub to dump into first and second sprites
    a = (uint8_t)(a + 0x08 + HIBYTE(wide)); // add eight pixels, plus the carry out of the horizontal add above
    writeData(Sprite_Y_Position + 8 + y, a); // and store into third sprite
    // get vertical coordinate for floatey number
    writeData(0x02, M(FlagpoleFNum_Y_Pos)); // store it here
    writeData(0x03, 0x01); // set value for flip which will not be used, and
    writeData(0x04, 0x01); // attribute byte for floatey number
    writeData(Sprite_Attributes + y, 0x01); // set attribute bytes for all three sprites
    writeData(Sprite_Attributes + 4 + y, 0x01);
    writeData(Sprite_Attributes + 8 + y, 0x01);
    writeData(Sprite_Tilenumber + y, 0x7e); // put triangle shaped tile
    writeData(Sprite_Tilenumber + 8 + y, 0x7e); // into first and third sprites
    writeData(Sprite_Tilenumber + 4 + y, 0x7f); // put skull tile into second sprite
    // get vertical coordinate at time of collision
    if (M(FlagpoleCollisionYPos) != 0)
    { // if zero, branch ahead
        a = y;
        a += 0x0c;
        y = a; // put back in Y
        a = M(FlagpoleScore); // get offset used to award points for touching flagpole
        a <<= 1; // multiply by 2 to get proper offset here
        x = a;
        // get appropriate tile data
        writeData(0x00, M(FlagpoleScoreNumTiles + x));
        a = M(FlagpoleScoreNumTiles + 1 + x);
        DrawOneSpriteRow(); // use it to render floatey number
    } // ChkFlagOffscreen
    x = M(ObjectOffset); // get object offset for flag
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    // get offscreen bits
    a = M(Enemy_OffscreenBits) & 0b00001110; // mask out all but d3-d1
    if (a == 0)
    { // if none of these bits set, branch to leave
        return;
    }
    MoveSixSpritesOffscreen();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::DrawLargePlatform()
{
    bool shiftedBit = false;

    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    writeData(0x02, y); // store here
    ++y; // add 3 to it for offset
    ++y; // to X coordinate
    ++y;
    a = M(Enemy_Rel_XPos); // get horizontal relative coordinate
    SixSpriteStacker(); // store X coordinates using A as base, stack horizontally
    x = M(ObjectOffset);
    a = M(Enemy_Y_Position + x); // get vertical coordinate
    DumpFourSpr(); // dump into first four sprites as Y coordinate
    if (M(AreaType) != 0x03)
    {
        // check for secondary hard mode flag set
        if (M(SecondaryHardMode) == 0)
            goto SetLast2Platform; // branch if not set elsewhere
    } // ShrinkPlatform
    a = 0xf8; // load offscreen coordinate if flag set or castle-type level

SetLast2Platform:
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    writeData(Sprite_Y_Position + 16 + y, a); // store vertical coordinate or offscreen
    writeData(Sprite_Y_Position + 20 + y, a); // coordinate into last two sprites as Y coordinate
    a = 0x5b; // load default tile for platform (girder)
    if (M(CloudTypeOverride) != 0)
    { // if cloud level override flag not set, use
        a = 0x75; // otherwise load other tile for platform (puff)
    } // SetPlatformTilenum
    x = M(ObjectOffset); // get enemy object buffer offset
    ++y; // increment Y for tile offset
    DumpSixSpr(); // dump tile number into all six sprites
    a = 0x02; // set palette controls
    ++y; // increment Y for sprite attributes
    DumpSixSpr(); // dump attributes into all six sprites
    ++x; // increment X for enemy objects
    GetXOffscreenBits(); // get offscreen bits again
    --x;
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d7, save the remaining
    pha(); // bits to the stack
    if (shiftedBit)
    {
        a = 0xf8; // if d7 was set, move first sprite offscreen
        writeData(Sprite_Y_Position + y, 0xf8);
    } // SChk2: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d6
    pha(); // save to stack
    if (shiftedBit)
    {
        a = 0xf8; // if d6 was set, move second sprite offscreen
        writeData(Sprite_Y_Position + 4 + y, 0xf8);
    } // SChk3: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d5
    pha(); // save to stack
    if (shiftedBit)
    {
        a = 0xf8; // if d5 was set, move third sprite offscreen
        writeData(Sprite_Y_Position + 8 + y, 0xf8);
    } // SChk4: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d4
    pha(); // save to stack
    if (shiftedBit)
    {
        a = 0xf8; // if d4 was set, move fourth sprite offscreen
        writeData(Sprite_Y_Position + 12 + y, 0xf8);
    } // SChk5: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d3
    pha(); // save to stack
    if (shiftedBit)
    {
        a = 0xf8; // if d3 was set, move fifth sprite offscreen
        writeData(Sprite_Y_Position + 16 + y, 0xf8);
    } // SChk6: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    if (shiftedBit) // and d2
    { // save to stack
        a = 0xf8;
        writeData(Sprite_Y_Position + 20 + y, 0xf8); // if d2 was set, move sixth sprite offscreen
    } // SLChk: check d7 of offscreen bits
    a = M(Enemy_OffscreenBits);
    a <<= 1; // and if d7 is not set, skip sub
    if ((M(Enemy_OffscreenBits) & 0x80) != 0)
    {
        MoveSixSpritesOffscreen(); // otherwise branch to move all sprites offscreen
    } // ExDLPl
    return;
}






//------------------------------------------------------------------------

// if row = $0e, increment three bytes
void SMBEngine::Inc3B()
{
    ++M(EnemyDataOffset);
    Inc2B();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::XMovingPlatform()
{
    a = 0x0e; // load preset maximum value for secondary counter
    XMoveCntr_Platform(); // do a sub to increment counters for movement
    MoveWithXMCntrs(); // do a sub to move platform accordingly, and return value
    a = M(PlatformCollisionFlag + x); // if no collision with player,
    if ((a & 0x80) != 0)
    { // branch ahead to leave
        return;
    }
    PositionPlayerOnHPlat();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PositionPlayerOnHPlat()
{
    uint32_t wide = 0;

    y = M(0x00); // the saved value from the second subroutine is a signed adder
    wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + (int8_t)M(0x00);
    writeData(Player_X_Position, LOBYTE(wide)); // position player accordingly in horizontal position
    writeData(Player_PageLoc, HIBYTE(wide)); // SetPVar: save result to player's page location
    a = HIBYTE(wide);
    writeData(Platform_X_Scroll, y); // put saved value from second sub here to be used later
    PositionPlayerOnVPlat(); // position player vertically and appropriately
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RightPlatform()
{
    MoveEnemyHorizontally(); // move platform with current horizontal speed, if any
    writeData(0x00, a); // store saved value here (residual code)
    a = M(PlatformCollisionFlag + x); // check collision flag, if no collision between player
    if ((a & 0x80) == 0)
    { // and platform, branch ahead, leave speed unaltered
        a = 0x10;
        writeData(Enemy_X_Speed + x, 0x10); // otherwise set new speed (gets moving if motionless)
        PositionPlayerOnHPlat(); // use saved value from earlier sub to position player
    } // ExRPl: then leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveDropPlatform()
{
    y = 0x7f; // set movement amount for drop platform
    NotMoveEnemySlowVert();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveEnemySlowVert()
{
    y = 0x0f; // set movement amount for bowser/other objects
    NotMoveEnemySlowVert();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::NotMoveEnemySlowVert()
{
    a = 0x02;
    if (a != 0)
    {
        SetXMoveAmt(); // unconditional branch
        return;
    }
    MoveJ_EnemyVertically();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveJ_EnemyVertically()
{
    y = 0x1c; // set movement amount for podoboo/other objects
    SetHiMax();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::LargePlatformSubroutines()
{
    a = M(Enemy_ID + x); // subtract $24 to get proper offset for jump table
    a -= 0x24;
    switch (a)
    {
    case 0:
        BalancePlatform(); // table used by objects $24-$2a
        return;
    case 1:
        YMovingPlatform();
        return;
    case 2:
        MoveLargeLiftPlat();
        return;
    case 3:
        MoveLargeLiftPlat();
        return;
    case 4:
        XMovingPlatform();
        return;
    case 5:
        DropPlatform();
        return;
    case 6:
        RightPlatform();
        return;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

void SMBEngine::MovePodoboo()
{
    // check enemy timer
    if (M(EnemyIntervalTimer + x) == 0)
    { // branch to move enemy if not expired
        InitPodoboo(); // otherwise set up podoboo again
        // get part of LSFR
        a = M(PseudoRandomBitReg + 1 + x) | 0b10000000; // set d7
        writeData(Enemy_Y_MoveForce + x, a); // store as movement force
        a &= 0b00001111; // mask out high nybble
        a |= 0x06; // set for at least six intervals
        writeData(EnemyIntervalTimer + x, a); // store as new enemy timer
        a = 0xf9;
        writeData(Enemy_Y_Speed + x, 0xf9); // set vertical speed to move podoboo upwards
    } // PdbM: branch to impose gravity on podoboo
    MoveJ_EnemyVertically();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveJumpingEnemy()
{
    MoveJ_EnemyVertically(); // do a sub to impose gravity on green paratroopa
    MoveEnemyHorizontally(); // jump to move enemy horizontally
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveBloober()
{
    uint32_t wide = 0;
    bool blooberCarry = false;
    bool enemyRightOfPlayer = false;

    a = M(Enemy_State + x) & 0b00100000; // check enemy state for d5 set
    if (a == 0)
    { // branch if set to move defeated bloober
        y = M(SecondaryHardMode); // use secondary hard mode flag as offset
        // get LSFR
        a = M(PseudoRandomBitReg + 1 + x) & M(BlooberBitmasks + y); // mask out bits in LSFR using bitmask loaded with offset
        blooberCarry = false; // the jump engine that dispatched here left the carry clear
        if (a == 0)
        { // if any bits set, skip ahead to make swim
            if ((x & 0x01) != 0)
            { // check to see if on second or fourth slot (1 or 3)
                y = M(Player_MovingDir); // otherwise, load player's moving direction and
                blooberCarry = true; // the shift of an odd slot number carries its d0 out
                goto SBMDir; // do an unconditional branch to set
            } // FBLeft: set left moving direction by default
            y = 0x02;
            enemyRightOfPlayer = PlayerEnemyDiff(); // get horizontal difference between player and bloober
            blooberCarry = enemyRightOfPlayer; // the difference leaves its no-borrow behind
            if ((a & 0x80) == 0)
                goto SBMDir; // if enemy to the right of player, keep left
            --y; // otherwise decrement to set right moving direction

SBMDir: // set moving direction of bloober, then continue on here
            writeData(Enemy_MovingDir + x, y);
        } // BlooberSwim
        ProcSwimmingB(blooberCarry); // execute sub to make bloober swim characteristically
        a = M(Enemy_Y_Position + x); // get vertical coordinate
        a -= M(Enemy_Y_MoveForce + x); // subtract movement force
        if (a >= 0x20)
        { // if so, don't do it
            writeData(Enemy_Y_Position + x, a); // otherwise, set new vertical position, make bloober swim
        } // SwimX: check moving direction
        y = M(Enemy_MovingDir + x);
        --y;
        if (y == 0)
        { // if moving to the left, branch to second part
            wide = ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x)) + M(BlooperMoveSpeed + x);
            writeData(Enemy_X_Position + x, LOBYTE(wide)); // store result as new horizontal coordinate
            writeData(Enemy_PageLoc + x, HIBYTE(wide)); // store as new page location and leave
            a = HIBYTE(wide);
            return;

        //------------------------------------------------------------------------
        } // LeftSwim
        wide = ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x)) - M(BlooperMoveSpeed + x);
        writeData(Enemy_X_Position + x, LOBYTE(wide)); // store result as new horizontal coordinate
        writeData(Enemy_PageLoc + x, HIBYTE(wide)); // store as new page location and leave
        a = HIBYTE(wide);
        return;

    //------------------------------------------------------------------------
    } // MoveDefeatedBloober
    MoveEnemySlowVert(); // jump to move defeated bloober downwards
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveBulletBill()
{
    // check bullet bill's enemy object state for d5 set
    a = M(Enemy_State + x) & 0b00100000;
    if (a != 0)
    { // if not set, continue with movement code
        MoveJ_EnemyVertically(); // otherwise jump to move defeated bullet bill downwards
        return;
    } // NotDefB: set bullet bill's horizontal speed
    a = 0xe8;
    writeData(Enemy_X_Speed + x, 0xe8); // and move it accordingly (note: this bullet bill
    MoveEnemyHorizontally(); // object occurs in frenzy object $17, not from cannons)
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveSwimmingCheepCheep()
{
    uint32_t wide = 0;

    // check cheep-cheep's enemy object state
    a = M(Enemy_State + x) & 0b00100000; // for d5 set
    if (a != 0)
    { // if not set, continue with movement code
        MoveEnemySlowVert(); // otherwise jump to move defeated cheep-cheep downwards
        return;
    } // CCSwim: save enemy state in $03
    writeData(0x03, a);
    a = M(Enemy_ID + x); // get enemy identifier
    a -= 0x0a; // subtract ten for cheep-cheep identifiers
    y = a; // use as offset
    // load value here
    writeData(0x02, M(SwimCCXMoveData + y));
    // page:coordinate:force is one 24-bit quantity here, so the borrow runs all the way up
    wide = (M(Enemy_PageLoc + x) << 16) | (M(Enemy_X_Position + x) << 8) | M(Enemy_X_MoveForce + x);
    wide -= M(0x02); // subtract preset value from horizontal force
    writeData(Enemy_X_MoveForce + x, LOBYTE(wide)); // store as new horizontal force
    writeData(Enemy_X_Position + x, HIBYTE(wide)); // and save as new horizontal coordinate
    writeData(Enemy_PageLoc + x, (uint8_t)(wide >> 16)); // page location, then save
    a = (uint8_t)(wide >> 16);
    a = 0x20;
    writeData(0x02, 0x20); // save new value here
    if (x < 0x02)
        return; // if in first or second slot, branch to leave
    // check movement flag
    if (M(CheepCheepMoveMFlag + x) >= 0x10)
    { // branch to move upwards
        // highpos:position:dummy is one 24-bit quantity
        wide = (M(Enemy_Y_HighPos + x) << 16) | (M(Enemy_Y_Position + x) << 8) | M(Enemy_YMF_Dummy + x);
        wide += (M(0x03) << 8) | M(0x02); // add preset value to the dummy and enemy state to the coordinate
        writeData(Enemy_YMF_Dummy + x, LOBYTE(wide)); // and save dummy
        writeData(Enemy_Y_Position + x, HIBYTE(wide)); // save as new vertical coordinate, slowly moved downwards
        a = (uint8_t)(wide >> 16); // and the page location
    } // CCSwimUpwards
    else // jump to end of movement code
    {
        // highpos:position:dummy is one 24-bit quantity
        wide = (M(Enemy_Y_HighPos + x) << 16) | (M(Enemy_Y_Position + x) << 8) | M(Enemy_YMF_Dummy + x);
        wide -= (M(0x03) << 8) | M(0x02); // subtract preset value from the dummy and enemy state from the coordinate
        writeData(Enemy_YMF_Dummy + x, LOBYTE(wide)); // and save dummy
        writeData(Enemy_Y_Position + x, HIBYTE(wide)); // save as new vertical coordinate, slowly moved upwards
        a = (uint8_t)(wide >> 16); // and the page location
    } // ChkSwimYPos
    writeData(Enemy_Y_HighPos + x, a); // save new page location here
    y = 0x00; // load movement speed to upwards by default
    a = M(Enemy_Y_Position + x); // get vertical coordinate
    a -= M(CheepCheepOrigYPos + x); // subtract original coordinate from current
    if ((a & 0x80) != 0)
    { // if result positive, skip to next part
        y = 0x10; // otherwise load movement speed to downwards
        a ^= 0xff;
        a += 0x01; // to obtain total difference of original vs. current
    } // YPDiff: if difference between original vs. current vertical
    if (a < 0x0f)
        return; // coordinates < 15 pixels, leave movement speed alone
    a = y;
    writeData(CheepCheepMoveMFlag + x, a); // otherwise change movement speed

    return; // ExSwCC: leave
}

//------------------------------------------------------------------------

void SMBEngine::MoveFlyingCheepCheep()
{
    // check cheep-cheep's enemy state
    a = M(Enemy_State + x) & 0b00100000; // for d5 set
    if (a != 0)
    { // branch to continue code if not set
        a = 0x00;
        writeData(Enemy_SprAttrib + x, 0x00); // otherwise clear sprite attributes
        MoveJ_EnemyVertically(); // and jump to move defeated cheep-cheep downwards
        return;
    } // FlyCC: move cheep-cheep horizontally based on speed and force
    MoveEnemyHorizontally();
    y = 0x0d; // set vertical movement amount
    a = 0x05; // set maximum speed
    SetXMoveAmt(); // branch to impose gravity on flying cheep-cheep
    a = M(Enemy_Y_MoveForce + x) >> 4; // get vertical movement force and move high nybble to low
    y = a; // save as offset (note this tends to go into reach of code)
    a = M(Enemy_Y_Position + x); // get vertical position
    a -= M(PRandomSubtracter + y);
    if ((a & 0x80) != 0)
    { // if result within top half of screen, skip this part
        a ^= 0xff;
        a += 0x01;
    } // AddCCF: if result or two's compliment greater than eight,
    if (a < 0x08)
    { // skip to the end without changing movement force
        a = M(Enemy_Y_MoveForce + x);
        a += 0x10; // otherwise add to it
        writeData(Enemy_Y_MoveForce + x, a);
        a >>= 1; // move high nybble to low again
        a >>= 1;
        a >>= 1;
        a >>= 1;
        y = a;
    } // BPGet: load bg priority data and store (this is very likely
    a = M(FlyCCBPriority + y);
    writeData(Enemy_SprAttrib + x, a); // broken or residual code, value is overwritten before
    return; // drawing it next frame), then leave
}

//------------------------------------------------------------------------

void SMBEngine::DropPlatform()
{
    a = M(PlatformCollisionFlag + x); // if no collision between platform and player
    if ((a & 0x80) == 0)
    { // occurred, just leave without moving anything
        MoveDropPlatform(); // otherwise do a sub to move platform down very quickly
        PositionPlayerOnVPlat(); // do a sub to position player appropriately
    } // ExDPl: leave
    return;
}

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

void SMBEngine::PlayerLoseLife()
{
    bool endGame = false;

    ++M(DisableScreenFlag); // disable screen and sprite 0 check
    writeData(Sprite0HitDetectFlag, 0x00);
    a = Silence; // silence music
    writeData(EventMusicQueue, Silence);
    --M(NumberofLives); // take one life from player
    if ((M(NumberofLives) & 0x80) != 0)
    { // if player still has lives, branch
        writeData(OperMode_Task, 0x00); // initialize mode task,
        a = GameOverModeValue; // switch to game over mode
        writeData(OperMode, GameOverModeValue); // and leave
        return;

    //------------------------------------------------------------------------
    } // StillInGame: multiply world number by 2 and use
    a = M(WorldNumber);
    a <<= 1; // as offset
    x = a;
    // if in area -3 or -4, increment
    a = M(LevelNumber) & 0x02; // offset by one byte, otherwise
    if (a != 0)
    { // leave offset alone
        ++x;
    } // GetHalfway: get halfway page number with offset
    y = M(HalfwayPageNybbles + x);
    a = y; // if in area -2 or -4, use lower nybble
    if ((M(LevelNumber) & 0x01) == 0)
    {
        a >>= 1; // move higher nybble to lower if area
        a >>= 1; // number is -1 or -3
        a >>= 1;
        a >>= 1;
    } // MaskHPNyb: mask out all but lower nybble
    a &= 0b00001111;
    if (a == M(ScreenLeft_PageLoc))
        goto SetHalfway; // left side of screen must be at the halfway page,
    if (a < M(ScreenLeft_PageLoc))
        goto SetHalfway; // otherwise player must start at the
    a = 0x00; // beginning of the level

SetHalfway: // store as halfway page for player
    writeData(HalfwayPage, a);
    endGame = TransposePlayers(); // switch players around if 2-player game
    ContinueGame(); // continue the game
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

void SMBEngine::ContinueGame()
{
    LoadAreaPointer(); // update level pointer with
    // actual world and area numbers, then
    writeData(PlayerSize, 0x01); // reset player's size, status, and
    ++M(FetchNewGameTimerFlag); // set game timer flag to reload
    // game timer from header
    writeData(TimerControl, 0x00); // also set flag for timers to count again
    writeData(PlayerStatus, 0x00);
    writeData(GameEngineSubroutine, 0x00); // reset task for game core
    writeData(OperMode_Task, 0x00); // set modes and leave
    a = 0x01; // if in game over mode, switch back to
    writeData(OperMode, 0x01); // game mode, because game is still on
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Entrance_GameTimerSetup()
{
    // set current page for area objects
    writeData(Player_PageLoc, M(ScreenLeft_PageLoc)); // as page location for player
    // store value here
    writeData(VerticalForceDown, 0x28); // for fractional movement downwards if necessary
    // set high byte of player position and
    writeData(PlayerFacingDir, 0x01); // set facing direction so that player faces right
    writeData(Player_Y_HighPos, 0x01);
    // set player state to on the ground by default
    writeData(Player_State, 0x00);
    --M(Player_CollisionBits); // initialize player's collision bits
    y = 0x00; // initialize halfway page
    writeData(HalfwayPage, 0x00);
    // check area type
    if (M(AreaType) == 0)
    { // if water type, set swimming flag, otherwise do not set
        y = 0x01;
    } // ChkStPos
    writeData(SwimmingFlag, y);
    x = M(PlayerEntranceCtrl); // get starting position loaded from header
    y = M(AltEntranceControl); // check alternate mode of entry flag for 0 or 1
    if (y == 0)
        goto SetStPos;
    if (y == 0x01)
        goto SetStPos;
    x = M(AltYPosOffset - 2 + y); // if not 0 or 1, override $0710 with new offset in X

SetStPos: // load appropriate horizontal position
    writeData(Player_X_Position, M(PlayerStarting_X_Pos + y)); // and vertical positions for the player, using
    // AltEntranceControl as offset for horizontal and either $0710
    writeData(Player_Y_Position, M(PlayerStarting_Y_Pos + x)); // or value that overwrote $0710 as offset for vertical
    writeData(Player_SprAttrib, M(PlayerBGPriorityData + x)); // set player sprite attributes using offset in X
    GetPlayerColors(); // get appropriate player palette
    y = M(GameTimerSetting); // get timer control value from header
    if (y == 0)
        goto ChkOverR; // if set to zero, branch (do not use dummy byte for this)
    // do we need to set the game timer? if not, use
    if (M(FetchNewGameTimerFlag) == 0)
        goto ChkOverR; // old game timer setting
    // if game timer is set and game timer flag is also set,
    writeData(GameTimerDisplay, M(GameTimerData + y)); // use value of game timer control for first digit of game timer
    writeData(GameTimerDisplay + 2, 0x01); // set last digit of game timer to 1
    a = 0x00;
    writeData(GameTimerDisplay + 1, 0x00); // set second digit of game timer
    writeData(FetchNewGameTimerFlag, 0x00); // clear flag for game timer reset
    writeData(StarInvincibleTimer, 0x00); // clear star mario timer

ChkOverR: // if controller bits not set, branch to skip this part
    if (M(JoypadOverride) != 0)
    {
        a = 0x03; // set player state to climbing
        writeData(Player_State, 0x03);
        x = 0x00; // set offset for first slot, for block object
        InitBlock_XY_Pos();
        a = 0xf0; // set vertical coordinate for block object
        writeData(Block_Y_Position, 0xf0);
        x = 0x05; // set offset in X for last enemy object buffer slot
        y = 0x00; // set offset in Y for object coordinates used earlier
        Setup_Vine(); // do a sub to grow vine
    } // ChkSwimE: if level not water-type,
    y = M(AreaType);
    if (y == 0)
    { // skip this subroutine
        writeData(0x07, 145); // LYNN HACK: simulate reading stray $07 value from JumpEngine,
                              // read by SetupBubble
        SetupBubble(); // otherwise, execute sub to set up air bubbles
    } // SetPESub: set to run player entrance subroutine
    a = 0x07;
    writeData(GameEngineSubroutine, 0x07); // on the next frame of game engine
    return;
}

//------------------------------------------------------------------------

void SMBEngine::BubbleCheck()
{
    // get part of LSFR
    a = M(PseudoRandomBitReg + 1 + x) & 0x01;
    writeData(0x07, a); // store pseudorandom bit here
    // get vertical coordinate for air bubble
    if (M(Bubble_Y_Position + x) != 0xf8)
    { // branch to move air bubble
        MoveBubl();
        return;
    }
    a = M(AirBubbleTimer); // if air bubble timer not expired,
    if (a != 0)
        return; // branch to leave, otherwise create new air bubble
    SetupBubble();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::SetupBubble()
{
    uint32_t wide = 0;

    y = 0x00; // load default value here
    if ((M(PlayerFacingDir) & 0x01) != 0)
    { // use the default value if facing left
        y = 0x09; // otherwise eight pixels over, plus the one d0 of the facing direction carries in
    } // PosBubl: use value loaded as adder
    wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + y; // add to player's horizontal position
    writeData(Bubble_X_Position + x, LOBYTE(wide)); // save as horizontal position for airbubble
    writeData(Bubble_PageLoc + x, HIBYTE(wide)); // save as page location for airbubble
    a = HIBYTE(wide);
    a = M(Player_Y_Position);
    a += 0x08;
    writeData(Bubble_Y_Position + x, a); // save as vertical position for air bubble
    writeData(Bubble_Y_HighPos + x, 0x01); // set vertical high byte for air bubble
    y = M(0x07); // get pseudorandom bit, use as offset
    // get data for air bubble timer
    writeData(AirBubbleTimer, M(BubbleTimerData + y)); // set air bubble timer
    MoveBubl();
    return;
}

//------------------------------------------------------------------------

// get pseudorandom bit again, use as offset
void SMBEngine::MoveBubl()
{
    uint32_t wide = 0;

    y = M(0x07);
    // position:dummy is one 16-bit quantity
    wide = ((M(Bubble_Y_Position + x) << 8) | M(Bubble_YMF_Dummy + x))
         - M(Bubble_MForceData + y); // subtract pseudorandom amount from dummy variable
    writeData(Bubble_YMF_Dummy + x, LOBYTE(wide)); // save dummy variable
    a = HIBYTE(wide); // the airbubble's vertical coordinate, less the borrow
    if (a < 0x20)
    { // branch to go ahead and use to move air bubble upwards
        a = 0xf8; // otherwise set offscreen coordinate
    } // Y_Bubl: store as new vertical coordinate for air bubble
    writeData(Bubble_Y_Position + x, a);

    return; // ExitBubl: leave
}









//------------------------------------------------------------------------

void SMBEngine::KillAllEnemies()
{
    x = 0x04; // start with last enemy slot

    do // KillLoop: branch to kill enemy objects
    {
        EraseEnemyObject();
        --x; // move onto next enemy slot
    } while ((x & 0x80) == 0); // do this until all slots are emptied
    writeData(EnemyFrenzyBuffer, a); // empty frenzy buffer
    x = M(ObjectOffset); // get enemy object offset and leave
    return;
}


//------------------------------------------------------------------------


//------------------------------------------------------------------------

void SMBEngine::GetFireballBoundBox()
{
    a = x; // add seven bytes to offset
    a += 0x07;
    x = a;
    y = 0x02; // set offset for relative coordinates
    FBallB();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetMiscBoundBox()
{
    a = x; // add nine bytes to offset
    a += 0x09;
    x = a;
    y = 0x06; // set offset for relative coordinates
    FBallB();
    return;
}

//------------------------------------------------------------------------

// get bounding box coordinates
void SMBEngine::FBallB()
{
    BoundingBoxCore();
    CheckRightScreenBBox(); // jump to handle any offscreen coordinates
    return;
}

//------------------------------------------------------------------------

void SMBEngine::LargePlatformCollision()
{
    // save value here
    writeData(PlatformCollisionFlag + x, 0xff);
    a = M(TimerControl); // check master timer control
    if (a != 0)
    {
        // get enemy object buffer offset and leave
        x = M(ObjectOffset);
        return;
    }
    a = M(Enemy_State + x); // if d7 set in object state,
    if ((a & 0x80) != 0)
    {
        // get enemy object buffer offset and leave
        x = M(ObjectOffset);
        return;
    }
    if (M(Enemy_ID + x) != 0x24)
    {
        ChkForPlayerC_LargeP(); // balance platform, branch if not found
        return;
    }
    x = M(Enemy_State + x); // set state as enemy offset here
    ChkForPlayerC_LargeP(); // perform code with state offset, then original offset, in X
    ChkForPlayerC_LargeP();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ChkForPlayerC_LargeP()
{
    bool collisionFound = false;
    bool playerVerticalOutOfRange = false;

    playerVerticalOutOfRange = CheckPlayerVertical(); // figure out if player is below a certain point
    if (playerVerticalOutOfRange)
    {
        // get enemy object buffer offset and leave
        x = M(ObjectOffset);
        return;
    }
    a = x;
    GetEnemyBoundBoxOfsArg(); // get bounding box offset in Y
    // store vertical coordinate in
    writeData(0x00, M(Enemy_Y_Position + x)); // temp variable for now
    a = x; // send offset we're on to the stack
    pha();
    collisionFound = PlayerCollisionCore(); // do player-to-platform collision detection
    pla(); // retrieve offset from the stack
    x = a;
    if (!collisionFound)
    {
        // get enemy object buffer offset and leave
        x = M(ObjectOffset);
        return;
    }
    ProcLPlatCollisions(); // otherwise collision, perform sub

    // get enemy object buffer offset and leave
    x = M(ObjectOffset);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::AutoControlPlayer()
{
    writeData(SavedJoypadBits, a); // override controller bits with contents of A if executing here
    PlayerCtrlRoutine();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PlayerCtrlRoutine()
{
    // check task here
    if (M(GameEngineSubroutine) == 0x0b)
        goto SizeChk;
    // are we in a water type area?
    if (M(AreaType) != 0)
        goto SaveJoyp; // if not, branch
    y = M(Player_Y_HighPos);
    --y; // if not in vertical area between
    if (y == 0)
    { // status bar and bottom, branch
        if (M(Player_Y_Position) < 0xd0)
            goto SaveJoyp; // not in the vertical area between status bar or bottom,
    } // DisJoyp: disable controller bits
    a = 0x00;
    writeData(SavedJoypadBits, 0x00);

SaveJoyp: // otherwise store A and B buttons in $0a
    a = M(SavedJoypadBits) & 0b11000000;
    writeData(A_B_Buttons, a);
    // store left and right buttons in $0c
    a = M(SavedJoypadBits) & 0b00000011;
    writeData(Left_Right_Buttons, a);
    // store up and down buttons in $0b
    a = M(SavedJoypadBits) & 0b00001100;
    writeData(Up_Down_Buttons, a);
    a &= 0b00000100; // check for pressing down
    if (a == 0)
        goto SizeChk; // if not, branch
    // check player's state
    if (M(Player_State) != 0)
        goto SizeChk; // if not on the ground, branch
    // check left and right
    if (M(Left_Right_Buttons) == 0)
        goto SizeChk; // if neither pressed, branch
    a = 0x00;
    writeData(Left_Right_Buttons, 0x00); // if pressing down while on the ground,
    writeData(Up_Down_Buttons, 0x00); // nullify directional bits

SizeChk: // run movement subroutines
    PlayerMovementSubs();
    y = 0x01; // is player small?
    if (M(PlayerSize) != 0)
        goto ChkMoveDir;
    y = 0x00; // check for if crouching
    if (M(CrouchingFlag) == 0)
        goto ChkMoveDir; // if not, branch ahead
    y = 0x02; // if big and crouching, load y with 2

ChkMoveDir: // set contents of Y as player's bounding box size control
    writeData(Player_BoundBoxCtrl, y);
    a = 0x01; // set moving direction to right by default
    y = M(Player_X_Speed); // check player's horizontal speed
    if (y != 0)
    { // if not moving at all horizontally, skip this part
        if ((y & 0x80) != 0)
        { // if moving to the right, use default moving direction
            a = 0x02; // otherwise change to move to the left
        } // SetMoveDir: set moving direction
        writeData(Player_MovingDir, a);
    } // PlayerSubs: move the screen if necessary
    ScrollHandler();
    GetPlayerOffscreenBits(); // get player's offscreen bits
    RelativePlayerPosition(); // get coordinates relative to the screen
    x = 0x00; // set offset for player object
    BoundingBoxCore(); // get player's bounding box coordinates
    PlayerBGCollision(); // do collision detection and process
    if (M(Player_Y_Position) < 0x40)
        goto PlayerHole; // if so, branch ahead
    a = M(GameEngineSubroutine);
    if (a == 0x05)
        goto PlayerHole;
    if (a == 0x07)
        goto PlayerHole;
    if (a < 0x04)
        goto PlayerHole;
    a = M(Player_SprAttrib) & 0b11011111; // otherwise nullify player's
    writeData(Player_SprAttrib, a); // background priority flag

PlayerHole: // check player's vertical high byte
    a = M(Player_Y_HighPos);
    if (((a - 0x02) & 0x80) != 0)
        return; // branch to leave if not that far down
    writeData(ScrollLock, 0x01); // set scroll lock
    writeData(0x07, 0x04); // set value here
    x = 0x00; // use X as flag, and clear for cloud level
    // check game timer expiration flag
    if (M(GameTimerExpiredFlag) == 0)
    { // if set, branch
        y = M(CloudTypeOverride); // check for cloud type override
        if (y != 0)
            goto ChkHoleX; // skip to last part if found
    } // HoleDie: set flag in X for player death
    x = 0x01;
    y = M(GameEngineSubroutine);
    if (y == 0x0b)
        goto ChkHoleX; // if so, branch ahead
    y = M(DeathMusicLoaded); // check value here
    if (y == 0)
    { // if already set, branch to next part
        ++y;
        writeData(EventMusicQueue, y); // otherwise play death music
        writeData(DeathMusicLoaded, y); // and set value here
    } // HoleBottom
    y = 0x06;
    writeData(0x07, 0x06); // change value here

ChkHoleX: // compare vertical high byte with value set here
    if (((a - M(0x07)) & 0x80) != 0)
        return; // if less, branch to leave
    --x; // otherwise decrement flag in X
    if ((x & 0x80) == 0)
    { // if flag was clear, branch to set modes and other values
        y = M(EventMusicBuffer); // check to see if music is still playing
        if (y != 0)
            return; // branch to leave if so
        a = 0x06; // otherwise set to run lose life routine
        writeData(GameEngineSubroutine, 0x06); // on next frame

        return; // ExitCtrl: leave

    //------------------------------------------------------------------------
    } // CloudExit
    a = 0x00;
    writeData(JoypadOverride, 0x00); // clear controller override bits if any are set
    SetEntr(); // do sub to set secondary mode
    ++M(AltEntranceControl); // set mode of entry to 3
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Vine_AutoClimb()
{
    // check to see whether player reached position
    if (M(Player_Y_HighPos) == 0)
    { // above the status bar yet and if so, set modes
        if (M(Player_Y_Position) < 0xe4)
        {
            SetEntr();
            return;
        }
    } // AutoClimb: set controller bits override to up
    a = 0b00001000;
    writeData(JoypadOverride, 0b00001000);
    y = 0x03; // set player state to climbing
    writeData(Player_State, 0x03);
    AutoControlPlayer();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::VerticalPipeEntry()
{
    a = 0x01; // set 1 as movement amount
    MovePlayerYAxis(); // do sub to move player downwards
    ScrollHandler(); // do sub to scroll screen with saved force if necessary
    y = 0x00; // load default mode of entry
    a = M(WarpZoneControl); // check warp zone control variable/flag
    if (a != 0)
    {
        ChgAreaPipe(); // if set, branch to use mode 0
        return;
    }
    y = 0x01;
    a = M(AreaType); // check for castle level type
    if (a != 0x03)
    {
        ChgAreaPipe(); // if not castle type level, use mode 1
        return;
    }
    y = 0x02;
    ChgAreaPipe(); // otherwise use mode 2
    return;
}

//------------------------------------------------------------------------

void SMBEngine::SideExitPipeEntry()
{
    EnterSidePipe(); // execute sub to move player to the right
    y = 0x02;
    ChgAreaPipe();
    return;
}

//------------------------------------------------------------------------

// decrement timer for change of area
void SMBEngine::ChgAreaPipe()
{
    --M(ChangeAreaTimer);
    if (M(ChangeAreaTimer) != 0)
    {
        return;
    }
    writeData(AltEntranceControl, y); // when timer expires set mode of alternate entry
    ChgAreaMode();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::EnterSidePipe()
{
    // set player's horizontal speed
    writeData(Player_X_Speed, 0x08);
    y = 0x01; // set controller right button by default
    // mask out higher nybble of player's
    a = M(Player_X_Position) & 0b00001111; // horizontal position
    if (a == 0)
    {
        writeData(Player_X_Speed, a); // if lower nybble = 0, set as horizontal speed
        y = a; // and nullify controller bit override here
    } // RightPipe: use contents of Y to
    a = y;
    AutoControlPlayer(); // execute player control routine with ctrl bits nulled
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PlayerDeath()
{
    a = M(TimerControl); // check master timer control
    if (a < 0xf0)
    { // branch to leave if before that point
        PlayerCtrlRoutine(); // otherwise run player control routine
        return;

    //------------------------------------------------------------------------
    } // ExitDeath
    return; // leave from death routine
}

//------------------------------------------------------------------------

void SMBEngine::FlagpoleSlide()
{
    a = M(Enemy_ID + 5); // check special use enemy slot
    if (a == FlagpoleFlagObject)
    { // if not found, branch to something residual
        // load flagpole sound
        writeData(Square1SoundQueue, M(FlagpoleSoundQueue)); // into square 1's sfx queue
        a = 0x00;
        writeData(FlagpoleSoundQueue, 0x00); // init flagpole sound queue
        if (M(Player_Y_Position) < 0x9e)
        { // far enough, and if so, branch with no controller bits set
            a = 0x04; // otherwise force player to climb down (to slide)
        } // SlidePlayer: jump to player control routine
        AutoControlPlayer();
        return;
    } // NoFPObj: increment to next routine (this may
    ++M(GameEngineSubroutine);
    return; // be residual code)
}

//------------------------------------------------------------------------

void SMBEngine::PlayerEntrance()
{
    // check for mode of alternate entry
    if (M(AltEntranceControl) != 0x02)
    { // if found, branch to enter from pipe or with vine
        a = 0x00;
        y = M(Player_Y_Position); // if vertical position above a certain
        if (y < 0x30)
        {
            AutoControlPlayer(); // with player movement code, do not return
            return;
        }
        a = M(PlayerEntranceCtrl); // check player entry bits from header
        if (a != 0x06)
        { // if set to 6 or 7, execute pipe intro code
            if (a != 0x07)
                goto PlayerRdy;
        } // ChkBehPipe: check for sprite attributes
        if (M(Player_SprAttrib) == 0)
        { // branch if found
            a = 0x01;
            AutoControlPlayer(); // force player to walk to the right
            return;
        } // IntroEntr: execute sub to move player to the right
        EnterSidePipe();
        --M(ChangeAreaTimer); // decrement timer for change of area
        if (M(ChangeAreaTimer) != 0)
            return; // branch to exit if not yet expired
        ++M(DisableIntermediate); // set flag to skip world and lives display
        NextArea(); // jump to increment to next area and set modes
        return;
    } // EntrMode2: if controller override bits set here,
    if (M(JoypadOverride) == 0)
    { // branch to enter with vine
        a = 0xff; // otherwise, set value here then execute sub
        MovePlayerYAxis(); // to move player upwards (note $ff = -1)
        a = M(Player_Y_Position); // check to see if player is at a specific coordinate
        if (a < 0x91)
            goto PlayerRdy; // to be at specific height to look/function right) branch
        return; // to the last part, otherwise leave

    //------------------------------------------------------------------------
    } // VineEntr
    a = M(VineHeight);
    if (a != 0x60)
        return; // if vine not yet reached maximum height, branch to leave
    a = M(Player_Y_Position); // get player's vertical coordinate
    y = 0x00; // load default values to be written to
    a = 0x01; // this value moves player to the right off the vine
    if (M(Player_Y_Position) >= 0x99)
    { // if vertical coordinate < preset value, use defaults
        writeData(Player_State, 0x03); // otherwise set player state to climbing
        y = 0x01; // increment value in Y
        a = 0x08; // set block in block buffer to cover hole, then
        writeData(Block_Buffer_1 + 0xb4, 0x08); // use same value to force player to climb
    } // OffVine: set collision detection disable flag
    writeData(DisableCollisionDet, y);
    AutoControlPlayer(); // use contents of A to move player up or right, execute sub
    a = M(Player_X_Position);
    if (a < 0x48)
        return; // if not far enough to the right, branch to leave

PlayerRdy: // set routine to be executed by game engine next frame
    writeData(GameEngineSubroutine, 0x08);
    // set to face player to the right
    writeData(PlayerFacingDir, 0x01);
    a = 0x00; // init A
    writeData(AltEntranceControl, 0x00); // init mode of entry
    writeData(DisableCollisionDet, 0x00); // init collision detection disable flag
    writeData(JoypadOverride, 0x00); // nullify controller override bits

    return; // ExitEntr: leave!
}

//------------------------------------------------------------------------

void SMBEngine::PlayerEndLevel()
{
    a = 0x01; // force player to walk to the right
    AutoControlPlayer();
    // check player's vertical position
    if (M(Player_Y_Position) < 0xae)
        goto ChkStop; // if player is not yet off the flagpole, skip this part
    // if scroll lock not set, branch ahead to next part
    if (M(ScrollLock) == 0)
        goto ChkStop; // because we only need to do this part once
    writeData(EventMusicQueue, EndOfLevelMusic); // load win level music in event music queue
    a = 0x00;
    writeData(ScrollLock, 0x00); // turn off scroll lock to skip this part later

ChkStop: // get player collision bits
    if ((M(Player_CollisionBits) & 0x01) == 0) // check for d0 set
    { // if d0 set, skip to next part
        // if star flag task control already set,
        if (M(StarFlagTaskControl) == 0)
        { // go ahead with the rest of the code
            ++M(StarFlagTaskControl); // otherwise set task control now (this gets ball rolling!)
        } // InCastle: set player's background priority bit to
        a = 0b00100000;
        writeData(Player_SprAttrib, 0b00100000); // give illusion of being inside the castle
    } // RdyNextA
    a = M(StarFlagTaskControl);
    if (a != 0x05)
    { // beyond last valid task number, branch to leave
        return;
    }
    ++M(LevelNumber); // increment level number used for game logic
    if (M(LevelNumber) != 0x03)
    {
        NextArea(); // and skip this last part here if not
        return;
    }
    y = M(WorldNumber); // get world number as offset
    // check third area coin tally for bonus 1-ups
    if (M(CoinTallyFor1Ups) < M(Hidden1UpCoinAmts + y))
    {
        NextArea(); // at least this number of coins, leave flag clear
        return;
    }
    ++M(Hidden1UpFlag); // otherwise set hidden 1-up box control flag
    NextArea();
    return;
}

//------------------------------------------------------------------------

// increment area number used for address loader
void SMBEngine::NextArea()
{
    ++M(AreaNumber);
    LoadAreaPointer(); // get new level pointer
    ++M(FetchNewGameTimerFlag); // set flag to load new game timer
    ChgAreaMode(); // do sub to set secondary mode, disable screen and sprite 0
    writeData(HalfwayPage, a); // reset halfway page to 0 (beginning)
    a = Silence;
    writeData(EventMusicQueue, Silence); // silence music and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GameRoutines()
{
    // run routine based on number (a few of these routines are
    switch (M(GameEngineSubroutine))
    {
    case 0:
        Entrance_GameTimerSetup();
        return;
    case 1:
        Vine_AutoClimb();
        return;
    case 2:
        SideExitPipeEntry();
        return;
    case 3:
        VerticalPipeEntry();
        return;
    case 4:
        FlagpoleSlide();
        return;
    case 5:
        PlayerEndLevel();
        return;
    case 6:
        PlayerLoseLife();
        return;
    case 7:
        PlayerEntrance();
        return;
    case 8:
        PlayerCtrlRoutine();
        return;
    case 9:
        goto PlayerChangeSize;
    case 10:
        goto PlayerInjuryBlink;
    case 11:
        PlayerDeath();
        return;
    case 12:
        PlayerFireFlower();
        return;
    default:
        bad_jump();
        return;
    } // merely placeholders as conditions for other routines)





//------------------------------------------------------------------------

PlayerChangeSize:
    a = M(TimerControl); // check master timer control
    if (a == 0xf8)
    { // branch if before or after that point
        goto InitChangeSize;
    } // EndChgSize: check again for another specific moment
    // otherwise run code to get growing/shrinking going
    if (a == 0xc4)
    { // and branch to leave if before or after that point
        DonePlayerTask(); // otherwise do sub to init timer control and set routine
    } // ExitChgSize: and then leave
    return;

//------------------------------------------------------------------------

PlayerInjuryBlink:
    a = M(TimerControl); // check master timer control
    if (a < 0xf0)
    { // branch if before that point
        if (a == 0xc8)
        {
            DonePlayerTask(); // branch if at that point, and not before or after
            return;
        }
        PlayerCtrlRoutine(); // otherwise run player control routine
        return;
    } // ExitBlink: do unconditional branch to leave
    if (a != 0xf0)
        return;
InitChangeSize:
    y = M(PlayerChangeSizeFlag); // if growing/shrinking flag already set
    if (y != 0)
        return; // then branch to leave
    writeData(PlayerAnimCtrl, y); // otherwise initialize player's animation frame control
    ++M(PlayerChangeSizeFlag); // set growing/shrinking flag
    a = M(PlayerSize) ^ 0x01; // invert player's size
    writeData(PlayerSize, a);

    return; // ExitBoth: leave
}



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

void SMBEngine::AreaParserTaskHandler()
{
    y = M(AreaParserTaskNum); // check number of tasks here
    if (y == 0)
    { // if already set, go ahead
        y = 0x08;
        writeData(AreaParserTaskNum, 0x08); // otherwise, set eight by default
    } // DoAPTasks
    --y;
    a = y;
    AreaParserTasks();
    --M(AreaParserTaskNum); // if all tasks not complete do not
    if (M(AreaParserTaskNum) == 0)
    { // render attribute table yet
        RenderAttributeTables();
    } // SkipATRender
    return;
}

//------------------------------------------------------------------------

void SMBEngine::AreaParserTasks()
{
    switch (a)
    {
    case 0:
        IncrementColumnPos();
        return;
    case 1:
        RenderAreaGraphics();
        return;
    case 2:
        RenderAreaGraphics();
        return;
    case 3:
        AreaParserCore();
        return;
    case 4:
        IncrementColumnPos();
        return;
    case 5:
        RenderAreaGraphics();
        return;
    case 6:
        RenderAreaGraphics();
        return;
    case 7:
        AreaParserCore();
        return;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

void SMBEngine::AreaParserCore()
{
    // check to see if we are starting right of start
    if (M(BackloadingFlag) != 0)
    { // if not, go ahead and render background, foreground and terrain
        ProcessAreaData(); // otherwise skip ahead and load level data
    } // RenderSceneryTerrain
    x = 0x0c;
    a = 0x00;

    do // ClrMTBuf: clear out metatile buffer
    {
        writeData(MetatileBuffer + x, 0x00);
        --x;
    } while ((x & 0x80) == 0);
    y = M(BackgroundScenery); // do we need to render the background scenery?
    if (y == 0)
        goto RendFore; // if not, skip to check the foreground
    a = M(CurrentPageLoc); // otherwise check for every third page

ThirdP:
    if (a >= 3)
    { // if less than three we're there
        a -= 0x03; // if 3 or more, subtract 3 and
        if ((a & 0x80) == 0)
            goto ThirdP; // do an unconditional branch
    } // RendBack: move results to higher nybble
    a <<= 1;
    a <<= 1;
    a <<= 1;
    a <<= 1;
    a += M(BSceneDataOffsets - 1 + y); // add to it offset loaded from here
    a += M(CurrentColumnPos); // add to the result our current column position
    x = a;
    a = M(BackSceneryData + x); // load data from sum of offsets
    if (a == 0)
        goto RendFore; // if zero, no scenery for that part
    pha();
    a &= 0x0f; // save to stack and clear high nybble
    a -= 0x01; // subtract one (because low nybble is $01-$0c)
    writeData(0x00, a); // save low nybble
    a <<= 1; // multiply by three (shift to left and add result to old one)
    a += M(0x00);
    x = a; // save as offset for background scenery metatile data
    pla(); // get high nybble from stack, move low
    a >>= 1;
    a >>= 1;
    a >>= 1;
    a >>= 1;
    y = a; // use as second offset (used to determine height)
    a = 0x03; // use previously saved memory location for counter
    writeData(0x00, 0x03);

    do // SceLoop1: load metatile data from offset of (lsb - 1) * 3
    {
        writeData(MetatileBuffer + y, M(BackSceneryMetatiles + x)); // store into buffer from offset of (msb / 16)
        ++x;
        ++y;
        if (y == 0x0b)
            goto RendFore;
        --M(0x00); // decrement until counter expires, barring exception
    } while (M(0x00) != 0);

RendFore: // check for foreground data needed or not
    x = M(ForegroundScenery);
    if (x != 0)
    { // if not, skip this part
        y = M(FSceneDataOffsets - 1 + x); // load offset from location offset by header value, then
        x = 0x00; // reinit X

        do // SceLoop2: load data until counter expires
        {
            a = M(ForeSceneryData + y);
            if (a != 0)
            { // do not store if zero found
                writeData(MetatileBuffer + x, a);
            } // NoFore
            ++y;
            ++x;
        } while (x != 0x0d);
    } // RendTerr: check world type for water level
    y = M(AreaType);
    if (y != 0)
        goto TerMTile; // if not water level, skip this part
    // check world number, if not world number eight
    if (M(WorldNumber) != World8)
        goto TerMTile;
    a = 0x62; // if set as water level and world number eight,
    StoreMT(); // use castle wall metatile as terrain type
    return;

TerMTile: // otherwise get appropriate metatile for area type
    a = M(TerrainMetatiles + y);
    // check for cloud type override
    if (M(CloudTypeOverride) == 0)
    {
        StoreMT(); // if not set, keep value otherwise
        return;
    }
    a = 0x88; // use cloud block terrain
    StoreMT();
    return;
}

//------------------------------------------------------------------------

// store value here
void SMBEngine::StoreMT()
{
    writeData(0x07, a);
    x = 0x00; // initialize X, use as metatile buffer offset
    a = M(TerrainControl); // use yet another value from the header
    a <<= 1; // multiply by 2 and use as yet another offset
    y = a;

TerrLoop: // get one of the terrain rendering bit data
    writeData(0x00, M(TerrainRenderBits + y));
    ++y; // increment Y and use as offset next time around
    writeData(0x01, y);
    // skip if value here is zero
    if (M(CloudTypeOverride) == 0)
        goto NoCloud2;
    if (x == 0x00)
        goto NoCloud2;
    // if not, mask out all but d3
    a = M(0x00) & 0b00001000;
    writeData(0x00, a);

NoCloud2: // start at beginning of bitmasks
    y = 0x00;

TerrBChk: // load bitmask, then perform AND on contents of first byte
    if ((M(Bitmasks + y) & M(0x00)) != 0)
    { // if not set, skip this part (do not write terrain to buffer)
        writeData(MetatileBuffer + x, M(0x07)); // load terrain type metatile number and store into buffer here
    } // NextTBit: continue until end of buffer
    ++x;
    if (x != 0x0d)
    { // if we're at the end, break out of this loop
        // check world type for underground area
        if (M(AreaType) != 0x02)
            goto EndUChk; // if not underground, skip this part
        if (x != 0x0b)
            goto EndUChk; // if we're at the bottom of the screen, override
        a = 0x54; // old terrain type with ground level terrain type
        writeData(0x07, 0x54);

EndUChk: // increment bitmasks offset in Y
        ++y;
        if (y != 0x08)
            goto TerrBChk; // if not all bits checked, loop back
        y = M(0x01);
        if (y != 0)
            goto TerrLoop; // unconditional branch, use Y to load next byte
    } // RendBBuf: do the area data loading routine now
    ProcessAreaData();
    a = M(BlockBufferColumnPos);
    GetBlockBufferAddr(); // get block buffer address from where we're at
    x = 0x00;
    y = 0x00; // init index regs and start at beginning of smaller buffer

    do // ChkMTLow
    {
        writeData(0x00, y);
        // load stored metatile number
        a = M(MetatileBuffer + x) & 0b11000000; // mask out all but 2 MSB
        a >>= 6; // make %xx000000 into %000000xx
        y = a; // use as offset in Y
        a = M(MetatileBuffer + x); // reload original unmasked value here
        if (a < M(BlockBuffLowBounds + y))
        { // if equal or greater, branch
            a = 0x00; // if less, init value before storing
        } // StrBlock: get offset for block buffer
        y = M(0x00);
        writeData(W(0x06) + y, a); // store value into block buffer
        a = y;
        a += 0x10;
        y = a;
        ++x; // increment column value
    } while (x < 0x0d); // continue until we pass last row, then leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ProcessAreaData()
{
ProcessAreaDataStart:
    x = 0x02; // start at the end of area object buffer

    do // ProcADLoop
    {
        writeData(ObjectOffset, x);
        // reset flag
        writeData(BehindAreaParserFlag, 0x00);
        y = M(AreaDataOffset); // get offset of area data pointer
        // get first byte of area object
        if (M(W(AreaData) + y) == 0xfd)
            goto RdyDecode;
        // check area object buffer flag
        if ((M(AreaObjectLength + x) & 0x80) == 0)
            goto RdyDecode; // if buffer not negative, branch, otherwise
        ++y;
        a = M(W(AreaData) + y); // get second byte of area object
        a <<= 1; // check for page select bit (d7), branch if not set
        if ((M(W(AreaData) + y) & 0x80) == 0)
            goto Chk1Row13;
        // check page select
        if (M(AreaObjectPageSel) != 0)
            goto Chk1Row13;
        ++M(AreaObjectPageSel); // if not already set, set it now
        ++M(AreaObjectPageLoc); // and increment page location

Chk1Row13:
        --y;
        // reread first byte of level object
        a = M(W(AreaData) + y) & 0x0f; // mask out high nybble
        if (a == 0x0d)
        {
            ++y; // if so, reread second byte of level object
            a = M(W(AreaData) + y);
            --y; // decrement to get ready to read first byte
            a &= 0b01000000; // check for d6 set (if not, object is page control)
            if (a != 0)
                goto CheckRear;
            // if page select is set, do not reread
            if (M(AreaObjectPageSel) != 0)
                goto CheckRear;
            ++y; // if d6 not set, reread second byte
            a = M(W(AreaData) + y) & 0b00011111; // mask out all but 5 LSB and store in page control
            writeData(AreaObjectPageLoc, a);
            ++M(AreaObjectPageSel); // increment page select
        } // Chk1Row14: row 14?
        else
        {
            if (a != 0x0e)
                goto CheckRear;
            // check flag for saved page number and branch if set
            if (M(BackloadingFlag) != 0)
                goto RdyDecode; // to render the object (otherwise bg might not look right)

CheckRear: // check to see if current page of level object is
            if (M(AreaObjectPageLoc) >= M(CurrentPageLoc))
            { // if so branch

RdyDecode: // do sub and do not turn on flag
                DecodeAreaData();
                goto ChkLength;
            } // SetBehind: turn on flag if object is behind renderer
            ++M(BehindAreaParserFlag);
        } // NextAObj: increment buffer offset and move on
        IncAreaObjOffset();

ChkLength: // get buffer offset
        x = M(ObjectOffset);
        // check object length for anything stored here
        if ((M(AreaObjectLength + x) & 0x80) == 0)
        { // if not, branch to handle loopback
            --M(AreaObjectLength + x); // otherwise decrement length or get rid of it
        } // ProcLoopb: decrement buffer offset
        --x;
    } while ((x & 0x80) == 0); // and loopback unless exceeded buffer
    // check for flag set if objects were behind renderer
    if (M(BehindAreaParserFlag) != 0)
        goto ProcessAreaDataStart; // branch if true to load more level data, otherwise
    a = M(BackloadingFlag); // check for flag set if starting right of page $00
    if (a != 0)
        goto ProcessAreaDataStart; // branch if true to load more level data, otherwise leave

    return;
}

//------------------------------------------------------------------------

    //------------------------------------------------------------------------

void SMBEngine::DecodeAreaData()
{
    // check current buffer flag
    if ((M(AreaObjectLength + x) & 0x80) == 0)
    {
        y = M(AreaObjOffsetBuffer + x); // if not, get offset from buffer
    } // Chk1stB: load offset of 16 for special row 15
    x = 0x10;
    a = M(W(AreaData) + y); // get first byte of level object again
    if (a == 0xfd)
        return; // if end of level, leave this routine
    a &= 0x0f; // otherwise, mask out low nybble
    if (a == 0x0f)
        goto ChkRow14; // if so, keep the offset of 16
    x = 0x08; // otherwise load offset of 8 for special row 12
    if (a == 0x0c)
        goto ChkRow14; // if so, keep the offset value of 8
    x = 0x00; // otherwise nullify value by default

ChkRow14: // store whatever value we just loaded here
    writeData(0x07, x);
    x = M(ObjectOffset); // get object offset again
    if (a == 0x0e)
    {
        // if so, load offset with $00
        writeData(0x07, 0x00);
        a = 0x2e; // and load A with another value
        if (a != 0)
        {
            NormObj(); // unconditional branch
            return;
        }
    } // ChkRow13: row 13?
    if (a == 0x0d)
    {
        // if so, load offset with 34
        writeData(0x07, 0x22);
        ++y; // get next byte
        a = M(W(AreaData) + y) & 0b01000000; // mask out all but d6 (page control obj bit)
        if (a == 0)
            return; // if d6 clear, branch to leave (we handled this earlier)
        // otherwise, get byte again
        a = M(W(AreaData) + y) & 0b01111111; // mask out d7
        if (a == 0x4b)
        { // (plus d6 set for object other than page control)
            ++M(LoopCommand); // if loop command, set loop command flag
        } // Mask2MSB: mask out d7 and d6
        a &= 0b00111111;
        NormObj(); // and jump
        return;
    } // ChkSRows: row 12-15?
    if (a < 0x0c)
    {
        ++y; // if not, get second byte of level object
        a = M(W(AreaData) + y) & 0b01110000; // mask out all but d6-d4
        if (a == 0)
        { // if any bits set, branch to handle large object
            writeData(0x07, 0x16); // otherwise set offset of 24 for small object
            // reload second byte of level object
            a = M(W(AreaData) + y) & 0b00001111; // mask out higher nybble and jump
            NormObj();
            return;
        } // LrgObj: store value here (branch for large objects)
        writeData(0x00, a);
        if (a != 0x70)
            goto NotWPipe;
        // if not, reload second byte
        a = M(W(AreaData) + y) & 0b00001000; // mask out all but d3 (usage control bit)
        if (a == 0)
            goto NotWPipe; // if d3 clear, branch to get original value
        a = 0x00; // otherwise, nullify value for warp pipe
        writeData(0x00, 0x00);

NotWPipe: // get value and jump ahead
        a = M(0x00);
    } // SpecObj: branch here for rows 12-15
    else
    {
        ++y;
        a = M(W(AreaData) + y) & 0b01110000; // get next byte and mask out all but d6-d4
    } // MoveAOId: move d6-d4 to lower nybble
    a >>= 1;
    a >>= 1;
    a >>= 1;
    a >>= 1;
    NormObj();
    return;
}

//------------------------------------------------------------------------

// store value here (branch for small objects and rows 13 and 14)
void SMBEngine::NormObj()
{
    writeData(0x00, a);
    // is there something stored here already?
    if ((M(AreaObjectLength + x) & 0x80) != 0)
    { // if so, branch to do its particular sub
        // otherwise check to see if the object we've loaded is on the
        if (M(AreaObjectPageLoc) != M(CurrentPageLoc))
        {
            y = M(AreaDataOffset); // if not, get old offset of level pointer
            // and reload first byte
            a = M(W(AreaData) + y) & 0b00001111;
            if (a != 0x0e)
                return;
            a = M(BackloadingFlag); // if so, check backloading flag
            if (a != 0)
                goto StrAObj; // if set, branch to render object, else leave

            return; // LeavePar

        //------------------------------------------------------------------------
        } // InitRear: check backloading flag to see if it's been initialized
        if (M(BackloadingFlag) != 0)
        { // branch to column-wise check
            a = 0x00; // if not, initialize both backloading and
            writeData(BackloadingFlag, 0x00); // behind-renderer flags and leave
            writeData(BehindAreaParserFlag, 0x00);
            writeData(ObjectOffset, 0x00);

            return; // LoopCmdE

        //------------------------------------------------------------------------
        } // BackColC: get first byte again
        y = M(AreaDataOffset);
        a = M(W(AreaData) + y) & 0b11110000; // mask out low nybble and move high to low
        a >>= 1;
        a >>= 1;
        a >>= 1;
        a >>= 1;
        if (a != M(CurrentColumnPos))
            return; // if not, branch to leave

StrAObj: // if so, load area obj offset and store in buffer
        writeData(AreaObjOffsetBuffer + x, M(AreaDataOffset));
        IncAreaObjOffset(); // do sub to increment to next object data
    } // RunAObj: get stored value and add offset to it
    a = M(0x00);
    a += M(0x07);
    switch (a)
    {
    case 0:
        VerticalPipe(); // used by warp pipes
        return;
    case 1:
        AreaStyleObject();
        return;
    case 2:
        RowOfBricks();
        return;
    case 3:
        RowOfSolidBlocks();
        return;
    case 4:
        RowOfCoins();
        return;
    case 5:
        ColumnOfBricks();
        return;
    case 6:
        ColumnOfSolidBlocks();
        return;
    case 7:
        VerticalPipe(); // used by decoration pipes
        return;
    case 8:
        Hole_Empty();
        return;
    case 9:
        PulleyRopeObject();
        return;
    case 10:
        Bridge_High();
        return;
    case 11:
        Bridge_Middle();
        return;
    case 12:
        Bridge_Low();
        return;
    case 13:
        Hole_Water();
        return;
    case 14:
        QuestionBlockRow_High();
        return;
    case 15:
        QuestionBlockRow_Low();
        return;
    case 16:
        EndlessRope();
        return;
    case 17:
        BalancePlatRope();
        return;
    case 18:
        CastleObject();
        return;
    case 19:
        StaircaseObject();
        return;
    case 20:
        ExitPipe();
        return;
    case 21:
        FlagBalls_Residual();
        return;
    case 22:
        QuestionBlock(); // power-up
        return;
    case 23:
        QuestionBlock(); // coin
        return;
    case 24:
        QuestionBlock(); // hidden, coin
        return;
    case 25:
        Hidden1UpBlock(); // hidden, 1-up
        return;
    case 26:
        BrickWithItem(); // brick, power-up
        return;
    case 27:
        BrickWithItem(); // brick, vine
        return;
    case 28:
        BrickWithItem(); // brick, star
        return;
    case 29:
        BrickWithCoins(); // brick, coins
        return;
    case 30:
        BrickWithItem(); // brick, 1-up
        return;
    case 31:
        WaterPipe();
        return;
    case 32:
        EmptyBlock();
        return;
    case 33:
        Jumpspring();
        return;
    case 34:
        IntroPipe();
        return;
    case 35:
        FlagpoleObject();
        return;
    case 36:
        AxeObj();
        return;
    case 37:
        ChainObj();
        return;
    case 38:
        CastleBridgeObj();
        return;
    case 39:
        ScrollLockObject_Warp();
        return;
    case 40:
        ScrollLockObject();
        return;
    case 41:
        ScrollLockObject();
        return;
    case 42:
        AreaFrenzy(); // flying cheep-cheeps
        return;
    case 43:
        AreaFrenzy(); // bullet bills or swimming cheep-cheeps
        return;
    case 44:
        AreaFrenzy(); // stop frenzy
        return;
    case 45:
        return;
    case 46:
        AlterAreaAttributes();
        return;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

void SMBEngine::Hidden1UpBlock()
{
    a = M(Hidden1UpFlag); // if flag not set, do not render object
    if (a == 0)
    {
        return;
    }

    a = 0x00; // if set, init for the next one
    writeData(Hidden1UpFlag, 0x00);
    BrickWithItem(); // jump to code shared with unbreakable bricks
    return;
}

//------------------------------------------------------------------------

void SMBEngine::QuestionBlock()
{
    GetAreaObjectID(); // get value from level decoder routine
    DrawQBlk(); // go to render it
    return;
}

//------------------------------------------------------------------------

void SMBEngine::BrickWithCoins()
{
    a = 0x00; // initialize multi-coin timer flag
    writeData(BrickCoinTimerFlag, 0x00);
    BrickWithItem();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::BrickWithItem()
{
    GetAreaObjectID(); // save area object ID
    writeData(0x07, y);
    a = 0x00; // load default adder for bricks with lines
    y = M(AreaType); // check level type for ground level
    --y;
    if (y != 0)
    { // if ground type, do not start with 5
        a = 0x05; // otherwise use adder for bricks without lines
    } // BWithL: add object ID to adder
    a += M(0x07);
    y = a; // use as offset for metatile

    DrawQBlk();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetAreaObjectID()
{
    a = M(0x00); // get value saved from area parser routine
    a -= 0x00; // possibly residual code
    y = a; // save to Y
    return;
}

//------------------------------------------------------------------------

void SMBEngine::UpdScrollVar()
{
    a = M(VRAM_Buffer_AddrCtrl);
    if (a == 0x06)
        return; // then branch to leave
    // otherwise check number of tasks
    if (M(AreaParserTaskNum) == 0)
    {
        a = M(ScrollThirtyTwo); // get horizontal scroll in 0-31 or $00-$20 range
        if (((a - 0x20) & 0x80) != 0)
            return; // branch to leave if not
        a = M(ScrollThirtyTwo);
        a -= 0x20; // otherwise subtract $20 to set appropriately
        writeData(ScrollThirtyTwo, a); // and store
        a = 0x00; // reset vram buffer offset used in conjunction with
        writeData(VRAM_Buffer2_Offset, 0x00); // level graphics buffer at $0341-$035f
    } // RunParser: update the name table with more level graphics
    AreaParserTaskHandler();

    return; // ExitEng: and after all that, we're finally done!
}

//------------------------------------------------------------------------

void SMBEngine::RunSmallPlatform()
{
    GetEnemyOffscreenBits();
    RelativeEnemyPosition();
    SmallPlatformBoundBox();
    SmallPlatformCollision();
    RelativeEnemyPosition();
    DrawSmallPlatform();
    MoveSmallPlatform();
    OffscreenBoundsCheck();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RunLargePlatform()
{
    GetEnemyOffscreenBits();
    RelativeEnemyPosition();
    LargePlatformBoundBox();
    LargePlatformCollision();
    // if master timer control set,
    if (M(TimerControl) == 0)
    { // skip subroutine tree
        LargePlatformSubroutines();
    } // SkipPT
    RelativeEnemyPosition();
    DrawLargePlatform();
    OffscreenBoundsCheck();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::SmallPlatformBoundBox()
{
    // store bitmask here for now
    writeData(0x00, 0x08);
    y = 0x04; // store another bitmask here for now
    GetMaskedOffScrBits();
    return;
}


//------------------------------------------------------------------------

void SMBEngine::LargePlatformBoundBox()
{
    ++x; // increment X to get the proper offset
    GetXOffscreenBits(); // then jump directly to the sub for horizontal offscreen bits
    --x; // decrement to return to original offset
    if (a >= 0xfe)
    {
        MoveBoundBoxOffscreen(); // box offscreen, otherwise start getting coordinates
        return;
    }

    SetupEOffsetFBBox();
    return;
}







//------------------------------------------------------------------------


//------------------------------------------------------------------------

void SMBEngine::HurtBowser()
{
    --M(BowserHitPoints); // decrement bowser's hit points
    if (M(BowserHitPoints) != 0)
        return; // if bowser still has hit points, branch to leave
    InitVStf(); // otherwise do sub to init vertical speed and movement force
    writeData(Enemy_X_Speed + x, a); // initialize horizontal speed
    writeData(EnemyFrenzyBuffer, a); // init enemy frenzy buffer
    writeData(Enemy_Y_Speed + x, 0xfe); // set vertical speed to make defeated bowser jump a little
    y = M(WorldNumber); // use world number as offset
    // get enemy identifier to replace bowser with
    writeData(Enemy_ID + x, M(BowserIdentities + y)); // set as new enemy identifier
    a = 0x20; // set A to use starting value for state
    if (y < 0x03)
    { // branch if so
        a = 0x23; // otherwise add 3 to enemy state
    } // SetDBSte: set defeated enemy state
    writeData(Enemy_State + x, a);
    writeData(Square2SoundQueue, Sfx_BowserFall); // load bowser defeat sound
    x = M(0x01); // get enemy offset
    a = 0x09; // award 5000 points to player for defeating bowser
    EnemySmackScore(); // unconditional branch to award points
    return;
}



//------------------------------------------------------------------------

void SMBEngine::EnemiesCollision()
{
    bool collisionFound = false;

    // check counter for d0 set
    a = M(FrameCounter) >> 1;
    if ((M(FrameCounter) & 0x01) == 0)
        return; // if d0 not set, leave
    a = M(AreaType);
    if (a == 0)
        return; // if water area type, leave
    a = M(Enemy_ID + x);
    if (a >= 0x15)
        goto ExitECRoutine;
    if (a == Lakitu)
        goto ExitECRoutine;
    if (a == PiranhaPlant)
        goto ExitECRoutine;
    a = M(EnemyOffscrBitsMasked + x); // if masked offscreen bits nonzero, branch to leave
    if (a != 0)
        goto ExitECRoutine;
    GetEnemyBoundBoxOfs(); // otherwise, do sub, get appropriate bounding box offset for
    --x; // first enemy we're going to compare, then decrement for second
    if ((x & 0x80) != 0)
        goto ExitECRoutine; // branch to leave if there are no other enemies

    do // ECLoop: save enemy object buffer offset for second enemy here
    {
        writeData(0x01, x);
        a = y; // save first enemy's bounding box offset to stack
        pha();
        // check enemy object enable flag
        if (M(Enemy_Flag + x) == 0)
            goto ReadyNextEnemy; // branch if flag not set
        a = M(Enemy_ID + x);
        if (a >= 0x15)
            goto ReadyNextEnemy; // branch if true
        if (a == Lakitu)
            goto ReadyNextEnemy; // branch if enemy object is lakitu
        if (a == PiranhaPlant)
            goto ReadyNextEnemy; // branch if enemy object is piranha plant
        if (M(EnemyOffscrBitsMasked + x) != 0)
            goto ReadyNextEnemy; // branch if masked offscreen bits set
        a = x; // get second enemy object's bounding box offset
        a <<= 1; // multiply by four, then add four
        a <<= 1;
        a += 0x04;
        x = a; // use as new contents of X
        collisionFound = SprObjectCollisionCore(); // do collision detection using the two enemies here
        x = M(ObjectOffset); // use first enemy offset for X
        y = M(0x01); // use second enemy offset for Y
        if (collisionFound)
        { // no collision, branch ahead of this
            a = M(Enemy_State + x) | M(Enemy_State + y); // check both enemy states for d7 set
            a &= 0b10000000;
            if (a == 0)
            { // branch if at least one of them is set
                // load first enemy's collision-related bits
                a = M(Enemy_CollisionBits + y) & M(SetBitsMask + x); // check to see if bit connected to second enemy is
                if (a != 0)
                    goto ReadyNextEnemy; // already set, and move onto next enemy slot if set
                a = M(Enemy_CollisionBits + y) | M(SetBitsMask + x); // if the bit is not set, set it now
                writeData(Enemy_CollisionBits + y, a);
            } // YesEC: react according to the nature of collision
            ProcEnemyCollisions();
            goto ReadyNextEnemy; // move onto next enemy slot
        } // NoEnemyCollision
        // load first enemy's collision-related bits
        a = M(Enemy_CollisionBits + y) & M(ClearBitsMask + x); // clear bit connected to second enemy
        writeData(Enemy_CollisionBits + y, a); // then move onto next enemy slot

ReadyNextEnemy:
        pla(); // get first enemy's bounding box offset from the stack
        y = a; // use as Y again
        x = M(0x01); // get and decrement second enemy's object buffer offset
        --x;
    } while ((x & 0x80) == 0); // loop until all enemy slots have been checked

ExitECRoutine:
    x = M(ObjectOffset); // get enemy object buffer offset
    return; // leave
}

//------------------------------------------------------------------------

void SMBEngine::ProcEnemyCollisions()
{
    // check both enemy states for d5 set
    a = M(Enemy_State + y) | M(Enemy_State + x);
    a &= 0b00100000; // if d5 is set in either state, or both, branch
    if (a != 0)
        return; // to leave and do nothing else at this point
    if (M(Enemy_State + x) >= 0x06)
    {
        a = M(Enemy_ID + x); // check second enemy identifier for hammer bro
        if (a == HammerBro)
            return;
        if ((M(Enemy_State + y) & 0x80) != 0) // check first enemy state for d7 set
        { // branch if d7 is clear
            a = 0x06;
            SetupFloateyNumber(); // award 1000 points for killing enemy
            ShellOrBlockDefeat(); // then kill enemy, then load
            y = M(0x01); // original offset of second enemy
        } // ShellCollisions
        a = y; // move Y to X
        x = a;
        ShellOrBlockDefeat(); // kill second enemy
        x = M(ObjectOffset);
        a = M(ShellChainCounter + x); // get chain counter for shell
        a += 0x04; // add four to get appropriate point offset
        x = M(0x01);
        SetupFloateyNumber(); // award appropriate number of points for second enemy
        x = M(ObjectOffset); // load original offset of first enemy
        ++M(ShellChainCounter + x); // increment chain counter for additional enemies

        return; // ExitProcessEColl: leave!!!

    //------------------------------------------------------------------------
    } // ProcSecondEnemyColl
    // if first enemy state < $06, branch elsewhere
    if (M(Enemy_State + y) >= 0x06)
    {
        a = M(Enemy_ID + y); // check first enemy identifier for hammer bro
        if (a == HammerBro)
            return;
        ShellOrBlockDefeat(); // otherwise, kill first enemy
        y = M(0x01);
        a = M(ShellChainCounter + y); // get chain counter for shell
        a += 0x04; // add four to get appropriate point offset
        x = M(ObjectOffset);
        SetupFloateyNumber(); // award appropriate number of points for first enemy
        x = M(0x01); // load original offset of second enemy
        ++M(ShellChainCounter + x); // increment chain counter for additional enemies
        return; // leave!!!

    //------------------------------------------------------------------------
    } // MoveEOfs
    a = y; // move Y ($01) to X
    x = a;
    EnemyTurnAround(); // do the sub here using value from $01
    x = M(ObjectOffset); // then do it again using value from $08
    EnemyTurnAround();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ProcFireball_Bubble()
{
    // check player's status
    if (M(PlayerStatus) >= 0x02)
    { // if not fiery, branch
        a = M(A_B_Buttons) & B_Button; // check for b button pressed
        if (a == 0)
            goto ProcFireballs; // branch if not pressed
        a &= M(PreviousA_B_Buttons);
        if (a != 0)
            goto ProcFireballs; // if button pressed in previous frame, branch
        // load fireball counter
        a = M(FireballCounter) & 0b00000001; // get LSB and use as offset for buffer
        x = a;
        a = M(Fireball_State + x); // load fireball state
        if (a != 0)
            goto ProcFireballs; // if not inactive, branch
        y = M(Player_Y_HighPos); // if player too high or too low, branch
        --y;
        if (y != 0)
            goto ProcFireballs;
        a = M(CrouchingFlag); // if player crouching, branch
        if (a != 0)
            goto ProcFireballs;
        a = M(Player_State); // if player's state = climbing, branch
        if (a == 0x03)
            goto ProcFireballs;
        // play fireball sound effect
        writeData(Square1SoundQueue, Sfx_Fireball);
        a = 0x02; // load state
        writeData(Fireball_State + x, 0x02);
        y = M(PlayerAnimTimerSet); // copy animation frame timer setting
        writeData(FireballThrowingTimer, y); // into fireball throwing timer
        --y;
        writeData(PlayerAnimTimer, y); // decrement and store in player's animation timer
        ++M(FireballCounter); // increment fireball counter

ProcFireballs:
        x = 0x00;
        FireballObjCore(); // process first fireball object
        x = 0x01;
        FireballObjCore(); // process second fireball object, then do air bubbles
    } // ProcAirBubbles
    a = M(AreaType); // if not water type level, skip the rest of this
    if (a == 0)
    {
        x = 0x02; // otherwise load counter and use as offset

        do // BublLoop: store offset
        {
            writeData(ObjectOffset, x);
            BubbleCheck(); // check timers and coordinates, create air bubble
            RelativeBubblePosition(); // get relative coordinates
            GetBubbleOffscreenBits(); // get offscreen information
            DrawBubble(); // draw the air bubble
            --x;
        } while ((x & 0x80) == 0); // do this until all three are handled
    } // BublExit: then leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::FireballObjCore()
{
    uint32_t wide = 0;

    writeData(ObjectOffset, x); // store offset as current object
    if ((M(Fireball_State + x) & 0x80) == 0) // check for d7 = 1
    { // if so, branch to get relative coordinates and draw explosion
        y = M(Fireball_State + x); // if fireball inactive, branch to leave
        if (y != 0)
        {
            --y; // if fireball state set to 1, skip this part and just run it
            if (y != 0)
            {
                // get player's horizontal position
                wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + 0x04; // add four pixels and store as fireball's horizontal position
                writeData(Fireball_X_Position + x, LOBYTE(wide));
                writeData(Fireball_PageLoc + x, HIBYTE(wide));
                a = HIBYTE(wide);// get player's page location
                // get player's vertical position and store
                writeData(Fireball_Y_Position + x, M(Player_Y_Position));
                // set high byte of vertical position
                writeData(Fireball_Y_HighPos + x, 0x01);
                y = M(PlayerFacingDir); // get player's facing direction
                --y; // decrement to use as offset here
                // set horizontal speed of fireball accordingly
                writeData(Fireball_X_Speed + x, M(FireballXSpdData + y));
                // set vertical speed of fireball
                writeData(Fireball_Y_Speed + x, 0x04);
                a = 0x07;
                writeData(Fireball_BoundBoxCtrl + x, 0x07); // set bounding box size control for fireball
                --M(Fireball_State + x); // decrement state to 1 to skip this part from now on
            } // RunFB: add 7 to offset to use
            a = x;
            a += 0x07;
            x = a;
            // set downward movement force here
            writeData(0x00, 0x50);
            // set maximum speed here
            writeData(0x02, 0x03);
            a = 0x00;
            ImposeGravity(); // do sub here to impose gravity on fireball and move vertically
            MoveObjectHorizontally(); // do another sub to move it horizontally
            x = M(ObjectOffset); // return fireball offset to X
            RelativeFireballPosition(); // get relative coordinates
            GetFireballOffscreenBits(); // get offscreen information
            GetFireballBoundBox(); // get bounding box coordinates
            FireballBGCollision(); // do fireball to background collision detection
            // get fireball offscreen bits
            a = M(FBall_OffscreenBits) & 0b11001100; // mask out certain bits
            if (a == 0)
            { // if any bits still set, branch to kill fireball
                FireballEnemyCollision(); // do fireball to enemy collision detection and deal with collisions
                DrawFireball(); // draw fireball appropriately and leave
                return;
            } // EraseFB: erase fireball state
            a = 0x00;
            writeData(Fireball_State + x, 0x00);
        } // NoFBall: leave
        return;

    //------------------------------------------------------------------------
    } // FireballExplosion
    RelativeFireballPosition();
    DrawExplosion_Fireball();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::FireballEnemyCollision()
{
    bool shiftedBit = false;
    bool collisionFound = false;

    a = M(Fireball_State + x); // check to see if fireball state is set at all
    if (a == 0)
        goto ExitFBallEnemy; // branch to leave if not
    shiftedBit = (a & 0x80) != 0;
    a <<= 1;
    if (shiftedBit)
        goto ExitFBallEnemy; // branch to leave also if d7 in state is set
    a = M(FrameCounter) >> 1; // get LSB of frame counter
    if ((M(FrameCounter) & 0x01) != 0)
        goto ExitFBallEnemy; // branch to leave if set (do routine every other frame)
    a = x;
    a <<= 1; // multiply fireball offset by four
    a <<= 1;
    a += 0x1c; // then add $1c or 28 bytes to it
    y = a; // to use fireball's bounding box coordinates
    x = 0x04;

    do // FireballEnemyCDLoop
    {
        writeData(0x01, x); // store enemy object offset here
        a = y;
        pha(); // push fireball offset to the stack
        a = M(Enemy_State + x) & 0b00100000; // check to see if d5 is set in enemy state
        if (a != 0)
            goto NoFToECol; // if so, skip to next enemy slot
        // check to see if buffer flag is set
        if (M(Enemy_Flag + x) == 0)
            goto NoFToECol; // if not, skip to next enemy slot
        a = M(Enemy_ID + x); // check enemy identifier
        if (a >= 0x24)
        { // if < $24, branch to check further
            if (a < 0x2b)
                goto NoFToECol; // if in range $24-$2a, skip to next enemy slot
        } // GoombaDie: check for goomba identifier
        if (a == Goomba)
        { // if not found, continue with code
            // otherwise check for defeated state
            if (M(Enemy_State + x) >= 0x02)
                goto NoFToECol; // skip to next enemy slot
        } // NotGoomba: if any masked offscreen bits set,
        if (M(EnemyOffscrBitsMasked + x) != 0)
            goto NoFToECol; // skip to next enemy slot
        a = x;
        a <<= 1; // otherwise multiply enemy offset by four
        a <<= 1;
        a += 0x04; // add 4 bytes to it
        x = a; // to use enemy's bounding box coordinates
        collisionFound = SprObjectCollisionCore(); // do fireball-to-enemy collision detection
        x = M(ObjectOffset); // return fireball's original offset
        if (!collisionFound)
            goto NoFToECol; // no collision, thus do next enemy slot
        a = 0b10000000;
        writeData(Fireball_State + x, 0b10000000); // set d7 in enemy state
        x = M(0x01); // get enemy offset
        HandleEnemyFBallCol(); // jump to handle fireball to enemy collision

NoFToECol: // pull fireball offset from stack
        pla();
        y = a; // put it in Y
        x = M(0x01); // get enemy object offset
        --x; // decrement it
    } while ((x & 0x80) == 0); // loop back until collision detection done on all enemies

ExitFBallEnemy:
    x = M(ObjectOffset); // get original fireball offset and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::HandleEnemyFBallCol()
{
    RelativeEnemyPosition(); // get relative coordinate of enemy
    x = M(0x01); // get current enemy object offset
    a = M(Enemy_Flag + x); // check buffer flag for d7 set
    if ((a & 0x80) != 0)
    { // branch if not set to continue
        a &= 0b00001111; // otherwise mask out high nybble and
        x = a; // use low nybble as enemy offset
        a = M(Enemy_ID + x);
        if (a == Bowser)
        {
            HurtBowser(); // branch if found
            return;
        }
        x = M(0x01); // otherwise retrieve current enemy offset
    } // ChkBuzzyBeetle
    a = M(Enemy_ID + x);
    if (a == BuzzyBeetle)
        return; // branch if found to leave (buzzy beetles fireproof)
    if (a != Bowser)
        goto ChkOtherEnemies;
    HurtBowser();
    return;

ChkOtherEnemies:
    if (a == BulletBill_FrenzyVar)
        return; // branch to leave if bullet bill (frenzy variant)
    if (a == Podoboo)
        return; // branch to leave if podoboo
    if (a >= 0x15)
        return; // branch to leave if identifier => $15
    ShellOrBlockDefeat();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::KillEnemyAboveBlock()
{
    ShellOrBlockDefeat(); // do this sub to kill enemy
    a = 0xfc; // alter vertical speed of enemy and leave
    writeData(Enemy_Y_Speed + x, 0xfc);
    return;
}

//------------------------------------------------------------------------

// do a sub here to move enemy downwards
void SMBEngine::FallE()
{
    MoveD_EnemyVertically();
    y = 0x00;
    a = M(Enemy_State + x); // check for enemy state $02
    if (a != 0x02)
    { // if found, branch to move enemy horizontally
        a &= 0b01000000; // check for d6 set
        if (a == 0)
        {
            SteadM(); // if not set, branch to something else
            return;
        }
        a = M(Enemy_ID + x);
        if (a == PowerUpObject)
        {
            SteadM();
            return;
        }
        if (a != PowerUpObject)
            goto SlowM; // if any other object where d6 set, jump to set Y
    } // MEHor: jump here to move enemy horizontally for <> $2e and d6 set
    MoveEnemyHorizontally();
    return;

SlowM: // if branched here, increment Y to slow horizontal movement
    y = 0x01;
    SteadM();
    return;
}

//------------------------------------------------------------------------

// get current horizontal speed
void SMBEngine::SteadM()
{
    a = M(Enemy_X_Speed + x);
    pha(); // save to stack
    if ((a & 0x80) != 0)
    { // if not moving or moving right, skip, leave Y alone
        ++y;
        ++y; // otherwise increment Y to next data
    } // AddHS
    a += M(XSpeedAdderData + y); // add value here to slow enemy down if necessary
    writeData(Enemy_X_Speed + x, a); // save as horizontal speed temporarily
    MoveEnemyHorizontally(); // then do a sub to move horizontally
    pla();
    writeData(Enemy_X_Speed + x, a); // get old horizontal speed from stack and return to
    return; // original memory location, then leave
}

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

void SMBEngine::RunGameTimer()
{
    a = M(OperMode); // get primary mode of operation
    if (a == 0)
        return; // branch to leave if in title screen mode
    a = M(GameEngineSubroutine);
    if (a < 0x08)
        return; // branch to leave
    if (a == 0x0b)
        return; // branch to leave
    a = M(Player_Y_HighPos);
    if (a >= 0x02)
        return; // branch to leave regardless of level type
    a = M(GameTimerCtrlTimer); // if game timer control not yet expired,
    if (a != 0)
        return; // branch to leave
    a = M(GameTimerDisplay) | M(GameTimerDisplay + 1); // otherwise check game timer digits
    a |= M(GameTimerDisplay + 2);
    if (a != 0)
    { // if game timer digits at 000, branch to time-up code
        y = M(GameTimerDisplay); // otherwise check first digit
        --y; // if first digit not on 1,
        if (y != 0)
            goto ResGTCtrl; // branch to reset game timer control
        // otherwise check second and third digits
        a = M(GameTimerDisplay + 1) | M(GameTimerDisplay + 2);
        if (a != 0)
            goto ResGTCtrl; // if timer not at 100, branch to reset game timer control
        a = TimeRunningOutMusic;
        writeData(EventMusicQueue, TimeRunningOutMusic); // otherwise load time running out music

ResGTCtrl: // reset game timer control
        writeData(GameTimerCtrlTimer, 0x18);
        y = 0x23; // set offset for last digit
        a = 0xff; // set value to decrement game timer digit
        writeData(DigitModifier + 5, 0xff);
        DigitsMathRoutine(); // do sub to decrement game timer slowly
        a = 0xa4; // set status nybbles to update game timer display
        PrintStatusBarNumbers(); // do sub to update the display
        return;
    } // TimeUpOn: init player status (note A will always be zero here)
    writeData(PlayerStatus, a);
    ForceInjury(); // do sub to kill the player (note player is small here)
    ++M(GameTimerExpiredFlag); // set game timer expiration flag

    return; // ExGTimer: leave
}

//------------------------------------------------------------------------

void SMBEngine::ProcHammerObj()
{
    uint32_t wide = 0;

    // if master timer control set
    if (M(TimerControl) != 0)
        goto RunHSubs; // skip all of this code and go to last subs at the end
    // otherwise get hammer's state
    a = M(Misc_State + x) & 0b01111111; // mask out d7
    y = M(HammerEnemyOffset + x); // get enemy object offset that spawned this hammer
    if (a != 0x02)
    { // if currently at 2, branch
        if (a >= 0x02)
            goto SetHPos; // if greater than 2, branch elsewhere
        a = x;
        a += 0x0d; // proper misc object
        x = a; // return offset to X
        writeData(0x00, 0x10); // set downward movement force
        writeData(0x01, 0x0f); // set upward movement force (not used)
        writeData(0x02, 0x04); // set maximum vertical speed
        a = 0x00; // set A to impose gravity on hammer
        ImposeGravity(); // do sub to impose gravity on hammer and move vertically
        MoveObjectHorizontally(); // do sub to move it horizontally
        x = M(ObjectOffset); // get original misc object offset
    } // SetHSpd
    else // branch to essential subroutines
    {
        writeData(Misc_Y_Speed + x, 0xfe); // set hammer's vertical speed
        // get enemy object state
        a = M(Enemy_State + y) & 0b11110111; // mask out d3
        writeData(Enemy_State + y, a); // store new state
        x = M(Enemy_MovingDir + y); // get enemy's moving direction
        --x; // decrement to use as offset
        a = M(HammerXSpdData + x); // get proper speed to use based on moving direction
        x = M(ObjectOffset); // reobtain hammer's buffer offset
        writeData(Misc_X_Speed + x, a); // set hammer's horizontal speed

SetHPos: // decrement hammer's state
        --M(Misc_State + x);
        // get enemy's horizontal position
        wide = ((M(Enemy_PageLoc + y) << 8) | M(Enemy_X_Position + y)) + 0x02; // set position 2 pixels to the right
        writeData(Misc_X_Position + x, LOBYTE(wide)); // store as hammer's horizontal position
        writeData(Misc_PageLoc + x, HIBYTE(wide)); // store as hammer's page location
        a = HIBYTE(wide); // get enemy's page location
        a = M(Enemy_Y_Position + y); // get enemy's vertical position
        a -= 0x0a; // move position 10 pixels upward
        writeData(Misc_Y_Position + x, a); // store as hammer's vertical position
        a = 0x01;
        writeData(Misc_Y_HighPos + x, 0x01); // set hammer's vertical high byte
        if (a != 0)
            goto RunHSubs; // unconditional branch to skip first routine
    } // RunAllH: handle collisions
    PlayerHammerCollision();

RunHSubs: // get offscreen information
    GetMiscOffscreenBits();
    RelativeMiscPosition(); // get relative coordinates
    GetMiscBoundBox(); // get bounding box coordinates
    DrawHammer(); // draw the hammer
    return; // and we are done here
}

//------------------------------------------------------------------------

void SMBEngine::MiscObjectsCore()
{
    bool shiftedBit = false;
    uint32_t wide = 0;

    x = 0x08; // set at end of misc object buffer

    do // MiscLoop: store misc object offset here
    {
        writeData(ObjectOffset, x);
        a = M(Misc_State + x); // check misc object state
        if (a == 0)
            goto MiscLoopBack; // branch to check next slot
        shiftedBit = (a & 0x80) != 0;
        a <<= 1; // otherwise take d7
        if (shiftedBit)
        { // if d7 not set, jumping coin, thus skip to rest of code here
            ProcHammerObj(); // otherwise go to process hammer,
            goto MiscLoopBack; // then check next slot
        } // ProcJumpCoin
        y = M(Misc_State + x); // check misc object state
        --y; // decrement to see if it's set to 1
        if (y != 0)
        { // if so, branch to handle jumping coin
            ++M(Misc_State + x); // otherwise increment state to either start off or as timer
            // get horizontal coordinate for misc object
            wide = ((M(Misc_PageLoc + x) << 8) | M(Misc_X_Position + x)) + M(ScrollAmount); // add current scroll speed
            writeData(Misc_X_Position + x, LOBYTE(wide)); // store as new horizontal coordinate
            writeData(Misc_PageLoc + x, HIBYTE(wide)); // store as new page location
            a = HIBYTE(wide); // get page location
            if (M(Misc_State + x) != 0x30)
                goto RunJCSubs; // if not yet reached, branch to subroutines
            a = 0x00;
            writeData(Misc_State + x, 0x00); // otherwise nullify object state
            goto MiscLoopBack; // and move onto next slot
        } // JCoinRun
        a = x;
        a += 0x0d;
        x = a;
        // set downward movement amount
        writeData(0x00, 0x50);
        // set maximum vertical speed
        writeData(0x02, 0x06);
        // divide by 2 and set
        writeData(0x01, 0x03); // as upward movement amount (apparently residual)
        a = 0x00; // set A to impose gravity on jumping coin
        ImposeGravity(); // do sub to move coin vertically and impose gravity on it
        x = M(ObjectOffset); // get original misc object offset
        // check vertical speed
        if (M(Misc_Y_Speed + x) != 0x05)
            goto RunJCSubs; // if not moving downward fast enough, keep state as-is
        ++M(Misc_State + x); // otherwise increment state to change to floatey number

RunJCSubs: // get relative coordinates
        RelativeMiscPosition();
        GetMiscOffscreenBits(); // get offscreen information
        GetMiscBoundBox(); // get bounding box coordinates (why?)
        JCoinGfxHandler(); // draw the coin or floatey number

MiscLoopBack:
        --x; // decrement misc object offset
    } while ((x & 0x80) == 0); // loop back until all misc objects handled
    return; // then leave
}

//------------------------------------------------------------------------

void SMBEngine::RunFirebarObj()
{
    ProcFirebar();
    OffscreenBoundsCheck();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ProcFirebar()
{
    GetEnemyOffscreenBits(); // get offscreen information
    // check for d3 set
    a = M(Enemy_OffscreenBits) & 0b00001000; // if so, branch to leave
    if (a == 0)
    {
        // if master timer control set, branch
        if (M(TimerControl) == 0)
        { // ahead of this part
            a = M(FirebarSpinSpeed + x); // load spinning speed of firebar
            FirebarSpin(); // modify current spinstate
            a &= 0b00011111; // mask out all but 5 LSB
            writeData(FirebarSpinState_High + x, a); // and store as new high byte of spinstate
        } // SusFbar: get high byte of spinstate
        a = M(FirebarSpinState_High + x);
        // check enemy identifier
        if (M(Enemy_ID + x) < 0x1f)
            goto SetupGFB; // if < $1f (long firebar), branch
        if (a != 0x08)
        { // if eight, branch to change
            if (a != 0x18)
                goto SetupGFB; // if not at twenty-four branch to not change
        } // SkpFSte
        a += 0x01; // add one to spinning thing to avoid horizontal state
        writeData(FirebarSpinState_High + x, a);

SetupGFB: // save high byte of spinning thing, modified or otherwise
        writeData(0xef, a);
        RelativeEnemyPosition(); // get relative coordinates to screen
        GetFirebarPosition(); // do a sub here (residual, too early to be used now)
        y = M(Enemy_SprDataOffset + x); // get OAM data offset
        a = M(Enemy_Rel_YPos); // get relative vertical coordinate
        writeData(Sprite_Y_Position + y, a); // store as Y in OAM data
        writeData(0x07, a); // also save here
        a = M(Enemy_Rel_XPos); // get relative horizontal coordinate
        writeData(Sprite_X_Position + y, a); // store as X in OAM data
        writeData(0x06, a); // also save here
        a = 0x01;
        writeData(0x00, 0x01); // set $01 value here (not necessary)
        FirebarCollision(); // draw fireball part and do collision detection
        y = 0x05; // load value for short firebars by default
        if (M(Enemy_ID + x) >= 0x1f)
        { // no, branch then
            y = 0x0b; // otherwise load value for long firebars
        } // SetMFbar: store maximum value for length of firebars
        writeData(0xed, y);
        a = 0x00;
        writeData(0x00, 0x00); // initialize counter here

        do // DrawFbar: load high byte of spinstate
        {
            a = M(0xef);
            GetFirebarPosition(); // get fireball position data depending on firebar part
            DrawFirebar_Collision(); // position it properly, draw it and do collision detection
            // check which firebar part
            if (M(0x00) == 0x04)
            {
                y = M(DuplicateObj_Offset); // if we arrive at fifth firebar part,
                // get offset from long firebar and load OAM data offset
                writeData(0x06, M(Enemy_SprDataOffset + y)); // using long firebar offset, then store as new one here
            } // NextFbar: move onto the next firebar part
            ++M(0x00);
            a = M(0x00);
        } while (a < M(0xed)); // otherwise go back and do another
    } // SkipFBar
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawFirebar_Collision()
{
    bool shiftedBit = false;

    // store mirror data elsewhere
    writeData(0x05, M(0x03));
    y = M(0x06); // load OAM data offset for firebar
    a = M(0x01); // load horizontal adder we got from position loader
    shiftedBit = (M(0x05) & 0x01) != 0;
    M(0x05) >>= 1; // shift LSB of mirror data
    if (!shiftedBit)
    { // if the bit was set, skip this part
        a ^= 0xff;
        a += 0x01; // otherwise get two's compliment of horizontal adder
    } // AddHA: add horizontal coordinate relative to screen to
    a += M(Enemy_Rel_XPos); // horizontal adder, modified or otherwise
    writeData(Sprite_X_Position + y, a); // store as X coordinate here
    writeData(0x06, a); // store here for now, note offset is saved in Y still
    if (a < M(Enemy_Rel_XPos))
    { // if sprite coordinate => original coordinate, branch
        a = M(Enemy_Rel_XPos);
        a -= M(0x06); // original one and skip this part
    } // SubtR1: subtract original X from the
    else
    {
        a -= M(Enemy_Rel_XPos); // current sprite X
    } // ChkFOfs: if difference of coordinates within a certain range,
    if (a >= 0x59)
    { // continue by handling vertical adder
        a = 0xf8; // otherwise, load offscreen Y coordinate
        if (a != 0)
            goto SetVFbr; // and unconditionally branch to move sprite offscreen
    } // VAHandl: if vertical relative coordinate offscreen,
    a = M(Enemy_Rel_YPos);
    if (a == 0xf8)
        goto SetVFbr;
    a = M(0x02); // load vertical adder we got from position loader
    shiftedBit = (M(0x05) & 0x01) != 0;
    M(0x05) >>= 1; // shift LSB of mirror data one more time
    if (!shiftedBit)
    { // if the bit was set, skip this part
        a ^= 0xff;
        a += 0x01; // otherwise get two's compliment of second part
    } // AddVA: add vertical coordinate relative to screen to
    a += M(Enemy_Rel_YPos); // the second data, modified or otherwise

SetVFbr: // store as Y coordinate here
    writeData(Sprite_Y_Position + y, a);
    writeData(0x07, a); // also store here for now
    FirebarCollision();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::FirebarCollision()
{
    DrawFirebar(); // run sub here to draw current tile of firebar
    a = y; // return OAM data offset and save
    pha(); // to the stack for now
    // if star mario invincibility timer
    a = M(StarInvincibleTimer) | M(TimerControl); // or master timer controls set
    if (a != 0)
        goto NoColFB; // then skip all of this
    writeData(0x05, a); // otherwise initialize counter
    y = M(Player_Y_HighPos);
    --y; // if player's vertical high byte offscreen,
    if (y != 0)
        goto NoColFB; // skip all of this
    y = M(Player_Y_Position); // get player's vertical position
    // get player's size
    if (M(PlayerSize) == 0)
    { // if player small, branch to alter variables
        if (M(CrouchingFlag) == 0)
            goto BigJp; // if player big and not crouching, jump ahead
    } // AdjSm: if small or big but crouching, execute this part
    ++M(0x05);
    ++M(0x05); // first increment our counter twice (setting $02 as flag)
    a = y;
    a += 0x18; // vertical coordinate
    y = a;

BigJp: // get vertical coordinate, altered or otherwise, from Y
    a = y;

FBCLoop: // subtract vertical position of firebar
    a -= M(0x07); // from the vertical coordinate of the player
    if ((a & 0x80) != 0)
    { // if player lower on the screen than firebar,
        a ^= 0xff; // skip two's compliment part
        a += 0x01;
    } // ChkVFBD: if difference => 8 pixels, skip ahead of this part
    if (a >= 0x08)
        goto Chk2Ofs;
    // if firebar on far right on the screen, skip this,
    if (M(0x06) >= 0xf0)
        goto Chk2Ofs;
    a = M(Sprite_X_Position + 4); // get OAM X coordinate for sprite #1
    a += 0x04; // add four pixels
    writeData(0x04, a); // store here
    a -= M(0x06); // from the X coordinate of player's sprite 1
    if ((a & 0x80) != 0)
    { // if modded X coordinate to the right of firebar
        a ^= 0xff; // skip two's compliment part
        a += 0x01;
    } // ChkFBCl: if difference < 8 pixels, collision, thus branch
    if (a >= 0x08)
    { // to process

Chk2Ofs: // if value of $02 was set earlier for whatever reason,
        if (M(0x05) == 0x02)
            goto NoColFB;
        y = M(0x05); // otherwise get temp here and use as offset
        a = M(Player_Y_Position);
        a += M(FirebarYPos + y); // add value loaded with offset to player's vertical coordinate
        ++M(0x05); // then increment temp and jump back
        goto FBCLoop;
    } // ChgSDir: set movement direction by default
    x = 0x01;
    // if OAM X coordinate of player's sprite 1
    if (M(0x04) < M(0x06))
    { // then do not alter movement direction
        x = 0x02; // otherwise increment it
    } // SetSDir: store movement direction here
    writeData(Enemy_MovingDir, x);
    x = 0x00;
    a = M(0x00); // save value written to $00 to stack
    pha();
    InjurePlayer(); // perform sub to hurt or kill player
    pla();
    writeData(0x00, a); // get value of $00 from stack

NoColFB: // get OAM data offset
    pla();
    a += 0x04;
    writeData(0x06, a);
    x = M(ObjectOffset); // get enemy object buffer offset and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PlayerHammerCollision()
{
    bool collisionFound = false;

    // get frame counter
    a = M(FrameCounter) >> 1; // take d0
    if ((M(FrameCounter) & 0x01) == 0)
        return; // branch to leave if d0 not set to execute every other frame
    // if either master timer control
    a = M(TimerControl) | M(Misc_OffscreenBits); // or any offscreen bits for hammer are set,
    if (a != 0)
        return; // branch to leave
    a = x;
    a <<= 1; // multiply misc object offset by four
    a <<= 1;
    a += 0x24; // add 36 or $24 bytes to get proper offset
    y = a; // for misc object bounding box coordinates
    collisionFound = PlayerCollisionCore(); // do player-to-hammer collision detection
    x = M(ObjectOffset); // get misc object offset
    if (collisionFound)
    { // if no collision, then branch
        a = M(Misc_Collision_Flag + x); // otherwise read collision flag
        if (a != 0)
            return; // if collision flag already set, branch to leave
        writeData(Misc_Collision_Flag + x, 0x01); // otherwise set collision flag now
        a = M(Misc_X_Speed + x) ^ 0xff; // get two's compliment of
        a += 0x01;
        writeData(Misc_X_Speed + x, a); // set to send hammer flying the opposite direction
        a = M(StarInvincibleTimer); // if star mario invincibility timer set,
        if (a != 0)
            return; // branch to leave
        InjurePlayer(); // otherwise jump to hurt player, do not return
        return;
    } // ClHCol: clear collision flag
    a = 0x00;
    writeData(Misc_Collision_Flag + x, 0x00);

    return; // ExPHC
}




//------------------------------------------------------------------------

void SMBEngine::ProcessCannons()
{
    a = M(AreaType); // get area type
    if (a != 0)
    { // if water type area, branch to leave
        x = 0x02;

        do // ThreeSChk: start at third enemy slot
        {
            writeData(ObjectOffset, x);
            // check enemy buffer flag
            if (M(Enemy_Flag + x) != 0)
                goto Chk_BB; // if set, branch to check enemy
            // otherwise get part of LSFR
            y = M(SecondaryHardMode); // get secondary hard mode flag, use as offset
            a = M(PseudoRandomBitReg + 1 + x) & M(CannonBitmasks + y); // mask out bits of LSFR as decided by flag
            if (a >= 0x06)
                goto Chk_BB; // if so, branch to check enemy
            y = a; // transfer masked contents of LSFR to Y as pseudorandom offset
            // get page location
            if (M(Cannon_PageLoc + y) == 0)
                goto Chk_BB; // if not set or on page 0, branch to check enemy
            a = M(Cannon_Timer + y); // get cannon timer
            if (a != 0)
            { // if expired, branch to fire cannon
                --a; // otherwise subtract the borrow (note carry will always be clear here)
                writeData(Cannon_Timer + y, a); // to count timer down
                goto Chk_BB; // then jump ahead to check enemy
            } // FireCannon
            // if master timer control set,
            if (M(TimerControl) != 0)
                goto Chk_BB; // branch to check enemy
            // otherwise we start creating one
            writeData(Cannon_Timer + y, 0x0e); // first, reset cannon timer
            // get page location of cannon
            writeData(Enemy_PageLoc + x, M(Cannon_PageLoc + y)); // save as page location of bullet bill
            // get horizontal coordinate of cannon
            writeData(Enemy_X_Position + x, M(Cannon_X_Position + y)); // save as horizontal coordinate of bullet bill
            a = M(Cannon_Y_Position + y); // get vertical coordinate of cannon
            a -= 0x08; // subtract eight pixels (because enemies are 24 pixels tall)
            writeData(Enemy_Y_Position + x, a); // save as vertical coordinate of bullet bill
            writeData(Enemy_Y_HighPos + x, 0x01); // set vertical high byte of bullet bill
            writeData(Enemy_Flag + x, 0x01); // set buffer flag
            writeData(Enemy_State + x, 0x00); // then initialize enemy's state
            writeData(Enemy_BoundBoxCtrl + x, 0x09); // set bounding box size control for bullet bill
            a = BulletBill_CannonVar;
            writeData(Enemy_ID + x, BulletBill_CannonVar); // load identifier for bullet bill (cannon variant)
            goto Next3Slt; // move onto next slot

Chk_BB: // check enemy identifier for bullet bill (cannon variant)
            a = M(Enemy_ID + x);
            if (a != BulletBill_CannonVar)
                goto Next3Slt; // if not found, branch to get next slot
            OffscreenBoundsCheck(); // otherwise, check to see if it went offscreen
            a = M(Enemy_Flag + x); // check enemy buffer flag
            if (a == 0)
                goto Next3Slt; // if not set, branch to get next slot
            GetEnemyOffscreenBits(); // otherwise, get offscreen information
            BulletBillHandler(); // then do sub to handle bullet bill

Next3Slt: // move onto next slot
            --x;
        } while ((x & 0x80) == 0); // do this until first three slots are checked
    } // ExCannon: then leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::BulletBillHandler()
{
    bool enemyRightOfPlayer = false;

    // if master timer control set,
    if (M(TimerControl) == 0)
    { // branch to run subroutines except movement sub
        if (M(Enemy_State + x) == 0)
        { // if bullet bill's state set, branch to check defeated state
            // otherwise load offscreen bits
            a = M(Enemy_OffscreenBits) & 0b00001100; // mask out bits
            if (a == 0b00001100)
                goto KillBB; // if so, branch to kill this object
            y = 0x01; // set to move right by default
            enemyRightOfPlayer = PlayerEnemyDiff(); // get horizontal difference between player and bullet bill
            if ((a & 0x80) == 0)
            { // if enemy to the left of player, branch
                ++y; // otherwise increment to move left
            } // SetupBB: set bullet bill's moving direction
            writeData(Enemy_MovingDir + x, y);
            --y; // decrement to use as offset
            // get horizontal speed based on moving direction
            writeData(Enemy_X_Speed + x, M(BulletBillXSpdData + y)); // and store it
            a = (uint8_t)(M(0x00) + 0x28 + (enemyRightOfPlayer ? 1 : 0)); // get horizontal difference, add 40 pixels plus the no-borrow left above
            if (a < 0x50)
                goto KillBB; // to cannon either on left or right side, thus branch
            writeData(Enemy_State + x, 0x01); // otherwise set bullet bill's state
            writeData(EnemyFrameTimer + x, 0x0a); // set enemy frame timer
            a = Sfx_Blast;
            writeData(Square2SoundQueue, Sfx_Blast); // play fireworks/gunfire sound
        } // ChkDSte: check enemy state for d5 set
        a = M(Enemy_State + x) & 0b00100000;
        if (a != 0)
        { // if not set, skip to move horizontally
            MoveD_EnemyVertically(); // otherwise do sub to move bullet bill vertically
        } // BBFly: do sub to move bullet bill horizontally
        MoveEnemyHorizontally();
    } // RunBBSubs: get offscreen information
    GetEnemyOffscreenBits();
    RelativeEnemyPosition(); // get relative coordinates
    GetEnemyBoundBox(); // get bounding box coordinates
    PlayerEnemyCollision(); // handle player to enemy collisions
    EnemyGfxHandler(); // draw the bullet bill and leave
    return;

KillBB: // kill bullet bill and leave
    EraseEnemyObject();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RunBowserFlame()
{
    ProcBowserFlame();
    GetEnemyOffscreenBits();
    RelativeEnemyPosition();
    GetEnemyBoundBox();
    PlayerEnemyCollision();
    OffscreenBoundsCheck();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveD_Bowser()
{
        MoveEnemySlowVert(); // do a sub to move bowser downwards
        BowserGfxHandler(); // jump to draw bowser's front and rear, then leave
        return;
}

//------------------------------------------------------------------------

void SMBEngine::RunBowser()
{
    bool enemyRightOfPlayer = false;
    bool hammerSpawned = false;

    // if d5 in enemy state is not set
    a = M(Enemy_State + x) & 0b00100000; // then branch elsewhere to run bowser
    if (a == 0)
        goto BowserControl;
    // otherwise check vertical position
    if (M(Enemy_Y_Position + x) < 0xe0)
    {
        MoveD_Bowser(); // otherwise proceed to KillAllEnemies
        return;
    }
    KillAllEnemies();
    return;

    //------------------------------------------------------------------------
BowserControl:
    writeData(EnemyFrenzyBuffer, 0x00); // empty frenzy buffer
    // if master timer control not set,
    if (M(TimerControl) != 0)
    { // skip jump and execute code here
        goto SkipToFB; // otherwise, jump over a bunch of code
    } // ChkMouth: check bowser's mouth
    if ((M(BowserBodyControls) & 0x80) != 0)
    { // if bit clear, go ahead with code here
        goto HammerChk; // otherwise skip a whole section starting here
    } // FeetTmr: decrement timer to control bowser's feet
    --M(BowserFeetCounter);
    if (M(BowserFeetCounter) == 0)
    { // if not expired, skip this part
        // otherwise, reset timer
        writeData(BowserFeetCounter, 0x20);
        // and invert bit used
        a = M(BowserBodyControls) ^ 0b00000001; // to control bowser's feet
        writeData(BowserBodyControls, a);
    } // ResetMDr: check frame counter
    a = M(FrameCounter) & 0b00001111; // if not on every sixteenth frame, skip
    if (a == 0)
    { // ahead to continue code
        a = 0x02; // otherwise reset moving/facing direction every
        writeData(Enemy_MovingDir + x, 0x02); // sixteen frames
    } // B_FaceP: if timer set here expired,
    if (M(EnemyFrameTimer + x) == 0)
        goto GetPRCmp; // branch to next section
    enemyRightOfPlayer = PlayerEnemyDiff(); // get horizontal difference between player and bowser,
    if ((a & 0x80) == 0)
        goto GetPRCmp; // and branch if bowser to the right of the player
    writeData(Enemy_MovingDir + x, 0x01); // set bowser to move and face to the right
    writeData(BowserMovementSpeed, 0x02); // set movement speed
    writeData(EnemyFrameTimer + x, 0x20); // set timer here
    writeData(BowserFireBreathTimer, 0x20); // set timer used for bowser's flame
    if (M(Enemy_X_Position + x) >= 0xc8)
        goto HammerChk; // skip ahead to some other section

GetPRCmp: // get frame counter
    a = M(FrameCounter) & 0b00000011;
    if (a != 0)
        goto HammerChk; // execute this code every fourth frame, otherwise branch
    if (M(Enemy_X_Position + x) == M(BowserOrigXPos))
    { // branch to skip this part
        a = M(PseudoRandomBitReg + x) & 0b00000011; // get pseudorandom offset
        y = a;
        // load value using pseudorandom offset
        writeData(MaxRangeFromOrigin, M(PRandomRange + y)); // and store here
    } // GetDToO
    a = M(Enemy_X_Position + x);
    a += M(BowserMovementSpeed); // coordinate and save as new horizontal position
    writeData(Enemy_X_Position + x, a);
    if (M(Enemy_MovingDir + x) == 0x01)
        goto HammerChk;
    y = 0xff; // set default movement speed here (move left)
    a -= M(BowserOrigXPos); // horizontal position
    if ((a & 0x80) != 0)
    { // if current position to the right of original, skip ahead
        a ^= 0xff;
        a += 0x01;
        y = 0x01; // set alternate movement speed here (move right)
    } // CompDToO: compare difference with pseudorandom value
    if (a < M(MaxRangeFromOrigin))
        goto HammerChk; // if difference < pseudorandom value, leave speed alone
    writeData(BowserMovementSpeed, y); // otherwise change bowser's movement speed

HammerChk: // if timer set here not expired yet, skip ahead to
    a = M(EnemyFrameTimer + x);
    if (a == 0)
    { // some other section of code
        MoveEnemySlowVert(); // otherwise start by moving bowser downwards
        // check world number
        if (M(WorldNumber) < World6)
            goto SetHmrTmr; // if world 1-5, skip this part (not time to throw hammers yet)
        a = M(FrameCounter) & 0b00000011; // check to see if it's time to execute sub
        if (a != 0)
            goto SetHmrTmr; // if not, skip sub, otherwise
        hammerSpawned = SpawnHammerObj(); // execute sub on every fourth frame to spawn misc object (hammer)

SetHmrTmr: // get current vertical position
        if (M(Enemy_Y_Position + x) < 0x80)
        {
            ChkFireB(); // then skip to world number check for flames
            return;
        }
        a = M(PseudoRandomBitReg + x) & 0b00000011; // get pseudorandom offset
        y = a;
        // get value using pseudorandom offset
        writeData(EnemyFrameTimer + x, M(PRandomRange + y)); // set for timer here

SkipToFB: // jump to execute flames code
        ChkFireB();
        return;
    } // MakeBJump: if timer not yet about to expire,
    if (a != 0x01)
    {
        ChkFireB(); // skip ahead to next part
        return;
    }
    --M(Enemy_Y_Position + x); // otherwise decrement vertical coordinate
    InitVStf(); // initialize movement amount
    a = 0xfe;
    writeData(Enemy_Y_Speed + x, 0xfe); // set vertical speed to move bowser upwards
    ChkFireB();
    return;
}

//------------------------------------------------------------------------

// check world number here
void SMBEngine::ChkFireB()
{
ChkFireB:
    a = M(WorldNumber);
    if (a != World8)
    { // if so, execute this part here
        if (a >= World6)
        {
            BowserGfxHandler(); // if so, skip this part here
            return;
        }
    } // SpawnFBr: check timer here
    if (M(BowserFireBreathTimer) != 0)
    {
        BowserGfxHandler(); // if not expired yet, skip all of this
        return;
    }
    writeData(BowserFireBreathTimer, 0x20); // set timer here
    a = M(BowserBodyControls) ^ 0b10000000; // invert bowser's mouth bit to open
    writeData(BowserBodyControls, a); // and close bowser's mouth
    if ((a & 0x80) != 0)
        goto ChkFireB; // if bowser's mouth open, loop back
    SetFlameTimer(); // get timing for bowser's flame
    if (M(SecondaryHardMode) != 0)
    { // if secondary hard mode flag not set, skip this
        a -= 0x10; // otherwise subtract from value in A
    } // SetFBTmr: set value as timer here
    writeData(BowserFireBreathTimer, a);
    a = BowserFlame; // put bowser's flame identifier
    writeData(EnemyFrenzyBuffer, BowserFlame); // in enemy frenzy buffer
    BowserGfxHandler();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::BowserGfxHandler()
{
    ProcessBowserHalf(); // do a sub here to process bowser's front
    y = 0x10; // load default value here to position bowser's rear
    if ((M(Enemy_MovingDir + x) & 0x01) != 0) // check moving direction
    { // if moving left, use default
        y = 0xf0; // otherwise load alternate positioning value here
    } // CopyFToR: move bowser's rear object position value to A
    a = y;
    a += M(Enemy_X_Position + x); // add to bowser's front object horizontal coordinate
    y = M(DuplicateObj_Offset); // get bowser's rear object offset
    writeData(Enemy_X_Position + y, a); // store A as bowser's rear horizontal coordinate
    a = M(Enemy_Y_Position + x);
    a += 0x08; // vertical coordinate and store as vertical coordinate
    writeData(Enemy_Y_Position + y, a); // for bowser's rear
    writeData(Enemy_State + y, M(Enemy_State + x)); // copy enemy state directly from front to rear
    writeData(Enemy_MovingDir + y, M(Enemy_MovingDir + x)); // copy moving direction also
    a = M(ObjectOffset); // save enemy object offset of front to stack
    pha();
    x = M(DuplicateObj_Offset); // put enemy object offset of rear as current
    writeData(ObjectOffset, x);
    a = Bowser; // set bowser's enemy identifier
    writeData(Enemy_ID + x, Bowser); // store in bowser's rear object
    ProcessBowserHalf(); // do a sub here to process bowser's rear
    pla();
    writeData(ObjectOffset, a); // get original enemy object offset
    x = a;
    a = 0x00; // nullify bowser's front/rear graphics flag
    writeData(BowserGfxFlag, 0x00);
    return;
}

//------------------------------------------------------------------------

    //------------------------------------------------------------------------

void SMBEngine::ProcessBowserHalf()
{
    ++M(BowserGfxFlag); // increment bowser's graphics flag, then run subroutines
    RunRetainerObj(); // to get offscreen bits, relative position and draw bowser (finally!)
    a = M(Enemy_State + x);
    if (a != 0)
        return; // if either enemy object not in normal state, branch to leave
    a = 0x0a;
    writeData(Enemy_BoundBoxCtrl + x, 0x0a); // set bounding box size control
    GetEnemyBoundBox(); // get bounding box coordinates
    PlayerEnemyCollision(); // do player-to-enemy collision detection
    return;
}


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

// check enemy buffer flag for non-active enemy slot
void SMBEngine::ChkNoEn()
{
ChkNoEn:
    a = M(Enemy_Flag + x);
    if (a != 0)
    { // branch out of loop if found
        --x; // otherwise check next slot
        if ((x & 0x80) == 0)
            goto ChkNoEn; // branch until all slots are checked
        if ((x & 0x80) != 0)
        {
            // if no empty slots were found, branch to leave
            x = M(ObjectOffset);
            return;
        }
    } // CreateL: initialize enemy state
    writeData(Enemy_State + x, 0x00);
    a = Lakitu; // create lakitu enemy object
    writeData(Enemy_ID + x, Lakitu);
    SetupLakitu(); // do a sub to set up lakitu
    a = 0x20;
    PutAtRightExtent(); // finish setting up lakitu
    x = M(ObjectOffset);
    return; // ExLSHand
}

//------------------------------------------------------------------------

void SMBEngine::PutAtRightExtent()
{
    uint32_t wide = 0;

    writeData(Enemy_Y_Position + x, a); // set vertical position
    wide = ((M(ScreenRight_PageLoc) << 8) | M(ScreenRight_X_Pos)) + 0x20; // place enemy 32 pixels beyond right side of screen
    writeData(Enemy_X_Position + x, LOBYTE(wide));
    writeData(Enemy_PageLoc + x, HIBYTE(wide));
    a = HIBYTE(wide);
    FinishFlame();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::FinishFlame()
{
    // set $08 for bounding box control
    writeData(Enemy_BoundBoxCtrl + x, 0x08);
    // set high byte of vertical and
    writeData(Enemy_Y_HighPos + x, 0x01); // enemy buffer flag
    writeData(Enemy_Flag + x, 0x01);
    a = 0x00;
    writeData(Enemy_X_MoveForce + x, 0x00); // initialize horizontal movement force, and
    writeData(Enemy_State + x, 0x00); // enemy state
    return;
}

//------------------------------------------------------------------------

void SMBEngine::LakituAndSpinyHandler()
{
    a = M(FrenzyEnemyTimer); // if timer here not expired, leave
    if (a != 0)
        return;
    if (x >= 0x05)
        return;
    a = 0x80; // set timer
    writeData(FrenzyEnemyTimer, 0x80);
    y = 0x04; // start with the last enemy slot
    ChkLak();
    return;
}

//------------------------------------------------------------------------

// check all enemy slots to see
void SMBEngine::ChkLak()
{
ChkLak:
    if (M(Enemy_ID + y) == Lakitu)
        goto CreateSpiny;
    --y; // otherwise check another slot
    if ((y & 0x80) == 0)
        goto ChkLak; // loop until all slots are checked
    ++M(LakituReappearTimer); // increment reappearance timer
    a = M(LakituReappearTimer);
    if (a < 0x07)
        return; // if not, leave
    x = 0x04; // start with the last enemy slot again
    ChkNoEn();
    return;

    //------------------------------------------------------------------------
CreateSpiny:
    a = M(Player_Y_Position); // if player above a certain point, branch to leave
    if (a < 0x2c)
        return;
    a = M(Enemy_State + y); // if lakitu is not in normal state, branch to leave
    if (a != 0)
        return;
    // store horizontal coordinates (high and low) of lakitu
    writeData(Enemy_PageLoc + x, M(Enemy_PageLoc + y)); // into the coordinates of the spiny we're going to create
    writeData(Enemy_X_Position + x, M(Enemy_X_Position + y));
    // put spiny within vertical screen unit
    writeData(Enemy_Y_HighPos + x, 0x01);
    a = M(Enemy_Y_Position + y); // put spiny eight pixels above where lakitu is
    a -= 0x08;
    writeData(Enemy_Y_Position + x, a);
    // get 2 LSB of LSFR and save to Y
    a = M(PseudoRandomBitReg + x) & 0b00000011;
    y = a;
    x = 0x02;

    do // DifLoop: get three values and save them
    {
        writeData(0x01 + x, M(PRDiffAdjustData + y)); // to $01-$03
        ++y;
        ++y; // increment Y four bytes for each value
        ++y;
        ++y;
        --x; // decrement X for each one
    } while ((x & 0x80) == 0); // loop until all three are written
    x = M(ObjectOffset); // get enemy object buffer offset
    PlayerLakituDiff(); // move enemy, change direction, get value - difference

    // LYNN: I think this code has no effect because of
    // https://tcrf.net/Super_Mario_Bros.#Unused_Spiny_Egg_Behavior
    //
    // // check player's horizontal speed
    // if (M(Player_X_Speed) < 0x08)
    // { // if moving faster than a certain amount, branch elsewhere
    //     y = a; // otherwise save value in A to Y for now
    //     a = M(PseudoRandomBitReg + 1 + x) & 0b00000011; // get one of the LSFR parts and save the 2 LSB
    //     if (a != 0)
    //     { // branch if neither bits are set
    //         a = y;
    //         a ^= 0b11111111; // otherwise get two's compliment of Y
    //         y = a;
    //         ++y;
    //     } // UsePosv: put value from A in Y back to A (they will be lost anyway)
    //     a = y;
    // } // SetSpSpd: set bounding box control, init attributes, lose contents of A

    SmallBBox();
    // y = 0x02;
    writeData(Enemy_X_Speed + x, 0); // set horizontal speed to zero because previous contents
    // branch if ((a & 0x80) == 0) never taken for the same reason
    y = 0x01;
    writeData(Enemy_MovingDir + x, 0x01);
    writeData(Enemy_Y_Speed + x, 0xfd); // set vertical speed to move upwards
    writeData(Enemy_Flag + x, 0x01); // enable enemy object by setting flag
    a = 0x05;
    writeData(Enemy_State + x, 0x05); // put spiny in egg state and leave

    return; // ChpChpEx
}

//------------------------------------------------------------------------

    //------------------------------------------------------------------------

void SMBEngine::InitBowserFlame()
{
    a = M(FrenzyEnemyTimer); // if timer not expired yet, branch to leave
    if (a != 0)
        return;
    writeData(Enemy_Y_MoveForce + x, a); // reset something here
    a = M(NoiseSoundQueue) | Sfx_BowserFlame; // load bowser's flame sound into queue
    writeData(NoiseSoundQueue, a);
    y = M(BowserFront_Offset); // get bowser's buffer offset
    // check for bowser
    if (M(Enemy_ID + y) == Bowser)
        goto SpawnFromMouth;
    SetFlameTimer(); // get timer data based on flame counter
    a += 0x20; // add 32 frames by default
    if (M(SecondaryHardMode) != 0)
    { // if secondary mode flag not set, use as timer setting
        a -= 0x10; // otherwise subtract 16 frames for secondary hard mode
    } // SetFrT: set timer accordingly
    writeData(FrenzyEnemyTimer, a);
    a = M(PseudoRandomBitReg + x) & 0b00000011; // get 2 LSB from first part of LSFR
    writeData(BowserFlamePRandomOfs + x, a); // set here
    y = a; // use as offset
    a = M(FlameYPosData + y); // load vertical position based on pseudorandom offset
    PutAtRightExtent();
    return;
SpawnFromMouth:
    a = M(Enemy_X_Position + y); // get bowser's horizontal position
    a -= 0x0e; // subtract 14 pixels
    writeData(Enemy_X_Position + x, a); // save as flame's horizontal position
    writeData(Enemy_PageLoc + x, M(Enemy_PageLoc + y)); // copy page location from bowser to flame
    a = M(Enemy_Y_Position + y);
    a += 0x08;
    writeData(Enemy_Y_Position + x, a); // save as flame's vertical position
    a = M(PseudoRandomBitReg + x) & 0b00000011; // get 2 LSB from first part of LSFR
    writeData(Enemy_YMF_Dummy + x, a); // save here
    y = a; // use as offset
    a = M(FlameYPosData + y); // get value here using bits as offset
    y = 0x00; // load default offset
    if (a >= M(Enemy_Y_Position + x))
    { // if less, do not increment offset
        y = 0x01; // otherwise increment now
    } // SetMF: get value here and save
    writeData(Enemy_Y_MoveForce + x, M(FlameYMFAdderData + y)); // to vertical movement force
    a = 0x00;
    writeData(EnemyFrenzyBuffer, 0x00); // clear enemy frenzy buffer
    FinishFlame();
    return;
}

//------------------------------------------------------------------------

// get coordinates relative to screen
void SMBEngine::RunPUSubs()
{
    RelativeEnemyPosition();
    GetEnemyOffscreenBits(); // get offscreen bits
    GetEnemyBoundBox(); // get bounding box coordinates
    DrawPowerUp(); // draw the power-up object
    PlayerEnemyCollision(); // check for collision with player
    OffscreenBoundsCheck(); // check to see if it went offscreen

    return; // ExitPUp: and we're done
}


//------------------------------------------------------------------------

void SMBEngine::PowerUpObjHandler()
{
    bool shiftedBit = false;

    x = 0x05; // set object offset for last slot in enemy object buffer
    writeData(ObjectOffset, 0x05);
    a = M(Enemy_State + 5); // check power-up object's state
    if (a == 0)
        return; // if not set, branch to leave
    shiftedBit = (a & 0x80) != 0;
    if (shiftedBit) // shift to check if d7 was set in object state
    { // if not set, branch ahead to skip this part
        // if master timer control set,
        if (M(TimerControl) != 0)
        {
            RunPUSubs(); // branch ahead to enemy object routines
            return;
        }
        a = M(PowerUpType); // check power-up type
        if (a == 0)
            goto ShroomM; // if normal mushroom, branch ahead to move it
        if (a == 0x03)
            goto ShroomM; // if 1-up mushroom, branch ahead to move it
        if (a != 0x02)
        {
            RunPUSubs(); // if not star, branch elsewhere to skip movement
            return;
        }
        MoveJumpingEnemy(); // otherwise impose gravity on star power-up and make it jump
        EnemyJump(); // note that green paratroopa shares the same code here
        RunPUSubs(); // then jump to other power-up subroutines
        return;

ShroomM: // do sub to make mushrooms move
        MoveNormalEnemy();
        EnemyToBGCollisionDet(); // deal with collisions
        RunPUSubs(); // run the other subroutines
        return;
    } // GrowThePowerUp
    // get frame counter
    a = M(FrameCounter) & 0x03; // mask out all but 2 LSB
    if (a != 0)
        goto ChkPUSte; // if any bits set here, branch
    --M(Enemy_Y_Position + 5); // otherwise decrement vertical coordinate slowly
    a = M(Enemy_State + 5); // load power-up object state
    ++M(Enemy_State + 5); // increment state for next frame (to make power-up rise)
    if (a < 0x11)
        goto ChkPUSte; // branch ahead to last part here
    writeData(Enemy_X_Speed + x, 0x10); // otherwise set horizontal speed
    writeData(Enemy_State + 5, 0b10000000); // and then set d7 in power-up object's state
    writeData(Enemy_SprAttrib + 5, 0x00); // initialize background priority bit set here
    a = 0x01; // rotate A to set right moving direction
    writeData(Enemy_MovingDir + x, 0x01); // set moving direction

ChkPUSte: // check power-up object's state
    a = M(Enemy_State + 5);
    if (a < 0x06)
        return; // if not, don't even bother running these routines
    RunPUSubs();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RunNormalEnemies()
{
    a = 0x00; // init sprite attributes
    writeData(Enemy_SprAttrib + x, 0x00);
    GetEnemyOffscreenBits();
    RelativeEnemyPosition();
    EnemyGfxHandler();
    GetEnemyBoundBox();
    EnemyToBGCollisionDet();
    EnemiesCollision();
    PlayerEnemyCollision();
    y = M(TimerControl); // if master timer control set, skip to last routine
    if (y == 0)
    {
        EnemyMovementSubs();
    } // SkipMove
    OffscreenBoundsCheck();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::EnemyMovementSubs()
{
    a = M(Enemy_ID + x);
    switch (a)
    {
    case 0:
        MoveNormalEnemy(); // only objects $00-$14 use this table
        return;
    case 1:
        MoveNormalEnemy();
        return;
    case 2:
        MoveNormalEnemy();
        return;
    case 3:
        MoveNormalEnemy();
        return;
    case 4:
        MoveNormalEnemy();
        return;
    case 5:
        ProcHammerBro();
        return;
    case 6:
        MoveNormalEnemy();
        return;
    case 7:
        MoveBloober();
        return;
    case 8:
        MoveBulletBill();
        return;
    case 9:
        return;
    case 10:
        MoveSwimmingCheepCheep();
        return;
    case 11:
        MoveSwimmingCheepCheep();
        return;
    case 12:
        MovePodoboo();
        return;
    case 13:
        MovePiranhaPlant();
        return;
    case 14:
        MoveJumpingEnemy();
        return;
    case 15:
        ProcMoveRedPTroopa();
        return;
    case 16:
        MoveFlyGreenPTroopa();
        return;
    case 17:
        MoveLakitu();
        return;
    case 18:
        MoveNormalEnemy();
        return;
    case 19:
        return; // dummy
    case 20:
        MoveFlyingCheepCheep();
        return;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

void SMBEngine::ProcHammerBro()
{
    bool hammerSpawned = false;

    // check hammer bro's enemy state for d5 set
    a = M(Enemy_State + x) & 0b00100000;
    if (a != 0)
    { // if not set, go ahead with code
        MoveD_EnemyVertically(); // otherwise move the defeated hammer bro downwards
        MoveEnemyHorizontally(); // and then horizontally
        return;
    } // ChkJH: check jump timer
    if (M(HammerBroJumpTimer + x) != 0)
    { // if expired, branch to jump
        --M(HammerBroJumpTimer + x); // otherwise decrement jump timer
        a = M(Enemy_OffscreenBits) & 0b00001100; // check offscreen bits
        if (a != 0)
        {
            MoveHammerBroXDir(); // if hammer bro a little offscreen, skip to movement code
            return;
        }
        // check hammer throwing timer
        if (M(HammerThrowingTimer + x) != 0)
            goto DecHT; // if not expired, skip ahead, do not throw hammer
        y = M(SecondaryHardMode); // otherwise get secondary hard mode flag
        // get timer data using flag as offset
        writeData(HammerThrowingTimer + x, M(HammerThrowTmrData + y)); // set as new timer
        hammerSpawned = SpawnHammerObj(); // do a sub here to spawn hammer object
        if (!hammerSpawned)
            goto DecHT; // hammer not spawned, skip to decrement timer
        a = M(Enemy_State + x) | 0b00001000; // set d3 in enemy state for hammer throw
        writeData(Enemy_State + x, a);
        MoveHammerBroXDir(); // jump to move hammer bro
        return;

DecHT: // decrement timer
        --M(HammerThrowingTimer + x);
        MoveHammerBroXDir(); // jump to move hammer bro
        return;
    } // HammerBroJumpCode
    // get hammer bro's enemy state
    a = M(Enemy_State + x) & 0b00000111; // mask out all but 3 LSB
    if (a == 0x01)
    {
        MoveHammerBroXDir(); // if set, branch ahead to moving code
        return;
    }
    // load default value here
    writeData(0x00, 0x00); // save into temp variable for now
    y = 0xfa; // set default vertical speed
    a = M(Enemy_Y_Position + x); // check hammer bro's vertical coordinate
    if ((a & 0x80) != 0)
    {
        SetHJ(); // if on the bottom half of the screen, use current speed
        return;
    }
    y = 0xfd; // otherwise set alternate vertical speed
    ++M(0x00); // increment preset value to $01
    if (a < 0x70)
    {
        SetHJ(); // if above the middle of the screen, use current speed and $01
        return;
    }
    --M(0x00); // otherwise return value to $00
    // get part of LSFR, mask out all but LSB
    a = M(PseudoRandomBitReg + 1 + x) & 0x01;
    if (a != 0)
    {
        SetHJ(); // if d0 of LSFR set, branch and use current speed and $00
        return;
    }
    y = 0xfa; // otherwise reset to default vertical speed
    SetHJ();
    return;
}

//------------------------------------------------------------------------

// set vertical speed for jumping
void SMBEngine::SetHJ()
{
    writeData(Enemy_Y_Speed + x, y);
    // set d0 in enemy state for jumping
    a = M(Enemy_State + x) | 0x01;
    writeData(Enemy_State + x, a);
    // load preset value here to use as bitmask
    a = M(0x00) & M(PseudoRandomBitReg + 2 + x); // and do bit-wise comparison with part of LSFR
    y = a; // then use as offset
    a = M(SecondaryHardMode); // check secondary hard mode flag
    if (a == 0)
    {
        y = a; // if secondary hard mode flag clear, set offset to 0
    } // HJump: get jump length timer data using offset from before
    writeData(EnemyFrameTimer + x, M(HammerBroJumpLData + y)); // save in enemy timer
    a = M(PseudoRandomBitReg + 1 + x) | 0b11000000; // get contents of part of LSFR, set d7 and d6, then
    writeData(HammerBroJumpTimer + x, a); // store in jump timer
    MoveHammerBroXDir();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveHammerBroXDir()
{
    bool enemyRightOfPlayer = false;

    y = 0xfc; // move hammer bro a little to the left
    a = M(FrameCounter) & 0b01000000; // change hammer bro's direction every 64 frames
    if (a == 0)
    {
        y = 0x04; // if d6 set in counter, move him a little to the right
    } // Shimmy: store horizontal speed
    writeData(Enemy_X_Speed + x, y);
    y = 0x01; // set to face right by default
    enemyRightOfPlayer = PlayerEnemyDiff(); // get horizontal difference between player and hammer bro
    if ((a & 0x80) != 0)
    {
        SetShim(); // if enemy to the left of player, skip this part
        return;
    }
    ++y; // set to face left
    // check walking timer
    if (M(EnemyIntervalTimer + x) != 0)
    {
        SetShim(); // if not yet expired, skip to set moving direction
        return;
    }
    a = 0xf8;
    writeData(Enemy_X_Speed + x, 0xf8); // otherwise, make the hammer bro walk left towards player
    SetShim();
    return;
}

//------------------------------------------------------------------------

// set moving direction
void SMBEngine::SetShim()
{
    writeData(Enemy_MovingDir + x, y);
    MoveNormalEnemy();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveNormalEnemy()
{
    y = 0x00; // init Y to leave horizontal movement as-is
    a = M(Enemy_State + x) & 0b01000000; // check enemy state for d6 set, if set skip
    if (a != 0)
    {
        FallE(); // to move enemy vertically, then horizontally if necessary
        return;
    }
    a = M(Enemy_State + x);
    a <<= 1; // check enemy state for d7 set
    if ((M(Enemy_State + x) & 0x80) != 0)
    {
        SteadM(); // if set, branch to move enemy horizontally
        return;
    }
    a = M(Enemy_State + x) & 0b00100000; // check enemy state for d5 set
    if (a != 0)
        goto MoveDefeatedEnemy; // if set, branch to move defeated enemy object
    a = M(Enemy_State + x) & 0b00000111; // check d2-d0 of enemy state for any set bits
    if (a == 0)
    {
        SteadM(); // if enemy in normal state, branch to move enemy horizontally
        return;
    }
    if (a == 0x05)
    {
        FallE(); // if enemy in state used by spiny's egg, go ahead here
        return;
    }
    if (a >= 0x03)
        goto ReviveStunned;
    FallE();
    return;

    //------------------------------------------------------------------------
ReviveStunned:
    a = M(EnemyIntervalTimer + x); // if enemy timer not expired yet,
    if (a != 0)
        goto ChkKillGoomba;

    writeData(Enemy_State + x, a); // otherwise initialize enemy state to normal
    a = M(FrameCounter) & 0x01; // get d0 of frame counter
    y = a; // use as Y and increment for movement direction
    ++y;
    writeData(Enemy_MovingDir + x, y); // store as pseudorandom movement direction
    --y; // decrement for use as pointer
    // check primary hard mode flag
    if (M(PrimaryHardMode) != 0)
    { // if not set, use pointer as-is
        ++y;
        ++y; // otherwise increment 2 bytes to next data
    } // SetRSpd: load and store new horizontal speed
    a = M(RevivedXSpeed + y);
    writeData(Enemy_X_Speed + x, a); // and leave
    return;

    //------------------------------------------------------------------------

MoveDefeatedEnemy:
    MoveD_EnemyVertically(); // execute sub to move defeated enemy downwards
    MoveEnemyHorizontally(); // now move defeated enemy horizontally
    return;
ChkKillGoomba:
    if (a != 0x0e)
        return; // a certain point, and branch to leave if not
    a = M(Enemy_ID + x);
    if (a != Goomba)
        return; // branch if not found
    EraseEnemyObject(); // otherwise, kill this goomba object

    return; // NKGmba: leave!
}

//------------------------------------------------------------------------

void SMBEngine::EnemyToBGCollisionDet()
{
    bool shiftedBit = false;
    bool enemyRightOfPlayer = false;
    bool enemyYPosInRange = false;

    // check enemy state for d6 set
    a = M(Enemy_State + x) & 0b00100000;
    if (a != 0)
        return; // if set, branch to leave
    enemyYPosInRange = SubtEnemyYPos(); // otherwise, do a subroutine here
    if (!enemyYPosInRange)
        return; // if enemy vertical coord + 62 < 68, branch to leave
    y = M(Enemy_ID + x);
    if (y == Spiny)
    {
        a = M(Enemy_Y_Position + x);
        if (a < 0x25)
            return;
    } // DoIDCheckBGColl
    if (y == GreenParatroopaJump)
    { // branch if not found
        EnemyJump(); // otherwise jump elsewhere
        return;
    } // HBChk: check for hammer bro
    if (y == HammerBro)
    { // branch if not found
        goto HammerBroBGColl;
    }

    // CInvu: if enemy object is spiny, branch

    if (y == Spiny)
        goto YesIn;
    if (y == PowerUpObject)
        goto YesIn;
    if (y >= 0x07)
    {
        return;
    }

YesIn: // if enemy object < $07, or = $12 or $2e, do this sub
    ChkUnderEnemy();
    if (a == 0)
    { // if block underneath enemy, branch
        goto ChkForRedKoopa; // otherwise skip and do something else
    } // HandleEToBGCollision
    ChkForNonSolids(); // if something is underneath enemy, find out what
    if (a == 0x26 || a == 0xc2 || a == 0xc3
        || a == 0x5f || a == 0x60)
        goto ChkForRedKoopa; // if blank $26, coins, or hidden blocks, jump, enemy falls through
    if (a != 0x23)
        goto LandEnemyProperly; // check for blank metatile $23 and branch if not found
    y = M(0x02); // get vertical coordinate used to find block
    // store default blank metatile in that spot so we won't
    writeData(W(0x06) + y, 0x00); // trigger this routine accidentally again
    a = M(Enemy_ID + x);
    if (a >= 0x15)
    {
        ChkToStunEnemies();
        return;
    }
    if (a == Goomba)
    {
        KillEnemyAboveBlock(); // if enemy object IS goomba, do this sub
    } // GiveOEPoints
    a = 0x01; // award 100 points for hitting block beneath enemy
    SetupFloateyNumber();
    ChkToStunEnemies();
    return;

//------------------------------------------------------------------------

LandEnemyProperly:
    a = M(0x04); // check lower nybble of vertical coordinate saved earlier
    a -= 0x08; // subtract eight pixels
    if (a >= 0x05)
        goto ChkForRedKoopa; // branch if lower nybble in range of $0d-$0f before subtract
    a = M(Enemy_State + x) & 0b01000000; // branch if d6 in enemy state is set
    if (a != 0)
        goto LandEnemyInitState;
    a = M(Enemy_State + x);
    a <<= 1; // branch if d7 in enemy state is not set
    if ((M(Enemy_State + x) & 0x80) != 0)
    {

SChkA: // if lower nybble < $0d, d7 set but d6 not set, jump here
        DoEnemySideCheck();
        return;
    } // ChkLandedEnemyState
    a = M(Enemy_State + x); // if enemy in normal state, branch back to jump here
    if (a == 0)
        goto SChkA;
    if (a == 0x05)
        goto ProcEnemyDirection; // then branch elsewhere
    if (a < 0x03)
    { // or in higher numbered state, branch to leave
        // load enemy state again (why?)
        if (M(Enemy_State + x) != 0x02)
            goto ProcEnemyDirection; // then branch elsewhere
        a = 0x10; // load default timer here
        y = M(Enemy_ID + x); // check enemy identifier for spiny
        if (y == Spiny)
        { // branch if not found
            a = 0x00; // set timer for $00 if spiny
        } // SetForStn: set timer here
        writeData(EnemyIntervalTimer + x, a);
        a = 0x03; // set state here, apparently used to render
        writeData(Enemy_State + x, 0x03); // upside-down koopas and buzzy beetles
        EnemyLanding(); // then land it properly
    } // ExSteChk: then leave
    return;

//------------------------------------------------------------------------

ProcEnemyDirection:
    a = M(Enemy_ID + x); // check enemy identifier for goomba
    if (a == Goomba)
        goto LandEnemyInitState;
    if (a == Spiny)
    { // branch if not found
        writeData(Enemy_MovingDir + x, 0x01); // send enemy moving to the right by default
        writeData(Enemy_X_Speed + x, 0x08); // set horizontal speed accordingly
        a = M(FrameCounter) & 0b00000111; // if timed appropriately, spiny will skip over
        if (a == 0)
            goto LandEnemyInitState; // trying to face the player
    } // InvtD: load 1 for enemy to face the left (inverted here)
    y = 0x01;
    enemyRightOfPlayer = PlayerEnemyDiff(); // get horizontal difference between player and enemy
    if ((a & 0x80) != 0)
    { // if enemy to the right of player, branch
        ++y; // if to the left, increment by one for enemy to face right (inverted)
    } // CNwCDir
    a = y;
    if (a != M(Enemy_MovingDir + x))
        goto LandEnemyInitState;
    ChkForBump_HammerBroJ(); // if equal, not facing in correct dir, do sub to turn around

LandEnemyInitState:
    EnemyLanding(); // land enemy properly
    a = M(Enemy_State + x) & 0b10000000; // if d7 of enemy state is set, branch
    if (a == 0)
    {
        a = 0x00; // otherwise initialize enemy state and leave
        writeData(Enemy_State + x, 0x00); // note this will also turn spiny's egg into spiny
        return;

    //------------------------------------------------------------------------
    } // NMovShellFallBit
    // nullify d6 of enemy state, save other bits
    a = M(Enemy_State + x) & 0b10111111; // and store, then leave
    writeData(Enemy_State + x, a);
    return;

//------------------------------------------------------------------------

ChkForRedKoopa:
    // check for red koopa troopa $03
    if (M(Enemy_ID + x) == RedKoopa)
    { // branch if not found
        if (M(Enemy_State + x) == 0)
        {
            ChkForBump_HammerBroJ(); // if enemy found and in normal state, branch
            return;
        }
    } // Chk2MSBSt: save enemy state into Y
    a = M(Enemy_State + x);
    y = a;
    shiftedBit = (a & 0x80) != 0;
    if (shiftedBit) // check for d7 set
    { // branch if not set
        a = M(Enemy_State + x) | 0b01000000; // set d6
    } // GetSteFromD: load new enemy state with old as offset
    else // jump ahead of this part
    {
        a = M(EnemyBGCStateData + y);
    } // SetD6Ste: set as new state
    writeData(Enemy_State + x, a);
    DoEnemySideCheck(); // then check for horizontal blockage and leave
    return;

HammerBroBGColl:
    ChkUnderEnemy(); // check to see if hammer bro is standing on anything
    if (a == 0)
        goto NoUnderHammerBro;
    if (a == 0x23)
    {
        KillEnemyAboveBlock();
        return;
    }
    // check timer used by hammer bro
    if (M(EnemyFrameTimer + x) != 0)
        goto NoUnderHammerBro; // branch if not expired
    a = M(Enemy_State + x) & 0b10001000; // save d7 and d3 from enemy state, nullify other bits
    writeData(Enemy_State + x, a); // and store
    EnemyLanding(); // modify vertical coordinate, speed and something else
    DoEnemySideCheck(); // then check for horizontal blockage and leave
    return;

NoUnderHammerBro:
    // if hammer bro is not standing on anything, set d0
    a = M(Enemy_State + x) | 0x01; // in the enemy state to indicate jumping or falling, then leave
    writeData(Enemy_State + x, a);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DoEnemySideCheck()
{
    a = M(Enemy_Y_Position + x); // if enemy within status bar, branch to leave
    if (a >= 0x20)
    {
        y = 0x16; // start by finding block to the left of enemy ($00,$14)
        a = 0x02; // set value here in what is also used as
        writeData(0xeb, 0x02); // OAM data offset

        do // SdeCLoop: check value
        {
            a = M(0xeb);
            if (a != M(Enemy_MovingDir + x))
                goto NextSdeC; // branch if different and do not seek block there
            a = 0x01; // set flag in A for save horizontal coordinate
            BlockBufferChk_Enemy(); // find block to left or right of enemy object
            if (a == 0)
                goto NextSdeC; // if nothing found, branch
            ChkForNonSolids(); // check for non-solid blocks
            if (a != 0x26 && a != 0xc2 && a != 0xc3
                && a != 0x5f && a != 0x60)
            {
                ChkForBump_HammerBroJ(); // branch if not found
                return;
            }

NextSdeC: // move to the next direction
            --M(0xeb);
            ++y;
        } while (y < 0x18); // enemy ($00, $14) and ($10, $14) pixel coordinates
    } // ExESdeC
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ChkForBump_HammerBroJ()
{
    if (x == 0x05)
    {
        NoBump(); // and if so, branch ahead and do not play sound
        return;
    }
    a = M(Enemy_State + x); // if enemy state d7 not set, branch
    a <<= 1; // ahead and do not play sound
    if ((M(Enemy_State + x) & 0x80) == 0)
    {
        NoBump();
        return;
    }
    a = Sfx_Bump; // otherwise, play bump sound
    writeData(Square1SoundQueue, Sfx_Bump); // sound will never be played if branching from ChkForRedKoopa
    NoBump();
    return;
}

//------------------------------------------------------------------------

// check for hammer bro
void SMBEngine::NoBump()
{
    if (M(Enemy_ID + x) == 0x05)
    { // branch if not found
        a = 0x00;
        writeData(0x00, 0x00); // initialize value here for bitmask
        y = 0xfa; // load default vertical speed for jumping
        SetHJ(); // jump to code that makes hammer bro jump
        return;
    } // InvEnemyDir
    RXSpd(); // jump to turn the enemy around
    return;
}

//------------------------------------------------------------------------

void SMBEngine::EnemyJump()
{
    bool enemyYPosInRange = false;

    enemyYPosInRange = SubtEnemyYPos(); // do a sub here
    if (!enemyYPosInRange)
        goto DoSide; // if enemy vertical coord + 62 < 68, branch to leave
    a = M(Enemy_Y_Speed + x);
    a += 0x02;
    if (a < 0x03)
        goto DoSide;
    ChkUnderEnemy(); // otherwise, check to see if green paratroopa is
    if (a == 0)
        goto DoSide; // standing on anything, then branch to same place if not
    ChkForNonSolids(); // check for non-solid blocks
    if (a == 0x26 || a == 0xc2 || a == 0xc3
        || a == 0x5f || a == 0x60)
        goto DoSide; // branch if found
    EnemyLanding(); // change vertical coordinate and speed
    a = 0xfd;
    writeData(Enemy_Y_Speed + x, 0xfd); // make the paratroopa jump again

DoSide: // check for horizontal blockage, then leave
    DoEnemySideCheck();
    return;
}



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

//------------------------------------------------------------------------

void SMBEngine::GameCoreRoutine()
{
    x = M(CurrentPlayer); // get which player is on the screen
    // use appropriate player's controller bits
    writeData(SavedJoypadBits, M(SavedJoypadBits + x)); // as the master controller bits
    GameRoutines(); // execute one of many possible subs
    a = M(OperMode_Task); // check major task of operating mode
    if (a < 0x03)
    { // branch to the game engine itself
        return;

    //------------------------------------------------------------------------
    } // GameEngine
    ProcFireball_Bubble(); // process fireballs and air bubbles
    x = 0x00;

    do // ProcELoop: put incremented offset in X as enemy object offset
    {
        writeData(ObjectOffset, x);
        EnemiesAndLoopsCore(); // process enemy objects
        FloateyNumbersRoutine(); // process floatey numbers
        ++x;
    } while (x != 0x06);
    GetPlayerOffscreenBits(); // get offscreen bits for player object
    RelativePlayerPosition(); // get relative coordinates for player object
    PlayerGfxHandler(); // draw the player
    BlockObjMT_Updater(); // replace block objects with metatiles if necessary
    x = 0x01;
    writeData(ObjectOffset, 0x01); // set offset for second
    BlockObjectsCore(); // process second block object
    --x;
    writeData(ObjectOffset, x); // set offset for first
    BlockObjectsCore(); // process first block object
    MiscObjectsCore(); // process misc objects (hammer, jumping coins)
    ProcessCannons(); // process bullet bill cannons
    ProcessWhirlpools(); // process whirlpools
    FlagpoleRoutine(); // process the flagpole
    RunGameTimer(); // count down the game timer
    ColorRotation(); // cycle one of the background colors
    if (((M(Player_Y_HighPos) - 0x02) & 0x80) == 0)
        goto NoChgMus;
    a = M(StarInvincibleTimer); // if star mario invincibility timer at zero,
    if (a != 0)
    { // skip this part
        if (a != 0x04)
            goto NoChgMus; // if not yet at a certain point, continue
        // if interval timer not yet expired,
        if (M(IntervalTimerControl) != 0)
            goto NoChgMus; // branch ahead, don't bother with the music
        GetAreaMusic(); // to re-attain appropriate level music

NoChgMus: // get invincibility timer
        y = M(StarInvincibleTimer);
        a = M(FrameCounter); // get frame counter
        if (y < 0x08)
        { // branch to cycle player's palette quickly
            a >>= 1; // otherwise, divide by 8 to cycle every eighth frame
            a >>= 1;
        } // CycleTwo: if branched here, divide by 2 to cycle every other frame
        a >>= 1;
        CyclePlayerPalette(); // do sub to cycle the palette (note: shares fire flower code)
    } // ClrPlrPal: do sub to clear player's palette bits in attributes
    else // then skip this sub to finish up the game engine
    {
        ResetPalStar();
    } // SaveAB: save current A and B button
    writeData(PreviousA_B_Buttons, M(A_B_Buttons)); // into temp variable to be used on next frame
    a = 0x00;
    writeData(Left_Right_Buttons, 0x00); // nullify left and right buttons temp variable
    UpdScrollVar();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::EnemiesAndLoopsCore()
{
    bool shiftedBit = false;

    a = M(Enemy_Flag + x); // check data here for MSB set
    pha(); // save in stack
    shiftedBit = (a & 0x80) != 0;
    if (!shiftedBit)
    { // if MSB set in enemy flag, branch ahead of jumps
        pla(); // get from stack
        if (a != 0)
        { // if data zero, branch
            goto RunEnemyObjectsCore; // otherwise, jump to run enemy subroutines
        } // ChkAreaTsk: check number of tasks to perform
        a = M(AreaParserTaskNum) & 0x07;
        if (a == 0x07)
            return;
        ProcLoopCommand(); // otherwise, jump to process loop command/load enemies
        return;
    } // ChkBowserF: get data from stack
    pla();
    a &= 0b00001111; // mask out high nybble
    y = a;
    a = M(Enemy_Flag + y); // use as pointer and load same place with different offset
    if (a != 0)
        return;
    writeData(Enemy_Flag + x, a); // if second enemy flag not set, also clear first one

    return; // ExitELCore

//------------------------------------------------------------------------

RunEnemyObjectsCore:
    x = M(ObjectOffset); // get offset for enemy object buffer
    a = 0x00; // load value 0 for jump engine by default
    y = M(Enemy_ID + x);
    if (y >= 0x15)
    {
        a = y; // otherwise subtract $14 from the value and use
        a -= 0x14; // as value for jump engine
    } // JmpEO
    switch (a)
    {
    case 0:
        RunNormalEnemies(); // for objects $00-$14
        return;
    case 1:
        RunBowserFlame(); // for objects $15-$1f
        return;
    case 2:
        RunFireworks();
        return;
    case 3:
        return;
    case 4:
        return;
    case 5:
        return;
    case 6:
        return;
    case 7:
        RunFirebarObj();
        return;
    case 8:
        RunFirebarObj();
        return;
    case 9:
        RunFirebarObj();
        return;
    case 10:
        RunFirebarObj();
        return;
    case 11:
        RunFirebarObj();
        return;
    case 12:
        RunFirebarObj(); // for objects $20-$2f
        return;
    case 13:
        RunFirebarObj();
        return;
    case 14:
        RunFirebarObj();
        return;
    case 15:
        return;
    case 16:
        RunLargePlatform();
        return;
    case 17:
        RunLargePlatform();
        return;
    case 18:
        RunLargePlatform();
        return;
    case 19:
        RunLargePlatform();
        return;
    case 20:
        RunLargePlatform();
        return;
    case 21:
        RunLargePlatform();
        return;
    case 22:
        RunLargePlatform();
        return;
    case 23:
        RunSmallPlatform();
        return;
    case 24:
        RunSmallPlatform();
        return;
    case 25:
        RunBowser();
        return;
    case 26:
        PowerUpObjHandler();
        return;
    case 27:
        VineObjectHandler();
        return;
    case 28:
        return; // for objects $30-$35
    case 29:
        RunStarFlagObj();
        return;
    case 30:
        JumpspringHandler();
        return;
    case 31:
        return;
    case 32:
        WarpZoneObject();
        return;
    case 33:
        RunRetainerObj();
        return;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

void SMBEngine::ProcLoopCommand()
{
    uint32_t wide = 0;

ProcLoopCommand:
    // check if loop command was found
    if (M(LoopCommand) == 0)
        goto ChkEnemyFrenzy;
    // check to see if we're still on the first page
    if (M(CurrentColumnPos) != 0)
        goto ChkEnemyFrenzy; // if not, do not loop yet
    y = 0x0b; // start at the end of each set of loop data

FindLoop:
    --y;
    if ((y & 0x80) != 0)
        goto ChkEnemyFrenzy; // if all data is checked and not match, do not loop
    // check to see if one of the world numbers
    if (M(WorldNumber) != M(LoopCmdWorldNumber + y))
        goto FindLoop;
    // check to see if one of the page numbers
    if (M(CurrentPageLoc) != M(LoopCmdPageNumber + y))
        goto FindLoop;
    // check to see if the player is at the correct position
    if (M(Player_Y_Position) != M(LoopCmdYPosition + y))
        goto WrongChk;
    // check to see if the player is
    if (M(Player_State) != 0x00)
        goto WrongChk; // if not, player fails to pass loop, and loopback
    // are we in world 7? (check performed on correct
    if (M(WorldNumber) != World7)
        goto InitMLp; // if not, initialize flags used there, otherwise
    ++M(MultiLoopCorrectCntr); // increment counter for correct progression

IncMLoop: // increment master multi-part counter
    ++M(MultiLoopPassCntr);
    // have we done all three parts?
    if (M(MultiLoopPassCntr) == 0x03)
    { // if not, skip this part
        a = M(MultiLoopCorrectCntr); // if so, have we done them all correctly?
        if (a == 0x03)
            goto InitMLp; // if so, branch past unnecessary check here
        if (a == 0x03)
        { // unconditional branch if previous branch fails

WrongChk: // are we in world 7? (check performed on
            if (M(WorldNumber) == World7)
                goto IncMLoop;
        } // DoLpBack: if player is not in right place, loop back
        ExecGameLoopback();
        KillAllEnemies();

InitMLp: // initialize counters used for multi-part loop commands
        a = 0x00;
        writeData(MultiLoopPassCntr, 0x00);
        writeData(MultiLoopCorrectCntr, 0x00);
    } // InitLCmd: initialize loop command flag
    a = 0x00;
    writeData(LoopCommand, 0x00);

ChkEnemyFrenzy:
    a = M(EnemyFrenzyQueue); // check for enemy object in frenzy queue
    if (a != 0)
    { // if not, skip this part
        writeData(Enemy_ID + x, a); // store as enemy object identifier here
        writeData(Enemy_Flag + x, 0x01); // activate enemy object flag
        a = 0x00;
        writeData(Enemy_State + x, 0x00); // initialize state and frenzy queue
        writeData(EnemyFrenzyQueue, 0x00);
        InitEnemyObject(); // and then jump to deal with this enemy
        return;
    } // ProcessEnemyData
    y = M(EnemyDataOffset); // get offset of enemy object data
    a = M(W(EnemyData) + y); // load first byte
    if (a == 0xff)
    {
        goto CheckFrenzyBuffer; // if found, jump to check frenzy buffer, otherwise
    } // CheckEndofBuffer
    a &= 0b00001111; // check for special row $0e
    if (a == 0x0e)
        goto CheckRightBounds; // if found, branch, otherwise
    if (x < 0x05)
        goto CheckRightBounds; // if not at end of buffer, branch
    ++y;
    // check for specific value here
    a = M(W(EnemyData) + y) & 0b00111111; // not sure what this was intended for, exactly
    if (a == 0x2e)
        goto CheckRightBounds; // but it has the effect of keeping enemies out of
    return; // the sixth slot

//------------------------------------------------------------------------

CheckRightBounds:
    wide = ((M(ScreenRight_PageLoc) << 8) | M(ScreenRight_X_Pos)) + 0x30; // add 48 to pixel coordinate of right boundary
    a = LOBYTE(wide) & 0b11110000; // store high nybble
    writeData(0x07, a);
    a = HIBYTE(wide); // add carry to page location of right boundary
    writeData(0x06, a); // store page location + carry
    y = M(EnemyDataOffset);
    ++y;
    a = M(W(EnemyData) + y); // if MSB of enemy object is clear, branch to check for row $0f
    a <<= 1;
    if ((M(W(EnemyData) + y) & 0x80) == 0)
        goto CheckPageCtrlRow;
    // if page select already set, do not set again
    if (M(EnemyObjectPageSel) != 0)
        goto CheckPageCtrlRow;
    ++M(EnemyObjectPageSel); // otherwise, if MSB is set, set page select
    ++M(EnemyObjectPageLoc); // and increment page control

CheckPageCtrlRow:
    --y;
    // reread first byte
    a = M(W(EnemyData) + y) & 0x0f;
    if (a != 0x0f)
        goto PositionEnemyObj; // if not found, branch to position enemy object
    // if page select set,
    if (M(EnemyObjectPageSel) != 0)
        goto PositionEnemyObj; // branch without reading second byte
    ++y;
    // otherwise, get second byte, mask out 2 MSB
    a = M(W(EnemyData) + y) & 0b00111111;
    writeData(EnemyObjectPageLoc, a); // store as page control for enemy object data
    ++M(EnemyDataOffset); // increment enemy object data offset 2 bytes
    ++M(EnemyDataOffset);
    ++M(EnemyObjectPageSel); // set page select for enemy object data and
    goto ProcLoopCommand; // jump back to process loop commands again

PositionEnemyObj:
    // store page control as page location
    writeData(Enemy_PageLoc + x, M(EnemyObjectPageLoc)); // for enemy object
    // get first byte of enemy object
    a = M(W(EnemyData) + y) & 0b11110000;
    writeData(Enemy_X_Position + x, a); // store column position
    if (((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x)) // check column position against right boundary
        < ((M(ScreenRight_PageLoc) << 8) | M(ScreenRight_X_Pos)))
    { // if enemy object beyond or at boundary, branch
        a = M(W(EnemyData) + y) & 0b00001111; // check for special row $0e
        if (a == 0x0e)
            goto ParseRow0e;
    } // CheckRightExtBounds
    else // if not found, unconditional jump
    {
        if (((M(0x06) << 8) | M(0x07)) // check right boundary + 48 against the column position
            < ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x)))
            goto CheckFrenzyBuffer; // if enemy object beyond extended boundary, branch
        // store value in vertical high byte
        writeData(Enemy_Y_HighPos + x, 0x01);
        a = M(W(EnemyData) + y); // get first byte again
        a <<= 1; // multiply by four to get the vertical
        a <<= 1; // coordinate
        a <<= 1;
        a <<= 1;
        writeData(Enemy_Y_Position + x, a);
        if (a == 0xe0)
            goto ParseRow0e; // (necessary if branched to $c1cb)
        ++y;
        // get second byte of object
        a = M(W(EnemyData) + y) & 0b01000000; // check to see if hard mode bit is set
        if (a != 0)
        { // if not, branch to check for group enemy objects
            // if set, check to see if secondary hard mode flag
            if (M(SecondaryHardMode) == 0)
            {
                Inc2B(); // is on, and if not, branch to skip this object completely
                return;
            }
        } // CheckForEnemyGroup
        // get second byte and mask out 2 MSB
        a = M(W(EnemyData) + y) & 0b00111111;
        if (a >= 0x37)
        {
            if (a < 0x3f)
                goto DoGroup; // below $3f, branch if below $3f
        } // BuzzyBeetleMutate
        if (a != Goomba)
            goto StrID; // value ($3f or more always fails)
        // check if primary hard mode flag is set
        if (M(PrimaryHardMode) == 0)
            goto StrID; // and if so, change goomba to buzzy beetle
        a = BuzzyBeetle;

StrID: // store enemy object number into buffer
        writeData(Enemy_ID + x, a);
        a = 0x01;
        writeData(Enemy_Flag + x, 0x01); // set flag for enemy in buffer
        InitEnemyObject();
        a = M(Enemy_Flag + x); // check to see if flag is set
        if (a != 0)
        {
            Inc2B(); // if not, leave, otherwise branch
            return;
        }
        return;

    //------------------------------------------------------------------------

CheckFrenzyBuffer:
        a = M(EnemyFrenzyBuffer); // if enemy object stored in frenzy buffer
        if (a == 0)
        { // then branch ahead to store in enemy object buffer
            a = M(VineFlagOffset); // otherwise check vine flag offset
            if (a != 0x01)
                return; // if other value <> 1, leave
            a = VineObject; // otherwise put vine in enemy identifier
        } // StrFre: store contents of frenzy buffer into enemy identifier value
        writeData(Enemy_ID + x, a);
        InitEnemyObject(); // then go and initialize it
        return;

    //------------------------------------------------------------------------

DoGroup:
        HandleGroupEnemies(); // handle enemy group objects
        return;

ParseRow0e:
        ++y; // increment Y to load third byte of object
        ++y;
        a = M(W(EnemyData) + y) >> 5; // move 3 MSB to the bottom, effectively making %xxx00000 into %00000xxx
        if (a == M(WorldNumber))
        { // if not, do not use (this allows multiple uses
            --y; // of the same area, like the underground bonus areas)
            // otherwise, get second byte and use as offset
            writeData(AreaPointer, M(W(EnemyData) + y)); // to addresses for level and enemy object data
            ++y;
            // get third byte again, and this time mask out
            a = M(W(EnemyData) + y) & 0b00011111; // the 3 MSB from before, save as page number to be
            writeData(EntrancePage, a); // used upon entry to area, if area is entered
        } // NotUse
        Inc3B();
        return;
    } // CheckThreeBytes
    y = M(EnemyDataOffset); // load current offset for enemy object data
    // get first byte
    a = M(W(EnemyData) + y) & 0b00001111; // check for special row $0e
    if (a != 0x0e)
    {
        Inc2B();
        return;
    }
    Inc3B();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitEnemyObject()
{
    a = 0x00; // initialize enemy state
    writeData(Enemy_State + x, 0x00);
    CheckpointEnemyID(); // jump ahead to run jump engine and subroutines

    return; // ExEPar: then leave
}

//------------------------------------------------------------------------

void SMBEngine::CheckpointEnemyID()
{
CheckpointEnemyID:
    a = M(Enemy_ID + x);
    if (a < 0x15)
    { // and branch straight to the jump engine if found
        y = a; // save identifier in Y register for now
        a = M(Enemy_Y_Position + x);
        a += 0x08; // add eight pixels to what will eventually be the
        writeData(Enemy_Y_Position + x, a); // enemy object's vertical coordinate ($00-$14 only)
        writeData(EnemyOffscrBitsMasked + x, 0x01); // set offscreen masked bit
        a = y; // get identifier back and use as offset for jump engine
    } // InitEnemyRoutines
    y = a * 2 + 2;
    switch (a)
    {
    case 0:
        InitNormalEnemy(); // for objects $00-$0f
        return;
    case 1:
        InitNormalEnemy();
        return;
    case 2:
        InitNormalEnemy();
        return;
    case 3:
        InitRedKoopa();
        return;
    case 4:
        return;
    case 5:
        InitHammerBro();
        return;
    case 6:
        InitGoomba();
        return;
    case 7:
        InitBloober();
        return;
    case 8:
        InitBulletBill();
        return;
    case 9:
        return;
    case 10:
        InitCheepCheep();
        return;
    case 11:
        InitCheepCheep();
        return;
    case 12:
        InitPodoboo();
        return;
    case 13:
        InitPiranhaPlant();
        return;
    case 14:
        InitJumpGPTroopa();
        return;
    case 15:
        InitRedPTroopa();
        return;
    case 16:
        InitHorizFlySwimEnemy(); // for objects $10-$1f
        return;
    case 17:
        InitLakitu();
        return;
    case 18:
        goto InitEnemyFrenzy;
    case 19:
        return;
    case 20:
        goto InitEnemyFrenzy;
    case 21:
        goto InitEnemyFrenzy;
    case 22:
        goto InitEnemyFrenzy;
    case 23:
        goto InitEnemyFrenzy;
    case 24:
        EndFrenzy();
        return;
    case 25:
        return;
    case 26:
        return;
    case 27:
        InitShortFirebar();
        return;
    case 28:
        InitShortFirebar();
        return;
    case 29:
        InitShortFirebar();
        return;
    case 30:
        InitShortFirebar();
        return;
    case 31:
        InitLongFirebar();
        return;
    case 32:
        return; // for objects $20-$2f
    case 33:
        return;
    case 34:
        return;
    case 35:
        return;
    case 36:
        InitBalPlatform();
        return;
    case 37:
        InitVertPlatform();
        return;
    case 38:
        LargeLiftUp();
        return;
    case 39:
        LargeLiftDown();
        return;
    case 40:
        InitHoriPlatform();
        return;
    case 41:
        InitDropPlatform();
        return;
    case 42:
        InitHoriPlatform();
        return;
    case 43:
        PlatLiftUp();
        return;
    case 44:
        PlatLiftDown();
        return;
    case 45:
        InitBowser();
        return;
    case 46:
        PwrUpJmp(); // possibly dummy value
        return;
    case 47:
        Setup_Vine();
        return;
    case 48:
        return; // for objects $30-$36
    case 49:
        return;
    case 50:
        return;
    case 51:
        return;
    case 52:
        return;
    case 53:
        InitRetainerObj();
        return;
    case 54:
        return;
    default:
        bad_jump();
        return;
    }

//------------------------------------------------------------------------

BulletBillCheepCheep:
    a = M(FrenzyEnemyTimer); // if timer not expired yet, branch to leave
    if (a != 0)
        return;
    a = M(AreaType); // are we in a water-type level?
    if (a != 0)
        goto DoBulletBills;

    if (x >= 0x03)
        return; // if so, branch to leave
    y = 0x00; // load default offset
    if (M(PseudoRandomBitReg + x) >= 0xaa)
    { // if less than preset, do not increment offset
        y = 0x01; // otherwise increment
    } // ChkW2: check world number
    if (M(WorldNumber) != World2)
    { // if we're on world 2, do not increment offset
        ++y; // otherwise increment
    } // Get17ID
    a = y;
    a &= 0b00000001; // mask out all but last bit of offset
    y = a;
    a = M(SwimCC_IDData + y); // load identifier for cheep-cheeps

Set17ID: // store whatever's in A as enemy identifier
    writeData(Enemy_ID + x, a);
    if (M(BitMFilter) == 0xff)
    {
        a = 0x00; // initialize vertical position filter
        writeData(BitMFilter, 0x00);
    } // GetRBit: get first part of LSFR
    a = M(PseudoRandomBitReg + x) & 0b00000111; // mask out all but 3 LSB

ChkRBit: // use as offset
    y = a;
    a = M(Bitmasks + y); // load bitmask
    if ((a & M(BitMFilter)) != 0) // perform AND on filter without changing it
    {
        ++y; // increment offset
        a = y;
        a &= 0b00000111; // mask out all but 3 LSB thus keeping it 0-7
        goto ChkRBit; // do another check
    } // AddFBit: add bit to already set bits in filter
    a |= M(BitMFilter);
    writeData(BitMFilter, a); // and store
    a = M(Enemy17YPosData + y); // load vertical position using offset
    PutAtRightExtent(); // set vertical position and other values
    writeData(Enemy_YMF_Dummy + x, a); // initialize dummy variable
    a = 0x20; // set timer
    writeData(FrenzyEnemyTimer, 0x20);
    goto CheckpointEnemyID; // process our new enemy object

DoBulletBills:
    y = 0xff; // start at beginning of enemy slots

BB_SLoop: // move onto the next slot
    ++y;
    if (y < 0x05)
    {
        // if enemy buffer flag not set,
        if (M(Enemy_Flag + y) == 0)
            goto BB_SLoop; // loop back and check another slot
        a = M(Enemy_ID + y);
        if (a != BulletBill_FrenzyVar)
            goto BB_SLoop; // bullet bill object (frenzy variant)

        return; // ExF17: if found, leave

    //------------------------------------------------------------------------
    } // FireBulletBill
    a = M(Square2SoundQueue) | Sfx_Blast; // play fireworks/gunfire sound
    writeData(Square2SoundQueue, a);
    a = BulletBill_FrenzyVar; // load identifier for bullet bill object
    goto Set17ID; // unconditional branch

InitEnemyFrenzy:
    a = M(Enemy_ID + x); // load enemy identifier
    writeData(EnemyFrenzyBuffer, a); // save in enemy frenzy buffer
    a -= 0x12; // subtract 12 and use as offset for jump engine
    switch (a)
    {
    case 0:
        LakituAndSpinyHandler();
        return;
    case 1:
        return;
    case 2:
        InitFlyingCheepCheep();
        return;
    case 3:
        InitBowserFlame();
        return;
    case 4:
        InitFireworks();
        return;
    case 5:
        goto BulletBillCheepCheep;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

void SMBEngine::HandleGroupEnemies()
{
    bool shiftedBit = false;
    uint32_t wide = 0;

    y = 0x00; // load value for green koopa troopa
    a -= 0x37; // subtract $37 from second byte read
    pha(); // save result in stack for now
    if (a < 0x04)
    { // if so, branch
        pha(); // save another copy to stack
        y = Goomba; // load value for goomba enemy
        // if primary hard mode flag not set,
        if (M(PrimaryHardMode) != 0)
        { // branch, otherwise change to value
            y = BuzzyBeetle; // for buzzy beetle
        } // PullID: get second copy from stack
        pla();
    } // SnglID: save enemy id here
    writeData(0x01, y);
    y = 0xb0; // load default y coordinate
    a &= 0x02; // check to see if d1 was set
    if (a != 0)
    { // if so, move y coordinate up,
        y = 0x70; // otherwise branch and use default
    } // SetYGp: save y coordinate here
    writeData(0x00, y);
    // get page number of right edge of screen
    writeData(0x02, M(ScreenRight_PageLoc)); // save here
    // get pixel coordinate of right edge
    writeData(0x03, M(ScreenRight_X_Pos)); // save here
    y = 0x02; // load two enemies by default
    pla(); // get first copy from stack
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // check to see if d0 was set
    if (shiftedBit)
    { // if not, use default value
        y = 0x03; // otherwise increment to three enemies
    } // CntGrp: save number of enemies here
    writeData(NumberofGroupEnemies, y);

GrLoop: // start at beginning of enemy buffers
    x = 0xff;

GSltLp: // increment and branch if past
    ++x;
    if (x < 0x05)
    {
        // check to see if enemy is already
        if (M(Enemy_Flag + x) != 0)
            goto GSltLp; // stored in buffer, and branch if so
        writeData(Enemy_ID + x, M(0x01)); // store enemy object identifier
        writeData(Enemy_PageLoc + x, M(0x02)); // store page location for enemy object
        a = M(0x03);
        writeData(Enemy_X_Position + x, a); // store x coordinate for enemy object
        wide = ((M(0x02) << 8) | a) + 0x18; // add 24 pixels for next enemy
        writeData(0x03, LOBYTE(wide));
        writeData(0x02, HIBYTE(wide));
        a = HIBYTE(wide); // add carry to page location for
        // store y coordinate for enemy object
        writeData(Enemy_Y_Position + x, M(0x00));
        a = 0x01; // activate flag for buffer, and
        writeData(Enemy_Y_HighPos + x, 0x01); // put enemy within the screen vertically
        writeData(Enemy_Flag + x, 0x01);
        CheckpointEnemyID(); // process each enemy object separately
        --M(NumberofGroupEnemies); // do this until we run out of enemy objects
        if (M(NumberofGroupEnemies) != 0)
            goto GrLoop;
    } // NextED: jump to increment data offset and leave
    Inc2B();
    return;
}
