// The EnemyGfxHandler subsystem: everything EnemyGfxHandler() reaches that nothing
// outside it reaches, and so nothing outside it needs.
//
// Moved out of SMB.cpp by tools/split.py. These are methods of SMBEngine like every other
// routine of the game and are declared in SMBEngine.hpp; the file they are written in is the
// only thing that is different, and tools/layers.py is what keeps it meaning something.
//
#include "SMB.hpp"

//------------------------------------------------------------------------

// Inputs: x = index into EnemyGraphicsTable_data, and the sprite-pair index forwarded to
// DrawOneSpriteRow; y = OAM slot index forwarded to DrawOneSpriteRow
// Outputs: x = x+2; y = y+8 (see DrawOneSpriteRow/DrawSpriteObject; the caller invokes this three
// times in a row to advance across one row of tiles)
void SMBEngine::DrawEnemyObjRow()
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

    // load two tiles of enemy graphics
    writeData(0x00, EnemyGraphicsTable_data[x]);
    a = EnemyGraphicsTable_data[1 + x];

    DrawOneSpriteRow(a, x, y);
    return;
}

//------------------------------------------------------------------------

// Inputs: x = enemy object buffer offset (matches ObjectOffset)
// Outputs: none (register state on return is whatever the delegated call -
// DrawEnemyObject/CheckForHammerBro/CheckDefeatedState/SprObjectOffscrChk - leaves it in)
void SMBEngine::EnemyGfxHandler()
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

    // get enemy object vertical position
    writeData(0x02, M(Enemy_Y_Position + x));
    // get enemy object horizontal position
    writeData(0x05, M(Enemy_Rel_XPos)); // relative to screen
    writeData(0xeb, M(Enemy_SprDataOffset + x)); // get sprite data offset
    writeData(VerticalFlipFlag, 0x00); // initialize vertical flip flag by default
    writeData(0x03, M(Enemy_MovingDir + x)); // get enemy object moving direction
    writeData(0x04, M(Enemy_SprAttrib + x)); // get enemy object sprite attributes
    a = M(Enemy_ID + x);
    if (a != PiranhaPlant)
        goto CheckForRetainerObj; // if not, branch
    if ((M(PiranhaPlant_Y_Speed + x) & 0x80) != 0)
        goto CheckForRetainerObj; // if piranha plant moving upwards, branch
    y = M(EnemyFrameTimer + x);
    if (y == 0)
        goto CheckForRetainerObj; // if timer for movement expired, branch
    return; // if all conditions fail, leave

//------------------------------------------------------------------------

CheckForRetainerObj:
    a = M(Enemy_State + x); // store enemy state
    writeData(0xed, a);
    a &= 0b00011111; // nullify all but 5 LSB and use as Y
    y = a;
    a = M(Enemy_ID + x); // check for mushroom retainer/princess object
    if (a == RetainerObject)
    { // if not found, branch
        y = 0x00; // if found, nullify saved state in Y
        // set value that will not be used
        writeData(0x03, 0x01);
        a = 0x15; // set value $15 as code for mushroom retainer/princess object
    } // CheckForBulletBillCV
    if (a == BulletBill_CannonVar)
    { // if not found, branch again
        --M(0x02); // decrement saved vertical position
        a = 0x03;
        // get timer for enemy object
        if (M(EnemyFrameTimer + x) != 0)
        { // if expired, do not set priority bit
            a = 0b00100011; // otherwise do so
        } // SBBAt: set new sprite attributes
        writeData(0x04, a);
        y = 0x00; // nullify saved enemy state both in Y and in
        writeData(0xed, 0x00); // memory location here
        a = 0x08; // set specific value to unconditionally branch once
    } // CheckForJumpspring
    if (a == JumpspringObject)
    {
        y = 0x03; // set enemy state -2 MSB here for jumpspring object
        x = M(JumpspringAnimCtrl); // get current frame number for jumpspring object
        a = JumpspringFrameOffsets_data[x]; // load data using frame number as offset
    } // CheckForPodoboo
    writeData(0xef, a); // store saved enemy object value here
    writeData(0xec, y); // and Y here (enemy state -2 MSB if not changed)
    x = M(ObjectOffset); // get enemy object offset
    if (a != 0x0c)
        goto CheckBowserGfxFlag; // branch if not found
    // if moving upwards, branch
    if ((M(Enemy_Y_Speed + x) & 0x80) != 0)
        goto CheckBowserGfxFlag;
    ++M(VerticalFlipFlag); // otherwise, set flag for vertical flip

