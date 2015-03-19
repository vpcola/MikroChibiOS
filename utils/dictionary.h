#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

/**
 * Author : Cola Vergil
 * Email  : vpcola@gmail.com
 * Date : Fri Mar 06 2015
 **/
#ifdef __cplusplus
extern "C" {
#endif

#define HASHSIZE 101

typedef struct _nlist 
{ /* table entry: */
  _nlist *next; /* next entry in chain */
  char *name; /* defined name */
  char *defn; /* replacement text */
} nlist;

nlist *dict_install(const char *name, const char *defn);
nlist *dict_lookup(const char *s);

#ifdef __cplusplus
}
#endif

#endif

