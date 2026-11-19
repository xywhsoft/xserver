/* XS static GraphQLite entry point; uses the host's SQLite instance. */
#ifndef XS_GRAPHQLITE_H
#define XS_GRAPHQLITE_H
#include <sqlite3.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Call once on a newly opened connection, before preparing statements.
 * Creates GraphQLite's schema. On failure, free the error with sqlite3_free
 * and close the connection.
 * pApi must be NULL for the statically linked XS build. */
int sqlite3_graphqlite_init(sqlite3* db, char** pzErrMsg, const sqlite3_api_routines* pApi);
#ifdef __cplusplus
}
#endif
#endif
