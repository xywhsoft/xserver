/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Skeleton implementation for Bison GLR parsers in C

   Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C GLR parser skeleton written by Paul Hilfinger.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "glr.c"

/* Pure parsers.  */
#define YYPURE 1



/* Substitute the type names.  */
#define YYSTYPE CYPHER_YYSTYPE
#define YYLTYPE CYPHER_YYLTYPE
/* Substitute the variable and function names.  */
#define yyparse cypher_yyparse
#define yylex   cypher_yylex
#define yyerror cypher_yyerror
#define yydebug cypher_yydebug

/* First part of user prologue.  */
#line 1 "lib/graphqlite/src/backend/parser/cypher_gram.y"

/*
 * Cypher Grammar for GraphQLite
 * Simplified version based on Apache AGE grammar for SQLite compatibility
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "parser/cypher_ast.h"
#include "parser/cypher_parser.h"

/* Forward declarations */
void cypher_yyerror(CYPHER_YYLTYPE *yylloc, cypher_parser_context *context, const char *msg);
int cypher_yylex(CYPHER_YYSTYPE *yylval, CYPHER_YYLTYPE *yylloc, cypher_parser_context *context);


#line 84 "lib/graphqlite/generated/cypher_gram.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "cypher_gram.tab.h"

/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_INTEGER = 3,                    /* INTEGER  */
  YYSYMBOL_DECIMAL = 4,                    /* DECIMAL  */
  YYSYMBOL_STRING = 5,                     /* STRING  */
  YYSYMBOL_IDENTIFIER = 6,                 /* IDENTIFIER  */
  YYSYMBOL_PARAMETER = 7,                  /* PARAMETER  */
  YYSYMBOL_BQIDENT = 8,                    /* BQIDENT  */
  YYSYMBOL_NOT_EQ = 9,                     /* NOT_EQ  */
  YYSYMBOL_LT_EQ = 10,                     /* LT_EQ  */
  YYSYMBOL_GT_EQ = 11,                     /* GT_EQ  */
  YYSYMBOL_DOT_DOT = 12,                   /* DOT_DOT  */
  YYSYMBOL_TYPECAST = 13,                  /* TYPECAST  */
  YYSYMBOL_PLUS_EQ = 14,                   /* PLUS_EQ  */
  YYSYMBOL_REGEX_MATCH = 15,               /* REGEX_MATCH  */
  YYSYMBOL_MATCH = 16,                     /* MATCH  */
  YYSYMBOL_RETURN = 17,                    /* RETURN  */
  YYSYMBOL_CREATE = 18,                    /* CREATE  */
  YYSYMBOL_WHERE = 19,                     /* WHERE  */
  YYSYMBOL_WITH = 20,                      /* WITH  */
  YYSYMBOL_SET = 21,                       /* SET  */
  YYSYMBOL_DELETE = 22,                    /* DELETE  */
  YYSYMBOL_REMOVE = 23,                    /* REMOVE  */
  YYSYMBOL_MERGE = 24,                     /* MERGE  */
  YYSYMBOL_UNWIND = 25,                    /* UNWIND  */
  YYSYMBOL_DETACH = 26,                    /* DETACH  */
  YYSYMBOL_FOREACH = 27,                   /* FOREACH  */
  YYSYMBOL_CALL = 28,                      /* CALL  */
  YYSYMBOL_YIELD = 29,                     /* YIELD  */
  YYSYMBOL_OPTIONAL = 30,                  /* OPTIONAL  */
  YYSYMBOL_DISTINCT = 31,                  /* DISTINCT  */
  YYSYMBOL_ORDER = 32,                     /* ORDER  */
  YYSYMBOL_BY = 33,                        /* BY  */
  YYSYMBOL_SKIP = 34,                      /* SKIP  */
  YYSYMBOL_LIMIT = 35,                     /* LIMIT  */
  YYSYMBOL_AS = 36,                        /* AS  */
  YYSYMBOL_ASC = 37,                       /* ASC  */
  YYSYMBOL_DESC = 38,                      /* DESC  */
  YYSYMBOL_AND = 39,                       /* AND  */
  YYSYMBOL_OR = 40,                        /* OR  */
  YYSYMBOL_XOR = 41,                       /* XOR  */
  YYSYMBOL_NOT = 42,                       /* NOT  */
  YYSYMBOL_IN = 43,                        /* IN  */
  YYSYMBOL_IS = 44,                        /* IS  */
  YYSYMBOL_NULL_P = 45,                    /* NULL_P  */
  YYSYMBOL_TRUE_P = 46,                    /* TRUE_P  */
  YYSYMBOL_FALSE_P = 47,                   /* FALSE_P  */
  YYSYMBOL_EXISTS = 48,                    /* EXISTS  */
  YYSYMBOL_ANY = 49,                       /* ANY  */
  YYSYMBOL_NONE = 50,                      /* NONE  */
  YYSYMBOL_SINGLE = 51,                    /* SINGLE  */
  YYSYMBOL_REDUCE = 52,                    /* REDUCE  */
  YYSYMBOL_UNION = 53,                     /* UNION  */
  YYSYMBOL_ALL = 54,                       /* ALL  */
  YYSYMBOL_CASE = 55,                      /* CASE  */
  YYSYMBOL_WHEN = 56,                      /* WHEN  */
  YYSYMBOL_THEN = 57,                      /* THEN  */
  YYSYMBOL_ELSE = 58,                      /* ELSE  */
  YYSYMBOL_END_P = 59,                     /* END_P  */
  YYSYMBOL_ON = 60,                        /* ON  */
  YYSYMBOL_SHORTESTPATH = 61,              /* SHORTESTPATH  */
  YYSYMBOL_ALLSHORTESTPATHS = 62,          /* ALLSHORTESTPATHS  */
  YYSYMBOL_PATTERN = 63,                   /* PATTERN  */
  YYSYMBOL_EXPLAIN = 64,                   /* EXPLAIN  */
  YYSYMBOL_LOAD = 65,                      /* LOAD  */
  YYSYMBOL_CSV = 66,                       /* CSV  */
  YYSYMBOL_FROM = 67,                      /* FROM  */
  YYSYMBOL_HEADERS = 68,                   /* HEADERS  */
  YYSYMBOL_FIELDTERMINATOR = 69,           /* FIELDTERMINATOR  */
  YYSYMBOL_STARTS = 70,                    /* STARTS  */
  YYSYMBOL_ENDS = 71,                      /* ENDS  */
  YYSYMBOL_CONTAINS = 72,                  /* CONTAINS  */
  YYSYMBOL_73_ = 73,                       /* '='  */
  YYSYMBOL_74_ = 74,                       /* '<'  */
  YYSYMBOL_75_ = 75,                       /* '>'  */
  YYSYMBOL_76_ = 76,                       /* '+'  */
  YYSYMBOL_77_ = 77,                       /* '-'  */
  YYSYMBOL_78_ = 78,                       /* '*'  */
  YYSYMBOL_79_ = 79,                       /* '/'  */
  YYSYMBOL_80_ = 80,                       /* '%'  */
  YYSYMBOL_81_ = 81,                       /* '^'  */
  YYSYMBOL_UNARY_MINUS = 82,               /* UNARY_MINUS  */
  YYSYMBOL_UNARY_PLUS = 83,                /* UNARY_PLUS  */
  YYSYMBOL_84_ = 84,                       /* '.'  */
  YYSYMBOL_85_ = 85,                       /* '['  */
  YYSYMBOL_86_ = 86,                       /* ';'  */
  YYSYMBOL_87_ = 87,                       /* ','  */
  YYSYMBOL_88_ = 88,                       /* '('  */
  YYSYMBOL_89_ = 89,                       /* '|'  */
  YYSYMBOL_90_ = 90,                       /* ')'  */
  YYSYMBOL_91_ = 91,                       /* '{'  */
  YYSYMBOL_92_ = 92,                       /* '}'  */
  YYSYMBOL_93_ = 93,                       /* ':'  */
  YYSYMBOL_94_ = 94,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 95,                  /* $accept  */
  YYSYMBOL_stmt = 96,                      /* stmt  */
  YYSYMBOL_union_query = 97,               /* union_query  */
  YYSYMBOL_single_query = 98,              /* single_query  */
  YYSYMBOL_clause_list = 99,               /* clause_list  */
  YYSYMBOL_clause = 100,                   /* clause  */
  YYSYMBOL_match_clause = 101,             /* match_clause  */
  YYSYMBOL_from_graph_opt = 102,           /* from_graph_opt  */
  YYSYMBOL_optional_opt = 103,             /* optional_opt  */
  YYSYMBOL_return_clause = 104,            /* return_clause  */
  YYSYMBOL_with_clause = 105,              /* with_clause  */
  YYSYMBOL_unwind_clause = 106,            /* unwind_clause  */
  YYSYMBOL_foreach_clause = 107,           /* foreach_clause  */
  YYSYMBOL_call_clause = 108,              /* call_clause  */
  YYSYMBOL_proc_args_opt = 109,            /* proc_args_opt  */
  YYSYMBOL_yield_item_list = 110,          /* yield_item_list  */
  YYSYMBOL_yield_item = 111,               /* yield_item  */
  YYSYMBOL_procedure_name = 112,           /* procedure_name  */
  YYSYMBOL_load_csv_clause = 113,          /* load_csv_clause  */
  YYSYMBOL_foreach_update_list = 114,      /* foreach_update_list  */
  YYSYMBOL_distinct_opt = 115,             /* distinct_opt  */
  YYSYMBOL_order_by_opt = 116,             /* order_by_opt  */
  YYSYMBOL_skip_opt = 117,                 /* skip_opt  */
  YYSYMBOL_limit_opt = 118,                /* limit_opt  */
  YYSYMBOL_where_opt = 119,                /* where_opt  */
  YYSYMBOL_order_by_list = 120,            /* order_by_list  */
  YYSYMBOL_order_by_item = 121,            /* order_by_item  */
  YYSYMBOL_return_item_list = 122,         /* return_item_list  */
  YYSYMBOL_return_item = 123,              /* return_item  */
  YYSYMBOL_set_item_list = 124,            /* set_item_list  */
  YYSYMBOL_set_target = 125,               /* set_target  */
  YYSYMBOL_set_item = 126,                 /* set_item  */
  YYSYMBOL_create_clause = 127,            /* create_clause  */
  YYSYMBOL_merge_clause = 128,             /* merge_clause  */
  YYSYMBOL_on_create_clause = 129,         /* on_create_clause  */
  YYSYMBOL_on_match_clause = 130,          /* on_match_clause  */
  YYSYMBOL_set_clause = 131,               /* set_clause  */
  YYSYMBOL_delete_clause = 132,            /* delete_clause  */
  YYSYMBOL_delete_item_list = 133,         /* delete_item_list  */
  YYSYMBOL_delete_item = 134,              /* delete_item  */
  YYSYMBOL_remove_clause = 135,            /* remove_clause  */
  YYSYMBOL_remove_item_list = 136,         /* remove_item_list  */
  YYSYMBOL_remove_item = 137,              /* remove_item  */
  YYSYMBOL_detach_opt = 138,               /* detach_opt  */
  YYSYMBOL_pattern_list = 139,             /* pattern_list  */
  YYSYMBOL_simple_path = 140,              /* simple_path  */
  YYSYMBOL_path = 141,                     /* path  */
  YYSYMBOL_node_pattern = 142,             /* node_pattern  */
  YYSYMBOL_rel_pattern = 143,              /* rel_pattern  */
  YYSYMBOL_variable_opt = 144,             /* variable_opt  */
  YYSYMBOL_varlen_range_opt = 145,         /* varlen_range_opt  */
  YYSYMBOL_label_opt = 146,                /* label_opt  */
  YYSYMBOL_label_name = 147,               /* label_name  */
  YYSYMBOL_label_alt_list = 148,           /* label_alt_list  */
  YYSYMBOL_label_list = 149,               /* label_list  */
  YYSYMBOL_rel_type_list = 150,            /* rel_type_list  */
  YYSYMBOL_expr = 151,                     /* expr  */
  YYSYMBOL_primary_expr = 152,             /* primary_expr  */
  YYSYMBOL_literal_expr = 153,             /* literal_expr  */
  YYSYMBOL_function_call = 154,            /* function_call  */
  YYSYMBOL_list_predicate = 155,           /* list_predicate  */
  YYSYMBOL_reduce_expr = 156,              /* reduce_expr  */
  YYSYMBOL_argument_list = 157,            /* argument_list  */
  YYSYMBOL_list_literal = 158,             /* list_literal  */
  YYSYMBOL_list_comprehension = 159,       /* list_comprehension  */
  YYSYMBOL_pattern_comprehension = 160,    /* pattern_comprehension  */
  YYSYMBOL_comp_steps = 161,               /* comp_steps  */
  YYSYMBOL_map_literal = 162,              /* map_literal  */
  YYSYMBOL_map_projection = 163,           /* map_projection  */
  YYSYMBOL_map_projection_list = 164,      /* map_projection_list  */
  YYSYMBOL_map_projection_item = 165,      /* map_projection_item  */
  YYSYMBOL_case_expression = 166,          /* case_expression  */
  YYSYMBOL_when_clause_list = 167,         /* when_clause_list  */
  YYSYMBOL_when_clause = 168,              /* when_clause  */
  YYSYMBOL_literal = 169,                  /* literal  */
  YYSYMBOL_identifier = 170,               /* identifier  */
  YYSYMBOL_soft_kw = 171,                  /* soft_kw  */
  YYSYMBOL_var_name = 172,                 /* var_name  */
  YYSYMBOL_non_reserved_kw = 173,          /* non_reserved_kw  */
  YYSYMBOL_parameter = 174,                /* parameter  */
  YYSYMBOL_properties_opt = 175,           /* properties_opt  */
  YYSYMBOL_map_pair_list = 176,            /* map_pair_list  */
  YYSYMBOL_map_pair = 177                  /* map_pair  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;


/* Default (constant) value used for initialization for null
   right-hand sides.  Unlike the standard yacc.c template, here we set
   the default value of $$ to a zeroed-out value.  Since the default
   value is undefined, this behavior is technically correct.  */
static YYSTYPE yyval_default;
static YYLTYPE yyloc_default
# if defined CYPHER_YYLTYPE_IS_TRIVIAL && CYPHER_YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;


/* Unqualified %code blocks.  */
#line 26 "lib/graphqlite/src/backend/parser/cypher_gram.y"

/* The pattern list of a pattern comprehension: one path of `first`
 * followed by the relationship-node `steps` (moved, the list freed). */
static ast_list *comp_pattern(cypher_node_pattern *first, ast_list *steps)
{
    ast_list *elements = ast_list_create();
    ast_list_append(elements, (ast_node*)first);
    for (int i = 0; steps && i < steps->count; i++)
        ast_list_append(elements, steps->items[i]);
    if (steps) {
        steps->count = 0;          /* items moved: free only the list */
        ast_list_free(steps);
    }
    ast_list *pattern = ast_list_create();
    ast_list_append(pattern, (ast_node*)make_path(elements));
    return pattern;
}

/* COUNT { pattern [WHERE expr] } or COUNT { query } (GQLITE-T-0485): the
 * internal call __gql_count_subquery(EXISTS { query }), whose transform
 * counts the rows of the subquery. A pattern becomes MATCH pattern [WHERE].
 * `name` must be count; NULL after a syntax error. Takes `name`. */
static ast_node *make_count_subquery(cypher_parser_context *context, char *name,
                                     ast_list *pattern, ast_node *where,
                                     ast_node *query, CYPHER_YYLTYPE *loc)
{
    if (!name || strcasecmp(name, "count") != 0) {
        cypher_yyerror(loc, context, "a { } subquery is only valid after COUNT or EXISTS");
        free(name);
        return NULL;
    }
    free(name);
    if (!query) {
        ast_list *clauses = ast_list_create();
        ast_list_append(clauses, (ast_node*)make_cypher_match(pattern, where, false, NULL));
        query = (ast_node*)make_cypher_query(clauses, false);
    }
    cypher_exists_expr *ee = make_exists_query_expr(query, loc->first_line);
    if (ee) ee->is_subquery = true;
    ast_list *args = ast_list_create();
    ast_list_append(args, (ast_node*)ee);
    return (ast_node*)make_function_call("__gql_count_subquery", args, false, loc->first_line);
}

/* The source text from (line, col) to (last_line, last_col), both
 * 1-based and inclusive, trimmed. NULL when the input cannot be read. */
static char *source_text_span(cypher_parser_context *context, int line, int col,
                              int last_line, int last_col)
{
    const char *in = (context && context->scanner) ? context->scanner->input_string : NULL;
    if (!in || line < 1 || col < 1 || last_line < line) return NULL;
    const char *p = in;
    for (int l = 1; l < line && *p; p++) if (*p == '\n') l++;
    for (int c = 1; c < col && *p && *p != '\n'; c++) p++;
    const char *q = p;
    int l = line, c = col;
    while (*q && (l < last_line || (l == last_line && c < last_col))) {
        if (*q == '\n') { l++; c = 1; } else c++;
        q++;
    }
    if (*q) q++;   /* inclusive end */
    while (p < q && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
    while (q > p && (q[-1] == ' ' || q[-1] == '\t' || q[-1] == '\n' || q[-1] == '\r')) q--;
    if (q <= p) return NULL;
    return strndup(p, (size_t)(q - p));
}

/* The spelling of a keyword as written, at `line` / `col` (1-based) of the
 * input, for a keyword used as a map key: `{NULL: 1}` keeps the key "NULL"
 * (the scanner upper-cases keywords; Map1 [5], Map2 [5]). `fallback` when
 * the input cannot be read. Owned by the caller. */
static char *keyword_text_at(cypher_parser_context *context, int line, int col,
                             const char *fallback)
{
    const char *in = (context && context->scanner) ? context->scanner->input_string : NULL;
    if (!in || line < 1 || col < 1) return strdup(fallback);
    for (int l = 1; l < line && *in; in++) if (*in == '\n') l++;
    for (int c = 1; c < col && *in && *in != '\n'; c++) in++;
    size_t n = 0;
    while (in[n] && ((in[n] >= 'A' && in[n] <= 'Z') || (in[n] >= 'a' && in[n] <= 'z') ||
                     in[n] == '_' || (in[n] >= '0' && in[n] <= '9'))) n++;
    if (n == 0 || strncasecmp(in, fallback, n) != 0 || strlen(fallback) != n) return strdup(fallback);
    return strndup(in, n);
}

#line 394 "lib/graphqlite/generated/cypher_gram.tab.c"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif
#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YYFREE
# define YYFREE free
#endif
#ifndef YYMALLOC
# define YYMALLOC malloc
#endif
#ifndef YYREALLOC
# define YYREALLOC realloc
#endif

#ifdef __cplusplus
  typedef bool yybool;
# define yytrue true
# define yyfalse false
#else
  /* When we move to stdbool, get rid of the various casts to yybool.  */
  typedef signed char yybool;
# define yytrue 1
# define yyfalse 0
#endif

#ifndef YYSETJMP
# include <setjmp.h>
# define YYJMP_BUF jmp_buf
# define YYSETJMP(Env) setjmp (Env)
/* Pacify Clang and ICC.  */
# define YYLONGJMP(Env, Val)                    \
 do {                                           \
   longjmp (Env, Val);                          \
   YY_ASSERT (0);                               \
 } while (yyfalse)
#endif

#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* The _Noreturn keyword of C11.  */
#ifndef _Noreturn
# if (defined __cplusplus \
      && ((201103 <= __cplusplus && !(__GNUC__ == 4 && __GNUC_MINOR__ == 7)) \
          || (defined _MSC_VER && 1900 <= _MSC_VER)))
#  define _Noreturn [[noreturn]]
# elif ((!defined __cplusplus || defined __clang__) \
        && (201112 <= (defined __STDC_VERSION__ ? __STDC_VERSION__ : 0) \
            || (!defined __STRICT_ANSI__ \
                && (4 < __GNUC__ + (7 <= __GNUC_MINOR__) \
                    || (defined __apple_build_version__ \
                        ? 6000000 <= __apple_build_version__ \
                        : 3 < __clang_major__ + (5 <= __clang_minor__))))))
   /* _Noreturn works as-is.  */
# elif (2 < __GNUC__ + (8 <= __GNUC_MINOR__) || defined __clang__ \
        || 0x5110 <= __SUNPRO_C)
#  define _Noreturn __attribute__ ((__noreturn__))
# elif 1200 <= (defined _MSC_VER ? _MSC_VER : 0)
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  129
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   4665

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  95
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  83
/* YYNRULES -- Number of rules.  */
#define YYNRULES  405
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  806
/* YYMAXRHS -- Maximum number of symbols on right-hand side of rule.  */
#define YYMAXRHS 14
/* YYMAXLEFT -- Maximum number of symbols to the left of a handle
   accessed by $0, $-1, etc., in any rule.  */
#define YYMAXLEFT 0

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   329

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,    80,     2,     2,
      88,    90,    78,    76,    87,    77,    84,    79,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    93,    86,
      74,    73,    75,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    85,     2,    94,    81,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    91,    89,    92,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    82,    83
};

#if CYPHER_YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   266,   266,   271,   276,   285,   296,   300,   304,   311,
     318,   323,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   347,   354,   355,   362,   363,   368,
     372,   377,   385,   389,   394,   405,   410,   420,   425,   439,
     445,   452,   457,   462,   470,   471,   472,   476,   481,   489,
     494,   503,   505,   517,   523,   529,   536,   547,   552,   557,
     562,   567,   572,   577,   582,   587,   592,   597,   602,   610,
     611,   615,   616,   620,   621,   625,   626,   630,   631,   635,
     640,   648,   649,   650,   655,   660,   668,   676,   681,   692,
     697,   702,   718,   745,   746,   747,   752,   757,   765,   769,
     774,   787,   795,   799,   803,   807,   811,   818,   825,   833,
     841,   848,   853,   861,   870,   877,   882,   887,   903,   927,
     936,   945,   954,   963,   970,   975,   987,   992,   999,  1004,
    1013,  1019,  1038,  1042,  1048,  1054,  1061,  1069,  1074,  1081,
    1089,  1097,  1113,  1118,  1124,  1130,  1137,  1145,  1150,  1156,
    1162,  1169,  1179,  1184,  1190,  1196,  1202,  1210,  1215,  1221,
    1227,  1234,  1242,  1246,  1250,  1256,  1263,  1264,  1265,  1266,
    1272,  1273,  1275,  1277,  1279,  1281,  1283,  1285,  1299,  1300,
    1301,  1312,  1313,  1314,  1318,  1325,  1334,  1341,  1348,  1355,
    1362,  1369,  1379,  1389,  1400,  1410,  1421,  1431,  1442,  1452,
    1463,  1470,  1477,  1484,  1495,  1496,  1497,  1526,  1527,  1528,
    1529,  1530,  1531,  1532,  1533,  1536,  1537,  1538,  1539,  1540,
    1541,  1542,  1543,  1544,  1545,  1546,  1547,  1548,  1549,  1550,
    1551,  1557,  1570,  1584,  1589,  1594,  1599,  1603,  1607,  1611,
    1618,  1619,  1620,  1621,  1622,  1623,  1624,  1625,  1626,  1627,
    1628,  1629,  1630,  1637,  1653,  1666,  1683,  1710,  1717,  1727,
    1738,  1753,  1779,  1786,  1794,  1799,  1809,  1816,  1827,  1833,
    1838,  1844,  1854,  1858,  1863,  1872,  1877,  1882,  1887,  1896,
    1907,  1912,  1921,  1925,  1945,  1950,  1955,  1960,  1977,  1983,
    1990,  1999,  2013,  2019,  2029,  2033,  2041,  2050,  2055,  2063,
    2069,  2074,  2081,  2087,  2102,  2106,  2111,  2115,  2122,  2127,
    2135,  2142,  2146,  2150,  2155,  2159,  2163,  2170,  2175,  2181,
    2186,  2199,  2200,  2201,  2202,  2203,  2204,  2205,  2206,  2207,
    2208,  2209,  2210,  2211,  2212,  2213,  2214,  2218,  2219,  2225,
    2226,  2227,  2228,  2229,  2230,  2231,  2232,  2233,  2234,  2235,
    2236,  2237,  2238,  2239,  2240,  2241,  2242,  2243,  2244,  2245,
    2246,  2247,  2248,  2249,  2250,  2251,  2252,  2253,  2254,  2255,
    2256,  2257,  2258,  2259,  2260,  2261,  2262,  2263,  2264,  2265,
    2266,  2267,  2268,  2269,  2270,  2271,  2272,  2273,  2274,  2275,
    2276,  2277,  2281,  2290,  2291,  2298,  2305,  2310,  2318,  2323,
    2328,  2333,  2338,  2342,  2349,  2355
};
#endif

