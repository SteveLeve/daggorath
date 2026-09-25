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

local function beside(path, suffix)
    local stripped, n = path:gsub("%.raw%.tsv$", suffix)
    if n == 0 then return path .. suffix end
    return stripped
end

local task_path = env("DOD_TASKLOG", beside(raw_path, ".task.tsv"))
local spin_path = env("DOD_SPINLOG", beside(raw_path, ".spin.tsv"))
local sound_path = env("DOD_SOUNDLOG", beside(raw_path, ".sound.tsv"))
local pop_path = env("DOD_POPLOG", beside(raw_path, ".pop.tsv"))

local raw = io.open(raw_path, "w")
local trace = io.open(trace_path, "w")
local tasklog = io.open(task_path, "w")
local spinlog = io.open(spin_path, "w")
local soundlog = io.open(sound_path, "w")
local poplog = io.open(pop_path, "w")
if not raw or not trace or not tasklog or not spinlog or not soundlog or not poplog then
    die("cannot open capture outputs")
end
raw:write("# isr\tsymbol\thex\n")
trace:write("# jiffy\tclock\tevent\tdetail\n")
tasklog:write("# isr\tkind\taddr\tname\n")
spinlog:write("# phase\tspin\tentry_isr\texit_isr\tsecond_entry\tseed_entry\tseed_exit\tcalls\tnote\n")
soundlog:write("# kind\tisr\tpc\tseed\tsndrnd\tdetail\n")
poplog:write("# when\tphase\tisr\tlevel\tsecond\tseed\tcmxlnd\tcreatures\tmaze_sum\n")

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
local by_addr = {}
do
    local f = io.open(symbols_path, "r")
    for line in f:lines() do
        local name, addr = line:match("^(%S+)%s+(%x+)%s*$")
        if name and addr then
            local a = tonumber(addr, 16)
            if not by_addr[a] then by_addr[a] = name end
        end
    end
    f:close()
end

local function symbol_at(addr)
    return by_addr[addr] or string.format("$%04X", addr)
end

local HOUR, MINUTE, SECOND = sym("HOUR"), sym("MINUTE"), sym("SECOND")
local TENTH = sym("TENTH")
local SEED, SNDRND = sym("SEED"), sym("SNDRND")
local MAZLND = sym("MAZLND")
local LEVEL = sym("LEVEL")
local CMXLND, CCBLND = sym("CMXLND"), sym("CCBLND")
local LINBUF = sym("LINBUF")
local SCHED_JSR = sym("SCHED_JSR")
local HMAN50 = sym("HMAN50")
local CREGEN = sym("CREGEN")
local DGEN90 = sym("DGEN90")
local SNOISE = sym("SNOISE")
local NEWLVX = sym("NEWLVX")
local NLVL50 = sym("NLVL50")
local PSTEP = sym("PSTEP")

-- RTS immediately after the DGEN90 loop on this image is DGEN90+5
-- (SWI, FCB, DECB, BNE, RTS). Confirmed against the LWTOOLS 4.25 bytes.
local DGEN_RTS = DGEN90 + 5
local SNOISE_RTS = SNOISE + 11

local function find_byte(origin, byte, limit)
    for i = 0, limit - 1 do
        if mem:read_u8(origin + i) == byte then return origin + i end
    end
    return nil
end

local NEWLVL_RTS = find_byte(NLVL50, 0x39, 32)
if not NEWLVL_RTS then die("no RTS after NLVL50") end
local CREGEN_RTS = find_byte(CREGEN, 0x39, 40)
if not CREGEN_RTS then die("no RTS after CREGEN") end

-- PSTEP's blocked-move sound is SWI / ISOUND / A$THUD (3F 1B 14 on this image).
local THUD_SWI = nil
for i = 0, 48 do
    if mem:read_u8(PSTEP + i) == 0x3F
        and mem:read_u8(PSTEP + i + 1) == 0x1B
        and mem:read_u8(PSTEP + i + 2) == 0x14 then
        THUD_SWI = PSTEP + i
        break
    end
end
if not THUD_SWI then die("PSTEP has no ISOUND A$THUD") end
local THUD_RESUME = THUD_SWI + 3

