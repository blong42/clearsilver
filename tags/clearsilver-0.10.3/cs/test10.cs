test new alt tag

<?cs alt:#0 ?>
this should always display [1]
<?cs /alt ?>

<?cs alt:"this should display [2]" ?>
ERROR: this shouldn't display (static string)
<?cs /alt ?>

<?cs alt:#1 ?>
ERROR: this should never display (#1)
<?cs /alt ?>

<?cs var:Foo.Bar.Baz.0 ?>
<?cs alt:Foo.Bar.Baz.0 ?>
ERROR: this should never display (Foo.Bar.Baz.0 exists)
<?cs /alt ?>

<?cs alt:MyDadday ?>
This should display [3]
<?cs /alt ?>