#define YYPACT_NINF (-528)
#define YYTABLE_NINF (-339)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    1706,    23,  2357,    40,   397,  2404,  2357,  1343,  -528,   -29,
      11,  -528,  1506,    59,   143,    83,  -528,   655,  -528,  -528,
     162,  -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,
    -528,  -528,   134,   104,   178,  1343,  -528,   180,  -528,  -528,
    -528,  -528,  -528,  -528,  -528,   172,   193,  -528,  -528,  -528,
    -528,  -528,  -528,  -528,  4492,   230,   -28,  -528,  -528,  -528,
     268,  -528,    12,  1343,   249,  -528,  -528,  -528,  -528,  1343,
     263,    20,  -528,  -528,   279,   258,   272,  1343,   275,  -528,
     -21,   119,  -528,  -528,  -528,   311,  -528,  1343,  -528,  -528,
    -528,   280,   290,   291,   295,   318,   329,   968,   330,   334,
     347,  1343,  1343,   742,  1418,  1569,   -28,  3230,  -528,  -528,
    -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,
    -528,   343,  -528,  4518,  -528,  1506,    47,    89,    53,  -528,
    2208,  -528,  -528,  2357,  1343,   178,   424,   439,    13,  -528,
    3277,    67,   399,   399,  -528,  -528,   386,  -528,  2357,   427,
      60,   399,    92,  1343,   439,    13,  2465,   397,  1343,  1343,
    4023,  4090,    61,  2513,  2404,   325,  4090,   426,   449,   450,
     511,   892,  2260,  3832,  2432,  2329,  4596,  4596,  4596,  4596,
    4596,  1343,  3324,   338,  -528,  1343,  1343,  1343,   140,   140,
    1493,  -528,   151,    10,   350,   360,  2561,   425,   430,   445,
     457,  -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,
    -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,
    -528,  -528,   458,   459,  -528,  -528,  -528,  -528,  -528,  -528,
    -528,   460,   461,  -528,  -528,  -528,  -528,  -528,  -528,  -528,
    -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,
    -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,  -528,   464,
     231,  -528,   399,  1343,  1343,  1343,  1343,  4544,  1343,  1343,
    1343,  1343,   369,   513,   538,  1343,  1343,  1343,  1343,  1343,
    1343,  1343,  1343,  1343,  1343,  4157,  1043,  4090,   516,   517,
      70,   555,  1118,   533,  -528,   495,   559,  1506,  -528,   -32,
     478,  -528,  3703,   439,  1343,  1343,   547,  1343,   439,  4570,
     497,   498,   -28,    73,   145,  4224,   493,   494,  -528,   107,
     514,  4492,  -528,   503,   504,   -28,    13,   547,   439,  -528,
    -528,   500,  3703,  3703,  -528,  -528,  -528,  -528,  -528,   501,
    -528,  -528,  -528,   515,  -528,   -18,  -528,  -528,   502,   577,
     580,   586,  -528,   585,  -528,   520,  1343,   527,  -528,  3703,
     380,   -36,   -29,    11,    59,    30,   512,    29,   243,  -528,
     526,   381,   529,    32,   579,   581,   582,   550,   583,  3372,
     396,  1343,  -528,  -528,   382,   391,   393,   386,  -528,  1343,
     539,  -528,  1343,  1343,  1343,  1343,  1343,  1343,  1343,  1343,
    3956,  -528,   412,   535,   535,   535,   535,  -528,  -528,  3832,
    3750,  3793,   353,   584,  -528,  1343,  1343,   353,   535,   535,
     535,   435,   435,   210,   210,   210,   140,  -528,  -528,  -528,
    1343,  1710,   287,   542,  1343,  1343,  -528,  -528,  -528,   440,
    3891,   563,   596,  -528,   630,   618,  1343,   547,   551,  -528,
    3183,  3703,  1343,  -528,  -528,   547,  -528,  -528,   399,   399,
    -528,  -528,   556,   558,   560,   561,   562,  1646,   566,  4291,
     573,  4492,  -528,    88,   399,   399,   439,   618,   547,  4090,
    4090,   392,  4090,  4090,   397,   397,  1193,  2609,  -528,  1343,
    -528,  1268,  -528,  -528,  -528,  1343,  -528,    14,  -528,   646,
    -528,  -528,  1343,  -528,  1343,  1343,  1343,  1343,  1343,  1343,
    1343,  -528,  3421,  -528,  -528,  -528,   493,   624,  4492,  3703,
    3703,  3703,  3703,  3703,  3703,  3703,  3703,  -528,   399,  -528,
     353,   353,  1804,   817,  -528,  4090,  4090,  4090,  2897,  2944,
    -528,  -528,    19,  -528,   617,   652,  4596,  -528,  1343,  -528,
    -528,  -528,  1343,  -528,  -528,  3703,  -528,   213,   238,  -528,
     256,  -528,  -528,  -528,  -528,  -528,   111,    28,  4358,   493,
     242,   283,   547,  -528,   618,  -528,   565,  -528,  -528,  -528,
     567,   263,   263,  -528,   441,  -528,  3703,   653,  3703,   496,
     568,  -528,   572,  2180,  3468,  3515,  3562,  3132,  3609,  3703,
    3656,  -528,   576,  1343,  1343,  -528,   386,  -528,  -528,  -528,
    1851,  -528,   591,  -528,   389,   389,  1343,  4596,  4596,   633,
     605,  3703,  -528,  -528,  -528,  -528,  4425,   493,   674,   685,
     689,   -31,   118,   176,   615,   612,  -528,  -528,   618,  -528,
    4090,  4090,  -528,  -528,  -528,  -528,  -528,  1343,  1343,  1343,
    4596,  1343,  -528,   455,  1757,  1898,   493,  -528,  4090,  -528,
      58,  -528,  -528,  -528,  -528,  -528,   216,  3703,  -528,  -528,
    4596,   705,   -31,   118,   176,   615,   620,   708,  -528,  -528,
      18,   493,    21,   493,    24,   493,   493,   638,  -528,  -528,
    -528,  2657,  2705,  2753,   673,  2801,   399,    33,  1343,  -528,
    -528,   627,   626,  -528,  -528,  -528,  -528,  -528,  -528,  -528,
    -528,   654,  -528,   493,   493,   493,   493,   644,  -528,  -528,
    -528,   488,   631,  -528,  -528,   540,   632,  -528,  -528,   541,
     634,   636,   649,  -528,  -528,  -528,  1343,  -528,  -528,  1343,
    1343,   399,  1945,   -28,  4090,   722,   637,   640,   641,   642,
     662,  -528,  -528,   661,  -528,  -528,   663,  -528,  -528,   664,
     666,  -528,  2991,  3038,  1992,  -528,  -528,   222,  -528,  -528,
     667,   675,   676,   677,  -528,   680,   681,   683,   684,  1343,
    1343,  -528,  1343,  1343,   686,   687,   688,   690,  -528,  -528,
    -528,  -528,  2849,  2039,  3085,  2086,  -528,  -528,  -528,  -528,
    -528,  -528,  1343,  -528,  2133,  -528
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
      27,    69,     0,    69,     0,     0,     0,     0,   126,     0,
       0,    28,    27,     0,     0,     2,     6,     9,    10,    12,
       0,    13,    14,    15,    16,    23,    17,    18,    19,    20,
      21,    22,     0,    70,    71,     0,   337,     0,   334,   333,
     321,   330,   331,   332,   329,   335,   336,   327,   328,   324,
     323,   322,   325,   326,   166,   101,   132,   128,   130,   338,
       0,    70,    71,     0,   317,   319,   320,   335,   336,     0,
     109,     0,    89,    93,   318,     0,     0,     0,   114,   115,
       0,   102,   311,   312,   313,   317,   392,     0,   316,   314,
     315,     0,   330,   331,   332,   329,     0,     0,     0,     0,
       0,     0,     0,     0,   166,     0,     0,     0,   204,   240,
     243,   244,   245,   246,   247,   248,   249,   250,   251,   257,
     241,     0,   242,     0,    51,    27,    44,     4,     0,     1,
      27,     3,    11,     0,     0,    71,     0,    73,    71,    84,
      86,     0,     0,     0,   168,   169,   178,   167,     0,     0,
       0,     0,     0,     0,    73,    71,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   103,   104,
       0,     0,    27,   227,     0,    27,     0,     0,     0,     0,
       0,     0,     0,     0,   308,     0,     0,     0,   205,   206,
     166,   282,     0,     0,   319,   320,     0,   167,     0,     0,
       0,   368,   369,   370,   371,   372,   373,   374,   375,   376,
     377,   378,   379,   380,   353,   381,   382,   361,   362,   383,
     384,   356,     0,     0,   358,   357,   359,   360,   354,   355,
     363,     0,     0,   343,   340,   341,   339,   347,   385,   342,
     364,   365,   366,   367,   348,   349,   390,   391,   350,   389,
     352,   351,   386,   387,   388,   345,   346,   344,   294,     0,
       0,   396,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    40,     5,     0,     0,    27,     7,    25,
     110,   111,   113,    73,     0,     0,    75,     0,    73,     0,
       0,     0,   134,     0,     0,     0,   393,   179,   129,     0,
     165,   166,   131,     0,     0,   133,    71,    75,    73,    94,
      90,     0,    99,    98,    95,    96,    97,   181,   182,   100,
     183,   121,   122,     0,   116,     0,   119,   120,   125,     0,
       0,     0,   105,     0,   106,     0,     0,     0,   260,   280,
       0,   303,   334,   333,   324,     0,     0,     0,     0,   297,
     337,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   304,   309,     0,     0,     0,   178,   283,     0,
       0,   230,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   295,   231,   214,   217,   218,   219,    36,    35,   220,
     221,   222,   223,     0,   228,     0,     0,   226,   213,   215,
     216,   207,   208,   209,   210,   211,   212,   233,   234,   235,
       0,     0,   252,   253,     0,     0,    39,    52,    45,     0,
       0,     0,     0,     8,     0,    77,     0,    75,    72,    79,
      81,    74,     0,    30,    85,    75,    88,    87,     0,     0,
     137,   140,   186,   187,     0,   180,   188,     0,     0,     0,
     163,   166,   162,   170,     0,     0,    73,    77,    75,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   263,     0,
     261,     0,   299,   300,   270,     0,   268,     0,   296,     0,
     264,   266,     0,   265,     0,     0,     0,     0,     0,     0,
       0,   306,     0,   273,   274,   272,   393,     0,   166,   400,
     398,   399,   402,   405,   403,   404,   401,   397,     0,   229,
     224,   225,     0,     0,   236,     0,     0,     0,     0,     0,
      46,    41,    42,    47,    49,     0,     0,    26,     0,    24,
     112,    31,     0,    82,    83,    76,    29,     0,     0,   394,
       0,   141,   189,   191,   190,   164,   170,   171,     0,   393,
       0,     0,    75,    33,    77,   100,    91,   123,   124,   125,
     117,   108,   107,   259,     0,   262,   281,     0,   302,     0,
     303,   298,     0,     0,     0,     0,     0,     0,     0,   310,
       0,   305,     0,     0,     0,   284,   178,   232,   239,   238,
       0,   184,   254,   185,   127,   127,     0,     0,     0,     0,
      53,    78,    80,   136,   139,   395,     0,   393,   172,   176,
       0,   170,   170,   170,   170,     0,   135,   138,    77,    32,
       0,     0,   258,   301,   269,   271,   267,     0,     0,     0,
       0,     0,   307,     0,     0,     0,   393,   237,     0,    62,
     127,    57,    60,    58,    59,    61,   127,    43,    48,    50,
       0,     0,   170,   170,   170,   170,     0,   174,   175,   177,
       0,   393,     0,   393,     0,   393,   393,     0,    34,    92,
     118,     0,     0,     0,     0,     0,     0,     0,     0,   285,
     286,     0,   255,    38,    68,    63,    66,    64,    65,    67,
      37,    54,    55,   393,   393,   393,   393,     0,   173,   192,
     194,     0,     0,   196,   198,     0,     0,   200,   201,     0,
       0,     0,   157,   276,   277,   278,     0,   275,   292,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     147,   193,   195,     0,   197,   199,     0,   202,   203,     0,
       0,   142,     0,     0,     0,   293,   287,     0,   256,    56,
       0,     0,     0,     0,   152,   158,   159,   161,   160,     0,
       0,   288,     0,     0,   148,   149,   151,   150,   143,   144,
     146,   145,     0,     0,     0,     0,   153,   154,   156,   155,
     279,   289,     0,   290,     0,   291
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -528,  -528,     9,  -111,  -528,   734,  -528,  -528,  -528,  -528,
    -528,  -528,  -527,  -528,  -528,  -528,   122,  -528,  -528,   149,
     757,   -46,  -131,  -317,  -462,  -528,   214,   -38,   467,  -218,
    -528,   610,  -326,  -322,   599,   609,  -313,  -305,  -528,   332,
    -301,  -528,   616,  -528,    54,  -138,   635,    16,   -97,  -189,
    -348,  -385,  -116,   466,  -528,   153,    -7,  -528,  -528,  -528,
    -528,  -528,  -179,  -528,  -528,  -528,    39,  -528,  -528,  -528,
     288,  -528,   604,  -172,  -528,     8,    72,    37,   -92,  -528,
    -483,   331,   400
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    14,    15,    16,    17,    18,    19,   445,    20,    21,
      22,    23,    24,    25,   293,   542,   543,   126,    26,   660,
      35,   137,   306,   453,   549,   448,   449,   138,   139,    70,
      71,    72,    27,    28,   168,   169,    29,    30,   300,   301,
      31,    78,    79,    32,    55,    56,    57,   106,   151,   146,
     569,   316,   339,   433,   317,   633,   359,   108,   109,   110,
     111,   112,   360,   113,   114,   115,   697,   116,   117,   368,
     369,   118,   183,   184,   119,   120,    74,   121,   340,   122,
     468,   260,   261
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     107,   387,   516,   312,   313,   314,   384,   385,   386,   262,
     477,   383,    73,   259,   325,   573,   154,   124,    58,   298,
     590,   127,    58,   327,   719,   155,   720,   723,   140,   724,
     727,   628,   728,   602,   158,   444,   492,  -337,   616,    60,
     629,    75,    80,    60,   136,   136,   149,   567,   495,   150,
     348,   502,   739,   389,    33,   148,   140,   491,   680,   123,
      81,   366,   156,   165,   372,   192,   165,   341,   336,   342,
     163,    61,   166,   295,    59,   482,     2,    59,    59,     4,
     173,     5,     6,   390,     8,     9,   635,   659,   659,   303,
     182,   147,   308,   159,   188,   189,   140,   196,   365,   153,
     307,    34,   125,   287,   160,   630,   617,   149,   493,   328,
     150,   721,   639,   439,   725,   326,   148,   729,    62,   148,
     296,   496,   740,   130,   503,   128,    59,   302,   310,   311,
     551,   291,   473,   704,   290,   292,   130,   320,   556,   704,
     193,   197,   130,   129,   676,   321,   140,   149,   703,    58,
     150,   332,   333,   323,   324,    54,   134,    58,    58,    58,
     289,   574,   436,   460,    58,    73,   567,   322,    58,   131,
      60,   432,   447,   701,   379,   294,   688,   455,   133,   167,
      54,   568,   135,   196,   470,    60,   443,   299,    58,   567,
      58,    58,   471,   429,   331,    59,   567,   478,   722,   464,
     726,   345,   730,   731,   626,    59,   148,   682,   383,    60,
     136,    60,    60,   374,   375,   376,   377,   378,   627,   149,
      59,   656,   150,   466,   285,   286,   367,   197,   371,   373,
     746,   747,   748,   749,     2,   461,    59,     4,   307,     5,
       6,   782,     8,     9,    59,   388,    59,    59,    59,    59,
      59,    59,    59,   141,   567,   638,   403,   404,   405,   406,
     142,   409,   410,   411,   412,   684,   581,   582,   417,   418,
     419,   420,   421,   422,   423,   424,   425,   426,   402,   431,
     476,   143,   566,   681,   683,   685,   686,   149,   661,   661,
     150,   284,   662,   662,   285,   286,   149,   450,   451,   150,
     140,   663,   663,   623,   408,   528,   710,   584,   259,   664,
     664,   783,   149,   665,   665,   150,   149,   148,   400,   150,
     557,   558,  -318,   401,   713,   714,   715,   716,   624,   606,
     497,   346,   636,   347,   705,   498,   570,   571,   706,    59,
     705,   152,  -337,   400,   706,   572,   457,   707,   625,   487,
     157,   161,  -318,   707,  -317,   708,   162,   149,   147,   709,
     150,   708,   164,   575,   576,   709,   579,   580,   174,  -318,
    -338,   175,  -338,   637,   512,   259,   535,   564,   176,   177,
     536,    59,   517,   178,  -317,   519,   520,   521,   522,   523,
     524,   525,   526,    59,   181,   170,   381,   382,   577,   171,
     578,  -317,   172,    64,  -337,    65,   179,     2,   530,   531,
       4,   413,     5,     6,   414,     8,     9,   180,   185,   611,
     612,   613,   186,   532,    38,    39,    40,   538,   539,   279,
     280,   281,   282,   283,   284,   187,   287,   285,   286,   302,
    -168,  -168,   349,  -168,   350,   555,    41,    42,    43,    44,
    -169,  -169,   181,  -169,   510,   511,    66,   304,    67,    68,
      47,    48,    49,    50,    51,    52,    53,   489,   148,   489,
     490,   500,   513,   305,    58,    58,   634,   544,   489,   315,
     489,   514,   586,   515,   588,    69,   149,    54,   589,   150,
      58,    58,    73,    73,   751,   593,   752,   594,   595,   596,
     597,   598,   599,   600,   319,   263,   264,   265,   147,   351,
     353,   266,    59,   281,   282,   283,   284,   355,   287,   285,
     286,    75,    75,   392,   689,   690,   610,   489,   489,   149,
     540,   642,   150,   415,   675,   268,   269,   270,   393,   271,
     272,   621,   702,    59,   607,   450,   754,   757,   755,   758,
     394,   395,   396,   397,   398,   147,   696,   399,   416,   434,
     435,   437,   440,   441,   442,   446,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   271,   272,
     285,   286,   452,   620,   467,   458,   459,   469,   644,   472,
      59,   474,   475,   479,   480,   483,   654,   655,   484,   481,
     741,   485,   349,   350,   494,   273,   274,   275,   486,   667,
     499,   279,   280,   281,   282,   283,   284,   488,    59,   285,
     286,   501,   504,   507,   505,   506,   508,   518,   768,   529,
     545,   537,   546,   263,   264,   265,   547,   548,   552,   266,
     691,   692,   693,   603,   695,  -181,   696,  -182,   565,   535,
     537,  -183,   592,   618,   544,   669,   561,   619,   640,   643,
     641,   491,   645,   268,   269,   270,   653,   271,   272,   670,
     741,   -27,     1,     2,   671,     3,     4,  -127,     5,     6,
       7,     8,     9,    10,   658,    11,   677,   694,   678,    59,
      59,   742,   679,   567,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   687,   711,   285,   286,
     712,   718,   738,   604,   717,   732,   736,   743,   605,   744,
      13,   750,    59,   745,   761,   753,   756,   769,   759,   762,
     760,   770,   763,   764,   771,   772,   773,   774,   775,   668,
     776,   777,    59,   778,   784,    82,    83,    84,    85,    86,
      65,   132,   785,   786,   787,   788,   789,   765,   790,   791,
      63,   796,   797,   798,   666,   799,   622,   330,   354,    38,
      39,    40,   792,   793,   454,   794,   795,   352,   550,   674,
     344,   465,   767,   318,    87,   591,   380,    88,    89,    90,
      91,    92,    93,    94,    95,   804,    96,    97,   560,     0,
     527,    66,     0,    67,    68,    47,    48,    49,    50,    51,
      52,    53,    98,    99,   100,     0,     0,     0,   101,   102,
      82,    83,    84,    85,    86,    65,     0,   103,     0,     0,
     190,     0,     0,   105,     0,     0,   191,     0,     0,     0,
       0,     0,     0,     0,    38,    39,    40,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    87,
       0,     0,    88,    89,    90,    91,    92,    93,    94,    95,
       0,    96,    97,     0,     0,     0,    66,     0,    67,    68,
      47,    48,    49,    50,    51,    52,    53,    98,    99,   100,
       0,     0,     0,   101,   102,    82,    83,    84,    85,    86,
      65,     0,   103,     0,     0,   104,     0,     0,   105,     0,
       0,   609,     0,     0,     0,     0,     0,     0,     0,    38,
      39,    40,     0,   356,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    87,     0,     0,    88,    89,    90,
      91,    92,    93,    94,    95,     0,    96,    97,     0,     0,
       0,    66,     0,    67,    68,    47,    48,    49,    50,    51,
      52,    53,    98,    99,   100,     0,     0,     0,   101,   102,
     357,    82,    83,    84,    85,    86,    65,   103,     0,     0,
     104,     0,   358,   105,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    38,    39,    40,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      87,     0,     0,    88,    89,    90,    91,    92,    93,    94,
      95,     0,    96,    97,   181,     0,     0,    66,     0,    67,
      68,    47,    48,    49,    50,    51,    52,    53,    98,    99,
     100,     0,     0,     0,   101,   102,    82,    83,    84,    85,
      86,    65,     0,   103,     0,   430,   104,     0,     0,   105,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      38,    39,    40,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    87,     0,     0,    88,    89,
      90,    91,    92,    93,    94,    95,     0,    96,    97,     0,
       0,     0,    66,     0,    67,    68,    47,    48,    49,    50,
      51,    52,    53,    98,    99,   100,     0,     0,     0,   101,
     102,    82,    83,    84,    85,    86,    65,     0,   103,     0,
       0,   104,     0,     0,   105,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    38,    39,    40,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      87,     0,     0,    88,    89,    90,    91,    92,    93,    94,
      95,     0,    96,    97,     0,     0,     0,    66,     0,    67,
      68,    47,    48,    49,    50,    51,    52,    53,    98,    99,
     100,     0,     0,     0,   101,   102,    82,    83,    84,    85,
      86,    65,     0,   103,     0,     0,   104,     0,   438,   105,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      38,    39,    40,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    87,     0,     0,    88,    89,
      90,    91,    92,    93,    94,    95,     0,    96,    97,     0,
       0,     0,    66,     0,    67,    68,    47,    48,    49,    50,
      51,    52,    53,    98,    99,   100,     0,     0,     0,   101,
     102,    82,    83,    84,    85,    86,    65,     0,   103,     0,
       0,   104,     0,   583,   105,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    38,    39,    40,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      87,     0,     0,    88,    89,    90,    91,    92,    93,    94,
      95,     0,    96,    97,     0,     0,     0,    66,     0,    67,
      68,    47,    48,    49,    50,    51,    52,    53,    98,    99,
     100,     0,     0,     0,   101,   102,    82,    83,    84,    85,
      86,    65,   587,   103,     0,     0,   104,     0,     0,   105,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      38,    39,    40,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    87,     0,     0,    88,    89,
      90,    91,    92,    93,    94,    95,     0,    96,    97,     0,
       0,     0,    66,     0,    67,    68,    47,    48,    49,    50,
      51,    52,    53,    98,    99,   100,     0,     0,     0,   101,
     102,    82,    83,    84,    85,    86,   194,     0,   103,     0,
       0,   104,     0,     0,   105,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    38,    39,    40,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      87,     0,     0,    88,    89,    90,    91,    92,    93,    94,
      95,     0,    96,    97,     0,     0,     0,   195,     0,    67,
      68,    47,    48,    49,    50,    51,    52,    53,    98,    99,
     100,     0,     0,     0,   101,   102,    82,    83,    84,    85,
      86,   194,     0,   103,     0,     0,   104,     0,     0,   105,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      38,    39,    40,     1,     2,     0,     3,     4,  -127,     5,
       6,     7,     8,     9,    10,    87,    11,     0,    88,    89,
      90,    91,    92,    93,    94,    95,     0,    96,    97,     0,
       0,     0,   195,     0,    67,    68,    47,    48,    49,    50,
      51,    52,    53,    98,    99,   100,     0,     0,     0,   101,
     102,    13,     0,     0,   198,   199,     0,   200,   103,     0,
       0,   104,     0,     0,   105,   201,   202,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,   257,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   198,   199,     0,   200,     0,     0,     0,     0,     0,
       0,   258,   201,   202,   203,   204,   205,   206,   207,   208,
     209,   210,   211,   212,   213,   214,   215,   216,   217,   218,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,   243,   244,   245,   246,   247,   248,
     249,   250,   251,   252,   253,   254,   255,   256,   257,   263,
     264,   265,   533,     1,     2,   266,     3,     4,  -127,     5,
       6,     7,     8,     9,    10,     0,    11,     0,   559,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   268,
     269,   270,     0,   271,   272,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   263,   264,   265,     0,
      12,    13,   266,     0,     0,     0,     0,     0,     0,     0,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   282,
     283,   284,     0,     0,   285,   286,   268,   269,   270,     0,
     271,   272,     0,     0,   534,     0,     0,     0,     0,     0,
       0,     0,     0,   263,   264,   265,     0,     0,     0,   266,
       0,     0,     0,     0,     0,     0,     0,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,   283,   284,     0,
       0,   285,   286,   268,   269,   270,   698,   271,   272,     0,
       0,   699,     0,     0,     0,     0,     0,     0,     0,     0,
     263,   264,   265,     0,     0,     0,   266,     0,     0,     0,
       0,     0,     0,     0,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,     0,     0,   285,   286,
     268,   269,   270,     0,   271,   272,     0,     0,   608,     0,
       0,     0,     0,     0,     0,     0,     0,   263,   264,   265,
       0,     0,     0,   266,     0,     0,     0,     0,     0,     0,
       0,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,     0,     0,   285,   286,   268,   269,   270,
       0,   271,   272,     0,     0,   657,     0,     0,     0,     0,
       0,     0,     0,     0,   263,   264,   265,     0,     0,     0,
     266,     0,     0,     0,     0,     0,     0,     0,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
       0,     0,   285,   286,   268,   269,   270,     0,   271,   272,
       0,     0,   700,     0,     0,     0,     0,     0,     0,     0,
       0,   263,   264,   265,     0,     0,     0,   266,     0,     0,
       0,     0,     0,     0,     0,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,     0,     0,   285,
     286,   268,   269,   270,     0,   271,   272,     0,     0,   766,
       0,     0,     0,     0,     0,     0,     0,     0,   263,   264,
     265,     0,     0,     0,   266,     0,     0,     0,     0,     0,
       0,     0,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,     0,     0,   285,   286,   268,   269,
     270,     0,   271,   272,     0,     0,   781,     0,     0,     0,
       0,     0,     0,     0,     0,   263,   264,   265,     0,     0,
       0,   266,     0,     0,     0,     0,     0,     0,     0,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,     0,     0,   285,   286,   268,   269,   270,     0,   271,
     272,     0,     0,   801,     0,     0,     0,     0,     0,     0,
       0,     0,   263,   264,   265,     0,     0,     0,   266,     0,
       0,     0,     0,     0,     0,     0,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,     0,     0,
     285,   286,   268,   269,   270,     0,   271,   272,     0,     0,
     803,     0,     0,     0,     0,     0,     0,     0,     0,   263,
     264,   265,     0,     0,     0,   266,     0,     0,     0,     0,
       0,     0,     0,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,     0,     0,   285,   286,   268,
     269,   270,     0,   271,   272,     1,     2,   805,     3,     4,
    -127,     5,     6,     7,     8,     9,    10,     0,    11,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   282,
     283,   284,   297,     0,   285,   286,   361,     0,    37,     0,
       0,     0,   646,    13,     0,     0,     0,     1,     2,     0,
       3,     4,  -127,     5,     6,     7,     8,   362,   363,    40,
      11,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    41,
      42,    43,    44,     0,     0,     0,     0,     0,     0,     0,
       0,    45,    46,    47,    48,   364,    50,    51,    52,    53,
       0,     0,     0,     0,     0,    36,     0,    37,     0,     0,
       0,     0,     0,     0,   365,     0,     1,     2,    54,     3,
       4,  -127,     5,     6,     7,     8,   362,   363,    40,    11,
       0,     0,     0,    36,     0,    37,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    41,    42,
      43,    44,     0,     0,    38,    39,    40,     0,     0,     0,
      45,    46,    47,    48,   364,    50,    51,    52,    53,     0,
       0,     0,     0,     0,     0,     0,    41,    42,    43,    44,
      36,     0,    76,     0,     0,     0,     0,    54,    45,    46,
      47,    48,    49,    50,    51,    52,    53,     0,     0,     0,
       0,    38,    39,    40,     0,     0,     0,     0,   370,     0,
      37,     0,     0,     0,     0,    54,     0,     0,     0,     0,
       0,     0,     0,    41,    42,    43,    44,     0,     0,    38,
      39,    40,     0,     0,     0,    67,    68,    47,    48,    49,
      50,    51,    52,    53,   263,   264,   265,     0,     0,     0,
     266,    41,    42,    43,    44,     0,     0,     0,     0,     0,
       0,     0,    77,    45,    46,    47,    48,    49,    50,    51,
      52,    53,     0,     0,   268,   269,   270,     0,   271,   272,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      54,     0,   263,   264,   265,     0,     0,     0,   266,     0,
       0,     0,     0,     0,     0,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,     0,     0,   285,
     286,     0,   268,   269,   270,   329,   271,   272,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     263,   264,   265,     0,     0,     0,   266,     0,     0,     0,
       0,     0,     0,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,     0,     0,   285,   286,     0,
     268,   269,   270,   343,   271,   272,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   263,   264,
     265,     0,     0,     0,   266,     0,     0,     0,     0,     0,
       0,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,     0,     0,   285,   286,     0,   268,   269,
     270,   391,   271,   272,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   263,   264,   265,     0,
       0,     0,   266,     0,     0,     0,     0,     0,     0,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,     0,     0,   285,   286,     0,   268,   269,   270,   585,
     271,   272,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   263,   264,   265,     0,     0,     0,
     266,     0,     0,     0,     0,     0,     0,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,   283,   284,     0,
       0,   285,   286,     0,   268,   269,   270,   733,   271,   272,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   263,   264,   265,     0,     0,     0,   266,     0,
       0,     0,     0,     0,     0,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,     0,     0,   285,
     286,     0,   268,   269,   270,   734,   271,   272,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     263,   264,   265,     0,     0,     0,   266,     0,     0,     0,
       0,     0,     0,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,     0,     0,   285,   286,     0,
     268,   269,   270,   735,   271,   272,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   263,   264,
     265,     0,     0,     0,   266,     0,     0,     0,     0,     0,
       0,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,     0,     0,   285,   286,     0,   268,   269,
     270,   737,   271,   272,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   263,   264,   265,     0,
       0,     0,   266,     0,     0,     0,     0,     0,     0,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,     0,     0,   285,   286,     0,   268,   269,   270,   800,
     271,   272,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   263,   264,   265,     0,     0,     0,   266,
       0,     0,     0,     0,     0,     0,     0,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,   283,   284,     0,
       0,   285,   286,   268,   269,   270,   614,   271,   272,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     263,   264,   265,     0,     0,     0,   266,     0,     0,     0,
       0,     0,     0,     0,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,     0,     0,   285,   286,
     268,   269,   270,   615,   271,   272,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   263,   264,   265,
       0,     0,     0,   266,     0,     0,     0,     0,     0,     0,
       0,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,     0,     0,   285,   286,   268,   269,   270,
     779,   271,   272,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   263,   264,   265,     0,     0,     0,
     266,     0,     0,     0,     0,     0,     0,     0,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
       0,     0,   285,   286,   268,   269,   270,   780,   271,   272,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   263,   264,   265,     0,     0,     0,   266,     0,     0,
       0,     0,     0,     0,     0,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,     0,     0,   285,
     286,   268,   269,   270,   802,   271,   272,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   263,   264,   265,     0,     0,     0,   266,     0,
       0,     0,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,     0,     0,   285,   286,     0,   650,
     553,   554,   268,   269,   270,     0,   271,   272,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   263,
     264,   265,     0,     0,     0,   266,     0,     0,     0,     0,
       0,     0,     0,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,     0,   267,   285,   286,   268,
     269,   270,     0,   271,   272,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   263,   264,   265,     0,
       0,     0,   266,     0,     0,     0,     0,     0,     0,     0,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   282,
     283,   284,     0,   309,   285,   286,   268,   269,   270,     0,
     271,   272,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   263,   264,   265,     0,     0,     0,   266,
       0,     0,     0,     0,     0,     0,     0,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,   283,   284,     0,
       0,   285,   286,   268,   269,   270,     0,   271,   272,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     181,   263,   264,   265,     0,     0,     0,   266,     0,     0,
       0,     0,     0,     0,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,     0,     0,   285,   286,
       0,   268,   269,   270,     0,   271,   272,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   509,
     263,   264,   265,     0,     0,     0,   266,     0,     0,     0,
       0,     0,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,     0,     0,   285,   286,     0,     0,
     268,   269,   270,     0,   271,   272,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   263,   264,   265,
     601,     0,     0,   266,     0,     0,     0,   647,     0,     0,
       0,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,   283,   284,     0,     0,   285,   286,   268,   269,   270,
       0,   271,   272,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   263,   264,   265,     0,     0,     0,
     266,     0,     0,     0,   648,     0,     0,     0,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
       0,     0,   285,   286,   268,   269,   270,     0,   271,   272,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   263,   264,   265,     0,     0,     0,   266,     0,     0,
       0,   649,     0,     0,     0,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,   283,   284,     0,     0,   285,
     286,   268,   269,   270,     0,   271,   272,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   263,   264,
     265,     0,     0,     0,   266,     0,     0,     0,   651,     0,
       0,     0,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,     0,     0,   285,   286,   268,   269,
     270,     0,   271,   272,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   263,   264,   265,     0,     0,
       0,   266,     0,     0,     0,     0,     0,     0,     0,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     284,     0,     0,   285,   286,   268,   269,   270,     0,   271,
     272,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   263,   264,   265,   652,     0,     0,   266,     0,
       0,     0,     0,     0,     0,     0,   273,   274,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,     0,     0,
     285,   286,   268,   269,   270,     0,   271,   272,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   263,
     264,   265,     0,     0,     0,   266,     0,     0,     0,     0,
       0,     0,     0,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,     0,     0,   285,   286,   268,
       0,   270,     0,   271,   272,     0,     0,     0,     0,     0,
       0,     0,   263,   264,   265,     0,     0,     0,   266,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     273,   274,   275,   276,   277,   278,   279,   280,   281,   282,
     283,   284,   268,     0,   285,   286,   271,   272,     0,     0,
       0,   263,   264,   265,     0,     0,     0,   266,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   271,   272,   285,   286,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    36,     0,     0,
       0,     0,   273,   274,   275,   276,   277,   278,   279,   280,
     281,   282,   283,   284,     0,     0,   285,   286,    38,    39,
      40,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      41,    42,    43,    44,     0,     0,     0,     0,     0,     0,
       0,     0,    67,    68,    47,    48,    49,    50,    51,    52,
      53,   198,   199,     0,   200,     0,     0,     0,     0,   541,
       0,     0,   201,   202,   203,   204,   205,   206,   207,   208,
     209,   210,   211,   212,   213,   214,   215,   216,   217,   218,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   235,   236,   237,   238,
     239,   240,   241,   242,   243,   244,   245,   246,   247,   248,
     249,   250,   251,   252,   253,   254,   255,   256,   257,   334,
       0,   335,     0,     0,     0,     0,     0,     0,     0,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
       0,     0,   224,   225,   226,   227,   228,   229,   230,     0,
       0,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   250,   251,
     252,   253,   254,   255,   256,   257,   337,     0,   338,     0,
       0,     0,     0,     0,     0,     0,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,     0,     0,   224,
     225,   226,   227,   228,   229,   230,     0,     0,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,   253,   254,
     255,   256,   257,   427,     0,   428,     0,     0,     0,     0,
       0,     0,     0,   201,   202,   203,   204,   205,   206,   207,
     208,   209,   210,   211,   212,   213,   214,   215,   216,   217,
     218,   219,   220,   221,     0,     0,   224,   225,   226,   227,
     228,   229,   230,     0,     0,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,   251,   252,   253,   254,   255,   256,   257,
     462,     0,   463,     0,     0,     0,     0,     0,     0,     0,
     201,   202,   203,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,     0,     0,   224,   225,   226,   227,   228,   229,   230,
       0,     0,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   562,     0,   563,
       0,     0,     0,     0,     0,     0,     0,   201,   202,   203,
     204,   205,   206,   207,   208,   209,   210,   211,   212,   213,
     214,   215,   216,   217,   218,   219,   220,   221,     0,     0,
     224,   225,   226,   227,   228,   229,   230,     0,     0,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   250,   251,   252,   253,
     254,   255,   256,   257,   631,     0,   632,     0,     0,     0,
       0,     0,     0,     0,   201,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,     0,     0,   224,   225,   226,
     227,   228,   229,   230,     0,     0,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,   251,   252,   253,   254,   255,   256,
     257,   672,     0,   673,     0,     0,     0,     0,     0,     0,
       0,   201,   202,   203,   204,   205,   206,   207,   208,   209,
     210,   211,   212,   213,   214,   215,   216,   217,   218,   219,
     220,   221,     0,     0,   224,   225,   226,   227,   228,   229,
     230,     0,     0,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255,   256,   257,    36,     0,
     144,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    38,
      39,    40,     0,     0,    36,     0,   288,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    41,    42,    43,    44,    38,    39,    40,     0,     0,
      36,   145,   407,    67,    68,    47,    48,    49,    50,    51,
      52,    53,     0,     0,     0,     0,     0,    41,    42,    43,
      44,    38,    39,    40,     0,     0,    36,     0,   456,    67,
      68,    47,    48,    49,    50,    51,    52,    53,     0,     0,
       0,     0,     0,    41,    42,    43,    44,    38,    39,    40,
       0,     0,    36,     0,     0,    67,    68,    47,    48,    49,
      50,    51,    52,    53,     0,     0,     0,     0,     0,    41,
      42,    43,    44,    38,    39,    40,     0,     0,     0,     0,
       0,    67,    68,    47,    48,    49,    50,    51,    52,    53,
       0,     0,     0,     0,     0,    41,    42,    43,    44,     0,
       0,     0,     0,     0,     0,     0,     0,    67,    68,    47,
      48,    49,    50,    51,    52,    53
};

