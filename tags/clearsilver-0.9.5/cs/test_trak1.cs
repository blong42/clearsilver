<?cs if:(parent_id == #-1 && faq.topic_id == Query.topic) || (parent_id != #-1 && faq.sub_topic_id == Query.topic) ?>
  matched
<?cs /if ?>

<?cs def:test_macro(foo) ?>
  <?cs var:foo ?>
<?cs /def ?>

<?cs call:test_macro(len(faq)) ?>
