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
  YYSYMBOL_DISTANCE = 73,                  /* DISTANCE  */
  YYSYMBOL_TYPE = 74,                      /* TYPE  */
  YYSYMBOL_CHANGE = 75,                    /* CHANGE  */
  YYSYMBOL_LISTS = 76,                     /* LISTS  */
  YYSYMBOL_PROBES = 77,                    /* PROBES  */
  YYSYMBOL_IVFFLAT = 78,                   /* IVFFLAT  */
  YYSYMBOL_EQ = 79,                        /* EQ  */
  YYSYMBOL_LT = 80,                        /* LT  */
  YYSYMBOL_GT = 81,                        /* GT  */
  YYSYMBOL_LE = 82,                        /* LE  */
  YYSYMBOL_GE = 83,                        /* GE  */
  YYSYMBOL_NE = 84,                        /* NE  */
  YYSYMBOL_LIKE = 85,                      /* LIKE  */
  YYSYMBOL_IS = 86,                        /* IS  */
  YYSYMBOL_RENAME = 87,                    /* RENAME  */
  YYSYMBOL_TO = 88,                        /* TO  */
  YYSYMBOL_NUMBER = 89,                    /* NUMBER  */
  YYSYMBOL_FLOAT = 90,                     /* FLOAT  */
  YYSYMBOL_ID = 91,                        /* ID  */
  YYSYMBOL_SSS = 92,                       /* SSS  */
  YYSYMBOL_93_ = 93,                       /* '+'  */
  YYSYMBOL_94_ = 94,                       /* '-'  */
  YYSYMBOL_95_ = 95,                       /* '*'  */
  YYSYMBOL_96_ = 96,                       /* '/'  */
  YYSYMBOL_UMINUS = 97,                    /* UMINUS  */
  YYSYMBOL_YYACCEPT = 98,                  /* $accept  */
  YYSYMBOL_commands = 99,                  /* commands  */
  YYSYMBOL_command_wrapper = 100,          /* command_wrapper  */
  YYSYMBOL_exit_stmt = 101,                /* exit_stmt  */
  YYSYMBOL_help_stmt = 102,                /* help_stmt  */
  YYSYMBOL_sync_stmt = 103,                /* sync_stmt  */
  YYSYMBOL_begin_stmt = 104,               /* begin_stmt  */
  YYSYMBOL_commit_stmt = 105,              /* commit_stmt  */
  YYSYMBOL_rollback_stmt = 106,            /* rollback_stmt  */
  YYSYMBOL_drop_table_stmt = 107,          /* drop_table_stmt  */
  YYSYMBOL_alter_table_stmt = 108,         /* alter_table_stmt  */
  YYSYMBOL_change_column_type = 109,       /* change_column_type  */
  YYSYMBOL_change_column_type_body = 110,  /* change_column_type_body  */
  YYSYMBOL_change_column_nullable = 111,   /* change_column_nullable  */
  YYSYMBOL_show_tables_stmt = 112,         /* show_tables_stmt  */
  YYSYMBOL_desc_table_stmt = 113,          /* desc_table_stmt  */
  YYSYMBOL_show_index_stmt = 114,          /* show_index_stmt  */
  YYSYMBOL_create_index_stmt = 115,        /* create_index_stmt  */
  YYSYMBOL_opt_unique = 116,               /* opt_unique  */
  YYSYMBOL_index_type = 117,               /* index_type  */
  YYSYMBOL_vector_index_config = 118,      /* vector_index_config  */
  YYSYMBOL_attr_list = 119,                /* attr_list  */
  YYSYMBOL_drop_index_stmt = 120,          /* drop_index_stmt  */
  YYSYMBOL_create_table_stmt = 121,        /* create_table_stmt  */
  YYSYMBOL_create_view_stmt = 122,         /* create_view_stmt  */
  YYSYMBOL_drop_view_stmt = 123,           /* drop_view_stmt  */
  YYSYMBOL_attr_def_list = 124,            /* attr_def_list  */
  YYSYMBOL_attr_def = 125,                 /* attr_def  */
  YYSYMBOL_nullable_constraint = 126,      /* nullable_constraint  */
  YYSYMBOL_type = 127,                     /* type  */
  YYSYMBOL_insert_stmt = 128,              /* insert_stmt  */
  YYSYMBOL_values_list = 129,              /* values_list  */
  YYSYMBOL_digits = 130,                   /* digits  */
  YYSYMBOL_digits_list = 131,              /* digits_list  */
  YYSYMBOL_value_list = 132,               /* value_list  */
  YYSYMBOL_value = 133,                    /* value  */
  YYSYMBOL_nonnegative_value = 134,        /* nonnegative_value  */
  YYSYMBOL_storage_format = 135,           /* storage_format  */
  YYSYMBOL_delete_stmt = 136,              /* delete_stmt  */
  YYSYMBOL_update_stmt = 137,              /* update_stmt  */
  YYSYMBOL_set_clauses = 138,              /* set_clauses  */
  YYSYMBOL_set_clause = 139,               /* set_clause  */
  YYSYMBOL_select_stmt = 140,              /* select_stmt  */
  YYSYMBOL_select_union_list = 141,        /* select_union_list  */
  YYSYMBOL_select_union_item = 142,        /* select_union_item  */
  YYSYMBOL_select_core = 143,              /* select_core  */
  YYSYMBOL_calc_stmt = 144,                /* calc_stmt  */
  YYSYMBOL_expression_list = 145,          /* expression_list  */
  YYSYMBOL_expression = 146,               /* expression  */
  YYSYMBOL_alias = 147,                    /* alias  */
  YYSYMBOL_func_expr = 148,                /* func_expr  */
  YYSYMBOL_sub_query_expr = 149,           /* sub_query_expr  */
  YYSYMBOL_rel_attr = 150,                 /* rel_attr  */
  YYSYMBOL_relation = 151,                 /* relation  */
  YYSYMBOL_rel_list = 152,                 /* rel_list  */
  YYSYMBOL_join_clauses = 153,             /* join_clauses  */
  YYSYMBOL_where = 154,                    /* where  */
  YYSYMBOL_condition = 155,                /* condition  */
  YYSYMBOL_comp_op = 156,                  /* comp_op  */
  YYSYMBOL_opt_order_by = 157,             /* opt_order_by  */
  YYSYMBOL_sort_list = 158,                /* sort_list  */
  YYSYMBOL_sort_unit = 159,                /* sort_unit  */
  YYSYMBOL_group_by = 160,                 /* group_by  */
  YYSYMBOL_opt_having = 161,               /* opt_having  */
  YYSYMBOL_opt_limit = 162,                /* opt_limit  */
  YYSYMBOL_explain_stmt = 163,             /* explain_stmt  */
  YYSYMBOL_set_variable_stmt = 164,        /* set_variable_stmt  */
  YYSYMBOL_opt_semicolon = 165             /* opt_semicolon  */
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
#define YYFINAL  83
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   468

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  98
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  68
/* YYNRULES -- Number of rules.  */
#define YYNRULES  187
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  380

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   348


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
       2,     2,    95,    93,     2,    94,     2,    96,     2,     2,
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
      85,    86,    87,    88,    89,    90,    91,    92,    97
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   301,   301,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,   319,   320,   321,   322,   323,   324,   325,
     326,   327,   328,   329,   330,   331,   335,   341,   346,   352,
     358,   364,   370,   377,   389,   399,   412,   425,   431,   437,
     442,   447,   453,   458,   464,   470,   478,   488,   500,   516,
     517,   521,   528,   535,   544,   556,   562,   571,   581,   585,
     589,   593,   597,   604,   612,   624,   634,   637,   650,   668,
     697,   701,   705,   710,   716,   717,   718,   719,   720,   721,
     725,   735,   749,   755,   762,   766,   770,   774,   782,   785,
     790,   798,   801,   807,   815,   818,   822,   829,   833,   837,
     843,   846,   849,   852,   855,   872,   893,   896,   903,   915,
     929,   934,   941,   951,   963,   966,   979,   986,   996,  1034,
    1067,  1073,  1082,  1085,  1094,  1110,  1113,  1116,  1119,  1122,
    1130,  1133,  1138,  1144,  1147,  1150,  1153,  1160,  1163,  1166,
    1171,  1176,  1181,  1186,  1194,  1201,  1206,  1216,  1222,  1232,
    1249,  1256,  1268,  1271,  1277,  1281,  1294,  1298,  1305,  1306,
    1307,  1308,  1309,  1310,  1311,  1312,  1313,  1314,  1315,  1316,
    1317,  1318,  1323,  1326,  1334,  1339,  1347,  1353,  1359,  1369,
    1372,  1380,  1383,  1391,  1394,  1402,  1410,  1421
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
  "VECTOR_TO_STRING", "DISTANCE", "TYPE", "CHANGE", "LISTS", "PROBES",
  "IVFFLAT", "EQ", "LT", "GT", "LE", "GE", "NE", "LIKE", "IS", "RENAME",
  "TO", "NUMBER", "FLOAT", "ID", "SSS", "'+'", "'-'", "'*'", "'/'",
  "UMINUS", "$accept", "commands", "command_wrapper", "exit_stmt",
  "help_stmt", "sync_stmt", "begin_stmt", "commit_stmt", "rollback_stmt",
  "drop_table_stmt", "alter_table_stmt", "change_column_type",
  "change_column_type_body", "change_column_nullable", "show_tables_stmt",
  "desc_table_stmt", "show_index_stmt", "create_index_stmt", "opt_unique",
  "index_type", "vector_index_config", "attr_list", "drop_index_stmt",
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

