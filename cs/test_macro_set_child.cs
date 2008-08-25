<?cs # Test passing a non-existent HDF node to a macro, having it add on a child reference and
 then pass it on to another macro to output or set ?>
<?cs def:say(name) ?><?cs var:name ?><?cs /def?>
<?cs def:write(name, value) ?>"<?cs var:name ?>" -> <?cs set:name = value ?>"<?cs var:name ?>"<?cs /def?>
<?cs def:childSay(parent, child) ?><?cs call:say(parent[child]) ?><?cs /def?>
<?cs def:childWrite(parent, child, value) ?><?cs call:write(parent[child], value) ?><?cs /def?>
Start: "<?cs var:A.B ?>"
ChildSay: "<?cs call:childSay(A, "B") ?>"
AfterSay: "<?cs var:A.B ?>"
ChildWrite: <?cs call:childWrite(A, "B", "A.B") ?>
<?cs if:!?A.B ?>A.B was not set!<?cs /if ?>
AfterWrite: "<?cs var:A.B ?>"
ChildSay: "<?cs call:childSay(A, "B") ?>"
