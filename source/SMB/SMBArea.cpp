// The AreaParserTaskHandler subsystem: everything AreaParserTaskHandler() reaches that nothing
// outside it reaches, and so nothing outside it needs.
//
// Moved out of SMB.cpp by tools/split.py. These are methods of SMBEngine like every other
// routine of the game and are declared in SMBEngine.hpp; the file they are written in is the
// only thing that is different, and tools/layers.py is what keeps it meaning something.
//
#include <algorithm>

#include "SMB.hpp"

void SMBEngine::IncAreaObjOffset()
{
    ++M(AreaDataOffset); // increment offset of level pointer
    ++M(AreaDataOffset);
    a = 0x00; // reset page select
    writeData(AreaObjectPageSel, 0x00);
}

bool SMBEngine::FindEmptyEnemySlot()
{
    bool allEnemySlotsFull = false;

    x = 0x00; // start at first enemy slot

EmptyChkLoop: // assume a slot is free by default
    allEnemySlotsFull = false;
    a = M(Enemy_Flag + x); // check enemy buffer for nonzero
    if (a != 0)
    { // if zero, leave
        ++x;
        if (x != 0x05)
        {
            goto EmptyChkLoop;
        }
        allEnemySlotsFull = true; // ExitEmptyChk: all values nonzero
    }
    return allEnemySlotsFull;
}

void SMBEngine::GetLrgObjAttrib()
{
    y = M(AreaObjOffsetBuffer + x); // get offset saved from area obj decoding routine
    // get first byte of level object
    a = M(W(AreaData) + y) & 0b00001111;
    writeData(0x07, a); // save row location
    ++y;
    // get next byte, save lower nybble (length or height)
    a = M(W(AreaData) + y) & 0b00001111; // as Y, then leave
    y = a;
}

void SMBEngine::GetAreaObjXPosition()
{
    a = M(CurrentColumnPos); // multiply current offset where we're at by 16
    a <<= 1;                 // to obtain horizontal pixel coordinate
    a <<= 1;
    a <<= 1;
    a <<= 1;
}

void SMBEngine::GetAreaObjYPosition()
{
    a = M(0x07); // multiply value by 16
    a <<= 1;
    a <<= 1; // this will give us the proper vertical pixel coordinate
    a <<= 1;
    a <<= 1;
    a += 32; // add 32 pixels for the status bar
}

// use area object identifier bit as offset
void SMBEngine::AreaFrenzy()
{
    const uint8_t FrenzyIDData_data[] = {FlyCheepCheepFrenzy, BBill_CCheep_Frenzy, Stop_Frenzy};

    x = M(0x00);
    a = FrenzyIDData_data[x - 8]; // note that it starts at 8, thus weird address here
    y = 0x05;

FreCompLoop: // check regular slots of enemy object buffer
    --y;
    if ((y & 0x80) == 0)
    { // if all slots checked and enemy object not found, branch to store
        if (a != M(Enemy_ID + y))
        {
            goto FreCompLoop;
        }
        a = 0x00; // if enemy object already present, nullify queue and leave
    } // ExitAFrenzy: store enemy into frenzy queue
    writeData(EnemyFrenzyQueue, a);
}

void SMBEngine::WaterPipe()
{
    GetLrgObjAttrib();                   // get row and lower nybble
    y = M(AreaObjectLength + x);         // get length (residual code, water pipe is 1 col thick)
    x = M(0x07);                         // get row
    writeData(MetatileBuffer + x, 0x6b); // draw something here and below it
    a = 0x6c;
    writeData(MetatileBuffer + 1 + x, 0x6c);
}

void SMBEngine::Jumpspring()
{
    bool allEnemySlotsFull = false;

    GetLrgObjAttrib();
    allEnemySlotsFull = FindEmptyEnemySlot(); // find empty space in enemy object buffer
    GetAreaObjXPosition();                    // get horizontal coordinate for jumpspring
    writeData(Enemy_X_Position + x, a);       // and store
    // store page location of jumpspring
    writeData(Enemy_PageLoc + x, M(CurrentPageLoc));
    GetAreaObjYPosition();                     // get vertical coordinate for jumpspring
    writeData(Enemy_Y_Position + x, a);        // and store
    writeData(Jumpspring_FixedYPos + x, a);    // store as permanent coordinate here
    writeData(Enemy_ID + x, JumpspringObject); // write jumpspring object to enemy object buffer
    y = 0x01;
    writeData(Enemy_Y_HighPos + x, 0x01); // store vertical high byte
    ++M(Enemy_Flag + x);                  // set flag for enemy object buffer
    x = M(0x07);
    // draw metatiles in two rows where jumpspring is
    writeData(MetatileBuffer + x, 0x67);
    a = 0x68;
    writeData(MetatileBuffer + 1 + x, 0x68);
}

