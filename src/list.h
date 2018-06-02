#ifndef _LIST_H_
#define _LIST_H_

#include "typedefs.h"


typedef struct ListElement_ {
	void *data;
	struct ListElement_ *prev;
	struct ListElement_ *next;
} ListElement;

typedef struct {
	u32 size;

	void (*destroy)(void *data);

	ListElement *head;
	ListElement *tail;
} List;


/* 
Returns the number of elements in the given list
*/
#define list_size(list) ((list)->size)

/*
Get the header or tail of the given list
*/ 
#define list_head(list) ((list)->head)
#define list_tail(list) ((list)->tail)

/*
Access the data for the given element
*/
#define list_data(element) ((element)->data)
#define list_next(element) ((element)->next)
#define list_prev(element) ((element)->prev)


/*
list_new creates a new List and must be called before the list can be used.
Pass a pointer to a function that will clean up dynamically allocated
memory used by the data contained in list elements. 
e.g. for malloc, send a pointer to free. For data that shouldn't be cleaned
up, send NULL.
*/
List * list_new(void (*destroy)(void* data));

/*
Inserts an element into the list after the given element.
The new element contains a pointer to data, so the memory referenced
by data should remain valid for the life of the element.
Returns true on success, false on failure.
*/
bool list_insert_after(List *list, ListElement *element, void *data);

// TODO - comment
ListElement * list_item_at(List *list, u32 index);

/*
Removes the given element. If element is NULL, removes the head of the list. 
Returns a pointer to the data stored in the element, or NULL on failure.
*/
void * list_remove(List *list, ListElement *element);

/*
Removes the element containing the given data, if such an element is found. 
*/
void list_remove_element_with_data(List *list, void *data);

/* 
Returns the element containing the given data. Returns NULL if no elements
in the list contain the given data.
*/
ListElement * list_search(List *list, void * data);

/*
Removes all elements from the given list and calls the function 
passed as destroy on the data in each element in the list.
*/
void list_destroy(List *list);


#endif // _LIST_H_
