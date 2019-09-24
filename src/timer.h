
#ifndef JADAQ_TIMER
#define JADAQ_TIMER

#include <chrono>
#include <cstdint>

/// read time stamp counter - runs at processer Hz
class TSCTimer {

public:

///
TSCTimer(void) { reset(); }

///
void reset(void) { timestamp_count = rdtsc(); }

///
uint64_t timetsc(void) { return (rdtsc() - timestamp_count); }

private:
  uint64_t timestamp_count;

  unsigned long long rdtsc(void) {
    unsigned hi, lo;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
  }
};


// timer based on high-resolution clock
class Timer {

  typedef std::chrono::high_resolution_clock HRClock;
  typedef std::chrono::time_point<HRClock> TP;

  public:
  ///
  Timer(void) { t1 = HRClock::now(); }

  ///
  void reset(void) { t1 = HRClock::now(); }

  ///
  uint64_t timeus(void) {
    Timer::TP t2 = Timer::HRClock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
  }

  private:
    TP t1;
};


// timer based on steady clock
class SteadyTimer {
 public:
 SteadyTimer() : start(clock::now()) {}
  void reset() { start = clock::now(); }
  uint64_t elapsedms() const {
    return std::chrono::duration_cast<milli>
      (clock::now() - start).count(); }
  uint64_t elapsedus() const {
    return std::chrono::duration_cast<micro>
      (clock::now() - start).count(); }

 private:
  typedef std::chrono::steady_clock clock;
  typedef std::chrono::duration<uint64_t, std::milli > milli;
  typedef std::chrono::duration<uint64_t, std::micro > micro;
  std::chrono::time_point<clock> start;
};

#endif // JADAQ_TIMER
