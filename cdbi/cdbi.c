

#include "cs_config.h"

#include <stdlib.h>
#include <string.h>
#include <dbi/dbi.h>

#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/neo_str.h"

#include "cdbi.h"


/* We should be using dbi_driver_quote_string, but that function SUCKS,
 * it frees the string you pass it, which means we'd have to dup
 * everything we were going to pass it, ugh */
NEOERR *cdbi_escape(char *buf, char **o_buf)
{
  NEOERR *err = NULL;
  STRING out_s;
  char *p, *b;

  string_init(&out_s);
  b = buf;
  p = strchr(b, '\'');
  while (p != NULL)
  {
    err = string_appendn (&out_s, b, p-b);
    if (err) break;
    err = string_append_char(&out_s, '\\');
    if (err) break;
    err = string_append_char(&out_s, *p);
    if (err) break;
    b = p+1;
    p = strchr(b, '\'');
  }
  if (!err) err = string_append(&out_s, b);
  if (err)
  {
    string_clear(&out_s);
    return nerr_pass(err);
  }
  if (out_s.buf)
  {
    *o_buf = out_s.buf;
  }
  else
  {
    *o_buf = strdup("");
  }
  return STATUS_OK;
}

static NEOERR *_expand_primary_key(CDBI_ROW *row, char **buf)
{
  NEOERR *err = NULL;
  STRING str;
  void *pk;
  long i;
  char *s, *esc = NULL;
  int x = 0;
  int first = 1;

  *buf = NULL;
  string_init(&str);

  while (row->_table->defn[x].name)
  {
    if (row->_table->defn[x].flags & DBF_PRIMARY_KEY)
    {
      if (!first) 
      {
	err = string_append(&str, " and ");
	if (err) break;
      }
      first = 0;
      err = string_append(&str, row->_table->defn[x].name);
      if (err) break;
      err = string_append_char(&str, '=');
      if (err) break;

      pk = (char *)row + row->_table->defn[x].pk_shadow_offset;
      switch (row->_table->defn[x].data_type) 
      {
	case kInteger:
	  i = *(int *)pk;
	  err = string_appendf(&str, "%ld", i);
	  break;
	case kVarString:
	case kFixedString:
	  s = (char *)pk;
	  err = cdbi_escape(s, &esc);
	  if (err) break;
	  err = string_appendf(&str, "'%s'", esc);
	  free(esc);
	  esc = NULL;
	  break;
	default:
	  err = nerr_raise(NERR_ASSERT, "We only support primary key's on integer/var/fixed string columns");
	  break;
      }
      if (err) break;
    }
    x++;
  }
  if (esc) free(esc);
  if (err) 
  {
    string_clear(&str);
    return nerr_pass(err);
  }
  if (str.buf)
  {
    *buf = str.buf;
  }
  else
  {
    return nerr_raise(NERR_ASSERT, "Empty Primary Key");
  }

  return STATUS_OK;
}

static NEOERR *_expand_update(CDBI_ROW *row, char **buf)
{
  NEOERR *err = NULL;
  STRING str;
  void *val;
  long i;
  char *s, *esc = NULL;
  int x = 0;

  *buf = NULL;
  string_init(&str);

  while (row->_table->defn[x].name)
  {
    if (x) 
    {
      err = string_append_char(&str, ',');
      if (err) break;
    }
    err = string_append(&str, row->_table->defn[x].name);
    if (err) break;
    err = string_append_char(&str, '=');
    if (err) break;

    val = (char *)row + row->_table->defn[x].offset;
    switch (row->_table->defn[x].data_type) 
    {
      case kInteger:
	i = *(int *)val;
	err = string_appendf(&str, "%ld", i);
	break;
      case kVarString:
      case kFixedString:
      case kBigString:
	s = (char *)val;
	err = cdbi_escape(s, &esc);
	if (err) break;
	err = string_appendf(&str, "'%s'", esc);
	free(esc);
	esc = NULL;
	break;
      default:
	err = nerr_raise(NERR_ASSERT, "Unknown column type %d", row->_table->defn[x].data_type);
	break;
    }
    if (err) break;
    x++;
  }
  if (esc) free(esc);
  if (err) 
  {
    string_clear(&str);
    return nerr_pass(err);
  }
  if (str.buf)
  {
    *buf = str.buf;
  }
  else
  {
    return nerr_raise(NERR_ASSERT, "Empty Update");
  }

  return STATUS_OK;
}

