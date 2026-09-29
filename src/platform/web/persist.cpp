#include "persist.hpp"

#include <emscripten.h>

namespace {

EM_ASYNC_JS(void, idbfs_mount, (const char* dir), {
    const path = UTF8ToString(dir);
    FS.mkdirTree(path);
    FS.mount(IDBFS, {}, path);
    await new Promise((resolve) => FS.syncfs(true, (err) => {
        if (err) console.error("save directory: load from IndexedDB failed", err);
        resolve();
    }));
});

EM_JS(void, idbfs_flush, (), {
    FS.syncfs(false, (err) => {
        if (err) console.error("save directory: write to IndexedDB failed", err);
    });
});

}  // namespace

namespace dag::web {

void mount_persistent(const std::string& dir) {
    std::string path = dir;  // SDL_GetPrefPath ends in '/'; FS.mount wants none
    while (path.size() > 1 && path.back() == '/') path.pop_back();
    idbfs_mount(path.c_str());
}

void flush_persistent() { idbfs_flush(); }

}  // namespace dag::web
