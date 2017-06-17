#ifndef lint
static char const 
SoCalc_yyrcsid[] = "$FreeBSD: src/usr.bin/yacc/skeleton.c,v 1.28 2000/01/17 02:04:06 bde Exp $";
#endif
#include <stdlib.h>
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX SoCalc_yylex()
#define YYEMPTY -1
#define SoCalc_yyclearin (SoCalc_yychar=(YYEMPTY))
#define SoCalc_yyerrok (SoCalc_yyerrflag=0)
#define YYRECOVERING() (SoCalc_yyerrflag!=0)
static int SoCalc_yygrowstack();
#define YYPREFIX "SoCalc_yy"
#line 19 "SoCalcParse.y"
#include "inv/misc/SoBasic.H"
#include "inv/errors/SoDebugError.H"
#include "SoCalcExpr.H"
#include <stdio.h>
#ifdef __APPLE__
#include <float.h>
#include <limits.h>
#else
#include <values.h>
#endif
#include <math.h>
#include <ctype.h>
#include <strings.h>
#include <stdlib.h>

void SoCalc_yyerror(const char *);
extern "C" int	SoCalc_yylex();
int SoCalc_yyparse();

static const char *In;
static ExprList	*EList;

static const struct {
    const char *name;
    float	val;
} Constants[] = {
    { "MAXFLOAT",	MAXFLOAT },
#ifdef __APPLE__
    { "MINFLOAT",	FLT_MIN },
#else
    { "MINFLOAT",	MINFLOAT },
#endif
    { "M_E",		M_E },
    { "M_LOG2E",	M_LOG2E },
    { "M_LOG10E",	M_LOG10E },
    { "M_LN2",		M_LN2 },
    { "M_LN10",		M_LN10 },
    { "M_PI",		M_PI },
    { "M_SQRT2",	M_SQRT2 },
    { "M_SQRT1_2",	M_SQRT1_2 },
};
#define NCONSTANTS (sizeof(Constants)/sizeof(Constants[0]))

static const struct {
    const char *name;
    Expr::Type	type;
} Inputs[] = {
    { "a", Expr::FLOAT}, { "b", Expr::FLOAT},
    { "c", Expr::FLOAT}, { "d", Expr::FLOAT},
    { "e", Expr::FLOAT}, { "f", Expr::FLOAT},
    { "g", Expr::FLOAT}, { "h", Expr::FLOAT},
    { "A", Expr::VEC3F}, { "B", Expr::VEC3F},
    { "C", Expr::VEC3F}, { "D", Expr::VEC3F},
    { "E", Expr::VEC3F}, { "F", Expr::VEC3F},
    { "G", Expr::VEC3F}, { "H", Expr::VEC3F},
}, Outputs[] = {
    { "oa", Expr::FLOAT}, { "ob", Expr::FLOAT},
    { "oc", Expr::FLOAT}, { "od", Expr::FLOAT},
    { "oA", Expr::VEC3F}, { "oB", Expr::VEC3F},
    { "oC", Expr::VEC3F}, { "oD", Expr::VEC3F},
}, Vars[] = {
    { "ta", Expr::FLOAT}, { "tb", Expr::FLOAT},
    { "tc", Expr::FLOAT}, { "td", Expr::FLOAT},
    { "te", Expr::FLOAT}, { "tf", Expr::FLOAT},
    { "tg", Expr::FLOAT}, { "th", Expr::FLOAT},
    { "tA", Expr::VEC3F}, { "tB", Expr::VEC3F},
    { "tC", Expr::VEC3F}, { "tD", Expr::VEC3F},
    { "tE", Expr::VEC3F}, { "tF", Expr::VEC3F},
    { "tG", Expr::VEC3F}, { "tH", Expr::VEC3F},
};
#define NINPUTS (sizeof(Inputs)/sizeof(Inputs[0]))
#define NOUTPUTS (sizeof(Outputs)/sizeof(Outputs[0]))
#define NVARS (sizeof(Vars)/sizeof(Vars[0]))

static double rand(double max) { return max * drand48(); }
static SbVec3f cross(const SbVec3f &a, const SbVec3f &b) { return a.cross(b); }
static double dot(const SbVec3f &a, const SbVec3f &b) { return a.dot(b); }
static double length(const SbVec3f &a) { return a.length(); }
static SbVec3f normalize(const SbVec3f &v) { SbVec3f t=v; t.normalize(); return t; }
static SbVec3f vec3f(double a, double b, double c) { return SbVec3f(a,b,c); }

/* Keep this up to date with the info in initFuncs()*/
#define NFUNCS 25

static struct {
    const Func *func;
} Funcs[NFUNCS];

