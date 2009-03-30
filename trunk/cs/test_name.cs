

<?cs var:name(Wow.Foo) ?>

<?cs each:count = Foo.Bar.Baz ?>
  <?cs var:name(count) ?>
<?cs /each ?>

<?cs if:name(Empty[Empty]) == #1 ?>
not here
<?cs /if ?>

<?cs if:name(Empty[notexist]) == ("A"||"B") ?>
not here either
<?cs /if ?>
