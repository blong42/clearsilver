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

CssValidate
<?cs set:GoodUrl = "http://www.google.com" ?>
<?cs var:css_url_validate(GoodUrl) ?>
<?cs set:GoodUrl2 = "https://www.google.com/search?q=f&hl=en" ?>
<?cs var:css_url_validate(GoodUrl2) ?>
<?cs set:JsUrl = "javascript:alert(document.cookie)" ?>
<?cs var:css_url_validate(JsUrl) ?>
<?cs set:RelativeUrl = "/search?q=green flowers&hl=en" ?>
<?cs var:css_url_validate(RelativeUrl) ?>
<?cs set:HardUrl = "http://www.google.com/s?q='bla'&b=(<tag>)&c=*\\bla" ?>
<?cs var:css_url_validate(HardUrl) ?>
<?cs set:QuotesUrl = 'http://www.google.com/s?q="bla"' ?>
<?cs var:css_url_validate(QuotesUrl) ?>

