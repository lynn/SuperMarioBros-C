// smb_physics_min.cpp -- a minimal single-file SDL2 demo of Super Mario Bros.'s player
// physics and player-to-background collision, with rectangles for graphics.
//
// The routines keep the names they have in source/SMB/SMBPlayer.cpp and SMBObject.cpp, and
// the data tables and arithmetic are the originals, 8-bit wraparound and all. Only small
// Mario on land is modelled: swimming, climbing, coins, pipes, jumpsprings and the axe are
// gone, along with the block-metatile attribute tables (every block here is solid).
//
// What is faithfully preserved is the feel, including the parts that are arguably bugs:
//
//   * Speed is 8.8 fixed point, but the player is moved by the *high nybble* of the whole
//     part, sign-extended, with the low nybble accumulated into a fraction. Horizontal
//     motion is quantised in a way the speed value alone does not predict.
//   * ImposeFriction and MoveObjectHorizontally accumulate into two different fraction
//     bytes ($0705 and $0400 in the original). That is genuine SMB, not a transcription
//     slip -- see Player::xFrictionFrac.
//   * The player is set to Falling at the top of every collision pass and only the foot
//     check sets it back to Ground. Walking off a ledge falls out of that.
//   * The foot check prefers the left foot and falls back to the right, but lands the
//     player using the *left* foot's low nybble either way.
//   * Only one side collision is handled per frame -- the first one found returns out of
//     the whole routine, skipping the other side.
//   * Head bumps test a single point at the player's horizontal centre.
//   * Small Mario's two samples per side land on the same point, so each side is really
//     one test at one height; only the vertical gates around them differ.
//
// Build: c++ -O2 -o smb_physics_min smb_physics_min.cpp $(pkg-config --cflags --libs sdl2)
// Controls: arrow keys to move, X to jump, Z to run. R resets.

#include <SDL2/SDL.h>

#include <cstdint>
#include <initializer_list>

namespace
{

//------------------------------------------------------------------------
// The level. The game keeps a rolling 32-column window of the level in memory and the
// column index wraps within it; this level is exactly that wide, so it needs no scrolling
// machinery and the wrap is invisible. Rows start below the two-row status bar.

constexpr int kLevelColumns = 32;
constexpr int kLevelRows = 13;
constexpr int kBlockSize = 16;
constexpr int kStatusBarHeight = 2 * kBlockSize;

const char* const kLevel[kLevelRows] = {
    "                                ",
    "                                ",
    "                                ",
    "                    ###         ",
    "            ##                  ",
    "                          ####  ",
    "       ###        #             ",
    "                  #             ",
    "   ##             #      ###    ",
    "                  #             ",
    "##########    ##################",
    "##########    ##################",
    "##########    ##################",
};

bool blockAt(int column, int row)
{
    return kLevel[row][column] == '#';
}

//------------------------------------------------------------------------
// Fixed point. A speed is 8.8: whole pixels per frame in the high byte, signed, and a
// fraction in the low byte. Both halves wrap, and the wrapping is load-bearing.

uint16_t pack(uint8_t high, uint8_t low)
{
    return uint16_t((high << 8) | low);
}

void unpack(uint16_t value, uint8_t& whole, uint8_t& frac)
{
    whole = uint8_t(value >> 8);
    frac = uint8_t(value);
}

bool isNegative(uint8_t value)
{
    return (value & 0x80) != 0;
}

//------------------------------------------------------------------------
// Input. The left/right bits double as the facing and moving directions -- the physics
// compares a direction against raw button bits, so the numbering matters. Pressing both
// gives 3, which matches neither.

constexpr uint8_t kRight = 1;
constexpr uint8_t kLeft = 2;

struct Buttons
{
    bool a = false;
    bool b = false;
    bool down = false;

    // Ducking on the ground blanks the direction the movement code sees -- but not the
    // one the skid check sees, which reads the pad directly. Holding down while turning
    // around therefore still skids.
    uint8_t leftRight = 0;
    uint8_t rawLeftRight = 0;

