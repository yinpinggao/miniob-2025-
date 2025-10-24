/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_YACC_SQL_HPP_INCLUDED
# define YY_YY_YACC_SQL_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    SEMICOLON = 258,               /* SEMICOLON  */
    AS = 259,                      /* AS  */
    ASC = 260,                     /* ASC  */
    BY = 261,                      /* BY  */
    CREATE = 262,                  /* CREATE  */
    DROP = 263,                    /* DROP  */
    ALTER = 264,                   /* ALTER  */
    EXISTS = 265,                  /* EXISTS  */
    GROUP = 266,                   /* GROUP  */
    HAVING = 267,                  /* HAVING  */
    ORDER = 268,                   /* ORDER  */
    TABLE = 269,                   /* TABLE  */
    TABLES = 270,                  /* TABLES  */
    ADD = 271,                     /* ADD  */
    INDEX = 272,                   /* INDEX  */
    COLUMN = 273,                  /* COLUMN  */
    CALC = 274,                    /* CALC  */
    SELECT = 275,                  /* SELECT  */
    DESC = 276,                    /* DESC  */
    SHOW = 277,                    /* SHOW  */
    SYNC = 278,                    /* SYNC  */
    INSERT = 279,                  /* INSERT  */
    DELETE = 280,                  /* DELETE  */
    UPDATE = 281,                  /* UPDATE  */
    LBRACE = 282,                  /* LBRACE  */
    RBRACE = 283,                  /* RBRACE  */
    LSBRACE = 284,                 /* LSBRACE  */
    RSBRACE = 285,                 /* RSBRACE  */
    COMMA = 286,                   /* COMMA  */
    TRX_BEGIN = 287,               /* TRX_BEGIN  */
    TRX_COMMIT = 288,              /* TRX_COMMIT  */
    TRX_ROLLBACK = 289,            /* TRX_ROLLBACK  */
    INT_T = 290,                   /* INT_T  */
    IN = 291,                      /* IN  */
    TRUE = 292,                    /* TRUE  */
    FALSE = 293,                   /* FALSE  */
    STRING_T = 294,                /* STRING_T  */
    FLOAT_T = 295,                 /* FLOAT_T  */
    DATE_T = 296,                  /* DATE_T  */
    TEXT_T = 297,                  /* TEXT_T  */
    VECTOR_T = 298,                /* VECTOR_T  */
    NOT = 299,                     /* NOT  */
    UNIQUE = 300,                  /* UNIQUE  */
    NULL_T = 301,                  /* NULL_T  */
    LIMIT = 302,                   /* LIMIT  */
    NULLABLE = 303,                /* NULLABLE  */
    HELP = 304,                    /* HELP  */
    QUOTE = 305,                   /* QUOTE  */
    EXIT = 306,                    /* EXIT  */
    DOT = 307,                     /* DOT  */
    INTO = 308,                    /* INTO  */
    VALUES = 309,                  /* VALUES  */
    FROM = 310,                    /* FROM  */
    WHERE = 311,                   /* WHERE  */
    AND = 312,                     /* AND  */
    OR = 313,                      /* OR  */
    SET = 314,                     /* SET  */
    ON = 315,                      /* ON  */
    INFILE = 316,                  /* INFILE  */
    EXPLAIN = 317,                 /* EXPLAIN  */
    STORAGE = 318,                 /* STORAGE  */
    FORMAT = 319,                  /* FORMAT  */
    INNER = 320,                   /* INNER  */
    JOIN = 321,                    /* JOIN  */
    UNION = 322,                   /* UNION  */
    ALL = 323,                     /* ALL  */
    VIEW = 324,                    /* VIEW  */
    WITH = 325,                    /* WITH  */
    STRING_TO_VECTOR = 326,        /* STRING_TO_VECTOR  */
    VECTOR_TO_STRING = 327,        /* VECTOR_TO_STRING  */
    DISTANCE = 328,                /* DISTANCE  */
    TYPE = 329,                    /* TYPE  */
    CHANGE = 330,                  /* CHANGE  */
    LISTS = 331,                   /* LISTS  */
    PROBES = 332,                  /* PROBES  */
    IVFFLAT = 333,                 /* IVFFLAT  */
    EQ = 334,                      /* EQ  */
    LT = 335,                      /* LT  */
    GT = 336,                      /* GT  */
    LE = 337,                      /* LE  */
    GE = 338,                      /* GE  */
    NE = 339,                      /* NE  */
    LIKE = 340,                    /* LIKE  */
    IS = 341,                      /* IS  */
    RENAME = 342,                  /* RENAME  */
    TO = 343,                      /* TO  */
    NUMBER = 344,                  /* NUMBER  */
    FLOAT = 345,                   /* FLOAT  */
    ID = 346,                      /* ID  */
    SSS = 347,                     /* SSS  */
    UMINUS = 348                   /* UMINUS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 185 "yacc_sql.y"

  ParsedSqlNode *                            sql_node;
  Value *                                    value;
  enum CompOp                                comp;
  RelAttrSqlNode *                           rel_attr;
  std::vector<AttrInfoSqlNode> *             attr_infos;
  AttrInfoSqlNode *                          attr_info;
  Expression *                               expression;
  std::vector<std::unique_ptr<Expression>> * expression_list;
  std::vector<Value> *                       value_list;
  std::vector<std::vector<Value>> *          values_list;
  SetClauseSqlNode *                         set_clause;
  std::vector<SetClauseSqlNode> *            set_clauses;
  JoinSqlNode *                              join_clauses;
  std::vector<RelAttrSqlNode> *              rel_attr_list;
  std::vector<RelationNode> *                relation_list;
  OrderBySqlNode *                           orderby_unit;
  std::vector<OrderBySqlNode> *              orderby_list;
  LimitSqlNode *                             limited_info;
  char *                                     string;
  int                                        number;
  float                                      floats;
  bool                                       nullable_info;
  std::vector<std::string> *                 index_attr_list;
  bool                                       unique;
  enum IndexType                             index_type;
  VectorIndexConfig *                        vector_index_config;
  float                                      digits;
  std::vector<float> *                       digits_list;
  SetOperatorSqlNode *                       set_operator_node;
  std::vector<SetOperatorSqlNode> *          set_operator_list;

#line 190 "yacc_sql.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif




int yyparse (const char * sql_string, ParsedSqlResult * sql_result, void * scanner);


#endif /* !YY_YY_YACC_SQL_HPP_INCLUDED  */
