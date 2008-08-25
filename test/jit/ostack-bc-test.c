/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include <vm/vm.h>
#include <jit/expression.h>
#include <jit/compiler.h>
#include <stdlib.h>
#include <libharness.h>

#include "bc-test-utils.h"

static void assert_pop_stack(unsigned char opc)
{
	unsigned char code[] = { opc };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct compilation_unit *cu;
	struct expression *expr;

	expr = value_expr(J_INT, 1);
	cu = alloc_simple_compilation_unit(&method);
	stack_push(cu->expr_stack, expr);
	convert_to_ir(cu);
	assert_true(stack_is_empty(cu->expr_stack));

	expr_put(expr);
	free_compilation_unit(cu);
}

void test_convert_pop(void)
{
	assert_pop_stack(OPC_POP);
	assert_pop_stack(OPC_POP2);
}

static struct methodblock *alloc_simple_method(unsigned char opc)
{
	struct compilation_unit *cu;
	struct methodblock *ret;
	unsigned char *code;

	code = malloc(1);
	code[0] = opc;

	ret = malloc(sizeof *ret);
	ret->jit_code = code;
	ret->code_size = 1;
	ret->args_count = 0;
	ret->max_locals = 0;

	cu = alloc_simple_compilation_unit(ret);
	ret->compilation_unit = cu;

	return ret;
}

static void free_simple_method(struct methodblock *method)
{
	free(method->jit_code);
	free_compilation_unit(method->compilation_unit);
	free(method);
}

static void assert_dup_stack(unsigned char opc, struct expression *value)
{
	struct compilation_unit *cu;
	struct methodblock *method;
	struct statement *stmt;

	method = alloc_simple_method(opc);
	cu = method->compilation_unit;

	stack_push(cu->expr_stack, value);

	convert_to_ir(cu);
        stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);

	assert_store_stmt(stmt);
	assert_ptr_equals(value, to_expr(stmt->store_src));
	assert_temporary_expr(stmt->store_dest);

	assert_ptr_equals(to_expr(stmt->store_dest), stack_pop(cu->expr_stack));
	assert_ptr_equals(to_expr(stmt->store_dest), stack_pop(cu->expr_stack));

	assert_true(stack_is_empty(cu->expr_stack));

	free_simple_method(method);
}

static void assert_dup2_stack(unsigned char opc, struct expression *value, struct expression *value2)
{
	struct compilation_unit *cu;
	struct methodblock *method;
	struct statement *stmt, *stmt2;

	if (value->vm_type == J_LONG || value->vm_type == J_DOUBLE) {
		assert_dup_stack(opc, value);
		return;
	}

	method = alloc_simple_method(opc);
	cu = method->compilation_unit;

	stack_push(cu->expr_stack, value2);
	stack_push(cu->expr_stack, value);

	convert_to_ir(cu);
	stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);
	stmt2 = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next->next);

	assert_store_stmt(stmt);
	assert_ptr_equals(value, to_expr(stmt->store_src));
	assert_temporary_expr(stmt->store_dest);

	assert_store_stmt(stmt2);
	assert_ptr_equals(value2, to_expr(stmt2->store_src));
	assert_temporary_expr(stmt->store_dest);

	assert_ptr_equals(to_expr(stmt->store_dest), stack_pop(cu->expr_stack));
	assert_ptr_equals(to_expr(stmt2->store_dest), stack_pop(cu->expr_stack));
	assert_ptr_equals(to_expr(stmt->store_dest), stack_pop(cu->expr_stack));
	assert_ptr_equals(to_expr(stmt2->store_dest), stack_pop(cu->expr_stack));

	assert_true(stack_is_empty(cu->expr_stack));

	free_simple_method(method);
}

void test_convert_dup(void)
{
	struct expression *value = value_expr(J_REFERENCE, 0xdeadbeef);
	struct expression *value2 = value_expr(J_REFERENCE, 0xcafedeca);
	struct expression *value3 = value_expr(J_LONG, 0xcafecafecafecafe);

	expr_get(value);
	expr_get(value2);
	expr_get(value3);

	assert_dup_stack(OPC_DUP, value);
	assert_dup2_stack(OPC_DUP2, value, value2);
	assert_dup2_stack(OPC_DUP2, value3, NULL);

	expr_put(value2);
	expr_put(value3);
}

