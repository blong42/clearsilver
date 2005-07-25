Test for bug where in certain cases we didn't find the ending cs tag if
there was a newline

<?cs each:agent = CGI.agents ?>
<?cs /each
?>
