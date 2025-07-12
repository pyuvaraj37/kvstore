#ifndef KRNL_HPP
#define KRNL_HPP

#include "globals.hpp"

void kvstore(
    //MU Interface
    hls::stream<ProposedValue>& input,
    hls::stream<ap_uint<256>>& output,
    hls::stream<ap_uint<64>>& remoteUpdate,

    //HT INTERFACE


    //CPU INTERFACE
    ap_uint<64> operation,
    volatile int* HBM_PTR,
    int board_number,
    int number_of_nodes
);


#endif