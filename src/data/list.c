/*
 * list.c
 * ------
 *
 * A simple double linked list
 */

#include <assert.h>
#include <stdlib.h>

#include "list.h"

#include "../quit.h"

// --------

/*********************************************************************
 *                                                                   *
 *                          Public Interface                         *
 *                                                                   *
 *********************************************************************/

list
*list_create(void)
{
	list *new;

	if ((new = calloc(1, sizeof(list))) == NULL)
	{
		quit_error(POUTOFMEM);
	}

	return new;
}

void
list_destroy(list *lheader, void (*callback)(void *data))
{
	listnode *cur;

	assert(lheader);

	while (lheader->last)
	{
		cur = lheader->last;

		if (cur->prev)
		{
			cur->prev->next = NULL;
			lheader->last = cur->prev;
		}
		else
		{
			lheader->first = NULL;
			lheader->last = NULL;
		}

		lheader->count--;

		if (callback)
		{
			callback(cur->data);
		}
		else
		{
			free(cur->data);
		}

		free(cur);
	}

	assert(lheader->count == 0);
	assert(lheader->first == 0);
	assert(lheader->last == 0);

	free(lheader);
}

void
*list_pop(list *lheader)
{
	listnode *cur;
	void *data;

	assert(lheader);
	assert(lheader->count);

	cur = lheader->last;
	data = cur->data;

	if (cur->prev)
	{
		cur->prev->next = NULL;
		lheader->last = cur->prev;
	}
	else
	{
		lheader->first = NULL;
		lheader->last = NULL;
	}

	lheader->count--;
	free(cur);

	return data;
}

void
list_push(list *lheader, void *data)
{
	listnode *new;

	assert(lheader);
	assert(data);

	if ((new = calloc(1, sizeof(listnode))) == NULL)
	{
		quit_error(POUTOFMEM);
	}

	new->data = data;

	if (!lheader->first)
	{
		lheader->first = new;
		lheader->last = new;
	}
	else
	{
		new->prev = lheader->last;
		new->prev->next = new;
		lheader->last = new;
	}

	lheader->count++;
}

void
*list_shift(list *lheader)
{
	listnode *cur;
	void *data;

	assert(lheader);
	assert(lheader->count);

	cur = lheader->first;
	data = cur->data;

	if (cur->next)
	{
		cur->next->prev = NULL;
		lheader->first = cur->next;
	}
	else
	{
		lheader->first = NULL;
		lheader->last = NULL;
	}

	lheader->count--;
	free(cur);

	return data;

}

void
list_sort(list *lheader, int32_t (*callback)(const void *, const void *))
{
	listnode *cur;
	listnode **larray;
	uint16_t i;

	assert(lheader);

	if (!lheader->count)
	{
		return;
	}

	if ((larray = malloc(lheader->count * sizeof(listnode *))) == NULL)
	{
		quit_error(POUTOFMEM);
	}

	cur = lheader->first;
	i = 0;

	while (cur)
	{
		larray[i] = cur;

		cur = cur->next;
		i++;
	}

	qsort(larray, lheader->count, sizeof(listnode *), callback);

	for (i = 0; i < lheader->count; i++)
	{
		if (i == 0)
		{
			larray[i]->prev = NULL;
			lheader->first = larray[i];
		}
		else
		{
			larray[i]->prev = larray[i - 1];
		}

		if (i == lheader->count - 1)
		{
			larray[i]->next = NULL;
			lheader->last = larray[i];
		}
		else
		{
			larray[i]->next = larray[i + 1];
		}
	}

	free(larray);
}

void
list_unshift(list *lheader, void *data)
{
	listnode *new;

	assert(lheader);
	assert(data);

	if ((new = calloc(1, sizeof(listnode))) == NULL)
	{
		quit_error(POUTOFMEM);
	}

	new->data = data;

	if (!lheader->first)
	{
		lheader->first = new;
		lheader->last = new;
	}
	else
	{
		new->next = lheader->first;
		lheader->first->prev = new;
		lheader->first = new;
	}

	lheader->count++;
}
