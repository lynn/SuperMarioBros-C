#ifndef SMBENGINE_HPP
#define SMBENGINE_HPP

#include <cstdint>
#include <cstddef>
#include <utility>

#include "SMBDataPointers.hpp"

class APU;
class Controller;
class PPU;

/**
 * Engine that runs Super Mario Bros.
 * Handles emulation of various NES subsystems for compatibility and accuracy.
 */
class SMBEngine
{
    friend class PPU;
public:
    /**
     * Size of the NES RAM, in bytes.
     */
    static const std::size_t RAM_SIZE = 0x800;

    /**
     * Construct a new SMBEngine instance.
     *
     * @param romImage     the data from the Super Mario Bros. ROM image.
     * @param enableAudio  whether to run the APU. It accumulates samples into a
     *                     fixed-size buffer that something has to drain, so this
     *                     must be false when nothing is playing them back.
     */
    SMBEngine(uint8_t* romImage, bool enableAudio = true);

    ~SMBEngine();

    /**
     * Callback for handling audio buffering.
     */
    void audioCallback(uint8_t* stream, int length);

    /**
     * Get player 1's controller.
     */
    Controller& getController1();

    /**
     * Get player 2's controller.
     */
    Controller& getController2();

    /**
     * Get the NES RAM, RAM_SIZE bytes of it.
     */
    const uint8_t* getRam() const;

    /**
     * Get whether the frame that just ran would have been a "lag frame" on a
     * real console.
     *
     * The game runs entirely inside the NMI handler, and the handler disables
     * NMIs while it runs. When a frame's work takes longer than a frame to
     * finish, the console misses the next NMI: the game does not advance that
     * frame, and it never reads the controller for it. The engine runs the
     * handler as a plain function call and has no notion of cycles, so it cannot
     * time it. Instead it uses the fact that the handler only overruns while it
     * is initializing the screen for a newly loaded area.
     *
     * This only matters for movie playback, which records one frame of input per
     * frame of the console rather than per frame of game logic. See Fm2Movie.
     */
    bool isLagFrame() const;

    /**
     * Render the screen to a buffer.
     *
     * @param buffer a 256x240 32-bit color buffer for storing the rendering.
     */
    void render(uint32_t* buffer);

    /**
     * Reset the game engine to power-on state.
     */
    void reset();

    /**
     * Update the game engine by one frame.
     */
    void update();

private:
    bool audioEnabled;           /**< Whether the APU is run at all. */

    // NES Emulation subsystems:
    APU* apu;
    PPU* ppu;
    Controller* controller1;
    Controller* controller2;

    // Fields for NES CPU emulation:
    uint8_t a;                   /**< Accumulator register. */
    uint8_t x;                   /**< X index register. */
    uint8_t y;                   /**< Y index register. */
    uint8_t s;                   /**< Stack index register. */
    uint8_t dataStorage[0x8000]; /**< 32kb of storage for constant data. */
    uint8_t ram[RAM_SIZE];       /**< 2kb of RAM. */
    uint8_t* chr;                /**< Pointer to CHR data from the ROM. */

    // Pointers to constant data used in the decompiled code
    //
    SMBDataPointers dataPointers;

    /**
     * Run the decompiled code for the game.
     *
     * See SMB.cpp for implementation.
     *
     * @param mode the mode to run. 0 runs initialization routines, 1 runs the logic for frames.
     */
    void code(int mode);

