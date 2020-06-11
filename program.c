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
bool VM_SETTING_SILENT = FALSE;


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
};


enum __attribute__ ((__packed__))
ModlType
{
  ModlTypeNil      = 0,
  ModlTypeBoolean  = 1,
  ModlTypeInteger  = 2,
  ModlTypeFloating = 3,
  ModlTypeString   = 4,
  ModlTypeTable    = 5,
  ModlTypeFunction = 6,
};

static char const * const modl_types_names_table[256] =
{
  [ModlTypeNil]      = "Nil",
  [ModlTypeBoolean]  = "Boolean",
  [ModlTypeInteger]  = "Integer",
  [ModlTypeFloating] = "Floating",
  [ModlTypeString]   = "String",
  [ModlTypeTable]    = "Table",
  [ModlTypeFunction] = "Function",
};


struct ModlObject
{
  enum ModlType type;
  union
  {
    bool boolean;
    int64_t integer;
    double floating;
    char * string;

    struct ModlTable
    {
      struct ModlTableNode
      {
        struct ModlObject const * key;
        struct ModlObject * value;
        struct ModlTableNode * next;
      } *integer_nodes, *string_nodes
      , *last_consecutive_integer_node;
    } table;

    struct ModlTypeFunctionInfo
    {
      bool is_external;
      uint64_t position;
      struct Environment * context;
    } fun;
  } value;

  size_t instances_count;
};


void display_modl_object(struct ModlObject const * object);

bool modl_object_release(struct ModlObject * const self)
{
  if (NULL == self) return TRUE;
  if (self->type == ModlTypeNil) return TRUE;

  self->instances_count -= 1;

  if (self->instances_count >= (size_t)-1)
  {
    printf("\x1b[31;1m  %s  --> %ld\x1b[0m\n", "Liek watafak? instances count is negative?!?!", self->instances_count);
    exit(EXIT_FAILURE);
  }

  if (self->instances_count == 0)
  {
    // printf("  %s: ", "destroying object");
    // display_modl_object(self);
    // printf("%c", '\n');

    switch (self->type)
    {
      case ModlTypeString: free(self->value.string); break;

      case ModlTypeTable:
      {
        {
          struct ModlTableNode * node = self->value.table.integer_nodes;
          while (NULL != node->key)
          {
            /* discard const to free object's memory */
            modl_object_release((struct ModlObject *) node->key);
            modl_object_release(node->value);

            struct ModlTableNode * tmp = node;
            node = node->next;
            free(tmp);
          }
          free(node);
        }
        {
          struct ModlTableNode * node = self->value.table.string_nodes;
          while (NULL != node->key)
          {
            /* discard const to free object's memory */
            modl_object_release((struct ModlObject *) node->key);
            modl_object_release(node->value);

            struct ModlTableNode * tmp = node;
            node = node->next;
            free(tmp);
          }
          free(node);
        }
      } break;

      default: break;
    }

    free(self);
    return TRUE;
  }

  return FALSE;
}

bool modl_object_release_tmp(struct ModlObject * const self)
{
  if (NULL == self) return TRUE;
  if (ModlTypeNil == self->type) return TRUE;

  self->instances_count += 1;
  return modl_object_release(self);
}

struct ModlObject * modl_object_disown(struct ModlObject * const self)
{
  if (NULL == self) return NULL;
  if (ModlTypeNil == self->type) return self;

  self->instances_count -= 1;
  if (self->instances_count >= (size_t)-1)
  {
    printf("\x1b[31;1m  %s  --> %ld\x1b[0m\n", "Liek watafak? instances count is negative?!?!", self->instances_count);
    exit(EXIT_FAILURE);
  }

  return self;
}


struct ModlObject * modl_object_take(struct ModlObject * const self)
{
  if (NULL == self) return NULL;
  if (ModlTypeNil == self->type) return self;
  self->instances_count += 1;
  return self;
}

bool modl_object_is_tmp(struct ModlObject const * object)
{
  if (NULL == object) return TRUE;
  return object->instances_count == 0;
}


void display_modl_object(struct ModlObject const * object)
{
  if (NULL == object)
  {
    printf("\x1b[31;1m%s\x1b[0m", "undefined");
    return;
  }

  printf("{%lu}", object->instances_count);
  switch (object->type)
  {
    case ModlTypeNil:
    {
      printf("\x1b[37;1m%s\x1b[0m", "nil");
    } break;

    case ModlTypeBoolean:
    {
      printf("%s", object->value.boolean ? "true" : "false");
    } break;

    case ModlTypeInteger:
    {
      printf("\x1b[33m%ld\x1b[0m", object->value.integer);
    } break;

    case ModlTypeFloating:
    {
      printf("\x1b[33m%.17g\x1b[0m", object->value.floating);
    } break;

    case ModlTypeString:
    {
      printf("\x1b[32m\"%s\"\x1b[0m", object->value.string);
    } break;

    case ModlTypeTable:
    {
      printf("%c", '[');

      {
        int64_t last_key = -1;
        struct ModlTableNode const * node = object->value.table.integer_nodes;
        while (NULL != node->key)
        {
          if (node->key->value.integer != last_key + 1)
          {
            display_modl_object(node->key);
            printf("%s", ": ");
          }
          display_modl_object(node->value);

          if (NULL != node->next->key)
            printf("%s", "; ");

          last_key = node->key->value.integer;
          node = node->next;
        }
      }
      {
        struct ModlTableNode const * node = object->value.table.string_nodes;
        while (NULL != node->key)
        {
          display_modl_object(node->key);
          printf("%s", ": ");
          display_modl_object(node->value);

          if (NULL != node->next->key)
            printf("%s", "; ");

          node = node->next;
        }
      }
      printf("%c", ']');
    } break;

    case ModlTypeFunction:
    {
      printf("\x1b[33;1m&%04lx\x1b[0m", object->value.fun.position);
    } break;
  }
}


struct ModlObject * modl_nil()
{
  static struct ModlObject nil_object = {
    .type = ModlTypeNil,
    .instances_count = 1,
  };

  return &nil_object;

  // struct ModlObject * object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  // object->type = ModlTypeNil;
  // object->instances_count = 0;
  // return object;
}

struct ModlObject * modl_nil_new()
{
  return modl_nil();
  // struct ModlObject * object = modl_nil();
  // object->instances_count = 1;
  // return object;
}

struct ModlObject * modl_table()
{
  struct ModlObject * object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  object->type = ModlTypeTable;
  object->value.table.integer_nodes = (struct ModlTableNode *) calloc(1, sizeof (struct ModlTableNode));
  object->value.table.last_consecutive_integer_node = NULL;
  object->value.table.string_nodes = (struct ModlTableNode *) calloc(1, sizeof (struct ModlTableNode));
  object->instances_count = 0;
  return object;
}

