// The EnemiesAndLoopsCore subsystem: everything EnemiesAndLoopsCore() reaches that nothing
// outside it reaches, and so nothing outside it needs.
//
// Moved out of SMB.cpp by tools/split.py. These are methods of SMBEngine like every other
// routine of the game and are declared in SMBEngine.hpp; the file they are written in is the
// only thing that is different, and tools/layers.py is what keeps it meaning something.
//
#include "SMB.hpp"

#include <tuple>

//------------------------------------------------------------------------

// Inputs: none (reads PseudoRandomBitReg, Misc_State, Enemy_Flag and ObjectOffset from memory)
// Outputs: return value = whether a hammer was spawned; x is reloaded from ObjectOffset (same
// enemy offset as on entry, not a new value) because both callers index by it on return
bool SMBEngine::SpawnHammerObj()
{
    const uint8_t HammerEnemyOfsData_data[] = {0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06};

    // get pseudorandom bits from the second part of the LSFR: d2-d0 if any of them are set,
    // otherwise d3
    const uint8_t lowBits = M(PseudoRandomBitReg + 1) & 0b00000111;
    // SetMOfs: use either d3 or d2-d0 for offset here
    const uint8_t miscOffset = (lowBits != 0) ? lowBits : (M(PseudoRandomBitReg + 1) & 0b00001000);

    const uint8_t enemyOffset = M(ObjectOffset); // get original enemy object offset

    // NoHammer: leave if any values are loaded in $2a-$32 where the offset is
    if (M(Misc_State + miscOffset) != 0)
    {
        return false;
    }
    // NoHammer: leave if the enemy buffer flag at the slot this hammer would use is set
    if (M(Enemy_Flag + HammerEnemyOfsData_data[miscOffset]) != 0)
    {
        return false;
    }
    writeData(HammerEnemyOffset + miscOffset, enemyOffset); // save here
    writeData(Misc_State + miscOffset, 0x90);        // save hammer's state here
    writeData(Misc_BoundBoxCtrl + miscOffset, 0x07); // set something else entirely, here
    return true;
}

//------------------------------------------------------------------------

// Inputs: loopIndex = loop command index, offset into AreaDataOfsLoopback_data
// Outputs: none
void SMBEngine::ExecGameLoopback(uint8_t loopIndex)
{
    const uint8_t AreaDataOfsLoopback_data[] = {0x12, 0x36, 0x0e, 0x0e, 0x0e, 0x32, 0x32, 0x32, 0x0a, 0x26, 0x40};

    writeData(Player_PageLoc, M(Player_PageLoc) - 0x04);           // send player back four pages
    writeData(CurrentPageLoc, M(CurrentPageLoc) - 0x04);           // send current page back four pages
    writeData(ScreenLeft_PageLoc, M(ScreenLeft_PageLoc) - 0x04);   // subtract four from page location
    writeData(ScreenRight_PageLoc, M(ScreenRight_PageLoc) - 0x04); // do the same for the page location
    writeData(AreaObjectPageLoc, M(AreaObjectPageLoc) - 0x04);     // subtract four from page control
    // initialize page select for both
    writeData(EnemyObjectPageSel, 0x00); // area and enemy objects
    writeData(AreaObjectPageSel, 0x00);
    writeData(EnemyDataOffset, 0x00);    // initialize enemy object data offset
    writeData(EnemyObjectPageLoc, 0x00); // and enemy object page control
    // adjust area object offset based on which loop command we encountered
    writeData(AreaDataOffset, AreaDataOfsLoopback_data[loopIndex]);
}

//------------------------------------------------------------------------

// Inputs: offsetIndex = index into PlatPosDataHigh_data/PlatPosDataLow_data (0, 1 or 2)
// selecting the offset to add; e = enemy object buffer offset
// Outputs: none
void SMBEngine::PosPlatform(uint8_t offsetIndex, uint8_t e)
{
    const uint8_t PlatPosDataHigh_data[] = {0x00, 0x00, 0xff};

    const uint8_t PlatPosDataLow_data[] = {0x08, 0x0c, 0xf8};

    // add or subtract pixels depending on offset, as one 16-bit page:coordinate
    const uint32_t wide = ((M(Enemy_PageLoc + e) << 8) | M(Enemy_X_Position + e)) +
                          ((PlatPosDataHigh_data[offsetIndex] << 8) | PlatPosDataLow_data[offsetIndex]);
    writeData(Enemy_X_Position + e, LOBYTE(wide)); // store as new horizontal coordinate
    writeData(Enemy_PageLoc + e, HIBYTE(wide));    // store as new page location
    // and go back
}

//------------------------------------------------------------------------

// Inputs: blooberCarry = carry left behind by the caller's swim-direction subtraction; e = enemy
// object buffer offset
// Outputs: none
void SMBEngine::ProcSwimmingB(bool blooberCarry, uint8_t e)
{
    // Floatdown: sink one pixel every other frame, then leave. Two separate places drop into
    // this, one of them from outside the block it used to live in.
    const auto floatdown = [&]()
    {
        // get frame counter and check for d0 set
        if ((M(FrameCounter) & 0x01) == 0)
        {
            ++M(Enemy_Y_Position + e); // otherwise increment vertical coordinate
        } // NoFD: leave
    };

    // get enemy's movement counter and check for d1 set
    if ((M(BlooperMoveCounter + e) & 0b00000010) == 0)
    {
        const uint8_t frameBits = M(FrameCounter) & 0b00000111; // get 3 LSB of frame counter
        // execute the code below only every eighth frame
        if (frameBits != 0)
        {
            return;
        }
        // d0 of the enemy's movement counter picks speeding up from slowing down
        if ((M(BlooperMoveCounter + e) & 0x01) == 0)
        {
            const uint8_t newForce = M(Enemy_Y_MoveForce + e) + 0x01;
            writeData(Enemy_Y_MoveForce + e, newForce); // set movement force
            writeData(BlooperMoveSpeed + e, newForce);  // set as movement speed
            if (newForce != 0x02)
            {
                return; // not yet at the top speed, leave
            }
            ++M(BlooperMoveCounter + e); // otherwise increment movement counter
            return;                      // BSwimE
        }

        // SlowSwim
        const uint8_t newForce = M(Enemy_Y_MoveForce + e) - 0x01;
        writeData(Enemy_Y_MoveForce + e, newForce); // set movement force
        writeData(BlooperMoveSpeed + e, newForce);  // set as movement speed
        if (newForce != 0)
        {
            return; // if any speed left, leave
        }
        ++M(BlooperMoveCounter + e);             // otherwise increment movement counter
        writeData(EnemyIntervalTimer + e, 0x02); // set enemy's timer
        return;                                  // NoSSw: leave
    }

    // ChkForFloatdown: get enemy timer, branch if expired
    if (M(EnemyIntervalTimer + e) != 0)
    {
        floatdown();
        return;
    }

    // ChkNearPlayer: get vertical coordinate, add sixteen pixels, plus whatever carry the swim
    // code left behind
    const uint8_t modifiedYPos = static_cast<uint8_t>(M(Enemy_Y_Position + e) + 0x10 + (blooberCarry ? 1 : 0));
    if (modifiedYPos < M(Player_Y_Position))
    {
        floatdown(); // modified vertical less than player's
        return;
    }
    writeData(BlooperMoveCounter + e, 0x00); // otherwise nullify movement counter
}

//------------------------------------------------------------------------

// Inputs: spinstateHigh = high byte of the firebar's spinstate (reads the firebar's index from
// zero-page 0x00)
// Outputs: none (results are communicated via zero-page 0x01-0x03: horizontal adder, vertical
// adder, and mirroring data)
void SMBEngine::GetFirebarPosition(uint8_t spinstateHigh)
{
    const uint8_t FirebarPosLookupTbl_data[] = {// sine with amplitude 0x08
                                                0x00, 0x01, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08,
                                                // sine with amplitude 0x10
                                                0x00, 0x03, 0x06, 0x09, 0x0b, 0x0d, 0x0e, 0x0f, 0x10,
                                                // sine with amplitude 0x18
                                                0x00, 0x04, 0x09, 0x0d, 0x10, 0x13, 0x16, 0x17, 0x18,
                                                // sine with amplitude 0x20
                                                0x00, 0x06, 0x0c, 0x12, 0x16, 0x1a, 0x1d, 0x1f, 0x20,
                                                // sine with amplitude 0x28
                                                0x00, 0x07, 0x0f, 0x16, 0x1c, 0x21, 0x25, 0x27, 0x28,
                                                // sine with amplitude 0x30
                                                0x00, 0x09, 0x12, 0x1b, 0x21, 0x27, 0x2c, 0x2f, 0x30,
                                                // sine with amplitude 0x38
                                                0x00, 0x0b, 0x15, 0x1f, 0x27, 0x2e, 0x33, 0x37, 0x38,
                                                // sine with amplitude 0x40
                                                0x00, 0x0c, 0x18, 0x24, 0x2d, 0x35, 0x3b, 0x3e, 0x40,
                                                // sine with amplitude 0x48
                                                0x00, 0x0e, 0x1b, 0x28, 0x32, 0x3b, 0x42, 0x46, 0x48,
                                                // sine with amplitude 0x50
                                                0x00, 0x0f, 0x1f, 0x2d, 0x38, 0x42, 0x4a, 0x4e, 0x50,
                                                // sine with amplitude 0x58
                                                0x00, 0x11, 0x22, 0x31, 0x3e, 0x49, 0x51, 0x56, 0x58};

    // mask out low nybble; if $09 or higher, get two's complement to oscillate
    uint8_t hAdder = spinstateHigh & 0b00001111;
    if (hAdder >= 0x09)
    {
        hAdder = (hAdder ^ 0b00001111) + 0x01;
    } // GetHAdder: store result, modified or not, here
    writeData(0x01, hAdder);
    // load number of firebar ball where we're at, load offset to firebar position data, add
    // oscillated high byte of spinstate to offset here and use as new offset
    const uint8_t hIndex = M(FirebarTblOffsets + M(0x00)) + M(0x01);
    // get data here and store as horizontal adder
    writeData(0x01, FirebarPosLookupTbl_data[hIndex]);
    // add eight this time, to get vertical adder, and mask out high nybble
    uint8_t vAdder = (spinstateHigh + 0x08) & 0b00001111;
    if (vAdder >= 0x09)
    {
        vAdder = (vAdder ^ 0b00001111) + 0x01; // otherwise get two's compliment
    } // GetVAdder: store result here
    writeData(0x02, vAdder);
    // load offset to firebar position data again, this time add value in $02 to offset here
    // and use as offset
    const uint8_t vIndex = M(FirebarTblOffsets + M(0x00)) + M(0x02);
    // get data here and store as vertical adder
    writeData(0x02, FirebarPosLookupTbl_data[vIndex]);
    // divide the high byte of the spinstate by eight and use as offset
    writeData(0x03, M(FirebarMirrorData + (spinstateHigh >> 3))); // load mirroring data here and store
}

//------------------------------------------------------------------------

// Inputs: spinSpeed = spinning speed to apply this frame; e = enemy object buffer offset
// (selects spin direction and spin state)
// Outputs: return value = new (unmasked) high byte of spin state; the sole caller masks it
// before storing
uint8_t SMBEngine::FirebarSpin(uint8_t spinSpeed, uint8_t e)
{
    writeData(0x07, spinSpeed); // save spinning speed here
    // check spinning direction: add spinning speed to what would normally be the horizontal
    // speed, or subtract it if moving counter-clockwise
    const uint16_t spinState = (M(FirebarSpinState_High + e) << 8) | M(FirebarSpinState_Low + e);
    const uint32_t wide = (M(FirebarSpinDirection + e) == 0) ? (spinState + M(0x07)) // SpinClockwise
                                                             : (spinState - M(0x07)); // SpinCounterClockwise
    writeData(FirebarSpinState_Low + e, LOBYTE(wide));
    // what would normally be the vertical speed, never stored back
    return HIBYTE(wide);
}

//------------------------------------------------------------------------

