
CXX = g++
CXXFLAGS = -Wall -Wcast-qual -fno-exceptions -std=c++17  -pedantic -Wextra -Wshadow -m64 -DUSE_PTHREADS -DNDEBUG -O3 -DIS_64BIT -msse -msse3 -mpopcnt -DUSE_POPCNT -DUSE_AVX2 -mavx2 -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -flto -fopenmp
LDFLAGS = -m64 -Wl,--no-as-needed -lpthread -Wall -Wcast-qual -fno-exceptions -std=c++17  -pedantic -Wextra -Wshadow -m64 -DUSE_PTHREADS -DNDEBUG -O3 -DIS_64BIT -msse -msse3 -mpopcnt -DUSE_POPCNT -DUSE_AVX2 -mavx2 -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -flto -flto=jobserver -fopenmp
SRC = *.cpp
EXE = Vengeance


debug:
	$(CXX) $(CXXFLAGS) $(SRC) -g -D DEBUG -o $(EXE) $(LDFLAGS)

release: 
	$(CXX) $(CXXFLAGS) $(SRC) -o $(EXE) $(LDFLAGS)

all: release

clean:
	rm -rf $(EXE)  $(OBJECTS) 
	find . -name "*~" -exec rm {} \;
	find . -name "*.o" -exec rm {} \;
