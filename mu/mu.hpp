#ifndef MU_HPP
#define MU_HPP

#include "globals.hpp"
#include "smr.hpp"
#include "mem_manager.hpp"

void mu(
    hls::stream<pkt256>& smr_tx_meta,
    hls::stream<pkt64>& smr_tx_data,
    hls::stream<ProposedValue>& input,
    hls::stream<ap_uint<256>> output,
    hls::stream<ap_uint<64>> remoteUpdate,
    int* HBM_PTR,
    int board_number,
    int number_of_nodes
);

#endif 