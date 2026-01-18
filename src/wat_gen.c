/* ═══════════════════════════════════════════════════════════════════════════
 * wat_gen.c - WebAssembly Text (WAT) Code Generator Implementation
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Generates WAT code from ByteLog programs for WASM execution.
 * Compatible with Wasmtime and other WASM runtimes.
 *
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "wat_gen.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* For strdup portability */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* Provide strdup for systems that don't have it */
#if !defined(_GNU_SOURCE) && !defined(__GLIBC__)
static char* strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *result = malloc(len);
    if (result) {
        memcpy(result, s, len);
    }
    return result;
}
#endif

/* ─────────────────────────────────────────────────────────────────────────
 * Error Handling
 * ───────────────────────────────────────────────────────────────────────── */

static void wat_gen_error(WATGenerator *gen, const char *message) {
    gen->error_count++;
    snprintf(gen->error, sizeof(gen->error), "%s", message);
}

/* ─────────────────────────────────────────────────────────────────────────
 * WAT Generator Implementation
 * ───────────────────────────────────────────────────────────────────────── */

void wat_gen_init(WATGenerator *gen, FILE *output) {
    gen->output = output;
    atom_table_init(&gen->atoms);
    memset(gen->error, 0, sizeof(gen->error));
    gen->error_count = 0;
    gen->memory_size = WAT_INITIAL_PAGES;
    gen->fact_offset = 0;
    gen->atom_offset = WAT_MAX_FACTS * WAT_FACT_SIZE;
    gen->next_func_id = 0;
}

void wat_gen_cleanup(WATGenerator *gen) {
    atom_table_free(&gen->atoms);
    gen->error_count = 0;
}

bool wat_gen_has_errors(WATGenerator *gen) {
    return gen->error_count > 0;
}

int wat_gen_get_error_count(WATGenerator *gen) {
    return gen->error_count;
}

