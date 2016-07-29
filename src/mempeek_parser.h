/*  Copyright (c) 2015, Martin Hammel
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __mempeek_parser_h__
#define __mempeek_parser_h__

#include <string>
#include <vector>
#include <memory>
#include <utility>

class ASTNode;
class Environment;

typedef std::shared_ptr<ASTNode> yynodeptr_t;

typedef void* yyscan_t;

typedef std::vector< std::pair<yynodeptr_t,std::string> > arglist_t;

typedef struct {
    std::string value = "";
    int token = 0;
    yynodeptr_t node = nullptr;
    arglist_t arglist;
} yyvalue_t;

typedef Environment* yyenv_t;

typedef struct {
	std::string file;
	int first_line;
	int last_line;
} yylloc_t;

#define YYLTYPE yylloc_t

#define YYLLOC_DEFAULT(Current, Rhs, N)                                 \
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
