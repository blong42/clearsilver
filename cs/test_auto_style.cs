-- Test Style --
<?cs set: StyleVar="2px \x123 <>()' solid #ddd;" ?>
In style attr "<?cs uvar:StyleVar ?>": <input name=x style="border: <?cs var:StyleVar ?>">

In unquoted style attr: <input name=x style=border:<?cs var:StyleVar ?>>
<?cs set: GoodStyleVar=" 95%" ?>
Valid style attr "<?cs uvar:GoodStyleVar ?>": <input name=x style="font-size:<?cs var:GoodStyleVar ?>">

Valid unquoted style attr: <input name=x style=font-size:<?cs var:GoodStyleVar ?>>
<?cs set: StyleVar="color:expression(alert(1))" ?>
<?cs var: StyleVar ?><A STYLE="<?cs var: StyleVar ?>">X</A>
<?cs set: StyleVar="color: expression\028\061lert\028\031\029\029" ?>
<?cs var: StyleVar ?><a style="<?cs var: StyleVar ?>">X</a>
<?cs set: StyleVar="red; font: arial" ?>
<?cs var: StyleVar ?><a style='color: <?cs var: StyleVar ?>'>X</a>

Inside style tag:
<style>
 <?cs set: color=' #110022' ?>
 div.paddedRadioOption {
 /* Valid style property: "<?cs uvar: color ?>" */ color: <?cs var: color ?>;
    }

 <?cs set: badbody="body {background-image: url(javascript:alert(1));}" ?>
 /* Invalid style body: "<?cs uvar: badbody ?>" */ <?cs var:badbody ?>

 <?cs set: import = '@import "javascript:alert(1)"' ?>
 /* Import statement with URL: [<?cs uvar: import ?>] */ <?cs var: import ?>

 /* Non ascii: "<?cs uvar: NonAscii ?>" */ font-family: <?cs var: NonAscii ?>
</style>

Inside uppercase style tag
<?cs set: StyleVar = "url('javascript:alert(1)')" ?>
<STYLE>
body {
  /* <?cs uvar: StyleVar ?> */
  behavior : <?cs var: StyleVar ?>
}
</STYLE>

