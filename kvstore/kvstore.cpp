#include "kvstore.hpp"


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
    int number_of_nodes,
) {

    // PUT
    if (operation.range(1, 0)) {
        //Fill stream to mu

        //Fill hash table stream for Put

    // GET
    }  else {
        //Fill hash table stream for Get

    }

}