// Browser implementation of storage.hpp (ADR-0011): localStorage, one key per
// save ("dod.save.<NAME>"), one per menu slot ("dod.slot.<1-5>"), plus
// "dod.prefs". localStorage is synchronous, so
// setItem either updates the storage area or throws (quota exhausted, storage
// blocked) before it returns; store_save's result is therefore known at once,
// not a promise of a later flush. When the browser writes the area to disk is
// its own business. A save is about 8 KB of text and a slot
// about 8.5 KB plus 8 KB per cassette entry; together they can reach the
// origin's quota over long play, which then fails a store like any other.
#include "daggorath/storage.hpp"

#include <emscripten.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

EM_JS_DEPS(dod_storage, "$stringToNewUTF8");

// Every key under `prefix`, minus the prefix, newline-separated. Null when
// storage cannot be read at all (blocked, or disabled by the browser).
EM_JS(char*, dod_ls_list, (const char* prefix), {
    try {
        const p = UTF8ToString(prefix);
        const names = [];
        for (let i = 0; i < localStorage.length; ++i) {
            const key = localStorage.key(i);
            if (key && key.startsWith(p)) names.push(key.slice(p.length));
        }
        return stringToNewUTF8(names.join("\n"));
    } catch (e) {
        console.error("storage: list failed", e);
        return 0;
    }
});

// The value, or null when absent; `*failed` is set when the read threw.
EM_JS(char*, dod_ls_get, (const char* key, int* failed), {
    try {
        const value = localStorage.getItem(UTF8ToString(key));
        return value === null ? 0 : stringToNewUTF8(value);
    } catch (e) {
        console.error("storage: read failed", e);
        HEAP32[failed >> 2] = 1;
        return 0;
    }
});

EM_JS(int, dod_ls_set, (const char* key, const char* value), {
    try {
        localStorage.setItem(UTF8ToString(key), UTF8ToString(value));
        return 1;
    } catch (e) {
        console.error("storage: write failed", e);
        return 0;
    }
});

EM_JS(void, dod_notice, (const char* text), {
    const message = UTF8ToString(text);
    if (typeof window.dodNotice === "function") window.dodNotice(message);
    else console.warn(message);
});

constexpr const char* kSavePrefix = "dod.save.";
constexpr const char* kSlotPrefix = "dod.slot.";
constexpr const char* kPrefsKey = "dod.prefs";

// Takes ownership of a string the JS side allocated with stringToNewUTF8.
std::optional<std::string> adopt(char* text) {
    if (text == nullptr) return {};
    std::string out(text);
    std::free(text);
    return out;
}

}  // namespace

namespace dag::platform {

Storage::Storage(std::string dir) : dir_(std::move(dir)) {}

SaveLoad Storage::load_saves() const {
    SaveLoad result;
    const auto listed = adopt(dod_ls_list(kSavePrefix));
    if (!listed) {
        result.error = "browser storage is blocked or unavailable";
        return result;
    }
    std::string_view names = *listed;
    while (!names.empty()) {
        const auto end = names.find('\n');
        const std::string name(names.substr(0, end));
        names = end == std::string_view::npos ? std::string_view{} : names.substr(end + 1);
        if (!tape_name_ok(name)) continue;
        int failed = 0;
        auto image = adopt(dod_ls_get((kSavePrefix + name).c_str(), &failed));
        if (failed) result.error = "could not read save " + name;
        if (!image || image->rfind("DAGRAM 1", 0) != 0) continue;
        result.saves.push_back({name, std::move(*image)});
    }
    std::sort(result.saves.begin(), result.saves.end(),
              [](const StoredSave& a, const StoredSave& b) { return a.name < b.name; });
    return result;
}

bool Storage::store_save(const std::string& name, const std::string& image) {
    if (!tape_name_ok(name)) return false;
    return dod_ls_set((kSavePrefix + name).c_str(), image.c_str()) != 0;
}

SlotLoad Storage::load_slots() const {
    SlotLoad result;
    for (std::size_t n = 1; n <= kStoredSlots; ++n) {
        int failed = 0;
        const auto text = adopt(dod_ls_get((kSlotPrefix + std::to_string(n)).c_str(), &failed));
        if (failed) {
            result.error = "browser storage is blocked or unavailable";
            return result;
        }
        if (!text) continue;
        if (auto slot = decode_slot(n, *text)) result.slots.push_back(std::move(*slot));
        else result.damaged.push_back(n);
    }
    return result;
}

bool Storage::store_slot(std::size_t number, const std::string& name, const std::string& snapshot) {
    if (number < 1 || number > kStoredSlots || name.find('\n') != std::string::npos) return false;
    return dod_ls_set((kSlotPrefix + std::to_string(number)).c_str(),
                      encode_slot(name, snapshot).c_str()) != 0;
}

std::optional<std::string> Storage::load_prefs() const {
    int failed = 0;
    return adopt(dod_ls_get(kPrefsKey, &failed));
}

bool Storage::store_prefs(const std::string& text) {
    return dod_ls_set(kPrefsKey, text.c_str()) != 0;
}

void report_storage_problem(const std::string& message) {
    std::cerr << "storage: " << message << "\n";
    dod_notice(message.c_str());
}

}  // namespace dag::platform
