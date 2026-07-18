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

    uint32_t wide = 0;

    // check player state
    if (M(Player_State) == 0x03)
    { // if not climbing, branch
        y = 0x00;
        // get controller bits for up/down
        a = M(Up_Down_Buttons) & M(Player_CollisionBits); // check against player's collision detection bits
        if (a == 0)
        {
            goto ProcClimb; // if not pressing up or down, branch
        }
        y = 0x01;
        a &= 0b00001000; // check for pressing up
        if (a != 0)
        {
            goto ProcClimb;
        }
        y = 0x02;

    ProcClimb:                                                     // load value here
        writeData(Player_Y_MoveForce, Climb_Y_MForceData_data[y]); // store as vertical movement force
        a = 0x08;                                                  // load default animation timing
        x = Climb_Y_SpeedData_data[y];                             // load some other value here
        writeData(Player_Y_Speed, x);                              // store as vertical speed
        if ((x & 0x80) == 0)
        {             // if climbing down, use default animation timing value
            a = 0x04; // otherwise divide timer setting by 2
        } // SetCAnim: store animation timer setting and leave
        writeData(PlayerAnimTimerSet, a);
        return;

        //------------------------------------------------------------------------
    } // CheckForJumping
    // if jumpspring animating,
    if (M(JumpspringAnimCtrl) != 0)
    {
        goto NoJump; // skip ahead to something else
    }
    // check for A button press
    a = M(A_B_Buttons) & A_Button;
    if (a == 0)
    {
        goto NoJump; // if not, branch to something else
    }
    a &= M(PreviousA_B_Buttons); // if button not pressed in previous frame, branch
    if (a != 0)
    {

    NoJump: // otherwise, jump to something else
        goto X_Physics;
    } // ProcJumping
    // check player state
    if (M(Player_State) == 0)
    {
        goto InitJS; // if on the ground, branch
    }
    // if swimming flag not set, jump to do something else
    if (M(SwimmingFlag) == 0)
    {
        goto NoJump; // to prevent midair jumping, otherwise continue
    }
    // if jump/swim timer nonzero, branch
    if (M(JumpSwimTimer) != 0)
    {
        goto InitJS;
    }
    // check player's vertical speed
    if ((M(Player_Y_Speed) & 0x80) == 0)
    {
        goto InitJS; // if player's vertical speed motionless or down, branch
    }
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
    {
        goto ChkWtr; // branch if below certain values, increment Y
    }
    y = 0x01; // for each amount equal or exceeded
    if (a < 0x10)
    {
        goto ChkWtr;
    }
    y = 0x02;
    if (a < 0x19)
    {
        goto ChkWtr;
    }
    y = 0x03;
    if (a < 0x1c)
    {
        goto ChkWtr; // note that for jumping, range is 0-4 for Y
    }
    y = 0x04;

ChkWtr: // set value here (apparently always set to 1)
    writeData(DiffToHaltJump, 0x01);
    // if swimming flag disabled, branch
    if (M(SwimmingFlag) == 0)
    {
        goto GetYPhy;
    }
    y = 0x05; // otherwise set Y to 5, range is 5-6
    // if whirlpool flag not set, branch
    if (M(Whirlpool_Flag) == 0)
    {
        goto GetYPhy;
    }
    y = 0x06; // otherwise increment to 6

GetYPhy:                                              // store appropriate jump/swim
    writeData(VerticalForce, JumpMForceData_data[y]); // data here
    writeData(VerticalForceDown, FallMForceData_data[y]);
    writeData(Player_Y_MoveForce, InitMForceData_data[y]);
    writeData(Player_Y_Speed, PlayerYSpdData_data[y]);
    // if swimming flag disabled, branch
    if (M(SwimmingFlag) != 0)
    {
        // load swim/goomba stomp sound into
        writeData(Square1SoundQueue, Sfx_EnemyStomp); // square 1's sfx queue
        if (M(Player_Y_Position) >= 0x14)
        {
            goto X_Physics; // if below a certain point, branch
        }
        a = 0x00;                        // otherwise reset player's vertical speed
        writeData(Player_Y_Speed, 0x00); // and jump to something else to keep player
        goto X_Physics;                  // from swimming above water level
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
        {
            goto GetXPhy; // if =>$19 branch here
        }
        if (a < 0x19)
        {
            goto ChkRFast; // if not branch elsewhere
        }
    } // ProcPRun: if mario on the ground, increment Y
    y = 0x01;
    // check area type
    if (M(AreaType) == 0)
    {
        goto ChkRFast; // if water type, branch
    }
    y = 0x00; // decrement Y by default for non-water type area
    // get left/right controller bits
    if (M(Left_Right_Buttons) != M(Player_MovingDir))
    {
        goto ChkRFast; // if controller bits <> moving direction, skip this part
    }
    // check for b button pressed
    a = M(A_B_Buttons) & B_Button;
    if (a == 0)
    { // if pressed, skip ahead to set timer
        // check for running timer set
        if (M(RunningTimer) != 0)
        {
            goto GetXPhy; // if set, branch
        }

    ChkRFast: // if running timer not set or level type is water,
        ++y;
        ++M(0x00); // increment Y again and temp variable in memory
        if (M(RunningSpeed) == 0)
        { // if running speed set here, branch
            if (M(Player_XSpeedAbsolute) < 0x21)
            {
                goto GetXPhy; // if less than a certain amount, branch ahead
            }
        } // FastXSp: if running speed set or speed => $21 increment $00
        ++M(0x00);
        goto GetXPhy; // and jump ahead
    } // SetRTmr: if b button pressed, set running timer
    a = 0x0a;
    writeData(RunningTimer, 0x0a);

