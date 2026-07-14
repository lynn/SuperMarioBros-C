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
    }


Start:
    /* sei */ // pretty standard 6502 type init here
    /* cld */
    a = 0b00010000; // init PPU control register 1
    writeData(PPU_CTRL_REG1, a);
    x = 0xff; // reset stack pointer
    s = x;

    do // VBlank1: wait two frames
    {
        a = readData(PPU_STATUS);
    } while ((a & 0x80) == 0);

    do // VBlank2
    {
        a = readData(PPU_STATUS);
    } while ((a & 0x80) == 0);
    y = ColdBootOffset; // load default cold boot pointer
    x = 0x05; // this is where we check for a warm boot

    do // WBootCheck: check each score digit in the top score
    {
        a = M(TopScoreDisplay + x);
        if (a >= 10)
            goto ColdBoot; // if not, give up and proceed with cold boot
        --x;
    } while ((x & 0x80) == 0);
    a = M(WarmBootValidation); // second checkpoint, check to see if
    if (a != 0xa5)
        goto ColdBoot;
    y = WarmBootOffset; // if passed both, load warm boot pointer

ColdBoot: // clear memory using pointer in Y
    JSR(InitializeMemory, 0);
    writeData(SND_DELTA_REG + 1, a); // reset delta counter load register
    writeData(OperMode, a); // reset primary mode of operation
    a = 0xa5; // set warm boot flag
    writeData(WarmBootValidation, a);
    writeData(PseudoRandomBitReg, a); // set seed for pseudorandom register
    a = 0b00001111;
    writeData(SND_MASTERCTRL_REG, a); // enable all sound channels except dmc
    a = 0b00000110;
    writeData(PPU_CTRL_REG2, a); // turn off clipping for OAM and background
    JSR(MoveAllSpritesOffscreen, 1);
    JSR(InitializeNameTables, 2); // initialize both name tables
    ++M(DisableScreenFlag); // set flag to disable screen output
    a = M(Mirror_PPU_CTRL_REG1);
    a |= 0b10000000; // enable NMIs
    JSR(WritePPUReg1, 3);

EndlessLoop: // endless loop, need I say more?
    return;

NonMaskableInterrupt:
    a = M(Mirror_PPU_CTRL_REG1); // disable NMIs in mirror reg
    a &= 0b01111111; // save all other bits
    writeData(Mirror_PPU_CTRL_REG1, a);
    a &= 0b01111110; // alter name table address to be $2800
    writeData(PPU_CTRL_REG1, a); // (essentially $2000) but save other bits
    a = M(Mirror_PPU_CTRL_REG2); // disable OAM and background display by default
    a &= 0b11100110;
    y = M(DisableScreenFlag); // get screen disable flag
    if (y == 0)
    { // if set, used bits as-is
        a = M(Mirror_PPU_CTRL_REG2); // otherwise reenable bits and save them
        a |= 0b00011110;
    } // ScreenOff: save bits for later but not in register at the moment
    writeData(Mirror_PPU_CTRL_REG2, a);
    a &= 0b11100111; // disable screen for now
    writeData(PPU_CTRL_REG2, a);
    x = readData(PPU_STATUS); // reset flip-flop and reset scroll registers to zero
    a = 0x00;
    JSR(InitScroll, 4);
    writeData(PPU_SPR_ADDR, a); // reset spr-ram address register
    a = 0x02; // perform spr-ram DMA access on $0200-$02ff
    writeData(SPR_DMA, a);
    x = M(VRAM_Buffer_AddrCtrl); // load control for pointer to buffer contents
    a = M(VRAM_AddrTable_Low + x); // set indirect at $00 to pointer
    writeData(0x00, a);
    a = M(VRAM_AddrTable_High + x);
    writeData(0x01, a);
    JSR(UpdateScreen, 5); // update screen with buffer contents
    y = 0x00;
    x = M(VRAM_Buffer_AddrCtrl); // check for usage of $0341
    if (x == 0x06)
    {
        ++y; // get offset based on usage
    } // InitBuffer
    x = M(VRAM_Buffer_Offset + y);
    a = 0x00; // clear buffer header at last location
    writeData(VRAM_Buffer1_Offset + x, a);
    writeData(VRAM_Buffer1 + x, a);
    writeData(VRAM_Buffer_AddrCtrl, a); // reinit address control to $0301
    a = M(Mirror_PPU_CTRL_REG2); // copy mirror of $2001 to register
    writeData(PPU_CTRL_REG2, a);
    JSR(SoundEngine, 6); // play sound
    JSR(ReadJoypads, 7); // read joypads
    JSR(PauseRoutine, 8); // handle pause
    JSR(UpdateTopScore, 9);
    if ((M(GamePauseStatus) & 0x01) == 0) // check for pause status
    {
        a = M(TimerControl); // if master timer control not set, decrement
        if (a != 0)
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
        writeData(IntervalTimerControl, a); // if control for interval timers expired,
        x = 0x23; // interval timers will decrement along with frame timers

DecTimersLoop: // check current timer
        a = M(Timers + x);
        if (a != 0)
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
    a = M(PseudoRandomBitReg); // get first memory location of LSFR bytes
    a &= 0b00000010; // mask out all but d1
    writeData(0x00, a); // save here
    a = M(PseudoRandomBitReg + 1); // get second memory location
    a &= 0b00000010; // mask out all but d1
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
    a = M(Sprite0HitDetectFlag); // check for flag here
    if (a != 0)
    {

        do // Sprite0Clr: wait for sprite 0 flag to clear, which will
        {
            a = readData(PPU_STATUS);
            a &= 0b01000000; // not happen until vblank has ended
        } while (a != 0);
        a = M(GamePauseStatus); // if in pause mode, do not bother with sprites at all
        a >>= 1;
        if ((M(GamePauseStatus) & 0x01) != 0)
            goto Sprite0Hit;
        JSR(MoveSpritesOffscreen, 10);
        JSR(SpriteShuffler, 11);

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
    a = M(HorizontalScroll);
    writeData(PPU_SCROLL_REG, a);
    a = M(VerticalScroll);
    writeData(PPU_SCROLL_REG, a);
    a = M(Mirror_PPU_CTRL_REG1); // load saved mirror of $2000
    pha();
    writeData(PPU_CTRL_REG1, a);
    a = M(GamePauseStatus); // if in pause mode, do not perform operation mode stuff
    a >>= 1;
    if ((M(GamePauseStatus) & 0x01) == 0)
    {
        JSR(OperModeExecutionTree, 12); // otherwise do one of many, many possible subroutines
    } // SkipMainOper: reset flip-flop
    a = readData(PPU_STATUS);
    pla();
    a |= 0b10000000; // reactivate NMIs
    writeData(PPU_CTRL_REG1, a);
    return; // we are done until the next frame!

PauseRoutine:
    a = M(OperMode); // are we in victory mode?
    if (a != VictoryModeValue)
    {
        if (a != GameModeValue)
            goto ExitPause; // if not, leave
        a = M(OperMode_Task); // if we are in game mode, are we running game engine?
        if (a != 0x03)
            goto ExitPause; // if not, leave
    } // ChkPauseTimer: check if pause timer is still counting down
    a = M(GamePauseTimer);
    if (a != 0)
    {
        --M(GamePauseTimer); // if so, decrement and leave
        goto Return;

    //------------------------------------------------------------------------
    } // ChkStart: check to see if start is pressed
    a = M(SavedJoypad1Bits);
    a &= Start_Button; // on controller 1
    if (a != 0)
    {
        a = M(GamePauseStatus); // check to see if timer flag is set
        a &= 0b10000000; // and if so, do not reset timer (residual,
        if (a != 0)
            goto ExitPause; // joypad reading routine makes this unnecessary)
        a = 0x2b; // set pause timer
        writeData(GamePauseTimer, a);
        a = M(GamePauseStatus);
        y = a;
        ++y; // set pause sfx queue for next pause mode
        writeData(PauseSoundQueue, y);
        a ^= 0b00000001; // invert d0 and set d7
        a |= 0b10000000;
        if (a != 0)
            goto SetPause; // unconditional branch
    } // ClrPauseTimer: clear timer flag if timer is at zero and start button
    a = M(GamePauseStatus);
    a &= 0b01111111; // is not pressed

SetPause:
    writeData(GamePauseStatus, a);

ExitPause:
    goto Return;

//------------------------------------------------------------------------

SpriteShuffler:
    y = M(AreaType); // load level type, likely residual code
    a = 0x28; // load preset value which will put it at
    writeData(0x00, a); // sprite #10
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
    goto Return;

//------------------------------------------------------------------------

OperModeExecutionTree:
    a = M(OperMode); // this is the heart of the entire program,
    switch (a)
    {
    case 0:
        goto TitleScreenMode;
    case 1:
        goto GameMode;
    case 2:
        goto VictoryMode;
    case 3:
        goto GameOverMode;
    } // most of what goes on starts here

MoveAllSpritesOffscreen:
    y = 0x00; // this routine moves all sprites off the screen
    goto Skip_0;

MoveSpritesOffscreen:
    y = 0x04; // this routine moves all but sprite 0
Skip_0:
    a = 0xf8; // off the screen

    do // SprInitLoop: write 248 into OAM data's Y coordinate
    {
        writeData(Sprite_Y_Position + y, a);
        ++y; // which will move it off the screen
        ++y;
        ++y;
        ++y;
    } while (y != 0);
    goto Return;

//------------------------------------------------------------------------

TitleScreenMode:
    a = M(OperMode_Task);
    switch (a)
    {
    case 0:
        goto InitializeGame;
    case 1:
        goto ScreenRoutines;
    case 2:
        goto PrimaryGameSetup;
    case 3:
        goto GameMenuRoutine;
    }

GameMenuRoutine:
    y = 0x00;
    a = M(SavedJoypad1Bits); // check to see if either player pressed
    a |= M(SavedJoypad2Bits); // only the start button (either joypad)
    if (a != Start_Button)
    {
        if (a != A_Button + Start_Button)
            goto ChkSelect; // if not, branch to check select button
    } // StartGame: if either start or A + start, execute here
    goto ChkContinue;

ChkSelect: // check to see if the select button was pressed
    if (a != Select_Button)
    { // if so, branch reset demo timer
        x = M(DemoTimer); // otherwise check demo timer
        if (x == 0)
        { // if demo timer not expired, branch to check world selection
            writeData(SelectTimer, a); // set controller bits here if running demo
            JSR(DemoEngine, 13); // run through the demo actions
            if (demoOver)
                goto ResetTitle; // demo over, thus branch
            goto RunDemo; // otherwise, run game engine for demo
        } // ChkWorldSel: check to see if world selection has been enabled
        x = M(WorldSelectEnableFlag);
        if (x == 0)
            goto NullJoypad;
        if (a != B_Button)
            goto NullJoypad;
        ++y; // if so, increment Y and execute same code as select
    } // SelectBLogic: if select or B pressed, check demo timer one last time
    a = M(DemoTimer);
    if (a == 0)
        goto ResetTitle; // if demo timer expired, branch to reset title screen mode
    a = 0x18; // otherwise reset demo timer
    writeData(DemoTimer, a);
    a = M(SelectTimer); // check select/B button timer
    if (a != 0)
        goto NullJoypad; // if not expired, branch
    a = 0x10; // otherwise reset select button timer
    writeData(SelectTimer, a);
    if (y != 0x01)
    { // note this will not be run if world selection is disabled
        a = M(NumberOfPlayers); // if no, must have been the select button, therefore
        a ^= 0b00000001; // change number of players and draw icon accordingly
        writeData(NumberOfPlayers, a);
        JSR(DrawMushroomIcon, 14);
        goto NullJoypad;
    } // IncWorldSel: increment world select number
    x = M(WorldSelectNumber);
    ++x;
    a = x;
    a &= 0b00000111; // mask out higher bits
    writeData(WorldSelectNumber, a); // store as current world select number
    JSR(GoContinue, 15);

    do // UpdateShroom: write template for world select in vram buffer
    {
        a = M(WSelectBufferTemplate + x);
        writeData(VRAM_Buffer1 - 1 + x, a); // do this until all bytes are written
        ++x;
    } while (((x - 0x06) & 0x80) != 0);
    y = M(WorldNumber); // get world number from variable and increment for
    ++y; // proper display, and put in blank byte before
    writeData(VRAM_Buffer1 + 3, y); // null terminator

NullJoypad: // clear joypad bits for player 1
    a = 0x00;
    writeData(SavedJoypad1Bits, a);

RunDemo: // run game engine
    JSR(GameCoreRoutine, 16);
    a = M(GameEngineSubroutine); // check to see if we're running lose life routine
    if (a == 0x06)
    { // if not, do not do all the resetting below

ResetTitle: // reset game modes, disable
        a = 0x00;
        writeData(OperMode, a); // sprite 0 check and disable
        writeData(OperMode_Task, a); // screen output
        writeData(Sprite0HitDetectFlag, a);
        ++M(DisableScreenFlag);
        goto Return;

    //------------------------------------------------------------------------

ChkContinue: // if timer for demo has expired, reset modes
        y = M(DemoTimer);
        if (y == 0)
            goto ResetTitle;
        shiftedBit = (a & 0x80) != 0;
        if (shiftedBit) // check to see if A button was also pushed
        { // if not, don't load continue function's world number
            a = M(ContinueWorld); // load previously saved world number for secret
            JSR(GoContinue, 17); // continue function when pressing A + start
        } // StartWorld1
        JSR(LoadAreaPointer, 18);
        ++M(Hidden1UpFlag); // set 1-up box flag for both players
        ++M(OffScr_Hidden1UpFlag);
        ++M(FetchNewGameTimerFlag); // set fetch new game timer flag
        ++M(OperMode); // set next game mode
        a = M(WorldSelectEnableFlag); // if world select flag is on, then primary
        writeData(PrimaryHardMode, a); // hard mode must be on as well
        a = 0x00;
        writeData(OperMode_Task, a); // set game mode here, and clear demo timer
        writeData(DemoTimer, a);
        x = 0x17;
        a = 0x00;

        do // InitScores: clear player scores and coin displays
        {
            writeData(ScoreAndCoinDisplay + x, a);
            --x;
        } while ((x & 0x80) == 0);
    } // ExitMenu
    goto Return;

//------------------------------------------------------------------------

GoContinue: // start both players at the first area
    writeData(WorldNumber, a);
    writeData(OffScr_WorldNumber, a); // of the previously saved world number
    x = 0x00; // note that on power-up using this function
    writeData(AreaNumber, x); // will make no difference
    writeData(OffScr_AreaNumber, x);
    goto Return;

//------------------------------------------------------------------------

DrawMushroomIcon:
    y = 0x07; // read eight bytes to be read by transfer routine

    do // IconDataRead: note that the default position is set for a
    {
        a = M(MushroomIconData + y);
        writeData(VRAM_Buffer1 - 1 + y, a); // 1-player game
        --y;
    } while ((y & 0x80) == 0);
    a = M(NumberOfPlayers); // check number of players
    if (a != 0)
    { // if set to 1-player game, we're done
        a = 0x24; // otherwise, load blank tile in 1-player position
        writeData(VRAM_Buffer1 + 3, a);
        a = 0xce; // then load shroom icon tile in 2-player position
        writeData(VRAM_Buffer1 + 5, a);
    } // ExitIcon
    goto Return;

//------------------------------------------------------------------------

DemoEngine:
    x = M(DemoAction); // load current demo action
    a = M(DemoActionTimer); // load current action timer
    if (a == 0)
    { // if timer still counting down, skip
        ++x;
        ++M(DemoAction); // if expired, increment action, X, and
        demoOver = true; // demo over by default
        a = M(DemoTimingData - 1 + x); // get next timer
        writeData(DemoActionTimer, a); // store as current timer
        if (a == 0)
            goto DemoOver; // if timer already at zero, skip
    } // DoAction: get and perform action (current or next)
    a = M(DemoActionData - 1 + x);
    writeData(SavedJoypad1Bits, a);
    --M(DemoActionTimer); // decrement action timer
    demoOver = false; // demo still going

DemoOver:
    goto Return;

//------------------------------------------------------------------------

VictoryMode:
    JSR(VictoryModeSubroutines, 19); // run victory mode subroutines
    a = M(OperMode_Task); // get current task of victory mode
    if (a != 0)
    { // if on bridge collapse, skip enemy processing
        x = 0x00;
        writeData(ObjectOffset, x); // otherwise reset enemy object offset
        JSR(EnemiesAndLoopsCore, 20); // and run enemy code
    } // AutoPlayer: get player's relative coordinates
    JSR(RelativePlayerPosition, 21);
    goto PlayerGfxHandler; // draw the player, then leave

VictoryModeSubroutines:
    a = M(OperMode_Task);
    switch (a)
    {
    case 0:
        goto BridgeCollapse;
    case 1:
        goto SetupVictoryMode;
    case 2:
        goto PlayerVictoryWalk;
    case 3:
        goto PrintVictoryMessages;
    case 4:
        goto PlayerEndWorld;
    }

SetupVictoryMode:
    x = M(ScreenRight_PageLoc); // get page location of right side of screen
    ++x; // increment to next page
    writeData(DestinationPageLoc, x); // store here
    a = EndOfCastleMusic;
    writeData(EventMusicQueue, a); // play win castle music
    goto IncModeTask_B; // jump to set next major task in victory mode

PlayerVictoryWalk:
    y = 0x00; // set value here to not walk player by default
    writeData(VictoryWalkControl, y);
    a = M(Player_PageLoc); // get player's page location
    if (a == M(DestinationPageLoc))
    { // if page locations don't match, branch
        a = M(Player_X_Position); // otherwise get player's horizontal position
        if (a >= 0x60)
            goto DontWalk; // if still on other page, branch ahead
    } // PerformWalk: otherwise increment value and Y
    ++M(VictoryWalkControl);
    ++y; // note Y will be used to walk the player

DontWalk: // put contents of Y in A and
    a = y;
    JSR(AutoControlPlayer, 22); // use A to move player to the right or not
    a = M(ScreenLeft_PageLoc); // check page location of left side of screen
    if (a != M(DestinationPageLoc))
    { // branch if equal to change modes if necessary
        wide = M(ScrollFractional) + 0x80; // do fixed point math on fractional part of scroll
        writeData(ScrollFractional, LOBYTE(wide)); // save fractional movement amount
        a = (uint8_t)(0x01 + HIBYTE(wide)); // one pixel per frame, plus the carry out of the fraction
        y = a; // use as scroll amount
        JSR(ScrollScreen, 23); // do sub to scroll the screen
        JSR(UpdScrollVar, 24); // do another sub to update screen and scroll variables
        ++M(VictoryWalkControl); // increment value to stay in this routine
    } // ExitVWalk: load value set here
    a = M(VictoryWalkControl);
    if (a != 0)
    { // if zero, branch to change modes
        goto Return; // otherwise leave

    //------------------------------------------------------------------------

PrintVictoryMessages:
        a = M(SecondaryMsgCounter); // load secondary message counter
        if (a != 0)
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
            a = M(CurrentPlayer); // otherwise get player currently on the screen
            if (a == 0)
                goto EvalForMusic; // if mario, branch
            ++y; // otherwise increment Y once for luigi and
            if (y != 0)
                goto EvalForMusic; // do an unconditional branch to the same place
        } // SecondPartMsg: increment Y to do world 8's message
        ++y;
        a = M(WorldNumber);
        if (a == World8)
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
                writeData(EventMusicQueue, a); // otherwise load victory music first (world 8 only)
            } // PrintMsg: put primary message counter in A
            a = y;
            a += 0x0c; // ($0c-$0d = first), ($0e = world 1-7's), ($0f-$12 = world 8's)
            writeData(VRAM_Buffer_AddrCtrl, a); // write message counter to vram address controller

IncMsgCounter:
            a = M(SecondaryMsgCounter);
            wide = ((M(PrimaryMsgCounter) << 8) | a) + 0x04; // add four to secondary message counter
            writeData(SecondaryMsgCounter, LOBYTE(wide));
            writeData(PrimaryMsgCounter, HIBYTE(wide));
            a = HIBYTE(wide);

            // SetEndTimer: if not reached value yet, branch to leave
            if (a < 0x07)
                goto ExitMsgs;
        }
        a = 0x06;
        writeData(WorldEndTimer, a); // otherwise set world end timer
    } // IncModeTask_A: move onto next task in mode
    ++M(OperMode_Task);

ExitMsgs: // leave
    goto Return;

//------------------------------------------------------------------------

PlayerEndWorld:
    a = M(WorldEndTimer); // check to see if world end timer expired
    if (a != 0)
        goto EndExitOne; // branch to leave if not
    y = M(WorldNumber); // check world number
    if (y < World8)
    { // thus branch to read controller
        a = 0x00;
        writeData(AreaNumber, a); // otherwise initialize area number used as offset
        writeData(LevelNumber, a); // and level number control to start at area 1
        writeData(OperMode_Task, a); // initialize secondary mode of operation
        ++M(WorldNumber); // increment world number to move onto the next world
        JSR(LoadAreaPointer, 25); // get area address offset for the next area
        ++M(FetchNewGameTimerFlag); // set flag to load game timer from header
        a = GameModeValue;
        writeData(OperMode, a); // set mode of operation to game mode

EndExitOne: // and leave
        goto Return;

    //------------------------------------------------------------------------
    } // EndChkBButton
    a = M(SavedJoypad1Bits);
    a |= M(SavedJoypad2Bits); // check to see if B button was pressed on
    a &= B_Button; // either controller
    if (a != 0)
    { // branch to leave if not
        a = 0x01; // otherwise set world selection flag
        writeData(WorldSelectEnableFlag, a);
        a = 0xff; // remove onscreen player's lives
        writeData(NumberofLives, a);
        JSR(TerminateGame, 26); // do sub to continue other player or end game
    } // EndExitTwo: leave
    goto Return;

//------------------------------------------------------------------------

FloateyNumbersRoutine:
    a = M(FloateyNum_Control + x); // load control for floatey number
    if (a == 0)
        goto EndExitOne; // if zero, branch to leave
    if (a >= 0x0b)
    {
        a = 0x0b; // otherwise set to $0b, thus keeping
        writeData(FloateyNum_Control + x, a); // it in range
    } // ChkNumTimer: use as Y
    y = a;
    a = M(FloateyNum_Timer + x); // check value here
    if (a == 0)
    { // if nonzero, branch ahead
        writeData(FloateyNum_Control + x, a); // initialize floatey number control and leave
        goto Return;

    //------------------------------------------------------------------------
    } // DecNumTimer: decrement value here
    --M(FloateyNum_Timer + x);
    if (a == 0x2b)
    {
        if (y == 0x0b)
        { // branch ahead if not found
            ++M(NumberofLives); // give player one extra life (1-up)
            a = Sfx_ExtraLife;
            writeData(Square2SoundQueue, a); // and play the 1-up sound
        } // LoadNumTiles: load point value here
        a = M(ScoreUpdateData + y);
        a >>= 1; // move high nybble to low
        a >>= 1;
        a >>= 1;
        a >>= 1;
        x = a; // use as X offset, essentially the digit
        a = M(ScoreUpdateData + y); // load again and this time
        a &= 0b00001111; // mask out the high nybble
        writeData(DigitModifier + x, a); // store as amount to add to the digit
        JSR(AddToScore, 27); // update the score accordingly
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
    a = M(Enemy_State + x);
    if (a >= 0x02)
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
    JSR(DumpTwoSpr, 28); // left and right sprite's Y coordinates
    a = M(FloateyNum_X_Pos + x); // get horizontal coordinate
    writeData(Sprite_X_Position + y, a); // store into X coordinate of left sprite
    a += 0x08; // add eight pixels and store into X
    writeData(Sprite_X_Position + 4 + y, a); // coordinate of right sprite
    a = 0x02;
    writeData(Sprite_Attributes + y, a); // set palette control in attribute bytes
    writeData(Sprite_Attributes + 4 + y, a); // of left and right sprites
    a = M(FloateyNum_Control + x);
    a <<= 1; // multiply our floatey number control by 2
    x = a; // and use as offset for look-up table
    a = M(FloateyNumTileData + x);
    writeData(Sprite_Tilenumber + y, a); // display first half of number of points
    a = M(FloateyNumTileData + 1 + x);
    writeData(Sprite_Tilenumber + 4 + y, a); // display the second half
    x = M(ObjectOffset); // get enemy object offset and leave
    goto Return;

//------------------------------------------------------------------------

ScreenRoutines:
    a = M(ScreenRoutineTask); // run one of the following subroutines
    switch (a)
    {
    case 0:
        goto InitScreen;
    case 1:
        goto SetupIntermediate;
    case 2:
        goto WriteTopStatusLine;
    case 3:
        goto WriteBottomStatusLine;
    case 4:
        goto DisplayTimeUp;
    case 5:
        goto ResetSpritesAndScreenTimer;
    case 6:
        goto DisplayIntermediate;
    case 7:
        goto ResetSpritesAndScreenTimer;
    case 8:
        goto AreaParserTaskControl;
    case 9:
        goto GetAreaPalette;
    case 10:
        goto GetBackgroundColor;
    case 11:
        goto GetAlternatePalette1;
    case 12:
        goto DrawTitleScreen;
    case 13:
        goto ClearBuffersDrawIcon;
    case 14:
        goto WriteTopScore;
    }

InitScreen:
    JSR(MoveAllSpritesOffscreen, 29); // initialize all sprites including sprite #0
    JSR(InitializeNameTables, 30); // and erase both name and attribute tables
    a = M(OperMode);
    if (a != 0)
    { // if mode still 0, do not load
        x = 0x03; // into buffer pointer
        goto SetVRAMAddr_A;

SetupIntermediate:
        a = M(BackgroundColorCtrl); // save current background color control
        pha(); // and player status to stack
        a = M(PlayerStatus);
        pha();
        a = 0x00; // set background color to black
        writeData(PlayerStatus, a); // and player status to not fiery
        a = 0x02; // this is the ONLY time background color control
        writeData(BackgroundColorCtrl, a); // is set to less than 4
        JSR(GetPlayerColors, 31);
        pla(); // we only execute this routine for
        writeData(PlayerStatus, a); // the intermediate lives display
        pla(); // and once we're done, we return bg
        writeData(BackgroundColorCtrl, a); // color ctrl and player status from stack
        goto IncSubtask; // then move onto the next task

GetAreaPalette:
        y = M(AreaType); // select appropriate palette to load
        x = M(AreaPalette + y); // based on area type

SetVRAMAddr_A: // store offset into buffer control
        writeData(VRAM_Buffer_AddrCtrl, x);
    } // NextSubtask: move onto next task
    goto IncSubtask;

GetBackgroundColor:
    y = M(BackgroundColorCtrl); // check background color control
    if (y != 0)
    { // if not set, increment task and fetch palette
        a = M(BGColorCtrl_Addr - 4 + y); // put appropriate palette into vram
        writeData(VRAM_Buffer_AddrCtrl, a); // note that if set to 5-7, $0301 will not be read
    } // NoBGColor: increment to next subtask and plod on through
    ++M(ScreenRoutineTask);

GetPlayerColors:
    x = M(VRAM_Buffer1_Offset); // get current buffer offset
    y = 0x00;
    a = M(CurrentPlayer); // check which player is on the screen
    if (a != 0)
    {
        y = 0x04; // load offset for luigi
    } // ChkFiery: check player status
    a = M(PlayerStatus);
    if (a == 0x02)
    { // if fiery, load alternate offset for fiery player
        y = 0x08;
    } // StartClrGet: do four colors
    a = 0x03;
    writeData(0x00, a);

    do // ClrGetLoop: fetch player colors and store them
    {
        a = M(PlayerColors + y);
        writeData(VRAM_Buffer1 + 3 + x, a); // in the buffer
        ++y;
        ++x;
        --M(0x00);
    } while ((M(0x00) & 0x80) == 0);
    x = M(VRAM_Buffer1_Offset); // load original offset from before
    y = M(BackgroundColorCtrl); // if this value is four or greater, it will be set
    if (y == 0)
    { // therefore use it as offset to background color
        y = M(AreaType); // otherwise use area type bits from area offset as offset
    } // SetBGColor: to background color instead
    a = M(BackgroundColors + y);
    writeData(VRAM_Buffer1 + 3 + x, a);
    a = 0x3f; // set for sprite palette address
    writeData(VRAM_Buffer1 + x, a); // save to buffer
    a = 0x10;
    writeData(VRAM_Buffer1 + 1 + x, a);
    a = 0x04; // write length byte to buffer
    writeData(VRAM_Buffer1 + 2 + x, a);
    a = 0x00; // now the null terminator
    writeData(VRAM_Buffer1 + 7 + x, a);
    a = x; // move the buffer pointer ahead 7 bytes
    a += 0x07;

SetVRAMOffset: // store as new vram buffer offset
    writeData(VRAM_Buffer1_Offset, a);
    goto Return;

//------------------------------------------------------------------------

GetAlternatePalette1:
    a = M(AreaStyle); // check for mushroom level style
    if (a == 0x01)
    {
        a = 0x0b; // if found, load appropriate palette

SetVRAMAddr_B:
        writeData(VRAM_Buffer_AddrCtrl, a);
    } // NoAltPal: now onto the next task
    goto IncSubtask;

WriteTopStatusLine:
    a = 0x00; // select main status bar
    JSR(WriteGameText, 32); // output it
    goto IncSubtask; // onto the next task

WriteBottomStatusLine:
    JSR(GetSBNybbles, 33); // write player's score and coin tally to screen
    x = M(VRAM_Buffer1_Offset);
    a = 0x20; // write address for world-area number on screen
    writeData(VRAM_Buffer1 + x, a);
    a = 0x73;
    writeData(VRAM_Buffer1 + 1 + x, a);
    a = 0x03; // write length for it
    writeData(VRAM_Buffer1 + 2 + x, a);
    y = M(WorldNumber); // first the world number
    ++y;
    a = y;
    writeData(VRAM_Buffer1 + 3 + x, a);
    a = 0x28; // next the dash
    writeData(VRAM_Buffer1 + 4 + x, a);
    y = M(LevelNumber); // next the level number
    ++y; // increment for proper number display
    a = y;
    writeData(VRAM_Buffer1 + 5 + x, a);
    a = 0x00; // put null terminator on
    writeData(VRAM_Buffer1 + 6 + x, a);
    a = x; // move the buffer offset up by 6 bytes
    a += 0x06;
    writeData(VRAM_Buffer1_Offset, a);
    goto IncSubtask;

DisplayTimeUp:
    a = M(GameTimerExpiredFlag); // if game timer not expired, increment task
    if (a != 0)
    { // control 2 tasks forward, otherwise, stay here
        a = 0x00;
        writeData(GameTimerExpiredFlag, a); // reset timer expiration flag
        a = 0x02; // output time-up screen to buffer
        goto OutputInter;
    } // NoTimeUp: increment control task 2 tasks forward
    ++M(ScreenRoutineTask);
    goto IncSubtask;

DisplayIntermediate:
    a = M(OperMode); // check primary mode of operation
    if (a == 0)
        goto NoInter; // if in title screen mode, skip this
    if (a != GameOverModeValue)
    { // if so, proceed to display game over screen
        a = M(AltEntranceControl); // otherwise check for mode of alternate entry
        if (a != 0)
            goto NoInter; // and branch if found
        y = M(AreaType); // check if we are on castle level
        if (y != 0x03)
        {
            a = M(DisableIntermediate); // if this flag is set, skip intermediate lives display
            if (a != 0)
                goto NoInter; // and jump to specific task, otherwise
        } // PlayerInter: put player in appropriate place for
        JSR(DrawPlayer_Intermediate, 34);
        a = 0x01; // lives display, then output lives display to buffer

OutputInter:
        JSR(WriteGameText, 35);
        JSR(ResetScreenTimer, 36);
        a = 0x00;
        writeData(DisableScreenFlag, a); // reenable screen output
        goto Return;

    //------------------------------------------------------------------------
    } // GameOverInter: set screen timer
    a = 0x12;
    writeData(ScreenTimer, a);
    a = 0x03; // output game over screen to buffer
    JSR(WriteGameText, 37);
    goto IncModeTask_B;

NoInter: // set for specific task and leave
    a = 0x08;
    writeData(ScreenRoutineTask, a);
    goto Return;

//------------------------------------------------------------------------

AreaParserTaskControl:
    ++M(DisableScreenFlag); // turn off screen

    do // TaskLoop: render column set of current area
    {
        JSR(AreaParserTaskHandler, 38);
        a = M(AreaParserTaskNum); // check number of tasks
    } while (a != 0); // if tasks still not all done, do another one
    --M(ColumnSets); // do we need to render more column sets?
    if ((M(ColumnSets) & 0x80) != 0)
    {
        ++M(ScreenRoutineTask); // if not, move on to the next task
    } // OutputCol: set vram buffer to output rendered column set
    a = 0x06;
    writeData(VRAM_Buffer_AddrCtrl, a); // on next NMI
    goto Return;

//------------------------------------------------------------------------

DrawTitleScreen:
    a = M(OperMode); // are we in title screen mode?
    if (a != 0)
        goto IncModeTask_B; // if not, exit
    a = HIBYTE(TitleScreenDataOffset); // load address $1ec0 into
    writeData(PPU_ADDRESS, a); // the vram address register
    a = LOBYTE(TitleScreenDataOffset);
    writeData(PPU_ADDRESS, a);
    a = 0x03; // put address $0300 into
    writeData(0x01, a); // the indirect at $00
    y = 0x00;
    writeData(0x00, y);
    a = readData(PPU_DATA); // do one garbage read

OutputTScr: // get title screen from chr-rom
    a = readData(PPU_DATA);
    writeData(W(0x00) + y, a); // store 256 bytes into buffer
    ++y;
    if (y == 0)
    { // if not past 256 bytes, do not increment
        ++M(0x01); // otherwise increment high byte of indirect
    } // ChkHiByte: check high byte?
    a = M(0x01);
    if (a != 0x04)
        goto OutputTScr; // if not, loop back and do another
    if (y < 0x3a)
        goto OutputTScr; // if not, loop back and do another
    a = 0x05; // set buffer transfer control to $0300,
    goto SetVRAMAddr_B; // increment task and exit

ClearBuffersDrawIcon:
    a = M(OperMode); // check game mode
    if (a != 0)
        goto IncModeTask_B; // if not title screen mode, leave
    x = 0x00; // otherwise, clear buffer space

    do // TScrClear
    {
        writeData(VRAM_Buffer1 - 1 + x, a);
        writeData(VRAM_Buffer1 - 1 + 0x100 + x, a);
        --x;
    } while (x != 0);
    JSR(DrawMushroomIcon, 39); // draw player select icon

IncSubtask: // move onto next task
    ++M(ScreenRoutineTask);
    goto Return;

//------------------------------------------------------------------------

WriteTopScore:
    a = 0xfa; // run display routine to display top score on title
    JSR(UpdateNumber, 40);

IncModeTask_B: // move onto next mode
    ++M(OperMode_Task);
    goto Return;

//------------------------------------------------------------------------

WriteGameText:
    pha(); // save text number to stack
    a <<= 1;
    y = a; // multiply by 2 and use as offset
    if (y < 0x04)
        goto LdGameText; // branch to use current offset as-is
    if (y >= 0x08)
    { // branch to check players
        y = 0x08; // otherwise warp zone, therefore set offset
    } // Chk2Players: check for number of players
    a = M(NumberOfPlayers);
    if (a != 0)
        goto LdGameText; // if there are two, use current offset to also print name
    ++y; // otherwise increment offset by one to not print name

LdGameText: // get offset to message we want to print
    x = M(GameTextOffsets + y);
    y = 0x00;

GameTextLoop: // load message data
    a = M(GameText + x);
    if (a != 0xff)
    { // branch to end text if found
        writeData(VRAM_Buffer1 + y, a); // otherwise write data to buffer
        ++x; // and increment increment
        ++y;
        if (y != 0)
            goto GameTextLoop; // do this for 256 bytes if no terminator found
    } // EndGameText: put null terminator at end
    a = 0x00;
    writeData(VRAM_Buffer1 + y, a);
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
                writeData(VRAM_Buffer1 + 7, y); // the number of lives exceeds 19
            } // PutLives
            writeData(VRAM_Buffer1 + 8, a);
            y = M(WorldNumber); // write world and level numbers (incremented for display)
            ++y; // to the buffer in the spaces surrounding the dash
            writeData(VRAM_Buffer1 + 19, y);
            y = M(LevelNumber);
            ++y;
            writeData(VRAM_Buffer1 + 21, y); // we're done here
            goto Return;

        //------------------------------------------------------------------------
        } // CheckPlayerName
        a = M(NumberOfPlayers); // check number of players
        if (a == 0)
            goto ExitChkName; // if only 1 player, leave
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
            goto ExitChkName; // if mario is current player, do not change the name
        y = 0x04;

        do // NameLoop: otherwise, replace "MARIO" with "LUIGI"
        {
            a = M(LuigiName + y);
            writeData(VRAM_Buffer1 + 3 + y, a);
            --y;
        } while ((y & 0x80) == 0); // do this until each letter is replaced

ExitChkName:
        goto Return;

    //------------------------------------------------------------------------
    } // PrintWarpZoneNumbers
    a -= 0x04; // subtract 4 and then shift to the left
    a <<= 1; // twice to get proper warp zone number
    a <<= 1; // offset
    x = a;
    y = 0x00;

    do // WarpNumLoop: print warp zone numbers into the
    {
        a = M(WarpZoneNumbers + x);
        writeData(VRAM_Buffer1 + 27 + y, a); // placeholders from earlier
        ++x;
        ++y; // put a number in every fourth space
        ++y;
        ++y;
        ++y;
    } while (y < 0x0c);
    a = 0x2c; // load new buffer pointer at end of message
    goto SetVRAMOffset;

ResetSpritesAndScreenTimer:
    a = M(ScreenTimer); // check if screen timer has expired
    if (a == 0)
    { // if not, branch to leave
        JSR(MoveAllSpritesOffscreen, 41); // otherwise reset sprites now

ResetScreenTimer:
        a = 0x07; // reset timer again
        writeData(ScreenTimer, a);
        ++M(ScreenRoutineTask); // move onto next task
    } // NoReset
    goto Return;

//------------------------------------------------------------------------

RenderAreaGraphics:
    a = M(CurrentColumnPos); // store LSB of where we're at
    a &= 0x01;
    writeData(0x05, a);
    y = M(VRAM_Buffer2_Offset); // store vram buffer offset
    writeData(0x00, y);
    a = M(CurrentNTAddr_Low); // get current name table address we're supposed to render
    writeData(VRAM_Buffer2 + 1 + y, a);
    a = M(CurrentNTAddr_High);
    writeData(VRAM_Buffer2 + y, a);
    a = 0x9a; // store length byte of 26 here with d7 set
    writeData(VRAM_Buffer2 + 2 + y, a); // to increment by 32 (in columns)
    a = 0x00; // init attribute row
    writeData(0x04, a);
    x = a;

    do // DrawMTLoop: store init value of 0 or incremented offset for buffer
    {
        writeData(0x01, x);
        a = M(MetatileBuffer + x); // get first metatile number, and mask out all but 2 MSB
        a &= 0b11000000;
        writeData(0x03, a); // store attribute table bits here
        a >>= 6; // note that metatile format is %xx000000 attribute table bits,
        y = a; // %00xxxxxx metatile number, so move the bits to d1-d0 and use as offset here
        a = M(MetatileGraphics_Low + y); // get address to graphics table from here
        writeData(0x06, a);
        a = M(MetatileGraphics_High + y);
        writeData(0x07, a);
        a = M(MetatileBuffer + x); // get metatile number again
        a <<= 1; // multiply by 4 and use as tile offset
        a <<= 1;
        writeData(0x02, a);
        a = M(AreaParserTaskNum); // get current task number for level processing and
        a &= 0b00000001; // mask out all but LSB, then invert LSB, multiply by 2
        a ^= 0b00000001; // to get the correct column position in the metatile,
        a <<= 1; // then add to the tile offset so we can draw either side
        a += M(0x02); // of the metatiles
        y = a;
        x = M(0x00); // use vram buffer offset from before as X
        a = M(W(0x06) + y);
        writeData(VRAM_Buffer2 + 3 + x, a); // get first tile number (top left or top right) and store
        ++y;
        a = M(W(0x06) + y); // now get the second (bottom left or bottom right) and store
        writeData(VRAM_Buffer2 + 4 + x, a);
        y = M(0x04); // get current attribute row
        a = M(0x05); // get LSB of current column where we're at, and
        if (a == 0)
        { // branch if set (clear = left attrib, set = right)
            a = M(0x01); // get current row we're rendering
            a >>= 1; // branch if LSB set (clear = top left, set = bottom left)
            if ((M(0x01) & 0x01) != 0)
                goto LLeft;
            M(0x03) >>= 6; // move the attribute bits into d1-d0, for upper left square
            goto SetAttrib;
        } // RightCheck: get LSB of current row we're rendering
        a = M(0x01);
        a >>= 1; // branch if set (clear = top right, set = bottom right)
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
        a = M(AttributeBuffer + y);
        a |= M(0x03); // if any, and put new bits, if any, onto
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
    a = 0x00;
    writeData(VRAM_Buffer2 + y, a); // put null terminator at end of data for name table
    writeData(VRAM_Buffer2_Offset, y); // store new buffer offset
    ++M(CurrentNTAddr_Low); // increment name table address low
    a = M(CurrentNTAddr_Low); // check current low byte
    a &= 0b00011111; // if no wraparound, just skip this part
    if (a == 0)
    {
        a = 0x80; // if wraparound occurs, make sure low byte stays
        writeData(CurrentNTAddr_Low, a); // just under the status bar
        a = M(CurrentNTAddr_High); // and then invert d2 of the name table address high
        a ^= 0b00000100; // to move onto the next appropriate name table
        writeData(CurrentNTAddr_High, a);
    } // ExitDrawM: jump to set buffer to $0341 and leave
    goto SetVRAMCtrl;

RenderAttributeTables:
    a = M(CurrentNTAddr_Low); // get low byte of next name table address
    a &= 0b00011111; // to be written to, mask out all but 5 LSB,
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
        a = M(0x00);
        writeData(VRAM_Buffer2 + y, a); // store high byte of attribute table address
        a = M(0x01);
        a += 0x08; // below the status bar, and store
        writeData(VRAM_Buffer2 + 1 + y, a);
        writeData(0x01, a); // also store in temp again
        a = M(AttributeBuffer + x); // fetch current attribute table byte and store
        writeData(VRAM_Buffer2 + 3 + y, a); // in the buffer
        a = 0x01;
        writeData(VRAM_Buffer2 + 2 + y, a); // store length of 1 in buffer
        a >>= 1;
        writeData(AttributeBuffer + x, a); // clear current byte in attribute buffer
        ++y; // increment buffer offset by 4 bytes
        ++y;
        ++y;
        ++y;
        ++x; // increment attribute offset and check to see
    } while (x < 0x07);
    writeData(VRAM_Buffer2 + y, a); // put null terminator at the end
    writeData(VRAM_Buffer2_Offset, y); // store offset in case we want to do any more

SetVRAMCtrl:
    a = 0x06;
    writeData(VRAM_Buffer_AddrCtrl, a); // set buffer to $0341 and leave
    goto Return;

//------------------------------------------------------------------------

ColorRotation:
    a = M(FrameCounter); // get frame counter
    a &= 0x07; // mask out all but three LSB
    if (a != 0)
        goto ExitColorRot; // branch if not set to zero to do this every eighth frame
    x = M(VRAM_Buffer1_Offset); // check vram buffer offset
    if (x >= 0x31)
        goto ExitColorRot; // if offset over 48 bytes, branch to leave
    y = a; // otherwise use frame counter's 3 LSB as offset here

    do // GetBlankPal: get blank palette for palette 3
    {
        a = M(BlankPalette + y);
        writeData(VRAM_Buffer1 + x, a); // store it in the vram buffer
        ++x; // increment offsets
        ++y;
    } while (y < 0x08); // do this until all bytes are copied
    x = M(VRAM_Buffer1_Offset); // get current vram buffer offset
    a = 0x03;
    writeData(0x00, a); // set counter here
    a = M(AreaType); // get area type
    a <<= 1; // multiply by 4 to get proper offset
    a <<= 1;
    y = a; // save as offset here

    do // GetAreaPal: fetch palette to be written based on area type
    {
        a = M(Palette3Data + y);
        writeData(VRAM_Buffer1 + 3 + x, a); // store it to overwrite blank palette in vram buffer
        ++y;
        ++x;
        --M(0x00); // decrement counter
    } while ((M(0x00) & 0x80) == 0); // do this until the palette is all copied
    x = M(VRAM_Buffer1_Offset); // get current vram buffer offset
    y = M(ColorRotateOffset); // get color cycling offset
    a = M(ColorRotatePalette + y);
    writeData(VRAM_Buffer1 + 4 + x, a); // get and store current color in second slot of palette
    a = M(VRAM_Buffer1_Offset);
    a += 0x07;
    writeData(VRAM_Buffer1_Offset, a);
    ++M(ColorRotateOffset); // increment color cycling offset
    a = M(ColorRotateOffset);
    if (a < 0x06)
        goto ExitColorRot; // if so, branch to leave
    a = 0x00;
    writeData(ColorRotateOffset, a); // otherwise, init to keep it in range

ExitColorRot: // leave
    goto Return;

//------------------------------------------------------------------------

RemoveCoin_Axe:
    y = 0x41; // set low byte so offset points to $0341
    a = 0x03; // load offset for default blank metatile
    x = M(AreaType); // check area type
    if (x == 0)
    { // if not water type, use offset
        a = 0x04; // otherwise load offset for blank metatile used in water
    } // WriteBlankMT: do a sub to write blank metatile to vram buffer
    JSR(PutBlockMetatile, 42);
    a = 0x06;
    writeData(VRAM_Buffer_AddrCtrl, a); // set vram address controller to $0341 and leave
    goto Return;

//------------------------------------------------------------------------

ReplaceBlockMetatile:
    JSR(WriteBlockMetatile, 43); // write metatile to vram buffer to replace block object
    ++M(Block_ResidualCounter); // increment unused counter (residual code)
    --M(Block_RepFlag + x); // decrement flag (residual code)
    goto Return; // leave

//------------------------------------------------------------------------

DestroyBlockMetatile:
    a = 0x00; // force blank metatile if branched/jumped to this point

WriteBlockMetatile:
    y = 0x03; // load offset for blank metatile
    if (a == 0x00)
        goto UseBOffset; // branch if found (unconditional if branched from 8a6b)
    y = 0x00; // load offset for brick metatile w/ line
    if (a == 0x58)
        goto UseBOffset; // use offset if metatile is brick with coins (w/ line)
    if (a == 0x51)
        goto UseBOffset; // use offset if metatile is breakable brick w/ line
    ++y; // increment offset for brick metatile w/o line
    if (a == 0x5d)
        goto UseBOffset; // use offset if metatile is brick with coins (w/o line)
    if (a == 0x52)
        goto UseBOffset; // use offset if metatile is breakable brick w/o line
    ++y; // if any other metatile, increment offset for empty block

UseBOffset: // put Y in A
    a = y;
    y = M(VRAM_Buffer1_Offset); // get vram buffer offset
    ++y; // move onto next byte
    JSR(PutBlockMetatile, 44); // get appropriate block data and write to vram buffer

MoveVOffset: // decrement vram buffer offset
    --y;
    a = y; // add 10 bytes to it
    a += 10;
    goto SetVRAMOffset; // branch to store as new vram buffer offset

PutBlockMetatile:
    writeData(0x00, x); // store control bit from SprDataOffset_Ctrl
    writeData(0x01, y); // store vram buffer offset for next byte
    a <<= 1;
    a <<= 1; // multiply A by four and use as X
    x = a;
    y = 0x20; // load high byte for name table 0
    a = M(0x06); // get low byte of block buffer pointer
    if (a >= 0xd0)
    { // if not, use current high byte
        y = 0x24; // otherwise load high byte for name table 1
    } // SaveHAdder: save high byte here
    writeData(0x03, y);
    a &= 0x0f; // mask out high nybble of block buffer pointer
    a <<= 1; // multiply by 2 to get appropriate name table low byte
    writeData(0x04, a); // and then store it here
    // the vertical offset, times four, is a ten-bit quantity; add the sixteen-bit
    // name table address in $03:$04 to it and store the result back in $05:$04
    wide = (uint8_t)(M(0x02) + 0x20) << 2; // add 32 pixels for the status bar
    wide += (M(0x03) << 8) | M(0x04); // add the name table address
    writeData(0x04, LOBYTE(wide)); // and store here
    writeData(0x05, HIBYTE(wide)); // store here
    a = HIBYTE(wide);
    y = M(0x01); // get vram buffer offset to be used

RemBridge: // write top left and top right
    a = M(BlockGfxData + x);
    writeData(VRAM_Buffer1 + 2 + y, a); // tile numbers into first spot
    a = M(BlockGfxData + 1 + x);
    writeData(VRAM_Buffer1 + 3 + y, a);
    a = M(BlockGfxData + 2 + x); // write bottom left and bottom
    writeData(VRAM_Buffer1 + 7 + y, a); // right tiles numbers into
    a = M(BlockGfxData + 3 + x); // second spot
    writeData(VRAM_Buffer1 + 8 + y, a);
    a = M(0x04);
    writeData(VRAM_Buffer1 + y, a); // write low byte of name table
    a += 0x20; // add 32 bytes to value
    writeData(VRAM_Buffer1 + 5 + y, a); // write low byte of name table
    a = M(0x05); // plus 32 bytes into second slot
    writeData(VRAM_Buffer1 - 1 + y, a); // write high byte of name
    writeData(VRAM_Buffer1 + 4 + y, a); // table address to both slots
    a = 0x02;
    writeData(VRAM_Buffer1 + 1 + y, a); // put length of 2 in
    writeData(VRAM_Buffer1 + 6 + y, a); // both slots
    a = 0x00;
    writeData(VRAM_Buffer1 + 9 + y, a); // put null terminator at end
    x = M(0x00); // get offset control bit here
    goto Return; // and leave

//------------------------------------------------------------------------

// The ROM reaches a routine from a table of addresses by calling this, and the
// decompiled code dispatches with a switch instead.
//
JumpEngine:
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

InitializeNameTables:
    a = readData(PPU_STATUS); // reset flip-flop
    a = M(Mirror_PPU_CTRL_REG1); // load mirror of ppu reg $2000
    a |= 0b00010000; // set sprites for first 4k and background for second 4k
    a &= 0b11110000; // clear rest of lower nybble, leave higher alone
    JSR(WritePPUReg1, 45);
    a = 0x24; // set vram address to start of name table 1
    JSR(WriteNTAddr, 46);
    a = 0x20; // and then set it to name table 0

WriteNTAddr:
    writeData(PPU_ADDRESS, a);
    a = 0x00;
    writeData(PPU_ADDRESS, a);
    x = 0x04; // clear name table with blank tile #24
    y = 0xc0;
    a = 0x24;

InitNTLoop: // count out exactly 768 tiles
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
    goto InitScroll; // initialize scroll registers to zero

ReadJoypads:
    a = 0x01; // reset and clear strobe of joypad ports
    writeData(JOYPAD_PORT, a);
    a >>= 1;
    x = a; // start with joypad 1's port
    writeData(JOYPAD_PORT, a);
    JSR(ReadPortBits, 47);
    ++x; // increment for joypad 2's port

ReadPortBits:
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
        goto Return;

    //------------------------------------------------------------------------
    } // Save8Bits
    pla();
    writeData(JoypadBitMask + x, a); // save with all bits in another place and leave
    goto Return;

//------------------------------------------------------------------------

    do // WriteBufferToScreen
    {
        writeData(PPU_ADDRESS, a); // store high byte of vram address
        ++y;
        a = M(W(0x00) + y); // load next byte (second)
        writeData(PPU_ADDRESS, a); // store low byte of vram address
        ++y;
        a = M(W(0x00) + y); // load next byte (third)
        a <<= 1; // shift to left and save in stack
        pha();
        a = M(Mirror_PPU_CTRL_REG1); // load mirror of $2000,
        a |= 0b00000100; // set ppu to increment by 32 by default
        if ((M(W(0x00) + y) & 0x80) == 0)
        { // if d7 of third byte was clear, ppu will
            a &= 0b11111011; // only increment by 1
        } // SetupWrites: write to register
        JSR(WritePPUReg1, 48);
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
            a = M(W(0x00) + y);
            writeData(PPU_DATA, a);
            --x; // done writing?
        } while (x != 0);
        wide = ((M(0x01) << 8) | M(0x00)) + y + 1; // add end length plus one to the indirect at $00
        writeData(0x00, LOBYTE(wide)); // to allow this routine to read another set of updates
        writeData(0x01, HIBYTE(wide));
        a = HIBYTE(wide);
        a = 0x3f; // sets vram address to $3f00
        writeData(PPU_ADDRESS, a);
        a = 0x00;
        writeData(PPU_ADDRESS, a);
        writeData(PPU_ADDRESS, a); // then reinitializes it for some reason
        writeData(PPU_ADDRESS, a);

UpdateScreen: // reset flip-flop
        x = readData(PPU_STATUS);
        y = 0x00; // load first byte from indirect as a pointer
        a = M(W(0x00) + y);
    } while (a != 0); // if byte is zero we have no further updates to make here

InitScroll: // store contents of A into scroll registers
    writeData(PPU_SCROLL_REG, a);
    writeData(PPU_SCROLL_REG, a); // and end whatever subroutine led us here
    goto Return;

//------------------------------------------------------------------------

WritePPUReg1:
    writeData(PPU_CTRL_REG1, a); // write contents of A to PPU register 1
    writeData(Mirror_PPU_CTRL_REG1, a); // and its mirror
    goto Return;

//------------------------------------------------------------------------

PrintStatusBarNumbers:
    writeData(0x00, a); // store player-specific offset
    JSR(OutputNumbers, 49); // use first nybble to print the coin display
    a = M(0x00); // move high nybble to low
    a >>= 1; // and print to score display
    a >>= 1;
    a >>= 1;
    a >>= 1;

OutputNumbers:
    a += 0x01;
    a &= 0b00001111; // mask out high nybble
    if (a < 0x06)
    {
        pha(); // save incremented value to stack for now and
        a <<= 1; // shift to left and use as offset
        y = a;
        x = M(VRAM_Buffer1_Offset); // get current buffer pointer
        a = 0x20; // put at top of screen by default
        if (y == 0x00)
        {
            a = 0x22; // if so, put further down on the screen
        } // SetupNums
        writeData(VRAM_Buffer1 + x, a);
        a = M(StatusBarData + y); // write low vram address and length of thing
        writeData(VRAM_Buffer1 + 1 + x, a); // we're printing to the buffer
        a = M(StatusBarData + 1 + y);
        writeData(VRAM_Buffer1 + 2 + x, a);
        writeData(0x03, a); // save length byte in counter
        writeData(0x02, x); // and buffer pointer elsewhere for now
        pla(); // pull original incremented value from stack
        x = a;
        a = M(StatusBarOffset + x); // load offset to value we want to write
        a -= M(StatusBarData + 1 + y); // subtract from length byte we read before
        y = a; // use value as offset to display digits
        x = M(0x02);

        do // DigitPLoop: write digits to the buffer
        {
            a = M(DisplayDigits + y);
            writeData(VRAM_Buffer1 + 3 + x, a);
            ++x;
            ++y;
            --M(0x03); // do this until all the digits are written
        } while (M(0x03) != 0);
        a = 0x00; // put null terminator at end
        writeData(VRAM_Buffer1 + 3 + x, a);
        ++x; // increment buffer pointer by 3
        ++x;
        ++x;
        writeData(VRAM_Buffer1_Offset, x); // store it in case we want to use it again
    } // ExitOutputN
    goto Return;

//------------------------------------------------------------------------

DigitsMathRoutine:
    a = M(OperMode); // check mode of operation
    if (a != TitleScreenModeValue)
    { // if in title screen mode, branch to lock score
        x = 0x05;

        do // AddModLoop: load digit amount to increment
        {
            a = M(DigitModifier + x);
            a += M(DisplayDigits + y); // add to current digit
            if (a >= 0x80)
                goto BorrowOne; // if result is a negative number, branch to subtract
            if (a >= 10)
                goto CarryOne; // if digit greater than $09, branch to add

StoreNewD: // store as new score or game timer digit
            writeData(DisplayDigits + y, a);
            --y; // move onto next digits in score or game timer
            --x; // and digit amounts to increment
        } while ((x & 0x80) == 0); // loop back if we're not done yet
    } // EraseDMods: store zero here
    a = 0x00;
    x = 0x06; // start with the last digit

    do // EraseMLoop: initialize the digit amounts to increment
    {
        writeData(DigitModifier - 1 + x, a);
        --x;
    } while ((x & 0x80) == 0); // do this until they're all reset, then leave
    goto Return;

//------------------------------------------------------------------------

BorrowOne: // decrement the previous digit, then put $09 in
    --M(DigitModifier - 1 + x);
    a = 0x09; // the game timer digit we're currently on to "borrow
    if (a != 0)
        goto StoreNewD; // the one", then do an unconditional branch back

CarryOne: // subtract ten from our digit to make it a
    a -= 10; // proper BCD number, then increment the digit
    ++M(DigitModifier - 1 + x); // preceding current digit to "carry the one" properly
    goto StoreNewD; // go back to just after we branched here

UpdateTopScore:
    x = 0x05; // start with mario's score
    JSR(TopScoreCheck, 50);
    x = 0x0b; // now do luigi's score

TopScoreCheck:
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
    goto Return;

//------------------------------------------------------------------------

InitializeGame:
    y = 0x6f; // clear all memory as in initialization procedure,
    JSR(InitializeMemory, 51); // but this time, clear only as far as $076f
    y = 0x1f;

    do // ClrSndLoop: clear out memory used
    {
        writeData(SoundMemory + y, a);
        --y; // by the sound engines
    } while ((y & 0x80) == 0);
    a = 0x18; // set demo timer
    writeData(DemoTimer, a);
    JSR(LoadAreaPointer, 52);

InitializeArea:
    y = 0x4b; // clear all memory again, only as far as $074b
    JSR(InitializeMemory, 53); // this is only necessary if branching from
    x = 0x21;
    a = 0x00;

    do // ClrTimersLoop: clear out memory between
    {
        writeData(Timers + x, a);
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
    JSR(GetScreenPosition, 54); // get pixel coordinates for screen borders
    y = 0x20; // if on odd numbered page, use $2480 as start of rendering
    a &= 0b00000001; // otherwise use $2080, this address used later as name table
    if (a != 0)
    { // address for rendering of game area
        y = 0x24;
    } // SetInitNTHigh: store name table address
    writeData(CurrentNTAddr_High, y);
    y = 0x80;
    writeData(CurrentNTAddr_Low, y);
    a <<= 1; // store LSB of page number in high nybble
    a <<= 1; // of block buffer column position
    a <<= 1;
    a <<= 1;
    writeData(BlockBufferColumnPos, a);
    --M(AreaObjectLength); // set area object lengths for all empty
    --M(AreaObjectLength + 1);
    --M(AreaObjectLength + 2);
    a = 0x0b; // set value for renderer to update 12 column sets
    writeData(ColumnSets, a); // 12 column sets = 24 metatile columns = 1 1/2 screens
    JSR(GetAreaDataAddrs, 55); // get enemy and level addresses and load header
    a = M(PrimaryHardMode); // check to see if primary hard mode has been activated
    if (a != 0)
        goto SetSecHard; // if so, activate the secondary no matter where we're at
    a = M(WorldNumber); // otherwise check world number
    if (a < World5)
        goto CheckHalfway;
    if (a != World5)
        goto SetSecHard; // if not equal to, then world > 5, thus activate
    a = M(LevelNumber); // otherwise, world 5, so check level number
    if (a < Level3)
        goto CheckHalfway;

SetSecHard: // set secondary hard mode flag for areas 5-3 and beyond
    ++M(SecondaryHardMode);

CheckHalfway:
    a = M(HalfwayPage);
    if (a != 0)
    {
        a = 0x02; // if halfway page set, overwrite start position from header
        writeData(PlayerEntranceCtrl, a);
    } // DoneInitArea: silence music
    a = Silence;
    writeData(AreaMusicQueue, a);
    a = 0x01; // disable screen output
    writeData(DisableScreenFlag, a);
    ++M(OperMode_Task); // increment one of the modes
    goto Return;

//------------------------------------------------------------------------

PrimaryGameSetup:
    a = 0x01;
    writeData(FetchNewGameTimerFlag, a); // set flag to load game timer from header
    writeData(PlayerSize, a); // set player's size to small
    a = 0x02;
    writeData(NumberofLives, a); // give each player three lives
    writeData(OffScr_NumberofLives, a);

SecondaryGameSetup:
    a = 0x00;
    writeData(DisableScreenFlag, a); // enable screen output
    y = a;

    do // ClearVRLoop: clear buffer at $0300-$03ff
    {
        writeData(VRAM_Buffer1 - 1 + y, a);
        ++y;
    } while (y != 0);
    writeData(GameTimerExpiredFlag, a); // clear game timer exp flag
    writeData(DisableIntermediate, a); // clear skip lives display flag
    writeData(BackloadingFlag, a); // clear value here
    a = 0xff;
    writeData(BalPlatformAlignment, a); // initialize balance platform assignment flag
    // put the LSB of the left side page location into d0 of the ppu register #1
    // mirror, to set the proper PPU name table
    a = (uint8_t)((M(Mirror_PPU_CTRL_REG1) & 0xfe) | (M(ScreenLeft_PageLoc) & 0x01));
    writeData(Mirror_PPU_CTRL_REG1, a);
    JSR(GetAreaMusic, 56); // load proper music into queue
    a = 0x38; // load sprite shuffle amounts to be used later
    writeData(SprShuffleAmt + 2, a);
    a = 0x48;
    writeData(SprShuffleAmt + 1, a);
    a = 0x58;
    writeData(SprShuffleAmt, a);
    x = 0x0e; // load default OAM offsets into $06e4-$06f2

    do // ShufAmtLoop
    {
        a = M(DefaultSprOffsets + x);
        writeData(SprDataOffset + x, a);
        --x; // do this until they're all set
    } while ((x & 0x80) == 0);
    y = 0x03; // set up sprite #0

    do // ISpr0Loop
    {
        a = M(Sprite0Data + y);
        writeData(Sprite_Data + y, a);
        --y;
    } while ((y & 0x80) == 0);
    JSR(DoNothing2, 57); // these jsrs doesn't do anything useful
    JSR(DoNothing1, 58);
    ++M(Sprite0HitDetectFlag); // set sprite #0 check flag
    ++M(OperMode_Task); // increment to next task
    goto Return;

//------------------------------------------------------------------------

InitializeMemory:
    x = 0x07; // set initial high byte to $0700-$07ff
    a = 0x00; // set initial low byte to start of page (at $00 of page)
    writeData(0x06, a);

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
    goto Return;

//------------------------------------------------------------------------

GetAreaMusic:
    a = M(OperMode); // if in title screen mode, leave
    if (a != 0)
    {
        a = M(AltEntranceControl); // check for specific alternate mode of entry
        if (a != 0x02)
        { // from area object data header
            y = 0x05; // select music for pipe intro scene by default
            a = M(PlayerEntranceCtrl); // check value from level header for certain values
            if (a == 0x06)
                goto StoreMusic; // load music for pipe intro scene if header
            if (a == 0x07)
                goto StoreMusic;
        } // ChkAreaType: load area type as offset for music bit
        y = M(AreaType);
        a = M(CloudTypeOverride);
        if (a == 0)
            goto StoreMusic; // check for cloud type override
        y = 0x04; // select music for cloud type level if found

StoreMusic: // otherwise select appropriate music for level type
        a = M(MusicSelectData + y);
        writeData(AreaMusicQueue, a); // store in queue and leave
    } // ExitGetM
    goto Return;

//------------------------------------------------------------------------

Entrance_GameTimerSetup:
    a = M(ScreenLeft_PageLoc); // set current page for area objects
    writeData(Player_PageLoc, a); // as page location for player
    a = 0x28; // store value here
    writeData(VerticalForceDown, a); // for fractional movement downwards if necessary
    a = 0x01; // set high byte of player position and
    writeData(PlayerFacingDir, a); // set facing direction so that player faces right
    writeData(Player_Y_HighPos, a);
    a = 0x00; // set player state to on the ground by default
    writeData(Player_State, a);
    --M(Player_CollisionBits); // initialize player's collision bits
    y = 0x00; // initialize halfway page
    writeData(HalfwayPage, y);
    a = M(AreaType); // check area type
    if (a == 0)
    { // if water type, set swimming flag, otherwise do not set
        ++y;
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
    a = M(PlayerStarting_X_Pos + y);
    writeData(Player_X_Position, a); // and vertical positions for the player, using
    a = M(PlayerStarting_Y_Pos + x); // AltEntranceControl as offset for horizontal and either $0710
    writeData(Player_Y_Position, a); // or value that overwrote $0710 as offset for vertical
    a = M(PlayerBGPriorityData + x);
    writeData(Player_SprAttrib, a); // set player sprite attributes using offset in X
    JSR(GetPlayerColors, 59); // get appropriate player palette
    y = M(GameTimerSetting); // get timer control value from header
    if (y == 0)
        goto ChkOverR; // if set to zero, branch (do not use dummy byte for this)
    a = M(FetchNewGameTimerFlag); // do we need to set the game timer? if not, use
    if (a == 0)
        goto ChkOverR; // old game timer setting
    a = M(GameTimerData + y); // if game timer is set and game timer flag is also set,
    writeData(GameTimerDisplay, a); // use value of game timer control for first digit of game timer
    a = 0x01;
    writeData(GameTimerDisplay + 2, a); // set last digit of game timer to 1
    a >>= 1;
    writeData(GameTimerDisplay + 1, a); // set second digit of game timer
    writeData(FetchNewGameTimerFlag, a); // clear flag for game timer reset
    writeData(StarInvincibleTimer, a); // clear star mario timer

ChkOverR: // if controller bits not set, branch to skip this part
    y = M(JoypadOverride);
    if (y != 0)
    {
        a = 0x03; // set player state to climbing
        writeData(Player_State, a);
        x = 0x00; // set offset for first slot, for block object
        JSR(InitBlock_XY_Pos, 60);
        a = 0xf0; // set vertical coordinate for block object
        writeData(Block_Y_Position, a);
        x = 0x05; // set offset in X for last enemy object buffer slot
        y = 0x00; // set offset in Y for object coordinates used earlier
        JSR(Setup_Vine, 61); // do a sub to grow vine
    } // ChkSwimE: if level not water-type,
    y = M(AreaType);
    if (y == 0)
    { // skip this subroutine
        writeData(0x07, 145); // LYNN HACK: simulate reading stray $07 value from JumpEngine,
                              // read by SetupBubble
        JSR(SetupBubble, 62); // otherwise, execute sub to set up air bubbles
    } // SetPESub: set to run player entrance subroutine
    a = 0x07;
    writeData(GameEngineSubroutine, a); // on the next frame of game engine
    goto Return;

//------------------------------------------------------------------------

PlayerLoseLife:
    ++M(DisableScreenFlag); // disable screen and sprite 0 check
    a = 0x00;
    writeData(Sprite0HitDetectFlag, a);
    a = Silence; // silence music
    writeData(EventMusicQueue, a);
    --M(NumberofLives); // take one life from player
    if ((M(NumberofLives) & 0x80) != 0)
    { // if player still has lives, branch
        a = 0x00;
        writeData(OperMode_Task, a); // initialize mode task,
        a = GameOverModeValue; // switch to game over mode
        writeData(OperMode, a); // and leave
        goto Return;

    //------------------------------------------------------------------------
    } // StillInGame: multiply world number by 2 and use
    a = M(WorldNumber);
    a <<= 1; // as offset
    x = a;
    a = M(LevelNumber); // if in area -3 or -4, increment
    a &= 0x02; // offset by one byte, otherwise
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
    JSR(TransposePlayers, 63); // switch players around if 2-player game
    goto ContinueGame; // continue the game

GameOverMode:
    a = M(OperMode_Task);
    switch (a)
    {
    case 0:
        goto SetupGameOver;
    case 1:
        goto ScreenRoutines;
    case 2:
        goto RunGameOver;
    }

SetupGameOver:
    a = 0x00; // reset screen routine task control for title screen, game,
    writeData(ScreenRoutineTask, a); // and game over modes
    writeData(Sprite0HitDetectFlag, a); // disable sprite 0 check
    a = GameOverMusic;
    writeData(EventMusicQueue, a); // put game over music in secondary queue
    ++M(DisableScreenFlag); // disable screen output
    ++M(OperMode_Task); // set secondary mode to 1
    goto Return;

//------------------------------------------------------------------------

RunGameOver:
    a = 0x00; // reenable screen
    writeData(DisableScreenFlag, a);
    a = M(SavedJoypad1Bits); // check controller for start pressed
    a &= Start_Button;
    if (a != 0)
        goto TerminateGame;
    a = M(ScreenTimer); // if not pressed, wait for
    if (a == 0)
    { // screen timer to expire

TerminateGame:
        a = Silence; // silence music
        writeData(EventMusicQueue, a);
        JSR(TransposePlayers, 64); // check if other player can keep
        if (!endGame)
            goto ContinueGame; // going, and do so if possible
        a = M(WorldNumber); // otherwise put world number of current
        writeData(ContinueWorld, a); // player into secret continue function variable
        a = 0x00;
        a <<= 1; // residual ASL instruction
        writeData(OperMode_Task, a); // reset all modes to title screen and
        writeData(ScreenTimer, a); // leave
        writeData(OperMode, a);
        goto Return;

    //------------------------------------------------------------------------

ContinueGame:
        JSR(LoadAreaPointer, 65); // update level pointer with
        a = 0x01; // actual world and area numbers, then
        writeData(PlayerSize, a); // reset player's size, status, and
        ++M(FetchNewGameTimerFlag); // set game timer flag to reload
        a = 0x00; // game timer from header
        writeData(TimerControl, a); // also set flag for timers to count again
        writeData(PlayerStatus, a);
        writeData(GameEngineSubroutine, a); // reset task for game core
        writeData(OperMode_Task, a); // set modes and leave
        a = 0x01; // if in game over mode, switch back to
        writeData(OperMode, a); // game mode, because game is still on
    } // GameIsOn
    goto Return;

//------------------------------------------------------------------------

TransposePlayers:
    endGame = true; // end the game by default
    a = M(NumberOfPlayers); // if only a 1 player game, leave
    if (a == 0)
        goto ExTrans;
    a = M(OffScr_NumberofLives); // does offscreen player have any lives left?
    if ((a & 0x80) != 0)
        goto ExTrans; // branch if not
    a = M(CurrentPlayer); // invert bit to update
    a ^= 0b00000001; // which player is on the screen
    writeData(CurrentPlayer, a);
    x = 0x06;

    do // TransLoop: transpose the information
    {
        a = M(OnscreenPlayerInfo + x);
        pha(); // of the onscreen player
        a = M(OffscreenPlayerInfo + x); // with that of the offscreen player
        writeData(OnscreenPlayerInfo + x, a);
        pla();
        writeData(OffscreenPlayerInfo + x, a);
        --x;
    } while ((x & 0x80) == 0);
    endGame = false; // get the game going

ExTrans:
    goto Return;

//------------------------------------------------------------------------

DoNothing1:
    a = 0xff; // this is residual code, this value is
    writeData(0x06c9, a); // not used anywhere in the program

DoNothing2:
    goto Return;

//------------------------------------------------------------------------

AreaParserTaskHandler:
    y = M(AreaParserTaskNum); // check number of tasks here
    if (y == 0)
    { // if already set, go ahead
        y = 0x08;
        writeData(AreaParserTaskNum, y); // otherwise, set eight by default
    } // DoAPTasks
    --y;
    a = y;
    JSR(AreaParserTasks, 66);
    --M(AreaParserTaskNum); // if all tasks not complete do not
    if (M(AreaParserTaskNum) == 0)
    { // render attribute table yet
        JSR(RenderAttributeTables, 67);
    } // SkipATRender
    goto Return;

//------------------------------------------------------------------------

AreaParserTasks:
    switch (a)
    {
    case 0:
        goto IncrementColumnPos;
    case 1:
        goto RenderAreaGraphics;
    case 2:
        goto RenderAreaGraphics;
    case 3:
        goto AreaParserCore;
    case 4:
        goto IncrementColumnPos;
    case 5:
        goto RenderAreaGraphics;
    case 6:
        goto RenderAreaGraphics;
    case 7:
        goto AreaParserCore;
    }

IncrementColumnPos:
    ++M(CurrentColumnPos); // increment column where we're at
    a = M(CurrentColumnPos);
    a &= 0b00001111; // mask out higher nybble
    if (a == 0)
    {
        writeData(CurrentColumnPos, a); // if no bits left set, wrap back to zero (0-f)
        ++M(CurrentPageLoc); // and increment page number where we're at
    } // NoColWrap: increment column offset where we're at
    ++M(BlockBufferColumnPos);
    a = M(BlockBufferColumnPos);
    a &= 0b00011111; // mask out all but 5 LSB (0-1f)
    writeData(BlockBufferColumnPos, a); // and save
    goto Return;

//------------------------------------------------------------------------

AreaParserCore:
    a = M(BackloadingFlag); // check to see if we are starting right of start
    if (a != 0)
    { // if not, go ahead and render background, foreground and terrain
        JSR(ProcessAreaData, 68); // otherwise skip ahead and load level data
    } // RenderSceneryTerrain
    x = 0x0c;
    a = 0x00;

    do // ClrMTBuf: clear out metatile buffer
    {
        writeData(MetatileBuffer + x, a);
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
    writeData(0x00, a);

    do // SceLoop1: load metatile data from offset of (lsb - 1) * 3
    {
        a = M(BackSceneryMetatiles + x);
        writeData(MetatileBuffer + y, a); // store into buffer from offset of (msb / 16)
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
    a = M(WorldNumber); // check world number, if not world number eight
    if (a != World8)
        goto TerMTile;
    a = 0x62; // if set as water level and world number eight,
    goto StoreMT; // use castle wall metatile as terrain type

TerMTile: // otherwise get appropriate metatile for area type
    a = M(TerrainMetatiles + y);
    y = M(CloudTypeOverride); // check for cloud type override
    if (y == 0)
        goto StoreMT; // if not set, keep value otherwise
    a = 0x88; // use cloud block terrain

StoreMT: // store value here
    writeData(0x07, a);
    x = 0x00; // initialize X, use as metatile buffer offset
    a = M(TerrainControl); // use yet another value from the header
    a <<= 1; // multiply by 2 and use as yet another offset
    y = a;

TerrLoop: // get one of the terrain rendering bit data
    a = M(TerrainRenderBits + y);
    writeData(0x00, a);
    ++y; // increment Y and use as offset next time around
    writeData(0x01, y);
    a = M(CloudTypeOverride); // skip if value here is zero
    if (a == 0)
        goto NoCloud2;
    if (x == 0x00)
        goto NoCloud2;
    a = M(0x00); // if not, mask out all but d3
    a &= 0b00001000;
    writeData(0x00, a);

NoCloud2: // start at beginning of bitmasks
    y = 0x00;

TerrBChk: // load bitmask, then perform AND on contents of first byte
    a = M(Bitmasks + y);
    if ((a & M(0x00)) != 0)
    { // if not set, skip this part (do not write terrain to buffer)
        a = M(0x07);
        writeData(MetatileBuffer + x, a); // load terrain type metatile number and store into buffer here
    } // NextTBit: continue until end of buffer
    ++x;
    if (x != 0x0d)
    { // if we're at the end, break out of this loop
        a = M(AreaType); // check world type for underground area
        if (a != 0x02)
            goto EndUChk; // if not underground, skip this part
        if (x != 0x0b)
            goto EndUChk; // if we're at the bottom of the screen, override
        a = 0x54; // old terrain type with ground level terrain type
        writeData(0x07, a);

EndUChk: // increment bitmasks offset in Y
        ++y;
        if (y != 0x08)
            goto TerrBChk; // if not all bits checked, loop back
        y = M(0x01);
        if (y != 0)
            goto TerrLoop; // unconditional branch, use Y to load next byte
    } // RendBBuf: do the area data loading routine now
    JSR(ProcessAreaData, 69);
    a = M(BlockBufferColumnPos);
    JSR(GetBlockBufferAddr, 70); // get block buffer address from where we're at
    x = 0x00;
    y = 0x00; // init index regs and start at beginning of smaller buffer

    do // ChkMTLow
    {
        writeData(0x00, y);
        a = M(MetatileBuffer + x); // load stored metatile number
        a &= 0b11000000; // mask out all but 2 MSB
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
    goto Return;

//------------------------------------------------------------------------

ProcessAreaData:
    x = 0x02; // start at the end of area object buffer

    do // ProcADLoop
    {
        writeData(ObjectOffset, x);
        a = 0x00; // reset flag
        writeData(BehindAreaParserFlag, a);
        y = M(AreaDataOffset); // get offset of area data pointer
        a = M(W(AreaData) + y); // get first byte of area object
        if (a == 0xfd)
            goto RdyDecode;
        a = M(AreaObjectLength + x); // check area object buffer flag
        if ((a & 0x80) == 0)
            goto RdyDecode; // if buffer not negative, branch, otherwise
        ++y;
        a = M(W(AreaData) + y); // get second byte of area object
        a <<= 1; // check for page select bit (d7), branch if not set
        if ((M(W(AreaData) + y) & 0x80) == 0)
            goto Chk1Row13;
        a = M(AreaObjectPageSel); // check page select
        if (a != 0)
            goto Chk1Row13;
        ++M(AreaObjectPageSel); // if not already set, set it now
        ++M(AreaObjectPageLoc); // and increment page location

Chk1Row13:
        --y;
        a = M(W(AreaData) + y); // reread first byte of level object
        a &= 0x0f; // mask out high nybble
        if (a == 0x0d)
        {
            ++y; // if so, reread second byte of level object
            a = M(W(AreaData) + y);
            --y; // decrement to get ready to read first byte
            a &= 0b01000000; // check for d6 set (if not, object is page control)
            if (a != 0)
                goto CheckRear;
            a = M(AreaObjectPageSel); // if page select is set, do not reread
            if (a != 0)
                goto CheckRear;
            ++y; // if d6 not set, reread second byte
            a = M(W(AreaData) + y);
            a &= 0b00011111; // mask out all but 5 LSB and store in page control
            writeData(AreaObjectPageLoc, a);
            ++M(AreaObjectPageSel); // increment page select
        } // Chk1Row14: row 14?
        else
        {
            if (a != 0x0e)
                goto CheckRear;
            a = M(BackloadingFlag); // check flag for saved page number and branch if set
            if (a != 0)
                goto RdyDecode; // to render the object (otherwise bg might not look right)

CheckRear: // check to see if current page of level object is
            a = M(AreaObjectPageLoc);
            if (a >= M(CurrentPageLoc))
            { // if so branch

RdyDecode: // do sub and do not turn on flag
                JSR(DecodeAreaData, 71);
                goto ChkLength;
            } // SetBehind: turn on flag if object is behind renderer
            ++M(BehindAreaParserFlag);
        } // NextAObj: increment buffer offset and move on
        JSR(IncAreaObjOffset, 72);

ChkLength: // get buffer offset
        x = M(ObjectOffset);
        a = M(AreaObjectLength + x); // check object length for anything stored here
        if ((a & 0x80) == 0)
        { // if not, branch to handle loopback
            --M(AreaObjectLength + x); // otherwise decrement length or get rid of it
        } // ProcLoopb: decrement buffer offset
        --x;
    } while ((x & 0x80) == 0); // and loopback unless exceeded buffer
    a = M(BehindAreaParserFlag); // check for flag set if objects were behind renderer
    if (a != 0)
        goto ProcessAreaData; // branch if true to load more level data, otherwise
    a = M(BackloadingFlag); // check for flag set if starting right of page $00
    if (a != 0)
        goto ProcessAreaData; // branch if true to load more level data, otherwise leave

    do // EndAParse
    {
        goto Return;

    //------------------------------------------------------------------------

IncAreaObjOffset:
        ++M(AreaDataOffset); // increment offset of level pointer
        ++M(AreaDataOffset);
        a = 0x00; // reset page select
        writeData(AreaObjectPageSel, a);
        goto Return;

    //------------------------------------------------------------------------

DecodeAreaData:
        a = M(AreaObjectLength + x); // check current buffer flag
        if ((a & 0x80) == 0)
        {
            y = M(AreaObjOffsetBuffer + x); // if not, get offset from buffer
        } // Chk1stB: load offset of 16 for special row 15
        x = 0x10;
        a = M(W(AreaData) + y); // get first byte of level object again
    } while (a == 0xfd); // if end of level, leave this routine
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
        a = 0x00; // if so, load offset with $00
        writeData(0x07, a);
        a = 0x2e; // and load A with another value
        if (a != 0)
            goto NormObj; // unconditional branch
    } // ChkRow13: row 13?
    if (a == 0x0d)
    {
        a = 0x22; // if so, load offset with 34
        writeData(0x07, a);
        ++y; // get next byte
        a = M(W(AreaData) + y);
        a &= 0b01000000; // mask out all but d6 (page control obj bit)
        if (a == 0)
            goto LeavePar; // if d6 clear, branch to leave (we handled this earlier)
        a = M(W(AreaData) + y); // otherwise, get byte again
        a &= 0b01111111; // mask out d7
        if (a == 0x4b)
        { // (plus d6 set for object other than page control)
            ++M(LoopCommand); // if loop command, set loop command flag
        } // Mask2MSB: mask out d7 and d6
        a &= 0b00111111;
        goto NormObj; // and jump
    } // ChkSRows: row 12-15?
    if (a < 0x0c)
    {
        ++y; // if not, get second byte of level object
        a = M(W(AreaData) + y);
        a &= 0b01110000; // mask out all but d6-d4
        if (a == 0)
        { // if any bits set, branch to handle large object
            a = 0x16;
            writeData(0x07, a); // otherwise set offset of 24 for small object
            a = M(W(AreaData) + y); // reload second byte of level object
            a &= 0b00001111; // mask out higher nybble and jump
            goto NormObj;
        } // LrgObj: store value here (branch for large objects)
        writeData(0x00, a);
        if (a != 0x70)
            goto NotWPipe;
        a = M(W(AreaData) + y); // if not, reload second byte
        a &= 0b00001000; // mask out all but d3 (usage control bit)
        if (a == 0)
            goto NotWPipe; // if d3 clear, branch to get original value
        a = 0x00; // otherwise, nullify value for warp pipe
        writeData(0x00, a);

NotWPipe: // get value and jump ahead
        a = M(0x00);
    } // SpecObj: branch here for rows 12-15
    else
    {
        ++y;
        a = M(W(AreaData) + y);
        a &= 0b01110000; // get next byte and mask out all but d6-d4
    } // MoveAOId: move d6-d4 to lower nybble
    a >>= 1;
    a >>= 1;
    a >>= 1;
    a >>= 1;

NormObj: // store value here (branch for small objects and rows 13 and 14)
    writeData(0x00, a);
    a = M(AreaObjectLength + x); // is there something stored here already?
    if ((a & 0x80) != 0)
    { // if so, branch to do its particular sub
        a = M(AreaObjectPageLoc); // otherwise check to see if the object we've loaded is on the
        if (a != M(CurrentPageLoc))
        {
            y = M(AreaDataOffset); // if not, get old offset of level pointer
            a = M(W(AreaData) + y); // and reload first byte
            a &= 0b00001111;
            if (a != 0x0e)
                goto LeavePar;
            a = M(BackloadingFlag); // if so, check backloading flag
            if (a != 0)
                goto StrAObj; // if set, branch to render object, else leave

LeavePar:
            goto Return;

        //------------------------------------------------------------------------
        } // InitRear: check backloading flag to see if it's been initialized
        a = M(BackloadingFlag);
        if (a != 0)
        { // branch to column-wise check
            a = 0x00; // if not, initialize both backloading and
            writeData(BackloadingFlag, a); // behind-renderer flags and leave
            writeData(BehindAreaParserFlag, a);
            writeData(ObjectOffset, a);

LoopCmdE:
            goto Return;

        //------------------------------------------------------------------------
        } // BackColC: get first byte again
        y = M(AreaDataOffset);
        a = M(W(AreaData) + y);
        a &= 0b11110000; // mask out low nybble and move high to low
        a >>= 1;
        a >>= 1;
        a >>= 1;
        a >>= 1;
        if (a != M(CurrentColumnPos))
            goto LeavePar; // if not, branch to leave

StrAObj: // if so, load area obj offset and store in buffer
        a = M(AreaDataOffset);
        writeData(AreaObjOffsetBuffer + x, a);
        JSR(IncAreaObjOffset, 73); // do sub to increment to next object data
    } // RunAObj: get stored value and add offset to it
    a = M(0x00);
    a += M(0x07);
    switch (a)
    {
    case 0:
        goto VerticalPipe; // used by warp pipes
    case 1:
        goto AreaStyleObject;
    case 2:
        goto RowOfBricks;
    case 3:
        goto RowOfSolidBlocks;
    case 4:
        goto RowOfCoins;
    case 5:
        goto ColumnOfBricks;
    case 6:
        goto ColumnOfSolidBlocks;
    case 7:
        goto VerticalPipe; // used by decoration pipes
    case 8:
        goto Hole_Empty;
    case 9:
        goto PulleyRopeObject;
    case 10:
        goto Bridge_High;
    case 11:
        goto Bridge_Middle;
    case 12:
        goto Bridge_Low;
    case 13:
        goto Hole_Water;
    case 14:
        goto QuestionBlockRow_High;
    case 15:
        goto QuestionBlockRow_Low;
    case 16:
        goto EndlessRope;
    case 17:
        goto BalancePlatRope;
    case 18:
        goto CastleObject;
    case 19:
        goto StaircaseObject;
    case 20:
        goto ExitPipe;
    case 21:
        goto FlagBalls_Residual;
    case 22:
        goto QuestionBlock; // power-up
    case 23:
        goto QuestionBlock; // coin
    case 24:
        goto QuestionBlock; // hidden, coin
    case 25:
        goto Hidden1UpBlock; // hidden, 1-up
    case 26:
        goto BrickWithItem; // brick, power-up
    case 27:
        goto BrickWithItem; // brick, vine
    case 28:
        goto BrickWithItem; // brick, star
    case 29:
        goto BrickWithCoins; // brick, coins
    case 30:
        goto BrickWithItem; // brick, 1-up
    case 31:
        goto WaterPipe;
    case 32:
        goto EmptyBlock;
    case 33:
        goto Jumpspring;
    case 34:
        goto IntroPipe;
    case 35:
        goto FlagpoleObject;
    case 36:
        goto AxeObj;
    case 37:
        goto ChainObj;
    case 38:
        goto CastleBridgeObj;
    case 39:
        goto ScrollLockObject_Warp;
    case 40:
        goto ScrollLockObject;
    case 41:
        goto ScrollLockObject;
    case 42:
        goto AreaFrenzy; // flying cheep-cheeps
    case 43:
        goto AreaFrenzy; // bullet bills or swimming cheep-cheeps
    case 44:
        goto AreaFrenzy; // stop frenzy
    case 45:
        goto LoopCmdE;
    case 46:
        goto AlterAreaAttributes;
    }

AlterAreaAttributes:
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
        goto Return;

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
    goto Return;

//------------------------------------------------------------------------

ScrollLockObject_Warp:
    x = 0x04; // load value of 4 for game text routine as default
    a = M(WorldNumber); // warp zone (4-3-2), then check world number
    if (a == 0)
        goto WarpNum;
    ++x; // if world number > 1, increment for next warp zone (5)
    y = M(AreaType); // check area type
    --y;
    if (y != 0)
        goto WarpNum; // if ground area type, increment for last warp zone
    ++x; // (8-7-6) and move on

WarpNum:
    a = x;
    writeData(WarpZoneControl, a); // store number here to be used by warp zone routine
    JSR(WriteGameText, 74); // print text and warp zone numbers
    a = PiranhaPlant;
    JSR(KillEnemies, 75); // load identifier for piranha plants and do sub

ScrollLockObject:
    a = M(ScrollLock); // invert scroll lock to turn it on
    a ^= 0b00000001;
    writeData(ScrollLock, a);
    goto Return;

//------------------------------------------------------------------------

KillEnemies:
    writeData(0x00, a); // store identifier here
    a = 0x00;
    x = 0x04; // check for identifier in enemy object buffer

    do // KillELoop
    {
        y = M(Enemy_ID + x);
        if (y == M(0x00))
        {
            writeData(Enemy_Flag + x, a); // if found, deactivate enemy object flag
        } // NoKillE: do this until all slots are checked
        --x;
    } while ((x & 0x80) == 0);
    goto Return;

//------------------------------------------------------------------------

AreaFrenzy: // use area object identifier bit as offset
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
    goto Return;

//------------------------------------------------------------------------

AreaStyleObject:
    a = M(AreaStyle); // load level object style and jump to the right sub
    switch (a)
    {
    case 0:
        goto TreeLedge; // also used for cloud type levels
    case 1:
        goto MushroomLedge;
    case 2:
        goto BulletBillCannon;
    }

TreeLedge:
    JSR(GetLrgObjAttrib, 76); // get row and length of green ledge
    a = M(AreaObjectLength + x); // check length counter for expiration
    if (a != 0)
    {
        if ((a & 0x80) == 0)
            goto MidTreeL;
        a = y;
        writeData(AreaObjectLength + x, a); // store lower nybble into buffer flag as length of ledge
        a = M(CurrentPageLoc);
        a |= M(CurrentColumnPos); // are we at the start of the level?
        if (a == 0)
            goto MidTreeL;
        a = 0x16; // render start of tree ledge
        goto NoUnder;

MidTreeL:
        x = M(0x07);
        a = 0x17; // render middle of tree ledge
        writeData(MetatileBuffer + x, a); // note that this is also used if ledge position is
        a = 0x4c; // at the start of level for continuous effect
        goto AllUnder; // now render the part underneath
    } // EndTreeL: render end of tree ledge
    a = 0x18;
    goto NoUnder;

MushroomLedge:
    JSR(ChkLrgObjLength, 77); // get shroom dimensions
    writeData(0x06, y); // store length here for now
    if (lrgObjJustStarted)
    {
        a = M(AreaObjectLength + x); // divide length by 2 and store elsewhere
        a >>= 1;
        writeData(MushroomLedgeHalfLen + x, a);
        a = 0x19; // render start of mushroom
        goto NoUnder;
    } // EndMushL: if at the end, render end of mushroom
    a = 0x1b;
    y = M(AreaObjectLength + x);
    if (y == 0)
        goto NoUnder;
    a = M(MushroomLedgeHalfLen + x); // get divided length and store where length
    writeData(0x06, a); // was stored originally
    x = M(0x07);
    a = 0x1a;
    writeData(MetatileBuffer + x, a); // render middle of mushroom
    if (y == M(0x06))
    { // if not, branch to leave
        ++x;
        a = 0x4f;
        writeData(MetatileBuffer + x, a); // render stem top of mushroom underneath the middle
        a = 0x50;

AllUnder:
        ++x;
        y = 0x0f; // set $0f to render all way down
        goto RenderUnderPart; // now render the stem of mushroom

NoUnder: // load row of ledge
        x = M(0x07);
        y = 0x00; // set 0 for no bottom on this part
        goto RenderUnderPart;

PulleyRopeObject:
        JSR(ChkLrgObjLength, 78); // get length of pulley/rope object
        y = 0x00; // initialize metatile offset
        if (lrgObjJustStarted)
            goto RenderPul; // if starting, render left pulley
        ++y;
        a = M(AreaObjectLength + x); // if not at the end, render rope
        if (a != 0)
            goto RenderPul;
        ++y; // otherwise render right pulley

RenderPul:
        a = M(PulleyRopeMetatiles + y);
        writeData(MetatileBuffer, a); // render at the top of the screen
    } // MushLExit: and leave
    goto Return;

//------------------------------------------------------------------------

CastleObject:
    JSR(GetLrgObjAttrib, 79); // save lower nybble as starting row
    writeData(0x07, y); // if starting row is above $0a, game will crash!!!
    y = 0x04;
    JSR(ChkLrgObjFixedLength, 80); // load length of castle if not already loaded
    a = x;
    pha(); // save obj buffer offset to stack
    y = M(AreaObjectLength + x); // use current length as offset for castle data
    x = M(0x07); // begin at starting row
    a = 0x0b;
    writeData(0x06, a); // load upper limit of number of rows to print

    do // CRendLoop: load current byte using offset
    {
        a = M(CastleMetatiles + y);
        writeData(MetatileBuffer + x, a);
        ++x; // store in buffer and increment buffer offset
        a = M(0x06);
        if (a != 0)
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
        goto ExitCastle; // if we're at page 0, we do not need to do anything else
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
        goto ExitCastle; // if we aren't and the castle is tall, don't create flag yet
    JSR(GetAreaObjXPosition, 81); // otherwise, obtain and save horizontal pixel coordinate
    pha();
    JSR(FindEmptyEnemySlot, 82); // find an empty place on the enemy object buffer
    pla();
    writeData(Enemy_X_Position + x, a); // then write horizontal coordinate for star flag
    a = M(CurrentPageLoc);
    writeData(Enemy_PageLoc + x, a); // set page location for star flag
    a = 0x01;
    writeData(Enemy_Y_HighPos + x, a); // set vertical high byte
    writeData(Enemy_Flag + x, a); // set flag for buffer
    a = 0x90;
    writeData(Enemy_Y_Position + x, a); // set vertical coordinate
    a = StarFlagObject; // set star flag value in buffer itself
    writeData(Enemy_ID + x, a);
    goto Return;

//------------------------------------------------------------------------

PlayerStop: // put brick at floor to stop player at end of level
    y = 0x52;
    writeData(MetatileBuffer + 10, y); // this is only done if we're on the second column

ExitCastle:
    goto Return;

//------------------------------------------------------------------------

WaterPipe:
    JSR(GetLrgObjAttrib, 83); // get row and lower nybble
    y = M(AreaObjectLength + x); // get length (residual code, water pipe is 1 col thick)
    x = M(0x07); // get row
    a = 0x6b;
    writeData(MetatileBuffer + x, a); // draw something here and below it
    a = 0x6c;
    writeData(MetatileBuffer + 1 + x, a);
    goto Return;

//------------------------------------------------------------------------

IntroPipe:
    y = 0x03; // check if length set, if not set, set it
    JSR(ChkLrgObjFixedLength, 84);
    y = 0x0a; // set fixed value and render the sideways part
    JSR(RenderSidewaysPipe, 85);
    if (sidePipeShaftDrawn)
    { // if the shaft was not drawn, not time to draw vertical pipe part
        x = 0x06; // blank everything above the vertical pipe part

        do // VPipeSectLoop: all the way to the top of the screen
        {
            a = 0x00;
            writeData(MetatileBuffer + x, a); // because otherwise it will look like exit pipe
            --x;
        } while ((x & 0x80) == 0);
        a = M(VerticalPipeData + y); // draw the end of the vertical pipe part
        writeData(MetatileBuffer + 7, a);
    } // NoBlankP
    goto Return;

//------------------------------------------------------------------------

ExitPipe:
    y = 0x03; // check if length set, if not set, set it
    JSR(ChkLrgObjFixedLength, 86);
    JSR(GetLrgObjAttrib, 87); // get vertical length, then plow on through RenderSidewaysPipe

RenderSidewaysPipe:
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
        JSR(RenderUnderPart, 88); // and render vertical shaft using tile number in A
        sidePipeShaftDrawn = true; // used by IntroPipe
    }
    else
    {
        sidePipeShaftDrawn = false;
    } // DrawSidePart: render side pipe part at the bottom
    y = M(0x06);
    a = M(SidePipeTopPart + y);
    writeData(MetatileBuffer + x, a); // note that the pipe parts are stored
    a = M(SidePipeBottomPart + y); // backwards horizontally
    writeData(MetatileBuffer + 1 + x, a);
    goto Return;

//------------------------------------------------------------------------

VerticalPipe:
    JSR(GetPipeHeight, 89);
    a = M(0x00); // check to see if value was nullified earlier
    if (a != 0)
    { // (if d3, the usage control bit of second byte, was set)
        ++y;
        ++y;
        ++y;
        ++y; // add four if usage control bit was not set
    } // WarpPipe: save value in stack
    a = y;
    pha();
    a = M(AreaNumber);
    a |= M(WorldNumber); // if at world 1-1, do not add piranha plant ever
    if (a == 0)
        goto DrawPipe;
    y = M(AreaObjectLength + x); // if on second column of pipe, branch
    if (y == 0)
        goto DrawPipe; // (because we only need to do this once)
    JSR(FindEmptyEnemySlot, 90); // check for an empty moving data buffer space
    if (allEnemySlotsFull)
        goto DrawPipe; // if not found, too many enemies, thus skip
    JSR(GetAreaObjXPosition, 91); // get horizontal pixel coordinate
    wide = ((M(CurrentPageLoc) << 8) | a) + 0x08; // add eight to put the piranha plant in the center
    writeData(Enemy_X_Position + x, LOBYTE(wide)); // store as enemy's horizontal coordinate
    writeData(Enemy_PageLoc + x, HIBYTE(wide)); // store as enemy's page coordinate
    a = HIBYTE(wide); // add carry to current page number
    a = 0x01;
    writeData(Enemy_Y_HighPos + x, a);
    writeData(Enemy_Flag + x, a); // activate enemy flag
    JSR(GetAreaObjYPosition, 92); // get piranha plant's vertical coordinate and store here
    writeData(Enemy_Y_Position + x, a);
    a = PiranhaPlant; // write piranha plant's value into buffer
    writeData(Enemy_ID + x, a);
    JSR(InitPiranhaPlant, 93);

DrawPipe: // get value saved earlier and use as Y
    pla();
    y = a;
    x = M(0x07); // get buffer offset
    a = M(VerticalPipeData + y); // draw the appropriate pipe with the Y we loaded earlier
    writeData(MetatileBuffer + x, a); // render the top of the pipe
    ++x;
    a = M(VerticalPipeData + 2 + y); // render the rest of the pipe
    y = M(0x06); // subtract one from length and render the part underneath
    --y;
    goto RenderUnderPart;

GetPipeHeight:
    y = 0x01; // check for length loaded, if not, load
    JSR(ChkLrgObjFixedLength, 94); // pipe length of 2 (horizontal)
    JSR(GetLrgObjAttrib, 95);
    a = y; // get saved lower nybble as height
    a &= 0x07; // save only the three lower bits as
    writeData(0x06, a); // vertical length, then load Y with
    y = M(AreaObjectLength + x); // length left over
    goto Return;

//------------------------------------------------------------------------

FindEmptyEnemySlot:
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
    goto Return;

//------------------------------------------------------------------------

Hole_Water:
    JSR(ChkLrgObjLength, 96); // get low nybble and save as length
    a = 0x86; // render waves
    writeData(MetatileBuffer + 10, a);
    x = 0x0b;
    y = 0x01; // now render the water underneath
    a = 0x87;
    goto RenderUnderPart;

QuestionBlockRow_High:
    a = 0x03; // start on the fourth row
    goto Skip_1;

QuestionBlockRow_Low:
    a = 0x07; // start on the eighth row
Skip_1:
    pha(); // save whatever row to the stack for now
    JSR(ChkLrgObjLength, 97); // get low nybble and save as length
    pla();
    x = a; // render question boxes with coins
    a = 0xc0;
    writeData(MetatileBuffer + x, a);
    goto Return;

//------------------------------------------------------------------------

Bridge_High:
    a = 0x06; // start on the seventh row from top of screen
    goto Skip_2;

Bridge_Middle:
    a = 0x07; // start on the eighth row
Skip_2:
    goto Skip_3;

Bridge_Low:
    a = 0x09; // start on the tenth row
Skip_3:
    pha(); // save whatever row to the stack for now
    JSR(ChkLrgObjLength, 98); // get low nybble and save as length
    pla();
    x = a; // render bridge railing
    a = 0x0b;
    writeData(MetatileBuffer + x, a);
    ++x;
    y = 0x00; // now render the bridge itself
    a = 0x63;
    goto RenderUnderPart;

FlagBalls_Residual:
    JSR(GetLrgObjAttrib, 99); // get low nybble from object byte
    x = 0x02; // render flag balls on third row from top
    a = 0x6d; // of screen downwards based on low nybble
    goto RenderUnderPart;

FlagpoleObject:
    a = 0x24; // render flagpole ball on top
    writeData(MetatileBuffer, a);
    x = 0x01; // now render the flagpole shaft
    y = 0x08;
    a = 0x25;
    JSR(RenderUnderPart, 100);
    a = 0x61; // render solid block at the bottom
    writeData(MetatileBuffer + 10, a);
    JSR(GetAreaObjXPosition, 101);
    wide = ((M(CurrentPageLoc) << 8) | a) - 0x08; // subtract eight pixels and use as horizontal
    writeData(Enemy_X_Position + 5, LOBYTE(wide)); // coordinate for the flag
    writeData(Enemy_PageLoc + 5, HIBYTE(wide)); // page location for the flag
    a = HIBYTE(wide);
    a = 0x30;
    writeData(Enemy_Y_Position + 5, a); // set vertical coordinate for flag
    a = 0xb0;
    writeData(FlagpoleFNum_Y_Pos, a); // set initial vertical coordinate for flagpole's floatey number
    a = FlagpoleFlagObject;
    writeData(Enemy_ID + 5, a); // set flag identifier, note that identifier and coordinates
    ++M(Enemy_Flag + 5); // use last space in enemy object buffer
    goto Return;

//------------------------------------------------------------------------

EndlessRope:
    x = 0x00; // render rope from the top to the bottom of screen
    y = 0x0f;
    goto DrawRope;

BalancePlatRope:
    a = x; // save object buffer offset for now
    pha();
    x = 0x01; // blank out all from second row to the bottom
    y = 0x0f; // with blank used for balance platform rope
    a = 0x44;
    JSR(RenderUnderPart, 102);
    pla(); // get back object buffer offset
    x = a;
    JSR(GetLrgObjAttrib, 103); // get vertical length from lower nybble
    x = 0x01;

DrawRope: // render the actual rope
    a = 0x40;
    goto RenderUnderPart;

RowOfCoins:
    y = M(AreaType); // get area type
    a = M(CoinMetatileData + y); // load appropriate coin metatile
    goto GetRow;

CastleBridgeObj:
    y = 0x0c; // load length of 13 columns
    JSR(ChkLrgObjFixedLength, 104);
    goto ChainObj;

AxeObj:
    a = 0x08; // load bowser's palette into sprite portion of palette
    writeData(VRAM_Buffer_AddrCtrl, a);

ChainObj:
    y = M(0x00); // get value loaded earlier from decoder
    x = M(C_ObjectRow - 2 + y); // get appropriate row and metatile for object
    a = M(C_ObjectMetatile - 2 + y);
    goto ColObj;

EmptyBlock:
    JSR(GetLrgObjAttrib, 105); // get row location
    x = M(0x07);
    a = 0xc4;

ColObj: // column length of 1
    y = 0x00;
    goto RenderUnderPart;

RowOfBricks:
    y = M(AreaType); // load area type obtained from area offset pointer
    a = M(CloudTypeOverride); // check for cloud type override
    if (a != 0)
    {
        y = 0x04; // if cloud type, override area type
    } // DrawBricks: get appropriate metatile
    a = M(BrickMetatiles + y);
    goto GetRow; // and go render it

RowOfSolidBlocks:
    y = M(AreaType); // load area type obtained from area offset pointer
    a = M(SolidBlockMetatiles + y); // get metatile

GetRow: // store metatile here
    pha();
    JSR(ChkLrgObjLength, 106); // get row number, load length

DrawRow:
    x = M(0x07);
    y = 0x00; // set vertical height of 1
    pla();
    goto RenderUnderPart; // render object

ColumnOfBricks:
    y = M(AreaType); // load area type obtained from area offset
    a = M(BrickMetatiles + y); // get metatile (no cloud override as for row)
    goto GetRow2;

ColumnOfSolidBlocks:
    y = M(AreaType); // load area type obtained from area offset
    a = M(SolidBlockMetatiles + y); // get metatile

GetRow2: // save metatile to stack for now
    pha();
    JSR(GetLrgObjAttrib, 107); // get length and row
    pla(); // restore metatile
    x = M(0x07); // get starting row
    goto RenderUnderPart; // now render the column

BulletBillCannon:
    JSR(GetLrgObjAttrib, 108); // get row and length of bullet bill cannon
    x = M(0x07); // start at first row
    a = 0x64; // render bullet bill cannon
    writeData(MetatileBuffer + x, a);
    ++x;
    --y; // done yet?
    if ((y & 0x80) != 0)
        goto SetupCannon;
    a = 0x65; // if not, render middle part
    writeData(MetatileBuffer + x, a);
    ++x;
    --y; // done yet?
    if ((y & 0x80) != 0)
        goto SetupCannon;
    a = 0x66; // if not, render bottom until length expires
    JSR(RenderUnderPart, 109);

SetupCannon: // get offset for data used by cannons and whirlpools
    x = M(Cannon_Offset);
    JSR(GetAreaObjYPosition, 110); // get proper vertical coordinate for cannon
    writeData(Cannon_Y_Position + x, a); // and store it here
    a = M(CurrentPageLoc);
    writeData(Cannon_PageLoc + x, a); // store page number for cannon here
    JSR(GetAreaObjXPosition, 111); // get proper horizontal coordinate for cannon
    writeData(Cannon_X_Position + x, a); // and store it here
    ++x;
    if (x >= 0x06)
    { // if not yet reached sixth cannon, branch to save offset
        x = 0x00; // otherwise initialize it
    } // StrCOffset: save new offset and leave
    writeData(Cannon_Offset, x);
    goto Return;

//------------------------------------------------------------------------

StaircaseObject:
    JSR(ChkLrgObjLength, 112); // check and load length
    if (lrgObjJustStarted)
    { // if length already loaded, skip init part
        a = 0x09; // start past the end for the bottom
        writeData(StaircaseControl, a); // of the staircase
    } // NextStair: move onto next step (or first if starting)
    --M(StaircaseControl);
    y = M(StaircaseControl);
    x = M(StaircaseRowData + y); // get starting row and height to render
    a = M(StaircaseHeightData + y);
    y = a;
    a = 0x61; // now render solid block staircase
    goto RenderUnderPart;

Jumpspring:
    JSR(GetLrgObjAttrib, 113);
    JSR(FindEmptyEnemySlot, 114); // find empty space in enemy object buffer
    JSR(GetAreaObjXPosition, 115); // get horizontal coordinate for jumpspring
    writeData(Enemy_X_Position + x, a); // and store
    a = M(CurrentPageLoc); // store page location of jumpspring
    writeData(Enemy_PageLoc + x, a);
    JSR(GetAreaObjYPosition, 116); // get vertical coordinate for jumpspring
    writeData(Enemy_Y_Position + x, a); // and store
    writeData(Jumpspring_FixedYPos + x, a); // store as permanent coordinate here
    a = JumpspringObject;
    writeData(Enemy_ID + x, a); // write jumpspring object to enemy object buffer
    y = 0x01;
    writeData(Enemy_Y_HighPos + x, y); // store vertical high byte
    ++M(Enemy_Flag + x); // set flag for enemy object buffer
    x = M(0x07);
    a = 0x67; // draw metatiles in two rows where jumpspring is
    writeData(MetatileBuffer + x, a);
    a = 0x68;
    writeData(MetatileBuffer + 1 + x, a);
    goto Return;

//------------------------------------------------------------------------

Hidden1UpBlock:
    a = M(Hidden1UpFlag); // if flag not set, do not render object
    if (a != 0)
    {
        a = 0x00; // if set, init for the next one
        writeData(Hidden1UpFlag, a);
        goto BrickWithItem; // jump to code shared with unbreakable bricks

QuestionBlock:
        JSR(GetAreaObjectID, 117); // get value from level decoder routine
        goto DrawQBlk; // go to render it

BrickWithCoins:
        a = 0x00; // initialize multi-coin timer flag
        writeData(BrickCoinTimerFlag, a);

BrickWithItem:
        JSR(GetAreaObjectID, 118); // save area object ID
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

DrawQBlk: // get appropriate metatile for brick (question block
        a = M(BrickQBlockMetatiles + y);
        pha(); // if branched to here from question block routine)
        JSR(GetLrgObjAttrib, 119); // get row from location byte
        goto DrawRow; // now render the object

GetAreaObjectID:
        a = M(0x00); // get value saved from area parser routine
        a -= 0x00; // possibly residual code
        y = a; // save to Y
    } // ExitDecBlock
    goto Return;

//------------------------------------------------------------------------

Hole_Empty:
    JSR(ChkLrgObjLength, 120); // get lower nybble and save as length
    if (!lrgObjJustStarted)
        goto NoWhirlP; // skip this part if length already loaded
    a = M(AreaType); // check for water type level
    if (a != 0)
        goto NoWhirlP; // if not water type, skip this part
    x = M(Whirlpool_Offset); // get offset for data used by cannons and whirlpools
    JSR(GetAreaObjXPosition, 121); // get proper vertical coordinate of where we're at
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

NoWhirlP: // get appropriate metatile, then
    x = M(AreaType);
    a = M(HoleMetatiles + x); // render the hole proper
    x = 0x08;
    y = 0x0f; // start at ninth row and go to bottom, run RenderUnderPart

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

DrawThisRow: // render contents of A from routine that called this
    writeData(MetatileBuffer + x, a);

WaitOneRow:
    ++x;
    if (x < 0x0d)
    {
        y = M(AreaObjectHeight); // decrement, and stop rendering if there is no more length
        --y;
        if ((y & 0x80) == 0)
            goto RenderUnderPart;
    } // ExitUPartR
    goto Return;

//------------------------------------------------------------------------

ChkLrgObjLength:
    JSR(GetLrgObjAttrib, 122); // get row location and size (length if branched to from here)

ChkLrgObjFixedLength:
    a = M(AreaObjectLength + x); // check for set length counter
    lrgObjJustStarted = false; // not just starting
    if ((a & 0x80) != 0)
    { // if counter not set, load it, otherwise leave alone
        a = y; // save length into length counter
        writeData(AreaObjectLength + x, a);
        lrgObjJustStarted = true; // just starting
    } // LenSet
    goto Return;

//------------------------------------------------------------------------

GetLrgObjAttrib:
    y = M(AreaObjOffsetBuffer + x); // get offset saved from area obj decoding routine
    a = M(W(AreaData) + y); // get first byte of level object
    a &= 0b00001111;
    writeData(0x07, a); // save row location
    ++y;
    a = M(W(AreaData) + y); // get next byte, save lower nybble (length or height)
    a &= 0b00001111; // as Y, then leave
    y = a;
    goto Return;

//------------------------------------------------------------------------

GetAreaObjXPosition:
    a = M(CurrentColumnPos); // multiply current offset where we're at by 16
    a <<= 1; // to obtain horizontal pixel coordinate
    a <<= 1;
    a <<= 1;
    a <<= 1;
    goto Return;

//------------------------------------------------------------------------

GetAreaObjYPosition:
    a = M(0x07); // multiply value by 16
    a <<= 1;
    a <<= 1; // this will give us the proper vertical pixel coordinate
    a <<= 1;
    a <<= 1;
    a += 32; // add 32 pixels for the status bar
    goto Return;

//------------------------------------------------------------------------

GetBlockBufferAddr:
    pha(); // take value of A, save
    a >>= 1; // move high nybble to low
    a >>= 1;
    a >>= 1;
    a >>= 1;
    y = a; // use nybble as pointer to high byte
    a = M(BlockBufferAddr + 2 + y); // of indirect here
    writeData(0x07, a);
    pla();
    a &= 0b00001111; // pull from stack, mask out high nybble
    a += M(BlockBufferAddr + y); // add to low byte
    writeData(0x06, a); // store here and leave
    goto Return;

//------------------------------------------------------------------------

LoadAreaPointer:
    JSR(FindAreaPointer, 123); // find it and store it here
    writeData(AreaPointer, a);

GetAreaType: // mask out all but d6 and d5
    a &= 0b01100000;
    a >>= 5; // make %0xx00000 into %000000xx
    writeData(AreaType, a); // save 2 MSB as area type
    goto Return;

//------------------------------------------------------------------------

FindAreaPointer:
    y = M(WorldNumber); // load offset from world variable
    a = M(WorldAddrOffsets + y);
    a += M(AreaNumber);
    y = a;
    a = M(AreaAddrOffsets + y); // from there we have our area pointer
    goto Return;

//------------------------------------------------------------------------

GetAreaDataAddrs:
    a = M(AreaPointer); // use 2 MSB for Y
    JSR(GetAreaType, 124);
    y = a;
    a = M(AreaPointer); // mask out all but 5 LSB
    a &= 0b00011111;
    writeData(AreaAddrsLOffset, a); // save as low offset
    a = M(EnemyAddrHOffsets + y); // load base value with 2 altered MSB,
    a += M(AreaAddrsLOffset); // becomes offset for level data
    y = a;
    a = M(EnemyDataAddrLow + y); // use offset to load pointer
    writeData(EnemyDataLow, a);
    a = M(EnemyDataAddrHigh + y);
    writeData(EnemyDataHigh, a);
    y = M(AreaType); // use area type as offset
    a = M(AreaDataHOffsets + y); // do the same thing but with different base value
    a += M(AreaAddrsLOffset);
    y = a;
    a = M(AreaDataAddrLow + y); // use this offset to load another pointer
    writeData(AreaDataLow, a);
    a = M(AreaDataAddrHigh + y);
    writeData(AreaDataHigh, a);
    y = 0x00; // load first byte of header
    a = M(W(AreaData) + y);
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
    a = M(AreaDataLow); // increment area data address by 2 bytes
    wide = ((M(AreaDataHigh) << 8) | a) + 0x02;
    writeData(AreaDataLow, LOBYTE(wide));
    writeData(AreaDataHigh, HIBYTE(wide));
    a = HIBYTE(wide);
    goto Return;

//------------------------------------------------------------------------

GameMode:
    a = M(OperMode_Task);
    switch (a)
    {
    case 0:
        goto InitializeArea;
    case 1:
        goto ScreenRoutines;
    case 2:
        goto SecondaryGameSetup;
    case 3:
        goto GameCoreRoutine;
    }

GameCoreRoutine:
    x = M(CurrentPlayer); // get which player is on the screen
    a = M(SavedJoypadBits + x); // use appropriate player's controller bits
    writeData(SavedJoypadBits, a); // as the master controller bits
    JSR(GameRoutines, 125); // execute one of many possible subs
    a = M(OperMode_Task); // check major task of operating mode
    if (a < 0x03)
    { // branch to the game engine itself
        goto Return;

    //------------------------------------------------------------------------
    } // GameEngine
    JSR(ProcFireball_Bubble, 126); // process fireballs and air bubbles
    x = 0x00;

    do // ProcELoop: put incremented offset in X as enemy object offset
    {
        writeData(ObjectOffset, x);
        JSR(EnemiesAndLoopsCore, 127); // process enemy objects
        JSR(FloateyNumbersRoutine, 128); // process floatey numbers
        ++x;
    } while (x != 0x06);
    JSR(GetPlayerOffscreenBits, 129); // get offscreen bits for player object
    JSR(RelativePlayerPosition, 130); // get relative coordinates for player object
    JSR(PlayerGfxHandler, 131); // draw the player
    JSR(BlockObjMT_Updater, 132); // replace block objects with metatiles if necessary
    x = 0x01;
    writeData(ObjectOffset, x); // set offset for second
    JSR(BlockObjectsCore, 133); // process second block object
    --x;
    writeData(ObjectOffset, x); // set offset for first
    JSR(BlockObjectsCore, 134); // process first block object
    JSR(MiscObjectsCore, 135); // process misc objects (hammer, jumping coins)
    JSR(ProcessCannons, 136); // process bullet bill cannons
    JSR(ProcessWhirlpools, 137); // process whirlpools
    JSR(FlagpoleRoutine, 138); // process the flagpole
    JSR(RunGameTimer, 139); // count down the game timer
    JSR(ColorRotation, 140); // cycle one of the background colors
    a = M(Player_Y_HighPos);
    if (((a - 0x02) & 0x80) == 0)
        goto NoChgMus;
    a = M(StarInvincibleTimer); // if star mario invincibility timer at zero,
    if (a != 0)
    { // skip this part
        if (a != 0x04)
            goto NoChgMus; // if not yet at a certain point, continue
        a = M(IntervalTimerControl); // if interval timer not yet expired,
        if (a != 0)
            goto NoChgMus; // branch ahead, don't bother with the music
        JSR(GetAreaMusic, 141); // to re-attain appropriate level music

NoChgMus: // get invincibility timer
        y = M(StarInvincibleTimer);
        a = M(FrameCounter); // get frame counter
        if (y < 0x08)
        { // branch to cycle player's palette quickly
            a >>= 1; // otherwise, divide by 8 to cycle every eighth frame
            a >>= 1;
        } // CycleTwo: if branched here, divide by 2 to cycle every other frame
        a >>= 1;
        JSR(CyclePlayerPalette, 142); // do sub to cycle the palette (note: shares fire flower code)
    } // ClrPlrPal: do sub to clear player's palette bits in attributes
    else // then skip this sub to finish up the game engine
    {
        JSR(ResetPalStar, 143);
    } // SaveAB: save current A and B button
    a = M(A_B_Buttons);
    writeData(PreviousA_B_Buttons, a); // into temp variable to be used on next frame
    a = 0x00;
    writeData(Left_Right_Buttons, a); // nullify left and right buttons temp variable

UpdScrollVar:
    a = M(VRAM_Buffer_AddrCtrl);
    if (a == 0x06)
        goto ExitEng; // then branch to leave
    a = M(AreaParserTaskNum); // otherwise check number of tasks
    if (a == 0)
    {
        a = M(ScrollThirtyTwo); // get horizontal scroll in 0-31 or $00-$20 range
        if (((a - 0x20) & 0x80) != 0)
            goto ExitEng; // branch to leave if not
        a = M(ScrollThirtyTwo);
        a -= 0x20; // otherwise subtract $20 to set appropriately
        writeData(ScrollThirtyTwo, a); // and store
        a = 0x00; // reset vram buffer offset used in conjunction with
        writeData(VRAM_Buffer2_Offset, a); // level graphics buffer at $0341-$035f
    } // RunParser: update the name table with more level graphics
    JSR(AreaParserTaskHandler, 144);

ExitEng: // and after all that, we're finally done!
    goto Return;

//------------------------------------------------------------------------

ScrollHandler:
    a = M(Player_X_Scroll); // load value saved here
    a += M(Platform_X_Scroll); // add value used by left/right platforms
    writeData(Player_X_Scroll, a); // save as new value here to impose force on scroll
    a = M(ScrollLock); // check scroll lock flag
    if (a != 0)
        goto InitScrlAmt; // skip a bunch of code here if set
    a = M(Player_Pos_ForScroll);
    if (a < 0x50)
        goto InitScrlAmt; // if less than 80 pixels to the right, branch
    a = M(SideCollisionTimer); // if timer related to player's side collision
    if (a != 0)
        goto InitScrlAmt; // not expired, branch
    y = M(Player_X_Scroll); // get value and decrement by one
    --y; // if value originally set to zero or otherwise
    if ((y & 0x80) != 0)
        goto InitScrlAmt; // negative for left movement, branch
    ++y;
    if (y >= 0x02)
    {
        --y; // otherwise decrement by one
    } // ChkNearMid
    a = M(Player_Pos_ForScroll);
    if (a < 0x70)
        goto ScrollScreen; // if less than 112 pixels to the right, branch
    y = M(Player_X_Scroll); // otherwise get original value undecremented

ScrollScreen:
    a = y;
    writeData(ScrollAmount, a); // save value here
    a += M(ScrollThirtyTwo); // add to value already set here
    writeData(ScrollThirtyTwo, a); // save as new value here
    wide = ((M(ScreenLeft_PageLoc) << 8) | M(ScreenLeft_X_Pos)) + y; // add to left side coordinate
    writeData(ScreenLeft_X_Pos, LOBYTE(wide)); // save as new left side coordinate
    writeData(HorizontalScroll, LOBYTE(wide)); // save here also
    writeData(ScreenLeft_PageLoc, HIBYTE(wide)); // add carry to page location for left side of the screen
    a = HIBYTE(wide);
    a &= 0x01; // get LSB of page location
    writeData(0x00, a); // save as temp variable for PPU register 1 mirror
    a = M(Mirror_PPU_CTRL_REG1); // get PPU register 1 mirror
    a &= 0b11111110; // save all bits except d0
    a |= M(0x00); // get saved bit here and save in PPU register 1
    writeData(Mirror_PPU_CTRL_REG1, a); // mirror to be used to set name table later
    JSR(GetScreenPosition, 145); // figure out where the right side is
    a = 0x08;
    writeData(ScrollIntervalTimer, a); // set scroll timer (residual, not used elsewhere)
    goto ChkPOffscr; // skip this part

InitScrlAmt:
    a = 0x00;
    writeData(ScrollAmount, a); // initialize value here

ChkPOffscr: // set X for player offset
    x = 0x00;
    JSR(GetXOffscreenBits, 146); // get horizontal offscreen bits for player
    writeData(0x00, a); // save them here
    y = 0x00; // load default offset (left side)
    shiftedBit = (a & 0x80) != 0;
    if (!shiftedBit) // if d7 of offscreen bits are set,
    { // branch with default offset
        ++y; // otherwise use different offset (right side)
        a = M(0x00);
        a &= 0b00100000; // check offscreen bits for d5 set
        if (a == 0)
            goto InitPlatScrl; // if not set, branch ahead of this part
    } // KeepOnscr: get left or right side coordinate based on offset
    a = M(ScreenEdge_X_Pos + y);
    wide = ((M(ScreenEdge_PageLoc + y) << 8) | a) - M(X_SubtracterData + y); // subtract amount based on offset
    writeData(Player_X_Position, LOBYTE(wide)); // store as player position to prevent movement further
    writeData(Player_PageLoc, HIBYTE(wide)); // save as player's page location
    a = HIBYTE(wide); // get left or right page location based on offset
    a = M(Left_Right_Buttons); // check saved controller bits
    if (a == M(OffscrJoypadBitsData + y))
        goto InitPlatScrl; // if not equal, branch
    a = 0x00;
    writeData(Player_X_Speed, a); // otherwise nullify horizontal speed of player

InitPlatScrl: // nullify platform force imposed on scroll
    a = 0x00;
    writeData(Platform_X_Scroll, a);
    goto Return;

//------------------------------------------------------------------------

GetScreenPosition:
    a = M(ScreenLeft_X_Pos); // get coordinate of screen's left boundary
    wide = ((M(ScreenLeft_PageLoc) << 8) | a) + 0xff; // add 255 pixels
    writeData(ScreenRight_X_Pos, LOBYTE(wide)); // store as coordinate of screen's right boundary
    writeData(ScreenRight_PageLoc, HIBYTE(wide)); // store as page number where right boundary is
    a = HIBYTE(wide); // get page number where left boundary is
    goto Return;

//------------------------------------------------------------------------

GameRoutines:
    a = M(GameEngineSubroutine); // run routine based on number (a few of these routines are
    switch (a)
    {
    case 0:
        goto Entrance_GameTimerSetup;
    case 1:
        goto Vine_AutoClimb;
    case 2:
        goto SideExitPipeEntry;
    case 3:
        goto VerticalPipeEntry;
    case 4:
        goto FlagpoleSlide;
    case 5:
        goto PlayerEndLevel;
    case 6:
        goto PlayerLoseLife;
    case 7:
        goto PlayerEntrance;
    case 8:
        goto PlayerCtrlRoutine;
    case 9:
        goto PlayerChangeSize;
    case 10:
        goto PlayerInjuryBlink;
    case 11:
        goto PlayerDeath;
    case 12:
        goto PlayerFireFlower;
    } // merely placeholders as conditions for other routines)

PlayerEntrance:
    a = M(AltEntranceControl); // check for mode of alternate entry
    if (a != 0x02)
    { // if found, branch to enter from pipe or with vine
        a = 0x00;
        y = M(Player_Y_Position); // if vertical position above a certain
        if (y < 0x30)
            goto AutoControlPlayer; // with player movement code, do not return
        a = M(PlayerEntranceCtrl); // check player entry bits from header
        if (a != 0x06)
        { // if set to 6 or 7, execute pipe intro code
            if (a != 0x07)
                goto PlayerRdy;
        } // ChkBehPipe: check for sprite attributes
        a = M(Player_SprAttrib);
        if (a == 0)
        { // branch if found
            a = 0x01;
            goto AutoControlPlayer; // force player to walk to the right
        } // IntroEntr: execute sub to move player to the right
        JSR(EnterSidePipe, 147);
        --M(ChangeAreaTimer); // decrement timer for change of area
        if (M(ChangeAreaTimer) != 0)
            goto ExitEntr; // branch to exit if not yet expired
        ++M(DisableIntermediate); // set flag to skip world and lives display
        goto NextArea; // jump to increment to next area and set modes
    } // EntrMode2: if controller override bits set here,
    a = M(JoypadOverride);
    if (a == 0)
    { // branch to enter with vine
        a = 0xff; // otherwise, set value here then execute sub
        JSR(MovePlayerYAxis, 148); // to move player upwards (note $ff = -1)
        a = M(Player_Y_Position); // check to see if player is at a specific coordinate
        if (a < 0x91)
            goto PlayerRdy; // to be at specific height to look/function right) branch
        goto Return; // to the last part, otherwise leave

    //------------------------------------------------------------------------
    } // VineEntr
    a = M(VineHeight);
    if (a != 0x60)
        goto ExitEntr; // if vine not yet reached maximum height, branch to leave
    a = M(Player_Y_Position); // get player's vertical coordinate
    y = 0x00; // load default values to be written to
    a = 0x01; // this value moves player to the right off the vine
    if (M(Player_Y_Position) >= 0x99)
    { // if vertical coordinate < preset value, use defaults
        a = 0x03;
        writeData(Player_State, a); // otherwise set player state to climbing
        ++y; // increment value in Y
        a = 0x08; // set block in block buffer to cover hole, then
        writeData(Block_Buffer_1 + 0xb4, a); // use same value to force player to climb
    } // OffVine: set collision detection disable flag
    writeData(DisableCollisionDet, y);
    JSR(AutoControlPlayer, 149); // use contents of A to move player up or right, execute sub
    a = M(Player_X_Position);
    if (a < 0x48)
        goto ExitEntr; // if not far enough to the right, branch to leave

PlayerRdy: // set routine to be executed by game engine next frame
    a = 0x08;
    writeData(GameEngineSubroutine, a);
    a = 0x01; // set to face player to the right
    writeData(PlayerFacingDir, a);
    a >>= 1; // init A
    writeData(AltEntranceControl, a); // init mode of entry
    writeData(DisableCollisionDet, a); // init collision detection disable flag
    writeData(JoypadOverride, a); // nullify controller override bits

ExitEntr: // leave!
    goto Return;

//------------------------------------------------------------------------

AutoControlPlayer:
    writeData(SavedJoypadBits, a); // override controller bits with contents of A if executing here

PlayerCtrlRoutine:
    a = M(GameEngineSubroutine); // check task here
    if (a == 0x0b)
        goto SizeChk;
    a = M(AreaType); // are we in a water type area?
    if (a != 0)
        goto SaveJoyp; // if not, branch
    y = M(Player_Y_HighPos);
    --y; // if not in vertical area between
    if (y == 0)
    { // status bar and bottom, branch
        a = M(Player_Y_Position);
        if (a < 0xd0)
            goto SaveJoyp; // not in the vertical area between status bar or bottom,
    } // DisJoyp: disable controller bits
    a = 0x00;
    writeData(SavedJoypadBits, a);

SaveJoyp: // otherwise store A and B buttons in $0a
    a = M(SavedJoypadBits);
    a &= 0b11000000;
    writeData(A_B_Buttons, a);
    a = M(SavedJoypadBits); // store left and right buttons in $0c
    a &= 0b00000011;
    writeData(Left_Right_Buttons, a);
    a = M(SavedJoypadBits); // store up and down buttons in $0b
    a &= 0b00001100;
    writeData(Up_Down_Buttons, a);
    a &= 0b00000100; // check for pressing down
    if (a == 0)
        goto SizeChk; // if not, branch
    a = M(Player_State); // check player's state
    if (a != 0)
        goto SizeChk; // if not on the ground, branch
    y = M(Left_Right_Buttons); // check left and right
    if (y == 0)
        goto SizeChk; // if neither pressed, branch
    a = 0x00;
    writeData(Left_Right_Buttons, a); // if pressing down while on the ground,
    writeData(Up_Down_Buttons, a); // nullify directional bits

SizeChk: // run movement subroutines
    JSR(PlayerMovementSubs, 150);
    y = 0x01; // is player small?
    a = M(PlayerSize);
    if (a != 0)
        goto ChkMoveDir;
    y = 0x00; // check for if crouching
    a = M(CrouchingFlag);
    if (a == 0)
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
            a <<= 1; // otherwise change to move to the left
        } // SetMoveDir: set moving direction
        writeData(Player_MovingDir, a);
    } // PlayerSubs: move the screen if necessary
    JSR(ScrollHandler, 151);
    JSR(GetPlayerOffscreenBits, 152); // get player's offscreen bits
    JSR(RelativePlayerPosition, 153); // get coordinates relative to the screen
    x = 0x00; // set offset for player object
    JSR(BoundingBoxCore, 154); // get player's bounding box coordinates
    JSR(PlayerBGCollision, 155); // do collision detection and process
    a = M(Player_Y_Position);
    if (a < 0x40)
        goto PlayerHole; // if so, branch ahead
    a = M(GameEngineSubroutine);
    if (a == 0x05)
        goto PlayerHole;
    if (a == 0x07)
        goto PlayerHole;
    if (a < 0x04)
        goto PlayerHole;
    a = M(Player_SprAttrib);
    a &= 0b11011111; // otherwise nullify player's
    writeData(Player_SprAttrib, a); // background priority flag

PlayerHole: // check player's vertical high byte
    a = M(Player_Y_HighPos);
    if (((a - 0x02) & 0x80) != 0)
        goto ExitCtrl; // branch to leave if not that far down
    x = 0x01;
    writeData(ScrollLock, x); // set scroll lock
    y = 0x04;
    writeData(0x07, y); // set value here
    x = 0x00; // use X as flag, and clear for cloud level
    y = M(GameTimerExpiredFlag); // check game timer expiration flag
    if (y == 0)
    { // if set, branch
        y = M(CloudTypeOverride); // check for cloud type override
        if (y != 0)
            goto ChkHoleX; // skip to last part if found
    } // HoleDie: set flag in X for player death
    ++x;
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
    writeData(0x07, y); // change value here

ChkHoleX: // compare vertical high byte with value set here
    if (((a - M(0x07)) & 0x80) != 0)
        goto ExitCtrl; // if less, branch to leave
    --x; // otherwise decrement flag in X
    if ((x & 0x80) == 0)
    { // if flag was clear, branch to set modes and other values
        y = M(EventMusicBuffer); // check to see if music is still playing
        if (y != 0)
            goto ExitCtrl; // branch to leave if so
        a = 0x06; // otherwise set to run lose life routine
        writeData(GameEngineSubroutine, a); // on next frame

ExitCtrl: // leave
        goto Return;

    //------------------------------------------------------------------------
    } // CloudExit
    a = 0x00;
    writeData(JoypadOverride, a); // clear controller override bits if any are set
    JSR(SetEntr, 156); // do sub to set secondary mode
    ++M(AltEntranceControl); // set mode of entry to 3
    goto Return;

//------------------------------------------------------------------------

Vine_AutoClimb:
    a = M(Player_Y_HighPos); // check to see whether player reached position
    if (a == 0)
    { // above the status bar yet and if so, set modes
        a = M(Player_Y_Position);
        if (a < 0xe4)
            goto SetEntr;
    } // AutoClimb: set controller bits override to up
    a = 0b00001000;
    writeData(JoypadOverride, a);
    y = 0x03; // set player state to climbing
    writeData(Player_State, y);
    goto AutoControlPlayer;

SetEntr: // set starting position to override
    a = 0x02;
    writeData(AltEntranceControl, a);
    goto ChgAreaMode; // set modes

VerticalPipeEntry:
    a = 0x01; // set 1 as movement amount
    JSR(MovePlayerYAxis, 157); // do sub to move player downwards
    JSR(ScrollHandler, 158); // do sub to scroll screen with saved force if necessary
    y = 0x00; // load default mode of entry
    a = M(WarpZoneControl); // check warp zone control variable/flag
    if (a != 0)
        goto ChgAreaPipe; // if set, branch to use mode 0
    ++y;
    a = M(AreaType); // check for castle level type
    if (a != 0x03)
        goto ChgAreaPipe; // if not castle type level, use mode 1
    ++y;
    goto ChgAreaPipe; // otherwise use mode 2

MovePlayerYAxis:
    a += M(Player_Y_Position); // add contents of A to player position
    writeData(Player_Y_Position, a);
    goto Return;

//------------------------------------------------------------------------

SideExitPipeEntry:
    JSR(EnterSidePipe, 159); // execute sub to move player to the right
    y = 0x02;

ChgAreaPipe: // decrement timer for change of area
    --M(ChangeAreaTimer);
    if (M(ChangeAreaTimer) == 0)
    {
        writeData(AltEntranceControl, y); // when timer expires set mode of alternate entry

ChgAreaMode: // set flag to disable screen output
        ++M(DisableScreenFlag);
        a = 0x00;
        writeData(OperMode_Task, a); // set secondary mode of operation
        writeData(Sprite0HitDetectFlag, a); // disable sprite 0 check
    } // ExitCAPipe: leave
    goto Return;

//------------------------------------------------------------------------

EnterSidePipe:
    a = 0x08; // set player's horizontal speed
    writeData(Player_X_Speed, a);
    y = 0x01; // set controller right button by default
    a = M(Player_X_Position); // mask out higher nybble of player's
    a &= 0b00001111; // horizontal position
    if (a == 0)
    {
        writeData(Player_X_Speed, a); // if lower nybble = 0, set as horizontal speed
        y = a; // and nullify controller bit override here
    } // RightPipe: use contents of Y to
    a = y;
    JSR(AutoControlPlayer, 160); // execute player control routine with ctrl bits nulled
    goto Return;

//------------------------------------------------------------------------

PlayerChangeSize:
    a = M(TimerControl); // check master timer control
    if (a == 0xf8)
    { // branch if before or after that point
    } // EndChgSize: check again for another specific moment
    else // otherwise run code to get growing/shrinking going
    {
        if (a == 0xc4)
        { // and branch to leave if before or after that point
            JSR(DonePlayerTask, 161); // otherwise do sub to init timer control and set routine
        } // ExitChgSize: and then leave
        goto Return;

    //------------------------------------------------------------------------

PlayerInjuryBlink:
        a = M(TimerControl); // check master timer control
        if (a < 0xf0)
        { // branch if before that point
            if (a == 0xc8)
                goto DonePlayerTask; // branch if at that point, and not before or after
            goto PlayerCtrlRoutine; // otherwise run player control routine
        } // ExitBlink: do unconditional branch to leave
        if (a != 0xf0)
            goto ExitBoth;
    } // InitChangeSize
    y = M(PlayerChangeSizeFlag); // if growing/shrinking flag already set
    if (y != 0)
        goto ExitBoth; // then branch to leave
    writeData(PlayerAnimCtrl, y); // otherwise initialize player's animation frame control
    ++M(PlayerChangeSizeFlag); // set growing/shrinking flag
    a = M(PlayerSize);
    a ^= 0x01; // invert player's size
    writeData(PlayerSize, a);

ExitBoth: // leave
    goto Return;

//------------------------------------------------------------------------

PlayerDeath:
    a = M(TimerControl); // check master timer control
    if (a < 0xf0)
    { // branch to leave if before that point
        goto PlayerCtrlRoutine; // otherwise run player control routine

DonePlayerTask:
        a = 0x00;
        writeData(TimerControl, a); // initialize master timer control to continue timers
        a = 0x08;
        writeData(GameEngineSubroutine, a); // set player control routine to run next frame
        goto Return; // leave

    //------------------------------------------------------------------------

PlayerFireFlower:
        a = M(TimerControl); // check master timer control
        if (a != 0xc0)
        { // branch if at moment, not before or after
            a = M(FrameCounter); // get frame counter
            a >>= 1;
            a >>= 1; // divide by four to change every four frames

CyclePlayerPalette:
            a &= 0x03; // mask out all but d1-d0 (previously d3-d2)
            writeData(0x00, a); // store result here to use as palette bits
            a = M(Player_SprAttrib); // get player attributes
            a &= 0b11111100; // save any other bits but palette bits
            a |= M(0x00); // add palette bits
            writeData(Player_SprAttrib, a); // store as new player attributes
            goto Return; // and leave

        //------------------------------------------------------------------------
        } // ResetPalFireFlower
        JSR(DonePlayerTask, 162); // do sub to init timer control and run player control routine

ResetPalStar:
        a = M(Player_SprAttrib); // get player attributes
        a &= 0b11111100; // mask out palette bits to force palette 0
        writeData(Player_SprAttrib, a); // store as new player attributes
        goto Return; // and leave

    //------------------------------------------------------------------------
    } // ExitDeath
    goto Return; // leave from death routine

//------------------------------------------------------------------------

FlagpoleSlide:
    a = M(Enemy_ID + 5); // check special use enemy slot
    if (a == FlagpoleFlagObject)
    { // if not found, branch to something residual
        a = M(FlagpoleSoundQueue); // load flagpole sound
        writeData(Square1SoundQueue, a); // into square 1's sfx queue
        a = 0x00;
        writeData(FlagpoleSoundQueue, a); // init flagpole sound queue
        y = M(Player_Y_Position);
        if (y < 0x9e)
        { // far enough, and if so, branch with no controller bits set
            a = 0x04; // otherwise force player to climb down (to slide)
        } // SlidePlayer: jump to player control routine
        goto AutoControlPlayer;
    } // NoFPObj: increment to next routine (this may
    ++M(GameEngineSubroutine);
    goto Return; // be residual code)

//------------------------------------------------------------------------

PlayerEndLevel:
    a = 0x01; // force player to walk to the right
    JSR(AutoControlPlayer, 163);
    a = M(Player_Y_Position); // check player's vertical position
    if (a < 0xae)
        goto ChkStop; // if player is not yet off the flagpole, skip this part
    a = M(ScrollLock); // if scroll lock not set, branch ahead to next part
    if (a == 0)
        goto ChkStop; // because we only need to do this part once
    a = EndOfLevelMusic;
    writeData(EventMusicQueue, a); // load win level music in event music queue
    a = 0x00;
    writeData(ScrollLock, a); // turn off scroll lock to skip this part later

ChkStop: // get player collision bits
    if ((M(Player_CollisionBits) & 0x01) == 0) // check for d0 set
    { // if d0 set, skip to next part
        a = M(StarFlagTaskControl); // if star flag task control already set,
        if (a == 0)
        { // go ahead with the rest of the code
            ++M(StarFlagTaskControl); // otherwise set task control now (this gets ball rolling!)
        } // InCastle: set player's background priority bit to
        a = 0b00100000;
        writeData(Player_SprAttrib, a); // give illusion of being inside the castle
    } // RdyNextA
    a = M(StarFlagTaskControl);
    if (a == 0x05)
    { // beyond last valid task number, branch to leave
        ++M(LevelNumber); // increment level number used for game logic
        a = M(LevelNumber);
        if (a != 0x03)
            goto NextArea; // and skip this last part here if not
        y = M(WorldNumber); // get world number as offset
        a = M(CoinTallyFor1Ups); // check third area coin tally for bonus 1-ups
        if (a < M(Hidden1UpCoinAmts + y))
            goto NextArea; // at least this number of coins, leave flag clear
        ++M(Hidden1UpFlag); // otherwise set hidden 1-up box control flag

NextArea: // increment area number used for address loader
        ++M(AreaNumber);
        JSR(LoadAreaPointer, 164); // get new level pointer
        ++M(FetchNewGameTimerFlag); // set flag to load new game timer
        JSR(ChgAreaMode, 165); // do sub to set secondary mode, disable screen and sprite 0
        writeData(HalfwayPage, a); // reset halfway page to 0 (beginning)
        a = Silence;
        writeData(EventMusicQueue, a); // silence music and leave
    } // ExitNA
    goto Return;

//------------------------------------------------------------------------

PlayerMovementSubs:
    a = 0x00; // set A to init crouch flag by default
    y = M(PlayerSize); // is player small?
    if (y == 0)
    { // if so, branch
        a = M(Player_State); // check state of player
        if (a != 0)
            goto ProcMove; // if not on the ground, branch
        a = M(Up_Down_Buttons); // load controller bits for up and down
        a &= 0b00000100; // single out bit for down button
    } // SetCrouch: store value in crouch flag
    writeData(CrouchingFlag, a);

ProcMove: // run sub related to jumping and swimming
    JSR(PlayerPhysicsSub, 166);
    a = M(PlayerChangeSizeFlag); // if growing/shrinking flag set,
    if (a == 0)
    { // branch to leave
        a = M(Player_State);
        if (a != 0x03)
        { // if climbing, branch ahead, leave timer unset
            y = 0x18;
            writeData(ClimbSideTimer, y); // otherwise reset timer now
        } // MoveSubs
        switch (a)
        {
        case 0:
            goto OnGroundStateSub;
        case 1:
            goto JumpSwimSub;
        case 2:
            goto FallingSub;
        case 3:
            goto ClimbingSub;
        }
    } // NoMoveSub
    goto Return;

//------------------------------------------------------------------------

OnGroundStateSub:
    JSR(GetPlayerAnimSpeed, 167); // do a sub to set animation frame timing
    a = M(Left_Right_Buttons);
    if (a != 0)
    { // if left/right controller bits not set, skip instruction
        writeData(PlayerFacingDir, a); // otherwise set new facing direction
    } // GndMove: do a sub to impose friction on player's walk/run
    JSR(ImposeFriction, 168);
    JSR(MovePlayerHorizontally, 169); // do another sub to move player horizontally
    writeData(Player_X_Scroll, a); // set returned value as player's movement speed for scroll
    goto Return;

//------------------------------------------------------------------------

FallingSub:
    a = M(VerticalForceDown);
    writeData(VerticalForce, a); // dump vertical movement force for falling into main one
    goto LRAir; // movement force, then skip ahead to process left/right movement

JumpSwimSub:
    y = M(Player_Y_Speed); // if player's vertical speed zero
    if ((y & 0x80) != 0)
    { // or moving downwards, branch to falling
        a = M(A_B_Buttons);
        a &= A_Button; // check to see if A button is being pressed
        a &= M(PreviousA_B_Buttons); // and was pressed in previous frame
        if (a != 0)
            goto ProcSwim; // if so, branch elsewhere
        a = M(JumpOrigin_Y_Position); // get vertical position player jumped from
        a -= M(Player_Y_Position); // subtract current from original vertical coordinate
        if (a < M(DiffToHaltJump))
            goto ProcSwim; // or just starting to jump, if just starting, skip ahead
    } // DumpFall: otherwise dump falling into main fractional
    a = M(VerticalForceDown);
    writeData(VerticalForce, a);

ProcSwim: // if swimming flag not set,
    a = M(SwimmingFlag);
    if (a == 0)
        goto LRAir; // branch ahead to last part
    JSR(GetPlayerAnimSpeed, 170); // do a sub to get animation frame timing
    a = M(Player_Y_Position);
    if (a < 0x14)
    { // if not yet reached a certain position, branch ahead
        a = 0x18;
        writeData(VerticalForce, a); // otherwise set fractional
    } // LRWater: check left/right controller bits (check for swimming)
    a = M(Left_Right_Buttons);
    if (a == 0)
        goto LRAir; // if not pressing any, skip
    writeData(PlayerFacingDir, a); // otherwise set facing direction accordingly

LRAir: // check left/right controller bits (check for jumping/falling)
    a = M(Left_Right_Buttons);
    if (a != 0)
    { // if not pressing any, skip
        JSR(ImposeFriction, 171); // otherwise process horizontal movement
    } // JSMove: do a sub to move player horizontally
    JSR(MovePlayerHorizontally, 172);
    writeData(Player_X_Scroll, a); // set player's speed here, to be used for scroll later
    a = M(GameEngineSubroutine);
    if (a == 0x0b)
    { // branch if not set to run
        a = 0x28;
        writeData(VerticalForce, a); // otherwise set fractional
    } // ExitMov1: jump to move player vertically, then leave
    goto MovePlayerVertically;

ClimbingSub:
    y = 0x00; // set default adder here
    a = M(Player_Y_Speed); // get player's vertical speed
    if ((a & 0x80) != 0)
    { // if not moving upwards, branch
        --y; // otherwise set adder to $ff
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
    a = M(Left_Right_Buttons); // compare left/right controller bits
    a &= M(Player_CollisionBits); // to collision flag
    if (a != 0)
    { // if not set, skip to end
        y = M(ClimbSideTimer); // otherwise check timer
        if (y == 0)
        { // if timer not expired, branch to leave
            y = 0x18;
            writeData(ClimbSideTimer, y); // otherwise set timer now
            x = 0x00; // set default offset here
            y = M(PlayerFacingDir); // get facing direction
            shiftedBit = (a & 0x01) != 0;
            if (!shiftedBit) // check the right button controller bit
            { // if controller right pressed, branch ahead
                ++x;
                ++x; // otherwise increment offset by 2 bytes
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
            a = M(Left_Right_Buttons); // get left/right controller bits again
            a ^= 0b00000011; // invert them and store them while player
            writeData(PlayerFacingDir, a); // is on vine to face player in opposite direction
        } // ExitCSub: then leave
        goto Return;

    //------------------------------------------------------------------------
    } // InitCSTimer: initialize timer here
    writeData(ClimbSideTimer, a);
    goto Return;

//------------------------------------------------------------------------

PlayerPhysicsSub:
    a = M(Player_State); // check player state
    if (a == 0x03)
    { // if not climbing, branch
        y = 0x00;
        a = M(Up_Down_Buttons); // get controller bits for up/down
        a &= M(Player_CollisionBits); // check against player's collision detection bits
        if (a == 0)
            goto ProcClimb; // if not pressing up or down, branch
        ++y;
        a &= 0b00001000; // check for pressing up
        if (a != 0)
            goto ProcClimb;
        ++y;

ProcClimb: // load value here
        x = M(Climb_Y_MForceData + y);
        writeData(Player_Y_MoveForce, x); // store as vertical movement force
        a = 0x08; // load default animation timing
        x = M(Climb_Y_SpeedData + y); // load some other value here
        writeData(Player_Y_Speed, x); // store as vertical speed
        if ((x & 0x80) == 0)
        { // if climbing down, use default animation timing value
            a >>= 1; // otherwise divide timer setting by 2
        } // SetCAnim: store animation timer setting and leave
        writeData(PlayerAnimTimerSet, a);
        goto Return;

    //------------------------------------------------------------------------
    } // CheckForJumping
    a = M(JumpspringAnimCtrl); // if jumpspring animating,
    if (a != 0)
        goto NoJump; // skip ahead to something else
    a = M(A_B_Buttons); // check for A button press
    a &= A_Button;
    if (a == 0)
        goto NoJump; // if not, branch to something else
    a &= M(PreviousA_B_Buttons); // if button not pressed in previous frame, branch
    if (a != 0)
    {

NoJump: // otherwise, jump to something else
        goto X_Physics;
    } // ProcJumping
    a = M(Player_State); // check player state
    if (a == 0)
        goto InitJS; // if on the ground, branch
    a = M(SwimmingFlag); // if swimming flag not set, jump to do something else
    if (a == 0)
        goto NoJump; // to prevent midair jumping, otherwise continue
    a = M(JumpSwimTimer); // if jump/swim timer nonzero, branch
    if (a != 0)
        goto InitJS;
    a = M(Player_Y_Speed); // check player's vertical speed
    if ((a & 0x80) == 0)
        goto InitJS; // if player's vertical speed motionless or down, branch
    goto X_Physics; // if timer at zero and player still rising, do not swim

InitJS: // set jump/swim timer
    a = 0x20;
    writeData(JumpSwimTimer, a);
    y = 0x00; // initialize vertical force and dummy variable
    writeData(Player_YMF_Dummy, y);
    writeData(Player_Y_MoveForce, y);
    a = M(Player_Y_HighPos); // get vertical high and low bytes of jump origin
    writeData(JumpOrigin_Y_HighPos, a); // and store them next to each other here
    a = M(Player_Y_Position);
    writeData(JumpOrigin_Y_Position, a);
    a = 0x01; // set player state to jumping/swimming
    writeData(Player_State, a);
    a = M(Player_XSpeedAbsolute); // check value related to walking/running speed
    if (a < 0x09)
        goto ChkWtr; // branch if below certain values, increment Y
    ++y; // for each amount equal or exceeded
    if (a < 0x10)
        goto ChkWtr;
    ++y;
    if (a < 0x19)
        goto ChkWtr;
    ++y;
    if (a < 0x1c)
        goto ChkWtr; // note that for jumping, range is 0-4 for Y
    ++y;

ChkWtr: // set value here (apparently always set to 1)
    a = 0x01;
    writeData(DiffToHaltJump, a);
    a = M(SwimmingFlag); // if swimming flag disabled, branch
    if (a == 0)
        goto GetYPhy;
    y = 0x05; // otherwise set Y to 5, range is 5-6
    a = M(Whirlpool_Flag); // if whirlpool flag not set, branch
    if (a == 0)
        goto GetYPhy;
    ++y; // otherwise increment to 6

GetYPhy: // store appropriate jump/swim
    a = M(JumpMForceData + y);
    writeData(VerticalForce, a); // data here
    a = M(FallMForceData + y);
    writeData(VerticalForceDown, a);
    a = M(InitMForceData + y);
    writeData(Player_Y_MoveForce, a);
    a = M(PlayerYSpdData + y);
    writeData(Player_Y_Speed, a);
    a = M(SwimmingFlag); // if swimming flag disabled, branch
    if (a != 0)
    {
        a = Sfx_EnemyStomp; // load swim/goomba stomp sound into
        writeData(Square1SoundQueue, a); // square 1's sfx queue
        a = M(Player_Y_Position);
        if (a >= 0x14)
            goto X_Physics; // if below a certain point, branch
        a = 0x00; // otherwise reset player's vertical speed
        writeData(Player_Y_Speed, a); // and jump to something else to keep player
        goto X_Physics; // from swimming above water level
    } // PJumpSnd: load big mario's jump sound by default
    a = Sfx_BigJump;
    y = M(PlayerSize); // is mario big?
    if (y != 0)
    {
        a = Sfx_SmallJump; // if not, load small mario's jump sound
    } // SJumpSnd: store appropriate jump sound in square 1 sfx queue
    writeData(Square1SoundQueue, a);

X_Physics:
    y = 0x00;
    writeData(0x00, y); // init value here
    a = M(Player_State); // if mario is on the ground, branch
    if (a != 0)
    {
        a = M(Player_XSpeedAbsolute); // check something that seems to be related
        if (a >= 0x19)
            goto GetXPhy; // if =>$19 branch here
        if (a < 0x19)
            goto ChkRFast; // if not branch elsewhere
    } // ProcPRun: if mario on the ground, increment Y
    ++y;
    a = M(AreaType); // check area type
    if (a == 0)
        goto ChkRFast; // if water type, branch
    --y; // decrement Y by default for non-water type area
    a = M(Left_Right_Buttons); // get left/right controller bits
    if (a != M(Player_MovingDir))
        goto ChkRFast; // if controller bits <> moving direction, skip this part
    a = M(A_B_Buttons); // check for b button pressed
    a &= B_Button;
    if (a == 0)
    { // if pressed, skip ahead to set timer
        a = M(RunningTimer); // check for running timer set
        if (a != 0)
            goto GetXPhy; // if set, branch

ChkRFast: // if running timer not set or level type is water,
        ++y;
        ++M(0x00); // increment Y again and temp variable in memory
        a = M(RunningSpeed);
        if (a == 0)
        { // if running speed set here, branch
            a = M(Player_XSpeedAbsolute);
            if (a < 0x21)
                goto GetXPhy; // if less than a certain amount, branch ahead
        } // FastXSp: if running speed set or speed => $21 increment $00
        ++M(0x00);
        goto GetXPhy; // and jump ahead
    } // SetRTmr: if b button pressed, set running timer
    a = 0x0a;
    writeData(RunningTimer, a);

GetXPhy: // get maximum speed to the left
    a = M(MaxLeftXSpdData + y);
    writeData(MaximumLeftSpeed, a);
    a = M(GameEngineSubroutine); // check for specific routine running
    if (a == 0x07)
    { // if not running, skip and use old value of Y
        y = 0x03; // otherwise set Y to 3
    } // GetXPhy2: get maximum speed to the right
    a = M(MaxRightXSpdData + y);
    writeData(MaximumRightSpeed, a);
    y = M(0x00); // get other value in memory
    a = M(FrictionData + y); // get value using value in memory as offset
    writeData(FrictionAdderLow, a);
    a = 0x00;
    writeData(FrictionAdderHigh, a); // init something here
    a = M(PlayerFacingDir);
    if (a != M(Player_MovingDir))
    { // if the same, branch to leave
        wide = ((M(FrictionAdderHigh) << 8) | M(FrictionAdderLow)) << 1; // otherwise double the 16-bit friction adder
        writeData(FrictionAdderLow, LOBYTE(wide));
        writeData(FrictionAdderHigh, HIBYTE(wide));
    } // ExitPhy: and then leave
    goto Return;

//------------------------------------------------------------------------

GetPlayerAnimSpeed:
    y = 0x00; // initialize offset in Y
    a = M(Player_XSpeedAbsolute); // check player's walking/running speed
    if (a < 0x1c)
    { // if greater than a certain amount, branch ahead
        ++y; // otherwise increment Y
        if (a < 0x0e)
        { // if greater than this but not greater than first, skip increment
            ++y; // otherwise increment Y again
        } // ChkSkid: get controller bits
        a = M(SavedJoypadBits);
        a &= 0b01111111; // mask out A button
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
    a = M(Player_XSpeedAbsolute);
    if (a >= 0x0b)
        goto SetAnimSpd; // if greater than this amount, branch
    a = M(PlayerFacingDir);
    writeData(Player_MovingDir, a); // otherwise use facing direction to set moving direction
    a = 0x00;
    writeData(Player_X_Speed, a); // nullify player's horizontal speed
    writeData(Player_X_MoveForce, a); // and dummy variable for player

SetAnimSpd: // get animation timer setting using Y as offset
    a = M(PlayerAnimTmrData + y);
    writeData(PlayerAnimTimerSet, a);
    goto Return;

//------------------------------------------------------------------------

ImposeFriction:
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
    goto Return;

//------------------------------------------------------------------------

ProcFireball_Bubble:
    a = M(PlayerStatus); // check player's status
    if (a >= 0x02)
    { // if not fiery, branch
        a = M(A_B_Buttons);
        a &= B_Button; // check for b button pressed
        if (a == 0)
            goto ProcFireballs; // branch if not pressed
        a &= M(PreviousA_B_Buttons);
        if (a != 0)
            goto ProcFireballs; // if button pressed in previous frame, branch
        a = M(FireballCounter); // load fireball counter
        a &= 0b00000001; // get LSB and use as offset for buffer
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
        a = Sfx_Fireball; // play fireball sound effect
        writeData(Square1SoundQueue, a);
        a = 0x02; // load state
        writeData(Fireball_State + x, a);
        y = M(PlayerAnimTimerSet); // copy animation frame timer setting
        writeData(FireballThrowingTimer, y); // into fireball throwing timer
        --y;
        writeData(PlayerAnimTimer, y); // decrement and store in player's animation timer
        ++M(FireballCounter); // increment fireball counter

ProcFireballs:
        x = 0x00;
        JSR(FireballObjCore, 173); // process first fireball object
        x = 0x01;
        JSR(FireballObjCore, 174); // process second fireball object, then do air bubbles
    } // ProcAirBubbles
    a = M(AreaType); // if not water type level, skip the rest of this
    if (a == 0)
    {
        x = 0x02; // otherwise load counter and use as offset

        do // BublLoop: store offset
        {
            writeData(ObjectOffset, x);
            JSR(BubbleCheck, 175); // check timers and coordinates, create air bubble
            JSR(RelativeBubblePosition, 176); // get relative coordinates
            JSR(GetBubbleOffscreenBits, 177); // get offscreen information
            JSR(DrawBubble, 178); // draw the air bubble
            --x;
        } while ((x & 0x80) == 0); // do this until all three are handled
    } // BublExit: then leave
    goto Return;

//------------------------------------------------------------------------

FireballObjCore:
    writeData(ObjectOffset, x); // store offset as current object
    if ((M(Fireball_State + x) & 0x80) == 0) // check for d7 = 1
    { // if so, branch to get relative coordinates and draw explosion
        y = M(Fireball_State + x); // if fireball inactive, branch to leave
        if (y != 0)
        {
            --y; // if fireball state set to 1, skip this part and just run it
            if (y != 0)
            {
                a = M(Player_X_Position); // get player's horizontal position
                wide = ((M(Player_PageLoc) << 8) | a) + 0x04;// add four pixels and store as fireball's horizontal position
                writeData(Fireball_X_Position + x, LOBYTE(wide));
                writeData(Fireball_PageLoc + x, HIBYTE(wide));
                a = HIBYTE(wide);// get player's page location
                a = M(Player_Y_Position); // get player's vertical position and store
                writeData(Fireball_Y_Position + x, a);
                a = 0x01; // set high byte of vertical position
                writeData(Fireball_Y_HighPos + x, a);
                y = M(PlayerFacingDir); // get player's facing direction
                --y; // decrement to use as offset here
                a = M(FireballXSpdData + y); // set horizontal speed of fireball accordingly
                writeData(Fireball_X_Speed + x, a);
                a = 0x04; // set vertical speed of fireball
                writeData(Fireball_Y_Speed + x, a);
                a = 0x07;
                writeData(Fireball_BoundBoxCtrl + x, a); // set bounding box size control for fireball
                --M(Fireball_State + x); // decrement state to 1 to skip this part from now on
            } // RunFB: add 7 to offset to use
            a = x;
            a += 0x07;
            x = a;
            a = 0x50; // set downward movement force here
            writeData(0x00, a);
            a = 0x03; // set maximum speed here
            writeData(0x02, a);
            a = 0x00;
            JSR(ImposeGravity, 179); // do sub here to impose gravity on fireball and move vertically
            JSR(MoveObjectHorizontally, 180); // do another sub to move it horizontally
            x = M(ObjectOffset); // return fireball offset to X
            JSR(RelativeFireballPosition, 181); // get relative coordinates
            JSR(GetFireballOffscreenBits, 182); // get offscreen information
            JSR(GetFireballBoundBox, 183); // get bounding box coordinates
            JSR(FireballBGCollision, 184); // do fireball to background collision detection
            a = M(FBall_OffscreenBits); // get fireball offscreen bits
            a &= 0b11001100; // mask out certain bits
            if (a == 0)
            { // if any bits still set, branch to kill fireball
                JSR(FireballEnemyCollision, 185); // do fireball to enemy collision detection and deal with collisions
                goto DrawFireball; // draw fireball appropriately and leave
            } // EraseFB: erase fireball state
            a = 0x00;
            writeData(Fireball_State + x, a);
        } // NoFBall: leave
        goto Return;

    //------------------------------------------------------------------------
    } // FireballExplosion
    JSR(RelativeFireballPosition, 186);
    goto DrawExplosion_Fireball;

BubbleCheck:
    a = M(PseudoRandomBitReg + 1 + x); // get part of LSFR
    a &= 0x01;
    writeData(0x07, a); // store pseudorandom bit here
    a = M(Bubble_Y_Position + x); // get vertical coordinate for air bubble
    if (a == 0xf8)
    { // branch to move air bubble
        a = M(AirBubbleTimer); // if air bubble timer not expired,
        if (a != 0)
            goto ExitBubl; // branch to leave, otherwise create new air bubble

SetupBubble:
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
        a = 0x01;
        writeData(Bubble_Y_HighPos + x, a); // set vertical high byte for air bubble
        y = M(0x07); // get pseudorandom bit, use as offset
        a = M(BubbleTimerData + y); // get data for air bubble timer
        writeData(AirBubbleTimer, a); // set air bubble timer
    } // MoveBubl: get pseudorandom bit again, use as offset
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

ExitBubl: // leave
    goto Return;

//------------------------------------------------------------------------

RunGameTimer:
    a = M(OperMode); // get primary mode of operation
    if (a == 0)
        goto ExGTimer; // branch to leave if in title screen mode
    a = M(GameEngineSubroutine);
    if (a < 0x08)
        goto ExGTimer; // branch to leave
    if (a == 0x0b)
        goto ExGTimer; // branch to leave
    a = M(Player_Y_HighPos);
    if (a >= 0x02)
        goto ExGTimer; // branch to leave regardless of level type
    a = M(GameTimerCtrlTimer); // if game timer control not yet expired,
    if (a != 0)
        goto ExGTimer; // branch to leave
    a = M(GameTimerDisplay);
    a |= M(GameTimerDisplay + 1); // otherwise check game timer digits
    a |= M(GameTimerDisplay + 2);
    if (a != 0)
    { // if game timer digits at 000, branch to time-up code
        y = M(GameTimerDisplay); // otherwise check first digit
        --y; // if first digit not on 1,
        if (y != 0)
            goto ResGTCtrl; // branch to reset game timer control
        a = M(GameTimerDisplay + 1); // otherwise check second and third digits
        a |= M(GameTimerDisplay + 2);
        if (a != 0)
            goto ResGTCtrl; // if timer not at 100, branch to reset game timer control
        a = TimeRunningOutMusic;
        writeData(EventMusicQueue, a); // otherwise load time running out music

ResGTCtrl: // reset game timer control
        a = 0x18;
        writeData(GameTimerCtrlTimer, a);
        y = 0x23; // set offset for last digit
        a = 0xff; // set value to decrement game timer digit
        writeData(DigitModifier + 5, a);
        JSR(DigitsMathRoutine, 187); // do sub to decrement game timer slowly
        a = 0xa4; // set status nybbles to update game timer display
        goto PrintStatusBarNumbers; // do sub to update the display
    } // TimeUpOn: init player status (note A will always be zero here)
    writeData(PlayerStatus, a);
    JSR(ForceInjury, 188); // do sub to kill the player (note player is small here)
    ++M(GameTimerExpiredFlag); // set game timer expiration flag

ExGTimer: // leave
    goto Return;

//------------------------------------------------------------------------

WarpZoneObject:
    a = M(ScrollLock); // check for scroll lock flag
    if (a == 0)
        goto ExGTimer; // branch if not set to leave
    a = M(Player_Y_Position); // check to see if player's vertical coordinate has
    a &= M(Player_Y_HighPos); // same bits set as in vertical high byte (why?)
    if (a != 0)
        goto ExGTimer; // if so, branch to leave
    writeData(ScrollLock, a); // otherwise nullify scroll lock flag
    ++M(WarpZoneControl); // increment warp zone flag to make warp pipes for warp zone
    goto EraseEnemyObject; // kill this object

ProcessWhirlpools:
    a = M(AreaType); // check for water type level
    if (a != 0)
        goto ExitWh; // branch to leave if not found
    writeData(Whirlpool_Flag, a); // otherwise initialize whirlpool flag
    a = M(TimerControl); // if master timer control set,
    if (a != 0)
        goto ExitWh; // branch to leave
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

ExitWh: // leave
        goto Return;

    //------------------------------------------------------------------------
    } // WhirlpoolActivate
    a = M(Whirlpool_Length + y); // get length of whirlpool
    a >>= 1; // divide by 2
    writeData(0x00, a); // save here
    a = M(Whirlpool_LeftExtent + y); // get left extent of whirlpool
    wide = ((M(Whirlpool_PageLoc + y) << 8) | a) + M(0x00); // add length divided by 2
    writeData(0x01, LOBYTE(wide)); // save as center of whirlpool
    writeData(0x00, HIBYTE(wide)); // save as page location of whirlpool center
    a = HIBYTE(wide); // get page location
    a = M(FrameCounter); // get frame counter
    a >>= 1; // check d0 (to run on every other frame)
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
        a = M(Player_CollisionBits);
        a >>= 1; // take d0
        if ((M(Player_CollisionBits) & 0x01) == 0)
            goto WhPull; // if d0 not set, branch
        wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + 0x01; // otherwise slowly pull player right, towards the center
        writeData(Player_X_Position, LOBYTE(wide)); // set player's new horizontal coordinate
        a = HIBYTE(wide);
    } // SetPWh: set player's new page location
    writeData(Player_PageLoc, a);

WhPull:
    a = 0x10;
    writeData(0x00, a); // set vertical movement force
    a = 0x01;
    writeData(Whirlpool_Flag, a); // set whirlpool flag to be used later
    writeData(0x02, a); // also set maximum vertical speed
    a >>= 1;
    x = a; // set X for player offset
    goto ImposeGravity; // jump to put whirlpool effect on player vertically, do not return

FlagpoleRoutine:
    x = 0x05; // set enemy object offset
    writeData(ObjectOffset, x); // to special use slot
    a = M(Enemy_ID + x);
    if (a == FlagpoleFlagObject)
    { // branch to leave
        a = M(GameEngineSubroutine);
        if (a != 0x04)
            goto SkipScore; // branch to near the end of code
        a = M(Player_State);
        if (a != 0x03)
            goto SkipScore; // branch to near the end of code
        a = M(Enemy_Y_Position + x); // check flagpole flag's vertical coordinate
        if (a >= 0xaa)
            goto GiveFPScr; // branch to end the level
        a = M(Player_Y_Position); // check player's vertical coordinate
        if (a >= 0xa2)
            goto GiveFPScr; // branch to end the level
        // position:dummy is one 16-bit quantity; the compare above left the carry clear
        wide = ((M(Enemy_Y_Position + x) << 8) | M(Enemy_YMF_Dummy + x)) + 0x01ff; // add movement amount to move flag down
        writeData(Enemy_YMF_Dummy + x, LOBYTE(wide)); // save dummy variable
        writeData(Enemy_Y_Position + x, HIBYTE(wide)); // store vertical coordinate
        wide = ((M(FlagpoleFNum_Y_Pos) << 8) | M(FlagpoleFNum_YMFDummy)) - 0x01ff; // subtract the same to move the floatey number up
        writeData(FlagpoleFNum_YMFDummy, LOBYTE(wide)); // save dummy variable
        writeData(FlagpoleFNum_Y_Pos, HIBYTE(wide)); // and store vertical coordinate here
        a = HIBYTE(wide);

SkipScore: // jump to skip ahead and draw flag and floatey number
        goto FPGfx;

GiveFPScr: // get score offset from earlier (when player touched flagpole)
        y = M(FlagpoleScore);
        a = M(FlagpoleScoreMods + y); // get amount to award player points
        x = M(FlagpoleScoreDigits + y); // get digit with which to award points
        writeData(DigitModifier + x, a); // store in digit modifier
        JSR(AddToScore, 189); // do sub to award player points depending on height of collision
        a = 0x05;
        writeData(GameEngineSubroutine, a); // set to run end-of-level subroutine on next frame

FPGfx: // get offscreen information
        JSR(GetEnemyOffscreenBits, 190);
        JSR(RelativeEnemyPosition, 191); // get relative coordinates
        JSR(FlagpoleGfxHandler, 192); // draw flagpole flag and floatey number
    } // ExitFlagP
    goto Return;

//------------------------------------------------------------------------

JumpspringHandler:
    JSR(GetEnemyOffscreenBits, 193); // get offscreen information
    a = M(TimerControl); // check master timer control
    if (a != 0)
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
    a = M(A_B_Buttons);
    a &= A_Button; // check saved controller bits for A button press
    if (a == 0)
        goto BounceJS; // skip to next part if A not pressed
    a &= M(PreviousA_B_Buttons); // check for A button pressed in previous frame
    if (a != 0)
        goto BounceJS; // skip to next part if so
    a = 0xf4;
    writeData(JumpspringForce, a); // otherwise write new jumpspring force here

BounceJS: // check frame control offset again
    if (y != 0x03)
        goto DrawJSpr; // skip to last part if not yet at fifth frame ($03)
    a = M(JumpspringForce);
    writeData(Player_Y_Speed, a); // store jumpspring force as player's new vertical speed
    a = 0x00;
    writeData(JumpspringAnimCtrl, a); // initialize jumpspring frame control

DrawJSpr: // get jumpspring's relative coordinates
    JSR(RelativeEnemyPosition, 194);
    JSR(EnemyGfxHandler, 195); // draw jumpspring
    JSR(OffscreenBoundsCheck, 196); // check to see if we need to kill it
    a = M(JumpspringAnimCtrl); // if frame control at zero, don't bother
    if (a == 0)
        goto ExJSpring; // trying to animate it, just leave
    a = M(JumpspringTimer);
    if (a != 0)
        goto ExJSpring; // if jumpspring timer not expired yet, leave
    a = 0x04;
    writeData(JumpspringTimer, a); // otherwise initialize jumpspring timer
    ++M(JumpspringAnimCtrl); // increment frame control to animate jumpspring

ExJSpring: // leave
    goto Return;

//------------------------------------------------------------------------

Setup_Vine:
    a = VineObject; // load identifier for vine object
    writeData(Enemy_ID + x, a); // store in buffer
    a = 0x01;
    writeData(Enemy_Flag + x, a); // set flag for enemy object buffer
    a = M(Block_PageLoc + y);
    writeData(Enemy_PageLoc + x, a); // copy page location from previous object
    a = M(Block_X_Position + y);
    writeData(Enemy_X_Position + x, a); // copy horizontal coordinate from previous object
    a = M(Block_Y_Position + y);
    writeData(Enemy_Y_Position + x, a); // copy vertical coordinate from previous object
    y = M(VineFlagOffset); // load vine flag/offset to next available vine slot
    if (y == 0)
    { // if set at all, don't bother to store vertical
        writeData(VineStart_Y_Position, a); // otherwise store vertical coordinate here
    } // NextVO: store object offset to next available vine slot
    a = x;
    writeData(VineObjOffset + y, a); // using vine flag as offset
    ++M(VineFlagOffset); // increment vine flag offset
    a = Sfx_GrowVine;
    writeData(Square2SoundQueue, a); // load vine grow sound
    goto Return;

//------------------------------------------------------------------------

VineObjectHandler:
    if (x != 0x05)
        goto ExitVH; // if not in last slot, branch to leave
    y = M(VineFlagOffset);
    --y; // decrement vine flag in Y, use as offset
    a = M(VineHeight);
    if (a == M(VineHeightData + y))
        goto RunVSubs; // branch ahead to skip this part
    a = M(FrameCounter); // get frame counter
    a >>= 1; // take d1
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
    JSR(RelativeEnemyPosition, 197); // get relative coordinates of vine,
    JSR(GetEnemyOffscreenBits, 198); // and any offscreen bits
    y = 0x00; // initialize offset used in draw vine sub

    do // VDrawLoop: draw vine
    {
        JSR(DrawVine, 199);
        ++y; // increment offset
    } while (y != M(VineFlagOffset)); // do not yet match, loop back to draw more vine
    a = M(Enemy_OffscreenBits);
    a &= 0b00001100; // mask offscreen bits
    if (a != 0)
    { // if none of the saved offscreen bits set, skip ahead
        --y; // otherwise decrement Y to get proper offset again

        do // KillVine: get enemy object offset for this vine object
        {
            x = M(VineObjOffset + y);
            JSR(EraseEnemyObject, 200); // kill this vine object
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
    JSR(BlockBufferCollision, 201); // do a sub to get block buffer address set, return contents
    y = M(0x02);
    if (y >= 0xd0)
        goto ExitVH; // current block buffer, branch to leave, do not write
    a = M(W(0x06) + y); // otherwise check contents of block buffer at
    if (a != 0)
        goto ExitVH; // current offset, if not empty, branch to leave
    a = 0x26;
    writeData(W(0x06) + y, a); // otherwise, write climbing metatile to block buffer

ExitVH: // get enemy object offset and leave
    x = M(ObjectOffset);
    goto Return;

//------------------------------------------------------------------------

ProcessCannons:
    a = M(AreaType); // get area type
    if (a != 0)
    { // if water type area, branch to leave
        x = 0x02;

        do // ThreeSChk: start at third enemy slot
        {
            writeData(ObjectOffset, x);
            a = M(Enemy_Flag + x); // check enemy buffer flag
            if (a != 0)
                goto Chk_BB; // if set, branch to check enemy
            a = M(PseudoRandomBitReg + 1 + x); // otherwise get part of LSFR
            y = M(SecondaryHardMode); // get secondary hard mode flag, use as offset
            a &= M(CannonBitmasks + y); // mask out bits of LSFR as decided by flag
            if (a >= 0x06)
                goto Chk_BB; // if so, branch to check enemy
            y = a; // transfer masked contents of LSFR to Y as pseudorandom offset
            a = M(Cannon_PageLoc + y); // get page location
            if (a == 0)
                goto Chk_BB; // if not set or on page 0, branch to check enemy
            a = M(Cannon_Timer + y); // get cannon timer
            if (a != 0)
            { // if expired, branch to fire cannon
                --a; // otherwise subtract the borrow (note carry will always be clear here)
                writeData(Cannon_Timer + y, a); // to count timer down
                goto Chk_BB; // then jump ahead to check enemy
            } // FireCannon
            a = M(TimerControl); // if master timer control set,
            if (a != 0)
                goto Chk_BB; // branch to check enemy
            a = 0x0e; // otherwise we start creating one
            writeData(Cannon_Timer + y, a); // first, reset cannon timer
            a = M(Cannon_PageLoc + y); // get page location of cannon
            writeData(Enemy_PageLoc + x, a); // save as page location of bullet bill
            a = M(Cannon_X_Position + y); // get horizontal coordinate of cannon
            writeData(Enemy_X_Position + x, a); // save as horizontal coordinate of bullet bill
            a = M(Cannon_Y_Position + y); // get vertical coordinate of cannon
            a -= 0x08; // subtract eight pixels (because enemies are 24 pixels tall)
            writeData(Enemy_Y_Position + x, a); // save as vertical coordinate of bullet bill
            a = 0x01;
            writeData(Enemy_Y_HighPos + x, a); // set vertical high byte of bullet bill
            writeData(Enemy_Flag + x, a); // set buffer flag
            a >>= 1; // shift right once to init A
            writeData(Enemy_State + x, a); // then initialize enemy's state
            a = 0x09;
            writeData(Enemy_BoundBoxCtrl + x, a); // set bounding box size control for bullet bill
            a = BulletBill_CannonVar;
            writeData(Enemy_ID + x, a); // load identifier for bullet bill (cannon variant)
            goto Next3Slt; // move onto next slot

Chk_BB: // check enemy identifier for bullet bill (cannon variant)
            a = M(Enemy_ID + x);
            if (a != BulletBill_CannonVar)
                goto Next3Slt; // if not found, branch to get next slot
            JSR(OffscreenBoundsCheck, 202); // otherwise, check to see if it went offscreen
            a = M(Enemy_Flag + x); // check enemy buffer flag
            if (a == 0)
                goto Next3Slt; // if not set, branch to get next slot
            JSR(GetEnemyOffscreenBits, 203); // otherwise, get offscreen information
            JSR(BulletBillHandler, 204); // then do sub to handle bullet bill

Next3Slt: // move onto next slot
            --x;
        } while ((x & 0x80) == 0); // do this until first three slots are checked
    } // ExCannon: then leave
    goto Return;

//------------------------------------------------------------------------

BulletBillHandler:
    a = M(TimerControl); // if master timer control set,
    if (a == 0)
    { // branch to run subroutines except movement sub
        a = M(Enemy_State + x);
        if (a == 0)
        { // if bullet bill's state set, branch to check defeated state
            a = M(Enemy_OffscreenBits); // otherwise load offscreen bits
            a &= 0b00001100; // mask out bits
            if (a == 0b00001100)
                goto KillBB; // if so, branch to kill this object
            y = 0x01; // set to move right by default
            JSR(PlayerEnemyDiff, 205); // get horizontal difference between player and bullet bill
            if ((a & 0x80) == 0)
            { // if enemy to the left of player, branch
                ++y; // otherwise increment to move left
            } // SetupBB: set bullet bill's moving direction
            writeData(Enemy_MovingDir + x, y);
            --y; // decrement to use as offset
            a = M(BulletBillXSpdData + y); // get horizontal speed based on moving direction
            writeData(Enemy_X_Speed + x, a); // and store it
            a = (uint8_t)(M(0x00) + 0x28 + (enemyRightOfPlayer ? 1 : 0)); // get horizontal difference, add 40 pixels plus the no-borrow left above
            if (a < 0x50)
                goto KillBB; // to cannon either on left or right side, thus branch
            a = 0x01;
            writeData(Enemy_State + x, a); // otherwise set bullet bill's state
            a = 0x0a;
            writeData(EnemyFrameTimer + x, a); // set enemy frame timer
            a = Sfx_Blast;
            writeData(Square2SoundQueue, a); // play fireworks/gunfire sound
        } // ChkDSte: check enemy state for d5 set
        a = M(Enemy_State + x);
        a &= 0b00100000;
        if (a != 0)
        { // if not set, skip to move horizontally
            JSR(MoveD_EnemyVertically, 206); // otherwise do sub to move bullet bill vertically
        } // BBFly: do sub to move bullet bill horizontally
        JSR(MoveEnemyHorizontally, 207);
    } // RunBBSubs: get offscreen information
    JSR(GetEnemyOffscreenBits, 208);
    JSR(RelativeEnemyPosition, 209); // get relative coordinates
    JSR(GetEnemyBoundBox, 210); // get bounding box coordinates
    JSR(PlayerEnemyCollision, 211); // handle player to enemy collisions
    goto EnemyGfxHandler; // draw the bullet bill and leave

KillBB: // kill bullet bill and leave
    JSR(EraseEnemyObject, 212);
    goto Return;

//------------------------------------------------------------------------

SpawnHammerObj:
    a = M(PseudoRandomBitReg + 1); // get pseudorandom bits from
    a &= 0b00000111; // second part of LSFR
    if (a == 0)
    { // if any bits are set, branch and use as offset
        a = M(PseudoRandomBitReg + 1);
        a &= 0b00001000; // get d3 from same part of LSFR
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
    a = 0x90;
    writeData(Misc_State + y, a); // save hammer's state here
    a = 0x07;
    writeData(Misc_BoundBoxCtrl + y, a); // set something else entirely, here
    hammerSpawned = true;
    goto Return;

//------------------------------------------------------------------------

NoHammer: // get original enemy object offset
    x = M(ObjectOffset);
    hammerSpawned = false;
    goto Return;

//------------------------------------------------------------------------

ProcHammerObj:
    a = M(TimerControl); // if master timer control set
    if (a != 0)
        goto RunHSubs; // skip all of this code and go to last subs at the end
    a = M(Misc_State + x); // otherwise get hammer's state
    a &= 0b01111111; // mask out d7
    y = M(HammerEnemyOffset + x); // get enemy object offset that spawned this hammer
    if (a != 0x02)
    { // if currently at 2, branch
        if (a >= 0x02)
            goto SetHPos; // if greater than 2, branch elsewhere
        a = x;
        a += 0x0d; // proper misc object
        x = a; // return offset to X
        a = 0x10;
        writeData(0x00, a); // set downward movement force
        a = 0x0f;
        writeData(0x01, a); // set upward movement force (not used)
        a = 0x04;
        writeData(0x02, a); // set maximum vertical speed
        a = 0x00; // set A to impose gravity on hammer
        JSR(ImposeGravity, 213); // do sub to impose gravity on hammer and move vertically
        JSR(MoveObjectHorizontally, 214); // do sub to move it horizontally
        x = M(ObjectOffset); // get original misc object offset
    } // SetHSpd
    else // branch to essential subroutines
    {
        a = 0xfe;
        writeData(Misc_Y_Speed + x, a); // set hammer's vertical speed
        a = M(Enemy_State + y); // get enemy object state
        a &= 0b11110111; // mask out d3
        writeData(Enemy_State + y, a); // store new state
        x = M(Enemy_MovingDir + y); // get enemy's moving direction
        --x; // decrement to use as offset
        a = M(HammerXSpdData + x); // get proper speed to use based on moving direction
        x = M(ObjectOffset); // reobtain hammer's buffer offset
        writeData(Misc_X_Speed + x, a); // set hammer's horizontal speed

SetHPos: // decrement hammer's state
        --M(Misc_State + x);
        a = M(Enemy_X_Position + y); // get enemy's horizontal position
        wide = ((M(Enemy_PageLoc + y) << 8) | a) + 0x02; // set position 2 pixels to the right
        writeData(Misc_X_Position + x, LOBYTE(wide)); // store as hammer's horizontal position
        writeData(Misc_PageLoc + x, HIBYTE(wide)); // store as hammer's page location
        a = HIBYTE(wide); // get enemy's page location
        a = M(Enemy_Y_Position + y); // get enemy's vertical position
        a -= 0x0a; // move position 10 pixels upward
        writeData(Misc_Y_Position + x, a); // store as hammer's vertical position
        a = 0x01;
        writeData(Misc_Y_HighPos + x, a); // set hammer's vertical high byte
        if (a != 0)
            goto RunHSubs; // unconditional branch to skip first routine
    } // RunAllH: handle collisions
    JSR(PlayerHammerCollision, 215);

RunHSubs: // get offscreen information
    JSR(GetMiscOffscreenBits, 216);
    JSR(RelativeMiscPosition, 217); // get relative coordinates
    JSR(GetMiscBoundBox, 218); // get bounding box coordinates
    JSR(DrawHammer, 219); // draw the hammer
    goto Return; // and we are done here

//------------------------------------------------------------------------

CoinBlock:
    JSR(FindEmptyMiscSlot, 220); // set offset for empty or last misc object buffer slot
    a = M(Block_PageLoc + x); // get page location of block object
    writeData(Misc_PageLoc + y, a); // store as page location of misc object
    a = M(Block_X_Position + x); // get horizontal coordinate of block object
    a |= 0x05; // add 5 pixels
    writeData(Misc_X_Position + y, a); // store as horizontal coordinate of misc object
    a = M(Block_Y_Position + x); // get vertical coordinate of block object
    // the jump engine reaches CoinBlock with the carry clear, so the slot search
    // above only leaves it set if it got as far as its compare
    a = (uint8_t)(a - 0x10 - (miscSlotSearched ? 0 : 1)); // subtract 16 pixels
    writeData(Misc_Y_Position + y, a); // store as vertical coordinate of misc object
    goto JCoinC; // jump to rest of code as applies to this misc object

SetupJumpCoin:
    JSR(FindEmptyMiscSlot, 221); // set offset for empty or last misc object buffer slot
    a = M(Block_PageLoc2 + x); // get page location saved earlier
    writeData(Misc_PageLoc + y, a); // and save as page location for misc object
    a = M(0x06); // get low byte of block buffer offset
    shiftedBit = (a & 0x10) != 0; // the fourth shift below carries d4 out
    a <<= 1;
    a <<= 1; // multiply by 16 to use lower nybble
    a <<= 1;
    a <<= 1;
    a |= 0x05; // add five pixels
    writeData(Misc_X_Position + y, a); // save as horizontal coordinate for misc object
    a = M(0x02); // get vertical high nybble offset from earlier
    a = (uint8_t)(a + 0x20 + (shiftedBit ? 1 : 0)); // add 32 pixels for the status bar, plus the bit shifted out above
    writeData(Misc_Y_Position + y, a); // store as vertical coordinate

JCoinC:
    a = 0xfb;
    writeData(Misc_Y_Speed + y, a); // set vertical speed
    a = 0x01;
    writeData(Misc_Y_HighPos + y, a); // set vertical high byte
    writeData(Misc_State + y, a); // set state for misc object
    writeData(Square2SoundQueue, a); // load coin grab sound
    writeData(ObjectOffset, x); // store current control bit as misc object offset
    JSR(GiveOneCoin, 222); // update coin tally on the screen and coin amount variable
    ++M(CoinTallyFor1Ups); // increment coin tally used to activate 1-up block flag
    goto Return;

//------------------------------------------------------------------------

FindEmptyMiscSlot:
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
    goto Return;

//------------------------------------------------------------------------

MiscObjectsCore:
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
            JSR(ProcHammerObj, 223); // otherwise go to process hammer,
            goto MiscLoopBack; // then check next slot
        } // ProcJumpCoin
        y = M(Misc_State + x); // check misc object state
        --y; // decrement to see if it's set to 1
        if (y != 0)
        { // if so, branch to handle jumping coin
            ++M(Misc_State + x); // otherwise increment state to either start off or as timer
            a = M(Misc_X_Position + x); // get horizontal coordinate for misc object
            wide = ((M(Misc_PageLoc + x) << 8) | a) + M(ScrollAmount); // add current scroll speed
            writeData(Misc_X_Position + x, LOBYTE(wide)); // store as new horizontal coordinate
            writeData(Misc_PageLoc + x, HIBYTE(wide)); // store as new page location
            a = HIBYTE(wide); // get page location
            a = M(Misc_State + x);
            if (a != 0x30)
                goto RunJCSubs; // if not yet reached, branch to subroutines
            a = 0x00;
            writeData(Misc_State + x, a); // otherwise nullify object state
            goto MiscLoopBack; // and move onto next slot
        } // JCoinRun
        a = x;
        a += 0x0d;
        x = a;
        a = 0x50; // set downward movement amount
        writeData(0x00, a);
        a = 0x06; // set maximum vertical speed
        writeData(0x02, a);
        a >>= 1; // divide by 2 and set
        writeData(0x01, a); // as upward movement amount (apparently residual)
        a = 0x00; // set A to impose gravity on jumping coin
        JSR(ImposeGravity, 224); // do sub to move coin vertically and impose gravity on it
        x = M(ObjectOffset); // get original misc object offset
        a = M(Misc_Y_Speed + x); // check vertical speed
        if (a != 0x05)
            goto RunJCSubs; // if not moving downward fast enough, keep state as-is
        ++M(Misc_State + x); // otherwise increment state to change to floatey number

RunJCSubs: // get relative coordinates
        JSR(RelativeMiscPosition, 225);
        JSR(GetMiscOffscreenBits, 226); // get offscreen information
        JSR(GetMiscBoundBox, 227); // get bounding box coordinates (why?)
        JSR(JCoinGfxHandler, 228); // draw the coin or floatey number

MiscLoopBack:
        --x; // decrement misc object offset
    } while ((x & 0x80) == 0); // loop back until all misc objects handled
    goto Return; // then leave

//------------------------------------------------------------------------

GiveOneCoin:
    a = 0x01; // set digit modifier to add 1 coin
    writeData(DigitModifier + 5, a); // to the current player's coin tally
    x = M(CurrentPlayer); // get current player on the screen
    y = M(CoinTallyOffsets + x); // get offset for player's coin tally
    JSR(DigitsMathRoutine, 229); // update the coin tally
    ++M(CoinTally); // increment onscreen player's coin amount
    a = M(CoinTally);
    if (a == 100)
    { // if not, skip all of this
        a = 0x00;
        writeData(CoinTally, a); // otherwise, reinitialize coin amount
        ++M(NumberofLives); // give the player an extra life
        a = Sfx_ExtraLife;
        writeData(Square2SoundQueue, a); // play 1-up sound
    } // CoinPoints
    a = 0x02; // set digit modifier to award
    writeData(DigitModifier + 4, a); // 200 points to the player

AddToScore:
    x = M(CurrentPlayer); // get current player
    y = M(ScoreOffsets + x); // get offset for player's score
    JSR(DigitsMathRoutine, 230); // update the score internally with value in digit modifier

GetSBNybbles:
    y = M(CurrentPlayer); // get current player
    a = M(StatusBarNybbles + y); // get nybbles based on player, use to update score and coins

UpdateNumber:
    JSR(PrintStatusBarNumbers, 231); // print status bar numbers based on nybbles, whatever they be
    y = M(VRAM_Buffer1_Offset);
    a = M(VRAM_Buffer1 - 6 + y); // check highest digit of score
    if (a == 0)
    { // if zero, overwrite with space tile for zero suppression
        a = 0x24;
        writeData(VRAM_Buffer1 - 6 + y, a);
    } // NoZSup: get enemy object buffer offset
    x = M(ObjectOffset);
    goto Return;

//------------------------------------------------------------------------

SetupPowerUp:
    a = PowerUpObject; // load power-up identifier into
    writeData(Enemy_ID + 5, a); // special use slot of enemy object buffer
    a = M(Block_PageLoc + x); // store page location of block object
    writeData(Enemy_PageLoc + 5, a); // as page location of power-up object
    a = M(Block_X_Position + x); // store horizontal coordinate of block object
    writeData(Enemy_X_Position + 5, a); // as horizontal coordinate of power-up object
    a = 0x01;
    writeData(Enemy_Y_HighPos + 5, a); // set vertical high byte of power-up object
    a = M(Block_Y_Position + x); // get vertical coordinate of block object
    a -= 0x08; // subtract 8 pixels
    writeData(Enemy_Y_Position + 5, a); // and use as vertical coordinate of power-up object

PwrUpJmp: // this is a residual jump point in enemy object jump table
    a = 0x01;
    writeData(Enemy_State + 5, a); // set power-up object's state
    writeData(Enemy_Flag + 5, a); // set buffer flag
    a = 0x03;
    writeData(Enemy_BoundBoxCtrl + 5, a); // set bounding box size control for power-up object
    a = M(PowerUpType);
    if (a < 0x02)
    { // if star or 1-up, branch ahead
        a = M(PlayerStatus); // otherwise check player's current status
        if (a >= 0x02)
        { // if player not fiery, use status as power-up type
            a >>= 1; // otherwise shift right to force fire flower type
        } // StrType: store type here
        writeData(PowerUpType, a);
    } // PutBehind
    a = 0b00100000;
    writeData(Enemy_SprAttrib + 5, a); // set background priority bit
    a = Sfx_GrowPowerUp;
    writeData(Square2SoundQueue, a); // load power-up reveal sound and leave
    goto Return;

//------------------------------------------------------------------------

PowerUpObjHandler:
    x = 0x05; // set object offset for last slot in enemy object buffer
    writeData(ObjectOffset, x);
    a = M(Enemy_State + 5); // check power-up object's state
    if (a == 0)
        goto ExitPUp; // if not set, branch to leave
    shiftedBit = (a & 0x80) != 0;
    if (shiftedBit) // shift to check if d7 was set in object state
    { // if not set, branch ahead to skip this part
        a = M(TimerControl); // if master timer control set,
        if (a != 0)
            goto RunPUSubs; // branch ahead to enemy object routines
        a = M(PowerUpType); // check power-up type
        if (a == 0)
            goto ShroomM; // if normal mushroom, branch ahead to move it
        if (a == 0x03)
            goto ShroomM; // if 1-up mushroom, branch ahead to move it
        if (a != 0x02)
            goto RunPUSubs; // if not star, branch elsewhere to skip movement
        JSR(MoveJumpingEnemy, 232); // otherwise impose gravity on star power-up and make it jump
        JSR(EnemyJump, 233); // note that green paratroopa shares the same code here
        goto RunPUSubs; // then jump to other power-up subroutines

ShroomM: // do sub to make mushrooms move
        JSR(MoveNormalEnemy, 234);
        JSR(EnemyToBGCollisionDet, 235); // deal with collisions
        goto RunPUSubs; // run the other subroutines
    } // GrowThePowerUp
    a = M(FrameCounter); // get frame counter
    a &= 0x03; // mask out all but 2 LSB
    if (a != 0)
        goto ChkPUSte; // if any bits set here, branch
    --M(Enemy_Y_Position + 5); // otherwise decrement vertical coordinate slowly
    a = M(Enemy_State + 5); // load power-up object state
    ++M(Enemy_State + 5); // increment state for next frame (to make power-up rise)
    if (a < 0x11)
        goto ChkPUSte; // branch ahead to last part here
    a = 0x10;
    writeData(Enemy_X_Speed + x, a); // otherwise set horizontal speed
    a = 0b10000000;
    writeData(Enemy_State + 5, a); // and then set d7 in power-up object's state
    a = 0x00; // shift once to init A
    writeData(Enemy_SprAttrib + 5, a); // initialize background priority bit set here
    a = 0x01; // rotate A to set right moving direction
    writeData(Enemy_MovingDir + x, a); // set moving direction

ChkPUSte: // check power-up object's state
    a = M(Enemy_State + 5);
    if (a < 0x06)
        goto ExitPUp; // if not, don't even bother running these routines

RunPUSubs: // get coordinates relative to screen
    JSR(RelativeEnemyPosition, 236);
    JSR(GetEnemyOffscreenBits, 237); // get offscreen bits
    JSR(GetEnemyBoundBox, 238); // get bounding box coordinates
    JSR(DrawPowerUp, 239); // draw the power-up object
    JSR(PlayerEnemyCollision, 240); // check for collision with player
    JSR(OffscreenBoundsCheck, 241); // check to see if it went offscreen

ExitPUp: // and we're done
    goto Return;

//------------------------------------------------------------------------

PlayerHeadCollision:
    pha(); // store metatile number to stack
    a = 0x11; // load unbreakable block object state by default
    x = M(SprDataOffset_Ctrl); // load offset control bit here
    y = M(PlayerSize); // check player's size
    if (y == 0)
    { // if small, branch
        a = 0x12; // otherwise load breakable block object state
    } // DBlockSte: store into block object buffer
    writeData(Block_State + x, a);
    JSR(DestroyBlockMetatile, 242); // store blank metatile in vram buffer to write to name table
    x = M(SprDataOffset_Ctrl); // load offset control bit
    a = M(0x02); // get vertical high nybble offset used in block buffer routine
    writeData(Block_Orig_YPos + x, a); // set as vertical coordinate for block object
    y = a;
    a = M(0x06); // get low byte of block buffer address used in same routine
    writeData(Block_BBuf_Low + x, a); // save as offset here to be used later
    a = M(W(0x06) + y); // get contents of block buffer at old address at $06, $07
    JSR(BlockBumpedChk, 243); // do a sub to check which block player bumped head on
    writeData(0x00, a); // store metatile here
    y = M(PlayerSize); // check player's size
    if (y == 0)
    { // if small, use metatile itself as contents of A
        a = y; // otherwise init A (note: big = 0)
    } // ChkBrick: if no match was found in previous sub, skip ahead
    if (!bumpedBlockFound)
        goto PutMTileB;
    y = 0x11; // otherwise load unbreakable state into block object buffer
    writeData(Block_State + x, y); // note this applies to both player sizes
    a = 0xc4; // load empty block metatile into A for now
    y = M(0x00); // get metatile from before
    if (y != 0x58)
    { // if so, branch
        if (y != 0x5d)
            goto PutMTileB; // if not, branch ahead to store empty block metatile
    } // StartBTmr: check brick coin timer flag
    a = M(BrickCoinTimerFlag);
    if (a == 0)
    { // if set, timer expired or counting down, thus branch
        a = 0x0b;
        writeData(BrickCoinTimer, a); // if not set, set brick coin timer
        ++M(BrickCoinTimerFlag); // and set flag linked to it
    } // ContBTmr: check brick coin timer
    a = M(BrickCoinTimer);
    if (a == 0)
    { // if not yet expired, branch to use current metatile
        y = 0xc4; // otherwise use empty block metatile
    } // PutOldMT: put metatile into A
    a = y;

PutMTileB: // store whatever metatile be appropriate here
    writeData(Block_Metatile + x, a);
    JSR(InitBlock_XY_Pos, 244); // get block object horizontal coordinates saved
    y = M(0x02); // get vertical high nybble offset
    a = 0x23;
    writeData(W(0x06) + y, a); // write blank metatile $23 to block buffer
    a = 0x10;
    writeData(BlockBounceTimer, a); // set block bounce timer
    pla(); // pull original metatile from stack
    writeData(0x05, a); // and save here
    y = 0x00; // set default offset
    a = M(CrouchingFlag); // is player crouching?
    if (a == 0)
    { // if so, branch to increment offset
        a = M(PlayerSize); // is player big?
        if (a == 0)
            goto BigBP; // if so, branch to use default offset
    } // SmallBP: increment for small or big and crouching
    ++y;

BigBP: // get player's vertical coordinate
    a = M(Player_Y_Position);
    a += M(BlockYPosAdderData + y); // add value determined by size
    a &= 0xf0; // mask out low nybble to get 16-pixel correspondence
    writeData(Block_Y_Position + x, a); // save as vertical coordinate for block object
    y = M(Block_State + x); // get block object state
    if (y != 0x11)
    { // if set to value loaded for unbreakable, branch
        JSR(BrickShatter, 245); // execute code for breakable brick
    } // Unbreak: execute code for unbreakable brick or question block
    else // skip subroutine to do last part of code here
    {
        JSR(BumpBlock, 246);
    } // InvOBit: invert control bit used by block objects
    a = M(SprDataOffset_Ctrl);
    a ^= 0x01; // and floatey numbers
    writeData(SprDataOffset_Ctrl, a);
    goto Return; // leave!

//------------------------------------------------------------------------

InitBlock_XY_Pos:
    wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + 0x08; // add eight pixels
    a = LOBYTE(wide) & 0xf0; // mask out low nybble to give 16-pixel correspondence
    writeData(Block_X_Position + x, a); // save as horizontal coordinate for block object
    a = HIBYTE(wide); // the page location of the player, plus the carry
    writeData(Block_PageLoc + x, a); // save as page location of block object
    writeData(Block_PageLoc2 + x, a); // save elsewhere to be used later
    a = M(Player_Y_HighPos);
    writeData(Block_Y_HighPos + x, a); // save vertical high byte of player into
    goto Return; // vertical high byte of block object and leave

//------------------------------------------------------------------------

BumpBlock:
    JSR(CheckTopOfBlock, 247); // check to see if there's a coin directly above this block
    a = Sfx_Bump;
    writeData(Square1SoundQueue, a); // play bump sound
    a = 0x00;
    writeData(Block_X_Speed + x, a); // initialize horizontal speed for block object
    writeData(Block_Y_MoveForce + x, a); // init fractional movement force
    writeData(Player_Y_Speed, a); // init player's vertical speed
    a = 0xfe;
    writeData(Block_Y_Speed + x, a); // set vertical speed for block object
    a = M(0x05); // get original metatile from stack
    JSR(BlockBumpedChk, 248); // do a sub to check which block player bumped head on
    if (bumpedBlockFound)
    { // if no match was found, branch to leave
        a = y; // move block number to A
        if (a >= 0x09)
        { // branch to use current number
            a -= 0x05; // otherwise subtract 5 for second set to get proper number
        } // BlockCode: run appropriate subroutine depending on block number
        switch (a)
        {
        case 0:
            goto MushFlowerBlock;
        case 1:
            goto CoinBlock;
        case 2:
            goto CoinBlock;
        case 3:
            goto ExtraLifeMushBlock;
        case 4:
            goto MushFlowerBlock;
        case 5:
            goto VineBlock;
        case 6:
            goto StarBlock;
        case 7:
            goto CoinBlock;
        case 8:
            goto ExtraLifeMushBlock;
        }

MushFlowerBlock:
        a = 0x00; // load mushroom/fire flower into power-up type
        goto Skip_4;

StarBlock:
        a = 0x02; // load star into power-up type
Skip_4:
        goto Skip_5;

ExtraLifeMushBlock:
        a = 0x03; // load 1-up mushroom into power-up type
Skip_5:
        writeData(0x39, a); // store correct power-up type
        goto SetupPowerUp;

VineBlock:
        x = 0x05; // load last slot for enemy object buffer
        y = M(SprDataOffset_Ctrl); // get control bit
        JSR(Setup_Vine, 249); // set up vine object
    } // ExitBlockChk
    goto Return; // leave

//------------------------------------------------------------------------

BlockBumpedChk:
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
    goto Return;

//------------------------------------------------------------------------

BrickShatter:
    JSR(CheckTopOfBlock, 250); // check to see if there's a coin directly above this block
    a = Sfx_BrickShatter;
    writeData(Block_RepFlag + x, a); // set flag for block object to immediately replace metatile
    writeData(NoiseSoundQueue, a); // load brick shatter sound
    JSR(SpawnBrickChunks, 251); // create brick chunk objects
    a = 0xfe;
    writeData(Player_Y_Speed, a); // set vertical speed for player
    a = 0x05;
    writeData(DigitModifier + 5, a); // set digit modifier to give player 50 points
    JSR(AddToScore, 252); // do sub to update the score
    x = M(SprDataOffset_Ctrl); // load control bit and leave
    goto Return;

//------------------------------------------------------------------------

CheckTopOfBlock:
    x = M(SprDataOffset_Ctrl); // load control bit
    y = M(0x02); // get vertical high nybble offset used in block buffer
    if (y == 0)
        goto TopEx; // branch to leave if set to zero, because we're at the top
    a = y; // otherwise set to A
    a -= 0x10; // subtract $10 to move up one row in the block buffer
    writeData(0x02, a); // store as new vertical high nybble offset
    y = a;
    a = M(W(0x06) + y); // get contents of block buffer in same column, one row up
    if (a != 0xc2)
        goto TopEx; // if not, branch to leave
    a = 0x00;
    writeData(W(0x06) + y, a); // otherwise put blank metatile where coin was
    JSR(RemoveCoin_Axe, 253); // write blank metatile to vram buffer
    x = M(SprDataOffset_Ctrl); // get control bit
    JSR(SetupJumpCoin, 254); // create jumping coin object and update coin variables

TopEx: // leave!
    goto Return;

//------------------------------------------------------------------------

SpawnBrickChunks:
    a = M(Block_X_Position + x); // set horizontal coordinate of block object
    writeData(Block_Orig_XPos + x, a); // as original horizontal coordinate here
    a = 0xf0;
    writeData(Block_X_Speed + x, a); // set horizontal speed for brick chunk objects
    writeData(Block_X_Speed + 2 + x, a);
    a = 0xfa;
    writeData(Block_Y_Speed + x, a); // set vertical speed for one
    a = 0xfc;
    writeData(Block_Y_Speed + 2 + x, a); // set lower vertical speed for the other
    a = 0x00;
    writeData(Block_Y_MoveForce + x, a); // init fractional movement force for both
    writeData(Block_Y_MoveForce + 2 + x, a);
    a = M(Block_PageLoc + x);
    writeData(Block_PageLoc + 2 + x, a); // copy page location
    a = M(Block_X_Position + x);
    writeData(Block_X_Position + 2 + x, a); // copy horizontal coordinate
    a = M(Block_Y_Position + x);
    a += 0x08; // and save as vertical coordinate for one of them
    writeData(Block_Y_Position + 2 + x, a);
    a = 0xfa;
    writeData(Block_Y_Speed + x, a); // set vertical speed...again??? (redundant)
    goto Return;

//------------------------------------------------------------------------

BlockObjectsCore:
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
        JSR(ImposeGravityBlock, 255); // do sub to impose gravity on one block object object
        JSR(MoveObjectHorizontally, 256); // do another sub to move horizontally
        a = x;
        a += 0x02;
        x = a;
        JSR(ImposeGravityBlock, 257); // do sub to impose gravity on other block object
        JSR(MoveObjectHorizontally, 258); // do another sub to move horizontally
        x = M(ObjectOffset); // get block object offset used for both
        JSR(RelativeBlockPosition, 259); // get relative coordinates
        JSR(GetBlockOffscreenBits, 260); // get offscreen information
        JSR(DrawBrickChunks, 261); // draw the brick chunks
        pla(); // get lower nybble of saved state
        y = M(Block_Y_HighPos + x); // check vertical high byte of block object
        if (y == 0)
            goto UpdSte; // if above the screen, branch to kill it
        pha(); // otherwise save state back into stack
        a = 0xf0;
        if (a < M(Block_Y_Position + 2 + x))
        { // to the bottom of the screen, and branch if not
            writeData(Block_Y_Position + 2 + x, a); // otherwise set offscreen coordinate
        } // ChkTop: get top block object's vertical coordinate
        a = M(Block_Y_Position + x);
        pla(); // pull block object state from stack
        if (M(Block_Y_Position + x) < 0xf0)
            goto UpdSte; // if not, branch to save state
        if (M(Block_Y_Position + x) >= 0xf0)
            goto KillBlock; // otherwise do unconditional branch to kill it
    } // BouncingBlockHandler
    JSR(ImposeGravityBlock, 262); // do sub to impose gravity on block object
    x = M(ObjectOffset); // get block object offset
    JSR(RelativeBlockPosition, 263); // get relative coordinates
    JSR(GetBlockOffscreenBits, 264); // get offscreen information
    JSR(DrawBlock, 265); // draw the block
    a = M(Block_Y_Position + x); // get vertical coordinate
    a &= 0x0f; // mask out high nybble
    pla(); // pull state from stack
    if ((M(Block_Y_Position + x) & 0x0f) >= 0x05)
        goto UpdSte; // if still above amount, not time to kill block yet, thus branch
    a = 0x01;
    writeData(Block_RepFlag + x, a); // otherwise set flag to replace metatile

KillBlock: // if branched here, nullify object state
    a = 0x00;

UpdSte: // store contents of A in block object state
    writeData(Block_State + x, a);
    goto Return;

//------------------------------------------------------------------------

BlockObjMT_Updater:
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
        a = M(Block_BBuf_Low + x); // get low byte of block buffer
        writeData(0x06, a); // store into block buffer address
        a = 0x05;
        writeData(0x07, a); // set high byte of block buffer address
        a = M(Block_Orig_YPos + x); // get original vertical coordinate of block object
        writeData(0x02, a); // store here and use as offset to block buffer
        y = a;
        a = M(Block_Metatile + x); // get metatile to be written
        writeData(W(0x06) + y, a); // write it to the block buffer
        JSR(ReplaceBlockMetatile, 266); // do sub to replace metatile where block object is
        a = 0x00;
        writeData(Block_RepFlag + x, a); // clear block object flag

NextBUpd: // decrement block object offset
        --x;
    } while ((x & 0x80) == 0); // do this until both block objects are dealt with
    goto Return; // then leave

//------------------------------------------------------------------------

MoveEnemyHorizontally:
    ++x; // increment offset for enemy offset
    JSR(MoveObjectHorizontally, 267); // position object horizontally according to
    x = M(ObjectOffset); // counters, return with saved value in A,
    goto Return; // put enemy offset back in X and leave

//------------------------------------------------------------------------

MovePlayerHorizontally:
    a = M(JumpspringAnimCtrl); // if jumpspring currently animating,
    if (a != 0)
        goto ExXMove; // branch to leave
    x = a; // otherwise set zero for offset to use player's stuff

MoveObjectHorizontally:
    a = M(SprObject_X_Speed + x); // get currently saved value (horizontal
    a <<= 1; // speed, secondary counter, whatever)
    a <<= 1; // and move low nybble to high
    a <<= 1;
    a <<= 1;
    writeData(0x01, a); // store result here
    a = M(SprObject_X_Speed + x); // get saved value again
    a >>= 1; // move high nybble to low
    a >>= 1;
    a >>= 1;
    a >>= 1;
    if (a >= 0x08)
    {
        a |= 0b11110000; // otherwise alter high nybble
    } // SaveXSpd: save result here
    writeData(0x00, a);
    y = 0x00; // load default Y value here
    if ((a & 0x80) != 0)
    {
        --y; // otherwise decrement Y
    } // UseAdder: save Y here
    writeData(0x02, y);
    wide = M(SprObject_X_MoveForce + x) + M(0x01); // add low nybble moved to high
    writeData(SprObject_X_MoveForce + x, LOBYTE(wide)); // store result here
    carry = HIBYTE(wide) != 0; // the original saves this carry on the stack for the end
    // pageloc:position is one 16-bit quantity, and $02:$00 the signed 16-bit amount
    // to move the object by (the high nybble moved to low, plus $f0 if necessary)
    wide = ((M(SprObject_PageLoc + x) << 8) | M(SprObject_X_Position + x))
         + ((M(0x02) << 8) | M(0x00)) + (carry ? 1 : 0);
    writeData(SprObject_X_Position + x, LOBYTE(wide)); // to object's horizontal position
    writeData(SprObject_PageLoc + x, HIBYTE(wide)); // and the object's page location and save
    a = (uint8_t)((carry ? 1 : 0) + M(0x00)); // add the old carry to the high nybble moved to low

ExXMove: // and leave
    goto Return;

//------------------------------------------------------------------------

MovePlayerVertically:
    x = 0x00; // set X for player offset
    a = M(TimerControl);
    if (a == 0)
    { // if master timer control set, branch ahead
        a = M(JumpspringAnimCtrl); // otherwise check to see if jumpspring is animating
        if (a != 0)
            goto ExXMove; // branch to leave if so
    } // NoJSChk: dump vertical force
    a = M(VerticalForce);
    writeData(0x00, a);
    a = 0x04; // set maximum vertical speed here
    goto ImposeGravitySprObj; // then jump to move player vertically

MoveD_EnemyVertically:
    y = 0x3d; // set quick movement amount downwards
    a = M(Enemy_State + x); // then check enemy state
    if (a == 0x05)
    { // and use, otherwise set different movement amount, continue on

MoveFallingPlatform:
        y = 0x20; // set movement amount
    } // ContVMove: jump to skip the rest of this
    goto SetHiMax;

MoveRedPTroopaDown:
    y = 0x00; // set Y to move downwards
    goto MoveRedPTroopa; // skip to movement routine

MoveRedPTroopaUp:
    y = 0x01; // set Y to move upwards

MoveRedPTroopa:
    ++x; // increment X for enemy offset
    a = 0x03;
    writeData(0x00, a); // set downward movement amount here
    a = 0x06;
    writeData(0x01, a); // set upward movement amount here
    a = 0x02;
    writeData(0x02, a); // set maximum speed here
    a = y; // set movement direction in A, and
    goto RedPTroopaGrav; // jump to move this thing

MoveDropPlatform:
    y = 0x7f; // set movement amount for drop platform
    if (y == 0)
    { // skip ahead of other value set here

MoveEnemySlowVert:
        y = 0x0f; // set movement amount for bowser/other objects
    } // SetMdMax: set maximum speed in A
    a = 0x02;
    if (a != 0)
        goto SetXMoveAmt; // unconditional branch

MoveJ_EnemyVertically:
    y = 0x1c; // set movement amount for podoboo/other objects

SetHiMax: // set maximum speed in A
    a = 0x03;

SetXMoveAmt: // set movement amount here
    writeData(0x00, y);
    ++x; // increment X for enemy offset
    JSR(ImposeGravitySprObj, 268); // do a sub to move enemy object downwards
    x = M(ObjectOffset); // get enemy object buffer offset and leave
    goto Return;

//------------------------------------------------------------------------

ResidualGravityCode:
    y = 0x00; // this part appears to be residual,
    goto Skip_6;

ImposeGravityBlock:
    y = 0x01; // set offset for maximum speed
Skip_6:
    a = 0x50; // set movement amount here
    writeData(0x00, a);
    a = M(MaxSpdBlockData + y); // get maximum speed

ImposeGravitySprObj:
    writeData(0x02, a); // set maximum speed here
    a = 0x00; // set value to move downwards
    goto ImposeGravity; // jump to the code that actually moves it

MovePlatformDown:
    a = 0x00; // save value to stack (if branching here, execute next
    goto Skip_7;

MovePlatformUp:
    a = 0x01; // save value to stack
Skip_7:
    pha();
    y = M(Enemy_ID + x); // get enemy object identifier
    ++x; // increment offset for enemy object
    a = 0x05; // load default value here
    if (y == 0x29)
    { // this code, thus unconditional branch here
        a = 0x09; // residual code
    } // SetDplSpd: save downward movement amount here
    writeData(0x00, a);
    a = 0x0a; // save upward movement amount here
    writeData(0x01, a);
    a = 0x03; // save maximum vertical speed here
    writeData(0x02, a);
    pla(); // get value from stack
    y = a; // use as Y, then move onto code shared by red koopa

RedPTroopaGrav:
    JSR(ImposeGravity, 269); // do a sub to move object gradually
    x = M(ObjectOffset); // get enemy object offset and leave
    goto Return;

//------------------------------------------------------------------------

ImposeGravity:
    pha(); // push value to stack
    y = 0x00; // set Y to zero by default
    a = M(SprObject_Y_Speed + x); // get current vertical speed
    if ((a & 0x80) != 0)
    { // if currently moving downwards, do not decrement Y
        --y; // otherwise decrement Y
    } // AlterYP: store Y here
    writeData(0x07, y);
    // highpos:position:dummy is one 24-bit quantity, and $07:speed:force the
    // signed 24-bit amount to move the object by
    wide = (M(SprObject_Y_HighPos + x) << 16) | (M(SprObject_Y_Position + x) << 8) | M(SprObject_YMF_Dummy + x);
    wide += (M(0x07) << 16) | (M(SprObject_Y_Speed + x) << 8) | M(SprObject_Y_MoveForce + x);
    writeData(SprObject_YMF_Dummy + x, LOBYTE(wide)); // add value in movement force to contents of dummy variable
    writeData(SprObject_Y_Position + x, HIBYTE(wide)); // store as new vertical position
    writeData(SprObject_Y_HighPos + x, (uint8_t)(wide >> 16)); // store as new vertical high byte
    a = (uint8_t)(wide >> 16);
    a = M(SprObject_Y_MoveForce + x);
    wide = ((M(SprObject_Y_Speed + x) << 8) | a) + M(0x00); // add downward movement amount to contents of $0433
    writeData(SprObject_Y_MoveForce + x, LOBYTE(wide));
    writeData(SprObject_Y_Speed + x, HIBYTE(wide));
    a = HIBYTE(wide); // add carry to vertical speed and store
    if (((a - M(0x02)) & 0x80) != 0)
        goto ChkUpM; // if less than preset value, skip this part
    a = M(SprObject_Y_MoveForce + x);
    if (a < 0x80)
        goto ChkUpM;
    a = M(0x02);
    writeData(SprObject_Y_Speed + x, a); // keep vertical speed within maximum value
    a = 0x00;
    writeData(SprObject_Y_MoveForce + x, a); // clear fractional

ChkUpM: // get value from stack
    pla();
    if (a == 0)
        goto ExVMove; // if set to zero, branch to leave
    a = M(0x02);
    a ^= 0b11111111; // otherwise get two's compliment of maximum speed
    y = a;
    ++y;
    writeData(0x07, y); // store two's compliment here
    a = M(SprObject_Y_MoveForce + x);
    wide = ((M(SprObject_Y_Speed + x) << 8) | a) - M(0x01); // of movement force, note that $01 is twice as large as $00,
    writeData(SprObject_Y_MoveForce + x, LOBYTE(wide)); // thus it effectively undoes add we did earlier
    writeData(SprObject_Y_Speed + x, HIBYTE(wide));
    a = HIBYTE(wide);
    if (((a - M(0x07)) & 0x80) == 0)
        goto ExVMove; // if less negatively than preset maximum, skip this part
    a = M(SprObject_Y_MoveForce + x);
    if (a >= 0x80)
        goto ExVMove; // and if so, branch to leave
    a = M(0x07);
    writeData(SprObject_Y_Speed + x, a); // keep vertical speed within maximum value
    a = 0xff;
    writeData(SprObject_Y_MoveForce + x, a); // clear fractional

ExVMove: // leave!
    goto Return;

//------------------------------------------------------------------------

EnemiesAndLoopsCore:
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
        a = M(AreaParserTaskNum);
        a &= 0x07;
        if (a == 0x07)
            goto ExitELCore;
        goto ProcLoopCommand; // otherwise, jump to process loop command/load enemies
    } // ChkBowserF: get data from stack
    pla();
    a &= 0b00001111; // mask out high nybble
    y = a;
    a = M(Enemy_Flag + y); // use as pointer and load same place with different offset
    if (a != 0)
        goto ExitELCore;
    writeData(Enemy_Flag + x, a); // if second enemy flag not set, also clear first one

ExitELCore:
    goto Return;

//------------------------------------------------------------------------

ExecGameLoopback:
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
    a = 0x00; // initialize page select for both
    writeData(EnemyObjectPageSel, a); // area and enemy objects
    writeData(AreaObjectPageSel, a);
    writeData(EnemyDataOffset, a); // initialize enemy object data offset
    writeData(EnemyObjectPageLoc, a); // and enemy object page control
    a = M(AreaDataOfsLoopback + y); // adjust area object offset based on
    writeData(AreaDataOffset, a); // which loop command we encountered
    goto Return;

//------------------------------------------------------------------------

ProcLoopCommand:
    a = M(LoopCommand); // check if loop command was found
    if (a == 0)
        goto ChkEnemyFrenzy;
    a = M(CurrentColumnPos); // check to see if we're still on the first page
    if (a != 0)
        goto ChkEnemyFrenzy; // if not, do not loop yet
    y = 0x0b; // start at the end of each set of loop data

FindLoop:
    --y;
    if ((y & 0x80) != 0)
        goto ChkEnemyFrenzy; // if all data is checked and not match, do not loop
    a = M(WorldNumber); // check to see if one of the world numbers
    if (a != M(LoopCmdWorldNumber + y))
        goto FindLoop;
    a = M(CurrentPageLoc); // check to see if one of the page numbers
    if (a != M(LoopCmdPageNumber + y))
        goto FindLoop;
    a = M(Player_Y_Position); // check to see if the player is at the correct position
    if (a != M(LoopCmdYPosition + y))
        goto WrongChk;
    a = M(Player_State); // check to see if the player is
    if (a != 0x00)
        goto WrongChk; // if not, player fails to pass loop, and loopback
    a = M(WorldNumber); // are we in world 7? (check performed on correct
    if (a != World7)
        goto InitMLp; // if not, initialize flags used there, otherwise
    ++M(MultiLoopCorrectCntr); // increment counter for correct progression

IncMLoop: // increment master multi-part counter
    ++M(MultiLoopPassCntr);
    a = M(MultiLoopPassCntr); // have we done all three parts?
    if (a == 0x03)
    { // if not, skip this part
        a = M(MultiLoopCorrectCntr); // if so, have we done them all correctly?
        if (a == 0x03)
            goto InitMLp; // if so, branch past unnecessary check here
        if (a == 0x03)
        { // unconditional branch if previous branch fails

WrongChk: // are we in world 7? (check performed on
            a = M(WorldNumber);
            if (a == World7)
                goto IncMLoop;
        } // DoLpBack: if player is not in right place, loop back
        JSR(ExecGameLoopback, 270);
        JSR(KillAllEnemies, 271);

InitMLp: // initialize counters used for multi-part loop commands
        a = 0x00;
        writeData(MultiLoopPassCntr, a);
        writeData(MultiLoopCorrectCntr, a);
    } // InitLCmd: initialize loop command flag
    a = 0x00;
    writeData(LoopCommand, a);

ChkEnemyFrenzy:
    a = M(EnemyFrenzyQueue); // check for enemy object in frenzy queue
    if (a != 0)
    { // if not, skip this part
        writeData(Enemy_ID + x, a); // store as enemy object identifier here
        a = 0x01;
        writeData(Enemy_Flag + x, a); // activate enemy object flag
        a = 0x00;
        writeData(Enemy_State + x, a); // initialize state and frenzy queue
        writeData(EnemyFrenzyQueue, a);
        goto InitEnemyObject; // and then jump to deal with this enemy
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
    a = M(W(EnemyData) + y); // check for specific value here
    a &= 0b00111111; // not sure what this was intended for, exactly
    if (a == 0x2e)
        goto CheckRightBounds; // but it has the effect of keeping enemies out of
    goto Return; // the sixth slot

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
    a = M(EnemyObjectPageSel); // if page select already set, do not set again
    if (a != 0)
        goto CheckPageCtrlRow;
    ++M(EnemyObjectPageSel); // otherwise, if MSB is set, set page select
    ++M(EnemyObjectPageLoc); // and increment page control

CheckPageCtrlRow:
    --y;
    a = M(W(EnemyData) + y); // reread first byte
    a &= 0x0f;
    if (a != 0x0f)
        goto PositionEnemyObj; // if not found, branch to position enemy object
    a = M(EnemyObjectPageSel); // if page select set,
    if (a != 0)
        goto PositionEnemyObj; // branch without reading second byte
    ++y;
    a = M(W(EnemyData) + y); // otherwise, get second byte, mask out 2 MSB
    a &= 0b00111111;
    writeData(EnemyObjectPageLoc, a); // store as page control for enemy object data
    ++M(EnemyDataOffset); // increment enemy object data offset 2 bytes
    ++M(EnemyDataOffset);
    ++M(EnemyObjectPageSel); // set page select for enemy object data and
    goto ProcLoopCommand; // jump back to process loop commands again

PositionEnemyObj:
    a = M(EnemyObjectPageLoc); // store page control as page location
    writeData(Enemy_PageLoc + x, a); // for enemy object
    a = M(W(EnemyData) + y); // get first byte of enemy object
    a &= 0b11110000;
    writeData(Enemy_X_Position + x, a); // store column position
    if (((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x)) // check column position against right boundary
        < ((M(ScreenRight_PageLoc) << 8) | M(ScreenRight_X_Pos)))
    { // if enemy object beyond or at boundary, branch
        a = M(W(EnemyData) + y);
        a &= 0b00001111; // check for special row $0e
        if (a == 0x0e)
            goto ParseRow0e;
    } // CheckRightExtBounds
    else // if not found, unconditional jump
    {
        if (((M(0x06) << 8) | M(0x07)) // check right boundary + 48 against the column position
            < ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x)))
            goto CheckFrenzyBuffer; // if enemy object beyond extended boundary, branch
        a = 0x01; // store value in vertical high byte
        writeData(Enemy_Y_HighPos + x, a);
        a = M(W(EnemyData) + y); // get first byte again
        a <<= 1; // multiply by four to get the vertical
        a <<= 1; // coordinate
        a <<= 1;
        a <<= 1;
        writeData(Enemy_Y_Position + x, a);
        if (a == 0xe0)
            goto ParseRow0e; // (necessary if branched to $c1cb)
        ++y;
        a = M(W(EnemyData) + y); // get second byte of object
        a &= 0b01000000; // check to see if hard mode bit is set
        if (a != 0)
        { // if not, branch to check for group enemy objects
            a = M(SecondaryHardMode); // if set, check to see if secondary hard mode flag
            if (a == 0)
                goto Inc2B; // is on, and if not, branch to skip this object completely
        } // CheckForEnemyGroup
        a = M(W(EnemyData) + y); // get second byte and mask out 2 MSB
        a &= 0b00111111;
        if (a >= 0x37)
        {
            if (a < 0x3f)
                goto DoGroup; // below $3f, branch if below $3f
        } // BuzzyBeetleMutate
        if (a != Goomba)
            goto StrID; // value ($3f or more always fails)
        y = M(PrimaryHardMode); // check if primary hard mode flag is set
        if (y == 0)
            goto StrID; // and if so, change goomba to buzzy beetle
        a = BuzzyBeetle;

StrID: // store enemy object number into buffer
        writeData(Enemy_ID + x, a);
        a = 0x01;
        writeData(Enemy_Flag + x, a); // set flag for enemy in buffer
        JSR(InitEnemyObject, 272);
        a = M(Enemy_Flag + x); // check to see if flag is set
        if (a != 0)
            goto Inc2B; // if not, leave, otherwise branch
        goto Return;

    //------------------------------------------------------------------------

CheckFrenzyBuffer:
        a = M(EnemyFrenzyBuffer); // if enemy object stored in frenzy buffer
        if (a == 0)
        { // then branch ahead to store in enemy object buffer
            a = M(VineFlagOffset); // otherwise check vine flag offset
            if (a != 0x01)
                goto ExEPar; // if other value <> 1, leave
            a = VineObject; // otherwise put vine in enemy identifier
        } // StrFre: store contents of frenzy buffer into enemy identifier value
        writeData(Enemy_ID + x, a);

InitEnemyObject:
        a = 0x00; // initialize enemy state
        writeData(Enemy_State + x, a);
        JSR(CheckpointEnemyID, 273); // jump ahead to run jump engine and subroutines

ExEPar: // then leave
        goto Return;

    //------------------------------------------------------------------------

DoGroup:
        goto HandleGroupEnemies; // handle enemy group objects

ParseRow0e:
        ++y; // increment Y to load third byte of object
        ++y;
        a = M(W(EnemyData) + y);
        a >>= 1; // move 3 MSB to the bottom, effectively
        a >>= 1; // making %xxx00000 into %00000xxx
        a >>= 1;
        a >>= 1;
        a >>= 1;
        if (a == M(WorldNumber))
        { // if not, do not use (this allows multiple uses
            --y; // of the same area, like the underground bonus areas)
            a = M(W(EnemyData) + y); // otherwise, get second byte and use as offset
            writeData(AreaPointer, a); // to addresses for level and enemy object data
            ++y;
            a = M(W(EnemyData) + y); // get third byte again, and this time mask out
            a &= 0b00011111; // the 3 MSB from before, save as page number to be
            writeData(EntrancePage, a); // used upon entry to area, if area is entered
        } // NotUse
        goto Inc3B;
    } // CheckThreeBytes
    y = M(EnemyDataOffset); // load current offset for enemy object data
    a = M(W(EnemyData) + y); // get first byte
    a &= 0b00001111; // check for special row $0e
    if (a != 0x0e)
        goto Inc2B;

Inc3B: // if row = $0e, increment three bytes
    ++M(EnemyDataOffset);

Inc2B: // otherwise increment two bytes
    ++M(EnemyDataOffset);
    ++M(EnemyDataOffset);
    a = 0x00; // init page select for enemy objects
    writeData(EnemyObjectPageSel, a);
    x = M(ObjectOffset); // reload current offset in enemy buffers
    goto Return; // and leave

//------------------------------------------------------------------------

CheckpointEnemyID:
    a = M(Enemy_ID + x);
    if (a < 0x15)
    { // and branch straight to the jump engine if found
        y = a; // save identifier in Y register for now
        a = M(Enemy_Y_Position + x);
        a += 0x08; // add eight pixels to what will eventually be the
        writeData(Enemy_Y_Position + x, a); // enemy object's vertical coordinate ($00-$14 only)
        a = 0x01;
        writeData(EnemyOffscrBitsMasked + x, a); // set offscreen masked bit
        a = y; // get identifier back and use as offset for jump engine
    } // InitEnemyRoutines
    y = a * 2 + 2;
    switch (a)
    {
    case 0:
        goto InitNormalEnemy; // for objects $00-$0f
    case 1:
        goto InitNormalEnemy;
    case 2:
        goto InitNormalEnemy;
    case 3:
        goto InitRedKoopa;
    case 4:
        goto NoInitCode;
    case 5:
        goto InitHammerBro;
    case 6:
        goto InitGoomba;
    case 7:
        goto InitBloober;
    case 8:
        goto InitBulletBill;
    case 9:
        goto NoInitCode;
    case 10:
        goto InitCheepCheep;
    case 11:
        goto InitCheepCheep;
    case 12:
        goto InitPodoboo;
    case 13:
        goto InitPiranhaPlant;
    case 14:
        goto InitJumpGPTroopa;
    case 15:
        goto InitRedPTroopa;
    case 16:
        goto InitHorizFlySwimEnemy; // for objects $10-$1f
    case 17:
        goto InitLakitu;
    case 18:
        goto InitEnemyFrenzy;
    case 19:
        goto NoInitCode;
    case 20:
        goto InitEnemyFrenzy;
    case 21:
        goto InitEnemyFrenzy;
    case 22:
        goto InitEnemyFrenzy;
    case 23:
        goto InitEnemyFrenzy;
    case 24:
        goto EndFrenzy;
    case 25:
        goto NoInitCode;
    case 26:
        goto NoInitCode;
    case 27:
        goto InitShortFirebar;
    case 28:
        goto InitShortFirebar;
    case 29:
        goto InitShortFirebar;
    case 30:
        goto InitShortFirebar;
    case 31:
        goto InitLongFirebar;
    case 32:
        goto NoInitCode; // for objects $20-$2f
    case 33:
        goto NoInitCode;
    case 34:
        goto NoInitCode;
    case 35:
        goto NoInitCode;
    case 36:
        goto InitBalPlatform;
    case 37:
        goto InitVertPlatform;
    case 38:
        goto LargeLiftUp;
    case 39:
        goto LargeLiftDown;
    case 40:
        goto InitHoriPlatform;
    case 41:
        goto InitDropPlatform;
    case 42:
        goto InitHoriPlatform;
    case 43:
        goto PlatLiftUp;
    case 44:
        goto PlatLiftDown;
    case 45:
        goto InitBowser;
    case 46:
        goto PwrUpJmp; // possibly dummy value
    case 47:
        goto Setup_Vine;
    case 48:
        goto NoInitCode; // for objects $30-$36
    case 49:
        goto NoInitCode;
    case 50:
        goto NoInitCode;
    case 51:
        goto NoInitCode;
    case 52:
        goto NoInitCode;
    case 53:
        goto InitRetainerObj;
    case 54:
        goto EndOfEnemyInitCode;
    }

NoInitCode:
    goto Return; // this executed when enemy object has no init code

//------------------------------------------------------------------------

InitGoomba:
    JSR(InitNormalEnemy, 274); // set appropriate horizontal speed
    goto SmallBBox; // set $09 as bounding box control, set other values

InitPodoboo:
    a = 0x02; // set enemy position to below
    writeData(Enemy_Y_HighPos + x, a); // the bottom of the screen
    writeData(Enemy_Y_Position + x, a);
    a >>= 1;
    writeData(EnemyIntervalTimer + x, a); // set timer for enemy
    a >>= 1;
    writeData(Enemy_State + x, a); // initialize enemy state, then jump to use
    goto SmallBBox; // $09 as bounding box size and set other things

InitRetainerObj:
    a = 0xb8; // set fixed vertical position for
    writeData(Enemy_Y_Position + x, a); // princess/mushroom retainer object
    goto Return;

//------------------------------------------------------------------------

InitNormalEnemy:
    y = 0x01; // load offset of 1 by default
    a = M(PrimaryHardMode); // check for primary hard mode flag set
    if (a == 0)
    {
        --y; // if not set, decrement offset
    } // GetESpd: get appropriate horizontal speed
    a = M(NormalXSpdData + y);

SetESpd: // store as speed for enemy object
    writeData(Enemy_X_Speed + x, a);
    goto TallBBox; // branch to set bounding box control and other data

InitRedKoopa:
    JSR(InitNormalEnemy, 275); // load appropriate horizontal speed
    a = 0x01; // set enemy state for red koopa troopa $03
    writeData(Enemy_State + x, a);
    goto Return;

//------------------------------------------------------------------------

InitHammerBro:
    a = 0x00; // init horizontal speed and timer used by hammer bro
    writeData(HammerThrowingTimer + x, a); // apparently to time hammer throwing
    writeData(Enemy_X_Speed + x, a);
    y = M(SecondaryHardMode); // get secondary hard mode flag
    a = M(HBroWalkingTimerData + y);
    writeData(EnemyIntervalTimer + x, a); // set value as delay for hammer bro to walk left
    a = 0x0b; // set specific value for bounding box size control
    goto SetBBox;

InitHorizFlySwimEnemy:
    a = 0x00; // initialize horizontal speed
    goto SetESpd;

InitBloober:
    a = 0x00; // initialize horizontal speed
    writeData(BlooperMoveSpeed + x, a);

SmallBBox: // set specific bounding box size control
    a = 0x09;
    if (a != 0)
        goto SetBBox; // unconditional branch

InitRedPTroopa:
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

TallBBox: // set specific bounding box size control
    a = 0x03;

SetBBox: // set bounding box control here
    writeData(Enemy_BoundBoxCtrl + x, a);
    a = 0x02; // set moving direction for left
    writeData(Enemy_MovingDir + x, a);

InitVStf: // initialize vertical speed
    a = 0x00;
    writeData(Enemy_Y_Speed + x, a); // and movement force
    writeData(Enemy_Y_MoveForce + x, a);
    goto Return;

//------------------------------------------------------------------------

InitBulletBill:
    a = 0x02; // set moving direction for left
    writeData(Enemy_MovingDir + x, a);
    a = 0x09; // set bounding box control for $09
    writeData(Enemy_BoundBoxCtrl + x, a);
    goto Return;

//------------------------------------------------------------------------

InitCheepCheep:
    JSR(SmallBBox, 276); // set vertical bounding box, speed, init others
    a = M(PseudoRandomBitReg + x); // check one portion of LSFR
    a &= 0b00010000; // get d4 from it
    writeData(CheepCheepMoveMFlag + x, a); // save as movement flag of some sort
    a = M(Enemy_Y_Position + x);
    writeData(CheepCheepOrigYPos + x, a); // save original vertical coordinate here
    goto Return;

//------------------------------------------------------------------------

InitLakitu:
    a = M(EnemyFrenzyBuffer); // check to see if an enemy is already in
    if (a == 0)
    { // the frenzy buffer, and branch to kill lakitu if so

SetupLakitu:
        a = 0x00; // erase counter for lakitu's reappearance
        writeData(LakituReappearTimer, a);
        JSR(InitHorizFlySwimEnemy, 277); // set $03 as bounding box, set other attributes
        goto TallBBox2; // set $03 as bounding box again (not necessary) and leave
    } // KillLakitu
    goto EraseEnemyObject;

LakituAndSpinyHandler:
    a = M(FrenzyEnemyTimer); // if timer here not expired, leave
    if (a != 0)
        goto ExLSHand;
    if (x >= 0x05)
        goto ExLSHand;
    a = 0x80; // set timer
    writeData(FrenzyEnemyTimer, a);
    y = 0x04; // start with the last enemy slot

ChkLak: // check all enemy slots to see
    a = M(Enemy_ID + y);
    if (a != Lakitu)
    { // if so, branch out of this loop
        --y; // otherwise check another slot
        if ((y & 0x80) == 0)
            goto ChkLak; // loop until all slots are checked
        ++M(LakituReappearTimer); // increment reappearance timer
        a = M(LakituReappearTimer);
        if (a < 0x07)
            goto ExLSHand; // if not, leave
        x = 0x04; // start with the last enemy slot again

ChkNoEn: // check enemy buffer flag for non-active enemy slot
        a = M(Enemy_Flag + x);
        if (a != 0)
        { // branch out of loop if found
            --x; // otherwise check next slot
            if ((x & 0x80) == 0)
                goto ChkNoEn; // branch until all slots are checked
            if ((x & 0x80) != 0)
                goto RetEOfs; // if no empty slots were found, branch to leave
        } // CreateL: initialize enemy state
        a = 0x00;
        writeData(Enemy_State + x, a);
        a = Lakitu; // create lakitu enemy object
        writeData(Enemy_ID + x, a);
        JSR(SetupLakitu, 278); // do a sub to set up lakitu
        a = 0x20;
        JSR(PutAtRightExtent, 279); // finish setting up lakitu

RetEOfs: // get enemy object buffer offset again and leave
        x = M(ObjectOffset);

ExLSHand:
        goto Return;

    //------------------------------------------------------------------------
    } // CreateSpiny
    a = M(Player_Y_Position); // if player above a certain point, branch to leave
    if (a < 0x2c)
        goto ExLSHand;
    a = M(Enemy_State + y); // if lakitu is not in normal state, branch to leave
    if (a != 0)
        goto ExLSHand;
    a = M(Enemy_PageLoc + y); // store horizontal coordinates (high and low) of lakitu
    writeData(Enemy_PageLoc + x, a); // into the coordinates of the spiny we're going to create
    a = M(Enemy_X_Position + y);
    writeData(Enemy_X_Position + x, a);
    a = 0x01; // put spiny within vertical screen unit
    writeData(Enemy_Y_HighPos + x, a);
    a = M(Enemy_Y_Position + y); // put spiny eight pixels above where lakitu is
    a -= 0x08;
    writeData(Enemy_Y_Position + x, a);
    a = M(PseudoRandomBitReg + x); // get 2 LSB of LSFR and save to Y
    a &= 0b00000011;
    y = a;
    x = 0x02;

    do // DifLoop: get three values and save them
    {
        a = M(PRDiffAdjustData + y);
        writeData(0x01 + x, a); // to $01-$03
        ++y;
        ++y; // increment Y four bytes for each value
        ++y;
        ++y;
        --x; // decrement X for each one
    } while ((x & 0x80) == 0); // loop until all three are written
    x = M(ObjectOffset); // get enemy object buffer offset
    JSR(PlayerLakituDiff, 280); // move enemy, change direction, get value - difference
    y = M(Player_X_Speed); // check player's horizontal speed
    if (y < 0x08)
    { // if moving faster than a certain amount, branch elsewhere
        y = a; // otherwise save value in A to Y for now
        a = M(PseudoRandomBitReg + 1 + x);
        a &= 0b00000011; // get one of the LSFR parts and save the 2 LSB
        if (a != 0)
        { // branch if neither bits are set
            a = y;
            a ^= 0b11111111; // otherwise get two's compliment of Y
            y = a;
            ++y;
        } // UsePosv: put value from A in Y back to A (they will be lost anyway)
        a = y;
    } // SetSpSpd: set bounding box control, init attributes, lose contents of A
    JSR(SmallBBox, 281);
    y = 0x02;
    writeData(Enemy_X_Speed + x, a); // set horizontal speed to zero because previous contents
    if ((a & 0x80) == 0)
    { // the same reason
        --y;
    } // SpinyRte: set moving direction to the right
    writeData(Enemy_MovingDir + x, y);
    a = 0xfd;
    writeData(Enemy_Y_Speed + x, a); // set vertical speed to move upwards
    a = 0x01;
    writeData(Enemy_Flag + x, a); // enable enemy object by setting flag
    a = 0x05;
    writeData(Enemy_State + x, a); // put spiny in egg state and leave

ChpChpEx:
    goto Return;

//------------------------------------------------------------------------

InitLongFirebar:
    JSR(DuplicateEnemyObj, 282); // create enemy object for long firebar

InitShortFirebar:
    a = 0x00; // initialize low byte of spin state
    writeData(FirebarSpinState_Low + x, a);
    a = M(Enemy_ID + x); // subtract $1b from enemy identifier
    a -= 0x1b;
    y = a;
    a = M(FirebarSpinSpdData + y); // get spinning speed of firebar
    writeData(FirebarSpinSpeed + x, a);
    a = M(FirebarSpinDirData + y); // get spinning direction of firebar
    writeData(FirebarSpinDirection + x, a);
    a = M(Enemy_Y_Position + x);
    a += 0x04;
    writeData(Enemy_Y_Position + x, a);
    a = M(Enemy_X_Position + x);
    wide = ((M(Enemy_PageLoc + x) << 8) | a) + 0x04;
    writeData(Enemy_X_Position + x, LOBYTE(wide));
    writeData(Enemy_PageLoc + x, HIBYTE(wide));
    a = HIBYTE(wide);
    goto TallBBox2; // set bounding box control (not used) and leave

InitFlyingCheepCheep:
    a = M(FrenzyEnemyTimer); // if timer here not expired yet, branch to leave
    if (a != 0)
        goto ChpChpEx;
    JSR(SmallBBox, 283); // jump to set bounding box size $09 and init other values
    a = M(PseudoRandomBitReg + 1 + x);
    a &= 0b00000011; // set pseudorandom offset here
    y = a;
    a = M(FlyCCTimerData + y); // load timer with pseudorandom offset
    writeData(FrenzyEnemyTimer, a);
    y = 0x03; // load Y with default value
    a = M(SecondaryHardMode);
    if (a != 0)
    { // if secondary hard mode flag not set, do not increment Y
        ++y; // otherwise, increment Y to allow as many as four onscreen
    } // MaxCC: store whatever pseudorandom bits are in Y
    writeData(0x00, y);
    if (x >= M(0x00))
        goto ChpChpEx; // if X => Y, branch to leave
    a = M(PseudoRandomBitReg + x);
    a &= 0b00000011; // get last two bits of LSFR, first part
    writeData(0x00, a); // and store in two places
    writeData(0x01, a);
    a = 0xfb; // set vertical speed for cheep-cheep
    writeData(Enemy_Y_Speed + x, a);
    a = 0x00; // load default value
    y = M(Player_X_Speed); // check player's horizontal speed
    if (y == 0)
        goto GSeed; // if player not moving left or right, skip this part
    a = 0x04;
    if (y < 0x19)
        goto GSeed; // do not change A
    a <<= 1; // otherwise, multiply A by 2

GSeed: // save to stack
    pha();
    a += M(0x00); // add to last two bits of LSFR we saved earlier
    writeData(0x00, a); // save it there
    a = M(PseudoRandomBitReg + 1 + x);
    a &= 0b00000011; // if neither of the last two bits of second LSFR set,
    if (a != 0)
    { // skip this part and save contents of $00
        a = M(PseudoRandomBitReg + 2 + x);
        a &= 0b00001111; // otherwise overwrite with lower nybble of
        writeData(0x00, a); // third LSFR part
    } // RSeed: get value from stack we saved earlier
    pla();
    a += M(0x01); // add to last two bits of LSFR we saved in other place
    y = a; // use as pseudorandom offset here
    a = M(FlyCCXSpeedData + y); // get horizontal speed using pseudorandom offset
    writeData(Enemy_X_Speed + x, a);
    a = 0x01; // set to move towards the right
    writeData(Enemy_MovingDir + x, a);
    a = M(Player_X_Speed); // if player moving left or right, branch ahead of this part
    if (a != 0)
        goto D2XPos1;
    y = M(0x00); // get first LSFR or third LSFR lower nybble
    a = y; // and check for d1 set
    a &= 0b00000010;
    if (a == 0)
        goto D2XPos1; // if d1 not set, branch
    a = M(Enemy_X_Speed + x);
    a ^= 0xff; // if d1 set, change horizontal speed
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
    a = 0x01;
    writeData(Enemy_Flag + x, a); // set enemy's buffer flag
    writeData(Enemy_Y_HighPos + x, a); // set enemy's high vertical byte
    a = 0xf8;
    writeData(Enemy_Y_Position + x, a); // put enemy below the screen, and we are done
    goto Return;

//------------------------------------------------------------------------

InitBowser:
    JSR(DuplicateEnemyObj, 284); // jump to create another bowser object
    writeData(BowserFront_Offset, x); // save offset of first here
    a = 0x00;
    writeData(BowserBodyControls, a); // initialize bowser's body controls
    writeData(BridgeCollapseOffset, a); // and bridge collapse offset
    a = M(Enemy_X_Position + x);
    writeData(BowserOrigXPos, a); // store original horizontal position here
    a = 0xdf;
    writeData(BowserFireBreathTimer, a); // store something here
    writeData(Enemy_MovingDir + x, a); // and in moving direction
    a = 0x20;
    writeData(BowserFeetCounter, a); // set bowser's feet timer and in enemy timer
    writeData(EnemyFrameTimer + x, a);
    a = 0x05;
    writeData(BowserHitPoints, a); // give bowser 5 hit points
    a >>= 1;
    writeData(BowserMovementSpeed, a); // set default movement speed here
    goto Return;

//------------------------------------------------------------------------

DuplicateEnemyObj:
    y = 0xff; // start at beginning of enemy slots

    do // FSLoop: increment one slot
    {
        ++y;
        a = M(Enemy_Flag + y); // check enemy buffer flag for empty slot
    } while (a != 0); // if set, branch and keep checking
    writeData(DuplicateObj_Offset, y); // otherwise set offset here
    a = x; // transfer original enemy buffer offset
    a |= 0b10000000; // store with d7 set as flag in new enemy
    writeData(Enemy_Flag + y, a); // slot as well as enemy offset
    a = M(Enemy_PageLoc + x);
    writeData(Enemy_PageLoc + y, a); // copy page location and horizontal coordinates
    a = M(Enemy_X_Position + x); // from original enemy to new enemy
    writeData(Enemy_X_Position + y, a);
    a = 0x01;
    writeData(Enemy_Flag + x, a); // set flag as normal for original enemy
    writeData(Enemy_Y_HighPos + y, a); // set high vertical byte for new enemy
    a = M(Enemy_Y_Position + x);
    writeData(Enemy_Y_Position + y, a); // copy vertical coordinate from original to new

    do // FlmEx: and then leave
    {
        goto Return;

    //------------------------------------------------------------------------

InitBowserFlame:
        a = M(FrenzyEnemyTimer); // if timer not expired yet, branch to leave
    } while (a != 0);
    writeData(Enemy_Y_MoveForce + x, a); // reset something here
    a = M(NoiseSoundQueue);
    a |= Sfx_BowserFlame; // load bowser's flame sound into queue
    writeData(NoiseSoundQueue, a);
    y = M(BowserFront_Offset); // get bowser's buffer offset
    a = M(Enemy_ID + y); // check for bowser
    if (a != Bowser)
    { // branch if found
        JSR(SetFlameTimer, 285); // get timer data based on flame counter
        a += 0x20; // add 32 frames by default
        y = M(SecondaryHardMode);
        if (y != 0)
        { // if secondary mode flag not set, use as timer setting
            a -= 0x10; // otherwise subtract 16 frames for secondary hard mode
        } // SetFrT: set timer accordingly
        writeData(FrenzyEnemyTimer, a);
        a = M(PseudoRandomBitReg + x);
        a &= 0b00000011; // get 2 LSB from first part of LSFR
        writeData(BowserFlamePRandomOfs + x, a); // set here
        y = a; // use as offset
        a = M(FlameYPosData + y); // load vertical position based on pseudorandom offset

PutAtRightExtent:
        writeData(Enemy_Y_Position + x, a); // set vertical position
        a = M(ScreenRight_X_Pos);
        wide = ((M(ScreenRight_PageLoc) << 8) | a) + 0x20; // place enemy 32 pixels beyond right side of screen
        writeData(Enemy_X_Position + x, LOBYTE(wide));
        writeData(Enemy_PageLoc + x, HIBYTE(wide));
        a = HIBYTE(wide);
    } // SpawnFromMouth
    else // skip this part to finish setting values
    {
        a = M(Enemy_X_Position + y); // get bowser's horizontal position
        a -= 0x0e; // subtract 14 pixels
        writeData(Enemy_X_Position + x, a); // save as flame's horizontal position
        a = M(Enemy_PageLoc + y);
        writeData(Enemy_PageLoc + x, a); // copy page location from bowser to flame
        a = M(Enemy_Y_Position + y);
        a += 0x08;
        writeData(Enemy_Y_Position + x, a); // save as flame's vertical position
        a = M(PseudoRandomBitReg + x);
        a &= 0b00000011; // get 2 LSB from first part of LSFR
        writeData(Enemy_YMF_Dummy + x, a); // save here
        y = a; // use as offset
        a = M(FlameYPosData + y); // get value here using bits as offset
        y = 0x00; // load default offset
        if (a >= M(Enemy_Y_Position + x))
        { // if less, do not increment offset
            ++y; // otherwise increment now
        } // SetMF: get value here and save
        a = M(FlameYMFAdderData + y);
        writeData(Enemy_Y_MoveForce + x, a); // to vertical movement force
        a = 0x00;
        writeData(EnemyFrenzyBuffer, a); // clear enemy frenzy buffer
    } // FinishFlame
    a = 0x08; // set $08 for bounding box control
    writeData(Enemy_BoundBoxCtrl + x, a);
    a = 0x01; // set high byte of vertical and
    writeData(Enemy_Y_HighPos + x, a); // enemy buffer flag
    writeData(Enemy_Flag + x, a);
    a >>= 1;
    writeData(Enemy_X_MoveForce + x, a); // initialize horizontal movement force, and
    writeData(Enemy_State + x, a); // enemy state
    goto Return;

//------------------------------------------------------------------------

InitFireworks:
    a = M(FrenzyEnemyTimer); // if timer not expired yet, branch to leave
    if (a == 0)
    {
        a = 0x20; // otherwise reset timer
        writeData(FrenzyEnemyTimer, a);
        --M(FireworksCounter); // decrement for each explosion
        y = 0x06; // start at last slot

        do // StarFChk
        {
            --y;
            a = M(Enemy_ID + y); // check for presence of star flag object
        } while (a != StarFlagObject); // routine goes into infinite loop = crash
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
        a = M(FireworksYPosData + y); // get vertical position using same offset
        writeData(Enemy_Y_Position + x, a); // and store as vertical coordinate for fireworks object
        a = 0x01;
        writeData(Enemy_Y_HighPos + x, a); // store in vertical high byte
        writeData(Enemy_Flag + x, a); // and activate enemy buffer flag
        a >>= 1;
        writeData(ExplosionGfxCounter + x, a); // initialize explosion counter
        a = 0x08;
        writeData(ExplosionTimerCounter + x, a); // set explosion timing counter
    } // ExitFWk
    goto Return;

//------------------------------------------------------------------------

BulletBillCheepCheep:
    a = M(FrenzyEnemyTimer); // if timer not expired yet, branch to leave
    if (a != 0)
        goto ExF17;
    a = M(AreaType); // are we in a water-type level?
    if (a == 0)
    { // if not, branch elsewhere
        if (x >= 0x03)
            goto ExF17; // if so, branch to leave
        y = 0x00; // load default offset
        a = M(PseudoRandomBitReg + x);
        if (a >= 0xaa)
        { // if less than preset, do not increment offset
            ++y; // otherwise increment
        } // ChkW2: check world number
        a = M(WorldNumber);
        if (a != World2)
        { // if we're on world 2, do not increment offset
            ++y; // otherwise increment
        } // Get17ID
        a = y;
        a &= 0b00000001; // mask out all but last bit of offset
        y = a;
        a = M(SwimCC_IDData + y); // load identifier for cheep-cheeps

Set17ID: // store whatever's in A as enemy identifier
        writeData(Enemy_ID + x, a);
        a = M(BitMFilter);
        if (a == 0xff)
        {
            a = 0x00; // initialize vertical position filter
            writeData(BitMFilter, a);
        } // GetRBit: get first part of LSFR
        a = M(PseudoRandomBitReg + x);
        a &= 0b00000111; // mask out all but 3 LSB

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
        JSR(PutAtRightExtent, 286); // set vertical position and other values
        writeData(Enemy_YMF_Dummy + x, a); // initialize dummy variable
        a = 0x20; // set timer
        writeData(FrenzyEnemyTimer, a);
        goto CheckpointEnemyID; // process our new enemy object
    } // DoBulletBills
    y = 0xff; // start at beginning of enemy slots

BB_SLoop: // move onto the next slot
    ++y;
    if (y < 0x05)
    {
        a = M(Enemy_Flag + y); // if enemy buffer flag not set,
        if (a == 0)
            goto BB_SLoop; // loop back and check another slot
        a = M(Enemy_ID + y);
        if (a != BulletBill_FrenzyVar)
            goto BB_SLoop; // bullet bill object (frenzy variant)

ExF17: // if found, leave
        goto Return;

    //------------------------------------------------------------------------
    } // FireBulletBill
    a = M(Square2SoundQueue);
    a |= Sfx_Blast; // play fireworks/gunfire sound
    writeData(Square2SoundQueue, a);
    a = BulletBill_FrenzyVar; // load identifier for bullet bill object
    if (a != 0)
        goto Set17ID; // unconditional branch

HandleGroupEnemies:
    y = 0x00; // load value for green koopa troopa
    a -= 0x37; // subtract $37 from second byte read
    pha(); // save result in stack for now
    if (a < 0x04)
    { // if so, branch
        pha(); // save another copy to stack
        y = Goomba; // load value for goomba enemy
        a = M(PrimaryHardMode); // if primary hard mode flag not set,
        if (a != 0)
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
    a = M(ScreenRight_PageLoc); // get page number of right edge of screen
    writeData(0x02, a); // save here
    a = M(ScreenRight_X_Pos); // get pixel coordinate of right edge
    writeData(0x03, a); // save here
    y = 0x02; // load two enemies by default
    pla(); // get first copy from stack
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // check to see if d0 was set
    if (shiftedBit)
    { // if not, use default value
        ++y; // otherwise increment to three enemies
    } // CntGrp: save number of enemies here
    writeData(NumberofGroupEnemies, y);

GrLoop: // start at beginning of enemy buffers
    x = 0xff;

GSltLp: // increment and branch if past
    ++x;
    if (x < 0x05)
    {
        a = M(Enemy_Flag + x); // check to see if enemy is already
        if (a != 0)
            goto GSltLp; // stored in buffer, and branch if so
        a = M(0x01);
        writeData(Enemy_ID + x, a); // store enemy object identifier
        a = M(0x02);
        writeData(Enemy_PageLoc + x, a); // store page location for enemy object
        a = M(0x03);
        writeData(Enemy_X_Position + x, a); // store x coordinate for enemy object
        wide = ((M(0x02) << 8) | a) + 0x18; // add 24 pixels for next enemy
        writeData(0x03, LOBYTE(wide));
        writeData(0x02, HIBYTE(wide));
        a = HIBYTE(wide); // add carry to page location for
        a = M(0x00); // store y coordinate for enemy object
        writeData(Enemy_Y_Position + x, a);
        a = 0x01; // activate flag for buffer, and
        writeData(Enemy_Y_HighPos + x, a); // put enemy within the screen vertically
        writeData(Enemy_Flag + x, a);
        JSR(CheckpointEnemyID, 287); // process each enemy object separately
        --M(NumberofGroupEnemies); // do this until we run out of enemy objects
        if (M(NumberofGroupEnemies) != 0)
            goto GrLoop;
    } // NextED: jump to increment data offset and leave
    goto Inc2B;

InitPiranhaPlant:
    a = 0x01; // set initial speed
    writeData(PiranhaPlant_Y_Speed + x, a);
    a >>= 1;
    writeData(Enemy_State + x, a); // initialize enemy state and what would normally
    writeData(PiranhaPlant_MoveFlag + x, a); // be used as vertical speed, but not in this case
    a = M(Enemy_Y_Position + x);
    writeData(PiranhaPlantDownYPos + x, a); // save original vertical coordinate here
    a -= 0x18;
    writeData(PiranhaPlantUpYPos + x, a); // save original vertical coordinate - 24 pixels here
    a = 0x09;
    goto SetBBox2; // set specific value for bounding box control

InitEnemyFrenzy:
    a = M(Enemy_ID + x); // load enemy identifier
    writeData(EnemyFrenzyBuffer, a); // save in enemy frenzy buffer
    a -= 0x12; // subtract 12 and use as offset for jump engine
    switch (a)
    {
    case 0:
        goto LakituAndSpinyHandler;
    case 1:
        goto NoFrenzyCode;
    case 2:
        goto InitFlyingCheepCheep;
    case 3:
        goto InitBowserFlame;
    case 4:
        goto InitFireworks;
    case 5:
        goto BulletBillCheepCheep;
    }

NoFrenzyCode:
    goto Return;

//------------------------------------------------------------------------

EndFrenzy:
    y = 0x05; // start at last slot

    do // LakituChk: check enemy identifiers
    {
        a = M(Enemy_ID + y);
        if (a == Lakitu)
        {
            a = 0x01; // if found, set state
            writeData(Enemy_State + y, a);
        } // NextFSlot: move onto the next slot
        --y;
    } while ((y & 0x80) == 0); // do this until all slots are checked
    a = 0x00;
    writeData(EnemyFrenzyBuffer, a); // empty enemy frenzy buffer
    writeData(Enemy_Flag + x, a); // disable enemy buffer flag for this object
    goto Return;

//------------------------------------------------------------------------

InitJumpGPTroopa:
    a = 0x02; // set for movement to the left
    writeData(Enemy_MovingDir + x, a);
    a = 0xf8; // set horizontal speed
    writeData(Enemy_X_Speed + x, a);

TallBBox2: // set specific value for bounding box control
    a = 0x03;

SetBBox2: // set bounding box control then leave
    writeData(Enemy_BoundBoxCtrl + x, a);
    goto Return;

//------------------------------------------------------------------------

InitBalPlatform:
    --M(Enemy_Y_Position + x); // raise vertical position by two pixels
    --M(Enemy_Y_Position + x);
    y = M(SecondaryHardMode); // if secondary hard mode flag not set,
    if (y == 0)
    { // branch ahead
        y = 0x02; // otherwise set value here
        JSR(PosPlatform, 288); // do a sub to add or subtract pixels
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
    writeData(Enemy_MovingDir + x, a); // init moving direction
    y = a; // init Y
    JSR(PosPlatform, 289); // do a sub to add 8 pixels, then run shared code here

InitDropPlatform:
    a = 0xff;
    writeData(PlatformCollisionFlag + x, a); // set some value here
    goto CommonPlatCode; // then jump ahead to execute more code

InitHoriPlatform:
    a = 0x00;
    writeData(XMoveSecondaryCounter + x, a); // init one of the moving counters
    goto CommonPlatCode; // jump ahead to execute more code

InitVertPlatform:
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

CommonPlatCode:
    JSR(InitVStf, 290); // do a sub to init certain other values

SPBBox: // set default bounding box size control
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
    goto Return;

//------------------------------------------------------------------------

LargeLiftUp:
    JSR(PlatLiftUp, 291); // execute code for platforms going up
    goto LargeLiftBBox; // overwrite bounding box for large platforms

LargeLiftDown:
    JSR(PlatLiftDown, 292); // execute code for platforms going down

LargeLiftBBox:
    goto SPBBox; // jump to overwrite bounding box size control

PlatLiftUp:
    a = 0x10; // set movement amount here
    writeData(Enemy_Y_MoveForce + x, a);
    a = 0xff; // set moving speed for platforms going up
    writeData(Enemy_Y_Speed + x, a);
    goto CommonSmallLift; // skip ahead to part we should be executing

PlatLiftDown:
    a = 0xf0; // set movement amount here
    writeData(Enemy_Y_MoveForce + x, a);
    a = 0x00; // set moving speed for platforms going down
    writeData(Enemy_Y_Speed + x, a);

CommonSmallLift:
    y = 0x01;
    JSR(PosPlatform, 293); // do a sub to add 12 pixels due to preset value
    a = 0x04;
    writeData(Enemy_BoundBoxCtrl + x, a); // set bounding box control for small platforms
    goto Return;

//------------------------------------------------------------------------

PosPlatform:
    // add or subtract pixels depending on offset, as one 16-bit page:coordinate
    wide = ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x))
         + ((M(PlatPosDataHigh + y) << 8) | M(PlatPosDataLow + y));
    writeData(Enemy_X_Position + x, LOBYTE(wide)); // store as new horizontal coordinate
    writeData(Enemy_PageLoc + x, HIBYTE(wide)); // store as new page location
    a = HIBYTE(wide);
    goto Return; // and go back

//------------------------------------------------------------------------

EndOfEnemyInitCode:
    goto Return;

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
        goto RunNormalEnemies; // for objects $00-$14
    case 1:
        goto RunBowserFlame; // for objects $15-$1f
    case 2:
        goto RunFireworks;
    case 3:
        goto NoRunCode;
    case 4:
        goto NoRunCode;
    case 5:
        goto NoRunCode;
    case 6:
        goto NoRunCode;
    case 7:
        goto RunFirebarObj;
    case 8:
        goto RunFirebarObj;
    case 9:
        goto RunFirebarObj;
    case 10:
        goto RunFirebarObj;
    case 11:
        goto RunFirebarObj;
    case 12:
        goto RunFirebarObj; // for objects $20-$2f
    case 13:
        goto RunFirebarObj;
    case 14:
        goto RunFirebarObj;
    case 15:
        goto NoRunCode;
    case 16:
        goto RunLargePlatform;
    case 17:
        goto RunLargePlatform;
    case 18:
        goto RunLargePlatform;
    case 19:
        goto RunLargePlatform;
    case 20:
        goto RunLargePlatform;
    case 21:
        goto RunLargePlatform;
    case 22:
        goto RunLargePlatform;
    case 23:
        goto RunSmallPlatform;
    case 24:
        goto RunSmallPlatform;
    case 25:
        goto RunBowser;
    case 26:
        goto PowerUpObjHandler;
    case 27:
        goto VineObjectHandler;
    case 28:
        goto NoRunCode; // for objects $30-$35
    case 29:
        goto RunStarFlagObj;
    case 30:
        goto JumpspringHandler;
    case 31:
        goto NoRunCode;
    case 32:
        goto WarpZoneObject;
    case 33:
        goto RunRetainerObj;
    }

NoRunCode:
    goto Return;

//------------------------------------------------------------------------

RunRetainerObj:
    JSR(GetEnemyOffscreenBits, 294);
    JSR(RelativeEnemyPosition, 295);
    goto EnemyGfxHandler;

RunNormalEnemies:
    a = 0x00; // init sprite attributes
    writeData(Enemy_SprAttrib + x, a);
    JSR(GetEnemyOffscreenBits, 296);
    JSR(RelativeEnemyPosition, 297);
    JSR(EnemyGfxHandler, 298);
    JSR(GetEnemyBoundBox, 299);
    JSR(EnemyToBGCollisionDet, 300);
    JSR(EnemiesCollision, 301);
    JSR(PlayerEnemyCollision, 302);
    y = M(TimerControl); // if master timer control set, skip to last routine
    if (y == 0)
    {
        JSR(EnemyMovementSubs, 303);
    } // SkipMove
    goto OffscreenBoundsCheck;

EnemyMovementSubs:
    a = M(Enemy_ID + x);
    switch (a)
    {
    case 0:
        goto MoveNormalEnemy; // only objects $00-$14 use this table
    case 1:
        goto MoveNormalEnemy;
    case 2:
        goto MoveNormalEnemy;
    case 3:
        goto MoveNormalEnemy;
    case 4:
        goto MoveNormalEnemy;
    case 5:
        goto ProcHammerBro;
    case 6:
        goto MoveNormalEnemy;
    case 7:
        goto MoveBloober;
    case 8:
        goto MoveBulletBill;
    case 9:
        goto NoMoveCode;
    case 10:
        goto MoveSwimmingCheepCheep;
    case 11:
        goto MoveSwimmingCheepCheep;
    case 12:
        goto MovePodoboo;
    case 13:
        goto MovePiranhaPlant;
    case 14:
        goto MoveJumpingEnemy;
    case 15:
        goto ProcMoveRedPTroopa;
    case 16:
        goto MoveFlyGreenPTroopa;
    case 17:
        goto MoveLakitu;
    case 18:
        goto MoveNormalEnemy;
    case 19:
        goto NoMoveCode; // dummy
    case 20:
        goto MoveFlyingCheepCheep;
    }

NoMoveCode:
    goto Return;

//------------------------------------------------------------------------

RunBowserFlame:
    JSR(ProcBowserFlame, 304);
    JSR(GetEnemyOffscreenBits, 305);
    JSR(RelativeEnemyPosition, 306);
    JSR(GetEnemyBoundBox, 307);
    JSR(PlayerEnemyCollision, 308);
    goto OffscreenBoundsCheck;

RunFirebarObj:
    JSR(ProcFirebar, 309);
    goto OffscreenBoundsCheck;

RunSmallPlatform:
    JSR(GetEnemyOffscreenBits, 310);
    JSR(RelativeEnemyPosition, 311);
    JSR(SmallPlatformBoundBox, 312);
    JSR(SmallPlatformCollision, 313);
    JSR(RelativeEnemyPosition, 314);
    JSR(DrawSmallPlatform, 315);
    JSR(MoveSmallPlatform, 316);
    goto OffscreenBoundsCheck;

RunLargePlatform:
    JSR(GetEnemyOffscreenBits, 317);
    JSR(RelativeEnemyPosition, 318);
    JSR(LargePlatformBoundBox, 319);
    JSR(LargePlatformCollision, 320);
    a = M(TimerControl); // if master timer control set,
    if (a == 0)
    { // skip subroutine tree
        JSR(LargePlatformSubroutines, 321);
    } // SkipPT
    JSR(RelativeEnemyPosition, 322);
    JSR(DrawLargePlatform, 323);
    goto OffscreenBoundsCheck;

LargePlatformSubroutines:
    a = M(Enemy_ID + x); // subtract $24 to get proper offset for jump table
    a -= 0x24;
    switch (a)
    {
    case 0:
        goto BalancePlatform; // table used by objects $24-$2a
    case 1:
        goto YMovingPlatform;
    case 2:
        goto MoveLargeLiftPlat;
    case 3:
        goto MoveLargeLiftPlat;
    case 4:
        goto XMovingPlatform;
    case 5:
        goto DropPlatform;
    case 6:
        goto RightPlatform;
    }

EraseEnemyObject:
    a = 0x00; // clear all enemy object variables
    writeData(Enemy_Flag + x, a);
    writeData(Enemy_ID + x, a);
    writeData(Enemy_State + x, a);
    writeData(FloateyNum_Control + x, a);
    writeData(EnemyIntervalTimer + x, a);
    writeData(ShellChainCounter + x, a);
    writeData(Enemy_SprAttrib + x, a);
    writeData(EnemyFrameTimer + x, a);
    goto Return;

//------------------------------------------------------------------------

MovePodoboo:
    a = M(EnemyIntervalTimer + x); // check enemy timer
    if (a == 0)
    { // branch to move enemy if not expired
        JSR(InitPodoboo, 324); // otherwise set up podoboo again
        a = M(PseudoRandomBitReg + 1 + x); // get part of LSFR
        a |= 0b10000000; // set d7
        writeData(Enemy_Y_MoveForce + x, a); // store as movement force
        a &= 0b00001111; // mask out high nybble
        a |= 0x06; // set for at least six intervals
        writeData(EnemyIntervalTimer + x, a); // store as new enemy timer
        a = 0xf9;
        writeData(Enemy_Y_Speed + x, a); // set vertical speed to move podoboo upwards
    } // PdbM: branch to impose gravity on podoboo
    goto MoveJ_EnemyVertically;

ProcHammerBro:
    a = M(Enemy_State + x); // check hammer bro's enemy state for d5 set
    a &= 0b00100000;
    if (a != 0)
    { // if not set, go ahead with code
        goto MoveDefeatedEnemy; // otherwise jump to something else
    } // ChkJH: check jump timer
    a = M(HammerBroJumpTimer + x);
    if (a != 0)
    { // if expired, branch to jump
        --M(HammerBroJumpTimer + x); // otherwise decrement jump timer
        a = M(Enemy_OffscreenBits);
        a &= 0b00001100; // check offscreen bits
        if (a != 0)
            goto MoveHammerBroXDir; // if hammer bro a little offscreen, skip to movement code
        a = M(HammerThrowingTimer + x); // check hammer throwing timer
        if (a != 0)
            goto DecHT; // if not expired, skip ahead, do not throw hammer
        y = M(SecondaryHardMode); // otherwise get secondary hard mode flag
        a = M(HammerThrowTmrData + y); // get timer data using flag as offset
        writeData(HammerThrowingTimer + x, a); // set as new timer
        JSR(SpawnHammerObj, 325); // do a sub here to spawn hammer object
        if (!hammerSpawned)
            goto DecHT; // hammer not spawned, skip to decrement timer
        a = M(Enemy_State + x);
        a |= 0b00001000; // set d3 in enemy state for hammer throw
        writeData(Enemy_State + x, a);
        goto MoveHammerBroXDir; // jump to move hammer bro

DecHT: // decrement timer
        --M(HammerThrowingTimer + x);
        goto MoveHammerBroXDir; // jump to move hammer bro
    } // HammerBroJumpCode
    a = M(Enemy_State + x); // get hammer bro's enemy state
    a &= 0b00000111; // mask out all but 3 LSB
    if (a == 0x01)
        goto MoveHammerBroXDir; // if set, branch ahead to moving code
    a = 0x00; // load default value here
    writeData(0x00, a); // save into temp variable for now
    y = 0xfa; // set default vertical speed
    a = M(Enemy_Y_Position + x); // check hammer bro's vertical coordinate
    if ((a & 0x80) != 0)
        goto SetHJ; // if on the bottom half of the screen, use current speed
    y = 0xfd; // otherwise set alternate vertical speed
    ++M(0x00); // increment preset value to $01
    if (a < 0x70)
        goto SetHJ; // if above the middle of the screen, use current speed and $01
    --M(0x00); // otherwise return value to $00
    a = M(PseudoRandomBitReg + 1 + x); // get part of LSFR, mask out all but LSB
    a &= 0x01;
    if (a != 0)
        goto SetHJ; // if d0 of LSFR set, branch and use current speed and $00
    y = 0xfa; // otherwise reset to default vertical speed

SetHJ: // set vertical speed for jumping
    writeData(Enemy_Y_Speed + x, y);
    a = M(Enemy_State + x); // set d0 in enemy state for jumping
    a |= 0x01;
    writeData(Enemy_State + x, a);
    a = M(0x00); // load preset value here to use as bitmask
    a &= M(PseudoRandomBitReg + 2 + x); // and do bit-wise comparison with part of LSFR
    y = a; // then use as offset
    a = M(SecondaryHardMode); // check secondary hard mode flag
    if (a == 0)
    {
        y = a; // if secondary hard mode flag clear, set offset to 0
    } // HJump: get jump length timer data using offset from before
    a = M(HammerBroJumpLData + y);
    writeData(EnemyFrameTimer + x, a); // save in enemy timer
    a = M(PseudoRandomBitReg + 1 + x);
    a |= 0b11000000; // get contents of part of LSFR, set d7 and d6, then
    writeData(HammerBroJumpTimer + x, a); // store in jump timer

MoveHammerBroXDir:
    y = 0xfc; // move hammer bro a little to the left
    a = M(FrameCounter);
    a &= 0b01000000; // change hammer bro's direction every 64 frames
    if (a == 0)
    {
        y = 0x04; // if d6 set in counter, move him a little to the right
    } // Shimmy: store horizontal speed
    writeData(Enemy_X_Speed + x, y);
    y = 0x01; // set to face right by default
    JSR(PlayerEnemyDiff, 326); // get horizontal difference between player and hammer bro
    if ((a & 0x80) != 0)
        goto SetShim; // if enemy to the left of player, skip this part
    ++y; // set to face left
    a = M(EnemyIntervalTimer + x); // check walking timer
    if (a != 0)
        goto SetShim; // if not yet expired, skip to set moving direction
    a = 0xf8;
    writeData(Enemy_X_Speed + x, a); // otherwise, make the hammer bro walk left towards player

SetShim: // set moving direction
    writeData(Enemy_MovingDir + x, y);

MoveNormalEnemy:
    y = 0x00; // init Y to leave horizontal movement as-is
    a = M(Enemy_State + x);
    a &= 0b01000000; // check enemy state for d6 set, if set skip
    if (a != 0)
        goto FallE; // to move enemy vertically, then horizontally if necessary
    a = M(Enemy_State + x);
    a <<= 1; // check enemy state for d7 set
    if ((M(Enemy_State + x) & 0x80) != 0)
        goto SteadM; // if set, branch to move enemy horizontally
    a = M(Enemy_State + x);
    a &= 0b00100000; // check enemy state for d5 set
    if (a != 0)
        goto MoveDefeatedEnemy; // if set, branch to move defeated enemy object
    a = M(Enemy_State + x);
    a &= 0b00000111; // check d2-d0 of enemy state for any set bits
    if (a == 0)
        goto SteadM; // if enemy in normal state, branch to move enemy horizontally
    if (a == 0x05)
        goto FallE; // if enemy in state used by spiny's egg, go ahead here
    if (a < 0x03)
    { // if enemy in states $03 or $04, skip ahead to yet another part

FallE: // do a sub here to move enemy downwards
        JSR(MoveD_EnemyVertically, 327);
        y = 0x00;
        a = M(Enemy_State + x); // check for enemy state $02
        if (a != 0x02)
        { // if found, branch to move enemy horizontally
            a &= 0b01000000; // check for d6 set
            if (a == 0)
                goto SteadM; // if not set, branch to something else
            a = M(Enemy_ID + x);
            if (a == PowerUpObject)
                goto SteadM;
            if (a != PowerUpObject)
                goto SlowM; // if any other object where d6 set, jump to set Y
        } // MEHor: jump here to move enemy horizontally for <> $2e and d6 set
        goto MoveEnemyHorizontally;

SlowM: // if branched here, increment Y to slow horizontal movement
        y = 0x01;

SteadM: // get current horizontal speed
        a = M(Enemy_X_Speed + x);
        pha(); // save to stack
        if ((a & 0x80) != 0)
        { // if not moving or moving right, skip, leave Y alone
            ++y;
            ++y; // otherwise increment Y to next data
        } // AddHS
        a += M(XSpeedAdderData + y); // add value here to slow enemy down if necessary
        writeData(Enemy_X_Speed + x, a); // save as horizontal speed temporarily
        JSR(MoveEnemyHorizontally, 328); // then do a sub to move horizontally
        pla();
        writeData(Enemy_X_Speed + x, a); // get old horizontal speed from stack and return to
        goto Return; // original memory location, then leave

    //------------------------------------------------------------------------
    } // ReviveStunned
    a = M(EnemyIntervalTimer + x); // if enemy timer not expired yet,
    if (a == 0)
    { // skip ahead to something else
        writeData(Enemy_State + x, a); // otherwise initialize enemy state to normal
        a = M(FrameCounter);
        a &= 0x01; // get d0 of frame counter
        y = a; // use as Y and increment for movement direction
        ++y;
        writeData(Enemy_MovingDir + x, y); // store as pseudorandom movement direction
        --y; // decrement for use as pointer
        a = M(PrimaryHardMode); // check primary hard mode flag
        if (a != 0)
        { // if not set, use pointer as-is
            ++y;
            ++y; // otherwise increment 2 bytes to next data
        } // SetRSpd: load and store new horizontal speed
        a = M(RevivedXSpeed + y);
        writeData(Enemy_X_Speed + x, a); // and leave
        goto Return;

    //------------------------------------------------------------------------

MoveDefeatedEnemy:
        JSR(MoveD_EnemyVertically, 329); // execute sub to move defeated enemy downwards
        goto MoveEnemyHorizontally; // now move defeated enemy horizontally
    } // ChkKillGoomba
    if (a != 0x0e)
        goto NKGmba; // a certain point, and branch to leave if not
    a = M(Enemy_ID + x);
    if (a != Goomba)
        goto NKGmba; // branch if not found
    JSR(EraseEnemyObject, 330); // otherwise, kill this goomba object

NKGmba: // leave!
    goto Return;

//------------------------------------------------------------------------

MoveJumpingEnemy:
    JSR(MoveJ_EnemyVertically, 331); // do a sub to impose gravity on green paratroopa
    goto MoveEnemyHorizontally; // jump to move enemy horizontally

ProcMoveRedPTroopa:
    a = M(Enemy_Y_Speed + x);
    a |= M(Enemy_Y_MoveForce + x); // check for any vertical force or speed
    if (a != 0)
        goto MoveRedPTUpOrDown; // branch if any found
    writeData(Enemy_YMF_Dummy + x, a); // initialize something here
    a = M(Enemy_Y_Position + x); // check current vs. original vertical coordinate
    if (a >= M(RedPTroopaOrigXPos + x))
        goto MoveRedPTUpOrDown; // if current => original, skip ahead to more code
    a = M(FrameCounter); // get frame counter
    a &= 0b00000111; // mask out all but 3 LSB
    if (a == 0)
    { // if any bits set, branch to leave
        ++M(Enemy_Y_Position + x); // otherwise increment red paratroopa's vertical position
    } // NoIncPT: leave
    goto Return;

//------------------------------------------------------------------------

MoveRedPTUpOrDown:
    a = M(Enemy_Y_Position + x); // check current vs. central vertical coordinate
    if (a >= M(RedPTroopaCenterYPos + x))
    { // if current < central, jump to move downwards
        goto MoveRedPTroopaUp; // otherwise jump to move upwards
    } // MovPTDwn: move downwards
    goto MoveRedPTroopaDown;

MoveFlyGreenPTroopa:
    JSR(XMoveCntr_GreenPTroopa, 332); // do sub to increment primary and secondary counters
    JSR(MoveWithXMCntrs, 333); // do sub to move green paratroopa accordingly, and horizontally
    y = 0x01; // set Y to move green paratroopa down
    a = M(FrameCounter);
    a &= 0b00000011; // check frame counter 2 LSB for any bits set
    if (a == 0)
    { // branch to leave if set to move up/down every fourth frame
        a = M(FrameCounter);
        a &= 0b01000000; // check frame counter for d6 set
        if (a == 0)
        { // branch to move green paratroopa down if set
            y = 0xff; // otherwise set Y to move green paratroopa up
        } // YSway: store adder here
        writeData(0x00, y);
        a = M(Enemy_Y_Position + x);
        a += M(0x00); // to give green paratroopa a wavy flight
        writeData(Enemy_Y_Position + x, a);
    } // NoMGPT: leave!
    goto Return;

//------------------------------------------------------------------------

XMoveCntr_GreenPTroopa:
    a = 0x13; // load preset maximum value for secondary counter

XMoveCntr_Platform:
    writeData(0x01, a); // store value here
    a = M(FrameCounter);
    a &= 0b00000011; // branch to leave if not on
    if (a == 0)
    { // every fourth frame
        y = M(XMoveSecondaryCounter + x); // get secondary counter
        a = M(XMovePrimaryCounter + x); // get primary counter
        a >>= 1;
        if ((M(XMovePrimaryCounter + x) & 0x01) != 0)
            goto DecSeXM; // if d0 of primary counter set, branch elsewhere
        if (y == M(0x01))
            goto IncPXM; // if equal, branch ahead of this part
        ++M(XMoveSecondaryCounter + x); // increment secondary counter and leave
    } // NoIncXM
    goto Return;

//------------------------------------------------------------------------

IncPXM: // increment primary counter and leave
    ++M(XMovePrimaryCounter + x);
    goto Return;

//------------------------------------------------------------------------

DecSeXM: // put secondary counter in A
    a = y;
    if (a == 0)
        goto IncPXM; // if secondary counter at zero, branch back
    --M(XMoveSecondaryCounter + x); // otherwise decrement secondary counter and leave
    goto Return;

//------------------------------------------------------------------------

MoveWithXMCntrs:
    a = M(XMoveSecondaryCounter + x); // save secondary counter to stack
    pha();
    y = 0x01; // set value here by default
    a = M(XMovePrimaryCounter + x);
    a &= 0b00000010; // if d1 of primary counter is
    if (a == 0)
    { // set, branch ahead of this part here
        a = M(XMoveSecondaryCounter + x);
        a ^= 0xff; // otherwise change secondary
        a += 0x01;
        writeData(XMoveSecondaryCounter + x, a);
        y = 0x02; // load alternate value here
    } // XMRight: store as moving direction
    writeData(Enemy_MovingDir + x, y);
    JSR(MoveEnemyHorizontally, 334);
    writeData(0x00, a); // save value obtained from sub here
    pla(); // get secondary counter from stack
    writeData(XMoveSecondaryCounter + x, a); // and return to original place
    goto Return;

//------------------------------------------------------------------------

MoveBloober:
    a = M(Enemy_State + x);
    a &= 0b00100000; // check enemy state for d5 set
    if (a == 0)
    { // branch if set to move defeated bloober
        y = M(SecondaryHardMode); // use secondary hard mode flag as offset
        a = M(PseudoRandomBitReg + 1 + x); // get LSFR
        a &= M(BlooberBitmasks + y); // mask out bits in LSFR using bitmask loaded with offset
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
            JSR(PlayerEnemyDiff, 335); // get horizontal difference between player and bloober
            blooberCarry = enemyRightOfPlayer; // the difference leaves its no-borrow behind
            if ((a & 0x80) == 0)
                goto SBMDir; // if enemy to the right of player, keep left
            --y; // otherwise decrement to set right moving direction

SBMDir: // set moving direction of bloober, then continue on here
            writeData(Enemy_MovingDir + x, y);
        } // BlooberSwim
        JSR(ProcSwimmingB, 336); // execute sub to make bloober swim characteristically
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
            a = M(Enemy_X_Position + x);
            wide = ((M(Enemy_PageLoc + x) << 8) | a) + M(BlooperMoveSpeed + x);
            writeData(Enemy_X_Position + x, LOBYTE(wide)); // store result as new horizontal coordinate
            writeData(Enemy_PageLoc + x, HIBYTE(wide)); // store as new page location and leave
            a = HIBYTE(wide);
            goto Return;

        //------------------------------------------------------------------------
        } // LeftSwim
        a = M(Enemy_X_Position + x);
        wide = ((M(Enemy_PageLoc + x) << 8) | a) - M(BlooperMoveSpeed + x);
        writeData(Enemy_X_Position + x, LOBYTE(wide)); // store result as new horizontal coordinate
        writeData(Enemy_PageLoc + x, HIBYTE(wide)); // store as new page location and leave
        a = HIBYTE(wide);
        goto Return;

    //------------------------------------------------------------------------
    } // MoveDefeatedBloober
    goto MoveEnemySlowVert; // jump to move defeated bloober downwards

ProcSwimmingB:
    a = M(BlooperMoveCounter + x); // get enemy's movement counter
    a &= 0b00000010; // check for d1 set
    if (a == 0)
    { // branch if set
        a = M(FrameCounter);
        a &= 0b00000111; // get 3 LSB of frame counter
        pha(); // and save it to the stack
        if ((M(BlooperMoveCounter + x) & 0x01) == 0) // check d0 of the enemy's movement counter
        { // branch if set
            pla(); // pull 3 LSB of frame counter from the stack
            if (a != 0)
                goto BSwimE; // branch to leave, execute code only every eighth frame
            a = M(Enemy_Y_MoveForce + x);
            a += 0x01;
            writeData(Enemy_Y_MoveForce + x, a); // set movement force
            writeData(BlooperMoveSpeed + x, a); // set as movement speed
            if (a != 0x02)
                goto BSwimE; // if certain horizontal speed, branch to leave
            ++M(BlooperMoveCounter + x); // otherwise increment movement counter

BSwimE:
            goto Return;

        //------------------------------------------------------------------------
        } // SlowSwim
        pla(); // pull 3 LSB of frame counter from the stack
        if (a != 0)
            goto NoSSw; // branch to leave, execute code only every eighth frame
        a = M(Enemy_Y_MoveForce + x);
        a -= 0x01;
        writeData(Enemy_Y_MoveForce + x, a); // set movement force
        writeData(BlooperMoveSpeed + x, a); // set as movement speed
        if (a != 0)
            goto NoSSw; // if any speed, branch to leave
        ++M(BlooperMoveCounter + x); // otherwise increment movement counter
        a = 0x02;
        writeData(EnemyIntervalTimer + x, a); // set enemy's timer

NoSSw: // leave
        goto Return;

    //------------------------------------------------------------------------
    } // ChkForFloatdown
    a = M(EnemyIntervalTimer + x); // get enemy timer
    if (a != 0)
    { // branch if expired

Floatdown:
        a = M(FrameCounter); // get frame counter
        a >>= 1; // check for d0 set
        if ((M(FrameCounter) & 0x01) == 0)
        { // branch to leave on every other frame
            ++M(Enemy_Y_Position + x); // otherwise increment vertical coordinate
        } // NoFD: leave
        goto Return;

    //------------------------------------------------------------------------
    } // ChkNearPlayer
    a = M(Enemy_Y_Position + x); // get vertical coordinate
    a = (uint8_t)(a + 0x10 + (blooberCarry ? 1 : 0)); // add sixteen pixels, plus whatever carry the swim code left behind
    if (a < M(Player_Y_Position))
        goto Floatdown; // if modified vertical less than player's, branch
    a = 0x00;
    writeData(BlooperMoveCounter + x, a); // otherwise nullify movement counter
    goto Return;

//------------------------------------------------------------------------

MoveBulletBill:
    a = M(Enemy_State + x); // check bullet bill's enemy object state for d5 set
    a &= 0b00100000;
    if (a != 0)
    { // if not set, continue with movement code
        goto MoveJ_EnemyVertically; // otherwise jump to move defeated bullet bill downwards
    } // NotDefB: set bullet bill's horizontal speed
    a = 0xe8;
    writeData(Enemy_X_Speed + x, a); // and move it accordingly (note: this bullet bill
    goto MoveEnemyHorizontally; // object occurs in frenzy object $17, not from cannons)

MoveSwimmingCheepCheep:
    a = M(Enemy_State + x); // check cheep-cheep's enemy object state
    a &= 0b00100000; // for d5 set
    if (a != 0)
    { // if not set, continue with movement code
        goto MoveEnemySlowVert; // otherwise jump to move defeated cheep-cheep downwards
    } // CCSwim: save enemy state in $03
    writeData(0x03, a);
    a = M(Enemy_ID + x); // get enemy identifier
    a -= 0x0a; // subtract ten for cheep-cheep identifiers
    y = a; // use as offset
    a = M(SwimCCXMoveData + y); // load value here
    writeData(0x02, a);
    // page:coordinate:force is one 24-bit quantity here, so the borrow runs all the way up
    wide = (M(Enemy_PageLoc + x) << 16) | (M(Enemy_X_Position + x) << 8) | M(Enemy_X_MoveForce + x);
    wide -= M(0x02); // subtract preset value from horizontal force
    writeData(Enemy_X_MoveForce + x, LOBYTE(wide)); // store as new horizontal force
    writeData(Enemy_X_Position + x, HIBYTE(wide)); // and save as new horizontal coordinate
    writeData(Enemy_PageLoc + x, (uint8_t)(wide >> 16)); // page location, then save
    a = (uint8_t)(wide >> 16);
    a = 0x20;
    writeData(0x02, a); // save new value here
    if (x < 0x02)
        goto ExSwCC; // if in first or second slot, branch to leave
    a = M(CheepCheepMoveMFlag + x); // check movement flag
    if (a >= 0x10)
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
        goto ExSwCC; // coordinates < 15 pixels, leave movement speed alone
    a = y;
    writeData(CheepCheepMoveMFlag + x, a); // otherwise change movement speed

ExSwCC: // leave
    goto Return;

//------------------------------------------------------------------------

ProcFirebar:
    JSR(GetEnemyOffscreenBits, 337); // get offscreen information
    a = M(Enemy_OffscreenBits); // check for d3 set
    a &= 0b00001000; // if so, branch to leave
    if (a == 0)
    {
        a = M(TimerControl); // if master timer control set, branch
        if (a == 0)
        { // ahead of this part
            a = M(FirebarSpinSpeed + x); // load spinning speed of firebar
            JSR(FirebarSpin, 338); // modify current spinstate
            a &= 0b00011111; // mask out all but 5 LSB
            writeData(FirebarSpinState_High + x, a); // and store as new high byte of spinstate
        } // SusFbar: get high byte of spinstate
        a = M(FirebarSpinState_High + x);
        y = M(Enemy_ID + x); // check enemy identifier
        if (y < 0x1f)
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
        JSR(RelativeEnemyPosition, 339); // get relative coordinates to screen
        JSR(GetFirebarPosition, 340); // do a sub here (residual, too early to be used now)
        y = M(Enemy_SprDataOffset + x); // get OAM data offset
        a = M(Enemy_Rel_YPos); // get relative vertical coordinate
        writeData(Sprite_Y_Position + y, a); // store as Y in OAM data
        writeData(0x07, a); // also save here
        a = M(Enemy_Rel_XPos); // get relative horizontal coordinate
        writeData(Sprite_X_Position + y, a); // store as X in OAM data
        writeData(0x06, a); // also save here
        a = 0x01;
        writeData(0x00, a); // set $01 value here (not necessary)
        JSR(FirebarCollision, 341); // draw fireball part and do collision detection
        y = 0x05; // load value for short firebars by default
        a = M(Enemy_ID + x);
        if (a >= 0x1f)
        { // no, branch then
            y = 0x0b; // otherwise load value for long firebars
        } // SetMFbar: store maximum value for length of firebars
        writeData(0xed, y);
        a = 0x00;
        writeData(0x00, a); // initialize counter here

        do // DrawFbar: load high byte of spinstate
        {
            a = M(0xef);
            JSR(GetFirebarPosition, 342); // get fireball position data depending on firebar part
            JSR(DrawFirebar_Collision, 343); // position it properly, draw it and do collision detection
            a = M(0x00); // check which firebar part
            if (a == 0x04)
            {
                y = M(DuplicateObj_Offset); // if we arrive at fifth firebar part,
                a = M(Enemy_SprDataOffset + y); // get offset from long firebar and load OAM data offset
                writeData(0x06, a); // using long firebar offset, then store as new one here
            } // NextFbar: move onto the next firebar part
            ++M(0x00);
            a = M(0x00);
        } while (a < M(0xed)); // otherwise go back and do another
    } // SkipFBar
    goto Return;

//------------------------------------------------------------------------

DrawFirebar_Collision:
    a = M(0x03); // store mirror data elsewhere
    writeData(0x05, a);
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

FirebarCollision:
    JSR(DrawFirebar, 344); // run sub here to draw current tile of firebar
    a = y; // return OAM data offset and save
    pha(); // to the stack for now
    a = M(StarInvincibleTimer); // if star mario invincibility timer
    a |= M(TimerControl); // or master timer controls set
    if (a != 0)
        goto NoColFB; // then skip all of this
    writeData(0x05, a); // otherwise initialize counter
    y = M(Player_Y_HighPos);
    --y; // if player's vertical high byte offscreen,
    if (y != 0)
        goto NoColFB; // skip all of this
    y = M(Player_Y_Position); // get player's vertical position
    a = M(PlayerSize); // get player's size
    if (a == 0)
    { // if player small, branch to alter variables
        a = M(CrouchingFlag);
        if (a == 0)
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
    a = M(0x06); // if firebar on far right on the screen, skip this,
    if (a >= 0xf0)
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
        a = M(0x05);
        if (a == 0x02)
            goto NoColFB;
        y = M(0x05); // otherwise get temp here and use as offset
        a = M(Player_Y_Position);
        a += M(FirebarYPos + y); // add value loaded with offset to player's vertical coordinate
        ++M(0x05); // then increment temp and jump back
        goto FBCLoop;
    } // ChgSDir: set movement direction by default
    x = 0x01;
    a = M(0x04); // if OAM X coordinate of player's sprite 1
    if (a < M(0x06))
    { // then do not alter movement direction
        ++x; // otherwise increment it
    } // SetSDir: store movement direction here
    writeData(Enemy_MovingDir, x);
    x = 0x00;
    a = M(0x00); // save value written to $00 to stack
    pha();
    JSR(InjurePlayer, 345); // perform sub to hurt or kill player
    pla();
    writeData(0x00, a); // get value of $00 from stack

NoColFB: // get OAM data offset
    pla();
    a += 0x04;
    writeData(0x06, a);
    x = M(ObjectOffset); // get enemy object buffer offset and leave
    goto Return;

//------------------------------------------------------------------------

GetFirebarPosition:
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
    a = M(FirebarPosLookupTbl + y); // get data here and store as horizontal adder
    writeData(0x01, a);
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
    a = M(FirebarPosLookupTbl + y); // get data here and store as vertica adder
    writeData(0x02, a);
    pla(); // pull out whatever was in A one last time
    a >>= 1; // divide by eight or shift three to the right
    a >>= 1;
    a >>= 1;
    y = a; // use as offset
    a = M(FirebarMirrorData + y); // load mirroring data here
    writeData(0x03, a); // store
    goto Return;

//------------------------------------------------------------------------

MoveFlyingCheepCheep:
    a = M(Enemy_State + x); // check cheep-cheep's enemy state
    a &= 0b00100000; // for d5 set
    if (a != 0)
    { // branch to continue code if not set
        a = 0x00;
        writeData(Enemy_SprAttrib + x, a); // otherwise clear sprite attributes
        goto MoveJ_EnemyVertically; // and jump to move defeated cheep-cheep downwards
    } // FlyCC: move cheep-cheep horizontally based on speed and force
    JSR(MoveEnemyHorizontally, 346);
    y = 0x0d; // set vertical movement amount
    a = 0x05; // set maximum speed
    JSR(SetXMoveAmt, 347); // branch to impose gravity on flying cheep-cheep
    a = M(Enemy_Y_MoveForce + x);
    a >>= 1; // get vertical movement force and
    a >>= 1; // move high nybble to low
    a >>= 1;
    a >>= 1;
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
    goto Return; // drawing it next frame), then leave

//------------------------------------------------------------------------

MoveLakitu:
    a = M(Enemy_State + x); // check lakitu's enemy state
    a &= 0b00100000; // for d5 set
    if (a != 0)
    { // if not set, continue with code
        goto MoveD_EnemyVertically; // otherwise jump to move defeated lakitu downwards
    } // ChkLS: if lakitu's enemy state not set at all,
    a = M(Enemy_State + x);
    if (a != 0)
    { // go ahead and continue with code
        a = 0x00;
        writeData(LakituMoveDirection + x, a); // otherwise initialize moving direction to move to left
        writeData(EnemyFrenzyBuffer, a); // initialize frenzy buffer
        a = 0x10;
        if (a != 0)
            goto SetLSpd; // load horizontal speed and do unconditional branch
    } // Fr12S
    a = Spiny;
    writeData(EnemyFrenzyBuffer, a); // set spiny identifier in frenzy buffer
    y = 0x02;

    do // LdLDa: load values
    {
        a = M(LakituDiffAdj + y);
        writeData(0x0001 + y, a); // store in zero page
        --y;
    } while ((y & 0x80) == 0); // do this until all values are stired
    JSR(PlayerLakituDiff, 348); // execute sub to set speed and create spinys

SetLSpd: // set movement speed returned from sub
    writeData(LakituMoveSpeed + x, a);
    y = 0x01; // set moving direction to right by default
    a = M(LakituMoveDirection + x);
    a &= 0x01; // get LSB of moving direction
    if (a == 0)
    { // if set, branch to the end to use moving direction
        a = M(LakituMoveSpeed + x);
        a ^= 0xff; // get two's compliment of moving speed
        a += 0x01;
        writeData(LakituMoveSpeed + x, a); // store as new moving speed
        ++y; // increment moving direction to left
    } // SetLMov: store moving direction
    writeData(Enemy_MovingDir + x, y);
    goto MoveEnemyHorizontally; // move lakitu horizontally

PlayerLakituDiff:
    y = 0x00; // set Y for default value
    JSR(PlayerEnemyDiff, 349); // get horizontal difference between enemy and player
    if ((a & 0x80) != 0)
    { // branch if enemy is to the right of the player
        ++y; // increment Y for left of player
        a = M(0x00);
        a ^= 0xff; // get two's compliment of low byte of horizontal difference
        a += 0x01; // store two's compliment as horizontal difference
        writeData(0x00, a);
    } // ChkLakDif: get low byte of horizontal difference
    a = M(0x00);
    if (a < 0x3c)
        goto ChkPSpeed;
    a = 0x3c; // otherwise set maximum distance
    writeData(0x00, a);
    a = M(Enemy_ID + x); // check if lakitu is in our current enemy slot
    if (a != Lakitu)
        goto ChkPSpeed; // if not, branch elsewhere
    a = y; // compare contents of Y, now in A
    if (a == M(LakituMoveDirection + x))
        goto ChkPSpeed; // if moving toward the player, branch, do not alter
    a = M(LakituMoveDirection + x); // if moving to the left beyond maximum distance,
    if (a != 0)
    { // branch and alter without delay
        --M(LakituMoveSpeed + x); // decrement horizontal speed
        a = M(LakituMoveSpeed + x); // if horizontal speed not yet at zero, branch to leave
        if (a != 0)
            goto ExMoveLak;
    } // SetLMovD: set horizontal direction depending on horizontal
    a = y;
    writeData(LakituMoveDirection + x, a); // difference between enemy and player if necessary

ChkPSpeed:
    a = M(0x00);
    a &= 0b00111100; // mask out all but four bits in the middle
    a >>= 1; // divide masked difference by four
    a >>= 1;
    writeData(0x00, a); // store as new value
    y = 0x00; // init offset
    a = M(Player_X_Speed);
    if (a == 0)
        goto SubDifAdj; // if player not moving horizontally, branch
    a = M(ScrollAmount);
    if (a == 0)
        goto SubDifAdj; // if scroll speed not set, branch to same place
    ++y; // otherwise increment offset
    a = M(Player_X_Speed);
    if (a < 0x19)
        goto ChkSpinyO;
    a = M(ScrollAmount);
    if (a < 0x02)
        goto ChkSpinyO; // to same place
    ++y; // otherwise increment once more

ChkSpinyO: // check for spiny object
    a = M(Enemy_ID + x);
    if (a == Spiny)
    { // branch if not found
        a = M(Player_X_Speed); // if player not moving, skip this part
        if (a != 0)
            goto SubDifAdj;
    } // ChkEmySpd: check vertical speed
    a = M(Enemy_Y_Speed + x);
    if (a != 0)
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

ExMoveLak: // leave!!!
    goto Return;

//------------------------------------------------------------------------

BridgeCollapse:
    x = M(BowserFront_Offset); // get enemy offset for bowser
    a = M(Enemy_ID + x); // check enemy object identifier for bowser
    if (a != Bowser)
        goto SetM2; // metatile removal not necessary
    writeData(ObjectOffset, x); // store as enemy offset here
    a = M(Enemy_State + x); // if bowser in normal state, skip all of this
    if (a != 0)
    {
        a &= 0b01000000; // if bowser's state has d6 clear, skip to silence music
        if (a == 0)
            goto SetM2;
        a = M(Enemy_Y_Position + x); // check bowser's vertical coordinate
        if (a < 0xe0)
            goto MoveD_Bowser;

SetM2: // silence music
        a = Silence;
        writeData(EventMusicQueue, a);
        ++M(OperMode_Task); // move onto next secondary mode in autoctrl mode
        goto KillAllEnemies; // jump to empty all enemy slots and then leave

MoveD_Bowser:
        JSR(MoveEnemySlowVert, 350); // do a sub to move bowser downwards
        goto BowserGfxHandler; // jump to draw bowser's front and rear, then leave
    } // RemoveBridge
    --M(BowserFeetCounter); // decrement timer to control bowser's feet
    if (M(BowserFeetCounter) != 0)
        goto NoBFall; // if not expired, skip all of this
    a = 0x04;
    writeData(BowserFeetCounter, a); // otherwise, set timer now
    a = M(BowserBodyControls);
    a ^= 0x01; // invert bit to control bowser's feet
    writeData(BowserBodyControls, a);
    a = 0x22; // put high byte of name table address here for now
    writeData(0x05, a);
    y = M(BridgeCollapseOffset); // get bridge collapse offset here
    a = M(BridgeCollapseData + y); // load low byte of name table address and store here
    writeData(0x04, a);
    y = M(VRAM_Buffer1_Offset); // increment vram buffer offset
    ++y;
    x = 0x0c; // set offset for tile data for sub to draw blank metatile
    JSR(RemBridge, 351); // do sub here to remove bowser's bridge metatiles
    x = M(ObjectOffset); // get enemy offset
    JSR(MoveVOffset, 352); // set new vram buffer offset
    a = Sfx_Blast; // load the fireworks/gunfire sound into the square 2 sfx
    writeData(Square2SoundQueue, a); // queue while at the same time loading the brick
    a = Sfx_BrickShatter; // shatter sound into the noise sfx queue thus
    writeData(NoiseSoundQueue, a); // producing the unique sound of the bridge collapsing
    ++M(BridgeCollapseOffset); // increment bridge collapse offset
    a = M(BridgeCollapseOffset);
    if (a != 0x0f)
        goto NoBFall; // the end, go ahead and skip this part
    JSR(InitVStf, 353); // initialize whatever vertical speed bowser has
    a = 0b01000000;
    writeData(Enemy_State + x, a); // set bowser's state to one of defeated states (d6 set)
    a = Sfx_BowserFall;
    writeData(Square2SoundQueue, a); // play bowser defeat sound

NoBFall: // jump to code that draws bowser
    goto BowserGfxHandler;

RunBowser:
    a = M(Enemy_State + x); // if d5 in enemy state is not set
    a &= 0b00100000; // then branch elsewhere to run bowser
    if (a != 0)
    {
        a = M(Enemy_Y_Position + x); // otherwise check vertical position
        if (a < 0xe0)
            goto MoveD_Bowser; // otherwise proceed to KillAllEnemies

KillAllEnemies:
        x = 0x04; // start with last enemy slot

        do // KillLoop: branch to kill enemy objects
        {
            JSR(EraseEnemyObject, 354);
            --x; // move onto next enemy slot
        } while ((x & 0x80) == 0); // do this until all slots are emptied
        writeData(EnemyFrenzyBuffer, a); // empty frenzy buffer
        x = M(ObjectOffset); // get enemy object offset and leave
        goto Return;

    //------------------------------------------------------------------------
    } // BowserControl
    a = 0x00;
    writeData(EnemyFrenzyBuffer, a); // empty frenzy buffer
    a = M(TimerControl); // if master timer control not set,
    if (a != 0)
    { // skip jump and execute code here
        goto SkipToFB; // otherwise, jump over a bunch of code
    } // ChkMouth: check bowser's mouth
    a = M(BowserBodyControls);
    if ((a & 0x80) != 0)
    { // if bit clear, go ahead with code here
        goto HammerChk; // otherwise skip a whole section starting here
    } // FeetTmr: decrement timer to control bowser's feet
    --M(BowserFeetCounter);
    if (M(BowserFeetCounter) == 0)
    { // if not expired, skip this part
        a = 0x20; // otherwise, reset timer
        writeData(BowserFeetCounter, a);
        a = M(BowserBodyControls); // and invert bit used
        a ^= 0b00000001; // to control bowser's feet
        writeData(BowserBodyControls, a);
    } // ResetMDr: check frame counter
    a = M(FrameCounter);
    a &= 0b00001111; // if not on every sixteenth frame, skip
    if (a == 0)
    { // ahead to continue code
        a = 0x02; // otherwise reset moving/facing direction every
        writeData(Enemy_MovingDir + x, a); // sixteen frames
    } // B_FaceP: if timer set here expired,
    a = M(EnemyFrameTimer + x);
    if (a == 0)
        goto GetPRCmp; // branch to next section
    JSR(PlayerEnemyDiff, 355); // get horizontal difference between player and bowser,
    if ((a & 0x80) == 0)
        goto GetPRCmp; // and branch if bowser to the right of the player
    a = 0x01;
    writeData(Enemy_MovingDir + x, a); // set bowser to move and face to the right
    a = 0x02;
    writeData(BowserMovementSpeed, a); // set movement speed
    a = 0x20;
    writeData(EnemyFrameTimer + x, a); // set timer here
    writeData(BowserFireBreathTimer, a); // set timer used for bowser's flame
    a = M(Enemy_X_Position + x);
    if (a >= 0xc8)
        goto HammerChk; // skip ahead to some other section

GetPRCmp: // get frame counter
    a = M(FrameCounter);
    a &= 0b00000011;
    if (a != 0)
        goto HammerChk; // execute this code every fourth frame, otherwise branch
    a = M(Enemy_X_Position + x);
    if (a == M(BowserOrigXPos))
    { // branch to skip this part
        a = M(PseudoRandomBitReg + x);
        a &= 0b00000011; // get pseudorandom offset
        y = a;
        a = M(PRandomRange + y); // load value using pseudorandom offset
        writeData(MaxRangeFromOrigin, a); // and store here
    } // GetDToO
    a = M(Enemy_X_Position + x);
    a += M(BowserMovementSpeed); // coordinate and save as new horizontal position
    writeData(Enemy_X_Position + x, a);
    y = M(Enemy_MovingDir + x);
    if (y == 0x01)
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
        JSR(MoveEnemySlowVert, 356); // otherwise start by moving bowser downwards
        a = M(WorldNumber); // check world number
        if (a < World6)
            goto SetHmrTmr; // if world 1-5, skip this part (not time to throw hammers yet)
        a = M(FrameCounter);
        a &= 0b00000011; // check to see if it's time to execute sub
        if (a != 0)
            goto SetHmrTmr; // if not, skip sub, otherwise
        JSR(SpawnHammerObj, 357); // execute sub on every fourth frame to spawn misc object (hammer)

SetHmrTmr: // get current vertical position
        a = M(Enemy_Y_Position + x);
        if (a < 0x80)
            goto ChkFireB; // then skip to world number check for flames
        a = M(PseudoRandomBitReg + x);
        a &= 0b00000011; // get pseudorandom offset
        y = a;
        a = M(PRandomRange + y); // get value using pseudorandom offset
        writeData(EnemyFrameTimer + x, a); // set for timer here

SkipToFB: // jump to execute flames code
        goto ChkFireB;
    } // MakeBJump: if timer not yet about to expire,
    if (a != 0x01)
        goto ChkFireB; // skip ahead to next part
    --M(Enemy_Y_Position + x); // otherwise decrement vertical coordinate
    JSR(InitVStf, 358); // initialize movement amount
    a = 0xfe;
    writeData(Enemy_Y_Speed + x, a); // set vertical speed to move bowser upwards

ChkFireB: // check world number here
    a = M(WorldNumber);
    if (a != World8)
    { // if so, execute this part here
        if (a >= World6)
            goto BowserGfxHandler; // if so, skip this part here
    } // SpawnFBr: check timer here
    a = M(BowserFireBreathTimer);
    if (a != 0)
        goto BowserGfxHandler; // if not expired yet, skip all of this
    a = 0x20;
    writeData(BowserFireBreathTimer, a); // set timer here
    a = M(BowserBodyControls);
    a ^= 0b10000000; // invert bowser's mouth bit to open
    writeData(BowserBodyControls, a); // and close bowser's mouth
    if ((a & 0x80) != 0)
        goto ChkFireB; // if bowser's mouth open, loop back
    JSR(SetFlameTimer, 359); // get timing for bowser's flame
    y = M(SecondaryHardMode);
    if (y != 0)
    { // if secondary hard mode flag not set, skip this
        a -= 0x10; // otherwise subtract from value in A
    } // SetFBTmr: set value as timer here
    writeData(BowserFireBreathTimer, a);
    a = BowserFlame; // put bowser's flame identifier
    writeData(EnemyFrenzyBuffer, a); // in enemy frenzy buffer

BowserGfxHandler:
    JSR(ProcessBowserHalf, 360); // do a sub here to process bowser's front
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
    a = M(Enemy_State + x);
    writeData(Enemy_State + y, a); // copy enemy state directly from front to rear
    a = M(Enemy_MovingDir + x);
    writeData(Enemy_MovingDir + y, a); // copy moving direction also
    a = M(ObjectOffset); // save enemy object offset of front to stack
    pha();
    x = M(DuplicateObj_Offset); // put enemy object offset of rear as current
    writeData(ObjectOffset, x);
    a = Bowser; // set bowser's enemy identifier
    writeData(Enemy_ID + x, a); // store in bowser's rear object
    JSR(ProcessBowserHalf, 361); // do a sub here to process bowser's rear
    pla();
    writeData(ObjectOffset, a); // get original enemy object offset
    x = a;
    a = 0x00; // nullify bowser's front/rear graphics flag
    writeData(BowserGfxFlag, a);

    do // ExBGfxH: leave!
    {
        goto Return;

    //------------------------------------------------------------------------

ProcessBowserHalf:
        ++M(BowserGfxFlag); // increment bowser's graphics flag, then run subroutines
        JSR(RunRetainerObj, 362); // to get offscreen bits, relative position and draw bowser (finally!)
        a = M(Enemy_State + x);
    } while (a != 0); // if either enemy object not in normal state, branch to leave
    a = 0x0a;
    writeData(Enemy_BoundBoxCtrl + x, a); // set bounding box size control
    JSR(GetEnemyBoundBox, 363); // get bounding box coordinates
    goto PlayerEnemyCollision; // do player-to-enemy collision detection

SetFlameTimer:
    y = M(BowserFlameTimerCtrl); // load counter as offset
    ++M(BowserFlameTimerCtrl); // increment
    a = M(BowserFlameTimerCtrl); // mask out all but 3 LSB
    a &= 0b00000111; // to keep in range of 0-7
    writeData(BowserFlameTimerCtrl, a);
    a = M(FlameTimerData + y); // load value to be used then leave

    do // ExFl
    {
        goto Return;

    //------------------------------------------------------------------------

ProcBowserFlame:
        a = M(TimerControl); // if master timer control flag set,
        if (a != 0)
            goto SetGfxF; // skip all of this
        a = 0x40; // load default movement force
        y = M(SecondaryHardMode);
        if (y != 0)
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
        JSR(RelativeEnemyPosition, 364);
        a = M(Enemy_State + x); // if bowser's flame not in normal state,
    } while (a != 0); // branch to leave
    a = 0x51; // otherwise, continue
    writeData(0x00, a); // write first tile number
    y = 0x02; // load attributes without vertical flip by default
    a = M(FrameCounter);
    a &= 0b00000010; // invert vertical flip bit every 2 frames
    if (a != 0)
    { // if d1 not set, write default value
        y = 0x82; // otherwise write value with vertical flip bit set
    } // FlmeAt: set bowser's flame sprite attributes here
    writeData(0x01, y);
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    x = 0x00;

    do // DrawFlameLoop
    {
        a = M(Enemy_Rel_YPos); // get Y relative coordinate of current enemy object
        writeData(Sprite_Y_Position + y, a); // write into Y coordinate of OAM data
        a = M(0x00);
        writeData(Sprite_Tilenumber + y, a); // write current tile number into OAM data
        ++M(0x00); // increment tile number to draw more bowser's flame
        a = M(0x01);
        writeData(Sprite_Attributes + y, a); // write saved attributes into OAM data
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
    JSR(GetEnemyOffscreenBits, 365); // get offscreen information
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    a = M(Enemy_OffscreenBits); // get enemy object offscreen bits
    a >>= 1; // take d0, and push the rest to the stack
    pha();
    if ((M(Enemy_OffscreenBits) & 0x01) != 0)
    { // branch if it was not set
        a = 0xf8; // otherwise move sprite offscreen, this part likely
        writeData(Sprite_Y_Position + 12 + y, a); // residual since flame is only made of three sprites
    } // M3FOfs: get bits from stack
    pla();
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // take d1, and push the bits back to the stack
    pha();
    if (shiftedBit)
    { // branch if it was not set again
        a = 0xf8; // otherwise move third sprite offscreen
        writeData(Sprite_Y_Position + 8 + y, a);
    } // M2FOfs: get bits from stack again
    pla();
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // take d2, and push the bits back to the stack again
    pha();
    if (shiftedBit)
    { // branch if it was not set yet again
        a = 0xf8; // otherwise move second sprite offscreen
        writeData(Sprite_Y_Position + 4 + y, a);
    } // M1FOfs: get bits from stack one last time
    pla();
    shiftedBit = (a & 0x01) != 0;
    if (shiftedBit) // and d3
    { // branch if it was not set one last time
        a = 0xf8;
        writeData(Sprite_Y_Position + y, a); // otherwise move first sprite offscreen
    } // ExFlmeD: leave
    goto Return;

//------------------------------------------------------------------------

RunFireworks:
    --M(ExplosionTimerCounter + x); // decrement explosion timing counter here
    if (M(ExplosionTimerCounter + x) == 0)
    { // if not expired, skip this part
        a = 0x08;
        writeData(ExplosionTimerCounter + x, a); // reset counter
        ++M(ExplosionGfxCounter + x); // increment explosion graphics counter
        a = M(ExplosionGfxCounter + x);
        if (a >= 0x03)
            goto FireworksSoundScore; // if at a certain point, branch to kill this object
    } // SetupExpl: get relative coordinates of explosion
    JSR(RelativeEnemyPosition, 366);
    a = M(Enemy_Rel_YPos); // copy relative coordinates
    writeData(Fireball_Rel_YPos, a); // from the enemy object to the fireball object
    a = M(Enemy_Rel_XPos); // first vertical, then horizontal
    writeData(Fireball_Rel_XPos, a);
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    a = M(ExplosionGfxCounter + x); // get explosion graphics counter
    JSR(DrawExplosion_Fireworks, 367); // do a sub to draw the explosion then leave
    goto Return;

//------------------------------------------------------------------------

FireworksSoundScore:
    a = 0x00; // disable enemy buffer flag
    writeData(Enemy_Flag + x, a);
    a = Sfx_Blast; // play fireworks/gunfire sound
    writeData(Square2SoundQueue, a);
    a = 0x05; // set part of score modifier for 500 points
    writeData(DigitModifier + 4, a);
    goto EndAreaPoints; // jump to award points accordingly then leave

RunStarFlagObj:
    a = 0x00; // initialize enemy frenzy buffer
    writeData(EnemyFrenzyBuffer, a);
    a = M(StarFlagTaskControl); // check star flag object task number here
    if (a >= 0x05)
        goto StarFlagExit;
    switch (a)
    {
    case 0:
        goto StarFlagExit;
    case 1:
        goto GameTimerFireworks;
    case 2:
        goto AwardGameTimerPoints;
    case 3:
        goto RaiseFlagSetoffFWorks;
    case 4:
        goto DelayToAreaEnd;
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

StarFlagExit:
        goto Return; // leave

    //------------------------------------------------------------------------

AwardGameTimerPoints:
        a = M(GameTimerDisplay); // check all game timer digits for any intervals left
        a |= M(GameTimerDisplay + 1);
        a |= M(GameTimerDisplay + 2);
    } while (a == 0); // if no time left on game timer at all, branch to next task
    a = M(FrameCounter);
    a &= 0b00000100; // check frame counter for d2 set (skip ahead
    if (a != 0)
    { // for four frames every four frames) branch if not set
        a = Sfx_TimerTick;
        writeData(Square2SoundQueue, a); // load timer tick sound
    } // NoTTick: set offset here to subtract from game timer's last digit
    y = 0x23;
    a = 0xff; // set adder here to $ff, or -1, to subtract one
    writeData(DigitModifier + 5, a); // from the last digit of the game timer
    JSR(DigitsMathRoutine, 368); // subtract digit
    a = 0x05; // set now to add 50 points
    writeData(DigitModifier + 5, a); // per game timer interval subtracted

EndAreaPoints:
    y = 0x0b; // load offset for mario's score by default
    a = M(CurrentPlayer); // check player on the screen
    if (a != 0)
    { // if mario, do not change
        y = 0x11; // otherwise load offset for luigi's score
    } // ELPGive: award 50 points per game timer interval
    JSR(DigitsMathRoutine, 369);
    a = M(CurrentPlayer); // get player on the screen (or 500 points per
    a <<= 1; // fireworks explosion if branched here from there)
    a <<= 1; // shift to high nybble
    a <<= 1;
    a <<= 1;
    a |= 0b00000100; // add four to set nybble for game timer
    goto UpdateNumber; // jump to print the new score and game timer

RaiseFlagSetoffFWorks:
    a = M(Enemy_Y_Position + x); // check star flag's vertical position
    if (a >= 0x72)
    { // if star flag higher vertically, branch to other code
        --M(Enemy_Y_Position + x); // otherwise, raise star flag by one pixel
        goto DrawStarFlag; // and skip this part here
    } // SetoffF: check fireworks counter
    a = M(FireworksCounter);
    if (a == 0)
        goto DrawFlagSetTimer; // if no fireworks left to go off, skip this part
    if ((a & 0x80) != 0)
        goto DrawFlagSetTimer; // if no fireworks set to go off, skip this part
    a = Fireworks;
    writeData(EnemyFrenzyBuffer, a); // otherwise set fireworks object in frenzy queue

DrawStarFlag:
    JSR(RelativeEnemyPosition, 370); // get relative coordinates of star flag
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    x = 0x03; // do four sprites

    do // DSFLoop: get relative vertical coordinate
    {
        a = M(Enemy_Rel_YPos);
        a += M(StarFlagYPosAdder + x); // add Y coordinate adder data
        writeData(Sprite_Y_Position + y, a); // store as Y coordinate
        a = M(StarFlagTileData + x); // get tile number
        writeData(Sprite_Tilenumber + y, a); // store as tile number
        a = 0x22; // set palette and background priority bits
        writeData(Sprite_Attributes + y, a); // store as attributes
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
    goto Return;

//------------------------------------------------------------------------

DrawFlagSetTimer:
    JSR(DrawStarFlag, 371); // do sub to draw star flag
    a = 0x06;
    writeData(EnemyIntervalTimer + x, a); // set interval timer here

IncrementSFTask2:
    ++M(StarFlagTaskControl); // move onto next task
    goto Return;

//------------------------------------------------------------------------

DelayToAreaEnd:
    JSR(DrawStarFlag, 372); // do sub to draw star flag
    a = M(EnemyIntervalTimer + x); // if interval timer set in previous task
    if (a == 0)
    { // not yet expired, branch to leave
        a = M(EventMusicBuffer); // if event music buffer empty,
        if (a == 0)
            goto IncrementSFTask2; // branch to increment task
    } // StarFlagExit2
    goto Return; // otherwise leave

//------------------------------------------------------------------------

MovePiranhaPlant:
    a = M(Enemy_State + x); // check enemy state
    if (a != 0)
        goto PutinPipe; // if set at all, branch to leave
    a = M(EnemyFrameTimer + x); // check enemy's timer here
    if (a != 0)
        goto PutinPipe; // branch to end if not yet expired
    a = M(PiranhaPlant_MoveFlag + x); // check movement flag
    if (a == 0)
    { // if moving, skip to part ahead
        a = M(PiranhaPlant_Y_Speed + x); // if currently rising, branch
        if ((a & 0x80) == 0)
        { // to move enemy upwards out of pipe
            JSR(PlayerEnemyDiff, 373); // get horizontal difference between player and
            if ((a & 0x80) != 0)
            { // piranha plant, and branch if enemy to right of player
                a = M(0x00); // otherwise get saved horizontal difference
                a ^= 0xff;
                a += 0x01;
                writeData(0x00, a); // save as new horizontal difference
            } // ChkPlayerNearPipe
            a = M(0x00); // get saved horizontal difference
            if (a < 0x21)
                goto PutinPipe; // if player within a certain distance, branch to leave
        } // ReversePlantSpeed
        a = M(PiranhaPlant_Y_Speed + x); // get vertical speed
        a ^= 0xff;
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
    a = M(FrameCounter); // get frame counter
    a >>= 1;
    if ((M(FrameCounter) & 0x01) == 0)
        goto PutinPipe; // branch to leave if d0 set (execute code every other frame)
    a = M(TimerControl); // get master timer control
    if (a != 0)
        goto PutinPipe; // branch to leave if set (likely not necessary)
    a = M(Enemy_Y_Position + x); // get current vertical coordinate
    a += M(PiranhaPlant_Y_Speed + x); // add vertical speed to move up or down
    writeData(Enemy_Y_Position + x, a); // save as new vertical coordinate
    if (a != M(0x00))
        goto PutinPipe; // branch to leave if not yet reached
    a = 0x00;
    writeData(PiranhaPlant_MoveFlag + x, a); // otherwise clear movement flag
    a = 0x40;
    writeData(EnemyFrameTimer + x, a); // set timer to delay piranha plant movement

PutinPipe:
    a = 0b00100000; // set background priority bit in sprite
    writeData(Enemy_SprAttrib + x, a); // attributes to give illusion of being inside pipe
    goto Return; // then leave

//------------------------------------------------------------------------

FirebarSpin:
    writeData(0x07, a); // save spinning speed here
    a = M(FirebarSpinDirection + x); // check spinning direction
    if (a == 0)
    { // if moving counter-clockwise, branch to other part
        y = 0x18; // possibly residual ldy
        wide = ((M(FirebarSpinState_High + x) << 8) | M(FirebarSpinState_Low + x))
             + M(0x07); // add spinning speed to what would normally be the horizontal speed
        writeData(FirebarSpinState_Low + x, LOBYTE(wide));
        a = HIBYTE(wide); // what would normally be the vertical speed, never stored back
        goto Return;

    //------------------------------------------------------------------------
    } // SpinCounterClockwise
    y = 0x08; // possibly residual ldy
    wide = ((M(FirebarSpinState_High + x) << 8) | M(FirebarSpinState_Low + x))
         - M(0x07); // subtract spinning speed from what would normally be the horizontal speed
    writeData(FirebarSpinState_Low + x, LOBYTE(wide));
    a = HIBYTE(wide); // what would normally be the vertical speed, never stored back
    goto Return;

//------------------------------------------------------------------------

BalancePlatform:
    a = M(Enemy_Y_HighPos + x); // check high byte of vertical position
    if (a == 0x03)
    {
        goto EraseEnemyObject; // if far below screen, kill the object
    } // DoBPl: get object's state (set to $ff or other platform offset)
    a = M(Enemy_State + x);
    if ((a & 0x80) != 0)
    { // if doing other balance platform, branch to leave
        goto Return;

    //------------------------------------------------------------------------
    } // CheckBalPlatform
    y = a; // save offset from state as Y
    a = M(PlatformCollisionFlag + x); // get collision flag of platform
    writeData(0x00, a); // store here
    a = M(Enemy_MovingDir + x); // get moving direction
    if (a != 0)
    {
    } // ChkForFall
    else // if set, jump here
    {
        a = 0x2d; // check if platform is above a certain point
        if (a >= M(Enemy_Y_Position + x))
        { // if not, branch elsewhere
            if (y == M(0x00))
                goto MakePlatformFall; // enemy state, branch to make platforms fall
            a += 0x02; // otherwise add 2 pixels to vertical position
            writeData(Enemy_Y_Position + x, a); // of current platform and branch elsewhere
            goto StopPlatforms; // to make platforms stop

MakePlatformFall:
        } // ChkOtherForFall
        else // make platforms fall
        {
            if (a >= M(Enemy_Y_Position + y))
            { // if not, branch elsewhere
                if (x == M(0x00))
                    goto MakePlatformFall; // enemy state, branch to make platforms fall
                a += 0x02; // otherwise add 2 pixels to vertical position
                writeData(Enemy_Y_Position + y, a); // of other platform and branch elsewhere
                goto StopPlatforms; // jump to stop movement and do not return
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
            JSR(MovePlatformUp, 374);
            goto DoOtherPlatform; // jump ahead to remaining code

PlatSt: // do a sub to stop movement
            JSR(StopPlatforms, 375);
            goto DoOtherPlatform; // jump ahead to remaining code

PlatDn: // do a sub to move downwards
            JSR(MovePlatformDown, 376);

DoOtherPlatform:
            y = M(Enemy_State + x); // get offset of other platform
            pla(); // get old vertical coordinate from stack
            a -= M(Enemy_Y_Position + x); // get difference of old vs. new coordinate
            a += M(Enemy_Y_Position + y); // add difference to vertical coordinate of other
            writeData(Enemy_Y_Position + y, a); // platform to move it in the opposite direction
            a = M(PlatformCollisionFlag + x); // if no collision, skip this part here
            if ((a & 0x80) == 0)
            {
                x = a; // put offset which collision occurred here
                JSR(PositionPlayerOnVPlat, 377); // and use it to position player accordingly
            } // DrawEraseRope
            y = M(ObjectOffset); // get enemy object offset
            a = M(Enemy_Y_Speed + y); // check to see if current platform is
            a |= M(Enemy_Y_MoveForce + y); // moving at all
            if (a == 0)
                goto ExitRp; // if not, skip all of this and branch to leave
            x = M(VRAM_Buffer1_Offset); // get vram buffer offset
            if (x >= 0x20)
                goto ExitRp; // and skip this, branch to leave
            a = M(Enemy_Y_Speed + y);
            pha(); // save two copies of vertical speed to stack
            pha();
            JSR(SetupPlatformRope, 378); // do a sub to figure out where to put new bg tiles
            a = M(0x01); // write name table address to vram buffer
            writeData(VRAM_Buffer1 + x, a); // first the high byte, then the low
            a = M(0x00);
            writeData(VRAM_Buffer1 + 1 + x, a);
            a = 0x02; // set length for 2 bytes
            writeData(VRAM_Buffer1 + 2 + x, a);
            a = M(Enemy_Y_Speed + y); // if platform moving upwards, branch
            if ((a & 0x80) == 0)
            { // to do something else
                a = 0xa2;
                writeData(VRAM_Buffer1 + 3 + x, a); // otherwise put tile numbers for left
                a = 0xa3; // and right sides of rope in vram buffer
                writeData(VRAM_Buffer1 + 4 + x, a);
            } // EraseR1: put blank tiles in vram buffer
            else // jump to skip this part
            {
                a = 0x24;
                writeData(VRAM_Buffer1 + 3 + x, a); // to erase rope
                writeData(VRAM_Buffer1 + 4 + x, a);
            } // OtherRope
            a = M(Enemy_State + y); // get offset of other platform from state
            y = a; // use as Y here
            pla(); // pull second copy of vertical speed from stack
            a ^= 0xff; // invert bits to reverse speed
            JSR(SetupPlatformRope, 379); // do sub again to figure out where to put bg tiles
            a = M(0x01); // write name table address to vram buffer
            writeData(VRAM_Buffer1 + 5 + x, a); // this time we're doing putting tiles for
            a = M(0x00); // the other platform
            writeData(VRAM_Buffer1 + 6 + x, a);
            a = 0x02;
            writeData(VRAM_Buffer1 + 7 + x, a); // set length again for 2 bytes
            pla(); // pull first copy of vertical speed from stack
            if ((a & 0x80) != 0)
            { // if moving upwards (note inversion earlier), skip this
                a = 0xa2;
                writeData(VRAM_Buffer1 + 8 + x, a); // otherwise put tile numbers for left
                a = 0xa3; // and right sides of rope in vram
                writeData(VRAM_Buffer1 + 9 + x, a); // transfer buffer
            } // EraseR2: put blank tiles in vram buffer
            else // jump to skip this part
            {
                a = 0x24;
                writeData(VRAM_Buffer1 + 8 + x, a); // to erase rope
                writeData(VRAM_Buffer1 + 9 + x, a);
            } // EndRp: put null terminator at the end
            a = 0x00;
            writeData(VRAM_Buffer1 + 10 + x, a);
            a = M(VRAM_Buffer1_Offset); // add ten bytes to the vram buffer offset
            a += 10;
            writeData(VRAM_Buffer1_Offset, a);

ExitRp: // get enemy object buffer offset and leave
            x = M(ObjectOffset);
            goto Return;

        //------------------------------------------------------------------------

SetupPlatformRope:
            pha(); // save second/third copy to stack
            wide = M(Enemy_X_Position + y) + 0x08; // get horizontal coordinate, add eight pixels
            a = LOBYTE(wide);
            x = M(SecondaryHardMode); // if secondary hard mode flag set,
            if (x == 0)
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
            a = M(0x02); // get saved page location from earlier
            a &= 0x01; // mask out all but LSB
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
                a = M(0x00);
                a &= 0b10111111; // mask out d6 of low byte of name table address
                writeData(0x00, a);
            } // ExPRp: leave!
            goto Return;

        //------------------------------------------------------------------------
        } // InitPlatformFall
        a = y; // move offset of other platform from Y to X
        x = a;
        JSR(GetEnemyOffscreenBits, 380); // get offscreen bits
        a = 0x06;
        JSR(SetupFloateyNumber, 381); // award 1000 points to player
        a = M(Player_Rel_XPos);
        writeData(FloateyNum_X_Pos + x, a); // put floatey number coordinates where player is
        a = M(Player_Y_Position);
        writeData(FloateyNum_Y_Pos + x, a);
        a = 0x01; // set moving direction as flag for
        writeData(Enemy_MovingDir + x, a); // falling platforms

StopPlatforms:
        JSR(InitVStf, 382); // initialize vertical speed and low byte
        writeData(Enemy_Y_Speed + y, a); // for both platforms and leave
        writeData(Enemy_Y_MoveForce + y, a);
        goto Return;

    //------------------------------------------------------------------------
    } // PlatformFall
    a = y; // save offset for other platform to stack
    pha();
    JSR(MoveFallingPlatform, 383); // make current platform fall
    pla();
    x = a; // pull offset from stack and save to X
    JSR(MoveFallingPlatform, 384); // make other platform fall
    x = M(ObjectOffset);
    a = M(PlatformCollisionFlag + x); // if player not standing on either platform,
    if ((a & 0x80) == 0)
    { // skip this part
        x = a; // transfer collision flag offset as offset to X
        JSR(PositionPlayerOnVPlat, 385); // and position player appropriately
    } // ExPF: get enemy object buffer offset and leave
    x = M(ObjectOffset);
    goto Return;

//------------------------------------------------------------------------

YMovingPlatform:
    a = M(Enemy_Y_Speed + x); // if platform moving up or down, skip ahead to
    a |= M(Enemy_Y_MoveForce + x); // check on other position
    if (a != 0)
        goto ChkYCenterPos;
    writeData(Enemy_YMF_Dummy + x, a); // initialize dummy variable
    a = M(Enemy_Y_Position + x);
    if (a >= M(YPlatformTopYPos + x))
        goto ChkYCenterPos; // ahead of all this
    a = M(FrameCounter);
    a &= 0b00000111; // check for every eighth frame
    if (a == 0)
    {
        ++M(Enemy_Y_Position + x); // increase vertical position every eighth frame
    } // SkipIY: skip ahead to last part
    goto ChkYPCollision;

ChkYCenterPos:
    a = M(Enemy_Y_Position + x); // if current vertical position < central position, branch
    if (a >= M(YPlatformCenterYPos + x))
    {
        JSR(MovePlatformUp, 386); // otherwise start slowing descent/moving upwards
        goto ChkYPCollision;
    } // YMDown: start slowing ascent/moving downwards
    JSR(MovePlatformDown, 387);

ChkYPCollision:
    a = M(PlatformCollisionFlag + x); // if collision flag not set here, branch
    if ((a & 0x80) == 0)
    { // to leave
        JSR(PositionPlayerOnVPlat, 388); // otherwise position player appropriately
    } // ExYPl: leave
    goto Return;

//------------------------------------------------------------------------

XMovingPlatform:
    a = 0x0e; // load preset maximum value for secondary counter
    JSR(XMoveCntr_Platform, 389); // do a sub to increment counters for movement
    JSR(MoveWithXMCntrs, 390); // do a sub to move platform accordingly, and return value
    a = M(PlatformCollisionFlag + x); // if no collision with player,
    if ((a & 0x80) == 0)
    { // branch ahead to leave

PositionPlayerOnHPlat:
        y = M(0x00); // the saved value from the second subroutine is a signed adder
        wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + (int8_t)M(0x00);
        writeData(Player_X_Position, LOBYTE(wide)); // position player accordingly in horizontal position
        writeData(Player_PageLoc, HIBYTE(wide)); // SetPVar: save result to player's page location
        a = HIBYTE(wide);
        writeData(Platform_X_Scroll, y); // put saved value from second sub here to be used later
        JSR(PositionPlayerOnVPlat, 391); // position player vertically and appropriately
    } // ExXMP: and we are done here
    goto Return;

//------------------------------------------------------------------------

DropPlatform:
    a = M(PlatformCollisionFlag + x); // if no collision between platform and player
    if ((a & 0x80) == 0)
    { // occurred, just leave without moving anything
        JSR(MoveDropPlatform, 392); // otherwise do a sub to move platform down very quickly
        JSR(PositionPlayerOnVPlat, 393); // do a sub to position player appropriately
    } // ExDPl: leave
    goto Return;

//------------------------------------------------------------------------

RightPlatform:
    JSR(MoveEnemyHorizontally, 394); // move platform with current horizontal speed, if any
    writeData(0x00, a); // store saved value here (residual code)
    a = M(PlatformCollisionFlag + x); // check collision flag, if no collision between player
    if ((a & 0x80) == 0)
    { // and platform, branch ahead, leave speed unaltered
        a = 0x10;
        writeData(Enemy_X_Speed + x, a); // otherwise set new speed (gets moving if motionless)
        JSR(PositionPlayerOnHPlat, 395); // use saved value from earlier sub to position player
    } // ExRPl: then leave
    goto Return;

//------------------------------------------------------------------------

MoveLargeLiftPlat:
    JSR(MoveLiftPlatforms, 396); // execute common to all large and small lift platforms
    goto ChkYPCollision; // branch to position player correctly

MoveSmallPlatform:
    JSR(MoveLiftPlatforms, 397); // execute common to all large and small lift platforms
    goto ChkSmallPlatCollision; // branch to position player correctly

MoveLiftPlatforms:
    a = M(TimerControl); // if master timer control set, skip all of this
    if (a != 0)
        goto ExLiftP; // and branch to leave
    // position:dummy and speed:force are each one 16-bit quantity
    wide = ((M(Enemy_Y_Position + x) << 8) | M(Enemy_YMF_Dummy + x))
         + ((M(Enemy_Y_Speed + x) << 8) | M(Enemy_Y_MoveForce + x)); // move up or down
    writeData(Enemy_YMF_Dummy + x, LOBYTE(wide));
    writeData(Enemy_Y_Position + x, HIBYTE(wide)); // and then leave
    a = HIBYTE(wide);
    goto Return;

//------------------------------------------------------------------------

ChkSmallPlatCollision:
    a = M(PlatformCollisionFlag + x); // get bounding box counter saved in collision flag
    if (a == 0)
        goto ExLiftP; // if none found, leave player position alone
    JSR(PositionPlayerOnS_Plat, 398); // use to position player correctly

ExLiftP: // then leave
    goto Return;

//------------------------------------------------------------------------

OffscreenBoundsCheck:
    a = M(Enemy_ID + x); // check for cheep-cheep object
    if (a == FlyingCheepCheep)
        goto ExScrnBd;
    a = M(ScreenLeft_X_Pos); // get horizontal coordinate for left side of screen
    y = M(Enemy_ID + x);
    if (y != HammerBro)
    {
        carry = y >= PiranhaPlant; // this compare's carry is what ExtendLB subtracts with
        if (y != PiranhaPlant)
            goto ExtendLB; // these two will be erased sooner than others if too far left
    } // LimitB: add 57 pixels to coordinate if hammer bro or piranha plant
    // 56, plus the one carried in by the compare that sent us here, which found
    // the identifier equal and so always left the carry set
    wide = a + 0x39;
    a = LOBYTE(wide);
    carry = HIBYTE(wide) != 0; // and this add's carry is what ExtendLB subtracts with

    ExtendLB: // subtract 72 pixels regardless of enemy object
    wide = ((M(ScreenLeft_PageLoc) << 8) | a) - 0x48 - (carry ? 0 : 1);
    writeData(0x01, LOBYTE(wide)); // store result here
    writeData(0x00, HIBYTE(wide)); // store result here
    carry = (wide & 0x10000) == 0; // the left edge did not borrow
    // the original never clears the carry here either, so a left edge that did not
    // borrow pushes the right edge one pixel further out
    wide = ((M(ScreenRight_PageLoc) << 8) | M(ScreenRight_X_Pos))
         + 0x48 + (carry ? 1 : 0); // add 72 pixels to the right side horizontal coordinate
    writeData(0x03, LOBYTE(wide)); // store result here
    writeData(0x02, HIBYTE(wide)); // and store result here
    a = HIBYTE(wide);
    // the enemy object and the modified left edge are each one 16-bit page:coordinate
    wide = ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x))
         - ((M(0x00) << 8) | M(0x01));
    a = HIBYTE(wide);
    if ((a & 0x80) == 0)
    { // if enemy object is too far left, branch to erase it
        // the enemy object and the modified right edge are each one 16-bit page:coordinate
        wide = ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x))
             - ((M(0x02) << 8) | M(0x03));
        a = HIBYTE(wide);
        if ((a & 0x80) != 0)
            goto ExScrnBd; // if enemy object is on the screen, leave, do not erase enemy
        a = M(Enemy_State + x); // if at this point, enemy is offscreen to the right, so check
        if (a == HammerBro)
            goto ExScrnBd;
        if (y == PiranhaPlant)
            goto ExScrnBd;
        if (y == FlagpoleFlagObject)
            goto ExScrnBd;
        if (y == StarFlagObject)
            goto ExScrnBd;
        if (y == JumpspringObject)
            goto ExScrnBd; // erase all others too far to the right
    } // TooFar: erase object if necessary
    JSR(EraseEnemyObject, 399);

ExScrnBd: // leave
    goto Return;

//------------------------------------------------------------------------

FireballEnemyCollision:
    a = M(Fireball_State + x); // check to see if fireball state is set at all
    if (a == 0)
        goto ExitFBallEnemy; // branch to leave if not
    shiftedBit = (a & 0x80) != 0;
    a <<= 1;
    if (shiftedBit)
        goto ExitFBallEnemy; // branch to leave also if d7 in state is set
    a = M(FrameCounter);
    a >>= 1; // get LSB of frame counter
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
        a = M(Enemy_State + x);
        a &= 0b00100000; // check to see if d5 is set in enemy state
        if (a != 0)
            goto NoFToECol; // if so, skip to next enemy slot
        a = M(Enemy_Flag + x); // check to see if buffer flag is set
        if (a == 0)
            goto NoFToECol; // if not, skip to next enemy slot
        a = M(Enemy_ID + x); // check enemy identifier
        if (a >= 0x24)
        { // if < $24, branch to check further
            if (a < 0x2b)
                goto NoFToECol; // if in range $24-$2a, skip to next enemy slot
        } // GoombaDie: check for goomba identifier
        if (a == Goomba)
        { // if not found, continue with code
            a = M(Enemy_State + x); // otherwise check for defeated state
            if (a >= 0x02)
                goto NoFToECol; // skip to next enemy slot
        } // NotGoomba: if any masked offscreen bits set,
        a = M(EnemyOffscrBitsMasked + x);
        if (a != 0)
            goto NoFToECol; // skip to next enemy slot
        a = x;
        a <<= 1; // otherwise multiply enemy offset by four
        a <<= 1;
        a += 0x04; // add 4 bytes to it
        x = a; // to use enemy's bounding box coordinates
        JSR(SprObjectCollisionCore, 400); // do fireball-to-enemy collision detection
        x = M(ObjectOffset); // return fireball's original offset
        if (!collisionFound)
            goto NoFToECol; // no collision, thus do next enemy slot
        a = 0b10000000;
        writeData(Fireball_State + x, a); // set d7 in enemy state
        x = M(0x01); // get enemy offset
        JSR(HandleEnemyFBallCol, 401); // jump to handle fireball to enemy collision

NoFToECol: // pull fireball offset from stack
        pla();
        y = a; // put it in Y
        x = M(0x01); // get enemy object offset
        --x; // decrement it
    } while ((x & 0x80) == 0); // loop back until collision detection done on all enemies

ExitFBallEnemy:
    x = M(ObjectOffset); // get original fireball offset and leave
    goto Return;

//------------------------------------------------------------------------

HandleEnemyFBallCol:
    JSR(RelativeEnemyPosition, 402); // get relative coordinate of enemy
    x = M(0x01); // get current enemy object offset
    a = M(Enemy_Flag + x); // check buffer flag for d7 set
    if ((a & 0x80) != 0)
    { // branch if not set to continue
        a &= 0b00001111; // otherwise mask out high nybble and
        x = a; // use low nybble as enemy offset
        a = M(Enemy_ID + x);
        if (a == Bowser)
            goto HurtBowser; // branch if found
        x = M(0x01); // otherwise retrieve current enemy offset
    } // ChkBuzzyBeetle
    a = M(Enemy_ID + x);
    if (a == BuzzyBeetle)
        goto ExHCF; // branch if found to leave (buzzy beetles fireproof)
    if (a == Bowser)
    { // if not found, branch to check other enemies

HurtBowser:
        --M(BowserHitPoints); // decrement bowser's hit points
        if (M(BowserHitPoints) != 0)
            goto ExHCF; // if bowser still has hit points, branch to leave
        JSR(InitVStf, 403); // otherwise do sub to init vertical speed and movement force
        writeData(Enemy_X_Speed + x, a); // initialize horizontal speed
        writeData(EnemyFrenzyBuffer, a); // init enemy frenzy buffer
        a = 0xfe;
        writeData(Enemy_Y_Speed + x, a); // set vertical speed to make defeated bowser jump a little
        y = M(WorldNumber); // use world number as offset
        a = M(BowserIdentities + y); // get enemy identifier to replace bowser with
        writeData(Enemy_ID + x, a); // set as new enemy identifier
        a = 0x20; // set A to use starting value for state
        if (y < 0x03)
        { // branch if so
            a |= 0x03; // otherwise add 3 to enemy state
        } // SetDBSte: set defeated enemy state
        writeData(Enemy_State + x, a);
        a = Sfx_BowserFall;
        writeData(Square2SoundQueue, a); // load bowser defeat sound
        x = M(0x01); // get enemy offset
        a = 0x09; // award 5000 points to player for defeating bowser
        if (a != 0)
            goto EnemySmackScore; // unconditional branch to award points
    } // ChkOtherEnemies
    if (a == BulletBill_FrenzyVar)
        goto ExHCF; // branch to leave if bullet bill (frenzy variant)
    if (a == Podoboo)
        goto ExHCF; // branch to leave if podoboo
    if (a >= 0x15)
        goto ExHCF; // branch to leave if identifier => $15

ShellOrBlockDefeat:
    a = M(Enemy_ID + x); // check for piranha plant
    if (a == PiranhaPlant)
    { // branch if not found
        a = M(Enemy_Y_Position + x);
        a += 0x19; // add 24 pixels, plus the one carried in by the compare above
        writeData(Enemy_Y_Position + x, a);
    } // StnE: do yet another sub
    JSR(ChkToStunEnemies, 404);
    a = M(Enemy_State + x);
    a &= 0b00011111; // mask out 2 MSB of enemy object's state
    a |= 0b00100000; // set d5 to defeat enemy and save as new state
    writeData(Enemy_State + x, a);
    a = 0x02; // award 200 points by default
    y = M(Enemy_ID + x); // check for hammer bro
    if (y == HammerBro)
    { // branch if not found
        a = 0x06; // award 1000 points for hammer bro
    } // GoombaPoints
    if (y != Goomba)
        goto EnemySmackScore; // branch if not found
    a = 0x01; // award 100 points for goomba

EnemySmackScore:
    JSR(SetupFloateyNumber, 405); // update necessary score variables
    a = Sfx_EnemySmack; // play smack enemy sound
    writeData(Square1SoundQueue, a);

ExHCF: // and now let's leave
    goto Return;

//------------------------------------------------------------------------

PlayerHammerCollision:
    a = M(FrameCounter); // get frame counter
    a >>= 1; // take d0
    if ((M(FrameCounter) & 0x01) == 0)
        goto ExPHC; // branch to leave if d0 not set to execute every other frame
    a = M(TimerControl); // if either master timer control
    a |= M(Misc_OffscreenBits); // or any offscreen bits for hammer are set,
    if (a != 0)
        goto ExPHC; // branch to leave
    a = x;
    a <<= 1; // multiply misc object offset by four
    a <<= 1;
    a += 0x24; // add 36 or $24 bytes to get proper offset
    y = a; // for misc object bounding box coordinates
    JSR(PlayerCollisionCore, 406); // do player-to-hammer collision detection
    x = M(ObjectOffset); // get misc object offset
    if (collisionFound)
    { // if no collision, then branch
        a = M(Misc_Collision_Flag + x); // otherwise read collision flag
        if (a != 0)
            goto ExPHC; // if collision flag already set, branch to leave
        a = 0x01;
        writeData(Misc_Collision_Flag + x, a); // otherwise set collision flag now
        a = M(Misc_X_Speed + x);
        a ^= 0xff; // get two's compliment of
        a += 0x01;
        writeData(Misc_X_Speed + x, a); // set to send hammer flying the opposite direction
        a = M(StarInvincibleTimer); // if star mario invincibility timer set,
        if (a != 0)
            goto ExPHC; // branch to leave
        goto InjurePlayer; // otherwise jump to hurt player, do not return
    } // ClHCol: clear collision flag
    a = 0x00;
    writeData(Misc_Collision_Flag + x, a);

ExPHC:
    goto Return;

//------------------------------------------------------------------------

HandlePowerUpCollision:
    JSR(EraseEnemyObject, 407); // erase the power-up object
    a = 0x06;
    JSR(SetupFloateyNumber, 408); // award 1000 points to player by default
    a = Sfx_PowerUpGrab;
    writeData(Square2SoundQueue, a); // play the power-up sound
    a = M(PowerUpType); // check power-up type
    if (a >= 0x02)
    { // if mushroom or fire flower, branch
        if (a == 0x03)
            goto SetFor1Up; // if 1-up mushroom, branch
        a = 0x23; // otherwise set star mario invincibility
        writeData(StarInvincibleTimer, a); // timer, and load the star mario music
        a = StarPowerMusic; // into the area music queue, then leave
        writeData(AreaMusicQueue, a);
        goto Return;

    //------------------------------------------------------------------------
    } // Shroom_Flower_PUp
    a = M(PlayerStatus); // if player status = small, branch
    if (a != 0)
    {
        if (a != 0x01)
            goto NoPUp;
        x = M(ObjectOffset); // get enemy offset, not necessary
        a = 0x02; // set player status to fiery
        writeData(PlayerStatus, a);
        JSR(GetPlayerColors, 409); // run sub to change colors of player
        x = M(ObjectOffset); // get enemy offset again, and again not necessary
        a = 0x0c; // set value to be used by subroutine tree (fiery)
        goto UpToFiery; // jump to set values accordingly

SetFor1Up:
        a = 0x0b; // change 1000 points into 1-up instead
        writeData(FloateyNum_Control + x, a); // and then leave
        goto Return;

    //------------------------------------------------------------------------
    } // UpToSuper
    a = 0x01; // set player status to super
    writeData(PlayerStatus, a);
    a = 0x09; // set value to be used by subroutine tree (super)

UpToFiery:
    y = 0x00; // set value to be used as new player state
    JSR(SetPRout, 410); // set values to stop certain things in motion

NoPUp:
    goto Return;

//------------------------------------------------------------------------

PlayerEnemyCollision:
    a = M(FrameCounter); // check counter for d0 set
    a >>= 1;
    if ((M(FrameCounter) & 0x01) != 0)
        goto NoPUp; // if set, branch to leave
    JSR(CheckPlayerVertical, 411); // if player object is completely offscreen or
    if (playerVerticalOutOfRange)
        goto NoPECol; // if down past 224th pixel row, branch to leave
    a = M(EnemyOffscrBitsMasked + x); // if current enemy is offscreen by any amount,
    if (a != 0)
        goto NoPECol; // go ahead and branch to leave
    a = M(GameEngineSubroutine);
    if (a != 0x08)
        goto NoPECol; // on next frame, branch to leave
    a = M(Enemy_State + x);
    a &= 0b00100000; // if enemy state has d5 set, branch to leave
    if (a != 0)
        goto NoPECol;
    JSR(GetEnemyBoundBoxOfs, 412); // get bounding box offset for current enemy object
    JSR(PlayerCollisionCore, 413); // do collision detection on player vs. enemy
    x = M(ObjectOffset); // get enemy object buffer offset
    if (!collisionFound)
    { // if collision, branch past this part here
        a = M(Enemy_CollisionBits + x);
        a &= 0b11111110; // otherwise, clear d0 of current enemy object's
        writeData(Enemy_CollisionBits + x, a); // collision bit

NoPECol:
        goto Return;

    //------------------------------------------------------------------------
    } // CheckForPUpCollision
    y = M(Enemy_ID + x);
    if (y == PowerUpObject)
    { // if not found, branch to next part
        goto HandlePowerUpCollision; // otherwise, unconditional jump backwards
    } // EColl: if star mario invincibility timer expired,
    a = M(StarInvincibleTimer);
    if (a != 0)
    { // perform task here, otherwise kill enemy like
        goto ShellOrBlockDefeat; // hit with a shell, or from beneath
    } // HandlePECollisions
    a = M(Enemy_CollisionBits + x); // check enemy collision bits for d0 set
    a &= 0b00000001; // or for being offscreen at all
    a |= M(EnemyOffscrBitsMasked + x);
    if (a != 0)
        goto ExPEC; // branch to leave if either is true
    a = 0x01;
    a |= M(Enemy_CollisionBits + x); // otherwise set d0 now
    writeData(Enemy_CollisionBits + x, a);
    if (y == Spiny)
        goto ChkForPlayerInjury;
    if (y == PiranhaPlant)
        goto InjurePlayer;
    if (y == Podoboo)
        goto InjurePlayer;
    if (y == BulletBill_CannonVar)
        goto ChkForPlayerInjury;
    if (y >= 0x15)
        goto InjurePlayer;
    a = M(AreaType); // branch if water type level
    if (a == 0)
        goto InjurePlayer;
    a = M(Enemy_State + x); // branch if d7 of enemy state was set
    a <<= 1;
    if ((M(Enemy_State + x) & 0x80) != 0)
        goto ChkForPlayerInjury;
    a = M(Enemy_State + x); // mask out all but 3 LSB of enemy state
    a &= 0b00000111;
    if (a < 0x02)
        goto ChkForPlayerInjury;
    a = M(Enemy_ID + x); // branch to leave if goomba in defeated state
    if (a == Goomba)
        goto ExPEC;
    a = Sfx_EnemySmack; // play smack enemy sound
    writeData(Square1SoundQueue, a);
    a = M(Enemy_State + x); // set d7 in enemy state, thus become moving shell
    a |= 0b10000000;
    writeData(Enemy_State + x, a);
    JSR(EnemyFacePlayer, 414); // set moving direction and get offset
    a = M(KickedShellXSpdData + y); // load and set horizontal speed data with offset
    writeData(Enemy_X_Speed + x, a);
    a = 0x03; // add three to whatever the stomp counter contains
    a += M(StompChainCounter);
    y = M(EnemyIntervalTimer + x); // check shell enemy's timer
    if (y < 0x03)
    { // data obtained from the stomp counter + 3
        a = M(KickedShellPtsData + y); // otherwise, set points based on proximity to timer expiration
    } // KSPts: set values for floatey number now
    JSR(SetupFloateyNumber, 415);

ExPEC: // leave!!!
    goto Return;

//------------------------------------------------------------------------

ChkForPlayerInjury:
    a = M(Player_Y_Speed); // check player's vertical speed
    if ((a & 0x80) == 0)
    { // perform procedure below if player moving upwards
        if (a != 0)
            goto EnemyStomped; // or not at all, and branch elsewhere if moving downwards
    } // ChkInj: branch if enemy object < $07
    a = M(Enemy_ID + x);
    if (a >= Bloober)
    {
        a = M(Player_Y_Position); // add 12 pixels to player's vertical position
        a += 0x0c;
        if (a < M(Enemy_Y_Position + x))
            goto EnemyStomped; // branch if this player's position above (less than) enemy's
    } // ChkETmrs: check stomp timer
    a = M(StompTimer);
    if (a != 0)
        goto EnemyStomped; // branch if set
    a = M(InjuryTimer); // check to see if injured invincibility timer still
    if (a != 0)
        goto ExInjColRoutines; // counting down, and branch elsewhere to leave if so
    a = M(Player_Rel_XPos);
    if (a >= M(Enemy_Rel_XPos))
    { // relative position, branch here
    } // TInjE: if enemy moving towards the left,
    else // otherwise do a jump here
    {
        a = M(Enemy_MovingDir + x);
        if (a != 0x01)
            goto InjurePlayer; // to turn the enemy around
        goto LInj;

InjurePlayer:
        a = M(InjuryTimer); // check again to see if injured invincibility timer is
        if (a != 0)
            goto ExInjColRoutines; // at zero, and branch to leave if so

ForceInjury:
        x = M(PlayerStatus); // check player's status
        if (x != 0)
        { // branch if small
            writeData(PlayerStatus, a); // otherwise set player's status to small
            a = 0x08;
            writeData(InjuryTimer, a); // set injured invincibility timer
            a <<= 1;
            writeData(Square1SoundQueue, a); // play pipedown/injury sound
            JSR(GetPlayerColors, 416); // change player's palette if necessary
            a = 0x0a; // set subroutine to run on next frame

SetKRout: // set new player state
            y = 0x01;

SetPRout: // load new value to run subroutine on next frame
            writeData(GameEngineSubroutine, a);
            writeData(Player_State, y); // store new player state
            y = 0xff;
            writeData(TimerControl, y); // set master timer control flag to halt timers
            ++y;
            writeData(ScrollAmount, y); // initialize scroll speed

ExInjColRoutines:
            x = M(ObjectOffset); // get enemy offset and leave
            goto Return;

        //------------------------------------------------------------------------
        } // KillPlayer
        writeData(Player_X_Speed, x); // halt player's horizontal movement by initializing speed
        ++x;
        writeData(EventMusicQueue, x); // set event music queue to death music
        a = 0xfc;
        writeData(Player_Y_Speed, a); // set new vertical speed
        a = 0x0b; // set subroutine to run on next frame
        if (a != 0)
            goto SetKRout; // branch to set player's state and other things

EnemyStomped:
        a = M(Enemy_ID + x); // check for spiny, branch to hurt player
        if (a == Spiny)
            goto InjurePlayer;
        a = Sfx_EnemyStomp; // otherwise play stomp/swim sound
        writeData(Square1SoundQueue, a);
        a = M(Enemy_ID + x);
        y = 0x00; // initialize points data offset for stomped enemies
        if (a == FlyingCheepCheep)
            goto EnemyStompedPts;
        if (a == BulletBill_FrenzyVar)
            goto EnemyStompedPts;
        if (a == BulletBill_CannonVar)
            goto EnemyStompedPts;
        if (a == Podoboo)
            goto EnemyStompedPts; // for cpu to take due to earlier checking of podoboo)
        ++y; // increment points data offset
        if (a == HammerBro)
            goto EnemyStompedPts;
        ++y; // increment points data offset
        if (a == Lakitu)
            goto EnemyStompedPts;
        ++y; // increment points data offset
        if (a == Bloober)
        {

EnemyStompedPts:
            a = M(StompedEnemyPtsData + y); // load points data using offset in Y
            JSR(SetupFloateyNumber, 417); // run sub to set floatey number controls
            a = M(Enemy_MovingDir + x);
            pha(); // save enemy movement direction to stack
            JSR(SetStun, 418); // run sub to kill enemy
            pla();
            writeData(Enemy_MovingDir + x, a); // return enemy movement direction from stack
            a = 0b00100000;
            writeData(Enemy_State + x, a); // set d5 in enemy state
            JSR(InitVStf, 419); // nullify vertical speed, physics-related thing,
            writeData(Enemy_X_Speed + x, a); // and horizontal speed
            a = 0xfd; // set player's vertical speed, to give bounce
            writeData(Player_Y_Speed, a);
            goto Return;

        //------------------------------------------------------------------------
        } // ChkForDemoteKoopa
        if (a >= 0x09)
        {
            a &= 0b00000001; // demote koopa paratroopas to ordinary troopas
            writeData(Enemy_ID + x, a);
            y = 0x00; // return enemy to normal state
            writeData(Enemy_State + x, y);
            a = 0x03; // award 400 points to the player
            JSR(SetupFloateyNumber, 420);
            JSR(InitVStf, 421); // nullify physics-related thing and vertical speed
            JSR(EnemyFacePlayer, 422); // turn enemy around if necessary
            a = M(DemotedKoopaXSpdData + y);
            writeData(Enemy_X_Speed + x, a); // set appropriate moving speed based on direction
        } // HandleStompedShellE
        else // then move onto something else
        {
            a = 0x04; // set defeated state for enemy
            writeData(Enemy_State + x, a);
            ++M(StompChainCounter); // increment the stomp counter
            a = M(StompChainCounter); // add whatever is in the stomp counter
            a += M(StompTimer);
            JSR(SetupFloateyNumber, 423); // award points accordingly
            ++M(StompTimer); // increment stomp timer of some sort
            y = M(PrimaryHardMode); // check primary hard mode flag
            a = M(RevivalRateData + y); // load timer setting according to flag
            writeData(EnemyIntervalTimer + x, a); // set as enemy timer to revive stomped enemy
        } // SBnce: set player's vertical speed for bounce
        a = 0xfc;
        writeData(Player_Y_Speed, a); // and then leave!!!
        goto Return;

    //------------------------------------------------------------------------
    } // ChkEnemyFaceRight
    a = M(Enemy_MovingDir + x); // check to see if enemy is moving to the right
    if (a != 0x01)
        goto LInj; // if not, branch
    goto InjurePlayer; // otherwise go back to hurt player

LInj: // turn the enemy around, if necessary
    JSR(EnemyTurnAround, 424);
    goto InjurePlayer; // go back to hurt player

EnemyFacePlayer:
    y = 0x01; // set to move right by default
    JSR(PlayerEnemyDiff, 425); // get horizontal difference between player and enemy
    if ((a & 0x80) != 0)
    { // if enemy is to the right of player, do not increment
        ++y; // otherwise, increment to set to move to the left
    } // SFcRt: set moving direction here
    writeData(Enemy_MovingDir + x, y);
    --y; // then decrement to use as a proper offset
    goto Return;

//------------------------------------------------------------------------

SetupFloateyNumber:
    writeData(FloateyNum_Control + x, a); // set number of points control for floatey numbers
    a = 0x30;
    writeData(FloateyNum_Timer + x, a); // set timer for floatey numbers
    a = M(Enemy_Y_Position + x);
    writeData(FloateyNum_Y_Pos + x, a); // set vertical coordinate
    a = M(Enemy_Rel_XPos);
    writeData(FloateyNum_X_Pos + x, a); // set horizontal coordinate and leave

ExSFN:
    goto Return;

//------------------------------------------------------------------------

EnemiesCollision:
    a = M(FrameCounter); // check counter for d0 set
    a >>= 1;
    if ((M(FrameCounter) & 0x01) == 0)
        goto ExSFN; // if d0 not set, leave
    a = M(AreaType);
    if (a == 0)
        goto ExSFN; // if water area type, leave
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
    JSR(GetEnemyBoundBoxOfs, 426); // otherwise, do sub, get appropriate bounding box offset for
    --x; // first enemy we're going to compare, then decrement for second
    if ((x & 0x80) != 0)
        goto ExitECRoutine; // branch to leave if there are no other enemies

    do // ECLoop: save enemy object buffer offset for second enemy here
    {
        writeData(0x01, x);
        a = y; // save first enemy's bounding box offset to stack
        pha();
        a = M(Enemy_Flag + x); // check enemy object enable flag
        if (a == 0)
            goto ReadyNextEnemy; // branch if flag not set
        a = M(Enemy_ID + x);
        if (a >= 0x15)
            goto ReadyNextEnemy; // branch if true
        if (a == Lakitu)
            goto ReadyNextEnemy; // branch if enemy object is lakitu
        if (a == PiranhaPlant)
            goto ReadyNextEnemy; // branch if enemy object is piranha plant
        a = M(EnemyOffscrBitsMasked + x);
        if (a != 0)
            goto ReadyNextEnemy; // branch if masked offscreen bits set
        a = x; // get second enemy object's bounding box offset
        a <<= 1; // multiply by four, then add four
        a <<= 1;
        a += 0x04;
        x = a; // use as new contents of X
        JSR(SprObjectCollisionCore, 427); // do collision detection using the two enemies here
        x = M(ObjectOffset); // use first enemy offset for X
        y = M(0x01); // use second enemy offset for Y
        if (collisionFound)
        { // no collision, branch ahead of this
            a = M(Enemy_State + x);
            a |= M(Enemy_State + y); // check both enemy states for d7 set
            a &= 0b10000000;
            if (a == 0)
            { // branch if at least one of them is set
                a = M(Enemy_CollisionBits + y); // load first enemy's collision-related bits
                a &= M(SetBitsMask + x); // check to see if bit connected to second enemy is
                if (a != 0)
                    goto ReadyNextEnemy; // already set, and move onto next enemy slot if set
                a = M(Enemy_CollisionBits + y);
                a |= M(SetBitsMask + x); // if the bit is not set, set it now
                writeData(Enemy_CollisionBits + y, a);
            } // YesEC: react according to the nature of collision
            JSR(ProcEnemyCollisions, 428);
            goto ReadyNextEnemy; // move onto next enemy slot
        } // NoEnemyCollision
        a = M(Enemy_CollisionBits + y); // load first enemy's collision-related bits
        a &= M(ClearBitsMask + x); // clear bit connected to second enemy
        writeData(Enemy_CollisionBits + y, a); // then move onto next enemy slot

ReadyNextEnemy:
        pla(); // get first enemy's bounding box offset from the stack
        y = a; // use as Y again
        x = M(0x01); // get and decrement second enemy's object buffer offset
        --x;
    } while ((x & 0x80) == 0); // loop until all enemy slots have been checked

ExitECRoutine:
    x = M(ObjectOffset); // get enemy object buffer offset
    goto Return; // leave

//------------------------------------------------------------------------

ProcEnemyCollisions:
    a = M(Enemy_State + y); // check both enemy states for d5 set
    a |= M(Enemy_State + x);
    a &= 0b00100000; // if d5 is set in either state, or both, branch
    if (a != 0)
        goto ExitProcessEColl; // to leave and do nothing else at this point
    a = M(Enemy_State + x);
    if (a >= 0x06)
    {
        a = M(Enemy_ID + x); // check second enemy identifier for hammer bro
        if (a == HammerBro)
            goto ExitProcessEColl;
        if ((M(Enemy_State + y) & 0x80) != 0) // check first enemy state for d7 set
        { // branch if d7 is clear
            a = 0x06;
            JSR(SetupFloateyNumber, 429); // award 1000 points for killing enemy
            JSR(ShellOrBlockDefeat, 430); // then kill enemy, then load
            y = M(0x01); // original offset of second enemy
        } // ShellCollisions
        a = y; // move Y to X
        x = a;
        JSR(ShellOrBlockDefeat, 431); // kill second enemy
        x = M(ObjectOffset);
        a = M(ShellChainCounter + x); // get chain counter for shell
        a += 0x04; // add four to get appropriate point offset
        x = M(0x01);
        JSR(SetupFloateyNumber, 432); // award appropriate number of points for second enemy
        x = M(ObjectOffset); // load original offset of first enemy
        ++M(ShellChainCounter + x); // increment chain counter for additional enemies

ExitProcessEColl:
        goto Return; // leave!!!

    //------------------------------------------------------------------------
    } // ProcSecondEnemyColl
    a = M(Enemy_State + y); // if first enemy state < $06, branch elsewhere
    if (a >= 0x06)
    {
        a = M(Enemy_ID + y); // check first enemy identifier for hammer bro
        if (a == HammerBro)
            goto ExitProcessEColl;
        JSR(ShellOrBlockDefeat, 433); // otherwise, kill first enemy
        y = M(0x01);
        a = M(ShellChainCounter + y); // get chain counter for shell
        a += 0x04; // add four to get appropriate point offset
        x = M(ObjectOffset);
        JSR(SetupFloateyNumber, 434); // award appropriate number of points for first enemy
        x = M(0x01); // load original offset of second enemy
        ++M(ShellChainCounter + x); // increment chain counter for additional enemies
        goto Return; // leave!!!

    //------------------------------------------------------------------------
    } // MoveEOfs
    a = y; // move Y ($01) to X
    x = a;
    JSR(EnemyTurnAround, 435); // do the sub here using value from $01
    x = M(ObjectOffset); // then do it again using value from $08

EnemyTurnAround:
    a = M(Enemy_ID + x); // check for specific enemies
    if (a == PiranhaPlant)
        goto ExTA; // if piranha plant, leave
    if (a == Lakitu)
        goto ExTA; // if lakitu, leave
    if (a == HammerBro)
        goto ExTA; // if hammer bro, leave
    if (a == Spiny)
        goto RXSpd; // if spiny, turn it around
    if (a == GreenParatroopaJump)
        goto RXSpd; // if green paratroopa, turn it around
    if (a >= 0x07)
        goto ExTA; // if any OTHER enemy object => $07, leave

RXSpd: // load horizontal speed
    a = M(Enemy_X_Speed + x);
    a ^= 0xff; // get two's compliment for horizontal speed
    y = a;
    ++y;
    writeData(Enemy_X_Speed + x, y); // store as new horizontal speed
    a = M(Enemy_MovingDir + x);
    a ^= 0b00000011; // invert moving direction and store, then leave
    writeData(Enemy_MovingDir + x, a); // thus effectively turning the enemy around

ExTA: // leave!!!
    goto Return;

//------------------------------------------------------------------------

LargePlatformCollision:
    a = 0xff; // save value here
    writeData(PlatformCollisionFlag + x, a);
    a = M(TimerControl); // check master timer control
    if (a != 0)
        goto ExLPC; // if set, branch to leave
    a = M(Enemy_State + x); // if d7 set in object state,
    if ((a & 0x80) != 0)
        goto ExLPC; // branch to leave
    a = M(Enemy_ID + x);
    if (a != 0x24)
        goto ChkForPlayerC_LargeP; // balance platform, branch if not found
    a = M(Enemy_State + x);
    x = a; // set state as enemy offset here
    JSR(ChkForPlayerC_LargeP, 436); // perform code with state offset, then original offset, in X

ChkForPlayerC_LargeP:
    JSR(CheckPlayerVertical, 437); // figure out if player is below a certain point
    if (playerVerticalOutOfRange)
        goto ExLPC; // or offscreen, branch to leave if true
    a = x;
    JSR(GetEnemyBoundBoxOfsArg, 438); // get bounding box offset in Y
    a = M(Enemy_Y_Position + x); // store vertical coordinate in
    writeData(0x00, a); // temp variable for now
    a = x; // send offset we're on to the stack
    pha();
    JSR(PlayerCollisionCore, 439); // do player-to-platform collision detection
    pla(); // retrieve offset from the stack
    x = a;
    if (!collisionFound)
        goto ExLPC; // if no collision, branch to leave
    JSR(ProcLPlatCollisions, 440); // otherwise collision, perform sub

ExLPC: // get enemy object buffer offset and leave
    x = M(ObjectOffset);
    goto Return;

//------------------------------------------------------------------------

SmallPlatformCollision:
    a = M(TimerControl); // if master timer control set,
    if (a != 0)
        goto ExSPC; // branch to leave
    writeData(PlatformCollisionFlag + x, a); // otherwise initialize collision flag
    JSR(CheckPlayerVertical, 441); // do a sub to see if player is below a certain point
    if (playerVerticalOutOfRange)
        goto ExSPC; // or entirely offscreen, and branch to leave if true
    a = 0x02;
    writeData(0x00, a); // load counter here for 2 bounding boxes

    do // ChkSmallPlatLoop
    {
        x = M(ObjectOffset); // get enemy object offset
        JSR(GetEnemyBoundBoxOfs, 442); // get bounding box offset in Y
        a &= 0b00000010; // if d1 of offscreen lower nybble bits was set
        if (a != 0)
            goto ExSPC; // then branch to leave
        a = M(BoundingBox_UL_YPos + y); // check top of platform's bounding box for being
        if (a >= 0x20)
        { // if so, branch, don't do collision detection
            JSR(PlayerCollisionCore, 443); // otherwise, perform player-to-platform collision detection
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
    goto Return;

//------------------------------------------------------------------------

ProcSPlatCollisions:
    x = M(ObjectOffset); // return enemy object buffer offset to X, then continue

ProcLPlatCollisions:
    a = M(BoundingBox_DR_YPos + y); // get difference by subtracting the top
    a -= M(BoundingBox_UL_YPos); // of the platform's bounding box
    if (a >= 0x04)
        goto ChkForTopCollision; // branch, do not alter vertical speed of player
    a = M(Player_Y_Speed); // check to see if player's vertical speed is moving down
    if ((a & 0x80) == 0)
        goto ChkForTopCollision; // if so, don't mess with it
    a = 0x01; // otherwise, set vertical
    writeData(Player_Y_Speed, a); // speed of player to kill jump

ChkForTopCollision:
    a = M(BoundingBox_DR_YPos); // get difference by subtracting the top
    a -= M(BoundingBox_UL_YPos + y); // of the player's bounding box
    if (a >= 0x06)
        goto PlatformSideCollisions; // if difference not close enough, skip all of this
    a = M(Player_Y_Speed);
    if ((a & 0x80) != 0)
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
    writeData(Player_State, a); // set player state to normal then leave
    goto Return;

//------------------------------------------------------------------------

PlatformSideCollisions:
    a = 0x01; // set value here to indicate possible horizontal
    writeData(0x00, a); // collision on left side of platform
    a = M(BoundingBox_DR_XPos); // get difference by subtracting platform's left edge
    a -= M(BoundingBox_UL_XPos + y);
    if (a >= 0x08)
    {
        ++M(0x00); // otherwise increment value set here for right side collision
        a = M(BoundingBox_DR_XPos + y); // get difference by subtracting player's left edge
        // the original clears the carry rather than setting it here, so the
        // subtraction takes one pixel more than it means to
        a = (uint8_t)(a - M(BoundingBox_UL_XPos) - 1); // from platform's right edge
        if (a >= 0x09)
            goto NoSideC; // and instead branch to leave (no collision)
    } // SideC: deal with horizontal collision
    JSR(ImpedePlayerMove, 444);

NoSideC: // return with enemy object buffer offset
    x = M(ObjectOffset);
    goto Return;

//------------------------------------------------------------------------

PositionPlayerOnS_Plat:
    y = a; // use bounding box counter saved in collision flag
    a = M(Enemy_Y_Position + x); // for offset
    a += M(PlayerPosSPlatData - 1 + y); // coordinate
    goto Skip_8;

PositionPlayerOnVPlat:
    a = M(Enemy_Y_Position + x); // get vertical coordinate
Skip_8:
    y = M(GameEngineSubroutine);
    if (y == 0x0b)
        goto ExPlPos; // skip all of this
    y = M(Enemy_Y_HighPos + x);
    if (y != 0x01)
        goto ExPlPos;
    wide = ((y << 8) | a) - 0x20; // subtract 32 pixels from the vertical coordinate for the player object's height
    writeData(Player_Y_Position, LOBYTE(wide)); // save as player's new vertical coordinate
    writeData(Player_Y_HighPos, HIBYTE(wide)); // and store as player's new vertical high byte
    a = HIBYTE(wide);
    a = 0x00;
    writeData(Player_Y_Speed, a); // initialize vertical speed and low byte of force
    writeData(Player_Y_MoveForce, a); // and then leave

ExPlPos:
    goto Return;

//------------------------------------------------------------------------

CheckPlayerVertical:
    a = M(Player_OffscreenBits); // if player object is completely offscreen
    if (a >= 0xf0)
    {
        playerVerticalOutOfRange = true;
        goto ExCPV;
    }
    y = M(Player_Y_HighPos); // if player high vertical byte is not
    --y; // within the screen, leave this routine
    if (y != 0)
    {
        playerVerticalOutOfRange = false;
        goto ExCPV;
    }
    a = M(Player_Y_Position); // if on the screen, check to see how far down
    playerVerticalOutOfRange = a >= 0xd0; // the player is vertically

ExCPV:
    goto Return;

//------------------------------------------------------------------------

GetEnemyBoundBoxOfs:
    a = M(ObjectOffset); // get enemy object buffer offset

GetEnemyBoundBoxOfsArg:
    a <<= 1; // multiply A by four, then add four
    a <<= 1; // to skip player's bounding box
    a += 0x04;
    y = a; // send to Y
    a = M(Enemy_OffscreenBits); // get offscreen bits for enemy object
    a &= 0b00001111; // save low nybble
    goto Return;

//------------------------------------------------------------------------

PlayerBGCollision:
    a = M(DisableCollisionDet); // if collision detection disabled flag set,
    if (a != 0)
        goto ExPBGCol; // branch to leave
    a = M(GameEngineSubroutine);
    if (a == 0x0b)
        goto ExPBGCol; // branch to leave
    if (a < 0x04)
        goto ExPBGCol; // if running routines $00-$03 branch to leave
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
        goto ExPBGCol; // branch to leave if not
    a = 0xff;
    writeData(Player_CollisionBits, a); // initialize player's collision flag
    a = M(Player_Y_Position);
    if (a >= 0xcf)
    { // if not too close to the bottom of screen, continue

ExPBGCol: // otherwise leave
        goto Return;

    //------------------------------------------------------------------------
    } // ChkCollSize
    y = 0x02; // load default offset
    a = M(CrouchingFlag);
    if (a != 0)
        goto GBBAdr; // if player crouching, skip ahead
    a = M(PlayerSize);
    if (a != 0)
        goto GBBAdr; // if player small, skip ahead
    --y; // otherwise decrement offset for big player not crouching
    a = M(SwimmingFlag);
    if (a != 0)
        goto GBBAdr; // if swimming flag set, skip ahead
    --y; // otherwise decrement offset

GBBAdr: // get value using offset
    a = M(BlockBufferAdderData + y);
    writeData(0xeb, a); // store value here
    y = a; // put value into Y, as offset for block buffer routine
    x = M(PlayerSize); // get player's size as offset
    a = M(CrouchingFlag);
    if (a != 0)
    { // if player not crouching, branch ahead
        ++x; // otherwise increment size as offset
    } // HeadChk: get player's vertical coordinate
    a = M(Player_Y_Position);
    if (a < M(PlayerBGUpperExtent + x))
        goto DoFootCheck; // if player is too high, skip this part
    JSR(BlockBufferColli_Head, 445); // do player-to-bg collision detection on top of
    if (a == 0)
        goto DoFootCheck; // player, and branch if nothing above player's head
    JSR(CheckForCoinMTiles, 446); // check to see if player touched coin with their head
    if (coinMTileFound)
        goto AwardTouchedCoin; // if so, branch to some other part of code
    y = M(Player_Y_Speed); // check player's vertical speed
    if ((y & 0x80) == 0)
        goto DoFootCheck; // if player not moving upwards, branch elsewhere
    y = M(0x04); // check lower nybble of vertical coordinate returned
    if (y < 0x04)
        goto DoFootCheck; // if low nybble < 4, branch
    JSR(CheckForSolidMTiles, 447); // check to see what player's head bumped on
    if (!solidMTileFound)
    { // if player collided with solid metatile, branch
        y = M(AreaType); // otherwise check area type
        if (y == 0)
            goto NYSpd; // if water level, branch ahead
        y = M(BlockBounceTimer); // if block bounce timer not expired,
        if (y != 0)
            goto NYSpd; // branch ahead, do not process collision
        JSR(PlayerHeadCollision, 448); // otherwise do a sub to process collision
        goto DoFootCheck; // jump ahead to skip these other parts here
    } // SolidOrClimb
    if (a == 0x26)
        goto NYSpd; // branch ahead and do not play sound
    a = Sfx_Bump;
    writeData(Square1SoundQueue, a); // otherwise load bump sound

NYSpd: // set player's vertical speed to nullify
    a = 0x01;
    writeData(Player_Y_Speed, a); // jump or swim

DoFootCheck:
    y = M(0xeb); // get block buffer adder offset
    a = M(Player_Y_Position);
    if (a >= 0xcf)
        goto DoPlayerSideCheck; // if player is too far down on screen, skip all of this
    JSR(BlockBufferColli_Feet, 449); // do player-to-bg collision detection on bottom left of player
    JSR(CheckForCoinMTiles, 450); // check to see if player touched coin with their left foot
    if (coinMTileFound)
        goto AwardTouchedCoin; // if so, branch to some other part of code
    pha(); // save bottom left metatile to stack
    JSR(BlockBufferColli_Feet, 451); // do player-to-bg collision detection on bottom right of player
    writeData(0x00, a); // save bottom right metatile here
    pla();
    writeData(0x01, a); // pull bottom left metatile and save here
    if (a != 0)
        goto ChkFootMTile; // if anything here, skip this part
    a = M(0x00); // otherwise check for anything in bottom right metatile
    if (a == 0)
        goto DoPlayerSideCheck; // and skip ahead if not
    JSR(CheckForCoinMTiles, 452); // check to see if player touched coin with their right foot
    if (!coinMTileFound)
        goto ChkFootMTile; // if not, skip unconditional jump and continue code

AwardTouchedCoin:
    goto HandleCoinMetatile; // follow the code to erase coin and award to player 1 coin

ChkFootMTile:
    JSR(CheckForClimbMTiles, 453); // check to see if player landed on climbable metatiles
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
        JSR(ChkInvisibleMTiles, 454);
        if (a == 0x5f || a == 0x60)
            goto DoPlayerSideCheck; // if either found, branch
        y = M(JumpspringAnimCtrl); // if jumpspring animating right now,
        if (y == 0)
        { // branch ahead
            y = M(0x04); // check lower nybble of vertical coordinate returned
            if (y >= 0x05)
            { // if lower nybble < 5, branch
                a = M(Player_MovingDir);
                writeData(0x00, a); // use player's moving direction as temp variable
                goto ImpedePlayerMove; // jump to impede player's movement in that direction
            } // LandPlyr: do sub to check for jumpspring metatiles and deal with it
            JSR(ChkForLandJumpSpring, 455);
            a = 0xf0;
            a &= M(Player_Y_Position); // mask out lower nybble of player's vertical position
            writeData(Player_Y_Position, a); // and store as new vertical position to land player properly
            JSR(HandlePipeEntry, 456); // do sub to process potential pipe entry
            a = 0x00;
            writeData(Player_Y_Speed, a); // initialize vertical speed and fractional
            writeData(Player_Y_MoveForce, a); // movement force to stop player's vertical movement
            writeData(StompChainCounter, a); // initialize enemy stomp counter
        } // InitSteP
        a = 0x00;
        writeData(Player_State, a); // set player's state to normal

DoPlayerSideCheck:
        y = M(0xeb); // get block buffer adder offset
        ++y;
        ++y; // increment offset 2 bytes to use adders for side collisions
        a = 0x02; // set value here to be used as counter
        writeData(0x00, a);

        do // SideCheckLoop
        {
            ++y; // move onto the next one
            writeData(0xeb, y); // store it
            a = M(Player_Y_Position);
            if (a < 0x20)
                goto BHalf; // if player is in status bar area, branch ahead to skip this part
            if (a >= 0xe4)
                goto ExSCH; // branch to leave if player is too far down
            JSR(BlockBufferColli_Side, 457); // do player-to-bg collision detection on one half of player
            if (a == 0)
                goto BHalf; // branch ahead if nothing found
            if (a == 0x1c)
                goto BHalf; // if collided with sideways pipe (top), branch ahead
            if (a == 0x6b)
                goto BHalf; // if collided with water pipe (top), branch ahead
            JSR(CheckForClimbMTiles, 458); // do sub to see if player bumped into anything climbable
            if (!climbMTileFound)
                goto CheckSideMTiles; // if not, branch to alternate section of code

BHalf: // load block adder offset
            y = M(0xeb);
            ++y; // increment it
            a = M(Player_Y_Position); // get player's vertical position
            if (a < 0x08)
                goto ExSCH; // if too high, branch to leave
            if (a >= 0xd0)
                goto ExSCH; // if too low, branch to leave
            JSR(BlockBufferColli_Side, 459); // do player-to-bg collision detection on other half of player
            if (a != 0)
                goto CheckSideMTiles; // if something found, branch
            --M(0x00); // otherwise decrement counter
        } while (M(0x00) != 0); // run code until both sides of player are checked

ExSCH: // leave
        goto Return;

    //------------------------------------------------------------------------

CheckSideMTiles:
        JSR(ChkInvisibleMTiles, 460); // check for hidden or coin 1-up blocks
        if (a == 0x5f || a == 0x60)
            goto ExCSM; // branch to leave if either found
        JSR(CheckForClimbMTiles, 461); // check for climbable metatiles
        if (climbMTileFound)
        { // if not found, skip and continue with code
            goto HandleClimbing; // otherwise jump to handle climbing
        } // ContSChk: check to see if player touched coin
        JSR(CheckForCoinMTiles, 462);
        if (coinMTileFound)
            goto HandleCoinMetatile; // if so, execute code to erase coin and award to player 1 coin
        JSR(ChkJumpspringMetatiles, 463); // check for jumpspring metatiles
        if (jumpspringFound)
        { // if not found, branch ahead to continue cude
            a = M(JumpspringAnimCtrl); // otherwise check jumpspring animation control
            if (a != 0)
                goto ExCSM; // branch to leave if set
            goto StopPlayerMove; // otherwise jump to impede player's movement
        } // ChkPBtm: get player's state
        y = M(Player_State);
        if (y != 0x00)
            goto StopPlayerMove; // if not, branch to impede player's movement
        y = M(PlayerFacingDir); // get player's facing direction
        --y;
        if (y != 0)
            goto StopPlayerMove; // if facing left, branch to impede movement
        if (a != 0x6c)
        { // if collided with sideways pipe (bottom), branch
            if (a != 0x1f)
                goto StopPlayerMove; // otherwise branch to impede player's movement
        } // PipeDwnS: check player's attributes
        a = M(Player_SprAttrib);
        if (a == 0)
        { // if already set, branch, do not play sound again
            y = Sfx_PipeDown_Injury;
            writeData(Square1SoundQueue, y); // otherwise load pipedown/injury sound
        } // PlyrPipe
        a |= 0b00100000;
        writeData(Player_SprAttrib, a); // set background priority bit in player attributes
        a = M(Player_X_Position);
        a &= 0b00001111; // get lower nybble of player's horizontal coordinate
        if (a != 0)
        { // if at zero, branch ahead to skip this part
            y = 0x00; // set default offset for timer setting data
            a = M(ScreenLeft_PageLoc); // load page location for left side of screen
            if (a != 0)
            { // if at page zero, use default offset
                ++y; // otherwise increment offset
            } // SetCATmr: set timer for change of area as appropriate
            a = M(AreaChangeTimerData + y);
            writeData(ChangeAreaTimer, a);
        } // ChkGERtn: get number of game engine routine running
        a = M(GameEngineSubroutine);
        if (a == 0x07)
            goto ExCSM; // if running player entrance routine or
        if (a != 0x08)
            goto ExCSM;
        a = 0x02;
        writeData(GameEngineSubroutine, a); // otherwise set sideways pipe entry routine to run
        goto Return; // and leave

    //------------------------------------------------------------------------

StopPlayerMove:
        JSR(ImpedePlayerMove, 464); // stop player's movement

ExCSM: // leave
        goto Return;

    //------------------------------------------------------------------------

HandleCoinMetatile:
        JSR(ErACM, 465); // do sub to erase coin metatile from block buffer
        ++M(CoinTallyFor1Ups); // increment coin tally used for 1-up blocks
        goto GiveOneCoin; // update coin amount and tally on the screen
    } // HandleAxeMetatile
    a = 0x00;
    writeData(OperMode_Task, a); // reset secondary mode
    a = 0x02;
    writeData(OperMode, a); // set primary mode to autoctrl mode
    a = 0x18;
    writeData(Player_X_Speed, a); // set horizontal speed and continue to erase axe metatile

ErACM: // load vertical high nybble offset for block buffer
    y = M(0x02);
    a = 0x00; // load blank metatile
    writeData(W(0x06) + y, a); // store to remove old contents from block buffer
    goto RemoveCoin_Axe; // update the screen accordingly

HandleClimbing:
    y = M(0x04); // check low nybble of horizontal coordinate returned from
    if (y >= 0x06)
    { // makes actual physical part of vine or flagpole thinner
        if (y < 0x0a)
            goto ChkForFlagpole;
    } // ExHC: leave if too far left or too far right
    goto Return;

//------------------------------------------------------------------------

ChkForFlagpole:
    if (a != 0x24)
    { // branch if flagpole ball found
        if (a != 0x25)
            goto VineCollision; // branch to alternate code if flagpole shaft not found
    } // FlagpoleCollision
    a = M(GameEngineSubroutine);
    if (a == 0x05)
        goto PutPlayerOnVine; // if running, branch to end of climbing code
    a = 0x01;
    writeData(PlayerFacingDir, a); // set player's facing direction to right
    ++M(ScrollLock); // set scroll lock flag
    a = M(GameEngineSubroutine);
    if (a != 0x04)
    { // if running, branch to end of flagpole code here
        a = BulletBill_CannonVar; // load identifier for bullet bills (cannon variant)
        JSR(KillEnemies, 466); // get rid of them
        a = Silence;
        writeData(EventMusicQueue, a); // silence music
        a >>= 1;
        writeData(FlagpoleSoundQueue, a); // load flagpole sound into flagpole sound queue
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
    writeData(GameEngineSubroutine, a); // set value to run flagpole slide routine
    goto PutPlayerOnVine; // jump to end of climbing code

VineCollision:
    if (a != 0x26)
        goto PutPlayerOnVine;
    a = M(Player_Y_Position); // check player's vertical coordinate
    if (a >= 0x20)
        goto PutPlayerOnVine; // branch if not that far up
    a = 0x01;
    writeData(GameEngineSubroutine, a); // otherwise set to run autoclimb routine next frame

PutPlayerOnVine:
    a = 0x03; // set player state to climbing
    writeData(Player_State, a);
    a = 0x00; // nullify player's horizontal speed
    writeData(Player_X_Speed, a); // and fractional horizontal movement force
    writeData(Player_X_MoveForce, a);
    a = M(Player_X_Position); // get player's horizontal coordinate
    a -= M(ScreenLeft_X_Pos); // subtract from left side horizontal coordinate
    if (a < 0x10)
    { // if 16 or more pixels difference, do not alter facing direction
        a = 0x02;
        writeData(PlayerFacingDir, a); // otherwise force player to face left
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
    goto Return;

//------------------------------------------------------------------------

ChkInvisibleMTiles:
    if (a != 0x5f)
    { // branch to leave if found
    } // ExCInvT: leave with zero flag set if either found
    goto Return;

//------------------------------------------------------------------------

ChkForLandJumpSpring:
    JSR(ChkJumpspringMetatiles, 467); // do sub to check if player landed on jumpspring
    if (jumpspringFound)
    { // jumpspring not found, therefore leave
        a = 0x70;
        writeData(VerticalForce, a); // otherwise set vertical movement force for player
        a = 0xf9;
        writeData(JumpspringForce, a); // set default jumpspring force
        a = 0x03;
        writeData(JumpspringTimer, a); // set jumpspring timer to be used later
        a >>= 1;
        writeData(JumpspringAnimCtrl, a); // set jumpspring animation control to start animating
    } // ExCJSp: and leave
    goto Return;

//------------------------------------------------------------------------

ChkJumpspringMetatiles:
    if (a != 0x67)
    { // branch to note the jumpspring if found
        jumpspringFound = false;
        if (a != 0x68)
            goto NoJSFnd; // branch if not found
    } // JSFnd
    jumpspringFound = true;

NoJSFnd: // leave
    goto Return;

//------------------------------------------------------------------------

HandlePipeEntry:
    a = M(Up_Down_Buttons); // check saved controller bits from earlier
    a &= 0b00000100; // for pressing down
    if (a == 0)
        goto ExPipeE; // if not pressing down, branch to leave
    a = M(0x00);
    if (a != 0x11)
        goto ExPipeE; // branch to leave if not found
    a = M(0x01);
    if (a != 0x10)
        goto ExPipeE; // branch to leave if not found
    a = 0x30;
    writeData(ChangeAreaTimer, a); // set timer for change of area
    a = 0x03;
    writeData(GameEngineSubroutine, a); // set to run vertical pipe entry routine on next frame
    a = Sfx_PipeDown_Injury;
    writeData(Square1SoundQueue, a); // load pipedown/injury sound
    a = 0b00100000;
    writeData(Player_SprAttrib, a); // set background priority bit in player's attributes
    a = M(WarpZoneControl); // check warp zone control
    if (a == 0)
        goto ExPipeE; // branch to leave if none found
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
    a = M(AreaAddrOffsets + x); // get area offset based on world offset
    writeData(AreaPointer, a); // store area offset here to be used to change areas
    a = Silence;
    writeData(EventMusicQueue, a); // silence music
    a = 0x00;
    writeData(EntrancePage, a); // initialize starting page number
    writeData(AreaNumber, a); // initialize area number used for area address offset
    writeData(LevelNumber, a); // initialize level number used for world display
    writeData(AltEntranceControl, a); // initialize mode of entry
    ++M(Hidden1UpFlag); // set flag for hidden 1-up blocks
    ++M(FetchNewGameTimerFlag); // set flag to load new game timer

ExPipeE: // leave!!!
    goto Return;

//------------------------------------------------------------------------

ImpedePlayerMove:
    a = 0x00; // initialize value here
    y = M(Player_X_Speed); // get player's horizontal speed
    x = M(0x00); // check value set earlier for
    --x; // left side collision
    if (x == 0)
    { // if right side collision, skip this part
        ++x; // return value to X
        if ((y & 0x80) != 0)
            goto ExIPM; // branch to invert bit and leave
        a = 0xff; // otherwise load A with value to be used later
    } // RImpd: return $02 to X
    else // and jump to affect movement
    {
        x = 0x02;
        if (((y - 0x01) & 0x80) == 0)
            goto ExIPM; // branch to invert bit and leave
        a = 0x01; // otherwise load A with value to be used here
    } // NXSpd
    y = 0x10;
    writeData(SideCollisionTimer, y); // set timer of some sort
    y = 0x00;
    writeData(Player_X_Speed, y); // nullify player's horizontal speed
    if ((a & 0x80) != 0)
    { // branch ahead, do not decrement Y
        --y; // otherwise decrement Y now
    } // PlatF: store Y as high bits of horizontal adder
    writeData(0x00, y);
    // $00:a is the signed 16-bit amount to move the player left or right by
    wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + ((M(0x00) << 8) | a);
    writeData(Player_X_Position, LOBYTE(wide));
    writeData(Player_PageLoc, HIBYTE(wide)); // page location if necessary
    a = HIBYTE(wide);

ExIPM: // invert contents of X
    a = x;
    a ^= 0xff;
    a &= M(Player_CollisionBits); // mask out bit that was set here
    writeData(Player_CollisionBits, a); // store to clear bit
    goto Return;

//------------------------------------------------------------------------

CheckForSolidMTiles:
    JSR(GetMTileAttrib, 468); // find appropriate offset based on metatile's 2 MSB
    solidMTileFound = a >= M(SolidMTileUpperExt + x); // compare current metatile with solid metatiles
    goto Return;

//------------------------------------------------------------------------

CheckForClimbMTiles:
    JSR(GetMTileAttrib, 469); // find appropriate offset based on metatile's 2 MSB
    climbMTileFound = a >= M(ClimbMTileUpperExt + x); // compare current metatile with climbable metatiles
    goto Return;

//------------------------------------------------------------------------

CheckForCoinMTiles:
    if (a == 0xc2)
        goto CoinSd; // branch if found
    if (a == 0xc3)
        goto CoinSd; // branch if found
    coinMTileFound = false; // otherwise leave
    goto Return;

//------------------------------------------------------------------------

CoinSd:
    coinMTileFound = true;
    a = Sfx_CoinGrab;
    writeData(Square2SoundQueue, a); // load coin grab sound and leave
    goto Return;

//------------------------------------------------------------------------

GetMTileAttrib:
    y = a; // save metatile value into Y
    a &= 0b11000000; // mask out all but 2 MSB
    a >>= 6; // shift d7-d6 down to d1-d0
    x = a; // use as offset for metatile data
    a = y; // get original metatile value back

ExEBG: // leave
    goto Return;

//------------------------------------------------------------------------

EnemyToBGCollisionDet:
    a = M(Enemy_State + x); // check enemy state for d6 set
    a &= 0b00100000;
    if (a != 0)
        goto ExEBG; // if set, branch to leave
    JSR(SubtEnemyYPos, 470); // otherwise, do a subroutine here
    if (!enemyYPosInRange)
        goto ExEBG; // if enemy vertical coord + 62 < 68, branch to leave
    y = M(Enemy_ID + x);
    if (y == Spiny)
    {
        a = M(Enemy_Y_Position + x);
        if (a < 0x25)
            goto ExEBG;
    } // DoIDCheckBGColl
    if (y == GreenParatroopaJump)
    { // branch if not found
        goto EnemyJump; // otherwise jump elsewhere
    } // HBChk: check for hammer bro
    if (y == HammerBro)
    { // branch if not found
    } // CInvu: if enemy object is spiny, branch
    else // otherwise jump elsewhere
    {
        if (y == Spiny)
            goto YesIn;
        if (y == PowerUpObject)
            goto YesIn;
        if (y < 0x07)
        {

YesIn: // if enemy object < $07, or = $12 or $2e, do this sub
            JSR(ChkUnderEnemy, 471);
            if (a == 0)
            { // if block underneath enemy, branch

NoEToBGCollision:
                goto ChkForRedKoopa; // otherwise skip and do something else
            } // HandleEToBGCollision
            JSR(ChkForNonSolids, 472); // if something is underneath enemy, find out what
            if (a == 0x26 || a == 0xc2 || a == 0xc3
                || a == 0x5f || a == 0x60)
                goto NoEToBGCollision; // if blank $26, coins, or hidden blocks, jump, enemy falls through
            if (a != 0x23)
                goto LandEnemyProperly; // check for blank metatile $23 and branch if not found
            y = M(0x02); // get vertical coordinate used to find block
            a = 0x00; // store default blank metatile in that spot so we won't
            writeData(W(0x06) + y, a); // trigger this routine accidentally again
            a = M(Enemy_ID + x);
            if (a >= 0x15)
                goto ChkToStunEnemies;
            if (a == Goomba)
            {
                JSR(KillEnemyAboveBlock, 473); // if enemy object IS goomba, do this sub
            } // GiveOEPoints
            a = 0x01; // award 100 points for hitting block beneath enemy
            JSR(SetupFloateyNumber, 474);

ChkToStunEnemies:
            if (a < 0x09)
                goto SetStun;
            if (a >= 0x11)
                goto SetStun; // $09, $0e, $0f or $10, it will be modified, and not
            if (a >= 0x0a)
            { // always fail this test because A will still have vertical
                if (a < PiranhaPlant)
                    goto SetStun; // are only necessary if branching from $d7a1
            } // Demote: erase all but LSB, essentially turning enemy object
            a &= 0b00000001;
            writeData(Enemy_ID + x, a); // into green or red koopa troopa to demote them

SetStun: // load enemy state
            a = M(Enemy_State + x);
            a &= 0b11110000; // save high nybble
            a |= 0b00000010;
            writeData(Enemy_State + x, a); // set d1 of enemy state
            --M(Enemy_Y_Position + x);
            --M(Enemy_Y_Position + x); // subtract two pixels from enemy's vertical position
            a = M(Enemy_ID + x);
            if (a != Bloober)
            {
                a = 0xfd; // set default vertical speed
                y = M(AreaType);
                if (y != 0)
                    goto SetNotW; // if area type not water, set as speed, otherwise
            } // SetWYSpd: change the vertical speed
            a = 0xff;

SetNotW: // set vertical speed now
            writeData(Enemy_Y_Speed + x, a);
            y = 0x01;
            JSR(PlayerEnemyDiff, 475); // get horizontal difference between player and enemy object
            if ((a & 0x80) != 0)
            { // branch if enemy is to the right of player
                ++y; // increment Y if not
            } // ChkBBill
            a = M(Enemy_ID + x);
            if (a == BulletBill_CannonVar)
                goto NoCDirF;
            if (a == BulletBill_FrenzyVar)
                goto NoCDirF; // branch if either found, direction does not change
            writeData(Enemy_MovingDir + x, y); // store as moving direction

NoCDirF: // decrement and use as offset
            --y;
            a = M(EnemyBGCXSpdData + y); // get proper horizontal speed
            writeData(Enemy_X_Speed + x, a); // and store, then leave
        } // ExEBGChk
        goto Return;

    //------------------------------------------------------------------------

LandEnemyProperly:
        a = M(0x04); // check lower nybble of vertical coordinate saved earlier
        a -= 0x08; // subtract eight pixels
        if (a >= 0x05)
            goto ChkForRedKoopa; // branch if lower nybble in range of $0d-$0f before subtract
        a = M(Enemy_State + x);
        a &= 0b01000000; // branch if d6 in enemy state is set
        if (a != 0)
            goto LandEnemyInitState;
        a = M(Enemy_State + x);
        a <<= 1; // branch if d7 in enemy state is not set
        if ((M(Enemy_State + x) & 0x80) != 0)
        {

SChkA: // if lower nybble < $0d, d7 set but d6 not set, jump here
            goto DoEnemySideCheck;
        } // ChkLandedEnemyState
        a = M(Enemy_State + x); // if enemy in normal state, branch back to jump here
        if (a == 0)
            goto SChkA;
        if (a == 0x05)
            goto ProcEnemyDirection; // then branch elsewhere
        if (a < 0x03)
        { // or in higher numbered state, branch to leave
            a = M(Enemy_State + x); // load enemy state again (why?)
            if (a != 0x02)
                goto ProcEnemyDirection; // then branch elsewhere
            a = 0x10; // load default timer here
            y = M(Enemy_ID + x); // check enemy identifier for spiny
            if (y == Spiny)
            { // branch if not found
                a = 0x00; // set timer for $00 if spiny
            } // SetForStn: set timer here
            writeData(EnemyIntervalTimer + x, a);
            a = 0x03; // set state here, apparently used to render
            writeData(Enemy_State + x, a); // upside-down koopas and buzzy beetles
            JSR(EnemyLanding, 476); // then land it properly
        } // ExSteChk: then leave
        goto Return;

    //------------------------------------------------------------------------

ProcEnemyDirection:
        a = M(Enemy_ID + x); // check enemy identifier for goomba
        if (a == Goomba)
            goto LandEnemyInitState;
        if (a == Spiny)
        { // branch if not found
            a = 0x01;
            writeData(Enemy_MovingDir + x, a); // send enemy moving to the right by default
            a = 0x08;
            writeData(Enemy_X_Speed + x, a); // set horizontal speed accordingly
            a = M(FrameCounter);
            a &= 0b00000111; // if timed appropriately, spiny will skip over
            if (a == 0)
                goto LandEnemyInitState; // trying to face the player
        } // InvtD: load 1 for enemy to face the left (inverted here)
        y = 0x01;
        JSR(PlayerEnemyDiff, 477); // get horizontal difference between player and enemy
        if ((a & 0x80) != 0)
        { // if enemy to the right of player, branch
            ++y; // if to the left, increment by one for enemy to face right (inverted)
        } // CNwCDir
        a = y;
        if (a != M(Enemy_MovingDir + x))
            goto LandEnemyInitState;
        JSR(ChkForBump_HammerBroJ, 478); // if equal, not facing in correct dir, do sub to turn around

LandEnemyInitState:
        JSR(EnemyLanding, 479); // land enemy properly
        a = M(Enemy_State + x);
        a &= 0b10000000; // if d7 of enemy state is set, branch
        if (a == 0)
        {
            a = 0x00; // otherwise initialize enemy state and leave
            writeData(Enemy_State + x, a); // note this will also turn spiny's egg into spiny
            goto Return;

        //------------------------------------------------------------------------
        } // NMovShellFallBit
        a = M(Enemy_State + x); // nullify d6 of enemy state, save other bits
        a &= 0b10111111; // and store, then leave
        writeData(Enemy_State + x, a);
        goto Return;

    //------------------------------------------------------------------------

ChkForRedKoopa:
        a = M(Enemy_ID + x); // check for red koopa troopa $03
        if (a == RedKoopa)
        { // branch if not found
            a = M(Enemy_State + x);
            if (a == 0)
                goto ChkForBump_HammerBroJ; // if enemy found and in normal state, branch
        } // Chk2MSBSt: save enemy state into Y
        a = M(Enemy_State + x);
        y = a;
        shiftedBit = (a & 0x80) != 0;
        if (shiftedBit) // check for d7 set
        { // branch if not set
            a = M(Enemy_State + x);
            a |= 0b01000000; // set d6
        } // GetSteFromD: load new enemy state with old as offset
        else // jump ahead of this part
        {
            a = M(EnemyBGCStateData + y);
        } // SetD6Ste: set as new state
        writeData(Enemy_State + x, a);

DoEnemySideCheck:
        a = M(Enemy_Y_Position + x); // if enemy within status bar, branch to leave
        if (a >= 0x20)
        {
            y = 0x16; // start by finding block to the left of enemy ($00,$14)
            a = 0x02; // set value here in what is also used as
            writeData(0xeb, a); // OAM data offset

            do // SdeCLoop: check value
            {
                a = M(0xeb);
                if (a != M(Enemy_MovingDir + x))
                    goto NextSdeC; // branch if different and do not seek block there
                a = 0x01; // set flag in A for save horizontal coordinate
                JSR(BlockBufferChk_Enemy, 480); // find block to left or right of enemy object
                if (a == 0)
                    goto NextSdeC; // if nothing found, branch
                JSR(ChkForNonSolids, 481); // check for non-solid blocks
                if (a != 0x26 && a != 0xc2 && a != 0xc3
                    && a != 0x5f && a != 0x60)
                    goto ChkForBump_HammerBroJ; // branch if not found

NextSdeC: // move to the next direction
                --M(0xeb);
                ++y;
            } while (y < 0x18); // enemy ($00, $14) and ($10, $14) pixel coordinates
        } // ExESdeC
        goto Return;

    //------------------------------------------------------------------------

ChkForBump_HammerBroJ:
        if (x == 0x05)
            goto NoBump; // and if so, branch ahead and do not play sound
        a = M(Enemy_State + x); // if enemy state d7 not set, branch
        a <<= 1; // ahead and do not play sound
        if ((M(Enemy_State + x) & 0x80) == 0)
            goto NoBump;
        a = Sfx_Bump; // otherwise, play bump sound
        writeData(Square1SoundQueue, a); // sound will never be played if branching from ChkForRedKoopa

NoBump: // check for hammer bro
        a = M(Enemy_ID + x);
        if (a == 0x05)
        { // branch if not found
            a = 0x00;
            writeData(0x00, a); // initialize value here for bitmask
            y = 0xfa; // load default vertical speed for jumping
            goto SetHJ; // jump to code that makes hammer bro jump
        } // InvEnemyDir
        goto RXSpd; // jump to turn the enemy around

PlayerEnemyDiff:
        // get the distance between the enemy object and the player, each one 16-bit page:coordinate
        wide = ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x))
             - ((M(Player_PageLoc) << 8) | M(Player_X_Position));
        writeData(0x00, LOBYTE(wide)); // and store here
        enemyRightOfPlayer = (wide & 0x10000) == 0; // the subtraction did not borrow
        a = HIBYTE(wide); // then leave
        goto Return;

    //------------------------------------------------------------------------

EnemyLanding:
        JSR(InitVStf, 482); // do something here to vertical speed and something else
        a = M(Enemy_Y_Position + x);
        a &= 0b11110000; // save high nybble of vertical coordinate, and
        a |= 0b00001000; // set d3, then store, probably used to set enemy object
        writeData(Enemy_Y_Position + x, a); // neatly on whatever it's landing on
        goto Return;

    //------------------------------------------------------------------------

SubtEnemyYPos:
        a = M(Enemy_Y_Position + x); // add 62 pixels to enemy object's
        a += 0x3e;
        enemyYPosInRange = a >= 0x44; // compare against a certain range
        goto Return; // and leave with the result for a conditional branch

    //------------------------------------------------------------------------

EnemyJump:
        JSR(SubtEnemyYPos, 483); // do a sub here
        if (!enemyYPosInRange)
            goto DoSide; // if enemy vertical coord + 62 < 68, branch to leave
        a = M(Enemy_Y_Speed + x);
        a += 0x02;
        if (a < 0x03)
            goto DoSide;
        JSR(ChkUnderEnemy, 484); // otherwise, check to see if green paratroopa is
        if (a == 0)
            goto DoSide; // standing on anything, then branch to same place if not
        JSR(ChkForNonSolids, 485); // check for non-solid blocks
        if (a == 0x26 || a == 0xc2 || a == 0xc3
            || a == 0x5f || a == 0x60)
            goto DoSide; // branch if found
        JSR(EnemyLanding, 486); // change vertical coordinate and speed
        a = 0xfd;
        writeData(Enemy_Y_Speed + x, a); // make the paratroopa jump again

DoSide: // check for horizontal blockage, then leave
        goto DoEnemySideCheck;
    } // HammerBroBGColl
    JSR(ChkUnderEnemy, 487); // check to see if hammer bro is standing on anything
    if (a == 0)
        goto NoUnderHammerBro;
    if (a == 0x23)
    {

KillEnemyAboveBlock:
        JSR(ShellOrBlockDefeat, 488); // do this sub to kill enemy
        a = 0xfc; // alter vertical speed of enemy and leave
        writeData(Enemy_Y_Speed + x, a);
        goto Return;

    //------------------------------------------------------------------------
    } // UnderHammerBro
    a = M(EnemyFrameTimer + x); // check timer used by hammer bro
    if (a != 0)
        goto NoUnderHammerBro; // branch if not expired
    a = M(Enemy_State + x);
    a &= 0b10001000; // save d7 and d3 from enemy state, nullify other bits
    writeData(Enemy_State + x, a); // and store
    JSR(EnemyLanding, 489); // modify vertical coordinate, speed and something else
    goto DoEnemySideCheck; // then check for horizontal blockage and leave

NoUnderHammerBro:
    a = M(Enemy_State + x); // if hammer bro is not standing on anything, set d0
    a |= 0x01; // in the enemy state to indicate jumping or falling, then leave
    writeData(Enemy_State + x, a);
    goto Return;

//------------------------------------------------------------------------

ChkUnderEnemy:
    a = 0x00; // set flag in A for save vertical coordinate
    y = 0x15; // set Y to check the bottom middle (8,18) of enemy object
    goto BlockBufferChk_Enemy; // hop to it!

ChkForNonSolids:
    if (a == 0x26)
        goto NSFnd;
    if (a == 0xc2)
        goto NSFnd;
    if (a == 0xc3)
        goto NSFnd;
    if (a == 0x5f)
        goto NSFnd;

NSFnd:
    goto Return;

//------------------------------------------------------------------------

FireballBGCollision:
    a = M(Fireball_Y_Position + x); // check fireball's vertical coordinate
    if (a < 0x18)
        goto ClearBounceFlag; // if within the status bar area of the screen, branch ahead
    JSR(BlockBufferChk_FBall, 490); // do fireball to background collision detection on bottom of it
    if (a == 0)
        goto ClearBounceFlag; // if nothing underneath fireball, branch
    JSR(ChkForNonSolids, 491); // check for non-solid metatiles
    if (a == 0x26 || a == 0xc2 || a == 0xc3
        || a == 0x5f || a == 0x60)
        goto ClearBounceFlag; // branch if any found
    a = M(Fireball_Y_Speed + x); // if fireball's vertical speed set to move upwards,
    if ((a & 0x80) != 0)
        goto InitFireballExplode; // branch to set exploding bit in fireball's state
    a = M(FireballBouncingFlag + x); // if bouncing flag already set,
    if (a != 0)
        goto InitFireballExplode; // branch to set exploding bit in fireball's state
    a = 0xfd;
    writeData(Fireball_Y_Speed + x, a); // otherwise set vertical speed to move upwards (give it bounce)
    a = 0x01;
    writeData(FireballBouncingFlag + x, a); // set bouncing flag
    a = M(Fireball_Y_Position + x);
    a &= 0xf8; // modify vertical coordinate to land it properly
    writeData(Fireball_Y_Position + x, a); // store as new vertical coordinate
    goto Return; // leave

//------------------------------------------------------------------------

ClearBounceFlag:
    a = 0x00;
    writeData(FireballBouncingFlag + x, a); // clear bouncing flag by default
    goto Return; // leave

//------------------------------------------------------------------------

InitFireballExplode:
    a = 0x80;
    writeData(Fireball_State + x, a); // set exploding flag in fireball's state
    a = Sfx_Bump;
    writeData(Square1SoundQueue, a); // load bump sound
    goto Return; // leave

//------------------------------------------------------------------------

GetFireballBoundBox:
    a = x; // add seven bytes to offset
    a += 0x07;
    x = a;
    y = 0x02; // set offset for relative coordinates
    if (y == 0)
    { // unconditional branch

GetMiscBoundBox:
        a = x; // add nine bytes to offset
        a += 0x09;
        x = a;
        y = 0x06; // set offset for relative coordinates
    } // FBallB: get bounding box coordinates
    JSR(BoundingBoxCore, 492);
    goto CheckRightScreenBBox; // jump to handle any offscreen coordinates

GetEnemyBoundBox:
    y = 0x48; // store bitmask here for now
    writeData(0x00, y);
    y = 0x44; // store another bitmask here for now and jump
    goto GetMaskedOffScrBits;

SmallPlatformBoundBox:
    y = 0x08; // store bitmask here for now
    writeData(0x00, y);
    y = 0x04; // store another bitmask here for now

GetMaskedOffScrBits:
    // the enemy object and the left side of the screen are each one 16-bit page:coordinate
    wide = ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x))
         - ((M(ScreenLeft_PageLoc) << 8) | M(ScreenLeft_X_Pos));
    writeData(0x01, LOBYTE(wide)); // store here
    a = HIBYTE(wide);
    if ((a & 0x80) != 0)
        goto CMBits; // if enemy object is beyond left edge, branch
    a |= M(0x01);
    if (a == 0)
        goto CMBits; // if precisely at the left edge, branch
    y = M(0x00); // if to the right of left edge, use value in $00 for A

CMBits: // otherwise use contents of Y
    a = y;
    a &= M(Enemy_OffscreenBits); // preserve bitwise whatever's in here
    writeData(EnemyOffscrBitsMasked + x, a); // save masked offscreen bits here
    if (a != 0)
        goto MoveBoundBoxOffscreen; // if anything set here, branch
    goto SetupEOffsetFBBox; // otherwise, do something else

LargePlatformBoundBox:
    ++x; // increment X to get the proper offset
    JSR(GetXOffscreenBits, 493); // then jump directly to the sub for horizontal offscreen bits
    --x; // decrement to return to original offset
    if (a >= 0xfe)
        goto MoveBoundBoxOffscreen; // box offscreen, otherwise start getting coordinates

SetupEOffsetFBBox:
    a = x; // add 1 to offset to properly address
    a += 0x01;
    x = a;
    y = 0x01; // load 1 as offset here, same reason
    JSR(BoundingBoxCore, 494); // do a sub to get the coordinates of the bounding box
    goto CheckRightScreenBBox; // jump to handle offscreen coordinates of bounding box

MoveBoundBoxOffscreen:
    a = x; // multiply offset by 4
    a <<= 1;
    a <<= 1;
    y = a; // use as offset here
    a = 0xff;
    writeData(EnemyBoundingBoxCoord + y, a); // load value into four locations here and leave
    writeData(EnemyBoundingBoxCoord + 1 + y, a);
    writeData(EnemyBoundingBoxCoord + 2 + y, a);
    writeData(EnemyBoundingBoxCoord + 3 + y, a);
    goto Return;

//------------------------------------------------------------------------

BoundingBoxCore:
    writeData(0x00, x); // save offset here
    a = M(SprObject_Rel_YPos + y); // store object coordinates relative to screen
    writeData(0x02, a); // vertically and horizontally, respectively
    a = M(SprObject_Rel_XPos + y);
    writeData(0x01, a);
    a = x; // multiply offset by four and save to stack
    a <<= 1;
    a <<= 1;
    pha();
    y = a; // use as offset for Y, X is left alone
    a = M(SprObj_BoundBoxCtrl + x); // load value here to be used as offset for X
    a <<= 1; // multiply that by four and use as X
    a <<= 1;
    x = a;
    a = M(0x01); // add the first number in the bounding box data to the
    a += M(BoundBoxCtrlData + x); // and store somewhere using same offset * 4
    writeData(BoundingBox_UL_Corner + y, a); // store here
    a = M(0x01);
    a += M(BoundBoxCtrlData + 2 + x); // add the third number in the bounding box data to the
    writeData(BoundingBox_LR_Corner + y, a); // relative horizontal coordinate and store
    ++x; // increment both offsets
    ++y;
    a = M(0x02); // add the second number to the relative vertical coordinate
    a += M(BoundBoxCtrlData + x); // incremented offset
    writeData(BoundingBox_UL_Corner + y, a);
    a = M(0x02);
    a += M(BoundBoxCtrlData + 2 + x); // add the fourth number to the relative vertical coordinate
    writeData(BoundingBox_LR_Corner + y, a); // and store
    pla(); // get original offset loaded into $00 * y from stack
    y = a; // use as Y
    x = M(0x00); // get original offset and use as X again
    goto Return;

//------------------------------------------------------------------------

CheckRightScreenBBox:
    a = M(ScreenLeft_X_Pos); // add 128 pixels to left side of screen
    wide = ((M(ScreenLeft_PageLoc) << 8) | a) + 0x80;
    writeData(0x02, LOBYTE(wide));
    writeData(0x01, HIBYTE(wide));
    a = HIBYTE(wide); // add carry to page location of left side of screen
    if (((M(SprObject_PageLoc + x) << 8) | M(SprObject_X_Position + x)) // compare against the middle of the screen
        >= ((M(0x01) << 8) | M(0x02)))
    { // if object is on the left side of the screen, branch
        a = M(BoundingBox_DR_XPos + y); // check right-side edge of bounding box for offscreen
        if ((a & 0x80) == 0)
        { // coordinates, branch if still on the screen
            a = 0xff; // load offscreen value here to use on one or both horizontal sides
            x = M(BoundingBox_UL_XPos + y); // check left-side edge of bounding box for offscreen
            if ((x & 0x80) == 0)
            { // coordinates, and branch if still on the screen
                writeData(BoundingBox_UL_XPos + y, a); // store offscreen value for left side
            } // SORte: store offscreen value for right side
            writeData(BoundingBox_DR_XPos + y, a);
        } // NoOfs: get object offset and leave
        x = M(ObjectOffset);
        goto Return;

    //------------------------------------------------------------------------
    } // CheckLeftScreenBBox
    a = M(BoundingBox_UL_XPos + y); // check left-side edge of bounding box for offscreen
    if ((a & 0x80) == 0)
        goto NoOfs2; // coordinates, and branch if still on the screen
    if (a < 0xa0)
        goto NoOfs2; // screen or really offscreen, and branch if still on
    a = 0x00;
    x = M(BoundingBox_DR_XPos + y); // check right-side edge of bounding box for offscreen
    if ((x & 0x80) != 0)
    { // coordinates, branch if still onscreen
        writeData(BoundingBox_DR_XPos + y, a); // store offscreen value for right side
    } // SOLft: store offscreen value for left side
    writeData(BoundingBox_UL_XPos + y, a);

NoOfs2: // get object offset and leave
    x = M(ObjectOffset);
    goto Return;

//------------------------------------------------------------------------

PlayerCollisionCore:
    x = 0x00; // initialize X to use player's bounding box for comparison

SprObjectCollisionCore:
    writeData(0x06, y); // save contents of Y here
    a = 0x01;
    writeData(0x07, a); // save value 1 here as counter, compare horizontal coordinates first

    do // CollisionCoreLoop
    {
        a = M(BoundingBox_UL_Corner + y); // compare left/top coordinates
        if (a < M(BoundingBox_UL_Corner + x))
        { // if first left/top => second, branch
            if (a >= M(BoundingBox_LR_Corner + x))
            { // if first left/top < second right/bottom, branch elsewhere
                if (a == M(BoundingBox_LR_Corner + x))
                    goto CollisionFound; // if somehow equal, collision, thus branch
                a = M(BoundingBox_LR_Corner + y); // if somehow greater, check to see if bottom of
                if (a < M(BoundingBox_UL_Corner + y))
                    goto CollisionFound; // if somehow less, vertical wrap collision, thus branch
                if (a >= M(BoundingBox_UL_Corner + x))
                    goto CollisionFound; // of second box, and if equal or greater, collision, thus branch
                collisionFound = false;
                y = M(0x06); // otherwise return with Y = $0006
                goto Return; // note horizontal wrapping never occurs

            //------------------------------------------------------------------------
            } // SecondBoxVerticalChk
            a = M(BoundingBox_LR_Corner + x); // check to see if the vertical bottom of the box
            if (a < M(BoundingBox_UL_Corner + x))
                goto CollisionFound; // if somehow less, vertical wrap collision, thus branch
            a = M(BoundingBox_LR_Corner + y); // otherwise compare horizontal right or vertical bottom
            if (a >= M(BoundingBox_UL_Corner + x))
                goto CollisionFound; // if equal or greater, collision, thus branch
            collisionFound = false;
            y = M(0x06); // otherwise return with Y = $0006
            goto Return;

        //------------------------------------------------------------------------
        } // FirstBoxGreater
        if (a == M(BoundingBox_UL_Corner + x))
            goto CollisionFound; // if first coordinate = second, collision, thus branch
        if (a < M(BoundingBox_LR_Corner + x))
            goto CollisionFound; // if left/top of first less than or equal to right/bottom of second
        if (a == M(BoundingBox_LR_Corner + x))
            goto CollisionFound; // then collision, thus branch
        if (a < M(BoundingBox_LR_Corner + y))
            goto NoCollisionFound; // if less than or equal, no collision, branch to end
        if (a == M(BoundingBox_LR_Corner + y))
            goto NoCollisionFound;
        a = M(BoundingBox_LR_Corner + y); // otherwise compare bottom of first to top of second
        if (a >= M(BoundingBox_UL_Corner + x))
            goto CollisionFound; // collision, and branch, otherwise, proceed onwards here

NoCollisionFound:
        collisionFound = false; // then load value set earlier, then leave
        y = M(0x06); // like previous ones, if horizontal coordinates do not collide, we do
        goto Return; // not bother checking vertical ones, because what's the point?

    //------------------------------------------------------------------------

CollisionFound:
        ++x; // increment offsets on both objects to check
        ++y; // the vertical coordinates
        --M(0x07); // decrement counter to reflect this
    } while ((M(0x07) & 0x80) == 0); // if counter not expired, branch to loop
    collisionFound = true; // otherwise we already did both sets, therefore collision
    y = M(0x06); // load original value set here earlier, then leave
    goto Return;

//------------------------------------------------------------------------

BlockBufferChk_Enemy:
    pha(); // save contents of A to stack
    a = x;
    a += 0x01;
    x = a;
    pla(); // pull A from stack and jump elsewhere
    goto BBChk_E;

ResidualMiscObjectCode:
    a = x;
    a += 0x0d; // miscellaneous objects
    x = a;
    y = 0x1b; // supposedly used once to set offset for block buffer data
    goto ResJmpM; // probably used in early stages to do misc to bg collision detection

BlockBufferChk_FBall:
    y = 0x1a; // set offset for block buffer adder data
    a = x;
    a += 0x07; // add seven bytes to use
    x = a;

ResJmpM: // set A to return vertical coordinate
    a = 0x00;

BBChk_E: // do collision detection subroutine for sprite object
    JSR(BlockBufferCollision, 495);
    x = M(ObjectOffset); // get object offset
    goto Return;

//------------------------------------------------------------------------

BlockBufferColli_Feet:
    ++y; // if branched here, increment to next set of adders

BlockBufferColli_Head:
    a = 0x00; // set flag to return vertical coordinate
    goto Skip_9;

BlockBufferColli_Side:
    a = 0x01; // set flag to return horizontal coordinate
Skip_9:
    x = 0x00; // set offset for player object

BlockBufferCollision:
    pha(); // save contents of A to stack
    writeData(0x04, y); // save contents of Y here
    wide = ((M(SprObject_PageLoc + x) << 8) | M(SprObject_X_Position + x))
         + M(BlockBuffer_X_Adder + y); // add horizontal coordinate of object to value obtained using Y as offset
    writeData(0x05, LOBYTE(wide)); // store here
    a = HIBYTE(wide); // the page location, plus the carry
    a = (uint8_t)(((a & 0x01) << 7) | (M(0x05) >> 1)); // put the LSB of the page location above the stored value
    a >>= 1; // and effectively move high nybble to
    a >>= 1; // lower, LSB which became MSB will be
    a >>= 1; // d4 at this point
    JSR(GetBlockBufferAddr, 496); // get address of block buffer into $06, $07
    y = M(0x04); // get old contents of Y
    a = M(SprObject_Y_Position + x); // get vertical coordinate of object
    a += M(BlockBuffer_Y_Adder + y); // add it to value obtained using Y as offset
    a &= 0b11110000; // mask out low nybble
    a -= 0x20; // subtract 32 pixels for the status bar
    writeData(0x02, a); // store result here
    y = a; // use as offset for block buffer
    a = M(W(0x06) + y); // check current content of block buffer
    writeData(0x03, a); // and store here
    y = M(0x04); // get old contents of Y again
    pla(); // pull A from stack
    if (a == 0)
    { // if A = 1, branch
        a = M(SprObject_Y_Position + x); // if A = 0, load vertical coordinate
    } // RetXC: otherwise load horizontal coordinate
    else // and jump
    {
        a = M(SprObject_X_Position + x);
    } // RetYC: and mask out high nybble
    a &= 0b00001111;
    writeData(0x04, a); // store masked out result here
    a = M(0x03); // get saved content of block buffer
    goto Return; // and leave

//------------------------------------------------------------------------

DrawVine:
    writeData(0x00, y); // save offset here
    a = M(Enemy_Rel_YPos); // get relative vertical coordinate
    a += M(VineYPosAdder + y); // add value using offset in Y to get value
    x = M(VineObjOffset + y); // get offset to vine
    y = M(Enemy_SprDataOffset + x); // get sprite data offset
    writeData(0x02, y); // store sprite data offset here
    JSR(SixSpriteStacker, 497); // stack six sprites on top of each other vertically
    a = M(Enemy_Rel_XPos); // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, a); // store in first, third and fifth sprites
    writeData(Sprite_X_Position + 8 + y, a);
    writeData(Sprite_X_Position + 16 + y, a);
    a += 0x06; // add six pixels to second, fourth and sixth sprites
    writeData(Sprite_X_Position + 4 + y, a); // to give characteristic staggered vine shape to
    writeData(Sprite_X_Position + 12 + y, a); // our vertical stack of sprites
    writeData(Sprite_X_Position + 20 + y, a);
    a = 0b00100001; // set bg priority and palette attribute bits
    writeData(Sprite_Attributes + y, a); // set in first, third and fifth sprites
    writeData(Sprite_Attributes + 8 + y, a);
    writeData(Sprite_Attributes + 16 + y, a);
    a |= 0b01000000; // additionally, set horizontal flip bit
    writeData(Sprite_Attributes + 4 + y, a); // for second, fourth and sixth sprites
    writeData(Sprite_Attributes + 12 + y, a);
    writeData(Sprite_Attributes + 20 + y, a);
    x = 0x05; // set tiles for six sprites

    do // VineTL: set tile number for sprite
    {
        a = 0xe1;
        writeData(Sprite_Tilenumber + y, a);
        ++y; // move offset to next sprite data
        ++y;
        ++y;
        ++y;
        --x; // move onto next sprite
    } while ((x & 0x80) == 0); // loop until all sprites are done
    y = M(0x02); // get original offset
    a = M(0x00); // get offset to vine adding data
    if (a == 0)
    { // if offset not zero, skip this part
        a = 0xe0;
        writeData(Sprite_Tilenumber + y, a); // set other tile number for top of vine
    } // SkpVTop: start with the first sprite again
    x = 0x00;

    do // ChkFTop: get original starting vertical coordinate
    {
        a = M(VineStart_Y_Position);
        a -= M(Sprite_Y_Position + y); // subtract top-most sprite's Y coordinate
        if (a >= 0x64)
        { // apart, skip this to leave sprite alone
            a = 0xf8;
            writeData(Sprite_Y_Position + y, a); // otherwise move sprite offscreen
        } // NextVSp: move offset to next OAM data
        ++y;
        ++y;
        ++y;
        ++y;
        ++x; // move onto next sprite
    } while (x != 0x06);
    y = M(0x00); // return offset set earlier
    goto Return;

//------------------------------------------------------------------------

SixSpriteStacker:
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
    goto Return;

//------------------------------------------------------------------------

DrawHammer:
    y = M(Misc_SprDataOffset + x); // get misc object OAM data offset
    a = M(TimerControl);
    if (a == 0)
    { // if master timer control set, skip this part
        a = M(Misc_State + x); // otherwise get hammer's state
        a &= 0b01111111; // mask out d7
        if (a == 0x01)
            goto GetHPose; // if so, branch
    } // ForceHPose: reset offset here
    x = 0x00;
    if (x != 0)
    { // do unconditional branch to rendering part

GetHPose: // get frame counter
        a = M(FrameCounter);
        a >>= 1; // move d3-d2 to d1-d0
        a >>= 1;
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
    a = M(FirstSprTilenum + x);
    writeData(Sprite_Tilenumber + y, a); // get and store tile number of first sprite
    a = M(SecondSprTilenum + x);
    writeData(Sprite_Tilenumber + 4 + y, a); // get and store tile number of second sprite
    a = M(HammerSprAttrib + x);
    writeData(Sprite_Attributes + y, a); // get and store attribute bytes for both
    writeData(Sprite_Attributes + 4 + y, a); // note in this case they use the same data
    x = M(ObjectOffset); // get misc object offset
    a = M(Misc_OffscreenBits);
    a &= 0b11111100; // check offscreen bits
    if (a != 0)
    { // if all bits clear, leave object alone
        a = 0x00;
        writeData(Misc_State + x, a); // otherwise nullify misc object state
        a = 0xf8;
        JSR(DumpTwoSpr, 498); // do sub to move hammer sprites offscreen
    } // NoHOffscr: leave
    goto Return;

//------------------------------------------------------------------------

FlagpoleGfxHandler:
    y = M(Enemy_SprDataOffset + x); // get sprite data offset for flagpole flag
    a = M(Enemy_Rel_XPos); // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, a); // store as X coordinate for first sprite
    a += 0x08; // add eight pixels and store
    writeData(Sprite_X_Position + 4 + y, a); // as X coordinate for second and third sprites
    writeData(Sprite_X_Position + 8 + y, a);
    wide = a + 0x0c; // add twelve more pixels and
    writeData(0x05, LOBYTE(wide)); // store here to be used later by floatey number
    a = M(Enemy_Y_Position + x); // get vertical coordinate
    JSR(DumpTwoSpr, 499); // and do sub to dump into first and second sprites
    a = (uint8_t)(a + 0x08 + HIBYTE(wide)); // add eight pixels, plus the carry out of the horizontal add above
    writeData(Sprite_Y_Position + 8 + y, a); // and store into third sprite
    a = M(FlagpoleFNum_Y_Pos); // get vertical coordinate for floatey number
    writeData(0x02, a); // store it here
    a = 0x01;
    writeData(0x03, a); // set value for flip which will not be used, and
    writeData(0x04, a); // attribute byte for floatey number
    writeData(Sprite_Attributes + y, a); // set attribute bytes for all three sprites
    writeData(Sprite_Attributes + 4 + y, a);
    writeData(Sprite_Attributes + 8 + y, a);
    a = 0x7e;
    writeData(Sprite_Tilenumber + y, a); // put triangle shaped tile
    writeData(Sprite_Tilenumber + 8 + y, a); // into first and third sprites
    a = 0x7f;
    writeData(Sprite_Tilenumber + 4 + y, a); // put skull tile into second sprite
    a = M(FlagpoleCollisionYPos); // get vertical coordinate at time of collision
    if (a != 0)
    { // if zero, branch ahead
        a = y;
        a += 0x0c;
        y = a; // put back in Y
        a = M(FlagpoleScore); // get offset used to award points for touching flagpole
        a <<= 1; // multiply by 2 to get proper offset here
        x = a;
        a = M(FlagpoleScoreNumTiles + x); // get appropriate tile data
        writeData(0x00, a);
        a = M(FlagpoleScoreNumTiles + 1 + x);
        JSR(DrawOneSpriteRow, 500); // use it to render floatey number
    } // ChkFlagOffscreen
    x = M(ObjectOffset); // get object offset for flag
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    a = M(Enemy_OffscreenBits); // get offscreen bits
    a &= 0b00001110; // mask out all but d3-d1
    if (a != 0)
    { // if none of these bits set, branch to leave

MoveSixSpritesOffscreen:
        a = 0xf8; // set offscreen coordinate if jumping here

DumpSixSpr:
        writeData(Sprite_Data + 20 + y, a); // dump A contents
        writeData(Sprite_Data + 16 + y, a); // into third row sprites

DumpFourSpr:
        writeData(Sprite_Data + 12 + y, a); // into second row sprites

DumpThreeSpr:
        writeData(Sprite_Data + 8 + y, a);

DumpTwoSpr:
        writeData(Sprite_Data + 4 + y, a); // and into first row sprites
        writeData(Sprite_Data + y, a);
    } // ExitDumpSpr
    goto Return;

//------------------------------------------------------------------------

DrawLargePlatform:
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    writeData(0x02, y); // store here
    ++y; // add 3 to it for offset
    ++y; // to X coordinate
    ++y;
    a = M(Enemy_Rel_XPos); // get horizontal relative coordinate
    JSR(SixSpriteStacker, 501); // store X coordinates using A as base, stack horizontally
    x = M(ObjectOffset);
    a = M(Enemy_Y_Position + x); // get vertical coordinate
    JSR(DumpFourSpr, 502); // dump into first four sprites as Y coordinate
    y = M(AreaType);
    if (y != 0x03)
    {
        y = M(SecondaryHardMode); // check for secondary hard mode flag set
        if (y == 0)
            goto SetLast2Platform; // branch if not set elsewhere
    } // ShrinkPlatform
    a = 0xf8; // load offscreen coordinate if flag set or castle-type level

SetLast2Platform:
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    writeData(Sprite_Y_Position + 16 + y, a); // store vertical coordinate or offscreen
    writeData(Sprite_Y_Position + 20 + y, a); // coordinate into last two sprites as Y coordinate
    a = 0x5b; // load default tile for platform (girder)
    x = M(CloudTypeOverride);
    if (x != 0)
    { // if cloud level override flag not set, use
        a = 0x75; // otherwise load other tile for platform (puff)
    } // SetPlatformTilenum
    x = M(ObjectOffset); // get enemy object buffer offset
    ++y; // increment Y for tile offset
    JSR(DumpSixSpr, 503); // dump tile number into all six sprites
    a = 0x02; // set palette controls
    ++y; // increment Y for sprite attributes
    JSR(DumpSixSpr, 504); // dump attributes into all six sprites
    ++x; // increment X for enemy objects
    JSR(GetXOffscreenBits, 505); // get offscreen bits again
    --x;
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d7, save the remaining
    pha(); // bits to the stack
    if (shiftedBit)
    {
        a = 0xf8; // if d7 was set, move first sprite offscreen
        writeData(Sprite_Y_Position + y, a);
    } // SChk2: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d6
    pha(); // save to stack
    if (shiftedBit)
    {
        a = 0xf8; // if d6 was set, move second sprite offscreen
        writeData(Sprite_Y_Position + 4 + y, a);
    } // SChk3: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d5
    pha(); // save to stack
    if (shiftedBit)
    {
        a = 0xf8; // if d5 was set, move third sprite offscreen
        writeData(Sprite_Y_Position + 8 + y, a);
    } // SChk4: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d4
    pha(); // save to stack
    if (shiftedBit)
    {
        a = 0xf8; // if d4 was set, move fourth sprite offscreen
        writeData(Sprite_Y_Position + 12 + y, a);
    } // SChk5: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d3
    pha(); // save to stack
    if (shiftedBit)
    {
        a = 0xf8; // if d3 was set, move fifth sprite offscreen
        writeData(Sprite_Y_Position + 16 + y, a);
    } // SChk6: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    if (shiftedBit) // and d2
    { // save to stack
        a = 0xf8;
        writeData(Sprite_Y_Position + 20 + y, a); // if d2 was set, move sixth sprite offscreen
    } // SLChk: check d7 of offscreen bits
    a = M(Enemy_OffscreenBits);
    a <<= 1; // and if d7 is not set, skip sub
    if ((M(Enemy_OffscreenBits) & 0x80) != 0)
    {
        JSR(MoveSixSpritesOffscreen, 506); // otherwise branch to move all sprites offscreen
    } // ExDLPl
    goto Return;

//------------------------------------------------------------------------

    do // DrawFloateyNumber_Coin
    {
        if ((M(FrameCounter) & 0x01) == 0) // get frame counter divide by 2
        { // branch if d0 not set to raise number every other frame
            --M(Misc_Y_Position + x); // otherwise, decrement vertical coordinate
        } // NotRsNum: get vertical coordinate
        a = M(Misc_Y_Position + x);
        JSR(DumpTwoSpr, 507); // dump into both sprites
        a = M(Misc_Rel_XPos); // get relative horizontal coordinate
        writeData(Sprite_X_Position + y, a); // store as X coordinate for first sprite
        a += 0x08; // add eight pixels
        writeData(Sprite_X_Position + 4 + y, a); // store as X coordinate for second sprite
        a = 0x02;
        writeData(Sprite_Attributes + y, a); // store attribute byte in both sprites
        writeData(Sprite_Attributes + 4 + y, a);
        a = 0xf7;
        writeData(Sprite_Tilenumber + y, a); // put tile numbers into both sprites
        a = 0xfb; // that resemble "200"
        writeData(Sprite_Tilenumber + 4 + y, a);
        goto ExJCGfx; // then jump to leave (why not an rts here instead?)

JCoinGfxHandler:
        y = M(Misc_SprDataOffset + x); // get coin/floatey number's OAM data offset
        a = M(Misc_State + x); // get state of misc object
    } while (a >= 0x02); // branch to draw floatey number
    a = M(Misc_Y_Position + x); // store vertical coordinate as
    writeData(Sprite_Y_Position + y, a); // Y coordinate for first sprite
    a += 0x08; // add eight pixels
    writeData(Sprite_Y_Position + 4 + y, a); // store as Y coordinate for second sprite
    a = M(Misc_Rel_XPos); // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, a);
    writeData(Sprite_X_Position + 4 + y, a); // store as X coordinate for first and second sprites
    a = M(FrameCounter); // get frame counter
    a >>= 1; // divide by 2 to alter every other frame
    a &= 0b00000011; // mask out d2-d1
    x = a; // use as graphical offset
    a = M(JumpingCoinTiles + x); // load tile number
    ++y; // increment OAM data offset to write tile numbers
    JSR(DumpTwoSpr, 508); // do sub to dump tile number into both sprites
    --y; // decrement to get old offset
    a = 0x02;
    writeData(Sprite_Attributes + y, a); // set attribute byte in first sprite
    a = 0x82;
    writeData(Sprite_Attributes + 4 + y, a); // set attribute byte with vertical flip in second sprite
    x = M(ObjectOffset); // get misc object offset

ExJCGfx: // leave
    goto Return;

//------------------------------------------------------------------------

DrawPowerUp:
    y = M(Enemy_SprDataOffset + 5); // get power-up's sprite data offset
    a = M(Enemy_Rel_YPos); // get relative vertical coordinate
    a += 0x08; // add eight pixels
    writeData(0x02, a); // store result here
    a = M(Enemy_Rel_XPos); // get relative horizontal coordinate
    writeData(0x05, a); // store here
    x = M(PowerUpType); // get power-up type
    a = M(PowerUpAttributes + x); // get attribute data for power-up type
    a |= M(Enemy_SprAttrib + 5); // add background priority bit if set
    writeData(0x04, a); // store attributes here
    a = x;
    pha(); // save power-up type to the stack
    a <<= 1;
    a <<= 1; // multiply by four to get proper offset
    x = a; // use as X
    a = 0x01;
    writeData(0x07, a); // set counter here to draw two rows of sprite object
    writeData(0x03, a); // init d1 of flip control

    do // PUpDrawLoop
    {
        a = M(PowerUpGfxTable + x); // load left tile of power-up object
        writeData(0x00, a);
        a = M(PowerUpGfxTable + 1 + x); // load right tile
        JSR(DrawOneSpriteRow, 509); // branch to draw one row of our power-up object
        --M(0x07); // decrement counter
    } while ((M(0x07) & 0x80) == 0); // branch until two rows are drawn
    y = M(Enemy_SprDataOffset + 5); // get sprite data offset again
    pla(); // pull saved power-up type from the stack
    if (a == 0)
        goto PUpOfs; // if regular mushroom, branch, do not change colors or flip
    if (a == 0x03)
        goto PUpOfs; // if 1-up mushroom, branch, do not change colors or flip
    writeData(0x00, a); // store power-up type here now
    a = M(FrameCounter); // get frame counter
    a >>= 1; // divide by 2 to change colors every two frames
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
    a = M(Sprite_Attributes + 4 + y);
    a |= 0b01000000; // set horizontal flip bit for top right sprite
    writeData(Sprite_Attributes + 4 + y, a);
    a = M(Sprite_Attributes + 12 + y);
    a |= 0b01000000; // set horizontal flip bit for bottom right sprite
    writeData(Sprite_Attributes + 12 + y, a); // note these are only done for fire flower and star power-ups

PUpOfs: // jump to check to see if power-up is offscreen at all, then leave
    goto SprObjectOffscrChk;

EnemyGfxHandler:
    a = M(Enemy_Y_Position + x); // get enemy object vertical position
    writeData(0x02, a);
    a = M(Enemy_Rel_XPos); // get enemy object horizontal position
    writeData(0x05, a); // relative to screen
    y = M(Enemy_SprDataOffset + x);
    writeData(0xeb, y); // get sprite data offset
    a = 0x00;
    writeData(VerticalFlipFlag, a); // initialize vertical flip flag by default
    a = M(Enemy_MovingDir + x);
    writeData(0x03, a); // get enemy object moving direction
    a = M(Enemy_SprAttrib + x);
    writeData(0x04, a); // get enemy object sprite attributes
    a = M(Enemy_ID + x);
    if (a != PiranhaPlant)
        goto CheckForRetainerObj; // if not, branch
    y = M(PiranhaPlant_Y_Speed + x);
    if ((y & 0x80) != 0)
        goto CheckForRetainerObj; // if piranha plant moving upwards, branch
    y = M(EnemyFrameTimer + x);
    if (y == 0)
        goto CheckForRetainerObj; // if timer for movement expired, branch
    goto Return; // if all conditions fail, leave

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
        a = 0x01; // set value that will not be used
        writeData(0x03, a);
        a = 0x15; // set value $15 as code for mushroom retainer/princess object
    } // CheckForBulletBillCV
    if (a == BulletBill_CannonVar)
    { // if not found, branch again
        --M(0x02); // decrement saved vertical position
        a = 0x03;
        y = M(EnemyFrameTimer + x); // get timer for enemy object
        if (y != 0)
        { // if expired, do not set priority bit
            a |= 0b00100000; // otherwise do so
        } // SBBAt: set new sprite attributes
        writeData(0x04, a);
        y = 0x00; // nullify saved enemy state both in Y and in
        writeData(0xed, y); // memory location here
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
    a = M(Enemy_Y_Speed + x); // if moving upwards, branch
    if ((a & 0x80) != 0)
        goto CheckBowserGfxFlag;
    ++M(VerticalFlipFlag); // otherwise, set flag for vertical flip

CheckBowserGfxFlag:
    a = M(BowserGfxFlag); // if not drawing bowser at all, skip to something else
    if (a != 0)
    {
        y = 0x16; // if set to 1, draw bowser's front
        if (a != 0x01)
        {
            ++y; // otherwise draw bowser's rear
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
        writeData(0xec, x);
    } // GmbaAnim: check for d5 set in enemy object state
    a &= 0b00100000;
    a |= M(TimerControl); // or timer disable flag set
    if (a != 0)
        goto CheckBowserFront; // if either condition true, do not animate goomba
    a = M(FrameCounter);
    a &= 0b00001000; // check for every eighth frame
    if (a != 0)
        goto CheckBowserFront;
    a = M(0x03);
    a ^= 0b00000011; // invert bits to flip horizontally every eight frames
    writeData(0x03, a); // leave alone otherwise

CheckBowserFront:
    a = M(EnemyAttributeData + y); // load sprite attribute using enemy object
    a |= M(0x04); // as offset, and add to bits already loaded
    writeData(0x04, a);
    a = M(EnemyGfxTableOffsets + y); // load value based on enemy object as offset
    x = a; // save as X
    y = M(0xec); // get previously saved value
    a = M(BowserGfxFlag);
    if (a != 0)
    { // if not drawing bowser object at all, skip all of this
        if (a == 0x01)
        { // if not drawing front part, branch to draw the rear part
            a = M(BowserBodyControls); // check bowser's body control bits
            if ((a & 0x80) != 0)
            { // branch if d7 not set (control's bowser's mouth)
                x = 0xde; // otherwise load offset for second frame
            } // ChkFrontSte: check saved enemy state
            a = M(0xed);
            a &= 0b00100000; // if bowser not defeated, do not set flag
            if (a == 0)
                goto DrawBowser;

FlipBowserOver:
            writeData(VerticalFlipFlag, x); // set vertical flip flag to nonzero

DrawBowser:
            goto DrawEnemyObject; // draw bowser's graphics now
        } // CheckBowserRear
        a = M(BowserBodyControls); // check bowser's body control bits
        a &= 0x01;
        if (a != 0)
        { // branch if d0 not set (control's bowser's feet)
            x = 0xe4; // otherwise load offset for second frame
        } // ChkRearSte: check saved enemy state
        a = M(0xed);
        a &= 0b00100000; // if bowser not defeated, do not set flag
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
            a = 0x02;
            writeData(0x03, a); // set enemy direction to reverse sprites horizontally
            a = 0x05;
            writeData(0xec, a); // set enemy state
        } // NotEgg: skip a big chunk of this if we found spiny but not in egg
        goto CheckForHammerBro;
    } // CheckForLakitu
    if (x == 0x90)
    { // branch if not loaded
        a = M(0xed);
        a &= 0b00100000; // check for d5 set in enemy state
        if (a != 0)
            goto NoLAFr; // branch if set
        a = M(FrenzyEnemyTimer);
        if (a >= 0x10)
            goto NoLAFr; // branch if not
        x = 0x96; // if d6 not set and timer in range, load alt frame for lakitu

NoLAFr: // skip this next part if we found lakitu but alt frame not needed
        goto CheckDefeatedState;
    } // CheckUpsideDownShell
    a = M(0xef); // check for enemy object => $04
    if (a >= 0x04)
        goto CheckRightSideUpShell; // branch if true
    if (y < 0x02)
        goto CheckRightSideUpShell; // branch if enemy state < $02
    x = 0x5a; // set for upside-down koopa shell by default
    y = M(0xef);
    if (y != BuzzyBeetle)
        goto CheckRightSideUpShell;
    x = 0x7e; // set for upside-down buzzy beetle shell if found
    ++M(0x02); // increment vertical position by one pixel

CheckRightSideUpShell:
    a = M(0xec); // check for value set here
    if (a != 0x04)
        goto CheckForHammerBro; // enemy state => $02 but not = $04, leave shell upside-down
    x = 0x72; // set right-side up buzzy beetle shell by default
    ++M(0x02); // increment saved vertical position by one pixel
    y = M(0xef);
    if (y != BuzzyBeetle)
    { // branch if found
        x = 0x66; // change to right-side up koopa shell if not found
        ++M(0x02); // and increment saved vertical position again
    } // CheckForDefdGoomba
    if (y != Goomba)
        goto CheckForHammerBro; // failed buzzy beetle object test)
    x = 0x54; // load for regular goomba
    a = M(0xed); // note that this only gets performed if enemy state => $02
    a &= 0b00100000; // check saved enemy state for d5 set
    if (a != 0)
        goto CheckForHammerBro; // branch if set
    x = 0x8a; // load offset for defeated goomba
    --M(0x02); // set different value and decrement saved vertical position

CheckForHammerBro:
    y = M(ObjectOffset);
    a = M(0xef); // check for hammer bro object
    if (a == HammerBro)
    { // branch if not found
        a = M(0xed);
        if (a == 0)
            goto CheckToAnimateEnemy; // branch if not in normal enemy state
        a &= 0b00001000;
        if (a == 0)
            goto CheckDefeatedState; // if d3 not set, branch further away
        x = 0xb4; // otherwise load offset for different frame
        if (x != 0)
            goto CheckToAnimateEnemy; // unconditional branch
    } // CheckForBloober
    if (x == 0x48)
        goto CheckToAnimateEnemy; // branch if found
    a = M(EnemyIntervalTimer + y);
    if (a >= 0x05)
        goto CheckDefeatedState; // branch if some timer is above a certain point
    if (x != 0x3c)
        goto CheckToAnimateEnemy; // branch if not found this time
    if (a == 0x01)
        goto CheckDefeatedState; // branch if timer is set to certain point
    ++M(0x02); // increment saved vertical coordinate three pixels
    ++M(0x02);
    ++M(0x02);
    goto CheckAnimationStop; // and do something else

CheckToAnimateEnemy:
    a = M(0xef); // check for specific enemy objects
    if (a == Goomba)
        goto CheckDefeatedState; // branch if goomba
    if (a == 0x08)
        goto CheckDefeatedState; // branch if bullet bill (note both variants use $08 here)
    if (a == Podoboo)
        goto CheckDefeatedState; // branch if podoboo
    if (a >= 0x18)
        goto CheckDefeatedState;
    y = 0x00;
    if (a == 0x15)
    { // which uses different code here, branch if not found
        ++y; // residual instruction
        a = M(WorldNumber); // are we on world 8?
        if (a >= World8)
            goto CheckDefeatedState; // if so, leave the offset alone (use princess)
        x = 0xa2; // otherwise, set for mushroom retainer object instead
        a = 0x03; // set alternate state here
        writeData(0xec, a);
        if (a != 0)
            goto CheckDefeatedState; // unconditional branch
    } // CheckForSecondFrame
    a = M(FrameCounter); // load frame counter
    a &= M(EnemyAnimTimingBMask + y); // mask it (partly residual, one byte not ever used)
    if (a != 0)
        goto CheckDefeatedState; // branch if timing is off

CheckAnimationStop:
    a = M(0xed); // check saved enemy state
    a &= 0b10100000; // for d7 or d5, or check for timers stopped
    a |= M(TimerControl);
    if (a != 0)
        goto CheckDefeatedState; // if either condition true, branch
    a = x;
    a += 0x06; // add $06 to current enemy offset
    x = a; // to animate various enemy objects

CheckDefeatedState:
    a = M(0xed); // check saved enemy state
    a &= 0b00100000; // for d5 set
    if (a == 0)
        goto DrawEnemyObject; // branch if not set
    a = M(0xef);
    if (a < 0x04)
        goto DrawEnemyObject; // branch if less
    y = 0x01;
    writeData(VerticalFlipFlag, y); // set vertical flip flag
    --y;
    writeData(0xec, y); // init saved value here

DrawEnemyObject:
    y = M(0xeb); // load sprite data offset
    JSR(DrawEnemyObjRow, 510); // draw six tiles of data
    JSR(DrawEnemyObjRow, 511); // into sprite data
    JSR(DrawEnemyObjRow, 512);
    x = M(ObjectOffset); // get enemy object offset
    y = M(Enemy_SprDataOffset + x); // get sprite data offset
    a = M(0xef);
    if (a == 0x08)
    { // for bullet bill, branch if not found

SkipToOffScrChk:
        goto SprObjectOffscrChk; // jump if found
    } // CheckForVerticalFlip
    a = M(VerticalFlipFlag); // check if vertical flip flag is set here
    if (a != 0)
    { // branch if not
        a = M(Sprite_Attributes + y); // get attributes of first sprite we dealt with
        a |= 0b10000000; // set bit for vertical flip
        ++y;
        ++y; // increment two bytes so that we store the vertical flip
        JSR(DumpSixSpr, 513); // in attribute bytes of enemy obj sprite data
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
        a = M(Sprite_Tilenumber + 16 + y); // exchange third row tiles
        writeData(Sprite_Tilenumber + x, a); // with first or second row tiles
        a = M(Sprite_Tilenumber + 20 + y);
        writeData(Sprite_Tilenumber + 4 + x, a);
        pla(); // pull first or second row tiles from stack
        writeData(Sprite_Tilenumber + 20 + y, a); // and save in third row
        pla();
        writeData(Sprite_Tilenumber + 16 + y, a);
    } // CheckForESymmetry
    a = M(BowserGfxFlag); // are we drawing bowser at all?
    if (a != 0)
        goto SkipToOffScrChk; // branch if so
    a = M(0xef);
    x = M(0xec); // get alternate enemy state
    if (a == 0x05)
    {
        goto SprObjectOffscrChk; // jump if found
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
        writeData(Sprite_Attributes + 20 + y, a); // note that palette bits were already set earlier
    } // SpnySC: if alternate enemy state set to 1 or 0, branch
    if (x < 0x02)
        goto CheckToMirrorLakitu;

MirrorEnemyGfx:
    a = M(BowserGfxFlag); // if enemy object is bowser, skip all of this
    if (a != 0)
        goto CheckToMirrorLakitu;
    a = M(Sprite_Attributes + y); // load attribute bits of first sprite
    a &= 0b10100011;
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
    a = M(Sprite_Attributes + 8 + y); // get second row left sprite attributes
    a |= 0b10000000;
    writeData(Sprite_Attributes + 8 + y, a); // store bits with vertical flip in
    writeData(Sprite_Attributes + 16 + y, a); // second and third row left sprites
    a |= 0b01000000;
    writeData(Sprite_Attributes + 12 + y, a); // store with horizontal and vertical flip in
    writeData(Sprite_Attributes + 20 + y, a); // second and third row right sprites

CheckToMirrorLakitu:
    a = M(0xef); // check for lakitu enemy object
    if (a == Lakitu)
    { // branch if not found
        a = M(VerticalFlipFlag);
        if (a == 0)
        { // branch if vertical flip flag not set
            a = M(Sprite_Attributes + 16 + y); // save vertical flip and palette bits
            a &= 0b10000001; // in third row left sprite
            writeData(Sprite_Attributes + 16 + y, a);
            a = M(Sprite_Attributes + 20 + y); // set horizontal flip and palette bits
            a |= 0b01000001; // in third row right sprite
            writeData(Sprite_Attributes + 20 + y, a);
            x = M(FrenzyEnemyTimer); // check timer
            if (x >= 0x10)
                goto SprObjectOffscrChk; // branch if timer has not reached a certain range
            writeData(Sprite_Attributes + 12 + y, a); // otherwise set same for second row right sprite
            a &= 0b10000001;
            writeData(Sprite_Attributes + 8 + y, a); // preserve vertical flip and palette bits for left sprite
            if (x < 0x10)
                goto SprObjectOffscrChk; // unconditional branch
        } // NVFLak: get first row left sprite attributes
        a = M(Sprite_Attributes + y);
        a &= 0b10000001;
        writeData(Sprite_Attributes + y, a); // save vertical flip and palette bits
        a = M(Sprite_Attributes + 4 + y); // get first row right sprite attributes
        a |= 0b01000001; // set horizontal flip and palette bits
        writeData(Sprite_Attributes + 4 + y, a); // note that vertical flip is left as-is
    } // CheckToMirrorJSpring
    a = M(0xef); // check for jumpspring object (any frame)
    if (a < 0x18)
        goto SprObjectOffscrChk; // branch if not jumpspring object at all
    a = 0x82;
    writeData(Sprite_Attributes + 8 + y, a); // set vertical flip and palette bits of
    writeData(Sprite_Attributes + 16 + y, a); // second and third row left sprites
    a |= 0b01000000;
    writeData(Sprite_Attributes + 12 + y, a); // set, in addition to those, horizontal flip
    writeData(Sprite_Attributes + 20 + y, a); // for second and third row right sprites

SprObjectOffscrChk:
    x = M(ObjectOffset); // get enemy buffer offset
    a = M(Enemy_OffscreenBits); // check offscreen information
    a >>= 1;
    a >>= 1; // shift three times to the right
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // which takes d2
    pha(); // save to stack
    if (shiftedBit)
    { // branch if not set
        a = 0x04; // set for right column sprites
        JSR(MoveESprColOffscreen, 514); // and move them offscreen
    } // LcChk: get from stack
    pla();
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // take d3
    pha(); // save to stack
    if (shiftedBit)
    { // branch if not set
        a = 0x00; // set for left column sprites,
        JSR(MoveESprColOffscreen, 515); // move them offscreen
    } // Row3C: get from stack again
    pla();
    a >>= 1; // take d5 this time
    shiftedBit = (a & 0x01) != 0;
    a >>= 1;
    pha(); // save to stack again
    if (shiftedBit)
    { // branch if it was not set
        a = 0x10; // set for third row of sprites
        JSR(MoveESprRowOffscreen, 516); // and move them offscreen
    } // Row23C: get from stack
    pla();
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // take d6
    pha(); // save to stack
    if (shiftedBit)
    {
        a = 0x08; // set for second and third rows
        JSR(MoveESprRowOffscreen, 517); // move them offscreen
    } // AllRowC: get from stack once more
    pla();
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // take d7
    if (!shiftedBit)
        goto ExEGHandler;
    JSR(MoveESprRowOffscreen, 518); // move all sprites offscreen (A should be 0 by now)
    a = M(Enemy_ID + x);
    if (a == Podoboo)
        goto ExEGHandler; // skip this part if found, we do not want to erase podoboo!
    a = M(Enemy_Y_HighPos + x); // check high byte of vertical position
    if (a != 0x02)
        goto ExEGHandler;
    JSR(EraseEnemyObject, 519); // what it says

ExEGHandler:
    goto Return;

//------------------------------------------------------------------------

DrawEnemyObjRow:
    a = M(EnemyGraphicsTable + x); // load two tiles of enemy graphics
    writeData(0x00, a);
    a = M(EnemyGraphicsTable + 1 + x);

DrawOneSpriteRow:
    writeData(0x01, a);
    goto DrawSpriteObject; // draw them

MoveESprRowOffscreen:
    a += M(Enemy_SprDataOffset + x);
    y = a; // use as offset
    a = 0xf8;
    goto DumpTwoSpr; // move first row of sprites offscreen

MoveESprColOffscreen:
    a += M(Enemy_SprDataOffset + x);
    y = a; // use as offset
    JSR(MoveColOffscreen, 520); // move first and second row sprites in column offscreen
    writeData(Sprite_Data + 16 + y, a); // move third row sprite in column offscreen
    goto Return;

//------------------------------------------------------------------------

DrawBlock:
    a = M(Block_Rel_YPos); // get relative vertical coordinate of block object
    writeData(0x02, a); // store here
    a = M(Block_Rel_XPos); // get relative horizontal coordinate of block object
    writeData(0x05, a); // store here
    a = 0x03;
    writeData(0x04, a); // set attribute byte here
    a >>= 1;
    writeData(0x03, a); // set horizontal flip bit here (will not be used)
    y = M(Block_SprDataOffset + x); // get sprite data offset
    x = 0x00; // reset X for use as offset to tile data

    do // DBlkLoop: get left tile number
    {
        a = M(DefaultBlockObjTiles + x);
        writeData(0x00, a); // set here
        a = M(DefaultBlockObjTiles + 1 + x); // get right tile number
        JSR(DrawOneSpriteRow, 521); // do sub to write tile numbers to first row of sprites
    } while (x != 0x04); // and loop back until all four sprites are done
    x = M(ObjectOffset); // get block object offset
    y = M(Block_SprDataOffset + x); // get sprite data offset
    a = M(AreaType);
    if (a != 0x01)
    { // if found, branch to next part
        a = 0x86;
        writeData(Sprite_Tilenumber + y, a); // otherwise remove brick tiles with lines
        writeData(Sprite_Tilenumber + 4 + y, a); // and replace then with lineless brick tiles
    } // ChkRep: check replacement metatile
    a = M(Block_Metatile + x);
    if (a == 0xc4)
    { // branch ahead to use current graphics
        a = 0x87; // set A for used block tile
        ++y; // increment Y to write to tile bytes
        JSR(DumpFourSpr, 522); // do sub to dump into all four sprites
        --y; // return Y to original offset
        a = 0x03; // set palette bits
        x = M(AreaType);
        --x; // check for ground level type area again
        if (x != 0)
        { // if found, use current palette bits
            a >>= 1; // otherwise set to $01
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
        writeData(Sprite_Y_Position + 4 + y, a); // on the right side
        writeData(Sprite_Y_Position + 12 + y, a);
    } // PullOfsB: pull offscreen bits from stack
    pla();

ChkLeftCo: // check to see if d3 in offscreen bits are set
    a &= 0b00001000;
    if (a != 0)
    { // if not set, branch, otherwise move sprites offscreen

MoveColOffscreen:
        a = 0xf8; // move offscreen two OAMs
        writeData(Sprite_Y_Position + y, a); // on the left side (or two rows of enemy on either side
        writeData(Sprite_Y_Position + 8 + y, a); // if branched here from enemy graphics handler)
    } // ExDBlk
    goto Return;

//------------------------------------------------------------------------

DrawBrickChunks:
    a = 0x02; // set palette bits here
    writeData(0x00, a);
    a = 0x75; // set tile number for ball (something residual, likely)
    y = M(GameEngineSubroutine);
    if (y != 0x05)
    { // use palette and tile number assigned
        a = 0x03; // otherwise set different palette bits
        writeData(0x00, a);
        a = 0x84; // and set tile number for brick chunks
    } // DChunks: get OAM data offset
    y = M(Block_SprDataOffset + x);
    ++y; // increment to start with tile bytes in OAM
    JSR(DumpFourSpr, 523); // do sub to dump tile number into all four sprites
    a = M(FrameCounter); // get frame counter
    a <<= 1;
    a <<= 1;
    a <<= 1; // move low nybble to high
    a <<= 1;
    a &= 0xc0; // get what was originally d3-d2 of low nybble
    a |= M(0x00); // add palette bits
    ++y; // increment offset for attribute bytes
    JSR(DumpFourSpr, 524); // do sub to dump attribute data into all four sprites
    --y;
    --y; // decrement offset to Y coordinate
    a = M(Block_Rel_YPos); // get first block object's relative vertical coordinate
    JSR(DumpTwoSpr, 525); // do sub to dump current Y coordinate into two sprites
    a = M(Block_Rel_XPos); // get first block object's relative horizontal coordinate
    writeData(Sprite_X_Position + y, a); // save into X coordinate of first sprite
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
    a = M(Block_Rel_XPos + 1); // get second block object's relative horizontal coordinate
    writeData(Sprite_X_Position + 8 + y, a); // save into X coordinate of third sprite
    a = M(0x00); // use original relative horizontal position
    carry = a >= M(Block_Rel_XPos + 1); // the borrow this subtract leaves is read by the add below
    a -= M(Block_Rel_XPos + 1); // get difference of relative positions of original - current
    wide = a + M(0x00) + (carry ? 1 : 0); // add original relative position to result
    a = (uint8_t)(LOBYTE(wide) + 0x06 + HIBYTE(wide)); // plus 6 pixels, and this add's own carry, to position fourth brick chunk correctly
    writeData(Sprite_X_Position + 12 + y, a); // save into X coordinate of fourth sprite
    a = M(Block_OffscreenBits); // get offscreen bits for block object
    JSR(ChkLeftCo, 526); // do sub to move left half of sprites offscreen if necessary
    if ((M(Block_OffscreenBits) & 0x80) != 0) // check d7 of the offscreen bits
    { // if d7 not set, branch to last part
        a = 0xf8;
        JSR(DumpTwoSpr, 527); // otherwise move top sprites offscreen
    } // ChnkOfs: if relative position on left side of screen,
    a = M(0x00);
    if ((a & 0x80) == 0)
        goto ExBCDr; // go ahead and leave
    a = M(Sprite_X_Position + y); // otherwise compare left-side X coordinate
    if (a < M(Sprite_X_Position + 4 + y))
        goto ExBCDr; // branch to leave if less
    a = 0xf8; // otherwise move right half of sprites offscreen
    writeData(Sprite_Y_Position + 4 + y, a);
    writeData(Sprite_Y_Position + 12 + y, a);

ExBCDr: // leave
    goto Return;

//------------------------------------------------------------------------

DrawFireball:
    y = M(FBall_SprDataOffset + x); // get fireball's sprite data offset
    a = M(Fireball_Rel_YPos); // get relative vertical coordinate
    writeData(Sprite_Y_Position + y, a); // store as sprite Y coordinate
    a = M(Fireball_Rel_XPos); // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, a); // store as sprite X coordinate, then do shared code

DrawFirebar:
    a = M(FrameCounter); // get frame counter
    a >>= 1; // divide by four
    a >>= 1;
    pha(); // save result to stack
    a &= 0x01; // mask out all but last bit
    a ^= 0x64; // set either tile $64 or $65 as fireball tile
    writeData(Sprite_Tilenumber + y, a); // thus tile changes every four frames
    pla(); // get from stack
    a >>= 1; // divide by four again
    shiftedBit = (a & 0x01) != 0;
    a = 0x02; // load value $02 to set palette in attrib byte
    if (shiftedBit)
    { // if last bit shifted out was not set, skip this
        a |= 0b11000000; // otherwise flip both ways every eight frames
    } // FireA: store attribute byte and leave
    writeData(Sprite_Attributes + y, a);
    goto Return;

//------------------------------------------------------------------------

DrawExplosion_Fireball:
    y = M(Alt_SprDataOffset + x); // get OAM data offset of alternate sort for fireball's explosion
    a = M(Fireball_State + x); // load fireball state
    ++M(Fireball_State + x); // increment state for next frame
    a >>= 1; // divide by 2
    a &= 0b00000111; // mask out all but d3-d1
    if (a < 0x03)
    { // branch if so, otherwise continue to draw explosion

DrawExplosion_Fireworks:
        x = a; // use whatever's in A for offset
        a = M(ExplosionTiles + x); // get tile number using offset
        ++y; // increment Y (contains sprite data offset)
        JSR(DumpFourSpr, 528); // and dump into tile number part of sprite data
        --y; // decrement Y so we have the proper offset again
        x = M(ObjectOffset); // return enemy object buffer offset to X
        a = M(Fireball_Rel_YPos); // get relative vertical coordinate
        a -= 0x04; // for first and third sprites
        writeData(Sprite_Y_Position + y, a);
        writeData(Sprite_Y_Position + 8 + y, a);
        a += 0x08; // for second and fourth sprites
        writeData(Sprite_Y_Position + 4 + y, a);
        writeData(Sprite_Y_Position + 12 + y, a);
        a = M(Fireball_Rel_XPos); // get relative horizontal coordinate
        a -= 0x04; // for first and second sprites
        writeData(Sprite_X_Position + y, a);
        writeData(Sprite_X_Position + 4 + y, a);
        a += 0x08; // for third and fourth sprites
        writeData(Sprite_X_Position + 8 + y, a);
        writeData(Sprite_X_Position + 12 + y, a);
        a = 0x02; // set palette attributes for all sprites, but
        writeData(Sprite_Attributes + y, a); // set no flip at all for first sprite
        a = 0x82;
        writeData(Sprite_Attributes + 4 + y, a); // set vertical flip for second sprite
        a = 0x42;
        writeData(Sprite_Attributes + 8 + y, a); // set horizontal flip for third sprite
        a = 0xc2;
        writeData(Sprite_Attributes + 12 + y, a); // set both flips for fourth sprite
        goto Return; // we are done

    //------------------------------------------------------------------------
    } // KillFireBall
    a = 0x00; // clear fireball state to kill it
    writeData(Fireball_State + x, a);
    goto Return;

//------------------------------------------------------------------------

DrawSmallPlatform:
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    a = 0x5b; // load tile number for small platforms
    ++y; // increment offset for tile numbers
    JSR(DumpSixSpr, 529); // dump tile number into all six sprites
    ++y; // increment offset for attributes
    a = 0x02; // load palette controls
    JSR(DumpSixSpr, 530); // dump attributes into all six sprites
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
    JSR(DumpThreeSpr, 531);
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
        writeData(Sprite_Y_Position + y, a); // fourth sprites offscreen
        writeData(Sprite_Y_Position + 12 + y, a);
    } // SOfs: move out and back into stack
    pla();
    pha();
    a &= 0b00000100; // check d2
    if (a != 0)
    {
        a = 0xf8; // if d2 was set, move second and
        writeData(Sprite_Y_Position + 4 + y, a); // fifth sprites offscreen
        writeData(Sprite_Y_Position + 16 + y, a);
    } // SOfs2: get from stack
    pla();
    a &= 0b00000010; // check d1
    if (a != 0)
    {
        a = 0xf8; // if d1 was set, move third and
        writeData(Sprite_Y_Position + 8 + y, a); // sixth sprites offscreen
        writeData(Sprite_Y_Position + 20 + y, a);
    } // ExSPl: get enemy object offset and leave
    x = M(ObjectOffset);
    goto Return;

//------------------------------------------------------------------------

DrawBubble:
    y = M(Player_Y_HighPos); // if player's vertical high position
    --y; // not within screen, skip all of this
    if (y != 0)
        goto ExDBub;
    a = M(Bubble_OffscreenBits); // check air bubble's offscreen bits
    a &= 0b00001000;
    if (a != 0)
        goto ExDBub; // if bit set, branch to leave
    y = M(Bubble_SprDataOffset + x); // get air bubble's OAM data offset
    a = M(Bubble_Rel_XPos); // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, a); // store as X coordinate here
    a = M(Bubble_Rel_YPos); // get relative vertical coordinate
    writeData(Sprite_Y_Position + y, a); // store as Y coordinate here
    a = 0x74;
    writeData(Sprite_Tilenumber + y, a); // put air bubble tile into OAM data
    a = 0x02;
    writeData(Sprite_Attributes + y, a); // set attribute byte

ExDBub: // leave
    goto Return;

//------------------------------------------------------------------------

PlayerGfxHandler:
    a = M(InjuryTimer); // if player's injured invincibility timer
    if (a != 0)
    { // not set, skip checkpoint and continue code
        a = M(FrameCounter);
        a >>= 1; // otherwise check frame counter and branch
        if ((M(FrameCounter) & 0x01) != 0)
            goto ExPGH; // to leave on every other frame (when d0 is set)
    } // CntPl: if executing specific game engine routine,
    a = M(GameEngineSubroutine);
    if (a != 0x0b)
    {
        a = M(PlayerChangeSizeFlag); // if grow/shrink flag set
        if (a == 0)
        { // then branch to some other code
            y = M(SwimmingFlag); // if swimming flag set, branch to
            if (y == 0)
                goto FindPlayerAction; // different part, do not return
            a = M(Player_State);
            if (a == 0x00)
                goto FindPlayerAction; // branch and do not return
            JSR(FindPlayerAction, 532); // otherwise jump and return
            a = M(FrameCounter);
            a &= 0b00000100; // check frame counter for d2 set (8 frames every
            if (a != 0)
                goto ExPGH; // eighth frame), and branch if set to leave
            x = a; // initialize X to zero
            y = M(Player_SprDataOffset); // get player sprite data offset
            if ((M(PlayerFacingDir) & 0x01) == 0) // get player's facing direction
            { // if player facing to the right, use current offset
                ++y;
                ++y; // otherwise move to next OAM data
                ++y;
                ++y;
            } // SwimKT: check player's size
            a = M(PlayerSize);
            if (a != 0)
            { // if big, use first tile
                a = M(Sprite_Tilenumber + 24 + y); // check tile number of seventh/eighth sprite
                if (a == M(SwimTileRepOffset))
                    goto ExPGH; // if spr7/spr8 tile number = value, branch to leave
                ++x; // otherwise increment X for second tile
            } // BigKTS: overwrite tile number in sprite 7/8
            a = M(SwimKickTileNum + x);
            writeData(Sprite_Tilenumber + 24 + y, a); // to animate player's feet when swimming

ExPGH: // then leave
            goto Return;

        //------------------------------------------------------------------------

FindPlayerAction:
            JSR(ProcessPlayerAction, 533); // find proper offset to graphics table by player's actions
            goto PlayerGfxProcessing; // draw player, then process for fireball throwing
        } // DoChangeSize
        JSR(HandleChangeSize, 534); // find proper offset to graphics table for grow/shrink
        goto PlayerGfxProcessing; // draw player, then process for fireball throwing
    } // PlayerKilled
    y = 0x0e; // load offset for player killed
    a = M(PlayerGfxTblOffsets + y); // get offset to graphics table

PlayerGfxProcessing:
    writeData(PlayerGfxOffset, a); // store offset to graphics table here
    a = 0x04;
    JSR(RenderPlayerSub, 535); // draw player based on offset loaded
    JSR(ChkForPlayerAttrib, 536); // set horizontal flip bits as necessary
    a = M(FireballThrowingTimer);
    if (a == 0)
        goto PlayerOffscreenChk; // if fireball throw timer not set, skip to the end
    y = 0x00; // set value to initialize by default
    a = M(PlayerAnimTimer); // get animation frame timer
    if (a >= M(FireballThrowingTimer))
    {
        writeData(FireballThrowingTimer, y); // initialize fireball throw timer
        goto PlayerOffscreenChk; // if animation frame timer => fireball throw timer skip to end
    }
    writeData(FireballThrowingTimer, a); // otherwise store animation timer into fireball throw timer
    y = 0x07; // load offset for throwing
    a = M(PlayerGfxTblOffsets + y); // get offset to graphics table
    writeData(PlayerGfxOffset, a); // store it for use later
    y = 0x04; // set to update four sprite rows by default
    a = M(Player_X_Speed);
    a |= M(Left_Right_Buttons); // check for horizontal speed or left/right button press
    if (a != 0)
    { // if no speed or button press, branch using set value in Y
        --y; // otherwise set to update only three sprite rows
    } // SUpdR: save in A for use
    a = y;
    JSR(RenderPlayerSub, 537); // in sub, draw player object again

PlayerOffscreenChk:
    a = M(Player_OffscreenBits); // get player's offscreen bits
    a >>= 1;
    a >>= 1; // move vertical bits to low nybble
    a >>= 1;
    a >>= 1;
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
            JSR(DumpTwoSpr, 538); // otherwise dump offscreen Y coordinate into sprite data
        } // NPROffscr
        a = y;
        a -= 0x08; // next row up
        y = a;
        --x; // decrement row counter
    } while ((x & 0x80) == 0); // do this until all sprite rows are checked
    goto Return; // then we are done!

//------------------------------------------------------------------------

DrawPlayer_Intermediate:
    x = 0x05; // store data into zero page memory

    do // PIntLoop: load data to display player as he always
    {
        a = M(IntermediatePlayerData + x);
        writeData(0x02 + x, a); // appears on world/lives display
        --x;
    } while ((x & 0x80) == 0); // do this until all data is loaded
    x = 0xb8; // load offset for small standing
    y = 0x04; // load sprite data offset
    JSR(DrawPlayerLoop, 539); // draw player accordingly
    a = M(Sprite_Attributes + 36); // get empty sprite attributes
    a |= 0b01000000; // set horizontal flip bit for bottom-right sprite
    writeData(Sprite_Attributes + 32, a); // store and leave
    goto Return;

//------------------------------------------------------------------------

RenderPlayerSub:
    writeData(0x07, a); // store number of rows of sprites to draw
    a = M(Player_Rel_XPos);
    writeData(Player_Pos_ForScroll, a); // store player's relative horizontal position
    writeData(0x05, a); // store it here also
    a = M(Player_Rel_YPos);
    writeData(0x02, a); // store player's vertical position
    a = M(PlayerFacingDir);
    writeData(0x03, a); // store player's facing direction
    a = M(Player_SprAttrib);
    writeData(0x04, a); // store player's sprite attributes
    x = M(PlayerGfxOffset); // load graphics table offset
    y = M(Player_SprDataOffset); // get player's sprite data offset

DrawPlayerLoop:
    a = M(PlayerGraphicsTable + x); // load player's left side
    writeData(0x00, a);
    a = M(PlayerGraphicsTable + 1 + x); // now load right side
    JSR(DrawOneSpriteRow, 540);
    --M(0x07); // decrement rows of sprites to draw
    if (M(0x07) != 0)
        goto DrawPlayerLoop; // do this until all rows are drawn
    goto Return;

//------------------------------------------------------------------------

ProcessPlayerAction:
    a = M(Player_State); // get player's state
    if (a != 0x03)
    { // if climbing, branch here
        if (a != 0x02)
        { // if falling, branch here
            if (a == 0x01)
            { // if not jumping, branch here
                a = M(SwimmingFlag);
                if (a != 0)
                    goto ActionSwimming; // if swimming flag set, branch elsewhere
                y = 0x06; // load offset for crouching
                a = M(CrouchingFlag); // get crouching flag
                if (a != 0)
                    goto NonAnimatedActs; // if set, branch to get offset for graphics table
                y = 0x00; // otherwise load offset for jumping
                goto NonAnimatedActs; // go to get offset to graphics table
            } // ProcOnGroundActs
            y = 0x06; // load offset for crouching
            a = M(CrouchingFlag); // get crouching flag
            if (a != 0)
                goto NonAnimatedActs; // if set, branch to get offset for graphics table
            y = 0x02; // load offset for standing
            a = M(Player_X_Speed); // check player's horizontal speed
            a |= M(Left_Right_Buttons); // and left/right controller bits
            if (a == 0)
                goto NonAnimatedActs; // if no speed or buttons pressed, use standing offset
            a = M(Player_XSpeedAbsolute); // load walking/running speed
            if (a < 0x09)
                goto ActionWalkRun; // if less than a certain amount, branch, too slow to skid
            a = M(Player_MovingDir); // otherwise check to see if moving direction
            a &= M(PlayerFacingDir); // and facing direction are the same
            if (a != 0)
                goto ActionWalkRun; // if moving direction = facing direction, branch, don't skid
            ++y; // otherwise increment to skid offset ($03)

NonAnimatedActs:
            JSR(GetGfxOffsetAdder, 541); // do a sub here to get offset adder for graphics table
            a = 0x00;
            writeData(PlayerAnimCtrl, a); // initialize animation frame control
            a = M(PlayerGfxTblOffsets + y); // load offset to graphics table using size as offset
            goto Return;

        //------------------------------------------------------------------------
        } // ActionFalling
        y = 0x04; // load offset for walking/running
        JSR(GetGfxOffsetAdder, 542); // get offset to graphics table
        goto GetCurrentAnimOffset; // execute instructions for falling state

ActionWalkRun:
        y = 0x04; // load offset for walking/running
        JSR(GetGfxOffsetAdder, 543); // get offset to graphics table
        goto FourFrameExtent; // execute instructions for normal state
    } // ActionClimbing
    y = 0x05; // load offset for climbing
    a = M(Player_Y_Speed); // check player's vertical speed
    if (a == 0)
        goto NonAnimatedActs; // if no speed, branch, use offset as-is
    JSR(GetGfxOffsetAdder, 544); // otherwise get offset for graphics table
    goto ThreeFrameExtent; // then skip ahead to more code

ActionSwimming:
    y = 0x01; // load offset for swimming
    JSR(GetGfxOffsetAdder, 545);
    a = M(JumpSwimTimer); // check jump/swim timer
    a |= M(PlayerAnimCtrl); // and animation frame control
    if (a != 0)
        goto FourFrameExtent; // if any one of these set, branch ahead
    a = M(A_B_Buttons);
    a <<= 1; // check for A button pressed
    if ((M(A_B_Buttons) & 0x80) != 0)
        goto FourFrameExtent; // branch to same place if A button pressed

GetCurrentAnimOffset:
    a = M(PlayerAnimCtrl); // get animation frame control
    goto GetOffsetFromAnimCtrl; // jump to get proper offset to graphics table

FourFrameExtent:
    a = 0x03; // load upper extent for frame control
    goto AnimationControl; // jump to get offset and animate player object

ThreeFrameExtent:
    a = 0x02; // load upper extent for frame control for climbing

AnimationControl:
    writeData(0x00, a); // store upper extent here
    JSR(GetCurrentAnimOffset, 546); // get proper offset to graphics table
    pha(); // save offset to stack
    a = M(PlayerAnimTimer); // load animation frame timer
    if (a == 0)
    { // branch if not expired
        a = M(PlayerAnimTimerSet); // get animation frame timer amount
        writeData(PlayerAnimTimer, a); // and set timer accordingly
        a = M(PlayerAnimCtrl);
        a += 0x01;
        if (a >= M(0x00))
        { // if frame control + 1 < upper extent, use as next
            a = 0x00; // otherwise initialize frame control
        } // SetAnimC: store as new animation frame control
        writeData(PlayerAnimCtrl, a);
    } // ExAnimC: get offset to graphics table from stack and leave
    pla();
    goto Return;

//------------------------------------------------------------------------

GetGfxOffsetAdder:
    a = M(PlayerSize); // get player's size
    if (a != 0)
    { // if player big, use current offset as-is
        a = y; // for big player
        a += 0x08; // for small player
        y = a;
    } // SzOfs: go back
    goto Return;

//------------------------------------------------------------------------

HandleChangeSize:
    y = M(PlayerAnimCtrl); // get animation frame control
    a = M(FrameCounter);
    a &= 0b00000011; // get frame counter and execute this code every
    if (a == 0)
    { // fourth frame, otherwise branch ahead
        ++y; // increment frame control
        if (y >= 0x0a)
        { // if not there yet, skip ahead to use
            y = 0x00; // otherwise initialize both grow/shrink flag
            writeData(PlayerChangeSizeFlag, y); // and animation frame control
        } // CSzNext: store proper frame control
        writeData(PlayerAnimCtrl, y);
    } // GorSLog: get player's size
    a = M(PlayerSize);
    if (a == 0)
    { // if player small, skip ahead to next part
        a = M(ChangeSizeOffsetAdder + y); // get offset adder based on frame control as offset
        y = 0x0f; // load offset for player growing

GetOffsetFromAnimCtrl:
        a <<= 1; // multiply animation frame control
        a <<= 1; // by eight to get proper amount
        a <<= 1; // to add to our offset
        a += M(PlayerGfxTblOffsets + y); // add to offset to graphics table
        goto Return; // and return with result in A

    //------------------------------------------------------------------------
    } // ShrinkPlayer
    a = y; // add ten bytes to frame control as offset
    a += 0x0a; // this thing apparently uses two of the swimming frames
    x = a; // to draw the player shrinking
    y = 0x09; // load offset for small player swimming
    a = M(ChangeSizeOffsetAdder + x); // get what would normally be offset adder
    if (a == 0)
    { // and branch to use offset if nonzero
        y = 0x01; // otherwise load offset for big player swimming
    } // ShrPlF: get offset to graphics table based on offset loaded
    a = M(PlayerGfxTblOffsets + y);
    goto Return; // and leave

//------------------------------------------------------------------------

ChkForPlayerAttrib:
    y = M(Player_SprDataOffset); // get sprite data offset
    a = M(GameEngineSubroutine);
    if (a != 0x0b)
    { // branch to change third and fourth row OAM attributes
        a = M(PlayerGfxOffset); // get graphics table offset
        if (a == 0x50)
            goto C_S_IGAtt; // if crouch offset, either standing offset,
        if (a == 0xb8)
            goto C_S_IGAtt; // go ahead and execute code to change
        if (a == 0xc0)
            goto C_S_IGAtt;
        if (a != 0xc8)
            goto ExPlyrAt; // if none of these, branch to leave
    } // KilledAtt
    a = M(Sprite_Attributes + 16 + y);
    a &= 0b00111111; // mask out horizontal and vertical flip bits
    writeData(Sprite_Attributes + 16 + y, a); // for third row sprites and save
    a = M(Sprite_Attributes + 20 + y);
    a &= 0b00111111;
    a |= 0b01000000; // set horizontal flip bit for second
    writeData(Sprite_Attributes + 20 + y, a); // sprite in the third row

C_S_IGAtt:
    a = M(Sprite_Attributes + 24 + y);
    a &= 0b00111111; // mask out horizontal and vertical flip bits
    writeData(Sprite_Attributes + 24 + y, a); // for fourth row sprites and save
    a = M(Sprite_Attributes + 28 + y);
    a &= 0b00111111;
    a |= 0b01000000; // set horizontal flip bit for second
    writeData(Sprite_Attributes + 28 + y, a); // sprite in the fourth row

ExPlyrAt: // leave
    goto Return;

//------------------------------------------------------------------------

RelativePlayerPosition:
    x = 0x00; // set offsets for relative cooordinates
    y = 0x00; // routine to correspond to player object
    goto RelWOfs; // get the coordinates

RelativeBubblePosition:
    y = 0x01; // set for air bubble offsets
    JSR(GetProperObjOffset, 547); // modify X to get proper air bubble offset
    y = 0x03;
    goto RelWOfs; // get the coordinates

RelativeFireballPosition:
    y = 0x00; // set for fireball offsets
    JSR(GetProperObjOffset, 548); // modify X to get proper fireball offset
    y = 0x02;

RelWOfs: // get the coordinates
    JSR(GetObjRelativePosition, 549);
    x = M(ObjectOffset); // return original offset
    goto Return; // leave

//------------------------------------------------------------------------

RelativeMiscPosition:
    y = 0x02; // set for misc object offsets
    JSR(GetProperObjOffset, 550); // modify X to get proper misc object offset
    y = 0x06;
    goto RelWOfs; // get the coordinates

RelativeEnemyPosition:
    a = 0x01; // get coordinates of enemy object
    y = 0x01; // relative to the screen
    goto VariableObjOfsRelPos;

RelativeBlockPosition:
    a = 0x09; // get coordinates of one block object
    y = 0x04; // relative to the screen
    JSR(VariableObjOfsRelPos, 551);
    ++x; // adjust offset for other block object if any
    ++x;
    a = 0x09;
    ++y; // adjust other and get coordinates for other one

VariableObjOfsRelPos:
    writeData(0x00, x); // store value to add to A here
    a += M(0x00); // add A to value stored
    x = a; // use as enemy offset
    JSR(GetObjRelativePosition, 552);
    x = M(ObjectOffset); // reload old object offset and leave
    goto Return;

//------------------------------------------------------------------------

GetObjRelativePosition:
    a = M(SprObject_Y_Position + x); // load vertical coordinate low
    writeData(SprObject_Rel_YPos + y, a); // store here
    a = M(SprObject_X_Position + x); // load horizontal coordinate
    a -= M(ScreenLeft_X_Pos);
    writeData(SprObject_Rel_XPos + y, a); // store result here
    goto Return;

//------------------------------------------------------------------------

GetPlayerOffscreenBits:
    x = 0x00; // set offsets for player-specific variables
    y = 0x00; // and get offscreen information about player
    goto GetOffScreenBitsSet;

GetFireballOffscreenBits:
    y = 0x00; // set for fireball offsets
    JSR(GetProperObjOffset, 553); // modify X to get proper fireball offset
    y = 0x02; // set other offset for fireball's offscreen bits
    goto GetOffScreenBitsSet; // and get offscreen information about fireball

GetBubbleOffscreenBits:
    y = 0x01; // set for air bubble offsets
    JSR(GetProperObjOffset, 554); // modify X to get proper air bubble offset
    y = 0x03; // set other offset for airbubble's offscreen bits
    goto GetOffScreenBitsSet; // and get offscreen information about air bubble

GetMiscOffscreenBits:
    y = 0x02; // set for misc object offsets
    JSR(GetProperObjOffset, 555); // modify X to get proper misc object offset
    y = 0x06; // set other offset for misc object's offscreen bits
    goto GetOffScreenBitsSet; // and get offscreen information about misc object

GetProperObjOffset:
    a = x; // move offset to A
    a += M(ObjOffsetData + y); // add amount of bytes to offset depending on setting in Y
    x = a; // put back in X and leave
    goto Return;

//------------------------------------------------------------------------

GetEnemyOffscreenBits:
    a = 0x01; // set A to add 1 byte in order to get enemy offset
    y = 0x01; // set Y to put offscreen bits in Enemy_OffscreenBits
    goto SetOffscrBitsOffset;

GetBlockOffscreenBits:
    a = 0x09; // set A to add 9 bytes in order to get block obj offset
    y = 0x04; // set Y to put offscreen bits in Block_OffscreenBits

SetOffscrBitsOffset:
    writeData(0x00, x);
    a += M(0x00); // appropriate offset, then give back to X
    x = a;

GetOffScreenBitsSet:
    a = y; // save offscreen bits offset to stack for now
    pha();
    JSR(RunOffscrBitsSubs, 556);
    a <<= 1; // move low nybble to high nybble
    a <<= 1;
    a <<= 1;
    a <<= 1;
    a |= M(0x00); // mask together with previously saved low nybble
    writeData(0x00, a); // store both here
    pla(); // get offscreen bits offset from stack
    y = a;
    a = M(0x00); // get value here and store elsewhere
    writeData(SprObject_OffscrBits + y, a);
    x = M(ObjectOffset);
    goto Return;

//------------------------------------------------------------------------

RunOffscrBitsSubs:
    JSR(GetXOffscreenBits, 557); // do subroutine here
    a >>= 1; // move high nybble to low
    a >>= 1;
    a >>= 1;
    a >>= 1;
    writeData(0x00, a); // store here
    goto GetYOffscreenBits;

GetXOffscreenBits:
    writeData(0x04, x); // save position in buffer to here
    y = 0x01; // start with right side of screen

XOfsLoop: // get pixel coordinate of edge
    // the edge and the object position are each one 16-bit page:coordinate
    wide = ((M(ScreenEdge_PageLoc + y) << 8) | M(ScreenEdge_X_Pos + y))
         - ((M(SprObject_PageLoc + x) << 8) | M(SprObject_X_Position + x)); // get difference between them
    writeData(0x07, LOBYTE(wide)); // store here
    a = HIBYTE(wide);
    x = M(DefaultXOnscreenOfs + y); // load offset value here
    if ((a & 0x80) != 0)
        goto XLdBData; // if beyond right edge or in front of left edge, branch
    x = M(DefaultXOnscreenOfs + 1 + y); // if not, load alternate offset value here
    if (((a - 0x01) & 0x80) == 0)
        goto XLdBData; // if one page or more to the left of either edge, branch
    a = 0x38; // if no branching, load value here and store
    writeData(0x06, a);
    a = 0x08; // load some other value and execute subroutine
    JSR(DividePDiff, 558);

XLdBData: // get bits here
    a = M(XOffscreenBitsData + x);
    x = M(0x04); // reobtain position in buffer
    if (a == 0x00)
    {
        --y; // otherwise, do left side of screen now
        if ((y & 0x80) == 0)
            goto XOfsLoop; // branch if not already done with left side
    } // ExXOfsBS
    goto Return;

//------------------------------------------------------------------------

GetYOffscreenBits:
    writeData(0x04, x); // save position in buffer to here
    y = 0x01; // start with top of screen

YOfsLoop: // load coordinate for edge of vertical unit
    // the edge of the vertical unit and the object position are each one 16-bit highpos:coordinate
    wide = ((0x01 << 8) | M(HighPosUnitData + y))
         - ((M(SprObject_Y_HighPos + x) << 8) | M(SprObject_Y_Position + x)); // subtract from vertical coordinate of object
    writeData(0x07, LOBYTE(wide)); // store here
    a = HIBYTE(wide);
    x = M(DefaultYOnscreenOfs + y); // load offset value here
    if ((a & 0x80) != 0)
        goto YLdBData; // if under top of the screen or beyond bottom, branch
    x = M(DefaultYOnscreenOfs + 1 + y); // if not, load alternate offset value here
    if (((a - 0x01) & 0x80) == 0)
        goto YLdBData; // if one vertical unit or more above the screen, branch
    a = 0x20; // if no branching, load value here and store
    writeData(0x06, a);
    a = 0x04; // load some other value and execute subroutine
    JSR(DividePDiff, 559);

YLdBData: // get offscreen data bits using offset
    a = M(YOffscreenBitsData + x);
    x = M(0x04); // reobtain position in buffer
    if (a == 0x00)
    { // if bits not zero, branch to leave
        --y; // otherwise, do bottom of the screen now
        if ((y & 0x80) == 0)
            goto YOfsLoop;
    } // ExYOfsBS
    goto Return;

//------------------------------------------------------------------------

DividePDiff:
    writeData(0x05, a); // store current value in A here
    a = M(0x07); // get pixel difference
    if (a < M(0x06))
    { // if pixel difference >= preset value, branch
        a >>= 1; // divide by eight
        a >>= 1;
        a >>= 1;
        a &= 0x07; // mask out all but 3 LSB
        if (y < 0x01)
        { // if so, branch, use difference / 8 as offset
            a += M(0x05); // if not, add value to difference / 8
        } // SetOscrO: use as offset
        x = a;
    } // ExDivPD: leave
    goto Return;

//------------------------------------------------------------------------

DrawSpriteObject:
    a = M(0x03); // get saved flip control bits
    a >>= 1;
    shiftedBit = (a & 0x01) != 0;
    a = M(0x00);
    if (shiftedBit)
    { // if d1 not set, branch
        writeData(Sprite_Tilenumber + 4 + y, a); // store first tile into second sprite
        a = M(0x01); // and second into first sprite
        writeData(Sprite_Tilenumber + y, a);
        a = 0x40; // activate horizontal flip OAM attribute
        if (a != 0)
            goto SetHFAt; // and unconditionally branch
    } // NoHFlip: store first tile into first sprite
    writeData(Sprite_Tilenumber + y, a);
    a = M(0x01); // and second into second sprite
    writeData(Sprite_Tilenumber + 4 + y, a);
    a = 0x00; // clear bit for horizontal flip

SetHFAt: // add other OAM attributes if necessary
    a |= M(0x04);
    writeData(Sprite_Attributes + y, a); // store sprite attributes
    writeData(Sprite_Attributes + 4 + y, a);
    a = M(0x02); // now the y coordinates
    writeData(Sprite_Y_Position + y, a); // note because they are
    writeData(Sprite_Y_Position + 4 + y, a); // side by side, they are the same
    a = M(0x05);
    writeData(Sprite_X_Position + y, a); // store x coordinate, then
    a += 0x08; // put them side by side
    writeData(Sprite_X_Position + 4 + y, a);
    a = M(0x02); // add eight pixels to the next y
    a += 0x08;
    writeData(0x02, a);
    a = y; // add eight to the offset in Y to
    a += 0x08;
    y = a;
    ++x; // increment offset to return it to the
    ++x; // routine that called this subroutine
    goto Return;

//------------------------------------------------------------------------

SoundEngine:
    a = M(OperMode); // are we in title screen mode?
    if (a == 0)
    {
        writeData(SND_MASTERCTRL_REG, a); // if so, disable sound and leave
        goto Return;

    //------------------------------------------------------------------------
    } // SndOn
    a = 0xff;
    writeData(JOYPAD_PORT2, a); // disable irqs and set frame counter mode???
    a = 0x0f;
    writeData(SND_MASTERCTRL_REG, a); // enable first four channels
    a = M(PauseModeFlag); // is sound already in pause mode?
    if (a == 0)
    {
        a = M(PauseSoundQueue); // if not, check pause sfx queue
        if (a != 0x01)
            goto RunSoundSubroutines; // if queue is empty, skip pause mode routine
    } // InPause: check pause sfx buffer
    a = M(PauseSoundBuffer);
    if (a == 0)
    {
        a = M(PauseSoundQueue); // check pause queue
        if (a == 0)
            goto SkipSoundSubroutines;
        writeData(PauseSoundBuffer, a); // if queue full, store in buffer and activate
        writeData(PauseModeFlag, a); // pause mode to interrupt game sounds
        a = 0x00; // disable sound and clear sfx buffers
        writeData(SND_MASTERCTRL_REG, a);
        writeData(Square1SoundBuffer, a);
        writeData(Square2SoundBuffer, a);
        writeData(NoiseSoundBuffer, a);
        a = 0x0f;
        writeData(SND_MASTERCTRL_REG, a); // enable sound again
        a = 0x2a; // store length of sound in pause counter
        writeData(Squ1_SfxLenCounter, a);

PTone1F: // play first tone
        a = 0x44;
        if (a != 0)
            goto PTRegC; // unconditional branch
    } // ContPau: check pause length left
    a = M(Squ1_SfxLenCounter);
    if (a != 0x24)
    {
        if (a == 0x1e)
            goto PTone1F;
        if (a != 0x18)
            goto DecPauC; // only load regs during times, otherwise skip
    } // PTone2F: store reg contents and play the pause sfx
    a = 0x64;

PTRegC:
    x = 0x84;
    y = 0x7f;
    JSR(PlaySqu1Sfx, 560);

DecPauC: // decrement pause sfx counter
    --M(Squ1_SfxLenCounter);
    if (M(Squ1_SfxLenCounter) != 0)
        goto SkipSoundSubroutines;
    a = 0x00; // disable sound if in pause mode and
    writeData(SND_MASTERCTRL_REG, a); // not currently playing the pause sfx
    a = M(PauseSoundBuffer); // if no longer playing pause sfx, check to see
    if (a == 0x02)
    {
        a = 0x00; // clear pause mode to allow game sounds again
        writeData(PauseModeFlag, a);
    } // SkipPIn: clear pause sfx buffer
    a = 0x00;
    writeData(PauseSoundBuffer, a);
    if (a == 0)
        goto SkipSoundSubroutines;

RunSoundSubroutines:
    JSR(Square1SfxHandler, 561); // play sfx on square channel 1
    JSR(Square2SfxHandler, 562); //  ''  ''  '' square channel 2
    JSR(NoiseSfxHandler, 563); //  ''  ''  '' noise channel
    JSR(MusicHandler, 564); // play music on all channels
    a = 0x00; // clear the music queues
    writeData(AreaMusicQueue, a);
    writeData(EventMusicQueue, a);

SkipSoundSubroutines:
    a = 0x00; // clear the sound effects queues
    writeData(Square1SoundQueue, a);
    writeData(Square2SoundQueue, a);
    writeData(NoiseSoundQueue, a);
    writeData(PauseSoundQueue, a);
    y = M(DAC_Counter); // load some sort of counter
    a = M(AreaMusicBuffer);
    a &= 0b00000011; // check for specific music
    if (a != 0)
    {
        ++M(DAC_Counter); // increment and check counter
        if (y < 0x30)
            goto StrWave; // if not there yet, just store it
    } // NoIncDAC
    a = y;
    if (a == 0)
        goto StrWave; // if we are at zero, do not decrement
    --M(DAC_Counter); // decrement counter

StrWave: // store into DMC load register (??)
    writeData(SND_DELTA_REG + 1, y);
    goto Return; // we are done here

//------------------------------------------------------------------------

Dump_Squ1_Regs:
    writeData(SND_SQUARE1_REG + 1, y); // dump the contents of X and Y into square 1's control regs
    writeData(SND_SQUARE1_REG, x);
    goto Return;

//------------------------------------------------------------------------

PlaySqu1Sfx:
    JSR(Dump_Squ1_Regs, 565); // do sub to set ctrl regs for square 1, then set frequency regs

SetFreq_Squ1:
    x = 0x00; // set frequency reg offset for square 1 sound channel

Dump_Freq_Regs:
    y = a;
    a = M(FreqRegLookupTbl + 1 + y); // use previous contents of A for sound reg offset
    if (a != 0)
    { // if zero, then do not load
        writeData(SND_REGISTER + 2 + x, a); // first byte goes into LSB of frequency divider
        a = M(FreqRegLookupTbl + y); // second byte goes into 3 MSB plus extra bit for
        a |= 0b00001000; // length counter
        writeData(SND_REGISTER + 3 + x, a);
    } // NoTone
    goto Return;

//------------------------------------------------------------------------

Dump_Sq2_Regs:
    writeData(SND_SQUARE2_REG, x); // dump the contents of X and Y into square 2's control regs
    writeData(SND_SQUARE2_REG + 1, y);
    goto Return;

//------------------------------------------------------------------------

PlaySqu2Sfx:
    JSR(Dump_Sq2_Regs, 566); // do sub to set ctrl regs for square 2, then set frequency regs

SetFreq_Squ2:
    x = 0x04; // set frequency reg offset for square 2 sound channel
    if (x != 0)
        goto Dump_Freq_Regs; // unconditional branch

SetFreq_Tri:
    x = 0x08; // set frequency reg offset for triangle sound channel
    if (x != 0)
        goto Dump_Freq_Regs; // unconditional branch

PlayFlagpoleSlide:
    a = 0x40; // store length of flagpole sound
    writeData(Squ1_SfxLenCounter, a);
    a = 0x62; // load part of reg contents for flagpole sound
    JSR(SetFreq_Squ1, 567);
    x = 0x99; // now load the rest
    if (x == 0)
    {

PlaySmallJump:
        a = 0x26; // branch here for small mario jumping sound
        if (a == 0)
        {

PlayBigJump:
            a = 0x18; // branch here for big mario jumping sound
        } // JumpRegContents
        x = 0x82; // note that small and big jump borrow each others' reg contents
        y = 0xa7; // anyway, this loads the first part of mario's jumping sound
        JSR(PlaySqu1Sfx, 568);
        a = 0x28; // store length of sfx for both jumping sounds
        writeData(Squ1_SfxLenCounter, a); // then continue on here

ContinueSndJump:
        a = M(Squ1_SfxLenCounter); // jumping sounds seem to be composed of three parts
        if (a == 0x25)
        {
            x = 0x5f; // load second part
            y = 0xf6;
            if (y != 0)
                goto DmpJpFPS; // unconditional branch
        } // N2Prt: check for third part
        if (a != 0x20)
            goto DecJpFPS;
        x = 0x48; // load third part
    } // FPS2nd: the flagpole slide sound shares part of third part
    y = 0xbc;

DmpJpFPS:
    JSR(Dump_Squ1_Regs, 569);
    if (y != 0)
        goto DecJpFPS; // unconditional branch outta here

PlayFireballThrow:
    a = 0x05;
    y = 0x99; // load reg contents for fireball throw sound
    if (y == 0)
    { // unconditional branch

PlayBump:
        a = 0x0a; // load length of sfx and reg contents for bump sound
        y = 0x93;
    } // Fthrow: the fireball sound shares reg contents with the bump sound
    x = 0x9e;
    writeData(Squ1_SfxLenCounter, a);
    a = 0x0c; // load offset for bump sound
    JSR(PlaySqu1Sfx, 570);

ContinueBumpThrow:
    a = M(Squ1_SfxLenCounter); // check for second part of bump sound
    if (a != 0x06)
        goto DecJpFPS;
    a = 0xbb; // load second part directly
    writeData(SND_SQUARE1_REG + 1, a);

DecJpFPS: // unconditional branch, however we got here
    goto BranchToDecLength1;

Square1SfxHandler:
    y = M(Square1SoundQueue); // check for sfx in queue
    if (y != 0)
    {
        writeData(Square1SoundBuffer, y); // if found, put in buffer
        if ((y & Sfx_SmallJump) != 0)
            goto PlaySmallJump; // small jump
        if ((y & Sfx_BigJump) != 0)
            goto PlayBigJump; // big jump
        if ((y & Sfx_Bump) != 0)
            goto PlayBump; // bump
        if ((y & Sfx_EnemyStomp) != 0)
            goto PlaySwimStomp; // swim/stomp
        if ((y & Sfx_EnemySmack) != 0)
            goto PlaySmackEnemy; // smack enemy
        if ((y & Sfx_PipeDown_Injury) != 0)
            goto PlayPipeDownInj; // pipedown/injury
        if ((y & Sfx_Fireball) != 0)
            goto PlayFireballThrow; // fireball throw
        if ((y & Sfx_Flagpole) != 0)
            goto PlayFlagpoleSlide; // slide flagpole
    } // CheckSfx1Buffer
    a = M(Square1SoundBuffer); // check for sfx in buffer
    if (a != 0)
    { // if not found, exit sub
        if ((a & Sfx_SmallJump) != 0)
            goto ContinueSndJump; // small mario jump
        if ((a & Sfx_BigJump) != 0)
            goto ContinueSndJump; // big mario jump
        if ((a & Sfx_Bump) != 0)
            goto ContinueBumpThrow; // bump
        if ((a & Sfx_EnemyStomp) != 0)
            goto ContinueSwimStomp; // swim/stomp
        if ((a & Sfx_EnemySmack) != 0)
            goto ContinueSmackEnemy; // smack enemy
        if ((a & Sfx_PipeDown_Injury) != 0)
            goto ContinuePipeDownInj; // pipedown/injury
        if ((a & Sfx_Fireball) != 0)
            goto ContinueBumpThrow; // fireball throw
        if ((a & Sfx_Flagpole) != 0)
            goto DecrementSfx1Length; // slide flagpole
    } // ExS1H
    goto Return;

//------------------------------------------------------------------------

PlaySwimStomp:
    a = 0x0e; // store length of swim/stomp sound
    writeData(Squ1_SfxLenCounter, a);
    y = 0x9c; // store reg contents for swim/stomp sound
    x = 0x9e;
    a = 0x26;
    JSR(PlaySqu1Sfx, 571);

ContinueSwimStomp:
    y = M(Squ1_SfxLenCounter); // look up reg contents in data section based on
    a = M(SwimStompEnvelopeData - 1 + y); // length of sound left, used to control sound's
    writeData(SND_SQUARE1_REG, a); // envelope
    if (y != 0x06)
        goto BranchToDecLength1;
    a = 0x9e; // when the length counts down to a certain point, put this
    writeData(SND_SQUARE1_REG + 2, a); // directly into the LSB of square 1's frequency divider

BranchToDecLength1:
    goto DecrementSfx1Length; // unconditional branch (regardless of how we got here)

PlaySmackEnemy:
    a = 0x0e; // store length of smack enemy sound
    y = 0xcb;
    x = 0x9f;
    writeData(Squ1_SfxLenCounter, a);
    a = 0x28; // store reg contents for smack enemy sound
    JSR(PlaySqu1Sfx, 572);
    if (a != 0)
        goto DecrementSfx1Length; // unconditional branch

ContinueSmackEnemy:
    y = M(Squ1_SfxLenCounter); // check about halfway through
    if (y == 0x08)
    {
        a = 0xa0; // if we're at the about-halfway point, make the second tone
        writeData(SND_SQUARE1_REG + 2, a); // in the smack enemy sound
        a = 0x9f;
        if (a != 0)
            goto SmTick;
    } // SmSpc: this creates spaces in the sound, giving it its distinct noise
    a = 0x90;

SmTick:
    writeData(SND_SQUARE1_REG, a);

DecrementSfx1Length:
    --M(Squ1_SfxLenCounter); // decrement length of sfx
    if (M(Squ1_SfxLenCounter) == 0)
    {

StopSquare1Sfx:
        x = 0x00; // if end of sfx reached, clear buffer
        writeData(0xf1, x); // and stop making the sfx
        x = 0x0e;
        writeData(SND_MASTERCTRL_REG, x);
        x = 0x0f;
        writeData(SND_MASTERCTRL_REG, x);
    } // ExSfx1
    goto Return;

//------------------------------------------------------------------------

PlayPipeDownInj:
    a = 0x2f; // load length of pipedown sound
    writeData(Squ1_SfxLenCounter, a);

ContinuePipeDownInj:
    a = M(Squ1_SfxLenCounter); // some bitwise logic, forces the regs
    a >>= 1; // to be written to only during six specific times
    if ((M(Squ1_SfxLenCounter) & 0x01) != 0)
        goto NoPDwnL; // during which d3 must be set and d1-0 must be clear
    shiftedBit = (a & 0x01) != 0;
    a >>= 1;
    if (shiftedBit)
        goto NoPDwnL;
    a &= 0b00000010;
    if (a == 0)
        goto NoPDwnL;
    y = 0x91; // and this is where it actually gets written in
    x = 0x9a;
    a = 0x44;
    JSR(PlaySqu1Sfx, 573);

NoPDwnL:
    goto DecrementSfx1Length;

PlayCoinGrab:
    a = 0x35; // load length of coin grab sound
    x = 0x8d; // and part of reg contents
    if (x == 0)
    {

PlayTimerTick:
        a = 0x06; // load length of timer tick sound
        x = 0x98; // and part of reg contents
    } // CGrab_TTickRegL
    writeData(Squ2_SfxLenCounter, a);
    y = 0x7f; // load the rest of reg contents
    a = 0x42; // of coin grab and timer tick sound
    JSR(PlaySqu2Sfx, 574);

ContinueCGrabTTick:
    a = M(Squ2_SfxLenCounter); // check for time to play second tone yet
    if (a == 0x30)
    {
        a = 0x54; // if so, load the tone directly into the reg
        writeData(SND_SQUARE2_REG + 2, a);
    } // N2Tone
    goto DecrementSfx2Length; // unconditional branch, however we got here

PlayBlast:
    a = 0x20; // load length of fireworks/gunfire sound
    writeData(Squ2_SfxLenCounter, a);
    y = 0x94; // load reg contents of fireworks/gunfire sound
    a = 0x5e;
    if (a == 0)
    {

ContinueBlast:
        a = M(Squ2_SfxLenCounter); // check for time to play second part
        if (a != 0x18)
            goto DecrementSfx2Length;
        y = 0x93; // load second part reg contents then
        a = 0x18;
    } // SBlasJ: unconditional branch to load rest of reg contents
    if (a == 0)
    {

PlayPowerUpGrab:
        a = 0x36; // load length of power-up grab sound
        writeData(Squ2_SfxLenCounter, a);

ContinuePowerUpGrab:
        a = M(Squ2_SfxLenCounter); // load frequency reg based on length left over
        a >>= 1; // divide by 2
        if ((M(Squ2_SfxLenCounter) & 0x01) != 0)
            goto DecrementSfx2Length; // alter frequency every other frame
        y = a;
        a = M(PowerUpGrabFreqData - 1 + y); // use length left over / 2 for frequency offset
        x = 0x5d; // store reg contents of power-up grab sound
        y = 0x7f;

LoadSqu2Regs:
        JSR(PlaySqu2Sfx, 575);

DecrementSfx2Length:
        --M(Squ2_SfxLenCounter); // decrement length of sfx
        if (M(Squ2_SfxLenCounter) == 0)
        {

EmptySfx2Buffer:
            x = 0x00; // initialize square 2's sound effects buffer
            writeData(Square2SoundBuffer, x);

StopSquare2Sfx:
            x = 0x0d; // stop playing the sfx
            writeData(SND_MASTERCTRL_REG, x);
            x = 0x0f;
            writeData(SND_MASTERCTRL_REG, x);
        } // ExSfx2
        goto Return;

    //------------------------------------------------------------------------

Square2SfxHandler:
        a = M(Square2SoundBuffer); // special handling for the 1-up sound to keep it
        a &= Sfx_ExtraLife; // from being interrupted by other sounds on square 2
        if (a != 0)
            goto ContinueExtraLife;
        y = M(Square2SoundQueue); // check for sfx in queue
        if (y != 0)
        {
            writeData(Square2SoundBuffer, y); // if found, put in buffer and check for the following
            if ((y & Sfx_BowserFall) != 0)
                goto PlayBowserFall; // bowser fall
            if ((y & Sfx_CoinGrab) != 0)
                goto PlayCoinGrab; // coin grab
            if ((y & Sfx_GrowPowerUp) != 0)
                goto PlayGrowPowerUp; // power-up reveal
            if ((y & Sfx_GrowVine) != 0)
                goto PlayGrowVine; // vine grow
            if ((y & Sfx_Blast) != 0)
                goto PlayBlast; // fireworks/gunfire
            if ((y & Sfx_TimerTick) != 0)
                goto PlayTimerTick; // timer tick
            if ((y & Sfx_PowerUpGrab) != 0)
                goto PlayPowerUpGrab; // power-up grab
            if ((y & Sfx_ExtraLife) != 0)
                goto PlayExtraLife; // 1-up
        } // CheckSfx2Buffer
        a = M(Square2SoundBuffer); // check for sfx in buffer
        if (a != 0)
        { // if not found, exit sub
            if ((a & Sfx_BowserFall) != 0)
                goto ContinueBowserFall; // bowser fall
            if ((a & Sfx_CoinGrab) != 0)
                goto Cont_CGrab_TTick; // coin grab
            if ((a & Sfx_GrowPowerUp) != 0)
                goto ContinueGrowItems; // power-up reveal
            if ((a & Sfx_GrowVine) != 0)
                goto ContinueGrowItems; // vine grow
            if ((a & Sfx_Blast) != 0)
                goto ContinueBlast; // fireworks/gunfire
            if ((a & Sfx_TimerTick) != 0)
                goto Cont_CGrab_TTick; // timer tick
            if ((a & Sfx_PowerUpGrab) != 0)
                goto ContinuePowerUpGrab; // power-up grab
            if ((a & Sfx_ExtraLife) != 0)
                goto ContinueExtraLife; // 1-up
        } // ExS2H
        goto Return;

    //------------------------------------------------------------------------

Cont_CGrab_TTick:
        goto ContinueCGrabTTick;

JumpToDecLength2:
        goto DecrementSfx2Length;

PlayBowserFall:
        a = 0x38; // load length of bowser defeat sound
        writeData(Squ2_SfxLenCounter, a);
        y = 0xc4; // load contents of reg for bowser defeat sound
        a = 0x18;
    } // BlstSJp
    if (a == 0)
    {

ContinueBowserFall:
        a = M(Squ2_SfxLenCounter); // check for almost near the end
        if (a != 0x08)
            goto DecrementSfx2Length;
        y = 0xa4; // if so, load the rest of reg contents for bowser defeat sound
        a = 0x5a;
    } // PBFRegs: the fireworks/gunfire sound shares part of reg contents here
    x = 0x9f;

    do // EL_LRegs: this is an unconditional branch outta here
    {
        goto LoadSqu2Regs;

PlayExtraLife:
        a = 0x30; // load length of 1-up sound
        writeData(Squ2_SfxLenCounter, a);

ContinueExtraLife:
        a = M(Squ2_SfxLenCounter);
        x = 0x03; // load new tones only every eight frames

        do // DivLLoop
        {
            shiftedBit = (a & 0x01) != 0;
            a >>= 1;
            if (shiftedBit)
                goto JumpToDecLength2; // if any bits set here, branch to dec the length
            --x;
        } while (x != 0); // do this until all bits checked, if none set, continue
        y = a;
        a = M(ExtraLifeFreqData - 1 + y); // load our reg contents
        x = 0x82;
        y = 0x7f;
    } while (y != 0); // unconditional branch

PlayGrowPowerUp:
    a = 0x10; // load length of power-up reveal sound
    if (a == 0)
    {

PlayGrowVine:
        a = 0x20; // load length of vine grow sound
    } // GrowItemRegs
    writeData(Squ2_SfxLenCounter, a);
    a = 0x7f; // load contents of reg for both sounds directly
    writeData(SND_SQUARE2_REG + 1, a);
    a = 0x00; // start secondary counter for both sounds
    writeData(Sfx_SecondaryCounter, a);

ContinueGrowItems:
    ++M(Sfx_SecondaryCounter); // increment secondary counter for both sounds
    a = M(Sfx_SecondaryCounter); // this sound doesn't decrement the usual counter
    a >>= 1; // divide by 2 to get the offset
    y = a;
    if (y != M(Squ2_SfxLenCounter))
    { // if so, branch to jump, and stop playing sounds
        a = 0x9d; // load contents of other reg directly
        writeData(SND_SQUARE2_REG, a);
        a = M(PUp_VGrow_FreqData + y); // use secondary counter / 2 as offset for frequency regs
        JSR(SetFreq_Squ2, 576);
        goto Return;

    //------------------------------------------------------------------------
    } // StopGrowItems
    goto EmptySfx2Buffer; // branch to stop playing sounds

PlayBrickShatter:
    a = 0x20; // load length of brick shatter sound
    writeData(Noise_SfxLenCounter, a);

ContinueBrickShatter:
    a = M(Noise_SfxLenCounter);
    a >>= 1; // divide by 2 and check for bit set to use offset
    if ((M(Noise_SfxLenCounter) & 0x01) != 0)
    {
        y = a;
        x = M(BrickShatterFreqData + y); // load reg contents of brick shatter sound
        a = M(BrickShatterEnvData + y);

PlayNoiseSfx:
        writeData(SND_NOISE_REG, a); // play the sfx
        writeData(SND_NOISE_REG + 2, x);
        a = 0x18;
        writeData(SND_NOISE_REG + 3, a);
    } // DecrementSfx3Length
    --M(Noise_SfxLenCounter); // decrement length of sfx
    if (M(Noise_SfxLenCounter) == 0)
    {
        a = 0xf0; // if done, stop playing the sfx
        writeData(SND_NOISE_REG, a);
        a = 0x00;
        writeData(NoiseSoundBuffer, a);
    } // ExSfx3
    goto Return;

//------------------------------------------------------------------------

NoiseSfxHandler:
    y = M(NoiseSoundQueue); // check for sfx in queue
    if (y != 0)
    {
        writeData(NoiseSoundBuffer, y); // if found, put in buffer
        if ((y & Sfx_BrickShatter) != 0)
            goto PlayBrickShatter; // brick shatter
        if ((y & Sfx_BowserFlame) != 0)
            goto PlayBowserFlame; // bowser flame
    } // CheckNoiseBuffer
    a = M(NoiseSoundBuffer); // check for sfx in buffer
    if (a != 0)
    { // if not found, exit sub
        if ((a & Sfx_BrickShatter) != 0)
            goto ContinueBrickShatter; // brick shatter
        if ((a & Sfx_BowserFlame) != 0)
            goto ContinueBowserFlame; // bowser flame
    } // ExNH
    goto Return;

//------------------------------------------------------------------------

PlayBowserFlame:
    a = 0x40; // load length of bowser flame sound
    writeData(Noise_SfxLenCounter, a);

ContinueBowserFlame:
    a = M(Noise_SfxLenCounter);
    a >>= 1;
    y = a;
    x = 0x0f; // load reg contents of bowser flame sound
    a = M(BowserFlameEnvData - 1 + y);
    if (a != 0)
        goto PlayNoiseSfx; // unconditional branch here

ContinueMusic:
    goto HandleSquare2Music; // if we have music, start with square 2 channel

MusicHandler:
    a = M(EventMusicQueue); // check event music queue
    if (a != 0)
        goto LoadEventMusic;
    a = M(AreaMusicQueue); // check area music queue
    if (a == 0)
    {
        a = M(EventMusicBuffer); // check both buffers
        a |= M(AreaMusicBuffer);
        if (a != 0)
            goto ContinueMusic;
        goto Return; // no music, then leave

    //------------------------------------------------------------------------

LoadEventMusic:
        writeData(EventMusicBuffer, a); // copy event music queue contents to buffer
        if (a == DeathMusic)
        { // if not, jump elsewhere
            JSR(StopSquare1Sfx, 577); // stop sfx in square 1 and 2
            JSR(StopSquare2Sfx, 578); // but clear only square 1's sfx buffer
        } // NoStopSfx
        x = M(AreaMusicBuffer);
        writeData(AreaMusicBuffer_Alt, x); // save current area music buffer to be re-obtained later
        y = 0x00;
        writeData(NoteLengthTblAdder, y); // default value for additional length byte offset
        writeData(AreaMusicBuffer, y); // clear area music buffer
        if (a != TimeRunningOutMusic)
            goto FindEventMusicHeader;
        x = 0x08; // load offset to be added to length byte of header
        writeData(NoteLengthTblAdder, x);
        if (x != 0)
            goto FindEventMusicHeader; // unconditional branch
    } // LoadAreaMusic
    if (a == 0x04)
    { // no, do not stop square 1 sfx
        JSR(StopSquare1Sfx, 579);
    } // NoStop1: start counter used only by ground level music
    y = 0x10;

GMLoopB:
    writeData(GroundMusicHeaderOfs, y);

HandleAreaMusicLoopB:
    y = 0x00; // clear event music buffer
    writeData(EventMusicBuffer, y);
    writeData(AreaMusicBuffer, a); // copy area music queue contents to buffer
    if (a == 0x01)
    {
        ++M(GroundMusicHeaderOfs); // increment but only if playing ground level music
        y = M(GroundMusicHeaderOfs); // is it time to loopback ground level music?
        if (y != 0x32)
            goto LoadHeader; // branch ahead with alternate offset
        y = 0x11;
        if (y != 0)
            goto GMLoopB; // unconditional branch
    } // FindAreaMusicHeader
    y = 0x08; // load Y for offset of area music
    writeData(MusicOffset_Square2, y); // residual instruction here

FindEventMusicHeader:
    ++y; // increment Y pointer based on previously loaded queue contents
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // bit shift and increment until we find a set bit for music
    if (!shiftedBit)
        goto FindEventMusicHeader;

LoadHeader:
    a = M(MusicHeaderOffsetData + y); // load offset for header
    y = a;
    a = M(MusicHeaderData + y); // now load the header
    writeData(NoteLenLookupTblOfs, a);
    a = M(MusicHeaderData + 1 + y);
    writeData(MusicDataLow, a);
    a = M(MusicHeaderData + 2 + y);
    writeData(MusicDataHigh, a);
    a = M(MusicHeaderData + 3 + y);
    writeData(MusicOffset_Triangle, a);
    a = M(MusicHeaderData + 4 + y);
    writeData(MusicOffset_Square1, a);
    a = M(MusicHeaderData + 5 + y);
    writeData(MusicOffset_Noise, a);
    writeData(NoiseDataLoopbackOfs, a);
    a = 0x01; // initialize music note counters
    writeData(Squ2_NoteLenCounter, a);
    writeData(Squ1_NoteLenCounter, a);
    writeData(Tri_NoteLenCounter, a);
    writeData(Noise_BeatLenCounter, a);
    a = 0x00; // initialize music data offset for square 2
    writeData(MusicOffset_Square2, a);
    writeData(AltRegContentFlag, a); // initialize alternate control reg data used by square 1
    a = 0x0b; // disable triangle channel and reenable it
    writeData(SND_MASTERCTRL_REG, a);
    a = 0x0f;
    writeData(SND_MASTERCTRL_REG, a);

HandleSquare2Music:
    --M(Squ2_NoteLenCounter); // decrement square 2 note length
    if (M(Squ2_NoteLenCounter) == 0)
    { // is it time for more data?  if not, branch to end tasks
        y = M(MusicOffset_Square2); // increment square 2 music offset and fetch data
        ++M(MusicOffset_Square2);
        a = M(W(MusicData) + y);
        if (a != 0)
        { // if zero, the data is a null terminator
            if ((a & 0x80) == 0)
                goto Squ2NoteHandler; // if non-negative, data is a note
            if (a != 0)
                goto Squ2LengthHandler; // otherwise it is length data
        } // EndOfMusicData
        a = M(EventMusicBuffer); // check secondary buffer for time running out music
        if (a == TimeRunningOutMusic)
        {
            a = M(AreaMusicBuffer_Alt); // load previously saved contents of primary buffer
            if (a != 0)
                goto MusicLoopBack; // and start playing the song again if there is one
        } // NotTRO: check for victory music (the only secondary that loops)
        a &= VictoryMusic;
        if (a == 0)
        {
            a = M(AreaMusicBuffer); // check primary buffer for any music except pipe intro
            a &= 0b01011111;
            if (a != 0)
                goto MusicLoopBack; // if any area music except pipe intro, music loops
            a = 0x00; // clear primary and secondary buffers and initialize
            writeData(AreaMusicBuffer, a); // control regs of square and triangle channels
            writeData(EventMusicBuffer, a);
            writeData(SND_TRIANGLE_REG, a);
            a = 0x90;
            writeData(SND_SQUARE1_REG, a);
            writeData(SND_SQUARE2_REG, a);
            goto Return;

        //------------------------------------------------------------------------

MusicLoopBack:
            goto HandleAreaMusicLoopB;
        } // VictoryMLoopBack
        goto LoadEventMusic;

Squ2LengthHandler:
        JSR(ProcessLengthData, 580); // store length of note
        writeData(Squ2_NoteLenBuffer, a);
        y = M(MusicOffset_Square2); // fetch another byte (MUST NOT BE LENGTH BYTE!)
        ++M(MusicOffset_Square2);
        a = M(W(MusicData) + y);

Squ2NoteHandler:
        x = M(Square2SoundBuffer); // is there a sound playing on this channel?
        if (x == 0)
        {
            JSR(SetFreq_Squ2, 581); // no, then play the note
            if (a != 0)
            { // check to see if note is rest
                JSR(LoadControlRegs, 582); // if not, load control regs for square 2
            } // Rest: save contents of A
            writeData(Squ2_EnvelopeDataCtrl, a);
            JSR(Dump_Sq2_Regs, 583); // dump X and Y into square 2 control regs
        } // SkipFqL1: save length in square 2 note counter
        a = M(Squ2_NoteLenBuffer);
        writeData(Squ2_NoteLenCounter, a);
    } // MiscSqu2MusicTasks
    a = M(Square2SoundBuffer); // is there a sound playing on square 2?
    if (a != 0)
        goto HandleSquare1Music;
    a = M(EventMusicBuffer); // check for death music or d4 set on secondary buffer
    a &= 0b10010001; // note that regs for death music or d4 are loaded by default
    if (a != 0)
        goto HandleSquare1Music;
    y = M(Squ2_EnvelopeDataCtrl); // check for contents saved from LoadControlRegs
    if (y != 0)
    {
        --M(Squ2_EnvelopeDataCtrl); // decrement unless already zero
    } // NoDecEnv1: do a load of envelope data to replace default
    JSR(LoadEnvelopeData, 584);
    writeData(SND_SQUARE2_REG, a); // based on offset set by first load unless playing
    x = 0x7f; // death music or d4 set on secondary buffer
    writeData(SND_SQUARE2_REG + 1, x);

HandleSquare1Music:
    y = M(MusicOffset_Square1); // is there a nonzero offset here?
    if (y == 0)
        goto HandleTriangleMusic; // if not, skip ahead to the triangle channel
    --M(Squ1_NoteLenCounter); // decrement square 1 note length
    if (M(Squ1_NoteLenCounter) == 0)
    { // is it time for more data?

FetchSqu1MusicData:
        y = M(MusicOffset_Square1); // increment square 1 music offset and fetch data
        ++M(MusicOffset_Square1);
        a = M(W(MusicData) + y);
        if (a == 0)
        { // if nonzero, then skip this part
            a = 0x83;
            writeData(SND_SQUARE1_REG, a); // store some data into control regs for square 1
            a = 0x94; // and fetch another byte of data, used to give
            writeData(SND_SQUARE1_REG + 1, a); // death music its unique sound
            writeData(AltRegContentFlag, a);
            if (a != 0)
                goto FetchSqu1MusicData; // unconditional branch
        } // Squ1NoteHandler
        JSR(AlternateLengthHandler, 585);
        writeData(Squ1_NoteLenCounter, a); // save contents of A in square 1 note counter
        y = M(Square1SoundBuffer); // is there a sound playing on square 1?
        if (y != 0)
            goto HandleTriangleMusic;
        a = x;
        a &= 0b00111110; // change saved data to appropriate note format
        JSR(SetFreq_Squ1, 586); // play the note
        if (a != 0)
        {
            JSR(LoadControlRegs, 587);
        } // SkipCtrlL: save envelope offset
        writeData(Squ1_EnvelopeDataCtrl, a);
        JSR(Dump_Squ1_Regs, 588);
    } // MiscSqu1MusicTasks
    a = M(Square1SoundBuffer); // is there a sound playing on square 1?
    if (a != 0)
        goto HandleTriangleMusic;
    a = M(EventMusicBuffer); // check for death music or d4 set on secondary buffer
    a &= 0b10010001;
    if (a == 0)
    {
        y = M(Squ1_EnvelopeDataCtrl); // check saved envelope offset
        if (y != 0)
        {
            --M(Squ1_EnvelopeDataCtrl); // decrement unless already zero
        } // NoDecEnv2: do a load of envelope data
        JSR(LoadEnvelopeData, 589);
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
        goto HandleNoiseMusic; // is it time for more data?
    y = M(MusicOffset_Triangle); // increment square 1 music offset and fetch data
    ++M(MusicOffset_Triangle);
    a = M(W(MusicData) + y);
    if (a == 0)
        goto LoadTriCtrlReg; // if zero, skip all this and move on to noise
    if ((a & 0x80) != 0)
    { // if non-negative, data is note
        JSR(ProcessLengthData, 590); // otherwise, it is length data
        writeData(Tri_NoteLenBuffer, a); // save contents of A
        a = 0x1f;
        writeData(SND_TRIANGLE_REG, a); // load some default data for triangle control reg
        y = M(MusicOffset_Triangle); // fetch another byte
        ++M(MusicOffset_Triangle);
        a = M(W(MusicData) + y);
        if (a == 0)
            goto LoadTriCtrlReg; // check once more for nonzero data
    } // TriNoteHandler
    JSR(SetFreq_Tri, 591);
    x = M(Tri_NoteLenBuffer); // save length in triangle note counter
    writeData(Tri_NoteLenCounter, x);
    a = M(EventMusicBuffer);
    a &= 0b01101110; // check for death music or d4 set on secondary buffer
    if (a == 0)
    { // if playing any other secondary, skip primary buffer check
        a = M(AreaMusicBuffer); // check primary buffer for water or castle level music
        a &= 0b00001010;
        if (a == 0)
            goto HandleNoiseMusic; // if playing any other primary, or death or d4, go on to noise routine
    } // NotDOrD4: if playing water or castle music or any secondary
    a = x;
    if (a < 0x12)
    {
        a = M(EventMusicBuffer); // check for win castle music again if not playing a long note
        a &= EndOfCastleMusic;
        if (a != 0)
        {
            a = 0x0f; // load value $0f if playing the win castle music and playing a short
            if (a != 0)
                goto LoadTriCtrlReg; // note, load value $1f if playing water or castle level music or any
        } // MediN: secondary besides death and d4 except win castle or win castle and playing
        a = 0x1f;
        if (a != 0)
            goto LoadTriCtrlReg; // a short note, and load value $ff if playing a long note on water, castle
    } // LongN: or any secondary (including win castle) except death and d4
    a = 0xff;

LoadTriCtrlReg:
    writeData(SND_TRIANGLE_REG, a); // save final contents of A into control reg for triangle

HandleNoiseMusic:
    a = M(AreaMusicBuffer); // check if playing underground or castle music
    a &= 0b11110011;
    if (a == 0)
        goto ExitMusicHandler; // if so, skip the noise routine
    --M(Noise_BeatLenCounter); // decrement noise beat length
    if (M(Noise_BeatLenCounter) != 0)
        goto ExitMusicHandler; // is it time for more data?

FetchNoiseBeatData:
    y = M(MusicOffset_Noise); // increment noise beat offset and fetch data
    ++M(MusicOffset_Noise);
    a = M(W(MusicData) + y); // get noise beat data, if nonzero, branch to handle
    if (a == 0)
    {
        a = M(NoiseDataLoopbackOfs); // if data is zero, reload original noise beat offset
        writeData(MusicOffset_Noise, a); // and loopback next time around
        if (a != 0)
            goto FetchNoiseBeatData; // unconditional branch
    } // NoiseBeatHandler
    JSR(AlternateLengthHandler, 592);
    writeData(Noise_BeatLenCounter, a); // store length in noise beat counter
    a = x;
    a &= 0b00111110; // reload data and erase length bits
    if (a == 0)
        goto SilentBeat; // if no beat data, silence
    if (a != 0x30)
    { // noise accordingly
        if (a != 0x20)
        {
            a &= 0b00010000;
            if (a == 0)
                goto SilentBeat;
            a = 0x1c; // short beat data
            x = 0x03;
            y = 0x18;
            if (y != 0)
                goto PlayBeat;
        } // StrongBeat
        a = 0x1c; // strong beat data
        x = 0x0c;
        y = 0x18;
        if (y != 0)
            goto PlayBeat;
    } // LongBeat
    a = 0x1c; // long beat data
    x = 0x03;
    y = 0x58;
    if (y != 0)
        goto PlayBeat;

SilentBeat:
    a = 0x10; // silence

PlayBeat:
    writeData(SND_NOISE_REG, a); // load beat data into noise regs
    writeData(SND_NOISE_REG + 2, x);
    writeData(SND_NOISE_REG + 3, y);

ExitMusicHandler:
    goto Return;

//------------------------------------------------------------------------

AlternateLengthHandler:
    x = a; // save a copy of original byte into X
    // turn xx00000x into 00000xxx, with d0 of the original as the MSB here
    a = (uint8_t)(((a & 0x01) << 2) | ((a >> 6) & 0x03));

ProcessLengthData:
    a &= 0b00000111; // clear all but the three LSBs
    a += M(0xf0); // add offset loaded from first header byte
    a += M(NoteLengthTblAdder); // add extra if time running out music
    y = a;
    a = M(MusicLengthLookupTbl + y); // load length
    goto Return;

//------------------------------------------------------------------------

LoadControlRegs:
    a = M(EventMusicBuffer); // check secondary buffer for win castle music
    a &= EndOfCastleMusic;
    if (a != 0)
    {
        a = 0x04; // this value is only used for win castle music
        if (a != 0)
            goto AllMus; // unconditional branch
    } // NotECstlM
    a = M(AreaMusicBuffer);
    a &= 0b01111101; // check primary buffer for water music
    if (a != 0)
    {
        a = 0x08; // this is the default value for all other music
        if (a != 0)
            goto AllMus;
    } // WaterMus: this value is used for water music and all other event music
    a = 0x28;

AllMus: // load contents of other sound regs for square 2
    x = 0x82;
    y = 0x7f;
    goto Return;

//------------------------------------------------------------------------

LoadEnvelopeData:
    a = M(EventMusicBuffer); // check secondary buffer for win castle music
    a &= EndOfCastleMusic;
    if (a != 0)
    {
        a = M(EndOfCastleMusicEnvData + y); // load data from offset for win castle music
        goto Return;

    //------------------------------------------------------------------------
    } // LoadUsualEnvData
    a = M(AreaMusicBuffer); // check primary buffer for water music
    a &= 0b01111101;
    if (a != 0)
    {
        a = M(AreaMusicEnvData + y); // load default data from offset for all other music
        goto Return;

    //------------------------------------------------------------------------
    } // LoadWaterEventMusEnvData
    a = M(WaterEventMusEnvData + y); // load data from offset for water music and all other event music
    goto Return;

//------------------------------------------------------------------------
// Return handler
// This emulates the RTS instruction using a generated jump table
//
Return:
    switch (popReturnIndex())
    {
    case 0:
        goto Return_0;
    case 1:
        goto Return_1;
    case 2:
        goto Return_2;
    case 3:
        goto Return_3;
    case 4:
        goto Return_4;
    case 5:
        goto Return_5;
    case 6:
        goto Return_6;
    case 7:
        goto Return_7;
    case 8:
        goto Return_8;
    case 9:
        goto Return_9;
    case 10:
        goto Return_10;
    case 11:
        goto Return_11;
    case 12:
        goto Return_12;
    case 13:
        goto Return_13;
    case 14:
        goto Return_14;
    case 15:
        goto Return_15;
    case 16:
        goto Return_16;
    case 17:
        goto Return_17;
    case 18:
        goto Return_18;
    case 19:
        goto Return_19;
    case 20:
        goto Return_20;
    case 21:
        goto Return_21;
    case 22:
        goto Return_22;
    case 23:
        goto Return_23;
    case 24:
        goto Return_24;
    case 25:
        goto Return_25;
    case 26:
        goto Return_26;
    case 27:
        goto Return_27;
    case 28:
        goto Return_28;
    case 29:
        goto Return_29;
    case 30:
        goto Return_30;
    case 31:
        goto Return_31;
    case 32:
        goto Return_32;
    case 33:
        goto Return_33;
    case 34:
        goto Return_34;
    case 35:
        goto Return_35;
    case 36:
        goto Return_36;
    case 37:
        goto Return_37;
    case 38:
        goto Return_38;
    case 39:
        goto Return_39;
    case 40:
        goto Return_40;
    case 41:
        goto Return_41;
    case 42:
        goto Return_42;
    case 43:
        goto Return_43;
    case 44:
        goto Return_44;
    case 45:
        goto Return_45;
    case 46:
        goto Return_46;
    case 47:
        goto Return_47;
    case 48:
        goto Return_48;
    case 49:
        goto Return_49;
    case 50:
        goto Return_50;
    case 51:
        goto Return_51;
    case 52:
        goto Return_52;
    case 53:
        goto Return_53;
    case 54:
        goto Return_54;
    case 55:
        goto Return_55;
    case 56:
        goto Return_56;
    case 57:
        goto Return_57;
    case 58:
        goto Return_58;
    case 59:
        goto Return_59;
    case 60:
        goto Return_60;
    case 61:
        goto Return_61;
    case 62:
        goto Return_62;
    case 63:
        goto Return_63;
    case 64:
        goto Return_64;
    case 65:
        goto Return_65;
    case 66:
        goto Return_66;
    case 67:
        goto Return_67;
    case 68:
        goto Return_68;
    case 69:
        goto Return_69;
    case 70:
        goto Return_70;
    case 71:
        goto Return_71;
    case 72:
        goto Return_72;
    case 73:
        goto Return_73;
    case 74:
        goto Return_74;
    case 75:
        goto Return_75;
    case 76:
        goto Return_76;
    case 77:
        goto Return_77;
    case 78:
        goto Return_78;
    case 79:
        goto Return_79;
    case 80:
        goto Return_80;
    case 81:
        goto Return_81;
    case 82:
        goto Return_82;
    case 83:
        goto Return_83;
    case 84:
        goto Return_84;
    case 85:
        goto Return_85;
    case 86:
        goto Return_86;
    case 87:
        goto Return_87;
    case 88:
        goto Return_88;
    case 89:
        goto Return_89;
    case 90:
        goto Return_90;
    case 91:
        goto Return_91;
    case 92:
        goto Return_92;
    case 93:
        goto Return_93;
    case 94:
        goto Return_94;
    case 95:
        goto Return_95;
    case 96:
        goto Return_96;
    case 97:
        goto Return_97;
    case 98:
        goto Return_98;
    case 99:
        goto Return_99;
    case 100:
        goto Return_100;
    case 101:
        goto Return_101;
    case 102:
        goto Return_102;
    case 103:
        goto Return_103;
    case 104:
        goto Return_104;
    case 105:
        goto Return_105;
    case 106:
        goto Return_106;
    case 107:
        goto Return_107;
    case 108:
        goto Return_108;
    case 109:
        goto Return_109;
    case 110:
        goto Return_110;
    case 111:
        goto Return_111;
    case 112:
        goto Return_112;
    case 113:
        goto Return_113;
    case 114:
        goto Return_114;
    case 115:
        goto Return_115;
    case 116:
        goto Return_116;
    case 117:
        goto Return_117;
    case 118:
        goto Return_118;
    case 119:
        goto Return_119;
    case 120:
        goto Return_120;
    case 121:
        goto Return_121;
    case 122:
        goto Return_122;
    case 123:
        goto Return_123;
    case 124:
        goto Return_124;
    case 125:
        goto Return_125;
    case 126:
        goto Return_126;
    case 127:
        goto Return_127;
    case 128:
        goto Return_128;
    case 129:
        goto Return_129;
    case 130:
        goto Return_130;
    case 131:
        goto Return_131;
    case 132:
        goto Return_132;
    case 133:
        goto Return_133;
    case 134:
        goto Return_134;
    case 135:
        goto Return_135;
    case 136:
        goto Return_136;
    case 137:
        goto Return_137;
    case 138:
        goto Return_138;
    case 139:
        goto Return_139;
    case 140:
        goto Return_140;
    case 141:
        goto Return_141;
    case 142:
        goto Return_142;
    case 143:
        goto Return_143;
    case 144:
        goto Return_144;
    case 145:
        goto Return_145;
    case 146:
        goto Return_146;
    case 147:
        goto Return_147;
    case 148:
        goto Return_148;
    case 149:
        goto Return_149;
    case 150:
        goto Return_150;
    case 151:
        goto Return_151;
    case 152:
        goto Return_152;
    case 153:
        goto Return_153;
    case 154:
        goto Return_154;
    case 155:
        goto Return_155;
    case 156:
        goto Return_156;
    case 157:
        goto Return_157;
    case 158:
        goto Return_158;
    case 159:
        goto Return_159;
    case 160:
        goto Return_160;
    case 161:
        goto Return_161;
    case 162:
        goto Return_162;
    case 163:
        goto Return_163;
    case 164:
        goto Return_164;
    case 165:
        goto Return_165;
    case 166:
        goto Return_166;
    case 167:
        goto Return_167;
    case 168:
        goto Return_168;
    case 169:
        goto Return_169;
    case 170:
        goto Return_170;
    case 171:
        goto Return_171;
    case 172:
        goto Return_172;
    case 173:
        goto Return_173;
    case 174:
        goto Return_174;
    case 175:
        goto Return_175;
    case 176:
        goto Return_176;
    case 177:
        goto Return_177;
    case 178:
        goto Return_178;
    case 179:
        goto Return_179;
    case 180:
        goto Return_180;
    case 181:
        goto Return_181;
    case 182:
        goto Return_182;
    case 183:
        goto Return_183;
    case 184:
        goto Return_184;
    case 185:
        goto Return_185;
    case 186:
        goto Return_186;
    case 187:
        goto Return_187;
    case 188:
        goto Return_188;
    case 189:
        goto Return_189;
    case 190:
        goto Return_190;
    case 191:
        goto Return_191;
    case 192:
        goto Return_192;
    case 193:
        goto Return_193;
    case 194:
        goto Return_194;
    case 195:
        goto Return_195;
    case 196:
        goto Return_196;
    case 197:
        goto Return_197;
    case 198:
        goto Return_198;
    case 199:
        goto Return_199;
    case 200:
        goto Return_200;
    case 201:
        goto Return_201;
    case 202:
        goto Return_202;
    case 203:
        goto Return_203;
    case 204:
        goto Return_204;
    case 205:
        goto Return_205;
    case 206:
        goto Return_206;
    case 207:
        goto Return_207;
    case 208:
        goto Return_208;
    case 209:
        goto Return_209;
    case 210:
        goto Return_210;
    case 211:
        goto Return_211;
    case 212:
        goto Return_212;
    case 213:
        goto Return_213;
    case 214:
        goto Return_214;
    case 215:
        goto Return_215;
    case 216:
        goto Return_216;
    case 217:
        goto Return_217;
    case 218:
        goto Return_218;
    case 219:
        goto Return_219;
    case 220:
        goto Return_220;
    case 221:
        goto Return_221;
    case 222:
        goto Return_222;
    case 223:
        goto Return_223;
    case 224:
        goto Return_224;
    case 225:
        goto Return_225;
    case 226:
        goto Return_226;
    case 227:
        goto Return_227;
    case 228:
        goto Return_228;
    case 229:
        goto Return_229;
    case 230:
        goto Return_230;
    case 231:
        goto Return_231;
    case 232:
        goto Return_232;
    case 233:
        goto Return_233;
    case 234:
        goto Return_234;
    case 235:
        goto Return_235;
    case 236:
        goto Return_236;
    case 237:
        goto Return_237;
    case 238:
        goto Return_238;
    case 239:
        goto Return_239;
    case 240:
        goto Return_240;
    case 241:
        goto Return_241;
    case 242:
        goto Return_242;
    case 243:
        goto Return_243;
    case 244:
        goto Return_244;
    case 245:
        goto Return_245;
    case 246:
        goto Return_246;
    case 247:
        goto Return_247;
    case 248:
        goto Return_248;
    case 249:
        goto Return_249;
    case 250:
        goto Return_250;
    case 251:
        goto Return_251;
    case 252:
        goto Return_252;
    case 253:
        goto Return_253;
    case 254:
        goto Return_254;
    case 255:
        goto Return_255;
    case 256:
        goto Return_256;
    case 257:
        goto Return_257;
    case 258:
        goto Return_258;
    case 259:
        goto Return_259;
    case 260:
        goto Return_260;
    case 261:
        goto Return_261;
    case 262:
        goto Return_262;
    case 263:
        goto Return_263;
    case 264:
        goto Return_264;
    case 265:
        goto Return_265;
    case 266:
        goto Return_266;
    case 267:
        goto Return_267;
    case 268:
        goto Return_268;
    case 269:
        goto Return_269;
    case 270:
        goto Return_270;
    case 271:
        goto Return_271;
    case 272:
        goto Return_272;
    case 273:
        goto Return_273;
    case 274:
        goto Return_274;
    case 275:
        goto Return_275;
    case 276:
        goto Return_276;
    case 277:
        goto Return_277;
    case 278:
        goto Return_278;
    case 279:
        goto Return_279;
    case 280:
        goto Return_280;
    case 281:
        goto Return_281;
    case 282:
        goto Return_282;
    case 283:
        goto Return_283;
    case 284:
        goto Return_284;
    case 285:
        goto Return_285;
    case 286:
        goto Return_286;
    case 287:
        goto Return_287;
    case 288:
        goto Return_288;
    case 289:
        goto Return_289;
    case 290:
        goto Return_290;
    case 291:
        goto Return_291;
    case 292:
        goto Return_292;
    case 293:
        goto Return_293;
    case 294:
        goto Return_294;
    case 295:
        goto Return_295;
    case 296:
        goto Return_296;
    case 297:
        goto Return_297;
    case 298:
        goto Return_298;
    case 299:
        goto Return_299;
    case 300:
        goto Return_300;
    case 301:
        goto Return_301;
    case 302:
        goto Return_302;
    case 303:
        goto Return_303;
    case 304:
        goto Return_304;
    case 305:
        goto Return_305;
    case 306:
        goto Return_306;
    case 307:
        goto Return_307;
    case 308:
        goto Return_308;
    case 309:
        goto Return_309;
    case 310:
        goto Return_310;
    case 311:
        goto Return_311;
    case 312:
        goto Return_312;
    case 313:
        goto Return_313;
    case 314:
        goto Return_314;
    case 315:
        goto Return_315;
    case 316:
        goto Return_316;
    case 317:
        goto Return_317;
    case 318:
        goto Return_318;
    case 319:
        goto Return_319;
    case 320:
        goto Return_320;
    case 321:
        goto Return_321;
    case 322:
        goto Return_322;
    case 323:
        goto Return_323;
    case 324:
        goto Return_324;
    case 325:
        goto Return_325;
    case 326:
        goto Return_326;
    case 327:
        goto Return_327;
    case 328:
        goto Return_328;
    case 329:
        goto Return_329;
    case 330:
        goto Return_330;
    case 331:
        goto Return_331;
    case 332:
        goto Return_332;
    case 333:
        goto Return_333;
    case 334:
        goto Return_334;
    case 335:
        goto Return_335;
    case 336:
        goto Return_336;
    case 337:
        goto Return_337;
    case 338:
        goto Return_338;
    case 339:
        goto Return_339;
    case 340:
        goto Return_340;
    case 341:
        goto Return_341;
    case 342:
        goto Return_342;
    case 343:
        goto Return_343;
    case 344:
        goto Return_344;
    case 345:
        goto Return_345;
    case 346:
        goto Return_346;
    case 347:
        goto Return_347;
    case 348:
        goto Return_348;
    case 349:
        goto Return_349;
    case 350:
        goto Return_350;
    case 351:
        goto Return_351;
    case 352:
        goto Return_352;
    case 353:
        goto Return_353;
    case 354:
        goto Return_354;
    case 355:
        goto Return_355;
    case 356:
        goto Return_356;
    case 357:
        goto Return_357;
    case 358:
        goto Return_358;
    case 359:
        goto Return_359;
    case 360:
        goto Return_360;
    case 361:
        goto Return_361;
    case 362:
        goto Return_362;
    case 363:
        goto Return_363;
    case 364:
        goto Return_364;
    case 365:
        goto Return_365;
    case 366:
        goto Return_366;
    case 367:
        goto Return_367;
    case 368:
        goto Return_368;
    case 369:
        goto Return_369;
    case 370:
        goto Return_370;
    case 371:
        goto Return_371;
    case 372:
        goto Return_372;
    case 373:
        goto Return_373;
    case 374:
        goto Return_374;
    case 375:
        goto Return_375;
    case 376:
        goto Return_376;
    case 377:
        goto Return_377;
    case 378:
        goto Return_378;
    case 379:
        goto Return_379;
    case 380:
        goto Return_380;
    case 381:
        goto Return_381;
    case 382:
        goto Return_382;
    case 383:
        goto Return_383;
    case 384:
        goto Return_384;
    case 385:
        goto Return_385;
    case 386:
        goto Return_386;
    case 387:
        goto Return_387;
    case 388:
        goto Return_388;
    case 389:
        goto Return_389;
    case 390:
        goto Return_390;
    case 391:
        goto Return_391;
    case 392:
        goto Return_392;
    case 393:
        goto Return_393;
    case 394:
        goto Return_394;
    case 395:
        goto Return_395;
    case 396:
        goto Return_396;
    case 397:
        goto Return_397;
    case 398:
        goto Return_398;
    case 399:
        goto Return_399;
    case 400:
        goto Return_400;
    case 401:
        goto Return_401;
    case 402:
        goto Return_402;
    case 403:
        goto Return_403;
    case 404:
        goto Return_404;
    case 405:
        goto Return_405;
    case 406:
        goto Return_406;
    case 407:
        goto Return_407;
    case 408:
        goto Return_408;
    case 409:
        goto Return_409;
    case 410:
        goto Return_410;
    case 411:
        goto Return_411;
    case 412:
        goto Return_412;
    case 413:
        goto Return_413;
    case 414:
        goto Return_414;
    case 415:
        goto Return_415;
    case 416:
        goto Return_416;
    case 417:
        goto Return_417;
    case 418:
        goto Return_418;
    case 419:
        goto Return_419;
    case 420:
        goto Return_420;
    case 421:
        goto Return_421;
    case 422:
        goto Return_422;
    case 423:
        goto Return_423;
    case 424:
        goto Return_424;
    case 425:
        goto Return_425;
    case 426:
        goto Return_426;
    case 427:
        goto Return_427;
    case 428:
        goto Return_428;
    case 429:
        goto Return_429;
    case 430:
        goto Return_430;
    case 431:
        goto Return_431;
    case 432:
        goto Return_432;
    case 433:
        goto Return_433;
    case 434:
        goto Return_434;
    case 435:
        goto Return_435;
    case 436:
        goto Return_436;
    case 437:
        goto Return_437;
    case 438:
        goto Return_438;
    case 439:
        goto Return_439;
    case 440:
        goto Return_440;
    case 441:
        goto Return_441;
    case 442:
        goto Return_442;
    case 443:
        goto Return_443;
    case 444:
        goto Return_444;
    case 445:
        goto Return_445;
    case 446:
        goto Return_446;
    case 447:
        goto Return_447;
    case 448:
        goto Return_448;
    case 449:
        goto Return_449;
    case 450:
        goto Return_450;
    case 451:
        goto Return_451;
    case 452:
        goto Return_452;
    case 453:
        goto Return_453;
    case 454:
        goto Return_454;
    case 455:
        goto Return_455;
    case 456:
        goto Return_456;
    case 457:
        goto Return_457;
    case 458:
        goto Return_458;
    case 459:
        goto Return_459;
    case 460:
        goto Return_460;
    case 461:
        goto Return_461;
    case 462:
        goto Return_462;
    case 463:
        goto Return_463;
    case 464:
        goto Return_464;
    case 465:
        goto Return_465;
    case 466:
        goto Return_466;
    case 467:
        goto Return_467;
    case 468:
        goto Return_468;
    case 469:
        goto Return_469;
    case 470:
        goto Return_470;
    case 471:
        goto Return_471;
    case 472:
        goto Return_472;
    case 473:
        goto Return_473;
    case 474:
        goto Return_474;
    case 475:
        goto Return_475;
    case 476:
        goto Return_476;
    case 477:
        goto Return_477;
    case 478:
        goto Return_478;
    case 479:
        goto Return_479;
    case 480:
        goto Return_480;
    case 481:
        goto Return_481;
    case 482:
        goto Return_482;
    case 483:
        goto Return_483;
    case 484:
        goto Return_484;
    case 485:
        goto Return_485;
    case 486:
        goto Return_486;
    case 487:
        goto Return_487;
    case 488:
        goto Return_488;
    case 489:
        goto Return_489;
    case 490:
        goto Return_490;
    case 491:
        goto Return_491;
    case 492:
        goto Return_492;
    case 493:
        goto Return_493;
    case 494:
        goto Return_494;
    case 495:
        goto Return_495;
    case 496:
        goto Return_496;
    case 497:
        goto Return_497;
    case 498:
        goto Return_498;
    case 499:
        goto Return_499;
    case 500:
        goto Return_500;
    case 501:
        goto Return_501;
    case 502:
        goto Return_502;
    case 503:
        goto Return_503;
    case 504:
        goto Return_504;
    case 505:
        goto Return_505;
    case 506:
        goto Return_506;
    case 507:
        goto Return_507;
    case 508:
        goto Return_508;
    case 509:
        goto Return_509;
    case 510:
        goto Return_510;
    case 511:
        goto Return_511;
    case 512:
        goto Return_512;
    case 513:
        goto Return_513;
    case 514:
        goto Return_514;
    case 515:
        goto Return_515;
    case 516:
        goto Return_516;
    case 517:
        goto Return_517;
    case 518:
        goto Return_518;
    case 519:
        goto Return_519;
    case 520:
        goto Return_520;
    case 521:
        goto Return_521;
    case 522:
        goto Return_522;
    case 523:
        goto Return_523;
    case 524:
        goto Return_524;
    case 525:
        goto Return_525;
    case 526:
        goto Return_526;
    case 527:
        goto Return_527;
    case 528:
        goto Return_528;
    case 529:
        goto Return_529;
    case 530:
        goto Return_530;
    case 531:
        goto Return_531;
    case 532:
        goto Return_532;
    case 533:
        goto Return_533;
    case 534:
        goto Return_534;
    case 535:
        goto Return_535;
    case 536:
        goto Return_536;
    case 537:
        goto Return_537;
    case 538:
        goto Return_538;
    case 539:
        goto Return_539;
    case 540:
        goto Return_540;
    case 541:
        goto Return_541;
    case 542:
        goto Return_542;
    case 543:
        goto Return_543;
    case 544:
        goto Return_544;
    case 545:
        goto Return_545;
    case 546:
        goto Return_546;
    case 547:
        goto Return_547;
    case 548:
        goto Return_548;
    case 549:
        goto Return_549;
    case 550:
        goto Return_550;
    case 551:
        goto Return_551;
    case 552:
        goto Return_552;
    case 553:
        goto Return_553;
    case 554:
        goto Return_554;
    case 555:
        goto Return_555;
    case 556:
        goto Return_556;
    case 557:
        goto Return_557;
    case 558:
        goto Return_558;
    case 559:
        goto Return_559;
    case 560:
        goto Return_560;
    case 561:
        goto Return_561;
    case 562:
        goto Return_562;
    case 563:
        goto Return_563;
    case 564:
        goto Return_564;
    case 565:
        goto Return_565;
    case 566:
        goto Return_566;
    case 567:
        goto Return_567;
    case 568:
        goto Return_568;
    case 569:
        goto Return_569;
    case 570:
        goto Return_570;
    case 571:
        goto Return_571;
    case 572:
        goto Return_572;
    case 573:
        goto Return_573;
    case 574:
        goto Return_574;
    case 575:
        goto Return_575;
    case 576:
        goto Return_576;
    case 577:
        goto Return_577;
    case 578:
        goto Return_578;
    case 579:
        goto Return_579;
    case 580:
        goto Return_580;
    case 581:
        goto Return_581;
    case 582:
        goto Return_582;
    case 583:
        goto Return_583;
    case 584:
        goto Return_584;
    case 585:
        goto Return_585;
    case 586:
        goto Return_586;
    case 587:
        goto Return_587;
    case 588:
        goto Return_588;
    case 589:
        goto Return_589;
    case 590:
        goto Return_590;
    case 591:
        goto Return_591;
    case 592:
        goto Return_592;
    }
}
