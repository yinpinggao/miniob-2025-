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
  YYSYMBOL_EXISTS = 9,                     /* EXISTS  */
  YYSYMBOL_GROUP = 10,                     /* GROUP  */
  YYSYMBOL_HAVING = 11,                    /* HAVING  */
  YYSYMBOL_ORDER = 12,                     /* ORDER  */
  YYSYMBOL_TABLE = 13,                     /* TABLE  */
  YYSYMBOL_TABLES = 14,                    /* TABLES  */
  YYSYMBOL_INDEX = 15,                     /* INDEX  */
  YYSYMBOL_CALC = 16,                      /* CALC  */
  YYSYMBOL_SELECT = 17,                    /* SELECT  */
  YYSYMBOL_DESC = 18,                      /* DESC  */
  YYSYMBOL_SHOW = 19,                      /* SHOW  */
  YYSYMBOL_SYNC = 20,                      /* SYNC  */
  YYSYMBOL_INSERT = 21,                    /* INSERT  */
  YYSYMBOL_DELETE = 22,                    /* DELETE  */
  YYSYMBOL_UPDATE = 23,                    /* UPDATE  */
  YYSYMBOL_LBRACE = 24,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 25,                    /* RBRACE  */
  YYSYMBOL_LSBRACE = 26,                   /* LSBRACE  */
  YYSYMBOL_RSBRACE = 27,                   /* RSBRACE  */
  YYSYMBOL_COMMA = 28,                     /* COMMA  */
  YYSYMBOL_TRX_BEGIN = 29,                 /* TRX_BEGIN  */
  YYSYMBOL_TRX_COMMIT = 30,                /* TRX_COMMIT  */
  YYSYMBOL_TRX_ROLLBACK = 31,              /* TRX_ROLLBACK  */
  YYSYMBOL_INT_T = 32,                     /* INT_T  */
  YYSYMBOL_IN = 33,                        /* IN  */
  YYSYMBOL_TRUE = 34,                      /* TRUE  */
  YYSYMBOL_FALSE = 35,                     /* FALSE  */
  YYSYMBOL_STRING_T = 36,                  /* STRING_T  */
  YYSYMBOL_FLOAT_T = 37,                   /* FLOAT_T  */
  YYSYMBOL_DATE_T = 38,                    /* DATE_T  */
  YYSYMBOL_TEXT_T = 39,                    /* TEXT_T  */
  YYSYMBOL_VECTOR_T = 40,                  /* VECTOR_T  */
  YYSYMBOL_NOT = 41,                       /* NOT  */
  YYSYMBOL_UNIQUE = 42,                    /* UNIQUE  */
  YYSYMBOL_NULL_T = 43,                    /* NULL_T  */
  YYSYMBOL_LIMIT = 44,                     /* LIMIT  */
  YYSYMBOL_NULLABLE = 45,                  /* NULLABLE  */
  YYSYMBOL_HELP = 46,                      /* HELP  */
  YYSYMBOL_QUOTE = 47,                     /* QUOTE  */
  YYSYMBOL_EXIT = 48,                      /* EXIT  */
  YYSYMBOL_DOT = 49,                       /* DOT  */
  YYSYMBOL_INTO = 50,                      /* INTO  */
  YYSYMBOL_VALUES = 51,                    /* VALUES  */
  YYSYMBOL_FROM = 52,                      /* FROM  */
  YYSYMBOL_WHERE = 53,                     /* WHERE  */
  YYSYMBOL_AND = 54,                       /* AND  */
  YYSYMBOL_OR = 55,                        /* OR  */
  YYSYMBOL_SET = 56,                       /* SET  */
  YYSYMBOL_ON = 57,                        /* ON  */
  YYSYMBOL_INFILE = 58,                    /* INFILE  */
  YYSYMBOL_EXPLAIN = 59,                   /* EXPLAIN  */
  YYSYMBOL_STORAGE = 60,                   /* STORAGE  */
  YYSYMBOL_FORMAT = 61,                    /* FORMAT  */
  YYSYMBOL_INNER = 62,                     /* INNER  */
  YYSYMBOL_JOIN = 63,                      /* JOIN  */
  YYSYMBOL_UNION = 64,                     /* UNION  */
  YYSYMBOL_ALL = 65,                       /* ALL  */
  YYSYMBOL_VIEW = 66,                      /* VIEW  */
  YYSYMBOL_WITH = 67,                      /* WITH  */
  YYSYMBOL_STRING_TO_VECTOR = 68,          /* STRING_TO_VECTOR  */
  YYSYMBOL_VECTOR_TO_STRING = 69,          /* VECTOR_TO_STRING  */
  YYSYMBOL_DISTANCE = 70,                  /* DISTANCE  */
  YYSYMBOL_TYPE = 71,                      /* TYPE  */
  YYSYMBOL_LISTS = 72,                     /* LISTS  */
  YYSYMBOL_PROBES = 73,                    /* PROBES  */
  YYSYMBOL_IVFFLAT = 74,                   /* IVFFLAT  */
  YYSYMBOL_EQ = 75,                        /* EQ  */
  YYSYMBOL_LT = 76,                        /* LT  */
  YYSYMBOL_GT = 77,                        /* GT  */
  YYSYMBOL_LE = 78,                        /* LE  */
  YYSYMBOL_GE = 79,                        /* GE  */
  YYSYMBOL_NE = 80,                        /* NE  */
  YYSYMBOL_LIKE = 81,                      /* LIKE  */
  YYSYMBOL_IS = 82,                        /* IS  */
  YYSYMBOL_NUMBER = 83,                    /* NUMBER  */
  YYSYMBOL_FLOAT = 84,                     /* FLOAT  */
  YYSYMBOL_ID = 85,                        /* ID  */
  YYSYMBOL_SSS = 86,                       /* SSS  */
  YYSYMBOL_87_ = 87,                       /* '+'  */
  YYSYMBOL_88_ = 88,                       /* '-'  */
  YYSYMBOL_89_ = 89,                       /* '*'  */
  YYSYMBOL_90_ = 90,                       /* '/'  */
  YYSYMBOL_UMINUS = 91,                    /* UMINUS  */
  YYSYMBOL_YYACCEPT = 92,                  /* $accept  */
  YYSYMBOL_commands = 93,                  /* commands  */
  YYSYMBOL_command_wrapper = 94,           /* command_wrapper  */
  YYSYMBOL_exit_stmt = 95,                 /* exit_stmt  */
  YYSYMBOL_help_stmt = 96,                 /* help_stmt  */
  YYSYMBOL_sync_stmt = 97,                 /* sync_stmt  */
  YYSYMBOL_begin_stmt = 98,                /* begin_stmt  */
  YYSYMBOL_commit_stmt = 99,               /* commit_stmt  */
  YYSYMBOL_rollback_stmt = 100,            /* rollback_stmt  */
  YYSYMBOL_drop_table_stmt = 101,          /* drop_table_stmt  */
  YYSYMBOL_show_tables_stmt = 102,         /* show_tables_stmt  */
  YYSYMBOL_desc_table_stmt = 103,          /* desc_table_stmt  */
  YYSYMBOL_show_index_stmt = 104,          /* show_index_stmt  */
  YYSYMBOL_create_index_stmt = 105,        /* create_index_stmt  */
  YYSYMBOL_opt_unique = 106,               /* opt_unique  */
  YYSYMBOL_index_type = 107,               /* index_type  */
  YYSYMBOL_vector_index_config = 108,      /* vector_index_config  */
  YYSYMBOL_attr_list = 109,                /* attr_list  */
  YYSYMBOL_drop_index_stmt = 110,          /* drop_index_stmt  */
  YYSYMBOL_create_table_stmt = 111,        /* create_table_stmt  */
  YYSYMBOL_create_view_stmt = 112,         /* create_view_stmt  */
  YYSYMBOL_drop_view_stmt = 113,           /* drop_view_stmt  */
  YYSYMBOL_attr_def_list = 114,            /* attr_def_list  */
  YYSYMBOL_attr_def = 115,                 /* attr_def  */
  YYSYMBOL_nullable_constraint = 116,      /* nullable_constraint  */
  YYSYMBOL_type = 117,                     /* type  */
  YYSYMBOL_insert_stmt = 118,              /* insert_stmt  */
  YYSYMBOL_values_list = 119,              /* values_list  */
  YYSYMBOL_digits = 120,                   /* digits  */
  YYSYMBOL_digits_list = 121,              /* digits_list  */
  YYSYMBOL_value_list = 122,               /* value_list  */
  YYSYMBOL_value = 123,                    /* value  */
  YYSYMBOL_nonnegative_value = 124,        /* nonnegative_value  */
  YYSYMBOL_storage_format = 125,           /* storage_format  */
  YYSYMBOL_delete_stmt = 126,              /* delete_stmt  */
  YYSYMBOL_update_stmt = 127,              /* update_stmt  */
  YYSYMBOL_set_clauses = 128,              /* set_clauses  */
  YYSYMBOL_set_clause = 129,               /* set_clause  */
  YYSYMBOL_select_stmt = 130,              /* select_stmt  */
  YYSYMBOL_select_union_list = 131,        /* select_union_list  */
  YYSYMBOL_select_union_item = 132,        /* select_union_item  */
  YYSYMBOL_select_core = 133,              /* select_core  */
  YYSYMBOL_calc_stmt = 134,                /* calc_stmt  */
  YYSYMBOL_expression_list = 135,          /* expression_list  */
  YYSYMBOL_expression = 136,               /* expression  */
  YYSYMBOL_alias = 137,                    /* alias  */
  YYSYMBOL_func_expr = 138,                /* func_expr  */
  YYSYMBOL_sub_query_expr = 139,           /* sub_query_expr  */
  YYSYMBOL_rel_attr = 140,                 /* rel_attr  */
  YYSYMBOL_relation = 141,                 /* relation  */
  YYSYMBOL_rel_list = 142,                 /* rel_list  */
  YYSYMBOL_join_clauses = 143,             /* join_clauses  */
  YYSYMBOL_where = 144,                    /* where  */
  YYSYMBOL_condition = 145,                /* condition  */
  YYSYMBOL_comp_op = 146,                  /* comp_op  */
  YYSYMBOL_opt_order_by = 147,             /* opt_order_by  */
  YYSYMBOL_sort_list = 148,                /* sort_list  */
  YYSYMBOL_sort_unit = 149,                /* sort_unit  */
  YYSYMBOL_group_by = 150,                 /* group_by  */
  YYSYMBOL_opt_having = 151,               /* opt_having  */
  YYSYMBOL_opt_limit = 152,                /* opt_limit  */
  YYSYMBOL_explain_stmt = 153,             /* explain_stmt  */
  YYSYMBOL_set_variable_stmt = 154,        /* set_variable_stmt  */
  YYSYMBOL_opt_semicolon = 155             /* opt_semicolon  */
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
#define YYFINAL  80
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   418

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  92
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  64
/* YYNRULES -- Number of rules.  */
#define YYNRULES  175
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  354

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   342


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
       2,     2,    89,    87,     2,    88,     2,    90,     2,     2,
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
      85,    86,    91
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   291,   291,   299,   300,   301,   302,   303,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,   319,   320,   324,   330,   335,   341,   347,
     353,   359,   366,   372,   380,   390,   402,   418,   419,   423,
     430,   437,   446,   458,   464,   473,   483,   487,   491,   495,
     499,   506,   514,   526,   536,   539,   552,   570,   599,   603,
     607,   612,   618,   619,   620,   621,   622,   623,   627,   637,
     651,   657,   664,   668,   672,   676,   684,   687,   692,   700,
     703,   709,   717,   720,   724,   731,   735,   739,   745,   748,
     751,   754,   757,   774,   795,   798,   805,   817,   831,   836,
     843,   853,   865,   868,   881,   888,   898,   936,   969,   975,
     984,   987,   996,  1012,  1015,  1018,  1021,  1024,  1032,  1035,
    1040,  1046,  1049,  1052,  1055,  1062,  1065,  1068,  1073,  1078,
    1083,  1088,  1096,  1103,  1108,  1118,  1124,  1134,  1151,  1158,
    1170,  1173,  1179,  1183,  1196,  1200,  1207,  1208,  1209,  1210,
    1211,  1212,  1213,  1214,  1215,  1216,  1217,  1218,  1219,  1220,
    1225,  1228,  1236,  1241,  1249,  1255,  1261,  1271,  1274,  1282,
    1285,  1293,  1296,  1304,  1312,  1323
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
  "ASC", "BY", "CREATE", "DROP", "EXISTS", "GROUP", "HAVING", "ORDER",
  "TABLE", "TABLES", "INDEX", "CALC", "SELECT", "DESC", "SHOW", "SYNC",
  "INSERT", "DELETE", "UPDATE", "LBRACE", "RBRACE", "LSBRACE", "RSBRACE",
  "COMMA", "TRX_BEGIN", "TRX_COMMIT", "TRX_ROLLBACK", "INT_T", "IN",
  "TRUE", "FALSE", "STRING_T", "FLOAT_T", "DATE_T", "TEXT_T", "VECTOR_T",
  "NOT", "UNIQUE", "NULL_T", "LIMIT", "NULLABLE", "HELP", "QUOTE", "EXIT",
  "DOT", "INTO", "VALUES", "FROM", "WHERE", "AND", "OR", "SET", "ON",
  "INFILE", "EXPLAIN", "STORAGE", "FORMAT", "INNER", "JOIN", "UNION",
  "ALL", "VIEW", "WITH", "STRING_TO_VECTOR", "VECTOR_TO_STRING",
  "DISTANCE", "TYPE", "LISTS", "PROBES", "IVFFLAT", "EQ", "LT", "GT", "LE",
  "GE", "NE", "LIKE", "IS", "NUMBER", "FLOAT", "ID", "SSS", "'+'", "'-'",
  "'*'", "'/'", "UMINUS", "$accept", "commands", "command_wrapper",
  "exit_stmt", "help_stmt", "sync_stmt", "begin_stmt", "commit_stmt",
  "rollback_stmt", "drop_table_stmt", "show_tables_stmt",
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

