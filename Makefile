GXX = $(CROSS_COMPILE)g++
FLEX = flex
BISON = bison

OBJS = main.o console.o mmap.o lexer.o parser.o mempeek_ast.o
GENERATED = lexer.cpp parser.cpp

DEFINES = -DYYDEBUG=1 -DASTDEBUG
INCLUDES = -Isrc -Igenerated
CFLAGS = -std=c++11 -g
LIBS = -ledit

all: prepare bin/mempeek

prepare: bin obj generated $(addprefix generated/, $(GENERATED))

vpath %.cpp src
vpath %.cpp generated
vpath %.l src
vpath %.y src

bin obj generated:
	mkdir $@

clean:
	rm -rf bin
	rm -rf obj
	rm -rf generated

bin/mempeek: $(addprefix obj/, $(OBJS))
	$(GXX) -o $@ $^ $(LIBS)

obj/%.o: %.cpp
	$(GXX) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MP -c -o $@ $<

generated/%.cpp: %.l
	$(FLEX) -o $@ $<

generated/%.cpp: %.y
	$(BISON) -d -o $@ $<

-include obj/*.d
