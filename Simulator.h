//
// Created by 86158 on 2022/6/27.
//

#ifndef SIMULATOR_SIMULATOR_H
#define SIMULATOR_SIMULATOR_H

#include "CDB.h"
#include "Register.h"
#include "Reservation Station.h"
#include "InstructionQueue.cuh"
#include "Save&LoadBuffer.h"
#include "ROB.h"
#include "Commit.h"
#include "Predictor.h"
#include "Utility.h"

class Simulator {
private:
    unsigned int PC;
    unsigned int edge = 4;
    unsigned int memory_base = 0;
    CommonDataBuss memory_write;
    unsigned char Memory[2000000];
    Register register_up[32], register_down[32];
    Commit register_write;
    InstructionQueue iq;
    unsigned int iq_pointer;
    ReorderBuffer rob_up, rob_down;
    ROB_Node rob_next_node, commit_node;
    CommonDataBuss rob_lsb_up, rob_lsb_down;
    ReservationStation rs_up, rs_down;
    RS_Node rs_next_node;
    Commit rename_info;
    LoadStoreBuffer lsb_up, lsb_down;
    LSB_Node lsb_next_node;
    CommonDataBuss cdb_lsb_up, cdb_lsb_down, cdb_execute;
    RS_Node execute_node;
    // 可供使用的不同预测器
//    TLAP TLApredictor;
    LBP LBpredictor;
    int jump_result;
    int success_num = 0;
    int fail_num = 0;

    int op_count;
    bool jumped = false;

    unsigned int MemoryRead(unsigned int object) {
        return (unsigned int) Memory[memory_base + object];
    }

    void MemoryWrite(unsigned int object, unsigned int value) {
        Memory[memory_base + object] = value;
    }

    unsigned int PCRead(unsigned int start, unsigned int length, bool is_signed) {
        unsigned int ans = 0;
        for (int i = 0; i < length; i++) {
            ans = ans + MemoryRead(start + i) * (1 << (8 * i));
        }
        if (is_signed) {
            if (ans / (1 << (2 * length - 1))) {
                ans += (((0xffffffff) >> (16 - 4 * length)) << (16 - 4 * length));
            }
        }
        return ans;
    }

    void PCWrite(unsigned int pos, unsigned int value, unsigned int length) {
        for (int i = 0; i < length; i++) {
            MemoryWrite(pos + i, value % (1 << 8));
            value = (value >> 8);
        }
    }

    void Update() {
        rob_down = rob_up;
        rs_down = rs_up;
        lsb_down = lsb_up;
        for (int i = 0; i < 32; i++) {
            register_down[i] = register_up[i];
        }
        cdb_lsb_down = cdb_lsb_up;
        rob_lsb_down = rob_lsb_up;
    }

    void RunROB() {
        commit_node.Clear();
        rob_lsb_up.Clear();
        rob_up = rob_down;
        if (jump_result == -1) {
            rob_up.Clear();
            rob_next_node.Clear();
            return;
        } else if (jump_result == 1) {
            rob_up.PopHead();
        }
        if (cdb_execute.reorder_num != (unsigned int) -1) {
            ROB_Node temp = rob_down.Get(cdb_execute.reorder_num);
            temp.value = cdb_execute.value;
            temp.jump_step = cdb_execute.jump_num;
            temp.jump_predicted = cdb_execute.jump_predicted;
            temp.pc = cdb_execute.pc;
            temp.ready = true;
            rob_up.Set(temp, cdb_execute.reorder_num);
            rob_lsb_up.reorder_num = cdb_execute.reorder_num;
            rob_lsb_up.value = cdb_execute.value;
            rob_lsb_up.jump_predicted = cdb_execute.jump_predicted;
            rob_lsb_up.pc = cdb_execute.pc;
        }
        if (cdb_lsb_down.reorder_num != (unsigned int) -1) {
            ROB_Node temp = rob_down.Get(cdb_lsb_down.reorder_num);
            temp.value = cdb_lsb_down.value;
            temp.jump_predicted = cdb_lsb_down.jump_predicted;
            temp.pc = cdb_lsb_down.pc;
            temp.ready = true;
            rob_up.Set(temp, cdb_lsb_down.reorder_num);
        }
        if (!rob_up.Empty()) {
            ROB_Node temp = rob_up.Front();
            if (temp.op_code == 35 || temp.ready)
                commit_node = temp;
        }
        if (rob_next_node.op_code != 0) // 可添加
            rob_up.Push(rob_next_node);
        rob_next_node.Clear();
    }

