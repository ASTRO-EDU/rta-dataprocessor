#include "WF.pb.h"
#include "HK.pb.h"

void print_HK(HeaderHK h) {
    std::cout << "abstime: " << h.abstime() << std::endl;
    std::cout << "apid: " << h.apid() << std::endl;
    std::cout << "configID: " << h.configid() << std::endl;
    std::cout << "counter: " << h.counter() << std::endl;
    std::cout << "CRC: 0x" << std::hex << h.crc() << std::dec << std::endl;
    std::cout << "Flags: 0x" << std::hex << h.flags() << std::dec << std::endl;
    std::cout << "runID: " << h.runid() << std::endl;
    std::cout << "time s: " << h.time().tv_sec() << std::endl;
    std::cout << "time ns: " << h.time().tv_nsec() << std::endl;
    std::cout << "type: " << h.type() << std::endl;
    std::cout << "wformcount: " << h.wformcount() << std::endl;
}


//limit print of data to first n samples, 0 prints all 
void print_WF(const HeaderandWaveform& h, const int limit_print) {
    std::cout << "abstime: " << h.abstime() << std::endl;
    std::cout << "apid: " << h.apid() << std::endl;
    std::cout << "configID: " << h.configid() << std::endl;
    std::cout << "counter: " << h.counter() << std::endl;
    std::cout << "runID: " << h.runid() << std::endl;
    std::cout << "time s: " << h.time().tv_sec() << std::endl;
    std::cout << "time ns: " << h.time().tv_nsec() << std::endl;
    std::cout << "type: " << h.type() << std::endl;
    std::cout << "equalizationLevel: " << h.equalizationlevel() << std::endl;
    std::cout << "decimation: " << h.decimation() << std::endl;
    std::cout << "curOffset: " << h.curoffset() << std::endl;
    std::cout << "trigOff: " << h.trigoff() << std::endl;
    std::cout << "size: " << h.size() << std::endl;

    size_t end;
    std::string end_data ;
    if(limit_print > 0){
        end = limit_print;
        end_data = ", ...";
    }
    else {
        end = h.data().size();
        end_data = "";
    }
    std::cout << "  Data: [";
    for (size_t i = 0; i < end; ++i) {  // Print only the first 10 elements for brevity
        std::cout << h.data().Get(i);
        if (i < end-1) {
            std::cout << ", ";
        }
    }
    std::cout << end_data << "]" << std::endl; // Indicating there are more elements not shown

    std::cout << "CRC: 0x" << std::hex << h.crc() << std::dec << std::endl;

    std::cout << std::endl;
}

// Function to print HeaderandWaveform
void print_WF(const HeaderandWaveform& h){
    print_WF(h, 0);
}

// // Function to compare TimeSpec instances
// bool areEqual(const HK::TimeSpec& lhs, const HK::TimeSpec& rhs) {
//     return (lhs.tv_sec == rhs.tv_sec) && (lhs.tv_nsec == rhs.tv_nsec);
// }

// bool areEqual(const WF::TimeSpec& lhs, const WF::TimeSpec& rhs) {
//     return (lhs.tv_sec == rhs.tv_sec) && (lhs.tv_nsec == rhs.tv_nsec);
// }

// // Function to compare HeaderHK instances
// bool areEqual(const HeaderHK& lhs, const HeaderHK& rhs) {
//     return (lhs.apid == rhs.apid) &&
//            (lhs.counter == rhs.counter) &&
//            (lhs.type == rhs.type) &&
//            areEqual(lhs.time, rhs.time) &&
//            (lhs.abstime == rhs.abstime) &&
//            (lhs.runID == rhs.runID) &&
//            (lhs.configID == rhs.configID) &&
//            (lhs.wformcount == rhs.wformcount) &&
//            (lhs.flags == rhs.flags) &&
//            (lhs.crc == rhs.crc);
// }

// bool areEqual(const HeaderandWaveform& lhs, const HeaderandWaveform& rhs) {
//     return (lhs.apid == rhs.apid) &&
//            (lhs.counter == rhs.counter) &&
//            (lhs.type == rhs.type) &&
//            areEqual(lhs.time, rhs.time) &&
//            (lhs.abstime == rhs.abstime) &&
//            (lhs.runID == rhs.runID) &&
//            (lhs.configID == rhs.configID) &&
//            (lhs.equalizationLevel == rhs.equalizationLevel) &&
//            (lhs.decimation == rhs.decimation) &&
//            (lhs.curOffset == rhs.curOffset) &&
//            (lhs.trigOff == rhs.trigOff) &&
//            (lhs.size == rhs.size) &&
//            (lhs.data == rhs.data) &&  // Compares the data vectors
//            (lhs.crc == rhs.crc);
// }