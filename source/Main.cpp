#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <SDL2/SDL.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include "Emulation/Controller.hpp"
#include "Emulation/Fm2Movie.hpp"
#include "SMB/SMBEngine.hpp"
#include "Util/RamMap.hpp"
#include "Util/Video.hpp"

#include "Constants.hpp"

uint8_t* romImage;
static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* texture;
static SDL_Texture* scanlineTexture;
static SDL_Texture* screenTexture;
static SDL_Texture* nametableTexture;
static SMBEngine* smbEngine = nullptr;
static Fm2Movie movie;
static uint32_t renderBuffer[RENDER_WIDTH * RENDER_HEIGHT];
static uint32_t nametableBuffer[NAMETABLE_WIDTH * NAMETABLE_HEIGHT];

/**
 * Which of the workspace's views the taskbar has open.
 */
static bool showGame = true;
static bool showNametables = false;
static bool showRam = false;

/**
 * The layout of the RAM view: the 2KB of NES RAM as rows of this many bytes.
 * RAM_VIEW_WIDTH below is what fits a row of them.
 */
static const int RAM_BYTES_PER_ROW = 16;
static const int RAM_ROWS = int(SMBEngine::RAM_SIZE) / RAM_BYTES_PER_ROW;

/**
 * Where the views are put on the workspace, and how big they are, the first
 * time they are opened. In unscaled pixels, and multiplied by uiScale in use.
 *
 * The game takes the left of the workspace, and the debugging views stack in a
 * column to the right of it. They are only starting points: the views are moved
 * and resized freely, and ImGui remembers where they were left.
 */
static const float LAYOUT_MARGIN = 20.0f;
static const float LAYOUT_TOP = 40.0f; /**< clear of the taskbar */
static const float LAYOUT_COLUMN = LAYOUT_MARGIN * 2 + RENDER_WIDTH * RENDER_SCALE;
static const float NAMETABLE_VIEW_HEIGHT = NAMETABLE_HEIGHT + 60.0f;
static const float RAM_VIEW_WIDTH = 420.0f; /**< a row of RAM_BYTES_PER_ROW bytes, and the key above it */
static const float RAM_VIEW_HEIGHT = 560.0f;

/**
 * The factor the interface is drawn at, so that it stays a usable size on a
 * dense display. Everything the interface sizes in pixels is multiplied by it.
 */
static float uiScale = 1.0f;

/**
 * Work out how far to scale the interface up for the display it is on, from the
 * density the display reports. A scale of one is assumed for a display that
 * reports nothing usable.
 *
 * The factor is rounded to a whole number: the game is pixel art, and a whole
 * factor is the one that keeps its pixels square and evenly sized.
 */
static float detectUiScale()
{
    float dpi = 0.0f;
    if (SDL_GetDisplayDPI(0, NULL, &dpi, NULL) != 0 || dpi <= 0.0f)
    {
        return 1.0f;
    }

    return std::max(1.0f, std::round(dpi / BASE_DPI));
}

/**
 * Load the Super Mario Bros. ROM image.
 */
static bool loadRomImage()
{
    FILE* file = fopen(ROM_FILE_NAME, "r");
    if (file == NULL)
    {
        std::cout << "Failed to open the file \"" << ROM_FILE_NAME << "\". Exiting.\n";
        return false;
    }

    // Find the size of the file
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    // Read the entire file into a buffer
    romImage = new uint8_t[fileSize];
    fread(romImage, sizeof(uint8_t), fileSize, file);
    fclose(file);

    return true;
}

/**
 * SDL Audio callback function.
 */
static void audioCallback(void* userdata, uint8_t* buffer, int len)
{
    if (smbEngine != nullptr)
    {
        smbEngine->audioCallback(buffer, len);
    }
}

/**
 * Initialize SDL, the window, and audio. The capture commands run the game with
 * no window and no audio, and do not call this.
 */
