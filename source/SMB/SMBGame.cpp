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

    // get frame counter, mask out all but three LSB
    if ((M(FrameCounter) & 0x07) != 0)
    {
        return; // branch if not set to zero to do this every eighth frame
    }
    // check vram buffer offset
    if (M(VRAM_Buffer1_Offset) >= 49)
    {
        return; // if offset over 48 bytes, branch to leave
    }
    uint8_t bufferOfs = M(VRAM_Buffer1_Offset);
    for (uint8_t i = 0; i < 8; ++i)
    { // GetBlankPal: get blank palette for palette 3
        // store it in the vram buffer, until all bytes are copied
        ram[VRAM_Buffer1 + bufferOfs + i] = BlankPalette_data[i];
    }
    bufferOfs = M(VRAM_Buffer1_Offset); // get current vram buffer offset
    uint8_t counter = 3;                // set counter here
    uint8_t palOfs = M(AreaType) << 2;  // get area type, multiply by 4 to get proper offset

    do // GetAreaPal: fetch palette to be written based on area type
    {
        ram[VRAM_Buffer1 + 3 + bufferOfs] = Palette3Data_data[palOfs]; // store it to overwrite blank palette in vram buffer
        ++palOfs;
        ++bufferOfs;
        --counter; // decrement counter
    } while ((counter & 0x80) == 0); // do this until the palette is all copied
    bufferOfs = M(VRAM_Buffer1_Offset); // get current vram buffer offset
    // get and store current color in second slot of palette, using the color cycling offset
    ram[VRAM_Buffer1 + 4 + bufferOfs] = ColorRotatePalette_data[M(ColorRotateOffset)];
    ram[VRAM_Buffer1_Offset] = bufferOfs + 7;
    ++M(ColorRotateOffset); // increment color cycling offset
    if (M(ColorRotateOffset) >= 6)
    {
        ram[ColorRotateOffset] = 0; // otherwise, init to keep it in range
    }
    // ExitColorRot: leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::GetAreaMusic()
{
    const uint8_t MusicSelectData_data[] = {WaterMusic, GroundMusic, UndergroundMusic, CastleMusic, CloudMusic, PipeIntroMusic};

    if (M(OperMode) == TitleScreenModeValue)
    {
        return; // if in title screen mode, leave
    }
    uint8_t musicIdx;
    // check for specific alternate mode of entry from area object data header,
    // and check value from level header for certain values
    if (M(AltEntranceControl) != 2 && (M(PlayerEntranceCtrl) == 6 || M(PlayerEntranceCtrl) == 7))
    {
        musicIdx = 5; // load music for pipe intro scene if header
    }
    else if (M(CloudTypeOverride) != 0)
    {                 // check for cloud type override
        musicIdx = 4; // select music for cloud type level if found
    }
    else
    { // ChkAreaType: load area type as offset for music bit
        musicIdx = M(AreaType);
    }
    // StoreMusic: otherwise select appropriate music for level type
    ram[AreaMusicQueue] = MusicSelectData_data[musicIdx]; // store in queue and leave
    // ExitGetM
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none beyond the bool return
bool SMBEngine::TransposePlayers()
{
    // end the game by default
    if (M(NumberOfPlayers) == 0)
    {
        return true; // if only a 1 player game, leave
    }
    if ((M(OffScr_NumberofLives) & 0x80) != 0)
    {
        return true; // branch if offscreen player has no lives left
    }
    // invert bit to update which player is on the screen
    ram[CurrentPlayer] = M(CurrentPlayer) ^ 0b00000001;

    for (uint8_t i = 6; (i & 0x80) == 0; --i)
    { // TransLoop: transpose the information of the onscreen player
        // with that of the offscreen player
        uint8_t saved = M(OnscreenPlayerInfo + i);
        ram[OnscreenPlayerInfo + i] = M(OffscreenPlayerInfo + i);
        ram[OffscreenPlayerInfo + i] = saved;
    }
    return false; // ExTrans: get the game going
}

//------------------------------------------------------------------------

// Inputs: a = signed offset to add to Player_Y_Position
// Outputs: none
void SMBEngine::MovePlayerYAxis(uint8_t amount)
{
    // add amount to player position
    ram[Player_Y_Position] = amount + M(Player_Y_Position);
}

//------------------------------------------------------------------------

// Inputs: slot = air bubble object buffer offset
// Outputs: none
void SMBEngine::DrawBubble(uint8_t slot)
{
    // if player's vertical high position not within screen, skip all of this
    if (M(Player_Y_HighPos) != 1)
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
    ram[Sprite_X_Position + oamSlot] = M(Bubble_Rel_XPos); // store as X coordinate here
    // get relative vertical coordinate
    ram[Sprite_Y_Position + oamSlot] = M(Bubble_Rel_YPos); // store as Y coordinate here
    ram[Sprite_Tilenumber + oamSlot] = 0x74;               // put air bubble tile into OAM data
    ram[Sprite_Attributes + oamSlot] = 0x02;               // set attribute byte

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
    return baseIdx + 8; // for small player, use current offset + 8
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ChkForPlayerAttrib()
{
    const uint8_t oamSlot = M(Player_SprDataOffset); // get sprite data offset

    // KilledAtt: whether the third row of sprites is modified too
    bool thirdRowToo = true;
    if (M(GameEngineSubroutine) != Gs_PlayerDeath)
    {                                                 // branch to change third and fourth row OAM attributes
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
        ram[Sprite_Attributes + 16 + oamSlot] = M(Sprite_Attributes + 16 + oamSlot) & 0b00111111;
        // set horizontal flip bit for second sprite in the third row
        ram[Sprite_Attributes + 20 + oamSlot] = (M(Sprite_Attributes + 20 + oamSlot) & 0b00111111) | 0b01000000;
    }
    // C_S_IGAtt: mask out horizontal and vertical flip bits
    // for fourth row sprites and save
    ram[Sprite_Attributes + 24 + oamSlot] = M(Sprite_Attributes + 24 + oamSlot) & 0b00111111;
    // set horizontal flip bit for second sprite in the fourth row
    ram[Sprite_Attributes + 28 + oamSlot] = (M(Sprite_Attributes + 28 + oamSlot) & 0b00111111) | 0b01000000;

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
    // check for water type level
    if (M(AreaType) != 0)
    {
        return; // branch to leave if not found
    }
    ram[Whirlpool_Flag] = 0; // otherwise initialize whirlpool flag
    if (M(TimerControl) != 0)
    {
        return; // branch to leave if master timer control set
    }
    // otherwise start with last whirlpool data,
    // and do this until all whirlpools are checked
    for (uint8_t i = 4; (i & 0x80) == 0; --i) // WhLoop
    {
        // get left extent of whirlpool, add length of whirlpool
        uint32_t wide = ((M(Whirlpool_PageLoc + i) << 8) | M(Whirlpool_LeftExtent + i)) + M(Whirlpool_Length + i);
        // get page location
        if (M(Whirlpool_PageLoc + i) == 0)
        {
            continue; // NextWh: if none or page 0, branch to get next data
        }
        const uint16_t rightExtent = (uint16_t)wide; // page location and coordinate of the right extent
        // the player and the left extent are each one 16-bit page:coordinate
        wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) -
               ((M(Whirlpool_PageLoc + i) << 8) | M(Whirlpool_LeftExtent + i)); // subtract left extent
        if ((HIBYTE(wide) & 0x80) != 0)
        {
            continue; // NextWh: if player too far left, branch to get next data
        }
        // the right extent and the player are each one 16-bit page:coordinate
        wide = rightExtent                                          // otherwise get right extent
               - ((M(Player_PageLoc) << 8) | M(Player_X_Position)); // subtract player's horizontal coordinate
        if ((HIBYTE(wide) & 0x80) != 0)
        {
            continue; // NextWh: if player past right extent, branch to get next data
        }
        // WhirlpoolActivate: if player within right extent, run whirlpool code
        // get length of whirlpool, divide by 2 and save here
        const uint8_t halfLength = M(Whirlpool_Length + i) >> 1;
        // get left extent of whirlpool, add length divided by 2
        wide = ((M(Whirlpool_PageLoc + i) << 8) | M(Whirlpool_LeftExtent + i)) + halfLength;
        const uint16_t center = (uint16_t)wide; // page location and coordinate of the whirlpool center
        // get frame counter, check d0 (to run on every other frame)
        if ((M(FrameCounter) & 0x01) != 0)
        {
            // the center and the player are each one 16-bit page:coordinate
            wide = center                                               // get center
                   - ((M(Player_PageLoc) << 8) | M(Player_X_Position)); // subtract player's horizontal coordinate
            if ((HIBYTE(wide) & 0x80) != 0)
            {                                                                 // if player to the left of center, branch
                wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) - 1; // otherwise slowly pull player left, towards the center
                ram[Player_X_Position] = LOBYTE(wide);                        // set player's new horizontal coordinate
                ram[Player_PageLoc] = HIBYTE(wide);                           // set player's new page location
            } // LeftWh: get player's collision bits, take d0
            else if ((M(Player_CollisionBits) & 0x01) != 0)
            { // if d0 set: slowly pull player right, towards the center
                wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + 1;
                ram[Player_X_Position] = LOBYTE(wide); // set player's new horizontal coordinate
                ram[Player_PageLoc] = HIBYTE(wide);    // SetPWh: set player's new page location
            }
        }
        // WhPull
        ram[Whirlpool_Flag] = 1; // set whirlpool flag to be used later
        // jump to put whirlpool effect on player vertically, do not return; vertical movement
        // force of 16 and maximum vertical speed of 1
        ImposeGravity(0x00, 0x00, 0x10, 0x00, 0x01);
        return;
    }
    // ExitWh: leave
}

//------------------------------------------------------------------------

