//Program that compiles and runs multiple programs written with EBNF type syntax
//Works right using one or two threads

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define SUCCESS 1
#define FAIL 99
#define F 3
#define T 2
#define RUNNING 4
#define BLOCKED 5
#define KILLED 6
#define FINISHED 7
#define READY 8
#define BRANCH 9

typedef struct{
    char buff[50];
    int pos;
    int check;
}init;

typedef struct{
    char name[20];
    int value[30];
	int queue[20];
    int counter;
}int_variable;

typedef struct{
    char name[20];
    int count_char;
}label;

typedef struct{
    char name[20];
    int state;
    int label_len, new_var_pos;
    int count_char, thread;
    int first;
    int_variable *var;
    label *labels;
}process;

typedef struct{
    int pos, pos_to_pos;
}var_return;

pthread_mutex_t monitor_exec, monitor_run;
pthread_cond_t *cond_start, *cond_finish;
init *t_argums=NULL;
int pid=0, *check_start, *check_finish, new_globalVar_pos=0, nofcom=3, N;
int_variable *globalVar;
process *processes=NULL;

//search if a variable exists
int search(int_variable *var, int var_pos, char name[10]){
    int i;
    for(i=0; i<var_pos; i++){
        if(strcmp(var[i].name, name) == 0){
            return i;
        }
    }
    return (-1);
}

//search if label exists
int searchL(label *labels, int label_pos, char name[20]){
    int i;
    for(i=0; i<label_pos; i++){
        if(strcmp(labels[i].name, name) == 0){
            return i;
        }
    }
    return (-1);
}

int readVarVal(char string[20],  int *new_var_pos, int_variable **var){
	int value, var_pos, i, check=0, pos, temp;
	char tabletemp[10], postemp[10];

    if(string[0]!= '$'){ //if value
		if(string[0]!= '-'){
			i=0;
			while(string[i] != '\0'){
				if(isdigit(string[i]) == 0){
					printf("Wrong syntax1\n");
                    return FAIL;
                }
                i++;
            }
            value=atoi(string);
        }
        else{
			i=1;
            while(string[i] != '\0'){
                if(isdigit(string[i]) == 0){
                    printf("Wrong syntax2");
                    return FAIL;
                }
                i++;
            }
            value=atoi(string+1);
            value=0-value;
        }
	}
	else{ //if variable
		for(i=1; i<strlen(string); i++){
			if(string[i]=='['){
				check=1;
				temp=i;
			}
			else if(string[i]==']' && check==1){
				check=2;
				break;
			}
		}
		if(check==2){ //if table
			for(i=0; i<temp; i++){
				tabletemp[i]=string[i];
			}
			tabletemp[i]='\0';
			for(i=temp+1; i<strlen(string)-1; i++){
				postemp[i-temp-1]=string[i];
			}
			postemp[i-temp-1]='\0';
			pos=readVarVal(postemp, new_var_pos, var);
			
			var_pos=search(*var, *new_var_pos, tabletemp);
			if(var_pos == -1){
				*var=(int_variable *)realloc(*var, (*new_var_pos+1)*sizeof(int_variable));
				if(var == NULL){
					perror("Creating new var");
					return FAIL;
				}
				strcpy((*var)[*new_var_pos].name, tabletemp);
				(*var)[*new_var_pos].value[pos]=0;
				var_pos=*new_var_pos;
				*new_var_pos=*new_var_pos+1;
			}
			value=(*var)[var_pos].value[pos];
		}
		else{ //if not table
			var_pos=search(*var, *new_var_pos, string);
			value=(*var)[var_pos].value[0];
		}
	}
	return value;
}

var_return readVar(char string[20], int *new_var_pos, int_variable **var){
    int var_pos, i, check=0, pos, temp;
	char tabletemp[10], postemp[10];
	var_return ret;

    if(string[0]!= '$'){
        printf("Wrong syntax\n");
        return ret;
    }
    for(i=1; i<strlen(string); i++){
		if(string[i]=='['){
			check=1;
			temp=i;
		}
		else if(string[i]==']' && check==1){
			check=2;
			break;
		}
	}
	if(check==2){ //if table
		for(i=0; i<temp; i++){
			tabletemp[i]=string[i];
		}
		tabletemp[i]='\0';
		for(i=temp+1; i<strlen(string)-1; i++){
			postemp[i-temp-1]=string[i];
		}
		postemp[i-temp-1]='\0';
		pos=readVarVal(postemp, new_var_pos, var);
		
		var_pos=search(*var, *new_var_pos, tabletemp);
		if(var_pos == -1){
			*var=(int_variable *)realloc(*var, (*new_var_pos+1)*sizeof(int_variable));
			if(var == NULL){
				perror("Creating new var");
				return ret;
			}
			strcpy((*var)[*new_var_pos].name, tabletemp);
			(*var)[*new_var_pos].value[pos]=0;
			var_pos=*new_var_pos;
			*new_var_pos=*new_var_pos+1;
		}
		ret.pos=var_pos;
		ret.pos_to_pos=pos;
	}
	else{
		var_pos=search(*var, *new_var_pos, string);
		if(var_pos == -1){
			*var=(int_variable *)realloc(*var, (*new_var_pos+1)*sizeof(int_variable));
			if(var == NULL){
				perror("Creating new var");
				return ret;
			}
			strcpy((*var)[*new_var_pos].name, string);
			(*var)[*new_var_pos].value[0]=0;
			var_pos=*new_var_pos;
			*new_var_pos=*new_var_pos+1;
		}
		ret.pos=var_pos;
		ret.pos_to_pos=0;
	}
	return ret;
}

