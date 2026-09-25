#include "map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// // Help function for bsearch
// int data_cmp(void const *lhs, void const *rhs)
// {
//     const key_t *l = lhs;
//     const key_t *r = rhs;

//     if(*l < *r) return -1;
//     else if (*l > *r) return 1;
//     else return 0;
// }

#define MAP_SIZE 32

void map_init(struct map* m)
{
    m->content = malloc(MAP_SIZE * sizeof(value_t));
    m->keys = malloc(MAP_SIZE * sizeof(key_t));
    m->size = 0;
}

key_t map_insert(struct map* m, value_t v)
{ 
    if (m->size >= MAP_SIZE) {
        return -1;  // Map full
    }

    m->keys[m->size] = m->size + 1;
    m->content[m->size] = v;
    m->size++;

    return m->size;
}


value_t map_find(struct map* m, key_t k)
{
    for (int i = 0; i < m->size; i++) {
        if (m->keys[i] == k) {
            return m->content[i];
        }
    }
    return NULL;
}


value_t map_remove(struct map* m, key_t k)
{
    for(int i = 0; i < m->size; i++)
    {
        if(m->keys[i] == k)
        {
            value_t removed_value = m->content[i];


            for(int j = i; j < m->size - 1; j++)
            {
                m->content[j] = m->content[j + 1];
                m->keys[j] = m->keys[j + 1];
            }

            m->size--;
            return removed_value;
        }
    }

    return NULL;
}


void map_for_each(struct map* m, void (*exec)(key_t k, value_t v, int aux), int aux)
{
    for(int i = 0; i < m->size; i++)
    {
        exec(m->keys[i], m->content[i], aux);
    }
}

void map_remove_if(struct map* m,bool (*cond)(key_t k, value_t v, int aux),int aux)
{
    int i = 0;
    while (i < m->size) {
        if (cond(m->keys[i], m->content[i], aux)) {
            map_remove(m, m->keys[i]);
        } else {
            i++;
        }
    }
    free(m->keys);
    free(m->content);
}
