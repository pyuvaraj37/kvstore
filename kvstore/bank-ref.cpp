#include "krnl.hpp"

#define TH 0

const int BUFFER_SIZE = 16;
const int SWAP_SIZE = BUFFER_SIZE/2;


void stream_2_to_1(
    hls::stream<ap_uint<256>>& a_tx_meta,
    hls::stream<ap_uint<256>>& b_tx_meta,
    hls::stream<ap_uint<64>>& a_tx_data,
    hls::stream<ap_uint<64>>& b_tx_data,
    hls::stream<ap_uint<256>>& c_tx_meta,
    hls::stream<ap_uint<64>>& c_tx_data
) {

    static ap_uint<256> temp_val_256; 
    static ap_uint<64> temp_val_64; 

    if (!a_tx_meta.empty()) {
        a_tx_meta.read(temp_val_256);
        c_tx_meta.write(temp_val_256);
    } else if (!b_tx_meta.empty()) {
        b_tx_meta.read(temp_val_256);
        c_tx_meta.write(temp_val_256);
    }

    if (!a_tx_data.empty()) {
        a_tx_data.read(temp_val_64);
        c_tx_data.write(temp_val_64);
    } else if (!b_tx_data.empty()) {
        b_tx_data.read(temp_val_64);
        c_tx_data.write(temp_val_64);
    }

}


void meta_merger(
    hls::stream<ap_uint<256>>& a_tx_meta,
    hls::stream<ap_uint<256>>& b_tx_meta,
    //hls::stream<ap_uint<64>>& a_tx_data,
    //hls::stream<ap_uint<64>>& b_tx_data,
    hls::stream<pkt256>& d_tx_meta
    //hls::stream<pkt64>& c_tx_data
) {
    #pragma HLS inline off
    #pragma HLS pipeline II=1

    static ap_uint<256> temp_val_256; 
    static pkt256 temp_pkt_256; 

    while (!a_tx_meta.empty()) {
        a_tx_meta.read(temp_val_256);
        temp_pkt_256.data(255, 0) = temp_val_256.range(255, 0); 
        d_tx_meta.write(temp_pkt_256);
    } 
    while (!b_tx_meta.empty()) {
        b_tx_meta.read(temp_val_256);
        temp_pkt_256.data(255, 0) = temp_val_256.range(255, 0); 
        d_tx_meta.write(temp_pkt_256);  
    } 

}

void data_merger(
    hls::stream<ap_uint<64>>& a_tx_data,
    hls::stream<ap_uint<64>>& b_tx_data,
    hls::stream<pkt64>& d_tx_data
) {

    #pragma HLS inline off
    #pragma HLS pipeline II=1
    static ap_uint<64> temp_val_64; 
    static pkt64 temp_pkt_64; 

    while (!a_tx_data.empty()) {
        a_tx_data.read(temp_val_64);
        temp_pkt_64.data(63, 0) = temp_val_64.range(63, 0); 
        temp_pkt_64.keep(7, 0) = 0xff;
        temp_pkt_64.last = 1; 
        d_tx_data.write(temp_pkt_64);
    } 
    while (!b_tx_data.empty()) {
        b_tx_data.read(temp_val_64);
        temp_pkt_64.data(63, 0) = temp_val_64.range(63, 0); 
        temp_pkt_64.keep(7, 0) = 0xff;
        temp_pkt_64.last = 1; 
        d_tx_data.write(temp_pkt_64);       
    } 

}


void rdma_read(
    int s_axi_lqpn,
    ap_uint<64> s_axi_laddr,
    ap_uint<64> s_axi_raddr,
    int s_axi_len,
    hls::stream<ap_uint<256>>& m_axis_tx_meta
){
    //#pragma HLS dataflow
    //#pragma HLS inline off
    #pragma HLS pipeline II=1
    #pragma HLS INTERFACE axis port = m_axis_tx_meta
    
    ap_uint<256> tx_meta;
    ap_uint<64> tx_data;

    /*RDMA OP*/
    tx_meta.range(2,0) = 0x00000000; 
    /*lQPN*/
    tx_meta.range(26,3) = s_axi_lqpn; 
    /*
    lAddr
    */
    tx_meta.range(74, 27) = s_axi_laddr; 
    /*rAddr*/
    tx_meta.range(122, 75) = s_axi_raddr; 
    //+(itt*4)
    /*len*/
    tx_meta.range(154, 123) = s_axi_len;
    m_axis_tx_meta.write(tx_meta);
    
}

