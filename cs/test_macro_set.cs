<?cs def:getWidthStyle(width, output) ?>
  <?cs set:output = "style='width:" + width + "px;'" ?>
  In macro: <?cs var:output ?>
<?cs /def ?>

Testing "pass by reference" to macro calls so they can "return" data

Testing non-existent var
<?cs call:getWidthStyle(100, attr) ?>
<?cs var:attr ?>

Testing non-existent var sub var
<?cs call:getWidthStyle(300, attr2.foo) ?>
<?cs var:attr2.foo ?>

Testing non-existent sub var
<?cs call:getWidthStyle(400, attr.foo) ?>
<?cs var:attr.foo ?>

Testing existent var
<?cs set:attr3 = "" ?>
<?cs call:getWidthStyle(200, attr3) ?>
<?cs var:attr3 ?>
