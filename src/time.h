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
    
    static TimePoint time_now() {
        
        return std::chrono::steady_clock::now();
    } 

    static int time_elapsed_milliseconds(TimePoint t) {
        
        return std::chrono::duration_cast<std::chrono::milliseconds>(time_now() - t).count();
    } 

    static int time_elapsed_seconds(TimePoint t) {
        
        return std::chrono::duration_cast<std::chrono::seconds>(time_now() - t).count();
    } 
};

#endif
