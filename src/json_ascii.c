#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "json.h"
#include "json_ascii_utils.h"

#ifdef DEBUG
#define DEBUG_PRINT(x) fprintf(stderr, x)
#else
#define DEBUG_PRINT(x) do {} while (0)
#endif

/* Functions that are used in this file, but are not declared in the header files. */
int test_buffer_size(unsigned int size_of_buffer, unsigned int index_within_buffer);
struct json_value* json_string_value(char *tmp_buffer);
int json_append_value(struct json_data *json, struct json_value *json_value);
struct json_value* json_add_structure(struct json_data *json, int json_type);
int json_add_element(struct json_value *jv_parent, struct json_value *jv);
struct json_value* json_parse_value(struct json_data *json, const char *buffer);

/* Public functions. */
size_t json_write_file(const char *path, const char *mode, const struct json_data *json) {
  FILE *fptr = 0;
  size_t values_written = 0;
  fptr = fopen(path, mode);
  if(!fptr) return 0;
  values_written = json_write(fptr, mode, json);
  fclose(fptr);
  return values_written;
}

void json_print(const struct json_data *json) {
  json_write(stdout,"a",json);
}

size_t json_write(FILE *fptr, const char *mode, const struct json_data *json) {
  unsigned int i = 0;
  size_t values_written = 0;
  if(!json) {
    fprintf(stderr, "Error: json_data pointer is a null.  No data written.\n");
    return 0;
  }
  for(i=0;i<json->n_json_values;i++) {
    if(!(json->json_values[i])) {
      fprintf(stderr, "Warning: json_value[%u] is a null pointer.\n",i);
      continue;
    }

    /* Only write the trees for json_values that do not have a parent. */
    if(!(json->json_values[i]->parent)) {
      values_written += json_write_tree(fptr, mode, json->json_values[i], 0);
    }
  }
  return values_written;
}


size_t json_write_tree(FILE *fptr, const char *mode, const struct json_value *jv, long indent){
  size_t json_values_written = 0;
  long i;
  long indent_next = indent;
  long indent_reduction = 0;
  const struct json_value *jv_parent_tmp = 0;
  const struct json_value *jv_child_tmp = 0;

  int print_indent = 0, print_newline = 0;

  const long indent_step = 2;

  /* The pointer must not be null */
  if(!jv) return 0;

  /* Check if a newline should be printed or not */
  if((jv->json_type == JSON_OBJECT ||
      jv->json_type == JSON_ARRAY))
    indent_next += indent_step;

  /* Check if an indent should be printed or not */
  if(jv->parent) {
    if((jv->parent->json_type == JSON_OBJECT || 
	jv->parent->json_type == JSON_ARRAY)){
      print_indent = 1;
      print_newline = 1;
    }
  }

  if(print_newline) fprintf(fptr, "\n");

  /* Print the indent if needed. */
  if(print_indent) {
    for(i=0;i<indent;i++) {
      fprintf(fptr, " ");
    }
  }

  /* Print the values */
  if(jv->json_type == JSON_OBJECT) fprintf(fptr, "{");
  else if(jv->json_type == JSON_ARRAY) fprintf(fptr, "[");
  else if(jv->json_type == JSON_PAIR) fprintf(fptr, "\"%s\": ", jv->value.str_value);
  else if(jv->json_type == JSON_STRING) fprintf(fptr, "\"%s\"", jv->value.str_value);
  else if(jv->json_type == JSON_INT) fprintf(fptr, "%ld", jv->value.l_value);
  else if(jv->json_type == JSON_FLOAT) fprintf(fptr, "%lf", jv->value.d_value);
  else if(jv->json_type == JSON_BOOLEAN) {
    if(jv->value.b_value) fprintf(fptr, "true");
    else fprintf(fptr, "false");
  }
  else if(jv->json_type == JSON_NULL) fprintf(fptr, "null");
  else { 
    fprintf(fptr, "Error: json_type=%u is out of range", jv->json_type);
    assert(0);
  }

  /* Print all of the children of this json_value */
  for(i=0;i<jv->nchildren;i++) {
    json_values_written += json_write_tree(fptr, mode, jv->children[i], indent_next);
  }

  /* Handle the trailing characters, if this is the end of an array or
     object.  Only start appending trailing characters when the bottom
     of the tree has been reached. */
  if(jv->nchildren == 0) {
    indent_reduction = 0;
    jv_parent_tmp = jv->parent;
    jv_child_tmp = jv;
    while(jv_parent_tmp && jv_child_tmp) {
      
      if(!json_is_last_child(jv_child_tmp, jv_parent_tmp)) {
	if(jv_parent_tmp->json_type == JSON_OBJECT ||
	   jv_parent_tmp->json_type == JSON_ARRAY) {
	  fprintf(fptr, ",");
	}
	break;
      }
      
      if(jv_parent_tmp->json_type == JSON_OBJECT ||
	 jv_parent_tmp->json_type == JSON_ARRAY) {
	
	fprintf(fptr, "\n");
	/* Using longs, to catch a potential error where the indent
	   becomes negative. */
	for(i=0;i<(indent-indent_reduction);i++) {
	  fprintf(fptr, " ");
	}
	if(jv_parent_tmp->json_type == JSON_OBJECT) {
	  fprintf(fptr, "}");
	}
	else if(jv_parent_tmp->json_type == JSON_ARRAY) {
	  fprintf(fptr, "]");
	}
      }
      
      /* Reduce the indent */
      indent_reduction += indent_step;
      
      /* Navigate back up the tree */
      jv_child_tmp = jv_parent_tmp;
      jv_parent_tmp = jv_parent_tmp->parent;
    }
  }

  /* Print the final newline */
  if(!jv->parent) {
    fprintf(fptr, "\n");
  }

  /* Add one for this json_value */
  json_values_written++;

  return json_values_written;
}

