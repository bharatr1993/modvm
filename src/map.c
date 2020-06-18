#include <stdio.h>
#include <stdlib.h>

#include "map.h"
#include "object.h"


struct ModlMap *modl_map_init(struct ModlMap *self, size_t initial_size)
{
    self->vec = calloc(initial_size, sizeof(struct ModlMapBucket));
    self->capacity = initial_size;
    self->size = 0;

    return self;
}

void modl_map_dispose(struct ModlMap *self)
{
    // modl_map_print(self);
    
    for (uint32_t i = 0; i < self->capacity; ++i)
    {
        // printf("removing %d bucket\n", i);
        struct ModlMapBucket * b = &self->vec[i];
        modl_object_release(b->obj);
        modl_object_release(b->key);
        b = b->next;
        while (NULL != b)
        {
            if (b->next)
            {
                modl_object_release(b->obj);
                modl_object_release(b->key);
            }
            struct ModlMapBucket *tmp = b;
            b = b->next;
            free(tmp);
        }
    }
    
    free(self->vec);
    // DO NOT!!!
    // free(self);
}

struct ModlMap * modl_map_resize(struct ModlMap * self)
{
    // printf("%s\n", "BEFORE REALLOC:");
    // modl_map_print(self);
    // printf("%c", '\n');

    uint32_t old_capacity = self->capacity;
    self->capacity *= 2;
    self->vec = realloc(self->vec, sizeof(struct ModlMapBucket) * self->capacity);
    // zero memory after reallocation
    memset(&self->vec[old_capacity], 0, old_capacity * sizeof (struct ModlMapBucket));

    // printf("increasing capacity from %d to %d\n", old_capacity, self->capacity);

    for (uint32_t i = 0; i < old_capacity; ++i)
    {
        struct ModlMapBucket *bkt_prev = NULL;
        struct ModlMapBucket *bkt = &self->vec[i];

        while (bkt->next)
        {
            struct ModlMapBucket * next_bucket = bkt->next;

            uint32_t new_index = modl_object_hash(bkt->key) % self->capacity;
            // printf("  index change from %d to %d\n", i, new_index);
            // printf("  changing key ");
            // modl_object_display(&bkt->key);
            // printf(" with hash=%u and index=%u->%u\n", modl_object_hash(bkt->key), i, new_index);
            if (new_index != i)
            {
                // printf("%s\n", "    moving!");

                struct ModlMapBucket tmp_obj;
                tmp_obj = self->vec[new_index];  // [OB] [NB] [tmp_object = NB]
                self->vec[new_index] = *bkt;     // [NB = OB]

                if (NULL == bkt_prev)
                {
                    struct ModlMapBucket * tmp_ref = bkt->next;
                    self->vec[i] = *bkt->next;
                    next_bucket = &self->vec[i];
                    bkt = tmp_ref;
                }
                else
                {
                    // [OB->...->_remove_->next ]
                    bkt_prev->next = bkt->next;
                }

                *bkt = tmp_obj;
                self->vec[new_index].next = bkt;
            }
            else
            {
                bkt_prev = bkt;
            }

            bkt = next_bucket;
        }
    }

    // printf("%s\n", "AFTER REALLOC:");
    // modl_map_print(self);
    // printf("%c", '\n');

    return self;
}

struct ModlObject * modl_map_get(struct ModlMap *self, struct ModlObject key)
{
    unsigned int hash = modl_object_hash(key);
    // printf("getting key ");
    // modl_object_display(&key);
    // printf(" with hash=%u and index=%u\n", hash, hash % self->capacity);
    hash %= self->capacity;

    struct ModlMapBucket *bkt_iter = &self->vec[hash];

    while (bkt_iter->next)
    {
        if (modl_object_equals(key, bkt_iter->key))
        {
            // printf("%s", "  found! ");
            // modl_object_display(&bkt_iter->obj);
            // printf("%c", '\n');
            return &bkt_iter->obj;
        }

        bkt_iter = bkt_iter->next;
    }

    return NULL;
}

void modl_map_print(struct ModlMap * self)
{
    printf("%s", "[ ");
    for (uint32_t i = 0; i < self->capacity; ++i)
    {
        // printf("  bucket #%d [ ", i);
        struct ModlMapBucket * bkt = &self->vec[i];
        while (bkt && bkt->next)
        {
            // printf("(%lx)", (size_t)bkt);
            modl_object_display(&bkt->key);
            printf("%s", ": ");
            modl_object_display(&bkt->obj);
            printf("%s", ", ");

            bkt = bkt->next;
        }
        // printf("%c\n", ']');
    }
    printf("%c", ']');
}

void modl_map_set(struct ModlMap * self, struct ModlObject key, struct ModlObject val)
{   
    unsigned int hash = modl_object_hash(key);
    // printf("adding key ");
    // modl_object_display(&key);
    // printf(" with hash=%u and index=%u\n", hash, hash % self->capacity);
    hash %= self->capacity;

    // modl_map_print(self);
    // printf("%c", '\n');

    struct ModlMapBucket *bkt_iter;
    bkt_iter = &self->vec[hash];

    while (bkt_iter->next)
    {
        if (modl_object_equals(key, bkt_iter->key))
        {
            // and
            modl_object_take(val);
            modl_object_release(bkt_iter->obj);
            bkt_iter->obj = val;
            return;
        }

        bkt_iter = bkt_iter->next;
    }

    if (self->size > 2*self->capacity/3)
    {
        modl_map_resize(self);
        hash = modl_object_hash(key);
        // printf("adding key ");
        // modl_object_display(&key);
        // printf(" with hash=%u and index=%u\n", hash, hash % self->capacity);
        bkt_iter = &self->vec[hash % self->capacity];
        while (bkt_iter->next) bkt_iter = bkt_iter->next;
    }

    self->size += 1;
    *bkt_iter = (struct ModlMapBucket) {
        .key = modl_object_take(key),
        .obj = modl_object_take(val),
        .next = calloc(1, sizeof (struct ModlMapBucket))
    };
}
