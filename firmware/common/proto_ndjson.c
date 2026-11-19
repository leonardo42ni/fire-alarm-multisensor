#include "proto_ndjson.h"
#include "proto_ndjson_internal.h"

int proto_hex_val(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int proto_ieee_ok(const char *ieee)
{
    size_t i;

    if (ieee == NULL) return 0;
    for (i = 0; i < PROTO_IEEE_LEN - 1; i++) {
        if (proto_hex_val(ieee[i]) < 0) return 0;
    }
    return ieee[PROTO_IEEE_LEN - 1] == '\0';
}

void line_asm_init(line_asm_t *la)
{
    if (la == NULL) return;
    la->len = 0;
    la->overflow = false;
    la->n_lines = 0;
    la->n_overflow = 0;
}

size_t line_asm_feed(line_asm_t *la, const uint8_t *d, size_t n, line_cb_t cb, void *ctx)
{
    size_t i, emitted = 0;

    if (la == NULL || d == NULL) return 0;
    for (i = 0; i < n; i++) {
        char c = (char)d[i];
        if (c == '\n') {
            if (la->overflow) {
                la->overflow = false;
                la->len = 0;
                continue;
            }
            if (la->len > 0 && la->buf[la->len - 1] == '\r') la->len--;
            if (la->len > 0 && cb != NULL) {
                la->buf[la->len] = '\0';
                cb(la->buf, la->len, ctx);
                emitted++;
                la->n_lines++;
            }
            la->len = 0;
            continue;
        }
        if (la->overflow) continue;
        if (c == '\r' && la->len == PROTO_MAX_LINE - 1) continue; /* CR cuoi khong tinh vao do dai */
        if (la->len < PROTO_MAX_LINE - 1) {
            la->buf[la->len++] = c;
        } else {
            la->overflow = true;
            la->len = 0;
            la->n_overflow++;
        }
    }
    return emitted;
}
