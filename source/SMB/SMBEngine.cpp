#include <cstring>

#include "../Emulation/APU.hpp"
#include "../Emulation/Controller.hpp"
#include "../Emulation/PPU.hpp"

#include "SMBConstants.hpp"
#include "SMBEngine.hpp"

#define DATA_STORAGE_OFFSET 0x8000 // Starting address for storing constant data

//---------------------------------------------------------------------
// Public interface
//---------------------------------------------------------------------

const std::size_t SMBEngine::RAM_SIZE;

/**
 * The whole of what the game changes as it runs, which is what a save state has to
 * put back.
 *
 * dataStorage is not here. The data tables are written into it once, at startup, and
 * nothing writes to it afterwards -- the single-byte ram[]= only reaches the RAM,
 * the PPU and the IO registers, and drops everything above them. chr and dataPointers
 * are likewise fixed once the ROM is loaded.
 *
 * musicData points into the song tables, which are constants in the program, so it
 * keeps meaning the same thing for as long as the program runs. That is all a save
 * state has to survive; it is never written to a file.
 */
struct SMBEngine::State
{
    uint8_t ram[RAM_SIZE];
    const uint8_t *musicData;
    PPUState ppu;
};

SMBEngine::SMBEngine(uint8_t *romImage, bool enableAudio)
    : audioEnabled(enableAudio), altRegContentFlag_(ram[AltRegContentFlag]), groundMusicHeaderOfs_(ram[GroundMusicHeaderOfs]),
      pauseModeFlag_(ram[PauseModeFlag]), areaMusicBuffer_Alt_(ram[AreaMusicBuffer_Alt]), noteLengthTblAdder_(ram[NoteLengthTblAdder]),
      noiseDataLoopbackOfs_(ram[NoiseDataLoopbackOfs]), dAC_Counter_(ram[DAC_Counter]), noteLenLookupTblOfs_(ram[NoteLenLookupTblOfs]),
      musicOffset_Noise_(ram[MusicOffset_Noise]), musicOffset_Triangle_(ram[MusicOffset_Triangle]),
      musicOffset_Square1_(ram[MusicOffset_Square1]), musicOffset_Square2_(ram[MusicOffset_Square2]),
      pauseSoundBuffer_(ram[PauseSoundBuffer]), eventMusicBuffer_(ram[EventMusicBuffer]), areaMusicBuffer_(ram[AreaMusicBuffer]),
      noiseSoundBuffer_(ram[NoiseSoundBuffer]), square2SoundBuffer_(ram[Square2SoundBuffer]), square1SoundBuffer_(ram[Square1SoundBuffer]),
      eventMusicQueue_(ram[EventMusicQueue]), areaMusicQueue_(ram[AreaMusicQueue]), noiseSoundQueue_(ram[NoiseSoundQueue]),
      square2SoundQueue_(ram[Square2SoundQueue]), square1SoundQueue_(ram[Square1SoundQueue]), pauseSoundQueue_(ram[PauseSoundQueue]),
      noise_SfxLenCounter_(ram[Noise_SfxLenCounter]), sfx_SecondaryCounter_(ram[Sfx_SecondaryCounter]),
      squ2_SfxLenCounter_(ram[Squ2_SfxLenCounter]), squ1_SfxLenCounter_(ram[Squ1_SfxLenCounter]),
      noise_BeatLenCounter_(ram[Noise_BeatLenCounter]), tri_NoteLenCounter_(ram[Tri_NoteLenCounter]),
      tri_NoteLenBuffer_(ram[Tri_NoteLenBuffer]), squ1_EnvelopeDataCtrl_(ram[Squ1_EnvelopeDataCtrl]),
      squ1_NoteLenCounter_(ram[Squ1_NoteLenCounter]), squ2_EnvelopeDataCtrl_(ram[Squ2_EnvelopeDataCtrl]),
      squ2_NoteLenCounter_(ram[Squ2_NoteLenCounter]), squ2_NoteLenBuffer_(ram[Squ2_NoteLenBuffer]),
      fireworksCounter_(ram[FireworksCounter]), maxRangeFromOrigin_(ram[MaxRangeFromOrigin]), bowserHitPoints_(ram[BowserHitPoints]),
      bowserGfxFlag_(ram[BowserGfxFlag]), bridgeCollapseOffset_(ram[BridgeCollapseOffset]), bowserFront_Offset_(ram[BowserFront_Offset]),
      bowserFlameTimerCtrl_(ram[BowserFlameTimerCtrl]), bowserOrigXPos_(ram[BowserOrigXPos]),
      bowserMovementSpeed_(ram[BowserMovementSpeed]), bowserFeetCounter_(ram[BowserFeetCounter]),
      bowserBodyControls_(ram[BowserBodyControls]), numberofGroupEnemies_(ram[NumberofGroupEnemies]),
      duplicateObj_Offset_(ram[DuplicateObj_Offset]), lakituReappearTimer_(ram[LakituReappearTimer]), bitMFilter_(ram[BitMFilter]),
      jumpCoinMiscOffset_(ram[JumpCoinMiscOffset]), fireballThrowingTimer_(ram[FireballThrowingTimer]),
      fireballCounter_(ram[FireballCounter]), powerUpType_(ram[PowerUpType]), boundingBox_DR_YPos_(ram[BoundingBox_DR_YPos]),
      boundingBox_DR_XPos_(ram[BoundingBox_DR_XPos]), boundingBox_UL_YPos_(ram[BoundingBox_UL_YPos]),
      boundingBox_UL_XPos_(ram[BoundingBox_UL_XPos]), block_ResidualCounter_(ram[Block_ResidualCounter]),
      vineStart_Y_Position_(ram[VineStart_Y_Position]), vineHeight_(ram[VineHeight]), vineFlagOffset_(ram[VineFlagOffset]),
      whirlpool_Flag_(ram[Whirlpool_Flag]), whirlpool_Offset_(ram[Whirlpool_Offset]), cannon_Offset_(ram[Cannon_Offset]),
      misc_OffscreenBits_(ram[Misc_OffscreenBits]), block_OffscreenBits_(ram[Block_OffscreenBits]),
      bubble_OffscreenBits_(ram[Bubble_OffscreenBits]), fBall_OffscreenBits_(ram[FBall_OffscreenBits]),
      enemy_OffscreenBits_(ram[Enemy_OffscreenBits]), player_OffscreenBits_(ram[Player_OffscreenBits]),
      maximumRightSpeed_(ram[MaximumRightSpeed]), maximumLeftSpeed_(ram[MaximumLeftSpeed]), crouchingFlag_(ram[CrouchingFlag]),
      flagpoleSoundQueue_(ram[FlagpoleSoundQueue]), deathMusicLoaded_(ram[DeathMusicLoaded]), playerAnimCtrl_(ram[PlayerAnimCtrl]),
      playerAnimTimerSet_(ram[PlayerAnimTimerSet]), playerChangeSizeFlag_(ram[PlayerChangeSizeFlag]),
      verticalForceDown_(ram[VerticalForceDown]), verticalForce_(ram[VerticalForce]), jumpOrigin_Y_Position_(ram[JumpOrigin_Y_Position]),
      jumpOrigin_Y_HighPos_(ram[JumpOrigin_Y_HighPos]), diffToHaltJump_(ram[DiffToHaltJump]), player_X_MoveForce_(ram[Player_X_MoveForce]),
      swimmingFlag_(ram[SwimmingFlag]), runningSpeed_(ram[RunningSpeed]), frictionAdderLow_(ram[FrictionAdderLow]),
      frictionAdderHigh_(ram[FrictionAdderHigh]), player_XSpeedAbsolute_(ram[Player_XSpeedAbsolute]),
      playerGfxOffset_(ram[PlayerGfxOffset]), enemyFrenzyQueue_(ram[EnemyFrenzyQueue]), enemyFrenzyBuffer_(ram[EnemyFrenzyBuffer]),
      player_BoundBoxCtrl_(ram[Player_BoundBoxCtrl]), player_CollisionBits_(ram[Player_CollisionBits]),
      disableCollisionDet_(ram[DisableCollisionDet]), player_Y_MoveForce_(ram[Player_Y_MoveForce]),
      player_YMF_Dummy_(ram[Player_YMF_Dummy]), player_SprAttrib_(ram[Player_SprAttrib]), misc_Rel_YPos_(ram[Misc_Rel_YPos]),
      block_Rel_YPos_(ram[Block_Rel_YPos]), bubble_Rel_YPos_(ram[Bubble_Rel_YPos]), fireball_Rel_YPos_(ram[Fireball_Rel_YPos]),
      enemy_Rel_YPos_(ram[Enemy_Rel_YPos]), player_Rel_YPos_(ram[Player_Rel_YPos]), misc_Rel_XPos_(ram[Misc_Rel_XPos]),
      block_Rel_XPos_(ram[Block_Rel_XPos]), bubble_Rel_XPos_(ram[Bubble_Rel_XPos]), fireball_Rel_XPos_(ram[Fireball_Rel_XPos]),
      enemy_Rel_XPos_(ram[Enemy_Rel_XPos]), player_Rel_XPos_(ram[Player_Rel_XPos]), block_Y_Position_(ram[Block_Y_Position]),
      player_Y_Position_(ram[Player_Y_Position]), player_Y_HighPos_(ram[Player_Y_HighPos]), player_Y_Speed_(ram[Player_Y_Speed]),
      player_X_Position_(ram[Player_X_Position]), player_PageLoc_(ram[Player_PageLoc]), jumpspringForce_(ram[JumpspringForce]),
      jumpspringAnimCtrl_(ram[JumpspringAnimCtrl]), player_X_Speed_(ram[Player_X_Speed]), enemy_MovingDir_(ram[Enemy_MovingDir]),
      player_MovingDir_(ram[Player_MovingDir]), player_State_(ram[Player_State]), sprDataOffset_Ctrl_(ram[SprDataOffset_Ctrl]),
      player_SprDataOffset_(ram[Player_SprDataOffset]), sprShuffleAmt_(ram[SprShuffleAmt]), sprShuffleAmtOffset_(ram[SprShuffleAmtOffset]),
      warmBootValidation_(ram[WarmBootValidation]), pseudoRandomBitReg_(ram[PseudoRandomBitReg]),
      starFlagTaskControl_(ram[StarFlagTaskControl]), brickCoinTimerFlag_(ram[BrickCoinTimerFlag]),
      platform_X_Scroll_(ram[Platform_X_Scroll]), balPlatformAlignment_(ram[BalPlatformAlignment]),
      offScr_AreaNumber_(ram[OffScr_AreaNumber]), offScr_WorldNumber_(ram[OffScr_WorldNumber]),
      offScr_Hidden1UpFlag_(ram[OffScr_Hidden1UpFlag]), offScr_NumberofLives_(ram[OffScr_NumberofLives]),
      coinTallyFor1Ups_(ram[CoinTallyFor1Ups]), areaNumber_(ram[AreaNumber]), worldNumber_(ram[WorldNumber]), coinTally_(ram[CoinTally]),
      hidden1UpFlag_(ram[Hidden1UpFlag]), levelNumber_(ram[LevelNumber]), halfwayPage_(ram[HalfwayPage]),
      numberofLives_(ram[NumberofLives]), playerStatus_(ram[PlayerStatus]), playerSize_(ram[PlayerSize]),
      currentPlayer_(ram[CurrentPlayer]), continueWorld_(ram[ContinueWorld]), worldSelectEnableFlag_(ram[WorldSelectEnableFlag]),
      worldSelectNumber_(ram[WorldSelectNumber]), secondaryHardMode_(ram[SecondaryHardMode]), primaryHardMode_(ram[PrimaryHardMode]),
      gameTimerExpiredFlag_(ram[GameTimerExpiredFlag]), fetchNewGameTimerFlag_(ram[FetchNewGameTimerFlag]),
      multiLoopPassCntr_(ram[MultiLoopPassCntr]), multiLoopCorrectCntr_(ram[MultiLoopCorrectCntr]), changeAreaTimer_(ram[ChangeAreaTimer]),
      warpZoneControl_(ram[WarpZoneControl]), numberOfPlayers_(ram[NumberOfPlayers]), entrancePage_(ram[EntrancePage]),
      altEntranceControl_(ram[AltEntranceControl]), gameTimerSetting_(ram[GameTimerSetting]), playerEntranceCtrl_(ram[PlayerEntranceCtrl]),
      areaPointer_(ram[AreaPointer]), areaAddrsLOffset_(ram[AreaAddrsLOffset]), areaType_(ram[AreaType]),
      backgroundColorCtrl_(ram[BackgroundColorCtrl]), cloudTypeOverride_(ram[CloudTypeOverride]),
      backgroundScenery_(ram[BackgroundScenery]), foregroundScenery_(ram[ForegroundScenery]), areaStyle_(ram[AreaStyle]),
      terrainControl_(ram[TerrainControl]), colorRotateOffset_(ram[ColorRotateOffset]), disableIntermediate_(ram[DisableIntermediate]),
      disableScreenFlag_(ram[DisableScreenFlag]), sprite0HitDetectFlag_(ram[Sprite0HitDetectFlag]),
      vRAM_Buffer_AddrCtrl_(ram[VRAM_Buffer_AddrCtrl]), vRAM_Buffer2_Offset_(ram[VRAM_Buffer2_Offset]), vRAM_Buffer1_(ram[VRAM_Buffer1]),
      vRAM_Buffer1_Offset_(ram[VRAM_Buffer1_Offset]), stompChainCounter_(ram[StompChainCounter]),
      flagpoleCollisionYPos_(ram[FlagpoleCollisionYPos]), flagpoleScore_(ram[FlagpoleScore]),
      flagpoleFNum_YMFDummy_(ram[FlagpoleFNum_YMFDummy]), flagpoleFNum_Y_Pos_(ram[FlagpoleFNum_Y_Pos]),
      verticalFlipFlag_(ram[VerticalFlipFlag]), gameTimerDisplay_(ram[GameTimerDisplay]), loopCommand_(ram[LoopCommand]),
      currentNTAddr_High_(ram[CurrentNTAddr_High]), currentNTAddr_Low_(ram[CurrentNTAddr_Low]),
      blockBufferColumnPos_(ram[BlockBufferColumnPos]), metatileBuffer_(ram[MetatileBuffer]), enemyObjectPageSel_(ram[EnemyObjectPageSel]),
      enemyObjectPageLoc_(ram[EnemyObjectPageLoc]), enemyDataOffset_(ram[EnemyDataOffset]), areaObjectHeight_(ram[AreaObjectHeight]),
      staircaseControl_(ram[StaircaseControl]), areaObjectLength_(ram[AreaObjectLength]), areaDataOffset_(ram[AreaDataOffset]),
      areaObjectPageSel_(ram[AreaObjectPageSel]), areaObjectPageLoc_(ram[AreaObjectPageLoc]),
      behindAreaParserFlag_(ram[BehindAreaParserFlag]), backloadingFlag_(ram[BackloadingFlag]), currentColumnPos_(ram[CurrentColumnPos]),
      currentPageLoc_(ram[CurrentPageLoc]), columnSets_(ram[ColumnSets]), areaParserTaskNum_(ram[AreaParserTaskNum]),
      enemyDataHigh_(ram[EnemyDataHigh]), enemyDataLow_(ram[EnemyDataLow]), areaDataHigh_(ram[AreaDataHigh]),
      areaDataLow_(ram[AreaDataLow]), scrollAmount_(ram[ScrollAmount]), player_Pos_ForScroll_(ram[Player_Pos_ForScroll]),
      player_X_Scroll_(ram[Player_X_Scroll]), scrollThirtyTwo_(ram[ScrollThirtyTwo]), scrollLock_(ram[ScrollLock]),
      verticalScroll_(ram[VerticalScroll]), horizontalScroll_(ram[HorizontalScroll]), secondaryMsgCounter_(ram[SecondaryMsgCounter]),
      primaryMsgCounter_(ram[PrimaryMsgCounter]), scrollFractional_(ram[ScrollFractional]), victoryWalkControl_(ram[VictoryWalkControl]),
      destinationPageLoc_(ram[DestinationPageLoc]), playerFacingDir_(ram[PlayerFacingDir]), screenRight_X_Pos_(ram[ScreenRight_X_Pos]),
      screenLeft_X_Pos_(ram[ScreenLeft_X_Pos]), screenRight_PageLoc_(ram[ScreenRight_PageLoc]),
      screenLeft_PageLoc_(ram[ScreenLeft_PageLoc]), demoTimer_(ram[DemoTimer]), worldEndTimer_(ram[WorldEndTimer]),
      screenTimer_(ram[ScreenTimer]), starInvincibleTimer_(ram[StarInvincibleTimer]), injuryTimer_(ram[InjuryTimer]),
      brickCoinTimer_(ram[BrickCoinTimer]), scrollIntervalTimer_(ram[ScrollIntervalTimer]), airBubbleTimer_(ram[AirBubbleTimer]),
      stompTimer_(ram[StompTimer]), bowserFireBreathTimer_(ram[BowserFireBreathTimer]), frenzyEnemyTimer_(ram[FrenzyEnemyTimer]),
      climbSideTimer_(ram[ClimbSideTimer]), gameTimerCtrlTimer_(ram[GameTimerCtrlTimer]), jumpspringTimer_(ram[JumpspringTimer]),
      sideCollisionTimer_(ram[SideCollisionTimer]), blockBounceTimer_(ram[BlockBounceTimer]), runningTimer_(ram[RunningTimer]),
      jumpSwimTimer_(ram[JumpSwimTimer]), playerAnimTimer_(ram[PlayerAnimTimer]), selectTimer_(ram[SelectTimer]),
      intervalTimerControl_(ram[IntervalTimerControl]), timerControl_(ram[TimerControl]), demoActionTimer_(ram[DemoActionTimer]),
      demoAction_(ram[DemoAction]), gamePauseTimer_(ram[GamePauseTimer]), gamePauseStatus_(ram[GamePauseStatus]),
      screenRoutineTask_(ram[ScreenRoutineTask]), operMode_Task_(ram[OperMode_Task]), operMode_(ram[OperMode]),
      mirror_PPU_CTRL_REG2_(ram[Mirror_PPU_CTRL_REG2]), mirror_PPU_CTRL_REG1_(ram[Mirror_PPU_CTRL_REG1]),
      savedJoypad2Bits_(ram[SavedJoypad2Bits]), savedJoypad1Bits_(ram[SavedJoypad1Bits]), savedJoypadBits_(ram[SavedJoypadBits]),
      frameCounter_(ram[FrameCounter]), objectOffset_(ram[ObjectOffset]), gameEngineSubroutine_(ram[GameEngineSubroutine]),
      left_Right_Buttons_(ram[Left_Right_Buttons]), up_Down_Buttons_(ram[Up_Down_Buttons]), previousA_B_Buttons_(ram[PreviousA_B_Buttons]),
      a_B_Buttons_(ram[A_B_Buttons]), joypadOverride_(ram[JoypadOverride]), timerControl(timerControl_), savedState(nullptr)
{
    apu = new APU();
    ppu = new PPU(*this);
    controller1 = new Controller();
    controller2 = new Controller();

    // Anything the game reads from the address space that loadConstantData()
    // does not write is read from here as-is, so start from a known value rather
    // than from whatever happens to be on the heap.
    //
    memset(dataStorage, 0, sizeof(dataStorage));

    // CHR Location in ROM: Header (16 bytes) + 2 PRG pages (16k each)
    chr = (romImage + 16 + (static_cast<ptrdiff_t>(16384) * 2));
}

