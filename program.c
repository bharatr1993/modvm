#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <getopt.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>

#include "helpers.h"


typedef uint8_t byte;
typedef byte bool;

#define FALSE ((bool) 0)
#define TRUE ((bool) 1)
#define not !


#define REG(N) ((byte) N)
#define REGS(A, B) ((byte) ((A << 4) | B))


#define VM_SETTING_REGITERS_COUNT 16


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
  OP_SETR    = 0x07,
  OP_SETC    = 0x08,
  OP_CALLR   = 0x09,
  OP_CALLC   = 0x0A,
  OP_LOADD   = 0x0B,
  OP_LOADFUN = 0x0C,

  OP_ADD  = 0x10,
  OP_SUB  = 0x11,
  OP_MUL  = 0x12,
  OP_DIV  = 0x13,
  OP_MOD  = 0x14,
  OP_AND  = 0x15,
  OP_OR   = 0x16,
  OP_XOR  = 0x17,
  OP_NAND = 0x18,
  OP_NOR  = 0x19,
  OP_NXOR = 0x1A,

  OP_JMP   = 0x1F,

  OP_CMPEQ  = 0x20,
  OP_CMPNEQ = 0x21,
  OP_CMPLT  = 0x22,
  OP_CMPNLT = 0x23,
  OP_CMPGT  = 0x24,
  OP_CMPNGT = 0x25,
  OP_CMPLE  = 0x26,
  OP_CMPNLE = 0x27,
  OP_CMPGE  = 0x28,
  OP_CMPNGE = 0x29,

  OP_JCF    = 0x30,
  OP_JCT    = 0x31,

  OP_POP     = 0x40,
  OP_PUSH    = 0x41,
  OP_TBLPUSH = 0x42,
  OP_TBLSETR = 0x43,
  OP_TBLSETC = 0x44,
  OP_ENVGETR = 0x45,
  OP_ENVGETC = 0x46,
  OP_ENVSETR = 0x47,
  OP_ENVSETC = 0x48,
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
  [0x07] = "SETR",
  [0x08] = "SETC",
  [0x09] = "CALLR",
  [0x0A] = "CALLC",
  [0x0B] = "LOADD",
  [0x0C] = "LOADFUN",

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
};


enum __attribute__ ((__packed__))
ModlType
{
  ModlNil      = 0,
  ModlBoolean  = 1,
  ModlInteger  = 2,
  ModlFloating = 3,
  ModlString   = 4,
  ModlTable    = 5,
  ModlFunction = 6,
};

struct ModlObject
{
  enum ModlType type;
  union
  {
    bool boolean;
    int64_t integer;
    double floating;
    char const *string;

    struct ModlTableNode
    {
      struct ModlObject * key;
      struct ModlObject * value;
      struct ModlTableNode * next;
    } *table;

    struct ModlFunctionInfo
    {
      bool is_external;
      uint64_t position;
      struct Environment * context;
    } fun;
  } value;
};


void display_modl_object(struct ModlObject const * object)
{
  if (NULL == object)
  {
    printf("\x1b[31;1m%s\x1b[0m", "undefined");
    return;
  }

  switch (object->type)
  {
    case ModlNil:
    {
      printf("\x1b[37;1m%s\x1b[0m", "nil");
    } break;

    case ModlBoolean:
    {
      printf("%s", object->value.boolean ? "true" : "false");
    } break;

    case ModlInteger:
    {
      printf("\x1b[33m%ld\x1b[0m", object->value.integer);
    } break;

    case ModlFloating:
    {
      printf("\x1b[33m%lf\x1b[0m", object->value.floating);
    } break;

    case ModlString:
    {
      printf("\x1b[32m\"%s\"\x1b[0m", object->value.string);
    } break;

    case ModlTable:
    {
      printf("%c", '[');

      struct ModlTableNode const * node = object->value.table;
      while (NULL != node->key)
      {
        display_modl_object(node->key);
        printf("%s", ": ");
        display_modl_object(node->value);

        if (NULL != node->next->key)
          printf("%s", "; ");

        node = node->next;
      }
      printf("%c", ']');
    } break;

    case ModlFunction:
    {
      printf("\x1b[33;1m&%04lx\x1b[0m", object->value.fun.position);
    } break;
  }
}


struct ModlObject * modl_nil()
{
  return (struct ModlObject*) calloc(1, sizeof (struct ModlObject));
}

struct ModlObject * modl_table()
{
  struct ModlObject * object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  object->type = ModlTable;
  object->value.table = (struct ModlTableNode *) calloc(1, sizeof (struct ModlTableNode));
  return object;
}


bool modl_object_equals(struct ModlObject const * const self, struct ModlObject const * const other)
{
  if (NULL == self || NULL == other) return FALSE;
  if (self == other) return TRUE;
  if (self->type != other->type) return FALSE;

  switch (self->type)
  {
    case ModlNil: return TRUE;
    case ModlBoolean: return self->value.boolean == other->value.boolean;
    case ModlInteger: return self->value.integer == other->value.integer;
    case ModlFloating: return self->value.floating == other->value.floating;
    case ModlString: return 0 == strcmp(self->value.string, other->value.string);
    case ModlTable: return FALSE;
    case ModlFunction:
      return self->value.fun.is_external == other->value.fun.is_external
          && self->value.fun.position    == other->value.fun.position
          && self->value.fun.context     == other->value.fun.context;
  }
}