CheckBowserGfxFlag:
    a = M(BowserGfxFlag); // if not drawing bowser at all, skip to something else
    if (a != 0)
    {
        y = 0x16; // if set to 1, draw bowser's front
        if (a != 0x01)
        {
            y = 0x17; // otherwise draw bowser's rear
        } // SBwsrGfxOfs
        writeData(0xef, y);
    } // CheckForGoomba
    y = M(0xef); // check value for goomba object
    if (y != Goomba)
        goto CheckBowserFront; // branch if not found
    a = M(Enemy_State + x);
    if (a >= 0x02)
    { // if not defeated, go ahead and animate
        x = 0x04; // if defeated, write new value here
        writeData(0xec, 0x04);
    } // GmbaAnim: check for d5 set in enemy object state
    a &= 0b00100000;
    a |= M(TimerControl); // or timer disable flag set
    if (a != 0)
        goto CheckBowserFront; // if either condition true, do not animate goomba
    a = M(FrameCounter) & 0b00001000; // check for every eighth frame
    if (a != 0)
        goto CheckBowserFront;
    a = M(0x03) ^ 0b00000011; // invert bits to flip horizontally every eight frames
    writeData(0x03, a); // leave alone otherwise

CheckBowserFront:
    // load sprite attribute using enemy object
    a = EnemyAttributeData_data[y] | M(0x04); // as offset, and add to bits already loaded
    writeData(0x04, a);
    // load value based on enemy object as offset
    x = EnemyGfxTableOffsets_data[y]; // save as X
    y = M(0xec); // get previously saved value
    a = M(BowserGfxFlag);
    if (a != 0)
    { // if not drawing bowser object at all, skip all of this
        if (a == 0x01)
        { // if not drawing front part, branch to draw the rear part
            // check bowser's body control bits
            if ((M(BowserBodyControls) & 0x80) != 0)
            { // branch if d7 not set (control's bowser's mouth)
                x = 0xde; // otherwise load offset for second frame
            } // ChkFrontSte: check saved enemy state
            a = M(0xed) & 0b00100000; // if bowser not defeated, do not set flag
            if (a == 0)
                goto DrawBowser;

FlipBowserOver:
            writeData(VerticalFlipFlag, x); // set vertical flip flag to nonzero

DrawBowser:
            DrawEnemyObject(); // draw bowser's graphics now
            return;
        } // CheckBowserRear
        // check bowser's body control bits
        a = M(BowserBodyControls) & 0x01;
        if (a != 0)
        { // branch if d0 not set (control's bowser's feet)
            x = 0xe4; // otherwise load offset for second frame
        } // ChkRearSte: check saved enemy state
        a = M(0xed) & 0b00100000; // if bowser not defeated, do not set flag
        if (a == 0)
            goto DrawBowser;
        a = M(0x02); // subtract 16 pixels from
        a -= 0x10;
        writeData(0x02, a);
        goto FlipBowserOver; // jump to set vertical flip flag
    } // CheckForSpiny
    if (x == 0x24)
    { // if not found, branch
        if (y == 0x05)
        { // otherwise branch
            x = 0x30; // set to spiny egg offset
            writeData(0x03, 0x02); // set enemy direction to reverse sprites horizontally
            a = 0x05;
            writeData(0xec, 0x05); // set enemy state
        } // NotEgg: skip a big chunk of this if we found spiny but not in egg
        CheckForHammerBro();
        return;
    } // CheckForLakitu
    if (x == 0x90)
    { // branch if not loaded
        a = M(0xed) & 0b00100000; // check for d5 set in enemy state
        if (a != 0)
            goto NoLAFr; // branch if set
        if (M(FrenzyEnemyTimer) >= 0x10)
            goto NoLAFr; // branch if not
        x = 0x96; // if d6 not set and timer in range, load alt frame for lakitu

NoLAFr: // skip this next part if we found lakitu but alt frame not needed
        CheckDefeatedState();
        return;
    } // CheckUpsideDownShell
    // check for enemy object => $04
    if (M(0xef) >= 0x04)
        goto CheckRightSideUpShell; // branch if true
    if (y < 0x02)
        goto CheckRightSideUpShell; // branch if enemy state < $02
    x = 0x5a; // set for upside-down koopa shell by default
    if (M(0xef) != BuzzyBeetle)
        goto CheckRightSideUpShell;
    x = 0x7e; // set for upside-down buzzy beetle shell if found
    ++M(0x02); // increment vertical position by one pixel