static const yytype_int16 yycheck[] =
{
       7,   190,   387,   141,   142,   143,   185,   186,   187,   106,
     327,   183,     4,   105,   152,   477,    62,     6,     2,   130,
       6,    12,     6,   154,     6,    63,     8,     6,    35,     8,
       6,     3,     8,   516,    14,    67,     6,    73,    19,     2,
      12,     4,     5,     6,    32,    32,    74,    78,    19,    77,
     166,    19,    19,    43,    31,    87,    63,    93,    89,    88,
       6,   172,    69,    84,   175,   103,    84,     6,   160,     8,
      77,    31,    93,    20,     2,    93,    18,     5,     6,    21,
      87,    23,    24,    73,    26,    27,   569,   614,   615,   135,
      97,    54,   138,    73,   101,   102,   103,   104,    84,    87,
      87,    78,    91,    93,    84,    77,    87,    74,    78,   155,
      77,    93,   574,   292,    93,   153,    87,    93,    78,    87,
      67,    92,    89,    53,    92,    66,    54,   134,    61,    62,
     447,    84,   321,   660,   125,    88,    53,    77,   455,   666,
     103,   104,    53,     0,   627,    85,   153,    74,    90,   133,
      77,   158,   159,    61,    62,    88,    22,   141,   142,   143,
     123,   478,    92,    90,   148,   157,    78,   151,   152,    86,
     133,   287,   303,   656,   181,    86,   638,   308,    16,    60,
      88,    93,    78,   190,    77,   148,   297,   133,   172,    78,
     174,   175,    85,   285,   157,   123,    78,   328,   681,   315,
     683,   164,   685,   686,    93,   133,    87,    89,   380,   172,
      32,   174,   175,   176,   177,   178,   179,   180,   566,    74,
     148,   606,    77,   315,    84,    85,   172,   190,   174,   175,
     713,   714,   715,   716,    18,    90,   164,    21,    87,    23,
      24,    19,    26,    27,   172,    94,   174,   175,   176,   177,
     178,   179,   180,    73,    78,   572,   263,   264,   265,   266,
      88,   268,   269,   270,   271,    89,   484,   485,   275,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   262,   286,
     326,    88,   471,   631,   632,   633,   634,    74,   614,   615,
      77,    81,   614,   615,    84,    85,    74,   304,   305,    77,
     307,   614,   615,    90,   267,   402,    90,   486,   400,   614,
     615,    89,    74,   614,   615,    77,    74,    87,    87,    77,
     458,   459,    43,    92,   672,   673,   674,   675,    90,   518,
      87,     6,    90,     8,   660,    92,   474,   475,   660,   267,
     666,    73,    93,    87,   666,   476,   309,   660,    92,   356,
      87,    93,    73,   666,    43,   660,    84,    74,   321,   660,
      77,   666,    87,   479,   480,   666,   482,   483,    88,    90,
      91,    91,    93,    90,   381,   467,    89,   469,    88,    88,
      93,   309,   389,    88,    73,   392,   393,   394,   395,   396,
     397,   398,   399,   321,    56,    84,    58,    59,     6,    88,
       8,    90,    91,     6,    93,     8,    88,    18,   415,   416,
      21,    42,    23,    24,    45,    26,    27,    88,    88,   535,
     536,   537,    88,   430,    27,    28,    29,   434,   435,    76,
      77,    78,    79,    80,    81,    88,    93,    84,    85,   446,
      90,    91,    16,    93,    18,   452,    49,    50,    51,    52,
      90,    91,    56,    93,    58,    59,    59,    33,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    87,    87,    87,
      90,    90,    90,    34,   458,   459,   568,   440,    87,    93,
      87,    90,   489,    90,   491,    88,    74,    88,   495,    77,
     474,   475,   484,   485,     6,   502,     8,   504,   505,   506,
     507,   508,   509,   510,    77,     9,    10,    11,   471,    60,
      60,    15,   440,    78,    79,    80,    81,     6,    93,    84,
      85,   484,   485,    93,   640,   641,   533,    87,    87,    74,
      90,    90,    77,    20,   626,    39,    40,    41,    93,    43,
      44,   548,   658,   471,   528,   552,     6,     6,     8,     8,
      93,    93,    93,    93,    93,   518,   653,    93,    20,    43,
      43,     6,    29,    68,     5,    87,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    43,    44,
      84,    85,    35,   546,    91,    88,    88,    93,    92,    75,
     518,    88,    88,    93,    93,    93,   603,   604,    21,    84,
     697,    21,    16,    18,    92,    70,    71,    72,    88,   616,
      84,    76,    77,    78,    79,    80,    81,    90,   546,    84,
      85,    92,    43,    73,    43,    43,    43,    88,   744,    45,
      67,    89,    36,     9,    10,    11,     6,    19,    87,    15,
     647,   648,   649,    19,   651,    89,   743,    89,    75,    89,
      89,    89,     6,    36,   617,   618,    90,     5,    93,     6,
      93,    93,    90,    39,    40,    41,    90,    43,    44,    36,
     767,    16,    17,    18,    69,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    93,    30,    12,   650,     3,   617,
     618,   698,     3,    78,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    94,   670,    84,    85,
       5,     3,   696,    89,    94,    77,    43,    90,    94,    93,
      65,    77,   650,    69,    75,    94,    94,     5,    94,   736,
      94,    94,   739,   740,    94,    94,    94,    75,    77,   617,
      77,    77,   670,    77,    77,     3,     4,     5,     6,     7,
       8,    17,    77,    77,    77,    75,    75,   741,    75,    75,
       3,    75,    75,    75,   615,    75,   552,   157,   169,    27,
      28,    29,   779,   780,   307,   782,   783,   168,   446,   626,
     164,   315,   743,   148,    42,   497,   182,    45,    46,    47,
      48,    49,    50,    51,    52,   802,    54,    55,   467,    -1,
     400,    59,    -1,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    76,    77,
       3,     4,     5,     6,     7,     8,    -1,    85,    -1,    -1,
      88,    -1,    -1,    91,    -1,    -1,    94,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    27,    28,    29,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    42,
      -1,    -1,    45,    46,    47,    48,    49,    50,    51,    52,
      -1,    54,    55,    -1,    -1,    -1,    59,    -1,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    -1,    -1,    76,    77,     3,     4,     5,     6,     7,
       8,    -1,    85,    -1,    -1,    88,    -1,    -1,    91,    -1,
      -1,    94,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    27,
      28,    29,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    42,    -1,    -1,    45,    46,    47,
      48,    49,    50,    51,    52,    -1,    54,    55,    -1,    -1,
      -1,    59,    -1,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    76,    77,
      78,     3,     4,     5,     6,     7,     8,    85,    -1,    -1,
      88,    -1,    90,    91,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    27,    28,    29,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      42,    -1,    -1,    45,    46,    47,    48,    49,    50,    51,
      52,    -1,    54,    55,    56,    -1,    -1,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    -1,    -1,    -1,    76,    77,     3,     4,     5,     6,
       7,     8,    -1,    85,    -1,    12,    88,    -1,    -1,    91,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      27,    28,    29,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    42,    -1,    -1,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    54,    55,    -1,
      -1,    -1,    59,    -1,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    -1,    -1,    -1,    76,
      77,     3,     4,     5,     6,     7,     8,    -1,    85,    -1,
      -1,    88,    -1,    -1,    91,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    27,    28,    29,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      42,    -1,    -1,    45,    46,    47,    48,    49,    50,    51,
      52,    -1,    54,    55,    -1,    -1,    -1,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    -1,    -1,    -1,    76,    77,     3,     4,     5,     6,
       7,     8,    -1,    85,    -1,    -1,    88,    -1,    90,    91,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      27,    28,    29,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    42,    -1,    -1,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    54,    55,    -1,
      -1,    -1,    59,    -1,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    -1,    -1,    -1,    76,
      77,     3,     4,     5,     6,     7,     8,    -1,    85,    -1,
      -1,    88,    -1,    90,    91,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    27,    28,    29,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      42,    -1,    -1,    45,    46,    47,    48,    49,    50,    51,
      52,    -1,    54,    55,    -1,    -1,    -1,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    -1,    -1,    -1,    76,    77,     3,     4,     5,     6,
       7,     8,    84,    85,    -1,    -1,    88,    -1,    -1,    91,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      27,    28,    29,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    42,    -1,    -1,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    54,    55,    -1,
      -1,    -1,    59,    -1,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    -1,    -1,    -1,    76,
      77,     3,     4,     5,     6,     7,     8,    -1,    85,    -1,
      -1,    88,    -1,    -1,    91,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    27,    28,    29,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      42,    -1,    -1,    45,    46,    47,    48,    49,    50,    51,
      52,    -1,    54,    55,    -1,    -1,    -1,    59,    -1,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    -1,    -1,    -1,    76,    77,     3,     4,     5,     6,
       7,     8,    -1,    85,    -1,    -1,    88,    -1,    -1,    91,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      27,    28,    29,    17,    18,    -1,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    42,    30,    -1,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    54,    55,    -1,
      -1,    -1,    59,    -1,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    -1,    -1,    -1,    76,
      77,    65,    -1,    -1,     5,     6,    -1,     8,    85,    -1,
      -1,    88,    -1,    -1,    91,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     5,     6,    -1,     8,    -1,    -1,    -1,    -1,    -1,
      -1,    92,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,     9,
      10,    11,    12,    17,    18,    15,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    -1,    30,    -1,    92,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      40,    41,    -1,    43,    44,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     9,    10,    11,    -1,
      64,    65,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    -1,    -1,    84,    85,    39,    40,    41,    -1,
      43,    44,    -1,    -1,    94,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     9,    10,    11,    -1,    -1,    -1,    15,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    -1,
      -1,    84,    85,    39,    40,    41,    89,    43,    44,    -1,
      -1,    94,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       9,    10,    11,    -1,    -1,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    -1,    -1,    84,    85,
      39,    40,    41,    -1,    43,    44,    -1,    -1,    94,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     9,    10,    11,
      -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    -1,    -1,    84,    85,    39,    40,    41,
      -1,    43,    44,    -1,    -1,    94,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     9,    10,    11,    -1,    -1,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      -1,    -1,    84,    85,    39,    40,    41,    -1,    43,    44,
      -1,    -1,    94,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     9,    10,    11,    -1,    -1,    -1,    15,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    -1,    -1,    84,
      85,    39,    40,    41,    -1,    43,    44,    -1,    -1,    94,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     9,    10,
      11,    -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    -1,    -1,    84,    85,    39,    40,
      41,    -1,    43,    44,    -1,    -1,    94,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     9,    10,    11,    -1,    -1,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    -1,    -1,    84,    85,    39,    40,    41,    -1,    43,
      44,    -1,    -1,    94,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     9,    10,    11,    -1,    -1,    -1,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    -1,    -1,
      84,    85,    39,    40,    41,    -1,    43,    44,    -1,    -1,
      94,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     9,
      10,    11,    -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    -1,    -1,    84,    85,    39,
      40,    41,    -1,    43,    44,    17,    18,    94,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    -1,    30,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    54,    -1,    84,    85,     6,    -1,     8,    -1,
      -1,    -1,    92,    65,    -1,    -1,    -1,    17,    18,    -1,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,
      50,    51,    52,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      -1,    -1,    -1,    -1,    -1,     6,    -1,     8,    -1,    -1,
      -1,    -1,    -1,    -1,    84,    -1,    17,    18,    88,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      -1,    -1,    -1,     6,    -1,     8,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    -1,    -1,    27,    28,    29,    -1,    -1,    -1,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
       6,    -1,     8,    -1,    -1,    -1,    -1,    88,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    -1,    -1,    -1,
      -1,    27,    28,    29,    -1,    -1,    -1,    -1,     6,    -1,
       8,    -1,    -1,    -1,    -1,    88,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    49,    50,    51,    52,    -1,    -1,    27,
      28,    29,    -1,    -1,    -1,    61,    62,    63,    64,    65,
      66,    67,    68,    69,     9,    10,    11,    -1,    -1,    -1,
      15,    49,    50,    51,    52,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    88,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    -1,    -1,    39,    40,    41,    -1,    43,    44,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      88,    -1,     9,    10,    11,    -1,    -1,    -1,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    -1,    -1,    84,
      85,    -1,    39,    40,    41,    90,    43,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       9,    10,    11,    -1,    -1,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    -1,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    -1,    -1,    84,    85,    -1,
      39,    40,    41,    90,    43,    44,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     9,    10,
      11,    -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    -1,    -1,    84,    85,    -1,    39,    40,
      41,    90,    43,    44,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     9,    10,    11,    -1,
      -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    -1,    -1,    84,    85,    -1,    39,    40,    41,    90,
      43,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     9,    10,    11,    -1,    -1,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    -1,
      -1,    84,    85,    -1,    39,    40,    41,    90,    43,    44,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     9,    10,    11,    -1,    -1,    -1,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    -1,    -1,    84,
      85,    -1,    39,    40,    41,    90,    43,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       9,    10,    11,    -1,    -1,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    -1,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    -1,    -1,    84,    85,    -1,
      39,    40,    41,    90,    43,    44,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     9,    10,
      11,    -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    -1,    -1,    84,    85,    -1,    39,    40,
      41,    90,    43,    44,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     9,    10,    11,    -1,
      -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    -1,    -1,    84,    85,    -1,    39,    40,    41,    90,
      43,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     9,    10,    11,    -1,    -1,    -1,    15,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    -1,
      -1,    84,    85,    39,    40,    41,    89,    43,    44,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       9,    10,    11,    -1,    -1,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    -1,    -1,    84,    85,
      39,    40,    41,    89,    43,    44,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     9,    10,    11,
      -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    -1,    -1,    84,    85,    39,    40,    41,
      89,    43,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     9,    10,    11,    -1,    -1,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      -1,    -1,    84,    85,    39,    40,    41,    89,    43,    44,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     9,    10,    11,    -1,    -1,    -1,    15,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    -1,    -1,    84,
      85,    39,    40,    41,    89,    43,    44,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     9,    10,    11,    -1,    -1,    -1,    15,    -1,
      -1,    -1,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    -1,    -1,    84,    85,    -1,    87,
      37,    38,    39,    40,    41,    -1,    43,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     9,
      10,    11,    -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    -1,    36,    84,    85,    39,
      40,    41,    -1,    43,    44,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     9,    10,    11,    -1,
      -1,    -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    -1,    36,    84,    85,    39,    40,    41,    -1,
      43,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     9,    10,    11,    -1,    -1,    -1,    15,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    -1,
      -1,    84,    85,    39,    40,    41,    -1,    43,    44,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      56,     9,    10,    11,    -1,    -1,    -1,    15,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    -1,    -1,    84,    85,
      -1,    39,    40,    41,    -1,    43,    44,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,
       9,    10,    11,    -1,    -1,    -1,    15,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    -1,    -1,    84,    85,    -1,    -1,
      39,    40,    41,    -1,    43,    44,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     9,    10,    11,
      59,    -1,    -1,    15,    -1,    -1,    -1,    19,    -1,    -1,
      -1,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    -1,    -1,    84,    85,    39,    40,    41,
      -1,    43,    44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,     9,    10,    11,    -1,    -1,    -1,
      15,    -1,    -1,    -1,    19,    -1,    -1,    -1,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      -1,    -1,    84,    85,    39,    40,    41,    -1,    43,    44,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     9,    10,    11,    -1,    -1,    -1,    15,    -1,    -1,
      -1,    19,    -1,    -1,    -1,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    -1,    -1,    84,
      85,    39,    40,    41,    -1,    43,    44,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     9,    10,
      11,    -1,    -1,    -1,    15,    -1,    -1,    -1,    19,    -1,
      -1,    -1,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    -1,    -1,    84,    85,    39,    40,
      41,    -1,    43,    44,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     9,    10,    11,    -1,    -1,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    -1,    -1,    84,    85,    39,    40,    41,    -1,    43,
      44,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     9,    10,    11,    59,    -1,    -1,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    -1,    -1,
      84,    85,    39,    40,    41,    -1,    43,    44,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     9,
      10,    11,    -1,    -1,    -1,    15,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    -1,    -1,    84,    85,    39,
      -1,    41,    -1,    43,    44,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     9,    10,    11,    -1,    -1,    -1,    15,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    39,    -1,    84,    85,    43,    44,    -1,    -1,
      -1,     9,    10,    11,    -1,    -1,    -1,    15,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    43,    44,    84,    85,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     6,    -1,    -1,
      -1,    -1,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    -1,    -1,    84,    85,    27,    28,
      29,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      49,    50,    51,    52,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    61,    62,    63,    64,    65,    66,    67,    68,
      69,     5,     6,    -1,     8,    -1,    -1,    -1,    -1,    78,
      -1,    -1,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,     6,
      -1,     8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      -1,    -1,    39,    40,    41,    42,    43,    44,    45,    -1,
      -1,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,     6,    -1,     8,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    -1,    -1,    39,
      40,    41,    42,    43,    44,    45,    -1,    -1,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,     6,    -1,     8,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    -1,    -1,    39,    40,    41,    42,
      43,    44,    45,    -1,    -1,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
       6,    -1,     8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    -1,    -1,    39,    40,    41,    42,    43,    44,    45,
      -1,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,     6,    -1,     8,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    -1,    -1,
      39,    40,    41,    42,    43,    44,    45,    -1,    -1,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,     6,    -1,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    -1,    -1,    39,    40,    41,
      42,    43,    44,    45,    -1,    -1,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,     6,    -1,     8,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    -1,    -1,    39,    40,    41,    42,    43,    44,
      45,    -1,    -1,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,     6,    -1,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    27,
      28,    29,    -1,    -1,     6,    -1,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    27,    28,    29,    -1,    -1,
       6,    59,     8,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,
      52,    27,    28,    29,    -1,    -1,     6,    -1,     8,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    -1,    -1,
      -1,    -1,    -1,    49,    50,    51,    52,    27,    28,    29,
      -1,    -1,     6,    -1,    -1,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    -1,    -1,    -1,    -1,    -1,    49,
      50,    51,    52,    27,    28,    29,    -1,    -1,    -1,    -1,
      -1,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,    62,    63,
      64,    65,    66,    67,    68,    69
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    17,    18,    20,    21,    23,    24,    25,    26,    27,
      28,    30,    64,    65,    96,    97,    98,    99,   100,   101,
     103,   104,   105,   106,   107,   108,   113,   127,   128,   131,
     132,   135,   138,    31,    78,   115,     6,     8,    27,    28,
      29,    49,    50,    51,    52,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    88,   139,   140,   141,   142,   171,
     172,    31,    78,   115,     6,     8,    59,    61,    62,    88,
     124,   125,   126,   170,   171,   172,     8,    88,   136,   137,
     172,   139,     3,     4,     5,     6,     7,    42,    45,    46,
      47,    48,    49,    50,    51,    52,    54,    55,    70,    71,
      72,    76,    77,    85,    88,    91,   142,   151,   152,   153,
     154,   155,   156,   158,   159,   160,   162,   163,   166,   169,
     170,   172,   174,    88,     6,    91,   112,    97,    66,     0,
      53,    86,   100,    16,    22,    78,    32,   116,   122,   123,
     151,    73,    88,    88,     8,    59,   144,   172,    87,    74,
      77,   143,    73,    87,   116,   122,   151,    87,    14,    73,
      84,    93,    84,   151,    87,    84,    93,    60,   129,   130,
      84,    88,    91,   151,    88,    91,    88,    88,    88,    88,
      88,    56,   151,   167,   168,    88,    88,    88,   151,   151,
      88,    94,   122,   172,     8,    59,   151,   172,     5,     6,
       8,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    92,   173,
     176,   177,   143,     9,    10,    11,    15,    36,    39,    40,
      41,    43,    44,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    84,    85,    93,     8,   172,
      97,    84,    88,   109,    86,    20,    67,    54,    98,   139,
     133,   134,   151,   116,    33,    34,   117,    87,   116,    36,
      61,    62,   140,   140,   140,    93,   146,   149,   141,    77,
      77,    85,   142,    61,    62,   140,   122,   117,   116,    90,
     126,   172,   151,   151,     6,     8,   173,     6,     8,   147,
     173,     6,     8,    90,   137,   172,     6,     8,   147,    16,
      18,    60,   130,    60,   129,     6,    31,    78,    90,   151,
     157,     6,    27,    28,    65,    84,    98,   139,   164,   165,
       6,   139,    98,   139,   172,   172,   172,   172,   172,   151,
     167,    58,    59,   168,   157,   157,   157,   144,    94,    43,
      73,    90,    93,    93,    93,    93,    93,    93,    93,    93,
      87,    92,   142,   151,   151,   151,   151,     8,   172,   151,
     151,   151,   151,    42,    45,    20,    20,   151,   151,   151,
     151,   151,   151,   151,   151,   151,   151,     6,     8,   173,
      12,   151,   147,   148,    43,    43,    92,     6,    90,   157,
      29,    68,     5,    98,    67,   102,    87,   117,   120,   121,
     151,   151,    35,   118,   123,   117,     8,   172,    88,    88,
      90,    90,     6,     8,   147,   148,   173,    91,   175,    93,
      77,    85,    75,   144,    88,    88,   116,   118,   117,    93,
      93,    84,    93,    93,    21,    21,    88,   151,    90,    87,
      90,    93,     6,    78,    92,    19,    92,    87,    92,    84,
      90,    92,    19,    92,    43,    43,    43,    73,    43,    57,
      58,    59,   151,    90,    90,    90,   146,   151,    88,   151,
     151,   151,   151,   151,   151,   151,   151,   177,   143,    45,
     151,   151,   151,    12,    94,    89,    93,    89,   151,   151,
      90,    78,   110,   111,   172,    67,    36,     6,    19,   119,
     134,   118,    87,    37,    38,   151,   118,   140,   140,    92,
     176,    90,     6,     8,   173,    75,   144,    78,    93,   145,
     140,   140,   117,   119,   118,   147,   147,     6,     8,   147,
     147,   124,   124,    90,   157,    90,   151,    84,   151,   151,
       6,   165,     6,   151,   151,   151,   151,   151,   151,   151,
     151,    59,   175,    19,    89,    94,   144,   142,    94,    94,
     151,   147,   147,   147,    89,    89,    19,    87,    36,     5,
     172,   151,   121,    90,    90,    92,    93,   145,     3,    12,
      77,     6,     8,   150,   173,   175,    90,    90,   118,   119,
      93,    93,    90,     6,    92,    90,    92,    19,    19,    19,
      87,    19,    59,    90,   151,   151,   146,    94,    93,   107,
     114,   127,   128,   131,   132,   135,   114,   151,   111,   172,
      36,    69,     6,     8,   150,   173,   175,    12,     3,     3,
      89,   145,    89,   145,    89,   145,   145,    94,   119,   147,
     147,   151,   151,   151,   172,   151,   143,   161,    89,    94,
      94,   175,   147,    90,   107,   127,   128,   131,   132,   135,
      90,   172,     5,   145,   145,   145,   145,    94,     3,     6,
       8,    93,   175,     6,     8,    93,   175,     6,     8,    93,
     175,   175,    77,    90,    90,    90,    43,    90,   142,    19,
      89,   143,   151,    90,    93,    69,   175,   175,   175,   175,
      77,     6,     8,    94,     6,     8,    94,     6,     8,    94,
      94,    75,   151,   151,   151,   142,    94,   161,   147,     5,
      94,    94,    94,    94,    75,    77,    77,    77,    77,    89,
      89,    94,    19,    89,    77,    77,    77,    77,    75,    75,
      75,    75,   151,   151,   151,   151,    75,    75,    75,    75,
      90,    94,    89,    94,   151,    94
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    95,    96,    96,    96,    96,    97,    97,    97,    98,
      99,    99,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   101,   102,   102,   103,   103,   104,
     104,   104,   105,   105,   105,   106,   106,   107,   107,   108,
     108,   108,   108,   108,   109,   109,   109,   110,   110,   111,
     111,   112,   112,   113,   113,   113,   113,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   115,
     115,   116,   116,   117,   117,   118,   118,   119,   119,   120,
     120,   121,   121,   121,   122,   122,   123,   123,   123,   124,
     124,   124,   124,   125,   125,   125,   125,   125,   126,   126,
     126,   127,   128,   128,   128,   128,   128,   129,   130,   131,
     132,   133,   133,   134,   135,   136,   136,   136,   136,   137,
     137,   137,   137,   137,   137,   137,   138,   138,   139,   139,
     140,   140,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   142,   143,   143,   143,   143,   143,   143,   143,   143,
     143,   143,   143,   143,   143,   143,   143,   143,   143,   143,
     143,   143,   143,   143,   143,   143,   144,   144,   144,   144,
     145,   145,   145,   145,   145,   145,   145,   145,   146,   146,
     146,   147,   147,   147,   148,   148,   149,   149,   149,   149,
     149,   149,   150,   150,   150,   150,   150,   150,   150,   150,
     150,   150,   150,   150,   151,   151,   151,   151,   151,   151,
     151,   151,   151,   151,   151,   151,   151,   151,   151,   151,
     151,   151,   151,   151,   151,   151,   151,   151,   151,   151,
     151,   151,   151,   151,   151,   151,   151,   151,   151,   151,
     152,   152,   152,   152,   152,   152,   152,   152,   152,   152,
     152,   152,   152,   152,   152,   152,   152,   153,   154,   154,
     154,   154,   154,   154,   154,   154,   154,   154,   154,   154,
     154,   154,   154,   154,   154,   155,   155,   155,   155,   156,
     157,   157,   158,   158,   159,   159,   159,   159,   160,   160,
     160,   160,   161,   161,   162,   162,   163,   164,   164,   165,
     165,   165,   165,   165,   166,   166,   166,   166,   167,   167,
     168,   169,   169,   169,   169,   169,   169,   170,   170,   170,
     170,   171,   171,   171,   171,   171,   171,   171,   171,   171,
     171,   171,   171,   171,   171,   171,   171,   172,   172,   173,
     173,   173,   173,   173,   173,   173,   173,   173,   173,   173,
     173,   173,   173,   173,   173,   173,   173,   173,   173,   173,
     173,   173,   173,   173,   173,   173,   173,   173,   173,   173,
     173,   173,   173,   173,   173,   173,   173,   173,   173,   173,
     173,   173,   173,   173,   173,   173,   173,   173,   173,   173,
     173,   173,   174,   175,   175,   175,   176,   176,   177,   177,
     177,   177,   177,   177,   177,   177
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     2,     2,     3,     1,     3,     4,     1,
       1,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     5,     0,     2,     0,     1,     6,
       5,     6,     7,     6,     8,     4,     4,     8,     8,     4,
       3,     5,     5,     7,     0,     2,     3,     1,     3,     1,
       3,     1,     3,     6,     8,     8,    10,     1,     1,     1,
       1,     1,     1,     2,     2,     2,     2,     2,     2,     0,
       1,     0,     3,     0,     2,     0,     2,     0,     2,     1,
       3,     1,     2,     2,     1,     3,     1,     3,     3,     1,
       3,     5,     7,     1,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     3,     3,     4,     4,     4,     4,     2,
       3,     1,     3,     1,     2,     1,     3,     5,     7,     3,
       3,     3,     3,     5,     5,     3,     1,     0,     1,     3,
       1,     3,     1,     3,     3,     6,     6,     4,     6,     6,
       4,     5,     8,    10,    10,    10,    10,     8,    10,    10,
      10,    10,     9,    11,    11,    11,    11,     7,     9,     9,
       9,     9,     3,     3,     4,     2,     0,     1,     1,     1,
       0,     1,     2,     4,     3,     3,     2,     3,     0,     1,
       2,     1,     1,     1,     3,     3,     2,     2,     2,     3,
       3,     3,     3,     4,     3,     4,     3,     4,     3,     4,
       3,     3,     4,     4,     1,     2,     2,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     4,     4,     3,     2,     3,     4,
       3,     3,     5,     3,     3,     3,     4,     6,     5,     5,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     3,     5,     7,     9,     1,     6,     5,
       3,     4,     5,     4,     4,     4,     4,     6,     4,     6,
       4,     6,     4,     4,     4,     8,     8,     8,     8,    12,
       1,     3,     2,     3,     5,     7,     7,     9,    10,    12,
      12,    14,     2,     3,     2,     3,     4,     1,     3,     2,
       2,     4,     3,     1,     3,     5,     4,     6,     1,     2,
       4,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     2,     3,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     3
};


/* YYDPREC[RULE-NUM] -- Dynamic precedence of rule #RULE-NUM (0 if none).  */
static const yytype_int8 yydprec[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     1,     1,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     2,     2,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     1,     2,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     1,     1,     2,     2,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0
};

/* YYMERGER[RULE-NUM] -- Index of merging function for rule #RULE-NUM.  */
static const yytype_int8 yymerger[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0
};

/* YYIMMEDIATE[RULE-NUM] -- True iff rule #RULE-NUM is not to be deferred, as
   in the case of predicates.  */
static const yybool yyimmediate[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0
};

/* YYCONFLP[YYPACT[STATE-NUM]] -- Pointer into YYCONFL of start of
   list of conflicting reductions corresponding to action entry for
   state STATE-NUM in yytable.  0 means no conflicts.  The list in
   yyconfl is terminated by a rule number of 0.  */
static const yytype_int8 yyconflp[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     3,     0,     7,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     5,
       0,     0,     0,     0,     0,     0,    31,     0,     0,     0,
       0,     0,     0,     0,     9,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    11,     0,     0,     0,     0,
       0,    13,    15,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      21,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      23,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    27,     0,     0,    29,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    25,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    35,
       0,     0,    37,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    33,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    17,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    19,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0
};

/* YYCONFL[I] -- lists of conflicting rule numbers, each terminated by
   0, pointed into by YYCONFLP.  */
static const short yyconfl[] =
{
       0,   338,     0,   338,     0,   338,     0,   337,     0,   337,
       0,   317,     0,   337,     0,   337,     0,   166,     0,   166,
       0,   319,     0,   320,     0,   167,     0,   231,     0,   231,
       0,   252,     0,   253,     0,   141,     0,   141,     0
};


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

# define YYRHSLOC(Rhs, K) ((Rhs)[K].yystate.yyloc)



#undef yynerrs
#define yynerrs (yystackp->yyerrcnt)
#undef yychar
#define yychar (yystackp->yyrawchar)
#undef yylval
#define yylval (yystackp->yyval)
#undef yylloc
#define yylloc (yystackp->yyloc)
#define cypher_yynerrs yynerrs
#define cypher_yychar yychar
#define cypher_yylval yylval
#define cypher_yylloc yylloc