struct ModlObject * modl_table_new()
{
  struct ModlObject * object = modl_table();
  object->instances_count = 1;
  return object;
}



bool modl_object_equals(struct ModlObject const * const self, struct ModlObject const * const other)
{
  if (NULL == self || NULL == other) return FALSE;
  if (self == other) return TRUE;
  if (self->type != other->type) return FALSE;

  switch (self->type)
  {
    case ModlTypeNil: return TRUE;
    case ModlTypeBoolean: return self->value.boolean == other->value.boolean;
    case ModlTypeInteger: return self->value.integer == other->value.integer;
    case ModlTypeFloating: return self->value.floating == other->value.floating;
    case ModlTypeString: return 0 == strcmp(self->value.string, other->value.string);
    case ModlTypeTable: return FALSE;
    case ModlTypeFunction:
      return self->value.fun.is_external == other->value.fun.is_external
          && self->value.fun.position    == other->value.fun.position
          && self->value.fun.context     == other->value.fun.context;

    default: return FALSE;
  }
}


struct ModlObject * bool_to_modl(bool data)
{
  struct ModlObject* object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  object->type = ModlTypeBoolean;
  object->value.boolean = data;
  object->instances_count = 0;
  return object;
}

struct ModlObject * int_to_modl(int64_t data)
{
  struct ModlObject* object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  object->type = ModlTypeInteger;
  object->value.integer = data;
  object->instances_count = 0;
  return object;
}

struct ModlObject * double_to_modl(double data)
{
  struct ModlObject* object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  object->type = ModlTypeFloating;
  object->value.floating = data;
  object->instances_count = 0;
  return object;
}

struct ModlObject * str_to_modl(char const * data)
{
  struct ModlObject* object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  object->type = ModlTypeString;
  object->value.string = strcpy(malloc((strlen(data) + 1) * sizeof(char)), data);
  object->instances_count = 0;
  return object;
}

struct ModlObject * ifun_to_modl(struct Environment * environment, uint64_t position)
{
  struct ModlObject* object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  object->type = ModlTypeFunction;
  object->value.fun.is_external = FALSE;
  object->value.fun.position = position;
  object->value.fun.context = environment;
  object->instances_count = 0;
  return object;
}

struct ModlObject * efun_to_modl(uint64_t pointer)
{
  struct ModlObject* object = (struct ModlObject*) malloc(sizeof (struct ModlObject));
  object->type = ModlTypeFunction;
  object->value.fun.is_external = TRUE;
  object->value.fun.position = pointer;
  object->value.fun.context = NULL;
  object->instances_count = 0;
  return object;
}


bool modl_to_bool(struct ModlObject const * object)
{
  if (NULL == object) return FALSE;
  // if (ModlTypeBoolean != object->type) return TRUE;
  return (object->value.boolean ? TRUE : FALSE);
}

int64_t modl_to_int(struct ModlObject const * object)
{
  if (NULL == object) return 0;
  // if (ModlTypeInteger != object->type) return 0;
  return object->value.integer;
}

double modl_to_double(struct ModlObject const * object)
{
  if (NULL == object) return 0.0;
  // if (ModlTypeFloating != object->type) return 0.0;
  return object->value.floating;
}

char const * modl_to_str(struct ModlObject const * object)
{
  if (NULL == object) return NULL;
  if (ModlTypeString != object->type) return NULL;
  return object->value.string;
}


struct ModlObject * modl_object_copy_value(struct ModlObject const * const self)
{
  if (NULL == self) return NULL;

  // printf("  copying object by val: ");
  // display_modl_object(self);
  // printf("%c", '\n');

  if (modl_object_is_tmp(self))
  {
    // printf("%s\n", "    object is temporary; transfering ownership");
    return ((struct ModlObject *) self);
  }

  switch (self->type)
  {
    case ModlTypeNil: return modl_nil();
    case ModlTypeBoolean: return bool_to_modl(self->value.boolean);
    case ModlTypeInteger: return int_to_modl(self->value.integer);
    case ModlTypeFloating: return double_to_modl(self->value.floating);
    case ModlTypeString: return str_to_modl(strcpy(malloc((strlen(self->value.string) + 1) * sizeof (char)), self->value.string));
    case ModlTypeTable:
    {
      // TODO: implement
      printf("\x1b[31;1m  %s\x1b[0m\n", "ModlTypeTable copy by value is not implemented");
      exit(EXIT_FAILURE);
      return NULL;

      // struct ModlObject * obj = modl_table();
      // obj->value.table = self->value.table;
      // return obj;
    }
    case ModlTypeFunction:
    {
      struct ModlObject * obj = ifun_to_modl(self->value.fun.context, self->value.fun.position);
      obj->value.fun.is_external = self->value.fun.is_external;
      return obj;
    }

    default: return NULL;
  }
}

struct ModlObject * modl_object_copy_ref(struct ModlObject * const self)
{
  if (NULL == self) return NULL;

  // printf("  copying object by ref: ");
  // display_modl_object(self);
  // printf("%c", '\n');

  if (modl_object_is_tmp(self))
  {
    // printf("%s\n", "    object is temporary; transfering ownership");
    return self;
  }

  switch (self->type)
  {
    case ModlTypeNil: return self; // modl_nil();
    case ModlTypeBoolean: return self; // bool_to_modl(self->value.boolean);
    case ModlTypeInteger: return self; // int_to_modl(self->value.integer);
    case ModlTypeFloating: return self; // double_to_modl(self->value.floating);
    case ModlTypeString: return self; // str_to_modl(strcpy(malloc((strlen(self->value.string) + 1) * sizeof (char)), self->value.string));
    case ModlTypeTable:
    {
      // self->instances_count += 1;
      return self;

      // struct ModlObject * obj = modl_table();
      // obj->value.table = self->value.table;
      // return obj;
    }
    case ModlTypeFunction:
    {
      struct ModlObject * obj = ifun_to_modl(self->value.fun.context, self->value.fun.position);
      obj->value.fun.is_external = self->value.fun.is_external;
      return obj;
    }

    default: return NULL;
  }
}



bool modl_table_has_k(struct ModlObject * const self, struct ModlObject const * key)
{
  if (NULL == self)
  {
    printf("\x1b[31;1m  %s\x1b[0m\n", "((ModlObject*) self) == NULL!");
    exit(EXIT_FAILURE);
  }

  if (ModlTypeTable != self->type) return FALSE;

  struct ModlTableNode * node =
    key->type == ModlTypeInteger
      ? self->value.table.integer_nodes
      : self->value.table.string_nodes;

  while (NULL != node->key)
  {
    if (modl_object_equals(node->key, key))
      return TRUE;
    node = node->next;
  }

  return FALSE;
}

