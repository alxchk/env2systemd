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

// https://thispointer.com/c-case-insensitive-string-comparison-using-stl-c11-boost-library/

bool compareChar(const char & c1, const char & c2)
{
	if (c1 == c2)
		return true;
	else if (std::toupper(c1) == std::toupper(c2))
		return true;
	return false;
}

/*
 * Case Insensitive String Comparision
 */
bool cmpstr(const std::string & str1, const std::string &str2)
{
	return (
                (str1.size() == str2.size() ) &&
		std::equal(str1.begin(), str1.end(), str2.begin(), &compareChar)
        );
}