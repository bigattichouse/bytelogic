/* ═══════════════════════════════════════════════════════════════════════════
 * test_atoms.c - Unit Tests for ByteLog Atom System
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Tests for atom table functionality and atom-based parsing.
 *
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "atoms.h"
#include "parser.h"
#include "ast.h"
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
 * Atom Table Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_atom_table_init() {
    AtomTable table;
    atom_table_init(&table);
    
    ASSERT_EQ(table.count, 0);
    ASSERT_EQ(table.next_id, 0);
    
    atom_table_free(&table);
    return true;
}

static bool test_atom_intern_single() {
    AtomTable table;
    atom_table_init(&table);
    
    int id = atom_table_intern(&table, "hello");
    ASSERT_EQ(id, 0);
    ASSERT_EQ(table.count, 1);
    
    atom_table_free(&table);
    return true;
}

static bool test_atom_intern_multiple() {
    AtomTable table;
    atom_table_init(&table);
    
    int id1 = atom_table_intern(&table, "hello");
    int id2 = atom_table_intern(&table, "world");
    int id3 = atom_table_intern(&table, "foo");
    
    ASSERT_EQ(id1, 0);
    ASSERT_EQ(id2, 1);
    ASSERT_EQ(id3, 2);
    ASSERT_EQ(table.count, 3);
    
    atom_table_free(&table);
    return true;
}

static bool test_atom_intern_duplicate() {
    AtomTable table;
    atom_table_init(&table);
    
    int id1 = atom_table_intern(&table, "hello");
    int id2 = atom_table_intern(&table, "hello");
    
    ASSERT_EQ(id1, id2);
    ASSERT_EQ(id1, 0);
    ASSERT_EQ(table.count, 1);
    
    atom_table_free(&table);
    return true;
}

static bool test_atom_lookup() {
    AtomTable table;
    atom_table_init(&table);
    
    atom_table_intern(&table, "hello");
    atom_table_intern(&table, "world");
    
    int id1 = atom_table_lookup(&table, "hello");
    int id2 = atom_table_lookup(&table, "world");
    int id3 = atom_table_lookup(&table, "notfound");
    
    ASSERT_EQ(id1, 0);
    ASSERT_EQ(id2, 1);
    ASSERT_EQ(id3, -1);
    
    atom_table_free(&table);
    return true;
}

static bool test_atom_name() {
    AtomTable table;
    atom_table_init(&table);
    
    atom_table_intern(&table, "hello");
    atom_table_intern(&table, "world");
    
    ASSERT_STR_EQ(atom_table_name(&table, 0), "hello");
    ASSERT_STR_EQ(atom_table_name(&table, 1), "world");
    ASSERT(atom_table_name(&table, 99) == NULL);
    
    atom_table_free(&table);
    return true;
}

static bool test_atom_table_many_atoms() {
    AtomTable table;
    atom_table_init(&table);
    
    /* Add many atoms to test hash table functionality */
    for (int i = 0; i < 100; i++) {
        char name[32];
        snprintf(name, sizeof(name), "atom%d", i);
        int id = atom_table_intern(&table, name);
        ASSERT_EQ(id, i);
    }
    
    ASSERT_EQ(table.count, 100);
    ASSERT_EQ(table.next_id, 100);
    
    /* Verify all atoms are still accessible */
    for (int i = 0; i < 100; i++) {
        char name[32];
        snprintf(name, sizeof(name), "atom%d", i);
        int id = atom_table_lookup(&table, name);
        ASSERT_EQ(id, i);
    }
    
    atom_table_free(&table);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Atom Parsing Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_parse_fact_with_atoms() {
    char error_buf[512];
    ASTNode *ast = parse_string("FACT likes alice pizza", error_buf, sizeof(error_buf));
    
    ASSERT(ast != NULL);
    ASSERT_EQ(ast->type, AST_PROGRAM);
    
    ASTNode *fact = ast->data.program.statements;
    ASSERT(fact != NULL);
    ASSERT_EQ(fact->type, AST_FACT);
    ASSERT_STR_EQ(fact->data.fact.relation, "likes");
    ASSERT_EQ(fact->data.fact.a, 0);  /* alice = 0 */
    ASSERT_EQ(fact->data.fact.b, 1);  /* pizza = 1 */
    ASSERT_STR_EQ(fact->data.fact.atom_a, "alice");
    ASSERT_STR_EQ(fact->data.fact.atom_b, "pizza");
    
    ast_free_tree(ast);
    return true;
}

static bool test_parse_fact_mixed_args() {
    char error_buf[512];
    ASTNode *ast = parse_string("FACT test alice 42", error_buf, sizeof(error_buf));
    
    ASSERT(ast != NULL);
    ASSERT_EQ(ast->type, AST_PROGRAM);
    
    ASTNode *fact = ast->data.program.statements;
    ASSERT(fact != NULL);
    ASSERT_EQ(fact->type, AST_FACT);
    ASSERT_STR_EQ(fact->data.fact.relation, "test");
    ASSERT_EQ(fact->data.fact.a, 0);   /* alice = 0 */
    ASSERT_EQ(fact->data.fact.b, 42);  /* 42 = 42 */
    ASSERT_STR_EQ(fact->data.fact.atom_a, "alice");
    ASSERT(fact->data.fact.atom_b == NULL);  /* Was integer */
    
    ast_free_tree(ast);
    return true;
}

