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
  YYSYMBOL_show_tables_stmt = 109,         /* show_tables_stmt  */
  YYSYMBOL_desc_table_stmt = 110,          /* desc_table_stmt  */
  YYSYMBOL_show_index_stmt = 111,          /* show_index_stmt  */
  YYSYMBOL_create_index_stmt = 112,        /* create_index_stmt  */
  YYSYMBOL_opt_unique = 113,               /* opt_unique  */
  YYSYMBOL_index_type = 114,               /* index_type  */
  YYSYMBOL_vector_index_config = 115,      /* vector_index_config  */
  YYSYMBOL_attr_list = 116,                /* attr_list  */
  YYSYMBOL_drop_index_stmt = 117,          /* drop_index_stmt  */
  YYSYMBOL_create_table_stmt = 118,        /* create_table_stmt  */
  YYSYMBOL_create_view_stmt = 119,         /* create_view_stmt  */
  YYSYMBOL_drop_view_stmt = 120,           /* drop_view_stmt  */
  YYSYMBOL_attr_def_list = 121,            /* attr_def_list  */
  YYSYMBOL_attr_def = 122,                 /* attr_def  */
  YYSYMBOL_nullable_constraint = 123,      /* nullable_constraint  */
  YYSYMBOL_type = 124,                     /* type  */
  YYSYMBOL_insert_stmt = 125,              /* insert_stmt  */
  YYSYMBOL_values_list = 126,              /* values_list  */
  YYSYMBOL_digits = 127,                   /* digits  */
  YYSYMBOL_digits_list = 128,              /* digits_list  */
  YYSYMBOL_value_list = 129,               /* value_list  */
  YYSYMBOL_value = 130,                    /* value  */
  YYSYMBOL_nonnegative_value = 131,        /* nonnegative_value  */
  YYSYMBOL_storage_format = 132,           /* storage_format  */
  YYSYMBOL_delete_stmt = 133,              /* delete_stmt  */
  YYSYMBOL_update_stmt = 134,              /* update_stmt  */
  YYSYMBOL_set_clauses = 135,              /* set_clauses  */
  YYSYMBOL_set_clause = 136,               /* set_clause  */
  YYSYMBOL_select_stmt = 137,              /* select_stmt  */
  YYSYMBOL_select_union_list = 138,        /* select_union_list  */
  YYSYMBOL_select_union_item = 139,        /* select_union_item  */
  YYSYMBOL_select_core = 140,              /* select_core  */
  YYSYMBOL_calc_stmt = 141,                /* calc_stmt  */
  YYSYMBOL_expression_list = 142,          /* expression_list  */
  YYSYMBOL_expression = 143,               /* expression  */
  YYSYMBOL_alias = 144,                    /* alias  */
  YYSYMBOL_func_expr = 145,                /* func_expr  */
  YYSYMBOL_sub_query_expr = 146,           /* sub_query_expr  */
  YYSYMBOL_rel_attr = 147,                 /* rel_attr  */
  YYSYMBOL_relation = 148,                 /* relation  */
  YYSYMBOL_rel_list = 149,                 /* rel_list  */
  YYSYMBOL_join_clauses = 150,             /* join_clauses  */
  YYSYMBOL_where = 151,                    /* where  */
  YYSYMBOL_condition = 152,                /* condition  */
  YYSYMBOL_comp_op = 153,                  /* comp_op  */
  YYSYMBOL_opt_order_by = 154,             /* opt_order_by  */
  YYSYMBOL_sort_list = 155,                /* sort_list  */
  YYSYMBOL_sort_unit = 156,                /* sort_unit  */
  YYSYMBOL_group_by = 157,                 /* group_by  */
  YYSYMBOL_opt_having = 158,               /* opt_having  */
  YYSYMBOL_opt_limit = 159,                /* opt_limit  */
  YYSYMBOL_explain_stmt = 160,             /* explain_stmt  */
  YYSYMBOL_set_variable_stmt = 161,        /* set_variable_stmt  */
  YYSYMBOL_opt_semicolon = 162             /* opt_semicolon  */
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
#define YYLAST   453

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  98
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  65
/* YYNRULES -- Number of rules.  */
#define YYNRULES  180
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  371

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
       0,   298,   298,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,   317,   318,   319,   320,   321,   322,
     323,   324,   325,   326,   327,   328,   332,   338,   343,   349,
     355,   361,   367,   374,   386,   396,   408,   421,   427,   435,
     445,   457,   473,   474,   478,   485,   492,   501,   513,   519,
     528,   538,   542,   546,   550,   554,   561,   569,   581,   591,
     594,   607,   625,   654,   658,   662,   667,   673,   674,   675,
     676,   677,   678,   682,   692,   706,   712,   719,   723,   727,
     731,   739,   742,   747,   755,   758,   764,   772,   775,   779,
     786,   790,   794,   800,   803,   806,   809,   812,   829,   850,
     853,   860,   872,   886,   891,   898,   908,   920,   923,   936,
     943,   953,   991,  1024,  1030,  1039,  1042,  1051,  1067,  1070,
    1073,  1076,  1079,  1087,  1090,  1095,  1101,  1104,  1107,  1110,
    1117,  1120,  1123,  1128,  1133,  1138,  1143,  1151,  1158,  1163,
    1173,  1179,  1189,  1206,  1213,  1225,  1228,  1234,  1238,  1251,
    1255,  1262,  1263,  1264,  1265,  1266,  1267,  1268,  1269,  1270,
    1271,  1272,  1273,  1274,  1275,  1280,  1283,  1291,  1296,  1304,
    1310,  1316,  1326,  1329,  1337,  1340,  1348,  1351,  1359,  1367,
    1378
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
  "drop_table_stmt", "alter_table_stmt", "show_tables_stmt",
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