/* A function to parse an ASCII buffer that contains json. */
size_t json_read_ascii_buffer(struct json_data *json, const struct char_buffer *buffer) {
  char* tmp_buffer = 0;
  unsigned int size_of_tmp_buffer = 0;
  unsigned int i = 0;
  unsigned int i_tmp = 0;
  int quote;
  int reading_str;
  
  struct json_value *jv_parent = 0;
  struct json_value *jv = 0;

  /* Create a buffer to collect string components.  In the case where
     the file contains one long string, this will be the same size as
     the file buffer.  The buffer needs to be one character bigger, to
     allow for a string terminator to be added. */
  size_of_tmp_buffer = buffer->size+1;
  tmp_buffer = (char*)malloc(size_of_tmp_buffer*sizeof(char));
  if(!tmp_buffer) {
    printf("Error: could not allocate temporary buffer to parse this file.");
    return 0;
  }
  
  /* Reset the flags. */
  reading_str = 0;
  quote = 0;
  for(i=0;i<buffer->size;i++) {
    
    /* First check to see if a string has been reached.  The
       punctuation that describes a json file can be inside a string.
       Therefore, the string case needs to be dealt with first. */
    quote = 0;
    if(buffer->buffer[i] == '"') {
      quote = 1;
      
      /* Check if this is an escaped " character or not. */
      if((i-1) > 0) {
	if(buffer->buffer[i-1] == '\\') {
	  quote = 0;
	}
      }
    }
    
    /* If this is a string quote, then either close or open the string. */
    if(quote) {
      
      /* If a string is being read, then finish the string. */
      if(reading_str) {
	assert(!test_buffer_size(size_of_tmp_buffer, i_tmp));
	
	/* Add the string terminator */
	tmp_buffer[i_tmp] = '\0';
	
	DEBUG_PRINT("Creating a string: ");
	jv = json_string_value(tmp_buffer);
	if(DEBUG) json_print_value(jv);
	assert(jv);

	/* Add the json_value to the json_data json, since the
	   top-level json value will not have a parent. */
	assert(!json_append_value(json, jv));
	
	/* Set the reading string flag to zero. */
	reading_str = 0;
	
	/* Set the temporary string index to zero. */
	i_tmp = 0;
      }
      else {
	/* Set the reading string flag. */
	reading_str = 1;
      }
      continue; /* Skip this " character */
    }
    
    
    /* If a string is being read */
    if(reading_str) {
      assert(!test_buffer_size(size_of_tmp_buffer, i_tmp));
      
      /* Store this character in the temporary buffer. */
      tmp_buffer[i_tmp] = buffer->buffer[i];
      
      /* Increment the temporary buffer. */
      i_tmp++;
      
      /* This is a string.  Therefore, do not check for json punctuation. */
      continue;
    }

    /* Check for the beginning of an object. */
    if(buffer->buffer[i] == '{'){
      DEBUG_PRINT("Creating an object: ");
      jv = json_add_structure(json, JSON_OBJECT);
      if(DEBUG) json_print_value(jv);
      assert(!json_add_element(jv_parent, jv));
      jv_parent = jv; /* Prepare to collect elements. */
      jv = 0; /* Clear to prevent it being reused. */
    }

    /* Check for the beginning of an array. */
    else if(buffer->buffer[i] == '['){
      DEBUG_PRINT("Creating an array: ");
      jv = json_add_structure(json, JSON_ARRAY);
      if(DEBUG) json_print_value(jv);
      assert(!json_add_element(jv_parent, jv));
      jv_parent = jv; /* Prepare to collect elements. */
      jv = 0; /* Clear to prevent it being reused. */
    }
    
    /* Check for the end of a json value, which can be triggered by:
       the end of an object,
       the end of an array,
       the separator in a pair,
       the end of an element of an array,
       or a white space character or new line */
    else if(buffer->buffer[i] == '}' ||
	    buffer->buffer[i] == ']' ||
	    buffer->buffer[i] == ',' ||
	    buffer->buffer[i] == ':' ||
	    buffer->buffer[i] == ' ' ||
	    buffer->buffer[i] == '\t' ||
	    buffer->buffer[i] == '\n'){
      
      /* If the tmp string has a non-zero length, create a new json
	 value. */
      if(i_tmp > 0) {
	
	/* Check to make sure that the next index is not outside the
	   temporary buffer. */
	assert(!test_buffer_size(size_of_tmp_buffer, i_tmp));
	
	/* Add the string terminator */
	tmp_buffer[i_tmp] = '\0';
	
	/* Parse the string */
	jv = json_parse_value(json, tmp_buffer);
	assert(jv);
	
	/* Set the temporary string index to zero. */
	i_tmp = 0;
      }
     
      /* A colon indicates that this is a pair */
      if(buffer->buffer[i] == ':'){
	DEBUG_PRINT("Creating a pair: ");
	jv->json_type = JSON_PAIR; /* Change the type to a pair. */
	if(DEBUG) json_print_value(jv);
	assert(!json_add_element(jv_parent, jv));
	jv_parent = jv; /* Prepare to collect an element. */
	jv = 0; /* Clear to prevent it being reused. */
      }
      
      /* A comma is used to separate elements within arrays and
	 objects. */
      else if(buffer->buffer[i] == ','){

	/* In the case of an array or an object, the object is added
	   to the parent when it is first opened.  Therefore, the jv
	   value will be null here.  For other values, the jv value
	   should not be null. */
	if(jv) {
	  DEBUG_PRINT("Adding an element to current parent\n");
	  assert(!json_add_element(jv_parent, jv));
	  jv = 0; /* Clear to prevent it being reused. */
	}

	/* If this is an element of a pair, then go back to the
	   parent of the pair. */
	if(jv_parent) {
	  if(jv_parent->json_type == JSON_PAIR) {
	    jv_parent = jv_parent->parent;
	  }
	}
      }
      else if(buffer->buffer[i] == ']' ||
	      buffer->buffer[i] == '}'){

	/* If there is a current json_value that has not been added,
	   then add it to the tree.  When a json array or object is
	   empty, the jv will be null since it will be set to null
	   after the array or object is added. */
	if(jv) {
	  assert(!json_add_element(jv_parent, jv));
	  jv = 0; /* Clear to prevent it being reused. */
	}

	/* If this is an element of a pair, then go back to the
	   parent of the pair. */
	if(jv_parent) {
	  if(jv_parent->json_type == JSON_PAIR) {
	    jv_parent = jv_parent->parent;
	  }
	}
	
	/* A ']' character closes an array */
	if(buffer->buffer[i] == ']'){
	  DEBUG_PRINT("Array completed.  Going back to the parent.\n");
	  assert(!json_check_type(jv_parent, JSON_ARRAY));
	  jv_parent = jv_parent->parent; /* Navigate back up the tree */
	}

	/* A '}' character closes an object */
	else if(buffer->buffer[i] == '}'){
	  DEBUG_PRINT("Object completed. Going back to the parent.\n");
	  assert(!json_check_type(jv_parent, JSON_OBJECT));
	  jv_parent = jv_parent->parent; /* Navigate back up the tree */
	}
      }
      else {
	/* Skip the white space characters */
	continue;
      }
    }
    else {
      assert(!test_buffer_size(size_of_tmp_buffer, i_tmp));
      
      /* Store this character in the temporary buffer. */
      tmp_buffer[i_tmp] = buffer->buffer[i];
      
      /* Increment the temporary buffer. */
      i_tmp++;
    }
  }

  /* Free the memory associated with the temporary buffer */
  free(tmp_buffer);

  /* Return the number of json_values read */
  return json->n_json_values;
}

