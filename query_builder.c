/* Copyright (C) 2005 Jukka Zitting <jz@yukatan.fi>
 * Copyright (C) 2008, 2009 Piotr Pokora <piotrek.pokora@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "php_midgard.h"
#include "php_midgard_gobject.h"

#include "php_midgard__helpers.h"

#include <zend_interfaces.h>

#define _GET_BUILDER_OBJECT \
	zval *zval_object = getThis(); \
	php_midgard_gobject *php_gobject = __php_objstore_object(zval_object); \
	MidgardQueryBuilder *builder = MIDGARD_QUERY_BUILDER(php_gobject->gobject); \
	if (!builder) \
		php_error(E_ERROR, "Can not find underlying builder instance");

zend_class_entry *php_midgard_query_builder_class;

/* Object constructor */
static PHP_METHOD(midgard_query_builder, __construct)
{
	MidgardConnection *mgd = mgd_handle(TSRMLS_C);
	CHECK_MGD(mgd);

	char *classname;
	long classname_length;
	zval *zval_object = getThis();
	GObject *gobject;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &classname, &classname_length) == FAILURE) {
		return;
	}

	zend_class_entry *ce = zend_fetch_class(classname, classname_length, ZEND_FETCH_CLASS_AUTO TSRMLS_CC);

	if (ce == NULL) {
		php_error(E_WARNING, "Didn't find %s class", classname);
		php_midgard_error_exception_force_throw(mgd, MGD_ERR_INVALID_OBJECT TSRMLS_CC);
	}

	zend_class_entry *base_ce = php_midgard_get_baseclass_ptr(ce);
	const gchar *g_base_class_name = php_class_name_to_g_class_name(base_ce->name);

	GType classtype = g_type_from_name(g_base_class_name);

	if (!g_type_is_a(classtype, MIDGARD_TYPE_DBOBJECT)) {
		php_error(E_WARNING, "Expected %s derived class", g_type_name(MIDGARD_TYPE_DBOBJECT));
		php_midgard_error_exception_force_throw(mgd, MGD_ERR_INVALID_OBJECT TSRMLS_CC);
		return;
	}

	gobject = __php_gobject_ptr(zval_object);

	if (!gobject) {
		MidgardQueryBuilder *builder = midgard_query_builder_new(mgd, g_base_class_name);

		if (!builder) {
			php_midgard_error_exception_throw(mgd TSRMLS_CC);
			return;
		}

		MGD_PHP_SET_GOBJECT(zval_object, builder);
	} else {
		// we already have gobject injected
	}

	php_midgard_gobject *php_gobject = __php_objstore_object(zval_object);
	/* Set user defined class. We might need it when execute is invoked */
	php_gobject->user_ce = ce;
	php_gobject->user_class_name = ce->name;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_mqb___construct, 0, 0, 1)
	ZEND_ARG_INFO(0, classname)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_builder, add_constraint)
{
	RETVAL_FALSE;
	MidgardConnection *mgd = mgd_handle(TSRMLS_C);
	CHECK_MGD(mgd);

	char *name, *op;
	long name_length, op_length;
	zval *value;
	zval *zval_object = getThis();
	zend_bool rv;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz", &name, &name_length, &op, &op_length, &value) != SUCCESS) {
		return;
	}

	MidgardQueryBuilder *builder = MIDGARD_QUERY_BUILDER(__php_gobject_ptr(zval_object));

	GValue *gvalue = php_midgard_zval2gvalue(value TSRMLS_CC);

	if (gvalue == NULL)
		RETURN_FALSE;

	rv = midgard_query_builder_add_constraint(builder, name, op, gvalue);

	g_value_unset(gvalue);
	g_free(gvalue);

	RETURN_BOOL(rv);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_mqb_add_constraint, 0, 0, 3)
	ZEND_ARG_INFO(0, property)
	ZEND_ARG_INFO(0, operator)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_builder, add_constraint_with_property)
{
	RETVAL_FALSE;
	MidgardConnection *mgd = mgd_handle(TSRMLS_C);
	CHECK_MGD(mgd);

	char *name_a, *name_b, *op;
	int name_a_length, name_b_length, op_length;
	gboolean rv;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss",
				&name_a, &name_a_length,
				&op, &op_length,
				&name_b, &name_b_length) != SUCCESS)
	{
		return;
	}

	_GET_BUILDER_OBJECT;

	rv = midgard_query_builder_add_constraint_with_property(builder,
			(const gchar *)name_a, (const gchar *)op, (const gchar *)name_b);

	RETURN_BOOL(rv);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_mqb_add_constraint_with_property, 0, 0, 3)
	ZEND_ARG_INFO(0, property)
	ZEND_ARG_INFO(0, operator)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_builder, begin_group)
{
	RETVAL_FALSE;
	MidgardConnection *mgd = mgd_handle(TSRMLS_C);
	CHECK_MGD(mgd);

	char *type = NULL;
	int type_length;
	gboolean rv;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &type, &type_length) != SUCCESS) {
		return;
	}

	_GET_BUILDER_OBJECT;

	rv = midgard_query_builder_begin_group(builder, type);

	RETURN_BOOL(rv);
}

