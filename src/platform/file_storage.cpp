#include "daggorath/storage.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <system_error>

namespace dag::platform {

namespace {

namespace fs = std::filesystem;

constexpr const char* kSaveExt = ".dagram";
constexpr const char* kPrefsFile = "dod.prefs";

std::optional<std::string> read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

// Write beside the target and rename over it, so a crash mid-write leaves
// the previous file rather than a truncated one.
bool write_atomically(const fs::path& path, const std::string& bytes) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    const fs::path tmp = path.string() + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        out.flush();
        if (!out) {
            fs::remove(tmp, ec);
            return false;
        }
    }
    fs::rename(tmp, path, ec);
    if (ec) fs::remove(tmp, ec);
    return !ec;
}

}  // namespace

Storage::Storage(std::string dir) : dir_(std::move(dir)) {}

SaveLoad Storage::load_saves() const {
    SaveLoad result;
    std::error_code ec;
    if (!fs::exists(dir_, ec)) return result;  // no directory yet: no saves
    for (fs::directory_iterator it(dir_, ec), end; !ec && it != end; it.increment(ec)) {
        const fs::path& path = it->path();
        if (path.extension() != kSaveExt) continue;
        const std::string name = path.stem().string();
        if (!tape_name_ok(name)) continue;
        auto image = read_file(path);
        if (!image) {
            result.error = "could not read " + path.string();
            continue;
        }
        if (image->rfind("DAGRAM 1", 0) != 0) continue;
        result.saves.push_back({name, std::move(*image)});
    }
    if (ec) result.error = "could not list " + dir_ + ": " + ec.message();
    std::sort(result.saves.begin(), result.saves.end(),
              [](const StoredSave& a, const StoredSave& b) { return a.name < b.name; });
    return result;
}

bool Storage::store_save(const std::string& name, const std::string& image) {
    if (!tape_name_ok(name)) return false;
    return write_atomically(fs::path(dir_) / (name + kSaveExt), image);
}

std::optional<std::string> Storage::load_prefs() const {
    return read_file(fs::path(dir_) / kPrefsFile);
}

bool Storage::store_prefs(const std::string& text) {
    return write_atomically(fs::path(dir_) / kPrefsFile, text);
}

void report_storage_problem(const std::string& message) {
    std::cerr << "storage: " << message << "\n";
}

}  // namespace dag::platform