static void assert_dup_x1_stack(unsigned char opc, struct expression *value1,
				struct expression *value2)
{
	struct compilation_unit *cu;
	struct methodblock *method;
	struct statement *stmt;

	method = alloc_simple_method(opc);
	cu = method->compilation_unit;

	stack_push(cu->expr_stack, value2);
	stack_push(cu->expr_stack, value1);

	convert_to_ir(cu);
        stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);

	assert_store_stmt(stmt);
	assert_ptr_equals(value1, to_expr(stmt->store_src));
	assert_temporary_expr(stmt->store_dest);

	assert_ptr_equals(to_expr(stmt->store_dest), stack_pop(cu->expr_stack));
	assert_ptr_equals(value2, stack_pop(cu->expr_stack));
	assert_ptr_equals(to_expr(stmt->store_dest), stack_pop(cu->expr_stack));

	assert_true(stack_is_empty(cu->expr_stack));

	free_simple_method(method);
}

static void assert_dup2_x1_stack(unsigned char opc, struct expression *value1,
				struct expression *value2, struct expression *value3)
{
	struct compilation_unit *cu;
	struct methodblock *method;
	struct statement *stmt, *stmt2;

	if (value1->vm_type == J_LONG || value2->vm_type == J_DOUBLE) {
		assert_dup_x1_stack(opc, value1, value2);
		return;
	}

	method = alloc_simple_method(opc);
	cu = method->compilation_unit;

	stack_push(cu->expr_stack, value3);
	stack_push(cu->expr_stack, value2);
	stack_push(cu->expr_stack, value1);

	convert_to_ir(cu);
        stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);
	stmt2 = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next->next);

	assert_store_stmt(stmt);
	assert_ptr_equals(value1, to_expr(stmt->store_src));
	assert_temporary_expr(stmt->store_dest);

	assert_store_stmt(stmt2);
	assert_ptr_equals(value2, to_expr(stmt2->store_src));
	assert_temporary_expr(stmt2->store_dest);

	assert_ptr_equals(to_expr(stmt->store_dest), stack_pop(cu->expr_stack));
	assert_ptr_equals(to_expr(stmt2->store_dest), stack_pop(cu->expr_stack));
	assert_ptr_equals(value3, stack_pop(cu->expr_stack));
	assert_ptr_equals(to_expr(stmt->store_dest), stack_pop(cu->expr_stack));
	assert_ptr_equals(to_expr(stmt2->store_dest), stack_pop(cu->expr_stack));

	assert_true(stack_is_empty(cu->expr_stack));

	free_simple_method(method);
}

void test_convert_dup_x1(void)
{
	struct expression *value1, *value2, *value3;

	value1 = value_expr(J_REFERENCE, 0xdeadbeef);
	value2 = value_expr(J_REFERENCE, 0xcafebabe);
	value3 = value_expr(J_LONG, 0xdecacafebabebeef);

	expr_get(value1);

	assert_dup_x1_stack(OPC_DUP_X1, value1, value2);
	assert_dup2_x1_stack(OPC_DUP2_X1, value1, value2, value3);
	assert_dup2_x1_stack(OPC_DUP2_X1, value3, value2, value1);
}

static void assert_dup_x2_stack(unsigned char opc, struct expression *value1,
				struct expression *value2, struct expression *value3)
{
	struct compilation_unit *cu;
	struct methodblock *method;
	struct statement *stmt;

	method = alloc_simple_method(opc);
	cu = method->compilation_unit;

	stack_push(cu->expr_stack, value3);
	stack_push(cu->expr_stack, value2);
	stack_push(cu->expr_stack, value1);