#define YYPACT_NINF (-217)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-85)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     353,     2,     4,   249,   249,   -61,    46,  -217,   -24,   -17,
     -44,  -217,  -217,  -217,  -217,  -217,   -29,   353,   114,    68,
    -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,
    -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,  -217,
    -217,  -217,  -217,    53,   105,  -217,    55,   116,    61,    66,
      92,   115,    60,  -217,  -217,  -217,   157,   169,   178,  -217,
    -217,     6,  -217,   249,  -217,  -217,  -217,    18,  -217,  -217,
    -217,   153,  -217,  -217,   158,   127,   129,   159,   143,  -217,
    -217,  -217,  -217,   155,    -3,   131,    35,   132,  -217,   163,
    -217,   249,   196,   197,  -217,  -217,    17,  -217,   141,   256,
     256,   249,   249,   -51,  -217,   139,  -217,   249,   249,   249,
     249,   195,   140,   140,     7,   174,   144,    -6,     8,  -217,
     145,   171,    48,   176,   211,   154,   180,   160,   153,  -217,
    -217,  -217,  -217,  -217,    60,   322,    64,  -217,    88,   213,
     117,   215,   223,   224,  -217,  -217,  -217,   107,   107,  -217,
    -217,   249,  -217,     5,   174,  -217,   154,   226,   227,  -217,
     177,   -10,  -217,   232,   233,   103,  -217,  -217,   211,  -217,
      97,   231,   188,   211,  -217,   179,  -217,   238,   242,   184,
    -217,  -217,   142,   150,  -217,    -6,  -217,  -217,  -217,  -217,
    -217,  -217,   208,   244,   264,   251,    -6,   250,  -217,  -217,
       3,  -217,  -217,  -217,  -217,  -217,  -217,  -217,   236,    84,
     152,   249,   249,   144,  -217,    -6,    -6,  -217,  -217,  -217,
    -217,  -217,  -217,  -217,  -217,  -217,    40,   145,   254,   200,
    -217,   257,   154,   282,   263,  -217,   140,   140,   283,   277,
     243,   151,   269,  -217,  -217,  -217,  -217,   249,   227,   227,
      67,    67,  -217,   217,   255,  -217,  -217,  -217,   231,   241,
    -217,   154,  -217,   211,   154,   265,   174,     9,  -217,   249,
     227,   302,   226,  -217,    -6,    67,  -217,   266,   296,  -217,
    -217,    71,   298,  -217,   303,   227,   264,  -217,   152,   321,
     285,   250,   166,    85,   211,  -217,   276,  -217,    -9,  -217,
     249,   247,  -217,  -217,  -217,  -217,   307,   273,    22,  -217,
     319,  -217,   138,  -217,   140,  -217,  -217,   249,   274,   275,
    -217,  -217,   267,   279,   323,  -217,   326,   284,   288,   287,
     289,   279,   278,   167,   331,  -217,   294,   295,   293,   304,
      -6,    -6,   349,   350,   308,   312,   305,   311,    -6,    -6,
     362,   363,  -217,  -217
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    38,     0,   110,   110,     0,     0,    27,     0,     0,
       0,    28,    29,    30,    26,    25,     0,     0,     0,     0,
      24,    23,    17,    18,    19,    20,     9,    10,    11,    14,
      12,    13,     8,    15,    16,     5,     7,     6,     3,   102,
       4,    21,    22,     0,     0,    37,     0,     0,     0,     0,
       0,   110,    76,    88,    89,    90,     0,     0,     0,    85,
      86,   133,    87,     0,   121,   119,   108,   125,   123,   124,
     120,   109,    33,    32,     0,     0,     0,     0,     0,   173,
       1,   175,     2,   101,    94,     0,     0,     0,    31,     0,
      53,   110,     0,     0,    72,    74,     0,    77,     0,    79,
      79,   110,   110,     0,   118,     0,   126,     0,     0,     0,
       0,   111,     0,     0,     0,   140,     0,     0,     0,   103,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   132,
     117,    73,    75,    91,     0,     0,     0,    80,   119,     0,
       0,     0,     0,     0,   134,   122,   127,   113,   114,   115,
     116,   110,   135,   125,   140,    34,     0,     0,     0,    96,
       0,   140,    98,     0,     0,     0,   174,    82,     0,   104,
       0,    54,     0,     0,    50,     0,    51,    43,     0,     0,
      45,    78,    85,    86,    92,     0,   130,    93,   131,   129,
     128,   112,     0,   136,   167,     0,    79,    68,   158,   156,
       0,   146,   147,   148,   149,   150,   151,   154,   152,     0,
     141,     0,     0,     0,    97,    79,    79,    83,    84,   105,
      62,    63,    64,    65,    66,    67,    61,     0,     0,     0,
      49,     0,     0,     0,     0,    81,     0,     0,     0,   169,
       0,     0,     0,   159,   157,   155,   153,     0,     0,     0,
     143,   100,    99,     0,     0,    60,    59,    57,    54,    94,
      95,     0,    44,     0,     0,     0,   140,   125,   137,   110,
       0,   160,     0,    70,    79,   142,   144,   145,     0,    58,
      55,    48,     0,    52,     0,     0,   167,   168,   170,     0,
     171,    69,     0,    61,     0,    47,     0,    35,   138,   107,
       0,     0,   106,    71,    56,    46,     0,     0,   164,   161,
     162,   172,     0,    36,     0,   166,   165,     0,     0,     0,
     139,   163,     0,     0,     0,    39,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    40,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    41,    42
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -217,  -217,   372,  -217,  -217,  -217,  -217,  -217,  -217,  -217,
    -217,  -217,  -217,  -217,  -217,    62,  -217,  -146,  -217,  -217,
    -217,  -217,   136,   168,   104,  -217,  -217,   124,   268,  -217,
     -92,  -106,   -94,   156,  -217,  -217,  -217,   185,   -50,  -217,
    -217,  -102,  -217,    -4,   -59,   333,  -217,  -217,  -217,  -110,
     181,    89,  -147,  -216,   204,  -217,    87,  -217,   128,  -217,
    -217,  -217,  -217,  -217
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    47,   326,   313,   178,    31,    32,
      33,    34,   228,   171,   257,   226,    35,   197,    97,    98,
     136,   137,    65,   122,    36,    37,   161,   162,    38,    83,
     119,    39,    40,    66,    67,   193,    68,    69,    70,   265,
     154,   266,   159,   210,   211,   290,   309,   310,   239,   271,
     302,    41,    42,    82
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      71,    92,   153,   155,   104,   138,   138,   194,   140,   105,
     195,   166,   243,   105,   214,    43,   169,    48,   213,    49,
      52,   120,   105,   167,    72,    91,    75,   315,    53,    54,
     102,   156,   276,   277,   144,    76,   244,    55,   145,   124,
     316,    77,    44,   158,    45,   248,   249,    93,   147,   148,
     149,   150,   173,   307,   288,   103,    78,   121,   157,   125,
      73,    74,   163,   164,   253,    91,   219,   192,    46,   298,
      50,    81,   174,   168,   176,   294,   104,    59,    60,   235,
      62,   254,   165,   255,   245,   256,   262,   128,    91,   184,
     106,   167,   185,   198,   106,   139,   141,   142,   143,   209,
     131,   132,   167,   106,   241,   107,   108,   109,   110,   107,
     108,   109,   110,   -82,    80,   282,   -82,   199,   284,   286,
      85,   167,   167,   230,   140,   200,   254,   267,   255,   220,
     256,    87,    91,   221,   222,   223,   224,   225,    84,    51,
      86,    52,   187,    94,    95,   185,    88,   191,    96,    53,
      54,    89,   250,   251,   107,   108,   109,   110,    55,   201,
     202,   203,   204,   205,   206,   207,   208,   -83,   133,   134,
     -83,   107,   108,   109,   110,   -84,   273,    90,   -84,   185,
     167,    99,   292,    56,    57,    58,   217,   218,   275,   209,
     209,   303,   335,   100,   185,   336,   109,   110,    59,    60,
      61,    62,   101,    63,    64,   112,   248,   249,   318,   319,
     113,   209,   114,   283,   115,   116,   123,   126,   117,   118,
     127,   129,   130,   151,   146,   152,   209,   158,    91,   160,
     170,   295,   172,   175,   342,   343,   198,   179,   186,   177,
     188,   308,   350,   351,   305,   180,   167,   167,   189,   190,
     196,    51,   212,    52,   167,   167,   215,   216,   308,   227,
     199,    53,    54,   229,   231,   287,   232,   233,   200,   234,
      55,   236,   237,    51,   238,    52,   240,   246,   242,   259,
      51,   261,    52,    53,    54,   260,   263,   264,   270,   269,
      53,    54,    55,   274,   272,    56,    57,    58,   279,    55,
     278,   121,   201,   202,   203,   204,   205,   206,   207,   208,
      59,    60,    61,    62,   289,    63,    64,    56,    57,    58,
     248,   293,   285,   296,    56,    57,    58,   300,   297,   301,
     311,   312,    59,    60,    61,    62,   314,    63,    64,    59,
      60,    61,    62,   306,   135,    64,    51,   317,    52,   322,
     323,   327,   324,   325,   328,   329,    53,    54,   330,   337,
       1,     2,   331,   334,   332,    55,   338,   339,   340,     3,
       4,     5,     6,     7,     8,     9,    10,   344,   345,   341,
     348,   346,    11,    12,    13,   347,   349,   352,   353,    79,
      56,    57,    58,   333,   280,   258,   291,   304,   252,    14,
     111,    15,   181,   320,   321,   182,   183,    61,    62,    16,
      63,    64,    17,   247,   299,   281,     0,     0,   268
};