GetXPhy: // get maximum speed to the left
    writeData(MaximumLeftSpeed, MaxLeftXSpdData_data[y]);
    // check for specific routine running
    if (M(GameEngineSubroutine) == 0x07)
    {             // if not running, skip and use old value of Y
        y = 0x03; // otherwise set Y to 3
    } // GetXPhy2: get maximum speed to the right
    writeData(MaximumRightSpeed, MaxRightXSpdData_data[y]);
    y = M(0x00); // get other value in memory
    // get value using value in memory as offset
    writeData(FrictionAdderLow, FrictionData_data[y]);
    writeData(FrictionAdderHigh, 0x00); // init something here
    a = M(PlayerFacingDir);
    if (a != M(Player_MovingDir))
    {                                                                    // if the same, branch to leave
        wide = ((M(FrictionAdderHigh) << 8) | M(FrictionAdderLow)) << 1; // otherwise double the 16-bit friction adder
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

    y = 0x00;                     // initialize offset in Y
    a = M(Player_XSpeedAbsolute); // check player's walking/running speed
    if (a < 0x1c)
    {             // if greater than a certain amount, branch ahead
        y = 0x01; // otherwise increment Y
        if (a < 0x0e)
        {             // if greater than this but not greater than first, skip increment
            y = 0x02; // otherwise increment Y again
        } // ChkSkid: get controller bits
        a = M(SavedJoypadBits) & 0b01111111; // mask out A button
        if (a == 0)
        {
            goto SetAnimSpd; // if no other buttons pressed, branch ahead of all this
        }
        a &= 0x03; // mask out all others except left and right
        if (a != M(Player_MovingDir))
        {
            goto ProcSkid; // if left/right controller bits <> moving direction, branch
        }
        a = 0x00; // otherwise set zero value here
    } // SetRunSpd: store zero or running speed here
    writeData(RunningSpeed, a);
    goto SetAnimSpd;

ProcSkid: // check player's walking/running speed
    if (M(Player_XSpeedAbsolute) >= 0x0b)
    {
        goto SetAnimSpd; // if greater than this amount, branch
    }
    writeData(Player_MovingDir, M(PlayerFacingDir)); // otherwise use facing direction to set moving direction
    a = 0x00;
    writeData(Player_X_Speed, 0x00);     // nullify player's horizontal speed
    writeData(Player_X_MoveForce, 0x00); // and dummy variable for player

SetAnimSpd: // get animation timer setting using Y as offset
    a = PlayerAnimTmrData_data[y];
    writeData(PlayerAnimTimerSet, a);
}

//------------------------------------------------------------------------