static bool initialize(bool enableAudio, float requestedScale)
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        std::cout << "SDL_Init() failed during initialize(): " << SDL_GetError() << std::endl;
        return false;
    }

    // The display is only there to be asked about once SDL is up.
    //
    uiScale = (requestedScale > 0.0f) ? requestedScale : detectUiScale();

    // Create the window: the workspace the views sit on. It starts out big
    // enough for the layout they default to, but no bigger than the display can
    // show, since that layout is wider than a small display has room for.
    //
    int workspaceWidth = int((LAYOUT_COLUMN + RAM_VIEW_WIDTH + LAYOUT_MARGIN) * uiScale);
    int workspaceHeight = int((LAYOUT_TOP + NAMETABLE_VIEW_HEIGHT + LAYOUT_MARGIN
                               + RAM_VIEW_HEIGHT + LAYOUT_MARGIN) * uiScale);

    SDL_Rect usable;
    if (SDL_GetDisplayUsableBounds(0, &usable) == 0)
    {
        workspaceWidth = std::min(workspaceWidth, usable.w);
        workspaceHeight = std::min(workspaceHeight, usable.h);
    }

    window = SDL_CreateWindow(APP_TITLE,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              workspaceWidth,
                              workspaceHeight,
                              SDL_WINDOW_RESIZABLE);
    if (window == nullptr)
    {
        std::cout << "SDL_CreateWindow() failed during initialize(): " << SDL_GetError() << std::endl;
        return false;
    }

    // Setup the renderer and texture buffer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        std::cout << "SDL_CreateRenderer() failed during initialize(): " << SDL_GetError() << std::endl;
        return false;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, RENDER_WIDTH, RENDER_HEIGHT);
    if (texture == nullptr)
    {
        std::cout << "SDL_CreateTexture() failed during initialize(): " << SDL_GetError() << std::endl;
        return false;
    }

    nametableTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, NAMETABLE_WIDTH, NAMETABLE_HEIGHT);
    if (nametableTexture == nullptr)
    {
        std::cout << "SDL_CreateTexture() failed during initialize(): " << SDL_GetError() << std::endl;
        return false;
    }

    scanlineTexture = generateScanlineTexture(renderer);

    // The screen and its scanline overlay are composed into a texture of their
    // own, which the interface then shows at whatever size the game's view has
    // been given. The overlay is drawn at 3x, so the composition is too.
    //
    screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, RENDER_WIDTH * 3, RENDER_HEIGHT * 3);
    if (screenTexture == nullptr)
    {
        std::cout << "SDL_CreateTexture() failed during initialize(): " << SDL_GetError() << std::endl;
        return false;
    }

    // Set up Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Scale the interface for the display. ScaleAllSizes() takes care of the
    // spacing, padding and borders; the fonts are scaled separately, and ImGui
    // rasterizes them at the size asked for rather than stretching them.
    //
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(uiScale);
    style.FontScaleDpi = uiScale;

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    if (enableAudio)
    {
        // Initialize audio
        SDL_AudioSpec desiredSpec;
        desiredSpec.freq = AUDIO_FREQUENCY;
        desiredSpec.format = AUDIO_S8;
        desiredSpec.channels = 1;
        desiredSpec.samples = 2048;
        desiredSpec.callback = audioCallback;
        desiredSpec.userdata = NULL;

        SDL_AudioSpec obtainedSpec;
        SDL_OpenAudio(&desiredSpec, &obtainedSpec);

        // Start playing audio
        SDL_PauseAudio(0);
    }

    return true;
}

/**
 * Shutdown libraries for exit.
 */
static void shutdown()
{
    SDL_CloseAudio();

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyTexture(nametableTexture);
    SDL_DestroyTexture(screenTexture);
    SDL_DestroyTexture(scanlineTexture);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
}

/**
 * Draw the taskbar across the top of the window, from which the debugging views
 * are opened.
 */
