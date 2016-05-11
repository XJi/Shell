
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#include "command.h"


//Define ambiguous redirect variable
extern "C" void disp( int sig )
{
    /**if(ctrl-c){
        printf("\n");
        Command::_currentCommand.prompt();
    }**/
    printf("\n");
    Command::_currentCommand.prompt();
    //handle zombie process (another signal integer)
   /** {
        
    }**/
}
void killzombie( int sig ){
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}
#define ARGLENGTH 50
void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
    
	int index = 0;
	char * totalArg = (char *) malloc (ARGLENGTH );
	int nNewArg = 0;
    int argLen = strlen(argument);
	// ${C}p${D}
	for (int i = 0; i < argLen ; i++) {
		if (argument[i] == '$' && argument[i+1] == '{') {
            char * argEnv = (char *) malloc ( ARGLENGTH );
            char * newArg = (char *) malloc (ARGLENGTH);
			i = i + 2;
			while (argument[i] != '}') {
				newArg[index++] = argument[i++];
			}
			
			newArg[index] = '\0';
			argEnv = getenv(newArg);
			strcat(totalArg,argEnv );
			nNewArg = nNewArg + strlen(argEnv);
			index = 0;
		}
		else {
			totalArg[nNewArg++] = argument[i];
		}
	}
    _arguments[ _numberOfArguments ] = strdup(totalArg);
	_arguments[ _numberOfArguments + 1] = NULL;
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
    _append = 0;
	_background = 0;

}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile  && _outFile!=_errFile) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
    _append = 0;
	_background = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}


void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
	//print();
    int tmpin = dup(0);
    int tmpout = dup(1);
    int tmperr = dup(2);
    
    int fdin = 0;
    int fdout = 1;
    int fderr = 2;
    
    if(_inputFile){
        fdin = open(_inputFile, O_RDONLY, 0666);
    }
    else{
        fdin=dup(tmpin);
    }
    if(fdin < 0){
        perror("open");
        exit(1);
    }
    int ret;
    int fdpipe[2] = {-1, -1};
    for( int i =0; i < _numberOfSimpleCommands; i++) {
        dup2(fdin,0);
        close(fdin);
        
        //Ignore CTRL-C
        if ( !strcmp(_simpleCommands[i] -> _arguments[0] , "exit" ) ) {
			exit( 1 );
		}
        

        
        //Set environment variables
        else if(!strcmp(_simpleCommands[i] -> _arguments[0] , "setenv" ) ){
            /**if(# of argnum == 0){
                print man page
             if it's one, set it to empty
             otherwise do the following
            }
             
             **/
            if(_simpleCommands[i]->_numberOfArguments == 1){
                printf("\n");
            }
            else{
                setenv(_simpleCommands[i]->_arguments[1], _simpleCommands[i]->_arguments[2],1);
            }
            clear();
            prompt();
            return;
        }
    
        //Unset environment variables
        else if(!strcmp(_simpleCommands[i] -> _arguments[0] , "unsetenv" ) ){
            unsetenv(_simpleCommands[i]->_arguments[1]);
            return;
        }
        else if(!strcmp(_simpleCommands[i] -> _arguments[0] , "cd" )){
            int retval = 0;
            if(_simpleCommands[i] -> _arguments[1]!= NULL){
                retval = chdir(_simpleCommands[0] -> _arguments[1]);
            }
            else {
                retval = chdir(getenv("HOME"));
            }
            if(retval < 0){
                perror("directory");
                exit(2);
            }
            clear();
            prompt();
            return;
        }

        //open the file for redirecting output
        else {
            if(i == _numberOfSimpleCommands -1){
            if(_outFile ){
                if(_append)
                    fdout = open(_outFile, O_WRONLY|O_CREAT|O_APPEND, 0666);
                else fdout = open(_outFile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
                if(fdout < 0){
                    perror("open");
                    exit(1);
                }
            }
            else{
                fdout = dup(tmpout);
            }
            if(_errFile){
                if(_append)
                    fderr = open(_errFile, O_WRONLY|O_CREAT|O_APPEND, 0666);
                else fderr = open(_errFile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
            }
            else{
                fderr = dup(tmperr);
            }
            dup2(fderr,2);
            close(fderr);
            dup2(fdout,1);
            close(fdout);
        }
         // end of opening the redirecting file
        else{
            if( pipe(fdpipe) == -1){
                perror("pipe");
                exit(2);
            }
            fdout=fdpipe[1];
            fdin=fdpipe[0];
        }
        dup2(fdout,1);
        close(fdout);
        ret = fork();
        if(ret == 0){
            //3. printenv
           if ( !strcmp(_simpleCommands[i] -> _arguments[0] , "printenv" ) ) {
                for (char** var = environ; *var != NULL; ++var)
                    printf("%s\n", *var);
                exit(0);
            }
            //End of 3
            close(fdpipe[0]);
            close(fdpipe[1]);
            close( tmpin );
            close( tmpout );
            close( tmperr );
            execvp(_simpleCommands[i] -> _arguments[0], _simpleCommands[i] -> _arguments);
            perror("execvp");
            _exit(1);
        }
        if (ret < 0){
            perror("fork");
            exit(2);
        }
        }
    }

    dup2(tmpin,0);
    dup2(tmpout,1);
    dup2(tmperr,2);
    //if(fdpipe[0] != -1)
        close(fdpipe[0]);
    //if(fdpipe[1] != -1)
        close(fdpipe[1]);
    close(tmpin);
    close(tmpout);
    close(tmperr);
    if(!_background){
        waitpid(ret, 0,0);
    }

    
	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec
	// if ambiguous redirect is true; do exit()
	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
  if(isatty(0)){
	printf("myshell>");
	fflush(stdout);
  }
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

int main()
{
    struct sigaction signalAction; signalAction.sa_handler = disp;
    sigemptyset(&signalAction.sa_mask); signalAction.sa_flags = SA_RESTART;
    int error = sigaction(SIGINT, &signalAction, NULL );
    if ( error ) {
        perror( "sigaction" );
        exit( -1 );
    }
    struct sigaction signalAction2;
    signalAction2.sa_handler = killzombie;
    sigemptyset(&signalAction2.sa_mask); signalAction2.sa_flags = SA_RESTART;
    int error2 = sigaction(SIGCHLD, &signalAction2, NULL );
    if ( error2 )
    {
        perror( "sigaction" );
        exit( -1 );
    }
    
	Command::_currentCommand.prompt();
	yyparse();
}

