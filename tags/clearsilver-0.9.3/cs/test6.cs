
Testing existence
<?cs if:Color ?>
  Color=Color (CORRECT)
<?cs else ?>
  ERROR
<?cs /if ?>

Testing not existence
<?cs if:!NoVar ?>
  Correct, doesn't exist
<?cs else ?>
  ERROR, doesn't exist
<?cs /if ?>

<?cs if:NoVar ?>
  ERROR, doesn't exist
<?cs else ?>
  Correct, doesn't exist
<?cs /if ?>
