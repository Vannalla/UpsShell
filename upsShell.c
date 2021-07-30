// By Logan with help from Ellis, Kyler, StackOverFlow, GeeksForGeeks, GitHub, Tutorialspoint and youtube

// BUGS
//- first line sometimess doesnt run ? but if you try to run it again in the next line it works. Shows rarely. 

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

typedef int bool;

#define true 1 // set def for true  
#define false 0 // set def for false
#define UPS_RL_BUFSIZE 1024 // buffer size used for reading user input 
#define UPS_TOK_BUFSIZE 64 // buffer size used for splitting the arguments 
#define HISTORY_LEN 100 // buffer size used for storing history of commands 
#define UPS_TOK_DELIM " \t\r\n\a" // delimiters for parsing the arguments ? 
#define EXIT_FAILURE 1// built in failure exit macros
#define EXIT_SUCCESS 0// built in success exit macros

bool conc = false; // global variable to check parent and child process concurrency 
int cur_pos = -1; // global variable to point to the last command executed 
char *history[HISTORY_LEN]; // global variable storing the history of commands executed 
int cur_bufsize = UPS_TOK_BUFSIZE; // global variable that hold various sizes from the above defined UPS BUFFSIZE

// Function declarations for built-in shell commands 
int ups_cd(char **args);
int ups_exit(char **args);
int ups_mypwd(char **args);

// Function declaration to remove warning with built ins
pid_t waitpid(); 
void exit();	 

// List of built-in commands
char *builtin_str[] = {
    "cd",
    "exit",
    "mypwd"
};

// List of built in commands and their corresponding functions 
int (*builtin_func[]) (char **) = {
    &ups_cd,
    &ups_exit,
    &ups_mypwd
};

// Function to grab the total number of built ins 
int ups_num_builtins(){
    return sizeof(builtin_str) / sizeof(char *);
}

// Built in function implementations 
int ups_cd(char **args){
    if(args[1] == NULL){ // make sure its has all the args required 
        fprintf(stderr, "ups: expected argument to \"cd\"\n");
    }else{
        if(chdir(args[1]) != 0){ // check to see if it failed and pushed error
            perror("ups: unexpected error");
        }
    }
    return 1; // return one to keep our shell loop going
}

// Exit help function that returns zero to end the ups loop which ends the main method
int ups_exit(char **args){
    return 0; // return 0 in order to end the main loop
}

// Function to print current directory
int ups_mypwd(char **args){
	char cwd[128];
    getcwd(cwd, 128); // making an array and storing the cwd path into it
    printf("%s\n", cwd);
    return 1; // return one to keep our shell loop going
}

// Launch a program 
int ups_launch(char **args){
    pid_t pid, wpid; // need for forks 
    int status;		 // variable to hold wether or not we are running 

    pid = fork();
    if(pid == 0){ // child process 
        if(execvp(args[0], args) == -1) perror("UPS: unexpected error");
        exit(EXIT_FAILURE);
    }else if(pid > 0){ // parent process 
        if(!conc){
            do{
                wpid =  waitpid(pid, &status, WUNTRACED);
            }while(!WIFEXITED(status) && !WIFSIGNALED(status)); // built in methods with pid_t
        }
    }else{ // else their is an error forking 
        perror("UPS: unexpected error");
    }
    conc = false; // change our global variable
    return 1; // return one to keep our shell loop going
}

// Parse input to get the arguments 
char **ups_split_line(char *line){
    cur_bufsize = UPS_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(cur_bufsize * sizeof(char*));
    char *token;
    if(!tokens){
        fprintf(stderr, "UPS: allocation error\n"); // stderr standard error with printf
        exit(EXIT_FAILURE);
    }
    token = strtok(line, UPS_TOK_DELIM);
    while(token != NULL){
        tokens[position] = token;
        position++;
        if(position >= cur_bufsize){
            cur_bufsize += UPS_TOK_BUFSIZE;
            tokens = realloc(tokens, cur_bufsize * sizeof(char*));
            if(!tokens){
                fprintf(stderr, "UPS: allocation error\n"); // stderr standard error with printf
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, UPS_TOK_DELIM);
    }
    if(position > 0 && strcmp(tokens[position - 1], "&") == 0) {
        conc = true; // change global variable so we can enter the right loop in launch
        tokens[position - 1] = NULL;
    }
    tokens[position] = NULL;
    return tokens;
}

// History of commands to 100, can change the constant HISTORY_LEN at the top to inc/dec
int ups_history(char **args){
    if(cur_pos == -1 || history[cur_pos] == NULL){ // making sure it isnt emtpy
        fprintf(stderr, "No commands in history\n");
    }
    else if(strcmp(args[0], "history") == 0){ // making sure we were actually passed history
        int last_pos = 0, position = cur_pos, count = 0;
        if(cur_pos != HISTORY_LEN && history[cur_pos + 1] != NULL){
            last_pos = cur_pos + 1;
        }
        count = (cur_pos - last_pos + HISTORY_LEN) % HISTORY_LEN + 1; 
        while(count > 0){
            char *command = history[position];
            printf("%d %s\n", count, command);
            position = position - 1;
            position = (position + HISTORY_LEN) % HISTORY_LEN; // by modding we can reverse the order so we print from 100 down instead of 1 down in the terminal
            count --;
        }
    }
    else{
            perror("UPS: unexpected error");
        }
    }


// Execute the parsed arguments 
int ups_execute(char *line){
    int i;
    char **args = ups_split_line(line);
    if(args[0] == NULL){ // empty command was entered 
        return 1;
    }else if(strcmp(args[0], "history") == 0){
        return ups_history(args);
    }
    cur_pos = (cur_pos + 1) % HISTORY_LEN; // by modding we can reverse the order so we print from 100 down instead of 1 down in the terminal
    history[cur_pos] = malloc(cur_bufsize * sizeof(char));
    char **temp_args = args;
    int count=0;
    while(*temp_args != NULL){
        strcat(history[cur_pos], *temp_args);
        strcat(history[cur_pos], " ");
        temp_args++;
    }
    //printf("%s\n", history[cur_pos]);
    //printf("Inserted %s\n", history[cur_pos-1]); test for if history method is working
    for (i = 0; i < ups_num_builtins(); i++){
        if(strcmp(args[0], builtin_str[i]) == 0){
            return (*builtin_func[i])(args);
        }
    }
    return ups_launch(args);
}

// Read input from stdin 
char *ups_read_line(void){
    cur_bufsize = UPS_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * cur_bufsize);
    int c;
    if(!buffer){
        fprintf(stderr, "UPS: allocation error\n");
        exit(EXIT_FAILURE);
    }
    while(1){
        c = getchar(); // Read a character
        if(c == EOF || c == '\n'){ // E0F is the end of the file
            buffer[position] = '\0';
            return buffer;
        }else{
            buffer[position] = c;
        }
        position++;
        if(position >= cur_bufsize){ // If buffer exceeded, reallocate buffer 
            cur_bufsize += UPS_RL_BUFSIZE;
            buffer = realloc(buffer, cur_bufsize);
            if(!buffer){
                fprintf(stderr, "UPS: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

// Loop for getting input and executing it 
void ups_loop(void){
    char *line;
    int status;
    do {
        printf("UPS>");
        line = ups_read_line();
        status = ups_execute(line);

        free(line);
    } while(status);
}

int main(void){
    ups_loop(); //loops to querey user for input in the terminal
    return EXIT_SUCCESS;
}