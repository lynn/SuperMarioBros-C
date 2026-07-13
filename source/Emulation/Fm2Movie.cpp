#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "Controller.hpp"
#include "Fm2Movie.hpp"

/**
 * Console commands that a frame can request, as a bit mask.
 */
#define FM2_COMMAND_SOFT_RESET 0x01
#define FM2_COMMAND_HARD_RESET 0x02

/**
 * Split a line of text into the fields delimited by '|'.
 *
 * A frame of input looks like "|commands|port0|port1|port2|", so the first
 * field is always the (empty) text preceding the leading delimiter.
 */
static std::vector<std::string> splitFields(const std::string& line)
{
    std::vector<std::string> fields;
    std::istringstream stream(line);
    std::string field;

    while (std::getline(stream, field, '|'))
    {
        fields.push_back(field);
    }

    return fields;
}

/**
 * Parse the buttons held on a controller for a frame.
 *
 * Buttons are recorded as a fixed sequence of characters, one per button. A
 * button is held if its character is anything other than '.' or a space.
 */
static uint8_t parseButtons(const std::string& field)
{
    static const ControllerButton buttonOrder[] = {
        BUTTON_RIGHT,
        BUTTON_LEFT,
        BUTTON_DOWN,
        BUTTON_UP,
        BUTTON_START,
        BUTTON_SELECT,
        BUTTON_B,
        BUTTON_A
    };
    const size_t buttonCount = sizeof(buttonOrder) / sizeof(buttonOrder[0]);

    uint8_t buttons = 0;
    for (size_t i = 0; i < buttonCount && i < field.length(); i++)
    {
        if (field[i] != '.' && field[i] != ' ')
        {
            buttons |= 1 << buttonOrder[i];
        }
    }

    return buttons;
}

Fm2Movie::Fm2Movie() :
    loaded(false)
{
}

void Fm2Movie::applyFrame(int frameIndex, Controller& controller1, Controller& controller2) const
{
    if (frameIndex < 0 || frameIndex >= getFrameCount())
    {
        return;
    }

    const Frame& frame = frames[frameIndex];
    for (int button = BUTTON_A; button <= BUTTON_RIGHT; button++)
    {
        ControllerButton controllerButton = static_cast<ControllerButton>(button);
        controller1.setButtonState(controllerButton, (frame.port0 & (1 << button)) != 0);
        controller2.setButtonState(controllerButton, (frame.port1 & (1 << button)) != 0);
    }
}

int Fm2Movie::getFirstFrame() const
{
    // The boot frames are lag frames, and playback skips every lag frame, so a
    // movie whose lag frames are known needs no estimate of where they end: it
    // starts at the top and the skipping takes care of the rest.
    //
    return hasLagFrames() ? 0 : BOOT_FRAMES;
}

int Fm2Movie::getFrameCount() const
{
    return int(frames.size());
}

bool Fm2Movie::hasLagFrames() const
{
    return !lagFrames.empty();
}

bool Fm2Movie::isLagFrame(int frameIndex) const
{
    return lagFrames.find(frameIndex) != lagFrames.end();
}

bool Fm2Movie::isLoaded() const
{
    return loaded;
}

bool Fm2Movie::isResetFrame(int frameIndex) const
{
    if (frameIndex < 0 || frameIndex >= getFrameCount())
    {
        return false;
    }

    return (frames[frameIndex].commands & (FM2_COMMAND_SOFT_RESET | FM2_COMMAND_HARD_RESET)) != 0;
}

bool Fm2Movie::load(const std::string& fileName)
{
    std::ifstream file(fileName.c_str());
    if (!file.good())
    {
        std::cout << "Failed to open the movie file \"" << fileName << "\"." << std::endl;
        return false;
    }

    frames.clear();
    loaded = false;

    std::string line;
    while (std::getline(file, line))
    {
        // Movie files may have been written with Windows line endings
        //
        if (!line.empty() && line[line.length() - 1] == '\r')
        {
            line.erase(line.length() - 1);
        }

        if (line.empty())
        {
            continue;
        }

        if (line[0] != '|')
        {
            // Anything that isn't a frame of input is a "key value" header
            //
            std::istringstream header(line);
            std::string key;
            std::string value;
            header >> key >> value;

            if (key == "binary" && value != "0")
            {
                std::cout << "The movie file \"" << fileName << "\" has a binary input log, which is not supported." << std::endl;
                return false;
            }
            if (key == "savestate")
            {
                std::cout << "The movie file \"" << fileName << "\" starts from a savestate, which is not supported." << std::endl;
                return false;
            }
            if (key == "fourscore" && value != "0")
            {
                std::cout << "The movie file \"" << fileName << "\" was recorded with a Four Score, which is not supported." << std::endl;
                return false;
            }
            continue;
        }

        // The fields of a frame are "|commands|port0|port1|port2|", where the
        // ports of any controllers that weren't recorded are left empty
        //
        std::vector<std::string> fields = splitFields(line);

        Frame frame;
        frame.commands = (fields.size() > 1) ? uint8_t(std::atoi(fields[1].c_str())) : 0;
        frame.port0 = (fields.size() > 2) ? parseButtons(fields[2]) : 0;
        frame.port1 = (fields.size() > 3) ? parseButtons(fields[3]) : 0;
        frames.push_back(frame);
    }

    if (frames.empty())
    {
        std::cout << "The movie file \"" << fileName << "\" does not contain any frames of input." << std::endl;
        return false;
    }

    loaded = true;

    std::cout << "Loaded the movie file \"" << fileName << "\" (" << getFrameCount() << " frames)." << std::endl;

    return true;
}

void Fm2Movie::setLagFrames(const std::vector<int>& frameIndices)
{
    lagFrames.clear();
    lagFrames.insert(frameIndices.begin(), frameIndices.end());
}
