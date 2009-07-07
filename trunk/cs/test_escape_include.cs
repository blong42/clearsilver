<?cs var: Title ?>
<?cs if: Title ?><?cs var: Title ?><?cs /if ?>
lvar:<?cs lvar: csvar ?>
include:<?cs include: "test_include.cs" ?>
linclude:<?cs linclude: "test_include.cs" ?>

<?cs def:some_include() ?><?cs var: Title ?><?cs /def ?>
call:<?cs call:some_include() ?>

Inside escape command:<?cs escape: "js" ?>
lvar:<?cs lvar: csvar ?>
include:<?cs include: "test_include.cs" ?>
linclude:<?cs linclude: "test_include.cs" ?>

call:<?cs call:some_include() ?>
<?cs /escape ?>
