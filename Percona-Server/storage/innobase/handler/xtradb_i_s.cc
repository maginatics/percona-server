/*****************************************************************************

Copyright (c) 2007, 2012, Oracle and/or its affiliates. All Rights Reserved.
Copyright (c) 2010-2012, Percona Inc. All Rights Reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA

*****************************************************************************/

#include <mysqld_error.h>
#include <sql_acl.h>				// PROCESS_ACL

#include <m_ctype.h>
#include <hash.h>
#include <myisampack.h>
#include <mysys_err.h>
#include <my_sys.h>
#include "i_s.h"
#include <sql_plugin.h>
#include <mysql/innodb_priv.h>

#include <read0i_s.h>
#include "srv0start.h"	/* for srv_was_started */

#define PLUGIN_AUTHOR "Percona Inc."

#define OK(expr)		\
	if ((expr) != 0) {	\
		DBUG_RETURN(1);	\
	}

#define RETURN_IF_INNODB_NOT_STARTED(plugin_name)			\
do {									\
	if (!srv_was_started) {						\
		push_warning_printf(thd, Sql_condition::WARN_LEVEL_WARN,	\
				    ER_CANT_FIND_SYSTEM_REC,		\
				    "InnoDB: SELECTing from "		\
				    "INFORMATION_SCHEMA.%s but "	\
				    "the InnoDB storage engine "	\
				    "is not installed", plugin_name);	\
		DBUG_RETURN(0);						\
	}								\
} while (0)

#if !defined __STRICT_ANSI__ && defined __GNUC__ && (__GNUC__) > 2 &&	\
	!defined __INTEL_COMPILER && !defined __clang__
#define STRUCT_FLD(name, value)	name: value
#else
#define STRUCT_FLD(name, value)	value
#endif

#define END_OF_ST_FIELD_INFO \
	{STRUCT_FLD(field_name,		NULL), \
	 STRUCT_FLD(field_length,	0), \
	 STRUCT_FLD(field_type,		MYSQL_TYPE_NULL), \
	 STRUCT_FLD(value,		0), \
	 STRUCT_FLD(field_flags,	0), \
	 STRUCT_FLD(old_name,		""), \
	 STRUCT_FLD(open_method,	SKIP_OPEN_TABLE)}


/*******************************************************************//**
Auxiliary function to store ulint value in MYSQL_TYPE_LONGLONG field.
If the value is ULINT_UNDEFINED then the field it set to NULL.
@return	0 on success */
static
int
field_store_ulint(
/*==============*/
	Field*	field,	/*!< in/out: target field for storage */
	ulint	n)	/*!< in: value to store */
{
	int	ret;

	if (n != ULINT_UNDEFINED) {

		ret = field->store(n);
		field->set_notnull();
	} else {

		ret = 0; /* success */
		field->set_null();
	}

	return(ret);
}

/*******************************************************************//**
Auxiliary function to store char* value in MYSQL_TYPE_STRING field.
@return	0 on success */
static
int
field_store_string(
/*===============*/
	Field*		field,	/*!< in/out: target field for storage */
	const char*	str)	/*!< in: NUL-terminated utf-8 string,
				or NULL */
{
	int	ret;

	if (str != NULL) {

		ret = field->store(str, strlen(str),
				   system_charset_info);
		field->set_notnull();
	} else {

		ret = 0; /* success */
		field->set_null();
	}

	return(ret);
}

static
int
i_s_common_deinit(
/*==============*/
	void*	p)	/*!< in/out: table schema object */
{
	DBUG_ENTER("i_s_common_deinit");

	/* Do nothing */

	DBUG_RETURN(0);
}