#define YYPACT_NINF (-233)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-97)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     406,    -2,    10,    14,   260,   260,   -30,    33,  -233,    12,
      40,    29,  -233,  -233,  -233,  -233,  -233,    31,   406,   133,
     100,  -233,  -233,  -233,  -233,  -233,  -233,  -233,  -233,  -233,
    -233,  -233,  -233,  -233,  -233,  -233,  -233,  -233,  -233,  -233,
    -233,  -233,  -233,  -233,  -233,    51,    88,  -233,    54,   135,
      65,    92,    98,   125,   140,   -54,  -233,  -233,  -233,   164,
     179,   190,  -233,  -233,    -8,  -233,   260,  -233,  -233,  -233,
      17,  -233,  -233,  -233,   163,  -233,  -233,   165,   128,   130,
     109,   143,  -233,  -233,  -233,  -233,   158,     3,   132,    30,
     137,  -233,   173,  -233,     7,   260,   196,   208,  -233,  -233,
     -51,  -233,    28,   301,   301,   260,   260,   -31,  -233,   146,
    -233,   260,   260,   260,   260,   207,   148,   148,    -9,   184,
     150,    47,     9,  -233,   152,   182,    42,   187,   228,   160,
     192,   162,   236,   237,   238,   174,   163,  -233,  -233,  -233,
    -233,  -233,   -54,   329,    50,  -233,    60,   239,    61,   243,
     244,   247,  -233,  -233,  -233,   -26,   -26,  -233,  -233,   260,
    -233,     6,   184,  -233,   160,   250,   232,  -233,   185,     0,
    -233,   252,   253,   112,  -233,  -233,   228,  -233,   131,   254,
     204,   228,  -233,   193,  -233,   255,   262,   197,  -233,   200,
     152,   201,   202,  -233,   103,   115,  -233,    47,  -233,  -233,
    -233,  -233,  -233,  -233,   229,   263,   285,   271,    47,   269,
    -233,  -233,     1,  -233,  -233,  -233,  -233,  -233,  -233,  -233,
     257,   114,   118,   260,   260,   150,  -233,    47,    47,  -233,
    -233,  -233,  -233,  -233,  -233,  -233,  -233,  -233,   105,   152,
     274,   216,  -233,   281,   160,   305,   283,  -233,  -233,   234,
    -233,  -233,   148,   148,   313,   308,   275,   134,   307,  -233,
    -233,  -233,  -233,   260,   232,   232,    86,    86,  -233,   246,
     290,  -233,  -233,  -233,   254,   277,  -233,   160,  -233,   228,
     160,   131,   282,   184,    13,  -233,   260,   232,   324,   250,
    -233,    47,    86,  -233,   284,   315,  -233,  -233,    43,   316,
    -233,   317,  -233,   113,   232,   285,  -233,   118,   340,   306,
     269,   156,    77,   228,  -233,   278,  -233,   268,  -233,  -233,
    -233,    90,  -233,   260,   270,  -233,  -233,  -233,  -233,   333,
     334,   295,    21,  -233,   332,  -233,   141,  -233,    77,   148,
    -233,  -233,   260,   286,   289,  -233,  -233,  -233,   273,   291,
     339,  -233,   345,   297,   304,   299,   300,   291,   292,   157,
     349,  -233,   309,   310,   302,   303,    47,    47,   353,   356,
     311,   312,   318,   319,    47,    47,   366,   371,  -233,  -233
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    50,     0,     0,   122,   122,     0,     0,    28,     0,
       0,     0,    29,    30,    31,    27,    26,     0,     0,     0,
       0,    25,    24,    18,    19,    20,    21,     9,    10,    11,
      12,    15,    13,    14,     8,    16,    17,     5,     7,     6,
       3,   114,     4,    22,    23,     0,     0,    49,     0,     0,
       0,     0,     0,     0,   122,    88,   100,   101,   102,     0,
       0,     0,    97,    98,   145,    99,     0,   133,   131,   120,
     137,   135,   136,   132,   121,    45,    44,     0,     0,     0,
       0,     0,   185,     1,   187,     2,   113,   106,     0,     0,
       0,    32,     0,    65,     0,   122,     0,     0,    84,    86,
       0,    89,     0,    91,    91,   122,   122,     0,   130,     0,
     138,     0,     0,     0,     0,   123,     0,     0,     0,   152,
       0,     0,     0,   115,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   144,   129,    85,
      87,   103,     0,     0,     0,    92,   131,     0,     0,     0,
       0,     0,   146,   134,   139,   125,   126,   127,   128,   122,
     147,   137,   152,    46,     0,     0,     0,   108,     0,   152,
     110,     0,     0,     0,   186,    94,     0,   116,     0,    66,
       0,     0,    62,     0,    63,    55,     0,     0,    57,     0,
       0,     0,     0,    90,    97,    98,   104,     0,   142,   105,
     143,   141,   140,   124,     0,   148,   179,     0,    91,    80,
     170,   168,     0,   158,   159,   160,   161,   162,   163,   166,
     164,     0,   153,     0,     0,     0,   109,    91,    91,    95,
      96,   117,    74,    75,    76,    77,    78,    79,    73,     0,
       0,     0,    61,     0,     0,     0,     0,    34,    33,     0,
      36,    93,     0,     0,     0,   181,     0,     0,     0,   171,
     169,   167,   165,     0,     0,     0,   155,   112,   111,     0,
       0,    72,    71,    69,    66,   106,   107,     0,    56,     0,
       0,    38,     0,   152,   137,   149,   122,     0,   172,     0,
      82,    91,   154,   156,   157,     0,    70,    67,    60,     0,
      64,     0,    35,    41,     0,   179,   180,   182,     0,   183,
      81,     0,    73,     0,    59,     0,    47,     0,    37,    40,
      42,   150,   119,     0,     0,   118,    83,    68,    58,     0,
       0,     0,   176,   173,   174,   184,     0,    48,    43,     0,
     178,   177,     0,     0,     0,    39,   151,   175,     0,     0,
       0,    51,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    52,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    53,    54
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -233,  -233,   385,  -233,  -233,  -233,  -233,  -233,  -233,  -233,
    -233,  -233,  -233,    66,  -233,  -233,  -233,  -233,  -233,    48,
    -233,  -142,  -233,  -233,  -233,  -233,   136,  -165,  -225,   126,
    -233,   117,   266,  -233,   -99,  -117,  -101,   142,  -233,  -233,
    -233,   186,   -53,  -233,  -233,  -108,  -233,    -5,   -60,   342,
    -233,  -233,  -233,  -109,   169,    70,  -153,  -232,   195,  -233,
      91,  -233,   129,  -233,  -233,  -233,  -233,  -233
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,   302,   318,   319,    29,    30,    31,    32,    49,   352,
     337,   186,    33,    34,    35,    36,   240,   179,   320,   238,
      37,   209,   101,   102,   144,   145,    68,   126,    38,    39,
     169,   170,    40,    86,   123,    41,    42,    69,    70,   205,
      71,    72,    73,   282,   162,   283,   167,   222,   223,   309,
     333,   334,   255,   288,   325,    43,    44,    85
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      74,    96,   146,   146,   174,   148,   108,   161,   163,   206,
     109,   259,    45,   273,   177,   132,   226,   109,   164,   106,
     175,   109,   207,   133,    50,   248,   340,    51,    53,    95,
     124,   225,   293,   294,   128,    98,    99,   260,   139,   140,
     100,    46,   341,    47,   107,   165,   181,   313,    76,    97,
      77,   155,   156,   157,   158,   307,   166,   129,   141,   142,
     152,    75,    95,    95,   153,    78,   125,    48,   231,   113,
     114,   204,   321,   182,   274,   184,    55,   176,   196,    52,
     251,   197,   134,   108,    56,    57,   261,   327,   -94,   199,
     136,   -94,   197,    58,   135,    79,   175,   110,   147,   149,
     150,   151,   278,    84,   110,    88,   221,   175,   110,   257,
     111,   112,   113,   114,   111,   112,   113,   114,   171,   172,
      80,   270,    81,   271,   210,   272,   175,   175,   242,   148,
     305,   -95,   269,    83,   -95,   299,    62,    63,   301,    65,
     317,   173,    87,   -96,   284,    89,   -96,   264,   265,   270,
     211,   271,    90,   272,   203,   331,    91,   270,   212,   271,
      95,   272,   290,   266,   267,   197,   232,    54,   120,    55,
     233,   234,   235,   236,   237,   264,   265,    56,    57,   111,
     112,   113,   114,    92,   326,   361,    58,   197,   362,    93,
     175,   103,   311,   213,   214,   215,   216,   217,   218,   219,
     220,   229,   230,   292,   221,   221,   104,   111,   112,   113,
     114,    59,    60,    61,   343,   344,    94,   105,   116,   118,
     117,   119,   121,   127,   137,   122,   300,   221,   130,    62,
      63,    64,    65,   131,    66,    67,   138,   154,   159,   160,
     166,   168,   210,   178,   221,   314,   180,   183,    95,   368,
     369,   185,   187,   188,   189,   190,   191,   376,   377,    54,
     328,    55,   192,   332,   224,   175,   175,   198,   211,    56,
      57,   200,   201,   175,   175,   202,   212,   208,    58,   227,
     228,   306,   332,   241,   243,   239,   244,    54,   246,    55,
     245,   247,   249,   250,   253,   252,   254,    56,    57,   256,
     258,   262,   275,    59,    60,    61,    58,   276,   277,   279,
     280,   213,   214,   215,   216,   217,   218,   219,   220,   286,
     287,    62,    63,    64,    65,   281,    66,    67,    54,   289,
      55,    59,    60,    61,   291,   295,   296,   308,    56,    57,
     125,   264,   304,   312,   315,   316,   323,    58,   329,    62,
      63,    64,    65,   324,    66,    67,    54,   330,    55,   335,
     336,   339,   338,   342,   350,   348,    56,    57,   349,   351,
     353,   355,    59,    60,    61,    58,   354,   356,   357,   358,
     363,   366,   367,   360,   370,   364,   365,   371,   372,   373,
      62,    63,    64,    65,   378,   143,    67,   374,   375,   379,
      59,    60,    61,    82,   345,   359,   310,   303,   193,   346,
     297,   268,   115,     1,     2,     3,   263,   298,   194,   195,
      64,    65,   285,    66,    67,     4,     5,     6,     7,     8,
       9,    10,    11,   347,   322,     0,     0,     0,    12,    13,
      14,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    15,     0,    16,     0,     0,
       0,     0,     0,     0,     0,    17,     0,     0,    18
};

