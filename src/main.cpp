extern int yyparse();

extern int yydebug;

int main()
{
	yydebug = 1;

    yyparse();
    return 0;
}