enum { YYENOMEM = -2 };

typedef enum { yyok, yyaccept, yyabort, yyerr, yynomem } YYRESULTTAG;

#define YYCHK(YYE)                              \
  do {                                          \
    YYRESULTTAG yychk_flag = YYE;               \
    if (yychk_flag != yyok)                     \
      return yychk_flag;                        \
  } while (0)

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYMAXDEPTH * sizeof (GLRStackItem)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

/* Minimum number of free items on the stack allowed after an
   allocation.  This is to allow allocation and initialization
   to be completed by functions that call yyexpandGLRStack before the
   stack is expanded, thus insuring that all necessary pointers get
   properly redirected to new data.  */
#define YYHEADROOM 2

#ifndef YYSTACKEXPANDABLE
#  define YYSTACKEXPANDABLE 1
#endif

#if YYSTACKEXPANDABLE
# define YY_RESERVE_GLRSTACK(Yystack)                   \
  do {                                                  \
    if (Yystack->yyspaceLeft < YYHEADROOM)              \
      yyexpandGLRStack (Yystack);                       \
  } while (0)
#else
# define YY_RESERVE_GLRSTACK(Yystack)                   \
  do {                                                  \
    if (Yystack->yyspaceLeft < YYHEADROOM)              \
      yyMemoryExhausted (Yystack);                      \
  } while (0)
#endif

/** State numbers. */
typedef int yy_state_t;

/** Rule numbers. */
typedef int yyRuleNum;

/** Item references. */
typedef short yyItemNum;

typedef struct yyGLRState yyGLRState;
typedef struct yyGLRStateSet yyGLRStateSet;
typedef struct yySemanticOption yySemanticOption;
typedef union yyGLRStackItem yyGLRStackItem;
typedef struct yyGLRStack yyGLRStack;

struct yyGLRState
{
  /** Type tag: always true.  */
  yybool yyisState;
  /** Type tag for yysemantics.  If true, yyval applies, otherwise
   *  yyfirstVal applies.  */
  yybool yyresolved;
  /** Number of corresponding LALR(1) machine state.  */
  yy_state_t yylrState;
  /** Preceding state in this stack */
  yyGLRState* yypred;
  /** Source position of the last token produced by my symbol */
  YYPTRDIFF_T yyposn;
  union {
    /** First in a chain of alternative reductions producing the
     *  nonterminal corresponding to this state, threaded through
     *  yynext.  */
    yySemanticOption* yyfirstVal;
    /** Semantic value for this state.  */
    YYSTYPE yyval;
  } yysemantics;
  /** Source location for this state.  */
  YYLTYPE yyloc;
};

struct yyGLRStateSet
{
  yyGLRState** yystates;
  /** During nondeterministic operation, yylookaheadNeeds tracks which
   *  stacks have actually needed the current lookahead.  During deterministic
   *  operation, yylookaheadNeeds[0] is not maintained since it would merely
   *  duplicate yychar != CYPHER_CYPHER_YYEMPTY.  */
  yybool* yylookaheadNeeds;
  YYPTRDIFF_T yysize;
  YYPTRDIFF_T yycapacity;
};

struct yySemanticOption
{
  /** Type tag: always false.  */
  yybool yyisState;
  /** Rule number for this reduction */
  yyRuleNum yyrule;
  /** The last RHS state in the list of states to be reduced.  */
  yyGLRState* yystate;
  /** The lookahead for this reduction.  */
  int yyrawchar;
  YYSTYPE yyval;
  YYLTYPE yyloc;
  /** Next sibling in chain of options.  To facilitate merging,
   *  options are chained in decreasing order by address.  */
  yySemanticOption* yynext;
};

/** Type of the items in the GLR stack.  The yyisState field
 *  indicates which item of the union is valid.  */
union yyGLRStackItem {
  yyGLRState yystate;
  yySemanticOption yyoption;
};

struct yyGLRStack {
  int yyerrState;
  /* To compute the location of the error token.  */
  yyGLRStackItem yyerror_range[3];

  int yyerrcnt;
  int yyrawchar;
  YYSTYPE yyval;
  YYLTYPE yyloc;

  YYJMP_BUF yyexception_buffer;
  yyGLRStackItem* yyitems;
  yyGLRStackItem* yynextFree;
  YYPTRDIFF_T yyspaceLeft;
  yyGLRState* yysplitPoint;
  yyGLRState* yylastDeleted;
  yyGLRStateSet yytops;
};

#if YYSTACKEXPANDABLE
static void yyexpandGLRStack (yyGLRStack* yystackp);
#endif

_Noreturn static void
yyFail (yyGLRStack* yystackp, YYLTYPE *yylocp, cypher_parser_context *context, const char* yymsg)
{
  if (yymsg != YY_NULLPTR)
    yyerror (yylocp, context, yymsg);
  YYLONGJMP (yystackp->yyexception_buffer, 1);
}

_Noreturn static void
yyMemoryExhausted (yyGLRStack* yystackp)
{
  YYLONGJMP (yystackp->yyexception_buffer, 2);
}

/** Accessing symbol of state YYSTATE.  */
static inline yysymbol_kind_t
yy_accessing_symbol (yy_state_t yystate)
{
  return YY_CAST (yysymbol_kind_t, yystos[yystate]);
}

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  static const char *const yy_sname[] =
  {
  "end of file", "error", "invalid token", "INTEGER", "DECIMAL", "STRING",
  "IDENTIFIER", "PARAMETER", "BQIDENT", "NOT_EQ", "LT_EQ", "GT_EQ",
  "DOT_DOT", "TYPECAST", "PLUS_EQ", "REGEX_MATCH", "MATCH", "RETURN",
  "CREATE", "WHERE", "WITH", "SET", "DELETE", "REMOVE", "MERGE", "UNWIND",
  "DETACH", "FOREACH", "CALL", "YIELD", "OPTIONAL", "DISTINCT", "ORDER",
  "BY", "SKIP", "LIMIT", "AS", "ASC", "DESC", "AND", "OR", "XOR", "NOT",
  "IN", "IS", "NULL_P", "TRUE_P", "FALSE_P", "EXISTS", "ANY", "NONE",
  "SINGLE", "REDUCE", "UNION", "ALL", "CASE", "WHEN", "THEN", "ELSE",
  "END_P", "ON", "SHORTESTPATH", "ALLSHORTESTPATHS", "PATTERN", "EXPLAIN",
  "LOAD", "CSV", "FROM", "HEADERS", "FIELDTERMINATOR", "STARTS", "ENDS",
  "CONTAINS", "'='", "'<'", "'>'", "'+'", "'-'", "'*'", "'/'", "'%'",
  "'^'", "UNARY_MINUS", "UNARY_PLUS", "'.'", "'['", "';'", "','", "'('",
  "'|'", "')'", "'{'", "'}'", "':'", "']'", "$accept", "stmt",
  "union_query", "single_query", "clause_list", "clause", "match_clause",
  "from_graph_opt", "optional_opt", "return_clause", "with_clause",
  "unwind_clause", "foreach_clause", "call_clause", "proc_args_opt",
  "yield_item_list", "yield_item", "procedure_name", "load_csv_clause",
  "foreach_update_list", "distinct_opt", "order_by_opt", "skip_opt",
  "limit_opt", "where_opt", "order_by_list", "order_by_item",
  "return_item_list", "return_item", "set_item_list", "set_target",
  "set_item", "create_clause", "merge_clause", "on_create_clause",
  "on_match_clause", "set_clause", "delete_clause", "delete_item_list",
  "delete_item", "remove_clause", "remove_item_list", "remove_item",
  "detach_opt", "pattern_list", "simple_path", "path", "node_pattern",
  "rel_pattern", "variable_opt", "varlen_range_opt", "label_opt",
  "label_name", "label_alt_list", "label_list", "rel_type_list", "expr",
  "primary_expr", "literal_expr", "function_call", "list_predicate",
  "reduce_expr", "argument_list", "list_literal", "list_comprehension",
  "pattern_comprehension", "comp_steps", "map_literal", "map_projection",
  "map_projection_list", "map_projection_item", "case_expression",
  "when_clause_list", "when_clause", "literal", "identifier", "soft_kw",
  "var_name", "non_reserved_kw", "parameter", "properties_opt",
  "map_pair_list", "map_pair", YY_NULLPTR
  };
  return yy_sname[yysymbol];
}
#endif

/** Left-hand-side symbol for rule #YYRULE.  */
static inline yysymbol_kind_t
yylhsNonterm (yyRuleNum yyrule)
{
  return YY_CAST (yysymbol_kind_t, yyr1[yyrule]);
}

#if CYPHER_YYDEBUG

# ifndef YYFPRINTF
#  define YYFPRINTF fprintf
# endif

# define YY_FPRINTF                             \
  YY_IGNORE_USELESS_CAST_BEGIN YY_FPRINTF_

# define YY_FPRINTF_(Args)                      \
  do {                                          \
    YYFPRINTF Args;                             \
    YY_IGNORE_USELESS_CAST_END                  \
  } while (0)

# define YY_DPRINTF                             \
  YY_IGNORE_USELESS_CAST_BEGIN YY_DPRINTF_

# define YY_DPRINTF_(Args)                      \
  do {                                          \
    if (yydebug)                                \
      YYFPRINTF Args;                           \
    YY_IGNORE_USELESS_CAST_END                  \
  } while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined CYPHER_YYLTYPE_IS_TRIVIAL && CYPHER_YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */



/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, cypher_parser_context *context)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  YY_USE (context);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, cypher_parser_context *context)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp, context);
  YYFPRINTF (yyo, ")");
}

# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                  \
  do {                                                                  \
    if (yydebug)                                                        \
      {                                                                 \
        YY_FPRINTF ((stderr, "%s ", Title));                            \
        yy_symbol_print (stderr, Kind, Value, Location, context);        \
        YY_FPRINTF ((stderr, "\n"));                                    \
      }                                                                 \
  } while (0)

static inline void
yy_reduce_print (yybool yynormal, yyGLRStackItem* yyvsp, YYPTRDIFF_T yyk,
                 yyRuleNum yyrule, cypher_parser_context *context);

# define YY_REDUCE_PRINT(Args)          \
  do {                                  \
    if (yydebug)                        \
      yy_reduce_print Args;             \
  } while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;

static void yypstack (yyGLRStack* yystackp, YYPTRDIFF_T yyk)
  YY_ATTRIBUTE_UNUSED;
static void yypdumpstack (yyGLRStack* yystackp)
  YY_ATTRIBUTE_UNUSED;

#else /* !CYPHER_YYDEBUG */

# define YY_DPRINTF(Args) do {} while (yyfalse)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_REDUCE_PRINT(Args)

#endif /* !CYPHER_YYDEBUG */

#ifndef yystrlen
# define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif



/** Fill in YYVSP[YYLOW1 .. YYLOW0-1] from the chain of states starting
 *  at YYVSP[YYLOW0].yystate.yypred.  Leaves YYVSP[YYLOW1].yystate.yypred
 *  containing the pointer to the next state in the chain.  */
static void yyfillin (yyGLRStackItem *, int, int) YY_ATTRIBUTE_UNUSED;
static void
yyfillin (yyGLRStackItem *yyvsp, int yylow0, int yylow1)
{
  int i;
  yyGLRState *s = yyvsp[yylow0].yystate.yypred;
  for (i = yylow0-1; i >= yylow1; i -= 1)
    {
#if CYPHER_YYDEBUG
      yyvsp[i].yystate.yylrState = s->yylrState;
#endif
      yyvsp[i].yystate.yyresolved = s->yyresolved;
      if (s->yyresolved)
        yyvsp[i].yystate.yysemantics.yyval = s->yysemantics.yyval;
      else
        /* The effect of using yyval or yyloc (in an immediate rule) is
         * undefined.  */
        yyvsp[i].yystate.yysemantics.yyfirstVal = YY_NULLPTR;
      yyvsp[i].yystate.yyloc = s->yyloc;
      s = yyvsp[i].yystate.yypred = s->yypred;
    }
}


/** If yychar is empty, fetch the next token.  */
static inline yysymbol_kind_t
yygetToken (int *yycharp, yyGLRStack* yystackp, cypher_parser_context *context)
{
  yysymbol_kind_t yytoken;
  YY_USE (context);
  if (*yycharp == CYPHER_CYPHER_YYEMPTY)
    {
      YY_DPRINTF ((stderr, "Reading a token\n"));
      *yycharp = yylex (&yylval, &yylloc, context);
    }
  if (*yycharp <= CYPHER_YYEOF)
    {
      *yycharp = CYPHER_YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YY_DPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (*yycharp);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }
  return yytoken;
}

/* Do nothing if YYNORMAL or if *YYLOW <= YYLOW1.  Otherwise, fill in
 * YYVSP[YYLOW1 .. *YYLOW-1] as in yyfillin and set *YYLOW = YYLOW1.
 * For convenience, always return YYLOW1.  */
static inline int yyfill (yyGLRStackItem *, int *, int, yybool)
     YY_ATTRIBUTE_UNUSED;
static inline int
yyfill (yyGLRStackItem *yyvsp, int *yylow, int yylow1, yybool yynormal)
{
  if (!yynormal && yylow1 < *yylow)
    {
      yyfillin (yyvsp, *yylow, yylow1);
      *yylow = yylow1;
    }
  return yylow1;
}

/** Perform user action for rule number YYN, with RHS length YYRHSLEN,
 *  and top stack item YYVSP.  YYLVALP points to place to put semantic
 *  value ($$), and yylocp points to place for location information
 *  (@$).  Returns yyok for normal return, yyaccept for YYACCEPT,
 *  yyerr for YYERROR, yyabort for YYABORT, yynomem for YYNOMEM.  */
static YYRESULTTAG
yyuserAction (yyRuleNum yyrule, int yyrhslen, yyGLRStackItem* yyvsp,
              yyGLRStack* yystackp, YYPTRDIFF_T yyk,
              YYSTYPE* yyvalp, YYLTYPE *yylocp, cypher_parser_context *context)
{
  const yybool yynormal YY_ATTRIBUTE_UNUSED = yystackp->yysplitPoint == YY_NULLPTR;
  int yylow = 1;
  YY_USE (yyvalp);
  YY_USE (yylocp);
  YY_USE (context);
  YY_USE (yyk);
  YY_USE (yyrhslen);
# undef yyerrok
# define yyerrok (yystackp->yyerrState = 0)
# undef YYACCEPT
# define YYACCEPT return yyaccept
# undef YYABORT
# define YYABORT return yyabort
# undef YYNOMEM
# define YYNOMEM return yynomem
# undef YYERROR
# define YYERROR return yyerrok, yyerr
# undef YYRECOVERING
# define YYRECOVERING() (yystackp->yyerrState != 0)
# undef yyclearin
# define yyclearin (yychar = CYPHER_CYPHER_YYEMPTY)
# undef YYFILL
# define YYFILL(N) yyfill (yyvsp, &yylow, (N), yynormal)
# undef YYBACKUP
# define YYBACKUP(Token, Value)                                              \
  return yyerror (yylocp, context, YY_("syntax error: cannot back up")),     \
         yyerrok, yyerr

  if (yyrhslen == 0)
    *yyvalp = yyval_default;
  else
    *yyvalp = yyvsp[YYFILL (1-yyrhslen)].yystate.yysemantics.yyval;
  /* Default location. */
  YYLLOC_DEFAULT ((*yylocp), (yyvsp - yyrhslen), yyrhslen);
  yystackp->yyerror_range[1].yystate.yyloc = *yylocp;
  /* If yyk == -1, we are running a deferred action on a temporary
     stack.  In that case, YY_REDUCE_PRINT must not play with YYFILL,
     so pretend the stack is "normal". */
  YY_REDUCE_PRINT ((yynormal || yyk == -1, yyvsp, yyk, yyrule, context));
  switch (yyrule)
    {
  case 2: /* stmt: union_query  */
#line 267 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node);
            context->result = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node);
        }
#line 3294 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 3: /* stmt: union_query ';'  */
#line 272 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node);
            context->result = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node);
        }
#line 3303 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 4: /* stmt: EXPLAIN union_query  */
#line 277 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* For EXPLAIN with UNION, wrap if needed */
            if ((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node)->type == AST_NODE_QUERY) {
                ((cypher_query*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node))->explain = true;
            }
            ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node);
            context->result = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node);
        }
#line 3316 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 5: /* stmt: EXPLAIN union_query ';'  */
#line 286 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            if ((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node)->type == AST_NODE_QUERY) {
                ((cypher_query*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node))->explain = true;
            }
            ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node);
            context->result = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node);
        }
#line 3328 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 6: /* union_query: single_query  */
#line 297 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node);
        }
#line 3336 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 7: /* union_query: union_query UNION single_query  */
#line 301 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_cypher_union((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), false, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line);
        }
#line 3344 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 8: /* union_query: union_query UNION ALL single_query  */
#line 305 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_cypher_union((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), true, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
        }
#line 3352 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 9: /* single_query: clause_list  */
#line 312 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_cypher_query((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list), false);
        }
#line 3360 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 10: /* clause_list: clause  */
#line 319 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
        }
#line 3369 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 11: /* clause_list: clause_list clause  */
#line 324 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list);
        }
#line 3378 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 12: /* clause: match_clause  */
#line 331 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.match); }
#line 3384 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 13: /* clause: return_clause  */
#line 332 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.return_clause); }
#line 3390 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 14: /* clause: with_clause  */
#line 333 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.with_clause); }
#line 3396 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 15: /* clause: unwind_clause  */
#line 334 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 3402 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 16: /* clause: foreach_clause  */
#line 335 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 3408 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 17: /* clause: load_csv_clause  */
#line 336 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 3414 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 18: /* clause: create_clause  */
#line 337 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.create); }
#line 3420 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 19: /* clause: merge_clause  */
#line 338 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.merge); }
#line 3426 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 20: /* clause: set_clause  */
#line 339 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.set); }
#line 3432 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 21: /* clause: delete_clause  */
#line 340 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.delete); }
#line 3438 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 22: /* clause: remove_clause  */
#line 341 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.remove); }
#line 3444 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 23: /* clause: call_clause  */
#line 342 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 3450 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 24: /* match_clause: optional_opt MATCH pattern_list from_graph_opt where_opt  */
#line 348 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).match) = make_cypher_match((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.boolean), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.string));
        }
#line 3458 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 25: /* from_graph_opt: %empty  */
#line 354 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = NULL; }
#line 3464 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 26: /* from_graph_opt: FROM IDENTIFIER  */
#line 356 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).string) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string);
        }
#line 3472 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 27: /* optional_opt: %empty  */
#line 362 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).boolean) = false; }
#line 3478 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 28: /* optional_opt: OPTIONAL  */
#line 363 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).boolean) = true; }
#line 3484 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 29: /* return_clause: RETURN distinct_opt return_item_list order_by_opt skip_opt limit_opt  */
#line 369 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).return_clause) = make_cypher_return((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.boolean), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
        }
#line 3492 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 30: /* return_clause: RETURN '*' order_by_opt skip_opt limit_opt  */
#line 373 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* RETURN * - return all bound variables (items=NULL signals star) */
            ((*yyvalp).return_clause) = make_cypher_return(false, NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
        }
#line 3501 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 31: /* return_clause: RETURN DISTINCT '*' order_by_opt skip_opt limit_opt  */
#line 378 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).return_clause) = make_cypher_return(true, NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
        }
#line 3509 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 32: /* with_clause: WITH distinct_opt return_item_list order_by_opt skip_opt limit_opt where_opt  */
#line 386 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).with_clause) = make_cypher_with((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.boolean), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
        }
#line 3517 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 33: /* with_clause: WITH '*' order_by_opt skip_opt limit_opt where_opt  */
#line 390 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* WITH * - pass all variables through */
            ((*yyvalp).with_clause) = make_cypher_with(false, NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
        }
#line 3526 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 34: /* with_clause: WITH '*' ',' return_item_list order_by_opt skip_opt limit_opt where_opt  */
#line 395 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* WITH *, a.v AS x: every bound variable and the items; the
             * validator adds the variables (GQLITE-T-0485). */
            ((*yyvalp).with_clause) = make_cypher_with(false, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
            if (((*yyvalp).with_clause)) ((*yyvalp).with_clause)->star_plus = true;
        }
#line 3537 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 35: /* unwind_clause: UNWIND expr AS var_name  */
#line 406 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_cypher_unwind((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 3546 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 36: /* unwind_clause: UNWIND expr AS BQIDENT  */
#line 411 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Backtick-quoted alias (scanner stripped the backticks). */
            ((*yyvalp).node) = (ast_node*)make_cypher_unwind((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 3556 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 37: /* foreach_clause: FOREACH '(' var_name IN expr '|' foreach_update_list ')'  */
#line 421 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_cypher_foreach((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 3565 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 38: /* foreach_clause: FOREACH '(' BQIDENT IN expr '|' foreach_update_list ')'  */
#line 426 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* T-0329: backtick-quoted loop variable. */
            ((*yyvalp).node) = (ast_node*)make_cypher_foreach((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 3575 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 39: /* call_clause: CALL '{' union_query '}'  */
#line 440 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list *branches = ast_list_create();
            ast_list_append(branches, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node));
            ((*yyvalp).node) = (ast_node*)make_cypher_call_subquery(branches, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
        }
#line 3585 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 40: /* call_clause: CALL procedure_name proc_args_opt  */
#line 446 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* `CALL pageRank()`: the graph algorithms are procedures; any
             * other name is ProcedureNotFound at run time (GQLITE-T-0389). */
            ((*yyvalp).node) = (ast_node*)make_cypher_call_procedure((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list), NULL, false, NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.string));
        }
#line 3596 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 41: /* call_clause: CALL procedure_name proc_args_opt YIELD '*'  */
#line 453 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_cypher_call_procedure((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), NULL, true, NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
        }
#line 3605 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 42: /* call_clause: CALL procedure_name proc_args_opt YIELD yield_item_list  */
#line 458 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_cypher_call_procedure((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list), false, NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
        }
#line 3614 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 43: /* call_clause: CALL procedure_name proc_args_opt YIELD yield_item_list WHERE expr  */
#line 463 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_cypher_call_procedure((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), false, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 3623 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 44: /* proc_args_opt: %empty  */
#line 470 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                                 { ((*yyvalp).list) = NULL; }
#line 3629 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 45: /* proc_args_opt: '(' ')'  */
#line 471 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                                 { ((*yyvalp).list) = ast_list_create(); }
#line 3635 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 46: /* proc_args_opt: '(' argument_list ')'  */
#line 472 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                                 { ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list); }
#line 3641 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 47: /* yield_item_list: yield_item  */
#line 477 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.return_item));
        }
#line 3650 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 48: /* yield_item_list: yield_item_list ',' yield_item  */
#line 482 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.return_item));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
        }
#line 3659 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 49: /* yield_item: var_name  */
#line 490 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).return_item) = make_return_item((ast_node*)make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 3668 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 50: /* yield_item: var_name AS var_name  */
#line 495 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).return_item) = make_return_item((ast_node*)make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 3678 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 51: /* procedure_name: IDENTIFIER  */
#line 504 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        { ((*yyvalp).string) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string); }
#line 3684 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 52: /* procedure_name: procedure_name '.' IDENTIFIER  */
#line 506 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            size_t n = strlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string)) + strlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string)) + 2;
            ((*yyvalp).string) = malloc(n);
            snprintf(((*yyvalp).string), n, "%s.%s", (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 3696 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 53: /* load_csv_clause: LOAD CSV FROM STRING AS var_name  */
#line 518 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_cypher_load_csv((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), false, NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 3706 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 54: /* load_csv_clause: LOAD CSV WITH HEADERS FROM STRING AS var_name  */
#line 524 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_cypher_load_csv((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), true, NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 3716 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 55: /* load_csv_clause: LOAD CSV FROM STRING AS var_name FIELDTERMINATOR STRING  */
#line 530 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_cypher_load_csv((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), false, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 3727 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 56: /* load_csv_clause: LOAD CSV WITH HEADERS FROM STRING AS var_name FIELDTERMINATOR STRING  */
#line 537 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_cypher_load_csv((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), true, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-9)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 3738 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 57: /* foreach_update_list: create_clause  */
#line 548 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.create));
        }
#line 3747 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 58: /* foreach_update_list: set_clause  */
#line 553 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.set));
        }
#line 3756 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 59: /* foreach_update_list: delete_clause  */
#line 558 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.delete));
        }
#line 3765 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 60: /* foreach_update_list: merge_clause  */
#line 563 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.merge));
        }
#line 3774 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 61: /* foreach_update_list: remove_clause  */
#line 568 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.remove));
        }
#line 3783 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 62: /* foreach_update_list: foreach_clause  */
#line 573 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
        }
#line 3792 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 63: /* foreach_update_list: foreach_update_list create_clause  */
#line 578 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.create));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list);
        }
#line 3801 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 64: /* foreach_update_list: foreach_update_list set_clause  */
#line 583 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.set));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list);
        }
#line 3810 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 65: /* foreach_update_list: foreach_update_list delete_clause  */
#line 588 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.delete));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list);
        }
#line 3819 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 66: /* foreach_update_list: foreach_update_list merge_clause  */
#line 593 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.merge));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list);
        }
#line 3828 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 67: /* foreach_update_list: foreach_update_list remove_clause  */
#line 598 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.remove));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list);
        }
#line 3837 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 68: /* foreach_update_list: foreach_update_list foreach_clause  */
#line 603 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list);
        }
#line 3846 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 69: /* distinct_opt: %empty  */
#line 610 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).boolean) = false; }
#line 3852 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 70: /* distinct_opt: DISTINCT  */
#line 611 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).boolean) = true; }
#line 3858 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 71: /* order_by_opt: %empty  */
#line 615 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).list) = NULL; }
#line 3864 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 72: /* order_by_opt: ORDER BY order_by_list  */
#line 616 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                             { ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list); }
#line 3870 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 73: /* skip_opt: %empty  */
#line 620 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).node) = NULL; }
#line 3876 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 74: /* skip_opt: SKIP expr  */
#line 621 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 3882 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 75: /* limit_opt: %empty  */
#line 625 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).node) = NULL; }
#line 3888 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 76: /* limit_opt: LIMIT expr  */
#line 626 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 3894 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 77: /* where_opt: %empty  */
#line 630 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).node) = NULL; }
#line 3900 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 78: /* where_opt: WHERE expr  */
#line 631 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 3906 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 79: /* order_by_list: order_by_item  */
#line 636 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.order_by_item));
        }
#line 3915 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 80: /* order_by_list: order_by_list ',' order_by_item  */
#line 641 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.order_by_item));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
        }
#line 3924 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 81: /* order_by_item: expr  */
#line 648 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).order_by_item) = make_order_by_item((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), false); /* Default is ASC */ }
#line 3930 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 82: /* order_by_item: expr ASC  */
#line 649 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).order_by_item) = make_order_by_item((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), false); }
#line 3936 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 83: /* order_by_item: expr DESC  */
#line 650 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).order_by_item) = make_order_by_item((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), true); }
#line 3942 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 84: /* return_item_list: return_item  */
#line 656 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.return_item));
        }
#line 3951 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 85: /* return_item_list: return_item_list ',' return_item  */
#line 661 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.return_item));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
        }
#line 3960 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 86: /* return_item: expr  */
#line 669 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).return_item) = make_return_item((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), NULL);
            /* The column of an unaliased item is named as written
             * (`count( * )`, `n.missing IS NULL`). */
            if (((*yyvalp).return_item)) ((*yyvalp).return_item)->source_text = source_text_span(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column,
                                                       (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).last_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).last_column);
        }
#line 3972 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 87: /* return_item: expr AS var_name  */
#line 677 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).return_item) = make_return_item((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 3981 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 88: /* return_item: expr AS BQIDENT  */
#line 682 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Backtick-quoted alias (e.g. RETURN x AS `name`) — the
             * scanner has already stripped the surrounding backticks.
             * Same shape as IDENTIFIER. */
            ((*yyvalp).return_item) = make_return_item((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 3993 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 89: /* set_item_list: set_item  */
#line 693 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.set_item));
        }
#line 4002 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 90: /* set_item_list: set_item_list ',' set_item  */
#line 698 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.set_item));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
        }
#line 4011 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 91: /* set_item_list: var_name ':' label_name ':' label_name  */
#line 703 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* SET n:L1:L2 — produce two set_items, one per label. */
            ((*yyvalp).list) = ast_list_create();
            {
              cypher_identifier *v1 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
              cypher_label_expr *l1 = make_label_expr((ast_node*)v1, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
              ast_list_append(((*yyvalp).list), (ast_node*)make_cypher_set_item((ast_node*)l1, NULL, false));
            }
            {
              cypher_identifier *v2 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
              cypher_label_expr *l2 = make_label_expr((ast_node*)v2, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
              ast_list_append(((*yyvalp).list), (ast_node*)make_cypher_set_item((ast_node*)l2, NULL, false));
            }
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string));
        }
#line 4031 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 92: /* set_item_list: var_name ':' label_name ':' label_name ':' label_name  */
#line 719 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* SET n:L1:L2:L3 — three labels. */
            ((*yyvalp).list) = ast_list_create();
            {
              cypher_identifier *v1 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
              cypher_label_expr *l1 = make_label_expr((ast_node*)v1, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
              ast_list_append(((*yyvalp).list), (ast_node*)make_cypher_set_item((ast_node*)l1, NULL, false));
            }
            {
              cypher_identifier *v2 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
              cypher_label_expr *l2 = make_label_expr((ast_node*)v2, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
              ast_list_append(((*yyvalp).list), (ast_node*)make_cypher_set_item((ast_node*)l2, NULL, false));
            }
            {
              cypher_identifier *v3 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
              cypher_label_expr *l3 = make_label_expr((ast_node*)v3, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
              ast_list_append(((*yyvalp).list), (ast_node*)make_cypher_set_item((ast_node*)l3, NULL, false));
            }
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string));
        }
#line 4056 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 93: /* set_target: identifier  */
#line 745 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                                   { ((*yyvalp).node) = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.identifier); }
#line 4062 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 94: /* set_target: '(' expr ')'  */
#line 746 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                                   { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node); if (((*yyvalp).node)) ((*yyvalp).node)->parenthesized = true; }
#line 4068 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 95: /* set_target: set_target '.' IDENTIFIER  */
#line 748 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_property((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4077 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 96: /* set_target: set_target '.' BQIDENT  */
#line 753 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_property((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4086 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 97: /* set_target: set_target '.' non_reserved_kw  */
#line 758 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_property((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4095 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 98: /* set_item: set_target '=' expr  */
#line 766 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).set_item) = make_cypher_set_item((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), false);
        }
#line 4103 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 99: /* set_item: set_target PLUS_EQ expr  */
#line 770 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* SET n += {map} — merge properties */
            ((*yyvalp).set_item) = make_cypher_set_item((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), true);
        }
#line 4112 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 100: /* set_item: var_name ':' label_name  */
#line 775 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* SET n:Label syntax */
            cypher_identifier *var = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            cypher_label_expr *label = make_label_expr((ast_node*)var, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ((*yyvalp).set_item) = make_cypher_set_item((ast_node*)label, NULL, false);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4125 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 101: /* create_clause: CREATE pattern_list  */
#line 788 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).create) = make_cypher_create((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list));
        }
