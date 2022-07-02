//
// Created by 86158 on 2022/7/1.
//

#ifndef SIMULATOR_PREDICTOR_H
#define SIMULATOR_PREDICTOR_H

#include "Utility.h"

struct Predictor {
    unsigned int sit;

    Predictor() {
        sit = 1;
    }

    ~Predictor() {}

    void Clear() {
        sit = 1;
    }

    bool Jump() {
        return sit >= 2;
    }

    void Flush(bool jumped) {
        if (jumped) {
            if (sit < 3)
                sit++;
        } else {
            if (sit > 0)
                sit--;
        }
    }
};

class TLAP {
private:
    unsigned int history;
    Predictor predictor[8];
public:
    TLAP() {
        history = 0;
    }

    ~TLAP() {}

    void Clear() {
        history = 0;
        for (int i = 0; i < 8; i++) {
            predictor[i].Clear();
        }
    }

    bool Jump() {
        return predictor[history].Jump();
    }

    void Flush(bool jumped) {
        predictor[history].Flush(jumped);
        history = history % 4;
        history = history << 1;
        if (jumped)
            history++;
    }

};

class LBP {
private:
    Predictor predictor[1331];

    unsigned int Hash_(unsigned int in) {
        return in % 1331;
    }

public:
    LBP() {}

    ~LBP() {}

    void Clear() {
        for (int i = 0; i < 1331; i++) {
            predictor[i].Clear();
        }
    }

    bool Jump(unsigned int pc_in) {
        return predictor[Hash_(pc_in)].Jump();
    }

    void Flush(unsigned int pc_in, bool jump) {
        predictor[Hash_(pc_in)].Flush(jump);
    }
};


#endif //SIMULATOR_PREDICTOR_H
