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
    if (operMode_ != TitleScreenModeValue)
    { // if in title screen mode, branch to lock score
        uint8_t digitIndex = digitOffset;
        uint8_t modifierIndex = 5;

        do // AddModLoop: load digit amount to increment, add to current digit
        {
            const uint8_t sum = (uint8_t)(M(DigitModifier + modifierIndex) + M(DisplayDigits + digitIndex));
            uint8_t newDigit;
            if (sum >= 0x80)
            { // BorrowOne: the result is a negative number, so decrement the previous digit
                // and put $09 in the digit we're currently on to "borrow the one"
                --M(DigitModifier - 1 + modifierIndex);
                newDigit = 9;
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
            ram[DisplayDigits + digitIndex] = newDigit;
            --digitIndex;    // move onto next digits in score or game timer
            --modifierIndex; // and digit amounts to increment
        } while ((modifierIndex & 0x80) == 0); // loop back if we're not done yet
    } // EraseDMods: store zero here

    uint8_t eraseIndex = 6; // start with the last digit

    do // EraseMLoop: initialize the digit amounts to increment
    {
        ram[DigitModifier - 1 + eraseIndex] = 0;
        --eraseIndex;
    } while ((eraseIndex & 0x80) == 0); // do this until they're all reset, then leave
}

//------------------------------------------------------------------------

// Inputs: enemyId = enemy identifier to remove
// Outputs: none
void SMBEngine::KillEnemies(uint8_t enemyId)
{
    const uint8_t wantedId = enemyId; // keep identifier here

    uint8_t enemyIndex = 4; // check for identifier in enemy object buffer

    do // KillELoop
    {
        if (M(Enemy_ID + enemyIndex) == wantedId)
        {
            ram[Enemy_Flag + enemyIndex] = 0; // if found, deactivate enemy object flag
        } // NoKillE: do this until all slots are checked
        --enemyIndex;
    } while ((enemyIndex & 0x80) == 0);
}

//------------------------------------------------------------------------