var_return readGlobalVar(char string[20], int *new_var_pos, int_variable **var){
    int globalVar_pos,i, check=0, pos=0, temp;
    char tabletemp[10],postemp[10];
    var_return ret;

    if(string[0]!= '$'){
        printf("Wrong syntax\n");
        ret.pos=FAIL;
        return ret;
    }
    //check if table
    for(i=1; i<strlen(string); i++){
        if(string[i]=='['){
            check=1;
            temp=i;
        }
        else if(string[i]==']' && check==1){
            check=2;
            break;
        }
    }
    if(check==2){ //table
        for(i=0; i<temp; i++){
            tabletemp[i]=string[i];
        }
        tabletemp[i]='\0';
        for(i=temp+1; i<strlen(string)-1; i++){
            postemp[i-temp-1]=string[i];
        }
        postemp[i-temp-1]='\0';
        pos=readVarVal(postemp, new_var_pos, var);

        globalVar_pos=search(globalVar, new_globalVar_pos, tabletemp);
        if(globalVar_pos == -1){
            globalVar=(int_variable *)realloc(globalVar, (new_globalVar_pos+1)*sizeof(int_variable));
            if(globalVar == NULL){
                perror("Creating new globalVar");
                ret.pos=FAIL;
                return ret;
            }
            strcpy(globalVar[new_globalVar_pos].name, tabletemp);
            globalVar[new_globalVar_pos].value[pos]=0;
            ret.pos=new_globalVar_pos;
            ret.pos_to_pos=pos;
            new_globalVar_pos++;
        }
        else{
            ret.pos=globalVar_pos;
            ret.pos_to_pos=pos;
        }
    }
    else{
        globalVar_pos=search(globalVar, new_globalVar_pos, string);
        if(globalVar_pos == -1){
            globalVar=(int_variable *)realloc(globalVar, (new_globalVar_pos+1)*sizeof(int_variable));
            if(globalVar == NULL){
                perror("Creating new globalVar");
                ret.pos=FAIL;
                return ret;
            }
            strcpy(globalVar[new_globalVar_pos].name, string);
            globalVar[new_globalVar_pos].value[0]=0;
            globalVar_pos=new_globalVar_pos;
            new_globalVar_pos++;
            ret.pos=globalVar_pos;
            ret.pos_to_pos=0;
        }
    }
    return ret;
}

int load(char buffline[100], int_variable **var, int *new_var_pos, int pos){
    char string[20], space;
    var_return ret_var, ret_globalVar;

    //read var
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos=pos+1;
        sscanf(buffline+pos, "%c", &space);
    }

    ret_var=readVar(string, new_var_pos, var);

    //read globalVar
    sscanf(buffline+pos, "%s", string);

    ret_globalVar=readGlobalVar(string,new_var_pos,var);

    //execute LOAD
    (*var)[ret_var.pos].value[ret_var.pos_to_pos]=globalVar[ret_globalVar.pos].value[ret_globalVar.pos_to_pos];

    return SUCCESS;
}

int store(char buffline[100], int_variable **var, int *new_var_pos, int pos){
    int value;
    char string[20], space;
    var_return ret;

    //read globalVar
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos=pos+1;
        sscanf(buffline+pos, "%c", &space);
    }

    ret=readGlobalVar(string,new_var_pos,var);

    //read varVal
    sscanf(buffline+pos, "%s", string);

    value=readVarVal(string,new_var_pos,var);
    //execute STORE
    globalVar[ret.pos].value[ret.pos_to_pos]=value;

    return SUCCESS;
}

