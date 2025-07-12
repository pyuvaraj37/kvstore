
#include <hls_stream.h>
#include <iostream>
#include <stdlib.h>
#include <ap_int.h>
#include <ap_fixed.h>
#include "ap_axi_sdata.h"
#include <ap_fixed.h>
#include "ap_int.h" 
#include "hls_stream.h"

#pragma once

#define DWIDTH512 512
#define DWIDTH256 256
#define DWIDTH128 128
#define DWIDTH64 64
#define DWIDTH32 32
#define DWIDTH16 16
#define DWIDTH8 8

typedef ap_axiu<DWIDTH512, 0, 0, 0> pkt512;
typedef ap_axiu<DWIDTH256, 0, 0, 0> pkt256;
typedef ap_axiu<DWIDTH128, 0, 0, 0> pkt128;
typedef ap_axiu<DWIDTH64, 0, 0, 0> pkt64;
typedef ap_axiu<DWIDTH32, 0, 0, 0> pkt32;
typedef ap_axiu<DWIDTH16, 0, 0, 0> pkt16;
typedef ap_axiu<DWIDTH8, 0, 0, 0> pkt8;


const int NUM_NODES = 12; 
const int SYNC_GROUPS = 1; 

// Need for QP info
const ap_uint<32> BASE_IP_ADDR = 0xe0d4010b;
const uint32_t UDP = 0x000012b7;

// Written by Leader Switch and read by Log Handler
static bool FOLLOWER_LIST[NUM_NODES-1] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

//25% write log size (smaller % can use this as well)
const int NUM_SLOTS = 125000 * 2; 
const int FIFO_LENGTH = 5;

// Constants for HeartBeat Memory
// Only 1 HB regardless of number of sync groups
const int HB_BASE_PTR = 0;
const int HB_BASE_ADDR = 0;
const int HB_PTR_LEN = NUM_NODES; 
const int HB_ADDR_LEN = 4 * HB_PTR_LEN; 

// Constants for replication logs
// Scales with numberof sync groups
const int LOG_BASE_PTR = HB_PTR_LEN; 
const int LOG_BASE_ADDR = HB_ADDR_LEN; 
const int LOG_MIN_PROP_PTR_LEN = 2 + (NUM_NODES-1) * FIFO_LENGTH; // local heartbeat and remote heartbeat queue
const int LOG_MIN_PROP_ADDR_LEN = 4 * LOG_MIN_PROP_PTR_LEN; 
const int LOG_LOCAL_LOG_PTR_LEN = NUM_SLOTS; // local log 
const int LOG_LOCAL_LOG_ADDR_LEN = 4 * LOG_LOCAL_LOG_PTR_LEN; 
const int LOG_REMOTE_LOG_QUEUE_PTR_LEN = 2 * (NUM_NODES-1) * FIFO_LENGTH;
const int LOG_REMOTE_LOG_QUEUE_ADDR_LEN = 4 * LOG_REMOTE_LOG_QUEUE_PTR_LEN; 
const int LOG_PTR_LEN = LOG_MIN_PROP_PTR_LEN + LOG_LOCAL_LOG_PTR_LEN + LOG_REMOTE_LOG_QUEUE_PTR_LEN; 
const int LOG_ADDR_LEN = LOG_PTR_LEN * 4; 

const int DEPOSIT_PTR = HB_PTR_LEN + SYNC_GROUPS * LOG_PTR_LEN;
const int DEPOSIT_ADDR = HB_ADDR_LEN + SYNC_GROUPS * LOG_ADDR_LEN;
const int DEPOSIT_LEN = NUM_NODES; 


struct LocalMemOp {
    bool read; 
    ap_uint<32> index;
    ap_uint<32> value; 
    LocalMemOp()
        :read(false) {}
    LocalMemOp(bool r, ap_uint<32> i, ap_uint<32> v)
        :read(r), index(i), value(v) {}
};

struct ProposedValue {
    ap_uint<32> value; 
    ap_uint<32> syncronizationGroup;
    ProposedValue()
        :value(0), syncronizationGroup(0){}
    ProposedValue(int v, ap_uint<32> s)
        : value(v), syncronizationGroup(s) {}
};

struct updateLocalValue {
    ap_uint<32> value; 
    ap_uint<32> syncGroup;
    updateLocalValue()
        :value(0), syncGroup(0) {}
    updateLocalValue(ap_uint<32> v, ap_uint<32> s)
        :value(v), syncGroup(s) {} 
};

struct LogEntry
{
    ap_uint<32> propVal;
    ap_uint<32> value;
	ap_uint<32> fuo;
	ap_uint<32> syncGroup; 
	bool valid; 
    LogEntry()
        :valid(false) {}
    LogEntry(ap_uint<32> p, ap_uint<32> f)
        :propVal(p), fuo(f) {}
    LogEntry(ap_uint<32> s)
        :valid(false), syncGroup(s) {}
    LogEntry(ap_uint<32> p, ap_uint<32> v, ap_uint<32> s) 
        :propVal(p), value(v), syncGroup(s) {}
    LogEntry(ap_uint<32> p, ap_uint<32> v, bool va) 
        :propVal(p), value(v), valid(va) {}
};


void bank(
    hls::stream<pkt256>& m_axis_tx_meta,
    hls::stream<pkt64>& m_axis_tx_data,
    hls::stream<ap_uint<32>>&  s_axis_summary,
    int board_number,
    hls::stream<ap_uint<64>>& operation_stream,
    int number_of_operations,
    int number_of_nodes,
    int number_of_deposits,
    int throughput_check_value,
    volatile int* HBM_PTR,
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

void hybrid_account_stream_krnl(
    hls::stream<pkt256>& m_axis_tx_meta,
    hls::stream<pkt64>& m_axis_tx_data,
    hls::stream<pkt64>& s_axis_tx_status,
    hls::stream<ap_uint<32>>& s_axis_summary,
    ap_uint<64>* operation_list,
    volatile int* HBM_PTR,
    int board_number,
    int number_of_operations,
    int number_of_nodes,
    int throughput_check_value,
    int number_of_deposits
);