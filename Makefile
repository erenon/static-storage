.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.PHONY: clean

MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

test: test.cpp other_translation_unit.cpp storage.hpp Makefile
	$(CXX) -Wall -Werror -Wextra -std=c++17 test.cpp other_translation_unit.cpp -o $@

check: test
	diff <(./test|sort) <(echo -en "alice\nbob\ncharlie\ndom\n")

clean:
	rm test