struct ModlObject * bool_to_modl(bool data)
{
  struct ModlObject* object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  object->type = ModlBoolean;
  object->value.boolean = data;
  return object;
}

struct ModlObject * int_to_modl(int64_t data)
{
  struct ModlObject* object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  object->type = ModlInteger;
  object->value.integer = data;
  return object;
}

struct ModlObject * double_to_modl(double data)
{
  struct ModlObject* object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  object->type = ModlFloating;
  object->value.floating = data;
  return object;
}

struct ModlObject * str_to_modl(char const * data)
{
  struct ModlObject* object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  object->type = ModlString;
  object->value.string = data;
  return object;
}

struct ModlObject * ifun_to_modl(struct Environment * environment, uint64_t position)
{
  struct ModlObject* object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  object->type = ModlFunction;
  object->value.fun.is_external = FALSE;
  object->value.fun.position = position;
  object->value.fun.context = environment;
  return object;
}

struct ModlObject * efun_to_modl(uint64_t pointer)
{
  struct ModlObject* object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  object->type = ModlFunction;
  object->value.fun.is_external = TRUE;
  object->value.fun.position = pointer;
  object->value.fun.context = NULL;
  return object;
}


bool modl_to_bool(struct ModlObject const * object)
{
  if (NULL == object) return FALSE;
  // if (ModlBoolean != object->type) return TRUE;
  return (object->value.boolean ? TRUE : FALSE);
}

int64_t modl_to_int(struct ModlObject const * object)
{
  if (NULL == object) return 0;
  // if (ModlInteger != object->type) return 0;
  return object->value.integer;
}

double modl_to_double(struct ModlObject const * object)
{
  if (NULL == object) return 0.0;
  // if (ModlFloating != object->type) return 0.0;
  return object->value.floating;
}

char const * modl_to_str(struct ModlObject const * object)
{
  if (NULL == object) return NULL;
  if (ModlString != object->type) return NULL;
  return object->value.string;
}


struct ModlObject * modl_object_copy(struct ModlObject const * const self)
{
  if (NULL == self) return NULL;

  // printf("  copying object: ");
  // display_modl_object(self);
  // printf("%c", '\n');

  switch (self->type)
  {
    case ModlNil: return modl_nil();
    case ModlBoolean: return bool_to_modl(self->value.boolean);
    case ModlInteger: return int_to_modl(self->value.integer);
    case ModlFloating: return double_to_modl(self->value.floating);
    case ModlString: return str_to_modl(self->value.string);
    case ModlTable:
    {
      struct ModlObject * obj = modl_table();
      obj->value.table = self->value.table;
      return obj;
    }
    case ModlFunction:
    {
      struct ModlObject * obj = ifun_to_modl(self->value.fun.context, self->value.fun.position);
      obj->value.fun.is_external = self->value.fun.is_external;
      return obj;
    }
  }

  printf("%s", "HERES THE COPY ERROR!!!\n");
}



bool modl_table_has_k(struct ModlObject * const self, struct ModlObject const * key)
{
  if (NULL == self)
  {
    printf("\x1b[31;1m  %s\x1b[0m\n", "((ModlObject*) self) == NULL!");
    exit(EXIT_FAILURE);
  }

  if (ModlTable != self->type) return FALSE;

  struct ModlTableNode * node = self->value.table;
  while (NULL != node->key)
  {
    if (modl_object_equals(node->key, key))
      return TRUE;
    node = node->next;
  }

  return FALSE;
}

void modl_table_insert_kv(struct ModlObject * const self, struct ModlObject const * const key, struct ModlObject * const value)
{
  if (NULL == self)
  {
    printf("\x1b[31;1m  %s\x1b[0m\n", "((ModlObject*) self) == NULL!");
    exit(EXIT_FAILURE);
  }

  if (ModlTable != self->type)
  {
    printf("\x1b[31;1m  %s\x1b[0m\n", "((ModlObject *) self)->type != ModlTable!");
    exit(EXIT_FAILURE);
  }

  if (ModlString != key->type && ModlInteger != key->type)
  {
    printf("\x1b[31;1m  %s\x1b[0m\n", "((ModlObject *) key)->type not in (ModlString, ModlInteger)!");
    exit(EXIT_FAILURE);
  }

  struct ModlTableNode * node = self->value.table;
  while (NULL != node->key)
  {
    if (modl_object_equals(node->key, key))
    {
      node->value = value;
      return;
    }
    node = node->next;
  }

  node->key = modl_object_copy(key);
  node->value = value;
  node->next = (struct ModlTableNode *) calloc(1, sizeof (struct ModlTableNode));
}

void modl_table_push_v(struct ModlObject * const self, struct ModlObject * const value)
{
  struct ModlObject * key = int_to_modl(0);
  while (modl_table_has_k(self, key))
    key->value.integer += 1;

  modl_table_insert_kv(self, key, value);
}

