
/*
 * CS-252 Spring 2013
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD

%token 	NOTOKEN GREAT NEWLINE GREATGREATAMPERSAND GREATAMPERSAND GREATGREAT PIPE LESS AMPERSAND

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#include <stdio.h>
#include "command.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <regex.h>
#include <dirent.h>
#include <fstream>

void yyerror(const char * s);
int yylex();
void expandWildcard(char* prefix, char* suffix);

%}

%%

goal:	
	commands
	;

commands: 
	 commands command 
	| command
	;
command:
    simple_command
    |
	;
simple_command:	
	pipe_list iomodifier_opt1 background_optional NEWLINE {
		//printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE
	| error NEWLINE { yyerrok; }
	;
iomodifier_opt1:
    iomodifier_opt1 iomodifier_opt
    | iomodifier_opt
    |
    ;

pipe_list:
    pipe_list PIPE command_and_args
    | command_and_args
    ;
command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
	arg_list argument
	| 
	;

argument:
	WORD {
             //  printf("   Yacc: insert argument \"%s\"\n", $1);
             char* prefixChar = (char* )malloc(1);
             expandWildcard(prefixChar, $1);
	       //Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

command_word:
	WORD {
             //  printf("   Yacc: insert command \"%s\"\n", $1);
	       
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_opt:
/*add each if statement to each of them */
	GREAT WORD {
        if(Command::_currentCommand._outFile){
            printf("Ambiguous output redirect");
            exit(0);
        }
		//printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
	}
	| /* can be empty */
    GREATGREAT WORD {
        if(Command::_currentCommand._outFile){
            printf("Ambiguous output redirect");
            exit(0);
        }
      //  printf("    Yacc: insert append \"%s\"\n", $2);
        Command::_currentCommand._append = 1;
        Command::_currentCommand._outFile = $2;
    }
    |
    GREATGREATAMPERSAND WORD {
       // printf("    Yacc: insert input \"%s\"\n", $2);
       if(Command::_currentCommand._outFile){
           printf("Ambiguous output redirect");
           exit(0);
       }
        Command::_currentCommand._outFile = $2;
        Command::_currentCommand._errFile = $2;
        Command::_currentCommand._append = 1;
        
    }
    |
    GREATAMPERSAND WORD {
        if(Command::_currentCommand._outFile){
            printf("Ambiguous output redirect");
            exit(0);
        }
    	//printf("	Yacc: insert input \"%s\"\n", $2);
        Command::_currentCommand._outFile = $2;
        Command::_currentCommand._errFile = $2;
	}
    |
    LESS WORD {
    // printf("    Yacc: insert input \"%s\"\n", $2);
        Command::_currentCommand._inputFile = $2;
    }
    ;

background_optional:
	AMPERSAND {
		Command::_currentCommand._background = 1;			/*chang Command flag*/
	}
	|
	;
%%

#define MAXFILENAME 1024
void expandWildcard(char* prefix, char *suffix){
    if(suffix[0] == 0){
        //suffix is empty, Put prefix in argument.
	//printf("%s before \n ", prefix);
        //prefix++;
	//printf("after %s\n", prefix);
	Command::_currentSimpleCommand->insertArgument(strdup(prefix));
        /**Alternative: do sorting here**/
        return;
        
    }
    char *s = strchr(suffix, '/');
    char component[MAXFILENAME];
    if(s!= NULL){
        strncpy(component, suffix, s-suffix);
	//	printf("component %s\n", component);
        suffix = s+1;
    }
    else{
        strcpy(component, suffix);
        suffix = suffix + strlen(suffix);
    }
    //printf("%s component \n", component);
    char newPrefix[MAXFILENAME];
    if( strchr(component,'*') == NULL && strchr(component,'?') == NULL)/**component does not hace * or ?**/{
        sprintf(newPrefix, "%s\%s", prefix, component);
      //  printf("%s newPrefix, %s component\n", newPrefix, component);
	expandWildcard(newPrefix, suffix);
        return;
    }
    // Component has wildcards
    // Convert it to regular expression
    char * reg = (char*)malloc(2*strlen(component)+10);
    char * a = component;
    char * r = reg;
    *r = '^'; r++; // match beginning of line
    while (*a) {
        if (*a == '*') { *r='.'; r++; *r='*'; r++; }
        else if (*a == '?') { *r='.'; r++;}
        else if (*a == '.') { *r='\\'; r++; *r='.'; r++;}
        else { *r=*a; r++;}
        a++;
    }
    *r='$'; r++; *r=0;

    regex_t expression;
    int expbuf = regcomp(&expression, reg,0 );
    if (expbuf!= 0) {
        perror("compile");
        return;
    }
// 3. List directory and add as arguments the entries
// that match the regular expression
    // if prefix is empty, then list current directory
   char* dir = (char* )malloc(MAXFILENAME);
   
   if(prefix == NULL || prefix[0] == 0){/**prefix is empty**/
        *dir++ = '.';
        *dir = '\0';
    }
    else dir = strdup(prefix);
    //printf("dir is %s\n", dir);
    struct dirent ** ent;
    int count = scandir(dir, &ent, NULL, alphasort);
    if(count >= 0){
       for(int i =0; i < count; i++){
            regmatch_t pmatch[2];
            size_t nmatch = 2;
            if(regexec( &expression,ent[i]->d_name,nmatch,pmatch,0)==0){
                sprintf(newPrefix, "%s\%s", prefix, component);
                expandWildcard(newPrefix, suffix);
            }
         }
    }
    else return;
        
}


void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}
#if 0
main()
{
	yyparse();
}
#endif