static ST_FIELD_INFO xtradb_read_view_fields_info[] =
{
#define READ_VIEW_UNDO_NUMBER		0
	{STRUCT_FLD(field_name,		"READ_VIEW_UNDO_NUMBER"),
	 STRUCT_FLD(field_length,	MY_INT64_NUM_DECIMAL_DIGITS),
	 STRUCT_FLD(field_type,		MYSQL_TYPE_LONGLONG),
	 STRUCT_FLD(value,		0),
	 STRUCT_FLD(field_flags,	MY_I_S_UNSIGNED),
	 STRUCT_FLD(old_name,		""),
	 STRUCT_FLD(open_method,	SKIP_OPEN_TABLE)},

#define READ_VIEW_LOW_LIMIT_NUMBER	1
	{STRUCT_FLD(field_name,		"READ_VIEW_LOW_LIMIT_TRX_NUMBER"),
	 STRUCT_FLD(field_length,	TRX_ID_MAX_LEN + 1),
	 STRUCT_FLD(field_type,		MYSQL_TYPE_STRING),
	 STRUCT_FLD(value,		0),
	 STRUCT_FLD(field_flags,	0),
	 STRUCT_FLD(old_name,		""),
	 STRUCT_FLD(open_method,	SKIP_OPEN_TABLE)},

#define READ_VIEW_UPPER_LIMIT_ID	2
	{STRUCT_FLD(field_name,		"READ_VIEW_UPPER_LIMIT_TRX_ID"),
	 STRUCT_FLD(field_length,	TRX_ID_MAX_LEN + 1),
	 STRUCT_FLD(field_type,		MYSQL_TYPE_STRING),
	 STRUCT_FLD(value,		0),
	 STRUCT_FLD(field_flags,	0),
	 STRUCT_FLD(old_name,		""),
	 STRUCT_FLD(open_method,	SKIP_OPEN_TABLE)},

#define READ_VIEW_LOW_LIMIT_ID		3
	{STRUCT_FLD(field_name,		"READ_VIEW_LOW_LIMIT_TRX_ID"),

	 STRUCT_FLD(field_length,	TRX_ID_MAX_LEN + 1),
	 STRUCT_FLD(field_type,		MYSQL_TYPE_STRING),
	 STRUCT_FLD(value,		0),
	 STRUCT_FLD(field_flags,	0),
	 STRUCT_FLD(old_name,		""),
	 STRUCT_FLD(open_method,	SKIP_OPEN_TABLE)},

	END_OF_ST_FIELD_INFO
};

static int xtradb_read_view_fill_table(THD* thd, TABLE_LIST* tables, Item*)
{
	const char*		table_name;
	Field**	fields;
	TABLE* table;
	char		trx_id[TRX_ID_MAX_LEN + 1];


	DBUG_ENTER("xtradb_read_view_fill_table");

	/* deny access to non-superusers */
	if (check_global_access(thd, PROCESS_ACL)) {

		DBUG_RETURN(0);
	}

	table_name = tables->schema_table_name;
	table = tables->table;
	fields = table->field;

	RETURN_IF_INNODB_NOT_STARTED(table_name);

	i_s_xtradb_read_view_t read_view;

	if (read_fill_i_s_xtradb_read_view(&read_view) == NULL)
		DBUG_RETURN(0);

	OK(field_store_ulint(fields[READ_VIEW_UNDO_NUMBER], read_view.undo_no));

	ut_snprintf(trx_id, sizeof(trx_id), TRX_ID_FMT, read_view.low_limit_no);
	OK(field_store_string(fields[READ_VIEW_LOW_LIMIT_NUMBER], trx_id));

	ut_snprintf(trx_id, sizeof(trx_id), TRX_ID_FMT, read_view.up_limit_id);
	OK(field_store_string(fields[READ_VIEW_UPPER_LIMIT_ID], trx_id));

	ut_snprintf(trx_id, sizeof(trx_id), TRX_ID_FMT, read_view.low_limit_id);
	OK(field_store_string(fields[READ_VIEW_LOW_LIMIT_ID], trx_id));

	OK(schema_table_store_record(thd, table));

	DBUG_RETURN(0);
}


static int xtradb_read_view_init(void* p)
{
	ST_SCHEMA_TABLE*	schema;

	DBUG_ENTER("xtradb_read_view_init");

	schema = (ST_SCHEMA_TABLE*) p;

	schema->fields_info = xtradb_read_view_fields_info;
	schema->fill_table = xtradb_read_view_fill_table;

	DBUG_RETURN(0);
}

static struct st_mysql_information_schema i_s_info =
{
	MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION
};

UNIV_INTERN struct st_mysql_plugin i_s_xtradb_read_view =
{
	STRUCT_FLD(type, MYSQL_INFORMATION_SCHEMA_PLUGIN),
	STRUCT_FLD(info, &i_s_info),
	STRUCT_FLD(name, "XTRADB_READ_VIEW"),
	STRUCT_FLD(author, PLUGIN_AUTHOR),
	STRUCT_FLD(descr, "InnoDB Read View information"),
	STRUCT_FLD(license, PLUGIN_LICENSE_GPL),
	STRUCT_FLD(init, xtradb_read_view_init),
	STRUCT_FLD(deinit, i_s_common_deinit),
	STRUCT_FLD(version, INNODB_VERSION_SHORT),
	STRUCT_FLD(status_vars, NULL),
	STRUCT_FLD(system_vars, NULL),
	STRUCT_FLD(__reserved1, NULL),
	STRUCT_FLD(flags, 0UL),
};
