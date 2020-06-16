#include <stdio.h>
#include <string.h>

#include "sebo.h"


static double double_reverse_endianness(byte const * data)
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
static double ntohd(double src)
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

      struct ModlObject obj = str_to_modl(value);
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

struct Sebo modl_encode_sebo(struct ModlObject object)
{
  switch (object.type)
  {
    case ModlTypeNil: return (struct Sebo) { array_of__0, 1, object };
    case ModlTypeBoolean: return (struct Sebo) { object.value.boolean ? array_of__2 : array_of__1, 1, object };
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
      modl_object_display(&object);
      printf("%c\x1b[0m", '\n');

      exit(EXIT_FAILURE);
    }
  }
}
