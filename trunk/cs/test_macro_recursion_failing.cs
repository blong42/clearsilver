Here we begin with a macro recursion.
<?cs def:map_val(val) ?>
  <?cs var:val ?>
  <?cs call:map_val(val + 1) ?>
<?cs /def ?>
<?cs call:map_val(#0) ?>
End of the macro recursion with problems. This line should not be displayed.