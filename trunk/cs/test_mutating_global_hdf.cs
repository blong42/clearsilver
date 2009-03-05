
This shouldn't be able to modify the global tree.

Original: <?cs var:GlobalTree.0.Bar ?>

<?cs each:child = GlobalTree ?>
  <?cs set:child.Bar = 'Foo' ?>
<?cs /each ?>

After: <?cs var:GlobalTree.0.Bar ?>
