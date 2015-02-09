/*
 * linkedlist.c:
 *  - A simple, generic double linked list
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "linkedlist.h"
#include "../utility.h"

// --------

list
*listcreate(void)
{
	list *new;

	if ((new = calloc(1, sizeof(list))) == NULL)
	{
		perror("PANIC: Couldn't allocate memory");
		quit(1);
	}

	return new;
}

void
listdestroy(list *lheader, void (*callback)())
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

		free(cur);
	}

	assert(lheader->count == 0);
	assert(lheader->first == 0);
	assert(lheader->last == 0);

	free(lheader);
}

// --------

void
listpush(list *lheader, void *data)
{
	listnode *new;

	assert(lheader);
	assert(data);

	if ((new = calloc(1, sizeof(listnode))) == NULL)
	{
		perror("PANIC: Couldn't allocate memory");
		quit(1);
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
*listpop(list *lheader)
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
listunshift(list *lheader, void *data)
{
	listnode *new;

	assert(lheader);
	assert(data);

	if ((new = calloc(1, sizeof(listnode))) == NULL)
	{
		perror("PANIC: Couldn't allocate memory");
		quit(1);
	}

	new->data = data;

	if (!lheader->first)
	{
		lheader->first = NULL;
		lheader->last = NULL;
	}
	else
	{
        new->next = lheader->first;
		lheader->first->prev = new;
		lheader->first = new;
	}

	lheader->count++;
}

void
*listshift(list *lheader)
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
