/*
 * Neotonic ClearSilver Templating System
 *
 * This code is made available under the terms of the 
 * Neotonic ClearSilver License.
 * http://www.neotonic.com/clearsilver/license.hdf
 *
 * Copyright (C) 2001 by Brandon Long
 */

#ifndef __NEO_RAND_H_
#define __NEO_RAND_H_ 1

__BEGIN_DECLS

void neo_seed_rand (long int seed);
int neo_rand (int max);
int neo_rand_string (char *s, int slen);
int neo_rand_word (char *s, int slen);

__END_DECLS

#endif /* __NEO_RAND_H_ */
