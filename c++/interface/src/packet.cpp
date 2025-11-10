//
//  packet.cpp
//  Packet
//
//  Created by Alessio Aboudan on 20/05/21.
//  Adapted for RTA Data Processor interface
//

#include "../include/packet.h"

#include <cstdio>
#include <cstring>
#include <cassert>

// Ref.: https://github.com/gcc-mirror/gcc/blob/master/libiberty/crc32.c
// For more information on CRC, see, e.g., http://www.ross.net/crc/download/crc_v3.txt.

// Table generated with seed 0x05D7B3A1
static const unsigned int crc32Table[] = {
  0x00000000, 0x05D7B3A1, 0x0BAF6742, 0x0E78D4E3,
  0x175ECE84, 0x12897D25, 0x1CF1A9C6, 0x19261A67,
  0x2EBD9D08, 0x2B6A2EA9, 0x2512FA4A, 0x20C549EB,
  0x39E3538C, 0x3C34E02D, 0x324C34CE, 0x379B876F,
  0x5D7B3A10, 0x58AC89B1, 0x56D45D52, 0x5303EEF3,
  0x4A25F494, 0x4FF24735, 0x418A93D6, 0x445D2077,
  0x73C6A718, 0x761114B9, 0x7869C05A, 0x7DBE73FB,
  0x6498699C, 0x614FDA3D, 0x6F370EDE, 0x6AE0BD7F,
  0xBAF67420, 0xBF21C781, 0xB1591362, 0xB48EA0C3,
  0xADA8BAA4, 0xA87F0905, 0xA607DDE6, 0xA3D06E47,
  0x944BE928, 0x919C5A89, 0x9FE48E6A, 0x9A333DCB,
  0x831527AC, 0x86C2940D, 0x88BA40EE, 0x8D6DF34F,
  0xE78D4E30, 0xE25AFD91, 0xEC222972, 0xE9F59AD3,
  0xF0D380B4, 0xF5043315, 0xFB7CE7F6, 0xFEAB5457,
  0xC930D338, 0xCCE76099, 0xC29FB47A, 0xC74807DB,
  0xDE6E1DBC, 0xDBB9AE1D, 0xD5C17AFE, 0xD016C95F,
  0x703B5BE1, 0x75ECE840, 0x7B943CA3, 0x7E438F02,
  0x67659565, 0x62B226C4, 0x6CCAF227, 0x691D4186,
  0x5E86C6E9, 0x5B517548, 0x5529A1AB, 0x50FE120A,
  0x49D8086D, 0x4C0FBBCC, 0x42776F2F, 0x47A0DC8E,
  0x2D4061F1, 0x2897D250, 0x26EF06B3, 0x2338B512,
  0x3A1EAF75, 0x3FC91CD4, 0x31B1C837, 0x34667B96,
  0x03FDFCF9, 0x062A4F58, 0x08529BBB, 0x0D85281A,
  0x14A3327D, 0x117481DC, 0x1F0C553F, 0x1ADBE69E,
  0xCACD2FC1, 0xCF1A9C60, 0xC1624883, 0xC4B5FB22,
  0xDD93E145, 0xD84452E4, 0xD63C8607, 0xD3EB35A6,
  0xE470B2C9, 0xE1A70168, 0xEFDFD58B, 0xEA08662A,
  0xF32E7C4D, 0xF6F9CFEC, 0xF8811B0F, 0xFD56A8AE,
  0x97B615D1, 0x9261A670, 0x9C197293, 0x99CEC132,
  0x80E8DB55, 0x853F68F4, 0x8B47BC17, 0x8E900FB6,
  0xB90B88D9, 0xBCDC3B78, 0xB2A4EF9B, 0xB7735C3A,
  0xAE55465D, 0xAB82F5FC, 0xA5FA211F, 0xA02D92BE,
  0xE076B7C2, 0xE5A10463, 0xEBD9D080, 0xEE0E6321,
  0xF7287946, 0xF2FFCAE7, 0xFC871E04, 0xF950ADA5,
  0xCECB2ACA, 0xCB1C996B, 0xC5644D88, 0xC0B3FE29,
  0xD995E44E, 0xDC4257EF, 0xD23A830C, 0xD7ED30AD,
  0xBD0D8DD2, 0xB8DA3E73, 0xB6A2EA90, 0xB3755931,
  0xAA534356, 0xAF84F0F7, 0xA1FC2414, 0xA42B97B5,
  0x93B010DA, 0x9667A37B, 0x981F7798, 0x9DC8C439,
  0x84EEDE5E, 0x81396DFF, 0x8F41B91C, 0x8A960ABD,
  0x5A80C3E2, 0x5F577043, 0x512FA4A0, 0x54F81701,
  0x4DDE0D66, 0x4809BEC7, 0x46716A24, 0x43A6D985,
  0x743D5EEA, 0x71EAED4B, 0x7F9239A8, 0x7A458A09,
  0x6363906E, 0x66B423CF, 0x68CCF72C, 0x6D1B448D,
  0x07FBF9F2, 0x022C4A53, 0x0C549EB0, 0x09832D11,
  0x10A53776, 0x157284D7, 0x1B0A5034, 0x1EDDE395,
  0x294664FA, 0x2C91D75B, 0x22E903B8, 0x273EB019,
  0x3E18AA7E, 0x3BCF19DF, 0x35B7CD3C, 0x30607E9D,
  0x904DEC23, 0x959A5F82, 0x9BE28B61, 0x9E3538C0,
  0x871322A7, 0x82C49106, 0x8CBC45E5, 0x896BF644,
  0xBEF0712B, 0xBB27C28A, 0xB55F1669, 0xB088A5C8,
  0xA9AEBFAF, 0xAC790C0E, 0xA201D8ED, 0xA7D66B4C,
  0xCD36D633, 0xC8E16592, 0xC699B171, 0xC34E02D0,
  0xDA6818B7, 0xDFBFAB16, 0xD1C77FF5, 0xD410CC54,
  0xE38B4B3B, 0xE65CF89A, 0xE8242C79, 0xEDF39FD8,
  0xF4D585BF, 0xF102361E, 0xFF7AE2FD, 0xFAAD515C,
  0x2ABB9803, 0x2F6C2BA2, 0x2114FF41, 0x24C34CE0,
  0x3DE55687, 0x3832E526, 0x364A31C5, 0x339D8264,
  0x0406050B, 0x01D1B6AA, 0x0FA96249, 0x0A7ED1E8,
  0x1358CB8F, 0x168F782E, 0x18F7ACCD, 0x1D201F6C,
  0x77C0A213, 0x721711B2, 0x7C6FC551, 0x79B876F0,
  0x609E6C97, 0x6549DF36, 0x6B310BD5, 0x6EE6B874,
  0x597D3F1B, 0x5CAA8CBA, 0x52D25859, 0x5705EBF8,
  0x4E23F19F, 0x4BF4423E, 0x458C96DD, 0x405B257C
};

