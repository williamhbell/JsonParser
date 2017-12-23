void char_buffer_clear(struct char_buffer *buffer); /* Zero all variables. */
void char_buffer_free(struct char_buffer *buffer); /* Zero variables and free allocated memory. */

/* Append a character to the buffer, increasing the size of the buffer
   if necessary. */
int char_buffer_append(struct char_buffer *buffer, char c);

