OBJECTS = $(addprefix bin/,\
	contexts.o \
	logic.o \
	main.o \
	lib/json11/json11.o \
)

CXXFLAGS = -g -O2 -std=c++11 -Wall -Werror

all: synthesis

synthesis: $(OBJECTS)
	clang++ -g -o synthesis -lz3 $(OBJECTS)

bin/%.o: src/%.cpp src/*.h
	@mkdir -p $(basename $@)
	clang++ $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf bin synthesis
