#ifndef TIME_H
#define TIME_H

#include <ctime>
#include <ratio>
#include <chrono>

#include "types.h"

// for time management

class TimeManager {

public:
    
    static TimeManager timeManager;
    
    bool timeSet, stopped;
    int timePerMove;
    
    TimePoint startTime, stopTime;
};

#endif
