//
// Created by 86158 on 2022/6/24.
//

#ifndef SIMULATOR_RESERVATION_STATION_H
#define SIMULATOR_RESERVATION_STATION_H

#include <iostream>

#include "Utility.h"

struct RS_Node {
    unsigned int vj, vk, qj, qk;
    unsigned int rd, imm;
    unsigned int op_code;
    op_type_list op_type;
    unsigned int reorder_num;
    unsigned int pc;
    bool used;
    bool jump_predicted;

    RS_Node() {
        vj = vk = rd = 0;
        op_code = imm = 0;
        qj = qk = pc = -1;
        reorder_num = 0;
        used = false;
        jump_predicted=false;
    }

    ~RS_Node() {}

    void Clear() {
        vj = vk = rd = 0;
        op_code = imm = 0;
        qj = qk = pc = -1;
        reorder_num = 0;
        used = false;
        jump_predicted=false;
    }

    bool Ready() {
        if (op_code == 55 || op_code == 23 || op_code == 111)
            return true;
        if (op_code == 103)
            return int(qj) == -1;
        if (op_code == 99) //Type-B
            return int(qj) == -1 && int(qk) == -1;
        if (op_code == 3) //Type-L
            return int(qj) == -1;
        if (op_code == 35) //Type-S
            return int(qj) == -1 && int(qk) == -1;
        if (op_code == 19) //ADDI-SRAI
            return int(qj) == -1;
        if (op_code == 51) //ADD-AND
            return int(qj) == -1 && int(qk) == -1;
        return false;
    }
};

class ReservationStation {
private:
    RS_Node storage_[k_size];
public:
    ReservationStation() {}

    ~ReservationStation() {}

    void Clear() {
        for (int i = 0; i < k_size; i++)
            storage_[i].Clear();
    }

    void Set(RS_Node node_in, int index) {
        storage_[index] = node_in;
    }

    void Erase(int index) {
        storage_[index].Clear();
    }

    bool Full() {
        for (int i = 0; i < k_size; i++) {
            if (!storage_[i].used)
                return false;
        }
        return true;
    }

    RS_Node Get(int index) {
        return storage_[index];
    }

    unsigned int Spare() {
        for (unsigned int i = 0; i < k_size; i++) {
            if (!storage_[i].used)
                return i;
        }
        return -1;
    }
};


#endif //SIMULATOR_RESERVATION_STATION_H