static const yytype_int16 yycheck[] =
{
       4,    51,   112,   113,    63,    99,   100,   154,   100,     4,
     156,   117,     9,     4,   161,    13,   118,    13,    28,    15,
      26,    24,     4,   117,    85,    17,    50,     5,    34,    35,
      24,    24,   248,   249,    85,    52,    33,    43,    89,     4,
      18,    85,    40,    53,    42,    54,    55,    51,   107,   108,
     109,   110,     4,    62,   270,    49,    85,    60,    51,    24,
      14,    15,    68,    69,    24,    17,   168,    62,    66,   285,
      66,     3,   122,    65,   124,     4,   135,    83,    84,   185,
      86,    41,    88,    43,    81,    45,   232,    91,    17,    25,
      85,   185,    28,     9,    85,    99,   100,   101,   102,   158,
      83,    84,   196,    85,   196,    87,    88,    89,    90,    87,
      88,    89,    90,    25,     0,   261,    28,    33,   264,   266,
      15,   215,   216,   173,   216,    41,    41,   237,    43,    32,
      45,    15,    17,    36,    37,    38,    39,    40,    85,    24,
      85,    26,    25,    83,    84,    28,    85,   151,    88,    34,
      35,    85,   211,   212,    87,    88,    89,    90,    43,    75,
      76,    77,    78,    79,    80,    81,    82,    25,    27,    28,
      28,    87,    88,    89,    90,    25,    25,    85,    28,    28,
     274,    24,   274,    68,    69,    70,    83,    84,   247,   248,
     249,    25,    25,    24,    28,    28,    89,    90,    83,    84,
      85,    86,    24,    88,    89,    52,    54,    55,    70,    71,
      52,   270,    85,   263,    85,    56,    85,    85,    75,    64,
      57,    25,    25,    28,    85,    85,   285,    53,    17,    85,
      85,   281,    61,    57,   340,   341,     9,    57,    25,    85,
      25,   300,   348,   349,   294,    85,   340,   341,    25,    25,
      24,    24,    75,    26,   348,   349,    24,    24,   317,    28,
      33,    34,    35,    75,    85,   269,    28,    25,    41,    85,
      43,    63,    28,    24,    10,    26,    25,    41,    28,    25,
      24,    24,    26,    34,    35,    85,     4,    24,    11,     6,
      34,    35,    43,    24,    51,    68,    69,    70,    43,    43,
      83,    60,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    12,    88,    89,    68,    69,    70,
      54,    25,    57,    25,    68,    69,    70,     6,    25,    44,
      83,    24,    83,    84,    85,    86,    63,    88,    89,    83,
      84,    85,    86,    67,    88,    89,    24,    28,    26,    75,
      75,    28,    85,    74,    28,    71,    34,    35,    70,    28,
       7,     8,    75,    85,    75,    43,    72,    72,    75,    16,
      17,    18,    19,    20,    21,    22,    23,    28,    28,    75,
      75,    73,    29,    30,    31,    73,    75,    25,    25,    17,
      68,    69,    70,   331,   258,   227,   272,   293,   213,    46,
      67,    48,   134,   314,   317,    83,    84,    85,    86,    56,
      88,    89,    59,   209,   286,   259,    -1,    -1,   237
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     7,     8,    16,    17,    18,    19,    20,    21,    22,
      23,    29,    30,    31,    46,    48,    56,    59,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   110,   111,   112,   113,   118,   126,   127,   130,   133,
     134,   153,   154,    13,    40,    42,    66,   106,    13,    15,
      66,    24,    26,    34,    35,    43,    68,    69,    70,    83,
      84,    85,    86,    88,    89,   124,   135,   136,   138,   139,
     140,   135,    85,    14,    15,    50,    52,    85,    85,    94,
       0,     3,   155,   131,    85,    15,    85,    15,    85,    85,
      85,    17,   130,   135,    83,    84,    88,   120,   121,    24,
      24,    24,    24,    49,   136,     4,    85,    87,    88,    89,
      90,   137,    52,    52,    85,    85,    56,    75,    64,   132,
      24,    60,   125,    85,     4,    24,    85,    57,   135,    25,
      25,    83,    84,    27,    28,    88,   122,   123,   124,   135,
     122,   135,   135,   135,    85,    89,    85,   136,   136,   136,
     136,    28,    85,   141,   142,   141,    24,    51,    53,   144,
      85,   128,   129,    68,    69,    88,   123,   124,    65,   133,
      85,   115,    61,     4,   130,    57,   130,    85,   109,    57,
      85,   120,    83,    84,    25,    28,    25,    25,    25,    25,
      25,   135,    62,   137,   144,   109,    24,   119,     9,    33,
      41,    75,    76,    77,    78,    79,    80,    81,    82,   136,
     145,   146,    75,    28,   144,    24,    24,    83,    84,   133,
      32,    36,    37,    38,    39,    40,   117,    28,   114,    75,
     130,    85,    28,    25,    85,   123,    63,    28,    10,   150,
      25,   122,    28,     9,    33,    81,    41,   146,    54,    55,
     136,   136,   129,    24,    41,    43,    45,   116,   115,    25,
      85,    24,   109,     4,    24,   141,   143,   141,   142,     6,
      11,   151,    51,    25,    24,   136,   145,   145,    83,    43,
     114,   125,   109,   130,   109,    57,   144,   135,   145,    12,
     147,   119,   122,    25,     4,   130,    25,    25,   145,   150,
       6,    44,   152,    25,   116,   130,    67,    62,   136,   148,
     149,    83,    24,   108,    63,     5,    18,    28,    70,    71,
     143,   148,    75,    75,    85,    74,   107,    28,    28,    71,
      70,    75,    75,   107,    85,    25,    28,    28,    72,    72,
      75,    75,   123,   123,    28,    28,    73,    73,    75,    75,
     123,   123,    25,    25
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    92,    93,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   105,   106,   106,   107,
     108,   108,   108,   109,   109,   110,   111,   111,   111,   111,
     111,   112,   112,   113,   114,   114,   115,   115,   116,   116,
     116,   116,   117,   117,   117,   117,   117,   117,   118,   118,
     119,   119,   120,   120,   120,   120,   121,   121,   121,   122,
     122,   122,   123,   123,   123,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   125,   125,   126,   127,   128,   128,
     129,   130,   131,   131,   132,   132,   133,   133,   134,   134,
     135,   135,   135,   136,   136,   136,   136,   136,   136,   136,
     136,   136,   136,   136,   136,   137,   137,   137,   138,   138,
     138,   138,   139,   140,   140,   141,   142,   142,   143,   143,
     144,   144,   145,   145,   145,   145,   146,   146,   146,   146,
     146,   146,   146,   146,   146,   146,   146,   146,   146,   146,
     147,   147,   148,   148,   149,   149,   149,   150,   150,   151,
     151,   152,   152,   153,   154,   155
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     2,     2,     4,     9,    11,     1,     0,     1,
       9,    17,    17,     1,     3,     5,    10,     9,     8,     6,
       5,     5,     8,     3,     0,     3,     6,     3,     2,     1,
       1,     0,     1,     1,     1,     1,     1,     1,     5,     8,
       3,     5,     1,     2,     1,     2,     0,     1,     3,     0,
       1,     3,     1,     2,     2,     1,     1,     1,     1,     1,
       1,     3,     4,     4,     0,     4,     4,     5,     1,     3,
       3,     2,     0,     2,     2,     3,     9,     9,     2,     2,
       0,     2,     4,     3,     3,     3,     3,     3,     2,     1,
       1,     1,     3,     1,     1,     0,     1,     2,     4,     4,
       4,     4,     3,     1,     3,     1,     2,     4,     3,     6,
       0,     2,     3,     2,     3,     3,     1,     1,     1,     1,
       1,     1,     1,     2,     1,     2,     1,     2,     1,     2,
       0,     3,     1,     3,     1,     2,     2,     0,     3,     0,
       2,     0,     2,     2,     4,     1
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
#line 292 "yacc_sql.y"
  {
    std::unique_ptr<ParsedSqlNode> sql_node = std::unique_ptr<ParsedSqlNode>((yyvsp[-1].sql_node));
    sql_result->add_sql_node(std::move(sql_node));
  }
#line 1972 "yacc_sql.cpp"
    break;

  case 25: /* exit_stmt: EXIT  */
#line 324 "yacc_sql.y"
         {
      (void)yynerrs;  // 这么写为了消除yynerrs未使用的告警。如果你有更好的方法欢迎提PR
      (yyval.sql_node) = new ParsedSqlNode(SCF_EXIT);
    }
#line 1981 "yacc_sql.cpp"
    break;

  case 26: /* help_stmt: HELP  */
#line 330 "yacc_sql.y"
         {
      (yyval.sql_node) = new ParsedSqlNode(SCF_HELP);
    }
#line 1989 "yacc_sql.cpp"
    break;

  case 27: /* sync_stmt: SYNC  */
#line 335 "yacc_sql.y"
         {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SYNC);
    }
