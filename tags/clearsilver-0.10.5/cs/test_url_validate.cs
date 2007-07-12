The following urls should be allowed unmodified:
<?cs var:BlahUrl ?> = <?cs var:url_validate(BlahUrl) ?>
<?cs var:SslUrl ?> = <?cs var:url_validate(SslUrl) ?>
<?cs var:FtpUrl ?> = <?cs var:url_validate(FtpUrl) ?>
<?cs var:MailUrl ?> = <?cs var:url_validate(MailUrl) ?>

<?cs var:AbsUrl ?> = <?cs var:url_validate(AbsUrl) ?>
<?cs var:AbsUrl2 ?> = <?cs var:url_validate(AbsUrl2) ?>
<?cs var:RelUrl ?> = <?cs var:url_validate(RelUrl) ?>

<?cs var:ColonUrl ?> = <?cs var:url_validate(ColonUrl) ?>
<?cs var:ColonUrlTwo ?> = <?cs var:url_validate(ColonUrlTwo) ?>

This URL should be html escaped:
<?cs var:HtmlUrl ?> = <?cs var:url_validate(HtmlUrl) ?>


These URLs should be changed to #
<?cs var:JsUrl ?> = <?cs var:url_validate(JsUrl) ?>
<?cs var:DataUrl ?> = <?cs var:url_validate(DataUrl) ?>
<?cs var:InvalidUrl ?> = <?cs var:url_validate(InvalidUrl) ?>
<?cs var:InvalidUrl2 ?> = <?cs var:url_validate(InvalidUrl2) ?>
<?cs var:ShortUrl ?> = <?cs var:url_validate(ShortUrl) ?>