void SMBEngine::CastleObject()
{
    const uint8_t CastleMetatiles_data[] = {0x00, 0x45, 0x45, 0x45, 0x00, 0x00, 0x48, 0x47, 0x46, 0x00, 0x45, 0x49, 0x49, 0x49,
                                            0x45, 0x47, 0x47, 0x4a, 0x47, 0x47, 0x47, 0x47, 0x4b, 0x47, 0x47, 0x49, 0x49, 0x49,
                                            0x49, 0x49, 0x47, 0x4a, 0x47, 0x4a, 0x47, 0x47, 0x4b, 0x47, 0x4b, 0x47, 0x47, 0x47,
                                            0x47, 0x47, 0x47, 0x4a, 0x47, 0x4a, 0x47, 0x4a, 0x4b, 0x47, 0x4b, 0x47, 0x4b};

    bool allEnemySlotsFull = false;
    bool lrgObjJustStarted = false;

    GetLrgObjAttrib();  // save lower nybble as starting row
    writeData(0x07, y); // if starting row is above $0a, game will crash!!!
    y = 0x04;
    lrgObjJustStarted = ChkLrgObjFixedLength(); // load length of castle if not already loaded
    a = x;
    pha();                       // save obj buffer offset to stack
    y = M(AreaObjectLength + x); // use current length as offset for castle data
    x = M(0x07);                 // begin at starting row
    a = 0x0b;
    writeData(0x06, 0x0b); // load upper limit of number of rows to print

    do // CRendLoop: load current byte using offset
    {
        writeData(MetatileBuffer + x, CastleMetatiles_data[y]);
        ++x; // store in buffer and increment buffer offset
        if (M(0x06) != 0)
        {        // have we reached upper limit yet?
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
    {
        return; // if we're at page 0, we do not need to do anything else
    }
    a = M(AreaObjectLength + x); // check length
    if (a == 0x01)
    {
        goto PlayerStop;
    }
    y = M(0x07); // check starting row for tall castle ($00)
    if (y == 0)
    {
        if (a == 0x03)
        {
            goto PlayerStop;
        }
    } // NotTall: if not tall castle, check to see if we're at the third column
    if (a != 0x02)
    {
        return; // if we aren't and the castle is tall, don't create flag yet
    }
    GetAreaObjXPosition(); // otherwise, obtain and save horizontal pixel coordinate
    pha();
    allEnemySlotsFull = FindEmptyEnemySlot(); // find an empty place on the enemy object buffer
    pla();
    writeData(Enemy_X_Position + x, a);              // then write horizontal coordinate for star flag
    writeData(Enemy_PageLoc + x, M(CurrentPageLoc)); // set page location for star flag
    writeData(Enemy_Y_HighPos + x, 0x01);            // set vertical high byte
    writeData(Enemy_Flag + x, 0x01);                 // set flag for buffer
    writeData(Enemy_Y_Position + x, 0x90);           // set vertical coordinate
    a = StarFlagObject;                              // set star flag value in buffer itself
    writeData(Enemy_ID + x, StarFlagObject);
    return;

PlayerStop: // put brick at floor to stop player at end of level
    y = 0x52;
    writeData(MetatileBuffer + 10, 0x52); // this is only done if we're on the second column

    // ExitCastle
}

void SMBEngine::GetPipeHeight()
{
    bool lrgObjJustStarted = false;

    y = 0x01;                                   // check for length loaded, if not, load
    lrgObjJustStarted = ChkLrgObjFixedLength(); // pipe length of 2 (horizontal)
    GetLrgObjAttrib();
    a = y;                       // get saved lower nybble as height
    a &= 0x07;                   // save only the three lower bits as
    writeData(0x06, a);          // vertical length, then load Y with
    y = M(AreaObjectLength + x); // length left over
}

bool SMBEngine::ChkLrgObjLength()
{
    bool lrgObjJustStarted = false;

    GetLrgObjAttrib(); // get row location and size (length if branched to from here)
    lrgObjJustStarted = ChkLrgObjFixedLength();
    return lrgObjJustStarted;
}

bool SMBEngine::ChkLrgObjFixedLength()
{
    bool lrgObjJustStarted = false;

    a = M(AreaObjectLength + x); // check for set length counter
    lrgObjJustStarted = false;   // not just starting
    if ((a & 0x80) != 0)
    {          // if counter not set, load it, otherwise leave alone
        a = y; // save length into length counter
        writeData(AreaObjectLength + x, a);
        lrgObjJustStarted = true; // just starting
    } // LenSet
    return lrgObjJustStarted;
}

void SMBEngine::Hole_Water()
{
    bool lrgObjJustStarted = false;

    lrgObjJustStarted = ChkLrgObjLength(); // get low nybble and save as length
    // render waves
    writeData(MetatileBuffer + 10, 0x86);
    x = 0x0b;
    y = 0x01; // now render the water underneath
    a = 0x87;
    RenderUnderPart();
}

void SMBEngine::FlagBalls_Residual()
{
    GetLrgObjAttrib(); // get low nybble from object byte
    x = 0x02;          // render flag balls on third row from top
    a = 0x6d;          // of screen downwards based on low nybble
    RenderUnderPart();
}

void SMBEngine::FlagpoleObject()
{
    uint32_t wide = 0;

    // render flagpole ball on top
    writeData(MetatileBuffer, 0x24);
    x = 0x01; // now render the flagpole shaft
    y = 0x08;
    a = 0x25;
    RenderUnderPart();
    a = 0x61; // render solid block at the bottom
    writeData(MetatileBuffer + 10, 0x61);
    GetAreaObjXPosition();
    wide = ((M(CurrentPageLoc) << 8) | a) - 0x08;  // subtract eight pixels and use as horizontal
    writeData(Enemy_X_Position + 5, LOBYTE(wide)); // coordinate for the flag
    writeData(Enemy_PageLoc + 5, HIBYTE(wide));    // page location for the flag
    a = HIBYTE(wide);
    writeData(Enemy_Y_Position + 5, 0x30); // set vertical coordinate for flag
    writeData(FlagpoleFNum_Y_Pos, 0xb0);   // set initial vertical coordinate for flagpole's floatey number
    a = FlagpoleFlagObject;
    writeData(Enemy_ID + 5, FlagpoleFlagObject); // set flag identifier, note that identifier and coordinates
    ++M(Enemy_Flag + 5);                         // use last space in enemy object buffer
}

void SMBEngine::RowOfCoins()
{
    const uint8_t CoinMetatileData_data[] = {0xc3, 0xc2, 0xc2, 0xc2};

    y = M(AreaType);              // get area type
    a = CoinMetatileData_data[y]; // load appropriate coin metatile
    GetRow();
}

void SMBEngine::EmptyBlock()
{
    GetLrgObjAttrib(); // get row location
    x = M(0x07);
    a = 0xc4;
    ColObj();
}

// column length of 1
void SMBEngine::ColObj()
{
    y = 0x00;
    RenderUnderPart();
}

void SMBEngine::RowOfBricks()
{
    y = M(AreaType); // load area type obtained from area offset pointer
    // check for cloud type override
    if (M(CloudTypeOverride) != 0)
    {
        y = 0x04; // if cloud type, override area type
    } // DrawBricks: get appropriate metatile
    a = M(BrickMetatiles + y);
    GetRow(); // and go render it
}

void SMBEngine::RowOfSolidBlocks()
{
    y = M(AreaType);                // load area type obtained from area offset pointer
    a = M(SolidBlockMetatiles + y); // get metatile
    GetRow();
}

// store metatile here
void SMBEngine::GetRow()
{
    bool lrgObjJustStarted = false;

    pha();
    lrgObjJustStarted = ChkLrgObjLength(); // get row number, load length
    DrawRow();
}

void SMBEngine::DrawRow()
{
    x = M(0x07);
    y = 0x00; // set vertical height of 1
    pla();
    RenderUnderPart(); // render object
}

void SMBEngine::ColumnOfBricks()
{
    y = M(AreaType);           // load area type obtained from area offset
    a = M(BrickMetatiles + y); // get metatile (no cloud override as for row)
    GetRow2();
}

void SMBEngine::ColumnOfSolidBlocks()
{
    y = M(AreaType);                // load area type obtained from area offset
    a = M(SolidBlockMetatiles + y); // get metatile
    GetRow2();
}

// save metatile to stack for now
void SMBEngine::GetRow2()
{
    pha();
    GetLrgObjAttrib(); // get length and row
    pla();             // restore metatile
    x = M(0x07);       // get starting row
    RenderUnderPart(); // now render the column
}

void SMBEngine::BulletBillCannon()
{
    GetLrgObjAttrib(); // get row and length of bullet bill cannon
    x = M(0x07);       // start at first row
    a = 0x64;          // render bullet bill cannon
    writeData(MetatileBuffer + x, 0x64);
    ++x;
    --y; // done yet?
    if ((y & 0x80) != 0)
    {
        goto SetupCannon;
    }
    a = 0x65; // if not, render middle part
    writeData(MetatileBuffer + x, 0x65);
    ++x;
    --y; // done yet?
    if ((y & 0x80) != 0)
    {
        goto SetupCannon;
    }
    a = 0x66; // if not, render bottom until length expires
    RenderUnderPart();

SetupCannon: // get offset for data used by cannons and whirlpools
    x = M(Cannon_Offset);
    GetAreaObjYPosition();                            // get proper vertical coordinate for cannon
    writeData(Cannon_Y_Position + x, a);              // and store it here
    writeData(Cannon_PageLoc + x, M(CurrentPageLoc)); // store page number for cannon here
    GetAreaObjXPosition();                            // get proper horizontal coordinate for cannon
    writeData(Cannon_X_Position + x, a);              // and store it here
    ++x;
    if (x >= 0x06)
    {             // if not yet reached sixth cannon, branch to save offset
        x = 0x00; // otherwise initialize it
    } // StrCOffset: save new offset and leave
    writeData(Cannon_Offset, x);
}

void SMBEngine::StaircaseObject()
{
    const uint8_t StaircaseRowData_data[] = {0x03, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a};

    const uint8_t StaircaseHeightData_data[] = {0x07, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00};

    bool lrgObjJustStarted = false;

    lrgObjJustStarted = ChkLrgObjLength(); // check and load length
    if (lrgObjJustStarted)
    {                                      // if length already loaded, skip init part
        a = 0x09;                          // start past the end for the bottom
        writeData(StaircaseControl, 0x09); // of the staircase
    } // NextStair: move onto next step (or first if starting)
    --M(StaircaseControl);
    y = M(StaircaseControl);
    x = StaircaseRowData_data[y]; // get starting row and height to render
    y = StaircaseHeightData_data[y];
    a = 0x61; // now render solid block staircase
    RenderUnderPart();
}

void SMBEngine::Hole_Empty()
{
    const uint8_t HoleMetatiles_data[] = {0x87, 0x00, 0x00, 0x00};

    uint32_t wide = 0;
    bool lrgObjJustStarted = false;

    lrgObjJustStarted = ChkLrgObjLength(); // get lower nybble and save as length
    if (!lrgObjJustStarted)
    {
        goto NoWhirlP; // skip this part if length already loaded
    }
    // check for water type level
    if (M(AreaType) != 0)
    {
        goto NoWhirlP; // if not water type, skip this part
    }
    x = M(Whirlpool_Offset);                           // get offset for data used by cannons and whirlpools
    GetAreaObjXPosition();                             // get proper vertical coordinate of where we're at
    wide = ((M(CurrentPageLoc) << 8) | a) - 0x10;      // subtract 16 pixels
    writeData(Whirlpool_LeftExtent + x, LOBYTE(wide)); // store as left extent of whirlpool
    writeData(Whirlpool_PageLoc + x, HIBYTE(wide));    // save as page location of whirlpool
    a = HIBYTE(wide);                                  // get page location of where we're at
    ++y;
    ++y; // increment length by 2
    a = y;
    a <<= 1;                            // multiply by 16 to get size of whirlpool
    a <<= 1;                            // note that whirlpool will always be
    a <<= 1;                            // two blocks bigger than actual size of hole
    a <<= 1;                            // and extend one block beyond each edge
    writeData(Whirlpool_Length + x, a); // save size of whirlpool here
    ++x;
    if (x >= 0x05)
    {             // if not yet reached fifth whirlpool, branch to save offset
        x = 0x00; // otherwise initialize it
    } // StrWOffset: save new offset here
    writeData(Whirlpool_Offset, x);
    goto NoWhirlP;

NoWhirlP: // get appropriate metatile, then
    x = M(AreaType);
    a = HoleMetatiles_data[x]; // render the hole proper
    x = 0x08;
    y = 0x0f; // start at ninth row and go to bottom, run RenderUnderPart
    RenderUnderPart();
}

void SMBEngine::RenderUnderPart()
{
RenderUnderPart:
    writeData(AreaObjectHeight, y); // store vertical length to render
    y = M(MetatileBuffer + x);      // check current spot to see if there's something
    if (y == 0)
    {
        goto DrawThisRow; // we need to keep, if nothing, go ahead
    }
    if (y == 0x17)
    {
        goto WaitOneRow; // if middle part (tree ledge), wait until next row
    }
    if (y == 0x1a)
    {
        goto WaitOneRow; // if middle part (mushroom ledge), wait until next row
    }
    if (y == 0xc0)
    {
        goto DrawThisRow; // if question block w/ coin, overwrite
    }
    if (y >= 0xc0)
    {
        goto WaitOneRow; // if any other metatile with palette 3, wait until next row
    }
    if (y != 0x54)
    {
        goto DrawThisRow; // if cracked rock terrain, overwrite
    }
    if (a == 0x50)
    {
        goto WaitOneRow; // if stem top of mushroom, wait until next row
    }
    goto DrawThisRow;

DrawThisRow: // render contents of A from routine that called this
    writeData(MetatileBuffer + x, a);
    goto WaitOneRow;

WaitOneRow:
    ++x;
    if (x < 0x0d)
    {
        y = M(AreaObjectHeight); // decrement, and stop rendering if there is no more length
        --y;
        if ((y & 0x80) == 0)
        {
            goto RenderUnderPart;
        }
    } // ExitUPartR
}

void SMBEngine::AreaStyleObject()
{
    bool lrgObjJustStarted = false;

    // load level object style and jump to the right sub
    switch (M(AreaStyle))
    {
    case 0:
        goto TreeLedge; // also used for cloud type levels
    case 1:
        goto MushroomLedge;
    case 2:
        BulletBillCannon();
        return;
    default:
        bad_jump();
        return;
    }

TreeLedge:
    GetLrgObjAttrib();           // get row and length of green ledge
    a = M(AreaObjectLength + x); // check length counter for expiration
    if (a != 0)
    {
        if ((a & 0x80) == 0)
        {
            goto MidTreeL;
        }
        a = y;
        writeData(AreaObjectLength + x, a);          // store lower nybble into buffer flag as length of ledge
        a = M(CurrentPageLoc) | M(CurrentColumnPos); // are we at the start of the level?
        if (a == 0)
        {
            goto MidTreeL;
        }
        a = 0x16; // render start of tree ledge
        goto NoUnder;

    MidTreeL:
        x = M(0x07);
        // render middle of tree ledge
        writeData(MetatileBuffer + x, 0x17); // note that this is also used if ledge position is
        a = 0x4c;                            // at the start of level for continuous effect
        goto AllUnder;                       // now render the part underneath
    } // EndTreeL: render end of tree ledge
    a = 0x18;
    goto NoUnder;

MushroomLedge:
    lrgObjJustStarted = ChkLrgObjLength(); // get shroom dimensions
    writeData(0x06, y);                    // store length here for now
    if (lrgObjJustStarted)
    {
        // divide length by 2 and store elsewhere
        a = M(AreaObjectLength + x) >> 1;
        writeData(MushroomLedgeHalfLen + x, a);
        a = 0x19; // render start of mushroom
        goto NoUnder;
    } // EndMushL: if at the end, render end of mushroom
    a = 0x1b;
    y = M(AreaObjectLength + x);
    if (y == 0)
    {
        goto NoUnder;
    }
    // get divided length and store where length
    writeData(0x06, M(MushroomLedgeHalfLen + x)); // was stored originally
    x = M(0x07);
    a = 0x1a;
    writeData(MetatileBuffer + x, 0x1a); // render middle of mushroom
    if (y != M(0x06))
    {
        return; // are we smack dab in the center? if not, branch to leave
    }
    ++x;
    writeData(MetatileBuffer + x, 0x4f); // render stem top of mushroom underneath the middle
    a = 0x50;
    goto AllUnder;

AllUnder:
    ++x;
    y = 0x0f;          // set $0f to render all way down
    RenderUnderPart(); // now render the stem of mushroom
    return;

NoUnder: // load row of ledge
    x = M(0x07);
    y = 0x00; // set 0 for no bottom on this part
    RenderUnderPart();
}

void SMBEngine::PulleyRopeObject()
{
    const uint8_t PulleyRopeMetatiles_data[] = {0x42, 0x41, 0x43};

    bool lrgObjJustStarted = false;

    lrgObjJustStarted = ChkLrgObjLength(); // get length of pulley/rope object
    y = 0x00;                              // initialize metatile offset
    if (lrgObjJustStarted)
    {
        goto RenderPul; // if starting, render left pulley
    }
    y = 0x01;
    // if not at the end, render rope
    if (M(AreaObjectLength + x) != 0)
    {
        goto RenderPul;
    }
    y = 0x02; // otherwise render right pulley
    goto RenderPul;

RenderPul:
    a = PulleyRopeMetatiles_data[y];
    writeData(MetatileBuffer, a); // render at the top of the screen
}

void SMBEngine::VerticalPipe()
{
    uint32_t wide = 0;
    bool allEnemySlotsFull = false;

    GetPipeHeight();
    // check to see if value was nullified earlier
    if (M(0x00) != 0)
    { // (if d3, the usage control bit of second byte, was set)
        ++y;
        ++y;
        ++y;
        ++y; // add four if usage control bit was not set
    } // WarpPipe: save value in stack
    a = y;
    pha();
    a = M(AreaNumber) | M(WorldNumber); // if at world 1-1, do not add piranha plant ever
    if (a == 0)
    {
        goto DrawPipe;
    }
    y = M(AreaObjectLength + x); // if on second column of pipe, branch
    if (y == 0)
    {
        goto DrawPipe; // (because we only need to do this once)
    }
    allEnemySlotsFull = FindEmptyEnemySlot(); // check for an empty moving data buffer space
    if (allEnemySlotsFull)
    {
        goto DrawPipe; // if not found, too many enemies, thus skip
    }
    GetAreaObjXPosition();                         // get horizontal pixel coordinate
    wide = ((M(CurrentPageLoc) << 8) | a) + 0x08;  // add eight to put the piranha plant in the center
    writeData(Enemy_X_Position + x, LOBYTE(wide)); // store as enemy's horizontal coordinate
    writeData(Enemy_PageLoc + x, HIBYTE(wide));    // store as enemy's page coordinate
    a = HIBYTE(wide);                              // add carry to current page number
    a = 0x01;
    writeData(Enemy_Y_HighPos + x, 0x01);
    writeData(Enemy_Flag + x, 0x01); // activate enemy flag
    GetAreaObjYPosition();           // get piranha plant's vertical coordinate and store here
    writeData(Enemy_Y_Position + x, a);
    a = PiranhaPlant; // write piranha plant's value into buffer
    writeData(Enemy_ID + x, PiranhaPlant);
    InitPiranhaPlant();

DrawPipe: // get value saved earlier and use as Y
    pla();
    y = a;
    x = M(0x07); // get buffer offset
    // draw the appropriate pipe with the Y we loaded earlier
    writeData(MetatileBuffer + x, M(VerticalPipeData + y)); // render the top of the pipe
    ++x;
    a = M(VerticalPipeData + 2 + y); // render the rest of the pipe
    y = M(0x06);                     // subtract one from length and render the part underneath
    --y;
    RenderUnderPart();
}

// text_number is A, saved to stack
void SMBEngine::WriteGameText(uint8_t text_number)
{
    const uint8_t GameTextOffsets_data[] = {
        static_cast<uint8_t>(TopStatusBarLine - GameText),  static_cast<uint8_t>(TopStatusBarLine - GameText),
        static_cast<uint8_t>(WorldLivesDisplay - GameText), static_cast<uint8_t>(WorldLivesDisplay - GameText),
        static_cast<uint8_t>(TwoPlayerTimeUp - GameText),   static_cast<uint8_t>(OnePlayerTimeUp - GameText),
        static_cast<uint8_t>(TwoPlayerGameOver - GameText), static_cast<uint8_t>(OnePlayerGameOver - GameText),
        static_cast<uint8_t>(WarpZoneWelcome - GameText),   static_cast<uint8_t>(WarpZoneWelcome - GameText)};

    bool shiftedBit = false;

    // Compute index into GameTextOffsets:
    y = 2 * text_number;
    if (y >= 4)
    {
        y = std::min<uint8_t>(y, 8); // warp zone
        if (M(NumberOfPlayers) == 0)
        {        // single-player?
            ++y; // increment offset by one to not print name
        }
    }

    x = GameTextOffsets_data[y];
    y = 0;

    const uint8_t LuigiName_data[] = {
        0x15, 0x1e, 0x12, 0x10, 0x12 //  "LUIGI", no address or length
    };

    while ((a = M(GameText + x)) != 0xff) // terminator
    {
        writeData(VRAM_Buffer1 + y, a); // otherwise write data to buffer
        ++x;
        ++y;
        if (y == 0)
        {
            break; // do this for 256 bytes if no terminator found
        }
    }
    writeData(VRAM_Buffer1 + y, 0x00);
    a = text_number; // pull original text number from stack
    x = a;
    if (a < 0x04)
    {
        --x; // are we printing the world/lives display?
        if (x == 0)
        {                         // if not, branch to check player's name
            a = M(NumberofLives); // otherwise, check number of lives
            a += 0x01;
            if (a >= 10)
            {
                a -= 10;                           // if so, subtract 10 and put a crown tile
                y = 0x9f;                          // next to the difference...strange things happen if
                writeData(VRAM_Buffer1 + 7, 0x9f); // the number of lives exceeds 19
            } // PutLives
            writeData(VRAM_Buffer1 + 8, a);
            y = M(WorldNumber); // write world and level numbers (incremented for display)
            ++y;                // to the buffer in the spaces surrounding the dash
            writeData(VRAM_Buffer1 + 19, y);
            y = M(LevelNumber);
            ++y;
            writeData(VRAM_Buffer1 + 21, y); // we're done here
            return;

            //------------------------------------------------------------------------
        } // CheckPlayerName
        a = M(NumberOfPlayers); // check number of players
        if (a == 0)
        {
            return; // if only 1 player, leave
        }
        a = M(CurrentPlayer); // load current player
        --x;                  // check to see if current message number is for time up
        if (x != 0)
        {
            goto ChkLuigi;
        }
        y = M(OperMode); // check for game over mode
        if (y == GameOverModeValue)
        {
            goto ChkLuigi;
        }
        a ^= 0b00000001; // if not, must be time up, invert d0 to do other player

    ChkLuigi:
        shiftedBit = (a & 0x01) != 0;
        a >>= 1;
        if (!shiftedBit)
        {
            return; // if mario is current player, do not change the name
        }
        y = 0x04;

        do // NameLoop: otherwise, replace "MARIO" with "LUIGI"
        {
            a = LuigiName_data[y];
            writeData(VRAM_Buffer1 + 3 + y, a);
            --y;
        } while ((y & 0x80) == 0); // do this until each letter is replaced

        return; // ExitChkName

        //------------------------------------------------------------------------
    } // PrintWarpZoneNumbers
    a -= 0x04; // subtract 4 and then shift to the left
    a <<= 1;   // twice to get proper warp zone number
    a <<= 1;   // offset
    x = a;
    y = 0x00;

    do // WarpNumLoop: print warp zone numbers into the
    {
        writeData(VRAM_Buffer1 + 27 + y, M(WarpZoneNumbers + x)); // placeholders from earlier
        ++x;
        y += 4; // put a number in every fourth space
    } while (y < 0x0c);
    a = 0x2c; // load new buffer pointer at end of message
    SetVRAMOffset();
}

void SMBEngine::RenderAttributeTables()
{
    bool shiftedBit = false;

    // get low byte of next name table address
    a = M(CurrentNTAddr_Low) & 0b00011111; // to be written to, mask out all but 5 LSB,
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
    a = M(0x01);                  // get low byte - 4, divide by 4, add offset for
    shiftedBit = (a & 0x02) != 0; // the second shift carries d1 out
    a >>= 1;                      // attribute table and store
    a >>= 1;
    a = (uint8_t)(a + 0xc0 + (shiftedBit ? 1 : 0)); // we should now have the appropriate block of
    writeData(0x01, a);                             // attribute table in our temp address
    x = 0x00;
    y = M(VRAM_Buffer2_Offset); // get buffer offset

    do // AttribLoop
    {
        writeData(VRAM_Buffer2 + y, M(0x00)); // store high byte of attribute table address
        a = M(0x01);
        a += 0x08; // below the status bar, and store
        writeData(VRAM_Buffer2 + 1 + y, a);
        writeData(0x01, a); // also store in temp again
        // fetch current attribute table byte and store
        writeData(VRAM_Buffer2 + 3 + y, M(AttributeBuffer + x)); // in the buffer
        writeData(VRAM_Buffer2 + 2 + y, 0x01);                   // store length of 1 in buffer
        a = 0x00;
        writeData(AttributeBuffer + x, 0x00); // clear current byte in attribute buffer
        ++y;                                  // increment buffer offset by 4 bytes
        ++y;
        ++y;
        ++y;
        ++x; // increment attribute offset and check to see
    } while (x < 0x07);
    writeData(VRAM_Buffer2 + y, a);    // put null terminator at the end
    writeData(VRAM_Buffer2_Offset, y); // store offset in case we want to do any more

    SetVRAMCtrl();
}

void SMBEngine::SetVRAMCtrl()
{
    a = 0x06;
    writeData(VRAM_Buffer_AddrCtrl, 0x06); // set buffer to $0341 and leave
}

void SMBEngine::IncrementColumnPos()
{
    ++M(CurrentColumnPos);                // increment column where we're at
    a = M(CurrentColumnPos) & 0b00001111; // mask out higher nybble
    if (a == 0)
    {
        writeData(CurrentColumnPos, a); // if no bits left set, wrap back to zero (0-f)
        ++M(CurrentPageLoc);            // and increment page number where we're at
    } // NoColWrap: increment column offset where we're at
    ++M(BlockBufferColumnPos);
    a = M(BlockBufferColumnPos) & 0b00011111; // mask out all but 5 LSB (0-1f)
    writeData(BlockBufferColumnPos, a);       // and save
}

void SMBEngine::AlterAreaAttributes()
{
    y = M(AreaObjOffsetBuffer + x); // load offset for level object data saved in buffer
    ++y;                            // load second byte
    a = M(W(AreaData) + y);
    pha(); // save in stack for now
    a &= 0b01000000;
    if (a == 0)
    { // branch if d6 is set
        pla();
        pha();                        // pull and push offset to copy to A
        a &= 0b00001111;              // mask out high nybble and store as
        writeData(TerrainControl, a); // new terrain height type bits
        pla();
        a &= 0b00110000; // pull and mask out all but d5 and d4
        a >>= 1;         // move bits to lower nybble and store
        a >>= 1;         // as new background scenery bits
        a >>= 1;
        a >>= 1;
        writeData(BackgroundScenery, a); // then leave
        return;

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
}

void SMBEngine::ScrollLockObject_Warp()
{
    x = 0x04; // load value of 4 for game text routine as default
    // warp zone (4-3-2), then check world number
    if (M(WorldNumber) == 0)
    {
        goto WarpNum;
    }
    x = 0x05;        // if world number > 1, increment for next warp zone (5)
    y = M(AreaType); // check area type
    --y;
    if (y != 0)
    {
        goto WarpNum; // if ground area type, increment for last warp zone
    }
    x = 0x06; // (8-7-6) and move on

WarpNum:
    a = x;
    writeData(WarpZoneControl, a); // store number here to be used by warp zone routine
    WriteGameText(a);              // print text and warp zone numbers
    a = PiranhaPlant;
    KillEnemies(); // load identifier for piranha plants and do sub

    ScrollLockObject();
}

void SMBEngine::ScrollLockObject()
{
    // invert scroll lock to turn it on
    a = M(ScrollLock) ^ 0b00000001;
    writeData(ScrollLock, a);
}

void SMBEngine::IntroPipe()
{
    bool lrgObjJustStarted = false;
    bool sidePipeShaftDrawn = false;

    y = 0x03; // check if length set, if not set, set it
    lrgObjJustStarted = ChkLrgObjFixedLength();
    y = 0x0a; // set fixed value and render the sideways part
    sidePipeShaftDrawn = RenderSidewaysPipe();
    if (sidePipeShaftDrawn)
    {             // if the shaft was not drawn, not time to draw vertical pipe part
        x = 0x06; // blank everything above the vertical pipe part

        do // VPipeSectLoop: all the way to the top of the screen
        {
            a = 0x00;
            writeData(MetatileBuffer + x, 0x00); // because otherwise it will look like exit pipe
            --x;
        } while ((x & 0x80) == 0);
        a = M(VerticalPipeData + y); // draw the end of the vertical pipe part
        writeData(MetatileBuffer + 7, a);
    } // NoBlankP
}

void SMBEngine::ExitPipe()
{
    bool lrgObjJustStarted = false;
    bool sidePipeShaftDrawn = false;

    y = 0x03; // check if length set, if not set, set it
    lrgObjJustStarted = ChkLrgObjFixedLength();
    GetLrgObjAttrib(); // get vertical length, then plow on through RenderSidewaysPipe

    sidePipeShaftDrawn = RenderSidewaysPipe();
}

bool SMBEngine::RenderSidewaysPipe()
{
    const uint8_t SidePipeBottomPart_data[] = {0x15, 0x21, // bottom part of sideways part of pipe
                                               0x20, 0x1f};

    const uint8_t SidePipeTopPart_data[] = {0x15, 0x1e, // top part of sideways part of pipe
                                            0x1d, 0x1c};

    const uint8_t SidePipeShaftData_data[] = {
        0x15, 0x14, // used to control whether or not vertical pipe shaft
        0x00, 0x00  // is drawn, and if so, controls the metatile number
    };

    bool sidePipeShaftDrawn = false;

    --y; // decrement twice to make room for shaft at bottom
    --y; // and store here for now as vertical length
    writeData(0x05, y);
    y = M(AreaObjectLength + x); // get length left over and store here
    writeData(0x06, y);
    x = M(0x05); // get vertical length plus one, use as buffer offset
    ++x;
    a = SidePipeShaftData_data[y]; // check for value $00 based on horizontal offset
    if (a != 0x00)
    { // if found, do not draw the vertical pipe shaft
        x = 0x00;
        y = M(0x05);               // init buffer offset and get vertical length
        RenderUnderPart();         // and render vertical shaft using tile number in A
        sidePipeShaftDrawn = true; // used by IntroPipe
    }
    else
    {
        sidePipeShaftDrawn = false;
    } // DrawSidePart: render side pipe part at the bottom
    y = M(0x06);
    writeData(MetatileBuffer + x, SidePipeTopPart_data[y]); // note that the pipe parts are stored
    a = SidePipeBottomPart_data[y];                         // backwards horizontally
    writeData(MetatileBuffer + 1 + x, a);
    return sidePipeShaftDrawn;
}

void SMBEngine::QuestionBlockRow_High()
{
    a = 0x03; // start on the fourth row
    Skip_1();
}

void SMBEngine::QuestionBlockRow_Low()
{
    a = 0x07; // start on the eighth row
    Skip_1();
}

void SMBEngine::Skip_1()
{
    bool lrgObjJustStarted = false;

    pha();                                 // save whatever row to the stack for now
    lrgObjJustStarted = ChkLrgObjLength(); // get low nybble and save as length
    pla();
    x = a; // render question boxes with coins
    a = 0xc0;
    writeData(MetatileBuffer + x, 0xc0);
}

void SMBEngine::Bridge_High()
{
    a = 0x06; // start on the seventh row from top of screen
    Skip_2();
}

void SMBEngine::Bridge_Middle()
{
    a = 0x07; // start on the eighth row
    Skip_2();
}

void SMBEngine::Skip_2() { Skip_3(); }

void SMBEngine::Bridge_Low()
{
    a = 0x09; // start on the tenth row
    Skip_3();
}

void SMBEngine::Skip_3()
{
    bool lrgObjJustStarted = false;

    pha();                                 // save whatever row to the stack for now
    lrgObjJustStarted = ChkLrgObjLength(); // get low nybble and save as length
    pla();
    x = a; // render bridge railing
    writeData(MetatileBuffer + x, 0x0b);
    ++x;
    y = 0x00; // now render the bridge itself
    a = 0x63;
    RenderUnderPart();
}

void SMBEngine::EndlessRope()
{
    x = 0x00; // render rope from the top to the bottom of screen
    y = 0x0f;
    DrawRope();
}

void SMBEngine::BalancePlatRope()
{
    a = x; // save object buffer offset for now
    pha();
    x = 0x01; // blank out all from second row to the bottom
    y = 0x0f; // with blank used for balance platform rope
    a = 0x44;
    RenderUnderPart();
    pla(); // get back object buffer offset
    x = a;
    GetLrgObjAttrib(); // get vertical length from lower nybble
    x = 0x01;

    DrawRope();
}

// render the actual rope
void SMBEngine::DrawRope()
{
    a = 0x40;
    RenderUnderPart();
}

void SMBEngine::CastleBridgeObj()
{
    bool lrgObjJustStarted = false;

    y = 0x0c; // load length of 13 columns
    lrgObjJustStarted = ChkLrgObjFixedLength();
    ChainObj();
}

void SMBEngine::AxeObj()
{
    a = 0x08; // load bowser's palette into sprite portion of palette
    writeData(VRAM_Buffer_AddrCtrl, 0x08);

    ChainObj();
}

void SMBEngine::ChainObj()
{
    const uint8_t C_ObjectMetatile_data[] = {0xc5, 0x0c, 0x89};

    const uint8_t C_ObjectRow_data[] = {0x06, 0x07, 0x08};

    y = M(0x00);                 // get value loaded earlier from decoder
    x = C_ObjectRow_data[y - 2]; // get appropriate row and metatile for object
    a = C_ObjectMetatile_data[y - 2];
    ColObj();
}

// get appropriate metatile for brick (question block
void SMBEngine::DrawQBlk()
{
    a = M(BrickQBlockMetatiles + y);
    pha();             // if branched to here from question block routine)
    GetLrgObjAttrib(); // get row from location byte
    DrawRow();         // now render the object
}

void SMBEngine::RenderAreaGraphics()
{
    const uint8_t MetatileGraphics_High_data[] = {HIBYTE(Palette0_MTiles), HIBYTE(Palette1_MTiles), HIBYTE(Palette2_MTiles),
                                                  HIBYTE(Palette3_MTiles)};

    const uint8_t MetatileGraphics_Low_data[] = {LOBYTE(Palette0_MTiles), LOBYTE(Palette1_MTiles), LOBYTE(Palette2_MTiles),
                                                 LOBYTE(Palette3_MTiles)};

    // store LSB of where we're at
    a = M(CurrentColumnPos) & 0x01;
    writeData(0x05, a);
    y = M(VRAM_Buffer2_Offset); // store vram buffer offset
    writeData(0x00, y);
    // get current name table address we're supposed to render
    writeData(VRAM_Buffer2 + 1 + y, M(CurrentNTAddr_Low));
    writeData(VRAM_Buffer2 + y, M(CurrentNTAddr_High));
    // store length byte of 26 here with d7 set
    writeData(VRAM_Buffer2 + 2 + y, 0x9a); // to increment by 32 (in columns)
    a = 0x00;                              // init attribute row
    writeData(0x04, 0x00);
    x = 0x00;

    do // DrawMTLoop: store init value of 0 or incremented offset for buffer
    {
        writeData(0x01, x);
        // get first metatile number, and mask out all but 2 MSB
        a = M(MetatileBuffer + x) & 0b11000000;
        writeData(0x03, a); // store attribute table bits here
        a >>= 6;            // note that metatile format is %xx000000 attribute table bits,
        y = a;              // %00xxxxxx metatile number, so move the bits to d1-d0 and use as offset here
        // get address to graphics table from here
        writeData(0x06, MetatileGraphics_Low_data[y]);
        writeData(0x07, MetatileGraphics_High_data[y]);
        a = M(MetatileBuffer + x); // get metatile number again
        a <<= 1;                   // multiply by 4 and use as tile offset
        a <<= 1;
        writeData(0x02, a);
        // get current task number for level processing and
        a = M(AreaParserTaskNum) & 0b00000001; // mask out all but LSB, then invert LSB, multiply by 2
        a ^= 0b00000001;                       // to get the correct column position in the metatile,
        a <<= 1;                               // then add to the tile offset so we can draw either side
        a += M(0x02);                          // of the metatiles
        y = a;
        x = M(0x00);                                     // use vram buffer offset from before as X
        writeData(VRAM_Buffer2 + 3 + x, M(W(0x06) + y)); // get first tile number (top left or top right) and store
        ++y;
        // now get the second (bottom left or bottom right) and store
        writeData(VRAM_Buffer2 + 4 + x, M(W(0x06) + y));
        y = M(0x04); // get current attribute row
        // get LSB of current column where we're at, and
        if (M(0x05) == 0)
        { // branch if set (clear = left attrib, set = right)
            // get current row we're rendering
            a = M(0x01) >> 1; // branch if LSB set (clear = top left, set = bottom left)
            if ((M(0x01) & 0x01) != 0)
            {
                goto LLeft;
            }
            M(0x03) >>= 6; // move the attribute bits into d1-d0, for upper left square
            goto SetAttrib;
        } // RightCheck: get LSB of current row we're rendering
        a = M(0x01) >> 1; // branch if set (clear = top right, set = bottom right)
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

    SetAttrib:                                // get previously saved bits from before
        a = M(AttributeBuffer + y) | M(0x03); // if any, and put new bits, if any, onto
        writeData(AttributeBuffer + y, a);    // the old, and store
        ++M(0x00);                            // increment vram buffer offset by 2
        ++M(0x00);
        x = M(0x01); // get current gfx buffer row, and check for
        ++x;         // the bottom of the screen
    } while (x < 0x0d); // if not there yet, loop back
    y = M(0x00); // get current vram buffer offset, increment by 3
    ++y;         // (for name table address and length bytes)
    ++y;
    ++y;
    writeData(VRAM_Buffer2 + y, 0x00); // put null terminator at end of data for name table
    writeData(VRAM_Buffer2_Offset, y); // store new buffer offset
    ++M(CurrentNTAddr_Low);            // increment name table address low
    // check current low byte
    a = M(CurrentNTAddr_Low) & 0b00011111; // if no wraparound, just skip this part
    if (a == 0)
    {
        // if wraparound occurs, make sure low byte stays
        writeData(CurrentNTAddr_Low, 0x80); // just under the status bar
        // and then invert d2 of the name table address high
        a = M(CurrentNTAddr_High) ^ 0b00000100; // to move onto the next appropriate name table
        writeData(CurrentNTAddr_High, a);
    } // ExitDrawM: jump to set buffer to $0341 and leave
    SetVRAMCtrl();
}

void SMBEngine::AreaParserTaskHandler()
{
    y = M(AreaParserTaskNum); // check number of tasks here
    if (y == 0)
    { // if already set, go ahead
        y = 0x08;
        writeData(AreaParserTaskNum, 0x08); // otherwise, set eight by default
    } // DoAPTasks
    --y;
    a = y;
    AreaParserTasks();
    --M(AreaParserTaskNum); // if all tasks not complete do not
    if (M(AreaParserTaskNum) == 0)
    { // render attribute table yet
        RenderAttributeTables();
    } // SkipATRender
}

void SMBEngine::AreaParserTasks()
{
    switch (a)
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
    x = 0x0c;
    a = 0x00;

    do // ClrMTBuf: clear out metatile buffer
    {
        writeData(MetatileBuffer + x, 0x00);
        --x;
    } while ((x & 0x80) == 0);
    y = M(BackgroundScenery); // do we need to render the background scenery?
    if (y == 0)
    {
        goto RendFore; // if not, skip to check the foreground
    }
    a = M(CurrentPageLoc); // otherwise check for every third page

ThirdP:
    if (a >= 3)
    {              // if less than three we're there
        a -= 0x03; // if 3 or more, subtract 3 and
        if ((a & 0x80) == 0)
        {
            goto ThirdP; // do an unconditional branch
        }
    } // RendBack: move results to higher nybble
    a <<= 1;
    a <<= 1;
    a <<= 1;
    a <<= 1;
    a += BSceneDataOffsets_data[y - 1]; // add to it offset loaded from here
    a += M(CurrentColumnPos);           // add to the result our current column position
    x = a;
    a = BackSceneryData_data[x]; // load data from sum of offsets
    if (a == 0)
    {
        goto RendFore; // if zero, no scenery for that part
    }
    pha();
    a &= 0x0f;          // save to stack and clear high nybble
    a -= 0x01;          // subtract one (because low nybble is $01-$0c)
    writeData(0x00, a); // save low nybble
    a <<= 1;            // multiply by three (shift to left and add result to old one)
    a += M(0x00);
    x = a; // save as offset for background scenery metatile data
    pla(); // get high nybble from stack, move low
    a >>= 1;
    a >>= 1;
    a >>= 1;
    a >>= 1;
    y = a;    // use as second offset (used to determine height)
    a = 0x03; // use previously saved memory location for counter
    writeData(0x00, 0x03);

    do // SceLoop1: load metatile data from offset of (lsb - 1) * 3
    {
        writeData(MetatileBuffer + y, static_cast<uint8_t>(BackSceneryMetatiles_data[x])); // store into buffer from offset of (msb / 16)
        ++x;
        ++y;
        if (y == 0x0b)
        {
            goto RendFore;
        }
        --M(0x00); // decrement until counter expires, barring exception
    } while (M(0x00) != 0);

RendFore: // check for foreground data needed or not
    x = M(ForegroundScenery);
    if (x != 0)
    {                                      // if not, skip this part
        y = FSceneDataOffsets_data[x - 1]; // load offset from location offset by header value, then
        x = 0x00;                          // reinit X

        do // SceLoop2: load data until counter expires
        {
            a = ForeSceneryData_data[y];
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
    {
        goto TerMTile; // if not water level, skip this part
    }
    // check world number, if not world number eight
    if (M(WorldNumber) != World8)
    {
        goto TerMTile;
    }
    a = 0x62;  // if set as water level and world number eight,
    StoreMT(); // use castle wall metatile as terrain type
    return;

TerMTile: // otherwise get appropriate metatile for area type
    a = TerrainMetatiles_data[y];
    // check for cloud type override
    if (M(CloudTypeOverride) == 0)
    {
        StoreMT(); // if not set, keep value otherwise
        return;
    }
    a = 0x88; // use cloud block terrain
    StoreMT();
}

// store value here
void SMBEngine::StoreMT()
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

    writeData(0x07, a);
    x = 0x00;              // initialize X, use as metatile buffer offset
    a = M(TerrainControl); // use yet another value from the header
    a <<= 1;               // multiply by 2 and use as yet another offset
    y = a;

TerrLoop: // get one of the terrain rendering bit data
    writeData(0x00, TerrainRenderBits_data[y]);
    ++y; // increment Y and use as offset next time around
    writeData(0x01, y);
    // skip if value here is zero
    if (M(CloudTypeOverride) == 0)
    {
        goto NoCloud2;
    }
    if (x == 0x00)
    {
        goto NoCloud2;
    }
    // if not, mask out all but d3
    a = M(0x00) & 0b00001000;
    writeData(0x00, a);

NoCloud2: // start at beginning of bitmasks
    y = 0x00;

TerrBChk: // load bitmask, then perform AND on contents of first byte
    if ((M(Bitmasks + y) & M(0x00)) != 0)
    {                                           // if not set, skip this part (do not write terrain to buffer)
        writeData(MetatileBuffer + x, M(0x07)); // load terrain type metatile number and store into buffer here
    } // NextTBit: continue until end of buffer
    ++x;
    if (x != 0x0d)
    { // if we're at the end, break out of this loop
        // check world type for underground area
        if (M(AreaType) != 0x02)
        {
            goto EndUChk; // if not underground, skip this part
        }
        if (x != 0x0b)
        {
            goto EndUChk; // if we're at the bottom of the screen, override
        }
        a = 0x54; // old terrain type with ground level terrain type
        writeData(0x07, 0x54);

    EndUChk: // increment bitmasks offset in Y
        ++y;
        if (y != 0x08)
        {
            goto TerrBChk; // if not all bits checked, loop back
        }
        y = M(0x01);
        if (y != 0)
        {
            goto TerrLoop; // unconditional branch, use Y to load next byte
        }
    } // RendBBuf: do the area data loading routine now
    ProcessAreaData();
    a = M(BlockBufferColumnPos);
    GetBlockBufferAddr(); // get block buffer address from where we're at
    x = 0x00;
    y = 0x00; // init index regs and start at beginning of smaller buffer

    do // ChkMTLow
    {
        writeData(0x00, y);
        // load stored metatile number
        a = M(MetatileBuffer + x) & 0b11000000; // mask out all but 2 MSB
        a >>= 6;                                // make %xx000000 into %000000xx
        y = a;                                  // use as offset in Y
        a = M(MetatileBuffer + x);              // reload original unmasked value here
        if (a < BlockBuffLowBounds_data[y])
        {             // if equal or greater, branch
            a = 0x00; // if less, init value before storing
        } // StrBlock: get offset for block buffer
        y = M(0x00);
        writeData(W(0x06) + y, a); // store value into block buffer
        a = y;
        a += 0x10;
        y = a;
        ++x; // increment column value
    } while (x < 0x0d); // continue until we pass last row, then leave
}

void SMBEngine::ProcessAreaData()
{
ProcessAreaDataStart:
    x = 0x02; // start at the end of area object buffer

    do // ProcADLoop
    {
        writeData(ObjectOffset, x);
        // reset flag
        writeData(BehindAreaParserFlag, 0x00);
        y = M(AreaDataOffset); // get offset of area data pointer
        // get first byte of area object
        if (M(W(AreaData) + y) == 0xfd)
        {
            goto RdyDecode;
        }
        // check area object buffer flag
        if ((M(AreaObjectLength + x) & 0x80) == 0)
        {
            goto RdyDecode; // if buffer not negative, branch, otherwise
        }
        ++y;
        a = M(W(AreaData) + y); // get second byte of area object
        a <<= 1;                // check for page select bit (d7), branch if not set
        if ((M(W(AreaData) + y) & 0x80) == 0)
        {
            goto Chk1Row13;
        }
        // check page select
        if (M(AreaObjectPageSel) != 0)
        {
            goto Chk1Row13;
        }
        ++M(AreaObjectPageSel); // if not already set, set it now
        ++M(AreaObjectPageLoc); // and increment page location

    Chk1Row13:
        --y;
        // reread first byte of level object
        a = M(W(AreaData) + y) & 0x0f; // mask out high nybble
        if (a == 0x0d)
        {
            ++y; // if so, reread second byte of level object
            a = M(W(AreaData) + y);
            --y;             // decrement to get ready to read first byte
            a &= 0b01000000; // check for d6 set (if not, object is page control)
            if (a != 0)
            {
                goto CheckRear;
            }
            // if page select is set, do not reread
            if (M(AreaObjectPageSel) != 0)
            {
                goto CheckRear;
            }
            ++y;                                 // if d6 not set, reread second byte
            a = M(W(AreaData) + y) & 0b00011111; // mask out all but 5 LSB and store in page control
            writeData(AreaObjectPageLoc, a);
            ++M(AreaObjectPageSel); // increment page select
        } // Chk1Row14: row 14?
        else
        {
            if (a != 0x0e)
            {
                goto CheckRear;
            }
            // check flag for saved page number and branch if set
            if (M(BackloadingFlag) != 0)
            {
                goto RdyDecode; // to render the object (otherwise bg might not look right)
            }

        CheckRear: // check to see if current page of level object is
            if (M(AreaObjectPageLoc) >= M(CurrentPageLoc))
            { // if so branch

            RdyDecode: // do sub and do not turn on flag
                DecodeAreaData();
                goto ChkLength;
            } // SetBehind: turn on flag if object is behind renderer
            ++M(BehindAreaParserFlag);
        } // NextAObj: increment buffer offset and move on
        IncAreaObjOffset();

    ChkLength: // get buffer offset
        x = M(ObjectOffset);
        // check object length for anything stored here
        if ((M(AreaObjectLength + x) & 0x80) == 0)
        {                              // if not, branch to handle loopback
            --M(AreaObjectLength + x); // otherwise decrement length or get rid of it
        } // ProcLoopb: decrement buffer offset
        --x;
    } while ((x & 0x80) == 0); // and loopback unless exceeded buffer
    // check for flag set if objects were behind renderer
    if (M(BehindAreaParserFlag) != 0)
    {
        goto ProcessAreaDataStart; // branch if true to load more level data, otherwise
    }
    a = M(BackloadingFlag); // check for flag set if starting right of page $00
    if (a != 0)
    {
        goto ProcessAreaDataStart; // branch if true to load more level data, otherwise leave
    }
}

void SMBEngine::DecodeAreaData()
{
    // check current buffer flag
    if ((M(AreaObjectLength + x) & 0x80) == 0)
    {
        y = M(AreaObjOffsetBuffer + x); // if not, get offset from buffer
    } // Chk1stB: load offset of 16 for special row 15
    x = 0x10;
    a = M(W(AreaData) + y); // get first byte of level object again
    if (a == 0xfd)
    {
        return; // if end of level, leave this routine
    }
    a &= 0x0f; // otherwise, mask out low nybble
    if (a == 0x0f)
    {
        goto ChkRow14; // if so, keep the offset of 16
    }
    x = 0x08; // otherwise load offset of 8 for special row 12
    if (a == 0x0c)
    {
        goto ChkRow14; // if so, keep the offset value of 8
    }
    x = 0x00; // otherwise nullify value by default

ChkRow14: // store whatever value we just loaded here
    writeData(0x07, x);
    x = M(ObjectOffset); // get object offset again
    if (a == 0x0e)
    {
        // if so, load offset with $00
        writeData(0x07, 0x00);
        a = 0x2e; // and load A with another value
        if (a != 0)
        {
            NormObj(); // unconditional branch
            return;
        }
    } // ChkRow13: row 13?
    if (a == 0x0d)
    {
        // if so, load offset with 34
        writeData(0x07, 0x22);
        ++y;                                 // get next byte
        a = M(W(AreaData) + y) & 0b01000000; // mask out all but d6 (page control obj bit)
        if (a == 0)
        {
            return; // if d6 clear, branch to leave (we handled this earlier)
        }
        // otherwise, get byte again
        a = M(W(AreaData) + y) & 0b01111111; // mask out d7
        if (a == 0x4b)
        {                     // (plus d6 set for object other than page control)
            ++M(LoopCommand); // if loop command, set loop command flag
        } // Mask2MSB: mask out d7 and d6
        a &= 0b00111111;
        NormObj(); // and jump
        return;
    } // ChkSRows: row 12-15?
    if (a < 0x0c)
    {
        ++y;                                 // if not, get second byte of level object
        a = M(W(AreaData) + y) & 0b01110000; // mask out all but d6-d4
        if (a == 0)
        {                          // if any bits set, branch to handle large object
            writeData(0x07, 0x16); // otherwise set offset of 24 for small object
            // reload second byte of level object
            a = M(W(AreaData) + y) & 0b00001111; // mask out higher nybble and jump
            NormObj();
            return;
        } // LrgObj: store value here (branch for large objects)
        writeData(0x00, a);
        if (a != 0x70)
        {
            goto NotWPipe;
        }
        // if not, reload second byte
        a = M(W(AreaData) + y) & 0b00001000; // mask out all but d3 (usage control bit)
        if (a == 0)
        {
            goto NotWPipe; // if d3 clear, branch to get original value
        }
        a = 0x00; // otherwise, nullify value for warp pipe
        writeData(0x00, 0x00);

    NotWPipe: // get value and jump ahead
        a = M(0x00);
    } // SpecObj: branch here for rows 12-15
    else
    {
        ++y;
        a = M(W(AreaData) + y) & 0b01110000; // get next byte and mask out all but d6-d4
    } // MoveAOId: move d6-d4 to lower nybble
    a >>= 1;
    a >>= 1;
    a >>= 1;
    a >>= 1;
    NormObj();
}

// store value here (branch for small objects and rows 13 and 14)
void SMBEngine::NormObj()
{
    writeData(0x00, a);
    // is there something stored here already?
    if ((M(AreaObjectLength + x) & 0x80) != 0)
    { // if so, branch to do its particular sub
        // otherwise check to see if the object we've loaded is on the
        if (M(AreaObjectPageLoc) != M(CurrentPageLoc))
        {
            y = M(AreaDataOffset); // if not, get old offset of level pointer
            // and reload first byte
            a = M(W(AreaData) + y) & 0b00001111;
            if (a != 0x0e)
            {
                return;
            }
            a = M(BackloadingFlag); // if so, check backloading flag
            if (a != 0)
            {
                goto StrAObj; // if set, branch to render object, else leave
            }

            return; // LeavePar

            //------------------------------------------------------------------------
        } // InitRear: check backloading flag to see if it's been initialized
        if (M(BackloadingFlag) != 0)
        {                                     // branch to column-wise check
            a = 0x00;                         // if not, initialize both backloading and
            writeData(BackloadingFlag, 0x00); // behind-renderer flags and leave
            writeData(BehindAreaParserFlag, 0x00);
            writeData(ObjectOffset, 0x00);

            return; // LoopCmdE

            //------------------------------------------------------------------------
        } // BackColC: get first byte again
        y = M(AreaDataOffset);
        a = M(W(AreaData) + y) & 0b11110000; // mask out low nybble and move high to low
        a >>= 1;
        a >>= 1;
        a >>= 1;
        a >>= 1;
        if (a != M(CurrentColumnPos))
        {
            return; // if not, branch to leave
        }

    StrAObj: // if so, load area obj offset and store in buffer
        writeData(AreaObjOffsetBuffer + x, M(AreaDataOffset));
        IncAreaObjOffset(); // do sub to increment to next object data
    } // RunAObj: get stored value and add offset to it
    a = M(0x00);
    a += M(0x07);
    switch (a)
    {
    case 0:
        VerticalPipe(); // used by warp pipes
        return;
    case 1:
        AreaStyleObject();
        return;
    case 2:
        RowOfBricks();
        return;
    case 3:
        RowOfSolidBlocks();
        return;
    case 4:
        RowOfCoins();
        return;
    case 5:
        ColumnOfBricks();
        return;
    case 6:
        ColumnOfSolidBlocks();
        return;
    case 7:
        VerticalPipe(); // used by decoration pipes
        return;
    case 8:
        Hole_Empty();
        return;
    case 9:
        PulleyRopeObject();
        return;
    case 10:
        Bridge_High();
        return;
    case 11:
        Bridge_Middle();
        return;
    case 12:
        Bridge_Low();
        return;
    case 13:
        Hole_Water();
        return;
    case 14:
        QuestionBlockRow_High();
        return;
    case 15:
        QuestionBlockRow_Low();
        return;
    case 16:
        EndlessRope();
        return;
    case 17:
        BalancePlatRope();
        return;
    case 18:
        CastleObject();
        return;
    case 19:
        StaircaseObject();
        return;
    case 20:
        ExitPipe();
        return;
    case 21:
        FlagBalls_Residual();
        return;
    case 22:
        QuestionBlock(); // power-up
        return;
    case 23:
        QuestionBlock(); // coin
        return;
    case 24:
        QuestionBlock(); // hidden, coin
        return;
    case 25:
        Hidden1UpBlock(); // hidden, 1-up
        return;
    case 26:
        BrickWithItem(); // brick, power-up
        return;
    case 27:
        BrickWithItem(); // brick, vine
        return;
    case 28:
        BrickWithItem(); // brick, star
        return;
    case 29:
        BrickWithCoins(); // brick, coins
        return;
    case 30:
        BrickWithItem(); // brick, 1-up
        return;
    case 31:
        WaterPipe();
        return;
    case 32:
        EmptyBlock();
        return;
    case 33:
        Jumpspring();
        return;
    case 34:
        IntroPipe();
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
        CastleBridgeObj();
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
        AlterAreaAttributes();
        return;
    default:
        bad_jump();
        return;
    }
}

void SMBEngine::Hidden1UpBlock()
{
    a = M(Hidden1UpFlag); // if flag not set, do not render object
    if (a == 0)
    {
        return;
    }

    a = 0x00; // if set, init for the next one
    writeData(Hidden1UpFlag, 0x00);
    BrickWithItem(); // jump to code shared with unbreakable bricks
}

void SMBEngine::QuestionBlock()
{
    GetAreaObjectID(); // get value from level decoder routine
    DrawQBlk();        // go to render it
}

void SMBEngine::BrickWithCoins()
{
    a = 0x00; // initialize multi-coin timer flag
    writeData(BrickCoinTimerFlag, 0x00);
    BrickWithItem();
}

void SMBEngine::BrickWithItem()
{
    GetAreaObjectID(); // save area object ID
    writeData(0x07, y);
    a = 0x00;        // load default adder for bricks with lines
    y = M(AreaType); // check level type for ground level
    --y;
    if (y != 0)
    {             // if ground type, do not start with 5
        a = 0x05; // otherwise use adder for bricks without lines
    } // BWithL: add object ID to adder
    a += M(0x07);
    y = a; // use as offset for metatile

    DrawQBlk();
}

void SMBEngine::GetAreaObjectID()
{
    a = M(0x00); // get value saved from area parser routine
    a -= 0x00;   // possibly residual code
    y = a;       // save to Y
}
