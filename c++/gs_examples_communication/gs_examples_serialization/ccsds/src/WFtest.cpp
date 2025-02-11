#include "tchandler.h"
#include <queue>
#include "generators.hh"
#include <iostream>

int main() {

    size_t total_size = sizeof(HeaderWF);
    std::cout << "Total memory used by packet: " 
            << total_size << " bytes" << std::endl;

    std::queue<std::vector<uint8_t>> serializedQueue;

    TcHandler<HeaderWF> handler(serializedQueue);

    WFGenerator generator(1);
    auto packet = generator.get();
    // Bufferize the generated structures
    handler.bufferize(packet);

    // Variables to hold debufferized data
    HeaderWF receivedPacket;

    // Debufferize the content back into structures
    handler.debufferize(receivedPacket);

    // Verify the content of the debufferized data
    std::cout << "Debufferized Header APID: " << receivedPacket.h.apid << std::endl;
    std::cout << "Debufferized Data size: " << receivedPacket.d.size << std::endl;

    
    std::cout << "size of timespec: " << sizeof(packet.h.ts) << ". alignment:" << alignof(packet.h.ts) << std::endl;
    
    HeaderWF::print(receivedPacket, 10);

    return 0;
}