static void drawTaskbar(bool& running, SMBEngine& engine, bool& saveRequested, bool& loadRequested)
{
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save state", "C"))
        {
            saveRequested = true;
        }
        if (ImGui::MenuItem("Load state", "V", false, engine.hasState()))
        {
            loadRequested = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Quit", "Esc"))
        {
            running = false;
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        ImGui::MenuItem("Game", nullptr, &showGame);
        ImGui::MenuItem("Nametables", nullptr, &showNametables);
        ImGui::MenuItem("RAM", nullptr, &showRam);
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

/**
 * Draw the game itself, as one view on the workspace among the others. The
 * picture keeps its aspect ratio and is centred in whatever space the view has
 * been given, so that resizing it never distorts the game.
 */
static void drawGameWindow()
{
    ImGui::SetNextWindowPos(ImVec2(LAYOUT_MARGIN * uiScale, LAYOUT_TOP * uiScale), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(RENDER_WIDTH * RENDER_SCALE * uiScale,
                                    RENDER_HEIGHT * RENDER_SCALE * uiScale + ImGui::GetFrameHeight()),
                             ImGuiCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    bool open = ImGui::Begin("Game", &showGame);
    ImGui::PopStyleVar();

    if (!open)
    {
        ImGui::End();
        return;
    }

    ImVec2 available = ImGui::GetContentRegionAvail();
    float scale = std::min(available.x / float(RENDER_WIDTH), available.y / float(RENDER_HEIGHT));
    ImVec2 size(RENDER_WIDTH * scale, RENDER_HEIGHT * scale);

    ImGui::SetCursorPos(ImVec2((available.x - size.x) * 0.5f, (available.y - size.y) * 0.5f));
    ImGui::Image((ImTextureID)(intptr_t)screenTexture, size);

    ImGui::End();
}

/**
 * The colour a byte of RAM is drawn in: the colour of the region it belongs to,
 * or plain white if it belongs to none.
 *
 * A byte that is zero is drawn faded, whatever its region. Most of RAM is zero
 * most of the time, so this leaves the bytes that are actually holding
 * something standing out, without losing which region they are in.
 */
static ImU32 ramByteColor(const RamRegion* region, uint8_t value)
{
    uint32_t rgb = (region != nullptr) ? region->color : 0xffffff;
    int alpha = (value == 0) ? 90 : 255;

    return IM_COL32((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff, alpha);
}

/**
 * Draw the key to the colours the regions are drawn in, each named in its own
 * colour.
 */
static void drawRamLegend()
{
    for (std::size_t i = 0; i < ramRegionsLength; i++)
    {
        if (i > 0)
        {
            ImGui::SameLine();
        }

        ImGui::PushStyleColor(ImGuiCol_Text, ramByteColor(&ramRegions[i], 1));
        ImGui::TextUnformatted(ramRegions[i].name);
        ImGui::PopStyleColor();
    }
}

/**
 * Name the address being pointed at in the RAM view, in a tooltip: the region
 * it is in, and what the disassembly calls it. An address can carry more than
 * one name, and most carry none: only the ones the disassembly bothered to name
 * are known here, and it named the start of an array rather than every byte.
 */
static void drawRamLabelTooltip(uint16_t address)
{
    ImGui::BeginTooltip();
    ImGui::Text("$%04x", address);

    const RamRegion* region = findRamRegion(address);
    if (region != nullptr)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ramByteColor(region, 1));
        ImGui::TextUnformatted(region->name);
        ImGui::PopStyleColor();
    }

    bool named = false;
    for (std::size_t i = 0; i < ramMapLength; i++)
    {
        if (ramMap[i].address == address)
        {
            ImGui::TextUnformatted(ramMap[i].name);
            named = true;
        }
    }

    if (!named)
    {
        ImGui::TextDisabled("unnamed");
    }

    ImGui::EndTooltip();
}

/**
 * Draw the view of the NES RAM: all 2KB of it, live, as rows of hex bytes
 * labelled with the address they start at. Pointing at a byte names it.
 */
static void drawRamWindow(SMBEngine& engine)
{
    ImGui::SetNextWindowPos(ImVec2(LAYOUT_COLUMN * uiScale,
                                   (LAYOUT_TOP + NAMETABLE_VIEW_HEIGHT + LAYOUT_MARGIN) * uiScale),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(RAM_VIEW_WIDTH * uiScale, RAM_VIEW_HEIGHT * uiScale),
                             ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("RAM", &showRam))
    {
        ImGui::End();
        return;
    }

    const uint8_t* ram = engine.getRam();

    drawRamLegend();
    ImGui::Separator();

    // A row of the byte's offset within its row, to count along.
    //
    char heading[RAM_BYTES_PER_ROW * 3 + 8];
    int at = snprintf(heading, sizeof(heading), "    ");
    for (int column = 0; column < RAM_BYTES_PER_ROW; column++)
    {
        at += snprintf(heading + at, sizeof(heading) - at, " %02x", column);
    }
    ImGui::TextDisabled("%s", heading);

    // The bytes are drawn one at a time rather than a row at a time, so that
    // each can be greyed out or pointed at on its own. The gap between them is
    // the width of the space that would otherwise separate them, which keeps
    // them under the headings.
    //
    float gap = ImGui::CalcTextSize(" ").x;

    for (int row = 0; row < RAM_ROWS; row++)
    {
        ImGui::Text("%03x:", row * RAM_BYTES_PER_ROW);

        for (int column = 0; column < RAM_BYTES_PER_ROW; column++)
        {
            uint16_t address = uint16_t(row * RAM_BYTES_PER_ROW + column);
            uint8_t value = ram[address];

            ImGui::SameLine(0.0f, gap);

            ImGui::PushStyleColor(ImGuiCol_Text, ramByteColor(findRamRegion(address), value));
            ImGui::Text("%02x", value);
            ImGui::PopStyleColor();

            if (ImGui::IsItemHovered())
            {
                drawRamLabelTooltip(address);
            }
        }
    }

    ImGui::End();
}

/**
 * Draw the view of the PPU's nametables: the whole background it holds, rather
 * than the region of it the scroll registers put on screen. That region is
 * outlined, and wraps around the edges the way the PPU wraps it.
 */
static void drawNametableWindow(SMBEngine& engine)
{
    int scrollX = 0;
    int scrollY = 0;
    engine.renderNametables(nametableBuffer, scrollX, scrollY);

    SDL_UpdateTexture(nametableTexture, NULL, nametableBuffer, sizeof(uint32_t) * NAMETABLE_WIDTH);

    ImGui::SetNextWindowPos(ImVec2(LAYOUT_COLUMN * uiScale, LAYOUT_TOP * uiScale), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(NAMETABLE_WIDTH * uiScale, NAMETABLE_VIEW_HEIGHT * uiScale),
                             ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Nametables", &showNametables))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("Scroll: %d, %d", scrollX, scrollY);

    // Fit the picture to the window's width, so that resizing the window zooms.
    //
    float scale = ImGui::GetContentRegionAvail().x / float(NAMETABLE_WIDTH);
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 size(NAMETABLE_WIDTH * scale, NAMETABLE_HEIGHT * scale);
    ImGui::Image((ImTextureID)(intptr_t)nametableTexture, size);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true);

    // The scrolled region can straddle any edge, so it is drawn once for each
    // way it can wrap around. The clip rectangle discards the copies that are
    // wholly off the picture.
    //
    for (int wrapX = -NAMETABLE_WIDTH; wrapX <= 0; wrapX += NAMETABLE_WIDTH)
    {
        for (int wrapY = -NAMETABLE_HEIGHT; wrapY <= 0; wrapY += NAMETABLE_HEIGHT)
        {
            ImVec2 topLeft(origin.x + (scrollX + wrapX) * scale,
                           origin.y + (scrollY + wrapY) * scale);
            ImVec2 bottomRight(topLeft.x + RENDER_WIDTH * scale,
                               topLeft.y + RENDER_HEIGHT * scale);
            drawList->AddRect(topLeft, bottomRight, IM_COL32(255, 64, 64, 255));
        }
    }

    // The boundaries between the four nametables.
    //
    drawList->AddLine(ImVec2(origin.x + size.x * 0.5f, origin.y),
                      ImVec2(origin.x + size.x * 0.5f, origin.y + size.y),
                      IM_COL32(255, 255, 255, 64));
    drawList->AddLine(ImVec2(origin.x, origin.y + size.y * 0.5f),
                      ImVec2(origin.x + size.x, origin.y + size.y * 0.5f),
                      IM_COL32(255, 255, 255, 64));

    drawList->PopClipRect();

    ImGui::End();
}

static void mainLoop(bool enableAudio)
{
    SMBEngine engine(romImage, enableAudio);
    smbEngine = &engine;
    engine.reset();

    bool running = true;
    int progStartTime = SDL_GetTicks();
    int frame = 0;
    int movieFrame = movie.getFirstFrame();

    // Where the movie had got to when the state was saved, so that loading one during
    // playback rewinds the movie along with the game rather than leaving the two out
    // of step with each other.
    //
    int savedMovieFrame = movie.getFirstFrame();

    // Set by the keys below or by the File menu, and acted on at the top of the frame.
    //
    bool saveRequested = false;
    bool loadRequested = false;
    bool saveKeyWasDown = false;
    bool loadKeyWasDown = false;

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            switch (event.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event)
                {
                case SDL_WINDOWEVENT_CLOSE:
                    running = false;
                    break;
                }
                break;

            default:
                break;
            }
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Keys typed at the interface are not the game's to read.
        //
        static const Uint8 noKeys[SDL_NUM_SCANCODES] = {};
        const Uint8* keys = ImGui::GetIO().WantCaptureKeyboard ? noKeys : SDL_GetKeyboardState(NULL);

        Controller& controller1 = engine.getController1();

        // The console read no controller on a lag frame, so the row of input the
        // movie holds for one is a row that was never used. Where the movie's lag
        // frames are known, playing it back is a matter of ignoring those rows.
        //
        while (movie.isLagFrame(movieFrame))
        {
            movieFrame++;
        }

        if (movie.isLoaded() && movieFrame < movie.getFrameCount())
        {
            // Play back the recorded input for this frame of the movie, in
            // place of the keyboard
            //
            if (movie.isResetFrame(movieFrame))
            {
                engine.reset();
            }

            movie.applyFrame(movieFrame, controller1, engine.getController2());
            movieFrame++;

            if (movieFrame == movie.getFrameCount())
            {
                std::cout << "The movie has finished playing. Control returns to the keyboard." << std::endl;
            }
        }
        else
        {
            controller1.setButtonState(BUTTON_A, keys[SDL_SCANCODE_X]);
            controller1.setButtonState(BUTTON_B, keys[SDL_SCANCODE_Z]);
            controller1.setButtonState(BUTTON_SELECT, keys[SDL_SCANCODE_BACKSPACE]);
            controller1.setButtonState(BUTTON_START, keys[SDL_SCANCODE_RETURN]);
            controller1.setButtonState(BUTTON_UP, keys[SDL_SCANCODE_UP]);
            controller1.setButtonState(BUTTON_DOWN, keys[SDL_SCANCODE_DOWN]);
            controller1.setButtonState(BUTTON_LEFT, keys[SDL_SCANCODE_LEFT]);
            controller1.setButtonState(BUTTON_RIGHT, keys[SDL_SCANCODE_RIGHT]);
        }

        if (keys[SDL_SCANCODE_R])
        {
            // Reset, restarting the movie from the beginning if one is loaded
            engine.reset();
            movieFrame = movie.getFirstFrame();
        }
        if (keys[SDL_SCANCODE_ESCAPE])
        {
            // quit
            running = false;
            break;
        }
        if (keys[SDL_SCANCODE_F])
        {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        }

        // Save and load on the key going down rather than on it being held, so that
        // resting on C saves once instead of overwriting the state every frame.
        //
        const bool saveKeyDown = keys[SDL_SCANCODE_C];
        const bool loadKeyDown = keys[SDL_SCANCODE_V];
        saveRequested = saveRequested || (saveKeyDown && !saveKeyWasDown);
        loadRequested = loadRequested || (loadKeyDown && !loadKeyWasDown);
        saveKeyWasDown = saveKeyDown;
        loadKeyWasDown = loadKeyDown;

        if (saveRequested)
        {
            engine.saveState();
            savedMovieFrame = movieFrame;
        }
        if (loadRequested && engine.loadState())
        {
            movieFrame = savedMovieFrame;
        }
        saveRequested = false;
        loadRequested = false;

        engine.update();

        // Failing a list of the movie's lag frames, the engine guesses at where
        // they are, and drifts a frame out of step at each one it guesses wrong.
        //
        if (movie.isLoaded() && !movie.hasLagFrames() && engine.isLagFrame())
        {
            movieFrame++;
        }

        engine.render(renderBuffer);

        SDL_UpdateTexture(texture, NULL, renderBuffer, sizeof(uint32_t) * RENDER_WIDTH);

        // Compose the screen and its scanlines into a texture for the interface
        // to show.
        //
        SDL_SetRenderTarget(renderer, screenTexture);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderCopy(renderer, scanlineTexture, NULL, NULL);
        SDL_SetRenderTarget(renderer, NULL);

        // Lay the workspace out.
        //
        drawTaskbar(running, engine, saveRequested, loadRequested);
        if (showGame)
        {
            drawGameWindow();
        }
        if (showNametables)
        {
            drawNametableWindow(engine);
        }
        if (showRam)
        {
            drawRamWindow(engine);
        }

        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);

        /**
         * Ensure that the framerate stays as close to the desired FPS as possible. If the frame was rendered faster, then delay.
         * If the frame was slower, reset time so that the game doesn't try to "catch up", going super-speed.
         */
        int now = SDL_GetTicks();
        int delay = progStartTime + int(double(frame) * double(MS_PER_SEC) / double(FRAME_RATE)) - now;
        if(delay > 0)
        {
            SDL_Delay(delay);
        }
        else
        {
            frame = 0;
            progStartTime = now;
        }
        frame++;
    }
}

