#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>

#ifdef __APPLE__
#include <sys/time.h>
#endif

#include "util/ETimer.H"


ETimer::ETimer()
{
  p_start = 0;
  p_stop = 0;
  p_clockstart = 0;
  p_clockstop = 0;
}

ETimer::ETimer(const ETimer& rhs)
{
  p_start = rhs.p_start;
  p_stop = rhs.p_stop;
  p_clockstart = rhs.p_clockstart;
  p_clockstop = rhs.p_clockstop;
}


ETimer::~ETimer()
{
}

void ETimer::start()
{
  p_start = ETimer::currentMilliseconds();
  p_clockstart = clock();
  p_stop = 0;
  p_clockstop = 0;

}

void ETimer::stop()
{
  if (p_start > 0) { 
    p_stop = ETimer::currentMilliseconds();
    p_clockstop = clock();
  }
}

void ETimer::reset()
{
  p_start = 0;
  p_stop = 0;
  p_clockstart = 0;
  p_clockstop = 0;
}

//////////////////////////////////////////////////////////////////////////////
// elapsedTime
//   Returns the seconds (with millisecond resolution) of the timer.
//   We try to do something reasonable even if the client didn't properly
//   use start() and stop()
//////////////////////////////////////////////////////////////////////////////
float ETimer::elapsedTime() const 
{
  if (p_stop == 0)
    return (ETimer::currentMilliseconds() - p_start) / 1000.0;
  else if (p_start == 0) 
    return 0;
  else
   return (p_stop - p_start) / 1000.0;
}

//////////////////////////////////////////////////////////////////////////////
// CPUTime
//   Returns the number of cpu seconds for this timer.
//   CLOCKS_PER_SEC defined in <time.h>
//////////////////////////////////////////////////////////////////////////////
float ETimer::CPUTime() const 
{
  if (p_clockstop == 0) {
    clock_t tmp = clock();
    return (tmp - p_clockstart) / double(CLOCKS_PER_SEC);
  } else {
    return (p_clockstop - p_clockstart) / double(CLOCKS_PER_SEC);
  }
}


//////////////////////////////////////////////////////////////////////////////
// milliseconds
//   Returns the number of milliseconds of the timer.
//   We try to do something reasonable even if the client didn't properly
//   use start() and stop()
//////////////////////////////////////////////////////////////////////////////
unsigned long ETimer::milliseconds() const
{
  if (p_stop == 0)
    return ETimer::currentMilliseconds() - p_start;
  else
    return p_stop - p_start;
}

//////////////////////////////////////////////////////////////////////////////
// currentMilliseconds
//   Stolen from xerces parser code.  This is platform specific stuff but
//   probably fairly standard on unix systems.
//////////////////////////////////////////////////////////////////////////////
unsigned long ETimer::currentMilliseconds()
{
#ifdef __APPLE__
  timeval aTime;
  gettimeofday(&aTime, NULL);
  return aTime.tv_sec*1000 + aTime.tv_usec/1000;
#else
  timeb aTime;
  ftime(&aTime);
  return (unsigned long)(aTime.time*1000 + aTime.millitm);
#endif
}