#line 1997 "yacc_sql.cpp"
    break;

  case 28: /* begin_stmt: TRX_BEGIN  */
#line 341 "yacc_sql.y"
               {
      (yyval.sql_node) = new ParsedSqlNode(SCF_BEGIN);
    }
#line 2005 "yacc_sql.cpp"
    break;

  case 29: /* commit_stmt: TRX_COMMIT  */
#line 347 "yacc_sql.y"
               {
      (yyval.sql_node) = new ParsedSqlNode(SCF_COMMIT);
    }
#line 2013 "yacc_sql.cpp"
    break;

  case 30: /* rollback_stmt: TRX_ROLLBACK  */
#line 353 "yacc_sql.y"
                  {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ROLLBACK);
    }
#line 2021 "yacc_sql.cpp"
    break;

  case 31: /* drop_table_stmt: DROP TABLE ID  */
#line 359 "yacc_sql.y"
                  {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_TABLE);
      (yyval.sql_node)->drop_table.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2031 "yacc_sql.cpp"
    break;

  case 32: /* show_tables_stmt: SHOW TABLES  */
#line 366 "yacc_sql.y"
                {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SHOW_TABLES);
    }
#line 2039 "yacc_sql.cpp"
    break;

  case 33: /* desc_table_stmt: DESC ID  */
#line 372 "yacc_sql.y"
             {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DESC_TABLE);
      (yyval.sql_node)->desc_table.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2049 "yacc_sql.cpp"
    break;

  case 34: /* show_index_stmt: SHOW INDEX FROM relation  */
#line 381 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SHOW_INDEX);
      ShowIndexSqlNode &show_index = (yyval.sql_node)->show_index;
      show_index.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2060 "yacc_sql.cpp"
    break;

  case 35: /* create_index_stmt: CREATE opt_unique INDEX ID ON ID LBRACE attr_list RBRACE  */
#line 391 "yacc_sql.y"
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
#line 2076 "yacc_sql.cpp"
    break;

  case 36: /* create_index_stmt: CREATE VECTOR_T INDEX ID ON ID LBRACE attr_list RBRACE WITH vector_index_config  */
#line 403 "yacc_sql.y"
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
#line 2093 "yacc_sql.cpp"
    break;

  case 37: /* opt_unique: UNIQUE  */
#line 418 "yacc_sql.y"
           { (yyval.unique) = true; }
#line 2099 "yacc_sql.cpp"
    break;

  case 38: /* opt_unique: %empty  */
#line 419 "yacc_sql.y"
                { (yyval.unique) = false; }
#line 2105 "yacc_sql.cpp"
    break;

  case 39: /* index_type: IVFFLAT  */
#line 424 "yacc_sql.y"
    {
      (yyval.index_type) = IndexType::VectorIVFFlatIndex;
    }
#line 2113 "yacc_sql.cpp"
    break;

  case 40: /* vector_index_config: LBRACE DISTANCE EQ ID COMMA TYPE EQ index_type RBRACE  */
#line 431 "yacc_sql.y"
    {
      (yyval.vector_index_config) = new VectorIndexConfig;
      (yyval.vector_index_config)->distance_fn = (yyvsp[-5].string);
      (yyval.vector_index_config)->index_type = (yyvsp[-1].index_type);
      free((yyvsp[-5].string));
    }
#line 2124 "yacc_sql.cpp"
    break;

  case 41: /* vector_index_config: LBRACE DISTANCE EQ ID COMMA TYPE EQ index_type COMMA LISTS EQ value COMMA PROBES EQ value RBRACE  */
