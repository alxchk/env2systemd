/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/* Stolen from systemd. LGPL v2, (c) Lennart Poettering  */

#include <string.h>

static inline char hexchar(int x) {
        static const char table[17] = "0123456789abcdef";

        return table[x & 15];
}

static inline char *xescape(const char *s, const char *bad) {
        char *r, *t;
        const char *f;

        /* Escapes all chars in bad, in addition to \ and all special
         * chars, in \xFF style escaping. May be reversed with
         * cunescape. */

        r = (char *)malloc(strlen(s) * 4 + 1);
        if (!r)
                return NULL;

        for (f = s, t = r; *f; f++) {

                if ((*f < ' ') || (*f >= 127) ||
                    (*f == '\\') || strchr(bad, *f)) {
                        *(t++) = '\\';
                        *(t++) = 'x';
                        *(t++) = hexchar(*f >> 4);
                        *(t++) = hexchar(*f);
                } else
                        *(t++) = *f;
        }

        *t = 0;

        return r;
}