    void RunLSB() {
        cdb_lsb_up.Clear();
        if (jump_result == (unsigned int) -1) {
            lsb_up.Clear();
            lsb_next_node.Clear();
            unsigned pos = lsb_down.Head();
            for (int i = 0; i < k_size; i++) { //逐条检查commit情况
                LSB_Node temp = lsb_down.Get((pos + i) % k_size);
                if (!temp.op_code)
                    break;
                if (temp.commit) {
                    temp.count = 0;
                    lsb_up.Push(temp);
                }
            }
        } else {
            lsb_up = lsb_down;
        }
        if (memory_write.reorder_num != (unsigned int) -1) {
            for (int i = 0; i < k_size; i++) {
                LSB_Node check = lsb_up.Get(i);
                if (check.reorder_num == memory_write.reorder_num) {
                    check.commit = true;
                    check.vj = register_down[check.qj].value;
                    check.vk = register_down[check.qk].value;
                    lsb_up.Set(check, i);
                    break;
                }
            }
        }
        if (rob_lsb_down.reorder_num != (unsigned int) -1) {
            for (int i = 0; i < k_size; i++) {
                LSB_Node check = lsb_up.Get(i);
                if (check.op_code == 3 && check.qj == rob_lsb_down.reorder_num) {
                    check.qj = -1;
                    check.vj = rob_lsb_down.value;
                    lsb_up.Set(check, i);
                }
            }
        }
        LSB_Node temp = lsb_up.Front();
        if (temp.Ready()) {
            temp.count++;
            if (temp.count) {
                if (temp.op_code == 3) { //Load_op
                    unsigned int pos = temp.vj + temp.imm;
                    cdb_lsb_up.reorder_num = temp.reorder_num;
                    if (temp.op_type == LB)
                        cdb_lsb_up.value = PCRead(pos, 1, true);
                    else if (temp.op_type == LH) {
                        cdb_lsb_up.value = PCRead(pos, 2, true);
                    } else if (temp.op_type == LW) {
                        cdb_lsb_up.value = PCRead(pos, 4, false);
                    } else if (temp.op_type == LBU) {
                        cdb_lsb_up.value = PCRead(pos, 1, false);
                    } else if (temp.op_type == LHU) {
                        cdb_lsb_up.value = PCRead(pos, 1, false);
                    }
                } else if (temp.op_code == 35) { //Store_op
                    unsigned int pos = temp.vj + temp.imm;
                    if (temp.op_type == SB)
                        PCWrite(pos, temp.vk, 1);
                    else if (temp.op_type == SH)
                        PCWrite(pos, temp.vk, 2);
                    else if (temp.op_type == SW)
                        PCWrite(pos, temp.vk, 4);
                }
                lsb_up.PopHead();
            } else {
                lsb_up.SetFront(temp);
            }
        }
        if (lsb_next_node.op_code != 0)
            lsb_up.Push(lsb_next_node);
        lsb_next_node.Clear();
    }

    void RunRS() {
        rs_up = rs_down;
        if (jump_result == (unsigned int) -1) {
            rs_up.Clear();
            rs_next_node.Clear();
            return;
        }
        if (cdb_execute.reorder_num != (unsigned int) -1) {
            for (int i = 0; i < k_size; i++) {// 逐条检查有无计算出
                RS_Node check = rs_up.Get(i);
                if (check.used) {
                    if (check.qj == cdb_execute.reorder_num) {// 若此处值已经计算出
                        check.qj = -1;
                        check.vj = cdb_execute.value;
                    }
                    if (check.qk == cdb_execute.reorder_num) {
                        check.qk = -1;
                        check.vk = cdb_execute.value;
                    }
                    rs_up.Set(check, i);
                }
            }
            if (rs_next_node.op_code != 0) {// next也要检查
                if (rs_next_node.qj == cdb_execute.reorder_num) {
                    rs_next_node.qj = -1;
                    rs_next_node.vj = cdb_execute.value;
                }
                if (rs_next_node.qk == cdb_execute.reorder_num) {
                    rs_next_node.qk = -1;
                    rs_next_node.vk = cdb_execute.value;
                }
            }
        }
        if (cdb_lsb_down.reorder_num != (unsigned int) -1) {
            for (int i = 0; i < k_size; i++) {
                RS_Node check = rs_up.Get(i);
                if (check.used) {
                    if (check.qj == cdb_lsb_down.reorder_num) {
                        check.qj = -1;
                        check.vj = cdb_lsb_down.value;
                    }
                    if (check.qk == cdb_lsb_down.reorder_num) {
                        check.qk = -1;
                        check.vk = cdb_lsb_down.value;
                    }
                    rs_up.Set(check, i);
                }
            }
            if (rs_next_node.op_code != 0) {
                if (rs_next_node.qj == cdb_lsb_down.reorder_num) {
                    rs_next_node.qj = -1;
                    rs_next_node.vj = cdb_lsb_down.value;
                }
                if (rs_next_node.qk == cdb_lsb_down.reorder_num) {
                    rs_next_node.qk = -1;
                    rs_next_node.vk = cdb_lsb_down.value;
                }
            }
        }
        // 取出接下来要处理的指令
        execute_node.Clear();
        for (int i = 0; i < k_size; i++) {
            RS_Node temp = rs_up.Get(i);
            if (temp.Ready()) {
                execute_node = temp;
                rs_up.Erase(i);
                break;
            }
        }
        // 添加新指令
        if (rs_next_node.op_code != 0) {
            unsigned int pos = rs_up.Spare();
            rs_up.Set(rs_next_node, pos);
            rs_next_node.Clear();
        }
    }