static const yytype_int16 yycheck[] =
{
       5,    54,   103,   104,   121,   104,    66,   116,   117,   162,
       4,    10,    14,   238,   122,     8,   169,     4,    27,    27,
     121,     4,   164,    16,    14,   190,     5,    17,    14,    20,
      27,    31,   264,   265,     4,    89,    90,    36,    89,    90,
      94,    43,    21,    45,    52,    54,     4,     4,    15,    54,
      17,   111,   112,   113,   114,   287,    56,    27,    30,    31,
      91,    91,    20,    20,    95,    53,    63,    69,   176,    95,
      96,    65,   304,   126,   239,   128,    29,    68,    28,    69,
     197,    31,    75,   143,    37,    38,    85,   312,    28,    28,
      95,    31,    31,    46,    87,    55,   197,    91,   103,   104,
     105,   106,   244,     3,    91,    17,   166,   208,    91,   208,
      93,    94,    95,    96,    93,    94,    95,    96,    71,    72,
      91,    44,    91,    46,    10,    48,   227,   228,   181,   228,
     283,    28,    27,     0,    31,   277,    89,    90,   280,    92,
      27,    94,    91,    28,   253,    91,    31,    57,    58,    44,
      36,    46,    17,    48,   159,    65,    91,    44,    44,    46,
      20,    48,    28,   223,   224,    31,    35,    27,    59,    29,
      39,    40,    41,    42,    43,    57,    58,    37,    38,    93,
      94,    95,    96,    91,    28,    28,    46,    31,    31,    91,
     291,    27,   291,    79,    80,    81,    82,    83,    84,    85,
      86,    89,    90,   263,   264,   265,    27,    93,    94,    95,
      96,    71,    72,    73,    73,    74,    91,    27,    55,    91,
      55,    91,    79,    91,    28,    67,   279,   287,    91,    89,
      90,    91,    92,    60,    94,    95,    28,    91,    31,    91,
      56,    91,    10,    91,   304,   298,    64,    60,    20,   366,
     367,    91,    60,    91,    18,    18,    18,   374,   375,    27,
     313,    29,    88,   323,    79,   366,   367,    28,    36,    37,
      38,    28,    28,   374,   375,    28,    44,    27,    46,    27,
      27,   286,   342,    79,    91,    31,    31,    27,    91,    29,
      28,    91,    91,    91,    31,    66,    11,    37,    38,    28,
      31,    44,    28,    71,    72,    73,    46,    91,    27,     4,
      27,    79,    80,    81,    82,    83,    84,    85,    86,     6,
      12,    89,    90,    91,    92,    91,    94,    95,    27,    54,
      29,    71,    72,    73,    27,    89,    46,    13,    37,    38,
      63,    57,    60,    28,    28,    28,     6,    46,    70,    89,
      90,    91,    92,    47,    94,    95,    27,    89,    29,    89,
      27,    66,    28,    31,    91,    79,    37,    38,    79,    78,
      31,    74,    71,    72,    73,    46,    31,    73,    79,    79,
      31,    79,    79,    91,    31,    76,    76,    31,    77,    77,
      89,    90,    91,    92,    28,    94,    95,    79,    79,    28,
      71,    72,    73,    18,   338,   357,   289,   281,   142,   339,
     274,   225,    70,     7,     8,     9,   221,   275,    89,    90,
      91,    92,   253,    94,    95,    19,    20,    21,    22,    23,
      24,    25,    26,   342,   305,    -1,    -1,    -1,    32,    33,
      34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    -1,    51,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    59,    -1,    -1,    62
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     7,     8,     9,    19,    20,    21,    22,    23,    24,
      25,    26,    32,    33,    34,    49,    51,    59,    62,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   112,
     113,   114,   115,   120,   121,   122,   123,   128,   136,   137,
     140,   143,   144,   163,   164,    14,    43,    45,    69,   116,
      14,    17,    69,    14,    27,    29,    37,    38,    46,    71,
      72,    73,    89,    90,    91,    92,    94,    95,   134,   145,
     146,   148,   149,   150,   145,    91,    15,    17,    53,    55,
      91,    91,   100,     0,     3,   165,   141,    91,    17,    91,
      17,    91,    91,    91,    91,    20,   140,   145,    89,    90,
      94,   130,   131,    27,    27,    27,    27,    52,   146,     4,
      91,    93,    94,    95,    96,   147,    55,    55,    91,    91,
      59,    79,    67,   142,    27,    63,   135,    91,     4,    27,
      91,    60,     8,    16,    75,    87,   145,    28,    28,    89,
      90,    30,    31,    94,   132,   133,   134,   145,   132,   145,
     145,   145,    91,    95,    91,   146,   146,   146,   146,    31,
      91,   151,   152,   151,    27,    54,    56,   154,    91,   138,
     139,    71,    72,    94,   133,   134,    68,   143,    91,   125,
      64,     4,   140,    60,   140,    91,   119,    60,    91,    18,
      18,    18,    88,   130,    89,    90,    28,    31,    28,    28,
      28,    28,    28,   145,    65,   147,   154,   119,    27,   129,
      10,    36,    44,    79,    80,    81,    82,    83,    84,    85,
      86,   146,   155,   156,    79,    31,   154,    27,    27,    89,
      90,   143,    35,    39,    40,    41,    42,    43,   127,    31,
     124,    79,   140,    91,    31,    28,    91,    91,   125,    91,
      91,   133,    66,    31,    11,   160,    28,   132,    31,    10,
      36,    85,    44,   156,    57,    58,   146,   146,   139,    27,
      44,    46,    48,   126,   125,    28,    91,    27,   119,     4,
      27,    91,   151,   153,   151,   152,     6,    12,   161,    54,
      28,    27,   146,   155,   155,    89,    46,   124,   135,   119,
     140,   119,   109,   127,    60,   154,   145,   155,    13,   157,
     129,   132,    28,     4,   140,    28,    28,    27,   110,   111,
     126,   155,   160,     6,    47,   162,    28,   126,   140,    70,
      89,    65,   146,   158,   159,    89,    27,   118,    28,    66,
       5,    21,    31,    73,    74,   111,   153,   158,    79,    79,
      91,    78,   117,    31,    31,    74,    73,    79,    79,   117,
      91,    28,    31,    31,    76,    76,    79,    79,   133,   133,
      31,    31,    77,    77,    79,    79,   133,   133,    28,    28
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    98,    99,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   108,   108,   108,   109,   109,   110,
     110,   110,   111,   111,   112,   113,   114,   115,   115,   116,
     116,   117,   118,   118,   118,   119,   119,   120,   121,   121,
     121,   121,   121,   122,   122,   123,   124,   124,   125,   125,
     126,   126,   126,   126,   127,   127,   127,   127,   127,   127,
     128,   128,   129,   129,   130,   130,   130,   130,   131,   131,
     131,   132,   132,   132,   133,   133,   133,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   135,   135,   136,   137,
     138,   138,   139,   140,   141,   141,   142,   142,   143,   143,
     144,   144,   145,   145,   145,   146,   146,   146,   146,   146,
     146,   146,   146,   146,   146,   146,   146,   147,   147,   147,
     148,   148,   148,   148,   149,   150,   150,   151,   152,   152,
     153,   153,   154,   154,   155,   155,   155,   155,   156,   156,
     156,   156,   156,   156,   156,   156,   156,   156,   156,   156,
     156,   156,   157,   157,   158,   158,   159,   159,   159,   160,
     160,   161,   161,   162,   162,   163,   164,   165
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     6,     6,     8,     6,     2,     0,     4,
       1,     0,     1,     0,     2,     2,     4,     9,    11,     1,
       0,     1,     9,    17,    17,     1,     3,     5,    10,     9,
       8,     6,     5,     5,     8,     3,     0,     3,     6,     3,
       2,     1,     1,     0,     1,     1,     1,     1,     1,     1,
       5,     8,     3,     5,     1,     2,     1,     2,     0,     1,
       3,     0,     1,     3,     1,     2,     2,     1,     1,     1,
       1,     1,     1,     3,     4,     4,     0,     4,     4,     5,
       1,     3,     3,     2,     0,     2,     2,     3,     9,     9,
       2,     2,     0,     2,     4,     3,     3,     3,     3,     3,
       2,     1,     1,     1,     3,     1,     1,     0,     1,     2,
       4,     4,     4,     4,     3,     1,     3,     1,     2,     4,
       3,     6,     0,     2,     3,     2,     3,     3,     1,     1,
       1,     1,     1,     1,     1,     2,     1,     2,     1,     2,
       1,     2,     0,     3,     1,     3,     1,     2,     2,     0,
       3,     0,     2,     0,     2,     2,     4,     1
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
#line 302 "yacc_sql.y"
  {
    std::unique_ptr<ParsedSqlNode> sql_node = std::unique_ptr<ParsedSqlNode>((yyvsp[-1].sql_node));
    sql_result->add_sql_node(std::move(sql_node));
  }
#line 2003 "yacc_sql.cpp"
    break;

  case 26: /* exit_stmt: EXIT  */
#line 335 "yacc_sql.y"
         {
      (void)yynerrs;  // 这么写为了消除yynerrs未使用的告警。如果你有更好的方法欢迎提PR
      (yyval.sql_node) = new ParsedSqlNode(SCF_EXIT);
    }
#line 2012 "yacc_sql.cpp"
    break;

  case 27: /* help_stmt: HELP  */
#line 341 "yacc_sql.y"
         {
      (yyval.sql_node) = new ParsedSqlNode(SCF_HELP);
    }
#line 2020 "yacc_sql.cpp"
    break;

  case 28: /* sync_stmt: SYNC  */
#line 346 "yacc_sql.y"
         {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SYNC);
    }