/*======================================================*/
/* Functions that are not declared in the header files. */

/* A function to check if an index is smaller than another index. */
int test_buffer_size(unsigned int size_of_buffer,
		     unsigned int index_within_buffer) {
  if(index_within_buffer >= size_of_buffer) {
    printf("Error: the index is out of range of the buffer size.\n");
    return 1;
  }
  else {
    return 0;
  }
}

/* A function to remove a single backslash in front of a double
   quote. */
void clean_escaped_quote(char *str) {
  char *pos = strstr(str, "\\\"");
  if(!pos) return;
  memmove(pos, pos+1, strlen(str)-1);
}

/* A function to create a json_value that contains a string. */
struct json_value* json_string_value(char *tmp_buffer){
  struct json_value *jv = 0;
  unsigned int str_len;

  /* Create a pointer to hold the dynamically allocated json_value */
  jv = (struct json_value*)malloc(sizeof (struct json_value));
  if(!jv) {
    printf("Error: could not allocate a json_value.");
    return jv;
  }
  json_clear_value(jv);

  jv->json_type = JSON_STRING;
  str_len = strlen(tmp_buffer); /* The length of the string */

  /* Create a character array that is just big enough to hold the string. */
  jv->value.str_value = (char *)malloc((str_len+1)*sizeof(char));
  if(!jv->value.str_value) {
    printf("Error: could not allocate memory for json_value string.\n");
    return 0;
  }
  
