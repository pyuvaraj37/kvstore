#include "kvstore.hpp"


void kvstore(
    //MU Interface
    hls::stream<ProposedValue>& input,
    hls::stream<ap_uint<256>>& output,
    hls::stream<ap_uint<64>>& remoteUpdate,

    //HT INTERFACE
    hls::stream<htLookupReq<KEY_SIZE> >&               s_axis_lup_req,
    hls::stream<htUpdateReq<KEY_SIZE,VALUE_SIZE> >&    s_axis_upd_req,
    hls::stream<htLookupResp<KEY_SIZE,VALUE_SIZE> >&   m_axis_lup_rsp,
    hls::stream<htUpdateResp<KEY_SIZE,VALUE_SIZE> >&   m_axis_upd_rsp,

    //CPU INTERFACE
    ap_uint<64> operation,
    int board_number,
    int number_of_nodes
) {

    // PUT
    if (operation.range(1, 0) == 1) {
        //Fill stream to mu
        std::cout << "PUT" << std::endl;
        //Fill hash table stream for Put

    // GET
    }  else {
        //Fill hash table stream for Get
        std::cout << "GET" << std::endl;
    }

}