int set(char buffline[100], int_variable **var, int *new_var_pos, int pos){
    int var_pos, value;
    char string[20],space;
	var_return ret;

    //read var
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    ret=readVar(string,new_var_pos,var);

    //read varVal
    sscanf(buffline+pos, "%s", string);
    value=readVarVal(string,new_var_pos,var);

    //execute SET
    (*var)[ret.pos].value[ret.pos_to_pos]=value;
    printf(" ");

    return SUCCESS;
}

int add(char buffline[100], int_variable **var, int *new_var_pos, int pos){
    int value1, value2;
    char string[20], space;
	var_return ret;

    //read var
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    ret=readVar(string,new_var_pos,var);

    //read varVal1
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    value1=readVarVal(string,new_var_pos,var);

    //read varVal2
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    value2=readVarVal(string,new_var_pos,var);
    
    (*var)[ret.pos].value[ret.pos_to_pos]=value1+value2;

    return SUCCESS;
}

int sub(char buffline[100], int_variable **var, int *new_var_pos, int pos){
    int value1, value2;
    char string[20], space;
	var_return ret;

    //read var
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    ret=readVar(string,new_var_pos,var);

    //read varVal1
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    value1=readVarVal(string,new_var_pos,var);

    //read varVal2
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    value2=readVarVal(string,new_var_pos,var);

    (*var)[ret.pos].value[ret.pos_to_pos]=value1-value2;
   
    return SUCCESS;
}

int mul(char buffline[100], int_variable **var, int *new_var_pos, int pos){
    int  value1, value2;
    char string[20], space;
	var_return ret;

    //read var
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    ret=readVar(string,new_var_pos,var);

    //read varVal1
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    value1=readVarVal(string,new_var_pos,var);

    //read varVal2
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    value2=readVarVal(string,new_var_pos,var);

    (*var)[ret.pos].value[ret.pos_to_pos]=value1*value2;

    return SUCCESS;
}

int divide(char buffline[100], int_variable **var, int *new_var_pos, int pos){
    int value1, value2;
    char string[20], space;
	var_return ret;

    //read var
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    ret=readVar(string,new_var_pos,var);

    //read varVal1
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    value1=readVarVal(string,new_var_pos,var);

    //read varVal2
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    value2=readVarVal(string,new_var_pos,var);

    (*var)[ret.pos].value[ret.pos_to_pos]=value1/value2;

    return SUCCESS;
}

int mod(char buffline[100], int_variable **var, int *new_var_pos, int pos){
    int value1, value2;
    char string[20], space;
	var_return ret;

    //read var
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    ret=readVar(string,new_var_pos,var);

    //read varVal1
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    value1=readVarVal(string,new_var_pos,var);

    //read varVal2
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    value2=readVarVal(string,new_var_pos,var);

    (*var)[ret.pos].value[ret.pos_to_pos]=value1%value2;

    return SUCCESS;
}

int brgt(char buffline[100], int_variable **var, int *new_var_pos, int pos, label *labels, int new_label_pos, FILE **fd, int id){
    int value1, value2, label_pos;
    char string[20], space;

    //read varVal1
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }

    value1=readVarVal(string,new_var_pos,var);

    //read varVal2
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    value2=readVarVal(string,new_var_pos,var);

    //read label
    sscanf(buffline+pos, "%s", string);
    if(string[0] != 'L'){
        printf("Wrong syntax\n");
        return FAIL;
    }

    label_pos=searchL(labels, new_label_pos, string);
    if(label_pos == -1){
        printf("Label doesn't exist\n");
        return FAIL;
    }

    //execute BRGT
    if(value1 > value2){
        fseek(*fd,labels[label_pos].count_char,SEEK_SET);
        processes[id].count_char=labels[label_pos].count_char;
        return BRANCH;
    }

    return SUCCESS;
}

int brge(char buffline[100], int_variable **var, int *new_var_pos, int pos, label *labels, int new_label_pos, FILE **fd, int id){
    int value1, value2, label_pos;
    char string[20], space;

    //read varVal1
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }

    value1=readVarVal(string,new_var_pos,var);

    //read varVal2
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    value2=readVarVal(string,new_var_pos,var);

    //read label
    sscanf(buffline+pos, "%s", string);
    if(string[0] != 'L'){
        printf("Wrong syntax\n");
        return FAIL;
    }

    label_pos=searchL(labels, new_label_pos, string);
    if(label_pos == -1){
        printf("Label doesn't exist\n");
        return FAIL;
    }

    //execute BRGE
    if(value1 >= value2){
        fseek(*fd,labels[label_pos].count_char,SEEK_SET);
        processes[id].count_char=labels[label_pos].count_char;
        return BRANCH;
    }

    return SUCCESS;
}

