#include "section_loader.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include "bridge_adapter.h"
#include <pcrecomp/modules.h>

namespace pcrecomp::runtime {

namespace {
std::vector<LoadedSectionInfo> s_last_loaded;
}

const std::vector<LoadedSectionInfo>& SectionLoader::last_loaded_sections() {
    return s_last_loaded;
}

bool SectionLoader::load_pe_data_sections(const std::filesystem::path& exe_path, GuestMemory& memory) {
    s_last_loaded.clear();

    if (!memory.is_initialized()) {
        std::cerr << "[SectionLoader] Error: GuestMemory not initialized.\n";
        return false;
    }

    std::ifstream file(exe_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[SectionLoader] Error: Failed to open executable: " << exe_path << "\n";
        return false;
    }

    // 1. Read DOS Header
    uint8_t dos_header[64];
    if (!file.read(reinterpret_cast<char*>(dos_header), sizeof(dos_header))) {
        std::cerr << "[SectionLoader] Error: Short read on DOS header.\n";
        return false;
    }

    if (dos_header[0] != 'M' || dos_header[1] != 'Z') {
        std::cerr << "[SectionLoader] Error: Missing MZ signature.\n";
        return false;
    }

    uint32_t pe_offset = *reinterpret_cast<const uint32_t*>(&dos_header[0x3C]);
    file.seekg(pe_offset);

    // 2. Read PE Header Signature & FileHeader
    uint8_t pe_sig[4];
    if (!file.read(reinterpret_cast<char*>(pe_sig), sizeof(pe_sig)) ||
        pe_sig[0] != 'P' || pe_sig[1] != 'E' || pe_sig[2] != 0 || pe_sig[3] != 0) {
        std::cerr << "[SectionLoader] Error: Missing PE signature.\n";
        return false;
    }

    uint16_t machine = 0;
    uint16_t num_sections = 0;
    uint32_t timedatestamp = 0;
    uint32_t sym_table = 0;
    uint32_t num_syms = 0;
    uint16_t opt_header_size = 0;
    uint16_t characteristics = 0;

    file.read(reinterpret_cast<char*>(&machine), 2);
    file.read(reinterpret_cast<char*>(&num_sections), 2);
    file.read(reinterpret_cast<char*>(&timedatestamp), 4);
    file.read(reinterpret_cast<char*>(&sym_table), 4);
    file.read(reinterpret_cast<char*>(&num_syms), 4);
    file.read(reinterpret_cast<char*>(&opt_header_size), 2);
    file.read(reinterpret_cast<char*>(&characteristics), 2);

    if (machine != 0x014C) {
        std::cerr << "[SectionLoader] Warning: Machine type is not i386 (0x014C): 0x"
                  << std::hex << machine << std::dec << "\n";
    }

    std::vector<uint8_t> opt_hdr_buf(opt_header_size);
    if (!file.read(reinterpret_cast<char*>(opt_hdr_buf.data()), opt_header_size)) return false;
    auto read_le32 = [](const uint8_t* p) {
        return uint32_t(p[0]) | uint32_t(p[1])<<8 | uint32_t(p[2])<<16 | uint32_t(p[3])<<24;
    };
    const uint32_t image_base = read_le32(opt_hdr_buf.data()+28);
    const uint32_t image_size = read_le32(opt_hdr_buf.data()+56);
    const uint64_t entry_point = uint64_t(image_base)+read_le32(opt_hdr_buf.data()+16);
    if (entry_point > UINT32_MAX || !memory.is_valid(image_base,image_size)) return false;

    // Map DOS and PE headers into guest memory at image_base (so CRT header checks succeed)
    file.seekg(0);
    uint8_t* base_dest = memory.translate(image_base);
    if (base_dest) {
        file.read(reinterpret_cast<char*>(base_dest), 0x1000);
        std::cout << "[SectionLoader] Mapped DOS/PE headers at VA 0x" << std::hex << image_base << std::dec << "\n";
    }

    // 3. Skip Optional Header to reach Section Headers
    file.seekg(pe_offset + 4 + 20 + opt_header_size);

    std::cout << "[SectionLoader] Parsing " << num_sections << " PE sections from " << exe_path.filename()
              << " (ImageBase: 0x" << std::hex << image_base << std::dec << ")...\n";

    // 4. Iterate Section Headers
    for (uint16_t i = 0; i < num_sections; ++i) {
        uint8_t sec_hdr[40];
        if (!file.read(reinterpret_cast<char*>(sec_hdr), sizeof(sec_hdr))) {
            break;
        }

        char name_buf[9] = {0};
        std::memcpy(name_buf, &sec_hdr[0], 8);
        std::string sec_name(name_buf);

        uint32_t virtual_size   = *reinterpret_cast<const uint32_t*>(&sec_hdr[8]);
        uint32_t virtual_addr   = *reinterpret_cast<const uint32_t*>(&sec_hdr[12]);
        uint32_t raw_size       = *reinterpret_cast<const uint32_t*>(&sec_hdr[16]);
        uint32_t raw_ptr        = *reinterpret_cast<const uint32_t*>(&sec_hdr[20]);
        uint32_t chars          = *reinterpret_cast<const uint32_t*>(&sec_hdr[36]);

        uint32_t sec_va = image_base + virtual_addr;

        LoadedSectionInfo info;
        info.name = sec_name;
        info.va = sec_va;
        info.virtual_size = virtual_size;
        info.raw_size = raw_size;
        info.loaded = false;

        // Load clean PE sections including .text (for embedded jump/switch tables and constants)
        bool is_data_section = (sec_name == ".text" || sec_name == ".rdata" || sec_name == ".data" ||
                                sec_name == ".idata" || sec_name == ".shr" || sec_name == "CONST" ||
                                sec_name == "_TEXT_HA" || sec_name == "_rwdseg" ||
                                sec_name == ".rsrc");

        // Alternatively check non-executable sections below the DRM boundary (< 0x008B0000 RVA)
        if (!is_data_section && (chars & 0x20000000) == 0 && virtual_addr < 0x008B0000) {
            is_data_section = true;
        }

        if (is_data_section && raw_size > 0 && raw_ptr > 0) {
            uint8_t* dest = memory.translate(sec_va);
            if (dest && memory.is_valid(sec_va, virtual_size)) {
                auto current_pos = file.tellg();
                file.seekg(raw_ptr);

                uint32_t copy_size = std::min(raw_size, virtual_size);
                if (file.read(reinterpret_cast<char*>(dest), copy_size)) {
                    // Zero uninitialized tail (BSS inside section)
                    if (virtual_size > copy_size) {
                        std::memset(dest + copy_size, 0, virtual_size - copy_size);
                    }
                    info.loaded = true;
                    std::cout << "  [+] Loaded " << sec_name << " at VA 0x"
                              << std::hex << sec_va << " (RVA 0x" << virtual_addr
                              << ", size: 0x" << copy_size << ")"
                              << std::dec << "\n";
                }
                file.seekg(current_pos);
            }
        }

        s_last_loaded.push_back(info);
    }

    // Populate IAT slots with their own VAs so indirect calls dispatch through the IAT slot address
    uint32_t imp_dir_rva = (opt_header_size >= 104) ? read_le32(opt_hdr_buf.data() + 96 + 8 * 1) : 0;
    if (imp_dir_rva > 0) {
        uint32_t desc_va = image_base + imp_dir_rva;
        uint32_t bound_slots = 0;
        while (memory.is_valid(desc_va, 20)) {
            uint32_t first_thunk = memory.read32(desc_va + 16);
            uint32_t name_rva = memory.read32(desc_va + 12);
            if (first_thunk == 0 && name_rva == 0) break;
            if (first_thunk != 0) {
                uint32_t thunk_va = image_base + first_thunk;
                while (memory.is_valid(thunk_va, 4) && memory.read32(thunk_va) != 0) {
                    memory.write32(thunk_va, thunk_va);
                    bound_slots++;
                    thunk_va += 4;
                }
            }
            desc_va += 20;
        }
        std::cout << "[SectionLoader] Initialized " << bound_slots << " IAT slots with dispatch addresses.\n";
    }

    if (!verify_data_integrity(memory)) return false;
    const auto name = exe_path.filename().string();
    const PcGuestModule module{name.c_str(),image_base,image_size,static_cast<uint32_t>(entry_point)};
    const auto status=pc_register_module(bridge_runtime(),&module);
    if (status == PC_DUPLICATE) {
        PcGuestModule existing{};
        return pc_find_module(bridge_runtime(),image_base,&existing)==PC_OK &&
            existing.image_base==image_base && existing.image_size==image_size &&
            existing.entry_point==entry_point && name==existing.name;
    }
    return status==PC_OK;
}

bool SectionLoader::verify_data_integrity(const GuestMemory& memory) {
    if (!memory.is_initialized()) return false;

    // Check .rdata range (typically starts at VA 0x00858000)
    const uint8_t* rdata = memory.translate(0x00858000);
    bool rdata_has_data = false;
    if (rdata) {
        for (size_t i = 0; i < 4096; ++i) {
            if (rdata[i] != 0) {
                rdata_has_data = true;
                break;
            }
        }
    }

    // Check .data range (typically starts at VA 0x008A4000)
    const uint8_t* data = memory.translate(0x008A4000);
    bool data_has_data = false;
    if (data) {
        for (size_t i = 0; i < 4096; ++i) {
            if (data[i] != 0) {
                data_has_data = true;
                break;
            }
        }
    }

    std::cout << "[SectionLoader] Integrity check: .rdata populated: "
              << (rdata_has_data ? "YES" : "NO")
              << ", .data populated: "
              << (data_has_data ? "YES" : "NO") << "\n";

    return rdata_has_data || data_has_data;
}

} // namespace pcrecomp::runtime