#line 113 "SoCalcParse.y"
typedef union
{
    Expr	*expr;
    ExprList	*list;
} YYSTYPE;
#line 112 "y.tab.c"
#define YYERRCODE 256
#define LEXERR 257
#define CONST 258
#define FUNC 259
#define INPUT 260
#define OUTPUT 261
#define VAR 262
#define OROR 263
#define ANDAND 264
#define GEQ 265
#define LEQ 266
#define EQEQ 267
#define NEQ 268
const short SoCalc_yylhs[] = {                                        -1,
    0,    0,    0,    1,    1,    1,    1,    2,    2,    2,
    2,    2,    3,    3,    3,   12,   12,    4,    4,    4,
    5,    5,    5,    5,    6,    6,    6,    7,    7,    7,
    7,    7,    8,    8,    8,    9,    9,   10,   10,   11,
   11,
};
const short SoCalc_yylen[] = {                                         2,
    1,    3,    2,    3,    6,    3,    6,    1,    1,    1,
    1,    3,    1,    4,    4,    1,    3,    1,    2,    2,
    1,    3,    3,    3,    1,    3,    3,    1,    3,    3,
    3,    3,    1,    3,    3,    1,    3,    1,    3,    1,
    5,
};
const short SoCalc_yydefred[] = {                                      0,
    0,    0,    0,    1,    0,    0,    0,    0,    0,    8,
    0,    9,   10,   11,    0,    0,    0,   13,    0,   21,
    0,    0,    0,    0,    0,    0,    4,    0,    6,    0,
    2,    0,    0,   19,   20,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   16,    0,   12,    0,   22,   23,   24,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   15,    0,   14,    0,    5,    7,   17,   41,
};
const short SoCalc_yydgoto[] = {                                       3,
    4,   18,   19,   20,   21,   22,   23,   24,   25,   26,
   27,   54,
};
const short SoCalc_yysindex[] = {                                   -245,
  -61,  -56,  -46,    0,  -21,  -21,  -21,  -21, -245,    0,
  -22,    0,    0,    0,  -21,  -21,  -21,    0,  -68,    0,
  -27,  -36,  -58, -246, -235,  -57,    0,  -59,    0,  -54,
    0,  -21,    6,    0,    0,  -21,  -21,  -21,  -21,  -21,
  -21,  -21,  -21,  -21,  -21,  -21,  -21,  -21,  -21,  -21,
  -20,  -11,    0,  -30,    0,  -39,    0,    0,    0,  -27,
  -27,  -36,  -36,  -36,  -36,  -58,  -58, -246, -235,   -3,
  -21,  -21,    0,  -21,    0,  -21,    0,    0,    0,    0,
};
const short SoCalc_yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,    3,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    1,    0,
    8,   65,  136,  184,  349,  343,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   33,
   40,   72,   97,  104,  110,  142,  152,  192,  380,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
