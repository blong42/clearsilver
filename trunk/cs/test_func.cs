testing func calls

Testing register strfunc
<?cs var:test_strfunc(Foo) ?>
<?cs var:test_strfunc("Foo" + Foo) ?>


<?cs var:test_strfunc((5,2)) ?>
<?cs var:test_strfunc((5,2) + 2) ?>

testing passing expressions to builtins
<?cs loop:x = 0, 20, 2 ?>
 <?cs var:abs(x - 5) ?>
<?cs /loop ?>

25: <?cs var:max(20-5, 20+5) ?>
15: <?cs var:min(20-5, 20+5) ?>

6: <?cs var:string.length("foo" + "bar") ?>

25: <?cs var:max(20-5, 20+5) ?>

4: <?cs var:subcount(Foo.Bar["Baz"]) ?>
Baz: <?cs var:name(Foo.Bar["Baz"]) ?>

oba: <?cs var:string.slice("foo" + "bar", 5-3, 2+3) ?>

first/last only run on local vars, which can't be created by expressions


