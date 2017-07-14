
<?cs set: BadJs=' > < / " \ & ' + "'" ?>
[<?cs uvar: BadJs ?>]: <input name=x onclick="doA('<?cs var:BadJs ?>')">

<?cs set: HtmlEscapedJs = "&quot;&apos;&#39;" ?>
[<?cs uvar: HtmlEscapedJs ?>]: <input name=x onclick="doA('<?cs var:HtmlEscapedJs ?>')">

[<?cs uvar: BadJs ?>] (Unquoted JS inside quoted attr): <input name=x onclick="doA(<?cs var:BadJs ?>)">

<?cs set: UnquotedJsAttr = BadJs + '=' ?>
[<?cs uvar: UnquotedJsAttr ?>] (Quoted JS inside unquoted attr): <input name=x onclick=doA('<?cs var: UnquotedJsAttr ?>')>

[<?cs uvar: BadJs ?>] (Unquoted JS inside unquoted attr): <input name=x onclick=doA(<?cs var:BadJs ?>)>

[<?cs uvar: SpaceAttr ?>] (Quoted JS with spaces in unquoted attr): <input onclick=doA('<?cs var:SpaceAttr ?>')>

[<?cs uvar: CtrlAttr ?>] (Quoted JS with ctrl chars in unquoted attr): <input onclick=doA('<?cs var:CtrlAttr ?>')>

<?cs set: JsNumber=10 ?>
[<?cs uvar: JsNumber ?>] (unquoted js in unquoted attr): <input name=x onclick=doA(<?cs var:JsNumber ?>)>
[<?cs uvar: JsNumber ?>] (unquoted js in quoted attr): <input name=x onclick="doA(<?cs var:JsNumber ?>)">

<?cs set: JsNumber2="true" ?>
[<?cs uvar: JsNumber2 ?>] (unquoted js in unquoted attr): <input name=x onclick=doA(<?cs var:JsNumber2 ?>)>

<?cs set: JsNumber3="0x45" ?>
[<?cs uvar: JsNumber3 ?>] (unquoted js in unquoted attr): <input name=x onclick=doA(<?cs var:JsNumber3 ?>)>

<?cs set: JsNumber4="45.2345" ?>
[<?cs uvar: JsNumber4 ?>] (unquoted js in unquoted attr): <input name=x onclick=doA(<?cs var:JsNumber4 ?>)>

<?cs set: JsNumber5="trueer" ?>
[<?cs uvar: JsNumber5 ?>] (unquoted js in unquoted attr): <input name=x onclick=doA(<?cs var:JsNumber5 ?>)>

<?cs set: JsNumber6="12.er" ?>
[<?cs uvar: JsNumber6 ?>] (unquoted js in unquoted attr): <input name=x onclick=doA(<?cs var:JsNumber6 ?>)>

<?cs set: JsNumber="10hello"?>
[<?cs uvar: JsNumber ?>] (unquoted js in unquoted attr): <input name=x onclick=doA(<?cs var:JsNumber ?>)>

<?cs set: JsNumber="10	 hello"?>
[<?cs uvar: JsNumber ?>] (unquoted js in unquoted attr): <input name=x onclick=doA(<?cs var:JsNumber ?>)>

<?cs set: JsNumber="tr ue"?>
[<?cs uvar: JsNumber ?>] (unquoted js in unquoted attr): <input name=x onclick=doA(<?cs var:JsNumber ?>)>

<?cs set: JsNumber="TRUE"?>
[<?cs uvar: JsNumber ?>] (unquoted js in unquoted attr): <input name=x onclick=doA(<?cs var:JsNumber ?>)>

------ Other common attributes

<?cs set: BadJs='" alert(1);' ?>
[<?cs uvar: BadJs ?>]: <input name=x onmouseover="doA('<?cs var:BadJs ?>')">
[<?cs uvar: BadJs ?>]: <img src=x onerror="doA('<?cs var:BadJs ?>')">
[<?cs uvar: BadJs ?>]: <a href=x onblur="doA('<?cs var:BadJs ?>')" onfocus='doX("<?cs var: BadJs ?>")'>xyz</a>
[<?cs uvar: BadJs ?>]: <body ONLOAD="doA('<?cs var:BadJs ?>')">xxx</body>

------ Script tag

Inside javascript:
<script>
// <?cs uvar: BadJs ?>
var unquoted = <?cs var:BadJs ?>
<?cs set: ScriptNumber = 10 ?>
// <?cs uvar: ScriptNumber ?>
var unquoted_num = <?cs var:ScriptNumber ?>
<?cs set: ScriptBool = "false" ?>
// <?cs uvar: ScriptBool ?>
var unquoted_bool = <?cs var:ScriptBool ?>
<?cs set: BadNumber = "0x45ye" ?>
// <?cs uvar: BadNumber ?>
var bad_number = <?cs var:BadNumber ?>

// <?cs uvar: BadJs ?>
var quoted = "<?cs var: BadJs ?>"
<?cs set: BadJs="'; alert(1);" ?>
// <?cs uvar: BadJs ?>
var singlequoted = '<?cs var: BadJs ?>';
<?cs set: BadJs=";
alert(1);" ?>
/* <?cs uvar: BadJs ?> */
var injectNewline = "<?cs var: BadJs ?>";
</script>

<SCRIPT>
<?cs set: BadJs='" alert(1);' ?>
// <?cs uvar: BadJs ?>
var x = "<?cs var: BadJs ?>";
</SCRIPT>
