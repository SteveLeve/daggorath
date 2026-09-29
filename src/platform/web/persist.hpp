// Browser persistence for the Emscripten build (ADR-0011). Emscripten's
// default filesystem lives in memory; this backs the save directory with
// IndexedDB so ZSAVE files outlive a page reload. Nothing here is compiled
// outside __EMSCRIPTEN__.
#pragma once

#include <string>

namespace dag::web {

// Mounts IDBFS at `dir` and blocks (via ASYNCIFY) until IndexedDB's copy has
// been loaded into it, so mount_saves() sees earlier sessions' files.
void mount_persistent(const std::string& dir);

// Starts writing the in-memory directory back to IndexedDB. Asynchronous;
// a write lost to an immediate tab close is lost, as a desktop crash would be.
void flush_persistent();

}  // namespace dag::web