CheckRightSideUpShell:
    // check for value set here
    if (M(0xec) != 0x04)
    {
        CheckForHammerBro(); // enemy state => $02 but not = $04, leave shell upside-down
        return;
    }
    x = 0x72; // set right-side up buzzy beetle shell by default
    ++M(0x02); // increment saved vertical position by one pixel
    y = M(0xef);
    if (y != BuzzyBeetle)
    { // branch if found
        x = 0x66; // change to right-side up koopa shell if not found
        ++M(0x02); // and increment saved vertical position again
    } // CheckForDefdGoomba
    if (y != Goomba)
    {
        CheckForHammerBro(); // failed buzzy beetle object test)
        return;
    }
    x = 0x54; // load for regular goomba
    // note that this only gets performed if enemy state => $02
    a = M(0xed) & 0b00100000; // check saved enemy state for d5 set
    if (a != 0)
    {
        CheckForHammerBro(); // branch if set
        return;
    }
    x = 0x8a; // load offset for defeated goomba
    --M(0x02); // set different value and decrement saved vertical position
    CheckForHammerBro();
    return;
}

//------------------------------------------------------------------------

// Inputs: x = graphics table offset selected by the caller (EnemyGfxHandler's
// EnemyGfxTableOffsets_data[y] lookup); also reads zero-page 0xed/0xef/0xec left by EnemyGfxHandler
// Outputs: none (delegates to CheckDefeatedState, which controls the final register state)
void SMBEngine::CheckForHammerBro()
{
    const uint8_t EnemyAnimTimingBMask_data[] = {
        0x08, 0x18
    };

    y = M(ObjectOffset);
    // check for hammer bro object
    if (M(0xef) == HammerBro)
    { // branch if not found
        a = M(0xed);
        if (a == 0)
            goto CheckToAnimateEnemy; // branch if not in normal enemy state
        a &= 0b00001000;
        if (a == 0)
        {
            CheckDefeatedState(); // if d3 not set, branch further away
            return;
        }
        x = 0xb4; // otherwise load offset for different frame
        if (x != 0)
            goto CheckToAnimateEnemy; // unconditional branch
    } // CheckForBloober
    if (x == 0x48)
        goto CheckToAnimateEnemy; // branch if found
    a = M(EnemyIntervalTimer + y);
    if (a >= 0x05)
    {
        CheckDefeatedState(); // branch if some timer is above a certain point
        return;
    }
    if (x != 0x3c)
        goto CheckToAnimateEnemy; // branch if not found this time
    if (a == 0x01)
    {
        CheckDefeatedState(); // branch if timer is set to certain point
        return;
    }
    ++M(0x02); // increment saved vertical coordinate three pixels
    ++M(0x02);
    ++M(0x02);
    goto CheckAnimationStop; // and do something else

CheckToAnimateEnemy:
    a = M(0xef); // check for specific enemy objects
    if (a == Goomba)
    {
        CheckDefeatedState(); // branch if goomba
        return;
    }
    if (a == 0x08)
    {
        CheckDefeatedState(); // branch if bullet bill (note both variants use $08 here)
        return;
    }
    if (a == Podoboo)
    {
        CheckDefeatedState(); // branch if podoboo
        return;
    }
    if (a >= 0x18)
    {
        CheckDefeatedState();
        return;
    }
    y = 0x00;
    if (a == 0x15)
    { // which uses different code here, branch if not found
        y = 0x01; // residual instruction
        // are we on world 8?
        if (M(WorldNumber) >= World8)
        {
            CheckDefeatedState(); // if so, leave the offset alone (use princess)
            return;
        }
        x = 0xa2; // otherwise, set for mushroom retainer object instead
        a = 0x03; // set alternate state here
        writeData(0xec, 0x03);
        if (a != 0)
        {
            CheckDefeatedState(); // unconditional branch
            return;
        }
    } // CheckForSecondFrame
    // load frame counter
    a = M(FrameCounter) & EnemyAnimTimingBMask_data[y]; // mask it (partly residual, one byte not ever used)
    if (a != 0)
    {
        CheckDefeatedState(); // branch if timing is off
        return;
    }

CheckAnimationStop:
    // check saved enemy state
    a = M(0xed) & 0b10100000; // for d7 or d5, or check for timers stopped
    a |= M(TimerControl);
    if (a != 0)
    {
        CheckDefeatedState(); // if either condition true, branch
        return;
    }
    a = x;
    a += 0x06; // add $06 to current enemy offset
    x = a; // to animate various enemy objects
    CheckDefeatedState();
    return;
}

