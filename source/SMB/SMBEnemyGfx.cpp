// The EnemyGfxHandler subsystem: everything EnemyGfxHandler() reaches that nothing
// outside it reaches, and so nothing outside it needs.
//
// Moved out of SMB.cpp by tools/split.py. These are methods of SMBEngine like every other
// routine of the game and are declared in SMBEngine.hpp; the file they are written in is the
// only thing that is different, and tools/layers.py is what keeps it meaning something.
//
#include "SMB.hpp"

//------------------------------------------------------------------------

// Inputs: gfxOffset = index into EnemyGraphicsTable_data, doubling as the sprite-pair index
// forwarded to DrawOneSpriteRow; oamSlot = OAM slot index forwarded to DrawOneSpriteRow
// Outputs: pair of {gfxOffset+2, oamSlot+8} (see DrawOneSpriteRow/DrawSpriteObject; the caller
// invokes this three times in a row to advance across one row of tiles)
std::pair<uint8_t, uint8_t> SMBEngine::DrawEnemyObjRow(uint8_t gfxOffset, uint8_t oamSlot, EnemyGfxState& gfx)
{
    const uint8_t EnemyGraphicsTable_data[] = {
        0xfc, 0xfc, 0xaa, 0xab, 0xac, 0xad, // buzzy beetle frame 1
        0xfc, 0xfc, 0xae, 0xaf, 0xb0, 0xb1, //              frame 2
        0xfc, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, // koopa troopa frame 1
        0xfc, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, //              frame 2
        0x69, 0xa5, 0x6a, 0xa7, 0xa8, 0xa9, // koopa paratroopa frame 1
        0x6b, 0xa0, 0x6c, 0xa2, 0xa3, 0xa4, //                  frame 2
        0xfc, 0xfc, 0x96, 0x97, 0x98, 0x99, // spiny frame 1
        0xfc, 0xfc, 0x9a, 0x9b, 0x9c, 0x9d, //       frame 2
        0xfc, 0xfc, 0x8f, 0x8e, 0x8e, 0x8f, // spiny's egg frame 1
        0xfc, 0xfc, 0x95, 0x94, 0x94, 0x95, //             frame 2
        0xfc, 0xfc, 0xdc, 0xdc, 0xdf, 0xdf, // bloober frame 1
        0xdc, 0xdc, 0xdd, 0xdd, 0xde, 0xde, //         frame 2
        0xfc, 0xfc, 0xb2, 0xb3, 0xb4, 0xb5, // cheep-cheep frame 1
        0xfc, 0xfc, 0xb6, 0xb3, 0xb7, 0xb5, //             frame 2
        0xfc, 0xfc, 0x70, 0x71, 0x72, 0x73, // goomba
        0xfc, 0xfc, 0x6e, 0x6e, 0x6f, 0x6f, // koopa shell frame 1 (upside-down)
        0xfc, 0xfc, 0x6d, 0x6d, 0x6f, 0x6f, //             frame 2
        0xfc, 0xfc, 0x6f, 0x6f, 0x6e, 0x6e, // koopa shell frame 1 (rightsideup)
        0xfc, 0xfc, 0x6f, 0x6f, 0x6d, 0x6d, //             frame 2
        0xfc, 0xfc, 0xf4, 0xf4, 0xf5, 0xf5, // buzzy beetle shell frame 1 (rightsideup)
        0xfc, 0xfc, 0xf4, 0xf4, 0xf5, 0xf5, //                    frame 2
        0xfc, 0xfc, 0xf5, 0xf5, 0xf4, 0xf4, // buzzy beetle shell frame 1 (upside-down)
        0xfc, 0xfc, 0xf5, 0xf5, 0xf4, 0xf4, //                    frame 2
        0xfc, 0xfc, 0xfc, 0xfc, 0xef, 0xef, // defeated goomba
        0xb9, 0xb8, 0xbb, 0xba, 0xbc, 0xbc, // lakitu frame 1
        0xfc, 0xfc, 0xbd, 0xbd, 0xbc, 0xbc, //        frame 2
        0x7a, 0x7b, 0xda, 0xdb, 0xd8, 0xd8, // princess
        0xcd, 0xcd, 0xce, 0xce, 0xcf, 0xcf, // mushroom retainer
        0x7d, 0x7c, 0xd1, 0x8c, 0xd3, 0xd2, // hammer bro frame 1
        0x7d, 0x7c, 0x89, 0x88, 0x8b, 0x8a, //            frame 2
        0xd5, 0xd4, 0xe3, 0xe2, 0xd3, 0xd2, //            frame 3
        0xd5, 0xd4, 0xe3, 0xe2, 0x8b, 0x8a, //            frame 4
        0xe5, 0xe5, 0xe6, 0xe6, 0xeb, 0xeb, // piranha plant frame 1
        0xec, 0xec, 0xed, 0xed, 0xee, 0xee, //               frame 2
        0xfc, 0xfc, 0xd0, 0xd0, 0xd7, 0xd7, // podoboo
        0xbf, 0xbe, 0xc1, 0xc0, 0xc2, 0xfc, // bowser front frame 1
        0xc4, 0xc3, 0xc6, 0xc5, 0xc8, 0xc7, // bowser rear frame 1
        0xbf, 0xbe, 0xca, 0xc9, 0xc2, 0xfc, //        front frame 2
        0xc4, 0xc3, 0xc6, 0xc5, 0xcc, 0xcb, //        rear frame 2
        0xfc, 0xfc, 0xe8, 0xe7, 0xea, 0xe9, // bullet bill
        0xf2, 0xf2, 0xf3, 0xf3, 0xf2, 0xf2, // jumpspring frame 1
        0xf1, 0xf1, 0xf1, 0xf1, 0xfc, 0xfc, //            frame 2
        0xf0, 0xf0, 0xfc, 0xfc, 0xfc, 0xfc //            frame 3
    };

    // load two tiles of enemy graphics; the row advances gfx.yPos by eight for the next one
    return DrawOneSpriteRow(EnemyGraphicsTable_data[gfxOffset], EnemyGraphicsTable_data[gfxOffset + 1],
                            gfxOffset, oamSlot, gfx.flipBits, gfx.attributes, gfx.xPos, gfx.yPos);
}

