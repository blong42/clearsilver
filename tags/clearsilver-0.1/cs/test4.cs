
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
