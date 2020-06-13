/*
    This file is part of Ample.

    Ample is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ample is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ample.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "interpreter.h"
#include "array.h"
#include "dict_vars.h"
#include "hash.h"
static DICT (IntVars) int_vars;
static DICT (StrVars) str_vars;
static DICT (Vars) var_types;

void
interpreter__erase_variable_if_exists (const char *var)
{
  /* erase variable type mapping */
  enum VarTypes type;
  if (!DictVars_get_and_erase (&var_types, var, &type))
    {
      printf ("Variable has no type\n");
      exit (1);
    }

  /* erase actual variable */
  switch (type)
    {
    case VAR_INTEGER:
      {
        IntVariable *int_var;
        bool success = DictIntVars_get_and_erase (&int_vars, var, &int_var);
        if (success)
          {
            obj_dec_refcount (int_var);
            return;
          }
        else
          {
            printf ("Type mapping and value mismatch\n");
            exit (1);
          }
      }
      break;
    case VAR_STRING:
      {
        StrVariable *str_var;
        bool success = DictStrVars_get_and_erase (&str_vars, var, &str_var);
        if (success)
          {
            obj_dec_refcount (str_var);
            return;
          }
        else
          {
            printf ("Type mapping and value mismatch\n");
            exit (1);
          }
      }
      break;
    }
}
void
interpreter__add_integer_variable (const char *var_name, int val)
{
  interpreter__erase_variable_if_exists (var_name);
  struct IntVariable *v = variable_int_create (val);
  DictIntVars_insert (&int_vars, var_name, v);
  DictVars_insert (&var_types, var_name, VAR_INTEGER);
}

void
interpreter__add_string_variable (const char *var_name, const char *val)
{
  interpreter__erase_variable_if_exists (var_name);
  struct StrVariable *v = variable_str_create (val);
  DictStrVars_insert (&str_vars, var_name, v);
  DictVars_insert (&var_types, var_name, VAR_STRING);
}

void
interpreter_start (ASTHandle head)
{
  /* initialize all variable maps */
  DictStrVars_init (&str_vars, hash_string, string_compare, 10);
  DictIntVars_init (&int_vars, hash_string, string_compare, 10);
  DictVars_init (&var_types, hash_string, string_compare, 10);
  struct AST *h = ast_get_node (head);
  if (h->type == AST_SCOPE)
    {
      for (unsigned int i = 0; i < ARRAY_COUNT (h->scope_data.statements); i++)
        {
          interpreter__evaluate_statement (h->scope_data.statements[i]);
        }
    }

  /* end of the program */
  /* all variables have reached the end of their scope */
  for (int i = 0; i < int_vars.capacity; i++)
    {
      if (int_vars.map[i] != 0)
        {
          struct IntVariable *val
              = DictIntVars_get_entry_pointer (&int_vars, int_vars.map[i])
                    ->val;
          obj_dec_refcount (val);
        }
    }
  for (int i = 0; i < str_vars.capacity; i++)
    {
      if (str_vars.map[i] != 0)
        {
          StrVariable *val
              = DictStrVars_get_entry_pointer (&str_vars, str_vars.map[i])
                    ->val;
          obj_dec_refcount (val);
        }
    }
  DictStrVars_free (&str_vars);
  DictIntVars_free (&int_vars);
  DictVars_free (&var_types);
}

void
interpreter__evaluate_statement (ASTHandle statement)
{
  struct AST *s = ast_get_node (statement);
  if (s->type == AST_ASSIGNMENT)
    {
      interpreter__evaluate_assignment (statement);
    }
  else if (s->type == AST_BINARY_OP)
    {
      interpreter__evaluate_binary_op (statement);
    }
}

int
interpreter__evaluate_binary_op (ASTHandle handle)
{
  int left_value = 0;
  int right_value = 0;
  struct AST *node = ast_get_node (handle);

  if (node->type == AST_BINARY_OP)
    {
      ASTHandle right_handle = node->bop_data.right;
      ASTHandle left_handle = node->bop_data.left;

      struct AST *right = ast_get_node (right_handle);
      if (right->type == AST_INTEGER)
        {
          right_value = right->int_data.value;
        }
      else if (right->type == AST_BINARY_OP)
        {
          right_value = interpreter__evaluate_binary_op (right_handle);
        }
      else
        {
          exit (1);
        }

      struct AST *left = ast_get_node (left_handle);
      if (left->type == AST_INTEGER)
        {
          left_value = left->int_data.value;
        }
      else if (left->type == AST_BINARY_OP)
        {
          left_value = interpreter__evaluate_binary_op (left_handle);
        }
      else
        {
          exit (1);
        }

      switch (node->bop_data.op)
        {
        case '+':
          return right_value + left_value;
          break;
        case '-':
          return right_value - left_value;
          break;
        case '*':
          return right_value * left_value;
          break;
        case '/':
          return right_value / left_value;
          break;
        default:
          return 0;
        }
    }
  return 0;
}

void
interpreter__duplicate_variable (const char *var, const char *assign)
{
  enum VarTypes type;
  bool success = DictVars_get (&var_types, var, &type);
  if (!success)
    {
      printf ("Variable %s does not exist\n", var);
      exit (1);
    }
  switch (type)
    {
    case VAR_INTEGER:
      {
        struct IntVariable *val;
        success = DictIntVars_get (&int_vars, var, &val);
        if (!success)
          {
            printf ("Variable %s does not exist\n", var);
            exit (1);
          }
        obj_inc_refcount (val);
        DictIntVars_insert (&int_vars, assign, val);
      }
      break;
    case VAR_STRING:
      {
        struct StrVariable *val;
        success = DictStrVars_get (&str_vars, var, &val);
        if (!success)
          {
            printf ("Variable %s does not exist\n", var);
            exit (1);
          }
        obj_dec_refcount (val);
        DictStrVars_insert (&str_vars, assign, val);
      }
    }
}

void
interpreter__evaluate_assignment (ASTHandle statement)
{
  struct AST *s = ast_get_node (statement);
  struct AST *expr = ast_get_node (s->asgn_data.expr);
  if (expr->type == AST_INTEGER)
    {
      int val = expr->int_data.value;
      interpreter__add_integer_variable (s->asgn_data.var, val);
    }
  else if (expr->type == AST_BINARY_OP)
    {
      int val = interpreter__evaluate_binary_op (s->asgn_data.expr);
      interpreter__add_integer_variable (s->asgn_data.var, val);
    }
  else if (expr->type == AST_STRING)
    {
      const char *val = expr->str_data.str;
      interpreter__add_string_variable (s->asgn_data.var, val);
    }
  else if (expr->type == AST_IDENTIFIER)
    {
      const char *var = s->asgn_data.var;
      const char *expr_var = expr->id_data.id;
      interpreter__duplicate_variable (expr_var, var);
    }
}
