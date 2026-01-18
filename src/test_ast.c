/* ═══════════════════════════════════════════════════════════════════════════
 * test_ast.c - Unit Tests for ByteLog AST
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Comprehensive unit tests for AST node creation, manipulation, and utilities.
 * Tests all AST node types and operations.
 *
 * ═══════════════════════════════════════════════════════════════════════════
 */

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
 * AST Node Creation Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_make_rel_decl() {
    ASTNode *node = ast_make_rel_decl("parent", 1, 5);
    
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(node->type, AST_REL_DECL);
    ASSERT_STR_EQ(node->data.rel_decl.name, "parent");
    ASSERT_EQ(node->line, 1);
    ASSERT_EQ(node->column, 5);
    ASSERT_NULL(node->next);
    
    ast_free(node);
    return true;
}

static bool test_make_fact() {
    ASTNode *node = ast_make_fact("parent", 42, -17, 2, 10);
    
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(node->type, AST_FACT);
    ASSERT_STR_EQ(node->data.fact.relation, "parent");
    ASSERT_EQ(node->data.fact.a, 42);
    ASSERT_EQ(node->data.fact.b, -17);
    ASSERT_EQ(node->line, 2);
    ASSERT_EQ(node->column, 10);
    
    ast_free(node);
    return true;
}

static bool test_make_scan() {
    /* Test scan without match */
    ASTNode *node1 = ast_make_scan("relation", false, -1, 3, 1);
    
    ASSERT_NOT_NULL(node1);
    ASSERT_EQ(node1->type, AST_SCAN);
    ASSERT_STR_EQ(node1->data.scan.relation, "relation");
    ASSERT_EQ(node1->data.scan.has_match, false);
    
    ast_free(node1);
    
    /* Test scan with match */
    ASTNode *node2 = ast_make_scan("other", true, 5, 4, 2);
    
    ASSERT_NOT_NULL(node2);
    ASSERT_EQ(node2->type, AST_SCAN);
    ASSERT_STR_EQ(node2->data.scan.relation, "other");
    ASSERT_EQ(node2->data.scan.has_match, true);
    ASSERT_EQ(node2->data.scan.match_var, 5);
    
    ast_free(node2);
    return true;
}

static bool test_make_join() {
    ASTNode *node = ast_make_join("target", 42, 5, 3);
    
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(node->type, AST_JOIN);
    ASSERT_STR_EQ(node->data.join.relation, "target");
    ASSERT_EQ(node->data.join.match_var, 42);
    ASSERT_EQ(node->line, 5);
    ASSERT_EQ(node->column, 3);
    
    ast_free(node);
    return true;
}

static bool test_make_emit() {
    ASTNode *node = ast_make_emit("result", 0, 1, 6, 8);
    
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(node->type, AST_EMIT);
    ASSERT_STR_EQ(node->data.emit.relation, "result");
    ASSERT_EQ(node->data.emit.var_a, 0);
    ASSERT_EQ(node->data.emit.var_b, 1);
    ASSERT_EQ(node->line, 6);
    ASSERT_EQ(node->column, 8);
    
    ast_free(node);
    return true;
}

static bool test_make_rule() {
    /* Create body operations */
    ASTNode *scan = ast_make_scan("parent", false, -1, 1, 1);
    ASTNode *join = ast_make_join("ancestor", 1, 1, 2);
    ASTNode *body = ast_append(scan, join);
    
    /* Create emit */
    ASTNode *emit = ast_make_emit("ancestor", 0, 2, 1, 3);
    
    /* Create rule */
    ASTNode *rule = ast_make_rule("ancestor", body, emit, 7, 1);
    
    ASSERT_NOT_NULL(rule);
    ASSERT_EQ(rule->type, AST_RULE);
    ASSERT_STR_EQ(rule->data.rule.target, "ancestor");
    ASSERT_NOT_NULL(rule->data.rule.body);
    ASSERT_NOT_NULL(rule->data.rule.emit);
    ASSERT_EQ(rule->line, 7);
    ASSERT_EQ(rule->column, 1);
    
    /* Verify body structure */
    ASSERT_EQ(rule->data.rule.body->type, AST_SCAN);
    ASSERT_EQ(rule->data.rule.body->next->type, AST_JOIN);
    
    /* Verify emit */
    ASSERT_EQ(rule->data.rule.emit->type, AST_EMIT);
    
    ast_free_tree(rule);
    return true;
}