void rdma_write(
    int s_axi_lqpn,
    ap_uint<64> s_axi_laddr,
    ap_uint<64> s_axi_raddr,
    int s_axi_len,
    ap_uint<64>  write_value,
    hls::stream<ap_uint<256>>& m_axis_tx_meta, 
    hls::stream<ap_uint<64>>& m_axis_tx_data
){
    //#pragma HLS dataflow
    //#pragma HLS inline off
    #pragma HLS pipeline II=1
    #pragma HLS INTERFACE axis port = m_axis_tx_meta
    #pragma HLS INTERFACE axis port = m_axis_tx_data
    
    ap_uint<256> tx_meta;
    ap_uint<64> tx_data;

    /*RDMA OP*/
    tx_meta.range(2,0) = 0x00000001; 
    /*lQPN*/
    tx_meta.range(26,3) = s_axi_lqpn; 
    /*
    lAddr
    if 0 writes from tx_data. 
    */
    tx_meta.range(74, 27) = s_axi_laddr; 
    /*rAddr*/
    tx_meta.range(122, 75) = s_axi_raddr; 
    //+(itt*4)
    /*len*/
    tx_meta.range(154, 123) = s_axi_len;
    

    m_axis_tx_meta.write(tx_meta);

    //Write data only if laddr is 0
    if (s_axi_laddr == 0) {
        tx_data.range(63, 0) = write_value;
        //tx_data.keep(7, 0) = 0xff;
        //tx_data.last = 1; 
        
        m_axis_tx_data.write(tx_data);
    }

}