/**
 * Play the loaded movie back for a number of frames, with no window and no
 * real-time delay, leaving the engine on the last of them.
 *
 * If ramDump is given, the 2KB of NES RAM is written to it after every frame.
 */
static void runMovie(SMBEngine& engine, int frameCount, FILE* ramDump = nullptr)
{
    int movieFrame = movie.getFirstFrame();
    for (int frame = 0; frame < frameCount; frame++)
    {
        Controller& controller1 = engine.getController1();

        // See mainLoop(): a lag frame consumes no controller read, so the movie's
        // input for it is never used.
        //
        while (movie.isLagFrame(movieFrame))
        {
            movieFrame++;
        }

        if (movieFrame < movie.getFrameCount())
        {
            if (movie.isResetFrame(movieFrame))
            {
                engine.reset();
            }

            movie.applyFrame(movieFrame, controller1, engine.getController2());
            movieFrame++;
        }

        engine.update();

        if (!movie.hasLagFrames() && engine.isLagFrame())
        {
            movieFrame++;
        }

        if (ramDump != nullptr)
        {
            fwrite(engine.getRam(), 1, SMBEngine::RAM_SIZE, ramDump);
        }
    }
}

/**
 * Save the screen on a given frame of the loaded movie, as a PNG.
 */
static bool captureScreenshot(int frameCount, const std::string& fileName)
{
    SMBEngine engine(romImage, /* enableAudio */ false);
    engine.reset();

    runMovie(engine, frameCount);

    engine.render(renderBuffer);

    return writeScreenshot(fileName, renderBuffer, RENDER_WIDTH, RENDER_HEIGHT);
}