    void RunRegister() {
//        committeed = false;
        for (int i = 0; i < k_size; i++) {
            register_up[i] = register_down[i];
        }
        if (jump_result == (unsigned int) -1) { //清空
            for (int i = 0; i < k_size; i++)
                register_up[i].rename = -1;
            if (register_write.rd != (unsigned int) -1) {
                register_up[register_write.rd].value = register_write.value;
//                committeed = true;
            }
        } else {
            if (register_write.rd != (unsigned int) -1) {
                register_up[register_write.rd].value = register_write.value;
                if (register_up[register_write.rd].rename == register_write.reorder_num)
                    register_up[register_write.rd].rename = -1;
//                committeed = true;
            }
            if (rename_info.rd != (unsigned int) -1) {
                register_up[rename_info.rd].rename = rename_info.value;
//                committeed = true;
            }
        }
//        Debug(committeed);
    }

    void RunExecute() {
        cdb_execute.Clear();
        if (execute_node.op_code == 0)
            return;
        cdb_execute.reorder_num = execute_node.reorder_num;
        cdb_execute.jump_predicted = execute_node.jump_predicted;
        cdb_execute.pc = execute_node.pc;
        if (execute_node.op_code == 99) {
            if (execute_node.op_type == BEQ) {
                if (execute_node.vj == execute_node.vk)
                    cdb_execute.jump_num = execute_node.pc + execute_node.imm;
            } else if (execute_node.op_type == BNE) {
                if (execute_node.vj != execute_node.vk)
                    cdb_execute.jump_num = execute_node.pc + execute_node.imm;
            } else if (execute_node.op_type == BLT) {
                if ((int) execute_node.vj < (int) execute_node.vk)
                    cdb_execute.jump_num = execute_node.pc + execute_node.imm;
            } else if (execute_node.op_type == BGE) {
                if ((int) execute_node.vj >= (int) execute_node.vk)
                    cdb_execute.jump_num = execute_node.pc + execute_node.imm;
            } else if (execute_node.op_type == BLTU) {
                if (execute_node.vj < execute_node.vk)
                    cdb_execute.jump_num = execute_node.pc + execute_node.imm;
            } else if (execute_node.op_type == BGEU) {
                if (execute_node.vj >= execute_node.vk)
                    cdb_execute.jump_num = execute_node.pc + execute_node.imm;
            }
        } else if (execute_node.op_code == 19) {
            if (execute_node.op_type == ADDI) {
                cdb_execute.value = execute_node.vj + execute_node.imm;
            } else if (execute_node.op_type == SLTI) {
                if ((int) execute_node.vj < (int) execute_node.imm)
                    cdb_execute.value = 1;
                else
                    cdb_execute.value = 0;
            } else if (execute_node.op_type == SLTIU) {
                if (execute_node.vj < execute_node.imm)
                    cdb_execute.value = 1;
                else
                    cdb_execute.value = 0;
            } else if (execute_node.op_type == XORI) {
                cdb_execute.value = execute_node.vj ^ execute_node.imm;
            } else if (execute_node.op_type == ORI) {
                cdb_execute.value = execute_node.vj | execute_node.imm;
            } else if (execute_node.op_type == ANDI) {
                cdb_execute.value = execute_node.vj & execute_node.imm;
            } else if (execute_node.op_type == SLLI) {
                cdb_execute.value = execute_node.vj << execute_node.imm;
            } else if (execute_node.op_type == SRLI) {
                cdb_execute.value = execute_node.vj >> execute_node.imm;
            } else if (execute_node.op_type == SRAI) {
                cdb_execute.value = execute_node.vj >> execute_node.imm;
                if (execute_node.vj >> 31 == 1)
                    cdb_execute.value += ((0xffffffff >> execute_node.imm) << execute_node.imm);
            }
        } else if (execute_node.op_code == 51) {
            if (execute_node.op_type == ADD) {
                cdb_execute.value = execute_node.vj + execute_node.vk;
            } else if (execute_node.op_type == SUB) {
                cdb_execute.value = execute_node.vj - execute_node.vk;
            } else if (execute_node.op_type == SLL) {
                cdb_execute.value = execute_node.vj << execute_node.vk;
            } else if (execute_node.op_type == SLT) {
                if ((int) execute_node.vj < (int) execute_node.vk)
                    cdb_execute.value = 1;
                else
                    cdb_execute.value = 0;
            } else if (execute_node.op_type == SLTU) {
                if (execute_node.vj < execute_node.vk)
                    cdb_execute.value = 1;
                else
                    cdb_execute.value = 0;
            } else if (execute_node.op_type == XOR) {
                cdb_execute.value = execute_node.vj ^ execute_node.vk;
            } else if (execute_node.op_type == SRL) {
                cdb_execute.value = execute_node.vj >> ((execute_node.vk << 27) >> 27);
            } else if (execute_node.op_type == SRA) {
                cdb_execute.value = execute_node.vj >> ((execute_node.vk << 27) >> 27);
                bool is_ne = execute_node.vj >> 31;
                if (is_ne) {
                    cdb_execute.value += 0xffffffff >> (32 - ((execute_node.vk << 27) >> 27))
                                                    << (32 - ((execute_node.vk << 27) >> 27));
                }
            } else if (execute_node.op_type == OR) {
                cdb_execute.value = execute_node.vj | execute_node.vk;
            } else if (execute_node.op_type == AND) {
                cdb_execute.value = execute_node.vj & execute_node.vk;
            }
        } else if (execute_node.op_type == LUI) {
            cdb_execute.value = execute_node.imm;
        } else if (execute_node.op_type == AUIPC) {
            cdb_execute.value = execute_node.imm + execute_node.pc;
        } else if (execute_node.op_type == JAL) {
            cdb_execute.value = execute_node.pc + 4;
            cdb_execute.jump_num = execute_node.pc + execute_node.imm;
        } else if (execute_node.op_type == JALR) {
            cdb_execute.value = execute_node.pc + 4;
            cdb_execute.jump_num = ((execute_node.vj + execute_node.imm) >> 1) << 1;
        }
    }

