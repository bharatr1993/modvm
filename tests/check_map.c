
#include <stdio.h>
#include <stdlib.h>

#include "test.h"
#include <src/map.h>
#include <src/object.h>


int test_map()
{
    TEST("map")
    {
        struct ModlMap *map = malloc(sizeof(struct ModlMap));
        TEST("creation")
        {
            INFO_MANUAL(printf("ModlObject size: %ld", sizeof(struct ModlObject)));
            EXPECT(NULL != map, "memory was allocated");

            modl_map_init(map, 2);
            EXPECT(map->capacity > 0, "map has space for elements");
        } END_TEST;

        TEST("get/set")
        {
            struct ModlObject key = str_to_modl("none");
            struct ModlObject keyB = int_to_modl(32);
            struct ModlObject keyC = str_to_modl("b");

            EXPECT(modl_map_get(map, key) == NULL, "Test if non-existing entry is NULL");
            modl_map_set(map, key, key);
            EXPECT(modl_object_equals(*modl_map_get(map, key), key), "Test if added entry can be read");
            EXPECT(modl_map_get(map, keyC) == NULL, "Test if non-existing entry is NULL");
            
            modl_map_set(map, keyB, keyC);
            EXPECT(modl_object_equals(*modl_map_get(map, keyB), keyC), "Access by integer key");
            EXPECT(modl_object_equals(*modl_map_get(map, key), key), "Old key is still accessible");

            modl_object_release_tmp(key);
            modl_object_release_tmp(keyB);
            modl_object_release_tmp(keyC);
        } END_TEST;

        TEST("random integer insers / reads")
        {
            srand(1234);
            struct ModlObject table = modl_table_new();

            const size_t TEST_ENTRIES_COUNT = 1024*256;
            struct ModlObject * keys = malloc(TEST_ENTRIES_COUNT * sizeof (struct ModlObject));
            struct ModlObject * values = malloc(TEST_ENTRIES_COUNT * sizeof (struct ModlObject));
            for (size_t i = 0; i < TEST_ENTRIES_COUNT; ++i)
            {
                keys[i] = int_to_modl(rand());
                values[i] = int_to_modl(rand());
            }

            for (size_t i = 0; i < TEST_ENTRIES_COUNT; ++i)
            {
                // modl_table_insert_kv(table, keys[i], values[i]);
                // const int entry_found = modl_object_equals(modl_table_get_v(table, keys[i]), values[i]);
                modl_map_set(map, keys[i], values[i]);
                // modl_map_print(map);
                // printf("\n");
                struct ModlObject * entry = modl_map_get(map, keys[i]);
                // EXPECT(NULL != entry);
                const int entry_found = modl_object_equals(*entry, values[i]);
                // EXPECT(entry_found);
            }

            for (size_t i = 0; i < TEST_ENTRIES_COUNT; ++i)
            {
                const int entry_found = modl_object_equals(*modl_map_get(map, keys[i]), values[i]);
                // const int entry_found = modl_object_equals(modl_table_get_v(table, keys[i]), values[i]);
                // EXPECT(entry_found);
            }

            for (size_t i = 0; i < TEST_ENTRIES_COUNT; ++i)
            {
                modl_object_release_tmp(keys[i]);
                modl_object_release_tmp(values[i]);
            }
            modl_object_release(table);
            free(keys);
            free(values);
        } END_TEST;
        TEST("random string inserts / reads")
        {
        //    char[]* strs = {"james", "anne", "viktor", "douglas", "bernie", ""} 
        } END_TEST;
        
        // modl_map_print(map);
        modl_map_dispose(map);
    } END_TEST;

    return 0;
}
