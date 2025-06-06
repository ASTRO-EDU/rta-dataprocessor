//
//  packet.cpp
//  Packet
//
//  Created by Alessio Aboudan on 20/05/21.
//

#include "packet.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <iostream>

void Header::print(const Header& header) {
    std::cout << "Header:" << std::endl;
    std::cout << "  APID: " << header.apid << std::endl;
    std::cout << "  Counter: " << header.counter << std::endl;
    std::cout << "  Type: " << header.type << std::endl;
    std::cout << "  Timestamp: " << header.ts.tv_sec << "s " << header.ts.tv_nsec << "ns" << std::endl;
    std::cout << "  Absolute Time: " << header.abstime << std::endl;
    std::cout << "  Run ID: " << header.runID << std::endl;
    std::cout << "  Config ID: " << header.configID << std::endl;
}

void Data_Hk::print(const Data_Hk& data) {
    std::cout << "Data_Hk:" << std::endl;
    std::cout << "  Waveform Count: " << data.wformcount << std::endl;
    std::cout << "  Flags: 0x" << std::hex << data.flags << std::dec << std::endl;
    std::cout << "  CRC: 0x" << std::hex << data.crc << std::dec << std::endl;
}

void Data_HkDams::print() {
    printf("Packet Data_HkDams\n");

    printf("  Type      : %02X\n", type);
    printf("  SubType   : %02X\n", subType);

    printf("  State     : %02X\n", state);
    printf("  Flags     : %02X\n", flags);

    printf("  WaveCount : %u\n", waveCount);

    printf("  Timestamp : %ld.%09ld seconds (tv_sec.tv_nsec)\n", ts.tv_sec, ts.tv_nsec);
}


void Data_Wf::print(const Data_Wf& data) {
    Data_Wf::print(data, 0);
}


// Limit print of data to first n samples, 0 prints all 
void Data_Wf::print(const Data_Wf& data, const int limit_print){
    std::cout << "Data_Wf:" << std::endl;
    std::cout << "  Equalization Level: " << data.equalizationLevel << std::endl;
    std::cout << "  Decimation: " << data.decimation << std::endl;
    std::cout << "  Current Offset: " << data.curOffset << std::endl;
    std::cout << "  Trigger Offset: " << data.trigOff << std::endl;
    std::cout << "  Size: " << data.size << std::endl;
    
    size_t end;
    std::string end_data ;
    if(limit_print > 0){
        end = limit_print;
        end_data = ", ...";
    }
    else {
        end = data.data.size();
        end_data = "";
    }
    std::cout << "  Data: [";
    for (size_t i = 0; i < end; ++i) {  // Print only the first 10 elements for brevity
        std::cout << data.data[i];
        if (i < end-1) {
            std::cout << ", ";
        }
    }
    std::cout << end_data << "]" << std::endl; // Indicating there are more elements not shown
    
    std::cout << "  CRC: 0x" << std::hex << data.crc << std::dec << std::endl;
}

void HeaderHK::print(const HeaderHK& p) {
    Header::print(p.h);
    Data_Hk::print(p.d);
}

void HeaderWF::print(const HeaderWF& p) {
    Header::print(p.h);
    Data_Wf::print(p.d);
}


void HeaderWF::print(const HeaderWF& p, const int limit_print) {
    Header::print(p.h);
    Data_Wf::print(p.d, limit_print);
}

//helpers
SerializedTimespec serializeTimespec(const struct timespec& ts) {
    SerializedTimespec sts;
    sts.tv_sec = static_cast<int32_t>(ts.tv_sec);   // Cast to 32-bit
    sts.tv_nsec = static_cast<int32_t>(ts.tv_nsec); // Cast to 32-bit
    return sts;
}

struct timespec deserializeTimespec(const SerializedTimespec& sts) {
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(sts.tv_sec);   // Cast back to `time_t`
    ts.tv_nsec = static_cast<long>(sts.tv_nsec);   // Cast back to `long`
    return ts;
}