void modl_table_insert_kv(struct ModlObject * const self, struct ModlObject * const key, struct ModlObject * const value)
{
  if (NULL == self)
  {
    printf("\x1b[31;1m  %s\x1b[0m\n", "((ModlObject*) self) == NULL!");
    exit(EXIT_FAILURE);
  }

  if (ModlTypeTable != self->type)
  {
    printf("\x1b[31;1m  %s\x1b[0m\n", "((ModlObject *) self)->type != ModlTypeTable!");
    exit(EXIT_FAILURE);
  }

  if (ModlTypeString != key->type && ModlTypeInteger != key->type)
  {
    printf("\x1b[31;1m  %s\x1b[0m\n", "((ModlObject *) key)->type not in (ModlTypeString, ModlTypeInteger)!");
    exit(EXIT_FAILURE);
  }

  if (ModlTypeInteger == key->type && NULL != self->value.table.last_consecutive_integer_node
      && self->value.table.last_consecutive_integer_node->key->value.integer + 1 == key->value.integer)
  {
    struct ModlTableNode * cn = self->value.table.last_consecutive_integer_node;
    struct ModlTableNode * ins_next = (struct ModlTableNode *) malloc(sizeof (struct ModlTableNode));
    ins_next->key = modl_object_take(modl_object_copy_ref(key));
    ins_next->value = modl_object_take(modl_object_copy_ref(value));
    ins_next->next = cn->next;
    cn->next = ins_next;

    self->value.table.last_consecutive_integer_node = ins_next;
    while (NULL != self->value.table.last_consecutive_integer_node->next->key
        && (self->value.table.last_consecutive_integer_node->next->key->value.integer
            == self->value.table.last_consecutive_integer_node->key->value.integer + 1))
      self->value.table.last_consecutive_integer_node = self->value.table.last_consecutive_integer_node->next;
    return;
  }

  struct ModlTableNode * node =
    key->type == ModlTypeInteger
      ? self->value.table.integer_nodes
      : self->value.table.string_nodes;

  while (NULL != node->key)
  {
    if (modl_object_equals(node->key, key))
    {
      modl_object_release(node->value);
      node->value = modl_object_take(modl_object_copy_ref(value));
      return;
    }
    else if (ModlTypeInteger == key->type && key->value.integer < node->key->value.integer)
    {
      struct ModlTableNode * ins_next = (struct ModlTableNode *) malloc(sizeof (struct ModlTableNode));
      ins_next->key = modl_object_take(modl_object_copy_ref(key));
      ins_next->value = modl_object_take(modl_object_copy_ref(value));
      ins_next->next = node->next;
      node->next = ins_next;

      if (key->value.integer == 0)
        self->value.table.last_consecutive_integer_node = ins_next;

      if (NULL == self->value.table.last_consecutive_integer_node)
        return;

      while (NULL != self->value.table.last_consecutive_integer_node->next->key
          && (self->value.table.last_consecutive_integer_node->next->key->value.integer
              == self->value.table.last_consecutive_integer_node->key->value.integer + 1))
        self->value.table.last_consecutive_integer_node = self->value.table.last_consecutive_integer_node->next;
      return;
    }
    node = node->next;
  }

  if (ModlTypeInteger == key->type && key->value.integer == 0)
    self->value.table.last_consecutive_integer_node = node;
  else if (ModlTypeInteger == key->type && NULL != self->value.table.last_consecutive_integer_node
    && self->value.table.last_consecutive_integer_node->key->value.integer + 1 == key->value.integer)
    self->value.table.last_consecutive_integer_node = node;


  node->key = modl_object_take(modl_object_copy_ref(key));
  node->value = modl_object_take(modl_object_copy_ref(value));
  node->next = (struct ModlTableNode *) calloc(1, sizeof (struct ModlTableNode));
}

void modl_table_push_v(struct ModlObject * const self, struct ModlObject * const value)
{
  struct ModlTableNode const * ln = self->value.table.last_consecutive_integer_node;
  int64_t next_id = 0;
  if (NULL != ln) next_id = ln->key->value.integer + 1;
  modl_table_insert_kv(self, int_to_modl(next_id), value);
}

struct ModlObject * modl_table_get_v(struct ModlObject const * const self, struct ModlObject const * const key)
{
  if (NULL == self)
  {
    printf("\x1b[31;1m  %s\x1b[0m\n", "((ModlObject*) self) == NULL!");
    exit(EXIT_FAILURE);
  }

  if (ModlTypeTable != self->type)
  {
    printf("\x1b[31;1m  %s\x1b[0m\n", "((ModlObject *) self)->type != ModlTypeTable!");
    exit(EXIT_FAILURE);
  }

  struct ModlTableNode const * node =
    key->type == ModlTypeInteger
      ? self->value.table.integer_nodes
      : self->value.table.string_nodes;

  while (NULL != node->key)
  {
    if (modl_object_equals(node->key, key))
      return node->value;

    node = node->next;
  }

  return modl_nil();
}


struct ModlObject * modl_maybe_cast(struct ModlObject * const object, enum ModlType target_type)
{
  if (NULL == object) return NULL;

  // printf("\x1b[32m  casting object from %d to %d\x1b[0m\n", object->type, target_type);
  if (object->type == target_type) return object;

  switch (object->type)
  {
    case ModlTypeInteger: switch(target_type)
    {
      case ModlTypeFloating: return double_to_modl((double) modl_to_int(object));
      default: break;
    } break;
    default: break;
  }

  {
    printf(
      "\x1b[31;1m  Cannot cast object of type %s to %s\x1b[0m\n",
      modl_types_names_table[object->type],
      modl_types_names_table[target_type]
    );
    exit(EXIT_FAILURE);
  }

  return NULL;
}


struct Sebo
{
  byte * data;
  size_t byte_length;
  struct ModlObject * object;
};

double double_reverse_endianness(byte const * data)
{
  double result;

  char *dest = ((char *) &result) + (sizeof (double));
  for(size_t i = 0; i < sizeof(double); ++i)
    *--dest = *data++;

  return result;
}

