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

<?cs def:test_explicit_escape() ?>
<script>
var x = "<?cs var: Title ?>"
var htmlX = <?cs escape:"html" ?>"<?cs var:Title ?>"<?cs /escape ?>
var noneX = <?cs escape:"none" ?>"<?cs var:Title ?>"<?cs /escape ?>
var xagain = "<?cs var: Title ?>"
</script>
<?cs /def ?>
<?cs call:test_explicit_escape() ?>

<script>
include inside escape html:
var htmlX = <?cs escape: "html" ?><?cs include: "test_include.cs" ?><?cs /escape ?>
var X = "<?cs var: Title ?>"
include inside escape none:
var noneX = <?cs escape: "none" ?><?cs include: "test_include.cs" ?><?cs /escape ?>
var xagain = "<?cs var: Title ?>"
</script>

--- Test all possible auto escaping cases ---

HTML body: <?cs var:Title ?>

<?cs set: HtmlAttr = ' < > " & ' + "'" ?>
HTML attr: <input type=text name="<?cs var:HtmlAttr ?>" >

<?cs set: UnquotedAttr = HtmlAttr + '=' ?>
Unquoted attr: <input value=<?cs var:UnquotedAttr ?> >

Unquoted attr with spaces: <input value=<?cs var:SpaceAttr ?>>

Unquoted attr with ctrl chars: <input value=<?cs var:CtrlAttr ?>>

<?cs set: BadName = "ab!#cs" ?>
Bad HTML tag name: <<?cs var: BadName ?> value=x >

Bad HTML attr name: <input <?cs var: BadName ?> value=x >

<?cs set: GoodName = "ab-cs" ?>
Good HTML tag name: <<?cs var: GoodName ?> value=x >

Good HTML attr name: <input <?cs var: GoodName ?> value=x >

URI attr: <a href="http://a.com?q=<?cs var:Title ?>">link </a>

Unquoted URI attr: <a href=http://a.com?q=<?cs var:BlahJs ?>>link </a>

<?cs # TODO(mugdha): This test fails currently, the parser never sees
     # the close tag.
-- Test passing variables to htmlparser --
set: TagName = "script"
Tag name:
<uvar: TagName>var q="var:BlahJs"</uvar: TagName>
?>

<?cs set: GoodUrl="http://www.google.com" ?>
Unquoted attr value as uvar:
<a href=<?cs uvar: GoodUrl ?> onclick="alert('<?cs var: BlahJs ?>')">

<?cs set: BadUrl="javascript:alert(1);" ?>
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

-- End of tests --