  /* Copy the string value into place. */
  strcpy(jv->value.str_value, tmp_buffer);

  return jv;
}

/* A function to parse a piece of ascii text and create a
   json_value */
struct json_value* json_parse_value(struct json_data *json, const char *buffer){
  int base = 0;  
  struct json_value *jv = 0;

  jv = (struct json_value*)malloc(sizeof(struct json_value));
  if(!jv) {
    printf("Error: could not allocate a json_value\n");
    return 0;
  }
  json_clear_value(jv);

  /* Check if this is a null */
  if(!strcmp(buffer, "null")) {
    jv->json_type = JSON_NULL;
  }

  /* Check if this is true */
  else if(!strcmp(buffer, "true")) {
    jv->value.b_value = 1;
    jv->json_type = JSON_BOOLEAN;
  }

  /* Check if this is false */
  else if(!strcmp(buffer, "false")) {
    jv->value.b_value = 0;
    jv->json_type = JSON_BOOLEAN;
  }

  /* Search for a floating point */
  else if(strstr(buffer, ".")) {
    errno = 0;
    jv->value.d_value = strtod(buffer, 0);
    if(errno != 0) {
      printf("Error: could not convert string to double\n");
      return 0;
    }
    jv->json_type = JSON_FLOAT;
  }

  /* Assume that this must be a long. */
  else {
    base = 10;
    errno = 0;
    jv->value.l_value = strtol(buffer, 0, base);
    if(errno != 0) {
      printf("Error: could not convert string to long\n");
      return 0;
    }
    jv->json_type = JSON_INT;
  }