struct ModlObject * modl_table_get_v(struct ModlObject const * const self, struct ModlObject const * const key)
{
  if (NULL == self)
  {
    printf("\x1b[31;1m  %s\x1b[0m\n", "((ModlObject*) self) == NULL!");
    exit(EXIT_FAILURE);
  }

  if (ModlTable != self->type)
  {
    printf("\x1b[31;1m  %s\x1b[0m\n", "((ModlObject *) self)->type != ModlTable!");
    exit(EXIT_FAILURE);
  }

  struct ModlTableNode const * node = self->value.table;
  while (NULL != node->key)
  {
    if (modl_object_equals(node->key, key))
      return node->value;

    node = node->next;
  }

  return modl_nil();
}


struct ModlObject const * modl_maybe_cast(struct ModlObject const * const object, enum ModlType target_type)
{
  if (NULL == object) return NULL;
  if (object->type == target_type) return object;

  switch (object->type)
  {
    case ModlInteger: switch(target_type)
    {
      case ModlFloating: return double_to_modl((double) modl_to_int(object));
    } break;
  }

  return object;
}


struct Sebo
{
  byte * data;
  size_t byte_length;
  struct ModlObject * object;
};

struct Sebo modl_decode_sebo(byte * data)
{
  switch (*data)
  {
    case 0x00: return (struct Sebo) { data, 1, modl_nil() };
    case 0x01: return (struct Sebo) { data, 1, bool_to_modl(FALSE) };
    case 0x02: return (struct Sebo) { data, 1, bool_to_modl(TRUE) };
    case 0x03: return (struct Sebo) { data, 2, int_to_modl(data[1]) };
    case 0x04:
    {
      int32_t value =
          (((int32_t) data[1]) << 8*3)
        | (((int32_t) data[2]) << 8*2)
        | (((int32_t) data[3]) << 8*1)
        | (((int32_t) data[4]) << 8*0);
      return (struct Sebo) { data, 5, int_to_modl((int64_t) value) };
    }
    case 0x05:
    {
      double value;
      memcpy(&value, data + 1, 8);
      return (struct Sebo) { data, 9, double_to_modl(value) };
    }
    case 0x06:
    {
      char * value;
      struct Sebo const length = modl_decode_sebo(data + 1);
      value = (char *) malloc(modl_to_int(length.object) + 1);
      memcpy(value, data + 1 + length.byte_length, modl_to_int(length.object));
      value[modl_to_int(length.object)] = (byte) 0x00;
      return (struct Sebo) { data, 1 + length.byte_length + modl_to_int(length.object), str_to_modl(value) };
    }
    case 0x0A:
    {
      struct Sebo const length = modl_decode_sebo(data + 1);
      if (modl_to_int(length.object) > 0)
      {
        printf("\x1b[31;1m  %s: %s\x1b[0m\n", "failed to decode modl object", "table is not empty");
        exit(EXIT_FAILURE);
      }

      return (struct Sebo) { data, 1 + length.byte_length, modl_table() };
    }

    default:
    {
      printf("\x1b[31;1m  %s: %s %d\x1b[0m\n", "failed to decode modl object", "unknown type code", *data);
      exit(EXIT_FAILURE);
    }
  }
}

static byte array_of__0[] = { 0 };
static byte array_of__1[] = { 1 };
static byte array_of__2[] = { 2 };


struct Sebo modl_encode_sebo(struct ModlObject * object)
{
  if (NULL == object) return (struct Sebo) { NULL, 0, modl_nil() };

  switch (object->type)
  {
    case ModlNil: return (struct Sebo) { array_of__0, 1, object };
    case ModlBoolean: return (struct Sebo) { object->value.boolean ? array_of__2 : array_of__1, 1, object };
    case ModlInteger:
    {
      uint64_t value = (uint64_t) modl_to_int(object);
      struct Sebo ret = { malloc(9 * (sizeof (byte))), 9, object };
      ret.data[0] = 0x0B;
      ret.data[1] = (value >> 7*8) & 0xFF;
      ret.data[2] = (value >> 6*8) & 0xFF;
      ret.data[3] = (value >> 5*8) & 0xFF;
      ret.data[4] = (value >> 4*8) & 0xFF;
      ret.data[5] = (value >> 3*8) & 0xFF;
      ret.data[6] = (value >> 2*8) & 0xFF;
      ret.data[7] = (value >> 1*8) & 0xFF;
      ret.data[8] = (value >> 0*8) & 0xFF;
      return ret;
    };

    default:
    {
      printf("\x1b[31;1m  %s: ", "failed to encode modl object");
      display_modl_object(object);
      printf("%c\x1b[0m", '\n');

      exit(EXIT_FAILURE);
    }
  }
}


struct Environment
{
  struct Environment * parent;
  struct ModlObject * vartable;
};

struct CallFrame
{
  size_t return_address;
  struct Environment * environment;
};

struct RegisterAccessMonitor
{
  bool read_after_rewrite[VM_SETTING_REGITERS_COUNT];
  size_t last_write_points[VM_SETTING_REGITERS_COUNT];
};

struct VMState
{
  byte * code;

  size_t const max_count_call_stack, max_count_stack, max_count_externals;

  size_t ip, csp, sp;
  size_t efc;

