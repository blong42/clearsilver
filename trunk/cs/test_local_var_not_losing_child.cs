<?cs def:aMacro(foo) ?><?cs
  each:item = foo.Sub ?><?cs
    var:name(item) ?>  <?cs
  /each ?><?cs
/def ?>
<?cs call:aMacro(unknownVar) ?>
<?cs call:aMacro(Files.0) ?>