#line 2028 "yacc_sql.cpp"
    break;

  case 29: /* begin_stmt: TRX_BEGIN  */
#line 352 "yacc_sql.y"
               {
      (yyval.sql_node) = new ParsedSqlNode(SCF_BEGIN);
    }
#line 2036 "yacc_sql.cpp"
    break;

  case 30: /* commit_stmt: TRX_COMMIT  */
#line 358 "yacc_sql.y"
               {
      (yyval.sql_node) = new ParsedSqlNode(SCF_COMMIT);
    }
#line 2044 "yacc_sql.cpp"
    break;

  case 31: /* rollback_stmt: TRX_ROLLBACK  */
#line 364 "yacc_sql.y"
                  {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ROLLBACK);
    }
#line 2052 "yacc_sql.cpp"
    break;

  case 32: /* drop_table_stmt: DROP TABLE ID  */
#line 370 "yacc_sql.y"
                  {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_TABLE);
      (yyval.sql_node)->drop_table.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2062 "yacc_sql.cpp"
    break;

  case 33: /* alter_table_stmt: ALTER TABLE ID ADD COLUMN attr_def  */
#line 378 "yacc_sql.y"
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
#line 2078 "yacc_sql.cpp"
    break;

  case 34: /* alter_table_stmt: ALTER TABLE ID DROP COLUMN ID  */
#line 390 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ALTER_TABLE);
      AlterTableSqlNode &alter_table = (yyval.sql_node)->alter_table;
      alter_table.table_name         = (yyvsp[-3].string);
      alter_table.alter_type         = AlterType::DROP_COLUMN;
      alter_table.column_name        = (yyvsp[0].string);
      free((yyvsp[-3].string));
      free((yyvsp[0].string));
    }
#line 2092 "yacc_sql.cpp"
    break;

  case 35: /* alter_table_stmt: ALTER TABLE ID CHANGE COLUMN ID ID change_column_type  */
#line 400 "yacc_sql.y"
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
#line 2109 "yacc_sql.cpp"
    break;

  case 36: /* alter_table_stmt: ALTER TABLE ID RENAME TO ID  */
#line 413 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ALTER_TABLE);
      AlterTableSqlNode &alter_table = (yyval.sql_node)->alter_table;
      alter_table.table_name         = (yyvsp[-3].string);
      alter_table.alter_type         = AlterType::RENAME_TABLE;
      alter_table.new_table_name     = (yyvsp[0].string);
      free((yyvsp[-3].string));
      free((yyvsp[0].string));
    }
#line 2123 "yacc_sql.cpp"
    break;

  case 37: /* change_column_type: type change_column_type_body  */
#line 426 "yacc_sql.y"
    {
      (void)(yyvsp[-1].number);
      (yyval.nullable_info) = (yyvsp[0].nullable_info);
    }
#line 2132 "yacc_sql.cpp"
    break;

  case 38: /* change_column_type: %empty  */
#line 431 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;
    }
#line 2140 "yacc_sql.cpp"
    break;

  case 39: /* change_column_type_body: LBRACE NUMBER RBRACE change_column_nullable  */
#line 438 "yacc_sql.y"
    {
      (void)(yyvsp[-2].number);
      (yyval.nullable_info) = (yyvsp[0].nullable_info);
    }
#line 2149 "yacc_sql.cpp"
    break;

  case 40: /* change_column_type_body: change_column_nullable  */
#line 443 "yacc_sql.y"
    {
      (yyval.nullable_info) = (yyvsp[0].nullable_info);
    }
#line 2157 "yacc_sql.cpp"
    break;

  case 41: /* change_column_type_body: %empty  */
#line 447 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;
    }
#line 2165 "yacc_sql.cpp"
    break;

  case 42: /* change_column_nullable: nullable_constraint  */
#line 454 "yacc_sql.y"
    {
      (yyval.nullable_info) = (yyvsp[0].nullable_info);
    }
#line 2173 "yacc_sql.cpp"
    break;

  case 43: /* change_column_nullable: %empty  */
#line 458 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;
    }
#line 2181 "yacc_sql.cpp"
    break;

  case 44: /* show_tables_stmt: SHOW TABLES  */
#line 464 "yacc_sql.y"
                {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SHOW_TABLES);
    }
#line 2189 "yacc_sql.cpp"
    break;

  case 45: /* desc_table_stmt: DESC ID  */
#line 470 "yacc_sql.y"
             {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DESC_TABLE);
      (yyval.sql_node)->desc_table.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2199 "yacc_sql.cpp"
    break;

  case 46: /* show_index_stmt: SHOW INDEX FROM relation  */
#line 479 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SHOW_INDEX);
      ShowIndexSqlNode &show_index = (yyval.sql_node)->show_index;
      show_index.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2210 "yacc_sql.cpp"
    break;

  case 47: /* create_index_stmt: CREATE opt_unique INDEX ID ON ID LBRACE attr_list RBRACE  */
#line 489 "yacc_sql.y"
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
#line 2226 "yacc_sql.cpp"
    break;

  case 48: /* create_index_stmt: CREATE VECTOR_T INDEX ID ON ID LBRACE attr_list RBRACE WITH vector_index_config  */
#line 501 "yacc_sql.y"
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
#line 2243 "yacc_sql.cpp"
    break;

  case 49: /* opt_unique: UNIQUE  */
#line 516 "yacc_sql.y"
           { (yyval.unique) = true; }
#line 2249 "yacc_sql.cpp"
    break;

  case 50: /* opt_unique: %empty  */
#line 517 "yacc_sql.y"
                { (yyval.unique) = false; }
#line 2255 "yacc_sql.cpp"
    break;

  case 51: /* index_type: IVFFLAT  */
#line 522 "yacc_sql.y"
    {
      (yyval.index_type) = IndexType::VectorIVFFlatIndex;
    }
#line 2263 "yacc_sql.cpp"
    break;

  case 52: /* vector_index_config: LBRACE DISTANCE EQ ID COMMA TYPE EQ index_type RBRACE  */
#line 529 "yacc_sql.y"
    {
      (yyval.vector_index_config) = new VectorIndexConfig;
      (yyval.vector_index_config)->distance_fn = (yyvsp[-5].string);
      (yyval.vector_index_config)->index_type = (yyvsp[-1].index_type);
      free((yyvsp[-5].string));
    }
#line 2274 "yacc_sql.cpp"
    break;

  case 53: /* vector_index_config: LBRACE DISTANCE EQ ID COMMA TYPE EQ index_type COMMA LISTS EQ value COMMA PROBES EQ value RBRACE  */