static NEOERR *_expand_insert(CDBI_ROW *row, char **cols, char **values)
{
  NEOERR *err = NULL;
  STRING values_s;
  STRING cols_s;
  void *val;
  long i;
  char *s, *esc = NULL;
  int x = 0;

  *cols = NULL;
  *values = NULL;
  string_init(&cols_s);
  string_init(&values_s);

  while (row->_table->defn[x].name)
  {
    if (!x) 
    {
      err = string_append_char(&cols_s, '(');
      if (err) break;
      err = string_append_char(&values_s, '(');
      if (err) break;
    }
    else 
    {
      err = string_append_char(&cols_s, ',');
      if (err) break;
      err = string_append_char(&values_s, ',');
      if (err) break;
    }
    err = string_append(&cols_s, row->_table->defn[x].name);
    if (err) break;

    val = (char *)row + row->_table->defn[x].offset;
    switch (row->_table->defn[x].data_type) 
    {
      case kInteger:
	i = *(int *)val;
	err = string_appendf(&values_s, "%ld", i);
	break;
      case kVarString:
      case kFixedString:
      case kBigString:
	s = (char *)val;
	err = cdbi_escape(s, &esc);
	if (err) break;
	err = string_appendf(&values_s, "'%s'", esc);
	free(esc);
	esc = NULL;
	break;
      default:
	err = nerr_raise(NERR_ASSERT, "Unknown column type %d", row->_table->defn[x].data_type);
	break;
    }
    if (err) break;
    x++;
  }
  if (!err)
    err = string_append_char(&cols_s, ')');
  if (!err) 
    err = string_append_char(&values_s, ')');
  if (esc) free(esc);
  if (err) 
  {
    string_clear(&cols_s);
    string_clear(&values_s);
    return nerr_pass(err);
  }
  if (cols_s.buf && values_s.buf)
  {
    *cols = cols_s.buf;
    *values = values_s.buf;
  }
  else
  {
    string_clear(&cols_s);
    string_clear(&values_s);
    return nerr_raise(NERR_ASSERT, "Empty Insert");
  }

  return STATUS_OK;
}

static NEOERR *_expand_cols(CDBI_TABLE *table, char **cols)
{
  NEOERR *err = NULL;
  STRING cols_s;
  int x = 0;

  *cols = NULL;
  string_init(&cols_s);

  while (table->defn[x].name)
  {
    if (x) 
    {
      err = string_append_char(&cols_s, ',');
      if (err) break;
    }
    err = string_append(&cols_s, table->defn[x].name);
    if (err) break;

    x++;
  }
  if (err) 
  {
    string_clear(&cols_s);
    return nerr_pass(err);
  }
  if (cols_s.buf)
  {
    *cols = cols_s.buf;
  }
  else
  {
    string_clear(&cols_s);
    return nerr_raise(NERR_ASSERT, "No Columns defined?");
  }

  return STATUS_OK;
}


/* FIXME: update & insert should handle some database errors like
 * "duplicate"  */
/* update table set a=a, b=b, c=c where a=a and b=b */
static NEOERR *_update_row(CDBI_ROW *row)
{
  NEOERR *err;
  char *err_msg = NULL;
  char *pk = NULL;
  char *data = NULL;
  STRING str;

  do
  {
    string_init(&str);
    err = _expand_primary_key(row, &pk);
    if (err) break;
    err = _expand_update(row, &data);
    if (err) break;
    err = string_appendf(&str, "update %s set %s where %s", row->_table->name, data, pk);
    if (err) break;
    ne_warn("SQL: %s", str.buf);
    if (dbi_conn_query(row->_db->conn, str.buf) == NULL)
    {
      dbi_conn_error(row->_db->conn, &err_msg);
      if (strstr(err_msg, "Duplicate entry") != NULL)
      {
	err = nerr_raise(NERR_DUPLICATE, "Duplicate entry (%s): %s", pk, err_msg);
      }
      else
      {
	err = nerr_raise(NERR_DB, "Update Failed for (%s): %s", pk, err_msg);
      }
    }
  } while (0);
  string_clear(&str);
  if (err_msg) free(err_msg);
  if (pk) free(pk);
  if (data) free(data);

  return nerr_pass(err);
}

