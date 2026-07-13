-- Record the console's side of the comparison: the 2KB of NES RAM at every NMI,
-- which is once per iteration of the game's logic, for a whole movie.
--
--   SMB_REF=fceux.bin fceux --loadlua tools/nmifull.lua --playmov movie.fm2 rom.nes
--
-- The flag is --playmov. FCEUX ignores an unrecognised one and runs with no movie
-- at all, recording the attract-mode demo without complaining.
--
-- Writes one 2KB record per iteration, and alongside it a .idx file holding the
-- movie frame each record was taken on. The gaps in that index are the frames the
-- console lagged: the NMI it missed is a frame the game never ran, and never read
-- the controller for. tools/predicate.py reads both.
--
-- RAM is zeroed first. FCEUX powers on with a fill pattern rather than zeroes, so
-- without this every cell the game never writes reads as garbage next to the
-- engine's zeroed RAM, and the comparison drowns in differences that mean nothing.
local nmi = memory.readbyte(0xfffa) + memory.readbyte(0xfffb) * 256
local path = assert(os.getenv("SMB_REF"), "set SMB_REF to the file to write")

local out = assert(io.open(path, "wb"))
local index = assert(io.open(path .. ".idx", "w"))

for address = 0, 0x7ff do memory.writebyte(address, 0) end

memory.registerexec(nmi, function()
    index:write(emu.framecount() .. "\n")
    out:write(memory.readbyterange(0, 0x800))
end)

for _ = 1, 71438 do emu.frameadvance() end

out:close()
index:close()
emu.exit()
