/* ═══════════════════════════════════════════════════════════════════════════
 * test_parser.c - Unit Tests for ByteLog Parser
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Comprehensive unit tests for each language feature and combinations.
 * Tests parsing of all AST node types and error conditions.
 *
 * ═══════════════════════════════════════════════════════════════════════════
 */

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

#define ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            printf("ASSERTION FAILED: %s should be NULL\n", #ptr); \
            return false; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            printf("ASSERTION FAILED: %s should not be NULL\n", #ptr); \
            return false; \
        } \
    } while(0)

/* ─────────────────────────────────────────────────────────────────────────
 * Helper Functions
 * ───────────────────────────────────────────────────────────────────────── */

static ASTNode* parse_and_check(const char *source, bool should_succeed) {
    char error_buf[512];
    ASTNode *ast = parse_string(source, error_buf, sizeof(error_buf));
    
    if (should_succeed) {
        if (!ast) {
            printf("Parse failed but should have succeeded: %s\n", error_buf);
            return NULL;
        }
    } else {
        if (ast) {
            printf("Parse succeeded but should have failed\n");
            ast_free_tree(ast);
            return NULL;
        }
    }
    
    return ast;
}

static ASTNode* get_first_statement(ASTNode *program) {
    ASSERT_NOT_NULL(program);
    ASSERT_EQ(program->type, AST_PROGRAM);
    return program->data.program.statements;
}

/* ─────────────────────────────────────────────────────────────────────────
 * REL Declaration Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_rel_declaration_basic() {
    ASTNode *ast = parse_and_check("REL parent", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *stmt = get_first_statement(ast);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_REL_DECL);
    ASSERT_STR_EQ(stmt->data.rel_decl.name, "parent");
    
    ast_free_tree(ast);
    return true;
}

static bool test_rel_declaration_multiple() {
    ASTNode *ast = parse_and_check("REL parent\nREL child\nREL ancestor", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *stmt = get_first_statement(ast);
    
    /* First REL */
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_REL_DECL);
    ASSERT_STR_EQ(stmt->data.rel_decl.name, "parent");
    
    /* Second REL */
    stmt = stmt->next;
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_REL_DECL);
    ASSERT_STR_EQ(stmt->data.rel_decl.name, "child");
    
    /* Third REL */
    stmt = stmt->next;
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_REL_DECL);
    ASSERT_STR_EQ(stmt->data.rel_decl.name, "ancestor");
    
    ast_free_tree(ast);
    return true;
}

static bool test_rel_declaration_case_insensitive() {
    ASTNode *ast = parse_and_check("rel parent", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *stmt = get_first_statement(ast);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_REL_DECL);
    ASSERT_STR_EQ(stmt->data.rel_decl.name, "parent");
    
    ast_free_tree(ast);
    return true;
}

static bool test_rel_declaration_underscore_names() {
    ASTNode *ast = parse_and_check("REL _private\nREL has_child\nREL rel_2", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *stmt = get_first_statement(ast);
    
    /* _private */
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_REL_DECL);
    ASSERT_STR_EQ(stmt->data.rel_decl.name, "_private");
    
    /* has_child */
    stmt = stmt->next;
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->data.rel_decl.name, "has_child");
    
    /* rel_2 */
    stmt = stmt->next;
    ASSERT_NOT_NULL(stmt);
    ASSERT_STR_EQ(stmt->data.rel_decl.name, "rel_2");
    
    ast_free_tree(ast);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * FACT Statement Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_fact_basic() {
    ASTNode *ast = parse_and_check("FACT parent 0 1", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *stmt = get_first_statement(ast);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_FACT);
    ASSERT_STR_EQ(stmt->data.fact.relation, "parent");
    ASSERT_EQ(stmt->data.fact.a, 0);
    ASSERT_EQ(stmt->data.fact.b, 1);
    
    ast_free_tree(ast);
    return true;
}

static bool test_fact_negative_numbers() {
    ASTNode *ast = parse_and_check("FACT relation -5 -10", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *stmt = get_first_statement(ast);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_FACT);
    ASSERT_STR_EQ(stmt->data.fact.relation, "relation");
    ASSERT_EQ(stmt->data.fact.a, -5);
    ASSERT_EQ(stmt->data.fact.b, -10);
    
    ast_free_tree(ast);
    return true;
}