#line 536 "yacc_sql.y"
    {
      (yyval.vector_index_config) = new VectorIndexConfig;
      (yyval.vector_index_config)->distance_fn = (yyvsp[-13].string);
      (yyval.vector_index_config)->index_type = (yyvsp[-9].index_type);
      (yyval.vector_index_config)->lists = std::move(*(yyvsp[-5].value));
      (yyval.vector_index_config)->probes = std::move(*(yyvsp[-1].value));
      free((yyvsp[-13].string));
    }
#line 2287 "yacc_sql.cpp"
    break;

  case 54: /* vector_index_config: LBRACE TYPE EQ index_type COMMA DISTANCE EQ ID COMMA LISTS EQ value COMMA PROBES EQ value RBRACE  */
#line 545 "yacc_sql.y"
    {
      (yyval.vector_index_config) = new VectorIndexConfig;
      (yyval.vector_index_config)->distance_fn = (yyvsp[-9].string);
      (yyval.vector_index_config)->index_type = (yyvsp[-13].index_type);
      (yyval.vector_index_config)->lists = std::move(*(yyvsp[-5].value));
      (yyval.vector_index_config)->probes = std::move(*(yyvsp[-1].value));
      free((yyvsp[-9].string));
    }
#line 2300 "yacc_sql.cpp"
    break;

  case 55: /* attr_list: ID  */
#line 557 "yacc_sql.y"
    {
      (yyval.index_attr_list) = new std::vector<std::string>; // 创建一个新的 vector
      (yyval.index_attr_list)->emplace_back((yyvsp[0].string)); // 将列名加入 vector
      free((yyvsp[0].string));
    }
#line 2310 "yacc_sql.cpp"
    break;

  case 56: /* attr_list: ID COMMA attr_list  */
#line 563 "yacc_sql.y"
    {
      (yyval.index_attr_list) = (yyvsp[0].index_attr_list); // 使用现有的 vector
      (yyval.index_attr_list)->emplace((yyval.index_attr_list)->begin(), (yyvsp[-2].string)); // 将新列名加入 vector 开头
      free((yyvsp[-2].string));
    }
#line 2320 "yacc_sql.cpp"
    break;

  case 57: /* drop_index_stmt: DROP INDEX ID ON ID  */
#line 572 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_INDEX);
      (yyval.sql_node)->drop_index.index_name = (yyvsp[-2].string);
      (yyval.sql_node)->drop_index.relation_name = (yyvsp[0].string);
      free((yyvsp[-2].string));
      free((yyvsp[0].string));
    }
#line 2332 "yacc_sql.cpp"
    break;

  case 58: /* create_table_stmt: CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE storage_format AS select_stmt  */
#line 582 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-7].string), (yyvsp[-5].attr_info), (yyvsp[-4].attr_infos), (yyvsp[-2].string), (yyvsp[0].sql_node));
    }
#line 2340 "yacc_sql.cpp"
    break;

  case 59: /* create_table_stmt: CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE storage_format select_stmt  */
#line 586 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-6].string), (yyvsp[-4].attr_info), (yyvsp[-3].attr_infos), (yyvsp[-1].string), (yyvsp[0].sql_node));
    }
#line 2348 "yacc_sql.cpp"
    break;

  case 60: /* create_table_stmt: CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE storage_format  */
#line 590 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-5].string), (yyvsp[-3].attr_info), (yyvsp[-2].attr_infos), (yyvsp[0].string), nullptr);
    }
#line 2356 "yacc_sql.cpp"
    break;

  case 61: /* create_table_stmt: CREATE TABLE ID storage_format AS select_stmt  */
#line 594 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-3].string), nullptr, nullptr, (yyvsp[-2].string), (yyvsp[0].sql_node));
    }
#line 2364 "yacc_sql.cpp"
    break;

  case 62: /* create_table_stmt: CREATE TABLE ID storage_format select_stmt  */
#line 598 "yacc_sql.y"
    {
      (yyval.sql_node) = create_table_sql_node((yyvsp[-2].string), nullptr, nullptr, (yyvsp[-1].string), (yyvsp[0].sql_node));
    }
#line 2372 "yacc_sql.cpp"
    break;

  case 63: /* create_view_stmt: CREATE VIEW ID AS select_stmt  */
#line 605 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CREATE_VIEW);
      CreateViewSqlNode &create_view = (yyval.sql_node)->create_view;
      create_view.relation_name = (yyvsp[-2].string);
      create_view.create_view_select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      free((yyvsp[-2].string));
    }
#line 2384 "yacc_sql.cpp"
    break;

  case 64: /* create_view_stmt: CREATE VIEW ID LBRACE attr_list RBRACE AS select_stmt  */
#line 613 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CREATE_VIEW);
      CreateViewSqlNode &create_view = (yyval.sql_node)->create_view;
      create_view.relation_name = (yyvsp[-5].string);
      create_view.attribute_names = std::move(*(yyvsp[-3].index_attr_list));
      create_view.create_view_select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      free((yyvsp[-5].string));
    }
#line 2397 "yacc_sql.cpp"
    break;

  case 65: /* drop_view_stmt: DROP VIEW ID  */
#line 625 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_VIEW);
      (yyval.sql_node)->drop_view.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2407 "yacc_sql.cpp"
    break;

  case 66: /* attr_def_list: %empty  */
#line 634 "yacc_sql.y"
    {
      (yyval.attr_infos) = nullptr;
    }
#line 2415 "yacc_sql.cpp"
    break;

  case 67: /* attr_def_list: COMMA attr_def attr_def_list  */
#line 638 "yacc_sql.y"
    {
      if ((yyvsp[0].attr_infos) != nullptr) {
        (yyval.attr_infos) = (yyvsp[0].attr_infos);
      } else {
        (yyval.attr_infos) = new std::vector<AttrInfoSqlNode>;
      }
      (yyval.attr_infos)->emplace_back(*(yyvsp[-1].attr_info));
      delete (yyvsp[-1].attr_info);
    }
#line 2429 "yacc_sql.cpp"
    break;

  case 68: /* attr_def: ID type LBRACE NUMBER RBRACE nullable_constraint  */
#line 651 "yacc_sql.y"
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
#line 2451 "yacc_sql.cpp"
    break;

  case 69: /* attr_def: ID type nullable_constraint  */
#line 669 "yacc_sql.y"
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
#line 2481 "yacc_sql.cpp"
    break;

  case 70: /* nullable_constraint: NOT NULL_T  */
#line 698 "yacc_sql.y"
    {
      (yyval.nullable_info) = false;  // NOT NULL 对应的可空性为 false
    }
#line 2489 "yacc_sql.cpp"
    break;

  case 71: /* nullable_constraint: NULLABLE  */
#line 702 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;  // NULLABLE 对应的可空性为 true 2022
    }
#line 2497 "yacc_sql.cpp"
    break;

  case 72: /* nullable_constraint: NULL_T  */
#line 706 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;  // NULL 对应的可空性也为 true 2023
    }
#line 2505 "yacc_sql.cpp"
    break;

  case 73: /* nullable_constraint: %empty  */
#line 710 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;  // 默认情况为 NULL
    }
#line 2513 "yacc_sql.cpp"
    break;

  case 74: /* type: INT_T  */
#line 716 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::INTS);   }
#line 2519 "yacc_sql.cpp"
    break;

  case 75: /* type: STRING_T  */
#line 717 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::CHARS);  }
#line 2525 "yacc_sql.cpp"
    break;

  case 76: /* type: FLOAT_T  */
#line 718 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::FLOATS); }
#line 2531 "yacc_sql.cpp"
    break;

  case 77: /* type: DATE_T  */
#line 719 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::DATES);  }
#line 2537 "yacc_sql.cpp"
    break;

  case 78: /* type: TEXT_T  */
#line 720 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::TEXTS);  }
#line 2543 "yacc_sql.cpp"
    break;

  case 79: /* type: VECTOR_T  */
#line 721 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::VECTORS);  }
#line 2549 "yacc_sql.cpp"
    break;

  case 80: /* insert_stmt: INSERT INTO ID VALUES values_list  */
#line 726 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_INSERT);
      (yyval.sql_node)->insertion.relation_name = (yyvsp[-2].string);
      if ((yyvsp[0].values_list) != nullptr) {
        (yyval.sql_node)->insertion.values_list.swap(*(yyvsp[0].values_list));
        delete (yyvsp[0].values_list);
      }
      free((yyvsp[-2].string));
    }
#line 2563 "yacc_sql.cpp"
    break;

  case 81: /* insert_stmt: INSERT INTO ID LBRACE attr_list RBRACE VALUES values_list  */
#line 736 "yacc_sql.y"
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
#line 2578 "yacc_sql.cpp"
    break;

  case 82: /* values_list: LBRACE value_list RBRACE  */
#line 750 "yacc_sql.y"
    {
      (yyval.values_list) = new std::vector<std::vector<Value>>;
      (yyval.values_list)->emplace_back(*(yyvsp[-1].value_list));
      delete (yyvsp[-1].value_list);
    }
#line 2588 "yacc_sql.cpp"
    break;

  case 83: /* values_list: values_list COMMA LBRACE value_list RBRACE  */
#line 756 "yacc_sql.y"
    {
      (yyval.values_list)->emplace_back(*(yyvsp[-1].value_list));
      delete (yyvsp[-1].value_list);
    }
