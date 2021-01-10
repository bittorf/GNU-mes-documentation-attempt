/* Copyright (C) 2016 Jeremiah Orians
 * Copyright (C) 2018 Jan (janneke) Nieuwenhuizen <janneke@gnu.org>
 * Copyright (C) 2020 deesix <deesix@tuta.io>
 * This file is part of M2-Planet.
 *
 * M2-Planet is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * M2-Planet is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with M2-Planet.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cc.h"
#include "gcc_req.h"
#include <stdint.h>

/* Global lists */
struct token_list* global_symbol_list;
struct token_list* global_function_list;
struct token_list* global_constant_list;

/* Core lists for this file */
struct token_list* function;

/* What we are currently working on */
struct type* current_target;
char* break_target_head;
char* break_target_func;
char* break_target_num;
char* continue_target_head;
struct token_list* break_frame;
int current_count;
int Address_of;

/* Imported functions */
char* number_to_hex(int a, int bytes);
char* numerate_number(int a);
char* parse_string(char* string);
int escape_lookup(char* c);
int numerate_string(char *a);
void require(int bool, char* error);
struct token_list* reverse_list(struct token_list* head);
/* Host touchy function will need custom on 64bit systems*/
int fixup_int32(int a);

struct token_list* emit(char *s, struct token_list* head)
{
	struct token_list* t = calloc(1, sizeof(struct token_list));
	require(NULL != t, "Exhusted memory while generating token to emit\n");
	t->next = head;
	t->s = s;
	return t;
}

void emit_out(char* s)
{
	output_list = emit(s, output_list);
}

struct token_list* uniqueID(char* s, struct token_list* l, char* num)
{
	l = emit("\n", emit(num, emit("_", emit(s, l))));
	return l;
}

void uniqueID_out(char* s, char* num)
{
	output_list = uniqueID(s, output_list, num);
}

struct token_list* sym_declare(char *s, struct type* t, struct token_list* list)
{
	struct token_list* a = calloc(1, sizeof(struct token_list));
	require(NULL != a, "Exhusted memory while attempting to declare a symbol\n");
	a->next = list;
	a->s = s;
	a->type = t;
	return a;
}

struct token_list* sym_lookup(char *s, struct token_list* symbol_list)
{
	struct token_list* i;
	for(i = symbol_list; NULL != i; i = i->next)
	{
		if(match(i->s, s)) return i;
	}
	return NULL;
}

void line_error()
{
	require(NULL != global_token, "EOF reached inside of line_error\n");
	file_print(global_token->filename, stderr);
	file_print(":", stderr);
	file_print(numerate_number(global_token->linenumber), stderr);
	file_print(":", stderr);
}

void require_match(char* message, char* required)
{
	require(NULL != global_token, "EOF reached inside of require match\n");
	if(!match(global_token->s, required))
	{
		line_error();
		file_print(message, stderr);
		exit(EXIT_FAILURE);
	}
	global_token = global_token->next;
	require(NULL != global_token, "EOF after require match occurred\n");
}

void expression();
void function_call(char* s, int bool)
{
	require_match("ERROR in process_expression_list\nNo ( was found\n", "(");
	int passed = 0;

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture))
	{
		emit_out("PUSHR R13 R15\t# Prevent overwriting in recursion\n");
		emit_out("PUSHR R14 R15\t# Protect the old base pointer\n");
		emit_out("COPY R13 R15\t# Copy new base pointer\n");
	}
	else if(X86 == Architecture)
	{
		emit_out("PUSH_edi\t# Prevent overwriting in recursion\n");
		emit_out("PUSH_ebp\t# Protect the old base pointer\n");
		emit_out("COPY_esp_to_edi\t# Copy new base pointer\n");
	}
	else if(AMD64 == Architecture)
	{
		emit_out("PUSH_RDI\t# Prevent overwriting in recursion\n");
		emit_out("PUSH_RBP\t# Protect the old base pointer\n");
		emit_out("COPY_RSP_to_RDI\t# Copy new base pointer\n");
	}
	else if(ARMV7L == Architecture)
	{
		emit_out("{R11} PUSH_ALWAYS\t# Prevent overwriting in recursion\n");
		emit_out("{BP} PUSH_ALWAYS\t# Protect the old base pointer\n");
		emit_out("'0' SP R11 NO_SHIFT MOVE_ALWAYS\t# Copy new base pointer\n");
	}
	else if(AARCH64 == Architecture)
	{
		emit_out("PUSH_X16\t# Protect a tmp register we're going to use\n");
		emit_out("PUSH_LR\t# Protect the old return pointer (link)\n");
		emit_out("PUSH_BP\t# Protect the old base pointer\n");
		emit_out("SET_X16_FROM_SP\t# The base pointer to-be\n");
	}

	if(global_token->s[0] != ')')
	{
		expression();
		require(NULL != global_token, "incomplete function call, recieved EOF instead of )\n");
		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("PUSHR R0 R15\t#_process_expression1\n");
		else if(X86 == Architecture) emit_out("PUSH_eax\t#_process_expression1\n");
		else if(AMD64 == Architecture) emit_out("PUSH_RAX\t#_process_expression1\n");
		else if(ARMV7L == Architecture) emit_out("{R0} PUSH_ALWAYS\t#_process_expression1\n");
		else if(AARCH64 == Architecture) emit_out("PUSH_X0\t#_process_expression1\n");
		passed = 1;

		while(global_token->s[0] == ',')
		{
			global_token = global_token->next;
			require(NULL != global_token, "incomplete function call, recieved EOF instead of argument\n");
			expression();
			if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("PUSHR R0 R15\t#_process_expression2\n");
			else if(X86 == Architecture) emit_out("PUSH_eax\t#_process_expression2\n");
			else if(AMD64 == Architecture) emit_out("PUSH_RAX\t#_process_expression2\n");
			else if(ARMV7L == Architecture) emit_out("{R0} PUSH_ALWAYS\t#_process_expression2\n");
			else if(AARCH64 == Architecture) emit_out("PUSH_X0\t#_process_expression2\n");
			passed = passed + 1;
		}
	}

	require_match("ERROR in process_expression_list\nNo ) was found\n", ")");

	if(TRUE == bool)
	{
		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture))
		{
			emit_out("LOAD R0 R14 ");
			emit_out(s);
			emit_out("\nMOVE R14 R13\n");
			emit_out("CALL R0 R15\n");
		}
		else if(X86 == Architecture)
		{
			emit_out("LOAD_BASE_ADDRESS_eax %");
			emit_out(s);
			emit_out("\nLOAD_INTEGER\n");
			emit_out("COPY_edi_to_ebp\n");
			emit_out("CALL_eax\n");
		}
		else if(AMD64 == Architecture)
		{
			emit_out("LOAD_BASE_ADDRESS_rax %");
			emit_out(s);
			emit_out("\nLOAD_INTEGER\n");
			emit_out("COPY_rdi_to_rbp\n");
			emit_out("CALL_rax\n");
		}
		else if(ARMV7L == Architecture)
		{
			emit_out("!");
			emit_out(s);
			emit_out(" R0 SUB BP ARITH_ALWAYS\n");
			emit_out("!0 R0 LOAD32 R0 MEMORY\n");
			emit_out("{LR} PUSH_ALWAYS\t# Protect the old link register\n");
			emit_out("'0' R11 BP NO_SHIFT MOVE_ALWAYS\n");
			emit_out("'3' R0 CALL_REG_ALWAYS\n");
			emit_out("{LR} POP_ALWAYS\t# Prevent overwrite\n");
		}
		else if(AARCH64 == Architecture)
		{
			emit_out("SET_X0_FROM_BP\n");
			emit_out("LOAD_W1_AHEAD\nSKIP_32_DATA\n%");
			emit_out(s);
			emit_out("\nSUB_X0_X0_X1\n");
			emit_out("DEREF_X0\n");
			emit_out("SET_BP_FROM_X16\n");
			emit_out("SET_X16_FROM_X0\n");
			emit_out("BLR_X16\n");
		}
	}
	else
	{
		if((KNIGHT_NATIVE == Architecture) || (KNIGHT_POSIX == Architecture))
		{
			emit_out("MOVE R14 R13\n");
			emit_out("LOADR R0 4\nJUMP 4\n&FUNCTION_");
			emit_out(s);
			emit_out("\nCALL R0 R15\n");
		}
		else if(X86 == Architecture)
		{
			emit_out("COPY_edi_to_ebp\n");
			emit_out("CALL_IMMEDIATE %FUNCTION_");
			emit_out(s);
			emit_out("\n");
		}
		else if(AMD64 == Architecture)
		{
			emit_out("COPY_rdi_to_rbp\n");
			emit_out("CALL_IMMEDIATE %FUNCTION_");
			emit_out(s);
			emit_out("\n");
		}
		else if(ARMV7L == Architecture)
		{
			emit_out("{LR} PUSH_ALWAYS\t# Protect the old link register\n");
			emit_out("'0' R11 BP NO_SHIFT MOVE_ALWAYS\n");
			emit_out("^~FUNCTION_");
			emit_out(s);
			emit_out(" CALL_ALWAYS\n");
			emit_out("{LR} POP_ALWAYS\t# Restore the old link register\n");
		}
		else if(AARCH64 == Architecture)
		{
			emit_out("SET_BP_FROM_X16\n");
			emit_out("LOAD_W16_AHEAD\nSKIP_32_DATA\n&FUNCTION_");
			emit_out(s);
			emit_out("\n");
			emit_out("BLR_X16\n");
		}
	}

	for(; passed > 0; passed = passed - 1)
	{
		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("POPR R1 R15\t# _process_expression_locals\n");
		else if(X86 == Architecture) emit_out("POP_ebx\t# _process_expression_locals\n");
		else if(AMD64 == Architecture) emit_out("POP_RBX\t# _process_expression_locals\n");
		else if(ARMV7L == Architecture) emit_out("{R1} POP_ALWAYS\t# _process_expression_locals\n");
		else if(AARCH64 == Architecture) emit_out("POP_X1\t# _process_expression_locals\n");
	}

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture))
	{
		emit_out("POPR R14 R15\t# Restore old base pointer\n");
		emit_out("POPR R13 R15\t# Prevent overwrite\n");
	}
	else if(X86 == Architecture)
	{
		emit_out("POP_ebp\t# Restore old base pointer\n");
		emit_out("POP_edi\t# Prevent overwrite\n");
	}
	else if(AMD64 == Architecture)
	{
		emit_out("POP_RBP\t# Restore old base pointer\n");
		emit_out("POP_RDI\t# Prevent overwrite\n");
	}
	else if(ARMV7L == Architecture)
	{
		emit_out("{BP} POP_ALWAYS\t# Restore old base pointer\n");
		emit_out("{R11} POP_ALWAYS\t# Prevent overwrite\n");
	}
	else if(AARCH64 == Architecture)
	{
		emit_out("POP_BP\t# Restore the old base pointer\n");
		emit_out("POP_LR\t# Restore the old return pointer (link)\n");
		emit_out("POP_X16\t# Restore a register we used as tmp\n");
	}
}

