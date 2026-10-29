#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    (void)challenge;
    // TODO: 根据 phase_01 的汇编，用 challenge 算出口令中的各个整数，依次存入 values，并把 count 改为整数个数。
    uint32_t lo = (uint32_t)challenge;
    uint32_t hi = (uint32_t)(challenge >> 32);
    uint32_t mid = (uint32_t)(challenge >> 16);
    uint32_t b = (lo * UINT32_C(5)) ^ (hi + UINT32_C(0x55b8));
    uint32_t a = (hi - mid) * UINT32_C(5) + UINT32_C(0xf9a6);
    values[0] = (a + UINT32_C(0xd953)) ^ b;
    count = 1;
    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    (void)challenge;
    // TODO: 根据 phase_02 的汇编，用 challenge 算出口令中的各个整数，依次存入 values，并把 count 改为整数个数。
    values[0] = UINT32_C(3) * (uint32_t)((challenge >> 35) & UINT64_C(0xff));
    values[1] = UINT32_C(5) * (values[0] + 1u) - (uint32_t)((challenge >> 15) & UINT64_C(0x3f));
    values[2] = UINT32_C(5) * (values[1] + 2u) - (uint32_t)((challenge >> 17) & UINT64_C(0x3f));
    values[3] = UINT32_C(5) * (values[2] + 3u) - (uint32_t)((challenge >> 19) & UINT64_C(0x3f));
    values[4] = UINT32_C(5) * (values[3] + 4u) - (uint32_t)((challenge >> 21) & UINT64_C(0x3f));
    values[5] = UINT32_C(5) * (values[4] + 5u) - (uint32_t)((challenge >> 23) & UINT64_C(0x3f));
    count = 6;
    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;

    uint32_t x = (uint32_t)((challenge >> 3) ^ (challenge >> 32));
    values[0] = (uint32_t)challenge & UINT32_C(7);
    switch (values[0]) {
        case 0: values[1] = (UINT32_C(4) * x) ^ UINT32_C(0xc0df); break;
        case 1: values[1] = (UINT32_C(5) * x) + UINT32_C(0xf797); break;
        case 2: values[1] = (UINT32_C(5) * x) + UINT32_C(0x25896); break;
        case 3: values[1] = (x ^ UINT32_C(0xa654)) + (x >> 2); break;
        case 4: values[1] = (UINT32_C(5) * x) + UINT32_C(0x36ed); break;
        case 5: values[1] = (x ^ UINT32_C(0xd1a0)) + (x >> 1); break;
        case 6: values[1] = (UINT32_C(4) * x) ^ UINT32_C(0xe128); break;
        case 7: values[1] = (UINT32_C(5) * x) + UINT32_C(0x3af2); break;
    }
    count = 2;

    return write_answer(out, cap, values, count);
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;

    uint32_t target = (uint32_t)challenge & UINT32_C(0x7f);
    uint32_t salt = (uint32_t)((challenge >> 7) ^ (challenge >> 32));
    uint32_t low = 0, high = 127, mid, result;
    uint32_t path_mid[8], path_tag[8];
    size_t depth = 0;
    while (1) {
        mid = low + ((high - low) >> 1);
        if (target == mid) {
            result = (mid + UINT32_C(0x1a)) ^ salt;
            break;
        }
        path_mid[depth] = mid;
        if (target < mid) {
            path_tag[depth] = UINT32_C(0x1a);
            high = mid - 1;
        } else {
            path_tag[depth] = UINT32_C(0x1f);
            low = mid + 1;
        }
        depth++;
    }
    while (depth > 0) {
        depth--;
        result = (result * UINT32_C(5)) ^ (path_mid[depth] + salt);
        result ^= path_tag[depth];
    }
    values[0] = result;
    count = 1;

    return write_answer(out, cap, values, count);
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;

    static const uint32_t node_value[12] = {UINT32_C(0x482d), UINT32_C(0x546c), UINT32_C(0x6932), UINT32_C(0xa462), UINT32_C(0xba73), UINT32_C(0x7948), UINT32_C(0x03b3), UINT32_C(0x9d81), UINT32_C(0x3b9d), UINT32_C(0x869f), UINT32_C(0xaa1d), UINT32_C(0x5ac8)};
    static const uint32_t node_tag[12] = {UINT32_C(0x0818), UINT32_C(0x0075), UINT32_C(0x0b5e), UINT32_C(0x0558), UINT32_C(0x0465), UINT32_C(0x0737), UINT32_C(0x090a), UINT32_C(0x021f), UINT32_C(0x0650), UINT32_C(0x0327), UINT32_C(0x0103), UINT32_C(0x0a37)};
    static const int next_node[12] = {2, 8, 10, 1, 0, 3, 9, 5, -1, 4, 7, 6};
    static const int roots[8] = {11, 6, 9, 4, 0, 2, 10, 7};

    uint32_t x = (uint32_t)((challenge >> 6) ^ (challenge >> 32));
    uint32_t acc = x ^ UINT32_C(0x57b);
    int current = roots[(uint32_t)challenge & UINT32_C(7)];
    uint32_t steps = ((uint32_t)(challenge >> 3) & UINT32_C(3)) + UINT32_C(1);
    for (uint32_t i = 0; i < steps; i++) {
        uint32_t tag = node_tag[current];
        acc = ((acc + node_value[current]) * UINT32_C(5)) ^ tag;
        values[1] = tag;
        current = next_node[current];
    }
    values[0] = acc;
    count = 2;

    return write_answer(out, cap, values, count);
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;

    uint32_t x = (uint32_t)((challenge >> 6) ^ (challenge >> 32));
    uint32_t value;
    switch ((uint32_t)challenge & UINT32_C(7)) {
        case 0: value = (x + UINT32_C(0x1f14)) ^ (x >> 2); break;
        case 1: value = (x >> 2) | (x << 30); break;
        case 2: value = ((x << 7) | (x >> 25)) ^ UINT32_C(0xa4a2); break;
        case 3: value = (x ^ UINT32_C(0x844f)) + (x << 7); break;
        case 4: value = (x + UINT32_C(0x1f14)) ^ (x >> 2); break;
        case 5: value = ((x >> 5) ^ x) + UINT32_C(0x3dd4); break;
        case 6: value = (x + UINT32_C(0x1f14)) ^ (x >> 2); break;
        case 7: value = (x * UINT32_C(8) + UINT32_C(0x1af9)) ^ (x * UINT32_C(5)); break;
    }
    value ^= UINT32_C(0xcf99);
    switch (((uint32_t)(challenge >> 3)) & UINT32_C(7)) {
        case 0: value = (value + UINT32_C(0x1f14)) ^ (value >> 2); break;
        case 1: value = (value >> 2) | (value << 30); break;
        case 2: value = ((value << 7) | (value >> 25)) ^ UINT32_C(0xa4a2); break;
        case 3: value = (value ^ UINT32_C(0x844f)) + (value << 7); break;
        case 4: value = (value + UINT32_C(0x1f14)) ^ (value >> 2); break;
        case 5: value = ((value >> 5) ^ value) + UINT32_C(0x3dd4); break;
        case 6: value = (value + UINT32_C(0x1f14)) ^ (value >> 2); break;
        case 7: value = (value * UINT32_C(8) + UINT32_C(0x1af9)) ^ (value * UINT32_C(5)); break;
    }
    value += UINT32_C(0x92f9);
    values[0] = value;
    count = 1;

    return write_answer(out, cap, values, count);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    if (cap <= 32) return 1;
    uint32_t c = (uint32_t)context & UINT32_C(3);
    uint32_t A = (uint32_t)((challenge >> 32) ^ challenge);
    uint32_t B = (uint32_t)((context >> 32) ^ context);
    uint32_t t = A ^ B;
    uint32_t m = ((t << 5) | (t >> 27)) + UINT32_C(0x8410);
    m ^= UINT32_C(0xfbf3);
    uint32_t hi16 = (uint32_t)(context >> 16);
    uint32_t n1 = m ^ ((c * UINT32_C(0x8410)) ^ hi16);
    uint32_t n2 = n1 ^ (B + c + UINT32_C(0xfbf3) + (A >> 5));
    for (size_t i = 0; i < 32; i++) out[i] = 'A';
    out[32] = '\0';
    int size = snprintf(out + 32, cap - 32, "OPEN %u %u %u", c, n1, n2);
    return size < 0 || (size_t)size >= cap - 32;
}
