OBJECTS = $(addprefix bin/,\
	contexts.o \
	logic.o \
	main.o \
	model.o \
	smt.o \
	benchmarking.o \
	grammar.o \
	bmc.o \
	enumerator.o \
	utils.o \
	sketch.o \
	synth_loop.o \
	ftree.o \
	quantifier_permutations.o \
	top_quantifier_desc.o \
	sketch_model.o \
	strengthen_invariant.o \
	sat_solver.o \
	lib/json11/json11.o \
)

LIBS = lib_glucose_debug.a

DEP_DIR = bin/deps

CXXFLAGS = -g -O2 -std=c++11 -Wall -Werror -Isrc/lib/glucose-syrup/

all: synthesis

synthesis: $(OBJECTS) $(LIBS)
	clang++ -g -o synthesis -lz3 $(OBJECTS) $(LIBS)

bin/lib_glucose_release.a:
	cd src/lib/glucose-syrup/simp/ && make libr
	cp src/lib/glucose-syrup/simp/lib_release.a bin/lib_glucose_release.a
bin/lib_glucose_debug.a:
	cd src/lib/glucose-syrup/simp/ && make libd
	cp src/lib/glucose-syrup/simp/lib_release.a bin/lib_glucose_debug.a

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