void constant_load(struct token_list* a)
{
	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("LOADI R0 ");
	else if(X86 == Architecture) emit_out("LOAD_IMMEDIATE_eax %");
	else if(AMD64 == Architecture) emit_out("LOAD_IMMEDIATE_rax %");
	else if(ARMV7L == Architecture) emit_out("!0 R0 LOAD32 R15 MEMORY\n~0 JUMP_ALWAYS\n%");
	else if(AARCH64 == Architecture) emit_out("LOAD_W0_AHEAD\nSKIP_32_DATA\n%");
	emit_out(a->arguments->s);
	emit_out("\n");
}

void variable_load(struct token_list* a)
{
	require(NULL != global_token, "incomplete variable load recieved\n");
	if(match("FUNCTION", a->type->name) && match("(", global_token->s))
	{
		function_call(numerate_number(a->depth), TRUE);
		return;
	}
	current_target = a->type;

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("ADDI R0 R14 ");
	else if(X86 == Architecture) emit_out("LOAD_BASE_ADDRESS_eax %");
	else if(AMD64 == Architecture) emit_out("LOAD_BASE_ADDRESS_rax %");
	else if(ARMV7L == Architecture) emit_out("!");
	else if(AARCH64 == Architecture) emit_out("SET_X0_FROM_BP\nLOAD_W1_AHEAD\nSKIP_32_DATA\n%");

	emit_out(numerate_number(a->depth));
	if(ARMV7L == Architecture) emit_out(" R0 SUB BP ARITH_ALWAYS");
	else if(AARCH64 == Architecture) emit_out("\nSUB_X0_X0_X1\n");
	emit_out("\n");

	if(TRUE == Address_of) return;
	if(match("=", global_token->s)) return;

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("LOAD R0 R0 0\n");
	else if(X86 == Architecture) emit_out("LOAD_INTEGER\n");
	else if(AMD64 == Architecture) emit_out("LOAD_INTEGER\n");
	else if(ARMV7L == Architecture) emit_out("!0 R0 LOAD32 R0 MEMORY\n");
	else if(AARCH64 == Architecture) emit_out("DEREF_X0\n");
}

void function_load(struct token_list* a)
{
	require(NULL != global_token, "incomplete function load\n");
	if(match("(", global_token->s))
	{
		function_call(a->s, FALSE);
		return;
	}

	if((KNIGHT_NATIVE == Architecture) || (KNIGHT_POSIX == Architecture)) emit_out("LOADR R0 4\nJUMP 4\n&FUNCTION_");
	else if(X86 == Architecture) emit_out("LOAD_IMMEDIATE_eax &FUNCTION_");
	else if(AMD64 == Architecture) emit_out("LOAD_IMMEDIATE_rax &FUNCTION_");
	else if(ARMV7L == Architecture) emit_out("!0 R0 LOAD32 R15 MEMORY\n~0 JUMP_ALWAYS\n&FUNCTION_");
	else if(AARCH64 == Architecture) emit_out("LOAD_W0_AHEAD\nSKIP_32_DATA\n&FUNCTION_");
	emit_out(a->s);
	emit_out("\n");
}

void global_load(struct token_list* a)
{
	current_target = a->type;
	if((KNIGHT_NATIVE == Architecture) || (KNIGHT_POSIX == Architecture)) emit_out("LOADR R0 4\nJUMP 4\n&GLOBAL_");
	else if(X86 == Architecture) emit_out("LOAD_IMMEDIATE_eax &GLOBAL_");
	else if(AMD64 == Architecture) emit_out("LOAD_IMMEDIATE_rax &GLOBAL_");
	else if(ARMV7L == Architecture) emit_out("!0 R0 LOAD32 R15 MEMORY\n~0 JUMP_ALWAYS\n&GLOBAL_");
	else if(AARCH64 == Architecture) emit_out("LOAD_W0_AHEAD\nSKIP_32_DATA\n&GLOBAL_");
	emit_out(a->s);
	emit_out("\n");

	require(NULL != global_token, "unterminated global load\n");
	if(!match("=", global_token->s))
	{
		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("LOAD R0 R0 0\n");
		else if(X86 == Architecture) emit_out("LOAD_INTEGER\n");
		else if(AMD64 == Architecture) emit_out("LOAD_INTEGER\n");
		else if(ARMV7L == Architecture) emit_out("!0 R0 LOAD32 R0 MEMORY\n");
		else if(AARCH64 == Architecture) emit_out("DEREF_X0\n");
	}
}

/*
 * primary-expr:
 * FAILURE
 * "String"
 * 'Char'
 * [0-9]*
 * [a-z,A-Z]*
 * ( expression )
 */

void primary_expr_failure()
{
	require(NULL != global_token, "hit EOF when expecting primary expression\n");
	line_error();
	file_print("Received ", stderr);
	file_print(global_token->s, stderr);
	file_print(" in primary_expr\n", stderr);
	exit(EXIT_FAILURE);
}

void primary_expr_string()
{
	char* number_string = numerate_number(current_count);
	current_count = current_count + 1;
	if((KNIGHT_NATIVE == Architecture) || (KNIGHT_POSIX == Architecture)) emit_out("LOADR R0 4\nJUMP 4\n&STRING_");
	else if(X86 == Architecture) emit_out("LOAD_IMMEDIATE_eax &STRING_");
	else if(AMD64 == Architecture) emit_out("LOAD_IMMEDIATE_rax &STRING_");
	else if(ARMV7L == Architecture) emit_out("!0 R0 LOAD32 R15 MEMORY\n~0 JUMP_ALWAYS\n&STRING_");
	else if(AARCH64 == Architecture) emit_out("LOAD_W0_AHEAD\nSKIP_32_DATA\n&STRING_");
	uniqueID_out(function->s, number_string);

	/* The target */
	strings_list = emit(":STRING_", strings_list);
	strings_list = uniqueID(function->s, strings_list, number_string);

	/* Parse the string */
	if('"' != global_token->next->s[0])
	{
		strings_list = emit(parse_string(global_token->s), strings_list);
		global_token = global_token->next;
	}
	else
	{
		char* s = calloc(MAX_STRING, sizeof(char));

		/* prefix leading string */
		s[0] = '"';
		int i = 1;

		int j;
		while('"' == global_token->s[0])
		{
			/* Step past the leading '"' */
			j = 1;

			/* Copy the rest of the string as is */
			while(0 != global_token->s[j])
			{
				require(i < MAX_STRING, "concat string exceeded max string length\n");
				s[i] = global_token->s[j];
				i = i + 1;
				j = j + 1;
			}

			/* Move on to the next token */
			global_token = global_token->next;
			require(NULL != global_token, "multi-string null is not valid C\n");
		}

		/* Now use it */
		strings_list = emit(parse_string(s), strings_list);
	}
}

void primary_expr_char()
{
	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("LOADI R0 ");
	else if(X86 == Architecture) emit_out("LOAD_IMMEDIATE_eax %");
	else if(AMD64 == Architecture) emit_out("LOAD_IMMEDIATE_rax %");
	else if(ARMV7L == Architecture) emit_out("!");
	else if(AARCH64 == Architecture) emit_out("LOAD_W0_AHEAD\nSKIP_32_DATA\n%");
	emit_out(numerate_number(escape_lookup(global_token->s + 1)));
	if(ARMV7L == Architecture) emit_out(" R0 LOADI8_ALWAYS");
	emit_out("\n");
	global_token = global_token->next;
}

void primary_expr_number()
{
	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture))
	{
		int size = fixup_int32(numerate_string(global_token->s));
		if((32767 > size) && (size > -32768))
		{
			emit_out("LOADI R0 ");
			emit_out(global_token->s);
		}
		else
		{
			emit_out("LOADR R0 4\nJUMP 4\n'");
			emit_out(number_to_hex(size, register_size));
			emit_out("'");
		}
	}
	else if(X86 == Architecture)
	{
		emit_out("LOAD_IMMEDIATE_eax %");
		emit_out(global_token->s);
	}
	else if(AMD64 == Architecture)
	{
		emit_out("LOAD_IMMEDIATE_rax %");
		emit_out(global_token->s);
	}
	else if(ARMV7L == Architecture)
	{
		emit_out("!0 R0 LOAD32 R15 MEMORY\n~0 JUMP_ALWAYS\n%");
		emit_out(global_token->s);
	}
	else if(AARCH64 == Architecture)
	{
		emit_out("LOAD_W0_AHEAD\nSKIP_32_DATA\n%");
		emit_out(global_token->s);
	}
	emit_out("\n");
	global_token = global_token->next;
}

void primary_expr_variable()
{
	char* s = global_token->s;
	global_token = global_token->next;
	struct token_list* a = sym_lookup(s, global_constant_list);
	if(NULL != a)
	{
		constant_load(a);
		return;
	}

	a= sym_lookup(s, function->locals);
	if(NULL != a)
	{
		variable_load(a);
		return;
	}

	a = sym_lookup(s, function->arguments);
	if(NULL != a)
	{
		variable_load(a);
		return;
	}

	a= sym_lookup(s, global_function_list);
	if(NULL != a)
	{
		function_load(a);
		return;
	}

	a = sym_lookup(s, global_symbol_list);
	if(NULL != a)
	{
		global_load(a);
		return;
	}

	line_error();
	file_print(s ,stderr);
	file_print(" is not a defined symbol\n", stderr);
	exit(EXIT_FAILURE);
}

