
#ifndef HASHMAP_H_
#define HASHMAP_H_

#include <stdint.h>

#include "darray.h"

typedef struct
{
	const char *key;
	uint32_t hash;
	void *data;
} hashnode;

typedef struct
{
	int32_t buckets;
	void **data;
} hashmap;

/*
 * Creates a new hashmap.
 *
 * buckets: Number of buckets in the map
 */
hashmap *hashmap_create(int32_t buckets);

/*
 * Destroys a hashmap. Each element is passed
 * to the optional callback function for
 * destruction.
 *
 * map: Map to destroy
 * callback: Optional callback
 */
void hashmap_destroy(hashmap *map, void (*callback)(void *data));

/*
 * Adds an element to the hashmap.
 *
 * map: Map to add the element to
 * key: Key
 * data: Data
 */
void hashmap_add(hashmap *map, const char *key, void *data);

/*
 * Retrieves an element from the hashmap.
 *
 * map: Mat to retrieve from
 * key: Key of the element
 */
void *hashmap_get(hashmap *map, const char *key);

#endif // HASHMAP_H_

