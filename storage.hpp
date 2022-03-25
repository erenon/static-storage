#include <string>
#include <vector>

#include <elf.h>
#include <link.h>
#include <sys/auxv.h>
#include <string.h>
#include <stdexcept>
#include <system_error>

#include "MemoryMappedFile.hpp"


// STORE(name, str)
//
// Append `str` to the shared static storage `name`.

#ifdef __clang__

  #define STORE(name, str) {               \
    __attribute__((section(name), used))   \
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

// Get the path of the currently running binary
inline const char* current_binary_path()
{
  char* path = nullptr;
  long unsigned path_val = getauxval(AT_EXECFN);
  memcpy(&path, &path_val, sizeof(path));

  if (!path)
  {
    throw std::runtime_error("Failed to get binary path");
  }
  return path;
}

// Get every string STOREd in `name` in the binary at `path`.
inline std::vector<std::string> fetch(const char* name, const char* path = current_binary_path())
{
  const MemoryMappedFile f(path);
  ElfW(Ehdr) ehdr;
  f.read(0, sizeof(ehdr), &ehdr);

  std::vector<ElfW(Shdr)> shdrs(ehdr.e_shnum);
  f.read(ehdr.e_shoff, shdrs.size() * sizeof(ElfW(Shdr)), shdrs.data());

  const ElfW(Off) string_table_offset = shdrs.at(ehdr.e_shstrndx).sh_offset;

  std::vector<std::string> result;

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

  return result;
}
