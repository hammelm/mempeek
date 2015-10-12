#ifndef __mempeek_parser_h__
#define __mempeek_parser_h__

#include <string>

class ASTNode;


typedef void* yyscan_t;

typedef struct {
    std::string value = "";
    int token = 0;
    ASTNode* node = nullptr;
} yyvalue_t;

typedef struct {
	const char* file;
	int first_line;
	int last_line;
} yylloc_t;

#define YYLTYPE yylloc_t

# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).file         = YYRHSLOC (Rhs, 1).file;              \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).file         = YYRHSLOC (Rhs, 0).file;              \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
        }                                                               \
    while (0)


#endif // __mempeek_parser_h__
