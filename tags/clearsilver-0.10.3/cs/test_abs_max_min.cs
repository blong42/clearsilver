
abs(-10) = <?cs var:abs(-10) ?>
abs("-10") = <?cs var:abs("-10") ?>
abs("10") = <?cs var:abs("10") ?>
abs("0") = <?cs var:abs("0") ?>

<?cs loop:x = -5, 5, 1 ?>
  abs(<?cs var:x ?>) = <?cs var:abs(x) ?>
<?cs /loop ?>

min(5,10) = <?cs var:min(5, 10) ?>
min(-5,10) = <?cs var:min(-5, 10) ?>
min(-5,-10) = <?cs var:min(-5, -10) ?>
min(5,-10) = <?cs var:min(5, -10) ?>

max(5,10) = <?cs var:max(5, 10) ?>
max(-5,10) = <?cs var:max(-5, 10) ?>
max(-5,-10) = <?cs var:max(-5, -10) ?>
max(5,-10) = <?cs var:max(5, -10) ?>

<?cs loop:x = -10, 10, 1 ?>
  <?cs loop:y = -10, 10, 2 ?>
    max(<?cs var:x ?>, <?cs var:y ?>) = <?cs var:max(x,y) ?>
    max(<?cs var:y ?>, <?cs var:x ?>) = <?cs var:max(y,x) ?>
    min(<?cs var:x ?>, <?cs var:y ?>) = <?cs var:min(x,y) ?>
    min(<?cs var:y ?>, <?cs var:x ?>) = <?cs var:min(y,x) ?>
  <?cs /loop ?>
<?cs /loop ?>
