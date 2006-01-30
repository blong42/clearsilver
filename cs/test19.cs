
<?cs loop:a=0,1,1 ?>
<?cs loop:b=0,1,1 ?>
a: <?cs var:a ?>  b: <?cs var:b ?>

!a || b: <?cs var:!a || b ?>

!(a || b): <?cs var:!(a || b) ?>
<?cs /loop ?>
<?cs /loop ?>

<?cs if:?Wow["Foo"] ?>
 Wow.Foo exists <?cs var:Wow.Foo ?>
<?cs /if ?>

<?cs if:?Wow["Bar"] ?>
 Wow.Bar exists <?cs var:Wow.Bar ?>
<?cs /if ?>

3: <?cs var:max((1,3),2) ?>
2: <?cs var:max((3,1),2) ?>
