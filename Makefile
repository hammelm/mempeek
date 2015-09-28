GXX = $(CROSS_COMPILE)g++
FLEX = flex
BISON = bison

OBJS = main.o console.o mmap.o lexer.o parser.o
GENERATED = lexer.cpp parser.cpp

all: prepare bin/mempeek

prepare: bin obj generated $(addprefix generated/, $(GENERATED))

vpath %.cpp src
vpath %.cpp generated
vpath %.l src
vpath %.y src
vpath %.o obj

bin obj generated:
	mkdir $@

clean:
	rm -rf bin
	rm -rf obj
	rm -rf generated

bin/mempeek: $(addprefix obj/, $(OBJS))
	$(GXX) -o $@ $^ -ledit

obj/%.o: %.cpp
	$(GXX) -std=c++11 -g -c -o $@ $<

generated/%.cpp: %.l
	$(FLEX) -o $@ $<

generated/%.cpp: %.y
	$(BISON) -d -o $@ $<
