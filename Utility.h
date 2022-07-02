//
// Created by 86158 on 2022/6/27.
//

#ifndef SIMULATOR_UTILITY_H
#define SIMULATOR_UTILITY_H

const int k_size=32;

const unsigned int Mi[33] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072,
                             262144, 524288, 1048576, 2097152, 4194304, 8388608, 16777216, 33554432, 67108864,
                             134217728, 268435456, 536870912, 1073741824, 2147483648};

enum op_type_list {
    LUI = 0b0'000'0110111,
    AUIPC = 0b0'000'0010111,
    JAL = 0b0'000'1101111,
    JALR = 0b0'000'1100111,
    BEQ = 0b0'000'1100011,
    BNE = 0b0'001'1100011,
    BLT = 0b0'100'1100011,
    BGE = 0b0'101'1100011,
    BLTU = 0b0'110'1100011,
    BGEU = 0b0'111'1100011,
    LB = 0b0'000'0000011,
    LH = 0b0'001'0000011,
    LW = 0b0'010'0000011,
    LBU = 0b0'100'0000011,
    LHU = 0b0'101'0000011,
    SB = 0b0'000'0100011,
    SH = 0b0'001'0100011,
    SW = 0b0'010'0100011,
    ADDI = 0b0'000'0010011,
    SLTI = 0b0'010'0010011,
    SLTIU = 0b0'011'0010011,
    XORI = 0b0'100'0010011,
    ORI = 0b0'110'0010011,
    ANDI = 0b0'111'0010011,
    SLLI = 0b0'001'0010011,
    SRLI = 0b0'101'0010011,
    SRAI = 0b1'101'0010011,
    ADD = 0b0'000'0110011,
    SUB = 0b1'000'0110011,
    SLL = 0b0'001'0110011,
    SLT = 0b0'010'0110011,
    SLTU = 0b0'011'0110011,
    XOR = 0b0'100'0110011,
    SRL = 0b0'101'0110011,
    SRA = 0b1'101'0110011,
    OR = 0b0'110'0110011,
    AND = 0b0'111'0110011
};

inline int BiToDec(const std::string &str, int l, int r) {
    int ans = 0;
    for (int i = 0; i <= (r - l); i++) {
        ans = ans * 2 + int(str[l + i] - '0');
    }
    return ans;
}

inline std::string DecToBi(int value_in) {
    std::string ans = "";
    while (value_in > 0) {
        ans = char(value_in % 2 + int('0')) + ans;
        value_in = value_in / 2;
    }
    return ans;
}

inline std::string HexToBi(const std::string &str_in) {
    std::string ans = "";
    for (int i = str_in.length() - 1; i >= 0; i--) {
        char a = str_in[i];
        int in;
        if (!(a >= '0' && a <= '9'))
            in = 10 + (a - 'a');
        else
            in = a - '0';
        for (int j = 0; j < 4; j++) {
            ans = char(in % 2 + int('0')) + ans;
            in = in / 2;
        }
    }
    return ans;
}

inline std::string LeftComplement(const std::string &str_in) {
    char ans[32];
    for (int i = 0; i < 32 - str_in.length(); i++)
        ans[i] = str_in[0];
    for (int i = 1; i <= str_in.length(); i++)
        ans[32 - i] = str_in[str_in.length() - i];
    return std::string(ans).substr(0, 32);
}

std::string register_name[32] = {"zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0/fp", "s1", "a0", "a1", "a2",
                                 "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10",
                                 "s11", "t3", "t4", "t5", "t6"};
#endif //SIMULATOR_UTILITY_H
