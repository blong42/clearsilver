
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
  <?cs var:$wow ?>
<?cs /def ?>

before weekday

<?cs call:Date._weekday(Wow.Foo,#5) ?>

before echo

echo a variable: 3
<?cs call:echo(Wow.Foo) ?>
echo a string: hellow world
<?cs call:echo("hello world") ?>
echo a number: 5
<?cs call:echo(#5) ?>

<?cs def:call_echo(wow) ?>
<?cs call:echo(wow) ?>
<?cs /def ?>

echo a variable: 3
<?cs call:call_echo(Wow.Foo) ?>
echo a string: hellow world
<?cs call:call_echo("hello world") ?>
echo a number: 5
<?cs call:call_echo(#5) ?>

<?cs def:echo2(bar) ?>
  <?cs var:wow ?>
<?cs /def ?>

<?cs def:call_echo2(wow, weird) ?>
  <?cs call:echo2(weird) ?>
<?cs /def ?>

these tests show that local variables are live in sub calls 
echo a variable: 3
<?cs call:call_echo2(Wow.Foo, "error") ?>
echo a string: hellow world
<?cs call:call_echo2("hello world", "error") ?>
echo a number: 5
<?cs call:call_echo2(#5, "error") ?>

after echo

<?cs def:print_day(d) ?>
  <?cs var:d ?> == <?cs var:d.Abbr ?>
<?cs /def ?>

testing macro calls in local vars in an each
<?cs each:day=Days ?>
  <?cs call:print_day(day) ?>
  <?cs call:echo(day.Abbr) ?>
<?cs /each ?>
