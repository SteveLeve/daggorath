// Where the platform keeps ZSAVE images, the system menu's save slots and
// preferences (ADR-0011). The core never sees this: it only receives DAGRAM 1
// images on its cassette (D-20), and the shell receives slot snapshots.
// Two implementations of the same class, chosen at build time:
//   file_storage.cpp     desktop: one file per save and per slot plus a prefs
//                        file in the SDL pref directory, each written through
//                        a temporary file and a rename
//   web/web_storage.cpp  browser: synchronous localStorage, so a failed write
//                        is known before ZSAVE is reported as stored
#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace dag::platform {

// ZSAVE names the platform will store: 1-8 of A-Z, 0-9. Anything else is
// refused on write and skipped on read.
bool tape_name_ok(const std::string& name);

struct StoredSave {
    std::string name;
    std::string image;  // a DAGRAM 1 image
};

struct SaveLoad {
    std::vector<StoredSave> saves;  // valid DAGRAM 1 images, sorted by name
    std::string error;              // empty: the store was read (or is empty)
};

// The system menu's save slots (ADR-0009 §6), numbered 1-5 as the menu shows
// them. Each is stored as a small envelope around the shell's DAGSNAP 1
// snapshot, carrying the slot's display name:
//   "DODSLOT 1\n<name>\n<snapshot>"
constexpr std::size_t kStoredSlots = 5;

struct StoredSlot {
    std::size_t number = 0;  // 1..kStoredSlots
    std::string name;        // the shell's auto-name, e.g. "L1 03:12"
    std::string snapshot;    // a DAGSNAP 1 snapshot
};

struct SlotLoad {
    std::vector<StoredSlot> slots;     // well-formed slots, by number
    std::vector<std::size_t> damaged;  // slots present but not well-formed
    std::string error;                 // empty: the store was read (or is empty)
};

// `name` must not contain a newline (store_slot refuses one).
std::string encode_slot(const std::string& name, const std::string& snapshot);
// Empty when `text` is not a DODSLOT 1 envelope around a DAGSNAP 1 snapshot.
std::optional<StoredSlot> decode_slot(std::size_t number, const std::string& text);

class Storage {
public:
    // `dir` is the desktop save directory; the web build ignores it.
    explicit Storage(std::string dir);

    SaveLoad load_saves() const;
    // True once the storage API accepted the image; false means it did not,
    // known before this returns. (When bytes reach the disk is up to the OS
    // or browser; neither backend forces it.)
    bool store_save(const std::string& name, const std::string& image);

    SlotLoad load_slots() const;
    // Same contract as store_save. `number` is 1..kStoredSlots.
    bool store_slot(std::size_t number, const std::string& name, const std::string& snapshot);

    std::optional<std::string> load_prefs() const;
    bool store_prefs(const std::string& text);

private:
    std::string dir_;
};

// Tells the player a storage operation failed: stderr on the desktop, a
// notice over the top of the page in the browser. Never touches the game screen,
// which belongs to the core's text page.
void report_storage_problem(const std::string& message);

}  // namespace dag::platform