#define YYPACT_NINF (-241)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-90)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     391,     0,     9,    42,   255,   255,   -60,    56,  -241,    19,
     -13,   -32,  -241,  -241,  -241,  -241,  -241,   -26,   391,    85,
      84,  -241,  -241,  -241,  -241,  -241,  -241,  -241,  -241,  -241,
    -241,  -241,  -241,  -241,  -241,  -241,  -241,  -241,  -241,  -241,
    -241,  -241,  -241,  -241,  -241,     2,    87,  -241,    22,    98,
      32,    34,    46,    58,   128,    62,  -241,  -241,  -241,    68,
     112,   126,  -241,  -241,     6,  -241,   255,  -241,  -241,  -241,
      25,  -241,  -241,  -241,   137,  -241,  -241,   139,   122,   123,
     156,   145,  -241,  -241,  -241,  -241,   154,     7,   125,    37,
     134,  -241,   168,  -241,     4,   255,   201,   202,  -241,  -241,
     -50,  -241,   159,   295,   295,   255,   255,   -11,  -241,   143,
    -241,   255,   255,   255,   255,   200,   144,   144,     3,   176,
     146,    17,    -2,  -241,   151,   169,    18,   178,   219,   157,
     180,   158,   227,   229,   232,   166,   137,  -241,  -241,  -241,
    -241,  -241,    62,   302,   119,  -241,   136,   233,   140,   239,
     240,   241,  -241,  -241,  -241,   110,   110,  -241,  -241,   255,
    -241,    12,   176,  -241,   157,   244,   226,  -241,   181,     5,
    -241,   247,   248,   118,  -241,  -241,   219,  -241,   101,   245,
     198,   219,  -241,   187,  -241,   249,   251,   192,  -241,   194,
     151,   195,   196,  -241,   141,   142,  -241,    17,  -241,  -241,
    -241,  -241,  -241,  -241,   222,   258,   279,   263,    17,   264,
    -241,  -241,     1,  -241,  -241,  -241,  -241,  -241,  -241,  -241,
     250,   102,   152,   255,   255,   146,  -241,    17,    17,  -241,
    -241,  -241,  -241,  -241,  -241,  -241,  -241,  -241,    48,   151,
     268,   209,  -241,   275,   157,   299,   277,  -241,  -241,   223,
    -241,  -241,   144,   144,   307,   311,   265,   147,   298,  -241,
    -241,  -241,  -241,   255,   226,   226,    65,    65,  -241,   246,
     284,  -241,  -241,  -241,   245,   271,  -241,   157,  -241,   219,
     157,  -241,   276,   176,    11,  -241,   255,   226,   324,   244,
    -241,    17,    65,  -241,   281,   314,  -241,  -241,    28,   315,
    -241,   323,   226,   279,  -241,   152,   346,   306,   264,   148,
      78,   219,  -241,   285,  -241,   -30,  -241,   255,   267,  -241,
    -241,  -241,  -241,   327,   291,    39,  -241,   328,  -241,   138,
    -241,   144,  -241,  -241,   255,   282,   283,  -241,  -241,   269,
     280,   332,  -241,   333,   296,   292,   290,   293,   280,   286,
     149,   340,  -241,   300,   303,   301,   304,    17,    17,   347,
     350,   305,   318,   309,   322,    17,    17,   374,   375,  -241,
    -241
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    43,     0,     0,   115,   115,     0,     0,    28,     0,
       0,     0,    29,    30,    31,    27,    26,     0,     0,     0,
       0,    25,    24,    18,    19,    20,    21,     9,    10,    11,
      12,    15,    13,    14,     8,    16,    17,     5,     7,     6,
       3,   107,     4,    22,    23,     0,     0,    42,     0,     0,
       0,     0,     0,     0,   115,    81,    93,    94,    95,     0,
       0,     0,    90,    91,   138,    92,     0,   126,   124,   113,
     130,   128,   129,   125,   114,    38,    37,     0,     0,     0,
       0,     0,   178,     1,   180,     2,   106,    99,     0,     0,
       0,    32,     0,    58,     0,   115,     0,     0,    77,    79,
       0,    82,     0,    84,    84,   115,   115,     0,   123,     0,
     131,     0,     0,     0,     0,   116,     0,     0,     0,   145,
       0,     0,     0,   108,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   137,   122,    78,
      80,    96,     0,     0,     0,    85,   124,     0,     0,     0,
       0,     0,   139,   127,   132,   118,   119,   120,   121,   115,
     140,   130,   145,    39,     0,     0,     0,   101,     0,   145,
     103,     0,     0,     0,   179,    87,     0,   109,     0,    59,
       0,     0,    55,     0,    56,    48,     0,     0,    50,     0,
       0,     0,     0,    83,    90,    91,    97,     0,   135,    98,
     136,   134,   133,   117,     0,   141,   172,     0,    84,    73,
     163,   161,     0,   151,   152,   153,   154,   155,   156,   159,
     157,     0,   146,     0,     0,     0,   102,    84,    84,    88,
      89,   110,    67,    68,    69,    70,    71,    72,    66,     0,
       0,     0,    54,     0,     0,     0,     0,    34,    33,     0,
      36,    86,     0,     0,     0,   174,     0,     0,     0,   164,
     162,   160,   158,     0,     0,     0,   148,   105,   104,     0,
       0,    65,    64,    62,    59,    99,   100,     0,    49,     0,
       0,    35,     0,   145,   130,   142,   115,     0,   165,     0,
      75,    84,   147,   149,   150,     0,    63,    60,    53,     0,
      57,     0,     0,   172,   173,   175,     0,   176,    74,     0,
      66,     0,    52,     0,    40,   143,   112,     0,     0,   111,
      76,    61,    51,     0,     0,   169,   166,   167,   177,     0,
      41,     0,   171,   170,     0,     0,     0,   144,   168,     0,
       0,     0,    44,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    45,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    46,
      47
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -241,  -241,   386,  -241,  -241,  -241,  -241,  -241,  -241,  -241,
    -241,  -241,  -241,  -241,  -241,  -241,    57,  -241,  -163,  -241,
    -241,  -241,  -241,   132,  -171,    97,  -241,  -241,   120,   266,
    -241,   -98,  -114,  -100,   153,  -241,  -241,  -241,   193,   -52,
    -241,  -241,  -109,  -241,    -5,   -61,   349,  -241,  -241,  -241,
    -108,   167,    90,  -152,  -240,   205,  -241,    88,  -241,   124,
    -241,  -241,  -241,  -241,  -241
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    49,   343,   330,   186,    33,
      34,    35,    36,   240,   179,   273,   238,    37,   209,   101,
     102,   144,   145,    68,   126,    38,    39,   169,   170,    40,
      86,   123,    41,    42,    69,    70,   205,    71,    72,    73,
     282,   162,   283,   167,   222,   223,   307,   326,   327,   255,
     288,   319,    43,    44,    85
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      74,   207,    96,   146,   146,   108,   148,   174,   161,   163,
     206,   259,   132,   177,    45,   109,   109,   226,    95,   248,
     133,   175,   181,    50,   293,   294,    51,   264,   265,   109,
     164,    75,   311,   106,   124,   324,   225,   260,    95,   139,
     140,   128,    79,    46,   332,    47,    55,   305,    95,    97,
     155,   156,   157,   158,    56,    57,    53,   165,   107,    80,
     333,   166,   315,    58,   129,    81,   176,   231,   274,    48,
     125,    76,    78,    77,   182,   269,   184,   204,    52,   134,
     152,   278,   108,   251,   153,    83,   261,    84,   171,   172,
     136,   135,   270,    87,   271,   103,   272,   175,   147,   149,
     150,   151,   110,   110,    88,   221,    62,    63,   175,    65,
     257,   173,   210,    89,   299,    90,   110,   301,   111,   112,
     113,   114,   270,    91,   271,    92,   272,   175,   175,   242,
     148,   303,   111,   112,   113,   114,   232,    93,   211,   104,
     233,   234,   235,   236,   237,   284,   212,   196,    95,    94,
     197,    98,    99,   105,   203,    54,   100,    55,   111,   112,
     113,   114,   266,   267,   -87,    56,    57,   -87,   199,   -88,
     -89,   197,   -88,   -89,    58,   290,   320,   352,   197,   197,
     353,   213,   214,   215,   216,   217,   218,   219,   220,   141,
     142,   175,   116,   309,   117,   111,   112,   113,   114,    59,
      60,    61,   292,   221,   221,   113,   114,   229,   230,   264,
     265,   335,   336,   118,   119,   120,   127,    62,    63,    64,
      65,   122,    66,    67,   121,   130,   221,   300,   131,   137,
     138,   159,   166,   180,   154,   160,   210,   168,   183,    95,
     187,   221,   178,   359,   360,   189,   312,   190,   185,   188,
     191,   367,   368,    54,   192,    55,   325,   175,   175,   322,
     224,   198,   211,    56,    57,   175,   175,   200,   201,   202,
     212,   208,    58,   325,   227,   228,   239,   241,   243,   245,
     244,   304,    54,   246,    55,   247,   249,   250,   252,   253,
     254,   256,    56,    57,   262,   258,   275,    59,    60,    61,
     276,    58,   277,   279,   280,   213,   214,   215,   216,   217,
     218,   219,   220,   286,   281,    62,    63,    64,    65,   289,
      66,    67,    54,   287,    55,   291,    59,    60,    61,    54,
     296,    55,    56,    57,   125,   295,   302,   306,   264,    56,
      57,    58,   310,   313,    62,    63,    64,    65,    58,    66,
      67,   314,   317,   318,   329,   323,   328,   331,   342,   334,
     341,   339,   340,   344,   345,   347,    59,    60,    61,   348,
     346,   354,   349,    59,    60,    61,   355,   351,   361,   356,
     357,   362,   363,   358,    62,    63,    64,    65,   365,   143,
      67,   194,   195,    64,    65,   364,    66,    67,     1,     2,
       3,   366,   369,   370,    82,   350,   297,   321,   193,   308,
       4,     5,     6,     7,     8,     9,    10,    11,   268,   115,
     285,   337,   338,    12,    13,    14,   263,   316,   298,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      15,     0,    16,     0,     0,     0,     0,     0,     0,     0,
      17,     0,     0,    18
};