const short SoCalc_yygindex[] = {                                      0,
   47,    0,    0,  215,   -9,  -17,  -10,    9,   20,    0,
  212,    0,
};
#define YYTABLESIZE 643
const short SoCalc_yytable[] = {                                       5,
   18,   44,    3,   45,    7,   50,   41,   25,   40,   39,
   73,   17,    9,   74,   37,    1,    2,   32,   15,   38,
   46,   47,   36,   16,   62,   63,   64,   65,   48,    6,
   60,   61,   27,   51,    8,   66,   67,   18,   52,   26,
   71,   18,   18,   18,   18,   18,   55,   18,   25,   72,
   25,   25,   25,   75,   76,   31,   68,    0,   18,   18,
   18,    3,   18,   18,   28,   25,   25,   25,   69,   25,
   25,   32,    0,   27,    0,   27,   27,   27,    0,    0,
   26,    0,   26,   26,   26,    0,    0,    0,    0,    0,
   27,   27,   27,   18,   27,   27,   31,   26,   26,   26,
   25,   26,   26,   29,    0,   28,    0,    0,   28,   30,
    0,    0,   32,    0,    0,   32,    0,    0,    0,    0,
    0,    0,   28,   28,   28,   27,   28,   28,    0,   32,
   32,   32,   26,   32,   32,   33,    0,   31,    0,    0,
   31,   34,    0,    0,   29,    0,    0,   29,    0,    0,
   30,   35,    0,   30,   31,   31,   31,   28,   31,   31,
    0,   29,   29,   29,   32,   29,   29,   30,   30,   30,
    0,   30,   30,    0,    0,    0,   33,    0,    0,   33,
    0,    0,   34,   36,    0,   34,    0,    0,    0,   31,
    0,   37,   35,   33,   33,   35,   29,    0,   33,   34,
   34,    0,   30,    0,   34,   49,   42,   43,    0,   35,
   35,    0,    0,    0,   35,    0,    0,   28,   29,   30,
    0,    0,    0,    0,   36,    0,   33,   36,   33,    0,
   34,   35,   37,    0,   34,   37,   10,   11,   12,   13,
   14,   36,   36,   53,   35,    0,   36,   56,    0,   37,
   37,   57,   58,   59,   37,    0,    0,    0,    0,    0,
    0,   70,    0,   18,   18,   18,   18,   18,   18,    0,
   25,   25,   25,   25,   25,   25,   36,    0,    0,    0,
    0,    0,   77,   78,   37,   79,    0,   80,    0,    0,
    0,    0,    0,    0,    0,   27,   27,   27,   27,   27,
   27,    0,   26,   26,   26,   26,   26,   26,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   28,   28,   28,
   28,   28,   28,    0,   32,   32,   32,   32,   32,   32,
    0,    0,   40,    0,    0,    0,    0,    0,   38,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   31,
   31,   31,   31,   31,   31,    0,   29,   29,   29,   29,
   29,   29,   30,   30,   30,   30,   30,   30,    0,   39,
    0,    0,    0,   40,    0,    0,   40,    0,    0,   38,
    0,    0,   38,    0,    0,    0,    0,    0,   33,   33,
   40,   40,   33,   33,   34,   34,   38,   38,   34,   34,
    0,   38,    0,    0,   35,   35,    0,    0,   35,   35,
   39,    0,    0,   39,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   40,    0,   39,   39,    0,
    0,   38,   39,    0,    0,    0,   36,   36,    0,    0,
    0,    0,    0,    0,   37,   37,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   39,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   38,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   39,
};
const short SoCalc_yycheck[] = {                                      61,
    0,   60,    0,   62,   61,   63,   43,    0,   45,   37,
   41,   33,   59,   44,   42,  261,  262,   40,   40,   47,
  267,  268,   91,   45,   42,   43,   44,   45,  264,   91,
   40,   41,    0,   93,   91,   46,   47,   37,   93,    0,
   61,   41,   42,   43,   44,   45,   41,   47,   41,   61,
   43,   44,   45,   93,   58,    9,   48,   -1,   58,   59,
   60,   59,   62,   63,    0,   58,   59,   60,   49,   62,
   63,    0,   -1,   41,   -1,   43,   44,   45,   -1,   -1,
   41,   -1,   43,   44,   45,   -1,   -1,   -1,   -1,   -1,
   58,   59,   60,   93,   62,   63,    0,   58,   59,   60,
   93,   62,   63,    0,   -1,   41,   -1,   -1,   44,    0,
   -1,   -1,   41,   -1,   -1,   44,   -1,   -1,   -1,   -1,
   -1,   -1,   58,   59,   60,   93,   62,   63,   -1,   58,
   59,   60,   93,   62,   63,    0,   -1,   41,   -1,   -1,
   44,    0,   -1,   -1,   41,   -1,   -1,   44,   -1,   -1,
   41,    0,   -1,   44,   58,   59,   60,   93,   62,   63,
   -1,   58,   59,   60,   93,   62,   63,   58,   59,   60,
   -1,   62,   63,   -1,   -1,   -1,   41,   -1,   -1,   44,
   -1,   -1,   41,    0,   -1,   44,   -1,   -1,   -1,   93,
   -1,    0,   41,   58,   59,   44,   93,   -1,   63,   58,
   59,   -1,   93,   -1,   63,  263,  265,  266,   -1,   58,
   59,   -1,   -1,   -1,   63,   -1,   -1,    6,    7,    8,
   -1,   -1,   -1,   -1,   41,   -1,   15,   44,   93,   -1,
   16,   17,   41,   -1,   93,   44,  258,  259,  260,  261,
  262,   58,   59,   32,   93,   -1,   63,   36,   -1,   58,
   59,   37,   38,   39,   63,   -1,   -1,   -1,   -1,   -1,
   -1,   50,   -1,  263,  264,  265,  266,  267,  268,   -1,
  263,  264,  265,  266,  267,  268,   93,   -1,   -1,   -1,
   -1,   -1,   71,   72,   93,   74,   -1,   76,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  263,  264,  265,  266,  267,
  268,   -1,  263,  264,  265,  266,  267,  268,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  263,  264,  265,
  266,  267,  268,   -1,  263,  264,  265,  266,  267,  268,
   -1,   -1,    0,   -1,   -1,   -1,   -1,   -1,    0,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  263,
  264,  265,  266,  267,  268,   -1,  263,  264,  265,  266,
  267,  268,  263,  264,  265,  266,  267,  268,   -1,    0,
   -1,   -1,   -1,   41,   -1,   -1,   44,   -1,   -1,   41,
   -1,   -1,   44,   -1,   -1,   -1,   -1,   -1,  263,  264,
   58,   59,  267,  268,  263,  264,   58,   59,  267,  268,
   -1,   63,   -1,   -1,  263,  264,   -1,   -1,  267,  268,
   41,   -1,   -1,   44,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   93,   -1,   58,   59,   -1,
   -1,   93,   63,   -1,   -1,   -1,  263,  264,   -1,   -1,
   -1,   -1,   -1,   -1,  263,  264,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   93,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  263,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  263,
};
#define YYFINAL 3
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 268
#if YYDEBUG
const char * const SoCalc_yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"'!'",0,0,0,"'%'",0,0,"'('","')'","'*'","'+'","','","'-'",0,"'/'",0,0,0,0,0,0,0,
0,0,0,"':'","';'","'<'","'='","'>'","'?'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,"'['",0,"']'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"LEXERR","CONST","FUNC","INPUT","OUTPUT",
"VAR","OROR","ANDAND","GEQ","LEQ","EQEQ","NEQ",
};
const char * const SoCalc_yyrule[] = {
"$accept : asgnlist",
"asgnlist : asgn",
"asgnlist : asgnlist ';' asgn",
"asgnlist : asgnlist ';'",
"asgn : OUTPUT '=' conditional_expression",
"asgn : OUTPUT '[' conditional_expression ']' '=' conditional_expression",
"asgn : VAR '=' conditional_expression",
"asgn : VAR '[' conditional_expression ']' '=' conditional_expression",
"primary_expression : CONST",
"primary_expression : INPUT",
"primary_expression : OUTPUT",
"primary_expression : VAR",
"primary_expression : '(' conditional_expression ')'",
"postfix_expression : primary_expression",
"postfix_expression : postfix_expression '[' conditional_expression ']'",
"postfix_expression : FUNC '(' args ')'",
"args : conditional_expression",
"args : args ',' conditional_expression",
"unary_expression : postfix_expression",
"unary_expression : '-' unary_expression",
"unary_expression : '!' unary_expression",
"multiplicative_expression : unary_expression",
"multiplicative_expression : multiplicative_expression '*' unary_expression",
"multiplicative_expression : multiplicative_expression '/' unary_expression",
"multiplicative_expression : multiplicative_expression '%' unary_expression",
"additive_expression : multiplicative_expression",
"additive_expression : additive_expression '+' multiplicative_expression",
"additive_expression : additive_expression '-' multiplicative_expression",
"relational_expression : additive_expression",
"relational_expression : relational_expression '<' additive_expression",
"relational_expression : relational_expression '>' additive_expression",
"relational_expression : relational_expression LEQ additive_expression",
"relational_expression : relational_expression GEQ additive_expression",
"equality_expression : relational_expression",
"equality_expression : equality_expression EQEQ relational_expression",
"equality_expression : equality_expression NEQ relational_expression",
"logical_AND_expression : equality_expression",
"logical_AND_expression : logical_AND_expression ANDAND equality_expression",
"logical_OR_expression : logical_AND_expression",
"logical_OR_expression : logical_OR_expression OROR logical_AND_expression",
"conditional_expression : logical_OR_expression",
"conditional_expression : logical_OR_expression '?' conditional_expression ':' conditional_expression",
};
#endif
#if YYDEBUG
#include <stdio.h>
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH 10000
#endif
#endif
#define YYINITSTACKSIZE 200
int SoCalc_yydebug;
int SoCalc_yynerrs;
int SoCalc_yyerrflag;
int SoCalc_yychar;
short *SoCalc_yyssp;
YYSTYPE *SoCalc_yyvsp;
YYSTYPE SoCalc_yyval;
YYSTYPE SoCalc_yylval;
short *SoCalc_yyss;
short *SoCalc_yysslim;
YYSTYPE *SoCalc_yyvs;
int SoCalc_yystacksize;
#line 230 "SoCalcParse.y"

