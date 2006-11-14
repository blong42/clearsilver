
<?cs each:num = Numbers ?>
  <?cs var:num ?> 
  is_first: <?cs var:first(num) ?> 
  is_last: <?cs var:last(num) ?> 
<?cs /each ?>

<?cs each:day = Days ?>
  <?cs var:day ?> 
  is_first: <?cs var:first(day) ?> 
  is_first: <?cs var:first(day.Abbr) ?> -> never
  is_last: <?cs var:last(day) ?> 
  is_last: <?cs var:last(day.Abbr) ?>  -> never
<?cs /each ?>

Only one, so should be first and last
<?cs each:test = My.Test2 ?>
  is_first: <?cs var:first(test) ?> 
  is_last: <?cs var:last(test) ?> 
<?cs /each ?>

Testing loop
<?cs loop:x = 0, 50, 5 ?>
  <?cs var:x ?>
  is_first: <?cs var:first(x) ?> 
  is_first: <?cs var:first(x.foo) ?> -> never
  is_last: <?cs var:last(x) ?> 
  is_last: <?cs var:last(x.foo) ?> -> never
<?cs /loop ?>