// Inputs: leftRightButtons = left/right controller bits (Left_Right_Buttons)
// Outputs: none beyond the Player_XSpeedAbsolute memory write (final a is scratch to the caller)
void SMBEngine::ImposeFriction(uint8_t leftRightButtons)
{
    bool shiftedBit = false;
    uint32_t wide = 0;

    a = leftRightButtons & M(Player_CollisionBits); // perform AND between left/right controller bits and collision flag
    if (a == 0x00)
    { // if any bits set, branch to next part
        a = M(Player_X_Speed);
        if (a == 0)
        {
            goto SetAbsSpd; // if player has no horizontal speed, branch ahead to last part
        }
        if ((a & 0x80) == 0)
        {
            goto RghtFrict; // if player moving to the right, branch to slow
        }
        if ((a & 0x80) != 0)
        {
            goto LeftFrict; // otherwise logic dictates player moving left, branch to slow
        }
    } // JoypFrict: take the right controller bit
    shiftedBit = (a & 0x01) != 0;
    a >>= 1;
    if (!shiftedBit)
    {
        goto RghtFrict; // if left button pressed, thus branch
    }

LeftFrict: // load value set here
    // speed:force is one 16-bit quantity, and so is the friction adder
    wide = ((M(Player_X_Speed) << 8) | M(Player_X_MoveForce)) +
           ((M(FrictionAdderHigh) << 8) | M(FrictionAdderLow)); // add to it another value set here
    writeData(Player_X_MoveForce, LOBYTE(wide));                // store here
    writeData(Player_X_Speed, HIBYTE(wide));                    // set as new horizontal speed
    a = HIBYTE(wide);
    if (((a - M(MaximumRightSpeed)) & 0x80) != 0)
    {
        goto XSpdSign; // if horizontal speed greater negatively, branch
    }
    a = M(MaximumRightSpeed);     // otherwise set preset value as horizontal speed
    writeData(Player_X_Speed, a); // thus slowing the player's left movement down
    goto SetAbsSpd;               // skip to the end

RghtFrict: // load value set here
    // speed:force is one 16-bit quantity, and so is the friction adder
    wide = ((M(Player_X_Speed) << 8) | M(Player_X_MoveForce)) -
           ((M(FrictionAdderHigh) << 8) | M(FrictionAdderLow)); // subtract from it another value set here
    writeData(Player_X_MoveForce, LOBYTE(wide));                // store here
    writeData(Player_X_Speed, HIBYTE(wide));                    // set as new horizontal speed
    a = HIBYTE(wide);
    if (((a - M(MaximumLeftSpeed)) & 0x80) == 0)
    {
        goto XSpdSign; // if horizontal speed greater positively, branch
    }
    a = M(MaximumLeftSpeed);      // otherwise set preset value as horizontal speed
    writeData(Player_X_Speed, a); // thus slowing the player's right movement down

XSpdSign: // if player not moving or moving to the right,
    if ((a & 0x80) == 0)
    {
        goto SetAbsSpd; // branch and leave horizontal speed value unmodified
    }
    a ^= 0xff;
    a += 0x01; // unsigned walking/running speed

SetAbsSpd: // store walking/running speed here and leave
    writeData(Player_XSpeedAbsolute, a);
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: pair of {carry flag (used by CoinBlock to adjust a subtraction by one), misc object
// buffer slot found}
std::pair<bool, uint8_t> SMBEngine::FindEmptyMiscSlot()
{
    bool miscSlotSearched = false;

    y = 0x08;                 // start at end of misc objects buffer
    miscSlotSearched = false; // no compare done yet

FMiscLoop: // get misc object state
    a = M(Misc_State + y);
    if (a != 0)
    {                            // branch if none found to use current offset
        --y;                     // decrement offset
        miscSlotSearched = true; // the offset never falls below five, so this sets the carry
        if (y != 0x05)
        {
            goto FMiscLoop; // do this until all slots are checked
        }
        y = 0x08; // if no empty slots found, use last slot
    } // UseMiscS: store offset of misc object buffer here (residual)
    writeData(JumpCoinMiscOffset, y);
    return {miscSlotSearched, y};
}

//------------------------------------------------------------------------

// Inputs: metatile = metatile value to compare against BrickQBlockMetatiles
// Outputs: pair of {whether a match was found, matching table index (valid when a match was
// found)}
std::pair<bool, uint8_t> SMBEngine::BlockBumpedChk(uint8_t metatile)
{
    bool bumpedBlockFound = false;

    y = 0x0d; // start at end of metatile data

BumpChkLoop: // check to see if current metatile matches
    if (metatile != M(BrickQBlockMetatiles + y))
    {        // metatile found in block buffer, branch if so
        --y; // otherwise move onto next metatile
        if ((y & 0x80) == 0)
        {
            goto BumpChkLoop; // do this until all metatiles are checked
        }
        bumpedBlockFound = false; // if none match
    }
    else
    { // MatchBump
        bumpedBlockFound = true;
    }
    return {bumpedBlockFound, y};
}

//------------------------------------------------------------------------

// Inputs: blockOffset = block object buffer offset
// Outputs: none
void SMBEngine::SpawnBrickChunks(uint8_t blockOffset)
{
    x = blockOffset;
    // set horizontal coordinate of block object
    writeData(Block_Orig_XPos + x, M(Block_X_Position + x)); // as original horizontal coordinate here
    writeData(Block_X_Speed + x, 0xf0);                      // set horizontal speed for brick chunk objects
    writeData(Block_X_Speed + 2 + x, 0xf0);
    writeData(Block_Y_Speed + x, 0xfa);     // set vertical speed for one
    writeData(Block_Y_Speed + 2 + x, 0xfc); // set lower vertical speed for the other
    writeData(Block_Y_MoveForce + x, 0x00); // init fractional movement force for both
    writeData(Block_Y_MoveForce + 2 + x, 0x00);
    writeData(Block_PageLoc + 2 + x, M(Block_PageLoc + x));       // copy page location
    writeData(Block_X_Position + 2 + x, M(Block_X_Position + x)); // copy horizontal coordinate
    a = M(Block_Y_Position + x);
    a += 0x08; // and save as vertical coordinate for one of them
    writeData(Block_Y_Position + 2 + x, a);
    a = 0xfa;
    writeData(Block_Y_Speed + x, 0xfa); // set vertical speed...again??? (redundant)
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
    // check saved controller bits from earlier
    a = M(Up_Down_Buttons) & 0b00000100; // for pressing down
    if (a == 0)
    {
        return; // if not pressing down, branch to leave
    }
    a = M(0x00);
    if (a != 0x11)
    {
        return; // branch to leave if not found
    }
    a = M(0x01);
    if (a != 0x10)
    {
        return; // branch to leave if not found
    }
    writeData(ChangeAreaTimer, 0x30);                  // set timer for change of area
    writeData(GameEngineSubroutine, 0x03);             // set to run vertical pipe entry routine on next frame
    writeData(Square1SoundQueue, Sfx_PipeDown_Injury); // load pipedown/injury sound
    writeData(Player_SprAttrib, 0b00100000);           // set background priority bit in player's attributes
    a = M(WarpZoneControl);                            // check warp zone control
    if (a == 0)
    {
        return; // branch to leave if none found
    }
    a &= 0b00000011; // mask out all but 2 LSB
    a <<= 1;
    a <<= 1;                  // multiply by four
    x = a;                    // save as offset to warp zone numbers (starts at left pipe)
    a = M(Player_X_Position); // get player's horizontal position
    if (a < 0x60)
    {
        goto GetWNum; // if player at left, not near middle, use offset and skip ahead
    }
    ++x; // otherwise increment for middle pipe
    if (a < 0xa0)
    {
        goto GetWNum; // if player at middle, but not too far right, use offset and skip
    }
    ++x; // otherwise increment for last pipe

GetWNum: // get warp zone numbers
    y = M(WarpZoneNumbers + x);
    --y;                         // decrement for use as world number
    writeData(WorldNumber, y);   // store as world number and offset
    x = M(WorldAddrOffsets + y); // get offset to where this world's area offsets are
    // get area offset based on world offset
    writeData(AreaPointer, M(AreaAddrOffsets + x)); // store area offset here to be used to change areas
    writeData(EventMusicQueue, Silence);            // silence music
    a = 0x00;
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

    bool shiftedBit = false;
    uint32_t wide = 0;

    y = 0x00; // set default adder here
    // get player's vertical speed
    if ((M(Player_Y_Speed) & 0x80) != 0)
    {             // if not moving upwards, branch
        y = 0xff; // otherwise set adder to $ff
    } // MoveOnVine: store adder here
    writeData(0x00, y);
    // highpos:position:dummy is one 24-bit quantity, and $00:speed the signed
    // 16-bit amount to move the player up or down by
    wide = (M(Player_Y_HighPos) << 16) | (M(Player_Y_Position) << 8) | M(Player_YMF_Dummy);
    wide += (M(0x00) << 16) | (M(Player_Y_Speed) << 8) | M(Player_Y_MoveForce);
    writeData(Player_YMF_Dummy, LOBYTE(wide));          // add movement force to dummy variable
    writeData(Player_Y_Position, HIBYTE(wide));         // and store to move player up or down
    writeData(Player_Y_HighPos, (uint8_t)(wide >> 16)); // and store
    a = (uint8_t)(wide >> 16);
    // compare left/right controller bits
    a = M(Left_Right_Buttons) & M(Player_CollisionBits); // to collision flag
    if (a != 0)
    {                          // if not set, skip to end
        y = M(ClimbSideTimer); // otherwise check timer
        if (y == 0)
        {                                    // if timer not expired, branch to leave
            writeData(ClimbSideTimer, 0x18); // otherwise set timer now
            x = 0x00;                        // set default offset here
            y = M(PlayerFacingDir);          // get facing direction
            shiftedBit = (a & 0x01) != 0;
            if (!shiftedBit) // check the right button controller bit
            {                // if controller right pressed, branch ahead
                x = 0x02;    // otherwise increment offset by 2 bytes
            } // ClimbFD: check to see if facing right
            --y;
            if (y != 0)
            {        // if so, branch, do not increment
                ++x; // otherwise increment by 1 byte
            } // CSetFDir
            // add to or subtract from the player's 16-bit horizontal position, using the
            // 16-bit value here as the adder and X as offset
            wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + ((ClimbAdderHigh_data[x] << 8) | ClimbAdderLow_data[x]);
            writeData(Player_X_Position, LOBYTE(wide));
            writeData(Player_PageLoc, HIBYTE(wide));
            a = HIBYTE(wide);
            // get left/right controller bits again
            a = M(Left_Right_Buttons) ^ 0b00000011; // invert them and store them while player
            writeData(PlayerFacingDir, a);          // is on vine to face player in opposite direction
        } // ExitCSub: then leave
        return;

        //------------------------------------------------------------------------
    } // InitCSTimer: initialize timer here
    writeData(ClimbSideTimer, a);
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
    PutBlockMetatile(metatileSel, areaType, 0x41); // low byte set so offset points to $0341
    writeData(VRAM_Buffer_AddrCtrl, 0x06);         // set vram address controller to $0341 and leave
}

//------------------------------------------------------------------------

// Inputs: controlBit = control bit/offset (forwarded to WriteBlockMetatile)
// Outputs: none
void SMBEngine::DestroyBlockMetatile(uint8_t controlBit)
{
    // force blank metatile if branched/jumped to this point
    WriteBlockMetatile(0x00, controlBit);
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
// Outputs: the metatile found (see BlockBufferCollision)
uint8_t SMBEngine::BlockBufferColli_Feet(uint8_t adderBaseOffset)
{
    // if branched here, increment to next set of adders
    return BlockBufferColli_Head(adderBaseOffset + 1);
}

//------------------------------------------------------------------------

// Inputs: adderOffset = block buffer adder offset (forwarded to Skip_9/BlockBufferCollision)
// Outputs: the metatile found (see BlockBufferCollision)
uint8_t SMBEngine::BlockBufferColli_Head(uint8_t adderOffset)
{
    y = adderOffset; // transition: register-based callers read member y after the call
    // set flag to return vertical coordinate
    return Skip_9(0x00, adderOffset);
}

//------------------------------------------------------------------------

// Inputs: adderOffset = block buffer adder offset (forwarded to Skip_9/BlockBufferCollision)
// Outputs: the metatile found (see BlockBufferCollision)
uint8_t SMBEngine::BlockBufferColli_Side(uint8_t adderOffset)
{
    y = adderOffset; // transition: register-based callers read member y after the call
    // set flag to return horizontal coordinate
    return Skip_9(0x01, adderOffset);
}

//------------------------------------------------------------------------

// Inputs: coordSelector = which coordinate's low nybble to report; cornerIdx = corner-selector
// index (both forwarded to BlockBufferCollision)
// Outputs: the metatile found (see BlockBufferCollision)
uint8_t SMBEngine::Skip_9(uint8_t coordSelector, uint8_t cornerIdx)
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
    writeData(0x00, M(VerticalForce));
    // set maximum vertical speed, use zero for player offset
    ImposeGravitySprObj(0x04, 0x00); // then jump to move player vertically
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
    a = 0x00; // set A to init crouch flag by default
    // is player small?
    if (M(PlayerSize) == 0)
    { // if so, branch
        // check state of player
        if (M(Player_State) != 0)
        {
            goto ProcMove; // if not on the ground, branch
        }
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
    goto LRAir;                                     // movement force, then skip ahead to process left/right movement

JumpSwimSub:
    y = M(Player_Y_Speed); // if player's vertical speed zero
    if ((y & 0x80) != 0)
    {                                  // or moving downwards, branch to falling
        a = M(A_B_Buttons) & A_Button; // check to see if A button is being pressed
        a &= M(PreviousA_B_Buttons);   // and was pressed in previous frame
        if (a != 0)
        {
            goto ProcSwim; // if so, branch elsewhere
        }
        a = M(JumpOrigin_Y_Position); // get vertical position player jumped from
        a -= M(Player_Y_Position);    // subtract current from original vertical coordinate
        if (a < M(DiffToHaltJump))
        {
            goto ProcSwim; // or just starting to jump, if just starting, skip ahead
        }
    } // DumpFall: otherwise dump falling into main fractional
    writeData(VerticalForce, M(VerticalForceDown));

ProcSwim: // if swimming flag not set,
    if (M(SwimmingFlag) == 0)
    {
        goto LRAir; // branch ahead to last part
    }
    GetPlayerAnimSpeed(); // do a sub to get animation frame timing
    if (M(Player_Y_Position) < 0x14)
    { // if not yet reached a certain position, branch ahead
        a = 0x18;
        writeData(VerticalForce, 0x18); // otherwise set fractional
    } // LRWater: check left/right controller bits (check for swimming)
    a = M(Left_Right_Buttons);
    if (a == 0)
    {
        goto LRAir; // if not pressing any, skip
    }
    writeData(PlayerFacingDir, a); // otherwise set facing direction accordingly

LRAir: // check left/right controller bits (check for jumping/falling)
    a = M(Left_Right_Buttons);
    if (a != 0)
    {                      // if not pressing any, skip
        ImposeFriction(a); // otherwise process horizontal movement
    } // JSMove: do a sub to move player horizontally
    a = MovePlayerHorizontally();
    writeData(Player_X_Scroll, a); // set player's speed here, to be used for scroll later
    if (M(GameEngineSubroutine) == 0x0b)
    { // branch if not set to run
        a = 0x28;
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
    a += M(ClimbXPosAdder - 1 + y);  // add pixels depending on facing direction
    writeData(Player_X_Position, a); // store as player's horizontal coordinate
    a = M(0x06);                     // get low byte of block buffer address again
    if (a == 0)
    {                                   // if not zero, branch
        a = M(ScreenRight_PageLoc);     // load page location of right side of screen
        a += M(ClimbPLocAdder - 1 + y); // add depending on facing location
        writeData(Player_PageLoc, a);   // store as player's page location
    } // ExPVne: finally, we're done!
}

//------------------------------------------------------------------------

// Inputs: collidedMetatile = the metatile the player's head collided with
// Outputs: none
void SMBEngine::PlayerHeadCollision(uint8_t collidedMetatile)
{
    const uint8_t BlockYPosAdderData_data[] = {0x04, 0x12};

    bool bumpedBlockFound = false;

    a = 0x11;                  // load unbreakable block object state by default
    x = M(SprDataOffset_Ctrl); // load offset control bit here
    // check player's size
    if (M(PlayerSize) == 0)
    {             // if small, branch
        a = 0x12; // otherwise load breakable block object state
    } // DBlockSte: store into block object buffer
    writeData(Block_State + x, a);
    DestroyBlockMetatile(x);           // store blank metatile in vram buffer to write to name table
    x = M(SprDataOffset_Ctrl);         // load offset control bit
    a = M(0x02);                       // get vertical high nybble offset used in block buffer routine
    writeData(Block_Orig_YPos + x, a); // set as vertical coordinate for block object
    y = a;
    // get low byte of block buffer address used in same routine
    writeData(Block_BBuf_Low + x, M(0x06));     // save as offset here to be used later
    a = M(W(0x06) + y);                         // get contents of block buffer at old address at $06, $07
    bumpedBlockFound = BlockBumpedChk(a).first; // do a sub to check which block player bumped head on
    writeData(0x00, a);                         // store metatile here
    y = M(PlayerSize);                          // check player's size
    if (y == 0)
    {          // if small, use metatile itself as contents of A
        a = y; // otherwise init A (note: big = 0)
    } // ChkBrick: if no match was found in previous sub, skip ahead
    if (!bumpedBlockFound)
    {
        goto PutMTileB;
    }
    // otherwise load unbreakable state into block object buffer
    writeData(Block_State + x, 0x11); // note this applies to both player sizes
    a = 0xc4;                         // load empty block metatile into A for now
    y = M(0x00);                      // get metatile from before
    if (y != 0x58)
    { // if so, branch
        if (y != 0x5d)
        {
            goto PutMTileB; // if not, branch ahead to store empty block metatile
        }
    } // StartBTmr: check brick coin timer flag
    if (M(BrickCoinTimerFlag) == 0)
    { // if set, timer expired or counting down, thus branch
        a = 0x0b;
        writeData(BrickCoinTimer, 0x0b); // if not set, set brick coin timer
        ++M(BrickCoinTimerFlag);         // and set flag linked to it
    } // ContBTmr: check brick coin timer
    if (M(BrickCoinTimer) == 0)
    {             // if not yet expired, branch to use current metatile
        y = 0xc4; // otherwise use empty block metatile
    } // PutOldMT: put metatile into A
    a = y;

PutMTileB: // store whatever metatile be appropriate here
    writeData(Block_Metatile + x, a);
    InitBlock_XY_Pos(x);               // get block object horizontal coordinates saved
    y = M(0x02);                       // get vertical high nybble offset
    writeData(W(0x06) + y, 0x23);      // write blank metatile $23 to block buffer
    writeData(BlockBounceTimer, 0x10); // set block bounce timer
    writeData(0x05, collidedMetatile); // save original metatile here
    y = 0x00;                          // set default offset
    // is player crouching?
    if (M(CrouchingFlag) == 0)
    { // if so, branch to increment offset
        // is player big?
        if (M(PlayerSize) == 0)
        {
            goto BigBP; // if so, branch to use default offset
        }
    } // SmallBP: increment for small or big and crouching
    y = 0x01;

BigBP: // get player's vertical coordinate
    a = M(Player_Y_Position);
    a += BlockYPosAdderData_data[y];    // add value determined by size
    a &= 0xf0;                          // mask out low nybble to get 16-pixel correspondence
    writeData(Block_Y_Position + x, a); // save as vertical coordinate for block object
    // get block object state
    if (M(Block_State + x) != 0x11)
    {                   // if set to value loaded for unbreakable, branch
        BrickShatter(); // execute code for breakable brick
    } // Unbreak: execute code for unbreakable brick or question block
    else // skip subroutine to do last part of code here
    {
        BumpBlock();
    } // InvOBit: invert control bit used by block objects
    a = M(SprDataOffset_Ctrl) ^ 0x01; // and floatey numbers
    writeData(SprDataOffset_Ctrl, a);
    // leave!
}

//------------------------------------------------------------------------

// Inputs: x = block object buffer offset
// Outputs: none
void SMBEngine::BumpBlock()
{
    bool bumpedBlockFound = false;

    x = CheckTopOfBlock();                  // check to see if there's a coin directly above this block
    writeData(Square1SoundQueue, Sfx_Bump); // play bump sound
    writeData(Block_X_Speed + x, 0x00);     // initialize horizontal speed for block object
    writeData(Block_Y_MoveForce + x, 0x00); // init fractional movement force
    writeData(Player_Y_Speed, 0x00);        // init player's vertical speed
    writeData(Block_Y_Speed + x, 0xfe);     // set vertical speed for block object
    a = M(0x05);                            // get original metatile from stack
    uint8_t blockIdx = 0;
    std::tie(bumpedBlockFound, blockIdx) = BlockBumpedChk(a); // do a sub to check which block player bumped head on
    if (!bumpedBlockFound)
    { // if no match was found, branch to leave
        return;
    }
    a = blockIdx; // move block number to A
    if (a >= 0x09)
    {              // branch to use current number
        a -= 0x05; // otherwise subtract 5 for second set to get proper number
    } // BlockCode: run appropriate subroutine depending on block number
    switch (a)
    {
    case 0:
        MushFlowerBlock(x);
        return;
    case 1:
        CoinBlock(x);
        return;
    case 2:
        CoinBlock(x);
        return;
    case 3:
        ExtraLifeMushBlock(x);
        return;
    case 4:
        MushFlowerBlock(x);
        return;
    case 5:
        VineBlock();
        return;
    case 6:
        StarBlock(x);
        return;
    case 7:
        CoinBlock(x);
        return;
    case 8:
        ExtraLifeMushBlock(x);
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
    x = 0x05;                  // load last slot for enemy object buffer
    y = M(SprDataOffset_Ctrl); // get control bit
    Setup_Vine(x, y);          // set up vine object
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

    bool climbMTileFound = false;
    bool coinMTileFound = false;
    bool jumpspringFound = false;
    bool solidMTileFound = false;

    a = M(DisableCollisionDet); // if collision detection disabled flag set,
    if (a != 0)
    {
        return; // branch to leave
    }
    a = M(GameEngineSubroutine);
    if (a == 0x0b)
    {
        return; // branch to leave
    }
    if (a < 0x04)
    {
        return; // if running routines $00-$03 branch to leave
    }
    a = 0x01;            // load default player state for swimming
    y = M(SwimmingFlag); // if swimming flag set,
    if (y == 0)
    {                        // branch ahead to set default state
        a = M(Player_State); // if player in normal state,
        if (a != 0)
        { // branch to set default state for falling
            if (a != 0x03)
            {
                goto ChkOnScr; // if in any other state besides climbing, skip to next part
            }
        } // SetFallS: load default player state for falling
        a = 0x02;
    } // SetPSte: set whatever player state is appropriate
    writeData(Player_State, a);

ChkOnScr:
    a = M(Player_Y_HighPos);
    if (a != 0x01)
    {
        return; // branch to leave if not
    }
    writeData(Player_CollisionBits, 0xff); // initialize player's collision flag
    a = M(Player_Y_Position);
    if (a >= 0xcf)
    { // if not too close to the bottom of screen, continue

        return; // ExPBGCol: otherwise leave

        //------------------------------------------------------------------------
    } // ChkCollSize
    y = 0x02; // load default offset
    if (M(CrouchingFlag) != 0)
    {
        goto GBBAdr; // if player crouching, skip ahead
    }
    if (M(PlayerSize) != 0)
    {
        goto GBBAdr; // if player small, skip ahead
    }
    y = 0x01; // otherwise decrement offset for big player not crouching
    if (M(SwimmingFlag) != 0)
    {
        goto GBBAdr; // if swimming flag set, skip ahead
    }
    y = 0x00; // otherwise decrement offset

GBBAdr: // get value using offset
    a = BlockBufferAdderData_data[y];
    writeData(0xeb, a); // store value here
    y = a;              // put value into Y, as offset for block buffer routine
    x = M(PlayerSize);  // get player's size as offset
    if (M(CrouchingFlag) != 0)
    {        // if player not crouching, branch ahead
        ++x; // otherwise increment size as offset
    } // HeadChk: get player's vertical coordinate
    if (M(Player_Y_Position) < PlayerBGUpperExtent_data[x])
    {
        goto DoFootCheck; // if player is too high, skip this part
    }
    a = BlockBufferColli_Head(y); // do player-to-bg collision detection on top of
    if (a == 0)
    {
        goto DoFootCheck; // player, and branch if nothing above player's head
    }
    coinMTileFound = CheckForCoinMTiles(a); // check to see if player touched coin with their head
    if (coinMTileFound)
    {
        goto AwardTouchedCoin; // if so, branch to some other part of code
    }
    // check player's vertical speed
    if ((M(Player_Y_Speed) & 0x80) == 0)
    {
        goto DoFootCheck; // if player not moving upwards, branch elsewhere
    }
    // check lower nybble of vertical coordinate returned
    if (M(0x04) < 0x04)
    {
        goto DoFootCheck; // if low nybble < 4, branch
    }
    solidMTileFound = CheckForSolidMTiles(a); // check to see what player's head bumped on
    if (!solidMTileFound)
    { // if player collided with solid metatile, branch
        // otherwise check area type
        if (M(AreaType) == 0)
        {
            goto NYSpd; // if water level, branch ahead
        }
        // if block bounce timer not expired,
        if (M(BlockBounceTimer) != 0)
        {
            goto NYSpd; // branch ahead, do not process collision
        }
        PlayerHeadCollision(a); // otherwise do a sub to process collision
        goto DoFootCheck;       // jump ahead to skip these other parts here
    } // SolidOrClimb
    if (a == 0x26)
    {
        goto NYSpd; // branch ahead and do not play sound
    }
    a = Sfx_Bump;
    writeData(Square1SoundQueue, Sfx_Bump); // otherwise load bump sound

NYSpd: // set player's vertical speed to nullify
    a = 0x01;
    writeData(Player_Y_Speed, 0x01); // jump or swim

DoFootCheck:
    y = M(0xeb); // get block buffer adder offset
    if (M(Player_Y_Position) >= 0xcf)
    {
        goto DoPlayerSideCheck; // if player is too far down on screen, skip all of this
    }
    a = BlockBufferColli_Feet(y);           // do player-to-bg collision detection on bottom left of player
    coinMTileFound = CheckForCoinMTiles(a); // check to see if player touched coin with their left foot
    if (coinMTileFound)
    {
        goto AwardTouchedCoin; // if so, branch to some other part of code
    }
    pha();                        // save bottom left metatile to stack
    a = BlockBufferColli_Feet(y); // do player-to-bg collision detection on bottom right of player
    writeData(0x00, a);           // save bottom right metatile here
    pla();
    writeData(0x01, a); // pull bottom left metatile and save here
    if (a != 0)
    {
        goto ChkFootMTile; // if anything here, skip this part
    }
    a = M(0x00); // otherwise check for anything in bottom right metatile
    if (a == 0)
    {
        goto DoPlayerSideCheck; // and skip ahead if not
    }
    coinMTileFound = CheckForCoinMTiles(a); // check to see if player touched coin with their right foot
    if (!coinMTileFound)
    {
        goto ChkFootMTile; // if not, skip unconditional jump and continue code
    }

AwardTouchedCoin:
    HandleCoinMetatile(); // follow the code to erase coin and award to player 1 coin
    return;

ChkFootMTile:
    climbMTileFound = CheckForClimbMTiles(a); // check to see if player landed on climbable metatiles
    if (climbMTileFound)
    {
        goto DoPlayerSideCheck; // if so, branch
    }
    y = M(Player_Y_Speed); // check player's vertical speed
    if ((y & 0x80) != 0)
    {
        goto DoPlayerSideCheck; // if player moving upwards, branch
    }
    if (a == 0xc5)
    { // if player did not touch axe, skip ahead
    } // ContChk: do sub to check for hidden coin or 1-up blocks
    else // otherwise jump to set modes of operation
    {
        if (ChkInvisibleMTiles(a))
        {
            goto DoPlayerSideCheck; // if either found, branch
        }
        // if jumpspring animating right now,
        if (M(JumpspringAnimCtrl) == 0)
        {                // branch ahead
            y = M(0x04); // check lower nybble of vertical coordinate returned
            if (y >= 0x05)
            {                                         // if lower nybble < 5, branch
                writeData(0x00, M(Player_MovingDir)); // use player's moving direction as temp variable
                ImpedePlayerMove();                   // jump to impede player's movement in that direction
                return;
            } // LandPlyr: do sub to check for jumpspring metatiles and deal with it
            ChkForLandJumpSpring(a);
            a = 0xf0;
            a &= M(Player_Y_Position);       // mask out lower nybble of player's vertical position
            writeData(Player_Y_Position, a); // and store as new vertical position to land player properly
            HandlePipeEntry();               // do sub to process potential pipe entry
            a = 0x00;
            writeData(Player_Y_Speed, 0x00);     // initialize vertical speed and fractional
            writeData(Player_Y_MoveForce, 0x00); // movement force to stop player's vertical movement
            writeData(StompChainCounter, 0x00);  // initialize enemy stomp counter
        } // InitSteP
        a = 0x00;
        writeData(Player_State, 0x00); // set player's state to normal

    DoPlayerSideCheck:
        y = M(0xeb); // get block buffer adder offset
        ++y;
        ++y;      // increment offset 2 bytes to use adders for side collisions
        a = 0x02; // set value here to be used as counter
        writeData(0x00, 0x02);

        do // SideCheckLoop
        {
            ++y;                // move onto the next one
            writeData(0xeb, y); // store it
            a = M(Player_Y_Position);
            if (a < 0x20)
            {
                goto BHalf; // if player is in status bar area, branch ahead to skip this part
            }
            if (a >= 0xe4)
            {
                return; // branch to leave if player is too far down
            }
            a = BlockBufferColli_Side(y); // do player-to-bg collision detection on one half of player
            if (a == 0)
            {
                goto BHalf; // branch ahead if nothing found
            }
            if (a == 0x1c)
            {
                goto BHalf; // if collided with sideways pipe (top), branch ahead
            }
            if (a == 0x6b)
            {
                goto BHalf; // if collided with water pipe (top), branch ahead
            }
            climbMTileFound = CheckForClimbMTiles(a); // do sub to see if player bumped into anything climbable
            if (!climbMTileFound)
            {
                goto CheckSideMTiles; // if not, branch to alternate section of code
            }

        BHalf: // load block adder offset
            y = M(0xeb);
            ++y;                      // increment it
            a = M(Player_Y_Position); // get player's vertical position
            if (a < 0x08)
            {
                return; // if too high, branch to leave
            }
            if (a >= 0xd0)
            {
                return; // if too low, branch to leave
            }
            a = BlockBufferColli_Side(y); // do player-to-bg collision detection on other half of player
            if (a != 0)
            {
                goto CheckSideMTiles; // if something found, branch
            }
            --M(0x00); // otherwise decrement counter
        } while (M(0x00) != 0); // run code until both sides of player are checked

        return; // ExSCH: leave

        //------------------------------------------------------------------------

    CheckSideMTiles:
        if (ChkInvisibleMTiles(a))
        {           // check for hidden or coin 1-up blocks
            return; // branch to leave if either found
        }
        climbMTileFound = CheckForClimbMTiles(a); // check for climbable metatiles
        if (climbMTileFound)
        {                        // if not found, skip and continue with code
            goto HandleClimbing; // otherwise jump to handle climbing
        } // ContSChk: check to see if player touched coin
        coinMTileFound = CheckForCoinMTiles(a);
        if (coinMTileFound)
        {
            HandleCoinMetatile(); // if so, execute code to erase coin and award to player 1 coin
            return;
        }
        jumpspringFound = ChkJumpspringMetatiles(a); // check for jumpspring metatiles
        if (jumpspringFound)
        {                              // if not found, branch ahead to continue cude
            a = M(JumpspringAnimCtrl); // otherwise check jumpspring animation control
            if (a != 0)
            {
                return; // branch to leave if set
            }
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
        writeData(Player_SprAttrib, a);        // set background priority bit in player attributes
        a = M(Player_X_Position) & 0b00001111; // get lower nybble of player's horizontal coordinate
        if (a != 0)
        {             // if at zero, branch ahead to skip this part
            y = 0x00; // set default offset for timer setting data
            // load page location for left side of screen
            if (M(ScreenLeft_PageLoc) != 0)
            {             // if at page zero, use default offset
                y = 0x01; // otherwise increment offset
            } // SetCATmr: set timer for change of area as appropriate
            writeData(ChangeAreaTimer, AreaChangeTimerData_data[y]);
        } // ChkGERtn: get number of game engine routine running
        a = M(GameEngineSubroutine);
        if (a == 0x07)
        {
            return; // if running player entrance routine or
        }
        if (a != 0x08)
        {
            return;
        }
        a = 0x02;
        writeData(GameEngineSubroutine, 0x02); // otherwise set sideways pipe entry routine to run
        return;                                // and leave

    } // HandleAxeMetatile
    writeData(OperMode_Task, 0x00); // reset secondary mode
    writeData(OperMode, 0x02);      // set primary mode to autoctrl mode
    a = 0x18;
    writeData(Player_X_Speed, 0x18); // set horizontal speed and continue to erase axe metatile

    ErACM();
    return;

HandleClimbing:
    y = M(0x04); // check low nybble of horizontal coordinate returned from
    if (y >= 0x06)
    { // makes actual physical part of vine or flagpole thinner
        if (y < 0x0a)
        {
            goto ChkForFlagpole;
        }
    } // ExHC: leave if too far left or too far right
    return;

    //------------------------------------------------------------------------

ChkForFlagpole:
    if (a != 0x24)
    { // branch if flagpole ball found
        if (a != 0x25)
        {
            goto VineCollision; // branch to alternate code if flagpole shaft not found
        }
    } // FlagpoleCollision
    if (M(GameEngineSubroutine) == 0x05)
    {
        PutPlayerOnVine(); // if running, branch to end of climbing code
        return;
    }
    writeData(PlayerFacingDir, 0x01); // set player's facing direction to right
    ++M(ScrollLock);                  // set scroll lock flag
    if (M(GameEngineSubroutine) != 0x04)
    {                                        // if running, branch to end of flagpole code here
        a = BulletBill_CannonVar;            // load identifier for bullet bills (cannon variant)
        KillEnemies(a);                      // get rid of them
        writeData(EventMusicQueue, Silence); // silence music
        writeData(FlagpoleSoundQueue, 0x40); // load flagpole sound into flagpole sound queue
        x = 0x04;                            // start at end of vertical coordinate data
        a = M(Player_Y_Position);
        writeData(FlagpoleCollisionYPos, a); // store player's vertical coordinate here to be used later

    ChkFlagpoleYPosLoop:
        if (a < FlagpoleYPosData_data[x])
        {        // if player's => current, branch to use current offset
            --x; // otherwise decrement offset to use
            if (x != 0)
            {
                goto ChkFlagpoleYPosLoop; // do this until all data is checked (use last one if all checked)
            }
        } // MtchF: store offset here to be used later
        writeData(FlagpoleScore, x);
    } // RunFR
    a = 0x04;
    writeData(GameEngineSubroutine, 0x04); // set value to run flagpole slide routine
    PutPlayerOnVine();                     // jump to end of climbing code
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
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerCtrlRoutine()
{
    // check task here
    if (M(GameEngineSubroutine) == 0x0b)
    {
        goto SizeChk;
    }
    // are we in a water type area?
    if (M(AreaType) != 0)
    {
        goto SaveJoyp; // if not, branch
    }
    y = M(Player_Y_HighPos);
    --y; // if not in vertical area between
    if (y == 0)
    { // status bar and bottom, branch
        if (M(Player_Y_Position) < 0xd0)
        {
            goto SaveJoyp; // not in the vertical area between status bar or bottom,
        }
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
    {
        goto SizeChk; // if not, branch
    }
    // check player's state
    if (M(Player_State) != 0)
    {
        goto SizeChk; // if not on the ground, branch
    }
    // check left and right
    if (M(Left_Right_Buttons) == 0)
    {
        goto SizeChk; // if neither pressed, branch
    }
    a = 0x00;
    writeData(Left_Right_Buttons, 0x00); // if pressing down while on the ground,
    writeData(Up_Down_Buttons, 0x00);    // nullify directional bits

SizeChk: // run movement subroutines
    PlayerMovementSubs();
    y = 0x01; // is player small?
    if (M(PlayerSize) != 0)
    {
        goto ChkMoveDir;
    }
    y = 0x00; // check for if crouching
    if (M(CrouchingFlag) == 0)
    {
        goto ChkMoveDir; // if not, branch ahead
    }
    y = 0x02; // if big and crouching, load y with 2

ChkMoveDir: // set contents of Y as player's bounding box size control
    writeData(Player_BoundBoxCtrl, y);
    a = 0x01;              // set moving direction to right by default
    y = M(Player_X_Speed); // check player's horizontal speed
    if (y != 0)
    { // if not moving at all horizontally, skip this part
        if ((y & 0x80) != 0)
        {             // if moving to the right, use default moving direction
            a = 0x02; // otherwise change to move to the left
        } // SetMoveDir: set moving direction
        writeData(Player_MovingDir, a);
    } // PlayerSubs: move the screen if necessary
    ScrollHandler();
    GetPlayerOffscreenBits(); // get player's offscreen bits
    RelativePlayerPosition(); // get coordinates relative to the screen
    x = 0x00;                 // set offset for player object
    BoundingBoxCore(x, y);    // get player's bounding box coordinates
    PlayerBGCollision();      // do collision detection and process
    if (M(Player_Y_Position) < 0x40)
    {
        goto PlayerHole; // if so, branch ahead
    }
    a = M(GameEngineSubroutine);
    if (a == 0x05)
    {
        goto PlayerHole;
    }
    if (a == 0x07)
    {
        goto PlayerHole;
    }
    if (a < 0x04)
    {
        goto PlayerHole;
    }
    a = M(Player_SprAttrib) & 0b11011111; // otherwise nullify player's
    writeData(Player_SprAttrib, a);       // background priority flag

PlayerHole: // check player's vertical high byte
    a = M(Player_Y_HighPos);
    if (((a - 0x02) & 0x80) != 0)
    {
        return; // branch to leave if not that far down
    }
    writeData(ScrollLock, 0x01); // set scroll lock
    writeData(0x07, 0x04);       // set value here
    x = 0x00;                    // use X as flag, and clear for cloud level
    // check game timer expiration flag
    if (M(GameTimerExpiredFlag) == 0)
    {                             // if set, branch
        y = M(CloudTypeOverride); // check for cloud type override
        if (y != 0)
        {
            goto ChkHoleX; // skip to last part if found
        }
    } // HoleDie: set flag in X for player death
    x = 0x01;
    y = M(GameEngineSubroutine);
    if (y == 0x0b)
    {
        goto ChkHoleX; // if so, branch ahead
    }
    y = M(DeathMusicLoaded); // check value here
    if (y == 0)
    { // if already set, branch to next part
        ++y;
        writeData(EventMusicQueue, y);  // otherwise play death music
        writeData(DeathMusicLoaded, y); // and set value here
    } // HoleBottom
    y = 0x06;
    writeData(0x07, 0x06); // change value here

ChkHoleX: // compare vertical high byte with value set here
    if (((a - M(0x07)) & 0x80) != 0)
    {
        return; // if less, branch to leave
    }
    --x; // otherwise decrement flag in X
    if ((x & 0x80) == 0)
    {                            // if flag was clear, branch to set modes and other values
        y = M(EventMusicBuffer); // check to see if music is still playing
        if (y != 0)
        {
            return; // branch to leave if so
        }
        a = 0x06;                              // otherwise set to run lose life routine
        writeData(GameEngineSubroutine, 0x06); // on next frame

        return; // ExitCtrl: leave

        //------------------------------------------------------------------------
    } // CloudExit
    a = 0x00;
    writeData(JoypadOverride, 0x00); // clear controller override bits if any are set
    SetEntr();                       // do sub to set secondary mode
    ++M(AltEntranceControl);         // set mode of entry to 3
}
