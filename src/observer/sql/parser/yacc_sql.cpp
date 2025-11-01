/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 2

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 2 "yacc_sql.y"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

#include "common/log/log.h"
#include "common/lang/string.h"
#include "sql/parser/parse_defs.h"
#include "sql/parser/yacc_sql.hpp"
#include "sql/parser/lex_sql.h"
#include "sql/expr/expression.h"

using namespace std;

string token_name(const char *sql_string, YYLTYPE *llocp)
{
  return string(sql_string + llocp->first_column, llocp->last_column - llocp->first_column + 1);
}

int yyerror(YYLTYPE *llocp, const char *sql_string, ParsedSqlResult *sql_result, yyscan_t scanner, const char *msg)
{
  std::unique_ptr<ParsedSqlNode> error_sql_node = std::make_unique<ParsedSqlNode>(SCF_ERROR);
  error_sql_node->error.error_msg = msg;
  error_sql_node->error.line = llocp->first_line;
  error_sql_node->error.column = llocp->first_column;
  sql_result->add_sql_node(std::move(error_sql_node));
  return 0;
}

ArithmeticExpr *create_arithmetic_expression(ArithmeticExpr::Type type,
                                             Expression *left,
                                             Expression *right,
                                             const char *sql_string,
                                             YYLTYPE *llocp)
{
  ArithmeticExpr *expr = new ArithmeticExpr(type, left, right);
  expr->set_name(token_name(sql_string, llocp));
  return expr;
}

UnboundFunctionExpr *create_aggregate_expression(const char *function_name,
                                                 std::vector<std::unique_ptr<Expression>> child,
                                                 const char *sql_string,
                                                 YYLTYPE *llocp)
{
  UnboundFunctionExpr *expr = new UnboundFunctionExpr(function_name, std::move(child));
  expr->set_name(token_name(sql_string, llocp));
  return expr;
}

ParsedSqlNode *create_table_sql_node(char *table_name,
                                     AttrInfoSqlNode* attr_def,
                                     std::vector<AttrInfoSqlNode> *attrinfos,
                                     char* storage_format,
                                     ParsedSqlNode *create_table_select)
{
    ParsedSqlNode *parsed_sql_node = new ParsedSqlNode(SCF_CREATE_TABLE);
    CreateTableSqlNode &create_table = parsed_sql_node->create_table;
    create_table.relation_name = table_name;

    if (attrinfos) {
        create_table.attr_infos.swap(*attrinfos);
        delete attrinfos;
    }
    if (attr_def) {
        create_table.attr_infos.emplace_back(*attr_def);
        std::reverse(create_table.attr_infos.begin(), create_table.attr_infos.end());
        delete attr_def;
    }
    if (storage_format != nullptr) {
        create_table.storage_format = storage_format;
        free(storage_format);
    }

    if (create_table_select) {
        create_table.create_table_select = std::make_unique<SelectSqlNode>(std::move(create_table_select->selection));
    }

    return parsed_sql_node;
}

#line 155 "yacc_sql.cpp"

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

