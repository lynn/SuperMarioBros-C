// The object kernel: the sprite, collision, gravity and offscreen primitives that
// every actor in the game is built out of. It is not a subsystem -- being reached
// from several of them at once is the whole of what it is -- so no subsystem owns
// it, and it calls nothing above itself.
//
// Moved out of SMB.cpp by tools/split.py. These are methods of SMBEngine like every other
// routine of the game and are declared in SMBEngine.hpp; the file they are written in is the
// only thing that is different, and tools/layers.py is what keeps it meaning something.
//
#include "SMB.hpp"


//------------------------------------------------------------------------

void SMBEngine::DigitsMathRoutine()
{
    // check mode of operation
    if (M(OperMode) != TitleScreenModeValue)
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
        writeData(DigitModifier - 1 + x, 0x00);
        --x;
    } while ((x & 0x80) == 0); // do this until they're all reset, then leave
    return;

//------------------------------------------------------------------------

BorrowOne: // decrement the previous digit, then put $09 in
    --M(DigitModifier - 1 + x);
    a = 0x09; // the game timer digit we're currently on to "borrow
    goto StoreNewD; // the one", then do an unconditional branch back

CarryOne: // subtract ten from our digit to make it a
    a -= 10; // proper BCD number, then increment the digit
    ++M(DigitModifier - 1 + x); // preceding current digit to "carry the one" properly
    goto StoreNewD; // go back to just after we branched here
}

//------------------------------------------------------------------------

