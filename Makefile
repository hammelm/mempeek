GXX = $(CROSS_COMPILE)g++
FLEX = flex
BISON = bison

OBJS = main.o console.o mmap.o lexer.o parser.o environment.o mempeek_ast.o mempeek_exceptions.o \
       builtins.o subroutines.o variables.o arrays.o
GENERATED = lexer.cpp parser.cpp

DEFINES = 
INCLUDES = -Isrc -Igenerated
CFLAGS = -std=c++11 -g
LIBS = -ledit

all: prepare bin/mempeek

prepare: bin obj generated $(addprefix generated/, $(GENERATED))
	@./create_buildinfo.sh

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
	$(GXX) -o $@ $^ generated/buildinfo.c $(LIBS)

obj/%.o: %.cpp
	$(GXX) $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MP -c -o $@ $<

generated/%.cpp: %.l
	$(FLEX) --header-file=$(basename $@).h -o $@ $<

generated/%.cpp: %.y
	$(BISON) --defines=$(basename $@).h -o $@ $<

-include obj/*.d
