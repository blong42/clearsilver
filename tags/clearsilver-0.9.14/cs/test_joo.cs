Testing Empty String and empty var
Empty has no value assigned (ie, empty), Foo has a value, NotExist doesn't exist
Some of these tests are pretty silly
----------------------------------------------------------------------------------

Testing  == '' 
[1] Empty == ''
<?cs if:Empty == '' ?>
PASS
<?cs else ?>
FAIL - empty string should equal empty string [1]
<?cs /if ?>
[2] Foo == ''
<?cs if:Foo == '' ?>
FAIL - existing var shouldn't equal empty string [2]
<?cs else ?>
PASS
<?cs /if ?>
[3] NotExist == ''
<?cs if:NotExist == '' ?>
FAIL - non-existing should be NULL, not empty string [3]
<?cs else ?>
PASS
<?cs /if ?>
	  
Testing  != '' 
[4] Empty != ''
<?cs if:Empty != '' ?>
FAIL - Empty var should equal empty string [4]
<?cs else ?>
PASS
<?cs /if ?>
[5] Foo != ''
<?cs if:Foo != '' ?>
PASS
<?cs else ?>
FAIL - [5]
<?cs /if ?>
[6] NotExist != ''
<?cs if:NotExist != '' ?>
PASS - Non existing var doesn't equal empty string
<?cs else ?>
FAIL - [6]
<?cs /if ?>
	  
Testing ? 
[7] ?Empty
<?cs if:?Empty ?>
PASS
<?cs else ?>
FAIL - [7]
<?cs /if ?>
[8] ?Foo
<?cs if:?Foo ?>
PASS
<?cs else ?>
FAIL - [8]
<?cs /if ?>
[9] ?NotExist
<?cs if:?NotExist ?>
FAIL - non existing var shouldn't exist [9]
<?cs else ?>
PASS
<?cs /if ?>

Testing ! 
[10] !Empty
<?cs if:!Empty ?>
PASS
<?cs else ?>
FAIL - [10]
<?cs /if ?>
[11] !Foo
<?cs if:!Foo ?>
FAIL - existing string shouldn't evaluate false [11]
<?cs else ?>
PASS
<?cs /if ?>
[12] !NotExist
<?cs if:!NotExist ?>
PASS
<?cs else ?>
FAIL - [12]
<?cs /if ?>
  
Testing !? 
[13] !?Empty
<?cs if:!?Empty ?>
FAIL - empty string shouldn't evaluate non-existing [13]
<?cs else ?>
PASS
<?cs /if ?>
[14] !?Foo
<?cs if:!?Foo ?>
FAIL - non-empty string shouldn't evaluate non-existing [14]
<?cs else ?>
PASS
<?cs /if ?>
[15] !?NotExist
<?cs if:!?NotExist ?>
PASS
<?cs else ?>
FAIL - [15]
<?cs /if ?>

Testing ?! - Existance only works on a var, otherwise always returns
true, so ?! is always true
[16] ?!Empty
<?cs if:?!Empty ?>
PASS
<?cs else ?>
FAIL - [16]
<?cs /if ?>
[17] ?!Foo
<?cs if:?!Foo ?>
PASS
<?cs else ?>
FAIL - [17]
<?cs /if ?>
[18] ?!NotExist
<?cs if:?!NotExist ?>
PASS
<?cs else ?>
FAIL - [18]
<?cs /if ?>

Testing ? and == '' - boolean vs. equality? um... boolean is a number,
so these are numeric evals, and empty string is 0
[19] ?Empty == ''
<?cs if:?Empty == ''?>
FAIL - IF boolean true equals empty [19]
<?cs else ?>
PASS - ELSE boolean true doesn't equal empty (1 != 0)
<?cs /if ?>
[20] ?Foo == ''
<?cs if:?Foo == ''?>
FAIL - IF boolean true equals empty [20]
<?cs else ?>
PASS - ELSE boolean true doesn't equal empty (1 != 0)
<?cs /if ?>
[21] ?NotExist == ''
<?cs if:?NotExist == '' ?>
PASS - IF boolean false equals empty (0 == 0)
<?cs else ?>
[22] FAIL - ELSE boolean false doesn't equal empty [22]
<?cs /if ?>

Testing ? and != '' - boolean vs. in-equality? um...
[23] ?Empty != ''
<?cs if:?Empty != ''?>
PASS - IF boolean true not equal empty (1 != 0)
<?cs else ?>
FAIL - ELSE boolean true equals empty [23]
<?cs /if ?>
[24] ?Foo != ''
<?cs if:?Foo != ''?>
PASS - IF boolean true not equal empty (1 != 0)
<?cs else ?>
FAIL - ELSE boolean true equals empty [24]
<?cs /if ?>
[25] ?NotExist != ''
<?cs if:?NotExist != '' ?>
FAIL - IF boolean false not equal empty [25]
<?cs else ?>
PASS - ELSE boolean false equals empty (0 == 0)
<?cs /if ?>

Testing !? and == '' - all boolean true equals empty
[26] !?Empty == ''
<?cs if:!?Empty == ''?>
PASS - IF boolean true equals empty
<?cs else ?>
FAIL - ELSE boolean true doesn't equal empty [26]
<?cs /if ?>
[27] !?Foo == ''
<?cs if:!?Foo == ''?>
PASS - IF boolean true equals empty
<?cs else ?>
FAIL - ELSE boolean true doesn't equal empty [27]
<?cs /if ?>
[28] !?NotExist == ''
<?cs if:!?NotExist == '' ?>
FAIL - IF boolean true equals empty [28]
<?cs else ?>
PASS - ELSE boolean true doesn't equal empty
<?cs /if ?>

Testing !? and != ''
[29] !?Empty != ''
<?cs if:!?Empty != '' ?>
FAIL - IF boolean true not equal empty [29]
<?cs else ?>
PASS - ELSE boolean true equals empty
<?cs /if ?>
[30] !?Foo != ''
<?cs if:!?Foo != '' ?>
FAIL - IF boolean true not equal empty [30]
<?cs else ?>
PASS - ELSE boolean true equals empty
<?cs /if ?>
[31] !?NotExist != ''
<?cs if:!?NotExist != '' ?>
PASS - IF boolean true not equal empty
<?cs else ?>
FAIL - ELSE boolean true equals empty [31]
<?cs /if ?>
