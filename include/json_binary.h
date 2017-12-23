#include <inttypes.h>

uint32_t swap_uint32(uint32_t v);
int32_t swap_int32(int32_t v);
int64_t swap_int64(int64_t v);
uint64_t swap_uint64(uint64_t v);
int little_endian();

int ubjson_write_object_begin(FILE *ptr);
int ubjson_write_object_end(FILE *ptr);
int ubjson_write_array_begin(FILE *ptr);
int ubjson_write_array_end(FILE *ptr);
int ubjson_write_null(FILE *ptr);
int ubjson_write_bool(unsigned char b, FILE *ptr);
int ubjson_write_int8(int8_t v, FILE *ptr);
int ubjson_write_int32(int32_t v, FILE *ptr);
int ubjson_write_int64(int64_t v, FILE *ptr);
int ubjson_write_string(const char *str, size_t n_char, FILE *ptr, int use_prefix);