/**
 * Save the NES RAM of every frame of the loaded movie up to a given one, as one
 * raw 2KB record per frame.
 *
 * A single frame is the last record, but the whole run is what the engine is
 * compared against the console with: FCEUX records the same thing on its side,
 * and the first record that differs is the frame the engine got something wrong
 * on. See tools/README.md.
 */
static bool captureRam(int frameCount, const std::string& fileName)
{
    FILE* file = fopen(fileName.c_str(), "wb");
    if (file == NULL)
    {
        std::cout << "Failed to open \"" << fileName << "\" for writing." << std::endl;
        return false;
    }

    SMBEngine engine(romImage, /* enableAudio */ false);
    engine.reset();

    runMovie(engine, frameCount, file);

    bool written = (ferror(file) == 0);
    if (!written)
    {
        std::cout << "Failed to write the RAM to \"" << fileName << "\"." << std::endl;
    }
    fclose(file);

    return written;
}

/**
 * Parse the comma-separated list of frame numbers given to --lag.
 */
static bool parseFrameList(const std::string& text, std::vector<int>& frames)
{
    std::istringstream list(text);
    std::string number;

    while (std::getline(list, number, ','))
    {
        char* end = nullptr;
        long frame = std::strtol(number.c_str(), &end, 10);

        if (number.empty() || *end != '\0' || frame < 0)
        {
            std::cout << "\"" << number << "\" is not a frame number.\n";
            return false;
        }

        frames.push_back(int(frame));
    }

    return !frames.empty();
}

