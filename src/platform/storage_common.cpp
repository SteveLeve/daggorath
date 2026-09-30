// The parts of storage.hpp both implementations share.
#include "daggorath/storage.hpp"

#include <string_view>

namespace dag::platform {

bool tape_name_ok(const std::string& name) {
    if (name.empty() || name.size() > 8) return false;
    for (unsigned char c : name) {
        const bool letter = c >= 'A' && c <= 'Z';
        const bool digit = c >= '0' && c <= '9';
        if (!letter && !digit) return false;
    }
    return true;
}

namespace {
constexpr std::string_view kSlotMagic = "DODSLOT 1\n";
constexpr std::string_view kSnapMagic = "DAGSNAP 1";
}  // namespace

std::string encode_slot(const std::string& name, const std::string& snapshot) {
    return std::string(kSlotMagic) + name + "\n" + snapshot;
}

std::optional<StoredSlot> decode_slot(std::size_t number, const std::string& text) {
    if (number < 1 || number > kStoredSlots) return {};
    if (text.compare(0, kSlotMagic.size(), kSlotMagic) != 0) return {};
    const auto name_end = text.find('\n', kSlotMagic.size());
    if (name_end == std::string::npos) return {};
    StoredSlot slot;
    slot.number = number;
    slot.name = text.substr(kSlotMagic.size(), name_end - kSlotMagic.size());
    slot.snapshot = text.substr(name_end + 1);
    if (slot.snapshot.compare(0, kSnapMagic.size(), kSnapMagic) != 0) return {};
    return slot;
}

}  // namespace dag::platform