  struct CallFrame  * call_stack;
  struct ModlObject * registers[VM_SETTING_REGITERS_COUNT];
  struct ModlObject * *stack;
  struct ModlObject * (* *external_functions) (struct VMState *);

  struct RegisterAccessMonitor ram;
};

struct CallFrame vm_get_current_call_frame(struct VMState const * const self)
{
  return self->call_stack[self->csp];
}

struct Environment * vm_get_current_environment(struct VMState const * const self)
{
  return vm_get_current_call_frame(self).environment;
}

uint64_t vm_add_external_function(struct VMState * self, struct ModlObject * (*ef) (struct VMState *))
{
  if (self->efc + 1 > self->max_count_externals)
  {
    printf(
      "\x1b[31;1m  %s: external_functions_max_count=%lu\x1b[0m\n",
      "  maximum external functions count exceeded",
      self->max_count_externals
    );
    exit(EXIT_FAILURE);
  }

  self->external_functions[self->efc++] = ef;
}

struct ModlObject * (* vm_get_external_function(struct VMState const * const self, uint64_t id)) (struct VMState *)
{
  if (id >= self->efc)
  {
    printf(
      "\x1b[31;1m  %s: id=%lu\x1b[0m\n",
      "  external function with given id does not exists",
      id
    );
    exit(EXIT_FAILURE);
  }

  return self->external_functions[id];
}



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
  enum TemplateParameter p[4];
};

struct InstructionParametersTemplate instruction_parameters_templates[256] =
{
  [OP_NOP]     = {{ TP_EMPTY, }},
  [OP_RET]     = {{ TP_EMPTY, }},

  [OP_LOADC]   = {{ TP_REGAL, TP_SEBO, }},
  [OP_TBLGETR] = {{ TP_REGSP, }},

  [OP_CALLR]   = {{ TP_REGAL, }},
  [OP_LOADFUN] = {{ TP_REGAL, TP_INT64, }},

  [OP_JMP]     = {{ TP_INT64, }},

  [OP_ADD]     = {{ TP_REGSP, }},
  [OP_SUB]     = {{ TP_REGSP, }},
  [OP_MUL]     = {{ TP_REGSP, }},
  [OP_DIV]     = {{ TP_REGSP, }},

  [OP_CMPEQ]   = {{ TP_REGSP, }},
  [OP_CMPNEQ]  = {{ TP_REGSP, }},
  [OP_CMPLT]   = {{ TP_REGSP, }},
  [OP_CMPNLT]  = {{ TP_REGSP, }},
  [OP_CMPGT]   = {{ TP_REGSP, }},
  [OP_CMPNGT]  = {{ TP_REGSP, }},

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
};

struct __attribute__ ((__packed__))
Instruction
{
  enum ModlOpcode opcode;
  size_t byte_length;

  union
  {
    int64_t i64;
    byte r[2];
    struct ModlObject * object;
  } a[4];
};

static struct Instruction decode_instruction(struct VMState * state)
{
  enum ModlOpcode opcode = state->code[state->ip];
  struct Instruction instruction = { .opcode = opcode };
  struct InstructionParametersTemplate template = instruction_parameters_templates[opcode];
  if (TP_ERROR == template.p[0])
  {
    char const * name = instructions_names_table[opcode];
    if (NULL == name) name = "#???#";
    printf("\x1b[31;1mfailed to decode instruction: %s\x1b[0m\n", name);
    exit(EXIT_FAILURE);
  }

  size_t offset = 1;
  for (byte i = 0; i < 4; ++i)
  {
    switch (template.p[i])
    {
      case TP_ERROR: case TP_EMPTY: break;

      case TP_REGAL:
      {
        instruction.a[i].r[0] = state->code[state->ip + offset] & 0xF;
        offset += 1;
      } break;

      case TP_REGSP:
      {
        instruction.a[i].r[0] = (state->code[state->ip + offset] >> 4) & 0xF;
        instruction.a[i].r[1] = (state->code[state->ip + offset] >> 0) & 0xF;
        offset += 1;
      } break;

      case TP_INT64:
      {
        instruction.a[i].i64 = ((int64_t) 0)
          | (((int64_t) state->code[state->ip + offset + 0]) << 8*7)
          | (((int64_t) state->code[state->ip + offset + 1]) << 8*6)
          | (((int64_t) state->code[state->ip + offset + 2]) << 8*5)
          | (((int64_t) state->code[state->ip + offset + 3]) << 8*4)
          | (((int64_t) state->code[state->ip + offset + 4]) << 8*3)
          | (((int64_t) state->code[state->ip + offset + 5]) << 8*2)
          | (((int64_t) state->code[state->ip + offset + 6]) << 8*1)
          | (((int64_t) state->code[state->ip + offset + 7]) << 8*0);
        offset += 8;
      } break;

      case TP_SEBO:
      {
        struct Sebo data = modl_decode_sebo(&state->code[state->ip] + offset);
        instruction.a[i].object = data.object;
        offset += data.byte_length;
      } break;
    }
  }

  instruction.byte_length = offset;
  return instruction;
}

