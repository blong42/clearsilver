Here we begin with a macro recursion.
<?cs def:map_val(val) ?>
  <?cs var:val ?>
  <?cs if:val<49 ?> 
    <?cs call:map_val(val + 1) ?>
  <?cs /if ?>
<?cs /def ?>
<?cs call:map_val(#0) ?>
End of the macro recursion with no problems.
