/* ═══════════════════════════════════════════════════════════════════════════
 * demo.c - ByteLog Compiler Demo
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Demonstrates parsing a ByteLog program and displaying the AST.
 *
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "parser.h"
#include "ast.h"
#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *filename = "example_family.bl";
    
    if (argc > 1) {
        filename = argv[1];
    }
    
    printf("ByteLog Compiler Demo\n");
    printf("═══════════════════════════════════════\n\n");
    printf("Parsing file: %s\n\n", filename);
    
    /* Parse the ByteLog file */
    char error_buf[512];
    ASTNode *ast = parse_file(filename, error_buf, sizeof(error_buf));
    
    if (!ast) {
        printf("❌ Parse failed: %s\n", error_buf);
        return 1;
    }
    
    printf("✅ Parse successful!\n\n");
    
    /* Display the Abstract Syntax Tree */
    printf("Abstract Syntax Tree:\n");
    printf("─────────────────────────────────────────\n");
    ast_print_tree(ast);
    
    printf("\nAnalysis:\n");
    printf("─────────────────────────────────────────\n");
    
    /* Count different types of statements */
    int rel_count = 0, fact_count = 0, rule_count = 0;
    int solve_count = 0, query_count = 0;
    
    ASTNode *stmt = ast->data.program.statements;
    while (stmt) {
        switch (stmt->type) {
            case AST_REL_DECL: rel_count++; break;
            case AST_FACT: fact_count++; break;
            case AST_RULE: rule_count++; break;
            case AST_SOLVE: solve_count++; break;
            case AST_QUERY: query_count++; break;
            default: break;
        }
        stmt = stmt->next;
    }
    
    printf("Relations declared: %d\n", rel_count);
    printf("Facts asserted: %d\n", fact_count);
    printf("Rules defined: %d\n", rule_count);
    printf("Solve statements: %d\n", solve_count);
    printf("Queries: %d\n", query_count);
    
    /* Show what the program does */
    printf("\nProgram Logic:\n");
    printf("─────────────────────────────────────────\n");
    
    stmt = ast->data.program.statements;
    while (stmt) {
        switch (stmt->type) {
            case AST_REL_DECL:
                printf("• Declares relation '%s'\n", stmt->data.rel_decl.name);
                break;
                
            case AST_FACT:
                printf("• Asserts fact: %s(%d, %d)\n", 
                       stmt->data.fact.relation, 
                       stmt->data.fact.a, 
                       stmt->data.fact.b);
                break;
                
            case AST_RULE:
                printf("• Defines rule for '%s'\n", stmt->data.rule.target);
                break;
                
            case AST_SOLVE:
                printf("• Computes fixpoint (derives all facts)\n");
                break;
                
            case AST_QUERY:
                if (stmt->data.query.arg_a != -1 && stmt->data.query.arg_b != -1) {
                    printf("• Queries: Is %s(%d, %d) true?\n",
                           stmt->data.query.relation,
                           stmt->data.query.arg_a, 
                           stmt->data.query.arg_b);
                } else if (stmt->data.query.arg_a != -1) {
                    printf("• Queries: All Y where %s(%d, Y)\n",
                           stmt->data.query.relation,
                           stmt->data.query.arg_a);
                } else if (stmt->data.query.arg_b != -1) {
                    printf("• Queries: All X where %s(X, %d)\n",
                           stmt->data.query.relation,
                           stmt->data.query.arg_b);
                } else {
                    printf("• Queries: All facts in %s\n",
                           stmt->data.query.relation);
                }
                break;
                
            default:
                break;
        }
        stmt = stmt->next;
    }
    
    /* Execute the program */
    printf("\nExecution:\n");
    printf("─────────────────────────────────────────\n");
    
    ExecutionEngine *engine = malloc(sizeof(ExecutionEngine));
    if (!engine) {
        printf("❌ Out of memory\n");
        ast_free_tree(ast);
        return 1;
    }
    
    engine_init(engine);
    engine_set_debug(engine, false);  /* Set to true for detailed execution trace */
    
    if (!engine_execute_program(engine, ast)) {
        printf("❌ Execution failed: %s\n", engine_get_error(engine));
        engine_cleanup(engine);
        free(engine);
        ast_free_tree(ast);
        return 1;
    }
    
    printf("✅ Execution successful!\n\n");
    
    /* Show the derived facts */
    printf("Derived Facts:\n");
    printf("─────────────────────────────────────────\n");
    factdb_print(&engine->facts, &engine->atoms);
    
    /* Answer all queries in the program */
    printf("\nQuery Results:\n");
    printf("─────────────────────────────────────────\n");
    
    stmt = ast->data.program.statements;
    int query_num = 1;
    while (stmt) {
        if (stmt->type == AST_QUERY) {
            printf("Query %d: ", query_num++);
            
            /* Print the query */
            if (stmt->data.query.atom_a && stmt->data.query.atom_b) {
                printf("%s(%s, %s)\n", stmt->data.query.relation,
                       stmt->data.query.arg_a == -1 ? "?" : stmt->data.query.atom_a,
                       stmt->data.query.arg_b == -1 ? "?" : stmt->data.query.atom_b);
            } else if (stmt->data.query.atom_a) {
                printf("%s(%s, %s)\n", stmt->data.query.relation,
                       stmt->data.query.atom_a,
                       stmt->data.query.arg_b == -1 ? "?" : "?");
            } else if (stmt->data.query.atom_b) {
                printf("%s(%s, %s)\n", stmt->data.query.relation,
                       stmt->data.query.arg_a == -1 ? "?" : "?",
                       stmt->data.query.atom_b);
            } else {
                printf("%s(?, ?)\n", stmt->data.query.relation);
            }
            
            /* Execute the query */
            QueryResult *results = engine_query(engine, stmt);
            if (results) {
                query_result_print(results, stmt->data.query.relation, &engine->atoms);
                query_result_free(results);
            } else {
                printf("  No results found.\n");
            }
            printf("\n");
        }
        stmt = stmt->next;
    }
    
    printf("🎯 ByteLog program executed successfully!\n");
    
    /* Clean up */
    engine_cleanup(engine);
    free(engine);
    ast_free_tree(ast);
    
    return 0;
}