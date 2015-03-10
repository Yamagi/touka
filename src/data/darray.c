/*
 * darray.c
 * --------
 *
 * A very simple dynamic array, which is in fact
 * more some kind of double ended queue.
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "darray.h"
#include "../quit.h"

// Initial number of elements
#define INT_ELEMENTS 16

// --------

/*********************************************************************
 *                                                                   *
 *                        Support Functions                          *
 *                                                                   *
 *********************************************************************/

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
	else if (array->end - array->elements > array->end / 2 && array->end > INT_ELEMENTS)
	{
		new = (array->end * sizeof(void *)) / 2;
	}
	else
	{
		return;
	}

	if ((array->data = realloc(array->data, new)) == NULL)
	{
		quit_error("Couldn't allocate memory");
	}

	array->end = new;
}

// ----

/*********************************************************************
 *                                                                   *
 *                         Public Interface                          *
 *                                                                   *
 *********************************************************************/

darray
*darray_create(void)
{
	darray *new;

	if ((new = malloc(sizeof(darray))) == NULL)
	{
		quit_error("Couldn't allocate memory");
	}

	new->elements = 0;
	new->end = INT_ELEMENTS;

	if ((new->data = malloc(sizeof(void *) * INT_ELEMENTS)) == NULL)
	{
		quit_error("Couldn't allocate memory");
	}

	return new;
}

void
darray_destroy(darray *array, void (*callback)(char *msg))
{
	int16_t i;

	assert(array);

    for (i = array->elements - 1; i >= 0; i--)
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
*darray_get(darray *array, uint32_t element)
{
	assert(array);
	assert(array->elements > element);

	return array->data[element];
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
darray_push(darray *array, void *data)
{
	assert(array);
	assert(data);

	array->data[array->elements] = data;
	array->elements++;

	darray_resize(array);
}

void
darray_sort(darray *array, int32_t (*callback)(const void *, const void*))
{
	assert(array);
	assert(callback);

	qsort(array->data, array->elements, sizeof(void *), callback);
}

