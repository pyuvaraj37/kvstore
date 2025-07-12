#include "smr.hpp"


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

}
