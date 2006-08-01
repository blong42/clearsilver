escape: not used
UrlArg: <?cs var:UrlArg ?>
BlahJs: <?cs var:BlahJs ?>
Title:  <?cs var:Title ?>

<?cs escape: "none" ?>
escape: none
UrlArg: <?cs var:UrlArg ?>
BlahJs: <?cs var:BlahJs ?>
Title:  <?cs var:Title ?>
<?cs /escape ?>

<?cs escape: "html" ?>
escape: html
UrlArg: <?cs var:UrlArg ?>
BlahJs: <?cs var:BlahJs ?>
Title:  <?cs var:Title ?>
<?cs /escape ?>

<?cs escape: "js" ?>
escape: js
UrlArg: <?cs var:UrlArg ?>
BlahJs: <?cs var:BlahJs ?>
Title:  <?cs var:Title ?>
<?cs /escape ?>

<?cs escape: "url" ?>
escape: url
UrlArg: <?cs var:UrlArg ?>
BlahJs: <?cs var:BlahJs ?>
Title:  <?cs var:Title ?>
<?cs /escape ?>

<?cs escape: "html" ?>
Nested escaping: html
The internal calls should take precedence
<?cs escape: "url"  ?>url  -> UrlArg: <?cs var:UrlArg ?><?cs /escape ?>
<?cs escape: "js"   ?>js   -> BlahJs: <?cs var:BlahJs ?><?cs /escape ?>
<?cs escape: "html" ?>html -> Title:  <?cs var:Title ?><?cs /escape ?>
<?cs /escape ?>

Defining the macro echo_all inside of a "html" escape.
<?cs escape: "html" ?><?cs def:echo_all(e) ?>
not used: <?cs var:e ?>
none:     <?cs escape: "none" ?><?cs var:e ?><?cs /escape ?>
url:      <?cs escape: "url" ?><?cs var:e ?><?cs /escape ?>
js:       <?cs escape: "js" ?><?cs  var:e ?><?cs /escape ?>
html:     <?cs escape: "html" ?><?cs var:e ?><?cs /escape ?>
<?cs /def ?><?cs /escape ?>

Calling echo_all() macro:
<?cs call:echo_all(Title + UrlArh + BlahJs) ?>

<?cs escape: "html" ?>
Calling echo_all() macro from within "html":
<?cs call:echo_all(Title + UrlArh + BlahJs) ?>
<?cs /escape ?>

<?cs escape: "js" ?>
Calling echo_all() macro from within "js":
<?cs call:echo_all(Title + UrlArh + BlahJs) ?>
<?cs /escape ?>

<?cs escape: "url" ?>
Calling echo_all() macro from within "url":
<?cs call:echo_all(Title + UrlArh + BlahJs) ?>
<?cs /escape ?>