    bool anyButtonBesidesA() const
    {
        return b || down || rawLeftRight != 0;
    }
};

//------------------------------------------------------------------------

enum class State : uint8_t
{
    Ground = 0,
    Jumping = 1,
    Falling = 2,
};

// Where the collision routines sample the level, relative to the player's position.
constexpr int kHeadX = 0x08, kHeadY = 0x12;
constexpr int kLeftFootX = 0x03, kRightFootX = 0x0c, kFootY = 0x20;
constexpr int kLeftSideX = 0x02, kRightSideX = 0x0d, kSideY = 0x18;

struct Player
{
    // Position, each held as one integer rather than the 6502's separate byte registers.
    // The horizontal position is a 16-bit world coordinate (the original's page:offset
    // pair) and wraps at 64K. The vertical position packs three bytes -- screen index,
    // pixel row, and subpixel fraction -- into the low 24 bits, and wraps within them; the
    // routines that add to it treat it as a single 24-bit quantity.
    uint16_t x = 0;
    uint32_t yPos = 0x010000; // screen 1 (the visible one), pixel row 0, no fraction

    uint8_t yPixel() const { return uint8_t(yPos >> 8); }
    uint8_t yScreen() const { return uint8_t(yPos >> 16); }

    uint8_t xSpeed = 0;
    uint8_t xMoveFrac = 0;     // the fraction MoveObjectHorizontally accumulates into
    uint8_t xFrictionFrac = 0; // the one ImposeFriction accumulates into -- a *different* byte
    uint8_t xSpeedAbsolute = 0;

    uint8_t ySpeed = 0;
    uint8_t yMoveFrac = 0; // the speed's fraction -- separate from the position's (low byte of yPos)

    // The game's entrance routine starts the player on the ground and lets the first
    // collision pass discover otherwise, which costs a frame of gravity if they are not.
    State state = State::Ground;
    uint8_t movingDir = kRight;
    uint8_t facingDir = kRight;

    // Cleared as each side collides, and consulted by ImposeFriction so that holding a
    // direction into a wall stops accelerating into it.
    uint8_t collisionBits = 0xff;

    uint8_t verticalForce = 0;         // gravity for this frame
    uint8_t verticalForceDown = 0x28;  // the stronger gravity used once falling
    uint8_t jumpOriginY = 0;
    uint8_t jumpIdx = 0;               // launch-strength bucket (0..4) chosen at takeoff
    uint8_t runningSpeed = 0;
    uint8_t runningTimer = 0;
    uint16_t frictionAdder = 0;
    uint8_t maxLeftSpeed = 0;
    uint8_t maxRightSpeed = 0;