void replication_engine_fsm(
    hls::stream<ProposedValue>& propose,
    hls::stream<LogEntry>& minPropReadBram_req,
    hls::stream<ap_uint<32>>& minPropReadBram_rsp,
    hls::stream<LogEntry>& readSlotsReadBram_req,
    hls::stream<LogEntry>& readSlotsReadBram_rsp,
    hls::stream<ap_uint<32>>& logReadBram_req,
    hls::stream<ap_uint<64>>& logReadBram_rsp,
    hls::stream<ap_uint<256>>& updateLocalValue_req,
    hls::stream<ap_uint<256>>& m_axis_tx_meta,
    hls::stream<ap_uint<64>>& m_axis_tx_data,
    int board_number,
    int number_of_nodes
) {

    #pragma HLS INTERFACE axis port = propose
    #pragma HLS INTERFACE axis port = minPropReadBram_req
    #pragma HLS INTERFACE axis port = minPropReadBram_rsp
    #pragma HLS INTERFACE axis port = readSlotsReadBram_req
    #pragma HLS INTERFACE axis port = readSlotsReadBram_rsp
    #pragma HLS INTERFACE axis port = logReadBram_req
    #pragma HLS INTERFACE axis port = logReadBram_rsp
    #pragma HLS INTERFACE axis port = updateLocalValue_req

    enum fsmStateType {
            INIT, 
            PROPOSE, 
            PREPARE_READ_PROP_REQ, 
            PREPARE_READ_PROP_MEM, 
            PREPARE_READ_PROP_RESP, 
            PREPARE_WRITE_READ_REQ, 
            PREPARE_SLOT_READ_MEM,
            PREPARE_SLOT_READ_RESP,
            ACCEPT
    };
    static fsmStateType state = INIT;
    static bool done = true;

    static ProposedValue pVal;
    static ap_uint<32> newHiPropNum = 0;
    static LogEntry slot;
    static int propValue = 0;
    static int reads = 0;
    static ap_uint<32> prepare_sGroup = 0, prepare2_sGroup = 0, prepare3_sGroup = 0, accept_sGroup = 0;
    static volatile int minPropFifoIndex[SYNC_GROUPS], slotReadFifoIndex[SYNC_GROUPS], slotAcceptFifoIndex[SYNC_GROUPS];
    static LogEntry logSlot;
    static volatile int fuo[SYNC_GROUPS];
    static ap_uint<32> newMinProp;
    static ap_uint<64> value;
    static ap_uint<32> minPropNumber = 0;
    static ap_uint<32> oldMinPropNumber = 0;
    static bool read_slot = true; 


    static ProposedValue proposed_value; 
    static int operation = 0;
    static ap_uint<32> sGroup = 0;
    static bool slowpath = true; 
    static bool slot_read = false; 


    //State of Replication
    switch (state) {
        case INIT:
            state = PROPOSE;
            break;

        case PROPOSE:
            if (!done) {
                if (slowpath) 
                    state = PREPARE_READ_PROP_REQ;
                else 
                    state = PREPARE_WRITE_READ_REQ;
            } else {
                state = PROPOSE; 
            }
            break;

        case PREPARE_READ_PROP_REQ:
            if (slowpath)
                state = PREPARE_READ_PROP_MEM; 
            else 
                state = ACCEPT; 
            break;

        case PREPARE_READ_PROP_MEM: 
            state = PREPARE_READ_PROP_RESP;
            break; 


        case PREPARE_READ_PROP_RESP: 
            if (minPropNumber != oldMinPropNumber || minPropFifoIndex[prepare_sGroup] == 0) {
                state = PREPARE_WRITE_READ_REQ; 
            } else {
                state = PREPARE_READ_PROP_MEM;
            }
            break; 


        case PREPARE_WRITE_READ_REQ:
            state = PREPARE_SLOT_READ_MEM; 
            break;


        case PREPARE_SLOT_READ_MEM:
            state = PREPARE_SLOT_READ_RESP;
            break; 


        case PREPARE_SLOT_READ_RESP:
            if (slot_read) {
                state = ACCEPT; 
            } else {
                state = PREPARE_SLOT_READ_MEM; 
            }
            break; 

        case ACCEPT:
            if (operation == propValue) {
                done = true;
            }
            state = PROPOSE; 
            break;

        default:
            break;
    }


    //Replication Actions
    switch(state) {
        case PROPOSE: {
            if (done == true && !propose.empty()) {
                propose.read(proposed_value);
                operation = proposed_value.value;
                sGroup = proposed_value.syncronizationGroup;
                done = false;
                std::cout << "Value: " << operation << std::endl; 
                //state = PREPARE_REQUEST;
            } 
            break; 
        }
         
        case PREPARE_READ_PROP_REQ: {
            int j=0;
            int qpn_tmp=board_number*(number_of_nodes-1);
            while (j<number_of_nodes){
                if(j!=board_number) {
                    if(!m_axis_tx_meta.full()){
                        int slot = (j < board_number) ? j : j-1;
                        rdma_read(
                            qpn_tmp,
                            LOG_BASE_ADDR + (LOG_ADDR_LEN * sGroup) + 8 + 4 * (FIFO_LENGTH * (slot) + (minPropFifoIndex[sGroup]%FIFO_LENGTH)),
                            LOG_BASE_ADDR + (LOG_ADDR_LEN * sGroup),
                            0x4,
                            m_axis_tx_meta
                        );
                        j++;
                        qpn_tmp++;
                    }
                }
                else {
                    j++;
                }
            }
            break; 
        }

        case PREPARE_READ_PROP_MEM: {
            if (!minPropReadBram_req.full()) {
                minPropReadBram_req.write(LogEntry(0, minPropFifoIndex[sGroup], sGroup));
            }
            break; 
        }

        case PREPARE_READ_PROP_RESP: {
            if (!minPropReadBram_rsp.empty() && !readSlotsReadBram_req.full()) {
                minPropReadBram_rsp.read(minPropNumber);
            }
            break; 
        }

        case PREPARE_WRITE_READ_REQ: {
            minPropFifoIndex[sGroup]+=1;
            oldMinPropNumber = minPropNumber;
            minPropNumber+=1;
            int j=0;
            int qpn_tmp=board_number*(number_of_nodes-1);
            while (j<number_of_nodes){
                if(j!=board_number){
                    if(!m_axis_tx_meta.full() && !m_axis_tx_data.full()){
                        rdma_write(
                            qpn_tmp,
                            0,
                            LOG_BASE_ADDR + (LOG_ADDR_LEN * sGroup),
                            0x8,
                            minPropNumber,
                            m_axis_tx_meta,
                            m_axis_tx_data
                            );
                        j++;
                        qpn_tmp++;
                    }
                }
                else {
                    j++;
                }
            }
            if (slowpath) {
                j=0;
                qpn_tmp=board_number*(number_of_nodes-1);
                while (j<number_of_nodes){
                    if(j!=board_number){
                        if(!m_axis_tx_meta.full()){
                            int slot = (j < board_number) ? j : j-1;
                            rdma_read(
                                qpn_tmp,
                                LOG_BASE_ADDR + LOG_ADDR_LEN * sGroup + LOG_MIN_PROP_ADDR_LEN + LOG_LOCAL_LOG_ADDR_LEN + 4 * (2 * FIFO_LENGTH * slot + (slotReadFifoIndex[prepare_sGroup]%NUM_SLOTS)),
                                LOG_BASE_ADDR + LOG_ADDR_LEN * sGroup + LOG_MIN_PROP_ADDR_LEN + 4 * (fuo[sGroup]%NUM_SLOTS),
                                0x8,
                                m_axis_tx_meta
                                );
                            j++;
                            qpn_tmp++;
                        }
                    }
                    else {
                        j++;
                    }
                }
            }
            break; 
        }

        case PREPARE_SLOT_READ_MEM: {
            readSlotsReadBram_req.write(LogEntry(minPropNumber, slotReadFifoIndex[sGroup], sGroup));
            break; 
        }

        case PREPARE_SLOT_READ_RESP: {
            if (!readSlotsReadBram_rsp.empty()) {
                readSlotsReadBram_rsp.read(slot);
                if(!slot.valid) {
                    slowpath =false; 
                } else {
                    slowpath = true; 
                }

                std::cout << "Value: " << operation << std::endl; 
                if (slot.valid) {
                    propValue = slot.value;
                } else {
                    propValue = operation;
                }
                std::cout << "Value: " << propValue << std::endl; 
                newHiPropNum = slot.propVal;

                slot_read = true;
                slotReadFifoIndex[sGroup]+=2;
            }
            break; 
        }

        case ACCEPT: {
            if (!updateLocalValue_req.full()) {
                ap_uint<64> sendLog;
                sendLog.range(31, 0) = newHiPropNum;
                sendLog.range(63, 32) = propValue;
                int j=0;
                int qpn_tmp=board_number*(number_of_nodes-1);
                while (j<number_of_nodes){
                    if(j!=board_number){
                        if(!m_axis_tx_meta.full() && !m_axis_tx_data.full()){
                            rdma_write(
                                qpn_tmp,
                                0,
                                LOG_BASE_ADDR + LOG_ADDR_LEN * sGroup + LOG_MIN_PROP_ADDR_LEN + 4 * (fuo[sGroup]%NUM_SLOTS),
                                0x8,
                                sendLog,
                                m_axis_tx_meta,
                                m_axis_tx_data
                                );
                            j++;
                            qpn_tmp++;
                        }
                    }
                    else {
                        j++;
                    }
                }
                fuo[sGroup]+=2;
                updateLocalValue_req.write(propValue);
            }
            break; 
        }
    }
}