#line 4133 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 102: /* merge_clause: MERGE pattern_list  */
#line 796 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).merge) = make_cypher_merge((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list), NULL, NULL);
        }
#line 4141 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 103: /* merge_clause: MERGE pattern_list on_create_clause  */
#line 800 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).merge) = make_cypher_merge((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list), NULL);
        }
#line 4149 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 104: /* merge_clause: MERGE pattern_list on_match_clause  */
#line 804 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).merge) = make_cypher_merge((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list));
        }
#line 4157 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 105: /* merge_clause: MERGE pattern_list on_create_clause on_match_clause  */
#line 808 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).merge) = make_cypher_merge((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list));
        }
#line 4165 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 106: /* merge_clause: MERGE pattern_list on_match_clause on_create_clause  */
#line 812 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).merge) = make_cypher_merge((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list));
        }
#line 4173 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 107: /* on_create_clause: ON CREATE SET set_item_list  */
#line 819 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list);
        }
#line 4181 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 108: /* on_match_clause: ON MATCH SET set_item_list  */
#line 826 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list);
        }
#line 4189 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 109: /* set_clause: SET set_item_list  */
#line 834 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).set) = make_cypher_set((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list));
        }
#line 4197 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 110: /* delete_clause: detach_opt DELETE delete_item_list  */
#line 842 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).delete) = make_cypher_delete((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.boolean));
        }
#line 4205 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 111: /* delete_item_list: delete_item  */
#line 849 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.delete_item));
        }
#line 4214 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 112: /* delete_item_list: delete_item_list ',' delete_item  */
#line 854 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.delete_item));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
        }
#line 4223 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 113: /* delete_item: expr  */
#line 862 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* `DELETE n`, or an expression: `DELETE list[0]`, `DELETE m.key`. */
            ((*yyvalp).delete_item) = make_delete_item_expr((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
        }
#line 4232 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 114: /* remove_clause: REMOVE remove_item_list  */
#line 871 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).remove) = make_cypher_remove((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list));
        }
#line 4240 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 115: /* remove_item_list: remove_item  */
#line 878 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.remove_item));
        }
#line 4249 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 116: /* remove_item_list: remove_item_list ',' remove_item  */
#line 883 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.remove_item));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
        }
#line 4258 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 117: /* remove_item_list: var_name ':' label_name ':' label_name  */
#line 888 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* REMOVE n:L1:L2 — produce two remove_items, one per label. */
            ((*yyvalp).list) = ast_list_create();
            {
              cypher_identifier *v1 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
              cypher_label_expr *l1 = make_label_expr((ast_node*)v1, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
              ast_list_append(((*yyvalp).list), (ast_node*)make_remove_item((ast_node*)l1));
            }
            {
              cypher_identifier *v2 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
              cypher_label_expr *l2 = make_label_expr((ast_node*)v2, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
              ast_list_append(((*yyvalp).list), (ast_node*)make_remove_item((ast_node*)l2));
            }
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string));
        }
#line 4278 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 118: /* remove_item_list: var_name ':' label_name ':' label_name ':' label_name  */
#line 904 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* REMOVE n:L1:L2:L3 — three labels. */
            ((*yyvalp).list) = ast_list_create();
            {
              cypher_identifier *v1 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
              cypher_label_expr *l1 = make_label_expr((ast_node*)v1, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
              ast_list_append(((*yyvalp).list), (ast_node*)make_remove_item((ast_node*)l1));
            }
            {
              cypher_identifier *v2 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
              cypher_label_expr *l2 = make_label_expr((ast_node*)v2, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
              ast_list_append(((*yyvalp).list), (ast_node*)make_remove_item((ast_node*)l2));
            }
            {
              cypher_identifier *v3 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
              cypher_label_expr *l3 = make_label_expr((ast_node*)v3, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
              ast_list_append(((*yyvalp).list), (ast_node*)make_remove_item((ast_node*)l3));
            }
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string));
        }
#line 4303 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 119: /* remove_item: var_name '.' IDENTIFIER  */
#line 928 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* REMOVE n.property - property access */
            cypher_identifier *base = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            ast_node *prop = (ast_node*)make_property((ast_node*)base, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ((*yyvalp).remove_item) = make_remove_item(prop);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4316 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 120: /* remove_item: var_name '.' BQIDENT  */
#line 937 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* REMOVE n.`special-key` - backtick-quoted property */
            cypher_identifier *base = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            ast_node *prop = (ast_node*)make_property((ast_node*)base, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ((*yyvalp).remove_item) = make_remove_item(prop);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4329 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 121: /* remove_item: BQIDENT '.' IDENTIFIER  */
#line 946 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* REMOVE `special-var`.property */
            cypher_identifier *base = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            ast_node *prop = (ast_node*)make_property((ast_node*)base, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ((*yyvalp).remove_item) = make_remove_item(prop);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4342 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 122: /* remove_item: BQIDENT '.' BQIDENT  */
#line 955 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* REMOVE `special-var`.`special-key` */
            cypher_identifier *base = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            ast_node *prop = (ast_node*)make_property((ast_node*)base, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ((*yyvalp).remove_item) = make_remove_item(prop);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4355 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 123: /* remove_item: '(' expr ')' '.' IDENTIFIER  */
#line 964 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* REMOVE (expr).property: the entity an expression gives
             * (GQLITE-T-0524). */
            ((*yyvalp).remove_item) = make_remove_item((ast_node*)make_property((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4366 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 124: /* remove_item: '(' expr ')' '.' BQIDENT  */
#line 971 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).remove_item) = make_remove_item((ast_node*)make_property((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4375 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 125: /* remove_item: var_name ':' label_name  */
#line 976 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* REMOVE n:Label syntax */
            cypher_identifier *var = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            cypher_label_expr *label = make_label_expr((ast_node*)var, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ((*yyvalp).remove_item) = make_remove_item((ast_node*)label);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4388 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 126: /* detach_opt: DETACH  */
#line 988 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).boolean) = true;
        }
#line 4396 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 127: /* detach_opt: %empty  */
#line 992 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).boolean) = false;
        }
#line 4404 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 128: /* pattern_list: path  */
#line 1000 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.path));
        }
#line 4413 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 129: /* pattern_list: pattern_list ',' path  */
#line 1005 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.path));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
        }
#line 4422 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 130: /* simple_path: node_pattern  */
#line 1014 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list *elements = ast_list_create();
            ast_list_append(elements, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node_pattern));
            ((*yyvalp).path) = make_path(elements);
        }
#line 4432 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 131: /* simple_path: simple_path rel_pattern node_pattern  */
#line 1020 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Create a new list copying elements from the existing path */
            ast_list *new_elements = ast_list_create();
            for (int i = 0; i < (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.path)->elements->count; i++) {
                ast_list_append(new_elements, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.path)->elements->items[i]);
            }
            ast_list_append(new_elements, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.rel_pattern));
            ast_list_append(new_elements, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node_pattern));

            /* Note: Let Bison handle memory cleanup of $1 automatically */
            /* Manual freeing during parsing can cause parser state corruption */

            ((*yyvalp).path) = make_path(new_elements);
        }
#line 4451 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 132: /* path: simple_path  */
#line 1039 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).path) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.path);
        }
#line 4459 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 133: /* path: var_name '=' simple_path  */
#line 1043 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).path) = make_path_with_var((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.path)->elements);
            /* Free the anonymous path structure, but keep its elements */
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.path));
        }
#line 4469 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 134: /* path: BQIDENT '=' simple_path  */
#line 1049 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* T-0329: backtick-quoted named-path variable. */
            ((*yyvalp).path) = make_path_with_var((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.path)->elements);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.path));
        }
#line 4479 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 135: /* path: var_name '=' SHORTESTPATH '(' simple_path ')'  */
#line 1055 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            cypher_path *sp = make_shortest_path((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.path)->elements, PATH_TYPE_SHORTEST);
            sp->var_name = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string);
            ((*yyvalp).path) = sp;
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.path));
        }
#line 4490 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 136: /* path: BQIDENT '=' SHORTESTPATH '(' simple_path ')'  */
#line 1062 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* T-0329: backtick-quoted variable on shortestPath. */
            cypher_path *sp = make_shortest_path((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.path)->elements, PATH_TYPE_SHORTEST);
            sp->var_name = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string);
            ((*yyvalp).path) = sp;
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.path));
        }
#line 4502 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 137: /* path: SHORTESTPATH '(' simple_path ')'  */
#line 1070 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).path) = make_shortest_path((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.path)->elements, PATH_TYPE_SHORTEST);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.path));
        }
#line 4511 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 138: /* path: var_name '=' ALLSHORTESTPATHS '(' simple_path ')'  */
#line 1075 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            cypher_path *sp = make_shortest_path((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.path)->elements, PATH_TYPE_ALL_SHORTEST);
            sp->var_name = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string);
            ((*yyvalp).path) = sp;
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.path));
        }
#line 4522 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 139: /* path: BQIDENT '=' ALLSHORTESTPATHS '(' simple_path ')'  */
#line 1082 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* T-0329: backtick-quoted variable on allShortestPaths. */
            cypher_path *sp = make_shortest_path((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.path)->elements, PATH_TYPE_ALL_SHORTEST);
            sp->var_name = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string);
            ((*yyvalp).path) = sp;
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.path));
        }
#line 4534 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 140: /* path: ALLSHORTESTPATHS '(' simple_path ')'  */
#line 1090 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).path) = make_shortest_path((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.path)->elements, PATH_TYPE_ALL_SHORTEST);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.path));
        }
#line 4543 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 141: /* node_pattern: '(' variable_opt label_opt properties_opt ')'  */
#line 1098 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node_pattern) = make_node_pattern((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.map));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
        }
#line 4552 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 142: /* rel_pattern: '-' '[' variable_opt varlen_range_opt properties_opt ']' '-' '>'  */
#line 1114 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), NULL, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.map), false, true, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 4561 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 143: /* rel_pattern: '-' '[' variable_opt ':' IDENTIFIER varlen_range_opt properties_opt ']' '-' '>'  */
#line 1119 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.map), false, true, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 4571 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 144: /* rel_pattern: '-' '[' variable_opt ':' BQIDENT varlen_range_opt properties_opt ']' '-' '>'  */
#line 1125 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.map), false, true, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 4581 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 145: /* rel_pattern: '-' '[' variable_opt ':' non_reserved_kw varlen_range_opt properties_opt ']' '-' '>'  */
#line 1131 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* T-0326: keyword-named rel types (CONTAINS/STARTS/ENDS/etc.). */
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.map), false, true, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 4592 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 146: /* rel_pattern: '-' '[' variable_opt ':' rel_type_list varlen_range_opt properties_opt ']' '-' '>'  */
#line 1138 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            cypher_rel_pattern *p = make_rel_pattern_multi_type((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.map), false, true);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string));
            if (p) p->varlen = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.varlen_range);
            ((*yyvalp).rel_pattern) = p;
        }
#line 4603 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 147: /* rel_pattern: '<' '-' '[' variable_opt varlen_range_opt properties_opt ']' '-'  */
#line 1146 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), NULL, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.map), true, false, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string));
        }
#line 4612 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 148: /* rel_pattern: '<' '-' '[' variable_opt ':' IDENTIFIER varlen_range_opt properties_opt ']' '-'  */
#line 1151 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.map), true, false, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string));
        }
#line 4622 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 149: /* rel_pattern: '<' '-' '[' variable_opt ':' BQIDENT varlen_range_opt properties_opt ']' '-'  */
#line 1157 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.map), true, false, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string));
        }
#line 4632 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 150: /* rel_pattern: '<' '-' '[' variable_opt ':' non_reserved_kw varlen_range_opt properties_opt ']' '-'  */
#line 1163 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* T-0326: keyword-named rel types (CONTAINS/STARTS/ENDS/etc.). */
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.map), true, false, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string));
        }
#line 4643 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 151: /* rel_pattern: '<' '-' '[' variable_opt ':' rel_type_list varlen_range_opt properties_opt ']' '-'  */
#line 1170 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            cypher_rel_pattern *p = make_rel_pattern_multi_type((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.map), true, false);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string));
            if (p) p->varlen = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.varlen_range);
            ((*yyvalp).rel_pattern) = p;
        }
#line 4654 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 152: /* rel_pattern: '<' '-' '[' variable_opt varlen_range_opt properties_opt ']' '-' '>'  */
#line 1180 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), NULL, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.map), false, false, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 4663 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 153: /* rel_pattern: '<' '-' '[' variable_opt ':' IDENTIFIER varlen_range_opt properties_opt ']' '-' '>'  */
#line 1185 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.map), false, false, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 4673 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 154: /* rel_pattern: '<' '-' '[' variable_opt ':' BQIDENT varlen_range_opt properties_opt ']' '-' '>'  */
#line 1191 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.map), false, false, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 4683 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 155: /* rel_pattern: '<' '-' '[' variable_opt ':' non_reserved_kw varlen_range_opt properties_opt ']' '-' '>'  */
#line 1197 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.map), false, false, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 4693 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 156: /* rel_pattern: '<' '-' '[' variable_opt ':' rel_type_list varlen_range_opt properties_opt ']' '-' '>'  */
#line 1203 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            cypher_rel_pattern *p = make_rel_pattern_multi_type((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.map), false, false);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string));
            if (p) p->varlen = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.varlen_range);
            ((*yyvalp).rel_pattern) = p;
        }
#line 4704 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 157: /* rel_pattern: '-' '[' variable_opt varlen_range_opt properties_opt ']' '-'  */
#line 1211 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), NULL, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.map), false, false, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string));
        }
#line 4713 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 158: /* rel_pattern: '-' '[' variable_opt ':' IDENTIFIER varlen_range_opt properties_opt ']' '-'  */
#line 1216 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.map), false, false, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string));
        }
#line 4723 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 159: /* rel_pattern: '-' '[' variable_opt ':' BQIDENT varlen_range_opt properties_opt ']' '-'  */
#line 1222 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.map), false, false, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string));
        }
#line 4733 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 160: /* rel_pattern: '-' '[' variable_opt ':' non_reserved_kw varlen_range_opt properties_opt ']' '-'  */
#line 1228 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* T-0326: keyword-named rel types (CONTAINS/STARTS/ENDS/etc.). */
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.map), false, false, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.varlen_range));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string));
        }
#line 4744 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 161: /* rel_pattern: '-' '[' variable_opt ':' rel_type_list varlen_range_opt properties_opt ']' '-'  */
#line 1235 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            cypher_rel_pattern *p = make_rel_pattern_multi_type((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.map), false, false);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string));
            if (p) p->varlen = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.varlen_range);
            ((*yyvalp).rel_pattern) = p;
        }
#line 4755 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 162: /* rel_pattern: '-' '-' '>'  */
#line 1243 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen(NULL, NULL, NULL, false, true, NULL);
        }
#line 4763 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 163: /* rel_pattern: '<' '-' '-'  */
#line 1247 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen(NULL, NULL, NULL, true, false, NULL);
        }
#line 4771 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 164: /* rel_pattern: '<' '-' '-' '>'  */
#line 1251 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Bidirectional bare arrow — equivalent to `--` (any direction).
             * openCypher accepts <--> as an explicit bidirectional form. */
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen(NULL, NULL, NULL, false, false, NULL);
        }
#line 4781 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 165: /* rel_pattern: '-' '-'  */
#line 1257 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).rel_pattern) = make_rel_pattern_varlen(NULL, NULL, NULL, false, false, NULL);
        }
#line 4789 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 166: /* variable_opt: %empty  */
#line 1263 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = NULL; }
#line 4795 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 167: /* variable_opt: var_name  */
#line 1264 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string); }
#line 4801 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 168: /* variable_opt: BQIDENT  */
#line 1265 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string); }
#line 4807 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 169: /* variable_opt: END_P  */
#line 1266 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = strdup("end"); }
#line 4813 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 170: /* varlen_range_opt: %empty  */
#line 1272 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        { ((*yyvalp).varlen_range) = NULL; }
#line 4819 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 171: /* varlen_range_opt: '*'  */
#line 1274 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        { ((*yyvalp).varlen_range) = make_varlen_range(1, -1); }
#line 4825 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 172: /* varlen_range_opt: '*' INTEGER  */
#line 1276 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        { ((*yyvalp).varlen_range) = make_varlen_range((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.integer), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.integer)); }
#line 4831 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 173: /* varlen_range_opt: '*' INTEGER DOT_DOT INTEGER  */
#line 1278 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        { ((*yyvalp).varlen_range) = make_varlen_range((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.integer), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.integer)); }
#line 4837 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 174: /* varlen_range_opt: '*' INTEGER DOT_DOT  */
#line 1280 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        { ((*yyvalp).varlen_range) = make_varlen_range((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.integer), -1); }
#line 4843 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 175: /* varlen_range_opt: '*' DOT_DOT INTEGER  */
#line 1282 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        { ((*yyvalp).varlen_range) = make_varlen_range(1, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.integer)); }
#line 4849 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 176: /* varlen_range_opt: '*' DOT_DOT  */
#line 1284 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        { ((*yyvalp).varlen_range) = make_varlen_range(1, -1); }
#line 4855 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 177: /* varlen_range_opt: '*' '-' INTEGER  */
#line 1286 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* `[*-2]` (Match4 [10]) */
            free(context->error_message);
            context->error_message = strdup("SyntaxError: InvalidRelationshipPattern: a variable-length bound cannot be negative");
            context->has_error = true;
            context->error_location = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line;
            context->error_column = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_column;
            ((*yyvalp).varlen_range) = NULL;
            YYERROR;
        }
#line 4870 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 178: /* label_opt: %empty  */
#line 1299 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).list) = NULL; }
#line 4876 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 179: /* label_opt: label_list  */
#line 1300 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list); }
#line 4882 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 180: /* label_opt: ':' label_alt_list  */
#line 1302 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* `:A|B`: one AST_NODE_LIST item; make_node_pattern moves it to
             * label_alternatives (GQLITE-T-0125) */
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)make_list((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line));
        }
#line 4893 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 181: /* label_name: IDENTIFIER  */
#line 1312 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).string) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string); }
#line 4899 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 182: /* label_name: BQIDENT  */
#line 1313 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).string) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string); }
#line 4905 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 183: /* label_name: non_reserved_kw  */
#line 1314 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).string) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string); }
#line 4911 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 184: /* label_alt_list: label_name '|' label_name  */
#line 1319 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line));
            ast_list_append(((*yyvalp).list), (ast_node*)make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string)); free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4922 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 185: /* label_alt_list: label_alt_list '|' label_name  */
#line 1326 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (ast_node*)make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
        }
#line 4932 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 186: /* label_list: ':' IDENTIFIER  */
#line 1335 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            cypher_literal *label = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append(((*yyvalp).list), (ast_node*)label);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4943 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 187: /* label_list: ':' BQIDENT  */
#line 1342 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            cypher_literal *label = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append(((*yyvalp).list), (ast_node*)label);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4954 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 188: /* label_list: ':' non_reserved_kw  */
#line 1349 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            cypher_literal *label = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append(((*yyvalp).list), (ast_node*)label);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4965 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 189: /* label_list: label_list ':' IDENTIFIER  */
#line 1356 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
            cypher_literal *label = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append(((*yyvalp).list), (ast_node*)label);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4976 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 190: /* label_list: label_list ':' non_reserved_kw  */
#line 1363 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
            cypher_literal *label = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append(((*yyvalp).list), (ast_node*)label);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4987 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 191: /* label_list: label_list ':' BQIDENT  */
#line 1370 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
            cypher_literal *label = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append(((*yyvalp).list), (ast_node*)label);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 4998 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 192: /* rel_type_list: IDENTIFIER '|' IDENTIFIER  */
#line 1380 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            cypher_literal *type_lit1 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            cypher_literal *type_lit3 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit1);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit3);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5012 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 193: /* rel_type_list: IDENTIFIER '|' ':' IDENTIFIER  */
#line 1390 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Support [:TYPE1|:TYPE2] syntax with colon before second type */
            ((*yyvalp).list) = ast_list_create();
            cypher_literal *type_lit1 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
            cypher_literal *type_lit4 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit1);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit4);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5027 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 194: /* rel_type_list: IDENTIFIER '|' BQIDENT  */
#line 1401 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            cypher_literal *type_lit1 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            cypher_literal *type_lit3 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit1);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit3);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5041 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 195: /* rel_type_list: IDENTIFIER '|' ':' BQIDENT  */
#line 1411 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Support [:TYPE1|:`backtick-type`] syntax */
            ((*yyvalp).list) = ast_list_create();
            cypher_literal *type_lit1 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
            cypher_literal *type_lit4 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit1);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit4);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5056 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 196: /* rel_type_list: BQIDENT '|' IDENTIFIER  */
#line 1422 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            cypher_literal *type_lit1 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            cypher_literal *type_lit3 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit1);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit3);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5070 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 197: /* rel_type_list: BQIDENT '|' ':' IDENTIFIER  */
#line 1432 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Support [:`backtick-type`|:TYPE2] syntax */
            ((*yyvalp).list) = ast_list_create();
            cypher_literal *type_lit1 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
            cypher_literal *type_lit4 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit1);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit4);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5085 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 198: /* rel_type_list: BQIDENT '|' BQIDENT  */
#line 1443 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            cypher_literal *type_lit1 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            cypher_literal *type_lit3 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit1);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit3);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5099 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 199: /* rel_type_list: BQIDENT '|' ':' BQIDENT  */
#line 1453 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Support [:`backtick-type`|:`backtick-type2`] syntax */
            ((*yyvalp).list) = ast_list_create();
            cypher_literal *type_lit1 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
            cypher_literal *type_lit4 = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit1);
            ast_list_append(((*yyvalp).list), (ast_node*)type_lit4);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5114 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 200: /* rel_type_list: rel_type_list '|' IDENTIFIER  */
#line 1464 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            cypher_literal *type_lit = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (ast_node*)type_lit);
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5125 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 201: /* rel_type_list: rel_type_list '|' BQIDENT  */
#line 1471 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            cypher_literal *type_lit = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (ast_node*)type_lit);
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5136 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 202: /* rel_type_list: rel_type_list '|' ':' IDENTIFIER  */
#line 1478 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            cypher_literal *type_lit = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.list), (ast_node*)type_lit);
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.list);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5147 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 203: /* rel_type_list: rel_type_list '|' ':' BQIDENT  */
#line 1485 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            cypher_literal *type_lit = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.list), (ast_node*)type_lit);
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.list);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5158 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 204: /* expr: primary_expr  */
#line 1495 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 5164 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 205: /* expr: '+' expr  */
#line 1496 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                                 { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 5170 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 206: /* expr: '-' expr  */
#line 1497 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                                  { 
        /* Handle unary minus */
        if ((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node)->type == AST_NODE_LITERAL) {
            cypher_literal *lit = (cypher_literal*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node);
            if (lit->literal_type == LITERAL_INTEGER) {
                if (lit->value.integer == INT64_MIN) {
                    /* -2^63: the scanner's pending INT64_MIN */
                    if (context && context->scanner && context->scanner->int_min_pending > 0)
                        context->scanner->int_min_pending--;
                } else {
                    lit->value.integer = -lit->value.integer;
                }
                ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node);
            } else if (lit->literal_type == LITERAL_DECIMAL) {
                lit->value.decimal = -lit->value.decimal;
                ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node);
            } else {
                /* Non-numeric literal: negate as 0 - lit. */
                ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_SUB,
                        (ast_node*)make_integer_literal(0, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line);
            }
        } else {
            /* Non-literal operand (e.g. -3^2, -x, -(a+b)): build a real
             * negation as 0 - expr; previously the negation was silently
             * dropped. (Precedence2 [4]/[5].) */
            ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_SUB,
                    (ast_node*)make_integer_literal(0, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line);
        }
    }
#line 5204 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 207: /* expr: expr '+' expr  */
#line 1526 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_ADD, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5210 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 208: /* expr: expr '-' expr  */
#line 1527 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_SUB, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5216 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 209: /* expr: expr '*' expr  */
#line 1528 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_MUL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5222 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 210: /* expr: expr '/' expr  */
#line 1529 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_DIV, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5228 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 211: /* expr: expr '%' expr  */
#line 1530 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_MOD, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5234 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 212: /* expr: expr '^' expr  */
#line 1531 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_POW, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5240 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 213: /* expr: expr '=' expr  */
#line 1532 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_EQ, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5246 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 214: /* expr: expr NOT_EQ expr  */
#line 1533 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_NEQ, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5252 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 215: /* expr: expr '<' expr  */
#line 1536 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_LT, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5258 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 216: /* expr: expr '>' expr  */
#line 1537 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_GT, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5264 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 217: /* expr: expr LT_EQ expr  */
#line 1538 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_LTE, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5270 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 218: /* expr: expr GT_EQ expr  */
#line 1539 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_GTE, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5276 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 219: /* expr: expr REGEX_MATCH expr  */
#line 1540 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                            { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_REGEX_MATCH, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5282 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 220: /* expr: expr AND expr  */
#line 1541 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_AND, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5288 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 221: /* expr: expr OR expr  */
#line 1542 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_OR, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5294 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 222: /* expr: expr XOR expr  */
#line 1543 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_XOR, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5300 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 223: /* expr: expr IN expr  */
#line 1544 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_IN, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5306 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 224: /* expr: expr STARTS WITH expr  */
#line 1545 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                                         { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_STARTS_WITH, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line); }
#line 5312 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 225: /* expr: expr ENDS WITH expr  */
#line 1546 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_ENDS_WITH, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line); }
#line 5318 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 226: /* expr: expr CONTAINS expr  */
#line 1547 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                                        { ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_CONTAINS, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5324 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 227: /* expr: NOT expr  */
#line 1548 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)make_not_expr((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5330 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 228: /* expr: expr IS NULL_P  */
#line 1549 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                          { ((*yyvalp).node) = (ast_node*)make_null_check((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), false, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line); }
#line 5336 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 229: /* expr: expr IS NOT NULL_P  */
#line 1550 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                          { ((*yyvalp).node) = (ast_node*)make_null_check((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), true, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line); }
#line 5342 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 230: /* expr: '(' expr ')'  */
#line 1551 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node); if (((*yyvalp).node)) ((*yyvalp).node)->parenthesized = true; }
#line 5348 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 231: /* expr: node_pattern rel_pattern node_pattern  */
#line 1558 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Build a path from the pattern elements */
            ast_list *elements = ast_list_create();
            ast_list_append(elements, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node_pattern));
            ast_list_append(elements, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.rel_pattern));
            ast_list_append(elements, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node_pattern));
            cypher_path *path = make_path(elements);
            /* Wrap in pattern list and create EXISTS expression */
            ast_list *pattern_list = ast_list_create();
            ast_list_append(pattern_list, (ast_node*)path);
            ((*yyvalp).node) = (ast_node*)make_exists_pattern_expr(pattern_list, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
        }
#line 5365 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 232: /* expr: node_pattern rel_pattern node_pattern rel_pattern node_pattern  */
#line 1571 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Chained pattern: (a)-[r1]->(b)-[r2]->(c) */
            ast_list *elements = ast_list_create();
            ast_list_append(elements, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.node_pattern));
            ast_list_append(elements, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.rel_pattern));
            ast_list_append(elements, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node_pattern));
            ast_list_append(elements, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.rel_pattern));
            ast_list_append(elements, (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node_pattern));
            cypher_path *path = make_path(elements);
            ast_list *pattern_list = ast_list_create();
            ast_list_append(pattern_list, (ast_node*)path);
            ((*yyvalp).node) = (ast_node*)make_exists_pattern_expr(pattern_list, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
        }
#line 5383 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 233: /* expr: expr '.' IDENTIFIER  */
#line 1585 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_property((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5392 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 234: /* expr: expr '.' BQIDENT  */
#line 1590 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_property((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5401 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 235: /* expr: expr '.' non_reserved_kw  */
#line 1595 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_property((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5410 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 236: /* expr: expr '[' expr ']'  */
#line 1600 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_subscript((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
        }
#line 5418 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 237: /* expr: expr '[' expr DOT_DOT expr ']'  */
#line 1604 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_slice((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
        }
#line 5426 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 238: /* expr: expr '[' expr DOT_DOT ']'  */
#line 1608 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_slice((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
        }
#line 5434 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 239: /* expr: expr '[' DOT_DOT expr ']'  */
#line 1612 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_slice((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.node), NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
        }
#line 5442 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 240: /* primary_expr: literal_expr  */
#line 1618 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 5448 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 241: /* primary_expr: identifier  */
#line 1619 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.identifier); }
#line 5454 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 242: /* primary_expr: parameter  */
#line 1620 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.parameter); }
#line 5460 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 243: /* primary_expr: function_call  */
#line 1621 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 5466 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 244: /* primary_expr: list_predicate  */
#line 1622 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 5472 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 245: /* primary_expr: reduce_expr  */
#line 1623 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 5478 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 246: /* primary_expr: list_literal  */
#line 1624 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 5484 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 247: /* primary_expr: list_comprehension  */
#line 1625 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                         { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 5490 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 248: /* primary_expr: pattern_comprehension  */
#line 1626 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                            { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 5496 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 249: /* primary_expr: map_literal  */
#line 1627 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 5502 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 250: /* primary_expr: map_projection  */
#line 1628 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 5508 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 251: /* primary_expr: case_expression  */
#line 1629 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node); }
#line 5514 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 252: /* primary_expr: var_name ':' label_name  */
#line 1631 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            cypher_identifier *base = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            ((*yyvalp).node) = (ast_node*)make_label_expr((ast_node*)base, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5525 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 253: /* primary_expr: var_name ':' label_alt_list  */
#line 1638 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Label disjunction as an expression `n:A|B` (GQLITE-T-0485):
             * (n:A) OR (n:B). */
            ast_node *acc = NULL;
            for (int li = 0; li < (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list)->count; li++) {
                cypher_literal *lit = (cypher_literal *)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list)->items[li];
                cypher_identifier *b = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
                ast_node *one = (ast_node *)make_label_expr((ast_node *)b, lit->value.string, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
                acc = acc ? (ast_node *)make_binary_op(BINARY_OP_OR, acc, one, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line) : one;
            }
            if (acc) acc->parenthesized = true;
            ast_list_free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.list));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            ((*yyvalp).node) = acc;
        }
#line 5545 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 254: /* primary_expr: var_name ':' label_name ':' label_name  */
#line 1654 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Conjunctive label expression `n:L1:L2` — emit
             * (n:L1) AND (n:L2). */
            cypher_identifier *b1 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
            cypher_identifier *b2 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
            cypher_label_expr *l1 = make_label_expr((ast_node*)b1, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            cypher_label_expr *l2 = make_label_expr((ast_node*)b2, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_AND,
                                            (ast_node*)l1, (ast_node*)l2,
                                            (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string)); free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string)); free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5562 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 255: /* primary_expr: var_name ':' label_name ':' label_name ':' label_name  */
