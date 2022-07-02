//
// Created by 86158 on 2022/6/27.
//

#ifndef SIMULATOR_REGISTER_H
#define SIMULATOR_REGISTER_H


#include "Utility.h"

struct Register {
    unsigned int value = 0, rename = -1;

    Register() {}

    ~Register() {}

    void Clear() {
        value = 0;
        rename = -1;
    }
};

#endif //SIMULATOR_REGISTER_H