static void printUsage()
{
    std::cout <<
        "usage:\n"
        "  smbc interactive [--mute]                      play the game with the keyboard\n"
        "  smbc movie <movie.fm2> [--mute]                play back an FCEUX movie\n"
        "  smbc screenshot <movie.fm2> <frame> [out.png]  save the screen on a frame of a movie\n"
        "  smbc ram <movie.fm2> <frame> [out.bin]         save the NES RAM of every frame up to one\n"
        "\n"
        "  --lag <frames>   the frames of the movie the console lagged on, comma separated,\n"
        "                   as \"1,2,3\". The game never read the controller on those frames,\n"
        "                   so playback ignores the movie's input for them, and stays in step\n"
        "                   with the recording. tools/lagframes.lua gets the list from FCEUX.\n"
        "  --scale <factor> how far to scale the interface up, as \"2\". Taken from the\n"
        "                   density of the display if not given.\n"
        "\n"
        "With no command, plays interactively.\n";
}

int main(int argc, char** argv)
{
    enum { COMMAND_INTERACTIVE, COMMAND_SCREENSHOT, COMMAND_RAM } command = COMMAND_INTERACTIVE;

    std::string movieFileName;
    std::string outputFileName;
    std::vector<int> lagFrames;
    int frameCount = 0;
    bool muted = false;
    float scale = 0.0f; // taken from the display unless --scale says otherwise

    // The flags are accepted anywhere; everything else is positional.
    //
    std::vector<std::string> arguments;
    for (int i = 1; i < argc; i++)
    {
        std::string argument = argv[i];
        if (argument == "--mute")
        {
            muted = true;
        }
        else if (argument == "--lag")
        {
            if (i + 1 >= argc || !parseFrameList(argv[++i], lagFrames))
            {
                std::cout << "--lag takes a comma-separated list of frame numbers, as \"1,2,3\".\n";
                return -1;
            }
        }
        else if (argument == "--scale")
        {
            char* end = nullptr;
            scale = (i + 1 < argc) ? std::strtof(argv[++i], &end) : 0.0f;

            if (end == nullptr || *end != '\0' || scale <= 0.0f)
            {
                std::cout << "--scale takes the factor to scale the interface by, as \"2\".\n";
                return -1;
            }
        }
        else
        {
            arguments.push_back(argument);
        }
    }

    if (!arguments.empty())
    {
        const std::string& commandName = arguments[0];
        size_t argumentCount = arguments.size() - 1;

        bool screenshot = (commandName == "screenshot");

        if (commandName == "interactive" && argumentCount == 0)
        {
            command = COMMAND_INTERACTIVE;
        }
        else if (commandName == "movie" && argumentCount == 1)
        {
            command = COMMAND_INTERACTIVE;
            movieFileName = arguments[1];
        }
        else if ((screenshot || commandName == "ram") && (argumentCount == 2 || argumentCount == 3))
        {
            command = screenshot ? COMMAND_SCREENSHOT : COMMAND_RAM;
            movieFileName = arguments[1];
            frameCount = std::atoi(arguments[2].c_str());
            outputFileName = (argumentCount == 3)
                ? arguments[3]
                : (screenshot ? "smbc-screenshot.png" : "smbc-ram.bin");

            if (frameCount <= 0)
            {
                std::cout << "The frame to capture must be a positive number of frames into the movie.\n";
                return -1;
            }
        }
        else
        {
            printUsage();
            return -1;
        }
    }

    if (!loadRomImage())
    {
        return -1;
    }

    if (!movieFileName.empty() && !movie.load(movieFileName))
    {
        std::cout << "Failed to load the movie. Please check previous error messages for more information. The program will now exit.\n";
        return -1;
    }

    if (!lagFrames.empty())
    {
        if (!movie.isLoaded())
        {
            std::cout << "--lag says which frames of a movie to ignore the input of, and there is no movie to play back.\n";
            return -1;
        }

        movie.setLagFrames(lagFrames);
    }

    // The capture commands play the movie back as fast as the engine will run it,
    // with nothing to show it on and nothing to play its audio, so they need no
    // SDL at all: they run without a display, and are not throttled to 60Hz.
    //
    if (command == COMMAND_SCREENSHOT)
    {
        return captureScreenshot(frameCount, outputFileName) ? 0 : -1;
    }
    if (command == COMMAND_RAM)
    {
        return captureRam(frameCount, outputFileName) ? 0 : -1;
    }

    if (!initialize(/* enableAudio */ !muted, scale))
    {
        std::cout << "Failed to initialize. Please check previous error messages for more information. The program will now exit.\n";
        return -1;
    }

    mainLoop(/* enableAudio */ !muted);

    shutdown();

    return 0;
}