//------------------------------------------------------------------------

// Inputs: enemyOffset = enemy object buffer offset (matches ObjectOffset)
// Outputs: none
void SMBEngine::EnemyGfxHandler(uint8_t enemyOffset)
{
    const uint8_t JumpspringFrameOffsets_data[] = {
        0x18, 0x19, 0x1a, 0x19, 0x18
    };

    const uint8_t EnemyAttributeData_data[] = {
        0x01, 0x02, 0x03, 0x02, 0x01, 0x01, 0x03, 0x03,
        0x03, 0x01, 0x01, 0x02, 0x02, 0x21, 0x01, 0x02,
        0x01, 0x01, 0x02, 0xff, 0x02, 0x02, 0x01, 0x01,
        0x02, 0x02, 0x02
    };

    const uint8_t EnemyGfxTableOffsets_data[] = {
        0x0c, 0x0c, 0x00, 0x0c, 0x0c, 0xa8, 0x54, 0x3c,
        0xea, 0x18, 0x48, 0x48, 0xcc, 0xc0, 0x18, 0x18,
        0x18, 0x90, 0x24, 0xff, 0x48, 0x9c, 0xd2, 0xd8,
        0xf0, 0xf6, 0xfc
    };

    EnemyGfxState gfx;
    gfx.yPos = M(Enemy_Y_Position + enemyOffset); // get enemy object vertical position
    gfx.xPos = M(Enemy_Rel_XPos);                 // get enemy object horizontal position, relative to screen
    writeData(0xeb, M(Enemy_SprDataOffset + enemyOffset)); // get sprite data offset
    writeData(VerticalFlipFlag, 0x00); // initialize vertical flip flag by default
    gfx.flipBits = M(Enemy_MovingDir + enemyOffset);   // get enemy object moving direction
    gfx.attributes = M(Enemy_SprAttrib + enemyOffset); // get enemy object sprite attributes
    if (M(Enemy_ID + enemyOffset) == PiranhaPlant                  // if not, branch
        && (M(PiranhaPlant_Y_Speed + enemyOffset) & 0x80) == 0     // if moving upwards, branch
        && M(EnemyFrameTimer + enemyOffset) != 0)                  // if timer expired, branch
    {
        return; // if all conditions fail, leave
    }

    //------------------------------------------------------------------------

    // CheckForRetainerObj
    uint8_t enemyState = M(Enemy_State + enemyOffset); // store enemy state
    writeData(0xed, enemyState);
    uint8_t altState = enemyState & 0b00011111; // nullify all but 5 LSB and use as the saved state
    uint8_t enemyCode = M(Enemy_ID + enemyOffset); // check for mushroom retainer/princess object
    if (enemyCode == RetainerObject)
    { // if not found, branch
        altState = 0x00; // if found, nullify saved state
        // set value that will not be used
        gfx.flipBits = 0x01;
        enemyCode = 0x15; // set value $15 as code for mushroom retainer/princess object
    } // CheckForBulletBillCV
    if (enemyCode == BulletBill_CannonVar)
    { // if not found, branch again
        --gfx.yPos; // decrement saved vertical position
        // get timer for enemy object; if expired, do not set priority bit, otherwise do so
        // SBBAt: set new sprite attributes
        gfx.attributes = M(EnemyFrameTimer + enemyOffset) != 0 ? 0b00100011 : 0x03;
        altState = 0x00; // nullify saved enemy state both here and in
        writeData(0xed, 0x00); // memory location here
        enemyCode = 0x08; // set specific value to unconditionally branch once
    } // CheckForJumpspring
    if (enemyCode == JumpspringObject)
    {
        altState = 0x03; // set enemy state -2 MSB here for jumpspring object
        // load data using the jumpspring's current frame number as offset
        enemyCode = JumpspringFrameOffsets_data[M(JumpspringAnimCtrl)];
    } // CheckForPodoboo
    writeData(0xef, enemyCode); // store saved enemy object value here
    writeData(0xec, altState); // and the enemy state -2 MSB, if not changed
    enemyOffset = M(ObjectOffset); // get enemy object offset
    // branch if not found, or if moving upwards
    if (enemyCode == 0x0c && (M(Enemy_Y_Speed + enemyOffset) & 0x80) == 0)
    {
        ++M(VerticalFlipFlag); // otherwise, set flag for vertical flip
    }

    // CheckBowserGfxFlag: if not drawing bowser at all, skip to something else
    uint8_t bowserGfxFlag = M(BowserGfxFlag);
    if (bowserGfxFlag != 0)
    { // SBwsrGfxOfs: if set to 1, draw bowser's front, otherwise draw bowser's rear
        writeData(0xef, bowserGfxFlag == 0x01 ? 0x16 : 0x17);
    }

    // CheckForGoomba: check value for goomba object
    uint8_t gfxId = M(0xef); // branch if not found
    if (gfxId == Goomba)
    {
        uint8_t state = M(Enemy_State + enemyOffset);
        if (state >= 0x02)
        { // if not defeated, go ahead and animate
            writeData(0xec, 0x04); // if defeated, write new value here
        } // GmbaAnim: check for d5 set in enemy object state, or timer disable flag set
        // if either condition true, do not animate goomba; also check for every eighth frame
        if (((state & 0b00100000) | M(TimerControl)) == 0 && (M(FrameCounter) & 0b00001000) == 0)
        {
            // invert bits to flip horizontally every eight frames, leave alone otherwise
            gfx.flipBits ^= 0b00000011;
        }
    }

    // CheckBowserFront: load sprite attribute using enemy object as offset, and add to bits
    // already loaded
    gfx.attributes |= EnemyAttributeData_data[gfxId];
    // load value based on enemy object as offset
    uint8_t gfxOffset = EnemyGfxTableOffsets_data[gfxId];
    altState = M(0xec); // get previously saved value
    if (bowserGfxFlag != 0)
    { // if not drawing bowser object at all, skip all of this
        if (bowserGfxFlag == 0x01)
        { // if not drawing front part, branch to draw the rear part
            // check bowser's body control bits; branch if d7 not set (controls bowser's mouth)
            if ((M(BowserBodyControls) & 0x80) != 0)
            {
                gfxOffset = 0xde; // otherwise load offset for second frame
            } // ChkFrontSte: check saved enemy state; if bowser not defeated, do not set flag
            if ((M(0xed) & 0b00100000) != 0)
            {
                // inlined FlipBowserOver: set vertical flip flag to nonzero
                writeData(VerticalFlipFlag, gfxOffset);
            }
        } // CheckBowserRear
        else
        {
            // check bowser's body control bits; branch if d0 not set (controls bowser's feet)
            if ((M(BowserBodyControls) & 0x01) != 0)
            {
                gfxOffset = 0xe4; // otherwise load offset for second frame
            } // ChkRearSte: check saved enemy state; if bowser not defeated, do not set flag
            if ((M(0xed) & 0b00100000) != 0)
            {
                gfx.yPos -= 0x10; // subtract 16 pixels from saved vertical position
                // inlined FlipBowserOver: set vertical flip flag to nonzero
                writeData(VerticalFlipFlag, gfxOffset);
            }
        } // DrawBowser
        DrawEnemyObject(gfxOffset, gfx); // draw bowser's graphics now
        return;
    }

    // CheckForSpiny: if not found, branch
    if (gfxOffset == 0x24)
    {
        if (altState == 0x05)
        { // otherwise branch
            gfxOffset = 0x30; // set to spiny egg offset
            gfx.flipBits = 0x02; // set enemy direction to reverse sprites horizontally
            writeData(0xec, 0x05); // set enemy state
        } // NotEgg: skip a big chunk of this if we found spiny but not in egg
        CheckForHammerBro(gfxOffset, gfx);
        return;
    }

    // CheckForLakitu: branch if not loaded
    if (gfxOffset == 0x90)
    {
        // check for d5 set in enemy state, and for the timer being in range
        if ((M(0xed) & 0b00100000) == 0 && M(FrenzyEnemyTimer) < 0x10)
        {
            gfxOffset = 0x96; // if d6 not set and timer in range, load alt frame for lakitu
        } // NoLAFr: skip this next part if we found lakitu but alt frame not needed
        CheckDefeatedState(gfxOffset, gfx);
        return;
    }

    // CheckUpsideDownShell: branch if enemy object => $04, or if enemy state < $02
    if (M(0xef) < 0x04 && altState >= 0x02)
    {
        gfxOffset = 0x5a; // set for upside-down koopa shell by default
        if (M(0xef) == BuzzyBeetle)
        {
            gfxOffset = 0x7e; // set for upside-down buzzy beetle shell if found
            ++gfx.yPos; // increment vertical position by one pixel
        }
    }

    // CheckRightSideUpShell: check for value set here
    if (M(0xec) != 0x04)
    {
        CheckForHammerBro(gfxOffset, gfx); // enemy state => $02 but not = $04, leave shell upside-down
        return;
    }
    gfxOffset = 0x72; // set right-side up buzzy beetle shell by default
    ++gfx.yPos; // increment saved vertical position by one pixel
    uint8_t enemyId = M(0xef);
    if (enemyId != BuzzyBeetle)
    { // branch if found
        gfxOffset = 0x66; // change to right-side up koopa shell if not found
        ++gfx.yPos; // and increment saved vertical position again
    } // CheckForDefdGoomba
    if (enemyId != Goomba)
    {
        CheckForHammerBro(gfxOffset, gfx); // failed buzzy beetle object test
        return;
    }
    gfxOffset = 0x54; // load for regular goomba
    // note that this only gets performed if enemy state => $02
    if ((M(0xed) & 0b00100000) == 0)
    { // check saved enemy state for d5 set, branch if set
        gfxOffset = 0x8a; // load offset for defeated goomba
        --gfx.yPos; // set different value and decrement saved vertical position
    }
    CheckForHammerBro(gfxOffset, gfx);
}

