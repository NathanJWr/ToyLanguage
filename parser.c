/*
    This file is part of Ample.

    Ample is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include "array.h"
#include "queue.h"
unsigned int statement_size (struct Statement s)
{
    return s.end - s.start + 1;
}

ASTHandle parse_tokens (struct Token* tokens)
{
    unsigned int index = 0;
    unsigned int head = ast_get_node_handle ();
    unsigned int* statements = NULL;
    while (index < ARRAY_COUNT (tokens)) {
        struct Statement s = parse__get_statement(tokens, &index);
        ARRAY_PUSH (statements, parse__statement (tokens, s));
    }

    struct AST* h = ast_get_node (head);
    *h = (struct AST) {
        .type = AST_SCOPE,
        .scope_data = {
            .statements = statements,
        }
    };
    return head;
}

struct Statement parse__get_statement (struct Token* restrict tokens, unsigned int* restrict index)
{
    unsigned int i = *index;
    struct Statement s = {0};
    while (i < ARRAY_COUNT (tokens) && tokens[i].value != STATEMENT_DELIM) {
        i++;
    }

    /* fill in statement info */
    s.start = *index;
    s.end = i - 1; /* don't care about the DELIM so the end if offset by 1 */

    *index = i + 1; /* offset by one to skip past the DELIM */
    return s;
}

ASTHandle parse__statement (struct Token* t_arr, struct Statement s)
{
    ASTHandle node = 0;
    node = parser__possible_assignment (t_arr, s);
    if (node) return node;

    node = parser__possible_integer (t_arr, s);
    if (node) return node;

    node = parser__possible_identifier (t_arr, s);
    if (node) return node;

    node = parser__possible_arithmetic (t_arr, s);
    if (node) return node;

    node = parser__possible_string (t_arr, s);
    if (node) return node;

    return 0;
}
ASTHandle parser__possible_integer (struct Token* t_arr, struct Statement s)
{
    ASTHandle node = 0;
    if (statement_size (s)  == 1 &&
        t_arr[s.start].value == TOK_INTEGER) { /* INTEGER literal */
        node = ast_get_node_handle ();
        struct AST* n = ast_get_node (node);
        *n = (struct AST) {
            .type = AST_INTEGER,
            .int_data = {
                .value = atoi (t_arr[s.start].string),
            },
        };
    }
    return node;
}

bool parser__is_arithmetic_op (TValue v)
{
    return (v == '+' || v == '-' || v == '/' || v == '*');
}

bool parser__greater_precedence (struct Token* left, struct Token* right)
{
    if ('*' == left->value) {
        if (right->value == '+' || right->value == '-')
            return true;
    } else if ('/' == left->value) {
        if (right->value == '+' || right->value == '-')
            return true;
    }
    return false;
    
}
bool parser__equal_precedence (struct Token* left, struct Token* right)
{
    if (('+' == left->value || '-' == left->value) &&
        ('+' == right->value || '-' == right->value)) {
        return true;
    }
    if (('*' == left->value || '/' == left->value) &&
        ('*' == right->value || '/' == right->value)) {
        return true;
    }
    return false;
}


QUEUE (TokenQueue) parser__convert_infix_to_postfix (QUEUE (TokenQueue)* expr_q)
{
    STACK (TokenStack) s = STACK_STRUCT_INIT (TokenStack, struct Token*, 10);
    QUEUE (TokenQueue) q = QUEUE_STRUCT_INIT (TokenQueue, struct Token*, 10);
    while (!QUEUE_EMPTY (expr_q)) {
        struct Token* n = QUEUE_FRONT (expr_q);
        QUEUE_POP (expr_q);

        if (n->value == TOK_INTEGER || n->value == TOK_IDENTIFIER) {
            QUEUE_PUSH (&q, n);
        }
        if (parser__is_arithmetic_op (n->value)) {
            if (!STACK_EMPTY (&s)) {
                struct Token* top_token = STACK_FRONT (&s);
                while ((parser__greater_precedence(top_token, n)) ||
                       ((parser__equal_precedence(top_token, n)) && (top_token->value != ')'))) {
                    STACK_POP (&s);
                    QUEUE_PUSH (&q, top_token);

                    if (STACK_EMPTY (&s)) break;
                    top_token = STACK_FRONT (&s);
                }
            }
            STACK_PUSH (&s, n);
        }
        if (n->value == '(') {
            STACK_PUSH (&s, n);
        }
        if (n->value == ')') {
            struct Token* tn = STACK_FRONT (&s);
            STACK_POP (&s);
            while (tn->value != '(') {
                QUEUE_PUSH (&q, tn);
                tn = STACK_FRONT (&s);
                STACK_POP (&s);
            }
        }
        if (n->value == STATEMENT_DELIM) break;
    }
    while (!STACK_EMPTY (&s)) {
        struct Token* tn = STACK_FRONT (&s);
        STACK_POP (&s);
        QUEUE_PUSH (&q, tn);
    }
    STACK_FREE (&s, TokenStack);
    QUEUE_FREE (expr_q, TokenQueue);
    return q;
}
ASTHandle parser__convert_postfix_to_ast (QUEUE (TokenQueue)* postfix_q, unsigned int expr_size)
{
    STACK (ASTHandleStack) s = STACK_STRUCT_INIT (ASTHandleStack, ASTHandle, 10);
    while (!QUEUE_EMPTY (postfix_q)) {
        struct Token* n = QUEUE_FRONT (postfix_q);
        QUEUE_POP (postfix_q);

        if (n->value == TOK_INTEGER) {
            ASTHandle ast_handle = ast_get_node_handle ();
            struct AST* integer_ast = ast_get_node (ast_handle);
            integer_ast->type = AST_INTEGER;
            integer_ast->int_data.value = atoi (n->string);

            STACK_PUSH (&s, ast_handle);
        } else if (parser__is_arithmetic_op (n->value)) {
            ASTHandle left, right, ast_handle;
            struct AST* op;

            /* Remove the first two elements from the stack.
               They will be the left and right params of a binary op */
            left = STACK_FRONT (&s);
            STACK_POP (&s);
            right = STACK_FRONT (&s);
            STACK_POP (&s);
            ast_handle = ast_get_node_handle ();
    
            /* Fill out information of binary op */
            op = ast_get_node (ast_handle);
            *op = (struct AST) {
                .type = AST_BINARY_OP,
                .bop_data = {
                    .left = left,
                    .right = right,
                    .op = n->value
                }
            };
            /* Insert the operation, instead of the left and right values */
            STACK_PUSH (&s, ast_handle);   
        }
    }
    ASTHandle ret = STACK_FRONT (&s);
    STACK_FREE (&s, ASTHandleStack);
    QUEUE_FREE (postfix_q, TokenQueue);
    return ret;
}

