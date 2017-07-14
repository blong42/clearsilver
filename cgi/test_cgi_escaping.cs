<?cs set: "A.<EvilName>" = "something" ?>
<?cs with: evilname = A["<EvilName>"] ?>

Testing for double escaping errors.
<?cs escape: "html" ?><?cs var: Title ?><?cs /escape ?>
<?cs escape: "html" ?><?cs var: html_escape(Title) ?><?cs /escape ?>
<?cs escape: "js" ?><?cs var: html_escape(Title) ?><?cs /escape ?>

<?cs escape: "html" ?>
<?cs alt: Title ?>NOTITLE<?cs /alt ?>
<?cs /escape ?>
<?cs escape: "html" ?>
<?cs alt: html_escape(Title) ?>NOTITLE<?cs /alt ?>
<?cs /escape ?>
<?cs escape: "js" ?>
<?cs alt: html_escape(Title) ?>NOTITLE<?cs /alt ?>
<?cs /escape ?>

<?cs escape: "html" ?><?cs name: evilname ?><?cs /escape ?>

Make sure escaping isn't affected by previous calls to explicit escaping.
<?cs escape: "html" ?>
<?cs var: js_escape(Title) ?>
<?cs var: Title ?>
<?cs /escape ?>

<?cs escape: "html" ?>
<?cs var: js_escape(Title) ?>
<?cs alt: Title ?>NOTITLE<?cs /alt ?>
<?cs /escape ?>

<?cs escape: "html" ?>
<?cs var: js_escape(Title) ?>
<?cs name: evilname ?>
<?cs /escape ?>

<?cs escape: "html" ?>
<?cs if: js_escape(Title) ?>
<?cs var: Title ?>
<?cs /if ?>
<?cs /escape ?>

<?cs escape: "html" ?>
<?cs if: js_escape(Title) ?>
<?cs alt: Title ?>NOTITLE<?cs /alt ?>
<?cs /if ?>
<?cs /escape ?>

<?cs escape: "html" ?>
<?cs if: js_escape(Title) ?>
<?cs name: evilname ?>
<?cs /if ?>
<?cs /escape ?>

<?cs var: url_escape(Foo) ?>
<?cs var: url_escape_rfc2396(Foo) ?>

<?cs /with ?>
