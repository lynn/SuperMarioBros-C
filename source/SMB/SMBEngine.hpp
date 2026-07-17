#ifndef SMBENGINE_HPP
#define SMBENGINE_HPP

#include <cstdint>
#include <cstddef>
#include <tuple>
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
     * Render the whole background the PPU holds, scroll ignored, for debugging.
     *
     * @param buffer a 512x480 32-bit color buffer for storing the rendering.
     * @param scrollX set to the left edge of the region render() draws.
     * @param scrollY set to the top edge of the region render() draws.
     */
    void renderNametables(uint32_t* buffer, int& scrollX, int& scrollY);

    /**
     * Reset the game engine to power-on state.
     */
    void reset();

    /**
     * Update the game engine by one frame.
     */
    void update();

    /**
     * Save everything the game is holding, for loadState() to put back. There is one
     * slot, so saving again replaces what was in it.
     *
     * The APU is not part of this. A sound that is playing when a state is loaded goes
     * on to the end of it, but which music the engine plays from there is in the RAM,
     * and comes back with the rest.
     */
    void saveState();

    /**
     * Put the game back the way the last saveState() left it.
     *
     * @return false if nothing has been saved yet, in which case nothing changes.
     */
    bool loadState();

    /**
     * Whether there is a saved state for loadState() to put back.
     */
    bool hasState() const;

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

    // state! wow!
    const uint8_t* musicData;

    /**
     * What saveState() keeps, defined in SMBEngine.cpp so that this header does not
     * need the PPU's. Null until the first save.
     */
    struct State;
    State* savedState;

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
    uint8_t BBChk_E(uint8_t coordSelector, uint8_t objectOffset, uint8_t cornerIdx);
    void BalancePlatRope(uint8_t areaObjBufferOffset);
    void BalancePlatform(uint8_t e);
    uint8_t BlockBufferChk_Enemy(uint8_t coordSelector, uint8_t cornerIdx, uint8_t e);
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
    void CheckAnimationStop(uint8_t gfxOffset);
    void CheckDefeatedState(uint8_t gfxOffset);
    static bool CheckForClimbMTiles(uint8_t metatile);
    bool CheckForCoinMTiles(uint8_t metatile);
    void CheckForHammerBro(uint8_t gfxOffset);
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
    void ChkForDemoteKoopa(uint8_t comparisonValue, uint8_t eid);
    void ChkForLandJumpSpring(uint8_t metatile);
    static bool ChkForNonSolids(uint8_t metatile);
    void ChkForPlayerAttrib();
    void ChkForPlayerC_LargeP();
    static bool ChkInvisibleMTiles(uint8_t metatile);
    static bool ChkJumpspringMetatiles(uint8_t metatile);
    void ChkLak(uint8_t startSlot, uint8_t spinySlot);
    void ChkLeftCo();
    bool ChkLrgObjFixedLength(uint8_t areaObjBufferOffset, uint8_t lengthIfUnset);
    bool ChkLrgObjLength(uint8_t areaObjBufferOffset, uint8_t& outLength);
    void ChkNoEn();
    void ChkPOffscr();
    void ChkSmallPlatCollision();
    void ChkToStunEnemies(uint8_t species, uint8_t eid);
    uint8_t ChkUnderEnemy(uint8_t e);
    void ChkYPCollision(uint8_t e);
    void ClearBuffersDrawIcon();
    void ClimbingSub();
    void CoinBlock(uint8_t blockOffset);
    void ColObj(uint8_t tile, uint8_t startCol);
    void ColorRotation();
    void ColumnOfBricks(uint8_t areaObjBufferOffset);
    void ColumnOfSolidBlocks(uint8_t areaObjBufferOffset);
    void CommonPlatCode(uint8_t e);
    void CommonSmallLift(uint8_t e);
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
    void DoOtherPlatform(uint8_t oldYPos, uint8_t e);
    void DonePlayerTask();
    void DrawBlock();
    void DrawBrickChunks();
    void DrawBubble();
    std::pair<uint8_t, uint8_t> DrawEnemyObjRow(uint8_t gfxOffset, uint8_t oamSlot);
    void DrawEnemyObject(uint8_t gfxOffset);
    void DrawExplosion_Fireball();
    void DrawExplosion_Fireworks(uint8_t frameSelector, uint8_t spriteDataBase);
    void DrawFireball();
    void DrawFirebar(uint8_t oamSlot);
    void DrawFirebar_Collision();
    void DrawHammer();
    void DrawLargePlatform(uint8_t e);
    void DrawMushroomIcon();
    std::pair<uint8_t, uint8_t> DrawOneSpriteRow(uint8_t tileNumber, uint8_t spritePairIdx, uint8_t oamSlot);
    void DrawPlayerLoop();
    void DrawPlayer_Intermediate();
    void DrawPowerUp();
    void DrawQBlk(uint8_t brickQBlockIndex, uint8_t areaObjBufferOffset);
    void DrawRope(uint8_t startCol, uint8_t numRows);
    void DrawRow(uint8_t tile);
    void DrawSmallPlatform(uint8_t e);
    std::pair<uint8_t, uint8_t> DrawSpriteObject(uint8_t spritePairIdx, uint8_t oamSlot);
    void DrawStarFlag();
    void DrawTitleScreen();
    void DrawVine(uint8_t segment);
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
    void EndFrenzy(uint8_t e);
    void EndlessRope();
    void EnemiesAndLoopsCore();
    void EnemiesCollision();
    uint8_t EnemyFacePlayer(uint8_t eid);
    void EnemyGfxHandler(uint8_t eid);
    void EnemyJump();
    void EnemyLanding(uint8_t e);
    void EnemyMovementSubs();
    void EnemySmackScore(uint8_t pointsControl, uint8_t eid);
    void EnemyToBGCollisionDet();
    void EnemyTurnAround(uint8_t eid);
    void EnterSidePipe();
    void Entrance_GameTimerSetup();
    void ErACM();
    void EraseEnemyObject(uint8_t eid);
    void ExInjColRoutines();
    void ExecGameLoopback(uint8_t loopIndex);
    void ExitPipe(uint8_t areaObjBufferOffset);
    void ExtraLifeMushBlock(uint8_t blockOffset);
    void FBallB();
    void FPS2nd(uint8_t ctrlByte);
    void FallE(uint8_t e);
    bool FindEmptyEnemySlot(uint8_t& outSlot);
    std::pair<bool, uint8_t> FindEmptyMiscSlot();
    void FindPlayerAction();
    void FinishFlame();
    void FireballBGCollision();
    void FireballEnemyCollision();
    void FireballObjCore();
    void FirebarCollision(uint8_t oamOffset);
    uint8_t FirebarSpin(uint8_t spinSpeed, uint8_t e);
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
    void GetEnemyBoundBox(uint8_t eid);
    std::pair<uint8_t, uint8_t> GetEnemyBoundBoxOfs();
    std::pair<uint8_t, uint8_t> GetEnemyBoundBoxOfsArg(uint8_t eid);
    void GetEnemyOffscreenBits();
    void GetFireballBoundBox();
    void GetFireballOffscreenBits();
    void GetFirebarPosition(uint8_t spinstateHigh);
    void GetGfxOffsetAdder();
    uint8_t GetLrgObjAttrib(uint8_t areaObjBufferOffset);
    static uint8_t GetMTileAttrib(uint8_t metatile);
    void GetMaskedOffScrBits(uint8_t eid, uint8_t defaultBitmask);
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
    void HandleNoiseMusic();
    void HandlePipeEntry();
    void HandleSquare1Music();
    void HandleTriangleMusic();
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
    void IncrementColumnPos();
    void InitBalPlatform(uint8_t e);
    void InitBlock_XY_Pos(uint8_t blockOffset);
    void InitBloober(uint8_t e);
    void InitBowser();
    void InitBowserFlame();
    void InitBulletBill(uint8_t e);
    void InitCheepCheep(uint8_t e);
    void InitDropPlatform(uint8_t e);
    void InitEnemyObject();
    void InitFireworks(uint8_t e);
    void InitFlyingCheepCheep();
    void InitGoomba(uint8_t e);
    void InitHammerBro(uint8_t e);
    void InitHoriPlatform(uint8_t e);
    void InitHorizFlySwimEnemy(uint8_t e);
    void InitJumpGPTroopa(uint8_t e);
    void InitLakitu(uint8_t e);
    void InitLongFirebar(uint8_t e);
    void InitNTLoop(uint8_t tile, uint8_t xCount, uint8_t yCount);
    void InitNormalEnemy(uint8_t e);
    void InitPiranhaPlant(uint8_t eid);
    void InitPodoboo(uint8_t e);
    void InitRedKoopa(uint8_t e);
    void InitRedPTroopa(uint8_t e);
    void InitRetainerObj(uint8_t e);
    void InitScreen();
    void InitScroll(uint8_t value);
    void InitShortFirebar(uint8_t e);
    void InitVStf(uint8_t eid);
    void InitVertPlatform(uint8_t e);
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
    void LInj(uint8_t eid);
    void LakituAndSpinyHandler();
    void LargeLiftBBox(uint8_t e);
    void LargeLiftDown(uint8_t e);
    void LargeLiftUp(uint8_t e);
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
    void MoveBloober(uint8_t e);
    void MoveBoundBoxOffscreen(uint8_t eid);
    void MoveBubl();
    void MoveBulletBill();
    uint8_t MoveColOffscreen(uint8_t yPosOffset);
    void MoveD_Bowser();
    void MoveD_EnemyVertically(uint8_t eid);
    void MoveDropPlatform();
    void MoveESprColOffscreen(uint8_t rowSelectorBase, uint8_t eid);
    void MoveESprRowOffscreen(uint8_t rowSelectorBase, uint8_t eid);
    uint8_t MoveEnemyHorizontally(uint8_t eid);
    void MoveEnemySlowVert();
    void MoveFallingPlatform();
    void MoveFlyGreenPTroopa(uint8_t e);
    void MoveFlyingCheepCheep(uint8_t e);
    void MoveHammerBroXDir(uint8_t e);
    void MoveJ_EnemyVertically();
    void MoveJumpingEnemy(uint8_t e);
    void MoveLakitu(uint8_t e);
    void MoveLargeLiftPlat(uint8_t e);
    void MoveLiftPlatforms();
    void MoveNormalEnemy(uint8_t e);
    uint8_t MoveObjectHorizontally(uint8_t objectOffset);
    void MovePiranhaPlant(uint8_t e);
    void MovePlatformDown(uint8_t e);
    void MovePlatformUp(uint8_t e);
    uint8_t MovePlayerHorizontally();
    void MovePlayerVertically();
    void MovePlayerYAxis();
    void MovePodoboo(uint8_t e);
    void MoveRedPTroopa(uint8_t moveDirection, uint8_t e);
    void MoveRedPTroopaDown(uint8_t e);
    void MoveSixSpritesOffscreen(uint8_t baseOffset);
    void MoveSmallPlatform();
    void MoveSpritesOffscreen();
    void MoveSwimmingCheepCheep(uint8_t e);
    void MoveVOffset(uint8_t vramOffset);
    void MoveWithXMCntrs(uint8_t e);
    void MushFlowerBlock(uint8_t blockOffset);
    void MusicHandler();
    void NextArea();
    void NoBump();
    void NoiseSfxHandler();
    void NonMaskableInterrupt();
    void NormObj(uint8_t objectId, uint8_t areaObjBufferOffset);
    void NotMoveEnemySlowVert();
    void NullJoypad();
    void OffscreenBoundsCheck(uint8_t eid);
    void OnGroundStateSub();
    void OperModeExecutionTree();
    void OutputInter(uint8_t text_number);
    void OutputNumbers(uint8_t nybbleIdx);
    void PauseRoutine();
    void PlatLiftDown(uint8_t e);
    void PlatLiftUp(uint8_t e);
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
    void PlayerEnemyCollision(uint8_t eid);
    std::pair<bool, uint8_t> PlayerEnemyDiff(uint8_t eid);
    void PlayerEntrance();
    void PlayerFireFlower();
    void PlayerGfxHandler();
    void PlayerGfxProcessing();
    void PlayerHammerCollision();
    void PlayerHeadCollision(uint8_t collidedMetatile);
    uint8_t PlayerLakituDiff(uint8_t e);
    void PlayerLoseLife();
    void PlayerMovementSubs();
    void PlayerOffscreenChk();
    void PlayerPhysicsSub();
    void PlayerVictoryWalk();
    void PosPlatform(uint8_t offsetIndex, uint8_t e);
    void PositionPlayerOnHPlat();
    void PositionPlayerOnS_Plat(uint8_t collisionFlag, uint8_t e);
    void PositionPlayerOnVPlat(uint8_t e);
    void PowerUpObjHandler();
    void PrimaryGameSetup();
    void PrintStatusBarNumbers(uint8_t playerOffset);
    void PrintVictoryMessages();
    void ProcBowserFlame();
    void ProcEnemyCollisions();
    void ProcFireball_Bubble();
    void ProcFirebar(uint8_t e);
    void ProcHammerBro(uint8_t e);
    void ProcHammerObj();
    void ProcLPlatCollisions();
    void ProcLoopCommand();
    void ProcMoveRedPTroopa(uint8_t e);
    void ProcSwimmingB(bool blooberCarry, uint8_t e);
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
    void RXSpd(uint8_t eid);
    void ReadJoypads();
    void ReadPortBits(uint8_t port);
    void RedPTroopaGrav(uint8_t moveDirection, uint8_t e);
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
    void RunBowserFlame(uint8_t e);
    void RunDemo();
    void RunFirebarObj(uint8_t e);
    void RunFireworks();
    void RunGameOver();
    void RunGameTimer();
    void RunLargePlatform(uint8_t e);
    void RunNormalEnemies();
    uint8_t RunOffscrBitsSubs(uint8_t objectOffset);
    void RunPUSubs(uint8_t e);
    void RunRetainerObj(uint8_t e);
    void RunSmallPlatform(uint8_t e);
    void RunStarFlagObj(uint8_t e);
    void SPBBox(uint8_t e);
    void ScreenRoutines();
    void ScrollHandler();
    void ScrollLockObject();
    void ScrollLockObject_Warp();
    void ScrollScreen(uint8_t scrollAmount);
    void SecondaryGameSetup();
    void SetBBox(uint8_t boundBoxCtrl, uint8_t e);
    void SetBBox2(uint8_t boundBoxCtrl, uint8_t eid);
    void SetESpd(uint8_t speed, uint8_t e);
    void SetEntr();
    void SetFlameTimer();
    uint8_t SetFreq_Squ1(uint8_t freqIndex);
    uint8_t SetFreq_Squ2(uint8_t freqIndex);
    uint8_t SetFreq_Tri(uint8_t freqIndex);
    void SetHJ(uint8_t verticalSpeed, uint8_t e);
    void SetHiMax();
    void SetKRout(uint8_t subroutineNum);
    void SetOffscrBitsOffset(uint8_t addend, uint8_t baseObjectOffset, uint8_t offscrArrayOffset);
    void SetPRout(uint8_t subroutineNum, uint8_t newPlayerState);
    void SetShim(uint8_t movingDir, uint8_t e);
    void SetStun(uint8_t eid);
    void SetVRAMAddr_A(uint8_t addrCtrl);
    void SetVRAMCtrl();
    void SetVRAMOffset(uint8_t newOffset);
    void SetXMoveAmt(uint8_t maxSpeed, uint8_t eid, uint8_t downwardMoveAmt);
    void SetupBubble();
    void SetupEOffsetFBBox(uint8_t objectOffset);
    uint8_t SetupFloateyNumber(uint8_t pointsControl, uint8_t eid);
    void SetupGameOver();
    void SetupIntermediate();
    void SetupJumpCoin(uint8_t blockOffset);
    void SetupLakitu(uint8_t e);
    void SetupPlatformRope(uint8_t vertSpeed, uint8_t e);
    void SetupPowerUp(uint8_t blockOffset);
    void SetupVictoryMode();
    void Setup_Vine(uint8_t eid, uint8_t blockOffset);
    void ShellOrBlockDefeat(uint8_t eid);
    void SideExitPipeEntry();
    uint8_t SixSpriteStacker(uint8_t baseCoord, uint8_t oamOffset);
    void Skip_0(uint8_t yOffset);
    void Skip_1(uint8_t startRow, uint8_t areaObjBufferOffset);
    void Skip_3(uint8_t startRow, uint8_t areaObjBufferOffset);
    void Skip_4(uint8_t powerUpType, uint8_t blockOffset);
    void Skip_5(uint8_t powerUpType, uint8_t blockOffset);
    void Skip_6();
    void Skip_7(uint8_t moveDirection, uint8_t e);
    void Skip_8(uint8_t yCoord, uint8_t e);
    void Skip_9();
    void SmallBBox(uint8_t e);
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
    void SteadM(uint8_t decelIndex, uint8_t e);
    void StopPlatforms(uint8_t e, uint8_t otherPlatform);
    void StopPlayerMove();
    void StopSquare1Sfx();
    void StopSquare2Sfx();
    void StoreMT(uint8_t terrainMetatile);
    bool SubtEnemyYPos(uint8_t e);
    void TallBBox(uint8_t e);
    void TallBBox2(uint8_t e);
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
    void XMoveCntr_GreenPTroopa(uint8_t e);
    void XMoveCntr_Platform(uint8_t maxSecondary, uint8_t e);
    void XMovingPlatform();
    void YMovingPlatform(uint8_t e);

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