#line 2597 "yacc_sql.cpp"
    break;

  case 84: /* digits: NUMBER  */
#line 763 "yacc_sql.y"
    {
      (yyval.digits) = float((yyvsp[0].number));
    }
#line 2605 "yacc_sql.cpp"
    break;

  case 85: /* digits: '-' NUMBER  */
#line 767 "yacc_sql.y"
    {
      (yyval.digits) = float(-(yyvsp[0].number));
    }
#line 2613 "yacc_sql.cpp"
    break;

  case 86: /* digits: FLOAT  */
#line 771 "yacc_sql.y"
    {
      (yyval.digits) = (yyvsp[0].floats);
    }
#line 2621 "yacc_sql.cpp"
    break;

  case 87: /* digits: '-' FLOAT  */
#line 775 "yacc_sql.y"
    {
      (yyval.digits) = (yyvsp[0].floats);
    }
#line 2629 "yacc_sql.cpp"
    break;

  case 88: /* digits_list: %empty  */
#line 782 "yacc_sql.y"
    {
      (yyval.digits_list) = new std::vector<float>();
    }
#line 2637 "yacc_sql.cpp"
    break;

  case 89: /* digits_list: digits  */
#line 786 "yacc_sql.y"
    {
      (yyval.digits_list) = new std::vector<float>();
      (yyval.digits_list)->push_back((yyvsp[0].digits));
    }
#line 2646 "yacc_sql.cpp"
    break;

  case 90: /* digits_list: digits_list COMMA digits  */
#line 791 "yacc_sql.y"
    {
      (yyval.digits_list)->push_back((yyvsp[0].digits));
    }
#line 2654 "yacc_sql.cpp"
    break;

  case 91: /* value_list: %empty  */
#line 798 "yacc_sql.y"
    {
      (yyval.value_list) = new std::vector<Value>;
    }
#line 2662 "yacc_sql.cpp"
    break;

  case 92: /* value_list: value  */
#line 802 "yacc_sql.y"
    {
      (yyval.value_list) = new std::vector<Value>;
      (yyval.value_list)->emplace_back(*(yyvsp[0].value));
      delete (yyvsp[0].value);
    }
#line 2672 "yacc_sql.cpp"
    break;

  case 93: /* value_list: value_list COMMA value  */
#line 808 "yacc_sql.y"
    {
      (yyval.value_list)->emplace_back(*(yyvsp[0].value));
      delete (yyvsp[0].value);
    }
#line 2681 "yacc_sql.cpp"
    break;

  case 94: /* value: nonnegative_value  */
#line 815 "yacc_sql.y"
                      {
      (yyval.value) = (yyvsp[0].value);
    }
#line 2689 "yacc_sql.cpp"
    break;

  case 95: /* value: '-' NUMBER  */
#line 818 "yacc_sql.y"
                 {
      (yyval.value) = new Value(-(yyvsp[0].number));
      (yyloc) = (yylsp[-1]);
    }
#line 2698 "yacc_sql.cpp"
    break;

  case 96: /* value: '-' FLOAT  */
#line 822 "yacc_sql.y"
                {
      (yyval.value) = new Value(-(yyvsp[0].floats));
      (yyloc) = (yylsp[-1]);
    }
#line 2707 "yacc_sql.cpp"
    break;

  case 97: /* nonnegative_value: NUMBER  */
#line 829 "yacc_sql.y"
           {
      (yyval.value) = new Value((yyvsp[0].number));
      (yyloc) = (yylsp[0]);
    }
#line 2716 "yacc_sql.cpp"
    break;

  case 98: /* nonnegative_value: FLOAT  */
#line 833 "yacc_sql.y"
            {
      (yyval.value) = new Value((yyvsp[0].floats));
      (yyloc) = (yylsp[0]);
    }
#line 2725 "yacc_sql.cpp"
    break;

  case 99: /* nonnegative_value: SSS  */
#line 837 "yacc_sql.y"
          {
      char *tmp = common::substr((yyvsp[0].string),1,strlen((yyvsp[0].string))-2);
      (yyval.value) = new Value(tmp);
      free(tmp);
      free((yyvsp[0].string));
    }
#line 2736 "yacc_sql.cpp"
    break;

  case 100: /* nonnegative_value: TRUE  */
#line 843 "yacc_sql.y"
           {
      (yyval.value) = new Value(true);
    }
#line 2744 "yacc_sql.cpp"
    break;

  case 101: /* nonnegative_value: FALSE  */
#line 846 "yacc_sql.y"
            {
      (yyval.value) = new Value(false);
    }
#line 2752 "yacc_sql.cpp"
    break;

  case 102: /* nonnegative_value: NULL_T  */
#line 849 "yacc_sql.y"
             {
      (yyval.value) = new Value(NullValue());
    }
#line 2760 "yacc_sql.cpp"
    break;

  case 103: /* nonnegative_value: LSBRACE digits_list RSBRACE  */
#line 852 "yacc_sql.y"
                                  {
      (yyval.value) = new Value(*(yyvsp[-1].digits_list));
    }
#line 2768 "yacc_sql.cpp"
    break;

  case 104: /* nonnegative_value: STRING_TO_VECTOR LBRACE value_list RBRACE  */
#line 855 "yacc_sql.y"
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
#line 2790 "yacc_sql.cpp"
    break;

  case 105: /* nonnegative_value: VECTOR_TO_STRING LBRACE value_list RBRACE  */
#line 872 "yacc_sql.y"
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
#line 2812 "yacc_sql.cpp"
    break;

  case 106: /* storage_format: %empty  */
#line 893 "yacc_sql.y"
    {
      (yyval.string) = nullptr;
    }
#line 2820 "yacc_sql.cpp"
    break;

  case 107: /* storage_format: STORAGE FORMAT EQ ID  */
#line 897 "yacc_sql.y"
    {
      (yyval.string) = (yyvsp[0].string);
    }
#line 2828 "yacc_sql.cpp"
    break;

  case 108: /* delete_stmt: DELETE FROM ID where  */
#line 904 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DELETE);
      (yyval.sql_node)->deletion.relation_name = (yyvsp[-1].string);
      if ((yyvsp[0].expression) != nullptr) {
        (yyval.sql_node)->deletion.condition = std::unique_ptr<Expression>((yyvsp[0].expression));
      }
      free((yyvsp[-1].string));
    }
#line 2841 "yacc_sql.cpp"
    break;

  case 109: /* update_stmt: UPDATE ID SET set_clauses where  */
#line 916 "yacc_sql.y"
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
#line 2856 "yacc_sql.cpp"
    break;

  case 110: /* set_clauses: set_clause  */
#line 930 "yacc_sql.y"
    {
      (yyval.set_clauses) = new std::vector<SetClauseSqlNode>;
      (yyval.set_clauses)->emplace_back(std::move(*(yyvsp[0].set_clause)));
    }
#line 2865 "yacc_sql.cpp"
    break;

  case 111: /* set_clauses: set_clauses COMMA set_clause  */
#line 935 "yacc_sql.y"
    {
      (yyval.set_clauses)->emplace_back(std::move(*(yyvsp[0].set_clause)));
    }
#line 2873 "yacc_sql.cpp"
    break;

  case 112: /* set_clause: ID EQ expression  */
#line 942 "yacc_sql.y"
    {
      (yyval.set_clause) = new SetClauseSqlNode;
      (yyval.set_clause)->field_name = (yyvsp[-2].string);
      (yyval.set_clause)->value = std::unique_ptr<Expression>((yyvsp[0].expression));
      free((yyvsp[-2].string));
    }
#line 2884 "yacc_sql.cpp"
    break;

  case 113: /* select_stmt: select_core select_union_list  */
#line 952 "yacc_sql.y"
    {
      (yyval.sql_node) = (yyvsp[-1].sql_node);
      if ((yyvsp[0].set_operator_list) != nullptr) {
        (yyval.sql_node)->selection.set_operations.swap(*(yyvsp[0].set_operator_list));
        delete (yyvsp[0].set_operator_list);
      }
    }
#line 2896 "yacc_sql.cpp"
    break;

  case 114: /* select_union_list: %empty  */
#line 963 "yacc_sql.y"
    {
      (yyval.set_operator_list) = nullptr;
    }
#line 2904 "yacc_sql.cpp"
    break;

  case 115: /* select_union_list: select_union_list select_union_item  */
#line 967 "yacc_sql.y"
    {
      if ((yyvsp[-1].set_operator_list) != nullptr) {
        (yyval.set_operator_list) = (yyvsp[-1].set_operator_list);
      } else {
        (yyval.set_operator_list) = new std::vector<SetOperatorSqlNode>();
      }
      (yyval.set_operator_list)->emplace_back(std::move(*(yyvsp[0].set_operator_node)));
      delete (yyvsp[0].set_operator_node);
    }
#line 2918 "yacc_sql.cpp"
    break;

  case 116: /* select_union_item: UNION select_core  */
#line 980 "yacc_sql.y"
    {
      (yyval.set_operator_node) = new SetOperatorSqlNode;
      (yyval.set_operator_node)->union_all = false;
      (yyval.set_operator_node)->select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      delete (yyvsp[0].sql_node);
    }