static const yytype_int16 yycheck[] =
{
       5,   164,    54,   103,   104,    66,   104,   121,   116,   117,
     162,    10,     8,   122,    14,     4,     4,   169,    20,   190,
      16,   121,     4,    14,   264,   265,    17,    57,    58,     4,
      27,    91,     4,    27,    27,    65,    31,    36,    20,    89,
      90,     4,    55,    43,     5,    45,    29,   287,    20,    54,
     111,   112,   113,   114,    37,    38,    14,    54,    52,    91,
      21,    56,   302,    46,    27,    91,    68,   176,   239,    69,
      63,    15,    53,    17,   126,    27,   128,    65,    69,    75,
      91,   244,   143,   197,    95,     0,    85,     3,    71,    72,
      95,    87,    44,    91,    46,    27,    48,   197,   103,   104,
     105,   106,    91,    91,    17,   166,    89,    90,   208,    92,
     208,    94,    10,    91,   277,    17,    91,   280,    93,    94,
      95,    96,    44,    91,    46,    91,    48,   227,   228,   181,
     228,   283,    93,    94,    95,    96,    35,    91,    36,    27,
      39,    40,    41,    42,    43,   253,    44,    28,    20,    91,
      31,    89,    90,    27,   159,    27,    94,    29,    93,    94,
      95,    96,   223,   224,    28,    37,    38,    31,    28,    28,
      28,    31,    31,    31,    46,    28,    28,    28,    31,    31,
      31,    79,    80,    81,    82,    83,    84,    85,    86,    30,
      31,   291,    55,   291,    55,    93,    94,    95,    96,    71,
      72,    73,   263,   264,   265,    95,    96,    89,    90,    57,
      58,    73,    74,    91,    91,    59,    91,    89,    90,    91,
      92,    67,    94,    95,    79,    91,   287,   279,    60,    28,
      28,    31,    56,    64,    91,    91,    10,    91,    60,    20,
      60,   302,    91,   357,   358,    18,   298,    18,    91,    91,
      18,   365,   366,    27,    88,    29,   317,   357,   358,   311,
      79,    28,    36,    37,    38,   365,   366,    28,    28,    28,
      44,    27,    46,   334,    27,    27,    31,    79,    91,    28,
      31,   286,    27,    91,    29,    91,    91,    91,    66,    31,
      11,    28,    37,    38,    44,    31,    28,    71,    72,    73,
      91,    46,    27,     4,    27,    79,    80,    81,    82,    83,
      84,    85,    86,     6,    91,    89,    90,    91,    92,    54,
      94,    95,    27,    12,    29,    27,    71,    72,    73,    27,
      46,    29,    37,    38,    63,    89,    60,    13,    57,    37,
      38,    46,    28,    28,    89,    90,    91,    92,    46,    94,
      95,    28,     6,    47,    27,    70,    89,    66,    78,    31,
      91,    79,    79,    31,    31,    73,    71,    72,    73,    79,
      74,    31,    79,    71,    72,    73,    76,    91,    31,    76,
      79,    31,    77,    79,    89,    90,    91,    92,    79,    94,
      95,    89,    90,    91,    92,    77,    94,    95,     7,     8,
       9,    79,    28,    28,    18,   348,   274,   310,   142,   289,
      19,    20,    21,    22,    23,    24,    25,    26,   225,    70,
     253,   331,   334,    32,    33,    34,   221,   303,   275,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      49,    -1,    51,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      59,    -1,    -1,    62
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     7,     8,     9,    19,    20,    21,    22,    23,    24,
      25,    26,    32,    33,    34,    49,    51,    59,    62,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   117,   118,   119,   120,   125,   133,   134,
     137,   140,   141,   160,   161,    14,    43,    45,    69,   113,
      14,    17,    69,    14,    27,    29,    37,    38,    46,    71,
      72,    73,    89,    90,    91,    92,    94,    95,   131,   142,
     143,   145,   146,   147,   142,    91,    15,    17,    53,    55,
      91,    91,   100,     0,     3,   162,   138,    91,    17,    91,
      17,    91,    91,    91,    91,    20,   137,   142,    89,    90,
      94,   127,   128,    27,    27,    27,    27,    52,   143,     4,
      91,    93,    94,    95,    96,   144,    55,    55,    91,    91,
      59,    79,    67,   139,    27,    63,   132,    91,     4,    27,
      91,    60,     8,    16,    75,    87,   142,    28,    28,    89,
      90,    30,    31,    94,   129,   130,   131,   142,   129,   142,
     142,   142,    91,    95,    91,   143,   143,   143,   143,    31,
      91,   148,   149,   148,    27,    54,    56,   151,    91,   135,
     136,    71,    72,    94,   130,   131,    68,   140,    91,   122,
      64,     4,   137,    60,   137,    91,   116,    60,    91,    18,
      18,    18,    88,   127,    89,    90,    28,    31,    28,    28,
      28,    28,    28,   142,    65,   144,   151,   116,    27,   126,
      10,    36,    44,    79,    80,    81,    82,    83,    84,    85,
      86,   143,   152,   153,    79,    31,   151,    27,    27,    89,
      90,   140,    35,    39,    40,    41,    42,    43,   124,    31,
     121,    79,   137,    91,    31,    28,    91,    91,   122,    91,
      91,   130,    66,    31,    11,   157,    28,   129,    31,    10,
      36,    85,    44,   153,    57,    58,   143,   143,   136,    27,
      44,    46,    48,   123,   122,    28,    91,    27,   116,     4,
      27,    91,   148,   150,   148,   149,     6,    12,   158,    54,
      28,    27,   143,   152,   152,    89,    46,   121,   132,   116,
     137,   116,    60,   151,   142,   152,    13,   154,   126,   129,
      28,     4,   137,    28,    28,   152,   157,     6,    47,   159,
      28,   123,   137,    70,    65,   143,   155,   156,    89,    27,
     115,    66,     5,    21,    31,    73,    74,   150,   155,    79,
      79,    91,    78,   114,    31,    31,    74,    73,    79,    79,
     114,    91,    28,    31,    31,    76,    76,    79,    79,   130,
     130,    31,    31,    77,    77,    79,    79,   130,   130,    28,
      28
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    98,    99,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   108,   108,   108,   109,   110,   111,
     112,   112,   113,   113,   114,   115,   115,   115,   116,   116,
     117,   118,   118,   118,   118,   118,   119,   119,   120,   121,
     121,   122,   122,   123,   123,   123,   123,   124,   124,   124,
     124,   124,   124,   125,   125,   126,   126,   127,   127,   127,
     127,   128,   128,   128,   129,   129,   129,   130,   130,   130,
     131,   131,   131,   131,   131,   131,   131,   131,   131,   132,
     132,   133,   134,   135,   135,   136,   137,   138,   138,   139,
     139,   140,   140,   141,   141,   142,   142,   142,   143,   143,
     143,   143,   143,   143,   143,   143,   143,   143,   143,   143,
     144,   144,   144,   145,   145,   145,   145,   146,   147,   147,
     148,   149,   149,   150,   150,   151,   151,   152,   152,   152,
     152,   153,   153,   153,   153,   153,   153,   153,   153,   153,
     153,   153,   153,   153,   153,   154,   154,   155,   155,   156,
     156,   156,   157,   157,   158,   158,   159,   159,   160,   161,
     162
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     6,     6,     7,     6,     2,     2,     4,
       9,    11,     1,     0,     1,     9,    17,    17,     1,     3,
       5,    10,     9,     8,     6,     5,     5,     8,     3,     0,
       3,     6,     3,     2,     1,     1,     0,     1,     1,     1,
       1,     1,     1,     5,     8,     3,     5,     1,     2,     1,
       2,     0,     1,     3,     0,     1,     3,     1,     2,     2,
       1,     1,     1,     1,     1,     1,     3,     4,     4,     0,
       4,     4,     5,     1,     3,     3,     2,     0,     2,     2,
       3,     9,     9,     2,     2,     0,     2,     4,     3,     3,
       3,     3,     3,     2,     1,     1,     1,     3,     1,     1,
       0,     1,     2,     4,     4,     4,     4,     3,     1,     3,
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
#line 299 "yacc_sql.y"
  {
    std::unique_ptr<ParsedSqlNode> sql_node = std::unique_ptr<ParsedSqlNode>((yyvsp[-1].sql_node));
    sql_result->add_sql_node(std::move(sql_node));
  }
#line 1997 "yacc_sql.cpp"
    break;

  case 26: /* exit_stmt: EXIT  */
#line 332 "yacc_sql.y"
         {
      (void)yynerrs;  // 这么写为了消除yynerrs未使用的告警。如果你有更好的方法欢迎提PR
      (yyval.sql_node) = new ParsedSqlNode(SCF_EXIT);
    }
#line 2006 "yacc_sql.cpp"
    break;

  case 27: /* help_stmt: HELP  */
#line 338 "yacc_sql.y"
         {
      (yyval.sql_node) = new ParsedSqlNode(SCF_HELP);
    }
#line 2014 "yacc_sql.cpp"
    break;

  case 28: /* sync_stmt: SYNC  */
#line 343 "yacc_sql.y"
         {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SYNC);
    }
#line 2022 "yacc_sql.cpp"
    break;

  case 29: /* begin_stmt: TRX_BEGIN  */
#line 349 "yacc_sql.y"
               {
      (yyval.sql_node) = new ParsedSqlNode(SCF_BEGIN);
    }
#line 2030 "yacc_sql.cpp"
    break;

  case 30: /* commit_stmt: TRX_COMMIT  */
