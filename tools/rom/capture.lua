-- MAME Lua capture for the procedure in
-- docs/archaeology/phase-0b/traces/README.md §3.
--
-- This script has not been executed in Phase 1: no CoCo emulator was
-- installed, and no retail ROM was available. It is the procedure, written
-- so a later run does not invent a second one.
--
-- Usage (illustrative; confirm against the MAME version actually installed):
--   mame coco -cart build/rom/daggorath.bin -autoboot_script tools/rom/capture.lua
--
-- Required environment (set before launch, or edit the locals below):
--   DOD_SYMBOLS   TSV "symbol<TAB>hex-address" from the lwasm listing
--   DOD_WATCHES   tools/rom/watchlist.tsv
--   DOD_SCRIPT    a Phase 0b "<jiffy> <KEY>" script
--   DOD_TRACE     where to write the interpreted trace
--   DOD_RAW       where to write the raw per-jiffy samples
--
-- Raw samples and the interpreted trace are different files. The raw file is
-- the observation; the trace is a reading of it in the dcli column format.
-- Do not hand-edit either.

local function env(name, default)
    local v = os.getenv(name)
    if v and v ~= "" then return v end
    return default
end

local symbols_path = env("DOD_SYMBOLS", "build/rom/symbols.tsv")
local watch_path = env("DOD_WATCHES", "tools/rom/watchlist.tsv")
local script_path = env("DOD_SCRIPT", "")
local trace_path = env("DOD_TRACE", "build/rom/capture.trace")
local raw_path = env("DOD_RAW", "build/rom/capture.raw.tsv")

local function die(msg)
    io.stderr:write("capture.lua: " .. msg .. "\n")
    os.exit(1)
end

local function load_symbols(path)
    local f = io.open(path, "r")
    if not f then die("cannot open symbols " .. path) end
    local map = {}
    for line in f:lines() do
        if line ~= "" and line:sub(1, 1) ~= "#" then
            local name, addr = line:match("^(%S+)%s+(%x+)%s*$")
            if name and addr then map[name] = tonumber(addr, 16) end
        end
    end
    f:close()
    return map
end

local function load_watches(path, symbols)
    local f = io.open(path, "r")
    if not f then die("cannot open watchlist " .. path) end
    local rows = {}
    for line in f:lines() do
        if line ~= "" and line:sub(1, 1) ~= "#" then
            local name, width = line:match("^(%S+)\t([%d%-]+)\t")
            if name and width then
                local addr = symbols[name]
                if not addr then die("symbol " .. name .. " is not in " .. symbols_path) end
                table.insert(rows, { name = name, addr = addr, width = tonumber(width) })
            end
        end
    end
    f:close()
    return rows
end

