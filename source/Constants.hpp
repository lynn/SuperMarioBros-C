/**
 * @file
 * @brief defines program constants
 */
#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

/**
 * Application title.
 */
#define APP_TITLE "Super Mario Bros."

/**
 * The path to the Super Mario Bros. ROM image, relative to the working directory.
 */
#define ROM_FILE_NAME "Super Mario Bros. (JU) (PRG0) [!].nes"

/**
 * Frequency of sampled audio output, in Hz.
 */
#define AUDIO_FREQUENCY 48000

/**
 * Frame rate of the game, in frames per second.
 */
#define FRAME_RATE 99999

/**
 * Width of the virtual screen.
 */
#define RENDER_WIDTH 256

/**
 * Height of the virtual screen.
 */
#define RENDER_HEIGHT 240

/**
 * Scaling factor for rendering: the window is this many times the size of the
 * virtual screen.
 */
#define RENDER_SCALE 3

/**
 * MilliSeconds per second
 */
#define MS_PER_SEC 1000

#endif // CONSTANTS_HPP