int brlt(char buffline[100], int_variable **var, int *new_var_pos, int pos, label *labels, int new_label_pos, FILE **fd, int id){
    int value1, value2, label_pos;
    char string[20], space;

    //read varVal1
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }

    value1=readVarVal(string,new_var_pos,var);

    //read varVal2
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    value2=readVarVal(string,new_var_pos,var);

    //read label
    sscanf(buffline+pos, "%s", string);
    if(string[0] != 'L'){
        printf("Wrong syntax\n");
        return FAIL;
    }

    label_pos=searchL(labels, new_label_pos, string);
    if(label_pos == -1){
        printf("Label doesn't exist\n");
        return FAIL;
    }

    //execute BRLT
    if(value1 < value2){
        fseek(*fd,labels[label_pos].count_char,SEEK_SET);
        processes[id].count_char=labels[label_pos].count_char;
        return BRANCH;
    }

    return SUCCESS;
}

int brle(char buffline[100], int_variable **var, int *new_var_pos, int pos, label *labels, int new_label_pos, FILE **fd, int id){
    int value1, value2, label_pos;
    char string[20], space;

    //read varVal1
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }

    value1=readVarVal(string,new_var_pos,var);

    //read varVal2
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    value2=readVarVal(string,new_var_pos,var);

    //read label
    sscanf(buffline+pos, "%s", string);
    if(string[0] != 'L'){
        printf("Wrong syntax\n");
        return FAIL;
    }

    label_pos=searchL(labels, new_label_pos, string);
    if(label_pos == -1){
        printf("Label doesn't exist\n");
        return FAIL;
    }

    //execute BRLE
    if(value1 <= value2){
        fseek(*fd,labels[label_pos].count_char,SEEK_SET);
        processes[id].count_char=labels[label_pos].count_char;
        return BRANCH;
    }

    return SUCCESS;
}

int breq(char buffline[100], int_variable **var, int *new_var_pos, int pos, label *labels, int new_label_pos, FILE **fd, int id){
    int value1, value2, label_pos;
    char string[20], space;

    //read varVal1
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }

    value1=readVarVal(string,new_var_pos,var);

    //read varVal2
    sscanf(buffline+pos, "%s", string);
    pos=pos+strlen(string);
    //pass the spaces
    sscanf(buffline+pos, "%c", &space);
    while(space == ' ' || space == '\t'){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    value2=readVarVal(string,new_var_pos,var);

    //read label
    sscanf(buffline+pos, "%s", string);
    if(string[0] != 'L'){
        printf("Wrong syntax\n");
        return FAIL;
    }

    label_pos=searchL(labels, new_label_pos, string);
    if(label_pos == -1){
        printf("Label doesn't exist\n");
        return FAIL;
    }

    //execute BREQ
    if(value1 == value2){
        fseek(*fd,labels[label_pos].count_char,SEEK_SET);
        processes[id].count_char=labels[label_pos].count_char;
        return BRANCH;
    }

    return SUCCESS;
}

int bra(char buffline[100], int pos, label *labels, int new_label_pos, FILE **fd, int id){
    char string[20];
    int label_pos;

    //read label
    sscanf(buffline+pos, "%s", string);
    if(string[0] != 'L'){
        printf("Wrong syntax\n");
        return FAIL;
    }
    label_pos=searchL(labels, new_label_pos, string);
    if(label_pos == -1){
        printf("Label doesn't exist\n");
        return FAIL;
    }

    //execute BRA
    fseek(*fd,labels[label_pos].count_char,SEEK_SET);
        processes[id].count_char=labels[label_pos].count_char;

    return BRANCH;
}

int sleeping(char buffline[100], int_variable **var, int *new_var_pos, int pos){
    int value;
    char string[20];

    //read varVal
    sscanf(buffline+pos, "%s", string);
    value=readVarVal(string,new_var_pos,var);

    //execute SLEEP
    sleep(value);
    return SUCCESS;
}

int print(char buffline[100], int_variable **var, int *new_var_pos, int pos){
    int i=0, value;
    char string[50],space;

    sscanf(buffline+pos, "%c", &string[0]);
    if(string[0]=='"'){
        pos++;
        sscanf(buffline+pos, "%c", &string[i]);
        pos++;
        while(string[i]!='"'){
            i++;
            sscanf(buffline+pos, "%c", &string[i]);
            pos++;
            if(pos>=strlen(buffline)){
                printf("Wrong syntax1\n");
                return FAIL;
            }
        }
        i;
        string[i]='\0';
        printf("%s", string);
    }
    //pass the space
    sscanf(buffline+pos, "%c", &space);
    while(space == ' '){
        pos++;
        sscanf(buffline+pos, "%c", &space);
    }
    
    while(buffline[pos] != '\n' && pos<strlen(buffline) ){
        //read valval
        sscanf(buffline+pos, "%s", string);
         //pass the space
        sscanf(buffline+pos, "%c", &space);
        while(space == ' '){
            pos++;
            sscanf(buffline+pos, "%c", &space);
        }
        pos=pos+strlen(string);
        value=readVarVal(string,new_var_pos,var);
        if(value!=99){
            printf("%d ", value);
        }
        //pass the space
        sscanf(buffline+pos, "%c", &space);
        while(space == ' '){
            pos++;
            sscanf(buffline+pos, "%c", &space);
        }
     }

    printf("\n");

    return SUCCESS;
}

