
LOOP Test

1, 3, 5
<?cs loop:x = #1, #5, #2 ?><?cs var:x ?>, <?cs /loop ?>

1, 3, 5... 205
<?cs loop:x = #1, #205, #2 ?><?cs var:x ?>, <?cs /loop ?>

backwards
<?cs loop:x = #205, #1, "-2" ?><?cs var:x ?>, <?cs /loop ?>

broken
<?cs loop:x = #1, #205, #-2 ?><?cs var:x ?>, <?cs /loop ?>

<?cs def:do_loop(var1, var2, var3) ?>
  <?cs loop:x = var1, var2, var3 ?><?cs var:x ?>, <?cs /loop ?>
<?cs /def ?>

<?cs call:do_loop(#1, #20, #2) ?>
