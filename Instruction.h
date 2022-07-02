//
// Created by 86158 on 2022/6/24.
//

#ifndef SIMULATOR_INSTRUCTION_H
#define SIMULATOR_INSTRUCTION_H


#include <iostream>
#include <cstring>

#include "Utility.h"

class Instruction {
public:
    unsigned int raw_;
    op_type_list op_name_;
    unsigned int op_code_;
    unsigned int rd_;
//    unsigned int func3_;
//    unsigned int func7_;
    unsigned int rs1_;
    unsigned int rs2_;
    unsigned int imm_num_;
    bool jump_predicted=false;
    unsigned int pc;

//    unsigned int imm1_; // for op L
//    unsigned int imm2_; // for op S
//    unsigned int imm3_; // for op B
//    unsigned int imm4_; // for op LU
//    unsigned int imm5_; // for op JAL
    Instruction() {}

    ~Instruction() {}

    void Init(unsigned int in) {
        raw_ = in;
        op_code_ = raw_ % Mi[7];
        rd_ = (raw_ % Mi[12]) / Mi[7];
        if (op_code_ == 3) {
            unsigned int func = (raw_ % Mi[15]) / Mi[12];
            rs1_ = (raw_ % Mi[20]) / Mi[15];
            imm_num_ = raw_ / Mi[20];
            if (imm_num_ >= (1 << 11))
                imm_num_ += ((0xffffffff >> 12) << 12);
            if (func == 0)
                op_name_ = LB;
            else if (func == 1)
                op_name_ = LH;
            else if (func == 2)
                op_name_ = LW;
            else if (func == 4)
                op_name_ = LBU;
            else if (func == 5)
                op_name_ = LHU;
        } else if (op_code_ == 35) {
            unsigned int func = (raw_ % Mi[15]) / Mi[12];
            rs1_ = (raw_ % Mi[20]) / Mi[15];
            rs2_ = (raw_ % Mi[25]) / Mi[20];
            imm_num_ = raw_ / Mi[25] * Mi[5] + (raw_ % Mi[12]) / Mi[7];
            if (imm_num_ >= (1 << 11))
                imm_num_ += ((0xffffffff >> 12) << 12);
            if (func == 0)
                op_name_ = SB;
            else if (func == 1)
                op_name_ = SH;
            else if (func == 2)
                op_name_ = SW;
        } else if (op_code_ == 51) {
            unsigned int func = (raw_ % Mi[15]) / Mi[12];
            unsigned int func_7 = raw_ >> 30;
            rs1_ = (raw_ % Mi[20]) / Mi[15];
            rs2_ = (raw_ % Mi[25]) / Mi[20];
            if (func == 0) {
                if (func_7 == 0)
                    op_name_ = ADD;
                else
                    op_name_ = SUB;
            } else if (func == 1) {
                op_name_ = SLL;
            } else if (func == 2) {
                op_name_ = SLT;
            } else if (func == 3) {
                op_name_ = SLTU;
            } else if (func == 4) {
                op_name_ = XOR;
            } else if (func == 5) {
                if (func_7 == 0)
                    op_name_ = SRL;
                else
                    op_name_ = SRA;
            } else if (func == 6) {
                op_name_ = OR;
            } else if (func == 7) {
                op_name_ = AND;
            }
        } else if (op_code_ == 99) {
            unsigned int func = (raw_ % Mi[15]) / Mi[12];
            rs1_ = (raw_ % Mi[20]) / Mi[15];
            rs2_ = (raw_ % Mi[25]) / Mi[20];
            imm_num_ = (raw_ % Mi[12]) / Mi[8] * 2 + (raw_ / Mi[25]) % Mi[6] * Mi[5] + (raw_ % Mi[8]) / Mi[7] * Mi[11] +
                       (raw_ >> 31) * Mi[12];
            if (imm_num_ >= (1 << 12))
                imm_num_ += ((0xffffffff >> 13) << 13);
            if (func == 0) {
                op_name_ = BEQ;
            } else if (func == 1) {
                op_name_ = BNE;
            } else if (func == 4) {
                op_name_ = BLT;
            } else if (func == 5) {
                op_name_ = BGE;
            } else if (func == 6) {
                op_name_ = BLTU;
            } else if (func == 7) {
                op_name_ = BGEU;
            }
        } else if (op_code_ == 19) {
            unsigned int func = (raw_ % Mi[15]) / Mi[12];
            rs1_ = (raw_ % Mi[20]) / Mi[15];
            rs2_ = (raw_ % Mi[25]) / Mi[20];
            if (func == 0) {
                imm_num_ = raw_ / Mi[20];
                if (imm_num_ >= (1 << 11))
                    imm_num_ += ((0xffffffff >> 12) << 12);
                op_name_ = ADDI;
            } else if (func == 1) {
                imm_num_ = rs2_;
                op_name_ = SLLI;
            } else if (func == 2) {
                imm_num_ = raw_ / Mi[20];
                if (imm_num_ >= (1 << 11))
                    imm_num_ += ((0xffffffff >> 12) << 12);
                op_name_ = SLTI;
            } else if (func == 3) {
                imm_num_ = raw_ / Mi[20];
                if (imm_num_ >= (1 << 11))
                    imm_num_ += ((0xffffffff >> 12) << 12);
                op_name_ = SLTIU;
            } else if (func == 4) {
                imm_num_ = raw_ / Mi[20];
                if (imm_num_ >= (1 << 11))
                    imm_num_ += ((0xffffffff >> 12) << 12);
                op_name_ = XORI;
            } else if (func == 5) {
                imm_num_ = rs2_;
                unsigned int func7 = raw_ >> 30;
                if (func7 == 0)
                    op_name_ = SRLI;
                else
                    op_name_ = SRAI;
            } else if (func == 6) {
                imm_num_ = raw_ / Mi[20];
                if (imm_num_ >= (1 << 11))
                    imm_num_ += ((0xffffffff >> 12) << 12);
                op_name_ = ORI;
            } else if (func == 7) {
                imm_num_ = raw_ / Mi[20];
                if (imm_num_ >= (1 << 11))
                    imm_num_ += ((0xffffffff >> 12) << 12);
                op_name_ = ANDI;
            }
        } else if (op_code_ == 103) {
            rs1_ = (raw_ % Mi[20]) / Mi[15];
            imm_num_ = raw_ / Mi[20];
            if (imm_num_ >= (1 << 11))
                imm_num_ += ((0xffffffff >> 12) << 12);
            op_name_ = JALR;
        } else if (op_code_ == 111) {
            imm_num_ = ((raw_ >> 21) % Mi[10] << 1) + ((raw_ >> 20) % 2) * Mi[11] + (raw_ % Mi[20]) / Mi[12] * Mi[12] +
                       ((raw_ >> 31) << 20);
            if (imm_num_ >= (1 << 20))
                imm_num_ += ((0xffffffff >> 21) << 21);
            op_name_ = JAL;
        } else if (op_code_ == 23) {
            imm_num_ = ((raw_ >> 12) << 12);
            op_name_ = AUIPC;
        } else if (op_code_ == 55) {
            imm_num_ = ((raw_ >> 12) << 12);
            op_name_ = LUI;
        } else
            std::cerr << "Error: " << op_code_ << std::endl;


//        std::cout << register_name[rd_] << ' ' << register_name[rs1_] << ' ' << register_name[rs2_] << ' ' << op_count
//                  << std::endl;
    }
};

#endif //SIMULATOR_INSTRUCTION_H