static NEOERR *_delete_row(CDBI_ROW *row)
{
  NEOERR *err;
  char *err_msg = NULL;
  char *pk = NULL;
  STRING query;

  do
  {
    string_init(&query);
    err = _expand_primary_key(row, &pk);
    if (err) break;
    err = string_appendf(&query, "delete from %s where %s", row->_table->name, pk);
    if (err) break;
    ne_warn("SQL: %s", query.buf);
    if (dbi_conn_query(row->_db->conn, query.buf) == NULL)
    {
      dbi_conn_error(row->_db->conn, &err_msg);
      err = nerr_raise(NERR_DB, "Delete Failed for (%s): %s", pk, err_msg);
    }
  } while (0);
  string_clear(&query);
  if (err_msg) free(err_msg);
  if (pk) free(pk);

  return nerr_pass(err);
}

/* update table set a=a, b=b, c=c where a=a, b=b */
static NEOERR *_insert_row(CDBI_ROW *row)
{
  NEOERR *err;
  char *err_msg = NULL;
  char *values = NULL;
  char *cols = NULL;
  int x = 0;
  long i;
  void *p, *pk;
  STRING query;

  do
  {
    string_init(&query);
    err = _expand_insert(row, &cols, &values);
    if (err) break;
    err = string_appendf(&query, "insert into %s %s values %s", row->_table->name, cols, values);
    if (err) break;
    ne_warn("SQL: %s", query.buf);
    if (dbi_conn_query(row->_db->conn, query.buf) == NULL)
    {
      dbi_conn_error(row->_db->conn, &err_msg);
      if (strstr(err_msg, "Duplicate entry") != NULL)
      {
	err = nerr_raise(NERR_DUPLICATE, "Duplicate entry (%s): %s", values, err_msg);
      }
      else
      {
	err = nerr_raise(NERR_DB, "Insert Failed for (%s): %s", values, err_msg);
      }
      break;
    }
    /* This isn't a new row anymore */
    row->_new = 0;

    /* Now, we have to look for an auto_inc row */
    while (row->_table->defn[x].name)
    {
      if (row->_table->defn[x].flags & DBF_AUTO_INC)
      {
	if (row->_table->defn[x].data_type != kInteger)
	{
	  err = nerr_raise(NERR_ASSERT, "Auto Increment on non-Integer field not allowed");
	  break;
	}
	i = dbi_conn_sequence_last(row->_db->conn, row->_table->defn[x].name);
	p = ((char *)row) + row->_table->defn[x].offset;
	pk = ((char *)row) + row->_table->defn[x].pk_shadow_offset;
	*(int *)p = i;
	/* In the MySQL case, this is always true */
	if (row->_table->defn[x].flags & DBF_PRIMARY_KEY)
	  *(int *)pk = i;
      }
      x++;
    }
  } while (0);
  string_clear(&query);
  if (err_msg) free(err_msg);
  if (cols) free(cols);
  if (values) free(values);

  return nerr_pass(err);
}

NEOERR *cdbi_row_save(CDBI_ROW *row)
{
  if (row->_new)
  {
    /* Do Insert */
    return nerr_pass(_insert_row(row));
  }
  else
  {
    /* Do Update */
    return nerr_pass(_update_row(row));
  }
}

NEOERR *cdbi_row_delete(CDBI_ROW *row)
{
  if (!row->_new)
  {
    /* Do delete */
    return nerr_pass(_delete_row(row));
  }
  /* Do nothing */
  return STATUS_OK;
}

