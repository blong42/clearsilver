
testing parenthesis for order of operations

<?cs var:#5 * (#4 + #3) + #5 ?> == 40
<?cs var:(#4 + #3) ?> == 7
<?cs var:#4 + #3 * #5 ?> == 19
<?cs var:(#4 + #3) * #5 ?> == 35
<?cs var:((#4 + #3) * #5) + #5 ?> == 40
<?cs var:#5 + ((#4 + #3) * #5) ?> == 40
<?cs var:(#4 + #5) * (#3 + #6) ?> == 81
<?cs var:#6 + #5 * (#5 + #3) ?> == 46
<?cs var:(#6 + #3) + #5 * (#5 + #3) ?> == 49

testing brackets for hdf var arrays

<?cs var:v[#5+#3] ?>
<?cs var:#3 + v[#5+#3] + #6 ?>

<?cs var:Days[#0] ?> == 0
<?cs var:Days[#1] ?> == 1
<?cs var:Days[#2] ?> == 2

<?cs var:Days[#0]["Abbr"] ?> == Mon
<?cs var:Days[#1]["Abbr"] ?> == Tues
<?cs var:Days[#2]["Abbr"] ?> == Wed

<?cs set:ins = "Inside" ?>
<?cs each:in=Outside[#1][ins] ?> <?cs var:in ?><?cs /each ?> == 2 3

<?cs loop:x=#1,#20 ?><?cs set:foo[x] = x ?><?cs /loop ?>
<?cs loop:x=#1,#20 ?><?cs var:foo[x] ?> == <?cs var:x ?>
<?cs /loop ?>
<?cs each:x=foo ?><?cs var:x ?>
<?cs /each ?>