#line 355 "yacc_sql.y"
               {
      (yyval.sql_node) = new ParsedSqlNode(SCF_COMMIT);
    }
#line 2038 "yacc_sql.cpp"
    break;

  case 31: /* rollback_stmt: TRX_ROLLBACK  */
#line 361 "yacc_sql.y"
                  {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ROLLBACK);
    }
#line 2046 "yacc_sql.cpp"
    break;

  case 32: /* drop_table_stmt: DROP TABLE ID  */
#line 367 "yacc_sql.y"
                  {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_TABLE);
      (yyval.sql_node)->drop_table.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2056 "yacc_sql.cpp"
    break;

  case 33: /* alter_table_stmt: ALTER TABLE ID ADD COLUMN attr_def  */
#line 375 "yacc_sql.y"
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
#line 2072 "yacc_sql.cpp"
    break;

  case 34: /* alter_table_stmt: ALTER TABLE ID DROP COLUMN ID  */
#line 387 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ALTER_TABLE);
      AlterTableSqlNode &alter_table = (yyval.sql_node)->alter_table;
      alter_table.table_name         = (yyvsp[-3].string);
      alter_table.alter_type         = AlterType::DROP_COLUMN;
      alter_table.column_name        = (yyvsp[0].string);
      free((yyvsp[-3].string));
      free((yyvsp[0].string));
    }
#line 2086 "yacc_sql.cpp"
    break;

  case 35: /* alter_table_stmt: ALTER TABLE ID CHANGE COLUMN ID ID  */
#line 397 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ALTER_TABLE);
      AlterTableSqlNode &alter_table = (yyval.sql_node)->alter_table;
      alter_table.table_name         = (yyvsp[-4].string);
      alter_table.alter_type         = AlterType::CHANGE_COLUMN;
      alter_table.column_name        = (yyvsp[-1].string);
      alter_table.new_column_name    = (yyvsp[0].string);
      free((yyvsp[-4].string));
      free((yyvsp[-1].string));
      free((yyvsp[0].string));
    }
#line 2102 "yacc_sql.cpp"
    break;

  case 36: /* alter_table_stmt: ALTER TABLE ID RENAME TO ID  */
#line 409 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ALTER_TABLE);
      AlterTableSqlNode &alter_table = (yyval.sql_node)->alter_table;
      alter_table.table_name         = (yyvsp[-3].string);
      alter_table.alter_type         = AlterType::RENAME_TABLE;
      alter_table.new_table_name     = (yyvsp[0].string);
      free((yyvsp[-3].string));
      free((yyvsp[0].string));
    }
#line 2116 "yacc_sql.cpp"
    break;

  case 37: /* show_tables_stmt: SHOW TABLES  */
#line 421 "yacc_sql.y"
                {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SHOW_TABLES);
    }
#line 2124 "yacc_sql.cpp"
    break;

  case 38: /* desc_table_stmt: DESC ID  */
#line 427 "yacc_sql.y"
             {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DESC_TABLE);
      (yyval.sql_node)->desc_table.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2134 "yacc_sql.cpp"
    break;

  case 39: /* show_index_stmt: SHOW INDEX FROM relation  */
#line 436 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SHOW_INDEX);
      ShowIndexSqlNode &show_index = (yyval.sql_node)->show_index;
      show_index.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2145 "yacc_sql.cpp"
    break;

  case 40: /* create_index_stmt: CREATE opt_unique INDEX ID ON ID LBRACE attr_list RBRACE  */
#line 446 "yacc_sql.y"
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
#line 2161 "yacc_sql.cpp"
    break;

  case 41: /* create_index_stmt: CREATE VECTOR_T INDEX ID ON ID LBRACE attr_list RBRACE WITH vector_index_config  */
#line 458 "yacc_sql.y"
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
#line 2178 "yacc_sql.cpp"
    break;

  case 42: /* opt_unique: UNIQUE  */
#line 473 "yacc_sql.y"
           { (yyval.unique) = true; }
#line 2184 "yacc_sql.cpp"
    break;

  case 43: /* opt_unique: %empty  */
#line 474 "yacc_sql.y"
                { (yyval.unique) = false; }
#line 2190 "yacc_sql.cpp"
    break;

  case 44: /* index_type: IVFFLAT  */
#line 479 "yacc_sql.y"
    {
      (yyval.index_type) = IndexType::VectorIVFFlatIndex;
    }
#line 2198 "yacc_sql.cpp"
    break;

  case 45: /* vector_index_config: LBRACE DISTANCE EQ ID COMMA TYPE EQ index_type RBRACE  */
#line 486 "yacc_sql.y"
    {
      (yyval.vector_index_config) = new VectorIndexConfig;
      (yyval.vector_index_config)->distance_fn = (yyvsp[-5].string);
      (yyval.vector_index_config)->index_type = (yyvsp[-1].index_type);
      free((yyvsp[-5].string));
    }
#line 2209 "yacc_sql.cpp"
    break;

  case 46: /* vector_index_config: LBRACE DISTANCE EQ ID COMMA TYPE EQ index_type COMMA LISTS EQ value COMMA PROBES EQ value RBRACE  */
#line 493 "yacc_sql.y"
    {
      (yyval.vector_index_config) = new VectorIndexConfig;
      (yyval.vector_index_config)->distance_fn = (yyvsp[-13].string);
      (yyval.vector_index_config)->index_type = (yyvsp[-9].index_type);
      (yyval.vector_index_config)->lists = std::move(*(yyvsp[-5].value));
      (yyval.vector_index_config)->probes = std::move(*(yyvsp[-1].value));
      free((yyvsp[-13].string));
    }
#line 2222 "yacc_sql.cpp"
    break;

  case 47: /* vector_index_config: LBRACE TYPE EQ index_type COMMA DISTANCE EQ ID COMMA LISTS EQ value COMMA PROBES EQ value RBRACE  */
#line 502 "yacc_sql.y"
    {
      (yyval.vector_index_config) = new VectorIndexConfig;
      (yyval.vector_index_config)->distance_fn = (yyvsp[-9].string);
      (yyval.vector_index_config)->index_type = (yyvsp[-13].index_type);
      (yyval.vector_index_config)->lists = std::move(*(yyvsp[-5].value));
      (yyval.vector_index_config)->probes = std::move(*(yyvsp[-1].value));
      free((yyvsp[-9].string));
    }
#line 2235 "yacc_sql.cpp"
    break;

  case 48: /* attr_list: ID  */
#line 514 "yacc_sql.y"
    {
      (yyval.index_attr_list) = new std::vector<std::string>; // 创建一个新的 vector
      (yyval.index_attr_list)->emplace_back((yyvsp[0].string)); // 将列名加入 vector
      free((yyvsp[0].string));
    }
#line 2245 "yacc_sql.cpp"
    break;

  case 49: /* attr_list: ID COMMA attr_list  */
#line 520 "yacc_sql.y"
    {
      (yyval.index_attr_list) = (yyvsp[0].index_attr_list); // 使用现有的 vector
      (yyval.index_attr_list)->emplace((yyval.index_attr_list)->begin(), (yyvsp[-2].string)); // 将新列名加入 vector 开头
      free((yyvsp[-2].string));
    }
#line 2255 "yacc_sql.cpp"
    break;

  case 50: /* drop_index_stmt: DROP INDEX ID ON ID  */
#line 529 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_INDEX);
      (yyval.sql_node)->drop_index.index_name = (yyvsp[-2].string);
      (yyval.sql_node)->drop_index.relation_name = (yyvsp[0].string);
      free((yyvsp[-2].string));
      free((yyvsp[0].string));
    }
#line 2267 "yacc_sql.cpp"
    break;

  case 51: /* create_table_stmt: CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE storage_format AS select_stmt  */
#line 539 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-7].string), (yyvsp[-5].attr_info), (yyvsp[-4].attr_infos), (yyvsp[-2].string), (yyvsp[0].sql_node));
    }
#line 2275 "yacc_sql.cpp"
    break;

  case 52: /* create_table_stmt: CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE storage_format select_stmt  */
#line 543 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-6].string), (yyvsp[-4].attr_info), (yyvsp[-3].attr_infos), (yyvsp[-1].string), (yyvsp[0].sql_node));
    }
#line 2283 "yacc_sql.cpp"
    break;

  case 53: /* create_table_stmt: CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE storage_format  */
#line 547 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-5].string), (yyvsp[-3].attr_info), (yyvsp[-2].attr_infos), (yyvsp[0].string), nullptr);
    }
