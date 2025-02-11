#include "tchandler.h"
#include "generators.hh"
#include <iostream>
#include <queue>

constexpr size_t buffSz = sizeof(HeaderHK);

int main() {

    size_t total_size = sizeof(HeaderHK);
    std::cout << "Total memory used by packet: " 
            << total_size << " bytes" << std::endl;

    std::queue<std::vector<uint8_t>> serializedQueue;

    TcHandler<HeaderHK> handler(serializedQueue);

    HKGenerator generator(1);

    HeaderHK genval = generator.get();

    // Bufferize the generated structures
    handler.bufferize(genval);

    // Variables to hold debufferized data
    HeaderHK debufferizedPack;

    // Debufferize the content back into structures
    handler.debufferize(debufferizedPack);

    // Verify the content of the debufferized data
    std::cout << "Debufferized Header APID: " << debufferizedPack.h.apid << std::endl;
    std::cout << "Debufferized Data Wformcount: " << debufferizedPack.d.wformcount << std::endl;

    HeaderHK::print(debufferizedPack);

    return 0;
}
