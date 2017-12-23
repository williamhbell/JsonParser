#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "json.h"

/* A function to clear a json_value struct */
void json_clear_value(struct json_value *jv) {
  jv->json_type = JSON_NDEF;
  jv->children = 0;
  jv->nchildren = 0;
  jv->parent = 0;
  jv->value.b_value = 0;
}

const char* json_type_to_string(unsigned int json_type) {
  static char *json_type_str[9] = {
    "NDEF",
    "OBJECT",
    "ARRAY",
    "PAIR",
    "STRING",
    "INT",
    "FLOAT",
    "BOOLEAN",
    "NULL"};
  if(json_type < 9) return json_type_str[json_type];
  return "OUT_OF_RANGE";
}

void json_print_value(const struct json_value *jv) {
  if(!jv) return;
  
  printf("%s, ", json_type_to_string(jv->json_type));
  if(jv->json_type == JSON_STRING) printf("value=\"%s\", ", jv->value.str_value);
  else if(jv->json_type == JSON_PAIR) printf("value=\"%s\", ", jv->value.str_value);
  else if(jv->json_type == JSON_INT) printf("value=%ld, ", jv->value.l_value);
  else if(jv->json_type == JSON_FLOAT) printf("value=%lf, ", jv->value.d_value);
  else if(jv->json_type == JSON_BOOLEAN) {
    if(jv->value.b_value) printf("value=true, ");
    else printf("value=false, ");
  }
  printf("\n");
}

void json_print_tree(const struct json_value *jv) {
  unsigned int i;
  if(!jv) return;
  json_print_value(jv);
  printf("nchildren=%d\n",jv->nchildren);
  for(i=0;i<jv->nchildren;i++) json_print_tree(jv->children[i]);
}

long json_find_child_index(const struct json_value *jv_child, 
			   const struct json_value *jv_parent) {
  long i = 0;
  while(i<jv_parent->nchildren) {
    if(jv_parent->children[i] == jv_child) {
      break;
    }
    else {
      i++;
    }
  }
  if(i == jv_parent->nchildren) i = -1;
  return i;
}

int json_is_last_child(const struct json_value *jv_child, 
		       const struct json_value *jv_parent) {
  long i;
  i = json_find_child_index(jv_child, jv_parent);
  assert(i>=0);
  if(i == (jv_parent->nchildren-1)) return 1;
  return 0;
}

void json_print_formatted(const struct json_value *jv, long indent) {
  long i;
  long indent_next = indent;
  long indent_reduction = 0;
  const struct json_value *jv_parent_tmp = 0;
  const struct json_value *jv_child_tmp = 0;

  int print_indent = 0, print_newline = 0;

  const long indent_step = 2;

  /* The pointer must not be null */
  if(!jv) return;

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

  if(print_newline) printf("\n");

  /* Print the indent if needed. */
  if(print_indent) {
    for(i=0;i<indent;i++) {
      printf(" ");
    }
  }

  /* Print the values */
  if(jv->json_type == JSON_OBJECT) printf("{");
  else if(jv->json_type == JSON_ARRAY) printf("[");
  else if(jv->json_type == JSON_PAIR) printf("\"%s\": ", jv->value.str_value);
  else if(jv->json_type == JSON_STRING) printf("\"%s\"", jv->value.str_value);
  else if(jv->json_type == JSON_INT) printf("%ld", jv->value.l_value);
  else if(jv->json_type == JSON_FLOAT) printf("%lf", jv->value.d_value);
  else if(jv->json_type == JSON_BOOLEAN) {
    if(jv->value.b_value) printf("true");
    else printf("false");
  }
  else if(jv->json_type == JSON_NULL) printf("null");
  else { 
    printf("ERROR");
    assert(0);
  }

  /* Print all of the children of this json_value */
  for(i=0;i<jv->nchildren;i++) {
    json_print_formatted(jv->children[i], indent_next);
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
	  printf(",");
	}
	break;
      }
      
      if(jv_parent_tmp->json_type == JSON_OBJECT ||
	 jv_parent_tmp->json_type == JSON_ARRAY) {
	
	printf("\n");
	/* Using longs, to catch a potential error where the indent
	   becomes negative. */
	for(i=0;i<(indent-indent_reduction);i++) {
	  printf(" ");
	}
	if(jv_parent_tmp->json_type == JSON_OBJECT) {
	  printf("}");
	}
	else if(jv_parent_tmp->json_type == JSON_ARRAY) {
	  printf("]");
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
    printf("\n");
  }
}

int json_check_type(const struct json_value *jv_parent, unsigned int required_type){
  if(!jv_parent) {
    printf("Error: json_value parent is null\n");
    return 1;
  }
  if(jv_parent->json_type !=  required_type) {
    printf("Error: parent type should be %s\n", json_type_to_string(required_type));
    return 3;
  }
  return 0;
}