// Inputs: metatile = metatile to write; blockOffset = block object buffer offset;
// cell = the block buffer cell the block object occupies
// Outputs: none
void SMBEngine::ReplaceBlockMetatile(uint8_t metatile, uint8_t blockOffset, BlockBufferCell cell)
{
    WriteBlockMetatile(metatile, cell); // write metatile to vram buffer to replace block object
    ++M(Block_ResidualCounter);         // increment unused counter (residual code)
    --M(Block_RepFlag + blockOffset);   // decrement flag (residual code)
    // leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::LoadAreaPointer()
{
    // FindAreaPointer, inlined
    // load offset from world variable, add area number for our area pointer
    uint8_t offset = M(WorldAddrOffsets + M(WorldNumber)) + M(AreaNumber);
    ram[AreaPointer] = M(AreaAddrOffsets + offset);

    GetAreaType(M(AreaPointer));
}

//------------------------------------------------------------------------

// mask out all but d6 and d5
// Inputs: areaPointerByte = area pointer byte
// Outputs: none (area type, 2 bits, stored to AreaType)
void SMBEngine::GetAreaType(uint8_t areaPointerByte)
{
    // make %0xx00000 into %000000xx and save 2 MSB as area type
    ram[AreaType] = (areaPointerByte & 0b01100000) >> 5;
}

//------------------------------------------------------------------------

// Inputs: bits = raw palette bits to cycle in (only d1-d0 used)
// Outputs: none
void SMBEngine::CyclePlayerPalette(uint8_t bits)
{
    const uint8_t paletteBits = bits & 0x03; // mask out all but d1-d0, use as palette bits
    // get player attributes, save any other bits but palette bits, add palette bits
    ram[Player_SprAttrib] = (M(Player_SprAttrib) & 0b11111100) | paletteBits;
    // store as new player attributes and leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ResetPalStar()
{
    // get player attributes, mask out palette bits to force palette 0
    ram[Player_SprAttrib] = M(Player_SprAttrib) & 0b11111100;
    // store as new player attributes and leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::BlockObjMT_Updater()
{
    uint8_t slot = 1; // set offset to start with second block object

    do // UpdateLoop: set offset here
    {
        ram[ObjectOffset] = slot;
        // if vram buffer not already being used here, and the flag for block object
        // is set,
        if (M(VRAM_Buffer1) == 0 && M(Block_RepFlag + slot) != 0)
        {
            // compose the block buffer address from the low byte saved earlier, with a
            // high byte of $05
            const uint16_t blockBufferAddr = (uint16_t)(0x0500 | M(Block_BBuf_Low + slot));
            const uint8_t vertOfs = M(Block_Orig_YPos + slot); // get original vertical coordinate of block object
            const uint8_t metatile = M(Block_Metatile + slot); // get metatile to be written
            ram[blockBufferAddr + vertOfs] = metatile;         // write it to the block buffer
            // do sub to replace metatile where block object is
            ReplaceBlockMetatile(metatile, slot, {vertOfs, blockBufferAddr});
            ram[Block_RepFlag + slot] = 0; // clear block object flag
        }
        // NextBUpd: decrement block object offset
        --slot;
    } while ((slot & 0x80) == 0); // do this until both block objects are dealt with
    // then leave
}

//------------------------------------------------------------------------

// Inputs: slot = block object buffer offset
// Outputs: none (delegates to ImposeBlockGravity with maximum-speed index 1)
void SMBEngine::ImposeGravityBlock(uint8_t slot)
{
    // set offset for maximum speed
    ImposeBlockGravity(1, slot);
}

//------------------------------------------------------------------------

// Inputs: maxSpeedIdx = index into MaxSpdBlockData_data (0 or 1); objectOffset = object buffer
// offset forwarded to ImposeGravitySprObj
// Outputs: none
void SMBEngine::ImposeBlockGravity(uint8_t maxSpeedIdx, uint8_t objectOffset)
{
    const uint8_t MaxSpdBlockData_data[] = {0x06, 0x08};

    // get maximum speed; the movement amount is 0x50
    ImposeGravitySprObj(MaxSpdBlockData_data[maxSpeedIdx], objectOffset, 0x50);
}

//------------------------------------------------------------------------

// Inputs: slot = fireball object buffer offset
// Outputs: the metatile found beneath the fireball, forwarded from BlockBufferCollision via
// ResJmpM/BBChk_E
uint8_t SMBEngine::BlockBufferChk_FBall(uint8_t slot)
{
    // add seven bytes to use, and 0x1a is the block buffer adder data offset
    return ResJmpM(slot + 0x07, 0x1a);
}

//------------------------------------------------------------------------

// Inputs: objectOffset = sprite object offset; cornerIdx = corner-selector index (forwarded to
// BBChk_E/BlockBufferCollision)
// Outputs: return value = the metatile found (see BlockBufferCollision); not every caller reads it
// (e.g. BlockBufferChk_Enemy's chain ignores it), but BlockBufferChk_FBall's caller does
uint8_t SMBEngine::ResJmpM(uint8_t objectOffset, uint8_t cornerIdx) { return BBChk_E(0, objectOffset, cornerIdx).metatile; }

//------------------------------------------------------------------------

// Inputs: slot = fireball object buffer offset
// Outputs: none
void SMBEngine::DrawFireball(uint8_t slot)
{
    const uint8_t oamSlot = M(FBall_SprDataOffset + slot); // get fireball's sprite data offset
    // get relative vertical coordinate
    ram[Sprite_Y_Position + oamSlot] = M(Fireball_Rel_YPos); // store as sprite Y coordinate
    // get relative horizontal coordinate
    ram[Sprite_X_Position + oamSlot] = M(Fireball_Rel_XPos); // store as sprite X coordinate, then do shared code

    DrawFirebar(oamSlot);
}

//------------------------------------------------------------------------

// Inputs: rows = number of sprite rows to draw
// Outputs: none (delegates to DrawPlayerLoop)
void SMBEngine::RenderPlayerSub(uint8_t rows)
{
    const uint8_t relXPos = M(Player_Rel_XPos);
    ram[Player_Pos_ForScroll] = relXPos; // store player's relative horizontal position
    // hand the graphics table offset and the player's sprite data offset to DrawPlayerLoop, along
    // with the player's facing direction, sprite attributes and horizontal/vertical position
    DrawPlayerLoop(M(PlayerGfxOffset), M(Player_SprDataOffset), M(PlayerFacingDir), M(Player_SprAttrib), relXPos, M(Player_Rel_YPos), rows);
}

//------------------------------------------------------------------------

// Inputs: gfxOffset = player graphics table offset; sprDataOffset = player sprite data offset;
// flipBits, attributeBits, xPos, yPos = forwarded to DrawOneSpriteRow; rows = number of sprite
// rows to draw
// Outputs: none
void SMBEngine::DrawPlayerLoop(uint8_t gfxOffset, uint8_t sprDataOffset, uint8_t flipBits, uint8_t attributeBits, uint8_t xPos,
                               uint8_t yPos, uint8_t rows)
{
    uint8_t spritePairIdx = gfxOffset;
    uint8_t oamSlot = sprDataOffset;

    do // DrawPlayerLoop: load player's left side
    {
        // load player's left side, then the right side
        std::tie(spritePairIdx, oamSlot) =
            DrawOneSpriteRow(M(PlayerGraphicsTable + spritePairIdx), M(PlayerGraphicsTable + 1 + spritePairIdx), spritePairIdx, oamSlot,
                             flipBits, attributeBits, xPos, yPos);
        --rows; // decrement rows of sprites to draw
    } while (rows != 0); // do this until all rows are drawn
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

// Inputs: upperExtent = upper extent for animation frame control; baseIdx = graphics-table-offset
// base index (forwarded to GetCurrentAnimOffset/GetOffsetFromAnimCtrl)
// Outputs: offset to graphics table (from GetCurrentAnimOffset)
uint8_t SMBEngine::AnimationControl(uint8_t upperExtent, uint8_t baseIdx)
{
    // get proper offset to graphics table
    const uint8_t gfxOffset = GetCurrentAnimOffset(baseIdx);
    // load animation frame timer
    if (M(PlayerAnimTimer) == 0)
    { // branch if not expired
        // get animation frame timer amount
        ram[PlayerAnimTimer] = M(PlayerAnimTimerSet); // and set timer accordingly
        uint8_t frameCtrl = M(PlayerAnimCtrl) + 1;
        if (frameCtrl >= upperExtent)
        {                  // if frame control + 1 < upper extent, use as next
            frameCtrl = 0; // otherwise initialize frame control
        } // SetAnimC: store as new animation frame control
        ram[PlayerAnimCtrl] = frameCtrl;
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
    RelWOfs(GetProperObjOffset(slot, 1), 3);
}

//------------------------------------------------------------------------

// Inputs: slot = fireball object buffer offset
// Outputs: none
void SMBEngine::RelativeFireballPosition(uint8_t slot)
{
    // modify slot to get proper fireball offset, then get the coordinates
    RelWOfs(GetProperObjOffset(slot, 0), 2);
}

//------------------------------------------------------------------------

// Inputs: slot = misc object buffer offset
// Outputs: none
void SMBEngine::RelativeMiscPosition(uint8_t slot)
{
    // modify slot to get proper misc object offset, then get the coordinates
    RelWOfs(GetProperObjOffset(slot, 2), 6);
}

//------------------------------------------------------------------------

// Inputs: slot = block object buffer offset (adjusted internally for the second block object)
// Outputs: none
void SMBEngine::RelativeBlockPosition(uint8_t slot)
{
    // get coordinates of one block object relative to the screen
    VariableObjOfsRelPos(9, slot, 4);
    // adjust offset for other block object if any, adjust other and get coordinates
    // for other one
    VariableObjOfsRelPos(9, slot + 2, 5);
}

//------------------------------------------------------------------------

// Inputs: slot = fireball object buffer offset
// Outputs: none
void SMBEngine::GetFireballOffscreenBits(uint8_t slot)
{
    // modify slot to get proper fireball offset, then get offscreen information
    // about fireball
    GetOffScreenBitsSet(GetProperObjOffset(slot, 0), 2);
}

//------------------------------------------------------------------------

// Inputs: slot = air bubble object buffer offset
// Outputs: none
void SMBEngine::GetBubbleOffscreenBits(uint8_t slot)
{
    // modify slot to get proper air bubble offset, then get offscreen information
    // about air bubble
    GetOffScreenBitsSet(GetProperObjOffset(slot, 1), 3);
}

//------------------------------------------------------------------------

// Inputs: slot = misc object buffer offset
// Outputs: none
void SMBEngine::GetMiscOffscreenBits(uint8_t slot)
{
    // modify slot to get proper misc object offset, then get offscreen information
    // about misc object
    GetOffScreenBitsSet(GetProperObjOffset(slot, 2), 6);
}

//------------------------------------------------------------------------

// Inputs: slot = block object buffer offset
// Outputs: none
void SMBEngine::GetBlockOffscreenBits(uint8_t slot)
{
    // add 9 bytes in order to get block obj offset, and put offscreen bits in
    // Block_OffscreenBits
    SetOffscrBitsOffset(9, slot, 4);
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
        if (frameCtrl >= 10)
        {                                  // if not there yet, skip ahead to use
            frameCtrl = 0;                 // otherwise initialize both grow/shrink flag
            ram[PlayerChangeSizeFlag] = 0; // and animation frame control
        } // CSzNext: store proper frame control
        ram[PlayerAnimCtrl] = frameCtrl;
    } // GorSLog: get player's size
    if (M(PlayerSize) == 0)
    { // if player big, get offset adder based on frame control as offset,
        // and use offset for player growing
        return GetOffsetFromAnimCtrl(ChangeSizeOffsetAdder_data[frameCtrl], 15);

        //------------------------------------------------------------------------
    } // ShrinkPlayer
    // add ten bytes to frame control as offset; this thing apparently uses two of the
    // swimming frames to draw the player shrinking
    const uint8_t shrinkIdx = frameCtrl + 10;
    // load offset for small player swimming, or if what would normally be the offset
    // adder is zero, load offset for big player swimming instead
    const uint8_t baseIdx = ChangeSizeOffsetAdder_data[shrinkIdx] != 0 ? 9 : 1;
    // ShrPlF: get offset to graphics table based on offset loaded and leave
    return M(PlayerGfxTblOffsets + baseIdx);
}

//------------------------------------------------------------------------

// Inputs: slot = fireball object buffer offset
// Outputs: none
void SMBEngine::FireballBGCollision(uint8_t slot)
{
    // check fireball's vertical coordinate
    if (M(Fireball_Y_Position + slot) >= 0x18)
    { // if not within the status bar area of the screen,
        // do fireball to background collision detection on bottom of it
        const uint8_t metatile = BlockBufferChk_FBall(slot);
        // if there is something underneath fireball, and no non-solid metatiles found,
        if (metatile != 0 && !ChkForNonSolids(metatile))
        {
            // if fireball's vertical speed not set to move upwards, and bouncing flag
            // not already set,
            if ((M(Fireball_Y_Speed + slot) & 0x80) == 0 && M(FireballBouncingFlag + slot) == 0)
            {
                ram[Fireball_Y_Speed + slot] = 0xfd;  // otherwise set vertical speed to move upwards (give it bounce)
                ram[FireballBouncingFlag + slot] = 1; // set bouncing flag
                // modify vertical coordinate to land it properly
                ram[Fireball_Y_Position + slot] = M(Fireball_Y_Position + slot) & 0xf8; // store as new vertical coordinate
                return;                                                                 // leave

                //------------------------------------------------------------------------
            }
            // InitFireballExplode
            ram[Fireball_State + slot] = 0x80; // set exploding flag in fireball's state
            ram[Square1SoundQueue] = Sfx_Bump; // load bump sound
            return;                            // leave

            //------------------------------------------------------------------------
        }
    }
    // ClearBounceFlag
    ram[FireballBouncingFlag + slot] = 0; // clear bouncing flag by default
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
    auto nonAnimatedActs = [&](uint8_t baseIdx) -> uint8_t
    {
        const uint8_t idx = GetGfxOffsetAdder(baseIdx);
        ram[PlayerAnimCtrl] = 0;
        return M(PlayerGfxTblOffsets + idx);
    };

    // FourFrameExtent: load upper extent for frame control, get offset and animate
    // player object
    auto fourFrameExtent = [&](uint8_t idx) -> uint8_t { return AnimationControl(3, idx); };

    const uint8_t state = M(Player_State); // get player's state
    if (state == 3)
    { // ActionClimbing: load offset for climbing
        // check player's vertical speed
        if (M(Player_Y_Speed) == 0)
        {
            return nonAnimatedActs(5); // if no speed, use offset as-is
        }
        // otherwise get offset for graphics table, then skip ahead to more code
        // ThreeFrameExtent: 2 is the upper extent for frame control for climbing
        return AnimationControl(2, GetGfxOffsetAdder(5));
    }
    if (state == 2)
    { // ActionFalling: load offset for walking/running, get offset to graphics table,
        // and execute instructions for falling state
        return GetCurrentAnimOffset(GetGfxOffsetAdder(4));
    }
    if (state == 1)
    { // jumping
        if (M(SwimmingFlag) != 0)
        { // ActionSwimming: load offset for swimming
            const uint8_t idx = GetGfxOffsetAdder(1);
            // check jump/swim timer and animation frame control
            if ((M(JumpSwimTimer) | M(PlayerAnimCtrl)) != 0)
            {
                return fourFrameExtent(idx); // if any one of these set, branch ahead
            }
            if ((M(A_B_Buttons) & 0x80) != 0)
            {                                // check for A button pressed
                return fourFrameExtent(idx); // branch to same place if A button pressed
            }
            return GetCurrentAnimOffset(idx);
        }
        // get crouching flag
        if (M(CrouchingFlag) != 0)
        {
            return nonAnimatedActs(6); // if set, load offset for crouching
        }
        return nonAnimatedActs(0); // otherwise load offset for jumping
    }
    // ProcOnGroundActs: get crouching flag
    if (M(CrouchingFlag) != 0)
    {
        return nonAnimatedActs(6); // if set, load offset for crouching
    }
    // check player's horizontal speed and left/right controller bits
    if ((M(Player_X_Speed) | M(Left_Right_Buttons)) == 0)
    {
        return nonAnimatedActs(2); // if no speed or buttons pressed, use standing offset
    }
    // load walking/running speed
    if (M(Player_XSpeedAbsolute) >= 9)
    { // if fast enough to skid, check to see if moving direction
        // and facing direction are the same
        if ((M(Player_MovingDir) & M(PlayerFacingDir)) == 0)
        {                              // if moving direction <> facing direction, skid
            return nonAnimatedActs(3); // load skid offset ($03)
        }
    }
    // ActionWalkRun: load offset for walking/running, get offset to graphics table,
    // and execute instructions for normal state
    return fourFrameExtent(GetGfxOffsetAdder(4));
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::DonePlayerTask()
{
    ram[TimerControl] = 0;                            // initialize master timer control to continue timers
    ram[GameEngineSubroutine] = Gs_PlayerCtrlRoutine; // set player control routine to run next frame
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
        // get frame counter, divide by four to change every four frames
        CyclePlayerPalette(M(FrameCounter) >> 2);
        return;

        //------------------------------------------------------------------------
    } // ResetPalFireFlower
    DonePlayerTask(); // do sub to init timer control and run player control routine

    ResetPalStar();
}

//------------------------------------------------------------------------

// Inputs: slot = enemy object buffer offset
// Outputs: none
void SMBEngine::FloateyNumbersRoutine(uint8_t slot)
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

    uint8_t control = M(FloateyNum_Control + slot); // load control for floatey number
    if (control == 0)
    {
        return; // if zero, branch to leave
    }
    if (control >= 11)
    {
        control = 11;                        // otherwise set to $0b, thus keeping
        ram[FloateyNum_Control + slot] = 11; // it in range
    } // ChkNumTimer
    const uint8_t timer = M(FloateyNum_Timer + slot); // check value here
    if (timer == 0)
    {                                           // if zero,
        ram[FloateyNum_Control + slot] = timer; // initialize floatey number control and leave
        return;

        //------------------------------------------------------------------------
    } // DecNumTimer: decrement value here
    --M(FloateyNum_Timer + slot);
    if (timer == 0x2b)
    { // if timer just started counting down,
        if (control == 11)
        {                                           // branch ahead if this is not a 1-up
            ++M(NumberofLives);                     // give player one extra life (1-up)
            ram[Square2SoundQueue] = Sfx_ExtraLife; // and play the 1-up sound
        } // LoadNumTiles: load point value here
        // move high nybble to low, essentially the digit
        const uint8_t digit = ScoreUpdateData_data[control] >> 4;
        // load again and this time mask out the high nybble, and store as amount to
        // add to the digit
        ram[DigitModifier + digit] = ScoreUpdateData_data[control] & 0b00001111;
        AddToScore(); // update the score accordingly
    } // ChkTallEnemy: get OAM data offset for enemy object
    uint8_t oamSlot = M(Enemy_SprDataOffset + slot);
    const uint8_t enemyId = M(Enemy_ID + slot); // get enemy object identifier

    // GetAltOffset is used for hammer bros, tall enemies ($09 or greater), and any
    // other enemy not in a defeated state ($02 or greater); spinies, piranha plants and
    // cheep-cheeps of either color always use the enemy's own OAM data offset
    bool useAltOffset;
    if (enemyId == Spiny || enemyId == PiranhaPlant)
    {
        useAltOffset = false;
    }
    else if (enemyId == HammerBro)
    {
        useAltOffset = true;
    }
    else if (enemyId == GreyCheepCheep || enemyId == RedCheepCheep)
    {
        useAltOffset = false;
    }
    else if (enemyId >= TallEnemy)
    {
        useAltOffset = true;
    }
    else
    {
        useAltOffset = M(Enemy_State + slot) < 2;
    }
    if (useAltOffset)
    { // GetAltOffset: load some kind of control bit and get alternate OAM data offset
        oamSlot = M(Alt_SprDataOffset + M(SprDataOffset_Ctrl));
    }

    // FloateyPart: get vertical coordinate for floatey number
    const uint8_t yPos = M(FloateyNum_Y_Pos + slot);
    const bool borrow = yPos < 0x18; // the compare's borrow is still live at the subtract below
    if (yPos >= 0x18)
    {                                            // if not into status bar,
        ram[FloateyNum_Y_Pos + slot] = yPos - 1; // subtract one and store as new
    } // SetupNumSpr: get vertical coordinate, subtract eight (and the borrow) and dump into
    // the left and right sprite's Y coordinates
    DumpTwoSpr((uint8_t)(M(FloateyNum_Y_Pos + slot) - 8 - (borrow ? 1 : 0)), oamSlot);
    const uint8_t xPos = M(FloateyNum_X_Pos + slot); // get horizontal coordinate
    ram[Sprite_X_Position + oamSlot] = xPos;         // store into X coordinate of left sprite
    // add eight pixels and store into X coordinate of right sprite
    ram[Sprite_X_Position + 4 + oamSlot] = xPos + 8;
    ram[Sprite_Attributes + oamSlot] = 0x02;     // set palette control in attribute bytes
    ram[Sprite_Attributes + 4 + oamSlot] = 0x02; // of left and right sprites
    // multiply our floatey number control by 2 and use as offset for look-up table
    const uint8_t tileOfs = M(FloateyNum_Control + slot) << 1;
    ram[Sprite_Tilenumber + oamSlot] = FloateyNumTileData_data[tileOfs];         // display first half of number of points
    ram[Sprite_Tilenumber + 4 + oamSlot] = FloateyNumTileData_data[1 + tileOfs]; // display the second half
    // and leave
}

//------------------------------------------------------------------------

// Inputs: slot = misc object buffer offset
// Outputs: none
void SMBEngine::DrawHammer(uint8_t slot)
{
    const uint8_t HammerSprAttrib_data[] = {0x03, 0x03, 0xc3, 0xc3};

    const uint8_t SecondSprTilenum_data[] = {0x81, 0x83, 0x80, 0x82};

    const uint8_t FirstSprTilenum_data[] = {0x80, 0x82, 0x81, 0x83};

    const uint8_t SecondSprYPos_data[] = {0x08, 0x00, 0x08, 0x00};

    const uint8_t SecondSprXPos_data[] = {0x00, 0x08, 0x00, 0x08};

    const uint8_t FirstSprYPos_data[] = {0x00, 0x04, 0x00, 0x04};

    const uint8_t FirstSprXPos_data[] = {0x04, 0x00, 0x04, 0x00};

    const uint8_t oamSlot = M(Misc_SprDataOffset + slot); // get misc object OAM data offset

    uint8_t pose = 0; // ForceHPose: reset offset here (do unconditional branch to rendering part)
    // unless master timer control set, get hammer's state with d7 masked out
    if (M(TimerControl) == 0 && (M(Misc_State + slot) & 0b01111111) == 1)
    { // GetHPose: get frame counter, move d3-d2 to d1-d0 and mask out all but d1-d0
        // (changes every four frames), use as timing offset
        pose = (M(FrameCounter) >> 2) & 0b00000011;
    }
    // RenderH: get relative vertical coordinate
    // add first sprite vertical adder based on offset
    const uint8_t sprY = M(Misc_Rel_YPos) + FirstSprYPos_data[pose];
    ram[Sprite_Y_Position + oamSlot] = sprY; // store as sprite Y coordinate for first sprite
    // add second sprite vertical adder based on offset
    ram[Sprite_Y_Position + 4 + oamSlot] = sprY + SecondSprYPos_data[pose]; // store as sprite Y coordinate for second sprite
    // get relative horizontal coordinate and add first sprite horizontal adder based on offset
    const uint8_t sprX = M(Misc_Rel_XPos) + FirstSprXPos_data[pose];
    ram[Sprite_X_Position + oamSlot] = sprX; // store as sprite X coordinate for first sprite
    // add second sprite horizontal adder based on offset
    ram[Sprite_X_Position + 4 + oamSlot] = sprX + SecondSprXPos_data[pose]; // store as sprite X coordinate for second sprite
    ram[Sprite_Tilenumber + oamSlot] = FirstSprTilenum_data[pose];          // get and store tile number of first sprite
    ram[Sprite_Tilenumber + 4 + oamSlot] = SecondSprTilenum_data[pose];     // get and store tile number of second sprite
    ram[Sprite_Attributes + oamSlot] = HammerSprAttrib_data[pose];          // get and store attribute bytes for both
    ram[Sprite_Attributes + 4 + oamSlot] = HammerSprAttrib_data[pose];      // note in this case they use the same data
    // check offscreen bits
    if ((M(Misc_OffscreenBits) & 0b11111100) != 0)
    {                               // if any bits set, otherwise leave object alone
        ram[Misc_State + slot] = 0; // nullify misc object state
        DumpTwoSpr(0xf8, oamSlot);  // do sub to move hammer sprites offscreen
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
    ram[PlayerGfxOffset] = gfxOffset; // store offset to graphics table here
    RenderPlayerSub(4);               // draw player based on offset loaded
    ChkForPlayerAttrib();             // set horizontal flip bits as necessary
    if (M(FireballThrowingTimer) == 0)
    {
        PlayerOffscreenChk(); // if fireball throw timer not set, skip to the end
        return;
    }
    const uint8_t animTimer = M(PlayerAnimTimer); // get animation frame timer
    if (animTimer >= M(FireballThrowingTimer))
    {
        ram[FireballThrowingTimer] = 0; // initialize fireball throw timer
        PlayerOffscreenChk();           // if animation frame timer => fireball throw timer skip to end
        return;
    }
    ram[FireballThrowingTimer] = animTimer; // otherwise store animation timer into fireball throw timer
    // load offset for throwing
    // get offset to graphics table
    ram[PlayerGfxOffset] = M(PlayerGfxTblOffsets + 7); // store it for use later
    uint8_t rows = 4;                                  // set to update four sprite rows by default
    if ((M(Player_X_Speed) | M(Left_Right_Buttons)) != 0)
    {             // check for horizontal speed or left/right button press
        rows = 3; // otherwise set to update only three sprite rows
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
    uint8_t vertBits = M(Player_OffscreenBits) >> 4; // move vertical bits to low nybble
    // get player's sprite data offset and add 24 bytes to start at bottom row
    uint8_t oamSlot = M(Player_SprDataOffset) + 24;
    uint8_t row = 3; // check all four rows of player sprites

    do // PROfsLoop
    {
        const bool offscreen = (vertBits & 0x01) != 0; // take the bit
        vertBits >>= 1;
        if (offscreen)
        { // if bit set, dump offscreen Y coordinate into sprite data
            DumpTwoSpr(0xf8, oamSlot);
        } // NPROffscr
        oamSlot -= 8; // next row up
        --row;        // decrement row counter
    } while ((row & 0x80) == 0); // do this until all sprite rows are checked
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
    if (M(GameEngineSubroutine) == Gs_PlayerDeath)
    { // PlayerKilled: use offset for player killed
        PlayerGfxProcessing(M(PlayerGfxTblOffsets + 14));
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
    if (M(SwimmingFlag) == 0 || M(Player_State) == 0)
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
    uint8_t tileIdx = 0;                       // initialize tile selector to zero
    uint8_t oamSlot = M(Player_SprDataOffset); // get player sprite data offset
    if ((M(PlayerFacingDir) & 0x01) == 0)      // get player's facing direction
    {                                          // if player facing to the right, use current offset
        oamSlot += 4;                          // otherwise move to next OAM data
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
    ram[Sprite_Tilenumber + 24 + oamSlot] = SwimKickTileNum_data[tileIdx];

    // ExPGH: then leave
}

//------------------------------------------------------------------------

// Inputs: slot = block object buffer offset
// Outputs: none
void SMBEngine::BlockObjectsCore(uint8_t slot)
{
    const uint8_t state = M(Block_State + slot); // get state of block object
    if (state == 0)
    { // if not set, branch to leave (UpdSte rewrites the zero state)
        ram[Block_State + slot] = 0;
        return;
    }
    const uint8_t savedState = state & 0x0f; // mask out high nybble and save (was pushed to stack)
    // add 9 bytes to offset (note two block objects are created when using brick
    // chunks, but only one offset for both)
    const uint8_t chunkOfs = slot + 9;
    if (savedState != 1)
    {                                         // solid block state not found, so this is brick chunks
        ImposeGravityBlock(chunkOfs);         // do sub to impose gravity on one block object object
        MoveObjectHorizontally(chunkOfs);     // do another sub to move horizontally
        ImposeGravityBlock(chunkOfs + 2);     // do sub to impose gravity on other block object
        MoveObjectHorizontally(chunkOfs + 2); // do another sub to move horizontally
        const uint8_t self = M(ObjectOffset); // get block object offset used for both
        RelativeBlockPosition(self);          // get relative coordinates
        GetBlockOffscreenBits(self);          // get offscreen information
        DrawBrickChunks(self);                // draw the brick chunks
        // check vertical high byte of block object
        if (M(Block_Y_HighPos + self) == 0)
        { // if above the screen, branch to kill it (UpdSte with the saved state)
            ram[Block_State + self] = savedState;
            return;
        }
        if (0xf0 < M(Block_Y_Position + 2 + self))
        {                                            // to the bottom of the screen, and branch if not
            ram[Block_Y_Position + 2 + self] = 0xf0; // otherwise set offscreen coordinate
        } // ChkTop: get top block object's vertical coordinate
        if (M(Block_Y_Position + self) < 0xf0)
        {                                         // if not at the bottom of the screen, branch to save state
            ram[Block_State + self] = savedState; // UpdSte
            return;
        }
        // otherwise KillBlock: nullify object state
        ram[Block_State + self] = 0; // UpdSte
        return;
    }
    // BouncingBlockHandler
    ImposeGravityBlock(chunkOfs);         // do sub to impose gravity on block object
    const uint8_t self = M(ObjectOffset); // get block object offset
    RelativeBlockPosition(self);          // get relative coordinates
    GetBlockOffscreenBits(self);          // get offscreen information
    DrawBlock(self);                      // draw the block
    // get vertical coordinate and mask out high nybble
    if ((M(Block_Y_Position + self) & 0x0f) >= 0x05)
    { // if still above amount, not time to kill block yet, thus save state (UpdSte)
        ram[Block_State + self] = savedState;
        return;
    }
    ram[Block_RepFlag + self] = 1; // otherwise set flag to replace metatile

    // KillBlock: nullify object state (UpdSte)
    ram[Block_State + self] = 0;
}

//------------------------------------------------------------------------

// Inputs: slot = block object buffer offset
// Outputs: none
void SMBEngine::DrawBlock(uint8_t slot)
{
    const uint8_t DefaultBlockObjTiles_data[] = {
        0x85, 0x85, 0x86, 0x86 // brick w/ line (these are sprite tiles, not BG!)
    };

    // get relative vertical coordinate of block object
    uint8_t yPos = M(Block_Rel_YPos); // store here
    // get relative horizontal coordinate of block object
    const uint8_t relXPos = M(Block_Rel_XPos);
    const uint8_t attributes = 3;                    // set attribute byte here
    const uint8_t flipBits = 1;                      // set horizontal flip bit here (will not be used)
    uint8_t oamSlot = M(Block_SprDataOffset + slot); // get sprite data offset
    uint8_t tileIdx = 0;                             // reset offset to tile data

    do // DBlkLoop: get left tile number
    {
        // get left and right tile numbers and do sub to write them to first row of sprites
        std::tie(tileIdx, oamSlot) = DrawOneSpriteRow(DefaultBlockObjTiles_data[tileIdx], DefaultBlockObjTiles_data[1 + tileIdx], tileIdx,
                                                      oamSlot, flipBits, attributes, relXPos, yPos);
    } while (tileIdx != 4); // and loop back until all four sprites are done
    oamSlot = M(Block_SprDataOffset + slot); // get sprite data offset back
    if (M(AreaType) != 1)
    {                                                // if not ground level type area,
        ram[Sprite_Tilenumber + oamSlot] = 0x86;     // remove brick tiles with lines
        ram[Sprite_Tilenumber + 4 + oamSlot] = 0x86; // and replace then with lineless brick tiles
    } // ChkRep: check replacement metatile
    if (M(Block_Metatile + slot) == 0xc4)
    { // if used block tile, dump it into all four sprites (offset incremented to
        // write to tile bytes)
        DumpFourSpr(0x87, oamSlot + 0x01);
        // set palette bits; check for ground level type area again, otherwise set to $01
        const uint8_t attributes = M(AreaType) == 1 ? 3 : 1;
        ram[Sprite_Attributes + oamSlot] = attributes; // store attribute byte as-is in first sprite
        // set horizontal flip bit for second sprite
        ram[Sprite_Attributes + 4 + oamSlot] = attributes | 0b01000000;
        // set both flip bits for fourth sprite
        ram[Sprite_Attributes + 12 + oamSlot] = attributes | 0b11000000;
        // set vertical flip bit for third sprite
        ram[Sprite_Attributes + 8 + oamSlot] = (attributes | 0b11000000) & 0b10000011;
    } // BlkOffscr: get offscreen bits for block object
    const uint8_t offscreenBits = M(Block_OffscreenBits);
    // check to see if d2 in offscreen bits are set
    if ((offscreenBits & 0b00000100) != 0)
    {                                                 // if set, move sprites offscreen:
        ram[Sprite_Y_Position + 4 + oamSlot] = 0xf8;  // move offscreen two OAMs
        ram[Sprite_Y_Position + 12 + oamSlot] = 0xf8; // on the right side
    } // PullOfsB
    ChkLeftCo(offscreenBits, oamSlot);
}

//------------------------------------------------------------------------

// check to see if d3 in offscreen bits are set
// Inputs: offscreenBits = block offscreen bits; oamSlot = OAM data offset
// Outputs: none
void SMBEngine::ChkLeftCo(uint8_t offscreenBits, uint8_t oamSlot)
{
    if ((offscreenBits & 0b00001000) == 0)
    { // if not set, branch, otherwise move sprites offscreen
        return;
    }
    MoveColOffscreen(oamSlot);
}

//------------------------------------------------------------------------

// Inputs: slot = block object buffer offset
// Outputs: none
void SMBEngine::DrawBrickChunks(uint8_t slot)
{
    // set palette bits here
    uint8_t paletteBits = 2;
    uint8_t tile = 0x75; // set tile number for ball (something residual, likely)
    if (M(GameEngineSubroutine) != Gs_PlayerEndLevel)
    {                    // use palette and tile number assigned unless end-of-level routine running
        paletteBits = 3; // otherwise set different palette bits
        tile = 0x84;     // and set tile number for brick chunks
    } // DChunks: get OAM data offset
    const uint8_t oamSlot = M(Block_SprDataOffset + slot);
    // increment to start with tile bytes in OAM, and dump tile number into all
    // four sprites
    DumpFourSpr(tile, oamSlot + 1);
    // get frame counter, move low nybble to high, and get what was originally d3-d2 of
    // the low nybble; add palette bits and increment offset for attribute bytes
    DumpFourSpr((uint8_t)((uint8_t)(M(FrameCounter) << 4) & 0xc0) | paletteBits, oamSlot + 2);
    // decrement offset to Y coordinate, and get first block object's relative vertical
    // coordinate; dump current Y coordinate into two sprites
    DumpTwoSpr(M(Block_Rel_YPos), oamSlot);
    // get first block object's relative horizontal coordinate
    ram[Sprite_X_Position + oamSlot] = M(Block_Rel_XPos);                    // save into X coordinate of first sprite
    const uint8_t origRel = M(Block_Orig_XPos + slot) - M(ScreenLeft_X_Pos); // subtract coordinate of left side from original coordinate
    bool carry = origRel >= M(Block_Rel_XPos);                               // the borrow this subtract leaves is read by the add below
    // get difference of relative positions of original - current, and add original
    // relative position to result
    uint32_t wide = (uint8_t)(origRel - M(Block_Rel_XPos)) + origRel + (carry ? 1 : 0);
    // plus 6 pixels, and this add's own carry, to position second brick chunk correctly
    ram[Sprite_X_Position + 4 + oamSlot] = (uint8_t)(LOBYTE(wide) + 6 + HIBYTE(wide));
    const uint8_t relYPos2 = M(Block_Rel_YPos + 1); // get second block object's relative vertical coordinate
    ram[Sprite_Y_Position + 8 + oamSlot] = relYPos2;
    ram[Sprite_Y_Position + 12 + oamSlot] = relYPos2; // dump into Y coordinates of third and fourth sprites
    // get second block object's relative horizontal coordinate
    ram[Sprite_X_Position + 8 + oamSlot] = M(Block_Rel_XPos + 1); // save into X coordinate of third sprite
    carry = origRel >= M(Block_Rel_XPos + 1);                     // the borrow this subtract leaves is read by the add below
    // use original relative horizontal position, get difference of relative positions of
    // original - current, and add original relative position to result
    wide = (uint8_t)(origRel - M(Block_Rel_XPos + 1)) + origRel + (carry ? 1 : 0);
    // plus 6 pixels, and this add's own carry, to position fourth brick chunk correctly
    ram[Sprite_X_Position + 12 + oamSlot] = (uint8_t)(LOBYTE(wide) + 6 + HIBYTE(wide));
    // get offscreen bits for block object, and do sub to move left half of sprites
    // offscreen if necessary
    ChkLeftCo(M(Block_OffscreenBits), oamSlot);
    if ((M(Block_OffscreenBits) & 0x80) != 0) // check d7 of the offscreen bits
    {                                         // if d7 set, move top sprites offscreen
        DumpTwoSpr(0xf8, oamSlot);
    } // ChnkOfs: if relative position on left side of screen,
    if ((origRel & 0x80) == 0)
    {
        return; // go ahead and leave
    }
    // otherwise compare left-side X coordinate
    if (M(Sprite_X_Position + oamSlot) < M(Sprite_X_Position + 4 + oamSlot))
    {
        return; // branch to leave if less
    }
    // otherwise move right half of sprites offscreen
    ram[Sprite_Y_Position + 4 + oamSlot] = 0xf8;
    ram[Sprite_Y_Position + 12 + oamSlot] = 0xf8;

    // ExBCDr: leave
}

//------------------------------------------------------------------------

// Inputs: slot = misc object buffer offset
// Outputs: none
void SMBEngine::JCoinGfxHandler(uint8_t slot)
{
    const uint8_t JumpingCoinTiles_data[] = {0x60, 0x61, 0x62, 0x63};

    const uint8_t oamSlot = M(Misc_SprDataOffset + slot); // get coin/floatey number's OAM data offset
    // get state of misc object
    if (M(Misc_State + slot) >= 2)
    { // DrawFloateyNumber_Coin
        if ((M(FrameCounter) & 0x01) == 0)
        {                                // get frame counter divide by 2
            --M(Misc_Y_Position + slot); // if d0 not set, decrement vertical coordinate
                                         // to raise number every other frame
        } // NotRsNum: get vertical coordinate
        DumpTwoSpr(M(Misc_Y_Position + slot), oamSlot); // dump into both sprites
        const uint8_t xPos = M(Misc_Rel_XPos);          // get relative horizontal coordinate
        ram[Sprite_X_Position + oamSlot] = xPos;        // store as X coordinate for first sprite
        // add eight pixels and store as X coordinate for second sprite
        ram[Sprite_X_Position + 4 + oamSlot] = xPos + 8;
        ram[Sprite_Attributes + oamSlot] = 0x02; // store attribute byte in both sprites
        ram[Sprite_Attributes + 4 + oamSlot] = 0x02;
        ram[Sprite_Tilenumber + oamSlot] = 0xf7;     // put tile numbers into both sprites
        ram[Sprite_Tilenumber + 4 + oamSlot] = 0xfb; // that resemble "200"
        return;                                      // then leave
    }
    const uint8_t yPos = M(Misc_Y_Position + slot); // store vertical coordinate as
    ram[Sprite_Y_Position + oamSlot] = yPos;        // Y coordinate for first sprite
    // add eight pixels and store as Y coordinate for second sprite
    ram[Sprite_Y_Position + 4 + oamSlot] = yPos + 8;
    const uint8_t xPos = M(Misc_Rel_XPos); // get relative horizontal coordinate
    ram[Sprite_X_Position + oamSlot] = xPos;
    ram[Sprite_X_Position + 4 + oamSlot] = xPos; // store as X coordinate for first and second sprites
    // get frame counter, divide by 2 to alter every other frame, and mask out d2-d1
    // to use as graphical offset
    const uint8_t tileIdx = (M(FrameCounter) >> 1) & 0b00000011;
    // load tile number, and increment OAM data offset to write tile numbers;
    // dump tile number into both sprites
    DumpTwoSpr(JumpingCoinTiles_data[tileIdx], oamSlot + 1);
    ram[Sprite_Attributes + oamSlot] = 0x02;     // set attribute byte in first sprite
    ram[Sprite_Attributes + 4 + oamSlot] = 0x82; // set attribute byte with vertical flip in second sprite

    // ExJCGfx: leave
}

//------------------------------------------------------------------------

// Inputs: slot = fireball object buffer offset
// Outputs: none
void SMBEngine::DrawExplosion_Fireball(uint8_t slot)
{
    // get OAM data offset of alternate sort for fireball's explosion
    const uint8_t oamSlot = M(Alt_SprDataOffset + slot);
    const uint8_t state = M(Fireball_State + slot);  // load fireball state
    ++M(Fireball_State + slot);                      // increment state for next frame
    const uint8_t frame = (state >> 1) & 0b00000111; // divide by 2, mask out all but d3-d1
    if (frame >= 3)
    {                                   // if past the third explosion frame,
        ram[Fireball_State + slot] = 0; // clear fireball state to kill it
        return;
    }
    DrawExplosion_Fireworks(frame, oamSlot); // otherwise continue to draw explosion
}

//------------------------------------------------------------------------

// Inputs: none (uses enemy object buffer slot 5, the flagpole flag's special-use slot)
// Outputs: none
void SMBEngine::FlagpoleRoutine()
{
    const uint8_t FlagpoleScoreDigits_data[] = {0x03, 0x03, 0x04, 0x04, 0x04};

    const uint8_t FlagpoleScoreMods_data[] = {0x05, 0x02, 0x08, 0x04, 0x01};

    // set enemy object offset to special use slot
    ram[ObjectOffset] = 5;
    if (M(Enemy_ID + 5) != FlagpoleFlagObject)
    {
        return; // ExitFlagP: branch to leave
    }
    if (M(GameEngineSubroutine) == Gs_FlagpoleSlide && M(Player_State) == 3)
    {
        // check flagpole flag's vertical coordinate,
        // and player's vertical coordinate
        if (M(Enemy_Y_Position + 0x05) >= 0xaa || M(Player_Y_Position) >= 0xa2)
        {
            // GiveFPScr: end the level; get score offset from earlier (when player touched flagpole)
            const uint8_t scoreOfs = M(FlagpoleScore);
            // get amount to award player points
            const uint8_t digit = FlagpoleScoreDigits_data[scoreOfs];      // get digit with which to award points
            ram[DigitModifier + digit] = FlagpoleScoreMods_data[scoreOfs]; // store in digit modifier
            AddToScore();                                                  // do sub to award player points depending on height of collision
            ram[GameEngineSubroutine] = Gs_PlayerEndLevel;                 // set to run end-of-level subroutine on next frame
        }
        else
        {
            // position:dummy is one 16-bit quantity; the compare above left the carry clear
            uint32_t wide = ((M(Enemy_Y_Position + 5) << 8) | M(Enemy_YMF_Dummy + 5)) + 0x01ff; // add movement amount to move flag down
            ram[Enemy_YMF_Dummy + 5] = LOBYTE(wide);                                            // save dummy variable
            ram[Enemy_Y_Position + 5] = HIBYTE(wide);                                           // store vertical coordinate
            wide = ((M(FlagpoleFNum_Y_Pos) << 8) | M(FlagpoleFNum_YMFDummy)) - 0x01ff; // subtract the same to move the floatey number up
            ram[FlagpoleFNum_YMFDummy] = LOBYTE(wide);                                 // save dummy variable
            ram[FlagpoleFNum_Y_Pos] = HIBYTE(wide);                                    // and store vertical coordinate here
        }
    } // SkipScore
    // FPGfx: get offscreen information
    GetEnemyOffscreenBits(5);
    RelativeEnemyPosition(5); // get relative coordinates
    FlagpoleGfxHandler(5);    // draw flagpole flag and floatey number
}

//------------------------------------------------------------------------

// Inputs: slot = enemy object buffer offset (expected to be 5, the flagpole flag's special-use slot)
// Outputs: none
void SMBEngine::FlagpoleGfxHandler(uint8_t slot)
{
    const uint8_t FlagpoleScoreNumTiles_data[] = {0xf9, 0x50, 0xf7, 0x50, 0xfa, 0xfb, 0xf8, 0xfb, 0xf6, 0xfb};

    uint8_t oamOfs = M(Enemy_SprDataOffset + slot); // get sprite data offset for flagpole flag
    uint8_t xPos = M(Enemy_Rel_XPos);               // get relative horizontal coordinate
    ram[Sprite_X_Position + oamOfs] = xPos;         // store as X coordinate for first sprite
    xPos += 8;                                      // add eight pixels and store
    ram[Sprite_X_Position + 4 + oamOfs] = xPos;     // as X coordinate for second and third sprites
    ram[Sprite_X_Position + 8 + oamOfs] = xPos;
    uint32_t wide = xPos + 12;                 // add twelve more pixels and
    const uint8_t numXPos = LOBYTE(wide);      // keep here to be used later by floatey number
    uint8_t yPos = M(Enemy_Y_Position + slot); // get vertical coordinate
    DumpTwoSpr(yPos, oamOfs);                  // and do sub to dump into first and second sprites
    // add eight pixels, plus the carry out of the horizontal add above
    yPos = (uint8_t)(yPos + 8 + HIBYTE(wide));
    ram[Sprite_Y_Position + 8 + oamOfs] = yPos; // and store into third sprite
    // get vertical coordinate for floatey number
    uint8_t fNumYPos = M(FlagpoleFNum_Y_Pos); // store it here
    // flip value of 1 (will not be used) and attribute byte of 1 for the floatey number
    // are passed to DrawOneSpriteRow below
    ram[Sprite_Attributes + oamOfs] = 0x01; // set attribute bytes for all three sprites
    ram[Sprite_Attributes + 4 + oamOfs] = 0x01;
    ram[Sprite_Attributes + 8 + oamOfs] = 0x01;
    ram[Sprite_Tilenumber + oamOfs] = 0x7e;     // put triangle shaped tile
    ram[Sprite_Tilenumber + 8 + oamOfs] = 0x7e; // into first and third sprites
    ram[Sprite_Tilenumber + 4 + oamOfs] = 0x7f; // put skull tile into second sprite
    // get vertical coordinate at time of collision
    if (M(FlagpoleCollisionYPos) != 0)
    { // if zero, branch ahead
        // get offset used to award points for touching flagpole,
        // multiplied by 2 to get proper offset here
        const uint8_t tileIdx = M(FlagpoleScore) << 1;
        // get appropriate tile data and use it to render floatey number
        DrawOneSpriteRow(FlagpoleScoreNumTiles_data[tileIdx], FlagpoleScoreNumTiles_data[1 + tileIdx], tileIdx, oamOfs + 12, 1, 1, numXPos,
                         fNumYPos);
    } // ChkFlagOffscreen
    const uint8_t flagSlot = M(ObjectOffset); // get object offset for flag
    // get offscreen bits, mask out all but d3-d1
    if ((M(Enemy_OffscreenBits) & 0b00001110) == 0)
    { // if none of these bits set, branch to leave
        return;
    }
    MoveSixSpritesOffscreen(M(Enemy_SprDataOffset + flagSlot)); // get OAM data offset
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerLoseLife()
{
    const uint8_t HalfwayPageNybbles_data[] = {0x56, 0x40, 0x65, 0x70, 0x66, 0x40, 0x66, 0x40,
                                               0x66, 0x40, 0x66, 0x60, 0x65, 0x70, 0x00, 0x00};

    ++M(DisableScreenFlag); // disable screen and sprite 0 check
    ram[Sprite0HitDetectFlag] = 0;
    ram[EventMusicQueue] = Silence; // silence music
    --M(NumberofLives);             // take one life from player
    if ((M(NumberofLives) & 0x80) != 0)
    {                                      // if player still has lives, branch
        ram[OperMode_Task] = 0;            // initialize mode task,
        ram[OperMode] = GameOverModeValue; // switch to game over mode and leave
        return;
    }
    // StillInGame: multiply world number by 2 and use as offset
    uint8_t nybbleOfs = M(WorldNumber) << 1;
    // if in area -3 or -4, increment offset by one byte,
    // otherwise leave offset alone
    if ((M(LevelNumber) & 0x02) != 0)
    {
        ++nybbleOfs;
    }
    // GetHalfway: get halfway page number with offset
    uint8_t halfwayPage = HalfwayPageNybbles_data[nybbleOfs];
    // if in area -2 or -4, use lower nybble
    if ((M(LevelNumber) & 0x01) == 0)
    {
        halfwayPage >>= 4; // move higher nybble to lower if area number is -1 or -3
    } // MaskHPNyb: mask out all but lower nybble
    halfwayPage &= 0b00001111;
    // left side of screen must be at the halfway page,
    // otherwise player must start at the beginning of the level
    if (halfwayPage > M(ScreenLeft_PageLoc))
    {
        halfwayPage = 0;
    }
    // SetHalfway: store as halfway page for player
    ram[HalfwayPage] = halfwayPage;
    TransposePlayers(); // switch players around if 2-player game
    ContinueGame();     // continue the game
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ContinueGame()
{
    LoadAreaPointer(); // update level pointer with
    // actual world and area numbers, then
    ram[PlayerSize] = 1;        // reset player's size, status, and
    ++M(FetchNewGameTimerFlag); // set game timer flag to reload
    // game timer from header
    ram[TimerControl] = 0; // also set flag for timers to count again
    ram[PlayerStatus] = 0;
    ram[GameEngineSubroutine] = Gs_Entrance_GameTimerSetup; // reset task for game core
    ram[OperMode_Task] = 0;                                 // set modes and leave
    ram[OperMode] = GameModeValue;                          // if in game over mode, switch back to
                                                            // game mode, because game is still on
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
    ram[Player_PageLoc] = M(ScreenLeft_PageLoc); // as page location for player
    // store value here
    ram[VerticalForceDown] = 0x28; // for fractional movement downwards if necessary
    // set high byte of player position and
    ram[PlayerFacingDir] = 1; // set facing direction so that player faces right
    ram[Player_Y_HighPos] = 1;
    // set player state to on the ground by default
    ram[Player_State] = 0;
    --M(Player_CollisionBits); // initialize player's collision bits
    ram[HalfwayPage] = 0;      // initialize halfway page
    // check area type; if water type, set swimming flag, otherwise do not set
    ram[SwimmingFlag] = M(AreaType) == 0 ? 1 : 0;
    uint8_t yPosOfs = M(PlayerEntranceCtrl);           // get starting position loaded from header
    const uint8_t altEntrance = M(AltEntranceControl); // check alternate mode of entry flag for 0 or 1
    if (altEntrance >= 2)
    {
        yPosOfs = AltYPosOffset_data[altEntrance - 2]; // if not 0 or 1, override $0710 with new offset
    }
    // SetStPos: load appropriate horizontal and vertical positions for the player, using
    // AltEntranceControl as offset for horizontal and either $0710
    // or value that overwrote $0710 as offset for vertical
    ram[Player_X_Position] = PlayerStarting_X_Pos_data[altEntrance];
    ram[Player_Y_Position] = PlayerStarting_Y_Pos_data[yPosOfs];
    ram[Player_SprAttrib] = M(PlayerBGPriorityData + yPosOfs); // set player sprite attributes
    // get appropriate player palette; its leftover buffer offset is the air
    // bubble slot on the non-vine path below
    uint8_t bubbleSlot = GetPlayerColors();
    const uint8_t timerSetting = M(GameTimerSetting); // get timer control value from header
    // if set to zero, branch (do not use dummy byte for this); do we need to
    // set the game timer? if not, use old game timer setting
    if (timerSetting != 0 && M(FetchNewGameTimerFlag) != 0)
    {
        // if game timer is set and game timer flag is also set,
        ram[GameTimerDisplay] = M(GameTimerData + timerSetting); // use value of game timer control for first digit of game timer
        ram[GameTimerDisplay + 2] = 1;                           // set last digit of game timer to 1
        ram[GameTimerDisplay + 1] = 0;                           // set second digit of game timer
        ram[FetchNewGameTimerFlag] = 0;                          // clear flag for game timer reset
        ram[StarInvincibleTimer] = 0;                            // clear star mario timer
    }
    // ChkOverR: if controller bits not set, branch to skip this part
    if (M(JoypadOverride) != 0)
    {
        ram[Player_State] = 3;        // set player state to climbing
        InitBlock_XY_Pos(0);          // set offset for first slot, for block object
        ram[Block_Y_Position] = 0xf0; // set vertical coordinate for block object
        bubbleSlot = 5;               // set offset for last enemy object buffer slot
        Setup_Vine(5, 0);             // do a sub to grow vine
    } // ChkSwimE: if level not water-type,
    if (M(AreaType) == 0)
    { // skip this subroutine
        // otherwise, execute sub to set up air bubbles. 145 is the stray value the pseudorandom
        // bit parameter had on this path on the console -- see the note above SetupBubble.
        SetupBubble(bubbleSlot, 145);
    } // SetPESub: set to run player entrance subroutine
    ram[GameEngineSubroutine] = Gs_PlayerEntrance; // on the next frame of game engine
}

//------------------------------------------------------------------------

// Inputs: slot = air bubble object buffer offset
// Outputs: none
void SMBEngine::BubbleCheck(uint8_t slot)
{
    // get part of LSFR and use as the pseudorandom bit
    const uint8_t randomBit = M(PseudoRandomBitReg + 1 + slot) & 0x01;
    // get vertical coordinate for air bubble
    if (M(Bubble_Y_Position + slot) != 0xf8)
    { // branch to move air bubble
        MoveBubl(slot, randomBit);
        return;
    }
    if (M(AirBubbleTimer) != 0)
    {
        return; // if air bubble timer not expired, branch to leave
    }
    SetupBubble(slot, randomBit); // otherwise create new air bubble
}

//------------------------------------------------------------------------

// Bubble_MForceData and BubbleTimerData
//
// Both of these two-entry tables are meant to be indexed with a pseudorandom
// bit, which BubbleCheck passes to SetupBubble as it falls through into it.
// But the player entrance code calls SetupBubble directly, and on that path
// the 6502 read $07, which still held whatever the last routine to use it
// left there: the high byte of the address the game's JumpEngine routine had
// last dispatched to, which is 145. (JumpEngine assembled a computed jump,
// which has no C++ equivalent, so this port never had a reachable copy of it;
// the entrance code passes the 145 explicitly instead.) So on the first frame
// of every water level these tables are indexed with an arbitrary byte, and
// the air bubbles start out with a timer and a movement force read from
// whatever the ROM happens to store after them.
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
void SMBEngine::SetupBubble(uint8_t slot, uint8_t randomBit)
{
    uint8_t adder = 0; // load default value here
    if ((M(PlayerFacingDir) & 0x01) != 0)
    {              // use the default value if facing left
        adder = 9; // otherwise eight pixels over, plus the one d0 of the facing direction carries in
    } // PosBubl: use value loaded as adder
    // add to player's horizontal position
    const uint32_t wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + adder;
    ram[Bubble_X_Position + slot] = LOBYTE(wide); // save as horizontal position for airbubble
    ram[Bubble_PageLoc + slot] = HIBYTE(wide);    // save as page location for airbubble
    // save player's vertical position plus eight pixels as vertical position for air bubble
    ram[Bubble_Y_Position + slot] = M(Player_Y_Position) + 8;
    ram[Bubble_Y_HighPos + slot] = 1; // set vertical high byte for air bubble
    // get pseudorandom bit, use as offset to get data for air bubble timer
    ram[AirBubbleTimer] = BubbleTimerData[randomBit]; // set air bubble timer
    MoveBubl(slot, randomBit);
}

//------------------------------------------------------------------------

// Inputs: slot = air bubble object buffer offset
// Outputs: none
void SMBEngine::MoveBubl(uint8_t slot, uint8_t randomBit)
{
    // get pseudorandom bit again, use as offset
    // position:dummy is one 16-bit quantity
    const uint32_t wide = ((M(Bubble_Y_Position + slot) << 8) | M(Bubble_YMF_Dummy + slot)) -
                          Bubble_MForceData[randomBit]; // subtract pseudorandom amount from dummy variable
    ram[Bubble_YMF_Dummy + slot] = LOBYTE(wide);        // save dummy variable
    uint8_t yPos = HIBYTE(wide);                        // the airbubble's vertical coordinate, less the borrow
    if (yPos < 0x20)
    {                // branch to go ahead and use to move air bubble upwards
        yPos = 0xf8; // otherwise set offscreen coordinate
    } // Y_Bubl: store as new vertical coordinate for air bubble
    ram[Bubble_Y_Position + slot] = yPos;

    // ExitBubl: leave
}

//------------------------------------------------------------------------

// Inputs: slot = fireball object buffer offset
// Outputs: none (forwards slot + 7 and relative-coordinates offset 2 into FBallB/BoundingBoxCore)
void SMBEngine::GetFireballBoundBox(uint8_t slot)
{
    // add seven bytes to offset, and set offset for relative coordinates
    FBallB(slot + 7, 2);
}

//------------------------------------------------------------------------

// Inputs: slot = misc object buffer offset
// Outputs: none (forwards slot + 9 and relative-coordinates offset 6 into FBallB/BoundingBoxCore)
void SMBEngine::GetMiscBoundBox(uint8_t slot)
{
    // add nine bytes to offset, and set offset for relative coordinates
    FBallB(slot + 9, 6);
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
    ram[SavedJoypadBits] = ctrlBits; // override controller bits with contents of A if executing here
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
    ram[JoypadOverride] = 0b00001000;
    ram[Player_State] = 3; // set player state to climbing
    AutoControlPlayer(0b00001000);
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::VerticalPipeEntry()
{
    MovePlayerYAxis(1); // set 1 as movement amount, do sub to move player downwards
    ScrollHandler();    // do sub to scroll screen with saved force if necessary
    if (M(WarpZoneControl) != 0)
    {                   // check warp zone control variable/flag
        ChgAreaPipe(0); // if set, branch to use mode 0
        return;
    }
    if (M(AreaType) != 3)
    {                   // check for castle level type
        ChgAreaPipe(1); // if not castle type level, use mode 1
        return;
    }
    ChgAreaPipe(2); // otherwise use mode 2
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::SideExitPipeEntry()
{
    EnterSidePipe(); // execute sub to move player to the right
    ChgAreaPipe(2);
}

//------------------------------------------------------------------------

// decrement timer for change of area
// Inputs: mode = mode of alternate entry to set once the change-area timer expires
// Outputs: none
void SMBEngine::ChgAreaPipe(uint8_t mode)
{
    --M(ChangeAreaTimer);
    if (M(ChangeAreaTimer) != 0)
    {
        return;
    }
    ram[AltEntranceControl] = mode; // when timer expires set mode of alternate entry
    ChgAreaMode();
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::EnterSidePipe()
{
    // set player's horizontal speed
    ram[Player_X_Speed] = 8;
    uint8_t ctrlBits = 1; // set controller right button by default
    // mask out higher nybble of player's horizontal position
    if ((M(Player_X_Position) & 0b00001111) == 0)
    {
        ram[Player_X_Speed] = 0; // if lower nybble = 0, set as horizontal speed
        ctrlBits = 0;            // and nullify controller bit override here
    } // RightPipe
    AutoControlPlayer(ctrlBits); // execute player control routine with ctrl bits nulled
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerDeath()
{
    if (M(TimerControl) < 0xf0)
    {                        // check master timer control; leave if past that point
        PlayerCtrlRoutine(); // otherwise run player control routine
    } // ExitDeath: leave from death routine
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::FlagpoleSlide()
{
    // check special use enemy slot
    if (M(Enemy_ID + 5) == FlagpoleFlagObject)
    { // if not found, branch to something residual
        // load flagpole sound
        ram[Square1SoundQueue] = M(FlagpoleSoundQueue); // into square 1's sfx queue
        ram[FlagpoleSoundQueue] = 0;                    // init flagpole sound queue
        uint8_t ctrlBits = 0;
        if (M(Player_Y_Position) < 0x9e)
        {                 // far enough, and if so, branch with no controller bits set
            ctrlBits = 4; // otherwise force player to climb down (to slide)
        } // SlidePlayer: jump to player control routine
        AutoControlPlayer(ctrlBits);
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
    if (M(AltEntranceControl) != 2)
    { // if found, branch to enter from pipe or with vine
        // if vertical position above a certain point, execute
        if (M(Player_Y_Position) < 0x30)
        {
            AutoControlPlayer(0); // with player movement code, do not return
            return;
        }
        const uint8_t entranceCtrl = M(PlayerEntranceCtrl); // check player entry bits from header
        if (entranceCtrl == 6 || entranceCtrl == 7)
        { // if set to 6 or 7, execute pipe intro code
            // ChkBehPipe: check for sprite attributes
            if (M(Player_SprAttrib) == 0)
            {                         // branch if found
                AutoControlPlayer(1); // force player to walk to the right
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
        }
        // otherwise fall through to PlayerRdy below
    }
    else if (M(JoypadOverride) == 0)
    {                          // EntrMode2: if controller override bits set here, branch to enter with vine
        MovePlayerYAxis(0xff); // otherwise, execute sub to move player upwards (note $ff = -1)
        // check to see if player is at a specific coordinate
        // (to be at specific height to look/function right)
        if (M(Player_Y_Position) >= 0x91)
        {
            return; // otherwise leave
        }
        // fall through to PlayerRdy below
    }
    else
    { // VineEntr
        if (M(VineHeight) != 0x60)
        {
            return; // if vine not yet reached maximum height, branch to leave
        }
        uint8_t disableFlag = 0; // load default values to be written to
        uint8_t ctrlBits = 1;    // this value moves player to the right off the vine
        // get player's vertical coordinate
        if (M(Player_Y_Position) >= 0x99)
        {                                      // if vertical coordinate < preset value, use defaults
            ram[Player_State] = 3;             // otherwise set player state to climbing
            disableFlag = 1;                   // increment value
            ram[Block_Buffer_1 + 0xb4] = 0x08; // set block in block buffer to cover hole, then
            ctrlBits = 8;                      // use same value to force player to climb
        } // OffVine: set collision detection disable flag
        ram[DisableCollisionDet] = disableFlag;
        AutoControlPlayer(ctrlBits); // move player up or right, execute sub
        if (M(Player_X_Position) < 0x48)
        {
            return; // if not far enough to the right, branch to leave
        }
    }
    // PlayerRdy: set routine to be executed by game engine next frame
    ram[GameEngineSubroutine] = Gs_PlayerCtrlRoutine;
    // set to face player to the right
    ram[PlayerFacingDir] = 1;
    ram[AltEntranceControl] = 0;  // init mode of entry
    ram[DisableCollisionDet] = 0; // init collision detection disable flag
    ram[JoypadOverride] = 0;      // nullify controller override bits

    // ExitEntr: leave!
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerEndLevel()
{
    const uint8_t Hidden1UpCoinAmts_data[] = {0x15, 0x23, 0x16, 0x1b, 0x17, 0x18, 0x23, 0x63};

    AutoControlPlayer(1); // force player to walk to the right
    // check player's vertical position, and whether scroll lock is set,
    // because we only need to do this part once
    if (M(Player_Y_Position) >= 0xae && M(ScrollLock) != 0)
    {                                           // if player is not yet off the flagpole, skip this part
        ram[EventMusicQueue] = EndOfLevelMusic; // load win level music in event music queue
        ram[ScrollLock] = 0;                    // turn off scroll lock to skip this part later
    }
    // ChkStop: get player collision bits
    if ((M(Player_CollisionBits) & 0x01) == 0) // check for d0 set
    {                                          // if d0 set, skip to next part
        // if star flag task control already set,
        if (M(StarFlagTaskControl) == 0)
        {                             // go ahead with the rest of the code
            ++M(StarFlagTaskControl); // otherwise set task control now (this gets ball rolling!)
        } // InCastle: set player's background priority bit to
        ram[Player_SprAttrib] = 0b00100000; // give illusion of being inside the castle
    } // RdyNextA
    if (M(StarFlagTaskControl) != 5)
    { // beyond last valid task number, branch to leave
        return;
    }
    ++M(LevelNumber); // increment level number used for game logic
    if (M(LevelNumber) != 3)
    {
        NextArea(); // and skip this last part here if not
        return;
    }
    // get world number as offset, check third area coin tally for bonus 1-ups
    if (M(CoinTallyFor1Ups) < Hidden1UpCoinAmts_data[M(WorldNumber)])
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
    LoadAreaPointer();              // get new level pointer
    ++M(FetchNewGameTimerFlag);     // set flag to load new game timer
    ChgAreaMode();                  // do sub to set secondary mode, disable screen and sprite 0
    ram[HalfwayPage] = 0;           // reset halfway page to 0 (beginning)
    ram[EventMusicQueue] = Silence; // silence music and leave
}

//------------------------------------------------------------------------

// Inputs: none (dispatches on M(GameEngineSubroutine))
// Outputs: none
void SMBEngine::GameRoutines()
{
    // run routine based on number (a few of these routines are
    switch (M(GameEngineSubroutine))
    {
    case Gs_Entrance_GameTimerSetup:
        Entrance_GameTimerSetup();
        return;
    case Gs_Vine_AutoClimb:
        Vine_AutoClimb();
        return;
    case Gs_SideExitPipeEntry:
        SideExitPipeEntry();
        return;
    case Gs_VerticalPipeEntry:
        VerticalPipeEntry();
        return;
    case Gs_FlagpoleSlide:
        FlagpoleSlide();
        return;
    case Gs_PlayerEndLevel:
        PlayerEndLevel();
        return;
    case Gs_PlayerLoseLife:
        PlayerLoseLife();
        return;
    case Gs_PlayerEntrance:
        PlayerEntrance();
        return;
    case Gs_PlayerCtrlRoutine:
        PlayerCtrlRoutine();
        return;
    case Gs_PlayerChangeSize:
        PlayerChangeSize();
        return;
    case Gs_PlayerInjuryBlink:
        PlayerInjuryBlink();
        return;
    case Gs_PlayerDeath:
        PlayerDeath();
        return;
    case Gs_PlayerFireFlower:
        PlayerFireFlower();
        return;
    default:
        bad_jump();
        return;
    } // merely placeholders as conditions for other routines)
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerChangeSize()
{
    uint8_t timer = M(TimerControl); // check master timer control
    if (timer == 0xf8)
    { // branch if before or after that point
        InitChangeSize();
        return;
    } // EndChgSize: check again for another specific moment
    // otherwise run code to get growing/shrinking going
    if (timer == 0xc4)
    {                     // and branch to leave if before or after that point
        DonePlayerTask(); // otherwise do sub to init timer control and set routine
    } // ExitChgSize: and then leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::PlayerInjuryBlink()
{
    uint8_t timer = M(TimerControl); // check master timer control
    if (timer < 0xf0)
    { // branch if before that point
        if (timer == 0xc8)
        {
            DonePlayerTask(); // branch if at that point, and not before or after
            return;
        }
        PlayerCtrlRoutine(); // otherwise run player control routine
        return;
    } // ExitBlink: do unconditional branch to leave
    if (timer == 0xf0)
    {
        InitChangeSize();
    }
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::InitChangeSize()
{
    // if growing/shrinking flag already set
    if (M(PlayerChangeSizeFlag) != 0)
    {
        return; // then branch to leave
    }
    ram[PlayerAnimCtrl] = 0;                // otherwise initialize player's animation frame control
    ++M(PlayerChangeSizeFlag);              // set growing/shrinking flag
    ram[PlayerSize] = M(PlayerSize) ^ 0x01; // invert player's size

    // ExitBoth: leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::UpdScrollVar()
{
    if (M(VRAM_Buffer_AddrCtrl) == 6)
    {
        return; // then branch to leave
    }
    // otherwise check number of tasks
    if (M(AreaParserTaskNum) == 0)
    {
        // get horizontal scroll in 0-31 or $00-$20 range
        if (((M(ScrollThirtyTwo) - 0x20) & 0x80) != 0)
        {
            return; // branch to leave if not
        }
        // otherwise subtract $20 to set appropriately and store
        ram[ScrollThirtyTwo] = M(ScrollThirtyTwo) - 0x20;
        ram[VRAM_Buffer2_Offset] = 0; // reset vram buffer offset used in conjunction with
                                      // level graphics buffer at $0341-$035f
    } // RunParser: update the name table with more level graphics
    AreaParserTaskHandler();

    // ExitEng: and after all that, we're finally done!
}

//------------------------------------------------------------------------

// Inputs: slot = enemy object buffer offset (Bowser's slot)
// Outputs: none
void SMBEngine::HurtBowser(uint8_t slot, uint8_t scoreSlot)
{
    const uint8_t BowserIdentities_data[] = {Goomba, GreenKoopa, BuzzyBeetle, Spiny, Lakitu, Bloober, HammerBro, Bowser};

    --M(BowserHitPoints); // decrement bowser's hit points
    if (M(BowserHitPoints) != 0)
    {
        return; // if bowser still has hit points, branch to leave
    }
    InitVStf(slot);                       // otherwise do sub to init vertical speed and movement force
    ram[Enemy_X_Speed + slot] = 0;        // initialize horizontal speed (InitVStf left 0 in A)
    ram[EnemyFrenzyBuffer] = 0;           // init enemy frenzy buffer
    ram[Enemy_Y_Speed + slot] = 0xfe;     // set vertical speed to make defeated bowser jump a little
    const uint8_t world = M(WorldNumber); // use world number as offset
    // get enemy identifier to replace bowser with
    ram[Enemy_ID + slot] = BowserIdentities_data[world]; // set as new enemy identifier
    // use starting value for state, or state + 3 in the earlier worlds
    // SetDBSte: set defeated enemy state
    ram[Enemy_State + slot] = world < 0x03 ? 0x23 : 0x20;
    ram[Square2SoundQueue] = Sfx_BowserFall; // load bowser defeat sound
    // get enemy offset, and award 5000 points to player for defeating bowser
    EnemySmackScore(9, scoreSlot); // unconditional branch to award points
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ProcFireball_Bubble()
{
    // check player's status
    if (M(PlayerStatus) >= 2)
    { // if fiery
        // check for b button pressed and not pressed in previous frame
        if ((M(A_B_Buttons) & B_Button) != 0 && (M(A_B_Buttons) & B_Button & M(PreviousA_B_Buttons)) == 0)
        {
            // load fireball counter, get LSB and use as offset for buffer
            const uint8_t slot = M(FireballCounter) & 0b00000001;
            if (M(Fireball_State + slot) == 0 // if fireball inactive,
                && M(Player_Y_HighPos) == 1   // player not too high or too low,
                && M(CrouchingFlag) == 0      // player not crouching,
                && M(Player_State) != 3)      // and player's state not climbing,
            {
                // play fireball sound effect
                ram[Square1SoundQueue] = Sfx_Fireball;
                ram[Fireball_State + slot] = 2;                     // load state
                const uint8_t animTimerSet = M(PlayerAnimTimerSet); // copy animation frame timer setting
                ram[FireballThrowingTimer] = animTimerSet;          // into fireball throwing timer
                // decrement and store in player's animation timer
                ram[PlayerAnimTimer] = animTimerSet - 1;
                ++M(FireballCounter); // increment fireball counter
            }
        }
        // ProcFireballs
        FireballObjCore(0); // process first fireball object
        FireballObjCore(1); // process second fireball object, then do air bubbles
    } // ProcAirBubbles
    if (M(AreaType) == 0)
    { // if water type level, load counter and use as offset
        uint8_t slot = 2;

        do // BublLoop: store offset
        {
            ram[ObjectOffset] = slot;
            BubbleCheck(slot);            // check timers and coordinates, create air bubble
            RelativeBubblePosition(slot); // get relative coordinates
            GetBubbleOffscreenBits(slot); // get offscreen information
            DrawBubble(slot);             // draw the air bubble
            --slot;
        } while ((slot & 0x80) == 0); // do this until all three are handled
    } // BublExit: then leave
}

//------------------------------------------------------------------------

// Inputs: slot = fireball object buffer offset (0 or 1)
// Outputs: none
void SMBEngine::FireballObjCore(uint8_t slot)
{
    ram[ObjectOffset] = slot; // store offset as current object
    if ((M(Fireball_State + slot) & 0x80) != 0)
    { // FireballExplosion: check for d7 = 1;
        // if so, get relative coordinates and draw explosion
        RelativeFireballPosition(slot);
        DrawExplosion_Fireball(slot);
        return;
    }
    const uint8_t state = M(Fireball_State + slot);
    if (state == 0)
    {
        return; // if fireball inactive, branch to leave
    }
    if (state != 1)
    { // if fireball state not yet set to 1, initialize the fireball
        // get player's horizontal position, add four pixels and store as fireball's
        // horizontal position
        const uint32_t wide = ((M(Player_PageLoc) << 8) | M(Player_X_Position)) + 4;
        ram[Fireball_X_Position + slot] = LOBYTE(wide);
        ram[Fireball_PageLoc + slot] = HIBYTE(wide);
        // get player's vertical position and store
        ram[Fireball_Y_Position + slot] = M(Player_Y_Position);
        // set high byte of vertical position
        ram[Fireball_Y_HighPos + slot] = 1;
        // get player's facing direction, decrement to use as offset here, and set
        // horizontal speed of fireball accordingly
        ram[Fireball_X_Speed + slot] = M(FireballXSpdData + (uint8_t)(M(PlayerFacingDir) - 1));
        // set vertical speed of fireball
        ram[Fireball_Y_Speed + slot] = 4;
        ram[Fireball_BoundBoxCtrl + slot] = 7; // set bounding box size control for fireball
        --M(Fireball_State + slot);            // decrement state to 1 to skip this part from now on
    }
    // RunFB: add 7 to offset to use
    const uint8_t fireballOfs = slot + 7;
    // do sub here to impose gravity on fireball and move vertically, with a downward movement
    // force of 0x50 and a maximum speed of 3
    ImposeGravity(0x00, fireballOfs, 0x50, 0x00, 0x03);
    MoveObjectHorizontally(fireballOfs);  // do another sub to move it horizontally
    const uint8_t self = M(ObjectOffset); // return fireball offset
    RelativeFireballPosition(self);       // get relative coordinates
    GetFireballOffscreenBits(self);       // get offscreen information
    GetFireballBoundBox(self);            // get bounding box coordinates
    FireballBGCollision(self);            // do fireball to background collision detection
    // get fireball offscreen bits and mask out certain bits
    if ((M(FBall_OffscreenBits) & 0b11001100) != 0)
    { // EraseFB: if any bits set, erase fireball state
        ram[Fireball_State + self] = 0;
        return; // NoFBall: leave
    }
    FireballEnemyCollision(self); // do fireball to enemy collision detection and deal with collisions
    DrawFireball(self);           // draw fireball appropriately and leave
}

//------------------------------------------------------------------------

// Inputs: slot = fireball object buffer offset
// Outputs: none
void SMBEngine::FireballEnemyCollision(uint8_t slot)
{
    // check to see if fireball state is set at all
    const uint8_t state = M(Fireball_State + slot);
    if (state == 0)
    {
        return; // branch to leave if not
    }
    if ((state & 0x80) != 0)
    {
        return; // branch to leave also if d7 in state is set
    }
    if ((M(FrameCounter) & 0x01) != 0)
    {
        return; // branch to leave if frame counter LSB set (do routine every other frame)
    }
    // multiply fireball offset by four, then add $1c or 28 bytes to it,
    // to use fireball's bounding box coordinates
    const uint8_t fireballBoundBoxIdx = (uint8_t)(slot << 2) + 0x1c;
    uint8_t enemySlot = 4;

    do // FireballEnemyCDLoop
    {
        // (the fireball bounding box offset was pushed to the stack here; it is a
        // local now)
        bool skipSlot = false;
        if ((M(Enemy_State + enemySlot) & 0b00100000) != 0)
        {
            skipSlot = true; // if d5 set in enemy state, skip to next enemy slot
        }
        else if (M(Enemy_Flag + enemySlot) == 0)
        {
            skipSlot = true; // if buffer flag not set, skip to next enemy slot
        }
        else
        {
            const uint8_t enemyId = M(Enemy_ID + enemySlot); // check enemy identifier
            if (enemyId >= 0x24 && enemyId < 0x2b)
            {
                skipSlot = true; // if in range $24-$2a, skip to next enemy slot
            }
            else if (enemyId == Goomba && M(Enemy_State + enemySlot) >= 2)
            {                    // GoombaDie: goomba in defeated state,
                skipSlot = true; // skip to next enemy slot
            }
            // NotGoomba: if any masked offscreen bits set,
            else if (M(EnemyOffscrBitsMasked + enemySlot) != 0)
            {
                skipSlot = true; // skip to next enemy slot
            }
        }
        if (!skipSlot)
        {
            // multiply enemy offset by four and add 4 bytes to it,
            // to use enemy's bounding box coordinates
            const uint8_t enemyBoundBoxIdx = (uint8_t)(enemySlot << 2) + 4;
            // do fireball-to-enemy collision detection
            const bool collisionFound = SprObjectCollisionCore(enemyBoundBoxIdx, fireballBoundBoxIdx);
            if (collisionFound)
            { // otherwise do next enemy slot
                // set d7 in fireball state (using the fireball's original offset)
                ram[Fireball_State + M(ObjectOffset)] = 0b10000000;
                HandleEnemyFBallCol(enemySlot); // jump to handle fireball to enemy collision
            }
        }
        // NoFToECol: get enemy object offset and decrement it
        --enemySlot;
    } while ((enemySlot & 0x80) == 0); // loop back until collision detection done on all enemies

    // ExitFBallEnemy: leave
}

//------------------------------------------------------------------------

// Inputs: enemySlot = enemy object buffer offset
// Outputs: none
void SMBEngine::HandleEnemyFBallCol(uint8_t enemySlot)
{
    RelativeEnemyPosition(enemySlot);            // get relative coordinate of enemy
    uint8_t target = enemySlot;                  // get current enemy object offset
    const uint8_t flag = M(Enemy_Flag + target); // check buffer flag for d7 set
    if ((flag & 0x80) != 0)
    { // branch if not set to continue
        // otherwise mask out high nybble and use low nybble as enemy offset
        target = flag & 0b00001111;
        if (M(Enemy_ID + target) == Bowser)
        {
            HurtBowser(target, enemySlot); // branch if found
            return;
        }
        target = enemySlot; // otherwise retrieve current enemy offset
    } // ChkBuzzyBeetle
    const uint8_t enemyId = M(Enemy_ID + target);
    if (enemyId == BuzzyBeetle)
    {
        return; // branch if found to leave (buzzy beetles fireproof)
    }
    if (enemyId == Bowser)
    {
        HurtBowser(target, enemySlot);
        return;
    }
    // ChkOtherEnemies
    if (enemyId == BulletBill_FrenzyVar)
    {
        return; // branch to leave if bullet bill (frenzy variant)
    }
    if (enemyId == Podoboo)
    {
        return; // branch to leave if podoboo
    }
    if (enemyId >= 0x15)
    {
        return; // branch to leave if identifier => $15
    }
    ShellOrBlockDefeat(target);
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::RunGameTimer()
{
    // get primary mode of operation
    if (M(OperMode) == TitleScreenModeValue)
    {
        return; // branch to leave if in title screen mode
    }
    const uint8_t engineSub = M(GameEngineSubroutine);
    if (engineSub < Gs_PlayerCtrlRoutine || engineSub == Gs_PlayerDeath)
    {
        return; // branch to leave
    }
    if (M(Player_Y_HighPos) >= 2)
    {
        return; // branch to leave regardless of level type
    }
    if (M(GameTimerCtrlTimer) != 0)
    {
        return; // branch to leave if game timer control not yet expired
    }
    // otherwise check game timer digits
    if ((M(GameTimerDisplay) | M(GameTimerDisplay + 1) | M(GameTimerDisplay + 2)) == 0)
    {                              // TimeUpOn: if game timer digits at 000, run time-up code
        ram[PlayerStatus] = 0;     // init player status
        ForceInjury(0);            // do sub to kill the player (note player is small here)
        ++M(GameTimerExpiredFlag); // set game timer expiration flag
        return;                    // ExGTimer: leave
    }
    // otherwise check first digit for 1, then second and third digits for 0
    if (M(GameTimerDisplay) == 1 && (M(GameTimerDisplay + 1) | M(GameTimerDisplay + 2)) == 0)
    {
        ram[EventMusicQueue] = TimeRunningOutMusic; // if timer at 100, load time running out music
    }
    // ResGTCtrl: reset game timer control
    ram[GameTimerCtrlTimer] = 24;
    ram[DigitModifier + 5] = 0xff; // set value to decrement game timer digit
    DigitsMathRoutine(0x23);       // do sub to decrement game timer slowly (offset for last digit)
    PrintStatusBarNumbers(0xa4);   // set status nybbles to update game timer display
}

//------------------------------------------------------------------------

// Inputs: slot = misc object buffer offset (hammer slot)
// Outputs: none
void SMBEngine::ProcHammerObj(uint8_t slot)
{
    const uint8_t HammerXSpdData_data[] = {0x10, 0xf0};

    // if master timer control set, skip all of this code and go to last subs at the end
    if (M(TimerControl) == 0)
    {
        // otherwise get hammer's state with d7 masked out
        const uint8_t state = M(Misc_State + slot) & 0b01111111;
        const uint8_t enemyOfs = M(HammerEnemyOffset + slot); // get enemy object offset that spawned this hammer
        if (state < 2)
        { // add 13 bytes to use proper misc object
            const uint8_t hammerOfs = slot + 13;
            // do sub to impose gravity on hammer and move vertically: downward movement force of
            // 16, upward movement force of 15 (not used) and maximum vertical speed of 4
            ImposeGravity(0x00, hammerOfs, 0x10, 0x0f, 0x04);
            MoveObjectHorizontally(hammerOfs); // do sub to move it horizontally
            // RunAllH: handle collisions (with the original misc object offset)
            PlayerHammerCollision(slot);
        }
        else
        {
            if (state == 2)
            {                                    // SetHSpd
                ram[Misc_Y_Speed + slot] = 0xfe; // set hammer's vertical speed
                // get enemy object state, mask out d3, and store new state
                ram[Enemy_State + enemyOfs] = M(Enemy_State + enemyOfs) & 0b11110111;
                // get enemy's moving direction, decrement to use as offset, and get
                // proper speed to use based on moving direction
                const uint8_t dirIdx = M(Enemy_MovingDir + enemyOfs) - 1;
                ram[Misc_X_Speed + slot] = HammerXSpdData_data[dirIdx]; // set hammer's horizontal speed
            }
            // SetHPos: decrement hammer's state
            --M(Misc_State + slot);
            // get enemy's horizontal position and set position 2 pixels to the right
            const uint32_t wide = ((M(Enemy_PageLoc + enemyOfs) << 8) | M(Enemy_X_Position + enemyOfs)) + 2;
            ram[Misc_X_Position + slot] = LOBYTE(wide); // store as hammer's horizontal position
            ram[Misc_PageLoc + slot] = HIBYTE(wide);    // store as hammer's page location
            // get enemy's vertical position and move position 10 pixels upward
            ram[Misc_Y_Position + slot] = M(Enemy_Y_Position + enemyOfs) - 10; // store as hammer's vertical position
            ram[Misc_Y_HighPos + slot] = 1;                                    // set hammer's vertical high byte
            // (unconditional branch to skip the collision routine)
        }
    }
    // RunHSubs: get offscreen information
    GetMiscOffscreenBits(slot);
    RelativeMiscPosition(slot); // get relative coordinates
    GetMiscBoundBox(slot);      // get bounding box coordinates
    DrawHammer(slot);           // draw the hammer
    // and we are done here
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::MiscObjectsCore()
{
    uint8_t slot = 8; // set at end of misc object buffer

    // RunJCSubs: get relative coordinates, offscreen information and bounding box
    // coordinates (why?), then draw the coin or floatey number
    auto runJCSubs = [&]()
    {
        RelativeMiscPosition(slot);
        GetMiscOffscreenBits(slot);
        GetMiscBoundBox(slot);
        JCoinGfxHandler(slot);
    };

    do // MiscLoop: store misc object offset here
    {
        ram[ObjectOffset] = slot;
        const uint8_t state = M(Misc_State + slot); // check misc object state
        if (state != 0)
        { // if not inactive, check d7
            if ((state & 0x80) != 0)
            {                        // if d7 set, this is a hammer,
                ProcHammerObj(slot); // so go process it, then check next slot
            }
            else if (state != 1)
            {                           // ProcJumpCoin: if state not set to 1,
                ++M(Misc_State + slot); // increment state to either start off or as timer
                // get horizontal coordinate for misc object and add current scroll speed
                const uint32_t wide = ((M(Misc_PageLoc + slot) << 8) | M(Misc_X_Position + slot)) + M(ScrollAmount);
                ram[Misc_X_Position + slot] = LOBYTE(wide); // store as new horizontal coordinate
                ram[Misc_PageLoc + slot] = HIBYTE(wide);    // store as new page location
                if (M(Misc_State + slot) != 0x30)
                {                // if not yet reached the end,
                    runJCSubs(); // branch to subroutines
                }
                else
                {
                    ram[Misc_State + slot] = 0; // otherwise nullify object state
                                                // and move onto next slot
                }
            }
            else
            { // JCoinRun: add 13 bytes to offset
                const uint8_t coinOfs = slot + 13;
                // do sub to move coin vertically and impose gravity on it: downward movement
                // amount of 0x50, maximum vertical speed of 6, and half of that as the upward
                // movement amount (apparently residual)
                ImposeGravity(0x00, coinOfs, 0x50, 0x03, 0x06);
                // check vertical speed
                if (M(Misc_Y_Speed + slot) == 5)
                {                           // if moving downward fast enough,
                    ++M(Misc_State + slot); // increment state to change to floatey number
                }
                runJCSubs();
            }
        }
        // MiscLoopBack: decrement misc object offset
        --slot;
    } while ((slot & 0x80) == 0); // loop back until all misc objects handled
    // then leave
}

//------------------------------------------------------------------------

// Inputs: slot = misc object buffer offset (hammer slot)
// Outputs: none
void SMBEngine::PlayerHammerCollision(uint8_t slot)
{
    // get frame counter d0; execute every other frame only
    if ((M(FrameCounter) & 0x01) == 0)
    {
        return; // branch to leave if d0 not set
    }
    // if either master timer control or any offscreen bits for hammer are set,
    if ((M(TimerControl) | M(Misc_OffscreenBits)) != 0)
    {
        return; // branch to leave
    }
    // multiply misc object offset by four and add 36 or $24 bytes to get proper offset
    // for misc object bounding box coordinates
    const uint8_t boundBoxIdx = (uint8_t)(slot << 2) + 0x24;
    const bool collisionFound = PlayerCollisionCore(boundBoxIdx); // do player-to-hammer collision detection
    if (!collisionFound)
    { // ClHCol: clear collision flag
        ram[Misc_Collision_Flag + slot] = 0;
        return;
    }
    // otherwise read collision flag
    if (M(Misc_Collision_Flag + slot) != 0)
    {
        return; // if collision flag already set, branch to leave
    }
    ram[Misc_Collision_Flag + slot] = 1; // otherwise set collision flag now
    // get two's compliment of hammer's horizontal speed
    ram[Misc_X_Speed + slot] = (uint8_t)(M(Misc_X_Speed + slot) ^ 0xff) + 0x01; // set to send hammer flying the opposite direction
    if (M(StarInvincibleTimer) != 0)
    {
        return; // if star mario invincibility timer set, branch to leave
    }
    InjurePlayer(); // otherwise jump to hurt player, do not return

    // ExPHC
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::ProcessCannons()
{
    const uint8_t CannonBitmasks_data[] = {0b00001111, 0b00000111};

    // get area type; if water type area, branch to leave
    if (M(AreaType) == 0)
    {
        return; // ExCannon
    }
    // ThreeSChk: start at third enemy slot,
    // and do this until first three slots are checked
    for (uint8_t slot = 2; (slot & 0x80) == 0; --slot)
    {
        ram[ObjectOffset] = slot;
        // check enemy buffer flag; if set, branch to check enemy
        if (M(Enemy_Flag + slot) == 0)
        {
            // otherwise get part of LSFR, masking out bits as decided by
            // the secondary hard mode flag, as pseudorandom cannon offset
            const uint8_t cannon = M(PseudoRandomBitReg + 1 + slot) & CannonBitmasks_data[M(SecondaryHardMode)];
            // if offset in range and page location set (not page 0), continue;
            // otherwise branch to check enemy
            if (cannon < 6 && M(Cannon_PageLoc + cannon) != 0)
            {
                // get cannon timer; if expired, branch to fire cannon
                if (M(Cannon_Timer + cannon) != 0)
                {
                    // otherwise subtract the borrow (note carry will always be
                    // clear here) to count timer down, then jump ahead to check enemy
                    ram[Cannon_Timer + cannon] = M(Cannon_Timer + cannon) - 1;
                }
                // FireCannon: if master timer control set, branch to check enemy
                else if (M(TimerControl) == 0)
                {
                    // otherwise we start creating one
                    ram[Cannon_Timer + cannon] = 14; // first, reset cannon timer
                    // get page location of cannon
                    ram[Enemy_PageLoc + slot] = M(Cannon_PageLoc + cannon); // save as page location of bullet bill
                    // get horizontal coordinate of cannon
                    ram[Enemy_X_Position + slot] = M(Cannon_X_Position + cannon); // save as horizontal coordinate of bullet bill
                    // get vertical coordinate of cannon, subtract eight pixels
                    // (because enemies are 24 pixels tall)
                    ram[Enemy_Y_Position + slot] = M(Cannon_Y_Position + cannon) - 8; // save as vertical coordinate of bullet bill
                    ram[Enemy_Y_HighPos + slot] = 1;                                  // set vertical high byte of bullet bill
                    ram[Enemy_Flag + slot] = 1;                                       // set buffer flag
                    ram[Enemy_State + slot] = 0;                                      // then initialize enemy's state
                    ram[Enemy_BoundBoxCtrl + slot] = 9;                               // set bounding box size control for bullet bill
                    ram[Enemy_ID + slot] = BulletBill_CannonVar;                      // load identifier for bullet bill (cannon variant)
                    continue;                                                         // Next3Slt: move onto next slot
                }
            }
        }
        // Chk_BB: check enemy identifier for bullet bill (cannon variant)
        if (M(Enemy_ID + slot) != BulletBill_CannonVar)
        {
            continue; // if not found, branch to get next slot
        }
        OffscreenBoundsCheck(slot); // otherwise, check to see if it went offscreen
        // check enemy buffer flag
        if (M(Enemy_Flag + slot) == 0)
        {
            continue; // if not set, branch to get next slot
        }
        GetEnemyOffscreenBits(slot); // otherwise, get offscreen information
        BulletBillHandler(slot);     // then do sub to handle bullet bill
    }
}

//------------------------------------------------------------------------

// Inputs: slot = enemy object buffer offset (cannon variant bullet bill slot)
// Outputs: none
void SMBEngine::BulletBillHandler(uint8_t slot)
{
    const uint8_t BulletBillXSpdData_data[] = {0x18, 0xe8};

    // if master timer control set,
    if (M(TimerControl) == 0)
    { // branch to run subroutines except movement sub
        if (M(Enemy_State + slot) == 0)
        { // if bullet bill's state set, branch to check defeated state
            // otherwise load offscreen bits, mask out bits
            if ((M(Enemy_OffscreenBits) & 0b00001100) == 0b00001100)
            {
                EraseEnemyObject(slot); // KillBB: kill bullet bill and leave
                return;
            }
            uint8_t movingDir = 1; // set to move right by default
            // get horizontal difference between player and bullet bill
            const auto [enemyRightOfPlayer, diff, diffLow] = PlayerEnemyDiff(slot);
            if ((diff & 0x80) == 0)
            {                // if enemy to the left of player, branch
                ++movingDir; // otherwise increment to move left
            } // SetupBB: set bullet bill's moving direction
            ram[Enemy_MovingDir + slot] = movingDir;
            // decrement to use as offset, get horizontal speed based on moving direction
            ram[Enemy_X_Speed + slot] = BulletBillXSpdData_data[movingDir - 1]; // and store it
            // get horizontal difference, add 40 pixels plus the no-borrow left above
            const uint8_t dist = (uint8_t)(diffLow + 0x28 + (enemyRightOfPlayer ? 1 : 0));
            if (dist < 0x50)
            {                           // if close to cannon either on left or right side, thus branch
                EraseEnemyObject(slot); // KillBB: kill bullet bill and leave
                return;
            }
            ram[Enemy_State + slot] = 1;        // otherwise set bullet bill's state
            ram[EnemyFrameTimer + slot] = 10;   // set enemy frame timer
            ram[Square2SoundQueue] = Sfx_Blast; // play fireworks/gunfire sound
        } // ChkDSte: check enemy state for d5 set
        if ((M(Enemy_State + slot) & 0b00100000) != 0)
        {                                // if not set, skip to move horizontally
            MoveD_EnemyVertically(slot); // otherwise do sub to move bullet bill vertically
        } // BBFly: do sub to move bullet bill horizontally
        MoveEnemyHorizontally(slot);
    } // RunBBSubs: get offscreen information
    GetEnemyOffscreenBits(slot);
    RelativeEnemyPosition(slot); // get relative coordinates
    GetEnemyBoundBox(slot);      // get bounding box coordinates
    PlayerEnemyCollision(slot);  // handle player to enemy collisions
    EnemyGfxHandler(slot);       // draw the bullet bill and leave
}

//------------------------------------------------------------------------

// Inputs: none
// Outputs: none
void SMBEngine::GameCoreRoutine()
{
    const uint8_t player = M(CurrentPlayer); // get which player is on the screen
    // use appropriate player's controller bits
    ram[SavedJoypadBits] = M(SavedJoypadBits + player); // as the master controller bits
    GameRoutines();                                     // execute one of many possible subs
    // check major task of operating mode
    if (M(OperMode_Task) < 3)
    { // branch to the game engine itself
        return;
    } // GameEngine
    ProcFireball_Bubble(); // process fireballs and air bubbles
    uint8_t i = 0;

    do // ProcELoop: put incremented offset in X as enemy object offset
    {
        ram[ObjectOffset] = i;
        EnemiesAndLoopsCore(i);   // process enemy objects
        FloateyNumbersRoutine(i); // process floatey numbers
        ++i;
    } while (i != 6);
    GetPlayerOffscreenBits(); // get offscreen bits for player object
    RelativePlayerPosition(); // get relative coordinates for player object
    PlayerGfxHandler();       // draw the player
    BlockObjMT_Updater();     // replace block objects with metatiles if necessary
    ram[ObjectOffset] = 1;    // set offset for second
    BlockObjectsCore(1);      // process second block object
    ram[ObjectOffset] = 0;    // set offset for first
    BlockObjectsCore(0);      // process first block object
    MiscObjectsCore();        // process misc objects (hammer, jumping coins)
    ProcessCannons();         // process bullet bill cannons
    ProcessWhirlpools();      // process whirlpools
    FlagpoleRoutine();        // process the flagpole
    RunGameTimer();           // count down the game timer
    ColorRotation();          // cycle one of the background colors
    // when the player is below the screen, skip the timer checks and cycle regardless
    const bool playerBelow = ((M(Player_Y_HighPos) - 0x02) & 0x80) == 0;
    // if star mario invincibility timer at zero, skip this part
    if (!playerBelow && M(StarInvincibleTimer) == 0)
    { // ClrPlrPal: do sub to clear player's palette bits in attributes
        // then skip this sub to finish up the game engine
        ResetPalStar();
    }
    else
    {
        // if the timer is at a certain point with the interval timer expired,
        if (!playerBelow && M(StarInvincibleTimer) == 4 && M(IntervalTimerControl) == 0)
        {
            GetAreaMusic(); // to re-attain appropriate level music
        }
        // NoChgMus: get invincibility timer and frame counter
        uint8_t bits = M(FrameCounter);
        if (M(StarInvincibleTimer) < 8)
        {               // branch to cycle player's palette quickly
            bits >>= 2; // otherwise, divide by 8 to cycle every eighth frame
        } // CycleTwo: if branched here, divide by 2 to cycle every other frame
        bits >>= 1;
        CyclePlayerPalette(bits); // do sub to cycle the palette (note: shares fire flower code)
    } // SaveAB: save current A and B button
    ram[PreviousA_B_Buttons] = M(A_B_Buttons); // into temp variable to be used on next frame
    ram[Left_Right_Buttons] = 0;               // nullify left and right buttons temp variable
    UpdScrollVar();
}