    void RunIssue() {
        rename_info.Clear();
        if (iq.Empty() || rob_down.Full())
            return;
        unsigned int order_in = iq.Front().raw;
        bool is_end = (order_in == 0x0ff00513);
        Instruction instruction;
        instruction.Init(order_in);
        instruction.jump_predicted = iq.Front().jump_predicted;
        instruction.pc = iq.Front().pc_position;
        // TODO: 是否可以优化
        if (instruction.op_code_ == 3) { //Load_op
            if (instruction.rd_ != 0) {
                if (lsb_down.Full())
                    return;
                unsigned int rob_pos = rob_down.Spare();
                rob_next_node.op_code = instruction.op_code_;
                rob_next_node.op_type = instruction.op_name_;
                rob_next_node.rd = instruction.rd_;
                rob_next_node.imm = instruction.imm_num_;
                rob_next_node.pc = iq.Front().pc_position;
                rob_next_node.ready = false;
                rob_next_node.jump_predicted = instruction.jump_predicted;
                lsb_next_node.op_code = instruction.op_code_;
                lsb_next_node.op_type = instruction.op_name_;
                lsb_next_node.imm = instruction.imm_num_;
                lsb_next_node.used = true;
                if (register_down[instruction.rs1_].rename == (unsigned int) -1)
                    lsb_next_node.vj = register_down[instruction.rs1_].value;
                else {
                    ROB_Node temp = rob_down.Get(register_down[instruction.rs1_].rename);
                    if (temp.ready)
                        lsb_next_node.vj = temp.value;
                    else
                        lsb_next_node.qj = register_down[instruction.rs1_].rename;
                }
                lsb_next_node.reorder_num = rob_pos;
                rename_info.rd = instruction.rd_;
                rename_info.value = rob_pos;
            }
        } else if (instruction.op_code_ == 35) { //Store_op
            if (rob_down.Full())
                return;
            unsigned int rob_pos = rob_down.Spare();
            rob_next_node.op_code = instruction.op_code_;
            rob_next_node.op_type = instruction.op_name_;
            rob_next_node.rd = instruction.rd_;
            rob_next_node.imm = instruction.imm_num_;
            rob_next_node.pc = iq.Front().pc_position;
            rob_next_node.ready = false;
            rob_next_node.jump_predicted = instruction.jump_predicted;
            lsb_next_node.op_code = instruction.op_code_;
            lsb_next_node.op_type = instruction.op_name_;
            lsb_next_node.imm = instruction.imm_num_;
            lsb_next_node.used = true;
            lsb_next_node.qj = instruction.rs1_;
            lsb_next_node.qk = instruction.rs2_;
            lsb_next_node.reorder_num = rob_pos;
        } else if (instruction.op_code_ == 51) {
            if (instruction.rd_ != 0) {
                if (rs_down.Full())
                    return;
                unsigned int rob_pos = rob_down.Spare();
                rob_next_node.op_code = instruction.op_code_;
                rob_next_node.op_type = instruction.op_name_;
                rob_next_node.rd = instruction.rd_;
                rob_next_node.imm = instruction.imm_num_;
                rob_next_node.pc = iq.Front().pc_position;
                rob_next_node.ready = false;
                rob_next_node.jump_predicted = instruction.jump_predicted;
                rs_next_node.jump_predicted = instruction.jump_predicted;
                rs_next_node.pc = instruction.pc;
                rs_next_node.op_code = instruction.op_code_;
                rs_next_node.op_type = instruction.op_name_;
                rs_next_node.imm = instruction.imm_num_;
                rs_next_node.used = true;
                rs_next_node.reorder_num = rob_pos;
                rs_next_node.pc = iq.Front().pc_position;
                if (register_down[instruction.rs1_].rename == (unsigned int) -1) {
                    rs_next_node.vj = register_down[instruction.rs1_].value;
                } else {
                    ROB_Node temp = rob_down.Get(register_down[instruction.rs1_].rename);
                    if (temp.ready)
                        rs_next_node.vj = temp.value;
                    else
                        rs_next_node.qj = register_down[instruction.rs1_].rename;
                }
                if (register_down[instruction.rs2_].rename == (unsigned int) -1) {
                    rs_next_node.vk = register_down[instruction.rs2_].value;
                } else {
                    ROB_Node temp = rob_down.Get(register_down[instruction.rs2_].rename);
                    if (temp.ready)
                        rs_next_node.vk = temp.value;
                    else
                        rs_next_node.qk = register_down[instruction.rs2_].rename;
                }
                rename_info.rd = instruction.rd_;
                rename_info.value = rob_pos;
            }
        } else if (instruction.op_code_ == 99) {
            if (rs_down.Full())
                return;
            unsigned int rob_pos = rob_down.Spare();
            rob_next_node.op_code = instruction.op_code_;
            rob_next_node.op_type = instruction.op_name_;
            rob_next_node.rd = instruction.rd_;
            rob_next_node.imm = instruction.imm_num_;
            rob_next_node.pc = iq.Front().pc_position;
            rob_next_node.ready = false;
            rob_next_node.jump_predicted = instruction.jump_predicted;
            rs_next_node.jump_predicted = instruction.jump_predicted;
            rs_next_node.pc = instruction.pc;
            rs_next_node.op_code = instruction.op_code_;
            rs_next_node.op_type = instruction.op_name_;
            rs_next_node.imm = instruction.imm_num_;
            rs_next_node.used = true;
            rs_next_node.reorder_num = rob_pos;
            rs_next_node.pc = iq.Front().pc_position;
            if (register_down[instruction.rs1_].rename == (unsigned int) -1) {
                rs_next_node.vj = register_down[instruction.rs1_].value;
            } else {
                ROB_Node temp = rob_down.Get(register_down[instruction.rs1_].rename);
                if (temp.ready)
                    rs_next_node.vj = temp.value;
                else
                    rs_next_node.qj = register_down[instruction.rs1_].rename;
            }
            if (register_down[instruction.rs2_].rename == (unsigned int) -1) {
                rs_next_node.vk = register_down[instruction.rs2_].value;
            } else {
                ROB_Node temp = rob_down.Get(register_down[instruction.rs2_].rename);
                if (temp.ready)
                    rs_next_node.vk = temp.value;
                else
                    rs_next_node.qk = register_down[instruction.rs2_].rename;
            }
        } else if (instruction.op_code_ == 19) {
            if (instruction.rd_ != 0) {
                if (rs_down.Full())
                    return;
                if (instruction.op_name_ == SLLI || instruction.op_name_ == SRLI || instruction.op_name_ == SRAI) {
                    unsigned int rob_pos = rob_down.Spare();
                    rob_next_node.op_code = instruction.op_code_;
                    rob_next_node.op_type = instruction.op_name_;
                    rob_next_node.rd = instruction.rd_;
                    rob_next_node.imm = instruction.imm_num_;
                    rob_next_node.pc = iq.Front().pc_position;
                    rob_next_node.ready = false;
                    rob_next_node.jump_predicted = instruction.jump_predicted;
                    rs_next_node.jump_predicted = instruction.jump_predicted;
                    rs_next_node.pc = instruction.pc;
                    rs_next_node.op_code = instruction.op_code_;
                    rs_next_node.op_type = instruction.op_name_;
                    rs_next_node.imm = instruction.imm_num_;
                    rs_next_node.reorder_num = rob_pos;
                    rs_next_node.pc = iq.Front().pc_position;
                    rs_next_node.used = true;
                    if (register_down[instruction.rs1_].rename == (unsigned int) -1) {
                        rs_next_node.vj = register_down[instruction.rs1_].value;
                    } else {
                        ROB_Node temp = rob_down.Get(register_down[instruction.rs1_].rename);
                        if (temp.ready)
                            rs_next_node.vj = temp.value;
                        else
                            rs_next_node.qj = register_down[instruction.rs1_].rename;
                    }
                    rename_info.rd = instruction.rd_;
                    rename_info.value = rob_pos;
                } else {
                    unsigned int rob_pos = rob_down.Spare();
                    rob_next_node.op_code = instruction.op_code_;
                    rob_next_node.op_type = instruction.op_name_;
                    rob_next_node.rd = instruction.rd_;
                    rob_next_node.imm = instruction.imm_num_;
                    rob_next_node.pc = iq.Front().pc_position;
                    rob_next_node.ready = false;
                    rob_next_node.is_end = is_end;
                    rob_next_node.jump_predicted = instruction.jump_predicted;
                    rs_next_node.jump_predicted = instruction.jump_predicted;
                    rs_next_node.pc = instruction.pc;
                    rs_next_node.op_code = instruction.op_code_;
                    rs_next_node.op_type = instruction.op_name_;
                    rs_next_node.imm = instruction.imm_num_;
                    rs_next_node.used = true;
                    rs_next_node.reorder_num = rob_pos;
                    rs_next_node.pc = iq.Front().pc_position;
                    if (register_down[instruction.rs1_].rename == (unsigned int) -1) {
                        rs_next_node.vj = register_down[instruction.rs1_].value;
                    } else {
                        ROB_Node temp = rob_down.Get(register_down[instruction.rs1_].rename);
                        if (temp.ready)
                            rs_next_node.vj = temp.value;
                        else
                            rs_next_node.qj = register_down[instruction.rs1_].rename;
                    }
                    rename_info.rd = instruction.rd_;
                    rename_info.value = rob_pos;
                }
            }
        } else if (instruction.op_code_ == 103) {
            if (rs_down.Full())
                return;
            unsigned int rob_pos = rob_down.Spare();
            rob_next_node.op_code = instruction.op_code_;
            rob_next_node.op_type = instruction.op_name_;
            rob_next_node.imm = instruction.imm_num_;
            if (instruction.rd_ != 0) // 忽略指向0号寄存器的指令
                rob_next_node.rd = instruction.rd_;
            rob_next_node.pc = iq.Front().pc_position;
            rob_next_node.jump_predicted = instruction.jump_predicted;
            rs_next_node.jump_predicted = instruction.jump_predicted;
            rs_next_node.pc = instruction.pc;
            rs_next_node.op_code = instruction.op_code_;
            rs_next_node.op_type = instruction.op_name_;
            rs_next_node.imm = instruction.imm_num_;
            rs_next_node.used = true;
            rs_next_node.reorder_num = rob_pos;
            rs_next_node.pc = iq.Front().pc_position;
            if (register_down[instruction.rs1_].rename == (unsigned int) -1) {
                rs_next_node.vj = register_down[instruction.rs1_].value;
            } else {
                ROB_Node temp = rob_down.Get(register_down[instruction.rs1_].rename);
                if (temp.ready)
                    rs_next_node.vj = temp.value;
                else
                    rs_next_node.qj = register_down[instruction.rs1_].rename;
            }
            if (instruction.rd_ != 0) {
                rename_info.rd = instruction.rd_;
                rename_info.value = rob_pos;
            }
        } else if (instruction.op_code_ == 111) {
            if (rs_down.Full())
                return;
            unsigned int rob_pos = rob_down.Spare();
            rob_next_node.op_code = instruction.op_code_;
            rob_next_node.op_type = instruction.op_name_;
            rob_next_node.imm = instruction.imm_num_;
            if (instruction.rd_ != 0)
                rob_next_node.rd = instruction.rd_;
            rob_next_node.pc = iq.Front().pc_position;
            rob_next_node.jump_predicted = instruction.jump_predicted;
            rs_next_node.jump_predicted = instruction.jump_predicted;
            rs_next_node.pc = instruction.pc;
            rs_next_node.op_code = instruction.op_code_;
            rs_next_node.op_type = instruction.op_name_;
            rs_next_node.imm = instruction.imm_num_;
            rs_next_node.used = true;
            rs_next_node.reorder_num = rob_pos;
            rs_next_node.pc = iq.Front().pc_position;
            if (instruction.rd_ != 0) {
                rename_info.rd = instruction.rd_;
                rename_info.value = rob_pos;
            }
        } else if (instruction.op_code_ == 23) {
            if (instruction.rd_ != 0) {
                if (rs_up.Full())
                    return;
                unsigned int rob_pos = rob_down.Spare();
                rob_next_node.op_code = instruction.op_code_;
                rob_next_node.op_type = instruction.op_name_;
                rob_next_node.imm = instruction.imm_num_;
                rob_next_node.rd = instruction.rd_;
                rob_next_node.pc = iq.Front().pc_position;
                rob_next_node.jump_predicted = instruction.jump_predicted;
                rs_next_node.jump_predicted = instruction.jump_predicted;
                rs_next_node.pc = instruction.pc;
                rs_next_node.op_code = instruction.op_code_;
                rs_next_node.op_type = instruction.op_name_;
                rs_next_node.imm = instruction.imm_num_;
                rs_next_node.used = true;
                rs_next_node.reorder_num = rob_pos;
                rs_next_node.pc = iq.Front().pc_position;
                rename_info.rd = instruction.rd_;
                rename_info.value = rob_pos;
            }
        } else if (instruction.op_code_ == 55) {
            if (instruction.rd_ != 0) {
                unsigned int rob_pos = rob_down.Spare();
                rob_next_node.op_code = instruction.op_code_;
                rob_next_node.op_type = instruction.op_name_;
                rob_next_node.imm = instruction.imm_num_;
                rob_next_node.rd = instruction.rd_;
                rob_next_node.pc = iq.Front().pc_position;
                rob_next_node.jump_predicted = instruction.jump_predicted;
                rs_next_node.jump_predicted = instruction.jump_predicted;
                rs_next_node.pc = instruction.pc;
                rs_next_node.op_code = instruction.op_code_;
                rs_next_node.op_type = instruction.op_name_;
                rs_next_node.imm = instruction.imm_num_;
                rs_next_node.used = true;
                rs_next_node.reorder_num = rob_pos;
                rs_next_node.pc = iq.Front().pc_position;
                rename_info.rd = instruction.rd_;
                rename_info.value = rob_pos;
            }
        }
        iq.PopHead();
    }

