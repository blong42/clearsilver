<?cs def:getWidthStyle(width, output) ?>
  <?cs set:output = "style='width:" + width + "px;'" ?>
<?cs /def ?>

Testing "pass by reference" to macro calls so they can "return" data

Testing non-existant var
<?cs call:getWidthStyle(100, attr) ?>
<?cs var:attr ?>

Testing non-existant var sub var
<?cs call:getWidthStyle(300, attr2.foo) ?>
<?cs var:attr2.foo ?>

Testing non-existant sub var
<?cs call:getWidthStyle(400, attr.foo) ?>
<?cs var:attr.foo ?>

Testing existant var
<?cs set:attr3 = "" ?>
<?cs call:getWidthStyle(200, attr3) ?>
<?cs var:attr3 ?>