#line 438 "yacc_sql.y"
    {
      (yyval.vector_index_config) = new VectorIndexConfig;
      (yyval.vector_index_config)->distance_fn = (yyvsp[-13].string);
      (yyval.vector_index_config)->index_type = (yyvsp[-9].index_type);
      (yyval.vector_index_config)->lists = std::move(*(yyvsp[-5].value));
      (yyval.vector_index_config)->probes = std::move(*(yyvsp[-1].value));
      free((yyvsp[-13].string));
    }
#line 2137 "yacc_sql.cpp"
    break;

  case 42: /* vector_index_config: LBRACE TYPE EQ index_type COMMA DISTANCE EQ ID COMMA LISTS EQ value COMMA PROBES EQ value RBRACE  */
#line 447 "yacc_sql.y"
    {
      (yyval.vector_index_config) = new VectorIndexConfig;
      (yyval.vector_index_config)->distance_fn = (yyvsp[-9].string);
      (yyval.vector_index_config)->index_type = (yyvsp[-13].index_type);
      (yyval.vector_index_config)->lists = std::move(*(yyvsp[-5].value));
      (yyval.vector_index_config)->probes = std::move(*(yyvsp[-1].value));
      free((yyvsp[-9].string));
    }
#line 2150 "yacc_sql.cpp"
    break;

  case 43: /* attr_list: ID  */
#line 459 "yacc_sql.y"
    {
      (yyval.index_attr_list) = new std::vector<std::string>; // 创建一个新的 vector
      (yyval.index_attr_list)->emplace_back((yyvsp[0].string)); // 将列名加入 vector
      free((yyvsp[0].string));
    }
#line 2160 "yacc_sql.cpp"
    break;

  case 44: /* attr_list: ID COMMA attr_list  */
#line 465 "yacc_sql.y"
    {
      (yyval.index_attr_list) = (yyvsp[0].index_attr_list); // 使用现有的 vector
      (yyval.index_attr_list)->emplace((yyval.index_attr_list)->begin(), (yyvsp[-2].string)); // 将新列名加入 vector 开头
      free((yyvsp[-2].string));
    }
#line 2170 "yacc_sql.cpp"
    break;

  case 45: /* drop_index_stmt: DROP INDEX ID ON ID  */
#line 474 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_INDEX);
      (yyval.sql_node)->drop_index.index_name = (yyvsp[-2].string);
      (yyval.sql_node)->drop_index.relation_name = (yyvsp[0].string);
      free((yyvsp[-2].string));
      free((yyvsp[0].string));
    }
#line 2182 "yacc_sql.cpp"
    break;

  case 46: /* create_table_stmt: CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE storage_format AS select_stmt  */
#line 484 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-7].string), (yyvsp[-5].attr_info), (yyvsp[-4].attr_infos), (yyvsp[-2].string), (yyvsp[0].sql_node));
    }
#line 2190 "yacc_sql.cpp"
    break;

  case 47: /* create_table_stmt: CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE storage_format select_stmt  */
#line 488 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-6].string), (yyvsp[-4].attr_info), (yyvsp[-3].attr_infos), (yyvsp[-1].string), (yyvsp[0].sql_node));
    }
#line 2198 "yacc_sql.cpp"
    break;

  case 48: /* create_table_stmt: CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE storage_format  */
#line 492 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-5].string), (yyvsp[-3].attr_info), (yyvsp[-2].attr_infos), (yyvsp[0].string), nullptr);
    }
#line 2206 "yacc_sql.cpp"
    break;

  case 49: /* create_table_stmt: CREATE TABLE ID storage_format AS select_stmt  */
#line 496 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-3].string), nullptr, nullptr, (yyvsp[-2].string), (yyvsp[0].sql_node));
    }
#line 2214 "yacc_sql.cpp"
    break;

  case 50: /* create_table_stmt: CREATE TABLE ID storage_format select_stmt  */
#line 500 "yacc_sql.y"
    {
      (yyval.sql_node) = create_table_sql_node((yyvsp[-2].string), nullptr, nullptr, (yyvsp[-1].string), (yyvsp[0].sql_node));
    }
#line 2222 "yacc_sql.cpp"
    break;

  case 51: /* create_view_stmt: CREATE VIEW ID AS select_stmt  */
#line 507 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CREATE_VIEW);
      CreateViewSqlNode &create_view = (yyval.sql_node)->create_view;
      create_view.relation_name = (yyvsp[-2].string);
      create_view.create_view_select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      free((yyvsp[-2].string));
    }
#line 2234 "yacc_sql.cpp"
    break;

  case 52: /* create_view_stmt: CREATE VIEW ID LBRACE attr_list RBRACE AS select_stmt  */
#line 515 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CREATE_VIEW);
      CreateViewSqlNode &create_view = (yyval.sql_node)->create_view;
      create_view.relation_name = (yyvsp[-5].string);
      create_view.attribute_names = std::move(*(yyvsp[-3].index_attr_list));
      create_view.create_view_select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      free((yyvsp[-5].string));
    }
#line 2247 "yacc_sql.cpp"
    break;

  case 53: /* drop_view_stmt: DROP VIEW ID  */
#line 527 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_VIEW);
      (yyval.sql_node)->drop_view.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2257 "yacc_sql.cpp"
    break;

  case 54: /* attr_def_list: %empty  */
#line 536 "yacc_sql.y"
    {
      (yyval.attr_infos) = nullptr;
    }
#line 2265 "yacc_sql.cpp"
    break;

  case 55: /* attr_def_list: COMMA attr_def attr_def_list  */
#line 540 "yacc_sql.y"
    {
      if ((yyvsp[0].attr_infos) != nullptr) {
        (yyval.attr_infos) = (yyvsp[0].attr_infos);
      } else {
        (yyval.attr_infos) = new std::vector<AttrInfoSqlNode>;
      }
      (yyval.attr_infos)->emplace_back(*(yyvsp[-1].attr_info));
      delete (yyvsp[-1].attr_info);
    }
#line 2279 "yacc_sql.cpp"
    break;

  case 56: /* attr_def: ID type LBRACE NUMBER RBRACE nullable_constraint  */
#line 553 "yacc_sql.y"
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
#line 2301 "yacc_sql.cpp"
    break;

  case 57: /* attr_def: ID type nullable_constraint  */
#line 571 "yacc_sql.y"
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
#line 2331 "yacc_sql.cpp"
    break;

  case 58: /* nullable_constraint: NOT NULL_T  */
#line 600 "yacc_sql.y"
    {
      (yyval.nullable_info) = false;  // NOT NULL 对应的可空性为 false
    }
#line 2339 "yacc_sql.cpp"
    break;

  case 59: /* nullable_constraint: NULLABLE  */
#line 604 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;  // NULLABLE 对应的可空性为 true 2022
    }
#line 2347 "yacc_sql.cpp"
    break;

  case 60: /* nullable_constraint: NULL_T  */
#line 608 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;  // NULL 对应的可空性也为 true 2023
    }
#line 2355 "yacc_sql.cpp"
    break;

  case 61: /* nullable_constraint: %empty  */
#line 612 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;  // 默认情况为 NULL
    }
#line 2363 "yacc_sql.cpp"
    break;

  case 62: /* type: INT_T  */
#line 618 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::INTS);   }
#line 2369 "yacc_sql.cpp"
    break;

  case 63: /* type: STRING_T  */
#line 619 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::CHARS);  }
#line 2375 "yacc_sql.cpp"
    break;

  case 64: /* type: FLOAT_T  */
#line 620 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::FLOATS); }
#line 2381 "yacc_sql.cpp"
    break;

  case 65: /* type: DATE_T  */
#line 621 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::DATES);  }
#line 2387 "yacc_sql.cpp"
    break;

  case 66: /* type: TEXT_T  */
#line 622 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::TEXTS);  }
#line 2393 "yacc_sql.cpp"
    break;

  case 67: /* type: VECTOR_T  */
#line 623 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::VECTORS);  }
#line 2399 "yacc_sql.cpp"
    break;

  case 68: /* insert_stmt: INSERT INTO ID VALUES values_list  */
#line 628 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_INSERT);
      (yyval.sql_node)->insertion.relation_name = (yyvsp[-2].string);
      if ((yyvsp[0].values_list) != nullptr) {
        (yyval.sql_node)->insertion.values_list.swap(*(yyvsp[0].values_list));
        delete (yyvsp[0].values_list);
      }
      free((yyvsp[-2].string));
    }
#line 2413 "yacc_sql.cpp"
    break;

  case 69: /* insert_stmt: INSERT INTO ID LBRACE attr_list RBRACE VALUES values_list  */
#line 638 "yacc_sql.y"
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
#line 2428 "yacc_sql.cpp"
    break;

  case 70: /* values_list: LBRACE value_list RBRACE  */
#line 652 "yacc_sql.y"
    {
      (yyval.values_list) = new std::vector<std::vector<Value>>;
      (yyval.values_list)->emplace_back(*(yyvsp[-1].value_list));
      delete (yyvsp[-1].value_list);
    }
#line 2438 "yacc_sql.cpp"
    break;

  case 71: /* values_list: values_list COMMA LBRACE value_list RBRACE  */
#line 658 "yacc_sql.y"
    {
      (yyval.values_list)->emplace_back(*(yyvsp[-1].value_list));
      delete (yyvsp[-1].value_list);
    }
