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
    
    inline static TimePoint time_now() 
    {
        return std::chrono::steady_clock::now();
    } 
    
    inline static int time_elapsed_milliseconds(TimePoint t) 
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(time_now() - t).count();
    } 
    
    inline static int time_elapsed_seconds(TimePoint t) 
    {
        return std::chrono::duration_cast<std::chrono::seconds>(time_now() - t).count();
    }
    
    
    inline bool isTimeSet() const 
    {
        return mIsTimeSet;
    }
    
    inline bool isStopped() const
    {
        return mIsStopped;
    }
    
    inline TimePoint getStartTime() const
    {
        return mStartTime;
    }
    
    inline TimePoint getStopTime() const
    {
        return mStopTime;
    }        
    
    inline int getTimePerMove() const
    {
        return mTimePerMove;
    }
    
    
    inline void updateTimeSet(const bool isTimeSet)
    {
        mIsTimeSet = isTimeSet;
    }
    
    inline void updateStopped(const bool isStopped)
    {
        mIsStopped = isStopped;
    }
    
    inline void updateTimePerMove(const int timePerMove)
    {
        mTimePerMove = timePerMove;
    }
    
    inline void setStartTime(const TimePoint startTime) 
    {
        mStartTime = startTime;
    }
    
    inline void setStopTime(const TimePoint stopTime)
    {
        mStopTime = stopTime;
    }
    
};

#endif
