/* ═══════════════════════════════════════════════════════════════════════════
 * test_lexer.c - Unit Tests for ByteLog Lexer
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Comprehensive unit tests for the lexer module.
 * Tests all token types, edge cases, and error conditions.
 *
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "lexer.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

/* ─────────────────────────────────────────────────────────────────────────
 * Test Framework
 * ───────────────────────────────────────────────────────────────────────── */

static int test_count = 0;
static int test_passed = 0;

#define TEST(name) \
    do { \
        test_count++; \
        printf("Test %d: %s ... ", test_count, #name); \
        if (test_##name()) { \
            test_passed++; \
            printf("PASS\n"); \
        } else { \
            printf("FAIL\n"); \
        } \
    } while(0)

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("ASSERTION FAILED: %s\n", #condition); \
            return false; \
        } \
    } while(0)

#define ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            printf("ASSERTION FAILED: %s != %s (got %d, expected %d)\n", \
                   #actual, #expected, (int)(actual), (int)(expected)); \
            return false; \
        } \
    } while(0)

#define ASSERT_STR_EQ(actual, expected) \
    do { \
        if (strcmp((actual), (expected)) != 0) { \
            printf("ASSERTION FAILED: %s != %s (got '%s', expected '%s')\n", \
                   #actual, #expected, (actual), (expected)); \
            return false; \
        } \
    } while(0)

/* ─────────────────────────────────────────────────────────────────────────
 * Helper Functions
 * ───────────────────────────────────────────────────────────────────────── */

static bool tokenize_and_check(const char *source, TokenType expected_types[], 
                               const char *expected_values[], int expected_count) {
    Lexer lexer;
    lexer_init(&lexer, source);
    
    for (int i = 0; i < expected_count; i++) {
        Token token = lexer_next_token(&lexer);
        
        if (token.type != expected_types[i]) {
            printf("Token %d: expected type %s, got %s\n", i, 
                   token_type_name(expected_types[i]), 
                   token_type_name(token.type));
            token_free(&token);
            return false;
        }
        
        if (expected_values[i] && token.value) {
            if (strcmp(token.value, expected_values[i]) != 0) {
                printf("Token %d: expected value '%s', got '%s'\n", i, 
                       expected_values[i], token.value);
                token_free(&token);
                return false;
            }
        }
        
        token_free(&token);
    }
    
    /* Check that we reach EOF */
    Token eof_token = lexer_next_token(&lexer);
    bool is_eof = (eof_token.type == TOK_EOF);
    token_free(&eof_token);
    
    return is_eof;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Basic Token Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_keywords() {
    TokenType expected[] = {TOK_REL, TOK_FACT, TOK_RULE, TOK_SCAN, TOK_JOIN, 
                           TOK_EMIT, TOK_MATCH, TOK_SOLVE, TOK_QUERY};
    const char *values[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    
    return tokenize_and_check("REL FACT RULE SCAN JOIN EMIT MATCH SOLVE QUERY", 
                             expected, values, 9);
}

static bool test_keywords_case_insensitive() {
    TokenType expected[] = {TOK_REL, TOK_FACT, TOK_RULE};
    const char *values[] = {NULL, NULL, NULL};
    
    return tokenize_and_check("rel Fact RULE", expected, values, 3);
}

static bool test_symbols() {
    TokenType expected[] = {TOK_COLON, TOK_COMMA, TOK_WILDCARD};
    const char *values[] = {NULL, NULL, NULL};
    
    return tokenize_and_check(": , ?", expected, values, 3);
}

static bool test_variables() {
    Lexer lexer;
    lexer_init(&lexer, "$0 $1 $42 $123");
    
    Token token;
    
    /* $0 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_VARIABLE);
    ASSERT_EQ(token.int_value, 0);
    token_free(&token);
    
    /* $1 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_VARIABLE);
    ASSERT_EQ(token.int_value, 1);
    token_free(&token);
    
    /* $42 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_VARIABLE);
    ASSERT_EQ(token.int_value, 42);
    token_free(&token);
    
    /* $123 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_VARIABLE);
    ASSERT_EQ(token.int_value, 123);
    token_free(&token);
    
    return true;
}

static bool test_integers() {
    Lexer lexer;
    lexer_init(&lexer, "0 42 -17 123");
    
    Token token;
    
    /* 0 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_INTEGER);
    ASSERT_EQ(token.int_value, 0);
    token_free(&token);
    
    /* 42 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_INTEGER);
    ASSERT_EQ(token.int_value, 42);
    token_free(&token);
    
    /* -17 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_INTEGER);
    ASSERT_EQ(token.int_value, -17);
    token_free(&token);
    
    /* 123 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_INTEGER);
    ASSERT_EQ(token.int_value, 123);
    token_free(&token);
    
    return true;
}

static bool test_identifiers() {
    TokenType expected[] = {TOK_IDENTIFIER, TOK_IDENTIFIER, TOK_IDENTIFIER, TOK_IDENTIFIER};
    const char *values[] = {"parent", "ancestor_of", "_private", "rel2"};
    
    return tokenize_and_check("parent ancestor_of _private rel2", 
                             expected, values, 4);
}

/* ─────────────────────────────────────────────────────────────────────────
 * Comment and Whitespace Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_semicolon_comments() {
    TokenType expected[] = {TOK_REL, TOK_IDENTIFIER};
    const char *values[] = {NULL, "parent"};
    
    return tokenize_and_check("REL ; this is a comment\nparent", 
                             expected, values, 2);
}

static bool test_cpp_style_comments() {
    TokenType expected[] = {TOK_REL, TOK_IDENTIFIER};
    const char *values[] = {NULL, "parent"};
    
    return tokenize_and_check("REL // this is a comment\nparent", 
                             expected, values, 2);
}

static bool test_whitespace_handling() {
    TokenType expected[] = {TOK_REL, TOK_IDENTIFIER, TOK_COLON, TOK_INTEGER};
    const char *values[] = {NULL, "test", NULL, NULL};
    
    return tokenize_and_check("  REL\t\ttest\n\n:\r  42  ", 
                             expected, values, 4);
}

/* ─────────────────────────────────────────────────────────────────────────
 * Complete Statement Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_rel_declaration() {
    TokenType expected[] = {TOK_REL, TOK_IDENTIFIER};
    const char *values[] = {NULL, "parent"};
    
    return tokenize_and_check("REL parent", expected, values, 2);
}

static bool test_fact_statement() {
    Lexer lexer;
    lexer_init(&lexer, "FACT parent 0 1");
    
    Token token;
    
    /* FACT */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_FACT);
    token_free(&token);
    
    /* parent */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_IDENTIFIER);
    ASSERT_STR_EQ(token.value, "parent");
    token_free(&token);
    
    /* 0 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_INTEGER);
    ASSERT_EQ(token.int_value, 0);
    token_free(&token);
    
    /* 1 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_INTEGER);
    ASSERT_EQ(token.int_value, 1);
    token_free(&token);
    
    return true;
}

static bool test_rule_statement() {
    const char *source = "RULE ancestor: SCAN parent, JOIN ancestor $1, EMIT ancestor $0 $2";
    
    TokenType expected[] = {
        TOK_RULE, TOK_IDENTIFIER, TOK_COLON,
        TOK_SCAN, TOK_IDENTIFIER, TOK_COMMA,
        TOK_JOIN, TOK_IDENTIFIER, TOK_VARIABLE, TOK_COMMA,
        TOK_EMIT, TOK_IDENTIFIER, TOK_VARIABLE, TOK_VARIABLE
    };
    
    const char *values[] = {
        NULL, "ancestor", NULL,
        NULL, "parent", NULL,
        NULL, "ancestor", NULL, NULL,
        NULL, "ancestor", NULL, NULL
    };
    
    return tokenize_and_check(source, expected, values, 14);
}

static bool test_query_statement() {
    Lexer lexer;
    lexer_init(&lexer, "QUERY parent 0 ?");
    
    Token token;
    
    /* QUERY */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_QUERY);
    token_free(&token);
    
    /* parent */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_IDENTIFIER);
    ASSERT_STR_EQ(token.value, "parent");
    token_free(&token);
    
    /* 0 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_INTEGER);
    ASSERT_EQ(token.int_value, 0);
    token_free(&token);
    
    /* ? */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_WILDCARD);
    token_free(&token);
    
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Error Handling Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_invalid_variable() {
    Lexer lexer;
    lexer_init(&lexer, "$");
    
    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_ERROR);
    token_free(&token);
    
    return true;
}