#line 2929 "yacc_sql.cpp"
    break;

  case 117: /* select_union_item: UNION ALL select_core  */
#line 987 "yacc_sql.y"
    {
      (yyval.set_operator_node) = new SetOperatorSqlNode;
      (yyval.set_operator_node)->union_all = true;
      (yyval.set_operator_node)->select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      delete (yyvsp[0].sql_node);
    }
#line 2940 "yacc_sql.cpp"
    break;

  case 118: /* select_core: SELECT expression_list FROM rel_list where group_by opt_having opt_order_by opt_limit  */
#line 997 "yacc_sql.y"
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
#line 2982 "yacc_sql.cpp"
    break;

  case 119: /* select_core: SELECT expression_list FROM relation INNER JOIN join_clauses where group_by  */
#line 1035 "yacc_sql.y"
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
#line 3016 "yacc_sql.cpp"
    break;

  case 120: /* calc_stmt: CALC expression_list  */
#line 1068 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CALC);
      (yyval.sql_node)->calc.expressions.swap(*(yyvsp[0].expression_list));
      delete (yyvsp[0].expression_list);
    }
#line 3026 "yacc_sql.cpp"
    break;

  case 121: /* calc_stmt: SELECT expression_list  */
#line 1074 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CALC);
      (yyval.sql_node)->calc.expressions.swap(*(yyvsp[0].expression_list));
      delete (yyvsp[0].expression_list);
    }
#line 3036 "yacc_sql.cpp"
    break;

  case 122: /* expression_list: %empty  */
#line 1082 "yacc_sql.y"
                {
      (yyval.expression_list) = new std::vector<std::unique_ptr<Expression>>;
    }
#line 3044 "yacc_sql.cpp"
    break;

  case 123: /* expression_list: expression alias  */
#line 1086 "yacc_sql.y"
    {
      (yyval.expression_list) = new std::vector<std::unique_ptr<Expression>>;
      if (nullptr != (yyvsp[0].string)) {
        (yyvsp[-1].expression)->set_alias((yyvsp[0].string));
      }
      (yyval.expression_list)->emplace_back((yyvsp[-1].expression));
      free((yyvsp[0].string));
    }
#line 3057 "yacc_sql.cpp"
    break;

  case 124: /* expression_list: expression alias COMMA expression_list  */
#line 1095 "yacc_sql.y"
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
#line 3074 "yacc_sql.cpp"
    break;

  case 125: /* expression: expression '+' expression  */