    // Is the level solid at a point offset from the player? The column wraps every 32.
    bool solidAt(int xAdder, int yAdder) const
    {
        const int column = ((x + xAdder) >> 4) & (kLevelColumns - 1);
        // The vertical extent checks in PlayerBGCollision bracket the player tightly
        // enough that this always lands inside the level once the status bar is taken
        // off; the original, which indexes memory here, relies on the same thing.
        const int row = (uint8_t(yPixel() + yAdder) >> 4) - (kStatusBarHeight / kBlockSize);
        return row >= 0 && row < kLevelRows && blockAt(column, row);
    }
};

//------------------------------------------------------------------------
// Movement.

// Moves the player by the high nybble of the horizontal speed, sign-extended, carrying the
// low nybble in a fraction.
void MoveObjectHorizontally(Player& p)
{
    const uint8_t fractionStep = uint8_t(p.xSpeed << 4);

    uint8_t wholeStep = p.xSpeed >> 4;
    if (wholeStep >= 8)
    {
        wholeStep |= 0xf0; // sign-extend the nybble
    }

    const uint16_t fraction = uint16_t(p.xMoveFrac + fractionStep);
    p.xMoveFrac = uint8_t(fraction);
    const uint8_t carry = (fraction > 0xff) ? 1 : 0;

    // x is one 16-bit quantity, and the step a signed amount that sign-extends into it
    p.x = uint16_t(p.x + int8_t(wholeStep) + carry);
}

// Applies gravity: moves the player by the vertical speed, then adds this frame's
// downward force to that speed, capped.
void ImposeGravity(Player& p, uint8_t downAmount, uint8_t maxSpeed)
{
    // yPos is one 24-bit quantity (screen:pixel:fraction), and the speed a signed amount
    // that sign-extends into it. The mask keeps the add wrapping within those 24 bits.
    const uint8_t speedHigh = isNegative(p.ySpeed) ? 0xff : 0x00;
    const uint32_t step = (uint32_t(speedHigh) << 16) | (uint32_t(p.ySpeed) << 8) | p.yMoveFrac;
    p.yPos = (p.yPos + step) & 0xffffff;

    unpack(uint16_t(pack(p.ySpeed, p.yMoveFrac) + downAmount), p.ySpeed, p.yMoveFrac);

    // Clamp, but only once the fraction has carried past halfway -- the original tests the
    // fraction's high bit here, which lets the speed sit one step above the cap.
    if (!isNegative(uint8_t(p.ySpeed - maxSpeed)) && p.yMoveFrac >= 0x80)
    {
        p.ySpeed = maxSpeed;
        p.yMoveFrac = 0;
    }
}

void MovePlayerVertically(Player& p)
{
    constexpr uint8_t kMaxFallSpeed = 4;
    ImposeGravity(p, p.verticalForce, kMaxFallSpeed);
}

//------------------------------------------------------------------------
// Physics: jump initiation, and the friction and speed caps for this frame.

void PlayerPhysicsSub(Player& p, const Buttons& pad, bool aWasHeld)
{
    // Indexed by how fast the player was moving when they jumped: the faster the launch,
    // the stronger the jump and the weaker the gravity holding it down.
    const uint8_t kJumpForce[] = {0x20, 0x20, 0x1e, 0x28, 0x28};
    const uint8_t kFallForce[] = {0x70, 0x70, 0x60, 0x90, 0x90};
    const uint8_t kJumpSpeed[] = {0xfc, 0xfc, 0xfc, 0xfb, 0xfb};

    // Running (index 0) accelerates harder and caps higher than walking (index 1).
    const uint8_t kMaxRightSpeed[] = {0x28, 0x18};
    const uint8_t kMaxLeftSpeed[] = {0xd8, 0xe8};
    const uint8_t kFriction[] = {0xe4, 0x98, 0xd0};

    // CheckForJumping: a fresh press of A, not one held from the previous frame
    if (pad.a && !aWasHeld && p.state == State::Ground)
    {
        p.yPos &= ~uint32_t(0xff); // clear the position's subpixel fraction (its low byte)
        p.yMoveFrac = 0;
        p.jumpOriginY = p.yPixel();
        p.state = State::Jumping;

        uint8_t jumpIdx = 0;
        if (p.xSpeedAbsolute >= 0x09) { jumpIdx = 1; }
        if (p.xSpeedAbsolute >= 0x10) { jumpIdx = 2; }
        if (p.xSpeedAbsolute >= 0x19) { jumpIdx = 3; }
        if (p.xSpeedAbsolute >= 0x1c) { jumpIdx = 4; }

        p.jumpIdx = jumpIdx;
        p.verticalForce = kJumpForce[jumpIdx];
        p.verticalForceDown = kFallForce[jumpIdx];
        p.ySpeed = kJumpSpeed[jumpIdx];
    }

    // X_Physics: pick between the running and walking sets. Running requires the B button
    // held while already moving the way you are pressing, and it lingers for ten frames.
    bool walking = false;
    if (p.state != State::Ground)
    {
        walking = p.xSpeedAbsolute < 0x19; // airborne, so keep whatever was in force
    }
    else if (pad.leftRight != p.movingDir)
    {
        walking = true;
    }
    else if (pad.b)
    {
        p.runningTimer = 10;
    }
    else if (p.runningTimer == 0)
    {
        walking = true;
    }

    uint8_t frictionIdx = 0;
    if (walking)
    {
        // A player who is already fast keeps the weaker friction of the third entry.
        frictionIdx = (p.runningSpeed != 0 || p.xSpeedAbsolute >= 0x21) ? 2 : 1;
    }
    const int speedIdx = walking ? 1 : 0;

    p.maxLeftSpeed = kMaxLeftSpeed[speedIdx];
    p.maxRightSpeed = kMaxRightSpeed[speedIdx];
    p.frictionAdder = kFriction[frictionIdx];
    if (p.facingDir != p.movingDir)
    {
        p.frictionAdder = uint16_t(p.frictionAdder << 1); // turning around brakes twice as hard
    }
}

// Only the skid survives from the original, which also picked the walk animation's speed.
void GetPlayerAnimSpeed(Player& p, const Buttons& pad)
{
    if (p.xSpeedAbsolute >= 0x1c)
    {
        p.runningSpeed = p.xSpeedAbsolute;
        return;
    }
    if (!pad.anyButtonBesidesA())
    {
        return; // ChkSkid: no buttons at all, so nothing to skid against
    }
    if (pad.rawLeftRight == p.movingDir)
    {
        p.runningSpeed = 0;
    }
    else if (p.xSpeedAbsolute < 11)
    {
        // ProcSkid: slow enough to snap around and set off the other way
        p.movingDir = p.facingDir;
        p.xSpeed = 0;
        p.xFrictionFrac = 0;
    }
}

// Accelerates towards a held direction, or decelerates towards a stop, and clamps to this
// frame's maximum. Despite the name it is what makes the player accelerate at all.
void ImposeFriction(Player& p, uint8_t leftRightButtons)
{
    const uint8_t pressed = leftRightButtons & p.collisionBits;

    bool speedingUpLeft = false;
    if (pressed != 0)
    {
        speedingUpLeft = (pressed & kRight) != 0;
    }
    else if (p.xSpeed == 0)
    {
        p.xSpeedAbsolute = 0;
        return;
    }
    else
    {
        speedingUpLeft = isNegative(p.xSpeed); // coasting: bleed off whichever way we move
    }

    if (speedingUpLeft)
    {
        unpack(uint16_t(pack(p.xSpeed, p.xFrictionFrac) + p.frictionAdder), p.xSpeed, p.xFrictionFrac);
        if (!isNegative(uint8_t(p.xSpeed - p.maxRightSpeed)))
        {
            p.xSpeed = p.maxRightSpeed;
            p.xSpeedAbsolute = p.maxRightSpeed;
            return;
        }
    }
    else
    {
        unpack(uint16_t(pack(p.xSpeed, p.xFrictionFrac) - p.frictionAdder), p.xSpeed, p.xFrictionFrac);
        if (isNegative(uint8_t(p.xSpeed - p.maxLeftSpeed)))
        {
            p.xSpeed = p.maxLeftSpeed;
        }
    }

    p.xSpeedAbsolute = isNegative(p.xSpeed) ? uint8_t(-p.xSpeed) : p.xSpeed;
}

void OnGroundStateSub(Player& p, const Buttons& pad)
{
    GetPlayerAnimSpeed(p, pad);
    if (pad.leftRight != 0)
    {
        p.facingDir = pad.leftRight;
    }
    ImposeFriction(p, pad.leftRight);
    MoveObjectHorizontally(p);
}

void PlayerMovementSubs(Player& p, const Buttons& pad, bool aWasHeld)
{
    PlayerPhysicsSub(p, pad, aWasHeld);

    if (p.state == State::Ground)
    {
        OnGroundStateSub(p, pad);
        return;
    }

    // Releasing A mid-rise swaps in the heavier falling gravity, which is what makes a
    // short hop short. Holding it, or being within a pixel of the launch height, keeps the
    // lighter jumping gravity.
    bool cutJumpShort = true;
    if (p.state == State::Jumping && isNegative(p.ySpeed))
    {
        const bool stillHoldingJump = pad.a && aWasHeld;
        const bool barelyOffTheGround = uint8_t(p.jumpOriginY - p.yPixel()) < 1;
        cutJumpShort = !stillHoldingJump && !barelyOffTheGround;
    }
    if (cutJumpShort)
    {
        p.verticalForce = p.verticalForceDown;
    }

    if (pad.leftRight != 0)
    {
        ImposeFriction(p, pad.leftRight); // air control
    }
    MoveObjectHorizontally(p);
    MovePlayerVertically(p);
}

//------------------------------------------------------------------------
// Collision.

// Stops the player at a wall, and nudges them a pixel out of it if they were not already
// moving away.
void ImpedePlayerMove(Player& p, uint8_t side)
{
    const bool leftSide = side == kRight;

    const uint8_t collisionBit = leftSide ? kRight : kLeft;
    const int8_t nudge = leftSide ? -1 : 1;
    const bool alreadyMovingAway = leftSide ? isNegative(p.xSpeed) : !isNegative(uint8_t(p.xSpeed - 1));

    if (!alreadyMovingAway)
    {
        p.xSpeed = 0;
        p.x = uint16_t(p.x + nudge);
    }

    p.collisionBits &= uint8_t(~collisionBit);
}

// Returns true if the routine should stop here, as the original's foot check does when it
// pushes the player sideways out of a block.
bool footCheck(Player& p)
{
    if (p.yPixel() >= 0xcf)
    {
        return false;
    }

    // The left foot wins if it found anything; the right is only a fallback.
    const bool onBlock = p.solidAt(kLeftFootX, kFootY) || p.solidAt(kRightFootX, kFootY);
    if (!onBlock || isNegative(p.ySpeed))
    {
        return false;
    }

    // How far into the block the player is, measured from the *left* foot's coordinate
    // whichever foot actually found the block.
    if ((p.yPixel() & 0x0f) >= 5)
    {
        ImpedePlayerMove(p, p.movingDir); // too deep to stand on: squeeze out sideways
        return true;
    }

    p.yPos &= ~uint32_t(0x0f00); // snap to the top of the block (clear the pixel's low nybble)
    p.ySpeed = 0;
    p.yMoveFrac = 0;
    p.state = State::Ground;
    return false;
}

void headCheck(Player& p)
{
    constexpr uint8_t kUpperExtent = 0x10;
    if (p.yPixel() < kUpperExtent || !p.solidAt(kHeadX, kHeadY))
    {
        return;
    }
    // Only counts while rising, and only once the player is far enough into the block.
    if (isNegative(p.ySpeed) && (p.yPixel() & 0x0f) >= 0x04)
    {
        p.ySpeed = 1; // kill the jump
    }
}

// Each side is sampled twice at the same point but behind different vertical gates, so a
// player high or low on the screen can have one sample skipped and not the other.
void sideCheck(Player& p)
{
    const int sideX[] = {kLeftSideX, kRightSideX};
    const uint8_t collidedSide[] = {kLeft, kRight};

    for (int side = 0; side < 2; ++side)
    {
        if (p.yPixel() >= 0x20)
        {
            if (p.yPixel() >= 0xe4)
            {
                return;
            }
            if (p.solidAt(sideX[side], kSideY))
            {
                ImpedePlayerMove(p, collidedSide[side]);
                return; // only ever one side per frame
            }
        }
        if (p.yPixel() < 0x08 || p.yPixel() >= 0xd0)
        {
            return;
        }
        if (p.solidAt(sideX[side], kSideY))
        {
            ImpedePlayerMove(p, collidedSide[side]);
            return;
        }
    }
}

void PlayerBGCollision(Player& p)
{
    // Falling is the default every frame and only the foot check takes it back. This is
    // what makes the player drop the moment they walk off a ledge.
    if (p.state == State::Ground)
    {
        p.state = State::Falling;
    }
    if (p.yScreen() != 1)
    {
        return; // off the bottom of the level
    }

    p.collisionBits = 0xff;
    if (p.yPixel() >= 0xcf)
    {
        return;
    }

    headCheck(p);
    if (footCheck(p))
    {
        return;
    }
    sideCheck(p);
}

//------------------------------------------------------------------------

struct Game
{
    Player player;
    bool aWasHeld = false;