	convert_to_ir(cu);
        stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);

	assert_store_stmt(stmt);
	assert_ptr_equals(value1, to_expr(stmt->store_src));
	assert_temporary_expr(stmt->store_dest);

	assert_ptr_equals(to_expr(stmt->store_dest), stack_pop(cu->expr_stack));
	assert_ptr_equals(value2, stack_pop(cu->expr_stack));
	assert_ptr_equals(value3, stack_pop(cu->expr_stack));
	assert_ptr_equals(to_expr(stmt->store_dest), stack_pop(cu->expr_stack));

	assert_true(stack_is_empty(cu->expr_stack));

	free_simple_method(method);
}

static void assert_dup2_x2_stack(unsigned char opc, struct expression *value1,
				struct expression *value2, struct expression *value3,
				struct expression *value4)
{
	struct compilation_unit *cu;
	struct methodblock *method;
	struct statement *stmt, *stmt2;

	if (value1->vm_type == J_LONG || value1->vm_type == J_DOUBLE) {
		if (value2->vm_type == J_LONG || value2->vm_type == J_DOUBLE) {
			assert_dup_x1_stack(opc, value1, value2);
			return;
		} else {
			assert_dup_x2_stack(opc, value1, value2, value3);
			return;
		}
	} else {
		if (value3->vm_type == J_LONG || value3->vm_type == J_DOUBLE) {
			assert_dup2_x1_stack(opc, value1, value2, value3);
			return;
		}
	}

	method = alloc_simple_method(opc);
	cu = method->compilation_unit;

	stack_push(cu->expr_stack, value4);
	stack_push(cu->expr_stack, value3);
	stack_push(cu->expr_stack, value2);
	stack_push(cu->expr_stack, value1);

	convert_to_ir(cu);
        stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);
	stmt2 = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next->next);

	assert_store_stmt(stmt);
	assert_ptr_equals(value1, to_expr(stmt->store_src));
	assert_temporary_expr(stmt->store_dest);

	assert_store_stmt(stmt2);
	assert_ptr_equals(value2, to_expr(stmt2->store_src));
	assert_temporary_expr(stmt2->store_dest);

	assert_ptr_equals(to_expr(stmt->store_dest), stack_pop(cu->expr_stack));
	assert_ptr_equals(to_expr(stmt2->store_dest), stack_pop(cu->expr_stack));
	assert_ptr_equals(value3, stack_pop(cu->expr_stack));
	assert_ptr_equals(value4, stack_pop(cu->expr_stack));
	assert_ptr_equals(to_expr(stmt->store_dest), stack_pop(cu->expr_stack));
	assert_ptr_equals(to_expr(stmt2->store_dest), stack_pop(cu->expr_stack));

	assert_true(stack_is_empty(cu->expr_stack));

	free_simple_method(method);
}

void test_convert_dup_x2(void)
{
	struct expression *value1, *value2, *value3, *value4;

	value1 = value_expr(J_REFERENCE, 0xdeadbeef);
	value2 = value_expr(J_REFERENCE, 0xcafebabe);
	value3 = value_expr(J_REFERENCE, 0xb4df00d);
	value4 = value_expr(J_REFERENCE, 0x6559570);

	expr_get(value1);

	assert_dup_x2_stack(OPC_DUP_X2, value1, value2, value3);
	assert_dup2_x2_stack(OPC_DUP2_X2, value1, value2, value3, value4);

	expr_put(value3);
	expr_put(value4);
}

static void assert_swap_stack(unsigned char opc,
			      void *expected1, void *expected2)
{
	unsigned char code[] = { opc };
	struct compilation_unit *cu;
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};

	cu = alloc_simple_compilation_unit(&method);

	stack_push(cu->expr_stack, expected1);
	stack_push(cu->expr_stack, expected2);

	convert_to_ir(cu);
	assert_ptr_equals(stack_pop(cu->expr_stack), expected1);
	assert_ptr_equals(stack_pop(cu->expr_stack), expected2);
	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
}

void test_convert_swap(void)
{
	assert_swap_stack(OPC_SWAP, (void *)1, (void *)2);
	assert_swap_stack(OPC_SWAP, (void *)2, (void *)3);
}