static bool test_parse_query_with_atoms() {
    char error_buf[512];
    ASTNode *ast = parse_string("QUERY likes alice ?", error_buf, sizeof(error_buf));
    
    ASSERT(ast != NULL);
    ASSERT_EQ(ast->type, AST_PROGRAM);
    
    ASTNode *query = ast->data.program.statements;
    ASSERT(query != NULL);
    ASSERT_EQ(query->type, AST_QUERY);
    ASSERT_STR_EQ(query->data.query.relation, "likes");
    ASSERT_EQ(query->data.query.arg_a, 0);   /* alice = 0 */
    ASSERT_EQ(query->data.query.arg_b, -1);  /* wildcard */
    ASSERT_STR_EQ(query->data.query.atom_a, "alice");
    ASSERT(query->data.query.atom_b == NULL);  /* Was wildcard */
    
    ast_free_tree(ast);
    return true;
}

static bool test_parse_multiple_facts_same_atom() {
    char error_buf[512];
    ASTNode *ast = parse_string("FACT likes alice pizza\nFACT likes bob pizza", 
                               error_buf, sizeof(error_buf));
    
    ASSERT(ast != NULL);
    ASSERT_EQ(ast->type, AST_PROGRAM);
    
    ASTNode *fact1 = ast->data.program.statements;
    ASTNode *fact2 = fact1->next;
    
    ASSERT(fact1 != NULL);
    ASSERT(fact2 != NULL);
    
    /* Both facts should use same ID for "pizza" */
    ASSERT_EQ(fact1->data.fact.b, fact2->data.fact.b);
    
    /* But different IDs for alice/bob */
    ASSERT(fact1->data.fact.a != fact2->data.fact.a);
    
    ast_free_tree(ast);
    return true;
}

static bool test_atom_case_sensitivity() {
    char error_buf[512];
    ASTNode *ast = parse_string("FACT test Alice alice\nFACT test alice ALICE", 
                               error_buf, sizeof(error_buf));
    
    ASSERT(ast != NULL);
    ASSERT_EQ(ast->type, AST_PROGRAM);
    
    ASTNode *fact1 = ast->data.program.statements;
    ASTNode *fact2 = fact1->next;
    ASSERT(fact1 != NULL);
    ASSERT(fact2 != NULL);
    
    /* Should treat Alice, alice, ALICE as different atoms */
    ASSERT(fact1->data.fact.a != fact1->data.fact.b);  /* Alice != alice */
    ASSERT(fact2->data.fact.a != fact2->data.fact.b);  /* alice != ALICE */
    ASSERT(fact1->data.fact.a != fact2->data.fact.b);  /* Alice != ALICE */
    
    ast_free_tree(ast);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Integration Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_complete_program_with_atoms() {
    char error_buf[512];
    const char *program = 
        "REL likes\n"
        "FACT likes alice pizza\n"
        "FACT likes bob pasta\n"
        "QUERY likes alice ?\n";
    
    ASTNode *ast = parse_string(program, error_buf, sizeof(error_buf));
    
    ASSERT(ast != NULL);
    ASSERT_EQ(ast->type, AST_PROGRAM);
    
    ASTNode *stmt = ast->data.program.statements;
    
    /* REL likes */
    ASSERT(stmt != NULL);
    ASSERT_EQ(stmt->type, AST_REL_DECL);
    
    /* FACT likes alice pizza */
    stmt = stmt->next;
    ASSERT(stmt != NULL);
    ASSERT_EQ(stmt->type, AST_FACT);
    ASSERT_STR_EQ(stmt->data.fact.atom_a, "alice");
    ASSERT_STR_EQ(stmt->data.fact.atom_b, "pizza");
    
    /* FACT likes bob pasta */
    stmt = stmt->next;
    ASSERT(stmt != NULL);
    ASSERT_EQ(stmt->type, AST_FACT);
    ASSERT_STR_EQ(stmt->data.fact.atom_a, "bob");
    ASSERT_STR_EQ(stmt->data.fact.atom_b, "pasta");
    
    /* QUERY likes alice ? */
    stmt = stmt->next;
    ASSERT(stmt != NULL);
    ASSERT_EQ(stmt->type, AST_QUERY);
    ASSERT_STR_EQ(stmt->data.query.atom_a, "alice");
    
    ast_free_tree(ast);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Test Runner
 * ───────────────────────────────────────────────────────────────────────── */

int main(void) {
    printf("ByteLog Atom System Tests\n");
    printf("═══════════════════════════════════════\n\n");

    /* Atom Table Tests */
    printf("Atom Table Tests:\n");
    printf("─────────────────\n");
    TEST(atom_table_init);
    TEST(atom_intern_single);
    TEST(atom_intern_multiple);
    TEST(atom_intern_duplicate);
    TEST(atom_lookup);
    TEST(atom_name);
    TEST(atom_table_many_atoms);
    printf("\n");

    /* Atom Parsing Tests */
    printf("Atom Parsing Tests:\n");
    printf("───────────────────\n");
    TEST(parse_fact_with_atoms);
    TEST(parse_fact_mixed_args);
    TEST(parse_query_with_atoms);
    TEST(parse_multiple_facts_same_atom);
    TEST(atom_case_sensitivity);
    printf("\n");

    /* Integration Tests */
    printf("Integration Tests:\n");
    printf("──────────────────\n");
    TEST(complete_program_with_atoms);
    printf("\n");

    printf("Test Results:\n");
    printf("═════════════\n");
    printf("Tests run: %d\n", test_count);
    printf("Tests passed: %d\n", test_passed);
    printf("Tests failed: %d\n", test_count - test_passed);
    
    if (test_passed == test_count) {
        printf("\n✅ All tests passed!\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed!\n");
        return 1;
    }
}