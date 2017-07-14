<?cs set: GoodUrl="http://www.google.com" ?>
[<?cs uvar: GoodUrl ?>]: <a href="<?cs var:GoodUrl ?>">link </a>
<?cs set: GoodUrlCaps="HTTP://WWW.GOOGLE.COM" ?>
[<?cs uvar: GoodUrlCaps ?>]: <a href="<?cs var:GoodUrlCaps ?>">link </a>
<?cs set: GoodUrlSsl="https://www.google.com" ?>
[<?cs uvar: GoodUrlSsl ?>]: <a href="<?cs var:GoodUrlSsl ?>">link </a>
<?cs set: GoodUrlSpace=" http://www.google.com" ?>
[<?cs uvar: GoodUrlSpace ?>]: <a href="<?cs var:GoodUrlSpace ?>">link </a>

[<?cs uvar: GoodUrl ?>] (unquoted URI attr): <a href=<?cs var:GoodUrl ?>>link </a>

[<?cs uvar: GoodUrlSpace ?>] (unquoted URI attr): <a href=<?cs var:GoodUrlSpace ?>>link </a>
<?cs set: RelativeUrl="logo.gif" ?>
[<?cs uvar: RelativeUrl ?>]: <a href="<?cs var:RelativeUrl ?>">link </a>
<?cs set: RelativeUrl2="Login?continue=http://www.google.com" ?>
[<?cs uvar: RelativeUrl2 ?>]: <a href="<?cs var:RelativeUrl2 ?>">link </a>
<?cs set: RelativeUrl3="images/logo.gif" ?>
[<?cs uvar: RelativeUrl3 ?>]: <a href="<?cs var:RelativeUrl3 ?>">link </a>
<?cs set: EmptyUrl="" ?>
[<?cs uvar: EmptyUrl ?>]: <a href="<?cs var:EmptyUrl ?>">link </a>
<?cs set: SpaceUrl=" " ?>
[<?cs uvar: SpaceUrl ?>]: <a href="<?cs var:SpaceUrl ?>">link </a>

<?cs set: AbsUrl="/logo.gif" ?>
[<?cs uvar: AbsUrl ?>]: <a href="<?cs var:AbsUrl ?>">link </a>
<?cs set: AbsUrl2="www.google.com" ?>
[<?cs uvar: AbsUrl2 ?>]: <a href="<?cs var:AbsUrl2 ?>">link </a>
<?cs set: AbsUrl3="//logo.gif" ?>
[<?cs uvar: AbsUrl3 ?>]: <a href="<?cs var:AbsUrl3 ?>">link </a>
<?cs set: AbsUrl4="http://user@google/abc" ?>
[<?cs uvar: AbsUrl4 ?>]: <a href="<?cs var:AbsUrl4 ?>">link </a>
<?cs set: AbsUrl5="//www.google.com/search" ?>
[<?cs uvar: AbsUrl5 ?>]: <a href="<?cs var:AbsUrl5 ?>">link </a>
<?cs set: AbsUrl6="http://user:password@google/abc" ?>
[<?cs uvar: AbsUrl6 ?>]: <a href="<?cs var:AbsUrl6 ?>">link </a>

<?cs set: UnknownScheme = "unknown://www.google.com" ?>
[<?cs uvar: UnknownScheme ?>]: <a href="<?cs var:UnknownScheme ?>">link </a>
<?cs set: BadUrl="javascript:alert(1)" ?>
[<?cs uvar: BadUrl ?>]: <a href="<?cs var:BadUrl ?>">link </a>
<?cs set: BadUrl2="data:text/html;base64,abcd" ?>
[<?cs uvar: BadUrl2 ?>]: <a href="<?cs var:BadUrl2 ?>">link </a>
<?cs set: BadUrl4="javascript:alert(1)//" ?>
[<?cs uvar: BadUrl4 ?>]: <a href="<?cs var:BadUrl4 ?>">link </a>
<?cs set: BadUrl5="jAvAscript:alert(1)" ?>
[<?cs uvar: BadUrl5 ?>]: <a href="<?cs var:BadUrl5 ?>">link </a>
<?cs set: BadUrl6 = "vbscript:MsgBox(1)" ?>
[<?cs uvar: BadUrl6 ?>]: <a href="<?cs var:BadUrl6 ?>">link </a>
<?cs set: BadUrl7="javascript	   :alert(1)" ?>
[<?cs uvar: BadUrl7 ?>]: <a href="<?cs var:BadUrl7 ?>">link </a>
<?cs set: BadUrl8="htt p://www.google.com" ?>
[<?cs uvar: BadUrl8 ?>]: <a href="<?cs var:BadUrl8 ?>">link </a>
<?cs set: BadUrlEscaped="javascript&#58;alert(1)" ?>
[<?cs uvar: BadUrlEscaped ?>]: <a href="<?cs var:BadUrlEscaped ?>">link </a>
<?cs set: BadUrlEscaped2="javascript&#0000058alert(1)" ?>
[<?cs uvar: BadUrlEscaped2 ?>]: <a href="<?cs var:BadUrlEscaped2 ?>">link </a>
<?cs set: SpaceUrlPrefix=" javascript:alert(1)" ?>
[<?cs uvar: SpaceUrlPrefix ?>]: <a href="<?cs var:SpaceUrlPrefix ?>">link </a>
<?cs set: TabUrlPrefix="   javascript:alert(1)" ?>
[<?cs uvar: TabUrlPrefix ?>]: <a href="<?cs var:TabUrlPrefix ?>">link </a>
<?cs set: NewlineUrlPrefix="
javascript:alert(1)" ?>
[<?cs uvar: NewlineUrlPrefix ?>]: <a href="<?cs var:NewlineUrlPrefix ?>">link </a>

[<?cs uvar: CtrlUrl ?>]: <a href="<?cs var:CtrlUrl ?>">link </a>

[<?cs uvar: BadUrl ?>] (unquoted URI attr): <a href=<?cs var:BadUrl ?>>link </a>
<?cs set: BadUrlSpace="jav ascript:alert(1);" ?>
[<?cs uvar: BadUrlSpace ?>] (unquoted URI attr): <a href=<?cs var:BadUrlSpace ?>>link </a>

---- Test with other common url attributes
<?cs set: BadUrl="javascript:alert(1)" ?>
[<?cs uvar: BadUrl ?>]: <img src="<?cs var:BadUrl ?>">
[<?cs uvar: BadUrl ?>]: <form method=get action="<?cs var:BadUrl ?>">link </form>
[<?cs uvar: BadUrl ?>]: <body background="<?cs var:BadUrl ?>">
[<?cs uvar: BadUrl ?>]: <object data="<?cs var:BadUrl ?>" width=100 height=100></object>




