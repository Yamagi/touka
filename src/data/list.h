/*
 * This is a simple doubled linked list.
 * Elements can be accessed from front
 * and back.
 */

#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

/*
 * One node in a list.
 */
typedef struct listnode
{
	struct listnode *prev;
	struct listnode *next;
	void *data;
} listnode;

/*
 * Header of a list.
 */
typedef struct list
{
	listnode *first;
	listnode *last;
	int count;
} list;

/*
 * Creates a new list.
 */
list *list_create(void);

/*
 * Destroys a list. All elements (including
 * *data) are freed. If a callback function
 * is provided, the contents of each element
 * is passed to it before destruction. This
 * can be used to destroy nested structs and
 * other things.
 *
 * lheader: List to destroy
 * callback: Optional callback function
 */
void list_destroy(list *lheader, void (*callback)());

/*
 * Creates an new element and
 * appends it to the list.
 *
 * data: Data to append
 */
void list_push(list *lheader, void *data);

/*
 * Returns the last elements data
 * and removes the element.
 */
void *list_pop(list *lheader);

/* Creates an new element and
 * unshifts it into the list.
 *
 * data: Data to unshift
 */
void list_unshift(list *lheader, void *data);

/*
 * Returns the first elements data
 * and removes the data.
 */
void *list_shift(list *lheader);

#endif // LINKEDLIST_H_

