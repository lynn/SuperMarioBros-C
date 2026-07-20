// The PlayerCtrlRoutine subsystem: everything PlayerCtrlRoutine() reaches that nothing
// outside it reaches, and so nothing outside it needs.
//
// Moved out of SMB.cpp by tools/split.py. These are methods of SMBEngine like every other
// routine of the game and are declared in SMBEngine.hpp; the file they are written in is the
// only thing that is different, and tools/layers.py is what keeps it meaning something.
//
#include "SMB.hpp"

#include <tuple>

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerPhysicsSub()
{
    const uint8_t Climb_Y_MForceData_data[] = {0x00, 0x20, 0xff};

    const uint8_t Climb_Y_SpeedData_data[] = {0x00, 0xff, 0x01};

    const uint8_t FrictionData_data[] = {0xe4, 0x98, 0xd0};

    const uint8_t MaxRightXSpdData_data[] = {
        0x28, 0x18, 0x10,
        0x0c // used for pipe intros
    };

    const uint8_t MaxLeftXSpdData_data[] = {0xd8, 0xe8, 0xf0};

    const uint8_t InitMForceData_data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00};

    const uint8_t PlayerYSpdData_data[] = {0xfc, 0xfc, 0xfc, 0xfb, 0xfb, 0xfe, 0xff};

    const uint8_t FallMForceData_data[] = {0x70, 0x70, 0x60, 0x90, 0x90, 0x0a, 0x09};

    const uint8_t JumpMForceData_data[] = {0x20, 0x20, 0x1e, 0x28, 0x28, 0x0d, 0x04};

    // check player state
    if (M(Player_State) == 0x03)
    { // if climbing, select offset by the controller bits for up/down
        // checked against player's collision detection bits
        const uint8_t upDown = M(Up_Down_Buttons) & M(Player_CollisionBits);
        uint8_t climbOfs = 0x00; // not pressing up or down
        if (upDown != 0)
        {
            climbOfs = (upDown & 0b00001000) != 0 ? 0x01 : 0x02; // pressing up : pressing down
        }
        // ProcClimb: load value here and store as vertical movement force
        writeData(Player_Y_MoveForce, Climb_Y_MForceData_data[climbOfs]);
        const uint8_t climbSpeed = Climb_Y_SpeedData_data[climbOfs]; // load some other value here
        writeData(Player_Y_Speed, climbSpeed);                       // store as vertical speed
        // if climbing down, divide default animation timing by 2
        // SetCAnim: store animation timer setting and leave
        writeData(PlayerAnimTimerSet, (climbSpeed & 0x80) == 0 ? 0x04 : 0x08);
        return;

        //------------------------------------------------------------------------
    } // CheckForJumping
    // unless a jumpspring is animating, check for A button press not held from the
    // previous frame
    const bool jumpPressed = M(JumpspringAnimCtrl) == 0 && (M(A_B_Buttons) & A_Button) != 0 &&
                             (M(A_B_Buttons) & A_Button & M(PreviousA_B_Buttons)) == 0;
    // ProcJumping: jump if on the ground; if swimming, also swim upwards unless the
    // jump/swim timer expired while the player is still rising (this prevents midair
    // jumping and swimming above water level)
    if (jumpPressed && (M(Player_State) == 0 ||
                        (M(SwimmingFlag) != 0 && (M(JumpSwimTimer) != 0 || (M(Player_Y_Speed) & 0x80) == 0))))
    {
        // InitJS: set jump/swim timer
        writeData(JumpSwimTimer, 0x20);
        // initialize vertical force and dummy variable
        writeData(Player_YMF_Dummy, 0x00);
        writeData(Player_Y_MoveForce, 0x00);
        // get vertical high and low bytes of jump origin
        writeData(JumpOrigin_Y_HighPos, M(Player_Y_HighPos)); // and store them next to each other here
        writeData(JumpOrigin_Y_Position, M(Player_Y_Position));
        // set player state to jumping/swimming
        writeData(Player_State, 0x01);
        // check value related to walking/running speed, incrementing the offset for each
        // amount equaled or exceeded (note that for jumping, range is 0-4)
        const uint8_t xSpeedAbs = M(Player_XSpeedAbsolute);
        uint8_t physicsOfs = 0x00;
        if (xSpeedAbs >= 0x09)
        {
            physicsOfs = 0x01;
        }
        if (xSpeedAbs >= 0x10)
        {
            physicsOfs = 0x02;
        }
        if (xSpeedAbs >= 0x19)
        {
            physicsOfs = 0x03;
        }
        if (xSpeedAbs >= 0x1c)
        {
            physicsOfs = 0x04;
        }
        // ChkWtr: set value here (apparently always set to 1)
        writeData(DiffToHaltJump, 0x01);
        if (M(SwimmingFlag) != 0)
        { // if swimming flag enabled,
            physicsOfs = 0x05; // otherwise set offset to 5, range is 5-6
            if (M(Whirlpool_Flag) != 0)
            {
                physicsOfs = 0x06; // if whirlpool flag set, increment to 6
            }
        }
        // GetYPhy: store appropriate jump/swim data here
        writeData(VerticalForce, JumpMForceData_data[physicsOfs]);
        writeData(VerticalForceDown, FallMForceData_data[physicsOfs]);
        writeData(Player_Y_MoveForce, InitMForceData_data[physicsOfs]);
        writeData(Player_Y_Speed, PlayerYSpdData_data[physicsOfs]);
        if (M(SwimmingFlag) != 0)
        { // if swimming flag enabled,
            // load swim/goomba stomp sound into square 1's sfx queue
            writeData(Square1SoundQueue, Sfx_EnemyStomp);
            if (M(Player_Y_Position) < 0x14)
            { // if above a certain point, reset player's vertical speed
                // to keep player from swimming above water level
                writeData(Player_Y_Speed, 0x00);
            }
        }
        else
        { // PJumpSnd/SJumpSnd: store appropriate jump sound in square 1 sfx queue,
            // big mario's jump sound by default, small mario's if mario is not big
            writeData(Square1SoundQueue, M(PlayerSize) != 0 ? Sfx_SmallJump : Sfx_BigJump);
        }
    }

    // X_Physics
    uint8_t frictionOfs = 0x00;
    writeData(0x00, 0x00); // init value here
    bool slowFriction = false; // whether to take the ChkRFast path below
    if (M(Player_State) != 0)
    { // if mario is not on the ground, check something that seems to be related
        if (M(Player_XSpeedAbsolute) < 0x19)
        {
            slowFriction = true; // if <$19 branch elsewhere
        }
    }
    else
    { // ProcPRun: if mario on the ground, check area type
        if (M(AreaType) == 0)
        { // if water type, branch with incremented offset
            frictionOfs = 0x01;
            slowFriction = true;
        }
        // get left/right controller bits
        else if (M(Left_Right_Buttons) != M(Player_MovingDir))
        {
            slowFriction = true; // if controller bits <> moving direction, skip this part
        }
        // check for b button pressed
        else if ((M(A_B_Buttons) & B_Button) != 0)
        { // SetRTmr: if b button pressed, set running timer
            writeData(RunningTimer, 0x0a);
        }
        else if (M(RunningTimer) == 0)
        { // if running timer not set, branch
            slowFriction = true;
        }
    }
    if (slowFriction)
    { // ChkRFast: if running timer not set or level type is water,
        // increment offset again and temp variable in memory
        ++frictionOfs;
        ++M(0x00);
        // FastXSp: if running speed set or speed => $21 increment $00
        if (M(RunningSpeed) != 0 || M(Player_XSpeedAbsolute) >= 0x21)
        {
            ++M(0x00);
        }
    }
    // GetXPhy: get maximum speed to the left
    writeData(MaximumLeftSpeed, MaxLeftXSpdData_data[frictionOfs]);
    // check for specific routine running
    if (M(GameEngineSubroutine) == 0x07)
    { // if running, set offset to 3 (used for pipe intros)
        frictionOfs = 0x03;
    } // GetXPhy2: get maximum speed to the right
    writeData(MaximumRightSpeed, MaxRightXSpdData_data[frictionOfs]);
    // get value using the temp variable in memory as offset
    writeData(FrictionAdderLow, FrictionData_data[M(0x00)]);
    writeData(FrictionAdderHigh, 0x00); // init something here
    if (M(PlayerFacingDir) != M(Player_MovingDir))
    { // if facing and moving direction differ, double the 16-bit friction adder
        const uint32_t wide = ((M(FrictionAdderHigh) << 8) | M(FrictionAdderLow)) << 1;
        writeData(FrictionAdderLow, LOBYTE(wide));
        writeData(FrictionAdderHigh, HIBYTE(wide));
    } // ExitPhy: and then leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::GetPlayerAnimSpeed()
{
    const uint8_t PlayerAnimTmrData_data[] = {0x02, 0x04, 0x07};

    uint8_t timerIdx = 0x00;                           // initialize offset
    const uint8_t absSpeed = M(Player_XSpeedAbsolute); // check player's walking/running speed
    if (absSpeed >= 0x1c)
    { // if greater than a certain amount, SetRunSpd: store running speed here
        writeData(RunningSpeed, absSpeed);
    }
    else
    {
        timerIdx = 0x01; // otherwise increment offset
        if (absSpeed < 0x0e)
        {                    // if greater than this but not greater than first, skip increment
            timerIdx = 0x02; // otherwise increment offset again
        } // ChkSkid: get controller bits
        const uint8_t buttons = M(SavedJoypadBits) & 0b01111111; // mask out A button
        if (buttons != 0)
        { // if no other buttons pressed, branch ahead of all this
            // mask out all others except left and right
            if ((buttons & 0x03) == M(Player_MovingDir))
            { // if left/right controller bits == moving direction,
                writeData(RunningSpeed, 0x00); // SetRunSpd: store zero value here
            } // ProcSkid: check player's walking/running speed
            else if (M(Player_XSpeedAbsolute) < 0x0b)
            { // if greater than this amount, branch
                writeData(Player_MovingDir, M(PlayerFacingDir)); // otherwise use facing direction to set moving direction
                writeData(Player_X_Speed, 0x00);     // nullify player's horizontal speed
                writeData(Player_X_MoveForce, 0x00); // and dummy variable for player
            }
        }
    }
    // SetAnimSpd: get animation timer setting using the offset
    writeData(PlayerAnimTimerSet, PlayerAnimTmrData_data[timerIdx]);
}