#include "yacc_sql.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_SEMICOLON = 3,                  /* SEMICOLON  */
  YYSYMBOL_AS = 4,                         /* AS  */
  YYSYMBOL_ASC = 5,                        /* ASC  */
  YYSYMBOL_BY = 6,                         /* BY  */
  YYSYMBOL_CREATE = 7,                     /* CREATE  */
  YYSYMBOL_DROP = 8,                       /* DROP  */
  YYSYMBOL_ALTER = 9,                      /* ALTER  */
  YYSYMBOL_EXISTS = 10,                    /* EXISTS  */
  YYSYMBOL_GROUP = 11,                     /* GROUP  */
  YYSYMBOL_HAVING = 12,                    /* HAVING  */
  YYSYMBOL_ORDER = 13,                     /* ORDER  */
  YYSYMBOL_TABLE = 14,                     /* TABLE  */
  YYSYMBOL_TABLES = 15,                    /* TABLES  */
  YYSYMBOL_ADD = 16,                       /* ADD  */
  YYSYMBOL_INDEX = 17,                     /* INDEX  */
  YYSYMBOL_COLUMN = 18,                    /* COLUMN  */
  YYSYMBOL_CALC = 19,                      /* CALC  */
  YYSYMBOL_SELECT = 20,                    /* SELECT  */
  YYSYMBOL_DESC = 21,                      /* DESC  */
  YYSYMBOL_SHOW = 22,                      /* SHOW  */
  YYSYMBOL_SYNC = 23,                      /* SYNC  */
  YYSYMBOL_INSERT = 24,                    /* INSERT  */
  YYSYMBOL_DELETE = 25,                    /* DELETE  */
  YYSYMBOL_UPDATE = 26,                    /* UPDATE  */
  YYSYMBOL_LBRACE = 27,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 28,                    /* RBRACE  */
  YYSYMBOL_LSBRACE = 29,                   /* LSBRACE  */
  YYSYMBOL_RSBRACE = 30,                   /* RSBRACE  */
  YYSYMBOL_COMMA = 31,                     /* COMMA  */
  YYSYMBOL_TRX_BEGIN = 32,                 /* TRX_BEGIN  */
  YYSYMBOL_TRX_COMMIT = 33,                /* TRX_COMMIT  */
  YYSYMBOL_TRX_ROLLBACK = 34,              /* TRX_ROLLBACK  */
  YYSYMBOL_INT_T = 35,                     /* INT_T  */
  YYSYMBOL_IN = 36,                        /* IN  */
  YYSYMBOL_TRUE = 37,                      /* TRUE  */
  YYSYMBOL_FALSE = 38,                     /* FALSE  */
  YYSYMBOL_STRING_T = 39,                  /* STRING_T  */
  YYSYMBOL_FLOAT_T = 40,                   /* FLOAT_T  */
  YYSYMBOL_DATE_T = 41,                    /* DATE_T  */
  YYSYMBOL_TEXT_T = 42,                    /* TEXT_T  */
  YYSYMBOL_VECTOR_T = 43,                  /* VECTOR_T  */
  YYSYMBOL_NOT = 44,                       /* NOT  */
  YYSYMBOL_UNIQUE = 45,                    /* UNIQUE  */
  YYSYMBOL_NULL_T = 46,                    /* NULL_T  */
  YYSYMBOL_LIMIT = 47,                     /* LIMIT  */
  YYSYMBOL_NULLABLE = 48,                  /* NULLABLE  */
  YYSYMBOL_HELP = 49,                      /* HELP  */
  YYSYMBOL_QUOTE = 50,                     /* QUOTE  */
  YYSYMBOL_EXIT = 51,                      /* EXIT  */
  YYSYMBOL_DOT = 52,                       /* DOT  */
  YYSYMBOL_INTO = 53,                      /* INTO  */
  YYSYMBOL_VALUES = 54,                    /* VALUES  */
  YYSYMBOL_FROM = 55,                      /* FROM  */
  YYSYMBOL_WHERE = 56,                     /* WHERE  */
  YYSYMBOL_AND = 57,                       /* AND  */
  YYSYMBOL_OR = 58,                        /* OR  */
  YYSYMBOL_SET = 59,                       /* SET  */
  YYSYMBOL_ON = 60,                        /* ON  */
  YYSYMBOL_INFILE = 61,                    /* INFILE  */
  YYSYMBOL_EXPLAIN = 62,                   /* EXPLAIN  */
  YYSYMBOL_STORAGE = 63,                   /* STORAGE  */
  YYSYMBOL_FORMAT = 64,                    /* FORMAT  */
  YYSYMBOL_INNER = 65,                     /* INNER  */
  YYSYMBOL_JOIN = 66,                      /* JOIN  */
  YYSYMBOL_UNION = 67,                     /* UNION  */
  YYSYMBOL_ALL = 68,                       /* ALL  */
  YYSYMBOL_VIEW = 69,                      /* VIEW  */
  YYSYMBOL_WITH = 70,                      /* WITH  */
  YYSYMBOL_STRING_TO_VECTOR = 71,          /* STRING_TO_VECTOR  */
  YYSYMBOL_VECTOR_TO_STRING = 72,          /* VECTOR_TO_STRING  */
  YYSYMBOL_TOKENIZE = 73,                  /* TOKENIZE  */
  YYSYMBOL_FULLTEXT = 74,                  /* FULLTEXT  */
  YYSYMBOL_PARSER = 75,                    /* PARSER  */
  YYSYMBOL_MATCH = 76,                     /* MATCH  */
  YYSYMBOL_AGAINST = 77,                   /* AGAINST  */
  YYSYMBOL_DISTANCE = 78,                  /* DISTANCE  */
  YYSYMBOL_TYPE = 79,                      /* TYPE  */
  YYSYMBOL_CHANGE = 80,                    /* CHANGE  */
  YYSYMBOL_LISTS = 81,                     /* LISTS  */
  YYSYMBOL_PROBES = 82,                    /* PROBES  */
  YYSYMBOL_IVFFLAT = 83,                   /* IVFFLAT  */
  YYSYMBOL_EQ = 84,                        /* EQ  */
  YYSYMBOL_LT = 85,                        /* LT  */
  YYSYMBOL_GT = 86,                        /* GT  */
  YYSYMBOL_LE = 87,                        /* LE  */
  YYSYMBOL_GE = 88,                        /* GE  */
  YYSYMBOL_NE = 89,                        /* NE  */
  YYSYMBOL_LIKE = 90,                      /* LIKE  */
  YYSYMBOL_IS = 91,                        /* IS  */
  YYSYMBOL_RENAME = 92,                    /* RENAME  */
  YYSYMBOL_TO = 93,                        /* TO  */
  YYSYMBOL_NUMBER = 94,                    /* NUMBER  */
  YYSYMBOL_FLOAT = 95,                     /* FLOAT  */
  YYSYMBOL_ID = 96,                        /* ID  */
  YYSYMBOL_SSS = 97,                       /* SSS  */
  YYSYMBOL_98_ = 98,                       /* '+'  */
  YYSYMBOL_99_ = 99,                       /* '-'  */
  YYSYMBOL_100_ = 100,                     /* '*'  */
  YYSYMBOL_101_ = 101,                     /* '/'  */
  YYSYMBOL_UMINUS = 102,                   /* UMINUS  */
  YYSYMBOL_YYACCEPT = 103,                 /* $accept  */
  YYSYMBOL_commands = 104,                 /* commands  */
  YYSYMBOL_command_wrapper = 105,          /* command_wrapper  */
  YYSYMBOL_exit_stmt = 106,                /* exit_stmt  */
  YYSYMBOL_help_stmt = 107,                /* help_stmt  */
  YYSYMBOL_sync_stmt = 108,                /* sync_stmt  */
  YYSYMBOL_begin_stmt = 109,               /* begin_stmt  */
  YYSYMBOL_commit_stmt = 110,              /* commit_stmt  */
  YYSYMBOL_rollback_stmt = 111,            /* rollback_stmt  */
  YYSYMBOL_drop_table_stmt = 112,          /* drop_table_stmt  */
  YYSYMBOL_alter_table_stmt = 113,         /* alter_table_stmt  */
  YYSYMBOL_change_column_type = 114,       /* change_column_type  */
  YYSYMBOL_change_column_type_body = 115,  /* change_column_type_body  */
  YYSYMBOL_change_column_nullable = 116,   /* change_column_nullable  */
  YYSYMBOL_show_tables_stmt = 117,         /* show_tables_stmt  */
  YYSYMBOL_desc_table_stmt = 118,          /* desc_table_stmt  */
  YYSYMBOL_show_index_stmt = 119,          /* show_index_stmt  */
  YYSYMBOL_create_index_stmt = 120,        /* create_index_stmt  */
  YYSYMBOL_opt_unique = 121,               /* opt_unique  */
  YYSYMBOL_index_type = 122,               /* index_type  */
  YYSYMBOL_vector_index_config = 123,      /* vector_index_config  */
  YYSYMBOL_attr_list = 124,                /* attr_list  */
  YYSYMBOL_drop_index_stmt = 125,          /* drop_index_stmt  */
  YYSYMBOL_create_table_stmt = 126,        /* create_table_stmt  */
  YYSYMBOL_create_view_stmt = 127,         /* create_view_stmt  */
  YYSYMBOL_drop_view_stmt = 128,           /* drop_view_stmt  */
  YYSYMBOL_attr_def_list = 129,            /* attr_def_list  */
  YYSYMBOL_attr_def = 130,                 /* attr_def  */
  YYSYMBOL_nullable_constraint = 131,      /* nullable_constraint  */
  YYSYMBOL_type = 132,                     /* type  */
  YYSYMBOL_insert_stmt = 133,              /* insert_stmt  */
  YYSYMBOL_values_list = 134,              /* values_list  */
  YYSYMBOL_digits = 135,                   /* digits  */
  YYSYMBOL_digits_list = 136,              /* digits_list  */
  YYSYMBOL_value_list = 137,               /* value_list  */
  YYSYMBOL_value = 138,                    /* value  */
  YYSYMBOL_nonnegative_value = 139,        /* nonnegative_value  */
  YYSYMBOL_storage_format = 140,           /* storage_format  */
  YYSYMBOL_delete_stmt = 141,              /* delete_stmt  */
  YYSYMBOL_update_stmt = 142,              /* update_stmt  */
  YYSYMBOL_set_clauses = 143,              /* set_clauses  */
  YYSYMBOL_set_clause = 144,               /* set_clause  */
  YYSYMBOL_select_stmt = 145,              /* select_stmt  */
  YYSYMBOL_select_union_list = 146,        /* select_union_list  */
  YYSYMBOL_select_union_item = 147,        /* select_union_item  */
  YYSYMBOL_select_core = 148,              /* select_core  */
  YYSYMBOL_calc_stmt = 149,                /* calc_stmt  */
  YYSYMBOL_expression_list = 150,          /* expression_list  */
  YYSYMBOL_expression = 151,               /* expression  */
  YYSYMBOL_alias = 152,                    /* alias  */
  YYSYMBOL_func_expr = 153,                /* func_expr  */
  YYSYMBOL_sub_query_expr = 154,           /* sub_query_expr  */
  YYSYMBOL_rel_attr = 155,                 /* rel_attr  */
  YYSYMBOL_relation = 156,                 /* relation  */
  YYSYMBOL_rel_list = 157,                 /* rel_list  */
  YYSYMBOL_join_clauses = 158,             /* join_clauses  */
  YYSYMBOL_where = 159,                    /* where  */
  YYSYMBOL_condition = 160,                /* condition  */
  YYSYMBOL_comp_op = 161,                  /* comp_op  */
  YYSYMBOL_opt_order_by = 162,             /* opt_order_by  */
  YYSYMBOL_sort_list = 163,                /* sort_list  */
  YYSYMBOL_sort_unit = 164,                /* sort_unit  */
  YYSYMBOL_group_by = 165,                 /* group_by  */
  YYSYMBOL_opt_having = 166,               /* opt_having  */
  YYSYMBOL_opt_limit = 167,                /* opt_limit  */
  YYSYMBOL_explain_stmt = 168,             /* explain_stmt  */
  YYSYMBOL_set_variable_stmt = 169,        /* set_variable_stmt  */
  YYSYMBOL_opt_semicolon = 170             /* opt_semicolon  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




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


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

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

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  85
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   494

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  103
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  68
/* YYNRULES -- Number of rules.  */
#define YYNRULES  190
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  401

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   353


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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,   100,    98,     2,    99,     2,   101,     2,     2,
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
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,   102
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   306,   306,   314,   315,   316,   317,   318,   319,   320,
     321,   322,   323,   324,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   335,   336,   340,   346,   351,   357,
     363,   369,   375,   382,   396,   408,   418,   431,   444,   450,
     456,   461,   466,   472,   477,   483,   489,   497,   507,   519,
     535,   536,   540,   547,   554,   563,   575,   581,   590,   600,
     604,   608,   612,   616,   623,   631,   643,   653,   656,   669,
     687,   716,   720,   724,   729,   735,   736,   737,   738,   739,
     740,   744,   754,   768,   774,   781,   785,   789,   793,   801,
     804,   809,   817,   820,   826,   834,   837,   841,   848,   852,
     856,   862,   865,   868,   871,   874,   891,   912,   915,   922,
     934,   948,   953,   960,   970,   982,   985,   998,  1005,  1015,
    1053,  1086,  1092,  1101,  1104,  1113,  1129,  1132,  1135,  1138,
    1141,  1149,  1152,  1157,  1163,  1166,  1169,  1172,  1179,  1182,
    1185,  1190,  1195,  1200,  1205,  1210,  1215,  1233,  1240,  1245,
    1255,  1261,  1271,  1288,  1295,  1307,  1310,  1316,  1320,  1333,
    1337,  1344,  1345,  1346,  1347,  1348,  1349,  1350,  1351,  1352,
    1353,  1354,  1355,  1356,  1357,  1362,  1365,  1373,  1378,  1386,
    1392,  1398,  1408,  1411,  1419,  1422,  1430,  1433,  1441,  1449,
    1460
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "SEMICOLON", "AS",
  "ASC", "BY", "CREATE", "DROP", "ALTER", "EXISTS", "GROUP", "HAVING",
  "ORDER", "TABLE", "TABLES", "ADD", "INDEX", "COLUMN", "CALC", "SELECT",
  "DESC", "SHOW", "SYNC", "INSERT", "DELETE", "UPDATE", "LBRACE", "RBRACE",
  "LSBRACE", "RSBRACE", "COMMA", "TRX_BEGIN", "TRX_COMMIT", "TRX_ROLLBACK",
  "INT_T", "IN", "TRUE", "FALSE", "STRING_T", "FLOAT_T", "DATE_T",
  "TEXT_T", "VECTOR_T", "NOT", "UNIQUE", "NULL_T", "LIMIT", "NULLABLE",
  "HELP", "QUOTE", "EXIT", "DOT", "INTO", "VALUES", "FROM", "WHERE", "AND",
  "OR", "SET", "ON", "INFILE", "EXPLAIN", "STORAGE", "FORMAT", "INNER",
  "JOIN", "UNION", "ALL", "VIEW", "WITH", "STRING_TO_VECTOR",
  "VECTOR_TO_STRING", "TOKENIZE", "FULLTEXT", "PARSER", "MATCH", "AGAINST",
  "DISTANCE", "TYPE", "CHANGE", "LISTS", "PROBES", "IVFFLAT", "EQ", "LT",
  "GT", "LE", "GE", "NE", "LIKE", "IS", "RENAME", "TO", "NUMBER", "FLOAT",
  "ID", "SSS", "'+'", "'-'", "'*'", "'/'", "UMINUS", "$accept", "commands",
  "command_wrapper", "exit_stmt", "help_stmt", "sync_stmt", "begin_stmt",
  "commit_stmt", "rollback_stmt", "drop_table_stmt", "alter_table_stmt",
  "change_column_type", "change_column_type_body",
  "change_column_nullable", "show_tables_stmt", "desc_table_stmt",
  "show_index_stmt", "create_index_stmt", "opt_unique", "index_type",
  "vector_index_config", "attr_list", "drop_index_stmt",
  "create_table_stmt", "create_view_stmt", "drop_view_stmt",
  "attr_def_list", "attr_def", "nullable_constraint", "type",
  "insert_stmt", "values_list", "digits", "digits_list", "value_list",
  "value", "nonnegative_value", "storage_format", "delete_stmt",
  "update_stmt", "set_clauses", "set_clause", "select_stmt",
  "select_union_list", "select_union_item", "select_core", "calc_stmt",
  "expression_list", "expression", "alias", "func_expr", "sub_query_expr",
  "rel_attr", "relation", "rel_list", "join_clauses", "where", "condition",
  "comp_op", "opt_order_by", "sort_list", "sort_unit", "group_by",
  "opt_having", "opt_limit", "explain_stmt", "set_variable_stmt",
  "opt_semicolon", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-244)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-98)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     432,     2,     3,    15,   118,   118,   -48,   163,  -244,    21,
       1,   -12,  -244,  -244,  -244,  -244,  -244,    -7,   432,    94,
     121,  -244,  -244,  -244,  -244,  -244,  -244,  -244,  -244,  -244,
    -244,  -244,  -244,  -244,  -244,  -244,  -244,  -244,  -244,  -244,
    -244,  -244,  -244,  -244,  -244,    32,   120,  -244,    45,   148,
      47,    81,    83,    99,   286,   -57,  -244,  -244,  -244,   171,
     173,   189,   196,   197,  -244,  -244,     8,  -244,   118,  -244,
    -244,  -244,    17,  -244,  -244,  -244,   170,  -244,  -244,   172,
     132,   134,   167,   147,  -244,  -244,  -244,  -244,   165,    -9,
     137,    35,   138,  -244,   176,  -244,    50,   118,   209,   211,
    -244,  -244,    87,  -244,   157,   338,   338,   118,   118,   118,
     118,   -32,  -244,   144,  -244,   118,   118,   118,   118,   210,
     146,   146,     7,   187,   149,   215,     5,  -244,   150,   183,
      20,   188,   229,   154,   191,   159,   238,    -4,   239,   169,
     170,  -244,  -244,  -244,  -244,  -244,   -57,   350,    13,  -244,
      95,   230,   103,   232,   235,   236,   240,   241,  -244,  -244,
    -244,    92,    92,  -244,  -244,   118,  -244,    18,   187,  -244,
     154,   243,   255,  -244,   182,    -1,  -244,   246,   247,   113,
    -244,  -244,   229,  -244,   162,   250,   193,   229,  -244,   192,
    -244,   252,   257,   199,  -244,   204,   150,   261,   206,   207,
    -244,   105,   126,  -244,   215,  -244,  -244,  -244,  -244,   190,
    -244,  -244,  -244,   242,   273,   294,   279,   215,   280,  -244,
    -244,     0,  -244,  -244,  -244,  -244,  -244,  -244,  -244,   272,
      85,   164,   118,   118,   149,  -244,   215,   215,  -244,  -244,
    -244,  -244,  -244,  -244,  -244,  -244,  -244,    19,   150,   289,
     222,  -244,   292,   154,   316,   295,  -244,  -244,   225,   233,
    -244,  -244,   298,   146,   146,   324,   322,   281,   130,   309,
    -244,  -244,  -244,  -244,   118,   255,   255,    52,    52,  -244,
     244,   291,  -244,  -244,  -244,   250,   284,  -244,   154,  -244,
     229,   154,   321,   162,   118,   293,   187,    24,  -244,   118,
     255,   343,   243,  -244,   215,    52,  -244,   303,   333,  -244,
    -244,    39,   335,  -244,   340,   270,  -244,   100,    -2,   255,
     294,  -244,   164,   363,   323,   280,   131,    33,   229,  -244,
     301,  -244,   344,   296,  -244,  -244,  -244,  -244,    28,  -244,
     118,   297,  -244,  -244,  -244,  -244,   346,   304,   361,   312,
      10,  -244,   362,  -244,   141,  -244,   317,    33,   146,  -244,
    -244,   118,   310,   311,   302,  -244,  -244,  -244,   305,   314,
    -244,   368,  -244,   369,   325,   327,   318,   319,   314,   328,
     135,   375,  -244,   326,   331,   329,   334,   215,   215,   377,
     384,   337,   345,   336,   341,   215,   215,   389,   401,  -244,
    -244
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    51,     0,     0,   123,   123,     0,     0,    28,     0,
       0,     0,    29,    30,    31,    27,    26,     0,     0,     0,
       0,    25,    24,    18,    19,    20,    21,     9,    10,    11,
      12,    15,    13,    14,     8,    16,    17,     5,     7,     6,
       3,   115,     4,    22,    23,     0,     0,    50,     0,     0,
       0,     0,     0,     0,   123,    89,   101,   102,   103,     0,
       0,     0,     0,     0,    98,    99,   148,   100,     0,   134,
     132,   121,   138,   136,   137,   133,   122,    46,    45,     0,
       0,     0,     0,     0,   188,     1,   190,     2,   114,   107,
       0,     0,     0,    32,     0,    66,     0,   123,     0,     0,
      85,    87,     0,    90,     0,    92,    92,   123,   123,   123,
     123,     0,   131,     0,   139,     0,     0,     0,     0,   124,
       0,     0,     0,   155,     0,     0,     0,   116,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   147,   130,    86,    88,   104,     0,     0,     0,    93,
     132,     0,     0,     0,     0,     0,     0,     0,   149,   135,
     140,   126,   127,   128,   129,   123,   150,   138,   155,    47,
       0,     0,     0,   109,     0,   155,   111,     0,     0,     0,
     189,    95,     0,   117,     0,    67,     0,     0,    63,     0,
      64,    56,     0,     0,    58,     0,     0,     0,     0,     0,
      91,    98,    99,   105,     0,   143,   106,   144,   145,     0,
     142,   141,   125,     0,   151,   182,     0,    92,    81,   173,
     171,     0,   161,   162,   163,   164,   165,   166,   169,   167,
       0,   156,     0,     0,     0,   110,    92,    92,    96,    97,
     118,    75,    76,    77,    78,    79,    80,    74,     0,     0,
       0,    62,     0,     0,     0,     0,    35,    34,     0,     0,
      37,    94,     0,     0,     0,     0,   184,     0,     0,     0,
     174,   172,   170,   168,     0,     0,     0,   158,   113,   112,
       0,     0,    73,    72,    70,    67,   107,   108,     0,    57,
       0,     0,     0,    39,     0,     0,   155,   138,   152,   123,
       0,   175,     0,    83,    92,   157,   159,   160,     0,    71,
      68,    61,     0,    65,     0,     0,    36,    42,     0,     0,
     182,   183,   185,     0,   186,    82,     0,    74,     0,    60,
       0,    48,     0,     0,    38,    41,    43,   146,   153,   120,
       0,     0,   119,    84,    69,    59,     0,     0,     0,     0,
     179,   176,   177,   187,     0,    49,     0,    44,     0,   181,
     180,     0,     0,     0,     0,    40,   154,   178,     0,     0,
      33,     0,    52,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    53,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    54,
      55
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -244,  -244,   412,  -244,  -244,  -244,  -244,  -244,  -244,  -244,
    -244,  -244,  -244,    74,  -244,  -244,  -244,  -244,  -244,    58,
    -244,  -166,  -244,  -244,  -244,  -244,   158,  -173,  -236,   155,
    -244,   140,   313,  -244,  -105,  -116,   -98,   174,  -244,  -244,
    -244,   227,   -52,  -244,  -244,  -113,  -244,    -5,   -65,   390,
    -244,  -244,  -244,  -115,   203,   110,  -156,  -243,   245,  -244,
     102,  -244,   151,  -244,  -244,  -244,  -244,  -244
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,   316,   334,   335,    29,    30,    31,    32,    49,   373,
     355,   192,    33,    34,    35,    36,   249,   185,   336,   247,
      37,   218,   103,   104,   148,   149,    70,   130,    38,    39,
     175,   176,    40,    88,   127,    41,    42,    71,    72,   214,
      73,    74,    75,   295,   168,   296,   173,   231,   232,   324,
     351,   352,   266,   301,   342,    43,    44,    87
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      76,   152,    98,   112,   216,   167,   169,   150,   150,   180,
     270,   284,   215,   183,   196,   359,    45,    50,   128,   235,
      51,   113,   113,   257,   187,    97,   337,   181,   113,    53,
     234,   360,   306,   307,   170,   110,   271,   100,   101,   132,
      97,   203,   102,   328,   204,    46,   280,    47,    77,    99,
     161,   162,   163,   164,   129,   172,    81,   322,   136,    97,
     111,   171,   133,   281,   158,   282,   137,   283,   159,   240,
     197,    48,    52,   182,    80,   285,   338,   281,   188,   282,
     190,   283,   112,   213,    82,   275,   276,   289,   261,    83,
     272,   344,   140,   349,    85,   219,   115,   116,   117,   118,
     151,   153,   154,   155,   156,   157,   181,   230,   115,   116,
     117,   118,   268,   114,   114,   115,   116,   117,   118,   181,
     114,   220,   312,   -95,    86,   314,   -95,   333,    89,   221,
     138,   206,   152,   -96,   204,   251,   -96,    90,   181,   181,
     320,    91,   139,    93,   281,    54,   282,    55,   283,   297,
     115,   116,   117,   118,   -97,    56,    57,   -97,   303,   343,
     212,   204,   204,   382,    58,    92,   383,   277,   278,   222,
     223,   224,   225,   226,   227,   228,   229,    94,    78,    95,
      79,   143,   144,   115,   116,   117,   118,   145,   146,    59,
      60,    61,   117,   118,    62,    96,    63,   241,   105,   326,
     106,   242,   243,   244,   245,   246,   181,   238,   239,   305,
     230,   230,    64,    65,    66,    67,   107,    68,    69,   362,
     363,   275,   276,   108,   109,   120,   124,   121,   122,   318,
     123,   125,   126,   131,   134,   230,   135,   141,   313,   142,
     160,   165,   166,   172,    55,   174,   184,   186,   189,    97,
     191,   193,    56,    57,   230,   194,   195,   198,   205,   329,
     207,    58,   199,   208,   209,   219,   233,   262,   210,   211,
     217,   389,   390,   236,   237,   350,   345,   250,   258,   397,
     398,   248,    54,   253,    55,   254,   177,   178,   252,   181,
     181,   220,    56,    57,   321,   255,   350,   181,   181,   221,
     256,    58,   259,   260,   264,   265,    97,   267,   263,    64,
      65,   269,    67,    54,   179,    55,   273,   286,   287,   288,
     290,   292,   291,    56,    57,   294,    59,    60,    61,   293,
     299,    62,    58,    63,   300,   302,   304,   309,   308,   222,
     223,   224,   225,   226,   227,   228,   229,   129,   315,    64,
      65,    66,    67,   319,    68,    69,   323,    59,    60,    61,
     275,   327,    62,   330,    63,    54,   332,    55,   331,   340,
     341,   346,   347,   354,   356,    56,    57,    54,   358,    55,
      64,    65,    66,    67,    58,    68,    69,    56,    57,   357,
     348,   353,   364,   361,   368,   369,    58,   372,   370,   374,
     375,   371,   378,   379,   376,   377,   384,   385,   391,    59,
      60,    61,   386,   387,    62,   392,    63,   399,   388,   393,
     395,    59,    60,    61,   381,   396,    62,   394,    63,   400,
      84,   365,    64,    65,    66,    67,   380,   147,    69,     1,
       2,     3,   325,   310,   201,   202,    66,    67,   317,    68,
      69,     4,     5,     6,     7,     8,     9,    10,    11,   200,
     311,   279,   119,   367,    12,    13,    14,   298,   366,     0,
       0,   339,     0,     0,     0,   274,     0,     0,     0,     0,
       0,    15,     0,    16,     0,     0,     0,     0,     0,     0,
       0,    17,     0,     0,    18
};

