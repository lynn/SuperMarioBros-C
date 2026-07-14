#ifndef SMBENGINE_HPP
#define SMBENGINE_HPP

#include <cstdint>
#include <cstddef>

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
    int returnIndexStack[100];   /**< Stack for managing JSR subroutines. */
    int returnIndexStackTop;     /**< Current index of the top of the call stack. */

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
     * inside code(), which is where these were lifted from. A call may be a
     * JSR, or a JMP that was a tail call all along.
     *
     * See SMB.cpp for implementations.
     */
    void AddToScore();
    void AlterAreaAttributes();
    void AlternateLengthHandler();
    void AnimationControl();
    void AreaFrenzy();
    void AreaStyleObject();
    void AxeObj();
    void BBChk_E();
    void BalancePlatRope();
    void BalancePlatform();
    void BlockBufferChk_Enemy();
    void BlockBufferChk_FBall();
    void BlockBufferColli_Feet();
    void BlockBufferColli_Head();
    void BlockBufferColli_Side();
    void BlockBufferCollision();
    bool BlockBumpedChk();
    void BlockObjMT_Updater();
    void BlockObjectsCore();
    void BoundingBoxCore();
    void BrickShatter();
    void Bridge_High();
    void Bridge_Low();
    void Bridge_Middle();
    void BulletBillCannon();
    void BumpBlock();
    void CastleBridgeObj();
    void CastleObject();
    void ChainObj();
    void CheckDefeatedState();
    bool CheckForClimbMTiles();
    bool CheckForCoinMTiles();
    void CheckForHammerBro();
    bool CheckForSolidMTiles();
    bool CheckPlayerVertical();
    void CheckRightScreenBBox();
    void CheckTopOfBlock();
    void ChkForLandJumpSpring();
    void ChkForNonSolids();
    void ChkForPlayerAttrib();
    void ChkInvisibleMTiles();
    bool ChkJumpspringMetatiles();
    void ChkLeftCo();
    bool ChkLrgObjFixedLength();
    bool ChkLrgObjLength();
    void ChkPOffscr();
    void ChkSmallPlatCollision();
    void ChkUnderEnemy();
    void ChkYPCollision();
    void ClearBuffersDrawIcon();
    void ClimbingSub();
    void CoinBlock();
    void ColObj();
    void ColorRotation();
    void ColumnOfBricks();
    void ColumnOfSolidBlocks();
    void CommonPlatCode();
    void CommonSmallLift();
    void CyclePlayerPalette();
    bool DemoEngine();
    void DestroyBlockMetatile();
    void DigitsMathRoutine();
    void DisplayIntermediate();
    void DisplayTimeUp();
    void DividePDiff();
    void DoOtherPlatform();
    void DonePlayerTask();
    void DrawBlock();
    void DrawBrickChunks();
    void DrawBubble();
    void DrawEnemyObjRow();
    void DrawEnemyObject();
    void DrawFireball();
    void DrawFirebar();
    void DrawHammer();
    void DrawMushroomIcon();
    void DrawOneSpriteRow();
    void DrawPlayerLoop();
    void DrawPlayer_Intermediate();
    void DrawPowerUp();
    void DrawQBlk();
    void DrawRope();
    void DrawRow();
    void DrawSmallPlatform();
    void DrawSpriteObject();
    void DrawStarFlag();
    void DrawVine();
    void DumpFourSpr();
    void DumpSixSpr();
    void DumpThreeSpr();
    void DumpTwoSpr();
    void Dump_Sq2_Regs();
    void Dump_Squ1_Regs();
    void EmptyBlock();
    void EndAreaPoints();
    void EndFrenzy();
    void EndlessRope();
    void EnemyFacePlayer();
    void EnemyGfxHandler();
    void EnemyLanding();
    void ErACM();
    void EraseEnemyObject();
    void ExecGameLoopback();
    void ExitPipe();
    void ExtraLifeMushBlock();
    void FindAreaPointer();
    bool FindEmptyEnemySlot();
    bool FindEmptyMiscSlot();
    void FindPlayerAction();
    void FireballBGCollision();
    void FirebarSpin();
    void FlagBalls_Residual();
    void FlagpoleObject();
    void FloateyNumbersRoutine();
    void GameTextLoop();
    void GetAreaDataAddrs();
    void GetAreaMusic();
    void GetAreaObjXPosition();
    void GetAreaObjYPosition();
    void GetAreaType();
    void GetBackgroundColor();
    void GetBlockBufferAddr();
    void GetBlockOffscreenBits();
    void GetBubbleOffscreenBits();
    void GetCurrentAnimOffset();
    void GetEnemyBoundBoxOfs();
    void GetEnemyBoundBoxOfsArg();
    void GetEnemyOffscreenBits();
    void GetFireballOffscreenBits();
    void GetFirebarPosition();
    void GetGfxOffsetAdder();
    void GetLrgObjAttrib();
    void GetMTileAttrib();
    void GetMiscOffscreenBits();
    void GetObjRelativePosition();
    void GetOffScreenBitsSet();
    void GetOffsetFromAnimCtrl();
    void GetPipeHeight();
    void GetPlayerAnimSpeed();
    void GetPlayerColors();
    void GetPlayerOffscreenBits();
    void GetProperObjOffset();
    void GetRow();
    void GetRow2();
    void GetSBNybbles();
    void GetScreenPosition();
    void GetXOffscreenBits();
    void GiveOneCoin();
    void GoContinue();
    void HandleChangeSize();
    void HandleCoinMetatile();
    void HandlePipeEntry();
    void Hole_Empty();
    void Hole_Water();
    void ImpedePlayerMove();
    void ImposeFriction();
    void ImposeGravity();
    void ImposeGravityBlock();
    void ImposeGravitySprObj();
    void Inc2B();
    void IncAreaObjOffset();
    void IncSubtask();
    void IncrementColumnPos();
    void InitBalPlatform();
    void InitBlock_XY_Pos();
    void InitBloober();
    void InitBulletBill();
    void InitCheepCheep();
    void InitDropPlatform();
    void InitFireworks();
    void InitFlyingCheepCheep();
    void InitGoomba();
    void InitHammerBro();
    void InitHoriPlatform();
    void InitHorizFlySwimEnemy();
    void InitJumpGPTroopa();
    void InitLakitu();
    void InitNTLoop();
    void InitNormalEnemy();
    void InitPiranhaPlant();
    void InitPodoboo();
    void InitRedKoopa();
    void InitRedPTroopa();
    void InitRetainerObj();
    void InitScroll();
    void InitShortFirebar();
    void InitVStf();
    void InitVertPlatform();
    void InitializeArea();
    void InitializeGame();
    void InitializeMemory();
    void InitializeNameTables();
    void IntroPipe();
    void JCoinC();
    void JumpEngine();
    void Jumpspring();
    void JumpspringHandler();
    void KillEnemies();
    void LargeLiftBBox();
    void LargeLiftDown();
    void LargeLiftUp();
    void LoadAreaPointer();
    void LoadControlRegs();
    void LoadEnvelopeData();
    void MoveAllSpritesOffscreen();
    void MoveColOffscreen();
    void MoveD_EnemyVertically();
    void MoveESprColOffscreen();
    void MoveESprRowOffscreen();
    void MoveEnemyHorizontally();
    void MoveFallingPlatform();
    void MoveFlyGreenPTroopa();
    void MoveLakitu();
    void MoveLargeLiftPlat();
    void MoveLiftPlatforms();
    void MoveObjectHorizontally();
    void MovePiranhaPlant();
    void MovePlatformDown();
    void MovePlatformUp();
    void MovePlayerHorizontally();
    void MovePlayerVertically();
    void MovePlayerYAxis();
    void MoveRedPTroopa();
    void MoveRedPTroopaDown();
    void MoveSmallPlatform();
    void MoveSpritesOffscreen();
    void MoveVOffset();
    void MoveWithXMCntrs();
    void MushFlowerBlock();
    void OffscreenBoundsCheck();
    void OnGroundStateSub();
    void OutputInter();
    void OutputNumbers();
    void PauseRoutine();
    void PlatLiftDown();
    void PlatLiftUp();
    void PlayerBGCollision();
    bool PlayerCollisionCore();
    bool PlayerEnemyDiff();
    void PlayerFireFlower();
    void PlayerGfxHandler();
    void PlayerGfxProcessing();
    void PlayerHeadCollision();
    void PlayerLakituDiff();
    void PlayerMovementSubs();
    void PlayerOffscreenChk();
    void PlayerPhysicsSub();
    void PosPlatform();
    void PositionPlayerOnS_Plat();
    void PositionPlayerOnVPlat();
    void PrimaryGameSetup();
    void PrintStatusBarNumbers();
    void ProcLPlatCollisions();
    void ProcMoveRedPTroopa();
    void ProcSwimmingB(bool blooberCarry);
    void ProcessLengthData();
    void ProcessPlayerAction();
    void ProcessWhirlpools();
    void PulleyRopeObject();
    void PutBlockMetatile();
    void PutPlayerOnVine();
    void PwrUpJmp();
    void QuestionBlockRow_High();
    void QuestionBlockRow_Low();
    void ReadJoypads();
    void ReadPortBits();
    void RedPTroopaGrav();
    void RelWOfs();
    void RelativeBlockPosition();
    void RelativeBubblePosition();
    void RelativeEnemyPosition();
    void RelativeFireballPosition();
    void RelativeMiscPosition();
    void RelativePlayerPosition();
    void RemBridge();
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
    void ResidualGravityCode();
    void ResidualMiscObjectCode();
    void RowOfBricks();
    void RowOfCoins();
    void RowOfSolidBlocks();
    void RunOffscrBitsSubs();
    void RunRetainerObj();
    void RunStarFlagObj();
    void SPBBox();
    void ScrollHandler();
    void ScrollLockObject();
    void ScrollLockObject_Warp();
    void ScrollScreen();
    void SecondaryGameSetup();
    void SetBBox();
    void SetBBox2();
    void SetESpd();
    void SetHiMax();
    void SetOffscrBitsOffset();
    void SetVRAMCtrl();
    void SetVRAMOffset();
    void SetXMoveAmt();
    void SetupEOffsetFBBox();
    void SetupFloateyNumber();
    void SetupGameOver();
    void SetupIntermediate();
    void SetupJumpCoin();
    void SetupLakitu();
    void SetupPlatformRope();
    void SetupPowerUp();
    void SetupVictoryMode();
    void Setup_Vine();
    void SixSpriteStacker();
    void Skip_0();
    void Skip_1();
    void Skip_2();
    void Skip_3();
    void Skip_4();
    void Skip_5();
    void Skip_6();
    void Skip_7();
    void Skip_8();
    void Skip_9();
    void SmallBBox();
    void SmallPlatformCollision();
    void SpawnBrickChunks();
    bool SpawnHammerObj();
    bool SprObjectCollisionCore();
    void SprObjectOffscrChk();
    void SpriteShuffler();
    void StaircaseObject();
    void StarBlock();
    void StopPlatforms();
    void StopPlayerMove();
    bool SubtEnemyYPos();
    void TallBBox();
    void TallBBox2();
    void ThreeFrameExtent();
    void TopScoreCheck();
    bool TransposePlayers();
    void UpdateNumber();
    void UpdateScreen();
    void UpdateTopScore();
    void VariableObjOfsRelPos();
    void VerticalPipe();
    void VineBlock();
    void VineObjectHandler();
    void WarpZoneObject();
    void WaterPipe();
    void WriteBlockMetatile();
    void WriteBottomStatusLine();
    void WriteGameText();
    void WriteNTAddr();
    void WritePPUReg1();
    void WriteTopScore();
    void WriteTopStatusLine();
    void XMoveCntr_GreenPTroopa();
    void XMoveCntr_Platform();
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
     * Pop an index from the call stack.
     */
    int popReturnIndex();

    /**
     * Push an index to the call stack.
     */
    void pushReturnIndex(int index);

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
