
#include "ClearSilver.h"

#include "cdbi.h"
#include "cdbi_ab.h"

int main(int argc, char **argv)
{
  NEOERR *err;
  CDBI_DB *db;
  peopleRow *results = NULL;
  peopleRow *match = NULL;
  peopleRow *new_row = NULL;
  int nresults = 0;
  int x = 0;
  
  do {
    err = cdbi_db_connect(&db, NULL, "mysql", "localhost", "blong", "blong", "qwerty");
    if (err) break;

    err = cdbi_match_new(db, &peopleTable, (CDBI_ROW **)&match);
    if (err) break;
    strcpy(match->first_name, "Brian");

    err = cdbi_fetchn(db, &peopleTable, (CDBI_ROW *)match, &nresults, (CDBI_ROW **)&results);
    if (err) break;
    for (x = 0; x < nresults; x++)
    {
      ne_warn("%d: %s", results[x].person_id, results[x].fullname);
    }

    err = cdbi_fetchnf(db, &peopleTable, NULL, &nresults, (CDBI_ROW **)&results, "fullname like '%%Jen%%'");
    if (err) break;
    for (x = 0; x < nresults; x++)
    {
      ne_warn("%d: %s", results[x].person_id, results[x].fullname);
    }

    /* Now, test insertion */
    err = cdbi_row_new(db, &peopleTable, (CDBI_ROW **)&new_row);
    if (err) break;
    strcpy(new_row->first_name, "Brandon");
    strcpy(new_row->last_name, "Long");
    strcpy(new_row->fullname, "Brandon Long");

    err = cdbi_row_save((CDBI_ROW *)new_row);
    if (err) break;
    ne_warn("New person_id is %d", new_row->person_id);

  } while (0);

  cdbi_db_close(&db);

  if (err)
  {
    nerr_log_error(err);
  }
  return 0;
}
