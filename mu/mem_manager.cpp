#include "mem_manager.hpp"


void mem_manager( 
    volatile int *network_ptr,
    int number_of_nodes,
    int board_number,
    hls::stream<LogEntry>& minPropReadBram_req,
    hls::stream<ap_uint<32>>& minPropReadBram_rsp,
    hls::stream<LogEntry>& readSlotsReadBram_req,
    hls::stream<LogEntry>& readSlotsReadBram_rsp,
    hls::stream<ap_uint<32>>& logReadBram_req,
    hls::stream<ap_uint<64>>& logReadBram_rsp,
    hls::stream<ap_uint<64>>& update_rsp,
){
    #pragma HLS PIPELINE
    #pragma HLS INTERFACE axis port = minPropReadBram_req
    #pragma HLS INTERFACE axis port = minPropReadBram_rsp
    #pragma HLS INTERFACE axis port = readSlotsReadBram_req
    #pragma HLS INTERFACE axis port = readSlotsReadBram_rsp
    #pragma HLS INTERFACE axis port = logReadBram_req
    #pragma HLS INTERFACE axis port = logReadBram_rsp
    #pragma HLS INTERFACE axis port = update_rsp

    static ap_uint<512> internal_clock=0;
    static int hbm_tmp=0;
    static int remote_bank_accounts = 0;
    static LogEntry slotIndex, minPropIndex;
    static ap_uint<32> slotRead, psig, ptemp;
    static int log_index[NUM_NODES];
    volatile bool sig = false;
    static ap_uint<64>  s_sig = 0; 
    static int minProp = 0;
    static ap_uint<64> maxPropNumber = 0, update;
    static int propNum, propValue;
    static bool check_throughput_counter = false;
    static bool read_slot = true; 

    static int FUO[SYNC_GROUPS];
    static bool running = true; 

    if (!minPropReadBram_req.empty() && !minPropReadBram_rsp.full()) {

        minPropReadBram_req.read(minPropIndex);

        VITIS_LOOP_622_2: for (int i = 0; i < number_of_nodes-1; i++) {
            int temp = network_ptr[LOG_BASE_PTR + (LOG_PTR_LEN * minPropIndex.syncGroup) + 2 + FIFO_LENGTH * i + (minPropIndex.value%FIFO_LENGTH)];
            if (temp > minProp) {
                minProp = temp;
            }
        }
        minPropReadBram_rsp.write(minProp);

    }

    if (!readSlotsReadBram_req.empty() && !readSlotsReadBram_rsp.full()) {

        readSlotsReadBram_req.read(slotIndex);
        if (read_slot) {
            maxPropNumber = 0;
            VITIS_LOOP_636_3: for (int i = 0; i < number_of_nodes-1; i++) {
                if (FOLLOWER_LIST[i]) {
                    propNum = network_ptr[LOG_BASE_PTR + (LOG_PTR_LEN * slotIndex.syncGroup) + LOG_MIN_PROP_PTR_LEN + LOG_LOCAL_LOG_PTR_LEN + (2 * i * FIFO_LENGTH) + (slotIndex.value%NUM_SLOTS)];
                    propValue = network_ptr[LOG_BASE_PTR + (LOG_PTR_LEN * slotIndex.syncGroup) + LOG_MIN_PROP_PTR_LEN + LOG_LOCAL_LOG_PTR_LEN + (2 * i * FIFO_LENGTH) + (slotIndex.value%NUM_SLOTS) + 1];
                    if (propNum != 0) {
                        maxPropNumber.range(31, 0) = propNum;
                        maxPropNumber.range(64, 32) = propValue;
                    }
                }
            }

            if (maxPropNumber.range(31,0) != 0) {
                readSlotsReadBram_rsp.write(LogEntry(maxPropNumber.range(31,0), maxPropNumber.range(63, 32), true));
            } else {
                read_slot = false; 
                readSlotsReadBram_rsp.write(LogEntry(slotIndex.propVal, 0));
            }
        } else {
            std::cout << "Value in MEM MNGER: " << slotIndex.propVal<< std::endl; 
            readSlotsReadBram_rsp.write(LogEntry(slotIndex.propVal, 0, false));
        }

    }
    
    if (!update_rsp.full()) {
        for (int i = 0; i < SYNC_GROUPS; i++) {
            int index = LOG_BASE_PTR + LOG_PTR_LEN * i + FUO[i]; 
            int log_proposal_number = network_ptr[LOG_BASE_PTR + LOG_PTR_LEN * i + LOG_MIN_PROP_PTR_LEN + FUO[i]];
            int log_operation = network_ptr[LOG_BASE_PTR + LOG_PTR_LEN * i + LOG_MIN_PROP_PTR_LEN + FUO[i] + 1];
            if (log_proposal_number != 0) {
                std::cout << "Log change found! Prop: " << log_proposal_number << " Operation: " << log_operation << std::endl; 
                update = log_operation;
                update_rsp.write(update);
                FUO[i]+=2; 
            }
        }
    }
            // internal_clock = 0; 
        //}

}