NEOERR *_cdbi_load(dbi_result result, CDBI_ROW *row)
{
  int x = 0;
  long i;
  const char *s;
  void *p, *pk;

  row->_new = 0;
  while (row->_table->defn[x].name)
  {
    p = ((char *)row) + row->_table->defn[x].offset;
    pk = ((char *)row) + row->_table->defn[x].pk_shadow_offset;
    switch (row->_table->defn[x].data_type) 
    {
      case kInteger:
	i = dbi_result_get_long(result, row->_table->defn[x].name);
	*(int *)p = i;
	if (row->_table->defn[x].flags & DBF_PRIMARY_KEY)
	  *(int *)pk = i;
	break;
      case kVarString:
      case kFixedString:
	s = dbi_result_get_string(result, row->_table->defn[x].name);
	if (s == NULL)
	{
	  ((char *)p)[0] = '\0';
	}
	else 
	{
	  strncpy(p, s, row->_table->defn[x].data_len);
	  ((char *)p)[row->_table->defn[x].data_len] = '\0';
	}
	if (row->_table->defn[x].flags & DBF_PRIMARY_KEY)
	{
	  if (s == NULL)
	  {
	    ((char *)pk)[0] = '\0';
	  }
	  else
	  {
	    strncpy(pk, s, row->_table->defn[x].data_len);
	    ((char *)pk)[row->_table->defn[x].data_len] = '\0';
	  }
	}
	break;
      case kBigString:
	s = dbi_result_get_string(result, row->_table->defn[x].name);
	p = strdup(s);
	if (p == NULL)
	{
	  return nerr_raise(NERR_NOMEM, "Unable to allocate memory for big string result");
	}
	break;
    }
    x++;
  }
  return STATUS_OK;
}

NEOERR *cdbi_row_new(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW **row)
{
  CDBI_ROW *myrow;

  myrow = (CDBI_ROW *) calloc (1, table->row_size);
  if (myrow == NULL)
    return nerr_raise(NERR_NOMEM, "Unable to allocate memory for new row for table %s", table->name);
  myrow->_new = 1;
  myrow->_db = db;
  myrow->_table = table;

  *row = myrow;
  return STATUS_OK;
}

NEOERR *cdbi_match_new(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW **row)
{
  CDBI_ROW *myrow;

  myrow = (CDBI_ROW *) calloc (1, table->row_size);
  if (myrow == NULL)
    return nerr_raise(NERR_NOMEM, "Unable to allocate memory for new row for table %s", table->name);
  
  myrow->_db = db;
  myrow->_table = table;

  *row = myrow;
  return STATUS_OK;
}

void cdbi_match_init(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *row)
{
  memset(row, 0, table->row_size);
  row->_db = db;
  row->_table = table;
}

NEOERR *cdbi_db_connect(CDBI_DB **db, char *driver_path, char *driver, char *host, char *dbname, char *user, char *pass)
{
  NEOERR *err;
  CDBI_DB *my_db;
  char *err_msg;
  int r;

  *db = NULL;
  my_db = (CDBI_DB *) calloc (1, sizeof(CDBI_DB));
  
  r = dbi_initialize(driver_path);
  if (r == -1)
  {
    free(my_db);
    return nerr_raise(NERR_DB, "Unable to initialize dbi");
  }
  if (r == 0)
  {
    free(my_db);
    return nerr_raise(NERR_DB, "No drivers found: %s", dlerror());
  }

  my_db->conn = dbi_conn_new(driver);
  if (my_db->conn == NULL)
  {
    free(my_db);
    return nerr_raise(NERR_DB, "Unable to create dbi conn for driver %s", driver);
  }

  my_db->name = strdup(dbname);
  if (my_db->name == NULL)
  {
    cdbi_db_close(&my_db);
    return nerr_raise(NERR_NOMEM, "Unable to alloc memory for dbname %s", dbname);
  }

  if (dbi_conn_set_option(my_db->conn, "host", host) == -1)
  {
    cdbi_db_close(&my_db);
    return nerr_raise(NERR_DB, "Unable to set dbi option host=%s", host);
  }
  if (dbi_conn_set_option(my_db->conn, "username", user) == -1)
  {
    cdbi_db_close(&my_db);
    return nerr_raise(NERR_DB, "Unable to set dbi option username=%s", user);
  }
  if (dbi_conn_set_option(my_db->conn, "password", pass) == -1)
  {
    cdbi_db_close(&my_db);
    return nerr_raise(NERR_DB, "Unable to set dbi option password=%s", pass);
  }
  if (dbi_conn_set_option(my_db->conn, "dbname", dbname) == -1)
  {
    cdbi_db_close(&my_db);
    return nerr_raise(NERR_DB, "Unable to set dbi option dbname=%s", dbname);
  }

  if (dbi_conn_connect(my_db->conn) == -1)
  {
    dbi_conn_error(my_db->conn, &err_msg);
    cdbi_db_close(&my_db);
    err = nerr_raise(NERR_DB, "Unable to connect to database %s:%s as %s: %s", host, dbname, user, err_msg);
    free(err_msg);
    return err;
  }

  /*
  if (dbi_conn_select_db(my_db->conn, my_db->name) == -1)
  {
    dbi_conn_error(my_db->conn, &err_msg);
    cdbi_db_close(&my_db);
    err = nerr_raise(NERR_DB, "Unable to select database %s:%s as %s: %s", host, dbname, user, err_msg);
    free(err_msg);
    return err;
  }
  */

  *db = my_db;

  return STATUS_OK;
}