static void display_instruction(struct Instruction instruction)
{
  printf("\x1b[36m%s\x1b[0m", instructions_names_table[instruction.opcode]);
  struct InstructionParametersTemplate template = instruction_parameters_templates[instruction.opcode];

  if (template.p[0] != TP_EMPTY)
    printf("%c", ' ');

  for (byte i = 0; i < 4 && template.p[i] > TP_EMPTY; ++i)
  {
    if (i > 0) printf("%s", ", ");
    switch (template.p[i])
    {
      case TP_REGAL:
        printf("\x1b[37;1mR%d\x1b[0m", instruction.a[i].r[0]);
        break;
      case TP_REGSP:
        printf("\x1b[37;1mR%d\x1b[0m, \x1b[37;1mR%d\x1b[0m", instruction.a[i].r[0], instruction.a[i].r[1]);
        break;
      case TP_INT64:
        printf("[%" PRId64 "]", instruction.a[i].i64);
        break;
      case TP_SEBO:
        display_modl_object(instruction.a[i].object);
        break;

      case TP_ERROR: case TP_EMPTY: /* unreachable */ break;
    }
  }

  printf("%c", '\n');
}



static struct ModlObject * vm_read_register(struct VMState * const state, byte r)
{
  state->ram.read_after_rewrite[r] = TRUE ;
  return state->registers[r];
}

static void vm_write_register(struct VMState * const state, byte r, struct ModlObject * val)
{
  if (not state->ram.read_after_rewrite[r])
  {
    printf(
      "\x1b[33;1m%s\x1b[0m[%04lx]\n",
      "  register value rewritten without previous reads at: ",
      state->ram.last_write_points[r]
    );
  }

  state->ram.read_after_rewrite[r] = FALSE;
  state->ram.last_write_points[r] = state->ip;
  state->registers[r] = val;
}


