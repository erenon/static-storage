#include "storage.hpp"

#include <iostream>

void f()
{
  STORE("names", "alice");
}

inline void g()
{
  STORE("names", "bob");
}

template <typename = void>
void h()
{
  STORE("names", "charlie");
}

int main(int argc, const char* argv[])
{
  if (argc < 1) { return 1; }
  g();
  h();
  for (const std::string& name : fetch("names", argv[0]))
  {
    std::cout << name << "\n";
  }
}
