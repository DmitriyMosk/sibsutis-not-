
#ifndef _LIB_BENCH 
#define _LIB_BENCH 

#include <chrono> 
#include <vector> 
#include <string> 
#include <iostream> 
#include <sstream> 
#include <iomanip> 

class Bench { 
public:
    using Clock     =   std::chrono::high_resolution_clock  ; 
    using TimePoint =   std::chrono::time_point<Clock>      ;
    using Duration  =   std::chrono::duration<double>       ; 

    Bench   (const std::string& name = "Benchmark")         ; 
    ~Bench  ()                                              ; 

    void start  ()                                          ; 
    void stop   ()                                          ; 
    void reset  ()                                          ; 
}; 

#endif  