-- MAME Lua capture for the procedure in
-- docs/archaeology/phase-0b/traces/README.md §3.
--
-- This script has not been executed in Phase 1: no CoCo emulator was
-- installed, and no retail ROM was available. It is the procedure, written
-- so a later run does not invent a second one.
--
-- Usage (illustrative; confirm against the MAME version actually installed):
--   mame coco2 -cart build/rom/daggorath.bin -autoboot_script tools/rom/capture.lua
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

-- CoCo keyboard matrix columns as MAME ioport tags. POLCAT scans rows.
-- One injected key is held for a single 60 Hz frame, which is one jiffy.
local KEY_PORT = {
    ["0"] = { ":row0", "0" }, ["1"] = { ":row0", "1" }, ["2"] = { ":row0", "2" },
    ["3"] = { ":row0", "3" }, ["4"] = { ":row0", "4" }, ["5"] = { ":row0", "5" },
    ["6"] = { ":row0", "6" }, ["7"] = { ":row0", "7" },
    ["8"] = { ":row1", "0" }, ["9"] = { ":row1", "1" },
    A = { ":row1", "2" }, B = { ":row1", "3" }, C = { ":row1", "4" },
    D = { ":row1", "5" }, E = { ":row1", "6" }, F = { ":row1", "7" },
    G = { ":row2", "0" }, H = { ":row2", "1" }, I = { ":row2", "2" },
    J = { ":row2", "3" }, K = { ":row2", "4" }, L = { ":row2", "5" },
    M = { ":row2", "6" }, N = { ":row2", "7" },
    O = { ":row3", "0" }, P = { ":row3", "1" }, Q = { ":row3", "2" },
    R = { ":row3", "3" }, S = { ":row3", "4" }, T = { ":row3", "5" },
    U = { ":row3", "6" }, V = { ":row3", "7" },
    W = { ":row4", "0" }, X = { ":row4", "1" }, Y = { ":row4", "2" },
    Z = { ":row4", "3" },
    SPACE = { ":row6", "5" }, CR = { ":row6", "0" }, BS = { ":row5", "6" },
}

local symbols = load_symbols(symbols_path)
local watches = load_watches(watch_path, symbols)
local script = load_script(script_path)

-- Host check of the file parsers and the key table. This does not boot a CoCo
-- and does not validate MAME ioport tags. Set DOD_SELFTEST=1.
if os.getenv("DOD_SELFTEST") == "1" then
    local need = { "A", "Z", "SPACE", "CR", "BS" }
    for _, key in ipairs(need) do
        if not KEY_PORT[key] then die("KEY_PORT has no entry for " .. key) end
    end
    if #watches < 1 then die("watchlist produced no rows") end
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
    if not cpu then die("no :maincpu device; start a coco2 or coco3 driver") end
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

    while script_at <= #script and script[script_at].jiffy == jiffy do
        local spec = KEY_PORT[script[script_at].key]
        if spec then
            local port = machine.ioport.ports[spec[1]]
            if port then
                port:field(spec[2]):set_value(1)
            else
                die("ioport " .. spec[1] .. " missing; the key matrix tags are unverified")
            end
        else
            die("no matrix entry for key " .. script[script_at].key)
        end
        script_at = script_at + 1
    end
    jiffy = jiffy + 1
end

if emu and emu.register_frame_done then
    emu.register_frame_done(sample)
elseif manager.machine.video and manager.machine.video.register_frame then
    manager.machine.video:register_frame(sample)
else
    die("this MAME build has neither emu.register_frame_done nor video:register_frame")
end