ZEND_BEGIN_ARG_INFO(arginfo_mqb_begin_group, 0)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_builder, end_group)
{
	RETVAL_FALSE;
	MidgardConnection *mgd = mgd_handle(TSRMLS_C);
	CHECK_MGD(mgd);

	gboolean rv;

	if (zend_parse_parameters_none() == FAILURE)
		return;

	_GET_BUILDER_OBJECT;

	rv = midgard_query_builder_end_group(builder);

	RETURN_BOOL(rv);
}

ZEND_BEGIN_ARG_INFO(arginfo_mqb_end_group, 0)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_builder, execute)
{
	RETVAL_FALSE;
	MidgardConnection *mgd = mgd_handle(TSRMLS_C);
	CHECK_MGD(mgd);

	if (zend_parse_parameters_none() == FAILURE)
		return;

	_GET_BUILDER_OBJECT;
	zend_class_entry *ce = php_gobject->user_ce;

	if (ce == NULL) {
		php_error(E_WARNING, "Query Builder instance not associated with any class");
		return;
	}

	guint i, n_objects;
	GObject **objects = midgard_query_builder_execute(builder, &n_objects);

	array_init(return_value);

	if (!objects)
		return;

	/* TODO: Should use iterator, instead, and create objects lazily */
	/* Initialize zend objects for the same class which was used to initialize Query Builder */
	for (i = 0; i < n_objects; i++) {
		GObject *gobject = objects[i];
		zval *zobject;

		/* TODO: Simplify code below. If possible. */
		MAKE_STD_ZVAL(zobject);
		object_init_ex(zobject, ce); /* Initialize new object for which QB has been created for */
		MGD_PHP_SET_GOBJECT(zobject, gobject); // inject our gobject
		zend_call_method_with_0_params(&zobject, ce, &ce->constructor, "__construct", NULL); /* Call class constructor on given instance */

		zend_hash_next_index_insert(HASH_OF(return_value), &zobject, sizeof(zval *), NULL);
	}

	if (objects)
		g_free(objects);
}