#line 1667 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Three-label conjunction. */
            cypher_identifier *b1 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
            cypher_identifier *b2 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
            cypher_identifier *b3 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
            cypher_label_expr *l1 = make_label_expr((ast_node*)b1, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
            cypher_label_expr *l2 = make_label_expr((ast_node*)b2, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            cypher_label_expr *l3 = make_label_expr((ast_node*)b3, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            cypher_binary_op *and12 = make_binary_op(BINARY_OP_AND,
                                                     (ast_node*)l1, (ast_node*)l2,
                                                     (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
            ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_AND,
                                            (ast_node*)and12, (ast_node*)l3,
                                            (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string)); free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string)); free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string)); free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5583 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 256: /* primary_expr: var_name ':' label_name ':' label_name ':' label_name ':' label_name  */
#line 1684 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Four-label conjunction (Graph5 [4], e.g. `a:C:A:A:C`).
             * Repeated labels are harmless — each conjunct is an
             * independent EXISTS check. */
            cypher_identifier *b1 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yyloc).first_line);
            cypher_identifier *b2 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yyloc).first_line);
            cypher_identifier *b3 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yyloc).first_line);
            cypher_identifier *b4 = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yyloc).first_line);
            cypher_label_expr *l1 = make_label_expr((ast_node*)b1, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
            cypher_label_expr *l2 = make_label_expr((ast_node*)b2, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
            cypher_label_expr *l3 = make_label_expr((ast_node*)b3, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            cypher_label_expr *l4 = make_label_expr((ast_node*)b4, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            cypher_binary_op *and12 = make_binary_op(BINARY_OP_AND,
                                                     (ast_node*)l1, (ast_node*)l2,
                                                     (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yyloc).first_line);
            cypher_binary_op *and123 = make_binary_op(BINARY_OP_AND,
                                                      (ast_node*)and12, (ast_node*)l3,
                                                      (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yyloc).first_line);
            ((*yyvalp).node) = (ast_node*)make_binary_op(BINARY_OP_AND,
                                            (ast_node*)and123, (ast_node*)l4,
                                            (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yysemantics.yyval.string)); free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.string)); free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string)); free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string)); free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 5611 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 257: /* literal_expr: literal  */
#line 1710 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).node) = (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.literal); }
#line 5617 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 258: /* function_call: IDENTIFIER '.' IDENTIFIER '(' argument_list ')'  */
#line 1718 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            size_t len = strlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string)) + 1 + strlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string)) + 1;
            char *qname = malloc(len);
            if (qname) snprintf(qname, len, "%s.%s", (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
            ((*yyvalp).node) = (ast_node*)make_function_call(qname ? qname : (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), false, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
            if (qname) free(qname);
        }
#line 5631 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 259: /* function_call: IDENTIFIER '.' IDENTIFIER '(' ')'  */
#line 1728 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            size_t len = strlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string)) + 1 + strlen((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string)) + 1;
            char *qname = malloc(len);
            if (qname) snprintf(qname, len, "%s.%s", (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            ast_list *args = ast_list_create();
            ((*yyvalp).node) = (ast_node*)make_function_call(qname ? qname : (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), args, false, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            if (qname) free(qname);
        }
#line 5646 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 260: /* function_call: IDENTIFIER '(' ')'  */
#line 1739 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Check if this is EXISTS function call */
            if (strcasecmp((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), "exists") == 0) {
                /* Empty EXISTS() - invalid */
                free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
                cypher_yyerror(&(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc), context, "EXISTS requires an argument");
                YYERROR;
            } else {
                ast_list *args = ast_list_create();
                ((*yyvalp).node) = (ast_node*)make_function_call((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), args, false, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
                free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
            }
        }
#line 5664 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 261: /* function_call: IDENTIFIER '(' argument_list ')'  */
#line 1754 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Check if this is EXISTS function call */
            if (strcasecmp((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), "exists") == 0) {
                /* EXISTS with argument list - check first arg */
                if ((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list)->count == 1 && (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list)->items[0] != NULL) {
                    ast_node *arg = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list)->items[0];
                    if (arg->type == AST_NODE_PROPERTY) {
                        ((*yyvalp).node) = (ast_node*)make_exists_property_expr(arg, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
                        (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list)->items[0] = NULL;  /* Transfer ownership */
                        ast_list_free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list));
                    } else {
                        ((*yyvalp).node) = (ast_node*)make_exists_pattern_expr((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
                    }
                } else {
                    cypher_yyerror(&(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc), context, "EXISTS requires exactly one argument");
                    ast_list_free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list));
                    free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
                    YYERROR;
                }
                free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
            } else {
                ((*yyvalp).node) = (ast_node*)make_function_call((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), false, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
                free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
            }
        }
#line 5694 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 262: /* function_call: IDENTIFIER '(' DISTINCT expr ')'  */
#line 1780 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list *args = ast_list_create();
            ast_list_append(args, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node));
            ((*yyvalp).node) = (ast_node*)make_function_call((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string), args, true, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.string));
        }
#line 5705 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 263: /* function_call: IDENTIFIER '(' '*' ')'  */
#line 1787 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list *args = ast_list_create();
            /* For count(*), we'll use a special NULL argument to indicate * */
            ast_list_append(args, NULL);
            ((*yyvalp).node) = (ast_node*)make_function_call((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), args, false, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
        }
#line 5717 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 264: /* function_call: EXISTS '(' pattern_list ')'  */
#line 1795 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* EXISTS((pattern)) - check for relationship/path existence */
            ((*yyvalp).node) = (ast_node*)make_exists_pattern_expr((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
        }
#line 5726 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 265: /* function_call: EXISTS '{' pattern_list '}'  */
#line 1800 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Existential subquery brace form: EXISTS { (n)-->() }
             * (ExistentialSubquery1 [1]/[3]). Reuses the pattern-existence
             * transform; correlated outer variables resolve via the
             * EXISTS_TYPE_PATTERN emitter's outer-alias lookup. */
            cypher_exists_expr *ee = make_exists_pattern_expr((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
            if (ee) ee->is_subquery = true;
            ((*yyvalp).node) = (ast_node*)ee;
        }
#line 5740 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 266: /* function_call: EXISTS '{' single_query '}'  */
#line 1810 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* GQLITE-T-0139: full existential subquery
             * EXISTS { MATCH (n)-->(m) [WITH ... WHERE ...] RETURN true }
             * (CIP2015-05-13-EXISTS level 3; ExistentialSubquery2/3). */
            ((*yyvalp).node) = (ast_node*)make_exists_query_expr((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
        }
#line 5751 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 267: /* function_call: EXISTS '{' pattern_list WHERE expr '}'  */
#line 1817 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Existential subquery with inner WHERE:
             * EXISTS { (n)-->(m) WHERE n.prop = m.prop }
             * (ExistentialSubquery1 [2]/[4]). The inner predicate is stored
             * on where_clause and folded into the subquery by the transform,
             * which registers the inner pattern variables first. */
            cypher_exists_expr *ee = make_exists_pattern_expr((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yyloc).first_line);
            if (ee) { ee->where_clause = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node); ee->is_subquery = true; }
            ((*yyvalp).node) = (ast_node*)ee;
        }
#line 5766 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 268: /* function_call: IDENTIFIER '{' pattern_list '}'  */
#line 1828 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* COUNT { (n)-->() } (GQLITE-T-0485): the number of matches. */
            ((*yyvalp).node) = make_count_subquery(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), NULL, NULL, &(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc));
            if (!((*yyvalp).node)) YYERROR;
        }
#line 5776 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 269: /* function_call: IDENTIFIER '{' pattern_list WHERE expr '}'  */
#line 1834 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = make_count_subquery(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), NULL, &(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yyloc));
            if (!((*yyvalp).node)) YYERROR;
        }
#line 5785 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 270: /* function_call: IDENTIFIER '{' single_query '}'  */
#line 1839 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* COUNT { MATCH ... [RETURN ...] }: the number of rows. */
            ((*yyvalp).node) = make_count_subquery(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), NULL, NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), &(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc));
            if (!((*yyvalp).node)) YYERROR;
        }
#line 5795 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 271: /* function_call: EXISTS '(' IDENTIFIER '.' IDENTIFIER ')'  */
#line 1845 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* EXISTS(n.property) - unambiguous property existence check */
            ast_node *var = (ast_node*)make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
            ast_node *prop = (ast_node*)make_property(var, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yyloc).first_line);
            ((*yyvalp).node) = (ast_node*)make_exists_property_expr(prop, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.string));
        }
#line 5808 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 272: /* function_call: CONTAINS '(' argument_list ')'  */
#line 1855 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_function_call("contains", (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), false, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
        }
#line 5816 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 273: /* function_call: STARTS '(' argument_list ')'  */
#line 1859 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* startsWith function uses STARTS keyword */
            ((*yyvalp).node) = (ast_node*)make_function_call("startsWith", (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), false, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
        }
#line 5825 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 274: /* function_call: ENDS '(' argument_list ')'  */
#line 1864 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* endsWith function uses ENDS keyword */
            ((*yyvalp).node) = (ast_node*)make_function_call("endsWith", (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), false, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
        }
#line 5834 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 275: /* list_predicate: ALL '(' var_name IN expr WHERE expr ')'  */
#line 1873 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_list_predicate(LIST_PRED_ALL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 5843 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 276: /* list_predicate: ANY '(' var_name IN expr WHERE expr ')'  */
#line 1878 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_list_predicate(LIST_PRED_ANY, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 5852 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 277: /* list_predicate: NONE '(' var_name IN expr WHERE expr ')'  */
#line 1883 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_list_predicate(LIST_PRED_NONE, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 5861 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 278: /* list_predicate: SINGLE '(' var_name IN expr WHERE expr ')'  */
#line 1888 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_list_predicate(LIST_PRED_SINGLE, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 5870 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 279: /* reduce_expr: REDUCE '(' var_name '=' expr ',' var_name IN expr '|' expr ')'  */
#line 1897 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* reduce(acc = initial, x IN list | expression) */
            ((*yyvalp).node) = (ast_node*)make_reduce_expr((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-9)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-11)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-9)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 5881 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 280: /* argument_list: expr  */
#line 1908 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
        }
#line 5890 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 281: /* argument_list: argument_list ',' expr  */
#line 1913 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
        }
#line 5899 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 282: /* list_literal: '[' ']'  */
#line 1922 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_list(ast_list_create(), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line);
        }
#line 5907 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 283: /* list_literal: '[' return_item_list ']'  */
#line 1926 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Reuse return_item_list for comma-separated expressions */
            /* But we need to extract the expressions from return_items */
            ast_list *exprs = ast_list_create();
            for (int i = 0; i < (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list)->count; i++) {
                cypher_return_item *item = (cypher_return_item*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list)->items[i];
                ast_list_append(exprs, item->expr);
                item->expr = NULL;  /* Prevent double-free */
            }
            ast_list_free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list));
            ((*yyvalp).node) = (ast_node*)make_list(exprs, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
        }
#line 5924 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 284: /* list_comprehension: '[' var_name IN expr ']'  */
#line 1946 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_list_comprehension((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), NULL, NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
        }
#line 5933 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 285: /* list_comprehension: '[' var_name IN expr WHERE expr ']'  */
#line 1951 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_list_comprehension((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 5942 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 286: /* list_comprehension: '[' var_name IN expr '|' expr ']'  */
#line 1956 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_list_comprehension((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.string));
        }
#line 5951 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 287: /* list_comprehension: '[' var_name IN expr WHERE expr '|' expr ']'  */
#line 1961 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_list_comprehension((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string));
        }
#line 5960 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 288: /* pattern_comprehension: '[' '(' variable_opt label_opt properties_opt ')' comp_steps '|' expr ']'  */
#line 1978 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_pattern_comprehension(
                comp_pattern(make_node_pattern((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.map)), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.list)), NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-9)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string));
        }
#line 5970 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 289: /* pattern_comprehension: '[' '(' variable_opt label_opt properties_opt ')' comp_steps WHERE expr '|' expr ']'  */
#line 1984 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_pattern_comprehension(
                comp_pattern(make_node_pattern((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-9)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.map)), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.list)), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-11)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-9)].yystate.yysemantics.yyval.string));
        }
#line 5980 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 290: /* pattern_comprehension: '[' var_name '=' '(' variable_opt label_opt properties_opt ')' comp_steps '|' expr ']'  */
#line 1991 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            cypher_pattern_comprehension *pc = make_pattern_comprehension(
                comp_pattern(make_node_pattern((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-6)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.map)), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.list)), NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-11)].yystate.yyloc).first_line);
            pc->path_var = strdup((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-10)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-10)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.string));
            ((*yyvalp).node) = (ast_node*)pc;
        }
#line 5993 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 291: /* pattern_comprehension: '[' var_name '=' '(' variable_opt label_opt properties_opt ')' comp_steps WHERE expr '|' expr ']'  */
#line 2000 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            cypher_pattern_comprehension *pc = make_pattern_comprehension(
                comp_pattern(make_node_pattern((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-9)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-8)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-7)].yystate.yysemantics.yyval.map)), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yysemantics.yyval.list)), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-13)].yystate.yyloc).first_line);
            pc->path_var = strdup((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-12)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-12)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-9)].yystate.yysemantics.yyval.string));
            ((*yyvalp).node) = (ast_node*)pc;
        }
#line 6006 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 292: /* comp_steps: rel_pattern node_pattern  */
#line 2014 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.rel_pattern));
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node_pattern));
        }
#line 6016 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 293: /* comp_steps: comp_steps rel_pattern node_pattern  */
#line 2020 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.rel_pattern));
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node_pattern));
        }
#line 6026 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 294: /* map_literal: '{' '}'  */
#line 2030 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_map(ast_list_create(), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line);
        }
#line 6034 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 295: /* map_literal: '{' map_pair_list '}'  */
#line 2034 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_map((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
        }
#line 6042 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 296: /* map_projection: IDENTIFIER '{' map_projection_list '}'  */
#line 2042 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            cypher_identifier *base = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
            ((*yyvalp).node) = (ast_node*)make_map_projection((ast_node*)base, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
        }
#line 6052 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 297: /* map_projection_list: map_projection_item  */
#line 2051 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
        }
#line 6061 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 298: /* map_projection_list: map_projection_list ',' map_projection_item  */
#line 2056 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
        }
#line 6070 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 299: /* map_projection_item: '.' IDENTIFIER  */
#line 2064 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Shorthand: .prop -> key=prop, property=prop, expr=NULL */
            ((*yyvalp).node) = (ast_node*)make_map_projection_item((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 6080 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 300: /* map_projection_item: '.' '*'  */
#line 2070 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* All properties: .* -> key=NULL, property="*", expr=NULL */
            ((*yyvalp).node) = (ast_node*)make_map_projection_item(NULL, "*", NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line);
        }
#line 6089 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 301: /* map_projection_item: IDENTIFIER ':' '.' IDENTIFIER  */
#line 2075 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Aliased property: alias: .prop */
            ((*yyvalp).node) = (ast_node*)make_map_projection_item((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.string));
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 6100 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 302: /* map_projection_item: IDENTIFIER ':' expr  */
#line 2082 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Computed value: alias: expr */
            ((*yyvalp).node) = (ast_node*)make_map_projection_item((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
        }
#line 6110 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 303: /* map_projection_item: IDENTIFIER  */
#line 2088 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Variable selector: `m {x}` is `m {x: x}` (GQLITE-T-0136) */
            ((*yyvalp).node) = (ast_node*)make_map_projection_item((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), NULL,
                     (ast_node*)make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 6121 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 304: /* case_expression: CASE when_clause_list END_P  */
#line 2103 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_case_expr(NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
        }
#line 6129 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 305: /* case_expression: CASE when_clause_list ELSE expr END_P  */
#line 2107 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_case_expr(NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yyloc).first_line);
        }
#line 6137 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 306: /* case_expression: CASE expr when_clause_list END_P  */
#line 2112 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_case_expr((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), NULL, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
        }
#line 6145 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 307: /* case_expression: CASE expr when_clause_list ELSE expr END_P  */
#line 2116 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_case_expr((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-4)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-5)].yystate.yyloc).first_line);
        }
#line 6153 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 308: /* when_clause_list: when_clause  */
#line 2123 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
        }
#line 6162 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 309: /* when_clause_list: when_clause_list when_clause  */
#line 2128 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list);
        }
#line 6171 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 310: /* when_clause: WHEN expr THEN expr  */
#line 2136 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).node) = (ast_node*)make_when_clause((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-3)].yystate.yyloc).first_line);
        }
#line 6179 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 311: /* literal: INTEGER  */
#line 2143 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).literal) = make_integer_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.integer), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
        }
#line 6187 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 312: /* literal: DECIMAL  */
#line 2147 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).literal) = make_decimal_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.decimal), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
        }
#line 6195 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 313: /* literal: STRING  */
#line 2151 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).literal) = make_string_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 6204 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 314: /* literal: TRUE_P  */
#line 2156 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).literal) = make_boolean_literal(true, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
        }
#line 6212 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 315: /* literal: FALSE_P  */
#line 2160 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).literal) = make_boolean_literal(false, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
        }
#line 6220 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 316: /* literal: NULL_P  */
#line 2164 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).literal) = make_null_literal((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
        }
#line 6228 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 317: /* identifier: IDENTIFIER  */
#line 2171 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).identifier) = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 6237 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 318: /* identifier: soft_kw  */
#line 2176 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* A non-reserved keyword used as a variable (GQLITE-T-0440). */
            ((*yyvalp).identifier) = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 6247 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 319: /* identifier: BQIDENT  */
#line 2182 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).identifier) = make_identifier((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 6256 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 320: /* identifier: END_P  */
#line 2187 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* Allow 'end' as an identifier - it's only reserved in CASE...END context */
            ((*yyvalp).identifier) = make_identifier("end", (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
        }
#line 6265 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 321: /* soft_kw: YIELD  */
#line 2199 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "YIELD"); }
#line 6271 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 322: /* soft_kw: FROM  */
#line 2200 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "FROM"); }
#line 6277 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 323: /* soft_kw: CSV  */
#line 2201 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "CSV"); }
#line 6283 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 324: /* soft_kw: LOAD  */
#line 2202 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "LOAD"); }
#line 6289 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 325: /* soft_kw: HEADERS  */
#line 2203 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "HEADERS"); }
#line 6295 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 326: /* soft_kw: FIELDTERMINATOR  */
#line 2204 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "FIELDTERMINATOR"); }
#line 6301 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 327: /* soft_kw: PATTERN  */
#line 2205 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "PATTERN"); }
#line 6307 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 328: /* soft_kw: EXPLAIN  */
#line 2206 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "EXPLAIN"); }
#line 6313 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 329: /* soft_kw: REDUCE  */
#line 2207 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "REDUCE"); }
#line 6319 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 330: /* soft_kw: ANY  */
#line 2208 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "ANY"); }
#line 6325 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 331: /* soft_kw: NONE  */
#line 2209 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "NONE"); }
#line 6331 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 332: /* soft_kw: SINGLE  */
#line 2210 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "SINGLE"); }
#line 6337 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 333: /* soft_kw: CALL  */
#line 2211 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "CALL"); }
#line 6343 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 334: /* soft_kw: FOREACH  */
#line 2212 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "FOREACH"); }
#line 6349 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 335: /* soft_kw: SHORTESTPATH  */
#line 2213 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "SHORTESTPATH"); }
#line 6355 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 336: /* soft_kw: ALLSHORTESTPATHS  */
#line 2214 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "ALLSHORTESTPATHS"); }
#line 6361 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 337: /* var_name: IDENTIFIER  */
#line 2218 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string); }
#line 6367 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 338: /* var_name: soft_kw  */
#line 2219 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string); }
#line 6373 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 339: /* non_reserved_kw: SINGLE  */
#line 2225 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "SINGLE"); }
#line 6379 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 340: /* non_reserved_kw: ANY  */
#line 2226 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "ANY"); }
#line 6385 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 341: /* non_reserved_kw: NONE  */
#line 2227 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "NONE"); }
#line 6391 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 342: /* non_reserved_kw: ALL  */
#line 2228 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "ALL"); }
#line 6397 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 343: /* non_reserved_kw: EXISTS  */
#line 2229 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "EXISTS"); }
#line 6403 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 344: /* non_reserved_kw: CONTAINS  */
#line 2230 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "CONTAINS"); }
#line 6409 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 345: /* non_reserved_kw: STARTS  */
#line 2231 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "STARTS"); }
#line 6415 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 346: /* non_reserved_kw: ENDS  */
#line 2232 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "ENDS"); }
#line 6421 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 347: /* non_reserved_kw: REDUCE  */
#line 2233 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "REDUCE"); }
#line 6427 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 348: /* non_reserved_kw: END_P  */
#line 2234 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "END"); }
#line 6433 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 349: /* non_reserved_kw: ON  */
#line 2235 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "ON"); }
#line 6439 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 350: /* non_reserved_kw: PATTERN  */
#line 2236 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "PATTERN"); }
#line 6445 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 351: /* non_reserved_kw: CSV  */
#line 2237 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "CSV"); }
#line 6451 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 352: /* non_reserved_kw: LOAD  */
#line 2238 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "LOAD"); }
#line 6457 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 353: /* non_reserved_kw: YIELD  */
#line 2239 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "YIELD"); }
#line 6463 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 354: /* non_reserved_kw: IN  */
#line 2240 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "IN"); }
#line 6469 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 355: /* non_reserved_kw: IS  */
#line 2241 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "IS"); }
#line 6475 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 356: /* non_reserved_kw: AS  */
#line 2242 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "AS"); }
#line 6481 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 357: /* non_reserved_kw: OR  */
#line 2243 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "OR"); }
#line 6487 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 358: /* non_reserved_kw: AND  */
#line 2244 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "AND"); }
#line 6493 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 359: /* non_reserved_kw: XOR  */
#line 2245 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "XOR"); }
#line 6499 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 360: /* non_reserved_kw: NOT  */
#line 2246 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "NOT"); }
#line 6505 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 361: /* non_reserved_kw: ORDER  */
#line 2247 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "ORDER"); }
#line 6511 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 362: /* non_reserved_kw: BY  */
#line 2248 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "BY"); }
#line 6517 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 363: /* non_reserved_kw: NULL_P  */
#line 2249 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "NULL"); }
#line 6523 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 364: /* non_reserved_kw: CASE  */
#line 2250 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "CASE"); }
#line 6529 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 365: /* non_reserved_kw: WHEN  */
#line 2251 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "WHEN"); }
#line 6535 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 366: /* non_reserved_kw: THEN  */
#line 2252 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "THEN"); }
#line 6541 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 367: /* non_reserved_kw: ELSE  */
#line 2253 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "ELSE"); }
#line 6547 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 368: /* non_reserved_kw: MATCH  */
#line 2254 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "MATCH"); }
#line 6553 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 369: /* non_reserved_kw: RETURN  */
#line 2255 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "RETURN"); }
#line 6559 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 370: /* non_reserved_kw: CREATE  */
#line 2256 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "CREATE"); }
#line 6565 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 371: /* non_reserved_kw: WHERE  */
#line 2257 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "WHERE"); }
#line 6571 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 372: /* non_reserved_kw: WITH  */
#line 2258 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "WITH"); }
#line 6577 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 373: /* non_reserved_kw: SET  */
#line 2259 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "SET"); }
#line 6583 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 374: /* non_reserved_kw: DELETE  */
#line 2260 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "DELETE"); }
#line 6589 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 375: /* non_reserved_kw: REMOVE  */
#line 2261 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "REMOVE"); }
#line 6595 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 376: /* non_reserved_kw: MERGE  */
#line 2262 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "MERGE"); }
#line 6601 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 377: /* non_reserved_kw: UNWIND  */
#line 2263 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "UNWIND"); }
#line 6607 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 378: /* non_reserved_kw: DETACH  */
#line 2264 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "DETACH"); }
#line 6613 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 379: /* non_reserved_kw: FOREACH  */
#line 2265 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "FOREACH"); }
#line 6619 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 380: /* non_reserved_kw: CALL  */
#line 2266 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "CALL"); }
#line 6625 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 381: /* non_reserved_kw: OPTIONAL  */
#line 2267 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "OPTIONAL"); }
#line 6631 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 382: /* non_reserved_kw: DISTINCT  */
#line 2268 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "DISTINCT"); }
#line 6637 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 383: /* non_reserved_kw: SKIP  */
#line 2269 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "SKIP"); }
#line 6643 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 384: /* non_reserved_kw: LIMIT  */
#line 2270 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "LIMIT"); }
#line 6649 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 385: /* non_reserved_kw: UNION  */
#line 2271 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "UNION"); }
#line 6655 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 386: /* non_reserved_kw: FROM  */
#line 2272 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "FROM"); }
#line 6661 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 387: /* non_reserved_kw: HEADERS  */
#line 2273 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "HEADERS"); }
#line 6667 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 388: /* non_reserved_kw: FIELDTERMINATOR  */
#line 2274 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                     { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "FIELDTERMINATOR"); }
#line 6673 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 389: /* non_reserved_kw: EXPLAIN  */
#line 2275 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "EXPLAIN"); }
#line 6679 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 390: /* non_reserved_kw: SHORTESTPATH  */
#line 2276 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                    { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "SHORTESTPATH"); }
#line 6685 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 391: /* non_reserved_kw: ALLSHORTESTPATHS  */
#line 2277 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                      { ((*yyvalp).string) = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_column, "ALLSHORTESTPATHS"); }
#line 6691 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 392: /* parameter: PARAMETER  */
#line 2282 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).parameter) = make_parameter((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.string));
        }
#line 6700 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 393: /* properties_opt: %empty  */
#line 2290 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        { ((*yyvalp).map) = NULL; }
#line 6706 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 394: /* properties_opt: '{' '}'  */
#line 2291 "lib/graphqlite/src/backend/parser/cypher_gram.y"
                        {
            /* Empty `{}` produces an empty-map node so downstream
             * validation distinguishes "no properties block written"
             * (NULL) from "explicit empty map" — the latter counts as
             * a property predicate for re-bind detection. */
            ((*yyvalp).map) = make_map(ast_list_create(), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yyloc).first_line);
        }
#line 6718 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 395: /* properties_opt: '{' map_pair_list '}'  */
#line 2299 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).map) = make_map((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-1)].yystate.yysemantics.yyval.list), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
        }
#line 6726 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 396: /* map_pair_list: map_pair  */
#line 2306 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).list) = ast_list_create();
            ast_list_append(((*yyvalp).list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.map_pair));
        }
#line 6735 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 397: /* map_pair_list: map_pair_list ',' map_pair  */
#line 2311 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ast_list_append((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list), (ast_node*)(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.map_pair));
            ((*yyvalp).list) = (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.list);
        }
#line 6744 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 398: /* map_pair: IDENTIFIER ':' expr  */
#line 2319 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).map_pair) = make_map_pair((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
        }
#line 6753 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 399: /* map_pair: BQIDENT ':' expr  */
#line 2324 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).map_pair) = make_map_pair((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
        }
#line 6762 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 400: /* map_pair: STRING ':' expr  */
#line 2329 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).map_pair) = make_map_pair((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
        }
#line 6771 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 401: /* map_pair: non_reserved_kw ':' expr  */
#line 2334 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).map_pair) = make_map_pair((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            free((YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yysemantics.yyval.string));
        }
#line 6780 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 402: /* map_pair: ASC ':' expr  */
#line 2339 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).map_pair) = make_map_pair("asc", (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
        }
#line 6788 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 403: /* map_pair: TRUE_P ':' expr  */
#line 2343 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            /* `{true: true}`: a keyword as a map key keeps its spelling (Boolean4 [4]). */
            char *k = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_column, "true");
            ((*yyvalp).map_pair) = make_map_pair(k, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            free(k);
        }
#line 6799 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 404: /* map_pair: FALSE_P ':' expr  */
#line 2350 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            char *k = keyword_text_at(context, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_column, "false");
            ((*yyvalp).map_pair) = make_map_pair(k, (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
            free(k);
        }
#line 6809 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;

  case 405: /* map_pair: DESC ':' expr  */
#line 2356 "lib/graphqlite/src/backend/parser/cypher_gram.y"
        {
            ((*yyvalp).map_pair) = make_map_pair("desc", (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (0)].yystate.yysemantics.yyval.node), (YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL (-2)].yystate.yyloc).first_line);
        }
#line 6817 "lib/graphqlite/generated/cypher_gram.tab.c"
    break;


#line 6821 "lib/graphqlite/generated/cypher_gram.tab.c"

      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yylhsNonterm (yyrule), yyvalp, yylocp);

  return yyok;
# undef yyerrok
# undef YYABORT
# undef YYACCEPT
# undef YYNOMEM
# undef YYERROR
# undef YYBACKUP
# undef yyclearin
# undef YYRECOVERING
}


static void
yyuserMerge (int yyn, YYSTYPE* yy0, YYSTYPE* yy1)
{
  YY_USE (yy0);
  YY_USE (yy1);

  switch (yyn)
    {

      default: break;
    }
}

                              /* Bison grammar-table manipulation.  */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, cypher_parser_context *context)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  YY_USE (context);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}

/** Number of symbols composing the right hand side of rule #RULE.  */
static inline int
yyrhsLength (yyRuleNum yyrule)
{
  return yyr2[yyrule];
}

static void
yydestroyGLRState (char const *yymsg, yyGLRState *yys, cypher_parser_context *context)
{
  if (yys->yyresolved)
    yydestruct (yymsg, yy_accessing_symbol (yys->yylrState),
                &yys->yysemantics.yyval, &yys->yyloc, context);
  else
    {
#if CYPHER_YYDEBUG
      if (yydebug)
        {
          if (yys->yysemantics.yyfirstVal)
            YY_FPRINTF ((stderr, "%s unresolved", yymsg));
          else
            YY_FPRINTF ((stderr, "%s incomplete", yymsg));
          YY_SYMBOL_PRINT ("", yy_accessing_symbol (yys->yylrState), YY_NULLPTR, &yys->yyloc);
        }
#endif

      if (yys->yysemantics.yyfirstVal)
        {
          yySemanticOption *yyoption = yys->yysemantics.yyfirstVal;
          yyGLRState *yyrh;
          int yyn;
          for (yyrh = yyoption->yystate, yyn = yyrhsLength (yyoption->yyrule);
               yyn > 0;
               yyrh = yyrh->yypred, yyn -= 1)
            yydestroyGLRState (yymsg, yyrh, context);
        }
    }
}

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

/** True iff LR state YYSTATE has only a default reduction (regardless
 *  of token).  */
static inline yybool
yyisDefaultedState (yy_state_t yystate)
{
  return yypact_value_is_default (yypact[yystate]);
}

/** The default reduction for YYSTATE, assuming it has one.  */
static inline yyRuleNum
yydefaultAction (yy_state_t yystate)
{
  return yydefact[yystate];
}

#define yytable_value_is_error(Yyn) \
  0

/** The action to take in YYSTATE on seeing YYTOKEN.
 *  Result R means
 *    R < 0:  Reduce on rule -R.
 *    R = 0:  Error.
 *    R > 0:  Shift to state R.
 *  Set *YYCONFLICTS to a pointer into yyconfl to a 0-terminated list
 *  of conflicting reductions.
 */
