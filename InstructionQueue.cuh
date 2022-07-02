//
// Created by 86158 on 2022/6/25.
//

#ifndef SIMULATOR_INSTRUCTIONQUEUE_CUH
#define SIMULATOR_INSTRUCTIONQUEUE_CUH

#include "Instruction.h"

struct IQ_Node {
    unsigned int raw;
    unsigned int pc_position;
    bool jump_predicted;
    unsigned int offset;

    IQ_Node() {
        raw = 0;
        pc_position = -1;
        jump_predicted= false;
        offset=0;
    }

    ~IQ_Node(){}

    void Clear() {
        raw = 0;
        pc_position = -1;
        jump_predicted= false;
        offset=0;
    }
};

class InstructionQueue {
private:
    IQ_Node storage_[k_size];
    int head_;
    int tail_;
    int size_;

public:
    InstructionQueue() {
        head_ = 0;
        tail_ = 0;
        size_ = 0;
    }

    ~InstructionQueue() {}

    void Push(IQ_Node node_in) {
        storage_[tail_] = node_in;
        tail_ = (tail_ + 1) % k_size;
        size_++;
    }

    bool Empty() {
        return size_ == 0;
    }

    IQ_Node Get(int index) {
        return storage_[index];
    }

    IQ_Node Front() {
        return storage_[head_];
    }

    void PopHead() {
        head_ = (head_ + 1) % k_size;
        size_--;
    }

    void PopBack() {
        tail_ = (tail_ - 1) % k_size;
        size_--;
    }

    bool Full() {
        return size_ == k_size;
    }

    unsigned int Size(){
        return size_;
    }

    unsigned int Head(){
        return head_;
    }
};

#endif //SIMULATOR_INSTRUCTIONQUEUE_CUH
