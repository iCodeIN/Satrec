//
// Satrec
// Copyright 2022 Wenting Zhang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "config.h"
#include "util.h"
#include "rom.h"
#include "disasm.h"

// Use directly with a, field beyond 7 are used internally
char *field[9] = {"P", "WP", "XS", "X", "S", "M", "B", "W", "A"};
#define F_P     0
#define F_WP    1
#define F_XS    2
#define F_X     3
#define F_S     4
#define F_M     5
#define F_B     6
#define F_W     7
#define F_A     8

#define SET_INFO(m,l) { strcpy(instr->disasm, m); instr->length = l; }
#define SET_INFO_A(m,v,l) { sprintf(instr->disasm, "%s %s", m, field[v]); instr->length = l; }
#define SET_INFO_X(m,v,l) { sprintf(instr->disasm, "%s %d", m, v + 1); instr->length = l; }
#define SET_INFO_N(m,v,l) { sprintf(instr->disasm, "%s %d", m, v); instr->length = l; }
#define SET_INFO_H(m,v,n,l) { sprintf(ftemp, "%%s %%0%dlX", n); sprintf(instr->disasm, ftemp, m, v); instr->length = l; }
#define SET_INFO_S2(m,v,l) { print_godst8(ftemp, v, l-2); sprintf(instr->disasm, "%s %s", m, ftemp); instr->length = l; }
#define SET_INFO_S(m,v,l) { print_s16(ftemp, v); sprintf(instr->disasm, "%s %s", m, ftemp); instr->length = l; }
#define SET_INFO_AH2(m,v1,v2,l) { sprintf(instr->disasm, "%s %s %02lX", m, field[v1], v2); instr->length = l; }
#define SET_INFO_AS2(m,v1,v2,l) { print_godst8(ftemp, v2, l-2); sprintf(instr->disasm, "%s %s %s", m, field[v1], ftemp); instr->length = l; }
#define SET_INFO_XH2(m,v1,v2,l) { sprintf(instr->disasm, "%s %d %02lX", m, v1 + 1, v2); instr->length = l; }
#define SET_INFO_NH2(m,v1,v2,l) { sprintf(instr->disasm, "%s %d %02lX", m, v1, v2); instr->length = l; }
#define SET_INFO_NS2(m,v1,v2,l) { print_godst8(ftemp, v2, l-2); sprintf(instr->disasm, "%s %d %s", m, v1, ftemp); instr->length = l; }
#define ILLEGAL_INSN() { strcpy(instr->disasm, "Illegal"); instr->length = 1; }

static uint64_t get_imm(DISASM *instr, int offset, int length) {
    uint8_t *op_ptr = &(instr->opcode[offset]);
    uint64_t imm = 0;
    for (int i = 0; i < length; i++) {
        imm |= (*op_ptr++) << (i * 4);
    }
    return imm;
}

// Print 8 bit signed immediate used in relative GOYES/ RTNYES
static void print_godst8(char *dst, uint8_t value, int offset) {
    if (value == 0)
        sprintf(dst, "RTNYES");
    else {
        int val = (int)((int8_t)value);
        val += offset; // Relative to the PC when it's decoded
        if (val < 0)
            sprintf(dst, "GOYES -%X", -val);
        else
            sprintf(dst, "GOYES +%X", val);
    }
}

static int get_s16(uint64_t value) {
    uint16_t v16 = value & 0xffff;
    int val = (int)((int16_t)v16);
    return val;
}

static int get_s12(uint64_t value) {
    int16_t val = value;
    val <<= 4;
    val >>= 4;
    return (int)val;
}

static void print_s16(char *dst, int value) {
    if (value < 0)
        sprintf(dst, "-%X", -value);
    else
        sprintf(dst, "+%X", value);
}