void primary_expr();
struct type* promote_type(struct type* a, struct type* b)
{
	if(NULL == b)
	{
		return a;
	}
	if(NULL == a)
	{
		return b;
	}

	struct type* i;
	for(i = global_types; NULL != i; i = i->next)
	{
		if(a->name == i->name) break;
		if(b->name == i->name) break;
		if(a->name == i->indirect->name) break;
		if(b->name == i->indirect->name) break;
	}
	return i;
}

void common_recursion(FUNCTION f)
{
	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("PUSHR R0 R15\t#_common_recursion\n");
	else if(X86 == Architecture) emit_out("PUSH_eax\t#_common_recursion\n");
	else if(AMD64 == Architecture) emit_out("PUSH_RAX\t#_common_recursion\n");
	else if(ARMV7L == Architecture) emit_out("{R0} PUSH_ALWAYS\t#_common_recursion\n");
	else if(AARCH64 == Architecture) emit_out("PUSH_X0\t#_common_recursion\n");

	struct type* last_type = current_target;
	global_token = global_token->next;
	require(NULL != global_token, "Recieved EOF in common_recursion\n");
	f();
	current_target = promote_type(current_target, last_type);

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("POPR R1 R15\t# _common_recursion\n");
	else if(X86 == Architecture) emit_out("POP_ebx\t# _common_recursion\n");
	else if(AMD64 == Architecture) emit_out("POP_RBX\t# _common_recursion\n");
	else if(ARMV7L == Architecture) emit_out("{R1} POP_ALWAYS\t# _common_recursion\n");
	else if(AARCH64 == Architecture) emit_out("POP_X1\t# _common_recursion\n");
}

void general_recursion(FUNCTION f, char* s, char* name, FUNCTION iterate)
{
	require(NULL != global_token, "Recieved EOF in general_recursion\n");
	if(match(name, global_token->s))
	{
		common_recursion(f);
		emit_out(s);
		iterate();
	}
}

void arithmetic_recursion(FUNCTION f, char* s1, char* s2, char* name, FUNCTION iterate)
{
	require(NULL != global_token, "Recieved EOF in arithmetic_recursion\n");
	if(match(name, global_token->s))
	{
		common_recursion(f);
		if(NULL == current_target)
		{
			emit_out(s1);
		}
		else if(current_target->is_signed)
		{
			emit_out(s1);
		}
		else
		{
			emit_out(s2);
		}
		iterate();
	}
}

int ceil_log2(int a)
{
	int result = 0;
	if((a & (a - 1)) == 0)
	{
		result = -1;
	}

	while(a > 0)
	{
		result = result + 1;
		a = a >> 1;
	}

	if(ARMV7L == Architecture) return (result >> 1);
	return result;
}

/*
 * postfix-expr:
 *         primary-expr
 *         postfix-expr [ expression ]
 *         postfix-expr ( expression-list-opt )
 *         postfix-expr -> member
 */
struct type* lookup_member(struct type* parent, char* name);
void postfix_expr_arrow()
{
	emit_out("# looking up offset\n");
	global_token = global_token->next;
	require(NULL != global_token, "naked -> not allowed\n");

	struct type* i = lookup_member(current_target, global_token->s);
	current_target = i->type;
	global_token = global_token->next;
	require(NULL != global_token, "Unterminated -> expression not allowed\n");

	if(0 != i->offset)
	{
		emit_out("# -> offset calculation\n");
		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture))
		{
			emit_out("ADDUI R0 R0 ");
			emit_out(numerate_number(i->offset));
			emit_out("\n");
		}
		else if(X86 == Architecture)
		{
			emit_out("LOAD_IMMEDIATE_ebx %");
			emit_out(numerate_number(i->offset));
			emit_out("\nADD_ebx_to_eax\n");
		}
		else if(AMD64 == Architecture)
		{
			emit_out("LOAD_IMMEDIATE_rbx %");
			emit_out(numerate_number(i->offset));
			emit_out("\nADD_rbx_to_rax\n");
		}
		else if(ARMV7L == Architecture)
		{
			emit_out("!0 R1 LOAD32 R15 MEMORY\n~0 JUMP_ALWAYS\n%");
			emit_out(numerate_number(i->offset));
			emit_out("\n'0' R0 R0 ADD R1 ARITH2_ALWAYS\n");
		}
		else if(AARCH64 == Architecture)
		{
			emit_out("LOAD_W1_AHEAD\nSKIP_32_DATA\n%");
			emit_out(numerate_number(i->offset));
			emit_out("\nADD_X0_X1_X0\n");
		}
	}

	if((!match("=", global_token->s) && (register_size >= i->size)))
	{
		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("LOAD R0 R0 0\n");
		else if(X86 == Architecture) emit_out("LOAD_INTEGER\n");
		else if(AMD64 == Architecture) emit_out("LOAD_INTEGER\n");
		else if(ARMV7L == Architecture) emit_out("!0 R0 LOAD32 R0 MEMORY\n");
		else if(AARCH64 == Architecture) emit_out("DEREF_X0\n");
	}
}

void postfix_expr_array()
{
	struct type* array = current_target;
	common_recursion(expression);
	current_target = array;
	require(NULL != current_target, "Arrays only apply to variables\n");

	char* assign;
	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) assign = "LOAD R0 R0 0\n";
	else if(X86 == Architecture) assign = "LOAD_INTEGER\n";
	else if(AMD64 == Architecture) assign = "LOAD_INTEGER\n";
	else if(ARMV7L == Architecture) assign = "!0 R0 LOAD32 R0 MEMORY\n";
	else if(AARCH64 == Architecture) assign = "DEREF_X0\n";

	/* Add support for Ints */
	if(match("char*", current_target->name))
	{
		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) assign = "LOAD8 R0 R0 0\n";
		else if(X86 == Architecture) assign = "LOAD_BYTE\n";
		else if(AMD64 == Architecture) assign = "LOAD_BYTE\n";
		else if(ARMV7L == Architecture) assign = "!0 R0 LOAD8 R0 MEMORY\n";
		else if(AARCH64 == Architecture) assign = "DEREF_X0_BYTE\n";
	}
	else
	{
		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("SALI R0 ");
		else if(X86 == Architecture) emit_out("SAL_eax_Immediate8 !");
		else if(AMD64 == Architecture) emit_out("SAL_rax_Immediate8 !");
		else if(ARMV7L == Architecture) emit_out("'0' R0 R0 '");
		else if(AARCH64 == Architecture) emit_out("LOAD_W2_AHEAD\nSKIP_32_DATA\n%");

		emit_out(numerate_number(ceil_log2(current_target->indirect->size)));
		if(ARMV7L == Architecture) emit_out("' MOVE_ALWAYS");
		else if(AARCH64 == Architecture) emit_out("\nLSHIFT_X0_X0_X2");
		emit_out("\n");
	}

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("ADD R0 R0 R1\n");
	else if(X86 == Architecture) emit_out("ADD_ebx_to_eax\n");
	else if(AMD64 == Architecture) emit_out("ADD_rbx_to_rax\n");
	else if(ARMV7L == Architecture) emit_out("'0' R0 R0 ADD R1 ARITH2_ALWAYS\n");
	else if(AARCH64 == Architecture) emit_out("ADD_X0_X1_X0\n");

	require_match("ERROR in postfix_expr\nMissing ]\n", "]");

	if(match("=", global_token->s))
	{
		assign = "";
	}

	emit_out(assign);
}

/*
 * unary-expr:
 *         postfix-expr
 *         - postfix-expr
 *         !postfix-expr
 *         sizeof ( type )
 */
struct type* type_name();
void unary_expr_sizeof()
{
	global_token = global_token->next;
	require(NULL != global_token, "Recieved EOF when starting sizeof\n");
	require_match("ERROR in unary_expr\nMissing (\n", "(");
	struct type* a = type_name();
	require_match("ERROR in unary_expr\nMissing )\n", ")");

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("LOADUI R0 ");
	else if(X86 == Architecture) emit_out("LOAD_IMMEDIATE_eax %");
	else if(AMD64 == Architecture) emit_out("LOAD_IMMEDIATE_rax %");
	else if(ARMV7L == Architecture) emit_out("!");
	else if(AARCH64 == Architecture) emit_out("LOAD_W0_AHEAD\nSKIP_32_DATA\n%");
	emit_out(numerate_number(a->size));
	if(ARMV7L == Architecture) emit_out(" R0 LOADI8_ALWAYS");
	emit_out("\n");
}

void postfix_expr_stub()
{
	require(NULL != global_token, "Unexpected EOF, improperly terminated primary expression\n");
	if(match("[", global_token->s))
	{
		postfix_expr_array();
		postfix_expr_stub();
	}

	if(match("->", global_token->s))
	{
		postfix_expr_arrow();
		postfix_expr_stub();
	}
}

void postfix_expr()
{
	primary_expr();
	postfix_expr_stub();
}

/*
 * additive-expr:
 *         postfix-expr
 *         additive-expr * postfix-expr
 *         additive-expr / postfix-expr
 *         additive-expr % postfix-expr
 *         additive-expr + postfix-expr
 *         additive-expr - postfix-expr
 *         additive-expr << postfix-expr
 *         additive-expr >> postfix-expr
 */
