OBJECTS = $(addprefix bin/,\
	contexts.o \
	logic.o \
	main.o \
	model.o \
	expr_gen_smt.o \
	benchmarking.o \
	grammar.o \
	bmc.o \
	enumerator.o \
	utils.o \
	synth_loop.o \
	ftree.o \
	quantifier_permutations.o \
	top_quantifier_desc.o \
	strengthen_invariant.o \
	model_isomorphisms.o \
	wpr.o \
	filter.o \
	naive_synth_enumerator.o \
	obviously_implies.o \
	var_lex_graph.o \
	big_disjunct_synth_enumerator.o \
	alt_synth_enumerator.o \
	alt_impl_synth_enumerator.o \
	alt_depth2_synth_enumerator.o \
	smt.o \
	smt_z3.o \
	smt_cvc4.o \
	tree_shapes.o \
	template_counter.o \
	lib/json11/json11.o \
)

ifdef GLUCOSE_RELEASE
GLUCOSE_LIB = bin/lib_glucose_release.a
else
GLUCOSE_LIB = bin/lib_glucose_debug.a
endif

#LIBS = $(GLUCOSE_LIB)
LIBS =

DEP_DIR = bin/deps

CXXFLAGS = -g -O2 -std=c++11 -Wall -Werror -Isrc/lib/glucose-syrup/ -Wsign-compare -Wunused-variable
#-DSMT_CVC4

all: synthesis

glucoselib: GLUCOSE_LIB

synthesis: $(OBJECTS) $(LIBS)
	clang++ -g -o synthesis $(LIBPATH) $(OBJECTS) $(LIBS) -lz3 -lcvc4 -lpthread

bin/lib_glucose_release.a:
	@mkdir -p $(basename $@)
	cd src/lib/glucose-syrup/simp/ && make libr
	cp src/lib/glucose-syrup/simp/lib_release.a bin/lib_glucose_release.a
bin/lib_glucose_debug.a:
	@mkdir -p $(basename $@)
	cd src/lib/glucose-syrup/simp/ && make libd
	cp src/lib/glucose-syrup/simp/lib_debug.a bin/lib_glucose_debug.a

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
