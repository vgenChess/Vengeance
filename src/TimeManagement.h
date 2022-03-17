#ifndef TIME_MANAGEMENT_H
#define TIME_MANAGEMENT_H

#include <ctime>
#include <ratio>
#include <chrono>

#include "types.h"

// for time management

class TimeManager 
{    
    
private:
    
    bool mIsTimeSet, mIsStopped;
    int mTimePerMove;
    
    TimePoint mStartTime, mStopTime;
    
public:
    
    static TimeManager sTimeManager;
    
    __always_inline static TimePoint time_now() 
    {
        return std::chrono::steady_clock::now();
    } 
    
    __always_inline static int time_elapsed_milliseconds(TimePoint t) 
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(time_now() - t).count();
    } 
    
    __always_inline static int time_elapsed_seconds(TimePoint t) 
    {
        return std::chrono::duration_cast<std::chrono::seconds>(time_now() - t).count();
    }
    
    
    __always_inline bool isTimeSet() const 
    {
        return mIsTimeSet;
    }
    
    __always_inline bool isStopped() const
    {
        return mIsStopped;
    }
    
    __always_inline TimePoint getStartTime() const
    {
        return mStartTime;
    }
    
    __always_inline TimePoint getStopTime() const
    {
        return mStopTime;
    }        
    
    __always_inline int getTimePerMove() const
    {
        return mTimePerMove;
    }
    
    
    __always_inline void updateTimeSet(const bool isTimeSet)
    {
        mIsTimeSet = isTimeSet;
    }
    
    __always_inline void updateStopped(const bool isStopped)
    {
        mIsStopped = isStopped;
    }
    
    __always_inline void updateTimePerMove(const int timePerMove)
    {
        mTimePerMove = timePerMove;
    }
    
    __always_inline void setStartTime(const TimePoint startTime) 
    {
        mStartTime = startTime;
    }
    
    __always_inline void setStopTime(const TimePoint stopTime)
    {
        mStopTime = stopTime;
    }
    
};

#endif
