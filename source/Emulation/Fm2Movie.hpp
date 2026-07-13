#ifndef FM2MOVIE_HPP
#define FM2MOVIE_HPP

#include <cstdint>
#include <set>
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
     * This is only an estimate of where a movie's first frame of input is, and
     * it is only used when the movie's lag frames are not known. They are the
     * exact answer: the console powers on with the game's logic not running and
     * so reading no controller, which is what a lag frame is, and the boot
     * frames of a movie are the lag frames at the start of it. See getFirstFrame()
     * and setLagFrames().
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
     * Get the frame of the movie that playback starts at: the first one whose
     * input the game's logic ran to read.
     *
     * The frames before it elapsed while the console was powering on, and the
     * engine has no equivalent of them to play them back into.
     */
    int getFirstFrame() const;

    /**
     * Get the number of frames of input in the movie.
     */
    int getFrameCount() const;

    /**
     * Get whether the movie has been told which of its frames the console
     * lagged on.
     */
    bool hasLagFrames() const;

    /**
     * Get whether the console lagged on a frame, so that the row of input the
     * movie holds for it is one that was never read.
     */
    bool isLagFrame(int frameIndex) const;

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

    /**
     * Tell the movie which of its frames the console lagged on.
     *
     * The game runs inside the NMI handler, and when a frame's work overruns the
     * handler the console misses the next NMI: the game's logic never runs for
     * that frame, and never reads the controller for it. The movie still records
     * a row of input for it, and that row is one the console never used, so
     * playback has to ignore it. The engine runs the game as compiled C++ and has
     * no notion of cycles, so it cannot work out where those frames are; only the
     * console can say, and tools/lagframes.lua asks it.
     *
     * With the lag frames of a movie given here, playback stays in step with the
     * recording for the whole of it. Without them, it drifts by a frame at each
     * one that goes unaccounted for, and a movie whose inputs are frame-perfect
     * (a tool-assisted speedrun) eventually plays back as nonsense.
     *
     * @param frameIndices the frames of the movie the console lagged on.
     */
    void setLagFrames(const std::vector<int>& frameIndices);

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
    std::set<int> lagFrames;
    bool loaded;
};

#endif // FM2MOVIE_HPP