// Inputs: column = block buffer column position
// Outputs: return value = the address of that column within the block buffer
uint16_t SMBEngine::GetBlockBufferAddr(uint8_t column)
{
    const uint8_t BlockBufferAddr_data[] = {LOBYTE(Block_Buffer_1), LOBYTE(Block_Buffer_2), HIBYTE(Block_Buffer_1), HIBYTE(Block_Buffer_2)};

    // the high nybble is a pointer to the high byte of the indirect here
    const uint8_t bufferIndex = column >> 4;
    const uint8_t high = BlockBufferAddr_data[2 + bufferIndex];
    // mask out the high nybble and add to the low byte
    const uint8_t low = (uint8_t)((column & 0b00001111) + BlockBufferAddr_data[bufferIndex]);

    return (uint16_t)((high << 8) | low);
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: return value = page number of the screen's right boundary
uint8_t SMBEngine::GetScreenPosition()
{
    uint32_t wide = 0;

    // get coordinate of screen's left boundary
    wide = ((screenLeft_PageLoc_ << 8) | screenLeft_X_Pos_) + 0xff; // add 255 pixels
    screenRight_X_Pos_ = LOBYTE(wide);                              // store as coordinate of screen's right boundary
    screenRight_PageLoc_ = HIBYTE(wide);                            // store as page number where right boundary is
    return HIBYTE(wide);                                            // get page number where left boundary is
}

//------------------------------------------------------------------------

// Inputs: blockOffset = block object buffer offset
// Outputs: none
void SMBEngine::InitBlock_XY_Pos(uint8_t blockOffset)
{
    // the player's page:coordinate, plus eight pixels
    const uint32_t wide = ((player_PageLoc_ << 8) | player_X_Position_) + 8;
    // mask out low nybble to give 16-pixel correspondence, save as horizontal coordinate
    ram[Block_X_Position + blockOffset] = LOBYTE(wide) & 0xf0;

    const uint8_t pageLoc = HIBYTE(wide);        // the page location of the player, plus the carry
    ram[Block_PageLoc + blockOffset] = pageLoc;  // save as page location of block object
    ram[Block_PageLoc2 + blockOffset] = pageLoc; // save elsewhere to be used later

    // save vertical high byte of player into vertical high byte of block object and leave
    ram[Block_Y_HighPos + blockOffset] = player_Y_HighPos_;
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none (the bool return communicates the result, like a status flag)
bool SMBEngine::CheckPlayerVertical()
{
    if (player_OffscreenBits_ >= 0xf0)
    {
        return true; // the player object is completely offscreen
    }
    if (player_Y_HighPos_ != 1)
    {
        return false; // the player's high vertical byte is not within the screen
    }

    // ExCPV: on the screen, so check how far down the player is vertically
    return player_Y_Position_ >= 0xd0;
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: tuple of {whether the subtraction did not borrow, high byte, low byte of the
// enemy-minus-player page:coordinate difference}; callers test the high byte's d7 rather than
// the bool, and most ignore the low byte
std::tuple<bool, uint8_t, uint8_t> SMBEngine::PlayerEnemyDiff(uint8_t e)
{
    // get the distance between the enemy object and the player, each one 16-bit page:coordinate
    const uint32_t wide = ((M(Enemy_PageLoc + e) << 8) | M(Enemy_X_Position + e)) - ((player_PageLoc_ << 8) | player_X_Position_);
    const bool enemyRightOfPlayer = (wide & 0x10000) == 0;   // the subtraction did not borrow
    return {enemyRightOfPlayer, HIBYTE(wide), LOBYTE(wide)}; // then leave
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

    const uint8_t boxCtrlIdx = boundBoxCtrlIdx; // keep offset here
    // store object coordinates relative to screen, vertically and horizontally respectively
    const uint8_t relYPos = M(SprObject_Rel_YPos + relPosIdx);
    const uint8_t relXPos = M(SprObject_Rel_XPos + relPosIdx);

    const uint8_t boundBoxIdx = (uint8_t)(boundBoxCtrlIdx << 2); // multiply offset by four
    // load the object's bounding box control value and multiply that by four to index the data
    const uint8_t boxDataIdx = (uint8_t)(M(SprObj_BoundBoxCtrl + boundBoxCtrlIdx) << 2);

    // add the first and third numbers in the bounding box data to the relative horizontal
    // coordinate, and store them using the same offset * 4
    ram[BoundingBox_UL_Corner + boundBoxIdx] = (uint8_t)(relXPos + BoundBoxCtrlData_data[boxDataIdx]);
    ram[BoundingBox_LR_Corner + boundBoxIdx] = (uint8_t)(relXPos + BoundBoxCtrlData_data[2 + boxDataIdx]);
    // add the second and fourth numbers to the relative vertical coordinate, at the
    // incremented offsets
    ram[BoundingBox_UL_Corner + 1 + boundBoxIdx] = (uint8_t)(relYPos + BoundBoxCtrlData_data[1 + boxDataIdx]);
    ram[BoundingBox_LR_Corner + 1 + boundBoxIdx] = (uint8_t)(relYPos + BoundBoxCtrlData_data[3 + boxDataIdx]);

    return boundBoxIdx;
}

//------------------------------------------------------------------------

// Inputs: objectOffset = object buffer offset; relPosIdx = relative-position index
// (SprObject_Rel_XPos/YPos)
// Outputs: none
void SMBEngine::GetObjRelativePosition(uint8_t objectOffset, uint8_t relPosIdx)
{
    // load vertical coordinate low and store here
    ram[SprObject_Rel_YPos + relPosIdx] = M(SprObject_Y_Position + objectOffset);
    // take the horizontal coordinate relative to the left of the screen and store it here
    ram[SprObject_Rel_XPos + relPosIdx] = (uint8_t)(M(SprObject_X_Position + objectOffset) - screenLeft_X_Pos_);
}

//------------------------------------------------------------------------

// Inputs: pixelDiff = difference between the screen edge and the object; threshold = the value
// the difference must fall below; value = value to conditionally add to the divided difference;
// flag = compared against 1; currentOffset = offset to return unchanged when the pixel
// difference is not below the threshold
// Outputs: return value = recomputed offset, but only when the pixel difference is below the
// threshold; otherwise currentOffset, unchanged
uint8_t SMBEngine::DividePDiff(uint8_t pixelDiff, uint8_t threshold, uint8_t value, uint8_t flag, uint8_t currentOffset)
{
    if (pixelDiff >= threshold)
    {
        return currentOffset; // ExDivPD: the pixel difference is not below the preset value
    }

    // divide the difference by eight, masking out all but the 3 LSB
    const uint8_t dividedDiff = (pixelDiff >> 3) & 0x07;

    // SetOscrO: use the difference / 8 as the offset, adding the value to it unless the flag is set
    return flag < 1 ? (uint8_t)(dividedDiff + value) : dividedDiff;
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: return value = moving-direction offset (0 = right, 1 = left), used by callers to index
// direction-dependent tables
uint8_t SMBEngine::EnemyFacePlayer(uint8_t e)
{
    // get horizontal difference between player and enemy
    const uint8_t diffHighByte = std::get<1>(PlayerEnemyDiff(e));
    // move right by default; if the enemy is to the right of the player, move left instead
    const uint8_t movingDir = (diffHighByte & 0x80) != 0 ? 0x02 : 0x01;

    ram[Enemy_MovingDir + e] = movingDir; // SFcRt: set moving direction here
    return movingDir - 1;                 // then decrement to use as a proper offset
}

//------------------------------------------------------------------------

// Inputs: objectOffset = sprite object buffer offset
// Outputs: return value = vertical offscreen bits nybble; the horizontal bits from the
// GetXOffscreenBits() call are left in zero-page 0 for the caller to combine
std::pair<uint8_t, uint8_t> SMBEngine::RunOffscrBitsSubs(uint8_t objectOffset)
{
    const uint8_t HighPosUnitData_data[] = {0xff, 0x00};

    const uint8_t DefaultYOnscreenOfs_data[] = {0x04, 0x00, 0x04};

    const uint8_t YOffscreenBitsData_data[] = {0x00, 0x08, 0x0c, 0x0e, 0x0f, 0x07, 0x03, 0x01, 0x00};

    // do subroutine here, then move the high nybble to low and keep it for the caller
    const uint8_t horizontalBits = GetXOffscreenBits(objectOffset) >> 4;

    // GetYOffscreenBits
    uint8_t edgeIdx = 1; // start with top of screen
    uint8_t bits = 0;

    do // YOfsLoop: load coordinate for edge of vertical unit
    {
        // the edge of the vertical unit and the object position are each one 16-bit
        // highpos:coordinate; subtract the vertical coordinate of the object from the edge
        const uint32_t wide = ((0x01 << 8) | HighPosUnitData_data[edgeIdx]) -
                              ((M(SprObject_Y_HighPos + objectOffset) << 8) | M(SprObject_Y_Position + objectOffset));
        const uint8_t pixelDiff = LOBYTE(wide);

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
            bitsIdx = DividePDiff(pixelDiff, 0x20, 0x04, edgeIdx, DefaultYOnscreenOfs_data[1 + edgeIdx]);
        }

        // YLdBData: get offscreen data bits using offset
        bits = YOffscreenBitsData_data[bitsIdx];
        --edgeIdx; // if the bits are zero, do the bottom of the screen now
    } while (bits == 0 && (edgeIdx & 0x80) == 0);

    return {bits, horizontalBits}; // ExYOfsBS
}

//------------------------------------------------------------------------

// Inputs: objectOffset = sprite object buffer offset
// Outputs: return value = horizontal offscreen bits
uint8_t SMBEngine::GetXOffscreenBits(uint8_t objectOffset)
{
    uint8_t edgeIdx = 1; // start with right side of screen
    uint8_t bits = 0;

    do // XOfsLoop: get pixel coordinate of edge
    {
        // the edge and the object position are each one 16-bit page:coordinate; get the
        // difference between them
        const uint32_t wide = ((M(ScreenEdge_PageLoc + edgeIdx) << 8) | M(ScreenEdge_X_Pos + edgeIdx)) -
                              ((M(SprObject_PageLoc + objectOffset) << 8) | M(SprObject_X_Position + objectOffset));
        const uint8_t pixelDiff = LOBYTE(wide);

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
            bitsIdx = DividePDiff(pixelDiff, 0x38, 0x08, edgeIdx, M(DefaultXOnscreenOfs + 1 + edgeIdx));
        }

        // XLdBData: get bits here
        bits = M(XOffscreenBitsData + bitsIdx);
        --edgeIdx; // if the bits are zero, do the left side of the screen now
    } while (bits == 0 && (edgeIdx & 0x80) == 0);

    return bits; // ExXOfsBS
}

//------------------------------------------------------------------------

// Inputs: e = new vine's enemy object buffer offset; blockOffset = source "Block" object
// offset to copy position from
// Outputs: none
void SMBEngine::Setup_Vine(uint8_t e, uint8_t blockOffset)
{
    // load identifier for vine object
    ram[Enemy_ID + e] = VineObject;                                // store in buffer
    ram[Enemy_Flag + e] = 1;                                       // set flag for enemy object buffer
    ram[Enemy_PageLoc + e] = M(Block_PageLoc + blockOffset);       // copy page location from previous object
    ram[Enemy_X_Position + e] = M(Block_X_Position + blockOffset); // copy horizontal coordinate

    const uint8_t yPosition = M(Block_Y_Position + blockOffset);
    ram[Enemy_Y_Position + e] = yPosition; // copy vertical coordinate from previous object

    const uint8_t vineSlot = vineFlagOffset_; // load vine flag/offset to next available vine slot
    if (vineSlot == 0)
    {                                      // if set at all, don't bother to store vertical
        vineStart_Y_Position_ = yPosition; // otherwise store vertical coordinate here
    } // NextVO: store object offset to next available vine slot, using vine flag as offset
    ram[VineObjOffset + vineSlot] = e;
    ++vineFlagOffset_; // increment vine flag offset

    square2SoundQueue_ = Sfx_GrowVine; // load vine grow sound
}

//------------------------------------------------------------------------

// Inputs: movementMode = movement mode (0 = apply downward gravity only; nonzero = also apply the
// upward-speed-capping pass); objectOffset = sprite object buffer offset; downAmount = downward
// movement amount; upAmount = upward movement amount (only used by the upward pass); maxSpeed =
// maximum vertical speed
// Outputs: none
void SMBEngine::ImposeGravity(uint8_t movementMode, uint8_t objectOffset, uint8_t downAmount, uint8_t upAmount, uint8_t maxSpeed)
{
    // get current vertical speed; if currently moving downwards, do not decrement
    // AlterYP: the high bits of the move amount
    const uint8_t moveHighBits = (M(SprObject_Y_Speed + objectOffset) & 0x80) != 0 ? 0xff : 0x00;

    // highpos:position:dummy is one 24-bit quantity, and moveHighBits:speed:force the
    // signed 24-bit amount to move the object by
    uint32_t position = (M(SprObject_Y_HighPos + objectOffset) << 16) | (M(SprObject_Y_Position + objectOffset) << 8) |
                        M(SprObject_YMF_Dummy + objectOffset);
    position += (moveHighBits << 16) | (M(SprObject_Y_Speed + objectOffset) << 8) | M(SprObject_Y_MoveForce + objectOffset);
    ram[SprObject_YMF_Dummy + objectOffset] = LOBYTE(position);          // add movement force to the dummy variable
    ram[SprObject_Y_Position + objectOffset] = HIBYTE(position);         // store as new vertical position
    ram[SprObject_Y_HighPos + objectOffset] = (uint8_t)(position >> 16); // store as new vertical high byte

    // add downward movement amount to the speed:force, the carry going into the speed
    const uint32_t downSpeed = ((M(SprObject_Y_Speed + objectOffset) << 8) | M(SprObject_Y_MoveForce + objectOffset)) + downAmount;
    ram[SprObject_Y_MoveForce + objectOffset] = LOBYTE(downSpeed);
    ram[SprObject_Y_Speed + objectOffset] = HIBYTE(downSpeed);

    // unless the new speed is still below the preset value, keep it within that maximum
    if (((HIBYTE(downSpeed) - maxSpeed) & 0x80) == 0 && M(SprObject_Y_MoveForce + objectOffset) >= 0x80)
    {
        ram[SprObject_Y_Speed + objectOffset] = maxSpeed; // keep vertical speed within maximum value
        ram[SprObject_Y_MoveForce + objectOffset] = 0;    // clear fractional
    }

    // ChkUpM: if the movement mode is zero, leave without the upward-speed-capping pass
    if (movementMode == 0)
    {
        return;
    }

    // otherwise negate maximum speed
    const uint8_t negatedMaxSpeed = -maxSpeed;

    // subtract the upward amount from the speed:force; note that it is twice as large as the
    // downward amount, thus it effectively undoes the add we did earlier
    const uint32_t upSpeed = ((M(SprObject_Y_Speed + objectOffset) << 8) | M(SprObject_Y_MoveForce + objectOffset)) - upAmount;
    ram[SprObject_Y_MoveForce + objectOffset] = LOBYTE(upSpeed);
    ram[SprObject_Y_Speed + objectOffset] = HIBYTE(upSpeed);

    if (((HIBYTE(upSpeed) - negatedMaxSpeed) & 0x80) == 0)
    {
        return; // if less negatively than preset maximum, skip this part
    }
    if (M(SprObject_Y_MoveForce + objectOffset) >= 0x80)
    {
        return; // and if so, branch to leave
    }
    ram[SprObject_Y_Speed + objectOffset] = negatedMaxSpeed; // keep vertical speed within maximum value
    ram[SprObject_Y_MoveForce + objectOffset] = 0xff;        // clear fractional

    // ExVMove: leave!
}

//------------------------------------------------------------------------

// Inputs: none (reads which side collided from zero-page 0, not a register)
// Outputs: none
void SMBEngine::ImpedePlayerMove(uint8_t side)
{
    const uint8_t playerSpeed = player_X_Speed_; // get player's horizontal speed
    // check the value passed in for left side collision
    const bool leftSideCollision = side == 1;

    // the collision bit to clear at the end, and the amount to move the player by
    const uint8_t collisionBit = leftSideCollision ? 1 : 2; // RImpd: return $02 to X
    const uint8_t moveAmount = leftSideCollision ? 0xff : 0x01;

    // if the player is already moving that way, only the bit gets cleared
    const bool alreadyMovingThatWay = leftSideCollision ? (playerSpeed & 0x80) != 0 : ((playerSpeed - 0x01) & 0x80) == 0;

    if (!alreadyMovingThatWay)
    {
        sideCollisionTimer_ = 16; // NXSpd: set timer of some sort
        player_X_Speed_ = 0;      // nullify player's horizontal speed
        // PlatF: the high bits of the horizontal adder
        const uint8_t moveAmountHigh = (moveAmount & 0x80) != 0 ? 0xff : 0x00;
        // moveAmountHigh:moveAmount is the signed 16-bit amount to move the player left or right by
        const uint32_t wide = ((player_PageLoc_ << 8) | player_X_Position_) + ((moveAmountHigh << 8) | moveAmount);
        player_X_Position_ = LOBYTE(wide);
        player_PageLoc_ = HIBYTE(wide); // page location if necessary
    }

    // ExIPM: invert the collision bit and mask out the bit that was set here, to clear it
    player_CollisionBits_ = (uint8_t)(collisionBit ^ 0xff) & player_CollisionBits_;
}

//------------------------------------------------------------------------

// Inputs: objectOffset = sprite object buffer offset; boundBoxIdx = index into the per-object
// BoundingBox_UL_XPos/DR_XPos arrays (typically objectOffset*4, from BoundingBoxCore)
// Outputs: none
void SMBEngine::CheckRightScreenBBox(uint8_t objectOffset, uint8_t boundBoxIdx)
{
    // add 128 pixels to left side of screen
    const uint32_t middle = ((screenLeft_PageLoc_ << 8) | screenLeft_X_Pos_) + 0x80;
    const uint16_t screenMiddle = (uint16_t)((HIBYTE(middle) << 8) | LOBYTE(middle)); // page location of left side of screen, plus carry

    // compare the object, a 16-bit page:coordinate, against the middle of the screen
    const bool objectOnRightSide = ((M(SprObject_PageLoc + objectOffset) << 8) | M(SprObject_X_Position + objectOffset)) >= screenMiddle;

    if (objectOnRightSide)
    {
        // check right-side edge of bounding box for offscreen coordinates, and do nothing
        // if it is still on the screen
        if ((M(BoundingBox_DR_XPos + boundBoxIdx) & 0x80) == 0)
        {
            // likewise the left-side edge; 0xff is the offscreen value for both sides
            if ((M(BoundingBox_UL_XPos + boundBoxIdx) & 0x80) == 0)
            {
                ram[BoundingBox_UL_XPos + boundBoxIdx] = 0xff; // store offscreen value for left side
            } // SORte: store offscreen value for right side
            ram[BoundingBox_DR_XPos + boundBoxIdx] = 0xff;
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
                ram[BoundingBox_DR_XPos + boundBoxIdx] = 0; // store offscreen value for right side
            } // SOLft: store offscreen value for left side
            ram[BoundingBox_UL_XPos + boundBoxIdx] = 0;
        }
    }
    // NoOfs2: leave
}

//------------------------------------------------------------------------

// Inputs: firstTile, secondTile = the two tile numbers; spritePairIdx = sprite data offset (pair
// index); oamSlot = sprite data offset (OAM slot); flipBits = flip control bits (d1 is the
// horizontal flip); attributeBits = other OAM attributes to add; xPos = x coordinate; yPos = y
// coordinate, advanced by eight pixels on return so a drawing loop moves down to the next row
// Outputs: pair of {spritePairIdx+2, oamSlot+8}, advancing to the next sprite pair and OAM row
std::pair<uint8_t, uint8_t> SMBEngine::DrawSpriteObject(uint8_t firstTile, uint8_t secondTile, uint8_t spritePairIdx, uint8_t oamSlot,
                                                        uint8_t flipBits, uint8_t attributeBits, uint8_t xPos, uint8_t &yPos)
{
    // get saved flip control bits; d1 is the horizontal flip
    const bool horizontalFlip = (flipBits & 0b00000010) != 0;

    uint8_t attributes;
    if (horizontalFlip)
    {
        ram[Sprite_Tilenumber + 4 + oamSlot] = firstTile; // store first tile into second sprite
        ram[Sprite_Tilenumber + oamSlot] = secondTile;    // and second into first sprite
        attributes = 0x40;                                // activate horizontal flip OAM attribute
    }
    else // NoHFlip
    {
        ram[Sprite_Tilenumber + oamSlot] = firstTile;      // store first tile into first sprite
        ram[Sprite_Tilenumber + 4 + oamSlot] = secondTile; // and second into second sprite
        attributes = 0;                                    // clear bit for horizontal flip
    }

    // SetHFAt: add other OAM attributes if necessary
    attributes |= attributeBits;
    ram[Sprite_Attributes + oamSlot] = attributes; // store sprite attributes
    ram[Sprite_Attributes + 4 + oamSlot] = attributes;

    ram[Sprite_Y_Position + oamSlot] = yPos;     // now the y coordinates; note because they
    ram[Sprite_Y_Position + 4 + oamSlot] = yPos; // are side by side, they are the same

    ram[Sprite_X_Position + oamSlot] = xPos;                    // store x coordinate, then
    ram[Sprite_X_Position + 4 + oamSlot] = (uint8_t)(xPos + 8); // put them side by side

    yPos = (uint8_t)(yPos + 8); // add eight pixels to the next y

    // advance both offsets to return them to the routine that called this subroutine
    return {(uint8_t)(spritePairIdx + 2), (uint8_t)(oamSlot + 8)};
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitPiranhaPlant(uint8_t e)
{
    // set initial speed
    ram[PiranhaPlant_Y_Speed + e] = 1;
    ram[Enemy_State + e] = 0;           // initialize enemy state and what would normally
    ram[PiranhaPlant_MoveFlag + e] = 0; // be used as vertical speed, but not in this case

    const uint8_t yPosition = M(Enemy_Y_Position + e);
    ram[PiranhaPlantDownYPos + e] = yPosition;                 // save original vertical coordinate here
    ram[PiranhaPlantUpYPos + e] = (uint8_t)(yPosition - 0x18); // and that - 24 pixels here

    SetBBox2(9, e); // set specific value for bounding box control
}

//------------------------------------------------------------------------

// set bounding box control then leave
// Inputs: boundBoxCtrl = bounding box control value; e = enemy object buffer offset
// Outputs: none
void SMBEngine::SetBBox2(uint8_t boundBoxCtrl, uint8_t e) { ram[Enemy_BoundBoxCtrl + e] = boundBoxCtrl; }

//------------------------------------------------------------------------

// initialize vertical speed
// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::InitVStf(uint8_t e)
{
    ram[Enemy_Y_Speed + e] = 0;     // initialize vertical speed
    ram[Enemy_Y_MoveForce + e] = 0; // and movement force
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: returns vRAM_Buffer1_Offset_, as it was before the seven bytes written here. The
// player entrance code in SMBGame.cpp falls out of here into SetupBubble, which takes the value as
// its air bubble slot -- so on the first frame of a water level, where that bubble goes depends on
// the vram buffer offset this happened to leave behind.
uint8_t SMBEngine::GetPlayerColors()
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
    uint8_t colorIdx = currentPlayer_ != 0 ? 4 : 0;
    if (playerStatus_ == 2)
    { // ChkFiery: if fiery, load alternate offset for fiery player
        colorIdx = 8;
    }

    uint8_t bufferIdx = vRAM_Buffer1_Offset_; // get current buffer offset
    uint8_t colorsLeft = 3;                   // StartClrGet: do four colors

    do // ClrGetLoop: fetch player colors and store them in the buffer
    {
        ram[VRAM_Buffer1 + 3 + bufferIdx] = PlayerColors_data[colorIdx];
        ++colorIdx;
        ++bufferIdx;
        --colorsLeft;
    } while ((colorsLeft & 0x80) == 0);

    const uint8_t offset = vRAM_Buffer1_Offset_; // load original offset from before
    // if the background color control is set it is used as the offset to the background
    // color, otherwise the area type bits from the area offset are used instead
    const uint8_t bgColorIdx = backgroundColorCtrl_ != 0 ? backgroundColorCtrl_ : areaType_;
    ram[VRAM_Buffer1 + 3 + offset] = BackgroundColors_data[bgColorIdx]; // SetBGColor

    // set for sprite palette address, saving to buffer
    ram[VRAM_Buffer1 + offset] = 0x3f;
    ram[VRAM_Buffer1 + 1 + offset] = 0x10;
    ram[VRAM_Buffer1 + 2 + offset] = 0x04; // write length byte to buffer
    ram[VRAM_Buffer1 + 7 + offset] = 0x00; // now the null terminator

    SetVRAMOffset((uint8_t)(offset + 7)); // move the buffer pointer ahead 7 bytes
    return offset;                        // see above: the entrance code reads this back
}

//------------------------------------------------------------------------

// store as new vram buffer offset
// Inputs: newOffset = new VRAM_Buffer1 offset
// Outputs: none
void SMBEngine::SetVRAMOffset(uint8_t newOffset) { vRAM_Buffer1_Offset_ = newOffset; }

//------------------------------------------------------------------------

// Inputs: metatile = metatile number to check; cell = the block buffer cell it lives in, needed
// by PutBlockMetatile
// Outputs: none
void SMBEngine::WriteBlockMetatile(uint8_t metatile, BlockBufferCell cell)
{
    uint8_t groupSelector;
    if (metatile == 0)
    {
        groupSelector = 3; // offset for blank metatile (unconditional if branched from 8a6b)
    }
    else if (metatile == 0x58 || metatile == 0x51)
    {
        // brick with coins (w/ line), or breakable brick w/ line
        groupSelector = 0; // offset for brick metatile w/ line
    }
    else if (metatile == 0x5d || metatile == 0x52)
    {
        // brick with coins (w/o line), or breakable brick w/o line
        groupSelector = 1; // offset for brick metatile w/o line
    }
    else
    {
        groupSelector = 2; // if any other metatile, the offset for an empty block
    }

    // UseBOffset: get vram buffer offset and move onto next byte
    const uint8_t vramOffset = (uint8_t)(vRAM_Buffer1_Offset_ + 1);
    // get appropriate block data and write to vram buffer
    PutBlockMetatile(groupSelector, cell, vramOffset);

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
// BlockGfxData_data); cell = the block buffer cell the metatile lives in; vramOffset = vram
// buffer offset for the next byte
// Outputs: none
void SMBEngine::PutBlockMetatile(uint8_t metatileGroupSelector, BlockBufferCell cell, uint8_t vramOffset)
{
    // multiply the selector by four to index the block graphics data
    const uint8_t metatileGroupOfs4 = (uint8_t)(metatileGroupSelector << 2);

    // get low byte of block buffer pointer; at $d0 or above use the high byte for name
    // table 1, otherwise the one for name table 0
    const uint8_t blockBufferLow = LOBYTE(cell.address);
    const uint8_t highAdder = blockBufferLow >= 0xd0 ? 0x24 : 0x20; // SaveHAdder: save high byte here

    // mask out the high nybble of the block buffer pointer and multiply by 2 to get the
    // appropriate name table low byte
    const uint8_t lowAdder = (uint8_t)((blockBufferLow & 0x0f) << 1);

    // the vertical offset, times four, is a ten-bit quantity; add the sixteen-bit
    // name table address to it
    uint32_t wide = (uint8_t)(cell.row + 0x20) << 2; // add 32 pixels for the status bar
    wide += (highAdder << 8) | lowAdder;             // add the name table address

    // get vram buffer offset to be used
    RemBridge(metatileGroupOfs4, vramOffset, LOBYTE(wide), HIBYTE(wide));
}

//------------------------------------------------------------------------

// write top left and top right
// Inputs: metatileGroupOfs4 = metatile-group offset (x4) into BlockGfxData_data, set by
// PutBlockMetatile; vramOffset = vram buffer offset; nameTableLow/High = name table address of
// the metatile to replace
// Outputs: none
void SMBEngine::RemBridge(uint8_t metatileGroupOfs4, uint8_t vramOffset, uint8_t nameTableLow, uint8_t nameTableHigh)
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
    ram[VRAM_Buffer1 + 2 + vramOffset] = BlockGfxData_data[metatileGroupOfs4];
    ram[VRAM_Buffer1 + 3 + vramOffset] = BlockGfxData_data[1 + metatileGroupOfs4];
    // write bottom left and bottom right tile numbers into second spot
    ram[VRAM_Buffer1 + 7 + vramOffset] = BlockGfxData_data[2 + metatileGroupOfs4];
    ram[VRAM_Buffer1 + 8 + vramOffset] = BlockGfxData_data[3 + metatileGroupOfs4];

    ram[VRAM_Buffer1 + vramOffset] = nameTableLow; // write low byte of name table
    // add 32 bytes to the value and write that low byte into the second slot
    ram[VRAM_Buffer1 + 5 + vramOffset] = (uint8_t)(nameTableLow + 0x20);

    ram[VRAM_Buffer1 - 1 + vramOffset] = nameTableHigh; // write high byte of name
    ram[VRAM_Buffer1 + 4 + vramOffset] = nameTableHigh; // table address to both slots

    ram[VRAM_Buffer1 + 1 + vramOffset] = 0x02; // put length of 2 in
    ram[VRAM_Buffer1 + 6 + vramOffset] = 0x02; // both slots
    ram[VRAM_Buffer1 + 9 + vramOffset] = 0x00; // put null terminator at end
    // leave
}

//------------------------------------------------------------------------

// Inputs: playerOffset = player-specific offset
// Outputs: none
void SMBEngine::PrintStatusBarNumbers(uint8_t playerOffset)
{
    OutputNumbers(playerOffset); // use first nybble to print the coin display

    // move high nybble to low and print to score display
    OutputNumbers(playerOffset >> 4);
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

    const uint8_t displayIdx = (uint8_t)(nybbleIdx + 1) & 0b00001111; // mask out high nybble
    if (displayIdx >= 6)
    {
        return; // ExitOutputN
    }

    const uint8_t barDataIdx = (uint8_t)(displayIdx << 1); // shift to left and use as offset
    const uint8_t bufferIdx = vRAM_Buffer1_Offset_;        // get current buffer pointer

    // SetupNums: put at top of screen by default, or further down for the top score display
    ram[VRAM_Buffer1 + bufferIdx] = barDataIdx == 0x00 ? 0x22 : 0x20;
    // write low vram address of the thing we're printing to the buffer
    ram[VRAM_Buffer1 + 1 + bufferIdx] = StatusBarData_data[barDataIdx];

    const uint8_t length = StatusBarData_data[1 + barDataIdx]; // and its length
    ram[VRAM_Buffer1 + 2 + bufferIdx] = length;
    uint8_t digitsLeft = length;              // save length byte in counter
    const uint8_t savedBufferIdx = bufferIdx; // and buffer pointer elsewhere for now

    // load offset to value we want to write, subtract the length byte we read before, and
    // use the value as the offset to the display digits
    uint8_t digitIdx = (uint8_t)(StatusBarOffset_data[displayIdx] - length);
    uint8_t writeIdx = savedBufferIdx;

    do // DigitPLoop: write digits to the buffer
    {
        ram[VRAM_Buffer1 + 3 + writeIdx] = M(DisplayDigits + digitIdx);
        ++writeIdx;
        ++digitIdx;
        --digitsLeft; // do this until all the digits are written
    } while (digitsLeft != 0);

    ram[VRAM_Buffer1 + 3 + writeIdx] = 0x00; // put null terminator at end
    // increment buffer pointer by 3, storing it in case we want to use it again
    vRAM_Buffer1_Offset_ = (uint8_t)(writeIdx + 3);
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
    const uint8_t offscreenBits = GetXOffscreenBits(0);
    const uint8_t bits = offscreenBits;

    // if d7 of the offscreen bits is set use the default offset (left side), otherwise the
    // offset for the right side
    const bool leftSide = (offscreenBits & 0x80) != 0;
    const uint8_t edgeIdx = leftSide ? 0 : 1;

    // on the right side, d5 of the offscreen bits also has to be set to keep the player on
    if (leftSide || (bits & 0b00100000) != 0)
    {
        // KeepOnscr: get left or right side coordinate based on offset, subtracting an
        // amount based on that same offset
        const uint32_t wide = ((M(ScreenEdge_PageLoc + edgeIdx) << 8) | M(ScreenEdge_X_Pos + edgeIdx)) - X_SubtracterData_data[edgeIdx];
        player_X_Position_ = LOBYTE(wide); // store as player position to prevent movement further
        player_PageLoc_ = HIBYTE(wide);    // save as player's page location

        // check saved controller bits, nullifying the player's horizontal speed if not equal
        if (left_Right_Buttons_ != OffscrJoypadBitsData_data[edgeIdx])
        {
            player_X_Speed_ = 0;
        }
    }

    // InitPlatScrl: nullify platform force imposed on scroll
    platform_X_Scroll_ = 0;
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::AddToScore()
{
    const uint8_t ScoreOffsets_data[] = {0x0b, 0x11};

    // get the offset for the current player's score, and update the score internally with
    // the value in the digit modifier
    DigitsMathRoutine(ScoreOffsets_data[currentPlayer_]);

    GetSBNybbles();
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::GetSBNybbles()
{
    const uint8_t StatusBarNybbles_data[] = {0x02, 0x13};

    // get nybbles based on the current player, used to update score and coins
    UpdateNumber(StatusBarNybbles_data[currentPlayer_]);
}

//------------------------------------------------------------------------

// Inputs: statusBarNybbles = status bar nybbles value (forwarded to PrintStatusBarNumbers)
// Outputs: none
void SMBEngine::UpdateNumber(uint8_t statusBarNybbles)
{
    PrintStatusBarNumbers(statusBarNybbles); // print status bar numbers based on nybbles, whatever they be

    const uint8_t bufferIdx = vRAM_Buffer1_Offset_;
    if (M(VRAM_Buffer1 - 6 + bufferIdx) == 0)
    { // if the highest digit of the score is zero, overwrite it with a space tile so that
        // it is suppressed
        ram[VRAM_Buffer1 - 6 + bufferIdx] = 0x24;
    } // NoZSup: leave
}

//------------------------------------------------------------------------

// this is a residual jump point in enemy object jump table
// Inputs: none (always operates on enemy slot 5)
// Outputs: none
void SMBEngine::PwrUpJmp()
{
    ram[Enemy_State + 5] = 1;        // set power-up object's state
    ram[Enemy_Flag + 5] = 1;         // set buffer flag
    ram[Enemy_BoundBoxCtrl + 5] = 3; // set bounding box size control for power-up object
    if (powerUpType_ < 2)
    {                                         // if star or 1-up, branch ahead
        const uint8_t status = playerStatus_; // otherwise check player's current status
        // if player not fiery, use status as power-up type, otherwise shift right to force
        // the fire flower type
        powerUpType_ = status >= 2 ? status >> 1 : status; // StrType: store type here
    } // PutBehind
    ram[Enemy_SprAttrib + 5] = 0b00100000; // set background priority bit
    square2SoundQueue_ = Sfx_GrowPowerUp;  // load power-up reveal sound and leave
}

//------------------------------------------------------------------------

// Inputs: maxSpeed = maximum speed; objectOffset = sprite object buffer offset; downAmount =
// downward movement amount (both passed through to ImposeGravity)
// Outputs: none
void SMBEngine::ImposeGravitySprObj(uint8_t maxSpeed, uint8_t objectOffset, uint8_t downAmount)
{
    // 0 is the movement mode that moves the object downwards; jump to the code that
    // actually moves it. The upward amount is unused in that mode.
    ImposeGravity(0, objectOffset, downAmount, 0, maxSpeed);
}

//------------------------------------------------------------------------

// Inputs: none (reads the current enemy from ObjectOffset)
// Outputs: pair of {boundBoxIdx, offscreenBits} (see GetEnemyBoundBoxOfsArg)
std::pair<uint8_t, uint8_t> SMBEngine::GetEnemyBoundBoxOfs()
{
    return GetEnemyBoundBoxOfsArg(objectOffset_); // get enemy object buffer offset
}

//------------------------------------------------------------------------

// Inputs: e = enemy object offset (0-4)
// Outputs: pair of {e*4+4, the index into the per-object BoundingBox arrays (skipping the
// player's own box); Enemy_OffscreenBits masked to its low nybble}
std::pair<uint8_t, uint8_t> SMBEngine::GetEnemyBoundBoxOfsArg(uint8_t e)
{
    // multiply the offset by four, then add four to skip the player's bounding box
    const uint8_t boundBoxIdx = (uint8_t)((e << 2) + 4);
    // get offscreen bits for enemy object, saving the low nybble
    const uint8_t offscreenBits = enemy_OffscreenBits_ & 0b00001111;

    return {boundBoxIdx, offscreenBits};
}

//------------------------------------------------------------------------

// Inputs: objectOffset = object offset
// Outputs: none
void SMBEngine::SetupEOffsetFBBox(uint8_t objectOffset)
{
    // add 1 to the offset to properly address the bounding box, and use 1 as the
    // relative-position index here for the same reason
    const uint8_t boundBoxCtrlIdx = (uint8_t)(objectOffset + 1);
    // do a sub to get the coordinates of the bounding box
    const uint8_t boundBoxIdx = BoundingBoxCore(boundBoxCtrlIdx, 1);

    // jump to handle offscreen coordinates of bounding box
    CheckRightScreenBBox(boundBoxCtrlIdx, boundBoxIdx);
}

//------------------------------------------------------------------------

// do collision detection subroutine for sprite object
// Inputs: coordSelector, objectOffset, cornerIdx (see BlockBufferCollision)
// Outputs: BlockBufferResult (see BlockBufferCollision)
BlockBufferResult SMBEngine::BBChk_E(uint8_t coordSelector, uint8_t objectOffset, uint8_t cornerIdx)
{ return BlockBufferCollision(coordSelector, objectOffset, cornerIdx); }

//------------------------------------------------------------------------

// Inputs: coordSelector = which coordinate's low nybble to also report (0 = vertical/Y, nonzero =
// horizontal/X); objectOffset = sprite object buffer offset; cornerIdx = corner-selector index
// into BlockBuffer_X_Adder_data/BlockBuffer_Y_Adder_data (0-27)
// Outputs: BlockBufferResult -- the metatile found, the selected coordinate's low nybble, and
// the block row the lookup used
BlockBufferResult SMBEngine::BlockBufferCollision(uint8_t coordSelector, uint8_t objectOffset, uint8_t cornerIdx)
{
    const uint8_t BlockBuffer_Y_Adder_data[] = {0x04, 0x20, 0x20, 0x08, 0x18, 0x08, 0x18, 0x02, 0x20, 0x20, 0x08, 0x18, 0x08, 0x18,
                                                0x12, 0x20, 0x20, 0x18, 0x18, 0x18, 0x18, 0x18, 0x14, 0x14, 0x06, 0x06, 0x08, 0x10};

    const uint8_t BlockBuffer_X_Adder_data[] = {0x08, 0x03, 0x0c, 0x02, 0x02, 0x0d, 0x0d, 0x08, 0x03, 0x0c, 0x02, 0x02, 0x0d, 0x0d,
                                                0x08, 0x03, 0x0c, 0x02, 0x02, 0x0d, 0x0d, 0x08, 0x00, 0x10, 0x04, 0x14, 0x04, 0x04};

    // add horizontal coordinate of object to value obtained using the corner index as offset
    const uint32_t wide =
        ((M(SprObject_PageLoc + objectOffset) << 8) | M(SprObject_X_Position + objectOffset)) + BlockBuffer_X_Adder_data[cornerIdx];

    // put the LSB of the page location (plus the carry) above the low byte, and
    // effectively move the high nybble to the lower; the LSB which became the MSB will be
    // d4 at this point
    const uint8_t blockColumn = (uint8_t)(((HIBYTE(wide) & 0x01) << 7) | (LOBYTE(wide) >> 1)) >> 3;
    const uint16_t blockBufferAddr = GetBlockBufferAddr(blockColumn); // get address of block buffer

    // add the vertical coordinate of the object to the value obtained using the corner
    // index as offset, mask out the low nybble, and subtract 32 pixels for the status bar
    const uint8_t blockRow =
        (uint8_t)(((uint8_t)(M(SprObject_Y_Position + objectOffset) + BlockBuffer_Y_Adder_data[cornerIdx]) & 0b11110000) - 0x20);
    // check current content of block buffer, using the row as offset
    const uint8_t metatile = M(blockBufferAddr + blockRow);

    // report the low nybble of the vertical coordinate, or of the horizontal one if the
    // coordinate selector is set
    const uint8_t coordinate = coordSelector == 0 ? M(SprObject_Y_Position + objectOffset)  // RetYC
                                                  : M(SprObject_X_Position + objectOffset); // RetXC

    // get saved content of block buffer and leave
    return {metatile, (uint8_t)(coordinate & 0b00001111), blockRow, blockBufferAddr};
}

//------------------------------------------------------------------------

// Inputs: firstTile, secondTile = the two tile numbers; spritePairIdx, oamSlot = sprite data
// offsets; flipBits, attributeBits, xPos, yPos = forwarded (see DrawSpriteObject)
// Outputs: pair of {spritePairIdx+2, oamSlot+8} (see DrawSpriteObject)
std::pair<uint8_t, uint8_t> SMBEngine::DrawOneSpriteRow(uint8_t firstTile, uint8_t secondTile, uint8_t spritePairIdx, uint8_t oamSlot,
                                                        uint8_t flipBits, uint8_t attributeBits, uint8_t xPos, uint8_t &yPos)
{
    // draw them
    return DrawSpriteObject(firstTile, secondTile, spritePairIdx, oamSlot, flipBits, attributeBits, xPos, yPos);
}

//------------------------------------------------------------------------

// Inputs: oamSlot = OAM sprite slot index
// Outputs: none
void SMBEngine::DrawFirebar(uint8_t oamSlot)
{
    const uint8_t quarterFrame = frameCounter_ >> 2; // get frame counter and divide by four

    // mask out all but the last bit to set either tile $64 or $65 as the fireball tile,
    // so that the tile changes every four frames
    ram[Sprite_Tilenumber + oamSlot] = (quarterFrame & 0x01) ^ 0x64;

    // $02 sets the palette in the attribute byte; every eight frames, flip both ways instead
    const bool flipBothWays = ((quarterFrame >> 1) & 0x01) != 0;
    ram[Sprite_Attributes + oamSlot] = flipBothWays ? 0b11000010 : 0x02; // FireA
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::RelativePlayerPosition()
{
    // the offsets for the relative coordinates routine correspond to the player object,
    // and getting the coordinates
    RelWOfs(0, 0);
}

//------------------------------------------------------------------------

// get the coordinates
// Inputs: objectOffset, relPosIdx (see GetObjRelativePosition)
// Outputs: none
void SMBEngine::RelWOfs(uint8_t objectOffset, uint8_t relPosIdx)
{
    GetObjRelativePosition(objectOffset, relPosIdx);
    // leave
}

//------------------------------------------------------------------------

// Inputs: offset = the object's position within its group (usually the object offset, but the
// fireball/enemy collision path passes the enemy being tested instead)
// Outputs: none
void SMBEngine::RelativeEnemyPosition(uint8_t offset)
{
    // get coordinates of enemy object relative to the screen
    VariableObjOfsRelPos(1, offset, 1);
}

//------------------------------------------------------------------------

// Inputs: baseValue = base value (e.g. 1 to skip a byte and land on the enemy offset); addend =
// the object's position within its group; relPosIdx = relative-position index (forwarded to
// GetObjRelativePosition)
// Outputs: none
void SMBEngine::VariableObjOfsRelPos(uint8_t baseValue, uint8_t addend, uint8_t relPosIdx)
{
    // add the base value to the addend, and use the sum as the enemy offset
    GetObjRelativePosition((uint8_t)(baseValue + addend), relPosIdx);
    // leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::GetPlayerOffscreenBits()
{
    // the offsets are for player-specific variables, and getting offscreen information
    // about the player
    GetOffScreenBitsSet(0, 0);
}

//------------------------------------------------------------------------

// Inputs: offset = base object offset (usually the current object offset, but BalancePlatform
// passes the partner platform)
// Outputs: none
void SMBEngine::GetEnemyOffscreenBits(uint8_t offset)
{
    // add 1 byte in order to get the enemy offset, and put the offscreen bits in
    // Enemy_OffscreenBits
    SetOffscrBitsOffset(1, offset, 1);
}

//------------------------------------------------------------------------

// Inputs: addend = addend (e.g. 1 to reach the enemy offset); baseObjectOffset = base object
// offset; offscrArrayOffset = offscreen-bits-array offset (forwarded to GetOffScreenBitsSet)
// Outputs: none
void SMBEngine::SetOffscrBitsOffset(uint8_t addend, uint8_t baseObjectOffset, uint8_t offscrArrayOffset)
{
    // add the base object offset to the addend to get the appropriate offset
    GetOffScreenBitsSet((uint8_t)(addend + baseObjectOffset), offscrArrayOffset);
}

//------------------------------------------------------------------------

// Inputs: objectOffset = sprite object buffer offset; offscrArrayOffset = offscreen-bits-array
// offset (selects where the result is stored, e.g. player vs. enemy)
// Outputs: none
void SMBEngine::GetOffScreenBitsSet(uint8_t objectOffset, uint8_t offscrArrayOffset)
{
    const auto [verticalBits, horizontalBits] = RunOffscrBitsSubs(objectOffset);
    // move the low nybble to the high nybble and mask it together with the horizontal
    // nybble the sub returned, then store both at the offscreen bits offset
    ram[SprObject_OffscrBits + offscrArrayOffset] = (uint8_t)(verticalBits << 4) | horizontalBits;
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: return value = saved value from MoveObjectHorizontally
uint8_t SMBEngine::MoveEnemyHorizontally(uint8_t e)
{
    // increment the offset for the enemy offset, and position the object horizontally
    // according to counters, returning with the saved value
    const uint8_t saved = MoveObjectHorizontally((uint8_t)(e + 1));
    return saved;
}

//------------------------------------------------------------------------

// Inputs: objectOffset = sprite object buffer offset
// Outputs: return value = carry-plus-high-nybble value from the horizontal movement force update
uint8_t SMBEngine::MoveObjectHorizontally(uint8_t objectOffset)
{
    // get the currently saved value (horizontal speed, secondary counter, whatever) and
    // move its low nybble to high, storing the result here
    const uint8_t speedLowNybble = (uint8_t)(M(SprObject_X_Speed + objectOffset) << 4);

    // get the saved value again and move the high nybble to low, altering the high nybble
    // if the result is negative
    const uint8_t speed = M(SprObject_X_Speed + objectOffset) >> 4;
    const uint8_t signedSpeed = speed >= 8 ? (uint8_t)(speed | 0b11110000) : speed; // SaveXSpd: keep result here

    // UseAdder: save the high bits of the horizontal adder here
    const uint8_t speedHighByte = (signedSpeed & 0x80) != 0 ? 0xff : 0x00;

    const uint32_t force = M(SprObject_X_MoveForce + objectOffset) + speedLowNybble; // add low nybble moved to high
    ram[SprObject_X_MoveForce + objectOffset] = LOBYTE(force);                       // store result here
    const bool carry = HIBYTE(force) != 0; // the original saves this carry on the stack for the end

    // pageloc:position is one 16-bit quantity, and $02:$00 the signed 16-bit amount
    // to move the object by (the high nybble moved to low, plus $f0 if necessary)
    const uint32_t wide = ((M(SprObject_PageLoc + objectOffset) << 8) | M(SprObject_X_Position + objectOffset)) +
                          ((speedHighByte << 8) | signedSpeed) + (carry ? 1 : 0);
    ram[SprObject_X_Position + objectOffset] = LOBYTE(wide); // to object's horizontal position
    ram[SprObject_PageLoc + objectOffset] = HIBYTE(wide);    // and the object's page location and save

    // add the old carry to the high nybble moved to low
    return (uint8_t)((carry ? 1 : 0) + signedSpeed); // ExXMove: and leave
}

//------------------------------------------------------------------------

// Inputs: pointsControl = points-control value; e = enemy object buffer offset
//
// Returns the x-coordinate of the created floatey. This is normally unused, but in one place
// the result is erroneously passed on to ChkToStunEnemies, causing this bug: https://themushroomkingdom.net/bugs/7
//
uint8_t SMBEngine::SetupFloateyNumber(uint8_t pointsControl, uint8_t e)
{
    ram[FloateyNum_Control + e] = pointsControl;         // set number of points control
    ram[FloateyNum_Timer + e] = 48;                      // set timer for floatey numbers
    ram[FloateyNum_Y_Pos + e] = M(Enemy_Y_Position + e); // set vertical coordinate

    const uint8_t relXPos = enemy_Rel_XPos_;
    ram[FloateyNum_X_Pos + e] = relXPos; // set horizontal coordinate and leave
    return relXPos;
    // ExSFN
}

//------------------------------------------------------------------------

// Inputs: boundBoxIdx = bounding-box index of the object to test against the player (the player's
// own box is always index 0, so x needs no input)
// Outputs: the bool return communicates whether a collision was found
bool SMBEngine::PlayerCollisionCore(uint8_t boundBoxIdx)
{
    // index 0 is the player's own bounding box, which is what we compare against
    return SprObjectCollisionCore(0, boundBoxIdx);
}

//------------------------------------------------------------------------

// Inputs: objIdx1, objIdx2 = the two objects' bounding-box indices to compare
// Outputs: the bool return communicates whether a collision was found
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

    uint8_t counter = 1; // start the counter at 1, compare horizontal coordinates first

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
        --counter;
    } while ((counter & 0x80) == 0); // if counter not expired, branch to loop
    // otherwise we already did both sets, therefore collision

    return collisionFound;
}

//------------------------------------------------------------------------

// Inputs: scrollAmount = scroll amount
// Outputs: none
void SMBEngine::ScrollScreen(uint8_t scrollAmount)
{
    scrollAmount_ = scrollAmount; // save value here
    // add to the value already set here and save that as the new value
    scrollThirtyTwo_ = (uint8_t)(scrollAmount + scrollThirtyTwo_);

    // add to the left side coordinate, which is one 16-bit page:coordinate
    const uint32_t wide = ((screenLeft_PageLoc_ << 8) | screenLeft_X_Pos_) + scrollAmount;
    screenLeft_X_Pos_ = LOBYTE(wide);   // save as new left side coordinate
    horizontalScroll_ = LOBYTE(wide);   // save here also
    screenLeft_PageLoc_ = HIBYTE(wide); // add carry to page location for left side of the screen

    // get the LSB of the page location and save it as a temp variable for the mirror
    const uint8_t nameTableBit = HIBYTE(wide) & 0x01;
    // get the PPU register 1 mirror and save all bits except d0, then get the saved bit
    // here and save it in the mirror, to be used to set the name table later
    mirror_PPU_CTRL_REG1_ = (mirror_PPU_CTRL_REG1_ & 0b11111110) | nameTableBit;

    GetScreenPosition();      // figure out where the right side is
    scrollIntervalTimer_ = 8; // set scroll timer (residual, not used elsewhere)

    ChkPOffscr(); // skip this part
}

//------------------------------------------------------------------------

// set maximum speed in A
// Inputs: e = enemy object buffer offset; downwardMoveAmt = downward movement amount
// Outputs: none
void SMBEngine::SetHiMax(uint8_t e, uint8_t downwardMoveAmt)
{
    // 3 is the maximum speed
    SetXMoveAmt(3, e, downwardMoveAmt);
}

//------------------------------------------------------------------------

// set movement amount here
// Inputs: maxSpeed = maximum speed (from the caller, e.g. SetHiMax); e = enemy object
// buffer offset; downwardMoveAmt = downward movement amount, low byte (passed to ImposeGravity)
// Outputs: none
void SMBEngine::SetXMoveAmt(uint8_t maxSpeed, uint8_t e, uint8_t downwardMoveAmt)
{
    // increment the offset for the enemy offset, and do a sub to move the enemy downwards
    ImposeGravitySprObj(maxSpeed, (uint8_t)(e + 1), downwardMoveAmt);
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ScrollHandler()
{
    // load value saved here, add the value used by left/right platforms, and save that as
    // the new value here to impose force on the scroll
    const uint8_t playerScroll = (uint8_t)(player_X_Scroll_ + platform_X_Scroll_);
    player_X_Scroll_ = playerScroll;

    // a bunch of code is skipped if the scroll lock flag is set, if the player is less than
    // 80 pixels to the right, or if the timer related to the player's side collision has
    // not expired
    const bool scrollLocked = scrollLock_ != 0 || player_Pos_ForScroll_ < 80 || sideCollisionTimer_ != 0;
    // the screen also does not scroll if the value was originally set to zero, or is
    // otherwise negative for left movement
    const bool scrollable = ((uint8_t)(playerScroll - 1) & 0x80) == 0;

    if (!scrollLocked && scrollable)
    {
        // ChkNearMid: if less than 112 pixels to the right, decrement the amount by one,
        // otherwise use the original undecremented value
        if (player_Pos_ForScroll_ < 0x70)
        {
            ScrollScreen(playerScroll >= 2 ? (uint8_t)(playerScroll - 1) : playerScroll);
        }
        else
        {
            ScrollScreen(player_X_Scroll_);
        }
        return;
    }

    // InitScrlAmt: initialize value here
    scrollAmount_ = 0;

    ChkPOffscr();
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::EraseEnemyObject(uint8_t e)
{
    // clear all enemy object variables
    ram[Enemy_Flag + e] = 0;
    ram[Enemy_ID + e] = 0;
    ram[Enemy_State + e] = 0;
    ram[FloateyNum_Control + e] = 0;
    ram[EnemyIntervalTimer + e] = 0;
    ram[ShellChainCounter + e] = 0;
    ram[Enemy_SprAttrib + e] = 0;
    ram[EnemyFrameTimer + e] = 0;
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

    uint8_t leftEdge = screenLeft_X_Pos_; // get horizontal coordinate for left side of screen
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
    const uint32_t left = ((screenLeft_PageLoc_ << 8) | leftEdge) - 0x48 - (carry ? 0 : 1);

    // the original never clears the carry here either, so a left edge that did not borrow
    // pushes the right edge one pixel further out
    const bool leftDidNotBorrow = (left & 0x10000) == 0;
    // add 72 pixels to the right side horizontal coordinate
    const uint32_t right = ((screenRight_PageLoc_ << 8) | screenRight_X_Pos_) + 0x48 + (leftDidNotBorrow ? 1 : 0);

    // the enemy object and the modified left edge are each one 16-bit page:coordinate
    const uint32_t fromLeft = ((M(Enemy_PageLoc + e) << 8) | M(Enemy_X_Position + e)) - (left & 0xffff);
    if ((HIBYTE(fromLeft) & 0x80) == 0)
    { // if enemy object is too far left, branch to erase it
        // the enemy object and the modified right edge are each one 16-bit page:coordinate
        const uint32_t fromRight = ((M(Enemy_PageLoc + e) << 8) | M(Enemy_X_Position + e)) - (right & 0xffff);
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
// Outputs: none
void SMBEngine::DumpSixSpr(uint8_t value, uint8_t baseOffset)
{
    ram[Sprite_Data + 20 + baseOffset] = value; // dump the value
    ram[Sprite_Data + 16 + baseOffset] = value; // into third row sprites
    DumpFourSpr(value, baseOffset);
}

//------------------------------------------------------------------------

// Inputs: value = value to dump; baseOffset = base sprite-data offset
// Outputs: none
void SMBEngine::DumpFourSpr(uint8_t value, uint8_t baseOffset)
{
    ram[Sprite_Data + 12 + baseOffset] = value; // into second row sprites
    DumpThreeSpr(value, baseOffset);
}

//------------------------------------------------------------------------

// Inputs: value = value to dump; baseOffset = base sprite-data offset
// Outputs: none
void SMBEngine::DumpThreeSpr(uint8_t value, uint8_t baseOffset)
{
    ram[Sprite_Data + 8 + baseOffset] = value;
    DumpTwoSpr(value, baseOffset);
}

//------------------------------------------------------------------------

// Inputs: value = value to dump; baseOffset = base sprite-data offset
// Outputs: none
void SMBEngine::DumpTwoSpr(uint8_t value, uint8_t baseOffset)
{
    ram[Sprite_Data + 4 + baseOffset] = value; // and into first row sprites
    ram[Sprite_Data + baseOffset] = value;
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
// Outputs: none
void SMBEngine::SprObjectOffscrChk()
{
    const uint8_t e = objectOffset_;                    // get enemy buffer offset
    const uint8_t offscreenBits = enemy_OffscreenBits_; // check offscreen information

    if ((offscreenBits & 0b00000100) != 0)
    { // d2: set for right column sprites and move them offscreen
        MoveESprColOffscreen(4, e);
    } // LcChk
    if ((offscreenBits & 0b00001000) != 0)
    { // d3: set for left column sprites and move them offscreen
        MoveESprColOffscreen(0, e);
    } // Row3C
    if ((offscreenBits & 0b00100000) != 0)
    { // d5: set for the third row of sprites and move them offscreen
        MoveESprRowOffscreen(0x10, e);
    } // Row23C
    if ((offscreenBits & 0b01000000) != 0)
    { // d6: set for the second and third rows and move them offscreen
        MoveESprRowOffscreen(8, e);
    } // AllRowC
    if ((offscreenBits & 0b10000000) == 0)
    {
        return; // d7 is what moves all the sprites offscreen
    }

    MoveESprRowOffscreen(0, e); // move all sprites offscreen

    if (M(Enemy_ID + e) == Podoboo)
    {
        return; // skip this part if found, we do not want to erase podoboo!
    }
    if (M(Enemy_Y_HighPos + e) != 2)
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
    ram[Sprite_Data + 16 + spriteOffset] = MoveColOffscreen(spriteOffset);
}

//------------------------------------------------------------------------

// Inputs: yPosOffset = sprite Y-position offset
// Outputs: return value = 0xf8 (offscreen Y sentinel), consumed by MoveESprColOffscreen
uint8_t SMBEngine::MoveColOffscreen(uint8_t yPosOffset)
{
    // move offscreen two OAMs on the left side (or two rows of enemy on either side if
    // branched here from the enemy graphics handler)
    ram[Sprite_Y_Position + yPosOffset] = 0xf8;
    ram[Sprite_Y_Position + 8 + yPosOffset] = 0xf8;

    return 0xf8; // ExDBlk
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::MoveD_EnemyVertically(uint8_t e)
{
    // a quick movement amount downwards, unless the enemy state calls for a different one
    SetHiMax(e, M(Enemy_State + e) == 0x05 ? 0x20 : 0x3d); // ContVMove: jump to skip the rest of this
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

    // get relative vertical coordinate, for the first and third sprites
    const uint8_t topY = (uint8_t)(fireball_Rel_YPos_ - 4);
    ram[Sprite_Y_Position + spriteDataBase] = topY;
    ram[Sprite_Y_Position + 8 + spriteDataBase] = topY;
    // and eight pixels down, for the second and fourth sprites
    ram[Sprite_Y_Position + 4 + spriteDataBase] = (uint8_t)(topY + 8);
    ram[Sprite_Y_Position + 12 + spriteDataBase] = (uint8_t)(topY + 8);

    // get relative horizontal coordinate, for the first and second sprites
    const uint8_t leftX = (uint8_t)(fireball_Rel_XPos_ - 4);
    ram[Sprite_X_Position + spriteDataBase] = leftX;
    ram[Sprite_X_Position + 4 + spriteDataBase] = leftX;
    // and eight pixels along, for the third and fourth sprites
    ram[Sprite_X_Position + 8 + spriteDataBase] = (uint8_t)(leftX + 8);
    ram[Sprite_X_Position + 12 + spriteDataBase] = (uint8_t)(leftX + 8);

    // set palette attributes for all sprites, but
    ram[Sprite_Attributes + spriteDataBase] = 0x02;      // set no flip at all for first sprite
    ram[Sprite_Attributes + 4 + spriteDataBase] = 0x82;  // set vertical flip for second sprite
    ram[Sprite_Attributes + 8 + spriteDataBase] = 0x42;  // set horizontal flip for third sprite
    ram[Sprite_Attributes + 12 + spriteDataBase] = 0xc2; // set both flips for fourth sprite
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
    if (enemyId == Spiny || enemyId == GreenParatroopaJump || enemyId < 7)
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
    ram[Enemy_X_Speed + e] = -M(Enemy_X_Speed + e);
    // Turn the enemy around:
    ram[Enemy_MovingDir + e] = M(Enemy_MovingDir + e) ^ 0b00000011;
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
    altEntranceControl_ = 2;
    ChgAreaMode(); // set modes
}

//------------------------------------------------------------------------

// set flag to disable screen output
// Inputs: none
// Outputs: none
void SMBEngine::ChgAreaMode()
{
    ++disableScreenFlag_;
    operMode_Task_ = 0;        // set secondary mode of operation
    sprite0HitDetectFlag_ = 0; // disable sprite 0 check
}

//------------------------------------------------------------------------

// Inputs: species = species stunned; e = enemy object buffer offset
// Outputs: none
void SMBEngine::ChkToStunEnemies(uint8_t species, uint8_t e)
{
    // Should we clip this enemy's wings on top of simply stunning it?
    const bool demote = species == 9               // unused value
                        || species == PiranhaPlant // included for some reason...
                        || species == GreenParatroopaJump || species == RedParatroopa || species == GreenParatroopaFly;

    if (demote)
    {
        // "species & 1" is a Green or Red Koopa.
        ram[Enemy_ID + e] = species & 1;
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
    ram[Enemy_State + e] = (M(Enemy_State + e) & 0b11110000) | 0b00000010;

    --M(Enemy_Y_Position + e);
    --M(Enemy_Y_Position + e); // subtract two pixels from enemy's vertical position

    // set the default vertical speed, unless this is a bloober or the area type is water,
    // in which case change it
    const bool defaultYSpeed = M(Enemy_ID + e) != Bloober && areaType_ != 0;
    ram[Enemy_Y_Speed + e] = defaultYSpeed ? 0xfd : 0xff; // SetWYSpd/SetNotW

    // get horizontal difference between player and enemy object: the moving direction is 1
    // if the enemy is to the right of the player, 2 if not
    const uint8_t diffHighByte = std::get<1>(PlayerEnemyDiff(e));
    const uint8_t movingDir = (diffHighByte & 0x80) != 0 ? 0x02 : 0x01;

    // ChkBBill: if either bullet bill is found, the direction does not change
    const uint8_t enemyId = M(Enemy_ID + e);
    if (enemyId != BulletBill_CannonVar && enemyId != BulletBill_FrenzyVar)
    {
        ram[Enemy_MovingDir + e] = movingDir; // store as moving direction
    }

    // NoCDirF: decrement and use as offset to get the proper horizontal speed, then store
    ram[Enemy_X_Speed + e] = EnemyBGCXSpdData_data[movingDir - 1];
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset
// Outputs: none
void SMBEngine::GetEnemyBoundBox(uint8_t e)
{
    // the first bitmask is for an enemy object onscreen, the second for one offscreen
    GetMaskedOffScrBits(e, 0x44, 0x48);
}

//------------------------------------------------------------------------

// Inputs: e = enemy object buffer offset; defaultBitmask = bitmask for an enemy object beyond
// the left edge (e.g. 0x44); onscreenBitmask = bitmask for one to the right of it
// Outputs: none
void SMBEngine::GetMaskedOffScrBits(uint8_t e, uint8_t defaultBitmask, uint8_t onscreenBitmask)
{
    // the enemy object and the left side of the screen are each one 16-bit page:coordinate
    const uint32_t wide = ((M(Enemy_PageLoc + e) << 8) | M(Enemy_X_Position + e)) - ((screenLeft_PageLoc_ << 8) | screenLeft_X_Pos_);
    const uint8_t diffLowByte = LOBYTE(wide); // keep here

    // an enemy object beyond the left edge, or precisely at it, uses the default bitmask;
    // one to the right of the left edge uses the other one instead
    const uint8_t diffHighByte = HIBYTE(wide);
    const bool beyondOrAtLeftEdge = (diffHighByte & 0x80) != 0 || (diffHighByte | diffLowByte) == 0;
    const uint8_t bitmask = beyondOrAtLeftEdge ? defaultBitmask : onscreenBitmask; // CMBits

    // preserve bitwise whatever's in here, and save the masked offscreen bits here
    const uint8_t maskedBits = bitmask & enemy_OffscreenBits_;
    ram[EnemyOffscrBitsMasked + e] = maskedBits;

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
    ram[EnemyBoundingBoxCoord + boxOffset] = 0xff;
    ram[EnemyBoundingBoxCoord + 1 + boxOffset] = 0xff;
    ram[EnemyBoundingBoxCoord + 2 + boxOffset] = 0xff;
    ram[EnemyBoundingBoxCoord + 3 + boxOffset] = 0xff;
}

//------------------------------------------------------------------------

// Inputs: subroutineNum = subroutine number to run next frame
// Outputs: none
void SMBEngine::UpToFiery(uint8_t subroutineNum)
{
    // 0 is the value to be used as the new player state; set values to stop certain
    // things in motion
    SetPRout(subroutineNum, 0);

    // NoPUp
}

//------------------------------------------------------------------------

// set new player state
// Inputs: subroutineNum = subroutine number to run next frame
// Outputs: none
void SMBEngine::SetKRout(uint8_t subroutineNum) { SetPRout(subroutineNum, 1); }

//------------------------------------------------------------------------

// load new value to run subroutine on next frame
// Inputs: subroutineNum = subroutine number; newPlayerState = new player state
// Outputs: none
void SMBEngine::SetPRout(uint8_t subroutineNum, uint8_t newPlayerState)
{
    gameEngineSubroutine_ = subroutineNum; // run this subroutine on the next frame
    player_State_ = newPlayerState;        // store new player state
    timerControl_ = 0xff;                   // set master timer control flag to halt timers
    scrollAmount_ = 0;                     // initialize scroll speed

    ExInjColRoutines();
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ExInjColRoutines() {}

//------------------------------------------------------------------------
// Inputs: comparisonValue = enemy id/state comparison value; e = enemy object buffer
// offset
// Outputs: none
void SMBEngine::ChkForDemoteKoopa(uint8_t comparisonValue, uint8_t e)
{
    const uint8_t RevivalRateData_data[] = {0x10, 0x0b};

    const uint8_t DemotedKoopaXSpdData_data[] = {0x08, 0xf8};

    if (comparisonValue >= 9)
    {
        // demote koopa paratroopas to ordinary troopas
        ram[Enemy_ID + e] = comparisonValue & 0b00000001;
        ram[Enemy_State + e] = 0; // return enemy to normal state
        SetupFloateyNumber(3, e); // award 400 points to the player
        InitVStf(e);              // nullify physics-related thing and vertical speed

        const uint8_t facingIdx = EnemyFacePlayer(e); // turn enemy around if necessary
        // set appropriate moving speed based on direction
        ram[Enemy_X_Speed + e] = DemotedKoopaXSpdData_data[facingIdx];
    } // HandleStompedShellE
    else // then move onto something else
    {
        ram[Enemy_State + e] = 4; // set defeated state for enemy
        ++stompChainCounter_;     // increment the stomp counter

        // award points according to whatever is in the stomp counter, plus the stomp timer
        SetupFloateyNumber((uint8_t)(stompChainCounter_ + stompTimer_), e);
        ++stompTimer_; // increment stomp timer of some sort

        // check primary hard mode flag and load the timer setting according to it, setting
        // it as the enemy timer to revive the stomped enemy
        ram[EnemyIntervalTimer + e] = RevivalRateData_data[primaryHardMode_];
    }

    // SBnce: set player's vertical speed for bounce, and then leave!!!
    player_Y_Speed_ = 0xfc;
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
        ram[Enemy_Y_Position + e] = stunValue;
    }

    ChkToStunEnemies(stunValue, e); // StnE: do yet another sub

    // mask out 2 MSB of enemy object's state, set d5 to defeat the enemy, and save as new state
    ram[Enemy_State + e] = (M(Enemy_State + e) & 0b00011111) | 0b00100000;

    const uint8_t defeatedId = M(Enemy_ID + e);

    uint8_t points = 2; // award 200 points by default
    if (defeatedId == HammerBro)
    {
        points = 6; // award 1000 points for hammer bro
    } // GoombaPoints
    if (defeatedId == Goomba)
    {
        points = 1; // award 100 points for goomba
    }

    EnemySmackScore(points, e);
}

//------------------------------------------------------------------------

// Inputs: pointsControl = points-control value; e = enemy object buffer offset
// Outputs: none
void SMBEngine::EnemySmackScore(uint8_t pointsControl, uint8_t e)
{
    SetupFloateyNumber(pointsControl, e); // update necessary score variables
    square1SoundQueue_ = Sfx_EnemySmack;  // play smack enemy sound

    // ExHCF: and now let's leave
}

//------------------------------------------------------------------------

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::InjurePlayer()
{
    // check again to see if the injured invincibility timer is at zero, and leave if not
    if (injuryTimer_ != 0)
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
    const uint8_t playerStatus = playerStatus_; // check player's status
    if (playerStatus == 0)
    { // KillPlayer: halt the player's horizontal movement by initializing the speed with
        // the zero the status just gave us
        player_X_Speed_ = playerStatus;
        eventMusicQueue_ = playerStatus + 1; // set event music queue to death music
        player_Y_Speed_ = 0xfc;              // set new vertical speed

        SetKRout(Gs_PlayerDeath); // branch to set player's state and other things
        return;
    }

    playerStatus_ = mustBeZero; // otherwise set player's status to small
    injuryTimer_ = 8;           // set injured invincibility timer
    square1SoundQueue_ = 0x10;  // play pipedown/injury sound
    GetPlayerColors();          // change player's palette if necessary

    SetKRout(Gs_PlayerInjuryBlink); // set subroutine to run on next frame
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
        ram[Enemy_MovingDir + e] = movingDir;             // and give it back

        ram[Enemy_State + e] = 0b00100000; // set d5 in enemy state
        InitVStf(e);                       // nullify vertical speed, physics-related thing,
        // and horizontal speed -- the original stores the zero InitVStf leaves in A here,
        // not the $20 it loaded just above
        ram[Enemy_X_Speed + e] = 0;

        player_Y_Speed_ = 0xfd; // set player's vertical speed, to give bounce
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
        square1SoundQueue_ = Sfx_EnemyStomp;

        // the points data offset for stomped enemies starts at zero and is incremented for
        // the enemies worth more
        const uint8_t stompedId = M(Enemy_ID + e);
        if (stompedId == FlyingCheepCheep || stompedId == BulletBill_FrenzyVar || stompedId == BulletBill_CannonVar ||
            stompedId == Podoboo) // (the podoboo is unreachable, due to the earlier checking of it)
        {
            enemyStompedPts(0);
        }
        else if (stompedId == HammerBro)
        {
            enemyStompedPts(1);
        }
        else if (stompedId == Lakitu)
        {
            enemyStompedPts(2);
        }
        else if (stompedId == Bloober)
        {
            enemyStompedPts(3);
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
        const uint8_t playerYSpeed = player_Y_Speed_;
        if ((playerYSpeed & 0x80) == 0 && playerYSpeed != 0)
        {
            enemyStomped();
            return;
        }

        // ChkInj: for enemy objects => $07, add 12 pixels to the player's vertical
        // position; if that is above (less than) the enemy's, it is a stomp
        if (M(Enemy_ID + e) >= Bloober && (uint8_t)(player_Y_Position_ + 12) < M(Enemy_Y_Position + e))
        {
            enemyStomped();
            return;
        }

        // ChkETmrs: check stomp timer
        if (stompTimer_ != 0)
        {
            enemyStomped();
            return;
        }
        // check to see if the injured invincibility timer is still counting down
        if (injuryTimer_ != 0)
        {
            ExInjColRoutines(); // branch elsewhere to leave if so
            return;
        }

        // if the player is to the right of the enemy and the enemy is moving to the right,
        // or the player is to its left and it is not, hurt the player; otherwise turn the
        // enemy around first
        const bool playerRightOfEnemy = player_Rel_XPos_ >= enemy_Rel_XPos_;
        const bool enemyMovingRight = M(Enemy_MovingDir + e) == 1;
        if (playerRightOfEnemy == enemyMovingRight)
        {
            InjurePlayer(); // go back to hurt player
        }
        else
        {
            // LInj: turn the enemy around, then go back to hurt player
            EnemyTurnAround(e);
            InjurePlayer();
        }
    };

    // HandlePowerUpCollision
    const auto handlePowerUpCollision = [&]()
    {
        EraseEnemyObject(e);                  // erase the power-up object
        SetupFloateyNumber(6, e);             // award 1000 points to player by default
        square2SoundQueue_ = Sfx_PowerUpGrab; // play the power-up sound

        const uint8_t powerUpType = powerUpType_; // check power-up type
        if (powerUpType == 3)
        {
            // SetFor1Up: change 1000 points into a 1-up instead, and then leave
            ram[FloateyNum_Control + e] = 11;
            return;
        }
        if (powerUpType >= 2)
        {
            // otherwise set star mario invincibility timer, and load the star mario music
            // into the area music queue, then leave
            starInvincibleTimer_ = 35;
            areaMusicQueue_ = StarPowerMusic;
            return;
        }

        // Shroom_Flower_PUp
        const uint8_t playerStatus = playerStatus_;
        if (playerStatus == 0)
        {
            // UpToSuper: set player status to super
            playerStatus_ = 1;
            UpToFiery(Gs_PlayerChangeSize); // set value to be used by subroutine tree (super)
            return;
        }
        if (playerStatus != 1)
        {
            return;
        }

        playerStatus_ = 2;              // set player status to fiery
        GetPlayerColors();              // run sub to change colors of player
        UpToFiery(Gs_PlayerFireFlower); // set value to be used by subroutine tree (fiery)
    };

    // check counter for d0 set
    if ((frameCounter_ & 0x01) != 0)
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
    if (gameEngineSubroutine_ != Gs_PlayerCtrlRoutine)
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

    if (!collisionFound)
    {
        // NoPECol: clear d0 of the current enemy object's collision bits
        ram[Enemy_CollisionBits + e] = M(Enemy_CollisionBits + e) & 0b11111110;
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
    if (starInvincibleTimer_ != 0)
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
    ram[Enemy_CollisionBits + e] = M(Enemy_CollisionBits + e) | 0x01;

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
    if (areaType_ == 0)
    {
        InjurePlayer(); // branch if water type level
        return;
    }

    // branch if d7 of the enemy state was set, or if all but its 3 LSB masked out is less
    // than two
    if ((M(Enemy_State + e) & 0x80) != 0 || (M(Enemy_State + e) & 0b00000111) < 2)
    {
        chkForPlayerInjury();
        return;
    }
    if (M(Enemy_ID + e) == Goomba)
    {
        return; // branch to leave if goomba in defeated state
    }

    // play smack enemy sound
    square1SoundQueue_ = Sfx_EnemySmack;
    // set d7 in enemy state, thus become moving shell
    ram[Enemy_State + e] = M(Enemy_State + e) | 0b10000000;

    // set moving direction and get offset, then load and set horizontal speed data with it
    const uint8_t facingIdx = EnemyFacePlayer(e);
    ram[Enemy_X_Speed + e] = KickedShellXSpdData_data[facingIdx];

    // add three to whatever the stomp counter contains, unless the shell enemy's timer is
    // near expiring, in which case set the points based on that proximity instead
    const uint8_t shellTimer = M(EnemyIntervalTimer + e);
    const uint8_t points = shellTimer < 3 ? KickedShellPtsData_data[shellTimer] : (uint8_t)(3 + stompChainCounter_);

    SetupFloateyNumber(points, e); // KSPts: set values for floatey number now

    // ExPEC: leave!!!
}
