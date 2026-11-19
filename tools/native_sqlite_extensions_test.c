#include <sqlite3.h>
#include "src/sdk/graphqlite.h"
#include "src/sdk/sqlite-vec.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void sql(sqlite3* db, const char* text)
{
    char* error = NULL;
    int rc = sqlite3_exec(db, text, NULL, NULL, &error);
    if (rc != SQLITE_OK) fprintf(stderr, "SQL failed: %s: %s\n", text, error);
    sqlite3_free(error);
    assert(rc == SQLITE_OK);
}

static int integer(sqlite3* db, const char* text)
{
    sqlite3_stmt* stmt = NULL;
    assert(sqlite3_prepare_v2(db, text, -1, &stmt, NULL) == SQLITE_OK);
    assert(sqlite3_step(stmt) == SQLITE_ROW);
    int result = sqlite3_column_int(stmt, 0);
    assert(sqlite3_finalize(stmt) == SQLITE_OK);
    return result;
}

static void dummy(sqlite3_context* context, int argc, sqlite3_value** argv)
{
    (void)argc; (void)argv;
    sqlite3_result_int(context, 1);
}

int main(int argc, char** argv)
{
    sqlite3* db = NULL;
    char* error = NULL;
    assert(sqlite3_open(":memory:", &db) == SQLITE_OK);
    if (argc > 1 && strcmp(argv[1], "busy") == 0) {
        sqlite3_stmt* stmt = NULL;
        assert(sqlite3_create_function(db, "cypher", -1, SQLITE_UTF8, NULL, dummy, NULL, NULL) == SQLITE_OK);
        assert(sqlite3_prepare_v2(db, "SELECT cypher()", -1, &stmt, NULL) == SQLITE_OK);
        assert(sqlite3_step(stmt) == SQLITE_ROW);
        assert(sqlite3_graphqlite_init(db, &error, NULL) != SQLITE_OK);
        sqlite3_free(error);
        assert(sqlite3_finalize(stmt) == SQLITE_OK);
        assert(sqlite3_close(db) == SQLITE_OK);
        puts("PASS GraphQLite busy registration closes without double free");
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "schema") == 0) {
        sql(db, "CREATE VIEW edges AS SELECT 1 AS incompatible");
        assert(sqlite3_graphqlite_init(db, &error, NULL) != SQLITE_OK);
        assert(error != NULL);
        assert(integer(db, "SELECT count(*) FROM sqlite_schema WHERE type='table'") == 0);
        assert(integer(db, "SELECT count(*) FROM edges") == 1);
        sqlite3_free(error);
        assert(sqlite3_close(db) == SQLITE_OK);
        puts("PASS GraphQLite schema failure is propagated");
        return 0;
    }
    assert(integer(db, "SELECT count(*) FROM sqlite_schema WHERE type='table'") == 0);
    assert(sqlite3_graphqlite_init(db, &error, NULL) == SQLITE_OK);
    assert(sqlite3_vec_init(db, &error, NULL) == SQLITE_OK);
    sql(db, "SELECT cypher('CREATE (:Person {name: \"Alice\"})')");
    assert(integer(db, "SELECT json_array_length(cypher('MATCH (p:Person) RETURN p.name'))") == 1);
    assert(integer(db, "SELECT json_extract(cypher('RETURN \"abc123\" =~ \"[a-z]+[0-9]+\" AS ok'), '$[0].ok')") == 1);
    assert(integer(db, "SELECT vec_distance_l2('[0,0]', '[3,4]')") == 5);
    sql(db, "CREATE VIRTUAL TABLE embeddings USING vec0(embedding float[2]);"
            "INSERT INTO embeddings(rowid,embedding) VALUES(1,'[0,0]'),(2,'[3,4]')");
    assert(integer(db, "SELECT rowid FROM embeddings WHERE embedding MATCH '[0,0]' AND k=1") == 1);
    sqlite3* plain = NULL;
    assert(sqlite3_open(":memory:", &plain) == SQLITE_OK);
    assert(integer(plain, "SELECT count(*) FROM sqlite_schema WHERE type='table'") == 0);
    sqlite3_stmt* absent = NULL;
    assert(sqlite3_prepare_v2(plain, "SELECT vec_version()", -1, &absent, NULL) == SQLITE_ERROR);
    assert(sqlite3_close(plain) == SQLITE_OK);
    assert(sqlite3_close(db) == SQLITE_OK);
    puts("PASS shared SQLite, graph/regex/vector queries and per-connection isolation");
    return 0;
}