static const yytype_int16 yycheck[] =
{
       5,   106,    54,    68,   170,   120,   121,   105,   106,   125,
      10,   247,   168,   126,    18,     5,    14,    14,    27,   175,
      17,     4,     4,   196,     4,    20,    28,   125,     4,    14,
      31,    21,   275,   276,    27,    27,    36,    94,    95,     4,
      20,    28,    99,     4,    31,    43,    27,    45,    96,    54,
     115,   116,   117,   118,    63,    56,    55,   300,     8,    20,
      52,    54,    27,    44,    96,    46,    16,    48,   100,   182,
      74,    69,    69,    68,    53,   248,   319,    44,   130,    46,
     132,    48,   147,    65,    96,    57,    58,   253,   204,    96,
      90,   327,    97,    65,     0,    10,    98,    99,   100,   101,
     105,   106,   107,   108,   109,   110,   204,   172,    98,    99,
     100,   101,   217,    96,    96,    98,    99,   100,   101,   217,
      96,    36,   288,    28,     3,   291,    31,    27,    96,    44,
      80,    28,   237,    28,    31,   187,    31,    17,   236,   237,
     296,    96,    92,    96,    44,    27,    46,    29,    48,   264,
      98,    99,   100,   101,    28,    37,    38,    31,    28,    28,
     165,    31,    31,    28,    46,    17,    31,   232,   233,    84,
      85,    86,    87,    88,    89,    90,    91,    96,    15,    96,
      17,    94,    95,    98,    99,   100,   101,    30,    31,    71,
      72,    73,   100,   101,    76,    96,    78,    35,    27,   304,
      27,    39,    40,    41,    42,    43,   304,    94,    95,   274,
     275,   276,    94,    95,    96,    97,    27,    99,   100,    78,
      79,    57,    58,    27,    27,    55,    59,    55,    96,   294,
      96,    84,    67,    96,    96,   300,    60,    28,   290,    28,
      96,    31,    96,    56,    29,    96,    96,    64,    60,    20,
      96,    60,    37,    38,   319,    96,    18,    18,    28,   311,
      28,    46,    93,    28,    28,    10,    84,    77,    28,    28,
      27,   387,   388,    27,    27,   340,   328,    84,    17,   395,
     396,    31,    27,    31,    29,    28,    71,    72,    96,   387,
     388,    36,    37,    38,   299,    96,   361,   395,   396,    44,
      96,    46,    96,    96,    31,    11,    20,    28,    66,    94,
      95,    31,    97,    27,    99,    29,    44,    28,    96,    27,
       4,    96,    27,    37,    38,    27,    71,    72,    73,    96,
       6,    76,    46,    78,    12,    54,    27,    46,    94,    84,
      85,    86,    87,    88,    89,    90,    91,    63,    27,    94,
      95,    96,    97,    60,    99,   100,    13,    71,    72,    73,
      57,    28,    76,    28,    78,    27,    96,    29,    28,     6,
      47,    70,    28,    27,    70,    37,    38,    27,    66,    29,
      94,    95,    96,    97,    46,    99,   100,    37,    38,    28,
      94,    94,    75,    31,    84,    84,    46,    83,    96,    31,
      31,    96,    84,    84,    79,    78,    31,    81,    31,    71,
      72,    73,    81,    84,    76,    31,    78,    28,    84,    82,
      84,    71,    72,    73,    96,    84,    76,    82,    78,    28,
      18,   357,    94,    95,    96,    97,   378,    99,   100,     7,
       8,     9,   302,   285,    94,    95,    96,    97,   293,    99,
     100,    19,    20,    21,    22,    23,    24,    25,    26,   146,
     286,   234,    72,   361,    32,    33,    34,   264,   358,    -1,
      -1,   320,    -1,    -1,    -1,   230,    -1,    -1,    -1,    -1,
      -1,    49,    -1,    51,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    59,    -1,    -1,    62
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     7,     8,     9,    19,    20,    21,    22,    23,    24,
      25,    26,    32,    33,    34,    49,    51,    59,    62,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   117,
     118,   119,   120,   125,   126,   127,   128,   133,   141,   142,
     145,   148,   149,   168,   169,    14,    43,    45,    69,   121,
      14,    17,    69,    14,    27,    29,    37,    38,    46,    71,
      72,    73,    76,    78,    94,    95,    96,    97,    99,   100,
     139,   150,   151,   153,   154,   155,   150,    96,    15,    17,
      53,    55,    96,    96,   105,     0,     3,   170,   146,    96,
      17,    96,    17,    96,    96,    96,    96,    20,   145,   150,
      94,    95,    99,   135,   136,    27,    27,    27,    27,    27,
      27,    52,   151,     4,    96,    98,    99,   100,   101,   152,
      55,    55,    96,    96,    59,    84,    67,   147,    27,    63,
     140,    96,     4,    27,    96,    60,     8,    16,    80,    92,
     150,    28,    28,    94,    95,    30,    31,    99,   137,   138,
     139,   150,   137,   150,   150,   150,   150,   150,    96,   100,
      96,   151,   151,   151,   151,    31,    96,   156,   157,   156,
      27,    54,    56,   159,    96,   143,   144,    71,    72,    99,
     138,   139,    68,   148,    96,   130,    64,     4,   145,    60,
     145,    96,   124,    60,    96,    18,    18,    74,    18,    93,
     135,    94,    95,    28,    31,    28,    28,    28,    28,    28,
      28,    28,   150,    65,   152,   159,   124,    27,   134,    10,
      36,    44,    84,    85,    86,    87,    88,    89,    90,    91,
     151,   160,   161,    84,    31,   159,    27,    27,    94,    95,
     148,    35,    39,    40,    41,    42,    43,   132,    31,   129,
      84,   145,    96,    31,    28,    96,    96,   130,    17,    96,
      96,   138,    77,    66,    31,    11,   165,    28,   137,    31,
      10,    36,    90,    44,   161,    57,    58,   151,   151,   144,
      27,    44,    46,    48,   131,   130,    28,    96,    27,   124,
       4,    27,    96,    96,    27,   156,   158,   156,   157,     6,
      12,   166,    54,    28,    27,   151,   160,   160,    94,    46,
     129,   140,   124,   145,   124,    27,   114,   132,   151,    60,
     159,   150,   160,    13,   162,   134,   137,    28,     4,   145,
      28,    28,    96,    27,   115,   116,   131,    28,   160,   165,
       6,    47,   167,    28,   131,   145,    70,    28,    94,    65,
     151,   163,   164,    94,    27,   123,    70,    28,    66,     5,
      21,    31,    78,    79,    75,   116,   158,   163,    84,    84,
      96,    96,    83,   122,    31,    31,    79,    78,    84,    84,
     122,    96,    28,    31,    31,    81,    81,    84,    84,   138,
     138,    31,    31,    82,    82,    84,    84,   138,   138,    28,
      28
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,   103,   104,   105,   105,   105,   105,   105,   105,   105,
     105,   105,   105,   105,   105,   105,   105,   105,   105,   105,
     105,   105,   105,   105,   105,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   113,   113,   113,   113,   114,   114,
     115,   115,   115,   116,   116,   117,   118,   119,   120,   120,
     121,   121,   122,   123,   123,   123,   124,   124,   125,   126,
     126,   126,   126,   126,   127,   127,   128,   129,   129,   130,
     130,   131,   131,   131,   131,   132,   132,   132,   132,   132,
     132,   133,   133,   134,   134,   135,   135,   135,   135,   136,
     136,   136,   137,   137,   137,   138,   138,   138,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   140,   140,   141,
     142,   143,   143,   144,   145,   146,   146,   147,   147,   148,
     148,   149,   149,   150,   150,   150,   151,   151,   151,   151,
     151,   151,   151,   151,   151,   151,   151,   151,   152,   152,
     152,   153,   153,   153,   153,   153,   153,   154,   155,   155,
     156,   157,   157,   158,   158,   159,   159,   160,   160,   160,
     160,   161,   161,   161,   161,   161,   161,   161,   161,   161,
     161,   161,   161,   161,   161,   162,   162,   163,   163,   164,
     164,   164,   165,   165,   166,   166,   167,   167,   168,   169,
     170
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,    13,     6,     6,     8,     6,     2,     0,
       4,     1,     0,     1,     0,     2,     2,     4,     9,    11,
       1,     0,     1,     9,    17,    17,     1,     3,     5,    10,
       9,     8,     6,     5,     5,     8,     3,     0,     3,     6,
       3,     2,     1,     1,     0,     1,     1,     1,     1,     1,
       1,     5,     8,     3,     5,     1,     2,     1,     2,     0,
       1,     3,     0,     1,     3,     1,     2,     2,     1,     1,
       1,     1,     1,     1,     3,     4,     4,     0,     4,     4,
       5,     1,     3,     3,     2,     0,     2,     2,     3,     9,
       9,     2,     2,     0,     2,     4,     3,     3,     3,     3,
       3,     2,     1,     1,     1,     3,     1,     1,     0,     1,
       2,     4,     4,     4,     4,     4,     8,     3,     1,     3,
       1,     2,     4,     3,     6,     0,     2,     3,     2,     3,
       3,     1,     1,     1,     1,     1,     1,     1,     2,     1,
       2,     1,     2,     1,     2,     0,     3,     1,     3,     1,
       2,     2,     0,     3,     0,     2,     0,     2,     2,     4,
       1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (&yylloc, sql_string, sql_result, scanner, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

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

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

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


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location, sql_string, sql_result, scanner); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  YY_USE (sql_string);
  YY_USE (sql_result);
  YY_USE (scanner);
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
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp, sql_string, sql_result, scanner);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule, const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]), sql_string, sql_result, scanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, sql_string, sql_result, scanner); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
  YYLTYPE *yylloc;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
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




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
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

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
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
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
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
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  YY_USE (sql_string);
  YY_USE (sql_result);
  YY_USE (scanner);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, &yylloc, scanner);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* commands: command_wrapper opt_semicolon  */