//
// The static Funcs array contains instances of objects that have
// constructors. Since this wouldn't get set up correctly in a DSO, we
// have to initialize the array at run-time.
//

static void
initFuncs()
{
    int	i = 0;

#define MAKEFUNC(CLASS, NAME)						      \
	Funcs[i++].func = new CLASS(SO__QUOTE(NAME),NAME)

    MAKEFUNC(Func_d, acos);
    MAKEFUNC(Func_d, asin);
    MAKEFUNC(Func_d, atan);
    MAKEFUNC(Func_dd, atan2);
    MAKEFUNC(Func_d, ceil);
    MAKEFUNC(Func_d, cos);
    MAKEFUNC(Func_d, cosh);
    MAKEFUNC(Funcv_vv, cross);
    MAKEFUNC(Func_vv, dot);
    MAKEFUNC(Func_d, exp);
    MAKEFUNC(Func_d, fabs);
    MAKEFUNC(Func_d, floor);
    MAKEFUNC(Func_dd, fmod);
    MAKEFUNC(Func_v, length);
    MAKEFUNC(Func_d, log);
    MAKEFUNC(Func_d, log10);
    MAKEFUNC(Funcv_v, normalize);
    MAKEFUNC(Func_dd, pow);
    MAKEFUNC(Func_d, rand);
    MAKEFUNC(Func_d, sin);
    MAKEFUNC(Func_d, sinh);
    MAKEFUNC(Func_d, sqrt);
    MAKEFUNC(Func_d, tan);
    MAKEFUNC(Func_d, tanh);
    MAKEFUNC(Funcv_ddd, vec3f);

#ifdef DEBUG
    // Sanity check
    if (i != NFUNCS)
	SoDebugError::post("SoCalcParse initFuncs (internal)",
			   "Wrong number of functions declared");
#endif /* DEBUG */
}

