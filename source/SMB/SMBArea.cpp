// The AreaParserTaskHandler subsystem: everything AreaParserTaskHandler() reaches that nothing
// outside it reaches, and so nothing outside it needs.
//
// Moved out of SMB.cpp by tools/split.py. These are methods of SMBEngine like every other
// routine of the game and are declared in SMBEngine.hpp; the file they are written in is the
// only thing that is different, and tools/layers.py is what keeps it meaning something.
//
#include <algorithm>

#include "SMB.hpp"

// Inputs: none
// Outputs: none
void SMBEngine::IncAreaObjOffset()
{
    ++M(AreaDataOffset); // increment offset of level pointer
    ++M(AreaDataOffset);
    writeData(AreaObjectPageSel, 0x00); // reset page select
}

// Inputs: none
// Outputs: outSlot = index of the empty enemy slot found (valid when the return value is false)
bool SMBEngine::FindEmptyEnemySlot(uint8_t &outSlot)
{
    for (uint8_t slot = 0x00; slot != 0x05; ++slot)
    {
        if (M(Enemy_Flag + slot) == 0)
        { // found an empty slot
            outSlot = slot;
            return false;
        }
    }
    return true; // all values nonzero
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: returns length/height nybble of the object; also writes the row location to zero-page
// 0x07 (real RAM state relied upon by other functions, so that write stays)
uint8_t SMBEngine::GetLrgObjAttrib(uint8_t areaObjBufferOffset)
{
    uint8_t offset = M(AreaObjOffsetBuffer + areaObjBufferOffset); // get offset saved from area obj decoding routine
    // get first byte of level object
    uint8_t row = M(W(AreaData) + offset) & 0b00001111;
    writeData(0x07, row); // save row location
    ++offset;
    // get next byte, save lower nybble (length or height)
    return M(W(AreaData) + offset) & 0b00001111;
}

// Inputs: none (reads CurrentColumnPos from memory)
// Outputs: returns horizontal pixel coordinate
uint8_t SMBEngine::GetAreaObjXPosition()
{
    return M(CurrentColumnPos) << 4; // multiply current offset where we're at by 16 to obtain horizontal pixel coordinate
}

// Inputs: none (reads row from zero-page 0x07, not a register)
// Outputs: returns vertical pixel coordinate
uint8_t SMBEngine::GetAreaObjYPosition()
{
    // multiply value by 16 to get proper vertical pixel coordinate, then add 32 pixels for the status bar
    return static_cast<uint8_t>((M(0x07) << 4) + 32);
}

// use area object identifier bit as offset
// Inputs: none (reads memory 0x00 for the frenzy id)
// Outputs: none
void SMBEngine::AreaFrenzy()
{
    const uint8_t FrenzyIDData_data[] = {FlyCheepCheepFrenzy, BBill_CCheep_Frenzy, Stop_Frenzy};

    uint8_t frenzyId = FrenzyIDData_data[M(0x00) - 8]; // note that it starts at 8, thus weird address here
    uint8_t queueValue = frenzyId;
    // check regular slots of enemy object buffer for the frenzy already being present
    for (uint8_t slot = 5; slot != 0;)
    {
        --slot;
        if (M(Enemy_ID + slot) == frenzyId)
        {
            queueValue = 0x00; // if enemy object already present, nullify queue and leave
            break;
        }
    }
    writeData(EnemyFrenzyQueue, queueValue); // store enemy into frenzy queue
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::WaterPipe(uint8_t areaObjBufferOffset)
{
    GetLrgObjAttrib(areaObjBufferOffset); // get row and lower nybble (length loaded as residual code, water pipe is 1 col thick)
    uint8_t row = M(0x07);
    writeData(MetatileBuffer + row, 0x6b); // draw something here and below it
    writeData(MetatileBuffer + 1 + row, 0x6c);
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::Jumpspring(uint8_t areaObjBufferOffset)
{
    GetLrgObjAttrib(areaObjBufferOffset);
    uint8_t enemySlot = 0;
    FindEmptyEnemySlot(enemySlot);                 // find empty space in enemy object buffer
    uint8_t xPos = GetAreaObjXPosition();          // get horizontal coordinate for jumpspring
    writeData(Enemy_X_Position + enemySlot, xPos); // and store
    // store page location of jumpspring
    writeData(Enemy_PageLoc + enemySlot, M(CurrentPageLoc));
    uint8_t yPos = GetAreaObjYPosition();              // get vertical coordinate for jumpspring
    writeData(Enemy_Y_Position + enemySlot, yPos);     // and store
    writeData(Jumpspring_FixedYPos + enemySlot, yPos); // store as permanent coordinate here
    writeData(Enemy_ID + enemySlot, JumpspringObject); // write jumpspring object to enemy object buffer
    writeData(Enemy_Y_HighPos + enemySlot, 0x01);      // store vertical high byte
    ++M(Enemy_Flag + enemySlot);                       // set flag for enemy object buffer
    uint8_t row = M(0x07);
    // draw metatiles in two rows where jumpspring is
    writeData(MetatileBuffer + row, 0x67);
    writeData(MetatileBuffer + 1 + row, 0x68);
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::CastleObject(uint8_t areaObjBufferOffset)
{
    const uint8_t CastleMetatiles_data[] = {0x00, 0x45, 0x45, 0x45, 0x00, 0x00, 0x48, 0x47, 0x46, 0x00, 0x45, 0x49, 0x49, 0x49,
                                            0x45, 0x47, 0x47, 0x4a, 0x47, 0x47, 0x47, 0x47, 0x4b, 0x47, 0x47, 0x49, 0x49, 0x49,
                                            0x49, 0x49, 0x47, 0x4a, 0x47, 0x4a, 0x47, 0x47, 0x4b, 0x47, 0x4b, 0x47, 0x47, 0x47,
                                            0x47, 0x47, 0x47, 0x4a, 0x47, 0x4a, 0x47, 0x4a, 0x4b, 0x47, 0x4b, 0x47, 0x4b};

    uint8_t startingRow = GetLrgObjAttrib(areaObjBufferOffset); // save lower nybble as starting row
    writeData(0x07, startingRow);                               // if starting row is above $0a, game will crash!!!
    ChkLrgObjFixedLength(areaObjBufferOffset, 0x04);            // load length of castle if not already loaded

    uint8_t castleDataOffset = M(AreaObjectLength + areaObjBufferOffset); // use current length as offset for castle data
    uint8_t col = M(0x07);                                                // begin at starting row
    writeData(0x06, 0x0b);                                                // load upper limit of number of rows to print

    do // CRendLoop: load current byte using offset
    {
        writeData(MetatileBuffer + col, CastleMetatiles_data[castleDataOffset]);
        ++col; // store in buffer and increment buffer offset
        if (M(0x06) != 0)
        {                          // have we reached upper limit yet?
            castleDataOffset += 5; // if not, increment column-wise to byte in next row
            --M(0x06);             // move closer to upper limit
        } // ChkCFloor: have we reached the row just before floor?
    } while (col != 0x0b); // if not, go back and do another row

    if (M(CurrentPageLoc) == 0)
    {
        return; // if we're at page 0, we do not need to do anything else
    }
    uint8_t length = M(AreaObjectLength + areaObjBufferOffset); // check length
    // PlayerStop conditions: second column, or the third column of a tall castle ($00 starting row)
    if (length == 0x01 || (startingRow == 0 && length == 0x03))
    {
        // put brick at floor to stop player at end of level (only on the second column)
        writeData(MetatileBuffer + 10, 0x52);
        return;
    } // NotTall: if not tall castle, check to see if we're at the third column
    if (length != 0x02)
    {
        return; // if we aren't and the castle is tall, don't create flag yet
    }
    // otherwise, obtain and save horizontal pixel coordinate
    uint8_t xPos = GetAreaObjXPosition();
    uint8_t enemySlot = 0;
    FindEmptyEnemySlot(enemySlot);                           // find an empty place on the enemy object buffer
    writeData(Enemy_X_Position + enemySlot, xPos);           // then write horizontal coordinate for star flag
    writeData(Enemy_PageLoc + enemySlot, M(CurrentPageLoc)); // set page location for star flag
    writeData(Enemy_Y_HighPos + enemySlot, 0x01);            // set vertical high byte
    writeData(Enemy_Flag + enemySlot, 0x01);                 // set flag for buffer
    writeData(Enemy_Y_Position + enemySlot, 0x90);           // set vertical coordinate
    writeData(Enemy_ID + enemySlot, StarFlagObject);         // set star flag value in buffer itself
    // ExitCastle
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: returns remaining pipe length
uint8_t SMBEngine::GetPipeHeight(uint8_t areaObjBufferOffset)
{
    ChkLrgObjFixedLength(areaObjBufferOffset, 0x01); // pipe length of 2 (horizontal)
    uint8_t heightNybble = GetLrgObjAttrib(areaObjBufferOffset);
    writeData(0x06, heightNybble & 0x07);             // save only the three lower bits as vertical length
    return M(AreaObjectLength + areaObjBufferOffset); // length left over
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: outLength = length/height nybble (from GetLrgObjAttrib)
bool SMBEngine::ChkLrgObjLength(uint8_t areaObjBufferOffset, uint8_t &outLength)
{
    bool lrgObjJustStarted = false;

    outLength = GetLrgObjAttrib(areaObjBufferOffset); // get row location and size (length if branched to from here)
    lrgObjJustStarted = ChkLrgObjFixedLength(areaObjBufferOffset, outLength);
    return lrgObjJustStarted;
}

// Inputs: areaObjBufferOffset = area object buffer offset; lengthIfUnset = length to store if the
// object's length counter is not yet set
// Outputs: none (the bool return communicates "just started", like a status flag)
bool SMBEngine::ChkLrgObjFixedLength(uint8_t areaObjBufferOffset, uint8_t lengthIfUnset)
{
    uint8_t length = M(AreaObjectLength + areaObjBufferOffset); // check for set length counter
    if ((length & 0x80) != 0)
    { // if counter not set, load it, otherwise leave alone
        writeData(AreaObjectLength + areaObjBufferOffset, lengthIfUnset); // save length into length counter
        return true;                                                     // just starting
    } // LenSet
    return false; // not just starting
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::Hole_Water(uint8_t areaObjBufferOffset)
{
    uint8_t length = 0;
    ChkLrgObjLength(areaObjBufferOffset, length); // get low nybble and save as length (result unused)
    writeData(MetatileBuffer + 10, 0x86);         // render waves
    RenderUnderPart(0x87, 0x0b, 1);               // now render the water underneath
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::FlagBalls_Residual(uint8_t areaObjBufferOffset)
{
    uint8_t numRows = GetLrgObjAttrib(areaObjBufferOffset); // get low nybble from object byte
    RenderUnderPart(0x6d, 0x02, numRows); // render flag balls on third row from top of screen downwards based on low nybble
}

// Inputs: none
// Outputs: none
void SMBEngine::FlagpoleObject()
{
    uint32_t wide = 0;

    writeData(MetatileBuffer, 0x24);      // render flagpole ball on top
    RenderUnderPart(0x25, 0x01, 0x08);    // now render the flagpole shaft
    writeData(MetatileBuffer + 10, 0x61); // render solid block at the bottom
    uint8_t xPos = GetAreaObjXPosition();
    wide = ((M(CurrentPageLoc) << 8) | xPos) - 0x08; // subtract eight pixels, horizontal coordinate for the flag
    writeData(Enemy_X_Position + 5, LOBYTE(wide));
    writeData(Enemy_PageLoc + 5, HIBYTE(wide));  // page location for the flag
    writeData(Enemy_Y_Position + 5, 0x30);       // set vertical coordinate for flag
    writeData(FlagpoleFNum_Y_Pos, 0xb0);         // set initial vertical coordinate for flagpole's floatey number
    writeData(Enemy_ID + 5, FlagpoleFlagObject); // set flag identifier, note identifier and coordinates use last slot
    ++M(Enemy_Flag + 5);                         // use last space in enemy object buffer
}

// Inputs: areaObjBufferOffset = area object buffer offset (consumed transitively by GetRow's callees)
// Outputs: none
void SMBEngine::RowOfCoins(uint8_t areaObjBufferOffset)
{
    const uint8_t CoinMetatileData_data[] = {0xc3, 0xc2, 0xc2, 0xc2};

    uint8_t tile = CoinMetatileData_data[M(AreaType)]; // load appropriate coin metatile
    GetRow(tile, areaObjBufferOffset);
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::EmptyBlock(uint8_t areaObjBufferOffset)
{
    GetLrgObjAttrib(areaObjBufferOffset); // get row location (written to M(0x07); return value unused)
    uint8_t row = M(0x07);
    ColObj(0xc4, row);
}

// column length of 1
// Inputs: tile = metatile to draw; startCol = starting row
// Outputs: none
void SMBEngine::ColObj(uint8_t tile, uint8_t startCol) { RenderUnderPart(tile, startCol, 0); }

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::RowOfBricks(uint8_t areaObjBufferOffset)
{
    uint8_t areaType = M(AreaType); // load area type obtained from area offset pointer
    // check for cloud type override
    if (M(CloudTypeOverride) != 0)
    {
        areaType = 0x04; // if cloud type, override area type
    } // DrawBricks: get appropriate metatile
    GetRow(M(BrickMetatiles + areaType), areaObjBufferOffset); // and go render it
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::RowOfSolidBlocks(uint8_t areaObjBufferOffset)
{
    uint8_t tile = M(SolidBlockMetatiles + M(AreaType)); // get metatile for this area type
    GetRow(tile, areaObjBufferOffset);
}

// store metatile here
// Inputs: tile = metatile to draw; areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::GetRow(uint8_t tile, uint8_t areaObjBufferOffset)
{
    uint8_t length = 0;

    ChkLrgObjLength(areaObjBufferOffset, length); // get row number, load length
    DrawRow(tile);
}

// Inputs: tile = metatile to draw; reads row from zero-page 0x07
// Outputs: none
void SMBEngine::DrawRow(uint8_t tile)
{
    RenderUnderPart(tile, M(0x07), 0); // render object
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::ColumnOfBricks(uint8_t areaObjBufferOffset)
{
    uint8_t tile = M(BrickMetatiles + M(AreaType)); // get metatile (no cloud override as for row)
    GetRow2(tile, areaObjBufferOffset);
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::ColumnOfSolidBlocks(uint8_t areaObjBufferOffset)
{
    uint8_t tile = M(SolidBlockMetatiles + M(AreaType)); // get metatile for this area type
    GetRow2(tile, areaObjBufferOffset);
}

// save metatile to stack for now
// Inputs: tile = metatile to draw; areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::GetRow2(uint8_t tile, uint8_t areaObjBufferOffset)
{
    uint8_t numRows = GetLrgObjAttrib(areaObjBufferOffset); // get length and row
    RenderUnderPart(tile, M(0x07), numRows);                // now render the column
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::BulletBillCannon(uint8_t areaObjBufferOffset)
{
    uint8_t numRows = GetLrgObjAttrib(areaObjBufferOffset); // get row and length of bullet bill cannon
    uint8_t col = M(0x07);                                  // start at first row
    writeData(MetatileBuffer + col, 0x64);                  // render bullet bill cannon
    ++col;
    --numRows; // done yet?
    if ((numRows & 0x80) == 0)
    {
        writeData(MetatileBuffer + col, 0x65); // if not, render middle part
        ++col;
        --numRows; // done yet?
        if ((numRows & 0x80) == 0)
        {
            RenderUnderPart(0x66, col, numRows); // if not, render bottom until length expires
        }
    }

    // SetupCannon: get offset for data used by cannons and whirlpools
    uint8_t cannonOffset = M(Cannon_Offset);
    writeData(Cannon_Y_Position + cannonOffset, GetAreaObjYPosition()); // get proper vertical coordinate for cannon
    writeData(Cannon_PageLoc + cannonOffset, M(CurrentPageLoc));        // store page number for cannon here
    writeData(Cannon_X_Position + cannonOffset, GetAreaObjXPosition()); // get proper horizontal coordinate for cannon
    ++cannonOffset;
    if (cannonOffset >= 0x06)
    {
        cannonOffset = 0x00; // if not yet reached sixth cannon, otherwise initialize it
    }
    writeData(Cannon_Offset, cannonOffset); // save new offset
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::StaircaseObject(uint8_t areaObjBufferOffset)
{
    const uint8_t StaircaseRowData_data[] = {3, 3, 4, 5, 6, 7, 8, 9, 10};
    const uint8_t StaircaseHeightData_data[] = {7, 7, 6, 5, 4, 3, 2, 1, 0};

    uint8_t length = 0;
    bool lrgObjJustStarted = ChkLrgObjLength(areaObjBufferOffset, length); // check and load length
    if (lrgObjJustStarted)
    {                                      // if length already loaded, skip init part
        writeData(StaircaseControl, 0x09); // start past the end for the bottom of the staircase
    } // NextStair: move onto next step (or first if starting)
    --M(StaircaseControl);
    uint8_t stair = M(StaircaseControl);
    uint8_t row = StaircaseRowData_data[stair]; // get starting row and height to render
    uint8_t numRows = StaircaseHeightData_data[stair];
    RenderUnderPart(0x61, row, numRows); // now render solid block staircase
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::Hole_Empty(uint8_t areaObjBufferOffset)
{
    const uint8_t HoleMetatiles_data[] = {0x87, 0x00, 0x00, 0x00};

    uint8_t length = 0;
    bool lrgObjJustStarted = ChkLrgObjLength(areaObjBufferOffset, length); // get lower nybble and save as length
    if (lrgObjJustStarted && M(AreaType) == 0)                             // check for water type level
    {
        uint8_t whirlpoolOffset = M(Whirlpool_Offset);                   // get offset for data used by cannons and whirlpools
        uint8_t xPos = GetAreaObjXPosition();                            // get proper coordinate of where we're at
        uint32_t wide = ((M(CurrentPageLoc) << 8) | xPos) - 0x10;        // subtract 16 pixels
        writeData(Whirlpool_LeftExtent + whirlpoolOffset, LOBYTE(wide)); // store as left extent of whirlpool
        writeData(Whirlpool_PageLoc + whirlpoolOffset, HIBYTE(wide));    // save as page location of whirlpool
        // multiply (length+2) by 16 to get size of whirlpool; whirlpool is always two blocks bigger
        // than the actual size of the hole and extends one block beyond each edge
        writeData(Whirlpool_Length + whirlpoolOffset, static_cast<uint8_t>((length + 2) << 4));
        ++whirlpoolOffset;
        if (whirlpoolOffset >= 0x05)
        {
            whirlpoolOffset = 0x00; // if not yet reached fifth whirlpool, otherwise initialize it
        }
        writeData(Whirlpool_Offset, whirlpoolOffset); // save new offset here
    }

    // NoWhirlP: get appropriate metatile, then render the hole proper
    uint8_t tile = HoleMetatiles_data[M(AreaType)];
    RenderUnderPart(tile, 0x08, 0x0f); // start at ninth row and go to bottom
}

// Inputs: tile = metatile to draw; startCol = starting column offset into MetatileBuffer; numRows =
// number of rows to render downward
// Outputs: returns the final column reached (x is left there by the original; RenderSidewaysPipe is
// the one caller that actually depends on this value, to render the pipe parts just past the shaft)
uint8_t SMBEngine::RenderUnderPart(uint8_t tile, uint8_t startCol, uint8_t numRows)
{
    // Note: the countdown is re-read from RAM (AreaObjectHeight) each iteration, exactly as the
    // original does, rather than tracked purely in a local — MetatileBuffer+col can alias
    // AreaObjectHeight when col runs up to/past the 0x0d boundary, and the original's reload would
    // observe that aliasing write. Also note the column bound is only checked *after* incrementing
    // col (in the original's WaitOneRow), so the very first iteration always executes even if
    // startCol >= 0x0d.
    uint8_t col = startCol;
    uint8_t rows = numRows;
    while (true)
    {
        writeData(AreaObjectHeight, rows);          // store vertical length to render
        uint8_t existing = M(MetatileBuffer + col); // check current spot to see if there's something
        // Original branch chain, in order:
        //   existing==0        -> draw
        //   existing==0x17     -> wait (tree ledge middle part)
        //   existing==0x1a     -> wait (mushroom ledge middle part)
        //   existing==0xc0     -> draw (question block w/ coin, overwrite)
        //   existing>=0xc0     -> wait (any other palette-3 metatile)
        //   existing!=0x54     -> draw (cracked rock terrain, overwrite)
        //   tile==0x50         -> wait (stem top of mushroom)
        //   else               -> draw
        bool shouldDraw =
            (existing == 0xc0) || (existing < 0xc0 && existing != 0x17 && existing != 0x1a && (existing != 0x54 || tile != 0x50));
        if (shouldDraw)
        {
            writeData(MetatileBuffer + col, tile); // render contents of tile from routine that called this
        }
        ++col;
        if (col >= 0x0d)
        {
            break; // ExitUPartR
        }
        rows = M(AreaObjectHeight); // decrement, and stop rendering if there is no more length
        --rows;
        if ((rows & 0x80) != 0)
        {
            break;
        }
    }
    return col;
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::AreaStyleObject(uint8_t areaObjBufferOffset)
{
    // load level object style and jump to the right sub
    switch (M(AreaStyle))
    {
    case 0:
        TreeLedge(areaObjBufferOffset); // also used for cloud type levels
        return;
    case 1:
        MushroomLedge(areaObjBufferOffset);
        return;
    case 2:
        BulletBillCannon(areaObjBufferOffset);
        return;
    default:
        bad_jump();
        return;
    }
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::TreeLedge(uint8_t areaObjBufferOffset)
{
    uint8_t ledgeLength = GetLrgObjAttrib(areaObjBufferOffset);        // get row and length of green ledge
    uint8_t lengthCounter = M(AreaObjectLength + areaObjBufferOffset); // check length counter for expiration
    if (lengthCounter == 0)
    { // EndTreeL: render end of tree ledge
        RenderUnderPart(0x18, M(0x07), 0x00);
        return;
    }

    bool middle = (lengthCounter & 0x80) == 0; // length still counting down: render the middle
    if (!middle)
    {
        writeData(AreaObjectLength + areaObjBufferOffset, ledgeLength); // store lower nybble as length of ledge
        // if at the start of the level, render the middle for a continuous effect
        middle = (M(CurrentPageLoc) | M(CurrentColumnPos)) == 0;
    }
    if (!middle)
    {
        RenderUnderPart(0x16, M(0x07), 0x00); // render start of tree ledge
        return;
    }

    // MidTreeL: render middle of tree ledge
    uint8_t col = M(0x07);
    writeData(MetatileBuffer + col, 0x17);
    RenderUnderPart(0x4c, col + 1, 0x0f); // now render the part underneath
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::MushroomLedge(uint8_t areaObjBufferOffset)
{
    uint8_t length = 0;
    bool lrgObjJustStarted = ChkLrgObjLength(areaObjBufferOffset, length); // get shroom dimensions
    writeData(0x06, length);                                               // store length here for now
    if (lrgObjJustStarted)
    {
        // divide length by 2 and store elsewhere
        writeData(MushroomLedgeHalfLen + areaObjBufferOffset, M(AreaObjectLength + areaObjBufferOffset) >> 1);
        RenderUnderPart(0x19, M(0x07), 0x00); // render start of mushroom
        return;
    }

    // EndMushL: if at the end, render end of mushroom
    uint8_t remainingLength = M(AreaObjectLength + areaObjBufferOffset);
    if (remainingLength == 0)
    {
        RenderUnderPart(0x1b, M(0x07), 0x00);
        return;
    }
    // get divided length and store where length was stored originally
    writeData(0x06, M(MushroomLedgeHalfLen + areaObjBufferOffset));
    uint8_t col = M(0x07);
    writeData(MetatileBuffer + col, 0x1a); // render middle of mushroom
    if (remainingLength != M(0x06))
    {
        return; // are we smack dab in the center? if not, branch to leave
    }
    ++col;
    writeData(MetatileBuffer + col, 0x4f); // render stem top of mushroom underneath the middle
    // render the stem all the way down
    RenderUnderPart(0x50, col + 1, 0x0f);
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::PulleyRopeObject(uint8_t areaObjBufferOffset)
{
    const uint8_t PulleyRopeMetatiles_data[] = {0x42, 0x41, 0x43};

    uint8_t length = 0;
    bool lrgObjJustStarted = ChkLrgObjLength(areaObjBufferOffset, length); // get length of pulley/rope object
    uint8_t metatileIndex;
    if (lrgObjJustStarted)
    {
        metatileIndex = 0; // if starting, render left pulley
    }
    else if (M(AreaObjectLength + areaObjBufferOffset) != 0)
    {
        metatileIndex = 1; // if not at the end, render rope
    }
    else
    {
        metatileIndex = 2; // otherwise render right pulley
    }
    writeData(MetatileBuffer, PulleyRopeMetatiles_data[metatileIndex]); // render at the top of the screen
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::VerticalPipe(uint8_t areaObjBufferOffset)
{
    uint8_t pipeDataIndex = GetPipeHeight(areaObjBufferOffset);
    // check to see if value was nullified earlier
    if (M(0x00) != 0)
    {                       // (if d3, the usage control bit of second byte, was set)
        pipeDataIndex += 4; // add four if usage control bit was not set
    }

    // if at world 1-1, do not add piranha plant ever
    if ((M(AreaNumber) | M(WorldNumber)) != 0 &&
        M(AreaObjectLength + areaObjBufferOffset) != 0) // if on second column of pipe, only do this once
    {
        uint8_t enemySlot = 0;
        if (!FindEmptyEnemySlot(enemySlot)) // if not found, too many enemies, thus skip
        {
            uint8_t xPos = GetAreaObjXPosition();                     // get horizontal pixel coordinate
            uint32_t wide = ((M(CurrentPageLoc) << 8) | xPos) + 0x08; // add eight to put the piranha plant in the center
            writeData(Enemy_X_Position + enemySlot, LOBYTE(wide));    // store as enemy's horizontal coordinate
            writeData(Enemy_PageLoc + enemySlot, HIBYTE(wide));       // store as enemy's page coordinate
            writeData(Enemy_Y_HighPos + enemySlot, 0x01);
            writeData(Enemy_Flag + enemySlot, 0x01);                        // activate enemy flag
            writeData(Enemy_Y_Position + enemySlot, GetAreaObjYPosition()); // get piranha plant's vertical coordinate
            writeData(Enemy_ID + enemySlot, PiranhaPlant);                  // write piranha plant's value into buffer
            InitPiranhaPlant(enemySlot);
        }
    }

    uint8_t row = M(0x07); // get buffer offset
    // draw the appropriate pipe with pipeDataIndex
    writeData(MetatileBuffer + row, M(VerticalPipeData + pipeDataIndex)); // render the top of the pipe
    uint8_t tile = M(VerticalPipeData + 2 + pipeDataIndex);               // render the rest of the pipe
    uint8_t numRows = M(0x06) - 1;                                        // subtract one from length and render the part underneath
    RenderUnderPart(tile, row + 1, numRows);
}

void SMBEngine::WriteGameText(uint8_t text_number)
{
    const uint8_t GameTextOffsets_data[] = {
        static_cast<uint8_t>(TopStatusBarLine - GameText),  static_cast<uint8_t>(TopStatusBarLine - GameText),
        static_cast<uint8_t>(WorldLivesDisplay - GameText), static_cast<uint8_t>(WorldLivesDisplay - GameText),
        static_cast<uint8_t>(TwoPlayerTimeUp - GameText),   static_cast<uint8_t>(OnePlayerTimeUp - GameText),
        static_cast<uint8_t>(TwoPlayerGameOver - GameText), static_cast<uint8_t>(OnePlayerGameOver - GameText),
        static_cast<uint8_t>(WarpZoneWelcome - GameText),   static_cast<uint8_t>(WarpZoneWelcome - GameText)};

    const uint8_t LuigiName_data[] = {
        0x15, 0x1e, 0x12, 0x10, 0x12 //  "LUIGI", no address or length
    };

    // Compute index into GameTextOffsets:
    uint8_t index = 2 * text_number;
    if (index >= 4)
    {
        index = std::min<uint8_t>(index, 8); // warp zone
        if (M(NumberOfPlayers) == 0)
        {            // single-player?
            ++index; // increment offset by one to not print name
        }
    }

    uint8_t textOffset = GameTextOffsets_data[index];
    uint8_t bufPos = 0;

    uint8_t ch;
    while ((ch = M(GameText + textOffset)) != 0xff) // terminator
    {
        writeData(VRAM_Buffer1 + bufPos, ch); // otherwise write data to buffer
        ++textOffset;
        ++bufPos;
        if (bufPos == 0)
        {
            break; // do this for 256 bytes if no terminator found
        }
    }
    writeData(VRAM_Buffer1 + bufPos, 0x00);

    if (text_number < 0x04)
    {
        if (text_number == 0x01) // are we printing the world/lives display?
        {
            uint8_t lives = M(NumberofLives) + 1;
            if (lives >= 10)
            {
                lives -= 10;                       // if so, subtract 10 and put a crown tile
                writeData(VRAM_Buffer1 + 7, 0x9f); // next to the difference...strange things happen if
                                                   // the number of lives exceeds 19
            }
            writeData(VRAM_Buffer1 + 8, lives);
            // write world and level numbers (incremented for display) to the buffer in the spaces
            // surrounding the dash
            writeData(VRAM_Buffer1 + 19, M(WorldNumber) + 1);
            writeData(VRAM_Buffer1 + 21, M(LevelNumber) + 1); // we're done here
            return;

            //------------------------------------------------------------------------
        } // CheckPlayerName
        if (M(NumberOfPlayers) == 0)
        {
            return; // if only 1 player, leave
        }
        uint8_t playerBit = M(CurrentPlayer); // load current player
        // if the message is "time up" (0x02) and we're not in game over mode, invert d0 to do the other player
        if (text_number == 0x02 && M(OperMode) != GameOverModeValue)
        {
            playerBit ^= 0b00000001;
        }

        // ChkLuigi
        bool shiftedBit = (playerBit & 0x01) != 0;
        playerBit >>= 1;
        if (!shiftedBit)
        {
            return; // if mario is current player, do not change the name
        }

        // NameLoop: otherwise, replace "MARIO" with "LUIGI"
        for (uint8_t i = 5; i != 0;)
        {
            --i;
            writeData(VRAM_Buffer1 + 3 + i, LuigiName_data[i]);
        }

        return; // ExitChkName

        //------------------------------------------------------------------------
    } // PrintWarpZoneNumbers
    // subtract 4 and then shift to the left twice to get proper warp zone number offset
    uint8_t warpIndex = static_cast<uint8_t>((text_number - 0x04) << 2);
    for (uint8_t n = 0; n < 0x0c; n += 4)
    {
        // print warp zone numbers into the placeholders from earlier
        writeData(VRAM_Buffer1 + 27 + n, M(WarpZoneNumbers + warpIndex));
        ++warpIndex;
    }
    SetVRAMOffset(0x2c); // load new buffer pointer at end of message
}

// Inputs: none
// Outputs: none
void SMBEngine::RenderAttributeTables()
{
    // get low byte of next name table address to be written to, mask out all but 5 LSB
    uint8_t lowByte = (M(CurrentNTAddr_Low) & 0b00011111) - 0x04;
    lowByte &= 0b00011111; // mask out bits again and store
    writeData(0x01, lowByte);
    uint8_t highByte = M(CurrentNTAddr_High); // get high byte and branch if the subtraction above borrowed
    if ((M(CurrentNTAddr_Low) & 0b00011111) < 0x04)
    {
        highByte ^= 0b00000100; // otherwise invert d2
    } // SetATHigh: mask out all other bits
    highByte &= 0b00000100;
    highByte |= 0x23; // add $2300 to the high byte and store
    writeData(0x00, highByte);
    uint8_t v = M(0x01);               // get low byte - 4, divide by 4, add offset for
    bool shiftedBit = (v & 0x02) != 0; // the second shift carries d1 out
    v >>= 1;                           // attribute table and store
    v >>= 1;
    v = (uint8_t)(v + 0xc0 + (shiftedBit ? 1 : 0)); // we should now have the appropriate block of
    writeData(0x01, v);                             // attribute table in our temp address
    uint8_t attribOffset = 0x00;
    uint8_t bufOffset = M(VRAM_Buffer2_Offset); // get buffer offset

    do // AttribLoop
    {
        writeData(VRAM_Buffer2 + bufOffset, M(0x00)); // store high byte of attribute table address
        uint8_t rowByte = M(0x01) + 0x08;             // below the status bar, and store
        writeData(VRAM_Buffer2 + 1 + bufOffset, rowByte);
        writeData(0x01, rowByte); // also store in temp again
        // fetch current attribute table byte and store
        writeData(VRAM_Buffer2 + 3 + bufOffset, M(AttributeBuffer + attribOffset)); // in the buffer
        writeData(VRAM_Buffer2 + 2 + bufOffset, 0x01);                              // store length of 1 in buffer
        writeData(AttributeBuffer + attribOffset, 0x00);                            // clear current byte in attribute buffer
        bufOffset += 4;                                                             // increment buffer offset by 4 bytes
        ++attribOffset;                                                             // increment attribute offset and check to see
    } while (attribOffset < 0x07);
    writeData(VRAM_Buffer2 + bufOffset, 0x00); // put null terminator at the end
    writeData(VRAM_Buffer2_Offset, bufOffset); // store offset in case we want to do any more

    SetVRAMCtrl();
}

// Inputs: none
// Outputs: none
void SMBEngine::SetVRAMCtrl()
{
    writeData(VRAM_Buffer_AddrCtrl, 0x06); // set buffer to $0341 and leave
}

// Inputs: none
// Outputs: none
void SMBEngine::IncrementColumnPos()
{
    ++M(CurrentColumnPos);                       // increment column where we're at
    if ((M(CurrentColumnPos) & 0b00001111) == 0) // mask out higher nybble
    {
        writeData(CurrentColumnPos, 0x00); // if no bits left set, wrap back to zero (0-f)
        ++M(CurrentPageLoc);               // and increment page number where we're at
    } // NoColWrap: increment column offset where we're at
    ++M(BlockBufferColumnPos);
    writeData(BlockBufferColumnPos, M(BlockBufferColumnPos) & 0b00011111); // mask out all but 5 LSB (0-1f) and save
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::AlterAreaAttributes(uint8_t areaObjBufferOffset)
{
    uint8_t offset =
        M(AreaObjOffsetBuffer + areaObjBufferOffset) + 1; // load offset for level object data saved in buffer, then second byte
    uint8_t attrByte = M(W(AreaData) + offset);
    if ((attrByte & 0b01000000) == 0) // branch if d6 is set
    {
        writeData(TerrainControl, attrByte & 0b00001111);           // mask out high nybble, new terrain height type bits
        writeData(BackgroundScenery, (attrByte & 0b00110000) >> 4); // move bits to lower nybble, new background scenery bits
        return;

        //------------------------------------------------------------------------
    } // Alter2
    uint8_t foreScenery = attrByte & 0b00000111; // mask out all but 3 LSB
    if (foreScenery >= 0x04)
    { // nullify foreground scenery bits
        writeData(BackgroundColorCtrl, foreScenery);
        foreScenery = 0x00;
    } // SetFore: otherwise set new foreground scenery bits
    writeData(ForegroundScenery, foreScenery);
}

// Inputs: none
// Outputs: none
void SMBEngine::ScrollLockObject_Warp()
{
    uint8_t warpNum = 0x04; // load value of 4 for game text routine as default (warp zone 4-3-2)
    if (M(WorldNumber) != 0)
    {
        warpNum = 0x05;       // if world number > 1, increment for next warp zone (5)
        if (M(AreaType) == 1) // check area type; if ground area type (1), increment for last warp zone
        {
            warpNum = 0x06; // (8-7-6)
        }
    }
    writeData(WarpZoneControl, warpNum); // store number here to be used by warp zone routine
    WriteGameText(warpNum);              // print text and warp zone numbers
    KillEnemies(PiranhaPlant);           // load identifier for piranha plants and do sub

    ScrollLockObject();
}

// Inputs: none
// Outputs: none
void SMBEngine::ScrollLockObject()
{
    writeData(ScrollLock, M(ScrollLock) ^ 0b00000001); // invert scroll lock to turn it on
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::IntroPipe(uint8_t areaObjBufferOffset)
{
    ChkLrgObjFixedLength(areaObjBufferOffset, 0x03); // check if length set, if not set, set it
    uint8_t pipeDataIndex = 0;
    bool sidePipeShaftDrawn = RenderSidewaysPipe(areaObjBufferOffset, 0x0a, pipeDataIndex); // set fixed value and render the sideways part
    if (sidePipeShaftDrawn)
    { // if the shaft was not drawn, not time to draw vertical pipe part
        // blank everything above the vertical pipe part, all the way to the top of the screen
        uint8_t col = 0x06;
        do // VPipeSectLoop
        {
            writeData(MetatileBuffer + col, 0x00); // because otherwise it will look like exit pipe
            --col;
        } while ((col & 0x80) == 0);
        writeData(MetatileBuffer + 7, M(VerticalPipeData + pipeDataIndex)); // draw the end of the vertical pipe part
    } // NoBlankP
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::ExitPipe(uint8_t areaObjBufferOffset)
{
    ChkLrgObjFixedLength(areaObjBufferOffset, 0x03);               // check if length set, if not set, set it
    uint8_t verticalLength = GetLrgObjAttrib(areaObjBufferOffset); // get vertical length, then plow on through RenderSidewaysPipe
    uint8_t pipeDataIndex = 0;
    RenderSidewaysPipe(areaObjBufferOffset, verticalLength, pipeDataIndex);
}

// Inputs: areaObjBufferOffset = area object buffer offset; verticalLength = vertical length of the
// pipe shaft (decremented internally)
// Outputs: outPipeDataIndex = remaining horizontal length, reused by IntroPipe as an index into
// VerticalPipeData
bool SMBEngine::RenderSidewaysPipe(uint8_t areaObjBufferOffset, uint8_t verticalLength, uint8_t &outPipeDataIndex)
{
    const uint8_t SidePipeBottomPart_data[] = {0x15, 0x21, // bottom part of sideways part of pipe
                                               0x20, 0x1f};

    const uint8_t SidePipeTopPart_data[] = {0x15, 0x1e, // top part of sideways part of pipe
                                            0x1d, 0x1c};

    const uint8_t SidePipeShaftData_data[] = {
        0x15, 0x14, // used to control whether or not vertical pipe shaft
        0x00, 0x00  // is drawn, and if so, controls the metatile number
    };

    verticalLength -= 2; // decrement twice to make room for shaft at bottom, use as vertical length
    uint8_t horizLength = M(AreaObjectLength + areaObjBufferOffset); // get length left over and store here
    writeData(0x06, horizLength);
    uint8_t col = verticalLength + 1; // get vertical length plus one, use as buffer offset

    bool sidePipeShaftDrawn = false;
    uint8_t shaftTile = SidePipeShaftData_data[horizLength]; // check for value $00 based on horizontal offset
    if (shaftTile != 0x00)
    {                                                        // if found, do not draw the vertical pipe shaft
        col = RenderUnderPart(shaftTile, 0, verticalLength); // render vertical shaft using tile number; the loop
                                                             // leaves col at the column just past the shaft, which
                                                             // is where the pipe top/bottom parts below get drawn
        sidePipeShaftDrawn = true;                           // used by IntroPipe
    } // DrawSidePart: render side pipe part at the bottom
    outPipeDataIndex = horizLength;
    writeData(MetatileBuffer + col, SidePipeTopPart_data[horizLength]);        // note that the pipe parts are stored
    writeData(MetatileBuffer + 1 + col, SidePipeBottomPart_data[horizLength]); // backwards horizontally
    return sidePipeShaftDrawn;
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::QuestionBlockRow_High(uint8_t areaObjBufferOffset)
{
    Skip_1(0x03, areaObjBufferOffset); // start on the fourth row
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::QuestionBlockRow_Low(uint8_t areaObjBufferOffset)
{
    Skip_1(0x07, areaObjBufferOffset); // start on the eighth row
}

// Inputs: startRow = starting row; areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::Skip_1(uint8_t startRow, uint8_t areaObjBufferOffset)
{
    bool lrgObjJustStarted = false;
    uint8_t length = 0;

    lrgObjJustStarted = ChkLrgObjLength(areaObjBufferOffset, length); // get low nybble and save as length
    writeData(MetatileBuffer + startRow, 0xc0);                       // render question boxes with coins
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::Bridge_High(uint8_t areaObjBufferOffset)
{
    Skip_3(0x06, areaObjBufferOffset); // inlined Skip_2 // start on the seventh row from top of screen
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::Bridge_Middle(uint8_t areaObjBufferOffset)
{
    Skip_3(0x07, areaObjBufferOffset); // inlined Skip_2 // start on the eighth row
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::Bridge_Low(uint8_t areaObjBufferOffset)
{
    Skip_3(0x09, areaObjBufferOffset); // start on the tenth row
}

// Inputs: startRow = starting row; areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::Skip_3(uint8_t startRow, uint8_t areaObjBufferOffset)
{
    bool lrgObjJustStarted = false;
    uint8_t length = 0;

    lrgObjJustStarted = ChkLrgObjLength(areaObjBufferOffset, length); // get low nybble and save as length
    writeData(MetatileBuffer + startRow, 0x0b);                       // render bridge railing
    RenderUnderPart(0x63, startRow + 1, 0);                           // now render the bridge itself
}

// Inputs: none
// Outputs: none
void SMBEngine::EndlessRope()
{
    DrawRope(0x00, 0x0f); // render rope from the top to the bottom of screen
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::BalancePlatRope(uint8_t areaObjBufferOffset)
{
    // blank out all from second row to the bottom, used for balance platform rope
    RenderUnderPart(0x44, 0x01, 0x0f);
    uint8_t numRows = GetLrgObjAttrib(areaObjBufferOffset); // get vertical length from lower nybble
    DrawRope(0x01, numRows);
}

// render the actual rope
// Inputs: startCol = starting column offset; numRows = vertical length to render
// Outputs: none
void SMBEngine::DrawRope(uint8_t startCol, uint8_t numRows) { RenderUnderPart(0x40, startCol, numRows); }

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::CastleBridgeObj(uint8_t areaObjBufferOffset)
{
    bool lrgObjJustStarted = false;

    lrgObjJustStarted = ChkLrgObjFixedLength(areaObjBufferOffset, 0x0c); // load length of 13 columns
    ChainObj();
}

// Inputs: none
// Outputs: none
void SMBEngine::AxeObj()
{
    writeData(VRAM_Buffer_AddrCtrl, 0x08); // load bowser's palette into sprite portion of palette

    ChainObj();
}

// Inputs: none (reads memory 0x00)
// Outputs: none
void SMBEngine::ChainObj()
{
    const uint8_t C_ObjectMetatile_data[] = {0xc5, 0x0c, 0x89};

    const uint8_t C_ObjectRow_data[] = {0x06, 0x07, 0x08};

    uint8_t objIndex = M(0x00) - 2; // get value loaded earlier from decoder
    // get appropriate row and metatile for object
    uint8_t row = C_ObjectRow_data[objIndex];
    uint8_t tile = C_ObjectMetatile_data[objIndex];
    ColObj(tile, row);
}

// get appropriate metatile for brick (question block
// Inputs: brickQBlockIndex = index into BrickQBlockMetatiles; areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::DrawQBlk(uint8_t brickQBlockIndex, uint8_t areaObjBufferOffset)
{
    uint8_t tile = M(BrickQBlockMetatiles + brickQBlockIndex);
    uint8_t row = GetLrgObjAttrib(areaObjBufferOffset); // get row from location byte (also writes M(0x07) for DrawRow)
    DrawRow(tile);                                      // now render the object
}

// Inputs: none
// Outputs: none
void SMBEngine::RenderAreaGraphics()
{
    const uint8_t MetatileGraphics_High_data[] = {HIBYTE(Palette0_MTiles), HIBYTE(Palette1_MTiles), HIBYTE(Palette2_MTiles),
                                                  HIBYTE(Palette3_MTiles)};

    const uint8_t MetatileGraphics_Low_data[] = {LOBYTE(Palette0_MTiles), LOBYTE(Palette1_MTiles), LOBYTE(Palette2_MTiles),
                                                 LOBYTE(Palette3_MTiles)};

    const uint8_t rightColumn = M(CurrentColumnPos) & 0x01; // store LSB of where we're at
    uint8_t vramOffset = M(VRAM_Buffer2_Offset);  // store vram buffer offset
    writeData(0x00, vramOffset);
    // get current name table address we're supposed to render
    writeData(VRAM_Buffer2 + 1 + vramOffset, M(CurrentNTAddr_Low));
    writeData(VRAM_Buffer2 + vramOffset, M(CurrentNTAddr_High));
    // store length byte of 26 here with d7 set to increment by 32 (in columns)
    writeData(VRAM_Buffer2 + 2 + vramOffset, 0x9a);
    uint8_t attribRowCounter = 0x00; // init attribute row

    uint8_t row = 0x00;
    do // DrawMTLoop: store init value of 0 or incremented offset for buffer
    {
        writeData(0x01, row);
        // get first metatile number, and mask out all but 2 MSB
        uint8_t attribBits = M(MetatileBuffer + row) & 0b11000000; // the attribute table bits
        // note that metatile format is %xx000000 attribute table bits, %00xxxxxx metatile number,
        // so move the bits to d1-d0 and use as offset to get address to graphics table from here
        uint8_t paletteIdx = attribBits >> 6;
        writeData(0x06, MetatileGraphics_Low_data[paletteIdx]);
        writeData(0x07, MetatileGraphics_High_data[paletteIdx]);
        uint8_t tileOffset = M(MetatileBuffer + row) << 1; // get metatile number again, multiply by 4
        tileOffset <<= 1;                                  // and use as tile offset
        writeData(0x02, tileOffset);
        // get current task number for level processing, mask out all but LSB, then invert LSB and
        // multiply by 2 to get the correct column position in the metatile, then add to the tile
        // offset so we can draw either side of the metatiles
        uint8_t gfxIndex = ((M(AreaParserTaskNum) & 0b00000001) ^ 0b00000001) << 1;
        gfxIndex += M(0x02);
        uint8_t vo = M(0x00); // use vram buffer offset from before
        // get first tile number (top left or top right) and store
        writeData(VRAM_Buffer2 + 3 + vo, M(W(0x06) + gfxIndex));
        // now get the second (bottom left or bottom right) and store
        writeData(VRAM_Buffer2 + 4 + vo, M(W(0x06) + gfxIndex + 1));

        uint8_t attribRow = attribRowCounter;      // get current attribute row (index before any increment)
        bool bottomSquare = (M(0x01) & 0x01) != 0; // LSB of current row: clear = top square, set = bottom
        if (rightColumn == 0)
        { // left attribute
            if (bottomSquare)
            {
                attribBits >>= 2;   // shift attribute bits 2 to the right,
                                    // thus into d5-d4 for lower left square
                ++attribRowCounter; // and move onto next attribute row
            }
            else
            {
                attribBits >>= 6; // move the attribute bits into d1-d0, for upper left square
            }
        }
        else
        { // right attribute
            if (bottomSquare)
            {
                ++attribRowCounter; // lower right square: bits already in d7-d6, just move to next attribute row
            }
            else
            {
                attribBits >>= 4; // shift attribute bits 4 to the right,
                                  // thus into d3-d2, for upper right square
            }
        }
        // SetAttrib: get previously saved bits from before, if any, put the new bits onto the old, and store
        writeData(AttributeBuffer + attribRow, M(AttributeBuffer + attribRow) | attribBits);
        ++M(0x00); // increment vram buffer offset by 2
        ++M(0x00);
        ++row;                // get current gfx buffer row, and check for the bottom of the screen
    } while (row < 0x0d); // if not there yet, loop back
    // get current vram buffer offset, increment by 3 (for name table address and length bytes)
    uint8_t endOffset = M(0x00) + 3;
    writeData(VRAM_Buffer2 + endOffset, 0x00); // put null terminator at end of data for name table
    writeData(VRAM_Buffer2_Offset, endOffset); // store new buffer offset
    ++M(CurrentNTAddr_Low);                    // increment name table address low
    // check current low byte; if no wraparound, just skip this part
    if ((M(CurrentNTAddr_Low) & 0b00011111) == 0)
    {
        // if wraparound occurs, make sure low byte stays just under the status bar
        writeData(CurrentNTAddr_Low, 0x80);
        // and then invert d2 of the name table address high to move onto the next appropriate name table
        writeData(CurrentNTAddr_High, M(CurrentNTAddr_High) ^ 0b00000100);
    } // ExitDrawM: jump to set buffer to $0341 and leave
    SetVRAMCtrl();
}

// Inputs: none
// Outputs: none
void SMBEngine::AreaParserTaskHandler()
{
    uint8_t taskNum = M(AreaParserTaskNum); // check number of tasks here
    if (taskNum == 0)
    { // if already set, go ahead
        taskNum = 0x08;
        writeData(AreaParserTaskNum, 0x08); // otherwise, set eight by default
    } // DoAPTasks
    --taskNum;
    AreaParserTasks(taskNum);
    --M(AreaParserTaskNum); // if all tasks not complete do not
    if (M(AreaParserTaskNum) == 0)
    { // render attribute table yet
        RenderAttributeTables();
    } // SkipATRender
}

// Inputs: taskNum = task number (0-7)
// Outputs: none
void SMBEngine::AreaParserTasks(uint8_t taskNum)
{
    switch (taskNum)
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

// Inputs: none
// Outputs: none
void SMBEngine::AreaParserCore()
{
    const uint8_t TerrainMetatiles_data[] = {0x69, 0x54, 0x52, 0x62};

    const uint8_t ForeSceneryData_data[] = {0x86, 0x87, 0x87, 0x87, 0x87, 0x87, 0x87,                                     // in water
                                            0x87, 0x87, 0x87, 0x87, 0x69, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x45, 0x47, // wall
                                            0x47, 0x47, 0x47, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // over water
                                            0x00, 0x00, 0x00, 0x00, 0x86, 0x87};

    const uint8_t FSceneDataOffsets_data[] = {0x00, 0x0d, 0x1a};

    const Mt BackSceneryMetatiles_data[] = {
        // cloud left
        Mt::CloudTopLeft,
        Mt::CloudBottomLeft,
        Mt::Blank,
        // cloud middle
        Mt::CloudTopMiddle,
        Mt::CloudBottomMiddle,
        Mt::Blank,
        // cloud right
        Mt::CloudTopRight,
        Mt::CloudBottomRight,
        Mt::Blank,
        // bush left
        Mt::BushLeft,
        Mt::Blank,
        Mt::Blank,
        // bush middle
        Mt::BushMiddle,
        Mt::Blank,
        Mt::Blank,
        // bush right
        Mt::BushRight,
        Mt::Blank,
        Mt::Blank,
        // mountain left
        Mt::Blank,
        Mt::MountainLeft,
        Mt::MountainTwoEyes,
        // mountain middle
        Mt::MountainTop,
        Mt::MountainTwoEyes,
        Mt::MountainSolid,
        // mountain right
        Mt::Blank,
        Mt::MountainRight,
        Mt::MountainOneEye,
        // fence
        Mt::Fence,
        Mt::Blank,
        Mt::Blank,
        // tall tree
        Mt::TallTreetopTopHalf,
        Mt::TallTreetopBottomHalf,
        Mt::TreeTrunk,
        // short tree
        Mt::ShortTreetop,
        Mt::TreeTrunk,
        Mt::TreeTrunk,
    };

    const uint8_t BackSceneryData_data[] = {
        0x93, 0x00, 0x00, 0x11, 0x12, 0x12, 0x13, 0x00, // clouds
        0x00, 0x51, 0x52, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x91, 0x92, 0x93, 0x00, 0x00, 0x00, 0x00, 0x51, 0x52, 0x53, 0x41, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x91, 0x92,
        0x97, 0x87, 0x88, 0x89, 0x99, 0x00, 0x00, 0x00, // mountains and bushes
        0x11, 0x12, 0x13, 0xa4, 0xa5, 0xa5, 0xa5, 0xa6, 0x97, 0x98, 0x99, 0x01, 0x02, 0x03, 0x00, 0xa4, 0xa5, 0xa6, 0x00, 0x11,
        0x12, 0x12, 0x12, 0x13, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x02, 0x03, 0x00, 0xa4, 0xa5, 0xa5, 0xa6, 0x00, 0x00, 0x00,
        0x11, 0x12, 0x12, 0x13, 0x00, 0x00, 0x00, 0x00, // trees and fences
        0x00, 0x00, 0x00, 0x9c, 0x00, 0x8b, 0xaa, 0xaa, 0xaa, 0xaa, 0x11, 0x12, 0x13, 0x8b, 0x00, 0x9c, 0x9c, 0x00, 0x00, 0x01,
        0x02, 0x03, 0x11, 0x12, 0x12, 0x13, 0x00, 0x00, 0x00, 0x00, 0xaa, 0xaa, 0x9c, 0xaa, 0x00, 0x8b, 0x00, 0x01, 0x02, 0x03};

    const uint8_t BSceneDataOffsets_data[] = {0x00, 0x30, 0x60};

    // check to see if we are starting right of start
    if (M(BackloadingFlag) != 0)
    {                      // if not, go ahead and render background, foreground and terrain
        ProcessAreaData(); // otherwise skip ahead and load level data
    } // RenderSceneryTerrain
    // ClrMTBuf: clear out metatile buffer
    for (uint8_t col = 0x0c; (col & 0x80) == 0; --col)
    {
        writeData(MetatileBuffer + col, 0x00);
    }

    // do we need to render the background scenery? if not, skip to check the foreground
    uint8_t bgScenery = M(BackgroundScenery);
    if (bgScenery != 0)
    {
        // ThirdP: reduce current page to which of every third page we're on
        uint8_t page = M(CurrentPageLoc);
        while (page >= 3)
        {
            page -= 0x03;
            if ((page & 0x80) != 0)
            {
                break;
            }
        }
        // RendBack: move results to higher nybble, add scenery offset and current column position
        uint8_t index = page << 4;
        index += BSceneDataOffsets_data[bgScenery - 1];
        index += M(CurrentColumnPos);
        uint8_t sceneryByte = BackSceneryData_data[index]; // load data from sum of offsets
        if (sceneryByte != 0)                              // if zero, no scenery for that part
        {
            // low nybble is $01-$0c; subtract one, then multiply by three for the metatile data offset
            uint8_t low = (sceneryByte & 0x0f) - 0x01;
            writeData(0x00, low); // save low nybble
            uint8_t mtOffset = (low << 1) + M(0x00);
            uint8_t bufRow = sceneryByte >> 4; // high nybble: second offset, used to determine height
            writeData(0x00, 0x03);             // use saved memory location as counter

            do // SceLoop1: load metatile data from offset of (lsb - 1) * 3
            {
                writeData(MetatileBuffer + bufRow, static_cast<uint8_t>(BackSceneryMetatiles_data[mtOffset]));
                ++mtOffset;
                ++bufRow;
                if (bufRow == 0x0b)
                {
                    break;
                }
                --M(0x00); // decrement until counter expires, barring exception
            } while (M(0x00) != 0);
        }
    }

    // RendFore: check for foreground data needed or not
    uint8_t fgScenery = M(ForegroundScenery);
    if (fgScenery != 0)
    {
        // load offset from location offset by header value
        uint8_t src = FSceneDataOffsets_data[fgScenery - 1];
        for (uint8_t col = 0x00; col != 0x0d; ++col, ++src) // SceLoop2: load data until counter expires
        {
            uint8_t tile = ForeSceneryData_data[src];
            if (tile != 0) // do not store if zero found
            {
                writeData(MetatileBuffer + col, tile);
            }
        }
    }

    // RendTerr: pick the terrain metatile
    uint8_t areaType = M(AreaType);
    uint8_t terrainMetatile;
    if (areaType == 0 && M(WorldNumber) == World8)
    {
        // water level in world eight: use castle wall metatile as terrain type
        terrainMetatile = 0x62;
    }
    else
    {
        // TerMTile: otherwise get appropriate metatile for area type
        terrainMetatile = TerrainMetatiles_data[areaType];
        if (M(CloudTypeOverride) != 0)
        {
            terrainMetatile = 0x88; // use cloud block terrain
        }
    }
    StoreMT(terrainMetatile);
}

// store value here
// Inputs: terrainMetatile = terrain metatile number to store
// Outputs: none
void SMBEngine::StoreMT(uint8_t terrainMetatile)
{
    const uint8_t BlockBuffLowBounds_data[] = {0x10, 0x51, 0x88, 0xc0};

    const uint8_t TerrainRenderBits_data[] = {
        0b00000000, 0b00000000, // no ceiling or floor
        0b00000000, 0b00011000, // no ceiling, floor 2
        0b00000001, 0b00011000, // ceiling 1, floor 2
        0b00000111, 0b00011000, // ceiling 3, floor 2
        0b00001111, 0b00011000, // ceiling 4, floor 2
        0b11111111, 0b00011000, // ceiling 8, floor 2
        0b00000001, 0b00011111, // ceiling 1, floor 5
        0b00000111, 0b00011111, // ceiling 3, floor 5
        0b00001111, 0b00011111, // ceiling 4, floor 5
        0b10000001, 0b00011111, // ceiling 1, floor 6
        0b00000001, 0b00000000, // ceiling 1, no floor
        0b10001111, 0b00011111, // ceiling 4, floor 6
        0b11110001, 0b00011111, // ceiling 1, floor 9
        0b11111001, 0b00011000, // ceiling 1, middle 5, floor 2
        0b11110001, 0b00011000, // ceiling 1, middle 4, floor 2
        0b11111111, 0b00011111  // completely solid top to bottom
    };

    writeData(0x07, terrainMetatile);
    uint8_t col = 0x00;                             // metatile buffer offset
    uint8_t renderBitsIdx = M(TerrainControl) << 1; // header value * 2, offset into terrain rendering bits

    // The inner column counter runs continuously across successive render-bit bytes and exits the
    // moment it fills the whole buffer (col == 0x0d), so the outer loop never actually runs out of
    // bytes; TerrLoop just hands it the next render-bit byte each time it needs one.
    bool done = false;
    while (!done)
    {
        // TerrLoop: get one of the terrain rendering bit data
        writeData(0x00, TerrainRenderBits_data[renderBitsIdx]);
        ++renderBitsIdx; // increment and use as offset next time around
        writeData(0x01, renderBitsIdx);
        // in cloud levels, mask out all but d3 (but never for the very first render-bit byte)
        if (M(CloudTypeOverride) != 0 && col != 0x00)
        {
            writeData(0x00, M(0x00) & 0b00001000);
        }

        // TerrBChk: for each of the eight bitmasks
        for (uint8_t bit = 0x00; bit != 0x08; ++bit)
        {
            // AND bitmask with the render bits; if set, write terrain metatile into the buffer here
            if ((M(Bitmasks + bit) & M(0x00)) != 0)
            {
                writeData(MetatileBuffer + col, M(0x07));
            } // NextTBit: continue until end of buffer
            ++col;
            if (col == 0x0d)
            { // reached the end of the buffer
                done = true;
                break;
            }
            // underground override: force ground-level terrain type at the bottom of the screen
            if (M(AreaType) == 0x02 && col == 0x0b)
            {
                writeData(0x07, 0x54);
            }
        }
    }

    // RendBBuf: do the area data loading routine now
    ProcessAreaData();
    GetBlockBufferAddr(M(BlockBufferColumnPos)); // get block buffer address from where we're at
    uint8_t bufOffset = 0x00;                    // start at beginning of smaller buffer
    for (uint8_t row = 0x00; row < 0x0d; ++row)  // ChkMTLow: continue until we pass last row
    {
        writeData(0x00, bufOffset);
        // load stored metatile number, mask out all but 2 MSB, %xx000000 -> %000000xx as offset
        uint8_t palette = (M(MetatileBuffer + row) & 0b11000000) >> 6;
        uint8_t value = M(MetatileBuffer + row); // reload original unmasked value here
        if (value < BlockBuffLowBounds_data[palette])
        {
            value = 0x00; // if less, init value before storing
        } // StrBlock: get offset for block buffer
        writeData(W(0x06) + M(0x00), value); // store value into block buffer
        bufOffset = M(0x00) + 0x10;
    }
}

// Inputs: none
// Outputs: none
void SMBEngine::ProcessAreaData()
{
    do // ProcessAreaDataStart: reprocess to load more level data while objects remain behind the
    { // renderer, or while starting right of page $00
        uint8_t objectOffset = 0x02; // start at the end of area object buffer
        do // ProcADLoop
        {
            writeData(ObjectOffset, objectOffset);
            writeData(BehindAreaParserFlag, 0x00); // reset flag
            uint8_t dataOff = M(AreaDataOffset);   // get offset of area data pointer

            bool decode = false;
            // get first byte of area object; decode straight away at end of data or if buffer flag clear
            if (M(W(AreaData) + dataOff) == 0xfd || (M(AreaObjectLength + objectOffset) & 0x80) == 0)
            {
                decode = true; // RdyDecode
            }
            else
            {
                // check for page select bit (d7) of second byte, and set page if not already set
                if ((M(W(AreaData) + dataOff + 1) & 0x80) != 0 && M(AreaObjectPageSel) == 0)
                {
                    ++M(AreaObjectPageSel); // if not already set, set it now
                    ++M(AreaObjectPageLoc); // and increment page location
                }
                // Chk1Row13: reread first byte, mask out high nybble
                uint8_t row = M(W(AreaData) + dataOff) & 0x0f;
                bool checkRear = false;
                if (row == 0x0d)
                {
                    // check d6 of second byte (page control obj bit)
                    if ((M(W(AreaData) + dataOff + 1) & 0b01000000) != 0)
                    {
                        checkRear = true;
                    }
                    else if (M(AreaObjectPageSel) != 0)
                    {
                        checkRear = true; // if page select is set, do not reread
                    }
                    else
                    {
                        // store 5 LSB of second byte as page control
                        writeData(AreaObjectPageLoc, M(W(AreaData) + dataOff + 1) & 0b00011111);
                        ++M(AreaObjectPageSel); // increment page select
                        // -> NextAObj
                    }
                }
                else if (row == 0x0e) // Chk1Row14: row 14?
                {
                    // render the object if backloading (otherwise bg might not look right)
                    if (M(BackloadingFlag) != 0)
                    {
                        decode = true; // RdyDecode
                    }
                    else
                    {
                        checkRear = true;
                    }
                }
                else
                {
                    checkRear = true;
                }

                if (checkRear)
                {
                    // CheckRear: is the object's page at or past the renderer's?
                    if (M(AreaObjectPageLoc) >= M(CurrentPageLoc))
                    {
                        decode = true; // RdyDecode
                    }
                    else
                    {
                        ++M(BehindAreaParserFlag); // SetBehind: object behind renderer
                    }
                }
            }

            if (decode)
            {
                DecodeAreaData(objectOffset, dataOff); // do sub and do not turn on flag
            }
            else
            {
                IncAreaObjOffset(); // NextAObj: increment buffer offset and move on
            }

            // ChkLength: get buffer offset (may have been reset to 0 while decoding)
            objectOffset = M(ObjectOffset);
            if ((M(AreaObjectLength + objectOffset) & 0x80) == 0)
            {
                --M(AreaObjectLength + objectOffset); // decrement length or get rid of it
            }
            --objectOffset;                       // ProcLoopb: decrement buffer offset
        } while ((objectOffset & 0x80) == 0); // and loopback unless exceeded buffer
    } while (M(BehindAreaParserFlag) != 0 || M(BackloadingFlag) != 0);
}

// Inputs: areaObjBufferOffset = area object buffer offset; areaDataOffset = AreaData offset (only
// consulted when the buffer's length flag is already set)
// Outputs: none
void SMBEngine::DecodeAreaData(uint8_t areaObjBufferOffset, uint8_t areaDataOffset)
{
    uint8_t dataOff = areaDataOffset;
    // check current buffer flag; if length flag clear, get offset from buffer instead
    if ((M(AreaObjectLength + areaObjBufferOffset) & 0x80) == 0)
    {
        dataOff = M(AreaObjOffsetBuffer + areaObjBufferOffset);
    }
    // Chk1stB: get first byte of level object again
    uint8_t firstByte = M(W(AreaData) + dataOff);
    if (firstByte == 0xfd)
    {
        return; // if end of level, leave this routine
    }
    uint8_t row = firstByte & 0x0f; // mask out low nybble (the *y value)
    // dispatch offset: 16 for special row 15, 8 for special row 12, otherwise 0
    uint8_t dispatchOffset;
    if (row == 0x0f)
    {
        dispatchOffset = 0x10;
    }
    else if (row == 0x0c)
    {
        dispatchOffset = 0x08;
    }
    else
    {
        dispatchOffset = 0x00;
    }
    // ChkRow14: store whatever value we just loaded here
    writeData(0x07, dispatchOffset);

    if (row == 0x0e)
    {
        writeData(0x07, 0x00);              // load offset with $00
        NormObj(0x2e, areaObjBufferOffset); // and load object id with another value
        return;
    }
    if (row == 0x0d) // ChkRow13: row 13?
    {
        writeData(0x07, 0x22); // load offset with 34
        ++dataOff;             // get next byte
        // mask out all but d6 (page control obj bit); if clear, leave (we handled this earlier)
        if ((M(W(AreaData) + dataOff) & 0b01000000) == 0)
        {
            return;
        }
        uint8_t objId = M(W(AreaData) + dataOff) & 0b01111111; // get byte again, mask out d7
        if (objId == 0x4b)
        {                     // (plus d6 set for object other than page control)
            ++M(LoopCommand); // if loop command, set loop command flag
        }
        NormObj(objId & 0b00111111, areaObjBufferOffset); // Mask2MSB: mask out d7 and d6, and jump
        return;
    }

    // ChkSRows: row 12-15?
    uint8_t objId;
    if (row < 0x0c)
    {
        ++dataOff;                                             // get second byte of level object
        uint8_t sizeBits = M(W(AreaData) + dataOff) & 0b01110000; // mask out all but d6-d4
        if (sizeBits == 0)
        {                          // if any bits set, branch to handle large object
            writeData(0x07, 0x16); // otherwise set offset of 0x16 for small object
            // use low nybble of second byte as object id
            NormObj(M(W(AreaData) + dataOff) & 0b00001111, areaObjBufferOffset);
            return;
        }
        // LrgObj: store value here (branch for large objects)
        writeData(0x00, sizeBits);
        // warp pipe: d3 (usage control bit) of second byte set -> nullify value
        if (sizeBits == 0x70 && (M(W(AreaData) + dataOff) & 0b00001000) != 0)
        {
            writeData(0x00, 0x00);
        }
        objId = M(0x00); // NotWPipe: get value and jump ahead
    }
    else
    {
        // SpecObj: branch here for rows 12-15
        ++dataOff;
        objId = M(W(AreaData) + dataOff) & 0b01110000; // get next byte and mask out all but d6-d4
    }
    objId >>= 4; // MoveAOId: move d6-d4 to lower nybble
    NormObj(objId, areaObjBufferOffset);
}

// store value here (branch for small objects and rows 13 and 14)
// Inputs: objectId = object type id/offset; areaObjBufferOffset = area object buffer offset (threaded
// through to whichever renderer is dispatched below); 0x07 = dispatch offset (e.g. 0x16 for small objects)
// Outputs: none
void SMBEngine::NormObj(uint8_t objectId, uint8_t areaObjBufferOffset)
{
    writeData(0x00, objectId);
    // is there something stored here already?
    if ((M(AreaObjectLength + areaObjBufferOffset) & 0x80) != 0)
    { // if so, branch to do its particular sub
        bool storeObj = false;
        // check to see if the object we've loaded is on the current page
        if (M(AreaObjectPageLoc) != M(CurrentPageLoc))
        {
            // reload first byte using old offset of level pointer
            if ((M(W(AreaData) + M(AreaDataOffset)) & 0b00001111) != 0x0e)
            {
                return;
            }
            if (M(BackloadingFlag) == 0)
            {
                return; // LeavePar: only render (StrAObj) when backloading
            }
            storeObj = true;
        }
        else if (M(BackloadingFlag) != 0)
        {
            // InitRear: not yet initialized -- clear backloading and behind-renderer flags and leave
            writeData(BackloadingFlag, 0x00);
            writeData(BehindAreaParserFlag, 0x00);
            writeData(ObjectOffset, 0x00);
            return; // LoopCmdE
        }
        else
        {
            // BackColC: get first byte again, move high nybble to low, compare to current column
            if ((M(W(AreaData) + M(AreaDataOffset)) >> 4) != M(CurrentColumnPos))
            {
                return;
            }
            storeObj = true;
        }
        if (storeObj)
        {
            // StrAObj: load area obj offset and store in buffer
            writeData(AreaObjOffsetBuffer + areaObjBufferOffset, M(AreaDataOffset));
            IncAreaObjOffset(); // do sub to increment to next object data
        }
    }
    // RunAObj: get stored value and add offset to it
    switch (static_cast<uint8_t>(M(0x00) + M(0x07)))
    {
    case 0:
        VerticalPipe(areaObjBufferOffset); // used by warp pipes
        return;
    case 1:
        AreaStyleObject(areaObjBufferOffset);
        return;
    case 2:
        RowOfBricks(areaObjBufferOffset);
        return;
    case 3:
        RowOfSolidBlocks(areaObjBufferOffset);
        return;
    case 4:
        RowOfCoins(areaObjBufferOffset);
        return;
    case 5:
        ColumnOfBricks(areaObjBufferOffset);
        return;
    case 6:
        ColumnOfSolidBlocks(areaObjBufferOffset);
        return;
    case 7:
        VerticalPipe(areaObjBufferOffset); // used by decoration pipes
        return;
    // y=12 special objects
    case 8:
        Hole_Empty(areaObjBufferOffset);
        return;
    case 9:
        PulleyRopeObject(areaObjBufferOffset);
        return;
    case 10:
        Bridge_High(areaObjBufferOffset);
        return;
    case 11:
        Bridge_Middle(areaObjBufferOffset);
        return;
    case 12:
        Bridge_Low(areaObjBufferOffset);
        return;
    case 13:
        Hole_Water(areaObjBufferOffset);
        return;
    case 14:
        QuestionBlockRow_High(areaObjBufferOffset);
        return;
    case 15:
        QuestionBlockRow_Low(areaObjBufferOffset);
        return;
    // y=15 special objects
    case 16:
        EndlessRope();
        return;
    case 17:
        BalancePlatRope(areaObjBufferOffset);
        return;
    case 18:
        CastleObject(areaObjBufferOffset);
        return;
    case 19:
        StaircaseObject(areaObjBufferOffset);
        return;
    case 20:
        ExitPipe(areaObjBufferOffset);
        return;
    case 21:
        FlagBalls_Residual(areaObjBufferOffset);
        return;
    case 22:
        QuestionBlock(areaObjBufferOffset); // power-up
        return;
    case 23:
        QuestionBlock(areaObjBufferOffset); // coin
        return;
    // SMALL OBJECTS (offset by 24):
    case 24:
        QuestionBlock(areaObjBufferOffset); // hidden, coin
        return;
    case 25:
        Hidden1UpBlock(areaObjBufferOffset); // hidden, 1-up
        return;
    case 26:
        BrickWithItem(areaObjBufferOffset); // brick, power-up
        return;
    case 27:
        BrickWithItem(areaObjBufferOffset); // brick, vine
        return;
    case 28:
        BrickWithItem(areaObjBufferOffset); // brick, star
        return;
    case 29:
        BrickWithCoins(areaObjBufferOffset); // brick, coins
        return;
    case 30:
        BrickWithItem(areaObjBufferOffset); // brick, 1-up
        return;
    case 31:
        WaterPipe(areaObjBufferOffset);
        return;
    case 32:
        EmptyBlock(areaObjBufferOffset);
        return;
    case 33:
        Jumpspring(areaObjBufferOffset);
        return;
    case 34:
        IntroPipe(areaObjBufferOffset);
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
        CastleBridgeObj(areaObjBufferOffset);
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
        AlterAreaAttributes(areaObjBufferOffset);
        return;
    default:
        bad_jump();
        return;
    }
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::Hidden1UpBlock(uint8_t areaObjBufferOffset)
{
    if (M(Hidden1UpFlag) == 0)
    {
        return; // if flag not set, do not render object
    }
    writeData(Hidden1UpFlag, 0x00);       // if set, init for the next one
    BrickWithItem(areaObjBufferOffset); // jump to code shared with unbreakable bricks
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::QuestionBlock(uint8_t areaObjBufferOffset)
{
    uint8_t objectId = GetAreaObjectID();   // get value from level decoder routine
    DrawQBlk(objectId, areaObjBufferOffset); // go to render it
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::BrickWithCoins(uint8_t areaObjBufferOffset)
{
    writeData(BrickCoinTimerFlag, 0x00); // initialize multi-coin timer flag
    BrickWithItem(areaObjBufferOffset);
}

// Inputs: areaObjBufferOffset = area object buffer offset
// Outputs: none
void SMBEngine::BrickWithItem(uint8_t areaObjBufferOffset)
{
    uint8_t objectId = GetAreaObjectID(); // save area object ID
    writeData(0x07, objectId);
    // ground level uses the adder for bricks with lines (0), other types the adder for bricks without lines (5)
    uint8_t adder = (M(AreaType) == 0x01) ? 0x00 : 0x05;
    uint8_t metatileIndex = adder + M(0x07); // BWithL: add object ID to adder, use as offset for metatile
    DrawQBlk(metatileIndex, areaObjBufferOffset);
}

// Inputs: none (reads memory 0x00)
// Outputs: returns the object id/offset saved by the area parser routine
uint8_t SMBEngine::GetAreaObjectID()
{
    return M(0x00); // get value saved from area parser routine (the "- 0" was residual code)
}