local function u16(addr)
    return mem:read_u8(addr) * 256 + mem:read_u8(addr + 1)
end

local function hex_at(addr, width)
    return read_bytes(mem, addr, width)
end

local function clock_now()
    return string.format("%d:%d:%d.%d.%d",
        mem:read_u8(HOUR), mem:read_u8(MINUTE), mem:read_u8(SECOND),
        mem:read_u8(TENTH), mem:read_u8(JIFFY))
end

local function emit_trace(kind, detail)
    trace:write(string.format("%d\t%s\t%s\t%s\n", isr, clock_now(), kind, detail))
end

local function decode_line()
    local chars = {}
    for i = 0, 31 do
        local b = mem:read_u8(LINBUF + i)
        if b == 0xFF then break end
        if b == 0 then
            chars[#chars + 1] = " "
        elseif b >= 1 and b <= 26 then
            chars[#chars + 1] = string.char(string.byte("A") + b - 1)
        else
            chars[#chars + 1] = "?"
        end
    end
    return table.concat(chars)
end

-- Creature block: CD.ASM CC.LEN = 17, P.CCUSE +12, P.CCTYP +13, P.CCROW +15, P.CCCOL +16.
local CC_LEN = 17
local function creature_summary()
    local parts = {}
    local live = 0
    for slot = 0, 31 do
        local base = CCBLND + slot * CC_LEN
        local use = mem:read_u8(base + 12)
        if use ~= 0 then
            live = live + 1
            parts[#parts + 1] = string.format("%d:%d@%d,%d",
                slot, mem:read_u8(base + 13), mem:read_u8(base + 15), mem:read_u8(base + 16))
        end
    end
    return live, table.concat(parts, " ")
end

local function dump_pop(when)
    local live, creatures = creature_summary()
    local maze_sum = 0
    for i = 0, 1023 do
        maze_sum = (maze_sum + mem:read_u8(MAZLND + i)) % 4294967296
    end
    poplog:write(string.format("%s\t%s\t%d\t%d\t%d\t%s\t%s\t%d %s\tmaze_sum=%08X\n",
        when, phase, isr, mem:read_u8(LEVEL), mem:read_u8(SECOND),
        hex_at(SEED, 3), hex_at(CMXLND, 60), live, creatures, maze_sum))
    if when == "NEWLVL-exit" then
        local bytes = {}
        for i = 0, 1023 do
            bytes[#bytes + 1] = string.char(mem:read_u8(MAZLND + i))
        end
        local maze_path = beside(raw_path, string.format(".isr%s-L%s.maze.bin",
            isr, mem:read_u8(LEVEL)))
        local mf = io.open(maze_path, "wb")
        if mf then
            mf:write(table.concat(bytes))
            mf:close()
        end
    end
    poplog:flush()
end

local spin_open = false
local spin_index = 0
local spin_calls = 0
local spin_entry_isr = 0
local spin_second = 0
local spin_seed = ""
local poke_second = tonumber(os.getenv("DOD_POKE_SECOND") or "")
local poke_on = tonumber(os.getenv("DOD_POKE_ON_SPIN") or "")
local poke_note = ""

local snoise_seed = nil
local snoise_rnd = nil
local snoise_isr = nil
local thud_isr = nil
local thud_seed = nil
local thud_rnd = nil
local thud_snoise = 0
local thud_dac = 0
local dac_total = 0

local stop_name, stop_count, stop_tail = nil, nil, nil
do
    local spec = os.getenv("DOD_STOP") or ""
    local name, count, tail = spec:match("^([%w]+):(%d+):(%d+)$")
    if name then
        stop_name, stop_count, stop_tail = name, tonumber(count), tonumber(tail)
    end
end
local stop_seen = 0
local stop_at = nil
local task_runs = {}

local function note_task(name)
    task_runs[name] = (task_runs[name] or 0) + 1
    if stop_name and name == stop_name and isr >= 100 then
        stop_seen = stop_seen + 1
        if stop_seen == stop_count then
            stop_at = isr + stop_tail
        end
    end
end

-- Opcode fetch of GAME50: level 0 is built and SCHED is next.
taps[#taps + 1] = mem:install_read_tap(GAME50, GAME50, "dod_game50", function(offset, data, mask)
    if phase == "build" then
        phase = "game"
        trace:write(string.format("# level build: %d interrupts from GAME10 IRQSYN to GAME50\n", build_isrs))
        dump_pop("GAME50")
    end
    return data
end)

-- LDB SECOND just before DGEN90. A poke here is harness-modified state:
-- the load has not executed, so SECOND is what the spin will count.
taps[#taps + 1] = mem:install_read_tap(DGEN90 - 2, DGEN90 - 2, "dod_ldb_second", function(offset, data, mask)
    if data ~= 0xD6 then return data end
    if not poke_second or not poke_on then return data end
    -- This fetch is the start of a spin. spin_index increments on DGEN90.
    if spin_index + 1 == poke_on then
        mem:write_u8(SECOND, poke_second)
        poke_note = string.format("harness-modified SECOND=%d written at LDB SECOND before spin %d",
            poke_second, poke_on)
        spinlog:write("# " .. poke_note .. "\n")
        spinlog:flush()
    end
    return data
end)

taps[#taps + 1] = mem:install_read_tap(DGEN90, DGEN90, "dod_dgen90", function(offset, data, mask)
    if data ~= 0x3F then return data end
    if not spin_open then
        spin_open = true
        spin_index = spin_index + 1
        spin_calls = 1
        spin_entry_isr = isr
        spin_second = mem:read_u8(SECOND)
        spin_seed = hex_at(SEED, 3)
    else
        spin_calls = spin_calls + 1
    end
    return data
end)

taps[#taps + 1] = mem:install_read_tap(DGEN_RTS, DGEN_RTS, "dod_dgen_rts", function(offset, data, mask)
    if data ~= 0x39 or not spin_open then return data end
    spin_open = false
    local note = poke_note
    poke_note = ""
    spinlog:write(string.format("%s\t%d\t%d\t%d\t%d\t%s\t%s\t%d\t%s\n",
        phase, spin_index, spin_entry_isr, isr, spin_second, spin_seed,
        hex_at(SEED, 3), spin_calls, note))
    spinlog:flush()
    return data
end)

taps[#taps + 1] = mem:install_read_tap(SCHED_JSR, SCHED_JSR, "dod_sched", function(offset, data, mask)
    if phase ~= "game" or data ~= 0xAD then return data end
    local u = cpu.state["U"].value
    local rtn = u16(u + 3)
    local name = symbol_at(rtn)
    tasklog:write(string.format("%d\tTASK\t%04X\t%s\n", isr, rtn, name))
    emit_trace("TASK", "run " .. name)
    note_task(name)
    if name == "CREGEN" then dump_pop("CREGEN-entry") end
    return data
end)

taps[#taps + 1] = mem:install_read_tap(HMAN50, HMAN50, "dod_hman50", function(offset, data, mask)
    if phase ~= "game" or data ~= 0x8E then return data end
    local line = decode_line()
    tasklog:write(string.format("%d\tLINE\t%s\t%s\n", isr, "LINBUF", line))
    emit_trace("LINE", '"' .. line .. '"')
    tasklog:flush()
    return data
end)

taps[#taps + 1] = mem:install_read_tap(CREGEN_RTS, CREGEN_RTS, "dod_cregen_rts", function(offset, data, mask)
    if data ~= 0x39 then return data end
    dump_pop("CREGEN-exit")
    return data
end)

taps[#taps + 1] = mem:install_read_tap(NEWLVL_RTS, NEWLVL_RTS, "dod_newlvl", function(offset, data, mask)
    if data ~= 0x39 then return data end
    dump_pop("NEWLVL-exit")
    return data
end)

taps[#taps + 1] = mem:install_read_tap(SNOISE, SNOISE, "dod_snoise", function(offset, data, mask)
    if data ~= 0xDC then return data end
    snoise_seed = hex_at(SEED, 3)
    snoise_rnd = hex_at(SNDRND, 2)
    snoise_isr = isr
    if thud_isr then thud_snoise = thud_snoise + 1 end
    return data
end)

taps[#taps + 1] = mem:install_read_tap(SNOISE_RTS, SNOISE_RTS, "dod_snoise_rts", function(offset, data, mask)
    if data ~= 0x39 or not snoise_seed then return data end
    local seed_now = hex_at(SEED, 3)
    soundlog:write(string.format("SNOISE\t%d\t%04X\t%s\t%s\tseed_after=%s\n",
        snoise_isr, SNOISE, snoise_seed, snoise_rnd, seed_now))
    snoise_seed = nil
    return data
end)

taps[#taps + 1] = mem:install_read_tap(THUD_SWI, THUD_SWI, "dod_thud", function(offset, data, mask)
    if data ~= 0x3F or thud_isr then return data end
    thud_isr = isr
    thud_seed = hex_at(SEED, 3)
    thud_rnd = hex_at(SNDRND, 2)
    thud_snoise = 0
    thud_dac = 0
    return data
end)

taps[#taps + 1] = mem:install_read_tap(THUD_RESUME, THUD_RESUME, "dod_thud_done", function(offset, data, mask)
    if not thud_isr then return data end
    soundlog:write(string.format(
        "THUD\t%d\t%04X\t%s\t%s\tend_isr=%d blocked_interrupts=%d snoise=%d dac=%d seed_after=%s\n",
        thud_isr, THUD_SWI, thud_seed, thud_rnd, isr, isr - thud_isr,
        thud_snoise, thud_dac, hex_at(SEED, 3)))
    soundlog:flush()
    thud_isr = nil
    return data
end)

taps[#taps + 1] = mem:install_write_tap(0xFF20, 0xFF20, "dod_dac", function(offset, data, mask)
    dac_total = dac_total + 1
    if thud_isr then thud_dac = thud_dac + 1 end
    if dac_total <= 8 or (thud_isr and thud_dac <= 4) then
        local pc = cpu.state["PC"].value
        soundlog:write(string.format("DAC\t%d\t%04X\t%s\t%s\tvalue=%02X\n",
            isr, pc, hex_at(SEED, 3), hex_at(SNDRND, 2), data & 0xFF))
    end
end)

-- Stop a long run shortly after the requested task, still honouring DOD_JIFFIES.
local prev_on_clock_tail = on_clock
-- on_clock is the clock handler above; extend the jiffy-limit check in place.
local jiffy_limit_saved = jiffy_limit
-- Re-bind nothing: the existing on_clock checks jiffy_limit. Fold DOD_STOP into that
-- by shrinking jiffy_limit once stop_at is known. See the wrapper installed below.

local clock_tap_index = 1 -- the JIFFY write tap is taps[1], already calling on_clock
-- Wrap by replacing on_clock's limit. Patch the check via a second read of isr
-- inside the SCHED tap's note_task, and exit from a tiny poll at the JIFFY tap.
-- The JIFFY tap calls on_clock, which already exits on jiffy_limit. Set the limit
-- dynamically:
local base_on_clock = on_clock
on_clock = function()
    base_on_clock()
    if stop_at and isr >= stop_at then
        raw:flush()
        trace:flush()
        tasklog:flush()
        spinlog:flush()
        soundlog:flush()
        poplog:flush()
        manager.machine:exit()
    end
end
-- The write tap closed over the original on_clock. Reinstall is not possible
-- without replacing the tap. Call the wrapper from the original by assignment
-- before the tap... too late. The tap captured on_clock as an upvalue.
-- Lua upvalues see the local, and `on_clock = function` after the tap does NOT
-- change the upvalue the tap already captured if it was a local function.
-- The tap was installed with `function` calling `on_clock` by name, so it looks
-- up the local. Reassigning the local updates the upvalue. The wrapper above
-- calls base_on_clock which is the original, then checks stop_at. That recurses
-- if base_on_clock is the wrapper. base_on_clock was captured before reassignment,
-- so it is the original. Good.

_G.dod_capture_taps = taps
-- NEWLVX is resolved so a missing symbol fails at startup rather than mid-run.
if not NEWLVX then die("NEWLVX missing") end