//------------------------------------------------------------------------

// Inputs: gfxOffset = graphics table offset selected by the caller (EnemyGfxHandler's
// EnemyGfxTableOffsets_data lookup); also reads zero-page 0xed/0xef left by EnemyGfxHandler
// Outputs: none
void SMBEngine::CheckForHammerBro(uint8_t gfxOffset, EnemyGfxState& gfx)
{
    const uint8_t EnemyAnimTimingBMask_data[] = {
        0x08, 0x18
    };

    // check for hammer bro object
    if (M(0xef) == HammerBro)
    {
        uint8_t enemyState = M(0xed);
        if (enemyState != 0)
        { // if in normal enemy state, fall through to animate it
            if ((enemyState & 0b00001000) == 0)
            {
                CheckDefeatedState(gfxOffset, gfx); // if d3 not set, branch further away
                return;
            }
            gfxOffset = 0xb4; // otherwise load offset for different frame
        }
    } // CheckForBloober: skipped if found
    else if (gfxOffset != 0x48)
    {
        uint8_t intervalTimer = M(EnemyIntervalTimer + M(ObjectOffset));
        if (intervalTimer >= 0x05)
        {
            CheckDefeatedState(gfxOffset, gfx); // branch if some timer is above a certain point
            return;
        }
        if (gfxOffset == 0x3c)
        { // branch if not found this time
            if (intervalTimer == 0x01)
            {
                CheckDefeatedState(gfxOffset, gfx); // branch if timer is set to certain point
                return;
            }
            gfx.yPos += 3; // increment saved vertical coordinate three pixels
            CheckAnimationStop(gfxOffset, gfx); // and do something else
            return;
        }
    }

    // CheckToAnimateEnemy: check for specific enemy objects
    uint8_t enemyId = M(0xef);
    // branch if goomba, bullet bill (note both variants use $08 here), or podoboo
    if (enemyId == Goomba || enemyId == 0x08 || enemyId == Podoboo || enemyId >= 0x18)
    {
        CheckDefeatedState(gfxOffset, gfx);
        return;
    }
    if (enemyId == 0x15)
    { // princess/mushroom retainer uses different code here
        // are we on world 8?
        if (M(WorldNumber) < World8)
        {
            gfxOffset = 0xa2;      // otherwise, set for mushroom retainer object instead
            writeData(0xec, 0x03); // set alternate state here
        } // if so, leave the offset alone (use princess)
        CheckDefeatedState(gfxOffset, gfx);
        return;
    } // CheckForSecondFrame
    // load frame counter and mask it (the second EnemyAnimTimingBMask_data byte is residual: the
    // only path that selects it returns above without ever reaching here)
    if ((M(FrameCounter) & EnemyAnimTimingBMask_data[0]) != 0)
    {
        CheckDefeatedState(gfxOffset, gfx); // branch if timing is off
        return;
    }
    CheckAnimationStop(gfxOffset, gfx);
}

