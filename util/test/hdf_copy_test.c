
#include <stdio.h>
#include <neo_hdf.h>

int main(void) {
  HDF *hdf_1;
  HDF *hdf_2;
  HDF *cur_node,*last_node;

  hdf_init(&hdf_1);

  hdf_read_file(hdf_1,"hdf_copy_test.hdf");

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

  return 0;
}
