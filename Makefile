GXX = $(CROSS_COMPILE)g++
FLEX = flex
BISON = bison

OBJS = main.o console.o mmap.o lexer.o parser.o environment.o mempeek_ast.o mempeek_exceptions.o builtins.o
GENERATED = lexer.cpp parser.cpp

DEFINES = -DUSE_EDITLINE
INCLUDES = -Isrc -Igenerated
CFLAGS = -g
LIBS = -ledit

all: bin/mempeek

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

bin/mempeek: | bin buildinfo
bin/mempeek: $(addprefix obj/, $(OBJS))
	$(GXX) -o $@ $^ generated/buildinfo.c $(LIBS)

obj/%.o: %.cpp | obj buildinfo $(addprefix generated/, $(GENERATED))
	$(GXX) -std=c++11 $(CFLAGS) $(DEFINES) $(INCLUDES) -MMD -MP -c -o $@ $<

generated/%.cpp: %.l | generated
	$(FLEX) --header-file=$(basename $@).h -o $@ $<

generated/%.cpp: %.y | generated
	$(BISON) --defines=$(basename $@).h -o $@ $<

.PHONY: buildinfo
buildinfo: | generated
	@./create_buildinfo.sh

obj/lexer.o: generated/parser.cpp

-include obj/*.d
