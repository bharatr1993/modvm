#pragma once

#include <string.h>
#include <inttypes.h>


struct ModlMapBucket;
struct ModlMap {
    uint32_t capacity;
    uint32_t size;
    struct ModlMapBucket * vec;
};

#include "object.h"

struct ModlMapBucket {
    struct ModlObject obj;
    struct ModlObject key;

    struct ModlMapBucket * next;
};


void modl_map_print(struct ModlMap * self);

void modl_map_dispose(struct ModlMap * self);

struct ModlMap *modl_map_resize(struct ModlMap *self);

struct ModlMap *modl_map_init(struct ModlMap * self, size_t initial_size);

struct ModlObject* modl_map_get(struct ModlMap * self, struct ModlObject key);

void modl_map_set(struct ModlMap *self, struct ModlObject key, struct ModlObject val);
