<?cs def:macro1(arg1, arg2) ?><?cs
  alt:arg1 ?>RIGHT<?cs /alt ?><?cs
/def ?>
<?cs def:macro2(arg1, arg2) ?><?cs
  call:macro1(arg2, arg1) ?><?cs
/def ?>
<?cs call:macro2(A, nonExistentVar) ?>
