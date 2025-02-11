#include <time.h>
#include "TimeSpecGen.hh"  // Include your generated header file for protobuf messages

// Function to generate a HeaderHK message
TimeSpec* generate_time() {    
    TimeSpec* message = new TimeSpec;
    timespec ts ;
    clock_gettime(CLOCK_REALTIME , &ts);
    message->set_tv_nsec(ts.tv_nsec);
    message->set_tv_sec(ts.tv_sec);

    return message;
}
