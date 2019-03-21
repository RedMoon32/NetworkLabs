#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct {
    size_t size;
    size_t count;
    void** array;
} array_list;

typedef array_list* p_array_list;

p_array_list create_array_list(size_t size);
void delete_array_list(p_array_list alist);
size_t expand_array_list(p_array_list alist);
int array_list_add(p_array_list alist, void* item);
int array_list_remove(p_array_list alist, void* item);
int array_list_remove_at(p_array_list alist, int index);
int array_list_iter(p_array_list alist);
int array_list_next(p_array_list alist, int index);
void* array_list_get(p_array_list alist, int index);