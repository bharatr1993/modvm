#include <stdio.h>
#include <string.h>

#include "object.h"


struct ModlObject modl_object_make_ref()
{
  struct ModlObject object = { .value = { .ref = malloc(sizeof (struct ModlObjectReference)) } };
  object.value.ref->count = 0;
  return object;
}

struct ModlObject modl_nil()
{
  return  (struct ModlObject) { .type = ModlTypeNil };
}

struct ModlObject modl_table()
{
  struct ModlObject object = { .type = ModlTypeTable, .value = { .ref = malloc(sizeof (struct ModlObjectReference)) } };
  object.value.ref->count = 0;
  modl_map_init(&object.value.ref->value.table, 8);
  return object;
}

struct ModlObject modl_table_new()
{
  struct ModlObject object = modl_table();
  object.value.ref->count = 1;
  return object;
}

bool modl_object_type_is(struct ModlObject self, enum ModlType type)
{
  return self.type == type;
}

bool modl_object_is_value_type(struct ModlObject self)
{
  return self.type <= ModlTypeFloating;
}


struct ModlObject bool_to_modl(bool data)
{
  return (struct ModlObject) { .type = ModlTypeBoolean, .value = { .boolean = data }};
}

struct ModlObject int_to_modl(int64_t data)
{
  return (struct ModlObject) { .type = ModlTypeInteger, .value = { .integer = data }};
}

struct ModlObject double_to_modl(double data)
{
  return (struct ModlObject) { .type = ModlTypeFloating, .value = { .floating = data }};
}

struct ModlObject transfer_str_to_modl(char * data)
{
  struct ModlObject object = modl_object_make_ref();
  object.type = ModlTypeString;
  object.value.ref->value.string = data;
  return object;
}

struct ModlObject str_to_modl(char const * data)
{
  struct ModlObject object = modl_object_make_ref();
  object.type = ModlTypeString;
  object.value.ref->value.string = strcpy(malloc((strlen(data) + 1) * sizeof(char)), data);
  return object;
}

struct ModlObject ifun_to_modl(struct Environment * environment, uint64_t position)
{
  struct ModlObject object = modl_object_make_ref();
  object.type = ModlTypeFunction;
  object.value.ref->value.fun = (struct ModlTypeFunctionInfo) {
    .is_external = FALSE,
    .position = position,
    .context = environment,
  };
  return object;
}

struct ModlObject efun_to_modl(uint64_t pointer)
{
  struct ModlObject object = modl_object_make_ref();
  object.type = ModlTypeFunction;
  object.value.ref->value.fun = (struct ModlTypeFunctionInfo) {
    .is_external = TRUE,
    .position = pointer,
    .context = NULL,
  };
  return object;
}


bool modl_to_bool(struct ModlObject object)
{
  // if (NULL == object) return FALSE;
  // if (ModlTypeBoolean != object->type) return TRUE;
  return (object.value.boolean ? TRUE : FALSE);
}

int64_t modl_to_int(struct ModlObject object)
{
  // if (NULL == object) return 0;
  // if (ModlTypeInteger != object->type) return 0;
  return object.value.integer;
}

double modl_to_double(struct ModlObject object)
{
  // if (NULL == object) return 0.0;
  // if (ModlTypeFloating != object->type) return 0.0;
  return object.value.floating;
}

char const * modl_to_str(struct ModlObject object)
{
  if (ModlTypeString != object.type) return NULL;
  return object.value.ref->value.string;
}


bool modl_object_is_tmp(struct ModlObject const * self)
{
  if (NULL == self) return TRUE;
  if (modl_object_is_value_type(*self)) return TRUE;
  return self->value.ref->count == 0;
}

bool modl_object_is_single(struct ModlObject const * self)
{
  if (NULL == self) return FALSE;
  if (modl_object_is_value_type(*self)) return TRUE;
  return self->value.ref->count == 1;
}


struct ModlObject modl_object_take(struct ModlObject self)
{
  if (modl_object_is_value_type(self)) return self;
  self.value.ref->count += 1;
  return self;
}