#line 2291 "yacc_sql.cpp"
    break;

  case 54: /* create_table_stmt: CREATE TABLE ID storage_format AS select_stmt  */
#line 551 "yacc_sql.y"
    {
        (yyval.sql_node) = create_table_sql_node((yyvsp[-3].string), nullptr, nullptr, (yyvsp[-2].string), (yyvsp[0].sql_node));
    }
#line 2299 "yacc_sql.cpp"
    break;

  case 55: /* create_table_stmt: CREATE TABLE ID storage_format select_stmt  */
#line 555 "yacc_sql.y"
    {
      (yyval.sql_node) = create_table_sql_node((yyvsp[-2].string), nullptr, nullptr, (yyvsp[-1].string), (yyvsp[0].sql_node));
    }
#line 2307 "yacc_sql.cpp"
    break;

  case 56: /* create_view_stmt: CREATE VIEW ID AS select_stmt  */
#line 562 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CREATE_VIEW);
      CreateViewSqlNode &create_view = (yyval.sql_node)->create_view;
      create_view.relation_name = (yyvsp[-2].string);
      create_view.create_view_select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      free((yyvsp[-2].string));
    }
#line 2319 "yacc_sql.cpp"
    break;

  case 57: /* create_view_stmt: CREATE VIEW ID LBRACE attr_list RBRACE AS select_stmt  */
#line 570 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CREATE_VIEW);
      CreateViewSqlNode &create_view = (yyval.sql_node)->create_view;
      create_view.relation_name = (yyvsp[-5].string);
      create_view.attribute_names = std::move(*(yyvsp[-3].index_attr_list));
      create_view.create_view_select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      free((yyvsp[-5].string));
    }
#line 2332 "yacc_sql.cpp"
    break;

  case 58: /* drop_view_stmt: DROP VIEW ID  */
#line 582 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_VIEW);
      (yyval.sql_node)->drop_view.relation_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 2342 "yacc_sql.cpp"
    break;

  case 59: /* attr_def_list: %empty  */
#line 591 "yacc_sql.y"
    {
      (yyval.attr_infos) = nullptr;
    }
#line 2350 "yacc_sql.cpp"
    break;

  case 60: /* attr_def_list: COMMA attr_def attr_def_list  */
#line 595 "yacc_sql.y"
    {
      if ((yyvsp[0].attr_infos) != nullptr) {
        (yyval.attr_infos) = (yyvsp[0].attr_infos);
      } else {
        (yyval.attr_infos) = new std::vector<AttrInfoSqlNode>;
      }
      (yyval.attr_infos)->emplace_back(*(yyvsp[-1].attr_info));
      delete (yyvsp[-1].attr_info);
    }
#line 2364 "yacc_sql.cpp"
    break;

  case 61: /* attr_def: ID type LBRACE NUMBER RBRACE nullable_constraint  */
#line 608 "yacc_sql.y"
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
#line 2386 "yacc_sql.cpp"
    break;

  case 62: /* attr_def: ID type nullable_constraint  */
#line 626 "yacc_sql.y"
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
#line 2416 "yacc_sql.cpp"
    break;

  case 63: /* nullable_constraint: NOT NULL_T  */
#line 655 "yacc_sql.y"
    {
      (yyval.nullable_info) = false;  // NOT NULL 对应的可空性为 false
    }
#line 2424 "yacc_sql.cpp"
    break;

  case 64: /* nullable_constraint: NULLABLE  */
#line 659 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;  // NULLABLE 对应的可空性为 true 2022
    }
#line 2432 "yacc_sql.cpp"
    break;

  case 65: /* nullable_constraint: NULL_T  */
#line 663 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;  // NULL 对应的可空性也为 true 2023
    }
#line 2440 "yacc_sql.cpp"
    break;

  case 66: /* nullable_constraint: %empty  */
#line 667 "yacc_sql.y"
    {
      (yyval.nullable_info) = true;  // 默认情况为 NULL
    }
#line 2448 "yacc_sql.cpp"
    break;

  case 67: /* type: INT_T  */
#line 673 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::INTS);   }
#line 2454 "yacc_sql.cpp"
    break;

  case 68: /* type: STRING_T  */
#line 674 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::CHARS);  }
#line 2460 "yacc_sql.cpp"
    break;

  case 69: /* type: FLOAT_T  */
#line 675 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::FLOATS); }
#line 2466 "yacc_sql.cpp"
    break;

  case 70: /* type: DATE_T  */
#line 676 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::DATES);  }
#line 2472 "yacc_sql.cpp"
    break;

  case 71: /* type: TEXT_T  */
#line 677 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::TEXTS);  }
#line 2478 "yacc_sql.cpp"
    break;

  case 72: /* type: VECTOR_T  */
#line 678 "yacc_sql.y"
                 { (yyval.number) = static_cast<int>(AttrType::VECTORS);  }
#line 2484 "yacc_sql.cpp"
    break;

  case 73: /* insert_stmt: INSERT INTO ID VALUES values_list  */
#line 683 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_INSERT);
      (yyval.sql_node)->insertion.relation_name = (yyvsp[-2].string);
      if ((yyvsp[0].values_list) != nullptr) {
        (yyval.sql_node)->insertion.values_list.swap(*(yyvsp[0].values_list));
        delete (yyvsp[0].values_list);
      }
      free((yyvsp[-2].string));
    }
#line 2498 "yacc_sql.cpp"
    break;

  case 74: /* insert_stmt: INSERT INTO ID LBRACE attr_list RBRACE VALUES values_list  */
#line 693 "yacc_sql.y"
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
#line 2513 "yacc_sql.cpp"
    break;

  case 75: /* values_list: LBRACE value_list RBRACE  */
#line 707 "yacc_sql.y"
    {
      (yyval.values_list) = new std::vector<std::vector<Value>>;
      (yyval.values_list)->emplace_back(*(yyvsp[-1].value_list));
      delete (yyvsp[-1].value_list);
    }
#line 2523 "yacc_sql.cpp"
    break;

  case 76: /* values_list: values_list COMMA LBRACE value_list RBRACE  */
#line 713 "yacc_sql.y"
    {
      (yyval.values_list)->emplace_back(*(yyvsp[-1].value_list));
      delete (yyvsp[-1].value_list);
    }
#line 2532 "yacc_sql.cpp"
    break;

  case 77: /* digits: NUMBER  */
#line 720 "yacc_sql.y"
    {
      (yyval.digits) = float((yyvsp[0].number));
    }
#line 2540 "yacc_sql.cpp"
    break;

  case 78: /* digits: '-' NUMBER  */
#line 724 "yacc_sql.y"
    {
      (yyval.digits) = float(-(yyvsp[0].number));
    }
#line 2548 "yacc_sql.cpp"
    break;

  case 79: /* digits: FLOAT  */
#line 728 "yacc_sql.y"
    {
      (yyval.digits) = (yyvsp[0].floats);
    }
#line 2556 "yacc_sql.cpp"
    break;

  case 80: /* digits: '-' FLOAT  */
#line 732 "yacc_sql.y"
    {
      (yyval.digits) = (yyvsp[0].floats);
    }
#line 2564 "yacc_sql.cpp"
    break;

  case 81: /* digits_list: %empty  */
#line 739 "yacc_sql.y"
    {
      (yyval.digits_list) = new std::vector<float>();
    }
#line 2572 "yacc_sql.cpp"
    break;

  case 82: /* digits_list: digits  */
#line 743 "yacc_sql.y"
    {
      (yyval.digits_list) = new std::vector<float>();
      (yyval.digits_list)->push_back((yyvsp[0].digits));
    }
#line 2581 "yacc_sql.cpp"
    break;

  case 83: /* digits_list: digits_list COMMA digits  */
#line 748 "yacc_sql.y"
    {
      (yyval.digits_list)->push_back((yyvsp[0].digits));
    }
#line 2589 "yacc_sql.cpp"
    break;

  case 84: /* value_list: %empty  */
#line 755 "yacc_sql.y"
    {
      (yyval.value_list) = new std::vector<Value>;
    }
#line 2597 "yacc_sql.cpp"
    break;

  case 85: /* value_list: value  */
#line 759 "yacc_sql.y"
    {
      (yyval.value_list) = new std::vector<Value>;
      (yyval.value_list)->emplace_back(*(yyvsp[0].value));
      delete (yyvsp[0].value);
    }
#line 2607 "yacc_sql.cpp"
    break;

  case 86: /* value_list: value_list COMMA value  */