//------------------------------------------------------------------------

// Inputs: none (reads zero-page 0xed/0xef left by EnemyGfxHandler/CheckForHammerBro)
// Outputs: none (delegates to DrawEnemyObject, which controls the final register state)
void SMBEngine::CheckDefeatedState()
{
    // check saved enemy state
    a = M(0xed) & 0b00100000; // for d5 set
    if (a == 0)
    {
        DrawEnemyObject(); // branch if not set
        return;
    }
    if (M(0xef) < 0x04)
    {
        DrawEnemyObject(); // branch if less
        return;
    }
    writeData(VerticalFlipFlag, 0x01); // set vertical flip flag
    y = 0x00;
    writeData(0xec, 0x00); // init saved value here
    DrawEnemyObject();
    return;
}

//------------------------------------------------------------------------

// Inputs: none (reloads x/y fresh from ObjectOffset/zero-page 0xeb; reads zero-page 0xef/0xec/0xed,
// VerticalFlipFlag, and BowserGfxFlag left by EnemyGfxHandler/CheckForHammerBro/CheckDefeatedState)
// Outputs: none
void SMBEngine::DrawEnemyObject()
{
    y = M(0xeb); // load sprite data offset
    DrawEnemyObjRow(); // draw six tiles of data
    DrawEnemyObjRow(); // into sprite data
    DrawEnemyObjRow();
    x = M(ObjectOffset); // get enemy object offset
    y = M(Enemy_SprDataOffset + x); // get sprite data offset
    if (M(0xef) == 0x08)
    { // for bullet bill, branch if not found

SkipToOffScrChk:
        SprObjectOffscrChk(); // jump if found
        return;
    } // CheckForVerticalFlip
    // check if vertical flip flag is set here
    if (M(VerticalFlipFlag) != 0)
    { // branch if not
        // get attributes of first sprite we dealt with
        a = M(Sprite_Attributes + y) | 0b10000000; // set bit for vertical flip
        ++y;
        ++y; // increment two bytes so that we store the vertical flip
        DumpSixSpr(a, y); // in attribute bytes of enemy obj sprite data
        --y;
        --y; // now go back to the Y coordinate offset
        a = y;
        x = a; // give offset to X
        a = M(0xef);
        if (a == HammerBro)
            goto FlipEnemyVertically;
        if (a == Lakitu)
            goto FlipEnemyVertically; // branch for hammer bro or lakitu
        if (a >= 0x15)
            goto FlipEnemyVertically; // also branch if enemy object => $15
        a = x;
        a += 0x08; // if not selected objects or => $15, set
        x = a; // offset in X for next row

FlipEnemyVertically:
        a = M(Sprite_Tilenumber + x); // load first or second row tiles
        pha(); // and save tiles to the stack
        a = M(Sprite_Tilenumber + 4 + x);
        pha();
        // exchange third row tiles
        writeData(Sprite_Tilenumber + x, M(Sprite_Tilenumber + 16 + y)); // with first or second row tiles
        writeData(Sprite_Tilenumber + 4 + x, M(Sprite_Tilenumber + 20 + y));
        pla(); // pull first or second row tiles from stack
        writeData(Sprite_Tilenumber + 20 + y, a); // and save in third row
        pla();
        writeData(Sprite_Tilenumber + 16 + y, a);
    } // CheckForESymmetry
    // are we drawing bowser at all?
    if (M(BowserGfxFlag) != 0)
        goto SkipToOffScrChk; // branch if so
    a = M(0xef);
    x = M(0xec); // get alternate enemy state
    if (a == 0x05)
    {
        SprObjectOffscrChk(); // jump if found
        return;
    } // ContES: check for bloober object
    if (a == Bloober)
        goto MirrorEnemyGfx;
    if (a == PiranhaPlant)
        goto MirrorEnemyGfx;
    if (a == Podoboo)
        goto MirrorEnemyGfx; // branch if either of three are found
    if (a == Spiny)
    { // branch closer if not found
        if (x != 0x05)
            goto CheckToMirrorLakitu; // branch if not an egg, otherwise
    } // ESRtnr: check for princess/mushroom retainer object
    if (a == 0x15)
    {
        a = 0x42; // set horizontal flip on bottom right sprite
        writeData(Sprite_Attributes + 20 + y, 0x42); // note that palette bits were already set earlier
    } // SpnySC: if alternate enemy state set to 1 or 0, branch
    if (x < 0x02)
        goto CheckToMirrorLakitu;

MirrorEnemyGfx:
    // if enemy object is bowser, skip all of this
    if (M(BowserGfxFlag) != 0)
        goto CheckToMirrorLakitu;
    // load attribute bits of first sprite
    a = M(Sprite_Attributes + y) & 0b10100011;
    writeData(Sprite_Attributes + y, a); // save vertical flip, priority, and palette bits
    writeData(Sprite_Attributes + 8 + y, a); // in left sprite column of enemy object OAM data
    writeData(Sprite_Attributes + 16 + y, a);
    a |= 0b01000000; // set horizontal flip
    if (x == 0x05)
    { // if alternate state not set to $05, branch
        a |= 0b10000000; // otherwise set vertical flip
    } // EggExc: set bits of right sprite column
    writeData(Sprite_Attributes + 4 + y, a);
    writeData(Sprite_Attributes + 12 + y, a); // of enemy object sprite data
    writeData(Sprite_Attributes + 20 + y, a);
    if (x != 0x04)
        goto CheckToMirrorLakitu; // branch if not $04
    // get second row left sprite attributes
    a = M(Sprite_Attributes + 8 + y) | 0b10000000;
    writeData(Sprite_Attributes + 8 + y, a); // store bits with vertical flip in
    writeData(Sprite_Attributes + 16 + y, a); // second and third row left sprites
    a |= 0b01000000;
    writeData(Sprite_Attributes + 12 + y, a); // store with horizontal and vertical flip in
    writeData(Sprite_Attributes + 20 + y, a); // second and third row right sprites

CheckToMirrorLakitu:
    // check for lakitu enemy object
    if (M(0xef) == Lakitu)
    { // branch if not found
        if (M(VerticalFlipFlag) == 0)
        { // branch if vertical flip flag not set
            // save vertical flip and palette bits
            a = M(Sprite_Attributes + 16 + y) & 0b10000001; // in third row left sprite
            writeData(Sprite_Attributes + 16 + y, a);
            // set horizontal flip and palette bits
            a = M(Sprite_Attributes + 20 + y) | 0b01000001; // in third row right sprite
            writeData(Sprite_Attributes + 20 + y, a);
            x = M(FrenzyEnemyTimer); // check timer
            if (x >= 0x10)
            {
                SprObjectOffscrChk(); // branch if timer has not reached a certain range
                return;
            }
            writeData(Sprite_Attributes + 12 + y, a); // otherwise set same for second row right sprite
            a &= 0b10000001;
            writeData(Sprite_Attributes + 8 + y, a); // preserve vertical flip and palette bits for left sprite
            if (x < 0x10)
            {
                SprObjectOffscrChk(); // unconditional branch
                return;
            }
        } // NVFLak: get first row left sprite attributes
        a = M(Sprite_Attributes + y) & 0b10000001;
        writeData(Sprite_Attributes + y, a); // save vertical flip and palette bits
        // get first row right sprite attributes
        a = M(Sprite_Attributes + 4 + y) | 0b01000001; // set horizontal flip and palette bits
        writeData(Sprite_Attributes + 4 + y, a); // note that vertical flip is left as-is
    } // CheckToMirrorJSpring
    // check for jumpspring object (any frame)
    if (M(0xef) < 0x18)
    {
        SprObjectOffscrChk(); // branch if not jumpspring object at all
        return;
    }
    writeData(Sprite_Attributes + 8 + y, 0x82); // set vertical flip and palette bits of
    writeData(Sprite_Attributes + 16 + y, 0x82); // second and third row left sprites
    a = 0b11000010;
    writeData(Sprite_Attributes + 12 + y, 0b11000010); // set, in addition to those, horizontal flip
    writeData(Sprite_Attributes + 20 + y, 0b11000010); // for second and third row right sprites
    SprObjectOffscrChk();
    return;
}
