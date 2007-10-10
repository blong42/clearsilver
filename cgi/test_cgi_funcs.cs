Raw
<?cs var:Title ?>
<?cs var:UrlArg ?>
<?cs var:BlahJs ?>
<?cs var:Foo ?>

Text2Html
<?cs var:text_html(Title) ?>
<?cs var:text_html(UrlArg) ?>
<?cs var:text_html(BlahJs) ?>
<?cs var:text_html(Foo) ?>
<?cs var:text_html("www.fiction.net/foo http://gmail.com/part&dad
blong@google.com") ?>

UrlEscape
<?cs var:url_escape(Title) ?>
<?cs var:url_escape(UrlArg) ?>
<?cs var:url_escape(BlahJs) ?>
<?cs var:url_escape(Foo) ?>

HtmlEscape
<?cs var:html_escape(Title) ?>
<?cs var:html_escape(UrlArg) ?>
<?cs var:html_escape(BlahJs) ?>
<?cs var:html_escape(Foo) ?>