ZEND_BEGIN_ARG_INFO(arginfo_mqb_execute, 0)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_builder, add_order)
{
	RETVAL_FALSE;
	MidgardConnection *mgd = mgd_handle(TSRMLS_C);
	CHECK_MGD(mgd);

	char *field, *order = "ASC";
	int field_length, order_length;
	gboolean rv;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &field, &field_length, &order, &order_length) != SUCCESS) {
		return;
	}

	_GET_BUILDER_OBJECT;

	rv = midgard_query_builder_add_order(builder, field, order);

	RETURN_BOOL(rv);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_mqb_add_order, 0, 0, 1)
	ZEND_ARG_INFO(0, property)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_builder, set_limit)
{
	RETVAL_FALSE;
	MidgardConnection *mgd = mgd_handle(TSRMLS_C);
	CHECK_MGD(mgd);

	long limit;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &limit) != SUCCESS) {
		return;
	}

	_GET_BUILDER_OBJECT;

	/* TODO, check if limit check can be ignored */
	if (limit < 0) {
		php_error(E_WARNING, "Ignoring a negative query limit");
		RETURN_FALSE;
	}

	midgard_query_builder_set_limit(builder, limit);
	RETURN_TRUE;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_mqb_set_limit, 0, 0, 1)
	ZEND_ARG_INFO(0, limit)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_builder, set_offset)
{
	RETVAL_FALSE;
	MidgardConnection *mgd = mgd_handle(TSRMLS_C);
	CHECK_MGD(mgd);

	long offset;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &offset) != SUCCESS) {
		return;
	}

	_GET_BUILDER_OBJECT;

	if (offset < 0) {
		php_error(E_WARNING, "Ingoring a negative query offset");
		RETURN_FALSE;
	}

	midgard_query_builder_set_offset(builder, offset);
	RETURN_TRUE;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_mqb_set_offset, 0, 0, 1)
	ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_builder, include_deleted)
{
	MidgardConnection *mgd = mgd_handle(TSRMLS_C);
	CHECK_MGD(mgd);

	RETVAL_FALSE;

	if (zend_parse_parameters_none() == FAILURE)
		return;

	_GET_BUILDER_OBJECT;

	midgard_query_builder_include_deleted(builder);

	RETURN_TRUE;
}

ZEND_BEGIN_ARG_INFO(arginfo_mqb_include_deleted, 0)
ZEND_END_ARG_INFO()

static PHP_METHOD(midgard_query_builder, count)
{
	RETVAL_FALSE;
	MidgardConnection *mgd = mgd_handle(TSRMLS_C);
	CHECK_MGD(mgd);

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	_GET_BUILDER_OBJECT;

	RETURN_LONG(midgard_query_builder_count(builder));
}

ZEND_BEGIN_ARG_INFO(arginfo_mqb_count, 0)
ZEND_END_ARG_INFO()

PHP_MINIT_FUNCTION(midgard2_query_builder)
{
	static function_entry query_builder_methods[] = {
		PHP_ME(midgard_query_builder, __construct,                  arginfo_mqb___construct,                  ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
		PHP_ME(midgard_query_builder, add_constraint,               arginfo_mqb_add_constraint,               ZEND_ACC_PUBLIC)
		PHP_ME(midgard_query_builder, add_constraint_with_property, arginfo_mqb_add_constraint_with_property, ZEND_ACC_PUBLIC)
		PHP_ME(midgard_query_builder, begin_group,                  arginfo_mqb_begin_group,                  ZEND_ACC_PUBLIC)
		PHP_ME(midgard_query_builder, end_group,                    arginfo_mqb_end_group,                    ZEND_ACC_PUBLIC)
		PHP_ME(midgard_query_builder, execute,                      arginfo_mqb_execute,                      ZEND_ACC_PUBLIC)
		PHP_ME(midgard_query_builder, add_order,                    arginfo_mqb_add_order,                    ZEND_ACC_PUBLIC)
		PHP_ME(midgard_query_builder, set_limit,                    arginfo_mqb_set_limit,                    ZEND_ACC_PUBLIC)
		PHP_ME(midgard_query_builder, set_offset,                   arginfo_mqb_set_offset,                   ZEND_ACC_PUBLIC)
		PHP_ME(midgard_query_builder, include_deleted,              arginfo_mqb_include_deleted,              ZEND_ACC_PUBLIC)
		PHP_ME(midgard_query_builder, count,                        arginfo_mqb_count,                        ZEND_ACC_PUBLIC)
		{NULL, NULL, NULL}
	};

	static zend_class_entry query_builder_class_entry;

	INIT_CLASS_ENTRY(query_builder_class_entry, "midgard_query_builder", query_builder_methods);
	php_midgard_query_builder_class = zend_register_internal_class(&query_builder_class_entry TSRMLS_CC);
	php_midgard_query_builder_class->create_object = php_midgard_gobject_new;
	php_midgard_query_builder_class->doc_comment = strdup("API for building complex data-queries");

	return SUCCESS;
}
