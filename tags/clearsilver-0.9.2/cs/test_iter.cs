
<?cs loop:foo = #0,#30 ?>
  <?cs set:Loop[foo] = foo ?>
<?cs /loop ?>

<?cs set:count = #0 ?>
<?cs set:Query.start = #10 ?>
<?cs set:next = #20 ?>
<?cs each:foo = Loop ?>
  <?cs if:#count > #Query.start && #count < #next ?>
  <?cs var:foo ?>
  <?cs /if ?>
  <?cs set:count = #count + #1 ?>
<?cs /each ?>