static Const *
isConst(const char *nm)
{
    for (int i=0; i<NCONSTANTS; i++)
	if (strcmp(nm, Constants[i].name)==0)
	    return new Const(Constants[i].val);
    return NULL;
}

static Func *
isFunc(const char *nm)
{
    for (int i=0; i<NFUNCS; i++) {
	const Func *f = Funcs[i].func;
	if (strcmp(nm, f->name)==0)
	    return f->dup();
    }
    return NULL;
}

static Var *
isInput(const char *nm)
{
    for (int i=0; i<NINPUTS; i++) if (strcmp(nm, Inputs[i].name)==0) 
	return new Var(nm, Inputs[i].type);
    return NULL;
}

static Var *
isOutput(const char *nm)
{
    for (int i=0; i<NOUTPUTS; i++) if (strcmp(nm, Outputs[i].name)==0) 
	return new Var(nm, Outputs[i].type);
    return NULL;
}

static Var *
isVar(const char *nm)
{
    for (int i=0; i<NVARS; i++) if (strcmp(nm, Vars[i].name)==0) 
	return new Var(nm, Vars[i].type);
    return NULL;
}

int SoCalc_yylex()
{
#define BSZ 100
    char c, buf[BSZ+1], *cp = buf;

    // skip over blanks
    while (isspace(*In))
	In++;

    if (!In[0])
	return EOF;

    if (isdigit(In[0]) || In[0] == '.') {

	// skip past a valid floating-point number
	// (don't need to compute the number, will
	// use atof() to do that).
	SbBool dot = FALSE;
	SbBool exp = FALSE;
	for (;;) {
	    c = *In++;
	    if (cp - buf < BSZ)
		*cp++ = c;
	    *cp = c;
	    if (isdigit(c))
		continue;
	    if (c== '.') {
		if (dot || exp)
		    return LEXERR;
		dot = TRUE;
		continue;
	    }
	    if (c == 'e' || c == 'E') {
		if (exp)
		    return LEXERR;
		exp = TRUE;
		continue;
	    }
	    // Check for negative exponent
	    if (c == '-' && exp && (In[-2] == 'e' || In[-2] == 'E')) {
		continue;
	    }
	    break;	// end of number
	}
	if (In[-1] == 'e')
	    return LEXERR;

	*cp = 0;
	In--;	// push back last character "read"

	SoCalc_yylval.expr = new Const(atof(buf));
	return CONST;
    }

    if (isalpha(In[0])) {
	while (isalnum(In[0]) || In[0] == '_') {
	    if (cp - buf < BSZ)
		*cp++ = *In++;
	}
	*cp = 0;

	if ((SoCalc_yylval.expr = isConst(buf)))
	    return CONST;

	if ((SoCalc_yylval.expr = isFunc(buf)))
	    return FUNC;

	if ((SoCalc_yylval.expr = isInput(buf)))
	    return INPUT;

	if ((SoCalc_yylval.expr = isOutput(buf)))
	    return OUTPUT;

	if ((SoCalc_yylval.expr = isVar(buf)))
	    return VAR;

    }

#define EQ2(x,y)	(x[0]==y[0] && x[1]==y[1])
    if (EQ2(In, "==")) { In += 2; return EQEQ; }
    if (EQ2(In, "!=")) { In += 2; return NEQ; }
    if (EQ2(In, ">=")) { In += 2; return GEQ; }
    if (EQ2(In, "<=")) { In += 2; return LEQ; }
    if (EQ2(In, "&&")) { In += 2; return ANDAND; }
    if (EQ2(In, "||")) { In += 2; return OROR; }

    return *In++;
}

void
SoCalc_yyerror(const char *)
{
    // printf("parse error: %s\n", s);
}

SbBool
SoCalcParse(ExprList *elist, const char *buf)
{
    static SbBool initted = FALSE;

    if (! initted) {
	initFuncs();
	initted = TRUE;
    }

    In = buf;
    EList = elist;
    return(SoCalc_yyparse());
}

#ifdef TESTMAIN

void
main()
{
    char buf[1000];
#if YYDEBUG
    extern int SoCalc_yydebug;
    SoCalc_yydebug = 1;
#endif
    EList = new ExprList;

    while (gets(buf)) {
	In = buf;
	EList->truncate(0);
	if (SoCalc_yyparse())
	    printf("Parse error.\n");
	else {
	    EList->eval();
	    for (int i=0; i<EList->length(); i++) {
		(*EList)[i]->pr();
	    }
	}
    }
}