struct ModlObject * run(struct VMState * const state)
{
  while (TRUE)
  {
    {
      // printf("%s", "Registers: [ ");
      // for (size_t i = 0; i < VM_SETTING_REGITERS_COUNT; ++i)
      // {
      //   display_modl_object(state->registers[i]);
      //   printf("%c", ' ');
      // }
      // printf("%c\n", ']');
      //
      // printf("Stack(%lu): [ ", state->sp);
      // for (size_t i = 0; i < state->sp; ++i)
      // {
      //   display_modl_object(state->stack[i]);
      //   printf("%c", ' ');
      // }
      // printf("%c\n", ']');
      //
      // printf("%s", "Environment:");
      // struct Environment * env = state->call_stack[state->csp].environment;
      // while (NULL != env)
      // {
      //   printf("%c", ' ');
      //   display_modl_object(env->vartable);
      //   env = env->parent;
      // }
      // printf("%c", '\n');
    }

    printf("[%04lx] ", state->ip);
    struct Instruction instruction = decode_instruction(state);
    display_instruction(instruction);

    switch (instruction.opcode)
    {
      case OP_NOP: break;

      case OP_RET:
      {
        if (0 == state->csp) return state->registers[0];
        state->ip = state->call_stack[state->csp--].return_address - instruction.byte_length;
      } break;

      case OP_LOADC:
      {
        vm_write_register(state, instruction.a[0].r[0], instruction.a[1].object);
      } break;

      case OP_TBLGETR:
      {
        byte const reg_tbl = instruction.a[0].r[0];
        byte const reg_name = instruction.a[0].r[1];

        struct ModlObject const * const obj_tbl = vm_read_register(state, reg_tbl);
        struct ModlObject const * const obj_name = vm_read_register(state, reg_name);

        vm_write_register(
          state, reg_tbl,
          modl_table_get_v(obj_tbl, obj_name)
        );
      } break;

      case OP_CALLR:
      {
        struct ModlObject const * const obj = vm_read_register(state, instruction.a[0].r[0]);

        if (ModlFunction != obj->type)
        {
          printf("\x1b[31;1m  Object of type %d is not callable\x1b[0m\n", obj->type);
          exit(EXIT_FAILURE);
        }

        if (state->csp + 1 >= state->max_count_call_stack)
        {
          printf(
            "\x1b[31;1m  %s: call_stack_max_size=%lu\x1b[0m\n",
            "  maximum call stack size exceeded",
            state->max_count_call_stack
          );
          exit(EXIT_FAILURE);
        }

        if (obj->value.fun.is_external)
        {
          struct ModlObject * ret = vm_get_external_function(state, obj->value.fun.position)(state);
          if (NULL != ret)
            vm_write_register(state, REG(0), ret);
          break;
        }

        struct Environment * env = calloc(1, sizeof (struct Environment));
        env->parent = obj->value.fun.context;
        env->vartable = modl_table();
        state->call_stack[++state->csp] = (struct CallFrame) {
          .return_address = state->ip + instruction.byte_length,
          .environment = env,
        };
        state->ip = obj->value.fun.position - instruction.byte_length;
      } break;

      case OP_LOADFUN:
      {
        struct Environment * env = state->call_stack[state->csp].environment;
        vm_write_register(state, instruction.a[0].r[0], ifun_to_modl(env, state->ip + instruction.a[1].i64));
      } break;

      case OP_JMP: state->ip += instruction.a[0].i64 - instruction.byte_length; break;

      case OP_ADD:
      case OP_SUB:
      case OP_MUL:
      case OP_DIV:
      case OP_MOD:
      case OP_AND:
      case OP_OR:
      case OP_XOR:
      case OP_NAND:
      case OP_NOR:
      case OP_NXOR:
      case OP_CMPEQ:
      case OP_CMPNEQ:
      case OP_CMPLT:
      case OP_CMPNLT:
      case OP_CMPGT:
      case OP_CMPNGT:
      {
        byte const reg_dst = instruction.a[0].r[0];
        byte const reg_src = instruction.a[0].r[1];

        struct ModlObject const * obj_l = vm_read_register(state, reg_dst);
        struct ModlObject const * obj_r = vm_read_register(state, reg_src);

        if (ModlFloating == obj_l->type) obj_r = modl_maybe_cast(obj_r, ModlFloating);
        if (ModlFloating == obj_r->type) obj_l = modl_maybe_cast(obj_l, ModlFloating);

        if (obj_l->type != obj_r->type)
        {
          printf("\x1b[31;1m  Values are required to have the same type: %d <> %d\x1b[0m\n", obj_l->type, obj_r->type);
          exit(EXIT_FAILURE);
        }

        if (ModlInteger != obj_l->type && ModlFloating != obj_l->type)
        {
          printf(
            "\x1b[31;1m  This operation requires operands of integer or floating types: %d <> %d/%d\x1b[0m\n",
            obj_l->type, ModlInteger, ModlFloating
          );
          exit(EXIT_FAILURE);
        }

        switch (ModlInteger == obj_l->type)
        {
          case TRUE: switch (instruction.opcode)
          {
            case OP_ADD: vm_write_register(state, reg_dst, int_to_modl(modl_to_int(obj_l) + modl_to_int(obj_r))); break;
            case OP_SUB: vm_write_register(state, reg_dst, int_to_modl(modl_to_int(obj_l) - modl_to_int(obj_r))); break;
            case OP_MUL: vm_write_register(state, reg_dst, int_to_modl(modl_to_int(obj_l) * modl_to_int(obj_r))); break;
            case OP_DIV: vm_write_register(state, reg_dst, double_to_modl((double)modl_to_int(obj_l) / (double)modl_to_int(obj_r))); break;
            case OP_MOD: vm_write_register(state, reg_dst, int_to_modl(modl_to_int(obj_l) % modl_to_int(obj_r))); break;
            case OP_AND: vm_write_register(state, reg_dst, int_to_modl(modl_to_int(obj_l) & modl_to_int(obj_r))); break;
            case OP_OR: vm_write_register(state, reg_dst, int_to_modl(modl_to_int(obj_l) | modl_to_int(obj_r))); break;
            case OP_XOR: vm_write_register(state, reg_dst, int_to_modl(modl_to_int(obj_l) ^ modl_to_int(obj_r))); break;
            case OP_NAND: vm_write_register(state, reg_dst, int_to_modl(~(modl_to_int(obj_l) & modl_to_int(obj_r)))); break;
            case OP_NOR: vm_write_register(state, reg_dst, int_to_modl(~(modl_to_int(obj_l) | modl_to_int(obj_r)))); break;
            case OP_NXOR: vm_write_register(state, reg_dst, int_to_modl(~(modl_to_int(obj_l) ^ modl_to_int(obj_r)))); break;
            case OP_CMPEQ: vm_write_register(state, reg_dst, bool_to_modl(modl_to_int(obj_l) == modl_to_int(obj_r))); break;
            case OP_CMPNEQ: vm_write_register(state, reg_dst, bool_to_modl(modl_to_int(obj_l) != modl_to_int(obj_r))); break;
            case OP_CMPLT: vm_write_register(state, reg_dst, bool_to_modl(modl_to_int(obj_l) < modl_to_int(obj_r))); break;
            case OP_CMPNLT: vm_write_register(state, reg_dst, bool_to_modl(!(modl_to_int(obj_l) < modl_to_int(obj_r)))); break;
            case OP_CMPGT: vm_write_register(state, reg_dst, bool_to_modl(modl_to_int(obj_l) > modl_to_int(obj_r))); break;
            case OP_CMPNGT: vm_write_register(state, reg_dst, bool_to_modl(!(modl_to_int(obj_l) > modl_to_int(obj_r)))); break;
            default: break;
          } break;

          case FALSE: switch (instruction.opcode)
          {
            case OP_ADD: vm_write_register(state, reg_dst, double_to_modl(modl_to_double(obj_l) + modl_to_double(obj_r))); break;
            case OP_SUB: vm_write_register(state, reg_dst, double_to_modl(modl_to_double(obj_l) - modl_to_double(obj_r))); break;
            case OP_MUL: vm_write_register(state, reg_dst, double_to_modl(modl_to_double(obj_l) * modl_to_double(obj_r))); break;
            case OP_DIV: vm_write_register(state, reg_dst, double_to_modl(modl_to_double(obj_l) / modl_to_double(obj_r))); break;
            case OP_MOD: vm_write_register(state, reg_dst, double_to_modl(fmod(modl_to_double(obj_l), modl_to_double(obj_r)))); break;
            // case OP_AND: vm_write_register(state, reg_dst, double_to_modl(modl_to_double(obj_l) & modl_to_double(obj_r))); break;
            // case OP_OR: vm_write_register(state, reg_dst, double_to_modl(modl_to_double(obj_l) | modl_to_double(obj_r))); break;
            // case OP_XOR: vm_write_register(state, reg_dst, double_to_modl(modl_to_double(obj_l) ^ modl_to_double(obj_r))); break;
            // case OP_NAND: vm_write_register(state, reg_dst, double_to_modl(~(modl_to_double(obj_l) & modl_to_double(obj_r)))); break;
            // case OP_NOR: vm_write_register(state, reg_dst, double_to_modl(~(modl_to_double(obj_l) | modl_to_double(obj_r)))); break;
            // case OP_NXOR: vm_write_register(state, reg_dst, double_to_modl(~(modl_to_double(obj_l) ^ modl_to_double(obj_r)))); break;
            case OP_CMPEQ: vm_write_register(state, reg_dst, bool_to_modl(modl_to_double(obj_l) == modl_to_double(obj_r))); break;
            case OP_CMPNEQ: vm_write_register(state, reg_dst, bool_to_modl(modl_to_double(obj_l) != modl_to_double(obj_r))); break;
            case OP_CMPLT: vm_write_register(state, reg_dst, bool_to_modl(modl_to_double(obj_l) < modl_to_double(obj_r))); break;
            case OP_CMPNLT: vm_write_register(state, reg_dst, bool_to_modl(!(modl_to_double(obj_l) < modl_to_double(obj_r)))); break;
            case OP_CMPGT: vm_write_register(state, reg_dst, bool_to_modl(modl_to_double(obj_l) > modl_to_double(obj_r))); break;
            case OP_CMPNGT: vm_write_register(state, reg_dst, bool_to_modl(!(modl_to_double(obj_l) > modl_to_double(obj_r)))); break;
            default:
            {
              printf("\x1b[31;1m  this operation is not supported on floats\x1b[0m\n");
              exit(EXIT_FAILURE);
            } break;
          } break;
        }
      } break;

      case OP_JCF:
      case OP_JCT:
      {
        if (modl_to_bool(vm_read_register(state, instruction.a[0].r[0])) == (instruction.opcode == OP_JCT))
          state->ip += instruction.a[1].i64 - instruction.byte_length;
      } break;

      case OP_POP:
      {
        if (0 == state->sp)
        {
          printf("\x1b[31;1m  Cannot pop from empty stack\x1b[0m\n");
          exit(EXIT_FAILURE);
        }

        vm_write_register(state, instruction.a[0].r[0], state->stack[--state->sp]);
      } break;

      case OP_PUSH:
      {
        if (state->sp + 1 > state->max_count_stack)
        {
          printf(
            "\x1b[31;1m  %s: stack_max_size=%lu\x1b[0m\n",
            "  maximum stack size exceeded",
            state->max_count_stack
          );
          exit(EXIT_FAILURE);
        }

        state->stack[state->sp++] = vm_read_register(state, instruction.a[0].r[0]);
      } break;

      case OP_TBLPUSH:
      {
        modl_table_push_v(
          vm_read_register(state, instruction.a[0].r[0]),
          vm_read_register(state, instruction.a[0].r[1])
        );
      } break;

      case OP_TBLSETR:
      {
        modl_table_insert_kv(
          vm_read_register(state, instruction.a[0].r[0]),
          vm_read_register(state, instruction.a[0].r[1]),
          vm_read_register(state, instruction.a[1].r[0])
        );
      } break;

      case OP_ENVGETC:
      {
        struct Environment * env = state->call_stack[state->csp].environment;

        while (NULL != env)
        {
          if (modl_table_has_k(env->vartable, instruction.a[1].object))
          {
            vm_write_register(
              state, instruction.a[0].r[0],
              modl_table_get_v(
                env->vartable,
                instruction.a[1].object
              )
            );
            goto CASE_OP_ENVGETC_END;
          }
          env = env->parent;
        }
        vm_write_register(state, instruction.a[0].r[0], modl_nil());
        CASE_OP_ENVGETC_END: break;
      } break;

      case OP_ENVSETC:
      {
        modl_table_insert_kv(
          state->call_stack[state->csp].environment->vartable,
          instruction.a[1].object,
          vm_read_register(state, instruction.a[0].r[0])
        );
      } break;

      default:
      {
        printf("\x1b[31;1m  Instruction implementation not found\x1b[0m\n");
        exit(EXIT_FAILURE);
      } break;
    }

    state->ip += instruction.byte_length;
  }