int sem_down(char buffline[100], int pos, int id){
    char string[20];
    int sem_pos,i,size;

    //read sem
    sscanf(buffline+pos, "%s", string);
    if(string[0]!= '$'){
    printf("Wrong syntax\n");
        return FAIL;
    }
    sem_pos=search(globalVar, new_globalVar_pos, string);
    if(sem_pos == -1){
        globalVar=(int_variable *)realloc(globalVar, (new_globalVar_pos+1)*sizeof(int_variable));
        if(globalVar == NULL){
            perror("Creating new globalVar");
            return FAIL;
        }
        strcpy(globalVar[new_globalVar_pos].name, string);
        sem_pos=new_globalVar_pos;
        new_globalVar_pos++;

        globalVar[sem_pos].value[0]=0;
        globalVar[sem_pos].counter=0;
    }
    //execute down
    globalVar[sem_pos].value[0]=globalVar[sem_pos].value[0]-1;
    if(globalVar[sem_pos].value[0]<0){
        globalVar[sem_pos].queue[globalVar[sem_pos].counter]=id;
        globalVar[sem_pos].counter=globalVar[sem_pos].counter+1;
    }
    return globalVar[sem_pos].value[0];
}

int sem_up(char buffline[100], int pos){
    char string[20];
    int sem_pos,temp, i,size;

    //read sem
    sscanf(buffline+pos, "%s", string);
    if(string[0]!= '$'){
    printf("Wrong syntax\n");
        return FAIL;
    }
    sem_pos=search(globalVar, new_globalVar_pos, string);
    if(sem_pos == -1){
        globalVar=(int_variable *)realloc(globalVar, (new_globalVar_pos+1)*sizeof(int_variable));
        if(globalVar == NULL){
            perror("Creating new globalVar");
            return FAIL;
        }
        strcpy(globalVar[new_globalVar_pos].name, string);
        sem_pos=new_globalVar_pos;
        new_globalVar_pos++;

        globalVar[sem_pos].value[0]=0;
        globalVar[sem_pos].counter=0;
    }
    else{
        if(globalVar[sem_pos].value[0]<0){
            processes[globalVar[sem_pos].queue[0]].state=READY;
            for(i=0;i<globalVar[sem_pos].counter;i++){
                globalVar[sem_pos].queue[i]=globalVar[sem_pos].queue[i+1];
            }
            globalVar[sem_pos].counter=globalVar[sem_pos].counter-1;
        }
    }

    //execute UP
    globalVar[sem_pos].value[0]=globalVar[sem_pos].value[0]+1;
    return  SUCCESS;

}

int find_labels(FILE *fd, label **labels){
    int i,label_len=0, pos, counter=0;
    char string[20], buff_line[100], space;

    while(1){
        if(fgets(buff_line, sizeof(buff_line), fd) == NULL){
            break;
        }
        pos=0;
        //pass the spaces
        sscanf(buff_line+pos, "%c", &space);
        while(space == ' '){
            pos++;
            sscanf(buff_line+pos, "%c", &space);
        }
        sscanf(buff_line+pos, "%s", string);
        pos=pos+strlen(string);
        //pass the spaces
        sscanf(buff_line+pos, "%c", &space);
        while(space == ' '){
            pos++;
            sscanf(buff_line+pos, "%c", &space);
        }

        //save labels
        if(string[0] == 'L' && strcmp(string,"LOAD") != 0){
            *labels=(label *)realloc(*labels, (label_len+1)*sizeof(label));
            strcpy((*labels)[label_len].name,string);
            (*labels)[label_len].count_char=counter;
            label_len++;
            sscanf(buff_line+pos, "%s", string);
        }

        counter=counter+strlen(buff_line);
    }

    return label_len;
}

