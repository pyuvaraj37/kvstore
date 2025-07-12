#ifndef MEM_MANAGER_HPP
#define MEM_MANAGER_HPP

#include "globals.hpp"

void mem_manager( 
    volatile int *network_ptr,
    int number_of_nodes,
    int board_number,
    int exe,
    hls::stream<LogEntry>& minPropReadBram_req,
    hls::stream<ap_uint<32>>& minPropReadBram_rsp,
    hls::stream<LogEntry>& readSlotsReadBram_req,
    hls::stream<LogEntry>& readSlotsReadBram_rsp,
    hls::stream<ap_uint<32>>& logReadBram_req,
    hls::stream<ap_uint<64>>& logReadBram_rsp,
    hls::stream<ap_uint<32>>& permissibility_req,
    hls::stream<ap_uint<32>>& permissibility_rsp,
    hls::stream<ap_uint<64>>& update_rsp,
    hls::stream<ap_uint<64>>& update_noncon_rsp,
    hls::stream<bool>& throughput_check
);

#endif