void cdbi_db_close(CDBI_DB **db)
{
  if (db == NULL || *db == NULL) return;
  if ((*db)->name)
  {
    free((*db)->name);
  }
  if ((*db)->conn)
  {
    dbi_conn_close((*db)->conn);
  }
  free(*db);
  *db = NULL;
}

static NEOERR *_expand_match(CDBI_TABLE *table, CDBI_ROW *match, char **buf)
{
  NEOERR *err = NULL;
  STRING str;
  void *val;
  long i;
  char *s, *esc = NULL;
  int first = 1;
  int x = 0;

  *buf = NULL;
  if (match == NULL) return STATUS_OK;
  string_init(&str);

  while (table->defn[x].name)
  {
    val = (char *)match + table->defn[x].offset;

    switch (table->defn[x].data_type) 
    {
      case kInteger:
	i = *(int *)val;
	if (i != 0)
	{
	  err = string_appendf(&str, "%s%s=%ld", first ? "" : " and ", table->defn[x].name, i);
	  first = 0;
	}
	break;
      case kVarString:
      case kFixedString:
      case kBigString:
	s = (char *)val;
	if (s && s[0])
	{
	  err = cdbi_escape(s, &esc);
	  if (err) break;
	  err = string_appendf(&str, "%s%s='%s'", first ? "" : " and ", table->defn[x].name, esc);
	  free(esc);
	  esc = NULL;
	  first = 0;
	}
	break;
      default:
	err = nerr_raise(NERR_ASSERT, "Unknown column type %d", table->defn[x].data_type);
	break;
    }
    if (err) break;
    x++;
  }
  if (esc) free(esc);
  if (err) 
  {
    string_clear(&str);
    return nerr_pass(err);
  }
  if (str.buf)
  {
    *buf = str.buf;
  }
  else
  {
    *buf = NULL;
  }

  return STATUS_OK;
}

