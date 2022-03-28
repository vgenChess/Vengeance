
CXX = g++
CXXFLAGS = -Wall -Wcast-qual -fno-exceptions -std=c++17  -pedantic -Wextra -Wshadow -m64 -DUSE_PTHREADS -DNDEBUG -O3 -DIS_64BIT -msse -msse3 -mpopcnt -DUSE_POPCNT -DUSE_AVX2 -mavx2 -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -flto -fopenmp
LDFLAGS = -m64 -Wl,--no-as-needed -lpthread -Wall -Wcast-qual -fno-exceptions -std=c++17  -pedantic -Wextra -Wshadow -m64 -DUSE_PTHREADS -DNDEBUG -O3 -DIS_64BIT -msse -msse3 -mpopcnt -DUSE_POPCNT -DUSE_AVX2 -mavx2 -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -flto -flto=jobserver -fopenmp
SRC = *.cpp
EXE = Vengeance

all: 
	$(CXX) $(CXXFLAGS) $(SRC) -o $(EXE) $(LDFLAGS)

tune:
	$(CXX) $(CXXFLAGS) $(SRC) -D __TUNE -o $(EXE) $(LDFLAGS)

tuneTest:
	$(CXX) $(CXXFLAGS) $(SRC) -D __TEST_TUNER -o $(EXE) $(LDFLAGS)

tuneWithTunedEvals:
	$(CXX) $(CXXFLAGS) $(SRC) -D __TUNE_WITH_TUNED_EVALS -o $(EXE) $(LDFLAGS)

debug:
	$(CXX) $(CXXFLAGS) $(SRC) -g -D __DEBUG -o $(EXE) $(LDFLAGS)

release: 
	$(CXX) $(CXXFLAGS) $(SRC) -o $(EXE) $(LDFLAGS)

clean:
	rm -rf $(EXE)  $(OBJECTS) 
	find . -name "*~" -exec rm {} \;
	find . -name "*.o" -exec rm {} \;
