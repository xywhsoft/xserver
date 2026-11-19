/* XS static sqlite-vec entry point; no loadable-extension macro redirection. */
#ifndef XS_SQLITE_VEC_H
#define XS_SQLITE_VEC_H
#include <sqlite3.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Call once per connection with pApi=NULL and an error-output pointer.
 * On failure, free the error with sqlite3_free and close the connection. */
int sqlite3_vec_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi);
#ifdef __cplusplus
}
#endif
#endif