static bool test_make_solve() {
    ASTNode *node = ast_make_solve(8, 1);
    
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(node->type, AST_SOLVE);
    ASSERT_EQ(node->line, 8);
    ASSERT_EQ(node->column, 1);
    
    ast_free(node);
    return true;
}

static bool test_make_query() {
    /* Test query with concrete values */
    ASTNode *query1 = ast_make_query("parent", 0, 1, 9, 1);
    
    ASSERT_NOT_NULL(query1);
    ASSERT_EQ(query1->type, AST_QUERY);
    ASSERT_STR_EQ(query1->data.query.relation, "parent");
    ASSERT_EQ(query1->data.query.arg_a, 0);
    ASSERT_EQ(query1->data.query.arg_b, 1);
    
    ast_free(query1);
    
    /* Test query with wildcards */
    ASTNode *query2 = ast_make_query("ancestor", -1, -1, 10, 1);
    
    ASSERT_NOT_NULL(query2);
    ASSERT_EQ(query2->type, AST_QUERY);
    ASSERT_STR_EQ(query2->data.query.relation, "ancestor");
    ASSERT_EQ(query2->data.query.arg_a, -1);  /* wildcard */
    ASSERT_EQ(query2->data.query.arg_b, -1);  /* wildcard */
    
    ast_free(query2);
    return true;
}

