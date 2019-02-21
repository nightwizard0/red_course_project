#pragma once

#include <string>
#include <chrono>
#include <iostream>
#include <map>

using namespace std;
using namespace std::chrono;

#define PROFILE_ENABLE 0

#define UNIQ_ID_IMPL(lineno) _a_local_var_##lineno
#define UNIQ_ID(lineno) UNIQ_ID_IMPL(lineno)

#if PROFILE_ENABLE
#define STAT_DURATION(profiler, message) \
  auto UNIQ_ID(__LINE__) = profiler.Measure(message);
#else
#define STAT_DURATION(profiler, message) 
#endif

class IProfiler
{
public:
    virtual void Update(const string& title, nanoseconds ns) = 0; 
};

class Duration
{
public:

    ~Duration()
    {
        auto duration = steady_clock::now() - m_start;
        m_profiler->Update(m_title, duration);
    }

private:
    IProfiler * m_profiler;
    string m_title;
    steady_clock::time_point m_start;
    
    Duration(const string& title, IProfiler * profiler) 
        : m_profiler(profiler),
        m_title(title),
        m_start(steady_clock::now())
    {}

    friend class Profiler;
};

class Profiler : IProfiler
{
public:
    Profiler() = default;

    Duration Measure(const string& title)
    {
        return Duration(title, this);
    }

    void Update(const string& title, nanoseconds ns)
    {
        m_durations[title] += ns;
    }

    ~Profiler()
    {
        for (const auto& [title, ns] : m_durations)
        {
            cerr << title << " " 
                << duration_cast<milliseconds>(ns).count()
                << " ms" << endl;
        }
    }

private:
    map<string, nanoseconds> m_durations;
    
};