/** Adjust the byte order from network to host.
    On a big endian machine this is a NOP.
*/
double ntohd(double src)
{
# if __FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return double_reverse_endianness((byte const *) &src);
# else
  return src;
# endif
}

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
      return (struct Sebo) { data, 9, double_to_modl(ntohd(value)) };
    }
    case 0x06:
    {
      char * value;
      struct Sebo const length = modl_decode_sebo(data + 1);
      size_t length_i = (size_t) modl_to_int(length.object);
      modl_object_release_tmp(length.object);

      value = (char *) malloc(length_i + 1);
      memcpy(value, data + 1 + length.byte_length, length_i);
      value[length_i] = (byte) 0x00;

      struct ModlObject * obj = str_to_modl(value);
      free(value);

      return (struct Sebo) { data, 1 + length.byte_length + length_i, obj };
    }
    case 0x0A:
    {
      struct Sebo const length = modl_decode_sebo(data + 1);
      if (modl_to_int(length.object) > 0)
      {
        printf("\x1b[31;1m  %s: %s\x1b[0m\n", "failed to decode modl object", "table is not empty");
        exit(EXIT_FAILURE);
      }

      // size_t length_i = (size_t) modl_to_int(length.object);
      modl_object_release_tmp(length.object);

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
  printf("%s\n", "  encoding sebo");
  if (NULL == object) return (struct Sebo) { NULL, 0, NULL };

  switch (object->type)
  {
    case ModlTypeNil: return (struct Sebo) { array_of__0, 1, object };
    case ModlTypeBoolean: return (struct Sebo) { object->value.boolean ? array_of__2 : array_of__1, 1, object };
    case ModlTypeInteger:
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

  self->external_functions[self->efc] = ef;
  return self->efc++;
}

struct ModlObject * (* vm_get_external_function(struct VMState const * const self, uint64_t id)) (struct VMState *)
{
  if (id >= self->efc)
  {
    printf(
      "\x1b[31;1m  %s: id=%lu\x1b[0m\n",
      "  external function with given id does not exist",
      id
    );
    exit(EXIT_FAILURE);
  }

  return self->external_functions[id];
}

struct ModlObject * vm_find_external_function(struct VMState const * const self, struct ModlObject * (*f) (struct VMState *))
{
  uint64_t id = 0;
  while (id < self->efc && self->external_functions[id] != f) ++id;

  if (id >= self->efc)
  {
    printf(
      "\x1b[31;1m  %s\x1b[0m\n",
      "  given external function is not registered"
    );
    exit(EXIT_FAILURE);
  }

  return efun_to_modl(id);
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

static void instruction_display(struct VMState * vm, struct Instruction instruction)
{
  printf("\x1b[36m%-8s\x1b[0m", instructions_names_table[instruction.opcode]);
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
        printf("%c", '{');
        display_modl_object(vm->registers[instruction.a[i].r[0]]);
        printf("%c", '}');
        break;
      case TP_REGSP:
        printf("\x1b[37;1mR%d\x1b[0m", instruction.a[i].r[0]);
        printf("%c", '{');
        display_modl_object(vm->registers[instruction.a[i].r[0]]);
        printf("%c", '}');
        printf(", \x1b[37;1mR%d\x1b[0m", instruction.a[i].r[1]);
        printf("%c", '{');
        display_modl_object(vm->registers[instruction.a[i].r[1]]);
        printf("%c", '}');
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

void instruction_release(struct Instruction instruction)
{
  struct InstructionParametersTemplate template = instruction_parameters_templates[instruction.opcode];
  for (byte i = 0; i < 4; ++i)
  {
    switch (template.p[i])
    {
      case TP_SEBO:
        // printf("%s\n", "  releasing instruction's temporary object");
        modl_object_release_tmp(instruction.a[i].object);
      default: break;
    }
  }
}


struct ModlObject * vm_reg_read(struct VMState * const state, byte r)
{
  state->ram.read_after_rewrite[r] = TRUE;
  return state->registers[r];
}

void vm_reg_write(struct VMState * const state, byte r, struct ModlObject * val)
{
  if (not VM_SETTING_SILENT)
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

  /** make object copy before releasing
   *    to avoid accidental memory freeing when `val == state->registers[r]`
  **/
  struct ModlObject * copy = modl_object_take(val);
  modl_object_release(state->registers[r]);
  state->registers[r] = copy;
}


struct ModlObject * run(struct VMState * const state);
struct ModlObject * vm_call_function(struct VMState * const state, struct ModlObject const * obj)
{
  if (ModlTypeFunction != obj->type)
  {
    printf("\x1b[31;1m  Object of type %s is not callable\x1b[0m\n", modl_types_names_table[obj->type]);
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

  struct Environment * env = calloc(1, sizeof (struct Environment));
  env->parent = obj->value.fun.context;
  env->vartable = modl_table_new();
  state->call_stack[++state->csp] = (struct CallFrame) {
    .return_address = state->ip,
    .environment = env,
  };

  struct ModlObject * ret;

  if (obj->value.fun.is_external)
  {
    ret = vm_get_external_function(state, obj->value.fun.position)(state);
    if (NULL == ret)
    {
      if (not VM_SETTING_SILENT)
        printf("\x1b[33;1m%s\x1b[0m", "    forcing nil as output");
      ret = modl_nil_new();
    }
    vm_reg_write(state, REG(0), ret);
  }
  else
  {
    state->ip = obj->value.fun.position;
    ret = run(state);
  }

  modl_object_release(env->vartable);
  free(env);
  state->ip = state->call_stack[state->csp].return_address;
  state->csp -= 1;

  modl_object_release_tmp((struct ModlObject *) obj);
  return ret;
}


struct ModlObject * run(struct VMState * const state)
{
  while (TRUE)
  {
    if (not VM_SETTING_SILENT)
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

    if (not VM_SETTING_SILENT) printf("[%04lx] ", state->ip);
    struct Instruction instruction = decode_instruction(state);
    if (not VM_SETTING_SILENT) instruction_display(state, instruction);

    switch (instruction.opcode)
    {
      case OP_NOP: break;

      case OP_RET:
      {
        return vm_reg_read(state, REG(0));
      } break;

      case OP_MOV:
      {
        vm_reg_write(state, instruction.a[0].r[0], vm_reg_read(state, instruction.a[0].r[1]));
      } break;

      case OP_LOADC:
      {
        vm_reg_write(state, instruction.a[0].r[0], instruction.a[1].object);
      } break;

      case OP_TBLGETR:
      {
        byte const reg_tbl = instruction.a[0].r[0];
        byte const reg_name = instruction.a[0].r[1];

        struct ModlObject const * const obj_tbl = vm_reg_read(state, reg_tbl);
        struct ModlObject const * const obj_name = vm_reg_read(state, reg_name);

        vm_reg_write(
          state, reg_tbl,
          modl_table_get_v(obj_tbl, obj_name)
        );
      } break;

      case OP_CALLR:
      {
        struct ModlObject const * const obj = vm_reg_read(state, instruction.a[0].r[0]);
        vm_call_function(state, obj);
      } break;

      case OP_LOADFUN:
      {
        struct Environment * env = state->call_stack[state->csp].environment;
        vm_reg_write(state, instruction.a[0].r[0], ifun_to_modl(env, state->ip + instruction.a[1].i64));
      } break;

      case OP_JMP: state->ip += instruction.a[0].i64 - instruction.byte_length; break;

      case OP_ROL:
      case OP_ROR:
      case OP_IDIV:
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
      case OP_CMPLT:
      case OP_CMPNLT:
      case OP_CMPGT:
      case OP_CMPNGT:
      case OP_CMPLE:
      case OP_CMPNLE:
      case OP_CMPGE:
      case OP_CMPNGE:
      {
        byte const reg_dst = instruction.a[0].r[0];
        byte const reg_src = instruction.a[0].r[1];

        struct ModlObject * obj_l = modl_object_take(vm_reg_read(state, reg_dst));
        struct ModlObject * obj_r = modl_object_take(vm_reg_read(state, reg_src));

        if (ModlTypeFloating == obj_l->type)
        {
          modl_object_disown(obj_r);
          obj_r = modl_object_take(modl_maybe_cast(obj_r, ModlTypeFloating));
        }
        if (ModlTypeFloating == obj_r->type)
        {
          modl_object_disown(obj_l);
          obj_l = modl_object_take(modl_maybe_cast(obj_l, ModlTypeFloating));
        }

        if (obj_l->type != obj_r->type)
        {
          printf(
            "\x1b[31;1m  Values are required to have the same type: %s <> %s\x1b[0m\n",
            modl_types_names_table[obj_l->type],
            modl_types_names_table[obj_r->type]
          );
          exit(EXIT_FAILURE);
        }

        if (ModlTypeInteger != obj_l->type && ModlTypeFloating != obj_l->type)
        {
          printf(
            "\x1b[31;1m  This operation requires operands of integer or floating types: %s <> %s/%s\x1b[0m\n",
            modl_types_names_table[obj_l->type],
            modl_types_names_table[ModlTypeInteger],
            modl_types_names_table[ModlTypeFloating]
          );
          exit(EXIT_FAILURE);
        }

        switch (ModlTypeInteger == obj_l->type)
        {
          case TRUE: switch (instruction.opcode)
          {
            case OP_ROL: vm_reg_write(state, reg_dst, int_to_modl(modl_to_int(obj_l) << modl_to_int(obj_r))); break;
            case OP_ROR: vm_reg_write(state, reg_dst, int_to_modl(modl_to_int(obj_l) >> modl_to_int(obj_r))); break;
            case OP_IDIV: vm_reg_write(state, reg_dst, int_to_modl(modl_to_int(obj_l) / modl_to_int(obj_r))); break;
            case OP_ADD: vm_reg_write(state, reg_dst, int_to_modl(modl_to_int(obj_l) + modl_to_int(obj_r))); break;
            case OP_SUB: vm_reg_write(state, reg_dst, int_to_modl(modl_to_int(obj_l) - modl_to_int(obj_r))); break;
            case OP_MUL: vm_reg_write(state, reg_dst, int_to_modl(modl_to_int(obj_l) * modl_to_int(obj_r))); break;
            case OP_DIV: vm_reg_write(state, reg_dst, double_to_modl((double)modl_to_int(obj_l) / (double)modl_to_int(obj_r))); break;
            case OP_MOD: vm_reg_write(state, reg_dst, int_to_modl(modl_to_int(obj_l) % modl_to_int(obj_r))); break;
            case OP_AND: vm_reg_write(state, reg_dst, int_to_modl(modl_to_int(obj_l) & modl_to_int(obj_r))); break;
            case OP_OR: vm_reg_write(state, reg_dst, int_to_modl(modl_to_int(obj_l) | modl_to_int(obj_r))); break;
            case OP_XOR: vm_reg_write(state, reg_dst, int_to_modl(modl_to_int(obj_l) ^ modl_to_int(obj_r))); break;
            case OP_NAND: vm_reg_write(state, reg_dst, int_to_modl(~(modl_to_int(obj_l) & modl_to_int(obj_r)))); break;
            case OP_NOR: vm_reg_write(state, reg_dst, int_to_modl(~(modl_to_int(obj_l) | modl_to_int(obj_r)))); break;
            case OP_NXOR: vm_reg_write(state, reg_dst, int_to_modl(~(modl_to_int(obj_l) ^ modl_to_int(obj_r)))); break;
            case OP_CMPLT: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_int(obj_l) < modl_to_int(obj_r))); break;
            case OP_CMPNLT: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_int(obj_l) < modl_to_int(obj_r)))); break;
            case OP_CMPGT: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_int(obj_l) > modl_to_int(obj_r))); break;
            case OP_CMPNGT: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_int(obj_l) > modl_to_int(obj_r)))); break;
            case OP_CMPLE: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_int(obj_l) <= modl_to_int(obj_r))); break;
            case OP_CMPNLE: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_int(obj_l) <= modl_to_int(obj_r)))); break;
            case OP_CMPGE: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_int(obj_l) >= modl_to_int(obj_r))); break;
            case OP_CMPNGE: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_int(obj_l) >= modl_to_int(obj_r)))); break;
            default: break;
          } break;

          case FALSE: switch (instruction.opcode)
          {
            case OP_ADD: vm_reg_write(state, reg_dst, double_to_modl(modl_to_double(obj_l) + modl_to_double(obj_r))); break;
            case OP_SUB: vm_reg_write(state, reg_dst, double_to_modl(modl_to_double(obj_l) - modl_to_double(obj_r))); break;
            case OP_MUL: vm_reg_write(state, reg_dst, double_to_modl(modl_to_double(obj_l) * modl_to_double(obj_r))); break;
            case OP_DIV: vm_reg_write(state, reg_dst, double_to_modl(modl_to_double(obj_l) / modl_to_double(obj_r))); break;
            case OP_MOD: vm_reg_write(state, reg_dst, double_to_modl(fmod(modl_to_double(obj_l), modl_to_double(obj_r)))); break;
            // case OP_AND: vm_reg_write(state, reg_dst, double_to_modl(modl_to_double(obj_l) & modl_to_double(obj_r))); break;
            // case OP_OR: vm_reg_write(state, reg_dst, double_to_modl(modl_to_double(obj_l) | modl_to_double(obj_r))); break;
            // case OP_XOR: vm_reg_write(state, reg_dst, double_to_modl(modl_to_double(obj_l) ^ modl_to_double(obj_r))); break;
            // case OP_NAND: vm_reg_write(state, reg_dst, double_to_modl(~(modl_to_double(obj_l) & modl_to_double(obj_r)))); break;
            // case OP_NOR: vm_reg_write(state, reg_dst, double_to_modl(~(modl_to_double(obj_l) | modl_to_double(obj_r)))); break;
            // case OP_NXOR: vm_reg_write(state, reg_dst, double_to_modl(~(modl_to_double(obj_l) ^ modl_to_double(obj_r)))); break;
            case OP_CMPLT: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_double(obj_l) < modl_to_double(obj_r))); break;
            case OP_CMPNLT: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_double(obj_l) < modl_to_double(obj_r)))); break;
            case OP_CMPGT: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_double(obj_l) > modl_to_double(obj_r))); break;
            case OP_CMPNGT: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_double(obj_l) > modl_to_double(obj_r)))); break;
            case OP_CMPLE: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_double(obj_l) <= modl_to_double(obj_r))); break;
            case OP_CMPNLE: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_double(obj_l) <= modl_to_double(obj_r)))); break;
            case OP_CMPGE: vm_reg_write(state, reg_dst, bool_to_modl(modl_to_double(obj_l) >= modl_to_double(obj_r))); break;
            case OP_CMPNGE: vm_reg_write(state, reg_dst, bool_to_modl(!(modl_to_double(obj_l) >= modl_to_double(obj_r)))); break;
            default:
            {
              printf("%s\n", "\x1b[31;1m  this operation is not supported on floats\x1b[0m");
              exit(EXIT_FAILURE);
            } break;
          } break;
        }

        modl_object_release(obj_l);
        modl_object_release(obj_r);
      } break;

      case OP_CMPEQ:
      case OP_CMPNEQ:
      {
        vm_reg_write(state, instruction.a[0].r[0], bool_to_modl(modl_object_equals(
          vm_reg_read(state, instruction.a[0].r[0]),
          vm_reg_read(state, instruction.a[0].r[1])
        ) == (instruction.opcode == OP_CMPEQ)));
      } break;


      case OP_JCF:
      case OP_JCT:
      {
        if (modl_to_bool(vm_reg_read(state, instruction.a[0].r[0])) == (instruction.opcode == OP_JCT))
          state->ip += instruction.a[1].i64 - instruction.byte_length;
      } break;

      case OP_POP:
      {
        if (0 == state->sp)
        {
          printf("\x1b[31;1m  Cannot pop from empty stack\x1b[0m\n");
          exit(EXIT_FAILURE);
        }

        state->sp -= 1;
        vm_reg_write(state, instruction.a[0].r[0], state->stack[state->sp]);
        modl_object_release(state->stack[state->sp]);
      } break;

      case OP_PUSH:
      {
        if (state->sp + 1 > state->max_count_stack)
        {
          printf(
            "\x1b[31;1m%s: stack_max_size=%lu\x1b[0m\n",
            "  maximum stack size exceeded",
            state->max_count_stack
          );
          exit(EXIT_FAILURE);
        }

        state->stack[state->sp++] = modl_object_take(vm_reg_read(state, instruction.a[0].r[0]));
      } break;

      case OP_TBLPUSH:
      {
        modl_table_push_v(
          vm_reg_read(state, instruction.a[0].r[0]),
          vm_reg_read(state, instruction.a[0].r[1])
        );
      } break;

      case OP_TBLSETR:
      {
        modl_table_insert_kv(
          vm_reg_read(state, instruction.a[0].r[0]),
          vm_reg_read(state, instruction.a[0].r[1]),
          vm_reg_read(state, instruction.a[1].r[0])
        );
      } break;

      case OP_ENVGETC:
      {
        struct Environment * env = state->call_stack[state->csp].environment;

        while (NULL != env)
        {
          if (modl_table_has_k(env->vartable, instruction.a[1].object))
          {
            vm_reg_write(
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
        vm_reg_write(state, instruction.a[0].r[0], modl_nil());
        CASE_OP_ENVGETC_END: break;
      } break;

      case OP_ENVSETC:
      {
        modl_table_insert_kv(
          state->call_stack[state->csp].environment->vartable,
          instruction.a[1].object,
          vm_reg_read(state, instruction.a[0].r[0])
        );
      } break;

      case OP_ENVUPKC:
      {
        byte const reg_val = instruction.a[0].r[0];
        byte const reg_arg = instruction.a[0].r[1];

        struct ModlObject * obj_vals = vm_reg_read(state, reg_val);
        struct ModlObject * obj_args = vm_reg_read(state, reg_arg);

        struct ModlObject * index = int_to_modl(0);
        while (modl_table_has_k(obj_vals, index) && modl_table_has_k(obj_args, index))
        {
          modl_table_insert_kv(
            vm_get_current_environment(state)->vartable,
            modl_table_get_v(obj_args, index),
            modl_table_get_v(obj_vals, index)
          );
          index->value.integer += 1;
        }

        modl_object_release_tmp(index);
      } break;

      case OP_ENVPUSH:
      {

      } break;

      default:
      {
        printf("\x1b[31;1m  Instruction implementation not found\x1b[0m\n");
        exit(EXIT_FAILURE);
      } break;
    }

    state->ip += instruction.byte_length;
    instruction_release(instruction);
  }
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
  modl_object_release(vm->stack[vm->sp]);
  return modl_nil();
}