static bool test_make_program() {
    /* Create some statements */
    ASTNode *rel1 = ast_make_rel_decl("parent", 1, 1);
    ASTNode *rel2 = ast_make_rel_decl("child", 2, 1);
    ASTNode *fact = ast_make_fact("parent", 0, 1, 3, 1);
    ASTNode *solve = ast_make_solve(4, 1);
    
    /* Build statement list */
    ASTNode *statements = NULL;
    statements = ast_append(statements, rel1);
    statements = ast_append(statements, rel2);
    statements = ast_append(statements, fact);
    statements = ast_append(statements, solve);
    
    /* Create program */
    ASTNode *program = ast_make_program(statements);
    
    ASSERT_NOT_NULL(program);
    ASSERT_EQ(program->type, AST_PROGRAM);
    ASSERT_NOT_NULL(program->data.program.statements);
    
    /* Verify statement chain */
    ASTNode *stmt = program->data.program.statements;
    ASSERT_EQ(stmt->type, AST_REL_DECL);
    
    stmt = stmt->next;
    ASSERT_EQ(stmt->type, AST_REL_DECL);
    
    stmt = stmt->next;
    ASSERT_EQ(stmt->type, AST_FACT);
    
    stmt = stmt->next;
    ASSERT_EQ(stmt->type, AST_SOLVE);
    
    ASSERT_NULL(stmt->next);
    
    ast_free_tree(program);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * AST Manipulation Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_ast_append() {
    ASTNode *node1 = ast_make_rel_decl("first", 1, 1);
    ASTNode *node2 = ast_make_rel_decl("second", 2, 1);
    ASTNode *node3 = ast_make_rel_decl("third", 3, 1);
    
    /* Test appending to NULL */
    ASTNode *list = ast_append(NULL, node1);
    ASSERT_EQ(list, node1);
    ASSERT_NULL(node1->next);
    
    /* Test appending to existing list */
    list = ast_append(list, node2);
    ASSERT_EQ(list, node1);
    ASSERT_EQ(node1->next, node2);
    ASSERT_NULL(node2->next);
    
    /* Test appending third node */
    list = ast_append(list, node3);
    ASSERT_EQ(list, node1);
    ASSERT_EQ(node1->next, node2);
    ASSERT_EQ(node2->next, node3);
    ASSERT_NULL(node3->next);
    
    /* Test appending NULL */
    ASTNode *original_list = list;
    list = ast_append(list, NULL);
    ASSERT_EQ(list, original_list);
    
    ast_free_tree(list);
    return true;
}

static bool test_ast_count_nodes() {
    /* Test empty list */
    ASSERT_EQ(ast_count_nodes(NULL), 0);
    
    /* Test single node */
    ASTNode *single = ast_make_solve(1, 1);
    ASSERT_EQ(ast_count_nodes(single), 1);
    ast_free(single);
    
    /* Test multiple nodes */
    ASTNode *list = NULL;
    list = ast_append(list, ast_make_rel_decl("a", 1, 1));
    list = ast_append(list, ast_make_rel_decl("b", 2, 1));
    list = ast_append(list, ast_make_rel_decl("c", 3, 1));
    list = ast_append(list, ast_make_solve(4, 1));
    
    ASSERT_EQ(ast_count_nodes(list), 4);
    
    ast_free_tree(list);
    return true;
}

static bool test_ast_get_nth() {
    /* Create list of nodes */
    ASTNode *list = NULL;
    ASTNode *rel_a = ast_make_rel_decl("a", 1, 1);
    ASTNode *rel_b = ast_make_rel_decl("b", 2, 1);
    ASTNode *rel_c = ast_make_rel_decl("c", 3, 1);
    
    list = ast_append(list, rel_a);
    list = ast_append(list, rel_b);
    list = ast_append(list, rel_c);
    
    /* Test getting nodes by index */
    ASSERT_EQ(ast_get_nth(list, 0), rel_a);
    ASSERT_EQ(ast_get_nth(list, 1), rel_b);
    ASSERT_EQ(ast_get_nth(list, 2), rel_c);
    
    /* Test out of bounds */
    ASSERT_NULL(ast_get_nth(list, 3));
    ASSERT_NULL(ast_get_nth(list, 100));
    
    /* Test NULL list */
    ASSERT_NULL(ast_get_nth(NULL, 0));
    
    ast_free_tree(list);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * AST Utility Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_ast_node_type_name() {
    ASSERT_STR_EQ(ast_node_type_name(AST_PROGRAM), "PROGRAM");
    ASSERT_STR_EQ(ast_node_type_name(AST_REL_DECL), "REL_DECL");
    ASSERT_STR_EQ(ast_node_type_name(AST_FACT), "FACT");
    ASSERT_STR_EQ(ast_node_type_name(AST_RULE), "RULE");
    ASSERT_STR_EQ(ast_node_type_name(AST_SCAN), "SCAN");
    ASSERT_STR_EQ(ast_node_type_name(AST_JOIN), "JOIN");
    ASSERT_STR_EQ(ast_node_type_name(AST_EMIT), "EMIT");
    ASSERT_STR_EQ(ast_node_type_name(AST_SOLVE), "SOLVE");
    ASSERT_STR_EQ(ast_node_type_name(AST_QUERY), "QUERY");
    
    return true;
}

static bool test_ast_clone() {
    /* Create original node */
    ASTNode *original = ast_make_fact("parent", 42, -17, 5, 10);
    
    /* Clone it */
    ASTNode *clone = ast_clone(original);
    
    ASSERT_NOT_NULL(clone);
    ASSERT_EQ(clone->type, original->type);
    ASSERT_STR_EQ(clone->data.fact.relation, original->data.fact.relation);
    ASSERT_EQ(clone->data.fact.a, original->data.fact.a);
    ASSERT_EQ(clone->data.fact.b, original->data.fact.b);
    ASSERT_EQ(clone->line, original->line);
    ASSERT_EQ(clone->column, original->column);
    
    /* Verify they're different objects */
    ASSERT(clone != original);
    ASSERT(clone->data.fact.relation != original->data.fact.relation);
    
    ast_free(original);
    ast_free(clone);
    return true;
}

static bool test_ast_clone_list() {
    /* Create list */
    ASTNode *list = NULL;
    list = ast_append(list, ast_make_rel_decl("parent", 1, 1));
    list = ast_append(list, ast_make_fact("parent", 0, 1, 2, 1));
    list = ast_append(list, ast_make_solve(3, 1));
    
    /* Clone list */
    ASTNode *clone_list = ast_clone(list);
    
    ASSERT_NOT_NULL(clone_list);
    ASSERT_EQ(ast_count_nodes(clone_list), 3);
    
    /* Verify structure */
    ASTNode *clone_node = clone_list;
    ASTNode *orig_node = list;
    
    while (orig_node && clone_node) {
        ASSERT_EQ(clone_node->type, orig_node->type);
        ASSERT(clone_node != orig_node);  /* Different objects */
        
        clone_node = clone_node->next;
        orig_node = orig_node->next;
    }
    
    ASSERT_NULL(clone_node);
    ASSERT_NULL(orig_node);
    
    ast_free_tree(list);
    ast_free_tree(clone_list);
    return true;
}

static bool test_ast_clone_rule() {
    /* Create complex rule */
    ASTNode *scan = ast_make_scan("parent", true, 5, 1, 1);
    ASTNode *join = ast_make_join("ancestor", 1, 1, 2);
    ASTNode *body = ast_append(scan, join);
    ASTNode *emit = ast_make_emit("ancestor", 0, 2, 1, 3);
    ASTNode *rule = ast_make_rule("ancestor", body, emit, 1, 1);
    
    /* Clone rule */
    ASTNode *clone = ast_clone(rule);
    
    ASSERT_NOT_NULL(clone);
    ASSERT_EQ(clone->type, AST_RULE);
    ASSERT_STR_EQ(clone->data.rule.target, "ancestor");
    
    /* Verify body is cloned */
    ASSERT_NOT_NULL(clone->data.rule.body);
    ASSERT(clone->data.rule.body != rule->data.rule.body);
    ASSERT_EQ(clone->data.rule.body->type, AST_SCAN);
    
    /* Verify emit is cloned */
    ASSERT_NOT_NULL(clone->data.rule.emit);
    ASSERT(clone->data.rule.emit != rule->data.rule.emit);
    ASSERT_EQ(clone->data.rule.emit->type, AST_EMIT);
    
    ast_free_tree(rule);
    ast_free_tree(clone);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * AST Visitor Tests
 * ───────────────────────────────────────────────────────────────────────── */

typedef struct {
    int rel_decl_count;
    int fact_count;
    int rule_count;
    int scan_count;
    int join_count;
    int emit_count;
    int solve_count;
    int query_count;
} VisitorContext;

static void count_rel_decl(const ASTNode *node, void *context) {
    VisitorContext *ctx = (VisitorContext*)context;
    ctx->rel_decl_count++;
}

static void count_fact(const ASTNode *node, void *context) {
    VisitorContext *ctx = (VisitorContext*)context;
    ctx->fact_count++;
}

static void count_rule(const ASTNode *node, void *context) {
    VisitorContext *ctx = (VisitorContext*)context;
    ctx->rule_count++;
}

static void count_scan(const ASTNode *node, void *context) {
    VisitorContext *ctx = (VisitorContext*)context;
    ctx->scan_count++;
}

static void count_join(const ASTNode *node, void *context) {
    VisitorContext *ctx = (VisitorContext*)context;
    ctx->join_count++;
}

static void count_emit(const ASTNode *node, void *context) {
    VisitorContext *ctx = (VisitorContext*)context;
    ctx->emit_count++;
}

static void count_solve(const ASTNode *node, void *context) {
    VisitorContext *ctx = (VisitorContext*)context;
    ctx->solve_count++;
}

static void count_query(const ASTNode *node, void *context) {
    VisitorContext *ctx = (VisitorContext*)context;
    ctx->query_count++;
}

static bool test_ast_visitor() {
    /* Create program with various nodes */
    ASTNode *rel1 = ast_make_rel_decl("parent", 1, 1);
    ASTNode *rel2 = ast_make_rel_decl("ancestor", 2, 1);
    ASTNode *fact1 = ast_make_fact("parent", 0, 1, 3, 1);
    ASTNode *fact2 = ast_make_fact("parent", 1, 2, 4, 1);
    
    /* Create rule */
    ASTNode *scan = ast_make_scan("parent", false, -1, 5, 1);
    ASTNode *join = ast_make_join("ancestor", 1, 5, 2);
    ASTNode *body = ast_append(scan, join);
    ASTNode *emit = ast_make_emit("ancestor", 0, 2, 5, 3);
    ASTNode *rule = ast_make_rule("ancestor", body, emit, 5, 1);
    
    ASTNode *solve = ast_make_solve(6, 1);
    ASTNode *query = ast_make_query("ancestor", 0, -1, 7, 1);
    
    /* Build program */
    ASTNode *statements = NULL;
    statements = ast_append(statements, rel1);
    statements = ast_append(statements, rel2);
    statements = ast_append(statements, fact1);
    statements = ast_append(statements, fact2);
    statements = ast_append(statements, rule);
    statements = ast_append(statements, solve);
    statements = ast_append(statements, query);
    
    ASTNode *program = ast_make_program(statements);
    
    /* Set up visitor */
    ASTVisitor visitor = {0};
    visitor.visit_rel_decl = count_rel_decl;
    visitor.visit_fact = count_fact;
    visitor.visit_rule = count_rule;
    visitor.visit_scan = count_scan;
    visitor.visit_join = count_join;
    visitor.visit_emit = count_emit;
    visitor.visit_solve = count_solve;
    visitor.visit_query = count_query;
    
    VisitorContext context = {0};
    
    /* Walk the AST */
    ast_walk(program, &visitor, &context);
    
    /* Verify counts */
    ASSERT_EQ(context.rel_decl_count, 2);
    ASSERT_EQ(context.fact_count, 2);
    ASSERT_EQ(context.rule_count, 1);
    ASSERT_EQ(context.scan_count, 1);
    ASSERT_EQ(context.join_count, 1);
    ASSERT_EQ(context.emit_count, 1);
    ASSERT_EQ(context.solve_count, 1);
    ASSERT_EQ(context.query_count, 1);
    
    ast_free_tree(program);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * AST Validation Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_ast_validate_valid_program() {
    /* Create valid program */
    ASTNode *statements = ast_make_solve(1, 1);
    ASTNode *program = ast_make_program(statements);
    
    char error_buf[256];
    bool valid = ast_validate(program, error_buf, sizeof(error_buf));
    
    ASSERT(valid);
    
    ast_free_tree(program);
    return true;
}

static bool test_ast_validate_null_program() {
    char error_buf[256];
    bool valid = ast_validate(NULL, error_buf, sizeof(error_buf));
    
    ASSERT(!valid);
    ASSERT_STR_EQ(error_buf, "Empty AST");
    
    return true;
}

static bool test_ast_validate_wrong_root_type() {
    /* Create non-program root */
    ASTNode *solve = ast_make_solve(1, 1);
    
    char error_buf[256];
    bool valid = ast_validate(solve, error_buf, sizeof(error_buf));
    
    ASSERT(!valid);
    ASSERT_STR_EQ(error_buf, "Root must be PROGRAM node");
    
    ast_free(solve);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Memory Management Tests
 * ───────────────────────────────────────────────────────────────────────── */

static bool test_memory_management() {
    /* Test that we can create and free many nodes without issues */
    for (int i = 0; i < 1000; i++) {
        ASTNode *node = ast_make_fact("relation", i, i + 1, 1, 1);
        ASSERT_NOT_NULL(node);
        ast_free(node);
    }
    
    /* Test creating and freeing complex tree */
    ASTNode *list = NULL;
    for (int i = 0; i < 100; i++) {
        ASTNode *rel = ast_make_rel_decl("relation", i, 1);
        list = ast_append(list, rel);
    }
    
    ASSERT_EQ(ast_count_nodes(list), 100);
    ast_free_tree(list);
    
    return true;
}

static bool test_null_string_handling() {
    /* Test that NULL strings are handled gracefully */
    ASTNode *node = ast_make_rel_decl(NULL, 1, 1);
    ASSERT_NOT_NULL(node);
    ASSERT_NULL(node->data.rel_decl.name);
    
    ast_free(node);
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Main Test Runner
 * ───────────────────────────────────────────────────────────────────────── */

int main() {
    printf("ByteLog AST Unit Tests\n");
    printf("═══════════════════════════════════════\n\n");
    
    /* Node creation tests */
    printf("Testing AST node creation:\n");
    TEST(make_rel_decl);
    TEST(make_fact);
    TEST(make_scan);
    TEST(make_join);
    TEST(make_emit);
    TEST(make_rule);
    TEST(make_solve);
    TEST(make_query);
    TEST(make_program);
    printf("\n");
    
    /* AST manipulation tests */
    printf("Testing AST manipulation:\n");
    TEST(ast_append);
    TEST(ast_count_nodes);
    TEST(ast_get_nth);
    printf("\n");
    
    /* Utility function tests */
    printf("Testing AST utilities:\n");
    TEST(ast_node_type_name);
    TEST(ast_clone);
    TEST(ast_clone_list);
    TEST(ast_clone_rule);
    printf("\n");
    
    /* Visitor pattern tests */
    printf("Testing AST visitor pattern:\n");
    TEST(ast_visitor);
    printf("\n");
    
    /* Validation tests */
    printf("Testing AST validation:\n");
    TEST(ast_validate_valid_program);
    TEST(ast_validate_null_program);
    TEST(ast_validate_wrong_root_type);
    printf("\n");
    
    /* Memory management tests */
    printf("Testing memory management:\n");
    TEST(memory_management);
    TEST(null_string_handling);
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