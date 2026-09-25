-- MAME Lua capture for the procedure in
-- docs/archaeology/phase-0b/traces/README.md §3.
--
-- Run by tools/rom/run-capture.sh against MAME 0.264 coco2b:
--   mame coco2b -cart <26-3093 image> -autoboot_script tools/rom/capture.lua
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

-- Keyboard matrix from MAME 0.264 coco_keyboard in src/mame/trs/coco12.cpp.
-- ":rowN" is the PIA0 port A input bit (the matrix row) and the mask is the
-- PIA0 port B strobe bit (the column). Keys are not injected through MAME's
-- ioport system, whose set_value takes effect at the next frame update.
-- Instead a read tap on $FF00 pulls a pressed key's row bit low whenever the
-- game has strobed that key's column low through $FF02 (§3.3: inject at the
-- PIA level).
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
local prev = {}

local raw = io.open(raw_path, "w")
local trace = io.open(trace_path, "w")
if not raw or not trace then die("cannot open capture outputs") end
raw:write("# isr\tsymbol\thex\n")
trace:write("# jiffy\tclock\tevent\tdetail\n")

local function read_bytes(mem, addr, width)
    local hex = {}
    for i = 0, width - 1 do
        hex[#hex + 1] = string.format("%02X", mem:read_u8(addr + i))
    end
    return table.concat(hex)
end

local DIRS = { [0] = "N", "E", "S", "W" }

-- Raw: every watch at game interrupt `isr`, read when CLOCK first writes JIFFY,
-- so the clock bytes are still the pre-bump values. Sample 0 is the state at
-- scheduler entry and is reported as INIT. A later sample is the state the
-- foreground left at the end of jiffy isr-1, which is how the trace labels it.
local function sample(mem, isr)
    local values = {}
    for _, w in ipairs(watches) do
        -- Width "-" (TCBLND, CCBLND) is a structure walk, not yet implemented.
        if w.width and w.width > 0 then
            local hex = read_bytes(mem, w.addr, w.width)
            values[w.name] = hex
            raw:write(string.format("%d\t%s\t%s\n", isr, w.name, hex))
        end
    end
    local jiffy = isr == 0 and 0 or isr - 1
    local function num(name) return tonumber(values[name] or "0", 16) or 0 end
    local clock = string.format("%d:%d:%d.%d.%d",
        num("HOUR"), num("MINUTE"), num("SECOND"), num("TENTH"), num("JIFFY"))
    local function emit(kind, detail)
        trace:write(string.format("%d\t%s\t%s\t%s\n", jiffy, clock, kind, detail))
    end
    -- Interpretation, kept in this file and not in the raw sample. Only state
    -- visible in RAM is reported, at the jiffy the change is first seen.
    if isr == 0 then
        emit("INIT", string.format("level=%d row=%d col=%d dir=%s second=%d",
            num("LEVEL"), num("PROW"), num("PCOL"), DIRS[num("PDIR") % 4], num("SECOND")))
    end
    local function changed(name)
        return values[name] and prev[name] and values[name] ~= prev[name]
    end
    if isr == 0 then
        for name, hex in pairs(values) do prev[name] = hex end
        return
    end
    if changed("PDIR") then
        emit("TURN", "dir=" .. DIRS[num("PDIR") % 4])
    end
    if changed("PROW") or changed("PCOL") then
        emit("MOVE", string.format("row=%d col=%d", num("PROW"), num("PCOL")))
    end
    if changed("PDAM") then
        emit("EXERT", string.format("damage=%d heart_rate=%d", num("PDAM"), num("HEARTR")))
    end
    for name, hex in pairs(values) do prev[name] = hex end
end

local jiffy_limit = tonumber(os.getenv("DOD_JIFFIES") or "")
local cpu = manager.machine.devices[":maincpu"]
if not cpu then die("no :maincpu device; start the coco2b driver") end
local mem = cpu.spaces["program"]
local function sym(name)
    local a = symbols[name]
    if not a then die("symbol " .. name .. " is not in " .. symbols_path) end
    return a
end
local JIFFY, AUTFLG = sym("JIFFY"), sym("AUTFLG")
local CLOCK, CLK50, GAME50 = sym("CLOCK"), sym("CLK50"), sym("GAME50")

-- Phases: "boot" until the autoplay demo sets AUTFLG; "abort" holds SPACE until
-- the demo's CLOCK sees it and transfers to GAME; "build" while GAME10..GAME50
-- builds level 0 with the clock already running; "game" counts interrupts from
-- the first CLOCK after GAME50 is fetched, which falls into SCHED. That
-- interrupt is jiffy 0: the reference slice emits INIT at scheduler entry.
-- The alignment is inferred, not source-proven. The build interrupts are
-- counted in build_isrs and reported, not hidden.
local phase = "boot"
local isr = -1          -- game interrupt counter; the first CLOCK after GAME50 is isr 0
local build_isrs = 0    -- interrupts between GAME10's IRQSYN and GAME50
local last_isr_time = nil
local pressed = {}      -- row index -> column mask currently held
local strobe = 0xFF     -- last value written to PIA0 port B ($FF02)

local function press(key)
    local spec = KEY_PORT[key]
    if not spec then die("no matrix entry for key " .. key) end
    local row = tonumber(spec[1]:match("(%d)$"))
    pressed[row] = (pressed[row] or 0) | spec[2]
end

local function release_all() pressed = {} end

local function on_clock()
    if phase == "boot" then
        if mem:read_u8(AUTFLG) ~= 0 then
            phase = "abort"
            press("SPACE")
        end
        return
    end
    if phase == "abort" then
        if mem:read_u8(AUTFLG) == 0 then
            release_all()
            phase = "build"
        else
            return
        end
    end
    if phase == "build" then
        build_isrs = build_isrs + 1
        return
    end
    isr = isr + 1
    sample(mem, isr)
    if jiffy_limit and isr - 1 >= jiffy_limit then
        raw:flush()
        trace:flush()
        manager.machine:exit()
        return
    end
    -- Keys for jiffy isr are down for this interrupt's keyboard scan (CLK60
    -- runs after the timer bump) and released at the next interrupt.
    release_all()
    while script_at <= #script and script[script_at].jiffy <= isr do
        if script[script_at].jiffy == isr then press(script[script_at].key) end
        script_at = script_at + 1
    end
