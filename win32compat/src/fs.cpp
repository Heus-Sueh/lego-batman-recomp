#include <win32compat/fs.h>
#include <pcmodern/file.h>
#include <pcmodern/filesystem.h>
#include <unordered_map>
#include <mutex>
#include <filesystem>

namespace win32compat {

namespace {

thread_local uint32_t tl_last_error = WIN32_ERROR_SUCCESS;

class HandleTable {
public:
    static HandleTable& instance() {
        static HandleTable s_table;
        return s_table;
    }

    uint32_t insert(pcmodern::HostFile* file) {
        std::lock_guard<std::mutex> lock(m_mutex);
        uint32_t id = ++m_counter;
        m_table[id] = file;
        return id;
    }

    pcmodern::HostFile* get(uint32_t id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_table.find(id);
        return (it != m_table.end()) ? it->second : nullptr;
    }

    bool remove(uint32_t id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_table.find(id);
        if (it != m_table.end()) {
            if (it->second) {
                pcmodern::file_close(it->second);
            }
            m_table.erase(it);
            return true;
        }
        return false;
    }

private:
    HandleTable() = default;
    std::mutex m_mutex;
    uint32_t m_counter{0x1000};
    std::unordered_map<uint32_t, pcmodern::HostFile*> m_table;
};

} // namespace

uint32_t get_last_error() {
    return tl_last_error;
}

void set_last_error(uint32_t error) {
    tl_last_error = error;
}

uint32_t create_file_a(const char* file_name, uint32_t desired_access, uint32_t share_mode,
                       void* security_attributes, uint32_t creation_disposition,
                       uint32_t flags_and_attributes, void* template_file) {
    (void)share_mode;
    (void)security_attributes;
    (void)flags_and_attributes;
    (void)template_file;

    if (!file_name) {
        set_last_error(WIN32_ERROR_INVALID_PARAMETER);
        return WIN32_INVALID_HANDLE_VALUE;
    }

    std::filesystem::path target_path;
    auto resolved = pcmodern::VirtualFileSystem::instance().resolve(file_name);
    if (resolved) {
        target_path = *resolved;
    } else if (std::filesystem::exists(file_name)) {
        target_path = std::filesystem::path(file_name);
    } else {
        if (creation_disposition == WIN32_OPEN_EXISTING) {
            set_last_error(WIN32_ERROR_FILE_NOT_FOUND);
            return WIN32_INVALID_HANDLE_VALUE;
        }
        target_path = std::filesystem::path(file_name);
    }

    const char* mode = "rb";
    if (desired_access & WIN32_GENERIC_WRITE) {
        if (creation_disposition == WIN32_CREATE_ALWAYS) mode = "wb";
        else if (creation_disposition == WIN32_OPEN_EXISTING) mode = "r+b";
        else if (creation_disposition == WIN32_OPEN_ALWAYS) mode = "a+b";
        else mode = "wb";
    }

    pcmodern::HostFile* file = pcmodern::file_open(target_path, mode);
    if (!file) {
        set_last_error(WIN32_ERROR_FILE_NOT_FOUND);
        return WIN32_INVALID_HANDLE_VALUE;
    }

    uint32_t handle_id = HandleTable::instance().insert(file);
    set_last_error(WIN32_ERROR_SUCCESS);
    return handle_id;
}

int32_t read_file(uint32_t handle, void* buffer, uint32_t bytes_to_read,
                  uint32_t* bytes_read, void* overlapped) {
    (void)overlapped;
    if (handle == 0 || handle == WIN32_INVALID_HANDLE_VALUE) {
        set_last_error(WIN32_ERROR_INVALID_HANDLE);
        return 0;
    }

    pcmodern::HostFile* file = HandleTable::instance().get(handle);
    if (!file) {
        set_last_error(WIN32_ERROR_INVALID_HANDLE);
        return 0;
    }

    if (!buffer && bytes_to_read > 0) {
        set_last_error(WIN32_ERROR_INVALID_PARAMETER);
        return 0;
    }

    size_t actual_read = pcmodern::file_read(file, buffer, bytes_to_read);
    if (bytes_read) {
        *bytes_read = static_cast<uint32_t>(actual_read);
    }

    set_last_error(WIN32_ERROR_SUCCESS);
    return 1;
}

int32_t write_file(uint32_t handle, const void* buffer, uint32_t bytes_to_write,
                   uint32_t* bytes_written, void* overlapped) {
    (void)overlapped;
    if (handle == 0 || handle == WIN32_INVALID_HANDLE_VALUE) {
        set_last_error(WIN32_ERROR_INVALID_HANDLE);
        return 0;
    }

    pcmodern::HostFile* file = HandleTable::instance().get(handle);
    if (!file) {
        set_last_error(WIN32_ERROR_INVALID_HANDLE);
        return 0;
    }

    if (!buffer && bytes_to_write > 0) {
        set_last_error(WIN32_ERROR_INVALID_PARAMETER);
        return 0;
    }

    size_t actual_written = pcmodern::file_write(file, buffer, bytes_to_write);
    if (bytes_written) {
        *bytes_written = static_cast<uint32_t>(actual_written);
    }

    set_last_error(WIN32_ERROR_SUCCESS);
    return 1;
}

uint32_t set_file_pointer(uint32_t handle, int32_t distance_to_move,
                          int32_t* distance_to_move_high, uint32_t move_method) {
    if (handle == 0 || handle == WIN32_INVALID_HANDLE_VALUE) {
        set_last_error(WIN32_ERROR_INVALID_HANDLE);
        return WIN32_INVALID_SET_FILE_POINTER;
    }

    pcmodern::HostFile* file = HandleTable::instance().get(handle);
    if (!file) {
        set_last_error(WIN32_ERROR_INVALID_HANDLE);
        return WIN32_INVALID_SET_FILE_POINTER;
    }

    int origin = 0; // SEEK_SET
    if (move_method == 1) origin = 1; // SEEK_CUR
    else if (move_method == 2) origin = 2; // SEEK_END

    int64_t result = pcmodern::file_seek(file, distance_to_move, origin);
    if (result < 0) {
        set_last_error(WIN32_ERROR_INVALID_PARAMETER);
        return WIN32_INVALID_SET_FILE_POINTER;
    }

    if (distance_to_move_high) {
        *distance_to_move_high = 0;
    }

    set_last_error(WIN32_ERROR_SUCCESS);
    return static_cast<uint32_t>(result);
}

uint32_t get_file_size(uint32_t handle, uint32_t* file_size_high) {
    if (handle == 0 || handle == WIN32_INVALID_HANDLE_VALUE) {
        set_last_error(WIN32_ERROR_INVALID_HANDLE);
        return WIN32_INVALID_FILE_SIZE;
    }

    pcmodern::HostFile* file = HandleTable::instance().get(handle);
    if (!file) {
        set_last_error(WIN32_ERROR_INVALID_HANDLE);
        return WIN32_INVALID_FILE_SIZE;
    }

    int64_t current = pcmodern::file_tell(file);
    pcmodern::file_seek(file, 0, 2); // SEEK_END
    int64_t size = pcmodern::file_tell(file);
    pcmodern::file_seek(file, current, 0); // SEEK_SET

    if (file_size_high) {
        *file_size_high = 0;
    }

    set_last_error(WIN32_ERROR_SUCCESS);
    return static_cast<uint32_t>(size);
}

int32_t close_handle(uint32_t handle) {
    if (handle == 0 || handle == WIN32_INVALID_HANDLE_VALUE) {
        set_last_error(WIN32_ERROR_INVALID_HANDLE);
        return 0;
    }

    // Recognize pseudo-handles (mutex/event) from host thread shims
    if (handle == 0xCAFE0001u || handle == 0xCAFE0002u) {
        set_last_error(WIN32_ERROR_SUCCESS);
        return 1;
    }

    if (!HandleTable::instance().remove(handle)) {
        set_last_error(WIN32_ERROR_INVALID_HANDLE);
        return 0;
    }

    set_last_error(WIN32_ERROR_SUCCESS);
    return 1;
}

} // namespace win32compat
