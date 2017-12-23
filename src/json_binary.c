#include <stdio.h>
#include "json_binary.h"

uint32_t swap_uint32(uint32_t v) {
  v = ((v << 8) & 0xff00ff00 ) | ((v >> 8) & 0xff00ff ); 
  return (v << 16) | (v >> 16);
}

int32_t swap_int32(int32_t v) {
  v = ((v << 8) & 0xff00ff00) 
    | ((v >> 8) & 0xff00ff); 
  return (v << 16) | ((v >> 16) & 0xffff);
}

int64_t swap_int64(int64_t v) {
  v = ((v << 8) & 0xff00ff00ff00ff00ULL) 
    | ((v >> 8) & 0x00ff00ff00ff00ffULL);
  v = ((v << 16) & 0xffff0000ffff0000ULL) 
    | ((v >> 16) & 0x0000ffff0000ffffULL);
  return (v << 32) | ((v >> 32) & 0xffffffffULL);
}

uint64_t swap_uint64(uint64_t v) {
  v = ((v << 8) & 0xff00ff00ff00ff00ULL) 
    | ((v >> 8) & 0x00ff00ff00ff00ffULL);
  v = ((v << 16) & 0xffff0000ffff0000ULL) 
    | ((v >> 16) & 0x0000ffff0000ffffULL);
  return (v << 32) | (v >> 32);
}

int little_endian() {
  int n = 1;
  if(*(char *)&n == 1) return 1;
  return 0;
}

int ubjson_write_object_begin(FILE *ptr){
  fwrite("{", sizeof(char),1,ptr);
}

int ubjson_write_object_end(FILE *ptr) {
  fwrite("}", sizeof(char),1,ptr);
}

int ubjson_write_array_begin(FILE *ptr){
  fwrite("[", sizeof(char),1,ptr);
}

int ubjson_write_array_end(FILE *ptr) {
  fwrite("]", sizeof(char),1,ptr);
}

int ubjson_write_null(FILE *ptr) {
  fwrite("Z", sizeof(char),1,ptr);
}

int ubjson_write_bool(unsigned char b, FILE *ptr) {
  static const char true_char = 'T';
  static const char false_char = 'F';
  if(b) fwrite(&true_char, sizeof(char), 1, ptr);
  else fwrite(&false_char, sizeof(char), 1, ptr);
  return 0;
}

int ubjson_write_int8(int8_t v, FILE *ptr) {
  static const char type_char = 'i';
  fwrite(&type_char, sizeof(char),1,ptr);
  fwrite(&v, sizeof(int8_t),1,ptr);
  return 0;
}

int ubjson_write_int32(int32_t v, FILE *ptr) {
  static const char type_char = 'I';
  int is_little_endian = little_endian();
  fwrite(&type_char, sizeof(char),1,ptr);

  if(is_little_endian) v = swap_int32(v);
  fwrite(&v,sizeof(int32_t),1,ptr);
  return 0;
}

int ubjson_write_int64(int64_t v, FILE *ptr) {
  static const char type_char = 'L';
  int is_little_endian = little_endian();
  fwrite(&type_char, sizeof(char),1,ptr);

  if(is_little_endian) v = swap_int64(v);
  fwrite(&v,sizeof(int64_t),1,ptr);  
  return 0;
}

int ubjson_write_string(const char *str, size_t n_char, FILE *ptr, int prefix) {
  static const char type_char = 'S';
  if(prefix) fwrite(&type_char, sizeof(char),1,ptr);
  if(n_char <= INT8_MAX) ubjson_write_int8(n_char, ptr);
  else if(n_char <= INT32_MAX) ubjson_write_int32(n_char, ptr);
  else if(n_char <= INT64_MAX) ubjson_write_int64(n_char, ptr);
  else {
    fprintf(stderr,"Error: string is too long to be written in ubjson\n");
    return 1;
  }
  fwrite(str, sizeof(char), n_char, ptr);
  return 0;
}
