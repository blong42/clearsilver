
Testing cs set

<?cs set:Set.1 = 5 ?>

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