#line 307 "yacc_sql.y"
  {
    std::unique_ptr<ParsedSqlNode> sql_node = std::unique_ptr<ParsedSqlNode>((yyvsp[-1].sql_node));
    sql_result->add_sql_node(std::move(sql_node));
  }
#line 2028 "yacc_sql.cpp"
    break;

  case 26: /* exit_stmt: EXIT  */
#line 340 "yacc_sql.y"
         {
      (void)yynerrs;  // 这么写为了消除yynerrs未使用的告警。如果你有更好的方法欢迎提PR
      (yyval.sql_node) = new ParsedSqlNode(SCF_EXIT);
    }
#line 2037 "yacc_sql.cpp"
    break;

  case 27: /* help_stmt: HELP  */
#line 346 "yacc_sql.y"
         {
      (yyval.sql_node) = new ParsedSqlNode(SCF_HELP);
    }
#line 2045 "yacc_sql.cpp"
    break;

  case 28: /* sync_stmt: SYNC  */
#line 351 "yacc_sql.y"
         {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SYNC);
    }
#line 2053 "yacc_sql.cpp"
    break;

  case 29: /* begin_stmt: TRX_BEGIN  */
#line 357 "yacc_sql.y"
               {
      (yyval.sql_node) = new ParsedSqlNode(SCF_BEGIN);
    }
#line 2061 "yacc_sql.cpp"
    break;

  case 30: /* commit_stmt: TRX_COMMIT  */
#line 363 "yacc_sql.y"
               {
      (yyval.sql_node) = new ParsedSqlNode(SCF_COMMIT);
    }
#line 2069 "yacc_sql.cpp"
    break;

  case 31: /* rollback_stmt: TRX_ROLLBACK  */
#line 369 "yacc_sql.y"
                  {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ROLLBACK);
    }
#line 2077 "yacc_sql.cpp"
    break;

  case 32: /* drop_table_stmt: DROP TABLE ID  */
#line 375 "yacc_sql.y"
                  {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_TABLE);
      (yyval.sql_node)->drop_table.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2087 "yacc_sql.cpp"
    break;

  case 33: /* alter_table_stmt: ALTER TABLE ID ADD FULLTEXT INDEX ID LBRACE ID RBRACE WITH PARSER ID  */
#line 383 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ALTER_TABLE);
      AlterTableSqlNode &alter_table = (yyval.sql_node)->alter_table;
      alter_table.table_name         = (yyvsp[-10].string);
      alter_table.alter_type         = AlterType::ADD_FULLTEXT_INDEX;
      alter_table.fulltext_index_config.index_name = (yyvsp[-6].string);
      alter_table.fulltext_index_config.column_name = (yyvsp[-4].string);
      alter_table.fulltext_index_config.parser = (yyvsp[0].string);
      free((yyvsp[-10].string));
      free((yyvsp[-6].string));
      free((yyvsp[-4].string));
      free((yyvsp[0].string));
    }
#line 2105 "yacc_sql.cpp"
    break;

  case 34: /* alter_table_stmt: ALTER TABLE ID ADD COLUMN attr_def  */
#line 397 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ALTER_TABLE);
      AlterTableSqlNode &alter_table = (yyval.sql_node)->alter_table;
      alter_table.table_name         = (yyvsp[-3].string);
      alter_table.alter_type         = AlterType::ADD_COLUMN;
      if ((yyvsp[0].attr_info) != nullptr) {
        alter_table.new_column = *(yyvsp[0].attr_info);
        delete (yyvsp[0].attr_info);
      }
      free((yyvsp[-3].string));
    }
#line 2121 "yacc_sql.cpp"
    break;

  case 35: /* alter_table_stmt: ALTER TABLE ID DROP COLUMN ID  */
#line 409 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ALTER_TABLE);
      AlterTableSqlNode &alter_table = (yyval.sql_node)->alter_table;
      alter_table.table_name         = (yyvsp[-3].string);
      alter_table.alter_type         = AlterType::DROP_COLUMN;
      alter_table.column_name        = (yyvsp[0].string);
      free((yyvsp[-3].string));
      free((yyvsp[0].string));
    }
#line 2135 "yacc_sql.cpp"
    break;

  case 36: /* alter_table_stmt: ALTER TABLE ID CHANGE COLUMN ID ID change_column_type  */
#line 419 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ALTER_TABLE);
      AlterTableSqlNode &alter_table = (yyval.sql_node)->alter_table;
      alter_table.table_name         = (yyvsp[-5].string);
      alter_table.alter_type         = AlterType::CHANGE_COLUMN;
      alter_table.column_name        = (yyvsp[-2].string);
      alter_table.new_column_name    = (yyvsp[-1].string);
      (void)(yyvsp[0].nullable_info);
      free((yyvsp[-5].string));
      free((yyvsp[-2].string));
      free((yyvsp[-1].string));
    }
#line 2152 "yacc_sql.cpp"
    break;

  case 37: /* alter_table_stmt: ALTER TABLE ID RENAME TO ID  */
#line 432 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ALTER_TABLE);
      AlterTableSqlNode &alter_table = (yyval.sql_node)->alter_table;
      alter_table.table_name         = (yyvsp[-3].string);
      alter_table.alter_type         = AlterType::RENAME_TABLE;
      alter_table.new_table_name     = (yyvsp[0].string);
      free((yyvsp[-3].string));
      free((yyvsp[0].string));
    }
#line 2166 "yacc_sql.cpp"
    break;

  case 38: /* change_column_type: type change_column_type_body  */
#line 445 "yacc_sql.y"
    {
      (void)(yyvsp[-1].number);
      (yyval.nullable_info) = (yyvsp[0].nullable_info);
    }
#line 2175 "yacc_sql.cpp"
    break;

  case 39: /* change_column_type: %empty  */
#line 450 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;
    }
#line 2183 "yacc_sql.cpp"
    break;

  case 40: /* change_column_type_body: LBRACE NUMBER RBRACE change_column_nullable  */
#line 457 "yacc_sql.y"
    {
      (void)(yyvsp[-2].number);
      (yyval.nullable_info) = (yyvsp[0].nullable_info);
    }
#line 2192 "yacc_sql.cpp"
    break;

  case 41: /* change_column_type_body: change_column_nullable  */
#line 462 "yacc_sql.y"
    {
      (yyval.nullable_info) = (yyvsp[0].nullable_info);
    }
#line 2200 "yacc_sql.cpp"
    break;

  case 42: /* change_column_type_body: %empty  */
#line 466 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;
    }
#line 2208 "yacc_sql.cpp"
    break;

  case 43: /* change_column_nullable: nullable_constraint  */
#line 473 "yacc_sql.y"
    {
      (yyval.nullable_info) = (yyvsp[0].nullable_info);
    }
#line 2216 "yacc_sql.cpp"
    break;

  case 44: /* change_column_nullable: %empty  */
#line 477 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;
    }
#line 2224 "yacc_sql.cpp"
    break;

  case 45: /* show_tables_stmt: SHOW TABLES  */
#line 483 "yacc_sql.y"
                {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SHOW_TABLES);
    }
#line 2232 "yacc_sql.cpp"
    break;

  case 46: /* desc_table_stmt: DESC ID  */
#line 489 "yacc_sql.y"
             {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DESC_TABLE);
      (yyval.sql_node)->desc_table.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2242 "yacc_sql.cpp"
    break;

  case 47: /* show_index_stmt: SHOW INDEX FROM relation  */
#line 498 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SHOW_INDEX);
      ShowIndexSqlNode &show_index = (yyval.sql_node)->show_index;
      show_index.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2253 "yacc_sql.cpp"
    break;

  case 48: /* create_index_stmt: CREATE opt_unique INDEX ID ON ID LBRACE attr_list RBRACE  */
#line 508 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CREATE_INDEX);
      CreateIndexSqlNode &create_index = (yyval.sql_node)->create_index;
      create_index.unique = (yyvsp[-7].unique); // 用 opt_unique 的返回值来确定是否 UNIQUE
      create_index.index_name = (yyvsp[-5].string);
      create_index.relation_name = (yyvsp[-3].string);
      create_index.attribute_name.swap(*(yyvsp[-1].index_attr_list)); // $8 是 vector<string> 类型
      delete (yyvsp[-1].index_attr_list); // 释放指针
      free((yyvsp[-5].string));
      free((yyvsp[-3].string));
    }
#line 2269 "yacc_sql.cpp"
    break;

  case 49: /* create_index_stmt: CREATE VECTOR_T INDEX ID ON ID LBRACE attr_list RBRACE WITH vector_index_config  */
