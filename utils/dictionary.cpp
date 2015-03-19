#include "dictionary.h"

#include <string.h>
#include <stdlib.h>

/**
 * This code is copied from the C Book
 * "The C Programming Language" by D. Ritchie and 
 * B. Kernighan.
 *
 * I tend to avoid C++ maps here in favor for 
 * a simple C implementation.
 **/

static nlist *hashtab[HASHSIZE]; /* pointer table */

/* hash: form hash value for string s */
static unsigned hash(const char *s)
{
    unsigned hashval;
    for (hashval = 0; *s != '\0'; s++)
        hashval = *s + 31 * hashval;
    return hashval % HASHSIZE;
}

/* lookup: look for s in hashtab */
nlist *dict_lookup(const char *s)
{
    nlist *np;
    for (np = hashtab[hash(s)]; np != NULL; np = np->next)
        if (strcmp(s, np->name) == 0)
            return np; /* found */
    return NULL; /* not found */
}

/* install: put (name, defn) in hashtab */
nlist *dict_install(const char *name, const char *defn)
{
    nlist *np;
    unsigned hashval;
    if ((np = dict_lookup(name)) == NULL) { /* not found */
        np = (nlist *) malloc(sizeof(*np));
        if (np == NULL || (np->name = strdup(name)) == NULL)
            return NULL;
        hashval = hash(name);
        np->next = hashtab[hashval];
        hashtab[hashval] = np;
    } else /* already there */
        free((void *) np->defn); /*free previous defn */
    if ((np->defn = strdup(defn)) == NULL)
        return NULL;
    return np;
}