#line 765 "yacc_sql.y"
    {
      (yyval.value_list)->emplace_back(*(yyvsp[0].value));
      delete (yyvsp[0].value);
    }
#line 2616 "yacc_sql.cpp"
    break;

  case 87: /* value: nonnegative_value  */
#line 772 "yacc_sql.y"
                      {
      (yyval.value) = (yyvsp[0].value);
    }
#line 2624 "yacc_sql.cpp"
    break;

  case 88: /* value: '-' NUMBER  */
#line 775 "yacc_sql.y"
                 {
      (yyval.value) = new Value(-(yyvsp[0].number));
      (yyloc) = (yylsp[-1]);
    }
#line 2633 "yacc_sql.cpp"
    break;

  case 89: /* value: '-' FLOAT  */
#line 779 "yacc_sql.y"
                {
      (yyval.value) = new Value(-(yyvsp[0].floats));
      (yyloc) = (yylsp[-1]);
    }
#line 2642 "yacc_sql.cpp"
    break;

  case 90: /* nonnegative_value: NUMBER  */
#line 786 "yacc_sql.y"
           {
      (yyval.value) = new Value((yyvsp[0].number));
      (yyloc) = (yylsp[0]);
    }
#line 2651 "yacc_sql.cpp"
    break;

  case 91: /* nonnegative_value: FLOAT  */
#line 790 "yacc_sql.y"
            {
      (yyval.value) = new Value((yyvsp[0].floats));
      (yyloc) = (yylsp[0]);
    }
#line 2660 "yacc_sql.cpp"
    break;

  case 92: /* nonnegative_value: SSS  */
#line 794 "yacc_sql.y"
          {
      char *tmp = common::substr((yyvsp[0].string),1,strlen((yyvsp[0].string))-2);
      (yyval.value) = new Value(tmp);
      free(tmp);
      free((yyvsp[0].string));
    }
#line 2671 "yacc_sql.cpp"
    break;

  case 93: /* nonnegative_value: TRUE  */
#line 800 "yacc_sql.y"
           {
      (yyval.value) = new Value(true);
    }
#line 2679 "yacc_sql.cpp"
    break;

  case 94: /* nonnegative_value: FALSE  */
#line 803 "yacc_sql.y"
            {
      (yyval.value) = new Value(false);
    }
#line 2687 "yacc_sql.cpp"
    break;

  case 95: /* nonnegative_value: NULL_T  */
#line 806 "yacc_sql.y"
             {
      (yyval.value) = new Value(NullValue());
    }
#line 2695 "yacc_sql.cpp"
    break;

  case 96: /* nonnegative_value: LSBRACE digits_list RSBRACE  */
#line 809 "yacc_sql.y"
                                  {
      (yyval.value) = new Value(*(yyvsp[-1].digits_list));
    }
#line 2703 "yacc_sql.cpp"
    break;

  case 97: /* nonnegative_value: STRING_TO_VECTOR LBRACE value_list RBRACE  */
#line 812 "yacc_sql.y"
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
#line 2725 "yacc_sql.cpp"
    break;

  case 98: /* nonnegative_value: VECTOR_TO_STRING LBRACE value_list RBRACE  */
#line 829 "yacc_sql.y"
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
#line 2747 "yacc_sql.cpp"
    break;

  case 99: /* storage_format: %empty  */
#line 850 "yacc_sql.y"
    {
      (yyval.string) = nullptr;
    }
#line 2755 "yacc_sql.cpp"
    break;

  case 100: /* storage_format: STORAGE FORMAT EQ ID  */
#line 854 "yacc_sql.y"
    {
      (yyval.string) = (yyvsp[0].string);
    }
#line 2763 "yacc_sql.cpp"
    break;

  case 101: /* delete_stmt: DELETE FROM ID where  */
#line 861 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DELETE);
      (yyval.sql_node)->deletion.relation_name = (yyvsp[-1].string);
      if ((yyvsp[0].expression) != nullptr) {
        (yyval.sql_node)->deletion.condition = std::unique_ptr<Expression>((yyvsp[0].expression));
      }
      free((yyvsp[-1].string));
    }
#line 2776 "yacc_sql.cpp"
    break;

  case 102: /* update_stmt: UPDATE ID SET set_clauses where  */
#line 873 "yacc_sql.y"
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
#line 2791 "yacc_sql.cpp"
    break;

  case 103: /* set_clauses: set_clause  */
#line 887 "yacc_sql.y"
    {
      (yyval.set_clauses) = new std::vector<SetClauseSqlNode>;
      (yyval.set_clauses)->emplace_back(std::move(*(yyvsp[0].set_clause)));
    }
#line 2800 "yacc_sql.cpp"
    break;

  case 104: /* set_clauses: set_clauses COMMA set_clause  */
#line 892 "yacc_sql.y"
    {
      (yyval.set_clauses)->emplace_back(std::move(*(yyvsp[0].set_clause)));
    }
#line 2808 "yacc_sql.cpp"
    break;

  case 105: /* set_clause: ID EQ expression  */
#line 899 "yacc_sql.y"
    {
      (yyval.set_clause) = new SetClauseSqlNode;
      (yyval.set_clause)->field_name = (yyvsp[-2].string);
      (yyval.set_clause)->value = std::unique_ptr<Expression>((yyvsp[0].expression));
      free((yyvsp[-2].string));
    }
#line 2819 "yacc_sql.cpp"
    break;

  case 106: /* select_stmt: select_core select_union_list  */
#line 909 "yacc_sql.y"
    {
      (yyval.sql_node) = (yyvsp[-1].sql_node);
      if ((yyvsp[0].set_operator_list) != nullptr) {
        (yyval.sql_node)->selection.set_operations.swap(*(yyvsp[0].set_operator_list));
        delete (yyvsp[0].set_operator_list);
      }
    }
#line 2831 "yacc_sql.cpp"
    break;

  case 107: /* select_union_list: %empty  */
#line 920 "yacc_sql.y"
    {
      (yyval.set_operator_list) = nullptr;
    }
#line 2839 "yacc_sql.cpp"
    break;

  case 108: /* select_union_list: select_union_list select_union_item  */
#line 924 "yacc_sql.y"
    {
      if ((yyvsp[-1].set_operator_list) != nullptr) {
        (yyval.set_operator_list) = (yyvsp[-1].set_operator_list);
      } else {
        (yyval.set_operator_list) = new std::vector<SetOperatorSqlNode>();
      }
      (yyval.set_operator_list)->emplace_back(std::move(*(yyvsp[0].set_operator_node)));
      delete (yyvsp[0].set_operator_node);
    }
#line 2853 "yacc_sql.cpp"
    break;

  case 109: /* select_union_item: UNION select_core  */
#line 937 "yacc_sql.y"
    {
      (yyval.set_operator_node) = new SetOperatorSqlNode;
      (yyval.set_operator_node)->union_all = false;
      (yyval.set_operator_node)->select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      delete (yyvsp[0].sql_node);
    }
#line 2864 "yacc_sql.cpp"
    break;

  case 110: /* select_union_item: UNION ALL select_core  */
#line 944 "yacc_sql.y"
    {
      (yyval.set_operator_node) = new SetOperatorSqlNode;
      (yyval.set_operator_node)->union_all = true;
      (yyval.set_operator_node)->select = std::make_unique<SelectSqlNode>(std::move((yyvsp[0].sql_node)->selection));
      delete (yyvsp[0].sql_node);
    }
#line 2875 "yacc_sql.cpp"
    break;

  case 111: /* select_core: SELECT expression_list FROM rel_list where group_by opt_having opt_order_by opt_limit  */
#line 954 "yacc_sql.y"
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
#line 2917 "yacc_sql.cpp"
    break;

  case 112: /* select_core: SELECT expression_list FROM relation INNER JOIN join_clauses where group_by  */
#line 992 "yacc_sql.y"
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
#line 2951 "yacc_sql.cpp"
    break;

  case 113: /* calc_stmt: CALC expression_list  */
#line 1025 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CALC);
      (yyval.sql_node)->calc.expressions.swap(*(yyvsp[0].expression_list));
      delete (yyvsp[0].expression_list);
    }
#line 2961 "yacc_sql.cpp"
    break;

  case 114: /* calc_stmt: SELECT expression_list  */
#line 1031 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CALC);
      (yyval.sql_node)->calc.expressions.swap(*(yyvsp[0].expression_list));
      delete (yyvsp[0].expression_list);
    }
#line 2971 "yacc_sql.cpp"
    break;

  case 115: /* expression_list: %empty  */