#line 520 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CREATE_INDEX);
      CreateIndexSqlNode &create_index = (yyval.sql_node)->create_index;
      create_index.unique = false; // 向量索引不支持
      create_index.index_name = (yyvsp[-7].string);
      create_index.relation_name = (yyvsp[-5].string);
      create_index.attribute_name.swap(*(yyvsp[-3].index_attr_list)); // $8 是 vector<string> 类型
      create_index.vector_index_config = std::move(*(yyvsp[0].vector_index_config));
      delete (yyvsp[-3].index_attr_list); // 释放指针
      free((yyvsp[-7].string));
      free((yyvsp[-5].string));
    }
#line 2286 "yacc_sql.cpp"
    break;

  case 50: /* opt_unique: UNIQUE  */
#line 535 "yacc_sql.y"
           { (yyval.unique) = true; }
#line 2292 "yacc_sql.cpp"
    break;

  case 51: /* opt_unique: %empty  */
#line 536 "yacc_sql.y"
                { (yyval.unique) = false; }
#line 2298 "yacc_sql.cpp"
    break;

  case 52: /* index_type: IVFFLAT  */
#line 541 "yacc_sql.y"
    {
      (yyval.index_type) = IndexType::VectorIVFFlatIndex;
    }
#line 2306 "yacc_sql.cpp"
    break;

  case 53: /* vector_index_config: LBRACE DISTANCE EQ ID COMMA TYPE EQ index_type RBRACE  */
#line 548 "yacc_sql.y"
    {
      (yyval.vector_index_config) = new VectorIndexConfig;
      (yyval.vector_index_config)->distance_fn = (yyvsp[-5].string);
      (yyval.vector_index_config)->index_type = (yyvsp[-1].index_type);
      free((yyvsp[-5].string));
    }
#line 2317 "yacc_sql.cpp"
    break;

  case 54: /* vector_index_config: LBRACE DISTANCE EQ ID COMMA TYPE EQ index_type COMMA LISTS EQ value COMMA PROBES EQ value RBRACE  */
#line 555 "yacc_sql.y"
    {
      (yyval.vector_index_config) = new VectorIndexConfig;
      (yyval.vector_index_config)->distance_fn = (yyvsp[-13].string);
      (yyval.vector_index_config)->index_type = (yyvsp[-9].index_type);
      (yyval.vector_index_config)->lists = std::move(*(yyvsp[-5].value));
      (yyval.vector_index_config)->probes = std::move(*(yyvsp[-1].value));
      free((yyvsp[-13].string));
    }
#line 2330 "yacc_sql.cpp"
    break;

  case 55: /* vector_index_config: LBRACE TYPE EQ index_type COMMA DISTANCE EQ ID COMMA LISTS EQ value COMMA PROBES EQ value RBRACE  */
#line 564 "yacc_sql.y"
    {
      (yyval.vector_index_config) = new VectorIndexConfig;
      (yyval.vector_index_config)->distance_fn = (yyvsp[-9].string);
      (yyval.vector_index_config)->index_type = (yyvsp[-13].index_type);
      (yyval.vector_index_config)->lists = std::move(*(yyvsp[-5].value));
      (yyval.vector_index_config)->probes = std::move(*(yyvsp[-1].value));
      free((yyvsp[-9].string));
    }
#line 2343 "yacc_sql.cpp"
    break;

  case 56: /* attr_list: ID  */
#line 576 "yacc_sql.y"
    {
      (yyval.index_attr_list) = new std::vector<std::string>; // 创建一个新的 vector
      (yyval.index_attr_list)->emplace_back((yyvsp[0].string)); // 将列名加入 vector
      free((yyvsp[0].string));
    }
#line 2353 "yacc_sql.cpp"
    break;

  case 57: /* attr_list: ID COMMA attr_list  */
#line 582 "yacc_sql.y"
    {
      (yyval.index_attr_list) = (yyvsp[0].index_attr_list); // 使用现有的 vector
      (yyval.index_attr_list)->emplace((yyval.index_attr_list)->begin(), (yyvsp[-2].string)); // 将新列名加入 vector 开头
      free((yyvsp[-2].string));
    }
#line 2363 "yacc_sql.cpp"
    break;

  case 58: /* drop_index_stmt: DROP INDEX ID ON ID  */
#line 591 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_INDEX);
      (yyval.sql_node)->drop_index.index_name = (yyvsp[-2].string);
      (yyval.sql_node)->drop_index.relation_name = (yyvsp[0].string);
      free((yyvsp[-2].string));
      free((yyvsp[0].string));
    }
#line 2375 "yacc_sql.cpp"
    break;

  case 59: /* create_table_stmt: CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE storage_format AS select_stmt  */
#line 601 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-7].string), (yyvsp[-5].attr_info), (yyvsp[-4].attr_infos), (yyvsp[-2].string), (yyvsp[0].sql_node));
    }
#line 2383 "yacc_sql.cpp"
    break;

  case 60: /* create_table_stmt: CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE storage_format select_stmt  */
#line 605 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-6].string), (yyvsp[-4].attr_info), (yyvsp[-3].attr_infos), (yyvsp[-1].string), (yyvsp[0].sql_node));
    }
#line 2391 "yacc_sql.cpp"
    break;

  case 61: /* create_table_stmt: CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE storage_format  */
#line 609 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-5].string), (yyvsp[-3].attr_info), (yyvsp[-2].attr_infos), (yyvsp[0].string), nullptr);
    }
#line 2399 "yacc_sql.cpp"
    break;

  case 62: /* create_table_stmt: CREATE TABLE ID storage_format AS select_stmt  */
#line 613 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-3].string), nullptr, nullptr, (yyvsp[-2].string), (yyvsp[0].sql_node));
    }
#line 2407 "yacc_sql.cpp"
    break;

  case 63: /* create_table_stmt: CREATE TABLE ID storage_format select_stmt  */
#line 617 "yacc_sql.y"
    {
      (yyval.sql_node) = create_table_sql_node((yyvsp[-2].string), nullptr, nullptr, (yyvsp[-1].string), (yyvsp[0].sql_node));
    }
#line 2415 "yacc_sql.cpp"
    break;

  case 64: /* create_view_stmt: CREATE VIEW ID AS select_stmt  */
#line 624 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CREATE_VIEW);
      CreateViewSqlNode &create_view = (yyval.sql_node)->create_view;
      create_view.relation_name = (yyvsp[-2].string);
      create_view.create_view_select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      free((yyvsp[-2].string));
    }
#line 2427 "yacc_sql.cpp"
    break;

  case 65: /* create_view_stmt: CREATE VIEW ID LBRACE attr_list RBRACE AS select_stmt  */
#line 632 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CREATE_VIEW);
      CreateViewSqlNode &create_view = (yyval.sql_node)->create_view;
      create_view.relation_name = (yyvsp[-5].string);
      create_view.attribute_names = std::move(*(yyvsp[-3].index_attr_list));
      create_view.create_view_select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      free((yyvsp[-5].string));
    }
#line 2440 "yacc_sql.cpp"
    break;

  case 66: /* drop_view_stmt: DROP VIEW ID  */
#line 644 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_VIEW);
      (yyval.sql_node)->drop_view.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2450 "yacc_sql.cpp"
    break;

  case 67: /* attr_def_list: %empty  */
#line 653 "yacc_sql.y"
    {
      (yyval.attr_infos) = nullptr;
    }
#line 2458 "yacc_sql.cpp"
    break;

  case 68: /* attr_def_list: COMMA attr_def attr_def_list  */
#line 657 "yacc_sql.y"
    {
      if ((yyvsp[0].attr_infos) != nullptr) {
        (yyval.attr_infos) = (yyvsp[0].attr_infos);
      } else {
        (yyval.attr_infos) = new std::vector<AttrInfoSqlNode>;
      }
      (yyval.attr_infos)->emplace_back(*(yyvsp[-1].attr_info));
      delete (yyvsp[-1].attr_info);
    }
#line 2472 "yacc_sql.cpp"
    break;

  case 69: /* attr_def: ID type LBRACE NUMBER RBRACE nullable_constraint  */
#line 670 "yacc_sql.y"
    {
      (yyval.attr_info) = new AttrInfoSqlNode;
      (yyval.attr_info)->name = (yyvsp[-5].string);
      (yyval.attr_info)->type = (AttrType)(yyvsp[-4].number);
      if ((yyval.attr_info)->type == AttrType::CHARS) {
        (yyval.attr_info)->length = (yyvsp[-2].number);
      } else if ((yyval.attr_info)->type == AttrType::VECTORS) {
        (yyval.attr_info)->length = sizeof(float) * (yyvsp[-2].number);
      } else {
        ASSERT(false, "$$->type is invalid.");
      }
      (yyval.attr_info)->nullable = (yyvsp[0].nullable_info);
      if ((yyval.attr_info)->nullable) {
        (yyval.attr_info)->length++;
      }
      free((yyvsp[-5].string));
    }
#line 2494 "yacc_sql.cpp"
    break;

  case 70: /* attr_def: ID type nullable_constraint  */
#line 688 "yacc_sql.y"
    {
      (yyval.attr_info) = new AttrInfoSqlNode;
      (yyval.attr_info)->type = (AttrType)(yyvsp[-1].number);
      (yyval.attr_info)->name = (yyvsp[-2].string);
      if ((yyval.attr_info)->type == AttrType::INTS) {
        (yyval.attr_info)->length = sizeof(int);
      } else if ((yyval.attr_info)->type == AttrType::FLOATS) {
        (yyval.attr_info)->length = sizeof(float);
      } else if ((yyval.attr_info)->type == AttrType::DATES) {
        (yyval.attr_info)->length = sizeof(int);
      } else if ((yyval.attr_info)->type == AttrType::CHARS) {
        (yyval.attr_info)->length = sizeof(char);
      } else if ((yyval.attr_info)->type == AttrType::VECTORS) {
        (yyval.attr_info)->length = sizeof(float) * 1;
      } else if ((yyval.attr_info)->type == AttrType::TEXTS) {
        (yyval.attr_info)->length = 65535;
      } else {
        ASSERT(false, "$$->type is invalid.");
      }
      (yyval.attr_info)->nullable = (yyvsp[0].nullable_info);  // 处理NULL/NOT NULL标记
      if ((yyval.attr_info)->nullable) {
        (yyval.attr_info)->length++;
      }
      free((yyvsp[-2].string));
    }
#line 2524 "yacc_sql.cpp"
    break;

  case 71: /* nullable_constraint: NOT NULL_T  */
#line 717 "yacc_sql.y"
    {
      (yyval.nullable_info) = false;  // NOT NULL 对应的可空性为 false
    }
#line 2532 "yacc_sql.cpp"
    break;

  case 72: /* nullable_constraint: NULLABLE  */
#line 721 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;  // NULLABLE 对应的可空性为 true 2022
    }
#line 2540 "yacc_sql.cpp"
    break;

  case 73: /* nullable_constraint: NULL_T  */
#line 725 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;  // NULL 对应的可空性也为 true 2023
    }
#line 2548 "yacc_sql.cpp"
    break;

  case 74: /* nullable_constraint: %empty  */
#line 729 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;  // 默认情况为 NULL
    }
#line 2556 "yacc_sql.cpp"
    break;

  case 75: /* type: INT_T  */
#line 735 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::INTS);   }
#line 2562 "yacc_sql.cpp"
    break;

  case 76: /* type: STRING_T  */
#line 736 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::CHARS);  }
#line 2568 "yacc_sql.cpp"
    break;

  case 77: /* type: FLOAT_T  */
#line 737 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::FLOATS); }
#line 2574 "yacc_sql.cpp"
    break;

  case 78: /* type: DATE_T  */
#line 738 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::DATES);  }
#line 2580 "yacc_sql.cpp"
    break;

  case 79: /* type: TEXT_T  */
#line 739 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::TEXTS);  }
#line 2586 "yacc_sql.cpp"
    break;

  case 80: /* type: VECTOR_T  */
#line 740 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::VECTORS);  }
#line 2592 "yacc_sql.cpp"
    break;

  case 81: /* insert_stmt: INSERT INTO ID VALUES values_list  */
#line 745 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_INSERT);
      (yyval.sql_node)->insertion.relation_name = (yyvsp[-2].string);
      if ((yyvsp[0].values_list) != nullptr) {
        (yyval.sql_node)->insertion.values_list.swap(*(yyvsp[0].values_list));
        delete (yyvsp[0].values_list);
      }
      free((yyvsp[-2].string));
    }
