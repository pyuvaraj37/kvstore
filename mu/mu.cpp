#include "mu.hpp"

void mu(
    hls::stream<pkt256>& smr_tx_meta,
    hls::stream<pkt64>& smr_tx_data,
    hls::stream<ProposedValue>& input,
    hls::stream<ap_uint<256>>& output,
    hls::stream<ap_uint<64>>& remoteUpdate,
    int* HBM_PTR,
    int board_number,
    int number_of_nodes
) {


    hls::stream<ap_uint<32>> logReadBram_req;
    hls::stream<ap_uint<64>> logReadBram_rsp;
    hls::stream<LogEntry> readSlotsReadBram_req;
    hls::stream<LogEntry> readSlotsReadBram_rsp;
    hls::stream<LogEntry> minPropReadBram_req;
    hls::stream<ap_uint<32>> minPropReadBram_rsp;


    smr(
        output,
        input,
        smr_tx_meta,
        smr_tx_data,
        logReadBram_req,
        logReadBram_rsp,
        readSlotsReadBram_req,
        readSlotsReadBram_rsp,
        minPropReadBram_req,
        minPropReadBram_rsp,
        board_number,
        number_of_nodes
    );

    mem_manager(
        HBM_PTR,
        number_of_nodes,
        board_number,
        minPropReadBram_req,
        minPropReadBram_rsp,
        readSlotsReadBram_req,
        readSlotsReadBram_rsp,
        logReadBram_req,
        logReadBram_rsp,
        remoteUpdate
    );


}