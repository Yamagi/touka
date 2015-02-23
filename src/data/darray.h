/*
 * darray.h
 * --------
 *
 * A simple dynamic array. This special type
 * of array is grown and narrowed automatically
 * as necessary.
 */

#ifndef DARRAY_H_
#define DARRAY_H_

#include <stdint.h>
#include <stdlib.h>

// --------

// Represents a dynamic array
typedef struct darray
{
	int32_t elements;
	int32_t end;
	void **data;

} darray;

/*
 * Returns a new dynamic array
 *
 * size: Size of one element
 */
darray *darray_create(void);

/*
 * Destroys a dynamic array. When the
 * optional callback is given, the
 * contents of each element is passed
 * to it for destruction.
 *
 * array: Dynamic array to destroy
 * callback: Optional callback function
 */
void darray_destroy(darray *array, void (*callback)(char *data));

/*
 * Adds an element to the end if the
 * dynamic array.
 *
 * array: Dynamic array to add data to
 * data: Data to add
 */
void darray_push(darray *array, void *data);

/*
 * Return the contents of the last
 * element and removes the element
 * from the array.
 *
 * array: Dynamic array to retrieve the
 *        data from
 */
void *darray_pop(darray *array);

/*
 * Returns the contents of an appriatary
 * element.
 *
 * array: Array to retrieve content from.
 * element: Number of element to retrieve.
 */
void *darray_get(darray *array, int32_t element);

/*
 * Sorts the given dynamic array with
 * qsort().
 *
 * array: Array to sort
 * callback: Callback function passed
 *           to qsort()
 */
void darray_sort(darray *array, int32_t (*callback)(const void*, const void*));

// --------

#endif // DARRAY_H_