    void RunCommit() {
        register_write.Clear();
        memory_write.Clear();
        jump_result = 0;
        if (!commit_node.op_code) {
        } else if (commit_node.op_code == 35) {
            memory_write.reorder_num = commit_node.reorder_num;
            memory_write.value = commit_node.value;
            jump_result = 1;
        } else if (commit_node.op_code == 99) {
            if (commit_node.jump_step != (unsigned int) -1) {
                PC = commit_node.jump_step;
                jump_result = -1;
//                if (commit_node.jump_predicted)
//                    TLApredictor.Flush(false);
//                else
//                    TLApredictor.Flush(true);
//                fail_num++;

//                TLApredictor.Flush(true);
                LBpredictor.Flush(commit_node.pc, true);
                if (commit_node.jump_predicted) {
                    success_num++;
                } else
                    fail_num++;

            } else {
                jump_result = 1;
//                if (commit_node.jump_predicted)
//                    TLApredictor.Flush(true);
//                else
//                    TLApredictor.Flush(false);
//                success_num++;

//                TLApredictor.Flush(false);
                LBpredictor.Flush(commit_node.pc, false);
                if (commit_node.jump_predicted)
                    fail_num++;
                else {
                    success_num++;
                }
            }
        } else if (commit_node.op_code == 103 || commit_node.op_code == 111) {
            PC = commit_node.jump_step;
            register_write.rd = commit_node.rd;
            register_write.reorder_num = commit_node.reorder_num;
            register_write.value = commit_node.value;
            jump_result = -1;
        } else {
            register_write.rd = commit_node.rd;
            register_write.reorder_num = commit_node.reorder_num;
            register_write.value = commit_node.value;
            jump_result = 1;
        }
    }