void crc32(unsigned int *crc, const unsigned char *buff, unsigned long len, const unsigned int *table) {
    while (len--) {
        *crc = (*crc << 8) ^ table[((*crc >> 24) ^ *buff) & 255];
        buff++;
    }
}

void HeaderDams::encode() {
    
    // Write the constant start byte
    start = START;
    
    // Compute the crc on the data segment
    crc = 0xFFFFFFFF;
    crc32(_p32 + 2, _p8 + 12, size, crc32Table);
    
}

int HeaderDams::decode() {
    
    if (start == START) {
        uint16_t group = sequence & GROUP_MASK;
        if (group == GROUP_CONT || group == GROUP_FIRST || group == GROUP_LAST || group == GROUP_STAND_ALONE) {
            uint32_t computedCrc = 0xFFFFFFFF;
            if (size) {
                crc32(&computedCrc, _p8 + 12, size, crc32Table);
            }
            if (computedCrc == crc) {
                return 0;
            }
        }
    }
    
    return -1;
    
}

void HeaderDams::print() {
    
    printf("Packet header\n");
           
    printf(" Start: %02X\n", start);
    if (apid & CLASS_TM) {
        printf(" Class: telemetry\n");
    } else {
        printf(" Class: telecommand\n");
    }
    printf("Source: %02X\n", apid & SOURCE_MASK);
    switch (sequence & GROUP_MASK) {
        case GROUP_CONT:
            printf(" Group: continuation\n");
            break;
        case GROUP_FIRST:
            printf(" Group: first\n");
            break;
        case GROUP_LAST:
            printf(" Group: last\n");
            break;
        case GROUP_STAND_ALONE:
            printf(" Group: stand alone\n");
            break;
        default:
            printf(" Group: unknown\n");
    }
    
    printf(" Count: %04X\n", sequence & COUNT_MASK);
    printf("Run ID: %04X\n", runID);
    printf("  Size: %04X\n", size);
    printf("   CRC: %08X\n", crc);
    
}

