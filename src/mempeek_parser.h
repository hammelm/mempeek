#ifndef __mempeek_parser_h__
#define __mempeek_parser_h__

#include "mempeek_ast.h"

typedef void* yyscan_t;

typedef struct {
    std::string value = "";
    int token = 0;
    ASTNode* node = nullptr;
} yyvalue_t;


#endif // __mempeek_parser_h__