#line 2606 "yacc_sql.cpp"
    break;

  case 82: /* insert_stmt: INSERT INTO ID LBRACE attr_list RBRACE VALUES values_list  */
#line 755 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_INSERT);
      (yyval.sql_node)->insertion.relation_name = (yyvsp[-5].string);
      (yyval.sql_node)->insertion.attr_names = std::move(*(yyvsp[-3].index_attr_list));
      if ((yyvsp[0].values_list) != nullptr) {
        (yyval.sql_node)->insertion.values_list.swap(*(yyvsp[0].values_list));
        delete (yyvsp[0].values_list);
      }
      free((yyvsp[-5].string));
    }
#line 2621 "yacc_sql.cpp"
    break;

  case 83: /* values_list: LBRACE value_list RBRACE  */
#line 769 "yacc_sql.y"
    {
      (yyval.values_list) = new std::vector<std::vector<Value>>;
      (yyval.values_list)->emplace_back(*(yyvsp[-1].value_list));
      delete (yyvsp[-1].value_list);
    }
#line 2631 "yacc_sql.cpp"
    break;

  case 84: /* values_list: values_list COMMA LBRACE value_list RBRACE  */
#line 775 "yacc_sql.y"
    {
      (yyval.values_list)->emplace_back(*(yyvsp[-1].value_list));
      delete (yyvsp[-1].value_list);
    }
#line 2640 "yacc_sql.cpp"
    break;

  case 85: /* digits: NUMBER  */
#line 782 "yacc_sql.y"
    {
      (yyval.digits) = float((yyvsp[0].number));
    }
#line 2648 "yacc_sql.cpp"
    break;

  case 86: /* digits: '-' NUMBER  */
#line 786 "yacc_sql.y"
    {
      (yyval.digits) = float(-(yyvsp[0].number));
    }
#line 2656 "yacc_sql.cpp"
    break;

  case 87: /* digits: FLOAT  */
#line 790 "yacc_sql.y"
    {
      (yyval.digits) = (yyvsp[0].floats);
    }
#line 2664 "yacc_sql.cpp"
    break;

  case 88: /* digits: '-' FLOAT  */
#line 794 "yacc_sql.y"
    {
      (yyval.digits) = (yyvsp[0].floats);
    }
#line 2672 "yacc_sql.cpp"
    break;

  case 89: /* digits_list: %empty  */
#line 801 "yacc_sql.y"
    {
      (yyval.digits_list) = new std::vector<float>();
    }
#line 2680 "yacc_sql.cpp"
    break;

  case 90: /* digits_list: digits  */
#line 805 "yacc_sql.y"
    {
      (yyval.digits_list) = new std::vector<float>();
      (yyval.digits_list)->push_back((yyvsp[0].digits));
    }
#line 2689 "yacc_sql.cpp"
    break;

  case 91: /* digits_list: digits_list COMMA digits  */
#line 810 "yacc_sql.y"
    {
      (yyval.digits_list)->push_back((yyvsp[0].digits));
    }
#line 2697 "yacc_sql.cpp"
    break;

  case 92: /* value_list: %empty  */
#line 817 "yacc_sql.y"
    {
      (yyval.value_list) = new std::vector<Value>;
    }
#line 2705 "yacc_sql.cpp"
    break;

  case 93: /* value_list: value  */
#line 821 "yacc_sql.y"
    {
      (yyval.value_list) = new std::vector<Value>;
      (yyval.value_list)->emplace_back(*(yyvsp[0].value));
      delete (yyvsp[0].value);
    }
#line 2715 "yacc_sql.cpp"
    break;

  case 94: /* value_list: value_list COMMA value  */
#line 827 "yacc_sql.y"
    {
      (yyval.value_list)->emplace_back(*(yyvsp[0].value));
      delete (yyvsp[0].value);
    }
#line 2724 "yacc_sql.cpp"
    break;

  case 95: /* value: nonnegative_value  */
#line 834 "yacc_sql.y"
                      {
      (yyval.value) = (yyvsp[0].value);
    }
#line 2732 "yacc_sql.cpp"
    break;

  case 96: /* value: '-' NUMBER  */
#line 837 "yacc_sql.y"
                 {
      (yyval.value) = new Value(-(yyvsp[0].number));
      (yyloc) = (yylsp[-1]);
    }
#line 2741 "yacc_sql.cpp"
    break;

  case 97: /* value: '-' FLOAT  */
#line 841 "yacc_sql.y"
                {
      (yyval.value) = new Value(-(yyvsp[0].floats));
      (yyloc) = (yylsp[-1]);
    }
#line 2750 "yacc_sql.cpp"
    break;

  case 98: /* nonnegative_value: NUMBER  */
#line 848 "yacc_sql.y"
           {
      (yyval.value) = new Value((yyvsp[0].number));
      (yyloc) = (yylsp[0]);
    }
#line 2759 "yacc_sql.cpp"
    break;

  case 99: /* nonnegative_value: FLOAT  */
#line 852 "yacc_sql.y"
            {
      (yyval.value) = new Value((yyvsp[0].floats));
      (yyloc) = (yylsp[0]);
    }
#line 2768 "yacc_sql.cpp"
    break;

  case 100: /* nonnegative_value: SSS  */
#line 856 "yacc_sql.y"
          {
      char *tmp = common::substr((yyvsp[0].string),1,strlen((yyvsp[0].string))-2);
      (yyval.value) = new Value(tmp);
      free(tmp);
      free((yyvsp[0].string));
    }
#line 2779 "yacc_sql.cpp"
    break;

  case 101: /* nonnegative_value: TRUE  */
#line 862 "yacc_sql.y"
           {
      (yyval.value) = new Value(true);
    }
#line 2787 "yacc_sql.cpp"
    break;

  case 102: /* nonnegative_value: FALSE  */
#line 865 "yacc_sql.y"
            {
      (yyval.value) = new Value(false);
    }
#line 2795 "yacc_sql.cpp"
    break;

  case 103: /* nonnegative_value: NULL_T  */
#line 868 "yacc_sql.y"
             {
      (yyval.value) = new Value(NullValue());
    }
#line 2803 "yacc_sql.cpp"
    break;

  case 104: /* nonnegative_value: LSBRACE digits_list RSBRACE  */
#line 871 "yacc_sql.y"
                                  {
      (yyval.value) = new Value(*(yyvsp[-1].digits_list));
    }
#line 2811 "yacc_sql.cpp"
    break;

  case 105: /* nonnegative_value: STRING_TO_VECTOR LBRACE value_list RBRACE  */
#line 874 "yacc_sql.y"
                                                {
      Value *val = nullptr;
      if ((yyvsp[-1].value_list) == nullptr || (yyvsp[-1].value_list)->size() != 1) {
        LOG_WARN("string_to_vector expects exactly one argument");
        delete (yyvsp[-1].value_list);
        YYERROR;
      }
      Value tmp;
      RC    rc = Value::cast_to((*(yyvsp[-1].value_list))[0], AttrType::VECTORS, tmp);
      delete (yyvsp[-1].value_list);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to cast argument to vector. rc=%d", static_cast<int>(rc));
        YYERROR;
      }
      val = new Value(tmp);
      (yyval.value)  = val;
    }
#line 2833 "yacc_sql.cpp"
    break;

  case 106: /* nonnegative_value: VECTOR_TO_STRING LBRACE value_list RBRACE  */
#line 891 "yacc_sql.y"
                                                {
      Value *val = nullptr;
      if ((yyvsp[-1].value_list) == nullptr || (yyvsp[-1].value_list)->size() != 1) {
        LOG_WARN("vector_to_string expects exactly one argument");
        delete (yyvsp[-1].value_list);
        YYERROR;
      }
      Value tmp;
      RC    rc = Value::cast_to((*(yyvsp[-1].value_list))[0], AttrType::CHARS, tmp);
      delete (yyvsp[-1].value_list);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to cast argument to string. rc=%d", static_cast<int>(rc));
        YYERROR;
      }
      val = new Value(tmp);
      (yyval.value) = val;
    }
#line 2855 "yacc_sql.cpp"
    break;

  case 107: /* storage_format: %empty  */
#line 912 "yacc_sql.y"
    {
      (yyval.string) = nullptr;
    }
#line 2863 "yacc_sql.cpp"
    break;

  case 108: /* storage_format: STORAGE FORMAT EQ ID  */
#line 916 "yacc_sql.y"
    {
      (yyval.string) = (yyvsp[0].string);
    }
#line 2871 "yacc_sql.cpp"
    break;

  case 109: /* delete_stmt: DELETE FROM ID where  */
#line 923 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DELETE);
      (yyval.sql_node)->deletion.relation_name = (yyvsp[-1].string);
      if ((yyvsp[0].expression) != nullptr) {
        (yyval.sql_node)->deletion.condition = std::unique_ptr<Expression>((yyvsp[0].expression));
      }
      free((yyvsp[-1].string));
    }
#line 2884 "yacc_sql.cpp"
    break;

  case 110: /* update_stmt: UPDATE ID SET set_clauses where  */
#line 935 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_UPDATE);
      (yyval.sql_node)->update.relation_name = (yyvsp[-3].string);
      (yyval.sql_node)->update.set_clauses.swap(*(yyvsp[-1].set_clauses));
      if ((yyvsp[0].expression) != nullptr) {
        (yyval.sql_node)->update.conditions = std::unique_ptr<Expression>((yyvsp[0].expression));
      }
      free((yyvsp[-3].string));
      delete (yyvsp[-1].set_clauses);
    }
#line 2899 "yacc_sql.cpp"
    break;

  case 111: /* set_clauses: set_clause  */
#line 949 "yacc_sql.y"
    {
      (yyval.set_clauses) = new std::vector<SetClauseSqlNode>;
      (yyval.set_clauses)->emplace_back(std::move(*(yyvsp[0].set_clause)));
    }
#line 2908 "yacc_sql.cpp"
    break;

  case 112: /* set_clauses: set_clauses COMMA set_clause  */
#line 954 "yacc_sql.y"
    {
      (yyval.set_clauses)->emplace_back(std::move(*(yyvsp[0].set_clause)));
    }
#line 2916 "yacc_sql.cpp"
    break;

  case 113: /* set_clause: ID EQ expression  */
#line 961 "yacc_sql.y"
    {
      (yyval.set_clause) = new SetClauseSqlNode;
      (yyval.set_clause)->field_name = (yyvsp[-2].string);
      (yyval.set_clause)->value = std::unique_ptr<Expression>((yyvsp[0].expression));
      free((yyvsp[-2].string));
    }
#line 2927 "yacc_sql.cpp"
    break;

  case 114: /* select_stmt: select_core select_union_list  */
#line 971 "yacc_sql.y"
    {
      (yyval.sql_node) = (yyvsp[-1].sql_node);
      if ((yyvsp[0].set_operator_list) != nullptr) {
        (yyval.sql_node)->selection.set_operations.swap(*(yyvsp[0].set_operator_list));
        delete (yyvsp[0].set_operator_list);
      }
    }
#line 2939 "yacc_sql.cpp"
    break;

  case 115: /* select_union_list: %empty  */
#line 982 "yacc_sql.y"
    {
      (yyval.set_operator_list) = nullptr;
    }
#line 2947 "yacc_sql.cpp"
    break;

  case 116: /* select_union_list: select_union_list select_union_item  */
#line 986 "yacc_sql.y"
    {
      if ((yyvsp[-1].set_operator_list) != nullptr) {
        (yyval.set_operator_list) = (yyvsp[-1].set_operator_list);
      } else {
        (yyval.set_operator_list) = new std::vector<SetOperatorSqlNode>();
      }
      (yyval.set_operator_list)->emplace_back(std::move(*(yyvsp[0].set_operator_node)));
      delete (yyvsp[0].set_operator_node);
    }
#line 2961 "yacc_sql.cpp"
    break;

  case 117: /* select_union_item: UNION select_core  */
#line 999 "yacc_sql.y"
    {
      (yyval.set_operator_node) = new SetOperatorSqlNode;
      (yyval.set_operator_node)->union_all = false;
      (yyval.set_operator_node)->select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      delete (yyvsp[0].sql_node);
    }
#line 2972 "yacc_sql.cpp"
    break;

  case 118: /* select_union_item: UNION ALL select_core  */