// Inputs: vertSpeed = vertical speed of the platform; e = the enemy offset of the platform to
// draw the rope for
// Outputs: none (results are communicated via zero-page 0x00-0x02, the name table address to
// write)
void SMBEngine::SetupPlatformRope(uint8_t vertSpeed, uint8_t e)
{
    // get horizontal coordinate, add eight pixels; unless the secondary hard mode flag is set,
    // add sixteen more, dropping the carry from the eight
    uint32_t wide = M(Enemy_X_Position + e) + 0x08;
    if (M(SecondaryHardMode) == 0)
    {
        wide = LOBYTE(wide) + 0x10;
    } // GetLRp
    const uint8_t horizCoord = LOBYTE(wide);
    // add carry to page location and save here
    writeData(0x02, (uint8_t)(M(Enemy_PageLoc + e) + HIBYTE(wide)));
    // mask out the low nybble and shift three bits to the right, storing the result as part of
    // the name table low byte
    writeData(0x00, (horizCoord & 0b11110000) >> 3);

    // get vertical coordinate; a platform moving upwards draws the rope eight pixels lower
    uint8_t vertCoord = M(Enemy_Y_Position + e);
    if ((vertSpeed & 0x80) != 0)
    {
        vertCoord += 0x08;
    } // GetHRp

    // keep d7 and d6 of the vertical coordinate aside, and rotate d7 round to d0
    const uint8_t highBits = vertCoord >> 6;
    const uint8_t rotatedVert = (uint8_t)((vertCoord << 2) | (vertCoord >> 7));
    // with d7 and d6 at the 2 LSB, set d5 to get the appropriate high byte of name table
    // address, then store
    writeData(0x01, highBits | 0b00100000);
    // get saved page location from earlier and mask out all but the LSB; shift twice to the left
    // and save with the rest of the bits of the high byte, to get the proper name table and the
    // right place on it
    writeData(0x01, ((M(0x02) & 0x01) << 2) | M(0x01));
    // mask out the low nybble and the LSB of the high nybble, and add to the horizontal part
    // saved here, to give the name table low byte
    writeData(0x00, (uint8_t)((rotatedVert & 0b11100000) + M(0x00)));

    if (M(Enemy_Y_Position + e) >= 0xe8)
    {
        // bottom of the screen, we're done: mask out d6 of low byte of name table address
        writeData(0x00, M(0x00) & 0b10111111);
    } // ExPRp: leave!
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: return value = whether Enemy_Y_Position + 62 is in range ($44 or more)
bool SMBEngine::SubtEnemyYPos(uint8_t e)
{
    // add 62 pixels to enemy object's vertical coordinate and compare against a certain
    // range, then leave with the result for a conditional branch
    const uint8_t adjustedYPos = M(Enemy_Y_Position + e) + 0x3e;
    return adjustedYPos >= 0x44;
}

//------------------------------------------------------------------------

// Inputs: baseCoord = base X or Y coordinate for the first sprite (each subsequent sprite adds
// 8 pixels); oamOffset = starting OAM data offset
// Outputs: return value = the OAM data offset reloaded from zero-page 0x02, where the caller
// stashed it (the same offset as on entry when the caller passed the stashed value)
uint8_t SMBEngine::SixSpriteStacker(uint8_t baseCoord, uint8_t oamOffset)
{
    // StkLp: store X or Y coordinate into OAM data, do six sprites
    for (uint8_t sprite = 0; sprite < 0x06; ++sprite)
    {
        writeData(Sprite_Data + (uint8_t)(oamOffset + sprite * 4), baseCoord + sprite * 0x08);
    }
    return M(0x02); // get saved OAM data offset and leave
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (lakitu's slot)
// Outputs: return value = adjusted movement speed, used by the caller to set LakituMoveSpeed
uint8_t SMBEngine::PlayerLakituDiff(uint8_t e)
{
    bool enemyRightOfPlayer = false;
    uint8_t hDiff = 0;

    std::tie(enemyRightOfPlayer, hDiff) = PlayerEnemyDiff(e); // get horizontal difference between enemy and player
    // 0 means lakitu should move right (towards a player on its right), 1 means left
    uint8_t lakituDir = 0x00;
    if ((hDiff & 0x80) != 0)
    {                  // enemy is to the right of the player
        lakituDir = 1; // move to the left of player
        // store two's compliment of low byte of horizontal difference
        writeData(0x00, (M(0x00) ^ 0xff) + 0x01);
    }

    // ChkLakDif: beyond the maximum distance, clamp it and turn lakitu around
    if (M(0x00) >= 0x3c)
    {
        writeData(0x00, 0x3c); // set maximum distance
        // check if lakitu is in our current enemy slot, and that it is not already moving
        // toward the player (in which case its direction is not altered)
        if (M(Enemy_ID + e) == Lakitu && lakituDir != M(LakituMoveDirection + e))
        {
            // if moving to the left beyond maximum distance, alter without delay
            if (M(LakituMoveDirection + e) != 0)
            {
                --M(LakituMoveSpeed + e); // decrement horizontal speed
                if (M(LakituMoveSpeed + e) != 0)
                {
                    // Horizontal speed not yet at zero, leave. The decremented speed is the
                    // value the caller writes back.
                    return M(LakituMoveSpeed + e);
                }
            }
            // SetLMovD: set horizontal direction depending on horizontal difference between
            // enemy and player if necessary
            writeData(LakituMoveDirection + e, lakituDir);
        }
    }

    // ChkPSpeed: mask out all but four bits in the middle and divide the difference by four
    writeData(0x00, (M(0x00) & 0b00111100) >> 2);

    // Pick one of the three values saved at $01-$03 according to how fast the player and the
    // screen are moving; a standing player or a stopped screen uses the first.
    uint8_t valueIndex = 0x00;
    if (M(Player_X_Speed) != 0 && M(ScrollAmount) != 0)
    {
        valueIndex = 0x01;
        if (M(Player_X_Speed) >= 0x19 && M(ScrollAmount) >= 0x02)
        {
            valueIndex = 0x02;
        }
        // ChkSpinyO: a spiny keeps the index picked above while the player is moving
        const bool spinyWithPlayerMoving = (M(Enemy_ID + e) == Spiny) && (M(Player_X_Speed) != 0);
        // ChkEmySpd: anything else with no vertical speed goes back to the first value
        if (!spinyWithPlayerMoving && M(Enemy_Y_Speed + e) == 0)
        {
            valueIndex = 0x00;
        }
    }

    // SubDifAdj: get one of three saved values from earlier
    uint8_t adjustedSpeed = M(0x0001 + valueIndex);

    // SPixelLak: subtract one for each pixel of horizontal difference from one of three saved
    // values, until all pixels are subtracted, to adjust difference
    for (uint8_t pixels = M(0x00); (pixels & 0x80) == 0; --pixels)
    {
        --adjustedSpeed;
    }

    return adjustedSpeed; // ExMoveLak: leave!!!
}

//------------------------------------------------------------------------

// Inputs: segment = which vine segment to draw, index into VineYPosAdder_data/VineObjOffset
// Outputs: none
void SMBEngine::DrawVine(uint8_t segment)
{
    const uint8_t VineYPosAdder_data[] = {0x00, 0x30};

    writeData(0x00, segment); // save offset here
    // get relative vertical coordinate and add value using segment offset to get base coordinate
    const uint8_t baseYPos = M(Enemy_Rel_YPos) + VineYPosAdder_data[segment];
    const uint8_t vineObj = M(VineObjOffset + segment);         // get offset to vine
    const uint8_t oamOffset = M(Enemy_SprDataOffset + vineObj); // get sprite data offset
    writeData(0x02, oamOffset);                                 // store sprite data offset here
    SixSpriteStacker(baseYPos, oamOffset); // stack six sprites on top of each other vertically
    const uint8_t relXPos = M(Enemy_Rel_XPos);          // get relative horizontal coordinate
    writeData(Sprite_X_Position + oamOffset, relXPos);  // store in first, third and fifth sprites
    writeData(Sprite_X_Position + 8 + oamOffset, relXPos);
    writeData(Sprite_X_Position + 16 + oamOffset, relXPos);
    // add six pixels to second, fourth and sixth sprites to give characteristic staggered vine
    // shape to our vertical stack of sprites
    writeData(Sprite_X_Position + 4 + oamOffset, relXPos + 0x06);
    writeData(Sprite_X_Position + 12 + oamOffset, relXPos + 0x06);
    writeData(Sprite_X_Position + 20 + oamOffset, relXPos + 0x06);
    // set bg priority and palette attribute bits
    writeData(Sprite_Attributes + oamOffset, 0b00100001); // set in first, third and fifth sprites
    writeData(Sprite_Attributes + 8 + oamOffset, 0b00100001);
    writeData(Sprite_Attributes + 16 + oamOffset, 0b00100001);
    // additionally, set horizontal flip bit for second, fourth and sixth sprites
    writeData(Sprite_Attributes + 4 + oamOffset, 0b01100001);
    writeData(Sprite_Attributes + 12 + oamOffset, 0b01100001);
    writeData(Sprite_Attributes + 20 + oamOffset, 0b01100001);

    // VineTL: set tile number for all six sprites
    for (uint8_t sprite = 0; sprite < 0x06; ++sprite)
    {
        writeData(Sprite_Tilenumber + (uint8_t)(oamOffset + sprite * 4), 0xe1);
    }
    // get offset to vine adding data; if offset not zero, skip this part
    if (M(0x00) == 0)
    {
        writeData(Sprite_Tilenumber + oamOffset, 0xe0); // set other tile number for top of vine
    } // SkpVTop: start with the first sprite again

    // ChkFTop: move offscreen any sprite whose Y coordinate is $64 or more below the original
    // starting vertical coordinate
    for (uint8_t sprite = 0; sprite < 0x06; ++sprite)
    {
        const uint8_t distance = M(VineStart_Y_Position) - M(Sprite_Y_Position + (uint8_t)(oamOffset + sprite * 4));
        if (distance >= 0x64)
        {
            writeData(Sprite_Y_Position + (uint8_t)(oamOffset + sprite * 4), 0xf8); // move sprite offscreen
        } // NextVSp: move offset to next OAM data
    }
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitRetainerObj(uint8_t e)
{
    // set fixed vertical position for princess/mushroom retainer object
    writeData(Enemy_Y_Position + e, 0xb8);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitBulletBill(uint8_t e)
{
    // set moving direction for left
    writeData(Enemy_MovingDir + e, 0x02);
    // set bounding box control for $09
    writeData(Enemy_BoundBoxCtrl + e, 0x09);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (the fireworks object's slot)
// Outputs: none
void SMBEngine::InitFireworks(uint8_t e)
{
    const uint8_t FireworksYPosData_data[] = {0x60, 0x40, 0x70, 0x40, 0x60, 0x30};

    const uint8_t FireworksXPosData_data[] = {0x00, 0x30, 0x60, 0x60, 0x00, 0x20};

    // if timer not expired yet, branch to leave
    if (M(FrenzyEnemyTimer) != 0)
    {
        return; // ExitFWk
    }
    writeData(FrenzyEnemyTimer, 0x20); // otherwise reset timer
    --M(FireworksCounter);             // decrement for each explosion

    // StarFChk: start at last slot and check for presence of star flag object
    uint8_t starFlagSlot = 0x06;
    do
    {
        --starFlagSlot;
    } while (M(Enemy_ID + starFlagSlot) != StarFlagObject); // routine goes into infinite loop = crash

    // get horizontal coordinate of star flag object, then subtract 48 pixels from it
    const uint32_t starFlagXPos = ((M(Enemy_PageLoc + starFlagSlot) << 8) | M(Enemy_X_Position + starFlagSlot)) - 0x30;
    writeData(0x00, HIBYTE(starFlagXPos)); // the page location of the star flag object, less the borrow

    // get fireworks counter, add state of star flag object (possibly not necessary), use as offset
    const uint8_t fireworksIndex = M(FireworksCounter) + M(Enemy_State + starFlagSlot);

    // add number based on offset of fireworks counter
    const uint32_t fireworksXPos = ((M(0x00) << 8) | LOBYTE(starFlagXPos)) + FireworksXPosData_data[fireworksIndex];
    writeData(Enemy_X_Position + e, LOBYTE(fireworksXPos)); // store as the fireworks object horizontal coordinate
    writeData(Enemy_PageLoc + e, HIBYTE(fireworksXPos));    // the fireworks object
    // get vertical position using same offset
    writeData(Enemy_Y_Position + e, FireworksYPosData_data[fireworksIndex]); // and store as vertical coordinate for fireworks object
    writeData(Enemy_Y_HighPos + e, 0x01);       // store in vertical high byte
    writeData(Enemy_Flag + e, 0x01);            // and activate enemy buffer flag
    writeData(ExplosionGfxCounter + e, 0x00);   // initialize explosion counter
    writeData(ExplosionTimerCounter + e, 0x08); // set explosion timing counter
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (the object being disabled)
// Outputs: none
void SMBEngine::EndFrenzy(uint8_t e)
{
    // LakituChk: start at last slot, check enemy identifiers in all slots
    for (uint8_t slot = 0x05; (slot & 0x80) == 0; --slot)
    {
        if (M(Enemy_ID + slot) == Lakitu)
        {
            writeData(Enemy_State + slot, 0x01); // if found, set state
        } // NextFSlot: move onto the next slot
    }
    writeData(EnemyFrenzyBuffer, 0x00); // empty enemy frenzy buffer
    writeData(Enemy_Flag + e, 0x00);    // disable enemy buffer flag for this object
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MovePiranhaPlant(uint8_t e)
{
    // Every one of the six ways this can bail out lands on the same tail (PutinPipe), so the
    // movement is a lambda that returns and the tail runs unconditionally afterwards.
    const auto movePlant = [&]()
    {
        // check enemy state; leave if set at all
        if (M(Enemy_State + e) != 0)
        {
            return;
        }
        // check enemy's timer here; leave if not yet expired
        if (M(EnemyFrameTimer + e) != 0)
        {
            return;
        }
        // check movement flag
        if (M(PiranhaPlant_MoveFlag + e) == 0)
        { // if moving, skip to part ahead
            // if currently rising, branch
            if ((M(PiranhaPlant_Y_Speed + e) & 0x80) == 0)
            { // to move enemy upwards out of pipe
                // get horizontal difference between player and piranha plant
                bool enemyRightOfPlayer = false;
                uint8_t horizDiff = 0;
                std::tie(enemyRightOfPlayer, horizDiff) = PlayerEnemyDiff(e);
                if ((horizDiff & 0x80) != 0)
                { // branch if enemy to right of player
                    // otherwise negate the saved horizontal difference
                    writeData(0x00, (M(0x00) ^ 0xff) + 0x01);
                } // ChkPlayerNearPipe
                // get saved horizontal difference; leave if player within a certain distance
                if (M(0x00) < 0x21)
                {
                    return;
                }
            } // ReversePlantSpeed
            // negate the vertical speed and save it as the new one
            writeData(PiranhaPlant_Y_Speed + e, (M(PiranhaPlant_Y_Speed + e) ^ 0xff) + 0x01);
            ++M(PiranhaPlant_MoveFlag + e); // increment to set movement flag
        } // SetupToMovePPlant
        // head for the highest point when rising (negative speed), the lowest point otherwise
        const bool movingUp = (M(PiranhaPlant_Y_Speed + e) & 0x80) != 0;
        const uint8_t targetYPos = movingUp ? M(PiranhaPlantUpYPos + e) : M(PiranhaPlantDownYPos + e);
        // RiseFallPiranhaPlant
        writeData(0x00, targetYPos); // save vertical coordinate here

        // execute the code below only every other frame
        if ((M(FrameCounter) & 0x01) == 0)
        {
            return;
        }
        // get master timer control; leave if set (likely not necessary)
        if (M(TimerControl) != 0)
        {
            return;
        }
        // add vertical speed to the current vertical coordinate to move up or down
        const uint8_t newYPos = M(Enemy_Y_Position + e) + M(PiranhaPlant_Y_Speed + e);
        writeData(Enemy_Y_Position + e, newYPos); // save as new vertical coordinate
        if (newYPos != M(0x00))
        {
            return; // leave if the target coordinate is not yet reached
        }
        writeData(PiranhaPlant_MoveFlag + e, 0x00); // otherwise clear movement flag
        writeData(EnemyFrameTimer + e, 0x40);       // set timer to delay piranha plant movement
    };
    movePlant();

    // PutinPipe: set background priority bit in sprite attributes to give illusion of being
    // inside pipe, then leave
    writeData(Enemy_SprAttrib + e, 0b00100000);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitJumpGPTroopa(uint8_t e)
{
    // set for movement to the left
    writeData(Enemy_MovingDir + e, 0x02);
    // set horizontal speed
    writeData(Enemy_X_Speed + e, 0xf8);
    TallBBox2(e);
}

//------------------------------------------------------------------------

// set specific value for bounding box control
// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::TallBBox2(uint8_t e) { SetBBox2(0x03, e); }

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitGoomba(uint8_t e)
{
    InitNormalEnemy(e); // set appropriate horizontal speed
    SmallBBox(e);       // set $09 as bounding box control, set other values
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitPodoboo(uint8_t e)
{
    // set enemy position to below
    writeData(Enemy_Y_HighPos + e, 0x02); // the bottom of the screen
    writeData(Enemy_Y_Position + e, 0x02);
    writeData(EnemyIntervalTimer + e, 0x01); // set timer for enemy
    writeData(Enemy_State + e, 0x00);        // initialize enemy state, then jump to use
    SmallBBox(e);                            // $09 as bounding box size and set other things
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (only forwarded to SetESpd)
// Outputs: none
void SMBEngine::InitNormalEnemy(uint8_t e)
{
    const uint8_t NormalXSpdData_data[] = {0xf8, 0xf4};

    // the faster speed applies when the primary hard mode flag is set
    const uint8_t speedIndex = (M(PrimaryHardMode) != 0) ? 0x01 : 0x00;
    // GetESpd: get appropriate horizontal speed
    SetESpd(NormalXSpdData_data[speedIndex], e);
}

//------------------------------------------------------------------------

// store as speed for enemy object
// Inputs: speed = horizontal speed to store; e = enemy object buffer offset
// Outputs: none
void SMBEngine::SetESpd(uint8_t speed, uint8_t e)
{
    writeData(Enemy_X_Speed + e, speed);
    TallBBox(e); // branch to set bounding box control and other data
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitRedKoopa(uint8_t e)
{
    InitNormalEnemy(e); // load appropriate horizontal speed
    // set enemy state for red koopa troopa $03
    writeData(Enemy_State + e, 0x01);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitHammerBro(uint8_t e)
{
    const uint8_t HBroWalkingTimerData_data[] = {0x80, 0x50};

    // init horizontal speed and timer used by hammer bro
    writeData(HammerThrowingTimer + e, 0x00); // apparently to time hammer throwing
    writeData(Enemy_X_Speed + e, 0x00);
    const uint8_t hardMode = M(SecondaryHardMode);                          // get secondary hard mode flag
    writeData(EnemyIntervalTimer + e, HBroWalkingTimerData_data[hardMode]); // set value as delay for hammer bro to walk left
    SetBBox(0x0b, e); // set specific value for bounding box size control
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitHorizFlySwimEnemy(uint8_t e)
{
    // initialize horizontal speed
    SetESpd(0x00, e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitBloober(uint8_t e)
{
    // initialize horizontal speed
    writeData(BlooperMoveSpeed + e, 0x00);
    SmallBBox(e);
}

//------------------------------------------------------------------------

// set specific bounding box size control
// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::SmallBBox(uint8_t e)
{
    SetBBox(0x09, e); // unconditional branch
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitRedPTroopa(uint8_t e)
{
    const uint8_t yPosition = M(Enemy_Y_Position + e); // set vertical coordinate into location to
    writeData(RedPTroopaOrigXPos + e, yPosition);      // be used as original vertical coordinate

    // central position adder is 48 pixels down, or 32 pixels up if the vertical coordinate >= $80
    const uint8_t centerAdder = ((yPosition & 0x80) != 0) ? 0xe0 : 0x30;
    // GetCent: add to current vertical coordinate
    writeData(RedPTroopaCenterYPos + e, (uint8_t)(centerAdder + yPosition)); // store as central vertical coordinate
    TallBBox(e);
}

//------------------------------------------------------------------------

// set specific bounding box size control
// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::TallBBox(uint8_t e) { SetBBox(0x03, e); }

//------------------------------------------------------------------------

// set bounding box control here
// Inputs: boundBoxCtrl = bounding box control value; e = enemy object buffer offset
// Outputs: none
void SMBEngine::SetBBox(uint8_t boundBoxCtrl, uint8_t e)
{
    writeData(Enemy_BoundBoxCtrl + e, boundBoxCtrl);
    // set moving direction for left
    writeData(Enemy_MovingDir + e, 0x02);
    InitVStf(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitCheepCheep(uint8_t e)
{
    SmallBBox(e); // set vertical bounding box, speed, init others
    // check one portion of LSFR, get d4 from it
    writeData(CheepCheepMoveMFlag + e, M(PseudoRandomBitReg + e) & 0b00010000); // save as movement flag of some sort
    writeData(CheepCheepOrigYPos + e, M(Enemy_Y_Position + e));                 // save original vertical coordinate here
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::EnemyLanding(uint8_t e)
{
    InitVStf(e); // do something here to vertical speed and something else
    // save high nybble of vertical coordinate, and set d3, then store, probably used
    // to set enemy object neatly on whatever it's landing on
    writeData(Enemy_Y_Position + e, (M(Enemy_Y_Position + e) & 0b11110000) | 0b00001000);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitHoriPlatform(uint8_t e)
{
    writeData(XMoveSecondaryCounter + e, 0x00); // init one of the moving counters
    CommonPlatCode(e);                          // jump ahead to execute more code
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitVertPlatform(uint8_t e)
{
    const uint8_t yPosition = M(Enemy_Y_Position + e); // check vertical position
    // if above a certain point, negate it and use the alternate value to add to it
    const bool aboveCenter = (yPosition & 0x80) != 0;
    const uint8_t topYPos = aboveCenter ? (uint8_t)((yPosition ^ 0xff) + 0x01) : yPosition;
    writeData(YPlatformTopYPos + e, topYPos); // SetYO: save as top vertical position
    const uint8_t centerAdder = aboveCenter ? 0xc0 : 0x40;
    // add to vertical position, save result as central vertical position
    writeData(YPlatformCenterYPos + e, (uint8_t)(centerAdder + yPosition));
    CommonPlatCode(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::CommonPlatCode(uint8_t e)
{
    InitVStf(e); // do a sub to init certain other values
    SPBBox(e);
}

//------------------------------------------------------------------------

// set default bounding box size control
// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::SPBBox(uint8_t e)
{
    // use the default value in castles or in secondary hard mode, the alternate value otherwise
    const bool useDefault = (M(AreaType) == 0x03) || (M(SecondaryHardMode) != 0);

    // CasPBB: set bounding box size control here and leave
    writeData(Enemy_BoundBoxCtrl + e, useDefault ? 0x05 : 0x06);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::PlatLiftUp(uint8_t e)
{
    // set movement amount here
    writeData(Enemy_Y_MoveForce + e, 0x10);
    // set moving speed for platforms going up
    writeData(Enemy_Y_Speed + e, 0xff);
    CommonSmallLift(e); // skip ahead to part we should be executing
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::PlatLiftDown(uint8_t e)
{
    // set movement amount here
    writeData(Enemy_Y_MoveForce + e, 0xf0);
    // set moving speed for platforms going down
    writeData(Enemy_Y_Speed + e, 0x00);
    CommonSmallLift(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::CommonSmallLift(uint8_t e)
{
    PosPlatform(0x01, e); // do a sub to add 12 pixels due to preset value
    writeData(Enemy_BoundBoxCtrl + e, 0x04); // set bounding box control for small platforms
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (forwarded to MoveRedPTroopa)
// Outputs: none
void SMBEngine::MoveRedPTroopaDown(uint8_t e)
{
    MoveRedPTroopa(0x00, e); // move downwards; skip to movement routine
}

//------------------------------------------------------------------------

// Inputs: moveDirection = movement mode forwarded to ImposeGravity (0 = down, 1 = up); e =
// enemy object buffer offset of the leading paratroopa (the trailing one at e+1 is moved)
// Outputs: none
void SMBEngine::MoveRedPTroopa(uint8_t moveDirection, uint8_t e)
{
    writeData(0x00, 0x03); // set downward movement amount here
    writeData(0x01, 0x06); // set upward movement amount here
    writeData(0x02, 0x02); // set maximum speed here
    // increment enemy offset and jump to move this thing
    RedPTroopaGrav(moveDirection, e + 1);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (forwarded to Skip_7)
// Outputs: none
void SMBEngine::MovePlatformDown(uint8_t e) { Skip_7(0x00, e); }

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (forwarded to Skip_7)
// Outputs: none
void SMBEngine::MovePlatformUp(uint8_t e) { Skip_7(0x01, e); }

//------------------------------------------------------------------------

// Inputs: moveDirection = movement direction (0 = down, 1 = up); e = enemy object buffer offset
// Outputs: none
void SMBEngine::Skip_7(uint8_t moveDirection, uint8_t e)
{
    // load default value here, or residual code for enemy object $29
    const uint8_t downwardAmount = (M(Enemy_ID + e) == 0x29) ? 0x09 : 0x05;
    writeData(0x00, downwardAmount); // SetDplSpd: save downward movement amount here
    // save upward movement amount here
    writeData(0x01, 0x0a);
    // save maximum vertical speed here
    writeData(0x02, 0x03);
    // increment offset for enemy object, then move onto code shared by red koopa
    RedPTroopaGrav(moveDirection, e + 1);
}

//------------------------------------------------------------------------

// Inputs: moveDirection = movement mode forwarded to ImposeGravity; e = object buffer offset
// of the object to move
// Outputs: none (x is reloaded from ObjectOffset, as callers pass e values offset from it)
void SMBEngine::RedPTroopaGrav(uint8_t moveDirection, uint8_t e)
{
    ImposeGravity(moveDirection, e); // do a sub to move object gradually
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::SetupLakitu(uint8_t e)
{
    // erase counter for lakitu's reappearance
    writeData(LakituReappearTimer, 0x00);
    InitHorizFlySwimEnemy(e); // set $03 as bounding box, set other attributes
    TallBBox2(e);             // set $03 as bounding box again (not necessary) and leave
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitShortFirebar(uint8_t e)
{
    const uint8_t FirebarSpinDirData_data[] = {0x00, 0x00, 0x10, 0x10, 0x00};

    const uint8_t FirebarSpinSpdData_data[] = {0x28, 0x38, 0x28, 0x38, 0x28};

    // initialize low byte of spin state
    writeData(FirebarSpinState_Low + e, 0x00);
    // subtract $1b from enemy identifier
    const uint8_t firebarType = M(Enemy_ID + e) - 0x1b;
    // get spinning speed of firebar
    writeData(FirebarSpinSpeed + e, FirebarSpinSpdData_data[firebarType]);
    // get spinning direction of firebar
    writeData(FirebarSpinDirection + e, FirebarSpinDirData_data[firebarType]);
    writeData(Enemy_Y_Position + e, M(Enemy_Y_Position + e) + 0x04);
    // add four to the horizontal coordinate as one 16-bit page:coordinate
    const uint32_t xPos = ((M(Enemy_PageLoc + e) << 8) | M(Enemy_X_Position + e)) + 0x04;
    writeData(Enemy_X_Position + e, LOBYTE(xPos));
    writeData(Enemy_PageLoc + e, HIBYTE(xPos));
    TallBBox2(e); // set bounding box control (not used) and leave
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitBalPlatform(uint8_t e)
{
    --M(Enemy_Y_Position + e); // raise vertical position by two pixels
    --M(Enemy_Y_Position + e);
    // if secondary hard mode flag set,
    if (M(SecondaryHardMode) == 0)
    {
        PosPlatform(0x02, e); // do a sub to add or subtract pixels
    }
    // AlignP: get current balance platform alignment
    const uint8_t alignment = M(BalPlatformAlignment);
    writeData(Enemy_State + e, alignment); // set platform alignment to object state here
    // if old alignment $ff, put object offset as alignment to make next positive; otherwise
    // put $ff as alignment for negative
    writeData(BalPlatformAlignment, ((alignment & 0x80) != 0) ? e : 0xff); // SetBPA
    writeData(Enemy_MovingDir + e, 0x00); // init moving direction
    PosPlatform(0x00, e); // do a sub to add 8 pixels, then run shared code here

    InitDropPlatform(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitDropPlatform(uint8_t e)
{
    writeData(PlatformCollisionFlag + e, 0xff); // set some value here
    CommonPlatCode(e);                          // then jump ahead to execute more code
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::LargeLiftUp(uint8_t e)
{
    PlatLiftUp(e);    // execute code for platforms going up
    LargeLiftBBox(e); // overwrite bounding box for large platforms
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::LargeLiftDown(uint8_t e)
{
    PlatLiftDown(e); // execute code for platforms going down

    LargeLiftBBox(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::LargeLiftBBox(uint8_t e)
{
    SPBBox(e); // jump to overwrite bounding box size control
}

//------------------------------------------------------------------------

// Inputs: none (reads CurrentPlayer from memory)
// Outputs: none
void SMBEngine::EndAreaPoints()
{
    // load offset for mario's score by default, or luigi's if he's the player on the screen
    const uint8_t scoreOffset = (M(CurrentPlayer) != 0) ? 0x11 : 0x0b;
    // ELPGive: award 50 points per game timer interval
    DigitsMathRoutine(scoreOffset);
    // get player on the screen (or 500 points per fireworks explosion if branched here from
    // there), shift to high nybble and add four to set nybble for game timer
    UpdateNumber((uint8_t)(M(CurrentPlayer) << 4) | 0b00000100); // jump to print the new score and game timer
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (star flag's slot)
// Outputs: x is reloaded from ObjectOffset (same value as on entry)
void SMBEngine::DrawStarFlag(uint8_t e)
{
    const uint8_t StarFlagTileData_data[] = {0x54, 0x55, 0x56, 0x57};

    const uint8_t StarFlagXPosAdder_data[] = {0x00, 0x08, 0x00, 0x08};

    const uint8_t StarFlagYPosAdder_data[] = {0x00, 0x00, 0x08, 0x08};

    RelativeEnemyPosition(e); // get relative coordinates of star flag
    // get OAM data offset; RelativeEnemyPosition has just put x back to ObjectOffset, which is
    // the offset the original indexed by here
    const uint8_t oamOffset = M(Enemy_SprDataOffset + M(ObjectOffset));

    // DSFLoop: do four sprites, walking the OAM data forward four bytes for each while walking
    // the adder tables backward from their last entry
    for (int sprite = 0; sprite < 4; ++sprite)
    {
        const uint8_t oam = (uint8_t)(oamOffset + sprite * 4);
        const int entry = 3 - sprite;
        // get relative vertical coordinate, add Y coordinate adder data, store as Y coordinate
        writeData(Sprite_Y_Position + oam, M(Enemy_Rel_YPos) + StarFlagYPosAdder_data[entry]);
        writeData(Sprite_Tilenumber + oam, StarFlagTileData_data[entry]); // store as tile number
        // set palette and background priority bits, storing as attributes
        writeData(Sprite_Attributes + oam, 0x22);
        // get relative horizontal coordinate, add X coordinate adder data, store as X coordinate
        writeData(Sprite_X_Position + oam, M(Enemy_Rel_XPos) + StarFlagXPosAdder_data[entry]);
    }
}

//------------------------------------------------------------------------

// Inputs: oldYPos = the platform's vertical position from before it was moved; e = enemy object
// buffer offset (this platform)
// Outputs: x is reloaded from ObjectOffset (same value as on entry)
void SMBEngine::DoOtherPlatform(uint8_t oldYPos, uint8_t e)
{
    const uint8_t otherPlatform = M(Enemy_State + e); // get offset of other platform
    // get difference of old vs. new coordinate, and add it to the vertical coordinate of the
    // other platform to move it in the opposite direction
    writeData(Enemy_Y_Position + otherPlatform,
              (uint8_t)(oldYPos - M(Enemy_Y_Position + e) + M(Enemy_Y_Position + otherPlatform)));

    const uint8_t collisionFlag = M(PlatformCollisionFlag + e); // if no collision, skip this part here
    if ((collisionFlag & 0x80) == 0)
    {
        // the flag doubles as the offset which the collision occurred at; use it to position
        // the player accordingly
        PositionPlayerOnVPlat(collisionFlag);
    } // DrawEraseRope

    const uint8_t self = M(ObjectOffset); // get enemy object offset
    // draw the rope only if the current platform is moving at all and there is room in the
    // vram buffer for the ten bytes it takes; otherwise fall straight through to ExitRp
    const bool platformMoving = (M(Enemy_Y_Speed + self) | M(Enemy_Y_MoveForce + self)) != 0;
    if (platformMoving && M(VRAM_Buffer1_Offset) < 0x20)
    {
        const uint8_t vertSpeed = M(Enemy_Y_Speed + self);
        SetupPlatformRope(vertSpeed, self); // do a sub to figure out where to put new bg tiles
        // The rope's ten bytes all go at the buffer offset as it stands; it is only advanced at
        // the end, so both halves below are written relative to the same place.
        const uint8_t vram = M(VRAM_Buffer1_Offset);

        // write name table address to vram buffer, first the high byte, then the low
        writeData(VRAM_Buffer1 + vram, M(0x01));
        writeData(VRAM_Buffer1 + 1 + vram, M(0x00));
        writeData(VRAM_Buffer1 + 2 + vram, 0x02); // set length for 2 bytes
        // A platform moving upwards erases the rope; one moving downwards draws the tile
        // numbers for the left and right sides of it.
        // EraseR1
        const bool movingDown = (vertSpeed & 0x80) == 0;
        writeData(VRAM_Buffer1 + 3 + vram, movingDown ? 0xa2 : 0x24);
        writeData(VRAM_Buffer1 + 4 + vram, movingDown ? 0xa3 : 0x24);

        // OtherRope: get offset of other platform from state, and do the sub again with the
        // speed inverted to reverse it, to figure out where to put its bg tiles
        SetupPlatformRope((uint8_t)(vertSpeed ^ 0xff), M(Enemy_State + self));
        // write name table address to vram buffer; this time we're putting tiles for the other
        // platform
        writeData(VRAM_Buffer1 + 5 + vram, M(0x01));
        writeData(VRAM_Buffer1 + 6 + vram, M(0x00));
        writeData(VRAM_Buffer1 + 7 + vram, 0x02); // set length again for 2 bytes
        // the other platform moves the opposite way, so it draws where this one erases
        // EraseR2
        writeData(VRAM_Buffer1 + 8 + vram, movingDown ? 0x24 : 0xa2);
        writeData(VRAM_Buffer1 + 9 + vram, movingDown ? 0x24 : 0xa3);

        writeData(VRAM_Buffer1 + 10 + vram, 0x00);   // EndRp: put null terminator at the end
        writeData(VRAM_Buffer1_Offset, vram + 10); // add ten bytes to the vram buffer offset
    }

    // ExitRp
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (this platform); otherPlatform = other platform's
// offset (also receives the same speed values)
// Outputs: none
void SMBEngine::StopPlatforms(uint8_t e, uint8_t otherPlatform)
{
    InitVStf(e); // initialize vertical speed and low byte
    writeData(Enemy_Y_Speed + otherPlatform, 0x00); // for both platforms and leave
    writeData(Enemy_Y_MoveForce + otherPlatform, 0x00);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::ChkYPCollision(uint8_t e)
{
    // if collision flag not set here, branch to leave
    if ((M(PlatformCollisionFlag + e) & 0x80) == 0)
    {
        PositionPlayerOnVPlat(e); // otherwise position player appropriately
    } // ExYPl: leave
}

//------------------------------------------------------------------------

// Inputs: collisionFlag = bounding box counter saved in the collision flag (1 or 2); e = enemy
// object buffer offset
// Outputs: none
void SMBEngine::PositionPlayerOnS_Plat(uint8_t collisionFlag, uint8_t e)
{
    const uint8_t PlayerPosSPlatData_data[] = {0x80, 0x00};

    // use bounding box counter saved in collision flag for offset into the coordinate adders
    Skip_8(M(Enemy_Y_Position + e) + PlayerPosSPlatData_data[collisionFlag - 1], e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::PositionPlayerOnVPlat(uint8_t e)
{
    // get vertical coordinate
    Skip_8(M(Enemy_Y_Position + e), e);
}

//------------------------------------------------------------------------

// Inputs: yCoord = vertical coordinate to position the player at; e = enemy object buffer
// offset (used to read Enemy_Y_HighPos)
// Outputs: none
void SMBEngine::Skip_8(uint8_t yCoord, uint8_t e)
{
    if (M(GameEngineSubroutine) == 0x0b)
    {
        return; // skip all of this
    }
    const uint8_t yHigh = M(Enemy_Y_HighPos + e);
    if (yHigh != 0x01)
    {
        return;
    }
    // subtract 32 pixels from the vertical coordinate for the player object's height
    const uint32_t wide = ((yHigh << 8) | yCoord) - 0x20;
    writeData(Player_Y_Position, LOBYTE(wide)); // save as player's new vertical coordinate
    writeData(Player_Y_HighPos, HIBYTE(wide));  // and store as player's new vertical high byte
    writeData(Player_Y_Speed, 0x00);     // initialize vertical speed and low byte of force
    writeData(Player_Y_MoveForce, 0x00); // and then leave

    // ExPlPos
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: return value = metatile found underneath the enemy (also left in a by
// BlockBufferCollision along the call chain)
uint8_t SMBEngine::ChkUnderEnemy(uint8_t e)
{
    // check the bottom middle (8,18) of enemy object, and save vertical coordinate
    return BlockBufferChk_Enemy(0x00, 0x15, e); // hop to it!
}

//------------------------------------------------------------------------

// Inputs: coordSelector = forwarded to BBChk_E (0 = also report Y low nybble, nonzero = X);
// cornerIdx = corner index forwarded to BBChk_E; e = enemy object buffer offset
// Outputs: return value = metatile found (also left in a by BlockBufferCollision); x is
// reloaded from ObjectOffset by BBChk_E
uint8_t SMBEngine::BlockBufferChk_Enemy(uint8_t coordSelector, uint8_t cornerIdx, uint8_t e)
{
    // add one to the enemy offset to address the sprite object buffer, and jump elsewhere
    return BBChk_E(coordSelector, e + 1, cornerIdx);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitFlyingCheepCheep(uint8_t e)
{
    const uint8_t FlyCCTimerData_data[] = {0x10, 0x60, 0x20, 0x48};

    const uint8_t FlyCCXSpeedData_data[] = {0x0e, 0x05, 0x06, 0x0e, 0x1c, 0x20, 0x10, 0x0c, 0x1e, 0x22, 0x18, 0x14};

    const uint8_t FlyCCXPositionData_data[] = {0x80, 0x30, 0x40, 0x80, 0x30, 0x50, 0x50, 0x70,
                                               0x20, 0x40, 0x80, 0xa0, 0x70, 0x40, 0x90, 0x68};

    // if timer here not expired yet, branch to leave
    if (M(FrenzyEnemyTimer) != 0)
    {
        return;
    }
    SmallBBox(e); // jump to set bounding box size $09 and init other values
    // load timer with a pseudorandom offset taken from the LSFR
    writeData(FrenzyEnemyTimer, FlyCCTimerData_data[M(PseudoRandomBitReg + 1 + e) & 0b00000011]);
    // secondary hard mode allows as many as four onscreen rather than the default three
    // MaxCC: store the maximum here
    writeData(0x00, (M(SecondaryHardMode) != 0) ? 0x04 : 0x03);
    if (e >= M(0x00))
    {
        return; // if this slot is at or past the maximum, branch to leave
    }
    // get last two bits of LSFR, first part, and store in two places
    const uint8_t lsfrBits = M(PseudoRandomBitReg + e) & 0b00000011;
    writeData(0x00, lsfrBits);
    writeData(0x01, lsfrBits);
    // set vertical speed for cheep-cheep
    writeData(Enemy_Y_Speed + e, 0xfb);
    // GSeed: a seed based on how fast the player is moving; a standing player seeds zero, and a
    // fast one (>= $19) seeds double what a slow one does
    const uint8_t playerSpeed = M(Player_X_Speed); // check player's horizontal speed
    uint8_t speedSeed = 0x00;
    if (playerSpeed != 0)
    {
        speedSeed = (playerSpeed < 0x19) ? 0x04 : 0x08;
    }

    writeData(0x00, speedSeed + M(0x00)); // add to last two bits of LSFR we saved earlier
    // if neither of the last two bits of second LSFR set, skip this part and save contents of $00
    if ((M(PseudoRandomBitReg + 1 + e) & 0b00000011) != 0)
    {
        // otherwise overwrite with lower nybble of third LSFR part
        writeData(0x00, M(PseudoRandomBitReg + 2 + e) & 0b00001111);
    } // RSeed
    // add the seed to the last two bits of LSFR we saved in the other place
    const uint8_t speedOffset = speedSeed + M(0x01); // use as pseudorandom offset here
    // get horizontal speed using pseudorandom offset
    writeData(Enemy_X_Speed + e, FlyCCXSpeedData_data[speedOffset]);
    // set to move towards the right
    writeData(Enemy_MovingDir + e, 0x01);

    // A moving player leaves the speed offset in place as the position offset; a standing one
    // replaces it with the first LSFR (or third LSFR lower nybble) and may reverse the speed.
    uint8_t posOffset = speedOffset;
    if (M(Player_X_Speed) == 0)
    {
        posOffset = M(0x00); // get first LSFR or third LSFR lower nybble
        if ((posOffset & 0b00000010) != 0)
        {
            // if d1 set, change horizontal speed direction
            writeData(Enemy_X_Speed + e, (M(Enemy_X_Speed + e) ^ 0xff) + 0x01);
            ++M(Enemy_MovingDir + e); // increment to move towards the left
        }
    }

    uint32_t wide = 0;

    // D2XPos1: get first LSFR or third LSFR lower nybble again and check for d1 set again
    const uint16_t playerPos = (M(Player_PageLoc) << 8) | M(Player_X_Position);
    if ((posOffset & 0b00000010) != 0)
    {
        // if d1 set, add value obtained from pseudorandom offset
        wide = playerPos + FlyCCXPositionData_data[posOffset];
    } // D2XPos2
    else
    {
        // if d1 not set, subtract value obtained from pseudorandom offset
        wide = playerPos - FlyCCXPositionData_data[posOffset];
    }
    writeData(Enemy_X_Position + e, LOBYTE(wide)); // save as enemy's horizontal position
    // FinCCSt: save as enemy's page location
    writeData(Enemy_PageLoc + e, HIBYTE(wide));
    writeData(Enemy_Flag + e, 0x01);      // set enemy's buffer flag
    writeData(Enemy_Y_HighPos + e, 0x01); // set enemy's high vertical byte
    writeData(Enemy_Y_Position + e, 0xf8); // put enemy below the screen, and we are done
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveFlyGreenPTroopa(uint8_t e)
{
    XMoveCntr_GreenPTroopa(e); // do sub to increment primary and secondary counters
    MoveWithXMCntrs(e);        // do sub to move green paratroopa accordingly, and horizontally

    // move up/down only every fourth frame, and d6 of the frame counter picks which way
    if ((M(FrameCounter) & 0b00000011) != 0)
    {
        return; // NoMGPT: leave!
    }
    // set Y to move green paratroopa down, or up if d6 is clear
    const uint8_t adder = ((M(FrameCounter) & 0b01000000) != 0) ? 0x01 : 0xff;
    writeData(0x00, adder); // YSway: store adder here
    // to give green paratroopa a wavy flight
    writeData(Enemy_Y_Position + e, M(Enemy_Y_Position + e) + M(0x00));
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (forwarded to XMoveCntr_Platform)
// Outputs: none
void SMBEngine::XMoveCntr_GreenPTroopa(uint8_t e)
{
    XMoveCntr_Platform(0x13, e); // load preset maximum value for secondary counter
}

//------------------------------------------------------------------------

// Inputs: maxSecondary = preset maximum value for the secondary movement counter; e = enemy
// object buffer offset
// Outputs: none
void SMBEngine::XMoveCntr_Platform(uint8_t maxSecondary, uint8_t e)
{
    writeData(0x01, maxSecondary); // store value here

    // NoIncXM: leave if not on every fourth frame
    if ((M(FrameCounter) & 0b00000011) != 0)
    {
        return;
    }

    // The secondary counter walks up to the maximum and back down to zero; d0 of the primary
    // counter says which way it is currently going. Reaching either end bumps the primary
    // counter, which reverses the direction for the next run.
    const uint8_t secondary = M(XMoveSecondaryCounter + e); // get secondary counter
    const bool countingDown = (M(XMovePrimaryCounter + e) & 0x01) != 0;
    if (countingDown)
    {
        // DecSeXM
        if (secondary != 0)
        {
            --M(XMoveSecondaryCounter + e); // decrement secondary counter and leave
            return;
        }
    }
    else
    {
        if (secondary != M(0x01))
        {
            ++M(XMoveSecondaryCounter + e); // increment secondary counter and leave
            return;
        }
    }

    // IncPXM: increment primary counter and leave
    ++M(XMovePrimaryCounter + e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none (the signed horizontal adder MoveEnemyHorizontally returns is stashed to
// zero-page 0x00 for later use by PositionPlayerOnHPlat)
void SMBEngine::MoveWithXMCntrs(uint8_t e)
{
    const uint8_t savedSecondary = M(XMoveSecondaryCounter + e); // remember the secondary counter

    // d1 of the primary counter picks the moving direction, and moving left runs the sub on the
    // negated secondary counter rather than the counter itself
    uint8_t movingDir = 0x01; // set value here by default
    if ((M(XMovePrimaryCounter + e) & 0b00000010) == 0)
    {
        // otherwise change secondary
        writeData(XMoveSecondaryCounter + e, (M(XMoveSecondaryCounter + e) ^ 0xff) + 0x01);
        movingDir = 0x02; // load alternate value here
    } // XMRight: store as moving direction
    writeData(Enemy_MovingDir + e, movingDir);
    writeData(0x00, MoveEnemyHorizontally(e));           // save value obtained from sub here
    writeData(XMoveSecondaryCounter + e, savedSecondary); // and return to original place
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (star flag's slot)
// Outputs: none
void SMBEngine::RunStarFlagObj(uint8_t e)
{
    // initialize enemy frenzy buffer
    writeData(EnemyFrenzyBuffer, 0x00);

    // IncrementSFTask1 / IncrementSFTask2: a task that has finished moves on to the next one
    const auto incrementTask = [&]() { ++M(StarFlagTaskControl); };

    const uint8_t task = M(StarFlagTaskControl); // check star flag object task number here
    if (task >= 0x05)
    {
        return;
    }
    switch (task)
    {
    case 0:
        return;

    case 1:
    {
        // GameTimerFireworks: the last digit of the game timer decides how many fireworks go
        // off, and the star flag object's state along with it
        const uint8_t lastDigit = M(GameTimerDisplay + 2); // get game timer's last digit
        uint8_t flagState = 0x00;
        uint8_t fireworksCounter = 0xff; // no fireworks by default
        if (lastDigit == 0x01)
        {
            flagState = 0x05;
            fireworksCounter = 0x01;
        }
        else if (lastDigit == 0x03)
        {
            flagState = 0x03;
            fireworksCounter = 0x03;
        }
        else if (lastDigit == 0x06)
        {
            flagState = 0x00;
            fireworksCounter = 0x06;
        }

        // SetFWC: set fireworks counter here
        writeData(FireworksCounter, fireworksCounter);
        writeData(Enemy_State + e, flagState); // set whatever state we have in star flag object
        incrementTask();                       // increment star flag object task number
        return;                                // StarFlagExit: leave
    }

    case 2:
    {
        // AwardGameTimerPoints: check all game timer digits for any intervals left
        const bool timeLeft = (M(GameTimerDisplay) | M(GameTimerDisplay + 1) | M(GameTimerDisplay + 2)) != 0;
        if (!timeLeft)
        {
            incrementTask(); // no time left on game timer at all, move to next task
            return;
        }
        // check frame counter for d2 set (for four frames every four frames)
        if ((M(FrameCounter) & 0b00000100) != 0)
        {
            writeData(Square2SoundQueue, Sfx_TimerTick); // load timer tick sound
        }
        // NoTTick: set adder to $ff, or -1, to subtract one from the last digit of the game
        // timer, at offset $23
        writeData(DigitModifier + 5, 0xff);
        DigitsMathRoutine(0x23);            // subtract digit
        writeData(DigitModifier + 5, 0x05); // set now to add 50 points per interval subtracted
        EndAreaPoints();
        return;
    }

    case 3:
    {
        // RaiseFlagSetoffFWorks: check star flag's vertical position
        if (M(Enemy_Y_Position + e) >= 0x72)
        {
            --M(Enemy_Y_Position + e); // raise star flag by one pixel
            DrawStarFlag(e);            // and skip this part here
            return;
        }
        // SetoffF: check fireworks counter; anything left to go off is queued up
        const uint8_t fireworksCounter = M(FireworksCounter);
        if (fireworksCounter != 0 && (fireworksCounter & 0x80) == 0)
        {
            writeData(EnemyFrenzyBuffer, Fireworks); // set fireworks object in frenzy queue
            DrawStarFlag(e);
            return;
        }
        // DrawFlagSetTimer
        DrawStarFlag(e);                          // do sub to draw star flag
        writeData(EnemyIntervalTimer + e, 0x06); // set interval timer here
        incrementTask();                         // move onto next task
        return;
    }

    case 4:
        // DelayToAreaEnd
        DrawStarFlag(e); // do sub to draw star flag
        // the interval timer set in the previous task must have expired, and the event music
        // buffer must be empty, before the next task
        if (M(EnemyIntervalTimer + e) == 0 && M(EventMusicBuffer) == 0)
        {
            incrementTask();
        }
        // StarFlagExit2: otherwise leave
        return;

    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::YMovingPlatform(uint8_t e)
{
    // if the platform is not moving up or down and has not yet reached its top position, drift
    // it down a pixel every eighth frame; otherwise fall through to ChkYCenterPos
    const bool platformMoving = (M(Enemy_Y_Speed + e) | M(Enemy_Y_MoveForce + e)) != 0;
    if (!platformMoving)
    {
        writeData(Enemy_YMF_Dummy + e, 0x00); // initialize dummy variable
        if (M(Enemy_Y_Position + e) < M(YPlatformTopYPos + e))
        {
            if ((M(FrameCounter) & 0b00000111) == 0)
            {
                ++M(Enemy_Y_Position + e); // increase vertical position every eighth frame
            } // SkipIY: skip ahead to last part
            ChkYPCollision(e);
            return;
        }
    }

    // ChkYCenterPos
    // if current vertical position < central position, branch
    if (M(Enemy_Y_Position + e) >= M(YPlatformCenterYPos + e))
    {
        MovePlatformUp(e); // otherwise start slowing descent/moving upwards
        ChkYPCollision(e);
        return;
    } // YMDown: start slowing ascent/moving downwards
    MovePlatformDown(e);

    ChkYPCollision(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveLargeLiftPlat(uint8_t e)
{
    MoveLiftPlatforms(e); // execute common to all large and small lift platforms
    ChkYPCollision(e);     // branch to position player correctly
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveSmallPlatform(uint8_t e)
{
    MoveLiftPlatforms(e);     // execute common to all large and small lift platforms
    ChkSmallPlatCollision(e); // branch to position player correctly
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveLiftPlatforms(uint8_t e)
{
    uint32_t wide = 0;

    if (M(TimerControl) != 0) // if master timer control set, skip all of this
    {
        return; // and branch to leave
    }
    // position:dummy and speed:force are each one 16-bit quantity
    wide = ((M(Enemy_Y_Position + e) << 8) | M(Enemy_YMF_Dummy + e)) +
           ((M(Enemy_Y_Speed + e) << 8) | M(Enemy_Y_MoveForce + e)); // move up or down
    writeData(Enemy_YMF_Dummy + e, LOBYTE(wide));
    writeData(Enemy_Y_Position + e, HIBYTE(wide)); // and then leave
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::ChkSmallPlatCollision(uint8_t e)
{
    const uint8_t collisionFlag = M(PlatformCollisionFlag + e); // bounding box counter saved in collision flag
    if (collisionFlag == 0)
    {
        return; // if none found, leave player position alone
    }
    PositionPlayerOnS_Plat(collisionFlag, e); // use to position player correctly

    // ExLiftP: then leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none (x is reloaded from ObjectOffset on every exit path)
void SMBEngine::SmallPlatformCollision()
{
    // The search reports whether it hit the player. Both of its outcomes reload the object
    // offset below (ExSPC), and a hit goes on to the shared platform collision code
    // (ProcSPlatCollisions), which reads the bounding box offset the search left in y.
    uint8_t boundBoxOfs = 0; // the box the hit was found on, needed by ProcLPlatCollisions
    const auto findCollision = [&]() -> bool
    {
        if (M(TimerControl) != 0)
        {
            return false; // master timer control set, leave
        }
        const uint8_t e = M(ObjectOffset);          // get enemy object offset
        writeData(PlatformCollisionFlag + e, 0x00); // otherwise initialize collision flag
        // leave if the player is below a certain point, or entirely offscreen
        if (CheckPlayerVertical())
        {
            return false;
        }
        writeData(0x00, 0x02); // load counter here for 2 bounding boxes

        do // ChkSmallPlatLoop
        {
            uint8_t offscreenBits;
            std::tie(boundBoxOfs, offscreenBits) = GetEnemyBoundBoxOfs(); // get bounding box offset
            if ((offscreenBits & 0b00000010) != 0)
            {
                return false; // d1 of offscreen lower nybble bits was set, leave
            }
            // check top of platform's bounding box for being in range
            if (M(BoundingBox_UL_YPos + boundBoxOfs) >= 0x20)
            {
                // perform player-to-platform collision detection
                if (PlayerCollisionCore(boundBoxOfs))
                {
                    return true;
                }
            }
            // MoveBoundBox: move bounding box vertical coordinates
            writeData(BoundingBox_UL_YPos + boundBoxOfs, M(BoundingBox_UL_YPos + boundBoxOfs) + 0x80);
            writeData(BoundingBox_DR_YPos + boundBoxOfs, M(BoundingBox_DR_YPos + boundBoxOfs) + 0x80);
            --M(0x00); // decrement counter we set earlier
        } while (M(0x00) != 0); // loop back until both bounding boxes are checked
        return false;
    };
    const bool collisionFound = findCollision();
    if (collisionFound)
    {
        // ExSPC / ProcSPlatCollisions
        ProcLPlatCollisions(boundBoxOfs, M(ObjectOffset));
    }
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (the platform, or the other object in a collision
// pair); boundBoxOfs = the platform's bounding box offset, set by an earlier GetEnemyBoundBoxOfs*
// call
// Outputs: none
void SMBEngine::ProcLPlatCollisions(uint8_t boundBoxOfs, uint8_t e)
{
    // get difference by subtracting the top of the platform's bounding box; a player close
    // underneath it and moving upwards has its jump killed
    const uint8_t platformTopDiff = static_cast<uint8_t>(M(BoundingBox_DR_YPos + boundBoxOfs) - M(BoundingBox_UL_YPos));
    if (platformTopDiff < 0x04 && (M(Player_Y_Speed) & 0x80) != 0)
    {
        writeData(Player_Y_Speed, 0x01); // set vertical speed of player to kill jump
    }

    // ChkForTopCollision: get difference by subtracting the top of the player's bounding box;
    // close enough with the player not moving upwards means it landed on the platform
    const uint8_t playerTopDiff = static_cast<uint8_t>(M(BoundingBox_DR_YPos) - M(BoundingBox_UL_YPos + boundBoxOfs));
    const bool landedOnPlatform = (playerTopDiff < 0x06) && ((M(Player_Y_Speed) & 0x80) == 0);
    if (landedOnPlatform)
    {
        // the two large lift IDs record the saved bounding box counter, everything else records
        // the enemy object buffer offset
        const uint8_t enemyId = M(Enemy_ID + e);
        const bool usesBoundBoxCounter = (enemyId == 0x2b) || (enemyId == 0x2c);
        const uint8_t collisionFlag = usesBoundBoxCounter ? M(0x00) : e;

        // SetCollisionFlag
        const uint8_t self = M(ObjectOffset);                   // get enemy object buffer offset
        writeData(PlatformCollisionFlag + self, collisionFlag); // save either bounding box counter or enemy offset here
        writeData(Player_State, 0x00);                          // set player state to normal then leave
        return;
    }

    // PlatformSideCollisions: set value here to indicate possible horizontal collision on the
    // left side of the platform
    writeData(0x00, 0x01);
    // get difference by subtracting platform's left edge
    const uint8_t leftEdgeDiff = static_cast<uint8_t>(M(BoundingBox_DR_XPos) - M(BoundingBox_UL_XPos + boundBoxOfs));
    bool sideCollision = true;
    if (leftEdgeDiff >= 0x08)
    {
        ++M(0x00); // otherwise increment value set here for right side collision
        // get difference by subtracting player's left edge from platform's right edge
        // the original clears the carry rather than setting it here, so the
        // subtraction takes one pixel more than it means to
        const uint8_t rightEdgeDiff = static_cast<uint8_t>(M(BoundingBox_DR_XPos + boundBoxOfs) - M(BoundingBox_UL_XPos) - 1);
        sideCollision = rightEdgeDiff < 0x09; // too far away is no collision at all
    }
    if (sideCollision)
    {
        // SideC: deal with horizontal collision
        ImpedePlayerMove();
    } // NoSideC: leave
}

//------------------------------------------------------------------------

// otherwise increment two bytes
// Inputs: none
// Outputs: x is reloaded from ObjectOffset
void SMBEngine::Inc2B()
{
    ++M(EnemyDataOffset);
    ++M(EnemyDataOffset);
    writeData(EnemyObjectPageSel, 0x00); // init page select for enemy objects
}

//------------------------------------------------------------------------

// Inputs: none (acts on the current object offset)
// Outputs: none
void SMBEngine::WarpZoneObject()
{
    // check for scroll lock flag
    if (M(ScrollLock) == 0)
    {
        return; // branch if not set to leave
    }
    // check to see if player's vertical coordinate has the same bits set as in vertical high
    // byte (why?)
    if ((M(Player_Y_Position) & M(Player_Y_HighPos)) != 0)
    {
        return; // if so, branch to leave
    }
    writeData(ScrollLock, 0x00);          // otherwise nullify scroll lock flag
    ++M(WarpZoneControl);                 // increment warp zone flag to make warp pipes for warp zone
    EraseEnemyObject(M(ObjectOffset));    // kill this object
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (must be 5, the last slot, or this exits immediately)
// Outputs: x is reloaded from ObjectOffset
void SMBEngine::VineObjectHandler()
{
    const uint8_t VineHeightData_data[] = {0x30, 0x60};

    // Every exit lands on the same tail (ExitVH), so the body is a lambda that returns and the
    // offset reload below runs unconditionally afterwards.
    const auto handleVine = [&]()
    {
        if (x != 0x05)
        {
            return; // if not in last slot, leave
        }
        y = M(VineFlagOffset);
        --y; // decrement vine flag in Y, use as offset

        // Grow the vine, unless it has already reached its full height, or this is not one of
        // the two frames in four that d1 of the frame counter selects.
        const bool atFullHeight = M(VineHeight) == VineHeightData_data[y];
        const bool growthFrame = (M(FrameCounter) & 0b00000010) != 0;
        if (!atFullHeight && growthFrame)
        {
            // subtract vertical position of vine one pixel every frame it's time
            writeData(Enemy_Y_Position + 5, M(Enemy_Y_Position + 5) - 0x01);
            ++M(VineHeight); // increment vine height
        }

        // RunVSubs: leave if vine still very small
        if (M(VineHeight) < 0x08)
        {
            return;
        }
        RelativeEnemyPosition(x); // get relative coordinates of vine,
        GetEnemyOffscreenBits(x); // and any offscreen bits
        y = 0x00;                // initialize offset used in draw vine sub

        do // VDrawLoop: draw vine
        {
            DrawVine(y);
            ++y; // increment offset
        } while (y != M(VineFlagOffset)); // do not yet match, loop back to draw more vine

        // if none of the saved offscreen bits set, skip ahead
        if ((M(Enemy_OffscreenBits) & 0b00001100) != 0)
        {
            --y; // otherwise decrement Y to get proper offset again

            do // KillVine: get enemy object offset for this vine object
            {
                x = M(VineObjOffset + y);
                EraseEnemyObject(x); // kill this vine object
                --y;                 // decrement Y
            } while ((y & 0x80) == 0); // if any vine objects left, loop back to kill it
            // The original re-used the zero EraseEnemyObject leaves in the accumulator here.
            writeData(VineFlagOffset, 0x00); // initialize vine flag/offset
            writeData(VineHeight, 0x00);     // initialize vine height
        }

        // WrCMTile: check vine height, leave if it is not tall enough yet
        if (M(VineHeight) < 0x20)
        {
            return;
        }
        // get block at ($04, $10) of coordinates from the last enemy slot; the horizontal
        // result lands in $04, but we don't care about it
        BlockBufferCollision(0x01, 0x06, 0x1b);
        const uint8_t blockOffset = M(0x02);
        if (blockOffset >= 0xd0)
        {
            return; // outside the current block buffer, leave, do not write
        }
        if (M(W(0x06) + blockOffset) != 0)
        {
            return; // block buffer not empty at the current offset, leave
        }
        writeData(W(0x06) + blockOffset, 0x26); // otherwise, write climbing metatile to block buffer
    };
    handleVine();

    // ExitVH: get enemy object offset and leave
    x = M(ObjectOffset);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitLakitu(uint8_t e)
{
    // check to see if an enemy is already in
    if (M(EnemyFrenzyBuffer) == 0)
    { // the frenzy buffer, and branch to kill lakitu if so

        SetupLakitu(e);
        return;
    } // KillLakitu
    EraseEnemyObject(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: x is reloaded from ObjectOffset
void SMBEngine::DrawSmallPlatform(uint8_t e)
{
    const uint8_t oamOffset = M(Enemy_SprDataOffset + e); // get OAM data offset

    DumpSixSpr(0x5b, oamOffset + 1); // dump tile number for small platforms into all six sprites
    DumpSixSpr(0x02, oamOffset + 2); // dump palette controls into all six sprites

    // get relative horizontal coordinate and dump it as the X coordinate into the first and
    // fourth sprites, then eight pixels along for each pair after that
    const uint8_t relX = M(Enemy_Rel_XPos);
    writeData(Sprite_X_Position + oamOffset, relX);
    writeData(Sprite_X_Position + 12 + oamOffset, relX);
    writeData(Sprite_X_Position + 4 + oamOffset, relX + 0x08); // dump into second and fifth sprites
    writeData(Sprite_X_Position + 16 + oamOffset, relX + 0x08);
    writeData(Sprite_X_Position + 8 + oamOffset, relX + 0x10); // dump into third and sixth sprites
    writeData(Sprite_X_Position + 20 + oamOffset, relX + 0x10);

    // get vertical coordinate; anything above the top of the screen goes offscreen instead
    const uint8_t yPos = M(Enemy_Y_Position + e);
    // TopSP: dump vertical coordinate into Y coordinates
    DumpThreeSpr((yPos < 0x20) ? 0xf8 : yPos, oamOffset);

    // BotSP: dump vertical coordinate + 128 pixels into Y coordinates, offscreen if it lands
    // above the top of the screen
    const uint8_t botYPos = yPos + 0x80;
    const uint8_t bot = (botYPos < 0x20) ? 0xf8 : botYPos;
    writeData(Sprite_Y_Position + 12 + oamOffset, bot);
    writeData(Sprite_Y_Position + 16 + oamOffset, bot);
    writeData(Sprite_Y_Position + 20 + oamOffset, bot);

    // Each of d3-d1 of the offscreen bits moves one column's pair of sprites offscreen.
    const uint8_t offscreenBits = M(Enemy_OffscreenBits);
    if ((offscreenBits & 0b00001000) != 0)
    {
        // if d3 was set, move first and fourth sprites offscreen
        writeData(Sprite_Y_Position + oamOffset, 0xf8);
        writeData(Sprite_Y_Position + 12 + oamOffset, 0xf8);
    } // SOfs
    if ((offscreenBits & 0b00000100) != 0)
    {
        // if d2 was set, move second and fifth sprites offscreen
        writeData(Sprite_Y_Position + 4 + oamOffset, 0xf8);
        writeData(Sprite_Y_Position + 16 + oamOffset, 0xf8);
    } // SOfs2
    if ((offscreenBits & 0b00000010) != 0)
    {
        // if d1 was set, move third and sixth sprites offscreen
        writeData(Sprite_Y_Position + 8 + oamOffset, 0xf8);
        writeData(Sprite_Y_Position + 20 + oamOffset, 0xf8);
    } // ExSPl
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::JumpspringHandler()
{
    const uint8_t Jumpspring_Y_PosData_data[] = {0x08, 0x10, 0x08, 0x00};

    GetEnemyOffscreenBits(x); // get offscreen information

    // The master timer control, or a jumpspring frame control of zero, means there is nothing to
    // animate; skip straight to drawing it.
    const bool animating = (M(TimerControl) == 0) && (M(JumpspringAnimCtrl) != 0);
    if (animating)
    {
        // subtract one from frame control, and use d1 of it to pick the direction
        const uint8_t frameCtrl = M(JumpspringAnimCtrl) - 1;
        if ((frameCtrl & 0b00000010) == 0)
        {
            M(Player_Y_Position) += 2; // move player's vertical position down two pixels
        } // DownJSpr
        else
        {
            M(Player_Y_Position) -= 2; // move player's vertical position up two pixels
        }

        // PosJSpr: the permanent vertical position plus a value using frame control as offset
        writeData(Enemy_Y_Position + x, M(Jumpspring_FixedYPos + x) + Jumpspring_Y_PosData_data[frameCtrl]);

        // From the third frame ($01) on, a fresh A press (not held from the previous frame)
        // writes a new jumpspring force.
        if (frameCtrl >= 0x01)
        {
            const uint8_t aPressed = M(A_B_Buttons) & A_Button; // check saved controller bits for A button press
            if (aPressed != 0 && (aPressed & M(PreviousA_B_Buttons)) == 0)
            {
                writeData(JumpspringForce, 0xf4);
            }
        }

        // BounceJS: at the fifth frame ($03) the force becomes the player's vertical speed
        if (frameCtrl == 0x03)
        {
            writeData(Player_Y_Speed, M(JumpspringForce)); // store jumpspring force as player's new vertical speed
            writeData(JumpspringAnimCtrl, 0x00);           // initialize jumpspring frame control
        }
    }

    // DrawJSpr: get jumpspring's relative coordinates
    RelativeEnemyPosition(x);
    EnemyGfxHandler(x);        // draw jumpspring
    OffscreenBoundsCheck(x);   // check to see if we need to kill it
    a = M(JumpspringAnimCtrl); // if frame control at zero, don't bother
    if (a == 0)
    {
        return; // trying to animate it, just leave
    }
    a = M(JumpspringTimer);
    if (a != 0)
    {
        return; // if jumpspring timer not expired yet, leave
    }
    a = 0x04;
    writeData(JumpspringTimer, 0x04); // otherwise initialize jumpspring timer
    ++M(JumpspringAnimCtrl);          // increment frame control to animate jumpspring

    // ExJSpring: leave
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::RunRetainerObj(uint8_t e)
{
    GetEnemyOffscreenBits(e);
    RelativeEnemyPosition(e);
    EnemyGfxHandler(e);
}

//------------------------------------------------------------------------

// Inputs: none (always operates on the fixed power-up slot 5, and reads PowerUpType from memory)
// Outputs: none
void SMBEngine::DrawPowerUp()
{
    const uint8_t PowerUpAttributes_data[] = {0x02, 0x01, 0x02, 0x01};

    const uint8_t PowerUpGfxTable_data[] = {
        0x76, 0x77, 0x78, 0x79, // regular mushroom
        0xd6, 0xd6, 0xd9, 0xd9, // fire flower
        0x8d, 0x8d, 0xe4, 0xe4, // star
        0x76, 0x77, 0x78, 0x79  // 1-up mushroom
    };

    const uint8_t powerUpType = M(PowerUpType); // get power-up type

    y = M(Enemy_SprDataOffset + 5);            // get power-up's sprite data offset
    writeData(0x02, M(Enemy_Rel_YPos) + 0x08); // relative vertical coordinate plus eight pixels
    writeData(0x05, M(Enemy_Rel_XPos));        // store relative horizontal coordinate here
    // get attribute data for power-up type, adding the background priority bit if set
    writeData(0x04, PowerUpAttributes_data[powerUpType] | M(Enemy_SprAttrib + 5));

    x = powerUpType << 2;  // multiply by four to get proper offset into the gfx table
    writeData(0x07, 0x01); // set counter here to draw two rows of sprite object
    writeData(0x03, 0x01); // init d1 of flip control

    do // PUpDrawLoop
    {
        // load left tile of power-up object
        writeData(0x00, PowerUpGfxTable_data[x]);
        a = PowerUpGfxTable_data[1 + x]; // load right tile
        DrawOneSpriteRow(a, x, y);       // branch to draw one row of our power-up object
        --M(0x07);                       // decrement counter
    } while ((M(0x07) & 0x80) == 0); // branch until two rows are drawn
    y = M(Enemy_SprDataOffset + 5); // get sprite data offset again

    // Only the fire flower and the star cycle colors and flip; the regular mushroom (0) and the
    // 1-up mushroom (3) are drawn as-is.
    const bool cyclesColors = (powerUpType != 0x00) && (powerUpType != 0x03);
    if (cyclesColors)
    {
        writeData(0x00, powerUpType); // store power-up type here now
        // frame counter divided by 2 to change colors every two frames, masked down to what
        // were d2 and d1, plus the background priority bit if any set
        const uint8_t paletteBits = ((M(FrameCounter) >> 1) & 0b00000011) | M(Enemy_SprAttrib + 5);
        writeData(Sprite_Attributes + y, paletteBits);     // set as new palette bits for top left and
        writeData(Sprite_Attributes + 4 + y, paletteBits); // top right sprites for fire flower and star
        if (powerUpType != 0x01)
        {                                                       // skip this part for the fire flower
            writeData(Sprite_Attributes + 8 + y, paletteBits);  // otherwise set new palette bits for bottom left
            writeData(Sprite_Attributes + 12 + y, paletteBits); // and bottom right sprites as well for star only
        } // FlipPUpRightSide
        // set horizontal flip bit for top right and bottom right sprites; note these are only
        // done for fire flower and star power-ups
        writeData(Sprite_Attributes + 4 + y, M(Sprite_Attributes + 4 + y) | 0b01000000);
        writeData(Sprite_Attributes + 12 + y, M(Sprite_Attributes + 12 + y) | 0b01000000);
    }

    // PUpOfs: check to see if power-up is offscreen at all, then leave
    SprObjectOffscrChk();
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (forwarded to SetHiMax)
// Outputs: none
void SMBEngine::MoveFallingPlatform(uint8_t e)
{
    SetHiMax(e, 0x20); // set movement amount
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveLakitu(uint8_t e)
{
    // check lakitu's enemy state for d5 set
    if ((M(Enemy_State + e) & 0b00100000) != 0)
    {
        MoveD_EnemyVertically(e); // jump to move defeated lakitu downwards
        return;
    }

    uint8_t moveSpeed = 0;
    // ChkLS: if lakitu's enemy state is not set at all, go ahead and continue with code
    if (M(Enemy_State + e) != 0)
    {
        writeData(LakituMoveDirection + e, 0x00); // otherwise initialize moving direction to move to left
        writeData(EnemyFrenzyBuffer, 0x00);       // initialize frenzy buffer
        moveSpeed = 0x10;                         // load horizontal speed
    }
    else
    {
        // Fr12S
        writeData(EnemyFrenzyBuffer, Spiny); // set spiny identifier in frenzy buffer

        // LdLDa: load values and store in zero page, until all values are stored
        for (int i = 0x02; i >= 0; --i)
        {
            writeData(0x0001 + i, M(LakituDiffAdj + i));
        }
        // execute sub to set speed and create spinys; the sub returns the movement speed
        moveSpeed = PlayerLakituDiff(e);
    }

    // SetLSpd: set movement speed returned from sub
    writeData(LakituMoveSpeed + e, moveSpeed);

    // move to the right by default; if the LSB of the moving direction is clear, negate the
    // speed and move to the left instead
    const bool moveRight = (M(LakituMoveDirection + e) & 0x01) != 0;
    if (!moveRight)
    {
        // get two's compliment of moving speed and store as new moving speed
        writeData(LakituMoveSpeed + e, (M(LakituMoveSpeed + e) ^ 0xff) + 0x01);
    }
    // SetLMov: store moving direction
    writeData(Enemy_MovingDir + e, moveRight ? 0x01 : 0x02);
    MoveEnemyHorizontally(e); // move lakitu horizontally
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none (most exit paths reload x from ObjectOffset via DoOtherPlatform; a couple of
// early returns leave x already equal to the enemy offset)
void SMBEngine::BalancePlatform(uint8_t e)
{
    // check high byte of vertical position
    if (M(Enemy_Y_HighPos + e) == 0x03)
    {
        EraseEnemyObject(e); // if far below screen, kill the object
        return;
    } // DoBPl: get object's state (set to $ff or other platform offset)
    const uint8_t state = M(Enemy_State + e);
    if ((state & 0x80) != 0)
    {
        return; // if doing other balance platform, branch to leave
    }

    // CheckBalPlatform: the state doubles as the other platform's offset
    const uint8_t otherPlatform = state;
    // get collision flag of platform and store here
    writeData(0x00, M(PlatformCollisionFlag + e));

    // get moving direction
    if (M(Enemy_MovingDir + e) != 0)
    {
        MoveFallingPlatform(e);             // make current platform fall
        MoveFallingPlatform(otherPlatform); // make other platform fall

        // if player not standing on either platform, skip this part
        const uint8_t collisionFlag = M(PlatformCollisionFlag + M(ObjectOffset));
        if ((collisionFlag & 0x80) == 0)
        {
            // the flag doubles as the offset of the platform collided with; position the player
            // accordingly
            PositionPlayerOnVPlat(collisionFlag);
        } // ExPF: leave
        return;
    } // ChkForFall

    // Either platform reaching the top breaks the pair. Whichever one it is, the points and the
    // falling flag go to the platform named by the state, and $00 holds the collision flag: if
    // it names the platform at the other end, the player is on it and gets the 1000 points.
    const auto checkBreak = [&](uint8_t which, uint8_t otherOfPair)
    {
        if (0x2d < M(Enemy_Y_Position + which))
        {
            return false;
        }
        if (otherOfPair == M(0x00))
        {
            // The offscreen bits are wanted for the other platform, but everything below lands
            // on this platform (via ObjectOffset), not the other one.
            GetEnemyOffscreenBits(otherPlatform); // get offscreen bits
            const uint8_t self = M(ObjectOffset);
            SetupFloateyNumber(6, self); // award 1000 points to player
            // put floatey number coordinates where player is
            writeData(FloateyNum_X_Pos + self, M(Player_Rel_XPos));
            writeData(FloateyNum_Y_Pos + self, M(Player_Y_Position));
            writeData(Enemy_MovingDir + self, 0x01); // falling platforms

            StopPlatforms(self, otherPlatform);
            return true;
        }
        // otherwise add 2 pixels to vertical position of current platform and branch elsewhere
        writeData(Enemy_Y_Position + which, 0x2f);
        StopPlatforms(e, otherPlatform); // to make platforms stop
        return true;
    };

    if (checkBreak(e, otherPlatform))
    {
        return;
    }
    if (checkBreak(otherPlatform, e))
    {
        return;
    }

    const uint8_t oldYPos = M(Enemy_Y_Position + e); // remember the vertical position before the move
    const uint8_t collisionFlag = M(PlatformCollisionFlag + e); // get collision flag
    if ((collisionFlag & 0x80) != 0)
    { // branch if collision
        // add $05 to contents of moveforce, whatever they be, as one 16-bit speed:force
        const uint32_t wide = ((M(Enemy_Y_Speed + e) << 8) | M(Enemy_Y_MoveForce + e)) + 0x05;
        writeData(0x00, LOBYTE(wide)); // store here
        const uint8_t speedPlusCarry = HIBYTE(wide); // the vertical speed, plus the carry
        if ((speedPlusCarry & 0x80) != 0)
        {
            MovePlatformDown(e);
            DoOtherPlatform(oldYPos, e);
            return;
        }
        if (speedPlusCarry != 0)
        {
            MovePlatformUp(e);
            DoOtherPlatform(oldYPos, e); // jump ahead to remaining code
            return;
        }
        if (M(0x00) < 0x0b)
        {
            StopPlatforms(e, otherPlatform);
            DoOtherPlatform(oldYPos, e); // jump ahead to remaining code
            return;
        }
        MovePlatformUp(e);
        DoOtherPlatform(oldYPos, e); // jump ahead to remaining code
        return;
    } // ColFlg: if collision flag matches

    if (collisionFlag == M(ObjectOffset))
    {
        MovePlatformDown(e);
        DoOtherPlatform(oldYPos, e);
        return;
    }

    MovePlatformUp(e);
    DoOtherPlatform(oldYPos, e); // jump ahead to remaining code
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::ProcMoveRedPTroopa(uint8_t e)
{
    // with no vertical force or speed, and still above its original coordinate, the paratroopa
    // just drifts down a pixel every eighth frame; otherwise fall through to MoveRedPTUpOrDown
    const bool anyVerticalMovement = (M(Enemy_Y_Speed + e) | M(Enemy_Y_MoveForce + e)) != 0;
    if (!anyVerticalMovement)
    {
        writeData(Enemy_YMF_Dummy + e, 0x00); // initialize something here
        // check current vs. original vertical coordinate
        if (M(Enemy_Y_Position + e) < M(RedPTroopaOrigXPos + e))
        {
            if ((M(FrameCounter) & 0b00000111) == 0)
            {
                ++M(Enemy_Y_Position + e); // increment red paratroopa's vertical position
            } // NoIncPT: leave
            return;
        }
    }

    // MoveRedPTUpOrDown
    // check current vs. central vertical coordinate
    if (M(Enemy_Y_Position + e) >= M(RedPTroopaCenterYPos + e))
    { // if current < central, jump to move downwards
        // otherwise jump to move upwards
        MoveRedPTroopa(0x01, e);
        return;

    } // MovPTDwn: move downwards
    MoveRedPTroopaDown(e);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (the long firebar's own slot; DuplicateEnemyObj
// creates its duplicate half)
// Outputs: none
void SMBEngine::InitLongFirebar(uint8_t e)
{
    DuplicateEnemyObj(e); // create enemy object for long firebar

    InitShortFirebar(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (bowser's front half slot)
// Outputs: none
void SMBEngine::InitBowser(uint8_t e)
{
    DuplicateEnemyObj(e);                               // jump to create another bowser object
    writeData(BowserFront_Offset, e);                   // save offset of first here
    writeData(BowserBodyControls, 0x00);                // initialize bowser's body controls
    writeData(BridgeCollapseOffset, 0x00);              // and bridge collapse offset
    writeData(BowserOrigXPos, M(Enemy_X_Position + e)); // store original horizontal position here
    writeData(BowserFireBreathTimer, 0xdf);             // store something here
    writeData(Enemy_MovingDir + e, 0xdf);               // and in moving direction
    writeData(BowserFeetCounter, 0x20);                 // set bowser's feet timer and in enemy timer
    writeData(EnemyFrameTimer + e, 0x20);
    writeData(BowserHitPoints, 0x05); // give bowser 5 hit points
    writeData(BowserMovementSpeed, 0x02); // set default movement speed here
}

//------------------------------------------------------------------------

// Inputs: e = original enemy object buffer offset (the one being duplicated)
// Outputs: the new enemy's slot is stashed in DuplicateObj_Offset memory, which is
// how callers actually retrieve it
void SMBEngine::DuplicateEnemyObj(uint8_t e)
{
    uint8_t slot = 0xff; // start at beginning of enemy slots

    do // FSLoop: increment one slot
    {
        ++slot;
        // check enemy buffer flag for empty slot
    } while (M(Enemy_Flag + slot) != 0); // if set, branch and keep checking
    writeData(DuplicateObj_Offset, slot); // otherwise set offset here
    // transfer original enemy buffer offset, with d7 set as flag in new enemy
    writeData(Enemy_Flag + slot, e | 0b10000000);          // slot as well as enemy offset
    writeData(Enemy_PageLoc + slot, M(Enemy_PageLoc + e)); // copy page location and horizontal coordinates
    // from original enemy to new enemy
    writeData(Enemy_X_Position + slot, M(Enemy_X_Position + e));
    writeData(Enemy_Flag + e, 0x01);         // set flag as normal for original enemy
    writeData(Enemy_Y_HighPos + slot, 0x01); // set high vertical byte for new enemy
    // copy vertical coordinate from original to new
    writeData(Enemy_Y_Position + slot, M(Enemy_Y_Position + e));
}

//------------------------------------------------------------------------

// Inputs: none (uses BowserFlameTimerCtrl from memory as its own index)
// Outputs: a = timer value selected from FlameTimerData_data, used by both callers
uint8_t SMBEngine::SetFlameTimer()
{
    const uint8_t FlameTimerData_data[] = {0xbf, 0x40, 0xbf, 0xbf, 0xbf, 0x40, 0x40, 0xbf};

    const uint8_t counter = M(BowserFlameTimerCtrl); // load counter as offset
    ++M(BowserFlameTimerCtrl);                        // increment
    // mask out all but 3 LSB to keep in range of 0-7
    writeData(BowserFlameTimerCtrl, M(BowserFlameTimerCtrl) & 0b00000111);
    return FlameTimerData_data[counter]; // value to be used then leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (bowser's flame slot)
// Outputs: none
void SMBEngine::ProcBowserFlame()
{
    bool shiftedBit = false;
    uint32_t wide = 0;

    // the master timer control flag freezes the flame in place; skip straight to SetGfxF
    if (M(TimerControl) == 0)
    {
        // the default movement force, or an alternate one to go faster in secondary hard mode
        const uint8_t moveForce = (M(SecondaryHardMode) != 0) ? 0x60 : 0x40;
        // SFlmX: store value here
        writeData(0x00, moveForce);
        // pageloc:position:force is one 24-bit quantity
        wide = (M(Enemy_PageLoc + x) << 16) | (M(Enemy_X_Position + x) << 8) | M(Enemy_X_MoveForce + x);
        wide -= (0x01 << 8) | M(0x00);                  // subtract value from movement force and one pixel from the position
        writeData(Enemy_X_MoveForce + x, LOBYTE(wide)); // save new value
        writeData(Enemy_X_Position + x, HIBYTE(wide));  // to move to the left
        writeData(Enemy_PageLoc + x, (uint8_t)(wide >> 16));

        y = M(BowserFlamePRandomOfs + x); // get some value here and use as offset
        // once the flame reaches its target coordinate, stop modifying it
        if (M(Enemy_Y_Position + x) != M(FlameYPosData + y))
        {
            // otherwise add the movement force to the coordinate and store as the new one
            writeData(Enemy_Y_Position + x, M(Enemy_Y_Position + x) + M(Enemy_Y_MoveForce + x));
        }
    }

    // SetGfxF: get new relative coordinates
    RelativeEnemyPosition(x);
    a = M(Enemy_State + x); // if bowser's flame not in normal state,
    if (a != 0)
    {
        return; // branch to leave
    }
    // otherwise, continue
    writeData(0x00, 0x51);            // write first tile number
    y = 0x02;                         // load attributes without vertical flip by default
    a = M(FrameCounter) & 0b00000010; // invert vertical flip bit every 2 frames
    if (a != 0)
    {             // if d1 not set, write default value
        y = 0x82; // otherwise write value with vertical flip bit set
    } // FlmeAt: set bowser's flame sprite attributes here
    writeData(0x01, y);
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    x = 0x00;

    do // DrawFlameLoop
    {
        // get Y relative coordinate of current enemy object
        writeData(Sprite_Y_Position + y, M(Enemy_Rel_YPos)); // write into Y coordinate of OAM data
        writeData(Sprite_Tilenumber + y, M(0x00));           // write current tile number into OAM data
        ++M(0x00);                                           // increment tile number to draw more bowser's flame
        writeData(Sprite_Attributes + y, M(0x01));           // write saved attributes into OAM data
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
    x = M(ObjectOffset);            // reload original enemy offset
    GetEnemyOffscreenBits(x);        // get offscreen information
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    // get enemy object offscreen bits
    a = M(Enemy_OffscreenBits) >> 1; // take d0, and push the rest to the stack
    pha();
    if ((M(Enemy_OffscreenBits) & 0x01) != 0)
    {                                                // branch if it was not set
        a = 0xf8;                                    // otherwise move sprite offscreen, this part likely
        writeData(Sprite_Y_Position + 12 + y, 0xf8); // residual since flame is only made of three sprites
    } // M3FOfs: get bits from stack
    pla();
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // take d1, and push the bits back to the stack
    pha();
    if (shiftedBit)
    {             // branch if it was not set again
        a = 0xf8; // otherwise move third sprite offscreen
        writeData(Sprite_Y_Position + 8 + y, 0xf8);
    } // M2FOfs: get bits from stack again
    pla();
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // take d2, and push the bits back to the stack again
    pha();
    if (shiftedBit)
    {             // branch if it was not set yet again
        a = 0xf8; // otherwise move second sprite offscreen
        writeData(Sprite_Y_Position + 4 + y, 0xf8);
    } // M1FOfs: get bits from stack one last time
    pla();
    shiftedBit = (a & 0x01) != 0;
    if (shiftedBit) // and d3
    {               // branch if it was not set one last time
        a = 0xf8;
        writeData(Sprite_Y_Position + y, 0xf8); // otherwise move first sprite offscreen
    } // ExFlmeD: leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (fireworks object's slot)
// Outputs: none
void SMBEngine::RunFireworks()
{
    --M(ExplosionTimerCounter + x); // decrement explosion timing counter here
    if (M(ExplosionTimerCounter + x) == 0)
    {                                               // if not expired, skip this part
        writeData(ExplosionTimerCounter + x, 0x08); // reset counter
        ++M(ExplosionGfxCounter + x);               // increment explosion graphics counter
        if (M(ExplosionGfxCounter + x) >= 0x03)
        {
            // FireworksSoundScore: the explosion has run its course, kill this object
            writeData(Enemy_Flag + x, 0x00);         // disable enemy buffer flag
            writeData(Square2SoundQueue, Sfx_Blast); // play fireworks/gunfire sound
            writeData(DigitModifier + 4, 0x05);      // set part of score modifier for 500 points
            EndAreaPoints();                         // award points accordingly then leave
            return;
        }
    } // SetupExpl: get relative coordinates of explosion
    RelativeEnemyPosition(x);
    // copy relative coordinates
    writeData(Fireball_Rel_YPos, M(Enemy_Rel_YPos)); // from the enemy object to the fireball object
    // first vertical, then horizontal
    writeData(Fireball_Rel_XPos, M(Enemy_Rel_XPos));
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    a = M(ExplosionGfxCounter + x); // get explosion graphics counter
    DrawExplosion_Fireworks(a, y);  // do a sub to draw the explosion then leave
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: x is reloaded from ObjectOffset
void SMBEngine::DrawLargePlatform(uint8_t e)
{
    const uint8_t oamOffset = M(Enemy_SprDataOffset + e); // get OAM data offset
    writeData(0x02, oamOffset);                           // store here

    // get horizontal relative coordinate; store X coordinates using it as base, stack
    // horizontally, starting 3 bytes along at the X coordinate
    SixSpriteStacker(M(Enemy_Rel_XPos), oamOffset + 3);

    const uint8_t objOffset = M(ObjectOffset);
    const uint8_t platformYPos = M(Enemy_Y_Position + objOffset); // get vertical coordinate
    DumpFourSpr(platformYPos, oamOffset);                         // dump into first four sprites as Y coordinate

    // ShrinkPlatform: castle-type levels and secondary hard mode move the last two sprites
    // offscreen, shrinking the platform
    const bool shrinkPlatform = (M(AreaType) == 0x03) || (M(SecondaryHardMode) != 0);
    const uint8_t lastTwoYPos = shrinkPlatform ? 0xf8 : platformYPos;

    // SetLast2Platform: store vertical coordinate or offscreen coordinate into the last two
    // sprites as Y coordinate
    writeData(Sprite_Y_Position + 16 + oamOffset, lastTwoYPos);
    writeData(Sprite_Y_Position + 20 + oamOffset, lastTwoYPos);

    // the girder is the default tile; cloud levels use a puff instead
    const uint8_t platformTile = (M(CloudTypeOverride) != 0) ? 0x75 : 0x5b;
    // SetPlatformTilenum
    DumpSixSpr(platformTile, oamOffset + 1); // dump tile number into all six sprites
    DumpSixSpr(0x02, oamOffset + 2);         // dump palette controls into all six sprites

    // get offscreen bits again, for the enemy objects one byte along
    uint8_t offscreenBits = GetXOffscreenBits(objOffset + 1);

    // SChk2 through SChk6: d7 down to d2 each move one sprite offscreen, in order
    for (int sprite = 0; sprite < 6; ++sprite)
    {
        if ((offscreenBits & 0x80) != 0)
        {
            writeData(Sprite_Y_Position + (sprite * 4) + oamOffset, 0xf8);
        }
        offscreenBits <<= 1;
    }

    // SLChk: check d7 of offscreen bits, and if d7 is not set, skip sub
    if ((M(Enemy_OffscreenBits) & 0x80) != 0)
    {
        MoveSixSpritesOffscreen(oamOffset); // otherwise branch to move all sprites offscreen
    } // ExDLPl
}

//------------------------------------------------------------------------

// if row = $0e, increment three bytes
// Inputs: none
// Outputs: x is reloaded from ObjectOffset (via Inc2B)
void SMBEngine::Inc3B()
{
    ++M(EnemyDataOffset);
    Inc2B();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::XMovingPlatform(uint8_t e)
{
    XMoveCntr_Platform(0x0e, e); // do a sub to increment counters for movement, with the preset
                                 // maximum value for the secondary counter
    MoveWithXMCntrs(e);          // do a sub to move platform accordingly, and return value
    // if no collision with player, branch ahead to leave
    if ((M(PlatformCollisionFlag + e) & 0x80) != 0)
    {
        return;
    }
    PositionPlayerOnHPlat(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (forwarded to PositionPlayerOnVPlat); reads zero-page
// 0x00 for the signed horizontal adder left by an earlier MoveEnemyHorizontally call
// Outputs: none
void SMBEngine::PositionPlayerOnHPlat(uint8_t e)
{
    uint32_t wide = 0;

    const uint8_t savedAdder = M(0x00); // the saved value from the second subroutine is a signed adder
    wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + (int8_t)M(0x00);
    writeData(Player_X_Position, LOBYTE(wide)); // position player accordingly in horizontal position
    writeData(Player_PageLoc, HIBYTE(wide));    // SetPVar: save result to player's page location
    writeData(Platform_X_Scroll, savedAdder);   // put saved value from second sub here to be used later
    PositionPlayerOnVPlat(e);                   // position player vertically and appropriately
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::RightPlatform(uint8_t e)
{
    const uint8_t adder = MoveEnemyHorizontally(e); // move platform with current horizontal speed, if any
    writeData(0x00, adder);                         // store saved value here (residual code)
    // check collision flag; if no collision between player and platform, leave speed unaltered
    if ((M(PlatformCollisionFlag + e) & 0x80) == 0)
    {
        writeData(Enemy_X_Speed + e, 0x10); // otherwise set new speed (gets moving if motionless)
        PositionPlayerOnHPlat(e);           // use saved value from earlier sub to position player
    } // ExRPl: then leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (forwarded to NotMoveEnemySlowVert)
// Outputs: none
void SMBEngine::MoveDropPlatform(uint8_t e)
{
    NotMoveEnemySlowVert(e, 0x7f); // set movement amount for drop platform
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (forwarded to NotMoveEnemySlowVert)
// Outputs: none
void SMBEngine::MoveEnemySlowVert(uint8_t e)
{
    NotMoveEnemySlowVert(e, 0x0f); // set movement amount for bowser/other objects
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset; y = movement amount preset by the caller (0x7f for
// drop platforms, 0x0f for bowser/other objects)
// Outputs: none
void SMBEngine::NotMoveEnemySlowVert(uint8_t e, uint8_t downwardMoveAmt)
{
    SetXMoveAmt(0x02, e, downwardMoveAmt);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (forwarded to SetHiMax)
// Outputs: none
void SMBEngine::MoveJ_EnemyVertically(uint8_t e)
{
    SetHiMax(e, 0x1c); // set movement amount for podoboo/other objects
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::LargePlatformSubroutines(uint8_t e)
{
    // subtract $24 to get proper offset for jump table
    const uint8_t platformType = static_cast<uint8_t>(M(Enemy_ID + e) - 0x24);
    switch (platformType)
    {
    case 0:
        BalancePlatform(e); // table used by objects $24-$2a
        return;
    case 1:
        YMovingPlatform(e);
        return;
    case 2:
        MoveLargeLiftPlat(e);
        return;
    case 3:
        MoveLargeLiftPlat(e);
        return;
    case 4:
        XMovingPlatform(e);
        return;
    case 5:
        DropPlatform(e);
        return;
    case 6:
        RightPlatform(e);
        return;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MovePodoboo(uint8_t e)
{
    // check enemy timer
    if (M(EnemyIntervalTimer + e) == 0)
    {                  // branch to move enemy if not expired
        InitPodoboo(e); // otherwise set up podoboo again
        // get part of LSFR and set d7
        const uint8_t force = M(PseudoRandomBitReg + 1 + e) | 0b10000000;
        writeData(Enemy_Y_MoveForce + e, force); // store as movement force
        // mask out high nybble and set for at least six intervals, then store as new enemy timer
        writeData(EnemyIntervalTimer + e, (force & 0b00001111) | 0x06);
        writeData(Enemy_Y_Speed + e, 0xf9); // set vertical speed to move podoboo upwards
    } // PdbM: branch to impose gravity on podoboo
    MoveJ_EnemyVertically(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveJumpingEnemy(uint8_t e)
{
    MoveJ_EnemyVertically(e);  // do a sub to impose gravity on green paratroopa
    MoveEnemyHorizontally(e); // jump to move enemy horizontally
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveBloober(uint8_t e)
{
    const uint8_t BlooberBitmasks_data[] = {0b00111111, 0b00000011};

    // check enemy state for d5 set, and branch if set to move defeated bloober
    if ((M(Enemy_State + e) & 0b00100000) != 0)
    {
        // MoveDefeatedBloober
        MoveEnemySlowVert(e); // jump to move defeated bloober downwards
        return;
    }

    // mask out bits in the LSFR using the bitmask the secondary hard mode flag selects; if any
    // bits are set, skip ahead to make it swim
    const uint8_t maskedBits = M(PseudoRandomBitReg + 1 + e) & BlooberBitmasks_data[M(SecondaryHardMode)];
    bool blooberCarry = false; // the jump engine that dispatched here left the carry clear
    if (maskedBits == 0)
    {
        uint8_t movingDir = 0;
        if ((e & 0x01) != 0)
        {                                    // on the second or fourth slot (1 or 3)
            movingDir = M(Player_MovingDir); // load player's moving direction and
            blooberCarry = true;             // the shift of an odd slot number carries its d0 out
        }
        else
        {
            // FBLeft: set left moving direction by default
            movingDir = 0x02;
            // get horizontal difference between player and bloober
            const auto [enemyRightOfPlayer, diff] = PlayerEnemyDiff(e);
            blooberCarry = enemyRightOfPlayer; // the difference leaves its no-borrow behind
            if ((diff & 0x80) != 0)
            {
                --movingDir; // enemy left of player, decrement to set right moving direction
            }
        }
        // SBMDir: set moving direction of bloober, then continue on here
        writeData(Enemy_MovingDir + e, movingDir);
    }

    // BlooberSwim
    ProcSwimmingB(blooberCarry, e); // execute sub to make bloober swim characteristically
    // get vertical coordinate and subtract movement force
    const uint8_t newY = M(Enemy_Y_Position + e) - M(Enemy_Y_MoveForce + e);
    if (newY >= 0x20)
    {
        // otherwise, set new vertical position, make bloober swim
        writeData(Enemy_Y_Position + e, newY);
    } // SwimX: check moving direction

    // moving to the left subtracts the swim speed rather than adding it
    const bool movingLeft = (M(Enemy_MovingDir + e) - 1) != 0;
    const uint16_t position = (M(Enemy_PageLoc + e) << 8) | M(Enemy_X_Position + e);
    // LeftSwim
    const uint32_t wide = movingLeft ? (position - M(BlooperMoveSpeed + e)) : (position + M(BlooperMoveSpeed + e));
    writeData(Enemy_X_Position + e, LOBYTE(wide)); // store result as new horizontal coordinate
    writeData(Enemy_PageLoc + e, HIBYTE(wide));    // store as new page location and leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveBulletBill(uint8_t e)
{
    // check bullet bill's enemy object state for d5 set
    if ((M(Enemy_State + e) & 0b00100000) != 0)
    {                            // if not set, continue with movement code
        MoveJ_EnemyVertically(e); // otherwise jump to move defeated bullet bill downwards
        return;
    } // NotDefB: set bullet bill's horizontal speed
    writeData(Enemy_X_Speed + e, 0xe8); // and move it accordingly (note: this bullet bill
    MoveEnemyHorizontally(e);           // object occurs in frenzy object $17, not from cannons)
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveSwimmingCheepCheep(uint8_t e)
{
    const uint8_t SwimCCXMoveData_data[] = {
        0x40, 0x80, 0x04, 0x04 // residual data, not used
    };

    // check cheep-cheep's enemy object state for d5 set; if not set, continue with movement code
    if ((M(Enemy_State + e) & 0b00100000) != 0)
    {
        MoveEnemySlowVert(e); // otherwise jump to move defeated cheep-cheep downwards
        return;
    }

    // CCSwim: save enemy state in $03. Only a clear d5 reaches here, so the state saved is zero.
    writeData(0x03, 0x00);
    // get enemy identifier and subtract ten for cheep-cheep identifiers, to use as offset
    writeData(0x02, SwimCCXMoveData_data[M(Enemy_ID + e) - 0x0a]); // load value here

    // page:coordinate:force is one 24-bit quantity here, so the borrow runs all the way up
    uint32_t wide = (M(Enemy_PageLoc + e) << 16) | (M(Enemy_X_Position + e) << 8) | M(Enemy_X_MoveForce + e);
    wide -= M(0x02);                                     // subtract preset value from horizontal force
    writeData(Enemy_X_MoveForce + e, LOBYTE(wide));      // store as new horizontal force
    writeData(Enemy_X_Position + e, HIBYTE(wide));       // and save as new horizontal coordinate
    writeData(Enemy_PageLoc + e, (uint8_t)(wide >> 16)); // page location, then save

    writeData(0x02, 0x20); // save new value here
    if (e < 0x02)
    {
        return; // if in first or second slot, branch to leave
    }

    // The movement flag picks the direction: highpos:position:dummy is one 24-bit quantity, and
    // the preset value is added to the dummy and the enemy state to the coordinate.
    const bool movingDown = M(CheepCheepMoveMFlag + e) >= 0x10; // check movement flag
    wide = (M(Enemy_Y_HighPos + e) << 16) | (M(Enemy_Y_Position + e) << 8) | M(Enemy_YMF_Dummy + e);
    const uint32_t adder = (M(0x03) << 8) | M(0x02);
    // CCSwimUpwards: subtracting moves it slowly upwards instead
    wide = movingDown ? (wide + adder) : (wide - adder);
    writeData(Enemy_YMF_Dummy + e, LOBYTE(wide));  // and save dummy
    writeData(Enemy_Y_Position + e, HIBYTE(wide)); // save as new vertical coordinate
    // ChkSwimYPos: save new page location here
    writeData(Enemy_Y_HighPos + e, (uint8_t)(wide >> 16));

    // get vertical coordinate and subtract original coordinate from current
    uint8_t diff = M(Enemy_Y_Position + e) - M(CheepCheepOrigYPos + e);
    uint8_t movementSpeed = 0x00; // load movement speed to upwards by default
    if ((diff & 0x80) != 0)
    {                        // if result positive, skip to next part
        movementSpeed = 0x10; // otherwise load movement speed to downwards
        diff = (diff ^ 0xff) + 0x01; // to obtain total difference of original vs. current
    } // YPDiff: if difference between original vs. current vertical
    if (diff < 0x0f)
    {
        return; // coordinates < 15 pixels, leave movement speed alone
    }
    writeData(CheepCheepMoveMFlag + e, movementSpeed); // otherwise change movement speed

    // ExSwCC: leave
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveFlyingCheepCheep(uint8_t e)
{
    // Why is this table so big? quoth Claude:
    //
    //     `MoveFlyingCheepCheep` indexes `PRandomSubtracter` and `FlyCCBPriority`, both
    //     five entries long, with the high nybble of a movement force, which counts to
    //     fifteen. Ten of the sixteen reads are off the end. The disassembly says so at the
    //     instruction that sets the index: *"note this tends to go into reach of code"*.
    //
    //     It is not residual code. The bytes it finds past the end of the tables are what
    //     decide how the cheep-cheep moves, so getting them wrong changes the motion. Both
    //     tables now carry the bytes the ROM has after them.

    const uint8_t FlyCCBPriority_data[] = {
        0x20, 0x20, 0x20, 0x00, 0x00, 0xb5, 0x1e, 0x29, 0x20, 0xf0, 0x08, 0xa9, 0x00, 0x9d, 0xc5, 0x03, 0x4c, 0x92, 0xbf, 0x20, 0x02, 0xbf,
        0xa0, 0x0d, 0xa9, 0x05, 0x20, 0x96, 0xbf, 0xbd, 0x34, 0x04, 0x4a, 0x4a, 0x4a, 0x4a, 0xa8, 0xb5, 0xcf, 0x38, 0xf9, 0xd5, 0xce, 0x10,
        0x05, 0x49, 0xff, 0x18, 0x69, 0x01, 0xc9, 0x08, 0xb0, 0x0e, 0xbd, 0x34, 0x04, 0x18, 0x69, 0x10, 0x9d, 0x34, 0x04, 0x4a, 0x4a, 0x4a,
        0x4a, 0xa8, 0xb9, 0xda, 0xce, 0x9d, 0xc5, 0x03, 0x60, 0x15, 0x30, 0x40, 0xb5, 0x1e, 0x29, 0x20, 0xf0, 0x03, 0x4c, 0x63, 0xbf, 0xb5,
        0x1e, 0xf0, 0x0b, 0xa9, 0x00, 0x95, 0xa0, 0x8d, 0xcb, 0x06, 0xa9, 0x10, 0xd0, 0x13, 0xa9, 0x12, 0x8d, 0xcb, 0x06, 0xa0, 0x02, 0xb9,
        0x25, 0xcf, 0x99, 0x01, 0x00, 0x88, 0x10, 0xf7, 0x20, 0x6c, 0xcf, 0x95, 0x58, 0xa0, 0x01, 0xb5, 0xa0, 0x29, 0x01, 0xd0, 0x0a, 0xb5,
        0x58, 0x49, 0xff, 0x18, 0x69, 0x01, 0x95, 0x58, 0xc8, 0x94, 0x46, 0x4c, 0x02, 0xbf, 0xa0, 0x00, 0x20, 0x43, 0xe1, 0x10, 0x0a, 0xc8,
        0xa5, 0x00, 0x49, 0xff, 0x18, 0x69, 0x01, 0x85, 0x00, 0xa5, 0x00, 0xc9, 0x3c, 0x90, 0x1c, 0xa9, 0x3c, 0x85, 0x00, 0xb5, 0x16, 0xc9,
        0x11, 0xd0, 0x12, 0x98, 0xd5, 0xa0, 0xf0, 0x0d, 0xb5, 0xa0, 0xf0, 0x06, 0xd6, 0x58, 0xb5, 0x58, 0xd0, 0x40, 0x98, 0x95, 0xa0, 0xa5,
        0x00, 0x29, 0x3c, 0x4a, 0x4a, 0x85, 0x00, 0xa0, 0x00, 0xa5, 0x57, 0xf0, 0x24, 0xad, 0x75, 0x07, 0xf0, 0x1f, 0xc8, 0xa5, 0x57, 0xc9,
        0x19, 0x90, 0x08, 0xad, 0x75, 0x07, 0xc9, 0x02, 0x90, 0x01, 0xc8, 0xb5, 0x16, 0xc9, 0x12, 0xd0, 0x04, 0xa5, 0x57, 0xd0, 0x06, 0xb5,
        0xa0, 0xd0, 0x02, 0xa0, 0x00, 0xb9, 0x01, 0x00, 0xa4, 0x00, 0x38, 0xe9, 0x01, 0x88};

    const uint8_t PRandomSubtracter_data[] = {0xf8, 0xa0, 0x70, 0xbd, 0x00, 0x20, 0x20, 0x20,
                                              0x00, 0x00, 0xb5, 0x1e, 0x29, 0x20, 0xf0, 0x08};

    // check cheep-cheep's enemy state for d5 set, and branch to continue code if not set
    if ((M(Enemy_State + e) & 0b00100000) != 0)
    {
        writeData(Enemy_SprAttrib + e, 0x00); // otherwise clear sprite attributes
        MoveJ_EnemyVertically(e);              // and jump to move defeated cheep-cheep downwards
        return;
    }

    // FlyCC: move cheep-cheep horizontally based on speed and force
    MoveEnemyHorizontally(e);
    // impose gravity on flying cheep-cheep, at a maximum speed of five and a vertical movement
    // amount of thirteen
    SetXMoveAmt(0x05, e, 0x0d);
    // get vertical movement force and move high nybble to low, to save as offset (note this
    // tends to go into reach of code)
    uint8_t priorityIndex = M(Enemy_Y_MoveForce + e) >> 4;

    // get vertical position and subtract the value the offset selects
    uint8_t diff = M(Enemy_Y_Position + e) - PRandomSubtracter_data[priorityIndex];
    if ((diff & 0x80) != 0)
    { // if result within top half of screen, skip this part
        diff = (diff ^ 0xff) + 0x01;
    } // AddCCF: if result or two's compliment greater than eight,
    if (diff < 0x08)
    { // skip to the end without changing movement force
        const uint8_t force = M(Enemy_Y_MoveForce + e) + 0x10; // otherwise add to it
        writeData(Enemy_Y_MoveForce + e, force);
        priorityIndex = force >> 4; // move high nybble to low again
    } // BPGet: load bg priority data and store (this is very likely
    // broken or residual code, value is overwritten before drawing it next frame), then leave
    writeData(Enemy_SprAttrib + e, FlyCCBPriority_data[priorityIndex]);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::DropPlatform(uint8_t e)
{
    // if no collision between platform and player occurred, just leave without moving anything
    if ((M(PlatformCollisionFlag + e) & 0x80) == 0)
    {
        MoveDropPlatform(e);      // otherwise do a sub to move platform down very quickly
        PositionPlayerOnVPlat(e); // do a sub to position player appropriately
    } // ExDPl: leave
}

//------------------------------------------------------------------------

// Inputs: none (always sweeps enemy slots 4 down to 0)
// Outputs: none
void SMBEngine::KillAllEnemies()
{
    for (int slot = 0x04; slot >= 0; --slot) // start with last enemy slot
    {
        EraseEnemyObject(slot); // branch to kill enemy objects
        // do this until all slots are emptied
    }
    writeData(EnemyFrenzyBuffer, 0x00); // empty frenzy buffer
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::LargePlatformCollision(uint8_t e)
{
    // save value here
    writeData(PlatformCollisionFlag + e, 0xff);
    if (M(TimerControl) != 0) // check master timer control
    {
        return; // leave
    }
    if ((M(Enemy_State + e) & 0x80) != 0) // if d7 set in object state,
    {
        return; // leave
    }
    if (M(Enemy_ID + e) != 0x24)
    {
        ChkForPlayerC_LargeP(e); // balance platform, branch if not found
        return;
    }
    // perform code with state as enemy offset, then with the original object offset
    ChkForPlayerC_LargeP(M(Enemy_State + e));
    ChkForPlayerC_LargeP(M(ObjectOffset));
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (this platform, or the balance-platform's "other" half
// per caller)
// Outputs: none
void SMBEngine::ChkForPlayerC_LargeP(uint8_t e)
{
    // figure out if player is below a certain point
    if (CheckPlayerVertical())
    {
        return; // leave
    }
    const uint8_t boundBoxOfs = GetEnemyBoundBoxOfsArg(e).first; // get bounding box offset
    // store vertical coordinate in temp variable for now
    writeData(0x00, M(Enemy_Y_Position + e));
    // do player-to-platform collision detection
    const bool collisionFound = PlayerCollisionCore(boundBoxOfs);
    if (!collisionFound)
    {
        return;
    }
    ProcLPlatCollisions(boundBoxOfs, e); // otherwise collision, perform sub
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::RunSmallPlatform(uint8_t e)
{
    GetEnemyOffscreenBits(e);
    RelativeEnemyPosition(e);
    SmallPlatformBoundBox(e);
    SmallPlatformCollision();
    RelativeEnemyPosition(e);
    DrawSmallPlatform(e);
    MoveSmallPlatform(e);
    OffscreenBoundsCheck(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::RunLargePlatform(uint8_t e)
{
    GetEnemyOffscreenBits(e);
    RelativeEnemyPosition(e);
    LargePlatformBoundBox(e);
    LargePlatformCollision(e);
    // if master timer control set,
    if (M(TimerControl) == 0)
    { // skip subroutine tree
        LargePlatformSubroutines(e);
    } // SkipPT
    RelativeEnemyPosition(e);
    DrawLargePlatform(e);
    OffscreenBoundsCheck(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::SmallPlatformBoundBox(uint8_t e)
{
    // store bitmask here for now
    writeData(0x00, 0x08);
    GetMaskedOffScrBits(e, 0x04); // store another bitmask here for now
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none (the caller reloads x via RelativeEnemyPosition before reading it)
void SMBEngine::LargePlatformBoundBox(uint8_t e)
{
    // use the next offset to get the proper offset for horizontal offscreen bits
    const uint8_t offscreenBits = GetXOffscreenBits(e + 1);
    if (offscreenBits >= 0xfe)
    {
        MoveBoundBoxOffscreen(e); // box offscreen, otherwise start getting coordinates
        return;
    }

    SetupEOffsetFBBox(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (first enemy)
// Outputs: none
void SMBEngine::EnemiesCollision(uint8_t e)
{
    const uint8_t ClearBitsMask_data[] = {0b01111111, 0b10111111, 0b11011111, 0b11101111, 0b11110111, 0b11111011, 0b11111101};

    const uint8_t SetBitsMask_data[] = {0b10000000, 0b01000000, 0b00100000, 0b00010000, 0b00001000, 0b00000100, 0b00000010};

    // check counter for d0 set; if d0 not set, leave
    if ((M(FrameCounter) & 0x01) == 0)
    {
        return;
    }
    if (M(AreaType) == 0)
    {
        return; // if water area type, leave
    }

    // Lakitu and the piranha plant never take part, and neither does anything offscreen.
    const uint8_t firstEnemyId = M(Enemy_ID + e);
    if (firstEnemyId >= 0x15 || firstEnemyId == Lakitu || firstEnemyId == PiranhaPlant)
    {
        return;
    }
    if (M(EnemyOffscrBitsMasked + e) != 0)
    {
        return; // masked offscreen bits nonzero, leave
    }
    // get appropriate bounding box offset for the first enemy; the original restored this
    // through the stack on every pass of the loop below, but it is the same value throughout
    const uint8_t firstBoundBoxOfs = GetEnemyBoundBoxOfs().first;

    // One pass of ECLoop: compare the first enemy against the second enemy, and either record
    // the collision or clear the bit. ReadyNextEnemy runs after it either way.
    const auto checkEnemyPair = [&](uint8_t second)
    {
        // check enemy object enable flag
        if (M(Enemy_Flag + second) == 0)
        {
            return; // branch if flag not set
        }
        const uint8_t secondEnemyId = M(Enemy_ID + second);
        if (secondEnemyId >= 0x15 || secondEnemyId == Lakitu || secondEnemyId == PiranhaPlant)
        {
            return;
        }
        if (M(EnemyOffscrBitsMasked + second) != 0)
        {
            return; // branch if masked offscreen bits set
        }
        // get second enemy object's bounding box offset: multiply by four, then add four
        const uint8_t secondBoundBoxOfs = (second << 2) + 0x04;
        // do collision detection using the two enemies here
        const bool collisionFound = SprObjectCollisionCore(secondBoundBoxOfs, firstBoundBoxOfs);
        const uint8_t first = M(ObjectOffset); // use first enemy offset
        if (!collisionFound)
        {
            // NoEnemyCollision: clear the bit connected to the second enemy
            writeData(Enemy_CollisionBits + second, M(Enemy_CollisionBits + second) & ClearBitsMask_data[first]);
            return;
        }
        // check both enemy states for d7 set; skip this part if at least one of them is set
        if (((M(Enemy_State + first) | M(Enemy_State + second)) & 0b10000000) == 0)
        {
            // check to see if bit connected to second enemy is already set
            if ((M(Enemy_CollisionBits + second) & SetBitsMask_data[first]) != 0)
            {
                return; // already set, move onto next enemy slot
            }
            // if the bit is not set, set it now
            writeData(Enemy_CollisionBits + second, M(Enemy_CollisionBits + second) | SetBitsMask_data[first]);
        }
        // YesEC: react according to the nature of collision
        ProcEnemyCollisions(first, second);
    };

    // ECLoop: decrement to the second enemy we're going to compare against; leave once all
    // enemy slots have been checked
    for (int second = e - 1; second >= 0; --second)
    {
        writeData(0x01, second); // save enemy object buffer offset for second enemy here
        checkEnemyPair(second);
        // ReadyNextEnemy
    }
}

//------------------------------------------------------------------------

// Inputs: first = the current enemy's object buffer offset (== ObjectOffset); second = the
// other enemy's offset (also available via zero-page 0x01)
// Outputs: none
void SMBEngine::ProcEnemyCollisions(uint8_t first, uint8_t second)
{
    // check both enemy states for d5 set; if d5 is set in either state, or both, branch
    if (((M(Enemy_State + second) | M(Enemy_State + first)) & 0b00100000) != 0)
    {
        return; // to leave and do nothing else at this point
    }
    if (M(Enemy_State + first) >= 0x06)
    {
        // check first enemy identifier for hammer bro
        if (M(Enemy_ID + first) == HammerBro)
        {
            return;
        }
        if ((M(Enemy_State + second) & 0x80) != 0) // check second enemy state for d7 set
        {                                          // branch if d7 is clear
            SetupFloateyNumber(0x06, first);       // award 1000 points for killing enemy
            ShellOrBlockDefeat(first);             // then kill enemy
        } // ShellCollisions
        ShellOrBlockDefeat(second); // kill second enemy
        // get chain counter for shell, add four to get appropriate point offset
        SetupFloateyNumber(M(ShellChainCounter + first) + 0x04, second); // award appropriate number of points for second enemy
        ++M(ShellChainCounter + first);                                  // increment chain counter for additional enemies
        return;                                                          // ExitProcessEColl: leave!!!

        //------------------------------------------------------------------------
    } // ProcSecondEnemyColl
    // if first enemy state < $06, branch elsewhere
    if (M(Enemy_State + second) >= 0x06)
    {
        // check second enemy identifier for hammer bro
        if (M(Enemy_ID + second) == HammerBro)
        {
            return;
        }
        ShellOrBlockDefeat(first); // otherwise, kill first enemy
        // get chain counter for shell, add four to get appropriate point offset
        SetupFloateyNumber(M(ShellChainCounter + second) + 0x04, first); // award appropriate number of points for first enemy
        ++M(ShellChainCounter + second);                                 // increment chain counter for additional enemies
        return;                                                          // leave!!!

        //------------------------------------------------------------------------
    } // MoveEOfs
    EnemyTurnAround(second); // do the sub here using value from $01
    EnemyTurnAround(first);  // then do it again using value from $08
}

//------------------------------------------------------------------------

// Inputs: none (acts on the current object offset)
// Outputs: none
void SMBEngine::KillEnemyAboveBlock()
{
    ShellOrBlockDefeat(M(ObjectOffset));                // do this sub to kill enemy
    writeData(Enemy_Y_Speed + M(ObjectOffset), 0xfc); // alter vertical speed of enemy and leave
}

//------------------------------------------------------------------------

// do a sub here to move enemy downwards
// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::FallE(uint8_t e)
{
    MoveD_EnemyVertically(e);

    const uint8_t enemyState = M(Enemy_State + e);
    if (enemyState == 0x02)
    {
        // MEHor: move enemy horizontally for state $02
        MoveEnemyHorizontally(e);
        return;
    }
    // Anything other than a power-up with d6 of its state set gets the slower deceleration
    // (SlowM).
    const bool d6Set = (enemyState & 0b01000000) != 0;
    const bool slowMovement = d6Set && (M(Enemy_ID + e) != PowerUpObject);
    SteadM(slowMovement ? 0x01 : 0x00, e);
}

//------------------------------------------------------------------------

// get current horizontal speed
// Inputs: decelIndex = table index selecting the deceleration to apply; e = enemy object buffer
// offset
// Outputs: none
void SMBEngine::SteadM(uint8_t decelIndex, uint8_t e)
{
    const uint8_t XSpeedAdderData_data[] = {0x00, 0xe8, 0x00, 0x18};

    const uint8_t savedSpeed = M(Enemy_X_Speed + e);
    // an enemy moving left takes the second half of the table, which slows it the other way; one
    // not moving or moving right leaves the index alone
    const uint8_t index = ((savedSpeed & 0x80) != 0) ? (decelIndex + 2) : decelIndex;
    // AddHS: add value here to slow enemy down if necessary, saving as horizontal speed temporarily
    writeData(Enemy_X_Speed + e, savedSpeed + XSpeedAdderData_data[index]);
    MoveEnemyHorizontally(e); // then do a sub to move horizontally
    // return the old horizontal speed to its original memory location, then leave
    writeData(Enemy_X_Speed + e, savedSpeed);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::RunFirebarObj(uint8_t e)
{
    ProcFirebar(e);
    OffscreenBoundsCheck(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (the firebar's own slot)
// Outputs: none
void SMBEngine::ProcFirebar(uint8_t e)
{
    GetEnemyOffscreenBits(e); // get offscreen information
    // check for d3 set; if so, branch to leave
    if ((M(Enemy_OffscreenBits) & 0b00001000) != 0)
    {
        return; // SkipFBar
    }

    // What the residual GetFirebarPosition call below is handed: the spinstate as it was before
    // the horizontal-state nudge, or zero when the master timer control skipped the spin. It is
    // whatever the 6502 happened to leave in the accumulator, which is why it is neither the
    // stored spinstate nor anything else in particular.
    uint8_t residualSpinState = 0x00;

    // if master timer control set, branch ahead of this part
    if (M(TimerControl) == 0)
    {
        // load spinning speed of firebar and modify current spinstate, masking out all but 5 LSB
        const uint8_t spun = FirebarSpin(M(FirebarSpinSpeed + e), e) & 0b00011111;
        writeData(FirebarSpinState_High + e, spun); // and store as new high byte of spinstate
        residualSpinState = spun;
    } // SusFbar: get high byte of spinstate
    uint8_t spinStateHigh = M(FirebarSpinState_High + e);
    // Long firebars (identifier < $1f) are left alone. Anything else nudges the spinstate
    // off the two horizontal states, eight and twenty-four.
    const bool atHorizontalState = (M(Enemy_ID + e) >= 0x1f) && (spinStateHigh == 0x08 || spinStateHigh == 0x18);
    if (atHorizontalState)
    {
        // SkpFSte: add one to spinning thing to avoid horizontal state
        ++spinStateHigh;
        writeData(FirebarSpinState_High + e, spinStateHigh);
    }

    // SetupGFB: save high byte of spinning thing, modified or otherwise
    writeData(0xef, spinStateHigh);
    RelativeEnemyPosition(e);              // get relative coordinates to screen
    GetFirebarPosition(residualSpinState); // do a sub here (residual, too early to be used now)

    const uint8_t oamOffset = M(Enemy_SprDataOffset + e);        // get OAM data offset
    writeData(Sprite_Y_Position + oamOffset, M(Enemy_Rel_YPos)); // store relative vertical coordinate as Y in OAM data
    writeData(0x07, M(Enemy_Rel_YPos));                          // also save here
    writeData(Sprite_X_Position + oamOffset, M(Enemy_Rel_XPos)); // store relative horizontal coordinate as X in OAM data
    writeData(0x06, M(Enemy_Rel_XPos));                          // also save here
    writeData(0x00, 0x01);                                       // set $01 value here (not necessary)
    FirebarCollision(oamOffset);                                 // draw fireball part and do collision detection

    // load value for short firebars by default, or the longer value for long firebars
    // SetMFbar: store maximum value for length of firebars
    writeData(0xed, (M(Enemy_ID + e) >= 0x1f) ? 0x0b : 0x05);
    writeData(0x00, 0x00); // initialize counter here

    do // DrawFbar: load high byte of spinstate
    {
        GetFirebarPosition(M(0xef)); // get fireball position data depending on firebar part
        DrawFirebar_Collision();     // position it properly, draw it and do collision detection
        // check which firebar part
        if (M(0x00) == 0x04)
        {
            // if we arrive at fifth firebar part, get the offset from the long firebar and load
            // the OAM data offset using it, then store as new one here
            writeData(0x06, M(Enemy_SprDataOffset + M(DuplicateObj_Offset)));
        } // NextFbar: move onto the next firebar part
        ++M(0x00);
    } while (M(0x00) < M(0xed)); // otherwise go back and do another

    // SkipFBar
}

//------------------------------------------------------------------------

// Inputs: none (reads the horizontal/vertical adders and mirror data GetFirebarPosition left in
// zero-page 0x01-0x03, and the OAM offset from zero-page 0x06)
// Outputs: none (the segment's screen coordinates are communicated to FirebarCollision via
// zero-page 0x06/0x07; x is whatever ProcFirebar left it as)
void SMBEngine::DrawFirebar_Collision()
{
    // store mirror data elsewhere
    writeData(0x05, M(0x03));
    // load OAM data offset for firebar. It has to be held here rather than read back from $06,
    // because the X coordinate computed below is written over $06 further down.
    const uint8_t oamOffset = M(0x06);

    // load horizontal adder we got from position loader, and shift the LSB of the mirror data;
    // if the bit was set, use the adder as-is, otherwise get its two's compliment
    uint8_t horizAdder = M(0x01);
    bool shiftedBit = (M(0x05) & 0x01) != 0;
    M(0x05) >>= 1;
    if (!shiftedBit)
    {
        horizAdder = (horizAdder ^ 0xff) + 0x01;
    } // AddHA: add horizontal coordinate relative to screen to
    // horizontal adder, modified or otherwise
    const uint8_t spriteXPos = horizAdder + M(Enemy_Rel_XPos);
    writeData(Sprite_X_Position + oamOffset, spriteXPos); // store as X coordinate here
    writeData(0x06, spriteXPos);                          // store here for now, note offset is saved separately

    // SubtR1: take the distance between the sprite X and the original X, whichever way round
    // they are
    const uint8_t relX = M(Enemy_Rel_XPos);
    const uint8_t apart = (spriteXPos < relX) ? (relX - spriteXPos) : (spriteXPos - relX);

    // ChkFOfs: the sprite goes offscreen if the coordinates are too far apart, or if the
    // vertical relative coordinate is offscreen already; otherwise handle the vertical adder.
    uint8_t spriteYPos = 0xf8;
    const bool tooFarApart = apart >= 0x59;
    // VAHandl
    if (!tooFarApart && M(Enemy_Rel_YPos) != 0xf8)
    {
        // load vertical adder we got from position loader, and shift the LSB of the mirror data
        // one more time; if the bit was set, use it as-is, otherwise get its two's compliment
        uint8_t vertAdder = M(0x02);
        shiftedBit = (M(0x05) & 0x01) != 0;
        M(0x05) >>= 1;
        if (!shiftedBit)
        {
            vertAdder = (vertAdder ^ 0xff) + 0x01;
        } // AddVA: add vertical coordinate relative to screen to
        spriteYPos = vertAdder + M(Enemy_Rel_YPos); // the second data, modified or otherwise
    }

    // SetVFbr: store as Y coordinate here
    writeData(Sprite_Y_Position + oamOffset, spriteYPos);
    writeData(0x07, spriteYPos); // also store here for now
    FirebarCollision(oamOffset);
}

//------------------------------------------------------------------------

// Inputs: oamOffset = OAM data offset for this firebar segment; reads the segment's coordinates
// from zero-page 0x06/0x07 (set by the caller)
// Outputs: x is reloaded from ObjectOffset
void SMBEngine::FirebarCollision(uint8_t oamOffset)
{
    DrawFirebar(oamOffset); // run sub here to draw current tile of firebar

    // Everything here lands on NoColFB, which advances the OAM offset regardless.
    const auto checkCollision = [&]()
    {
        // if star mario invincibility timer or master timer controls set, skip all of this
        if ((M(StarInvincibleTimer) | M(TimerControl)) != 0)
        {
            return;
        }
        writeData(0x05, 0x00); // otherwise initialize counter
        // if player's vertical high byte offscreen, skip all of this
        if (static_cast<uint8_t>(M(Player_Y_HighPos) - 1) != 0)
        {
            return;
        }

        uint8_t vertCoord = M(Player_Y_Position); // get player's vertical position
        // AdjSm: a small player, or a big one crouching, checks from 24 pixels lower down and
        // sets the counter to $02 as a flag
        const bool adjustForSmall = (M(PlayerSize) != 0) || (M(CrouchingFlag) != 0);
        if (adjustForSmall)
        {
            M(0x05) += 2; // first increment our counter twice (setting $02 as flag)
            vertCoord += 0x18;
        }

        // BigJp: get vertical coordinate, altered or otherwise
        a = vertCoord;

        while (true) // FBCLoop
        {
            a -= M(0x07); // subtract vertical position of firebar from the player's
            if ((a & 0x80) != 0)
            {              // if player lower on the screen than firebar,
                a ^= 0xff; // skip two's compliment part
                a += 0x01;
            }

            // ChkVFBD: a vertical difference of 8 pixels or more is no collision, and neither is
            // a firebar on the far right of the screen
            bool nextSegment = (a >= 0x08) || (M(0x06) >= 0xf0);
            if (!nextSegment)
            {
                a = M(Sprite_X_Position + 4); // get OAM X coordinate for sprite #1
                a += 0x04;                    // add four pixels
                writeData(0x04, a);           // store here
                a -= M(0x06);                 // subtract the X coordinate of the firebar
                if ((a & 0x80) != 0)
                {              // if modded X coordinate to the right of firebar
                    a ^= 0xff; // skip two's compliment part
                    a += 0x01;
                }
                // ChkFBCl: a difference under 8 pixels is a collision
                nextSegment = (a >= 0x08);
            }

            if (!nextSegment)
            {
                break; // collision, go and process it
            }

            // Chk2Ofs: if value of $02 was set earlier for whatever reason, leave
            if (M(0x05) == 0x02)
            {
                return;
            }
            y = M(0x05); // otherwise get temp here and use as offset
            // add value loaded with offset to player's vertical coordinate
            a = M(Player_Y_Position) + M(FirebarYPos + y);
            ++M(0x05); // then increment temp and go round again
        }

        // ChgSDir: set movement direction by default
        x = 0x01;
        // if OAM X coordinate of player's sprite 1 is left of the firebar, do not alter it
        if (M(0x04) < M(0x06))
        {
            x = 0x02; // otherwise increment it
        }
        // SetSDir: store movement direction here
        writeData(Enemy_MovingDir, x);
        x = 0x00;
        const uint8_t saved00 = M(0x00); // InjurePlayer overwrites $00, so save it across the call
        InjurePlayer();                  // perform sub to hurt or kill player
        writeData(0x00, saved00);
    };
    checkCollision();

    // NoColFB: advance the OAM data offset by one sprite
    writeData(0x06, oamOffset + 0x04);
    x = M(ObjectOffset); // get enemy object buffer offset and leave
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (bowser flame's slot)
// Outputs: none
void SMBEngine::RunBowserFlame(uint8_t e)
{
    ProcBowserFlame();
    GetEnemyOffscreenBits(e);
    RelativeEnemyPosition(e);
    GetEnemyBoundBox(e);
    PlayerEnemyCollision(e);
    OffscreenBoundsCheck(e);
}

//------------------------------------------------------------------------

// Inputs: enemyOffset = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveD_Bowser(uint8_t enemyOffset)
{
    MoveEnemySlowVert(enemyOffset); // do a sub to move bowser downwards
    BowserGfxHandler(enemyOffset);  // jump to draw bowser's front and rear, then leave
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (bowser's slot)
// Outputs: none
void SMBEngine::RunBowser(uint8_t e)
{
    const uint8_t PRandomRange_data[] = {0x21, 0x41, 0x11, 0x31};

    bool enemyRightOfPlayer = false;
    bool hammerSpawned = false;

    // d5 in enemy state means bowser is defeated and on his way down
    if ((M(Enemy_State + e) & 0b00100000) != 0)
    {
        // otherwise check vertical position
        if (M(Enemy_Y_Position + e) < 0xe0)
        {
            MoveD_Bowser(e);
            return;
        }
        KillAllEnemies();
        return;
    }

    // BowserControl
    writeData(EnemyFrenzyBuffer, 0x00); // empty frenzy buffer
    // if master timer control set, skip over a bunch of code straight to the flames (SkipToFB)
    if (M(TimerControl) != 0)
    {
        ChkFireB(e);
        return;
    }

    // ChkMouth through CompDToO: animate bowser's feet and walk him about. Every way out of
    // this lands on HammerChk below.
    const auto controlBowser = [&]()
    {
        // check bowser's mouth; with the bit set there is nothing to do here
        if ((M(BowserBodyControls) & 0x80) != 0)
        {
            return;
        }
        // FeetTmr: decrement timer to control bowser's feet
        --M(BowserFeetCounter);
        if (M(BowserFeetCounter) == 0)
        {                                       // if not expired, skip this part
            writeData(BowserFeetCounter, 0x20); // otherwise, reset timer
            // and invert bit used to control bowser's feet
            writeData(BowserBodyControls, M(BowserBodyControls) ^ 0b00000001);
        }
        // ResetMDr: reset moving/facing direction every sixteen frames
        if ((M(FrameCounter) & 0b00001111) == 0)
        {
            writeData(Enemy_MovingDir + e, 0x02);
        }
        // B_FaceP: with the timer still running, turn bowser to face a player on his left
        if (M(EnemyFrameTimer + e) != 0)
        {
            // get horizontal difference between player and bowser
            uint8_t diff = 0;
            std::tie(enemyRightOfPlayer, diff) = PlayerEnemyDiff(e);
            if ((diff & 0x80) != 0)
            {                                           // bowser to the left of the player
                writeData(Enemy_MovingDir + e, 0x01);   // set bowser to move and face to the right
                writeData(BowserMovementSpeed, 0x02);   // set movement speed
                writeData(EnemyFrameTimer + e, 0x20);   // set timer here
                writeData(BowserFireBreathTimer, 0x20); // set timer used for bowser's flame
                if (M(Enemy_X_Position + e) >= 0xc8)
                {
                    return; // skip ahead to some other section
                }
            }
        }

        // GetPRCmp: execute this code every fourth frame, otherwise leave
        if ((M(FrameCounter) & 0b00000011) != 0)
        {
            return;
        }
        // back at his original position, pick a new range to wander within
        if (M(Enemy_X_Position + e) == M(BowserOrigXPos))
        {
            const uint8_t randomOfs = M(PseudoRandomBitReg + e) & 0b00000011; // get pseudorandom offset
            // load value using pseudorandom offset and store here
            writeData(MaxRangeFromOrigin, PRandomRange_data[randomOfs]);
        }
        // GetDToO: add the movement speed to the coordinate and save as new horizontal position
        uint8_t pos = M(Enemy_X_Position + e) + M(BowserMovementSpeed);
        writeData(Enemy_X_Position + e, pos);
        if (M(Enemy_MovingDir + e) == 0x01)
        {
            return;
        }
        uint8_t newSpeed = 0xff; // set default movement speed here (move left)
        pos -= M(BowserOrigXPos);  // distance from the original horizontal position
        if ((pos & 0x80) != 0)
        { // if current position to the right of original, skip ahead
            pos = -pos;
            newSpeed = 0x01; // set alternate movement speed here (move right)
        }
        // CompDToO: compare difference with pseudorandom value
        if (pos < M(MaxRangeFromOrigin))
        {
            return; // if difference < pseudorandom value, leave speed alone
        }
        writeData(BowserMovementSpeed, newSpeed); // otherwise change bowser's movement speed
    };
    controlBowser();

    // HammerChk
    const uint8_t frameTimer = M(EnemyFrameTimer + e);
    if (frameTimer == 0)
    {
        MoveEnemySlowVert(e); // start by moving bowser downwards
        // From world 6 on it is time to throw hammers, on every fourth frame. Worlds 1-5 skip
        // this part entirely (SetHmrTmr).
        if (M(WorldNumber) >= World6 && (M(FrameCounter) & 0b00000011) == 0)
        {
            hammerSpawned = SpawnHammerObj(); // spawn misc object (hammer)
        }
        // SetHmrTmr: get current vertical position
        if (M(Enemy_Y_Position + e) >= 0x80)
        {
            const uint8_t randomOfs = M(PseudoRandomBitReg + e) & 0b00000011; // get pseudorandom offset
            // get value using pseudorandom offset and set for timer here
            writeData(EnemyFrameTimer + e, PRandomRange_data[randomOfs]);
        }
        // SkipToFB: jump to execute flames code
        ChkFireB(e);
        return;
    }
    // MakeBJump: if timer not yet about to expire, skip ahead to next part
    if (frameTimer != 0x01)
    {
        ChkFireB(e);
        return;
    }
    --M(Enemy_Y_Position + e);          // otherwise decrement vertical coordinate
    InitVStf(e);                        // initialize movement amount
    writeData(Enemy_Y_Speed + e, 0xfe); // set vertical speed to move bowser upwards
    ChkFireB(e);
}

//------------------------------------------------------------------------

// check world number here
// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::ChkFireB(uint8_t e)
{
    // ChkFireB: each pass toggles bowser's mouth. A pass that opens it loops back, where the
    // timer this pass just set sends it to the graphics handler instead of breathing fire.
    while (true)
    {
        const uint8_t worldNumber = M(WorldNumber);
        // only world 8, and worlds before 6, get to this part
        if (worldNumber != World8 && worldNumber >= World6)
        {
            BowserGfxHandler(e);
            return;
        }
        // SpawnFBr: check timer here
        if (M(BowserFireBreathTimer) != 0)
        {
            BowserGfxHandler(e); // if not expired yet, skip all of this
            return;
        }
        writeData(BowserFireBreathTimer, 0x20); // set timer here
        // invert bowser's mouth bit to open and close bowser's mouth
        const uint8_t bodyControls = M(BowserBodyControls) ^ 0b10000000;
        writeData(BowserBodyControls, bodyControls);
        if ((bodyControls & 0x80) == 0)
        {
            break; // bowser's mouth now closed, go on to breathe fire
        }
    }
    uint8_t flameTimer = SetFlameTimer(); // get timing for bowser's flame
    if (M(SecondaryHardMode) != 0)
    {                       // if secondary hard mode flag not set, skip this
        flameTimer -= 0x10; // otherwise subtract from value
    } // SetFBTmr: set value as timer here
    writeData(BowserFireBreathTimer, flameTimer);
    // put bowser's flame identifier in enemy frenzy buffer
    writeData(EnemyFrenzyBuffer, BowserFlame);
    BowserGfxHandler(e);
}

//------------------------------------------------------------------------

// Inputs: enemyOffset = enemy object buffer offset (bowser front's slot)
// Outputs: none (ObjectOffset is temporarily switched to the rear half's offset to
// process it, then restored)
void SMBEngine::BowserGfxHandler(uint8_t enemyOffset)
{
    ProcessBowserHalf(enemyOffset);                     // do a sub here to process bowser's front
    uint8_t rearOfs = 0x10;                             // load default value here to position bowser's rear
    if ((M(Enemy_MovingDir + enemyOffset) & 0x01) != 0) // check moving direction
    {                                                   // if moving left, use default
        rearOfs = 0xf0;                                 // otherwise load alternate positioning value here
    } // CopyFToR: move bowser's rear object position value to A
    const uint8_t rear = M(DuplicateObj_Offset); // get bowser's rear object offset
    // add to bowser's front object horizontal coordinate and store as bowser's rear horizontal coordinate
    writeData(Enemy_X_Position + rear, rearOfs + M(Enemy_X_Position + enemyOffset));
    // vertical coordinate and store as vertical coordinate for bowser's rear
    writeData(Enemy_Y_Position + rear, M(Enemy_Y_Position + enemyOffset) + 0x08);
    writeData(Enemy_State + rear, M(Enemy_State + enemyOffset));         // copy enemy state directly from front to rear
    writeData(Enemy_MovingDir + rear, M(Enemy_MovingDir + enemyOffset)); // copy moving direction also
    const uint8_t front = M(ObjectOffset);                               // save enemy object offset of front
    writeData(ObjectOffset, rear);      // put enemy object offset of rear as current
    writeData(Enemy_ID + rear, Bowser); // set bowser's enemy identifier, store in bowser's rear object
    ProcessBowserHalf(rear);            // do a sub here to process bowser's rear
    writeData(ObjectOffset, front);     // get original enemy object offset
    writeData(BowserGfxFlag, 0x00);     // nullify bowser's front/rear graphics flag
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (whichever half is being processed)
// Outputs: none
void SMBEngine::ProcessBowserHalf(uint8_t e)
{
    ++M(BowserGfxFlag); // increment bowser's graphics flag, then run subroutines
    RunRetainerObj(e);  // to get offscreen bits, relative position and draw bowser (finally!)
    if (M(Enemy_State + e) != 0)
    {
        return; // if either enemy object not in normal state, branch to leave
    }
    writeData(Enemy_BoundBoxCtrl + e, 0x0a); // set bounding box size control
    GetEnemyBoundBox(e);                     // get bounding box coordinates
    PlayerEnemyCollision(e);                 // do player-to-enemy collision detection
}

//------------------------------------------------------------------------

// check enemy buffer flag for non-active enemy slot
// Inputs: startSlot = enemy slot to start scanning downward from
// Outputs: none
void SMBEngine::ChkNoEn(uint8_t startSlot)
{
    // ChkNoEn: scan downward until an empty slot turns up, or the slots run out
    uint8_t slot = startSlot;
    while (M(Enemy_Flag + slot) != 0)
    {
        --slot; // otherwise check next slot
        if ((slot & 0x80) != 0)
        {
            // no empty slots were found, leave
            return;
        }
    }

    // CreateL: initialize enemy state
    writeData(Enemy_State + slot, 0x00);
    writeData(Enemy_ID + slot, Lakitu); // create lakitu enemy object
    SetupLakitu(slot);                  // do a sub to set up lakitu
    PutAtRightExtent(0x20, slot);       // finish setting up lakitu
    // ExLSHand
}

//------------------------------------------------------------------------

// Inputs: verticalPos = vertical position to set; e = enemy object buffer offset
// Outputs: none
void SMBEngine::PutAtRightExtent(uint8_t verticalPos, uint8_t e)
{
    uint32_t wide = 0;

    writeData(Enemy_Y_Position + e, verticalPos);                         // set vertical position
    wide = ((M(ScreenRight_PageLoc) << 8) | M(ScreenRight_X_Pos)) + 0x20; // place enemy 32 pixels beyond right side of screen
    writeData(Enemy_X_Position + e, LOBYTE(wide));
    writeData(Enemy_PageLoc + e, HIBYTE(wide));
    FinishFlame(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (the flame's slot)
// Outputs: none
void SMBEngine::FinishFlame(uint8_t e)
{
    // set $08 for bounding box control
    writeData(Enemy_BoundBoxCtrl + e, 0x08);
    // set high byte of vertical and
    writeData(Enemy_Y_HighPos + e, 0x01); // enemy buffer flag
    writeData(Enemy_Flag + e, 0x01);
    a = 0x00;
    writeData(Enemy_X_MoveForce + e, 0x00); // initialize horizontal movement force, and
    writeData(Enemy_State + e, 0x00);       // enemy state
}

//------------------------------------------------------------------------

// Inputs: slot = enemy object buffer offset (candidate lakitu/spiny slot; must be less than 5)
// Outputs: none
void SMBEngine::LakituAndSpinyHandler(uint8_t slot)
{
    if (M(FrenzyEnemyTimer) != 0) // if timer here not expired, leave
    {
        return;
    }
    if (slot >= 0x05)
    {
        return;
    }
    writeData(FrenzyEnemyTimer, 0x80); // set timer
    ChkLak(0x04, slot);                // start with the last enemy slot
}

//------------------------------------------------------------------------

// check all enemy slots to see
// Inputs: startSlot = enemy slot to start scanning downward from, looking for lakitu;
// spinySlot = the new spiny's slot, used once lakitu is found
// Outputs: none
void SMBEngine::ChkLak(uint8_t startSlot, uint8_t spinySlot)
{
    const uint8_t PRDiffAdjustData_data[] = {0x26, 0x2c, 0x32, 0x38, 0x20, 0x22, 0x24, 0x26, 0x13, 0x14, 0x15, 0x16};

    // ChkLak: scan downward until lakitu turns up, or the slots run out
    uint8_t lakituSlot = startSlot;
    while (M(Enemy_ID + lakituSlot) != Lakitu)
    {
        --lakituSlot; // otherwise check another slot
        if ((lakituSlot & 0x80) != 0)
        {
            // No lakitu in any slot. Once the reappearance timer is far enough along, go and
            // make a new one.
            ++M(LakituReappearTimer); // increment reappearance timer
            if (M(LakituReappearTimer) < 0x07)
            {
                return; // if not, leave
            }
            ChkNoEn(0x04); // start with the last enemy slot again
            return;
        }
    }

    // CreateSpiny: if player above a certain point, branch to leave
    if (M(Player_Y_Position) < 0x2c)
    {
        return;
    }
    // if lakitu is not in normal state, branch to leave
    if (M(Enemy_State + lakituSlot) != 0)
    {
        return;
    }

    // store horizontal coordinates (high and low) of lakitu
    writeData(Enemy_PageLoc + spinySlot, M(Enemy_PageLoc + lakituSlot)); // into the coordinates of the spiny we're going to create
    writeData(Enemy_X_Position + spinySlot, M(Enemy_X_Position + lakituSlot));
    // put spiny within vertical screen unit
    writeData(Enemy_Y_HighPos + spinySlot, 0x01);
    // put spiny eight pixels above where lakitu is
    writeData(Enemy_Y_Position + spinySlot, M(Enemy_Y_Position + lakituSlot) - 0x08);

    // get 2 LSB of LSFR to use as the offset into the difference adjustments
    uint8_t diffIndex = M(PseudoRandomBitReg + spinySlot) & 0b00000011;
    // DifLoop: get three values and save them to $01-$03, stepping four bytes for each value
    for (int i = 0x02; i >= 0; --i)
    {
        writeData(0x01 + i, PRDiffAdjustData_data[diffIndex]);
        diffIndex += 4;
    }

    const uint8_t spiny = M(ObjectOffset); // get enemy object buffer offset
    PlayerLakituDiff(spiny);               // move enemy, change direction, get value - difference

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

    SmallBBox(spiny);
    writeData(Enemy_X_Speed + spiny, 0); // set horizontal speed to zero because previous contents
    // branch if ((a & 0x80) == 0) never taken for the same reason
    writeData(Enemy_MovingDir + spiny, 0x01);
    writeData(Enemy_Y_Speed + spiny, 0xfd); // set vertical speed to move upwards
    writeData(Enemy_Flag + spiny, 0x01);    // enable enemy object by setting flag
    writeData(Enemy_State + spiny, 0x05);   // put spiny in egg state and leave

    // ChpChpEx
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (the flame's own slot)
// Outputs: none
void SMBEngine::InitBowserFlame(uint8_t e)
{
    const uint8_t FlameYMFAdderData_data[] = {0xff, 0x01};

    if (M(FrenzyEnemyTimer) != 0)
    {
        return; // timer not expired yet, leave
    }
    writeData(Enemy_Y_MoveForce + e, 0x00);                           // reset something here
    writeData(NoiseSoundQueue, M(NoiseSoundQueue) | Sfx_BowserFlame); // load bowser's flame sound into queue

    const uint8_t bowser = M(BowserFront_Offset); // get bowser's buffer offset
    // check for bowser; anything else spawns the flame at the right extent instead
    if (M(Enemy_ID + bowser) != Bowser)
    {
        // get timer data based on flame counter; add 32 frames by default, or 16 for secondary hard mode
        const uint8_t flameTimer = SetFlameTimer() + ((M(SecondaryHardMode) != 0) ? 0x10 : 0x20);
        // SetFrT: set timer accordingly
        writeData(FrenzyEnemyTimer, flameTimer);

        const uint8_t randomOfs = M(PseudoRandomBitReg + e) & 0b00000011; // get 2 LSB from first part of LSFR
        writeData(BowserFlamePRandomOfs + e, randomOfs);                  // set here
        // load vertical position based on pseudorandom offset
        PutAtRightExtent(M(FlameYPosData + randomOfs), e);
        return;
    }

    // SpawnFromMouth: get bowser's horizontal position, subtract 14 pixels,
    // save as flame's horizontal position
    writeData(Enemy_X_Position + e, M(Enemy_X_Position + bowser) - 0x0e);
    writeData(Enemy_PageLoc + e, M(Enemy_PageLoc + bowser)); // copy page location from bowser to flame
    // save bowser's vertical position plus 8 pixels as flame's vertical position
    writeData(Enemy_Y_Position + e, M(Enemy_Y_Position + bowser) + 0x08);
    const uint8_t randomOfs = M(PseudoRandomBitReg + e) & 0b00000011; // get 2 LSB from first part of LSFR
    writeData(Enemy_YMF_Dummy + e, randomOfs);                        // save here
    uint8_t adderOfs = 0x00;                                          // load default offset
    // get value here using bits as offset; if less, do not increment offset
    if (M(FlameYPosData + randomOfs) >= M(Enemy_Y_Position + e))
    {
        adderOfs = 0x01; // otherwise increment now
    } // SetMF: get value here and save
    writeData(Enemy_Y_MoveForce + e, FlameYMFAdderData_data[adderOfs]); // to vertical movement force
    writeData(EnemyFrenzyBuffer, 0x00);                                 // clear enemy frenzy buffer
    FinishFlame(e);
}

//------------------------------------------------------------------------

// get coordinates relative to screen
// Inputs: e = enemy object buffer offset (power-up's slot, typically 5)
// Outputs: none
void SMBEngine::RunPUSubs(uint8_t e)
{
    RelativeEnemyPosition(e);
    GetEnemyOffscreenBits(e); // get offscreen bits
    GetEnemyBoundBox(e);     // get bounding box coordinates
    DrawPowerUp();           // draw the power-up object
    PlayerEnemyCollision(e); // check for collision with player
    OffscreenBoundsCheck(e); // check to see if it went offscreen

    // ExitPUp: and we're done
}

//------------------------------------------------------------------------

// Inputs: none (always operates on the fixed power-up slot 5)
// Outputs: none
void SMBEngine::PowerUpObjHandler()
{
    writeData(ObjectOffset, 0x05); // set object offset for last slot in enemy object buffer

    const uint8_t powerUpState = M(Enemy_State + 5); // check power-up object's state
    if (powerUpState == 0)
    {
        return; // if not set, branch to leave
    }

    // d7 of the object state means the power-up has finished growing and is now moving
    if ((powerUpState & 0x80) != 0)
    {
        // if master timer control set, branch ahead to enemy object routines
        if (M(TimerControl) != 0)
        {
            RunPUSubs(0x05);
            return;
        }
        const uint8_t powerUpType = M(PowerUpType); // check power-up type
        // ShroomM: the normal mushroom and the 1-up mushroom both just move
        if (powerUpType == 0x00 || powerUpType == 0x03)
        {
            MoveNormalEnemy(0x05);
            EnemyToBGCollisionDet(0x05); // deal with collisions
            RunPUSubs(0x05);             // run the other subroutines
            return;
        }
        if (powerUpType != 0x02)
        {
            RunPUSubs(0x05); // if not star, branch elsewhere to skip movement
            return;
        }
        MoveJumpingEnemy(0x05); // otherwise impose gravity on star power-up and make it jump
        EnemyJump(0x05);        // note that green paratroopa shares the same code here
        RunPUSubs(0x05);        // then jump to other power-up subroutines
        return;
    }

    // GrowThePowerUp: rise out of the block a pixel every fourth frame
    if ((M(FrameCounter) & 0x03) == 0)
    {
        --M(Enemy_Y_Position + 5);                      // decrement vertical coordinate slowly
        const uint8_t risingState = M(Enemy_State + 5); // load power-up object state
        ++M(Enemy_State + 5);                           // increment state for next frame (to make power-up rise)
        if (risingState >= 0x11)
        {
            // fully risen: start it moving to the right
            writeData(Enemy_X_Speed + 5, 0x10);     // set horizontal speed
            writeData(Enemy_State + 5, 0b10000000); // and then set d7 in power-up object's state
            writeData(Enemy_SprAttrib + 5, 0x00);   // initialize background priority bit set here
            writeData(Enemy_MovingDir + 5, 0x01);   // set right moving direction
        }
    }

    // ChkPUSte: check power-up object's state
    if (M(Enemy_State + 5) < 0x06)
    {
        return; // if not far enough along, don't even bother running these routines
    }
    RunPUSubs(0x05);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::RunNormalEnemies(uint8_t e)
{
    writeData(Enemy_SprAttrib + e, 0x00); // init sprite attributes
    GetEnemyOffscreenBits(e);
    RelativeEnemyPosition(e);
    EnemyGfxHandler(e);
    GetEnemyBoundBox(e);
    EnemyToBGCollisionDet(e);
    EnemiesCollision(e);
    PlayerEnemyCollision(e);
    if (M(TimerControl) == 0) // if master timer control set, skip to last routine
    {
        EnemyMovementSubs(e);
    } // SkipMove
    OffscreenBoundsCheck(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::EnemyMovementSubs(uint8_t e)
{
    switch (M(Enemy_ID + e))
    {
    case 0:
        MoveNormalEnemy(e); // only objects $00-$14 use this table
        return;
    case 1:
        MoveNormalEnemy(e);
        return;
    case 2:
        MoveNormalEnemy(e);
        return;
    case 3:
        MoveNormalEnemy(e);
        return;
    case 4:
        MoveNormalEnemy(e);
        return;
    case 5:
        ProcHammerBro(e);
        return;
    case 6:
        MoveNormalEnemy(e);
        return;
    case 7:
        MoveBloober(e);
        return;
    case 8:
        MoveBulletBill(e);
        return;
    case 9:
        return;
    case 10:
        MoveSwimmingCheepCheep(e);
        return;
    case 11:
        MoveSwimmingCheepCheep(e);
        return;
    case 12:
        MovePodoboo(e);
        return;
    case 13:
        MovePiranhaPlant(e);
        return;
    case 14:
        MoveJumpingEnemy(e);
        return;
    case 15:
        ProcMoveRedPTroopa(e);
        return;
    case 16:
        MoveFlyGreenPTroopa(e);
        return;
    case 17:
        MoveLakitu(e);
        return;
    case 18:
        MoveNormalEnemy(e);
        return;
    case 19:
        return; // dummy
    case 20:
        MoveFlyingCheepCheep(e);
        return;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset (hammer bro's slot)
// Outputs: none
void SMBEngine::ProcHammerBro(uint8_t e)
{
    const uint8_t HammerThrowTmrData_data[] = {0x30, 0x1c};

    // check hammer bro's enemy state for d5 set; if not set, go ahead with code
    if ((M(Enemy_State + e) & 0b00100000) != 0)
    {
        MoveD_EnemyVertically(e); // otherwise move the defeated hammer bro downwards
        MoveEnemyHorizontally(e); // and then horizontally
        return;
    } // ChkJH: check jump timer
    if (M(HammerBroJumpTimer + e) != 0)
    {                                // if expired, branch to jump
        --M(HammerBroJumpTimer + e); // otherwise decrement jump timer
        // check offscreen bits
        if ((M(Enemy_OffscreenBits) & 0b00001100) != 0)
        {
            MoveHammerBroXDir(e); // if hammer bro a little offscreen, skip to movement code
            return;
        }
        // Only an expired hammer throwing timer gets to throw; it reloads the timer and tries to
        // spawn the hammer, which can still fail if there is no room for the object.
        bool hammerSpawned = false;
        if (M(HammerThrowingTimer + e) == 0)
        {
            // get timer data using the secondary hard mode flag as offset, and set as new timer
            writeData(HammerThrowingTimer + e, HammerThrowTmrData_data[M(SecondaryHardMode)]);
            hammerSpawned = SpawnHammerObj(); // do a sub here to spawn hammer object
        }
        if (hammerSpawned)
        {
            // set d3 in enemy state for hammer throw
            writeData(Enemy_State + e, M(Enemy_State + e) | 0b00001000);
        }
        else
        {
            // DecHT: decrement timer
            --M(HammerThrowingTimer + e);
        }
        MoveHammerBroXDir(e); // jump to move hammer bro
        return;
    }

    // HammerBroJumpCode: get hammer bro's enemy state, masking out all but the 3 LSB
    if ((M(Enemy_State + e) & 0b00000111) == 0x01)
    {
        MoveHammerBroXDir(e); // if set, branch ahead to moving code
        return;
    }

    // load default value here and save into temp variable for now
    writeData(0x00, 0x00);
    // check hammer bro's vertical coordinate
    const uint8_t yPos = M(Enemy_Y_Position + e);
    if ((yPos & 0x80) != 0)
    {
        SetHJ(0xfa, e); // if on the bottom half of the screen, use the default vertical speed
        return;
    }
    ++M(0x00); // increment preset value to $01
    if (yPos < 0x70)
    {
        // if above the middle of the screen, use the alternate vertical speed and $01
        SetHJ(0xfd, e);
        return;
    }
    --M(0x00); // otherwise return value to $00
    // get part of LSFR, mask out all but LSB
    if ((M(PseudoRandomBitReg + 1 + e) & 0x01) != 0)
    {
        // if d0 of LSFR set, branch and use the alternate speed and $00
        SetHJ(0xfd, e);
        return;
    }
    SetHJ(0xfa, e); // otherwise reset to default vertical speed
}

//------------------------------------------------------------------------

// set vertical speed for jumping
// Inputs: verticalSpeed = vertical speed to set; e = enemy object buffer offset (reads zero-page
// 0x00 for the bitmask the caller left there)
// Outputs: none
void SMBEngine::SetHJ(uint8_t verticalSpeed, uint8_t e)
{
    const uint8_t HammerBroJumpLData_data[] = {0x20, 0x37};

    writeData(Enemy_Y_Speed + e, verticalSpeed);
    // set d0 in enemy state for jumping
    writeData(Enemy_State + e, M(Enemy_State + e) | 0x01);

    // use the preset value as a bitmask against part of the LSFR, to use the result as an offset;
    // in anything but secondary hard mode the offset is 0
    uint8_t jumpLengthIndex = M(0x00) & M(PseudoRandomBitReg + 2 + e);
    if (M(SecondaryHardMode) == 0)
    {
        jumpLengthIndex = 0x00;
    } // HJump: get jump length timer data using offset from before
    writeData(EnemyFrameTimer + e, HammerBroJumpLData_data[jumpLengthIndex]); // save in enemy timer
    // get contents of part of LSFR, set d7 and d6, then store in jump timer
    writeData(HammerBroJumpTimer + e, M(PseudoRandomBitReg + 1 + e) | 0b11000000);
    MoveHammerBroXDir(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveHammerBroXDir(uint8_t e)
{
    // change hammer bro's direction every 64 frames: d6 set in the counter moves him a little to
    // the right, clear a little to the left
    const uint8_t speed = ((M(FrameCounter) & 0b01000000) != 0) ? 0xfc : 0x04;
    writeData(Enemy_X_Speed + e, speed); // Shimmy: store horizontal speed

    // get horizontal difference between player and hammer bro
    const auto [enemyRightOfPlayer, diff] = PlayerEnemyDiff(e);
    if ((diff & 0x80) != 0)
    {
        SetShim(0x01, e); // if enemy to the left of player, face right
        return;
    }
    // check walking timer
    if (M(EnemyIntervalTimer + e) != 0)
    {
        SetShim(0x02, e); // if not yet expired, skip to set moving direction, facing left
        return;
    }
    writeData(Enemy_X_Speed + e, 0xf8); // otherwise, make the hammer bro walk left towards player
    SetShim(0x02, e);
}

//------------------------------------------------------------------------

// set moving direction
// Inputs: movingDir = moving direction to store; e = enemy object buffer offset
// Outputs: none
void SMBEngine::SetShim(uint8_t movingDir, uint8_t e)
{
    writeData(Enemy_MovingDir + e, movingDir);
    MoveNormalEnemy(e);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveNormalEnemy(uint8_t e)
{
    const uint8_t RevivedXSpeed_data[] = {0x08, 0xf8, 0x0c, 0xf4};

    const uint8_t enemyState = M(Enemy_State + e);
    if ((enemyState & 0b01000000) != 0)
    {             // d6 set
        FallE(e); // move enemy vertically, then horizontally if necessary
        return;
    }
    if ((enemyState & 0x80) != 0)
    {                    // d7 set
        SteadM(0x00, e); // move enemy horizontally, without decelerating it
        return;
    }
    if ((enemyState & 0b00100000) != 0)
    {
        // MoveDefeatedEnemy: d5 set
        MoveD_EnemyVertically(e); // execute sub to move defeated enemy downwards
        MoveEnemyHorizontally(e); // now move defeated enemy horizontally
        return;
    }

    const uint8_t stunnedState = enemyState & 0b00000111; // d2-d0 of enemy state
    if (stunnedState == 0)
    {
        SteadM(0x00, e); // enemy in normal state, move it horizontally without decelerating it
        return;
    }
    if (stunnedState == 0x05 || stunnedState < 0x03)
    {
        FallE(e); // the state used by spiny's egg, and the two low stunned states, just fall
        return;
    }

    // ReviveStunned
    const uint8_t stunTimer = M(EnemyIntervalTimer + e);
    if (stunTimer != 0)
    {
        // ChkKillGoomba: a goomba stunned at a certain point in its timer is killed outright
        if (stunTimer != 0x0e)
        {
            return; // not at that point, leave
        }
        if (M(Enemy_ID + e) != Goomba)
        {
            return; // branch if not found
        }
        EraseEnemyObject(e); // otherwise, kill this goomba object
        return;
    }

    writeData(Enemy_State + e, 0x00);                // the timer expired, initialize enemy state to normal
    const uint8_t frameBit = M(FrameCounter) & 0x01; // get d0 of frame counter
    // store as pseudorandom movement direction
    writeData(Enemy_MovingDir + e, frameBit + 1);
    // primary hard mode moves 2 bytes on to the faster half of the data
    const uint8_t speedIndex = (M(PrimaryHardMode) != 0) ? (frameBit + 2) : frameBit;
    // SetRSpd: load and store new horizontal speed, and leave
    writeData(Enemy_X_Speed + e, RevivedXSpeed_data[speedIndex]);

    // NKGmba: leave!
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::EnemyToBGCollisionDet(uint8_t e)
{
    const uint8_t EnemyBGCStateData_data[] = {0x01, 0x01, 0x02, 0x02, 0x02, 0x05};

    bool enemyRightOfPlayer = false;

    // LandEnemyInitState: land the enemy and reset its state. Several places end here, and
    // ProcEnemyDirection below runs straight into it.
    const auto landEnemyInitState = [&]()
    {
        EnemyLanding(e); // land enemy properly
        // if d7 of enemy state is set, branch
        if ((M(Enemy_State + e) & 0b10000000) == 0)
        {
            // otherwise initialize enemy state and leave; note this will also turn spiny's egg
            // into spiny
            writeData(Enemy_State + e, 0x00);
            return;
        }
        // NMovShellFallBit: nullify d6 of enemy state, save other bits, and store
        writeData(Enemy_State + e, M(Enemy_State + e) & 0b10111111);
    };

    // ProcEnemyDirection: turn the enemy to face the player, then land it
    const auto procEnemyDirection = [&]()
    {
        const uint8_t enemyId = M(Enemy_ID + e); // check enemy identifier for goomba
        if (enemyId == Goomba)
        {
            landEnemyInitState();
            return;
        }
        if (enemyId == Spiny)
        {
            writeData(Enemy_MovingDir + e, 0x01); // send enemy moving to the right by default
            writeData(Enemy_X_Speed + e, 0x08);   // set horizontal speed accordingly
            // if timed appropriately, spiny will skip over trying to face the player
            if ((M(FrameCounter) & 0b00000111) == 0)
            {
                landEnemyInitState();
                return;
            }
        }
        // InvtD: load 1 for enemy to face the left (inverted here)
        uint8_t facingDir = 0x01;
        uint8_t diff = 0;
        std::tie(enemyRightOfPlayer, diff) = PlayerEnemyDiff(e); // get horizontal difference between player and enemy
        if ((diff & 0x80) != 0)
        {                // if enemy to the left of player, increment by one for the enemy to
            ++facingDir; // face right (inverted)
        }
        // CNwCDir
        if (facingDir != M(Enemy_MovingDir + e))
        {
            landEnemyInitState();
            return;
        }
        ChkForBump_HammerBroJ(e); // if equal, not facing in correct dir, do sub to turn around
        landEnemyInitState();
    };

    // ChkForRedKoopa
    const auto chkForRedKoopa = [&]()
    {
        // check for red koopa troopa $03 in normal state
        if (M(Enemy_ID + e) == RedKoopa && M(Enemy_State + e) == 0)
        {
            ChkForBump_HammerBroJ(e); // if enemy found and in normal state, branch
            return;
        }
        // Chk2MSBSt: with d7 of the state set, set d6 alongside it; otherwise the old state
        // indexes the new one (GetSteFromD)
        const uint8_t oldState = M(Enemy_State + e);
        const uint8_t newState = ((oldState & 0x80) != 0) ? (oldState | 0b01000000) : EnemyBGCStateData_data[oldState];
        // SetD6Ste: set as new state
        writeData(Enemy_State + e, newState);
        DoEnemySideCheck(e); // then check for horizontal blockage and leave
    };

    // NoUnderHammerBro: if hammer bro is not standing on anything, set d0 in the enemy state to
    // indicate jumping or falling, then leave
    const auto noUnderHammerBro = [&]() { writeData(Enemy_State + e, M(Enemy_State + e) | 0x01); };

    // HammerBroBGColl
    const auto hammerBroBGColl = [&]()
    {
        const uint8_t blockUnder = ChkUnderEnemy(e); // check to see if hammer bro is standing on anything
        if (blockUnder == 0)
        {
            noUnderHammerBro();
            return;
        }
        if (blockUnder == 0x23)
        {
            KillEnemyAboveBlock();
            return;
        }
        // check timer used by hammer bro
        if (M(EnemyFrameTimer + e) != 0)
        {
            noUnderHammerBro(); // branch if not expired
            return;
        }
        // save d7 and d3 from enemy state, nullify other bits, and store
        writeData(Enemy_State + e, M(Enemy_State + e) & 0b10001000);
        EnemyLanding(e);     // modify vertical coordinate, speed and something else
        DoEnemySideCheck(e); // then check for horizontal blockage and leave
    };

    // check enemy state for d6 set; if set, leave
    if ((M(Enemy_State + e) & 0b00100000) != 0)
    {
        return;
    }
    // otherwise, do a subroutine here; leave if enemy vertical coord + 62 < 68
    if (!SubtEnemyYPos(e))
    {
        return;
    }

    const uint8_t enemyId = M(Enemy_ID + e);
    if (enemyId == Spiny && M(Enemy_Y_Position + e) < 0x25)
    {
        return;
    }
    // DoIDCheckBGColl
    if (enemyId == GreenParatroopaJump)
    {
        EnemyJump(e); // jump elsewhere
        return;
    }
    // HBChk: check for hammer bro
    if (enemyId == HammerBro)
    {
        hammerBroBGColl();
        return;
    }
    // CInvu: only enemy objects < $07, the spiny ($12) and the power-up object ($2e) go on
    if (enemyId != Spiny && enemyId != PowerUpObject && enemyId >= 0x07)
    {
        return;
    }

    // YesIn
    const uint8_t blockUnder = ChkUnderEnemy(e);
    // HandleEToBGCollision: with no block underneath, or a blank $26, coins or hidden blocks,
    // the enemy falls through
    if (blockUnder == 0 || ChkForNonSolids(blockUnder))
    {
        chkForRedKoopa();
        return;
    }

    // check for blank metatile $23
    if (blockUnder == 0x23)
    {
        const uint8_t vertCoord = M(0x02); // get vertical coordinate used to find block
        // store default blank metatile in that spot so we won't
        writeData(W(0x06) + vertCoord, 0x00); // trigger this routine accidentally again
        const uint8_t enemyIdAbove = M(Enemy_ID + e);
        if (enemyIdAbove >= 0x15)
        {
            ChkToStunEnemies(enemyIdAbove, e);
            return;
        }
        if (enemyIdAbove == Goomba)
        {
            KillEnemyAboveBlock(); // if enemy object IS goomba, do this sub
        } // GiveOEPoints

        // award 100 points for hitting block beneath enemy
        const uint8_t floateyX = SetupFloateyNumber(1, e);
        // Bug in the original game: there should be another "M(Enemy_ID + e)" load here,
        // but instead the x-coordinate of the created floatey gets passed to ChkToStunEnemies.
        // This causes https://themushroomkingdom.net/bugs/7
        ChkToStunEnemies(floateyX, e);
        return;
    }

    // LandEnemyProperly: check lower nybble of vertical coordinate saved earlier, less eight
    // pixels; out of range means it was $0d-$0f before the subtract
    if (static_cast<uint8_t>(M(0x04) - 0x08) >= 0x05)
    {
        chkForRedKoopa();
        return;
    }

    const uint8_t landedState = M(Enemy_State + e);
    if ((landedState & 0b01000000) != 0)
    {
        landEnemyInitState(); // d6 in enemy state is set
        return;
    }
    // SChkA: lower nybble < $0d with d7 set but d6 not, and the normal state, both just check
    // the enemy's sides (ChkLandedEnemyState)
    if ((landedState & 0x80) != 0 || landedState == 0)
    {
        DoEnemySideCheck(e);
        return;
    }
    if (landedState == 0x05)
    {
        procEnemyDirection();
        return;
    }
    if (landedState < 0x03)
    {
        // load enemy state again (why?)
        if (M(Enemy_State + e) != 0x02)
        {
            procEnemyDirection();
            return;
        }
        // load default timer here, or $00 if the enemy identifier is spiny
        const uint8_t stunTimer = (M(Enemy_ID + e) == Spiny) ? 0x00 : 0x10;
        // SetForStn: set timer here
        writeData(EnemyIntervalTimer + e, stunTimer);
        // set state here, apparently used to render upside-down koopas and buzzy beetles
        writeData(Enemy_State + e, 0x03);
        EnemyLanding(e); // then land it properly
    }
    // ExSteChk: anything in a higher numbered state just leaves
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::DoEnemySideCheck(uint8_t e)
{
    if (M(Enemy_Y_Position + e) >= 0x20) // if enemy within status bar, branch to leave
    {
        writeData(0xeb, 0x02); // set value here in what is also used as OAM data offset

        // start by finding block to the left of enemy ($00,$14), through
        // ($10, $14) pixel coordinates
        for (uint8_t cornerIdx = 0x16; cornerIdx < 0x18; ++cornerIdx) // SdeCLoop
        {
            // seek a block only on the side the enemy is actually moving towards
            if (M(0xeb) == M(Enemy_MovingDir + e))
            {
                // set coordinate-selector flag to save horizontal coordinate; find block to
                // left or right of enemy object
                const uint8_t metatile = BlockBufferChk_Enemy(0x01, cornerIdx, e);
                // a solid block on that side blocks the enemy
                if (metatile != 0 && !ChkForNonSolids(metatile))
                {
                    ChkForBump_HammerBroJ(e);
                    return;
                }
            }

            // NextSdeC: move to the next direction
            --M(0xeb);
        }
    } // ExESdeC
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::ChkForBump_HammerBroJ(uint8_t e)
{
    if (e == 0x05)
    {
        NoBump(e); // and if so, branch ahead and do not play sound
        return;
    }
    // if enemy state d7 not set, branch ahead and do not play sound
    if ((M(Enemy_State + e) & 0x80) == 0)
    {
        NoBump(e);
        return;
    }
    writeData(Square1SoundQueue, Sfx_Bump); // otherwise, play bump sound (never played if branching from ChkForRedKoopa)
    NoBump(e);
}

//------------------------------------------------------------------------

// check for hammer bro
// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::NoBump(uint8_t e)
{
    if (M(Enemy_ID + e) == 0x05)
    {                          // branch if not found
        writeData(0x00, 0x00); // initialize value here for bitmask
        // jump to code that makes hammer bro jump, at the default vertical speed for jumping
        SetHJ(0xfa, e);
        return;
    } // InvEnemyDir
    RXSpd(e); // jump to turn the enemy around
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::EnemyJump(uint8_t e)
{
    // Every way of declining to jump lands on the same tail (DoSide), so the jump is a lambda
    // that returns and the side check below runs unconditionally afterwards.
    const auto jump = [&]()
    {
        if (!SubtEnemyYPos(e))
        {
            return; // enemy vertical coord + 62 < 68, leave
        }
        if (static_cast<uint8_t>(M(Enemy_Y_Speed + e) + 0x02) < 0x03)
        {
            return;
        }
        const uint8_t metatile = ChkUnderEnemy(e); // check to see if green paratroopa is standing on anything
        if (metatile == 0)
        {
            return; // it is not, leave
        }
        if (ChkForNonSolids(metatile))
        {
            return; // what it is standing on is not solid, leave
        }
        EnemyLanding(e);                    // change vertical coordinate and speed
        writeData(Enemy_Y_Speed + e, 0xfd); // make the paratroopa jump again
    };
    jump();

    // DoSide: check for horizontal blockage, then leave
    DoEnemySideCheck(e);
}

//------------------------------------------------------------------------

// Inputs: enemyOffset = enemy object buffer offset (the slot the area-parser task loop is on).
// Two member-x writes remain as a transition ABI: ProcLoopCommand still reads x = enemyOffset,
// and the parameterless enemy handlers (RunNormalEnemies, RunFireworks, RunBowser,
// VineObjectHandler, JumpspringHandler) read x = M(ObjectOffset).
// Outputs: none
void SMBEngine::EnemiesAndLoopsCore(uint8_t enemyOffset)
{
    const uint8_t enemyFlag = M(Enemy_Flag + enemyOffset); // check data here for MSB set

    if ((enemyFlag & 0x80) != 0)
    {
        // ChkBowserF: the low nybble points at a second enemy flag
        const uint8_t otherFlagOffset = enemyFlag & 0b00001111; // mask out high nybble
        if (M(Enemy_Flag + otherFlagOffset) != 0)
        {
            return;
        }
        writeData(Enemy_Flag + enemyOffset, 0x00); // if second enemy flag not set, also clear first one
        return;                                     // ExitELCore
    }
    if (enemyFlag == 0)
    {
        // ChkAreaTsk: check number of tasks to perform
        if ((M(AreaParserTaskNum) & 0x07) == 0x07)
        {
            return;
        }
        x = enemyOffset;   // ProcLoopCommand is still register-based off the loop slot
        ProcLoopCommand(); // otherwise, jump to process loop command/load enemies
        return;
    }

    // RunEnemyObjectsCore
    const uint8_t self = M(ObjectOffset); // get offset for enemy object buffer
    x = self;                             // keep member offset for the register-based handlers below
    const uint8_t enemyId = M(Enemy_ID + self);
    // load value 0 for jump engine by default; otherwise subtract $14 from the ID and use as index
    const uint8_t jumpIdx = (enemyId >= 0x15) ? static_cast<uint8_t>(enemyId - 0x14) : 0x00; // JmpEO
    switch (jumpIdx)
    {
    case 0:
        RunNormalEnemies(self); // for objects $00-$14
        return;
    case 1:
        RunBowserFlame(self); // for objects $15-$1f
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
        RunFirebarObj(self);
        return;
    case 8:
        RunFirebarObj(self);
        return;
    case 9:
        RunFirebarObj(self);
        return;
    case 10:
        RunFirebarObj(self);
        return;
    case 11:
        RunFirebarObj(self);
        return;
    case 12:
        RunFirebarObj(self); // for objects $20-$2f
        return;
    case 13:
        RunFirebarObj(self);
        return;
    case 14:
        RunFirebarObj(self);
        return;
    case 15:
        return;
    case 16:
        RunLargePlatform(self);
        return;
    case 17:
        RunLargePlatform(self);
        return;
    case 18:
        RunLargePlatform(self);
        return;
    case 19:
        RunLargePlatform(self);
        return;
    case 20:
        RunLargePlatform(self);
        return;
    case 21:
        RunLargePlatform(self);
        return;
    case 22:
        RunLargePlatform(self);
        return;
    case 23:
        RunSmallPlatform(self);
        return;
    case 24:
        RunSmallPlatform(self);
        return;
    case 25:
        RunBowser(self);
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
        RunStarFlagObj(self);
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
        RunRetainerObj(self);
        return;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (slot that will receive the next-parsed enemy object)
// Outputs: none
void SMBEngine::ProcLoopCommand()
{
    const uint8_t LoopCmdYPosition_data[] = {0x40, 0xb0, 0xb0, 0x80, 0x40, 0x40, 0x80, 0x40, 0xf0, 0xf0, 0xf0};

    const uint8_t LoopCmdPageNumber_data[] = {0x05, 0x09, 0x04, 0x05, 0x06, 0x08, 0x09, 0x0a, 0x06, 0x0b, 0x10};

    const uint8_t LoopCmdWorldNumber_data[] = {0x03, 0x03, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07};

    uint32_t wide = 0;

    // InitMLp: initialize counters used for multi-part loop commands
    const auto initMLp = [&]() {
        a = 0x00;
        writeData(MultiLoopPassCntr, 0x00);
        writeData(MultiLoopCorrectCntr, 0x00);
    };

    // DoLpBack: if player is not in right place, loop back
    const auto doLpBack = [&]() {
        ExecGameLoopback(y);
        KillAllEnemies();
        initMLp();
    };

    // IncMLoop: increment master multi-part counter
    const auto incMLoop = [&]() {
        ++M(MultiLoopPassCntr);
        // have we done all three parts?
        if (M(MultiLoopPassCntr) != 0x03)
        {
            return; // if not, skip this part
        }
        a = M(MultiLoopCorrectCntr); // if so, have we done them all correctly?
        if (a == 0x03)
        {
            initMLp(); // if so, branch past unnecessary check here
            return;
        }
        doLpBack();
    };

    // WrongChk: player fails to pass loop; are we in world 7?
    const auto wrongChk = [&]() {
        if (M(WorldNumber) == World7)
        {
            incMLoop();
            return;
        }
        doLpBack();
    };

    // CheckFrenzyBuffer
    const auto checkFrenzyBuffer = [&]() {
        a = M(EnemyFrenzyBuffer); // if enemy object stored in frenzy buffer
        if (a == 0)
        {                          // then branch ahead to store in enemy object buffer
            a = M(VineFlagOffset); // otherwise check vine flag offset
            if (a != 0x01)
            {
                return; // if other value <> 1, leave
            }
            a = VineObject; // otherwise put vine in enemy identifier
        } // StrFre: store contents of frenzy buffer into enemy identifier value
        writeData(Enemy_ID + x, a);
        InitEnemyObject(); // then go and initialize it
    };

    // ParseRow0e
    const auto parseRow0e = [&]() {
        ++y; // increment Y to load third byte of object
        ++y;
        a = M(W(EnemyData) + y) >> 5; // move 3 MSB to the bottom, effectively making %xxx00000 into %00000xxx
        if (a == M(WorldNumber))
        {        // if not, do not use (this allows multiple uses
            --y; // of the same area, like the underground bonus areas)
            // otherwise, get second byte and use as offset
            writeData(AreaPointer, M(W(EnemyData) + y)); // to addresses for level and enemy object data
            ++y;
            // get third byte again, and this time mask out
            a = M(W(EnemyData) + y) & 0b00011111; // the 3 MSB from before, save as page number to be
            writeData(EntrancePage, a);           // used upon entry to area, if area is entered
        } // NotUse
        Inc3B();
    };

    while (true) // ProcLoopCommand
    {
        // check if loop command was found, and if we're still on the first page
        if (M(LoopCommand) != 0 && M(CurrentColumnPos) == 0)
        {
            y = 0x0b; // start at the end of each set of loop data
            while (true)
            {
                --y; // FindLoop
                if ((y & 0x80) != 0)
                {
                    break; // if all data is checked and not match, do not loop
                }
                // check to see if one of the world numbers
                if (M(WorldNumber) != LoopCmdWorldNumber_data[y])
                {
                    continue;
                }
                // check to see if one of the page numbers
                if (M(CurrentPageLoc) != LoopCmdPageNumber_data[y])
                {
                    continue;
                }
                // check to see if the player is at the correct position and state
                if (M(Player_Y_Position) != LoopCmdYPosition_data[y] || M(Player_State) != 0x00)
                {
                    wrongChk(); // if not, player fails to pass loop, and loopback
                }
                // are we in world 7? (check performed on correct position and state)
                else if (M(WorldNumber) != World7)
                {
                    initMLp(); // if not, initialize flags used there
                }
                else
                {
                    ++M(MultiLoopCorrectCntr); // increment counter for correct progression
                    incMLoop();
                }
                // InitLCmd: initialize loop command flag
                a = 0x00;
                writeData(LoopCommand, 0x00);
                break;
            }
        }

        // ChkEnemyFrenzy
        a = M(EnemyFrenzyQueue); // check for enemy object in frenzy queue
        if (a != 0)
        {                                    // if not, skip this part
            writeData(Enemy_ID + x, a);      // store as enemy object identifier here
            writeData(Enemy_Flag + x, 0x01); // activate enemy object flag
            a = 0x00;
            writeData(Enemy_State + x, 0x00); // initialize state and frenzy queue
            writeData(EnemyFrenzyQueue, 0x00);
            InitEnemyObject(); // and then jump to deal with this enemy
            return;
        } // ProcessEnemyData
        y = M(EnemyDataOffset);  // get offset of enemy object data
        a = M(W(EnemyData) + y); // load first byte
        if (a == 0xff)
        {
            checkFrenzyBuffer(); // if found, jump to check frenzy buffer
            return;
        } // CheckEndofBuffer
        a &= 0b00001111;              // check for special row $0e
        if (a != 0x0e && x >= 0x05)   // if found, or not at end of buffer, branch to check bounds
        {
            ++y;
            // check for specific value here
            a = M(W(EnemyData) + y) & 0b00111111; // not sure what this was intended for, exactly
            if (a != 0x2e)
            {                                     // but it has the effect of keeping enemies out of
                return;                           // the sixth slot
            }
        }

        // CheckRightBounds
        wide = ((M(ScreenRight_PageLoc) << 8) | M(ScreenRight_X_Pos)) + 0x30; // add 48 to pixel coordinate of right boundary
        a = LOBYTE(wide) & 0b11110000;                                        // store high nybble
        writeData(0x07, a);
        a = HIBYTE(wide);   // add carry to page location of right boundary
        writeData(0x06, a); // store page location + carry
        y = M(EnemyDataOffset);
        ++y;
        a = M(W(EnemyData) + y); // if MSB of enemy object is clear, branch to check for row $0f
        a <<= 1;
        // if MSB is set and page select not already set, set page select
        if ((M(W(EnemyData) + y) & 0x80) != 0 && M(EnemyObjectPageSel) == 0)
        {
            ++M(EnemyObjectPageSel);
            ++M(EnemyObjectPageLoc); // and increment page control
        }

        // CheckPageCtrlRow
        --y;
        // reread first byte
        a = M(W(EnemyData) + y) & 0x0f;
        // if row $0f found and page select not set, this is a page control row
        if (a == 0x0f && M(EnemyObjectPageSel) == 0)
        {
            ++y;
            // otherwise, get second byte, mask out 2 MSB
            a = M(W(EnemyData) + y) & 0b00111111;
            writeData(EnemyObjectPageLoc, a); // store as page control for enemy object data
            ++M(EnemyDataOffset);             // increment enemy object data offset 2 bytes
            ++M(EnemyDataOffset);
            ++M(EnemyObjectPageSel); // set page select for enemy object data and
            continue;                // jump back to process loop commands again
        }

        // PositionEnemyObj
        // store page control as page location
        writeData(Enemy_PageLoc + x, M(EnemyObjectPageLoc)); // for enemy object
        // get first byte of enemy object
        a = M(W(EnemyData) + y) & 0b11110000;
        writeData(Enemy_X_Position + x, a);                         // store column position
        if (((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x)) // check column position against right boundary
            < ((M(ScreenRight_PageLoc) << 8) | M(ScreenRight_X_Pos)))
        {                                         // if enemy object beyond or at boundary, branch
            a = M(W(EnemyData) + y) & 0b00001111; // check for special row $0e
            if (a == 0x0e)
            {
                parseRow0e();
                return;
            }
            // CheckThreeBytes
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

        // CheckRightExtBounds
        if (((M(0x06) << 8) | M(0x07)) // check right boundary + 48 against the column position
            < ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x)))
        {
            checkFrenzyBuffer(); // if enemy object beyond extended boundary, branch
            return;
        }
        // store value in vertical high byte
        writeData(Enemy_Y_HighPos + x, 0x01);
        a = M(W(EnemyData) + y); // get first byte again
        a <<= 1;                 // multiply by four to get the vertical
        a <<= 1;                 // coordinate
        a <<= 1;
        a <<= 1;
        writeData(Enemy_Y_Position + x, a);
        if (a == 0xe0)
        {
            parseRow0e(); // (necessary if branched to $c1cb)
            return;
        }
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
        if (a >= 0x37 && a < 0x3f) // if $37-$3e, handle enemy group objects
        {
            HandleGroupEnemies(); // DoGroup
            return;
        } // BuzzyBeetleMutate ($3f or more always fails)
        // if goomba and primary hard mode flag set, change goomba to buzzy beetle
        if (a == Goomba && M(PrimaryHardMode) != 0)
        {
            a = BuzzyBeetle;
        }
        // StrID: store enemy object number into buffer
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
    }
}

//------------------------------------------------------------------------

// Inputs: none (acts on the current object offset; CheckpointEnemyID reads it from x)
// Outputs: none
void SMBEngine::InitEnemyObject()
{
    writeData(Enemy_State + M(ObjectOffset), 0x00); // initialize enemy state
    CheckpointEnemyID();                            // jump ahead to run jump engine and subroutines

    // ExEPar: then leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::CheckpointEnemyID()
{
    const uint8_t SwimCC_IDData_data[] = {0x0a, 0x0b};

    const uint8_t Enemy17YPosData_data[] = {0x40, 0x30, 0x90, 0x50, 0x20, 0x60, 0xa0, 0x70};

    // BulletBillCheepCheep: spawn a frenzy bullet bill or swimming cheep-cheep
    const auto bulletBillCheepCheep = [&]() {
        a = M(FrenzyEnemyTimer); // if timer not expired yet, branch to leave
        if (a != 0)
        {
            return;
        }
        a = M(AreaType); // are we in a water-type level?
        if (a != 0)
        {
            // DoBulletBills: start at beginning of enemy slots
            y = 0xff;
            while (true)
            {
                ++y; // BB_SLoop: move onto the next slot
                if (y >= 0x05)
                {
                    break; // FireBulletBill
                }
                // if enemy buffer flag not set,
                if (M(Enemy_Flag + y) == 0)
                {
                    continue; // loop back and check another slot
                }
                a = M(Enemy_ID + y);
                if (a != BulletBill_FrenzyVar)
                {
                    continue; // bullet bill object (frenzy variant)
                }
                return; // ExF17: if found, leave
            }
            // FireBulletBill
            a = M(Square2SoundQueue) | Sfx_Blast; // play fireworks/gunfire sound
            writeData(Square2SoundQueue, a);
            a = BulletBill_FrenzyVar; // load identifier for bullet bill object
            // and fall into Set17ID (unconditional branch)
        }
        else
        {
            if (x >= 0x03)
            {
                return; // if so, branch to leave
            }
            y = 0x00; // load default offset
            if (M(PseudoRandomBitReg + x) >= 0xaa)
            {             // if less than preset, do not increment offset
                y = 0x01; // otherwise increment
            } // ChkW2: check world number
            if (M(WorldNumber) != World2)
            {        // if we're on world 2, do not increment offset
                ++y; // otherwise increment
            } // Get17ID
            a = y;
            a &= 0b00000001; // mask out all but last bit of offset
            y = a;
            a = SwimCC_IDData_data[y]; // load identifier for cheep-cheeps
        }
        // Set17ID: store whatever's in A as enemy identifier
        writeData(Enemy_ID + x, a);
        if (M(BitMFilter) == 0xff)
        {
            a = 0x00; // initialize vertical position filter
            writeData(BitMFilter, 0x00);
        } // GetRBit: get first part of LSFR
        a = M(PseudoRandomBitReg + x) & 0b00000111; // mask out all but 3 LSB
        while (true)
        {
            // ChkRBit: use as offset
            y = a;
            a = M(Bitmasks + y);          // load bitmask
            if ((a & M(BitMFilter)) == 0) // perform AND on filter without changing it
            {
                break;
            }
            ++y; // increment offset
            a = y;
            a &= 0b00000111; // mask out all but 3 LSB thus keeping it 0-7, and do another check
        } // AddFBit: add bit to already set bits in filter
        a |= M(BitMFilter);
        writeData(BitMFilter, a);          // and store
        a = Enemy17YPosData_data[y];       // load vertical position using offset
        PutAtRightExtent(a, x);            // set vertical position and other values
        writeData(Enemy_YMF_Dummy + x, a); // initialize dummy variable
        a = 0x20;                          // set timer
        writeData(FrenzyEnemyTimer, 0x20);
        CheckpointEnemyID(); // process our new enemy object
    };

    // InitEnemyFrenzy
    const auto initEnemyFrenzy = [&]() {
        a = M(Enemy_ID + x);             // load enemy identifier
        writeData(EnemyFrenzyBuffer, a); // save in enemy frenzy buffer
        a -= 0x12;                       // subtract 12 and use as offset for jump engine
        switch (a)
        {
        case 0:
            LakituAndSpinyHandler(x);
            return;
        case 1:
            return;
        case 2:
            InitFlyingCheepCheep(x);
            return;
        case 3:
            InitBowserFlame(x);
            return;
        case 4:
            InitFireworks(x);
            return;
        case 5:
            bulletBillCheepCheep();
            return;
        default:
            bad_jump();
            return;
        }
    };

    a = M(Enemy_ID + x);
    if (a < 0x15)
    {          // and branch straight to the jump engine if found
        y = a; // save identifier in Y register for now
        a = M(Enemy_Y_Position + x);
        a += 0x08;                                  // add eight pixels to what will eventually be the
        writeData(Enemy_Y_Position + x, a);         // enemy object's vertical coordinate ($00-$14 only)
        writeData(EnemyOffscrBitsMasked + x, 0x01); // set offscreen masked bit
        a = y;                                      // get identifier back and use as offset for jump engine
    } // InitEnemyRoutines
    y = (a * 2) + 2;
    switch (a)
    {
    case 0:
        InitNormalEnemy(x); // for objects $00-$0f
        return;
    case 1:
        InitNormalEnemy(x);
        return;
    case 2:
        InitNormalEnemy(x);
        return;
    case 3:
        InitRedKoopa(x);
        return;
    case 4:
        return;
    case 5:
        InitHammerBro(x);
        return;
    case 6:
        InitGoomba(x);
        return;
    case 7:
        InitBloober(x);
        return;
    case 8:
        InitBulletBill(x);
        return;
    case 9:
        return;
    case 10:
        InitCheepCheep(x);
        return;
    case 11:
        InitCheepCheep(x);
        return;
    case 12:
        InitPodoboo(x);
        return;
    case 13:
        InitPiranhaPlant(x);
        return;
    case 14:
        InitJumpGPTroopa(x);
        return;
    case 15:
        InitRedPTroopa(x);
        return;
    case 16:
        InitHorizFlySwimEnemy(x); // for objects $10-$1f
        return;
    case 17:
        InitLakitu(x);
        return;
    case 18:
        initEnemyFrenzy();
        return;
    case 19:
        return;
    case 20:
        initEnemyFrenzy();
        return;
    case 21:
        initEnemyFrenzy();
        return;
    case 22:
        initEnemyFrenzy();
        return;
    case 23:
        initEnemyFrenzy();
        return;
    case 24:
        EndFrenzy(x);
        return;
    case 25:
        return;
    case 26:
        return;
    case 27:
        InitShortFirebar(x);
        return;
    case 28:
        InitShortFirebar(x);
        return;
    case 29:
        InitShortFirebar(x);
        return;
    case 30:
        InitShortFirebar(x);
        return;
    case 31:
        InitLongFirebar(x);
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
        InitBalPlatform(x);
        return;
    case 37:
        InitVertPlatform(x);
        return;
    case 38:
        LargeLiftUp(x);
        return;
    case 39:
        LargeLiftDown(x);
        return;
    case 40:
        InitHoriPlatform(x);
        return;
    case 41:
        InitDropPlatform(x);
        return;
    case 42:
        InitHoriPlatform(x);
        return;
    case 43:
        PlatLiftUp(x);
        return;
    case 44:
        PlatLiftDown(x);
        return;
    case 45:
        InitBowser(x);
        return;
    case 46:
        PwrUpJmp(); // possibly dummy value
        return;
    case 47:
        Setup_Vine(x, y);
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
        InitRetainerObj(x);
        return;
    case 54:
        return;
    default:
        bad_jump();
        return;
    }
}

//------------------------------------------------------------------------

// Inputs: a = the group-enemy descriptor byte (masked to 0x37-0x3e) that ProcLoopCommand's jump
// to DoGroup leaves in a; x = enemy object buffer offset (search starts from slot 0 regardless)
// Outputs: none
void SMBEngine::HandleGroupEnemies()
{
    uint32_t wide = 0;

    const uint8_t groupIndex = a - 0x37; // subtract $37 from second byte read

    // The green koopa troopa is the default; the low four descriptors are goombas instead, or
    // buzzy beetles in primary hard mode.
    uint8_t enemyId = 0x00; // load value for green koopa troopa
    if (groupIndex < 0x04)
    {
        enemyId = Goomba; // load value for goomba enemy
        if (M(PrimaryHardMode) != 0)
        {
            enemyId = BuzzyBeetle; // change to value for buzzy beetle
        }
    } // SnglID: save enemy id here
    writeData(0x01, enemyId);

    // SetYGp: d1 of the descriptor moves the y coordinate up from its default
    writeData(0x00, ((groupIndex & 0x02) != 0) ? 0x70 : 0xb0);

    // get page number and pixel coordinate of right edge of screen
    writeData(0x02, M(ScreenRight_PageLoc)); // save here
    writeData(0x03, M(ScreenRight_X_Pos));   // save here

    // CntGrp: two enemies by default, three if d0 of the descriptor is set
    writeData(NumberofGroupEnemies, ((groupIndex & 0x01) != 0) ? 0x03 : 0x02);

    bool enemiesLeft = true;
    while (enemiesLeft)
    {
        // GrLoop: start at beginning of enemy buffers
        x = 0xff;

        // GSltLp: increment past every slot already stored in the buffer
        do
        {
            ++x;
        } while (x < 0x05 && M(Enemy_Flag + x) != 0);

        if (x >= 0x05)
        {
            break; // ran out of slots
        }

        writeData(Enemy_ID + x, M(0x01));      // store enemy object identifier
        writeData(Enemy_PageLoc + x, M(0x02)); // store page location for enemy object
        a = M(0x03);
        writeData(Enemy_X_Position + x, a); // store x coordinate for enemy object
        wide = ((M(0x02) << 8) | a) + 0x18; // add 24 pixels for next enemy
        writeData(0x03, LOBYTE(wide));
        writeData(0x02, HIBYTE(wide)); // add carry to page location for next enemy
        // store y coordinate for enemy object
        writeData(Enemy_Y_Position + x, M(0x00));
        writeData(Enemy_Y_HighPos + x, 0x01); // put enemy within the screen vertically
        writeData(Enemy_Flag + x, 0x01);      // activate flag for buffer
        CheckpointEnemyID();                  // process each enemy object separately
        --M(NumberofGroupEnemies);            // do this until we run out of enemy objects
        enemiesLeft = M(NumberofGroupEnemies) != 0;
    }

    // NextED: jump to increment data offset and leave
    Inc2B();
}
