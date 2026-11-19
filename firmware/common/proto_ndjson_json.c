#include "proto_ndjson.h"
#include "proto_ndjson_internal.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static int is_ws(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void skip_ws(const char *s, size_t n, size_t *i)
{
    while (*i < n && is_ws(s[*i])) {
        (*i)++;
    }
}

static int scan_string(const char *s, size_t n, size_t *i)
{
    size_t k = *i;
    if (k >= n || s[k] != '"') return 0;
    k++;
    while (k < n) {
        unsigned char c = (unsigned char)s[k];
        if (c == '"') {
            *i = k + 1;
            return 1;
        }
        if (c < 0x20) return 0;
        if (c == '\\') {
            if (k + 1 >= n) return 0;
            if (s[k + 1] == 'u') {
                int j;
                if (k + 5 >= n) return 0;
                /* JSON \uXXXX: backslash + 'u' + exactly 4 hex digits. */
                for (j = 2; j < 6; j++) {
                    if (proto_hex_val(s[k + (size_t)j]) < 0) return 0;
                }
                k += 6;
            } else if (s[k + 1] != '\0' && strchr("\"\\/bfnrt", s[k + 1]) != NULL) {
                k += 2;
            } else {
                return 0;
            }
            continue;
        }
        k++;
    }
    return 0;
}

static int scan_number(const char *s, size_t n, size_t *i)
{
    size_t k = *i;
    if (k < n && s[k] == '-') k++;
    if (k >= n) return 0;
    if (s[k] == '0') {
        k++;
    } else if (s[k] >= '1' && s[k] <= '9') {
        while (k < n && s[k] >= '0' && s[k] <= '9') k++;
    } else {
        return 0;
    }
    if (k < n && s[k] == '.') {
        k++;
        if (k >= n || s[k] < '0' || s[k] > '9') return 0;
        while (k < n && s[k] >= '0' && s[k] <= '9') k++;
    }
    if (k < n && (s[k] == 'e' || s[k] == 'E')) {
        k++;
        if (k < n && (s[k] == '+' || s[k] == '-')) k++;
        if (k >= n || s[k] < '0' || s[k] > '9') return 0;
        while (k < n && s[k] >= '0' && s[k] <= '9') k++;
    }
    *i = k;
    return 1;
}

static int scan_literal(const char *s, size_t n, size_t *i, const char *lit)
{
    size_t len = strlen(lit);
    if (*i + len > n || memcmp(s + *i, lit, len) != 0) return 0;
    *i += len;
    return 1;
}

static int skip_value(const char *s, size_t n, size_t *i, int depth)
{
    char c, close;
    int object;

    if (depth > PROTO_JSON_MAX_DEPTH) return 0;
    skip_ws(s, n, i);
    if (*i >= n) return 0;
    c = s[*i];
    if (c == '"') return scan_string(s, n, i);
    if (c != '{' && c != '[') {
        if (c == 't') return scan_literal(s, n, i, "true");
        if (c == 'f') return scan_literal(s, n, i, "false");
        if (c == 'n') return scan_literal(s, n, i, "null");
        return scan_number(s, n, i);
    }
    close = (c == '{') ? '}' : ']';
    object = (c == '{');
    (*i)++;
    skip_ws(s, n, i);
    if (*i < n && s[*i] == close) {
        (*i)++;
        return 1;
    }
    for (;;) {
        if (object) {
            skip_ws(s, n, i);
            if (!scan_string(s, n, i)) return 0;
            skip_ws(s, n, i);
            if (*i >= n || s[*i] != ':') return 0;
            (*i)++;
        }
        if (!skip_value(s, n, i, depth + 1)) return 0;
        skip_ws(s, n, i);
        if (*i >= n) return 0;
        if (s[*i] == ',') {
            (*i)++;
            continue;
        }
        if (s[*i] == close) {
            (*i)++;
            return 1;
        }
        return 0;
    }
}

int proto_json_valid_object(const char *s, size_t n)
{
    size_t i = 0;

    skip_ws(s, n, &i);
    if (i >= n || s[i] != '{') return 0;
    i++;
    skip_ws(s, n, &i);
    if (i < n && s[i] == '}') {
        i++;
        skip_ws(s, n, &i);
        return i == n;
    }
    for (;;) {
        skip_ws(s, n, &i);
        if (!scan_string(s, n, &i)) return 0;
        skip_ws(s, n, &i);
        if (i >= n || s[i] != ':') return 0;
        i++;
        if (!skip_value(s, n, &i, 1)) return 0;
        skip_ws(s, n, &i);
        if (i >= n) return 0;
        if (s[i] == ',') {
            i++;
            continue;
        }
        if (s[i] == '}') {
            i++;
            break;
        }
        return 0;
    }
    skip_ws(s, n, &i);
    return i == n;
}

int flat_find(const char *s, size_t n, const char *key, const char **vs, size_t *vl)
{
    size_t i = 0;
    size_t klen;

    if (s == NULL || key == NULL || vs == NULL || vl == NULL) return 0;
    klen = strlen(key);
    skip_ws(s, n, &i);
    if (i >= n || s[i] != '{') return 0;
    i++;
    for (;;) {
        size_t ks, ke, vs_i, ve;
        skip_ws(s, n, &i);
        if (i >= n) return 0;
        if (s[i] == '}') return 0;
        if (s[i] != '"') return 0;
        ks = i + 1;
        if (!scan_string(s, n, &i)) return 0;
        ke = i - 1;
        skip_ws(s, n, &i);
        if (i >= n || s[i] != ':') return 0;
        i++;
        vs_i = i;
        if (!skip_value(s, n, &i, 1)) return 0;
        ve = i;
        skip_ws(s, n, &i);
        if (i >= n || (s[i] != ',' && s[i] != '}')) return 0;
        if ((ke - ks) == klen && memcmp(s + ks, key, klen) == 0) {
            size_t a = vs_i, b = ve;
            while (a < b && is_ws(s[a])) a++;
            while (b > a && is_ws(s[b - 1])) b--;
            *vs = s + a;
            *vl = b - a;
            return 1;
        }
        if (s[i] == ',') {
            i++;
            continue;
        }
        return 0;
    }
}

int flat_get_num(const char *s, size_t n, const char *key, double *out)
{
    const char *vs = NULL;
    size_t vl = 0;
    char tmp[PROTO_NUM_BUF_LEN];
    char *end = NULL;
    double d;

    if (out == NULL || !flat_find(s, n, key, &vs, &vl)) return 0;
    if (vl == 0 || vl >= sizeof(tmp)) return 0;
    if (!(vs[0] == '-' || (vs[0] >= '0' && vs[0] <= '9'))) return 0;
    memcpy(tmp, vs, vl);
    tmp[vl] = '\0';
    d = strtod(tmp, &end);
    if (end != tmp + vl || !isfinite(d)) return 0;
    *out = d;
    return 1;
}

int flat_get_str(const char *s, size_t n, const char *key, char *out, size_t cap)
{
    const char *vs = NULL;
    size_t vl = 0, len, i;

    if (out == NULL || cap == 0 || !flat_find(s, n, key, &vs, &vl)) return 0;
    if (vl < 2 || vs[0] != '"' || vs[vl - 1] != '"') return 0;
    len = vl - 2;
    if (len >= cap) return 0;
    for (i = 0; i < len; i++) {
        if (vs[1 + i] == '\\') return 0;
    }
    memcpy(out, vs + 1, len);
    out[len] = '\0';
    return 1;
}
