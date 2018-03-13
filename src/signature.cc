#include "stdafx.hh"

#include "signature.hh"

#if doghook_platform_linux()
#include <dirent.h>
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

using namespace signature;

static constexpr auto in_range(char x, char a, char b) {
    return (x >= a && x <= b);
}

static constexpr auto get_bits(char x) {
    if (in_range((x & (~0x20)), 'A', 'F')) {
        return (x & (~0x20)) - 'A' + 0xa;
    } else if (in_range(x, '0', '9')) {
        return x - '0';
    } else {
        return 0;
    }
}

static constexpr auto get_byte(const char *x) {
    return get_bits(x[0]) << 4 | get_bits(x[1]);
}

static u8 *find_pattern_internal(uptr start, uptr end, const char *pattern) {
    u8 *first = 0;

    const char *pat = pattern;

    for (uptr cur = start; cur < end; cur += 1) {
        if (!*pat) return first;
        if (*(u8 *)pat == '\?' || *(u8 *)cur == get_byte(pat)) {
            if (first == 0) first = reinterpret_cast<u8 *>(cur);
            if (!pat[2]) return first;
            if (*reinterpret_cast<const u16 *>(pat) == '\?\?' ||
                *reinterpret_cast<const u8 *>(pat) != '\?')
                pat += 3;
            else
                pat += 2;
        } else {
            pat   = pattern;
            first = 0;
        }
    }
    return nullptr;
}

static std::pair<uptr, uptr> find_module_code_section(const char *module_name) {
    auto module = resolve_library(module_name);

#if doghook_platform_windows()
    auto module_addr = reinterpret_cast<uptr>(module);
    auto dos_header  = (IMAGE_DOS_HEADER *)module;
    auto nt_header   = (IMAGE_NT_HEADERS32 *)(((u32)module) + dos_header->e_lfanew);

    return std::make_pair(module_addr + nt_header->OptionalHeader.BaseOfCode,
                          module_addr + nt_header->OptionalHeader.SizeOfCode);
#elif doghook_platform_linux()

    auto lm = (link_map *)module;
    assert(lm);
    assert(lm->l_name);

    // hack becuase the elf loader doesnt map the string table
    auto fd = open(lm->l_name, O_RDONLY);
    defer(close(fd));
    assert(fd != -1);

    auto size    = lseek(fd, 0, SEEK_END);
    auto address = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(address);
    defer(munmap(address, size));

    auto module_address = reinterpret_cast<uptr>(address);
    auto ehdr           = reinterpret_cast<Elf32_Ehdr *>(module_address);
    assert(ehdr);

    auto shdr = reinterpret_cast<Elf32_Shdr *>(module_address + ehdr->e_shoff);
    assert(shdr);

    auto strhdr = &shdr[ehdr->e_shstrndx];
    assert(strhdr);

    char *strtab      = nullptr;
    int   strtab_size = 0;

    if (strhdr->sh_type == 3) {
        strtab = reinterpret_cast<char *>(module_address + strhdr->sh_offset);
        assert(strtab);

        strtab_size = strhdr->sh_size;
    }

    for (int i = 0; i < ehdr->e_shnum; i++) {
        auto hdr = &shdr[i];
        assert(hdr);
        assert(hdr->sh_name < strtab_size);

        if (strcmp(strtab + hdr->sh_name, ".text") == 0)
            return std::make_pair((uptr)lm->l_addr + hdr->sh_offset, (uptr)lm->l_addr + hdr->sh_offset + hdr->sh_size);
    }
#elif doghook_platform_osx()
#error implement me!
#endif
    // this means that you need to implement your platform
    assert(0);
}

auto signature::resolve_library(const char *name) -> void * {
#if doghook_platform_windows()
    // TODO: actually check directories for this dll instead of
    // letting the loader do the work
    char buffer[1024];
    snprintf(buffer, 1023, "%s.dll", name);

    auto handle = GetModuleHandleA(buffer);

    assert(handle);

    return handle;

#elif doghook_platform_linux()
    char found[1024];
    auto search_directory = [](const char *to_find, const char *dirname, char *out) -> bool {
        auto d = opendir(dirname);
        assert(d);
        defer(closedir(d));

        auto dir = readdir(d);
        while (dir != nullptr) {
            if (dir->d_type == DT_REG && strstr(dir->d_name, to_find) && strstr(dir->d_name, ".so")) {
                sprintf(out, "%s/%s", dirname, dir->d_name);
                return true;
            }
            dir = readdir(d);
        }
        return false;
    };

    if (search_directory(name, ".", found) ||
        search_directory(name, "./tf/bin", found) ||
        search_directory(name, "./bin", found)) {
        auto handle = dlopen(found, RTLD_NOLOAD);
        if (handle == nullptr) {
            printf("force loading library %s\n", name);
            handle = dlopen(found, RTLD_NOW);
        }
        return handle;
    }

#elif doghook_platform_osx()
#error implement me!
#endif
    assert(0);

    return nullptr;
}
void *signature::resolve_import(void *handle, const char *name) {
#if doghook_platform_windows()
    return reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(handle), name));
#elif doghook_platform_linux()
    return reinterpret_cast<void *>(dlsym(handle, name));
#elif doghook_platform_osx()
#error implement me!
#endif
}

u8 *signature::find_pattern(const char *module_name, const char *pattern) {
    auto code_section = find_module_code_section(module_name);

    return find_pattern_internal(code_section.first, code_section.second, pattern);
}

u8 *signature::find_pattern(const char *pattern, uptr start, uptr length) {
    return find_pattern_internal(start, start + length, pattern);
}

void *signature::resolve_callgate(void *address) {
    // TODO: are these the only instructions here?
    assert(reinterpret_cast<i8 *>(address)[0] == '\xE8' || reinterpret_cast<i8 *>(address)[0] == '\xE9');

    return reinterpret_cast<void *>(*reinterpret_cast<uptr *>(
                                        (reinterpret_cast<uptr>(address) + 1)) +
                                    (reinterpret_cast<uptr>(address) + 5));
}