void additive_expr_stub()
{
	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture))
	{
		arithmetic_recursion(postfix_expr, "ADD R0 R1 R0\n", "ADDU R0 R1 R0\n", "+", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "SUB R0 R1 R0\n", "SUBU R0 R1 R0\n", "-", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "MUL R0 R1 R0\n", "MULU R0 R1 R0\n", "*", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "DIV R0 R1 R0\n", "DIVU R0 R1 R0\n", "/", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "MOD R0 R1 R0\n", "MODU R0 R1 R0\n", "%", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "SAL R0 R1 R0\n", "SL0 R0 R1 R0\n", "<<", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "SAR R0 R1 R0\n", "SR0 R0 R1 R0\n", ">>", additive_expr_stub);
	}
	else if(X86 == Architecture)
	{
		arithmetic_recursion(postfix_expr, "ADD_ebx_to_eax\n", "ADD_ebx_to_eax\n", "+", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "SUBTRACT_eax_from_ebx_into_ebx\nMOVE_ebx_to_eax\n", "SUBTRACT_eax_from_ebx_into_ebx\nMOVE_ebx_to_eax\n", "-", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "MULTIPLYS_eax_by_ebx_into_eax\n", "MULTIPLY_eax_by_ebx_into_eax\n", "*", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "XCHG_eax_ebx\nCDTQ\nDIVIDES_eax_by_ebx_into_eax\n", "XCHG_eax_ebx\nLOAD_IMMEDIATE_edx %0\nDIVIDE_eax_by_ebx_into_eax\n", "/", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "XCHG_eax_ebx\nCDTQ\nMODULUSS_eax_from_ebx_into_ebx\nMOVE_edx_to_eax\n", "XCHG_eax_ebx\nCDTQ\nMODULUS_eax_from_ebx_into_ebx\nMOVE_edx_to_eax\n", "%", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "COPY_eax_to_ecx\nCOPY_ebx_to_eax\nSAL_eax_cl\n", "COPY_eax_to_ecx\nCOPY_ebx_to_eax\nSHL_eax_cl\n", "<<", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "COPY_eax_to_ecx\nCOPY_ebx_to_eax\nSAR_eax_cl\n", "COPY_eax_to_ecx\nCOPY_ebx_to_eax\nSHR_eax_cl\n", ">>", additive_expr_stub);
	}
	else if(AMD64 == Architecture)
	{
		arithmetic_recursion(postfix_expr, "ADD_rbx_to_rax\n", "ADD_rbx_to_rax\n", "+", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "SUBTRACT_rax_from_rbx_into_rbx\nMOVE_rbx_to_rax\n", "SUBTRACT_rax_from_rbx_into_rbx\nMOVE_rbx_to_rax\n", "-", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "MULTIPLYS_rax_by_rbx_into_rax\n", "MULTIPLY_rax_by_rbx_into_rax\n", "*", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "XCHG_rax_rbx\nCQTO\nDIVIDES_rax_by_rbx_into_rax\n", "XCHG_rax_rbx\nLOAD_IMMEDIATE_rdx %0\nDIVIDE_rax_by_rbx_into_rax\n", "/", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "XCHG_rax_rbx\nCQTO\nMODULUSS_rax_from_rbx_into_rbx\nMOVE_rdx_to_rax\n", "XCHG_rax_rbx\nCQTO\nMODULUS_rax_from_rbx_into_rbx\nMOVE_rdx_to_rax\n", "%", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "COPY_rax_to_rcx\nCOPY_rbx_to_rax\nSAL_rax_cl\n", "COPY_rax_to_rcx\nCOPY_rbx_to_rax\nSHL_rax_cl\n", "<<", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "COPY_rax_to_rcx\nCOPY_rbx_to_rax\nSAR_rax_cl\n", "COPY_rax_to_rcx\nCOPY_rbx_to_rax\nSHR_rax_cl\n", ">>", additive_expr_stub);
	}
	else if(ARMV7L == Architecture)
	{
		arithmetic_recursion(postfix_expr, "'0' R0 R0 ADD R1 ARITH2_ALWAYS\n", "'0' R0 R0 ADD R1 ARITH2_ALWAYS\n", "+", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "'0' R0 R0 SUB R1 ARITH2_ALWAYS\n", "'0' R0 R0 SUB R1 ARITH2_ALWAYS\n", "-", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "'9' R0 '0' R1 MULS R0 ARITH2_ALWAYS\n", "'9' R0 '0' R1 MUL R0 ARITH2_ALWAYS\n", "*", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "{LR} PUSH_ALWAYS\n^~divides CALL_ALWAYS\n{LR} POP_ALWAYS\n", "{LR} PUSH_ALWAYS\n^~divide CALL_ALWAYS\n{LR} POP_ALWAYS\n", "/", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "{LR} PUSH_ALWAYS\n^~moduluss CALL_ALWAYS\n{LR} POP_ALWAYS\n", "{LR} PUSH_ALWAYS\n^~modulus CALL_ALWAYS\n{LR} POP_ALWAYS\n", "%", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "LEFT R1 R0 R0 SHIFT AUX_ALWAYS\n", "LEFT R1 R0 R0 SHIFT AUX_ALWAYS\n", "<<", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "ARITH_RIGHT R1 R0 R0 SHIFT AUX_ALWAYS\n", "RIGHT R1 R0 R0 SHIFT AUX_ALWAYS\n", ">>", additive_expr_stub);
	}
	else if(AARCH64 == Architecture)
	{
		general_recursion(postfix_expr, "ADD_X0_X1_X0\n", "+", additive_expr_stub);
		general_recursion(postfix_expr, "SUB_X0_X1_X0\n", "-", additive_expr_stub);
		general_recursion(postfix_expr, "MUL_X0_X1_X0\n", "*", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "SDIV_X0_X1_X0\n", "UDIV_X0_X1_X0\n", "/", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "SDIV_X2_X1_X0\nMSUB_X0_X0_X2_X1\n", "UDIV_X2_X1_X0\nMSUB_X0_X0_X2_X1\n", "%", additive_expr_stub);
		general_recursion(postfix_expr, "LSHIFT_X0_X1_X0\n", "<<", additive_expr_stub);
		arithmetic_recursion(postfix_expr, "ARITH_RSHIFT_X0_X1_X0\n", "LOGICAL_RSHIFT_X0_X1_X0\n", ">>", additive_expr_stub);
	}
}


void additive_expr()
{
	postfix_expr();
	additive_expr_stub();
}


/*
 * relational-expr:
 *         additive_expr
 *         relational-expr < additive_expr
 *         relational-expr <= additive_expr
 *         relational-expr >= additive_expr
 *         relational-expr > additive_expr
 */

void relational_expr_stub()
{
	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture))
	{
		arithmetic_recursion(additive_expr, "CMP R0 R1 R0\nSET.L R0 R0 1\n", "CMPU R0 R1 R0\nSET.L R0 R0 1\n", "<", relational_expr_stub);
		arithmetic_recursion(additive_expr, "CMP R0 R1 R0\nSET.LE R0 R0 1\n", "CMPU R0 R1 R0\nSET.LE R0 R0 1\n", "<=", relational_expr_stub);
		arithmetic_recursion(additive_expr, "CMP R0 R1 R0\nSET.GE R0 R0 1\n", "CMPU R0 R1 R0\nSET.GE R0 R0 1\n", ">=", relational_expr_stub);
		arithmetic_recursion(additive_expr, "CMP R0 R1 R0\nSET.G R0 R0 1\n", "CMPU R0 R1 R0\nSET.G R0 R0 1\n", ">", relational_expr_stub);
		arithmetic_recursion(additive_expr, "CMP R0 R1 R0\nSET.E R0 R0 1\n", "CMPU R0 R1 R0\nSET.E R0 R0 1\n", "==", relational_expr_stub);
		arithmetic_recursion(additive_expr, "CMP R0 R1 R0\nSET.NE R0 R0 1\n", "CMPU R0 R1 R0\nSET.NE R0 R0 1\n", "!=", relational_expr_stub);
	}
	else if(X86 == Architecture)
	{
		arithmetic_recursion(additive_expr, "CMP\nSETL\nMOVEZBL\n", "CMP\nSETB\nMOVEZBL\n", "<", relational_expr_stub);
		arithmetic_recursion(additive_expr, "CMP\nSETLE\nMOVEZBL\n", "CMP\nSETBE\nMOVEZBL\n", "<=", relational_expr_stub);
		arithmetic_recursion(additive_expr, "CMP\nSETGE\nMOVEZBL\n", "CMP\nSETAE\nMOVEZBL\n", ">=", relational_expr_stub);
		arithmetic_recursion(additive_expr, "CMP\nSETG\nMOVEZBL\n", "CMP\nSETA\nMOVEZBL\n", ">", relational_expr_stub);
		general_recursion(additive_expr, "CMP\nSETE\nMOVEZBL\n", "==", relational_expr_stub);
		general_recursion(additive_expr, "CMP\nSETNE\nMOVEZBL\n", "!=", relational_expr_stub);
	}
	else if(AMD64 == Architecture)
	{
		arithmetic_recursion(additive_expr, "CMP\nSETL\nMOVEZX\n", "CMP\nSETB\nMOVEZX\n", "<", relational_expr_stub);
		arithmetic_recursion(additive_expr, "CMP\nSETLE\nMOVEZX\n", "CMP\nSETBE\nMOVEZX\n", "<=", relational_expr_stub);
		arithmetic_recursion(additive_expr, "CMP\nSETGE\nMOVEZX\n", "CMP\nSETAE\nMOVEZX\n", ">=", relational_expr_stub);
		arithmetic_recursion(additive_expr, "CMP\nSETG\nMOVEZX\n", "CMP\nSETA\nMOVEZX\n", ">", relational_expr_stub);
		general_recursion(additive_expr, "CMP\nSETE\nMOVEZX\n", "==", relational_expr_stub);
		general_recursion(additive_expr, "CMP\nSETNE\nMOVEZX\n", "!=", relational_expr_stub);
	}
	else if(ARMV7L == Architecture)
	{
		arithmetic_recursion(additive_expr, "'0' R0 CMP R1 AUX_ALWAYS\n!0 R0 LOADI8_ALWAYS\n!1 R0 LOADI8_L\n", "'0' R0 CMP R1 AUX_ALWAYS\n!0 R0 LOADI8_ALWAYS\n!1 R0 LOADI8_LO\n", "<", relational_expr_stub);
		arithmetic_recursion(additive_expr, "'0' R0 CMP R1 AUX_ALWAYS\n!0 R0 LOADI8_ALWAYS\n!1 R0 LOADI8_LE\n", "'0' R0 CMP R1 AUX_ALWAYS\n!0 R0 LOADI8_ALWAYS\n!1 R0 LOADI8_LS\n", "<=", relational_expr_stub);
		arithmetic_recursion(additive_expr, "'0' R0 CMP R1 AUX_ALWAYS\n!0 R0 LOADI8_ALWAYS\n!1 R0 LOADI8_GE\n", "'0' R0 CMP R1 AUX_ALWAYS\n!0 R0 LOADI8_ALWAYS\n!1 R0 LOADI8_HS\n", ">=", relational_expr_stub);
		arithmetic_recursion(additive_expr, "'0' R0 CMP R1 AUX_ALWAYS\n!0 R0 LOADI8_ALWAYS\n!1 R0 LOADI8_G\n", "'0' R0 CMP R1 AUX_ALWAYS\n!0 R0 LOADI8_ALWAYS\n!1 R0 LOADI8_HI\n", ">", relational_expr_stub);
		general_recursion(additive_expr, "'0' R0 CMP R1 AUX_ALWAYS\n!0 R0 LOADI8_ALWAYS\n!1 R0 LOADI8_EQUAL\n", "==", relational_expr_stub);
		general_recursion(additive_expr, "'0' R0 CMP R1 AUX_ALWAYS\n!0 R0 LOADI8_ALWAYS\n!1 R0 LOADI8_NE\n", "!=", relational_expr_stub);
	}
	else if(AARCH64 == Architecture)
	{
		arithmetic_recursion(additive_expr, "CMP_X1_X0\nSET_X0_TO_1\nSKIP_INST_LT\nSET_X0_TO_0\n", "CMP_X1_X0\nSET_X0_TO_1\nSKIP_INST_LO\nSET_X0_TO_0\n", "<", relational_expr_stub);
		arithmetic_recursion(additive_expr, "CMP_X1_X0\nSET_X0_TO_1\nSKIP_INST_LE\nSET_X0_TO_0\n", "CMP_X1_X0\nSET_X0_TO_1\nSKIP_INST_LS\nSET_X0_TO_0\n", "<=", relational_expr_stub);
		arithmetic_recursion(additive_expr, "CMP_X1_X0\nSET_X0_TO_1\nSKIP_INST_GE\nSET_X0_TO_0\n", "CMP_X1_X0\nSET_X0_TO_1\nSKIP_INST_HS\nSET_X0_TO_0\n", ">=", relational_expr_stub);
		arithmetic_recursion(additive_expr, "CMP_X1_X0\nSET_X0_TO_1\nSKIP_INST_GT\nSET_X0_TO_0\n", "CMP_X1_X0\nSET_X0_TO_1\nSKIP_INST_HI\nSET_X0_TO_0\n", ">", relational_expr_stub);
		general_recursion(additive_expr, "CMP_X1_X0\nSET_X0_TO_1\nSKIP_INST_EQ\nSET_X0_TO_0\n", "==", relational_expr_stub);
		general_recursion(additive_expr, "CMP_X1_X0\nSET_X0_TO_1\nSKIP_INST_NE\nSET_X0_TO_0\n", "!=", relational_expr_stub);
	}
}

