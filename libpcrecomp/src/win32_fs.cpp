#include <pcrecomp/win32_fs.h>
#include <pcrecomp/memory.h>
#include <win32compat/fs.h>
#include <string>

namespace {

PcStatus create_file_a(PcRuntime* r, PcContext* c, void*) {
    if (c->esp > UINT32_MAX - 28) return PC_MEMORY_BOUNDS;

    uint32_t filename_addr = 0, desired_access = 0, share_mode = 0;
    uint32_t sec_attr = 0, creation_disp = 0, flags = 0, template_file = 0;

    auto s = pc_read32(r, c->esp + 4, &filename_addr);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 8, &desired_access);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 12, &share_mode);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 16, &sec_attr);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 20, &creation_disp);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 24, &flags);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 28, &template_file);
    if (s != PC_OK) return s;

    if (!filename_addr) {
        c->eax = win32compat::create_file_a(nullptr, desired_access, share_mode, nullptr,
                                            creation_disp, flags, nullptr);
        c->esp += 4 + 28;
        return PC_OK;
    }

    std::string filename;
    uint32_t curr = filename_addr;
    while (true) {
        uint8_t byte = 0;
        s = pc_read8(r, curr, &byte);
        if (s != PC_OK) return s;
        if (byte == 0) break;
        filename.push_back(static_cast<char>(byte));
        if (filename.size() > 1024) return PC_MEMORY_BOUNDS;
        ++curr;
    }

    c->eax = win32compat::create_file_a(filename.c_str(), desired_access, share_mode, nullptr,
                                        creation_disp, flags, nullptr);
    c->esp += 4 + 28;
    return PC_OK;
}

PcStatus read_file(PcRuntime* r, PcContext* c, void*) {
    if (c->esp > UINT32_MAX - 20) return PC_MEMORY_BOUNDS;

    uint32_t handle = 0, buffer_addr = 0, bytes_to_read = 0;
    uint32_t bytes_read_addr = 0, overlapped = 0;

    auto s = pc_read32(r, c->esp + 4, &handle);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 8, &buffer_addr);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 12, &bytes_to_read);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 16, &bytes_read_addr);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 20, &overlapped);
    if (s != PC_OK) return s;

    void* buffer = nullptr;
    if (bytes_to_read > 0) {
        if (!buffer_addr) return PC_MEMORY_BOUNDS;
        s = pc_translate(r, buffer_addr, bytes_to_read, &buffer);
        if (s != PC_OK) return s;
    }

    uint32_t bytes_read_local = 0;
    int32_t result = win32compat::read_file(handle, buffer, bytes_to_read, &bytes_read_local, nullptr);

    if (bytes_read_addr != 0) {
        s = pc_write32(r, bytes_read_addr, bytes_read_local);
        if (s != PC_OK) return s;
    }

    c->eax = static_cast<uint32_t>(result);
    c->esp += 4 + 20;
    return PC_OK;
}

PcStatus write_file(PcRuntime* r, PcContext* c, void*) {
    if (c->esp > UINT32_MAX - 20) return PC_MEMORY_BOUNDS;

    uint32_t handle = 0, buffer_addr = 0, bytes_to_write = 0;
    uint32_t bytes_written_addr = 0, overlapped = 0;

    auto s = pc_read32(r, c->esp + 4, &handle);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 8, &buffer_addr);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 12, &bytes_to_write);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 16, &bytes_written_addr);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 20, &overlapped);
    if (s != PC_OK) return s;

    void* buffer = nullptr;
    if (bytes_to_write > 0) {
        if (!buffer_addr) return PC_MEMORY_BOUNDS;
        s = pc_translate(r, buffer_addr, bytes_to_write, &buffer);
        if (s != PC_OK) return s;
    }

    uint32_t bytes_written_local = 0;
    int32_t result = win32compat::write_file(handle, buffer, bytes_to_write, &bytes_written_local, nullptr);

    if (bytes_written_addr != 0) {
        s = pc_write32(r, bytes_written_addr, bytes_written_local);
        if (s != PC_OK) return s;
    }

    c->eax = static_cast<uint32_t>(result);
    c->esp += 4 + 20;
    return PC_OK;
}

PcStatus close_handle(PcRuntime* r, PcContext* c, void*) {
    if (c->esp > UINT32_MAX - 4) return PC_MEMORY_BOUNDS;

    uint32_t handle = 0;
    auto s = pc_read32(r, c->esp + 4, &handle);
    if (s != PC_OK) return s;

    int32_t result = win32compat::close_handle(handle);
    c->eax = static_cast<uint32_t>(result);
    c->esp += 4 + 4;
    return PC_OK;
}

PcStatus set_file_pointer(PcRuntime* r, PcContext* c, void*) {
    if (c->esp > UINT32_MAX - 16) return PC_MEMORY_BOUNDS;

    uint32_t handle = 0, distance_addr = 0, move_method = 0;
    int32_t distance = 0;

    auto s = pc_read32(r, c->esp + 4, &handle);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 8, reinterpret_cast<uint32_t*>(&distance));
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 12, &distance_addr);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 16, &move_method);
    if (s != PC_OK) return s;

    int32_t high = 0;
    uint32_t result = win32compat::set_file_pointer(handle, distance, distance_addr ? &high : nullptr, move_method);

    if (distance_addr != 0) {
        s = pc_write32(r, distance_addr, static_cast<uint32_t>(high));
        if (s != PC_OK) return s;
    }

    c->eax = result;
    c->esp += 4 + 16;
    return PC_OK;
}

PcStatus get_file_size(PcRuntime* r, PcContext* c, void*) {
    if (c->esp > UINT32_MAX - 8) return PC_MEMORY_BOUNDS;

    uint32_t handle = 0, high_addr = 0;
    auto s = pc_read32(r, c->esp + 4, &handle);
    if (s != PC_OK) return s;
    s = pc_read32(r, c->esp + 8, &high_addr);
    if (s != PC_OK) return s;

    uint32_t high = 0;
    uint32_t result = win32compat::get_file_size(handle, high_addr ? &high : nullptr);

    if (high_addr != 0) {
        s = pc_write32(r, high_addr, high);
        if (s != PC_OK) return s;
    }

    c->eax = result;
    c->esp += 4 + 8;
    return PC_OK;
}

} // namespace

extern "C" PcStatus pc_register_fs_imports(PcRuntime* r) {
    if (!r) return PC_INVALID_ARGUMENT;
    struct Mapping { uint32_t id; PcGuestFunction fn; };
    const Mapping mappings[] = {
        {PC_FS_CREATE_FILE_A, create_file_a},
        {PC_FS_READ_FILE, read_file},
        {PC_FS_WRITE_FILE, write_file},
        {PC_FS_CLOSE_HANDLE, close_handle},
        {PC_FS_SET_FILE_POINTER, set_file_pointer},
        {PC_FS_GET_FILE_SIZE, get_file_size},
    };
    for (const auto& m : mappings) {
        auto status = pc_register_import(r, m.id, m.fn, nullptr);
        if (status != PC_OK) return status;
    }
    return PC_OK;
}
