#ifndef HK_UTILS_HH
#define HK_UTILS_HH

#include "WF.pb.h"
#include "HK.pb.h"


void print_HK(HeaderHK h);

void print_WF(const HeaderandWaveform& h);

void print_WF(const HeaderandWaveform& h, const int limit_print);

bool areEqual(const TimeSpec& lhs, const TimeSpec& rhs);

bool areEqual(const TimeSpec& lhs, const TimeSpec& rhs);

bool areEqual(const HeaderHK& lhs, const HeaderHK& rhs);

bool areEqual(const HeaderandWaveform& lhs, const HeaderandWaveform& rhs);

#endif // HK_UTILS_HH