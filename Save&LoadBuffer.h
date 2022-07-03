//
// Created by 86158 on 2022/6/26.
//

#ifndef SIMULATOR_SAVE_LOADBUFFER_H
#define SIMULATOR_SAVE_LOADBUFFER_H

#include "Utility.h"

struct LSB_Node {
    unsigned int vj, vk;
    unsigned int qj, qk;
    unsigned int rd;
    unsigned int imm;
    bool used, commit;
    unsigned int op_code;
    op_type_list op_type;
    unsigned int reorder_num;
    unsigned int count;

    LSB_Node() {
        vj = vk = 0;
        rd = 0;
        imm = 0;
        used = commit = false;
        op_code = 0;
        reorder_num = qj = qk = (unsigned int) -1;
        count = 0;
    }

    ~LSB_Node() {}

    void Clear() {
        vj = vk = 0;
        rd = 0;
        imm = 0;
        used = commit = false;
        op_code = 0;
        reorder_num = qj = qk = (unsigned int) -1;
        count = 0;
    }

    bool Ready() {
        if (op_code == 3)
            return int(qj) == -1;
        else if (op_code == 35)
            return commit;
        return false;
    }
};

class LoadStoreBuffer {
private:
    LSB_Node storage_[k_size];
    unsigned int head_;
    unsigned int tail_;
    unsigned int size_;
public:
    LoadStoreBuffer() {
        head_ = 0;
        tail_ = 0;
        size_ = 0;
    }

    ~LoadStoreBuffer() {}

    void Push(LSB_Node node_in) {
        storage_[tail_] = node_in;
        tail_ = (tail_ + 1) % k_size;
        size_++;
    }

    bool Empty() {
        head_ = tail_ = 0;
        return size_ == 0;
    }

    LSB_Node Get(unsigned int index) {
        return storage_[index];
    }

    void Set(LSB_Node node_in, unsigned int index) {
        storage_[index] = node_in;
    }

    LSB_Node Front() {
        return storage_[head_];
    }

    void SetFront(LSB_Node node_in) {
        storage_[head_] = node_in;
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

    void Clear() {
        for (int i = 0; i < k_size; i++)
            storage_[i].Clear();
        head_ = tail_ = size_ = 0;
    }

    unsigned int Size() {
        return size_;
    }

    unsigned int Head() {
        return head_;
    }
};

#endif //SIMULATOR_SAVE_LOADBUFFER_H
