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

// Inputs: digitOffset = starting digit offset into DisplayDigits/DigitModifier
// Outputs: none
void SMBEngine::DigitsMathRoutine(uint8_t digitOffset)
{
    // check mode of operation
    if (M(OperMode) != TitleScreenModeValue)
    { // if in title screen mode, branch to lock score
        uint8_t digitIndex = digitOffset;
        uint8_t modifierIndex = 0x05;

        do // AddModLoop: load digit amount to increment, add to current digit
        {
            const uint8_t sum = (uint8_t)(M(DigitModifier + modifierIndex) + M(DisplayDigits + digitIndex));
            uint8_t newDigit;
            if (sum >= 0x80)
            { // BorrowOne: the result is a negative number, so decrement the previous digit
                // and put $09 in the digit we're currently on to "borrow the one"
                --M(DigitModifier - 1 + modifierIndex);
                newDigit = 0x09;
            }
            else if (sum >= 10)
            { // CarryOne: subtract ten from our digit to make it a proper BCD number, then
                // increment the digit preceding the current one to "carry the one" properly
                newDigit = sum - 10;
                ++M(DigitModifier - 1 + modifierIndex);
            }
            else
            {
                newDigit = sum;
            }

            // StoreNewD: store as new score or game timer digit
            writeData(DisplayDigits + digitIndex, newDigit);
            --digitIndex;    // move onto next digits in score or game timer
            --modifierIndex; // and digit amounts to increment
        } while ((modifierIndex & 0x80) == 0); // loop back if we're not done yet
    } // EraseDMods: store zero here

    uint8_t eraseIndex = 0x06; // start with the last digit

    do // EraseMLoop: initialize the digit amounts to increment
    {
        writeData(DigitModifier - 1 + eraseIndex, 0x00);
        --eraseIndex;
    } while ((eraseIndex & 0x80) == 0); // do this until they're all reset, then leave
}

//------------------------------------------------------------------------

// Inputs: enemyId = enemy identifier to remove
// Outputs: none
void SMBEngine::KillEnemies(uint8_t enemyId)
{
    writeData(0x00, enemyId); // store identifier here

    uint8_t enemyIndex = 0x04; // check for identifier in enemy object buffer

    do // KillELoop
    {
        if (M(Enemy_ID + enemyIndex) == M(0x00))
        {
            writeData(Enemy_Flag + enemyIndex, 0x00); // if found, deactivate enemy object flag
        } // NoKillE: do this until all slots are checked
        --enemyIndex;
    } while ((enemyIndex & 0x80) == 0);
}

//------------------------------------------------------------------------

