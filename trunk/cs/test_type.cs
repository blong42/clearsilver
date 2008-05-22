Type tests, ensure that numerics and strings are parsed correctly

Addition as string:
149 : <?cs var: "14" + Numbers.hdf9 ?>
914 : <?cs var: Numbers.hdf9 + "14" ?>
914 : <?cs var: Numbers.hdf9 + Numbers.hdf14 ?>

Simple addition as numeric:
23 : <?cs var: 14 + 9 ?>
23 : <?cs var: 9 + 14 ?>
23 : <?cs var: #14 + "9" ?>
23 : <?cs var: "9" + #14 ?>
23 : <?cs var: 14 + Numbers.hdf9 ?>
23 : <?cs var: Numbers.hdf9 + 14 ?>
23 : <?cs var: #14 + Numbers.hdf9 ?>
23 : <?cs var: Numbers.hdf9 + #14 ?>
23 : <?cs var: "14" + #Numbers.hdf9 ?>
23 : <?cs var: #Numbers.hdf9 + "14" ?>
23 : <?cs var: Numbers.hdf9 + #Numbers.hdf14 ?>
23 : <?cs var: #Numbers.hdf9 + Numbers.hdf14 ?>
23 : <?cs var: #Numbers.hdf9 + #Numbers.hdf14 ?>
23 : <?cs var: 0XE + Numbers.hdf9 ?>
23 : <?cs var: Numbers.hdf9 + 0XE ?>

Left to right type inference (numeric):
25 : <?cs var: 2 + Numbers.hdf9 + Numbers.hdf14 ?>
25 : <?cs var: Numbers.hdf9 + #Numbers.hdf14 + 2 ?>
25 : <?cs var: "9" + #Numbers.hdf14 + 2 ?>

Left to right type inference (string/numeric):
916 : <?cs var: Numbers.hdf9 + Numbers.hdf14 + 2 ?>

Equality as string:
<?cs if:"9x" == Numbers.hdf9 + "x" ?>PASS<?cs else ?>FAIL<?cs /if ?>
<?cs if:"0x9" == "9" ?>FAIL<?cs else ?>PASS<?cs /if ?>
<?cs if:"0X9" == Numbers.hdf9 ?>FAIL<?cs else ?>PASS<?cs /if ?>

<?cs if:"0x9" != "9" ?>PASS<?cs else ?>FAIL<?cs /if ?>
<?cs if:"0X9" != Numbers.hdf9 ?>PASS<?cs else ?>FAIL<?cs /if ?>

Equality as number:
<?cs if:0xE == "14" ?>PASS<?cs else ?>FAIL<?cs /if ?>
<?cs if:"0XE" == 14 ?>PASS<?cs else ?>FAIL<?cs /if ?>
<?cs if:14 == Numbers.hdf14 ?>PASS<?cs else ?>FAIL<?cs /if ?>
<?cs if:0xE == Numbers.hdf14 ?>PASS<?cs else ?>FAIL<?cs /if ?>
<?cs if:"14" == #Numbers.hdf14 ?>PASS<?cs else ?>FAIL<?cs /if ?>
<?cs if:"0xE" == #Numbers.hdf14 ?>PASS<?cs else ?>FAIL<?cs /if ?>

<?cs if:0xE != "14" ?>FAIL<?cs else ?>PASS<?cs /if ?>
<?cs if:"0xE" != 14 ?>FAIL<?cs else ?>PASS<?cs /if ?>
<?cs if:14 != Numbers.hdf14 ?>FAIL<?cs else ?>PASS<?cs /if ?>
<?cs if:0xE != Numbers.hdf14 ?>FAIL<?cs else ?>PASS<?cs /if ?>
<?cs if:"14" != #Numbers.hdf14 ?>FAIL<?cs else ?>PASS<?cs /if ?>
<?cs if:"0xE" != #Numbers.hdf14 ?>FAIL<?cs else ?>PASS<?cs /if ?>

Comparators always as number:
<?cs if:"0xE" > "9" ?>PASS<?cs else ?>FAIL<?cs /if ?>
<?cs if:"0x9" < "14" ?>PASS<?cs else ?>FAIL<?cs /if ?>
