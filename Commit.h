//
// Created by 86158 on 2022/6/28.
//

#ifndef SIMULATOR_COMMIT_H
#define SIMULATOR_COMMIT_H

#include "Utility.h"

struct Commit {
    unsigned int rd;
    unsigned int value;
    unsigned int reorder_num;
    bool is_end;

    Commit() {
        is_end = false;
        rd = reorder_num = -1;
        value = 0;
    }

    ~Commit() {}

    void Clear() {
        is_end = false;
        rd = reorder_num = -1;
        value = 0;
    }
};

#endif //SIMULATOR_COMMIT_H
