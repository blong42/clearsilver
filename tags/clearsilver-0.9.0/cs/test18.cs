
<?cs set:foo["baz"].right = "dot right" ?>
<?cs set:foo["baz"].right.label = "dot right" ?>

<? cs set:foo["baz"] + right = "plus right" ?>
<?cs set:Files.0.Funk = "bam" ?>


<?cs def:style() ?>....<?cs /def ?>

<?cs call:style() ?>

with whitespace
<?cs call:style( ) ?>

with extra variable
<?  cs call:style( foo ) ?>

With whitespace in def...
<?cs def:style2( ) ?>....<?cs /def ?>

<?cs call:style2() ?>


<?cs def:set_info(side, label, value) ?>
   <?cs set:rows[row][side].label = label ?>
   <?cs set:rows[row][side].value = value ?>
   <?cs # the following test would throw a warning if enabled ?>
   <?cs # set:side.foo = row ?>
   <?cs set:row = row + #1 ?>
<?cs /def ?>

<?cs set:row = #0 ?>
<?cs call:set_info("left", "phone", Days.0.Abbr) ?>