    void reset()
    {
        player = Player{};
        player.x = 0x0028;
        player.yPos = 0x016000; // screen 1, pixel row 0x60
    }

    void update(const Buttons& rawPad)
    {
        Buttons pad = rawPad;
        pad.rawLeftRight = rawPad.leftRight;
        // Ducking on the ground locks out left and right.
        if (pad.down && player.state == State::Ground)
        {
            pad.leftRight = 0;
        }

        PlayerMovementSubs(player, pad, aWasHeld);
        if (player.xSpeed != 0)
        {
            player.movingDir = isNegative(player.xSpeed) ? kLeft : kRight;
        }
        PlayerBGCollision(player);

        if (player.runningTimer != 0)
        {
            --player.runningTimer;
        }
        aWasHeld = pad.a;

        // Keep the player inside the level: past the right edge the column index would
        // wrap and the level would repeat underneath them.
        if (player.x >= 0x1f0)
        {
            player.x = 0x01f0;
            player.xSpeed = 0;
        }
        if (player.yScreen() != 1)
        {
            reset(); // fell in a pit
        }
    }
};

Buttons readButtons()
{
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    Buttons pad;
    pad.a = keys[SDL_SCANCODE_X] != 0;
    pad.b = keys[SDL_SCANCODE_Z] != 0;
    pad.down = keys[SDL_SCANCODE_DOWN] != 0;
    pad.leftRight = uint8_t((keys[SDL_SCANCODE_RIGHT] != 0 ? kRight : 0) | (keys[SDL_SCANCODE_LEFT] != 0 ? kLeft : 0));
    return pad;
}

void render(SDL_Renderer* renderer, const Player& p)
{
    // Centre the camera on the player, clamped to the level.
    constexpr int kScreenWidth = 256;
    constexpr int kLevelWidth = kLevelColumns * kBlockSize;
    const int camera = SDL_clamp(int(p.x) - (kScreenWidth / 2), 0, kLevelWidth - kScreenWidth);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    for (int column = 0; column < kLevelColumns; ++column)
    {
        for (int row = 0; row < kLevelRows; ++row)
        {
            if (!blockAt(column, row))
            {
                continue;
            }
            SDL_Rect block = {(column * kBlockSize) - camera, kStatusBarHeight + (row * kBlockSize), kBlockSize,
                              kBlockSize};
            SDL_RenderFillRect(renderer, &block);
        }
    }

    // Draw the player as the box the collision routines actually test, and read its state
    // straight off the shape:
    //
    //   * State  -> how much of the box is filled: Ground is solid, Jumping fills the lower
    //     half under an outline, Falling is a hollow outline. (Fullness = groundedness.)
    //   * facingDir -> a marker on the side the player faces.
    //   * jumpIdx -> that many pips (0..4) along the top edge, the launch-strength bucket
    //     of the current or most recent jump.
    const int left = int(p.x) + kLeftSideX - camera;
    const int top = p.yPixel() + kHeadY;
    const int width = kRightSideX - kLeftSideX + 1;
    const int height = kFootY - kHeadY;
    const SDL_Rect body = {left, top, width, height};

    SDL_SetRenderDrawColor(renderer, 252, 216, 168, 255);
    if (p.state == State::Ground)
    {
        SDL_RenderFillRect(renderer, &body);
    }
    else if (p.state == State::Jumping)
    {
        const SDL_Rect lower = {left, top + (height / 2), width, height - (height / 2)};
        SDL_RenderFillRect(renderer, &lower);
        SDL_RenderDrawRect(renderer, &body);
    }
    else // Falling
    {
        SDL_RenderDrawRect(renderer, &body);
    }

    SDL_SetRenderDrawColor(renderer, 32, 56, 236, 255);
    constexpr int kMarkerWidth = 3;
    const SDL_Rect facing = {p.facingDir == kRight ? left + width - kMarkerWidth : left, top + (height / 2) - 1,
                             kMarkerWidth, 3};
    SDL_RenderFillRect(renderer, &facing);

    // SDL_SetRenderDrawColor(renderer, 216, 88, 84, 255);
    // for (int i = 0; i < p.jumpIdx; ++i)
    // {
    //     const SDL_Rect pip = {left + 1 + (i * 2), top - 2, 1, 1};
    //     SDL_RenderFillRect(renderer, &pip);
    // }

    // Mark the two points the foot check samples, since landing is decided by those and
    // not by the box.
    SDL_SetRenderDrawColor(renderer, 255, 100, 0, 255);
    for (int footX : {kLeftFootX, kRightFootX})
    {
        const SDL_Rect foot = {int(p.x) + footX - camera, p.yPixel() + kFootY, 1, 1};
        SDL_RenderFillRect(renderer, &foot);
    }

    SDL_RenderPresent(renderer);
}

} // namespace

int main(int, char**)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }

    constexpr int kScale = 6;
    SDL_Window* window = SDL_CreateWindow("SMB player physics", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          256 * kScale, 240 * kScale, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(renderer, 256, 240);

    Game game;
    game.reset();

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_R)
            {
                game.reset();
            }
        }

        game.update(readButtons());
        render(renderer, game.player);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
