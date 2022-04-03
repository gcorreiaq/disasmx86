#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "opcodes.h"
#define MOD_RM 0
#define SIB 1
#define CHECK_ONLY 1
#define NO_OP_LEN 2

typedef struct {
    char *prefix[2];
    char *opcode;
    char *addr_size;
    char *segment;
    char *args[3];
} opcode_format;

int check_prefix(unsigned char *base, opcode_gathered_info *op_info, int mode);

int toupper_string(char *str, int str_len)
{
    str_len--;
    for (;str_len >= 0;str_len--) str[str_len] = toupper(str[str_len]);
}

int opcode_arg_count_ex(opcode_extension_struct *opcode) // RETURN QUANTITY OF ARGUMENTS PRESENT FOR THE INSTRUCTION EX
{
    for (int i = 0;i < 3;i++)
    {
        if (opcode->args[i].arg_1_1 != 0) 
            continue;
        else return i;
    }
    return 3;
}

int opcode_arg_count(opcode_info_struct *opcode) // RETURN QUANTITY OF ARGUMENTS PRESENT FOR THE INSTRUCTION
{
    for (int i = 0;i < 3;i++)
    {
        if ((opcode->args[i].arg_1_1 != 0) && (opcode->args[i].arg_1_1 != K))
            continue;
        else return i;
    }
    return 3;
}

int displacement_calc(opcode_mod_rm *mod_rm, int addr_size) // RETURN WIDTH OF DISPLACEMENT OF THE INSTRUCTION (b, w, d)
{
    switch(mod_rm->mod)
    {
        case 0:
            if (mod_rm->rm == (5 + addr_size)) // IF addr_size = 1, RM MUST BE 6
            {
                if (addr_size) return w;
                else return d;
            }
            else return 0;
        case 1:
            return b;
        case 2:
            if (addr_size) return w;
            else return d;
        default:
            return 0;
    }
}

int fill_mod_rm(unsigned char *mod_rm, opcode_gathered_info *op_info, int addr_size) // FILL op_info WITH MOD_RM INFORMATION
{
    int opcode_len = 1; // INITIAL VALUE FOR MOD_RM
    op_info->has_sib = 0;
    op_info->has_mod_rm = 1;
    op_info->mod_rm.mod = ((mod_rm[0] & 0xc0) >> 6);
    op_info->mod_rm.reg = ((mod_rm[0] & 0x38) >> 3);
    op_info->mod_rm.rm = mod_rm [0] & 0x7;

    if ( ((op_info->mod_rm.mod != 3) && (op_info->mod_rm.rm == 4)) && (addr_size != 1)) // CHECK THE EXISTENCE OF SIB BYTE
    {
        opcode_len++; // SIB
        op_info->has_sib = 1;
        op_info->sib.mod = ((mod_rm[1] & 0xc0) >> 6);
        op_info->sib.rm = ((mod_rm[1] & 0x38) >> 3);
        op_info->sib.reg = mod_rm[1] & 0x7;
        if (op_info->mod_rm.mod == 0 && op_info->sib.reg == 5)
            opcode_len += 4; // DISPLACEMENT
    }
    switch(displacement_calc(&op_info->mod_rm, addr_size)) // CHECK THE EXISTENCE OF DISPLACEMENT
    {
        case b:
            opcode_len += 1;
            break;
        case w:
            opcode_len += 2;
            break;
        case d:
            opcode_len += 4;
            break;
    }
    op_info->opcode_len += opcode_len;
    return 1;
}

int op_size_calc(int arg, int op_size) // RETURN byte, word, dword, qword, fword
{
    if (((arg == v || arg == z) && op_size) || (arg == w)) // IF OPERAND SIZE PREFIX IS TRUE, return WORD
        return WORD;
    else if (arg == p) // EG. FAR CALLS
        if (op_size) 
            return DWORD;
        else 
            return FWORD;
    else if (arg == q) 
        return QWORD;
    else if (arg == b) 
        return BYTE;
    return DWORD;
}

