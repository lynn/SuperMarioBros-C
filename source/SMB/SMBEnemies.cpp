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

    x = M(ObjectOffset); // get original enemy object offset

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
    writeData(HammerEnemyOffset + miscOffset, x);    // save here
    writeData(Misc_State + miscOffset, 0x90);        // save hammer's state here
    writeData(Misc_BoundBoxCtrl + miscOffset, 0x07); // set something else entirely, here
    return true;
}

//------------------------------------------------------------------------

// Inputs: y = loop command index, offset into AreaDataOfsLoopback_data
// Outputs: none
void SMBEngine::ExecGameLoopback()
{
    const uint8_t AreaDataOfsLoopback_data[] = {0x12, 0x36, 0x0e, 0x0e, 0x0e, 0x32, 0x32, 0x32, 0x0a, 0x26, 0x40};

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
    writeData(EnemyDataOffset, 0x00);    // initialize enemy object data offset
    writeData(EnemyObjectPageLoc, 0x00); // and enemy object page control
    a = AreaDataOfsLoopback_data[y];     // adjust area object offset based on
    writeData(AreaDataOffset, a);        // which loop command we encountered
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset; y = index into PlatPosDataHigh_data/PlatPosDataLow_data
// (0, 1 or 2) selecting the offset to add
// Outputs: a is left holding the new page location, but no caller relies on it (scratch)
void SMBEngine::PosPlatform()
{
    const uint8_t PlatPosDataHigh_data[] = {0x00, 0x00, 0xff};

    const uint8_t PlatPosDataLow_data[] = {0x08, 0x0c, 0xf8};

    uint32_t wide = 0;

    // add or subtract pixels depending on offset, as one 16-bit page:coordinate
    wide = ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x)) + ((PlatPosDataHigh_data[y] << 8) | PlatPosDataLow_data[y]);
    writeData(Enemy_X_Position + x, LOBYTE(wide)); // store as new horizontal coordinate
    writeData(Enemy_PageLoc + x, HIBYTE(wide));    // store as new page location
    a = HIBYTE(wide);
    // and go back
}

//------------------------------------------------------------------------

// Inputs: blooberCarry = carry left behind by the caller's swim-direction subtraction; x = enemy
// object buffer offset
// Outputs: none
void SMBEngine::ProcSwimmingB(bool blooberCarry)
{
    // Floatdown: sink one pixel every other frame, then leave. Two separate places drop into
    // this, one of them from outside the block it used to live in.
    const auto floatdown = [&]()
    {
        // get frame counter and check for d0 set
        if ((M(FrameCounter) & 0x01) == 0)
        {
            ++M(Enemy_Y_Position + x); // otherwise increment vertical coordinate
        } // NoFD: leave
    };

    // get enemy's movement counter and check for d1 set
    if ((M(BlooperMoveCounter + x) & 0b00000010) == 0)
    {
        const uint8_t frameBits = M(FrameCounter) & 0b00000111; // get 3 LSB of frame counter
        // execute the code below only every eighth frame
        if (frameBits != 0)
        {
            return;
        }
        // d0 of the enemy's movement counter picks speeding up from slowing down
        if ((M(BlooperMoveCounter + x) & 0x01) == 0)
        {
            const uint8_t newForce = M(Enemy_Y_MoveForce + x) + 0x01;
            writeData(Enemy_Y_MoveForce + x, newForce); // set movement force
            writeData(BlooperMoveSpeed + x, newForce);  // set as movement speed
            if (newForce != 0x02)
            {
                return; // not yet at the top speed, leave
            }
            ++M(BlooperMoveCounter + x); // otherwise increment movement counter
            return;                      // BSwimE
        }

        // SlowSwim
        const uint8_t newForce = M(Enemy_Y_MoveForce + x) - 0x01;
        writeData(Enemy_Y_MoveForce + x, newForce); // set movement force
        writeData(BlooperMoveSpeed + x, newForce);  // set as movement speed
        if (newForce != 0)
        {
            return; // if any speed left, leave
        }
        ++M(BlooperMoveCounter + x);             // otherwise increment movement counter
        writeData(EnemyIntervalTimer + x, 0x02); // set enemy's timer
        return;                                  // NoSSw: leave
    }

    // ChkForFloatdown: get enemy timer, branch if expired
    if (M(EnemyIntervalTimer + x) != 0)
    {
        floatdown();
        return;
    }

    // ChkNearPlayer: get vertical coordinate, add sixteen pixels, plus whatever carry the swim
    // code left behind
    const uint8_t modifiedYPos = static_cast<uint8_t>(M(Enemy_Y_Position + x) + 0x10 + (blooberCarry ? 1 : 0));
    if (modifiedYPos < M(Player_Y_Position))
    {
        floatdown(); // modified vertical less than player's
        return;
    }
    writeData(BlooperMoveCounter + x, 0x00); // otherwise nullify movement counter
}

//------------------------------------------------------------------------

// Inputs: a = high byte of the firebar's spinstate (reads the firebar's index from zero-page
// 0x00, not a register)
// Outputs: none (results are communicated via zero-page 0x01-0x03: horizontal adder, vertical
// adder, and mirroring data)
void SMBEngine::GetFirebarPosition()
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

    pha();           // save high byte of spinstate to the stack
    a &= 0b00001111; // mask out low nybble
    if (a >= 0x09)
    {                    // if lower than $09, branch ahead
        a ^= 0b00001111; // otherwise get two's compliment to oscillate
        a += 0x01;
    } // GetHAdder: store result, modified or not, here
    writeData(0x01, a);
    y = M(0x00);                  // load number of firebar ball where we're at
    a = M(FirebarTblOffsets + y); // load offset to firebar position data
    a += M(0x01);                 // add oscillated high byte of spinstate
    y = a;                        // to offset here and use as new offset
    // get data here and store as horizontal adder
    writeData(0x01, FirebarPosLookupTbl_data[y]);
    pla();           // pull whatever was in A from the stack
    pha();           // save it again because we still need it
    a += 0x08;       // add eight this time, to get vertical adder
    a &= 0b00001111; // mask out high nybble
    if (a >= 0x09)
    {
        a ^= 0b00001111; // otherwise get two's compliment
        a += 0x01;
    } // GetVAdder: store result here
    writeData(0x02, a);
    y = M(0x00);
    a = M(FirebarTblOffsets + y); // load offset to firebar position data again
    a += M(0x02);                 // this time add value in $02 to offset here and use as offset
    y = a;
    // get data here and store as vertica adder
    writeData(0x02, FirebarPosLookupTbl_data[y]);
    pla();   // pull out whatever was in A one last time
    a >>= 1; // divide by eight or shift three to the right
    a >>= 1;
    a >>= 1;
    y = a;                        // use as offset
    a = M(FirebarMirrorData + y); // load mirroring data here
    writeData(0x03, a);           // store
}

//------------------------------------------------------------------------

// Inputs: a = spinning speed to apply this frame; x = enemy object buffer offset (selects spin
// direction and spin state)
// Outputs: a = new (unmasked) high byte of spin state; the sole caller masks it before storing
void SMBEngine::FirebarSpin()
{
    uint32_t wide = 0;

    writeData(0x07, a); // save spinning speed here
    // check spinning direction
    if (M(FirebarSpinDirection + x) == 0)
    {             // if moving counter-clockwise, branch to other part
        y = 0x18; // possibly residual ldy
        wide = ((M(FirebarSpinState_High + x) << 8) | M(FirebarSpinState_Low + x)) +
               M(0x07); // add spinning speed to what would normally be the horizontal speed
        writeData(FirebarSpinState_Low + x, LOBYTE(wide));
        a = HIBYTE(wide); // what would normally be the vertical speed, never stored back
        return;

        //------------------------------------------------------------------------
    } // SpinCounterClockwise
    y = 0x08; // possibly residual ldy
    wide = ((M(FirebarSpinState_High + x) << 8) | M(FirebarSpinState_Low + x)) -
           M(0x07); // subtract spinning speed from what would normally be the horizontal speed
    writeData(FirebarSpinState_Low + x, LOBYTE(wide));
    a = HIBYTE(wide); // what would normally be the vertical speed, never stored back
}

//------------------------------------------------------------------------

