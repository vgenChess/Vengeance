CC=g++
SRC = *.cpp
#DEBUG =-g -D DEBUG
#OBJECTS=$(patsubst %.cpp, %.o, $(SOURCES))
EXE=Vengeance

WFLAGS = -std=c++17 -Wall -Wextra -Wshadow -fopenmp
CFLAGS = -O3 $(WFLAGS) -g -flto -march=native
RFLAGS = -O3 $(WFLAGS) -g -flto -static

SSSE = -mssse3 -msse3 -msse2 -msse -mpopcnt
AVX2 = -mavx2 -mavx -mfma -msse4.1 $(SSSE)


#all:	$(EXECUTABLE)
#debug: CFLAGS += $(DEBUG)
#debug: all

#$(EXECUTABLE):	$(OBJECTS)
	#$(CC) $(CFLAGS) -o $@ $(OBJECTS)

#$(OBJECTS):	%.o	:	%.cpp
	#$(CC) $(CFLAGS) -c $< -o $@
	
avx2:
	$(CC) $(RFLAGS) $(SRC) $(AVX2) -o $(EXE)

all: avx2

clean:
	rm -rf $(EXE)  $(OBJECTS) 
	find . -name "*~" -exec rm {} \;
	find . -name "*.o" -exec rm {} \;

