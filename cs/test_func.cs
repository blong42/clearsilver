testing func calls

Testing register strfunc
<?cs var:test_strfunc(Foo) ?>
<?cs var:test_strfunc("Foo" + Foo) ?>


<?cs var:test_strfunc((5,2)) ?>
<?cs var:test_strfunc((5,2) + 2) ?>
