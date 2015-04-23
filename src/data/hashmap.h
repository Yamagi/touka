/*
 * hashmap.h
 * ---------
 *
 * A simple hashmap implementattion, based
 * around dynamic arrays and jenkins hash.
 */

// --------

#ifndef HASHMAP_H_
#define HASHMAP_H_

// --------

#include <stdint.h>

#include "darray.h"
#include "list.h"
#include "list.h"

#include "../main.h"

// --------

typedef struct
{
	const char *key;
	uint32_t hash;
	void *data;
	int8_t is_alias;
} hashnode;

typedef struct
{
	int32_t buckets;
	void **data;
} hashmap;

// --------

/*
 * Adds an element to the hashmap.
 *
 * map: Map to add the element to
 * key: Key
 * data: Data
 */
void hashmap_add(hashmap *map, const char *key, void *data, boolean alias);

/*
 * Creates a new hashmap.
 *
 * buckets: Number of buckets in the map
 */
hashmap *hashmap_create(uint16_t buckets);

/*
 * Destroys a hashmap. Each element is passed
 * to the optional callback function for
 * destruction.
 *
 * map: Map to destroy
 * callback: Optional callback
 */
void hashmap_destroy(hashmap *map, void (callback)(void *data));

/*
 * Retrieves an element from the hashmap.
 *
 * map: Mat to retrieve from
 * key: Key of the element
 */
void *hashmap_get(hashmap *map, const char *key);

/*
 * Returns a list with all elements of the
 * hashmap. The list is not sorted.
 *
 * map: Map to create the list from
 */
list *hashmap_to_list(hashmap *map);

// --------

#endif // HASHMAP_H_