#line 1110 "yacc_sql.y"
                              {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::ADD, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 3082 "yacc_sql.cpp"
    break;

  case 126: /* expression: expression '-' expression  */
#line 1113 "yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::SUB, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 3090 "yacc_sql.cpp"
    break;

  case 127: /* expression: expression '*' expression  */
#line 1116 "yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::MUL, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 3098 "yacc_sql.cpp"
    break;

  case 128: /* expression: expression '/' expression  */
#line 1119 "yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::DIV, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 3106 "yacc_sql.cpp"
    break;

  case 129: /* expression: LBRACE expression_list RBRACE  */
#line 1122 "yacc_sql.y"
                                    {
      if ((yyvsp[-1].expression_list)->size() == 1) {
        (yyval.expression) = (yyvsp[-1].expression_list)->front().get();
      } else {
        (yyval.expression) = new ListExpr(std::move(*(yyvsp[-1].expression_list)));
      }
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3119 "yacc_sql.cpp"
    break;

  case 130: /* expression: '-' expression  */
#line 1130 "yacc_sql.y"
                                  {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::NEGATIVE, (yyvsp[0].expression), nullptr, sql_string, &(yyloc));
    }
#line 3127 "yacc_sql.cpp"
    break;

  case 131: /* expression: nonnegative_value  */
#line 1133 "yacc_sql.y"
                        {
      (yyval.expression) = new ValueExpr(*(yyvsp[0].value));
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
      delete (yyvsp[0].value);
    }
#line 3137 "yacc_sql.cpp"
    break;

  case 132: /* expression: rel_attr  */
#line 1138 "yacc_sql.y"
               {
      RelAttrSqlNode *node = (yyvsp[0].rel_attr);
      (yyval.expression) = new UnboundFieldExpr(node->relation_name, node->attribute_name);
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
      delete (yyvsp[0].rel_attr);
    }
#line 3148 "yacc_sql.cpp"
    break;

  case 133: /* expression: '*'  */
#line 1144 "yacc_sql.y"
          {
      (yyval.expression) = new StarExpr();
    }
#line 3156 "yacc_sql.cpp"
    break;

  case 134: /* expression: ID DOT '*'  */
#line 1147 "yacc_sql.y"
                 {
      (yyval.expression) = new StarExpr((yyvsp[-2].string));
    }
#line 3164 "yacc_sql.cpp"
    break;

  case 135: /* expression: func_expr  */
#line 1150 "yacc_sql.y"
                {
      (yyval.expression) = (yyvsp[0].expression);      // AggrFuncExpr
    }
#line 3172 "yacc_sql.cpp"
    break;

  case 136: /* expression: sub_query_expr  */
#line 1153 "yacc_sql.y"
                     {
      (yyval.expression) = (yyvsp[0].expression); // SubQueryExpr
    }
#line 3180 "yacc_sql.cpp"
    break;

  case 137: /* alias: %empty  */
#line 1160 "yacc_sql.y"
                {
      (yyval.string) = nullptr;
    }
#line 3188 "yacc_sql.cpp"
    break;

  case 138: /* alias: ID  */
#line 1163 "yacc_sql.y"
         {
      (yyval.string) = (yyvsp[0].string);
    }
#line 3196 "yacc_sql.cpp"
    break;

  case 139: /* alias: AS ID  */
#line 1166 "yacc_sql.y"
            {
      (yyval.string) = (yyvsp[0].string);
    }
#line 3204 "yacc_sql.cpp"
    break;

  case 140: /* func_expr: ID LBRACE expression_list RBRACE  */
#line 1172 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr((yyvsp[-3].string), std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3213 "yacc_sql.cpp"
    break;

  case 141: /* func_expr: DISTANCE LBRACE expression_list RBRACE  */
#line 1177 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr("distance", std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3222 "yacc_sql.cpp"
    break;

  case 142: /* func_expr: STRING_TO_VECTOR LBRACE expression_list RBRACE  */
#line 1182 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr("string_to_vector", std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3231 "yacc_sql.cpp"
    break;

  case 143: /* func_expr: VECTOR_TO_STRING LBRACE expression_list RBRACE  */
#line 1187 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr("vector_to_string", std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3240 "yacc_sql.cpp"
    break;

  case 144: /* sub_query_expr: LBRACE select_stmt RBRACE  */
#line 1195 "yacc_sql.y"
    {
      (yyval.expression) = new SubQueryExpr((yyvsp[-1].sql_node)->selection);
    }
#line 3248 "yacc_sql.cpp"
    break;

  case 145: /* rel_attr: ID  */
#line 1201 "yacc_sql.y"
       {
      (yyval.rel_attr) = new RelAttrSqlNode;
      (yyval.rel_attr)->attribute_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 3258 "yacc_sql.cpp"
    break;

  case 146: /* rel_attr: ID DOT ID  */
#line 1206 "yacc_sql.y"
                {
      (yyval.rel_attr) = new RelAttrSqlNode;
      (yyval.rel_attr)->relation_name  = (yyvsp[-2].string);
      (yyval.rel_attr)->attribute_name = (yyvsp[0].string);
      free((yyvsp[-2].string));
      free((yyvsp[0].string));
    }
#line 3270 "yacc_sql.cpp"
    break;

  case 147: /* relation: ID  */
#line 1216 "yacc_sql.y"
       {
      (yyval.string) = (yyvsp[0].string);
    }
#line 3278 "yacc_sql.cpp"
    break;

  case 148: /* rel_list: relation alias  */
#line 1222 "yacc_sql.y"
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
#line 3293 "yacc_sql.cpp"
    break;

  case 149: /* rel_list: relation alias COMMA rel_list  */
#line 1232 "yacc_sql.y"
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
#line 3312 "yacc_sql.cpp"
    break;

  case 150: /* join_clauses: relation ON condition  */
#line 1250 "yacc_sql.y"
    {
      (yyval.join_clauses) = new JoinSqlNode;
      (yyval.join_clauses)->relations.emplace_back((yyvsp[-2].string));
      (yyval.join_clauses)->conditions = std::unique_ptr<Expression>((yyvsp[0].expression));
      free((yyvsp[-2].string));
    }
#line 3323 "yacc_sql.cpp"
    break;

  case 151: /* join_clauses: relation ON condition INNER JOIN join_clauses  */
#line 1257 "yacc_sql.y"
    {
      (yyval.join_clauses) = (yyvsp[0].join_clauses);
      (yyval.join_clauses)->relations.emplace_back((yyvsp[-5].string));
      auto ptr = (yyval.join_clauses)->conditions.release();
      (yyval.join_clauses)->conditions = std::make_unique<ConjunctionExpr>(ConjunctionExpr::Type::AND, ptr, (yyvsp[-3].expression));
      free((yyvsp[-5].string));
    }
#line 3335 "yacc_sql.cpp"
    break;

  case 152: /* where: %empty  */
#line 1268 "yacc_sql.y"
    {
      (yyval.expression) = nullptr;
    }
#line 3343 "yacc_sql.cpp"
    break;

  case 153: /* where: WHERE condition  */
#line 1271 "yacc_sql.y"
                      {
      (yyval.expression) = (yyvsp[0].expression);  
    }
#line 3351 "yacc_sql.cpp"
    break;

  case 154: /* condition: expression comp_op expression  */
#line 1278 "yacc_sql.y"
    {
      (yyval.expression) = new ComparisonExpr((yyvsp[-1].comp), (yyvsp[-2].expression), (yyvsp[0].expression));
    }
#line 3359 "yacc_sql.cpp"
    break;

  case 155: /* condition: comp_op expression  */
#line 1282 "yacc_sql.y"
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
#line 3376 "yacc_sql.cpp"
    break;

  case 156: /* condition: condition AND condition  */
#line 1295 "yacc_sql.y"
    {
      (yyval.expression) = new ConjunctionExpr(ConjunctionExpr::Type::AND, (yyvsp[-2].expression), (yyvsp[0].expression));
    }
#line 3384 "yacc_sql.cpp"
    break;

  case 157: /* condition: condition OR condition  */
#line 1299 "yacc_sql.y"
    {
      (yyval.expression) = new ConjunctionExpr(ConjunctionExpr::Type::OR, (yyvsp[-2].expression), (yyvsp[0].expression));
    }
#line 3392 "yacc_sql.cpp"
    break;

  case 158: /* comp_op: EQ  */
#line 1305 "yacc_sql.y"
         { (yyval.comp) = EQUAL_TO; }
#line 3398 "yacc_sql.cpp"
    break;

  case 159: /* comp_op: LT  */
#line 1306 "yacc_sql.y"
         { (yyval.comp) = LESS_THAN; }
#line 3404 "yacc_sql.cpp"
    break;

  case 160: /* comp_op: GT  */
#line 1307 "yacc_sql.y"
         { (yyval.comp) = GREAT_THAN; }
#line 3410 "yacc_sql.cpp"
    break;

  case 161: /* comp_op: LE  */
#line 1308 "yacc_sql.y"
         { (yyval.comp) = LESS_EQUAL; }
#line 3416 "yacc_sql.cpp"
    break;

  case 162: /* comp_op: GE  */
#line 1309 "yacc_sql.y"
         { (yyval.comp) = GREAT_EQUAL; }
#line 3422 "yacc_sql.cpp"
    break;

  case 163: /* comp_op: NE  */
#line 1310 "yacc_sql.y"
         { (yyval.comp) = NOT_EQUAL; }
#line 3428 "yacc_sql.cpp"
    break;

  case 164: /* comp_op: IS  */
#line 1311 "yacc_sql.y"
         { (yyval.comp) = IS_OP; }
#line 3434 "yacc_sql.cpp"
    break;

  case 165: /* comp_op: IS NOT  */
#line 1312 "yacc_sql.y"
             { (yyval.comp) = IS_NOT_OP; }
#line 3440 "yacc_sql.cpp"
    break;

  case 166: /* comp_op: LIKE  */
#line 1313 "yacc_sql.y"
           { (yyval.comp) = LIKE_OP;}
#line 3446 "yacc_sql.cpp"
    break;

  case 167: /* comp_op: NOT LIKE  */
#line 1314 "yacc_sql.y"
               {(yyval.comp) = NOT_LIKE_OP;}
#line 3452 "yacc_sql.cpp"
    break;

  case 168: /* comp_op: IN  */
#line 1315 "yacc_sql.y"
         { (yyval.comp) = IN_OP; }
#line 3458 "yacc_sql.cpp"
    break;

  case 169: /* comp_op: NOT IN  */
#line 1316 "yacc_sql.y"
             { (yyval.comp) = NOT_IN_OP; }
#line 3464 "yacc_sql.cpp"
    break;

  case 170: /* comp_op: EXISTS  */
#line 1317 "yacc_sql.y"
             { (yyval.comp) = EXISTS_OP; }
#line 3470 "yacc_sql.cpp"
    break;

  case 171: /* comp_op: NOT EXISTS  */
#line 1318 "yacc_sql.y"
                 { (yyval.comp) = NOT_EXISTS_OP; }
#line 3476 "yacc_sql.cpp"
    break;

  case 172: /* opt_order_by: %empty  */
#line 1323 "yacc_sql.y"
    {
      (yyval.orderby_list) = nullptr;
    }
#line 3484 "yacc_sql.cpp"
    break;

  case 173: /* opt_order_by: ORDER BY sort_list  */
#line 1327 "yacc_sql.y"
    {
      (yyval.orderby_list) = (yyvsp[0].orderby_list);
      std::reverse((yyval.orderby_list)->begin(),(yyval.orderby_list)->end());
    }
#line 3493 "yacc_sql.cpp"
    break;

  case 174: /* sort_list: sort_unit  */
#line 1335 "yacc_sql.y"
        {
      (yyval.orderby_list) = new std::vector<OrderBySqlNode>;
      (yyval.orderby_list)->emplace_back(std::move(*(yyvsp[0].orderby_unit)));
	}
#line 3502 "yacc_sql.cpp"
    break;

  case 175: /* sort_list: sort_unit COMMA sort_list  */
#line 1340 "yacc_sql.y"
        {
      (yyvsp[0].orderby_list)->emplace_back(std::move(*(yyvsp[-2].orderby_unit)));
      (yyval.orderby_list) = (yyvsp[0].orderby_list);
	}
#line 3511 "yacc_sql.cpp"
    break;

  case 176: /* sort_unit: expression  */
#line 1348 "yacc_sql.y"
        {
      (yyval.orderby_unit) = new OrderBySqlNode();
      (yyval.orderby_unit)->expr = std::unique_ptr<Expression>((yyvsp[0].expression));
      (yyval.orderby_unit)->is_asc = true;
	}
#line 3521 "yacc_sql.cpp"
    break;

  case 177: /* sort_unit: expression DESC  */
#line 1354 "yacc_sql.y"
        {
      (yyval.orderby_unit) = new OrderBySqlNode();
      (yyval.orderby_unit)->expr = std::unique_ptr<Expression>((yyvsp[-1].expression));
      (yyval.orderby_unit)->is_asc = false;
	}
#line 3531 "yacc_sql.cpp"
    break;

  case 178: /* sort_unit: expression ASC  */
#line 1360 "yacc_sql.y"
        {
      (yyval.orderby_unit) = new OrderBySqlNode(); // 默认是升序
      (yyval.orderby_unit)->expr = std::unique_ptr<Expression>((yyvsp[-1].expression));
      (yyval.orderby_unit)->is_asc = true;
	}
#line 3541 "yacc_sql.cpp"
    break;

  case 179: /* group_by: %empty  */
#line 1369 "yacc_sql.y"
    {
      (yyval.expression_list) = nullptr;
    }
#line 3549 "yacc_sql.cpp"
    break;

  case 180: /* group_by: GROUP BY expression_list  */
#line 1373 "yacc_sql.y"
    {
      (yyval.expression_list) = (yyvsp[0].expression_list);
    }
#line 3557 "yacc_sql.cpp"
    break;

  case 181: /* opt_having: %empty  */
#line 1380 "yacc_sql.y"
    {
      (yyval.expression) = nullptr;
    }
#line 3565 "yacc_sql.cpp"
    break;

  case 182: /* opt_having: HAVING condition  */
#line 1384 "yacc_sql.y"
    {
      (yyval.expression) = (yyvsp[0].expression);
    }
#line 3573 "yacc_sql.cpp"
    break;

  case 183: /* opt_limit: %empty  */
#line 1391 "yacc_sql.y"
    {
      (yyval.limited_info) = nullptr;
    }
#line 3581 "yacc_sql.cpp"
    break;

  case 184: /* opt_limit: LIMIT NUMBER  */
#line 1395 "yacc_sql.y"
    {
      (yyval.limited_info) = new LimitSqlNode();
      (yyval.limited_info)->number = (yyvsp[0].number);
    }
#line 3590 "yacc_sql.cpp"
    break;

  case 185: /* explain_stmt: EXPLAIN command_wrapper  */
#line 1403 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_EXPLAIN);
      (yyval.sql_node)->explain.sql_node = std::unique_ptr<ParsedSqlNode>((yyvsp[0].sql_node));
    }
#line 3599 "yacc_sql.cpp"
    break;

  case 186: /* set_variable_stmt: SET ID EQ value  */
#line 1411 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SET_VARIABLE);
      (yyval.sql_node)->set_variable.name  = (yyvsp[-2].string);
      (yyval.sql_node)->set_variable.value = *(yyvsp[0].value);
      free((yyvsp[-2].string));
      delete (yyvsp[0].value);
    }
#line 3611 "yacc_sql.cpp"
    break;


#line 3615 "yacc_sql.cpp"

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

#line 1423 "yacc_sql.y"

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
