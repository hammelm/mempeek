#include "console.h"

#include <sstream>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

using namespace std;


//////////////////////////////////////////////////////////////////////////////
// class Console implementation
//////////////////////////////////////////////////////////////////////////////

char* Console::s_Prompt = nullptr;
std::function<unsigned char( EditLine*, int )> Console::s_Completion = nullptr;


void Console::init( string name, string histfile, size_t histsize )
{
	// only one instance of Console is allowed
	if( s_Prompt ) abort();
	s_Prompt = strcpy( new char[3], "> " );

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
}

Console::~Console()
{
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
	s_Completion = completion;
}

string Console::get_line()
{
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

char* Console::get_prompt( EditLine* )
{
	return s_Prompt;
}

unsigned char Console::completion_callback( EditLine* el, int ch )
{
	if( s_Completion ) return s_Completion( el, ch );
	else return CC_NORM;
}
