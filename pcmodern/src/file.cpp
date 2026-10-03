#include <pcmodern/file.h>
#include <cstdio>

namespace pcmodern {

struct HostFile {
    FILE* fp{nullptr};
};

HostFile* file_open(const std::filesystem::path& path, const char* mode) {
    FILE* fp = std::fopen(path.string().c_str(), mode);
    if (!fp) return nullptr;
    return new HostFile{fp};
}

size_t file_read(HostFile* file, void* buffer, size_t size) {
    if (!file || !file->fp || !buffer || size == 0) return 0;
    return std::fread(buffer, 1, size, file->fp);
}

size_t file_write(HostFile* file, const void* buffer, size_t size) {
    if (!file || !file->fp || !buffer || size == 0) return 0;
    return std::fwrite(buffer, 1, size, file->fp);
}

int64_t file_seek(HostFile* file, int64_t offset, int origin) {
    if (!file || !file->fp) return -1;
    if (std::fseek(file->fp, static_cast<long>(offset), origin) != 0) return -1;
    return std::ftell(file->fp);
}

int64_t file_tell(HostFile* file) {
    if (!file || !file->fp) return -1;
    return std::ftell(file->fp);
}

void file_close(HostFile* file) {
    if (!file) return;
    if (file->fp) {
        std::fclose(file->fp);
        file->fp = nullptr;
    }
    delete file;
}

} // namespace pcmodern
