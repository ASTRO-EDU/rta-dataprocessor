#ifndef HK_UTILS_HH
#define HK_UTILS_HH

#include "HKSchema.hh"
#include "WFSchema.hh"
using namespace HK;
using namespace WF;

void print_HK(HeaderHK h);

void print_WF(const HeaderandWaveform& h);

void print_WF(const HeaderandWaveform& h, const int limit_print);

bool areEqual(const HK::TimeSpec& lhs, const HK::TimeSpec& rhs);

bool areEqual(const WF::TimeSpec& lhs, const WF::TimeSpec& rhs);

bool areEqual(const HeaderHK& lhs, const HeaderHK& rhs);

bool areEqual(const HeaderandWaveform& lhs, const HeaderandWaveform& rhs);

#endif // HK_UTILS_HH