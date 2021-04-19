OBJECTS = $(addprefix bin/,\
	contexts.o \
	logic.o \
	main.o \
	model.o \
	benchmarking.o \
	bmc.o \
	enumerator.o \
	utils.o \
	synth_loop.o \
	quantifier_permutations.o \
	top_quantifier_desc.o \
	strengthen_invariant.o \
	model_isomorphisms.o \
	wpr.o \
	filter.o \
	synth_enumerator.o \
	obviously_implies.o \
	var_lex_graph.o \
	alt_synth_enumerator.o \
	alt_depth2_synth_enumerator.o \
	smt.o \
	smt_z3.o \
	smt_cvc4.o \
	tree_shapes.o \
	template_counter.o \
	template_desc.o \
	template_priority.o \
	clause_gen.o \
	solve.o \
	auto_redundancy_filters.o \
	lib/json11/json11.o \
)

LIBS =

DEP_DIR = bin/deps

CXXFLAGS = -g -O2 -std=c++11 -Wall -Werror -Wsign-compare -Wunused-variable
#-DSMT_CVC4

all: synthesis

synthesis: $(OBJECTS) $(LIBS)
	clang++ -g -o synthesis $(LIBPATH) $(OBJECTS) $(LIBS) -lz3 -lcvc4 -lpthread

bin/%.o: src/%.cpp
	@mkdir -p $(basename $@)
	@mkdir -p $(DEP_DIR)/$(basename $<)
	clang++ $(CXXFLAGS) $(INCLUDES) -c -o $@ $< -MMD -MP -MF "$(DEP_DIR)/$(<:.cpp=.d)"

clean:
	rm -rf bin synthesis

# Include these files which contain all the dependencies
# between the source files and the header files, so if a header
# file changes, the correct things will re-compile.
# rwildcard is a helper to search recursively for .d files
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
-include $(call rwildcard,$(DEP_DIR)/,*.d)