#line 2447 "yacc_sql.cpp"
    break;

  case 72: /* digits: NUMBER  */
#line 665 "yacc_sql.y"
    {
      (yyval.digits) = float((yyvsp[0].number));
    }
#line 2455 "yacc_sql.cpp"
    break;

  case 73: /* digits: '-' NUMBER  */
#line 669 "yacc_sql.y"
    {
      (yyval.digits) = float(-(yyvsp[0].number));
    }
#line 2463 "yacc_sql.cpp"
    break;

  case 74: /* digits: FLOAT  */
#line 673 "yacc_sql.y"
    {
      (yyval.digits) = (yyvsp[0].floats);
    }
#line 2471 "yacc_sql.cpp"
    break;

  case 75: /* digits: '-' FLOAT  */
#line 677 "yacc_sql.y"
    {
      (yyval.digits) = (yyvsp[0].floats);
    }
#line 2479 "yacc_sql.cpp"
    break;

  case 76: /* digits_list: %empty  */
#line 684 "yacc_sql.y"
    {
      (yyval.digits_list) = new std::vector<float>();
    }
#line 2487 "yacc_sql.cpp"
    break;

  case 77: /* digits_list: digits  */
#line 688 "yacc_sql.y"
    {
      (yyval.digits_list) = new std::vector<float>();
      (yyval.digits_list)->push_back((yyvsp[0].digits));
    }
#line 2496 "yacc_sql.cpp"
    break;

  case 78: /* digits_list: digits_list COMMA digits  */
#line 693 "yacc_sql.y"
    {
      (yyval.digits_list)->push_back((yyvsp[0].digits));
    }
#line 2504 "yacc_sql.cpp"
    break;

  case 79: /* value_list: %empty  */
#line 700 "yacc_sql.y"
    {
      (yyval.value_list) = new std::vector<Value>;
    }
#line 2512 "yacc_sql.cpp"
    break;

  case 80: /* value_list: value  */
#line 704 "yacc_sql.y"
    {
      (yyval.value_list) = new std::vector<Value>;
      (yyval.value_list)->emplace_back(*(yyvsp[0].value));
      delete (yyvsp[0].value);
    }
#line 2522 "yacc_sql.cpp"
    break;

  case 81: /* value_list: value_list COMMA value  */
#line 710 "yacc_sql.y"
    {
      (yyval.value_list)->emplace_back(*(yyvsp[0].value));
      delete (yyvsp[0].value);
    }
#line 2531 "yacc_sql.cpp"
    break;

  case 82: /* value: nonnegative_value  */
#line 717 "yacc_sql.y"
                      {
      (yyval.value) = (yyvsp[0].value);
    }
#line 2539 "yacc_sql.cpp"
    break;

  case 83: /* value: '-' NUMBER  */
#line 720 "yacc_sql.y"
                 {
      (yyval.value) = new Value(-(yyvsp[0].number));
      (yyloc) = (yylsp[-1]);
    }
#line 2548 "yacc_sql.cpp"
    break;

  case 84: /* value: '-' FLOAT  */
#line 724 "yacc_sql.y"
                {
      (yyval.value) = new Value(-(yyvsp[0].floats));
      (yyloc) = (yylsp[-1]);
    }
#line 2557 "yacc_sql.cpp"
    break;

  case 85: /* nonnegative_value: NUMBER  */
#line 731 "yacc_sql.y"
           {
      (yyval.value) = new Value((yyvsp[0].number));
      (yyloc) = (yylsp[0]);
    }
#line 2566 "yacc_sql.cpp"
    break;

  case 86: /* nonnegative_value: FLOAT  */
#line 735 "yacc_sql.y"
            {
      (yyval.value) = new Value((yyvsp[0].floats));
      (yyloc) = (yylsp[0]);
    }
#line 2575 "yacc_sql.cpp"
    break;

  case 87: /* nonnegative_value: SSS  */
#line 739 "yacc_sql.y"
          {
      char *tmp = common::substr((yyvsp[0].string),1,strlen((yyvsp[0].string))-2);
      (yyval.value) = new Value(tmp);
      free(tmp);
      free((yyvsp[0].string));
    }
#line 2586 "yacc_sql.cpp"
    break;

  case 88: /* nonnegative_value: TRUE  */
#line 745 "yacc_sql.y"
           {
      (yyval.value) = new Value(true);
    }
#line 2594 "yacc_sql.cpp"
    break;

  case 89: /* nonnegative_value: FALSE  */
#line 748 "yacc_sql.y"
            {
      (yyval.value) = new Value(false);
    }
#line 2602 "yacc_sql.cpp"
    break;

  case 90: /* nonnegative_value: NULL_T  */
#line 751 "yacc_sql.y"
             {
      (yyval.value) = new Value(NullValue());
    }
#line 2610 "yacc_sql.cpp"
    break;

  case 91: /* nonnegative_value: LSBRACE digits_list RSBRACE  */
#line 754 "yacc_sql.y"
                                  {
      (yyval.value) = new Value(*(yyvsp[-1].digits_list));
    }
#line 2618 "yacc_sql.cpp"
    break;

  case 92: /* nonnegative_value: STRING_TO_VECTOR LBRACE value_list RBRACE  */
#line 757 "yacc_sql.y"
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
#line 2640 "yacc_sql.cpp"
    break;

  case 93: /* nonnegative_value: VECTOR_TO_STRING LBRACE value_list RBRACE  */
#line 774 "yacc_sql.y"
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
#line 2662 "yacc_sql.cpp"
    break;

  case 94: /* storage_format: %empty  */
#line 795 "yacc_sql.y"
    {
      (yyval.string) = nullptr;
    }
#line 2670 "yacc_sql.cpp"
    break;

  case 95: /* storage_format: STORAGE FORMAT EQ ID  */
#line 799 "yacc_sql.y"
    {
      (yyval.string) = (yyvsp[0].string);
    }
#line 2678 "yacc_sql.cpp"
    break;

  case 96: /* delete_stmt: DELETE FROM ID where  */
#line 806 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DELETE);
      (yyval.sql_node)->deletion.relation_name = (yyvsp[-1].string);
      if ((yyvsp[0].expression) != nullptr) {
        (yyval.sql_node)->deletion.condition = std::unique_ptr<Expression>((yyvsp[0].expression));
      }
      free((yyvsp[-1].string));
    }
#line 2691 "yacc_sql.cpp"
    break;

  case 97: /* update_stmt: UPDATE ID SET set_clauses where  */
#line 818 "yacc_sql.y"
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
#line 2706 "yacc_sql.cpp"
    break;

  case 98: /* set_clauses: set_clause  */
#line 832 "yacc_sql.y"
    {
      (yyval.set_clauses) = new std::vector<SetClauseSqlNode>;
      (yyval.set_clauses)->emplace_back(std::move(*(yyvsp[0].set_clause)));
    }
#line 2715 "yacc_sql.cpp"
    break;

  case 99: /* set_clauses: set_clauses COMMA set_clause  */
#line 837 "yacc_sql.y"
    {
      (yyval.set_clauses)->emplace_back(std::move(*(yyvsp[0].set_clause)));
    }
#line 2723 "yacc_sql.cpp"
    break;

  case 100: /* set_clause: ID EQ expression  */
#line 844 "yacc_sql.y"
    {
      (yyval.set_clause) = new SetClauseSqlNode;
      (yyval.set_clause)->field_name = (yyvsp[-2].string);
      (yyval.set_clause)->value = std::unique_ptr<Expression>((yyvsp[0].expression));
      free((yyvsp[-2].string));
    }
#line 2734 "yacc_sql.cpp"
    break;

  case 101: /* select_stmt: select_core select_union_list  */
#line 854 "yacc_sql.y"
    {
      (yyval.sql_node) = (yyvsp[-1].sql_node);
      if ((yyvsp[0].set_operator_list) != nullptr) {
        (yyval.sql_node)->selection.set_operations.swap(*(yyvsp[0].set_operator_list));
        delete (yyvsp[0].set_operator_list);
      }
    }
#line 2746 "yacc_sql.cpp"
    break;

  case 102: /* select_union_list: %empty  */
#line 865 "yacc_sql.y"
    {
      (yyval.set_operator_list) = nullptr;
    }
#line 2754 "yacc_sql.cpp"
    break;

  case 103: /* select_union_list: select_union_list select_union_item  */
#line 869 "yacc_sql.y"
    {
      if ((yyvsp[-1].set_operator_list) != nullptr) {
        (yyval.set_operator_list) = (yyvsp[-1].set_operator_list);
      } else {
        (yyval.set_operator_list) = new std::vector<SetOperatorSqlNode>();
      }
      (yyval.set_operator_list)->emplace_back(std::move(*(yyvsp[0].set_operator_node)));
      delete (yyvsp[0].set_operator_node);
    }
#line 2768 "yacc_sql.cpp"
    break;

  case 104: /* select_union_item: UNION select_core  */
#line 882 "yacc_sql.y"
    {
      (yyval.set_operator_node) = new SetOperatorSqlNode;
      (yyval.set_operator_node)->union_all = false;
      (yyval.set_operator_node)->select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      delete (yyvsp[0].sql_node);
    }
