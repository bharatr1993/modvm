#pragma once

#include "defs.h"
#include "map.h"
#include "object.h"


struct ModlObjectReference
{
  union
  {
    struct ModlMap table;
    struct ModlTypeFunctionInfo
    {
      bool is_external;
      uint64_t position;
      struct Environment * context;
    } fun;

    char * string;
  } value;

  int32_t count;
  uint32_t hash;

  bool has_hash;
};