    /**
     * The routines of the game that are routines in C too: a call is the only
     * way in and a return the only way out. The rest of them are still labels
     * inside code(), which is where these were lifted from.
     *
     * See SMB.cpp for implementations.
     */
    void AddToScore();
    void AlterAreaAttributes(uint8_t areaObjBufferOffset);
    uint8_t AlternateLengthHandler(uint8_t rawByte);
    void AnimationControl();
    void AreaFrenzy();
    void AreaParserCore();
    void AreaParserTaskControl();
    void AreaParserTaskHandler();
    void AreaParserTasks(uint8_t taskNum);
    void AreaStyleObject(uint8_t areaObjBufferOffset);
    void TreeLedge(uint8_t areaObjBufferOffset);
    void MushroomLedge(uint8_t areaObjBufferOffset);
    void AutoControlPlayer();
    void AxeObj();
    void BBChk_E(uint8_t coordSelector, uint8_t objectOffset, uint8_t cornerIdx);
    void BalancePlatRope(uint8_t areaObjBufferOffset);
    void BalancePlatform();
    void BlockBufferChk_Enemy();
    void BlockBufferChk_FBall();
    uint8_t BlockBufferColli_Feet(uint8_t adderBaseOffset);
    uint8_t BlockBufferColli_Head(uint8_t adderOffset);
    uint8_t BlockBufferColli_Side(uint8_t adderOffset);
    uint8_t BlockBufferCollision(uint8_t coordSelector, uint8_t objectOffset, uint8_t cornerIdx);
    std::pair<bool, uint8_t> BlockBumpedChk(uint8_t metatile);
    void BlockObjMT_Updater();
    void BlockObjectsCore();
    uint8_t BoundingBoxCore(uint8_t boundBoxCtrlIdx, uint8_t relPosIdx);
    void BowserGfxHandler();
    void BranchToDecLength1();
    void BrickShatter();
    void BrickWithCoins(uint8_t areaObjBufferOffset);
    void BrickWithItem(uint8_t areaObjBufferOffset);
    void BridgeCollapse();
    void Bridge_High(uint8_t areaObjBufferOffset);
    void Bridge_Low(uint8_t areaObjBufferOffset);
    void Bridge_Middle(uint8_t areaObjBufferOffset);
    void BubbleCheck();
    void BulletBillCannon(uint8_t areaObjBufferOffset);
    void BulletBillHandler();
    void BumpBlock();
    void CGrab_TTickRegL(uint8_t length, uint8_t ctrlByte);
    void CastleBridgeObj(uint8_t areaObjBufferOffset);
    void CastleObject(uint8_t areaObjBufferOffset);
    void ChainObj();
    void CheckDefeatedState();
    static bool CheckForClimbMTiles(uint8_t metatile);
    bool CheckForCoinMTiles(uint8_t metatile);
    void CheckForHammerBro();
    static bool CheckForSolidMTiles(uint8_t metatile);
    bool CheckPlayerVertical();
    void CheckRightScreenBBox(uint8_t objectOffset, uint8_t boundBoxIdx);
    uint8_t CheckTopOfBlock();
    void CheckpointEnemyID();
    void ChgAreaMode();
    void ChgAreaPipe();
    void ChkContinue(uint8_t joypadBits);
    void ChkFireB();
    void ChkForBump_HammerBroJ();
    void ChkForDemoteKoopa(uint8_t comparisonValue, uint8_t enemyOffset);
    void ChkForLandJumpSpring(uint8_t metatile);
    static bool ChkForNonSolids(uint8_t metatile);
    void ChkForPlayerAttrib();
    void ChkForPlayerC_LargeP();
    static bool ChkInvisibleMTiles(uint8_t metatile);
    static bool ChkJumpspringMetatiles(uint8_t metatile);
    void ChkLak();
    void ChkLeftCo();
    bool ChkLrgObjFixedLength(uint8_t areaObjBufferOffset, uint8_t lengthIfUnset);
    bool ChkLrgObjLength(uint8_t areaObjBufferOffset, uint8_t& outLength);
    void ChkNoEn();
    void ChkPOffscr();
    void ChkSmallPlatCollision();
    void ChkToStunEnemies(uint8_t comparisonValue, uint8_t enemyOffset);
    void ChkUnderEnemy();
    void ChkYPCollision();
    void ClearBuffersDrawIcon();
    void ClimbingSub();
    void CoinBlock(uint8_t blockOffset);
    void ColObj(uint8_t tile, uint8_t startCol);
    void ColorRotation();
    void ColumnOfBricks(uint8_t areaObjBufferOffset);
    void ColumnOfSolidBlocks(uint8_t areaObjBufferOffset);
    void CommonPlatCode();
    void CommonSmallLift();
    void ContinueBlast();
    void ContinueBowserFall();
    void ContinueBumpThrow();
    void ContinueCGrabTTick();
    void ContinueExtraLife();
    void ContinueBowserFlame();
    void ContinueBrickShatter();
    void ContinueGrowItems();
    void ContinuePowerUpGrab();
    void ContinueGame();
    void ContinuePipeDownInj();
    void ContinueSmackEnemy();
    void ContinueSndJump();
    void ContinueSwimStomp();
    void CyclePlayerPalette();
    void DecJpFPS();
    void DecodeAreaData(uint8_t areaObjBufferOffset, uint8_t areaDataOffset);
    void DecrementSfx1Length();
    void DecrementSfx2Length();
    void DecrementSfx3Length();
    bool DemoEngine();
    void DestroyBlockMetatile();
    void DigitsMathRoutine(uint8_t digitOffset);
    void DisplayIntermediate();
    void DisplayTimeUp();
    uint8_t DividePDiff(uint8_t value, uint8_t flag, uint8_t currentOffset);
    void DmpJpFPS(uint8_t ctrlByte, uint8_t sweepByte);
    void DoEnemySideCheck();
    void DoNothing1();
    static void DoNothing2();
    void DoOtherPlatform();
    void DonePlayerTask();
    void DrawBlock();
    void DrawBrickChunks();
    void DrawBubble();
    void DrawEnemyObjRow();
    void DrawEnemyObject();
    void DrawExplosion_Fireball();
    void DrawExplosion_Fireworks(uint8_t frameSelector, uint8_t spriteDataBase);
    void DrawFireball();
    void DrawFirebar(uint8_t oamSlot);
    void DrawFirebar_Collision();
    void DrawHammer();
    void DrawLargePlatform();
    void DrawMushroomIcon();
    std::pair<uint8_t, uint8_t> DrawOneSpriteRow(uint8_t tileNumber, uint8_t spritePairIdx, uint8_t oamSlot);
    void DrawPlayerLoop();
    void DrawPlayer_Intermediate();
    void DrawPowerUp();
    void DrawQBlk(uint8_t brickQBlockIndex, uint8_t areaObjBufferOffset);
    void DrawRope(uint8_t startCol, uint8_t numRows);
    void DrawRow(uint8_t tile);
    void DrawSmallPlatform();
    std::pair<uint8_t, uint8_t> DrawSpriteObject(uint8_t spritePairIdx, uint8_t oamSlot);
    void DrawStarFlag();
    void DrawTitleScreen();
    void DrawVine();
    void DropPlatform();
    void DumpFourSpr(uint8_t value, uint8_t baseOffset);
    void DumpSixSpr(uint8_t value, uint8_t baseOffset);
    void DumpThreeSpr(uint8_t value, uint8_t baseOffset);
    void DumpTwoSpr(uint8_t value, uint8_t baseOffset);
    uint8_t Dump_Freq_Regs(uint8_t freqIndex, uint8_t channelOffset);
    void Dump_Sq2_Regs(uint8_t ctrlByte, uint8_t sweepByte);
    void Dump_Squ1_Regs(uint8_t ctrlByte, uint8_t sweepByte);
    void DuplicateEnemyObj();
    void EmptyBlock(uint8_t areaObjBufferOffset);
    void EmptySfx2Buffer();
    void EndAreaPoints();
    void EndFrenzy();
    void EndlessRope();
    void EnemiesAndLoopsCore();
    void EnemiesCollision();
    uint8_t EnemyFacePlayer(uint8_t enemyOffset);
    void EnemyGfxHandler();
    void EnemyJump();
    void EnemyLanding();
    void EnemyMovementSubs();
    void EnemySmackScore(uint8_t pointsControl, uint8_t enemyOffset);
    void EnemyToBGCollisionDet();
    void EnemyTurnAround(uint8_t enemyOffset);
    void EnterSidePipe();
    void Entrance_GameTimerSetup();
    void ErACM();
    void EraseEnemyObject(uint8_t enemyOffset);
    void ExInjColRoutines();
    void ExecGameLoopback();
    void ExitPipe(uint8_t areaObjBufferOffset);
    void ExtraLifeMushBlock(uint8_t blockOffset);
    void FBallB();
    void FPS2nd(uint8_t ctrlByte);
    void FallE();
    bool FindEmptyEnemySlot(uint8_t& outSlot);
    std::pair<bool, uint8_t> FindEmptyMiscSlot();
    void FindPlayerAction();
    void FinishFlame();
    void FireballBGCollision();
    void FireballEnemyCollision();
    void FireballObjCore();
    void FirebarCollision();
    void FirebarSpin();
    void FlagBalls_Residual(uint8_t areaObjBufferOffset);
    void FlagpoleGfxHandler();
    void FlagpoleObject();
    void FlagpoleRoutine();
    void FlagpoleSlide();
    void FloateyNumbersRoutine();
    void ForceInjury(uint8_t mustBeZero);
    void Fthrow(uint8_t length, uint8_t sweepByte);
    void GameCoreRoutine();
    void GameMenuRoutine();
    void GameMode();
    void GameOverMode();
    void GameRoutines();
    void GameTextLoop();
    void GetAlternatePalette1();
    void GetAreaDataAddrs();
    void GetAreaMusic();
    uint8_t GetAreaObjXPosition();
    uint8_t GetAreaObjYPosition();
    uint8_t GetAreaObjectID();
    void GetAreaPalette();
    void GetAreaType();
    void GetBackgroundColor();
    void GetBlockBufferAddr(uint8_t column);
    void GetBlockOffscreenBits();
    void GetBubbleOffscreenBits();
    void GetCurrentAnimOffset();
    void GetEnemyBoundBox(uint8_t enemyOffset);
    std::pair<uint8_t, uint8_t> GetEnemyBoundBoxOfs();
    std::pair<uint8_t, uint8_t> GetEnemyBoundBoxOfsArg(uint8_t enemyOffset);
    void GetEnemyOffscreenBits();
    void GetFireballBoundBox();
    void GetFireballOffscreenBits();
    void GetFirebarPosition();
    void GetGfxOffsetAdder();
    uint8_t GetLrgObjAttrib(uint8_t areaObjBufferOffset);
    static uint8_t GetMTileAttrib(uint8_t metatile);
    void GetMaskedOffScrBits(uint8_t enemyOffset, uint8_t defaultBitmask);
    void GetMiscBoundBox();
    void GetMiscOffscreenBits();
    void GetObjRelativePosition(uint8_t objectOffset, uint8_t relPosIdx);
    void GetOffScreenBitsSet(uint8_t objectOffset, uint8_t offscrArrayOffset);
    void GetOffsetFromAnimCtrl();
    uint8_t GetPipeHeight(uint8_t areaObjBufferOffset);
    void GetPlayerAnimSpeed();
    void GetPlayerColors();
    void GetPlayerOffscreenBits();
    void GetProperObjOffset();
    void GetRow(uint8_t tile, uint8_t areaObjBufferOffset);
    void GetRow2(uint8_t tile, uint8_t areaObjBufferOffset);
    void GetSBNybbles();
    uint8_t GetScreenPosition();
    uint8_t GetXOffscreenBits(uint8_t objectOffset);
    void GiveOneCoin();
    void GoContinue(uint8_t worldNumber);
    void GrowItemRegs(uint8_t length);
    void HandleChangeSize();
    void HandleCoinMetatile();
    void HandleEnemyFBallCol();
    void HandleGroupEnemies();
    void HandlePipeEntry();
    void Hidden1UpBlock(uint8_t areaObjBufferOffset);
    void Hole_Empty(uint8_t areaObjBufferOffset);
    void Hole_Water(uint8_t areaObjBufferOffset);
    void HurtBowser();
    void ImpedePlayerMove();
    void ImposeFriction(uint8_t leftRightButtons);
    void ImposeGravity(uint8_t movementMode, uint8_t objectOffset);
    void ImposeGravityBlock();
    void ImposeGravitySprObj(uint8_t maxSpeed, uint8_t objectOffset);
    void Inc2B();
    void Inc3B();
    void IncAreaObjOffset();
    void IncSubtask();
    void IncrementColumnPos();
    void InitBalPlatform();
    void InitBlock_XY_Pos(uint8_t blockOffset);
    void InitBloober();
    void InitBowser();
    void InitBowserFlame();
    void InitBulletBill();
    void InitCheepCheep();
    void InitDropPlatform();
    void InitEnemyObject();
    void InitFireworks();
    void InitFlyingCheepCheep();
    void InitGoomba();
    void InitHammerBro();
    void InitHoriPlatform();
    void InitHorizFlySwimEnemy();
    void InitJumpGPTroopa();
    void InitLakitu();
    void InitLongFirebar();
    void InitNTLoop(uint8_t tile, uint8_t xCount, uint8_t yCount);
    void InitNormalEnemy();
    void InitPiranhaPlant(uint8_t enemyOffset);
    void InitPodoboo();
    void InitRedKoopa();
    void InitRedPTroopa();
    void InitRetainerObj();
    void InitScreen();
    void InitScroll(uint8_t value);
    void InitShortFirebar();
    void InitVStf(uint8_t enemyOffset);
    void InitVertPlatform();
    void InitializeArea();
    void InitializeGame();
    void InitializeMemory(uint8_t startOffset);
    void InitializeNameTables();
    void InjurePlayer();
    void IntroPipe(uint8_t areaObjBufferOffset);
    void JCoinC(uint8_t blockOffset, uint8_t miscSlot);
    void JCoinGfxHandler();
    void JumpEngine();
    void JumpRegContents(uint8_t freqIndex);
    void Jumpspring(uint8_t areaObjBufferOffset);
    void JumpspringHandler();
    void KillAllEnemies();
    void KillEnemies(uint8_t enemyId);
    void KillEnemyAboveBlock();
    void LInj(uint8_t enemyOffset);
    void LakituAndSpinyHandler();
    void LargeLiftBBox();
    void LargeLiftDown();
    void LargeLiftUp();
    void LargePlatformBoundBox();
    void LargePlatformCollision();
    void LargePlatformSubroutines();
    void LoadAreaPointer();
    uint8_t LoadControlRegs();
    uint8_t LoadEnvelopeData(uint8_t offset);
    void LoadSqu2Regs(uint8_t freqIndex, uint8_t ctrlByte, uint8_t sweepByte);
    void MiscObjectsCore();
    void MiscSqu2MusicTasks();
    void MoveAllSpritesOffscreen();
    void MoveBloober();
    void MoveBoundBoxOffscreen(uint8_t enemyOffset);
    void MoveBubl();
    void MoveBulletBill();
    uint8_t MoveColOffscreen(uint8_t yPosOffset);
    void MoveD_Bowser();
    void MoveD_EnemyVertically(uint8_t enemyOffset);
    void MoveDropPlatform();
    void MoveESprColOffscreen(uint8_t rowSelectorBase, uint8_t enemyOffset);
    void MoveESprRowOffscreen(uint8_t rowSelectorBase, uint8_t enemyOffset);
    uint8_t MoveEnemyHorizontally(uint8_t enemyOffset);
    void MoveEnemySlowVert();
    void MoveFallingPlatform();
    void MoveFlyGreenPTroopa();
    void MoveFlyingCheepCheep();
    void MoveHammerBroXDir();
    void MoveJ_EnemyVertically();
    void MoveJumpingEnemy();
    void MoveLakitu();
    void MoveLargeLiftPlat();
    void MoveLiftPlatforms();
    void MoveNormalEnemy();
    uint8_t MoveObjectHorizontally(uint8_t objectOffset);
    void MovePiranhaPlant();
    void MovePlatformDown();
    void MovePlatformUp();
    uint8_t MovePlayerHorizontally();
    void MovePlayerVertically();
    void MovePlayerYAxis();
    void MovePodoboo();
    void MoveRedPTroopa();
    void MoveRedPTroopaDown();
    void MoveSixSpritesOffscreen(uint8_t baseOffset);
    void MoveSmallPlatform();
    void MoveSpritesOffscreen();
    void MoveSwimmingCheepCheep();
    void MoveVOffset(uint8_t vramOffset);
    void MoveWithXMCntrs();
    void MushFlowerBlock(uint8_t blockOffset);
    void MusicHandler();
    void NextArea();
    void NoBump();
    void NoiseSfxHandler();
    void NonMaskableInterrupt();
    void NormObj(uint8_t objectId, uint8_t areaObjBufferOffset);
    void NotMoveEnemySlowVert();
    void NullJoypad();
    void OffscreenBoundsCheck(uint8_t enemyOffset);
    void OnGroundStateSub();
    void OperModeExecutionTree();
    void OutputInter(uint8_t text_number);
    void OutputNumbers(uint8_t nybbleIdx);
    void PauseRoutine();
    void PlatLiftDown();
    void PlatLiftUp();
    void PlayBeat(uint8_t noiseCtrl, uint8_t noiseLow, uint8_t noiseHigh);
    void PlayBrickShatter();
    void PlayNoiseSfx(uint8_t noiseEnv, uint8_t noiseFreq);
    uint8_t PlaySqu1Sfx(uint8_t freqIndex, uint8_t ctrlByte, uint8_t sweepByte);
    uint8_t PlaySqu2Sfx(uint8_t freqIndex, uint8_t ctrlByte, uint8_t sweepByte);
    void PlayerBGCollision();
    bool PlayerCollisionCore(uint8_t boundBoxIdx);
    void PlayerCtrlRoutine();
    void PlayerDeath();
    void PlayerEndLevel();
    void PlayerEndWorld();
    void PlayerEnemyCollision(uint8_t enemyOffset);
    std::pair<bool, uint8_t> PlayerEnemyDiff(uint8_t enemyOffset);
    void PlayerEntrance();
    void PlayerFireFlower();
    void PlayerGfxHandler();
    void PlayerGfxProcessing();
    void PlayerHammerCollision();
    void PlayerHeadCollision(uint8_t collidedMetatile);
    void PlayerLakituDiff();
    void PlayerLoseLife();
    void PlayerMovementSubs();
    void PlayerOffscreenChk();
    void PlayerPhysicsSub();
    void PlayerVictoryWalk();
    void PosPlatform();
    void PositionPlayerOnHPlat();
    void PositionPlayerOnS_Plat();
    void PositionPlayerOnVPlat();
    void PowerUpObjHandler();
    void PrimaryGameSetup();
    void PrintStatusBarNumbers(uint8_t playerOffset);
    void PrintVictoryMessages();
    void ProcBowserFlame();
    void ProcEnemyCollisions();
    void ProcFireball_Bubble();
    void ProcFirebar();
    void ProcHammerBro();
    void ProcHammerObj();
    void ProcLPlatCollisions();
    void ProcLoopCommand();
    void ProcMoveRedPTroopa();
    void ProcSwimmingB(bool blooberCarry);
    void ProcessAreaData();
    void ProcessBowserHalf();
    void ProcessCannons();
    uint8_t ProcessLengthData(uint8_t lengthCode);
    void ProcessPlayerAction();
    void ProcessWhirlpools();
    void PulleyRopeObject(uint8_t areaObjBufferOffset);
    void PutAtRightExtent();
    void PutBlockMetatile(uint8_t metatileGroupSelector, uint8_t controlBit, uint8_t vramOffset);
    void PutPlayerOnVine();
    void PwrUpJmp();
    void QuestionBlock(uint8_t areaObjBufferOffset);
    void QuestionBlockRow_High(uint8_t areaObjBufferOffset);
    void QuestionBlockRow_Low(uint8_t areaObjBufferOffset);
    void RXSpd(uint8_t enemyOffset);
    void ReadJoypads();
    void ReadPortBits(uint8_t port);
    void RedPTroopaGrav();
    void RelWOfs(uint8_t objectOffset, uint8_t relPosIdx);
    void RelativeBlockPosition();
    void RelativeBubblePosition();
    void RelativeEnemyPosition();
    void RelativeFireballPosition();
    void RelativeMiscPosition();
    void RelativePlayerPosition();
    void RemBridge(uint8_t metatileGroupOfs4, uint8_t vramOffset);
    void RemoveCoin_Axe();
    void RenderAreaGraphics();
    void RenderAttributeTables();
    void RenderPlayerSub();
    bool RenderSidewaysPipe(uint8_t areaObjBufferOffset, uint8_t verticalLength, uint8_t& outPipeDataIndex);
    uint8_t RenderUnderPart(uint8_t tile, uint8_t startCol, uint8_t numRows);
    void ReplaceBlockMetatile();
    void ResJmpM();
    void ResetPalStar();
    void ResetScreenTimer();
    void ResetSpritesAndScreenTimer();
    void ResetTitle();
    void ResidualGravityCode();
    void ResidualMiscObjectCode();
    void RightPlatform();
    void RowOfBricks(uint8_t areaObjBufferOffset);
    void RowOfCoins(uint8_t areaObjBufferOffset);
    void RowOfSolidBlocks(uint8_t areaObjBufferOffset);
    void RunBowser();
    void RunBowserFlame();
    void RunDemo();
    void RunFirebarObj();
    void RunFireworks();
    void RunGameOver();
    void RunGameTimer();
    void RunLargePlatform();
    void RunNormalEnemies();
    uint8_t RunOffscrBitsSubs(uint8_t objectOffset);
    void RunPUSubs();
    void RunRetainerObj();
    void RunSmallPlatform();
    void RunStarFlagObj();
    void SPBBox();
    void ScreenRoutines();
    void ScrollHandler();
    void ScrollLockObject();
    void ScrollLockObject_Warp();
    void ScrollScreen(uint8_t scrollAmount);
    void SecondaryGameSetup();
    void SetBBox();
    void SetBBox2(uint8_t boundBoxCtrl, uint8_t enemyOffset);
    void SetESpd();
    void SetEntr();
    void SetFlameTimer();
    uint8_t SetFreq_Squ1(uint8_t freqIndex);
    uint8_t SetFreq_Squ2(uint8_t freqIndex);
    uint8_t SetFreq_Tri(uint8_t freqIndex);
    void SetHJ();
    void SetHiMax();
    void SetKRout(uint8_t subroutineNum);
    void SetOffscrBitsOffset(uint8_t addend, uint8_t baseObjectOffset, uint8_t offscrArrayOffset);
    void SetPRout(uint8_t subroutineNum, uint8_t newPlayerState);
    void SetShim();
    void SetStun(uint8_t enemyOffset);
    void SetVRAMAddr_A(uint8_t addrCtrl);
    void SetVRAMCtrl();
    void SetVRAMOffset(uint8_t newOffset);
    void SetXMoveAmt(uint8_t maxSpeed, uint8_t enemyOffset, uint8_t downwardMoveAmt);
    void SetupBubble();
    void SetupEOffsetFBBox(uint8_t objectOffset);
    void SetupFloateyNumber(uint8_t pointsControl, uint8_t enemyOffset);
    void SetupGameOver();
    void SetupIntermediate();
    void SetupJumpCoin(uint8_t blockOffset);
    void SetupLakitu();
    void SetupPlatformRope();
    void SetupPowerUp(uint8_t blockOffset);
    void SetupVictoryMode();
    void Setup_Vine(uint8_t enemyOffset, uint8_t blockOffset);
    void ShellOrBlockDefeat(uint8_t enemyOffset);
    void SideExitPipeEntry();
    void SixSpriteStacker();
    void Skip_0(uint8_t yOffset);
    void Skip_1(uint8_t startRow, uint8_t areaObjBufferOffset);
    void Skip_3(uint8_t startRow, uint8_t areaObjBufferOffset);
    void Skip_4(uint8_t powerUpType, uint8_t blockOffset);
    void Skip_5(uint8_t powerUpType, uint8_t blockOffset);
    void Skip_6();
    void Skip_7();
    void Skip_8();
    void Skip_9();
    void SmallBBox();
    void SmallPlatformBoundBox();
    void SmallPlatformCollision();
    void SoundEngine();
    void SpawnBrickChunks(uint8_t blockOffset);
    bool SpawnHammerObj();
    bool SprObjectCollisionCore(uint8_t objIdx1, uint8_t objIdx2);
    void SprObjectOffscrChk();
    void SpriteShuffler();
    void Square1SfxHandler();
    void Square2SfxHandler();
    void StaircaseObject(uint8_t areaObjBufferOffset);
    void StarBlock(uint8_t blockOffset);
    void Start();
    void SteadM();
    void StopPlatforms();
    void StopPlayerMove();
    void StopSquare1Sfx();
    void StopSquare2Sfx();
    void StoreMT(uint8_t terrainMetatile);
    bool SubtEnemyYPos();
    void TallBBox();
    void TallBBox2();
    void TerminateGame();
    void ThreeFrameExtent();
    void TitleScreenMode();
    void TopScoreCheck(uint8_t scoreOffset);
    bool TransposePlayers();
    void UpToFiery(uint8_t subroutineNum);
    void UpdScrollVar();
    void UpdateNumber(uint8_t statusBarNybbles);
    void UpdateScreen();
    void UpdateTopScore();
    void VariableObjOfsRelPos(uint8_t baseValue, uint8_t addend, uint8_t relPosIdx);
    void VerticalPipe(uint8_t areaObjBufferOffset);
    void VerticalPipeEntry();
    void VictoryMode();
    void VictoryModeSubroutines();
    void VineBlock();
    void VineObjectHandler();
    void Vine_AutoClimb();
    void WarpZoneObject();
    void WaterPipe(uint8_t areaObjBufferOffset);
    void WriteBlockMetatile(uint8_t metatile, uint8_t controlBit);
    void WriteBottomStatusLine();
    void WriteGameText(uint8_t text_number);
    void WriteNTAddr(uint8_t highByte);
    void WritePPUReg1(uint8_t value);
    void WriteTopScore();
    void WriteTopStatusLine();
    void XMoveCntr_GreenPTroopa();
    void XMoveCntr_Platform();
    void XMovingPlatform();
    void YMovingPlatform();

    /**
     * Get CHR data from the ROM.
     */
    uint8_t* getCHR();

    /**
     * Get the byte of RAM or of constant data at an address.
     *
     * The hardware registers that sit between the two have side effects on read
     * and on write, and no byte to hand out; go through readData() and
     * writeData() for those.
     */
    uint8_t& getMemory(uint16_t address);

    /**
     * Get a word of memory from a zero-page address and the next byte (wrapped around),
     * in little-endian format.
     */
    uint16_t getMemoryWord(uint8_t address);

    /**
     * Load all constant data that was present in the SMB ROM.
     *
     * See SMBData.cpp for implementation.
     */
    void loadConstantData();

    /**
     * PHA instruction.
     */
    void pha();

    /**
     * PLA instruction.
     */
    void pla();

    /**
     * Read data from an address in the NES address space.
     */
    uint8_t readData(uint16_t address);

    /**
     * Write data to an address in the NES address space.
     */
    void writeData(uint16_t address, uint8_t value);

    /**
     * Map constant data to the address space. The address must be at least 0x8000.
     */
    void writeData(uint16_t address, const uint8_t* data, std::size_t length);
};

#endif // SMBENGINE_HPP
