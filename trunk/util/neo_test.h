/* 
 * Copyright (C) 2000  Neotonic
 *
 * All Rights Reserved.
 */

#ifndef __NEO_TEST_H_
#define __NEO_TEST_H_ 1

__BEGIN_DECLS

void neot_seed_rand (long int seed);
int neot_rand (int max);
int neot_rand_string (char *s, int slen);
int neot_rand_word (char *s, int slen);

__END_DECLS

#endif /* __NEO_TEST_H_ */
