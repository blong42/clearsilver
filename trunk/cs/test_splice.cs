
<?cs var:Foo ?>
<?cs loop:b=0,10 ?>
<?cs loop:e=10,0,-1 ?>
<?cs var:string.splice(Foo, b, e) ?>
<?cs /loop ?>
<?cs /loop ?>
<?cs var:string.splice(Foo, -5, -1) ?>