int segment_calc(unsigned char *base, int prefix_count) // RETURN d FOR PRESENT SEGMENT PREFIX
{
    char seg_prefix[] = "\x2e\x36\x3e\x26\x64\x65";
    for (int i = 0;i < prefix_count;i++)
        for (int d = 0;d < 6;d++)
            if (base[i] == seg_prefix[d]) 
                return d;
    return -1;
}
int strncat_args(char *buf, int buf_size, int data, int data_len, int mode) // PARSE AND STORE INFO ABOUT ARGUMENT IN buf
{
    const char **p = NULL;
    int disp = 0, adjust = 0xFFFFFFFF;
    switch(mode)
    {
        case GENERAL_REG: // IF GENERAL_REG, data = index of some general register
            if (data_len == b) p = general_registers_table[GENERAL_REG_B];
            else if (data_len == w) p = general_registers_table[GENERAL_REG_W];
            else p = general_registers_table[GENERAL_REG];
            strncat(buf, p[data], buf_size);
            break;
        case IMMEDIATE:
        // SAME CASE OF DISPLACEMENT
        case DISPLACEMENT:
            disp = data;
            if (data_len == b) adjust = 0xFF;
            else if (data_len == w) adjust = 0xFFFF;
            disp &= adjust;
            if ((disp == 0) && (mode != IMMEDIATE))
                break;
            if (mode != IMMEDIATE)
            {
                if ((data_len == b && (disp & 0x80)) || (data_len == w && (disp & 0x8000)) || (disp & 0x80000000))
                {   //NEGATIVE
                    strncat(buf, "-", buf_size);
                    disp = ~disp;
                    disp &= adjust;
                    disp++;
                }
                else strncat(buf, "+", buf_size);
            }
            itoa(disp, &buf[strlen(buf)], 16);
            break;
        case GENERAL_REG_16:
            strncat(buf, general_registers_16[data], buf_size);
            break;
    }
    return 1;
}
int strncat_sib(char *buf, unsigned char *sib_p, opcode_gathered_info *op_info, int buf_size) // PRINTS INFORMATION RELATED TO SIB. Eg DISPLACEMENT IN SIB
{
    opcode_mod_rm *mod_rm = &op_info->mod_rm;
    opcode_mod_rm *sib = &op_info->sib;
    int seg_calc = segment_calc(&sib_p[0 - (op_info->prefix_count + op_info->opcode_count + 1)], op_info->prefix_count);
    if (sib->reg != 5) // IF NOT DISPLACEMENT
    {
        if (sib->reg == 4 && seg_calc == -1) // REG = ESP
            memcpy(&buf[strlen(buf) - 4], segment_registers[SS], 2);
        strncat(buf, general_registers[sib->reg], buf_size);
    }
    else // sib->reg == 5
    {
        if  (mod_rm->mod != 0) // THEN ADDS EBP
        {
            strncat(buf, general_registers[EBP], buf_size);
            if (seg_calc == -1)
                memcpy(&buf[strlen(buf) - 7], segment_registers[SS], 2);
            if (sib->rm != 4) strncat(buf, "+", buf_size);
        }
        if (sib->rm != 4)
        {
            strncat(buf, general_registers[sib->rm], buf_size);
            strncat(buf, scaled_index[sib->mod], buf_size);
        }
        if (mod_rm->mod == 0)
        {
            int disp = *(int *)(&sib_p[1]);
            if (disp == 0) return 1;
            if (sib->rm != 4){
                if (disp & 0x8) strncat(buf, "-", buf_size); // LITTLE ENDIAN
                else strncat(buf, "+", buf_size);
            }
            itoa(disp, &buf[strlen(buf)], 16);
        }
        return 1;
    }
    if (sib->rm == 4) return 1;
    strncat(buf, "+", buf_size);
    strncat(buf, general_registers[sib->rm], buf_size);
    strncat(buf, scaled_index[sib->mod], buf_size);
    return 1;
}

int check_prefix_op(unsigned char *base, int prefix_count, char op) // SEARCH PRESENCE OF PREFIX GIVEN BY op
{
    for (int i = 0;i < prefix_count;i++)
        if (base[i] == op) return 1;
    return 0;
}