void smr(
    hls::stream<ap_uint<256>>& smr_update,
    hls::stream<ProposedValue>& proposedValue,
    hls::stream<ap_uint<256>>& m_axis_tx_meta,
    hls::stream<ap_uint<64>>& m_axis_tx_data,
    hls::stream<ap_uint<32>>& logReadBram_req,
    hls::stream<ap_uint<64>>& logReadBram_rsp,
    hls::stream<LogEntry>& readSlotsReadBram_req,
    hls::stream<LogEntry>& readSlotsReadBram_rsp,
    hls::stream<LogEntry>& minPropReadBram_req,
    hls::stream<ap_uint<32>>& minPropReadBram_rsp,
    int board_number,
    int number_of_nodes


) {
    // static ap_uint<64> localValues[SYNC_GROUPS];
    // static updateLocalValue update;
    // static int permissible;
    // static int query;
    // static int counter = 0;

    // static hls::stream<updateLocalValue> updateLocalValue_req;

    replication_engine_fsm(
        proposedValue,
        minPropReadBram_req,
        minPropReadBram_rsp,
        readSlotsReadBram_req,
        readSlotsReadBram_rsp,
        logReadBram_req,
        logReadBram_rsp,
        smr_update,
        m_axis_tx_meta,
        m_axis_tx_data,
        board_number,
        number_of_nodes
    );

    // if (!updateLocalValue_req.empty() && !smr_update.full()) {
    //     updateLocalValue_req.read(update);
    //     localValues[update.syncGroup] = update.value;
    //     smr_update.write(update.value);
    // }


}

