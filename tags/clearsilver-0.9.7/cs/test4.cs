
Testing cs set

<?cs set:Set.1 = $5 ?>

<?cs var:Set.1 ?>

<?cs set:Set.2 = #5 + #5 * #5?>

<?cs var:Set.2 ?>

<?cs set:Set.3 = Foo + " " + Blah ?>
<?cs var:Set.3 ?>

<?cs # test increment ?>
<?cs set:Set.1 = #5 ?>
<?cs set:Set.1 = Set.1 + #1 ?>
<?cs var:Set.1 ?>

<?cs each:set = Set ?>
  <?cs set:set.5 = "The end of the world." ?>
<?cs /each ?>

<?cs var:Set.1.5 ?>

<?cs if:#5 % #2 ?>
 5 % 2 == true
<?cs else ?>
ERROR! um, wrong?
 5 % 2 == false?
<?cs /if ?>

<?cs if:#4 % #2 ?>
 um, wrong?
ERROR! 4 % 2 == true?
<?cs else ?>
 4 % 2 == false
<?cs /if ?>

<?cs if:#4 > #0 ?>
  Yes, 4 > 0
<?cs else ?>
ERROR!  Wrong, 4 is not < 0
<?cs /if ?>

<?cs if:#4 < #0 ?>
ERROR!  4 > 0
<?cs else ?>
right, 4 > 0
<?cs /if ?>

<?cs if:#0 <= #5 ?>
right, 0 <= 5
<?cs else ?>
ERROR! 0 <= 5
<?cs /if ?>

<?cs if:#0 >= #5 ?>
ERROR! 0 >= 5
<?cs else ?>
right, 0 >= 5
<?cs /if ?>

<?cs if:"0" <= #5 ?>
right "0" <= #5
<?cs else ?>
ERROR! "0" <= #5
<?cs /if ?>

<?cs # -- double digits -- ?>

<?cs if:"9" > #14 ?>
ERROR! "9" > #14
<?cs else ?>
right "9" < #14
<?cs /if ?>


<?cs # --- explicit strings --- ?>

<?cs if:"9" > "14" ?>
ERROR "9" > "14" (strings)
<?cs else ?>
right "9" < "14" (strings)
<?cs /if ?>

<?cs # --- hdf strings --- ?>

<?cs if:Numbers.hdf9 > Numbers.hdf14 ?>
ERROR! hdf 9 > hdf 14
<?cs else ?>
right hdf "9" < hdf "14"
<?cs /if ?>


<?cs each:msg = CGI.box.msgs ?>

<?cs set:row_url = "/box_bm_body_frm.cs?boxid=" +
                        Query.boxid + "&cur=" + msg.ticket_id +
                       "&idx_cur=" + CGI.box.cur.min_box_idx +
                       "&split=" + Query.split  +
                       "&filter=" + Query.filter +
                       "&sort=" + Query.sort +
                       "&sort_dir=" + Query.sort_dir +
                       "&from_search=" + Query.from_search ?>

<?cs var:row_url ?>

<?cs /each ?>

<?cs each:msg = CGI.box.msgs ?>

<?cs set:row_url = "/box_bm_body_frm.cs?boxid=" +
                        boxid + "&cur=" + msg.ticket_id +
                       "&idx_cur=" + CGI.box.cur.min_box_idx +
                       "&split=" + split  +
                       "&filter=" + filter +
                       "&sort=" + sort +
                       "&sort_dir=" + sort_dir +
                       "&from_search=" + from_search ?>

<?cs var:row_url ?>

<?cs /each ?>