#endif
#line 633 "y.tab.c"
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int SoCalc_yygrowstack()
{
    int newsize, i;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = SoCalc_yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;
    i = SoCalc_yyssp - SoCalc_yyss;
    newss = SoCalc_yyss ? (short *)realloc(SoCalc_yyss, newsize * sizeof *newss) :
      (short *)malloc(newsize * sizeof *newss);
    if (newss == NULL)
        return -1;
    SoCalc_yyss = newss;
    SoCalc_yyssp = newss + i;
    newvs = SoCalc_yyvs ? (YYSTYPE *)realloc(SoCalc_yyvs, newsize * sizeof *newvs) :
      (YYSTYPE *)malloc(newsize * sizeof *newvs);
    if (newvs == NULL)
        return -1;
    SoCalc_yyvs = newvs;
    SoCalc_yyvsp = newvs + i;
    SoCalc_yystacksize = newsize;
    SoCalc_yysslim = SoCalc_yyss + newsize - 1;
    return 0;
}

#define YYABORT goto SoCalc_yyabort
#define YYREJECT goto SoCalc_yyabort
#define YYACCEPT goto SoCalc_yyaccept
#define YYERROR goto SoCalc_yyerrlab

#ifndef YYPARSE_PARAM
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG void
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif	/* ANSI-C/C++ */
#else	/* YYPARSE_PARAM */
#ifndef YYPARSE_PARAM_TYPE
#define YYPARSE_PARAM_TYPE void *
#endif
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG YYPARSE_PARAM_TYPE YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL YYPARSE_PARAM_TYPE YYPARSE_PARAM;
#endif	/* ANSI-C/C++ */
#endif	/* ! YYPARSE_PARAM */

int
SoCalc_yyparse (YYPARSE_PARAM_ARG)
    YYPARSE_PARAM_DECL
{
    register int SoCalc_yym, SoCalc_yyn, SoCalc_yystate;
#if YYDEBUG
    register const char *SoCalc_yys;

    if ((SoCalc_yys = getenv("YYDEBUG")))
    {
        SoCalc_yyn = *SoCalc_yys;
        if (SoCalc_yyn >= '0' && SoCalc_yyn <= '9')
            SoCalc_yydebug = SoCalc_yyn - '0';
    }
#endif

    SoCalc_yynerrs = 0;
    SoCalc_yyerrflag = 0;
    SoCalc_yychar = (-1);

    if (SoCalc_yyss == NULL && SoCalc_yygrowstack()) goto SoCalc_yyoverflow;
    SoCalc_yyssp = SoCalc_yyss;
    SoCalc_yyvsp = SoCalc_yyvs;
    *SoCalc_yyssp = SoCalc_yystate = 0;

SoCalc_yyloop:
    if ((SoCalc_yyn = SoCalc_yydefred[SoCalc_yystate])) goto SoCalc_yyreduce;
    if (SoCalc_yychar < 0)
    {
        if ((SoCalc_yychar = SoCalc_yylex()) < 0) SoCalc_yychar = 0;
#if YYDEBUG
        if (SoCalc_yydebug)
        {
            SoCalc_yys = 0;
            if (SoCalc_yychar <= YYMAXTOKEN) SoCalc_yys = SoCalc_yyname[SoCalc_yychar];
            if (!SoCalc_yys) SoCalc_yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, SoCalc_yystate, SoCalc_yychar, SoCalc_yys);
        }
#endif
    }
    if ((SoCalc_yyn = SoCalc_yysindex[SoCalc_yystate]) && (SoCalc_yyn += SoCalc_yychar) >= 0 &&
            SoCalc_yyn <= YYTABLESIZE && SoCalc_yycheck[SoCalc_yyn] == SoCalc_yychar)
    {
#if YYDEBUG
        if (SoCalc_yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, SoCalc_yystate, SoCalc_yytable[SoCalc_yyn]);
#endif
        if (SoCalc_yyssp >= SoCalc_yysslim && SoCalc_yygrowstack())
        {
            goto SoCalc_yyoverflow;
        }
        *++SoCalc_yyssp = SoCalc_yystate = SoCalc_yytable[SoCalc_yyn];
        *++SoCalc_yyvsp = SoCalc_yylval;
        SoCalc_yychar = (-1);
        if (SoCalc_yyerrflag > 0)  --SoCalc_yyerrflag;
        goto SoCalc_yyloop;
    }
    if ((SoCalc_yyn = SoCalc_yyrindex[SoCalc_yystate]) && (SoCalc_yyn += SoCalc_yychar) >= 0 &&
            SoCalc_yyn <= YYTABLESIZE && SoCalc_yycheck[SoCalc_yyn] == SoCalc_yychar)
    {
        SoCalc_yyn = SoCalc_yytable[SoCalc_yyn];
        goto SoCalc_yyreduce;
    }
    if (SoCalc_yyerrflag) goto SoCalc_yyinrecovery;
