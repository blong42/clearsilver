
<?cs loop:x=#0,#6 ?>
  <?cs with:abbr = Days[x].Abbr ?>
    <?cs var:x ?> - <?cs var:abbr ?>
  <?cs /with ?>
<?cs /loop ?>
