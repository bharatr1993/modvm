#pragma once

#include "object.h"


enum __attribute__ ((__packed__))
ModlOpcode
{
  OP_NOP     = 0x00,
  OP_RET     = 0x01,
  OP_MOV     = 0x02,
  OP_LOADR   = 0x03,
  OP_LOADC   = 0x04,
  OP_TBLGETR = 0x05,
  OP_TBLGETC = 0x06,
  // OP_SETR    = 0x07, // removed
  // OP_SETC    = 0x08, // removed
  OP_CALLR   = 0x09,
  OP_CALLC   = 0x0A,
  OP_LOADD   = 0x0B,
  OP_LOADFUN = 0x0C,

  OP_ROL     = 0x0D,
  OP_ROR     = 0x0E,
  OP_IDIV    = 0x0F,
  OP_ADD     = 0x10,
  OP_SUB     = 0x11,
  OP_MUL     = 0x12,
  OP_DIV     = 0x13,
  OP_MOD     = 0x14,
  OP_AND     = 0x15,
  OP_OR      = 0x16,
  OP_XOR     = 0x17,
  OP_NAND    = 0x18,
  OP_NOR     = 0x19,
  OP_NXOR    = 0x1A,

  OP_JMP     = 0x1F,

  OP_CMPEQ   = 0x20,
  OP_CMPNEQ  = 0x21,
  OP_CMPLT   = 0x22,
  OP_CMPNLT  = 0x23,
  OP_CMPGT   = 0x24,
  OP_CMPNGT  = 0x25,
  OP_CMPLE   = 0x26,
  OP_CMPNLE  = 0x27,
  OP_CMPGE   = 0x28,
  OP_CMPNGE  = 0x29,

  OP_JCF     = 0x30,
  OP_JCT     = 0x31,

  OP_POP     = 0x40,
  OP_PUSH    = 0x41,
  OP_TBLPUSH = 0x42,
  OP_TBLSETR = 0x43,
  OP_TBLSETC = 0x44,
  OP_ENVGETR = 0x45,
  OP_ENVGETC = 0x46,
  OP_ENVSETR = 0x47,
  OP_ENVSETC = 0x48,
  OP_ENVUPKR = 0x49,
  OP_ENVUPKC = 0x4A,
  OP_ENVUPKT = 0x4B,
  OP_ENVPUSH = 0x4C,
  OP_ENVPOP  = 0x4D,

  OP_NOT     = 0x50,
  OP_INV     = 0x51,
  OP_LEN     = 0x52,
  OP_NEG     = 0x53,
};

static char const * const instructions_names_table[256] =
{
  [0x00] = "NOP",
  [0x01] = "RET",
  [0x02] = "MOV",
  [0x03] = "LOADR",
  [0x04] = "LOADC",
  [0x05] = "TBLGETR",
  [0x06] = "TBLGETC",
  // [0x07] = "SETR", // removed
  // [0x08] = "SETC", // removed
  [0x09] = "CALLR",
  [0x0A] = "CALLC",
  [0x0B] = "LOADD",
  [0x0C] = "LOADFUN",

  [0x0D] = "ROL",
  [0x0E] = "ROR",
  [0x0F] = "IDIV",
  [0x10] = "ADD",
  [0x11] = "SUB",
  [0x12] = "MUL",
  [0x13] = "DIV",
  [0x14] = "MOD",
  [0x15] = "AND",
  [0x16] = "OR",
  [0x17] = "XOR",
  [0x18] = "NAND",
  [0x19] = "NOR",
  [0x1A] = "NXOR",

  [0x1F] = "JMP",

  [0x20] = "CMPEQ",
  [0x21] = "CMPNEQ",
  [0x22] = "CMPLT",
  [0x23] = "CMPNLT",
  [0x24] = "CMPGT",
  [0x25] = "CMPNGT",
  [0x26] = "CMPLE",
  [0x27] = "CMPNLE",
  [0x28] = "CMPGE",
  [0x29] = "CMPNGE",

  [0x30] = "JCF",
  [0x31] = "JCT",

  [0x40] = "POP",
  [0x41] = "PUSH",
  [0x42] = "TBLPUSH",
  [0x43] = "TBLSETR",
  [0x44] = "TBLSETC",
  [0x45] = "ENVGETR",
  [0x46] = "ENVGETC",
  [0x47] = "ENVSETR",
  [0x48] = "ENVSETC",
  [0x49] = "ENVUPKR",
  [0x4A] = "ENVUPKC",
  [0x4B] = "ENVUPKT",
  [0x4C] = "ENVPUSH",
  [0x4D] = "ENVPOP",

  [0x50] = "NOT",
  [0x51] = "INV",
  [0x52] = "LEN",
  [0x53] = "NEG",
};

