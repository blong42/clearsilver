

<?cs def:display_files(files) ?>
   <ul>
   <?cs each:file = files ?>
     <li><?cs var:file.Name ?></li>
     <?cs if:file.Sub.0.Name ?>
       <?cs call:display_files(file.Sub) ?>
     <?cs /if ?>
   <?cs /each ?>
   </ul>
<?cs /def ?>

<?cs call:display_files(Files) ?>

<?cs def:display_files2(files, spc) ?>
   <?cs # This tests whether we can set a local var ?>
   <?cs # Also, whether we can set a local var to itself.. ?>
   <?cs set:spc = spc ?>
   <?cs each:file = files ?>
     <?cs var:spc ?><?cs var:file.Name ?><br>
     <?cs if:file.Sub.0.Name ?>
       <?cs call:display_files2(file.Sub, spc + "&nbsp;") ?>
     <?cs /if ?>
   <?cs /each ?>
<?cs /def ?>

<?cs set:blank = "" ?>
<?cs call:display_files2(Files, blank) ?>

