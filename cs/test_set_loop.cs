Loop[foo] coerces the num to string and then the set overwrites it, had
memory leak before.
<?cs loop:foo = #0,#3 ?><?cs set:Loop[foo] = foo ?><?cs var:Loop[foo] ?><?cs set:foo = #0 ?><?cs var:foo ?><?cs /loop ?>