SMBEngine::~SMBEngine()
{
    delete apu;
    delete ppu;
    delete controller1;
    delete controller2;
    delete savedState;
}

void SMBEngine::saveState()
{
    if (savedState == nullptr)
    {
        savedState = new State();
    }

    memcpy(savedState->ram, ram, sizeof(ram));
    savedState->musicData = musicData;
    savedState->ppu = ppu->saveState();
}

bool SMBEngine::loadState()
{
    if (savedState == nullptr)
    {
        return false;
    }

    memcpy(ram, savedState->ram, sizeof(ram));
    musicData = savedState->musicData;
    ppu->loadState(savedState->ppu);

    return true;
}

bool SMBEngine::hasState() const { return savedState != nullptr; }

void SMBEngine::audioCallback(uint8_t *stream, int length) { apu->output(stream, length); }

Controller &SMBEngine::getController1() { return *controller1; }

Controller &SMBEngine::getController2() { return *controller2; }

const uint8_t *SMBEngine::getRam() const { return ram; }

bool SMBEngine::isLagFrame() const
{
    // The NMI handler only overruns a frame while it is drawing the screen for a
    // newly loaded area, which is the first of the screen routines (InitScreen).
    //
    // Checked against FCEUX over a full playthrough of the game: of the 53 lag
    // frames in it, this identifies 51, and never mistakes a normal frame for a
    // lag frame. It misses the frames that lag because an unusual number of
    // enemies are on screen, which only cycle-accurate timing could predict.
    //
    return screenRoutineTask_ == 0;
}