int check_prefix_gp(unsigned char *base)
{
    opcode_gathered_info op_info;
    int ret = 0, i = 0, prefix = 0;
    unsigned char prefix_char = *base++;
    while(1)
    {
        if (base[i] == prefix_char)
            return INV;
        ret = check_prefix(&base[i++], &op_info, NO_OP_LEN);
        if ((ret == NO_PREFIX) || (ret == INV)) 
            break;
    }
    if (prefix_char == '\xf3')
    {
        prefix = REP;
        switch((char)base[i])
        {
            case '\xae':
                prefix = REPE;
                break;
            case '\xaf':
                prefix = REPE;
                break;
            case '\xa6':
                prefix = REPE;
                break;
            case '\xa7':
                prefix = REPE;
                break;
            case '\x6c':
                break;
            case '\x6d':
                break;
            case '\xa4':
                break;
            case '\xa5':
                break;
            case '\x6e':
                break;
            case '\x6f':
                break;
            case '\xaa':
                break;
            case '\xab':
                break;
            case '\xac':
                break;
            case '\xad':
                break;
            default:
                prefix = INV;
                break;
        }
    }
    else if (prefix_char == '\xf2')
    {
        prefix = REPNE;
        switch((char)base[i])
        {
            case '\xa6':
                break;
            case '\xa7':
                break;
            case '\xae':
                break;
            case '\xaf':
                break;
            default:
                prefix = INV;
        }
    }
    return prefix;
}

int check_prefix(unsigned char *base, opcode_gathered_info *op_info, int mode) // RETURN INV IF PREFIX_G1, RETURN NO_PREFIX FOR NO PREFIX
{
    int prefix_table = 0, prefix = 0;
    if (mode != CHECK_ONLY) 
        op_info->prefix = 0;
    switch ((char)base[0])
    {
        case '\xf0':
            prefix_table = PREFIX_G1;
            prefix = LOCK;
            if (check_prefix_gp(base) == INV)
            {
                op_info->prefix_table = PREFIX_G1;
                op_info->prefix = LOCK;
                return INV;
            }
            break;
        case '\xf2':
            prefix_table = PREFIX_G1;
            prefix = check_prefix_gp(base);
            if (prefix == INV)
            {
                op_info->prefix_table = PREFIX_G1;
                op_info->prefix = REPNE;
                return INV;
            }
            break;
        case '\xf3':
            prefix_table = PREFIX_G1;
            prefix = check_prefix_gp(base);
            if (prefix == INV)
            {
                op_info->prefix_table = PREFIX_G1;
                op_info->prefix = REP;
                return INV;
            }
            break;
        case '\x2e':
            prefix_table = PREFIX_G2;
            prefix = CS;
            break;
        case '\x36':
            prefix_table = PREFIX_G2;
            prefix = SS;
            break;
        case '\x3e':
            prefix_table = PREFIX_G2;
            prefix = DS;
            break;
        case '\x26':
            prefix_table = PREFIX_G2;
            prefix = ES;
            break;
        case '\x64':
            prefix_table = PREFIX_G2;
            prefix = FS;
            break;
        case '\x65':
            prefix_table = PREFIX_G2;
            prefix = GS;
            break;
        case '\x66':
            prefix_table = PREFIX_G3;
            break;
        case '\x67':
            prefix_table = PREFIX_G4;
            break;
        default:
            prefix_table = NO_PREFIX;
            break;
    }
    if (mode != CHECK_ONLY)
    {
        op_info->prefix_table = prefix_table;
        op_info->prefix = prefix;
    }
    if ((prefix_table != NO_PREFIX) && (mode != NO_OP_LEN)) // NO_OP_LEN -> DON'T CHANGE opcode_len
        op_info->opcode_len++;
    return prefix_table;
}

int check_opcode_table(unsigned char *base, opcode_gathered_info *op_info) // fill opcode table
{
    opcode_info_struct *op_struct;
    int offset = 0, 
    table_array_size = 0;
    base += op_info->opcode_len;
    op_info->opcode = 0;
    op_info->opcode_table = OP1_TABLE;
    op_info->opcode_count = 0;
    if (base[0] == '\x0f')
    {
        offset++;
        if (base[1] == '\x38' || base[1] == '\x3a')
        {
            offset++;
            op_info->opcode_table = OP3_TABLE;
        } 
        else 
            op_info->opcode_table = OP2_TABLE;
    }
    op_struct = opcode_table[op_info->opcode_table];
    table_array_size = opcode_table_size[op_info->opcode_table] / sizeof(opcode_info_struct);
    for (int i = 0;i < table_array_size;i++)
    {
        if ((char)base[offset] == op_struct[i].opcode)
        {
            op_info->opcode = i;
            offset++;
            op_info->opcode_len += offset;
            op_info->opcode_count = offset;
            op_info->arg_count = opcode_arg_count(&op_struct[i]);
            return 1;
        }
    }
    op_info->opcode = INV;
    return INV;
}

