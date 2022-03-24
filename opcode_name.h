//PREFIXES
#define LOCK 0
#define REPNE 1
#define REP 2
#define REPE 3
#define BND 4
#define INV 0xff
//--PREFIXES

//REGS
#define GENERAL_REG 0
#define GENERAL_REG_W 1
#define GENERAL_REG_B 2
#define DISPLACEMENT 3
#define GENERAL_REG_16 4
#define IMMEDIATE 5
//--REGS

//OPERAND SIZES
#define BYTE 0
#define WORD 1
#define DWORD 2
#define QWORD 3
#define FWORD 4
#define TBYTE 5
//--OPERAND SIZES

enum segment_registers_enum{
    CS,SS,DS,ES,FS,GS
};
enum general_registers_enum{
    EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI
};
enum general_registers_w_enum{
    AX,EX,DX,BX,SP,BP,SI,DI
};
enum general_registers_b_enum{
    AL,CL,DL,BL,AH,CH,DH,BH
};
enum opcode_name{
    ADD,PUSH,POP,OR,ADC,SBB,AND,DAA,SUB,DAS,XOR,AAA,
    CMP,AAS,INC,DEC,PUSHAD,PUSHAW,POPAD,POPAW,BOUND,ARPL,IMUL,INS,
    OUTS,JO,JNO,JB,JNB,JE,JNE,JBE,JA,JS,JNS,JP,JNP,JL,JNL,JNG,JG,
    TEST,XCHG,MOV,LEA,NOP,CWDE,CBW,CDQ,CWD,CALL,WAIT,PUSHFD,PUSHFW,
    POPFD,POPFW,SAHF,LAHF,MOVS,CMPS,STOS,LODS,SCAS,ROL,ROR,RCL,RCR,
    SHL,SHR,SAR,RET,LES,LDS,ENTER,LEAVE,RETF,INT3,INT,INTO,IRETD,
    IRETW,AAM,AAD,SALC,XLAT,FADD,FMUL,FCOM,FCOMP,FSUB,FSUBR,FDIV,
    FDIVR,FLD,FST,FSTP,FLDENV,FLDCW,FSTENV,FSTCW,FXCH,FNOP,FCHS,
    FABS,FTST,FXAM,FLD1,FLDL2T,FLDL2E,FLDPI,FLDLG2,FLDLN2,FLDZ,
    F2XM1,FYL2X,FPTAN,FPATAN,FXTRACT,FPREM1,FDECSTP,FINCSTP,FPREM,
    FYL2XP1,FSQRT,FSINCOS,FRNDINT,FSCALE,FSIN,FCOS,FIADD,FIMUL,FICOM,
    FICOMP,FISUB,FISUBR,FIDIV,FIDIVR,FCMOVB,FCMOVE,FCMOVBE,FCMOVU,
    FUCOMPP,FILD,FISTTP,FIST,FISTP,FCMOVNB,FCMOVNE,FCMOVNBE,FCMOVNU,
    FENI,FDISI,FCLEX,FINIT,FUCOMI,FCOMI,FRSTOR,FSAVE,FSTSW,FFREE,FUCOM,
    FUCOMP,FADDP,FMULP,FCOMPP,FSUBRP,FSUBP,FDIVRP,FDIVP,FBLD,FBSTP,FUCOMIP,
    FCOMIP,LOOPDNE,LOOPWNE,LOOPDE,LOOPWE,LOOPD,LOOPW,JECXZ,JCXZ,IN,OUT,
    JMP,INT1,HLT,CMC,CLC,STC,CLI,STI,CLD,STD,NOT,NEG,MUL,DIV,IDIV
};
const char *scaled_index[]={
    "", "*2", "*4", "*8"
};
const char *segment_registers[]={
    "CS", "SS", "DS", "ES", "FS", "GS"
};
const char *general_registers[]={
    "EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"
};
const char *general_registers_word[]={
    "AX", "EX", "DX", "BX", "SP", "BP", "SI", "DI"
};
const char *general_registers_byte[]={
    "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"
};
const char *general_registers_16[]={
    "BX+SI", "BX+DI", "BP+SI", "BP+DI", "SI", "DI", "BP", "BX"
};
const char *operand_size[]={
    "BYTE", "WORD", "DWORD", "QWORD", "FWORD", "TBYTE"
};
const char *opcode_name[]={
    "ADD","PUSH","PUSH","OR","ADC","SBB","AND","DAA","SUB","DAS","XOR",
    "AAA","CMP","AAS","INC","DEC","PUSHAD","PUSHAW","POPAD","POPAW",
    "BOUND","ARPL","IMUL","INS","OUTS","JO","JNO","JB","JNB","JE",
    "JNE","JBE","JA","JS","JNS","JP","JNP","JL","JNL","JNG","JG",
    "TEST","XCHG","MOV","LEA","NOP","CWDE","CBW","CDQ","CWD","CALL",
    "WAIT","PUSHFD","PUSHFW","POPFD","POPFW","SAHF","LAHF","MOVS",
    "CMPS","STOS","LODS","SCAS","ROL","ROR","RCL","RCR","SHL","SHR",
    "SAR","RET","LES","LDS","ENTER","LEAVE","RETF","INT3","INT","INTO",
    "IRETD","IRETW","AAM","AAD","SALC","XLAT","FADD","FMUL","FCOM",
    "FCOMP","FSUB","FSUBR","FDIV","FDIVR","FLD","FST","FSTP","FLDENV",
    "FLDCW","FSTENV","FSTCW","FXCH","FNOP","FCHS","FABS","FTST","FXAM",
    "FLD1","FLDL2T","FLDL2E","FLDPI","FLDLG2","FLDN2","FLDZ","F2XM1",
    "FYL2X","FPTAN","FPATAN","FXTRACT","FPREM1","FDECSTP","FINCSTP",
    "FPREM","FYL2XP1","FSQRT","FSINCOS","FRNDINT","FSCALE","FSIN","FCOS",
    "FIADD","FIMUL","FICOM","FICOMP","FISUB","FSUBR","FIDIV","FIDIVR",
    "FCMOVB","FCMOVE","FCMOVBE","FCMOVU","FUCOMPP","FILD","FISTTP",
    "FIST","FISTP","FCMOVNB","FCMOVNE","FCMOVNBE","FCMOVNU","FENI",
    "FDISI","FCLEX","FINIT","FUCOMI","FCOMI","FRSTOR","FSAVE","FSTSW",
    "FFREE","FUCOM","FUCOMP","FADDP","FMULP","FCOMPP","FSUBRP","FSUBP","FDIVRP",
    "FDIVP","FBLD","FBSTP","FUCOMIP","FCOMIP","LOOPDNE","LOOPWNE","LOOPDE",
    "LOOPWE","LOOPD","LOOPW","JECXZ","JCXZ","IN","OUT","JMP","INT1","HLT",
    "CMC","CLC","STC","CLI","STI","CLD","STD","NOT","NEG","MUL","DIV","IDIV",

    };
const char *prefix_g1[]={
    "LOCK", "REPNE", "REP", "REPE", "BND"
};
const char *prefix_g2[]={
    segment_registers[CS], segment_registers[SS], segment_registers[DS],
    segment_registers[ES], segment_registers[FS], segment_registers[GS]
};
const char **prefix_table[]={
    prefix_g1, prefix_g2
};
const char **general_registers_table[]={
    general_registers, general_registers_word, general_registers_byte, general_registers_16
};