#line 2779 "yacc_sql.cpp"
    break;

  case 105: /* select_union_item: UNION ALL select_core  */
#line 889 "yacc_sql.y"
    {
      (yyval.set_operator_node) = new SetOperatorSqlNode;
      (yyval.set_operator_node)->union_all = true;
      (yyval.set_operator_node)->select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      delete (yyvsp[0].sql_node);
    }
#line 2790 "yacc_sql.cpp"
    break;

  case 106: /* select_core: SELECT expression_list FROM rel_list where group_by opt_having opt_order_by opt_limit  */
#line 899 "yacc_sql.y"
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
#line 2832 "yacc_sql.cpp"
    break;

  case 107: /* select_core: SELECT expression_list FROM relation INNER JOIN join_clauses where group_by  */
#line 937 "yacc_sql.y"
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
#line 2866 "yacc_sql.cpp"
    break;

  case 108: /* calc_stmt: CALC expression_list  */
#line 970 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CALC);
      (yyval.sql_node)->calc.expressions.swap(*(yyvsp[0].expression_list));
      delete (yyvsp[0].expression_list);
    }
#line 2876 "yacc_sql.cpp"
    break;

  case 109: /* calc_stmt: SELECT expression_list  */
#line 976 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CALC);
      (yyval.sql_node)->calc.expressions.swap(*(yyvsp[0].expression_list));
      delete (yyvsp[0].expression_list);
    }
#line 2886 "yacc_sql.cpp"
    break;

  case 110: /* expression_list: %empty  */
#line 984 "yacc_sql.y"
                {
      (yyval.expression_list) = new std::vector<std::unique_ptr<Expression>>;
    }
#line 2894 "yacc_sql.cpp"
    break;

  case 111: /* expression_list: expression alias  */
#line 988 "yacc_sql.y"
    {
      (yyval.expression_list) = new std::vector<std::unique_ptr<Expression>>;
      if (nullptr != (yyvsp[0].string)) {
        (yyvsp[-1].expression)->set_alias((yyvsp[0].string));
      }
      (yyval.expression_list)->emplace_back((yyvsp[-1].expression));
      free((yyvsp[0].string));
    }
#line 2907 "yacc_sql.cpp"
    break;

  case 112: /* expression_list: expression alias COMMA expression_list  */
#line 997 "yacc_sql.y"
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
#line 2924 "yacc_sql.cpp"
    break;

  case 113: /* expression: expression '+' expression  */