int parse_opcode(unsigned char *base, opcode_gathered_info *op_info) // fill opcode table
{
    int prefix = check_prefix(base, op_info, 0);
    if (prefix == INV)
    {
        op_info->opcode_len++;
        op_info->prefix_count++;
        return INV;
    }
    for(int i = 1;prefix != NO_PREFIX;i++)
    {
        op_info->prefix_count++;
        prefix = check_prefix(&base[i], op_info, 0);
        if (prefix == INV) 
        {
            op_info->opcode_len++;
            op_info->prefix_count++;
            return INV;
        }
    }
    if (check_opcode_table(base, op_info) == INV) 
        return 0;
    return 1;
}
int check_opcode_extension(unsigned char *base, opcode_info_struct *opcode, opcode_info_struct *op_extension, opcode_gathered_info *op_info, int addr_size)
{
    opcode_extension_struct *op_ex, *op_ex_base;
    unsigned char *op_p = (unsigned char *)&base[op_info->prefix_count]; // SKIP PREFIXES
    int arg = -1,
    arg_ex_count = 0, 
    arg_count = op_info->arg_count, 
    escape_op = (opcode->args[0].arg_1_1 == XR);
    memset(op_extension, '\x00', sizeof(opcode_info_struct));
    if (!escape_op)
    {
        for (int i = 0;i<arg_count;i++) 
            if (opcode->args[i].arg_1_1 == T) 
                arg = i;
        if (arg == -1) 
            return 0;
    }
    else // PROCEDURE FOR ESCAPE OPCODES D8-DF
    {
        fill_mod_rm(&op_p[op_info->opcode_count], op_info, addr_size);
        if (op_p[op_info->opcode_count] > 0xbf)
        {
            op_ex = &escape_opcodes[*op_p - 0xd8].after_bf[(op_p[1] / 0x8) - 0x18][op_p[1] % 0x8];
            op_ex_base = op_ex - (op_p[1] % 0x8);
        }
        else 
            op_ex = &escape_opcodes[*op_p - 0xd8].before_bf[(op_p[1] & 0x38) >> 3];
        arg_ex_count = opcode_arg_count_ex(op_ex);
        if (op_ex->opcode_name == NULL)
        {
            if (op_p[op_info->opcode_count] < 0xbf)
            {
                op_info->opcode_len--;
                return INV;
            }
            if (op_ex_base->opcode_name == NULL || op_ex_base->args[0].arg_1_1 != ST)
            {
                op_extension->args[0].arg_1_1 = E;
                op_extension->args[0].arg_1_2 = v;
                op_extension->opcode_name = escape_opcodes[*op_p - 0xd8].before_bf[(op_p[1] & 0x38) >> 3].opcode_name;
                op_info->arg_count = 1;
                return 1;
            } 
            else 
            {
                op_ex = op_ex_base;
                arg_ex_count = opcode_arg_count_ex(op_ex_base);
            }
        }
        op_extension->opcode_name = op_ex->opcode_name;
        opcode_args *args = op_extension->args, *args_ex = op_ex->args;
        for (int i = 0;i < arg_ex_count;i++){
            args[i].arg_1_1 = args_ex[i].arg_1_1;
            args[i].arg_1_2 = args_ex[i].arg_1_2;
        }
        op_info->arg_count = arg_ex_count;
        return 1;
    }
    fill_mod_rm(&op_p[op_info->opcode_count], op_info, addr_size);
    op_ex = &opcode_extension_table[opcode->args[arg].arg_1_2][op_info->mod_rm.reg]; // OPCODE EXTENSION, GROUPS
    if (op_ex->opcode_name == 0)
    {
        op_info->opcode_len = 1;
        return INV;
    }
    op_extension->opcode_name = op_ex->opcode_name;
    for (int i = 0, a;i < arg_count;i++){
        a = opcode->args[i].arg_1_1;
        if ((a != 0) && (a != T))
        {
            op_extension->args[arg_ex_count].arg_1_1 = a;
            op_extension->args[arg_ex_count++].arg_1_2 = opcode->args[i].arg_1_2;
        }
    }
    for (int i = 0, a, a2;i < 3;i++){
        a = op_ex->args[i].arg_1_1;
        a2 = op_ex->args[i].arg_1_2;
        if (a == 0) break;
        if (a == DR){
            if (opcode->args[i].arg_1_2 == b){
                a2 |= REG_BYTE;
                a = Z;
            }
        }
        else if (opcode->args[i].arg_1_1 == E && opcode->args[i].arg_1_2 == b) a2 = b;
        op_extension->args[arg_ex_count].arg_1_1 = a;
        op_extension->args[arg_ex_count++].arg_1_2 = a2;
    }
    op_info->arg_count = arg_ex_count;
    return 1;
}
int opcode_disasm_line(char *buf, int buf_size, unsigned char *base)
{
    opcode_gathered_info op_info;
    opcode_info_struct *opcode, op_ex;
    memset(buf, '\x00', buf_size);
    memset(&op_info, '\x00', sizeof(opcode_gathered_info));
    if (parse_opcode(base, &op_info) == 0)
    {
        strncat(buf, "???", buf_size);
        return op_info.opcode_len;
    }
    if (op_info.prefix_count != 0)
    {
        for (int i = 0;i < op_info.prefix_count;i++)
        {
            if (base[i] == '\xf0')
            {
                if (check_prefix(&base[i], &op_info, NO_OP_LEN) == INV)
                {
                    strncat(buf, "PREFIX LOCK:", buf_size);
                    return op_info.opcode_len;
                }
                strncpy(buf, prefix_g1[LOCK], buf_size);
                strncat(buf, " ", buf_size);
                break;
            }
        }
        for (int i = 0;i < op_info.prefix_count;i++){
            if (base[i] == '\xf0') 
                continue;
            if (check_prefix(&base[i], &op_info, NO_OP_LEN) == INV) // PREFIX IS REPEATED
            {
                strncat(buf, "PREFIX ", buf_size);
                strncat(buf, prefix_g1[op_info.prefix], buf_size);
                strncat(buf, ":", buf_size);
                return op_info.opcode_len;
            }
            if (op_info.prefix_table == PREFIX_G1)
            {
                strncat(buf, prefix_g1[op_info.prefix], buf_size);
                strncat(buf, " ", buf_size);
            }
        }
    }
    opcode = &opcode_table[op_info.opcode_table][op_info.opcode];
    int op_size = check_prefix_op(base, op_info.prefix_count, '\x66'); // flag for operand_size prefix
    int addr_size = check_prefix_op(base, op_info.prefix_count, '\x67'); // flag for address_size prefix
    int op_ex_ret = check_opcode_extension(base, opcode, &op_ex, &op_info, addr_size);
    if (op_ex_ret)
    {
        if (op_ex_ret == INV) 
        {
            strncat(buf, "???", buf_size);
            return op_info.opcode_len;
        } 
        else opcode = &op_ex;
    }
    const char *op_name = opcode->opcode_name;
    for (int i = 0, arg2;i < op_info.arg_count;i++)
        if (opcode->args[i].arg_1_1 == K)
        {
            arg2 = opcode->args[i].arg_1_2;
            if (arg2)
                if (!addr_size) 
                    break;
                else 
                    if (!op_size) 
                        break;
            op_name += strlen(op_name) + 1;
            break;
        }
    strncat(buf, op_name, buf_size);
    if (op_info.arg_count == 0) 
        return op_info.opcode_len;
    for (int i = 0, arg1, arg2, mod_rm_offset = op_info.prefix_count + op_info.opcode_count;i < op_info.arg_count;i++)
    {
        arg1 = opcode->args[i].arg_1_1;
        arg2 = opcode->args[i].arg_1_2;
        strncat(buf, " ", buf_size);
        switch(arg1)
        {
            case A: 
            {
                strncat(buf, "FAR ", buf_size);
                int opcode_len = 0;
                int imm = *(int *)(&base[mod_rm_offset]);
                arg2 = d;
                if (op_size){
                    opcode_len += 2;
                    arg2 = w;
                }
                else opcode_len += 4;
                strncat_args(buf, buf_size, *(int *)(&base[mod_rm_offset + opcode_len]), w, IMMEDIATE);
                strncat(buf, ":", buf_size);
                strncat_args(buf, buf_size, imm, arg2, IMMEDIATE);
                op_info.opcode_len += opcode_len + 2;
                break;
            }
            case O: {
                strncat(buf, operand_size[op_size_calc(arg2, op_size)], buf_size);
                strncat(buf, " PTR ", buf_size);
                int seg_calc = segment_calc(base, op_info.prefix_count);
                if (seg_calc == -1) seg_calc = DS;
                strncat(buf, segment_registers[seg_calc], buf_size);
                strncat(buf, ":[", buf_size);
                int imm = *(int *)(&base[mod_rm_offset]);
                int opcode_len = 4;
                if (addr_size){
                    opcode_len -= 2;
                    arg2 = w;
                } else arg2 = d;
                strncat_args(buf, buf_size, imm, arg2, IMMEDIATE);
                strncat(buf, "]", buf_size);
                op_info.opcode_len += opcode_len;
                break;
            }
            case M:
                if (opcode->opcode == '\x62' && op_info.opcode_table == OP1_TABLE){
                    if (arg2 == a && op_size) arg2 = d;
                    else arg2 = q;
                }
            case E: 
            {
                if (!op_info.has_mod_rm)
                    fill_mod_rm(&base[mod_rm_offset], &op_info, addr_size);
                if (op_info.mod_rm.mod == 3)
                {
                    int data_len = arg2;
                    if ((arg2 == v || arg2 == p) && op_size) data_len = w;
                    strncat_args(buf, buf_size, op_info.mod_rm.rm, data_len, GENERAL_REG);
                    break;
                }
                strncat(buf, operand_size[op_size_calc(arg2, op_size)], buf_size);
                strncat(buf, " PTR ", buf_size);
                int seg_calc = segment_calc(base, op_info.prefix_count);
                if (seg_calc == -1) 
                    seg_calc = DS;
                strncat(buf, segment_registers[seg_calc], buf_size);
                strncat(buf, ":[", buf_size);
                if (op_info.mod_rm.mod == 0)
                {
                    if (op_info.mod_rm.rm == (5 + addr_size)) 
                    {
                        strncat_args(buf, buf_size, *(int *)&base[mod_rm_offset + 1], displacement_calc(&op_info.mod_rm, addr_size), IMMEDIATE);
                        strncat(buf, "]", buf_size);
                        break;
                    }
                }
                if (op_info.has_sib)
                    strncat_sib(buf, &base[mod_rm_offset + 1], &op_info, buf_size);
                else 
                {
                    int mode = GENERAL_REG;
                    if (op_info.mod_rm.rm == (5 + addr_size) && segment_calc(base, op_info.prefix_count) == -1) memcpy(&buf[strlen(buf) - 4], segment_registers[SS], 2);
                    if (addr_size) mode = GENERAL_REG_16;
                    strncat_args(buf, buf_size, op_info.mod_rm.rm, d, mode);
                }
                int disp_size = 0;
                switch(op_info.mod_rm.mod)
                {
                    case 1:
                        disp_size = b;
                        break;
                    case 2:
                        if (addr_size) disp_size = w;
                        else disp_size = d;
                        break;
                }
                if (disp_size != 0)
                    strncat_args(buf, buf_size, *(int *)(&base[mod_rm_offset + op_info.has_sib + 1]), disp_size, DISPLACEMENT);
                strncat(buf, "]", buf_size);
                break;
            }
            case G:
                if (!op_info.has_mod_rm)
                    fill_mod_rm(&base[mod_rm_offset], &op_info, addr_size);
                if (op_size && (arg2 == v | arg2 == z)) arg2 = w;
                strncat_args(buf, buf_size, op_info.mod_rm.reg, arg2, GENERAL_REG);
                break;
            case IV:
                strncat_args(buf, buf_size, arg2, d, IMMEDIATE);
                break;
            case I: {
                int imm_size = arg2, opcode_len = 0;
                if (arg2 == z || arg2 == v){
                    if (op_size) imm_size = w;
                    else imm_size = d;
                }
                switch(imm_size){
                    case b:
                        opcode_len += 1;
                        break;
                    case w:
                        opcode_len += 2;
                        break;
                    case d:
                        opcode_len += 4;
                        break;
                }
                op_info.opcode_len += opcode_len;
                strncat_args(buf, buf_size, *(int *)(&base[op_info.opcode_len - opcode_len]), imm_size, IMMEDIATE);
                break;
            }
            case J: {
                unsigned int imm = *(int *)(&base[mod_rm_offset]), opcode_len = 0, imm_size = arg2;
                if (arg2 == z){
                    if (op_size) imm_size = w;
                    else imm_size = d;
                }
                switch(imm_size){
                    case b:
                        opcode_len += 1;
                        imm &= 0xff;
                        break;
                    case w:
                        opcode_len += 2;
                        imm &= 0xffff;
                        break;
                    case d:
                        opcode_len += 4;
                        break;
                }
                op_info.opcode_len += opcode_len;
                imm += (unsigned int)(base + op_info.opcode_len);
                if (op_size) imm_size = w;
                else imm_size = d;
                strncat_args(buf, buf_size, imm, imm_size, IMMEDIATE);
                break;
            }
            case S: {
                if (!op_info.has_mod_rm)
                    fill_mod_rm(&base[mod_rm_offset], &op_info, addr_size);
                int seg[] = {3,0,1,2,4,5};
                strncat(buf,segment_registers[seg[op_info.mod_rm.reg]], buf_size);
                break;
            }
            case X:

            case Y: {
                int reg_size, reg;
                if (arg1 == X) reg = ESI;
                else reg = EDI;
                strncat(buf, operand_size[op_size_calc(arg2, op_size)], buf_size);
                strncat(buf, " PTR ", buf_size);
                strncat(buf, segment_registers[ES], buf_size);
                strncat(buf, ":[", buf_size);
                if (addr_size) reg_size = w;
                else reg_size = d;
                strncat_args(buf, buf_size, reg, reg_size, GENERAL_REG);
                strncat(buf, "]", buf_size);
                break;
            }
            case DR:
                if (op_size) arg2 |= 0xc0;
            case Z: {
                int mode = arg2 & 0xc0;
                arg2 &= 0x3f;
                int reg_size = d;
                if (mode == SEG){
                    strncat(buf, segment_registers[arg2], buf_size);
                    break;
                }
                else if (mode == REG_BYTE) reg_size = b;
                else
                    if (op_size || mode == REG_WORD) reg_size = w;
                strncat_args(buf, buf_size, arg2, reg_size, GENERAL_REG);
                break;
            }
            case ST:
                strncat(buf, "ST(", buf_size);
                if (arg2)
                    itoa(((unsigned char)base[mod_rm_offset] % 0x8), &buf[strlen(buf)], 10);
                else strncat(buf, "0", buf_size);
                strncat(buf, ")", buf_size);
        }
        if (i < op_info.arg_count - 1) strncat(buf, ",", buf_size);
    }
    toupper_string(buf, strlen(buf));
    return op_info.opcode_len;
}

int main(){
    opcode_gathered_info op_info;
    unsigned char base[] = "\xd8\xc9";
    char buf[64];
    char chars[490];
    int opcode_size = 0, printf_ret;
    for (int i = 0, opcode;i < 10;i++){
        printf_ret = 0;
        opcode = opcode_disasm_line(buf, 64, &base[opcode_size]);
        if (opcode == 0) opcode = 1;
        printf("|%p| ", &base[opcode_size]);
        for (int d = 0;d < opcode;d++) printf_ret += printf("%02X ", (unsigned char)base[opcode_size+d]);
        while (printf_ret < 30) printf_ret += printf(" ");
        printf(" %s\n", buf);
        opcode_size += opcode;
    }
}
