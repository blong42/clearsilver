<?cs def:AutoEscape(v) ?>Inside macro which *should* be auto escaped
Argument: <?cs var:v ?>
Local var: <?cs var:Title ?>
End macro<?cs /def ?>
-- Parse successfully <....>
-- Test with relevant CS commands -- 

var: <?cs var:Title ?>

uvar: <?cs uvar:Title ?>

alt: <?cs alt:Title ?>Not me<?cs /alt ?>

<?cs set: myvar="<script>alert(1);</script>" ?>
Var created using 'set' command: <?cs var: myvar ?>

lvar: <?cs lvar: csvar ?>

evar: <?cs evar: csvar ?>

<?cs # Could not set this in .hdf, get parsing error ?>
<?cs set: "Names.<evilname>" = 1 ?>
<?cs each: x = Names ?>
name: <?cs name: x ?>
<?cs /each ?>

include: 
<?cs include: "test_include.cs" ?>
<?cs set: includefile="test_include.cs" ?>
linclude: 
<?cs linclude: includefile ?>

Call macro:<?cs call:AutoEscape(Title) ?>

-- Test with explicit escaping functions, which should take precedence --

<?cs escape: "js" ?>
Inside cs escape: "js" : <?cs var:Title ?>
<?cs /escape ?>

<?cs escape: "none" ?>
Inside cs escape: "none" : <?cs var:Title ?>
<?cs /escape ?>


--- Test all possible auto escaping cases ---

HTML body: <?cs var:Title ?>

HTML attr: <input type=text name="<?cs var:BlahJs ?>" > 

Unquoted attr: <input value=<?cs var:BlahJs ?> > 

Unquoted attr with spaces: <input value=<?cs var:SpaceAttr ?>>

Unquoted attr with ctrl chars: <input value=<?cs var:CtrlAttr ?>>

<?cs set: BadName = "ab!#cs" ?>
Bad HTML tag name: <<?cs var: BadName ?> value=x >

Bad HTML attr name: <input <?cs var: BadName ?> value=x >

<?cs set: GoodName = "ab-cs" ?>
Good HTML tag name: <<?cs var: GoodName ?> value=x >

Good HTML attr name: <input <?cs var: GoodName ?> value=x >

JS attr: <input name=x onclick="alert('<?cs var:BlahJs ?>')">

Unquoted JS inside quoted attr: <input name=x onclick="alert(<?cs var:BlahJs ?>)">

Quoted JS inside unquoted attr: <input name=x onclick=alert('<?cs var:BlahJs ?>')>

Unquoted JS inside unquoted attr: <input name=x onclick=alert(<?cs var:BlahJs ?>)>

Quoted JS with spaces in unquoted attr: <input onclick=alert('<?cs var:SpaceAttr ?>')>

Quoted JS with ctrl chars in unquoted attr: <input onclick=alert('<?cs var:CtrlAttr ?>')>

<?cs set: JsNumber=10 ?>
Valid unquoted JS attr: <input name=x onclick=alert(<?cs var:JsNumber ?>)>
Valid unquoted JS in quoted attr: <input name=x onclick="alert(<?cs var:JsNumber ?>)">

<?cs set: JsNumber2="true" ?>
Valid JS boolean literal: <input name=x onclick=alert(<?cs var:JsNumber2 ?>)>

<?cs set: JsNumber3="0x45" ?>
Valid JS numeric literal: <input name=x onclick=alert(<?cs var:JsNumber3 ?>)>

<?cs set: JsNumber4="45.2345" ?>
Valid JS numeric literal: <input name=x onclick=alert(<?cs var:JsNumber4 ?>)>

<?cs set: JsNumber5="trueer" ?>
Invalid JS boolean literal: <input name=x onclick=alert(<?cs var:JsNumber5 ?>)>

<?cs set: JsNumber6="12.er" ?>
Invalid JS numeric literal: <input name=x onclick=alert(<?cs var:JsNumber6 ?>)>

URI attr: <a href="http://a.com?q=<?cs var:Title ?>">link </a>

Unquoted URI attr: <a href=http://a.com?q=<?cs var:BlahJs ?>>link </a>
<?cs set: GoodUrl="http://www.google.com" ?>
GoodUrl in URI attr: <a href="<?cs var:GoodUrl ?>">link </a>
<?cs set: GoodUrlCaps="HTTP://WWW.GOOGLE.COM" ?>
GoodUrlCaps in URI attr: <a href="<?cs var:GoodUrlCaps ?>">link </a>

GoodUrl in unquoted URI attr: <a href=<?cs var:GoodUrl ?>>link </a>
<?cs set: RelativeUrl="logo.gif" ?>
RelativeUrl in URI attr: <a href="<?cs var:RelativeUrl ?>">link </a>
<?cs set: AbsUrl="/logo.gif" ?>
AbsUrl in URI attr: <a href="<?cs var:AbsUrl ?>">link </a>
<?cs set: AbsUrl2="www.google.com" ?>
AbsUrl2 in URI attr: <a href="<?cs var:AbsUrl2 ?>">link </a>
<?cs set: BadUrl="javascript:alert(1)" ?>
BadUrl in URI attr: <a href="<?cs var:BadUrl ?>">link </a>