enum __attribute__ ((__packed__))
TemplateParameter
{
  TP_ERROR,

  TP_EMPTY,

  TP_REGAL,
  TP_REGSP,
  TP_INT64,

  TP_SEBO,
};

struct InstructionParametersTemplate
{
  enum TemplateParameter p[2];
};

struct InstructionParametersTemplate instruction_parameters_templates[256] =
{
  [OP_NOP]     = {{ TP_EMPTY, }},
  [OP_RET]     = {{ TP_EMPTY, }},
  [OP_MOV]     = {{ TP_REGSP, }},

  [OP_LOADC]   = {{ TP_REGAL, TP_SEBO, }},
  [OP_TBLGETR] = {{ TP_REGSP, }},
  [OP_TBLGETC] = {{ TP_REGAL, TP_SEBO, }},

  [OP_CALLR]   = {{ TP_REGAL, }},
  [OP_LOADFUN] = {{ TP_REGAL, TP_INT64, }},

  [OP_JMP]     = {{ TP_INT64, }},

  [OP_ROL]     = {{ TP_REGSP, }},
  [OP_ROR]     = {{ TP_REGSP, }},
  [OP_IDIV]    = {{ TP_REGSP, }},
  [OP_ADD]     = {{ TP_REGSP, }},
  [OP_SUB]     = {{ TP_REGSP, }},
  [OP_MUL]     = {{ TP_REGSP, }},
  [OP_DIV]     = {{ TP_REGSP, }},
  [OP_MOD]     = {{ TP_REGSP, }},
  [OP_AND]     = {{ TP_REGSP, }},
  [OP_OR]      = {{ TP_REGSP, }},
  [OP_XOR]     = {{ TP_REGSP, }},
  [OP_NAND]    = {{ TP_REGSP, }},
  [OP_NOR]     = {{ TP_REGSP, }},
  [OP_NXOR]    = {{ TP_REGSP, }},

  [OP_CMPEQ]   = {{ TP_REGSP, }},
  [OP_CMPNEQ]  = {{ TP_REGSP, }},
  [OP_CMPLT]   = {{ TP_REGSP, }},
  [OP_CMPNLT]  = {{ TP_REGSP, }},
  [OP_CMPGT]   = {{ TP_REGSP, }},
  [OP_CMPNGT]  = {{ TP_REGSP, }},
  [OP_CMPLE]   = {{ TP_REGSP, }},
  [OP_CMPNLE]  = {{ TP_REGSP, }},
  [OP_CMPGE]   = {{ TP_REGSP, }},
  [OP_CMPNGE]  = {{ TP_REGSP, }},

  [OP_JCF]     = {{ TP_REGAL, TP_INT64, }},
  [OP_JCT]     = {{ TP_REGAL, TP_INT64, }},

  [OP_POP]     = {{ TP_REGAL, }},
  [OP_PUSH]    = {{ TP_REGAL, }},

  [OP_TBLPUSH] = {{ TP_REGSP, }},
  [OP_TBLSETR] = {{ TP_REGSP, TP_REGAL, }},

  [OP_ENVGETR] = {{ TP_REGSP, }},
  [OP_ENVGETC] = {{ TP_REGAL, TP_SEBO, }},
  [OP_ENVSETR] = {{ TP_REGSP, }},
  [OP_ENVSETC] = {{ TP_REGAL, TP_SEBO, }},
  [OP_ENVUPKR] = {{ TP_REGSP, }},
  [OP_ENVUPKC] = {{ TP_REGAL, TP_SEBO, }},
  [OP_ENVUPKT] = {{ TP_REGAL, }},

  [OP_NOT]     = {{ TP_REGAL, }},
  [OP_INV]     = {{ TP_REGAL, }},
  [OP_LEN]     = {{ TP_REGAL, }},
  [OP_NEG]     = {{ TP_REGAL, }},
};


#define INSTRUCTION_TEMPLATE_VALUES_COUNT 2

struct Instruction
{
  enum ModlOpcode opcode;
  size_t byte_length;

  union
  {
    int64_t i64;
    byte r[2];
    struct ModlObject object;
  } a[INSTRUCTION_TEMPLATE_VALUES_COUNT];
};
