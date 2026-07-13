#ifndef FM2MOVIE_HPP
#define FM2MOVIE_HPP

#include <cstdint>
#include <string>
#include <vector>

class Controller;

/**
 * Playback of an FCEUX movie (.fm2) file.
 *
 * A movie is a list of the controller inputs that were recorded for each frame
 * of a session, starting from power-on. Replaying those inputs into the
 * controllers reproduces the recorded session.
 */
class Fm2Movie
{
public:
    /**
     * The number of frames of a movie that elapse while the console powers on,
     * before the game's first frame of logic runs.
     *
     * A movie records one frame of input per frame of the console, but the game
     * only reads the controller once per frame of its own logic, and the two do
     * not run in lockstep: the console spends its first frames initializing,
     * with the game's engine not running at all. SMBEngine::reset() performs
     * that initialization instantly instead, so playback has to skip the frames
     * of the movie that elapsed during it, or every input in the movie lands
     * several frames early and the recording plays back as nonsense.
     *
     * Measured against FCEUX by dumping the 2KB of NES RAM at every frame of
     * both: with this value, the engine's RAM matches FCEUX's byte for byte.
     * (See also SMBEngine::isLagFrame(), which keeps the two in step after the
     * console has booted.)
     */
    static const int BOOT_FRAMES = 6;

    Fm2Movie();

    /**
     * Apply the inputs recorded for a frame to the controllers.
     *
     * @param frameIndex the frame of the movie to play back.
     */
    void applyFrame(int frameIndex, Controller& controller1, Controller& controller2) const;

    /**
     * Get the number of frames of input in the movie.
     */
    int getFrameCount() const;

    /**
     * Get whether a movie is currently loaded for playback.
     */
    bool isLoaded() const;

    /**
     * Get whether the movie asks for the console to be reset before a frame is
     * played back.
     */
    bool isResetFrame(int frameIndex) const;

    /**
     * Load a movie from an .fm2 file.
     *
     * @return true if the movie was loaded successfully.
     */
    bool load(const std::string& fileName);

private:
    /**
     * The inputs recorded for a single frame.
     */
    struct Frame
    {
        uint8_t commands; /**< Console commands (reset, etc.) requested for the frame. */
        uint8_t port0;    /**< Buttons held on the controller in port 0, indexed by ControllerButton. */
        uint8_t port1;    /**< Buttons held on the controller in port 1, indexed by ControllerButton. */
    };

    std::vector<Frame> frames;
    bool loaded;
};

#endif // FM2MOVIE_HPP
