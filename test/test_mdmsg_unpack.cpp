#include <iostream>
#include <cstdlib>
#include <cstring>
#include "raimd/md_msg.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <message>" << std::endl;
        return 1;
    }

    const char *msg = argv[1];
    size_t msg_len = std::strlen(msg);

    // Assuming we have a dictionary and memory management set up
    MDDict *dict = nullptr; // This should be properly initialized
    MDMsgMem mem; // This should be properly initialized

    MDMsg msg_obj(reinterpret_cast<void*>(const_cast<char*>(msg)), 0, msg_len, dict, mem);

    if (msg_obj.unpack() != 0) {
        std::cerr << "Failed to unpack message" << std::endl;
        return 1;
    }

    // Print the message
    MDOutput output;
    msg_obj.print_hex(&output, msg_obj.msg_buf, msg_obj.msg_end - msg_obj.msg_off);

    return 0;
}