  return NULL;
}


struct BytecodeCompiler
{
  byte* data;
  size_t offset;
};

size_t emit(struct BytecodeCompiler * compiler, byte value)
{
  size_t addr = compiler->offset;
  compiler->data[compiler->offset++] = value;
  return addr;
}

size_t emit_modl(struct BytecodeCompiler * compiler, struct ModlObject * object)
{
  struct Sebo repr = modl_encode_sebo(object);
  if (0 == repr.byte_length)
  {
    printf("%s\n", "Cannot encode NULL object");
    exit(EXIT_FAILURE);
  }

  for (size_t i = 0; i < repr.byte_length; ++i)
    emit(compiler, repr.data[i]);

  return repr.byte_length;
}


static struct ModlObject * modl_std_print(struct VMState * vm)
{
  display_modl_object(vm->stack[--vm->sp]);
  printf("%c", '\n');
  return NULL;
}


#define INPUT_SIZE 1024*1024*1
int main(int argc, char *argv[])
{
  int opt, long_index;

  byte input[INPUT_SIZE] = { [0 ... (INPUT_SIZE - 1)] = OP_RET };
  size_t input_length = 0;
  size_t max_count_call_stack = 64;
  size_t max_count_stack = 128;
  size_t max_count_externals = 128;
  double time_spent = 0.0;

  clock_t begin = clock();

  static struct option long_options[] = {
        {"stack_size",      required_argument, 0,  's' },
        {"call_stack_size", required_argument, 0,  'c' },
        {0,                 0,                 0,  0   }
    };

  while((opt = getopt_long(argc, argv, ":i:s:c", long_options, &long_index)) != -1)
  {
    switch(opt)
    {
      case 'i':
      {
        while (*optarg) switch(*optarg)
        {
          case '\0': break;
          case '\\': switch (*++optarg)
            {
                case 'a': input[input_length++] = '\a'; ++optarg; break;
                case 'b': input[input_length++] = '\b'; ++optarg; break;
                case 'f': input[input_length++] = '\f'; ++optarg; break;
                case 'n': input[input_length++] = '\n'; ++optarg; break;
                case 'r': input[input_length++] = '\r'; ++optarg; break;
                case 't': input[input_length++] = '\t'; ++optarg; break;
                case 's': input[input_length++] =  ' '; ++optarg; break;
                case '\\': input[input_length++] = '\\'; ++optarg; break;

                case '0': input[input_length++] = '\0'; ++optarg; break;

                case 'x': input[input_length++] = ((unsigned char)(decode_arbitrary_char(optarg[1]) << 4)
                                                    | decode_arbitrary_char(optarg[2])); optarg += 3; break;

                default: input[input_length++] = *optarg++; break;
            } break;
          default: input[input_length++] = *optarg++; break;
        }
      } break;

      case 's':
      {
        max_count_stack = atoi(optarg);
      } break;

      case 'c':
      {
        max_count_call_stack = atoi(optarg);
      } break;

      case ':':
      {
        printf("option needs a value\n");
        return 1;
      } break;

      case '?':
      {
        printf("unknown option: %c\n", optopt);
        return 1;
      } break;
    }
  }


  struct Environment base_environment = { NULL, modl_table() };
  struct VMState vm =
  {
    .code = NULL,

    .max_count_call_stack = max_count_call_stack,
    .max_count_stack = max_count_stack,
    .max_count_externals = 16,

    .ip = 0, .csp = 0, .sp = 0, .efc = 0,
    .call_stack = (struct CallFrame *) malloc(max_count_call_stack * sizeof (struct CallFrame)),
    .registers = { [0 ... 15] = modl_nil() },
    .stack = (struct ModlObject **) (malloc(max_count_stack * sizeof (struct ModlObject **))),
    .external_functions = (struct ModlObject*(**)(struct VMState*))(malloc(max_count_externals*sizeof(struct ModlObject*(**)(struct VMState*)))),

    .ram = {
      .read_after_rewrite = { [0 ... 15] = TRUE },
      .last_write_points = { 0, },
    },
  };
  vm.call_stack[0] = (struct CallFrame) { 0, &base_environment };

  uint64_t std_print_id = vm_add_external_function(&vm, modl_std_print);

  modl_table_insert_kv(base_environment.vartable, str_to_modl("print"), efun_to_modl(std_print_id));
  modl_table_insert_kv(base_environment.vartable, str_to_modl("stdout"), efun_to_modl(std_print_id));


  // struct BytecodeCompiler bcc = { (enum ModlOpcode*) calloc(16, sizeof(byte)), 0 };
  // emit(&bcc, OP_LOADC);
  // emit(&bcc, REG(0));
  // emit_modl(&bcc, int_to_modl(42));
  //
  // emit(&bcc, OP_PUSH);
  // emit(&bcc, REG(0));
  //
  // emit(&bcc, OP_LOADC);
  // emit(&bcc, REG(0));
  // emit_modl(&bcc, int_to_modl(400));
  //
  // emit(&bcc, OP_LOADC);
  // emit(&bcc, REG(1));
  // emit_modl(&bcc, int_to_modl(-123));
  //
  // emit(&bcc, OP_ADD);
  // emit(&bcc, REGS(REG(0), REG(1)));
  //
  // emit(&bcc, OP_POP);
  // emit(&bcc, REG(1));
  //
  // emit(&bcc, OP_SUB);
  // emit(&bcc, REGS(REG(0), REG(1)));
  //
  // emit(&bcc, OP_RET);

  printf("\x1b[34;1m%s\x1b[0m\n",   "-----=====      RUN       =====-----");
  vm.code = input;
  struct ModlObject * result = run(&vm);
  printf("\n\x1b[34;1m%s\x1b[0m\n", "-----=====     RESULT     =====-----");
  display_modl_object(result);
  printf("%c", '\n');

  clock_t end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

  printf("Time elapsed is %f seconds\n", time_spent);

  return EXIT_SUCCESS;
}