struct ModlObject modl_object_disown(struct ModlObject self)
{
  if (modl_object_is_value_type(self)) return self;

  self.value.ref->count -= 1;
  if (self.value.ref->count < 0)
  {
    printf("\x1b[31;1m  %s  --> %d\x1b[0m\n", "Liek watafak? instances count is negative?!?!", self.value.ref->count);
    exit(EXIT_FAILURE);
  }

  return self;
}

bool modl_object_release(struct ModlObject self)
{
  if (modl_object_is_value_type(self)) return TRUE;

  modl_object_disown(self);

  if (self.value.ref->count == 0)
  {
    // printf("  %s: ", "destroying object");
    // modl_object_display(self);
    // printf("%c", '\n');

    switch (self.type)
    {
      case ModlTypeString: free(self.value.ref->value.string); break;

      case ModlTypeTable:
      {
        modl_map_dispose(&self.value.ref->value.table);
      //   {
      //     struct ModlTableNode * node = self->value.table.integer_nodes;
      //     while (NULL != node->key)
      //     {
      //       /* discard const to free object's memory */
      //       modl_object_release((struct ModlObject *) node->key);
      //       modl_object_release(node->value);

      //       struct ModlTableNode * tmp = node;
      //       node = node->next;
      //       free(tmp);
      //     }
      //     free(node);
      //   }
      //   {
      //     struct ModlTableNode * node = self->value.table.string_nodes;
      //     while (NULL != node->key)
      //     {
      //       /* discard const to free object's memory */
      //       modl_object_release((struct ModlObject *) node->key);
      //       modl_object_release(node->value);

      //       struct ModlTableNode * tmp = node;
      //       node = node->next;
      //       free(tmp);
      //     }
      //     free(node);
      //   }
      } break;

      default: break;
    }

    free(self.value.ref);
    return TRUE;
  }

  return FALSE;
}

bool modl_object_release_tmp(struct ModlObject self)
{
  if (modl_object_is_value_type(self)) return TRUE;

  self.value.ref->count += 1;
  return modl_object_release(self);
}

int32_t modl_object_get_reference_count(struct ModlObject self)
{
  if (modl_object_is_value_type(self)) return 1;
  return self.value.ref->count;
}


bool modl_object_equals(struct ModlObject self, struct ModlObject other)
{
  // if (self == other) return TRUE;
  if (self.type != other.type) return FALSE;

  switch (self.type)
  {
    case ModlTypeNil: return TRUE;
    case ModlTypeBoolean: return self.value.boolean == other.value.boolean;
    case ModlTypeInteger: return self.value.integer == other.value.integer;
    case ModlTypeFloating: return self.value.floating == other.value.floating;
    case ModlTypeString: return 0 == strcmp(self.value.ref->value.string, other.value.ref->value.string);
    case ModlTypeTable: return FALSE;
    case ModlTypeFunction:
      return self.value.ref->value.fun.is_external == other.value.ref->value.fun.is_external
          && self.value.ref->value.fun.position    == other.value.ref->value.fun.position
          && self.value.ref->value.fun.context     == other.value.ref->value.fun.context;

    default: return FALSE;
  }
}

int modl_object_cmp(struct ModlObject self, struct ModlObject other)
{
  // if (NULL == self || NULL == other) return -1;
  // if (self == other) return 0;
  if (self.type != other.type) return -1;

  switch (self.type)
  {
    case ModlTypeNil: return 0;
    case ModlTypeBoolean: return self.value.boolean - other.value.boolean;
    case ModlTypeInteger:
      return self.value.integer > other.value.integer
          ? 1
        : self.value.integer == other.value.integer
          ? 0 : -1;
    case ModlTypeFloating:
    return self.value.floating > other.value.floating
        ? 1
      : self.value.floating == other.value.floating
        ? 0 : -1;
    case ModlTypeString: return strcmp(self.value.ref->value.string, other.value.ref->value.string);
    case ModlTypeTable: return -1;
    case ModlTypeFunction:
      return (self.value.ref->value.fun.is_external == other.value.ref->value.fun.is_external
           && self.value.ref->value.fun.position    == other.value.ref->value.fun.position
           && self.value.ref->value.fun.context     == other.value.ref->value.fun.context) ? 0 : -1;

    default: return -1;
  }
}