void relational_expr()
{
	additive_expr();
	relational_expr_stub();
}

/*
 * bitwise-expr:
 *         relational-expr
 *         bitwise-expr & bitwise-expr
 *         bitwise-expr && bitwise-expr
 *         bitwise-expr | bitwise-expr
 *         bitwise-expr || bitwise-expr
 *         bitwise-expr ^ bitwise-expr
 */
void bitwise_expr_stub()
{
	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture))
	{
		general_recursion(relational_expr, "AND R0 R0 R1\n", "&", bitwise_expr_stub);
		general_recursion(relational_expr, "AND R0 R0 R1\n", "&&", bitwise_expr_stub);
		general_recursion(relational_expr, "OR R0 R0 R1\n", "|", bitwise_expr_stub);
		general_recursion(relational_expr, "OR R0 R0 R1\n", "||", bitwise_expr_stub);
		general_recursion(relational_expr, "XOR R0 R0 R1\n", "^", bitwise_expr_stub);
	}
	else if(X86 == Architecture)
	{
		general_recursion(relational_expr, "AND_eax_ebx\n", "&", bitwise_expr_stub);
		general_recursion(relational_expr, "AND_eax_ebx\n", "&&", bitwise_expr_stub);
		general_recursion(relational_expr, "OR_eax_ebx\n", "|", bitwise_expr_stub);
		general_recursion(relational_expr, "OR_eax_ebx\n", "||", bitwise_expr_stub);
		general_recursion(relational_expr, "XOR_ebx_eax_into_eax\n", "^", bitwise_expr_stub);
	}
	else if(AMD64 == Architecture)
	{
		general_recursion(relational_expr, "AND_rax_rbx\n", "&", bitwise_expr_stub);
		general_recursion(relational_expr, "AND_rax_rbx\n", "&&", bitwise_expr_stub);
		general_recursion(relational_expr, "OR_rax_rbx\n", "|", bitwise_expr_stub);
		general_recursion(relational_expr, "OR_rax_rbx\n", "||", bitwise_expr_stub);
		general_recursion(relational_expr, "XOR_rbx_rax_into_rax\n", "^", bitwise_expr_stub);
	}
	else if(ARMV7L == Architecture)
	{
		general_recursion(relational_expr, "NO_SHIFT R0 R0 AND R1 ARITH2_ALWAYS\n", "&", bitwise_expr_stub);
		general_recursion(relational_expr, "NO_SHIFT R0 R0 AND R1 ARITH2_ALWAYS\n", "&&", bitwise_expr_stub);
		general_recursion(relational_expr, "NO_SHIFT R0 R0 OR R1 AUX_ALWAYS\n", "|", bitwise_expr_stub);
		general_recursion(relational_expr, "NO_SHIFT R0 R0 OR R1 AUX_ALWAYS\n", "||", bitwise_expr_stub);
		general_recursion(relational_expr, "'0' R0 R0 XOR R1 ARITH2_ALWAYS\n", "^", bitwise_expr_stub);
	}
	else if(AARCH64 == Architecture)
	{
		general_recursion(relational_expr, "AND_X0_X1_X0\n", "&", bitwise_expr_stub);
		general_recursion(relational_expr, "AND_X0_X1_X0\n", "&&", bitwise_expr_stub);
		general_recursion(relational_expr, "OR_X0_X1_X0\n", "|", bitwise_expr_stub);
		general_recursion(relational_expr, "OR_X0_X1_X0\n", "||", bitwise_expr_stub);
		general_recursion(relational_expr, "XOR_X0_X1_X0\n", "^", bitwise_expr_stub);
	}
}


void bitwise_expr()
{
	relational_expr();
	bitwise_expr_stub();
}

/*
 * expression:
 *         bitwise-or-expr
 *         bitwise-or-expr = expression
 */

void primary_expr()
{
	require(NULL != global_token, "Recieved EOF where primary expression expected\n");
	if(match("&", global_token->s))
	{
		Address_of = TRUE;
		global_token = global_token->next;
		require(NULL != global_token, "Recieved EOF after & where primary expression expected\n");
	}
	else
	{
		Address_of = FALSE;
	}

	if(match("sizeof", global_token->s)) unary_expr_sizeof();
	else if('-' == global_token->s[0])
	{
		if(X86 == Architecture) emit_out("LOAD_IMMEDIATE_eax %0\n");
		else if(AMD64 == Architecture) emit_out("LOAD_IMMEDIATE_rax %0\n");
		else if(ARMV7L == Architecture) emit_out("!0 R0 LOADI8_ALWAYS\n");
		else if(AARCH64 == Architecture) emit_out("SET_X0_TO_0\n");

		common_recursion(primary_expr);

		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("NEG R0 R0\n");
		else if(X86 == Architecture) emit_out("SUBTRACT_eax_from_ebx_into_ebx\nMOVE_ebx_to_eax\n");
		else if(AMD64 == Architecture) emit_out("SUBTRACT_rax_from_rbx_into_rbx\nMOVE_rbx_to_rax\n");
		else if(ARMV7L == Architecture) emit_out("'0' R0 R0 SUB R1 ARITH2_ALWAYS\n");
		else if(AARCH64 == Architecture) emit_out("SUB_X0_X1_X0\n");
	}
	else if('!' == global_token->s[0])
	{
		if(X86 == Architecture) emit_out("LOAD_IMMEDIATE_eax %1\n");
		else if(AMD64 == Architecture) emit_out("LOAD_IMMEDIATE_rax %1\n");
		else if(ARMV7L == Architecture) emit_out("!1 R0 LOADI8_ALWAYS\n");
		else if(AARCH64 == Architecture) emit_out("SET_X0_TO_1\n");

		common_recursion(postfix_expr);

		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("XORI R0 R0 1\n");
		else if(X86 == Architecture) emit_out("XOR_ebx_eax_into_eax\n");
		else if(AMD64 == Architecture) emit_out("XOR_rbx_rax_into_rax\n");
		else if(ARMV7L == Architecture) emit_out("'0' R0 R0 XOR R1 ARITH2_ALWAYS\n");
		else if(AARCH64 == Architecture) emit_out("XOR_X0_X1_X0\n");
	}
	else if('~' == global_token->s[0])
	{
		common_recursion(postfix_expr);

		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("NOT R0 R0\n");
		else if(X86 == Architecture) emit_out("NOT_eax\n");
		else if(AMD64 == Architecture) emit_out("NOT_rax\n");
		else if(ARMV7L == Architecture) emit_out("'0' R0 R0 MVN_ALWAYS\n");
		else if(AARCH64 == Architecture) emit_out("MVN_X0\n");
	}
	else if(global_token->s[0] == '(')
	{
		global_token = global_token->next;
		expression();
		require_match("Error in Primary expression\nDidn't get )\n", ")");
	}
	else if(global_token->s[0] == '\'') primary_expr_char();
	else if(global_token->s[0] == '"') primary_expr_string();
	else if(in_set(global_token->s[0], "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_")) primary_expr_variable();
	else if(in_set(global_token->s[0], "0123456789")) primary_expr_number();
	else primary_expr_failure();
}

