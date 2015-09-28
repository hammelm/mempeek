GXX = $(CROSS_COMPILE)g++
FLEX = flex
BISON = bison

OBJS = console.o mmap.o

all: bin obj generated bin/mempeek

vpath %.cpp src
vpath %.cpp generated
vpath %.o obj

bin obj generated:
	mkdir $@

clean:
	rm -rf bin
	rm -rf obj
	rm -rf generated

bin/mempeek: $(OBJS)
	$(GXX) -o $@ $^

%.o: %.cpp
	$(GXX) -std=c++11 -g -c -o obj/$@ $<

%.cpp: %.l
	$(FLEX) -o generated/$@ $<

%.cpp: %.y
	$(BISON) -d -o generated/$@ $<
