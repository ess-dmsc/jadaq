
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