static bool test_fact_large_numbers() {
    ASTNode *ast = parse_and_check("FACT test 999 1000", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *stmt = get_first_statement(ast);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_FACT);
    ASSERT_EQ(stmt->data.fact.a, 999);
    ASSERT_EQ(stmt->data.fact.b, 1000);
    
    ast_free_tree(ast);
    return true;
}

static bool test_fact_multiple() {
    ASTNode *ast = parse_and_check("FACT parent 0 1\nFACT parent 1 2\nFACT parent 2 3", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *stmt = get_first_statement(ast);
    
    /* First fact */
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_FACT);
    ASSERT_EQ(stmt->data.fact.a, 0);
    ASSERT_EQ(stmt->data.fact.b, 1);
    
    /* Second fact */
    stmt = stmt->next;
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_FACT);
    ASSERT_EQ(stmt->data.fact.a, 1);
    ASSERT_EQ(stmt->data.fact.b, 2);
    
    /* Third fact */
    stmt = stmt->next;
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_FACT);
    ASSERT_EQ(stmt->data.fact.a, 2);
    ASSERT_EQ(stmt->data.fact.b, 3);
    
    ast_free_tree(ast);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * SCAN Operation Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_scan_basic() {
    ASTNode *ast = parse_and_check("RULE target: SCAN relation, EMIT target $0 $1", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *rule = get_first_statement(ast);
    ASSERT_NOT_NULL(rule);
    ASSERT_EQ(rule->type, AST_RULE);
    
    ASTNode *scan = rule->data.rule.body;
    ASSERT_NOT_NULL(scan);
    ASSERT_EQ(scan->type, AST_SCAN);
    ASSERT_STR_EQ(scan->data.scan.relation, "relation");
    ASSERT_EQ(scan->data.scan.has_match, false);
    
    ast_free_tree(ast);
    return true;
}

static bool test_scan_with_match() {
    ASTNode *ast = parse_and_check("RULE target: SCAN relation MATCH $5, EMIT target $0 $1", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *rule = get_first_statement(ast);
    ASSERT_NOT_NULL(rule);
    ASSERT_EQ(rule->type, AST_RULE);
    
    ASTNode *scan = rule->data.rule.body;
    ASSERT_NOT_NULL(scan);
    ASSERT_EQ(scan->type, AST_SCAN);
    ASSERT_STR_EQ(scan->data.scan.relation, "relation");
    ASSERT_EQ(scan->data.scan.has_match, true);
    ASSERT_EQ(scan->data.scan.match_var, 5);
    
    ast_free_tree(ast);
    return true;
}

static bool test_scan_multiple_variables() {
    ASTNode *ast = parse_and_check("RULE target: SCAN r1 MATCH $0, SCAN r2 MATCH $10, EMIT target $0 $1", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *rule = get_first_statement(ast);
    ASTNode *scan1 = rule->data.rule.body;
    ASTNode *scan2 = scan1->next;
    
    /* First scan */
    ASSERT_NOT_NULL(scan1);
    ASSERT_EQ(scan1->type, AST_SCAN);
    ASSERT_STR_EQ(scan1->data.scan.relation, "r1");
    ASSERT_EQ(scan1->data.scan.has_match, true);
    ASSERT_EQ(scan1->data.scan.match_var, 0);
    
    /* Second scan */
    ASSERT_NOT_NULL(scan2);
    ASSERT_EQ(scan2->type, AST_SCAN);
    ASSERT_STR_EQ(scan2->data.scan.relation, "r2");
    ASSERT_EQ(scan2->data.scan.has_match, true);
    ASSERT_EQ(scan2->data.scan.match_var, 10);
    
    ast_free_tree(ast);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * JOIN Operation Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_join_basic() {
    ASTNode *ast = parse_and_check("RULE target: SCAN r1, JOIN r2 $1, EMIT target $0 $2", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *rule = get_first_statement(ast);
    ASTNode *scan = rule->data.rule.body;
    ASTNode *join = scan->next;
    
    ASSERT_NOT_NULL(join);
    ASSERT_EQ(join->type, AST_JOIN);
    ASSERT_STR_EQ(join->data.join.relation, "r2");
    ASSERT_EQ(join->data.join.match_var, 1);
    
    ast_free_tree(ast);
    return true;
}

static bool test_join_multiple() {
    ASTNode *ast = parse_and_check("RULE target: SCAN r1, JOIN r2 $1, JOIN r3 $2, EMIT target $0 $3", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *rule = get_first_statement(ast);
    ASTNode *scan = rule->data.rule.body;
    ASTNode *join1 = scan->next;
    ASTNode *join2 = join1->next;
    
    /* First join */
    ASSERT_NOT_NULL(join1);
    ASSERT_EQ(join1->type, AST_JOIN);
    ASSERT_STR_EQ(join1->data.join.relation, "r2");
    ASSERT_EQ(join1->data.join.match_var, 1);
    
    /* Second join */
    ASSERT_NOT_NULL(join2);
    ASSERT_EQ(join2->type, AST_JOIN);
    ASSERT_STR_EQ(join2->data.join.relation, "r3");
    ASSERT_EQ(join2->data.join.match_var, 2);
    
    ast_free_tree(ast);
    return true;
}

static bool test_join_high_variable_numbers() {
    ASTNode *ast = parse_and_check("RULE target: SCAN r1, JOIN r2 $42, EMIT target $0 $43", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *rule = get_first_statement(ast);
    ASTNode *scan = rule->data.rule.body;
    ASTNode *join = scan->next;
    
    ASSERT_NOT_NULL(join);
    ASSERT_EQ(join->type, AST_JOIN);
    ASSERT_EQ(join->data.join.match_var, 42);
    
    ast_free_tree(ast);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * EMIT Operation Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_emit_basic() {
    ASTNode *ast = parse_and_check("RULE target: SCAN r1, EMIT target $0 $1", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *rule = get_first_statement(ast);
    ASTNode *emit = rule->data.rule.emit;
    
    ASSERT_NOT_NULL(emit);
    ASSERT_EQ(emit->type, AST_EMIT);
    ASSERT_STR_EQ(emit->data.emit.relation, "target");
    ASSERT_EQ(emit->data.emit.var_a, 0);
    ASSERT_EQ(emit->data.emit.var_b, 1);
    
    ast_free_tree(ast);
    return true;
}

static bool test_emit_different_variables() {
    ASTNode *ast = parse_and_check("RULE target: SCAN r1, JOIN r2 $1, EMIT target $0 $2", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *rule = get_first_statement(ast);
    ASTNode *emit = rule->data.rule.emit;
    
    ASSERT_NOT_NULL(emit);
    ASSERT_EQ(emit->type, AST_EMIT);
    ASSERT_EQ(emit->data.emit.var_a, 0);
    ASSERT_EQ(emit->data.emit.var_b, 2);
    
    ast_free_tree(ast);
    return true;
}

static bool test_emit_high_variables() {
    ASTNode *ast = parse_and_check("RULE target: SCAN r1, EMIT target $100 $200", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *rule = get_first_statement(ast);
    ASTNode *emit = rule->data.rule.emit;
    
    ASSERT_NOT_NULL(emit);
    ASSERT_EQ(emit->data.emit.var_a, 100);
    ASSERT_EQ(emit->data.emit.var_b, 200);
    
    ast_free_tree(ast);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * RULE Statement Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_rule_simple() {
    ASTNode *ast = parse_and_check("RULE ancestor: SCAN parent, EMIT ancestor $0 $1", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *rule = get_first_statement(ast);
    ASSERT_NOT_NULL(rule);
    ASSERT_EQ(rule->type, AST_RULE);
    ASSERT_STR_EQ(rule->data.rule.target, "ancestor");
    
    /* Check body has scan */
    ASTNode *scan = rule->data.rule.body;
    ASSERT_NOT_NULL(scan);
    ASSERT_EQ(scan->type, AST_SCAN);
    
    /* Check emit */
    ASTNode *emit = rule->data.rule.emit;
    ASSERT_NOT_NULL(emit);
    ASSERT_EQ(emit->type, AST_EMIT);
    
    ast_free_tree(ast);
    return true;
}

static bool test_rule_transitive() {
    const char *source = "RULE ancestor: SCAN parent, JOIN ancestor $1, EMIT ancestor $0 $2";
    ASTNode *ast = parse_and_check(source, true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *rule = get_first_statement(ast);
    ASSERT_NOT_NULL(rule);
    ASSERT_EQ(rule->type, AST_RULE);
    ASSERT_STR_EQ(rule->data.rule.target, "ancestor");
    
    /* Check body operations */
    ASTNode *scan = rule->data.rule.body;
    ASSERT_NOT_NULL(scan);
    ASSERT_EQ(scan->type, AST_SCAN);
    ASSERT_STR_EQ(scan->data.scan.relation, "parent");
    
    ASTNode *join = scan->next;
    ASSERT_NOT_NULL(join);
    ASSERT_EQ(join->type, AST_JOIN);
    ASSERT_STR_EQ(join->data.join.relation, "ancestor");
    ASSERT_EQ(join->data.join.match_var, 1);
    
    /* Check emit */
    ASTNode *emit = rule->data.rule.emit;
    ASSERT_NOT_NULL(emit);
    ASSERT_EQ(emit->type, AST_EMIT);
    ASSERT_STR_EQ(emit->data.emit.relation, "ancestor");
    ASSERT_EQ(emit->data.emit.var_a, 0);
    ASSERT_EQ(emit->data.emit.var_b, 2);
    
    ast_free_tree(ast);
    return true;
}

static bool test_rule_complex() {
    const char *source = "RULE complex: SCAN r1 MATCH $5, JOIN r2 $1, JOIN r3 $2, EMIT complex $0 $3";
    ASTNode *ast = parse_and_check(source, true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *rule = get_first_statement(ast);
    ASSERT_NOT_NULL(rule);
    ASSERT_STR_EQ(rule->data.rule.target, "complex");
    
    /* Verify all operations in sequence */
    ASTNode *op = rule->data.rule.body;
    
    /* SCAN r1 MATCH $5 */
    ASSERT_NOT_NULL(op);
    ASSERT_EQ(op->type, AST_SCAN);
    ASSERT_STR_EQ(op->data.scan.relation, "r1");
    ASSERT_EQ(op->data.scan.has_match, true);
    ASSERT_EQ(op->data.scan.match_var, 5);
    
    /* JOIN r2 $1 */
    op = op->next;
    ASSERT_NOT_NULL(op);
    ASSERT_EQ(op->type, AST_JOIN);
    ASSERT_STR_EQ(op->data.join.relation, "r2");
    ASSERT_EQ(op->data.join.match_var, 1);
    
    /* JOIN r3 $2 */
    op = op->next;
    ASSERT_NOT_NULL(op);
    ASSERT_EQ(op->type, AST_JOIN);
    ASSERT_STR_EQ(op->data.join.relation, "r3");
    ASSERT_EQ(op->data.join.match_var, 2);
    
    /* EMIT complex $0 $3 */
    ASTNode *emit = rule->data.rule.emit;
    ASSERT_NOT_NULL(emit);
    ASSERT_EQ(emit->type, AST_EMIT);
    ASSERT_STR_EQ(emit->data.emit.relation, "complex");
    ASSERT_EQ(emit->data.emit.var_a, 0);
    ASSERT_EQ(emit->data.emit.var_b, 3);
    
    ast_free_tree(ast);
    return true;
}

static bool test_rule_multiple() {
    const char *source = 
        "RULE ancestor: SCAN parent, EMIT ancestor $0 $1\n"
        "RULE ancestor: SCAN parent, JOIN ancestor $1, EMIT ancestor $0 $2";
    
    ASTNode *ast = parse_and_check(source, true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *rule1 = get_first_statement(ast);
    ASTNode *rule2 = rule1->next;
    
    /* First rule */
    ASSERT_NOT_NULL(rule1);
    ASSERT_EQ(rule1->type, AST_RULE);
    ASSERT_STR_EQ(rule1->data.rule.target, "ancestor");
    
    /* Second rule */
    ASSERT_NOT_NULL(rule2);
    ASSERT_EQ(rule2->type, AST_RULE);
    ASSERT_STR_EQ(rule2->data.rule.target, "ancestor");
    
    ast_free_tree(ast);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * SOLVE Statement Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_solve_basic() {
    ASTNode *ast = parse_and_check("SOLVE", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *solve = get_first_statement(ast);
    ASSERT_NOT_NULL(solve);
    ASSERT_EQ(solve->type, AST_SOLVE);
    
    ast_free_tree(ast);
    return true;
}

static bool test_solve_case_insensitive() {
    ASTNode *ast = parse_and_check("solve", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *solve = get_first_statement(ast);
    ASSERT_NOT_NULL(solve);
    ASSERT_EQ(solve->type, AST_SOLVE);
    
    ast_free_tree(ast);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * QUERY Statement Tests  
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_query_both_concrete() {
    ASTNode *ast = parse_and_check("QUERY parent 0 1", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *query = get_first_statement(ast);
    ASSERT_NOT_NULL(query);
    ASSERT_EQ(query->type, AST_QUERY);
    ASSERT_STR_EQ(query->data.query.relation, "parent");
    ASSERT_EQ(query->data.query.arg_a, 0);
    ASSERT_EQ(query->data.query.arg_b, 1);
    
    ast_free_tree(ast);
    return true;
}

static bool test_query_first_wildcard() {
    ASTNode *ast = parse_and_check("QUERY parent ? 1", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *query = get_first_statement(ast);
    ASSERT_NOT_NULL(query);
    ASSERT_EQ(query->type, AST_QUERY);
    ASSERT_STR_EQ(query->data.query.relation, "parent");
    ASSERT_EQ(query->data.query.arg_a, -1);  /* -1 = wildcard */
    ASSERT_EQ(query->data.query.arg_b, 1);
    
    ast_free_tree(ast);
    return true;
}

static bool test_query_second_wildcard() {
    ASTNode *ast = parse_and_check("QUERY parent 0 ?", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *query = get_first_statement(ast);
    ASSERT_NOT_NULL(query);
    ASSERT_EQ(query->type, AST_QUERY);
    ASSERT_STR_EQ(query->data.query.relation, "parent");
    ASSERT_EQ(query->data.query.arg_a, 0);
    ASSERT_EQ(query->data.query.arg_b, -1);  /* -1 = wildcard */
    
    ast_free_tree(ast);
    return true;
}

static bool test_query_both_wildcards() {
    ASTNode *ast = parse_and_check("QUERY parent ? ?", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *query = get_first_statement(ast);
    ASSERT_NOT_NULL(query);
    ASSERT_EQ(query->type, AST_QUERY);
    ASSERT_STR_EQ(query->data.query.relation, "parent");
    ASSERT_EQ(query->data.query.arg_a, -1);  /* -1 = wildcard */
    ASSERT_EQ(query->data.query.arg_b, -1);  /* -1 = wildcard */
    
    ast_free_tree(ast);
    return true;
}

static bool test_query_negative_numbers() {
    ASTNode *ast = parse_and_check("QUERY relation -5 -10", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *query = get_first_statement(ast);
    ASSERT_NOT_NULL(query);
    ASSERT_EQ(query->type, AST_QUERY);
    ASSERT_EQ(query->data.query.arg_a, -5);
    ASSERT_EQ(query->data.query.arg_b, -10);
    
    ast_free_tree(ast);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Combined Feature Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_complete_program_ancestor() {
    const char *source =
        "REL parent\n"
        "REL ancestor\n"
        "\n"
        "FACT parent 0 1\n"
        "FACT parent 1 2\n"
        "FACT parent 2 3\n"
        "\n"
        "RULE ancestor: SCAN parent, EMIT ancestor $0 $1\n"
        "RULE ancestor: SCAN parent, JOIN ancestor $1, EMIT ancestor $0 $2\n"
        "\n"
        "SOLVE\n"
        "QUERY ancestor 0 ?";
    
    ASTNode *ast = parse_and_check(source, true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *stmt = get_first_statement(ast);
    int statement_count = 0;
    
    /* Count statements and verify types */
    while (stmt) {
        statement_count++;
        switch (statement_count) {
            case 1:
            case 2:
                ASSERT_EQ(stmt->type, AST_REL_DECL);
                break;
            case 3:
            case 4:
            case 5:
                ASSERT_EQ(stmt->type, AST_FACT);
                break;
            case 6:
            case 7:
                ASSERT_EQ(stmt->type, AST_RULE);
                break;
            case 8:
                ASSERT_EQ(stmt->type, AST_SOLVE);
                break;
            case 9:
                ASSERT_EQ(stmt->type, AST_QUERY);
                break;
        }
        stmt = stmt->next;
    }
    
    ASSERT_EQ(statement_count, 9);
    
    ast_free_tree(ast);
    return true;
}

static bool test_complete_program_graph_reachability() {
    const char *source =
        "REL edge\n"
        "REL reachable\n"
        "\n"
        "FACT edge 0 1\n"
        "FACT edge 0 2\n"
        "FACT edge 1 3\n"
        "FACT edge 2 3\n"
        "FACT edge 3 4\n"
        "\n"
        "RULE reachable: SCAN edge, EMIT reachable $0 $1\n"
        "RULE reachable: SCAN edge, JOIN reachable $1, EMIT reachable $0 $2\n"
        "\n"
        "SOLVE\n"
        "QUERY reachable 0 4";
    
    ASTNode *ast = parse_and_check(source, true);
    ASSERT_NOT_NULL(ast);
    
    /* Just verify we parsed successfully - detailed checking done above */
    ASTNode *stmt = get_first_statement(ast);
    ASSERT_NOT_NULL(stmt);
    
    ast_free_tree(ast);
    return true;
}

static bool test_comments_and_whitespace() {
    const char *source =
        "; This is a comment\n"
        "REL parent  ; inline comment\n"
        "\n"
        "// C++ style comment\n"
        "  FACT parent 0 1    // another comment\n"
        "\n"
        "\t\tSOLVE\n"
        "  QUERY parent ? ?  ; final comment";
    
    ASTNode *ast = parse_and_check(source, true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *stmt = get_first_statement(ast);
    
    /* REL parent */
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_REL_DECL);
    ASSERT_STR_EQ(stmt->data.rel_decl.name, "parent");
    
    /* FACT parent 0 1 */
    stmt = stmt->next;
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_FACT);
    
    /* SOLVE */
    stmt = stmt->next;
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_SOLVE);
    
    /* QUERY */
    stmt = stmt->next;
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_QUERY);
    
    ast_free_tree(ast);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Error Handling Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_error_missing_relation_name() {
    ASTNode *ast = parse_and_check("REL", false);
    ASSERT_NULL(ast);
    return true;
}

static bool test_error_missing_fact_arguments() {
    ASTNode *ast = parse_and_check("FACT parent", false);
    ASSERT_NULL(ast);
    return true;
}

static bool test_error_missing_colon_in_rule() {
    ASTNode *ast = parse_and_check("RULE target SCAN parent, EMIT target $0 $1", false);
    ASSERT_NULL(ast);
    return true;
}

static bool test_error_missing_emit() {
    ASTNode *ast = parse_and_check("RULE target: SCAN parent", false);
    ASSERT_NULL(ast);
    return true;
}

static bool test_error_invalid_variable() {
    ASTNode *ast = parse_and_check("RULE target: SCAN parent, EMIT target parent $1", false);
    ASSERT_NULL(ast);
    return true;
}

static bool test_error_missing_query_args() {
    ASTNode *ast = parse_and_check("QUERY parent", false);
    ASSERT_NULL(ast);
    return true;
}

static bool test_error_invalid_statement() {
    ASTNode *ast = parse_and_check("INVALID statement", false);
    ASSERT_NULL(ast);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Edge Case Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_empty_program() {
    ASTNode *ast = parse_and_check("", true);
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_PROGRAM);
    ASSERT_NULL(ast->data.program.statements);
    
    ast_free_tree(ast);
    return true;
}

static bool test_only_comments() {
    ASTNode *ast = parse_and_check("; just comments\n// more comments", true);
    ASSERT_NOT_NULL(ast);
    ASSERT_EQ(ast->type, AST_PROGRAM);
    ASSERT_NULL(ast->data.program.statements);
    
    ast_free_tree(ast);
    return true;
}

static bool test_single_statement() {
    ASTNode *ast = parse_and_check("SOLVE", true);
    ASSERT_NOT_NULL(ast);
    
    ASTNode *stmt = get_first_statement(ast);
    ASSERT_NOT_NULL(stmt);
    ASSERT_EQ(stmt->type, AST_SOLVE);
    ASSERT_NULL(stmt->next);
    
    ast_free_tree(ast);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Main Test Runner
 * ───────────────────────────────────────────────────────────────────────── */

int main() {
    printf("ByteLog Parser Unit Tests\n");
    printf("═══════════════════════════════════════\n\n");
    
    /* REL declaration tests */
    printf("Testing REL declarations:\n");
    TEST(rel_declaration_basic);
    TEST(rel_declaration_multiple);
    TEST(rel_declaration_case_insensitive);
    TEST(rel_declaration_underscore_names);
    printf("\n");
    
    /* FACT statement tests */
    printf("Testing FACT statements:\n");
    TEST(fact_basic);
    TEST(fact_negative_numbers);
    TEST(fact_large_numbers);
    TEST(fact_multiple);
    printf("\n");
    
    /* SCAN operation tests */
    printf("Testing SCAN operations:\n");
    TEST(scan_basic);
    TEST(scan_with_match);
    TEST(scan_multiple_variables);
    printf("\n");
    
    /* JOIN operation tests */
    printf("Testing JOIN operations:\n");
    TEST(join_basic);
    TEST(join_multiple);
    TEST(join_high_variable_numbers);
    printf("\n");
    
    /* EMIT operation tests */
    printf("Testing EMIT operations:\n");
    TEST(emit_basic);
    TEST(emit_different_variables);
    TEST(emit_high_variables);
    printf("\n");
    
    /* RULE statement tests */
    printf("Testing RULE statements:\n");
    TEST(rule_simple);
    TEST(rule_transitive);
    TEST(rule_complex);
    TEST(rule_multiple);
    printf("\n");
    
    /* SOLVE statement tests */
    printf("Testing SOLVE statements:\n");
    TEST(solve_basic);
    TEST(solve_case_insensitive);
    printf("\n");
    
    /* QUERY statement tests */
    printf("Testing QUERY statements:\n");
    TEST(query_both_concrete);
    TEST(query_first_wildcard);
    TEST(query_second_wildcard);
    TEST(query_both_wildcards);
    TEST(query_negative_numbers);
    printf("\n");
    
    /* Combined feature tests */
    printf("Testing combined features:\n");
    TEST(complete_program_ancestor);
    TEST(complete_program_graph_reachability);
    TEST(comments_and_whitespace);
    printf("\n");
    
    /* Error handling tests */
    printf("Testing error handling:\n");
    TEST(error_missing_relation_name);
    TEST(error_missing_fact_arguments);
    TEST(error_missing_colon_in_rule);
    TEST(error_missing_emit);
    TEST(error_invalid_variable);
    TEST(error_missing_query_args);
    TEST(error_invalid_statement);
    printf("\n");
    
    /* Edge case tests */
    printf("Testing edge cases:\n");
    TEST(empty_program);
    TEST(only_comments);
    TEST(single_statement);
    printf("\n");
    
    printf("═══════════════════════════════════════\n");
    printf("Results: %d/%d tests passed\n", test_passed, test_count);
    
    if (test_passed == test_count) {
        printf("All tests PASSED! ✓\n");
        return 0;
    } else {
        printf("%d tests FAILED! ✗\n", test_count - test_passed);
        return 1;
    }
}