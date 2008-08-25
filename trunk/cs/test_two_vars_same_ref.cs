Testing local variable resolution when two vars point to same non-existent node and one sets it.
<?cs def:aMacro(arg1, arg2) ?><?cs
	set:arg1 = "Set on arg1" ?><?cs
	var:arg2 ?><?cs
/def ?>
Empty: <?cs var:foo ?>
<?cs call:aMacro(foo, foo) ?>
Not empty: <?cs var:foo ?>
