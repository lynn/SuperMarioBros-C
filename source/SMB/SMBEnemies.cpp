// The EnemiesAndLoopsCore subsystem: everything EnemiesAndLoopsCore() reaches that nothing
// outside it reaches, and so nothing outside it needs.
//
// Moved out of SMB.cpp by tools/split.py. These are methods of SMBEngine like every other
// routine of the game and are declared in SMBEngine.hpp; the file they are written in is the
// only thing that is different, and tools/layers.py is what keeps it meaning something.
//
#include "SMB.hpp"

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
