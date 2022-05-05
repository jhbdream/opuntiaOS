/*
 * Copyright (C) 2020-2022 The opuntiaOS Project Authors.
 *  + Contributed by Nikita Melekhin <nimelehin@gmail.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <libkern/ctype.h>
#include <libkern/libkern.h>

const char __kern_ctypes[256] = {
    _C, _C, _C, _C, _C, _C, _C, _C,
    _C, _C | _S, _C | _S, _C | _S, _C | _S, _C | _S, _C, _C,
    _C, _C, _C, _C, _C, _C, _C, _C,
    _C, _C, _C, _C, _C, _C, _C, _C,
    (char)(_S | _B), _P, _P, _P, _P, _P, _P, _P,
    _P, _P, _P, _P, _P, _P, _P, _P,
    _N, _N, _N, _N, _N, _N, _N, _N,
    _N, _N, _P, _P, _P, _P, _P, _P,
    _P, _U | _X, _U | _X, _U | _X, _U | _X, _U | _X, _U | _X, _U,
    _U, _U, _U, _U, _U, _U, _U, _U,
    _U, _U, _U, _U, _U, _U, _U, _U,
    _U, _U, _U, _P, _P, _P, _P, _P,
    _P, _L | _X, _L | _X, _L | _X, _L | _X, _L | _X, _L | _X, _L,
    _L, _L, _L, _L, _L, _L, _L, _L,
    _L, _L, _L, _L, _L, _L, _L, _L,
    _L, _L, _L, _P, _P, _P, _P, _C
};

int stoi(void* strv, int len)
{
    char* str = (char*)strv;
    if (len > 9) {
        return 0;
    }
    int res = 0;
    int mult = 1;
    char* end = str + len - 1;
    while (end >= str) {
        res += (*end - '0') * mult;
        mult *= 10;
        end--;
    }
    return res;
}

void htos(uintptr_t hex, char str[])
{
    int i = 0;
    if (hex == 0) {
        str[0] = '0';
        i = 1;
    }
    while (hex != 0) {
        int num = hex % 16;
        hex /= 16;
        if (num < 10) {
            str[i++] = num + '0';
        } else {
            str[i++] = num + 'a' - 10;
        }
    }
    str[i] = '\0';
    reverse(str);
}

void dtos(uintptr_t dec, char str[])
{
    int i = 0;
    if (dec == 0) {
        str[0] = '0';
        i = 1;
    }
    while (dec != 0) {
        int num = dec % 10;
        dec /= 10;
        str[i++] = num + '0';
    }
    str[i] = '\0';
    reverse(str);
}

void reverse(char s[])
{
    int c, i, j;
    for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

size_t strlen(const char* s)
{
    size_t i = 0;
    while (s[i] != '\0')
        ++i;
    return i;
}

int strcmp(const char* a, const char* b)
{
    while (*a == *b && *a != 0 && *b != 0) {
        a++;
        b++;
    }

    if (*a < *b) {
        return -1;
    }
    if (*a > *b) {
        return 1;
    }
    return 0;
}

int strncmp(const char* a, const char* b, size_t num)
{
    while (*a == *b && *a != 0 && *b != 0 && num) {
        a++;
        b++;
        num--;
    }

    if (!num) {
        return 0;
    }

    if (*a < *b) {
        return -1;
    }
    if (*a > *b) {
        return 1;
    }
    return 0;
}

size_t ptrarray_len(const void** s)
{
    size_t len = 0;
    while (s[len] != 0) {
        ++len;
    }
    return len;
}
