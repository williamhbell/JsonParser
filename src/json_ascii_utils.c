#include <stdio.h>
#include <stdlib.h>
#include "json.h"
#include "json_ascii_utils.h"

#ifdef DEBUG
#define DEBUG_PRINT(x) fprintf(stderr, x)
#else
#define DEBUG_PRINT(x) do {} while (0)
#endif

void char_buffer_clear(struct char_buffer *buffer) {
  buffer->blocks = 0;
  buffer->position = 0;
  buffer->size = 0;
  buffer->buffer = 0;
}


void char_buffer_free(struct char_buffer *buffer) {
  buffer->blocks = 0;
  buffer->position = 0;
  buffer->size = 0;
  if(buffer->buffer) free(buffer->buffer);
  buffer->buffer = 0;
}

/* This buffer size was found to be I/O limited, rather than limited
   by the number of reallocation requests.  One allocation of the
   amount of memory corresponding to a given fixed size large ASCII
   file, was found to use the same amount of CPU time as 6000 requests
   with a 100kByte additional memory request each time. */
#define BUFFER_MEM_BLK_SZ 102400

/* Function to append a character to a character array.  The character
   array grows in large chunks, to reduce the number of reallocation
   calls. */
int char_buffer_append(struct char_buffer *buffer, char c){
  unsigned long new_size = 0;
  char *reallocated_buffer = 0;

  /* If the buffer position is greater than the size of the buffer,
     try to increase the buffer size. */
  if(buffer->position >= buffer->size) {

    /* If the current buffer is too small, then try to make the buffer
       bigger. */
    DEBUG_PRINT("realloc\n");

    /* Increase the memory allocation by one memory block. */
    buffer->blocks++;
    new_size = buffer->blocks * BUFFER_MEM_BLK_SZ;
    reallocated_buffer = (char*)realloc(buffer->buffer, new_size*sizeof(char));
    
    /* If the reallocation fails, then return an error code and the
       original buffer. */
    if(!reallocated_buffer){
      fprintf(stderr, "Error: could not allocate memory for file buffer (1).\n");
      return 1;
    }

    /* If the reallocation is successful, then update the buffer
       pointer. */
    buffer->buffer = reallocated_buffer;

    /* The size of the buffer is now the new size. */
    buffer->size = new_size;
  }


  /* This should not happen, if the allocation is successful. */
  if(buffer->position >= buffer->size) {
    fprintf(stderr, "Error: could not allocate memory for file buffer (2).\n");
    return 2;
  }

  /* Assign the character */
  buffer->buffer[buffer->position] = c;
  buffer->position++; /* Increment the buffer position for next time.*/
  return 0;
}
