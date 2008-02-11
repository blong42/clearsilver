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

<?cs escape: "html" ?>
Nested if escaping:
Should be: <?cs var:Title ?>
<?cs if: 1 ?>
If:        <?cs var:Title ?>
IfAlt:     <?cs alt:Title ?>Not me<?cs /alt ?>
<?cs /if ?>
<?cs if: 0 ?>
If:        <?cs var:Title ?>
IfAlt:     <?cs alt:Title ?>Not me<?cs /alt ?>
<?cs else ?>
Else:      <?cs var:Title ?>
<?cs /if ?>
<?cs if: 0 ?>
If:        <?cs var:Title ?>
IfAlt:     <?cs alt:Title ?>Not me<?cs /alt ?>
<?cs elif: 1 ?>
Elif:      <?cs var:Title ?>
If:        <?cs alt:Title ?>Not me<?cs /alt ?>
<?cs /if ?>
<?cs if: 1 ?>
<?cs if: 1 ?>
Nested If:        <?cs var:Title ?>
<?cs /if ?>
<?cs /if ?>
<?cs if: 1 ?>
Calling echo_all() macro inside escape, within if:
<?cs call:echo_all(Title + UrlArh + BlahJs) ?>
Defining show_title() inside escape, within if
<?cs def:show_title(t) ?>
No escaping: <?cs var:t ?>
none:     <?cs escape: "none" ?><?cs var:t ?><?cs /escape ?>
html:     <?cs escape: "html" ?><?cs var:t ?><?cs /escape ?>
<?cs /def ?>
<?cs /if ?>
<?cs /escape ?>

Calling show_title() outside escape:<?cs call:show_title(Title) ?>

Now a series of tests testing the interaction between 'escape' and various control flows
<?cs escape: "html" ?>
Title: <?cs var:Title ?>
<?cs def:some_func(t) ?>Inside def: <?cs var:t ?><?cs /def ?>
After def: <?cs var:Title ?>
<?cs if: 1 ?>
Inside if: <?cs var:Title ?>
<?cs /if ?>
After if: <?cs var:Title ?>
<?cs call:some_func(Title) ?>
After call: <?cs var:Title ?>
<?cs loop:x = #1, #5, #2 ?>
Inside loop: <?cs var:Title ?>
<?cs /loop ?>
After loop: <?cs var:Title ?>
<?cs each:sub = Numbers ?>
Inside each: <?cs var:Title ?>
<?cs /each ?>
After each: <?cs var:Title ?>
<?cs with:sub = Title ?>
Inside with: <?cs var: sub ?>
<?cs /with ?>
After with: <?cs var:Title ?>
<?cs /escape ?>

Now test escape with nested defs
<?cs escape: "html" ?>
<?cs def:outer_def(t) ?>
Inside def: <?cs var:t ?>
<?cs def:inner_def(y) ?>Inside inner def: <?cs var: y ?><?cs /def ?>
<?cs call:inner_def(t) ?>
<?cs /def ?>
Calling defs from inside escape:<?cs call:outer_def(Title) ?>
<?cs /escape ?>
Calling defs from outside escape:<?cs call:outer_def(Title) ?>

Now test escape inside def
<?cs def:escaped_func(t) ?>
Before escape: <?cs var: t ?>
<?cs escape: "html" ?>
Inside escape: <?cs var: t ?>
<?cs /escape ?>
After escape: <?cs var: t ?>
<?cs /def ?>
<?cs call:escaped_func(Title) ?>

