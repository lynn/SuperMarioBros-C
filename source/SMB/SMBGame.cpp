// The GameCoreRoutine subsystem: everything GameCoreRoutine() reaches that nothing
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
void SMBEngine::ColorRotation()
{
    const uint8_t Palette3Data_data[] = {0x0f, 0x07, 0x12, 0x0f, 0x0f, 0x07, 0x17, 0x0f, 0x0f, 0x07, 0x17, 0x1c, 0x0f, 0x07, 0x17, 0x00};

    const uint8_t BlankPalette_data[] = {0x3f, 0x0c, 0x04, 0xff, 0xff, 0xff, 0xff, 0x00};

    const uint8_t ColorRotatePalette_data[] = {0x27, 0x27, 0x27, 0x17, 0x07, 0x17};

    // get frame counter
    a = M(FrameCounter) & 0x07; // mask out all but three LSB
    if (a != 0)
    {
        return; // branch if not set to zero to do this every eighth frame
    }
    x = M(VRAM_Buffer1_Offset); // check vram buffer offset
    if (x >= 0x31)
    {
        return; // if offset over 48 bytes, branch to leave
    }
    y = a; // otherwise use frame counter's 3 LSB as offset here

    do // GetBlankPal: get blank palette for palette 3
    {
        writeData(VRAM_Buffer1 + x, BlankPalette_data[y]); // store it in the vram buffer
        ++x;                                               // increment offsets
        ++y;
    } while (y < 0x08); // do this until all bytes are copied
    x = M(VRAM_Buffer1_Offset); // get current vram buffer offset
    writeData(0x00, 0x03);      // set counter here
    a = M(AreaType);            // get area type
    a <<= 1;                    // multiply by 4 to get proper offset
    a <<= 1;
    y = a; // save as offset here

    do // GetAreaPal: fetch palette to be written based on area type
    {
        writeData(VRAM_Buffer1 + 3 + x, Palette3Data_data[y]); // store it to overwrite blank palette in vram buffer
        ++y;
        ++x;
        --M(0x00); // decrement counter
    } while ((M(0x00) & 0x80) == 0); // do this until the palette is all copied
    x = M(VRAM_Buffer1_Offset);                                  // get current vram buffer offset
    y = M(ColorRotateOffset);                                    // get color cycling offset
    writeData(VRAM_Buffer1 + 4 + x, ColorRotatePalette_data[y]); // get and store current color in second slot of palette
    a = M(VRAM_Buffer1_Offset);
    a += 0x07;
    writeData(VRAM_Buffer1_Offset, a);
    ++M(ColorRotateOffset); // increment color cycling offset
    a = M(ColorRotateOffset);
    if (a < 0x06)
    {
        return; // if so, branch to leave
    }
    a = 0x00;
    writeData(ColorRotateOffset, 0x00); // otherwise, init to keep it in range

    // ExitColorRot: leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::GetAreaMusic()
{
    const uint8_t MusicSelectData_data[] = {WaterMusic, GroundMusic, UndergroundMusic, CastleMusic, CloudMusic, PipeIntroMusic};

    a = M(OperMode); // if in title screen mode, leave
    if (a != 0)
    {
        // check for specific alternate mode of entry
        if (M(AltEntranceControl) != 0x02)
        {                              // from area object data header
            y = 0x05;                  // select music for pipe intro scene by default
            a = M(PlayerEntranceCtrl); // check value from level header for certain values
            if (a == 0x06)
            {
                goto StoreMusic; // load music for pipe intro scene if header
            }
            if (a == 0x07)
            {
                goto StoreMusic;
            }
        } // ChkAreaType: load area type as offset for music bit
        y = M(AreaType);
        if (M(CloudTypeOverride) == 0)
        {
            goto StoreMusic; // check for cloud type override
        }
        y = 0x04; // select music for cloud type level if found

    StoreMusic: // otherwise select appropriate music for level type
        a = MusicSelectData_data[y];
        writeData(AreaMusicQueue, a); // store in queue and leave
    } // ExitGetM
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none beyond the bool return
bool SMBEngine::TransposePlayers()
{
    bool endGame = false;

    endGame = true;         // end the game by default
    a = M(NumberOfPlayers); // if only a 1 player game, leave
    if (a == 0)
    {
        return endGame;
    }
    a = M(OffScr_NumberofLives); // does offscreen player have any lives left?
    if ((a & 0x80) != 0)
    {
        return endGame; // branch if not
    }
    // invert bit to update
    a = M(CurrentPlayer) ^ 0b00000001; // which player is on the screen
    writeData(CurrentPlayer, a);
    x = 0x06;

    do // TransLoop: transpose the information
    {
        a = M(OnscreenPlayerInfo + x);
        pha(); // of the onscreen player
        // with that of the offscreen player
        writeData(OnscreenPlayerInfo + x, M(OffscreenPlayerInfo + x));
        pla();
        writeData(OffscreenPlayerInfo + x, a);
        --x;
    } while ((x & 0x80) == 0);
    endGame = false; // get the game going

    return endGame; // ExTrans
}

//------------------------------------------------------------------------

// Inputs: a = signed offset to add to Player_Y_Position
// Outputs: none
void SMBEngine::MovePlayerYAxis(uint8_t amount)
{
    // add amount to player position
    writeData(Player_Y_Position, amount + M(Player_Y_Position));
}

//------------------------------------------------------------------------

// Inputs: slot = air bubble object buffer offset
// Outputs: none
void SMBEngine::DrawBubble(uint8_t slot)
{
    // if player's vertical high position not within screen, skip all of this
    if (M(Player_Y_HighPos) != 0x01)
    {
        return;
    }
    // check air bubble's offscreen bits
    if ((M(Bubble_OffscreenBits) & 0b00001000) != 0)
    {
        return; // if bit set, branch to leave
    }
    const uint8_t oamSlot = M(Bubble_SprDataOffset + slot); // get air bubble's OAM data offset
    // get relative horizontal coordinate
    writeData(Sprite_X_Position + oamSlot, M(Bubble_Rel_XPos)); // store as X coordinate here
    // get relative vertical coordinate
    writeData(Sprite_Y_Position + oamSlot, M(Bubble_Rel_YPos)); // store as Y coordinate here
    writeData(Sprite_Tilenumber + oamSlot, 0x74);               // put air bubble tile into OAM data
    writeData(Sprite_Attributes + oamSlot, 0x02);               // set attribute byte

    // ExDBub: leave
}

//------------------------------------------------------------------------

// Inputs: baseIdx = base graphics-table-offset adder
// Outputs: the same value, +8 for the small player
uint8_t SMBEngine::GetGfxOffsetAdder(uint8_t baseIdx)
{
    if (M(PlayerSize) == 0)
    {                   // get player's size
        return baseIdx; // if player big, use current offset as-is
    }
    return baseIdx + 0x08; // for small player, use current offset + 8
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ChkForPlayerAttrib()
{
    const uint8_t oamSlot = M(Player_SprDataOffset); // get sprite data offset

    // KilledAtt: whether the third row of sprites is modified too
    bool thirdRowToo = true;
    if (M(GameEngineSubroutine) != 0x0b)
    { // branch to change third and fourth row OAM attributes
        const uint8_t gfxOffset = M(PlayerGfxOffset); // get graphics table offset
        if (gfxOffset == 0x50 || gfxOffset == 0xb8 || gfxOffset == 0xc0)
        { // if crouch offset, either standing offset, go ahead and execute code to change
            // fourth row only
            thirdRowToo = false;
        }
        else if (gfxOffset != 0xc8)
        {
            return; // if none of these, branch to leave
        }
    }
    if (thirdRowToo)
    { // KilledAtt: mask out horizontal and vertical flip bits
        // for third row sprites and save
        writeData(Sprite_Attributes + 16 + oamSlot, M(Sprite_Attributes + 16 + oamSlot) & 0b00111111);
        // set horizontal flip bit for second sprite in the third row
        writeData(Sprite_Attributes + 20 + oamSlot, (M(Sprite_Attributes + 20 + oamSlot) & 0b00111111) | 0b01000000);
    }
    // C_S_IGAtt: mask out horizontal and vertical flip bits
    // for fourth row sprites and save
    writeData(Sprite_Attributes + 24 + oamSlot, M(Sprite_Attributes + 24 + oamSlot) & 0b00111111);
    // set horizontal flip bit for second sprite in the fourth row
    writeData(Sprite_Attributes + 28 + oamSlot, (M(Sprite_Attributes + 28 + oamSlot) & 0b00111111) | 0b01000000);

    // ExPlyrAt: leave
}

//------------------------------------------------------------------------

// Inputs: baseOffset = base offset; tableSelector = table selector (0/1/2 for fireball/bubble/misc)
// Outputs: baseOffset + ObjOffsetData_data[tableSelector]
uint8_t SMBEngine::GetProperObjOffset(uint8_t baseOffset, uint8_t tableSelector)
{
    const uint8_t ObjOffsetData_data[] = {0x07, 0x16, 0x0d};

    // add amount of bytes to offset depending on setting in the table selector
    return baseOffset + ObjOffsetData_data[tableSelector];
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ProcessWhirlpools()
{
    uint32_t wide = 0;

    a = M(AreaType); // check for water type level
    if (a != 0)
    {
        return; // branch to leave if not found
    }
    writeData(Whirlpool_Flag, a); // otherwise initialize whirlpool flag
    a = M(TimerControl);          // if master timer control set,
    if (a != 0)
    {
        return; // branch to leave
    }
    y = 0x04; // otherwise start with last whirlpool data

WhLoop:                                                                                               // get left extent of whirlpool
    wide = ((M(Whirlpool_PageLoc + y) << 8) | M(Whirlpool_LeftExtent + y)) + M(Whirlpool_Length + y); // add length of whirlpool
    writeData(0x02, LOBYTE(wide));                                                                    // store result as right extent here
    a = M(Whirlpool_PageLoc + y);                                                                     // get page location
    if (a == 0)
    {
        goto NextWh; // if none or page 0, branch to get next data
    }
    writeData(0x01, HIBYTE(wide)); // store result as page location of right extent here
    a = HIBYTE(wide);
    // the player and the left extent are each one 16-bit page:coordinate
    wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) -
           ((M(Whirlpool_PageLoc + y) << 8) | M(Whirlpool_LeftExtent + y)); // subtract left extent
    a = HIBYTE(wide);
    if ((a & 0x80) != 0)
    {
        goto NextWh; // if player too far left, branch to get next data
    }
    // the right extent and the player are each one 16-bit page:coordinate
    wide = ((M(0x01) << 8) | M(0x02))                           // otherwise get right extent
           - ((M(Player_PageLoc) << 8) | M(Player_X_Position)); // subtract player's horizontal coordinate
    a = HIBYTE(wide);
    if ((a & 0x80) != 0)
    { // if player within right extent, branch to whirlpool code

    NextWh: // move onto next whirlpool data
        --y;
        if ((y & 0x80) == 0)
        {
            goto WhLoop; // do this until all whirlpools are checked
        }

        return; // ExitWh: leave

        //------------------------------------------------------------------------
    } // WhirlpoolActivate
    // get length of whirlpool
    a = M(Whirlpool_Length + y) >> 1; // divide by 2
    writeData(0x00, a);               // save here
    // get left extent of whirlpool
    wide = ((M(Whirlpool_PageLoc + y) << 8) | M(Whirlpool_LeftExtent + y)) + M(0x00); // add length divided by 2
    writeData(0x01, LOBYTE(wide));                                                    // save as center of whirlpool
    writeData(0x00, HIBYTE(wide));                                                    // save as page location of whirlpool center
    a = HIBYTE(wide);                                                                 // get page location
    // get frame counter
    a = M(FrameCounter) >> 1; // check d0 (to run on every other frame)
    if ((M(FrameCounter) & 0x01) == 0)
    {
        goto WhPull; // if d0 not set, branch to last part of code
    }
    // the center and the player are each one 16-bit page:coordinate
    wide = ((M(0x00) << 8) | M(0x01))                           // get center
           - ((M(Player_PageLoc) << 8) | M(Player_X_Position)); // subtract player's horizontal coordinate
    a = HIBYTE(wide);
    if ((a & 0x80) != 0)
    {                                                                    // if player to the left of center, branch
        wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) - 0x01; // otherwise slowly pull player left, towards the center
        writeData(Player_X_Position, LOBYTE(wide));                      // set player's new horizontal coordinate
        a = HIBYTE(wide);
    } // LeftWh: get player's collision bits
    else // jump to set player's new page location
    {
        a = M(Player_CollisionBits) >> 1; // take d0
        if ((M(Player_CollisionBits) & 0x01) == 0)
        {
            goto WhPull; // if d0 not set, branch
        }
        wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + 0x01; // otherwise slowly pull player right, towards the center
        writeData(Player_X_Position, LOBYTE(wide));                      // set player's new horizontal coordinate
        a = HIBYTE(wide);
    } // SetPWh: set player's new page location
    writeData(Player_PageLoc, a);

WhPull:
    writeData(0x00, 0x10);           // set vertical movement force
    writeData(Whirlpool_Flag, 0x01); // set whirlpool flag to be used later
    writeData(0x02, 0x01);           // also set maximum vertical speed
    a = 0x00;
    x = 0x00;            // set X for player offset
    ImposeGravity(a, x); // jump to put whirlpool effect on player vertically, do not return
}

//------------------------------------------------------------------------

// Inputs: x = block object buffer offset (forwarded to WriteBlockMetatile)
// Outputs: none
void SMBEngine::ReplaceBlockMetatile()
{
    WriteBlockMetatile(a, x);   // write metatile to vram buffer to replace block object
    ++M(Block_ResidualCounter); // increment unused counter (residual code)
    --M(Block_RepFlag + x);     // decrement flag (residual code)
    // leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::LoadAreaPointer()
{
    // FindAreaPointer, inlined
    y = M(WorldNumber); // load offset from world variable
    a = M(WorldAddrOffsets + y);
    a += M(AreaNumber);
    y = a;
    a = M(AreaAddrOffsets + y); // from there we have our area pointer

    writeData(AreaPointer, a);

    GetAreaType(M(AreaPointer));
}

//------------------------------------------------------------------------

// mask out all but d6 and d5
// Inputs: areaPointerByte = area pointer byte
// Outputs: none (area type, 2 bits, stored to AreaType)
void SMBEngine::GetAreaType(uint8_t areaPointerByte)
{
    // make %0xx00000 into %000000xx and save 2 MSB as area type
    writeData(AreaType, (areaPointerByte & 0b01100000) >> 5);
}

//------------------------------------------------------------------------

// Inputs: a = raw palette bits to cycle in (only d1-d0 used)
// Outputs: none
void SMBEngine::CyclePlayerPalette()
{
    a &= 0x03;          // mask out all but d1-d0 (previously d3-d2)
    writeData(0x00, a); // store result here to use as palette bits
    // get player attributes
    a = M(Player_SprAttrib) & 0b11111100; // save any other bits but palette bits
    a |= M(0x00);                         // add palette bits
    writeData(Player_SprAttrib, a);       // store as new player attributes
                                          // and leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ResetPalStar()
{
    // get player attributes
    a = M(Player_SprAttrib) & 0b11111100; // mask out palette bits to force palette 0
    writeData(Player_SprAttrib, a);       // store as new player attributes
                                          // and leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::BlockObjMT_Updater()
{
    x = 0x01; // set offset to start with second block object

    do // UpdateLoop: set offset here
    {
        writeData(ObjectOffset, x);
        a = M(VRAM_Buffer1); // if vram buffer already being used here,
        if (a != 0)
        {
            goto NextBUpd; // branch to move onto next block object
        }
        a = M(Block_RepFlag + x); // if flag for block object already clear,
        if (a == 0)
        {
            goto NextBUpd; // branch to move onto next block object
        }
        // get low byte of block buffer
        writeData(0x06, M(Block_BBuf_Low + x)); // store into block buffer address
        writeData(0x07, 0x05);                  // set high byte of block buffer address
        a = M(Block_Orig_YPos + x);             // get original vertical coordinate of block object
        writeData(0x02, a);                     // store here and use as offset to block buffer
        y = a;
        a = M(Block_Metatile + x); // get metatile to be written
        writeData(W(0x06) + y, a); // write it to the block buffer
        ReplaceBlockMetatile();    // do sub to replace metatile where block object is
        a = 0x00;
        writeData(Block_RepFlag + x, 0x00); // clear block object flag

    NextBUpd: // decrement block object offset
        --x;
    } while ((x & 0x80) == 0); // do this until both block objects are dealt with
    // then leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none (delegates to Skip_6 with y = 0x01)
void SMBEngine::ImposeGravityBlock()
{
    // set offset for maximum speed; x is the block object offset
    Skip_6(0x01, x);
}

//------------------------------------------------------------------------

// Inputs: maxSpeedIdx = index into MaxSpdBlockData_data (0 or 1); objectOffset = object buffer
// offset forwarded to ImposeGravitySprObj
// Outputs: none
void SMBEngine::Skip_6(uint8_t maxSpeedIdx, uint8_t objectOffset)
{
    const uint8_t MaxSpdBlockData_data[] = {0x06, 0x08};

    // set movement amount here
    writeData(0x00, 0x50);

    ImposeGravitySprObj(MaxSpdBlockData_data[maxSpeedIdx], objectOffset); // get maximum speed
}

//------------------------------------------------------------------------

// Inputs: x = fireball object buffer offset
// Outputs: a = the metatile found beneath the fireball, forwarded from BlockBufferCollision via
// ResJmpM/BBChk_E
void SMBEngine::BlockBufferChk_FBall()
{
    // x = fireball object buffer offset; add seven bytes to use, and 0x1a is the block buffer
    // adder data offset
    a = ResJmpM((uint8_t)(x + 0x07), 0x1a);
}

//------------------------------------------------------------------------

// Inputs: objectOffset = sprite object offset; cornerIdx = corner-selector index (forwarded to
// BBChk_E/BlockBufferCollision)
// Outputs: return value = the metatile found (see BlockBufferCollision); not every caller reads it
// (e.g. BlockBufferChk_Enemy's chain ignores it), but BlockBufferChk_FBall's caller does
uint8_t SMBEngine::ResJmpM(uint8_t objectOffset, uint8_t cornerIdx)
{
    return BBChk_E(0x00, objectOffset, cornerIdx);
}

//------------------------------------------------------------------------

// Inputs: slot = fireball object buffer offset
// Outputs: none
void SMBEngine::DrawFireball(uint8_t slot)
{
    const uint8_t oamSlot = M(FBall_SprDataOffset + slot); // get fireball's sprite data offset
    // get relative vertical coordinate
    writeData(Sprite_Y_Position + oamSlot, M(Fireball_Rel_YPos)); // store as sprite Y coordinate
    // get relative horizontal coordinate
    writeData(Sprite_X_Position + oamSlot, M(Fireball_Rel_XPos)); // store as sprite X coordinate, then do shared code

    DrawFirebar(oamSlot);
}

//------------------------------------------------------------------------

// Inputs: rows = number of sprite rows to draw
// Outputs: none (delegates to DrawPlayerLoop)
void SMBEngine::RenderPlayerSub(uint8_t rows)
{
    writeData(0x07, rows); // store number of rows of sprites to draw
    const uint8_t relXPos = M(Player_Rel_XPos);
    writeData(Player_Pos_ForScroll, relXPos); // store player's relative horizontal position
    writeData(0x05, relXPos);                 // store it here also
    writeData(0x02, M(Player_Rel_YPos));      // store player's vertical position
    writeData(0x03, M(PlayerFacingDir));      // store player's facing direction
    writeData(0x04, M(Player_SprAttrib));     // store player's sprite attributes
    // hand the graphics table offset and the player's sprite data offset to DrawPlayerLoop
    DrawPlayerLoop(M(PlayerGfxOffset), M(Player_SprDataOffset));
}

//------------------------------------------------------------------------

// Inputs: gfxOffset = player graphics table offset; sprDataOffset = player sprite data offset
// (forwarded to DrawOneSpriteRow)
// Outputs: none
void SMBEngine::DrawPlayerLoop(uint8_t gfxOffset, uint8_t sprDataOffset)
{
    uint8_t spritePairIdx = gfxOffset;
    uint8_t oamSlot = sprDataOffset;

    do // DrawPlayerLoop: load player's left side
    {
        writeData(0x00, M(PlayerGraphicsTable + spritePairIdx));
        // now load right side
        std::tie(spritePairIdx, oamSlot) =
            DrawOneSpriteRow(M(PlayerGraphicsTable + 1 + spritePairIdx), spritePairIdx, oamSlot);
        --M(0x07);              // decrement rows of sprites to draw
    } while (M(0x07) != 0);     // do this until all rows are drawn
}

//------------------------------------------------------------------------

// Inputs: baseIdx = graphics-table-offset base index (forwarded to GetOffsetFromAnimCtrl)
// Outputs: offset to graphics table (see GetOffsetFromAnimCtrl)
uint8_t SMBEngine::GetCurrentAnimOffset(uint8_t baseIdx)
{
    // get animation frame control and jump to get proper offset to graphics table
    return GetOffsetFromAnimCtrl(M(PlayerAnimCtrl), baseIdx);
}

//------------------------------------------------------------------------

// Inputs: baseIdx = graphics-table-offset base index (forwarded through AnimationControl to
// GetCurrentAnimOffset/GetOffsetFromAnimCtrl)
// Outputs: offset to graphics table (see AnimationControl)
uint8_t SMBEngine::ThreeFrameExtent(uint8_t baseIdx)
{
    // load upper extent for frame control for climbing
    return AnimationControl(0x02, baseIdx);
}

//------------------------------------------------------------------------

// Inputs: upperExtent = upper extent for animation frame control; baseIdx = graphics-table-offset
// base index (forwarded to GetCurrentAnimOffset/GetOffsetFromAnimCtrl)
// Outputs: offset to graphics table (from GetCurrentAnimOffset)
uint8_t SMBEngine::AnimationControl(uint8_t upperExtent, uint8_t baseIdx)
{
    writeData(0x00, upperExtent); // store upper extent here
    // get proper offset to graphics table
    const uint8_t gfxOffset = GetCurrentAnimOffset(baseIdx);
    // load animation frame timer
    if (M(PlayerAnimTimer) == 0)
    { // branch if not expired
        // get animation frame timer amount
        writeData(PlayerAnimTimer, M(PlayerAnimTimerSet)); // and set timer accordingly
        uint8_t frameCtrl = M(PlayerAnimCtrl) + 0x01;
        if (frameCtrl >= M(0x00))
        {                     // if frame control + 1 < upper extent, use as next
            frameCtrl = 0x00; // otherwise initialize frame control
        } // SetAnimC: store as new animation frame control
        writeData(PlayerAnimCtrl, frameCtrl);
    } // ExAnimC: leave with the offset to the graphics table
    return gfxOffset;
}

//------------------------------------------------------------------------

// Inputs: frameCtrl = animation frame control value; baseIdx = graphics-table-offset base index
// Outputs: offset to graphics table (frameCtrl*8 + PlayerGfxTblOffsets[baseIdx])
uint8_t SMBEngine::GetOffsetFromAnimCtrl(uint8_t frameCtrl, uint8_t baseIdx)
{
    // multiply animation frame control by eight and add to our offset to the
    // graphics table
    return (uint8_t)((uint8_t)(frameCtrl << 3) + M(PlayerGfxTblOffsets + baseIdx));
}

//------------------------------------------------------------------------

// Inputs: slot = air bubble object buffer offset
// Outputs: none
void SMBEngine::RelativeBubblePosition(uint8_t slot)
{
    // modify slot to get proper air bubble offset, then get the coordinates
    RelWOfs(GetProperObjOffset(slot, 0x01), 0x03);
}

//------------------------------------------------------------------------

// Inputs: slot = fireball object buffer offset
// Outputs: none
void SMBEngine::RelativeFireballPosition(uint8_t slot)
{
    // modify slot to get proper fireball offset, then get the coordinates
    RelWOfs(GetProperObjOffset(slot, 0x00), 0x02);
}

//------------------------------------------------------------------------

// Inputs: slot = misc object buffer offset
// Outputs: none
void SMBEngine::RelativeMiscPosition(uint8_t slot)
{
    // modify slot to get proper misc object offset, then get the coordinates
    RelWOfs(GetProperObjOffset(slot, 0x02), 0x06);
}

//------------------------------------------------------------------------

// Inputs: slot = block object buffer offset (adjusted internally for the second block object)
// Outputs: none
void SMBEngine::RelativeBlockPosition(uint8_t slot)
{
    // get coordinates of one block object relative to the screen
    VariableObjOfsRelPos(0x09, slot, 0x04);
    // adjust offset for other block object if any, adjust other and get coordinates
    // for other one
    VariableObjOfsRelPos(0x09, slot + 0x02, 0x05);
}

//------------------------------------------------------------------------

// Inputs: slot = fireball object buffer offset
// Outputs: none
void SMBEngine::GetFireballOffscreenBits(uint8_t slot)
{
    // modify slot to get proper fireball offset, then get offscreen information
    // about fireball
    GetOffScreenBitsSet(GetProperObjOffset(slot, 0x00), 0x02);
}

//------------------------------------------------------------------------

// Inputs: slot = air bubble object buffer offset
// Outputs: none
void SMBEngine::GetBubbleOffscreenBits(uint8_t slot)
{
    // modify slot to get proper air bubble offset, then get offscreen information
    // about air bubble
    GetOffScreenBitsSet(GetProperObjOffset(slot, 0x01), 0x03);
}

//------------------------------------------------------------------------

// Inputs: slot = misc object buffer offset
// Outputs: none
void SMBEngine::GetMiscOffscreenBits(uint8_t slot)
{
    // modify slot to get proper misc object offset, then get offscreen information
    // about misc object
    GetOffScreenBitsSet(GetProperObjOffset(slot, 0x02), 0x06);
}

//------------------------------------------------------------------------

// Inputs: slot = block object buffer offset
// Outputs: none
void SMBEngine::GetBlockOffscreenBits(uint8_t slot)
{
    // add 9 bytes in order to get block obj offset, and put offscreen bits in
    // Block_OffscreenBits
    SetOffscrBitsOffset(0x09, slot, 0x04);
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: offset to graphics table (see GetOffsetFromAnimCtrl)
uint8_t SMBEngine::HandleChangeSize()
{
    const uint8_t ChangeSizeOffsetAdder_data[] = {0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x02, 0x00, 0x01, 0x02,
                                                  0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00};

    uint8_t frameCtrl = M(PlayerAnimCtrl); // get animation frame control
    if ((M(FrameCounter) & 0b00000011) == 0)
    {                // get frame counter and execute this code every fourth frame
        ++frameCtrl; // increment frame control
        if (frameCtrl >= 0x0a)
        {                                          // if not there yet, skip ahead to use
            frameCtrl = 0x00;                      // otherwise initialize both grow/shrink flag
            writeData(PlayerChangeSizeFlag, 0x00); // and animation frame control
        } // CSzNext: store proper frame control
        writeData(PlayerAnimCtrl, frameCtrl);
    } // GorSLog: get player's size
    if (M(PlayerSize) == 0)
    { // if player big, get offset adder based on frame control as offset,
        // and use offset for player growing
        return GetOffsetFromAnimCtrl(ChangeSizeOffsetAdder_data[frameCtrl], 0x0f);

        //------------------------------------------------------------------------
    } // ShrinkPlayer
    // add ten bytes to frame control as offset; this thing apparently uses two of the
    // swimming frames to draw the player shrinking
    const uint8_t shrinkIdx = frameCtrl + 0x0a;
    // load offset for small player swimming, or if what would normally be the offset
    // adder is zero, load offset for big player swimming instead
    const uint8_t baseIdx = ChangeSizeOffsetAdder_data[shrinkIdx] != 0 ? 0x09 : 0x01;
    // ShrPlF: get offset to graphics table based on offset loaded and leave
    return M(PlayerGfxTblOffsets + baseIdx);
}

//------------------------------------------------------------------------

// Inputs: x = fireball object buffer offset
// Outputs: none
void SMBEngine::FireballBGCollision()
{
    // check fireball's vertical coordinate
    if (M(Fireball_Y_Position + x) < 0x18)
    {
        goto ClearBounceFlag; // if within the status bar area of the screen, branch ahead
    }
    BlockBufferChk_FBall(); // do fireball to background collision detection on bottom of it
    if (a == 0)
    {
        goto ClearBounceFlag; // if nothing underneath fireball, branch
    }
    if (ChkForNonSolids(a))
    {                         // check for non-solid metatiles
        goto ClearBounceFlag; // branch if any found
    }
    // if fireball's vertical speed set to move upwards,
    if ((M(Fireball_Y_Speed + x) & 0x80) != 0)
    {
        goto InitFireballExplode; // branch to set exploding bit in fireball's state
    }
    // if bouncing flag already set,
    if (M(FireballBouncingFlag + x) != 0)
    {
        goto InitFireballExplode; // branch to set exploding bit in fireball's state
    }
    writeData(Fireball_Y_Speed + x, 0xfd);     // otherwise set vertical speed to move upwards (give it bounce)
    writeData(FireballBouncingFlag + x, 0x01); // set bouncing flag
    a = M(Fireball_Y_Position + x) & 0xf8;     // modify vertical coordinate to land it properly
    writeData(Fireball_Y_Position + x, a);     // store as new vertical coordinate
    return;                                    // leave

    //------------------------------------------------------------------------

ClearBounceFlag:
    a = 0x00;
    writeData(FireballBouncingFlag + x, 0x00); // clear bouncing flag by default
    return;                                    // leave

    //------------------------------------------------------------------------

InitFireballExplode:
    writeData(Fireball_State + x, 0x80); // set exploding flag in fireball's state
    a = Sfx_Bump;
    writeData(Square1SoundQueue, Sfx_Bump); // load bump sound
    // leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: offset to graphics table, consumed by the caller (FindPlayerAction) as the graphics
// table offset for PlayerGfxProcessing
uint8_t SMBEngine::ProcessPlayerAction()
{
    // NonAnimatedActs: get offset adder for graphics table, initialize animation frame
    // control, and load offset to graphics table using size as offset
    auto nonAnimatedActs = [&](uint8_t baseIdx) -> uint8_t {
        const uint8_t idx = GetGfxOffsetAdder(baseIdx);
        writeData(PlayerAnimCtrl, 0x00);
        return M(PlayerGfxTblOffsets + idx);
    };

    // FourFrameExtent: load upper extent for frame control, get offset and animate
    // player object
    auto fourFrameExtent = [&](uint8_t idx) -> uint8_t { return AnimationControl(0x03, idx); };

    const uint8_t state = M(Player_State); // get player's state
    if (state == 0x03)
    { // ActionClimbing: load offset for climbing
        // check player's vertical speed
        if (M(Player_Y_Speed) == 0)
        {
            return nonAnimatedActs(0x05); // if no speed, use offset as-is
        }
        // otherwise get offset for graphics table, then skip ahead to more code
        return ThreeFrameExtent(GetGfxOffsetAdder(0x05));
    }
    if (state == 0x02)
    { // ActionFalling: load offset for walking/running, get offset to graphics table,
        // and execute instructions for falling state
        return GetCurrentAnimOffset(GetGfxOffsetAdder(0x04));
    }
    if (state == 0x01)
    { // jumping
        if (M(SwimmingFlag) != 0)
        { // ActionSwimming: load offset for swimming
            const uint8_t idx = GetGfxOffsetAdder(0x01);
            // check jump/swim timer and animation frame control
            if ((M(JumpSwimTimer) | M(PlayerAnimCtrl)) != 0)
            {
                return fourFrameExtent(idx); // if any one of these set, branch ahead
            }
            if ((M(A_B_Buttons) & 0x80) != 0)
            { // check for A button pressed
                return fourFrameExtent(idx); // branch to same place if A button pressed
            }
            return GetCurrentAnimOffset(idx);
        }
        // get crouching flag
        if (M(CrouchingFlag) != 0)
        {
            return nonAnimatedActs(0x06); // if set, load offset for crouching
        }
        return nonAnimatedActs(0x00); // otherwise load offset for jumping
    }
    // ProcOnGroundActs: get crouching flag
    if (M(CrouchingFlag) != 0)
    {
        return nonAnimatedActs(0x06); // if set, load offset for crouching
    }
    // check player's horizontal speed and left/right controller bits
    if ((M(Player_X_Speed) | M(Left_Right_Buttons)) == 0)
    {
        return nonAnimatedActs(0x02); // if no speed or buttons pressed, use standing offset
    }
    // load walking/running speed
    if (M(Player_XSpeedAbsolute) >= 0x09)
    { // if fast enough to skid, check to see if moving direction
        // and facing direction are the same
        if ((M(Player_MovingDir) & M(PlayerFacingDir)) == 0)
        { // if moving direction <> facing direction, skid
            return nonAnimatedActs(0x03); // load skid offset ($03)
        }
    }
    // ActionWalkRun: load offset for walking/running, get offset to graphics table,
    // and execute instructions for normal state
    return fourFrameExtent(GetGfxOffsetAdder(0x04));
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::DonePlayerTask()
{
    writeData(TimerControl, 0x00); // initialize master timer control to continue timers
    a = 0x08;
    writeData(GameEngineSubroutine, 0x08); // set player control routine to run next frame
                                           // leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerFireFlower()
{
    // check master timer control
    if (M(TimerControl) != 0xc0)
    { // branch if at moment, not before or after
        // get frame counter
        a = M(FrameCounter) >> 2; // divide by four to change every four frames

        CyclePlayerPalette();
        return;

        //------------------------------------------------------------------------
    } // ResetPalFireFlower
    DonePlayerTask(); // do sub to init timer control and run player control routine

    ResetPalStar();
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset
// Outputs: x = M(ObjectOffset) (restored enemy object offset)
void SMBEngine::FloateyNumbersRoutine()
{
    const uint8_t ScoreUpdateData_data[] = {0xff, // dummy
                                            0x41, 0x42, 0x44, 0x45, 0x48, 0x31, 0x32, 0x34, 0x35, 0x38, 0x00};

    const uint8_t FloateyNumTileData_data[] = {
        0xff, 0xff, // dummy
        0xf6, 0xfb, //  "100"
        0xf7, 0xfb, //  "200"
        0xf8, 0xfb, //  "400"
        0xf9, 0xfb, //  "500"
        0xfa, 0xfb, //  "800"
        0xf6, 0x50, //  "1000"
        0xf7, 0x50, //  "2000"
        0xf8, 0x50, //  "4000"
        0xf9, 0x50, //  "5000"
        0xfa, 0x50, //  "8000"
        0xfd, 0xfe  //  "1-UP"
    };

    bool borrow = false;

    a = M(FloateyNum_Control + x); // load control for floatey number
    if (a == 0)
    {
        return; // if zero, branch to leave
    }
    if (a >= 0x0b)
    {
        a = 0x0b;                                // otherwise set to $0b, thus keeping
        writeData(FloateyNum_Control + x, 0x0b); // it in range
    } // ChkNumTimer: use as Y
    y = a;
    a = M(FloateyNum_Timer + x); // check value here
    if (a == 0)
    {                                         // if nonzero, branch ahead
        writeData(FloateyNum_Control + x, a); // initialize floatey number control and leave
        return;

        //------------------------------------------------------------------------
    } // DecNumTimer: decrement value here
    --M(FloateyNum_Timer + x);
    if (a == 0x2b)
    {
        if (y == 0x0b)
        {                       // branch ahead if not found
            ++M(NumberofLives); // give player one extra life (1-up)
            a = Sfx_ExtraLife;
            writeData(Square2SoundQueue, Sfx_ExtraLife); // and play the 1-up sound
        } // LoadNumTiles: load point value here
        a = ScoreUpdateData_data[y] >> 4; // move high nybble to low
        x = a;                            // use as X offset, essentially the digit
        // load again and this time
        a = ScoreUpdateData_data[y] & 0b00001111; // mask out the high nybble
        writeData(DigitModifier + x, a);          // store as amount to add to the digit
        AddToScore();                             // update the score accordingly
    } // ChkTallEnemy: get OAM data offset for enemy object
    y = M(Enemy_SprDataOffset + x);
    a = M(Enemy_ID + x); // get enemy object identifier
    if (a == Spiny)
    {
        goto FloateyPart; // branch if spiny
    }
    if (a == PiranhaPlant)
    {
        goto FloateyPart; // branch if piranha plant
    }
    if (a == HammerBro)
    {
        goto GetAltOffset; // branch elsewhere if hammer bro
    }
    if (a == GreyCheepCheep)
    {
        goto FloateyPart; // branch if cheep-cheep of either color
    }
    if (a == RedCheepCheep)
    {
        goto FloateyPart;
    }
    if (a >= TallEnemy)
    {
        goto GetAltOffset; // branch elsewhere if enemy object => $09
    }
    if (M(Enemy_State + x) >= 0x02)
    {
        goto FloateyPart; // $02 or greater, branch beyond this part
    }

GetAltOffset: // load some kind of control bit
    x = M(SprDataOffset_Ctrl);
    y = M(Alt_SprDataOffset + x); // get alternate OAM data offset
    x = M(ObjectOffset);          // get enemy object offset again

FloateyPart: // get vertical coordinate for
    a = M(FloateyNum_Y_Pos + x);
    borrow = a < 0x18; // the compare's borrow is still live at the subtract below
    if (a >= 0x18)
    { // status bar, branch
        --a;
        writeData(FloateyNum_Y_Pos + x, a); // otherwise subtract one and store as new
    } // SetupNumSpr: get vertical coordinate
    a = (uint8_t)(M(FloateyNum_Y_Pos + x) - 0x08 - (borrow ? 1 : 0)); // subtract eight (and the borrow) and dump into the
    DumpTwoSpr(a, y);                                                 // left and right sprite's Y coordinates
    a = M(FloateyNum_X_Pos + x);                                      // get horizontal coordinate
    writeData(Sprite_X_Position + y, a);                              // store into X coordinate of left sprite
    a += 0x08;                                                        // add eight pixels and store into X
    writeData(Sprite_X_Position + 4 + y, a);                          // coordinate of right sprite
    writeData(Sprite_Attributes + y, 0x02);                           // set palette control in attribute bytes
    writeData(Sprite_Attributes + 4 + y, 0x02);                       // of left and right sprites
    a = M(FloateyNum_Control + x);
    a <<= 1;                                                      // multiply our floatey number control by 2
    x = a;                                                        // and use as offset for look-up table
    writeData(Sprite_Tilenumber + y, FloateyNumTileData_data[x]); // display first half of number of points
    a = FloateyNumTileData_data[1 + x];
    writeData(Sprite_Tilenumber + 4 + y, a); // display the second half
    x = M(ObjectOffset);                     // get enemy object offset and leave
}

//------------------------------------------------------------------------

// Inputs: x = misc object buffer offset
// Outputs: x = M(ObjectOffset) (restored misc object offset)
void SMBEngine::DrawHammer()
{
    const uint8_t HammerSprAttrib_data[] = {0x03, 0x03, 0xc3, 0xc3};

    const uint8_t SecondSprTilenum_data[] = {0x81, 0x83, 0x80, 0x82};

    const uint8_t FirstSprTilenum_data[] = {0x80, 0x82, 0x81, 0x83};

    const uint8_t SecondSprYPos_data[] = {0x08, 0x00, 0x08, 0x00};

    const uint8_t SecondSprXPos_data[] = {0x00, 0x08, 0x00, 0x08};

    const uint8_t FirstSprYPos_data[] = {0x00, 0x04, 0x00, 0x04};

    const uint8_t FirstSprXPos_data[] = {0x04, 0x00, 0x04, 0x00};

    y = M(Misc_SprDataOffset + x); // get misc object OAM data offset
    if (M(TimerControl) == 0)
    { // if master timer control set, skip this part
        // otherwise get hammer's state
        a = M(Misc_State + x) & 0b01111111; // mask out d7
        if (a == 0x01)
        {
            goto GetHPose; // if so, branch
        }
    } // ForceHPose: reset offset here
    x = 0x00;
    if (x != 0)
    { // do unconditional branch to rendering part

    GetHPose:                     // get frame counter
        a = M(FrameCounter) >> 2; // move d3-d2 to d1-d0
        a &= 0b00000011;          // mask out all but d1-d0 (changes every four frames)
        x = a;                    // use as timing offset
    } // RenderH: get relative vertical coordinate
    a = M(Misc_Rel_YPos);
    a += FirstSprYPos_data[x];                                      // add first sprite vertical adder based on offset
    writeData(Sprite_Y_Position + y, a);                            // store as sprite Y coordinate for first sprite
    a += SecondSprYPos_data[x];                                     // add second sprite vertical adder based on offset
    writeData(Sprite_Y_Position + 4 + y, a);                        // store as sprite Y coordinate for second sprite
    a = M(Misc_Rel_XPos);                                           // get relative horizontal coordinate
    a += FirstSprXPos_data[x];                                      // add first sprite horizontal adder based on offset
    writeData(Sprite_X_Position + y, a);                            // store as sprite X coordinate for first sprite
    a += SecondSprXPos_data[x];                                     // add second sprite horizontal adder based on offset
    writeData(Sprite_X_Position + 4 + y, a);                        // store as sprite X coordinate for second sprite
    writeData(Sprite_Tilenumber + y, FirstSprTilenum_data[x]);      // get and store tile number of first sprite
    writeData(Sprite_Tilenumber + 4 + y, SecondSprTilenum_data[x]); // get and store tile number of second sprite
    a = HammerSprAttrib_data[x];
    writeData(Sprite_Attributes + y, a);     // get and store attribute bytes for both
    writeData(Sprite_Attributes + 4 + y, a); // note in this case they use the same data
    x = M(ObjectOffset);                     // get misc object offset
    a = M(Misc_OffscreenBits) & 0b11111100;  // check offscreen bits
    if (a != 0)
    {                                    // if all bits clear, leave object alone
        writeData(Misc_State + x, 0x00); // otherwise nullify misc object state
        a = 0xf8;
        DumpTwoSpr(a, y); // do sub to move hammer sprites offscreen
    } // NoHOffscr: leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::FindPlayerAction()
{
    // find proper offset to graphics table by player's actions,
    // then draw player and process for fireball throwing
    PlayerGfxProcessing(ProcessPlayerAction());
}

//------------------------------------------------------------------------

// Inputs: gfxOffset = graphics table offset to render
// Outputs: none
void SMBEngine::PlayerGfxProcessing(uint8_t gfxOffset)
{
    writeData(PlayerGfxOffset, gfxOffset); // store offset to graphics table here
    RenderPlayerSub(0x04);                 // draw player based on offset loaded
    ChkForPlayerAttrib();                  // set horizontal flip bits as necessary
    if (M(FireballThrowingTimer) == 0)
    {
        PlayerOffscreenChk(); // if fireball throw timer not set, skip to the end
        return;
    }
    const uint8_t animTimer = M(PlayerAnimTimer); // get animation frame timer
    if (animTimer >= M(FireballThrowingTimer))
    {
        writeData(FireballThrowingTimer, 0x00); // initialize fireball throw timer
        PlayerOffscreenChk();                   // if animation frame timer => fireball throw timer skip to end
        return;
    }
    writeData(FireballThrowingTimer, animTimer); // otherwise store animation timer into fireball throw timer
    // load offset for throwing
    // get offset to graphics table
    writeData(PlayerGfxOffset, M(PlayerGfxTblOffsets + 0x07)); // store it for use later
    uint8_t rows = 0x04;                                       // set to update four sprite rows by default
    if ((M(Player_X_Speed) | M(Left_Right_Buttons)) != 0)
    {                // check for horizontal speed or left/right button press
        rows = 0x03; // otherwise set to update only three sprite rows
    } // SUpdR: draw player object again
    RenderPlayerSub(rows);
    PlayerOffscreenChk();
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerOffscreenChk()
{
    // get player's offscreen bits
    writeData(0x00, M(Player_OffscreenBits) >> 4); // move vertical bits to low nybble and store here
    // get player's sprite data offset and add 24 bytes to start at bottom row
    uint8_t oamSlot = M(Player_SprDataOffset) + 0x18;
    uint8_t row = 0x03; // check all four rows of player sprites

    do // PROfsLoop
    {
        const bool offscreen = (M(0x00) & 0x01) != 0; // take the bit
        M(0x00) >>= 1;
        if (offscreen)
        { // if bit set, dump offscreen Y coordinate into sprite data
            DumpTwoSpr(0xf8, oamSlot);
        } // NPROffscr
        oamSlot -= 0x08;           // next row up
        --row;                     // decrement row counter
    } while ((row & 0x80) == 0);   // do this until all sprite rows are checked
    // then we are done!
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerGfxHandler()
{
    const uint8_t SwimKickTileNum_data[] = {0x31, 0x46};

    // if player's injured invincibility timer
    if (M(InjuryTimer) != 0)
    { // not set, skip checkpoint and continue code
        if ((M(FrameCounter) & 0x01) != 0)
        {
            return; // otherwise leave on every other frame (when d0 is set)
        }
    } // CntPl: if executing specific game engine routine,
    if (M(GameEngineSubroutine) == 0x0b)
    { // PlayerKilled: use offset for player killed
        PlayerGfxProcessing(M(PlayerGfxTblOffsets + 0x0e));
        return;
    }
    // if grow/shrink flag set
    if (M(PlayerChangeSizeFlag) != 0)
    { // DoChangeSize: find proper offset to graphics table for grow/shrink,
        // then draw player and process for fireball throwing
        PlayerGfxProcessing(HandleChangeSize());
        return;
    }
    // if swimming flag set and the player is not on the ground, animate the
    // player's feet on top of the usual processing
    if (M(SwimmingFlag) == 0 || M(Player_State) == 0x00)
    {
        FindPlayerAction(); // branch and do not return
        return;
    }
    FindPlayerAction(); // otherwise jump and return
    // check frame counter for d2 set (8 frames every eighth frame)
    if ((M(FrameCounter) & 0b00000100) != 0)
    {
        return; // and branch if set to leave
    }
    uint8_t tileIdx = 0x00;                   // initialize tile selector to zero
    uint8_t oamSlot = M(Player_SprDataOffset); // get player sprite data offset
    if ((M(PlayerFacingDir) & 0x01) == 0)     // get player's facing direction
    {                  // if player facing to the right, use current offset
        oamSlot += 0x04; // otherwise move to next OAM data
    } // SwimKT: check player's size
    if (M(PlayerSize) != 0)
    { // if big, use first tile
        // check tile number of seventh/eighth sprite
        if (M(Sprite_Tilenumber + 24 + oamSlot) == M(SwimTileRepOffset))
        {
            return; // if spr7/spr8 tile number = value, branch to leave
        }
        ++tileIdx; // otherwise increment tile selector for second tile
    } // BigKTS: overwrite tile number in sprite 7/8
    // to animate player's feet when swimming
    writeData(Sprite_Tilenumber + 24 + oamSlot, SwimKickTileNum_data[tileIdx]);

    // ExPGH: then leave
}

//------------------------------------------------------------------------

// Inputs: x = block object buffer offset
// Outputs: none
void SMBEngine::BlockObjectsCore()
{
    a = M(Block_State + x); // get state of block object
    if (a == 0)
    {
        goto UpdSte; // if not set, branch to leave
    }
    a &= 0x0f; // mask out high nybble
    pha();     // push to stack
    y = a;     // put in Y for now
    a = x;
    a += 0x09; // add 9 bytes to offset (note two block objects are created
    x = a;     // when using brick chunks, but only one offset for both)
    --y;       // decrement Y to check for solid block state
    if (y != 0)
    {                              // branch if found, otherwise continue for brick chunks
        ImposeGravityBlock();      // do sub to impose gravity on one block object object
        MoveObjectHorizontally(x); // do another sub to move horizontally
        a = x;
        a += 0x02;
        x = a;
        ImposeGravityBlock();       // do sub to impose gravity on other block object
        MoveObjectHorizontally(x);  // do another sub to move horizontally
        x = M(ObjectOffset);        // get block object offset used for both
        RelativeBlockPosition(x);   // get relative coordinates
        GetBlockOffscreenBits(x);   // get offscreen information
        DrawBrickChunks();          // draw the brick chunks
        pla();                      // get lower nybble of saved state
        y = M(Block_Y_HighPos + x); // check vertical high byte of block object
        if (y == 0)
        {
            goto UpdSte; // if above the screen, branch to kill it
        }
        pha(); // otherwise save state back into stack
        if (0xf0 < M(Block_Y_Position + 2 + x))
        {                                              // to the bottom of the screen, and branch if not
            writeData(Block_Y_Position + 2 + x, 0xf0); // otherwise set offscreen coordinate
        } // ChkTop: get top block object's vertical coordinate
        a = M(Block_Y_Position + x);
        pla(); // pull block object state from stack
        if (M(Block_Y_Position + x) < 0xf0)
        {
            goto UpdSte; // if not, branch to save state
        }
        if (M(Block_Y_Position + x) >= 0xf0)
        {
            goto KillBlock; // otherwise do unconditional branch to kill it
        }
    } // BouncingBlockHandler
    ImposeGravityBlock();    // do sub to impose gravity on block object
    x = M(ObjectOffset);     // get block object offset
    RelativeBlockPosition(x); // get relative coordinates
    GetBlockOffscreenBits(x); // get offscreen information
    DrawBlock();             // draw the block
    // get vertical coordinate
    a = M(Block_Y_Position + x) & 0x0f; // mask out high nybble
    pla();                              // pull state from stack
    if ((M(Block_Y_Position + x) & 0x0f) >= 0x05)
    {
        goto UpdSte; // if still above amount, not time to kill block yet, thus branch
    }
    a = 0x01;
    writeData(Block_RepFlag + x, 0x01); // otherwise set flag to replace metatile

KillBlock: // if branched here, nullify object state
    a = 0x00;

UpdSte: // store contents of A in block object state
    writeData(Block_State + x, a);
}

//------------------------------------------------------------------------

// Inputs: x = block object buffer offset
// Outputs: none
void SMBEngine::DrawBlock()
{
    const uint8_t DefaultBlockObjTiles_data[] = {
        0x85, 0x85, 0x86, 0x86 // brick w/ line (these are sprite tiles, not BG!)
    };

    // get relative vertical coordinate of block object
    writeData(0x02, M(Block_Rel_YPos)); // store here
    // get relative horizontal coordinate of block object
    writeData(0x05, M(Block_Rel_XPos)); // store here
    writeData(0x04, 0x03);              // set attribute byte here
    a = 0x01;
    writeData(0x03, 0x01);          // set horizontal flip bit here (will not be used)
    y = M(Block_SprDataOffset + x); // get sprite data offset
    x = 0x00;                       // reset X for use as offset to tile data

    do // DBlkLoop: get left tile number
    {
        writeData(0x00, DefaultBlockObjTiles_data[x]); // set here
        a = DefaultBlockObjTiles_data[1 + x];          // get right tile number
        DrawOneSpriteRow(a, x, y);                     // do sub to write tile numbers to first row of sprites
    } while (x != 0x04); // and loop back until all four sprites are done
    x = M(ObjectOffset);            // get block object offset
    y = M(Block_SprDataOffset + x); // get sprite data offset
    if (M(AreaType) != 0x01)
    { // if found, branch to next part
        a = 0x86;
        writeData(Sprite_Tilenumber + y, 0x86);     // otherwise remove brick tiles with lines
        writeData(Sprite_Tilenumber + 4 + y, 0x86); // and replace then with lineless brick tiles
    } // ChkRep: check replacement metatile
    if (M(Block_Metatile + x) == 0xc4)
    {                      // branch ahead to use current graphics
        a = 0x87;          // set A for used block tile
        ++y;               // increment Y to write to tile bytes
        DumpFourSpr(a, y); // do sub to dump into all four sprites
        --y;               // return Y to original offset
        a = 0x03;          // set palette bits
        x = M(AreaType);
        --x; // check for ground level type area again
        if (x != 0)
        {             // if found, use current palette bits
            a = 0x01; // otherwise set to $01
        } // SetBFlip: put block object offset back in X
        x = M(ObjectOffset);
        writeData(Sprite_Attributes + y, a); // store attribute byte as-is in first sprite
        a |= 0b01000000;
        writeData(Sprite_Attributes + 4 + y, a); // set horizontal flip bit for second sprite
        a |= 0b10000000;
        writeData(Sprite_Attributes + 12 + y, a); // set both flip bits for fourth sprite
        a &= 0b10000011;
        writeData(Sprite_Attributes + 8 + y, a); // set vertical flip bit for third sprite
    } // BlkOffscr: get offscreen bits for block object
    a = M(Block_OffscreenBits);
    pha();           // save to stack
    a &= 0b00000100; // check to see if d2 in offscreen bits are set
    if (a != 0)
    {                                               // if not set, branch, otherwise move sprites offscreen
        a = 0xf8;                                   // move offscreen two OAMs
        writeData(Sprite_Y_Position + 4 + y, 0xf8); // on the right side
        writeData(Sprite_Y_Position + 12 + y, 0xf8);
    } // PullOfsB: pull offscreen bits from stack
    pla();
    ChkLeftCo();
}

//------------------------------------------------------------------------

// check to see if d3 in offscreen bits are set
// Inputs: a = block offscreen bits
// Outputs: none
void SMBEngine::ChkLeftCo()
{
    a &= 0b00001000;
    if (a == 0)
    { // if not set, branch, otherwise move sprites offscreen
        return;
    }
    MoveColOffscreen(y);
}

//------------------------------------------------------------------------

// Inputs: x = block object buffer offset
// Outputs: none
void SMBEngine::DrawBrickChunks()
{
    uint32_t wide = 0;
    bool carry = false;

    // set palette bits here
    writeData(0x00, 0x02);
    a = 0x75; // set tile number for ball (something residual, likely)
    if (M(GameEngineSubroutine) != 0x05)
    { // use palette and tile number assigned
        // otherwise set different palette bits
        writeData(0x00, 0x03);
        a = 0x84; // and set tile number for brick chunks
    } // DChunks: get OAM data offset
    y = M(Block_SprDataOffset + x);
    ++y;                 // increment to start with tile bytes in OAM
    DumpFourSpr(a, y);   // do sub to dump tile number into all four sprites
    a = M(FrameCounter); // get frame counter
    a <<= 1;
    a <<= 1;
    a <<= 1; // move low nybble to high
    a <<= 1;
    a &= 0xc0;         // get what was originally d3-d2 of low nybble
    a |= M(0x00);      // add palette bits
    ++y;               // increment offset for attribute bytes
    DumpFourSpr(a, y); // do sub to dump attribute data into all four sprites
    --y;
    --y;                   // decrement offset to Y coordinate
    a = M(Block_Rel_YPos); // get first block object's relative vertical coordinate
    DumpTwoSpr(a, y);      // do sub to dump current Y coordinate into two sprites
    // get first block object's relative horizontal coordinate
    writeData(Sprite_X_Position + y, M(Block_Rel_XPos)); // save into X coordinate of first sprite
    a = M(Block_Orig_XPos + x);                          // get original horizontal coordinate
    a -= M(ScreenLeft_X_Pos);                            // subtract coordinate of left side from original coordinate
    writeData(0x00, a);                                  // store result as relative horizontal coordinate of original
    carry = a >= M(Block_Rel_XPos);                      // the borrow this subtract leaves is read by the add below
    a -= M(Block_Rel_XPos);                              // get difference of relative positions of original - current
    wide = a + M(0x00) + (carry ? 1 : 0);                // add original relative position to result
    a = (uint8_t)(LOBYTE(wide) + 0x06 + HIBYTE(wide)); // plus 6 pixels, and this add's own carry, to position second brick chunk correctly
    writeData(Sprite_X_Position + 4 + y, a);           // save into X coordinate of second sprite
    a = M(Block_Rel_YPos + 1);                         // get second block object's relative vertical coordinate
    writeData(Sprite_Y_Position + 8 + y, a);
    writeData(Sprite_Y_Position + 12 + y, a); // dump into Y coordinates of third and fourth sprites
    // get second block object's relative horizontal coordinate
    writeData(Sprite_X_Position + 8 + y, M(Block_Rel_XPos + 1)); // save into X coordinate of third sprite
    a = M(0x00);                                                 // use original relative horizontal position
    carry = a >= M(Block_Rel_XPos + 1);                          // the borrow this subtract leaves is read by the add below
    a -= M(Block_Rel_XPos + 1);                                  // get difference of relative positions of original - current
    wide = a + M(0x00) + (carry ? 1 : 0);                        // add original relative position to result
    a = (uint8_t)(LOBYTE(wide) + 0x06 + HIBYTE(wide)); // plus 6 pixels, and this add's own carry, to position fourth brick chunk correctly
    writeData(Sprite_X_Position + 12 + y, a);          // save into X coordinate of fourth sprite
    a = M(Block_OffscreenBits);                        // get offscreen bits for block object
    ChkLeftCo();                                       // do sub to move left half of sprites offscreen if necessary
    if ((M(Block_OffscreenBits) & 0x80) != 0)          // check d7 of the offscreen bits
    {                                                  // if d7 not set, branch to last part
        a = 0xf8;
        DumpTwoSpr(a, y); // otherwise move top sprites offscreen
    } // ChnkOfs: if relative position on left side of screen,
    a = M(0x00);
    if ((a & 0x80) == 0)
    {
        return; // go ahead and leave
    }
    a = M(Sprite_X_Position + y); // otherwise compare left-side X coordinate
    if (a < M(Sprite_X_Position + 4 + y))
    {
        return; // branch to leave if less
    }
    a = 0xf8; // otherwise move right half of sprites offscreen
    writeData(Sprite_Y_Position + 4 + y, 0xf8);
    writeData(Sprite_Y_Position + 12 + y, 0xf8);

    // ExBCDr: leave
}

//------------------------------------------------------------------------

// Inputs: x = misc object buffer offset
// Outputs: x = misc object buffer offset (unchanged/restored on both return paths)
void SMBEngine::JCoinGfxHandler()
{
    const uint8_t JumpingCoinTiles_data[] = {0x60, 0x61, 0x62, 0x63};

    goto JCoinGfxHandler2;
    do // DrawFloateyNumber_Coin
    {
        if ((M(FrameCounter) & 0x01) == 0) // get frame counter divide by 2
        {                                  // branch if d0 not set to raise number every other frame
            --M(Misc_Y_Position + x);      // otherwise, decrement vertical coordinate
        } // NotRsNum: get vertical coordinate
        a = M(Misc_Y_Position + x);
        DumpTwoSpr(a, y);                        // dump into both sprites
        a = M(Misc_Rel_XPos);                    // get relative horizontal coordinate
        writeData(Sprite_X_Position + y, a);     // store as X coordinate for first sprite
        a += 0x08;                               // add eight pixels
        writeData(Sprite_X_Position + 4 + y, a); // store as X coordinate for second sprite
        writeData(Sprite_Attributes + y, 0x02);  // store attribute byte in both sprites
        writeData(Sprite_Attributes + 4 + y, 0x02);
        writeData(Sprite_Tilenumber + y, 0xf7); // put tile numbers into both sprites
        a = 0xfb;                               // that resemble "200"
        writeData(Sprite_Tilenumber + 4 + y, 0xfb);
        return; // then jump to leave (why not an rts here instead?)

    JCoinGfxHandler2:
        y = M(Misc_SprDataOffset + x); // get coin/floatey number's OAM data offset
        // get state of misc object
    } while (M(Misc_State + x) >= 0x02); // branch to draw floatey number
    a = M(Misc_Y_Position + x);              // store vertical coordinate as
    writeData(Sprite_Y_Position + y, a);     // Y coordinate for first sprite
    a += 0x08;                               // add eight pixels
    writeData(Sprite_Y_Position + 4 + y, a); // store as Y coordinate for second sprite
    a = M(Misc_Rel_XPos);                    // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, a);
    writeData(Sprite_X_Position + 4 + y, a); // store as X coordinate for first and second sprites
    // get frame counter
    a = M(FrameCounter) >> 1;               // divide by 2 to alter every other frame
    a &= 0b00000011;                        // mask out d2-d1
    x = a;                                  // use as graphical offset
    a = JumpingCoinTiles_data[x];           // load tile number
    ++y;                                    // increment OAM data offset to write tile numbers
    DumpTwoSpr(a, y);                       // do sub to dump tile number into both sprites
    --y;                                    // decrement to get old offset
    writeData(Sprite_Attributes + y, 0x02); // set attribute byte in first sprite
    a = 0x82;
    writeData(Sprite_Attributes + 4 + y, 0x82); // set attribute byte with vertical flip in second sprite
    x = M(ObjectOffset);                        // get misc object offset

    // ExJCGfx: leave
}

//------------------------------------------------------------------------

// Inputs: x = fireball object buffer offset
// Outputs: none
void SMBEngine::DrawExplosion_Fireball()
{
    y = M(Alt_SprDataOffset + x); // get OAM data offset of alternate sort for fireball's explosion
    a = M(Fireball_State + x);    // load fireball state
    ++M(Fireball_State + x);      // increment state for next frame
    a >>= 1;                      // divide by 2
    a &= 0b00000111;              // mask out all but d3-d1
    if (a >= 0x03)
    { // branch if so, otherwise continue to draw explosion
        // moved
        a = 0x00; // clear fireball state to kill it
        writeData(Fireball_State + x, 0x00);
        return;
    }
    DrawExplosion_Fireworks(a, y);
}

//------------------------------------------------------------------------

// Inputs: none (uses enemy object buffer slot 5, the flagpole flag's special-use slot)
// Outputs: none
void SMBEngine::FlagpoleRoutine()
{
    const uint8_t FlagpoleScoreDigits_data[] = {0x03, 0x03, 0x04, 0x04, 0x04};

    const uint8_t FlagpoleScoreMods_data[] = {0x05, 0x02, 0x08, 0x04, 0x01};

    uint32_t wide = 0;

    x = 0x05;                      // set enemy object offset
    writeData(ObjectOffset, 0x05); // to special use slot
    a = M(Enemy_ID + 0x05);
    if (a == FlagpoleFlagObject)
    { // branch to leave
        if (M(GameEngineSubroutine) != 0x04)
        {
            goto SkipScore; // branch to near the end of code
        }
        if (M(Player_State) != 0x03)
        {
            goto SkipScore; // branch to near the end of code
        }
        // check flagpole flag's vertical coordinate
        if (M(Enemy_Y_Position + 0x05) >= 0xaa)
        {
            goto GiveFPScr; // branch to end the level
        }
        // check player's vertical coordinate
        if (M(Player_Y_Position) >= 0xa2)
        {
            goto GiveFPScr; // branch to end the level
        }
        // position:dummy is one 16-bit quantity; the compare above left the carry clear
        wide = ((M(Enemy_Y_Position + 0x05) << 8) | M(Enemy_YMF_Dummy + 0x05)) + 0x01ff; // add movement amount to move flag down
        writeData(Enemy_YMF_Dummy + 0x05, LOBYTE(wide));                                 // save dummy variable
        writeData(Enemy_Y_Position + 0x05, HIBYTE(wide));                                // store vertical coordinate
        wide = ((M(FlagpoleFNum_Y_Pos) << 8) | M(FlagpoleFNum_YMFDummy)) - 0x01ff;       // subtract the same to move the floatey number up
        writeData(FlagpoleFNum_YMFDummy, LOBYTE(wide));                                  // save dummy variable
        writeData(FlagpoleFNum_Y_Pos, HIBYTE(wide));                                     // and store vertical coordinate here
        a = HIBYTE(wide);

    SkipScore: // jump to skip ahead and draw flag and floatey number
        goto FPGfx;

    GiveFPScr: // get score offset from earlier (when player touched flagpole)
        y = M(FlagpoleScore);
        // get amount to award player points
        x = FlagpoleScoreDigits_data[y];                         // get digit with which to award points
        writeData(DigitModifier + x, FlagpoleScoreMods_data[y]); // store in digit modifier
        AddToScore();                                            // do sub to award player points depending on height of collision
        a = 0x05;
        writeData(GameEngineSubroutine, 0x05); // set to run end-of-level subroutine on next frame

    FPGfx: // get offscreen information
        GetEnemyOffscreenBits(x);
        RelativeEnemyPosition(x); // get relative coordinates
        FlagpoleGfxHandler();    // draw flagpole flag and floatey number
    } // ExitFlagP
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (expected to be 5, the flagpole flag's special-use slot)
// Outputs: none
void SMBEngine::FlagpoleGfxHandler()
{
    const uint8_t FlagpoleScoreNumTiles_data[] = {0xf9, 0x50, 0xf7, 0x50, 0xfa, 0xfb, 0xf8, 0xfb, 0xf6, 0xfb};

    uint32_t wide = 0;

    y = M(Enemy_SprDataOffset + x);          // get sprite data offset for flagpole flag
    a = M(Enemy_Rel_XPos);                   // get relative horizontal coordinate
    writeData(Sprite_X_Position + y, a);     // store as X coordinate for first sprite
    a += 0x08;                               // add eight pixels and store
    writeData(Sprite_X_Position + 4 + y, a); // as X coordinate for second and third sprites
    writeData(Sprite_X_Position + 8 + y, a);
    wide = a + 0x0c;                         // add twelve more pixels and
    writeData(0x05, LOBYTE(wide));           // store here to be used later by floatey number
    a = M(Enemy_Y_Position + x);             // get vertical coordinate
    DumpTwoSpr(a, y);                        // and do sub to dump into first and second sprites
    a = (uint8_t)(a + 0x08 + HIBYTE(wide));  // add eight pixels, plus the carry out of the horizontal add above
    writeData(Sprite_Y_Position + 8 + y, a); // and store into third sprite
    // get vertical coordinate for floatey number
    writeData(0x02, M(FlagpoleFNum_Y_Pos)); // store it here
    writeData(0x03, 0x01);                  // set value for flip which will not be used, and
    writeData(0x04, 0x01);                  // attribute byte for floatey number
    writeData(Sprite_Attributes + y, 0x01); // set attribute bytes for all three sprites
    writeData(Sprite_Attributes + 4 + y, 0x01);
    writeData(Sprite_Attributes + 8 + y, 0x01);
    writeData(Sprite_Tilenumber + y, 0x7e);     // put triangle shaped tile
    writeData(Sprite_Tilenumber + 8 + y, 0x7e); // into first and third sprites
    writeData(Sprite_Tilenumber + 4 + y, 0x7f); // put skull tile into second sprite
    // get vertical coordinate at time of collision
    if (M(FlagpoleCollisionYPos) != 0)
    { // if zero, branch ahead
        a = y;
        a += 0x0c;
        y = a;                // put back in Y
        a = M(FlagpoleScore); // get offset used to award points for touching flagpole
        a <<= 1;              // multiply by 2 to get proper offset here
        x = a;
        // get appropriate tile data
        writeData(0x00, FlagpoleScoreNumTiles_data[x]);
        a = FlagpoleScoreNumTiles_data[1 + x];
        DrawOneSpriteRow(a, x, y); // use it to render floatey number
    } // ChkFlagOffscreen
    x = M(ObjectOffset);            // get object offset for flag
    y = M(Enemy_SprDataOffset + x); // get OAM data offset
    // get offscreen bits
    a = M(Enemy_OffscreenBits) & 0b00001110; // mask out all but d3-d1
    if (a == 0)
    { // if none of these bits set, branch to leave
        return;
    }
    MoveSixSpritesOffscreen(y);
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerLoseLife()
{
    const uint8_t HalfwayPageNybbles_data[] = {0x56, 0x40, 0x65, 0x70, 0x66, 0x40, 0x66, 0x40,
                                               0x66, 0x40, 0x66, 0x60, 0x65, 0x70, 0x00, 0x00};

    bool endGame = false;

    ++M(DisableScreenFlag); // disable screen and sprite 0 check
    writeData(Sprite0HitDetectFlag, 0x00);
    a = Silence; // silence music
    writeData(EventMusicQueue, Silence);
    --M(NumberofLives); // take one life from player
    if ((M(NumberofLives) & 0x80) != 0)
    {                                           // if player still has lives, branch
        writeData(OperMode_Task, 0x00);         // initialize mode task,
        a = GameOverModeValue;                  // switch to game over mode
        writeData(OperMode, GameOverModeValue); // and leave
        return;

        //------------------------------------------------------------------------
    } // StillInGame: multiply world number by 2 and use
    a = M(WorldNumber);
    a <<= 1; // as offset
    x = a;
    // if in area -3 or -4, increment
    a = M(LevelNumber) & 0x02; // offset by one byte, otherwise
    if (a != 0)
    { // leave offset alone
        ++x;
    } // GetHalfway: get halfway page number with offset
    y = HalfwayPageNybbles_data[x];
    a = y; // if in area -2 or -4, use lower nybble
    if ((M(LevelNumber) & 0x01) == 0)
    {
        a >>= 1; // move higher nybble to lower if area
        a >>= 1; // number is -1 or -3
        a >>= 1;
        a >>= 1;
    } // MaskHPNyb: mask out all but lower nybble
    a &= 0b00001111;
    if (a == M(ScreenLeft_PageLoc))
    {
        goto SetHalfway; // left side of screen must be at the halfway page,
    }
    if (a < M(ScreenLeft_PageLoc))
    {
        goto SetHalfway; // otherwise player must start at the
    }
    a = 0x00; // beginning of the level

SetHalfway: // store as halfway page for player
    writeData(HalfwayPage, a);
    endGame = TransposePlayers(); // switch players around if 2-player game
    ContinueGame();               // continue the game
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ContinueGame()
{
    LoadAreaPointer(); // update level pointer with
    // actual world and area numbers, then
    writeData(PlayerSize, 0x01); // reset player's size, status, and
    ++M(FetchNewGameTimerFlag);  // set game timer flag to reload
    // game timer from header
    writeData(TimerControl, 0x00); // also set flag for timers to count again
    writeData(PlayerStatus, 0x00);
    writeData(GameEngineSubroutine, 0x00); // reset task for game core
    writeData(OperMode_Task, 0x00);        // set modes and leave
    a = 0x01;                              // if in game over mode, switch back to
    writeData(OperMode, 0x01);             // game mode, because game is still on
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::Entrance_GameTimerSetup()
{
    const uint8_t PlayerStarting_Y_Pos_data[] = {0x00, 0x20, 0xb0, 0x50, 0x00, 0x00, 0xb0, 0xb0, 0xf0};

    const uint8_t AltYPosOffset_data[] = {0x08, 0x00};

    const uint8_t PlayerStarting_X_Pos_data[] = {0x28, 0x18, 0x38, 0x28};

    // set current page for area objects
    writeData(Player_PageLoc, M(ScreenLeft_PageLoc)); // as page location for player
    // store value here
    writeData(VerticalForceDown, 0x28); // for fractional movement downwards if necessary
    // set high byte of player position and
    writeData(PlayerFacingDir, 0x01); // set facing direction so that player faces right
    writeData(Player_Y_HighPos, 0x01);
    // set player state to on the ground by default
    writeData(Player_State, 0x00);
    --M(Player_CollisionBits); // initialize player's collision bits
    y = 0x00;                  // initialize halfway page
    writeData(HalfwayPage, 0x00);
    // check area type
    if (M(AreaType) == 0)
    { // if water type, set swimming flag, otherwise do not set
        y = 0x01;
    } // ChkStPos
    writeData(SwimmingFlag, y);
    x = M(PlayerEntranceCtrl); // get starting position loaded from header
    y = M(AltEntranceControl); // check alternate mode of entry flag for 0 or 1
    if (y == 0)
    {
        goto SetStPos;
    }
    if (y == 0x01)
    {
        goto SetStPos;
    }
    x = AltYPosOffset_data[y - 2]; // if not 0 or 1, override $0710 with new offset in X

SetStPos:                                                       // load appropriate horizontal position
    writeData(Player_X_Position, PlayerStarting_X_Pos_data[y]); // and vertical positions for the player, using
    // AltEntranceControl as offset for horizontal and either $0710
    writeData(Player_Y_Position, PlayerStarting_Y_Pos_data[x]); // or value that overwrote $0710 as offset for vertical
    writeData(Player_SprAttrib, M(PlayerBGPriorityData + x));   // set player sprite attributes using offset in X
    GetPlayerColors();                                          // get appropriate player palette
    y = M(GameTimerSetting);                                    // get timer control value from header
    if (y == 0)
    {
        goto ChkOverR; // if set to zero, branch (do not use dummy byte for this)
    }
    // do we need to set the game timer? if not, use
    if (M(FetchNewGameTimerFlag) == 0)
    {
        goto ChkOverR; // old game timer setting
    }
    // if game timer is set and game timer flag is also set,
    writeData(GameTimerDisplay, M(GameTimerData + y)); // use value of game timer control for first digit of game timer
    writeData(GameTimerDisplay + 2, 0x01);             // set last digit of game timer to 1
    a = 0x00;
    writeData(GameTimerDisplay + 1, 0x00);  // set second digit of game timer
    writeData(FetchNewGameTimerFlag, 0x00); // clear flag for game timer reset
    writeData(StarInvincibleTimer, 0x00);   // clear star mario timer

ChkOverR: // if controller bits not set, branch to skip this part
    if (M(JoypadOverride) != 0)
    {
        a = 0x03; // set player state to climbing
        writeData(Player_State, 0x03);
        x = 0x00; // set offset for first slot, for block object
        InitBlock_XY_Pos(x);
        a = 0xf0; // set vertical coordinate for block object
        writeData(Block_Y_Position, 0xf0);
        x = 0x05;         // set offset in X for last enemy object buffer slot
        y = 0x00;         // set offset in Y for object coordinates used earlier
        Setup_Vine(x, y); // do a sub to grow vine
    } // ChkSwimE: if level not water-type,
    y = M(AreaType);
    if (y == 0)
    {                         // skip this subroutine
        writeData(0x07, 145); // LYNN HACK: simulate reading stray $07 value from JumpEngine,
                              // read by SetupBubble
        SetupBubble(x);       // otherwise, execute sub to set up air bubbles
    } // SetPESub: set to run player entrance subroutine
    a = 0x07;
    writeData(GameEngineSubroutine, 0x07); // on the next frame of game engine
}

//------------------------------------------------------------------------

// Inputs: slot = air bubble object buffer offset
// Outputs: none
void SMBEngine::BubbleCheck(uint8_t slot)
{
    // get part of LSFR and store pseudorandom bit here
    writeData(0x07, M(PseudoRandomBitReg + 1 + slot) & 0x01);
    // get vertical coordinate for air bubble
    if (M(Bubble_Y_Position + slot) != 0xf8)
    { // branch to move air bubble
        MoveBubl(slot);
        return;
    }
    if (M(AirBubbleTimer) != 0)
    {
        return; // if air bubble timer not expired, branch to leave
    }
    SetupBubble(slot); // otherwise create new air bubble
}

//------------------------------------------------------------------------

// Bubble_MForceData and BubbleTimerData
//
// Both of these two-entry tables are meant to be indexed with a pseudorandom
// bit, which BubbleCheck puts in $07 before it falls through into
// SetupBubble. But the player entrance code calls SetupBubble directly, and
// on that path $07 still holds whatever the last routine to use it left
// there, which is JumpEngine's high byte of the address of the last routine
// it dispatched to (see SMBEngine::jumpEngine()). So on the first frame of
// every water level these tables are indexed with an arbitrary byte, and the
// air bubbles start out with a timer and a movement force read from whatever
// the ROM happens to store after them.
//
// The two tables are written here as the 258 bytes the ROM has from the start
// of Bubble_MForceData ($b74b), which is every byte the two of them can be
// indexed with, so that those reads find what they find on the NES. Most of
// this is the game's own code, read as if it were data.
//
const uint8_t BubbleData[] = {
    0xff, 0x50, 0x40, 0x20, 0xad, 0x70, 0x07, 0xf0, 0x4f, 0xa5, 0x0e, 0xc9, 0x08, 0x90, 0x49, 0xc9, 0x0b, 0xf0, 0x45, 0xa5, 0xb5, 0xc9,
    0x02, 0xb0, 0x3f, 0xad, 0x87, 0x07, 0xd0, 0x3a, 0xad, 0xf8, 0x07, 0x0d, 0xf9, 0x07, 0x0d, 0xfa, 0x07, 0xf0, 0x26, 0xac, 0xf8, 0x07,
    0x88, 0xd0, 0x0c, 0xad, 0xf9, 0x07, 0x0d, 0xfa, 0x07, 0xd0, 0x04, 0xa9, 0x40, 0x85, 0xfc, 0xa9, 0x18, 0x8d, 0x87, 0x07, 0xa0, 0x23,
    0xa9, 0xff, 0x8d, 0x39, 0x01, 0x20, 0x5f, 0x8f, 0xa9, 0xa4, 0x4c, 0x06, 0x8f, 0x8d, 0x56, 0x07, 0x20, 0x31, 0xd9, 0xee, 0x59, 0x07,
    0x60, 0xad, 0x23, 0x07, 0xf0, 0xfa, 0xa5, 0xce, 0x25, 0xb5, 0xd0, 0xf4, 0x8d, 0x23, 0x07, 0xee, 0xd6, 0x06, 0x4c, 0x98, 0xc9, 0xad,
    0x4e, 0x07, 0xd0, 0x37, 0x8d, 0x7d, 0x04, 0xad, 0x47, 0x07, 0xd0, 0x2f, 0xa0, 0x04, 0xb9, 0x71, 0x04, 0x18, 0x79, 0x77, 0x04, 0x85,
    0x02, 0xb9, 0x6b, 0x04, 0xf0, 0x1c, 0x69, 0x00, 0x85, 0x01, 0xa5, 0x86, 0x38, 0xf9, 0x71, 0x04, 0xa5, 0x6d, 0xf9, 0x6b, 0x04, 0x30,
    0x0b, 0xa5, 0x02, 0x38, 0xe5, 0x86, 0xa5, 0x01, 0xe5, 0x6d, 0x10, 0x04, 0x88, 0x10, 0xd3, 0x60, 0xb9, 0x77, 0x04, 0x4a, 0x85, 0x00,
    0xb9, 0x71, 0x04, 0x18, 0x65, 0x00, 0x85, 0x01, 0xb9, 0x6b, 0x04, 0x69, 0x00, 0x85, 0x00, 0xa5, 0x09, 0x4a, 0x90, 0x2c, 0xa5, 0x01,
    0x38, 0xe5, 0x86, 0xa5, 0x00, 0xe5, 0x6d, 0x10, 0x0e, 0xa5, 0x86, 0x38, 0xe9, 0x01, 0x85, 0x86, 0xa5, 0x6d, 0xe9, 0x00, 0x4c, 0x39,
    0xb8, 0xad, 0x90, 0x04, 0x4a, 0x90, 0x0d, 0xa5, 0x86, 0x18, 0x69, 0x01, 0x85, 0x86, 0xa5, 0x6d, 0x69, 0x00, 0x85, 0x6d, 0xa9, 0x10,
    0x85, 0x00, 0xa9, 0x01, 0x8d, 0x7d, 0x04, 0x85, 0x02, 0x4a, 0xaa, 0x4c, 0xd7, 0xbf, 0x05, 0x02};

const uint8_t *Bubble_MForceData = BubbleData;
const uint8_t *BubbleTimerData = BubbleData + 2;

// Inputs: slot = air bubble object buffer offset
// Outputs: none
void SMBEngine::SetupBubble(uint8_t slot)
{
    uint8_t adder = 0x00; // load default value here
    if ((M(PlayerFacingDir) & 0x01) != 0)
    {                 // use the default value if facing left
        adder = 0x09; // otherwise eight pixels over, plus the one d0 of the facing direction carries in
    } // PosBubl: use value loaded as adder
    // add to player's horizontal position
    const uint32_t wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + adder;
    writeData(Bubble_X_Position + slot, LOBYTE(wide)); // save as horizontal position for airbubble
    writeData(Bubble_PageLoc + slot, HIBYTE(wide));    // save as page location for airbubble
    // save player's vertical position plus eight pixels as vertical position for air bubble
    writeData(Bubble_Y_Position + slot, M(Player_Y_Position) + 0x08);
    writeData(Bubble_Y_HighPos + slot, 0x01); // set vertical high byte for air bubble
    // get pseudorandom bit, use as offset to get data for air bubble timer
    writeData(AirBubbleTimer, BubbleTimerData[M(0x07)]); // set air bubble timer
    MoveBubl(slot);
}

//------------------------------------------------------------------------

// Inputs: slot = air bubble object buffer offset
// Outputs: none
void SMBEngine::MoveBubl(uint8_t slot)
{
    // get pseudorandom bit again, use as offset
    // position:dummy is one 16-bit quantity
    const uint32_t wide = ((M(Bubble_Y_Position + slot) << 8) | M(Bubble_YMF_Dummy + slot)) -
                          Bubble_MForceData[M(0x07)]; // subtract pseudorandom amount from dummy variable
    writeData(Bubble_YMF_Dummy + slot, LOBYTE(wide)); // save dummy variable
    uint8_t yPos = HIBYTE(wide); // the airbubble's vertical coordinate, less the borrow
    if (yPos < 0x20)
    {                // branch to go ahead and use to move air bubble upwards
        yPos = 0xf8; // otherwise set offscreen coordinate
    } // Y_Bubl: store as new vertical coordinate for air bubble
    writeData(Bubble_Y_Position + slot, yPos);

    // ExitBubl: leave
}

//------------------------------------------------------------------------

// Inputs: slot = fireball object buffer offset
// Outputs: none (forwards slot + 7 and relative-coordinates offset 2 into FBallB/BoundingBoxCore)
void SMBEngine::GetFireballBoundBox(uint8_t slot)
{
    // add seven bytes to offset, and set offset for relative coordinates
    FBallB(slot + 0x07, 0x02);
}

//------------------------------------------------------------------------

// Inputs: slot = misc object buffer offset
// Outputs: none (forwards slot + 9 and relative-coordinates offset 6 into FBallB/BoundingBoxCore)
void SMBEngine::GetMiscBoundBox(uint8_t slot)
{
    // add nine bytes to offset, and set offset for relative coordinates
    FBallB(slot + 0x09, 0x06);
}

//------------------------------------------------------------------------

// get bounding box coordinates
// Inputs: objectOffset, relPosIdx (forwarded to BoundingBoxCore, as prepared by
// GetFireballBoundBox/GetMiscBoundBox)
// Outputs: none (delegates to BoundingBoxCore/CheckRightScreenBBox)
void SMBEngine::FBallB(uint8_t objectOffset, uint8_t relPosIdx)
{
    const uint8_t boundBoxIdx = BoundingBoxCore(objectOffset, relPosIdx);
    CheckRightScreenBBox(objectOffset, boundBoxIdx); // jump to handle any offscreen coordinates
}

//------------------------------------------------------------------------

// Inputs: a = controller bits override value
// Outputs: none
void SMBEngine::AutoControlPlayer(uint8_t ctrlBits)
{
    writeData(SavedJoypadBits, ctrlBits); // override controller bits with contents of A if executing here
    PlayerCtrlRoutine();
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::Vine_AutoClimb()
{
    // check to see whether player reached position
    if (M(Player_Y_HighPos) == 0)
    { // above the status bar yet and if so, set modes
        if (M(Player_Y_Position) < 0xe4)
        {
            SetEntr();
            return;
        }
    } // AutoClimb: set controller bits override to up
    a = 0b00001000;
    writeData(JoypadOverride, 0b00001000);
    y = 0x03; // set player state to climbing
    writeData(Player_State, 0x03);
    AutoControlPlayer(a);
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::VerticalPipeEntry()
{
    MovePlayerYAxis(0x01);  // set 1 as movement amount, do sub to move player downwards
    ScrollHandler();        // do sub to scroll screen with saved force if necessary
    y = 0x00;               // load default mode of entry
    a = M(WarpZoneControl); // check warp zone control variable/flag
    if (a != 0)
    {
        ChgAreaPipe(); // if set, branch to use mode 0
        return;
    }
    y = 0x01;
    a = M(AreaType); // check for castle level type
    if (a != 0x03)
    {
        ChgAreaPipe(); // if not castle type level, use mode 1
        return;
    }
    y = 0x02;
    ChgAreaPipe(); // otherwise use mode 2
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::SideExitPipeEntry()
{
    EnterSidePipe(); // execute sub to move player to the right
    y = 0x02;
    ChgAreaPipe();
}

//------------------------------------------------------------------------

// decrement timer for change of area
// Inputs: y = mode of alternate entry to set once the change-area timer expires
// Outputs: none
void SMBEngine::ChgAreaPipe()
{
    --M(ChangeAreaTimer);
    if (M(ChangeAreaTimer) != 0)
    {
        return;
    }
    writeData(AltEntranceControl, y); // when timer expires set mode of alternate entry
    ChgAreaMode();
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::EnterSidePipe()
{
    // set player's horizontal speed
    writeData(Player_X_Speed, 0x08);
    y = 0x01; // set controller right button by default
    // mask out higher nybble of player's
    a = M(Player_X_Position) & 0b00001111; // horizontal position
    if (a == 0)
    {
        writeData(Player_X_Speed, a); // if lower nybble = 0, set as horizontal speed
        y = a;                        // and nullify controller bit override here
    } // RightPipe: use contents of Y to
    a = y;
    AutoControlPlayer(a); // execute player control routine with ctrl bits nulled
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerDeath()
{
    a = M(TimerControl); // check master timer control
    if (a < 0xf0)
    {                        // branch to leave if before that point
        PlayerCtrlRoutine(); // otherwise run player control routine
        return;

        //------------------------------------------------------------------------
    } // ExitDeath
    // leave from death routine
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::FlagpoleSlide()
{
    a = M(Enemy_ID + 5); // check special use enemy slot
    if (a == FlagpoleFlagObject)
    { // if not found, branch to something residual
        // load flagpole sound
        writeData(Square1SoundQueue, M(FlagpoleSoundQueue)); // into square 1's sfx queue
        a = 0x00;
        writeData(FlagpoleSoundQueue, 0x00); // init flagpole sound queue
        if (M(Player_Y_Position) < 0x9e)
        {             // far enough, and if so, branch with no controller bits set
            a = 0x04; // otherwise force player to climb down (to slide)
        } // SlidePlayer: jump to player control routine
        AutoControlPlayer(a);
        return;
    } // NoFPObj: increment to next routine (this may
    ++M(GameEngineSubroutine);
    // be residual code)
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerEntrance()
{
    // check for mode of alternate entry
    if (M(AltEntranceControl) != 0x02)
    { // if found, branch to enter from pipe or with vine
        a = 0x00;
        y = M(Player_Y_Position); // if vertical position above a certain
        if (y < 0x30)
        {
            AutoControlPlayer(a); // with player movement code, do not return
            return;
        }
        a = M(PlayerEntranceCtrl); // check player entry bits from header
        if (a != 0x06)
        { // if set to 6 or 7, execute pipe intro code
            if (a != 0x07)
            {
                goto PlayerRdy;
            }
        } // ChkBehPipe: check for sprite attributes
        if (M(Player_SprAttrib) == 0)
        { // branch if found
            a = 0x01;
            AutoControlPlayer(a); // force player to walk to the right
            return;
        } // IntroEntr: execute sub to move player to the right
        EnterSidePipe();
        --M(ChangeAreaTimer); // decrement timer for change of area
        if (M(ChangeAreaTimer) != 0)
        {
            return; // branch to exit if not yet expired
        }
        ++M(DisableIntermediate); // set flag to skip world and lives display
        NextArea();               // jump to increment to next area and set modes
        return;
    } // EntrMode2: if controller override bits set here,
    if (M(JoypadOverride) == 0)
    {                             // branch to enter with vine
        MovePlayerYAxis(0xff);    // otherwise, execute sub to move player upwards (note $ff = -1)
        a = M(Player_Y_Position); // check to see if player is at a specific coordinate
        if (a < 0x91)
        {
            goto PlayerRdy; // to be at specific height to look/function right) branch
        }
        return; // to the last part, otherwise leave

        //------------------------------------------------------------------------
    } // VineEntr
    a = M(VineHeight);
    if (a != 0x60)
    {
        return; // if vine not yet reached maximum height, branch to leave
    }
    a = M(Player_Y_Position); // get player's vertical coordinate
    y = 0x00;                 // load default values to be written to
    a = 0x01;                 // this value moves player to the right off the vine
    if (M(Player_Y_Position) >= 0x99)
    {                                           // if vertical coordinate < preset value, use defaults
        writeData(Player_State, 0x03);          // otherwise set player state to climbing
        y = 0x01;                               // increment value in Y
        a = 0x08;                               // set block in block buffer to cover hole, then
        writeData(Block_Buffer_1 + 0xb4, 0x08); // use same value to force player to climb
    } // OffVine: set collision detection disable flag
    writeData(DisableCollisionDet, y);
    AutoControlPlayer(a); // use contents of A to move player up or right, execute sub
    a = M(Player_X_Position);
    if (a < 0x48)
    {
        return; // if not far enough to the right, branch to leave
    }

PlayerRdy: // set routine to be executed by game engine next frame
    writeData(GameEngineSubroutine, 0x08);
    // set to face player to the right
    writeData(PlayerFacingDir, 0x01);
    a = 0x00;                             // init A
    writeData(AltEntranceControl, 0x00);  // init mode of entry
    writeData(DisableCollisionDet, 0x00); // init collision detection disable flag
    writeData(JoypadOverride, 0x00);      // nullify controller override bits

    // ExitEntr: leave!
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerEndLevel()
{
    const uint8_t Hidden1UpCoinAmts_data[] = {0x15, 0x23, 0x16, 0x1b, 0x17, 0x18, 0x23, 0x63};

    a = 0x01; // force player to walk to the right
    AutoControlPlayer(a);
    // check player's vertical position
    if (M(Player_Y_Position) < 0xae)
    {
        goto ChkStop; // if player is not yet off the flagpole, skip this part
    }
    // if scroll lock not set, branch ahead to next part
    if (M(ScrollLock) == 0)
    {
        goto ChkStop; // because we only need to do this part once
    }
    writeData(EventMusicQueue, EndOfLevelMusic); // load win level music in event music queue
    a = 0x00;
    writeData(ScrollLock, 0x00); // turn off scroll lock to skip this part later

ChkStop:                                       // get player collision bits
    if ((M(Player_CollisionBits) & 0x01) == 0) // check for d0 set
    {                                          // if d0 set, skip to next part
        // if star flag task control already set,
        if (M(StarFlagTaskControl) == 0)
        {                             // go ahead with the rest of the code
            ++M(StarFlagTaskControl); // otherwise set task control now (this gets ball rolling!)
        } // InCastle: set player's background priority bit to
        a = 0b00100000;
        writeData(Player_SprAttrib, 0b00100000); // give illusion of being inside the castle
    } // RdyNextA
    a = M(StarFlagTaskControl);
    if (a != 0x05)
    { // beyond last valid task number, branch to leave
        return;
    }
    ++M(LevelNumber); // increment level number used for game logic
    if (M(LevelNumber) != 0x03)
    {
        NextArea(); // and skip this last part here if not
        return;
    }
    y = M(WorldNumber); // get world number as offset
    // check third area coin tally for bonus 1-ups
    if (M(CoinTallyFor1Ups) < Hidden1UpCoinAmts_data[y])
    {
        NextArea(); // at least this number of coins, leave flag clear
        return;
    }
    ++M(Hidden1UpFlag); // otherwise set hidden 1-up box control flag
    NextArea();
}

//------------------------------------------------------------------------

// increment area number used for address loader
// Inputs: none
// Outputs: none
void SMBEngine::NextArea()
{
    ++M(AreaNumber);
    LoadAreaPointer();          // get new level pointer
    ++M(FetchNewGameTimerFlag); // set flag to load new game timer
    ChgAreaMode();              // do sub to set secondary mode, disable screen and sprite 0
    writeData(HalfwayPage, 0x00);        // reset halfway page to 0 (beginning)
    writeData(EventMusicQueue, Silence); // silence music and leave
}

//------------------------------------------------------------------------

// Inputs: none (dispatches on M(GameEngineSubroutine))
// Outputs: none
void SMBEngine::GameRoutines()
{
    // run routine based on number (a few of these routines are
    switch (M(GameEngineSubroutine))
    {
    case 0:
        Entrance_GameTimerSetup();
        return;
    case 1:
        Vine_AutoClimb();
        return;
    case 2:
        SideExitPipeEntry();
        return;
    case 3:
        VerticalPipeEntry();
        return;
    case 4:
        FlagpoleSlide();
        return;
    case 5:
        PlayerEndLevel();
        return;
    case 6:
        PlayerLoseLife();
        return;
    case 7:
        PlayerEntrance();
        return;
    case 8:
        PlayerCtrlRoutine();
        return;
    case 9:
        goto PlayerChangeSize;
    case 10:
        goto PlayerInjuryBlink;
    case 11:
        PlayerDeath();
        return;
    case 12:
        PlayerFireFlower();
        return;
    default:
        bad_jump();
        return;
    } // merely placeholders as conditions for other routines)

    //------------------------------------------------------------------------

PlayerChangeSize:
    a = M(TimerControl); // check master timer control
    if (a == 0xf8)
    { // branch if before or after that point
        goto InitChangeSize;
    } // EndChgSize: check again for another specific moment
    // otherwise run code to get growing/shrinking going
    if (a == 0xc4)
    {                     // and branch to leave if before or after that point
        DonePlayerTask(); // otherwise do sub to init timer control and set routine
    } // ExitChgSize: and then leave
    return;

    //------------------------------------------------------------------------

PlayerInjuryBlink:
    a = M(TimerControl); // check master timer control
    if (a < 0xf0)
    { // branch if before that point
        if (a == 0xc8)
        {
            DonePlayerTask(); // branch if at that point, and not before or after
            return;
        }
        PlayerCtrlRoutine(); // otherwise run player control routine
        return;
    } // ExitBlink: do unconditional branch to leave
    if (a != 0xf0)
    {
        return;
    }
InitChangeSize:
    y = M(PlayerChangeSizeFlag); // if growing/shrinking flag already set
    if (y != 0)
    {
        return; // then branch to leave
    }
    writeData(PlayerAnimCtrl, y); // otherwise initialize player's animation frame control
    ++M(PlayerChangeSizeFlag);    // set growing/shrinking flag
    a = M(PlayerSize) ^ 0x01;     // invert player's size
    writeData(PlayerSize, a);

    // ExitBoth: leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::UpdScrollVar()
{
    a = M(VRAM_Buffer_AddrCtrl);
    if (a == 0x06)
    {
        return; // then branch to leave
    }
    // otherwise check number of tasks
    if (M(AreaParserTaskNum) == 0)
    {
        a = M(ScrollThirtyTwo); // get horizontal scroll in 0-31 or $00-$20 range
        if (((a - 0x20) & 0x80) != 0)
        {
            return; // branch to leave if not
        }
        a = M(ScrollThirtyTwo);
        a -= 0x20;                            // otherwise subtract $20 to set appropriately
        writeData(ScrollThirtyTwo, a);        // and store
        a = 0x00;                             // reset vram buffer offset used in conjunction with
        writeData(VRAM_Buffer2_Offset, 0x00); // level graphics buffer at $0341-$035f
    } // RunParser: update the name table with more level graphics
    AreaParserTaskHandler();

    // ExitEng: and after all that, we're finally done!
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (Bowser's slot)
// Outputs: none
void SMBEngine::HurtBowser()
{
    const uint8_t BowserIdentities_data[] = {Goomba, GreenKoopa, BuzzyBeetle, Spiny, Lakitu, Bloober, HammerBro, Bowser};

    --M(BowserHitPoints); // decrement bowser's hit points
    if (M(BowserHitPoints) != 0)
    {
        return; // if bowser still has hit points, branch to leave
    }
    InitVStf(x);                        // otherwise do sub to init vertical speed and movement force
    writeData(Enemy_X_Speed + x, 0x00); // initialize horizontal speed (InitVStf left 0 in A)
    writeData(EnemyFrenzyBuffer, 0x00); // init enemy frenzy buffer
    writeData(Enemy_Y_Speed + x, 0xfe); // set vertical speed to make defeated bowser jump a little
    y = M(WorldNumber);                 // use world number as offset
    // get enemy identifier to replace bowser with
    writeData(Enemy_ID + x, BowserIdentities_data[y]); // set as new enemy identifier
    a = 0x20;                                          // set A to use starting value for state
    if (y < 0x03)
    {             // branch if so
        a = 0x23; // otherwise add 3 to enemy state
    } // SetDBSte: set defeated enemy state
    writeData(Enemy_State + x, a);
    writeData(Square2SoundQueue, Sfx_BowserFall); // load bowser defeat sound
    x = M(0x01);                                  // get enemy offset
    a = 0x09;                                     // award 5000 points to player for defeating bowser
    EnemySmackScore(a, x);                        // unconditional branch to award points
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ProcFireball_Bubble()
{
    // check player's status
    if (M(PlayerStatus) >= 0x02)
    {                                  // if not fiery, branch
        a = M(A_B_Buttons) & B_Button; // check for b button pressed
        if (a == 0)
        {
            goto ProcFireballs; // branch if not pressed
        }
        a &= M(PreviousA_B_Buttons);
        if (a != 0)
        {
            goto ProcFireballs; // if button pressed in previous frame, branch
        }
        // load fireball counter
        a = M(FireballCounter) & 0b00000001; // get LSB and use as offset for buffer
        x = a;
        a = M(Fireball_State + x); // load fireball state
        if (a != 0)
        {
            goto ProcFireballs; // if not inactive, branch
        }
        y = M(Player_Y_HighPos); // if player too high or too low, branch
        --y;
        if (y != 0)
        {
            goto ProcFireballs;
        }
        a = M(CrouchingFlag); // if player crouching, branch
        if (a != 0)
        {
            goto ProcFireballs;
        }
        a = M(Player_State); // if player's state = climbing, branch
        if (a == 0x03)
        {
            goto ProcFireballs;
        }
        // play fireball sound effect
        writeData(Square1SoundQueue, Sfx_Fireball);
        a = 0x02; // load state
        writeData(Fireball_State + x, 0x02);
        y = M(PlayerAnimTimerSet);           // copy animation frame timer setting
        writeData(FireballThrowingTimer, y); // into fireball throwing timer
        --y;
        writeData(PlayerAnimTimer, y); // decrement and store in player's animation timer
        ++M(FireballCounter);          // increment fireball counter

    ProcFireballs:
        x = 0x00;
        FireballObjCore(); // process first fireball object
        x = 0x01;
        FireballObjCore(); // process second fireball object, then do air bubbles
    } // ProcAirBubbles
    a = M(AreaType); // if not water type level, skip the rest of this
    if (a == 0)
    {
        x = 0x02; // otherwise load counter and use as offset

        do // BublLoop: store offset
        {
            writeData(ObjectOffset, x);
            BubbleCheck(x);            // check timers and coordinates, create air bubble
            RelativeBubblePosition(x); // get relative coordinates
            GetBubbleOffscreenBits(x); // get offscreen information
            DrawBubble(x);             // draw the air bubble
            --x;
        } while ((x & 0x80) == 0); // do this until all three are handled
    } // BublExit: then leave
}

//------------------------------------------------------------------------

// Inputs: x = fireball object buffer offset (0 or 1)
// Outputs: none
void SMBEngine::FireballObjCore()
{
    uint32_t wide = 0;

    writeData(ObjectOffset, x);              // store offset as current object
    if ((M(Fireball_State + x) & 0x80) == 0) // check for d7 = 1
    {                                        // if so, branch to get relative coordinates and draw explosion
        y = M(Fireball_State + x);           // if fireball inactive, branch to leave
        if (y != 0)
        {
            --y; // if fireball state set to 1, skip this part and just run it
            if (y != 0)
            {
                // get player's horizontal position
                wide =
                    ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + 0x04; // add four pixels and store as fireball's horizontal position
                writeData(Fireball_X_Position + x, LOBYTE(wide));
                writeData(Fireball_PageLoc + x, HIBYTE(wide));
                a = HIBYTE(wide); // get player's page location
                // get player's vertical position and store
                writeData(Fireball_Y_Position + x, M(Player_Y_Position));
                // set high byte of vertical position
                writeData(Fireball_Y_HighPos + x, 0x01);
                y = M(PlayerFacingDir); // get player's facing direction
                --y;                    // decrement to use as offset here
                // set horizontal speed of fireball accordingly
                writeData(Fireball_X_Speed + x, M(FireballXSpdData + y));
                // set vertical speed of fireball
                writeData(Fireball_Y_Speed + x, 0x04);
                a = 0x07;
                writeData(Fireball_BoundBoxCtrl + x, 0x07); // set bounding box size control for fireball
                --M(Fireball_State + x);                    // decrement state to 1 to skip this part from now on
            } // RunFB: add 7 to offset to use
            a = x;
            a += 0x07;
            x = a;
            // set downward movement force here
            writeData(0x00, 0x50);
            // set maximum speed here
            writeData(0x02, 0x03);
            a = 0x00;
            ImposeGravity(a, x);        // do sub here to impose gravity on fireball and move vertically
            MoveObjectHorizontally(x);  // do another sub to move it horizontally
            x = M(ObjectOffset);        // return fireball offset to X
            RelativeFireballPosition(x); // get relative coordinates
            GetFireballOffscreenBits(x); // get offscreen information
            GetFireballBoundBox(x);      // get bounding box coordinates
            FireballBGCollision();      // do fireball to background collision detection
            // get fireball offscreen bits
            a = M(FBall_OffscreenBits) & 0b11001100; // mask out certain bits
            if (a == 0)
            {                             // if any bits still set, branch to kill fireball
                FireballEnemyCollision(); // do fireball to enemy collision detection and deal with collisions
                DrawFireball(x);          // draw fireball appropriately and leave
                return;
            } // EraseFB: erase fireball state
            a = 0x00;
            writeData(Fireball_State + x, 0x00);
        } // NoFBall: leave
        return;

        //------------------------------------------------------------------------
    } // FireballExplosion
    RelativeFireballPosition(x);
    DrawExplosion_Fireball();
}

//------------------------------------------------------------------------

// Inputs: x = fireball object buffer offset
// Outputs: x = M(ObjectOffset) (restored fireball object offset)
void SMBEngine::FireballEnemyCollision()
{
    bool shiftedBit = false;
    bool collisionFound = false;

    a = M(Fireball_State + x); // check to see if fireball state is set at all
    if (a == 0)
    {
        goto ExitFBallEnemy; // branch to leave if not
    }
    shiftedBit = (a & 0x80) != 0;
    a <<= 1;
    if (shiftedBit)
    {
        goto ExitFBallEnemy; // branch to leave also if d7 in state is set
    }
    a = M(FrameCounter) >> 1; // get LSB of frame counter
    if ((M(FrameCounter) & 0x01) != 0)
    {
        goto ExitFBallEnemy; // branch to leave if set (do routine every other frame)
    }
    a = x;
    a <<= 1; // multiply fireball offset by four
    a <<= 1;
    a += 0x1c; // then add $1c or 28 bytes to it
    y = a;     // to use fireball's bounding box coordinates
    x = 0x04;

    do // FireballEnemyCDLoop
    {
        writeData(0x01, x); // store enemy object offset here
        a = y;
        pha();                               // push fireball offset to the stack
        a = M(Enemy_State + x) & 0b00100000; // check to see if d5 is set in enemy state
        if (a != 0)
        {
            goto NoFToECol; // if so, skip to next enemy slot
        }
        // check to see if buffer flag is set
        if (M(Enemy_Flag + x) == 0)
        {
            goto NoFToECol; // if not, skip to next enemy slot
        }
        a = M(Enemy_ID + x); // check enemy identifier
        if (a >= 0x24)
        { // if < $24, branch to check further
            if (a < 0x2b)
            {
                goto NoFToECol; // if in range $24-$2a, skip to next enemy slot
            }
        } // GoombaDie: check for goomba identifier
        if (a == Goomba)
        { // if not found, continue with code
            // otherwise check for defeated state
            if (M(Enemy_State + x) >= 0x02)
            {
                goto NoFToECol; // skip to next enemy slot
            }
        } // NotGoomba: if any masked offscreen bits set,
        if (M(EnemyOffscrBitsMasked + x) != 0)
        {
            goto NoFToECol; // skip to next enemy slot
        }
        a = x;
        a <<= 1; // otherwise multiply enemy offset by four
        a <<= 1;
        a += 0x04;                                     // add 4 bytes to it
        x = a;                                         // to use enemy's bounding box coordinates
        collisionFound = SprObjectCollisionCore(x, y); // do fireball-to-enemy collision detection
        x = M(ObjectOffset);                           // return fireball's original offset
        if (!collisionFound)
        {
            goto NoFToECol; // no collision, thus do next enemy slot
        }
        a = 0b10000000;
        writeData(Fireball_State + x, 0b10000000); // set d7 in enemy state
        x = M(0x01);                               // get enemy offset
        HandleEnemyFBallCol();                     // jump to handle fireball to enemy collision

    NoFToECol: // pull fireball offset from stack
        pla();
        y = a;       // put it in Y
        x = M(0x01); // get enemy object offset
        --x;         // decrement it
    } while ((x & 0x80) == 0); // loop back until collision detection done on all enemies

ExitFBallEnemy:
    x = M(ObjectOffset); // get original fireball offset and leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (same value already stored at M(0x01); consumed by
// RelativeEnemyPosition() before being reloaded from M(0x01))
// Outputs: none
void SMBEngine::HandleEnemyFBallCol()
{
    RelativeEnemyPosition(x); // get relative coordinate of enemy
    x = M(0x01);             // get current enemy object offset
    a = M(Enemy_Flag + x);   // check buffer flag for d7 set
    if ((a & 0x80) != 0)
    {                    // branch if not set to continue
        a &= 0b00001111; // otherwise mask out high nybble and
        x = a;           // use low nybble as enemy offset
        a = M(Enemy_ID + x);
        if (a == Bowser)
        {
            HurtBowser(); // branch if found
            return;
        }
        x = M(0x01); // otherwise retrieve current enemy offset
    } // ChkBuzzyBeetle
    a = M(Enemy_ID + x);
    if (a == BuzzyBeetle)
    {
        return; // branch if found to leave (buzzy beetles fireproof)
    }
    if (a != Bowser)
    {
        goto ChkOtherEnemies;
    }
    HurtBowser();
    return;

ChkOtherEnemies:
    if (a == BulletBill_FrenzyVar)
    {
        return; // branch to leave if bullet bill (frenzy variant)
    }
    if (a == Podoboo)
    {
        return; // branch to leave if podoboo
    }
    if (a >= 0x15)
    {
        return; // branch to leave if identifier => $15
    }
    ShellOrBlockDefeat(x);
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::RunGameTimer()
{
    a = M(OperMode); // get primary mode of operation
    if (a == 0)
    {
        return; // branch to leave if in title screen mode
    }
    a = M(GameEngineSubroutine);
    if (a < 0x08)
    {
        return; // branch to leave
    }
    if (a == 0x0b)
    {
        return; // branch to leave
    }
    a = M(Player_Y_HighPos);
    if (a >= 0x02)
    {
        return; // branch to leave regardless of level type
    }
    a = M(GameTimerCtrlTimer); // if game timer control not yet expired,
    if (a != 0)
    {
        return; // branch to leave
    }
    a = M(GameTimerDisplay) | M(GameTimerDisplay + 1); // otherwise check game timer digits
    a |= M(GameTimerDisplay + 2);
    if (a != 0)
    {                            // if game timer digits at 000, branch to time-up code
        y = M(GameTimerDisplay); // otherwise check first digit
        --y;                     // if first digit not on 1,
        if (y != 0)
        {
            goto ResGTCtrl; // branch to reset game timer control
        }
        // otherwise check second and third digits
        a = M(GameTimerDisplay + 1) | M(GameTimerDisplay + 2);
        if (a != 0)
        {
            goto ResGTCtrl; // if timer not at 100, branch to reset game timer control
        }
        a = TimeRunningOutMusic;
        writeData(EventMusicQueue, TimeRunningOutMusic); // otherwise load time running out music

    ResGTCtrl: // reset game timer control
        writeData(GameTimerCtrlTimer, 0x18);
        y = 0x23; // set offset for last digit
        a = 0xff; // set value to decrement game timer digit
        writeData(DigitModifier + 5, 0xff);
        DigitsMathRoutine(y);     // do sub to decrement game timer slowly
        a = 0xa4;                 // set status nybbles to update game timer display
        PrintStatusBarNumbers(a); // do sub to update the display
        return;
    } // TimeUpOn: init player status (note A will always be zero here)
    writeData(PlayerStatus, a);
    ForceInjury(0);            // do sub to kill the player (note player is small here)
    ++M(GameTimerExpiredFlag); // set game timer expiration flag

    // ExGTimer: leave
}

//------------------------------------------------------------------------

// Inputs: x = misc object buffer offset (hammer slot)
// Outputs: none
void SMBEngine::ProcHammerObj()
{
    const uint8_t HammerXSpdData_data[] = {0x10, 0xf0};

    uint32_t wide = 0;

    // if master timer control set
    if (M(TimerControl) != 0)
    {
        goto RunHSubs; // skip all of this code and go to last subs at the end
    }
    // otherwise get hammer's state
    a = M(Misc_State + x) & 0b01111111; // mask out d7
    y = M(HammerEnemyOffset + x);       // get enemy object offset that spawned this hammer
    if (a != 0x02)
    { // if currently at 2, branch
        if (a >= 0x02)
        {
            goto SetHPos; // if greater than 2, branch elsewhere
        }
        a = x;
        a += 0x0d;                 // proper misc object
        x = a;                     // return offset to X
        writeData(0x00, 0x10);     // set downward movement force
        writeData(0x01, 0x0f);     // set upward movement force (not used)
        writeData(0x02, 0x04);     // set maximum vertical speed
        a = 0x00;                  // set A to impose gravity on hammer
        ImposeGravity(a, x);       // do sub to impose gravity on hammer and move vertically
        MoveObjectHorizontally(x); // do sub to move it horizontally
        x = M(ObjectOffset);       // get original misc object offset
    } // SetHSpd
    else // branch to essential subroutines
    {
        writeData(Misc_Y_Speed + x, 0xfe); // set hammer's vertical speed
        // get enemy object state
        a = M(Enemy_State + y) & 0b11110111; // mask out d3
        writeData(Enemy_State + y, a);       // store new state
        x = M(Enemy_MovingDir + y);          // get enemy's moving direction
        --x;                                 // decrement to use as offset
        a = HammerXSpdData_data[x];          // get proper speed to use based on moving direction
        x = M(ObjectOffset);                 // reobtain hammer's buffer offset
        writeData(Misc_X_Speed + x, a);      // set hammer's horizontal speed

    SetHPos: // decrement hammer's state
        --M(Misc_State + x);
        // get enemy's horizontal position
        wide = ((M(Enemy_PageLoc + y) << 8) | M(Enemy_X_Position + y)) + 0x02; // set position 2 pixels to the right
        writeData(Misc_X_Position + x, LOBYTE(wide));                          // store as hammer's horizontal position
        writeData(Misc_PageLoc + x, HIBYTE(wide));                             // store as hammer's page location
        a = HIBYTE(wide);                                                      // get enemy's page location
        a = M(Enemy_Y_Position + y);                                           // get enemy's vertical position
        a -= 0x0a;                                                             // move position 10 pixels upward
        writeData(Misc_Y_Position + x, a);                                     // store as hammer's vertical position
        a = 0x01;
        writeData(Misc_Y_HighPos + x, 0x01); // set hammer's vertical high byte
        if (a != 0)
        {
            goto RunHSubs; // unconditional branch to skip first routine
        }
    } // RunAllH: handle collisions
    PlayerHammerCollision();

RunHSubs: // get offscreen information
    GetMiscOffscreenBits(x);
    RelativeMiscPosition(x); // get relative coordinates
    GetMiscBoundBox(x);      // get bounding box coordinates
    DrawHammer();            // draw the hammer
    // and we are done here
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::MiscObjectsCore()
{
    bool shiftedBit = false;
    uint32_t wide = 0;

    x = 0x08; // set at end of misc object buffer

    do // MiscLoop: store misc object offset here
    {
        writeData(ObjectOffset, x);
        a = M(Misc_State + x); // check misc object state
        if (a == 0)
        {
            goto MiscLoopBack; // branch to check next slot
        }
        shiftedBit = (a & 0x80) != 0;
        a <<= 1; // otherwise take d7
        if (shiftedBit)
        {                      // if d7 not set, jumping coin, thus skip to rest of code here
            ProcHammerObj();   // otherwise go to process hammer,
            goto MiscLoopBack; // then check next slot
        } // ProcJumpCoin
        y = M(Misc_State + x); // check misc object state
        --y;                   // decrement to see if it's set to 1
        if (y != 0)
        {                        // if so, branch to handle jumping coin
            ++M(Misc_State + x); // otherwise increment state to either start off or as timer
            // get horizontal coordinate for misc object
            wide = ((M(Misc_PageLoc + x) << 8) | M(Misc_X_Position + x)) + M(ScrollAmount); // add current scroll speed
            writeData(Misc_X_Position + x, LOBYTE(wide));                                   // store as new horizontal coordinate
            writeData(Misc_PageLoc + x, HIBYTE(wide));                                      // store as new page location
            a = HIBYTE(wide);                                                               // get page location
            if (M(Misc_State + x) != 0x30)
            {
                goto RunJCSubs; // if not yet reached, branch to subroutines
            }
            a = 0x00;
            writeData(Misc_State + x, 0x00); // otherwise nullify object state
            goto MiscLoopBack;               // and move onto next slot
        } // JCoinRun
        a = x;
        a += 0x0d;
        x = a;
        // set downward movement amount
        writeData(0x00, 0x50);
        // set maximum vertical speed
        writeData(0x02, 0x06);
        // divide by 2 and set
        writeData(0x01, 0x03); // as upward movement amount (apparently residual)
        a = 0x00;              // set A to impose gravity on jumping coin
        ImposeGravity(a, x);   // do sub to move coin vertically and impose gravity on it
        x = M(ObjectOffset);   // get original misc object offset
        // check vertical speed
        if (M(Misc_Y_Speed + x) != 0x05)
        {
            goto RunJCSubs; // if not moving downward fast enough, keep state as-is
        }
        ++M(Misc_State + x); // otherwise increment state to change to floatey number

    RunJCSubs: // get relative coordinates
        RelativeMiscPosition(x);
        GetMiscOffscreenBits(x); // get offscreen information
        GetMiscBoundBox(x);      // get bounding box coordinates (why?)
        JCoinGfxHandler();      // draw the coin or floatey number

    MiscLoopBack:
        --x; // decrement misc object offset
    } while ((x & 0x80) == 0); // loop back until all misc objects handled
    // then leave
}

//------------------------------------------------------------------------

// Inputs: x = misc object buffer offset (hammer slot)
// Outputs: x = M(ObjectOffset) (restored misc object offset)
void SMBEngine::PlayerHammerCollision()
{
    bool collisionFound = false;

    // get frame counter
    a = M(FrameCounter) >> 1; // take d0
    if ((M(FrameCounter) & 0x01) == 0)
    {
        return; // branch to leave if d0 not set to execute every other frame
    }
    // if either master timer control
    a = M(TimerControl) | M(Misc_OffscreenBits); // or any offscreen bits for hammer are set,
    if (a != 0)
    {
        return; // branch to leave
    }
    a = x;
    a <<= 1; // multiply misc object offset by four
    a <<= 1;
    a += 0x24;                               // add 36 or $24 bytes to get proper offset
    y = a;                                   // for misc object bounding box coordinates
    collisionFound = PlayerCollisionCore(y); // do player-to-hammer collision detection
    x = M(ObjectOffset);                     // get misc object offset
    if (collisionFound)
    {                                   // if no collision, then branch
        a = M(Misc_Collision_Flag + x); // otherwise read collision flag
        if (a != 0)
        {
            return; // if collision flag already set, branch to leave
        }
        writeData(Misc_Collision_Flag + x, 0x01); // otherwise set collision flag now
        a = M(Misc_X_Speed + x) ^ 0xff;           // get two's compliment of
        a += 0x01;
        writeData(Misc_X_Speed + x, a); // set to send hammer flying the opposite direction
        a = M(StarInvincibleTimer);     // if star mario invincibility timer set,
        if (a != 0)
        {
            return; // branch to leave
        }
        InjurePlayer(); // otherwise jump to hurt player, do not return
        return;
    } // ClHCol: clear collision flag
    a = 0x00;
    writeData(Misc_Collision_Flag + x, 0x00);

    // ExPHC
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ProcessCannons()
{
    const uint8_t CannonBitmasks_data[] = {0b00001111, 0b00000111};

    a = M(AreaType); // get area type
    if (a != 0)
    { // if water type area, branch to leave
        x = 0x02;

        do // ThreeSChk: start at third enemy slot
        {
            writeData(ObjectOffset, x);
            // check enemy buffer flag
            if (M(Enemy_Flag + x) != 0)
            {
                goto Chk_BB; // if set, branch to check enemy
            }
            // otherwise get part of LSFR
            y = M(SecondaryHardMode);                                   // get secondary hard mode flag, use as offset
            a = M(PseudoRandomBitReg + 1 + x) & CannonBitmasks_data[y]; // mask out bits of LSFR as decided by flag
            if (a >= 0x06)
            {
                goto Chk_BB; // if so, branch to check enemy
            }
            y = a; // transfer masked contents of LSFR to Y as pseudorandom offset
            // get page location
            if (M(Cannon_PageLoc + y) == 0)
            {
                goto Chk_BB; // if not set or on page 0, branch to check enemy
            }
            a = M(Cannon_Timer + y); // get cannon timer
            if (a != 0)
            {                                   // if expired, branch to fire cannon
                --a;                            // otherwise subtract the borrow (note carry will always be clear here)
                writeData(Cannon_Timer + y, a); // to count timer down
                goto Chk_BB;                    // then jump ahead to check enemy
            } // FireCannon
            // if master timer control set,
            if (M(TimerControl) != 0)
            {
                goto Chk_BB; // branch to check enemy
            }
            // otherwise we start creating one
            writeData(Cannon_Timer + y, 0x0e); // first, reset cannon timer
            // get page location of cannon
            writeData(Enemy_PageLoc + x, M(Cannon_PageLoc + y)); // save as page location of bullet bill
            // get horizontal coordinate of cannon
            writeData(Enemy_X_Position + x, M(Cannon_X_Position + y)); // save as horizontal coordinate of bullet bill
            a = M(Cannon_Y_Position + y);                              // get vertical coordinate of cannon
            a -= 0x08;                                                 // subtract eight pixels (because enemies are 24 pixels tall)
            writeData(Enemy_Y_Position + x, a);                        // save as vertical coordinate of bullet bill
            writeData(Enemy_Y_HighPos + x, 0x01);                      // set vertical high byte of bullet bill
            writeData(Enemy_Flag + x, 0x01);                           // set buffer flag
            writeData(Enemy_State + x, 0x00);                          // then initialize enemy's state
            writeData(Enemy_BoundBoxCtrl + x, 0x09);                   // set bounding box size control for bullet bill
            a = BulletBill_CannonVar;
            writeData(Enemy_ID + x, BulletBill_CannonVar); // load identifier for bullet bill (cannon variant)
            goto Next3Slt;                                 // move onto next slot

        Chk_BB: // check enemy identifier for bullet bill (cannon variant)
            a = M(Enemy_ID + x);
            if (a != BulletBill_CannonVar)
            {
                goto Next3Slt; // if not found, branch to get next slot
            }
            OffscreenBoundsCheck(x); // otherwise, check to see if it went offscreen
            a = M(Enemy_Flag + x);   // check enemy buffer flag
            if (a == 0)
            {
                goto Next3Slt; // if not set, branch to get next slot
            }
            GetEnemyOffscreenBits(x); // otherwise, get offscreen information
            BulletBillHandler();     // then do sub to handle bullet bill

        Next3Slt: // move onto next slot
            --x;
        } while ((x & 0x80) == 0); // do this until first three slots are checked
    } // ExCannon: then leave
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (cannon variant bullet bill slot)
// Outputs: none
void SMBEngine::BulletBillHandler()
{
    const uint8_t BulletBillXSpdData_data[] = {0x18, 0xe8};

    bool enemyRightOfPlayer = false;

    // if master timer control set,
    if (M(TimerControl) == 0)
    { // branch to run subroutines except movement sub
        if (M(Enemy_State + x) == 0)
        { // if bullet bill's state set, branch to check defeated state
            // otherwise load offscreen bits
            a = M(Enemy_OffscreenBits) & 0b00001100; // mask out bits
            if (a == 0b00001100)
            {
                goto KillBB; // if so, branch to kill this object
            }
            y = 0x01;                                             // set to move right by default
            std::tie(enemyRightOfPlayer, a) = PlayerEnemyDiff(x); // get horizontal difference between player and bullet bill
            if ((a & 0x80) == 0)
            {        // if enemy to the left of player, branch
                ++y; // otherwise increment to move left
            } // SetupBB: set bullet bill's moving direction
            writeData(Enemy_MovingDir + x, y);
            --y; // decrement to use as offset
            // get horizontal speed based on moving direction
            writeData(Enemy_X_Speed + x, BulletBillXSpdData_data[y]); // and store it
            a = (uint8_t)(M(0x00) + 0x28 +
                          (enemyRightOfPlayer ? 1 : 0)); // get horizontal difference, add 40 pixels plus the no-borrow left above
            if (a < 0x50)
            {
                goto KillBB; // to cannon either on left or right side, thus branch
            }
            writeData(Enemy_State + x, 0x01);     // otherwise set bullet bill's state
            writeData(EnemyFrameTimer + x, 0x0a); // set enemy frame timer
            a = Sfx_Blast;
            writeData(Square2SoundQueue, Sfx_Blast); // play fireworks/gunfire sound
        } // ChkDSte: check enemy state for d5 set
        a = M(Enemy_State + x) & 0b00100000;
        if (a != 0)
        {                             // if not set, skip to move horizontally
            MoveD_EnemyVertically(x); // otherwise do sub to move bullet bill vertically
        } // BBFly: do sub to move bullet bill horizontally
        MoveEnemyHorizontally(x);
    } // RunBBSubs: get offscreen information
    GetEnemyOffscreenBits(x);
    RelativeEnemyPosition(x); // get relative coordinates
    GetEnemyBoundBox(x);     // get bounding box coordinates
    PlayerEnemyCollision(x); // handle player to enemy collisions
    EnemyGfxHandler(x);      // draw the bullet bill and leave
    return;

KillBB: // kill bullet bill and leave
    EraseEnemyObject(x);
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::GameCoreRoutine()
{
    x = M(CurrentPlayer); // get which player is on the screen
    // use appropriate player's controller bits
    writeData(SavedJoypadBits, M(SavedJoypadBits + x)); // as the master controller bits
    GameRoutines();                                     // execute one of many possible subs
    a = M(OperMode_Task);                               // check major task of operating mode
    if (a < 0x03)
    { // branch to the game engine itself
        return;

        //------------------------------------------------------------------------
    } // GameEngine
    ProcFireball_Bubble(); // process fireballs and air bubbles
    uint8_t i = 0x00;

    do // ProcELoop: put incremented offset in X as enemy object offset
    {
        writeData(ObjectOffset, i);
        EnemiesAndLoopsCore(i);  // process enemy objects
        x = i;                   // FloateyNumbersRoutine is still register-based
        FloateyNumbersRoutine(); // process floatey numbers
        ++i;
    } while (i != 0x06);
    GetPlayerOffscreenBits(); // get offscreen bits for player object
    RelativePlayerPosition(); // get relative coordinates for player object
    PlayerGfxHandler();       // draw the player
    BlockObjMT_Updater();     // replace block objects with metatiles if necessary
    x = 0x01;
    writeData(ObjectOffset, 0x01); // set offset for second
    BlockObjectsCore();            // process second block object
    --x;
    writeData(ObjectOffset, x); // set offset for first
    BlockObjectsCore();         // process first block object
    MiscObjectsCore();          // process misc objects (hammer, jumping coins)
    ProcessCannons();           // process bullet bill cannons
    ProcessWhirlpools();        // process whirlpools
    FlagpoleRoutine();          // process the flagpole
    RunGameTimer();             // count down the game timer
    ColorRotation();            // cycle one of the background colors
    if (((M(Player_Y_HighPos) - 0x02) & 0x80) == 0)
    {
        goto NoChgMus;
    }
    a = M(StarInvincibleTimer); // if star mario invincibility timer at zero,
    if (a != 0)
    { // skip this part
        if (a != 0x04)
        {
            goto NoChgMus; // if not yet at a certain point, continue
        }
        // if interval timer not yet expired,
        if (M(IntervalTimerControl) != 0)
        {
            goto NoChgMus; // branch ahead, don't bother with the music
        }
        GetAreaMusic(); // to re-attain appropriate level music

    NoChgMus: // get invincibility timer
        y = M(StarInvincibleTimer);
        a = M(FrameCounter); // get frame counter
        if (y < 0x08)
        {            // branch to cycle player's palette quickly
            a >>= 1; // otherwise, divide by 8 to cycle every eighth frame
            a >>= 1;
        } // CycleTwo: if branched here, divide by 2 to cycle every other frame
        a >>= 1;
        CyclePlayerPalette(); // do sub to cycle the palette (note: shares fire flower code)
    } // ClrPlrPal: do sub to clear player's palette bits in attributes
    else // then skip this sub to finish up the game engine
    {
        ResetPalStar();
    } // SaveAB: save current A and B button
    writeData(PreviousA_B_Buttons, M(A_B_Buttons)); // into temp variable to be used on next frame
    a = 0x00;
    writeData(Left_Right_Buttons, 0x00); // nullify left and right buttons temp variable
    UpdScrollVar();
}
