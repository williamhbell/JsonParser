#ifndef JSON_H
#define JSON_H

#include <stdio.h>

/* Definitions of the different json value types. */
#define JSON_NDEF 0
#define JSON_OBJECT 1
#define JSON_ARRAY 2
#define JSON_PAIR 3
#define JSON_STRING 4
#define JSON_INT 5
#define JSON_FLOAT 6
#define JSON_BOOLEAN 7
#define JSON_NULL 8

/* A data struct to contain json data values.  The struct is used to
   store the json file as a tree, where the nodes are object, arrays
   or pairs. */
struct json_value {
  
  /* object, array, pair, string, int, float, boolean (int) */ 
  unsigned int json_type;

  /* Either null or an array of children. */  
  struct json_value **children;
  unsigned int nchildren;
  
  /* Either null or the address of a parent */
  struct json_value *parent;

  /* The value, stored as a union to reduce memory usage. */
  union {
    int b_value;
    long l_value;
    double d_value;
    char *str_value;
  } value;
};

/* A data structure to contain an array of pointers and the associated
   size of the pointer array. */
struct json_data {
  struct json_value **json_values;
  unsigned int n_json_values;
};

struct char_buffer {
  unsigned long blocks; /* Number of memory blocks allocated */
  unsigned long position; /* Current position within memory */
  unsigned long size; /* The size of the memory buffer array (n-char)*/
  char *buffer; /* A dynamically allocated character buffer */
};


/* A function to clear a json_value struct */
void json_clear_value(struct json_value *);

/* Returns a string representation of the given json_type. */
const char* json_type_to_string(const unsigned int);

/* Search for a child index in the given parent index.  The arguments
   are (child, parent) */
long json_find_child_index(const struct json_value *, const struct json_value *);

/* Check to see if this is the last child in a branch of the tree of
   json_value instances.  The arguments are (child, parent).*/
int json_is_last_child(const struct json_value *, const struct json_value *);

/* Print the json_value instance. */
void json_print_value(const struct json_value *);

/* Print the tree, starting from the json_value. */
void json_print_tree(const struct json_value *);

/* Print the ASCII formatted version of the json_value tree.  The
   arguments are the (top node, the initial indent) */
void json_print_formatted(const struct json_value *, long);

/* Check the type of the json_value.  The arguments are (json_value,
   json_type) */
int json_check_type(const struct json_value *, const unsigned int);

/* A function to read the character buffer that contains json.  The
   arguments are (pointer to buffer, number of characters in
   buffer). The function returns an array of json_values, where the
   first element is the top-level node in the tree.  The function
   returns the number of json_values read. */
size_t json_read_ascii_buffer(struct json_data *, const struct char_buffer *);


size_t json_write_file(const char *filename, const char *mode, const struct json_data *);
size_t json_write(FILE *, const char *mode, const struct json_data *);
size_t json_write_tree(FILE *, const char *mode, const struct json_value *json_value, long indent);
void json_print(const struct json_data *data);

/* Free all of the dynamically allocated memory associated with the
   json_value tree.  The argument is an array of json values. */
void json_free_value_array(struct json_data *);

#endif