static inline int
yygetLRActions (yy_state_t yystate, yysymbol_kind_t yytoken, const short** yyconflicts)
{
  int yyindex = yypact[yystate] + yytoken;
  if (yytoken == YYSYMBOL_YYerror)
    {
      // This is the error token.
      *yyconflicts = yyconfl;
      return 0;
    }
  else if (yyisDefaultedState (yystate)
           || yyindex < 0 || YYLAST < yyindex || yycheck[yyindex] != yytoken)
    {
      *yyconflicts = yyconfl;
      return -yydefact[yystate];
    }
  else if (! yytable_value_is_error (yytable[yyindex]))
    {
      *yyconflicts = yyconfl + yyconflp[yyindex];
      return yytable[yyindex];
    }
  else
    {
      *yyconflicts = yyconfl + yyconflp[yyindex];
      return 0;
    }
}

/** Compute post-reduction state.
 * \param yystate   the current state
 * \param yysym     the nonterminal to push on the stack
 */
static inline yy_state_t
yyLRgotoState (yy_state_t yystate, yysymbol_kind_t yysym)
{
  int yyr = yypgoto[yysym - YYNTOKENS] + yystate;
  if (0 <= yyr && yyr <= YYLAST && yycheck[yyr] == yystate)
    return yytable[yyr];
  else
    return yydefgoto[yysym - YYNTOKENS];
}

static inline yybool
yyisShiftAction (int yyaction)
{
  return 0 < yyaction;
}

static inline yybool
yyisErrorAction (int yyaction)
{
  return yyaction == 0;
}

                                /* GLRStates */

/** Return a fresh GLRStackItem in YYSTACKP.  The item is an LR state
 *  if YYISSTATE, and otherwise a semantic option.  Callers should call
 *  YY_RESERVE_GLRSTACK afterwards to make sure there is sufficient
 *  headroom.  */

static inline yyGLRStackItem*
yynewGLRStackItem (yyGLRStack* yystackp, yybool yyisState)
{
  yyGLRStackItem* yynewItem = yystackp->yynextFree;
  yystackp->yyspaceLeft -= 1;
  yystackp->yynextFree += 1;
  yynewItem->yystate.yyisState = yyisState;
  return yynewItem;
}

/** Add a new semantic action that will execute the action for rule
 *  YYRULE on the semantic values in YYRHS to the list of
 *  alternative actions for YYSTATE.  Assumes that YYRHS comes from
 *  stack #YYK of *YYSTACKP. */
static void
yyaddDeferredAction (yyGLRStack* yystackp, YYPTRDIFF_T yyk, yyGLRState* yystate,
                     yyGLRState* yyrhs, yyRuleNum yyrule)
{
  yySemanticOption* yynewOption =
    &yynewGLRStackItem (yystackp, yyfalse)->yyoption;
  YY_ASSERT (!yynewOption->yyisState);
  yynewOption->yystate = yyrhs;
  yynewOption->yyrule = yyrule;
  if (yystackp->yytops.yylookaheadNeeds[yyk])
    {
      yynewOption->yyrawchar = yychar;
      yynewOption->yyval = yylval;
      yynewOption->yyloc = yylloc;
    }
  else
    yynewOption->yyrawchar = CYPHER_CYPHER_YYEMPTY;
  yynewOption->yynext = yystate->yysemantics.yyfirstVal;
  yystate->yysemantics.yyfirstVal = yynewOption;

  YY_RESERVE_GLRSTACK (yystackp);
}

                                /* GLRStacks */

/** Initialize YYSET to a singleton set containing an empty stack.  */
static yybool
yyinitStateSet (yyGLRStateSet* yyset)
{
  yyset->yysize = 1;
  yyset->yycapacity = 16;
  yyset->yystates
    = YY_CAST (yyGLRState**,
               YYMALLOC (YY_CAST (YYSIZE_T, yyset->yycapacity)
                         * sizeof yyset->yystates[0]));
  if (! yyset->yystates)
    return yyfalse;
  yyset->yystates[0] = YY_NULLPTR;
  yyset->yylookaheadNeeds
    = YY_CAST (yybool*,
               YYMALLOC (YY_CAST (YYSIZE_T, yyset->yycapacity)
                         * sizeof yyset->yylookaheadNeeds[0]));
  if (! yyset->yylookaheadNeeds)
    {
      YYFREE (yyset->yystates);
      return yyfalse;
    }
  memset (yyset->yylookaheadNeeds,
          0,
          YY_CAST (YYSIZE_T, yyset->yycapacity) * sizeof yyset->yylookaheadNeeds[0]);
  return yytrue;
}

static void yyfreeStateSet (yyGLRStateSet* yyset)
{
  YYFREE (yyset->yystates);
  YYFREE (yyset->yylookaheadNeeds);
}

/** Initialize *YYSTACKP to a single empty stack, with total maximum
 *  capacity for all stacks of YYSIZE.  */
static yybool
yyinitGLRStack (yyGLRStack* yystackp, YYPTRDIFF_T yysize)
{
  yystackp->yyerrState = 0;
  yynerrs = 0;
  yystackp->yyspaceLeft = yysize;
  yystackp->yyitems
    = YY_CAST (yyGLRStackItem*,
               YYMALLOC (YY_CAST (YYSIZE_T, yysize)
                         * sizeof yystackp->yynextFree[0]));
  if (!yystackp->yyitems)
    return yyfalse;
  yystackp->yynextFree = yystackp->yyitems;
  yystackp->yysplitPoint = YY_NULLPTR;
  yystackp->yylastDeleted = YY_NULLPTR;
  return yyinitStateSet (&yystackp->yytops);
}


#if YYSTACKEXPANDABLE
# define YYRELOC(YYFROMITEMS, YYTOITEMS, YYX, YYTYPE)                   \
  &((YYTOITEMS)                                                         \
    - ((YYFROMITEMS) - YY_REINTERPRET_CAST (yyGLRStackItem*, (YYX))))->YYTYPE

/** If *YYSTACKP is expandable, extend it.  WARNING: Pointers into the
    stack from outside should be considered invalid after this call.
    We always expand when there are 1 or fewer items left AFTER an
    allocation, so that we can avoid having external pointers exist
    across an allocation.  */
static void
yyexpandGLRStack (yyGLRStack* yystackp)
{
  yyGLRStackItem* yynewItems;
  yyGLRStackItem* yyp0, *yyp1;
  YYPTRDIFF_T yynewSize;
  YYPTRDIFF_T yyn;
  YYPTRDIFF_T yysize = yystackp->yynextFree - yystackp->yyitems;
  if (YYMAXDEPTH - YYHEADROOM < yysize)
    yyMemoryExhausted (yystackp);
  yynewSize = 2*yysize;
  if (YYMAXDEPTH < yynewSize)
    yynewSize = YYMAXDEPTH;
  yynewItems
    = YY_CAST (yyGLRStackItem*,
               YYMALLOC (YY_CAST (YYSIZE_T, yynewSize)
                         * sizeof yynewItems[0]));
  if (! yynewItems)
    yyMemoryExhausted (yystackp);
  for (yyp0 = yystackp->yyitems, yyp1 = yynewItems, yyn = yysize;
       0 < yyn;
       yyn -= 1, yyp0 += 1, yyp1 += 1)
    {
      *yyp1 = *yyp0;
      if (*YY_REINTERPRET_CAST (yybool *, yyp0))
        {
          yyGLRState* yys0 = &yyp0->yystate;
          yyGLRState* yys1 = &yyp1->yystate;
          if (yys0->yypred != YY_NULLPTR)
            yys1->yypred =
              YYRELOC (yyp0, yyp1, yys0->yypred, yystate);
          if (! yys0->yyresolved && yys0->yysemantics.yyfirstVal != YY_NULLPTR)
            yys1->yysemantics.yyfirstVal =
              YYRELOC (yyp0, yyp1, yys0->yysemantics.yyfirstVal, yyoption);
        }
      else
        {
          yySemanticOption* yyv0 = &yyp0->yyoption;
          yySemanticOption* yyv1 = &yyp1->yyoption;
          if (yyv0->yystate != YY_NULLPTR)
            yyv1->yystate = YYRELOC (yyp0, yyp1, yyv0->yystate, yystate);
          if (yyv0->yynext != YY_NULLPTR)
            yyv1->yynext = YYRELOC (yyp0, yyp1, yyv0->yynext, yyoption);
        }
    }
  if (yystackp->yysplitPoint != YY_NULLPTR)
    yystackp->yysplitPoint = YYRELOC (yystackp->yyitems, yynewItems,
                                      yystackp->yysplitPoint, yystate);

  for (yyn = 0; yyn < yystackp->yytops.yysize; yyn += 1)
    if (yystackp->yytops.yystates[yyn] != YY_NULLPTR)
      yystackp->yytops.yystates[yyn] =
        YYRELOC (yystackp->yyitems, yynewItems,
                 yystackp->yytops.yystates[yyn], yystate);
  YYFREE (yystackp->yyitems);
  yystackp->yyitems = yynewItems;
  yystackp->yynextFree = yynewItems + yysize;
  yystackp->yyspaceLeft = yynewSize - yysize;
}
#endif

static void
yyfreeGLRStack (yyGLRStack* yystackp)
{
  YYFREE (yystackp->yyitems);
  yyfreeStateSet (&yystackp->yytops);
}

/** Assuming that YYS is a GLRState somewhere on *YYSTACKP, update the
 *  splitpoint of *YYSTACKP, if needed, so that it is at least as deep as
 *  YYS.  */
static inline void
yyupdateSplit (yyGLRStack* yystackp, yyGLRState* yys)
{
  if (yystackp->yysplitPoint != YY_NULLPTR && yystackp->yysplitPoint > yys)
    yystackp->yysplitPoint = yys;
}

/** Invalidate stack #YYK in *YYSTACKP.  */
static inline void
yymarkStackDeleted (yyGLRStack* yystackp, YYPTRDIFF_T yyk)
{
  if (yystackp->yytops.yystates[yyk] != YY_NULLPTR)
    yystackp->yylastDeleted = yystackp->yytops.yystates[yyk];
  yystackp->yytops.yystates[yyk] = YY_NULLPTR;
}

/** Undelete the last stack in *YYSTACKP that was marked as deleted.  Can
    only be done once after a deletion, and only when all other stacks have
    been deleted.  */
static void
yyundeleteLastStack (yyGLRStack* yystackp)
{
  if (yystackp->yylastDeleted == YY_NULLPTR || yystackp->yytops.yysize != 0)
    return;
  yystackp->yytops.yystates[0] = yystackp->yylastDeleted;
  yystackp->yytops.yysize = 1;
  YY_DPRINTF ((stderr, "Restoring last deleted stack as stack #0.\n"));
  yystackp->yylastDeleted = YY_NULLPTR;
}

static inline void
yyremoveDeletes (yyGLRStack* yystackp)
{
  YYPTRDIFF_T yyi, yyj;
  yyi = yyj = 0;
  while (yyj < yystackp->yytops.yysize)
    {
      if (yystackp->yytops.yystates[yyi] == YY_NULLPTR)
        {
          if (yyi == yyj)
            YY_DPRINTF ((stderr, "Removing dead stacks.\n"));
          yystackp->yytops.yysize -= 1;
        }
      else
        {
          yystackp->yytops.yystates[yyj] = yystackp->yytops.yystates[yyi];
          /* In the current implementation, it's unnecessary to copy
             yystackp->yytops.yylookaheadNeeds[yyi] since, after
             yyremoveDeletes returns, the parser immediately either enters
             deterministic operation or shifts a token.  However, it doesn't
             hurt, and the code might evolve to need it.  */
          yystackp->yytops.yylookaheadNeeds[yyj] =
            yystackp->yytops.yylookaheadNeeds[yyi];
          if (yyj != yyi)
            YY_DPRINTF ((stderr, "Rename stack %ld -> %ld.\n",
                        YY_CAST (long, yyi), YY_CAST (long, yyj)));
          yyj += 1;
        }
      yyi += 1;
    }
}

/** Shift to a new state on stack #YYK of *YYSTACKP, corresponding to LR
 * state YYLRSTATE, at input position YYPOSN, with (resolved) semantic
 * value *YYVALP and source location *YYLOCP.  */
static inline void
yyglrShift (yyGLRStack* yystackp, YYPTRDIFF_T yyk, yy_state_t yylrState,
            YYPTRDIFF_T yyposn,
            YYSTYPE* yyvalp, YYLTYPE* yylocp)
{
  yyGLRState* yynewState = &yynewGLRStackItem (yystackp, yytrue)->yystate;

  yynewState->yylrState = yylrState;
  yynewState->yyposn = yyposn;
  yynewState->yyresolved = yytrue;
  yynewState->yypred = yystackp->yytops.yystates[yyk];
  yynewState->yysemantics.yyval = *yyvalp;
  yynewState->yyloc = *yylocp;
  yystackp->yytops.yystates[yyk] = yynewState;

  YY_RESERVE_GLRSTACK (yystackp);
}

/** Shift stack #YYK of *YYSTACKP, to a new state corresponding to LR
 *  state YYLRSTATE, at input position YYPOSN, with the (unresolved)
 *  semantic value of YYRHS under the action for YYRULE.  */
static inline void
yyglrShiftDefer (yyGLRStack* yystackp, YYPTRDIFF_T yyk, yy_state_t yylrState,
                 YYPTRDIFF_T yyposn, yyGLRState* yyrhs, yyRuleNum yyrule)
{
  yyGLRState* yynewState = &yynewGLRStackItem (yystackp, yytrue)->yystate;
  YY_ASSERT (yynewState->yyisState);

  yynewState->yylrState = yylrState;
  yynewState->yyposn = yyposn;
  yynewState->yyresolved = yyfalse;
  yynewState->yypred = yystackp->yytops.yystates[yyk];
  yynewState->yysemantics.yyfirstVal = YY_NULLPTR;
  yystackp->yytops.yystates[yyk] = yynewState;

  /* Invokes YY_RESERVE_GLRSTACK.  */
  yyaddDeferredAction (yystackp, yyk, yynewState, yyrhs, yyrule);
}

#if CYPHER_YYDEBUG

/*----------------------------------------------------------------------.
| Report that stack #YYK of *YYSTACKP is going to be reduced by YYRULE. |
`----------------------------------------------------------------------*/

static inline void
yy_reduce_print (yybool yynormal, yyGLRStackItem* yyvsp, YYPTRDIFF_T yyk,
                 yyRuleNum yyrule, cypher_parser_context *context)
{
  int yynrhs = yyrhsLength (yyrule);
  int yylow = 1;
  int yyi;
  YY_FPRINTF ((stderr, "Reducing stack %ld by rule %d (line %d):\n",
               YY_CAST (long, yyk), yyrule - 1, yyrline[yyrule]));
  if (! yynormal)
    yyfillin (yyvsp, 1, -yynrhs);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YY_FPRINTF ((stderr, "   $%d = ", yyi + 1));
      yy_symbol_print (stderr,
                       yy_accessing_symbol (yyvsp[yyi - yynrhs + 1].yystate.yylrState),
                       &yyvsp[yyi - yynrhs + 1].yystate.yysemantics.yyval,
                       &(YY_CAST (yyGLRStackItem const *, yyvsp)[YYFILL ((yyi + 1) - (yynrhs))].yystate.yyloc)                       , context);
      if (!yyvsp[yyi - yynrhs + 1].yystate.yyresolved)
        YY_FPRINTF ((stderr, " (unresolved)"));
      YY_FPRINTF ((stderr, "\n"));
    }
}
#endif

/** Pop the symbols consumed by reduction #YYRULE from the top of stack
 *  #YYK of *YYSTACKP, and perform the appropriate semantic action on their
 *  semantic values.  Assumes that all ambiguities in semantic values
 *  have been previously resolved.  Set *YYVALP to the resulting value,
 *  and *YYLOCP to the computed location (if any).  Return value is as
 *  for userAction.  */
static inline YYRESULTTAG
yydoAction (yyGLRStack* yystackp, YYPTRDIFF_T yyk, yyRuleNum yyrule,
            YYSTYPE* yyvalp, YYLTYPE *yylocp, cypher_parser_context *context)
{
  int yynrhs = yyrhsLength (yyrule);

  if (yystackp->yysplitPoint == YY_NULLPTR)
    {
      /* Standard special case: single stack.  */
      yyGLRStackItem* yyrhs
        = YY_REINTERPRET_CAST (yyGLRStackItem*, yystackp->yytops.yystates[yyk]);
      YY_ASSERT (yyk == 0);
      yystackp->yynextFree -= yynrhs;
      yystackp->yyspaceLeft += yynrhs;
      yystackp->yytops.yystates[0] = & yystackp->yynextFree[-1].yystate;
      return yyuserAction (yyrule, yynrhs, yyrhs, yystackp, yyk,
                           yyvalp, yylocp, context);
    }
  else
    {
      yyGLRStackItem yyrhsVals[YYMAXRHS + YYMAXLEFT + 1];
      yyGLRState* yys = yyrhsVals[YYMAXRHS + YYMAXLEFT].yystate.yypred
        = yystackp->yytops.yystates[yyk];
      int yyi;
      if (yynrhs == 0)
        /* Set default location.  */
        yyrhsVals[YYMAXRHS + YYMAXLEFT - 1].yystate.yyloc = yys->yyloc;
      for (yyi = 0; yyi < yynrhs; yyi += 1)
        {
          yys = yys->yypred;
          YY_ASSERT (yys);
        }
      yyupdateSplit (yystackp, yys);
      yystackp->yytops.yystates[yyk] = yys;
      return yyuserAction (yyrule, yynrhs, yyrhsVals + YYMAXRHS + YYMAXLEFT - 1,
                           yystackp, yyk, yyvalp, yylocp, context);
    }
}

/** Pop items off stack #YYK of *YYSTACKP according to grammar rule YYRULE,
 *  and push back on the resulting nonterminal symbol.  Perform the
 *  semantic action associated with YYRULE and store its value with the
 *  newly pushed state, if YYFORCEEVAL or if *YYSTACKP is currently
 *  unambiguous.  Otherwise, store the deferred semantic action with
 *  the new state.  If the new state would have an identical input
 *  position, LR state, and predecessor to an existing state on the stack,
 *  it is identified with that existing state, eliminating stack #YYK from
 *  *YYSTACKP.  In this case, the semantic value is
 *  added to the options for the existing state's semantic value.
 */
static inline YYRESULTTAG
yyglrReduce (yyGLRStack* yystackp, YYPTRDIFF_T yyk, yyRuleNum yyrule,
             yybool yyforceEval, cypher_parser_context *context)
{
  YYPTRDIFF_T yyposn = yystackp->yytops.yystates[yyk]->yyposn;

  if (yyforceEval || yystackp->yysplitPoint == YY_NULLPTR)
    {
      YYSTYPE yyval;
      YYLTYPE yyloc;

      YYRESULTTAG yyflag = yydoAction (yystackp, yyk, yyrule, &yyval, &yyloc, context);
      if (yyflag == yyerr && yystackp->yysplitPoint != YY_NULLPTR)
        YY_DPRINTF ((stderr,
                     "Parse on stack %ld rejected by rule %d (line %d).\n",
                     YY_CAST (long, yyk), yyrule - 1, yyrline[yyrule]));
      if (yyflag != yyok)
        return yyflag;
      yyglrShift (yystackp, yyk,
                  yyLRgotoState (yystackp->yytops.yystates[yyk]->yylrState,
                                 yylhsNonterm (yyrule)),
                  yyposn, &yyval, &yyloc);
    }
  else
    {
      YYPTRDIFF_T yyi;
      int yyn;
      yyGLRState* yys, *yys0 = yystackp->yytops.yystates[yyk];
      yy_state_t yynewLRState;

      for (yys = yystackp->yytops.yystates[yyk], yyn = yyrhsLength (yyrule);
           0 < yyn; yyn -= 1)
        {
          yys = yys->yypred;
          YY_ASSERT (yys);
        }
      yyupdateSplit (yystackp, yys);
      yynewLRState = yyLRgotoState (yys->yylrState, yylhsNonterm (yyrule));
      YY_DPRINTF ((stderr,
                   "Reduced stack %ld by rule %d (line %d); action deferred.  "
                   "Now in state %d.\n",
                   YY_CAST (long, yyk), yyrule - 1, yyrline[yyrule],
                   yynewLRState));
      for (yyi = 0; yyi < yystackp->yytops.yysize; yyi += 1)
        if (yyi != yyk && yystackp->yytops.yystates[yyi] != YY_NULLPTR)
          {
            yyGLRState *yysplit = yystackp->yysplitPoint;
            yyGLRState *yyp = yystackp->yytops.yystates[yyi];
            while (yyp != yys && yyp != yysplit && yyp->yyposn >= yyposn)
              {
                if (yyp->yylrState == yynewLRState && yyp->yypred == yys)
                  {
                    yyaddDeferredAction (yystackp, yyk, yyp, yys0, yyrule);
                    yymarkStackDeleted (yystackp, yyk);
                    YY_DPRINTF ((stderr, "Merging stack %ld into stack %ld.\n",
                                 YY_CAST (long, yyk), YY_CAST (long, yyi)));
                    return yyok;
                  }
                yyp = yyp->yypred;
              }
          }
      yystackp->yytops.yystates[yyk] = yys;
      yyglrShiftDefer (yystackp, yyk, yynewLRState, yyposn, yys0, yyrule);
    }
  return yyok;
}

static YYPTRDIFF_T
yysplitStack (yyGLRStack* yystackp, YYPTRDIFF_T yyk)
{
  if (yystackp->yysplitPoint == YY_NULLPTR)
    {
      YY_ASSERT (yyk == 0);
      yystackp->yysplitPoint = yystackp->yytops.yystates[yyk];
    }
  if (yystackp->yytops.yycapacity <= yystackp->yytops.yysize)
    {
      YYPTRDIFF_T state_size = YYSIZEOF (yystackp->yytops.yystates[0]);
      YYPTRDIFF_T half_max_capacity = YYSIZE_MAXIMUM / 2 / state_size;
      if (half_max_capacity < yystackp->yytops.yycapacity)
        yyMemoryExhausted (yystackp);
      yystackp->yytops.yycapacity *= 2;

      {
        yyGLRState** yynewStates
          = YY_CAST (yyGLRState**,
                     YYREALLOC (yystackp->yytops.yystates,
                                (YY_CAST (YYSIZE_T, yystackp->yytops.yycapacity)
                                 * sizeof yynewStates[0])));
        if (yynewStates == YY_NULLPTR)
          yyMemoryExhausted (yystackp);
        yystackp->yytops.yystates = yynewStates;
      }

      {
        yybool* yynewLookaheadNeeds
          = YY_CAST (yybool*,
                     YYREALLOC (yystackp->yytops.yylookaheadNeeds,
                                (YY_CAST (YYSIZE_T, yystackp->yytops.yycapacity)
                                 * sizeof yynewLookaheadNeeds[0])));
        if (yynewLookaheadNeeds == YY_NULLPTR)
          yyMemoryExhausted (yystackp);
        yystackp->yytops.yylookaheadNeeds = yynewLookaheadNeeds;
      }
    }
  yystackp->yytops.yystates[yystackp->yytops.yysize]
    = yystackp->yytops.yystates[yyk];
  yystackp->yytops.yylookaheadNeeds[yystackp->yytops.yysize]
    = yystackp->yytops.yylookaheadNeeds[yyk];
  yystackp->yytops.yysize += 1;
  return yystackp->yytops.yysize - 1;
}

/** True iff YYY0 and YYY1 represent identical options at the top level.
 *  That is, they represent the same rule applied to RHS symbols
 *  that produce the same terminal symbols.  */
static yybool
yyidenticalOptions (yySemanticOption* yyy0, yySemanticOption* yyy1)
{
  if (yyy0->yyrule == yyy1->yyrule)
    {
      yyGLRState *yys0, *yys1;
      int yyn;
      for (yys0 = yyy0->yystate, yys1 = yyy1->yystate,
           yyn = yyrhsLength (yyy0->yyrule);
           yyn > 0;
           yys0 = yys0->yypred, yys1 = yys1->yypred, yyn -= 1)
        if (yys0->yyposn != yys1->yyposn)
          return yyfalse;
      return yytrue;
    }
  else
    return yyfalse;
}

/** Assuming identicalOptions (YYY0,YYY1), destructively merge the
 *  alternative semantic values for the RHS-symbols of YYY1 and YYY0.  */
static void
yymergeOptionSets (yySemanticOption* yyy0, yySemanticOption* yyy1)
{
  yyGLRState *yys0, *yys1;
  int yyn;
  for (yys0 = yyy0->yystate, yys1 = yyy1->yystate,
       yyn = yyrhsLength (yyy0->yyrule);
       0 < yyn;
       yys0 = yys0->yypred, yys1 = yys1->yypred, yyn -= 1)
    {
      if (yys0 == yys1)
        break;
      else if (yys0->yyresolved)
        {
          yys1->yyresolved = yytrue;
          yys1->yysemantics.yyval = yys0->yysemantics.yyval;
        }
      else if (yys1->yyresolved)
        {
          yys0->yyresolved = yytrue;
          yys0->yysemantics.yyval = yys1->yysemantics.yyval;
        }
      else
        {
          yySemanticOption** yyz0p = &yys0->yysemantics.yyfirstVal;
          yySemanticOption* yyz1 = yys1->yysemantics.yyfirstVal;
          while (yytrue)
            {
              if (yyz1 == *yyz0p || yyz1 == YY_NULLPTR)
                break;
              else if (*yyz0p == YY_NULLPTR)
                {
                  *yyz0p = yyz1;
                  break;
                }
              else if (*yyz0p < yyz1)
                {
                  yySemanticOption* yyz = *yyz0p;
                  *yyz0p = yyz1;
                  yyz1 = yyz1->yynext;
                  (*yyz0p)->yynext = yyz;
                }
              yyz0p = &(*yyz0p)->yynext;
            }
          yys1->yysemantics.yyfirstVal = yys0->yysemantics.yyfirstVal;
        }
    }
}

/** Y0 and Y1 represent two possible actions to take in a given
 *  parsing state; return 0 if no combination is possible,
 *  1 if user-mergeable, 2 if Y0 is preferred, 3 if Y1 is preferred.  */
static int
yypreference (yySemanticOption* y0, yySemanticOption* y1)
{
  yyRuleNum r0 = y0->yyrule, r1 = y1->yyrule;
  int p0 = yydprec[r0], p1 = yydprec[r1];

  if (p0 == p1)
    {
      if (yymerger[r0] == 0 || yymerger[r0] != yymerger[r1])
        return 0;
      else
        return 1;
    }
  if (p0 == 0 || p1 == 0)
    return 0;
  if (p0 < p1)
    return 3;
  if (p1 < p0)
    return 2;
  return 0;
}

static YYRESULTTAG
yyresolveValue (yyGLRState* yys, yyGLRStack* yystackp, cypher_parser_context *context);


/** Resolve the previous YYN states starting at and including state YYS
 *  on *YYSTACKP. If result != yyok, some states may have been left
 *  unresolved possibly with empty semantic option chains.  Regardless
 *  of whether result = yyok, each state has been left with consistent
 *  data so that yydestroyGLRState can be invoked if necessary.  */
static YYRESULTTAG
yyresolveStates (yyGLRState* yys, int yyn,
                 yyGLRStack* yystackp, cypher_parser_context *context)
{
  if (0 < yyn)
    {
      YY_ASSERT (yys->yypred);
      YYCHK (yyresolveStates (yys->yypred, yyn-1, yystackp, context));
      if (! yys->yyresolved)
        YYCHK (yyresolveValue (yys, yystackp, context));
    }
  return yyok;
}

/** Resolve the states for the RHS of YYOPT on *YYSTACKP, perform its
 *  user action, and return the semantic value and location in *YYVALP
 *  and *YYLOCP.  Regardless of whether result = yyok, all RHS states
 *  have been destroyed (assuming the user action destroys all RHS
 *  semantic values if invoked).  */
static YYRESULTTAG
yyresolveAction (yySemanticOption* yyopt, yyGLRStack* yystackp,
                 YYSTYPE* yyvalp, YYLTYPE *yylocp, cypher_parser_context *context)
{
  yyGLRStackItem yyrhsVals[YYMAXRHS + YYMAXLEFT + 1];
  int yynrhs = yyrhsLength (yyopt->yyrule);
  YYRESULTTAG yyflag =
    yyresolveStates (yyopt->yystate, yynrhs, yystackp, context);
  if (yyflag != yyok)
    {
      yyGLRState *yys;
      for (yys = yyopt->yystate; yynrhs > 0; yys = yys->yypred, yynrhs -= 1)
        yydestroyGLRState ("Cleanup: popping", yys, context);
      return yyflag;
    }

  yyrhsVals[YYMAXRHS + YYMAXLEFT].yystate.yypred = yyopt->yystate;
  if (yynrhs == 0)
    /* Set default location.  */
    yyrhsVals[YYMAXRHS + YYMAXLEFT - 1].yystate.yyloc = yyopt->yystate->yyloc;
  {
    int yychar_current = yychar;
    YYSTYPE yylval_current = yylval;
    YYLTYPE yylloc_current = yylloc;
    yychar = yyopt->yyrawchar;
    yylval = yyopt->yyval;
    yylloc = yyopt->yyloc;
    yyflag = yyuserAction (yyopt->yyrule, yynrhs,
                           yyrhsVals + YYMAXRHS + YYMAXLEFT - 1,
                           yystackp, -1, yyvalp, yylocp, context);
    yychar = yychar_current;
    yylval = yylval_current;
    yylloc = yylloc_current;
  }
  return yyflag;
}

#if CYPHER_YYDEBUG
static void
yyreportTree (yySemanticOption* yyx, int yyindent)
{
  int yynrhs = yyrhsLength (yyx->yyrule);
  int yyi;
  yyGLRState* yys;
  yyGLRState* yystates[1 + YYMAXRHS];
  yyGLRState yyleftmost_state;

  for (yyi = yynrhs, yys = yyx->yystate; 0 < yyi; yyi -= 1, yys = yys->yypred)
    yystates[yyi] = yys;
  if (yys == YY_NULLPTR)
    {
      yyleftmost_state.yyposn = 0;
      yystates[0] = &yyleftmost_state;
    }
  else
    yystates[0] = yys;

  if (yyx->yystate->yyposn < yys->yyposn + 1)
    YY_FPRINTF ((stderr, "%*s%s -> <Rule %d, empty>\n",
                 yyindent, "", yysymbol_name (yylhsNonterm (yyx->yyrule)),
                 yyx->yyrule - 1));
  else
    YY_FPRINTF ((stderr, "%*s%s -> <Rule %d, tokens %ld .. %ld>\n",
                 yyindent, "", yysymbol_name (yylhsNonterm (yyx->yyrule)),
                 yyx->yyrule - 1, YY_CAST (long, yys->yyposn + 1),
                 YY_CAST (long, yyx->yystate->yyposn)));
  for (yyi = 1; yyi <= yynrhs; yyi += 1)
    {
      if (yystates[yyi]->yyresolved)
        {
          if (yystates[yyi-1]->yyposn+1 > yystates[yyi]->yyposn)
            YY_FPRINTF ((stderr, "%*s%s <empty>\n", yyindent+2, "",
                         yysymbol_name (yy_accessing_symbol (yystates[yyi]->yylrState))));
          else
            YY_FPRINTF ((stderr, "%*s%s <tokens %ld .. %ld>\n", yyindent+2, "",
                         yysymbol_name (yy_accessing_symbol (yystates[yyi]->yylrState)),
                         YY_CAST (long, yystates[yyi-1]->yyposn + 1),
                         YY_CAST (long, yystates[yyi]->yyposn)));
        }
      else
        yyreportTree (yystates[yyi]->yysemantics.yyfirstVal, yyindent+2);
    }
}
#endif

