#include "utl.c"
#include <utl/assert.h>
#include <utl/log.h>
#include <utl/test.h>

#include <cmc/hashmap.h>

CMC_GENERATE_HASHMAP(hm, hashmap, size_t, size_t)

struct hashmap_ftab_key *hm_ftab_key =
    &(struct hashmap_ftab_key){ .cmp = cmp,
                                .cpy = copy,
                                .str = str,
                                .free = custom_free,
                                .hash = hash,
                                .pri = pri };

struct hashmap_ftab_val *hm_ftab_val =
    &(struct hashmap_ftab_val){ .cmp = cmp,
                                .cpy = copy,
                                .str = str,
                                .free = custom_free,
                                .hash = hash,
                                .pri = pri };

CMC_CREATE_UNIT(hashmap_test, true, {
    CMC_CREATE_TEST(new, {
        struct hashmap *map = hm_new(943722, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);
        cmc_assert_not_equals(ptr, NULL, map->buffer);
        cmc_assert_equals(size_t, 0, hm_count(map));
        cmc_assert_greater_equals(size_t, (943722 / 0.6), hm_capacity(map));

        hm_free(map);
    });

    CMC_CREATE_TEST(new[capacity = 0], {
        struct hashmap *map = hm_new(0, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_equals(ptr, NULL, map);
    });

    CMC_CREATE_TEST(new[capacity = UINT64_MAX], {
        struct hashmap *map =
            hm_new(UINT64_MAX, 0.99, hm_ftab_key, hm_ftab_val);

        cmc_assert_equals(ptr, NULL, map);
    });

    CMC_CREATE_TEST(clear[count capacity], {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        for (size_t i = 0; i < 50; i++)
            hm_insert(map, i, i);

        cmc_assert_equals(size_t, 50, hm_count(map));

        hm_clear(map);

        cmc_assert_equals(size_t, 0, hm_count(map));

        hm_free(map);
    });

    CMC_CREATE_TEST(insert, {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert(hm_insert(map, 1, 1));
        cmc_assert_equals(size_t, 1, hm_count(map));

        hm_free(map);
    });

    CMC_CREATE_TEST(insert[smallest capacity], {
        struct hashmap *map = hm_new(1, 0.99, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert_equals(size_t, cmc_hashtable_primes[0], hm_capacity(map));
        cmc_assert(hm_insert(map, 1, 1));

        hm_free(map);
    });

    CMC_CREATE_TEST(insert[element position], {
        struct hashmap *map = hm_new(1, 0.99, hm_ftab_key, hm_ftab_val);

        // Temporary change
        // Using the numhash the key is the hash itself
        hm_ftab_key->hash = numhash;

        cmc_assert_not_equals(ptr, NULL, map);

        size_t capacity = hm_capacity(map);

        cmc_assert_greater(size_t, 0, capacity);

        cmc_assert(hm_insert(map, capacity - 1, capacity - 1));
        cmc_assert(hm_insert(map, capacity, capacity));

        cmc_assert_equals(size_t, capacity - 1, map->buffer[capacity - 1].key);
        cmc_assert_equals(size_t, capacity, map->buffer[0].key);

        hm_ftab_key->hash = hash;

        hm_free(map);
    });

    CMC_CREATE_TEST(insert[distance], {
        struct hashmap *map = hm_new(500, 0.6, hm_ftab_key, hm_ftab_val);

        // Temporary change
        hm_ftab_key->hash = hash0;

        cmc_assert_not_equals(ptr, NULL, map);

        for (size_t i = 0; i < 200; i++)
            cmc_assert(hm_insert(map, i, i));

        // Everything is hashed to 0 so key 0 should have dist 0, key 1 dist 1,
        // etc
        for (size_t i = 0; i < 200; i++)
            cmc_assert_equals(size_t, i, map->buffer[i].dist);

        hm_ftab_key->hash = hash;

        hm_free(map);
    });

    CMC_CREATE_TEST(insert[distance wrap], {
        struct hashmap *map = hm_new(1, 0.99, hm_ftab_key, hm_ftab_val);

        // Temporary change
        hm_ftab_key->hash = hashcapminus1;

        cmc_assert_not_equals(ptr, NULL, map);

        size_t capacity = hm_capacity(map);

        // Just to be sure, not part of the test
        cmc_assert_equals(size_t, cmc_hashtable_primes[0] - 1,
                          hashcapminus1(0));
        cmc_assert_equals(size_t, capacity - 1, hashcapminus1(0));

        cmc_assert(hm_insert(map, 0, 0));
        cmc_assert(hm_insert(map, 1, 1));

        cmc_assert_equals(size_t, 0, map->buffer[capacity - 1].dist);
        cmc_assert_equals(size_t, 1, map->buffer[0].dist);

        hm_ftab_key->hash = hash;

        hm_free(map);
    });

    CMC_CREATE_TEST(insert[distances], {
        struct hashmap *map = hm_new(1, 0.99, hm_ftab_key, hm_ftab_val);

        // Temporary change
        hm_ftab_key->hash = hashcapminus4;

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert_equals(size_t, cmc_hashtable_primes[0], hm_capacity(map));

        for (size_t i = 0; i < 6; i++)
            cmc_assert(hm_insert(map, i, i));

        for (size_t i = 0; i < 6; i++)
            cmc_assert_equals(size_t, i, hm_impl_get_entry(map, i)->dist);

        hm_ftab_key->hash = hash;

        hm_free(map);
    });

    CMC_CREATE_TEST(insert[buffer growth and item preservation], {
        struct hashmap *map = hm_new(1, 0.99, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert_equals(size_t, cmc_hashtable_primes[0], hm_capacity(map));

        size_t p = 0;
        for (size_t i = 0; i < 100000; i++)
        {
            if (hm_full(map))
                p++;

            cmc_assert(hm_insert(map, i, i));
        }

        cmc_assert_equals(size_t, cmc_hashtable_primes[p], hm_capacity(map));

        for (size_t i = 0; i < 100000; i++)
            cmc_assert(hm_contains(map, i));

        hm_free(map);
    });

    CMC_CREATE_TEST(update, {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert(hm_insert(map, 1, 1));

        size_t old;
        cmc_assert(hm_update(map, 1, 2, &old));

        cmc_assert_equals(size_t, 1, old);

        hm_free(map);
    });

    CMC_CREATE_TEST(update[key not found], {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        for (size_t i = 0; i < 100; i++)
            cmc_assert(hm_insert(map, i, i));

        cmc_assert(!hm_update(map, 120, 120, NULL));

        hm_free(map);
    });

    CMC_CREATE_TEST(remove, {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        for (size_t i = 0; i < 100; i++)
            cmc_assert(hm_insert(map, i, i));

        for (size_t i = 0; i < 100; i++)
            cmc_assert(hm_remove(map, i, NULL));

        hm_free(map);
    });

    CMC_CREATE_TEST(remove[count = 0], {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        for (size_t i = 100; i < 200; i++)
            cmc_assert(!hm_remove(map, i, NULL));

        hm_free(map);
    });

    CMC_CREATE_TEST(remove[key not found], {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        for (size_t i = 0; i < 100; i++)
            cmc_assert(hm_insert(map, i, i));

        for (size_t i = 100; i < 200; i++)
            cmc_assert(!hm_remove(map, i, NULL));

        hm_free(map);
    });

    CMC_CREATE_TEST(max, {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        for (size_t i = 1; i <= 100; i++)
            cmc_assert(hm_insert(map, i, i));

        size_t key;
        size_t val;
        cmc_assert(hm_max(map, &key, &val));

        cmc_assert_equals(size_t, 100, key);
        cmc_assert_equals(size_t, 100, val);

        hm_free(map);
    });

    CMC_CREATE_TEST(max[count = 0], {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert(!hm_min(map, NULL, NULL));

        hm_free(map);
    });

    CMC_CREATE_TEST(min, {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        for (size_t i = 1; i <= 100; i++)
            cmc_assert(hm_insert(map, i, i));

        size_t key;
        size_t val;
        cmc_assert(hm_min(map, &key, &val));

        cmc_assert_equals(size_t, 1, key);
        cmc_assert_equals(size_t, 1, val);

        hm_free(map);
    });

    CMC_CREATE_TEST(min[count = 0], {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert(!hm_min(map, NULL, NULL));

        hm_free(map);
    });

    CMC_CREATE_TEST(get, {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert(hm_insert(map, 4321, 1234));

        cmc_assert_equals(size_t, 1234, hm_get(map, 4321));

        hm_free(map);
    });

    CMC_CREATE_TEST(get[count = 0], {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert_equals(size_t, (size_t){ 0 }, hm_get(map, 4321));

        hm_free(map);
    });

    CMC_CREATE_TEST(get_ref, {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert(hm_insert(map, 4321, 1234));

        size_t *result = hm_get_ref(map, 4321);

        cmc_assert_not_equals(ptr, NULL, result);
        cmc_assert_equals(size_t, 1234, *result);

        hm_free(map);
    });

    CMC_CREATE_TEST(get_ref[count = 0], {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert_equals(ptr, NULL, hm_get_ref(map, 4321));

        hm_free(map);
    });

    CMC_CREATE_TEST(contains, {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert(hm_insert(map, 987654321, 1));

        cmc_assert(hm_contains(map, 987654321));

        hm_free(map);
    });

    CMC_CREATE_TEST(contains[count = 0], {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert(!hm_contains(map, 987654321));

        hm_free(map);
    });

    CMC_CREATE_TEST(contains[sum], {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        for (size_t i = 101; i <= 200; i++)
            cmc_assert(hm_insert(map, i, i));

        size_t sum = 0;
        for (size_t i = 1; i <= 300; i++)
            if (hm_contains(map, i))
                sum += i;

        cmc_assert_equals(size_t, 15050, sum);

        hm_free(map);
    });

    CMC_CREATE_TEST(empty, {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert(hm_empty(map));

        cmc_assert(hm_insert(map, 1, 1));

        cmc_assert(!hm_empty(map));

        hm_free(map);
    });

    CMC_CREATE_TEST(full, {
        struct hashmap *map =
            hm_new(cmc_hashtable_primes[0], 0.99999, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert_equals(size_t, cmc_hashtable_primes[0], hm_capacity(map));

        for (size_t i = 0; i < hm_capacity(map); i++)
            cmc_assert(hm_insert(map, i, i));

        cmc_assert(hm_full(map));

        cmc_assert(hm_insert(map, 10000, 10000));

        cmc_assert_equals(size_t, cmc_hashtable_primes[1], hm_capacity(map));

        cmc_assert(!hm_full(map));

        hm_free(map);
    });

    CMC_CREATE_TEST(count, {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        for (size_t i = 1; i < 100; i++)
        {
            cmc_assert(hm_insert(map, i, i));
            cmc_assert_equals(size_t, i, hm_count(map));
        }

        hm_free(map);
    });

    CMC_CREATE_TEST(capacity, {
        struct hashmap *map = hm_new(2500, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert_greater_equals(size_t, (size_t)(2500 / 0.6),
                                  hm_capacity(map));

        hm_free(map);
    });

    CMC_CREATE_TEST(capacity[small], {
        struct hashmap *map = hm_new(1, 0.99, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert_equals(size_t, cmc_hashtable_primes[0], hm_capacity(map));

        hm_free(map);
    });

    CMC_CREATE_TEST(load, {
        struct hashmap *map = hm_new(1, 0.99, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert_in_range(double, 0.99 - 0.0001, 0.99 + 0.0001, hm_load(map));

        hm_free(map);
    });

    CMC_CREATE_TEST(flags, {
        struct hashmap *map = hm_new(100, 0.6, hm_ftab_key, hm_ftab_val);

        cmc_assert_not_equals(ptr, NULL, map);
        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map));

        // insert
        cmc_assert(hm_insert(map, 1, 1));
        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map));
        cmc_assert(!hm_insert(map, 1, 2));
        cmc_assert_equals(int32_t, cmc_flags.DUPLICATE, hm_flag(map));

        // clear
        hm_clear(map);
        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map));

        // update
        cmc_assert(!hm_update(map, 1, 2, NULL));
        cmc_assert_equals(int32_t, cmc_flags.EMPTY, hm_flag(map));

        cmc_assert(hm_insert(map, 1, 1));
        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map));

        cmc_assert(!hm_update(map, 2, 1, NULL));
        cmc_assert_equals(int32_t, cmc_flags.NOT_FOUND, hm_flag(map));

        cmc_assert(hm_update(map, 1, 2, NULL));
        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map));

        // remove
        cmc_assert(!hm_remove(map, 2, NULL));
        cmc_assert_equals(int32_t, cmc_flags.NOT_FOUND, hm_flag(map));

        cmc_assert(hm_remove(map, 1, NULL));
        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map));

        cmc_assert(!hm_remove(map, 1, NULL));
        cmc_assert_equals(int32_t, cmc_flags.EMPTY, hm_flag(map));

        // max
        cmc_assert(hm_insert(map, 1, 1));
        cmc_assert(hm_max(map, NULL, NULL));
        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map));

        cmc_assert(hm_remove(map, 1, NULL));
        cmc_assert(!hm_max(map, NULL, NULL));
        cmc_assert_equals(int32_t, cmc_flags.EMPTY, hm_flag(map));

        // min
        cmc_assert(hm_insert(map, 1, 1));
        cmc_assert(hm_min(map, NULL, NULL));
        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map));

        cmc_assert(hm_remove(map, 1, NULL));
        cmc_assert(!hm_min(map, NULL, NULL));
        cmc_assert_equals(int32_t, cmc_flags.EMPTY, hm_flag(map));

        // get
        hm_get(map, 1);
        cmc_assert_equals(int32_t, cmc_flags.EMPTY, hm_flag(map));

        cmc_assert(hm_insert(map, 1, 1));
        hm_get(map, 1);
        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map));

        hm_get(map, 2);
        cmc_assert_equals(int32_t, cmc_flags.NOT_FOUND, hm_flag(map));

        hm_clear(map);

        // get_ref
        hm_get_ref(map, 1);
        cmc_assert_equals(int32_t, cmc_flags.EMPTY, hm_flag(map));

        cmc_assert(hm_insert(map, 1, 1));
        hm_get_ref(map, 1);
        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map));

        hm_get_ref(map, 2);
        cmc_assert_equals(int32_t, cmc_flags.NOT_FOUND, hm_flag(map));

        // contains
        hm_contains(map, 1);
        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map));

        // copy
        struct hashmap *map2 = hm_copy_of(map);

        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map));
        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map2));

        size_t tmp = map->capacity;
        map->capacity = 0;

        struct hashmap *map3 = hm_copy_of(map);
        cmc_assert_equals(ptr, NULL, map3);

        cmc_assert_equals(int32_t, cmc_flags.ERROR, hm_flag(map));

        map->capacity = tmp;

        cmc_assert(hm_equals(map, map2));
        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map));
        cmc_assert_equals(int32_t, cmc_flags.OK, hm_flag(map2));

        hm_free(map);
        hm_free(map2);
    });

    CMC_CREATE_TEST(callbacks, {
        struct hashmap *map =
            hm_new_custom(100, 0.6, hm_ftab_key, hm_ftab_val, NULL, callbacks);

        cmc_assert_not_equals(ptr, NULL, map);

        cmc_assert(hm_insert(map, 1, 2));
        cmc_assert_equals(int32_t, 1, total_create);

        cmc_assert(hm_update(map, 1, 10, NULL));
        cmc_assert_equals(int32_t, 1, total_update);

        cmc_assert(hm_remove(map, 1, NULL));
        cmc_assert_equals(int32_t, 1, total_delete);

        cmc_assert(hm_insert(map, 1, 2));
        cmc_assert_equals(int32_t, 2, total_create);

        cmc_assert(hm_max(map, NULL, NULL));
        cmc_assert_equals(int32_t, 1, total_read);

        cmc_assert(hm_min(map, NULL, NULL));
        cmc_assert_equals(int32_t, 2, total_read);

        cmc_assert_equals(size_t, 2, hm_get(map, 1));
        cmc_assert_equals(int32_t, 3, total_read);

        cmc_assert_not_equals(ptr, NULL, hm_get_ref(map, 1));
        cmc_assert_equals(int32_t, 4, total_read);

        cmc_assert(hm_contains(map, 1));
        cmc_assert_equals(int32_t, 5, total_read);

        cmc_assert(hm_resize(map, 10000));
        cmc_assert_equals(int32_t, 1, total_resize);

        cmc_assert(hm_resize(map, 500));
        cmc_assert_equals(int32_t, 2, total_resize);

        cmc_assert_equals(int32_t, 2, total_create);
        cmc_assert_equals(int32_t, 5, total_read);
        cmc_assert_equals(int32_t, 1, total_update);
        cmc_assert_equals(int32_t, 1, total_delete);
        cmc_assert_equals(int32_t, 2, total_resize);

        hm_customize(map, NULL, NULL);

        cmc_assert_equals(ptr, NULL, map->callbacks);

        hm_clear(map);
        cmc_assert(hm_insert(map, 1, 2));
        cmc_assert(hm_update(map, 1, 10, NULL));
        cmc_assert(hm_remove(map, 1, NULL));
        cmc_assert(hm_insert(map, 1, 2));
        cmc_assert(hm_max(map, NULL, NULL));
        cmc_assert(hm_min(map, NULL, NULL));
        cmc_assert_equals(size_t, 2, hm_get(map, 1));
        cmc_assert_not_equals(ptr, NULL, hm_get_ref(map, 1));
        cmc_assert(hm_contains(map, 1));
        cmc_assert(hm_resize(map, 10000));
        cmc_assert(hm_resize(map, 500));

        cmc_assert_equals(int32_t, 2, total_create);
        cmc_assert_equals(int32_t, 5, total_read);
        cmc_assert_equals(int32_t, 1, total_update);
        cmc_assert_equals(int32_t, 1, total_delete);
        cmc_assert_equals(int32_t, 2, total_resize);

        cmc_assert_equals(ptr, NULL, map->callbacks);

        hm_free(map);

        total_create = 0;
        total_read = 0;
        total_update = 0;
        total_delete = 0;
        total_resize = 0;
    });
});