#if defined(lint) || defined(__GNUC__)
    goto SoCalc_yynewerror;
#endif
SoCalc_yynewerror:
    SoCalc_yyerror("syntax error");
#if defined(lint) || defined(__GNUC__)
    goto SoCalc_yyerrlab;
#endif
SoCalc_yyerrlab:
    ++SoCalc_yynerrs;
SoCalc_yyinrecovery:
    if (SoCalc_yyerrflag < 3)
    {
        SoCalc_yyerrflag = 3;
        for (;;)
        {
            if ((SoCalc_yyn = SoCalc_yysindex[*SoCalc_yyssp]) && (SoCalc_yyn += YYERRCODE) >= 0 &&
                    SoCalc_yyn <= YYTABLESIZE && SoCalc_yycheck[SoCalc_yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (SoCalc_yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *SoCalc_yyssp, SoCalc_yytable[SoCalc_yyn]);
#endif
                if (SoCalc_yyssp >= SoCalc_yysslim && SoCalc_yygrowstack())
                {
                    goto SoCalc_yyoverflow;
                }
                *++SoCalc_yyssp = SoCalc_yystate = SoCalc_yytable[SoCalc_yyn];
                *++SoCalc_yyvsp = SoCalc_yylval;
                goto SoCalc_yyloop;
            }
            else
            {
#if YYDEBUG
                if (SoCalc_yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *SoCalc_yyssp);
#endif
                if (SoCalc_yyssp <= SoCalc_yyss) goto SoCalc_yyabort;
                --SoCalc_yyssp;
                --SoCalc_yyvsp;
            }
        }
    }
    else
    {
        if (SoCalc_yychar == 0) goto SoCalc_yyabort;
#if YYDEBUG
        if (SoCalc_yydebug)
        {
            SoCalc_yys = 0;
            if (SoCalc_yychar <= YYMAXTOKEN) SoCalc_yys = SoCalc_yyname[SoCalc_yychar];
            if (!SoCalc_yys) SoCalc_yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, SoCalc_yystate, SoCalc_yychar, SoCalc_yys);
        }
#endif
        SoCalc_yychar = (-1);
        goto SoCalc_yyloop;
    }
SoCalc_yyreduce:
#if YYDEBUG
    if (SoCalc_yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, SoCalc_yystate, SoCalc_yyn, SoCalc_yyrule[SoCalc_yyn]);
