-- Record the frames of a movie that the console lagged on.
--
--   SMB_LAG=lag.txt fceux --loadlua tools/lagframes.lua --playmov movie.fm2 rom.nes
--
-- A lag frame is one whose work overran the NMI handler, so the console missed
-- the next NMI: the game's logic never ran for it, and it never read the
-- controller. The movie still holds a row of input for it, and that row is one
-- the console never used. emu.lagged() is FCEUX's own flag for exactly that.
--
-- Writes one frame number per line, which is what smbc's --lag option takes.
--
-- The flag is --playmov. FCEUX ignores an unrecognised one and runs with no movie
-- at all, reporting the attract-mode demo's lag without complaining.
local path = assert(os.getenv("SMB_LAG"), "set SMB_LAG to the file to write")
local out = assert(io.open(path, "w"))

emu.speedmode("maximum") -- there is nothing to watch; do not run this at 60Hz

-- The flag is only meaningful once a frame has run, so it is read after the frame
-- advance, and belongs to the frame the counter held before it. Do not reach for
-- emu.framecount() after the advance instead: it is a frame further on, and the
-- whole list comes out one frame late.
--
-- The frames the console spends powering on are lag frames too, and are reported
-- as such: the game's logic is not running yet, so it is reading no controller,
-- which is all a lag frame is.
--
-- (This does not line up with the index that tools/nmifull.lua writes, which is a
-- frame later throughout. FCEUX has already counted the frame by the time the NMI
-- for it runs. The input the handler then reads is still the frame's own, so it is
-- this list, not the gaps in that index, that says which rows of a movie the
-- console read.)
for _ = 1, 71438 do
    local frame = emu.framecount()
    emu.frameadvance()

    if emu.lagged() then
        out:write(frame .. "\n")
    end
end

out:close()
emu.exit()
