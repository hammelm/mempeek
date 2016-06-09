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

#include "console.h"

#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_EDITLINE
#include <fstream>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#else
#include <iostream>
#endif

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class Console implementation
//////////////////////////////////////////////////////////////////////////////

char* Console::s_Prompt = nullptr;


Console::Console( string name, string histfile, size_t histsize )
{
	// only one instance of Console is allowed
	if( s_Prompt ) abort();
	s_Prompt = strcpy( new char[3], "> " );

#ifdef USE_EDITLINE
	// create histfile name
	m_Histfile = histfile;
	if( histfile.length() > 0 && histfile[0] == '~' )  {
		passwd* pwd = getpwuid( getuid() );
		if( pwd ) {
			m_Histfile = pwd->pw_dir;
			m_Histfile += histfile.substr(1);
		}
	}

	// init history
	HistEvent hev;
	m_History = history_init();
	history( m_History, &hev, H_SETSIZE, histsize );
	history( m_History, &hev, H_SETUNIQUE, 1 );

	if( m_Histfile.length() > 0 ) {
		ifstream histfile( m_Histfile.c_str() );
		for(;;) {
			string line;
			std::getline( histfile, line );
			if( histfile.bad() || histfile.fail() ) break;
			line += '\n';
			history( m_History, &hev, H_ENTER, line.c_str() );
			if( histfile.eof() ) break;
		}
	}

	// init editline
	m_Editline = el_init( name.c_str(), stdin, stdout, stderr );
	el_set( m_Editline, EL_SIGNAL, 1 );
	el_set( m_Editline, EL_EDITOR, "emacs" );
	el_set( m_Editline, EL_PROMPT, Console::get_prompt );
	el_set( m_Editline, EL_HIST, history, m_History );
	el_set( m_Editline, EL_ADDFN, "ed-complete", "", completion_callback );
	el_set( m_Editline, EL_BIND, "^I", "ed-complete", nullptr );
#else
	(void)histfile;
	(void)histsize;
#endif
}

Console::~Console()
{
#ifdef USE_EDITLINE
	// save history
	if( m_Histfile.length() > 0 ) {
		ofstream histfile( m_Histfile.c_str(), ios::trunc );
		HistEvent hev;
		history( m_History, &hev, H_LAST );
		for(;;) {
			histfile << hev.str;
			if( history( m_History, &hev, H_PREV ) < 0 ) break;
		}
	}

	// cleanup
	el_end( m_Editline );
	history_end( m_History );
#endif

	delete[] s_Prompt;
	s_Prompt = nullptr;
}

void Console::set_prompt( string prompt )
{
	delete[] s_Prompt;
	s_Prompt = strcpy( new char[ prompt.length() + 1 ], prompt.c_str() );
}

void Console::set_completion( std::function<unsigned char( EditLine*, int )> completion )
{
#ifdef USE_EDITLINE
	s_Completion = completion;
#else
	(void)completion;
#endif
}

void Console::set_clientdata( void* data )
{
#ifdef USE_EDITLINE
    el_set( m_Editline, EL_CLIENTDATA, data );
#else
    (void)data;
#endif
}

string Console::get_line()
{
#ifdef USE_EDITLINE
	int count;
	const char* line = el_gets( m_Editline, &count );

	// do not add to history if line is empty
	if( line ) {
		for( const char* l = line; *l; l++ ) {
			if( !isspace(*l) ) {
				HistEvent hev;
				history( m_History, &hev, H_ENTER, line );
				return line;
			}
		}
	}

	return "";
#else
    cout << s_Prompt << flush;
    string ret = "";
    getline( cin, ret );
    ret += '\n';

    return ret;
#endif
}

Console::tokens_t Console::get_tokens()
{
	istringstream line( get_line() );

	tokens_t tokens;
	for(;;) {
		string token;
		line >> token;
		if( line.bad() || line.fail() ) return tokens;
		tokens.push_back( token );
		if( line.eof() ) return tokens;
	}
}

#ifdef USE_EDITLINE

std::function<unsigned char( EditLine*, int )> Console::s_Completion = nullptr;

char* Console::get_prompt( EditLine* )
{
	return s_Prompt;
}

unsigned char Console::completion_callback( EditLine* el, int ch )
{
	if( s_Completion ) return s_Completion( el, ch );
	else return CC_NORM;
}

#endif
