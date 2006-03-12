
<?cs var:-1 ?>

<?cs if:-1 == -1 ?>
  CORRECT: -1 == -1
<?cs else ?>
  ERROR: -1 should equal -1
<?cs /if ?>

<?cs if:-1 ?>
  CORRECT: -1 is boolean true
<?cs else ?>
  ERROR : -1 should be boolean true
<?cs /if ?>

<?cs if:#-1 ?>
  CORRECT: #-1 is boolean true
<?cs else ?>
  ERROR : #-1 should be boolean true
<?cs /if ?>

<?cs if:0 ?>
  ERROR: 0 should be boolean false
<?cs else ?>
  CORRECT: 0 is boolean false
<?cs /if ?>

<?cs if:00 ?>
  ERROR: 00 should be boolean false
<?cs else ?>
  CORRECT: 00 is boolean false
<?cs /if ?>

<?cs if:0x15 == 21 ?>
  CORRECT: 0x15 (hex) == 21
<?cs else ?>
  ERROR: 0x15 (hex) should equal 21
<?cs /if ?>

<?cs if:0x15 ?>
  CORRECT: 0x15 is boolean true
<?cs else ?>
  ERROR: 0x15 should be boolean true
<?cs /if ?>

<?cs if:(3*2)+4 == 10 ?>
  CORRECT: (3*2)+4 does equal 10
<?cs else ?>
  ERROR: (3*2)+4 should equal 10
<?cs /if ?>

<?cs if:Days[0] + 2 == 2 ?>
  CORRECT: 0 + 2 == 2
<?cs else ?>
  ERROR: 0 + 2 should equal 2
<?cs /if ?>
