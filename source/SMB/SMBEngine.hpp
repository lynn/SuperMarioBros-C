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
    void AlterAreaAttributes();
    void AlternateLengthHandler();
    void AnimationControl();
    void AreaFrenzy();
    void AreaParserCore();
    void AreaParserTaskControl();
    void AreaParserTaskHandler();
    void AreaParserTasks();
    void AreaStyleObject();
    void AutoControlPlayer();
    void AxeObj();
    void BBChk_E(uint8_t coordSelector, uint8_t objectOffset, uint8_t cornerIdx);
    void BalancePlatRope();
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
    void BrickWithCoins();
    void BrickWithItem();
    void BridgeCollapse();
    void Bridge_High();
    void Bridge_Low();
    void Bridge_Middle();
    void BubbleCheck();
    void BulletBillCannon();
    void BulletBillHandler();
    void BumpBlock();
    void CGrab_TTickRegL();
    void CastleBridgeObj();
    void CastleObject();
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
    bool ChkLrgObjFixedLength();
    bool ChkLrgObjLength();
    void ChkNoEn();
    void ChkPOffscr();
    void ChkSmallPlatCollision();
    void ChkToStunEnemies(uint8_t comparisonValue, uint8_t enemyOffset);
    void ChkUnderEnemy();
    void ChkYPCollision();
    void ClearBuffersDrawIcon();
    void ClimbingSub();
    void CoinBlock(uint8_t blockOffset);
    void ColObj();
    void ColorRotation();
    void ColumnOfBricks();
    void ColumnOfSolidBlocks();
    void CommonPlatCode();
    void CommonSmallLift();
    void ContinueBumpThrow();
    void ContinueCGrabTTick();
    void ContinueGame();
    void ContinuePipeDownInj();
    void ContinueSmackEnemy();
    void ContinueSndJump();
    void ContinueSwimStomp();
    void CyclePlayerPalette();
    void DecJpFPS();
    void DecodeAreaData();
    void DecrementSfx1Length();
    void DecrementSfx2Length();
    bool DemoEngine();
    void DestroyBlockMetatile();
    void DigitsMathRoutine(uint8_t digitOffset);
    void DisplayIntermediate();
    void DisplayTimeUp();
    uint8_t DividePDiff(uint8_t value, uint8_t flag, uint8_t currentOffset);
    void DmpJpFPS();
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
    void DrawQBlk();
    void DrawRope();
    void DrawRow();
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
    void Dump_Freq_Regs();
    void Dump_Sq2_Regs();
    void Dump_Squ1_Regs();
    void DuplicateEnemyObj();
    void EmptyBlock();
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
    void ExitPipe();
    void ExtraLifeMushBlock(uint8_t blockOffset);
    void FBallB();
    void FPS2nd();
    void FallE();
    bool FindEmptyEnemySlot();
    std::pair<bool, uint8_t> FindEmptyMiscSlot();
    void FindPlayerAction();
    void FinishFlame();
    void FireballBGCollision();
    void FireballEnemyCollision();
    void FireballObjCore();
    void FirebarCollision();
    void FirebarSpin();
    void FlagBalls_Residual();
    void FlagpoleGfxHandler();
    void FlagpoleObject();
    void FlagpoleRoutine();
    void FlagpoleSlide();
    void FloateyNumbersRoutine();
    void ForceInjury(uint8_t mustBeZero);
    void Fthrow();
    void GameCoreRoutine();
    void GameMenuRoutine();
    void GameMode();
    void GameOverMode();
    void GameRoutines();
    void GameTextLoop();
    void GetAlternatePalette1();
    void GetAreaDataAddrs();
    void GetAreaMusic();
    void GetAreaObjXPosition();
    void GetAreaObjYPosition();
    void GetAreaObjectID();
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
    void GetLrgObjAttrib();
    static uint8_t GetMTileAttrib(uint8_t metatile);
    void GetMaskedOffScrBits(uint8_t enemyOffset, uint8_t defaultBitmask);
    void GetMiscBoundBox();
    void GetMiscOffscreenBits();
    void GetObjRelativePosition(uint8_t objectOffset, uint8_t relPosIdx);
    void GetOffScreenBitsSet(uint8_t objectOffset, uint8_t offscrArrayOffset);
    void GetOffsetFromAnimCtrl();
    void GetPipeHeight();
    void GetPlayerAnimSpeed();
    void GetPlayerColors();
    void GetPlayerOffscreenBits();
    void GetProperObjOffset();
    void GetRow();
    void GetRow2();
    void GetSBNybbles();
    uint8_t GetScreenPosition();
    uint8_t GetXOffscreenBits(uint8_t objectOffset);
    void GiveOneCoin();
    void GoContinue(uint8_t worldNumber);
    void HandleChangeSize();
    void HandleCoinMetatile();
    void HandleEnemyFBallCol();
    void HandleGroupEnemies();
    void HandlePipeEntry();
    void Hidden1UpBlock();
    void Hole_Empty();
    void Hole_Water();
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
    void IntroPipe();
    void JCoinC(uint8_t blockOffset, uint8_t miscSlot);
    void JCoinGfxHandler();
    void JumpEngine();
    void JumpRegContents();
    void Jumpspring();
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
    void LoadControlRegs();
    void LoadEnvelopeData();
    void LoadSqu2Regs();
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
    void NormObj();
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
    void PlayBeat();
    void PlaySqu1Sfx();
    void PlaySqu2Sfx();
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
    void ProcessLengthData();
    void ProcessPlayerAction();
    void ProcessWhirlpools();
    void PulleyRopeObject();
    void PutAtRightExtent();
    void PutBlockMetatile(uint8_t metatileGroupSelector, uint8_t controlBit, uint8_t vramOffset);
    void PutPlayerOnVine();
    void PwrUpJmp();
    void QuestionBlock();
    void QuestionBlockRow_High();
    void QuestionBlockRow_Low();
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
    bool RenderSidewaysPipe();
    void RenderUnderPart();
    void ReplaceBlockMetatile();
    void ResJmpM();
    void ResetPalStar();
    void ResetScreenTimer();
    void ResetSpritesAndScreenTimer();
    void ResetTitle();
    void ResidualGravityCode();
    void ResidualMiscObjectCode();
    void RightPlatform();
    void RowOfBricks();
    void RowOfCoins();
    void RowOfSolidBlocks();
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
    void SetFreq_Squ1();
    void SetFreq_Squ2();
    void SetFreq_Tri();
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
    void Skip_1();
    void Skip_2();
    void Skip_3();
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
    void StaircaseObject();
    void StarBlock(uint8_t blockOffset);
    void Start();
    void SteadM();
    void StopPlatforms();
    void StopPlayerMove();
    void StopSquare1Sfx();
    void StopSquare2Sfx();
    void StoreMT();
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
    void VerticalPipe();
    void VerticalPipeEntry();
    void VictoryMode();
    void VictoryModeSubroutines();
    void VineBlock();
    void VineObjectHandler();
    void Vine_AutoClimb();
    void WarpZoneObject();
    void WaterPipe();
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
