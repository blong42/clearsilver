
This should work by creating an element in the local HDF tree that hides
the one in the Global tree.  cstest should validate that the global hdf
tree wasn't modified.

Original: <?cs var:GlobalTree.Foo ?>

<?cs set:GlobalTree.Foo = GlobalTree.0 ?>

After: <?cs var:GlobalTree.Foo ?>
