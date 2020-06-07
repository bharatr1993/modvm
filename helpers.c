#include "helpers.h"


void print_buf(char const *title, unsigned char const *buf, size_t buf_len)
{
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int hbc = 0;
  char const * ws_spacer = "        ";

  hbc = calculate_half_bytes_count(buf_len + 0xF);
  size_t title_length = strlen(title);

  fprintf(stdout, "%s%s", &(8 - hbc)[ws_spacer], "  \u257F");
  for (i = 0; i < (16*3 - title_length + 0) / 2; ++i)
    fprintf(stdout, "%c", ' ');
  fprintf(stdout, "%s", title);
  for (i = 0; i < (16*3 - title_length + 1) / 2; ++i)
    fprintf(stdout, "%c", ' ');
  fprintf(stdout, "%s", " \u257F\r\n");

  fprintf(stdout, "%s%s", &(8 - hbc)[ws_spacer], "  \u2502");
  for (i = 0; i < 16; ++i)
    fprintf(stdout, " %02X", i);
  fprintf(stdout, " \u2502 size: %ld\r\n", buf_len);

  if (buf_len > 0)
  {
    fprintf(stdout, "%s", "\u257E\u2500");
    for (i = 0; i < hbc; ++i)
      fprintf(stdout, "%s", "\u2500");
    fprintf(stdout, "%s", "\u253C");

    for (i = 0; i < 16; ++i)
      fprintf(stdout, "%s", "\u2500\u2500\u2500");
    fprintf(stdout, "%s", "\u2500\u253C\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u257C\r\n");
  }

  for(i = 0; i < buf_len; i += 16)
  {
    unsigned int cur_hbc = calculate_half_bytes_count(i);
    if (cur_hbc < 2) cur_hbc = 2;

    fprintf(stdout, "%s %02X \u2502 ", &(8 - hbc + cur_hbc)[ws_spacer], i);
    for (j = 0; j < 16; ++j)
      if (i + j < buf_len) fprintf(stdout, "%02X ", buf[i + j]);
      else fprintf(stdout, "%s", "   ");

    fprintf(stdout, "%s", "\u2502 ");
    for (j = 0; j < 16; ++j)
      if (i + j < buf_len) fprintf(stdout, "%c", isprint(buf[i + j]) ? buf[i + j] : '.');
      else fprintf(stdout, "%c", ' ');

    if (i + 16 < buf_len)
      fprintf(stdout, "%s", "\r\n");
  }

  if (buf_len > 0)
    fprintf(stdout, "%s", "\r\n");

  fprintf(stdout, "%s", "\u257E\u2500");
  for (i = 0; i < hbc; ++i)
    fprintf(stdout, "%s", "\u2500");
  fprintf(stdout, "%s", "\u2534");

  for (i = 0; i < 16; ++i)
    fprintf(stdout, "%s", "\u2500\u2500\u2500");
  fprintf(stdout, "%s", "\u2500\u2534\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u257C\r\n");
}

unsigned int calculate_half_bytes_count(size_t i)
{
  unsigned int hbc = 0;

  i |= (i >> 0x01);
  i |= (i >> 0x02);
  i |= (i >> 0x04);
  i |= (i >> 0x08);
  i |= (i >> 0x10);
  i |= (i >> 0x18);

  hbc += (i >> 0x00) & 1;
  hbc += (i >> 0x04) & 1;
  hbc += (i >> 0x08) & 1;
  hbc += (i >> 0x0C) & 1;
  hbc += (i >> 0x10) & 1;
  hbc += (i >> 0x14) & 1;
  hbc += (i >> 0x18) & 1;
  hbc += (i >> 0x1C) & 1;
  hbc += (i >> 0x20) & 1;

  return hbc;
}

unsigned char decode_arbitrary_char(char symbol)
{
  if ('0' <= symbol && symbol <= '9')
    return (unsigned char)(symbol - '0');

  if ('a' <= symbol && symbol <= 'z')
    return (unsigned char)(10 + (symbol - 'a'));

  if ('A' <= symbol && symbol <= 'Z')
    return (unsigned char)(10 + (symbol - 'A'));

  return 0xFF;
}
