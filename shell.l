
/*
 *
 * CS-252 Fall 2013
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>
#include "y.tab.h"

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
    unput(c);
}

%}

%%  /*Why I have to add this "%" ??*/


\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}

"|"     {
        return PIPE;
}

"<"     {
    return LESS;
}

"&" {
    return AMPERSAND;
}

">" 	{
		return GREAT;
	}
">>"    {
    return GREATGREAT;
}
">>&"	{
	return GREATGREATAMPERSAND;
}
">&"	{
	return GREATAMPERSAND;
}

[^ \t\n][^ \t\n]*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}

%%

