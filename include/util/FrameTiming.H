#ifndef FRAMETIMING_H
#define FRAMETIMING_H

// Per-frame phase timings of geometry-trace playback (#570).  With
// ECCE_FRAME_TIMING=<file> set, each processed frame appends one line
// "frame <step> total=<ms> <phase>=<ms> ... idle=<ms>" to that file, idle
// being the time since the previous frame ended (playback delay plus any
// render the event loop ran in between).  Unset, every call is a no-op.
namespace frametiming {
  bool enabled();
  double now();                                 // ms, monotonic
  void add(const char *phase, double ms);       // accumulates into the frame
  void begin(int step);
  void end();

  // Adds the time of its scope to a phase.
  class Scope {
  public:
    explicit Scope(const char *phase)
      : p_phase(phase), p_t0(enabled() ? now() : 0) {}
    ~Scope() { if (enabled()) add(p_phase, now() - p_t0); }
  private:
    const char *p_phase;
    double p_t0;
  };
}

#endif