    void RunIQ() {
        if (jump_result != (unsigned int) -1 && iq_pointer != (unsigned int) -1) {
            // 上升沿
            IQ_Node temp;
            temp.pc_position = iq_pointer;
            temp.raw = PCRead(iq_pointer, 4, false);
            if (((temp.raw << 25) >> 25) == 99) { // 若是branch指令
//                temp.jump_predicted = TLApredictor.Jump();
                temp.jump_predicted = LBpredictor.Jump(iq_pointer);
                if (temp.jump_predicted) {
                    temp.offset = (temp.raw % Mi[12]) / Mi[8] * 2 + (temp.raw / Mi[25]) % Mi[6] * Mi[5] +
                                  (temp.raw % Mi[8]) / Mi[7] * Mi[11] +
                                  (temp.raw >> 31) * Mi[12];
                    if (temp.offset >= (1 << 12))
                        temp.offset += ((0xffffffff >> 13) << 13);
                }
            }
            iq.Push(temp);
        }
        // 下降沿
        if (iq.Full())
            iq_pointer = -1;
        else
            iq_pointer = PC;
        PC = PC + edge;
    }

    bool EndJudge() {
        if (commit_node.is_end) {
            unsigned int out = register_down[10].value;
            out = out % (1 << 8);
            std::cout << out << std::endl;
            return true;
        }
        return false;
    }

public:
    Simulator() {}