//------------------------------------------------------------------------

// Inputs: leftRightButtons = left/right controller bits (Left_Right_Buttons)
// Outputs: none beyond the Player_XSpeedAbsolute memory write (final a is scratch to the caller)
void SMBEngine::ImposeFriction(uint8_t leftRightButtons)
{
    // perform AND between left/right controller bits and collision flag
    const uint8_t joypadCollision = leftRightButtons & M(Player_CollisionBits);
    bool slowLeftMovement = false; // i.e. take the LeftFrict path
    if (joypadCollision == 0x00)
    { // if any bits set, branch to next part
        const uint8_t speed = M(Player_X_Speed);
        if (speed == 0)
        {                                           // if player has no horizontal speed,
            writeData(Player_XSpeedAbsolute, speed); // SetAbsSpd: store walking/running speed here and leave
            return;
        }
        // if player moving to the right, slow rightward movement;
        // otherwise logic dictates player moving left, slow leftward movement
        slowLeftMovement = (speed & 0x80) != 0;
    }
    else
    { // JoypFrict: take the right controller bit
        slowLeftMovement = (joypadCollision & 0x01) != 0;
    }

    uint8_t newSpeed = 0;
    if (slowLeftMovement)
    { // LeftFrict: load value set here
        // speed:force is one 16-bit quantity, and so is the friction adder
        const uint32_t wide = ((M(Player_X_Speed) << 8) | M(Player_X_MoveForce)) +
                              ((M(FrictionAdderHigh) << 8) | M(FrictionAdderLow)); // add to it another value set here
        writeData(Player_X_MoveForce, LOBYTE(wide));                               // store here
        writeData(Player_X_Speed, HIBYTE(wide));                                   // set as new horizontal speed
        newSpeed = HIBYTE(wide);
        if (((newSpeed - M(MaximumRightSpeed)) & 0x80) == 0)
        {                                       // if horizontal speed not greater negatively,
            newSpeed = M(MaximumRightSpeed);    // set preset value as horizontal speed
            writeData(Player_X_Speed, newSpeed); // thus slowing the player's left movement down
            writeData(Player_XSpeedAbsolute, newSpeed); // SetAbsSpd: skip to the end
            return;
        }
    }
    else
    { // RghtFrict: load value set here
        // speed:force is one 16-bit quantity, and so is the friction adder
        const uint32_t wide = ((M(Player_X_Speed) << 8) | M(Player_X_MoveForce)) -
                              ((M(FrictionAdderHigh) << 8) | M(FrictionAdderLow)); // subtract from it another value set here
        writeData(Player_X_MoveForce, LOBYTE(wide));                               // store here
        writeData(Player_X_Speed, HIBYTE(wide));                                   // set as new horizontal speed
        newSpeed = HIBYTE(wide);
        if (((newSpeed - M(MaximumLeftSpeed)) & 0x80) != 0)
        {                                       // if horizontal speed not greater positively,
            newSpeed = M(MaximumLeftSpeed);     // set preset value as horizontal speed
            writeData(Player_X_Speed, newSpeed); // thus slowing the player's right movement down
        }
    }
    // XSpdSign: if player not moving or moving to the right,
    // leave horizontal speed value unmodified
    if ((newSpeed & 0x80) != 0)
    {
        newSpeed = (uint8_t)(~newSpeed + 0x01); // unsigned walking/running speed
    }
    writeData(Player_XSpeedAbsolute, newSpeed); // SetAbsSpd: store walking/running speed here and leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: pair of {carry flag (used by CoinBlock to adjust a subtraction by one), misc object
// buffer slot found}
std::pair<bool, uint8_t> SMBEngine::FindEmptyMiscSlot()
{
    bool miscSlotSearched = false; // no compare done yet
    uint8_t slot = 0x08;           // start at end of misc objects buffer

    // FMiscLoop: get misc object state; branch out if none found to use current offset
    while (M(Misc_State + slot) != 0)
    {
        --slot;                  // decrement offset
        miscSlotSearched = true; // the offset never falls below five, so this sets the carry
        if (slot == 0x05)
        {                // do this until all slots are checked
            slot = 0x08; // if no empty slots found, use last slot
            break;
        }
    } // UseMiscS: store offset of misc object buffer here (residual)
    writeData(JumpCoinMiscOffset, slot);
    return {miscSlotSearched, slot};
}

//------------------------------------------------------------------------

// Inputs: metatile = metatile value to compare against BrickQBlockMetatiles
// Outputs: pair of {whether a match was found, matching table index (valid when a match was
// found)}
std::pair<bool, uint8_t> SMBEngine::BlockBumpedChk(uint8_t metatile)
{
    // start at end of metatile data; do this until all metatiles are checked
    for (uint8_t i = 0x0d; (i & 0x80) == 0; --i)
    {
        // check to see if current metatile matches metatile found in block buffer
        if (metatile == M(BrickQBlockMetatiles + i))
        { // MatchBump
            return {true, i};
        } // otherwise move onto next metatile
    }
    return {false, 0xff}; // if none match
}

//------------------------------------------------------------------------

// Inputs: blockOffset = block object buffer offset
// Outputs: none
void SMBEngine::SpawnBrickChunks(uint8_t blockOffset)
{
    // set horizontal coordinate of block object
    writeData(Block_Orig_XPos + blockOffset, M(Block_X_Position + blockOffset)); // as original horizontal coordinate here
    writeData(Block_X_Speed + blockOffset, 0xf0);                                // set horizontal speed for brick chunk objects
    writeData(Block_X_Speed + 2 + blockOffset, 0xf0);
    writeData(Block_Y_Speed + blockOffset, 0xfa);     // set vertical speed for one
    writeData(Block_Y_Speed + 2 + blockOffset, 0xfc); // set lower vertical speed for the other
    writeData(Block_Y_MoveForce + blockOffset, 0x00); // init fractional movement force for both
    writeData(Block_Y_MoveForce + 2 + blockOffset, 0x00);
    writeData(Block_PageLoc + 2 + blockOffset, M(Block_PageLoc + blockOffset));       // copy page location
    writeData(Block_X_Position + 2 + blockOffset, M(Block_X_Position + blockOffset)); // copy horizontal coordinate
    // add 8 pixels to the vertical coordinate and save as vertical coordinate for one of them
    writeData(Block_Y_Position + 2 + blockOffset, (uint8_t)(M(Block_Y_Position + blockOffset) + 0x08));
    writeData(Block_Y_Speed + blockOffset, 0xfa); // set vertical speed...again??? (redundant)
}

//------------------------------------------------------------------------

// Inputs: metatile = metatile value to test
// Outputs: bool return indicates whether metatile is one of the hidden-block metatiles (0x5f/0x60)
bool SMBEngine::ChkInvisibleMTiles(uint8_t metatile) { return metatile == 0x5f || metatile == 0x60; }

//------------------------------------------------------------------------

// Inputs: metatile = metatile value to test (0x67/0x68)
// Outputs: none beyond the bool return
bool SMBEngine::ChkJumpspringMetatiles(uint8_t metatile)
{
    bool jumpspringFound = false;

    if (metatile != 0x67)
    { // branch to note the jumpspring if found
        jumpspringFound = false;
        if (metatile != 0x68)
        {
            return jumpspringFound; // branch if not found
        }
    } // JSFnd
    jumpspringFound = true;

    return jumpspringFound; // NoJSFnd: leave
}

//------------------------------------------------------------------------

// Inputs: none (reads Up_Down_Buttons and zero-page 0x00/0x01 set earlier by the caller's block
// buffer collision)
// Outputs: none
void SMBEngine::HandlePipeEntry()
{
    // check saved controller bits from earlier for pressing down
    if ((M(Up_Down_Buttons) & 0b00000100) == 0)
    {
        return; // if not pressing down, branch to leave
    }
    if (M(0x00) != 0x11)
    {
        return; // branch to leave if not found
    }
    if (M(0x01) != 0x10)
    {
        return; // branch to leave if not found
    }
    writeData(ChangeAreaTimer, 0x30);                  // set timer for change of area
    writeData(GameEngineSubroutine, 0x03);             // set to run vertical pipe entry routine on next frame
    writeData(Square1SoundQueue, Sfx_PipeDown_Injury); // load pipedown/injury sound
    writeData(Player_SprAttrib, 0b00100000);           // set background priority bit in player's attributes
    const uint8_t warpZone = M(WarpZoneControl);       // check warp zone control
    if (warpZone == 0)
    {
        return; // branch to leave if none found
    }
    // mask out all but 2 LSB, multiply by four,
    // save as offset to warp zone numbers (starts at left pipe)
    uint8_t pipeIdx = (warpZone & 0b00000011) << 2;
    const uint8_t playerX = M(Player_X_Position); // get player's horizontal position
    if (playerX >= 0x60)
    {                  // if player at left, not near middle, use offset and skip ahead
        ++pipeIdx;     // otherwise increment for middle pipe
        if (playerX >= 0xa0)
        {              // if player at middle, but not too far right, use offset and skip
            ++pipeIdx; // otherwise increment for last pipe
        }
    }
    // GetWNum: get warp zone numbers
    const uint8_t worldNum = M(WarpZoneNumbers + pipeIdx) - 1; // decrement for use as world number
    writeData(WorldNumber, worldNum);                          // store as world number and offset
    const uint8_t areaOfs = M(WorldAddrOffsets + worldNum);    // get offset to where this world's area offsets are
    // get area offset based on world offset
    writeData(AreaPointer, M(AreaAddrOffsets + areaOfs)); // store area offset here to be used to change areas
    writeData(EventMusicQueue, Silence);                  // silence music
    writeData(EntrancePage, 0x00);       // initialize starting page number
    writeData(AreaNumber, 0x00);         // initialize area number used for area address offset
    writeData(LevelNumber, 0x00);        // initialize level number used for world display
    writeData(AltEntranceControl, 0x00); // initialize mode of entry
    ++M(Hidden1UpFlag);                  // set flag for hidden 1-up blocks
    ++M(FetchNewGameTimerFlag);          // set flag to load new game timer

    // ExPipeE: leave!!!
}

//------------------------------------------------------------------------

// Inputs: metatile = metatile value to test (0xc2/0xc3)
// Outputs: none beyond the bool return
bool SMBEngine::CheckForCoinMTiles(uint8_t metatile)
{
    if (metatile == 0xc2 || metatile == 0xc3)
    {                                               // branch if found
        writeData(Square2SoundQueue, Sfx_CoinGrab); // load coin grab sound and leave
        return true;
    }
    return false; // otherwise leave
}

//------------------------------------------------------------------------

// Inputs: metatile = metatile value (forwarded to ChkJumpspringMetatiles)
// Outputs: none
void SMBEngine::ChkForLandJumpSpring(uint8_t metatile)
{
    bool jumpspringFound = false;

    jumpspringFound = ChkJumpspringMetatiles(metatile); // do sub to check if player landed on jumpspring
    if (jumpspringFound)
    {                                     // jumpspring not found, therefore leave
        writeData(VerticalForce, 0x70);   // otherwise set vertical movement force for player
        writeData(JumpspringForce, 0xf9); // set default jumpspring force
        writeData(JumpspringTimer, 0x03);    // set jumpspring timer to be used later
        writeData(JumpspringAnimCtrl, 0x01); // set jumpspring animation control to start animating
    } // ExCJSp: and leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ClimbingSub()
{
    const uint8_t ClimbAdderHigh_data[] = {0x00, 0x00, 0xff, 0xff};

    const uint8_t ClimbAdderLow_data[] = {0x0e, 0x04, 0xfc, 0xf2};

    uint8_t adder = 0x00; // set default adder here
    // get player's vertical speed
    if ((M(Player_Y_Speed) & 0x80) != 0)
    {                 // if not moving upwards, branch
        adder = 0xff; // otherwise set adder to $ff
    } // MoveOnVine: store adder here
    writeData(0x00, adder);
    // highpos:position:dummy is one 24-bit quantity, and $00:speed the signed
    // 16-bit amount to move the player up or down by
    uint32_t wide = (M(Player_Y_HighPos) << 16) | (M(Player_Y_Position) << 8) | M(Player_YMF_Dummy);
    wide += (M(0x00) << 16) | (M(Player_Y_Speed) << 8) | M(Player_Y_MoveForce);
    writeData(Player_YMF_Dummy, LOBYTE(wide));          // add movement force to dummy variable
    writeData(Player_Y_Position, HIBYTE(wide));         // and store to move player up or down
    writeData(Player_Y_HighPos, (uint8_t)(wide >> 16)); // and store
    // compare left/right controller bits to collision flag
    const uint8_t buttons = M(Left_Right_Buttons) & M(Player_CollisionBits);
    if (buttons == 0)
    {                                       // if not set, skip to end
        writeData(ClimbSideTimer, buttons); // InitCSTimer: initialize timer here
        return;
    }
    if (M(ClimbSideTimer) != 0)
    {           // otherwise check timer
        return; // if timer not expired, branch to leave
    }
    writeData(ClimbSideTimer, 0x18); // otherwise set timer now
    uint8_t adderIdx = 0x00;         // set default offset here
    // check the right button controller bit
    if ((buttons & 0x01) == 0)
    {                    // if controller right pressed, branch ahead
        adderIdx = 0x02; // otherwise increment offset by 2 bytes
    } // ClimbFD: check to see if facing right
    if (M(PlayerFacingDir) != 0x01)
    {               // if so, branch, do not increment
        ++adderIdx; // otherwise increment by 1 byte
    } // CSetFDir
    // add to or subtract from the player's 16-bit horizontal position, using the
    // 16-bit value here as the adder, selected by the offset
    const uint32_t pos = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) +
                         ((ClimbAdderHigh_data[adderIdx] << 8) | ClimbAdderLow_data[adderIdx]);
    writeData(Player_X_Position, LOBYTE(pos));
    writeData(Player_PageLoc, HIBYTE(pos));
    // get left/right controller bits again, invert them and store them while player
    writeData(PlayerFacingDir, M(Left_Right_Buttons) ^ 0b00000011); // is on vine to face player in opposite direction
    // ExitCSub: then leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::RemoveCoin_Axe()
{
    const uint8_t areaType = M(AreaType); // check area type
    uint8_t metatileSel = 0x03;           // load offset for default blank metatile
    if (areaType == 0)
    {                       // if not water type, use offset
        metatileSel = 0x04; // otherwise load offset for blank metatile used in water
    } // WriteBlankMT: do a sub to write blank metatile to vram buffer
    PutBlockMetatile(metatileSel, 0x41);   // low byte set so offset points to $0341
    writeData(VRAM_Buffer_AddrCtrl, 0x06);         // set vram address controller to $0341 and leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::DestroyBlockMetatile()
{
    // force blank metatile if branched/jumped to this point
    WriteBlockMetatile(0x00);
}

//------------------------------------------------------------------------

// Inputs: blockOffset = block object buffer offset
// Outputs: none (delegates to JCoinC)
void SMBEngine::CoinBlock(uint8_t blockOffset)
{
    bool miscSlotSearched = false;
    uint8_t miscSlot = 0;

    std::tie(miscSlotSearched, miscSlot) = FindEmptyMiscSlot(); // set offset for empty or last misc object buffer slot
    // get page location of block object
    writeData(Misc_PageLoc + miscSlot, M(Block_PageLoc + blockOffset)); // store as page location of misc object
    // get horizontal coordinate of block object, add 5 pixels
    writeData(Misc_X_Position + miscSlot, M(Block_X_Position + blockOffset) | 0x05); // store as horizontal coordinate of misc object
    // get vertical coordinate of block object, subtract 16 pixels
    // the jump engine reaches CoinBlock with the carry clear, so the slot search
    // above only leaves it set if it got as far as its compare
    writeData(Misc_Y_Position + miscSlot,
              (uint8_t)(M(Block_Y_Position + blockOffset) - 0x10 - (miscSlotSearched ? 0 : 1))); // store as vertical coordinate
    JCoinC(blockOffset, miscSlot); // jump to rest of code as applies to this misc object
}

//------------------------------------------------------------------------

// Inputs: blockOffset = block object buffer offset
// Outputs: none (delegates to JCoinC)
void SMBEngine::SetupJumpCoin(uint8_t blockOffset)
{
    bool miscSlotSearched = false;
    uint8_t miscSlot = 0;

    std::tie(miscSlotSearched, miscSlot) = FindEmptyMiscSlot(); // set offset for empty or last misc object buffer slot
    // get page location saved earlier
    writeData(Misc_PageLoc + miscSlot, M(Block_PageLoc2 + blockOffset)); // and save as page location for misc object
    const uint8_t bufLow = M(0x06);                    // get low byte of block buffer offset
    const bool shiftedBit = (bufLow & 0x10) != 0;      // the fourth shift below carries d4 out
    // multiply by 16 to use lower nybble, add five pixels
    writeData(Misc_X_Position + miscSlot, (uint8_t)(bufLow << 4) | 0x05); // save as horizontal coordinate for misc object
    // get vertical high nybble offset from earlier, add 32 pixels for the status bar,
    // plus the bit shifted out above
    writeData(Misc_Y_Position + miscSlot, (uint8_t)(M(0x02) + 0x20 + (shiftedBit ? 1 : 0))); // store as vertical coordinate

    JCoinC(blockOffset, miscSlot);
}

//------------------------------------------------------------------------

// Inputs: blockOffset = block object buffer offset (stored to ObjectOffset); miscSlot = misc
// object buffer slot (from FindEmptyMiscSlot)
// Outputs: none
void SMBEngine::JCoinC(uint8_t blockOffset, uint8_t miscSlot)
{
    writeData(Misc_Y_Speed + miscSlot, 0xfb);   // set vertical speed
    writeData(Misc_Y_HighPos + miscSlot, 0x01); // set vertical high byte
    writeData(Misc_State + miscSlot, 0x01);     // set state for misc object
    writeData(Square2SoundQueue, 0x01);         // load coin grab sound
    writeData(ObjectOffset, blockOffset);       // store current control bit as misc object offset
    GiveOneCoin();                              // update coin tally on the screen and coin amount variable
    ++M(CoinTallyFor1Ups);                      // increment coin tally used to activate 1-up block flag
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::GiveOneCoin()
{
    const uint8_t CoinTallyOffsets_data[] = {0x17, 0x1d};

    writeData(DigitModifier + 5, 0x01); // set digit modifier to add 1 coin to the current player's coin tally
    // get offset for the current onscreen player's coin tally
    DigitsMathRoutine(CoinTallyOffsets_data[M(CurrentPlayer)]); // update the coin tally
    ++M(CoinTally);                                             // increment onscreen player's coin amount
    if (M(CoinTally) == 100)
    {                               // if not, skip all of this
        writeData(CoinTally, 0x00); // otherwise, reinitialize coin amount
        ++M(NumberofLives);         // give the player an extra life
        writeData(Square2SoundQueue, Sfx_ExtraLife); // play 1-up sound
    } // CoinPoints
    writeData(DigitModifier + 4, 0x02); // set digit modifier to award 200 points to the player

    AddToScore();
}

//------------------------------------------------------------------------

// Inputs: blockOffset = block object buffer offset
// Outputs: none (delegates to PwrUpJmp)
void SMBEngine::SetupPowerUp(uint8_t blockOffset)
{
    // load power-up identifier into
    writeData(Enemy_ID + 5, PowerUpObject); // special use slot of enemy object buffer
    // store page location of block object
    writeData(Enemy_PageLoc + 5, M(Block_PageLoc + blockOffset)); // as page location of power-up object
    // store horizontal coordinate of block object
    writeData(Enemy_X_Position + 5, M(Block_X_Position + blockOffset)); // as horizontal coordinate of power-up object
    writeData(Enemy_Y_HighPos + 5, 0x01);                               // set vertical high byte of power-up object
    // get vertical coordinate of block object, subtract 8 pixels
    writeData(Enemy_Y_Position + 5, (uint8_t)(M(Block_Y_Position + blockOffset) - 0x08)); // use as power-up object's vertical coordinate

    PwrUpJmp();
}

//------------------------------------------------------------------------

// Inputs: blockOffset = block object buffer offset (forwarded through Skip_4/Skip_5 to SetupPowerUp)
// Outputs: none
void SMBEngine::MushFlowerBlock(uint8_t blockOffset)
{
    Skip_4(0x00, blockOffset); // load mushroom/fire flower into power-up type
}

//------------------------------------------------------------------------

// Inputs: blockOffset = block object buffer offset (forwarded through Skip_4/Skip_5 to SetupPowerUp)
// Outputs: none
void SMBEngine::StarBlock(uint8_t blockOffset)
{
    Skip_4(0x02, blockOffset); // load star into power-up type
}

//------------------------------------------------------------------------

// Inputs: powerUpType = power-up type selector; blockOffset = block object buffer offset (both
// forwarded to Skip_5)
// Outputs: none
void SMBEngine::Skip_4(uint8_t powerUpType, uint8_t blockOffset) { Skip_5(powerUpType, blockOffset); }

//------------------------------------------------------------------------

// Inputs: blockOffset = block object buffer offset (forwarded through Skip_5 to SetupPowerUp)
// Outputs: none
void SMBEngine::ExtraLifeMushBlock(uint8_t blockOffset)
{
    Skip_5(0x03, blockOffset); // load 1-up mushroom into power-up type
}

//------------------------------------------------------------------------

// Inputs: powerUpType = power-up type to store; blockOffset = block object buffer offset
// (forwarded to SetupPowerUp)
// Outputs: none
void SMBEngine::Skip_5(uint8_t powerUpType, uint8_t blockOffset)
{
    writeData(0x39, powerUpType); // store correct power-up type
    SetupPowerUp(blockOffset);
}

//------------------------------------------------------------------------

// Inputs: none (reads SprDataOffset_Ctrl via CheckTopOfBlock)
// Outputs: none
void SMBEngine::BrickShatter()
{
    const uint8_t blockOffset = CheckTopOfBlock(); // check to see if there's a coin directly above this block
    writeData(Block_RepFlag + blockOffset, Sfx_BrickShatter); // set flag for block object to immediately replace metatile
    writeData(NoiseSoundQueue, Sfx_BrickShatter);             // load brick shatter sound
    SpawnBrickChunks(blockOffset);                            // create brick chunk objects
    writeData(Player_Y_Speed, 0xfe);                          // set vertical speed for player
    writeData(DigitModifier + 5, 0x05);                       // set digit modifier to give player 50 points
    AddToScore();                                             // do sub to update the score
}

//------------------------------------------------------------------------

// Inputs: none (reads SprDataOffset_Ctrl and zero-page 0x02/0x06 itself)
// Outputs: block object buffer offset (M(SprDataOffset_Ctrl), reloaded on every return path)
uint8_t SMBEngine::CheckTopOfBlock()
{
    const uint8_t vertOfs = M(0x02); // get vertical high nybble offset used in block buffer
    if (vertOfs == 0)
    {
        return M(SprDataOffset_Ctrl); // branch to leave if set to zero, because we're at the top
    }
    const uint8_t rowUp = vertOfs - 0x10; // subtract $10 to move up one row in the block buffer
    writeData(0x02, rowUp);               // store as new vertical high nybble offset
    if (M(W(0x06) + rowUp) != 0xc2)
    {                                 // get contents of block buffer in same column, one row up
        return M(SprDataOffset_Ctrl); // if not a coin, branch to leave
    }
    writeData(W(0x06) + rowUp, 0x00);    // otherwise put blank metatile where coin was
    RemoveCoin_Axe();                    // write blank metatile to vram buffer
    SetupJumpCoin(M(SprDataOffset_Ctrl)); // create jumping coin object and update coin variables

    return M(SprDataOffset_Ctrl); // TopEx: leave!
}

//------------------------------------------------------------------------

// load vertical high nybble offset for block buffer
// Inputs: none
// Outputs: none
void SMBEngine::ErACM()
{
    const uint8_t vertOfs = M(0x02);
    // load blank metatile
    writeData(W(0x06) + vertOfs, 0x00); // store to remove old contents from block buffer
    RemoveCoin_Axe();                   // update the screen accordingly
}

//------------------------------------------------------------------------

// Inputs: adderBaseOffset = block buffer adder base offset (incremented then forwarded to
// BlockBufferColli_Head/BlockBufferCollision)
// Outputs: pair of {metatile, coordinate low nybble} (see BlockBufferCollision)
std::pair<uint8_t, uint8_t> SMBEngine::BlockBufferColli_Feet(uint8_t adderBaseOffset)
{
    // if branched here, increment to next set of adders
    return BlockBufferColli_Head(adderBaseOffset + 1);
}

//------------------------------------------------------------------------

// Inputs: adderOffset = block buffer adder offset (forwarded to Skip_9/BlockBufferCollision)
// Outputs: pair of {metatile, vertical coordinate low nybble} (see BlockBufferCollision)
std::pair<uint8_t, uint8_t> SMBEngine::BlockBufferColli_Head(uint8_t adderOffset)
{
    // set flag to return vertical coordinate
    return Skip_9(0x00, adderOffset);
}

//------------------------------------------------------------------------

// Inputs: adderOffset = block buffer adder offset (forwarded to Skip_9/BlockBufferCollision)
// Outputs: pair of {metatile, horizontal coordinate low nybble} (see BlockBufferCollision)
std::pair<uint8_t, uint8_t> SMBEngine::BlockBufferColli_Side(uint8_t adderOffset)
{
    // set flag to return horizontal coordinate
    return Skip_9(0x01, adderOffset);
}

//------------------------------------------------------------------------

// Inputs: coordSelector = which coordinate's low nybble to report; cornerIdx = corner-selector
// index (both forwarded to BlockBufferCollision)
// Outputs: pair of {metatile, coordinate low nybble} (see BlockBufferCollision)
std::pair<uint8_t, uint8_t> SMBEngine::Skip_9(uint8_t coordSelector, uint8_t cornerIdx)
{
    // set offset for player object
    return BlockBufferCollision(coordSelector, 0x00, cornerIdx);
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::OnGroundStateSub()
{
    GetPlayerAnimSpeed(); // do a sub to set animation frame timing
    const uint8_t buttons = M(Left_Right_Buttons);
    if (buttons != 0)
    {                                        // if left/right controller bits not set, skip instruction
        writeData(PlayerFacingDir, buttons); // otherwise set new facing direction
    } // GndMove: do a sub to impose friction on player's walk/run
    ImposeFriction(buttons);
    // do another sub to move player horizontally
    writeData(Player_X_Scroll, MovePlayerHorizontally()); // set returned value as player's movement speed for scroll
}

//------------------------------------------------------------------------

// Inputs: none (always operates on the player, object index 0)
// Outputs: carry-plus-high-nybble value from MoveObjectHorizontally when not animating a
// jumpspring; otherwise JumpspringAnimCtrl's value. The caller stores this into Player_X_Scroll
// either way
uint8_t SMBEngine::MovePlayerHorizontally()
{
    const uint8_t jumpspringAnim = M(JumpspringAnimCtrl); // if jumpspring currently animating,
    if (jumpspringAnim != 0)
    {
        return jumpspringAnim; // branch to leave
    }
    // otherwise set zero for offset to use player's stuff
    return MoveObjectHorizontally(0x00);
}

//------------------------------------------------------------------------

// Inputs: none (always operates on the player, object index 0)
// Outputs: none
void SMBEngine::MovePlayerVertically()
{
    if (M(TimerControl) == 0)
    { // if master timer control set, branch ahead
        if (M(JumpspringAnimCtrl) != 0)
        {           // otherwise check to see if jumpspring is animating
            return; // branch to leave if so
        }
    } // NoJSChk: dump vertical force
    // set maximum vertical speed, use zero for player offset, then jump to move player vertically
    ImposeGravitySprObj(0x04, 0x00, M(VerticalForce));
}

//------------------------------------------------------------------------

// Inputs: metatile = metatile value to test (forwarded to GetMTileAttrib)
// Outputs: none beyond the bool return
bool SMBEngine::CheckForSolidMTiles(uint8_t metatile)
{
    const uint8_t SolidMTileUpperExt_data[] = {0x10, 0x61, 0x88, 0xc4};

    bool solidMTileFound = false;

    uint8_t offset = GetMTileAttrib(metatile);                     // find appropriate offset based on metatile's 2 MSB
    solidMTileFound = metatile >= SolidMTileUpperExt_data[offset]; // compare current metatile with solid metatiles
    return solidMTileFound;
}

//------------------------------------------------------------------------

// Inputs: metatile = metatile value to test (forwarded to GetMTileAttrib)
// Outputs: none beyond the bool return
bool SMBEngine::CheckForClimbMTiles(uint8_t metatile)
{
    const uint8_t ClimbMTileUpperExt_data[] = {0x24, 0x6d, 0x8a, 0xc6};

    bool climbMTileFound = false;

    uint8_t offset = GetMTileAttrib(metatile);                     // find appropriate offset based on metatile's 2 MSB
    climbMTileFound = metatile >= ClimbMTileUpperExt_data[offset]; // compare current metatile with climbable metatiles
    return climbMTileFound;
}

//------------------------------------------------------------------------

// Inputs: metatile = metatile value
// Outputs: offset (0-3) selecting which upper-extent table entry to use, derived from the
// metatile's 2 MSB
uint8_t SMBEngine::GetMTileAttrib(uint8_t metatile)
{
    return (metatile & 0b11000000) >> 6; // ExEBG: leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerMovementSubs()
{
    uint8_t crouch = 0x00;       // set to init crouch flag by default
    bool storeCrouchFlag = true; // skipped when small and not on the ground
    // is player small?
    if (M(PlayerSize) == 0)
    { // if so, branch
        // check state of player
        if (M(Player_State) != 0)
        {
            storeCrouchFlag = false; // if not on the ground, branch
        }
        else
        {
            // load controller bits for up and down
            crouch = M(Up_Down_Buttons) & 0b00000100; // single out bit for down button
        }
    } // SetCrouch: store value in crouch flag
    if (storeCrouchFlag)
    {
        writeData(CrouchingFlag, crouch);
    }

    // ProcMove: run sub related to jumping and swimming
    PlayerPhysicsSub();
    if (M(PlayerChangeSizeFlag) != 0)
    {           // if growing/shrinking flag set,
        return; // NoMoveSub: branch to leave
    }
    const uint8_t state = M(Player_State);
    if (state != 0x03)
    {                                    // if climbing, branch ahead, leave timer unset
        writeData(ClimbSideTimer, 0x18); // otherwise reset timer now
    } // MoveSubs
    switch (state)
    {
    case 0:
        OnGroundStateSub();
        return;
    case 1: // JumpSwimSub
    case 2: // FallingSub
        break;
    case 3:
        ClimbingSub();
        return;
    default:
        bad_jump();
        return;
    }

    if (state == 2)
    { // FallingSub: dump vertical movement force for falling into main one
        writeData(VerticalForce, M(VerticalForceDown)); // movement force, then skip ahead to process left/right movement
    }
    else
    { // JumpSwimSub: if player's vertical speed zero
        bool dumpFall = true;
        if ((M(Player_Y_Speed) & 0x80) != 0)
        { // or moving downwards, branch to falling
            // check to see if A button is being pressed and was pressed in previous frame
            if ((M(A_B_Buttons) & A_Button & M(PreviousA_B_Buttons)) != 0)
            {
                dumpFall = false; // if so, branch to ProcSwim
            }
            // get vertical position player jumped from, subtract current from original vertical coordinate
            else if ((uint8_t)(M(JumpOrigin_Y_Position) - M(Player_Y_Position)) < M(DiffToHaltJump))
            {
                dumpFall = false; // or just starting to jump, if just starting, skip ahead to ProcSwim
            }
        }
        if (dumpFall)
        { // DumpFall: otherwise dump falling into main fractional
            writeData(VerticalForce, M(VerticalForceDown));
        }

        // ProcSwim: if swimming flag not set, branch ahead to last part
        if (M(SwimmingFlag) != 0)
        {
            GetPlayerAnimSpeed(); // do a sub to get animation frame timing
            if (M(Player_Y_Position) < 0x14)
            {                                   // if not yet reached a certain position, branch ahead
                writeData(VerticalForce, 0x18); // otherwise set fractional
            } // LRWater: check left/right controller bits (check for swimming)
            const uint8_t swimButtons = M(Left_Right_Buttons);
            if (swimButtons != 0)
            {                                            // if not pressing any, skip
                writeData(PlayerFacingDir, swimButtons); // otherwise set facing direction accordingly
            }
        }
    }

    // LRAir: check left/right controller bits (check for jumping/falling)
    const uint8_t buttons = M(Left_Right_Buttons);
    if (buttons != 0)
    {                            // if not pressing any, skip
        ImposeFriction(buttons); // otherwise process horizontal movement
    } // JSMove: do a sub to move player horizontally
    writeData(Player_X_Scroll, MovePlayerHorizontally()); // set player's speed here, to be used for scroll later
    if (M(GameEngineSubroutine) == 0x0b)
    {                                   // branch if not set to run
        writeData(VerticalForce, 0x28); // otherwise set fractional
    } // ExitMov1: jump to move player vertically, then leave
    MovePlayerVertically();
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none (delegates to ImpedePlayerMove)
void SMBEngine::StopPlayerMove()
{
    ImpedePlayerMove(); // stop player's movement

    // ExCSM: leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::HandleCoinMetatile()
{
    ErACM();               // do sub to erase coin metatile from block buffer
    ++M(CoinTallyFor1Ups); // increment coin tally used for 1-up blocks
    GiveOneCoin();         // update coin amount and tally on the screen
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PutPlayerOnVine()
{
    // set player state to climbing
    writeData(Player_State, 0x03);
    // nullify player's horizontal speed
    writeData(Player_X_Speed, 0x00); // and fractional horizontal movement force
    writeData(Player_X_MoveForce, 0x00);
    // get player's horizontal coordinate, subtract from left side horizontal coordinate
    if ((uint8_t)(M(Player_X_Position) - M(ScreenLeft_X_Pos)) < 0x10)
    {                                     // if 16 or more pixels difference, do not alter facing direction
        writeData(PlayerFacingDir, 0x02); // otherwise force player to face left
    } // SetVXPl: get current facing direction, use as offset
    const uint8_t facingDir = M(PlayerFacingDir);
    const uint8_t bufLow = M(0x06); // get low byte of block buffer address
    // move low nybble to high, add pixels depending on facing direction
    writeData(Player_X_Position, (uint8_t)((uint8_t)(bufLow << 4) + M(ClimbXPosAdder - 1 + facingDir))); // store as player's horizontal coordinate
    if (bufLow == 0)
    { // get low byte of block buffer address again; if not zero, branch
        // load page location of right side of screen, add depending on facing location
        writeData(Player_PageLoc, (uint8_t)(M(ScreenRight_PageLoc) + M(ClimbPLocAdder - 1 + facingDir))); // store as player's page location
    } // ExPVne: finally, we're done!
}

//------------------------------------------------------------------------

// Inputs: collidedMetatile = the metatile the player's head collided with
// Outputs: none
void SMBEngine::PlayerHeadCollision(uint8_t collidedMetatile)
{
    const uint8_t BlockYPosAdderData_data[] = {0x04, 0x12};

    const uint8_t blockOffset = M(SprDataOffset_Ctrl); // load offset control bit here
    uint8_t blockState = 0x11;                         // load unbreakable block object state by default
    // check player's size
    if (M(PlayerSize) == 0)
    {                      // if small, branch
        blockState = 0x12; // otherwise load breakable block object state
    } // DBlockSte: store into block object buffer
    writeData(Block_State + blockOffset, blockState);
    DestroyBlockMetatile();            // store blank metatile in vram buffer to write to name table
    const uint8_t vertOfs = M(0x02);   // get vertical high nybble offset used in block buffer routine
    writeData(Block_Orig_YPos + blockOffset, vertOfs); // set as vertical coordinate for block object
    // get low byte of block buffer address used in same routine
    writeData(Block_BBuf_Low + blockOffset, M(0x06)); // save as offset here to be used later
    const uint8_t oldMetatile = M(W(0x06) + vertOfs); // get contents of block buffer at old address at $06, $07
    const bool bumpedBlockFound = BlockBumpedChk(oldMetatile).first; // do a sub to check which block player bumped head on
    writeData(0x00, oldMetatile);                                    // store metatile here
    // check player's size: if small, use metatile itself, otherwise init to zero (note: big = 0)
    uint8_t storeMetatile = M(PlayerSize) == 0 ? 0x00 : oldMetatile;
    // ChkBrick: if no match was found in previous sub, skip ahead
    if (bumpedBlockFound)
    {
        // otherwise load unbreakable state into block object buffer
        writeData(Block_State + blockOffset, 0x11); // note this applies to both player sizes
        storeMetatile = 0xc4;                       // load empty block metatile for now
        const uint8_t bumpedMetatile = M(0x00);     // get metatile from before
        if (bumpedMetatile == 0x58 || bumpedMetatile == 0x5d)
        { // if not a coin brick, keep the empty block metatile
            // StartBTmr: check brick coin timer flag
            if (M(BrickCoinTimerFlag) == 0)
            {                                    // if set, timer expired or counting down, thus branch
                writeData(BrickCoinTimer, 0x0b); // if not set, set brick coin timer
                ++M(BrickCoinTimerFlag);         // and set flag linked to it
            } // ContBTmr: check brick coin timer
            storeMetatile = bumpedMetatile; // PutOldMT: use current metatile
            if (M(BrickCoinTimer) == 0)
            {                         // if not yet expired, branch to use current metatile
                storeMetatile = 0xc4; // otherwise use empty block metatile
            }
        }
    }
    // PutMTileB: store whatever metatile be appropriate here
    writeData(Block_Metatile + blockOffset, storeMetatile);
    InitBlock_XY_Pos(blockOffset);       // get block object horizontal coordinates saved
    writeData(W(0x06) + M(0x02), 0x23);  // get vertical high nybble offset; write blank metatile $23 to block buffer
    writeData(BlockBounceTimer, 0x10);   // set block bounce timer
    uint8_t sizeIdx = 0x00;              // set default offset
    // is player crouching? is player big? increment for small, or big and crouching
    if (M(CrouchingFlag) != 0 || M(PlayerSize) != 0)
    {                   // SmallBP
        sizeIdx = 0x01; // otherwise use default offset (BigBP)
    }
    // BigBP: get player's vertical coordinate, add value determined by size,
    // mask out low nybble to get 16-pixel correspondence
    writeData(Block_Y_Position + blockOffset,
              (M(Player_Y_Position) + BlockYPosAdderData_data[sizeIdx]) & 0xf0); // save as vertical coordinate for block object
    // get block object state
    if (M(Block_State + blockOffset) != 0x11)
    {                   // if set to value loaded for unbreakable, branch
        BrickShatter(); // execute code for breakable brick
    } // Unbreak: execute code for unbreakable brick or question block
    else // skip subroutine to do last part of code here
    {
        BumpBlock(collidedMetatile);
    } // InvOBit: invert control bit used by block objects
    writeData(SprDataOffset_Ctrl, M(SprDataOffset_Ctrl) ^ 0x01); // and floatey numbers
    // leave!
}

//------------------------------------------------------------------------

// Inputs: collidedMetatile = the original metatile the player's head collided with; also reads
// SprDataOffset_Ctrl via CheckTopOfBlock
// Outputs: none
void SMBEngine::BumpBlock(uint8_t collidedMetatile)
{
    bool bumpedBlockFound = false;

    const uint8_t blockOffset = CheckTopOfBlock();    // check to see if there's a coin directly above this block
    writeData(Square1SoundQueue, Sfx_Bump);           // play bump sound
    writeData(Block_X_Speed + blockOffset, 0x00);     // initialize horizontal speed for block object
    writeData(Block_Y_MoveForce + blockOffset, 0x00); // init fractional movement force
    writeData(Player_Y_Speed, 0x00);                  // init player's vertical speed
    writeData(Block_Y_Speed + blockOffset, 0xfe);     // set vertical speed for block object
    uint8_t blockIdx = 0;
    // get original metatile, do a sub to check which block player bumped head on
    std::tie(bumpedBlockFound, blockIdx) = BlockBumpedChk(collidedMetatile);
    if (!bumpedBlockFound)
    { // if no match was found, branch to leave
        return;
    }
    uint8_t blockNum = blockIdx; // move block number here
    if (blockNum >= 0x09)
    {                     // branch to use current number
        blockNum -= 0x05; // otherwise subtract 5 for second set to get proper number
    } // BlockCode: run appropriate subroutine depending on block number
    switch (blockNum)
    {
    case 0:
        MushFlowerBlock(blockOffset);
        return;
    case 1:
        CoinBlock(blockOffset);
        return;
    case 2:
        CoinBlock(blockOffset);
        return;
    case 3:
        ExtraLifeMushBlock(blockOffset);
        return;
    case 4:
        MushFlowerBlock(blockOffset);
        return;
    case 5:
        VineBlock();
        return;
    case 6:
        StarBlock(blockOffset);
        return;
    case 7:
        CoinBlock(blockOffset);
        return;
    case 8:
        ExtraLifeMushBlock(blockOffset);
        return;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::VineBlock()
{
    // load last slot for enemy object buffer, get control bit
    Setup_Vine(0x05, M(SprDataOffset_Ctrl)); // set up vine object
    // leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerBGCollision()
{
    const uint8_t BlockBufferAdderData_data[] = {0x00, 0x07, 0x0e};

    const uint8_t FlagpoleYPosData_data[] = {0x18, 0x22, 0x50, 0x68, 0x90};

    const uint8_t AreaChangeTimerData_data[] = {0xa0, 0x34};

    const uint8_t PlayerBGUpperExtent_data[] = {0x20, 0x10};

    // HandleClimbing / ChkForFlagpole / VineCollision: the player's side touched a
    // climbable metatile
    auto handleClimbing = [&](uint8_t metatile, uint8_t horizNybble) {
        // check low nybble of horizontal coordinate returned from collision detection;
        // this makes the actual physical part of the vine or flagpole thinner
        if (horizNybble < 0x06 || horizNybble >= 0x0a)
        {
            return; // ExHC: leave if too far left or too far right
        }
        // ChkForFlagpole: branch if flagpole ball or flagpole shaft found
        if (metatile == 0x24 || metatile == 0x25)
        { // FlagpoleCollision
            if (M(GameEngineSubroutine) == 0x05)
            { // if end-of-level routine running, branch to end of climbing code
                PutPlayerOnVine();
                return;
            }
            writeData(PlayerFacingDir, 0x01); // set player's facing direction to right
            ++M(ScrollLock);                  // set scroll lock flag
            if (M(GameEngineSubroutine) != 0x04)
            { // if flagpole slide routine not running yet, set it up
                KillEnemies(BulletBill_CannonVar);   // get rid of bullet bills (cannon variant)
                writeData(EventMusicQueue, Silence); // silence music
                writeData(FlagpoleSoundQueue, 0x40); // load flagpole sound into flagpole sound queue
                const uint8_t playerY = M(Player_Y_Position);
                writeData(FlagpoleCollisionYPos, playerY); // store player's vertical coordinate here to be used later

                // ChkFlagpoleYPosLoop: start at end of vertical coordinate data and
                // decrement the offset while the player is above the current coordinate
                // (use the last one if all are checked)
                uint8_t scoreOfs = 0x04;
                while (scoreOfs != 0 && playerY < FlagpoleYPosData_data[scoreOfs])
                {
                    --scoreOfs;
                }
                writeData(FlagpoleScore, scoreOfs); // MtchF: store offset here to be used later
            } // RunFR
            writeData(GameEngineSubroutine, 0x04); // set value to run flagpole slide routine
            PutPlayerOnVine();                     // jump to end of climbing code
            return;
        }
        // VineCollision: if the player collided with a vine far enough up the screen,
        if (metatile == 0x26 && M(Player_Y_Position) < 0x20)
        {
            writeData(GameEngineSubroutine, 0x01); // set to run autoclimb routine next frame
        }
        PutPlayerOnVine();
    };

    // CheckSideMTiles: the player's side bumped into a nonzero metatile
    auto checkSideMTiles = [&](uint8_t metatile, uint8_t horizNybble) {
        if (ChkInvisibleMTiles(metatile))
        {           // check for hidden or coin 1-up blocks
            return; // branch to leave if either found
        }
        if (CheckForClimbMTiles(metatile))
        {                                          // check for climbable metatiles
            handleClimbing(metatile, horizNybble); // if found, jump to handle climbing
            return;
        } // ContSChk: check to see if player touched coin
        if (CheckForCoinMTiles(metatile))
        {
            HandleCoinMetatile(); // if so, execute code to erase coin and award to player 1 coin
            return;
        }
        if (ChkJumpspringMetatiles(metatile))
        { // check for jumpspring metatiles
            if (M(JumpspringAnimCtrl) != 0)
            {           // check jumpspring animation control
                return; // branch to leave if set
            }
            StopPlayerMove(); // otherwise jump to impede player's movement
            return;
        } // ChkPBtm: get player's state
        if (M(Player_State) != 0x00)
        {                     // if not on the ground,
            StopPlayerMove(); // branch to impede player's movement
            return;
        }
        if (M(PlayerFacingDir) != 0x01)
        {                     // get player's facing direction
            StopPlayerMove(); // if facing left, branch to impede movement
            return;
        }
        if (metatile != 0x6c && metatile != 0x1f)
        {                     // if not collided with sideways pipe (bottom),
            StopPlayerMove(); // branch to impede player's movement
            return;
        } // PipeDwnS: check player's attributes
        if (M(Player_SprAttrib) == 0)
        { // if background priority bit already set, do not play sound again
            writeData(Square1SoundQueue, Sfx_PipeDown_Injury); // otherwise load pipedown/injury sound
        } // PlyrPipe: set background priority bit in player attributes
        writeData(Player_SprAttrib, M(Player_SprAttrib) | 0b00100000);
        // get lower nybble of player's horizontal coordinate
        if ((M(Player_X_Position) & 0b00001111) != 0)
        { // if not at zero, set timer for change of area:
            // use default offset for timer setting data if the left side of the screen is
            // at page zero, otherwise increment offset
            const uint8_t timerOfs = M(ScreenLeft_PageLoc) != 0 ? 0x01 : 0x00;
            // SetCATmr: set timer for change of area as appropriate
            writeData(ChangeAreaTimer, AreaChangeTimerData_data[timerOfs]);
        } // ChkGERtn: get number of game engine routine running
        const uint8_t engineRoutine = M(GameEngineSubroutine);
        if (engineRoutine == 0x07)
        {
            return; // if running player entrance routine or
        }
        if (engineRoutine != 0x08)
        {
            return; // player control routine, branch to leave
        }
        writeData(GameEngineSubroutine, 0x02); // otherwise set sideways pipe entry routine to run
    };

    if (M(DisableCollisionDet) != 0)
    {
        return; // if collision detection disabled flag set, branch to leave
    }
    const uint8_t engineRoutine = M(GameEngineSubroutine);
    if (engineRoutine == 0x0b)
    { // if running sideways pipe entry routine,
        return; // branch to leave
    }
    if (engineRoutine < 0x04)
    {
        return; // if running routines $00-$03 branch to leave
    }
    // set whatever player state is appropriate: swimming gets the swimming state, normal
    // and climbing get the falling state, any other state is left alone
    if (M(SwimmingFlag) != 0)
    { // if swimming flag set, set default player state for swimming
        writeData(Player_State, 0x01);
    }
    else if (M(Player_State) == 0x00 || M(Player_State) == 0x03)
    { // SetFallS: set default player state for falling
        writeData(Player_State, 0x02);
    }

    // ChkOnScr
    if (M(Player_Y_HighPos) != 0x01)
    {
        return; // branch to leave if player is not on the screen
    }
    writeData(Player_CollisionBits, 0xff); // initialize player's collision flag
    if (M(Player_Y_Position) >= 0xcf)
    {
        return; // ExPBGCol: leave if too close to the bottom of screen
    }

    // ChkCollSize: pick the block buffer adders — the third set for a crouching or small
    // player, the second for a big player swimming, the first for a big player walking
    uint8_t adderIdx = 0x02; // load default offset
    if (M(CrouchingFlag) == 0 && M(PlayerSize) == 0)
    {
        adderIdx = 0x01; // decrement offset for big player not crouching
        if (M(SwimmingFlag) == 0)
        {
            adderIdx = 0x00; // otherwise decrement offset
        }
    }
    // GBBAdr: get value using offset
    const uint8_t bbAdder = BlockBufferAdderData_data[adderIdx];
    writeData(0xeb, bbAdder); // store value here

    uint8_t upperExtentIdx = M(PlayerSize); // get player's size as offset
    if (M(CrouchingFlag) != 0)
    {
        ++upperExtentIdx; // if player crouching, increment size as offset
    }
    // HeadChk: get player's vertical coordinate
    if (M(Player_Y_Position) >= PlayerBGUpperExtent_data[upperExtentIdx])
    { // if player is not too high,
        // do player-to-bg collision detection on top of player
        const auto [headMetatile, headNybble] = BlockBufferColli_Head(bbAdder);
        if (headMetatile != 0)
        { // branch to foot check if nothing above player's head
            if (CheckForCoinMTiles(headMetatile))
            {                         // check to see if player touched coin with their head
                HandleCoinMetatile(); // AwardTouchedCoin: erase coin and award to player 1 coin
                return;
            }
            // check player's vertical speed and lower nybble of vertical coordinate returned
            if ((M(Player_Y_Speed) & 0x80) != 0 && headNybble >= 0x04)
            { // if player moving upwards and low nybble >= 4,
                const bool solid = CheckForSolidMTiles(headMetatile); // check to see what player's head bumped on
                if (!solid && M(AreaType) != 0 && M(BlockBounceTimer) == 0)
                { // if not solid, not a water level, and block bounce timer expired,
                    PlayerHeadCollision(headMetatile); // do a sub to process collision
                }
                else
                { // SolidOrClimb: for any solid metatile but the coral,
                    if (solid && headMetatile != 0x26)
                    {
                        writeData(Square1SoundQueue, Sfx_Bump); // load bump sound
                    }
                    // NYSpd: set player's vertical speed to nullify jump or swim
                    writeData(Player_Y_Speed, 0x01);
                }
            }
        }
    }

    // DoFootCheck: get block buffer adder offset
    const uint8_t footAdder = M(0xeb);
    if (M(Player_Y_Position) < 0xcf)
    { // if player is too far down on screen, skip all of this
        // do player-to-bg collision detection on bottom left of player
        const uint8_t leftFootMetatile = BlockBufferColli_Feet(footAdder).first;
        if (CheckForCoinMTiles(leftFootMetatile))
        {                         // check to see if player touched coin with their left foot
            HandleCoinMetatile(); // AwardTouchedCoin
            return;
        }
        // do player-to-bg collision detection on bottom right of player (the original
        // reached this call with Y left incremented by the first call)
        const auto [rightFootMetatile, footNybble] = BlockBufferColli_Feet(footAdder + 0x01);
        writeData(0x00, rightFootMetatile); // save bottom right metatile here
        writeData(0x01, leftFootMetatile);  // save bottom left metatile here

        uint8_t footMetatile = leftFootMetatile;
        if (leftFootMetatile == 0 && rightFootMetatile != 0)
        { // if nothing under the left foot but something under the right,
            if (CheckForCoinMTiles(rightFootMetatile))
            {                         // check to see if player touched coin with their right foot
                HandleCoinMetatile(); // AwardTouchedCoin
                return;
            }
            footMetatile = rightFootMetatile;
        }
        // ChkFootMTile: branch to side check if player landed on climbable metatile or is
        // moving upwards
        if (footMetatile != 0 && !CheckForClimbMTiles(footMetatile) && (M(Player_Y_Speed) & 0x80) == 0)
        {
            if (footMetatile == 0xc5)
            {                                   // HandleAxeMetatile: player touched the axe
                writeData(OperMode_Task, 0x00); // reset secondary mode
                writeData(OperMode, 0x02);      // set primary mode to autoctrl mode
                // set horizontal speed and continue to erase axe metatile
                writeData(Player_X_Speed, 0x18);
                ErACM();
                return;
            }
            // ContChk: do sub to check for hidden coin or 1-up blocks
            if (!ChkInvisibleMTiles(footMetatile))
            { // if neither found,
                if (M(JumpspringAnimCtrl) == 0)
                { // if jumpspring not animating right now,
                    // check lower nybble of vertical coordinate returned
                    if (footNybble >= 0x05)
                    { // if lower nybble >= 5,
                        writeData(0x00, M(Player_MovingDir)); // use player's moving direction as temp variable
                        ImpedePlayerMove();                   // jump to impede player's movement in that direction
                        return;
                    } // LandPlyr: do sub to check for jumpspring metatiles and deal with it
                    ChkForLandJumpSpring(footMetatile);
                    // mask out lower nybble of player's vertical position and store as new
                    // vertical position to land player properly
                    writeData(Player_Y_Position, 0xf0 & M(Player_Y_Position));
                    HandlePipeEntry();                   // do sub to process potential pipe entry
                    writeData(Player_Y_Speed, 0x00);     // initialize vertical speed and fractional
                    writeData(Player_Y_MoveForce, 0x00); // movement force to stop player's vertical movement
                    writeData(StompChainCounter, 0x00);  // initialize enemy stomp counter
                } // InitSteP
                writeData(Player_State, 0x00); // set player's state to normal
            }
        }
    }

    // DoPlayerSideCheck: get block buffer adder offset and increment it 2 bytes to use
    // adders for side collisions
    uint8_t sideAdder = M(0xeb) + 0x02;
    writeData(0x00, 0x02); // set value here to be used as counter

    do // SideCheckLoop
    {
        ++sideAdder;                // move onto the next one
        writeData(0xeb, sideAdder); // store it
        uint8_t playerY = M(Player_Y_Position);
        if (playerY >= 0x20)
        { // if player is in status bar area, skip this part
            if (playerY >= 0xe4)
            {
                return; // branch to leave if player is too far down
            }
            // do player-to-bg collision detection on one half of player
            const auto [sideMetatile, sideNybble] = BlockBufferColli_Side(sideAdder);
            if (sideMetatile != 0            // branch ahead if nothing found
                && sideMetatile != 0x1c      // if collided with sideways pipe (top), branch ahead
                && sideMetatile != 0x6b      // if collided with water pipe (top), branch ahead
                && !CheckForClimbMTiles(sideMetatile)) // or if player bumped into something climbable
            {
                checkSideMTiles(sideMetatile, sideNybble);
                return;
            }
        }
        // BHalf: load block adder offset and increment it
        const uint8_t otherHalfAdder = M(0xeb) + 0x01;
        playerY = M(Player_Y_Position); // get player's vertical position
        if (playerY < 0x08 || playerY >= 0xd0)
        {
            return; // if too high or too low, branch to leave
        }
        // do player-to-bg collision detection on other half of player
        const auto [otherHalfMetatile, otherHalfNybble] = BlockBufferColli_Side(otherHalfAdder);
        if (otherHalfMetatile != 0)
        { // if something found, branch
            checkSideMTiles(otherHalfMetatile, otherHalfNybble);
            return;
        }
        ++sideAdder; // (the original's Y register held the other half's offset here)
        --M(0x00);   // otherwise decrement counter
    } while (M(0x00) != 0); // run code until both sides of player are checked

    // ExSCH: leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerCtrlRoutine()
{
    // check task here
    if (M(GameEngineSubroutine) != 0x0b)
    {
        // are we in a water type area?
        if (M(AreaType) == 0)
        { // if not, branch
            // if not in the vertical area between status bar and bottom, branch
            if (M(Player_Y_HighPos) != 0x01 || M(Player_Y_Position) >= 0xd0)
            { // DisJoyp: disable controller bits
                writeData(SavedJoypadBits, 0x00);
            }
        }
        // SaveJoyp: otherwise store A and B buttons in $0a
        writeData(A_B_Buttons, M(SavedJoypadBits) & 0b11000000);
        // store left and right buttons in $0c
        writeData(Left_Right_Buttons, M(SavedJoypadBits) & 0b00000011);
        // store up and down buttons in $0b
        const uint8_t upDownButtons = M(SavedJoypadBits) & 0b00001100;
        writeData(Up_Down_Buttons, upDownButtons);
        // check for pressing down while on the ground with left or right also pressed
        if ((upDownButtons & 0b00000100) != 0 && M(Player_State) == 0 && M(Left_Right_Buttons) != 0)
        {
            writeData(Left_Right_Buttons, 0x00); // if pressing down while on the ground,
            writeData(Up_Down_Buttons, 0x00);    // nullify directional bits
        }
    }

    // SizeChk: run movement subroutines
    PlayerMovementSubs();
    uint8_t boundBoxCtrl = 0x01; // is player small?
    if (M(PlayerSize) == 0)
    {
        boundBoxCtrl = 0x00; // check for if crouching
        if (M(CrouchingFlag) != 0)
        {                        // if not, branch ahead
            boundBoxCtrl = 0x02; // if big and crouching, load 2
        }
    }
    // ChkMoveDir: set as player's bounding box size control
    writeData(Player_BoundBoxCtrl, boundBoxCtrl);
    const uint8_t xSpeed = M(Player_X_Speed); // check player's horizontal speed
    if (xSpeed != 0)
    {                             // if not moving at all horizontally, skip this part
        uint8_t movingDir = 0x01; // set moving direction to right by default
        if ((xSpeed & 0x80) != 0)
        {                     // if moving to the right, use default moving direction
            movingDir = 0x02; // otherwise change to move to the left
        } // SetMoveDir: set moving direction
        writeData(Player_MovingDir, movingDir);
    } // PlayerSubs: move the screen if necessary
    ScrollHandler();
    GetPlayerOffscreenBits();    // get player's offscreen bits
    RelativePlayerPosition();    // get coordinates relative to the screen
    BoundingBoxCore(0x00, 0x00); // get player's bounding box coordinates (offsets for player object)
    PlayerBGCollision();         // do collision detection and process
    if (M(Player_Y_Position) >= 0x40)
    { // if not that far down, branch ahead to PlayerHole
        const uint8_t task = M(GameEngineSubroutine);
        if (task != 0x05 && task != 0x07 && task >= 0x04)
        {
            writeData(Player_SprAttrib, M(Player_SprAttrib) & 0b11011111); // otherwise nullify player's background priority flag
        }
    }
    // PlayerHole: check player's vertical high byte
    const uint8_t vertHigh = M(Player_Y_HighPos);
    if (((vertHigh - 0x02) & 0x80) != 0)
    {
        return; // branch to leave if not that far down
    }
    writeData(ScrollLock, 0x01); // set scroll lock
    writeData(0x07, 0x04);       // set value here
    bool playerDies = false;     // used as flag, clear for cloud level
    // check game timer expiration flag; if set, branch;
    // also check for cloud type override, and skip to last part if found
    if (M(GameTimerExpiredFlag) != 0 || M(CloudTypeOverride) == 0)
    { // HoleDie: set flag for player death
        playerDies = true;
        if (M(GameEngineSubroutine) != 0x0b)
        { // if set to run the cloud level routine, branch ahead
            if (M(DeathMusicLoaded) == 0)
            {                                     // if already set, branch to next part
                writeData(EventMusicQueue, 0x01); // otherwise play death music
                writeData(DeathMusicLoaded, 0x01); // and set value here
            } // HoleBottom
            writeData(0x07, 0x06); // change value here
        }
    }
    // ChkHoleX: compare vertical high byte with value set here
    if (((vertHigh - M(0x07)) & 0x80) != 0)
    {
        return; // if less, branch to leave
    }
    if (playerDies)
    { // if the death flag was set, branch to set modes and other values
        if (M(EventMusicBuffer) != 0)
        {           // check to see if music is still playing
            return; // branch to leave if so
        }
        writeData(GameEngineSubroutine, 0x06); // otherwise set to run lose life routine on next frame

        return; // ExitCtrl: leave

        //------------------------------------------------------------------------
    } // CloudExit
    writeData(JoypadOverride, 0x00); // clear controller override bits if any are set
    SetEntr();                       // do sub to set secondary mode
    ++M(AltEntranceControl);         // set mode of entry to 3
}
