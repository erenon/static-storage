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

int main()
{
  g();
  h();
  for (const std::string& name : fetch("names"))
  {
    std::cout << name << "\n";
  }
}
