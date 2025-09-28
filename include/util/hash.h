#ifndef UTIL_HASH_H
#define UTIL_HASH_H

#include <stddef.h>

void hash_sha256_hex(const unsigned char *input, size_t input_len, char *output_hex, size_t output_len);

#endif
