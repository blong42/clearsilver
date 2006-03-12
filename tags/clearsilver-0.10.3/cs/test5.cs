
<?cs if:Neg == #-1 ?>
  Correct, Neg is -1
<?cs /if ?>

<?cs if:Neg != #-1 ?>
  ERROR!
<?cs /if ?>

<?cs set: foo = #1 ?>
<?cs set:bar = #1 ?>

<?cs set:blah=#10?>
<?cs set:blah2=#10+blah?>
blah2 = <?cs var:blah2?>

<?cs var:My.Test ?>
<?cs var:My.Test2.Abbr ?>