void disasm(DISASM *instr, uint32_t pc) {
    char *ptemp;
    char ftemp[20];
    uint8_t ntemp;
    for (int i = 0; i < INSTR_MAX_LENGTH; i++)
        instr->opcode[i] = rom_read(pc++);
    switch (instr->opcode[0]) {
    case 0x0: // Misc operations
        switch (instr->opcode[1]) {
        case 0x0: SET_INFO("RTNSXM", 2); break;
        case 0x1: SET_INFO("RTN", 2); break;
        case 0x2: SET_INFO("RTNSC", 2); break;
        case 0x3: SET_INFO("RTNCC", 2); break;
        case 0x4: SET_INFO("SETHEX", 2); break;
        case 0x5: SET_INFO("SETDEC", 2); break;
        case 0x6: SET_INFO("RSTK=C", 2); break;
        case 0x7: SET_INFO("C=RSTK", 2); break;
        case 0x8: SET_INFO("CLRST", 2); break;
        case 0x9: SET_INFO("C=ST", 2); break;
        case 0xA: SET_INFO("ST=C", 2); break;
        case 0xB: SET_INFO("CSTEX", 2); break;
        case 0xC: SET_INFO("P=P+1", 2); break;
        case 0xD: SET_INFO("P=P-1", 2); break;
        case 0xE:
            switch (instr->opcode[3]) {
            case 0x0: SET_INFO_A("A=A&B", instr->opcode[2], 4); break;
            case 0x1: SET_INFO_A("B=B&C", instr->opcode[2], 4); break;
            case 0x2: SET_INFO_A("C=C&A", instr->opcode[2], 4); break;
            case 0x3: SET_INFO_A("D=D&C", instr->opcode[2], 4); break;
            case 0x4: SET_INFO_A("B=B&A", instr->opcode[2], 4); break;
            case 0x5: SET_INFO_A("C=C&B", instr->opcode[2], 4); break;
            case 0x6: SET_INFO_A("A=A&C", instr->opcode[2], 4); break;
            case 0x7: SET_INFO_A("C=C&D", instr->opcode[2], 4); break;
            case 0x8: SET_INFO_A("A=A!B", instr->opcode[2], 4); break;
            case 0x9: SET_INFO_A("B=B!C", instr->opcode[2], 4); break;
            case 0xA: SET_INFO_A("C=C!A", instr->opcode[2], 4); break;
            case 0xB: SET_INFO_A("D=D!C", instr->opcode[2], 4); break;
            case 0xC: SET_INFO_A("B=B!A", instr->opcode[2], 4); break;
            case 0xD: SET_INFO_A("C=C!B", instr->opcode[2], 4); break;
            case 0xE: SET_INFO_A("A=A!C", instr->opcode[2], 4); break;
            case 0xF: SET_INFO_A("C=C!D", instr->opcode[2], 4); break;
            }
            break;
        case 0xF: SET_INFO("RTI", 2); break;
        }
    break;
    case 0x1: // Data movement
        switch (instr->opcode[1]) {
        case 0x0:
            switch (instr->opcode[2]) {
            case 0x0: SET_INFO("R0=A", 3); break;
            case 0x1: SET_INFO("R1=A", 3); break;
            case 0x2: SET_INFO("R2=A", 3); break;
            case 0x3: SET_INFO("R3=A", 3); break;
            case 0x4: SET_INFO("R4=A", 3); break;
            case 0x8: SET_INFO("R0=C", 3); break;
            case 0x9: SET_INFO("R1=C", 3); break;
            case 0xA: SET_INFO("R2=C", 3); break;
            case 0xB: SET_INFO("R3=C", 3); break;
            case 0xC: SET_INFO("R4=C", 3); break;
            default: ILLEGAL_INSN(); break;
            }
            break;
        case 0x1:
            switch (instr->opcode[2]) {
            case 0x0: SET_INFO("A=R0", 3); break;
            case 0x1: SET_INFO("A=R1", 3); break;
            case 0x2: SET_INFO("A=R2", 3); break;
            case 0x3: SET_INFO("A=R3", 3); break;
            case 0x4: SET_INFO("A=R4", 3); break;
            case 0x8: SET_INFO("C=R0", 3); break;
            case 0x9: SET_INFO("C=R1", 3); break;
            case 0xA: SET_INFO("C=R2", 3); break;
            case 0xB: SET_INFO("C=R3", 3); break;
            case 0xC: SET_INFO("C=R4", 3); break;
            default: ILLEGAL_INSN(); break;
            }
            break;
        case 0x2:
            switch (instr->opcode[2]) {
            case 0x0: SET_INFO("AR0EX", 3); break;
            case 0x1: SET_INFO("AR1EX", 3); break;
            case 0x2: SET_INFO("AR2EX", 3); break;
            case 0x3: SET_INFO("AR3EX", 3); break;
            case 0x4: SET_INFO("AR4EX", 3); break;
            case 0x8: SET_INFO("CR0EX", 3); break;
            case 0x9: SET_INFO("CR1EX", 3); break;
            case 0xA: SET_INFO("CR2EX", 3); break;
            case 0xB: SET_INFO("CR3EX", 3); break;
            case 0xC: SET_INFO("CR4EX", 3); break;
            default: ILLEGAL_INSN(); break;
            }
            break;
        case 0x3:
            switch (instr->opcode[2]) {
            case 0x0: SET_INFO("D0=A", 3); break;
            case 0x1: SET_INFO("D1=A", 3); break;
            case 0x2: SET_INFO("AD0EX", 3); break;
            case 0x3: SET_INFO("AD1EX", 3); break;
            case 0x4: SET_INFO("D0=C", 3); break;
            case 0x5: SET_INFO("D1=C", 3); break;
            case 0x6: SET_INFO("CD0EX", 3); break;
            case 0x7: SET_INFO("CD1EX", 3); break;
            case 0x8: SET_INFO("D0=AS", 3); break;
            case 0x9: SET_INFO("D1=AS", 3); break;
            case 0xA: SET_INFO("AD0XS", 3); break;
            case 0xB: SET_INFO("AD1XS", 3); break;
            case 0xC: SET_INFO("D0=CS", 3); break;
            case 0xD: SET_INFO("D1=CS", 3); break;
            case 0xE: SET_INFO("CD0XS", 3); break;
            case 0xF: SET_INFO("CD1XS", 3); break;
            }
            break;
        case 0x4:
        case 0x5:
            switch (instr->opcode[2] & 0x7) {
            case 0x0: ptemp = "DAT0=A"; break;
            case 0x1: ptemp = "DAT1=A"; break;
            case 0x2: ptemp = "A=DAT0"; break;
            case 0x3: ptemp = "A=DAT1"; break;
            case 0x4: ptemp = "DAT0=C"; break;
            case 0x5: ptemp = "DAT1=C"; break;
            case 0x6: ptemp = "C=DAT0"; break;
            case 0x7: ptemp = "C=DAT1"; break;
            }
            if (instr->opcode[1] == 0x4) {
                if (!(instr->opcode[2] & 0x8)) {
                    SET_INFO_A(ptemp, F_A, 3);
                }
                else {
                    SET_INFO_A(ptemp, F_B, 3);
                }
            }
            else if (instr->opcode[1] == 0x5) {
                if (!(instr->opcode[2] & 0x8)) {
                    SET_INFO_A(ptemp, instr->opcode[3], 4);
                }
                else {
                    SET_INFO_X(ptemp, instr->opcode[3], 4);
                }
            }
            break;
        case 0x6: SET_INFO_X("D0=D0+", instr->opcode[2], 3); break;
        case 0x7: SET_INFO_X("D1=D1+", instr->opcode[2], 3); break;
        case 0x8: SET_INFO_X("D0=D0-", instr->opcode[2], 3); break;
        case 0x9: SET_INFO_H("D0=HEX", get_imm(instr, 2, 2), 2, 4); break;
        case 0xA: SET_INFO_H("D0=HEX", get_imm(instr, 2, 4), 4, 6); break;
        case 0xB: SET_INFO_H("D0=HEX", get_imm(instr, 2, 5), 5, 7); break;
        case 0xC: SET_INFO_X("D1=D1-", instr->opcode[2], 3); break;
        case 0xD: SET_INFO_H("D1=HEX", get_imm(instr, 2, 2), 2, 4); break;
        case 0xE: SET_INFO_H("D1=HEX", get_imm(instr, 2, 4), 4, 6); break;
        case 0xF: SET_INFO_H("D1=HEX", get_imm(instr, 2, 5), 5, 7); break;
        }
    break;
    case 0x2: SET_INFO_N("P=", instr->opcode[1], 2); break;
    case 0x3: // LC
        ntemp = instr->opcode[1] + 1;
        SET_INFO_H("LCHEX", get_imm(instr, 2, ntemp), ntemp, 2 + ntemp);
        break;
    case 0x4:
        if ((instr->opcode[1] == 0) && (instr->opcode[2] == 0)) {
            SET_INFO("RTNC", 3);
        }
        else {
            SET_INFO_H("GOC", get_imm(instr, 1, 2) + 1, 2, 3);
        }
        break;
    case 0x5:
        if ((instr->opcode[1] == 0) && (instr->opcode[2] == 0)) {
            SET_INFO("RTNNC", 3);
        }
        else {
            SET_INFO_H("GONC", get_imm(instr, 1, 2) + 1, 2, 3);
        }
        break;
    case 0x6: SET_INFO_S("GOTO", get_s12(get_imm(instr, 1, 3)) + 1, 4); break;
    case 0x7: SET_INFO_S("GOSUB", get_s12(get_imm(instr, 1, 3)) + 4, 4); break;
    case 0x8:
        switch (instr->opcode[1]) {
        case 0x0:
            switch (instr->opcode[2]) {
            case 0x0: SET_INFO("OUT=CS", 3); break;
            case 0x1: SET_INFO("OUT=C", 3); break;
            case 0x2: SET_INFO("A=IN", 3); break;
            case 0x3: SET_INFO("C=IN", 3); break;
            case 0x4: SET_INFO("UNCNFG", 3); break;
            case 0x5: SET_INFO("CONFIG", 3); break;
            case 0x6: SET_INFO("C=ID", 3); break;
            case 0x7: SET_INFO("SHUTDN", 3); break;
            case 0x8:
                switch (instr->opcode[3]) {
                case 0x0: SET_INFO("INTON", 4); break;
                case 0x1:
                    if (instr->opcode[4] == 0x0) {
                        SET_INFO("RSI", 5);
                    }
                    else {
                        ILLEGAL_INSN();
                    }
                    break;
                case 0x2:
                    ntemp = instr->opcode[4] + 1;
                    SET_INFO_H("LAHEX", get_imm(instr, 5, ntemp), ntemp, 5 + ntemp);
                    break;
                case 0x3: SET_INFO("BUSCB", 4); break; // WARNING: This shouldn't appear
                case 0x4: SET_INFO_N("ABIT=0", instr->opcode[4], 5); break;
                case 0x5: SET_INFO_N("ABIT=1", instr->opcode[4], 5); break;
                case 0x6: SET_INFO_NS2("?ABIT=0", instr->opcode[4], get_imm(instr, 5, 2), 7); break;
                case 0x7: SET_INFO_NS2("?ABIT=1", instr->opcode[4], get_imm(instr, 5, 2), 7); break;
                case 0x8: SET_INFO_N("CBIT=0", instr->opcode[4], 5); break;
                case 0x9: SET_INFO_N("CBIT=1", instr->opcode[4], 5); break;
                case 0xA: SET_INFO_NS2("?CBIT=0", instr->opcode[4], get_imm(instr, 5, 2), 7); break;
                case 0xB: SET_INFO_NS2("?CBIT=1", instr->opcode[4], get_imm(instr, 5, 2), 7); break;
                case 0xC: SET_INFO("PC=(A)", 4); break;
                case 0xD: SET_INFO("BUSCD", 4); break; // WARNING: This shouldn't appear
                case 0xF: SET_INFO("INTOFF", 4); break;
                default: ILLEGAL_INSN(); break;
                }
                break;
            case 0x9: SET_INFO("C+P+1", 3); break;
            case 0xA: SET_INFO("RESET", 3); break;
            case 0xB: SET_INFO("BUSCC", 3); break; // WARNING: This is probably a prefix to Saturn+
            case 0xC: SET_INFO_N("C=P", instr->opcode[3], 4); break;
            case 0xD: SET_INFO_N("P=C", instr->opcode[3], 4); break;
            case 0xE: SET_INFO("SREQ", 3); break;
            case 0xF: SET_INFO_N("CPEX", instr->opcode[3], 4); break;
            }
            break;
        case 0x1:
            switch (instr->opcode[2]) {
            case 0x0: SET_INFO("ASLC", 3); break;
            case 0x1: SET_INFO("BSLC", 3); break;
            case 0x2: SET_INFO("CSLC", 3); break;
            case 0x3: SET_INFO("DSLC", 3); break;
            case 0x4: SET_INFO("ASRC", 3); break;
            case 0x5: SET_INFO("BSRC", 3); break;
            case 0x6: SET_INFO("CSRC", 3); break;
            case 0x7: SET_INFO("DSRC", 3); break;
            case 0xC: SET_INFO("ASRB", 3); break;
            case 0xD: SET_INFO("BSRB", 3); break;
            case 0xE: SET_INFO("CSRB", 3); break;
            case 0xF: SET_INFO("DSRB", 3); break;
            default: ILLEGAL_INSN(); break;
            }
            break;
        case 0x2:
            switch (instr->opcode[2]) {
            case 0x1: SET_INFO("XM=0", 3); break;
            case 0x2: SET_INFO("SB=0", 3); break;
            case 0x4: SET_INFO("SR=0", 3); break;
            case 0x8: SET_INFO("MP=0", 3); break;
            case 0xF: SET_INFO("CLRHST", 3); break;
            default: SET_INFO_N("CLRHSTBM", instr->opcode[2], 3); break;
            }
            break;
        case 0x3:
            switch (instr->opcode[2]) {
            case 0x1: SET_INFO_S2("?XM=0", get_imm(instr, 3, 2), 5); break;
            case 0x2: SET_INFO_S2("?SB=0", get_imm(instr, 3, 2), 5); break;
            case 0x4: SET_INFO_S2("?SR=0", get_imm(instr, 3, 2), 5); break;
            case 0x8: SET_INFO_S2("?MP=0", get_imm(instr, 3, 2), 5); break;
            default: ILLEGAL_INSN(); break;
            }
            break;
        case 0x4: SET_INFO_N("ST=0", instr->opcode[2], 3); break;
        case 0x5: SET_INFO_N("ST=1", instr->opcode[2], 3); break;
        case 0x6: SET_INFO_NS2("?ST=0", instr->opcode[2], get_imm(instr, 3, 2), 5); break;
        case 0x7: SET_INFO_NS2("?ST=1", instr->opcode[2], get_imm(instr, 3, 2), 5); break;
        case 0x8: SET_INFO_NS2("?P#", instr->opcode[2], get_imm(instr, 3, 2), 5); break;
        case 0x9: SET_INFO_NS2("?P=", instr->opcode[2], get_imm(instr, 3, 2), 5); break;
        // 0xA and 0xB are handled together with 0x9
        case 0xA:
        case 0xB:
            goto disasm_condbranch;
        case 0xC: SET_INFO_S("GOLONG", get_s16(get_imm(instr, 2, 4)) + 2, 6); break;
        case 0xD: SET_INFO_H("GOVLNG", get_imm(instr, 2, 5), 5, 7); break;
        case 0xE: SET_INFO_S("GOSUBL", get_s16(get_imm(instr, 2, 4)) + 6, 6); break;
        case 0xF: SET_INFO_H("GOSBVL", get_imm(instr, 2, 5), 5, 7); break;
        }
        break;
    case 0x9:
    disasm_condbranch:
        if (((instr->opcode[0] == 0x8) && (instr->opcode[1] == 0xA)) || 
                ((instr->opcode[0] == 0x9) && !(instr->opcode[1] & 0x8))) {
            switch (instr->opcode[2]) {
            case 0x0: ptemp = "?A=B"; break;
            case 0x1: ptemp = "?B=C"; break;
            case 0x2: ptemp = "?A=C"; break;
            case 0x3: ptemp = "?C=D"; break;
            case 0x4: ptemp = "?A#B"; break;
            case 0x5: ptemp = "?B#C"; break;
            case 0x6: ptemp = "?A#C"; break;
            case 0x7: ptemp = "?C#D"; break;
            case 0x8: ptemp = "?A=0"; break;
            case 0x9: ptemp = "?B=0"; break;
            case 0xA: ptemp = "?C=0"; break;
            case 0xB: ptemp = "?D=0"; break;
            case 0xC: ptemp = "?A#0"; break;
            case 0xD: ptemp = "?B#0"; break;
            case 0xE: ptemp = "?C#0"; break;
            case 0xF: ptemp = "?D#0"; break;
            }
        }
        else {
            switch (instr->opcode[2]) {
            case 0x0: ptemp = "?A>B"; break;
            case 0x1: ptemp = "?B>C"; break;
            case 0x2: ptemp = "?C>A"; break;
            case 0x3: ptemp = "?D>C"; break;
            case 0x4: ptemp = "?A<B"; break;
            case 0x5: ptemp = "?B<C"; break;
            case 0x6: ptemp = "?C<A"; break;
            case 0x7: ptemp = "?D<C"; break;
            case 0x8: ptemp = "?A>=B"; break;
            case 0x9: ptemp = "?B>=C"; break;
            case 0xA: ptemp = "?C>=A"; break;
            case 0xB: ptemp = "?D>=C"; break;
            case 0xC: ptemp = "?A<=B"; break;
            case 0xD: ptemp = "?B<=C"; break;
            case 0xE: ptemp = "?C<=A"; break;
            case 0xF: ptemp = "?D<=C"; break;
            }
        }
        if (instr->opcode[0] == 0x8) {
            SET_INFO_AS2(ptemp, F_A, get_imm(instr, 3, 2), 5);
        }
        else {
            SET_INFO_AS2(ptemp, instr->opcode[1] & 0x7, get_imm(instr, 3, 2), 5);
        }
        break;
    case 0xA:
    case 0xC:
    case 0xD:
        if (instr->opcode[0] == 0xA)
            ntemp = instr->opcode[2];
        else
            ntemp = instr->opcode[1];
        if (((instr->opcode[0] == 0xA) && !(instr->opcode[1] & 0x8)) || 
                (instr->opcode[0] == 0xC)) {
            switch (ntemp) {
            case 0x0: ptemp = "A=A+B"; break;
            case 0x1: ptemp = "B=B+C"; break;
            case 0x2: ptemp = "C=C+A"; break;
            case 0x3: ptemp = "D=D+C"; break;
            case 0x4: ptemp = "A=A+A"; break;
            case 0x5: ptemp = "B=B+B"; break;
            case 0x6: ptemp = "C=C+C"; break;
            case 0x7: ptemp = "D=D+D"; break;
            case 0x8: ptemp = "B=B+A"; break;
            case 0x9: ptemp = "C=C+B"; break;
            case 0xA: ptemp = "A=A+C"; break;
            case 0xB: ptemp = "C=C+D"; break;
            case 0xC: ptemp = "A=A-1"; break;
            case 0xD: ptemp = "B=B-1"; break;
            case 0xE: ptemp = "C=C-1"; break;
            case 0xF: ptemp = "D=D-1"; break;
            }
        }
        else {
            switch (ntemp) {
            case 0x0: ptemp = "A=0"; break;
            case 0x1: ptemp = "B=0"; break;
            case 0x2: ptemp = "C=0"; break;
            case 0x3: ptemp = "D=0"; break;
            case 0x4: ptemp = "A=B"; break;
            case 0x5: ptemp = "B=C"; break;
            case 0x6: ptemp = "C=A"; break;
            case 0x7: ptemp = "D=C"; break;
            case 0x8: ptemp = "B=A"; break;
            case 0x9: ptemp = "C=B"; break;
            case 0xA: ptemp = "A=C"; break;
            case 0xB: ptemp = "C=D"; break;
            case 0xC: ptemp = "ABEX"; break;
            case 0xD: ptemp = "BCEX"; break;
            case 0xE: ptemp = "ACEX"; break;
            case 0xF: ptemp = "CDEX"; break;
            }
        }
        if (instr->opcode[0] == 0xA) {
            SET_INFO_A(ptemp, instr->opcode[1] & 0x7, 3);
        }
        else {
            SET_INFO_A(ptemp, F_A, 2);
        }
        break;
    case 0xB:
    case 0xE:
    case 0xF:
        if (instr->opcode[0] == 0xB)
            ntemp = instr->opcode[2];
        else
            ntemp = instr->opcode[1];
        if (((instr->opcode[0] == 0xB) && !(instr->opcode[1] & 0x8)) || 
                (instr->opcode[0] == 0xE)) {
            switch (ntemp) {
            case 0x0: ptemp = "A=A-B"; break;
            case 0x1: ptemp = "B=B-C"; break;
            case 0x2: ptemp = "C=C-A"; break;
            case 0x3: ptemp = "D=D-C"; break;
            case 0x4: ptemp = "A=A+1"; break;
            case 0x5: ptemp = "B=B+1"; break;
            case 0x6: ptemp = "C=C+1"; break;
            case 0x7: ptemp = "D=D+1"; break;
            case 0x8: ptemp = "B=B-A"; break;
            case 0x9: ptemp = "C=C-B"; break;
            case 0xA: ptemp = "A=A-C"; break;
            case 0xB: ptemp = "C=C-D"; break;
            case 0xC: ptemp = "A=B-A"; break;
            case 0xD: ptemp = "B=C-B"; break;
            case 0xE: ptemp = "C=A-C"; break;
            case 0xF: ptemp = "D=C-D"; break;
            }
        }
        else {
            switch (ntemp) {
            case 0x0: ptemp = "ASL"; break;
            case 0x1: ptemp = "BSL"; break;
            case 0x2: ptemp = "CSL"; break;
            case 0x3: ptemp = "DSL"; break;
            case 0x4: ptemp = "ASR"; break;
            case 0x5: ptemp = "BSR"; break;
            case 0x6: ptemp = "CSR"; break;
            case 0x7: ptemp = "DSR"; break;
            case 0x8: ptemp = "A=-A"; break;
            case 0x9: ptemp = "B=-B"; break;
            case 0xA: ptemp = "C=-C"; break;
            case 0xB: ptemp = "D=-D"; break;
            case 0xC: ptemp = "A=-A-1"; break;
            case 0xD: ptemp = "B=-B-1"; break;
            case 0xE: ptemp = "C=-C-1"; break;
            case 0xF: ptemp = "D=-D-1"; break;
            }
        }
        if (instr->opcode[0] == 0xB) {
            SET_INFO_A(ptemp, instr->opcode[1] & 0x7, 3);
        }
        else {
            SET_INFO_A(ptemp, F_A, 2);
        }
        break;
    }
}