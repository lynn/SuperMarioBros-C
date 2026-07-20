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
    uint8_t s;                   /**< Stack index register (only JumpEngine, itself unreachable, still touches it). */
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
    uint8_t AnimationControl(uint8_t upperExtent, uint8_t baseIdx);
    void AreaFrenzy();
    void AreaParserCore();
    void AreaParserTaskControl();
    void AreaParserTaskHandler();
    void AreaParserTasks(uint8_t taskNum);
    void AreaStyleObject(uint8_t areaObjBufferOffset);
    void TreeLedge(uint8_t areaObjBufferOffset);
    void MushroomLedge(uint8_t areaObjBufferOffset);
    void AutoControlPlayer(uint8_t ctrlBits);
    void AxeObj();
    std::pair<uint8_t, uint8_t> BBChk_E(uint8_t coordSelector, uint8_t objectOffset, uint8_t cornerIdx);
    void BalancePlatRope(uint8_t areaObjBufferOffset);
    void BalancePlatform(uint8_t e);
    std::pair<uint8_t, uint8_t> BlockBufferChk_Enemy(uint8_t coordSelector, uint8_t cornerIdx, uint8_t e);
    uint8_t BlockBufferChk_FBall(uint8_t slot);
    std::pair<uint8_t, uint8_t> BlockBufferColli_Feet(uint8_t adderBaseOffset);
    std::pair<uint8_t, uint8_t> BlockBufferColli_Head(uint8_t adderOffset);
    std::pair<uint8_t, uint8_t> BlockBufferColli_Side(uint8_t adderOffset);
    std::pair<uint8_t, uint8_t> BlockBufferCollision(uint8_t coordSelector, uint8_t objectOffset, uint8_t cornerIdx);
    std::pair<bool, uint8_t> BlockBumpedChk(uint8_t metatile);
    void BlockObjMT_Updater();
    void BlockObjectsCore(uint8_t slot);
    uint8_t BoundingBoxCore(uint8_t boundBoxCtrlIdx, uint8_t relPosIdx);
    void BowserGfxHandler(uint8_t enemyOffset);
    void BranchToDecLength1();
    void BrickShatter();
    void BrickWithCoins(uint8_t areaObjBufferOffset);
    void BrickWithItem(uint8_t areaObjBufferOffset);
    void BridgeCollapse();
    void Bridge_High(uint8_t areaObjBufferOffset);
    void Bridge_Low(uint8_t areaObjBufferOffset);
    void Bridge_Middle(uint8_t areaObjBufferOffset);
    void BubbleCheck(uint8_t slot);
    void BulletBillCannon(uint8_t areaObjBufferOffset);
    void BulletBillHandler(uint8_t slot);
    void BumpBlock(uint8_t collidedMetatile);
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
    void CheckpointEnemyID(uint8_t e);
    void ChgAreaMode();
    void ChgAreaPipe(uint8_t mode);
    void ChkContinue(uint8_t joypadBits);
    void ChkFireB(uint8_t e);
    void ChkForBump_HammerBroJ(uint8_t e);
    void ChkForDemoteKoopa(uint8_t comparisonValue, uint8_t eid);
    void ChkForLandJumpSpring(uint8_t metatile);
    static bool ChkForNonSolids(uint8_t metatile);
    void ChkForPlayerAttrib();
    void ChkForPlayerC_LargeP(uint8_t e);
    static bool ChkInvisibleMTiles(uint8_t metatile);
    static bool ChkJumpspringMetatiles(uint8_t metatile);
    void ChkLak(uint8_t startSlot, uint8_t spinySlot);
    void ChkLeftCo(uint8_t offscreenBits, uint8_t oamSlot);
    bool ChkLrgObjFixedLength(uint8_t areaObjBufferOffset, uint8_t lengthIfUnset);
    bool ChkLrgObjLength(uint8_t areaObjBufferOffset, uint8_t& outLength);
    void ChkNoEn(uint8_t startSlot);
    void ChkPOffscr();
    void ChkSmallPlatCollision(uint8_t e);
    void ChkToStunEnemies(uint8_t species, uint8_t eid);
    std::pair<uint8_t, uint8_t> ChkUnderEnemy(uint8_t e);
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
    void CyclePlayerPalette(uint8_t bits);
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
    void DoEnemySideCheck(uint8_t e);
    void DoNothing1();
    static void DoNothing2();
    void DoOtherPlatform(uint8_t oldYPos, uint8_t e);
    void DonePlayerTask();
    void PlayerChangeSize();
    void PlayerInjuryBlink();
    void InitChangeSize();
    void DrawBlock(uint8_t slot);
    void DrawBrickChunks(uint8_t slot);
    void DrawBubble(uint8_t slot);
    std::pair<uint8_t, uint8_t> DrawEnemyObjRow(uint8_t gfxOffset, uint8_t oamSlot);
    void DrawEnemyObject(uint8_t gfxOffset);
    void DrawExplosion_Fireball(uint8_t slot);
    void DrawExplosion_Fireworks(uint8_t frameSelector, uint8_t spriteDataBase);
    void DrawFireball(uint8_t slot);
    void DrawFirebar(uint8_t oamSlot);
    void DrawFirebar_Collision(uint8_t mirrorData);
    void DrawHammer(uint8_t slot);
    void DrawLargePlatform(uint8_t e);
    void DrawMushroomIcon();
    std::pair<uint8_t, uint8_t> DrawOneSpriteRow(uint8_t firstTile, uint8_t secondTile, uint8_t spritePairIdx,
                                                 uint8_t oamSlot, uint8_t flipBits, uint8_t attributeBits,
                                                 uint8_t xPos, uint8_t& yPos);
    void DrawPlayerLoop(uint8_t gfxOffset, uint8_t sprDataOffset, uint8_t flipBits, uint8_t attributeBits,
                        uint8_t xPos, uint8_t yPos);
    void DrawPlayer_Intermediate();
    void DrawPowerUp();
    void DrawQBlk(uint8_t brickQBlockIndex, uint8_t areaObjBufferOffset);
    void DrawRope(uint8_t startCol, uint8_t numRows);
    void DrawRow(uint8_t tile);
    void DrawSmallPlatform(uint8_t e);
    std::pair<uint8_t, uint8_t> DrawSpriteObject(uint8_t firstTile, uint8_t secondTile, uint8_t spritePairIdx,
                                                 uint8_t oamSlot, uint8_t flipBits, uint8_t attributeBits,
                                                 uint8_t xPos, uint8_t& yPos);
    void DrawStarFlag(uint8_t e);
    void DrawTitleScreen();
    void DrawVine(uint8_t segment);
    void DropPlatform(uint8_t e);
    void DumpFourSpr(uint8_t value, uint8_t baseOffset);
    void DumpSixSpr(uint8_t value, uint8_t baseOffset);
    void DumpThreeSpr(uint8_t value, uint8_t baseOffset);
    void DumpTwoSpr(uint8_t value, uint8_t baseOffset);
    uint8_t Dump_Freq_Regs(uint8_t freqIndex, uint8_t channelOffset);
    void Dump_Sq2_Regs(uint8_t ctrlByte, uint8_t sweepByte);
    void Dump_Squ1_Regs(uint8_t ctrlByte, uint8_t sweepByte);
    void DuplicateEnemyObj(uint8_t e);
    void EmptyBlock(uint8_t areaObjBufferOffset);
    void EmptySfx2Buffer();
    void EndAreaPoints();
    void EndFrenzy(uint8_t e);
    void EndlessRope();
    void EnemiesAndLoopsCore(uint8_t enemyOffset);
    void EnemiesCollision(uint8_t e);
    uint8_t EnemyFacePlayer(uint8_t eid);
    void EnemyGfxHandler(uint8_t eid);
    void EnemyJump(uint8_t e);
    void EnemyLanding(uint8_t e);
    void EnemyMovementSubs(uint8_t e);
    void EnemySmackScore(uint8_t pointsControl, uint8_t eid);
    void EnemyToBGCollisionDet(uint8_t e);
    void EnemyTurnAround(uint8_t eid);
    void EnterSidePipe();
    void Entrance_GameTimerSetup();
    void ErACM();
    void EraseEnemyObject(uint8_t eid);
    void ExInjColRoutines();
    void ExecGameLoopback(uint8_t loopIndex);
    void ExitPipe(uint8_t areaObjBufferOffset);
    void ExtraLifeMushBlock(uint8_t blockOffset);
    void FBallB(uint8_t objectOffset, uint8_t relPosIdx);
    void FPS2nd(uint8_t ctrlByte);
    void FallE(uint8_t e);
    bool FindEmptyEnemySlot(uint8_t& outSlot);
    std::pair<bool, uint8_t> FindEmptyMiscSlot();
    void FindPlayerAction();
    void FinishFlame(uint8_t e);
    void FireballBGCollision(uint8_t slot);
    void FireballEnemyCollision(uint8_t slot);
    void FireballObjCore(uint8_t slot);
    void FirebarCollision(uint8_t oamOffset);
    uint8_t FirebarSpin(uint8_t spinSpeed, uint8_t e);
    void FlagBalls_Residual(uint8_t areaObjBufferOffset);
    void FlagpoleGfxHandler(uint8_t slot);
    void FlagpoleObject();
    void FlagpoleRoutine();
    void FlagpoleSlide();
    void FloateyNumbersRoutine(uint8_t slot);
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
    void GetAreaType(uint8_t areaPointerByte);
    void GetBackgroundColor();
    void GetBlockBufferAddr(uint8_t column);
    void GetBlockOffscreenBits(uint8_t slot);
    void GetBubbleOffscreenBits(uint8_t slot);
    uint8_t GetCurrentAnimOffset(uint8_t baseIdx);
    void GetEnemyBoundBox(uint8_t eid);
    std::pair<uint8_t, uint8_t> GetEnemyBoundBoxOfs();
    std::pair<uint8_t, uint8_t> GetEnemyBoundBoxOfsArg(uint8_t eid);
    void GetEnemyOffscreenBits(uint8_t offset);
    void GetFireballBoundBox(uint8_t slot);
    void GetFireballOffscreenBits(uint8_t slot);
    uint8_t GetFirebarPosition(uint8_t spinstateHigh);
    uint8_t GetGfxOffsetAdder(uint8_t baseIdx);
    uint8_t GetLrgObjAttrib(uint8_t areaObjBufferOffset);
    static uint8_t GetMTileAttrib(uint8_t metatile);
    void GetMaskedOffScrBits(uint8_t eid, uint8_t defaultBitmask);
    void GetMiscBoundBox(uint8_t slot);
    void GetMiscOffscreenBits(uint8_t slot);
    void GetObjRelativePosition(uint8_t objectOffset, uint8_t relPosIdx);
    void GetOffScreenBitsSet(uint8_t objectOffset, uint8_t offscrArrayOffset);
    uint8_t GetOffsetFromAnimCtrl(uint8_t frameCtrl, uint8_t baseIdx);
    uint8_t GetPipeHeight(uint8_t areaObjBufferOffset);
    void GetPlayerAnimSpeed();
    uint8_t GetPlayerColors();
    void GetPlayerOffscreenBits();
    uint8_t GetProperObjOffset(uint8_t baseOffset, uint8_t tableSelector);
    void GetRow(uint8_t tile, uint8_t areaObjBufferOffset);
    void GetRow2(uint8_t tile, uint8_t areaObjBufferOffset);
    void GetSBNybbles();
    uint8_t GetScreenPosition();
    uint8_t GetXOffscreenBits(uint8_t objectOffset);
    void GiveOneCoin();
    void GoContinue(uint8_t worldNumber);
    void GrowItemRegs(uint8_t length);
    uint8_t HandleChangeSize();
    void HandleCoinMetatile();
    void HandleEnemyFBallCol(uint8_t enemySlot);
    void HandleGroupEnemies(uint8_t enemyByte);
    void HandleNoiseMusic();
    void HandlePipeEntry(uint8_t rightFootMetatile, uint8_t leftFootMetatile);
    void HandleSquare1Music();
    void HandleTriangleMusic();
    void Hidden1UpBlock(uint8_t areaObjBufferOffset);
    void Hole_Empty(uint8_t areaObjBufferOffset);
    void Hole_Water(uint8_t areaObjBufferOffset);
    void HurtBowser(uint8_t slot, uint8_t scoreSlot);
    void ImpedePlayerMove(uint8_t side);
    void ImposeFriction(uint8_t leftRightButtons);
    void ImposeGravity(uint8_t movementMode, uint8_t objectOffset, uint8_t downAmount, uint8_t upAmount,
                       uint8_t maxSpeed);
    void ImposeGravityBlock(uint8_t slot);
    void ImposeGravitySprObj(uint8_t maxSpeed, uint8_t objectOffset, uint8_t downAmount);
    void Inc2B();
    void Inc3B();
    void IncAreaObjOffset();
    void IncrementColumnPos();
    void InitBalPlatform(uint8_t e);
    void InitBlock_XY_Pos(uint8_t blockOffset);
    void InitBloober(uint8_t e);
    void InitBowser(uint8_t e);
    void InitBowserFlame(uint8_t e);
    void InitBulletBill(uint8_t e);
    void InitCheepCheep(uint8_t e);
    void InitDropPlatform(uint8_t e);
    void InitEnemyObject();
    void InitFireworks(uint8_t e);
    void InitFlyingCheepCheep(uint8_t e);
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
    void JCoinGfxHandler(uint8_t slot);
    void JumpEngine(uint8_t tableIdx);
    void JumpRegContents(uint8_t freqIndex);
    void Jumpspring(uint8_t areaObjBufferOffset);
    void JumpspringHandler(uint8_t e);
    void KillAllEnemies();
    void KillEnemies(uint8_t enemyId);
    void KillEnemyAboveBlock();
    void LInj(uint8_t eid);
    void LakituAndSpinyHandler(uint8_t slot);
    void LargeLiftBBox(uint8_t e);
    void LargeLiftDown(uint8_t e);
    void LargeLiftUp(uint8_t e);
    void LargePlatformBoundBox(uint8_t e);
    void LargePlatformCollision(uint8_t e);
    void LargePlatformSubroutines(uint8_t e);
    void LoadAreaPointer();
    uint8_t LoadControlRegs();
    uint8_t LoadEnvelopeData(uint8_t offset);
    void LoadSqu2Regs(uint8_t freqIndex, uint8_t ctrlByte, uint8_t sweepByte);
    void MiscObjectsCore();
    void MiscSqu2MusicTasks();
    void MoveAllSpritesOffscreen();
    void MoveBloober(uint8_t e);
    void MoveBoundBoxOffscreen(uint8_t eid);
    void MoveBubl(uint8_t slot);
    void MoveBulletBill(uint8_t e);
    uint8_t MoveColOffscreen(uint8_t yPosOffset);
    void MoveD_Bowser(uint8_t enemyOffset);
    void MoveD_EnemyVertically(uint8_t eid);
    void MoveDropPlatform(uint8_t e);
    void MoveESprColOffscreen(uint8_t rowSelectorBase, uint8_t eid);
    void MoveESprRowOffscreen(uint8_t rowSelectorBase, uint8_t eid);
    uint8_t MoveEnemyHorizontally(uint8_t eid);
    void MoveEnemySlowVert(uint8_t e);
    void MoveFallingPlatform(uint8_t e);
    void MoveFlyGreenPTroopa(uint8_t e);
    void MoveFlyingCheepCheep(uint8_t e);
    void MoveHammerBroXDir(uint8_t e);
    void MoveJ_EnemyVertically(uint8_t e);
    void MoveJumpingEnemy(uint8_t e);
    void MoveLakitu(uint8_t e);
    void MoveLargeLiftPlat(uint8_t e);
    void MoveLiftPlatforms(uint8_t e);
    void MoveNormalEnemy(uint8_t e);
    uint8_t MoveObjectHorizontally(uint8_t objectOffset);
    void MovePiranhaPlant(uint8_t e);
    void MovePlatformDown(uint8_t e);
    void MovePlatformUp(uint8_t e);
    uint8_t MovePlayerHorizontally();
    void MovePlayerVertically();
    void MovePlayerYAxis(uint8_t amount);
    void MovePodoboo(uint8_t e);
    void MoveRedPTroopa(uint8_t moveDirection, uint8_t e);
    void MoveRedPTroopaDown(uint8_t e);
    void MoveSixSpritesOffscreen(uint8_t baseOffset);
    void MoveSmallPlatform(uint8_t e);
    void MoveSpritesOffscreen();
    void MoveSwimmingCheepCheep(uint8_t e);
    void MoveVOffset(uint8_t vramOffset);
    void MoveWithXMCntrs(uint8_t e);
    void MushFlowerBlock(uint8_t blockOffset);
    void MusicHandler();
    void NextArea();
    void NoBump(uint8_t e);
    void NoiseSfxHandler();
    void NonMaskableInterrupt();
    void NormObj(uint8_t objectId, uint8_t areaObjBufferOffset);
    void NotMoveEnemySlowVert(uint8_t e, uint8_t downwardMoveAmt);
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
    std::tuple<bool, uint8_t, uint8_t> PlayerEnemyDiff(uint8_t eid);
    void PlayerEntrance();
    void PlayerFireFlower();
    void PlayerGfxHandler();
    void PlayerGfxProcessing(uint8_t gfxOffset);
    void PlayerHammerCollision(uint8_t slot);
    void PlayerHeadCollision(uint8_t collidedMetatile);
    uint8_t PlayerLakituDiff(uint8_t e);
    void PlayerLoseLife();
    void PlayerMovementSubs();
    void PlayerOffscreenChk();
    void PlayerPhysicsSub();
    void PlayerVictoryWalk();
    void PosPlatform(uint8_t offsetIndex, uint8_t e);
    void PositionPlayerOnHPlat(uint8_t e);
    void PositionPlayerOnS_Plat(uint8_t collisionFlag, uint8_t e);
    void PositionPlayerOnVPlat(uint8_t e);
    void PowerUpObjHandler();
    void PrimaryGameSetup();
    void PrintStatusBarNumbers(uint8_t playerOffset);
    void PrintVictoryMessages();
    void ProcBowserFlame(uint8_t e);
    void ProcEnemyCollisions(uint8_t first, uint8_t second);
    void ProcFireball_Bubble();
    void ProcFirebar(uint8_t e);
    void ProcHammerBro(uint8_t e);
    void ProcHammerObj(uint8_t slot);
    void ProcLPlatCollisions(uint8_t boundBoxOfs, uint8_t e);
    void ProcLoopCommand(uint8_t e);
    void ProcMoveRedPTroopa(uint8_t e);
    void ProcSwimmingB(bool blooberCarry, uint8_t e);
    void ProcessAreaData();
    void ProcessBowserHalf(uint8_t e);
    void ProcessCannons();
    uint8_t ProcessLengthData(uint8_t lengthCode);
    uint8_t ProcessPlayerAction();
    void ProcessWhirlpools();
    void PulleyRopeObject(uint8_t areaObjBufferOffset);
    void PutAtRightExtent(uint8_t verticalPos, uint8_t e);
    void PutBlockMetatile(uint8_t metatileGroupSelector, uint8_t vramOffset);
    void PutPlayerOnVine();
    void PwrUpJmp();
    void QuestionBlock(uint8_t areaObjBufferOffset);
    void QuestionBlockRow_High(uint8_t areaObjBufferOffset);
    void QuestionBlockRow_Low(uint8_t areaObjBufferOffset);
    void RXSpd(uint8_t eid);
    void ReadJoypads();
    void ReadPortBits(uint8_t port);
    void RedPTroopaGrav(uint8_t moveDirection, uint8_t e, uint8_t downAmount, uint8_t upAmount, uint8_t maxSpeed);
    void RelWOfs(uint8_t objectOffset, uint8_t relPosIdx);
    void RelativeBlockPosition(uint8_t slot);
    void RelativeBubblePosition(uint8_t slot);
    void RelativeEnemyPosition(uint8_t offset);
    void RelativeFireballPosition(uint8_t slot);
    void RelativeMiscPosition(uint8_t slot);
    void RelativePlayerPosition();
    void RemBridge(uint8_t metatileGroupOfs4, uint8_t vramOffset, uint8_t nameTableLow, uint8_t nameTableHigh);
    void RemoveCoin_Axe();
    void RenderAreaGraphics();
    void RenderAttributeTables();
    void RenderPlayerSub(uint8_t rows);
    bool RenderSidewaysPipe(uint8_t areaObjBufferOffset, uint8_t verticalLength, uint8_t& outPipeDataIndex);
    uint8_t RenderUnderPart(uint8_t tile, uint8_t startCol, uint8_t numRows);
    void ReplaceBlockMetatile(uint8_t metatile, uint8_t blockOffset);
    uint8_t ResJmpM(uint8_t objectOffset, uint8_t cornerIdx);
    void ResetPalStar();
    void ResetScreenTimer();
    void ResetSpritesAndScreenTimer();
    void ResetTitle();
    void ResidualGravityCode(uint8_t objectOffset);
    void ResidualMiscObjectCode(uint8_t baseValue);
    void RightPlatform(uint8_t e);
    void RowOfBricks(uint8_t areaObjBufferOffset);
    void RowOfCoins(uint8_t areaObjBufferOffset);
    void RowOfSolidBlocks(uint8_t areaObjBufferOffset);
    void RunBowser(uint8_t e);
    void RunBowserFlame(uint8_t e);
    void RunDemo();
    void RunFirebarObj(uint8_t e);
    void RunFireworks(uint8_t e);
    void RunGameOver();
    void RunGameTimer();
    void RunLargePlatform(uint8_t e);
    void RunNormalEnemies(uint8_t e);
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
    uint8_t SetFlameTimer();
    uint8_t SetFreq_Squ1(uint8_t freqIndex);
    uint8_t SetFreq_Squ2(uint8_t freqIndex);
    uint8_t SetFreq_Tri(uint8_t freqIndex);
    void SetHJ(uint8_t verticalSpeed, uint8_t e);
    void SetHiMax(uint8_t e, uint8_t downwardMoveAmt);
    void SetKRout(uint8_t subroutineNum);
    void SetOffscrBitsOffset(uint8_t addend, uint8_t baseObjectOffset, uint8_t offscrArrayOffset);
    void SetPRout(uint8_t subroutineNum, uint8_t newPlayerState);
    void SetShim(uint8_t movingDir, uint8_t e);
    void SetStun(uint8_t eid);
    void SetVRAMAddr_A(uint8_t addrCtrl);
    void SetVRAMCtrl();
    void SetVRAMOffset(uint8_t newOffset);
    void SetXMoveAmt(uint8_t maxSpeed, uint8_t eid, uint8_t downwardMoveAmt);
    void SetupBubble(uint8_t slot);
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
    void Skip_6(uint8_t maxSpeedIdx, uint8_t objectOffset);
    void Skip_7(uint8_t moveDirection, uint8_t e);
    void Skip_8(uint8_t yCoord, uint8_t e);
    std::pair<uint8_t, uint8_t> Skip_9(uint8_t coordSelector, uint8_t cornerIdx);
    void SmallBBox(uint8_t e);
    void SmallPlatformBoundBox(uint8_t e);
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
    void StopPlayerMove(uint8_t side);
    void StopSquare1Sfx();
    void StopSquare2Sfx();
    void StoreMT(uint8_t terrainMetatile);
    bool SubtEnemyYPos(uint8_t e);
    void TallBBox(uint8_t e);
    void TallBBox2(uint8_t e);
    void TerminateGame();
    uint8_t ThreeFrameExtent(uint8_t baseIdx);
    void TitleScreenMode();
    void TopScoreCheck(uint8_t scoreOffset);
    bool TransposePlayers();
    void UpToFiery(uint8_t subroutineNum);
    void UpdScrollVar();
    void UpdateNumber(uint8_t statusBarNybbles);
    void UpdateScreen(uint16_t bufferAddr);
    void UpdateTopScore();
    void VariableObjOfsRelPos(uint8_t baseValue, uint8_t addend, uint8_t relPosIdx);
    void VerticalPipe(uint8_t areaObjBufferOffset);
    void VerticalPipeEntry();
    void VictoryMode();
    void VictoryModeSubroutines();
    void VineBlock();
    void VineObjectHandler(uint8_t e);
    void Vine_AutoClimb();
    void WarpZoneObject();
    void WaterPipe(uint8_t areaObjBufferOffset);
    void WriteBlockMetatile(uint8_t metatile);
    void WriteBottomStatusLine();
    void WriteGameText(uint8_t text_number);
    void WriteNTAddr(uint8_t highByte);
    void WritePPUReg1(uint8_t value);
    void WriteTopScore();
    void WriteTopStatusLine();
    void XMoveCntr_GreenPTroopa(uint8_t e);
    void XMoveCntr_Platform(uint8_t maxSecondary, uint8_t e);
    void XMovingPlatform(uint8_t e);
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
