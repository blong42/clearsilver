

#ifndef __CDBI_H_
#define __CDBI_H_ 1

#include <dbi/dbi.h>

#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/neo_hdf.h"


__BEGIN_DECLS

typedef struct _cdbi_db {
  dbi_conn conn;
  char *name;
} CDBI_DB;

typedef struct _cdbi_table {
  char *name;
  struct _cdbi_table_def *defn;
  int row_size;
} CDBI_TABLE;

typedef struct _cdbi_table_def {
  int data_type;
  int data_len;
  char *name;
  int flags;
  int offset;
  int pk_shadow_offset;
} CDBI_TABLE_DEF;

#define Row_HEAD \
	CDBI_DB *_db; \
	CDBI_TABLE *_table; \
	int _new; \
	int _empty; \
	int _flags; \
	int _skip_to; \
	int _limit_to; \
	char *_order_by;

typedef struct _cdbi_row {
  Row_HEAD
} CDBI_ROW;

#define DBF_PRIMARY_KEY (1<<0)
#define DBF_INDEXED     (1<<1)
#define DBF_UNIQUE      (1<<2)
#define DBF_AUTO_INC    (1<<3)
#define DBF_NOT_NULL    (1<<4)
#define DBF_TIME_T      (1<<5)

#define kInteger	1
#define kIncInteger	2
#define kFixedString	3
#define kVarString	4
#define kBigString	5

NEOERR *cdbi_db_connect(CDBI_DB **db, char *driver_path, char *dbname, char *driver, char *host, char *user, char *pass);
void cdbi_db_close(CDBI_DB **db);

/* Fetching a single row */
NEOERR *cdbi_fetch(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *match, CDBI_ROW **row);

/* Fetching multiple rows */
/* These all fetch all rows, we might want an interface to return only
 * the first x rows, and then more rows */
NEOERR *cdbi_fetchn(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *match, int *nrows, CDBI_ROW **rows);
NEOERR *cdbi_fetchnf(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *match, int *nrows, CDBI_ROW **rows, char *where, ...);
NEOERR *cdbi_fetchnvf(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *match, int *nrows, CDBI_ROW **rows, char *where, va_list args);

/* Delete rows */
NEOERR *cdbi_delete(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *match);
NEOERR *cdbi_deletef(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *match, char *where, ...);

NEOERR *cdbi_match_new(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW **row);
void cdbi_match_init(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW *match);

/* Updates */
NEOERR *cdbi_row_new(CDBI_DB *db, CDBI_TABLE *table, CDBI_ROW **row);
NEOERR *cdbi_row_save(CDBI_ROW *row);
NEOERR *cdbi_row_delete(CDBI_ROW *row);
void cdbi_row_free(CDBI_ROW *row);
/* You can access the data directly via the row struct, but sometimes
 * you want to do it programmatically (ie, construct the name of the
 * column you are accessing dynamically at run time) and you can use
 * these functions for that */
NEOERR *cdbi_row_get_str(CDBI_ROW *row, const char *colname, const char **value);
NEOERR *cdbi_row_get_int(CDBI_ROW *row, const char *colname, const int *value);
NEOERR *cdbi_row_get_float(CDBI_ROW *row, const char *colname, const double *value);
NEOERR *cdbi_row_set_str(CDBI_ROW *row, const char *colname, const char *value);
NEOERR *cdbi_row_set_int(CDBI_ROW *row, const char *colname, const int value);
NEOERR *cdbi_row_set_float(CDBI_ROW *row, const char *colname, const double value);
NEOERR *cdbi_row_inc_int(CDBI_ROW *row, const char *colname, const int value);

/* Exporting the data to HDF */
NEOERR *cdbi_row_hdf_export(CDBI_ROW *row, HDF *hdf, char *tz, char *prefix);
NEOERR *cdbi_row_hdf_exportvf(CDBI_ROW *row, HDF *hdf, char *tz, char *prefix, va_list ap);
NEOERR *cdbi_row_hdf_exportf(CDBI_ROW *row, HDF *hdf, char *tz, char *prefix, ...);
NEOERR *cdbi_rows_hdf_export(CDBI_ROW *rows, int nrows, HDF *hdf, char *tz, char *prefix);
NEOERR *cdbi_rows_hdf_exportvf(CDBI_ROW *rows, int nrows, HDF *hdf, char *tz, char *prefix, va_list ap); 
NEOERR *cdbi_rows_hdf_exportf(CDBI_ROW *rows, int nrows, HDF *hdf, char *tz, char *prefix, ...);

__END_DECLS

#endif /* __CDBI_H_ */
