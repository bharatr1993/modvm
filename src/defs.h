#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>


typedef uint8_t byte;
typedef byte bool;

#define FALSE ((bool) 0)
#define TRUE ((bool) 1)
#define not !

#define REG(N) ((byte) N)
#define REGS(A, B) ((byte) ((A << 4) | B))
