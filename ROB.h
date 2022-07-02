//
// Created by 86158 on 2022/6/26.
//

#ifndef SIMULATOR_ROB_H
#define SIMULATOR_ROB_H

#include "Utility.h"

struct ROB_Node {
    unsigned int op_code, imm, rd;
    op_type_list op_type;
    unsigned int value, jump_step;
    unsigned pc, reorder_num;
    bool ready, is_end;
    bool jump_predicted;


    ROB_Node() {
        op_code = imm = value = 0;
        rd = jump_step = pc = reorder_num = (unsigned int) -1;
        ready = is_end = false;
        jump_predicted = false;
    }

    ~ROB_Node() {}

    void Clear() {
        op_code = imm = value = 0;
        rd = jump_step = pc = reorder_num = (unsigned int) -1;
        ready = is_end = false;
        jump_predicted = false;
    }
};

class ReorderBuffer {
    ROB_Node storage_[k_size];
    unsigned int head_, tail_, size_;
public:
    ReorderBuffer() {
        head_ = 0;
        tail_ = 0;
        size_ = 0;
    }

    ~ReorderBuffer() {}

    void Push(ROB_Node node_in) {
        node_in.reorder_num = tail_;
        storage_[tail_] = node_in;
        tail_ = (tail_ + 1) % k_size;
        size_++;
    }

    bool Empty() {
        return size_ == 0;
    }

    void Clear() {
        head_ = tail_ = size_ = 0;
    }

    ROB_Node Get(unsigned int index) {
        return storage_[index];
    }

    ROB_Node Front() {
        return storage_[head_];
    }

    void PopHead() {
        if (size_ == 0)
            return;
        storage_[head_].Clear();
        head_ = (head_ + 1) % k_size;
        size_--;
    }

    void PopBack() {
        if (size_ == 0)
            return;
        storage_[(tail_ - 1 + k_size) % k_size].Clear();
        tail_ = (tail_ - 1) % k_size;
        size_--;
    }

    bool Full() {
        return size_ == k_size;
    }

    void Set(ROB_Node node_in, unsigned int index) {
        storage_[index] = node_in;
    }

    unsigned int Spare() {
        return tail_;
    }

    unsigned int Size() {
        return size_;
    }

    unsigned int Head() {
        return head_;
    }
};


#endif //SIMULATOR_ROB_H