// Inputs: column = block buffer column position
// Outputs: none (result is written to zero-page 0x06/0x07, not a register)
void SMBEngine::GetBlockBufferAddr(uint8_t column)
{
    const uint8_t BlockBufferAddr_data[] = {LOBYTE(Block_Buffer_1), LOBYTE(Block_Buffer_2), HIBYTE(Block_Buffer_1), HIBYTE(Block_Buffer_2)};

    // the high nybble is a pointer to the high byte of the indirect here
    const uint8_t bufferIndex = column >> 4;
    writeData(0x07, BlockBufferAddr_data[2 + bufferIndex]);
    // mask out the high nybble and add to the low byte
    writeData(0x06, (uint8_t)((column & 0b00001111) + BlockBufferAddr_data[bufferIndex]));
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: return value = page number of the screen's right boundary
uint8_t SMBEngine::GetScreenPosition()
{
    uint32_t wide = 0;

    // get coordinate of screen's left boundary
    wide = ((M(ScreenLeft_PageLoc) << 8) | M(ScreenLeft_X_Pos)) + 0xff; // add 255 pixels
    writeData(ScreenRight_X_Pos, LOBYTE(wide));                         // store as coordinate of screen's right boundary
    writeData(ScreenRight_PageLoc, HIBYTE(wide));                       // store as page number where right boundary is
    return HIBYTE(wide);                                                // get page number where left boundary is
}

//------------------------------------------------------------------------

// Inputs: blockOffset = block object buffer offset
// Outputs: none
void SMBEngine::InitBlock_XY_Pos(uint8_t blockOffset)
{
    // the player's page:coordinate, plus eight pixels
    const uint32_t wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + 0x08;
    // mask out low nybble to give 16-pixel correspondence, save as horizontal coordinate
    writeData(Block_X_Position + blockOffset, LOBYTE(wide) & 0xf0);

    const uint8_t pageLoc = HIBYTE(wide);             // the page location of the player, plus the carry
    writeData(Block_PageLoc + blockOffset, pageLoc);  // save as page location of block object
    writeData(Block_PageLoc2 + blockOffset, pageLoc); // save elsewhere to be used later

    // save vertical high byte of player into vertical high byte of block object and leave
    writeData(Block_Y_HighPos + blockOffset, M(Player_Y_HighPos));
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none (the bool return communicates the result, like a status flag)
bool SMBEngine::CheckPlayerVertical()
{
    if (M(Player_OffscreenBits) >= 0xf0)
    {
        return true; // the player object is completely offscreen
    }
    if (M(Player_Y_HighPos) != 1)
    {
        return false; // the player's high vertical byte is not within the screen
    }

    // ExCPV: on the screen, so check how far down the player is vertically
    return M(Player_Y_Position) >= 0xd0;
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: pair of {whether the subtraction did not borrow, high byte of the enemy-minus-player
// page:coordinate difference}; callers test the high byte's d7 rather than the bool
std::pair<bool, uint8_t> SMBEngine::PlayerEnemyDiff(uint8_t e)
{
    // get the distance between the enemy object and the player, each one 16-bit page:coordinate
    const uint32_t wide = ((M(Enemy_PageLoc + e) << 8) | M(Enemy_X_Position + e)) - ((M(Player_PageLoc) << 8) | M(Player_X_Position));
    writeData(0x00, LOBYTE(wide)); // and store here

    const bool enemyRightOfPlayer = (wide & 0x10000) == 0; // the subtraction did not borrow
    return {enemyRightOfPlayer, HIBYTE(wide)};             // then leave
}

//------------------------------------------------------------------------

// Inputs: metatile = metatile number to test
// Outputs: none (every branch is currently a no-op return)
bool SMBEngine::ChkForNonSolids(uint8_t metatile)
{
    switch (metatile)
    {
    case 0x26:
    case 0xc2:
    case 0xc3:
    case 0x5f:
    case 0x60:
        return true;
    default:
        return false;
    }
}

//------------------------------------------------------------------------

// Inputs: boundBoxCtrlIdx = object's bounding-box-control index (SprObj_BoundBoxCtrl); relPosIdx =
// object's relative-position index (SprObject_Rel_XPos/YPos)
// Outputs: return value = boundBoxCtrlIdx*4, the index into the per-object BoundingBox_UL_Corner/
// LR_Corner arrays used by later routines such as CheckRightScreenBBox; also left in y, with
// boundBoxCtrlIdx left in x, for callers such as FBallB that forward both onwards
uint8_t SMBEngine::BoundingBoxCore(uint8_t boundBoxCtrlIdx, uint8_t relPosIdx)
{
    const uint8_t BoundBoxCtrlData_data[] = {// bbox 0 (left, top, right, bottom)
                                             0x02, 0x08, 0x0e, 0x20,
                                             // bbox 1
                                             0x03, 0x14, 0x0d, 0x20,
                                             // bbox 2
                                             0x02, 0x14, 0x0e, 0x20,
                                             // bbox 3
                                             0x02, 0x09, 0x0e, 0x15,
                                             // bbox 4
                                             0x00, 0x00, 0x18, 0x06,
                                             // bbox 5
                                             0x00, 0x00, 0x20, 0x0d,
                                             // bbox 6
                                             0x00, 0x00, 0x30, 0x0d,
                                             // bbox 7
                                             0x00, 0x00, 0x08, 0x08,
                                             // bbox 8
                                             0x06, 0x04, 0x0a, 0x08,
                                             // bbox 9
                                             0x03, 0x0e, 0x0d, 0x14,
                                             // bbox 10
                                             0x00, 0x02, 0x10, 0x15,
                                             // bbox 11
                                             0x04, 0x04, 0x0c, 0x1c};

    writeData(0x00, boundBoxCtrlIdx); // save offset here
    // store object coordinates relative to screen, vertically and horizontally respectively
    writeData(0x02, M(SprObject_Rel_YPos + relPosIdx));
    writeData(0x01, M(SprObject_Rel_XPos + relPosIdx));

    const uint8_t boundBoxIdx = (uint8_t)(boundBoxCtrlIdx << 2); // multiply offset by four
    // load the object's bounding box control value and multiply that by four to index the data
    const uint8_t boxDataIdx = (uint8_t)(M(SprObj_BoundBoxCtrl + boundBoxCtrlIdx) << 2);

    // add the first and third numbers in the bounding box data to the relative horizontal
    // coordinate, and store them using the same offset * 4
    writeData(BoundingBox_UL_Corner + boundBoxIdx, (uint8_t)(M(0x01) + BoundBoxCtrlData_data[boxDataIdx]));
    writeData(BoundingBox_LR_Corner + boundBoxIdx, (uint8_t)(M(0x01) + BoundBoxCtrlData_data[2 + boxDataIdx]));
    // add the second and fourth numbers to the relative vertical coordinate, at the
    // incremented offsets
    writeData(BoundingBox_UL_Corner + 1 + boundBoxIdx, (uint8_t)(M(0x02) + BoundBoxCtrlData_data[1 + boxDataIdx]));
    writeData(BoundingBox_LR_Corner + 1 + boundBoxIdx, (uint8_t)(M(0x02) + BoundBoxCtrlData_data[3 + boxDataIdx]));

    // FBallB in SMBGame.cpp reads both registers back and forwards them straight into
    // CheckRightScreenBBox, so they have to survive the call
    x = boundBoxCtrlIdx;
    y = boundBoxIdx;
    return boundBoxIdx;
}

//------------------------------------------------------------------------

// Inputs: objectOffset = object buffer offset; relPosIdx = relative-position index
// (SprObject_Rel_XPos/YPos)
// Outputs: y = relPosIdx. PlayerCtrlRoutine in SMBPlayer.cpp calls RelativePlayerPosition and then
// feeds the register straight into BoundingBoxCore, so it has to survive the call.
void SMBEngine::GetObjRelativePosition(uint8_t objectOffset, uint8_t relPosIdx)
{
    // load vertical coordinate low and store here
    writeData(SprObject_Rel_YPos + relPosIdx, M(SprObject_Y_Position + objectOffset));
    // take the horizontal coordinate relative to the left of the screen and store it here
    writeData(SprObject_Rel_XPos + relPosIdx, (uint8_t)(M(SprObject_X_Position + objectOffset) - M(ScreenLeft_X_Pos)));

    y = relPosIdx;
}

//------------------------------------------------------------------------

// Inputs: value = value to conditionally add to the divided difference; flag = compared against
// 0x01; currentOffset = offset to return unchanged when the pixel difference is not below the
// caller's preset value (zero-page 0x06)
// Outputs: return value = recomputed offset, but only when the pixel difference is below the
// caller's preset value; otherwise currentOffset, unchanged
uint8_t SMBEngine::DividePDiff(uint8_t value, uint8_t flag, uint8_t currentOffset)
{
    writeData(0x05, value); // store current value here

    const uint8_t pixelDiff = M(0x07);
    if (pixelDiff >= M(0x06))
    {
        return currentOffset; // ExDivPD: the pixel difference is not below the preset value
    }

    // divide the difference by eight, masking out all but the 3 LSB
    const uint8_t dividedDiff = (pixelDiff >> 3) & 0x07;

    // SetOscrO: use the difference / 8 as the offset, adding the value to it unless the flag is set
    return flag < 0x01 ? (uint8_t)(dividedDiff + value) : dividedDiff;
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: return value = moving-direction offset (0 = right, 1 = left), used by callers to index
// direction-dependent tables
uint8_t SMBEngine::EnemyFacePlayer(uint8_t e)
{
    // get horizontal difference between player and enemy
    const uint8_t diffHighByte = PlayerEnemyDiff(e).second;
    // move right by default; if the enemy is to the right of the player, move left instead
    const uint8_t movingDir = (diffHighByte & 0x80) != 0 ? 0x02 : 0x01;

    writeData(Enemy_MovingDir + e, movingDir); // SFcRt: set moving direction here
    return movingDir - 1;                      // then decrement to use as a proper offset
}

//------------------------------------------------------------------------

// Inputs: objectOffset = sprite object buffer offset
// Outputs: return value = vertical offscreen bits nybble; the horizontal bits from the
// GetXOffscreenBits() call are left in zero-page 0x00 for the caller to combine
uint8_t SMBEngine::RunOffscrBitsSubs(uint8_t objectOffset)
{
    const uint8_t HighPosUnitData_data[] = {0xff, 0x00};

    const uint8_t DefaultYOnscreenOfs_data[] = {0x04, 0x00, 0x04};

    const uint8_t YOffscreenBitsData_data[] = {0x00, 0x08, 0x0c, 0x0e, 0x0f, 0x07, 0x03, 0x01, 0x00};

    // do subroutine here, then move the high nybble to low and store it for the caller
    writeData(0x00, GetXOffscreenBits(objectOffset) >> 4);

    // GetYOffscreenBits: save position in buffer to here
    writeData(0x04, objectOffset);

    uint8_t edgeIdx = 0x01; // start with top of screen
    uint8_t bits = 0x00;

    do // YOfsLoop: load coordinate for edge of vertical unit
    {
        // the edge of the vertical unit and the object position are each one 16-bit
        // highpos:coordinate; subtract the vertical coordinate of the object from the edge
        const uint32_t wide = ((0x01 << 8) | HighPosUnitData_data[edgeIdx]) -
                              ((M(SprObject_Y_HighPos + objectOffset) << 8) | M(SprObject_Y_Position + objectOffset));
        writeData(0x07, LOBYTE(wide)); // store here

        const uint8_t diffHighByte = HIBYTE(wide);
        uint8_t bitsIdx;
        if ((diffHighByte & 0x80) != 0)
        { // under the top of the screen or beyond the bottom
            bitsIdx = DefaultYOnscreenOfs_data[edgeIdx];
        }
        else if (((diffHighByte - 0x01) & 0x80) == 0)
        { // one vertical unit or more above the screen, so use the alternate offset value
            bitsIdx = DefaultYOnscreenOfs_data[1 + edgeIdx];
        }
        else
        {
            writeData(0x06, 0x20); // if no branching, load value here and store
            bitsIdx = DividePDiff(0x04, edgeIdx, DefaultYOnscreenOfs_data[1 + edgeIdx]);
        }

        // YLdBData: get offscreen data bits using offset
        bits = YOffscreenBitsData_data[bitsIdx];
        --edgeIdx; // if the bits are zero, do the bottom of the screen now
    } while (bits == 0x00 && (edgeIdx & 0x80) == 0);

    return bits; // ExYOfsBS
}

//------------------------------------------------------------------------

// Inputs: objectOffset = sprite object buffer offset
// Outputs: return value = horizontal offscreen bits; x = restored to the input value
uint8_t SMBEngine::GetXOffscreenBits(uint8_t objectOffset)
{
    writeData(0x04, objectOffset); // save position in buffer to here

    uint8_t edgeIdx = 0x01; // start with right side of screen
    uint8_t bits = 0x00;

    do // XOfsLoop: get pixel coordinate of edge
    {
        // the edge and the object position are each one 16-bit page:coordinate; get the
        // difference between them
        const uint32_t wide = ((M(ScreenEdge_PageLoc + edgeIdx) << 8) | M(ScreenEdge_X_Pos + edgeIdx)) -
                              ((M(SprObject_PageLoc + objectOffset) << 8) | M(SprObject_X_Position + objectOffset));
        writeData(0x07, LOBYTE(wide)); // store here

        const uint8_t diffHighByte = HIBYTE(wide);
        uint8_t bitsIdx;
        if ((diffHighByte & 0x80) != 0)
        { // beyond the right edge, or in front of the left edge
            bitsIdx = M(DefaultXOnscreenOfs + edgeIdx);
        }
        else if (((diffHighByte - 0x01) & 0x80) == 0)
        { // one page or more to the left of either edge, so use the alternate offset value
            bitsIdx = M(DefaultXOnscreenOfs + 1 + edgeIdx);
        }
        else
        {
            writeData(0x06, 0x38); // if no branching, load value here and store
            bitsIdx = DividePDiff(0x08, edgeIdx, M(DefaultXOnscreenOfs + 1 + edgeIdx));
        }

        // XLdBData: get bits here
        bits = M(XOffscreenBitsData + bitsIdx);
        --edgeIdx; // if the bits are zero, do the left side of the screen now
    } while (bits == 0x00 && (edgeIdx & 0x80) == 0);

    // LargePlatformBoundBox and DrawLargePlatform in SMBEnemies.cpp decrement x on return,
    // so the buffer position has to be back in it
    x = objectOffset;
    return bits; // ExXOfsBS
}

//------------------------------------------------------------------------

// Inputs: e = new vine's enemy object buffer offset; blockOffset = source "Block" object
// offset to copy position from
// Outputs: none
void SMBEngine::Setup_Vine(uint8_t e, uint8_t blockOffset)
{
    // load identifier for vine object
    writeData(Enemy_ID + e, VineObject);                                // store in buffer
    writeData(Enemy_Flag + e, 0x01);                                    // set flag for enemy object buffer
    writeData(Enemy_PageLoc + e, M(Block_PageLoc + blockOffset));       // copy page location from previous object
    writeData(Enemy_X_Position + e, M(Block_X_Position + blockOffset)); // copy horizontal coordinate

    const uint8_t yPosition = M(Block_Y_Position + blockOffset);
    writeData(Enemy_Y_Position + e, yPosition); // copy vertical coordinate from previous object

    const uint8_t vineSlot = M(VineFlagOffset); // load vine flag/offset to next available vine slot
    if (vineSlot == 0)
    {                                               // if set at all, don't bother to store vertical
        writeData(VineStart_Y_Position, yPosition); // otherwise store vertical coordinate here
    } // NextVO: store object offset to next available vine slot, using vine flag as offset
    writeData(VineObjOffset + vineSlot, e);
    ++M(VineFlagOffset); // increment vine flag offset

    writeData(Square2SoundQueue, Sfx_GrowVine); // load vine grow sound
}

//------------------------------------------------------------------------

// Inputs: movementMode = movement mode (0 = apply downward gravity only; nonzero = also apply the
// upward-speed-capping pass); objectOffset = sprite object buffer offset. Also expects zero-page
// 0x00 (downward movement amount) and 0x02 (maximum speed) to already be set by the caller.
// Outputs: none (x is left unchanged)
void SMBEngine::ImposeGravity(uint8_t movementMode, uint8_t objectOffset)
{
    // get current vertical speed; if currently moving downwards, do not decrement
    const uint8_t moveHighBits = (M(SprObject_Y_Speed + objectOffset) & 0x80) != 0 ? 0xff : 0x00;
    writeData(0x07, moveHighBits); // AlterYP: store the high bits here

    // highpos:position:dummy is one 24-bit quantity, and $07:speed:force the
    // signed 24-bit amount to move the object by
    uint32_t position = (M(SprObject_Y_HighPos + objectOffset) << 16) | (M(SprObject_Y_Position + objectOffset) << 8) |
                        M(SprObject_YMF_Dummy + objectOffset);
    position += (M(0x07) << 16) | (M(SprObject_Y_Speed + objectOffset) << 8) | M(SprObject_Y_MoveForce + objectOffset);
    writeData(SprObject_YMF_Dummy + objectOffset, LOBYTE(position));          // add movement force to the dummy variable
    writeData(SprObject_Y_Position + objectOffset, HIBYTE(position));         // store as new vertical position
    writeData(SprObject_Y_HighPos + objectOffset, (uint8_t)(position >> 16)); // store as new vertical high byte

    // add downward movement amount to the speed:force, the carry going into the speed
    const uint32_t downSpeed = ((M(SprObject_Y_Speed + objectOffset) << 8) | M(SprObject_Y_MoveForce + objectOffset)) + M(0x00);
    writeData(SprObject_Y_MoveForce + objectOffset, LOBYTE(downSpeed));
    writeData(SprObject_Y_Speed + objectOffset, HIBYTE(downSpeed));

    // unless the new speed is still below the preset value, keep it within that maximum
    if (((HIBYTE(downSpeed) - M(0x02)) & 0x80) == 0 && M(SprObject_Y_MoveForce + objectOffset) >= 0x80)
    {
        writeData(SprObject_Y_Speed + objectOffset, M(0x02));  // keep vertical speed within maximum value
        writeData(SprObject_Y_MoveForce + objectOffset, 0x00); // clear fractional
    }

    // ChkUpM: if the movement mode is zero, leave without the upward-speed-capping pass
    if (movementMode == 0)
    {
        return;
    }

    // otherwise get two's compliment of maximum speed and store it here
    writeData(0x07, (uint8_t)((M(0x02) ^ 0b11111111) + 1));

    // subtract $01 from the speed:force; note that $01 is twice as large as $00, thus it
    // effectively undoes the add we did earlier
    const uint32_t upSpeed = ((M(SprObject_Y_Speed + objectOffset) << 8) | M(SprObject_Y_MoveForce + objectOffset)) - M(0x01);
    writeData(SprObject_Y_MoveForce + objectOffset, LOBYTE(upSpeed));
    writeData(SprObject_Y_Speed + objectOffset, HIBYTE(upSpeed));

    if (((HIBYTE(upSpeed) - M(0x07)) & 0x80) == 0)
    {
        return; // if less negatively than preset maximum, skip this part
    }
    if (M(SprObject_Y_MoveForce + objectOffset) >= 0x80)
    {
        return; // and if so, branch to leave
    }
    writeData(SprObject_Y_Speed + objectOffset, M(0x07));  // keep vertical speed within maximum value
    writeData(SprObject_Y_MoveForce + objectOffset, 0xff); // clear fractional

    // ExVMove: leave!
}

//------------------------------------------------------------------------

// Inputs: none (reads which side collided from zero-page 0x00, not a register)
// Outputs: none
void SMBEngine::ImpedePlayerMove()
{
    const uint8_t playerSpeed = M(Player_X_Speed); // get player's horizontal speed
    // check the value set earlier for left side collision
    const bool leftSideCollision = M(0x00) == 0x01;

    // the collision bit to clear at the end, and the amount to move the player by
    const uint8_t collisionBit = leftSideCollision ? 0x01 : 0x02; // RImpd: return $02 to X
    const uint8_t moveAmount = leftSideCollision ? 0xff : 0x01;

    // if the player is already moving that way, only the bit gets cleared
    const bool alreadyMovingThatWay = leftSideCollision ? (playerSpeed & 0x80) != 0 : ((playerSpeed - 0x01) & 0x80) == 0;

    if (!alreadyMovingThatWay)
    {
        writeData(SideCollisionTimer, 0x10); // NXSpd: set timer of some sort
        writeData(Player_X_Speed, 0x00);     // nullify player's horizontal speed
        // PlatF: store the high bits of the horizontal adder
        writeData(0x00, (moveAmount & 0x80) != 0 ? 0xff : 0x00);
        // $00:moveAmount is the signed 16-bit amount to move the player left or right by
        const uint32_t wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + ((M(0x00) << 8) | moveAmount);
        writeData(Player_X_Position, LOBYTE(wide));
        writeData(Player_PageLoc, HIBYTE(wide)); // page location if necessary
    }

    // ExIPM: invert the collision bit and mask out the bit that was set here, to clear it
    writeData(Player_CollisionBits, (uint8_t)(collisionBit ^ 0xff) & M(Player_CollisionBits));
}

//------------------------------------------------------------------------

// Inputs: objectOffset = sprite object buffer offset; boundBoxIdx = index into the per-object
// BoundingBox_UL_XPos/DR_XPos arrays (typically objectOffset*4, from BoundingBoxCore)
// Outputs: x = M(ObjectOffset) (restores the current object buffer offset for whichever routine
// resumes after this)
void SMBEngine::CheckRightScreenBBox(uint8_t objectOffset, uint8_t boundBoxIdx)
{
    // add 128 pixels to left side of screen
    const uint32_t middle = ((M(ScreenLeft_PageLoc) << 8) | M(ScreenLeft_X_Pos)) + 0x80;
    writeData(0x02, LOBYTE(middle));
    writeData(0x01, HIBYTE(middle)); // add carry to page location of left side of screen

    // compare the object, a 16-bit page:coordinate, against the middle of the screen
    const bool objectOnRightSide =
        ((M(SprObject_PageLoc + objectOffset) << 8) | M(SprObject_X_Position + objectOffset)) >= ((M(0x01) << 8) | M(0x02));

    if (objectOnRightSide)
    {
        // check right-side edge of bounding box for offscreen coordinates, and do nothing
        // if it is still on the screen
        if ((M(BoundingBox_DR_XPos + boundBoxIdx) & 0x80) == 0)
        {
            // likewise the left-side edge; 0xff is the offscreen value for both sides
            if ((M(BoundingBox_UL_XPos + boundBoxIdx) & 0x80) == 0)
            {
                writeData(BoundingBox_UL_XPos + boundBoxIdx, 0xff); // store offscreen value for left side
            } // SORte: store offscreen value for right side
            writeData(BoundingBox_DR_XPos + boundBoxIdx, 0xff);
        } // NoOfs
    }
    else // CheckLeftScreenBBox
    {
        // check left-side edge of bounding box for offscreen coordinates; anything at $a0 or
        // above is really offscreen rather than merely past the left edge (d7 is implied by it)
        if (M(BoundingBox_UL_XPos + boundBoxIdx) >= 0xa0)
        {
            // check right-side edge of bounding box for offscreen coordinates
            if ((M(BoundingBox_DR_XPos + boundBoxIdx) & 0x80) != 0)
            {
                writeData(BoundingBox_DR_XPos + boundBoxIdx, 0x00); // store offscreen value for right side
            } // SOLft: store offscreen value for left side
            writeData(BoundingBox_UL_XPos + boundBoxIdx, 0x00);
        }
    }

    // NoOfs2: get object offset and leave
    x = M(ObjectOffset);
}

//------------------------------------------------------------------------

// Inputs: spritePairIdx = sprite data offset (pair index); oamSlot = sprite data offset (OAM
// slot); also reads zero-page 0x00-0x05 temporaries set by the caller
// Outputs: pair of {spritePairIdx+2, oamSlot+8}, advancing to the next sprite pair and OAM row;
// also left in x and y, which is how the drawing loops in SMBGame.cpp consume them
std::pair<uint8_t, uint8_t> SMBEngine::DrawSpriteObject(uint8_t spritePairIdx, uint8_t oamSlot)
{
    // get saved flip control bits; d1 is the horizontal flip
    const bool horizontalFlip = (M(0x03) & 0b00000010) != 0;

    uint8_t attributes;
    if (horizontalFlip)
    {
        writeData(Sprite_Tilenumber + 4 + oamSlot, M(0x00)); // store first tile into second sprite
        writeData(Sprite_Tilenumber + oamSlot, M(0x01));     // and second into first sprite
        attributes = 0x40;                                   // activate horizontal flip OAM attribute
    }
    else // NoHFlip
    {
        writeData(Sprite_Tilenumber + oamSlot, M(0x00));     // store first tile into first sprite
        writeData(Sprite_Tilenumber + 4 + oamSlot, M(0x01)); // and second into second sprite
        attributes = 0x00;                                   // clear bit for horizontal flip
    }

    // SetHFAt: add other OAM attributes if necessary
    attributes |= M(0x04);
    writeData(Sprite_Attributes + oamSlot, attributes); // store sprite attributes
    writeData(Sprite_Attributes + 4 + oamSlot, attributes);

    const uint8_t yPosition = M(0x02);                     // now the y coordinates
    writeData(Sprite_Y_Position + oamSlot, yPosition);     // note because they are
    writeData(Sprite_Y_Position + 4 + oamSlot, yPosition); // side by side, they are the same

    const uint8_t xPosition = M(0x05);
    writeData(Sprite_X_Position + oamSlot, xPosition);                       // store x coordinate, then
    writeData(Sprite_X_Position + 4 + oamSlot, (uint8_t)(xPosition + 0x08)); // put them side by side

    writeData(0x02, (uint8_t)(yPosition + 0x08)); // add eight pixels to the next y

    // advance both offsets to return them to the routine that called this subroutine.
    // DrawPlayerLoop, and the DBlkLoop loop in DrawBlock, both in SMBGame.cpp, loop on the
    // registers rather than on the returned pair, so both have to survive the call
    x = (uint8_t)(spritePairIdx + 2);
    y = (uint8_t)(oamSlot + 0x08);
    return {x, y};
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitPiranhaPlant(uint8_t e)
{
    // set initial speed
    writeData(PiranhaPlant_Y_Speed + e, 0x01);
    writeData(Enemy_State + e, 0x00);           // initialize enemy state and what would normally
    writeData(PiranhaPlant_MoveFlag + e, 0x00); // be used as vertical speed, but not in this case

    const uint8_t yPosition = M(Enemy_Y_Position + e);
    writeData(PiranhaPlantDownYPos + e, yPosition);                 // save original vertical coordinate here
    writeData(PiranhaPlantUpYPos + e, (uint8_t)(yPosition - 0x18)); // and that - 24 pixels here

    SetBBox2(0x09, e); // set specific value for bounding box control
}

//------------------------------------------------------------------------

// set bounding box control then leave
// Inputs: boundBoxCtrl = bounding box control value; e = enemy object buffer offset
// Outputs: none
void SMBEngine::SetBBox2(uint8_t boundBoxCtrl, uint8_t e) { writeData(Enemy_BoundBoxCtrl + e, boundBoxCtrl); }

//------------------------------------------------------------------------

// initialize vertical speed
// Inputs: e = enemy object buffer offset
// Outputs: a = 0; several callers go straight on to store that zero as a speed of their own,
// so it has to survive the call
void SMBEngine::InitVStf(uint8_t e)
{
    writeData(Enemy_Y_Speed + e, 0x00);     // initialize vertical speed
    writeData(Enemy_Y_MoveForce + e, 0x00); // and movement force

    a = 0x00;
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: x = M(VRAM_Buffer1_Offset), as it was before the seven bytes written here. The player
// entrance code in SMBGame.cpp falls out of here into SetupBubble, which takes the register as its
// air bubble slot -- so on the first frame of a water level, where that bubble goes depends on the
// vram buffer offset this happened to leave behind.
void SMBEngine::GetPlayerColors()
{
    const uint8_t PlayerColors_data[] = {
        0x22, 0x16, 0x27, 0x18, // mario's colors
        0x22, 0x30, 0x27, 0x19, // luigi's colors
        0x22, 0x37, 0x27, 0x16  // fiery (used by both)
    };

    const uint8_t BackgroundColors_data[] = {
        0x22, 0x22, 0x0f, 0x0f, // used by area type if bg color ctrl not set
        0x0f, 0x22, 0x0f, 0x0f  // used by background color control if set
    };

    // check which player is on the screen, loading the offset for luigi if it is him
    uint8_t colorIdx = M(CurrentPlayer) != 0 ? 0x04 : 0x00;
    if (M(PlayerStatus) == 0x02)
    { // ChkFiery: if fiery, load alternate offset for fiery player
        colorIdx = 0x08;
    }

    uint8_t bufferIdx = M(VRAM_Buffer1_Offset); // get current buffer offset
    writeData(0x00, 0x03);                      // StartClrGet: do four colors

    do // ClrGetLoop: fetch player colors and store them in the buffer
    {
        writeData(VRAM_Buffer1 + 3 + bufferIdx, PlayerColors_data[colorIdx]);
        ++colorIdx;
        ++bufferIdx;
        --M(0x00);
    } while ((M(0x00) & 0x80) == 0);

    const uint8_t offset = M(VRAM_Buffer1_Offset); // load original offset from before
    // if the background color control is set it is used as the offset to the background
    // color, otherwise the area type bits from the area offset are used instead
    const uint8_t bgColorIdx = M(BackgroundColorCtrl) != 0 ? M(BackgroundColorCtrl) : M(AreaType);
    writeData(VRAM_Buffer1 + 3 + offset, BackgroundColors_data[bgColorIdx]); // SetBGColor

    // set for sprite palette address, saving to buffer
    writeData(VRAM_Buffer1 + offset, 0x3f);
    writeData(VRAM_Buffer1 + 1 + offset, 0x10);
    writeData(VRAM_Buffer1 + 2 + offset, 0x04); // write length byte to buffer
    writeData(VRAM_Buffer1 + 7 + offset, 0x00); // now the null terminator

    x = offset;                              // see above: SetupBubble reads this back
    SetVRAMOffset((uint8_t)(offset + 0x07)); // move the buffer pointer ahead 7 bytes
}

//------------------------------------------------------------------------

// store as new vram buffer offset
// Inputs: newOffset = new VRAM_Buffer1 offset
// Outputs: none
void SMBEngine::SetVRAMOffset(uint8_t newOffset) { writeData(VRAM_Buffer1_Offset, newOffset); }

//------------------------------------------------------------------------

// Inputs: metatile = metatile number to check; controlBit = control bit/offset (passed through
// unchanged, needed by PutBlockMetatile)
// Outputs: x = unchanged (round-tripped through PutBlockMetatile/RemBridge); at least one caller
// reads it back afterward
void SMBEngine::WriteBlockMetatile(uint8_t metatile, uint8_t controlBit)
{
    uint8_t groupSelector;
    if (metatile == 0x00)
    {
        groupSelector = 0x03; // offset for blank metatile (unconditional if branched from 8a6b)
    }
    else if (metatile == 0x58 || metatile == 0x51)
    {
        // brick with coins (w/ line), or breakable brick w/ line
        groupSelector = 0x00; // offset for brick metatile w/ line
    }
    else if (metatile == 0x5d || metatile == 0x52)
    {
        // brick with coins (w/o line), or breakable brick w/o line
        groupSelector = 0x01; // offset for brick metatile w/o line
    }
    else
    {
        groupSelector = 0x02; // if any other metatile, the offset for an empty block
    }

    // UseBOffset: get vram buffer offset and move onto next byte
    const uint8_t vramOffset = (uint8_t)(M(VRAM_Buffer1_Offset) + 1);
    // get appropriate block data and write to vram buffer
    PutBlockMetatile(groupSelector, controlBit, vramOffset);

    MoveVOffset(vramOffset);
}

//------------------------------------------------------------------------

// decrement vram buffer offset
// Inputs: vramOffset = current vram buffer offset (post-increment)
// Outputs: none
void SMBEngine::MoveVOffset(uint8_t vramOffset)
{
    // decrement the offset, add 10 bytes to it, then store as the new vram buffer offset
    SetVRAMOffset((uint8_t)(vramOffset - 1 + 10));
}

//------------------------------------------------------------------------

// Inputs: metatileGroupSelector = metatile group selector (multiplied by 4 to index
// BlockGfxData_data); controlBit = control bit/offset, saved and restored across the call;
// vramOffset = vram buffer offset for the next byte
// Outputs: x = unchanged (round-tripped through zero-page 0x00 and restored by RemBridge)
void SMBEngine::PutBlockMetatile(uint8_t metatileGroupSelector, uint8_t controlBit, uint8_t vramOffset)
{
    writeData(0x00, controlBit); // store control bit from SprDataOffset_Ctrl
    writeData(0x01, vramOffset); // store vram buffer offset for next byte

    // multiply the selector by four to index the block graphics data
    const uint8_t metatileGroupOfs4 = (uint8_t)(metatileGroupSelector << 2);

    // get low byte of block buffer pointer; at $d0 or above use the high byte for name
    // table 1, otherwise the one for name table 0
    const uint8_t blockBufferLow = M(0x06);
    writeData(0x03, blockBufferLow >= 0xd0 ? 0x24 : 0x20); // SaveHAdder: save high byte here

    // mask out the high nybble of the block buffer pointer and multiply by 2 to get the
    // appropriate name table low byte, and then store it here
    writeData(0x04, (uint8_t)((blockBufferLow & 0x0f) << 1));

    // the vertical offset, times four, is a ten-bit quantity; add the sixteen-bit
    // name table address in $03:$04 to it and store the result back in $05:$04
    uint32_t wide = (uint8_t)(M(0x02) + 0x20) << 2; // add 32 pixels for the status bar
    wide += (M(0x03) << 8) | M(0x04);               // add the name table address
    writeData(0x04, LOBYTE(wide));                  // and store here
    writeData(0x05, HIBYTE(wide));                  // store here

    RemBridge(metatileGroupOfs4, M(0x01)); // get vram buffer offset to be used
}

//------------------------------------------------------------------------

// write top left and top right
// Inputs: metatileGroupOfs4 = metatile-group offset (x4) into BlockGfxData_data, set by
// PutBlockMetatile; vramOffset = vram buffer offset
// Outputs: x = restored to the value PutBlockMetatile saved in zero-page 0x00 (the control
// bit/offset from before PutBlockMetatile repurposed x)
void SMBEngine::RemBridge(uint8_t metatileGroupOfs4, uint8_t vramOffset)
{
    const uint8_t BlockGfxData_data[] = {// brick with line on top
                                         0x45, 0x45, 0x47, 0x47,
                                         // brick
                                         0x47, 0x47, 0x47, 0x47,
                                         // emptied block
                                         0x57, 0x58, 0x59, 0x5a,
                                         // empty
                                         0x24, 0x24, 0x24, 0x24,
                                         // solid
                                         0x26, 0x26, 0x26, 0x26};

    // write top left and top right tile numbers into first spot
    writeData(VRAM_Buffer1 + 2 + vramOffset, BlockGfxData_data[metatileGroupOfs4]);
    writeData(VRAM_Buffer1 + 3 + vramOffset, BlockGfxData_data[1 + metatileGroupOfs4]);
    // write bottom left and bottom right tile numbers into second spot
    writeData(VRAM_Buffer1 + 7 + vramOffset, BlockGfxData_data[2 + metatileGroupOfs4]);
    writeData(VRAM_Buffer1 + 8 + vramOffset, BlockGfxData_data[3 + metatileGroupOfs4]);

    const uint8_t nameTableLow = M(0x04);
    writeData(VRAM_Buffer1 + vramOffset, nameTableLow); // write low byte of name table
    // add 32 bytes to the value and write that low byte into the second slot
    writeData(VRAM_Buffer1 + 5 + vramOffset, (uint8_t)(nameTableLow + 0x20));

    const uint8_t nameTableHigh = M(0x05);
    writeData(VRAM_Buffer1 - 1 + vramOffset, nameTableHigh); // write high byte of name
    writeData(VRAM_Buffer1 + 4 + vramOffset, nameTableHigh); // table address to both slots

    writeData(VRAM_Buffer1 + 1 + vramOffset, 0x02); // put length of 2 in
    writeData(VRAM_Buffer1 + 6 + vramOffset, 0x02); // both slots
    writeData(VRAM_Buffer1 + 9 + vramOffset, 0x00); // put null terminator at end

    x = M(0x00); // get offset control bit here and leave
}

//------------------------------------------------------------------------

// Inputs: playerOffset = player-specific offset
// Outputs: none
void SMBEngine::PrintStatusBarNumbers(uint8_t playerOffset)
{
    writeData(0x00, playerOffset); // store player-specific offset
    OutputNumbers(playerOffset);   // use first nybble to print the coin display

    // move high nybble to low and print to score display
    OutputNumbers(M(0x00) >> 4);
}

//------------------------------------------------------------------------

// Inputs: nybbleIdx = nybble index (incremented by one at entry)
// Outputs: none
void SMBEngine::OutputNumbers(uint8_t nybbleIdx)
{
    const uint8_t StatusBarOffset_data[] = {0x06, 0x0c, 0x12, 0x18, 0x1e, 0x24};

    const uint8_t StatusBarData_data[] = {
        0xf0, 0x06,             //  top score display on title screen
        0x62, 0x06,             //  player score
        0x62, 0x06, 0x6d, 0x02, //  coin tally
        0x6d, 0x02, 0x7a, 0x03  //  game timer
    };

    const uint8_t displayIdx = (uint8_t)(nybbleIdx + 0x01) & 0b00001111; // mask out high nybble
    if (displayIdx >= 0x06)
    {
        return; // ExitOutputN
    }

    const uint8_t barDataIdx = (uint8_t)(displayIdx << 1); // shift to left and use as offset
    const uint8_t bufferIdx = M(VRAM_Buffer1_Offset);      // get current buffer pointer

    // SetupNums: put at top of screen by default, or further down for the top score display
    writeData(VRAM_Buffer1 + bufferIdx, barDataIdx == 0x00 ? 0x22 : 0x20);
    // write low vram address of the thing we're printing to the buffer
    writeData(VRAM_Buffer1 + 1 + bufferIdx, StatusBarData_data[barDataIdx]);

    const uint8_t length = StatusBarData_data[1 + barDataIdx]; // and its length
    writeData(VRAM_Buffer1 + 2 + bufferIdx, length);
    writeData(0x03, length);    // save length byte in counter
    writeData(0x02, bufferIdx); // and buffer pointer elsewhere for now

    // load offset to value we want to write, subtract the length byte we read before, and
    // use the value as the offset to the display digits
    uint8_t digitIdx = (uint8_t)(StatusBarOffset_data[displayIdx] - length);
    uint8_t writeIdx = M(0x02);

    do // DigitPLoop: write digits to the buffer
    {
        writeData(VRAM_Buffer1 + 3 + writeIdx, M(DisplayDigits + digitIdx));
        ++writeIdx;
        ++digitIdx;
        --M(0x03); // do this until all the digits are written
    } while (M(0x03) != 0);

    writeData(VRAM_Buffer1 + 3 + writeIdx, 0x00); // put null terminator at end
    // increment buffer pointer by 3, storing it in case we want to use it again
    writeData(VRAM_Buffer1_Offset, (uint8_t)(writeIdx + 3));
}

//------------------------------------------------------------------------

// set X for player offset
// Inputs: none (always operates on the player, object index 0)
// Outputs: none
void SMBEngine::ChkPOffscr()
{
    const uint8_t OffscrJoypadBitsData_data[] = {0x01, 0x02};

    const uint8_t X_SubtracterData_data[] = {0x00, 0x10};

    // get horizontal offscreen bits for player and save them here
    const uint8_t offscreenBits = GetXOffscreenBits(0x00);
    writeData(0x00, offscreenBits);

    // if d7 of the offscreen bits is set use the default offset (left side), otherwise the
    // offset for the right side
    const bool leftSide = (offscreenBits & 0x80) != 0;
    const uint8_t edgeIdx = leftSide ? 0x00 : 0x01;

    // on the right side, d5 of the offscreen bits also has to be set to keep the player on
    if (leftSide || (M(0x00) & 0b00100000) != 0)
    {
        // KeepOnscr: get left or right side coordinate based on offset, subtracting an
        // amount based on that same offset
        const uint32_t wide = ((M(ScreenEdge_PageLoc + edgeIdx) << 8) | M(ScreenEdge_X_Pos + edgeIdx)) - X_SubtracterData_data[edgeIdx];
        writeData(Player_X_Position, LOBYTE(wide)); // store as player position to prevent movement further
        writeData(Player_PageLoc, HIBYTE(wide));    // save as player's page location

        // check saved controller bits, nullifying the player's horizontal speed if not equal
        if (M(Left_Right_Buttons) != OffscrJoypadBitsData_data[edgeIdx])
        {
            writeData(Player_X_Speed, 0x00);
        }
    }

    // InitPlatScrl: nullify platform force imposed on scroll
    writeData(Platform_X_Scroll, 0x00);
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::AddToScore()
{
    const uint8_t ScoreOffsets_data[] = {0x0b, 0x11};

    // get the offset for the current player's score, and update the score internally with
    // the value in the digit modifier
    DigitsMathRoutine(ScoreOffsets_data[M(CurrentPlayer)]);

    GetSBNybbles();
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::GetSBNybbles()
{
    const uint8_t StatusBarNybbles_data[] = {0x02, 0x13};

    // get nybbles based on the current player, used to update score and coins
    UpdateNumber(StatusBarNybbles_data[M(CurrentPlayer)]);
}

//------------------------------------------------------------------------

// Inputs: statusBarNybbles = status bar nybbles value (forwarded to PrintStatusBarNumbers)
// Outputs: x = M(ObjectOffset) (restores the enemy object buffer offset for whichever routine
// resumes after this)
void SMBEngine::UpdateNumber(uint8_t statusBarNybbles)
{
    PrintStatusBarNumbers(statusBarNybbles); // print status bar numbers based on nybbles, whatever they be

    const uint8_t bufferIdx = M(VRAM_Buffer1_Offset);
    if (M(VRAM_Buffer1 - 6 + bufferIdx) == 0)
    { // if the highest digit of the score is zero, overwrite it with a space tile so that
        // it is suppressed
        writeData(VRAM_Buffer1 - 6 + bufferIdx, 0x24);
    } // NoZSup: get enemy object buffer offset
    x = M(ObjectOffset);
}

//------------------------------------------------------------------------

// this is a residual jump point in enemy object jump table
// Inputs: none (always operates on enemy slot 5)
// Outputs: none
void SMBEngine::PwrUpJmp()
{
    writeData(Enemy_State + 5, 0x01);        // set power-up object's state
    writeData(Enemy_Flag + 5, 0x01);         // set buffer flag
    writeData(Enemy_BoundBoxCtrl + 5, 0x03); // set bounding box size control for power-up object
    if (M(PowerUpType) < 0x02)
    {                                           // if star or 1-up, branch ahead
        const uint8_t status = M(PlayerStatus); // otherwise check player's current status
        // if player not fiery, use status as power-up type, otherwise shift right to force
        // the fire flower type
        writeData(PowerUpType, status >= 0x02 ? status >> 1 : status); // StrType: store type here
    } // PutBehind
    writeData(Enemy_SprAttrib + 5, 0b00100000);    // set background priority bit
    writeData(Square2SoundQueue, Sfx_GrowPowerUp); // load power-up reveal sound and leave
}

//------------------------------------------------------------------------

// Inputs: maxSpeed = maximum speed; objectOffset = sprite object buffer offset (passed through to
// ImposeGravity)
// Outputs: none
void SMBEngine::ImposeGravitySprObj(uint8_t maxSpeed, uint8_t objectOffset)
{
    writeData(0x02, maxSpeed); // set maximum speed here
    // 0x00 is the movement mode that moves the object downwards; jump to the code that
    // actually moves it
    ImposeGravity(0x00, objectOffset);
}

//------------------------------------------------------------------------

// Inputs: none (reads the current enemy from ObjectOffset)
// Outputs: pair of {y, a} (see GetEnemyBoundBoxOfsArg, which this forwards into)
std::pair<uint8_t, uint8_t> SMBEngine::GetEnemyBoundBoxOfs()
{
    return GetEnemyBoundBoxOfsArg(M(ObjectOffset)); // get enemy object buffer offset
}

//------------------------------------------------------------------------

// Inputs: e = enemy object offset (0-4)
// Outputs: pair of {y = e*4+4, the index into the per-object BoundingBox arrays
// (skipping the player's own box); a = Enemy_OffscreenBits masked to its low nybble}. Callers in
// SMBEnemies.cpp read the registers back rather than the pair, so both have to survive the call.
std::pair<uint8_t, uint8_t> SMBEngine::GetEnemyBoundBoxOfsArg(uint8_t e)
{
    // multiply the offset by four, then add four to skip the player's bounding box
    const uint8_t boundBoxIdx = (uint8_t)((e << 2) + 0x04);
    // get offscreen bits for enemy object, saving the low nybble
    const uint8_t offscreenBits = M(Enemy_OffscreenBits) & 0b00001111;

    y = boundBoxIdx;
    a = offscreenBits;
    return {boundBoxIdx, offscreenBits};
}

//------------------------------------------------------------------------

// Inputs: objectOffset = object offset
// Outputs: x = M(ObjectOffset) (restored via CheckRightScreenBBox, propagated to whichever routine
// resumes after this)
void SMBEngine::SetupEOffsetFBBox(uint8_t objectOffset)
{
    // add 1 to the offset to properly address the bounding box, and use 1 as the
    // relative-position index here for the same reason
    const uint8_t boundBoxCtrlIdx = (uint8_t)(objectOffset + 0x01);
    // do a sub to get the coordinates of the bounding box
    const uint8_t boundBoxIdx = BoundingBoxCore(boundBoxCtrlIdx, 0x01);

    // jump to handle offscreen coordinates of bounding box
    CheckRightScreenBBox(boundBoxCtrlIdx, boundBoxIdx);
}

//------------------------------------------------------------------------

// do collision detection subroutine for sprite object
// Inputs: coordSelector, objectOffset, cornerIdx (see BlockBufferCollision)
// Outputs: none (the block buffer content BlockBufferCollision returns is discarded here)
void SMBEngine::BBChk_E(uint8_t coordSelector, uint8_t objectOffset, uint8_t cornerIdx)
{
    BlockBufferCollision(coordSelector, objectOffset, cornerIdx);
    x = M(ObjectOffset); // get object offset
}

//------------------------------------------------------------------------

// Inputs: coordSelector = which coordinate's low nybble to also report (0 = vertical/Y, nonzero =
// horizontal/X); objectOffset = sprite object buffer offset; cornerIdx = corner-selector index
// into BlockBuffer_X_Adder_data/BlockBuffer_Y_Adder_data (0-27)
// Outputs: return value = the metatile found at that block-buffer position
uint8_t SMBEngine::BlockBufferCollision(uint8_t coordSelector, uint8_t objectOffset, uint8_t cornerIdx)
{
    const uint8_t BlockBuffer_Y_Adder_data[] = {0x04, 0x20, 0x20, 0x08, 0x18, 0x08, 0x18, 0x02, 0x20, 0x20, 0x08, 0x18, 0x08, 0x18,
                                                0x12, 0x20, 0x20, 0x18, 0x18, 0x18, 0x18, 0x18, 0x14, 0x14, 0x06, 0x06, 0x08, 0x10};

    const uint8_t BlockBuffer_X_Adder_data[] = {0x08, 0x03, 0x0c, 0x02, 0x02, 0x0d, 0x0d, 0x08, 0x03, 0x0c, 0x02, 0x02, 0x0d, 0x0d,
                                                0x08, 0x03, 0x0c, 0x02, 0x02, 0x0d, 0x0d, 0x08, 0x00, 0x10, 0x04, 0x14, 0x04, 0x04};

    writeData(0x04, cornerIdx); // save the corner index here

    // add horizontal coordinate of object to value obtained using the corner index as offset
    const uint32_t wide =
        ((M(SprObject_PageLoc + objectOffset) << 8) | M(SprObject_X_Position + objectOffset)) + BlockBuffer_X_Adder_data[cornerIdx];
    writeData(0x05, LOBYTE(wide)); // store here

    // put the LSB of the page location (plus the carry) above the stored value, and
    // effectively move the high nybble to the lower; the LSB which became the MSB will be
    // d4 at this point
    const uint8_t blockColumn = (uint8_t)(((HIBYTE(wide) & 0x01) << 7) | (M(0x05) >> 1)) >> 3;
    GetBlockBufferAddr(blockColumn); // get address of block buffer into $06, $07

    // add the vertical coordinate of the object to the value obtained using the corner
    // index as offset, mask out the low nybble, and subtract 32 pixels for the status bar
    const uint8_t blockRow =
        (uint8_t)(((uint8_t)(M(SprObject_Y_Position + objectOffset) + BlockBuffer_Y_Adder_data[cornerIdx]) & 0b11110000) - 0x20);
    writeData(0x02, blockRow); // store result here

    // check current content of block buffer, using the row as offset, and store it here
    writeData(0x03, M(W(0x06) + blockRow));

    // report the low nybble of the vertical coordinate, or of the horizontal one if the
    // coordinate selector is set
    const uint8_t coordinate = coordSelector == 0 ? M(SprObject_Y_Position + objectOffset)  // RetYC
                                                  : M(SprObject_X_Position + objectOffset); // RetXC
    writeData(0x04, coordinate & 0b00001111);                                               // store masked out result here

    // Skip_9 in SMBPlayer.cpp reads the block content back out of the register rather than
    // taking the return value
    a = M(0x03); // get saved content of block buffer
    return a;    // and leave
}

//------------------------------------------------------------------------

// Inputs: tileNumber = tile number; spritePairIdx, oamSlot = sprite data offsets (see
// DrawSpriteObject)
// Outputs: pair of {spritePairIdx+2, oamSlot+8} (see DrawSpriteObject)
std::pair<uint8_t, uint8_t> SMBEngine::DrawOneSpriteRow(uint8_t tileNumber, uint8_t spritePairIdx, uint8_t oamSlot)
{
    writeData(0x01, tileNumber);
    return DrawSpriteObject(spritePairIdx, oamSlot); // draw them
}

//------------------------------------------------------------------------

// Inputs: oamSlot = OAM sprite slot index
// Outputs: none
void SMBEngine::DrawFirebar(uint8_t oamSlot)
{
    const uint8_t quarterFrame = M(FrameCounter) >> 2; // get frame counter and divide by four

    // mask out all but the last bit to set either tile $64 or $65 as the fireball tile,
    // so that the tile changes every four frames
    writeData(Sprite_Tilenumber + oamSlot, (quarterFrame & 0x01) ^ 0x64);

    // $02 sets the palette in the attribute byte; every eight frames, flip both ways instead
    const bool flipBothWays = ((quarterFrame >> 1) & 0x01) != 0;
    writeData(Sprite_Attributes + oamSlot, flipBothWays ? 0b11000010 : 0x02); // FireA
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::RelativePlayerPosition()
{
    // the offsets for the relative coordinates routine correspond to the player object,
    // and getting the coordinates
    RelWOfs(0x00, 0x00);
}

//------------------------------------------------------------------------

// get the coordinates
// Inputs: objectOffset, relPosIdx (see GetObjRelativePosition)
// Outputs: none
void SMBEngine::RelWOfs(uint8_t objectOffset, uint8_t relPosIdx)
{
    GetObjRelativePosition(objectOffset, relPosIdx);
    x = M(ObjectOffset); // return original offset and leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: x = M(ObjectOffset) (restored via VariableObjOfsRelPos, propagated to whichever
// drawing routine the caller runs next)
void SMBEngine::RelativeEnemyPosition()
{
    // get coordinates of enemy object relative to the screen; x is the object's position
    // within its group, as the caller left it
    VariableObjOfsRelPos(0x01, x, 0x01);
}

//------------------------------------------------------------------------

// Inputs: baseValue = base value (e.g. 1 to skip a byte and land on the enemy offset); addend =
// the object's position within its group; relPosIdx = relative-position index (forwarded to
// GetObjRelativePosition)
// Outputs: x = M(ObjectOffset) (restored object offset)
void SMBEngine::VariableObjOfsRelPos(uint8_t baseValue, uint8_t addend, uint8_t relPosIdx)
{
    writeData(0x00, addend); // store the value to add to the base value here
    // add the base value to the value stored, and use the sum as the enemy offset
    GetObjRelativePosition((uint8_t)(baseValue + M(0x00)), relPosIdx);
    x = M(ObjectOffset); // reload old object offset and leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::GetPlayerOffscreenBits()
{
    // the offsets are for player-specific variables, and getting offscreen information
    // about the player
    GetOffScreenBitsSet(0x00, 0x00);
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::GetEnemyOffscreenBits()
{
    // add 1 byte in order to get the enemy offset, and put the offscreen bits in
    // Enemy_OffscreenBits; x is the base object offset, as the caller left it
    SetOffscrBitsOffset(0x01, x, 0x01);
}

//------------------------------------------------------------------------

// Inputs: addend = addend (e.g. 1 to reach the enemy offset); baseObjectOffset = base object
// offset; offscrArrayOffset = offscreen-bits-array offset (forwarded to GetOffScreenBitsSet)
// Outputs: none
void SMBEngine::SetOffscrBitsOffset(uint8_t addend, uint8_t baseObjectOffset, uint8_t offscrArrayOffset)
{
    writeData(0x00, baseObjectOffset);
    // add the base object offset to the addend to get the appropriate offset
    GetOffScreenBitsSet((uint8_t)(addend + M(0x00)), offscrArrayOffset);
}

//------------------------------------------------------------------------

// Inputs: objectOffset = sprite object buffer offset; offscrArrayOffset = offscreen-bits-array
// offset (selects where the result is stored, e.g. player vs. enemy)
// Outputs: none
void SMBEngine::GetOffScreenBitsSet(uint8_t objectOffset, uint8_t offscrArrayOffset)
{
    const uint8_t verticalBits = RunOffscrBitsSubs(objectOffset);
    // move the low nybble to the high nybble and mask it together with the horizontal
    // nybble the sub left in $00, then store both here
    writeData(0x00, (uint8_t)(verticalBits << 4) | M(0x00));

    // get the value here and store it elsewhere, at the offscreen bits offset
    writeData(SprObject_OffscrBits + offscrArrayOffset, M(0x00));

    x = M(ObjectOffset);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: return value = saved value from MoveObjectHorizontally; x = M(ObjectOffset) (restored
// enemy offset)
uint8_t SMBEngine::MoveEnemyHorizontally(uint8_t e)
{
    // increment the offset for the enemy offset, and position the object horizontally
    // according to counters, returning with the saved value
    const uint8_t saved = MoveObjectHorizontally((uint8_t)(e + 1));
    x = M(ObjectOffset); // put enemy offset back in X and leave
    return saved;
}

//------------------------------------------------------------------------

// Inputs: objectOffset = sprite object buffer offset
// Outputs: return value = carry-plus-high-nybble value from the horizontal movement force update
uint8_t SMBEngine::MoveObjectHorizontally(uint8_t objectOffset)
{
    // get the currently saved value (horizontal speed, secondary counter, whatever) and
    // move its low nybble to high, storing the result here
    writeData(0x01, (uint8_t)(M(SprObject_X_Speed + objectOffset) << 4));

    // get the saved value again and move the high nybble to low, altering the high nybble
    // if the result is negative
    const uint8_t speed = M(SprObject_X_Speed + objectOffset) >> 4;
    writeData(0x00, speed >= 0x08 ? speed | 0b11110000 : speed); // SaveXSpd: save result here

    // UseAdder: save the high bits of the horizontal adder here
    writeData(0x02, (M(0x00) & 0x80) != 0 ? 0xff : 0x00);

    const uint32_t force = M(SprObject_X_MoveForce + objectOffset) + M(0x01); // add low nybble moved to high
    writeData(SprObject_X_MoveForce + objectOffset, LOBYTE(force));           // store result here
    const bool carry = HIBYTE(force) != 0;                                    // the original saves this carry on the stack for the end

    // pageloc:position is one 16-bit quantity, and $02:$00 the signed 16-bit amount
    // to move the object by (the high nybble moved to low, plus $f0 if necessary)
    const uint32_t wide = ((M(SprObject_PageLoc + objectOffset) << 8) | M(SprObject_X_Position + objectOffset)) +
                          ((M(0x02) << 8) | M(0x00)) + (carry ? 1 : 0);
    writeData(SprObject_X_Position + objectOffset, LOBYTE(wide)); // to object's horizontal position
    writeData(SprObject_PageLoc + objectOffset, HIBYTE(wide));    // and the object's page location and save

    // add the old carry to the high nybble moved to low; callers in SMBPlayer.cpp and
    // SMBEnemies.cpp read this back out of the register rather than taking the return value
    a = (uint8_t)((carry ? 1 : 0) + M(0x00));
    return a; // ExXMove: and leave
}

//------------------------------------------------------------------------

// Inputs: pointsControl = points-control value; e = enemy object buffer offset
//
// Returns the x-coordinate of the created floatey. This is normally unused, but in one place
// the result is erroneously passed on to ChkToStunEnemies, causing this bug: https://themushroomkingdom.net/bugs/7
//
uint8_t SMBEngine::SetupFloateyNumber(uint8_t pointsControl, uint8_t e)
{
    writeData(FloateyNum_Control + e, pointsControl);         // set number of points control
    writeData(FloateyNum_Timer + e, 0x30);                    // set timer for floatey numbers
    writeData(FloateyNum_Y_Pos + e, M(Enemy_Y_Position + e)); // set vertical coordinate

    a = M(Enemy_Rel_XPos);
    writeData(FloateyNum_X_Pos + e, a); // set horizontal coordinate and leave
    return a;
    // ExSFN
}

//------------------------------------------------------------------------

// Inputs: boundBoxIdx = bounding-box index of the object to test against the player (the player's
// own box is always index 0, so x needs no input)
// Outputs: y = restored to the input value (see SprObjectCollisionCore)
bool SMBEngine::PlayerCollisionCore(uint8_t boundBoxIdx)
{
    // index 0 is the player's own bounding box, which is what we compare against
    return SprObjectCollisionCore(0x00, boundBoxIdx);
}

//------------------------------------------------------------------------

// Inputs: objIdx1, objIdx2 = the two objects' bounding-box indices to compare
// Outputs: y = restored to the input value on every return path; the bool return communicates
// whether a collision was found
bool SMBEngine::SprObjectCollisionCore(uint8_t objIdx1, uint8_t objIdx2)
{
    // whether the two boxes collide along the one axis the offsets currently address
    const auto coordinatesCollide = [this](uint8_t idx1, uint8_t idx2) -> bool
    {
        const uint8_t secondUL = M(BoundingBox_UL_Corner + idx2); // compare left/top coordinates

        if (secondUL < M(BoundingBox_UL_Corner + idx1))
        { // if first left/top => second, branch
            if (secondUL >= M(BoundingBox_LR_Corner + idx1))
            { // if first left/top < second right/bottom, branch elsewhere
                if (secondUL == M(BoundingBox_LR_Corner + idx1))
                {
                    return true; // if somehow equal, collision
                }
                // if somehow greater, check to see if the bottom of the second box wraps
                const uint8_t secondLR = M(BoundingBox_LR_Corner + idx2);
                if (secondLR < M(BoundingBox_UL_Corner + idx2))
                {
                    return true; // if somehow less, vertical wrap collision
                }
                // if equal or greater than the top of the first box, collision; note that
                // horizontal wrapping never occurs
                return secondLR >= M(BoundingBox_UL_Corner + idx1);
            } // SecondBoxVerticalChk

            // check to see if the vertical bottom of the box wraps, which is a collision
            if (M(BoundingBox_LR_Corner + idx1) < M(BoundingBox_UL_Corner + idx1))
            {
                return true;
            }
            // otherwise compare horizontal right or vertical bottom; equal or greater is
            // a collision
            return M(BoundingBox_LR_Corner + idx2) >= M(BoundingBox_UL_Corner + idx1);
        } // FirstBoxGreater

        if (secondUL == M(BoundingBox_UL_Corner + idx1))
        {
            return true; // if first coordinate = second, collision
        }
        if (secondUL <= M(BoundingBox_LR_Corner + idx1))
        { // if left/top of first less than or equal to right/bottom of second, collision
            return true;
        }
        if (secondUL <= M(BoundingBox_LR_Corner + idx2))
        {
            return false; // if less than or equal, no collision
        }
        // otherwise compare bottom of first to top of second; equal or greater is a collision
        return M(BoundingBox_LR_Corner + idx2) >= M(BoundingBox_UL_Corner + idx1);
    };

    writeData(0x06, objIdx2); // save contents of Y here
    writeData(0x07, 0x01);    // save value 1 here as counter, compare horizontal coordinates first

    bool collisionFound = true;
    uint8_t idx1 = objIdx1;
    uint8_t idx2 = objIdx2;

    do // CollisionCoreLoop
    {
        if (!coordinatesCollide(idx1, idx2))
        {
            // NoCollisionFound: if the horizontal coordinates do not collide we do not
            // bother checking the vertical ones, because what's the point?
            collisionFound = false;
            break;
        }

        // CollisionFound: increment offsets on both objects to check the vertical
        // coordinates, and decrement the counter to reflect this
        ++idx1;
        ++idx2;
        --M(0x07);
    } while ((M(0x07) & 0x80) == 0); // if counter not expired, branch to loop
    // otherwise we already did both sets, therefore collision

    y = M(0x06); // load original value set here earlier, then leave
    return collisionFound;
}

//------------------------------------------------------------------------

// Inputs: scrollAmount = scroll amount
// Outputs: none
void SMBEngine::ScrollScreen(uint8_t scrollAmount)
{
    writeData(ScrollAmount, scrollAmount); // save value here
    // add to the value already set here and save that as the new value
    writeData(ScrollThirtyTwo, (uint8_t)(scrollAmount + M(ScrollThirtyTwo)));

    // add to the left side coordinate, which is one 16-bit page:coordinate
    const uint32_t wide = ((M(ScreenLeft_PageLoc) << 8) | M(ScreenLeft_X_Pos)) + scrollAmount;
    writeData(ScreenLeft_X_Pos, LOBYTE(wide));   // save as new left side coordinate
    writeData(HorizontalScroll, LOBYTE(wide));   // save here also
    writeData(ScreenLeft_PageLoc, HIBYTE(wide)); // add carry to page location for left side of the screen

    // get the LSB of the page location and save it as a temp variable for the mirror
    writeData(0x00, HIBYTE(wide) & 0x01);
    // get the PPU register 1 mirror and save all bits except d0, then get the saved bit
    // here and save it in the mirror, to be used to set the name table later
    writeData(Mirror_PPU_CTRL_REG1, (M(Mirror_PPU_CTRL_REG1) & 0b11111110) | M(0x00));

    GetScreenPosition();                  // figure out where the right side is
    writeData(ScrollIntervalTimer, 0x08); // set scroll timer (residual, not used elsewhere)

    ChkPOffscr(); // skip this part
}

//------------------------------------------------------------------------

// set maximum speed in A
// Inputs: none
// Outputs: none
void SMBEngine::SetHiMax()
{
    // 3 is the maximum speed; x and y are the enemy offset and the downward movement
    // amount, as the caller left them
    SetXMoveAmt(0x03, x, y);
}

//------------------------------------------------------------------------

// set movement amount here
// Inputs: maxSpeed = maximum speed (from the caller, e.g. SetHiMax); e = enemy object
// buffer offset; downwardMoveAmt = downward movement amount, low byte (saved to zero-page 0x00
// for ImposeGravity)
// Outputs: x = M(ObjectOffset) (restored enemy offset)
void SMBEngine::SetXMoveAmt(uint8_t maxSpeed, uint8_t e, uint8_t downwardMoveAmt)
{
    writeData(0x00, downwardMoveAmt); // set movement amount here
    // increment the offset for the enemy offset, and do a sub to move the enemy downwards
    ImposeGravitySprObj(maxSpeed, (uint8_t)(e + 1));
    x = M(ObjectOffset); // get enemy object buffer offset and leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ScrollHandler()
{
    // load value saved here, add the value used by left/right platforms, and save that as
    // the new value here to impose force on the scroll
    const uint8_t playerScroll = (uint8_t)(M(Player_X_Scroll) + M(Platform_X_Scroll));
    writeData(Player_X_Scroll, playerScroll);

    // a bunch of code is skipped if the scroll lock flag is set, if the player is less than
    // 80 pixels to the right, or if the timer related to the player's side collision has
    // not expired
    const bool scrollLocked = M(ScrollLock) != 0 || M(Player_Pos_ForScroll) < 0x50 || M(SideCollisionTimer) != 0;
    // the screen also does not scroll if the value was originally set to zero, or is
    // otherwise negative for left movement
    const bool scrollable = ((uint8_t)(playerScroll - 1) & 0x80) == 0;

    if (!scrollLocked && scrollable)
    {
        // ChkNearMid: if less than 112 pixels to the right, decrement the amount by one,
        // otherwise use the original undecremented value
        if (M(Player_Pos_ForScroll) < 0x70)
        {
            ScrollScreen(playerScroll >= 0x02 ? (uint8_t)(playerScroll - 1) : playerScroll);
        }
        else
        {
            ScrollScreen(M(Player_X_Scroll));
        }
        return;
    }

    // InitScrlAmt: initialize value here
    writeData(ScrollAmount, 0x00);

    ChkPOffscr();
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: a = 0; KillAllEnemies in SMBEnemies.cpp empties the frenzy buffer with whatever this
// leaves behind, so it has to survive the call
void SMBEngine::EraseEnemyObject(uint8_t e)
{
    // clear all enemy object variables
    writeData(Enemy_Flag + e, 0x00);
    writeData(Enemy_ID + e, 0x00);
    writeData(Enemy_State + e, 0x00);
    writeData(FloateyNum_Control + e, 0x00);
    writeData(EnemyIntervalTimer + e, 0x00);
    writeData(ShellChainCounter + e, 0x00);
    writeData(Enemy_SprAttrib + e, 0x00);
    writeData(EnemyFrameTimer + e, 0x00);

    a = 0x00;
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::OffscreenBoundsCheck(uint8_t e)
{
    const uint8_t enemyId = M(Enemy_ID + e); // check for cheep-cheep object
    if (enemyId == FlyingCheepCheep)
    {
        return;
    }

    uint8_t leftEdge = M(ScreenLeft_X_Pos); // get horizontal coordinate for left side of screen
    // anything other than a hammer bro or a piranha plant will be erased sooner than those
    // two if too far left; this compare's carry is what ExtendLB subtracts with
    bool carry = enemyId >= PiranhaPlant;
    if (enemyId == HammerBro || enemyId == PiranhaPlant)
    {
        // LimitB: add 57 pixels to the coordinate -- 56, plus the one carried in by the
        // compare that sent us here, which found the identifier equal and so always left
        // the carry set
        const uint32_t limited = leftEdge + 0x39;
        leftEdge = LOBYTE(limited);
        carry = HIBYTE(limited) != 0; // and this add's carry is what ExtendLB subtracts with
    }

    // ExtendLB: subtract 72 pixels regardless of enemy object
    const uint32_t left = ((M(ScreenLeft_PageLoc) << 8) | leftEdge) - 0x48 - (carry ? 0 : 1);
    writeData(0x01, LOBYTE(left)); // store result here
    writeData(0x00, HIBYTE(left)); // store result here

    // the original never clears the carry here either, so a left edge that did not borrow
    // pushes the right edge one pixel further out
    const bool leftDidNotBorrow = (left & 0x10000) == 0;
    // add 72 pixels to the right side horizontal coordinate
    const uint32_t right = ((M(ScreenRight_PageLoc) << 8) | M(ScreenRight_X_Pos)) + 0x48 + (leftDidNotBorrow ? 1 : 0);
    writeData(0x03, LOBYTE(right)); // store result here
    writeData(0x02, HIBYTE(right)); // and store result here

    // the enemy object and the modified left edge are each one 16-bit page:coordinate
    const uint32_t fromLeft = ((M(Enemy_PageLoc + e) << 8) | M(Enemy_X_Position + e)) - ((M(0x00) << 8) | M(0x01));
    if ((HIBYTE(fromLeft) & 0x80) == 0)
    { // if enemy object is too far left, branch to erase it
        // the enemy object and the modified right edge are each one 16-bit page:coordinate
        const uint32_t fromRight = ((M(Enemy_PageLoc + e) << 8) | M(Enemy_X_Position + e)) - ((M(0x02) << 8) | M(0x03));
        if ((HIBYTE(fromRight) & 0x80) != 0)
        {
            return; // if enemy object is on the screen, leave, do not erase enemy
        }
        // if at this point, the enemy is offscreen to the right, so check its state
        if (M(Enemy_State + e) == HammerBro)
        {
            return;
        }
        // erase all others too far to the right
        if (enemyId == PiranhaPlant || enemyId == FlagpoleFlagObject || enemyId == StarFlagObject || enemyId == JumpspringObject)
        {
            return;
        }
    } // TooFar: erase object if necessary
    EraseEnemyObject(e);

    // ExScrnBd: leave
}

//------------------------------------------------------------------------

// Inputs: value = value to dump; baseOffset = base sprite-data offset
// Outputs: none (a and y are left unchanged)
void SMBEngine::DumpSixSpr(uint8_t value, uint8_t baseOffset)
{
    writeData(Sprite_Data + 20 + baseOffset, value); // dump the value
    writeData(Sprite_Data + 16 + baseOffset, value); // into third row sprites
    DumpFourSpr(value, baseOffset);
}

//------------------------------------------------------------------------

// Inputs: value = value to dump; baseOffset = base sprite-data offset
// Outputs: none (a and y are left unchanged)
void SMBEngine::DumpFourSpr(uint8_t value, uint8_t baseOffset)
{
    writeData(Sprite_Data + 12 + baseOffset, value); // into second row sprites
    DumpThreeSpr(value, baseOffset);
}

//------------------------------------------------------------------------

// Inputs: value = value to dump; baseOffset = base sprite-data offset
// Outputs: none (a and y are left unchanged)
void SMBEngine::DumpThreeSpr(uint8_t value, uint8_t baseOffset)
{
    writeData(Sprite_Data + 8 + baseOffset, value);
    DumpTwoSpr(value, baseOffset);
}

//------------------------------------------------------------------------

// Inputs: value = value to dump; baseOffset = base sprite-data offset
// Outputs: none (a and y are left unchanged)
void SMBEngine::DumpTwoSpr(uint8_t value, uint8_t baseOffset)
{
    writeData(Sprite_Data + 4 + baseOffset, value); // and into first row sprites
    writeData(Sprite_Data + baseOffset, value);
}

//------------------------------------------------------------------------

// Inputs: rowSelectorBase = row selector base (e.g. 0x00/0x04/0x08/0x10, per caller); e
// = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveESprRowOffscreen(uint8_t rowSelectorBase, uint8_t e)
{
    // the row selector plus the sprite data offset is used as the offset
    const uint8_t spriteOffset = (uint8_t)(rowSelectorBase + M(Enemy_SprDataOffset + e));
    DumpTwoSpr(0xf8, spriteOffset); // move first row of sprites offscreen
}

//------------------------------------------------------------------------

// Inputs: none (reads the current enemy from ObjectOffset)
// Outputs: x = M(ObjectOffset). DrawPowerUp in SMBEnemies.cpp leaves the power-up type in x and
// tail-calls this to get the enemy offset back, which the ProcELoop loop in GameCoreRoutine
// (SMBGame.cpp) then goes on to use as its loop counter.
void SMBEngine::SprObjectOffscrChk()
{
    const uint8_t e = M(ObjectOffset);                    // get enemy buffer offset
    const uint8_t offscreenBits = M(Enemy_OffscreenBits); // check offscreen information

    x = e;

    if ((offscreenBits & 0b00000100) != 0)
    { // d2: set for right column sprites and move them offscreen
        MoveESprColOffscreen(0x04, e);
    } // LcChk
    if ((offscreenBits & 0b00001000) != 0)
    { // d3: set for left column sprites and move them offscreen
        MoveESprColOffscreen(0x00, e);
    } // Row3C
    if ((offscreenBits & 0b00100000) != 0)
    { // d5: set for the third row of sprites and move them offscreen
        MoveESprRowOffscreen(0x10, e);
    } // Row23C
    if ((offscreenBits & 0b01000000) != 0)
    { // d6: set for the second and third rows and move them offscreen
        MoveESprRowOffscreen(0x08, e);
    } // AllRowC
    if ((offscreenBits & 0b10000000) == 0)
    {
        return; // d7 is what moves all the sprites offscreen
    }

    MoveESprRowOffscreen(0x00, e); // move all sprites offscreen

    if (M(Enemy_ID + e) == Podoboo)
    {
        return; // skip this part if found, we do not want to erase podoboo!
    }
    if (M(Enemy_Y_HighPos + e) != 0x02)
    {
        return; // check high byte of vertical position
    }

    EraseEnemyObject(e); // what it says

    // ExEGHandler
}

//------------------------------------------------------------------------

// Inputs: rowSelectorBase = row selector base; e = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveESprColOffscreen(uint8_t rowSelectorBase, uint8_t e)
{
    // the row selector plus the sprite data offset is used as the offset
    const uint8_t spriteOffset = (uint8_t)(rowSelectorBase + M(Enemy_SprDataOffset + e));
    // move the first and second row sprites in the column offscreen, then the third
    writeData(Sprite_Data + 16 + spriteOffset, MoveColOffscreen(spriteOffset));
}

//------------------------------------------------------------------------

// Inputs: yPosOffset = sprite Y-position offset
// Outputs: return value = 0xf8 (offscreen Y sentinel), consumed by MoveESprColOffscreen
uint8_t SMBEngine::MoveColOffscreen(uint8_t yPosOffset)
{
    // move offscreen two OAMs on the left side (or two rows of enemy on either side if
    // branched here from the enemy graphics handler)
    writeData(Sprite_Y_Position + yPosOffset, 0xf8);
    writeData(Sprite_Y_Position + 8 + yPosOffset, 0xf8);

    return 0xf8; // ExDBlk
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveD_EnemyVertically(uint8_t e)
{
    // a quick movement amount downwards, unless the enemy state calls for a different one.
    // SetHiMax takes both of these in the registers rather than as arguments
    y = M(Enemy_State + e) == 0x05 ? 0x20 : 0x3d;
    x = e;

    SetHiMax(); // ContVMove: jump to skip the rest of this
}

//------------------------------------------------------------------------

// Inputs: frameSelector = explosion frame/offset selector (0-2); spriteDataBase = sprite data
// offset base
// Outputs: none
void SMBEngine::DrawExplosion_Fireworks(uint8_t frameSelector, uint8_t spriteDataBase)
{
    const uint8_t ExplosionTiles_data[] = {0x68, 0x67, 0x66};

    // get tile number using the frame selector as offset, and dump it into the tile number
    // part of the sprite data, one byte along
    DumpFourSpr(ExplosionTiles_data[frameSelector], (uint8_t)(spriteDataBase + 1));

    x = M(ObjectOffset); // return enemy object buffer offset to X

    // get relative vertical coordinate, for the first and third sprites
    const uint8_t topY = (uint8_t)(M(Fireball_Rel_YPos) - 0x04);
    writeData(Sprite_Y_Position + spriteDataBase, topY);
    writeData(Sprite_Y_Position + 8 + spriteDataBase, topY);
    // and eight pixels down, for the second and fourth sprites
    writeData(Sprite_Y_Position + 4 + spriteDataBase, (uint8_t)(topY + 0x08));
    writeData(Sprite_Y_Position + 12 + spriteDataBase, (uint8_t)(topY + 0x08));

    // get relative horizontal coordinate, for the first and second sprites
    const uint8_t leftX = (uint8_t)(M(Fireball_Rel_XPos) - 0x04);
    writeData(Sprite_X_Position + spriteDataBase, leftX);
    writeData(Sprite_X_Position + 4 + spriteDataBase, leftX);
    // and eight pixels along, for the third and fourth sprites
    writeData(Sprite_X_Position + 8 + spriteDataBase, (uint8_t)(leftX + 0x08));
    writeData(Sprite_X_Position + 12 + spriteDataBase, (uint8_t)(leftX + 0x08));

    // set palette attributes for all sprites, but
    writeData(Sprite_Attributes + spriteDataBase, 0x02);      // set no flip at all for first sprite
    writeData(Sprite_Attributes + 4 + spriteDataBase, 0x82);  // set vertical flip for second sprite
    writeData(Sprite_Attributes + 8 + spriteDataBase, 0x42);  // set horizontal flip for third sprite
    writeData(Sprite_Attributes + 12 + spriteDataBase, 0xc2); // set both flips for fourth sprite
    // we are done
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::EnemyTurnAround(uint8_t e)
{
    const uint8_t enemyId = M(Enemy_ID + e); // check for specific enemies

    // piranha plants, lakitus and hammer bros are left alone
    if (enemyId == PiranhaPlant || enemyId == Lakitu || enemyId == HammerBro)
    {
        return;
    }
    // spinies and green paratroopas are turned around, as is any OTHER enemy object < $07
    if (enemyId == Spiny || enemyId == GreenParatroopaJump || enemyId < 0x07)
    {
        RXSpd(e);
    }
}

//------------------------------------------------------------------------

// load horizontal speed
// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::RXSpd(uint8_t e)
{
    // Negate the enemy's x speed:
    writeData(Enemy_X_Speed + e, -M(Enemy_X_Speed + e));
    // Turn the enemy around:
    writeData(Enemy_MovingDir + e, M(Enemy_MovingDir + e) ^ 0b00000011);
}

//------------------------------------------------------------------------

// Inputs: baseOffset = base sprite-data offset
// Outputs: none
void SMBEngine::MoveSixSpritesOffscreen(uint8_t baseOffset)
{
    DumpSixSpr(0xf8, baseOffset); // set offscreen coordinate if jumping here
}

//------------------------------------------------------------------------

// set starting position to override
// Inputs: none
// Outputs: none
void SMBEngine::SetEntr()
{
    writeData(AltEntranceControl, 0x02);
    ChgAreaMode(); // set modes
}

//------------------------------------------------------------------------

// set flag to disable screen output
// Inputs: none
// Outputs: a = 0; NextArea in SMBGame.cpp writes it straight into HalfwayPage, so it has to
// survive the call
void SMBEngine::ChgAreaMode()
{
    ++M(DisableScreenFlag);
    writeData(OperMode_Task, 0x00);        // set secondary mode of operation
    writeData(Sprite0HitDetectFlag, 0x00); // disable sprite 0 check

    a = 0x00;
}

//------------------------------------------------------------------------

// Inputs: species = species stunned; e = enemy object buffer offset
// Outputs: none
void SMBEngine::ChkToStunEnemies(uint8_t species, uint8_t e)
{
    // Should we clip this enemy's wings on top of simply stunning it?
    const bool demote = species == 0x09            // unused value
                        || species == PiranhaPlant // included for some reason...
                        || species == GreenParatroopaJump || species == RedParatroopa || species == GreenParatroopaFly;

    if (demote)
    {
        // "species & 1" is a Green or Red Koopa.
        writeData(Enemy_ID + e, species & 1);
    }

    SetStun(e);
}

//------------------------------------------------------------------------

// load enemy state
// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::SetStun(uint8_t e)
{
    const uint8_t EnemyBGCXSpdData_data[] = {0x10, 0xf0};

    // save the high nybble of the enemy state and set d1 of it
    writeData(Enemy_State + e, (M(Enemy_State + e) & 0b11110000) | 0b00000010);

    --M(Enemy_Y_Position + e);
    --M(Enemy_Y_Position + e); // subtract two pixels from enemy's vertical position

    // set the default vertical speed, unless this is a bloober or the area type is water,
    // in which case change it
    const bool defaultYSpeed = M(Enemy_ID + e) != Bloober && M(AreaType) != 0;
    writeData(Enemy_Y_Speed + e, defaultYSpeed ? 0xfd : 0xff); // SetWYSpd/SetNotW

    // get horizontal difference between player and enemy object: the moving direction is 1
    // if the enemy is to the right of the player, 2 if not
    const uint8_t diffHighByte = PlayerEnemyDiff(e).second;
    const uint8_t movingDir = (diffHighByte & 0x80) != 0 ? 0x02 : 0x01;

    // ChkBBill: if either bullet bill is found, the direction does not change
    const uint8_t enemyId = M(Enemy_ID + e);
    if (enemyId != BulletBill_CannonVar && enemyId != BulletBill_FrenzyVar)
    {
        writeData(Enemy_MovingDir + e, movingDir); // store as moving direction
    }

    // NoCDirF: decrement and use as offset to get the proper horizontal speed, then store
    writeData(Enemy_X_Speed + e, EnemyBGCXSpdData_data[movingDir - 1]);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: x = M(ObjectOffset) (restored via the GetMaskedOffScrBits/SetupEOffsetFBBox chain,
// propagated to routines such as PlayerEnemyCollision that the caller runs next)
void SMBEngine::GetEnemyBoundBox(uint8_t e)
{
    writeData(0x00, 0x48);        // store bitmask here for now
    GetMaskedOffScrBits(e, 0x44); // store another bitmask here for now and jump
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset; defaultBitmask = default bitmask (from the
// caller, e.g. 0x44)
// Outputs: x = current object offset (either left unchanged, or explicitly restored on the
// SetupEOffsetFBBox path)
void SMBEngine::GetMaskedOffScrBits(uint8_t e, uint8_t defaultBitmask)
{
    // the enemy object and the left side of the screen are each one 16-bit page:coordinate
    const uint32_t wide = ((M(Enemy_PageLoc + e) << 8) | M(Enemy_X_Position + e)) - ((M(ScreenLeft_PageLoc) << 8) | M(ScreenLeft_X_Pos));
    writeData(0x01, LOBYTE(wide)); // store here

    // an enemy object beyond the left edge, or precisely at it, uses the default bitmask;
    // one to the right of the left edge uses the value in $00 instead
    const uint8_t diffHighByte = HIBYTE(wide);
    const bool beyondOrAtLeftEdge = (diffHighByte & 0x80) != 0 || (diffHighByte | M(0x01)) == 0;
    const uint8_t bitmask = beyondOrAtLeftEdge ? defaultBitmask : M(0x00); // CMBits

    // preserve bitwise whatever's in here, and save the masked offscreen bits here
    const uint8_t maskedBits = bitmask & M(Enemy_OffscreenBits);
    writeData(EnemyOffscrBitsMasked + e, maskedBits);

    if (maskedBits != 0)
    {
        MoveBoundBoxOffscreen(e); // if anything set here, branch
        return;
    }

    SetupEOffsetFBBox(e); // otherwise, do something else
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveBoundBoxOffscreen(uint8_t e)
{
    const uint8_t boxOffset = (uint8_t)(e << 2); // multiply offset by 4 and use as offset here

    // load the value into four locations here and leave
    writeData(EnemyBoundingBoxCoord + boxOffset, 0xff);
    writeData(EnemyBoundingBoxCoord + 1 + boxOffset, 0xff);
    writeData(EnemyBoundingBoxCoord + 2 + boxOffset, 0xff);
    writeData(EnemyBoundingBoxCoord + 3 + boxOffset, 0xff);
}

//------------------------------------------------------------------------

// Inputs: subroutineNum = subroutine number to run next frame
// Outputs: none
void SMBEngine::UpToFiery(uint8_t subroutineNum)
{
    // 0 is the value to be used as the new player state; set values to stop certain
    // things in motion
    SetPRout(subroutineNum, 0x00);

    // NoPUp
}

//------------------------------------------------------------------------

// set new player state
// Inputs: subroutineNum = subroutine number to run next frame
// Outputs: none
void SMBEngine::SetKRout(uint8_t subroutineNum) { SetPRout(subroutineNum, 0x01); }

//------------------------------------------------------------------------

// load new value to run subroutine on next frame
// Inputs: subroutineNum = subroutine number; newPlayerState = new player state
// Outputs: none
void SMBEngine::SetPRout(uint8_t subroutineNum, uint8_t newPlayerState)
{
    writeData(GameEngineSubroutine, subroutineNum); // run this subroutine on the next frame
    writeData(Player_State, newPlayerState);        // store new player state
    writeData(TimerControl, 0xff);                  // set master timer control flag to halt timers
    writeData(ScrollAmount, 0x00);                  // initialize scroll speed

    ExInjColRoutines();
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ExInjColRoutines()
{
    x = M(ObjectOffset); // get enemy offset and leave
}

//------------------------------------------------------------------------
// Inputs: comparisonValue = enemy id/state comparison value; e = enemy object buffer
// offset
// Outputs: none
void SMBEngine::ChkForDemoteKoopa(uint8_t comparisonValue, uint8_t e)
{
    const uint8_t RevivalRateData_data[] = {0x10, 0x0b};

    const uint8_t DemotedKoopaXSpdData_data[] = {0x08, 0xf8};

    if (comparisonValue >= 0x09)
    {
        // demote koopa paratroopas to ordinary troopas
        writeData(Enemy_ID + e, comparisonValue & 0b00000001);
        writeData(Enemy_State + e, 0x00); // return enemy to normal state
        SetupFloateyNumber(0x03, e);      // award 400 points to the player
        InitVStf(e);                      // nullify physics-related thing and vertical speed

        const uint8_t facingIdx = EnemyFacePlayer(e); // turn enemy around if necessary
        // set appropriate moving speed based on direction
        writeData(Enemy_X_Speed + e, DemotedKoopaXSpdData_data[facingIdx]);
    } // HandleStompedShellE
    else // then move onto something else
    {
        writeData(Enemy_State + e, 0x04); // set defeated state for enemy
        ++M(StompChainCounter);           // increment the stomp counter

        // award points according to whatever is in the stomp counter, plus the stomp timer
        SetupFloateyNumber((uint8_t)(M(StompChainCounter) + M(StompTimer)), e);
        ++M(StompTimer); // increment stomp timer of some sort

        // check primary hard mode flag and load the timer setting according to it, setting
        // it as the enemy timer to revive the stomped enemy
        writeData(EnemyIntervalTimer + e, RevivalRateData_data[M(PrimaryHardMode)]);
    }

    // SBnce: set player's vertical speed for bounce, and then leave!!!
    writeData(Player_Y_Speed, 0xfc);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::ShellOrBlockDefeat(uint8_t e)
{
    const uint8_t enemyId = M(Enemy_ID + e); // check for piranha plant

    // a piranha plant is moved down; every other enemy passes its identifier to the sub
    // below, which is what makes the middle of its range unreachable
    uint8_t stunValue = enemyId;
    if (enemyId == PiranhaPlant)
    {
        // add 24 pixels, plus the one carried in by the compare above
        stunValue = (uint8_t)(M(Enemy_Y_Position + e) + 0x19);
        writeData(Enemy_Y_Position + e, stunValue);
    }

    ChkToStunEnemies(stunValue, e); // StnE: do yet another sub

    // mask out 2 MSB of enemy object's state, set d5 to defeat the enemy, and save as new state
    writeData(Enemy_State + e, (M(Enemy_State + e) & 0b00011111) | 0b00100000);

    const uint8_t defeatedId = M(Enemy_ID + e);

    uint8_t points = 0x02; // award 200 points by default
    if (defeatedId == HammerBro)
    {
        points = 0x06; // award 1000 points for hammer bro
    } // GoombaPoints
    if (defeatedId == Goomba)
    {
        points = 0x01; // award 100 points for goomba
    }

    EnemySmackScore(points, e);
}

//------------------------------------------------------------------------

// Inputs: pointsControl = points-control value; e = enemy object buffer offset
// Outputs: none
void SMBEngine::EnemySmackScore(uint8_t pointsControl, uint8_t e)
{
    SetupFloateyNumber(pointsControl, e);         // update necessary score variables
    writeData(Square1SoundQueue, Sfx_EnemySmack); // play smack enemy sound

    // ExHCF: and now let's leave
}

//------------------------------------------------------------------------

// turn the enemy around, if necessary
// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::LInj(uint8_t e)
{
    EnemyTurnAround(e);
    InjurePlayer(); // go back to hurt player
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::InjurePlayer()
{
    // check again to see if the injured invincibility timer is at zero, and leave if not
    if (M(InjuryTimer) != 0)
    {
        ExInjColRoutines();
        return;
    }

    ForceInjury(0);
}

//------------------------------------------------------------------------

// Inputs: mustBeZero = must be 0 on entry (an implicit invariant from InjurePlayer's
// InjuryTimer==0 check); used directly to reset PlayerStatus to "small" on the non-kill path
// Outputs: none
void SMBEngine::ForceInjury(uint8_t mustBeZero)
{
    const uint8_t playerStatus = M(PlayerStatus); // check player's status
    if (playerStatus == 0)
    { // KillPlayer: halt the player's horizontal movement by initializing the speed with
        // the zero the status just gave us
        writeData(Player_X_Speed, playerStatus);
        writeData(EventMusicQueue, playerStatus + 1); // set event music queue to death music
        writeData(Player_Y_Speed, 0xfc);              // set new vertical speed

        SetKRout(0x0b); // branch to set player's state and other things
        return;
    }

    writeData(PlayerStatus, mustBeZero); // otherwise set player's status to small
    writeData(InjuryTimer, 0x08);        // set injured invincibility timer
    writeData(Square1SoundQueue, 0x10);  // play pipedown/injury sound
    GetPlayerColors();                   // change player's palette if necessary

    SetKRout(0x0a); // set subroutine to run on next frame
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::PlayerEnemyCollision(uint8_t e)
{
    const uint8_t StompedEnemyPtsData_data[] = {0x02, 0x06, 0x05, 0x06};

    const uint8_t KickedShellPtsData_data[] = {0x0a, 0x06, 0x04};

    const uint8_t KickedShellXSpdData_data[] = {0x30, 0xd0};

    // EnemyStompedPts: award the points the offset selects, and kill the enemy
    const auto enemyStompedPts = [&](uint8_t ptsIdx)
    {
        // load points data using the offset, and run sub to set floatey number controls
        SetupFloateyNumber(StompedEnemyPtsData_data[ptsIdx], e);

        const uint8_t movingDir = M(Enemy_MovingDir + e); // save enemy movement direction
        SetStun(e);                                       // run sub to kill enemy
        writeData(Enemy_MovingDir + e, movingDir);        // and give it back

        writeData(Enemy_State + e, 0b00100000); // set d5 in enemy state
        InitVStf(e);                            // nullify vertical speed, physics-related thing,
        // and horizontal speed -- the original stores the zero InitVStf leaves in A here,
        // not the $20 it loaded just above
        writeData(Enemy_X_Speed + e, 0x00);

        writeData(Player_Y_Speed, 0xfd); // set player's vertical speed, to give bounce
    };

    // EnemyStomped
    const auto enemyStomped = [&]()
    {
        if (M(Enemy_ID + e) == Spiny)
        {
            InjurePlayer(); // check for spiny, branch to hurt player
            return;
        }

        // otherwise play stomp/swim sound
        writeData(Square1SoundQueue, Sfx_EnemyStomp);

        // the points data offset for stomped enemies starts at zero and is incremented for
        // the enemies worth more
        const uint8_t stompedId = M(Enemy_ID + e);
        if (stompedId == FlyingCheepCheep || stompedId == BulletBill_FrenzyVar || stompedId == BulletBill_CannonVar ||
            stompedId == Podoboo) // (the podoboo is unreachable, due to the earlier checking of it)
        {
            enemyStompedPts(0x00);
        }
        else if (stompedId == HammerBro)
        {
            enemyStompedPts(0x01);
        }
        else if (stompedId == Lakitu)
        {
            enemyStompedPts(0x02);
        }
        else if (stompedId == Bloober)
        {
            enemyStompedPts(0x03);
        }
        else
        {
            ChkForDemoteKoopa(stompedId, e);
        }
    };

    // ChkForPlayerInjury
    const auto chkForPlayerInjury = [&]()
    {
        // check player's vertical speed; the procedure below is performed if the player is
        // moving upwards or not at all, and it is a stomp if moving downwards
        const uint8_t playerYSpeed = M(Player_Y_Speed);
        if ((playerYSpeed & 0x80) == 0 && playerYSpeed != 0)
        {
            enemyStomped();
            return;
        }

        // ChkInj: for enemy objects => $07, add 12 pixels to the player's vertical
        // position; if that is above (less than) the enemy's, it is a stomp
        if (M(Enemy_ID + e) >= Bloober && (uint8_t)(M(Player_Y_Position) + 0x0c) < M(Enemy_Y_Position + e))
        {
            enemyStomped();
            return;
        }

        // ChkETmrs: check stomp timer
        if (M(StompTimer) != 0)
        {
            enemyStomped();
            return;
        }
        // check to see if the injured invincibility timer is still counting down
        if (M(InjuryTimer) != 0)
        {
            ExInjColRoutines(); // branch elsewhere to leave if so
            return;
        }

        // if the player is to the right of the enemy and the enemy is moving to the right,
        // or the player is to its left and it is not, hurt the player; otherwise turn the
        // enemy around first
        const bool playerRightOfEnemy = M(Player_Rel_XPos) >= M(Enemy_Rel_XPos);
        const bool enemyMovingRight = M(Enemy_MovingDir + e) == 0x01;
        if (playerRightOfEnemy == enemyMovingRight)
        {
            InjurePlayer(); // go back to hurt player
        }
        else
        {
            LInj(e); // to turn the enemy around
        }
    };

    // HandlePowerUpCollision
    const auto handlePowerUpCollision = [&]()
    {
        EraseEnemyObject(e);                           // erase the power-up object
        SetupFloateyNumber(0x06, e);                   // award 1000 points to player by default
        writeData(Square2SoundQueue, Sfx_PowerUpGrab); // play the power-up sound

        const uint8_t powerUpType = M(PowerUpType); // check power-up type
        if (powerUpType == 0x03)
        {
            // SetFor1Up: change 1000 points into a 1-up instead, and then leave
            writeData(FloateyNum_Control + e, 0x0b);
            return;
        }
        if (powerUpType >= 0x02)
        {
            // otherwise set star mario invincibility timer, and load the star mario music
            // into the area music queue, then leave
            writeData(StarInvincibleTimer, 0x23);
            writeData(AreaMusicQueue, StarPowerMusic);
            return;
        }

        // Shroom_Flower_PUp
        const uint8_t playerStatus = M(PlayerStatus);
        if (playerStatus == 0)
        {
            // UpToSuper: set player status to super
            writeData(PlayerStatus, 0x01);
            UpToFiery(0x09); // set value to be used by subroutine tree (super)
            return;
        }
        if (playerStatus != 0x01)
        {
            return;
        }

        writeData(PlayerStatus, 0x02); // set player status to fiery
        GetPlayerColors();             // run sub to change colors of player
        UpToFiery(0x0c);               // set value to be used by subroutine tree (fiery)
    };

    // check counter for d0 set
    if ((M(FrameCounter) & 0x01) != 0)
    {
        return; // if set, branch to leave
    }
    // if the player object is completely offscreen, or is down past the 224th pixel row, leave
    if (CheckPlayerVertical())
    {
        return;
    }
    if (M(EnemyOffscrBitsMasked + e) != 0)
    {
        return; // if current enemy is offscreen by any amount, go ahead and leave
    }
    if (M(GameEngineSubroutine) != 0x08)
    {
        return; // if not running the ordinary routine on the next frame, leave
    }
    if ((M(Enemy_State + e) & 0b00100000) != 0)
    {
        return; // if enemy state has d5 set, leave
    }

    // get bounding box offset for current enemy object, and do collision detection on
    // player vs. enemy
    const uint8_t boundBoxIdx = GetEnemyBoundBoxOfs().first;
    const bool collisionFound = PlayerCollisionCore(boundBoxIdx);
    x = M(ObjectOffset); // get enemy object buffer offset

    if (!collisionFound)
    {
        // NoPECol: clear d0 of the current enemy object's collision bits
        writeData(Enemy_CollisionBits + e, M(Enemy_CollisionBits + e) & 0b11111110);
        return;
    }

    // CheckForPUpCollision
    const uint8_t enemyId = M(Enemy_ID + e);
    if (enemyId == PowerUpObject)
    {
        handlePowerUpCollision();
        return;
    }

    // EColl: if the star mario invincibility timer has not expired, kill the enemy as if
    // it were hit with a shell, or from beneath
    if (M(StarInvincibleTimer) != 0)
    {
        ShellOrBlockDefeat(e);
        return;
    }

    // HandlePECollisions: check enemy collision bits for d0 set, or for being offscreen at
    // all, and leave if either is true
    if (((M(Enemy_CollisionBits + e) & 0b00000001) | M(EnemyOffscrBitsMasked + e)) != 0)
    {
        return;
    }
    // otherwise set d0 now
    writeData(Enemy_CollisionBits + e, M(Enemy_CollisionBits + e) | 0x01);

    if (enemyId == Spiny || enemyId == BulletBill_CannonVar)
    {
        chkForPlayerInjury();
        return;
    }
    if (enemyId == PiranhaPlant || enemyId == Podoboo || enemyId >= 0x15)
    {
        InjurePlayer();
        return;
    }
    if (M(AreaType) == 0)
    {
        InjurePlayer(); // branch if water type level
        return;
    }

    // branch if d7 of the enemy state was set, or if all but its 3 LSB masked out is less
    // than two
    if ((M(Enemy_State + e) & 0x80) != 0 || (M(Enemy_State + e) & 0b00000111) < 0x02)
    {
        chkForPlayerInjury();
        return;
    }
    if (M(Enemy_ID + e) == Goomba)
    {
        return; // branch to leave if goomba in defeated state
    }

    // play smack enemy sound
    writeData(Square1SoundQueue, Sfx_EnemySmack);
    // set d7 in enemy state, thus become moving shell
    writeData(Enemy_State + e, M(Enemy_State + e) | 0b10000000);

    // set moving direction and get offset, then load and set horizontal speed data with it
    const uint8_t facingIdx = EnemyFacePlayer(e);
    writeData(Enemy_X_Speed + e, KickedShellXSpdData_data[facingIdx]);

    // add three to whatever the stomp counter contains, unless the shell enemy's timer is
    // near expiring, in which case set the points based on that proximity instead
    const uint8_t shellTimer = M(EnemyIntervalTimer + e);
    const uint8_t points = shellTimer < 0x03 ? KickedShellPtsData_data[shellTimer] : (uint8_t)(0x03 + M(StompChainCounter));

    SetupFloateyNumber(points, e); // KSPts: set values for floatey number now

    // ExPEC: leave!!!
}
