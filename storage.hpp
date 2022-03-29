#include <string>
#include <vector>

#ifdef __linux__
  #include <elf.h>
  #include <link.h>
#elif defined(__APPLE__)
  #include <mach-o/loader.h>
#endif

#include <string.h>
#include <stdexcept>
#include <system_error>

#include "MemoryMappedFile.hpp"

// STORE(name, str)
//
// Append `str` to the shared static storage `name`.

#ifdef __clang__

  #ifdef __APPLE__
    #define SECTION_ATTR(name) __attribute__((section("__DATA_CONST," name), used))
  #else
    #define SECTION_ATTR(name) __attribute__((section(name), used))
  #endif

  #define STORE(name, str) {               \
    SECTION_ATTR(name)                     \
    static constexpr const char s[] = str; \
  }                                        \
  /**/

#else

  #define STORE(name, str)                            \
  __asm__ (                                           \
    ".pushsection \"" name "\",\"?\",@progbits" "\n"  \
    ".asciz \"" str "\""                        "\n"  \
    ".popsection"                               "\n"  \
  );                                                  \
  /**/

#endif

// Get every string STOREd in `name` in the binary at `path`.
inline std::vector<std::string> fetch(const char* name, const char* path)
{
  std::vector<std::string> result;

#ifdef __linux__
  const MemoryMappedFile f(path);
  ElfW(Ehdr) ehdr;
  f.read(0, sizeof(ehdr), &ehdr);

  std::vector<ElfW(Shdr)> shdrs(ehdr.e_shnum);
  f.read(ehdr.e_shoff, shdrs.size() * sizeof(ElfW(Shdr)), shdrs.data());

  const ElfW(Off) string_table_offset = shdrs.at(ehdr.e_shstrndx).sh_offset;

  for (const ElfW(Shdr)& shdr : shdrs)
  {
    const std::string_view section_name = f.string(string_table_offset + shdr.sh_name);
    if (section_name == name)
    {
      for (ElfW(Xword) offset = 0; offset < shdr.sh_size;)
      {
        const std::string_view str = f.string(shdr.sh_offset + offset);
        result.emplace_back(str);
        offset += str.size() + 1;
      }
    }
  }
#elif defined(__APPLE__)
  #ifdef __LP64__
    #define MachW(x) x ## _64
  #else
    #define MachW(x) x
  #endif

  const MemoryMappedFile f(path);
  MachW(mach_header) mhdr;
  f.read(0, sizeof(mhdr), &mhdr);

  std::size_t offset = sizeof(mhdr);
  for (uint32_t i = 0; i < mhdr.ncmds; ++i)
  {
    load_command lc;
    f.read(offset, sizeof(lc), &lc);
    if (lc.cmd == LC_SEGMENT || lc.cmd == LC_SEGMENT_64) {
      MachW(segment_command) sc;
      f.read(offset, sizeof(sc), &sc);
      if (strcmp(sc.segname, "__DATA_CONST") == 0)
      {
        std::size_t soffset = offset + sizeof(sc);
        for (uint32_t j = 0; j < sc.nsects; ++j)
        {
          MachW(section) s;
          f.read(soffset, sizeof(s), &s);
          if (strcmp(s.sectname, name) == 0)
          {
            for (uint64_t doffset = 0; doffset < s.size;)
            {
              const std::string_view str = f.string(s.offset + doffset);
              result.emplace_back(str);
              doffset += str.size() + 1;
            }
          }
          soffset += sizeof(s);
        }
      }
    }
    offset += lc.cmdsize;
  }
#endif

  return result;
}
