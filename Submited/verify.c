#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int port_check(char *port){
	int i;
	
	for(i = 0; i < strlen(port); i++){ // Verificação que todos os caracteres do PORT são numeros
		if(!(48 <= port[i] && port[i] <= 57)){
			return -1;
		}
	}
	
	i = atoi(port);
	if(0 <= i && i <= 65535){ // Verificação que é um PORT válido
		return i;
	}else
		return -1;	
}

int key_check(char *port){
	int n;
	
	if((strlen(port) == 1) && (48 <= port[0] && port[0] <= 57)){ // Verificação para o caso de se escolher uma chave entre 0 e 9
		n = ((int) port[0]) - 48;
		if(0 <= n && n <= 9){ // Confirmação redundante
			return n;	
		}else{
			return -1;
		}
	}else if(((strlen(port) == 2) && (48 <= port[1] && port[1] <= 57) && (49 == port[0] || 50 == port[0])) || ((strlen(port) == 2) && (48 <= port[1] && port[1] <= 49) && (port[0] == 51))){ // Verificação para o caso de se escolher uma chave entre 10 e 31
		n = (((int) port[0]) - 48) * 10 + ((int) port[1]) - 48;
		if(10 <= n && n <= 31){ // Confirmação redundante
			return n;
		}else{
			return -1;
		}
	}else{
		return -1;
	}
}

int arg_check(int argc,char* argv[]){
	int n;
	
	if(argc == 4){ // Verificação que o programa é chamado com ./ring bla bla bla
		if(strcmp(argv[0], "ring")){
			if((strlen(argv[1]) == 1) && (48 <= argv[1][0] && argv[1][0] <= 57)){ // Verificação para o caso de se escolher uma chave entre 0 e 9
				n = ((int) argv[1][0]) - 48;
				if(0 <= n && n <= 9){ // Confirmação redundante
					if(port_check(argv[3]) != -1){ // Verificação que é um PORT válido
						return n;
					}
				}
			}else if(((strlen(argv[1]) == 2) && (48 <= argv[1][1] && argv[1][1] <= 57) && (49 == argv[1][0] || 50 == argv[1][0])) || ((strlen(argv[1]) == 2) && (48 <= argv[1][1] && argv[1][1] <= 49) && (argv[1][0] == 51))){ // Verificação para o caso de se escolher uma chave entre 10 e 31
				n = (((int) argv[1][0]) - 48) * 10 + ((int) argv[1][1]) - 48;
				if(10 <= n && n <= 31){ // Confirmação redundante
					if(port_check(argv[3]) != -1){ // Verificação que é um PORT válido
						return n;
					}
				}			
			}
		}
	}
	return -1;
}