end

-- One call per interrupt: the first JIFFY write inside CLOCK. A rollover
-- writes JIFFY twice in the same interrupt; the 8 ms gap check drops it.
local taps = {}
taps[#taps + 1] = mem:install_write_tap(JIFFY, JIFFY, "dod_jiffy", function(offset, data, mask)
    local pc = cpu.state["PC"].value
    if pc < CLOCK or pc > CLK50 then return end
    local now = manager.machine.time:as_double()
    if last_isr_time and now - last_isr_time < 0.008 then return end
    last_isr_time = now
    on_clock()
end)
taps[#taps + 1] = mem:install_write_tap(0xFF02, 0xFF02, "dod_strobe", function(offset, data, mask)
    strobe = data & 0xFF
end)
taps[#taps + 1] = mem:install_read_tap(0xFF00, 0xFF00, "dod_rows", function(offset, data, mask)
    local low = 0
    for row, cols in pairs(pressed) do
        if (cols & ~strobe & 0xFF) ~= 0 then low = low | (1 << row) end
    end
    if low == 0 then return data end
    return data & ~low
end)
-- Opcode fetch of GAME50: level 0 is built and SCHED is next.
taps[#taps + 1] = mem:install_read_tap(GAME50, GAME50, "dod_game50", function(offset, data, mask)
    if phase == "build" then
        phase = "game"
        trace:write(string.format("# level build: %d interrupts from GAME10 IRQSYN to GAME50\n", build_isrs))
    end
    return data
end)
_G.dod_capture_taps = taps