void expression()
{
	bitwise_expr();
	if(match("=", global_token->s))
	{
		char* store = "";
		if(!match("]", global_token->prev->s) || !match("char*", current_target->name))
		{
			if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) store = "STORE R0 R1 0\n";
			else if(X86 == Architecture) store = "STORE_INTEGER\n";
			else if(AMD64 == Architecture) store = "STORE_INTEGER\n";
			else if(ARMV7L == Architecture) store = "!0 R0 STORE32 R1 MEMORY\n";
			else if(AARCH64 == Architecture) store = "STR_X0_[X1]\n";
		}
		else
		{
			if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) store = "STORE8 R0 R1 0\n";
			else if(X86 == Architecture) store = "STORE_CHAR\n";
			else if(AMD64 == Architecture) store = "STORE_CHAR\n";
			else if(ARMV7L == Architecture) store = "!0 R0 STORE8 R1 MEMORY\n";
			else if(AARCH64 == Architecture) store = "STR_BYTE_W0_[X1]\n";
		}

		common_recursion(expression);
		emit_out(store);
		current_target = NULL;
	}
}


/* Process local variable */
void collect_local()
{
	if(NULL != break_target_func)
	{
		file_print("Local variable initialized inside of loop in file: ", stderr);
		line_error();
		file_print("\nMove the variable outside of the loop to resolve\n", stderr);
		file_print("Otherwise the binary will segfault while running\n", stderr);
		exit(EXIT_FAILURE);
	}
	struct type* type_size = type_name();
	struct token_list* a = sym_declare(global_token->s, type_size, function->locals);
	if(match("main", function->s) && (NULL == function->locals))
	{
		if(KNIGHT_NATIVE == Architecture) a->depth = register_size;
		else if(KNIGHT_POSIX == Architecture) a->depth = 20;
		else if(X86 == Architecture) a->depth = -20;
		else if(AMD64 == Architecture) a->depth = -40;
		else if(ARMV7L == Architecture) a->depth = 16;
		else if(AARCH64 == Architecture) a->depth = 32; /* argc, argv, envp and the local (8 bytes each) */
	}
	else if((NULL == function->arguments) && (NULL == function->locals))
	{
		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) a->depth = register_size;
		else if(X86 == Architecture) a->depth = -8;
		else if(AMD64 == Architecture) a->depth = -16;
		else if(ARMV7L == Architecture) a->depth = 8;
		else if(AARCH64 == Architecture) a->depth = register_size;
	}
	else if(NULL == function->locals)
	{
		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) a->depth = function->arguments->depth + 8;
		else if(X86 == Architecture) a->depth = function->arguments->depth - 8;
		else if(AMD64 == Architecture) a->depth = function->arguments->depth - 16;
		else if(ARMV7L == Architecture) a->depth = function->arguments->depth + 8;
		else if(AARCH64 == Architecture) a->depth = function->arguments->depth + register_size;
	}
	else
	{
		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) a->depth = function->locals->depth + register_size;
		else if(X86 == Architecture) a->depth = function->locals->depth - register_size;
		else if(AMD64 == Architecture) a->depth = function->locals->depth - register_size;
		else if(ARMV7L == Architecture) a->depth = function->locals->depth + register_size;
		else if(AARCH64 == Architecture) a->depth = function->locals->depth + register_size;
	}

	function->locals = a;

	emit_out("# Defining local ");
	emit_out(global_token->s);
	emit_out("\n");

	global_token = global_token->next;
	require(NULL != global_token, "incomplete local missing name\n");

	if(match("=", global_token->s))
	{
		global_token = global_token->next;
		require(NULL != global_token, "incomplete local assignment\n");
		expression();
	}

	require_match("ERROR in collect_local\nMissing ;\n", ";");

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("PUSHR R0 R15\t#");
	else if(X86 == Architecture) emit_out("PUSH_eax\t#");
	else if(AMD64 == Architecture) emit_out("PUSH_RAX\t#");
	else if(ARMV7L == Architecture) emit_out("{R0} PUSH_ALWAYS\t#");
	else if(AARCH64 == Architecture) emit_out("PUSH_X0\t#");
	emit_out(a->s);
	emit_out("\n");
}

void statement();

/* Evaluate if statements */
void process_if()
{
	char* number_string = numerate_number(current_count);
	current_count = current_count + 1;

	emit_out("# IF_");
	uniqueID_out(function->s, number_string);

	global_token = global_token->next;
	require_match("ERROR in process_if\nMISSING (\n", "(");
	expression();

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("JUMP.Z R0 @ELSE_");
	else if(X86 == Architecture) emit_out("TEST\nJUMP_EQ %ELSE_");
	else if(AMD64 == Architecture) emit_out("TEST\nJUMP_EQ %ELSE_");
	else if(ARMV7L == Architecture) emit_out("!0 CMPI8 R0 IMM_ALWAYS\n^~ELSE_");
	else if(AARCH64 == Architecture) emit_out("CBNZ_X0_PAST_BR\nLOAD_W16_AHEAD\nSKIP_32_DATA\n&ELSE_");

	uniqueID_out(function->s, number_string);
	if(ARMV7L == Architecture) emit_out(" JUMP_EQUAL\n");
	else if(AARCH64 == Architecture) emit_out("\nBR_X16\n");

	require_match("ERROR in process_if\nMISSING )\n", ")");
	statement();
	require(NULL != global_token, "Reached EOF inside of function\n");

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("JUMP @_END_IF_");
	else if(X86 == Architecture) emit_out("JUMP %_END_IF_");
	else if(AMD64 == Architecture) emit_out("JUMP %_END_IF_");
	else if(ARMV7L == Architecture) emit_out("^~_END_IF_");
	else if(AARCH64 == Architecture) emit_out("LOAD_W16_AHEAD\nSKIP_32_DATA\n&_END_IF_");

	uniqueID_out(function->s, number_string);
	if(ARMV7L == Architecture) emit_out(" JUMP_ALWAYS\n");
	else if(AARCH64 == Architecture) emit_out("\nBR_X16\n");

	emit_out(":ELSE_");
	uniqueID_out(function->s, number_string);

	if(match("else", global_token->s))
	{
		global_token = global_token->next;
		require(NULL != global_token, "Recieved EOF where an else statement expected\n");
		statement();
		require(NULL != global_token, "Reached EOF inside of function\n");
	}
	emit_out(":_END_IF_");
	uniqueID_out(function->s, number_string);
}

void process_for()
{
	struct token_list* nested_locals = break_frame;
	char* nested_break_head = break_target_head;
	char* nested_break_func = break_target_func;
	char* nested_break_num = break_target_num;
	char* nested_continue_head = continue_target_head;

	char* number_string = numerate_number(current_count);
	current_count = current_count + 1;

	break_target_head = "FOR_END_";
	continue_target_head = "FOR_ITER_";
	break_target_num = number_string;
	break_frame = function->locals;
	break_target_func = function->s;

	emit_out("# FOR_initialization_");
	uniqueID_out(function->s, number_string);

	global_token = global_token->next;

	require_match("ERROR in process_for\nMISSING (\n", "(");
	if(!match(";",global_token->s))
	{
		expression();
	}

	emit_out(":FOR_");
	uniqueID_out(function->s, number_string);

	require_match("ERROR in process_for\nMISSING ;1\n", ";");
	expression();

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("JUMP.Z R0 @FOR_END_");
	else if(X86 == Architecture) emit_out("TEST\nJUMP_EQ %FOR_END_");
	else if(AMD64 == Architecture) emit_out("TEST\nJUMP_EQ %FOR_END_");
	else if(ARMV7L == Architecture) emit_out("!0 CMPI8 R0 IMM_ALWAYS\n^~FOR_END_");
	else if(AARCH64 == Architecture) emit_out("CBNZ_X0_PAST_BR\nLOAD_W16_AHEAD\nSKIP_32_DATA\n&FOR_END_");
	uniqueID_out(function->s, number_string);
	if(ARMV7L == Architecture) emit_out(" JUMP_EQUAL\n");
	else if(AARCH64 == Architecture) emit_out("\nBR_X16\n");

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("JUMP @FOR_THEN_");
	else if(X86 == Architecture) emit_out("JUMP %FOR_THEN_");
	else if(AMD64 == Architecture) emit_out("JUMP %FOR_THEN_");
	else if(ARMV7L == Architecture) emit_out("^~FOR_THEN_");
	else if(AARCH64 == Architecture) emit_out("LOAD_W16_AHEAD\nSKIP_32_DATA\n&FOR_THEN_");
	uniqueID_out(function->s, number_string);
	if(ARMV7L == Architecture) emit_out(" JUMP_ALWAYS\n");
	else if(AARCH64 == Architecture) emit_out("\nBR_X16\n");

	emit_out(":FOR_ITER_");
	uniqueID_out(function->s, number_string);

	require_match("ERROR in process_for\nMISSING ;2\n", ";");
	expression();

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("JUMP @FOR_");
	else if(X86 == Architecture) emit_out("JUMP %FOR_");
	else if(AMD64 == Architecture) emit_out("JUMP %FOR_");
	else if(ARMV7L == Architecture) emit_out("^~FOR_");
	else if(AARCH64 == Architecture) emit_out("LOAD_W16_AHEAD\nSKIP_32_DATA\n&FOR_");
	uniqueID_out(function->s, number_string);
	if(ARMV7L == Architecture) emit_out(" JUMP_ALWAYS\n");
	else if(AARCH64 == Architecture) emit_out("\nBR_X16\n");

	emit_out(":FOR_THEN_");
	uniqueID_out(function->s, number_string);

	require_match("ERROR in process_for\nMISSING )\n", ")");
	statement();
	require(NULL != global_token, "Reached EOF inside of function\n");

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("JUMP @FOR_ITER_");
	else if(X86 == Architecture) emit_out("JUMP %FOR_ITER_");
	else if(AMD64 == Architecture) emit_out("JUMP %FOR_ITER_");
	else if(ARMV7L == Architecture) emit_out("^~FOR_ITER_");
	else if(AARCH64 == Architecture) emit_out("LOAD_W16_AHEAD\nSKIP_32_DATA\n&FOR_ITER_");
	uniqueID_out(function->s, number_string);
	if(ARMV7L == Architecture) emit_out(" JUMP_ALWAYS\n");
	else if(AARCH64 == Architecture) emit_out("\nBR_X16\n");

	emit_out(":FOR_END_");
	uniqueID_out(function->s, number_string);

	break_target_head = nested_break_head;
	break_target_func = nested_break_func;
	break_target_num = nested_break_num;
	continue_target_head = nested_continue_head;
	break_frame = nested_locals;
}