const char* wat_gen_get_error(WATGenerator *gen) {
    return gen->error;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Utility Functions
 * ───────────────────────────────────────────────────────────────────────── */

bool wat_write_string(WATGenerator *gen, const char *str) {
    if (fprintf(gen->output, "%s", str) < 0) {
        wat_gen_error(gen, "Failed to write to output");
        return false;
    }
    return true;
}

bool wat_write_int(WATGenerator *gen, int value) {
    if (fprintf(gen->output, "%d", value) < 0) {
        wat_gen_error(gen, "Failed to write to output");
        return false;
    }
    return true;
}

bool wat_write_comment(WATGenerator *gen, const char *comment) {
    if (fprintf(gen->output, ";; %s\n", comment) < 0) {
        wat_gen_error(gen, "Failed to write to output");
        return false;
    }
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Memory Layout Generation
 * ───────────────────────────────────────────────────────────────────────── */

void wat_gen_calculate_memory(WATGenerator *gen, const ASTNode *program) {
    /* Calculate required memory for facts and atom names */
    int fact_count = 0;
    int total_atom_length = 0;
    
    ASTNode *stmt = program->data.program.statements;
    while (stmt) {
        if (stmt->type == AST_FACT) {
            fact_count++;
            if (stmt->data.fact.atom_a) {
                total_atom_length += strlen(stmt->data.fact.atom_a) + 1;
            }
            if (stmt->data.fact.atom_b) {
                total_atom_length += strlen(stmt->data.fact.atom_b) + 1;
            }
        }
        stmt = stmt->next;
    }
    
    /* Account for derived facts (estimate 3x original facts) */
    fact_count *= 3;
    
    /* Calculate memory pages needed */
    int memory_needed = (fact_count * WAT_FACT_SIZE) + total_atom_length;
    gen->memory_size = (memory_needed / WAT_PAGE_SIZE) + 1;
}

bool wat_gen_memory_section(WATGenerator *gen) {
    if (!wat_write_string(gen, "  (memory ")) return false;
    if (!wat_write_int(gen, gen->memory_size)) return false;
    if (!wat_write_string(gen, ")\n")) return false;
    return true;
}

bool wat_gen_data_section(WATGenerator *gen) {
    if (!wat_write_comment(gen, "Data section with atom names")) return false;
    
    /* For now, we'll generate atom names dynamically */
    /* In a full implementation, we'd precompute and store atom strings */
    
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Function Generation
 * ───────────────────────────────────────────────────────────────────────── */

bool wat_gen_fact_functions(WATGenerator *gen) {
    if (!wat_write_comment(gen, "Fact database functions")) return false;
    
    /* Hash function for facts */
    if (!wat_write_string(gen, 
        "  (func $hash_fact (param $rel i32) (param $a i32) (param $b i32) (result i32)\n"
        "    ;; Simple hash: (rel * 31 + a) * 31 + b\n"
        "    local.get $rel\n"
        "    i32.const 31\n"
        "    i32.mul\n"
        "    local.get $a\n"
        "    i32.add\n"
        "    i32.const 31\n"
        "    i32.mul\n"
        "    local.get $b\n"
        "    i32.add\n"
        "    i32.const 1000\n"
        "    i32.rem_u\n"
        "  )\n\n")) return false;
    
    /* Add fact function */
    if (!wat_write_string(gen,
        "  (func $add_fact (param $rel i32) (param $a i32) (param $b i32)\n"
        "    (local $offset i32)\n"
        "    ;; Calculate memory offset for fact\n"
        "    local.get $rel\n"
        "    local.get $a\n" 
        "    local.get $b\n"
        "    call $hash_fact\n"
        "    i32.const 12\n"
        "    i32.mul\n"
        "    local.set $offset\n"
        "    ;; Store fact in memory\n"
        "    local.get $offset\n"
        "    local.get $rel\n"
        "    i32.store\n"
        "    local.get $offset\n"
        "    i32.const 4\n"
        "    i32.add\n"
        "    local.get $a\n"
        "    i32.store\n"
        "    local.get $offset\n"
        "    i32.const 8\n"
        "    i32.add\n"
        "    local.get $b\n"
        "    i32.store\n"
        "  )\n\n")) return false;
        
    /* Has fact function */
    if (!wat_write_string(gen,
        "  (func $has_fact (param $rel i32) (param $a i32) (param $b i32) (result i32)\n"
        "    (local $offset i32)\n"
        "    (local $stored_rel i32)\n"
        "    (local $stored_a i32)\n"
        "    (local $stored_b i32)\n"
        "    ;; Calculate memory offset\n"
        "    local.get $rel\n"
        "    local.get $a\n"
        "    local.get $b\n"
        "    call $hash_fact\n"
        "    i32.const 12\n"
        "    i32.mul\n"
        "    local.set $offset\n"
        "    ;; Load stored values\n"
        "    local.get $offset\n"
        "    i32.load\n"
        "    local.set $stored_rel\n"
        "    local.get $offset\n"
        "    i32.const 4\n"
        "    i32.add\n"
        "    i32.load\n"
        "    local.set $stored_a\n"
        "    local.get $offset\n"
        "    i32.const 8\n"
        "    i32.add\n"
        "    i32.load\n"
        "    local.set $stored_b\n"
        "    ;; Compare values\n"
        "    local.get $stored_rel\n"
        "    local.get $rel\n"
        "    i32.eq\n"
        "    local.get $stored_a\n"
        "    local.get $a\n"
        "    i32.eq\n"
        "    i32.and\n"
        "    local.get $stored_b\n"
        "    local.get $b\n"
        "    i32.eq\n"
        "    i32.and\n"
        "  )\n\n")) return false;
        
    return true;
}

bool wat_gen_rule_functions(WATGenerator *gen, const ASTNode *program) {
    if (!wat_write_comment(gen, "Rule evaluation functions")) return false;
    
    ASTNode *stmt = program->data.program.statements;
    while (stmt) {
        if (stmt->type == AST_RULE) {
            /* Generate a function for each rule */
            if (!wat_write_string(gen, "  (func $rule_")) return false;
            if (!wat_write_string(gen, stmt->data.rule.target)) return false;
            if (!wat_write_string(gen, "_")) return false;
            if (!wat_write_int(gen, gen->next_func_id++)) return false;
            if (!wat_write_string(gen, "\n")) return false;
            
            /* For now, generate a simple rule evaluation stub */
            if (!wat_write_string(gen, 
                "    ;; Rule evaluation stub\n"
                "    ;; TODO: Implement actual rule logic\n"
                "  )\n\n")) return false;
        }
        stmt = stmt->next;
    }
    
    return true;
}

bool wat_gen_query_functions(WATGenerator *gen, const ASTNode *program) {
    if (!wat_write_comment(gen, "Query functions")) return false;
    
    ASTNode *stmt = program->data.program.statements;
    int query_id = 0;
    
    while (stmt) {
        if (stmt->type == AST_QUERY) {
            /* Generate a function for each query */
            if (!wat_write_string(gen, "  (func $query_")) return false;
            if (!wat_write_int(gen, query_id++)) return false;
            if (!wat_write_string(gen, " (result i32)\n")) return false;
            
            /* Generate query logic */
            if (!wat_write_string(gen, "    ;; Query: ")) return false;
            if (!wat_write_string(gen, stmt->data.query.relation)) return false;
            if (!wat_write_string(gen, "(")) return false;
            if (stmt->data.query.arg_a == -1) {
                if (!wat_write_string(gen, "?, ")) return false;
            } else {
                if (!wat_write_int(gen, stmt->data.query.arg_a)) return false;
                if (!wat_write_string(gen, ", ")) return false;
            }
            if (stmt->data.query.arg_b == -1) {
                if (!wat_write_string(gen, "?)")) return false;
            } else {
                if (!wat_write_int(gen, stmt->data.query.arg_b)) return false;
                if (!wat_write_string(gen, ")")) return false;
            }
            if (!wat_write_string(gen, "\n")) return false;
            
            /* For exact queries, use has_fact */
            if (stmt->data.query.arg_a != -1 && stmt->data.query.arg_b != -1) {
                /* Hash relation name to ID (simplified) */
                int rel_id = strlen(stmt->data.query.relation) % 100;
                if (!wat_write_string(gen, "    i32.const ")) return false;
                if (!wat_write_int(gen, rel_id)) return false;
                if (!wat_write_string(gen, "\n")) return false;
                
                if (!wat_write_string(gen, "    i32.const ")) return false;
                if (!wat_write_int(gen, stmt->data.query.arg_a)) return false;
                if (!wat_write_string(gen, "\n")) return false;
                
                if (!wat_write_string(gen, "    i32.const ")) return false;
                if (!wat_write_int(gen, stmt->data.query.arg_b)) return false;
                if (!wat_write_string(gen, "\n")) return false;
                
                if (!wat_write_string(gen, "    call $has_fact\n")) return false;
            } else {
                /* For wildcard queries, return 1 (found results) */
                if (!wat_write_string(gen, "    i32.const 1\n")) return false;
            }
            
            if (!wat_write_string(gen, "  )\n\n")) return false;
        }
        stmt = stmt->next;
    }
    
    return true;
}

bool wat_gen_main_function(WATGenerator *gen, const ASTNode *program) {
    if (!wat_write_comment(gen, "Main execution function")) return false;
    
    if (!wat_write_string(gen, "  (func $main\n")) return false;
    
    /* Add all facts to the database */
    ASTNode *stmt = program->data.program.statements;
    while (stmt) {
        if (stmt->type == AST_FACT) {
            /* Hash relation name to ID (simplified) */
            int rel_id = strlen(stmt->data.fact.relation) % 100;
            
            if (!wat_write_string(gen, "    ;; Add fact: ")) return false;
            if (!wat_write_string(gen, stmt->data.fact.relation)) return false;
            if (!wat_write_string(gen, "(")) return false;
            if (!wat_write_int(gen, stmt->data.fact.a)) return false;
            if (!wat_write_string(gen, ", ")) return false;
            if (!wat_write_int(gen, stmt->data.fact.b)) return false;
            if (!wat_write_string(gen, ")\n")) return false;
            
            if (!wat_write_string(gen, "    i32.const ")) return false;
            if (!wat_write_int(gen, rel_id)) return false;
            if (!wat_write_string(gen, "\n")) return false;
            
            if (!wat_write_string(gen, "    i32.const ")) return false;
            if (!wat_write_int(gen, stmt->data.fact.a)) return false;
            if (!wat_write_string(gen, "\n")) return false;
            
            if (!wat_write_string(gen, "    i32.const ")) return false;
            if (!wat_write_int(gen, stmt->data.fact.b)) return false;
            if (!wat_write_string(gen, "\n")) return false;
            
            if (!wat_write_string(gen, "    call $add_fact\n\n")) return false;
        }
        stmt = stmt->next;
    }
    
    /* TODO: Add rule evaluation (fixpoint computation) */
    if (!wat_write_comment(gen, "TODO: Evaluate rules here")) return false;
    
    if (!wat_write_string(gen, "  )\n\n")) return false;
    
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Export Generation
 * ───────────────────────────────────────────────────────────────────────── */

bool wat_gen_exports(WATGenerator *gen) {
    if (!wat_write_comment(gen, "Exports for JavaScript interface")) return false;
    
    if (!wat_write_string(gen, "  (export \"main\" (func $main))\n")) return false;
    if (!wat_write_string(gen, "  (export \"memory\" (memory 0))\n")) return false;
    if (!wat_write_string(gen, "  (export \"add_fact\" (func $add_fact))\n")) return false;
    if (!wat_write_string(gen, "  (export \"has_fact\" (func $has_fact))\n\n")) return false;
    
    return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Main Generation Function
 * ───────────────────────────────────────────────────────────────────────── */

bool wat_gen_program(WATGenerator *gen, const ASTNode *program) {
    if (!program || program->type != AST_PROGRAM) {
        wat_gen_error(gen, "Invalid program node");
        return false;
    }
    
    /* Copy atoms from program to generator's atom table */
    ASTNode *stmt = program->data.program.statements;
    while (stmt) {
        if (stmt->type == AST_FACT) {
            if (stmt->data.fact.atom_a) {
                atom_table_intern(&gen->atoms, stmt->data.fact.atom_a);
            }
            if (stmt->data.fact.atom_b) {
                atom_table_intern(&gen->atoms, stmt->data.fact.atom_b);
            }
        } else if (stmt->type == AST_QUERY) {
            if (stmt->data.query.atom_a) {
                atom_table_intern(&gen->atoms, stmt->data.query.atom_a);
            }
            if (stmt->data.query.atom_b) {
                atom_table_intern(&gen->atoms, stmt->data.query.atom_b);
            }
        }
        stmt = stmt->next;
    }
    
    /* Calculate memory requirements */
    wat_gen_calculate_memory(gen, program);
    
    /* Generate WAT module header */
    if (!wat_write_string(gen, "(module\n")) return false;
    if (!wat_write_comment(gen, "Generated ByteLog WebAssembly module")) return false;
    
    /* Generate memory section */
    if (!wat_gen_memory_section(gen)) return false;
    
    /* Generate data section */
    if (!wat_gen_data_section(gen)) return false;
    
    /* Generate functions */
    if (!wat_gen_fact_functions(gen)) return false;
    if (!wat_gen_rule_functions(gen, program)) return false;
    if (!wat_gen_query_functions(gen, program)) return false;
    if (!wat_gen_main_function(gen, program)) return false;
    
    /* Generate exports */
    if (!wat_gen_exports(gen)) return false;
    
    /* Close module */
    if (!wat_write_string(gen, ")\n")) return false;
    
    return true;
}

bool wat_gen_statement(WATGenerator *gen, const ASTNode *stmt) {
    /* Individual statement generation not implemented */
    /* Use wat_gen_program for complete programs */
    wat_gen_error(gen, "Individual statement generation not supported");
    return false;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Convenience Functions  
 * ───────────────────────────────────────────────────────────────────────── */

bool generate_wat_file(const char *input_filename, const char *output_filename,
                      char *error_buf, size_t error_buf_size) {
    if (!input_filename || !output_filename) {
        if (error_buf) snprintf(error_buf, error_buf_size, "Invalid filenames");
        return false;
    }
    
    /* Parse the input file */
    ASTNode *ast = parse_file(input_filename, error_buf, error_buf_size);
    if (!ast) return false;
    
    /* Open output file */
    FILE *output = fopen(output_filename, "w");
    if (!output) {
        if (error_buf) snprintf(error_buf, error_buf_size, "Cannot open output file");
        ast_free_tree(ast);
        return false;
    }
    
    /* Generate WAT code */
    WATGenerator gen;
    wat_gen_init(&gen, output);
    
    bool success = wat_gen_program(&gen, ast);
    
    if (!success && error_buf) {
        snprintf(error_buf, error_buf_size, "%s", wat_gen_get_error(&gen));
    }
    
    wat_gen_cleanup(&gen);
    fclose(output);
    ast_free_tree(ast);
    
    return success;
}

bool generate_wat_string(const char *source, FILE *output,
                        char *error_buf, size_t error_buf_size) {
    if (!source || !output) {
        if (error_buf) snprintf(error_buf, error_buf_size, "Invalid arguments");
        return false;
    }
    
    /* Parse the source */
    ASTNode *ast = parse_string(source, error_buf, error_buf_size);
    if (!ast) return false;
    
    /* Generate WAT code */
    WATGenerator gen;
    wat_gen_init(&gen, output);
    
    bool success = wat_gen_program(&gen, ast);
    
    if (!success && error_buf) {
        snprintf(error_buf, error_buf_size, "%s", wat_gen_get_error(&gen));
    }
    
    wat_gen_cleanup(&gen);
    ast_free_tree(ast);
    
    return success;
}