struct ModlObject modl_object_copy_value(struct ModlObject self)
{
  if (modl_object_is_value_type(self)) return self;

  // printf("  copying object by val: ");
  // modl_object_display(self);
  // printf("%c", '\n');

  if (modl_object_is_tmp(&self))
  {
    // printf("%s\n", "    object is temporary; transfering ownership");
    return self;
  }

  // TODO: Copy by value
  return self;

  // switch (self->type)
  // {
  //   case ModlTypeNil: return modl_nil();
  //   case ModlTypeBoolean: return bool_to_modl(self->value.boolean);
  //   case ModlTypeInteger: return int_to_modl(self->value.integer);
  //   case ModlTypeFloating: return double_to_modl(self->value.floating);
  //   case ModlTypeString: return str_to_modl(strcpy(malloc((strlen(self->value.string) + 1) * sizeof (char)), self->value.string));
  //   case ModlTypeTable:
  //   {
  //     // TODO: implement
  //     printf("\x1b[31;1m  %s\x1b[0m\n", "ModlTypeTable copy by value is not implemented");
  //     exit(EXIT_FAILURE);
  //     return NULL;

  //     // struct ModlObject * obj = modl_table();
  //     // obj->value.table = self->value.table;
  //     // return obj;
  //   }
  //   case ModlTypeFunction:
  //   {
  //     struct ModlObject * obj = ifun_to_modl(self->value.fun.context, self->value.fun.position);
  //     obj->value.fun.is_external = self->value.fun.is_external;
  //     return obj;
  //   }

  //   default: return NULL;
  // }
}


struct ModlObject modl_maybe_cast(struct ModlObject object, enum ModlType target_type)
{
  // printf("\x1b[32m  casting object from %d to %d\x1b[0m\n", object->type, target_type);
  if (object.type == target_type) return object;

  switch (object.type)
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
      modl_types_names_table[object.type],
      modl_types_names_table[target_type]
    );
    exit(EXIT_FAILURE);
  }

  return modl_nil();
}


bool modl_table_has_k(struct ModlObject * self, struct ModlObject key)
{
  if (NULL == self)
  {
    printf("\x1b[31;1m  %s\x1b[0m\n", "((ModlObject*) self) == NULL!");
    exit(EXIT_FAILURE);
  }

  if (ModlTypeTable != self->type) return FALSE;

  return NULL != modl_map_get(&self->value.ref->value.table, key);

  // struct ModlTableNode * node =
  //   key->type == ModlTypeInteger
  //     ? self->value.table.integer_nodes
  //     : self->value.table.string_nodes;

  // while (NULL != node->key)
  // {
  //   if (modl_object_equals(node->key, key))
  //     return TRUE;
  //   node = node->next;
  // }

  // return FALSE;
}