static YYRESULTTAG
yyreportAmbiguity (yySemanticOption* yyx0,
                   yySemanticOption* yyx1, YYLTYPE *yylocp, cypher_parser_context *context)
{
  YY_USE (yyx0);
  YY_USE (yyx1);

#if CYPHER_YYDEBUG
  YY_FPRINTF ((stderr, "Ambiguity detected.\n"));
  YY_FPRINTF ((stderr, "Option 1,\n"));
  yyreportTree (yyx0, 2);
  YY_FPRINTF ((stderr, "\nOption 2,\n"));
  yyreportTree (yyx1, 2);
  YY_FPRINTF ((stderr, "\n"));
#endif

  yyerror (yylocp, context, YY_("syntax is ambiguous"));
  return yyabort;
}

/** Resolve the locations for each of the YYN1 states in *YYSTACKP,
 *  ending at YYS1.  Has no effect on previously resolved states.
 *  The first semantic option of a state is always chosen.  */
static void
yyresolveLocations (yyGLRState *yys1, int yyn1,
                    yyGLRStack *yystackp, cypher_parser_context *context)
{
  if (0 < yyn1)
    {
      yyresolveLocations (yys1->yypred, yyn1 - 1, yystackp, context);
      if (!yys1->yyresolved)
        {
          yyGLRStackItem yyrhsloc[1 + YYMAXRHS];
          int yynrhs;
          yySemanticOption *yyoption = yys1->yysemantics.yyfirstVal;
          YY_ASSERT (yyoption);
          yynrhs = yyrhsLength (yyoption->yyrule);
          if (0 < yynrhs)
            {
              yyGLRState *yys;
              int yyn;
              yyresolveLocations (yyoption->yystate, yynrhs,
                                  yystackp, context);
              for (yys = yyoption->yystate, yyn = yynrhs;
                   yyn > 0;
                   yys = yys->yypred, yyn -= 1)
                yyrhsloc[yyn].yystate.yyloc = yys->yyloc;
            }
          else
            {
              /* Both yyresolveAction and yyresolveLocations traverse the GSS
                 in reverse rightmost order.  It is only necessary to invoke
                 yyresolveLocations on a subforest for which yyresolveAction
                 would have been invoked next had an ambiguity not been
                 detected.  Thus the location of the previous state (but not
                 necessarily the previous state itself) is guaranteed to be
                 resolved already.  */
              yyGLRState *yyprevious = yyoption->yystate;
              yyrhsloc[0].yystate.yyloc = yyprevious->yyloc;
            }
          YYLLOC_DEFAULT ((yys1->yyloc), yyrhsloc, yynrhs);
        }
    }
}

/** Resolve the ambiguity represented in state YYS in *YYSTACKP,
 *  perform the indicated actions, and set the semantic value of YYS.
 *  If result != yyok, the chain of semantic options in YYS has been
 *  cleared instead or it has been left unmodified except that
 *  redundant options may have been removed.  Regardless of whether
 *  result = yyok, YYS has been left with consistent data so that
 *  yydestroyGLRState can be invoked if necessary.  */
static YYRESULTTAG
yyresolveValue (yyGLRState* yys, yyGLRStack* yystackp, cypher_parser_context *context)
{
  yySemanticOption* yyoptionList = yys->yysemantics.yyfirstVal;
  yySemanticOption* yybest = yyoptionList;
  yySemanticOption** yypp;
  yybool yymerge = yyfalse;
  YYSTYPE yyval;
  YYRESULTTAG yyflag;
  YYLTYPE *yylocp = &yys->yyloc;

  for (yypp = &yyoptionList->yynext; *yypp != YY_NULLPTR; )
    {
      yySemanticOption* yyp = *yypp;

      if (yyidenticalOptions (yybest, yyp))
        {
          yymergeOptionSets (yybest, yyp);
          *yypp = yyp->yynext;
        }
      else
        {
          switch (yypreference (yybest, yyp))
            {
            case 0:
              yyresolveLocations (yys, 1, yystackp, context);
              return yyreportAmbiguity (yybest, yyp, yylocp, context);
              break;
            case 1:
              yymerge = yytrue;
              break;
            case 2:
              break;
            case 3:
              yybest = yyp;
              yymerge = yyfalse;
              break;
            default:
              /* This cannot happen so it is not worth a YY_ASSERT (yyfalse),
                 but some compilers complain if the default case is
                 omitted.  */
              break;
            }
          yypp = &yyp->yynext;
        }
    }

  if (yymerge)
    {
      yySemanticOption* yyp;
      int yyprec = yydprec[yybest->yyrule];
      yyflag = yyresolveAction (yybest, yystackp, &yyval, yylocp, context);
      if (yyflag == yyok)
        for (yyp = yybest->yynext; yyp != YY_NULLPTR; yyp = yyp->yynext)
          {
            if (yyprec == yydprec[yyp->yyrule])
              {
                YYSTYPE yyval_other;
                YYLTYPE yydummy;
                yyflag = yyresolveAction (yyp, yystackp, &yyval_other, &yydummy, context);
                if (yyflag != yyok)
                  {
                    yydestruct ("Cleanup: discarding incompletely merged value for",
                                yy_accessing_symbol (yys->yylrState),
                                &yyval, yylocp, context);
                    break;
                  }
                yyuserMerge (yymerger[yyp->yyrule], &yyval, &yyval_other);
              }
          }
    }
  else
    yyflag = yyresolveAction (yybest, yystackp, &yyval, yylocp, context);

  if (yyflag == yyok)
    {
      yys->yyresolved = yytrue;
      yys->yysemantics.yyval = yyval;
    }
  else
    yys->yysemantics.yyfirstVal = YY_NULLPTR;
  return yyflag;
}

static YYRESULTTAG
yyresolveStack (yyGLRStack* yystackp, cypher_parser_context *context)
{
  if (yystackp->yysplitPoint != YY_NULLPTR)
    {
      yyGLRState* yys;
      int yyn;

      for (yyn = 0, yys = yystackp->yytops.yystates[0];
           yys != yystackp->yysplitPoint;
           yys = yys->yypred, yyn += 1)
        continue;
      YYCHK (yyresolveStates (yystackp->yytops.yystates[0], yyn, yystackp
                             , context));
    }
  return yyok;
}

/** Called when returning to deterministic operation to clean up the extra
 * stacks. */
static void
yycompressStack (yyGLRStack* yystackp)
{
  /* yyr is the state after the split point.  */
  yyGLRState *yyr;

  if (yystackp->yytops.yysize != 1 || yystackp->yysplitPoint == YY_NULLPTR)
    return;

  {
    yyGLRState *yyp, *yyq;
    for (yyp = yystackp->yytops.yystates[0], yyq = yyp->yypred, yyr = YY_NULLPTR;
         yyp != yystackp->yysplitPoint;
         yyr = yyp, yyp = yyq, yyq = yyp->yypred)
      yyp->yypred = yyr;
  }

  yystackp->yyspaceLeft += yystackp->yynextFree - yystackp->yyitems;
  yystackp->yynextFree = YY_REINTERPRET_CAST (yyGLRStackItem*, yystackp->yysplitPoint) + 1;
  yystackp->yyspaceLeft -= yystackp->yynextFree - yystackp->yyitems;
  yystackp->yysplitPoint = YY_NULLPTR;
  yystackp->yylastDeleted = YY_NULLPTR;

  while (yyr != YY_NULLPTR)
    {
      yystackp->yynextFree->yystate = *yyr;
      yyr = yyr->yypred;
      yystackp->yynextFree->yystate.yypred = &yystackp->yynextFree[-1].yystate;
      yystackp->yytops.yystates[0] = &yystackp->yynextFree->yystate;
      yystackp->yynextFree += 1;
      yystackp->yyspaceLeft -= 1;
    }
}

static YYRESULTTAG
yyprocessOneStack (yyGLRStack* yystackp, YYPTRDIFF_T yyk,
                   YYPTRDIFF_T yyposn, YYLTYPE *yylocp, cypher_parser_context *context)
{
  while (yystackp->yytops.yystates[yyk] != YY_NULLPTR)
    {
      yy_state_t yystate = yystackp->yytops.yystates[yyk]->yylrState;
      YY_DPRINTF ((stderr, "Stack %ld Entering state %d\n",
                   YY_CAST (long, yyk), yystate));

      YY_ASSERT (yystate != YYFINAL);

      if (yyisDefaultedState (yystate))
        {
          YYRESULTTAG yyflag;
          yyRuleNum yyrule = yydefaultAction (yystate);
          if (yyrule == 0)
            {
              YY_DPRINTF ((stderr, "Stack %ld dies.\n", YY_CAST (long, yyk)));
              yymarkStackDeleted (yystackp, yyk);
              return yyok;
            }
          yyflag = yyglrReduce (yystackp, yyk, yyrule, yyimmediate[yyrule], context);
          if (yyflag == yyerr)
            {
              YY_DPRINTF ((stderr,
                           "Stack %ld dies "
                           "(predicate failure or explicit user error).\n",
                           YY_CAST (long, yyk)));
              yymarkStackDeleted (yystackp, yyk);
              return yyok;
            }
          if (yyflag != yyok)
            return yyflag;
        }
      else
        {
          yysymbol_kind_t yytoken = yygetToken (&yychar, yystackp, context);
          const short* yyconflicts;
          const int yyaction = yygetLRActions (yystate, yytoken, &yyconflicts);
          yystackp->yytops.yylookaheadNeeds[yyk] = yytrue;

          for (/* nothing */; *yyconflicts; yyconflicts += 1)
            {
              YYRESULTTAG yyflag;
              YYPTRDIFF_T yynewStack = yysplitStack (yystackp, yyk);
              YY_DPRINTF ((stderr, "Splitting off stack %ld from %ld.\n",
                           YY_CAST (long, yynewStack), YY_CAST (long, yyk)));
              yyflag = yyglrReduce (yystackp, yynewStack,
                                    *yyconflicts,
                                    yyimmediate[*yyconflicts], context);
              if (yyflag == yyok)
                YYCHK (yyprocessOneStack (yystackp, yynewStack,
                                          yyposn, yylocp, context));
              else if (yyflag == yyerr)
                {
                  YY_DPRINTF ((stderr, "Stack %ld dies.\n", YY_CAST (long, yynewStack)));
                  yymarkStackDeleted (yystackp, yynewStack);
                }
              else
                return yyflag;
            }

          if (yyisShiftAction (yyaction))
            break;
          else if (yyisErrorAction (yyaction))
            {
              YY_DPRINTF ((stderr, "Stack %ld dies.\n", YY_CAST (long, yyk)));
              yymarkStackDeleted (yystackp, yyk);
              break;
            }
          else
            {
              YYRESULTTAG yyflag = yyglrReduce (yystackp, yyk, -yyaction,
                                                yyimmediate[-yyaction], context);
              if (yyflag == yyerr)
                {
                  YY_DPRINTF ((stderr,
                               "Stack %ld dies "
                               "(predicate failure or explicit user error).\n",
                               YY_CAST (long, yyk)));
                  yymarkStackDeleted (yystackp, yyk);
                  break;
                }
              else if (yyflag != yyok)
                return yyflag;
            }
        }
    }
  return yyok;
}

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYSTACKP, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  */
static int
yypcontext_expected_tokens (const yyGLRStack* yystackp,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[yystackp->yytops.yystates[0]->yylrState];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}

static int
yy_syntax_error_arguments (const yyGLRStack* yystackp,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  yysymbol_kind_t yytoken = yychar == CYPHER_CYPHER_YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yystackp,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}



static void
yyreportSyntaxError (yyGLRStack* yystackp, cypher_parser_context *context)
{
  if (yystackp->yyerrState != 0)
    return;
  {
  yybool yysize_overflow = yyfalse;
  char* yymsg = YY_NULLPTR;
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount
    = yy_syntax_error_arguments (yystackp, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    yyMemoryExhausted (yystackp);

  switch (yycount)
    {
#define YYCASE_(N, S)                   \
      case N:                           \
        yyformat = S;                   \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysz
          = yystrlen (yysymbol_name (yyarg[yyi]));
        if (YYSIZE_MAXIMUM - yysize < yysz)
          yysize_overflow = yytrue;
        else
          yysize += yysz;
      }
  }

  if (!yysize_overflow)
    yymsg = YY_CAST (char *, YYMALLOC (YY_CAST (YYSIZE_T, yysize)));

  if (yymsg)
    {
      char *yyp = yymsg;
      int yyi = 0;
      while ((*yyp = *yyformat))
        {
          if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
            {
              yyp = yystpcpy (yyp, yysymbol_name (yyarg[yyi++]));
              yyformat += 2;
            }
          else
            {
              ++yyp;
              ++yyformat;
            }
        }
      yyerror (&yylloc, context, yymsg);
      YYFREE (yymsg);
    }
  else
    {
      yyerror (&yylloc, context, YY_("syntax error"));
      yyMemoryExhausted (yystackp);
    }
  }
  yynerrs += 1;
}

/* Recover from a syntax error on *YYSTACKP, assuming that *YYSTACKP->YYTOKENP,
   yylval, and yylloc are the syntactic category, semantic value, and location
   of the lookahead.  */
static void
yyrecoverSyntaxError (yyGLRStack* yystackp, cypher_parser_context *context)
{
  if (yystackp->yyerrState == 3)
    /* We just shifted the error token and (perhaps) took some
       reductions.  Skip tokens until we can proceed.  */
    while (yytrue)
      {
        yysymbol_kind_t yytoken;
        int yyj;
        if (yychar == CYPHER_YYEOF)
          yyFail (yystackp, &yylloc, context, YY_NULLPTR);
        if (yychar != CYPHER_CYPHER_YYEMPTY)
          {
            /* We throw away the lookahead, but the error range
               of the shifted error token must take it into account.  */
            yyGLRState *yys = yystackp->yytops.yystates[0];
            yyGLRStackItem yyerror_range[3];
            yyerror_range[1].yystate.yyloc = yys->yyloc;
            yyerror_range[2].yystate.yyloc = yylloc;
            YYLLOC_DEFAULT ((yys->yyloc), yyerror_range, 2);
            yytoken = YYTRANSLATE (yychar);
            yydestruct ("Error: discarding",
                        yytoken, &yylval, &yylloc, context);
            yychar = CYPHER_CYPHER_YYEMPTY;
          }
        yytoken = yygetToken (&yychar, yystackp, context);
        yyj = yypact[yystackp->yytops.yystates[0]->yylrState];
        if (yypact_value_is_default (yyj))
          return;
        yyj += yytoken;
        if (yyj < 0 || YYLAST < yyj || yycheck[yyj] != yytoken)
          {
            if (yydefact[yystackp->yytops.yystates[0]->yylrState] != 0)
              return;
          }
        else if (! yytable_value_is_error (yytable[yyj]))
          return;
      }

  /* Reduce to one stack.  */
  {
    YYPTRDIFF_T yyk;
    for (yyk = 0; yyk < yystackp->yytops.yysize; yyk += 1)
      if (yystackp->yytops.yystates[yyk] != YY_NULLPTR)
        break;
    if (yyk >= yystackp->yytops.yysize)
      yyFail (yystackp, &yylloc, context, YY_NULLPTR);
    for (yyk += 1; yyk < yystackp->yytops.yysize; yyk += 1)
      yymarkStackDeleted (yystackp, yyk);
    yyremoveDeletes (yystackp);
    yycompressStack (yystackp);
  }

  /* Pop stack until we find a state that shifts the error token.  */
  yystackp->yyerrState = 3;
  while (yystackp->yytops.yystates[0] != YY_NULLPTR)
    {
      yyGLRState *yys = yystackp->yytops.yystates[0];
      int yyj = yypact[yys->yylrState];
      if (! yypact_value_is_default (yyj))
        {
          yyj += YYSYMBOL_YYerror;
          if (0 <= yyj && yyj <= YYLAST && yycheck[yyj] == YYSYMBOL_YYerror
              && yyisShiftAction (yytable[yyj]))
            {
              /* Shift the error token.  */
              int yyaction = yytable[yyj];
              /* First adjust its location.*/
              YYLTYPE yyerrloc;
              yystackp->yyerror_range[2].yystate.yyloc = yylloc;
              YYLLOC_DEFAULT (yyerrloc, (yystackp->yyerror_range), 2);
              YY_SYMBOL_PRINT ("Shifting", yy_accessing_symbol (yyaction),
                               &yylval, &yyerrloc);
              yyglrShift (yystackp, 0, yyaction,
                          yys->yyposn, &yylval, &yyerrloc);
              yys = yystackp->yytops.yystates[0];
              break;
            }
        }
      yystackp->yyerror_range[1].yystate.yyloc = yys->yyloc;
      if (yys->yypred != YY_NULLPTR)
        yydestroyGLRState ("Error: popping", yys, context);
      yystackp->yytops.yystates[0] = yys->yypred;
      yystackp->yynextFree -= 1;
      yystackp->yyspaceLeft += 1;
    }
  if (yystackp->yytops.yystates[0] == YY_NULLPTR)
    yyFail (yystackp, &yylloc, context, YY_NULLPTR);
}

#define YYCHK1(YYE)                             \
  do {                                          \
    switch (YYE) {                              \
    case yyok:     break;                       \
    case yyabort:  goto yyabortlab;             \
    case yyaccept: goto yyacceptlab;            \
    case yyerr:    goto yyuser_error;           \
    case yynomem:  goto yyexhaustedlab;         \
    default:       goto yybuglab;               \
    }                                           \
  } while (0)

/*----------.
| yyparse.  |
`----------*/

int
yyparse (cypher_parser_context *context)
{
  int yyresult;
  yyGLRStack yystack;
  yyGLRStack* const yystackp = &yystack;
  YYPTRDIFF_T yyposn;

  YY_DPRINTF ((stderr, "Starting parse\n"));

  yychar = CYPHER_CYPHER_YYEMPTY;
  yylval = yyval_default;
  yylloc = yyloc_default;

  if (! yyinitGLRStack (yystackp, YYINITDEPTH))
    goto yyexhaustedlab;
  switch (YYSETJMP (yystack.yyexception_buffer))
    {
    case 0: break;
    case 1: goto yyabortlab;
    case 2: goto yyexhaustedlab;
    default: goto yybuglab;
    }
  yyglrShift (&yystack, 0, 0, 0, &yylval, &yylloc);
  yyposn = 0;

  while (yytrue)
    {
      /* For efficiency, we have two loops, the first of which is
         specialized to deterministic operation (single stack, no
         potential ambiguity).  */
      /* Standard mode. */
      while (yytrue)
        {
          yy_state_t yystate = yystack.yytops.yystates[0]->yylrState;
          YY_DPRINTF ((stderr, "Entering state %d\n", yystate));
          if (yystate == YYFINAL)
            goto yyacceptlab;
          if (yyisDefaultedState (yystate))
            {
              yyRuleNum yyrule = yydefaultAction (yystate);
              if (yyrule == 0)
                {
                  yystack.yyerror_range[1].yystate.yyloc = yylloc;
                  yyreportSyntaxError (&yystack, context);
                  goto yyuser_error;
                }
              YYCHK1 (yyglrReduce (&yystack, 0, yyrule, yytrue, context));
            }
          else
            {
              yysymbol_kind_t yytoken = yygetToken (&yychar, yystackp, context);
              const short* yyconflicts;
              int yyaction = yygetLRActions (yystate, yytoken, &yyconflicts);
              if (*yyconflicts)
                /* Enter nondeterministic mode.  */
                break;
              if (yyisShiftAction (yyaction))
                {
                  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
                  yychar = CYPHER_CYPHER_YYEMPTY;
                  yyposn += 1;
                  yyglrShift (&yystack, 0, yyaction, yyposn, &yylval, &yylloc);
                  if (0 < yystack.yyerrState)
                    yystack.yyerrState -= 1;
                }
              else if (yyisErrorAction (yyaction))
                {
                  yystack.yyerror_range[1].yystate.yyloc = yylloc;
                  /* Issue an error message unless the scanner already
                     did. */
                  if (yychar != CYPHER_CYPHER_YYerror)
                    yyreportSyntaxError (&yystack, context);
                  goto yyuser_error;
                }
              else
                YYCHK1 (yyglrReduce (&yystack, 0, -yyaction, yytrue, context));
            }
        }

      /* Nondeterministic mode. */
      while (yytrue)
        {
          yysymbol_kind_t yytoken_to_shift;
          YYPTRDIFF_T yys;

          for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
            yystackp->yytops.yylookaheadNeeds[yys] = yychar != CYPHER_CYPHER_YYEMPTY;

          /* yyprocessOneStack returns one of three things:

              - An error flag.  If the caller is yyprocessOneStack, it
                immediately returns as well.  When the caller is finally
                yyparse, it jumps to an error label via YYCHK1.

              - yyok, but yyprocessOneStack has invoked yymarkStackDeleted
                (&yystack, yys), which sets the top state of yys to NULL.  Thus,
                yyparse's following invocation of yyremoveDeletes will remove
                the stack.

              - yyok, when ready to shift a token.

             Except in the first case, yyparse will invoke yyremoveDeletes and
             then shift the next token onto all remaining stacks.  This
             synchronization of the shift (that is, after all preceding
             reductions on all stacks) helps prevent double destructor calls
             on yylval in the event of memory exhaustion.  */

          for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
            YYCHK1 (yyprocessOneStack (&yystack, yys, yyposn, &yylloc, context));
          yyremoveDeletes (&yystack);
          if (yystack.yytops.yysize == 0)
            {
              yyundeleteLastStack (&yystack);
              if (yystack.yytops.yysize == 0)
                yyFail (&yystack, &yylloc, context, YY_("syntax error"));
              YYCHK1 (yyresolveStack (&yystack, context));
              YY_DPRINTF ((stderr, "Returning to deterministic operation.\n"));
              yystack.yyerror_range[1].yystate.yyloc = yylloc;
              yyreportSyntaxError (&yystack, context);
              goto yyuser_error;
            }

          /* If any yyglrShift call fails, it will fail after shifting.  Thus,
             a copy of yylval will already be on stack 0 in the event of a
             failure in the following loop.  Thus, yychar is set to CYPHER_CYPHER_YYEMPTY
             before the loop to make sure the user destructor for yylval isn't
             called twice.  */
          yytoken_to_shift = YYTRANSLATE (yychar);
          yychar = CYPHER_CYPHER_YYEMPTY;
          yyposn += 1;
          for (yys = 0; yys < yystack.yytops.yysize; yys += 1)
            {
              yy_state_t yystate = yystack.yytops.yystates[yys]->yylrState;
              const short* yyconflicts;
              int yyaction = yygetLRActions (yystate, yytoken_to_shift,
                              &yyconflicts);
              /* Note that yyconflicts were handled by yyprocessOneStack.  */
              YY_DPRINTF ((stderr, "On stack %ld, ", YY_CAST (long, yys)));
              YY_SYMBOL_PRINT ("shifting", yytoken_to_shift, &yylval, &yylloc);
              yyglrShift (&yystack, yys, yyaction, yyposn,
                          &yylval, &yylloc);
              YY_DPRINTF ((stderr, "Stack %ld now in state %d\n",
                           YY_CAST (long, yys),
                           yystack.yytops.yystates[yys]->yylrState));
            }

          if (yystack.yytops.yysize == 1)
            {
              YYCHK1 (yyresolveStack (&yystack, context));
              YY_DPRINTF ((stderr, "Returning to deterministic operation.\n"));
              yycompressStack (&yystack);
              break;
            }
        }
      continue;
    yyuser_error:
      yyrecoverSyntaxError (&yystack, context);
      yyposn = yystack.yytops.yystates[0]->yyposn;
    }

 yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;

 yybuglab:
  YY_ASSERT (yyfalse);
  goto yyabortlab;

 yyabortlab:
  yyresult = 1;
  goto yyreturnlab;

 yyexhaustedlab:
  yyerror (&yylloc, context, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;

 yyreturnlab:
  if (yychar != CYPHER_CYPHER_YYEMPTY)
    yydestruct ("Cleanup: discarding lookahead",
                YYTRANSLATE (yychar), &yylval, &yylloc, context);

  /* If the stack is well-formed, pop the stack until it is empty,
     destroying its entries as we go.  But free the stack regardless
     of whether it is well-formed.  */
  if (yystack.yyitems)
    {
      yyGLRState** yystates = yystack.yytops.yystates;
      if (yystates)
        {
          YYPTRDIFF_T yysize = yystack.yytops.yysize;
          YYPTRDIFF_T yyk;
          for (yyk = 0; yyk < yysize; yyk += 1)
            if (yystates[yyk])
              {
                while (yystates[yyk])
                  {
                    yyGLRState *yys = yystates[yyk];
                    yystack.yyerror_range[1].yystate.yyloc = yys->yyloc;
                    if (yys->yypred != YY_NULLPTR)
                      yydestroyGLRState ("Cleanup: popping", yys, context);
                    yystates[yyk] = yys->yypred;
                    yystack.yynextFree -= 1;
                    yystack.yyspaceLeft += 1;
                  }
                break;
              }
        }
      yyfreeGLRStack (&yystack);
    }

  return yyresult;
}

/* DEBUGGING ONLY */
#if CYPHER_YYDEBUG
/* Print *YYS and its predecessors. */
static void
yy_yypstack (yyGLRState* yys)
{
  if (yys->yypred)
    {
      yy_yypstack (yys->yypred);
      YY_FPRINTF ((stderr, " -> "));
    }
  YY_FPRINTF ((stderr, "%d@%ld", yys->yylrState, YY_CAST (long, yys->yyposn)));
}

/* Print YYS (possibly NULL) and its predecessors. */
static void
yypstates (yyGLRState* yys)
{
  if (yys == YY_NULLPTR)
    YY_FPRINTF ((stderr, "<null>"));
  else
    yy_yypstack (yys);
  YY_FPRINTF ((stderr, "\n"));
}

/* Print the stack #YYK.  */
static void
yypstack (yyGLRStack* yystackp, YYPTRDIFF_T yyk)
{
  yypstates (yystackp->yytops.yystates[yyk]);
}

/* Print all the stacks.  */
static void
yypdumpstack (yyGLRStack* yystackp)
{
#define YYINDEX(YYX)                                                    \
  YY_CAST (long,                                                        \
           ((YYX)                                                       \
            ? YY_REINTERPRET_CAST (yyGLRStackItem*, (YYX)) - yystackp->yyitems \
            : -1))

  yyGLRStackItem* yyp;
  for (yyp = yystackp->yyitems; yyp < yystackp->yynextFree; yyp += 1)
    {
      YY_FPRINTF ((stderr, "%3ld. ",
                   YY_CAST (long, yyp - yystackp->yyitems)));
      if (*YY_REINTERPRET_CAST (yybool *, yyp))
        {
          YY_ASSERT (yyp->yystate.yyisState);
          YY_ASSERT (yyp->yyoption.yyisState);
          YY_FPRINTF ((stderr, "Res: %d, LR State: %d, posn: %ld, pred: %ld",
                       yyp->yystate.yyresolved, yyp->yystate.yylrState,
                       YY_CAST (long, yyp->yystate.yyposn),
                       YYINDEX (yyp->yystate.yypred)));
          if (! yyp->yystate.yyresolved)
            YY_FPRINTF ((stderr, ", firstVal: %ld",
                         YYINDEX (yyp->yystate.yysemantics.yyfirstVal)));
        }
      else
        {
          YY_ASSERT (!yyp->yystate.yyisState);
          YY_ASSERT (!yyp->yyoption.yyisState);
          YY_FPRINTF ((stderr, "Option. rule: %d, state: %ld, next: %ld",
                       yyp->yyoption.yyrule - 1,
                       YYINDEX (yyp->yyoption.yystate),
                       YYINDEX (yyp->yyoption.yynext)));
        }
      YY_FPRINTF ((stderr, "\n"));
    }

  YY_FPRINTF ((stderr, "Tops:"));
  {
    YYPTRDIFF_T yyi;
    for (yyi = 0; yyi < yystackp->yytops.yysize; yyi += 1)
      YY_FPRINTF ((stderr, "%ld: %ld; ", YY_CAST (long, yyi),
                   YYINDEX (yystackp->yytops.yystates[yyi])));
    YY_FPRINTF ((stderr, "\n"));
  }
#undef YYINDEX
}
#endif

#undef yylval
#undef yychar
#undef yynerrs
#undef yylloc

/* Substitute the variable and function names.  */
#define yyparse cypher_yyparse
#define yylex   cypher_yylex
#define yyerror cypher_yyerror
#define yylval  cypher_yylval
#define yychar  cypher_yychar
#define yydebug cypher_yydebug
#define yynerrs cypher_yynerrs
#define yylloc  cypher_yylloc


#line 2361 "lib/graphqlite/src/backend/parser/cypher_gram.y"


/* Error handling function */
void cypher_yyerror(CYPHER_YYLTYPE *yylloc, cypher_parser_context *context, const char *msg)
{
    if (!context || !msg) {
        return;
    }

    /* If the scanner already raised a specific error (e.g. IntegerOverflow),
     * keep its message — Bison's generic 'syntax error, unexpected end of
     * file' would otherwise mask it. */
    if (context->has_error && context->error_message) {
        return;
    }

    context->has_error = true;
    context->error_location = yylloc ? yylloc->first_line : -1;
    context->error_column = yylloc ? yylloc->first_column : -1;

    /* Create error message with line number - Bison's detailed error mode
     * provides good context about what was expected */
    char error_buffer[512];
    /* Bison's own error is the openCypher SyntaxError UnexpectedSyntax; two
     * tokens say more: a parameter where a pattern takes only a map literal
     * (`MATCH (n $p)`, Match1 [6]), and a range without `*` in a
     * relationship pattern (`[:T 1..2]`, Match4 [9]). */
    const char *cls = "";
    if (strncmp(msg, "syntax error", 12) == 0) {
        if (strstr(msg, "unexpected PARAMETER, expecting ')'") || strstr(msg, "unexpected PARAMETER, expecting ']'"))
            cls = "SyntaxError: InvalidParameterUse: ";
        else if (strstr(msg, "unexpected DOT_DOT, expecting ']'"))
            cls = "SyntaxError: InvalidRelationshipPattern: ";
        else
            cls = "SyntaxError: UnexpectedSyntax: ";
    }
    if (yylloc && yylloc->first_line > 0) {
        if (yylloc->first_column > 0) {
            snprintf(error_buffer, sizeof(error_buffer),
                     "%sLine %d, Col %d: %s", cls, yylloc->first_line, yylloc->first_column, msg);
        } else {
            snprintf(error_buffer, sizeof(error_buffer),
                     "%sLine %d: %s", cls, yylloc->first_line, msg);
        }
    } else {
        snprintf(error_buffer, sizeof(error_buffer), "%s%s", cls, msg);
    }

    free(context->error_message);
    context->error_message = strdup(error_buffer);
}

/* cypher_yylex function is implemented in cypher_parser.c */
