#pragma once

#include "defs.h"
#include "object.h"


struct Sebo
{
  byte * data;
  size_t byte_length;
  struct ModlObject object;
};


struct Sebo modl_decode_sebo(byte * data);
struct Sebo modl_encode_sebo(struct ModlObject object);
