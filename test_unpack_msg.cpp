#include <iostream>
#include <cstring>
#include <cstdlib>
#include "include/raimd/md_msg.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <hex_string>" << std::endl;
        return 1;
    }

    // Convert the hex string to a byte array
    size_t len = strlen(argv[1]);
    if (len % 2 != 0) {
        std::cerr << "Invalid hex string length" << std::endl;
        return 1;
    }

    size_t byteArrayLen = len / 2;
    unsigned char* byteArray = new unsigned char[byteArrayLen];
    for (size_t i = 0; i < byteArrayLen; ++i) {
        sscanf(argv[1] + 2*i, "%2hhx", &byteArray[i]);
    }

    // Create a memory manager
    rai::md::MDMsgMem mem;

    // Unpack the message
    rai::md::MDMsg* msg = rai::md::MDMsg::unpack(
        byteArray, 0, byteArrayLen, 0, nullptr, mem
    );

    if (!msg) {
        std::cerr << "Failed to unpack message" << std::endl;
        delete[] byteArray;
        return 1;
    }

    // Create an output object
    rai::md::MDOutput out;

    // Print the unpacked message
    out.printf("Unpacked message:\n");
    msg->print(&out);

    // Clean up
    delete[] byteArray;
    delete msg;
    return 0;
}
