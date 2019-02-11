OBJECTS = $(addprefix bin/,\
	contexts.o \
	logic.o \
	main.o \
	model.o \
	smt.o \
	benchmarking.o \
	grammar.o \
	lib/json11/json11.o \
)

DEP_DIR = bin/deps

CXXFLAGS = -g -O2 -std=c++11 -Wall -Werror

all: synthesis

synthesis: $(OBJECTS)
	clang++ -g -o synthesis -lz3 $(OBJECTS)

bin/%.o: src/%.cpp
	@mkdir -p $(basename $@)
	@mkdir -p $(DEP_DIR)/$(basename $<)
	clang++ $(CXXFLAGS) -c -o $@ $< -MMD -MP -MF "$(DEP_DIR)/$(<:.cpp=.d)"

clean:
	rm -rf bin synthesis

# Include these files which contain all the dependencies
# between the source files and the header files, so if a header
# file changes, the correct things will re-compile.
# rwildcard is a helper to search recursively for .d files
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
-include $(call rwildcard,$(DEP_DIR)/,*.d)
