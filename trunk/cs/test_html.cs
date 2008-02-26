<?cs def:AutoEscape(v) ?>Inside macro which *should* be auto escaped
Argument: <?cs var:v ?>
Local var: <?cs var:Title ?>
End macro<?cs /def ?>

-- Test with relevant CS commands -- 

var: <?cs var:Title ?>

uvar: <?cs uvar:Title ?>

alt: <?cs alt:Title ?>Not me<?cs /alt ?>

<?cs set: myvar="<script>alert(1);</script>" ?>
Var created using 'set' command: <?cs var: myvar ?>

lvar: <?cs lvar: csvar ?>

evar: <?cs evar: csvar ?>

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

JS attr: <input name=x onclick="alert('<?cs var:BlahJs ?>')">

Unquoted JS inside quoted attr: <input name=x onclick="alert(<?cs var:BlahJs ?>)">

Quoted JS inside unquoted attr: <input name=x onclick=alert('<?cs var:BlahJs ?>')>

Unquoted JS inside unquoted attr: <input name=x onclick=alert(<?cs var:BlahJs ?>)>

<?cs set: JsNumber=10 ?>
Valid unquoted JS attr: <input name=x onclick=alert(<?cs var:JsNumber ?>)>

URI attr: <a href="http://a.com?q=<?cs var:Title ?>">link </a>

Unquoted URI attr: <a href=http://a.com?q=<?cs var:BlahJs ?>>link </a>
<?cs set: GoodUrl="http://www.google.com" ?>
GoodUrl in URI attr: <a href="<?cs var:GoodUrl ?>">link </a>

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

-- Test passing variables to htmlparser --
<?cs set: TagName = "script" ?>
Tag name:
<<?cs var: TagName?>>var q="<?cs var:BlahJs ?>"</<?cs var: TagName?>>

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
<input <?cs var:Attr ?> onclick="alert('<?cs var: BlahJs ?>')">

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
<?cs set: StyleVar="border: 2px \x123 <>' solid #ddd;" ?>
In style attr "<?cs uvar:StyleVar ?>": <input name=x style="<?cs var:StyleVar ?>">

In unquoted style attr: <input name=x style=<?cs var:StyleVar ?>>
<?cs set: GoodStyleVar="font-size: 95%; border: 1px solid #aaa;" ?>
Valid style attr "<?cs uvar:GoodStyleVar ?>": <input name=x style="<?cs var:GoodStyleVar ?>">

Valid unquoted style attr: <input name=x style=<?cs var:GoodStyleVar ?>>
<?cs set: BadJs='" alert(1);' ?>
Inside javascript: 
<script>
var unquoted = <?cs var:BadJs ?>
var quoted = "<?cs var: BadJs ?>"
</script>

Inside style tag: 
<style>
 <?cs set: padding='3px;' ?>
 div.paddedRadioOption {
 /* Valid style body: "padding: <?cs uvar: padding ?>" */ padding: <?cs var: padding ?>
    }

 <?cs set: body="body {padding: 8px;}" ?>
 /* Valid style body: "<?cs uvar: body ?>" */ <?cs var:body ?>

 <?cs set: badbody="body {background-image: url(javascript:alert(1));}" ?>
 /* Invalid style body: "<?cs uvar: badbody ?>" */ <?cs var:badbody ?>
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

