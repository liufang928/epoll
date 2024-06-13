#pragma once

#include <iostream>
#include <chrono>

class Timer
{
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;

public:
    Timer()
    {
        update();
    }
    ~Timer()
    {
    }
    void update()
    {
        m_start = std::chrono::high_resolution_clock::now();
    }
    // 秒
    double getSecond()
    {
        return getNanoSec() * 0.000000001;
    }
    // 毫秒
    double getMilliSec()
    {
        return getNanoSec() * 0.000001;
    }
    // 微妙
    long long getMicroSec()
    {
        return getNanoSec() * 0.001;
    }
    // 纳秒
    long long getNanoSec()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_start).count();
    }
};
