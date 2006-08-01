


<html>
<head><title><?cs var:Title ?></title>
<style>
@import(http://www.clearsilver.net?<?cs var:UrlArg ?>)
</style>
<script>
var default_escape  = "<?cs var:BlahJs ?>";
var explicit_escape = "<?cs var:js_escape(BlahJs) ?>";
document.writeln("<style>");
var style_in_script = "<?cs var:BlahJs?>";
document.writeln("</style>");
</script>
<img alt="default escape"  src="http://www.clearsilver.net?<?cs var:UrlArg ?>"/>
<img alt="explicit escape" src="http://www.clearsilver.net?<?cs var:url_escape(UrlArg) ?>"/>
implicit:         <?cs var:Title ?>
explicit escape: <?cs var:html_escape(Title) ?>
uvar:          <?cs uvar:Title ?>
implicit_slice:             <?cs var:string.slice(Title, 7, 100) ?>
slice_explicit_escape:             <?cs var:string.slice(html_escape(Title), 7, 100) ?>
explicit_escape_slice:             <?cs var:html_escape(string.slice(Title, 7, 100)) ?>
</head>
</html>
non-html output: <?cs var:Title ?>
<script>Script outside HTML: <?cs var:BlahJs ?></script>
<script><html>Script outside HTML with HTML inside: <?cs var:BlahJs ?></html></script>

<script>Script outside HTML: <?cs var:BlahJs ?></script>


<?cs def:get_var(var) ?>
   get_var:  <?cs var:var ?>
 <?cs escape: "url" ?>
   get_var_inside_url_escape:                      <?cs var:var ?>
   get_var_inside_url_escape_with_explicit_escape: <?cs var:url_escape(var) ?>
 <?cs /escape ?>
 <?cs escape: "none" ?>
   get_var_inside_none_escape: <?cs var:var ?>
   get_var_inside_none_escape_with_explicit_url_escape: <?cs var:url_escape(var) ?>
 <?cs /escape ?>
<?cs /def ?>

<?cs if #1 == #1 ?>
  <?cs if #2 == #2 ?>
    nested ifs
  <?cs /if ?>
<?cs /if ?>

<?cs escape: "html" ?>
Calling get_var(UrlArg) from within 'escape: "html"' <br/>
--&gt; <?cs call:get_var(UrlArg) ?>
<?cs /escape ?>

Including test_escape.cs: <br/>
---
<?cs escape: "html" ?>
<?cs include! "test_escape.cs" ?>
<?cs /escape ?>
----

<?cs escape: "html" ?>
  escape level 1
  <?cs escape: "js" ?>
  escape level 2
  <?cs /escape ?>
<?cs /escape ?>