//------------------------------------------------------------------------

// Inputs: gfxOffset = graphics table offset of the enemy's current animation frame (reads zero-page
// 0xed left by EnemyGfxHandler)
// Outputs: none
void SMBEngine::CheckAnimationStop(uint8_t gfxOffset, EnemyGfxState& gfx)
{
    // check saved enemy state for d7 or d5, or check for timers stopped
    if (((M(0xed) & 0b10100000) | M(TimerControl)) == 0)
    { // if either condition true, leave the frame alone
        gfxOffset += 0x06; // add $06 to current enemy offset to animate various enemy objects
    }
    CheckDefeatedState(gfxOffset, gfx);
}

//------------------------------------------------------------------------

// Inputs: gfxOffset = graphics table offset chosen for the enemy, passed straight through (reads
// zero-page 0xed/0xef left by EnemyGfxHandler/CheckForHammerBro)
// Outputs: none
void SMBEngine::CheckDefeatedState(uint8_t gfxOffset, EnemyGfxState& gfx)
{
    // a defeated (d5 of saved enemy state set) enemy of $04 or above is drawn upside-down
    if ((M(0xed) & 0b00100000) != 0 && M(0xef) >= 0x04)
    {
        writeData(VerticalFlipFlag, 0x01); // set vertical flip flag
        writeData(0xec, 0x00);             // init saved value here
    }
    DrawEnemyObject(gfxOffset, gfx);
}