  /* Add the json_value to the array of all json_values, since the
     top-level json value will not have a parent. */
  assert(!json_append_value(json, jv));

  return jv;
}

int json_add_element(struct json_value *jv_parent, struct json_value *jv) {
  struct json_value **realloc_children = 0;
  unsigned int i;

  if(!jv) {
    fprintf(stderr, "Error: no json element associated with this compound json value.\n");
    return 1;
  }
  if(DEBUG) json_print_value(jv);

  /* This condition will be true for the top-level node in the json file. */
  if(!jv_parent) {
    return 0;
  }

  /* Make sure that a json_value is only added to the tree once. */
  if(jv->parent) {
    fprintf(stderr, "Error: the parent pointer has already been set.\n");
    return 2;
  }

  /* Set the parent pointer. */
  jv->parent = jv_parent;
  DEBUG_PRINT("Setting parent: ");
  if(DEBUG) {
    if(jv->parent) json_print_value(jv->parent);
    else printf("null\n");
  }

  /* Now reallocate space in the array of children pointers. */
  realloc_children = (struct json_value**)realloc(jv_parent->children,
						  (jv_parent->nchildren+1)*sizeof(struct json_value*));

  /* If the reallocation failed. */
  if(!realloc_children) {
    fprintf(stderr, "Error: could not allocate memory to store a pointer to the json element");
    return 3;
  }

  /* Update the children buffer */
  jv_parent->children = realloc_children;

  /* Append this jv to the list of children of the parent. */
  jv_parent->children[jv_parent->nchildren] = jv;

  /* Update the number of children. */
  jv_parent->nchildren++;

  if(DEBUG) {
    printf("=========================\n");
    printf("Adding ");
    json_print_value(jv);
    printf("to ");
    json_print_value(jv_parent);
    printf("----------------------\n");
    json_print_value(jv_parent);
    printf("has children:\n");
    for(i=0;i<jv_parent->nchildren;i++) {
      json_print_value(jv_parent->children[i]);
    }
    printf("=========================\n");
  }

  return 0; /* Success */
}

int json_append_value(struct json_data *json, struct json_value *jv) {
  struct json_value **realloc_all_json_values = 0;
  realloc_all_json_values = (struct json_value**)realloc(json->json_values,
							 ((json->n_json_values)+1)*sizeof(struct json_value*));
  if(!realloc_all_json_values) {
    printf("Error: could not allocate memory for json pointer array.\n");
    assert(realloc_all_json_values);
    return 1;
  }
  json->json_values = realloc_all_json_values;
  json->json_values[json->n_json_values] = jv;
  json->n_json_values++;

  return 0;
}

/* This is common functionality to objects and arrays. */
struct json_value* json_add_structure(struct json_data *json, int json_type) {
  struct json_value *jv = 0;
  jv = (struct json_value*)malloc(sizeof(struct json_value));
  assert(jv);
  json_clear_value(jv);
  jv->json_type = json_type;

  if(json_append_value(json, jv)) return 0;

  return jv;
}

int json_check_parent_type(struct json_value *jv_parent, unsigned int required_type){
  if(!jv_parent) {
    printf("Error: json_value parent is null\n");
    return 1;
  }
  if(jv_parent->json_type != required_type) {
    printf("Error: parent type should be %s\n", json_type_to_string(required_type));
    return 3;
  }
  return 0;
}