#endif
    SoCalc_yym = SoCalc_yylen[SoCalc_yyn];
    SoCalc_yyval = SoCalc_yyvsp[1-SoCalc_yym];
    switch (SoCalc_yyn)
    {
case 1:
#line 130 "SoCalcParse.y"
{ EList->append(SoCalc_yyvsp[0].expr); }
break;
case 2:
#line 131 "SoCalcParse.y"
{ EList->append(SoCalc_yyvsp[0].expr); }
break;
case 4:
#line 137 "SoCalcParse.y"
{ SoCalc_yyval.expr = new Assign(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 5:
#line 139 "SoCalcParse.y"
{ SoCalc_yyval.expr = new AssignIndex(SoCalc_yyvsp[-5].expr, SoCalc_yyvsp[-3].expr, SoCalc_yyvsp[0].expr); }
break;
case 6:
#line 141 "SoCalcParse.y"
{ SoCalc_yyval.expr = new Assign(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 7:
#line 143 "SoCalcParse.y"
{ SoCalc_yyval.expr = new AssignIndex(SoCalc_yyvsp[-5].expr, SoCalc_yyvsp[-3].expr, SoCalc_yyvsp[0].expr); }
break;
case 8:
#line 147 "SoCalcParse.y"
{ SoCalc_yyval.expr = SoCalc_yyvsp[0].expr; }
break;
case 9:
#line 148 "SoCalcParse.y"
{ SoCalc_yyval.expr = SoCalc_yyvsp[0].expr; }
break;
case 10:
#line 149 "SoCalcParse.y"
{ SoCalc_yyval.expr = SoCalc_yyvsp[0].expr; }
break;
case 11:
#line 150 "SoCalcParse.y"
{ SoCalc_yyval.expr = SoCalc_yyvsp[0].expr; }
break;
case 12:
#line 151 "SoCalcParse.y"
{ SoCalc_yyval.expr = SoCalc_yyvsp[-1].expr; }
break;
case 13:
#line 155 "SoCalcParse.y"
{ SoCalc_yyval.expr = SoCalc_yyvsp[0].expr; }
break;
case 14:
#line 157 "SoCalcParse.y"
{ SoCalc_yyval.expr = new Index(SoCalc_yyvsp[-3].expr, SoCalc_yyvsp[-1].expr); }
break;
case 15:
#line 158 "SoCalcParse.y"
{ ((Func *)SoCalc_yyvsp[-3].expr)->setArgs(SoCalc_yyvsp[-1].list); SoCalc_yyval.expr = SoCalc_yyvsp[-3].expr; }
break;
case 16:
#line 162 "SoCalcParse.y"
{ (SoCalc_yyval.list = new ExprList)->append(SoCalc_yyvsp[0].expr); }
break;
case 17:
#line 163 "SoCalcParse.y"
{ SoCalc_yyvsp[-2].list->append(SoCalc_yyvsp[0].expr); SoCalc_yyval.list = SoCalc_yyvsp[-2].list; }
break;
case 19:
#line 168 "SoCalcParse.y"
{ SoCalc_yyval.expr = new Negate(SoCalc_yyvsp[0].expr); }
break;
case 20:
#line 169 "SoCalcParse.y"
{ SoCalc_yyval.expr = new Not(SoCalc_yyvsp[0].expr); }
break;
case 22:
#line 175 "SoCalcParse.y"
{ SoCalc_yyval.expr = new Mult(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 23:
#line 177 "SoCalcParse.y"
{ SoCalc_yyval.expr = new Divide(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 24:
#line 179 "SoCalcParse.y"
{ SoCalc_yyval.expr = new Mod(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 26:
#line 185 "SoCalcParse.y"
{ SoCalc_yyval.expr = new Plus(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 27:
#line 187 "SoCalcParse.y"
{ SoCalc_yyval.expr = new Minus(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 29:
#line 193 "SoCalcParse.y"
{ SoCalc_yyval.expr = new LessThan(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 30:
#line 195 "SoCalcParse.y"
{ SoCalc_yyval.expr = new GreaterThan(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 31:
#line 197 "SoCalcParse.y"
{ SoCalc_yyval.expr = new LessEQ(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 32:
#line 199 "SoCalcParse.y"
{ SoCalc_yyval.expr = new GreaterEQ(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 34:
#line 205 "SoCalcParse.y"
{ SoCalc_yyval.expr = new Equals(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 35:
#line 207 "SoCalcParse.y"
{ SoCalc_yyval.expr = new NotEquals(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 37:
#line 213 "SoCalcParse.y"
{ SoCalc_yyval.expr = new And(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 39:
#line 219 "SoCalcParse.y"
{ SoCalc_yyval.expr = new Or(SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
case 41:
#line 226 "SoCalcParse.y"
{ SoCalc_yyval.expr = new Ternary(SoCalc_yyvsp[-4].expr, SoCalc_yyvsp[-2].expr, SoCalc_yyvsp[0].expr); }
break;
#line 956 "y.tab.c"
    }
    SoCalc_yyssp -= SoCalc_yym;
    SoCalc_yystate = *SoCalc_yyssp;
    SoCalc_yyvsp -= SoCalc_yym;
    SoCalc_yym = SoCalc_yylhs[SoCalc_yyn];
    if (SoCalc_yystate == 0 && SoCalc_yym == 0)
    {
#if YYDEBUG
        if (SoCalc_yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        SoCalc_yystate = YYFINAL;
        *++SoCalc_yyssp = YYFINAL;
        *++SoCalc_yyvsp = SoCalc_yyval;
        if (SoCalc_yychar < 0)
        {
            if ((SoCalc_yychar = SoCalc_yylex()) < 0) SoCalc_yychar = 0;
#if YYDEBUG
            if (SoCalc_yydebug)
            {
                SoCalc_yys = 0;
                if (SoCalc_yychar <= YYMAXTOKEN) SoCalc_yys = SoCalc_yyname[SoCalc_yychar];
                if (!SoCalc_yys) SoCalc_yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, SoCalc_yychar, SoCalc_yys);
            }
#endif
        }
        if (SoCalc_yychar == 0) goto SoCalc_yyaccept;
        goto SoCalc_yyloop;
    }
    if ((SoCalc_yyn = SoCalc_yygindex[SoCalc_yym]) && (SoCalc_yyn += SoCalc_yystate) >= 0 &&
            SoCalc_yyn <= YYTABLESIZE && SoCalc_yycheck[SoCalc_yyn] == SoCalc_yystate)
        SoCalc_yystate = SoCalc_yytable[SoCalc_yyn];
    else
        SoCalc_yystate = SoCalc_yydgoto[SoCalc_yym];
#if YYDEBUG
    if (SoCalc_yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *SoCalc_yyssp, SoCalc_yystate);
#endif
    if (SoCalc_yyssp >= SoCalc_yysslim && SoCalc_yygrowstack())
    {
        goto SoCalc_yyoverflow;
    }
    *++SoCalc_yyssp = SoCalc_yystate;
    *++SoCalc_yyvsp = SoCalc_yyval;
    goto SoCalc_yyloop;
SoCalc_yyoverflow:
    SoCalc_yyerror("yacc stack overflow");
SoCalc_yyabort:
    return (1);
SoCalc_yyaccept:
    return (0);
}