void SMBEngine::render(uint32_t *buffer) { ppu->render(buffer); }

void SMBEngine::renderNametables(uint32_t *buffer, int &scrollX, int &scrollY)
{
    ppu->renderNametables(buffer);
    ppu->getScroll(scrollX, scrollY);
}

void SMBEngine::reset()
{
    // Run the decompiled code for initialization
    code(0);
}

void SMBEngine::update()
{
    // Run the decompiled code for the NMI handler
    code(1);

    // Update the APU
    if (audioEnabled)
    {
        apu->stepFrame();
    }
}

//---------------------------------------------------------------------
// Private methods

uint8_t *SMBEngine::getCHR() { return chr; }

uint8_t &SMBEngine::getMemory(uint16_t address)
{
    // Constant data
    if (address >= DATA_STORAGE_OFFSET)
    {
        return dataStorage[address - DATA_STORAGE_OFFSET];
    }

    // RAM and Mirrors
    return ram[address & 0x7ff];
}

uint16_t SMBEngine::getMemoryWord(uint8_t address) { return (uint16_t)readData(address) + ((uint16_t)(readData(address + 1)) << 8); }

uint8_t SMBEngine::readData(uint16_t address)
{
    // Constant data
    if (address >= DATA_STORAGE_OFFSET)
    {
        return dataStorage[address - DATA_STORAGE_OFFSET];
    }
    // RAM and Mirrors
    if (address < 0x2000)
    {
        return ram[address & 0x7ff];
    }
    // PPU Registers and Mirrors
    if (address < 0x4000)
    {
        return ppu->readRegister(0x2000 + (address & 0x7));
    }
    // IO registers
    if (address < 0x4020)
    {
        switch (address)
        {
        case 0x4016:
            return controller1->readByte();
        case 0x4017:
            return controller2->readByte();
        default:
            return 0;
        }
    }

    return 0;
}

void SMBEngine::writeData(uint16_t address, const uint8_t *data, size_t length)
{
    address -= DATA_STORAGE_OFFSET;

    memcpy(dataStorage + (std::ptrdiff_t)address, data, length);
}