    ~Simulator() {}

    void Init() {
        memset(Memory, 0, sizeof(Memory));
        PC = 0;
        op_count = 0;
        std::string input;
        while (!std::cin.eof()) {
            std::cin >> input;
            if (input[0] == '@') {
                memory_base = 0;
                for (int i = 1; i <= 8; i++) {
                    if (input[i] >= '0' && input[i] <= '9')
                        memory_base = memory_base * 16 + (input[i] - '0');
                    else
                        memory_base = memory_base * 16 + (input[i] - 'A' + 10);
                }
            } else {
                unsigned int now = 0;
                for (int i = 0; i < 2; i++) {
                    if (input[i] >= '0' && input[i] <= '9')
                        now = now * 16 + (input[i] - '0');
                    else
                        now = now * 16 + (input[i] - 'A' + 10);
                }
                MemoryWrite(0, now);
                memory_base++;
            }
        }
        memory_base = 0;
        PC = 0;
//        std::cerr << "Init Finished" << std::endl;
    }

//    bool committeed;
    void Debug(bool is_committed) {
        if (is_committed) {
            commit_cycle++;
            std::cout << commit_cycle << std::endl;
            for (int i = 0; i < 32; i++) {
                std::cout << register_up[i].rename << ' ';
            }
            std::cout << std::endl;
            for (int i = 0; i < 32; i++) {
                std::cout << register_up[i].value << ' ';
            }
            std::cout << std::endl;
        }
//        std::cout << rob_down.Size() << std::endl;
//        std::cout << rs_down.Spare() << std::endl;
    }

    unsigned int commit_cycle = 0;

    void Run() {
        while (true) {
//            cycle++;
//            std::cout << cycle << std::endl;
//           Debug();
            // 此间更换顺序执行不影响结果
            RunIQ();
            RunRS();
            RunLSB();
            RunROB();

            Update();
            RunExecute();
            RunIssue();
            RunCommit();
            RunRegister();
            if (EndJudge())
                break;
        }
//        std::cout << "Predicate num:" << success_num + fail_num << std::endl;
//        std::cout << "Success rate: " << double(success_num) / double(success_num + fail_num) << std::endl;
    }
};

#endif //SIMULATOR_SIMULATOR_H