//------------------------------------------------------------------------

// Inputs: gfxOffset = graphics table offset of the enemy's first row of tiles (reads zero-page
// 0xeb/0xef/0xec, VerticalFlipFlag, and BowserGfxFlag left by
// EnemyGfxHandler/CheckForHammerBro/CheckDefeatedState)
// Outputs: none
void SMBEngine::DrawEnemyObject(uint8_t gfxOffset, EnemyGfxState& gfx)
{
    uint8_t oamSlot = M(0xeb); // load sprite data offset
    // draw six tiles of data into sprite data
    std::tie(gfxOffset, oamSlot) = DrawEnemyObjRow(gfxOffset, oamSlot, gfx);
    std::tie(gfxOffset, oamSlot) = DrawEnemyObjRow(gfxOffset, oamSlot, gfx);
    std::tie(gfxOffset, oamSlot) = DrawEnemyObjRow(gfxOffset, oamSlot, gfx);
    // get enemy object offset, and from it the sprite data offset
    uint8_t sprOffset = M(Enemy_SprDataOffset + M(ObjectOffset));
    uint8_t enemyCode = M(0xef);
    if (enemyCode == 0x08)
    { // SkipToOffScrChk: for bullet bill, jump if found
        SprObjectOffscrChk();
        return;
    }

    // CheckForVerticalFlip: check if vertical flip flag is set here, branch if not
    if (M(VerticalFlipFlag) != 0)
    {
        // get attributes of first sprite we dealt with and set the bit for vertical flip, then
        // store it two bytes along, in the attribute bytes of enemy obj sprite data
        DumpSixSpr(M(Sprite_Attributes + sprOffset) | 0b10000000, sprOffset + 2);
        uint8_t rowOffset = sprOffset;
        // hammer bro, lakitu and enemy object => $15 flip their first row; everything else its
        // second
        if (enemyCode != HammerBro && enemyCode != Lakitu && enemyCode < 0x15)
        {
            rowOffset += 0x08;
        } // FlipEnemyVertically: exchange third row tiles with first or second row tiles
        uint8_t leftTile = M(Sprite_Tilenumber + rowOffset); // load first or second row tiles
        uint8_t rightTile = M(Sprite_Tilenumber + 4 + rowOffset);
        writeData(Sprite_Tilenumber + rowOffset, M(Sprite_Tilenumber + 16 + sprOffset));
        writeData(Sprite_Tilenumber + 4 + rowOffset, M(Sprite_Tilenumber + 20 + sprOffset));
        writeData(Sprite_Tilenumber + 20 + sprOffset, rightTile); // and save in third row
        writeData(Sprite_Tilenumber + 16 + sprOffset, leftTile);
    }

    // CheckForESymmetry: are we drawing bowser at all?
    if (M(BowserGfxFlag) != 0)
    { // branch if so
        SprObjectOffscrChk();
        return;
    }
    uint8_t altState = M(0xec); // get alternate enemy state
    if (enemyCode == 0x05)
    {
        SprObjectOffscrChk(); // jump if found
        return;
    }
    // ContES: branch if bloober, piranha plant or podoboo
    bool mirrorEnemyGfx = enemyCode == Bloober || enemyCode == PiranhaPlant || enemyCode == Podoboo;
    if (!mirrorEnemyGfx && !(enemyCode == Spiny && altState != 0x05))
    { // spiny that is not an egg skips all of this
        // ESRtnr: check for princess/mushroom retainer object
        if (enemyCode == 0x15)
        { // set horizontal flip on bottom right sprite; note that palette bits were already set
            // earlier
            writeData(Sprite_Attributes + 20 + sprOffset, 0x42);
        } // SpnySC: if alternate enemy state set to 1 or 0, branch
        mirrorEnemyGfx = altState >= 0x02;
    }

    // MirrorEnemyGfx (the disassembly re-checks BowserGfxFlag here, but CheckForESymmetry above is
    // the only way in and has already left if it was set)
    if (mirrorEnemyGfx)
    {
        // load attribute bits of first sprite
        uint8_t attributes = M(Sprite_Attributes + sprOffset) & 0b10100011;
        writeData(Sprite_Attributes + sprOffset, attributes); // save vertical flip, priority, and
        writeData(Sprite_Attributes + 8 + sprOffset, attributes); // palette bits in left sprite
        writeData(Sprite_Attributes + 16 + sprOffset, attributes); // column of enemy object OAM data
        attributes |= 0b01000000; // set horizontal flip
        if (altState == 0x05)
        { // if alternate state not set to $05, branch
            attributes |= 0b10000000; // otherwise set vertical flip
        } // EggExc: set bits of right sprite column
        writeData(Sprite_Attributes + 4 + sprOffset, attributes);
        writeData(Sprite_Attributes + 12 + sprOffset, attributes); // of enemy object sprite data
        writeData(Sprite_Attributes + 20 + sprOffset, attributes);
        if (altState == 0x04)
        { // branch if not $04
            // get second row left sprite attributes
            attributes = M(Sprite_Attributes + 8 + sprOffset) | 0b10000000;
            writeData(Sprite_Attributes + 8 + sprOffset, attributes); // store bits with vertical
            writeData(Sprite_Attributes + 16 + sprOffset, attributes); // flip in 2nd/3rd row left
            attributes |= 0b01000000;
            writeData(Sprite_Attributes + 12 + sprOffset, attributes); // store with horizontal and
            writeData(Sprite_Attributes + 20 + sprOffset, attributes); // vertical flip in 2nd/3rd row right
        }
    }

    // CheckToMirrorLakitu: check for lakitu enemy object, branch if not found
    if (enemyCode == Lakitu)
    {
        if (M(VerticalFlipFlag) == 0)
        { // branch if vertical flip flag not set
            // save vertical flip and palette bits in third row left sprite
            writeData(Sprite_Attributes + 16 + sprOffset,
                      M(Sprite_Attributes + 16 + sprOffset) & 0b10000001);
            // set horizontal flip and palette bits in third row right sprite
            uint8_t attributes = M(Sprite_Attributes + 20 + sprOffset) | 0b01000001;
            writeData(Sprite_Attributes + 20 + sprOffset, attributes);
            if (M(FrenzyEnemyTimer) < 0x10)
            { // leave the rest alone if timer has not reached a certain range
                // otherwise set same for second row right sprite
                writeData(Sprite_Attributes + 12 + sprOffset, attributes);
                // preserve vertical flip and palette bits for left sprite
                writeData(Sprite_Attributes + 8 + sprOffset, attributes & 0b10000001);
            }
            SprObjectOffscrChk();
            return;
        } // NVFLak: get first row left sprite attributes
        // save vertical flip and palette bits
        writeData(Sprite_Attributes + sprOffset, M(Sprite_Attributes + sprOffset) & 0b10000001);
        // get first row right sprite attributes and set horizontal flip and palette bits, noting
        // that vertical flip is left as-is
        writeData(Sprite_Attributes + 4 + sprOffset,
                  M(Sprite_Attributes + 4 + sprOffset) | 0b01000001);
    } // CheckToMirrorJSpring
    // check for jumpspring object (any frame); branch if not jumpspring object at all
    if (enemyCode >= 0x18)
    {
        writeData(Sprite_Attributes + 8 + sprOffset, 0x82); // set vertical flip and palette bits of
        writeData(Sprite_Attributes + 16 + sprOffset, 0x82); // second and third row left sprites
        // set, in addition to those, horizontal flip for second and third row right sprites
        writeData(Sprite_Attributes + 12 + sprOffset, 0b11000010);
        writeData(Sprite_Attributes + 20 + sprOffset, 0b11000010);
    }
    SprObjectOffscrChk();
}
