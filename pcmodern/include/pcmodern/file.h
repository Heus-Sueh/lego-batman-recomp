#pragma once
#include <filesystem>
#include <cstddef>
#include <cstdint>

namespace pcmodern {

struct HostFile;

HostFile* file_open(const std::filesystem::path& path, const char* mode);
size_t file_read(HostFile* file, void* buffer, size_t size);
size_t file_write(HostFile* file, const void* buffer, size_t size);
int64_t file_seek(HostFile* file, int64_t offset, int origin);
int64_t file_tell(HostFile* file);
void file_close(HostFile* file);

} // namespace pcmodern
