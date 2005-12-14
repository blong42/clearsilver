
<?cs var:Foo ?>
<?cs loop:b=0,10 ?>
<?cs loop:e=10,0,-1 ?>
<?cs var:string.slice(Foo, b, e) ?>
<?cs /loop ?>
<?cs /loop ?>

Check end of string slice:
<?cs var:string.slice(Foo, -5, -1) ?>
<?cs var:string.slice(Foo, -5, 0) ?>
