/*
* list.c
*/

typedef struct ListElement_ {
	void *data;
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

/*
list_init must be called before a List can be used.
Pass a pointer to a function that will clean up dynamically allocated
memory used by the data contained in list elements. 
e.g. for malloc, send a pointer to free. For data that shouldn't be cleaned
up, send NULL.
*/
void list_init(List *list, void (*destroy)(void* data)) {
	list->size = 0;
	list->destroy = destroy;
	list->head = NULL;
	list->tail = NULL; 
}

/*
Inserts an element into the list after the given element.
The new element contains a pointer to data, so the memory referenced
by data should remain valid for the life of the element.
Returns true on success, false on failure.
*/
bool list_insert_after(List *list, ListElement *element, void **data) {
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
		list->head = newElement;
	} else {
		if (element->next == NULL) {
			list->tail = newElement;
		}

		newElement->next = element->next;
		element->next = newElement;
	}

	list->size += 1;
	return true;
}

/*
Removes the element after the given element. If element is NULL, removes
the head of the list. On return, data points to the data stored in the
element.
Returns true on success, false on failure.
*/
bool list_remove_after(List *list, ListElement *element, void **data) {
	ListElement *oldElement;

	if (list_size(list) == 0) {
		return false;
	}

	if (element == NULL) {
		*data = list->head->data;
		oldElement = list->head;
		list->head = list->head->next;
	} else {
		if (element->next == NULL) {
			return false;
		}

		*data = element->next->data;
		oldElement = element->next;
		element->next = element->next->next;

		if (element->next == NULL) {
			list->tail = element;
		}
	}

	free(oldElement);

	list->size -= 1;

	return true;
}

/*
Removes all elements from the given list and calls the function 
passed as destroy on the data in each element in the list.
*/
void list_destroy(List *list) {
	void *data;
	// Remove each element
	while (list_size(list) > 0) {
		if (list_remove_after(list, NULL, (void **)data) && list->destroy != NULL) {
			// Call the assigned function to free the dynamically allocated data
			list->destroy(data);
		}
	}

	// Clear the structure
	memset(list, 0, sizeof(List));
}