static struct ModlObject * modl_std_concat_strings(struct VMState * vm)
{
  struct ModlObject * a = vm->stack[--vm->sp];
  struct ModlObject * b = vm->stack[--vm->sp];

  char res[strlen(a->value.string) + strlen(b->value.string) + 1];
  strcpy(res, a->value.string);
  strcat(res, b->value.string);

  modl_object_release(a);
  modl_object_release(b);

  return str_to_modl(res);
}

static struct ModlObject * modl_std_to_string(struct VMState * vm)
{
  struct ModlObject * a = vm->stack[--vm->sp];

  switch (a->type)
  {
    case ModlTypeNil: modl_object_release(a); return str_to_modl("nil");
    case ModlTypeBoolean:
    {
      bool val = modl_to_bool(a);
      modl_object_release(a);
      return str_to_modl(val ? "true" : "false");
    }
    case ModlTypeInteger:
    {
      char res[21];
      sprintf(res, "%ld", modl_to_int(a));
      modl_object_release(a);
      return str_to_modl(res);
    }
    case ModlTypeFloating:
    {
      char res[64];
      sprintf(res, "%.17g", modl_to_double(a));
      modl_object_release(a);
      return str_to_modl(res);
    }
    case ModlTypeString:
    {
      return modl_object_disown(a);
    }

    default: modl_object_release(a); return modl_nil();
  }
}


