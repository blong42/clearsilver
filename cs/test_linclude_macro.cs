<?cs def:macro1() ?>This is macro1 in main<?cs /def ?>
<?cs linclude:"test_lincluded_macro.cs" ?>
Calling macro1 from main: <?cs call:macro1() ?>
