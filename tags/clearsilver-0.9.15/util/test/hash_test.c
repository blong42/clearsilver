
#include "cs_config.h"
#include <unistd.h>
#include <string.h>
#include "util/neo_misc.h"
#include "util/neo_err.h"
#include "util/neo_hash.h"

void dump_string_hash(NE_HASH *hash)
{
  NE_HASHNODE *node;
  int x;

  for (x = 0; x < hash->size; x++)
  {
    ne_warn("Node %d", x);
    for (node = hash->nodes[x]; node; node = node->next)
    {
      ne_warn("  %s = %s [%8x | %d]", node->key, node->value, node->hashv, node->hashv & (hash->size - 1));
    }
  }
}

NEOERR *dictionary_test (void)
{
  NEOERR *err = STATUS_OK;
  int x;
  char *word;
  NE_HASH *hash = NULL;
  FILE *fp;
  char buf[256];

  err = ne_hash_init(&hash, ne_hash_str_hash, ne_hash_str_comp);
  if (err)
    return nerr_pass(err);

  fp = fopen ("/usr/dict/words", "r");
  if (fp == NULL) {
    fp = fopen ("/usr/share/dict/words", "r");
    if (fp == NULL)
      return nerr_raise_errno(NERR_IO, "Unable to open file /usr/dict/words");
  }

  ne_warn("Loading words into hash");
  while (fgets (buf, sizeof(buf), fp) != NULL)
  {
    x = strlen (buf);
    if (buf[x-1] == '\n')
      buf[x-1] = '\0';

    word = strdup(buf);
    err = ne_hash_insert(hash, word, word);
    if (err) break;
    word = ne_hash_lookup(hash, buf);
    if (word == NULL)
    {
      err = nerr_raise(NERR_ASSERT, "Unable to find word %s in hash", buf);
      break;
    }
    if (strcmp(word, buf))
    {
      err = nerr_raise(NERR_ASSERT, "Lookup returned wrong word: %s != %s", buf, word);
      break;
    }
  }
  fclose (fp);
  ne_warn("Loaded %d words", hash->num);
  if (err) 
  {
    dump_string_hash(hash);
    return nerr_pass(err);
  }

  fp = fopen ("/usr/dict/words", "r");
  if (fp == NULL) {
    fp = fopen ("/usr/share/dict/words", "r");
    if (fp == NULL)
      return nerr_raise_errno(NERR_IO, "Unable to open file /usr/dict/words");
  }

  ne_warn("Testing words in hash");
  while (fgets (buf, sizeof(buf), fp) != NULL)
  {
    x = strlen (buf);
    if (buf[x-1] == '\n')
      buf[x-1] = '\0';

    if (!(word = ne_hash_lookup(hash, buf)))
    {
      err = nerr_raise(NERR_ASSERT, "Unable to find word %s in hash", buf);
      break;
    }
    if (strcmp(word, buf))
    {
      err = nerr_raise(NERR_ASSERT, "Lookup returned wrong word: %s != %s", buf, word);
      break;
    }
  }
  fclose (fp);
  ne_hash_destroy(&hash);

  return nerr_pass(err);
}

int main(int argc, char **argv)
{
  NEOERR *err;

  err = dictionary_test();
  if (err)
  {
    nerr_log_error(err);
    printf("FAIL\n");
    return -1;
  }
  printf("PASS\n");
  return 0;
}