NEOERR *cdbi_fetchnvf(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *match, int *nrows, CDBI_ROW **rows, char *where, va_list ap)
{
  NEOERR *err;
  dbi_result result = NULL;
  CDBI_ROW *row = NULL;
  STRING query;
  char *err_msg = NULL;
  char *match_spec = NULL;
  char *where_exp = NULL;
  char *cols = NULL;
  int x;

  string_init(&query);

  do 
  {
    err = _expand_cols(table, &cols);
    if (err) break;

    /* Expand Match Spec */
    err = _expand_match(table, match, &match_spec);
    if (err) break;

    /* Expand where */
    if (where)
    {
      where_exp = vsprintf_alloc(where, ap);
    }

    err = string_appendf(&query, "select %s from %s", cols, table->name);
    if (err) break;
    if (match_spec && match_spec[0] && where_exp && where_exp[0])
    {
      err = string_appendf(&query, " where %s and %s", match_spec, where_exp);
    }
    else if (match_spec && match_spec[0])
    {
      err = string_appendf(&query, " where %s", match_spec);
    }
    else if (where_exp && where_exp[0])
    {
      err = string_appendf(&query, " where %s", where_exp);
    }
    if (err) break;
    if (match)
    {
      if (match->_order_by)
      {
	err = string_appendf(&query, " order by %s", match->_order_by);
	if (err) break;
      }
      if (match->_skip_to && match->_limit_to)
      {
	err = string_appendf(&query, " limit %d, %d", match->_skip_to, match->_limit_to);
	if (err) break;
      }
      else if (match->_limit_to)
      {
	err = string_appendf(&query, " limit %d", match->_limit_to);
	if (err) break;
      }
    }

    /* Run Query */
    ne_warn("SQL: %s", query.buf);
    result = dbi_conn_query(db->conn, query.buf);
    if (result == NULL)
    {
      dbi_conn_error(db->conn, &err_msg);
      err = nerr_raise(NERR_DB, "Query failed '%s', Query: %s", err_msg, query);
      break;
    }

    /* Load Results */
    *nrows = dbi_result_get_numrows(result);
    if (*nrows > 0)
    {
      *rows = (CDBI_ROW *) calloc (*nrows, table->row_size);
      if (*rows == NULL)
      {
	err = nerr_raise(NERR_NOMEM, "Unable to allocate memory for result rows");
	break;
      }

      for (x = 0; x < *nrows; x++)
      {
	if (!dbi_result_next_row(result)) break;
	row = (CDBI_ROW *) ((char *)(*rows) + x * table->row_size);
	row->_db = db;
	row->_table = table;

	err = _cdbi_load(result, row);
	if (err) break;
      }
      if (err) break;
    }
  } while (0);

  if (result) dbi_result_free(result);
  if (cols) free (cols);
  if (match_spec) free (match_spec);
  if (where_exp) free (where_exp);
  string_clear(&query);
  if (err)
  {
    /* free(*rows); */
    return nerr_pass(err);
  }
  return STATUS_OK;
}

NEOERR *cdbi_fetchnf(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *match, int *nrows, CDBI_ROW **rows, char *where, ...)
{
  NEOERR *err;
  va_list ap;

  va_start(ap, where);
  err = cdbi_fetchnvf(db, table, match, nrows, rows, where, ap);
  va_end(ap);
  return nerr_pass(err);
}

NEOERR *cdbi_fetchn(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *match, int *nrows, CDBI_ROW **rows)
{
  return nerr_pass(cdbi_fetchnvf(db, table, match, nrows, rows, NULL, NULL));
}

NEOERR *cdbi_fetch(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *match, CDBI_ROW **row)
{
  NEOERR *err;
  int nrows = 0;

  /* We could check the match spec to insure we are using at least one
   * unique row... */
  err = cdbi_fetchnvf(db, table, match, &nrows, row, NULL, NULL);
  if (!err) 
  {
    if (nrows == 0)
    {
      err = nerr_raise(NERR_NOT_FOUND, "No row matches");
    }
    else if (nrows > 1)
    {
      err = nerr_raise(NERR_ASSERT, "Multiple Rows Match Unique");
      /* cdbi_rows_free(row, nrows) */
    }
  }
  return nerr_pass(err);
}

/* delete from table where ... */
NEOERR *cdbi_deletevf(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *match, char *where, va_list ap)
{
  NEOERR *err;
  dbi_result result = NULL;
  STRING query;
  char *err_msg = NULL;
  char *match_spec = NULL;
  char *where_exp = NULL;

  string_init(&query);

  do 
  {
    /* Expand Match Spec */
    err = _expand_match(table, match, &match_spec);
    if (err) break;

    /* Expand where */
    if (where)
    {
      where_exp = vsprintf_alloc(where, ap);
    }

    err = string_appendf(&query, "delete from %s", table->name);
    if (err) break;
    if (match_spec && match_spec[0] && where_exp && where_exp[0])
    {
      err = string_appendf(&query, " where %s and %s", match_spec, where_exp);
    }
    else if (match_spec && match_spec[0])
    {
      err = string_appendf(&query, " where %s", match_spec);
    }
    else if (where_exp && where_exp[0])
    {
      err = string_appendf(&query, " where %s", where_exp);
    }
    else
    {
      err = nerr_raise(NERR_ASSERT, "no where clause, won't delete all rows");
    }
    if (err) break;

    /* Run Query */
    ne_warn("SQL: %s", query.buf);
    result = dbi_conn_query(db->conn, query.buf);
    if (result == NULL)
    {
      dbi_conn_error(db->conn, &err_msg);
      err = nerr_raise(NERR_DB, "Query failed '%s', Query: %s", err_msg, query);
      break;
    }
  } while (0);

  if (result) dbi_result_free(result);
  if (match_spec) free (match_spec);
  if (where_exp) free (where_exp);
  string_clear(&query);
  if (err)
  {
    return nerr_pass(err);
  }
  return STATUS_OK;
}