ASTHandle parser__arithmetic (struct Token* t_arr, struct Statement s)
{
    /* storage necessary to put tokens into a queue/stack 
       no more memory should be allocated for this process */
    QUEUE (TokenQueue) expr_q = QUEUE_STRUCT_INIT (TokenQueue, struct Token*, 10);
    for (unsigned int i = s.start; i <= s.end; i++) {
        QUEUE_PUSH (&expr_q, t_arr+i);
    }
    QUEUE (TokenQueue) postfix = parser__convert_infix_to_postfix (&expr_q);
    parser__debug_print_queue (&postfix);
    ASTHandle op = parser__convert_postfix_to_ast (&postfix, statement_size (s));
    return op;
}


void parser__debug_print_queue (QUEUE (TokenQueue)* q)
{
    struct Token* np;
    for (int i = q->head; i <= q->tail; i++) {
        np = q->mem[i];
        if (np->value < 128)
            printf ("Value: %c ", np->value);
        else 
            printf ("Value: %d ", np->value);
        if (np->string)
            printf("String: %s", np->string);
        printf("\n");
    }
}


ASTHandle parser__possible_arithmetic (struct Token* t_arr, struct Statement s)
{
    ASTHandle node = 0;
    if (statement_size (s) >= 2 &&
        t_arr[s.start].value == TOK_INTEGER &&
        parser__is_arithmetic_op (t_arr[s.start + 1].value)) {
        node = parser__arithmetic (t_arr, s);
    }
    return node;
}

ASTHandle parser__possible_identifier (struct Token* t_arr, struct Statement s)
{
    ASTHandle node = 0;
    if (statement_size (s) == 1 &&
        t_arr[s.start].value == TOK_IDENTIFIER) {
        node = ast_get_node_handle ();
        struct AST* n = ast_get_node (node);
        *n = (struct AST) {
            .type = AST_IDENTIFIER,
            .id_data = {
                .id = t_arr[s.start].string,
            }
        };
    }
    return node;
}

ASTHandle parser__possible_string (struct Token* t_arr, struct Statement s)
{
    ASTHandle node = 0;
    if (statement_size (s) == 1 &&
        t_arr[s.start].value == TOK_STRING) {
        node = ast_get_node_handle ();
        struct AST* n = ast_get_node (node);
        *n = (struct AST) {
            .type = AST_STRING,
            .str_data = {
                .str = t_arr[s.start].string,
            }
        };
    }
    return node;
}

ASTHandle parser__possible_assignment (struct Token* t_arr, struct Statement s)
{
    ASTHandle node = 0;
    /* VAR = EXPR */
    if (statement_size (s) >= 3 &&
        t_arr[s.start].value == TOK_IDENTIFIER &&
        t_arr[s.start+1].value == '=' &&
        t_arr[s.start+2].value != '=') {
        node = ast_get_node_handle ();

        struct Statement sub_statement = {
            .start = s.start + 2,
            .end = s.end,
        };
        struct AST ast =  {
            .type = AST_ASSIGNMENT,
            .asgn_data = {
                .var = t_arr[s.start].string,
                .expr = parse__statement (t_arr, sub_statement),
            }
        };
        struct AST* n = ast_get_node (node);
        *n = ast;
    }
    return node;
}