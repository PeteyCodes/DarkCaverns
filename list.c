/*
* list.c
*/

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
list_init must be called before a List can be used.
Pass a pointer to a function that will clean up dynamically allocated
memory used by the data contained in list elements. 
e.g. for malloc, send a pointer to free. For data that shouldn't be cleaned
up, send NULL.
*/
List * list_init(void (*destroy)(void* data)) {
	List *list = malloc(sizeof(List));

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

	if ((newElement = (ListElement *)malloc(sizeof(ListElement))) == NULL) {
		return false;
	}

	newElement->data = (void *)data;

	if (element == NULL) {
		// Insert at head of list
		if (list_size(list) == 0) {
			list->tail = newElement;
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

void * list_item_at(List *list, u32 index) {
	void * data = NULL;
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
		data = element->next->data;
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