local function load_script(path)
    local keys = {}
    if path == "" then return keys end
    local f = io.open(path, "r")
    if not f then die("cannot open script " .. path) end
    for line in f:lines() do
        local hash = line:find("#")
        if hash then line = line:sub(1, hash - 1) end
        local jiffy, key = line:match("^%s*(%d+)%s+(%S+)%s*$")
        if jiffy and key then
            keys[#keys + 1] = { jiffy = tonumber(jiffy), key = key }
        end
    end
    f:close()
    table.sort(keys, function(a, b) return a.jiffy < b.jiffy end)
    return keys
end

-- MAME 0.264 coco_keyboard in src/mame/trs/coco12.cpp. Each entry is the
-- root ioport tag and the PORT_BIT mask. port:field(mask) selects that bit.
-- set_value(1) asserts the key; the port is active-low. Checked against that
-- source. Not executed: MAME exited before a frame, so the ":rowN" tag
-- spelling is still unconfirmed on a live machine.
-- Keys for a jiffy are asserted at the start of that frame and cleared at the
-- start of the next one.
local KEY_PORT = {
    ["0"] = { ":row4", 0x01 }, ["1"] = { ":row4", 0x02 }, ["2"] = { ":row4", 0x04 },
    ["3"] = { ":row4", 0x08 }, ["4"] = { ":row4", 0x10 }, ["5"] = { ":row4", 0x20 },
    ["6"] = { ":row4", 0x40 }, ["7"] = { ":row4", 0x80 },
    ["8"] = { ":row5", 0x01 }, ["9"] = { ":row5", 0x02 },
    A = { ":row0", 0x02 }, B = { ":row0", 0x04 }, C = { ":row0", 0x08 },
    D = { ":row0", 0x10 }, E = { ":row0", 0x20 }, F = { ":row0", 0x40 },
    G = { ":row0", 0x80 },
    H = { ":row1", 0x01 }, I = { ":row1", 0x02 }, J = { ":row1", 0x04 },
    K = { ":row1", 0x08 }, L = { ":row1", 0x10 }, M = { ":row1", 0x20 },
    N = { ":row1", 0x40 }, O = { ":row1", 0x80 },
    P = { ":row2", 0x01 }, Q = { ":row2", 0x02 }, R = { ":row2", 0x04 },
    S = { ":row2", 0x08 }, T = { ":row2", 0x10 }, U = { ":row2", 0x20 },
    V = { ":row2", 0x40 }, W = { ":row2", 0x80 },
    X = { ":row3", 0x01 }, Y = { ":row3", 0x02 }, Z = { ":row3", 0x04 },
    SPACE = { ":row3", 0x80 }, CR = { ":row6", 0x01 }, BS = { ":row3", 0x20 },
}

local symbols = load_symbols(symbols_path)
local watches = load_watches(watch_path, symbols)
local script = load_script(script_path)

-- Host check of the file parsers and the key table. This does not boot a CoCo
-- and does not validate MAME ioport tags. Set DOD_SELFTEST=1.
if os.getenv("DOD_SELFTEST") == "1" then
    local need = { "A", "Z", "SPACE", "CR", "BS" }
    for _, key in ipairs(need) do
        local spec = KEY_PORT[key]
        if not spec or type(spec[2]) ~= "number" or spec[2] < 1 then
            die("KEY_PORT has no mask for " .. key)
        end
    end
    if #watches < 1 then die("watchlist produced no rows") end
    for _, ev in ipairs(script) do
        if not KEY_PORT[ev.key] then die("no matrix entry for key " .. ev.key) end
    end
    io.write(string.format("selftest ok symbols=%d watches=%d script_keys=%d\n",
        (function() local n = 0; for _ in pairs(symbols) do n = n + 1 end; return n end)(),
        #watches, #script))
    return
end

local script_at = 1
local jiffy = 0
local prev = {}

local raw = io.open(raw_path, "w")
local trace = io.open(trace_path, "w")
if not raw or not trace then die("cannot open capture outputs") end
raw:write("# jiffy\tsymbol\thex\n")
trace:write("# jiffy\tclock\tevent\tdetail\n")

local function read_bytes(mem, addr, width)
    if width < 1 then return "" end
    local hex = {}
    for i = 0, width - 1 do
        hex[#hex + 1] = string.format("%02X", mem:read_u8(addr + i))
    end
    return table.concat(hex)
end

local function sample()
    local machine = manager.machine
    local cpu = machine.devices[":maincpu"]
    if not cpu then die("no :maincpu device; start the coco or coco3 driver") end
    local mem = cpu.spaces["program"]
    local values = {}
    for _, w in ipairs(watches) do
        if w.width > 0 then
            local hex = read_bytes(mem, w.addr, w.width)
            values[w.name] = hex
            raw:write(string.format("%d\t%s\t%s\n", jiffy, w.name, hex))
        end
    end
    local clock = string.format("%d:%d:%d.%d.%d",
        tonumber(values.HOUR or "0", 16) or 0,
        tonumber(values.MINUTE or "0", 16) or 0,
        tonumber(values.SECOND or "0", 16) or 0,
        tonumber(values.TENTH or "0", 16) or 0,
        tonumber(values.JIFFY or "0", 16) or 0)
    local function emit(kind, detail)
        trace:write(string.format("%d\t%s\t%s\t%s\n", jiffy, clock, kind, detail))
    end
    if jiffy == 0 then
        emit("INIT", "rom-capture")
    end
    -- Interpretation, kept in this file and not in the raw sample.
    local function changed(name)
        return values[name] and prev[name] and values[name] ~= prev[name]
    end
    if changed("PROW") or changed("PCOL") then
        emit("MOVE", string.format("row=%d col=%d",
            tonumber(values.PROW, 16), tonumber(values.PCOL, 16)))
    end
    if changed("PDIR") then
        emit("TURN", "dir=" .. tostring(tonumber(values.PDIR, 16)))
    end
    for name, hex in pairs(values) do prev[name] = hex end
    jiffy = jiffy + 1
end

local held = {}

local function release_held()
    for i = 1, #held do
        held[i]:clear_value()
    end
    held = {}
end

-- Press this jiffy's keys before the frame runs, and drop them before the next.
local function inject()
    local machine = manager.machine
    release_held()
    while script_at <= #script and script[script_at].jiffy == jiffy do
        local spec = KEY_PORT[script[script_at].key]
        if not spec then
            die("no matrix entry for key " .. script[script_at].key)
        end
        local port = machine.ioport.ports[spec[1]]
        if not port then
            die("ioport " .. spec[1] .. " missing; the :rowN tag spelling is unconfirmed")
        end
        local field = port:field(spec[2])
        if not field then
            die(string.format("ioport %s has no field mask 0x%02X", spec[1], spec[2]))
        end
        field:set_value(1)
        held[#held + 1] = field
        script_at = script_at + 1
    end
end

if emu and emu.register_frame and emu.register_frame_done then
    emu.register_frame(inject)
    emu.register_frame_done(sample)
else
    die("this MAME build has no emu.register_frame / emu.register_frame_done")
end
