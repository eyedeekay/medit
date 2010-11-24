#ifndef MOO_ARRAY_H
#define MOO_ARRAY_H

#include <mooutils/mooutils-mem.h>

#define MOO_DECLARE_PTR_ARRAY(ArrayType, array_type, ElmType)           \
                                                                        \
typedef struct ArrayType ArrayType;                                     \
                                                                        \
struct ArrayType {                                                      \
    MOO_IP_ARRAY_ELMS (ElmType*, elms);                                 \
};                                                                      \
                                                                        \
ArrayType *array_type##_new (void);                                     \
void array_type##_free (ArrayType *ar);                                 \
void array_type##_append (ArrayType *ar, ElmType *elm);                 \
void array_type##_take (ArrayType *ar, ElmType *elm);                   \
void array_type##_append_array (ArrayType *ar, ArrayType *ar2);         \
ArrayType *array_type##_copy (ArrayType *ar);                           \
                                                                        \
typedef void (*ArrayType##Foreach) (ElmType *elm,                       \
                                    gpointer data);                     \
                                                                        \
inline static void                                                      \
array_type##_foreach (const ArrayType *ar,                              \
                      ArrayType##Foreach func,                          \
                      gpointer data)                                    \
{                                                                       \
    guint i;                                                            \
    g_return_if_fail (ar != NULL && func != NULL);                      \
    for (i = 0; i < ar->n_elms; ++i)                                    \
        func (ar->elms[i], data);                                       \
}                                                                       \
                                                                        \
void array_type##_sort (ArrayType *ar, GCompareFunc func);              \
int array_type##_find (const ArrayType *ar, ElmType *elm);              \
void array_type##_remove (ArrayType *ar, ElmType *elm);                 \
guint array_type##_insert_sorted (ArrayType *ar, ElmType *elm,          \
                                    GCompareFunc func);                 \
                                                                        \
inline static gboolean array_type##_is_empty (ArrayType *ar)            \
{                                                                       \
    return !ar || !ar->n_elms;                                          \
}                                                                       \
                                                                        \
inline static gsize array_type##_get_size (ArrayType *ar)               \
{                                                                       \
    return ar ? ar->n_elms : 0;                                         \
}

#define MOO_DEFINE_PTR_ARRAY(ArrayType, array_type, ElmType,            \
                             copy_elm, free_elm)                        \
                                                                        \
ArrayType *                                                             \
array_type##_new (void)                                                 \
{                                                                       \
    ArrayType *ar = g_slice_new0 (ArrayType);                           \
    MOO_IP_ARRAY_INIT (ElmType*, ar, elms, 0);                          \
    return ar;                                                          \
}                                                                       \
                                                                        \
void                                                                    \
array_type##_free (ArrayType *ar)                                       \
{                                                                       \
    if (ar)                                                             \
    {                                                                   \
        gsize i;                                                        \
        for (i = 0; i < ar->n_elms; ++i)                                \
            free_elm (ar->elms[i]);                                     \
        MOO_IP_ARRAY_DESTROY (ar, elms);                                \
        g_slice_free (ArrayType, ar);                                   \
    }                                                                   \
}                                                                       \
                                                                        \
void                                                                    \
array_type##_append (ArrayType *ar, ElmType *elm)                       \
{                                                                       \
    g_return_if_fail (ar != NULL && elm != NULL);                       \
    MOO_IP_ARRAY_GROW (ElmType*, ar, elms, 1);                          \
    ar->elms[ar->n_elms - 1] = copy_elm (elm);                          \
}                                                                       \
                                                                        \
void array_type##_append_array (ArrayType *ar, ArrayType *ar2)          \
{                                                                       \
    guint i, old_size;                                                  \
    g_return_if_fail (ar != NULL && ar2 != NULL);                       \
    if (!ar2->n_elms)                                                   \
        return;                                                         \
    old_size = ar->n_elms;                                              \
    MOO_IP_ARRAY_GROW (ElmType*, ar, elms, ar2->n_elms);                \
    ar->n_elms += ar2->n_elms;                                          \
    for (i = 0; i < ar2->n_elms; ++i)                                   \
        ar->elms[old_size + i] = copy_elm (ar2->elms[i]);               \
}                                                                       \
                                                                        \