void modl_table_insert_kv(struct ModlObject * const self, struct ModlObject key, struct ModlObject value)
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

  if (ModlTypeString != key.type && ModlTypeInteger != key.type)
  {
    printf("\x1b[31;1m  %s\x1b[0m\n", "((ModlObject *) key)->type not in (ModlTypeString, ModlTypeInteger)!");
    exit(EXIT_FAILURE);
  }

  modl_map_set(&self->value.ref->value.table, key, value);
  modl_object_release_tmp(key);

  // if (ModlTypeInteger == key->type && NULL != self->value.table.last_consecutive_integer_node
  //     && self->value.table.last_consecutive_integer_node->key->value.integer + 1 == key->value.integer)
  // {
  //   struct ModlTableNode * cn = self->value.table.last_consecutive_integer_node;
  //   struct ModlTableNode * ins_next = (struct ModlTableNode *) malloc(sizeof (struct ModlTableNode));
  //   ins_next->key = modl_object_take(modl_object_copy_ref(key));
  //   ins_next->value = modl_object_take(modl_object_copy_ref(value));
  //   ins_next->next = cn->next;
  //   cn->next = ins_next;

  //   self->value.table.last_consecutive_integer_node = ins_next;
  //   while (NULL != self->value.table.last_consecutive_integer_node->next->key
  //       && (self->value.table.last_consecutive_integer_node->next->key->value.integer
  //           == self->value.table.last_consecutive_integer_node->key->value.integer + 1))
  //     self->value.table.last_consecutive_integer_node = self->value.table.last_consecutive_integer_node->next;
  //   return;
  // }

  // struct ModlTableNode * node =
  //   key->type == ModlTypeInteger
  //     ? self->value.table.integer_nodes
  //     : self->value.table.string_nodes;

  // while (NULL != node->key)
  // {
  //   if (modl_object_equals(node->key, key))
  //   {
  //     modl_object_release(node->value);
  //     node->value = modl_object_take(modl_object_copy_ref(value));
  //     return;
  //   }
  //   else if (ModlTypeInteger == key->type && key->value.integer < node->key->value.integer)
  //   {
  //     struct ModlTableNode * ins_next = (struct ModlTableNode *) malloc(sizeof (struct ModlTableNode));
  //     ins_next->key = modl_object_take(modl_object_copy_ref(key));
  //     ins_next->value = modl_object_take(modl_object_copy_ref(value));
  //     ins_next->next = node->next;
  //     node->next = ins_next;

  //     if (key->value.integer == 0)
  //       self->value.table.last_consecutive_integer_node = ins_next;

  //     if (NULL == self->value.table.last_consecutive_integer_node)
  //       return;

  //     while (NULL != self->value.table.last_consecutive_integer_node->next->key
  //         && (self->value.table.last_consecutive_integer_node->next->key->value.integer
  //             == self->value.table.last_consecutive_integer_node->key->value.integer + 1))
  //       self->value.table.last_consecutive_integer_node = self->value.table.last_consecutive_integer_node->next;
  //     return;
  //   }
  //   node = node->next;
  // }

  // if (ModlTypeInteger == key->type && key->value.integer == 0)
  //   self->value.table.last_consecutive_integer_node = node;
  // else if (ModlTypeInteger == key->type && NULL != self->value.table.last_consecutive_integer_node
  //   && self->value.table.last_consecutive_integer_node->key->value.integer + 1 == key->value.integer)
  //   self->value.table.last_consecutive_integer_node = node;


  // node->key = modl_object_take(modl_object_copy_ref(key));
  // node->value = modl_object_take(modl_object_copy_ref(value));
  // node->next = (struct ModlTableNode *) calloc(1, sizeof (struct ModlTableNode));
}

void modl_table_push_v(struct ModlObject * self, struct ModlObject value)
{
  struct ModlObject next_id = int_to_modl(0);
  while (modl_table_has_k(self, next_id)) next_id.value.integer += 1;
  modl_table_insert_kv(self, next_id, value);
  // struct ModlTableNode const * ln = self->value.table.last_consecutive_integer_node;
  // int64_t next_id = 0;
  // if (NULL != ln) next_id = ln->key->value.integer + 1;
  // modl_table_insert_kv(self, int_to_modl(next_id), value);
}


static struct ModlObject index_string = { .type = ModlTypeNil };
struct ModlObject modl_table_get_v(struct ModlObject const * self, struct ModlObject key)
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

  // struct ModlTableNode const * node =
  //   key->type == ModlTypeInteger
  //     ? self->value.table.integer_nodes
  //     : self->value.table.string_nodes;

  // while (NULL != node->key)
  // {
  //   if (modl_object_equals(node->key, key))
  //     return node->value;

  //   node = node->next;
  // }

  struct ModlObject * ret = modl_map_get(&self->value.ref->value.table, key);
  if (NULL == ret)
  {
    if (ModlTypeNil == index_string.type) index_string = modl_object_take(str_to_modl("__index"));
    struct ModlObject * itbl = modl_map_get(&self->value.ref->value.table, index_string);
    // if (ModlTypeFunction == itbl->type)
    //   return vm_call_function()
    // TODO: Do something with functions
    return itbl ? modl_table_get_v(itbl, key) : modl_nil();
  }

  return *ret;
}