static struct ModlObject * modl_std_table_keys(struct VMState * vm)
{
  struct ModlObject * table = vm->stack[--vm->sp];
  struct ModlObject * ret = modl_table();

  {
    struct ModlTableNode * node = table->value.table.integer_nodes;
    while (NULL != node->key)
    {
      modl_table_push_v(ret, (struct ModlObject *) node->key);
      node = node->next;
    }
  }

  {
    struct ModlTableNode * node = table->value.table.string_nodes;
    while (NULL != node->key)
    {
      modl_table_push_v(ret, (struct ModlObject *) node->key);
      node = node->next;
    }
  }

  modl_object_release(table);
  return ret;
}

static struct ModlObject * modl_std_table_size(struct VMState * vm)
{
  struct ModlObject * table = vm->stack[--vm->sp];
  int64_t count = 0;

  {
    struct ModlTableNode const * node = table->value.table.integer_nodes;
    while (NULL != node->key)
    {
      count += 1;
      node = node->next;
    }
  }

  {
    struct ModlTableNode const * node = table->value.table.string_nodes;
    while (NULL != node->key)
    {
      count += 1;
      node = node->next;
    }
  }

  modl_object_release(table);
  return int_to_modl(count);
}

static struct ModlObject * modl_std_table_empty(struct VMState * vm)
{
  struct ModlObject * table = vm->stack[--vm->sp];