BadUrl in unquoted URI attr: <a href=<?cs var:BadUrl ?>>link </a>

<?cs # TODO(mugdha): This test fails currently, the parser never sees
     # the close tag.
-- Test passing variables to htmlparser --
set: TagName = "script"
Tag name:
<uvar: TagName>var q="var:BlahJs"</uvar: TagName>
?>

Unquoted attr value as uvar:
<a href=<?cs uvar: GoodUrl ?> onclick="alert('<?cs var: BlahJs ?>')">

Unquoted attr value as var:
<a href=<?cs var:BadUrl ?> onclick="alert('<?cs var: BlahJs ?>')">

Unquoted attr value as var that is not modified:
<a href=<?cs var:GoodUrl ?> onclick="alert('<?cs var: BlahJs ?>')">

Quoted attr value as uvar:
<a href="<?cs uvar: GoodUrl ?>" onclick="alert('<?cs var: BlahJs ?>')">

Quoted attr value as var:
<a href="<?cs var:BadUrl ?>" onclick="alert('<?cs var: BlahJs ?>')">

Unquoted attr value pair:
<?cs set: Attr = "name=button" ?>
<input <?cs uvar:Attr ?> onclick="alert('<?cs var: BlahJs ?>')">

attr name as var:<?cs set: AttrName = "href" ?>
<a <?cs var: AttrName ?>="<?cs var: BadUrl ?>" onclick="alert('<?cs var: BlahJs ?>')">

Unquoted attr value as lvar:
<a href=<?cs lvar:GoodUrl ?> onclick="alert('<?cs var: BlahJs ?>')">

name:
<a href=<?cs name:GoodUrl ?> onclick="alert('<?cs var: BlahJs ?>')">

Number as var:
<a name=<?cs var:Numbers.hdf9 ?> onclick="alert('<?cs var: BlahJs ?>')">

Number as lvar:
<a name=<?cs lvar:Numbers.hdf9 ?> onclick="alert('<?cs var: BlahJs ?>')">

A kludgy way to validate that variable outside tag is not parsed:
<?cs set: ScriptTag = "<script>" ?>
<?cs uvar: ScriptTag ?>
var q="<?cs var:BlahJs ?>"
<?cs set: EndScriptTag = "</script>" ?>
<?cs uvar: EndScriptTag ?>

-- Test Style --
<?cs set: StyleVar="2px \x123 <>()' solid #ddd;" ?>
In style attr "<?cs uvar:StyleVar ?>": <input name=x style="border: <?cs var:StyleVar ?>">

In unquoted style attr: <input name=x style=border:<?cs var:StyleVar ?>>
<?cs set: GoodStyleVar=" 95%" ?>
Valid style attr "<?cs uvar:GoodStyleVar ?>": <input name=x style="font-size:<?cs var:GoodStyleVar ?>">

Valid unquoted style attr: <input name=x style=font-size:<?cs var:GoodStyleVar ?>>
<?cs set: BadJs='" alert(1);' ?>
Inside javascript: 
<script>
var unquoted = <?cs var:BadJs ?>
<?cs set: ScriptNumber = 10 ?>
var unquoted_num = <?cs var:ScriptNumber ?>
<?cs set: ScriptBool = "false" ?>
var unquoted_bool = <?cs var:ScriptBool ?>
<?cs set: BadNumber = "0x45ye" ?>
var bad_number = <?cs var:BadNumber ?>
var quoted = "<?cs var: BadJs ?>"
</script>

Inside style tag: 
<style>
 <?cs set: color=' #110022' ?>
 div.paddedRadioOption {
 /* Valid style property: "<?cs uvar: color ?>" */ color: <?cs var: color ?>;
    }

 <?cs set: badbody="body {background-image: url(javascript:alert(1));}" ?>
 /* Invalid style body: "<?cs uvar: badbody ?>" */ <?cs var:badbody ?>

 /* Non ascii: "<?cs uvar: NonAscii ?>" */ font-family: <?cs var: NonAscii ?>
</style>

-- Testing noautoescape command --
<?cs noautoescape ?>
Inside noautoescape: <?cs var:Title ?>
Calling include inside noautoescape: 
<?cs include: "test_include.cs" ?>
<?cs def:Defined(v) ?>Inside macro which should *not* be auto escaped
Argument: <?cs var:v ?>
Local var: <?cs var:Title ?>
End macro<?cs /def ?>

Call macro:<?cs call:AutoEscape(Title) ?>
<?cs /noautoescape ?>

Call macro:<?cs call:Defined(Title) ?>

-- End of tests --