#line 1006 "yacc_sql.y"
    {
      (yyval.set_operator_node) = new SetOperatorSqlNode;
      (yyval.set_operator_node)->union_all = true;
      (yyval.set_operator_node)->select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      delete (yyvsp[0].sql_node);
    }
#line 2983 "yacc_sql.cpp"
    break;

  case 119: /* select_core: SELECT expression_list FROM rel_list where group_by opt_having opt_order_by opt_limit  */
#line 1016 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SELECT);
      if ((yyvsp[-7].expression_list) != nullptr) {
        (yyval.sql_node)->selection.expressions.swap(*(yyvsp[-7].expression_list));
        delete (yyvsp[-7].expression_list);
      }

      if ((yyvsp[-5].relation_list) != nullptr) {
        (yyval.sql_node)->selection.relations.swap(*(yyvsp[-5].relation_list));
        delete (yyvsp[-5].relation_list);
      }

      (yyval.sql_node)->selection.conditions = nullptr;

      if ((yyvsp[-4].expression) != nullptr) {
        (yyval.sql_node)->selection.conditions = std::unique_ptr<Expression>((yyvsp[-4].expression));
      }

      if ((yyvsp[-3].expression_list) != nullptr) {
        (yyval.sql_node)->selection.group_by.swap(*(yyvsp[-3].expression_list));
        delete (yyvsp[-3].expression_list);
      }

      if ((yyvsp[-2].expression) != nullptr) {
        (yyval.sql_node)->selection.having_conditions = std::unique_ptr<Expression>((yyvsp[-2].expression));
      }

      if ((yyvsp[-1].orderby_list) != nullptr) {
        (yyval.sql_node)->selection.order_by.swap(*(yyvsp[-1].orderby_list));
        delete (yyvsp[-1].orderby_list);
      }

      if ((yyvsp[0].limited_info) != nullptr) {
        (yyval.sql_node)->selection.limit = std::make_unique<LimitSqlNode>(*(yyvsp[0].limited_info));
        delete (yyvsp[0].limited_info);
      }
    }
#line 3025 "yacc_sql.cpp"
    break;

  case 120: /* select_core: SELECT expression_list FROM relation INNER JOIN join_clauses where group_by  */
#line 1054 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SELECT);
      if ((yyvsp[-7].expression_list) != nullptr) {
        (yyval.sql_node)->selection.expressions.swap(*(yyvsp[-7].expression_list));
        delete (yyvsp[-7].expression_list);
      }

      if ((yyvsp[-5].string) != nullptr) {
        (yyval.sql_node)->selection.relations.emplace_back((yyvsp[-5].string));
        free((yyvsp[-5].string));
      }

      if ((yyvsp[-2].join_clauses) != nullptr) {
        for (auto it = (yyvsp[-2].join_clauses)->relations.rbegin(); it != (yyvsp[-2].join_clauses)->relations.rend(); ++it) {
          (yyval.sql_node)->selection.relations.emplace_back(std::move(*it));
        }
        (yyval.sql_node)->selection.conditions = std::move((yyvsp[-2].join_clauses)->conditions);
      }

      if ((yyvsp[-1].expression) != nullptr) {
        auto ptr = (yyval.sql_node)->selection.conditions.release();
        (yyval.sql_node)->selection.conditions = std::make_unique<ConjunctionExpr>(ConjunctionExpr::Type::AND, ptr, (yyvsp[-1].expression));
      }

      if ((yyvsp[0].expression_list) != nullptr) {
        (yyval.sql_node)->selection.group_by.swap(*(yyvsp[0].expression_list));
        delete (yyvsp[0].expression_list);
      }
    }
#line 3059 "yacc_sql.cpp"
    break;

  case 121: /* calc_stmt: CALC expression_list  */
#line 1087 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CALC);
      (yyval.sql_node)->calc.expressions.swap(*(yyvsp[0].expression_list));
      delete (yyvsp[0].expression_list);
    }
#line 3069 "yacc_sql.cpp"
    break;

  case 122: /* calc_stmt: SELECT expression_list  */
#line 1093 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CALC);
      (yyval.sql_node)->calc.expressions.swap(*(yyvsp[0].expression_list));
      delete (yyvsp[0].expression_list);
    }
#line 3079 "yacc_sql.cpp"
    break;

  case 123: /* expression_list: %empty  */
#line 1101 "yacc_sql.y"
                {
      (yyval.expression_list) = new std::vector<std::unique_ptr<Expression>>;
    }
#line 3087 "yacc_sql.cpp"
    break;

  case 124: /* expression_list: expression alias  */
#line 1105 "yacc_sql.y"
    {
      (yyval.expression_list) = new std::vector<std::unique_ptr<Expression>>;
      if (nullptr != (yyvsp[0].string)) {
        (yyvsp[-1].expression)->set_alias((yyvsp[0].string));
      }
      (yyval.expression_list)->emplace_back((yyvsp[-1].expression));
      free((yyvsp[0].string));
    }
#line 3100 "yacc_sql.cpp"
    break;

  case 125: /* expression_list: expression alias COMMA expression_list  */
#line 1114 "yacc_sql.y"
    {
      if ((yyvsp[0].expression_list) != nullptr) {
        (yyval.expression_list) = (yyvsp[0].expression_list);
      } else {
        (yyval.expression_list) = new std::vector<std::unique_ptr<Expression>>;
      }
      if (nullptr != (yyvsp[-2].string)) {
        (yyvsp[-3].expression)->set_alias((yyvsp[-2].string));
      }
      (yyval.expression_list)->emplace((yyval.expression_list)->begin(),std::move((yyvsp[-3].expression)));
      free((yyvsp[-2].string));
    }
#line 3117 "yacc_sql.cpp"
    break;

  case 126: /* expression: expression '+' expression  */
