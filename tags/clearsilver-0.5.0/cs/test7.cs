
LOOP Test

1, 3, 5
<?cs loop:x = #1, #5, #2 ?><?cs var:x ?>, <?cs /loop ?>

1, 3, 5... 205
<?cs loop:x = #1, #205, #2 ?><?cs var:x ?>, <?cs /loop ?>

backwards
<?cs loop:x = #205, #1, "-2" ?><?cs var:x ?>, <?cs /loop ?>

broken
<?cs loop:x = #1, #205, #-2 ?><?cs var:x ?>, <?cs /loop ?>