  {
    struct ModlTableNode * node = table->value.table.integer_nodes;
    if (NULL != node->key)
    {
      modl_object_release(table);
      return bool_to_modl(FALSE);
    }
  }

  {
    struct ModlTableNode * node = table->value.table.string_nodes;
    if (NULL != node->key)
    {
      modl_object_release(table);
      return bool_to_modl(FALSE);
    }
  }

  modl_object_release(table);
  return bool_to_modl(TRUE);
}

static struct ModlObject * modl_std_table_pop(struct VMState * vm)
{
  struct ModlObject * table = vm->stack[--vm->sp];

  if (NULL == table->value.table.last_consecutive_integer_node)
  {
    modl_object_release(table);
    return modl_nil();
  }

  struct ModlTableNode * ret_node = table->value.table.last_consecutive_integer_node;

  struct ModlTableNode * pred = NULL;
  {
    struct ModlTableNode * node = table->value.table.integer_nodes;
    while (NULL != node->key && node != ret_node)
    {
      if (node->next == ret_node)
      {
        pred = node;
        break;
      }
      node = node->next;
    }
  }

  if (NULL == pred)
  {
    table->value.table.integer_nodes = ret_node->next;
  }
  else
  {
    pred->next = ret_node->next;
  }

  if (ret_node->key->value.integer == 0)
  {
    table->value.table.last_consecutive_integer_node = NULL;
  }
  else
  {
    table->value.table.last_consecutive_integer_node = pred;
  }

  modl_object_release((struct ModlObject *) ret_node->key);
  modl_object_release(table);
  return modl_object_disown(ret_node->value);
}

static struct ModlObject * modl_std_table_zip(struct VMState * vm)
{
  struct ModlObject * kt = vm->stack[--vm->sp];
  struct ModlObject * vt = vm->stack[--vm->sp];

  struct ModlObject * rt = modl_table_new();

  if (NULL != kt->value.table.last_consecutive_integer_node)
  if (NULL != vt->value.table.last_consecutive_integer_node)
  {
    struct ModlTableNode * nodeK = kt->value.table.integer_nodes;
    struct ModlTableNode * nodeV = vt->value.table.integer_nodes;
    while (0 != nodeK->key->value.integer)
      nodeK = nodeK->next;
    while (0 != nodeV->key->value.integer)
      nodeV = nodeV->next;

    while (nodeK != kt->value.table.last_consecutive_integer_node->next
        && nodeV != vt->value.table.last_consecutive_integer_node->next)
    {
      modl_table_insert_kv(rt, nodeK->value, nodeV->value);
      nodeK = nodeK->next;
      nodeV = nodeV->next;
    }
  }

  modl_object_release(kt);
  modl_object_release(vt);
  return modl_object_disown(rt);
}

static struct ModlObject * modl_std_array_foreach(struct VMState * vm)
{
  struct ModlObject * array = vm->stack[--vm->sp];
  struct ModlObject * func = vm->stack[--vm->sp];

  if (NULL != array->value.table.last_consecutive_integer_node)
  {
    struct ModlTableNode * node = array->value.table.integer_nodes;
    while (0 != node->key->value.integer)
      node = node->next;

    while (node != array->value.table.last_consecutive_integer_node->next)
    {
      vm->stack[vm->sp++] = modl_object_take(node->value);
      vm_call_function(vm, func);
      node = node->next;
    }
  }

  modl_object_release(array);
  modl_object_release(func);
  return modl_nil();
}

static struct ModlObject * modl_std_array_map(struct VMState * vm)
{
  struct ModlObject * array = vm->stack[--vm->sp];
  struct ModlObject * func = vm->stack[--vm->sp];

  struct ModlObject * ret = modl_table_new();

  if (NULL != array->value.table.last_consecutive_integer_node)
  {
    struct ModlTableNode * node = array->value.table.integer_nodes;
    while (0 != node->key->value.integer)
      node = node->next;

    while (node != array->value.table.last_consecutive_integer_node->next)
    {
      vm->stack[vm->sp++] = modl_object_take(node->value);
      vm_call_function(vm, func);
      modl_table_push_v(ret, vm_reg_read(vm, REG(0)));
      node = node->next;
    }
  }

  modl_object_release(array);
  modl_object_release(func);
  return modl_object_disown(ret);
}

static struct ModlObject * modl_std_array_concat(struct VMState * vm)
{
  struct ModlObject * a = vm->stack[--vm->sp];
  struct ModlObject * b = vm->stack[--vm->sp];

  struct ModlObject * rt = modl_table_new();

  if (NULL != a->value.table.last_consecutive_integer_node)
  {
    struct ModlTableNode const * node = a->value.table.integer_nodes;
    while (0 != node->key->value.integer)
      node = node->next;

    while (node != a->value.table.last_consecutive_integer_node->next)
    {
      modl_table_push_v(rt, node->value);
      node = node->next;
    }
  }

  if (NULL != b->value.table.last_consecutive_integer_node)
  {
    struct ModlTableNode const * node = b->value.table.integer_nodes;
    while (0 != node->key->value.integer)
      node = node->next;

    while (node != b->value.table.last_consecutive_integer_node->next)
    {
      modl_table_push_v(rt, node->value);
      node = node->next;
    }
  }

  modl_object_release(a);
  modl_object_release(b);
  return modl_object_disown(rt);
}

static struct ModlObject * modl_std_array_contains(struct VMState * vm)
{
  struct ModlObject * array = vm->stack[--vm->sp];
  struct ModlObject * obj = vm->stack[--vm->sp];

  if (NULL == array->value.table.last_consecutive_integer_node)
  {
    modl_object_release(array);
    modl_object_release(obj);
    return bool_to_modl(FALSE);
  }

  {
    struct ModlTableNode * node = array->value.table.integer_nodes;
    while (0 != node->key->value.integer)
      node = node->next;

    while (node != array->value.table.last_consecutive_integer_node->next)
    {
      if (modl_object_equals(obj, node->value))
      {
        modl_object_release(array);
        modl_object_release(obj);
        return bool_to_modl(TRUE);
      }

      node = node->next;
    }
  }

