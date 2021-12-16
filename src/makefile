#CC=g++
#CFLAGS=-std=c++14 -pthread -Wall -Wextra -fopenmp -O2 -march=native
#DEBUG =-g -D DEBUG
#SOURCES=$(wildcard *.cpp)
#OBJECTS=$(patsubst %.cpp, %.o, $(SOURCES))
#EXECUTABLE=Vengeance

#all:	$(EXECUTABLE)
#debug: CFLAGS += $(DEBUG)
#debug: all

#$(EXECUTABLE):	$(OBJECTS)
	#$(CC) $(CFLAGS) -o $@ $(OBJECTS)

#$(OBJECTS):	%.o	:	%.cpp
	#$(CC) $(CFLAGS) -c $< -o $@

#clean:
	#rm -rf $(EXECUTABLE)  $(OBJECTS) 
	#find . -name "*~" -exec rm {} \;
	#find . -name "*.o" -exec rm {} \;
	
	
# new makefile below
	 
 ​CC​ = g++
 ​SRC​ = *.cpp
 ​EXE​ = Vengeance
 ​VERSION​ = 1-alpha
 ​EVALFILE​ = default.nn 
  
 ​WFLAGS​ = -std=c++17 -Wall -Wextra -Wshadow 
 ​CFLAGS​ = -O3 ​$(​WFLAGS​)​ -g -flto -march=native 
 ​RFLAGS​ = -O3 ​$(​WFLAGS​)​ -g -flto -static
 
 SSSE​ = -mssse3 -msse3 -msse2 -msse -mpopcnt 
 ​AVX2​ = -mavx2 -mavx -mfma -msse4.1 ​$(​SSSE​)

all: $(​CC​)​ ​$(​RFLAGS​)​ ​$(​SRC​)​​ ​$(​AVX2​)​ -o ​$(​EXE​)​

clean:
	rm -rf $(EXE)  $(OBJECTS) 
	find . -name "*~" -exec rm {} \;
	find . -name "*.o" -exec rm {} \;
