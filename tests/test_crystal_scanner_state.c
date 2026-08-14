/*
 * test_crystal_scanner_state.c — Crystal external-scanner state boundary tests.
 *
 * Tree-sitter restores external scanner state from syntax-tree storage. The
 * scanner must accept states produced by serialize() and discard malformed
 * state without retaining partial literals or heredocs.
 */
#include "test_framework.h"

#include <stdint.h>
#include <string.h>

#define CRYSTAL_SCANNER_SERIALIZATION_BUFFER_SIZE 1024

extern void *tree_sitter_crystal_external_scanner_create(void);
extern void tree_sitter_crystal_external_scanner_destroy(void *payload);
extern unsigned tree_sitter_crystal_external_scanner_serialize(void *payload, char *buffer);
extern void tree_sitter_crystal_external_scanner_deserialize(void *payload, const char *buffer,
                                                             unsigned length);

static int assert_empty_state(void *scanner) {
    const char expected[] = {0, 0, 0, 1, 0, 0};
    char serialized[CRYSTAL_SCANNER_SERIALIZATION_BUFFER_SIZE] = {0};
    unsigned length = tree_sitter_crystal_external_scanner_serialize(scanner, serialized);
    ASSERT_EQ(length, sizeof(expected));
    ASSERT(memcmp(serialized, expected, sizeof(expected)) == 0);
    return 0;
}

TEST(crystal_scanner_state_round_trip) {
    const char serialized_state[] = {
        1, 1, 1, 0,       /* booleans */
        1, 2, 3, 4, 5,    /* one PercentLiteral */
        1, 1, 1, 3, 'f', 'o', 'o', /* one heredoc */
    };
    char restored[CRYSTAL_SCANNER_SERIALIZATION_BUFFER_SIZE] = {0};
    void *scanner = tree_sitter_crystal_external_scanner_create();

    tree_sitter_crystal_external_scanner_deserialize(scanner, serialized_state,
                                                     sizeof(serialized_state));
    unsigned length = tree_sitter_crystal_external_scanner_serialize(scanner, restored);

    ASSERT_EQ(length, sizeof(serialized_state));
    ASSERT(memcmp(restored, serialized_state, sizeof(serialized_state)) == 0);
    tree_sitter_crystal_external_scanner_destroy(scanner);
    PASS();
}

TEST(crystal_scanner_state_discards_malformed_input) {
    const char truncated[] = {1};
    const char too_many_literals[] = {1, 1, 1, 0, 17};
    const char too_many_heredocs[] = {1, 1, 1, 0, 0, 17};
    char oversized_identifiers[6 + 3 * (3 + UINT8_MAX)] = {1, 1, 1, 0, 0, 3};
    void *scanner = tree_sitter_crystal_external_scanner_create();

    size_t offset = 6;
    for (size_t i = 0; i < 3; i++) {
        oversized_identifiers[offset++] = 1;
        oversized_identifiers[offset++] = 1;
        oversized_identifiers[offset++] = (char)UINT8_MAX;
        memset(&oversized_identifiers[offset], 'x', UINT8_MAX);
        offset += UINT8_MAX;
    }

    tree_sitter_crystal_external_scanner_deserialize(scanner, truncated, sizeof(truncated));
    ASSERT_EQ(assert_empty_state(scanner), 0);

    tree_sitter_crystal_external_scanner_deserialize(scanner, too_many_literals,
                                                     sizeof(too_many_literals));
    ASSERT_EQ(assert_empty_state(scanner), 0);

    tree_sitter_crystal_external_scanner_deserialize(scanner, too_many_heredocs,
                                                     sizeof(too_many_heredocs));
    ASSERT_EQ(assert_empty_state(scanner), 0);

    tree_sitter_crystal_external_scanner_deserialize(scanner, oversized_identifiers, offset);
    ASSERT_EQ(assert_empty_state(scanner), 0);

    tree_sitter_crystal_external_scanner_destroy(scanner);
    PASS();
}

void suite_crystal_scanner_state(void) {
    RUN_TEST(crystal_scanner_state_round_trip);
    RUN_TEST(crystal_scanner_state_discards_malformed_input);
}