void deposit(
    int board_number, 
    int number_of_nodes,
    hls::stream<ap_uint<32>>& broadcast_req, 
    hls::stream<ap_uint<256>>& m_axis_tx_meta, 
    hls::stream<ap_uint<64>>& m_axis_tx_data
) {
    #pragma HLS inline off

    #pragma HLS INTERFACE axis port = broadcast_req
    #pragma HLS INTERFACE axis port = m_axis_tx_meta
    #pragma HLS INTERFACE axis port = m_axis_tx_data
    static ap_uint<32> pValue; 

    if (!broadcast_req.empty()) {
        broadcast_req.read(pValue);
        int j=0; 
        int qpn_tmp=board_number*(number_of_nodes-1);

        //std::cout << "Stock Increment: Item " << pValue.range(31, 16) << " by " << pValue.range(15, 0) << std::endl; 
        while (j<number_of_nodes){
            if(j!=board_number){
                if(!m_axis_tx_meta.full() && !m_axis_tx_data.full()) { 
                    rdma_write(
                        qpn_tmp,
                        0,
                        DEPOSIT_ADDR + (4 * 2 * board_number),
                        0x8,
                        (ap_uint<64>) pValue.range(31, 0),
                        m_axis_tx_meta, 
                        m_axis_tx_data
                        );
                    j++;
                    qpn_tmp++;
                }
            }
            else {
                j++;
            }
        }
    }

}


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
){
    #pragma HLS PIPELINE
    #pragma HLS INTERFACE axis port = minPropReadBram_req
    #pragma HLS INTERFACE axis port = minPropReadBram_rsp
    #pragma HLS INTERFACE axis port = readSlotsReadBram_req
    #pragma HLS INTERFACE axis port = readSlotsReadBram_rsp
    #pragma HLS INTERFACE axis port = logReadBram_req
    #pragma HLS INTERFACE axis port = logReadBram_rsp
    #pragma HLS INTERFACE axis port = update_rsp
    #pragma HLS INTERFACE axis port = update_noncon_rsp

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

    while (running) {

        //internal_clock++;

        if (!throughput_check.empty()) {
            throughput_check.read(running);
            running = false; 
        }

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

        //if (internal_clock == 10000) {
            //deposit

        if (!update_noncon_rsp.full()) {
            hbm_tmp = 0;
            for (int i=0; i<number_of_nodes; i++){
                if(i!=board_number){
                    hbm_tmp+=network_ptr[DEPOSIT_PTR + 2 * i];
                }
            }
            update_noncon_rsp.write(hbm_tmp);
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
}


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

) {
    #pragma HLS PIPELINE II=1
    #pragma HLS INTERFACE axis port = m_axis_tx_meta
    #pragma HLS INTERFACE axis port = m_axis_tx_data
    #pragma HLS INTERFACE axis port = s_axis_summary

    /*
        Internal streams for SMR module
    */
    static hls::stream<ap_uint<256>> smr_updated;
    static hls::stream<ProposedValue> proposed;
    static hls::stream<ap_uint<256>> smr_tx_meta;
    static hls::stream<ap_uint<64>> smr_tx_data;
    #pragma HLS STREAM depth=64 variable=smr_updated
    #pragma HLS STREAM depth=64 variable=proposed
    #pragma HLS STREAM depth=64 variable=smr_tx_meta
    #pragma HLS STREAM depth=64 variable=smr_tx_data

    /*
        Internal streams for sellItem module
    */
    static hls::stream<ap_uint<32>> stock_req;
    static hls::stream<ap_uint<256>> stock_tx_meta;
    static hls::stream<ap_uint<64>> stock_tx_data;
    #pragma HLS STREAM depth=64 variable=stock_req
    #pragma HLS STREAM depth=64 variable=stock_tx_meta
    #pragma HLS STREAM depth=64 variable=stock_tx_data
    
    /*
        Internal streams for openAuction module
    */
    static hls::stream<ap_uint<32>> bid_req;
    static hls::stream<ap_uint<256>> bid_tx_meta;
    static hls::stream<ap_uint<64>> bid_tx_data;
    #pragma HLS STREAM depth=64 variable=bid_tx_meta
    #pragma HLS STREAM depth=64 variable=bid_tx_data

    /*DEBUG STREAMS*/
    static hls::stream<ap_uint<256>> debug_tx_meta;
    static hls::stream<ap_uint<64>> debug_tx_data;
    

    /*
        Interal streams between SMR and MEM Manager
    */
    ap_uint<32> proposed_value, temp_amount, permiss_rsp;
    ap_uint<64> update, update_noncon; 
    static int counter = 0;
    static int deposit_counter = 0;
    static bool done = true;
    static bool throughput_finished = false; 
    static int bank_accounts = 100000;
    static int remote_bank_accounts = 0;
    static int deposits = 0; 
    static int swap_at = 1; 
    static int withdraw_updates = 0; 
    static ap_uint<64> operation; 

    static bool loaded = false; 

    //std::cout << "Starting RUBiS accelerator..." << std::endl; 
    BANK_MAIN_LOOP: while (counter < number_of_operations || deposit_counter < number_of_deposits) {

        

        if (!loaded && !operation_stream.empty()) { 
            operation_stream.read(operation);
            loaded = true; 
        }

        std::cout << "Counter: " << counter <<  " Method: " << operation << std::endl; 
        if (!s_axis_summary.empty()) {
            ap_uint<32> snap = s_axis_summary.read();
            //bank_accounts += snap;      // ← add the batched deposits

            temp_amount = snap;
            bank_accounts += temp_amount.range(31, 0);
            deposits += temp_amount.range(31, 0);
            stock_req.write(deposits);
            //counter++; 
            deposit_counter++;
            //loaded = false; 
            return;
        }
        if (loaded) {
            
            switch (operation.range(31, 0))
            {
                case 0: {
                    //Withdraw
                    if (!proposed.full() && done) {
                        temp_amount = operation.range(63, 32);
                        if (bank_accounts + remote_bank_accounts - temp_amount >= 0) {
                            std::cout << "Withdraw: " << temp_amount.range(31, 0) << " Quantity: " << bank_accounts + remote_bank_accounts << std::endl;
                            proposed_value.range(31, 0) = temp_amount;
                            proposed.write(ProposedValue(proposed_value, 0));
                            done = false; 
                        } else {
                            done = true;
                            counter++; 
                        }
                        loaded = false; 
                    }
                    break;
                }
/*
                case 1: {
                    //Deposit
                    if (!stock_req.full()) {
                        temp_amount = operation.range(63, 32);
                        bank_accounts += temp_amount.range(31, 0);
                        deposits += temp_amount.range(31, 0);
                        stock_req.write(deposits);
                        counter++; 
                        loaded = false; 
                    }
                    break;
                }
*/
                case 2: {
                    //Query
                    std::cout << "Total: " << bank_accounts + remote_bank_accounts << std::endl;
                    counter++; 
                    loaded = false; 
                    break;
                }

            }
        }

        if(board_number == 0)
            smr(
                smr_updated,
                proposed,
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

        if(board_number != 0)
            deposit(
                board_number, 
                number_of_nodes,
                stock_req, 
                stock_tx_meta, 
                stock_tx_data
            );

        meta_merger(
            smr_tx_meta,
            stock_tx_meta,
            m_axis_tx_meta
        );

        data_merger(
            smr_tx_data,
            stock_tx_data,
            m_axis_tx_data
        );

        if (!smr_updated.empty()) {
            ap_uint<256> temp;
            smr_updated.read(temp);
            done = true;
            counter++;
            std::cout << "Withdraw Amount: " << temp.range(29, 0) << std::endl; 
            bank_accounts -= temp.range(29, 0);
        }

        if (!update_rsp.empty()) {
            update_rsp.read(update);
            std::cout << "Updaing from Log! Method: " << update.range(31, 30) << " Operation: " << update.range(29, 0) << std::endl; 
            std::cout << "Withdraw Amount: " << update.range(29, 0) << std::endl; 
            bank_accounts -= update.range(29, 0);
            withdraw_updates++;
        }

        if (!update_noncon_rsp.empty()) {
            update_noncon_rsp.read(update_noncon);
            remote_bank_accounts = update_noncon;
        }

    }

    if (withdraw_updates >= throughput_check_value) {
        throughput_check.write(false);
    }
}

void hybrid_account_stream_krnl(
    hls::stream<pkt256>& m_axis_tx_meta,
    hls::stream<pkt64>& m_axis_tx_data,
    hls::stream<pkt64>& s_axis_tx_status,
    ap_uint<64>* operation_list,
    volatile int* HBM_PTR,
    int board_number,
    int number_of_operations,
    int number_of_nodes,
    int throughput_check_value,
    int number_of_deposits
) {

    #pragma HLS INTERFACE m_axi port=operation_list bundle=gmem0
    #pragma HLS INTERFACE m_axi port=HBM_PTR bundle=gmem1
    #pragma HLS INTERFACE s_axilite port=operation_list bundle=control
    #pragma HLS INTERFACE s_axilite port=HBM_PTR bundle=control
    #pragma HLS INTERFACE s_axilite port=board_number bundle=control
    #pragma HLS INTERFACE s_axilite port=number_of_operations bundle=control
    #pragma HLS INTERFACE s_axilite port=number_of_nodes bundle=control
    #pragma HLS INTERFACE s_axilite port=throughput_check_value bundle=control
    #pragma HLS INTERFACE s_axilite port=number_of_deposits bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control
    #pragma HLS INTERFACE axis port = m_axis_tx_meta
    #pragma HLS INTERFACE axis port = m_axis_tx_data
    #pragma HLS INTERFACE axis port = s_axis_tx_status
    #pragma HLS INTERFACE axis port = s_axis_summary

    #pragma HLS dataflow

    pkt64 status; 
    static hls::stream<ap_uint<64>> operation_stream;
    #pragma HLS STREAM depth=SWAP_SIZE variable=operation_stream

    static hls::stream<LogEntry> minPropReadBram_req;
    static hls::stream<ap_uint<32>> minPropReadBram_rsp;
    static hls::stream<LogEntry> readSlotsReadBram_req;
    static hls::stream<LogEntry> readSlotsReadBram_rsp;
    static hls::stream<ap_uint<32>> logReadBram_req;
    static hls::stream<ap_uint<64>> logReadBram_rsp;
    static hls::stream<ap_uint<32>> permissibility_req;
    static hls::stream<ap_uint<32>> permissibility_rsp;
    static hls::stream<ap_uint<64>> update_rsp;
    static hls::stream<ap_uint<64>> update_noncon_rsp;
    static hls::stream<bool> throughput_check;
    #pragma HLS STREAM depth=8 variable=minPropReadBram_req
    #pragma HLS STREAM depth=8 variable=minPropReadBram_rsp
    #pragma HLS STREAM depth=8 variable=readSlotsReadBram_req
    #pragma HLS STREAM depth=8 variable=readSlotsReadBram_rsp
    #pragma HLS STREAM depth=8 variable=logReadBram_req
    #pragma HLS STREAM depth=8 variable=logReadBram_rsp
    #pragma HLS STREAM depth=8 variable=permissibility_req
    #pragma HLS STREAM depth=8 variable=permissibility_rsp
    #pragma HLS STREAM depth=8 variable=update_rsp
    #pragma HLS STREAM depth=8 variable=update_noncon_rsp
    #pragma HLS INTERFACE s_axilite port=return bundle=control

    if (!s_axis_tx_status.empty()) {
        s_axis_tx_status.read(status);
    }

    bank(
        m_axis_tx_meta,
        m_axis_tx_data,
        s_axis_summary,
        board_number,
        operation_stream,
        number_of_operations,
        number_of_nodes,
        number_of_deposits,
        throughput_check_value,
        HBM_PTR,
        minPropReadBram_req,
        minPropReadBram_rsp,
        readSlotsReadBram_req,
        readSlotsReadBram_rsp,
        logReadBram_req,
        logReadBram_rsp,
        permissibility_req,
        permissibility_rsp,
        update_rsp,
        update_noncon_rsp,
        throughput_check
    );


    mem_manager(
        HBM_PTR,
        number_of_nodes,
        board_number,
        number_of_deposits,
        minPropReadBram_req,
        minPropReadBram_rsp,
        readSlotsReadBram_req,
        readSlotsReadBram_rsp,
        logReadBram_req,
        logReadBram_rsp,
        permissibility_req,
        permissibility_rsp,
        update_rsp,
        update_noncon_rsp,
        throughput_check
    );


}