#ifndef SMBENGINE_HPP
#define SMBENGINE_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <tuple>
#include <utility>

#include "../Emulation/APU.hpp"
#include "../Emulation/Controller.hpp"
#include "../Emulation/PPU.hpp"
#include "SMBDataPointers.hpp"

class APU;
class Controller;
class PPU;

/**
 * What a block buffer collision check found, shared by BlockBufferCollision and
 * the whole family of wrappers around it.
 */
struct BlockBufferResult
{
    uint8_t metatile;   ///< the metatile found at that block-buffer position
    uint8_t coordinate; ///< the selected coordinate's low nybble
    uint8_t row;        ///< vertical high nybble offset into the block buffer
    uint16_t address;   ///< address of the block buffer column the lookup used
};

/**
 * One cell of the block buffer: a column address and a row within it. The block
 * handling routines pass this pair from one to the next all the way down to
 * PutBlockMetatile, which is why it travels as a unit. The row moves while the
 * column stays put -- CheckTopOfBlock walks it up one row -- so this is the same
 * column seen at different heights, not a fixed location.
 */
struct BlockBufferCell
{
    uint8_t row;      ///< vertical high nybble offset into the block buffer
    uint16_t address; ///< address of the block buffer column
};

/**
 * What ChkLrgObjLength() found: the attributes of a large area object, plus
 * whether this is the column on which it starts. Several of the render routines
 * want only one or two of the three, so the unused ones are simply not read.
 */
struct LrgObjLength
{
    bool justStarted; ///< whether the object's length counter was not yet set
    uint8_t length;   ///< length/height nybble
    uint8_t row;      ///< row location of the object
};

/**
 * The sprite-drawing values EnemyGfxHandler stages for the routines it flows
 * through, all the way down to DrawEnemyObjRow. The vertical position is
 * nudged repeatedly along the way, which is why this travels by reference.
 */
