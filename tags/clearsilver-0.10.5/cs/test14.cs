
Existence Tests

<?cs if:?#0 ?>
  All numbers exist
<?cs else ?>
  ERROR
<?cs /if ?>

<?cs if:?"Wow" ?>
  All strings exist
<?cs else ?>
  ERROR
<?cs /if ?>

<?cs if:?("wow" + #5) ?>
  All expressions exist
<?cs else ?>
  ERROR
<?cs /if ?>

<?cs if:?Blah ?>
  <?cs name:Blah ?> Exists
<?cs else ?>
  ERROR
<?cs /if ?>

<?cs if:?Wow.Foo ?>
  <?cs name:Wow.Foo ?> Exists
<?cs else ?>
  ERROR
<?cs /if ?>

<?cs if:?Blooey ?>
  ERROR
<?cs else ?>
  Blooey doesn't exist
<?cs /if ?>

<?cs if:!?Blooey ?>
  Blooey doesn't exist
<?cs else ?>
  ERROR
<?cs /if ?>

<?cs if:Blooey ?>
  ERROR
<?cs else ?>
  Blooey doesn't exist (implied)
<?cs /if ?>

<?cs if:!Blooey ?>
  Blooey doesn't exist (implied negative)
<?cs else ?>
  ERROR
<?cs /if ?>

<?cs if:Blooey || TestIf ?>
  ERROR
<?cs else ?>
  CORRECT boolean test, blooey doesn't exist, testif == 0 so its false
<?cs /if ?>

<?cs if:?Blooey || ?TestIf ?>
  explicit existence test
<?cs else ?>
  ERROR
<?cs /if ?>

testing not op

<?cs if:!#0 ?>
  Testing not zero
<?cs else ?>
  ERROR
<?cs /if ?>

<?cs if:!#1 ?>
  ERROR
<?cs else ?>
  Testing not one
<?cs /if ?>

<?cs if:!$1 ?>
  Testing not exist var one
<?cs else ?>
  ERROR
<?cs /if ?>

<?cs if:!(?Blooey || ?TestIf) ?>
 ERROR
<?cs else ?>
  not expression existence test
<?cs /if ?>

<?cs if:!(#0 || Blooey) ?>
  not expression test
<?cs else ?>
 ERROR
<?cs /if ?>

<?cs if:!(#0 || arg1) ?>
 ERROR
<?cs else ?>
  not expression test
<?cs /if ?>

 array exists test 
<?cs var:Days[TestIf] ?>
<?cs if:?Days[TestIf] ?>
 PASS
<?cs else ?>
 ERROR
<?cs /if ?>

array element exists test
<?cs var:Days[TestIf].Abbr ?>
<?cs if:?Days[TestIf].Abbr ?>
 PASS
<?cs else ?>
 ERROR
<?cs /if ?>

array element exists test false
<?cs var:Days[TestIf].foo ?>
<?cs if:?Days[TestIf].foo ?>
 ERROR
<?cs else ?>
 PASS
<?cs /if ?>
