#ifndef THREADEDIMAGEPROCESSOR_MACROS_H
#define THREADEDIMAGEPROCESSOR_MACROS_H

// Calls malloc, goto label on error
#define MALLOC(var, nbytes, label)      \
  do {                                        \
    (var) = malloc((size_t)(nbytes));         \
    if ((var) == nullptr) {                      \
      perror("Error allocating memory with malloc.");                            \
      goto label;                             \
    }                                         \
  } while (0)

// Calls calloc, goto label on error
#define CALLOC(var, count, size, label) \
  do {                                        \
    (var) = calloc((size_t)(count), (size_t)(size)); \
    if ((var) == nullptr) {                      \
      perror(msg);                            \
      goto label;                             \
    }                                         \
  } while (0)

// Calls realloc, goto label on error
#define REALLOC(var, nbytes, label)     \
  do {                                        \
    void *_tmp = realloc((var), (size_t)(nbytes)); \
    if (_tmp == nullptr) {                       \
      perror(msg);                            \
      goto label;                             \
    } else {                                  \
      (var) = _tmp;                           \
    }                                         \
  } while (0)

// Safe free: checks for NULL, frees, then sets pointer to NULL.
// Usage: SAFE_FREE(ptr);  // ptr must be an lvalue pointer expression
#define FREE(ptr)                 \
  do {                                 \
    if ((ptr) != nullptr) {               \
      free((ptr));                     \
      (ptr) = nullptr;                    \
    }                                  \
  } while (0)

/*
 * MALLOC_2D(out_rows, rows, cols, type, fail_label)
 *
 * On success:
 *   - (out_rows) becomes a 'type **' with 'rows' rows and 'cols' columns.
 *   - Each row i is individually malloc'd: out_rows[i] points to 'type[cols]'.
 *   - Caller frees with:
 *       for (size_t i = 0; i < rows; ++i) free(out_rows[i]);
 *       free(out_rows);
 *
 * On failure:
 *   - Prints perror(...) and frees any rows already allocated, then jumps.
 *
 * Requirements:
 *   - 'fail_label' must exist in the caller.
 *   - 'rows' and 'cols' must be side-effect-free (they’re read once).
 */
#define MALLOC_2D(out_rows, rows, cols, type, fail_label)                                   \
    do {                                                                                    \
        size_t _m2d_R = (size_t)(rows);                                                     \
        size_t _m2d_C = (size_t)(cols);                                                     \
        (out_rows) = NULL;                                                                  \
        if (_m2d_R == 0 || _m2d_C == 0) {                                                   \
            errno = EINVAL;                                                                 \
            perror("MALLOC_2D: zero dimension");                                            \
            goto fail_label;                                                                \
        }                                                                                   \
        type **_m2d_rows = (type**)calloc(_m2d_R, sizeof *_m2d_rows);                       \
        if (!_m2d_rows) {                                                                   \
            perror("MALLOC_2D: row-table calloc");                                          \
            goto fail_label;                                                                \
        }                                                                                   \
        size_t _m2d_i = 0;                                                                  \
        for (; _m2d_i < _m2d_R; ++_m2d_i) {                                                 \
            _m2d_rows[_m2d_i] = (type*)malloc(_m2d_C * sizeof **_m2d_rows);                 \
            if (!_m2d_rows[_m2d_i]) {                                                       \
                perror("MALLOC_2D: row malloc");                                            \
                while (_m2d_i-- > 0) free(_m2d_rows[_m2d_i]);                               \
                free(_m2d_rows);                                                            \
                goto fail_label;                                                            \
            }                                                                               \
        }                                                                                   \
        (out_rows) = _m2d_rows;                                                             \
    } while (0)


#endif //THREADEDIMAGEPROCESSOR_MACROS_H