int run_program(FILE **fd, FILE *label_fd, int *argum, int count_argum, int id){
    char command[20], space;
    char buff_line[100];
    int pos, label_len, new_var_pos=0, i, sem, j, k, p;
    int_variable *var=NULL;
    label *labels=NULL;

    if(processes[id].first == T){
        //pass arguments to the var table
        var=(int_variable *)realloc(var, 2*sizeof(int_variable));
        if(var == NULL){
			perror("Creating new var");
			processes[id].state=FINISHED;
			return FAIL;
		}
        strcpy(var[0].name, "$argc");
        var[0].value[0]=count_argum;
        strcpy(var[1].name, "$argv");
        for(i=0; i<count_argum; i++){
            var[1].value[i]=argum[i];
        }
        new_var_pos=i;
        label_len=find_labels(label_fd, &labels);
    }
    else{
        var=processes[id].var;
        labels=processes[id].labels;
        label_len=processes[id].label_len;
        new_var_pos=processes[id].new_var_pos;
    }

    processes[id].first=F;
    processes[id].state=RUNNING;
    for(j=0;j<nofcom;j++){
        if(processes[id].state!=KILLED){
           if(fgets(buff_line, sizeof(buff_line), *fd) == NULL){
                printf("No RETURN command\n");
                processes[id].state=FINISHED;
                return FAIL;
            }
            pos=0;

            //pass the spaces
            sscanf(buff_line+pos, "%c", &space);
            while(space == ' ' || space == '\t'){
                pos++;
                sscanf(buff_line+pos, "%c", &space);
            }

            //read and execute commands
            sscanf(buff_line+pos, "%s", command);
            pos=pos+strlen(command);
            //pass the spaces
            sscanf(buff_line+pos, "%c", &space);
            while(space == ' ' || space == '\t'){
                pos++;
                sscanf(buff_line+pos, "%c", &space);
            }

            if(command[0] == 'L' && strcmp(command,"LOAD") != 0){
                p=searchL(labels, label_len, command);
                processes[id].count_char=labels[p].count_char;
                sscanf(buff_line+pos, "%s", command);
                pos=pos+strlen(command);

                //pass the spaces
                sscanf(buff_line+pos, "%c",&space);
                while(space == ' ' || space == '\t'){
                    pos++;
                    sscanf(buff_line+pos, "%c", &space);
                }
            }
            if(strcmp(command,"LOAD")==0){
                if (load(buff_line, &var, &new_var_pos, pos) == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"STORE")==0){
                if (store(buff_line, &var, &new_var_pos, pos) == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"SET")==0){
                if (set(buff_line, &var, &new_var_pos, pos) == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"ADD")==0){
                if (add(buff_line, &var, &new_var_pos, pos) == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"SUB")==0){
                if (sub(buff_line, &var, &new_var_pos, pos) == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"MUL")==0){
                if (mul(buff_line, &var, &new_var_pos, pos) == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"DIV")==0){
                if (divide(buff_line, &var, &new_var_pos, pos) == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"MOD")==0){
                if (mod(buff_line, &var, &new_var_pos, pos) == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"BRGT")==0){
                k=brgt(buff_line, &var, &new_var_pos, pos, labels, label_len, fd, id);
                if(k == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"BRGE")==0){
                k=brge(buff_line, &var, &new_var_pos, pos, labels, label_len, fd, id);
                if(k == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"BRLT")==0){
                k=brlt(buff_line, &var, &new_var_pos, pos, labels, label_len, fd, id);
                if(k == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"BRLE")==0){
                k=brle(buff_line, &var, &new_var_pos, pos, labels, label_len, fd, id);
                if(k == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"BREQ")==0){
                k=breq(buff_line, &var, &new_var_pos, pos, labels, label_len, fd, id);
                if(k == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"BRA")==0){
                k=bra(buff_line, pos, labels, label_len, fd, id);
                if(k == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"DOWN")==0){
                sem=sem_down(buff_line,pos,id);
                if (sem == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
                else{
                    if(sem<0){
                        processes[id].var=var;
                        processes[id].labels=labels;
                        processes[id].label_len=label_len;
                        processes[id].new_var_pos=new_var_pos;
                        processes[id].state=BLOCKED;
                        processes[id].count_char=processes[id].count_char+strlen(buff_line);
                        return SUCCESS;
                    }
                }

            }
            else if(strcmp(command,"UP")==0){
                sem=sem_up(buff_line, pos);
                if (sem == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"SLEEP")==0){
                if (sleeping(buff_line, &var, &new_var_pos, pos) == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"PRINT")==0){
                if (print(buff_line, &var, &new_var_pos, pos) == FAIL){
                    processes[id].state=FINISHED;
                    return FAIL;
                }
            }
            else if(strcmp(command,"RETURN")==0){
                processes[id].state=FINISHED;
                return SUCCESS;
            }
        }
        if(command[0] != 'B' || k == SUCCESS){
            processes[id].count_char=processes[id].count_char+strlen(buff_line);
        }
    }

    processes[id].var=var;
    processes[id].labels=labels;
    processes[id].label_len=label_len;
    processes[id].new_var_pos=new_var_pos;
    processes[id].state=READY;
    //  printf("elegxos count char gia to pid %d : %d \n",id,processes[id].count_char);
    return SUCCESS;
}

void *thread_fun(void* id_st){
    char buff[50], string[20], space;
    int pos, next_pid=0;
    FILE *fd, *label_fd;
    int *argum=NULL, count_argum,i, state, check_open;
    int *id=(int*) id_st;

    while(1){
        if(t_argums[*id].check==T){   //if there is an exec command for this thread
            pthread_mutex_lock(&monitor_exec);
            if(check_start[*id] == F){
                pthread_cond_wait(&cond_start[*id], &monitor_exec);
                check_start[*id]=F;
            }
            //check=F For next time to not enter this if
            t_argums[*id].check=F;
            strcpy(buff, t_argums[*id].buff);
            pos=t_argums[*id].pos;
            //start reading the buff line
            sscanf(buff+pos, "%s", string);
            pos=pos+strlen(string);
            //pass the spaces
            sscanf(buff+pos, "%c", &space);
            while(space == ' ' || space == '\t'){
                pos++;
                sscanf(buff+pos, "%c", &space);
            }
            //opens file
            check_open=T;
            fd=fopen(string, "r");
            if(fd == NULL){
                check_open=F;
                perror("problem in fopen");
            }
            //opens file second time for keeping the labels
            label_fd=fopen(string, "r");
            if(label_fd == NULL){
                check_open=F;
                perror("problem in fopen");
            }

            if(check_open == T){//if opening was succesful
                //give pid
                count_argum=0;
                argum=(int *)realloc(argum, (count_argum+1)*sizeof(int));
                argum[count_argum]=pid;

                //place in table of processes
                processes=(process *)realloc(processes, (pid+1)*sizeof(process));
                if(processes == NULL){
                    perror("Problem in processes realloc");
                    return;
                }
                //each process is a specific program
                strcpy(processes[pid].name, string);
                processes[pid].first=T;
                processes[pid].state=READY;
                processes[pid].thread=*id;

                pid++; // for the next pid
                count_argum++;

                while(buff[pos] != '\n'){
                    //reading arguments
                    sscanf(buff+pos, "%s", string);
                    pos=pos+strlen(string);
                    //pass the spaces
                    sscanf(buff+pos, "%c", &space);
                    while(space == ' ' || space == '\t'){
                        pos++;
                        sscanf(buff+pos, "%c", &space);
                    }
                    if(count_argum == 0){
                        argum=(int *)malloc(sizeof(int));
                    }
                    argum=(int *)realloc(argum, (count_argum+1)*sizeof(int));
                    if(argum == NULL){
                        perror("Error in malloc for argum");
                    }
                    if(string[0]=='-'){
                        argum[count_argum]=0-atoi(string+1);
                    }
                    else{
                        argum[count_argum]=atoi(string);
                        
                    }
                    count_argum++;
                }
                printf("\n");

                //exits the monitor
                check_finish[*id]=T;
                pthread_cond_signal(&cond_finish[*id]);
                pthread_mutex_unlock(&monitor_exec);

                //yield so others can take turns
                pthread_yield();

                //run program
                pthread_mutex_lock(&monitor_run);
                fgets(string, 20, fd);
                processes[pid-1].count_char=strlen(string);
                if(string[0] == '#'){
                    state=run_program(&fd, label_fd, argum, count_argum, pid-1);
                }
                else{
                    printf("Wrong syntax\n");
                }
                fclose(fd);
                fclose(label_fd);
                pthread_mutex_unlock(&monitor_run);
            }
        }
        else{  //else if there isn't an exec command waiting find next process to run
            for(i=0; i<pid; i++){
                if(next_pid == pid-1){
                    next_pid=0;
                }
                else{
                    next_pid++;
                }
                //if the chosen pid is ready to run and it's corresponded to this thread
                if((processes[next_pid].state==READY) && (processes[next_pid].thread==*id)){
                    break;
                }
            }
            if(processes[next_pid].state==READY && (processes[next_pid].thread==*id)){ //if "for" worked
                pthread_mutex_lock(&monitor_run);
                fd=fopen(processes[next_pid].name, "r");
                check_open=T;
                if(fd == NULL){
                    check_open=F;
                    perror("problem in fopen");
                }
                if(check_open == T){
                    fseek(fd, processes[next_pid].count_char, SEEK_SET);
                    state=run_program(&fd, label_fd, argum, count_argum, next_pid);
                    fclose(fd);
                }
                pthread_mutex_unlock(&monitor_run);
            }
        }
        pthread_yield();
    }

    free(globalVar);
    free(processes);
    free(argum);
}

int main(int argc, char *argv[] ){
    FILE* fd, *label_fd;
    char string[20], buff[50], space;
    int pos, die, i, counter_thread=0, *array;
    pthread_t *worker;

    printf("\nGive number of Threads: ");
    scanf("%d", &N);

    cond_finish=(pthread_cond_t*)malloc(N*sizeof(pthread_cond_t));
    if(cond_finish==NULL){
        perror("Malloc problem for cond_finish");
        return 0;
    }
    cond_start=(pthread_cond_t*)malloc(N*sizeof(pthread_cond_t));
    if(cond_start==NULL){
        perror("Malloc problem for cond_start");
        return 0;
    }
    worker=(pthread_t*)malloc(N*sizeof(pthread_t));
    if(worker==NULL){
        perror("Malloc problem for worker");
        return 0;
    }
    array=(int*)malloc(N*sizeof(int));
    if(array==NULL){
        perror("Malloc problem for array");
        return 0;
    }
    check_start=(int*)malloc(N*sizeof(int));
    if(check_start==NULL){
        perror("Malloc problem for check_start");
        return 0;
    }
    check_finish=(int*)malloc(N*sizeof(int));
    if(check_finish==NULL){
        perror("Malloc problem for check_finish");
        return 0;
    }
    t_argums=(init*)malloc(N*sizeof(init));
    if(t_argums==NULL){
        perror("Malloc problem for t_argums");
        return 0;
    }
    pthread_mutex_init(&monitor_exec, NULL);
    pthread_mutex_init(&monitor_run, NULL);
       
    for(i=0;i<N; i++){
        array[i]=i;
        check_start[i]=F;
        check_finish[i]=F;
        t_argums[i].check=T;
        pthread_cond_init(&cond_start[i], NULL);
        pthread_cond_init(&cond_finish[i], NULL);
    }
     
    for(i=0;i<N; i++){
        pthread_create(&worker[i], NULL, thread_fun, &array[i]);
    }

    printf("\nPlease type your commands\n(exec program.txt args || list || kill pid)\n");

    while(1){
        
        fgets(buff, 50, stdin); //read user's command
        pos=0;
        sscanf(buff, "%s", string);
        pos=pos+strlen(string);
        //pass the spaces
        sscanf(buff+pos, "%c", &space);
        while(space == ' ' || space == '\t'){
            pos++;
            sscanf(buff+pos, "%c", &space);
        }

        if(strcmp(string, "exec") == 0){
            //enter monitor
            pthread_mutex_lock(&monitor_exec);
            //init t_argums for specific pid
            strcpy(t_argums[counter_thread%N].buff, buff);
            t_argums[counter_thread%N].check=T;
            t_argums[counter_thread%N].pos=pos;
            //check for condition
            check_start[counter_thread%N]=T;
            pthread_cond_signal(&cond_start[counter_thread%N]);
            if(check_finish[counter_thread%N] == F){
                pthread_cond_wait(&cond_finish[counter_thread%N], &monitor_exec);
                check_finish[counter_thread%N]=F;
            }
            //counter for next pid
            counter_thread++;
            pthread_mutex_unlock(&monitor_exec);
        }
        else if(strcmp(string, "list") == 0){
            for(i=0; i<pid; i++){
                printf("Pid #%d: %s -> ", i, processes[i].name);
                if(processes[i].state == RUNNING){
                    printf("RUNNING\n");
                }
                else if(processes[i].state == BLOCKED){
                    printf("BLOCKED\n");
                }
                else if(processes[i].state == KILLED){
                    printf("KILLED\n");
                }
                else if(processes[i].state == READY){
                    printf("READY\n");
                }
                else{
                    printf("FINISHED\n");
                }
            }
            printf("\n");
        }
        else if(strcmp(string, "kill") == 0){
            sscanf(buff+pos, "%s", string);
            die=atoi(string);
            //kill process with pid=die
            processes[die].state=KILLED;
        }
    }
    //End
    pthread_mutex_destroy(&monitor_exec);
    pthread_mutex_destroy(&monitor_run);
    for(i=0;i<N;i++){
        pthread_cond_destroy(&cond_start[i]);
        pthread_cond_destroy(&cond_finish[i]);
    }
    free(cond_start);
    free(cond_finish);
    free(worker);
    free(t_argums);
    free(check_finish);
    free(check_start);
    return 0;
}
