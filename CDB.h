//
// Created by 86158 on 2022/6/24.
//

#ifndef SIMULATOR_CDB_H
#define SIMULATOR_CDB_H

struct CommonDataBuss {
    unsigned int value;
    unsigned int reorder_num;
    unsigned int jump_num;
    bool jump_predicted;
    unsigned int pc;


    CommonDataBuss() {
        value = reorder_num = jump_num = -1;
        jump_predicted = false;
        pc=-1;
    }

    ~CommonDataBuss() {}

    void Clear() {
        value = reorder_num = jump_num = -1;
        jump_predicted = false;
        pc=-1;
    }
};


#endif //SIMULATOR_CDB_H
