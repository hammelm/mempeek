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

#include "mempeek_ast.h"
#include "mempeek_exceptions.h"
#include "console.h"

#if defined( YYDEBUG ) && YYDEBUG != 0
#include "mempeek_parser.h"
#include "parser.h"
#endif

#include <iostream>

#include <string.h>
#include <signal.h>

using namespace std;


static void signal_handler( int )
{
    ASTNode::set_terminate();
}

static void parse( const char* str, bool is_file )
{
    ASTNode::clear_terminate();
    signal( SIGABRT, signal_handler );
    signal( SIGINT, signal_handler );
    signal( SIGTERM, signal_handler );

    try {
        ASTNode::ptr yyroot = ASTNode::parse( str, is_file );

#ifdef ASTDEBUG
		cout << "executing ASTNode[" << yyroot << "]" << endl;
#endif
		yyroot->execute();
    }
    catch( ASTExceptionBreak& ) {
        // nothing to do
    }
    catch( ASTExceptionTerminate& ) {
        cout << endl << "terminated execution" << endl;
    }
    catch( const ASTCompileException& ex ) {
        cerr << ex.get_location() << "compile error: " << ex.what() << endl;
    }
    catch( const ASTRuntimeException& ex ) {
        cerr << ex.get_location() << "runtime error: " << ex.what() << endl;
    }
    catch( ... ) {
        signal( SIGABRT, SIG_DFL );
        signal( SIGINT, SIG_DFL );
        signal( SIGTERM, SIG_DFL );

        throw;
    }

    signal( SIGABRT, SIG_DFL );
    signal( SIGINT, SIG_DFL );
    signal( SIGTERM, SIG_DFL );
}

int main( int argc, char** argv )
{
#if defined( YYDEBUG ) && YYDEBUG != 0
    yydebug = 1;
#endif

    Console console( "mempeek", "~/.mempeek_history" );

    MMap::enable_signal_handler();

    try {
        bool is_interactive = false;
        bool has_commands = false;

        for( int i = 1; i < argc; i++ )
        {
            if( strcmp( argv[i], "-i" ) == 0 ) is_interactive = true;
            else if( strcmp( argv[i], "-I" ) == 0 ) {
                if( ++i >= argc ) {
                    cerr << "missing include path" << endl;
                    break;
                }

                ASTNode::add_include_path( argv[i] );
            }
            else if( strcmp( argv[i], "-c" ) == 0 ) {
                if( ++i >= argc ) {
                    cerr << "missing command" << endl;
                    break;
                }
                // TODO: parser should treat EOF as end of statement
                string cmd = string( argv[i] ) + '\n';

                parse( cmd.c_str(), false );
                has_commands = true;
            }
            else {
                parse( argv[i], true );
                has_commands = true;
            }
        }

        if( is_interactive || !has_commands ) {
            for(;;) {
                string line = console.get_line();
                parse( line.c_str(), false );
            }
        }
    }
    catch( ASTExceptionQuit ) {
        // nothing to do
    }

    return 0;
}
