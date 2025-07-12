#include "kvstore.hpp"

int main() {

    // MU Interface ports
    hls::stream<ProposedValue> input;
    hls::stream<ap_uint<256>> output;
    hls::stream<ap_uint<64>> remoteUpdate;

    //HT Interface ports
    hls::stream<htLookupReq<KEY_SIZE> >              s_axis_lup_req;
    hls::stream<htUpdateReq<KEY_SIZE,VALUE_SIZE> >    s_axis_upd_req;
    hls::stream<htLookupResp<KEY_SIZE,VALUE_SIZE> >   m_axis_lup_rsp;
    hls::stream<htUpdateResp<KEY_SIZE,VALUE_SIZE> >   m_axis_upd_rsp;

    //CPU INTERFACE
    ap_uint<64> operation;
    int board_number;
    int number_of_nodes;


    /*
        So this is our testbed for the kvstore. The kvstore should take an operations which is a 65 bit unsigned integer and then either do a Insert, or Query. 

        operation[1,0] or the first bit of the integer tells us if it's an insert (1) or query (0). 
        The rest of the bits are used for Key and Value:
        |________key___________|________value_______|.op|
        |.........32...........|.........31.........|.1.|     
    
    */


    /* This calls the module if you run csim it will run as a C program, if you run cosim it will run in RTL simulation*/
    kvstore(
        input,
        output,
        remoteUpdate,
        s_axis_lup_req,
        s_axis_upd_req,
        m_axis_lup_rsp,
        m_axis_upd_rsp,
        operation,
        board_number,
        number_of_nodes
    );

    /* Here is an example of how to read from a stream in HLS*/
    if (!input.empty()) {
        ProposedValue p = input.read();
    }

    /* Here is an example of how to write to a stream in HLS*/
    if (!output.full()) {
        output.write(10);
    }


    return 0;
}