#line 1129 "yacc_sql.y"
                              {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::ADD, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 3125 "yacc_sql.cpp"
    break;

  case 127: /* expression: expression '-' expression  */
#line 1132 "yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::SUB, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 3133 "yacc_sql.cpp"
    break;

  case 128: /* expression: expression '*' expression  */
#line 1135 "yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::MUL, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 3141 "yacc_sql.cpp"
    break;

  case 129: /* expression: expression '/' expression  */
#line 1138 "yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::DIV, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 3149 "yacc_sql.cpp"
    break;

  case 130: /* expression: LBRACE expression_list RBRACE  */
#line 1141 "yacc_sql.y"
                                    {
      if ((yyvsp[-1].expression_list)->size() == 1) {
        (yyval.expression) = (yyvsp[-1].expression_list)->front().get();
      } else {
        (yyval.expression) = new ListExpr(std::move(*(yyvsp[-1].expression_list)));
      }
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3162 "yacc_sql.cpp"
    break;

  case 131: /* expression: '-' expression  */
#line 1149 "yacc_sql.y"
                                  {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::NEGATIVE, (yyvsp[0].expression), nullptr, sql_string, &(yyloc));
    }
#line 3170 "yacc_sql.cpp"
    break;

  case 132: /* expression: nonnegative_value  */
#line 1152 "yacc_sql.y"
                        {
      (yyval.expression) = new ValueExpr(*(yyvsp[0].value));
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
      delete (yyvsp[0].value);
    }
#line 3180 "yacc_sql.cpp"
    break;

  case 133: /* expression: rel_attr  */
#line 1157 "yacc_sql.y"
               {
      RelAttrSqlNode *node = (yyvsp[0].rel_attr);
      (yyval.expression) = new UnboundFieldExpr(node->relation_name, node->attribute_name);
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
      delete (yyvsp[0].rel_attr);
    }
#line 3191 "yacc_sql.cpp"
    break;

  case 134: /* expression: '*'  */
#line 1163 "yacc_sql.y"
          {
      (yyval.expression) = new StarExpr();
    }
#line 3199 "yacc_sql.cpp"
    break;

  case 135: /* expression: ID DOT '*'  */
#line 1166 "yacc_sql.y"
                 {
      (yyval.expression) = new StarExpr((yyvsp[-2].string));
    }
#line 3207 "yacc_sql.cpp"
    break;

  case 136: /* expression: func_expr  */
#line 1169 "yacc_sql.y"
                {
      (yyval.expression) = (yyvsp[0].expression);      // AggrFuncExpr
    }
#line 3215 "yacc_sql.cpp"
    break;

  case 137: /* expression: sub_query_expr  */
#line 1172 "yacc_sql.y"
                     {
      (yyval.expression) = (yyvsp[0].expression); // SubQueryExpr
    }
#line 3223 "yacc_sql.cpp"
    break;

  case 138: /* alias: %empty  */
#line 1179 "yacc_sql.y"
                {
      (yyval.string) = nullptr;
    }
#line 3231 "yacc_sql.cpp"
    break;

  case 139: /* alias: ID  */
#line 1182 "yacc_sql.y"
         {
      (yyval.string) = (yyvsp[0].string);
    }
#line 3239 "yacc_sql.cpp"
    break;

  case 140: /* alias: AS ID  */
#line 1185 "yacc_sql.y"
            {
      (yyval.string) = (yyvsp[0].string);
    }
#line 3247 "yacc_sql.cpp"
    break;

  case 141: /* func_expr: ID LBRACE expression_list RBRACE  */
#line 1191 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr((yyvsp[-3].string), std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3256 "yacc_sql.cpp"
    break;

  case 142: /* func_expr: DISTANCE LBRACE expression_list RBRACE  */
#line 1196 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr("distance", std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3265 "yacc_sql.cpp"
    break;

  case 143: /* func_expr: STRING_TO_VECTOR LBRACE expression_list RBRACE  */
#line 1201 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr("string_to_vector", std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3274 "yacc_sql.cpp"
    break;

  case 144: /* func_expr: VECTOR_TO_STRING LBRACE expression_list RBRACE  */
#line 1206 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr("vector_to_string", std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3283 "yacc_sql.cpp"
    break;

  case 145: /* func_expr: TOKENIZE LBRACE expression_list RBRACE  */
#line 1211 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr("tokenize", std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3292 "yacc_sql.cpp"
    break;

  case 146: /* func_expr: MATCH LBRACE expression_list RBRACE AGAINST LBRACE expression RBRACE  */
#line 1216 "yacc_sql.y"
    {
        if ((yyvsp[-5].expression_list) == nullptr || (yyvsp[-5].expression_list)->size() != 1) {
            LOG_WARN("MATCH expects exactly one field expression");
            if ((yyvsp[-5].expression_list)) delete (yyvsp[-5].expression_list);
            if ((yyvsp[-1].expression)) delete (yyvsp[-1].expression);
            YYERROR;
        }
        std::unique_ptr<Expression> field_expr = std::move((yyvsp[-5].expression_list)->front());
        std::unique_ptr<Expression> search_expr((yyvsp[-1].expression));
        (yyvsp[-5].expression_list)->clear();  // 清空vector，避免双重删除
        delete (yyvsp[-5].expression_list);
        (yyval.expression) = new MatchAgainstExpr(std::move(field_expr), std::move(search_expr));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3311 "yacc_sql.cpp"
    break;

  case 147: /* sub_query_expr: LBRACE select_stmt RBRACE  */
#line 1234 "yacc_sql.y"
    {
      (yyval.expression) = new SubQueryExpr((yyvsp[-1].sql_node)->selection);
    }
#line 3319 "yacc_sql.cpp"
    break;

  case 148: /* rel_attr: ID  */
#line 1240 "yacc_sql.y"
       {
      (yyval.rel_attr) = new RelAttrSqlNode;
      (yyval.rel_attr)->attribute_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 3329 "yacc_sql.cpp"
    break;

  case 149: /* rel_attr: ID DOT ID  */
#line 1245 "yacc_sql.y"
                {
      (yyval.rel_attr) = new RelAttrSqlNode;
      (yyval.rel_attr)->relation_name  = (yyvsp[-2].string);
      (yyval.rel_attr)->attribute_name = (yyvsp[0].string);
      free((yyvsp[-2].string));
      free((yyvsp[0].string));
    }
#line 3341 "yacc_sql.cpp"
    break;

  case 150: /* relation: ID  */
#line 1255 "yacc_sql.y"
       {
      (yyval.string) = (yyvsp[0].string);
    }
#line 3349 "yacc_sql.cpp"
    break;

  case 151: /* rel_list: relation alias  */
#line 1261 "yacc_sql.y"
                   {
      (yyval.relation_list) = new std::vector<RelationNode>();
      if(nullptr!=(yyvsp[0].string)){
        (yyval.relation_list)->emplace_back((yyvsp[-1].string),(yyvsp[0].string));
        free((yyvsp[0].string));
      }else{
        (yyval.relation_list)->emplace_back((yyvsp[-1].string));
      }
      free((yyvsp[-1].string));
    }
#line 3364 "yacc_sql.cpp"
    break;

  case 152: /* rel_list: relation alias COMMA rel_list  */
#line 1271 "yacc_sql.y"
                                    {
      if ((yyvsp[0].relation_list) != nullptr) {
        (yyval.relation_list) = (yyvsp[0].relation_list);
      } else {
        (yyval.relation_list) = new std::vector<RelationNode>;
      }
      if(nullptr!=(yyvsp[-2].string)){
        (yyval.relation_list)->insert((yyval.relation_list)->begin(), RelationNode((yyvsp[-3].string),(yyvsp[-2].string)));
        free((yyvsp[-2].string));
      }else{
        (yyval.relation_list)->insert((yyval.relation_list)->begin(), RelationNode((yyvsp[-3].string)));
      }
      free((yyvsp[-3].string));
    }
#line 3383 "yacc_sql.cpp"
    break;

  case 153: /* join_clauses: relation ON condition  */
#line 1289 "yacc_sql.y"
    {
      (yyval.join_clauses) = new JoinSqlNode;
      (yyval.join_clauses)->relations.emplace_back((yyvsp[-2].string));
      (yyval.join_clauses)->conditions = std::unique_ptr<Expression>((yyvsp[0].expression));
      free((yyvsp[-2].string));
    }
#line 3394 "yacc_sql.cpp"
    break;

  case 154: /* join_clauses: relation ON condition INNER JOIN join_clauses  */
#line 1296 "yacc_sql.y"
    {
      (yyval.join_clauses) = (yyvsp[0].join_clauses);
      (yyval.join_clauses)->relations.emplace_back((yyvsp[-5].string));
      auto ptr = (yyval.join_clauses)->conditions.release();
      (yyval.join_clauses)->conditions = std::make_unique<ConjunctionExpr>(ConjunctionExpr::Type::AND, ptr, (yyvsp[-3].expression));
      free((yyvsp[-5].string));
    }
#line 3406 "yacc_sql.cpp"
    break;

  case 155: /* where: %empty  */
#line 1307 "yacc_sql.y"
    {
      (yyval.expression) = nullptr;
    }
#line 3414 "yacc_sql.cpp"
    break;

  case 156: /* where: WHERE condition  */
#line 1310 "yacc_sql.y"
                      {
      (yyval.expression) = (yyvsp[0].expression);  
    }
#line 3422 "yacc_sql.cpp"
    break;

  case 157: /* condition: expression comp_op expression  */
#line 1317 "yacc_sql.y"
    {
      (yyval.expression) = new ComparisonExpr((yyvsp[-1].comp), (yyvsp[-2].expression), (yyvsp[0].expression));
    }
#line 3430 "yacc_sql.cpp"
    break;

  case 158: /* condition: comp_op expression  */
#line 1321 "yacc_sql.y"
    {
      Value val;
      val.set_null(true);
      ValueExpr *temp_expr = new ValueExpr(val);

      // 新增
      if (((yyvsp[-1].comp) == EXISTS_OP || (yyvsp[-1].comp) == NOT_EXISTS_OP) && (yyvsp[0].expression)->type() == ExprType::SUBQUERY) {
        static_cast<SubQueryExpr *>((yyvsp[0].expression))->set_allow_multi_column(true);
      }

      (yyval.expression) = new ComparisonExpr((yyvsp[-1].comp),temp_expr, (yyvsp[0].expression));
    }
#line 3447 "yacc_sql.cpp"
    break;

  case 159: /* condition: condition AND condition  */
#line 1334 "yacc_sql.y"
    {
      (yyval.expression) = new ConjunctionExpr(ConjunctionExpr::Type::AND, (yyvsp[-2].expression), (yyvsp[0].expression));
    }
#line 3455 "yacc_sql.cpp"
    break;

  case 160: /* condition: condition OR condition  */
#line 1338 "yacc_sql.y"
    {
      (yyval.expression) = new ConjunctionExpr(ConjunctionExpr::Type::OR, (yyvsp[-2].expression), (yyvsp[0].expression));
    }
#line 3463 "yacc_sql.cpp"
    break;

  case 161: /* comp_op: EQ  */
#line 1344 "yacc_sql.y"
         { (yyval.comp) = EQUAL_TO; }
#line 3469 "yacc_sql.cpp"
    break;

  case 162: /* comp_op: LT  */
#line 1345 "yacc_sql.y"
         { (yyval.comp) = LESS_THAN; }
#line 3475 "yacc_sql.cpp"
    break;

  case 163: /* comp_op: GT  */
#line 1346 "yacc_sql.y"
         { (yyval.comp) = GREAT_THAN; }
#line 3481 "yacc_sql.cpp"
    break;

  case 164: /* comp_op: LE  */
#line 1347 "yacc_sql.y"
         { (yyval.comp) = LESS_EQUAL; }
#line 3487 "yacc_sql.cpp"
    break;

  case 165: /* comp_op: GE  */
#line 1348 "yacc_sql.y"
         { (yyval.comp) = GREAT_EQUAL; }
#line 3493 "yacc_sql.cpp"
    break;

  case 166: /* comp_op: NE  */
#line 1349 "yacc_sql.y"
         { (yyval.comp) = NOT_EQUAL; }
#line 3499 "yacc_sql.cpp"
    break;

  case 167: /* comp_op: IS  */
#line 1350 "yacc_sql.y"
         { (yyval.comp) = IS_OP; }
#line 3505 "yacc_sql.cpp"
    break;

  case 168: /* comp_op: IS NOT  */
#line 1351 "yacc_sql.y"
             { (yyval.comp) = IS_NOT_OP; }
#line 3511 "yacc_sql.cpp"
    break;

  case 169: /* comp_op: LIKE  */
#line 1352 "yacc_sql.y"
           { (yyval.comp) = LIKE_OP;}
#line 3517 "yacc_sql.cpp"
    break;

  case 170: /* comp_op: NOT LIKE  */
#line 1353 "yacc_sql.y"
               {(yyval.comp) = NOT_LIKE_OP;}
#line 3523 "yacc_sql.cpp"
    break;

  case 171: /* comp_op: IN  */
#line 1354 "yacc_sql.y"
         { (yyval.comp) = IN_OP; }
#line 3529 "yacc_sql.cpp"
    break;

  case 172: /* comp_op: NOT IN  */
#line 1355 "yacc_sql.y"
             { (yyval.comp) = NOT_IN_OP; }
#line 3535 "yacc_sql.cpp"
    break;

  case 173: /* comp_op: EXISTS  */
#line 1356 "yacc_sql.y"
             { (yyval.comp) = EXISTS_OP; }
#line 3541 "yacc_sql.cpp"
    break;

  case 174: /* comp_op: NOT EXISTS  */
#line 1357 "yacc_sql.y"
                 { (yyval.comp) = NOT_EXISTS_OP; }
#line 3547 "yacc_sql.cpp"
    break;

  case 175: /* opt_order_by: %empty  */
#line 1362 "yacc_sql.y"
    {
      (yyval.orderby_list) = nullptr;
    }
#line 3555 "yacc_sql.cpp"
    break;

  case 176: /* opt_order_by: ORDER BY sort_list  */
#line 1366 "yacc_sql.y"
    {
      (yyval.orderby_list) = (yyvsp[0].orderby_list);
      std::reverse((yyval.orderby_list)->begin(),(yyval.orderby_list)->end());
    }
#line 3564 "yacc_sql.cpp"
    break;

  case 177: /* sort_list: sort_unit  */
#line 1374 "yacc_sql.y"
        {
      (yyval.orderby_list) = new std::vector<OrderBySqlNode>;
      (yyval.orderby_list)->emplace_back(std::move(*(yyvsp[0].orderby_unit)));
	}
#line 3573 "yacc_sql.cpp"
    break;

  case 178: /* sort_list: sort_unit COMMA sort_list  */
#line 1379 "yacc_sql.y"
        {
      (yyvsp[0].orderby_list)->emplace_back(std::move(*(yyvsp[-2].orderby_unit)));
      (yyval.orderby_list) = (yyvsp[0].orderby_list);
	}
#line 3582 "yacc_sql.cpp"
    break;

  case 179: /* sort_unit: expression  */
#line 1387 "yacc_sql.y"
        {
      (yyval.orderby_unit) = new OrderBySqlNode();
      (yyval.orderby_unit)->expr = std::unique_ptr<Expression>((yyvsp[0].expression));
      (yyval.orderby_unit)->is_asc = true;
	}
#line 3592 "yacc_sql.cpp"
    break;

  case 180: /* sort_unit: expression DESC  */
#line 1393 "yacc_sql.y"
        {
      (yyval.orderby_unit) = new OrderBySqlNode();
      (yyval.orderby_unit)->expr = std::unique_ptr<Expression>((yyvsp[-1].expression));
      (yyval.orderby_unit)->is_asc = false;
	}
#line 3602 "yacc_sql.cpp"
    break;

  case 181: /* sort_unit: expression ASC  */
#line 1399 "yacc_sql.y"
        {
      (yyval.orderby_unit) = new OrderBySqlNode(); // 默认是升序
      (yyval.orderby_unit)->expr = std::unique_ptr<Expression>((yyvsp[-1].expression));
      (yyval.orderby_unit)->is_asc = true;
	}
#line 3612 "yacc_sql.cpp"
    break;

  case 182: /* group_by: %empty  */
#line 1408 "yacc_sql.y"
    {
      (yyval.expression_list) = nullptr;
    }
#line 3620 "yacc_sql.cpp"
    break;

  case 183: /* group_by: GROUP BY expression_list  */
#line 1412 "yacc_sql.y"
    {
      (yyval.expression_list) = (yyvsp[0].expression_list);
    }
#line 3628 "yacc_sql.cpp"
    break;

  case 184: /* opt_having: %empty  */
#line 1419 "yacc_sql.y"
    {
      (yyval.expression) = nullptr;
    }
#line 3636 "yacc_sql.cpp"
    break;

  case 185: /* opt_having: HAVING condition  */
#line 1423 "yacc_sql.y"
    {
      (yyval.expression) = (yyvsp[0].expression);
    }
#line 3644 "yacc_sql.cpp"
    break;

  case 186: /* opt_limit: %empty  */
#line 1430 "yacc_sql.y"
    {
      (yyval.limited_info) = nullptr;
    }
#line 3652 "yacc_sql.cpp"
    break;

  case 187: /* opt_limit: LIMIT NUMBER  */
#line 1434 "yacc_sql.y"
    {
      (yyval.limited_info) = new LimitSqlNode();
      (yyval.limited_info)->number = (yyvsp[0].number);
    }
#line 3661 "yacc_sql.cpp"
    break;

  case 188: /* explain_stmt: EXPLAIN command_wrapper  */
#line 1442 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_EXPLAIN);
      (yyval.sql_node)->explain.sql_node = std::unique_ptr<ParsedSqlNode>((yyvsp[0].sql_node));
    }
#line 3670 "yacc_sql.cpp"
    break;

  case 189: /* set_variable_stmt: SET ID EQ value  */
#line 1450 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SET_VARIABLE);
      (yyval.sql_node)->set_variable.name  = (yyvsp[-2].string);
      (yyval.sql_node)->set_variable.value = *(yyvsp[0].value);
      free((yyvsp[-2].string));
      delete (yyvsp[0].value);
    }
#line 3682 "yacc_sql.cpp"
    break;


#line 3686 "yacc_sql.cpp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken, &yylloc};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (&yylloc, sql_string, sql_result, scanner, yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, sql_string, sql_result, scanner);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp, sql_string, sql_result, scanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, sql_string, sql_result, scanner, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, sql_string, sql_result, scanner);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp, sql_string, sql_result, scanner);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 1462 "yacc_sql.y"

//_____________________________________________________________________
extern void scan_string(const char *str, yyscan_t scanner);

int sql_parse(const char *s, ParsedSqlResult *sql_result) {
  yyscan_t scanner;
  yylex_init(&scanner);
  scan_string(s, scanner);
  int result = yyparse(s, sql_result, scanner);
  yylex_destroy(scanner);
  return result;
}
