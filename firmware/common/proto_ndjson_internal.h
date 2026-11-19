#ifndef PROTO_NDJSON_INTERNAL_H
#define PROTO_NDJSON_INTERNAL_H

#include <stddef.h>

int proto_json_valid_object(const char *s, size_t n);
int proto_hex_val(char c);
int proto_ieee_ok(const char *ieee);

#endif /* PROTO_NDJSON_INTERNAL_H */
