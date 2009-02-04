
Testing for double escaping errors when using autoescape.

<?cs var: html_escape(Title) ?>
<?cs alt: html_escape(Title) ?>NOTITLE<?cs /alt ?>

Make sure escaping isn't affected by previous calls to explicit escaping.
<?cs var: js_escape(Title) ?>
<?cs var: Title ?>

<?cs var: js_escape(Title) ?>
<?cs alt: Title ?>NOTITLE<?cs /alt ?>

<?cs if: js_escape(Title) ?>
<?cs var: Title ?>
<?cs /if ?>

<?cs if: js_escape(Title) ?>
<?cs alt: Title ?>NOTITLE<?cs /alt ?>
<?cs /if ?>
