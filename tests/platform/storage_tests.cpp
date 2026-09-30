// Platform storage (storage.hpp, prefs.hpp): the preferences text format, the
// menu-slot envelope and the desktop file backend. The browser backend (web/web_storage.cpp) is
// exercised in headless Chrome by tools/web/storage-test.mjs.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "daggorath/game.hpp"
#include "daggorath/prefs.hpp"
#include "daggorath/storage.hpp"

namespace {

namespace fs = std::filesystem;
using dag::input::OverlayLayout;

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const std::string& what, const std::string& detail = "") {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::cout << "FAIL: " << what;
        if (!detail.empty()) std::cout << "  [" << detail << "]";
        std::cout << "\n";
    }
}

// A fresh, empty directory under the system temp dir, removed on scope exit.
struct TempDir {
    fs::path path;
    TempDir() {
        path = fs::temp_directory_path() /
               ("dod-storage-test-" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        fs::remove_all(path);
        fs::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

void write_raw(const fs::path& path, const std::string& bytes) {
    std::ofstream out(path, std::ios::binary);
    out << bytes;
}

// A real image from the core, so the storage round trip carries what ZSAVE writes.
std::string real_image() {
    dag::Game game;
    for (const char ch : std::string("ZSAVE ALPHA")) game.press(static_cast<std::uint8_t>(ch));
    game.press(0x0D);
    game.advance_jiffies(60);
    const std::string* image = game.cassette_image("ALPHA");
    return image ? *image : std::string();
}

void test_prefs_round_trip() {
    for (const bool crisp : {false, true})
        for (const auto layout : {OverlayLayout::PhoneLandscape, OverlayLayout::Tablet4x3}) {
            const dag::platform::Prefs in{crisp, layout};
            const auto out = dag::platform::parse_prefs(dag::platform::serialize_prefs(in));
            check(out.crisp == crisp && out.layout == layout, "prefs round trip",
                  dag::platform::serialize_prefs(in));
        }
    const auto unset = dag::platform::parse_prefs(dag::platform::serialize_prefs({}));
    check(!unset.crisp && !unset.layout, "unset layout stays unset");
}

void test_prefs_tolerant_parse() {
    const auto empty = dag::platform::parse_prefs("");
    check(!empty.crisp && !empty.layout, "empty text gives defaults");
    const auto odd = dag::platform::parse_prefs("junk\r\nvideo=crisp\r\nlayout=sideways\nfuture=1\n");
    check(odd.crisp, "CRLF line still read");
    check(!odd.layout, "bad layout value ignored");
    const auto last = dag::platform::parse_prefs("layout=phone\nlayout=tablet");
    check(last.layout == OverlayLayout::Tablet4x3, "last value wins, no trailing newline needed");
}

void test_tape_names() {
    check(dag::platform::tape_name_ok("A"), "one letter");
    check(dag::platform::tape_name_ok("GAME1234"), "eight characters");
    check(!dag::platform::tape_name_ok(""), "empty refused");
    check(!dag::platform::tape_name_ok("TOOLONG12"), "nine characters refused");
    check(!dag::platform::tape_name_ok("lower"), "lower case refused");
    check(!dag::platform::tape_name_ok("../X"), "path characters refused");
}

void test_file_saves_round_trip() {
    TempDir dir;
    dag::platform::Storage storage(dir.path.string());
    const std::string image = real_image();
    check(image.rfind("DAGRAM 1", 0) == 0, "core produced a DAGRAM 1 image");
    check(storage.store_save("BETA", image), "store BETA");
    check(storage.store_save("ALPHA", image), "store ALPHA");
    check(!storage.store_save("bad name", image), "invalid name refused");
    check(!fs::exists(dir.path / "BETA.dagram.tmp"), "no temporary file left behind");
    // Files the loader must skip: wrong header, wrong name, wrong extension.
    write_raw(dir.path / "JUNK.dagram", "not a save");
    write_raw(dir.path / "lower.dagram", image);
    write_raw(dir.path / "NOTES.txt", image);
    const auto loaded = storage.load_saves();
    check(loaded.error.empty(), "load reports no error", loaded.error);
    check(loaded.saves.size() == 2, "only the two valid saves load",
          std::to_string(loaded.saves.size()));
    if (loaded.saves.size() == 2) {
        check(loaded.saves[0].name == "ALPHA" && loaded.saves[1].name == "BETA", "sorted by name");
        check(loaded.saves[0].image == image, "image survives byte for byte");
    }
    // The loaded image mounts on a fresh game's cassette, as mount_saves does.
    dag::Game fresh;
    check(!loaded.saves.empty() && fresh.insert_cassette_image("ALPHA", loaded.saves[0].image),
          "stored image is accepted by the core");
}

void test_file_missing_directory() {
    TempDir dir;
    dag::platform::Storage storage((dir.path / "not-yet").string());
    const auto loaded = storage.load_saves();
    check(loaded.error.empty() && loaded.saves.empty(), "no directory yet: no saves, no error");
    check(!storage.load_prefs(), "no prefs yet");
    check(storage.store_prefs("DODPREFS 1\n"), "first write creates the directory");
}

void test_file_write_failure_reported() {
    TempDir dir;
    // The save directory's path is taken by a plain file, so nothing can be
    // written beneath it: the failure must come back as false, not silently.
    const fs::path blocked = dir.path / "blocked";
    write_raw(blocked, "a file, not a directory");
    dag::platform::Storage storage(blocked.string());
    check(!storage.store_save("GAMMA", real_image()), "unwritable save reported");
    check(!storage.store_prefs("DODPREFS 1\n"), "unwritable prefs reported");
}

void test_file_prefs_round_trip() {
    TempDir dir;
    dag::platform::Storage storage(dir.path.string());
    const dag::platform::Prefs in{true, OverlayLayout::PhoneLandscape};
    check(storage.store_prefs(dag::platform::serialize_prefs(in)), "store prefs");
    const auto text = storage.load_prefs();
    check(text.has_value(), "prefs read back");
    if (text) {
        const auto out = dag::platform::parse_prefs(*text);
        check(out.crisp && out.layout == OverlayLayout::PhoneLandscape, "prefs survive a restart");
    }
}

void test_slot_envelope() {
    const std::string snap = dag::Game().snapshot();
    const auto slot = dag::platform::decode_slot(2, dag::platform::encode_slot("L1 03:12", snap));
    check(slot && slot->number == 2 && slot->name == "L1 03:12" && slot->snapshot == snap,
          "slot envelope round trip");
    check(!dag::platform::decode_slot(2, "DODSLOT 1\nL1 00:00\nnot a snapshot"),
          "envelope around a non-snapshot refused");
    check(!dag::platform::decode_slot(2, snap), "bare snapshot without envelope refused");
    check(!dag::platform::decode_slot(0, dag::platform::encode_slot("X", snap)), "slot 0 refused");
    check(!dag::platform::decode_slot(6, dag::platform::encode_slot("X", snap)), "slot 6 refused");
}

void test_file_slots_round_trip() {
    TempDir dir;
    dag::platform::Storage storage(dir.path.string());
    check(storage.load_slots().slots.empty(), "no slots yet");
    const std::string snap = dag::Game().snapshot();
    check(storage.store_slot(1, "L1 00:05", snap), "store slot 1");
    check(storage.store_slot(5, "L2 10:00", snap), "store slot 5");
    check(!storage.store_slot(6, "L1 00:00", snap), "slot 6 refused");
    check(!fs::exists(dir.path / "slot1.dagsnap.tmp"), "no temporary slot file left behind");
    write_raw(dir.path / "slot3.dagsnap", "damaged");
    const auto loaded = storage.load_slots();
    check(loaded.error.empty(), "slot load reports no error", loaded.error);
    check(loaded.damaged.size() == 1 && loaded.damaged[0] == 3, "damaged slot 3 listed");
    check(!storage.store_slot(2, "L1\n00:00", snap), "name with a newline refused");
    check(loaded.slots.size() == 2, "only the two valid slots load",
          std::to_string(loaded.slots.size()));
    if (loaded.slots.size() == 2) {
        check(loaded.slots[0].number == 1 && loaded.slots[1].number == 5, "slots in order");
        check(loaded.slots[1].name == "L2 10:00", "slot name survives");
        check(loaded.slots[0].snapshot == snap, "snapshot survives byte for byte");
    }
    // Overwriting replaces the slot.
    check(storage.store_slot(1, "L1 09:09", snap), "overwrite slot 1");
    const auto again = storage.load_slots();
    check(!again.slots.empty() && again.slots[0].name == "L1 09:09", "overwrite read back");

    TempDir blocked_dir;
    const fs::path blocked = blocked_dir.path / "blocked";
    write_raw(blocked, "a file, not a directory");
    dag::platform::Storage unwritable(blocked.string());
    check(!unwritable.store_slot(1, "L1 00:00", snap), "unwritable slot reported");
}

}  // namespace

int main() {
    test_prefs_round_trip();
    test_prefs_tolerant_parse();
    test_tape_names();
    test_file_saves_round_trip();
    test_file_missing_directory();
    test_file_write_failure_reported();
    test_file_prefs_round_trip();
    test_slot_envelope();
    test_file_slots_round_trip();
    std::cout << (g_failures == 0 ? "PASS" : "FAILED") << ": " << g_checks << " checks, "
              << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
