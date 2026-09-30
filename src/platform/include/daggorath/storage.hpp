// Where the platform keeps ZSAVE images and preferences (ADR-0011). The core
// never sees this: it only receives DAGRAM 1 images on its cassette (D-20).
// Two implementations of the same class, chosen at build time:
//   file_storage.cpp     desktop: one file per save plus a prefs file in the
//                        SDL pref directory, each written atomically
//   web/web_storage.cpp  browser: synchronous localStorage, so a failed write
//                        is known before ZSAVE is reported as stored
#pragma once
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

class Storage {
public:
    // `dir` is the desktop save directory; the web build ignores it.
    explicit Storage(std::string dir);

    SaveLoad load_saves() const;
    // True once the storage API accepted the image; false means it did not,
    // known before this returns. (When bytes reach the disk is up to the OS
    // or browser; neither backend forces it.)
    bool store_save(const std::string& name, const std::string& image);

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