struct EnemyGfxState
{
    uint8_t yPos;       ///< vertical position of the object
    uint8_t flipBits;   ///< flip control, from the enemy's moving direction
    uint8_t attributes; ///< sprite attributes
    uint8_t xPos;       ///< horizontal position, relative to the screen
};

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
    SMBEngine(uint8_t *romImage, bool enableAudio = true);

    ~SMBEngine();

    /**
     * Callback for handling audio buffering.
     */
    void audioCallback(uint8_t *stream, int length);

    /**
     * Get player 1's controller.
     */
    Controller &getController1();

    /**
     * Get player 2's controller.
     */
    Controller &getController2();

    /**
     * Get the NES RAM, RAM_SIZE bytes of it.
     */
    const uint8_t *getRam() const;

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
    void render(uint32_t *buffer);

    /**
     * Render the whole background the PPU holds, scroll ignored, for debugging.
     *
     * @param buffer a 512x480 32-bit color buffer for storing the rendering.
     * @param scrollX set to the left edge of the region render() draws.
     * @param scrollY set to the top edge of the region render() draws.
     */
    void renderNametables(uint32_t *buffer, int &scrollX, int &scrollY);

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
    bool audioEnabled; /**< Whether the APU is run at all. */

    // NES Emulation subsystems:
    APU *apu;
    PPU *ppu;
    Controller *controller1;
    Controller *controller2;

    // Fields for NES CPU emulation:
    uint8_t dataStorage[0x8000]; /**< 32kb of storage for constant data. */
    uint8_t ram[RAM_SIZE];       /**< 2kb of RAM. */
    uint8_t *chr;                /**< Pointer to CHR data from the ROM. */

    uint8_t &joypadOverride_;        /**< Alias for ram[JoypadOverride]. */
    uint8_t &a_B_Buttons_;           /**< Alias for ram[A_B_Buttons]. */
    uint8_t &previousA_B_Buttons_;   /**< Alias for ram[PreviousA_B_Buttons]. */
    uint8_t &up_Down_Buttons_;       /**< Alias for ram[Up_Down_Buttons]. */
    uint8_t &left_Right_Buttons_;    /**< Alias for ram[Left_Right_Buttons]. */
    uint8_t &gameEngineSubroutine_;  /**< Alias for ram[GameEngineSubroutine]. */
    uint8_t &objectOffset_;          /**< Alias for ram[ObjectOffset]. */
    uint8_t &frameCounter_;          /**< Alias for ram[FrameCounter]. */
    uint8_t &savedJoypadBits_;       /**< Alias for ram[SavedJoypadBits]. */
    uint8_t &savedJoypad1Bits_;      /**< Alias for ram[SavedJoypad1Bits]. */
    uint8_t &savedJoypad2Bits_;      /**< Alias for ram[SavedJoypad2Bits]. */
    uint8_t &mirror_PPU_CTRL_REG1_;  /**< Alias for ram[Mirror_PPU_CTRL_REG1]. */
    uint8_t &mirror_PPU_CTRL_REG2_;  /**< Alias for ram[Mirror_PPU_CTRL_REG2]. */
    uint8_t &operMode_;              /**< Alias for ram[OperMode]. */
    uint8_t &operMode_Task_;         /**< Alias for ram[OperMode_Task]. */
    uint8_t &screenRoutineTask_;     /**< Alias for ram[ScreenRoutineTask]. */
    uint8_t &gamePauseStatus_;       /**< Alias for ram[GamePauseStatus]. */
    uint8_t &gamePauseTimer_;        /**< Alias for ram[GamePauseTimer]. */
    uint8_t &demoAction_;            /**< Alias for ram[DemoAction]. */
    uint8_t &demoActionTimer_;       /**< Alias for ram[DemoActionTimer]. */
    uint8_t timerControl_;           /**< Alias for ram[TimerControl]. */
    uint8_t intervalTimerControl_;   /**< Alias for ram[IntervalTimerControl]. */
    uint8_t &selectTimer_;           /**< Alias for ram[SelectTimer]. */
    uint8_t &playerAnimTimer_;       /**< Alias for ram[PlayerAnimTimer]. */
    uint8_t &jumpSwimTimer_;         /**< Alias for ram[JumpSwimTimer]. */
    uint8_t &runningTimer_;          /**< Alias for ram[RunningTimer]. */
    uint8_t &blockBounceTimer_;      /**< Alias for ram[BlockBounceTimer]. */
    uint8_t &sideCollisionTimer_;    /**< Alias for ram[SideCollisionTimer]. */
    uint8_t &jumpspringTimer_;       /**< Alias for ram[JumpspringTimer]. */
    uint8_t &gameTimerCtrlTimer_;    /**< Alias for ram[GameTimerCtrlTimer]. */
    uint8_t &climbSideTimer_;        /**< Alias for ram[ClimbSideTimer]. */
    uint8_t &frenzyEnemyTimer_;      /**< Alias for ram[FrenzyEnemyTimer]. */
    uint8_t &bowserFireBreathTimer_; /**< Alias for ram[BowserFireBreathTimer]. */
    uint8_t &stompTimer_;            /**< Alias for ram[StompTimer]. */
    uint8_t &airBubbleTimer_;        /**< Alias for ram[AirBubbleTimer]. */
    uint8_t &scrollIntervalTimer_;   /**< Alias for ram[ScrollIntervalTimer]. */
    uint8_t &brickCoinTimer_;        /**< Alias for ram[BrickCoinTimer]. */
    uint8_t &injuryTimer_;           /**< Alias for ram[InjuryTimer]. */
    uint8_t &starInvincibleTimer_;   /**< Alias for ram[StarInvincibleTimer]. */
    uint8_t &screenTimer_;           /**< Alias for ram[ScreenTimer]. */
    uint8_t &worldEndTimer_;         /**< Alias for ram[WorldEndTimer]. */
    uint8_t &demoTimer_;             /**< Alias for ram[DemoTimer]. */
    uint8_t &screenLeft_PageLoc_;    /**< Alias for ram[ScreenLeft_PageLoc]. */
    uint8_t &screenRight_PageLoc_;   /**< Alias for ram[ScreenRight_PageLoc]. */
    uint8_t &screenLeft_X_Pos_;      /**< Alias for ram[ScreenLeft_X_Pos]. */
    uint8_t &screenRight_X_Pos_;     /**< Alias for ram[ScreenRight_X_Pos]. */
    uint8_t &playerFacingDir_;       /**< Alias for ram[PlayerFacingDir]. */
    uint8_t &destinationPageLoc_;    /**< Alias for ram[DestinationPageLoc]. */
    uint8_t &victoryWalkControl_;    /**< Alias for ram[VictoryWalkControl]. */
    uint8_t &scrollFractional_;      /**< Alias for ram[ScrollFractional]. */
    uint8_t &primaryMsgCounter_;     /**< Alias for ram[PrimaryMsgCounter]. */
    uint8_t &secondaryMsgCounter_;   /**< Alias for ram[SecondaryMsgCounter]. */
    uint8_t &horizontalScroll_;      /**< Alias for ram[HorizontalScroll]. */
    uint8_t &verticalScroll_;        /**< Alias for ram[VerticalScroll]. */
    uint8_t &scrollLock_;            /**< Alias for ram[ScrollLock]. */
    uint8_t &scrollThirtyTwo_;       /**< Alias for ram[ScrollThirtyTwo]. */
    uint8_t &player_X_Scroll_;       /**< Alias for ram[Player_X_Scroll]. */
    uint8_t &player_Pos_ForScroll_;  /**< Alias for ram[Player_Pos_ForScroll]. */
    uint8_t &scrollAmount_;          /**< Alias for ram[ScrollAmount]. */
    uint8_t &areaDataLow_;           /**< Alias for ram[AreaDataLow]. */
    uint8_t &areaDataHigh_;          /**< Alias for ram[AreaDataHigh]. */
    uint8_t &enemyDataLow_;          /**< Alias for ram[EnemyDataLow]. */
    uint8_t &enemyDataHigh_;         /**< Alias for ram[EnemyDataHigh]. */
    uint8_t &areaParserTaskNum_;     /**< Alias for ram[AreaParserTaskNum]. */
    uint8_t &columnSets_;            /**< Alias for ram[ColumnSets]. */
    uint8_t &currentPageLoc_;        /**< Alias for ram[CurrentPageLoc]. */
    uint8_t &currentColumnPos_;      /**< Alias for ram[CurrentColumnPos]. */
    uint8_t &backloadingFlag_;       /**< Alias for ram[BackloadingFlag]. */
    uint8_t &behindAreaParserFlag_;  /**< Alias for ram[BehindAreaParserFlag]. */
    uint8_t &areaObjectPageLoc_;     /**< Alias for ram[AreaObjectPageLoc]. */
    uint8_t &areaObjectPageSel_;     /**< Alias for ram[AreaObjectPageSel]. */
    uint8_t &areaDataOffset_;        /**< Alias for ram[AreaDataOffset]. */
    uint8_t &areaObjectLength_;      /**< Alias for ram[AreaObjectLength]. */
    uint8_t &staircaseControl_;      /**< Alias for ram[StaircaseControl]. */
    uint8_t &areaObjectHeight_;      /**< Alias for ram[AreaObjectHeight]. */
    uint8_t &enemyDataOffset_;       /**< Alias for ram[EnemyDataOffset]. */
    uint8_t &enemyObjectPageLoc_;    /**< Alias for ram[EnemyObjectPageLoc]. */
    uint8_t &enemyObjectPageSel_;    /**< Alias for ram[EnemyObjectPageSel]. */
    uint8_t &metatileBuffer_;        /**< Alias for ram[MetatileBuffer]. */
    uint8_t &blockBufferColumnPos_;  /**< Alias for ram[BlockBufferColumnPos]. */
    uint8_t &currentNTAddr_Low_;     /**< Alias for ram[CurrentNTAddr_Low]. */
    uint8_t &currentNTAddr_High_;    /**< Alias for ram[CurrentNTAddr_High]. */
    uint8_t &loopCommand_;           /**< Alias for ram[LoopCommand]. */
    uint8_t &gameTimerDisplay_;      /**< Alias for ram[GameTimerDisplay]. */
    uint8_t &verticalFlipFlag_;      /**< Alias for ram[VerticalFlipFlag]. */
    uint8_t &flagpoleFNum_Y_Pos_;    /**< Alias for ram[FlagpoleFNum_Y_Pos]. */
    uint8_t &flagpoleFNum_YMFDummy_; /**< Alias for ram[FlagpoleFNum_YMFDummy]. */
    uint8_t &flagpoleScore_;         /**< Alias for ram[FlagpoleScore]. */
    uint8_t &flagpoleCollisionYPos_; /**< Alias for ram[FlagpoleCollisionYPos]. */
    uint8_t &stompChainCounter_;     /**< Alias for ram[StompChainCounter]. */
    uint8_t &vRAM_Buffer1_Offset_;   /**< Alias for ram[VRAM_Buffer1_Offset]. */
    uint8_t &vRAM_Buffer1_;          /**< Alias for ram[VRAM_Buffer1]. */
    uint8_t &vRAM_Buffer2_Offset_;   /**< Alias for ram[VRAM_Buffer2_Offset]. */
    uint8_t &vRAM_Buffer_AddrCtrl_;  /**< Alias for ram[VRAM_Buffer_AddrCtrl]. */
    uint8_t &sprite0HitDetectFlag_;  /**< Alias for ram[Sprite0HitDetectFlag]. */
    uint8_t &disableScreenFlag_;     /**< Alias for ram[DisableScreenFlag]. */
    uint8_t &disableIntermediate_;   /**< Alias for ram[DisableIntermediate]. */
    uint8_t &colorRotateOffset_;     /**< Alias for ram[ColorRotateOffset]. */
    uint8_t &terrainControl_;        /**< Alias for ram[TerrainControl]. */
    uint8_t &areaStyle_;             /**< Alias for ram[AreaStyle]. */
    uint8_t &foregroundScenery_;     /**< Alias for ram[ForegroundScenery]. */
    uint8_t &backgroundScenery_;     /**< Alias for ram[BackgroundScenery]. */
    uint8_t &cloudTypeOverride_;     /**< Alias for ram[CloudTypeOverride]. */
    uint8_t &backgroundColorCtrl_;   /**< Alias for ram[BackgroundColorCtrl]. */
    uint8_t &areaType_;              /**< Alias for ram[AreaType]. */
    uint8_t &areaAddrsLOffset_;      /**< Alias for ram[AreaAddrsLOffset]. */
    uint8_t &areaPointer_;           /**< Alias for ram[AreaPointer]. */
    uint8_t &playerEntranceCtrl_;    /**< Alias for ram[PlayerEntranceCtrl]. */
    uint8_t &gameTimerSetting_;      /**< Alias for ram[GameTimerSetting]. */
    uint8_t &altEntranceControl_;    /**< Alias for ram[AltEntranceControl]. */
    uint8_t &entrancePage_;          /**< Alias for ram[EntrancePage]. */
    uint8_t &numberOfPlayers_;       /**< Alias for ram[NumberOfPlayers]. */
    uint8_t &warpZoneControl_;       /**< Alias for ram[WarpZoneControl]. */
    uint8_t &changeAreaTimer_;       /**< Alias for ram[ChangeAreaTimer]. */
    uint8_t &multiLoopCorrectCntr_;  /**< Alias for ram[MultiLoopCorrectCntr]. */
    uint8_t &multiLoopPassCntr_;     /**< Alias for ram[MultiLoopPassCntr]. */
    uint8_t &fetchNewGameTimerFlag_; /**< Alias for ram[FetchNewGameTimerFlag]. */
    uint8_t &gameTimerExpiredFlag_;  /**< Alias for ram[GameTimerExpiredFlag]. */
    uint8_t &primaryHardMode_;       /**< Alias for ram[PrimaryHardMode]. */
    uint8_t &secondaryHardMode_;     /**< Alias for ram[SecondaryHardMode]. */
    uint8_t &worldSelectNumber_;     /**< Alias for ram[WorldSelectNumber]. */
    uint8_t &worldSelectEnableFlag_; /**< Alias for ram[WorldSelectEnableFlag]. */
    uint8_t &continueWorld_;         /**< Alias for ram[ContinueWorld]. */
    uint8_t &currentPlayer_;         /**< Alias for ram[CurrentPlayer]. */
    uint8_t &playerSize_;            /**< Alias for ram[PlayerSize]. */
    uint8_t &playerStatus_;          /**< Alias for ram[PlayerStatus]. */
    uint8_t &numberofLives_;         /**< Alias for ram[NumberofLives]. */
    uint8_t &halfwayPage_;           /**< Alias for ram[HalfwayPage]. */
    uint8_t &levelNumber_;           /**< Alias for ram[LevelNumber]. */
    uint8_t &hidden1UpFlag_;         /**< Alias for ram[Hidden1UpFlag]. */
    uint8_t &coinTally_;             /**< Alias for ram[CoinTally]. */
    uint8_t &worldNumber_;           /**< Alias for ram[WorldNumber]. */
    uint8_t &areaNumber_;            /**< Alias for ram[AreaNumber]. */
    uint8_t &coinTallyFor1Ups_;      /**< Alias for ram[CoinTallyFor1Ups]. */
    uint8_t &offScr_NumberofLives_;  /**< Alias for ram[OffScr_NumberofLives]. */
    uint8_t &offScr_Hidden1UpFlag_;  /**< Alias for ram[OffScr_Hidden1UpFlag]. */
    uint8_t &offScr_WorldNumber_;    /**< Alias for ram[OffScr_WorldNumber]. */
    uint8_t &offScr_AreaNumber_;     /**< Alias for ram[OffScr_AreaNumber]. */
    uint8_t &balPlatformAlignment_;  /**< Alias for ram[BalPlatformAlignment]. */
    uint8_t &platform_X_Scroll_;     /**< Alias for ram[Platform_X_Scroll]. */
    uint8_t &brickCoinTimerFlag_;    /**< Alias for ram[BrickCoinTimerFlag]. */
    uint8_t &starFlagTaskControl_;   /**< Alias for ram[StarFlagTaskControl]. */
    uint8_t &pseudoRandomBitReg_;    /**< Alias for ram[PseudoRandomBitReg]. */
    uint8_t &warmBootValidation_;    /**< Alias for ram[WarmBootValidation]. */
    uint8_t &sprShuffleAmtOffset_;   /**< Alias for ram[SprShuffleAmtOffset]. */
    uint8_t &sprShuffleAmt_;         /**< Alias for ram[SprShuffleAmt]. */
    uint8_t &player_SprDataOffset_;  /**< Alias for ram[Player_SprDataOffset]. */
    uint8_t &sprDataOffset_Ctrl_;    /**< Alias for ram[SprDataOffset_Ctrl]. */
    uint8_t &player_State_;          /**< Alias for ram[Player_State]. */
    uint8_t &player_MovingDir_;      /**< Alias for ram[Player_MovingDir]. */
    uint8_t &enemy_MovingDir_;       /**< Alias for ram[Enemy_MovingDir]. */
    uint8_t &player_X_Speed_;        /**< Alias for ram[Player_X_Speed]. */
    uint8_t &jumpspringAnimCtrl_;    /**< Alias for ram[JumpspringAnimCtrl]. */
    uint8_t &jumpspringForce_;       /**< Alias for ram[JumpspringForce]. */
    uint8_t &player_PageLoc_;        /**< Alias for ram[Player_PageLoc]. */
    uint8_t &player_X_Position_;     /**< Alias for ram[Player_X_Position]. */
    uint8_t &player_Y_Speed_;        /**< Alias for ram[Player_Y_Speed]. */
    uint8_t &player_Y_HighPos_;      /**< Alias for ram[Player_Y_HighPos]. */
    uint8_t &player_Y_Position_;     /**< Alias for ram[Player_Y_Position]. */
    uint8_t &block_Y_Position_;      /**< Alias for ram[Block_Y_Position]. */
    uint8_t &player_Rel_XPos_;       /**< Alias for ram[Player_Rel_XPos]. */
    uint8_t &enemy_Rel_XPos_;        /**< Alias for ram[Enemy_Rel_XPos]. */
    uint8_t &fireball_Rel_XPos_;     /**< Alias for ram[Fireball_Rel_XPos]. */
    uint8_t &bubble_Rel_XPos_;       /**< Alias for ram[Bubble_Rel_XPos]. */
    uint8_t &block_Rel_XPos_;        /**< Alias for ram[Block_Rel_XPos]. */
    uint8_t &misc_Rel_XPos_;         /**< Alias for ram[Misc_Rel_XPos]. */
    uint8_t &player_Rel_YPos_;       /**< Alias for ram[Player_Rel_YPos]. */
    uint8_t &enemy_Rel_YPos_;        /**< Alias for ram[Enemy_Rel_YPos]. */
    uint8_t &fireball_Rel_YPos_;     /**< Alias for ram[Fireball_Rel_YPos]. */
    uint8_t &bubble_Rel_YPos_;       /**< Alias for ram[Bubble_Rel_YPos]. */
    uint8_t &block_Rel_YPos_;        /**< Alias for ram[Block_Rel_YPos]. */
    uint8_t &misc_Rel_YPos_;         /**< Alias for ram[Misc_Rel_YPos]. */
    uint8_t &player_SprAttrib_;      /**< Alias for ram[Player_SprAttrib]. */
    uint8_t &player_YMF_Dummy_;      /**< Alias for ram[Player_YMF_Dummy]. */
    uint8_t &player_Y_MoveForce_;    /**< Alias for ram[Player_Y_MoveForce]. */
    uint8_t &disableCollisionDet_;   /**< Alias for ram[DisableCollisionDet]. */
    uint8_t &player_CollisionBits_;  /**< Alias for ram[Player_CollisionBits]. */
    uint8_t &player_BoundBoxCtrl_;   /**< Alias for ram[Player_BoundBoxCtrl]. */
    uint8_t &enemyFrenzyBuffer_;     /**< Alias for ram[EnemyFrenzyBuffer]. */
    uint8_t &enemyFrenzyQueue_;      /**< Alias for ram[EnemyFrenzyQueue]. */
    uint8_t &playerGfxOffset_;       /**< Alias for ram[PlayerGfxOffset]. */
    uint8_t &player_XSpeedAbsolute_; /**< Alias for ram[Player_XSpeedAbsolute]. */
    uint8_t &frictionAdderHigh_;     /**< Alias for ram[FrictionAdderHigh]. */
    uint8_t &frictionAdderLow_;      /**< Alias for ram[FrictionAdderLow]. */
    uint8_t &runningSpeed_;          /**< Alias for ram[RunningSpeed]. */
    uint8_t &swimmingFlag_;          /**< Alias for ram[SwimmingFlag]. */
    uint8_t &player_X_MoveForce_;    /**< Alias for ram[Player_X_MoveForce]. */
    uint8_t &diffToHaltJump_;        /**< Alias for ram[DiffToHaltJump]. */
    uint8_t &jumpOrigin_Y_HighPos_;  /**< Alias for ram[JumpOrigin_Y_HighPos]. */
    uint8_t &jumpOrigin_Y_Position_; /**< Alias for ram[JumpOrigin_Y_Position]. */
    uint8_t &verticalForce_;         /**< Alias for ram[VerticalForce]. */
    uint8_t &verticalForceDown_;     /**< Alias for ram[VerticalForceDown]. */
    uint8_t &playerChangeSizeFlag_;  /**< Alias for ram[PlayerChangeSizeFlag]. */
    uint8_t &playerAnimTimerSet_;    /**< Alias for ram[PlayerAnimTimerSet]. */
    uint8_t &playerAnimCtrl_;        /**< Alias for ram[PlayerAnimCtrl]. */
    uint8_t &deathMusicLoaded_;      /**< Alias for ram[DeathMusicLoaded]. */
    uint8_t &flagpoleSoundQueue_;    /**< Alias for ram[FlagpoleSoundQueue]. */
    uint8_t &crouchingFlag_;         /**< Alias for ram[CrouchingFlag]. */
    uint8_t &maximumLeftSpeed_;      /**< Alias for ram[MaximumLeftSpeed]. */
    uint8_t &maximumRightSpeed_;     /**< Alias for ram[MaximumRightSpeed]. */
    uint8_t &player_OffscreenBits_;  /**< Alias for ram[Player_OffscreenBits]. */
    uint8_t &enemy_OffscreenBits_;   /**< Alias for ram[Enemy_OffscreenBits]. */
    uint8_t &fBall_OffscreenBits_;   /**< Alias for ram[FBall_OffscreenBits]. */
    uint8_t &bubble_OffscreenBits_;  /**< Alias for ram[Bubble_OffscreenBits]. */
    uint8_t &block_OffscreenBits_;   /**< Alias for ram[Block_OffscreenBits]. */
    uint8_t &misc_OffscreenBits_;    /**< Alias for ram[Misc_OffscreenBits]. */
    uint8_t &cannon_Offset_;         /**< Alias for ram[Cannon_Offset]. */
    uint8_t &whirlpool_Offset_;      /**< Alias for ram[Whirlpool_Offset]. */
    uint8_t &whirlpool_Flag_;        /**< Alias for ram[Whirlpool_Flag]. */
    uint8_t &vineFlagOffset_;        /**< Alias for ram[VineFlagOffset]. */
    uint8_t &vineHeight_;            /**< Alias for ram[VineHeight]. */
    uint8_t &vineStart_Y_Position_;  /**< Alias for ram[VineStart_Y_Position]. */
    uint8_t &block_ResidualCounter_; /**< Alias for ram[Block_ResidualCounter]. */
    uint8_t &boundingBox_UL_XPos_;   /**< Alias for ram[BoundingBox_UL_XPos]. */
    uint8_t &boundingBox_UL_YPos_;   /**< Alias for ram[BoundingBox_UL_YPos]. */
    uint8_t &boundingBox_DR_XPos_;   /**< Alias for ram[BoundingBox_DR_XPos]. */
    uint8_t &boundingBox_DR_YPos_;   /**< Alias for ram[BoundingBox_DR_YPos]. */
    uint8_t &powerUpType_;           /**< Alias for ram[PowerUpType]. */
    uint8_t &fireballCounter_;       /**< Alias for ram[FireballCounter]. */
    uint8_t &fireballThrowingTimer_; /**< Alias for ram[FireballThrowingTimer]. */
    uint8_t &jumpCoinMiscOffset_;    /**< Alias for ram[JumpCoinMiscOffset]. */
    uint8_t &bitMFilter_;            /**< Alias for ram[BitMFilter]. */
    uint8_t &lakituReappearTimer_;   /**< Alias for ram[LakituReappearTimer]. */
    uint8_t &duplicateObj_Offset_;   /**< Alias for ram[DuplicateObj_Offset]. */
    uint8_t &numberofGroupEnemies_;  /**< Alias for ram[NumberofGroupEnemies]. */
    uint8_t &bowserBodyControls_;    /**< Alias for ram[BowserBodyControls]. */
    uint8_t &bowserFeetCounter_;     /**< Alias for ram[BowserFeetCounter]. */
    uint8_t &bowserMovementSpeed_;   /**< Alias for ram[BowserMovementSpeed]. */
    uint8_t &bowserOrigXPos_;        /**< Alias for ram[BowserOrigXPos]. */
    uint8_t &bowserFlameTimerCtrl_;  /**< Alias for ram[BowserFlameTimerCtrl]. */
    uint8_t &bowserFront_Offset_;    /**< Alias for ram[BowserFront_Offset]. */
    uint8_t &bridgeCollapseOffset_;  /**< Alias for ram[BridgeCollapseOffset]. */
    uint8_t &bowserGfxFlag_;         /**< Alias for ram[BowserGfxFlag]. */
    uint8_t &bowserHitPoints_;       /**< Alias for ram[BowserHitPoints]. */
    uint8_t &maxRangeFromOrigin_;    /**< Alias for ram[MaxRangeFromOrigin]. */
    uint8_t &fireworksCounter_;      /**< Alias for ram[FireworksCounter]. */

    uint8_t &squ2_NoteLenBuffer_;    /**< 0x07b3 */
    uint8_t &squ2_NoteLenCounter_;   /**< 0x07b4 */
    uint8_t &squ2_EnvelopeDataCtrl_; /**< 0x07b5 */
    uint8_t &squ1_NoteLenCounter_;   /**< 0x07b6 */
    uint8_t &squ1_EnvelopeDataCtrl_; /**< 0x07b7 */
    uint8_t &tri_NoteLenBuffer_;     /**< 0x07b8 */
    uint8_t &tri_NoteLenCounter_;    /**< 0x07b9 */
    uint8_t &noise_BeatLenCounter_;  /**< 0x07ba */
    uint8_t &squ1_SfxLenCounter_;    /**< 0x07bb */
    uint8_t &squ2_SfxLenCounter_;    /**< 0x07bd */
    uint8_t &sfx_SecondaryCounter_;  /**< 0x07be */
    uint8_t &noise_SfxLenCounter_;   /**< 0x07bf */

    uint8_t pauseSoundQueue_;    /**< 0xfa */
    uint8_t square1SoundQueue_;  /**< 0xff */
    uint8_t square2SoundQueue_;  /**< 0xfe */
    uint8_t noiseSoundQueue_;    /**< 0xfd */
    uint8_t areaMusicQueue_;     /**< 0xfb */
    uint8_t eventMusicQueue_;    /**< 0xfc */
    uint8_t square1SoundBuffer_; /**< 0xf1 */
    uint8_t square2SoundBuffer_; /**< 0xf2 */
    uint8_t noiseSoundBuffer_;   /**< 0xf3 */
    uint8_t areaMusicBuffer_;    /**< 0xf4 */
    uint8_t eventMusicBuffer_;   /**< 0x07b1 */
    uint8_t pauseSoundBuffer_;   /**< 0x07b2 */

    uint8_t &musicOffset_Square2_;  /**< Alias for ram[MusicOffset_Square2]. */
    uint8_t &musicOffset_Square1_;  /**< Alias for ram[MusicOffset_Square1]. */
    uint8_t &musicOffset_Triangle_; /**< Alias for ram[MusicOffset_Triangle]. */
    uint8_t &musicOffset_Noise_;    /**< Alias for ram[MusicOffset_Noise]. */

    uint8_t &noteLenLookupTblOfs_;  /**< Alias for ram[NoteLenLookupTblOfs]. */
    uint8_t &dAC_Counter_;          /**< Alias for ram[DAC_Counter]. */
    uint8_t &noiseDataLoopbackOfs_; /**< Alias for ram[NoiseDataLoopbackOfs]. */
    uint8_t &noteLengthTblAdder_;   /**< Alias for ram[NoteLengthTblAdder]. */
    uint8_t &areaMusicBuffer_Alt_;  /**< Alias for ram[AreaMusicBuffer_Alt]. */
    uint8_t &pauseModeFlag_;        /**< Alias for ram[PauseModeFlag]. */
    uint8_t &groundMusicHeaderOfs_; /**< Alias for ram[GroundMusicHeaderOfs]. */
    uint8_t &altRegContentFlag_;    /**< Alias for ram[AltRegContentFlag]. */

    // state! wow!
    const uint8_t *musicData;

    /**
     * What saveState() keeps, defined in SMBEngine.cpp so that this header does not
     * need the PPU's. Null until the first save.
     */
    struct State;
    State *savedState;

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
    void AreaFrenzy(uint8_t objectId);
    void AreaParserCore();
    void AreaParserTaskControl();
    void AreaParserTaskHandler();
    void AreaParserTasks(uint8_t taskNum);
    void AreaStyleObject(uint8_t areaObjBufferOffset);
    void TreeLedge(uint8_t areaObjBufferOffset);
    void MushroomLedge(uint8_t areaObjBufferOffset);
    void AutoControlPlayer(uint8_t ctrlBits);
    void AxeObj(uint8_t objectId);
    BlockBufferResult BBChk_E(uint8_t coordSelector, uint8_t objectOffset, uint8_t cornerIdx);
    void BalancePlatRope(uint8_t areaObjBufferOffset);
    void BalancePlatform(uint8_t e);
    BlockBufferResult BlockBufferChk_Enemy(uint8_t coordSelector, uint8_t cornerIdx, uint8_t e);
    uint8_t BlockBufferChk_FBall(uint8_t slot);
    BlockBufferResult BlockBufferColli_Feet(uint8_t adderBaseOffset);
    BlockBufferResult BlockBufferColli_Head(uint8_t adderOffset);
    BlockBufferResult BlockBufferColli_Side(uint8_t adderOffset);
    BlockBufferResult BlockBufferCollision(uint8_t coordSelector, uint8_t objectOffset, uint8_t cornerIdx);
    std::pair<bool, uint8_t> BlockBumpedChk(uint8_t metatile);
    void BlockObjMT_Updater();
    void BlockObjectsCore(uint8_t slot);
    uint8_t BoundingBoxCore(uint8_t boundBoxCtrlIdx, uint8_t relPosIdx);
    void BowserGfxHandler(uint8_t enemyOffset);
    void BranchToDecLength1();
    void BrickShatter(BlockBufferCell cell);
    void BrickWithCoins(uint8_t objectId, uint8_t areaObjBufferOffset);
    void BrickWithItem(uint8_t objectId, uint8_t areaObjBufferOffset);
    void BridgeCollapse();
    void Bridge_High(uint8_t areaObjBufferOffset);
    void Bridge_Low(uint8_t areaObjBufferOffset);
    void Bridge_Middle(uint8_t areaObjBufferOffset);
    void BubbleCheck(uint8_t slot);
    void BulletBillCannon(uint8_t areaObjBufferOffset);
    void BulletBillHandler(uint8_t slot);
    void BumpBlock(uint8_t collidedMetatile, BlockBufferCell cell);
    void CGrab_TTickRegL(uint8_t length, uint8_t ctrlByte);
    void CastleBridgeObj(uint8_t objectId, uint8_t areaObjBufferOffset);
    void CastleObject(uint8_t areaObjBufferOffset);
    void ChainObj(uint8_t objectId);
    void CheckAnimationStop(uint8_t gfxOffset, EnemyGfxState &gfx);
    void CheckDefeatedState(uint8_t gfxOffset, EnemyGfxState &gfx);
    static bool CheckForClimbMTiles(uint8_t metatile);
    bool CheckForCoinMTiles(uint8_t metatile);
    void CheckForHammerBro(uint8_t gfxOffset, EnemyGfxState &gfx);
    static bool CheckForSolidMTiles(uint8_t metatile);
    bool CheckPlayerVertical();
    void CheckRightScreenBBox(uint8_t objectOffset, uint8_t boundBoxIdx);
    uint8_t CheckTopOfBlock(BlockBufferCell &cell);
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
    LrgObjLength ChkLrgObjLength(uint8_t areaObjBufferOffset);
    void ChkNoEn(uint8_t startSlot);
    void ChkPOffscr();
    void ChkSmallPlatCollision(uint8_t e);
    void ChkToStunEnemies(uint8_t species, uint8_t eid);
    BlockBufferResult ChkUnderEnemy(uint8_t e);
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
    void DigitsMathRoutine(uint8_t digitOffset);
    void DisplayIntermediate();
    void DisplayTimeUp();
    static uint8_t DividePDiff(uint8_t pixelDiff, uint8_t threshold, uint8_t value, uint8_t flag, uint8_t currentOffset);
    void DmpJpFPS(uint8_t ctrlByte, uint8_t sweepByte);
    void DoEnemySideCheck(uint8_t e);
    static void DoNothing2();
    void DoOtherPlatform(uint8_t oldYPos, uint8_t e);
    void DonePlayerTask();
    void PlayerChangeSize();
    void PlayerInjuryBlink();
    void InitChangeSize();
    void DrawBlock(uint8_t slot);
    void DrawBrickChunks(uint8_t slot);
    void DrawBubble(uint8_t slot);
    std::pair<uint8_t, uint8_t> DrawEnemyObjRow(uint8_t gfxOffset, uint8_t oamSlot, EnemyGfxState &gfx);
    void DrawEnemyObject(uint8_t gfxOffset, EnemyGfxState &gfx);
    void DrawExplosion_Fireball(uint8_t slot);
    void DrawExplosion_Fireworks(uint8_t frameSelector, uint8_t spriteDataBase);
    void DrawFireball(uint8_t slot);
    void DrawFirebar(uint8_t oamSlot);
    uint8_t DrawFirebar_Collision(uint8_t oamOffset, uint8_t mirrorData, uint8_t horizAdder, uint8_t vertAdder);
    void DrawHammer(uint8_t slot);
    void DrawLargePlatform(uint8_t e);
    void DrawMushroomIcon();
    std::pair<uint8_t, uint8_t> DrawOneSpriteRow(uint8_t firstTile, uint8_t secondTile, uint8_t spritePairIdx, uint8_t oamSlot,
                                                 uint8_t flipBits, uint8_t attributeBits, uint8_t xPos, uint8_t &yPos);
    void DrawPlayerLoop(uint8_t gfxOffset, uint8_t sprDataOffset, uint8_t flipBits, uint8_t attributeBits, uint8_t xPos, uint8_t yPos,
                        uint8_t rows);
    void DrawPowerUp();
    void DrawQBlk(uint8_t brickQBlockIndex, uint8_t areaObjBufferOffset);
    void DrawRope(uint8_t startCol, uint8_t numRows);
    void DrawRow(uint8_t tile, uint8_t row);
    void DrawSmallPlatform(uint8_t e);
    std::pair<uint8_t, uint8_t> DrawSpriteObject(uint8_t firstTile, uint8_t secondTile, uint8_t spritePairIdx, uint8_t oamSlot,
                                                 uint8_t flipBits, uint8_t attributeBits, uint8_t xPos, uint8_t &yPos);
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
    void ErACM(BlockBufferCell cell);
    void EraseEnemyObject(uint8_t eid);
    void ExInjColRoutines();
    void ExecGameLoopback(uint8_t loopIndex);
    void ExitPipe(uint8_t areaObjBufferOffset);
    void ExtraLifeMushBlock(uint8_t blockOffset);
    void FBallB(uint8_t objectOffset, uint8_t relPosIdx);
    void FPS2nd(uint8_t ctrlByte);
    void FallE(uint8_t e);
    std::optional<uint8_t> FindEmptyEnemySlot();
    std::pair<bool, uint8_t> FindEmptyMiscSlot();
    void FindPlayerAction();
    void FinishFlame(uint8_t e);
    void FireballBGCollision(uint8_t slot);
    void FireballEnemyCollision(uint8_t slot);
    void FireballObjCore(uint8_t slot);
    uint8_t FirebarCollision(uint8_t oamOffset, uint8_t segmentX, uint8_t segmentY);
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
    uint8_t GetAreaObjYPosition(uint8_t row);
    void GetAreaPalette();
    void GetAreaType(uint8_t areaPointerByte);
    void GetBackgroundColor();
    static uint16_t GetBlockBufferAddr(uint8_t column);
    void GetBlockOffscreenBits(uint8_t slot);
    void GetBubbleOffscreenBits(uint8_t slot);
    uint8_t GetCurrentAnimOffset(uint8_t baseIdx);
    void GetEnemyBoundBox(uint8_t eid);
    std::pair<uint8_t, uint8_t> GetEnemyBoundBoxOfs();
    std::pair<uint8_t, uint8_t> GetEnemyBoundBoxOfsArg(uint8_t eid);
    void GetEnemyOffscreenBits(uint8_t offset);
    void GetFireballBoundBox(uint8_t slot);
    void GetFireballOffscreenBits(uint8_t slot);
    std::tuple<uint8_t, uint8_t, uint8_t> GetFirebarPosition(uint8_t spinstateHigh, uint8_t firebarPart);
    uint8_t GetGfxOffsetAdder(uint8_t baseIdx);
    std::pair<uint8_t, uint8_t> GetLrgObjAttrib(uint8_t areaObjBufferOffset);
    static uint8_t GetMTileAttrib(uint8_t metatile);
    void GetMaskedOffScrBits(uint8_t eid, uint8_t defaultBitmask, uint8_t onscreenBitmask);
    void GetMiscBoundBox(uint8_t slot);
    void GetMiscOffscreenBits(uint8_t slot);
    void GetObjRelativePosition(uint8_t objectOffset, uint8_t relPosIdx);
    void GetOffScreenBitsSet(uint8_t objectOffset, uint8_t offscrArrayOffset);
    uint8_t GetOffsetFromAnimCtrl(uint8_t frameCtrl, uint8_t baseIdx);
    std::tuple<uint8_t, uint8_t, uint8_t> GetPipeHeight(uint8_t areaObjBufferOffset);
    void GetPlayerAnimSpeed();
    uint8_t GetPlayerColors();
    void GetPlayerOffscreenBits();
    static uint8_t GetProperObjOffset(uint8_t baseOffset, uint8_t tableSelector);
    void GetRow(uint8_t tile, uint8_t areaObjBufferOffset);
    void GetRow2(uint8_t tile, uint8_t areaObjBufferOffset);
    void GetSBNybbles();
    uint8_t GetScreenPosition();
    uint8_t GetXOffscreenBits(uint8_t objectOffset);
    void GiveOneCoin();
    void GoContinue(uint8_t worldNumber);
    void GrowItemRegs(uint8_t length);
    uint8_t HandleChangeSize();
    void HandleCoinMetatile(BlockBufferCell cell);
    void HandleEnemyFBallCol(uint8_t enemySlot);
    void HandleGroupEnemies(uint8_t enemyByte);
    void HandleNoiseMusic();
    void HandlePipeEntry(uint8_t rightFootMetatile, uint8_t leftFootMetatile);
    void HandleSquare1Music();
    void HandleTriangleMusic();
    void Hidden1UpBlock(uint8_t objectId, uint8_t areaObjBufferOffset);
    void Hole_Empty(uint8_t areaObjBufferOffset);
    void Hole_Water(uint8_t areaObjBufferOffset);
    void HurtBowser(uint8_t slot, uint8_t scoreSlot);
    void ImpedePlayerMove(uint8_t side);
    void ImposeFriction(uint8_t leftRightButtons);
    void ImposeGravity(uint8_t movementMode, uint8_t objectOffset, uint8_t downAmount, uint8_t upAmount, uint8_t maxSpeed);
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
    void InitializeMemory(uint16_t clearUntil);
    void InitializeNameTables();
    void InjurePlayer();
    void IntroPipe(uint8_t areaObjBufferOffset);
    void JCoinC(uint8_t blockOffset, uint8_t miscSlot);
    void JCoinGfxHandler(uint8_t slot);
    void JumpRegContents(uint8_t freqIndex);
    void Jumpspring(uint8_t areaObjBufferOffset);
    void JumpspringHandler(uint8_t e);
    void KillAllEnemies();
    void KillEnemies(uint8_t enemyId);
    void KillEnemyAboveBlock();
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
    void MoveBubl(uint8_t slot, uint8_t randomBit);
    void MoveBulletBill(uint8_t e);
    uint8_t MoveColOffscreen(uint8_t yPosOffset);
    void MoveD_Bowser(uint8_t enemyOffset);
    void MoveD_EnemyVertically(uint8_t eid);
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
    void MoveSixSpritesOffscreen(uint8_t baseOffset);
    void MoveSpritesOffscreen();
    void MoveSwimmingCheepCheep(uint8_t e);
    void MoveVOffset(uint8_t vramOffset);
    uint8_t MoveWithXMCntrs(uint8_t e);
    void MushFlowerBlock(uint8_t blockOffset);
    void MusicHandler();
    void NextArea();
    void NoBump(uint8_t e);
    void NoiseSfxHandler();
    void NonMaskableInterrupt();
    void NormObj(uint8_t objectId, uint8_t dispatchOffset, uint8_t areaObjBufferOffset);
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
    void PlayerHeadCollision(uint8_t collidedMetatile, BlockBufferCell cell);
    uint8_t PlayerLakituDiff(uint8_t e);
    void PlayerLoseLife();
    void PlayerMovementSubs();
    void PlayerOffscreenChk();
    void PlayerPhysicsSub();
    void PlayerVictoryWalk();
    void PosPlatform(uint8_t offsetIndex, uint8_t e);
    void PositionPlayerOnHPlat(uint8_t e, uint8_t savedAdder);
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
    void ProcLPlatCollisions(uint8_t boundBoxOfs, uint8_t e, uint8_t stagedValue);
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
    void PutBlockMetatile(uint8_t metatileGroupSelector, BlockBufferCell cell, uint8_t vramOffset);
    void PutPlayerOnVine(uint16_t blockBufferAddr);
    void PwrUpJmp();
    void QuestionBlock(uint8_t objectId, uint8_t areaObjBufferOffset);
    void QuestionBlockRow_High(uint8_t areaObjBufferOffset);
    void QuestionBlockRow_Low(uint8_t areaObjBufferOffset);
    void RXSpd(uint8_t eid);
    void ReadJoypads();
    void ReadPortBits(uint8_t port);
    void RelWOfs(uint8_t objectOffset, uint8_t relPosIdx);
    void RelativeBlockPosition(uint8_t slot);
    void RelativeBubblePosition(uint8_t slot);
    void RelativeEnemyPosition(uint8_t offset);
    void RelativeFireballPosition(uint8_t slot);
    void RelativeMiscPosition(uint8_t slot);
    void RelativePlayerPosition();
    void RemBridge(uint8_t metatileGroupOfs4, uint8_t vramOffset, uint8_t nameTableLow, uint8_t nameTableHigh);
    void RemoveCoin_Axe(BlockBufferCell cell);
    void RenderAreaGraphics();
    void RenderAttributeTables();
    void RenderPlayerSub(uint8_t rows);
    std::pair<bool, uint8_t> RenderSidewaysPipe(uint8_t areaObjBufferOffset, uint8_t verticalLength);
    uint8_t RenderUnderPart(uint8_t tile, uint8_t startCol, uint8_t numRows);
    void ReplaceBlockMetatile(uint8_t metatile, uint8_t blockOffset, BlockBufferCell cell);
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
    std::pair<uint8_t, uint8_t> RunOffscrBitsSubs(uint8_t objectOffset);
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
    void SetHJ(uint8_t verticalSpeed, uint8_t e, uint8_t jumpLengthBitmask);
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
    void SetupBubble(uint8_t slot, uint8_t randomBit);
    void SetupEOffsetFBBox(uint8_t objectOffset);
    uint8_t SetupFloateyNumber(uint8_t pointsControl, uint8_t eid);
    void SetupGameOver();
    void SetupIntermediate();
    void SetupJumpCoin(uint8_t blockOffset, BlockBufferCell cell);
    void SetupLakitu(uint8_t e);
    std::pair<uint8_t, uint8_t> SetupPlatformRope(uint8_t vertSpeed, uint8_t e);
    void SetupPowerUp(uint8_t blockOffset);
    void SetupVictoryMode();
    void Setup_Vine(uint8_t eid, uint8_t blockOffset);
    void ShellOrBlockDefeat(uint8_t eid);
    void SideExitPipeEntry();
    uint8_t SixSpriteStacker(uint8_t baseCoord, uint8_t oamOffset);
    void SprInitLoop(uint8_t yOffset);
    void RenderQuestionBlockRow(uint8_t startRow, uint8_t areaObjBufferOffset);
    void RenderBridge(uint8_t startRow, uint8_t areaObjBufferOffset);
    void SetPowerUpType(uint8_t powerUpType, uint8_t blockOffset);
    void ImposeBlockGravity(uint8_t maxSpeedIdx, uint8_t objectOffset);
    void MovePlatform(uint8_t moveDirection, uint8_t e);
    void PositionPlayerOnPlatform(uint8_t yCoord, uint8_t e);
    BlockBufferResult BlockBufferColli_Player(uint8_t coordSelector, uint8_t cornerIdx);
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
    void TitleScreenMode();
    void TopScoreCheck(uint8_t scoreOffset);
    bool TransposePlayers();
    void UpToFiery(uint8_t subroutineNum);
    void UpdScrollVar();
    void UpdateNumber(uint8_t statusBarNybbles);
    void UpdateScreen(uint16_t bufferAddr);
    void VariableObjOfsRelPos(uint8_t baseValue, uint8_t addend, uint8_t relPosIdx);
    void VerticalPipe(uint8_t objectId, uint8_t areaObjBufferOffset);
    void VerticalPipeEntry();
    void VictoryMode();
    void VictoryModeSubroutines();
    void VineBlock();
    void VineObjectHandler(uint8_t e);
    void Vine_AutoClimb();
    void WarpZoneObject();
    void WaterPipe(uint8_t areaObjBufferOffset);
    void WriteBlockMetatile(uint8_t metatile, BlockBufferCell cell);
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
    uint8_t *getCHR();

    /**
     * Get the byte of RAM or of constant data at an address.
     *
     * The hardware registers that sit between the two have side effects on read
     * and on write, and no byte to hand out; go through readData() and
     * writeData() for those.
     */
    uint8_t &getMemory(uint16_t address);

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
    // void ram[uint16_t address] = uint8_t value;

    /**
     * Map constant data to the address space. The address must be at least 0x8000.
     */
    void writeData(uint16_t address, const uint8_t *data, std::size_t length);
};

#endif // SMBENGINE_HPP
