#include "util/hash.h"
#include <string.h>
#include <stdint.h>

typedef struct {
    uint32_t state[8];
    uint64_t bitlen;
    unsigned char data[64];
    size_t datalen;
} Sha256Context;

static uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

static uint32_t ep0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

static uint32_t ep1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

static uint32_t sig0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

static uint32_t sig1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

static const uint32_t k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void sha256_transform(Sha256Context *ctx, const unsigned char data[]) {
    uint32_t m[64];
    uint32_t a, b, c, d, e, f, g, h;
    int i = 0;
    while (i < 16) {
        m[i] = (uint32_t)data[i * 4] << 24;
        m[i] |= (uint32_t)data[i * 4 + 1] << 16;
        m[i] |= (uint32_t)data[i * 4 + 2] << 8;
        m[i] |= (uint32_t)data[i * 4 + 3];
        i = i + 1;
    }
    while (i < 64) {
        m[i] = sig1(m[i - 2]) + m[i - 7] + sig0(m[i - 15]) + m[i - 16];
        i = i + 1;
    }
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];
    i = 0;
    while (i < 64) {
        uint32_t t1 = h + ep1(e) + ch(e, f, g) + k[i] + m[i];
        uint32_t t2 = ep0(a) + maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
        i = i + 1;
    }
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static void sha256_init(Sha256Context *ctx) {
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(Sha256Context *ctx, const unsigned char *data, size_t len) {
    size_t i = 0;
    while (i < len) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen = ctx->datalen + 1;
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
        i = i + 1;
    }
}

static void sha256_final(Sha256Context *ctx, unsigned char hash[32]) {
    size_t i = ctx->datalen;
    if (ctx->datalen < 56) {
        ctx->data[i] = 0x80;
        i = i + 1;
        while (i < 56) {
            ctx->data[i] = 0;
            i = i + 1;
        }
    } else {
        ctx->data[i] = 0x80;
        i = i + 1;
        while (i < 64) {
            ctx->data[i] = 0;
            i = i + 1;
        }
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }
    ctx->bitlen += ctx->datalen * 8;
    ctx->data[63] = (unsigned char)(ctx->bitlen);
    ctx->data[62] = (unsigned char)(ctx->bitlen >> 8);
    ctx->data[61] = (unsigned char)(ctx->bitlen >> 16);
    ctx->data[60] = (unsigned char)(ctx->bitlen >> 24);
    ctx->data[59] = (unsigned char)(ctx->bitlen >> 32);
    ctx->data[58] = (unsigned char)(ctx->bitlen >> 40);
    ctx->data[57] = (unsigned char)(ctx->bitlen >> 48);
    ctx->data[56] = (unsigned char)(ctx->bitlen >> 56);
    sha256_transform(ctx, ctx->data);
    i = 0;
    while (i < 4) {
        hash[i] = (unsigned char)((ctx->state[0] >> (24 - i * 8)) & 0xff);
        hash[i + 4] = (unsigned char)((ctx->state[1] >> (24 - i * 8)) & 0xff);
        hash[i + 8] = (unsigned char)((ctx->state[2] >> (24 - i * 8)) & 0xff);
        hash[i + 12] = (unsigned char)((ctx->state[3] >> (24 - i * 8)) & 0xff);
        hash[i + 16] = (unsigned char)((ctx->state[4] >> (24 - i * 8)) & 0xff);
        hash[i + 20] = (unsigned char)((ctx->state[5] >> (24 - i * 8)) & 0xff);
        hash[i + 24] = (unsigned char)((ctx->state[6] >> (24 - i * 8)) & 0xff);
        hash[i + 28] = (unsigned char)((ctx->state[7] >> (24 - i * 8)) & 0xff);
        i = i + 1;
    }
}

void hash_sha256_hex(const unsigned char *input, size_t input_len, char *output_hex, size_t output_len) {
    unsigned char hash[32];
    const char hex_chars[] = "0123456789abcdef";
    size_t i = 0;
    Sha256Context ctx;
    if (output_len < 65) {
        if (output_len > 0) {
            output_hex[0] = '\0';
        }
        return;
    }
    sha256_init(&ctx);
    sha256_update(&ctx, input, input_len);
    sha256_final(&ctx, hash);
    while (i < 32) {
        output_hex[i * 2] = hex_chars[(hash[i] >> 4) & 0x0F];
        output_hex[i * 2 + 1] = hex_chars[hash[i] & 0x0F];
        i = i + 1;
    }
    output_hex[64] = '\0';
}
