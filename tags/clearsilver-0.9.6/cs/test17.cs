
Test from Chuck Simmons for setting of null value

<?cs def:set_field(card, out_field, in_field, default, color)
?><?cs   if:in_field.changed == #1
?><?cs     set:color="#ff0000"
?><?cs   /if
?><?cs   set:val = in_field
?><?cs   if:!?val || (val == "")
?><?cs     set:val = default
?><?cs   /if
?><?cs   set:card[out_field] = "<font color=" + color + ">" + val +
"</font>"
?><?cs /def ?>

<?cs call:set_field(Biz, "Address2", Biz.Address2, "", "#888888")
?><?cs call:set_field(Biz, "Address3", Biz.Address3, "", "#888888") ?>

Biz.Address2 = <?cs var:Biz.Address2 ?>
Biz.Address3 = <?cs var:Biz.Address3 ?>

