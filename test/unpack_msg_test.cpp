#include <iostream>
#include <cstdlib>
#include <cstring>
#include "raimd/md_msg.h"
#include "raimd/md_output.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <message>" << std::endl;
        return 1;
    }

    const char *msg = argv[1];
    size_t msg_len = std::strlen(msg);

    // Allocate memory for the message
    void *msg_buf = std::malloc(msg_len);
    if (!msg_buf) {
        std::cerr << "Failed to allocate memory for message" << std::endl;
        return 1;
    }

    // Copy the message into the allocated buffer
    std::memcpy(msg_buf, msg, msg_len);

    // Create a dictionary (assuming a default dictionary is needed)
    MDDict *dict = nullptr; // You might need to initialize this properly

    // Create a message memory object
    MDMsgMem mem;

    // Create an MDMsg object
    MDMsg md_msg(msg_buf, 0, msg_len, dict, mem);

    // Unpack the message
    if (md_msg.unpack() != 0) {
        std::cerr << "Failed to unpack message" << std::endl;
        std::free(msg_buf);
        return 1;
    }

    // Create an MDOutput object for printing
    MDOutput output;

    // Print the message
    md_msg.print_hex(&output);

    // Clean up
    md_msg.release();
    std::free(msg_buf);

    return 0;
}
