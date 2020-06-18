#pragma once

#include "defs.h"


struct Environment;


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


struct ModlObjectReference;
struct ModlObject
{
  enum ModlType type;
  union
  {
    bool boolean;
    int64_t integer;
    double floating;
    struct ModlObjectReference * ref;
  } value;
};

#include "object_reference.h"



/*  BASE OBJECT METHODS  */
struct ModlObject modl_object_make_ref();

inline struct ModlObject modl_nil()
{ return  (struct ModlObject) { .type = ModlTypeNil }; }
struct ModlObject modl_table();
struct ModlObject modl_table_new();

inline bool modl_object_type_is(struct ModlObject self, enum ModlType type)
{ return self.type == type; }
inline bool modl_object_is_value_type(struct ModlObject self)
{ return self.type <= ModlTypeFloating; }

inline struct ModlObject bool_to_modl(bool data)
{ return (struct ModlObject) { .type = ModlTypeBoolean, .value = { .boolean = data }}; }

inline struct ModlObject int_to_modl(int64_t data)
{ return (struct ModlObject) { .type = ModlTypeInteger, .value = { .integer = data }}; }

inline struct ModlObject double_to_modl(double data)
{ return (struct ModlObject) { .type = ModlTypeFloating, .value = { .floating = data }}; }

struct ModlObject transfer_str_to_modl(char * data);
struct ModlObject str_to_modl(char const * data);
struct ModlObject ifun_to_modl(struct Environment * environment, uint64_t position);
struct ModlObject efun_to_modl(uint64_t pointer);


bool modl_object_is_tmp(struct ModlObject const * self);
bool modl_object_is_single(struct ModlObject const * self);

struct ModlObject modl_object_take(struct ModlObject self);
struct ModlObject modl_object_disown(struct ModlObject self);
bool modl_object_release(struct ModlObject self);
bool modl_object_release_tmp(struct ModlObject self);
int32_t modl_object_get_reference_count(struct ModlObject self);

struct ModlObject modl_object_copy_value(struct ModlObject self);

bool modl_object_equals(struct ModlObject self, struct ModlObject other);
int modl_object_cmp(struct ModlObject self, struct ModlObject other);
uint32_t modl_object_hash(struct ModlObject self);

/*  CONVERTERS  */
inline bool modl_to_bool(struct ModlObject object)
{ return (object.value.boolean ? TRUE : FALSE); }

inline int64_t modl_to_int(struct ModlObject object)
{ return object.value.integer; }

inline double modl_to_double(struct ModlObject object)
{ return object.value.floating; }

char const * modl_to_str(struct ModlObject object);

struct ModlObject modl_maybe_cast(struct ModlObject object, enum ModlType target_type);


/*  TABLE METHODS  */
bool modl_table_has_k(struct ModlObject * self, struct ModlObject key);
void modl_table_insert_kv(struct ModlObject * self, struct ModlObject key, struct ModlObject value);
void modl_table_push_v(struct ModlObject * self, struct ModlObject value);
struct ModlObject modl_table_get_v(struct ModlObject const * self, struct ModlObject key);


void modl_object_display(struct ModlObject const * object);