// Inputs: a = vertical speed of the platform, consumed from the stack (the caller pushes it
// twice: one copy is pulled here, the other is left for the caller to pull later); y = the
// other platform's enemy offset
// Outputs: none (results are communicated via zero-page 0x00-0x02, the name table address to
// write)
void SMBEngine::SetupPlatformRope()
{
    uint32_t wide = 0;

    pha();                                 // save second/third copy to stack
    wide = M(Enemy_X_Position + y) + 0x08; // get horizontal coordinate, add eight pixels
    a = LOBYTE(wide);
    // if secondary hard mode flag set,
    if (M(SecondaryHardMode) == 0)
    {                    // use coordinate as-is
        wide = a + 0x10; // otherwise add sixteen more pixels, dropping the carry from the eight
        a = LOBYTE(wide);
    } // GetLRp: save modified horizontal coordinate to stack
    pha();
    a = (uint8_t)(M(Enemy_PageLoc + y) + HIBYTE(wide)); // add carry to page location
    writeData(0x02, a);                                 // and save here
    pla();                                              // pull modified horizontal coordinate
    a &= 0b11110000;                                    // from the stack, mask out low nybble
    a >>= 1;                                            // and shift three bits to the right
    a >>= 1;
    a >>= 1;
    writeData(0x00, a);          // store result here as part of name table low byte
    x = M(Enemy_Y_Position + y); // get vertical coordinate
    pla();                       // get second/third copy of vertical speed from stack
    if ((a & 0x80) != 0)
    { // skip this part if moving downwards or not at all
        a = x;
        a += 0x08; // add eight to vertical coordinate and
        x = a;     // save as X
    } // GetHRp: move vertical coordinate to A
    a = x;
    x = M(VRAM_Buffer1_Offset);         // get vram buffer offset
    wide = a >> 6;                      // keep d7 and d6 of the vertical coordinate aside
    a = (uint8_t)((a << 2) | (a >> 7)); // rotate d7 round to d0
    pha();                              // save modified vertical coordinate to stack
    a = (uint8_t)(wide | 0b00100000);   // with d7 and d6 at the 2 LSB, set d5 to get
    writeData(0x01, a);                 // the appropriate high byte of name table address, then store
    // get saved page location from earlier
    a = M(0x02) & 0x01; // mask out all but LSB
    a <<= 1;
    a <<= 1;            // shift twice to the left and save with the
    a |= M(0x01);       // rest of the bits of the high byte, to get
    writeData(0x01, a); // the proper name table and the right place on it
    pla();              // get modified vertical coordinate from stack
    a &= 0b11100000;    // mask out low nybble and LSB of high nybble
    a += M(0x00);       // add to horizontal part saved here
    writeData(0x00, a); // save as name table low byte
    a = M(Enemy_Y_Position + y);
    if (a >= 0xe8)
    {                             // bottom of the screen, we're done, branch to leave
        a = M(0x00) & 0b10111111; // mask out d6 of low byte of name table address
        writeData(0x00, a);
    } // ExPRp: leave!
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: return value = whether Enemy_Y_Position + 62 is in range ($44 or more)
bool SMBEngine::SubtEnemyYPos()
{
    bool enemyYPosInRange = false;

    a = M(Enemy_Y_Position + x); // add 62 pixels to enemy object's
    a += 0x3e;
    enemyYPosInRange = a >= 0x44; // compare against a certain range
    return enemyYPosInRange;      // and leave with the result for a conditional branch
}

//------------------------------------------------------------------------

// Inputs: a = base X or Y coordinate for the first sprite (each subsequent sprite adds 8
// pixels); y = starting OAM data offset, which the caller must also stash in zero-page 0x02
// Outputs: y = restored from zero-page 0x02 (same offset as on entry); a is scratch
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
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (lakitu's slot)
// Outputs: a = adjusted movement speed, used by the caller to set LakituMoveSpeed
void SMBEngine::PlayerLakituDiff()
{
    bool enemyRightOfPlayer = false;

    std::tie(enemyRightOfPlayer, a) = PlayerEnemyDiff(x); // get horizontal difference between enemy and player
    // 0 means lakitu should move right (towards a player on its right), 1 means left
    uint8_t lakituDir = 0x00;
    if ((a & 0x80) != 0)
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
        if (M(Enemy_ID + x) == Lakitu && lakituDir != M(LakituMoveDirection + x))
        {
            // if moving to the left beyond maximum distance, alter without delay
            if (M(LakituMoveDirection + x) != 0)
            {
                --M(LakituMoveSpeed + x); // decrement horizontal speed
                a = M(LakituMoveSpeed + x);
                if (a != 0)
                {
                    // Horizontal speed not yet at zero, leave. The decremented speed stays in
                    // the accumulator: this is the value the caller writes back.
                    return;
                }
            }
            // SetLMovD: set horizontal direction depending on horizontal difference between
            // enemy and player if necessary
            writeData(LakituMoveDirection + x, lakituDir);
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
        const bool spinyWithPlayerMoving = (M(Enemy_ID + x) == Spiny) && (M(Player_X_Speed) != 0);
        // ChkEmySpd: anything else with no vertical speed goes back to the first value
        if (!spinyWithPlayerMoving && M(Enemy_Y_Speed + x) == 0)
        {
            valueIndex = 0x00;
        }
    }

    // SubDifAdj: get one of three saved values from earlier
    y = valueIndex;
    a = M(0x0001 + y);
    y = M(0x00); // get saved horizontal difference

    do // SPixelLak: subtract one for each pixel of horizontal difference
    {
        a -= 0x01; // from one of three saved values
        --y;
    } while ((y & 0x80) == 0); // branch until all pixels are subtracted, to adjust difference

    // ExMoveLak: leave!!!
}

//------------------------------------------------------------------------

// Inputs: y = which vine segment to draw, index into VineYPosAdder_data/VineObjOffset
// Outputs: y = restored to its input value (reused internally as scratch, saved to zero-page
// 0x00 and reloaded before return)
void SMBEngine::DrawVine()
{
    const uint8_t VineYPosAdder_data[] = {0x00, 0x30};

    writeData(0x00, y);                  // save offset here
    a = M(Enemy_Rel_YPos);               // get relative vertical coordinate
    a += VineYPosAdder_data[y];          // add value using offset in Y to get value
    x = M(VineObjOffset + y);            // get offset to vine
    y = M(Enemy_SprDataOffset + x);      // get sprite data offset
    writeData(0x02, y);                  // store sprite data offset here
    SixSpriteStacker();                  // stack six sprites on top of each other vertically
    a = M(Enemy_Rel_XPos);               // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, a); // store in first, third and fifth sprites
    writeData(Sprite_X_Position + 8 + y, a);
    writeData(Sprite_X_Position + 16 + y, a);
    a += 0x06;                                // add six pixels to second, fourth and sixth sprites
    writeData(Sprite_X_Position + 4 + y, a);  // to give characteristic staggered vine shape to
    writeData(Sprite_X_Position + 12 + y, a); // our vertical stack of sprites
    writeData(Sprite_X_Position + 20 + y, a);
    // set bg priority and palette attribute bits
    writeData(Sprite_Attributes + y, 0b00100001); // set in first, third and fifth sprites
    writeData(Sprite_Attributes + 8 + y, 0b00100001);
    writeData(Sprite_Attributes + 16 + y, 0b00100001);
    a = 0b01100001;                                   // additionally, set horizontal flip bit
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
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none (a is scratch, mirrors the writeData call)
void SMBEngine::InitRetainerObj()
{
    a = 0xb8;                              // set fixed vertical position for
    writeData(Enemy_Y_Position + x, 0xb8); // princess/mushroom retainer object
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitBulletBill()
{
    // set moving direction for left
    writeData(Enemy_MovingDir + x, 0x02);
    a = 0x09; // set bounding box control for $09
    writeData(Enemy_BoundBoxCtrl + x, 0x09);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (the fireworks object's slot)
// Outputs: none
void SMBEngine::InitFireworks()
{
    const uint8_t FireworksYPosData_data[] = {0x60, 0x40, 0x70, 0x40, 0x60, 0x30};

    const uint8_t FireworksXPosData_data[] = {0x00, 0x30, 0x60, 0x60, 0x00, 0x20};

    uint32_t wide = 0;

    a = M(FrenzyEnemyTimer); // if timer not expired yet, branch to leave
    if (a == 0)
    {
        a = 0x20; // otherwise reset timer
        writeData(FrenzyEnemyTimer, 0x20);
        --M(FireworksCounter); // decrement for each explosion
        y = 0x06;              // start at last slot

        do // StarFChk
        {
            --y;
            // check for presence of star flag object
        } while (M(Enemy_ID + y) != StarFlagObject); // routine goes into infinite loop = crash
        wide = ((M(Enemy_PageLoc + y) << 8) | M(Enemy_X_Position + y)) // get horizontal coordinate of star flag object, then
               - 0x30;                                                 // subtract 48 pixels from it
        a = LOBYTE(wide);
        pha(); // and save to the stack
        a = HIBYTE(wide);
        writeData(0x00, a);                                      // the page location of the star flag object, less the borrow
        a = M(FireworksCounter);                                 // get fireworks counter
        a += M(Enemy_State + y);                                 // add state of star flag object (possibly not necessary)
        y = a;                                                   // use as offset
        pla();                                                   // get saved horizontal coordinate of star flag - 48 pixels
        wide = ((M(0x00) << 8) | a) + FireworksXPosData_data[y]; // add number based on offset of fireworks counter
        writeData(Enemy_X_Position + x, LOBYTE(wide));           // store as the fireworks object horizontal coordinate
        writeData(Enemy_PageLoc + x, HIBYTE(wide));              // the fireworks object
        a = HIBYTE(wide);
        // get vertical position using same offset
        writeData(Enemy_Y_Position + x, FireworksYPosData_data[y]); // and store as vertical coordinate for fireworks object
        writeData(Enemy_Y_HighPos + x, 0x01);                       // store in vertical high byte
        writeData(Enemy_Flag + x, 0x01);                            // and activate enemy buffer flag
        writeData(ExplosionGfxCounter + x, 0x00);                   // initialize explosion counter
        a = 0x08;
        writeData(ExplosionTimerCounter + x, 0x08); // set explosion timing counter
    } // ExitFWk
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (the object being disabled)
// Outputs: none
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
    writeData(Enemy_Flag + x, 0x00);    // disable enemy buffer flag for this object
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MovePiranhaPlant()
{
    // Every one of the six ways this can bail out lands on the same tail (PutinPipe), so the
    // movement is a lambda that returns and the tail runs unconditionally afterwards.
    const auto movePlant = [&]()
    {
        // check enemy state; leave if set at all
        if (M(Enemy_State + x) != 0)
        {
            return;
        }
        // check enemy's timer here; leave if not yet expired
        if (M(EnemyFrameTimer + x) != 0)
        {
            return;
        }
        // check movement flag
        if (M(PiranhaPlant_MoveFlag + x) == 0)
        { // if moving, skip to part ahead
            // if currently rising, branch
            if ((M(PiranhaPlant_Y_Speed + x) & 0x80) == 0)
            { // to move enemy upwards out of pipe
                // get horizontal difference between player and piranha plant
                bool enemyRightOfPlayer = false;
                std::tie(enemyRightOfPlayer, a) = PlayerEnemyDiff(x);
                if ((a & 0x80) != 0)
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
            writeData(PiranhaPlant_Y_Speed + x, (M(PiranhaPlant_Y_Speed + x) ^ 0xff) + 0x01);
            ++M(PiranhaPlant_MoveFlag + x); // increment to set movement flag
        } // SetupToMovePPlant
        // head for the highest point when rising (negative speed), the lowest point otherwise
        const bool movingUp = (M(PiranhaPlant_Y_Speed + x) & 0x80) != 0;
        const uint8_t targetYPos = movingUp ? M(PiranhaPlantUpYPos + x) : M(PiranhaPlantDownYPos + x);
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
        const uint8_t newYPos = M(Enemy_Y_Position + x) + M(PiranhaPlant_Y_Speed + x);
        writeData(Enemy_Y_Position + x, newYPos); // save as new vertical coordinate
        if (newYPos != M(0x00))
        {
            return; // leave if the target coordinate is not yet reached
        }
        writeData(PiranhaPlant_MoveFlag + x, 0x00); // otherwise clear movement flag
        writeData(EnemyFrameTimer + x, 0x40);       // set timer to delay piranha plant movement
    };
    movePlant();

    // PutinPipe: set background priority bit in sprite attributes to give illusion of being
    // inside pipe, then leave
    writeData(Enemy_SprAttrib + x, 0b00100000);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitJumpGPTroopa()
{
    // set for movement to the left
    writeData(Enemy_MovingDir + x, 0x02);
    a = 0xf8; // set horizontal speed
    writeData(Enemy_X_Speed + x, 0xf8);
    TallBBox2();
}

//------------------------------------------------------------------------

// set specific value for bounding box control
// Inputs: x = enemy object buffer offset (forwarded to SetBBox2)
// Outputs: none
void SMBEngine::TallBBox2()
{
    a = 0x03;
    SetBBox2(a, x);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitGoomba()
{
    InitNormalEnemy(); // set appropriate horizontal speed
    SmallBBox();       // set $09 as bounding box control, set other values
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitPodoboo()
{
    // set enemy position to below
    writeData(Enemy_Y_HighPos + x, 0x02); // the bottom of the screen
    writeData(Enemy_Y_Position + x, 0x02);
    writeData(EnemyIntervalTimer + x, 0x01); // set timer for enemy
    a = 0x00;
    writeData(Enemy_State + x, 0x00); // initialize enemy state, then jump to use
    SmallBBox();                      // $09 as bounding box size and set other things
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (not read directly here, only forwarded to SetESpd)
// Outputs: none
void SMBEngine::InitNormalEnemy()
{
    const uint8_t NormalXSpdData_data[] = {0xf8, 0xf4};

    y = 0x01; // load offset of 1 by default
    // check for primary hard mode flag set
    if (M(PrimaryHardMode) == 0)
    {
        y = 0x00; // if not set, decrement offset
    } // GetESpd: get appropriate horizontal speed
    a = NormalXSpdData_data[y];
    SetESpd();
}

//------------------------------------------------------------------------

// store as speed for enemy object
// Inputs: a = horizontal speed to store; x = enemy object buffer offset
// Outputs: none
void SMBEngine::SetESpd()
{
    writeData(Enemy_X_Speed + x, a);
    TallBBox(); // branch to set bounding box control and other data
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitRedKoopa()
{
    InitNormalEnemy(); // load appropriate horizontal speed
    a = 0x01;          // set enemy state for red koopa troopa $03
    writeData(Enemy_State + x, 0x01);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitHammerBro()
{
    const uint8_t HBroWalkingTimerData_data[] = {0x80, 0x50};

    // init horizontal speed and timer used by hammer bro
    writeData(HammerThrowingTimer + x, 0x00); // apparently to time hammer throwing
    writeData(Enemy_X_Speed + x, 0x00);
    y = M(SecondaryHardMode);                                        // get secondary hard mode flag
    writeData(EnemyIntervalTimer + x, HBroWalkingTimerData_data[y]); // set value as delay for hammer bro to walk left
    a = 0x0b;                                                        // set specific value for bounding box size control
    SetBBox();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitHorizFlySwimEnemy()
{
    a = 0x00; // initialize horizontal speed
    SetESpd();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitBloober()
{
    a = 0x00; // initialize horizontal speed
    writeData(BlooperMoveSpeed + x, 0x00);
    SmallBBox();
}

//------------------------------------------------------------------------

// set specific bounding box size control
// Inputs: x = enemy object buffer offset (forwarded to SetBBox)
// Outputs: none
void SMBEngine::SmallBBox()
{
    a = 0x09;
    SetBBox(); // unconditional branch
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitRedPTroopa()
{
    y = 0x30;                             // load central position adder for 48 pixels down
    a = M(Enemy_Y_Position + x);          // set vertical coordinate into location to
    writeData(RedPTroopaOrigXPos + x, a); // be used as original vertical coordinate
    if ((a & 0x80) != 0)
    {             // if vertical coordinate < $80
        y = 0xe0; // if => $80, load position adder for 32 pixels up
    } // GetCent: send central position adder to A
    a = y;
    a += M(Enemy_Y_Position + x);           // add to current vertical coordinate
    writeData(RedPTroopaCenterYPos + x, a); // store as central vertical coordinate
    TallBBox();
}

//------------------------------------------------------------------------

// set specific bounding box size control
// Inputs: x = enemy object buffer offset (forwarded to SetBBox)
// Outputs: none
void SMBEngine::TallBBox()
{
    a = 0x03;
    SetBBox();
}

//------------------------------------------------------------------------

// set bounding box control here
// Inputs: a = bounding box control value; x = enemy object buffer offset
// Outputs: none
void SMBEngine::SetBBox()
{
    writeData(Enemy_BoundBoxCtrl + x, a);
    a = 0x02; // set moving direction for left
    writeData(Enemy_MovingDir + x, 0x02);
    InitVStf(x);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitCheepCheep()
{
    SmallBBox(); // set vertical bounding box, speed, init others
    // check one portion of LSFR
    a = M(PseudoRandomBitReg + x) & 0b00010000; // get d4 from it
    writeData(CheepCheepMoveMFlag + x, a);      // save as movement flag of some sort
    a = M(Enemy_Y_Position + x);
    writeData(CheepCheepOrigYPos + x, a); // save original vertical coordinate here
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::EnemyLanding()
{
    InitVStf(x);                              // do something here to vertical speed and something else
    a = M(Enemy_Y_Position + x) & 0b11110000; // save high nybble of vertical coordinate, and
    a |= 0b00001000;                          // set d3, then store, probably used to set enemy object
    writeData(Enemy_Y_Position + x, a);       // neatly on whatever it's landing on
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitHoriPlatform()
{
    a = 0x00;
    writeData(XMoveSecondaryCounter + x, 0x00); // init one of the moving counters
    CommonPlatCode();                           // jump ahead to execute more code
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitVertPlatform()
{
    y = 0x40;                    // set default value here
    a = M(Enemy_Y_Position + x); // check vertical position
    if ((a & 0x80) != 0)
    { // if above a certain point, skip this part
        a ^= 0xff;
        a += 0x01;
        y = 0xc0; // get alternate value to add to vertical position
    } // SetYO: save as top vertical position
    writeData(YPlatformTopYPos + x, a);
    a = y;
    a += M(Enemy_Y_Position + x);          // to vertical position
    writeData(YPlatformCenterYPos + x, a); // save result as central vertical position
    CommonPlatCode();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::CommonPlatCode()
{
    InitVStf(x); // do a sub to init certain other values
    SPBBox();
}

//------------------------------------------------------------------------

// set default bounding box size control
// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::SPBBox()
{
    // use the default value in castles or in secondary hard mode, the alternate value otherwise
    const bool useDefault = (M(AreaType) == 0x03) || (M(SecondaryHardMode) != 0);

    // CasPBB: set bounding box size control here and leave
    writeData(Enemy_BoundBoxCtrl + x, useDefault ? 0x05 : 0x06);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::PlatLiftUp()
{
    // set movement amount here
    writeData(Enemy_Y_MoveForce + x, 0x10);
    a = 0xff; // set moving speed for platforms going up
    writeData(Enemy_Y_Speed + x, 0xff);
    CommonSmallLift(); // skip ahead to part we should be executing
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::PlatLiftDown()
{
    // set movement amount here
    writeData(Enemy_Y_MoveForce + x, 0xf0);
    a = 0x00; // set moving speed for platforms going down
    writeData(Enemy_Y_Speed + x, 0x00);
    CommonSmallLift();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::CommonSmallLift()
{
    y = 0x01;
    PosPlatform(); // do a sub to add 12 pixels due to preset value
    a = 0x04;
    writeData(Enemy_BoundBoxCtrl + x, 0x04); // set bounding box control for small platforms
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (forwarded to MoveRedPTroopa)
// Outputs: none
void SMBEngine::MoveRedPTroopaDown()
{
    y = 0x00;         // set Y to move downwards
    MoveRedPTroopa(); // skip to movement routine
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (of the leading paratroopa; the trailing one is x+1
// once incremented here); y = movement mode forwarded to ImposeGravity (0 = down, 1 = up)
// Outputs: none
void SMBEngine::MoveRedPTroopa()
{
    ++x;                   // increment X for enemy offset
    writeData(0x00, 0x03); // set downward movement amount here
    writeData(0x01, 0x06); // set upward movement amount here
    writeData(0x02, 0x02); // set maximum speed here
    a = y;                 // set movement direction in A, and
    RedPTroopaGrav();      // jump to move this thing
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (forwarded to Skip_7)
// Outputs: none
void SMBEngine::MovePlatformDown()
{
    a = 0x00; // save value to stack (if branching here, execute next
    Skip_7();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (forwarded to Skip_7)
// Outputs: none
void SMBEngine::MovePlatformUp()
{
    a = 0x01; // save value to stack
    Skip_7();
}

//------------------------------------------------------------------------

// Inputs: a = movement direction pushed to the stack by the caller (0 = down, 1 = up); x =
// enemy object buffer offset
// Outputs: none
void SMBEngine::Skip_7()
{
    pha();
    y = M(Enemy_ID + x); // get enemy object identifier
    ++x;                 // increment offset for enemy object
    a = 0x05;            // load default value here
    if (y == 0x29)
    {             // this code, thus unconditional branch here
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
}

//------------------------------------------------------------------------

// Inputs: a = movement mode; x = enemy object buffer offset
// Outputs: none (x is reloaded from ObjectOffset)
void SMBEngine::RedPTroopaGrav()
{
    ImposeGravity(a, x); // do a sub to move object gradually
    x = M(ObjectOffset); // get enemy object offset and leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::SetupLakitu()
{
    a = 0x00; // erase counter for lakitu's reappearance
    writeData(LakituReappearTimer, 0x00);
    InitHorizFlySwimEnemy(); // set $03 as bounding box, set other attributes
    TallBBox2();             // set $03 as bounding box again (not necessary) and leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitShortFirebar()
{
    const uint8_t FirebarSpinDirData_data[] = {0x00, 0x00, 0x10, 0x10, 0x00};

    const uint8_t FirebarSpinSpdData_data[] = {0x28, 0x38, 0x28, 0x38, 0x28};

    uint32_t wide = 0;

    // initialize low byte of spin state
    writeData(FirebarSpinState_Low + x, 0x00);
    a = M(Enemy_ID + x); // subtract $1b from enemy identifier
    a -= 0x1b;
    y = a;
    // get spinning speed of firebar
    writeData(FirebarSpinSpeed + x, FirebarSpinSpdData_data[y]);
    // get spinning direction of firebar
    writeData(FirebarSpinDirection + x, FirebarSpinDirData_data[y]);
    a = M(Enemy_Y_Position + x);
    a += 0x04;
    writeData(Enemy_Y_Position + x, a);
    wide = ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x)) + 0x04;
    writeData(Enemy_X_Position + x, LOBYTE(wide));
    writeData(Enemy_PageLoc + x, HIBYTE(wide));
    a = HIBYTE(wide);
    TallBBox2(); // set bounding box control (not used) and leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitBalPlatform()
{
    --M(Enemy_Y_Position + x); // raise vertical position by two pixels
    --M(Enemy_Y_Position + x);
    // if secondary hard mode flag not set,
    if (M(SecondaryHardMode) == 0)
    {                  // branch ahead
        y = 0x02;      // otherwise set value here
        PosPlatform(); // do a sub to add or subtract pixels
    } // AlignP: set default value here for now
    y = 0xff;
    a = M(BalPlatformAlignment);   // get current balance platform alignment
    writeData(Enemy_State + x, a); // set platform alignment to object state here
    if ((a & 0x80) != 0)
    {          // if old alignment $ff, put $ff as alignment for negative
        a = x; // if old contents already $ff, put
        y = a; // object offset as alignment to make next positive
    } // SetBPA: store whatever value's in Y here
    writeData(BalPlatformAlignment, y);
    a = 0x00;
    writeData(Enemy_MovingDir + x, 0x00); // init moving direction
    y = 0x00;                             // init Y
    PosPlatform();                        // do a sub to add 8 pixels, then run shared code here

    InitDropPlatform();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitDropPlatform()
{
    a = 0xff;
    writeData(PlatformCollisionFlag + x, 0xff); // set some value here
    CommonPlatCode();                           // then jump ahead to execute more code
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::LargeLiftUp()
{
    PlatLiftUp();    // execute code for platforms going up
    LargeLiftBBox(); // overwrite bounding box for large platforms
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::LargeLiftDown()
{
    PlatLiftDown(); // execute code for platforms going down

    LargeLiftBBox();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::LargeLiftBBox()
{
    SPBBox(); // jump to overwrite bounding box size control
}

//------------------------------------------------------------------------

// Inputs: none (reads CurrentPlayer from memory)
// Outputs: none
void SMBEngine::EndAreaPoints()
{
    y = 0x0b; // load offset for mario's score by default
    // check player on the screen
    if (M(CurrentPlayer) != 0)
    {             // if mario, do not change
        y = 0x11; // otherwise load offset for luigi's score
    } // ELPGive: award 50 points per game timer interval
    DigitsMathRoutine(y);
    a = M(CurrentPlayer); // get player on the screen (or 500 points per
    a <<= 1;              // fireworks explosion if branched here from there)
    a <<= 1;              // shift to high nybble
    a <<= 1;
    a <<= 1;
    a |= 0b00000100; // add four to set nybble for game timer
    UpdateNumber(a); // jump to print the new score and game timer
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (star flag's slot)
// Outputs: x is reloaded from ObjectOffset (same value as on entry)
void SMBEngine::DrawStarFlag()
{
    const uint8_t StarFlagTileData_data[] = {0x54, 0x55, 0x56, 0x57};

    const uint8_t StarFlagXPosAdder_data[] = {0x00, 0x08, 0x00, 0x08};

    const uint8_t StarFlagYPosAdder_data[] = {0x00, 0x00, 0x08, 0x08};

    RelativeEnemyPosition();        // get relative coordinates of star flag
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    x = 0x03;                       // do four sprites

    do // DSFLoop: get relative vertical coordinate
    {
        a = M(Enemy_Rel_YPos);
        a += StarFlagYPosAdder_data[x];      // add Y coordinate adder data
        writeData(Sprite_Y_Position + y, a); // store as Y coordinate
        // get tile number
        writeData(Sprite_Tilenumber + y, StarFlagTileData_data[x]); // store as tile number
        // set palette and background priority bits
        writeData(Sprite_Attributes + y, 0x22); // store as attributes
        a = M(Enemy_Rel_XPos);                  // get relative horizontal coordinate
        a += StarFlagXPosAdder_data[x];         // add X coordinate adder data
        writeData(Sprite_X_Position + y, a);    // store as X coordinate
        ++y;
        ++y; // increment OAM data offset four bytes
        ++y; // for next sprite
        ++y;
        --x; // move onto next sprite
    } while ((x & 0x80) == 0); // do this until all sprites are done
    x = M(ObjectOffset); // get enemy object offset and leave
}

//------------------------------------------------------------------------

// Inputs: a arrives via the stack (the platform's vertical position from before it was moved,
// pushed by the caller); x = enemy object buffer offset (this platform)
// Outputs: x is reloaded from ObjectOffset (same value as on entry)
void SMBEngine::DoOtherPlatform()
{
    y = M(Enemy_State + x);             // get offset of other platform
    pla();                              // get old vertical coordinate from stack
    a -= M(Enemy_Y_Position + x);       // get difference of old vs. new coordinate
    a += M(Enemy_Y_Position + y);       // add difference to vertical coordinate of other
    writeData(Enemy_Y_Position + y, a); // platform to move it in the opposite direction
    a = M(PlatformCollisionFlag + x);   // if no collision, skip this part here
    if ((a & 0x80) == 0)
    {
        x = a;                   // put offset which collision occurred here
        PositionPlayerOnVPlat(); // and use it to position player accordingly
    } // DrawEraseRope
    y = M(ObjectOffset); // get enemy object offset
    // draw the rope only if the current platform is moving at all and there is room in the
    // vram buffer for the ten bytes it takes; otherwise fall straight through to ExitRp
    const bool platformMoving = (M(Enemy_Y_Speed + y) | M(Enemy_Y_MoveForce + y)) != 0;
    if (platformMoving && M(VRAM_Buffer1_Offset) < 0x20)
    {
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
        {                                          // to do something else
            writeData(VRAM_Buffer1 + 3 + x, 0xa2); // otherwise put tile numbers for left
            a = 0xa3;                              // and right sides of rope in vram buffer
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
        pla();                  // pull second copy of vertical speed from stack
        a ^= 0xff;              // invert bits to reverse speed
        SetupPlatformRope();    // do sub again to figure out where to put bg tiles
        // write name table address to vram buffer
        writeData(VRAM_Buffer1 + 5 + x, M(0x01)); // this time we're doing putting tiles for
        // the other platform
        writeData(VRAM_Buffer1 + 6 + x, M(0x00));
        writeData(VRAM_Buffer1 + 7 + x, 0x02); // set length again for 2 bytes
        pla();                                 // pull first copy of vertical speed from stack
        if ((a & 0x80) != 0)
        {                                          // if moving upwards (note inversion earlier), skip this
            writeData(VRAM_Buffer1 + 8 + x, 0xa2); // otherwise put tile numbers for left
            a = 0xa3;                              // and right sides of rope in vram
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
    }

    // ExitRp: get enemy object buffer offset and leave
    x = M(ObjectOffset);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (this platform); y = other platform's offset (also
// receives the same speed values)
// Outputs: none
void SMBEngine::StopPlatforms()
{
    InitVStf(x);                     // initialize vertical speed and low byte
    writeData(Enemy_Y_Speed + y, a); // for both platforms and leave
    writeData(Enemy_Y_MoveForce + y, a);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::ChkYPCollision()
{
    a = M(PlatformCollisionFlag + x); // if collision flag not set here, branch
    if ((a & 0x80) == 0)
    {                            // to leave
        PositionPlayerOnVPlat(); // otherwise position player appropriately
    } // ExYPl: leave
}

//------------------------------------------------------------------------

// Inputs: a = bounding box counter saved in the collision flag (1 or 2); x = enemy object
// buffer offset
// Outputs: none
void SMBEngine::PositionPlayerOnS_Plat()
{
    const uint8_t PlayerPosSPlatData_data[] = {0x80, 0x00};

    y = a;                               // use bounding box counter saved in collision flag
    a = M(Enemy_Y_Position + x);         // for offset
    a += PlayerPosSPlatData_data[y - 1]; // coordinate
    Skip_8();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::PositionPlayerOnVPlat()
{
    a = M(Enemy_Y_Position + x); // get vertical coordinate
    Skip_8();
}

//------------------------------------------------------------------------

// Inputs: a = vertical coordinate to position the player at; x = enemy object buffer offset
// (used to read Enemy_Y_HighPos)
// Outputs: none
void SMBEngine::Skip_8()
{
    uint32_t wide = 0;

    y = M(GameEngineSubroutine);
    if (y == 0x0b)
    {
        return; // skip all of this
    }
    y = M(Enemy_Y_HighPos + x);
    if (y != 0x01)
    {
        return;
    }
    wide = ((y << 8) | a) - 0x20;               // subtract 32 pixels from the vertical coordinate for the player object's height
    writeData(Player_Y_Position, LOBYTE(wide)); // save as player's new vertical coordinate
    writeData(Player_Y_HighPos, HIBYTE(wide));  // and store as player's new vertical high byte
    a = HIBYTE(wide);
    a = 0x00;
    writeData(Player_Y_Speed, 0x00);     // initialize vertical speed and low byte of force
    writeData(Player_Y_MoveForce, 0x00); // and then leave

    // ExPlPos
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: a = metatile found underneath the enemy (BlockBufferCollision leaves it there even
// though the C++ return value is discarded along the call chain)
void SMBEngine::ChkUnderEnemy()
{
    a = 0x00;               // set flag in A for save vertical coordinate
    y = 0x15;               // set Y to check the bottom middle (8,18) of enemy object
    BlockBufferChk_Enemy(); // hop to it!
}

//------------------------------------------------------------------------

// Inputs: a = coordSelector flag forwarded to BBChk_E (0 = also report Y low nybble, nonzero =
// X); x = enemy object buffer offset; y = corner index forwarded to BBChk_E
// Outputs: a = metatile found (BlockBufferCollision leaves it there even though the C++ return
// value is discarded along the chain); x is reloaded from ObjectOffset, undoing the +1 applied
// here to address the sprite object buffer
void SMBEngine::BlockBufferChk_Enemy()
{
    pha(); // save contents of A to stack
    a = x;
    a += 0x01;
    x = a;
    pla(); // pull A from stack and jump elsewhere
    BBChk_E(a, x, y);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitFlyingCheepCheep()
{
    const uint8_t FlyCCTimerData_data[] = {0x10, 0x60, 0x20, 0x48};

    const uint8_t FlyCCXSpeedData_data[] = {0x0e, 0x05, 0x06, 0x0e, 0x1c, 0x20, 0x10, 0x0c, 0x1e, 0x22, 0x18, 0x14};

    const uint8_t FlyCCXPositionData_data[] = {0x80, 0x30, 0x40, 0x80, 0x30, 0x50, 0x50, 0x70,
                                               0x20, 0x40, 0x80, 0xa0, 0x70, 0x40, 0x90, 0x68};

    uint32_t wide = 0;

    a = M(FrenzyEnemyTimer); // if timer here not expired yet, branch to leave
    if (a != 0)
    {
        return;
    }
    SmallBBox();                                    // jump to set bounding box size $09 and init other values
    a = M(PseudoRandomBitReg + 1 + x) & 0b00000011; // set pseudorandom offset here
    y = a;
    // load timer with pseudorandom offset
    writeData(FrenzyEnemyTimer, FlyCCTimerData_data[y]);
    y = 0x03; // load Y with default value
    a = M(SecondaryHardMode);
    if (a != 0)
    {             // if secondary hard mode flag not set, do not increment Y
        y = 0x04; // otherwise, increment Y to allow as many as four onscreen
    } // MaxCC: store whatever pseudorandom bits are in Y
    writeData(0x00, y);
    if (x >= M(0x00))
    {
        return; // if X => Y, branch to leave
    }
    a = M(PseudoRandomBitReg + x) & 0b00000011; // get last two bits of LSFR, first part
    writeData(0x00, a);                         // and store in two places
    writeData(0x01, a);
    // set vertical speed for cheep-cheep
    writeData(Enemy_Y_Speed + x, 0xfb);
    // GSeed: a seed based on how fast the player is moving; a standing player seeds zero, and a
    // fast one (>= $19) seeds double what a slow one does
    const uint8_t playerSpeed = M(Player_X_Speed); // check player's horizontal speed
    uint8_t speedSeed = 0x00;
    if (playerSpeed != 0)
    {
        speedSeed = (playerSpeed < 0x19) ? 0x04 : 0x08;
    }

    writeData(0x00, speedSeed + M(0x00));           // add to last two bits of LSFR we saved earlier
    a = M(PseudoRandomBitReg + 1 + x) & 0b00000011; // if neither of the last two bits of second LSFR set,
    if (a != 0)
    {                                                   // skip this part and save contents of $00
        a = M(PseudoRandomBitReg + 2 + x) & 0b00001111; // otherwise overwrite with lower nybble of
        writeData(0x00, a);                             // third LSFR part
    } // RSeed
    // add the seed to the last two bits of LSFR we saved in the other place
    const uint8_t speedOffset = speedSeed + M(0x01); // use as pseudorandom offset here
    // get horizontal speed using pseudorandom offset
    writeData(Enemy_X_Speed + x, FlyCCXSpeedData_data[speedOffset]);
    // set to move towards the right
    writeData(Enemy_MovingDir + x, 0x01);

    // A moving player leaves the speed offset in place as the position offset; a standing one
    // replaces it with the first LSFR (or third LSFR lower nybble) and may reverse the speed.
    uint8_t posOffset = speedOffset;
    if (M(Player_X_Speed) == 0)
    {
        posOffset = M(0x00); // get first LSFR or third LSFR lower nybble
        if ((posOffset & 0b00000010) != 0)
        {
            // if d1 set, change horizontal speed direction
            writeData(Enemy_X_Speed + x, (M(Enemy_X_Speed + x) ^ 0xff) + 0x01);
            ++M(Enemy_MovingDir + x); // increment to move towards the left
        }
    }

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
    writeData(Enemy_X_Position + x, LOBYTE(wide)); // save as enemy's horizontal position
    a = HIBYTE(wide);
    // FinCCSt: save as enemy's page location
    writeData(Enemy_PageLoc + x, a);
    writeData(Enemy_Flag + x, 0x01);      // set enemy's buffer flag
    writeData(Enemy_Y_HighPos + x, 0x01); // set enemy's high vertical byte
    a = 0xf8;
    writeData(Enemy_Y_Position + x, 0xf8); // put enemy below the screen, and we are done
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveFlyGreenPTroopa()
{
    XMoveCntr_GreenPTroopa();         // do sub to increment primary and secondary counters
    MoveWithXMCntrs();                // do sub to move green paratroopa accordingly, and horizontally
    y = 0x01;                         // set Y to move green paratroopa down
    a = M(FrameCounter) & 0b00000011; // check frame counter 2 LSB for any bits set
    if (a == 0)
    {                                     // branch to leave if set to move up/down every fourth frame
        a = M(FrameCounter) & 0b01000000; // check frame counter for d6 set
        if (a == 0)
        {             // branch to move green paratroopa down if set
            y = 0xff; // otherwise set Y to move green paratroopa up
        } // YSway: store adder here
        writeData(0x00, y);
        a = M(Enemy_Y_Position + x);
        a += M(0x00); // to give green paratroopa a wavy flight
        writeData(Enemy_Y_Position + x, a);
    } // NoMGPT: leave!
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (forwarded to XMoveCntr_Platform)
// Outputs: none
void SMBEngine::XMoveCntr_GreenPTroopa()
{
    a = 0x13; // load preset maximum value for secondary counter
    XMoveCntr_Platform();
}

//------------------------------------------------------------------------

// Inputs: a = preset maximum value for the secondary movement counter; x = enemy object buffer
// offset
// Outputs: none
void SMBEngine::XMoveCntr_Platform()
{
    writeData(0x01, a); // store value here

    // NoIncXM: leave if not on every fourth frame
    if ((M(FrameCounter) & 0b00000011) != 0)
    {
        return;
    }

    // The secondary counter walks up to the maximum and back down to zero; d0 of the primary
    // counter says which way it is currently going. Reaching either end bumps the primary
    // counter, which reverses the direction for the next run.
    const uint8_t secondary = M(XMoveSecondaryCounter + x); // get secondary counter
    const bool countingDown = (M(XMovePrimaryCounter + x) & 0x01) != 0;
    if (countingDown)
    {
        // DecSeXM
        if (secondary != 0)
        {
            --M(XMoveSecondaryCounter + x); // decrement secondary counter and leave
            return;
        }
    }
    else
    {
        if (secondary != M(0x01))
        {
            ++M(XMoveSecondaryCounter + x); // increment secondary counter and leave
            return;
        }
    }

    // IncPXM: increment primary counter and leave
    ++M(XMovePrimaryCounter + x);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none (the new page location MoveEnemyHorizontally leaves in a is stashed to
// zero-page 0x00 for later use by PositionPlayerOnHPlat)
void SMBEngine::MoveWithXMCntrs()
{
    a = M(XMoveSecondaryCounter + x); // save secondary counter to stack
    pha();
    y = 0x01;                                    // set value here by default
    a = M(XMovePrimaryCounter + x) & 0b00000010; // if d1 of primary counter is
    if (a == 0)
    {                                            // set, branch ahead of this part here
        a = M(XMoveSecondaryCounter + x) ^ 0xff; // otherwise change secondary
        a += 0x01;
        writeData(XMoveSecondaryCounter + x, a);
        y = 0x02; // load alternate value here
    } // XMRight: store as moving direction
    writeData(Enemy_MovingDir + x, y);
    MoveEnemyHorizontally(x);
    writeData(0x00, a);                      // save value obtained from sub here
    pla();                                   // get secondary counter from stack
    writeData(XMoveSecondaryCounter + x, a); // and return to original place
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (star flag's slot)
// Outputs: none
void SMBEngine::RunStarFlagObj()
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
        writeData(Enemy_State + x, flagState); // set whatever state we have in star flag object
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
        if (M(Enemy_Y_Position + x) >= 0x72)
        {
            --M(Enemy_Y_Position + x); // raise star flag by one pixel
            DrawStarFlag();            // and skip this part here
            return;
        }
        // SetoffF: check fireworks counter; anything left to go off is queued up
        const uint8_t fireworksCounter = M(FireworksCounter);
        if (fireworksCounter != 0 && (fireworksCounter & 0x80) == 0)
        {
            writeData(EnemyFrenzyBuffer, Fireworks); // set fireworks object in frenzy queue
            DrawStarFlag();
            return;
        }
        // DrawFlagSetTimer
        DrawStarFlag();                          // do sub to draw star flag
        writeData(EnemyIntervalTimer + x, 0x06); // set interval timer here
        incrementTask();                         // move onto next task
        return;
    }

    case 4:
        // DelayToAreaEnd
        DrawStarFlag(); // do sub to draw star flag
        // the interval timer set in the previous task must have expired, and the event music
        // buffer must be empty, before the next task
        if (M(EnemyIntervalTimer + x) == 0 && M(EventMusicBuffer) == 0)
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

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::YMovingPlatform()
{
    // if the platform is not moving up or down and has not yet reached its top position, drift
    // it down a pixel every eighth frame; otherwise fall through to ChkYCenterPos
    const bool platformMoving = (M(Enemy_Y_Speed + x) | M(Enemy_Y_MoveForce + x)) != 0;
    if (!platformMoving)
    {
        writeData(Enemy_YMF_Dummy + x, 0x00); // initialize dummy variable
        if (M(Enemy_Y_Position + x) < M(YPlatformTopYPos + x))
        {
            if ((M(FrameCounter) & 0b00000111) == 0)
            {
                ++M(Enemy_Y_Position + x); // increase vertical position every eighth frame
            } // SkipIY: skip ahead to last part
            ChkYPCollision();
            return;
        }
    }

    // ChkYCenterPos
    // if current vertical position < central position, branch
    if (M(Enemy_Y_Position + x) >= M(YPlatformCenterYPos + x))
    {
        MovePlatformUp(); // otherwise start slowing descent/moving upwards
        ChkYPCollision();
        return;
    } // YMDown: start slowing ascent/moving downwards
    MovePlatformDown();

    ChkYPCollision();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveLargeLiftPlat()
{
    MoveLiftPlatforms(); // execute common to all large and small lift platforms
    ChkYPCollision();    // branch to position player correctly
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveSmallPlatform()
{
    MoveLiftPlatforms();     // execute common to all large and small lift platforms
    ChkSmallPlatCollision(); // branch to position player correctly
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: a is left holding the new high byte of position, but no caller relies on it (scratch)
void SMBEngine::MoveLiftPlatforms()
{
    uint32_t wide = 0;

    a = M(TimerControl); // if master timer control set, skip all of this
    if (a != 0)
    {
        return; // and branch to leave
    }
    // position:dummy and speed:force are each one 16-bit quantity
    wide = ((M(Enemy_Y_Position + x) << 8) | M(Enemy_YMF_Dummy + x)) +
           ((M(Enemy_Y_Speed + x) << 8) | M(Enemy_Y_MoveForce + x)); // move up or down
    writeData(Enemy_YMF_Dummy + x, LOBYTE(wide));
    writeData(Enemy_Y_Position + x, HIBYTE(wide)); // and then leave
    a = HIBYTE(wide);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::ChkSmallPlatCollision()
{
    a = M(PlatformCollisionFlag + x); // get bounding box counter saved in collision flag
    if (a == 0)
    {
        return; // if none found, leave player position alone
    }
    PositionPlayerOnS_Plat(); // use to position player correctly

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
    const auto findCollision = [&]() -> bool
    {
        if (M(TimerControl) != 0)
        {
            return false; // master timer control set, leave
        }
        writeData(PlatformCollisionFlag + x, 0x00); // otherwise initialize collision flag
        // leave if the player is below a certain point, or entirely offscreen
        if (CheckPlayerVertical())
        {
            return false;
        }
        writeData(0x00, 0x02); // load counter here for 2 bounding boxes

        do // ChkSmallPlatLoop
        {
            x = M(ObjectOffset);   // get enemy object offset
            GetEnemyBoundBoxOfs(); // get bounding box offset in Y
            if ((a & 0b00000010) != 0)
            {
                return false; // d1 of offscreen lower nybble bits was set, leave
            }
            // check top of platform's bounding box for being in range
            if (M(BoundingBox_UL_YPos + y) >= 0x20)
            {
                // perform player-to-platform collision detection
                if (PlayerCollisionCore(y))
                {
                    return true;
                }
            }
            // MoveBoundBox: move bounding box vertical coordinates
            writeData(BoundingBox_UL_YPos + y, M(BoundingBox_UL_YPos + y) + 0x80);
            writeData(BoundingBox_DR_YPos + y, M(BoundingBox_DR_YPos + y) + 0x80);
            --M(0x00); // decrement counter we set earlier
        } while (M(0x00) != 0); // loop back until both bounding boxes are checked
        return false;
    };
    const bool collisionFound = findCollision();

    // ExSPC: get enemy object buffer offset, then leave
    x = M(ObjectOffset);
    if (collisionFound)
    {
        // ProcSPlatCollisions
        ProcLPlatCollisions();
    }
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (the platform, or the other object in a collision
// pair); y = the platform's bounding box offset, set by an earlier GetEnemyBoundBoxOfs* call
// Outputs: none
void SMBEngine::ProcLPlatCollisions()
{
    // get difference by subtracting the top of the platform's bounding box; a player close
    // underneath it and moving upwards has its jump killed
    const uint8_t platformTopDiff = static_cast<uint8_t>(M(BoundingBox_DR_YPos + y) - M(BoundingBox_UL_YPos));
    if (platformTopDiff < 0x04 && (M(Player_Y_Speed) & 0x80) != 0)
    {
        writeData(Player_Y_Speed, 0x01); // set vertical speed of player to kill jump
    }

    // ChkForTopCollision: get difference by subtracting the top of the player's bounding box;
    // close enough with the player not moving upwards means it landed on the platform
    const uint8_t playerTopDiff = static_cast<uint8_t>(M(BoundingBox_DR_YPos) - M(BoundingBox_UL_YPos + y));
    const bool landedOnPlatform = (playerTopDiff < 0x06) && ((M(Player_Y_Speed) & 0x80) == 0);
    if (landedOnPlatform)
    {
        // the two large lift IDs record the saved bounding box counter, everything else records
        // the enemy object buffer offset
        const uint8_t enemyId = M(Enemy_ID + x);
        const bool usesBoundBoxCounter = (enemyId == 0x2b) || (enemyId == 0x2c);
        const uint8_t collisionFlag = usesBoundBoxCounter ? M(0x00) : x;

        // SetCollisionFlag
        x = M(ObjectOffset);                                 // get enemy object buffer offset
        writeData(PlatformCollisionFlag + x, collisionFlag); // save either bounding box counter or enemy offset here
        writeData(Player_State, 0x00);                       // set player state to normal then leave
        return;
    }

    // PlatformSideCollisions: set value here to indicate possible horizontal collision on the
    // left side of the platform
    writeData(0x00, 0x01);
    // get difference by subtracting platform's left edge
    const uint8_t leftEdgeDiff = static_cast<uint8_t>(M(BoundingBox_DR_XPos) - M(BoundingBox_UL_XPos + y));
    bool sideCollision = true;
    if (leftEdgeDiff >= 0x08)
    {
        ++M(0x00); // otherwise increment value set here for right side collision
        // get difference by subtracting player's left edge from platform's right edge
        // the original clears the carry rather than setting it here, so the
        // subtraction takes one pixel more than it means to
        const uint8_t rightEdgeDiff = static_cast<uint8_t>(M(BoundingBox_DR_XPos + y) - M(BoundingBox_UL_XPos) - 1);
        sideCollision = rightEdgeDiff < 0x09; // too far away is no collision at all
    }
    if (sideCollision)
    {
        // SideC: deal with horizontal collision
        ImpedePlayerMove();
    }

    // NoSideC: return with enemy object buffer offset
    x = M(ObjectOffset);
}

//------------------------------------------------------------------------

// otherwise increment two bytes
// Inputs: none
// Outputs: x is reloaded from ObjectOffset
void SMBEngine::Inc2B()
{
    ++M(EnemyDataOffset);
    ++M(EnemyDataOffset);
    a = 0x00; // init page select for enemy objects
    writeData(EnemyObjectPageSel, 0x00);
    x = M(ObjectOffset); // reload current offset in enemy buffers
    // and leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::WarpZoneObject()
{
    a = M(ScrollLock); // check for scroll lock flag
    if (a == 0)
    {
        return; // branch if not set to leave
    }
    // check to see if player's vertical coordinate has
    a = M(Player_Y_Position) & M(Player_Y_HighPos); // same bits set as in vertical high byte (why?)
    if (a != 0)
    {
        return; // if so, branch to leave
    }
    writeData(ScrollLock, a); // otherwise nullify scroll lock flag
    ++M(WarpZoneControl);     // increment warp zone flag to make warp pipes for warp zone
    EraseEnemyObject(x);      // kill this object
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
        RelativeEnemyPosition(); // get relative coordinates of vine,
        GetEnemyOffscreenBits(); // and any offscreen bits
        y = 0x00;                // initialize offset used in draw vine sub

        do // VDrawLoop: draw vine
        {
            DrawVine();
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

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitLakitu()
{
    // check to see if an enemy is already in
    if (M(EnemyFrenzyBuffer) == 0)
    { // the frenzy buffer, and branch to kill lakitu if so

        SetupLakitu();
        return;
    } // KillLakitu
    EraseEnemyObject(x);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: x is reloaded from ObjectOffset
void SMBEngine::DrawSmallPlatform()
{
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    a = 0x5b;                       // load tile number for small platforms
    ++y;                            // increment offset for tile numbers
    DumpSixSpr(a, y);               // dump tile number into all six sprites
    ++y;                            // increment offset for attributes
    a = 0x02;                       // load palette controls
    DumpSixSpr(a, y);               // dump attributes into all six sprites
    --y;                            // decrement for original offset
    --y;
    a = M(Enemy_Rel_XPos); // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, a);
    writeData(Sprite_X_Position + 12 + y, a); // dump as X coordinate into first and fourth sprites
    a += 0x08;                                // add eight pixels
    writeData(Sprite_X_Position + 4 + y, a);  // dump into second and fifth sprites
    writeData(Sprite_X_Position + 16 + y, a);
    a += 0x08;                               // add eight more pixels
    writeData(Sprite_X_Position + 8 + y, a); // dump into third and sixth sprites
    writeData(Sprite_X_Position + 20 + y, a);
    a = M(Enemy_Y_Position + x); // get vertical coordinate
    x = a;
    pha(); // save to stack
    if (x < 0x20)
    {             // do not mess with it
        a = 0xf8; // otherwise move first three sprites offscreen
    } // TopSP: dump vertical coordinate into Y coordinates
    DumpThreeSpr(a, y);
    pla();     // pull from stack
    a += 0x80; // add 128 pixels
    x = a;
    if (x < 0x20)
    {             // then do not change altered coordinate
        a = 0xf8; // otherwise move last three sprites offscreen
    } // BotSP: dump vertical coordinate + 128 pixels
    writeData(Sprite_Y_Position + 12 + y, a);
    writeData(Sprite_Y_Position + 16 + y, a); // into Y coordinates
    writeData(Sprite_Y_Position + 20 + y, a);
    a = M(Enemy_OffscreenBits); // get offscreen bits
    pha();                      // save to stack
    a &= 0b00001000;            // check d3
    if (a != 0)
    {
        a = 0xf8;                               // if d3 was set, move first and
        writeData(Sprite_Y_Position + y, 0xf8); // fourth sprites offscreen
        writeData(Sprite_Y_Position + 12 + y, 0xf8);
    } // SOfs: move out and back into stack
    pla();
    pha();
    a &= 0b00000100; // check d2
    if (a != 0)
    {
        a = 0xf8;                                   // if d2 was set, move second and
        writeData(Sprite_Y_Position + 4 + y, 0xf8); // fifth sprites offscreen
        writeData(Sprite_Y_Position + 16 + y, 0xf8);
    } // SOfs2: get from stack
    pla();
    a &= 0b00000010; // check d1
    if (a != 0)
    {
        a = 0xf8;                                   // if d1 was set, move third and
        writeData(Sprite_Y_Position + 8 + y, 0xf8); // sixth sprites offscreen
        writeData(Sprite_Y_Position + 20 + y, 0xf8);
    } // ExSPl: get enemy object offset and leave
    x = M(ObjectOffset);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::JumpspringHandler()
{
    const uint8_t Jumpspring_Y_PosData_data[] = {0x08, 0x10, 0x08, 0x00};

    GetEnemyOffscreenBits(); // get offscreen information

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
    RelativeEnemyPosition();
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

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::RunRetainerObj()
{
    GetEnemyOffscreenBits();
    RelativeEnemyPosition();
    EnemyGfxHandler(x);
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

// Inputs: x = enemy object buffer offset (forwarded to SetHiMax)
// Outputs: none
void SMBEngine::MoveFallingPlatform()
{
    y = 0x20; // set movement amount
    SetHiMax();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveLakitu()
{
    // check lakitu's enemy state for d5 set
    if ((M(Enemy_State + x) & 0b00100000) != 0)
    {
        MoveD_EnemyVertically(x); // jump to move defeated lakitu downwards
        return;
    }

    uint8_t moveSpeed = 0;
    // ChkLS: if lakitu's enemy state is not set at all, go ahead and continue with code
    if (M(Enemy_State + x) != 0)
    {
        writeData(LakituMoveDirection + x, 0x00); // otherwise initialize moving direction to move to left
        writeData(EnemyFrenzyBuffer, 0x00);       // initialize frenzy buffer
        moveSpeed = 0x10;                         // load horizontal speed
    }
    else
    {
        // Fr12S
        writeData(EnemyFrenzyBuffer, Spiny); // set spiny identifier in frenzy buffer
        y = 0x02;

        do // LdLDa: load values
        {
            writeData(0x0001 + y, M(LakituDiffAdj + y)); // store in zero page
            --y;
        } while ((y & 0x80) == 0); // do this until all values are stired
        PlayerLakituDiff(); // execute sub to set speed and create spinys
        moveSpeed = a;      // the sub returns the movement speed in the accumulator
    }

    // SetLSpd: set movement speed returned from sub
    writeData(LakituMoveSpeed + x, moveSpeed);

    // move to the right by default; if the LSB of the moving direction is clear, negate the
    // speed and move to the left instead
    const bool moveRight = (M(LakituMoveDirection + x) & 0x01) != 0;
    if (!moveRight)
    {
        // get two's compliment of moving speed and store as new moving speed
        writeData(LakituMoveSpeed + x, (M(LakituMoveSpeed + x) ^ 0xff) + 0x01);
    }
    // SetLMov: store moving direction
    writeData(Enemy_MovingDir + x, moveRight ? 0x01 : 0x02);
    MoveEnemyHorizontally(x); // move lakitu horizontally
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none (most exit paths reload x from ObjectOffset; a couple of early returns leave x
// already equal to the enemy offset)
void SMBEngine::BalancePlatform()
{
    uint32_t wide = 0;

    // check high byte of vertical position
    if (M(Enemy_Y_HighPos + x) == 0x03)
    {
        EraseEnemyObject(x); // if far below screen, kill the object
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
        a = y; // save offset for other platform to stack
        pha();
        MoveFallingPlatform(); // make current platform fall
        pla();
        x = a;                 // pull offset from stack and save to X
        MoveFallingPlatform(); // make other platform fall
        x = M(ObjectOffset);
        a = M(PlatformCollisionFlag + x); // if player not standing on either platform,
        if ((a & 0x80) == 0)
        {                            // skip this part
            x = a;                   // transfer collision flag offset as offset to X
            PositionPlayerOnVPlat(); // and position player appropriately
        } // ExPF: get enemy object buffer offset and leave
        x = M(ObjectOffset);
        return;
    } // ChkForFall

    const auto checkBreak = [&](uint8_t which, uint8_t other)
    {
        if (0x2d >= M(Enemy_Y_Position + which))
        {
            if (other == M(0x00))
            {
                x = y;
                GetEnemyOffscreenBits();                             // get offscreen bits
                SetupFloateyNumber(6, x);                            // award 1000 points to player
                writeData(FloateyNum_X_Pos + x, M(Player_Rel_XPos)); // put floatey number coordinates where player is
                writeData(FloateyNum_Y_Pos + x, M(Player_Y_Position));
                writeData(Enemy_MovingDir + x, 0x01); // falling platforms

                StopPlatforms();
                return true;
            }
            a = 0x2f;                                  // otherwise add 2 pixels to vertical position
            writeData(Enemy_Y_Position + which, 0x2f); // of current platform and branch elsewhere
            StopPlatforms();                           // to make platforms stop
            return true;
        }
        return false;
    };

    if (checkBreak(x, y))
    {
        return;
    }
    if (checkBreak(y, x))
    {
        return;
    }

    a = M(Enemy_Y_Position + x); // save vertical position to stack
    pha();
    a = M(PlatformCollisionFlag + x); // get collision flag
    if ((a & 0x80) != 0)
    {                                                                           // branch if collision
        wide = ((M(Enemy_Y_Speed + x) << 8) | M(Enemy_Y_MoveForce + x)) + 0x05; // add $05 to contents of moveforce, whatever they be
        writeData(0x00, LOBYTE(wide));                                          // store here
        a = HIBYTE(wide);                                                       // the vertical speed, plus the carry
        if ((a & 0x80) != 0)
        {
            MovePlatformDown();
            DoOtherPlatform();
            return;
        }
        if (a != 0)
        {
            MovePlatformUp();
            DoOtherPlatform(); // jump ahead to remaining code
            return;
        }
        a = M(0x00);
        if (a < 0x0b)
        {
            StopPlatforms();
            DoOtherPlatform(); // jump ahead to remaining code
            return;
        }
        if (a >= 0x0b)
        {
            MovePlatformUp();
            DoOtherPlatform(); // jump ahead to remaining code
            return;
        }
    } // ColFlg: if collision flag matches
    if (a == M(ObjectOffset))
    {
        MovePlatformDown();
        DoOtherPlatform();
        return;
    }

    MovePlatformUp();
    DoOtherPlatform(); // jump ahead to remaining code
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::ProcMoveRedPTroopa()
{
    // with no vertical force or speed, and still above its original coordinate, the paratroopa
    // just drifts down a pixel every eighth frame; otherwise fall through to MoveRedPTUpOrDown
    const bool anyVerticalMovement = (M(Enemy_Y_Speed + x) | M(Enemy_Y_MoveForce + x)) != 0;
    if (!anyVerticalMovement)
    {
        writeData(Enemy_YMF_Dummy + x, 0x00); // initialize something here
        // check current vs. original vertical coordinate
        if (M(Enemy_Y_Position + x) < M(RedPTroopaOrigXPos + x))
        {
            if ((M(FrameCounter) & 0b00000111) == 0)
            {
                ++M(Enemy_Y_Position + x); // increment red paratroopa's vertical position
            } // NoIncPT: leave
            return;
        }
    }

    // MoveRedPTUpOrDown
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
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (the long firebar's own slot; DuplicateEnemyObj
// creates its duplicate half)
// Outputs: none
void SMBEngine::InitLongFirebar()
{
    DuplicateEnemyObj(); // create enemy object for long firebar

    InitShortFirebar();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (bowser's front half slot)
// Outputs: none
void SMBEngine::InitBowser()
{
    DuplicateEnemyObj();                                // jump to create another bowser object
    writeData(BowserFront_Offset, x);                   // save offset of first here
    writeData(BowserBodyControls, 0x00);                // initialize bowser's body controls
    writeData(BridgeCollapseOffset, 0x00);              // and bridge collapse offset
    writeData(BowserOrigXPos, M(Enemy_X_Position + x)); // store original horizontal position here
    writeData(BowserFireBreathTimer, 0xdf);             // store something here
    writeData(Enemy_MovingDir + x, 0xdf);               // and in moving direction
    writeData(BowserFeetCounter, 0x20);                 // set bowser's feet timer and in enemy timer
    writeData(EnemyFrameTimer + x, 0x20);
    writeData(BowserHitPoints, 0x05); // give bowser 5 hit points
    a = 0x02;
    writeData(BowserMovementSpeed, 0x02); // set default movement speed here
}

//------------------------------------------------------------------------

// Inputs: x = original enemy object buffer offset (the one being duplicated)
// Outputs: y = the new enemy's slot found (also stashed in DuplicateObj_Offset memory, which is
// how callers actually retrieve it); a is scratch
void SMBEngine::DuplicateEnemyObj()
{
    y = 0xff; // start at beginning of enemy slots

    do // FSLoop: increment one slot
    {
        ++y;
        // check enemy buffer flag for empty slot
    } while (M(Enemy_Flag + y) != 0); // if set, branch and keep checking
    writeData(DuplicateObj_Offset, y);                  // otherwise set offset here
    a = x;                                              // transfer original enemy buffer offset
    a |= 0b10000000;                                    // store with d7 set as flag in new enemy
    writeData(Enemy_Flag + y, a);                       // slot as well as enemy offset
    writeData(Enemy_PageLoc + y, M(Enemy_PageLoc + x)); // copy page location and horizontal coordinates
    // from original enemy to new enemy
    writeData(Enemy_X_Position + y, M(Enemy_X_Position + x));
    writeData(Enemy_Flag + x, 0x01);      // set flag as normal for original enemy
    writeData(Enemy_Y_HighPos + y, 0x01); // set high vertical byte for new enemy
    a = M(Enemy_Y_Position + x);
    writeData(Enemy_Y_Position + y, a); // copy vertical coordinate from original to new
}

//------------------------------------------------------------------------

// Inputs: none (uses BowserFlameTimerCtrl from memory as its own index)
// Outputs: a = timer value selected from FlameTimerData_data, used by both callers
void SMBEngine::SetFlameTimer()
{
    const uint8_t FlameTimerData_data[] = {0xbf, 0x40, 0xbf, 0xbf, 0xbf, 0x40, 0x40, 0xbf};

    y = M(BowserFlameTimerCtrl); // load counter as offset
    ++M(BowserFlameTimerCtrl);   // increment
    // mask out all but 3 LSB
    a = M(BowserFlameTimerCtrl) & 0b00000111; // to keep in range of 0-7
    writeData(BowserFlameTimerCtrl, a);
    a = FlameTimerData_data[y]; // load value to be used then leave
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
    RelativeEnemyPosition();
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
    GetEnemyOffscreenBits();        // get offscreen information
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
    RelativeEnemyPosition();
    // copy relative coordinates
    writeData(Fireball_Rel_YPos, M(Enemy_Rel_YPos)); // from the enemy object to the fireball object
    // first vertical, then horizontal
    writeData(Fireball_Rel_XPos, M(Enemy_Rel_XPos));
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    a = M(ExplosionGfxCounter + x); // get explosion graphics counter
    DrawExplosion_Fireworks(a, y);  // do a sub to draw the explosion then leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::DrawLargePlatform()
{
    bool shiftedBit = false;

    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    writeData(0x02, y);             // store here
    ++y;                            // add 3 to it for offset
    ++y;                            // to X coordinate
    ++y;
    a = M(Enemy_Rel_XPos); // get horizontal relative coordinate
    SixSpriteStacker();    // store X coordinates using A as base, stack horizontally
    x = M(ObjectOffset);
    const uint8_t platformYPos = M(Enemy_Y_Position + x); // get vertical coordinate
    DumpFourSpr(platformYPos, y);                         // dump into first four sprites as Y coordinate

    // ShrinkPlatform: castle-type levels and secondary hard mode move the last two sprites
    // offscreen, shrinking the platform
    const bool shrinkPlatform = (M(AreaType) == 0x03) || (M(SecondaryHardMode) != 0);
    const uint8_t lastTwoYPos = shrinkPlatform ? 0xf8 : platformYPos;

    // SetLast2Platform
    y = M(Enemy_SprDataOffset + x);                     // get OAM data offset
    writeData(Sprite_Y_Position + 16 + y, lastTwoYPos); // store vertical coordinate or offscreen
    writeData(Sprite_Y_Position + 20 + y, lastTwoYPos); // coordinate into last two sprites as Y coordinate

    // the girder is the default tile; cloud levels use a puff instead
    const uint8_t platformTile = (M(CloudTypeOverride) != 0) ? 0x75 : 0x5b;
    // SetPlatformTilenum
    x = M(ObjectOffset);         // get enemy object buffer offset
    ++y;                         // increment Y for tile offset
    DumpSixSpr(platformTile, y); // dump tile number into all six sprites
    a = 0x02;                    // set palette controls
    ++y;                         // increment Y for sprite attributes
    DumpSixSpr(a, y);            // dump attributes into all six sprites
    ++x;                         // increment X for enemy objects
    a = GetXOffscreenBits(x);    // get offscreen bits again
    --x;
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d7, save the remaining
    pha();   // bits to the stack
    if (shiftedBit)
    {
        a = 0xf8; // if d7 was set, move first sprite offscreen
        writeData(Sprite_Y_Position + y, 0xf8);
    } // SChk2: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d6
    pha();   // save to stack
    if (shiftedBit)
    {
        a = 0xf8; // if d6 was set, move second sprite offscreen
        writeData(Sprite_Y_Position + 4 + y, 0xf8);
    } // SChk3: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d5
    pha();   // save to stack
    if (shiftedBit)
    {
        a = 0xf8; // if d5 was set, move third sprite offscreen
        writeData(Sprite_Y_Position + 8 + y, 0xf8);
    } // SChk4: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d4
    pha();   // save to stack
    if (shiftedBit)
    {
        a = 0xf8; // if d4 was set, move fourth sprite offscreen
        writeData(Sprite_Y_Position + 12 + y, 0xf8);
    } // SChk5: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    a <<= 1; // take d3
    pha();   // save to stack
    if (shiftedBit)
    {
        a = 0xf8; // if d3 was set, move fifth sprite offscreen
        writeData(Sprite_Y_Position + 16 + y, 0xf8);
    } // SChk6: get bits from stack
    pla();
    shiftedBit = (a & 0x80) != 0;
    if (shiftedBit) // and d2
    {               // save to stack
        a = 0xf8;
        writeData(Sprite_Y_Position + 20 + y, 0xf8); // if d2 was set, move sixth sprite offscreen
    } // SLChk: check d7 of offscreen bits
    a = M(Enemy_OffscreenBits);
    a <<= 1; // and if d7 is not set, skip sub
    if ((M(Enemy_OffscreenBits) & 0x80) != 0)
    {
        MoveSixSpritesOffscreen(y); // otherwise branch to move all sprites offscreen
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
void SMBEngine::XMovingPlatform()
{
    a = 0x0e;                         // load preset maximum value for secondary counter
    XMoveCntr_Platform();             // do a sub to increment counters for movement
    MoveWithXMCntrs();                // do a sub to move platform accordingly, and return value
    a = M(PlatformCollisionFlag + x); // if no collision with player,
    if ((a & 0x80) != 0)
    { // branch ahead to leave
        return;
    }
    PositionPlayerOnHPlat();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (forwarded to PositionPlayerOnVPlat); reads zero-page
// 0x00 for the signed horizontal adder left by an earlier MoveEnemyHorizontally call
// Outputs: none
void SMBEngine::PositionPlayerOnHPlat()
{
    uint32_t wide = 0;

    y = M(0x00); // the saved value from the second subroutine is a signed adder
    wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + (int8_t)M(0x00);
    writeData(Player_X_Position, LOBYTE(wide)); // position player accordingly in horizontal position
    writeData(Player_PageLoc, HIBYTE(wide));    // SetPVar: save result to player's page location
    a = HIBYTE(wide);
    writeData(Platform_X_Scroll, y); // put saved value from second sub here to be used later
    PositionPlayerOnVPlat();         // position player vertically and appropriately
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::RightPlatform()
{
    MoveEnemyHorizontally(x);         // move platform with current horizontal speed, if any
    writeData(0x00, a);               // store saved value here (residual code)
    a = M(PlatformCollisionFlag + x); // check collision flag, if no collision between player
    if ((a & 0x80) == 0)
    { // and platform, branch ahead, leave speed unaltered
        a = 0x10;
        writeData(Enemy_X_Speed + x, 0x10); // otherwise set new speed (gets moving if motionless)
        PositionPlayerOnHPlat();            // use saved value from earlier sub to position player
    } // ExRPl: then leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (forwarded to NotMoveEnemySlowVert)
// Outputs: none
void SMBEngine::MoveDropPlatform()
{
    y = 0x7f; // set movement amount for drop platform
    NotMoveEnemySlowVert();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (forwarded to NotMoveEnemySlowVert)
// Outputs: none
void SMBEngine::MoveEnemySlowVert()
{
    y = 0x0f; // set movement amount for bowser/other objects
    NotMoveEnemySlowVert();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset; y = movement amount preset by the caller (0x7f for
// drop platforms, 0x0f for bowser/other objects)
// Outputs: none
void SMBEngine::NotMoveEnemySlowVert()
{
    a = 0x02;
    if (a != 0)
    {
        SetXMoveAmt(a, x, y); // unconditional branch
        return;
    }
    MoveJ_EnemyVertically();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (forwarded to SetHiMax)
// Outputs: none
void SMBEngine::MoveJ_EnemyVertically()
{
    y = 0x1c; // set movement amount for podoboo/other objects
    SetHiMax();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
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

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MovePodoboo()
{
    // check enemy timer
    if (M(EnemyIntervalTimer + x) == 0)
    {                  // branch to move enemy if not expired
        InitPodoboo(); // otherwise set up podoboo again
        // get part of LSFR
        a = M(PseudoRandomBitReg + 1 + x) | 0b10000000; // set d7
        writeData(Enemy_Y_MoveForce + x, a);            // store as movement force
        a &= 0b00001111;                                // mask out high nybble
        a |= 0x06;                                      // set for at least six intervals
        writeData(EnemyIntervalTimer + x, a);           // store as new enemy timer
        a = 0xf9;
        writeData(Enemy_Y_Speed + x, 0xf9); // set vertical speed to move podoboo upwards
    } // PdbM: branch to impose gravity on podoboo
    MoveJ_EnemyVertically();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveJumpingEnemy()
{
    MoveJ_EnemyVertically();  // do a sub to impose gravity on green paratroopa
    MoveEnemyHorizontally(x); // jump to move enemy horizontally
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveBloober()
{
    const uint8_t BlooberBitmasks_data[] = {0b00111111, 0b00000011};

    uint32_t wide = 0;
    bool blooberCarry = false;
    bool enemyRightOfPlayer = false;

    a = M(Enemy_State + x) & 0b00100000; // check enemy state for d5 set
    if (a == 0)
    {                             // branch if set to move defeated bloober
        y = M(SecondaryHardMode); // use secondary hard mode flag as offset
        // get LSFR
        a = M(PseudoRandomBitReg + 1 + x) & BlooberBitmasks_data[y]; // mask out bits in LSFR using bitmask loaded with offset
        blooberCarry = false;                                        // the jump engine that dispatched here left the carry clear
        if (a == 0)
        { // if any bits set, skip ahead to make swim
            uint8_t movingDir = 0;
            if ((x & 0x01) != 0)
            {                                    // on the second or fourth slot (1 or 3)
                movingDir = M(Player_MovingDir); // load player's moving direction and
                blooberCarry = true;             // the shift of an odd slot number carries its d0 out
            }
            else
            {
                // FBLeft: set left moving direction by default
                movingDir = 0x02;
                std::tie(enemyRightOfPlayer, a) = PlayerEnemyDiff(x); // get horizontal difference between player and bloober
                blooberCarry = enemyRightOfPlayer;                    // the difference leaves its no-borrow behind
                if ((a & 0x80) != 0)
                {
                    --movingDir; // enemy left of player, decrement to set right moving direction
                }
            }
            // SBMDir: set moving direction of bloober, then continue on here
            writeData(Enemy_MovingDir + x, movingDir);
        } // BlooberSwim
        ProcSwimmingB(blooberCarry);   // execute sub to make bloober swim characteristically
        a = M(Enemy_Y_Position + x);   // get vertical coordinate
        a -= M(Enemy_Y_MoveForce + x); // subtract movement force
        if (a >= 0x20)
        {                                       // if so, don't do it
            writeData(Enemy_Y_Position + x, a); // otherwise, set new vertical position, make bloober swim
        } // SwimX: check moving direction
        y = M(Enemy_MovingDir + x);
        --y;
        if (y == 0)
        { // if moving to the left, branch to second part
            wide = ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x)) + M(BlooperMoveSpeed + x);
            writeData(Enemy_X_Position + x, LOBYTE(wide)); // store result as new horizontal coordinate
            writeData(Enemy_PageLoc + x, HIBYTE(wide));    // store as new page location and leave
            a = HIBYTE(wide);
            return;

            //------------------------------------------------------------------------
        } // LeftSwim
        wide = ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x)) - M(BlooperMoveSpeed + x);
        writeData(Enemy_X_Position + x, LOBYTE(wide)); // store result as new horizontal coordinate
        writeData(Enemy_PageLoc + x, HIBYTE(wide));    // store as new page location and leave
        a = HIBYTE(wide);
        return;

        //------------------------------------------------------------------------
    } // MoveDefeatedBloober
    MoveEnemySlowVert(); // jump to move defeated bloober downwards
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveBulletBill()
{
    // check bullet bill's enemy object state for d5 set
    a = M(Enemy_State + x) & 0b00100000;
    if (a != 0)
    {                            // if not set, continue with movement code
        MoveJ_EnemyVertically(); // otherwise jump to move defeated bullet bill downwards
        return;
    } // NotDefB: set bullet bill's horizontal speed
    a = 0xe8;
    writeData(Enemy_X_Speed + x, 0xe8); // and move it accordingly (note: this bullet bill
    MoveEnemyHorizontally(x);           // object occurs in frenzy object $17, not from cannons)
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveSwimmingCheepCheep()
{
    const uint8_t SwimCCXMoveData_data[] = {
        0x40, 0x80, 0x04, 0x04 // residual data, not used
    };

    uint32_t wide = 0;

    // check cheep-cheep's enemy object state
    a = M(Enemy_State + x) & 0b00100000; // for d5 set
    if (a != 0)
    {                        // if not set, continue with movement code
        MoveEnemySlowVert(); // otherwise jump to move defeated cheep-cheep downwards
        return;
    } // CCSwim: save enemy state in $03
    writeData(0x03, a);
    a = M(Enemy_ID + x); // get enemy identifier
    a -= 0x0a;           // subtract ten for cheep-cheep identifiers
    y = a;               // use as offset
    // load value here
    writeData(0x02, SwimCCXMoveData_data[y]);
    // page:coordinate:force is one 24-bit quantity here, so the borrow runs all the way up
    wide = (M(Enemy_PageLoc + x) << 16) | (M(Enemy_X_Position + x) << 8) | M(Enemy_X_MoveForce + x);
    wide -= M(0x02);                                     // subtract preset value from horizontal force
    writeData(Enemy_X_MoveForce + x, LOBYTE(wide));      // store as new horizontal force
    writeData(Enemy_X_Position + x, HIBYTE(wide));       // and save as new horizontal coordinate
    writeData(Enemy_PageLoc + x, (uint8_t)(wide >> 16)); // page location, then save
    a = (uint8_t)(wide >> 16);
    a = 0x20;
    writeData(0x02, 0x20); // save new value here
    if (x < 0x02)
    {
        return; // if in first or second slot, branch to leave
    }
    // check movement flag
    if (M(CheepCheepMoveMFlag + x) >= 0x10)
    { // branch to move upwards
        // highpos:position:dummy is one 24-bit quantity
        wide = (M(Enemy_Y_HighPos + x) << 16) | (M(Enemy_Y_Position + x) << 8) | M(Enemy_YMF_Dummy + x);
        wide += (M(0x03) << 8) | M(0x02);              // add preset value to the dummy and enemy state to the coordinate
        writeData(Enemy_YMF_Dummy + x, LOBYTE(wide));  // and save dummy
        writeData(Enemy_Y_Position + x, HIBYTE(wide)); // save as new vertical coordinate, slowly moved downwards
        a = (uint8_t)(wide >> 16);                     // and the page location
    } // CCSwimUpwards
    else // jump to end of movement code
    {
        // highpos:position:dummy is one 24-bit quantity
        wide = (M(Enemy_Y_HighPos + x) << 16) | (M(Enemy_Y_Position + x) << 8) | M(Enemy_YMF_Dummy + x);
        wide -= (M(0x03) << 8) | M(0x02);              // subtract preset value from the dummy and enemy state from the coordinate
        writeData(Enemy_YMF_Dummy + x, LOBYTE(wide));  // and save dummy
        writeData(Enemy_Y_Position + x, HIBYTE(wide)); // save as new vertical coordinate, slowly moved upwards
        a = (uint8_t)(wide >> 16);                     // and the page location
    } // ChkSwimYPos
    writeData(Enemy_Y_HighPos + x, a); // save new page location here
    y = 0x00;                          // load movement speed to upwards by default
    a = M(Enemy_Y_Position + x);       // get vertical coordinate
    a -= M(CheepCheepOrigYPos + x);    // subtract original coordinate from current
    if ((a & 0x80) != 0)
    {             // if result positive, skip to next part
        y = 0x10; // otherwise load movement speed to downwards
        a ^= 0xff;
        a += 0x01; // to obtain total difference of original vs. current
    } // YPDiff: if difference between original vs. current vertical
    if (a < 0x0f)
    {
        return; // coordinates < 15 pixels, leave movement speed alone
    }
    a = y;
    writeData(CheepCheepMoveMFlag + x, a); // otherwise change movement speed

    // ExSwCC: leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveFlyingCheepCheep()
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

    // check cheep-cheep's enemy state
    a = M(Enemy_State + x) & 0b00100000; // for d5 set
    if (a != 0)
    { // branch to continue code if not set
        a = 0x00;
        writeData(Enemy_SprAttrib + x, 0x00); // otherwise clear sprite attributes
        MoveJ_EnemyVertically();              // and jump to move defeated cheep-cheep downwards
        return;
    } // FlyCC: move cheep-cheep horizontally based on speed and force
    MoveEnemyHorizontally(x);
    y = 0x0d;                          // set vertical movement amount
    a = 0x05;                          // set maximum speed
    SetXMoveAmt(a, x, y);              // branch to impose gravity on flying cheep-cheep
    a = M(Enemy_Y_MoveForce + x) >> 4; // get vertical movement force and move high nybble to low
    y = a;                             // save as offset (note this tends to go into reach of code)
    a = M(Enemy_Y_Position + x);       // get vertical position
    a -= PRandomSubtracter_data[y];
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
    a = FlyCCBPriority_data[y];
    writeData(Enemy_SprAttrib + x, a); // broken or residual code, value is overwritten before
    // drawing it next frame), then leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::DropPlatform()
{
    a = M(PlatformCollisionFlag + x); // if no collision between platform and player
    if ((a & 0x80) == 0)
    {                            // occurred, just leave without moving anything
        MoveDropPlatform();      // otherwise do a sub to move platform down very quickly
        PositionPlayerOnVPlat(); // do a sub to position player appropriately
    } // ExDPl: leave
}

//------------------------------------------------------------------------

// Inputs: none (always sweeps enemy slots 4 down to 0)
// Outputs: x is reloaded from ObjectOffset
void SMBEngine::KillAllEnemies()
{
    x = 0x04; // start with last enemy slot

    do // KillLoop: branch to kill enemy objects
    {
        EraseEnemyObject(x);
        --x; // move onto next enemy slot
    } while ((x & 0x80) == 0); // do this until all slots are emptied
    writeData(EnemyFrenzyBuffer, a); // empty frenzy buffer
    x = M(ObjectOffset);             // get enemy object offset and leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: x is reloaded from ObjectOffset on every exit path (directly, or via
// ChkForPlayerC_LargeP)
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
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (this platform, or the balance-platform's "other" half
// per caller)
// Outputs: x is reloaded from ObjectOffset on every exit path
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
    GetEnemyBoundBoxOfsArg(a); // get bounding box offset in Y
    // store vertical coordinate in
    writeData(0x00, M(Enemy_Y_Position + x)); // temp variable for now
    a = x;                                    // send offset we're on to the stack
    pha();
    collisionFound = PlayerCollisionCore(y); // do player-to-platform collision detection
    pla();                                   // retrieve offset from the stack
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
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::RunSmallPlatform()
{
    GetEnemyOffscreenBits();
    RelativeEnemyPosition();
    SmallPlatformBoundBox();
    SmallPlatformCollision();
    RelativeEnemyPosition();
    DrawSmallPlatform();
    MoveSmallPlatform();
    OffscreenBoundsCheck(x);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
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
    OffscreenBoundsCheck(x);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::SmallPlatformBoundBox()
{
    // store bitmask here for now
    writeData(0x00, 0x08);
    y = 0x04; // store another bitmask here for now
    GetMaskedOffScrBits(x, y);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::LargePlatformBoundBox()
{
    ++x;                      // increment X to get the proper offset
    a = GetXOffscreenBits(x); // then jump directly to the sub for horizontal offscreen bits
    --x;                      // decrement to return to original offset
    if (a >= 0xfe)
    {
        MoveBoundBoxOffscreen(x); // box offscreen, otherwise start getting coordinates
        return;
    }

    SetupEOffsetFBBox(x);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (first enemy)
// Outputs: x is reloaded from ObjectOffset
void SMBEngine::EnemiesCollision()
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

    // Lakitu and the piranha plant never take part, and neither does anything offscreen. Every
    // way out from here on lands on ExitECRoutine, which reloads the object offset.
    const auto checkAgainstOtherEnemies = [&]()
    {
        const uint8_t firstEnemyId = M(Enemy_ID + x);
        if (firstEnemyId >= 0x15 || firstEnemyId == Lakitu || firstEnemyId == PiranhaPlant)
        {
            return;
        }
        if (M(EnemyOffscrBitsMasked + x) != 0)
        {
            return; // masked offscreen bits nonzero, leave
        }
        GetEnemyBoundBoxOfs(); // get appropriate bounding box offset for the first enemy
        // The loop below clobbers y, and the original restored this through the stack on every
        // pass; it is the same value throughout.
        const uint8_t firstBoundBoxOfs = y;
        --x; // decrement to the second enemy we're going to compare against
        if ((x & 0x80) != 0)
        {
            return; // leave if there are no other enemies
        }

        // One pass of ECLoop: compare the first enemy against the second enemy now in x, and
        // either record the collision or clear the bit. ReadyNextEnemy runs after it either way.
        const auto checkEnemyPair = [&]()
        {
            // check enemy object enable flag
            if (M(Enemy_Flag + x) == 0)
            {
                return; // branch if flag not set
            }
            const uint8_t secondEnemyId = M(Enemy_ID + x);
            if (secondEnemyId >= 0x15 || secondEnemyId == Lakitu || secondEnemyId == PiranhaPlant)
            {
                return;
            }
            if (M(EnemyOffscrBitsMasked + x) != 0)
            {
                return; // branch if masked offscreen bits set
            }
            // get second enemy object's bounding box offset: multiply by four, then add four
            x = (x << 2) + 0x04;
            // do collision detection using the two enemies here
            const bool collisionFound = SprObjectCollisionCore(x, y);
            x = M(ObjectOffset); // use first enemy offset for X
            y = M(0x01);         // use second enemy offset for Y
            if (!collisionFound)
            {
                // NoEnemyCollision: clear the bit connected to the second enemy
                writeData(Enemy_CollisionBits + y, M(Enemy_CollisionBits + y) & ClearBitsMask_data[x]);
                return;
            }
            // check both enemy states for d7 set; skip this part if at least one of them is set
            if (((M(Enemy_State + x) | M(Enemy_State + y)) & 0b10000000) == 0)
            {
                // check to see if bit connected to second enemy is already set
                if ((M(Enemy_CollisionBits + y) & SetBitsMask_data[x]) != 0)
                {
                    return; // already set, move onto next enemy slot
                }
                // if the bit is not set, set it now
                writeData(Enemy_CollisionBits + y, M(Enemy_CollisionBits + y) | SetBitsMask_data[x]);
            }
            // YesEC: react according to the nature of collision
            ProcEnemyCollisions();
        };

        do // ECLoop
        {
            writeData(0x01, x);   // save enemy object buffer offset for second enemy here
            y = firstBoundBoxOfs; // first enemy's bounding box offset
            checkEnemyPair();

            // ReadyNextEnemy: get and decrement second enemy's object buffer offset
            x = M(0x01);
            --x;
        } while ((x & 0x80) == 0); // loop until all enemy slots have been checked
    };
    checkAgainstOtherEnemies();

    // ExitECRoutine: get enemy object buffer offset and leave
    x = M(ObjectOffset);
}

//------------------------------------------------------------------------

// Inputs: x = first enemy's object buffer offset; y = second enemy's offset (also available via
// zero-page 0x01)
// Outputs: none
void SMBEngine::ProcEnemyCollisions()
{
    // check both enemy states for d5 set
    a = M(Enemy_State + y) | M(Enemy_State + x);
    a &= 0b00100000; // if d5 is set in either state, or both, branch
    if (a != 0)
    {
        return; // to leave and do nothing else at this point
    }
    if (M(Enemy_State + x) >= 0x06)
    {
        a = M(Enemy_ID + x); // check second enemy identifier for hammer bro
        if (a == HammerBro)
        {
            return;
        }
        if ((M(Enemy_State + y) & 0x80) != 0) // check first enemy state for d7 set
        {                                     // branch if d7 is clear
            a = 0x06;
            SetupFloateyNumber(a, x); // award 1000 points for killing enemy
            ShellOrBlockDefeat(x);    // then kill enemy, then load
            y = M(0x01);              // original offset of second enemy
        } // ShellCollisions
        a = y; // move Y to X
        x = a;
        ShellOrBlockDefeat(x); // kill second enemy
        x = M(ObjectOffset);
        a = M(ShellChainCounter + x); // get chain counter for shell
        a += 0x04;                    // add four to get appropriate point offset
        x = M(0x01);
        SetupFloateyNumber(a, x);   // award appropriate number of points for second enemy
        x = M(ObjectOffset);        // load original offset of first enemy
        ++M(ShellChainCounter + x); // increment chain counter for additional enemies

        return; // ExitProcessEColl: leave!!!

        //------------------------------------------------------------------------
    } // ProcSecondEnemyColl
    // if first enemy state < $06, branch elsewhere
    if (M(Enemy_State + y) >= 0x06)
    {
        a = M(Enemy_ID + y); // check first enemy identifier for hammer bro
        if (a == HammerBro)
        {
            return;
        }
        ShellOrBlockDefeat(x); // otherwise, kill first enemy
        y = M(0x01);
        a = M(ShellChainCounter + y); // get chain counter for shell
        a += 0x04;                    // add four to get appropriate point offset
        x = M(ObjectOffset);
        SetupFloateyNumber(a, x);   // award appropriate number of points for first enemy
        x = M(0x01);                // load original offset of second enemy
        ++M(ShellChainCounter + x); // increment chain counter for additional enemies
        return;                     // leave!!!

        //------------------------------------------------------------------------
    } // MoveEOfs
    a = y; // move Y ($01) to X
    x = a;
    EnemyTurnAround(x);  // do the sub here using value from $01
    x = M(ObjectOffset); // then do it again using value from $08
    EnemyTurnAround(x);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::KillEnemyAboveBlock()
{
    ShellOrBlockDefeat(x); // do this sub to kill enemy
    a = 0xfc;              // alter vertical speed of enemy and leave
    writeData(Enemy_Y_Speed + x, 0xfc);
}

//------------------------------------------------------------------------

// do a sub here to move enemy downwards
// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::FallE()
{
    MoveD_EnemyVertically(x);

    const uint8_t enemyState = M(Enemy_State + x);
    if (enemyState == 0x02)
    {
        // MEHor: move enemy horizontally for state $02
        MoveEnemyHorizontally(x);
        return;
    }
    // SteadM is register-based; y selects which deceleration it applies. Anything other than a
    // power-up with d6 of its state set gets the slower one (SlowM).
    const bool d6Set = (enemyState & 0b01000000) != 0;
    const bool slowMovement = d6Set && (M(Enemy_ID + x) != PowerUpObject);
    y = slowMovement ? 0x01 : 0x00;
    SteadM();
}

//------------------------------------------------------------------------

// get current horizontal speed
// Inputs: x = enemy object buffer offset; y = table index selecting the deceleration to apply,
// preset by the caller
// Outputs: none
void SMBEngine::SteadM()
{
    const uint8_t XSpeedAdderData_data[] = {0x00, 0xe8, 0x00, 0x18};

    a = M(Enemy_X_Speed + x);
    pha(); // save to stack
    if ((a & 0x80) != 0)
    { // if not moving or moving right, skip, leave Y alone
        ++y;
        ++y; // otherwise increment Y to next data
    } // AddHS
    a += XSpeedAdderData_data[y];    // add value here to slow enemy down if necessary
    writeData(Enemy_X_Speed + x, a); // save as horizontal speed temporarily
    MoveEnemyHorizontally(x);        // then do a sub to move horizontally
    pla();
    writeData(Enemy_X_Speed + x, a); // get old horizontal speed from stack and return to
    // original memory location, then leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::RunFirebarObj()
{
    ProcFirebar();
    OffscreenBoundsCheck(x);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (the firebar chain's own slot)
// Outputs: none
void SMBEngine::ProcFirebar()
{
    GetEnemyOffscreenBits(); // get offscreen information
    // check for d3 set
    a = M(Enemy_OffscreenBits) & 0b00001000; // if so, branch to leave
    if (a == 0)
    {
        // if master timer control set, branch
        if (M(TimerControl) == 0)
        {                                            // ahead of this part
            a = M(FirebarSpinSpeed + x);             // load spinning speed of firebar
            FirebarSpin();                           // modify current spinstate
            a &= 0b00011111;                         // mask out all but 5 LSB
            writeData(FirebarSpinState_High + x, a); // and store as new high byte of spinstate
        } // SusFbar: get high byte of spinstate
        uint8_t spinStateHigh = M(FirebarSpinState_High + x);
        // Long firebars (identifier < $1f) are left alone. Anything else nudges the spinstate
        // off the two horizontal states, eight and twenty-four.
        const bool atHorizontalState = (M(Enemy_ID + x) >= 0x1f) && (spinStateHigh == 0x08 || spinStateHigh == 0x18);
        if (atHorizontalState)
        {
            // SkpFSte: add one to spinning thing to avoid horizontal state
            ++spinStateHigh;
            writeData(FirebarSpinState_High + x, spinStateHigh);
        }

        // SetupGFB: save high byte of spinning thing, modified or otherwise
        writeData(0xef, spinStateHigh);
        RelativeEnemyPosition();             // get relative coordinates to screen
        GetFirebarPosition();                // do a sub here (residual, too early to be used now)
        y = M(Enemy_SprDataOffset + x);      // get OAM data offset
        a = M(Enemy_Rel_YPos);               // get relative vertical coordinate
        writeData(Sprite_Y_Position + y, a); // store as Y in OAM data
        writeData(0x07, a);                  // also save here
        a = M(Enemy_Rel_XPos);               // get relative horizontal coordinate
        writeData(Sprite_X_Position + y, a); // store as X in OAM data
        writeData(0x06, a);                  // also save here
        a = 0x01;
        writeData(0x00, 0x01); // set $01 value here (not necessary)
        FirebarCollision();    // draw fireball part and do collision detection
        y = 0x05;              // load value for short firebars by default
        if (M(Enemy_ID + x) >= 0x1f)
        {             // no, branch then
            y = 0x0b; // otherwise load value for long firebars
        } // SetMFbar: store maximum value for length of firebars
        writeData(0xed, y);
        a = 0x00;
        writeData(0x00, 0x00); // initialize counter here

        do // DrawFbar: load high byte of spinstate
        {
            a = M(0xef);
            GetFirebarPosition();    // get fireball position data depending on firebar part
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
}

//------------------------------------------------------------------------

// Inputs: none (reads the horizontal/vertical adders and mirror data GetFirebarPosition left in
// zero-page 0x01-0x03, and the OAM offset from zero-page 0x06)
// Outputs: none (the segment's screen coordinates are communicated to FirebarCollision via
// zero-page 0x06/0x07; x is whatever ProcFirebar left it as)
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
    a += M(Enemy_Rel_XPos);              // horizontal adder, modified or otherwise
    writeData(Sprite_X_Position + y, a); // store as X coordinate here
    writeData(0x06, a);                  // store here for now, note offset is saved in Y still
    if (a < M(Enemy_Rel_XPos))
    { // if sprite coordinate => original coordinate, branch
        a = M(Enemy_Rel_XPos);
        a -= M(0x06); // original one and skip this part
    } // SubtR1: subtract original X from the
    else
    {
        a -= M(Enemy_Rel_XPos); // current sprite X
    } // ChkFOfs

    // The sprite goes offscreen if the coordinates are too far apart, or if the vertical
    // relative coordinate is offscreen already; otherwise handle the vertical adder.
    uint8_t spriteYPos = 0xf8;
    const bool tooFarApart = a >= 0x59;
    // VAHandl
    if (!tooFarApart && M(Enemy_Rel_YPos) != 0xf8)
    {
        a = M(0x02); // load vertical adder we got from position loader
        shiftedBit = (M(0x05) & 0x01) != 0;
        M(0x05) >>= 1; // shift LSB of mirror data one more time
        if (!shiftedBit)
        { // if the bit was set, skip this part
            a ^= 0xff;
            a += 0x01; // otherwise get two's compliment of second part
        } // AddVA: add vertical coordinate relative to screen to
        spriteYPos = a + M(Enemy_Rel_YPos); // the second data, modified or otherwise
    }

    // SetVFbr: store as Y coordinate here
    writeData(Sprite_Y_Position + y, spriteYPos);
    writeData(0x07, spriteYPos); // also store here for now
    FirebarCollision();
}

//------------------------------------------------------------------------

// Inputs: y = OAM data offset for this firebar segment; reads the segment's coordinates from
// zero-page 0x06/0x07 (set by the caller)
// Outputs: x is reloaded from ObjectOffset
void SMBEngine::FirebarCollision()
{
    DrawFirebar(y);              // run sub here to draw current tile of firebar
    const uint8_t oamOffset = y; // the OAM data offset the tail below advances

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

// Inputs: x = enemy object buffer offset (bowser flame's slot)
// Outputs: none
void SMBEngine::RunBowserFlame()
{
    ProcBowserFlame();
    GetEnemyOffscreenBits();
    RelativeEnemyPosition();
    GetEnemyBoundBox(x);
    PlayerEnemyCollision(x);
    OffscreenBoundsCheck(x);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveD_Bowser()
{
    MoveEnemySlowVert(); // do a sub to move bowser downwards
    BowserGfxHandler();  // jump to draw bowser's front and rear, then leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (bowser's slot)
// Outputs: none
void SMBEngine::RunBowser()
{
    const uint8_t PRandomRange_data[] = {0x21, 0x41, 0x11, 0x31};

    bool enemyRightOfPlayer = false;
    bool hammerSpawned = false;

    // d5 in enemy state means bowser is defeated and on his way down
    if ((M(Enemy_State + x) & 0b00100000) != 0)
    {
        // otherwise check vertical position
        if (M(Enemy_Y_Position + x) < 0xe0)
        {
            MoveD_Bowser();
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
        ChkFireB();
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
            writeData(Enemy_MovingDir + x, 0x02);
        }
        // B_FaceP: with the timer still running, turn bowser to face a player on his left
        if (M(EnemyFrameTimer + x) != 0)
        {
            // get horizontal difference between player and bowser
            std::tie(enemyRightOfPlayer, a) = PlayerEnemyDiff(x);
            if ((a & 0x80) != 0)
            {                                           // bowser to the left of the player
                writeData(Enemy_MovingDir + x, 0x01);   // set bowser to move and face to the right
                writeData(BowserMovementSpeed, 0x02);   // set movement speed
                writeData(EnemyFrameTimer + x, 0x20);   // set timer here
                writeData(BowserFireBreathTimer, 0x20); // set timer used for bowser's flame
                if (M(Enemy_X_Position + x) >= 0xc8)
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
        if (M(Enemy_X_Position + x) == M(BowserOrigXPos))
        {
            const uint8_t randomOfs = M(PseudoRandomBitReg + x) & 0b00000011; // get pseudorandom offset
            // load value using pseudorandom offset and store here
            writeData(MaxRangeFromOrigin, PRandomRange_data[randomOfs]);
        }
        // GetDToO: add the movement speed to the coordinate and save as new horizontal position
        a = M(Enemy_X_Position + x) + M(BowserMovementSpeed);
        writeData(Enemy_X_Position + x, a);
        if (M(Enemy_MovingDir + x) == 0x01)
        {
            return;
        }
        uint8_t newSpeed = 0xff; // set default movement speed here (move left)
        a -= M(BowserOrigXPos);  // distance from the original horizontal position
        if ((a & 0x80) != 0)
        { // if current position to the right of original, skip ahead
            a ^= 0xff;
            a += 0x01;
            newSpeed = 0x01; // set alternate movement speed here (move right)
        }
        // CompDToO: compare difference with pseudorandom value
        if (a < M(MaxRangeFromOrigin))
        {
            return; // if difference < pseudorandom value, leave speed alone
        }
        writeData(BowserMovementSpeed, newSpeed); // otherwise change bowser's movement speed
    };
    controlBowser();

    // HammerChk
    const uint8_t frameTimer = M(EnemyFrameTimer + x);
    if (frameTimer == 0)
    {
        MoveEnemySlowVert(); // start by moving bowser downwards
        // From world 6 on it is time to throw hammers, on every fourth frame. Worlds 1-5 skip
        // this part entirely (SetHmrTmr).
        if (M(WorldNumber) >= World6 && (M(FrameCounter) & 0b00000011) == 0)
        {
            hammerSpawned = SpawnHammerObj(); // spawn misc object (hammer)
        }
        // SetHmrTmr: get current vertical position
        if (M(Enemy_Y_Position + x) >= 0x80)
        {
            const uint8_t randomOfs = M(PseudoRandomBitReg + x) & 0b00000011; // get pseudorandom offset
            // get value using pseudorandom offset and set for timer here
            writeData(EnemyFrameTimer + x, PRandomRange_data[randomOfs]);
        }
        // SkipToFB: jump to execute flames code
        ChkFireB();
        return;
    }
    // MakeBJump: if timer not yet about to expire, skip ahead to next part
    if (frameTimer != 0x01)
    {
        ChkFireB();
        return;
    }
    --M(Enemy_Y_Position + x);          // otherwise decrement vertical coordinate
    InitVStf(x);                        // initialize movement amount
    writeData(Enemy_Y_Speed + x, 0xfe); // set vertical speed to move bowser upwards
    ChkFireB();
}

//------------------------------------------------------------------------

// check world number here
// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::ChkFireB()
{
    // ChkFireB: each pass toggles bowser's mouth. A pass that opens it loops back, where the
    // timer this pass just set sends it to the graphics handler instead of breathing fire.
    while (true)
    {
        const uint8_t worldNumber = M(WorldNumber);
        // only world 8, and worlds before 6, get to this part
        if (worldNumber != World8 && worldNumber >= World6)
        {
            BowserGfxHandler();
            return;
        }
        // SpawnFBr: check timer here
        if (M(BowserFireBreathTimer) != 0)
        {
            BowserGfxHandler(); // if not expired yet, skip all of this
            return;
        }
        writeData(BowserFireBreathTimer, 0x20); // set timer here
        a = M(BowserBodyControls) ^ 0b10000000; // invert bowser's mouth bit to open
        writeData(BowserBodyControls, a);       // and close bowser's mouth
        if ((a & 0x80) == 0)
        {
            break; // bowser's mouth now closed, go on to breathe fire
        }
    }
    SetFlameTimer(); // get timing for bowser's flame
    if (M(SecondaryHardMode) != 0)
    {              // if secondary hard mode flag not set, skip this
        a -= 0x10; // otherwise subtract from value in A
    } // SetFBTmr: set value as timer here
    writeData(BowserFireBreathTimer, a);
    a = BowserFlame;                           // put bowser's flame identifier
    writeData(EnemyFrenzyBuffer, BowserFlame); // in enemy frenzy buffer
    BowserGfxHandler();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (bowser front's slot)
// Outputs: x is restored to its input value (temporarily switched to the rear half's offset to
// process it, then pulled back from the stack)
void SMBEngine::BowserGfxHandler()
{
    ProcessBowserHalf();                      // do a sub here to process bowser's front
    y = 0x10;                                 // load default value here to position bowser's rear
    if ((M(Enemy_MovingDir + x) & 0x01) != 0) // check moving direction
    {                                         // if moving left, use default
        y = 0xf0;                             // otherwise load alternate positioning value here
    } // CopyFToR: move bowser's rear object position value to A
    a = y;
    a += M(Enemy_X_Position + x);       // add to bowser's front object horizontal coordinate
    y = M(DuplicateObj_Offset);         // get bowser's rear object offset
    writeData(Enemy_X_Position + y, a); // store A as bowser's rear horizontal coordinate
    a = M(Enemy_Y_Position + x);
    a += 0x08;                                              // vertical coordinate and store as vertical coordinate
    writeData(Enemy_Y_Position + y, a);                     // for bowser's rear
    writeData(Enemy_State + y, M(Enemy_State + x));         // copy enemy state directly from front to rear
    writeData(Enemy_MovingDir + y, M(Enemy_MovingDir + x)); // copy moving direction also
    a = M(ObjectOffset);                                    // save enemy object offset of front to stack
    pha();
    x = M(DuplicateObj_Offset); // put enemy object offset of rear as current
    writeData(ObjectOffset, x);
    a = Bowser;                      // set bowser's enemy identifier
    writeData(Enemy_ID + x, Bowser); // store in bowser's rear object
    ProcessBowserHalf();             // do a sub here to process bowser's rear
    pla();
    writeData(ObjectOffset, a); // get original enemy object offset
    x = a;
    a = 0x00; // nullify bowser's front/rear graphics flag
    writeData(BowserGfxFlag, 0x00);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (whichever half is being processed)
// Outputs: none
void SMBEngine::ProcessBowserHalf()
{
    ++M(BowserGfxFlag); // increment bowser's graphics flag, then run subroutines
    RunRetainerObj();   // to get offscreen bits, relative position and draw bowser (finally!)
    a = M(Enemy_State + x);
    if (a != 0)
    {
        return; // if either enemy object not in normal state, branch to leave
    }
    a = 0x0a;
    writeData(Enemy_BoundBoxCtrl + x, 0x0a); // set bounding box size control
    GetEnemyBoundBox(x);                     // get bounding box coordinates
    PlayerEnemyCollision(x);                 // do player-to-enemy collision detection
}

//------------------------------------------------------------------------

// check enemy buffer flag for non-active enemy slot
// Inputs: x = enemy slot to start scanning downward from
// Outputs: x = the empty slot found; reloaded from ObjectOffset if none was found
void SMBEngine::ChkNoEn()
{
    // ChkNoEn: scan downward until an empty slot turns up, or the slots run out
    while (M(Enemy_Flag + x) != 0)
    {
        --x; // otherwise check next slot
        if ((x & 0x80) != 0)
        {
            // no empty slots were found, leave
            x = M(ObjectOffset);
            return;
        }
    }

    // CreateL: initialize enemy state
    writeData(Enemy_State + x, 0x00);
    a = Lakitu; // create lakitu enemy object
    writeData(Enemy_ID + x, Lakitu);
    SetupLakitu(); // do a sub to set up lakitu
    a = 0x20;
    PutAtRightExtent(); // finish setting up lakitu
    x = M(ObjectOffset);
    // ExLSHand
}

//------------------------------------------------------------------------

// Inputs: a = vertical position to set; x = enemy object buffer offset
// Outputs: none
void SMBEngine::PutAtRightExtent()
{
    uint32_t wide = 0;

    writeData(Enemy_Y_Position + x, a);                                   // set vertical position
    wide = ((M(ScreenRight_PageLoc) << 8) | M(ScreenRight_X_Pos)) + 0x20; // place enemy 32 pixels beyond right side of screen
    writeData(Enemy_X_Position + x, LOBYTE(wide));
    writeData(Enemy_PageLoc + x, HIBYTE(wide));
    a = HIBYTE(wide);
    FinishFlame();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::FinishFlame()
{
    // set $08 for bounding box control
    writeData(Enemy_BoundBoxCtrl + x, 0x08);
    // set high byte of vertical and
    writeData(Enemy_Y_HighPos + x, 0x01); // enemy buffer flag
    writeData(Enemy_Flag + x, 0x01);
    a = 0x00;
    writeData(Enemy_X_MoveForce + x, 0x00); // initialize horizontal movement force, and
    writeData(Enemy_State + x, 0x00);       // enemy state
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (candidate lakitu/spiny slot; must be less than 5)
// Outputs: none
void SMBEngine::LakituAndSpinyHandler()
{
    a = M(FrenzyEnemyTimer); // if timer here not expired, leave
    if (a != 0)
    {
        return;
    }
    if (x >= 0x05)
    {
        return;
    }
    a = 0x80; // set timer
    writeData(FrenzyEnemyTimer, 0x80);
    y = 0x04; // start with the last enemy slot
    ChkLak();
}

//------------------------------------------------------------------------

// check all enemy slots to see
// Inputs: y = enemy slot to start scanning downward from, looking for lakitu; x = the new
// spiny's slot, used once lakitu is found
// Outputs: none
void SMBEngine::ChkLak()
{
    const uint8_t PRDiffAdjustData_data[] = {0x26, 0x2c, 0x32, 0x38, 0x20, 0x22, 0x24, 0x26, 0x13, 0x14, 0x15, 0x16};

    // ChkLak: scan downward until lakitu turns up, or the slots run out
    while (M(Enemy_ID + y) != Lakitu)
    {
        --y; // otherwise check another slot
        if ((y & 0x80) != 0)
        {
            // No lakitu in any slot. Once the reappearance timer is far enough along, go and
            // make a new one.
            ++M(LakituReappearTimer); // increment reappearance timer
            if (M(LakituReappearTimer) < 0x07)
            {
                return; // if not, leave
            }
            x = 0x04; // start with the last enemy slot again
            ChkNoEn();
            return;
        }
    }

    // CreateSpiny
    a = M(Player_Y_Position); // if player above a certain point, branch to leave
    if (a < 0x2c)
    {
        return;
    }
    a = M(Enemy_State + y); // if lakitu is not in normal state, branch to leave
    if (a != 0)
    {
        return;
    }
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
        writeData(0x01 + x, PRDiffAdjustData_data[y]); // to $01-$03
        ++y;
        ++y; // increment Y four bytes for each value
        ++y;
        ++y;
        --x; // decrement X for each one
    } while ((x & 0x80) == 0); // loop until all three are written
    x = M(ObjectOffset); // get enemy object buffer offset
    PlayerLakituDiff();  // move enemy, change direction, get value - difference

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
    writeData(Enemy_Flag + x, 0x01);    // enable enemy object by setting flag
    a = 0x05;
    writeData(Enemy_State + x, 0x05); // put spiny in egg state and leave

    // ChpChpEx
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (the flame's own slot)
// Outputs: none
void SMBEngine::InitBowserFlame()
{
    const uint8_t FlameYMFAdderData_data[] = {0xff, 0x01};

    if (M(FrenzyEnemyTimer) != 0)
    {
        return; // timer not expired yet, leave
    }
    writeData(Enemy_Y_MoveForce + x, 0x00);                           // reset something here
    writeData(NoiseSoundQueue, M(NoiseSoundQueue) | Sfx_BowserFlame); // load bowser's flame sound into queue

    y = M(BowserFront_Offset); // get bowser's buffer offset
    // check for bowser; anything else spawns the flame at the right extent instead
    if (M(Enemy_ID + y) != Bowser)
    {
        SetFlameTimer(); // get timer data based on flame counter
        // add 32 frames by default, or 16 for secondary hard mode
        const uint8_t flameTimer = a + ((M(SecondaryHardMode) != 0) ? 0x10 : 0x20);
        // SetFrT: set timer accordingly
        writeData(FrenzyEnemyTimer, flameTimer);

        const uint8_t randomOfs = M(PseudoRandomBitReg + x) & 0b00000011; // get 2 LSB from first part of LSFR
        writeData(BowserFlamePRandomOfs + x, randomOfs);                  // set here
        a = M(FlameYPosData + randomOfs);                                 // load vertical position based on pseudorandom offset
        PutAtRightExtent();
        return;
    }

    // SpawnFromMouth
    a = M(Enemy_X_Position + y);                        // get bowser's horizontal position
    a -= 0x0e;                                          // subtract 14 pixels
    writeData(Enemy_X_Position + x, a);                 // save as flame's horizontal position
    writeData(Enemy_PageLoc + x, M(Enemy_PageLoc + y)); // copy page location from bowser to flame
    a = M(Enemy_Y_Position + y);
    a += 0x08;
    writeData(Enemy_Y_Position + x, a);         // save as flame's vertical position
    a = M(PseudoRandomBitReg + x) & 0b00000011; // get 2 LSB from first part of LSFR
    writeData(Enemy_YMF_Dummy + x, a);          // save here
    y = a;                                      // use as offset
    a = M(FlameYPosData + y);                   // get value here using bits as offset
    y = 0x00;                                   // load default offset
    if (a >= M(Enemy_Y_Position + x))
    {             // if less, do not increment offset
        y = 0x01; // otherwise increment now
    } // SetMF: get value here and save
    writeData(Enemy_Y_MoveForce + x, FlameYMFAdderData_data[y]); // to vertical movement force
    a = 0x00;
    writeData(EnemyFrenzyBuffer, 0x00); // clear enemy frenzy buffer
    FinishFlame();
}

//------------------------------------------------------------------------

// get coordinates relative to screen
// Inputs: x = enemy object buffer offset (power-up's slot, typically 5)
// Outputs: none
void SMBEngine::RunPUSubs()
{
    RelativeEnemyPosition();
    GetEnemyOffscreenBits(); // get offscreen bits
    GetEnemyBoundBox(x);     // get bounding box coordinates
    DrawPowerUp();           // draw the power-up object
    PlayerEnemyCollision(x); // check for collision with player
    OffscreenBoundsCheck(x); // check to see if it went offscreen

    // ExitPUp: and we're done
}

//------------------------------------------------------------------------

// Inputs: none (always operates on the fixed power-up slot 5)
// Outputs: none
void SMBEngine::PowerUpObjHandler()
{
    x = 0x05; // set object offset for last slot in enemy object buffer
    writeData(ObjectOffset, 0x05);

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
            RunPUSubs();
            return;
        }
        const uint8_t powerUpType = M(PowerUpType); // check power-up type
        // ShroomM: the normal mushroom and the 1-up mushroom both just move
        if (powerUpType == 0x00 || powerUpType == 0x03)
        {
            MoveNormalEnemy();
            EnemyToBGCollisionDet(); // deal with collisions
            RunPUSubs();             // run the other subroutines
            return;
        }
        if (powerUpType != 0x02)
        {
            RunPUSubs(); // if not star, branch elsewhere to skip movement
            return;
        }
        MoveJumpingEnemy(); // otherwise impose gravity on star power-up and make it jump
        EnemyJump();        // note that green paratroopa shares the same code here
        RunPUSubs();        // then jump to other power-up subroutines
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
            writeData(Enemy_X_Speed + x, 0x10);     // set horizontal speed
            writeData(Enemy_State + 5, 0b10000000); // and then set d7 in power-up object's state
            writeData(Enemy_SprAttrib + 5, 0x00);   // initialize background priority bit set here
            writeData(Enemy_MovingDir + x, 0x01);   // set right moving direction
        }
    }

    // ChkPUSte: check power-up object's state
    if (M(Enemy_State + 5) < 0x06)
    {
        return; // if not far enough along, don't even bother running these routines
    }
    RunPUSubs();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::RunNormalEnemies()
{
    a = 0x00; // init sprite attributes
    writeData(Enemy_SprAttrib + x, 0x00);
    GetEnemyOffscreenBits();
    RelativeEnemyPosition();
    EnemyGfxHandler(x);
    GetEnemyBoundBox(x);
    EnemyToBGCollisionDet();
    EnemiesCollision();
    PlayerEnemyCollision(x);
    y = M(TimerControl); // if master timer control set, skip to last routine
    if (y == 0)
    {
        EnemyMovementSubs();
    } // SkipMove
    OffscreenBoundsCheck(x);
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
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

// Inputs: x = enemy object buffer offset (hammer bro's slot)
// Outputs: none
void SMBEngine::ProcHammerBro()
{
    const uint8_t HammerThrowTmrData_data[] = {0x30, 0x1c};

    bool hammerSpawned = false;

    // check hammer bro's enemy state for d5 set
    a = M(Enemy_State + x) & 0b00100000;
    if (a != 0)
    {                             // if not set, go ahead with code
        MoveD_EnemyVertically(x); // otherwise move the defeated hammer bro downwards
        MoveEnemyHorizontally(x); // and then horizontally
        return;
    } // ChkJH: check jump timer
    if (M(HammerBroJumpTimer + x) != 0)
    {                                            // if expired, branch to jump
        --M(HammerBroJumpTimer + x);             // otherwise decrement jump timer
        a = M(Enemy_OffscreenBits) & 0b00001100; // check offscreen bits
        if (a != 0)
        {
            MoveHammerBroXDir(); // if hammer bro a little offscreen, skip to movement code
            return;
        }
        // Only an expired hammer throwing timer gets to throw; it reloads the timer and tries to
        // spawn the hammer, which can still fail if there is no room for the object.
        if (M(HammerThrowingTimer + x) == 0)
        {
            const uint8_t timerIndex = M(SecondaryHardMode); // get secondary hard mode flag
            // get timer data using flag as offset
            writeData(HammerThrowingTimer + x, HammerThrowTmrData_data[timerIndex]); // set as new timer
            hammerSpawned = SpawnHammerObj();                                        // do a sub here to spawn hammer object
        }
        if (hammerSpawned)
        {
            // set d3 in enemy state for hammer throw
            writeData(Enemy_State + x, M(Enemy_State + x) | 0b00001000);
        }
        else
        {
            // DecHT: decrement timer
            --M(HammerThrowingTimer + x);
        }
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
    writeData(0x00, 0x00);       // save into temp variable for now
    y = 0xfa;                    // set default vertical speed
    a = M(Enemy_Y_Position + x); // check hammer bro's vertical coordinate
    if ((a & 0x80) != 0)
    {
        SetHJ(); // if on the bottom half of the screen, use current speed
        return;
    }
    y = 0xfd;  // otherwise set alternate vertical speed
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
}

//------------------------------------------------------------------------

// set vertical speed for jumping
// Inputs: x = enemy object buffer offset; y = vertical speed to set, preset by the caller
// Outputs: none
void SMBEngine::SetHJ()
{
    const uint8_t HammerBroJumpLData_data[] = {0x20, 0x37};

    writeData(Enemy_Y_Speed + x, y);
    // set d0 in enemy state for jumping
    a = M(Enemy_State + x) | 0x01;
    writeData(Enemy_State + x, a);
    // load preset value here to use as bitmask
    a = M(0x00) & M(PseudoRandomBitReg + 2 + x); // and do bit-wise comparison with part of LSFR
    y = a;                                       // then use as offset
    a = M(SecondaryHardMode);                    // check secondary hard mode flag
    if (a == 0)
    {
        y = a; // if secondary hard mode flag clear, set offset to 0
    } // HJump: get jump length timer data using offset from before
    writeData(EnemyFrameTimer + x, HammerBroJumpLData_data[y]); // save in enemy timer
    a = M(PseudoRandomBitReg + 1 + x) | 0b11000000;             // get contents of part of LSFR, set d7 and d6, then
    writeData(HammerBroJumpTimer + x, a);                       // store in jump timer
    MoveHammerBroXDir();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveHammerBroXDir()
{
    bool enemyRightOfPlayer = false;

    y = 0xfc;                         // move hammer bro a little to the left
    a = M(FrameCounter) & 0b01000000; // change hammer bro's direction every 64 frames
    if (a == 0)
    {
        y = 0x04; // if d6 set in counter, move him a little to the right
    } // Shimmy: store horizontal speed
    writeData(Enemy_X_Speed + x, y);
    y = 0x01;                                             // set to face right by default
    std::tie(enemyRightOfPlayer, a) = PlayerEnemyDiff(x); // get horizontal difference between player and hammer bro
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
}

//------------------------------------------------------------------------

// set moving direction
// Inputs: x = enemy object buffer offset; y = moving direction to store
// Outputs: none
void SMBEngine::SetShim()
{
    writeData(Enemy_MovingDir + x, y);
    MoveNormalEnemy();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveNormalEnemy()
{
    const uint8_t RevivedXSpeed_data[] = {0x08, 0xf8, 0x0c, 0xf4};

    y = 0x00; // SteadM is register-based; leave its horizontal movement as-is

    const uint8_t enemyState = M(Enemy_State + x);
    if ((enemyState & 0b01000000) != 0)
    {            // d6 set
        FallE(); // move enemy vertically, then horizontally if necessary
        return;
    }
    if ((enemyState & 0x80) != 0)
    {             // d7 set
        SteadM(); // move enemy horizontally
        return;
    }
    if ((enemyState & 0b00100000) != 0)
    {
        // MoveDefeatedEnemy: d5 set
        MoveD_EnemyVertically(x); // execute sub to move defeated enemy downwards
        MoveEnemyHorizontally(x); // now move defeated enemy horizontally
        return;
    }

    const uint8_t stunnedState = enemyState & 0b00000111; // d2-d0 of enemy state
    if (stunnedState == 0)
    {
        SteadM(); // enemy in normal state, move it horizontally
        return;
    }
    if (stunnedState == 0x05 || stunnedState < 0x03)
    {
        FallE(); // the state used by spiny's egg, and the two low stunned states, just fall
        return;
    }

    // ReviveStunned
    const uint8_t stunTimer = M(EnemyIntervalTimer + x);
    if (stunTimer != 0)
    {
        // ChkKillGoomba: a goomba stunned at a certain point in its timer is killed outright
        if (stunTimer != 0x0e)
        {
            return; // not at that point, leave
        }
        if (M(Enemy_ID + x) != Goomba)
        {
            return; // branch if not found
        }
        EraseEnemyObject(x); // otherwise, kill this goomba object
        return;
    }

    writeData(Enemy_State + x, 0x00);                // the timer expired, initialize enemy state to normal
    const uint8_t frameBit = M(FrameCounter) & 0x01; // get d0 of frame counter
    // store as pseudorandom movement direction
    writeData(Enemy_MovingDir + x, frameBit + 1);
    // primary hard mode moves 2 bytes on to the faster half of the data
    const uint8_t speedIndex = (M(PrimaryHardMode) != 0) ? (frameBit + 2) : frameBit;
    // SetRSpd: load and store new horizontal speed, and leave
    writeData(Enemy_X_Speed + x, RevivedXSpeed_data[speedIndex]);

    // NKGmba: leave!
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::EnemyToBGCollisionDet()
{
    const uint8_t EnemyBGCStateData_data[] = {0x01, 0x01, 0x02, 0x02, 0x02, 0x05};

    bool enemyRightOfPlayer = false;

    // LandEnemyInitState: land the enemy and reset its state. Several places end here, and
    // ProcEnemyDirection below runs straight into it.
    const auto landEnemyInitState = [&]()
    {
        EnemyLanding(); // land enemy properly
        // if d7 of enemy state is set, branch
        if ((M(Enemy_State + x) & 0b10000000) == 0)
        {
            // otherwise initialize enemy state and leave; note this will also turn spiny's egg
            // into spiny
            writeData(Enemy_State + x, 0x00);
            return;
        }
        // NMovShellFallBit: nullify d6 of enemy state, save other bits, and store
        writeData(Enemy_State + x, M(Enemy_State + x) & 0b10111111);
    };

    // ProcEnemyDirection: turn the enemy to face the player, then land it
    const auto procEnemyDirection = [&]()
    {
        const uint8_t enemyId = M(Enemy_ID + x); // check enemy identifier for goomba
        if (enemyId == Goomba)
        {
            landEnemyInitState();
            return;
        }
        if (enemyId == Spiny)
        {
            writeData(Enemy_MovingDir + x, 0x01); // send enemy moving to the right by default
            writeData(Enemy_X_Speed + x, 0x08);   // set horizontal speed accordingly
            // if timed appropriately, spiny will skip over trying to face the player
            if ((M(FrameCounter) & 0b00000111) == 0)
            {
                landEnemyInitState();
                return;
            }
        }
        // InvtD: load 1 for enemy to face the left (inverted here)
        uint8_t facingDir = 0x01;
        std::tie(enemyRightOfPlayer, a) = PlayerEnemyDiff(x); // get horizontal difference between player and enemy
        if ((a & 0x80) != 0)
        {                // if enemy to the left of player, increment by one for the enemy to
            ++facingDir; // face right (inverted)
        }
        // CNwCDir
        if (facingDir != M(Enemy_MovingDir + x))
        {
            landEnemyInitState();
            return;
        }
        ChkForBump_HammerBroJ(); // if equal, not facing in correct dir, do sub to turn around
        landEnemyInitState();
    };

    // ChkForRedKoopa
    const auto chkForRedKoopa = [&]()
    {
        // check for red koopa troopa $03 in normal state
        if (M(Enemy_ID + x) == RedKoopa && M(Enemy_State + x) == 0)
        {
            ChkForBump_HammerBroJ(); // if enemy found and in normal state, branch
            return;
        }
        // Chk2MSBSt: with d7 of the state set, set d6 alongside it; otherwise the old state
        // indexes the new one (GetSteFromD)
        const uint8_t oldState = M(Enemy_State + x);
        const uint8_t newState = ((oldState & 0x80) != 0) ? (oldState | 0b01000000) : EnemyBGCStateData_data[oldState];
        // SetD6Ste: set as new state
        writeData(Enemy_State + x, newState);
        DoEnemySideCheck(); // then check for horizontal blockage and leave
    };

    // NoUnderHammerBro: if hammer bro is not standing on anything, set d0 in the enemy state to
    // indicate jumping or falling, then leave
    const auto noUnderHammerBro = [&]() { writeData(Enemy_State + x, M(Enemy_State + x) | 0x01); };

    // HammerBroBGColl
    const auto hammerBroBGColl = [&]()
    {
        ChkUnderEnemy(); // check to see if hammer bro is standing on anything
        if (a == 0)
        {
            noUnderHammerBro();
            return;
        }
        if (a == 0x23)
        {
            KillEnemyAboveBlock();
            return;
        }
        // check timer used by hammer bro
        if (M(EnemyFrameTimer + x) != 0)
        {
            noUnderHammerBro(); // branch if not expired
            return;
        }
        // save d7 and d3 from enemy state, nullify other bits, and store
        writeData(Enemy_State + x, M(Enemy_State + x) & 0b10001000);
        EnemyLanding();     // modify vertical coordinate, speed and something else
        DoEnemySideCheck(); // then check for horizontal blockage and leave
    };

    // check enemy state for d6 set; if set, leave
    if ((M(Enemy_State + x) & 0b00100000) != 0)
    {
        return;
    }
    // otherwise, do a subroutine here; leave if enemy vertical coord + 62 < 68
    if (!SubtEnemyYPos())
    {
        return;
    }

    const uint8_t enemyId = M(Enemy_ID + x);
    if (enemyId == Spiny && M(Enemy_Y_Position + x) < 0x25)
    {
        return;
    }
    // DoIDCheckBGColl
    if (enemyId == GreenParatroopaJump)
    {
        EnemyJump(); // jump elsewhere
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
    ChkUnderEnemy();
    const uint8_t blockUnder = a;
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
        y = M(0x02); // get vertical coordinate used to find block
        // store default blank metatile in that spot so we won't
        writeData(W(0x06) + y, 0x00); // trigger this routine accidentally again
        a = M(Enemy_ID + x);
        if (a >= 0x15)
        {
            ChkToStunEnemies(a, x);
            return;
        }
        if (a == Goomba)
        {
            KillEnemyAboveBlock(); // if enemy object IS goomba, do this sub
        } // GiveOEPoints

        // award 100 points for hitting block beneath enemy
        a = SetupFloateyNumber(1, x);
        // Bug in the original game: there should be another "a = M(Enemy_ID + x);" here,
        // but instead the x-coordinate of the created floatey gets passed to ChkToStunEnemies.
        // This causes https://themushroomkingdom.net/bugs/7
        ChkToStunEnemies(a, x);
        return;
    }

    // LandEnemyProperly: check lower nybble of vertical coordinate saved earlier, less eight
    // pixels; out of range means it was $0d-$0f before the subtract
    if (static_cast<uint8_t>(M(0x04) - 0x08) >= 0x05)
    {
        chkForRedKoopa();
        return;
    }

    const uint8_t landedState = M(Enemy_State + x);
    if ((landedState & 0b01000000) != 0)
    {
        landEnemyInitState(); // d6 in enemy state is set
        return;
    }
    // SChkA: lower nybble < $0d with d7 set but d6 not, and the normal state, both just check
    // the enemy's sides (ChkLandedEnemyState)
    if ((landedState & 0x80) != 0 || landedState == 0)
    {
        DoEnemySideCheck();
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
        if (M(Enemy_State + x) != 0x02)
        {
            procEnemyDirection();
            return;
        }
        // load default timer here, or $00 if the enemy identifier is spiny
        const uint8_t stunTimer = (M(Enemy_ID + x) == Spiny) ? 0x00 : 0x10;
        // SetForStn: set timer here
        writeData(EnemyIntervalTimer + x, stunTimer);
        // set state here, apparently used to render upside-down koopas and buzzy beetles
        writeData(Enemy_State + x, 0x03);
        EnemyLanding(); // then land it properly
    }
    // ExSteChk: anything in a higher numbered state just leaves
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::DoEnemySideCheck()
{
    a = M(Enemy_Y_Position + x); // if enemy within status bar, branch to leave
    if (a >= 0x20)
    {
        y = 0x16;              // start by finding block to the left of enemy ($00,$14)
        a = 0x02;              // set value here in what is also used as
        writeData(0xeb, 0x02); // OAM data offset

        do // SdeCLoop: check value
        {
            // seek a block only on the side the enemy is actually moving towards
            if (M(0xeb) == M(Enemy_MovingDir + x))
            {
                a = 0x01;               // set flag in A for save horizontal coordinate
                BlockBufferChk_Enemy(); // find block to left or right of enemy object
                // a solid block on that side blocks the enemy
                if (a != 0 && !ChkForNonSolids(a))
                {
                    ChkForBump_HammerBroJ();
                    return;
                }
            }

            // NextSdeC: move to the next direction
            --M(0xeb);
            ++y;
        } while (y < 0x18); // enemy ($00, $14) and ($10, $14) pixel coordinates
    } // ExESdeC
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::ChkForBump_HammerBroJ()
{
    if (x == 0x05)
    {
        NoBump(); // and if so, branch ahead and do not play sound
        return;
    }
    a = M(Enemy_State + x); // if enemy state d7 not set, branch
    a <<= 1;                // ahead and do not play sound
    if ((M(Enemy_State + x) & 0x80) == 0)
    {
        NoBump();
        return;
    }
    a = Sfx_Bump;                           // otherwise, play bump sound
    writeData(Square1SoundQueue, Sfx_Bump); // sound will never be played if branching from ChkForRedKoopa
    NoBump();
}

//------------------------------------------------------------------------

// check for hammer bro
// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::NoBump()
{
    if (M(Enemy_ID + x) == 0x05)
    { // branch if not found
        a = 0x00;
        writeData(0x00, 0x00); // initialize value here for bitmask
        y = 0xfa;              // load default vertical speed for jumping
        SetHJ();               // jump to code that makes hammer bro jump
        return;
    } // InvEnemyDir
    RXSpd(x); // jump to turn the enemy around
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::EnemyJump()
{
    // Every way of declining to jump lands on the same tail (DoSide), so the jump is a lambda
    // that returns and the side check below runs unconditionally afterwards.
    const auto jump = [&]()
    {
        if (!SubtEnemyYPos())
        {
            return; // enemy vertical coord + 62 < 68, leave
        }
        if (static_cast<uint8_t>(M(Enemy_Y_Speed + x) + 0x02) < 0x03)
        {
            return;
        }
        ChkUnderEnemy(); // check to see if green paratroopa is standing on anything
        if (a == 0)
        {
            return; // it is not, leave
        }
        if (ChkForNonSolids(a))
        {
            return; // what it is standing on is not solid, leave
        }
        EnemyLanding();                     // change vertical coordinate and speed
        writeData(Enemy_Y_Speed + x, 0xfd); // make the paratroopa jump again
    };
    jump();

    // DoSide: check for horizontal blockage, then leave
    DoEnemySideCheck();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (the slot the area-parser task loop is currently on)
// Outputs: none
void SMBEngine::EnemiesAndLoopsCore()
{
    const uint8_t enemyFlag = M(Enemy_Flag + x); // check data here for MSB set

    if ((enemyFlag & 0x80) != 0)
    {
        // ChkBowserF: the low nybble points at a second enemy flag
        const uint8_t otherFlagOffset = enemyFlag & 0b00001111; // mask out high nybble
        if (M(Enemy_Flag + otherFlagOffset) != 0)
        {
            return;
        }
        writeData(Enemy_Flag + x, 0x00); // if second enemy flag not set, also clear first one
        return;                          // ExitELCore
    }
    if (enemyFlag == 0)
    {
        // ChkAreaTsk: check number of tasks to perform
        if ((M(AreaParserTaskNum) & 0x07) == 0x07)
        {
            return;
        }
        ProcLoopCommand(); // otherwise, jump to process loop command/load enemies
        return;
    }

    // RunEnemyObjectsCore
    x = M(ObjectOffset); // get offset for enemy object buffer
    a = 0x00;            // load value 0 for jump engine by default
    y = M(Enemy_ID + x);
    if (y >= 0x15)
    {
        a = y;     // otherwise subtract $14 from the value and use
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

// Inputs: x = enemy object buffer offset (slot that will receive the next-parsed enemy object)
// Outputs: none
void SMBEngine::ProcLoopCommand()
{
    const uint8_t LoopCmdYPosition_data[] = {0x40, 0xb0, 0xb0, 0x80, 0x40, 0x40, 0x80, 0x40, 0xf0, 0xf0, 0xf0};

    const uint8_t LoopCmdPageNumber_data[] = {0x05, 0x09, 0x04, 0x05, 0x06, 0x08, 0x09, 0x0a, 0x06, 0x0b, 0x10};

    const uint8_t LoopCmdWorldNumber_data[] = {0x03, 0x03, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07};

    uint32_t wide = 0;

ProcLoopCommand:
    // check if loop command was found
    if (M(LoopCommand) == 0)
    {
        goto ChkEnemyFrenzy;
    }
    // check to see if we're still on the first page
    if (M(CurrentColumnPos) != 0)
    {
        goto ChkEnemyFrenzy; // if not, do not loop yet
    }
    y = 0x0b; // start at the end of each set of loop data

FindLoop:
    --y;
    if ((y & 0x80) != 0)
    {
        goto ChkEnemyFrenzy; // if all data is checked and not match, do not loop
    }
    // check to see if one of the world numbers
    if (M(WorldNumber) != LoopCmdWorldNumber_data[y])
    {
        goto FindLoop;
    }
    // check to see if one of the page numbers
    if (M(CurrentPageLoc) != LoopCmdPageNumber_data[y])
    {
        goto FindLoop;
    }
    // check to see if the player is at the correct position
    if (M(Player_Y_Position) != LoopCmdYPosition_data[y])
    {
        goto WrongChk;
    }
    // check to see if the player is
    if (M(Player_State) != 0x00)
    {
        goto WrongChk; // if not, player fails to pass loop, and loopback
    }
    // are we in world 7? (check performed on correct
    if (M(WorldNumber) != World7)
    {
        goto InitMLp; // if not, initialize flags used there, otherwise
    }
    ++M(MultiLoopCorrectCntr); // increment counter for correct progression

IncMLoop: // increment master multi-part counter
    ++M(MultiLoopPassCntr);
    // have we done all three parts?
    if (M(MultiLoopPassCntr) == 0x03)
    {                                // if not, skip this part
        a = M(MultiLoopCorrectCntr); // if so, have we done them all correctly?
        if (a == 0x03)
        {
            goto InitMLp; // if so, branch past unnecessary check here
        }
        if (a == 0x03)
        { // unconditional branch if previous branch fails

        WrongChk: // are we in world 7? (check performed on
            if (M(WorldNumber) == World7)
            {
                goto IncMLoop;
            }
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
        goto CheckFrenzyBuffer; // if found, jump to check frenzy buffer, otherwise
    } // CheckEndofBuffer
    a &= 0b00001111; // check for special row $0e
    if (a == 0x0e)
    {
        goto CheckRightBounds; // if found, branch, otherwise
    }
    if (x < 0x05)
    {
        goto CheckRightBounds; // if not at end of buffer, branch
    }
    ++y;
    // check for specific value here
    a = M(W(EnemyData) + y) & 0b00111111; // not sure what this was intended for, exactly
    if (a == 0x2e)
    {
        goto CheckRightBounds; // but it has the effect of keeping enemies out of
    }
    return; // the sixth slot

    //------------------------------------------------------------------------

CheckRightBounds:
    wide = ((M(ScreenRight_PageLoc) << 8) | M(ScreenRight_X_Pos)) + 0x30; // add 48 to pixel coordinate of right boundary
    a = LOBYTE(wide) & 0b11110000;                                        // store high nybble
    writeData(0x07, a);
    a = HIBYTE(wide);   // add carry to page location of right boundary
    writeData(0x06, a); // store page location + carry
    y = M(EnemyDataOffset);
    ++y;
    a = M(W(EnemyData) + y); // if MSB of enemy object is clear, branch to check for row $0f
    a <<= 1;
    if ((M(W(EnemyData) + y) & 0x80) == 0)
    {
        goto CheckPageCtrlRow;
    }
    // if page select already set, do not set again
    if (M(EnemyObjectPageSel) != 0)
    {
        goto CheckPageCtrlRow;
    }
    ++M(EnemyObjectPageSel); // otherwise, if MSB is set, set page select
    ++M(EnemyObjectPageLoc); // and increment page control

CheckPageCtrlRow:
    --y;
    // reread first byte
    a = M(W(EnemyData) + y) & 0x0f;
    if (a != 0x0f)
    {
        goto PositionEnemyObj; // if not found, branch to position enemy object
    }
    // if page select set,
    if (M(EnemyObjectPageSel) != 0)
    {
        goto PositionEnemyObj; // branch without reading second byte
    }
    ++y;
    // otherwise, get second byte, mask out 2 MSB
    a = M(W(EnemyData) + y) & 0b00111111;
    writeData(EnemyObjectPageLoc, a); // store as page control for enemy object data
    ++M(EnemyDataOffset);             // increment enemy object data offset 2 bytes
    ++M(EnemyDataOffset);
    ++M(EnemyObjectPageSel); // set page select for enemy object data and
    goto ProcLoopCommand;    // jump back to process loop commands again

PositionEnemyObj:
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
            goto ParseRow0e;
        }
    } // CheckRightExtBounds
    else // if not found, unconditional jump
    {
        if (((M(0x06) << 8) | M(0x07)) // check right boundary + 48 against the column position
            < ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x)))
        {
            goto CheckFrenzyBuffer; // if enemy object beyond extended boundary, branch
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
            goto ParseRow0e; // (necessary if branched to $c1cb)
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
        if (a >= 0x37)
        {
            if (a < 0x3f)
            {
                goto DoGroup; // below $3f, branch if below $3f
            }
        } // BuzzyBeetleMutate
        if (a != Goomba)
        {
            goto StrID; // value ($3f or more always fails)
        }
        // check if primary hard mode flag is set
        if (M(PrimaryHardMode) == 0)
        {
            goto StrID; // and if so, change goomba to buzzy beetle
        }
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
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::InitEnemyObject()
{
    a = 0x00; // initialize enemy state
    writeData(Enemy_State + x, 0x00);
    CheckpointEnemyID(); // jump ahead to run jump engine and subroutines

    // ExEPar: then leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: none
void SMBEngine::CheckpointEnemyID()
{
    const uint8_t SwimCC_IDData_data[] = {0x0a, 0x0b};

    const uint8_t Enemy17YPosData_data[] = {0x40, 0x30, 0x90, 0x50, 0x20, 0x60, 0xa0, 0x70};

CheckpointEnemyID:
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
        InitPiranhaPlant(x);
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
    {
        return;
    }
    a = M(AreaType); // are we in a water-type level?
    if (a != 0)
    {
        goto DoBulletBills;
    }

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
    a = M(Bitmasks + y);          // load bitmask
    if ((a & M(BitMFilter)) != 0) // perform AND on filter without changing it
    {
        ++y; // increment offset
        a = y;
        a &= 0b00000111; // mask out all but 3 LSB thus keeping it 0-7
        goto ChkRBit;    // do another check
    } // AddFBit: add bit to already set bits in filter
    a |= M(BitMFilter);
    writeData(BitMFilter, a);          // and store
    a = Enemy17YPosData_data[y];       // load vertical position using offset
    PutAtRightExtent();                // set vertical position and other values
    writeData(Enemy_YMF_Dummy + x, a); // initialize dummy variable
    a = 0x20;                          // set timer
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
        {
            goto BB_SLoop; // loop back and check another slot
        }
        a = M(Enemy_ID + y);
        if (a != BulletBill_FrenzyVar)
        {
            goto BB_SLoop; // bullet bill object (frenzy variant)
        }

        return; // ExF17: if found, leave

        //------------------------------------------------------------------------
    } // FireBulletBill
    a = M(Square2SoundQueue) | Sfx_Blast; // play fireworks/gunfire sound
    writeData(Square2SoundQueue, a);
    a = BulletBill_FrenzyVar; // load identifier for bullet bill object
    goto Set17ID;             // unconditional branch

InitEnemyFrenzy:
    a = M(Enemy_ID + x);             // load enemy identifier
    writeData(EnemyFrenzyBuffer, a); // save in enemy frenzy buffer
    a -= 0x12;                       // subtract 12 and use as offset for jump engine
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
