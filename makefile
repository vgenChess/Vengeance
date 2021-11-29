CC=g++
CFLAGS=-std=c++17 -pthread -Wall -Wextra -fopenmp -O2
DEBUG =-g -D DEBUG
SOURCES=$(wildcard src/*.cpp)
OBJECTS=$(patsubst %.cpp, %.o, $(SOURCES))
EXECUTABLE=Vengeance
TEST_EXECUTABLE=VengeanceTest

all:	$(EXECUTABLE)
debug: CFLAGS += $(DEBUG)
debug: all

test: $(TEST_EXECUTABLE)

$(TEST_EXECUTABLE):	$(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

$(EXECUTABLE):	$(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

$(OBJECTS):	src/%.o	:	src/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

build:
	@mkdir -p bin 

clean:
	rm -rf $(EXECUTABLE) $(TEST_EXECUTABLE) $(OBJECTS) bin
	find . -name "*~" -exec rm {} \;
	find . -name "*.o" -exec rm {} \;