void SMBEngine::KillEnemies()
{
    writeData(0x00, a); // store identifier here
    a = 0x00;
    x = 0x04; // check for identifier in enemy object buffer

    do // KillELoop
    {
        y = M(Enemy_ID + x);
        if (y == M(0x00))
        {
            writeData(Enemy_Flag + x, 0x00); // if found, deactivate enemy object flag
        } // NoKillE: do this until all slots are checked
        --x;
    } while ((x & 0x80) == 0);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetBlockBufferAddr()
{
    pha(); // take value of A, save
    a >>= 1; // move high nybble to low
    a >>= 1;
    a >>= 1;
    a >>= 1;
    y = a; // use nybble as pointer to high byte
    // of indirect here
    writeData(0x07, M(BlockBufferAddr + 2 + y));
    pla();
    a &= 0b00001111; // pull from stack, mask out high nybble
    a += M(BlockBufferAddr + y); // add to low byte
    writeData(0x06, a); // store here and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetScreenPosition()
{
    uint32_t wide = 0;

    // get coordinate of screen's left boundary
    wide = ((M(ScreenLeft_PageLoc) << 8) | M(ScreenLeft_X_Pos)) + 0xff; // add 255 pixels
    writeData(ScreenRight_X_Pos, LOBYTE(wide)); // store as coordinate of screen's right boundary
    writeData(ScreenRight_PageLoc, HIBYTE(wide)); // store as page number where right boundary is
    a = HIBYTE(wide); // get page number where left boundary is
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitBlock_XY_Pos()
{
    uint32_t wide = 0;

    wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + 0x08; // add eight pixels
    a = LOBYTE(wide) & 0xf0; // mask out low nybble to give 16-pixel correspondence
    writeData(Block_X_Position + x, a); // save as horizontal coordinate for block object
    a = HIBYTE(wide); // the page location of the player, plus the carry
    writeData(Block_PageLoc + x, a); // save as page location of block object
    writeData(Block_PageLoc2 + x, a); // save elsewhere to be used later
    a = M(Player_Y_HighPos);
    writeData(Block_Y_HighPos + x, a); // save vertical high byte of player into
    return; // vertical high byte of block object and leave
}

//------------------------------------------------------------------------

bool SMBEngine::CheckPlayerVertical()
{
    bool playerVerticalOutOfRange = false;

    a = M(Player_OffscreenBits); // if player object is completely offscreen
    if (a >= 0xf0)
    {
        playerVerticalOutOfRange = true;
        return playerVerticalOutOfRange;
    }
    y = M(Player_Y_HighPos); // if player high vertical byte is not
    --y; // within the screen, leave this routine
    if (y != 0)
    {
        playerVerticalOutOfRange = false;
        return playerVerticalOutOfRange;
    }
    a = M(Player_Y_Position); // if on the screen, check to see how far down
    playerVerticalOutOfRange = a >= 0xd0; // the player is vertically

    return playerVerticalOutOfRange; // ExCPV
}

//------------------------------------------------------------------------

bool SMBEngine::PlayerEnemyDiff()
{
    uint32_t wide = 0;
    bool enemyRightOfPlayer = false;

        // get the distance between the enemy object and the player, each one 16-bit page:coordinate
        wide = ((M(Enemy_PageLoc + x) << 8) | M(Enemy_X_Position + x))
             - ((M(Player_PageLoc) << 8) | M(Player_X_Position));
        writeData(0x00, LOBYTE(wide)); // and store here
        enemyRightOfPlayer = (wide & 0x10000) == 0; // the subtraction did not borrow
        a = HIBYTE(wide); // then leave
        return enemyRightOfPlayer;
}

//------------------------------------------------------------------------

void SMBEngine::ChkForNonSolids()
{
    if (a == 0x26)
        return;
    if (a == 0xc2)
        return;
    if (a == 0xc3)
        return;
    if (a == 0x5f)
        return;

    return; // NSFnd
}

//------------------------------------------------------------------------

void SMBEngine::BoundingBoxCore()
{
    writeData(0x00, x); // save offset here
    // store object coordinates relative to screen
    writeData(0x02, M(SprObject_Rel_YPos + y)); // vertically and horizontally, respectively
    writeData(0x01, M(SprObject_Rel_XPos + y));
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
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetObjRelativePosition()
{
    // load vertical coordinate low
    writeData(SprObject_Rel_YPos + y, M(SprObject_Y_Position + x)); // store here
    a = M(SprObject_X_Position + x); // load horizontal coordinate
    a -= M(ScreenLeft_X_Pos);
    writeData(SprObject_Rel_XPos + y, a); // store result here
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DividePDiff()
{
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
    return;
}

//------------------------------------------------------------------------

void SMBEngine::EnemyFacePlayer()
{
    bool enemyRightOfPlayer = false;

    y = 0x01; // set to move right by default
    enemyRightOfPlayer = PlayerEnemyDiff(); // get horizontal difference between player and enemy
    if ((a & 0x80) != 0)
    { // if enemy is to the right of player, do not increment
        ++y; // otherwise, increment to set to move to the left
    } // SFcRt: set moving direction here
    writeData(Enemy_MovingDir + x, y);
    --y; // then decrement to use as a proper offset
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RunOffscrBitsSubs()
{
    uint32_t wide = 0;

    GetXOffscreenBits(); // do subroutine here
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
    // if no branching, load value here and store
    writeData(0x06, 0x38);
    a = 0x08; // load some other value and execute subroutine
    DividePDiff();

XLdBData: // get bits here
    a = M(XOffscreenBitsData + x);
    x = M(0x04); // reobtain position in buffer
    if (a == 0x00)
    {
        --y; // otherwise, do left side of screen now
        if ((y & 0x80) == 0)
            goto XOfsLoop; // branch if not already done with left side
    } // ExXOfsBS
    return;

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
    // if no branching, load value here and store
    writeData(0x06, 0x20);
    a = 0x04; // load some other value and execute subroutine
    DividePDiff();

YLdBData: // get offscreen data bits using offset
    a = M(YOffscreenBitsData + x);
    x = M(0x04); // reobtain position in buffer
    if (a == 0x00)
    { // if bits not zero, branch to leave
        --y; // otherwise, do bottom of the screen now
        if ((y & 0x80) == 0)
            goto YOfsLoop;
    } // ExYOfsBS
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetXOffscreenBits()
{
    uint32_t wide = 0;

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
    // if no branching, load value here and store
    writeData(0x06, 0x38);
    a = 0x08; // load some other value and execute subroutine
    DividePDiff();

XLdBData: // get bits here
    a = M(XOffscreenBitsData + x);
    x = M(0x04); // reobtain position in buffer
    if (a == 0x00)
    {
        --y; // otherwise, do left side of screen now
        if ((y & 0x80) == 0)
            goto XOfsLoop; // branch if not already done with left side
    } // ExXOfsBS
    return;
}

//------------------------------------------------------------------------

void SMBEngine::Setup_Vine()
{
    // load identifier for vine object
    writeData(Enemy_ID + x, VineObject); // store in buffer
    writeData(Enemy_Flag + x, 0x01); // set flag for enemy object buffer
    writeData(Enemy_PageLoc + x, M(Block_PageLoc + y)); // copy page location from previous object
    writeData(Enemy_X_Position + x, M(Block_X_Position + y)); // copy horizontal coordinate from previous object
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
    writeData(Square2SoundQueue, Sfx_GrowVine); // load vine grow sound
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ImposeGravity()
{
    uint32_t wide = 0;

    pha(); // push value to stack
    y = 0x00; // set Y to zero by default
    // get current vertical speed
    if ((M(SprObject_Y_Speed + x) & 0x80) != 0)
    { // if currently moving downwards, do not decrement Y
        y = 0xff; // otherwise decrement Y
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
    wide = ((M(SprObject_Y_Speed + x) << 8) | M(SprObject_Y_MoveForce + x)) + M(0x00); // add downward movement amount to contents of $0433
    writeData(SprObject_Y_MoveForce + x, LOBYTE(wide));
    writeData(SprObject_Y_Speed + x, HIBYTE(wide));
    a = HIBYTE(wide); // add carry to vertical speed and store
    if (((a - M(0x02)) & 0x80) != 0)
        goto ChkUpM; // if less than preset value, skip this part
    if (M(SprObject_Y_MoveForce + x) < 0x80)
        goto ChkUpM;
    writeData(SprObject_Y_Speed + x, M(0x02)); // keep vertical speed within maximum value
    a = 0x00;
    writeData(SprObject_Y_MoveForce + x, 0x00); // clear fractional

ChkUpM: // get value from stack
    pla();
    if (a == 0)
        return; // if set to zero, branch to leave
    a = M(0x02) ^ 0b11111111; // otherwise get two's compliment of maximum speed
    y = a;
    ++y;
    writeData(0x07, y); // store two's compliment here
    wide = ((M(SprObject_Y_Speed + x) << 8) | M(SprObject_Y_MoveForce + x)) - M(0x01); // of movement force, note that $01 is twice as large as $00,
    writeData(SprObject_Y_MoveForce + x, LOBYTE(wide)); // thus it effectively undoes add we did earlier
    writeData(SprObject_Y_Speed + x, HIBYTE(wide));
    a = HIBYTE(wide);
    if (((a - M(0x07)) & 0x80) == 0)
        return; // if less negatively than preset maximum, skip this part
    a = M(SprObject_Y_MoveForce + x);
    if (a >= 0x80)
        return; // and if so, branch to leave
    writeData(SprObject_Y_Speed + x, M(0x07)); // keep vertical speed within maximum value
    a = 0xff;
    writeData(SprObject_Y_MoveForce + x, 0xff); // clear fractional

    return; // ExVMove: leave!
}

//------------------------------------------------------------------------

void SMBEngine::ImpedePlayerMove()
{
    uint32_t wide = 0;

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
    writeData(SideCollisionTimer, 0x10); // set timer of some sort
    y = 0x00;
    writeData(Player_X_Speed, 0x00); // nullify player's horizontal speed
    if ((a & 0x80) != 0)
    { // branch ahead, do not decrement Y
        y = 0xff; // otherwise decrement Y now
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
    return;
}

//------------------------------------------------------------------------

void SMBEngine::CheckRightScreenBBox()
{
    uint32_t wide = 0;

    // add 128 pixels to left side of screen
    wide = ((M(ScreenLeft_PageLoc) << 8) | M(ScreenLeft_X_Pos)) + 0x80;
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
            // check left-side edge of bounding box for offscreen
            if ((M(BoundingBox_UL_XPos + y) & 0x80) == 0)
            { // coordinates, and branch if still on the screen
                writeData(BoundingBox_UL_XPos + y, 0xff); // store offscreen value for left side
            } // SORte: store offscreen value for right side
            writeData(BoundingBox_DR_XPos + y, 0xff);
        } // NoOfs: get object offset and leave
        x = M(ObjectOffset);
        return;

    //------------------------------------------------------------------------
    } // CheckLeftScreenBBox
    a = M(BoundingBox_UL_XPos + y); // check left-side edge of bounding box for offscreen
    if ((a & 0x80) == 0)
        goto NoOfs2; // coordinates, and branch if still on the screen
    if (a < 0xa0)
        goto NoOfs2; // screen or really offscreen, and branch if still on
    a = 0x00;
    // check right-side edge of bounding box for offscreen
    if ((M(BoundingBox_DR_XPos + y) & 0x80) != 0)
    { // coordinates, branch if still onscreen
        writeData(BoundingBox_DR_XPos + y, 0x00); // store offscreen value for right side
    } // SOLft: store offscreen value for left side
    writeData(BoundingBox_UL_XPos + y, 0x00);

NoOfs2: // get object offset and leave
    x = M(ObjectOffset);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawSpriteObject()
{
    bool shiftedBit = false;

    // get saved flip control bits
    a = M(0x03) >> 1;
    shiftedBit = (a & 0x01) != 0;
    a = M(0x00);
    if (shiftedBit)
    { // if d1 not set, branch
        writeData(Sprite_Tilenumber + 4 + y, a); // store first tile into second sprite
        // and second into first sprite
        writeData(Sprite_Tilenumber + y, M(0x01));
        a = 0x40; // activate horizontal flip OAM attribute
        goto SetHFAt; // and unconditionally branch
    } // NoHFlip: store first tile into first sprite
    writeData(Sprite_Tilenumber + y, a);
    // and second into second sprite
    writeData(Sprite_Tilenumber + 4 + y, M(0x01));
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
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InitPiranhaPlant()
{
    // set initial speed
    writeData(PiranhaPlant_Y_Speed + x, 0x01);
    writeData(Enemy_State + x, 0x00); // initialize enemy state and what would normally
    writeData(PiranhaPlant_MoveFlag + x, 0x00); // be used as vertical speed, but not in this case
    a = M(Enemy_Y_Position + x);
    writeData(PiranhaPlantDownYPos + x, a); // save original vertical coordinate here
    a -= 0x18;
    writeData(PiranhaPlantUpYPos + x, a); // save original vertical coordinate - 24 pixels here
    a = 0x09;
    SetBBox2(); // set specific value for bounding box control
    return;
}

//------------------------------------------------------------------------

// set bounding box control then leave
void SMBEngine::SetBBox2()
{
    writeData(Enemy_BoundBoxCtrl + x, a);
    return;
}

//------------------------------------------------------------------------

// initialize vertical speed
void SMBEngine::InitVStf()
{
    a = 0x00;
    writeData(Enemy_Y_Speed + x, 0x00); // and movement force
    writeData(Enemy_Y_MoveForce + x, 0x00);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetPlayerColors()
{
    x = M(VRAM_Buffer1_Offset); // get current buffer offset
    y = 0x00;
    // check which player is on the screen
    if (M(CurrentPlayer) != 0)
    {
        y = 0x04; // load offset for luigi
    } // ChkFiery: check player status
    if (M(PlayerStatus) == 0x02)
    { // if fiery, load alternate offset for fiery player
        y = 0x08;
    } // StartClrGet: do four colors
    a = 0x03;
    writeData(0x00, 0x03);

    do // ClrGetLoop: fetch player colors and store them
    {
        writeData(VRAM_Buffer1 + 3 + x, M(PlayerColors + y)); // in the buffer
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
    writeData(VRAM_Buffer1 + 3 + x, M(BackgroundColors + y));
    // set for sprite palette address
    writeData(VRAM_Buffer1 + x, 0x3f); // save to buffer
    writeData(VRAM_Buffer1 + 1 + x, 0x10);
    // write length byte to buffer
    writeData(VRAM_Buffer1 + 2 + x, 0x04);
    // now the null terminator
    writeData(VRAM_Buffer1 + 7 + x, 0x00);
    a = x; // move the buffer pointer ahead 7 bytes
    a += 0x07;

    SetVRAMOffset();
    return;
}

//------------------------------------------------------------------------

// store as new vram buffer offset
void SMBEngine::SetVRAMOffset()
{
    writeData(VRAM_Buffer1_Offset, a);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::WriteBlockMetatile()
{
    y = 0x03; // load offset for blank metatile
    if (a == 0x00)
        goto UseBOffset; // branch if found (unconditional if branched from 8a6b)
    y = 0x00; // load offset for brick metatile w/ line
    if (a == 0x58)
        goto UseBOffset; // use offset if metatile is brick with coins (w/ line)
    if (a == 0x51)
        goto UseBOffset; // use offset if metatile is breakable brick w/ line
    y = 0x01; // increment offset for brick metatile w/o line
    if (a == 0x5d)
        goto UseBOffset; // use offset if metatile is brick with coins (w/o line)
    if (a == 0x52)
        goto UseBOffset; // use offset if metatile is breakable brick w/o line
    y = 0x02; // if any other metatile, increment offset for empty block

UseBOffset: // put Y in A
    a = y;
    y = M(VRAM_Buffer1_Offset); // get vram buffer offset
    ++y; // move onto next byte
    PutBlockMetatile(); // get appropriate block data and write to vram buffer

    MoveVOffset();
    return;
}

//------------------------------------------------------------------------

// decrement vram buffer offset
void SMBEngine::MoveVOffset()
{
    --y;
    a = y; // add 10 bytes to it
    a += 10;
    SetVRAMOffset(); // branch to store as new vram buffer offset
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PutBlockMetatile()
{
    uint32_t wide = 0;

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

    RemBridge();
    return;
}

//------------------------------------------------------------------------

// write top left and top right
void SMBEngine::RemBridge()
{
    writeData(VRAM_Buffer1 + 2 + y, M(BlockGfxData + x)); // tile numbers into first spot
    writeData(VRAM_Buffer1 + 3 + y, M(BlockGfxData + 1 + x));
    // write bottom left and bottom
    writeData(VRAM_Buffer1 + 7 + y, M(BlockGfxData + 2 + x)); // right tiles numbers into
    // second spot
    writeData(VRAM_Buffer1 + 8 + y, M(BlockGfxData + 3 + x));
    a = M(0x04);
    writeData(VRAM_Buffer1 + y, a); // write low byte of name table
    a += 0x20; // add 32 bytes to value
    writeData(VRAM_Buffer1 + 5 + y, a); // write low byte of name table
    a = M(0x05); // plus 32 bytes into second slot
    writeData(VRAM_Buffer1 - 1 + y, a); // write high byte of name
    writeData(VRAM_Buffer1 + 4 + y, a); // table address to both slots
    writeData(VRAM_Buffer1 + 1 + y, 0x02); // put length of 2 in
    writeData(VRAM_Buffer1 + 6 + y, 0x02); // both slots
    a = 0x00;
    writeData(VRAM_Buffer1 + 9 + y, 0x00); // put null terminator at end
    x = M(0x00); // get offset control bit here
    return; // and leave
}

//------------------------------------------------------------------------

void SMBEngine::PrintStatusBarNumbers()
{
    writeData(0x00, a); // store player-specific offset
    OutputNumbers(); // use first nybble to print the coin display
    // move high nybble to low
    a = M(0x00) >> 4; // and print to score display

    OutputNumbers();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::OutputNumbers()
{
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
        // write low vram address and length of thing
        writeData(VRAM_Buffer1 + 1 + x, M(StatusBarData + y)); // we're printing to the buffer
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
            writeData(VRAM_Buffer1 + 3 + x, M(DisplayDigits + y));
            ++x;
            ++y;
            --M(0x03); // do this until all the digits are written
        } while (M(0x03) != 0);
        a = 0x00; // put null terminator at end
        writeData(VRAM_Buffer1 + 3 + x, 0x00);
        ++x; // increment buffer pointer by 3
        ++x;
        ++x;
        writeData(VRAM_Buffer1_Offset, x); // store it in case we want to use it again
    } // ExitOutputN
    return;
}

//------------------------------------------------------------------------

// set X for player offset
void SMBEngine::ChkPOffscr()
{
    bool shiftedBit = false;
    uint32_t wide = 0;

    x = 0x00;
    GetXOffscreenBits(); // get horizontal offscreen bits for player
    writeData(0x00, a); // save them here
    y = 0x00; // load default offset (left side)
    shiftedBit = (a & 0x80) != 0;
    if (!shiftedBit) // if d7 of offscreen bits are set,
    { // branch with default offset
        y = 0x01; // otherwise use different offset (right side)
        a = M(0x00) & 0b00100000; // check offscreen bits for d5 set
        if (a == 0)
            goto InitPlatScrl; // if not set, branch ahead of this part
    } // KeepOnscr: get left or right side coordinate based on offset
    wide = ((M(ScreenEdge_PageLoc + y) << 8) | M(ScreenEdge_X_Pos + y)) - M(X_SubtracterData + y); // subtract amount based on offset
    writeData(Player_X_Position, LOBYTE(wide)); // store as player position to prevent movement further
    writeData(Player_PageLoc, HIBYTE(wide)); // save as player's page location
    a = HIBYTE(wide); // get left or right page location based on offset
    // check saved controller bits
    if (M(Left_Right_Buttons) == M(OffscrJoypadBitsData + y))
        goto InitPlatScrl; // if not equal, branch
    a = 0x00;
    writeData(Player_X_Speed, 0x00); // otherwise nullify horizontal speed of player

InitPlatScrl: // nullify platform force imposed on scroll
    a = 0x00;
    writeData(Platform_X_Scroll, 0x00);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::AddToScore()
{
    x = M(CurrentPlayer); // get current player
    y = M(ScoreOffsets + x); // get offset for player's score
    DigitsMathRoutine(); // update the score internally with value in digit modifier

    GetSBNybbles();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetSBNybbles()
{
    y = M(CurrentPlayer); // get current player
    a = M(StatusBarNybbles + y); // get nybbles based on player, use to update score and coins

    UpdateNumber();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::UpdateNumber()
{
    PrintStatusBarNumbers(); // print status bar numbers based on nybbles, whatever they be
    y = M(VRAM_Buffer1_Offset);
    a = M(VRAM_Buffer1 - 6 + y); // check highest digit of score
    if (a == 0)
    { // if zero, overwrite with space tile for zero suppression
        a = 0x24;
        writeData(VRAM_Buffer1 - 6 + y, 0x24);
    } // NoZSup: get enemy object buffer offset
    x = M(ObjectOffset);
    return;
}

//------------------------------------------------------------------------

// this is a residual jump point in enemy object jump table
void SMBEngine::PwrUpJmp()
{
    writeData(Enemy_State + 5, 0x01); // set power-up object's state
    writeData(Enemy_Flag + 5, 0x01); // set buffer flag
    writeData(Enemy_BoundBoxCtrl + 5, 0x03); // set bounding box size control for power-up object
    if (M(PowerUpType) < 0x02)
    { // if star or 1-up, branch ahead
        a = M(PlayerStatus); // otherwise check player's current status
        if (a >= 0x02)
        { // if player not fiery, use status as power-up type
            a >>= 1; // otherwise shift right to force fire flower type
        } // StrType: store type here
        writeData(PowerUpType, a);
    } // PutBehind
    writeData(Enemy_SprAttrib + 5, 0b00100000); // set background priority bit
    a = Sfx_GrowPowerUp;
    writeData(Square2SoundQueue, Sfx_GrowPowerUp); // load power-up reveal sound and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ImposeGravitySprObj()
{
    writeData(0x02, a); // set maximum speed here
    a = 0x00; // set value to move downwards
    ImposeGravity(); // jump to the code that actually moves it
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetEnemyBoundBoxOfs()
{
    a = M(ObjectOffset); // get enemy object buffer offset

    GetEnemyBoundBoxOfsArg();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetEnemyBoundBoxOfsArg()
{
    a <<= 1; // multiply A by four, then add four
    a <<= 1; // to skip player's bounding box
    a += 0x04;
    y = a; // send to Y
    // get offscreen bits for enemy object
    a = M(Enemy_OffscreenBits) & 0b00001111; // save low nybble
    return;
}

//------------------------------------------------------------------------

void SMBEngine::SetupEOffsetFBBox()
{
    a = x; // add 1 to offset to properly address
    a += 0x01;
    x = a;
    y = 0x01; // load 1 as offset here, same reason
    BoundingBoxCore(); // do a sub to get the coordinates of the bounding box
    CheckRightScreenBBox(); // jump to handle offscreen coordinates of bounding box
    return;
}

//------------------------------------------------------------------------

// do collision detection subroutine for sprite object
void SMBEngine::BBChk_E()
{
    BlockBufferCollision();
    x = M(ObjectOffset); // get object offset
    return;
}

//------------------------------------------------------------------------

void SMBEngine::BlockBufferCollision()
{
    uint32_t wide = 0;

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
    GetBlockBufferAddr(); // get address of block buffer into $06, $07
    y = M(0x04); // get old contents of Y
    a = M(SprObject_Y_Position + x); // get vertical coordinate of object
    a += M(BlockBuffer_Y_Adder + y); // add it to value obtained using Y as offset
    a &= 0b11110000; // mask out low nybble
    a -= 0x20; // subtract 32 pixels for the status bar
    writeData(0x02, a); // store result here
    y = a; // use as offset for block buffer
    // check current content of block buffer
    writeData(0x03, M(W(0x06) + y)); // and store here
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
    return; // and leave
}

//------------------------------------------------------------------------

void SMBEngine::DrawOneSpriteRow()
{
    writeData(0x01, a);
    DrawSpriteObject(); // draw them
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawFirebar()
{
    bool shiftedBit = false;

    // get frame counter
    a = M(FrameCounter) >> 2; // divide by four
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
        a = 0b11000010; // otherwise flip both ways every eight frames
    } // FireA: store attribute byte and leave
    writeData(Sprite_Attributes + y, a);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::RelativePlayerPosition()
{
    x = 0x00; // set offsets for relative cooordinates
    y = 0x00; // routine to correspond to player object
    RelWOfs(); // get the coordinates
    return;
}

//------------------------------------------------------------------------

// get the coordinates
void SMBEngine::RelWOfs()
{
    GetObjRelativePosition();
    x = M(ObjectOffset); // return original offset
    return; // leave
}

//------------------------------------------------------------------------

void SMBEngine::RelativeEnemyPosition()
{
    a = 0x01; // get coordinates of enemy object
    y = 0x01; // relative to the screen
    VariableObjOfsRelPos();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::VariableObjOfsRelPos()
{
    writeData(0x00, x); // store value to add to A here
    a += M(0x00); // add A to value stored
    x = a; // use as enemy offset
    GetObjRelativePosition();
    x = M(ObjectOffset); // reload old object offset and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetPlayerOffscreenBits()
{
    x = 0x00; // set offsets for player-specific variables
    y = 0x00; // and get offscreen information about player
    GetOffScreenBitsSet();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetEnemyOffscreenBits()
{
    a = 0x01; // set A to add 1 byte in order to get enemy offset
    y = 0x01; // set Y to put offscreen bits in Enemy_OffscreenBits
    SetOffscrBitsOffset();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::SetOffscrBitsOffset()
{
    writeData(0x00, x);
    a += M(0x00); // appropriate offset, then give back to X
    x = a;

    GetOffScreenBitsSet();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetOffScreenBitsSet()
{
    a = y; // save offscreen bits offset to stack for now
    pha();
    RunOffscrBitsSubs();
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
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveEnemyHorizontally()
{
    ++x; // increment offset for enemy offset
    MoveObjectHorizontally(); // position object horizontally according to
    x = M(ObjectOffset); // counters, return with saved value in A,
    return; // put enemy offset back in X and leave
}

//------------------------------------------------------------------------

void SMBEngine::MoveObjectHorizontally()
{
    uint32_t wide = 0;
    bool carry = false;

    a = M(SprObject_X_Speed + x); // get currently saved value (horizontal
    a <<= 1; // speed, secondary counter, whatever)
    a <<= 1; // and move low nybble to high
    a <<= 1;
    a <<= 1;
    writeData(0x01, a); // store result here
    // get saved value again
    a = M(SprObject_X_Speed + x) >> 4; // move high nybble to low
    if (a >= 0x08)
    {
        a |= 0b11110000; // otherwise alter high nybble
    } // SaveXSpd: save result here
    writeData(0x00, a);
    y = 0x00; // load default Y value here
    if ((a & 0x80) != 0)
    {
        y = 0xff; // otherwise decrement Y
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

    return; // ExXMove: and leave
}

//------------------------------------------------------------------------

void SMBEngine::SetupFloateyNumber()
{
    writeData(FloateyNum_Control + x, a); // set number of points control for floatey numbers
    writeData(FloateyNum_Timer + x, 0x30); // set timer for floatey numbers
    writeData(FloateyNum_Y_Pos + x, M(Enemy_Y_Position + x)); // set vertical coordinate
    a = M(Enemy_Rel_XPos);
    writeData(FloateyNum_X_Pos + x, a); // set horizontal coordinate and leave

    return; // ExSFN
}

//------------------------------------------------------------------------

bool SMBEngine::PlayerCollisionCore()
{
    bool collisionFound = false;

    x = 0x00; // initialize X to use player's bounding box for comparison
    collisionFound = SprObjectCollisionCore();
    return collisionFound;
}

//------------------------------------------------------------------------

bool SMBEngine::SprObjectCollisionCore()
{
    bool collisionFound = false;

    writeData(0x06, y); // save contents of Y here
    a = 0x01;
    writeData(0x07, 0x01); // save value 1 here as counter, compare horizontal coordinates first

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
                return collisionFound; // note horizontal wrapping never occurs

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
            return collisionFound;

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
        return collisionFound; // not bother checking vertical ones, because what's the point?

    //------------------------------------------------------------------------

CollisionFound:
        ++x; // increment offsets on both objects to check
        ++y; // the vertical coordinates
        --M(0x07); // decrement counter to reflect this
    } while ((M(0x07) & 0x80) == 0); // if counter not expired, branch to loop
    collisionFound = true; // otherwise we already did both sets, therefore collision
    y = M(0x06); // load original value set here earlier, then leave
    return collisionFound;
}

//------------------------------------------------------------------------

void SMBEngine::ScrollScreen()
{
    uint32_t wide = 0;

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
    // get PPU register 1 mirror
    a = M(Mirror_PPU_CTRL_REG1) & 0b11111110; // save all bits except d0
    a |= M(0x00); // get saved bit here and save in PPU register 1
    writeData(Mirror_PPU_CTRL_REG1, a); // mirror to be used to set name table later
    GetScreenPosition(); // figure out where the right side is
    a = 0x08;
    writeData(ScrollIntervalTimer, 0x08); // set scroll timer (residual, not used elsewhere)
    ChkPOffscr(); // skip this part
    return;
}

//------------------------------------------------------------------------

// set maximum speed in A
void SMBEngine::SetHiMax()
{
    a = 0x03;
    SetXMoveAmt();
    return;
}

//------------------------------------------------------------------------

// set movement amount here
void SMBEngine::SetXMoveAmt()
{
    writeData(0x00, y);
    ++x; // increment X for enemy offset
    ImposeGravitySprObj(); // do a sub to move enemy object downwards
    x = M(ObjectOffset); // get enemy object buffer offset and leave
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ScrollHandler()
{
    a = M(Player_X_Scroll); // load value saved here
    a += M(Platform_X_Scroll); // add value used by left/right platforms
    writeData(Player_X_Scroll, a); // save as new value here to impose force on scroll
    // check scroll lock flag
    if (M(ScrollLock) != 0)
        goto InitScrlAmt; // skip a bunch of code here if set
    if (M(Player_Pos_ForScroll) < 0x50)
        goto InitScrlAmt; // if less than 80 pixels to the right, branch
    // if timer related to player's side collision
    if (M(SideCollisionTimer) != 0)
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
    if (M(Player_Pos_ForScroll) < 0x70)
    {
        ScrollScreen(); // if less than 112 pixels to the right, branch
        return;
    }
    y = M(Player_X_Scroll); // otherwise get original value undecremented
    ScrollScreen();
    return;

InitScrlAmt:
    a = 0x00;
    writeData(ScrollAmount, 0x00); // initialize value here

    ChkPOffscr();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::EraseEnemyObject()
{
    a = 0x00; // clear all enemy object variables
    writeData(Enemy_Flag + x, 0x00);
    writeData(Enemy_ID + x, 0x00);
    writeData(Enemy_State + x, 0x00);
    writeData(FloateyNum_Control + x, 0x00);
    writeData(EnemyIntervalTimer + x, 0x00);
    writeData(ShellChainCounter + x, 0x00);
    writeData(Enemy_SprAttrib + x, 0x00);
    writeData(EnemyFrameTimer + x, 0x00);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::OffscreenBoundsCheck()
{
    uint32_t wide = 0;
    bool carry = false;

    a = M(Enemy_ID + x); // check for cheep-cheep object
    if (a == FlyingCheepCheep)
        return;
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
            return; // if enemy object is on the screen, leave, do not erase enemy
        a = M(Enemy_State + x); // if at this point, enemy is offscreen to the right, so check
        if (a == HammerBro)
            return;
        if (y == PiranhaPlant)
            return;
        if (y == FlagpoleFlagObject)
            return;
        if (y == StarFlagObject)
            return;
        if (y == JumpspringObject)
            return; // erase all others too far to the right
    } // TooFar: erase object if necessary
    EraseEnemyObject();

    return; // ExScrnBd: leave
}

//------------------------------------------------------------------------

void SMBEngine::DumpSixSpr()
{
    writeData(Sprite_Data + 20 + y, a); // dump A contents
    writeData(Sprite_Data + 16 + y, a); // into third row sprites
    DumpFourSpr();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DumpFourSpr()
{
    writeData(Sprite_Data + 12 + y, a); // into second row sprites
    DumpThreeSpr();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DumpThreeSpr()
{
    writeData(Sprite_Data + 8 + y, a);
    DumpTwoSpr();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DumpTwoSpr()
{
    writeData(Sprite_Data + 4 + y, a); // and into first row sprites
    writeData(Sprite_Data + y, a);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveESprRowOffscreen()
{
    a += M(Enemy_SprDataOffset + x);
    y = a; // use as offset
    a = 0xf8;
    DumpTwoSpr(); // move first row of sprites offscreen
    return;
}

//------------------------------------------------------------------------

void SMBEngine::SprObjectOffscrChk()
{
    bool shiftedBit = false;

    x = M(ObjectOffset); // get enemy buffer offset
    // check offscreen information
    a = M(Enemy_OffscreenBits) >> 2; // shift three times to the right
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // which takes d2
    pha(); // save to stack
    if (shiftedBit)
    { // branch if not set
        a = 0x04; // set for right column sprites
        MoveESprColOffscreen(); // and move them offscreen
    } // LcChk: get from stack
    pla();
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // take d3
    pha(); // save to stack
    if (shiftedBit)
    { // branch if not set
        a = 0x00; // set for left column sprites,
        MoveESprColOffscreen(); // move them offscreen
    } // Row3C: get from stack again
    pla();
    a >>= 1; // take d5 this time
    shiftedBit = (a & 0x01) != 0;
    a >>= 1;
    pha(); // save to stack again
    if (shiftedBit)
    { // branch if it was not set
        a = 0x10; // set for third row of sprites
        MoveESprRowOffscreen(); // and move them offscreen
    } // Row23C: get from stack
    pla();
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // take d6
    pha(); // save to stack
    if (shiftedBit)
    {
        a = 0x08; // set for second and third rows
        MoveESprRowOffscreen(); // move them offscreen
    } // AllRowC: get from stack once more
    pla();
    shiftedBit = (a & 0x01) != 0;
    a >>= 1; // take d7
    if (!shiftedBit)
        return;
    MoveESprRowOffscreen(); // move all sprites offscreen (A should be 0 by now)
    a = M(Enemy_ID + x);
    if (a == Podoboo)
        return; // skip this part if found, we do not want to erase podoboo!
    a = M(Enemy_Y_HighPos + x); // check high byte of vertical position
    if (a != 0x02)
        return;
    EraseEnemyObject(); // what it says

    return; // ExEGHandler
}

//------------------------------------------------------------------------

void SMBEngine::MoveESprColOffscreen()
{
    a += M(Enemy_SprDataOffset + x);
    y = a; // use as offset
    MoveColOffscreen(); // move first and second row sprites in column offscreen
    writeData(Sprite_Data + 16 + y, a); // move third row sprite in column offscreen
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveColOffscreen()
{
    a = 0xf8; // move offscreen two OAMs
    writeData(Sprite_Y_Position + y, 0xf8); // on the left side (or two rows of enemy on either side
    writeData(Sprite_Y_Position + 8 + y, 0xf8); // if branched here from enemy graphics handler)
    // ExDBlk
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveD_EnemyVertically()
{
    y = 0x3d; // set quick movement amount downwards
    // then check enemy state
    if (M(Enemy_State + x) == 0x05)
    { // and use, otherwise set different movement amount, continue on
        y = 0x20; // set movement amount
    } // ContVMove: jump to skip the rest of this
    SetHiMax();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::DrawExplosion_Fireworks()
{
    x = a; // use whatever's in A for offset
    a = M(ExplosionTiles + x); // get tile number using offset
    ++y; // increment Y (contains sprite data offset)
    DumpFourSpr(); // and dump into tile number part of sprite data
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
    // set palette attributes for all sprites, but
    writeData(Sprite_Attributes + y, 0x02); // set no flip at all for first sprite
    writeData(Sprite_Attributes + 4 + y, 0x82); // set vertical flip for second sprite
    writeData(Sprite_Attributes + 8 + y, 0x42); // set horizontal flip for third sprite
    a = 0xc2;
    writeData(Sprite_Attributes + 12 + y, 0xc2); // set both flips for fourth sprite
    return; // we are done
}

//------------------------------------------------------------------------

void SMBEngine::EnemyTurnAround()
{
    a = M(Enemy_ID + x); // check for specific enemies
    if (a == PiranhaPlant)
        return; // if piranha plant, leave
    if (a == Lakitu)
        return; // if lakitu, leave
    if (a == HammerBro)
        return; // if hammer bro, leave
    if (a == Spiny)
    {
        RXSpd(); // if spiny, turn it around
        return;
    }
    if (a == GreenParatroopaJump)
    {
        RXSpd(); // if green paratroopa, turn it around
        return;
    }
    if (a >= 0x07)
        return; // if any OTHER enemy object => $07, leave
    RXSpd();
    return;
}

//------------------------------------------------------------------------

// load horizontal speed
void SMBEngine::RXSpd()
{
    a = M(Enemy_X_Speed + x) ^ 0xff; // get two's compliment for horizontal speed
    y = a;
    ++y;
    writeData(Enemy_X_Speed + x, y); // store as new horizontal speed
    a = M(Enemy_MovingDir + x) ^ 0b00000011; // invert moving direction and store, then leave
    writeData(Enemy_MovingDir + x, a); // thus effectively turning the enemy around

    return; // ExTA: leave!!!
}

//------------------------------------------------------------------------

void SMBEngine::MoveSixSpritesOffscreen()
{
    a = 0xf8; // set offscreen coordinate if jumping here
    DumpSixSpr();
    return;
}

//------------------------------------------------------------------------

// set starting position to override
void SMBEngine::SetEntr()
{
    a = 0x02;
    writeData(AltEntranceControl, 0x02);
    ChgAreaMode(); // set modes
    return;
}

//------------------------------------------------------------------------

// set flag to disable screen output
void SMBEngine::ChgAreaMode()
{
    ++M(DisableScreenFlag);
    a = 0x00;
    writeData(OperMode_Task, 0x00); // set secondary mode of operation
    writeData(Sprite0HitDetectFlag, 0x00); // disable sprite 0 check
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ChkToStunEnemies()
{
    if (a < 0x09)
    {
        SetStun();
        return;
    }
    if (a >= 0x11)
    {
        SetStun(); // $09, $0e, $0f or $10, it will be modified, and not
        return;
    }
    if (a >= 0x0a)
    { // always fail this test because A will still have vertical
        if (a < PiranhaPlant)
        {
            SetStun(); // are only necessary if branching from $d7a1
            return;
        }
    } // Demote: erase all but LSB, essentially turning enemy object
    a &= 0b00000001;
    writeData(Enemy_ID + x, a); // into green or red koopa troopa to demote them
    SetStun();
    return;
}

//------------------------------------------------------------------------

// load enemy state
void SMBEngine::SetStun()
{
    bool enemyRightOfPlayer = false;

    a = M(Enemy_State + x) & 0b11110000; // save high nybble
    a |= 0b00000010;
    writeData(Enemy_State + x, a); // set d1 of enemy state
    --M(Enemy_Y_Position + x);
    --M(Enemy_Y_Position + x); // subtract two pixels from enemy's vertical position
    if (M(Enemy_ID + x) != Bloober)
    {
        a = 0xfd; // set default vertical speed
        if (M(AreaType) != 0)
            goto SetNotW; // if area type not water, set as speed, otherwise
    } // SetWYSpd: change the vertical speed
    a = 0xff;

SetNotW: // set vertical speed now
    writeData(Enemy_Y_Speed + x, a);
    y = 0x01;
    enemyRightOfPlayer = PlayerEnemyDiff(); // get horizontal difference between player and enemy object
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
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetEnemyBoundBox()
{
    // store bitmask here for now
    writeData(0x00, 0x48);
    y = 0x44; // store another bitmask here for now and jump
    GetMaskedOffScrBits();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::GetMaskedOffScrBits()
{
    uint32_t wide = 0;

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
    {
        MoveBoundBoxOffscreen(); // if anything set here, branch
        return;
    }
    SetupEOffsetFBBox(); // otherwise, do something else
    return;
}

//------------------------------------------------------------------------

void SMBEngine::MoveBoundBoxOffscreen()
{
    a = x; // multiply offset by 4
    a <<= 1;
    a <<= 1;
    y = a; // use as offset here
    a = 0xff;
    writeData(EnemyBoundingBoxCoord + y, 0xff); // load value into four locations here and leave
    writeData(EnemyBoundingBoxCoord + 1 + y, 0xff);
    writeData(EnemyBoundingBoxCoord + 2 + y, 0xff);
    writeData(EnemyBoundingBoxCoord + 3 + y, 0xff);
    return;
}

//------------------------------------------------------------------------

void SMBEngine::UpToFiery()
{
    y = 0x00; // set value to be used as new player state
    SetPRout(); // set values to stop certain things in motion

    return; // NoPUp
}

//------------------------------------------------------------------------

// set new player state
void SMBEngine::SetKRout()
{
    y = 0x01;
    SetPRout();
    return;
}

//------------------------------------------------------------------------

// load new value to run subroutine on next frame
void SMBEngine::SetPRout()
{
    writeData(GameEngineSubroutine, a);
    writeData(Player_State, y); // store new player state
    writeData(TimerControl, 0xff); // set master timer control flag to halt timers
    y = 0x00;
    writeData(ScrollAmount, 0x00); // initialize scroll speed
    ExInjColRoutines();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ExInjColRoutines()
{
    x = M(ObjectOffset); // get enemy offset and leave
    return;
}

    //------------------------------------------------------------------------
void SMBEngine::ChkForDemoteKoopa()
{
    if (a >= 0x09)
    {
        a &= 0b00000001; // demote koopa paratroopas to ordinary troopas
        writeData(Enemy_ID + x, a);
        y = 0x00; // return enemy to normal state
        writeData(Enemy_State + x, 0x00);
        a = 0x03; // award 400 points to the player
        SetupFloateyNumber();
        InitVStf(); // nullify physics-related thing and vertical speed
        EnemyFacePlayer(); // turn enemy around if necessary
        writeData(Enemy_X_Speed + x, M(DemotedKoopaXSpdData + y)); // set appropriate moving speed based on direction
    } // HandleStompedShellE
    else // then move onto something else
    {
        // set defeated state for enemy
        writeData(Enemy_State + x, 0x04);
        ++M(StompChainCounter); // increment the stomp counter
        a = M(StompChainCounter); // add whatever is in the stomp counter
        a += M(StompTimer);
        SetupFloateyNumber(); // award points accordingly
        ++M(StompTimer); // increment stomp timer of some sort
        y = M(PrimaryHardMode); // check primary hard mode flag
        // load timer setting according to flag
        writeData(EnemyIntervalTimer + x, M(RevivalRateData + y)); // set as enemy timer to revive stomped enemy
    } // SBnce: set player's vertical speed for bounce
    a = 0xfc;
    writeData(Player_Y_Speed, 0xfc); // and then leave!!!
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ShellOrBlockDefeat()
{
    a = M(Enemy_ID + x); // check for piranha plant
    if (a == PiranhaPlant)
    { // branch if not found
        a = M(Enemy_Y_Position + x);
        a += 0x19; // add 24 pixels, plus the one carried in by the compare above
        writeData(Enemy_Y_Position + x, a);
    } // StnE: do yet another sub
    ChkToStunEnemies();
    a = M(Enemy_State + x) & 0b00011111; // mask out 2 MSB of enemy object's state
    a |= 0b00100000; // set d5 to defeat enemy and save as new state
    writeData(Enemy_State + x, a);
    a = 0x02; // award 200 points by default
    y = M(Enemy_ID + x); // check for hammer bro
    if (y == HammerBro)
    { // branch if not found
        a = 0x06; // award 1000 points for hammer bro
    } // GoombaPoints
    if (y != Goomba)
    {
        EnemySmackScore(); // branch if not found
        return;
    }
    a = 0x01; // award 100 points for goomba
    EnemySmackScore();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::EnemySmackScore()
{
    SetupFloateyNumber(); // update necessary score variables
    a = Sfx_EnemySmack; // play smack enemy sound
    writeData(Square1SoundQueue, Sfx_EnemySmack);

    return; // ExHCF: and now let's leave
}

//------------------------------------------------------------------------

// turn the enemy around, if necessary
void SMBEngine::LInj()
{
    EnemyTurnAround();
    InjurePlayer(); // go back to hurt player
    return;
}

//------------------------------------------------------------------------

void SMBEngine::InjurePlayer()
{
    a = M(InjuryTimer); // check again to see if injured invincibility timer is
    if (a != 0)
    {
        ExInjColRoutines(); // at zero, and branch to leave if so
        return;
    }
    ForceInjury();
    return;
}

//------------------------------------------------------------------------

void SMBEngine::ForceInjury()
{
    x = M(PlayerStatus); // check player's status
    if (x == 0)
    { // branch if small
        goto KillPlayer;
    }

    writeData(PlayerStatus, a); // otherwise set player's status to small
    writeData(InjuryTimer, 0x08); // set injured invincibility timer
    a = 0x10;
    writeData(Square1SoundQueue, 0x10); // play pipedown/injury sound
    GetPlayerColors(); // change player's palette if necessary
    a = 0x0a; // set subroutine to run on next frame
    SetKRout();
    return;

    //------------------------------------------------------------------------
KillPlayer:
    writeData(Player_X_Speed, x); // halt player's horizontal movement by initializing speed
    ++x;
    writeData(EventMusicQueue, x); // set event music queue to death music
    writeData(Player_Y_Speed, 0xfc); // set new vertical speed
    a = 0x0b; // set subroutine to run on next frame
    SetKRout(); // branch to set player's state and other things
    return;
}

//------------------------------------------------------------------------

void SMBEngine::PlayerEnemyCollision()
{
    bool collisionFound = false;
    bool playerVerticalOutOfRange = false;

    // check counter for d0 set
    a = M(FrameCounter) >> 1;
    if ((M(FrameCounter) & 0x01) != 0)
        return; // if set, branch to leave
    playerVerticalOutOfRange = CheckPlayerVertical(); // if player object is completely offscreen or
    if (playerVerticalOutOfRange)
        return; // if down past 224th pixel row, branch to leave
    a = M(EnemyOffscrBitsMasked + x); // if current enemy is offscreen by any amount,
    if (a != 0)
        return; // go ahead and branch to leave
    a = M(GameEngineSubroutine);
    if (a != 0x08)
        return; // on next frame, branch to leave
    a = M(Enemy_State + x) & 0b00100000; // if enemy state has d5 set, branch to leave
    if (a != 0)
        return;
    GetEnemyBoundBoxOfs(); // get bounding box offset for current enemy object
    collisionFound = PlayerCollisionCore(); // do collision detection on player vs. enemy
    x = M(ObjectOffset); // get enemy object buffer offset
    if (!collisionFound)
    { // if collision, branch past this part here
        a = M(Enemy_CollisionBits + x) & 0b11111110; // otherwise, clear d0 of current enemy object's
        writeData(Enemy_CollisionBits + x, a); // collision bit

        return; // NoPECol

    //------------------------------------------------------------------------
    } // CheckForPUpCollision
    y = M(Enemy_ID + x);
    if (y == PowerUpObject)
    { // if not found, branch to next part
        goto HandlePowerUpCollision; // otherwise, unconditional jump backwards
    } // EColl: if star mario invincibility timer expired,
    if (M(StarInvincibleTimer) != 0)
    { // perform task here, otherwise kill enemy like
        ShellOrBlockDefeat(); // hit with a shell, or from beneath
        return;
    } // HandlePECollisions
    // check enemy collision bits for d0 set
    a = M(Enemy_CollisionBits + x) & 0b00000001; // or for being offscreen at all
    a |= M(EnemyOffscrBitsMasked + x);
    if (a != 0)
        return; // branch to leave if either is true
    a = 0x01;
    a |= M(Enemy_CollisionBits + x); // otherwise set d0 now
    writeData(Enemy_CollisionBits + x, a);
    if (y == Spiny)
        goto ChkForPlayerInjury;
    if (y == PiranhaPlant)
    {
        InjurePlayer();
        return;
    }
    if (y == Podoboo)
    {
        InjurePlayer();
        return;
    }
    if (y == BulletBill_CannonVar)
        goto ChkForPlayerInjury;
    if (y >= 0x15)
    {
        InjurePlayer();
        return;
    }
    // branch if water type level
    if (M(AreaType) == 0)
    {
        InjurePlayer();
        return;
    }
    a = M(Enemy_State + x); // branch if d7 of enemy state was set
    a <<= 1;
    if ((M(Enemy_State + x) & 0x80) != 0)
        goto ChkForPlayerInjury;
    // mask out all but 3 LSB of enemy state
    a = M(Enemy_State + x) & 0b00000111;
    if (a < 0x02)
        goto ChkForPlayerInjury;
    a = M(Enemy_ID + x); // branch to leave if goomba in defeated state
    if (a == Goomba)
        return;
    // play smack enemy sound
    writeData(Square1SoundQueue, Sfx_EnemySmack);
    // set d7 in enemy state, thus become moving shell
    a = M(Enemy_State + x) | 0b10000000;
    writeData(Enemy_State + x, a);
    EnemyFacePlayer(); // set moving direction and get offset
    // load and set horizontal speed data with offset
    writeData(Enemy_X_Speed + x, M(KickedShellXSpdData + y));
    a = 0x03; // add three to whatever the stomp counter contains
    a += M(StompChainCounter);
    y = M(EnemyIntervalTimer + x); // check shell enemy's timer
    if (y < 0x03)
    { // data obtained from the stomp counter + 3
        a = M(KickedShellPtsData + y); // otherwise, set points based on proximity to timer expiration
    } // KSPts: set values for floatey number now
    SetupFloateyNumber();

    return; // ExPEC: leave!!!

//------------------------------------------------------------------------

HandlePowerUpCollision:
    EraseEnemyObject(); // erase the power-up object
    a = 0x06;
    SetupFloateyNumber(); // award 1000 points to player by default
    writeData(Square2SoundQueue, Sfx_PowerUpGrab); // play the power-up sound
    a = M(PowerUpType); // check power-up type
    if (a >= 0x02)
    { // if mushroom or fire flower, branch
        if (a == 0x03)
            goto SetFor1Up; // if 1-up mushroom, branch
        // otherwise set star mario invincibility
        writeData(StarInvincibleTimer, 0x23); // timer, and load the star mario music
        a = StarPowerMusic; // into the area music queue, then leave
        writeData(AreaMusicQueue, StarPowerMusic);
        return;

    //------------------------------------------------------------------------
    } // Shroom_Flower_PUp
    a = M(PlayerStatus); // if player status = small, branch
    if (a != 0)
    {
        if (a != 0x01)
            return;
        x = M(ObjectOffset); // get enemy offset, not necessary
        a = 0x02; // set player status to fiery
        writeData(PlayerStatus, 0x02);
        GetPlayerColors(); // run sub to change colors of player
        x = M(ObjectOffset); // get enemy offset again, and again not necessary
        a = 0x0c; // set value to be used by subroutine tree (fiery)
        UpToFiery(); // jump to set values accordingly
        return;

SetFor1Up:
        a = 0x0b; // change 1000 points into 1-up instead
        writeData(FloateyNum_Control + x, 0x0b); // and then leave
        return;

    //------------------------------------------------------------------------
    } // UpToSuper
    // set player status to super
    writeData(PlayerStatus, 0x01);
    a = 0x09; // set value to be used by subroutine tree (super)
    UpToFiery();
    return;


//------------------------------------------------------------------------

ChkForPlayerInjury:
    a = M(Player_Y_Speed); // check player's vertical speed
    if ((a & 0x80) == 0)
    { // perform procedure below if player moving upwards
        if (a != 0)
            goto EnemyStomped; // or not at all, and branch elsewhere if moving downwards
    } // ChkInj: branch if enemy object < $07
    if (M(Enemy_ID + x) >= Bloober)
    {
        a = M(Player_Y_Position); // add 12 pixels to player's vertical position
        a += 0x0c;
        if (a < M(Enemy_Y_Position + x))
            goto EnemyStomped; // branch if this player's position above (less than) enemy's
    } // ChkETmrs: check stomp timer
    if (M(StompTimer) != 0)
        goto EnemyStomped; // branch if set
    a = M(InjuryTimer); // check to see if injured invincibility timer still
    if (a != 0)
    {
        ExInjColRoutines(); // counting down, and branch elsewhere to leave if so
        return;
    }
    if (M(Player_Rel_XPos) >= M(Enemy_Rel_XPos))
    {
        // check to see if enemy is moving to the right
        if (M(Enemy_MovingDir + x) != 0x01)
        {
            LInj(); // if not, branch
            return;
        }
        InjurePlayer(); // otherwise go back to hurt player
        return;
    }

    if (M(Enemy_MovingDir + x) != 0x01)
    {
        InjurePlayer(); // to turn the enemy around
        return;
    }
    LInj();
    return;

EnemyStomped:
    // check for spiny, branch to hurt player
    if (M(Enemy_ID + x) == Spiny)
    {
        InjurePlayer();
        return;
    }
    // otherwise play stomp/swim sound
    writeData(Square1SoundQueue, Sfx_EnemyStomp);
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
    y = 0x01; // increment points data offset
    if (a == HammerBro)
        goto EnemyStompedPts;
    y = 0x02; // increment points data offset
    if (a == Lakitu)
        goto EnemyStompedPts;
    y = 0x03; // increment points data offset
    if (a == Bloober)
        goto EnemyStompedPts;
    ChkForDemoteKoopa();
    return;

EnemyStompedPts:
    a = M(StompedEnemyPtsData + y); // load points data using offset in Y
    SetupFloateyNumber(); // run sub to set floatey number controls
    a = M(Enemy_MovingDir + x);
    pha(); // save enemy movement direction to stack
    SetStun(); // run sub to kill enemy
    pla();
    writeData(Enemy_MovingDir + x, a); // return enemy movement direction from stack
    a = 0b00100000;
    writeData(Enemy_State + x, 0b00100000); // set d5 in enemy state
    InitVStf(); // nullify vertical speed, physics-related thing,
    writeData(Enemy_X_Speed + x, a); // and horizontal speed
    a = 0xfd; // set player's vertical speed, to give bounce
    writeData(Player_Y_Speed, 0xfd);
    return;
}
