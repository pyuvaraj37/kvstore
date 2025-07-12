#ifndef KRNL_HPP
#define KRNL_HPP

#include "globals.hpp"

const uint32_t KEY_SIZE = 64;
const uint32_t VALUE_SIZE = 16;

//The hash table can easily support NUM_TABLES-1 * TABLE_SIZE
//for NUM_TABLES = 9 -> this equals to a load factor of 0.88


template <int K>
struct htLookupReq
{
   ap_uint<K>  key;
   ap_uint<1>  source;
   htLookupReq<K>() {}
   htLookupReq<K>(ap_uint<K> key, ap_uint<1> source)
      :key(key), source(source) {}
};

template <int K, int V>
struct htLookupResp
{
   ap_uint<K>  key;
   ap_uint<V>  value;
   bool        hit;
   ap_uint<1>  source;
};

typedef enum {KV_INSERT, KV_DELETE} kvOperation;

template <int K, int V>
struct htUpdateReq
{
   kvOperation op;
   ap_uint<K>  key;
   ap_uint<V>  value;
   ap_uint<1>  source;
   htUpdateReq<K,V>() {}
   htUpdateReq<K,V>(kvOperation op, ap_uint<K> key, ap_uint<V> value, ap_uint<1> source)
      :op(op), key(key), value(value), source(source) {}
};

template <int K, int V>
struct htUpdateResp
{
   kvOperation op;
   ap_uint<K>  key;
   ap_uint<V>  value;
   bool        success;
   ap_uint<1>  source;
};

template <int K, int V>
struct htEntry
{
   ap_uint<K>  key;
   ap_uint<V>  value;
   bool        valid;
   htEntry<K,V>() {}
   htEntry<K,V>(ap_uint<K> key, ap_uint<V> value)
      :key(key), value(value), valid(true) {}
};



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
);


#endif