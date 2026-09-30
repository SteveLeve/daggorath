// The parts of storage.hpp both implementations share.
#include "daggorath/storage.hpp"

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

}  // namespace dag::platform