void modl_object_display(struct ModlObject const * object)
{
  if (NULL == object)
  {
    printf("\x1b[31;1m%s\x1b[0m", "undefined");
    return;
  }

  if (not modl_object_is_value_type(*object))
  {
    printf("{%d}", modl_object_get_reference_count(*object));
  }

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
      printf("\x1b[32m\"%s\"\x1b[0m", object->value.ref->value.string);
    } break;

    case ModlTypeTable:
    {
      printf("%s", "[ ... ]");
      break;
      modl_map_print(&object->value.ref->value.table);
      // printf("%c", '[');

      // {
      //   int64_t last_key = -1;
      //   struct ModlTableNode const * node = object->value.table.integer_nodes;
      //   while (NULL != node->key)
      //   {
      //     if (node->key->value.integer != last_key + 1)
      //     {
      //       modl_object_display(node->key);
      //       printf("%s", ": ");
      //     }
      //     modl_object_display(node->value);

      //     if (NULL != node->next->key)
      //       printf("%s", "; ");

      //     last_key = node->key->value.integer;
      //     node = node->next;
      //   }
      // }
      // {
      //   struct ModlTableNode const * node = object->value.table.string_nodes;
      //   while (NULL != node->key)
      //   {
      //     modl_object_display(node->key);
      //     printf("%s", ": ");
      //     modl_object_display(node->value);

      //     if (NULL != node->next->key)
      //       printf("%s", "; ");

      //     node = node->next;
      //   }
      // }
      // printf("%c", ']');
    } break;

    case ModlTypeFunction:
    {
      printf("\x1b[33;1m&%04lx\x1b[0m", object->value.ref->value.fun.position);
    } break;
  }
}



#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

static uint32_t str_hash (char const * data, size_t len)
{
  uint32_t hash = len, tmp;
  int rem;

  if (0 == len || NULL == data) return 0;

  rem = len & 3;
  len >>= 2;

  /* Main loop */
  for (;len > 0; len--) {
    hash  += get16bits (data);
    tmp    = (get16bits (data+2) << 11) ^ hash;
    hash   = (hash << 16) ^ tmp;
    data  += 2*sizeof (uint16_t);
    hash  += hash >> 11;
  }

  /* Handle end cases */
  switch (rem) {
    case 3: hash += get16bits (data);
            hash ^= hash << 16;
            hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
            hash += hash >> 11;
            break;
    case 2: hash += get16bits (data);
            hash ^= hash << 11;
            hash += hash >> 17;
            break;
    case 1: hash += (signed char)*data;
            hash ^= hash << 10;
            hash += hash >> 1;
  }

  /* Force "avalanching" of final 127 bits */
  hash ^= hash << 3;
  hash += hash >> 5;
  hash ^= hash << 4;
  hash += hash >> 17;
  hash ^= hash << 25;
  hash += hash >> 6;

  return hash;
}

uint32_t modl_object_hash(struct ModlObject self)
{
  switch (self.type)
  {
    case ModlTypeNil: return 0u;
    case ModlTypeBoolean: return 1u + (uint32_t) self.value.boolean;
    case ModlTypeInteger: return self.value.integer;
    case ModlTypeFloating: return (uint32_t) self.value.floating;
    case ModlTypeString: return str_hash(self.value.ref->value.string, strlen(self.value.ref->value.string));
    case ModlTypeTable: return (uint32_t) ((size_t) &self.value.ref->value.table);
    case ModlTypeFunction: return (((uint32_t) self.value.ref->value.fun.is_external) << 31u) ^ ((uint32_t) self.value.ref->value.fun.position);
  }

  return 0u;
}
