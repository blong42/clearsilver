<?cs def:link_appinfo(link, file) ?>
  <?cs set:link = link + '?' ?>
  Link: <?cs var:link ?>, file.Name <?cs var:file.Name ?>
  <?cs if:file.Name ?>
    <?cs set:link = link + "&name=" + file.Name ?>
    <?cs set:link2 = link + "&name=" + file.Name ?>
  <?cs /if ?>
  <br>
  link: <?cs var:link ?>
  link2:<?cs var:link2 ?>
<?cs /def ?>
<?cs call:link_appinfo("/dashboard", Files.0) ?>
