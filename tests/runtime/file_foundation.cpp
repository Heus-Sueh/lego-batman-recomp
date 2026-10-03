#include <pcrecomp/memory.h>
#include <pcrecomp/imports.h>
#include <pcrecomp/win32_fs.h>
#include <pcmodern/filesystem.h>
#include <win32compat/fs.h>
#include <fstream>
#include <array>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <filesystem>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "check failed at %d: %s\n", __LINE__, #x); std::exit(1); } } while(false)

int main() {
    const auto temp_dir = std::filesystem::temp_directory_path() /
        std::filesystem::path("pcrecomp-file-test-" + std::to_string(std::rand()));
    std::filesystem::create_directories(temp_dir / "Data");

    const std::string fixture_content = "PCRECOMP_FILE_FIXTURE_12345";
    {
        std::ofstream out(temp_dir / "Data" / "Fixture.txt", std::ios::binary);
        out << fixture_content;
    }

    auto& vfs = pcmodern::VirtualFileSystem::instance();
    vfs.set_root(temp_dir);

    PcRuntimeConfig config{};
    PcRuntime* r = pc_runtime_create(&config);
    CHECK(r != nullptr);

    std::array<uint8_t, 4096> memory{};
    CHECK(pc_bind_memory(r, memory.data(), memory.size()) == PC_OK);
    CHECK(pc_register_fs_imports(r) == PC_OK);

    // Write "data\\fixture.TXT" into guest memory at VA 100
    const char* filename = "data\\fixture.TXT";
    for (size_t i = 0; i <= std::strlen(filename); ++i) {
        CHECK(pc_write8(r, 100 + static_cast<uint32_t>(i), static_cast<uint8_t>(filename[i])) == PC_OK);
    }

    // 1. CreateFileA: open existing mixed-case file
    uint32_t esp = 1000;
    CHECK(pc_write32(r, esp + 0, 0xDEADBEEF) == PC_OK); // return address
    CHECK(pc_write32(r, esp + 4, 100) == PC_OK);        // lpFileName
    CHECK(pc_write32(r, esp + 8, win32compat::WIN32_GENERIC_READ) == PC_OK);
    CHECK(pc_write32(r, esp + 12, 0) == PC_OK);
    CHECK(pc_write32(r, esp + 16, 0) == PC_OK);
    CHECK(pc_write32(r, esp + 20, win32compat::WIN32_OPEN_EXISTING) == PC_OK);
    CHECK(pc_write32(r, esp + 24, 0) == PC_OK);
    CHECK(pc_write32(r, esp + 28, 0) == PC_OK);

    PcContext c{};
    c.esp = esp;
    CHECK(pc_call_import(r, PC_FS_CREATE_FILE_A, &c, 0x200) == PC_OK);
    CHECK(c.eax >= 0x1000 && c.eax != win32compat::WIN32_INVALID_HANDLE_VALUE);
    CHECK(c.esp == esp + 4 + 28);
    CHECK(win32compat::get_last_error() == win32compat::WIN32_ERROR_SUCCESS);
    uint32_t handle = c.eax;

    // 2. GetFileSize
    esp = 1000;
    CHECK(pc_write32(r, esp + 4, handle) == PC_OK);
    CHECK(pc_write32(r, esp + 8, 350) == PC_OK); // lpFileSizeHigh
    c.esp = esp;
    CHECK(pc_call_import(r, PC_FS_GET_FILE_SIZE, &c, 0x200) == PC_OK);
    CHECK(c.eax == fixture_content.size());
    CHECK(c.esp == esp + 4 + 8);
    uint32_t size_high = 0xFF;
    CHECK(pc_read32(r, 350, &size_high) == PC_OK && size_high == 0);

    // 3. ReadFile: read first 10 bytes into VA 200
    esp = 1000;
    CHECK(pc_write32(r, esp + 4, handle) == PC_OK);
    CHECK(pc_write32(r, esp + 8, 200) == PC_OK);    // lpBuffer
    CHECK(pc_write32(r, esp + 12, 10) == PC_OK);    // nNumberOfBytesToRead
    CHECK(pc_write32(r, esp + 16, 300) == PC_OK);   // lpNumberOfBytesRead
    CHECK(pc_write32(r, esp + 20, 0) == PC_OK);     // lpOverlapped
    c.esp = esp;
    CHECK(pc_call_import(r, PC_FS_READ_FILE, &c, 0x200) == PC_OK);
    CHECK(c.eax == 1);
    CHECK(c.esp == esp + 4 + 20);
    uint32_t bytes_read = 0;
    CHECK(pc_read32(r, 300, &bytes_read) == PC_OK && bytes_read == 10);
    char buf[11]{};
    for (uint32_t i = 0; i < 10; ++i) {
        uint8_t byte = 0;
        CHECK(pc_read8(r, 200 + i, &byte) == PC_OK);
        buf[i] = static_cast<char>(byte);
    }
    CHECK(std::strcmp(buf, "PCRECOMP_F") == 0);

    // 4. SetFilePointer: seek back to offset 0
    esp = 1000;
    CHECK(pc_write32(r, esp + 4, handle) == PC_OK);
    CHECK(pc_write32(r, esp + 8, 0) == PC_OK); // distance
    CHECK(pc_write32(r, esp + 12, 0) == PC_OK); // high
    CHECK(pc_write32(r, esp + 16, 0) == PC_OK); // SEEK_SET
    c.esp = esp;
    CHECK(pc_call_import(r, PC_FS_SET_FILE_POINTER, &c, 0x200) == PC_OK);
    CHECK(c.eax == 0);
    CHECK(c.esp == esp + 4 + 16);

    // Read all 27 bytes
    esp = 1000;
    CHECK(pc_write32(r, esp + 4, handle) == PC_OK);
    CHECK(pc_write32(r, esp + 8, 200) == PC_OK);
    CHECK(pc_write32(r, esp + 12, 50) == PC_OK);
    CHECK(pc_write32(r, esp + 16, 300) == PC_OK);
    CHECK(pc_write32(r, esp + 20, 0) == PC_OK);
    c.esp = esp;
    CHECK(pc_call_import(r, PC_FS_READ_FILE, &c, 0x200) == PC_OK);
    CHECK(c.eax == 1);
    CHECK(pc_read32(r, 300, &bytes_read) == PC_OK && bytes_read == fixture_content.size());

    // 5. EOF test: reading past end of file returns success with 0 bytes read
    c.esp = esp;
    CHECK(pc_call_import(r, PC_FS_READ_FILE, &c, 0x200) == PC_OK);
    CHECK(c.eax == 1);
    CHECK(pc_read32(r, 300, &bytes_read) == PC_OK && bytes_read == 0);

    // 6. CloseHandle
    esp = 1000;
    CHECK(pc_write32(r, esp + 4, handle) == PC_OK);
    c.esp = esp;
    CHECK(pc_call_import(r, PC_FS_CLOSE_HANDLE, &c, 0x200) == PC_OK);
    CHECK(c.eax == 1);
    CHECK(c.esp == esp + 4 + 4);

    // 7. Negative control: Double-close returns 0 with ERROR_INVALID_HANDLE
    c.esp = esp;
    CHECK(pc_call_import(r, PC_FS_CLOSE_HANDLE, &c, 0x200) == PC_OK);
    CHECK(c.eax == 0);
    CHECK(win32compat::get_last_error() == win32compat::WIN32_ERROR_INVALID_HANDLE);

    // 8. Negative control: Read with closed handle returns 0 with ERROR_INVALID_HANDLE
    esp = 1000;
    CHECK(pc_write32(r, esp + 4, handle) == PC_OK);
    CHECK(pc_write32(r, esp + 8, 200) == PC_OK);
    CHECK(pc_write32(r, esp + 12, 10) == PC_OK);
    CHECK(pc_write32(r, esp + 16, 300) == PC_OK);
    CHECK(pc_write32(r, esp + 20, 0) == PC_OK);
    c.esp = esp;
    CHECK(pc_call_import(r, PC_FS_READ_FILE, &c, 0x200) == PC_OK);
    CHECK(c.eax == 0);
    CHECK(win32compat::get_last_error() == win32compat::WIN32_ERROR_INVALID_HANDLE);

    // 9. Negative control: Open non-existent file returns INVALID_HANDLE_VALUE with ERROR_FILE_NOT_FOUND
    const char* missing = "missing.bin";
    for (size_t i = 0; i <= std::strlen(missing); ++i) {
        CHECK(pc_write8(r, 500 + static_cast<uint32_t>(i), static_cast<uint8_t>(missing[i])) == PC_OK);
    }
    esp = 1000;
    CHECK(pc_write32(r, esp + 4, 500) == PC_OK);
    CHECK(pc_write32(r, esp + 8, win32compat::WIN32_GENERIC_READ) == PC_OK);
    CHECK(pc_write32(r, esp + 12, 0) == PC_OK);
    CHECK(pc_write32(r, esp + 16, 0) == PC_OK);
    CHECK(pc_write32(r, esp + 20, win32compat::WIN32_OPEN_EXISTING) == PC_OK);
    CHECK(pc_write32(r, esp + 24, 0) == PC_OK);
    CHECK(pc_write32(r, esp + 28, 0) == PC_OK);
    c.esp = esp;
    CHECK(pc_call_import(r, PC_FS_CREATE_FILE_A, &c, 0x200) == PC_OK);
    CHECK(c.eax == win32compat::WIN32_INVALID_HANDLE_VALUE);
    CHECK(win32compat::get_last_error() == win32compat::WIN32_ERROR_FILE_NOT_FOUND);

    // 10. Negative control: Out-of-bounds buffer detection
    // Open fixture again
    CHECK(pc_write32(r, esp + 4, 100) == PC_OK);
    c.esp = esp;
    CHECK(pc_call_import(r, PC_FS_CREATE_FILE_A, &c, 0x200) == PC_OK);
    uint32_t handle2 = c.eax;
    CHECK(handle2 != win32compat::WIN32_INVALID_HANDLE_VALUE);

    // Attempt to read 100 bytes into VA 4050 (arena size is 4096)
    esp = 1000;
    CHECK(pc_write32(r, esp + 4, handle2) == PC_OK);
    CHECK(pc_write32(r, esp + 8, 4050) == PC_OK);
    CHECK(pc_write32(r, esp + 12, 100) == PC_OK);
    CHECK(pc_write32(r, esp + 16, 300) == PC_OK);
    CHECK(pc_write32(r, esp + 20, 0) == PC_OK);
    c.esp = esp;
    CHECK(pc_call_import(r, PC_FS_READ_FILE, &c, 0x200) == PC_MEMORY_BOUNDS);

    // Clean up handle2
    esp = 1000;
    CHECK(pc_write32(r, esp + 4, handle2) == PC_OK);
    c.esp = esp;
    CHECK(pc_call_import(r, PC_FS_CLOSE_HANDLE, &c, 0x200) == PC_OK);

    // 11. Pseudo-handles (mutex/event) close returns 1
    CHECK(win32compat::close_handle(0xCAFE0001u) == 1);
    CHECK(win32compat::close_handle(0xCAFE0002u) == 1);

    pc_runtime_destroy(r);
    std::filesystem::remove_all(temp_dir);

    std::puts("Runtime file foundation: CreateFileA, ReadFile, SetFilePointer, GetFileSize, CloseHandle passed with negative controls");
    return 0;
}
