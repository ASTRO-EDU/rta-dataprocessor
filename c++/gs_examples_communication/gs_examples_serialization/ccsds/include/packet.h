//
//  packet.h
//  Packet
//
//  Created by Alessio Aboudan on 20/05/21.
//

#ifndef __PACKET_H__
#define __PACKET_H__

#include <cstdint>
#include <time.h>
#include <array>


#include "config.h"


#define TC_RX_OK 			0x00
#define TC_RX_ERR_WRONGSTT 	0xFF

#define TC_EX_OK 			0x00
#define TC_EX_ERR_RUPDFAIL 	0xFE
#define TC_EX_ERR_INITFAIL  0xFD

#define WF_SIZE 16384
// Define the HeaderHK struct

struct SerializedTimespec {
    int32_t tv_sec;   // Fixed-size 64-bit for seconds
    int32_t tv_nsec;  // Fixed-size 64-bit for nanoseconds
};

class Header {
public:
    // Header data
    union {
        uint8_t _p8[32];   // 7 * 4 bytes for 7 integers    
        uint16_t _p16[16]; // 7 * 2 2-byte integers
        uint32_t _p32[8];  // 7 integers
        struct __attribute__((packed)) {
            uint32_t apid;
            uint32_t counter;
            uint32_t type;
            alignas(4) SerializedTimespec ts;
            uint32_t abstime;
            uint32_t runID;
            uint32_t configID;
        };
    };

    static void print(const Header& header);
};

////////////////////////////////////
#pragma pack(push, 1)
class HeaderDams {
public:
    static const uint16_t MAX_PACK_SIZE = 4096;
    static const uint16_t MAX_DATA_SIZE = 4084;

    // Constant pattern
    enum {
        START = 0x8D
    };

    // APID
    enum {
        CLASS_TC = 0x00,
        CLASS_TM = 0x80,
        CLASS_MASK = 0x80,
        SOURCE_MASK = 0x7F
    };

    // Sequence
    enum {
        GROUP_CONT = 0x0000,
        GROUP_FIRST = 0x4000,
        GROUP_LAST = 0x8000,
        GROUP_STAND_ALONE = 0xC000,
        GROUP_MASK = 0xC000,
        COUNT_MASK = 0x3FFF
    };

    // Header data
    union {
        uint8_t _p8[12];
        uint16_t _p16[6];
        uint32_t _p32[3];
        struct __attribute__((packed)) {
            uint8_t start;
            uint8_t apid;
            uint16_t sequence;
            uint16_t runID;
            uint16_t size;
            uint32_t crc;
        };
    };
};
#pragma pack(pop)

class Data_HkDams {
public:
    static const uint8_t TYPE = 0x03;
    static const uint8_t SUB_TYPE = 0x01;

    // Header data
    union {
        uint8_t _p8[16];
        uint16_t _p16[8];
        uint32_t _p32[4];
        struct __attribute__((packed)) {
            uint8_t type;
            uint8_t subType;
            uint8_t state;
            uint8_t flags;
            uint32_t waveCount;
            struct timespec ts;
        };
    };
};

class Data_WaveHeader {
public:
    static const uint8_t TYPE = 0xA1;
    static const uint8_t SUB_TYPE = 0x01;

    // Header data
    union {
        uint8_t _p8[44];
        uint16_t _p16[22];
        uint32_t _p32[11];
        struct __attribute__((packed)) {
            // Type/subtype
            uint8_t type;		// 0
            uint8_t subType;
            uint8_t spare0;
            uint8_t spare1;

            // Session and configuration
            uint16_t sessionID;	// 1
            uint16_t configID;

            // Time tagging
            uint8_t timeSts;	// 2
            uint8_t ppsSliceNo;
            uint8_t year;
            uint8_t month;

            uint8_t day;		// 3
            uint8_t hh;
            uint8_t mm;
            uint8_t ss;

            uint32_t us;		// 4

            // Waveform acquisition params
            struct timespec ts;	// 5
            uint32_t dec;		// 7
            uint32_t currOff;	// 8
            uint32_t trigOff;	// 9
            uint32_t size;		// 10
        };
    };
};

class Data_WaveData {
public:
    static const uint8_t TYPE = 0xA1;
    static const uint8_t SUB_TYPE = 0x02;

    // Header data
    union {
        uint8_t _p8[4 * (1 + U32_X_PACKET)];
        uint16_t _p16[2 * (1 + U32_X_PACKET)];
        uint32_t _p32[1 + U32_X_PACKET];
        struct __attribute__((packed)) {
            uint8_t type;
            uint8_t subType;
            uint8_t spare0;
            uint8_t spare1;
            uint32_t buff[U32_X_PACKET];
        };
    };
};

// #pragma pack(push, 1)
class HKPacketDams {
public:
    struct __attribute__((packed)) {
        HeaderDams h;
        Data_HkDams d;
    };
};
// #pragma pack(pop)

class WfPacketDams {
public:
    struct __attribute__((packed)) {
        HeaderDams h;
        Data_WaveHeader w;
        Data_WaveData d;
    };
};
////////////////////////////////////


// Define the Data_Hk struct
class Data_Hk {
public:
    // Header data
    union {
        uint8_t _p8[12];   // 3 * 4 bytes for 3 integers
        uint16_t _p16[6];  // 3 * 2 2-byte integers
        uint32_t _p32[3];  // 3 integers
        struct __attribute__((packed)) {
            uint32_t wformcount;
            uint32_t flags;
            uint32_t crc;
        };
    };

    static void print(const Data_Hk& data);
};

// Define the Data_Wf struct
class Data_Wf {
public:

    // Header data
    union {
        uint8_t _p8[(WF_SIZE+6)*4];  
        uint16_t _p16[(WF_SIZE+6)*2];  
        uint32_t _p32[WF_SIZE+6];  
        struct __attribute__((packed)) {
            int32_t equalizationLevel;
            int32_t decimation;
            int32_t curOffset;
            int32_t trigOff;
            int32_t size;
            std::array<int32_t,16384> data;
            int32_t crc;
        };
    };

    static void print(const Data_Wf& data);
    static void print(const Data_Wf& data, const int limit_print);
};

class HeaderHK {
public:
    struct __attribute__((packed)) {
        Header h;
        Data_Hk d;
    };

    static void print(const HeaderHK& packet);
};

class HeaderWF {
public:
    struct __attribute__((packed)) {
        Header h;
        Data_Wf d;
    };


    static void print(const HeaderWF& packet);
    static void print(const HeaderWF& packet, const int limit_print);
};

SerializedTimespec serializeTimespec(const struct timespec& ts);
struct timespec deserializeTimespec(const SerializedTimespec& sts);


#endif // __PACKET_H__