#line 1039 "yacc_sql.y"
                {
      (yyval.expression_list) = new std::vector<std::unique_ptr<Expression>>;
    }
#line 2979 "yacc_sql.cpp"
    break;

  case 116: /* expression_list: expression alias  */
#line 1043 "yacc_sql.y"
    {
      (yyval.expression_list) = new std::vector<std::unique_ptr<Expression>>;
      if (nullptr != (yyvsp[0].string)) {
        (yyvsp[-1].expression)->set_alias((yyvsp[0].string));
      }
      (yyval.expression_list)->emplace_back((yyvsp[-1].expression));
      free((yyvsp[0].string));
    }
#line 2992 "yacc_sql.cpp"
    break;

  case 117: /* expression_list: expression alias COMMA expression_list  */
#line 1052 "yacc_sql.y"
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
#line 3009 "yacc_sql.cpp"
    break;

  case 118: /* expression: expression '+' expression  */
#line 1067 "yacc_sql.y"
                              {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::ADD, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 3017 "yacc_sql.cpp"
    break;

  case 119: /* expression: expression '-' expression  */
#line 1070 "yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::SUB, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 3025 "yacc_sql.cpp"
    break;

  case 120: /* expression: expression '*' expression  */
#line 1073 "yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::MUL, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 3033 "yacc_sql.cpp"
    break;

  case 121: /* expression: expression '/' expression  */
#line 1076 "yacc_sql.y"
                                {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::DIV, (yyvsp[-2].expression), (yyvsp[0].expression), sql_string, &(yyloc));
    }
#line 3041 "yacc_sql.cpp"
    break;

  case 122: /* expression: LBRACE expression_list RBRACE  */
#line 1079 "yacc_sql.y"
                                    {
      if ((yyvsp[-1].expression_list)->size() == 1) {
        (yyval.expression) = (yyvsp[-1].expression_list)->front().get();
      } else {
        (yyval.expression) = new ListExpr(std::move(*(yyvsp[-1].expression_list)));
      }
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3054 "yacc_sql.cpp"
    break;

  case 123: /* expression: '-' expression  */
#line 1087 "yacc_sql.y"
                                  {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::NEGATIVE, (yyvsp[0].expression), nullptr, sql_string, &(yyloc));
    }
#line 3062 "yacc_sql.cpp"
    break;

  case 124: /* expression: nonnegative_value  */
#line 1090 "yacc_sql.y"
                        {
      (yyval.expression) = new ValueExpr(*(yyvsp[0].value));
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
      delete (yyvsp[0].value);
    }
#line 3072 "yacc_sql.cpp"
    break;

  case 125: /* expression: rel_attr  */
#line 1095 "yacc_sql.y"
               {
      RelAttrSqlNode *node = (yyvsp[0].rel_attr);
      (yyval.expression) = new UnboundFieldExpr(node->relation_name, node->attribute_name);
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
      delete (yyvsp[0].rel_attr);
    }
#line 3083 "yacc_sql.cpp"
    break;

  case 126: /* expression: '*'  */
#line 1101 "yacc_sql.y"
          {
      (yyval.expression) = new StarExpr();
    }
#line 3091 "yacc_sql.cpp"
    break;

  case 127: /* expression: ID DOT '*'  */
#line 1104 "yacc_sql.y"
                 {
      (yyval.expression) = new StarExpr((yyvsp[-2].string));
    }
#line 3099 "yacc_sql.cpp"
    break;

  case 128: /* expression: func_expr  */
#line 1107 "yacc_sql.y"
                {
      (yyval.expression) = (yyvsp[0].expression);      // AggrFuncExpr
    }
#line 3107 "yacc_sql.cpp"
    break;

  case 129: /* expression: sub_query_expr  */
#line 1110 "yacc_sql.y"
                     {
      (yyval.expression) = (yyvsp[0].expression); // SubQueryExpr
    }
#line 3115 "yacc_sql.cpp"
    break;

  case 130: /* alias: %empty  */
#line 1117 "yacc_sql.y"
                {
      (yyval.string) = nullptr;
    }
#line 3123 "yacc_sql.cpp"
    break;

  case 131: /* alias: ID  */
#line 1120 "yacc_sql.y"
         {
      (yyval.string) = (yyvsp[0].string);
    }
#line 3131 "yacc_sql.cpp"
    break;

  case 132: /* alias: AS ID  */
#line 1123 "yacc_sql.y"
            {
      (yyval.string) = (yyvsp[0].string);
    }
#line 3139 "yacc_sql.cpp"
    break;

  case 133: /* func_expr: ID LBRACE expression_list RBRACE  */
#line 1129 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr((yyvsp[-3].string), std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3148 "yacc_sql.cpp"
    break;

  case 134: /* func_expr: DISTANCE LBRACE expression_list RBRACE  */
#line 1134 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr("distance", std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3157 "yacc_sql.cpp"
    break;

  case 135: /* func_expr: STRING_TO_VECTOR LBRACE expression_list RBRACE  */
#line 1139 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr("string_to_vector", std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3166 "yacc_sql.cpp"
    break;

  case 136: /* func_expr: VECTOR_TO_STRING LBRACE expression_list RBRACE  */
