#include "qsr_operations.h"
#include <SDL.h>
#include <SDL_filesystem.h>
#include <SDL_stdinc.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

namespace qsr::operations {

bool isSnapshotFile(const std::string& path) {
    // Check magic bytes first by reading the first 16 bytes.
    FILE* fp = fopen(path.c_str(), "rb");
    if (fp) {
        unsigned char magic[16] = {};
        size_t n = fread(magic, 1, sizeof(magic), fp);
        fclose(fp);
        if (n >= 4) {
            // UAE savestate: starts with "ASF " (0x41534620)
            if (magic[0] == 'A' && magic[1] == 'S' && magic[2] == 'F' && magic[3] == ' ')
                return true;
            // vAmiga snapshot: starts with "VASNAP"
            if (n >= 7 && magic[0] == 'V' && magic[1] == 'A' && magic[2] == 'S' &&
                magic[3] == 'N' && magic[4] == 'A' && magic[5] == 'P')
                return true;
        }
    }
    // Fall back to extension matching
    auto endsWith = [](const std::string& str, const char* suffix) {
        size_t len = strlen(suffix);
        return str.size() >= len && str.compare(str.size() - len, len, suffix) == 0;
    };
    return endsWith(path, ".uss") || endsWith(path, ".vasnap");
}

std::string getQuickSavePath() {
    namespace fs = std::filesystem;
    fs::path dir;

    char* base = SDL_GetBasePath();
    if (base) {
        dir = fs::path(base) / "data" / "snapshots";
        SDL_free(base);
    } else {
        dir = fs::path("data") / "snapshots";
    }

    std::error_code ec;
    fs::create_directories(dir, ec);  // ignore errors — save will report if it fails

    return (dir / "quicksave.uss").string();
}

};  // namespace qsr::operations
