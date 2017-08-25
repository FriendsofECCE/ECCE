///////////////////////////////////////////////////////////////////////////////
// FILENAME: ETimer.H
//
//
// CLASS SYNOPSIS:
//   Class for benchmarking with "Elapsed" time and CPU time.
//
// Description
//   This class is a replacement for using RWTime and RWTimer to benchmark
//   operations.  Elapsed time is in milliseconds so actually has better
//   resolution.  
//
//   CPU time is implemented with the system-dependent clock() which
//   returns the number of "ticks" since it was first called.  As a
//   result, it may not be able to measure intervals longer than some
//   system-dependent value (on some UNIX sytems, this value is just
//   under 36 minutes).
//
// Example:
//   ETimer timer;
//   timer.start();
//   <your code>
//   timer.stop();
//   cout << timer.elapsedTime() << endl;
//   cout << timer.CPUTime() << endl;
///////////////////////////////////////////////////////////////////////////////

#ifndef ETIMER_HH
#define ETIMER_HH


class ETimer {
  public:
    ETimer();
    ETimer(const ETimer& rhs);
    virtual ~ETimer();

    void start();
    void stop();
    void reset();
    float elapsedTime() const;  // seconds
    float CPUTime() const;      // seconds

    // Elapsed milliseconds between stop and start
    unsigned long milliseconds() const;

  protected:
    // Hid this just to keep interface small; expose if desired.

    static unsigned long currentMilliseconds();

    unsigned long p_start;
    unsigned long p_stop;
    long  p_clockstart;
    long  p_clockstop;


};

#endif
