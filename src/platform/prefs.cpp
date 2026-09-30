#include "daggorath/prefs.hpp"

namespace dag::platform {

std::string serialize_prefs(const Prefs& prefs) {
    std::string out = "DODPREFS 1\n";
    out += prefs.crisp ? "video=crisp\n" : "video=pixel\n";
    if (prefs.layout)
        out += *prefs.layout == dag::input::OverlayLayout::PhoneLandscape ? "layout=phone\n"
                                                                          : "layout=tablet\n";
    return out;
}

Prefs parse_prefs(std::string_view text) {
    Prefs prefs;
    while (!text.empty()) {
        const auto end = text.find('\n');
        std::string_view line = text.substr(0, end);
        text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (line == "video=crisp") prefs.crisp = true;
        else if (line == "video=pixel") prefs.crisp = false;
        else if (line == "layout=phone") prefs.layout = dag::input::OverlayLayout::PhoneLandscape;
        else if (line == "layout=tablet") prefs.layout = dag::input::OverlayLayout::Tablet4x3;
    }
    return prefs;
}

}  // namespace dag::platform
