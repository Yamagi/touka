/*
 * hashmap.c:
 *  - A simple hashmap
 */

#include "assert.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "darray.h"
#include "hashmap.h"
#include "../util.h"

// --------

/*
 * An implementation of Jenkins Hash
 */
static uint32_t
hashmap_hash(const char *key)
{
	int32_t i;
	size_t len;
	uint32_t hash;

	hash = 0;
	len = strlen(key);

	for (i = 0; i < len; i++)
	{
		hash += key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}

// --------

hashmap
*hashmap_create(int32_t buckets)
{
	hashmap *new;

	if ((new = malloc(sizeof(hashmap))) == NULL)
	{
		perror("PANIC: Couldn't allocate memory");
		quit_error();
	}

	if ((new->data = calloc(buckets, sizeof(darray *))) == NULL)
	{
		perror("PANIC: Couldn't allocate memory");
		quit_error();
	}

	new->buckets = buckets;

	return new;
}

void
hashmap_destroy(hashmap *map, void (*callback)(void *data))
{
	darray *array;
	hashnode *node;
	int32_t i;

	assert(map);

	for (i = 0; i < map->buckets; i++)
	{
		if (map->data[i])
		{
			array = map->data[i];

			while (array->elements)
			{
				node = darray_pop(array);

				if (callback)
				{
					callback(node->data);
				}
				else
				{
					free(node->data);
				}

				free(node);
			}

			darray_destroy(array, NULL);
		}
	}

	free(map);
}

void
hashmap_add(hashmap *map, const char *key, void *data)
{
	hashnode *node;
	int32_t bucket;

	assert(map);
	assert(key);
	assert(data);

	if ((node = malloc(sizeof(hashnode))) == NULL)
	{
		perror("PANIC: Couldn't allocate memory");
		quit_error();
	}

	node->key = key;
	node->data = data;
	node->hash = hashmap_hash(key);

	bucket = node->hash % map->buckets;

	if (!map->data[bucket])
	{
		map->data[bucket] = darray_create();
		darray_push(map->data[bucket], node);
	}
	else
	{
		darray_push(map->data[bucket], node);
	}
}

void
*hashmap_get(hashmap *map, const char *key)
{
	darray *array;
	hashnode *node;
	int32_t bucket;
	int32_t i;
	uint32_t hash;

	assert(map);
	assert(key);

	hash = hashmap_hash(key);
	bucket = hash % map->buckets;

	if (!map->data[bucket])
	{
		return NULL;
	}
	else
	{
		array = map->data[bucket];

        for (i = 0; i < array->elements; i++)
		{
			node = darray_get(array, i);

			if (node->hash == hash)
			{
				return node->data;
			}
		}
	}

	return NULL;
}