/* Process Assembly statements */
void process_asm()
{
	global_token = global_token->next;
	require_match("ERROR in process_asm\nMISSING (\n", "(");
	while('"' == global_token->s[0])
	{
		emit_out((global_token->s + 1));
		emit_out("\n");
		global_token = global_token->next;
		require(NULL != global_token, "Recieved EOF inside asm statement\n");
	}
	require_match("ERROR in process_asm\nMISSING )\n", ")");
	require_match("ERROR in process_asm\nMISSING ;\n", ";");
}

/* Process do while loops */
void process_do()
{
	struct token_list* nested_locals = break_frame;
	char* nested_break_head = break_target_head;
	char* nested_break_func = break_target_func;
	char* nested_break_num = break_target_num;
	char* nested_continue_head = continue_target_head;

	char* number_string = numerate_number(current_count);
	current_count = current_count + 1;

	break_target_head = "DO_END_";
	continue_target_head = "DO_TEST_";
	break_target_num = number_string;
	break_frame = function->locals;
	break_target_func = function->s;

	emit_out(":DO_");
	uniqueID_out(function->s, number_string);

	global_token = global_token->next;
	require(NULL != global_token, "Recieved EOF where do statement is expected\n");
	statement();
	require(NULL != global_token, "Reached EOF inside of function\n");

	emit_out(":DO_TEST_");
	uniqueID_out(function->s, number_string);

	require_match("ERROR in process_do\nMISSING while\n", "while");
	require_match("ERROR in process_do\nMISSING (\n", "(");
	expression();
	require_match("ERROR in process_do\nMISSING )\n", ")");
	require_match("ERROR in process_do\nMISSING ;\n", ";");

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("JUMP.NZ R0 @DO_");
	else if(X86 == Architecture) emit_out("TEST\nJUMP_NE %DO_");
	else if(AMD64 == Architecture) emit_out("TEST\nJUMP_NE %DO_");
	else if(ARMV7L == Architecture) emit_out("!0 CMPI8 R0 IMM_ALWAYS\n^~DO_");
	else if(AARCH64 == Architecture) emit_out("CBZ_X0_PAST_BR\nLOAD_W16_AHEAD\nSKIP_32_DATA\n&DO_");
	uniqueID_out(function->s, number_string);
	if(ARMV7L == Architecture) emit_out(" JUMP_NE\n");
	else if(AARCH64 == Architecture) emit_out("\nBR_X16\n");

	emit_out(":DO_END_");
	uniqueID_out(function->s, number_string);

	break_frame = nested_locals;
	break_target_head = nested_break_head;
	break_target_func = nested_break_func;
	break_target_num = nested_break_num;
	continue_target_head = nested_continue_head;
}


/* Process while loops */
void process_while()
{
	struct token_list* nested_locals = break_frame;
	char* nested_break_head = break_target_head;
	char* nested_break_func = break_target_func;
	char* nested_break_num = break_target_num;
	char* nested_continue_head = continue_target_head;

	char* number_string = numerate_number(current_count);
	current_count = current_count + 1;

	break_target_head = "END_WHILE_";
	continue_target_head = "WHILE_";
	break_target_num = number_string;
	break_frame = function->locals;
	break_target_func = function->s;

	emit_out(":WHILE_");
	uniqueID_out(function->s, number_string);

	global_token = global_token->next;
	require_match("ERROR in process_while\nMISSING (\n", "(");
	expression();

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("JUMP.Z R0 @END_WHILE_");
	else if(X86 == Architecture) emit_out("TEST\nJUMP_EQ %END_WHILE_");
	else if(AMD64 == Architecture) emit_out("TEST\nJUMP_EQ %END_WHILE_");
	else if(ARMV7L == Architecture) emit_out("!0 CMPI8 R0 IMM_ALWAYS\n^~END_WHILE_");
	else if(AARCH64 == Architecture) emit_out("CBNZ_X0_PAST_BR\nLOAD_W16_AHEAD\nSKIP_32_DATA\n&END_WHILE_");
	uniqueID_out(function->s, number_string);
	if(ARMV7L == Architecture) emit_out(" JUMP_EQUAL\t");
	else if(AARCH64 == Architecture) emit_out("\nBR_X16\n");
	emit_out("# THEN_while_");
	uniqueID_out(function->s, number_string);

	require_match("ERROR in process_while\nMISSING )\n", ")");
	statement();
	require(NULL != global_token, "Reached EOF inside of function\n");

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("JUMP @WHILE_");
	else if(X86 == Architecture) emit_out("JUMP %WHILE_");
	else if(AMD64 == Architecture) emit_out("JUMP %WHILE_");
	else if(ARMV7L == Architecture) emit_out("^~WHILE_");
	else if(AARCH64 == Architecture) emit_out("LOAD_W16_AHEAD\nSKIP_32_DATA\n&WHILE_");
	uniqueID_out(function->s, number_string);
	if(ARMV7L == Architecture) emit_out(" JUMP_ALWAYS\n");
	else if(AARCH64 == Architecture) emit_out("\nBR_X16\n");
	emit_out(":END_WHILE_");
	uniqueID_out(function->s, number_string);

	break_target_head = nested_break_head;
	break_target_func = nested_break_func;
	break_target_num = nested_break_num;
	continue_target_head = nested_continue_head;
	break_frame = nested_locals;
}

/* Ensure that functions return */
void return_result()
{
	global_token = global_token->next;
	require(NULL != global_token, "Incomplete return statement recieved\n");
	if(global_token->s[0] != ';') expression();

	require_match("ERROR in return_result\nMISSING ;\n", ";");

	struct token_list* i;
	for(i = function->locals; NULL != i; i = i->next)
	{
		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("POPR R1 R15\t# _return_result_locals\n");
		else if(X86 == Architecture) emit_out("POP_ebx\t# _return_result_locals\n");
		else if(AMD64 == Architecture) emit_out("POP_RBX\t# _return_result_locals\n");
		else if(ARMV7L == Architecture) emit_out("{R1} POP_ALWAYS\t# _return_result_locals\n");
		else if(AARCH64 == Architecture) emit_out("POP_X1\t# _return_result_locals\n");
	}

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("RET R15\n");
	else if(X86 == Architecture) emit_out("RETURN\n");
	else if(AMD64 == Architecture) emit_out("RETURN\n");
	else if(ARMV7L == Architecture) emit_out("'1' LR RETURN\n");
	else if(AARCH64 == Architecture) emit_out("RETURN\n");
}

void process_break()
{
	if(NULL == break_target_head)
	{
		line_error();
		file_print("Not inside of a loop or case statement\n", stderr);
		exit(EXIT_FAILURE);
	}
	struct token_list* i = function->locals;
	while(i != break_frame)
	{
		if(NULL == i) break;
		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("POPR R1 R15\t# break_cleanup_locals\n");
		else if(X86 == Architecture) emit_out("POP_ebx\t# break_cleanup_locals\n");
		else if(AMD64 == Architecture) emit_out("POP_RBX\t# break_cleanup_locals\n");
		else if(ARMV7L == Architecture) emit_out("{R1} POP_ALWAYS\t# break_cleanup_locals\n");
		else if(AARCH64 == Architecture) emit_out("POP_X1\t# break_cleanup_locals\n");
		i = i->next;
	}
	global_token = global_token->next;

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("JUMP @");
	else if(X86 == Architecture) emit_out("JUMP %");
	else if(AMD64 == Architecture) emit_out("JUMP %");
	else if(ARMV7L == Architecture) emit_out("^~");
	else if(AARCH64 == Architecture) emit_out("LOAD_W16_AHEAD\nSKIP_32_DATA\n&");

	emit_out(break_target_head);
	emit_out(break_target_func);
	emit_out("_");
	emit_out(break_target_num);
	if(ARMV7L == Architecture) emit_out(" JUMP_ALWAYS");
	else if(AARCH64 == Architecture) emit_out("\nBR_X16");
	emit_out("\n");
	require_match("ERROR in break statement\nMissing ;\n", ";");
}

void process_contine()
{
	if(NULL == continue_target_head)
	{
		line_error();
		file_print("Not inside of a loop\n", stderr);
		exit(EXIT_FAILURE);
	}
	global_token = global_token->next;

	if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("JUMP @");
	else if(X86 == Architecture) emit_out("JUMP %");
	else if(AMD64 == Architecture) emit_out("JUMP %");
	else if(ARMV7L == Architecture) emit_out("^~");
	else if(AARCH64 == Architecture) emit_out("LOAD_W16_AHEAD\nSKIP_32_DATA\n&");

	emit_out(continue_target_head);
	emit_out(break_target_func);
	emit_out("_");
	emit_out(break_target_num);
	if(ARMV7L == Architecture) emit_out(" JUMP_ALWAYS");
	else if(AARCH64 == Architecture) emit_out("\nBR_X16");
	emit_out("\n");
	require_match("ERROR in continue statement\nMissing ;\n", ";");
}

