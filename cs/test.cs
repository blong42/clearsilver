
Start of File

<?cs if:Blah == "wow" ?>
  Blah == wow
<?cs else ?>
  Blah != wow
<?cs /if ?>


<?cs  if:!#0 ?>

<?cs if:arg1 ?>
wow (true)
<?cs else ?>
other (false)
<?cs /if ?>

<?cs if:#5 ?>
  This is True
<?cs /if ?>

<?cs if:Blah == Foo ?>
<?cs /if ?>

<?cs if:Blah < #5 ?>
  <?cs var:Blah ?>
<?cs /if ?>

<?cs include!"test2.cs" ?>
<?cs linclude!"test2.cs" ?>

<?cs include!"test_escape.cs" ?>
<?cs escape: "html" ?><?cs call:echo_all(Title+BlahJs:UrlArg) ?><?cs /escape ?>

<?cs each: x=Foo.Bar.Baz ?>
  x = <?cs var:x ?>
  x.num = <?cs var:x.num ?>

<?cs if:#1 ?>
  This is True.
<?cs /if ?>
wow
<?cs /each ?>
<?cs /if ?>

<?cs if:#Wow.Foo ?>
  This is False.
<?cs /if ?>

<?cs each:x=Outside ?>
  Outside <?cs name:x ?>
  <?cs each:y=x.Inside ?>
    Inside = <?cs var:y ?>
  <?cs /each ?>
<?cs /each ?>

<?cs if:TestIf == "0" ?>
  TestIf == 0
<?cs elif:TestIf == "1" ?>
  <?cs var:TestIf ?>
  TestIf == 1
<?cs else ?>
  TestIf == else
<?cs /if ?>

<?cs if:"1" == "1" ?>
Correct, "1" == "1"
<?cs else ?>
WRONG, "1" != "1"
<?cs /if ?>

<?cs # This is a ClearSilver Comment ?>

between comments

<?cs ##########################################################
     # A multi-line
     # comment 
?>

More?