#line 1012 "yacc_sql.y"
                              {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::ADD, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 2932 "yacc_sql.cpp"
    break;

  case 114: /* expression: expression '-' expression  */
#line 1015 "yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::SUB, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 2940 "yacc_sql.cpp"
    break;

  case 115: /* expression: expression '*' expression  */
#line 1018 "yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::MUL, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 2948 "yacc_sql.cpp"
    break;

  case 116: /* expression: expression '/' expression  */
#line 1021 "yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::DIV, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 2956 "yacc_sql.cpp"
    break;

  case 117: /* expression: LBRACE expression_list RBRACE  */
#line 1024 "yacc_sql.y"
                                    {
      if ((yyvsp[-1].expression_list)->size() == 1) {
        (yyval.expression) = (yyvsp[-1].expression_list)->front().get();
      } else {
        (yyval.expression) = new ListExpr(std::move(*(yyvsp[-1].expression_list)));
      }
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 2969 "yacc_sql.cpp"
    break;

  case 118: /* expression: '-' expression  */
#line 1032 "yacc_sql.y"
                                  {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::NEGATIVE, (yyvsp[0].expression), nullptr, sql_string, &(yyloc));
    }
#line 2977 "yacc_sql.cpp"
    break;

  case 119: /* expression: nonnegative_value  */
#line 1035 "yacc_sql.y"
                        {
      (yyval.expression) = new ValueExpr(*(yyvsp[0].value));
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
      delete (yyvsp[0].value);
    }
#line 2987 "yacc_sql.cpp"
    break;

  case 120: /* expression: rel_attr  */
#line 1040 "yacc_sql.y"
               {
      RelAttrSqlNode *node = (yyvsp[0].rel_attr);
      (yyval.expression) = new UnboundFieldExpr(node->relation_name, node->attribute_name);
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
      delete (yyvsp[0].rel_attr);
    }
#line 2998 "yacc_sql.cpp"
    break;

  case 121: /* expression: '*'  */
#line 1046 "yacc_sql.y"
          {
      (yyval.expression) = new StarExpr();
    }
#line 3006 "yacc_sql.cpp"
    break;

  case 122: /* expression: ID DOT '*'  */
#line 1049 "yacc_sql.y"
                 {
      (yyval.expression) = new StarExpr((yyvsp[-2].string));
    }
#line 3014 "yacc_sql.cpp"
    break;

  case 123: /* expression: func_expr  */
#line 1052 "yacc_sql.y"
                {
      (yyval.expression) = (yyvsp[0].expression);      // AggrFuncExpr
    }
#line 3022 "yacc_sql.cpp"
    break;

  case 124: /* expression: sub_query_expr  */
#line 1055 "yacc_sql.y"
                     {
      (yyval.expression) = (yyvsp[0].expression); // SubQueryExpr
    }
#line 3030 "yacc_sql.cpp"
    break;

  case 125: /* alias: %empty  */
#line 1062 "yacc_sql.y"
                {
      (yyval.string) = nullptr;
    }
#line 3038 "yacc_sql.cpp"
    break;

  case 126: /* alias: ID  */
#line 1065 "yacc_sql.y"
         {
      (yyval.string) = (yyvsp[0].string);
    }
#line 3046 "yacc_sql.cpp"
    break;

  case 127: /* alias: AS ID  */
#line 1068 "yacc_sql.y"
            {
      (yyval.string) = (yyvsp[0].string);
    }
#line 3054 "yacc_sql.cpp"
    break;

  case 128: /* func_expr: ID LBRACE expression_list RBRACE  */
#line 1074 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr((yyvsp[-3].string), std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3063 "yacc_sql.cpp"
    break;

  case 129: /* func_expr: DISTANCE LBRACE expression_list RBRACE  */
#line 1079 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr("distance", std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3072 "yacc_sql.cpp"
    break;

  case 130: /* func_expr: STRING_TO_VECTOR LBRACE expression_list RBRACE  */
#line 1084 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr("string_to_vector", std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3081 "yacc_sql.cpp"
    break;

  case 131: /* func_expr: VECTOR_TO_STRING LBRACE expression_list RBRACE  */
#line 1089 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr("vector_to_string", std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3090 "yacc_sql.cpp"
    break;

  case 132: /* sub_query_expr: LBRACE select_stmt RBRACE  */
#line 1097 "yacc_sql.y"
    {
      (yyval.expression) = new SubQueryExpr((yyvsp[-1].sql_node)->selection);
    }
#line 3098 "yacc_sql.cpp"
    break;

  case 133: /* rel_attr: ID  */
#line 1103 "yacc_sql.y"
       {
      (yyval.rel_attr) = new RelAttrSqlNode;
      (yyval.rel_attr)->attribute_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 3108 "yacc_sql.cpp"
    break;

  case 134: /* rel_attr: ID DOT ID  */
#line 1108 "yacc_sql.y"
                {
      (yyval.rel_attr) = new RelAttrSqlNode;
      (yyval.rel_attr)->relation_name  = (yyvsp[-2].string);
      (yyval.rel_attr)->attribute_name = (yyvsp[0].string);
      free((yyvsp[-2].string));
      free((yyvsp[0].string));
    }
#line 3120 "yacc_sql.cpp"
    break;

  case 135: /* relation: ID  */
#line 1118 "yacc_sql.y"
       {
      (yyval.string) = (yyvsp[0].string);
    }
#line 3128 "yacc_sql.cpp"
    break;

  case 136: /* rel_list: relation alias  */
#line 1124 "yacc_sql.y"
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
#line 3143 "yacc_sql.cpp"
    break;

  case 137: /* rel_list: relation alias COMMA rel_list  */
#line 1134 "yacc_sql.y"
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
#line 3162 "yacc_sql.cpp"
    break;

  case 138: /* join_clauses: relation ON condition  */
#line 1152 "yacc_sql.y"
    {
      (yyval.join_clauses) = new JoinSqlNode;
      (yyval.join_clauses)->relations.emplace_back((yyvsp[-2].string));
      (yyval.join_clauses)->conditions = std::unique_ptr<Expression>((yyvsp[0].expression));
      free((yyvsp[-2].string));
    }
#line 3173 "yacc_sql.cpp"
    break;

  case 139: /* join_clauses: relation ON condition INNER JOIN join_clauses  */
#line 1159 "yacc_sql.y"
    {
      (yyval.join_clauses) = (yyvsp[0].join_clauses);
      (yyval.join_clauses)->relations.emplace_back((yyvsp[-5].string));
      auto ptr = (yyval.join_clauses)->conditions.release();
      (yyval.join_clauses)->conditions = std::make_unique<ConjunctionExpr>(ConjunctionExpr::Type::AND, ptr, (yyvsp[-3].expression));
      free((yyvsp[-5].string));
    }
#line 3185 "yacc_sql.cpp"
    break;

  case 140: /* where: %empty  */
#line 1170 "yacc_sql.y"
    {
      (yyval.expression) = nullptr;
    }
#line 3193 "yacc_sql.cpp"
    break;

  case 141: /* where: WHERE condition  */
#line 1173 "yacc_sql.y"
                      {
      (yyval.expression) = (yyvsp[0].expression);  
    }
#line 3201 "yacc_sql.cpp"
    break;

  case 142: /* condition: expression comp_op expression  */
#line 1180 "yacc_sql.y"
    {
      (yyval.expression) = new ComparisonExpr((yyvsp[-1].comp), (yyvsp[-2].expression), (yyvsp[0].expression));
    }
#line 3209 "yacc_sql.cpp"
    break;

  case 143: /* condition: comp_op expression  */
#line 1184 "yacc_sql.y"
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
#line 3226 "yacc_sql.cpp"
    break;

  case 144: /* condition: condition AND condition  */
#line 1197 "yacc_sql.y"
    {
      (yyval.expression) = new ConjunctionExpr(ConjunctionExpr::Type::AND, (yyvsp[-2].expression), (yyvsp[0].expression));
    }
#line 3234 "yacc_sql.cpp"
    break;

  case 145: /* condition: condition OR condition  */
#line 1201 "yacc_sql.y"
    {
      (yyval.expression) = new ConjunctionExpr(ConjunctionExpr::Type::OR, (yyvsp[-2].expression), (yyvsp[0].expression));
    }
#line 3242 "yacc_sql.cpp"
    break;

  case 146: /* comp_op: EQ  */
#line 1207 "yacc_sql.y"
         { (yyval.comp) = EQUAL_TO; }
#line 3248 "yacc_sql.cpp"
    break;

  case 147: /* comp_op: LT  */
#line 1208 "yacc_sql.y"
         { (yyval.comp) = LESS_THAN; }
#line 3254 "yacc_sql.cpp"
    break;

  case 148: /* comp_op: GT  */
#line 1209 "yacc_sql.y"
         { (yyval.comp) = GREAT_THAN; }
#line 3260 "yacc_sql.cpp"
    break;

  case 149: /* comp_op: LE  */
#line 1210 "yacc_sql.y"
         { (yyval.comp) = LESS_EQUAL; }
#line 3266 "yacc_sql.cpp"
    break;

  case 150: /* comp_op: GE  */
#line 1211 "yacc_sql.y"
         { (yyval.comp) = GREAT_EQUAL; }
#line 3272 "yacc_sql.cpp"
    break;

  case 151: /* comp_op: NE  */
#line 1212 "yacc_sql.y"
         { (yyval.comp) = NOT_EQUAL; }
#line 3278 "yacc_sql.cpp"
    break;

  case 152: /* comp_op: IS  */
#line 1213 "yacc_sql.y"
         { (yyval.comp) = IS_OP; }
#line 3284 "yacc_sql.cpp"
    break;

  case 153: /* comp_op: IS NOT  */
#line 1214 "yacc_sql.y"
             { (yyval.comp) = IS_NOT_OP; }
#line 3290 "yacc_sql.cpp"
    break;

  case 154: /* comp_op: LIKE  */
#line 1215 "yacc_sql.y"
           { (yyval.comp) = LIKE_OP;}
#line 3296 "yacc_sql.cpp"
    break;

  case 155: /* comp_op: NOT LIKE  */
#line 1216 "yacc_sql.y"
               {(yyval.comp) = NOT_LIKE_OP;}
#line 3302 "yacc_sql.cpp"
    break;

  case 156: /* comp_op: IN  */
#line 1217 "yacc_sql.y"
         { (yyval.comp) = IN_OP; }
#line 3308 "yacc_sql.cpp"
    break;

  case 157: /* comp_op: NOT IN  */
#line 1218 "yacc_sql.y"
             { (yyval.comp) = NOT_IN_OP; }
#line 3314 "yacc_sql.cpp"
    break;

  case 158: /* comp_op: EXISTS  */
#line 1219 "yacc_sql.y"
             { (yyval.comp) = EXISTS_OP; }
#line 3320 "yacc_sql.cpp"
    break;

  case 159: /* comp_op: NOT EXISTS  */
#line 1220 "yacc_sql.y"
                 { (yyval.comp) = NOT_EXISTS_OP; }
#line 3326 "yacc_sql.cpp"
    break;

  case 160: /* opt_order_by: %empty  */
#line 1225 "yacc_sql.y"
    {
      (yyval.orderby_list) = nullptr;
    }
#line 3334 "yacc_sql.cpp"
    break;

  case 161: /* opt_order_by: ORDER BY sort_list  */
#line 1229 "yacc_sql.y"
    {
      (yyval.orderby_list) = (yyvsp[0].orderby_list);
      std::reverse((yyval.orderby_list)->begin(),(yyval.orderby_list)->end());
    }
#line 3343 "yacc_sql.cpp"
    break;

  case 162: /* sort_list: sort_unit  */
#line 1237 "yacc_sql.y"
        {
      (yyval.orderby_list) = new std::vector<OrderBySqlNode>;
      (yyval.orderby_list)->emplace_back(std::move(*(yyvsp[0].orderby_unit)));
	}
#line 3352 "yacc_sql.cpp"
    break;

  case 163: /* sort_list: sort_unit COMMA sort_list  */
#line 1242 "yacc_sql.y"
        {
      (yyvsp[0].orderby_list)->emplace_back(std::move(*(yyvsp[-2].orderby_unit)));
      (yyval.orderby_list) = (yyvsp[0].orderby_list);
	}
#line 3361 "yacc_sql.cpp"
    break;

  case 164: /* sort_unit: expression  */
#line 1250 "yacc_sql.y"
        {
      (yyval.orderby_unit) = new OrderBySqlNode();
      (yyval.orderby_unit)->expr = std::unique_ptr<Expression>((yyvsp[0].expression));
      (yyval.orderby_unit)->is_asc = true;
	}
#line 3371 "yacc_sql.cpp"
    break;

  case 165: /* sort_unit: expression DESC  */
#line 1256 "yacc_sql.y"
        {
      (yyval.orderby_unit) = new OrderBySqlNode();
      (yyval.orderby_unit)->expr = std::unique_ptr<Expression>((yyvsp[-1].expression));
      (yyval.orderby_unit)->is_asc = false;
	}
#line 3381 "yacc_sql.cpp"
    break;

  case 166: /* sort_unit: expression ASC  */
#line 1262 "yacc_sql.y"
        {
      (yyval.orderby_unit) = new OrderBySqlNode(); // 默认是升序
      (yyval.orderby_unit)->expr = std::unique_ptr<Expression>((yyvsp[-1].expression));
      (yyval.orderby_unit)->is_asc = true;
	}
#line 3391 "yacc_sql.cpp"
    break;

  case 167: /* group_by: %empty  */
#line 1271 "yacc_sql.y"
    {
      (yyval.expression_list) = nullptr;
    }
#line 3399 "yacc_sql.cpp"
    break;

  case 168: /* group_by: GROUP BY expression_list  */
#line 1275 "yacc_sql.y"
    {
      (yyval.expression_list) = (yyvsp[0].expression_list);
    }
#line 3407 "yacc_sql.cpp"
    break;

  case 169: /* opt_having: %empty  */
#line 1282 "yacc_sql.y"
    {
      (yyval.expression) = nullptr;
    }
#line 3415 "yacc_sql.cpp"
    break;

  case 170: /* opt_having: HAVING condition  */
#line 1286 "yacc_sql.y"
    {
      (yyval.expression) = (yyvsp[0].expression);
    }
#line 3423 "yacc_sql.cpp"
    break;

  case 171: /* opt_limit: %empty  */
#line 1293 "yacc_sql.y"
    {
      (yyval.limited_info) = nullptr;
    }
#line 3431 "yacc_sql.cpp"
    break;

  case 172: /* opt_limit: LIMIT NUMBER  */
#line 1297 "yacc_sql.y"
    {
      (yyval.limited_info) = new LimitSqlNode();
      (yyval.limited_info)->number = (yyvsp[0].number);
    }
#line 3440 "yacc_sql.cpp"
    break;

  case 173: /* explain_stmt: EXPLAIN command_wrapper  */
#line 1305 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_EXPLAIN);
      (yyval.sql_node)->explain.sql_node = std::unique_ptr<ParsedSqlNode>((yyvsp[0].sql_node));
    }
#line 3449 "yacc_sql.cpp"
    break;

  case 174: /* set_variable_stmt: SET ID EQ value  */
#line 1313 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SET_VARIABLE);
      (yyval.sql_node)->set_variable.name  = (yyvsp[-2].string);
      (yyval.sql_node)->set_variable.value = *(yyvsp[0].value);
      free((yyvsp[-2].string));
      delete (yyvsp[0].value);
    }
#line 3461 "yacc_sql.cpp"
    break;


#line 3465 "yacc_sql.cpp"

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

#line 1325 "yacc_sql.y"

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
