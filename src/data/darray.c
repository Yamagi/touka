/*
 * darray.c:
 *  - Simple dynamic array
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "darray.h"
#include "../util.h"

// ----

// Initial number of elements
#define INT_ELEMENTS 16

// ----

/*
 * Resizes the array if necessary.
 *
 * array: Dynamic array to resize
 */
static void
darray_resize(darray *array)
{
	size_t new;

	assert(array);

	if (array->elements == array->end)
	{
		new = (array->elements * sizeof(void *)) * sizeof(void *);
	}
	else if (array->end - array->elements > array->end / 2)
	{
		new = (array->end * sizeof(void *)) / 2;
	}
	else
	{
		return;
	}

	if ((array->data = realloc(array->data, new)) == NULL)
	{
		perror("PANIC: Couldn't allocate memory");
		quit_error();
	}

	array->end = new;
}

// ----

darray
*darray_create(void)
{
	darray *new;

	if ((new = malloc(sizeof(darray))) == NULL)
	{
		perror("PANIC: Couldn't allocate memory");
		quit_error();
	}

	new->elements = 0;
	new->end = INT_ELEMENTS;

	if ((new->data = malloc(sizeof(void *) * INT_ELEMENTS)) == NULL)
	{
		perror("PANIC: Couldn't allocate memory");
		quit_error();
	}

	return new;
}

void
darray_destroy(darray *array, void (*callback)(char *msg))
{
	int i;

	assert(array);

    for (i = array->elements - 1; i > 0; i--)
	{
		if (callback)
		{
			callback(darray_get(array, i));
		}
		else
		{
			free(darray_get(array, i));
		}
	}

	free(array->data);
	free(array);
	array = NULL;
}

void
darray_push(darray *array, void *data)
{
	assert(array);
	assert(data);

	array->data[array->elements] = data;
	array->elements++;

	darray_resize(array);
}

void
*darray_pop(darray *array)
{
	void *data;

	assert(array);
    assert(array->elements);

	data = array->data[array->elements - 1];
	array->elements--;

	darray_resize(array);

	return data;
}

void
*darray_get(darray *array, int32_t element)
{
	assert(array);
	assert(array->elements > element);

	return array->data[element];
}

void
darray_sort(darray *array, int32_t (*callback)(const void *, const void*))
{
	assert(array);
	assert(callback);

	qsort(array->data, array->elements, sizeof(void *), callback);
}