  modl_object_release(array);
  modl_object_release(obj);
  return bool_to_modl(FALSE);
}

static struct ModlObject * modl_std_array_map_flat(struct VMState * vm)
{
  struct ModlObject * array = vm->stack[--vm->sp];
  struct ModlObject * func = vm->stack[--vm->sp];

  struct ModlObject * ret = modl_table_new();

  if (NULL == array->value.table.last_consecutive_integer_node)
  {
    modl_object_release(array);
    modl_object_release(func);
    return modl_object_disown(ret);
  }

  {
    struct ModlTableNode * node = array->value.table.integer_nodes;
    while (0 != node->key->value.integer)
      node = node->next;

    while (node != array->value.table.last_consecutive_integer_node->next)
    {
      vm->stack[vm->sp++] = modl_object_take(node->value);
      vm_call_function(vm, func);

      vm->stack[vm->sp++] = modl_object_take(vm_reg_read(vm, REG(0)));
      vm->stack[vm->sp++] = ret;
      vm_call_function(vm, vm_find_external_function(vm, modl_std_array_concat));
      ret = modl_object_take(vm_reg_read(vm, REG(0)));
      node = node->next;
    }
  }

  modl_object_release(array);
  modl_object_release(func);
  return modl_object_disown(ret);
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
      {"silent",          no_argument,       0,  'l' },
      {0,                 0,                 0,  0   }
  };

  while((opt = getopt_long(argc, argv, ":i:s:cl", long_options, &long_index)) != -1)
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

      case 'l':
      {
        VM_SETTING_SILENT = TRUE;
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


  struct Environment base_environment = { NULL, modl_table_new() };
  struct VMState vm =
  {
    .code = NULL,

    .max_count_call_stack = max_count_call_stack,
    .max_count_stack = max_count_stack,
    .max_count_externals = 16,

    .ip = 0, .csp = 0, .sp = 0, .efc = 0,
    .call_stack = (struct CallFrame *) malloc(max_count_call_stack * sizeof (struct CallFrame)),
    .registers = { NULL },
    .stack = (struct ModlObject **) (malloc(max_count_stack * sizeof (struct ModlObject **))),
    .external_functions = (struct ModlObject*(**)(struct VMState*))(malloc(max_count_externals*sizeof(struct ModlObject*(**)(struct VMState*)))),

    .ram = {
      .read_after_rewrite = { [0 ... VM_SETTING_REGITERS_COUNT-1] = TRUE },
      .last_write_points = { 0, },
    },
  };
  vm.call_stack[0] = (struct CallFrame) { 0, &base_environment };
  for (size_t i = 0; i < VM_SETTING_REGITERS_COUNT; ++i)
    vm.registers[i] = modl_nil_new();


  uint64_t std_print_id = vm_add_external_function(&vm, modl_std_print);
  uint64_t std_concat_id = vm_add_external_function(&vm, modl_std_concat_strings);
  uint64_t std_to_string_id = vm_add_external_function(&vm, modl_std_to_string);
  modl_table_insert_kv(base_environment.vartable, str_to_modl("print"), efun_to_modl(std_print_id));
  modl_table_insert_kv(base_environment.vartable, str_to_modl("concat"), efun_to_modl(std_concat_id));
  modl_table_insert_kv(base_environment.vartable, str_to_modl("toString"), efun_to_modl(std_to_string_id));


  {
    struct ModlObject * table_efuns = modl_table();

    uint64_t std_table_keys_id = vm_add_external_function(&vm, modl_std_table_keys);
    modl_table_insert_kv(table_efuns, str_to_modl("keys"), efun_to_modl(std_table_keys_id));

    uint64_t std_table_empty_id = vm_add_external_function(&vm, modl_std_table_empty);
    modl_table_insert_kv(table_efuns, str_to_modl("empty"), efun_to_modl(std_table_empty_id));

    uint64_t std_table_pop_id = vm_add_external_function(&vm, modl_std_table_pop);
    modl_table_insert_kv(table_efuns, str_to_modl("pop"), efun_to_modl(std_table_pop_id));

    uint64_t std_table_size_id = vm_add_external_function(&vm, modl_std_table_size);
    modl_table_insert_kv(table_efuns, str_to_modl("size"), efun_to_modl(std_table_size_id));

    uint64_t std_table_zip_id = vm_add_external_function(&vm, modl_std_table_zip);
    modl_table_insert_kv(table_efuns, str_to_modl("zip"), efun_to_modl(std_table_zip_id));

    modl_table_insert_kv(base_environment.vartable, str_to_modl("Table"), table_efuns);
  }

  {
    struct ModlObject * table_efuns = modl_table();

    uint64_t std_array_foreach_id = vm_add_external_function(&vm, modl_std_array_foreach);
    modl_table_insert_kv(table_efuns, str_to_modl("forEach"), efun_to_modl(std_array_foreach_id));

    uint64_t std_array_map_id = vm_add_external_function(&vm, modl_std_array_map);
    modl_table_insert_kv(table_efuns, str_to_modl("map"), efun_to_modl(std_array_map_id));

    uint64_t std_array_map_flat_id = vm_add_external_function(&vm, modl_std_array_map_flat);
    modl_table_insert_kv(table_efuns, str_to_modl("mapFlat"), efun_to_modl(std_array_map_flat_id));

    uint64_t std_array_concat_id = vm_add_external_function(&vm, modl_std_array_concat);
    modl_table_insert_kv(table_efuns, str_to_modl("concat"), efun_to_modl(std_array_concat_id));

    uint64_t std_array_contains_id = vm_add_external_function(&vm, modl_std_array_contains);
    modl_table_insert_kv(table_efuns, str_to_modl("contains"), efun_to_modl(std_array_contains_id));

    modl_table_insert_kv(base_environment.vartable, str_to_modl("Array"), table_efuns);
  }


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

  if (not VM_SETTING_SILENT)
  {
    printf("\x1b[34;1m%s\x1b[0m\n",   "-----=====      RUN       =====-----");
  }

  vm.code = input;
  struct ModlObject * result = run(&vm);

  if (not VM_SETTING_SILENT)
    printf("\n\x1b[34;1m%s\x1b[0m\n", "-----=====     RESULT     =====-----");

  display_modl_object(result);
  printf("%c", '\n');

  for (byte i = 0; i < VM_SETTING_REGITERS_COUNT; ++i)
    modl_object_release(vm.registers[i]);

  modl_object_release(base_environment.vartable);

  for (size_t i = 0; i < vm.sp; ++i)
    modl_object_release(vm.stack[i]);

  free(vm.call_stack);
  free(vm.stack);
  free(vm.external_functions);

  clock_t end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("\ntime: %fs\n", time_spent);

  return EXIT_SUCCESS;
}