void                                                                    \
array_type##_take (ArrayType *ar, ElmType *elm)                         \
{                                                                       \
    g_return_if_fail (ar != NULL && elm != NULL);                       \
    MOO_IP_ARRAY_GROW (ElmType*, ar, elms, 1);                          \
    ar->elms[ar->n_elms - 1] = elm;                                     \
}                                                                       \
                                                                        \
ArrayType *                                                             \
array_type##_copy (ArrayType *ar)                                       \
{                                                                       \
    ArrayType *copy;                                                    \
                                                                        \
    g_return_val_if_fail (ar != NULL, NULL);                            \
                                                                        \
    copy = array_type##_new ();                                         \
                                                                        \
    if (ar->n_elms)                                                     \
    {                                                                   \
        guint i;                                                        \
        MOO_IP_ARRAY_GROW (ElmType*, copy, elms, ar->n_elms);           \
        for (i = 0; i < ar->n_elms; ++i)                                \
            copy->elms[i] = copy_elm (ar->elms[i]);                     \
    }                                                                   \
                                                                        \
    return copy;                                                        \
}                                                                       \
                                                                        \
void array_type##_remove (ArrayType *ar, ElmType *elm)                  \
{                                                                       \
    guint i;                                                            \
                                                                        \
    g_return_if_fail (ar != NULL);                                      \
                                                                        \
    for (i = 0; i < ar->n_elms; ++i)                                    \
    {                                                                   \
        if (ar->elms[i] == elm)                                         \
        {                                                               \
            MOO_IP_ARRAY_REMOVE_INDEX (ar, elms, i);                    \
            free_elm (elm);                                             \
            return;                                                     \
        }                                                               \
    }                                                                   \
}                                                                       \
                                                                        \
static int                                                              \
array_type##_gcompare_data_func (gconstpointer a,                       \
                                 gconstpointer b,                       \
                                 gpointer      user_data)               \
{                                                                       \
    GCompareFunc func = (GCompareFunc) user_data;                       \
    ElmType **ea = (ElmType **) a;                                      \
    ElmType **eb = (ElmType **) b;                                      \
    return func (*ea, *eb);                                             \
}                                                                       \
                                                                        \
void                                                                    \
array_type##_sort (ArrayType *ar, GCompareFunc func)                    \
{                                                                       \
    g_return_if_fail (ar != NULL && func != NULL);                      \
                                                                        \
    if (ar->n_elms <= 1)                                                \
        return;                                                         \
                                                                        \
    g_qsort_with_data (ar->elms, ar->n_elms, sizeof (*ar->elms),        \
                       array_type##_gcompare_data_func,                 \
                       func);                                           \
}                                                                       \
                                                                        \
int array_type##_find (const ArrayType *ar, ElmType *elm)               \
{                                                                       \
    guint i;                                                            \
    g_return_val_if_fail (ar != NULL && elm != NULL, -1);               \
    for (i = 0; i < ar->n_elms; ++i)                                    \
        if (ar->elms[i] == elm)                                         \
            return i;                                                   \
    return -1;                                                          \
}


#define MOO_DECLARE_OBJECT_ARRAY(ArrayType, array_type, ElmType)        \
    MOO_DECLARE_PTR_ARRAY (ArrayType, array_type, ElmType)

#define MOO_DEFINE_OBJECT_ARRAY(ArrayType, array_type, ElmType)         \
    inline static ElmType *                                             \
    array_type##_ref_elm__ (ElmType *elm)                               \
    {                                                                   \
        return (ElmType*) g_object_ref (elm);                           \
    }                                                                   \
                                                                        \
    inline static void                                                  \
    array_type##_unref_elm__ (ElmType *elm)                             \
    {                                                                   \
        g_object_unref (elm);                                           \
    }                                                                   \
                                                                        \
    MOO_DEFINE_PTR_ARRAY (ArrayType, array_type, ElmType,               \
                          array_type##_ref_elm__,                       \
                          array_type##_unref_elm__)

#endif /* MOO_ARRAY_H */
