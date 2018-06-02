/*
* list.c
*/

#include <stdlib.h>

#include "list.h"

/*
list_new creates a new List and must be called before the list can be used.
Pass a pointer to a function that will clean up dynamically allocated
memory used by the data contained in list elements. 
e.g. for malloc, send a pointer to free. For data that shouldn't be cleaned
up, send NULL.
*/
List * list_new(void (*destroy)(void* data)) {
	List *list = calloc(1, sizeof(List));

	if (list != NULL) {
		list->size = 0;
		list->destroy = destroy;
		list->head = NULL;
		list->tail = NULL; 
	}

	return list;
}

/*
Inserts an element into the list after the given element.
The new element contains a pointer to data, so the memory referenced
by data should remain valid for the life of the element.
Returns true on success, false on failure.
*/
bool list_insert_after(List *list, ListElement *element, void *data) {
	ListElement *newElement;

	if ((newElement = (ListElement *)calloc(1, sizeof(ListElement))) == NULL) {
		return false;
	}

	newElement->data = (void *)data;

	if (element == NULL) {
		// Insert at head of list
		if (list_size(list) == 0) {
			list->tail = newElement;
		} else {
			list->head->prev = newElement;		
		}

		newElement->next = list->head;
		newElement->prev = NULL;
		list->head = newElement;

	} else {
		if (element->next == NULL) {
			list->tail = newElement;
		}

		newElement->next = element->next;
		newElement->prev = element;
		element->next = newElement;
	}

	list->size += 1;
	return true;
}

ListElement * list_item_at(List *list, u32 index) {
	u32 currIdx = 0;
	ListElement *currElement = list->head;

	while (currElement != NULL)	{
		if (currIdx == index) {
			return currElement;
		}
		currIdx += 1;
		currElement = currElement->next;
	}

	return NULL;
}

/*
Removes the given element. If element is NULL, removes the head of the list. 
Returns a pointer to the data stored in the element, or NULL on failure.
*/
void * list_remove(List *list, ListElement *element) {
	ListElement *elementToRemove;
	void *data = NULL;

	if (list_size(list) == 0) {
		return NULL;
	}

	if (element == NULL) {
		data = list->head->data;
		elementToRemove = list->head;
		list->head = list->head->next;
		if (list->head != NULL) {
			list->head->prev = NULL;
		}

	} else {
		data = element->data;
		elementToRemove = element;

		ListElement *prevElement = element->prev;
		ListElement *nextElement = element->next;

		if (prevElement != NULL && nextElement != NULL) {
			prevElement->next = nextElement;
			nextElement->prev = prevElement;

		} else {
			if (prevElement == NULL) {
				// We're at start of list
				if (nextElement != NULL) { nextElement->prev = NULL; }
				list->head = nextElement;
			} 
			if (nextElement == NULL) {
				// We're at end of list
				if (prevElement != NULL) { prevElement->next = NULL; }
				list->tail = prevElement;
			}
		}
	}

	free(elementToRemove);

	list->size -= 1;

	return data;
}


/*
Removes the element containing the given data, if such an element is found. 
*/
void list_remove_element_with_data(List *list, void *data) {
	if ((list_size(list) == 0) || (data == NULL)) {
		return;
	}

	ListElement *elementToRemove = list->head;
	while (elementToRemove != NULL) {
		if (elementToRemove->data == data) {
			break;
		}

		elementToRemove = elementToRemove->next;
	}

	if (elementToRemove != NULL) {
		list_remove(list, elementToRemove);
	}

	return;
}

/* 
Returns the element containing the given data. Returns NULL if no elements
in the list contain the given data.
*/
ListElement * list_search(List *list, void * data) {
	ListElement *currElement = list->head;

	while (currElement != NULL)	{
		if (currElement->data == data) {
			return currElement;
		}
		currElement = currElement->next;
	}

	return NULL;
}

/*
Removes all elements from the given list and calls the function 
passed as destroy on the data in each element in the list.
*/
void list_destroy(List *list) {
	// Remove each element
	while (list_size(list) > 0) {
		void *data = list_remove(list, NULL);
		if (data != NULL && list->destroy != NULL) {
			// Call the assigned function to free the dynamically allocated data
			list->destroy(data);
		}
	}

	// Free the list structure itself
	free(list);
}



