
<?cs def:Date._weekday(day,four) ?>
<?cs each:wday = Days ?>
  <?cs if:wday == day ?>
    <?cs var:wday.Abbr ?>
  <?cs /if ?>
<?cs /each ?>
<?cs if:day == "6" ?>
<?cs var:Days.0.Abbr ?>
<?cs elseif:day == "0" ?>
<?cs var:Days.1.Abbr ?>
<?cs elseif:day == "1" ?>
<?cs var:Days.2.Abbr ?>
<?cs elseif:day == "2" ?>
<?cs var:Days.3.Abbr ?>
<?cs elseif:day == "3" ?>
<?cs var:Days.4.Abbr ?>
<?cs elseif:day == "4" ?>
<?cs var:Days.5.Abbr ?>
<?cs elseif:day == "5" ?>
<?cs var:Days.6.Abbr ?>
<?cs /if ?>
<?cs /def ?>


<?cs def:echo(wow) ?>
  <?cs var:wow ?>
<?cs /def ?>

before weekday

<?cs call:Date._weekday(Wow.Foo,#5) ?>

before echo

<?cs call:echo(Woo.Foo) ?>

after echo