void recursive_statement()
{
	global_token = global_token->next;
	require(NULL != global_token, "Recieved EOF in recursive statement\n");
	struct token_list* frame = function->locals;

	while(!match("}", global_token->s))
	{
		statement();
		require(NULL != global_token, "Recieved EOF in recursive statement prior to }\n");
	}
	global_token = global_token->next;

	/* Clean up any locals added */

	if(((X86 == Architecture) && !match("RETURN\n", output_list->s)) ||
	   ((AMD64 == Architecture) && !match("RETURN\n", output_list->s)) ||
	   (((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) && !match("RET R15\n", output_list->s)) ||
	   ((ARMV7L == Architecture) && !match("'1' LR RETURN\n", output_list->s)) ||
	   ((AARCH64 == Architecture) && !match("RETURN\n", output_list->s)))
	{
		struct token_list* i;
		for(i = function->locals; frame != i; i = i->next)
		{
			if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("POPR R1 R15\t# _recursive_statement_locals\n");
			else if(X86 == Architecture) emit_out( "POP_ebx\t# _recursive_statement_locals\n");
			else if(AMD64 == Architecture) emit_out("POP_RBX\t# _recursive_statement_locals\n");
			else if(ARMV7L == Architecture) emit_out("{R1} POP_ALWAYS\t# _recursive_statement_locals\n");
			else if(AARCH64 == Architecture) emit_out("POP_X1\t# _recursive_statement_locals\n");
		}
	}
	function->locals = frame;
}

/*
 * statement:
 *     { statement-list-opt }
 *     type-name identifier ;
 *     type-name identifier = expression;
 *     if ( expression ) statement
 *     if ( expression ) statement else statement
 *     do statement while ( expression ) ;
 *     while ( expression ) statement
 *     for ( expression ; expression ; expression ) statement
 *     asm ( "assembly" ... "assembly" ) ;
 *     goto label ;
 *     label:
 *     return ;
 *     break ;
 *     expr ;
 */

struct type* lookup_type(char* s, struct type* start);
void statement()
{
	current_target = NULL;
	if(global_token->s[0] == '{')
	{
		recursive_statement();
	}
	else if(':' == global_token->s[0])
	{
		emit_out(global_token->s);
		emit_out("\t#C goto label\n");
		global_token = global_token->next;
	}
	else if((NULL != lookup_type(global_token->s, prim_types)) ||
	          match("struct", global_token->s))
	{
		collect_local();
	}
	else if(match("if", global_token->s))
	{
		process_if();
	}
	else if(match("do", global_token->s))
	{
		process_do();
	}
	else if(match("while", global_token->s))
	{
		process_while();
	}
	else if(match("for", global_token->s))
	{
		process_for();
	}
	else if(match("asm", global_token->s))
	{
		process_asm();
	}
	else if(match("goto", global_token->s))
	{
		global_token = global_token->next;
		require(NULL != global_token, "naked goto is not supported\n");
		if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) emit_out("JUMP @");
		else if(X86 == Architecture) emit_out("JUMP %");
		else if(AMD64 == Architecture) emit_out("JUMP %");
		else if(ARMV7L == Architecture) emit_out("^~");
		else if(AARCH64 == Architecture) emit_out("LOAD_W16_AHEAD\nSKIP_32_DATA\n&");
		emit_out(global_token->s);
		if(ARMV7L == Architecture) emit_out(" JUMP_ALWAYS");
		else if(AARCH64 == Architecture) emit_out("\nBR_X16");
		emit_out("\n");
		global_token = global_token->next;
		require_match("ERROR in statement\nMissing ;\n", ";");
	}
	else if(match("return", global_token->s))
	{
		return_result();
	}
	else if(match("break", global_token->s))
	{
		process_break();
	}
	else if(match("continue", global_token->s))
	{
		process_contine();
	}
	else
	{
		expression();
		require_match("ERROR in statement\nMISSING ;\n", ";");
	}
}

/* Collect function arguments */
void collect_arguments()
{
	global_token = global_token->next;
	require(NULL != global_token, "Recieved EOF when attempting to collect arguments\n");
	struct type* type_size;
	struct token_list* a;

	while(!match(")", global_token->s))
	{
		type_size = type_name();
		if(global_token->s[0] == ')')
		{
			/* foo(int,char,void) doesn't need anything done */
			continue;
		}
		else if(global_token->s[0] != ',')
		{
			/* deal with foo(int a, char b) */
			a = sym_declare(global_token->s, type_size, function->arguments);
			if(NULL == function->arguments)
			{
				if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) a->depth = 0;
				else if(X86 == Architecture) a->depth = -4;
				else if(AMD64 == Architecture) a->depth = -8;
				else if(ARMV7L == Architecture) a->depth = 4;
				else if(AARCH64 == Architecture) a->depth = register_size;
			}
			else
			{
				if((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) a->depth = function->arguments->depth + register_size;
				else if(X86 == Architecture) a->depth = function->arguments->depth - register_size;
				else if(AMD64 == Architecture) a->depth = function->arguments->depth - register_size;
				else if(ARMV7L == Architecture) a->depth = function->arguments->depth + register_size;
				else if(AARCH64 == Architecture) a->depth = function->arguments->depth + register_size;
			}

			global_token = global_token->next;
			require(NULL != global_token, "Incomplete argument list\n");
			function->arguments = a;
		}

		/* ignore trailing comma (needed for foo(bar(), 1); expressions*/
		if(global_token->s[0] == ',')
		{
			global_token = global_token->next;
			require(NULL != global_token, "naked comma in collect arguments\n");
		}

		require(NULL != global_token, "Argument list never completed\n");
	}
	global_token = global_token->next;
}

void declare_function()
{
	current_count = 0;
	function = sym_declare(global_token->prev->s, NULL, global_function_list);

	/* allow previously defined functions to be looked up */
	global_function_list = function;
	if((KNIGHT_NATIVE == Architecture) && match("main", function->s))
	{
		require_match("Impossible error ( vanished\n", "(");
		require_match("Reality ERROR (USING KNIGHT-NATIVE)\nHardware does not support arguments\nthus neither can main on this architecture\ntry tape_01 and tape_02 instead\n", ")");
	}
	else collect_arguments();

	require(NULL != global_token, "Function definitions either need to be prototypes or full\n");
	/* If just a prototype don't waste time */
	if(global_token->s[0] == ';') global_token = global_token->next;
	else
	{
		emit_out("# Defining function ");
		emit_out(function->s);
		emit_out("\n");
		emit_out(":FUNCTION_");
		emit_out(function->s);
		emit_out("\n");
		statement();

		/* Prevent duplicate RETURNS */
		if(((KNIGHT_POSIX == Architecture) || (KNIGHT_NATIVE == Architecture)) && !match("RET R15\n", output_list->s)) emit_out("RET R15\n");
		else if((X86 == Architecture) && !match("RETURN\n", output_list->s)) emit_out("RETURN\n");
		else if((AMD64 == Architecture) && !match("RETURN\n", output_list->s)) emit_out("RETURN\n");
		else if((ARMV7L == Architecture) && !match("'1' LR RETURN\n", output_list->s)) emit_out("'1' LR RETURN\n");
		else if((AARCH64 == Architecture) && !match("RETURN\n", output_list->s)) emit_out("RETURN\n");
	}
}

/*
 * program:
 *     declaration
 *     declaration program
 *
 * declaration:
 *     CONSTANT identifer value
 *     type-name identifier ;
 *     type-name identifier ( parameter-list ) ;
 *     type-name identifier ( parameter-list ) statement
 *
 * parameter-list:
 *     parameter-declaration
 *     parameter-list, parameter-declaration
 *
 * parameter-declaration:
 *     type-name identifier-opt
 */
void program()
{
	function = NULL;
	Address_of = FALSE;
	struct type* type_size;

new_type:
	if (NULL == global_token) return;
	if(match("CONSTANT", global_token->s))
	{
		global_token = global_token->next;
		require(NULL != global_token, "CONSTANT lacks a name\n");
		global_constant_list = sym_declare(global_token->s, NULL, global_constant_list);

		require(NULL != global_token->next, "CONSTANT lacks a value\n");
		if(match("sizeof", global_token->next->s))
		{
			global_token = global_token->next->next;
			require_match("ERROR in CONSTANT with sizeof\nMissing (\n", "(");
			struct type* a = type_name();
			require_match("ERROR in CONSTANT with sizeof\nMissing )\n", ")");
			global_token->prev->s = numerate_number(a->size);
			global_constant_list->arguments = global_token->prev;
		}
		else
		{
			global_constant_list->arguments = global_token->next;
			global_token = global_token->next->next;
		}
	}
	else
	{
		type_size = type_name();
		if(NULL == type_size)
		{
			goto new_type;
		}
		/* Add to global symbol table */
		global_symbol_list = sym_declare(global_token->s, type_size, global_symbol_list);
		global_token = global_token->next;
		require(NULL != global_token, "Unterminated global\n");
		if(match(";", global_token->s))
		{
			/* Ensure 4 bytes are allocated for the global */
			globals_list = emit(":GLOBAL_", globals_list);
			globals_list = emit(global_token->prev->s, globals_list);
			globals_list = emit("\nNULL\n", globals_list);

			global_token = global_token->next;
		}
		else if(match("(", global_token->s)) declare_function();
		else if(match("=",global_token->s))
		{
			/* Store the global's value*/
			globals_list = emit(":GLOBAL_", globals_list);
			globals_list = emit(global_token->prev->s, globals_list);
			globals_list = emit("\n", globals_list);
			global_token = global_token->next;
			require(NULL != global_token, "Global locals value in assignment\n");
			if(in_set(global_token->s[0], "0123456789"))
			{ /* Assume Int */
				globals_list = emit("%", globals_list);
				globals_list = emit(global_token->s, globals_list);
				globals_list = emit("\n", globals_list);
			}
			else if(('"' == global_token->s[0]))
			{ /* Assume a string*/
				globals_list = emit("&GLOBAL_", globals_list);
				globals_list = emit(global_token->prev->prev->s, globals_list);
				globals_list = emit("_contents\n", globals_list);

				globals_list = emit(":GLOBAL_", globals_list);
				globals_list = emit(global_token->prev->prev->s, globals_list);
				globals_list = emit("_contents\n", globals_list);
				globals_list = emit(parse_string(global_token->s), globals_list);
			}
			else
			{
				line_error();
				file_print("Received ", stderr);
				file_print(global_token->s, stderr);
				file_print(" in program\n", stderr);
				exit(EXIT_FAILURE);
			}

			global_token = global_token->next;
			require_match("ERROR in Program\nMissing ;\n", ";");
		}
		else
		{
			line_error();
			file_print("Received ", stderr);
			file_print(global_token->s, stderr);
			file_print(" in program\n", stderr);
			exit(EXIT_FAILURE);
		}
	}
	goto new_type;
}

void recursive_output(struct token_list* head, FILE* out)
{
	struct token_list* i = reverse_list(head);
	while(NULL != i)
	{
		file_print(i->s, out);
		i = i->next;
	}
}
