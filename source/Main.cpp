#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <SDL2/SDL.h>

#include "Emulation/Controller.hpp"
#include "Emulation/Fm2Movie.hpp"
#include "SMB/SMBEngine.hpp"
#include "Util/Video.hpp"

#include "Constants.hpp"

uint8_t* romImage;
static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* texture;
static SDL_Texture* scanlineTexture;
static SMBEngine* smbEngine = nullptr;
static Fm2Movie movie;
static uint32_t renderBuffer[RENDER_WIDTH * RENDER_HEIGHT];

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
static bool initialize(bool enableAudio)
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        std::cout << "SDL_Init() failed during initialize(): " << SDL_GetError() << std::endl;
        return false;
    }

    // Create the window
    window = SDL_CreateWindow(APP_TITLE,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              RENDER_WIDTH * RENDER_SCALE,
                              RENDER_HEIGHT * RENDER_SCALE,
                              0);
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

    if (SDL_RenderSetLogicalSize(renderer, RENDER_WIDTH, RENDER_HEIGHT) < 0)
    {
        std::cout << "SDL_RenderSetLogicalSize() failed during initialize(): " << SDL_GetError() << std::endl;
        return false;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, RENDER_WIDTH, RENDER_HEIGHT);
    if (texture == nullptr)
    {
        std::cout << "SDL_CreateTexture() failed during initialize(): " << SDL_GetError() << std::endl;
        return false;
    }

    scanlineTexture = generateScanlineTexture(renderer);

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

    SDL_DestroyTexture(scanlineTexture);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
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
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
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

        const Uint8* keys = SDL_GetKeyboardState(NULL);
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

        SDL_RenderClear(renderer);

        // Render the screen
        SDL_RenderSetLogicalSize(renderer, RENDER_WIDTH, RENDER_HEIGHT);
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        // Render scanlines
        SDL_RenderSetLogicalSize(renderer, RENDER_WIDTH * 3, RENDER_HEIGHT * 3);
        SDL_RenderCopy(renderer, scanlineTexture, NULL, NULL);

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

    if (!initialize(/* enableAudio */ !muted))
    {
        std::cout << "Failed to initialize. Please check previous error messages for more information. The program will now exit.\n";
        return -1;
    }

    mainLoop(/* enableAudio */ !muted);

    shutdown();

    return 0;
}
