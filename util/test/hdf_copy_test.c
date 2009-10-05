
#include "cs_config.h"

#include <stdio.h>
#include <string.h>

#include "util/neo_misc.h"
#include "util/neo_hdf.h"

/* Test copy and destroy, make sure we actually copy everything and
 * don't reference anything */
NEOERR *test_copy_destroy() {
  NEOERR *err;
  HDF *hdf_1, *hdf_2;

  ne_warn("Running test_copy_destroy");

  hdf_init(&hdf_1);
  hdf_init(&hdf_2);
  err = hdf_read_file(hdf_1, "hdf_copy_test.hdf");
  if (err) return nerr_pass(err);
  err = hdf_copy(hdf_2, "", hdf_1);
  if (err) return nerr_pass(err);
  hdf_destroy(&hdf_1);

  /* Walk the entire hierarchy after destruction to prove sanity */
  hdf_dump(hdf_2, NULL);
  hdf_destroy(&hdf_2);

  return STATUS_OK;
}

NEOERR *test_link_copy() {
  NEOERR *err;
  HDF *hdf_1, *hdf_2;
  char *value;

  ne_warn("Running test_link_copy");

  hdf_init(&hdf_1);
  hdf_init(&hdf_2);

  err = hdf_set_value(hdf_1, "Foo.Bar", "baz");
  if (err) return nerr_pass(err);

  err = hdf_set_value(hdf_1, "Foo.Bar.Bang", "barndoor");
  if (err) return nerr_pass(err);

  err = hdf_set_symlink(hdf_1, "Foo.Tar", "Foo.Bar");
  if (err) return nerr_pass(err);

  value = hdf_get_value(hdf_1, "Foo.Tar", NULL);
  if (value == NULL || strcmp(value, "baz")) {
    return nerr_raise(NERR_ASSERT,
                      "Symlink not followed, expected baz, got: %s",
                      value ? value :"NULL");
  }

  value = hdf_get_value(hdf_1, "Foo.Tar.Bang", NULL);
  if (value == NULL || strcmp(value, "barndoor")) {
    return nerr_raise(NERR_ASSERT,
                      "Symlink not followed, expected barndoor, got: %s",
                      value ? value :"NULL");
  }

  err = hdf_copy(hdf_2, "", hdf_1);
  if (err) return nerr_pass(err);

  value = hdf_get_value(hdf_2, "Foo.Tar", NULL);
  if (value == NULL || strcmp(value, "baz")) {
    return nerr_raise(NERR_ASSERT,
                      "Symlink not followed, expected baz, got: %s",
                      value ? value :"NULL");
  }

  value = hdf_get_value(hdf_2, "Foo.Tar.Bang", NULL);
  if (value == NULL || strcmp(value, "barndoor")) {
    return nerr_raise(NERR_ASSERT,
                      "Symlink not followed, expected barndoor, got: %s",
                      value ? value :"NULL");
  }

  /* Copy with-in the same tree */
  err = hdf_copy(hdf_1, "NewTop", hdf_get_obj(hdf_1, "Foo"));
  if (err) return nerr_pass(err);

  /* Note, the symlink still points to the top level, ie the same as before */
  value = hdf_get_value(hdf_1, "NewTop.Tar", NULL);
  if (value == NULL || strcmp(value, "baz")) {
    return nerr_raise(NERR_ASSERT,
                      "Symlink not followed, expected baz, got: %s",
                      value ? value :"NULL");
  }

  value = hdf_get_value(hdf_1, "NewTop.Tar.Bang", NULL);
  if (value == NULL || strcmp(value, "barndoor")) {
    return nerr_raise(NERR_ASSERT,
                      "Symlink not followed, expected barndoor, got: %s",
                      value ? value :"NULL");
  }

  /* Change the top level value, so the links should return the new value */
  err = hdf_set_value(hdf_1, "Foo.Bar" , "different");
  if (err) return nerr_pass(err);

  value = hdf_get_value(hdf_1, "NewTop.Tar", NULL);
  if (value == NULL || strcmp(value, "different")) {
    return nerr_raise(NERR_ASSERT,
                      "Symlink not followed, expected different, got: %s",
                      value ? value :"NULL");
  }

  hdf_destroy(&hdf_1);
  hdf_destroy(&hdf_2);

  return STATUS_OK;
}

int main(void) {
  NEOERR *err;

  HDF *hdf_1, *hdf_2;
  HDF *cur_node,*last_node;

  hdf_init(&hdf_1);

  hdf_read_file(hdf_1, "hdf_copy_test.hdf");
  hdf_dump(hdf_1,NULL);

  cur_node = hdf_get_obj(hdf_1,"Chart");
  last_node = cur_node;

  cur_node = hdf_get_obj(cur_node,"next_stage");
  while (hdf_get_obj(cur_node,"next_stage") && strcmp(hdf_get_value(cur_node,"Bucket.FieldId",""),"QUEUE")) {
    last_node = cur_node;
    cur_node = hdf_get_obj(cur_node,"next_stage");
  }

  if (hdf_get_obj(cur_node,"next_stage")) {
    hdf_copy(hdf_1,"TempHolderPlace",hdf_get_obj(cur_node,"next_stage"));
  }
  ne_warn("Delete tree from node: %s", hdf_obj_name(last_node));
  hdf_remove_tree(last_node,"next_stage");

  hdf_dump(hdf_1,NULL);
  fprintf(stderr,"-----------------\n");


  hdf_copy(last_node,"next_stage",hdf_get_obj(hdf_1,"TempHolderPlace"));
  hdf_dump(hdf_1,NULL);

  err = test_copy_destroy();
  if (err) {
    nerr_log_error(err);
    return -1;
  }

  err = test_link_copy();
  if (err) {
    nerr_log_error(err);
    return -1;
  }

  return 0;
}