NEOERR *cdbi_delete(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *match)
{
  return nerr_pass(cdbi_deletevf(db, table, match, NULL, NULL));
}

NEOERR *cdbi_deletef(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *match, char *where, ...)
{
  NEOERR *err;
  va_list ap;

  va_start(ap, where);
  err = cdbi_deletevf(db, table, match, where, ap);
  va_end(ap);
  return nerr_pass(err);
}


NEOERR *cdbi_row_hdf_export(CDBI_ROW *row, HDF *hdf, char *prefix)
{
  NEOERR *err = NULL;
  CDBI_TABLE *table;
  HDF *obj;
  void *val;
  long i;
  char *s;
  int x = 0;

  if (row == NULL) return STATUS_OK;

  table = row->_table;

  err = hdf_get_node(hdf, prefix, &obj);
  if (err) return nerr_pass(err);

  while (table->defn[x].name)
  {
    val = (char *)row + table->defn[x].offset;

    switch (table->defn[x].data_type) 
    {
      case kInteger:
	i = *(int *)val;
	err = hdf_set_int_value(obj, table->defn[x].name, i);
	break;
      case kVarString:
      case kFixedString:
      case kBigString:
	s = (char *)val;
	if (s && s[0])
	{
	  err = hdf_set_value(obj, table->defn[x].name, s);
	  if (err) break;
	}
	break;
      default:
	err = nerr_raise(NERR_ASSERT, "Unknown column type %d", table->defn[x].data_type);
	break;
    }
    if (err) break;
    x++;
  }
  return nerr_pass(err);
}

NEOERR *cdbi_row_hdf_exportvf(CDBI_ROW *row, HDF *hdf, char *prefix, va_list ap) 
{
  NEOERR *err;
  char *prefix_exp;

  prefix_exp = vsprintf_alloc(prefix, ap);
  if (prefix_exp == NULL)
    return nerr_raise(NERR_NOMEM, "Unable to expand prefix %s", prefix);
  err = cdbi_row_hdf_export(row, hdf, prefix_exp);
  free(prefix_exp);
  return nerr_pass(err);
}

NEOERR *cdbi_row_hdf_exportf(CDBI_ROW *row, HDF *hdf, char *prefix, ...)
{
  NEOERR *err;
  va_list ap;
  va_start(ap, prefix);
  err = cdbi_row_hdf_exportvf(row, hdf, prefix, ap);
  va_end(ap);
  return nerr_pass(err);
}

NEOERR *cdbi_rows_hdf_export(CDBI_ROW *rows, int nrows, HDF *hdf, char *prefix)
{
  NEOERR *err;
  int x;

  for (x = 0; x < nrows; x++)
  {
    err = cdbi_row_hdf_exportf((CDBI_ROW *)((char *)rows + (x * rows[0]._table->row_size)), hdf, "%s.%d", prefix, x);
    if (err) return nerr_pass(err);
  }
  return STATUS_OK;
}

NEOERR *cdbi_rows_hdf_exportvf(CDBI_ROW *rows, int nrows, HDF *hdf, char *prefix, va_list ap)
{
  NEOERR *err;
  char *prefix_exp;

  prefix_exp = vsprintf_alloc(prefix, ap);
  if (prefix_exp == NULL)
    return nerr_raise(NERR_NOMEM, "Unable to expand prefix %s", prefix);
  err = cdbi_rows_hdf_export(rows, nrows, hdf, prefix_exp);
  free(prefix_exp);
  return nerr_pass(err);
}

NEOERR *cdbi_rows_hdf_exportf(CDBI_ROW *rows, int nrows, HDF *hdf, char *prefix, ...)
{
  NEOERR *err;
  va_list ap;
  va_start(ap, prefix);
  err = cdbi_rows_hdf_exportvf(rows, nrows, hdf, prefix, ap);
  va_end(ap);
  return nerr_pass(err);
}