#line 1144 "yacc_sql.y"
    {
        (yyval.expression) = new UnboundFunctionExpr("vector_to_string", std::move(*(yyvsp[-1].expression_list)));
        (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    }
#line 3175 "yacc_sql.cpp"
    break;

  case 137: /* sub_query_expr: LBRACE select_stmt RBRACE  */
#line 1152 "yacc_sql.y"
    {
      (yyval.expression) = new SubQueryExpr((yyvsp[-1].sql_node)->selection);
    }
#line 3183 "yacc_sql.cpp"
    break;

  case 138: /* rel_attr: ID  */
#line 1158 "yacc_sql.y"
       {
      (yyval.rel_attr) = new RelAttrSqlNode;
      (yyval.rel_attr)->attribute_name = (yyvsp[0].string);
      free((yyvsp[0].string));
    }
#line 3193 "yacc_sql.cpp"
    break;

  case 139: /* rel_attr: ID DOT ID  */
#line 1163 "yacc_sql.y"
                {
      (yyval.rel_attr) = new RelAttrSqlNode;
      (yyval.rel_attr)->relation_name  = (yyvsp[-2].string);
      (yyval.rel_attr)->attribute_name = (yyvsp[0].string);
      free((yyvsp[-2].string));
      free((yyvsp[0].string));
    }
#line 3205 "yacc_sql.cpp"
    break;

  case 140: /* relation: ID  */
#line 1173 "yacc_sql.y"
       {
      (yyval.string) = (yyvsp[0].string);
    }
#line 3213 "yacc_sql.cpp"
    break;

  case 141: /* rel_list: relation alias  */
#line 1179 "yacc_sql.y"
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
#line 3228 "yacc_sql.cpp"
    break;

  case 142: /* rel_list: relation alias COMMA rel_list  */
#line 1189 "yacc_sql.y"
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
#line 3247 "yacc_sql.cpp"
    break;

  case 143: /* join_clauses: relation ON condition  */
#line 1207 "yacc_sql.y"
    {
      (yyval.join_clauses) = new JoinSqlNode;
      (yyval.join_clauses)->relations.emplace_back((yyvsp[-2].string));
      (yyval.join_clauses)->conditions = std::unique_ptr<Expression>((yyvsp[0].expression));
      free((yyvsp[-2].string));
    }
#line 3258 "yacc_sql.cpp"
    break;

  case 144: /* join_clauses: relation ON condition INNER JOIN join_clauses  */
#line 1214 "yacc_sql.y"
    {
      (yyval.join_clauses) = (yyvsp[0].join_clauses);
      (yyval.join_clauses)->relations.emplace_back((yyvsp[-5].string));
      auto ptr = (yyval.join_clauses)->conditions.release();
      (yyval.join_clauses)->conditions = std::make_unique<ConjunctionExpr>(ConjunctionExpr::Type::AND, ptr, (yyvsp[-3].expression));
      free((yyvsp[-5].string));
    }
#line 3270 "yacc_sql.cpp"
    break;

  case 145: /* where: %empty  */
#line 1225 "yacc_sql.y"
    {
      (yyval.expression) = nullptr;
    }
#line 3278 "yacc_sql.cpp"
    break;

  case 146: /* where: WHERE condition  */
#line 1228 "yacc_sql.y"
                      {
      (yyval.expression) = (yyvsp[0].expression);  
    }
#line 3286 "yacc_sql.cpp"
    break;

  case 147: /* condition: expression comp_op expression  */
#line 1235 "yacc_sql.y"
    {
      (yyval.expression) = new ComparisonExpr((yyvsp[-1].comp), (yyvsp[-2].expression), (yyvsp[0].expression));
    }
#line 3294 "yacc_sql.cpp"
    break;

  case 148: /* condition: comp_op expression  */
#line 1239 "yacc_sql.y"
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
#line 3311 "yacc_sql.cpp"
    break;

  case 149: /* condition: condition AND condition  */
#line 1252 "yacc_sql.y"
    {
      (yyval.expression) = new ConjunctionExpr(ConjunctionExpr::Type::AND, (yyvsp[-2].expression), (yyvsp[0].expression));
    }
#line 3319 "yacc_sql.cpp"
    break;

  case 150: /* condition: condition OR condition  */
#line 1256 "yacc_sql.y"
    {
      (yyval.expression) = new ConjunctionExpr(ConjunctionExpr::Type::OR, (yyvsp[-2].expression), (yyvsp[0].expression));
    }
#line 3327 "yacc_sql.cpp"
    break;

  case 151: /* comp_op: EQ  */
#line 1262 "yacc_sql.y"
         { (yyval.comp) = EQUAL_TO; }
#line 3333 "yacc_sql.cpp"
    break;

  case 152: /* comp_op: LT  */
#line 1263 "yacc_sql.y"
         { (yyval.comp) = LESS_THAN; }
#line 3339 "yacc_sql.cpp"
    break;

  case 153: /* comp_op: GT  */
#line 1264 "yacc_sql.y"
         { (yyval.comp) = GREAT_THAN; }
#line 3345 "yacc_sql.cpp"
    break;

  case 154: /* comp_op: LE  */
#line 1265 "yacc_sql.y"
         { (yyval.comp) = LESS_EQUAL; }
#line 3351 "yacc_sql.cpp"
    break;

  case 155: /* comp_op: GE  */
#line 1266 "yacc_sql.y"
         { (yyval.comp) = GREAT_EQUAL; }
#line 3357 "yacc_sql.cpp"
    break;

  case 156: /* comp_op: NE  */
#line 1267 "yacc_sql.y"
         { (yyval.comp) = NOT_EQUAL; }
#line 3363 "yacc_sql.cpp"
    break;

  case 157: /* comp_op: IS  */
#line 1268 "yacc_sql.y"
         { (yyval.comp) = IS_OP; }
#line 3369 "yacc_sql.cpp"
    break;

  case 158: /* comp_op: IS NOT  */
#line 1269 "yacc_sql.y"
             { (yyval.comp) = IS_NOT_OP; }
#line 3375 "yacc_sql.cpp"
    break;

  case 159: /* comp_op: LIKE  */
#line 1270 "yacc_sql.y"
           { (yyval.comp) = LIKE_OP;}
#line 3381 "yacc_sql.cpp"
    break;

  case 160: /* comp_op: NOT LIKE  */
#line 1271 "yacc_sql.y"
               {(yyval.comp) = NOT_LIKE_OP;}
#line 3387 "yacc_sql.cpp"
    break;

  case 161: /* comp_op: IN  */
#line 1272 "yacc_sql.y"
         { (yyval.comp) = IN_OP; }
#line 3393 "yacc_sql.cpp"
    break;

  case 162: /* comp_op: NOT IN  */
#line 1273 "yacc_sql.y"
             { (yyval.comp) = NOT_IN_OP; }
#line 3399 "yacc_sql.cpp"
    break;

  case 163: /* comp_op: EXISTS  */
#line 1274 "yacc_sql.y"
             { (yyval.comp) = EXISTS_OP; }
#line 3405 "yacc_sql.cpp"
    break;

  case 164: /* comp_op: NOT EXISTS  */
#line 1275 "yacc_sql.y"
                 { (yyval.comp) = NOT_EXISTS_OP; }
#line 3411 "yacc_sql.cpp"
    break;

  case 165: /* opt_order_by: %empty  */
#line 1280 "yacc_sql.y"
    {
      (yyval.orderby_list) = nullptr;
    }
#line 3419 "yacc_sql.cpp"
    break;

  case 166: /* opt_order_by: ORDER BY sort_list  */
#line 1284 "yacc_sql.y"
    {
      (yyval.orderby_list) = (yyvsp[0].orderby_list);
      std::reverse((yyval.orderby_list)->begin(),(yyval.orderby_list)->end());
    }
#line 3428 "yacc_sql.cpp"
    break;

  case 167: /* sort_list: sort_unit  */
#line 1292 "yacc_sql.y"
        {
      (yyval.orderby_list) = new std::vector<OrderBySqlNode>;
      (yyval.orderby_list)->emplace_back(std::move(*(yyvsp[0].orderby_unit)));
	}
#line 3437 "yacc_sql.cpp"
    break;

  case 168: /* sort_list: sort_unit COMMA sort_list  */
#line 1297 "yacc_sql.y"
        {
      (yyvsp[0].orderby_list)->emplace_back(std::move(*(yyvsp[-2].orderby_unit)));
      (yyval.orderby_list) = (yyvsp[0].orderby_list);
	}
#line 3446 "yacc_sql.cpp"
    break;

  case 169: /* sort_unit: expression  */
#line 1305 "yacc_sql.y"
        {
      (yyval.orderby_unit) = new OrderBySqlNode();
      (yyval.orderby_unit)->expr = std::unique_ptr<Expression>((yyvsp[0].expression));
      (yyval.orderby_unit)->is_asc = true;
	}
#line 3456 "yacc_sql.cpp"
    break;

  case 170: /* sort_unit: expression DESC  */
#line 1311 "yacc_sql.y"
        {
      (yyval.orderby_unit) = new OrderBySqlNode();
      (yyval.orderby_unit)->expr = std::unique_ptr<Expression>((yyvsp[-1].expression));
      (yyval.orderby_unit)->is_asc = false;
	}
#line 3466 "yacc_sql.cpp"
    break;

  case 171: /* sort_unit: expression ASC  */
#line 1317 "yacc_sql.y"
        {
      (yyval.orderby_unit) = new OrderBySqlNode(); // 默认是升序
      (yyval.orderby_unit)->expr = std::unique_ptr<Expression>((yyvsp[-1].expression));
      (yyval.orderby_unit)->is_asc = true;
	}
#line 3476 "yacc_sql.cpp"
    break;

  case 172: /* group_by: %empty  */
#line 1326 "yacc_sql.y"
    {
      (yyval.expression_list) = nullptr;
    }
#line 3484 "yacc_sql.cpp"
    break;

  case 173: /* group_by: GROUP BY expression_list  */
#line 1330 "yacc_sql.y"
    {
      (yyval.expression_list) = (yyvsp[0].expression_list);
    }
#line 3492 "yacc_sql.cpp"
    break;

  case 174: /* opt_having: %empty  */
#line 1337 "yacc_sql.y"
    {
      (yyval.expression) = nullptr;
    }
#line 3500 "yacc_sql.cpp"
    break;

  case 175: /* opt_having: HAVING condition  */
#line 1341 "yacc_sql.y"
    {
      (yyval.expression) = (yyvsp[0].expression);
    }
#line 3508 "yacc_sql.cpp"
    break;

  case 176: /* opt_limit: %empty  */
#line 1348 "yacc_sql.y"
    {
      (yyval.limited_info) = nullptr;
    }
#line 3516 "yacc_sql.cpp"
    break;

  case 177: /* opt_limit: LIMIT NUMBER  */
#line 1352 "yacc_sql.y"
    {
      (yyval.limited_info) = new LimitSqlNode();
      (yyval.limited_info)->number = (yyvsp[0].number);
    }
#line 3525 "yacc_sql.cpp"
    break;

  case 178: /* explain_stmt: EXPLAIN command_wrapper  */
#line 1360 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_EXPLAIN);
      (yyval.sql_node)->explain.sql_node = std::unique_ptr<ParsedSqlNode>((yyvsp[0].sql_node));
    }
#line 3534 "yacc_sql.cpp"
    break;

  case 179: /* set_variable_stmt: SET ID EQ value  */
#line 1368 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SET_VARIABLE);
      (yyval.sql_node)->set_variable.name  = (yyvsp[-2].string);
      (yyval.sql_node)->set_variable.value = *(yyvsp[0].value);
      free((yyvsp[-2].string));
      delete (yyvsp[0].value);
    }
#line 3546 "yacc_sql.cpp"
    break;


#line 3550 "yacc_sql.cpp"

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

#line 1380 "yacc_sql.y"

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