static bool test_invalid_character() {
    Lexer lexer;
    lexer_init(&lexer, "REL @invalid");
    
    Token token;
    
    /* REL */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_REL);
    token_free(&token);
    
    /* @ should cause error */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_ERROR);
    token_free(&token);
    
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Line and Column Tracking Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_line_column_tracking() {
    Lexer lexer;
    lexer_init(&lexer, "REL\n  parent");
    
    Token token;
    
    /* REL at line 1, column 1 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_REL);
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 1);
    token_free(&token);
    
    /* parent at line 2, column 3 */
    token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_IDENTIFIER);
    ASSERT_EQ(token.line, 2);
    ASSERT_EQ(token.column, 3);
    token_free(&token);
    
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Edge Case Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_empty_source() {
    Lexer lexer;
    lexer_init(&lexer, "");
    
    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_EOF);
    token_free(&token);
    
    return true;
}

static bool test_only_whitespace() {
    Lexer lexer;
    lexer_init(&lexer, "   \t\n\r  ");
    
    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_EOF);
    token_free(&token);
    
    return true;
}

static bool test_only_comments() {
    Lexer lexer;
    lexer_init(&lexer, "; just a comment\n// another comment");
    
    Token token = lexer_next_token(&lexer);
    ASSERT_EQ(token.type, TOK_EOF);
    token_free(&token);
    
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Integration Test - Complete Program
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_complete_program() {
    const char *source = 
        "REL parent\n"
        "REL ancestor\n"
        "\n"
        "; Facts about family\n"
        "FACT parent 0 1\n"
        "FACT parent 1 2\n"
        "\n"
        "; Rules for ancestor relationship\n"
        "RULE ancestor: SCAN parent, EMIT ancestor $0 $1\n"
        "RULE ancestor: SCAN parent, JOIN ancestor $1, EMIT ancestor $0 $2\n"
        "\n"
        "SOLVE\n"
        "QUERY ancestor 0 ?\n";
    
    Lexer lexer;
    lexer_init(&lexer, source);
    
    int token_count = 0;
    Token token;
    
    do {
        token = lexer_next_token(&lexer);
        if (token.type == TOK_ERROR) {
            printf("Unexpected error token: %s\n", lexer_get_error(&lexer));
            token_free(&token);
            return false;
        }
        token_count++;
        token_free(&token);
    } while (token.type != TOK_EOF);
    
    /* Should have parsed many tokens successfully */
    ASSERT(token_count > 30);
    
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Main Test Runner
 * ───────────────────────────────────────────────────────────────────────── */

int main() {
    printf("ByteLog Lexer Unit Tests\n");
    printf("═══════════════════════════════════════\n\n");
    
    /* Basic token tests */
    TEST(keywords);
    TEST(keywords_case_insensitive);
    TEST(symbols);
    TEST(variables);
    TEST(integers);
    TEST(identifiers);
    
    /* Comment and whitespace tests */
    TEST(semicolon_comments);
    TEST(cpp_style_comments);
    TEST(whitespace_handling);
    
    /* Statement tests */
    TEST(rel_declaration);
    TEST(fact_statement);
    TEST(rule_statement);
    TEST(query_statement);
    
    /* Error handling tests */
    TEST(invalid_variable);
    TEST(invalid_character);
    
    /* Line/column tracking */
    TEST(line_column_tracking);
    
    /* Edge cases */
    TEST(empty_source);
    TEST(only_whitespace);
    TEST(only_comments);
    
    /* Integration test */
    TEST(complete_program);
    
    printf("\n═══════════════════════════════════════\n");
    printf("Results: %d/%d tests passed\n", test_passed, test_count);
    
    if (test_passed == test_count) {
        printf("All tests PASSED! ✓\n");
        return 0;
    } else {
        printf("%d tests FAILED! ✗\n", test_count - test_passed);
        return 1;
    }
}