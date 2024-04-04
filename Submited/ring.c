#include "ring.h"
#define MAX_TIME_OUT 4

struct _Node{;
    char ip[50], port[7], key[3];
	int fd;
};

struct _Intruders{;
	struct sockaddr addr_intruders;
	Node no;
} list_intruders[100];

/* Esta função é usada para fazer a saída graciosa do programa */
void close_and_exit(Node *local, Node *predecessor, Node *successor, Node *intruder, Node *shortcut, int fd_tcp, int fd_udp, int newfd, Link **head, struct addrinfo **res_short){
	int i;
	printf("[*] Desligando\n");
	
	// Fecha todos os sockets abertos
	if((successor->fd == predecessor->fd) && (successor->fd != -1 && predecessor->fd != -1)){
		close(predecessor->fd);
	}else{
		if(successor->fd != -1){
			close(successor->fd);
		}
		if(predecessor->fd != -1){
			close(predecessor->fd);								
		}
	}
	if(shortcut->fd != -1){
		close(shortcut->fd);
		freeaddrinfo((*res_short));
	}
	if(intruder->fd != -1){
		close(intruder->fd);
	}
	if(fd_tcp != -1){close(fd_tcp);}
	if(fd_udp != -1){close(fd_udp);}
	if(newfd != -1){close(newfd);}
	
	// Free da memória alucada
	if((*head) != NULL){freeLink((*head));}
	
	for(i = 0; i < 100; i++){
		if(list_intruders[i].no.fd != -1){
			close(list_intruders[i].no.fd);
			list_intruders[i].no.fd = -1;
		}
	}
	
	// Fechar os file descriptors abertos por defeito
	close(0);close(1);close(2);
	exit(0);
}

/* Esta função copia as informação do Node scr para o Node nodo */
void node_cpy(Node *nodo, Node *scr){
	memset(&nodo->ip, 0, sizeof nodo->ip);
	memset(&nodo->port, 0, sizeof nodo->port);
	memset(&nodo->key, 0, sizeof nodo->key);
	strcpy(nodo->port, scr->port);
	strcpy(nodo->ip, scr->ip);
	strcpy(nodo->key, scr->key);
}

/* Esta função define os diversos campos da estrutura Node tendo em conta as strings passadas como argumento */
void node_setup(Node *nodo, char *ip, char *port, char *key, int fd){
	memset(&nodo->ip, 0, sizeof nodo->ip);
	memset(&nodo->port, 0, sizeof nodo->port);
	memset(&nodo->key, 0, sizeof nodo->key);
	strcpy(nodo->port, port);
	strcpy(nodo->ip, ip);
	strcpy(nodo->key, key);
	nodo->fd = fd;
}

/* Esta função cria um socket de comunicação TCP ou UDP, dependendo da opção escolhida, sendo também possivel escolher se o socket servirá de servidor ou cliente */
int create_socket(struct addrinfo *hints, struct addrinfo **res, Node n, char *tcp_or_udp, char *server_or_client){
	int fd, errcode;
	// Variavel que define o tempo de vida antes de dar "time out"
	struct timeval t_out={0,100000};
	
	if(strcmp(tcp_or_udp, "TCP") == 0){
		if((fd=socket(AF_INET, SOCK_STREAM, 0))==-1){printf("[X] Erro na função <socket> TCP\n");return -1;}//error
		memset(hints, 0, sizeof (*hints));
		hints->ai_family=AF_INET;// IPv4
		hints->ai_socktype=SOCK_STREAM;// TCP socket
		hints->ai_flags=AI_PASSIVE;
		
		if(strcmp(server_or_client, "SERVER") == 0){
			if((errcode=getaddrinfo(NULL, n.port, hints, res))!=0){printf("[X] Erro na função <getaddrinfo> TCP\n");return -1;}
			if(bind(fd, (*res)->ai_addr, (*res)->ai_addrlen)==-1){printf("[X] Erro na função <bind> TCP\n");return -1;}
			if(listen(fd, 5)==-1){printf("[X] Erro na função <listen>\n");return -1;}
		}
		if(strcmp(server_or_client, "CLIENT") == 0){
			if((errcode=getaddrinfo(n.ip, n.port, hints, res))!=0){printf("[X] Erro na função <getaddrinfo> TCP\n");return -1;}
			if((errcode=connect(fd, (*res)->ai_addr, (*res)->ai_addrlen)) == -1){printf("[X] Erro na função <connect>\n");return -1;}
		}
	}
	if(strcmp(tcp_or_udp, "UDP") == 0){
		if((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1){printf("[X] Erro na função <socket> UDP\n");return -1;}//error
		hints->ai_family=AF_INET;// IPv4
		hints->ai_socktype=SOCK_DGRAM;// UDP socket
		hints->ai_flags=AI_PASSIVE;

		if(strcmp(server_or_client, "SERVER") == 0){
			if((errcode=getaddrinfo(NULL, n.port, hints, res))!=0){printf("[X] Erro na função <getaddrinfo> UDP\n");return -1;}
			if(bind(fd, (*res)->ai_addr, (*res)->ai_addrlen)==-1){printf("[X] Erro na função <bind> UDP\n");return -1;}
		}
		if(strcmp(server_or_client, "CLIENT") == 0){
			if((errcode=getaddrinfo(n.ip, n.port, hints, res))!=0){printf("[X] Erro na função <getaddrinfo> UDP\n");return -1;}
		}
	}
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &t_out, sizeof(t_out));	
	return fd;
}

/* Esta função escreve no socket associado ao file descriptor fd a mensagem msg */
int send_msg(int *fd, const char *msg){
	char buffer[257], *ptr;
	int nbytes, nleft, nwritten;
	
	memset(&buffer, 0, sizeof buffer);
	ptr=strcpy(buffer, msg);
	nbytes=strlen(buffer);
	nleft=nbytes;
	while(nleft>0){
		nwritten=write((*fd), ptr, nleft);
		if(nwritten<=0){printf("[X] Erro na função <write>\n"); return -1;}
		
		nleft-=nwritten;
		ptr+=nwritten;
	}
	printf("%s", buffer);
	return 1;
}

/* Esta função trata de receber a mensagem ACK e validar a mesma */
int recv_ack(int fd){
	int n;
	char buffer[257];
	struct sockaddr addr; socklen_t addrlen;
	memset(&buffer, '\0', sizeof buffer);

	addrlen=sizeof(addr);
	n = recvfrom(fd, buffer, 256, 0, &addr, &addrlen);

	if(strcmp(buffer, "ACK") == 0){
		printf("[*] ACK recebido com sucesso\n");
		return 0;
	}else if(n == -1){
		printf("[X] Erro na função <recvfrom> ou timeout de um ACK\n");
		return 1;
	}else{
		printf("[X] Resposta UDP não esperada\n");
		return 1;
	}
}

/* Esta função trata do cálculo das distancias */
void distance_calculator(char* one, char* two, char* three, char* to_find, int* distance){
	int i;
	distance[LCL] =  key_check(to_find) - key_check(one);
	distance[SUCC] = key_check(to_find) - key_check(two);
	distance[SHRC] = key_check(to_find) - key_check(three);
	for(i=0; i < 3; i++){if(distance[i] < 0) distance[i] = distance[i] + 32;}
}
  
int main(int argc, char *argv[]){
    int key=0, i=0, one_node_ring = 0, identifier = -1, distance[3]={0,0,0}, counter = 0;
	int fd_tcp = -1, fd_udp = -1, newfd = -1;
    char *ptr = "\0", buffer[257], garbage[257], answer[257], to_find[4], modo = 'T';
	char host[NI_MAXHOST],service[NI_MAXSERV];
	struct addrinfo hints, *res, *res_short; 
	ssize_t n = 0;
	struct sockaddr addr; socklen_t addrlen; struct sockaddr client;
	fd_set current_sockets, ready_sockets;
    Node local, predecessor, successor, intruder, shortcut, origin;
	Link *head = initLink();
	time_t t;
	
	srand((unsigned) time(&t));
	
	// Função de verificação dos argumentos, descrita no ficheiro verify.c
	key = arg_check(argc,argv);

	if(key == -1){
		printf("-------------------------------------ERRO-------------------------------------\n");
		printf("Formato de utilização: ./ring i ip port\n");
		printf("0 <= i <= 31 & ip versão IPv4 & 0 <= port <= 65535\n");
		printf("------------------------------------------------------------------------------\n");
		exit(0);
	}
	
	node_setup(&local, argv[2], argv[3], argv[1], -1);
	node_setup(&predecessor, ptr, ptr, ptr, -1);
	node_setup(&successor, ptr, ptr, ptr, -1);
	node_setup(&shortcut, ptr, ptr, ptr, -1);
	node_setup(&intruder, ptr, ptr, ptr, -1);
	memset(&host, '\0', sizeof host);
	memset(&service, '\0', sizeof service);

	printf("--------------------------------OPÇÕES--PARA--INICIAR--------------------------------\n");
	printf("n/new - cria um anel contendo apenas o nó local\n");
	printf("b/bentry i ip port - entrada no anel procurando o seu sucessor pelo nó i\n");
	printf("p/pentry i ip port - insere o nó local num anel tomando i como o nó predecessor\n");
	printf("-----------------------------OPÇÕES--DEPOIS--DE--INICIAR-----------------------------\n");
	printf("c/chord i ip port - adicionar um atalho para o nodo i\n");
	printf("d/dchord - remover o atalho do nodo atual\n");	
	printf("s/show - mostra o estado do nó\n");	
	printf("f/find k - procura o nó k\n");
	printf("l/leave - sai do anel\n");
	printf("-------------------------------------------------------------------------------------\n");
	printf("e/exit - termina o programa\n");
	printf("-------------------------------EM--TODOS--OS--COMANDOS-------------------------------\n");
	printf("0 <= i <= 31 & ip versão IPv4 & 0 <= port <= 65535\n");
	printf("-------------------------------------------------------------------------------------\n");

	for(i = 0; i < 100; i++){
		node_setup(&list_intruders[i].no, ptr, ptr, ptr, -1);
	}
	
	FD_ZERO(&current_sockets);
	FD_SET(STDIN_FILENO, &current_sockets);
	
	while(1){
		ready_sockets = current_sockets;
		if(select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) <0){printf("[X] Erro na função <select>\n");close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}
		
		for(i = 0; i < FD_SETSIZE; i++){
			memset(&buffer, '\0', sizeof buffer);
			memset(&answer, '\0', sizeof answer);
			memset(&to_find, '\0', sizeof to_find);
			memset(&hints, 0, sizeof hints);
			addrlen=sizeof(addr);
			distance[SUCC] = 0;
			distance[LCL] = 0;
			distance[SHRC] = 0;
			counter = 0;
			
			if(FD_ISSET(i,&ready_sockets)){
				/* Este if vai até à linha 270 e trata de todas as ligações tcp estranhas ao utilizador local, são elas: 
				1 - mensagem "SELF" vinda de algum nodo do anel em resposta a um "pentry" local da qual podemos associar o proveniente como sendo sucessor e não devendo responder 
				2 - mensagem "SELF" vinda de algum novo nodo, estranho ao anel, que acabou de fazer "pentry" , que será tratado com sucessor */
				if(i == fd_tcp){
					if((newfd = accept(fd_tcp, &addr, &addrlen))==-1){printf("[X] Erro na função <accept>\n");close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}
					/* Um buffer de 256 bytes deve garantir a leitura de qualquer mensagem que seja transmitada para
					este tipo de protocolo, dessa forma evita-se o uso da função "while" para leitura de mensagens */
					if((n=read(newfd, buffer, 256))!=0){
						if(n==-1){printf("[X] Erro na função <read>\n");close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}
						if(buffer[(strlen(buffer) - 1)] != '\n'){printf("[X] Mensagem TCP com formato incorreto\n");close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}

						printf("[+] Recebido via TCP: %s",buffer);
						
						// Caso 1 descrito anteriormente
						if(!strncmp(buffer,"SELF",4) && (successor.fd == -1 && predecessor.fd != -1)){ 
							if(sscanf(buffer,"%s %s %s %s\n", garbage, intruder.key, intruder.ip, intruder.port) == 4){
								successor.fd = newfd; // É guardado o socket da comunicação usada para futuras mensagens que se queiram enviadar para o sucessor
								node_cpy(&successor, &intruder); // Copia do uma estrutura "node" para a outra
								printf("[*] P/Bentry executado com sucesso\n\n");
								FD_SET(successor.fd, &current_sockets); // Esta linha poderá ser excluída por não ser necessário ouvir mensagens do nosso sucessor
							}else{
								printf("[X] Formato da mensagem SELF errado\n");
								close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
							}
							
						// Caso 2 descrito anteriormente
						}else if(!strncmp(buffer,"SELF",4)){
							if(sscanf(buffer,"%s %s %s %s\n", garbage, intruder.key, intruder.ip, intruder.port) == 4){
								// No caso de ser um anel com mais que um nó a mensagem "PRED" deverá ser enviada ao sucessor
								if(!(one_node_ring)){
									if(key_check(intruder.key) == -1 || port_check(intruder.port) == -1){
										printf("[X] Formato da mensagem SELF errado\n");
										close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
									// (Caso bolinha)
									}else if(successor.fd != -1 && ((predecessor.fd == successor.fd) || ((key_check(local.key)) < (key_check(intruder.key)) && (key_check(intruder.key)) < (key_check(successor.key))) || ((key_check(intruder.key)) < (key_check(successor.key)) && (key_check(successor.key)) < (key_check(local.key))) || ((key_check(successor.key)) < (key_check(local.key)) && (key_check(local.key)) < (key_check(intruder.key))))){
										sprintf(answer,"PRED %s %s %s\n", intruder.key, intruder.ip, intruder.port);
										printf("[-] Enviei para o Sucessor: ");
										if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}	
									}else if(((key_check(local.key)) < (key_check(successor.key)) && (key_check(successor.key)) < (key_check(intruder.key))) || ((key_check(intruder.key)) < (key_check(local.key)) && (key_check(local.key)) < (key_check(successor.key))) || ((key_check(successor.key)) < (key_check(intruder.key)) && (key_check(intruder.key)) < (key_check(local.key)))){
										printf("[-] O sucessor saiu do anel\n[+] Novo sucessor\n");
									}
								// No caso de ser um anel com apenas um nó não será necessário enviar "PRED" a si mesmo acabando assim, por o nosso predecessor ser igual ao sucessor (Caso quadradinho)
								}else{
									sprintf(answer,"SELF %s %s %s\n", local.key, local.ip, local.port);
									predecessor.fd = newfd;
									printf("[-] Enviei para o Predecessor/Sucessor: ");
									if(send_msg(&predecessor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}	
									node_cpy(&predecessor, &intruder);
									FD_SET(predecessor.fd, &current_sockets);
									one_node_ring = 0;
								}
								// Se não estivermos no caso bolinha nem no caso quadradinho, podemos dar clear e close no nosso successor, uma vez que vai entra um novo sucessor
								if(predecessor.fd != newfd && predecessor.fd != successor.fd){FD_CLR(successor.fd, &current_sockets);close(successor.fd);}
								
								successor.fd = newfd;
								FD_SET(successor.fd, &current_sockets); // Esta linha poderá ser excluída por não ser necessário ouvir mensagens do nosso sucessor
								node_cpy(&successor, &intruder);
								
							}else{
								printf("[X] Formato da mensagem SELF errado\n");
								close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
							}
						}else{
							printf("[X] Mensagem TCP não reconhecida\n");
							close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);							
						}
					}else{
						printf("[X] Quebra da ligação inesperada\n");
						close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
					}
					
				/* Este if vai até à linha 507 e trata de todas as ligações udp estranhas ao utilizador local, são elas: 
				1 - mensagem "RSP" vinda de um atalho qualquer a qual devemos fazer seguir pelo caminho mais curto ou parar o seguimento
				2 - mensagem "FND" vinda de um atalho qualquer a qual devemos fazer seguir pelo caminho mais curto ou fazer seguir "RSP"
				3 - mensagem "EFND" vinda de um nó entrante qualquer a qual devemos iniciar um "FND" pelo caminho mais curto ou reenviar um "EPRED" */			
				}else if(i == fd_udp){
					if((n = recvfrom(fd_udp, buffer, 256, 0, &addr, &addrlen))!=0){
						// Enviar um ACK para cada mensagem UDP recebida
						if(n==-1){printf("[X] Erro na função <recvfrom>\n");close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}
						if(buffer[(strlen(buffer) - 1)] == '\n'){printf("[X] Mensagem UDP com formato incorreto\n");close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}
						printf("[+] Recebido via UDP: %s\n", buffer);
						sprintf(answer,"ACK");
						n = sendto(fd_udp,answer,3,0,&addr,addrlen);
						if(n==-1){printf("[X] Erro na função <sendto>\n");close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}//error
						printf("[-] Enviei ACK\n");
						
						// Caso 1 descrito anteriormente
						if(!strncmp(buffer,"RSP",3)){
							if(sscanf(buffer,"%s %s %d %s %s %s", garbage, to_find, &identifier, origin.key, origin.ip, origin.port) == 6){
								if(key_check(to_find) == -1 || identifier > 99 || identifier < 0){
									printf("[X] Formato da mensagem RSP errado\n");
									close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
								}
								
								distance_calculator(local.key, successor.key, shortcut.key, to_find, distance); // Cálculo das distancias
								if(shortcut.fd == -1){distance[SHRC] = 10000;} // Distâncias enormes caso não exista shortcuts ou sucessor (caso seja one_node_ring, por exemplo) 
								if(successor.fd == -1 || (strcmp(local.key, successor.key) == 0 && strcmp(local.key, predecessor.key) == 0)){distance[SUCC] = 10000;}

								// Avaliação dos vários cenários
								if(distance[LCL] != 0 && distance[SUCC] < distance[SHRC]){ // Cenário 1: Encaminhar a resposta para o sucessor via TCP
									printf("[-] Enviei para o Sucessor: ");
									sprintf(answer,"RSP %s %d %s %s %s\n",  to_find, identifier, origin.key, origin.ip, origin.port); 
									if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}		
									
								}else if(distance[LCL] != 0 && distance[SHRC] < distance[SUCC]){ // Cenário 2: Encaminhar a resposta para o atalho via UDP, ou, em caso de erro, via TCP
									sprintf(answer,"RSP %s %d %s %s %s",  to_find, identifier, origin.key, origin.ip, origin.port);	
									printf("[-] Enviei para o Atalho: %s\n", answer);
									
									// Reenvio das mensagens e caso de não haver receção de um ACK
									do{
										n = sendto(shortcut.fd,answer,strlen(answer),0,res_short->ai_addr,res_short->ai_addrlen);
										if(n==-1){
											printf("[X] Erro na função <sendto>\n");
											close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
										}//error
										counter++;
										if(counter > 1){printf("[@] Tentar de novo\n");}	
									}while(recv_ack(shortcut.fd) && counter <= MAX_TIME_OUT);
									
									if(counter == (MAX_TIME_OUT + 1)){ // Cenário 2.1: Encaminhar a resposta para sucessor via TCP por falha na ligação ao atalho
										printf("[X] Não foi possível enviar a mensagem UDP\n");
										if(successor.fd != -1){
											printf("[*] Tentar via TCP\n");
											printf("[-] Enviei para o Sucessor: ");
											if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}												
										}
									}
									
								}else if(distance[LCL] == 0){ // Cenário 3: O nó local fez a procura
									key = search_identifier(&head,identifier,&modo);
									if(key == -1){printf("[X] Procura duplicada ou já respondida\n");}
									
									if(key != -1 && modo == 'F'){
										printf("\n[*] Procura completa\n[*] A chave %d encontra-se no nó %s %s %s\n\n",key, origin.key, origin.ip, origin.port);
										
									}else if(key != -1 && modo == 'E'){
										sprintf(answer,"EPRED %s %s %s",origin.key, origin.ip, origin.port);	
										printf("[-] Enviei para o intruso: %s\n",answer);
										if(list_intruders[identifier].no.fd != -1){
											do{
												n = sendto(list_intruders[identifier].no.fd,answer, strlen(answer), 0, &list_intruders[identifier].addr_intruders, addrlen);
												if(n==-1){
													printf("[X] Erro na função <sendto>\n");
													close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
												}//error	
												counter++;
												if(counter > 1){printf("[@] Tentar de novo\n");}
											}while(recv_ack(fd_udp) && counter <= MAX_TIME_OUT);
											if(counter == (MAX_TIME_OUT + 1)){printf("[X] Não foi possível enviar a mensagem EPRED por UDP\n");}
										}
									}
								}
							}else{
								printf("[X] Formato da mensagem RSP errado\n");
								close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
							}
							
						// Caso 2 descrito anteriormente
						}else if(!strncmp(buffer,"FND",3)){
							if(sscanf(buffer,"%s %s %d %s %s %s", garbage, to_find, &identifier, origin.key, origin.ip, origin.port) == 6){
								if(key_check(to_find) == -1 || identifier > 99 || identifier < 0){
									printf("[X] Formato da mensagem FND errado\n");
									close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
								}
								distance_calculator(local.key, successor.key, shortcut.key, to_find, distance); // Cálculo das distancias
								if(shortcut.fd == -1){distance[SHRC] = 10000;} // Distâncias enormes caso não exista shortcuts ou sucessor (caso seja one_node_ring, por exemplo) 
								if(successor.fd == -1 || (strcmp(local.key,successor.key) == 0 && strcmp(local.key,predecessor.key) == 0)){distance[SUCC] = 10000;}

								memset(&answer,'\0',sizeof answer);	
						
								// Avaliação dos vários cenários						
								if(distance[SHRC] > distance[LCL] && distance[SUCC] > distance[LCL]){ // Cenário 1: Caso em que começam a ser enviadas mensagens "RSP" 
									distance_calculator(local.key, successor.key, shortcut.key, origin.key, distance);
									if(shortcut.fd == -1){distance[SHRC] = 10000;} // Distâncias enormes caso não exista shortcuts ou sucessor (caso seja one_node_ring, por exemplo) 
									if(successor.fd == -1 || (strcmp(local.key,successor.key) == 0 && strcmp(local.key,predecessor.key) == 0)){distance[SUCC] = 10000;}

									// Avaliação dos vários cenários	
									if(distance[SUCC] < distance[SHRC]){ // Cenário 1: Encaminhar a resposta para o sucessor via TCP
										printf("[-] Enviei para o Sucessor: ");
										sprintf(answer,"RSP %s %d %s %s %s\n", origin.key, identifier, local.key, local.ip, local.port); 
										if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}	
										
									}else if(distance[SHRC] < distance[SUCC]){ // Cenário 2: Encaminhar a resposta para o atalho via UDP
										sprintf(answer,"RSP %s %d %s %s %s", origin.key, identifier, local.key, local.ip, local.port);	
										printf("[-] Enviei para o Atalho: %s\n", answer);
										
										do{
											n = sendto(shortcut.fd,answer,strlen(answer),0,res_short->ai_addr,res_short->ai_addrlen);
											if(n==-1){
												printf("[X] Erro na função <sendto>\n");
												close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
											}//error
											counter++;
											if(counter > 1){printf("[@] Tentar de novo\n");}											
										}while(recv_ack(shortcut.fd) && counter <= MAX_TIME_OUT);
										
										if(counter == (MAX_TIME_OUT + 1)){
											printf("[X] Não foi possível enviar a mensagem UDP\n");
											if(successor.fd != -1){
												printf("[*] Tentar via TCP\n");
												printf("[-] Enviei para o Sucessor: ");
												if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}												
											}
										}	
									}

								}else if(distance[SHRC] >= distance[SUCC]){ // Cenário 2: Encaminhar a procura para o sucessor via TCP
									printf("[-] Enviei para o Sucessor: ");
									sprintf(answer,"FND %s %d %s %s %s\n", to_find, identifier, origin.key, origin.ip, origin.port); 
									if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}	
									
								}else if(distance[SHRC] < distance[SUCC]){ // Cenário 3: Encaminhar a procura para o atalho via UDP
									sprintf(answer,"FND %s %d %s %s %s", to_find, identifier, origin.key, origin.ip, origin.port);
									printf("[-] Enviei para o Atalho: %s\n", answer);	
									
									do{
										n = sendto(shortcut.fd,answer,strlen(answer),0,res_short->ai_addr,res_short->ai_addrlen);
										if(n==-1){
											printf("[X] Erro na função <sendto>\n");
											close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
										}//error
										counter++;
										if(counter > 1){printf("[@] Tentar de novo\n");}										
									}while(recv_ack(shortcut.fd) && counter <= MAX_TIME_OUT);	
									
									if(counter == (MAX_TIME_OUT + 1)){
										printf("[X] Não foi possível enviar a mensagem UDP\n");
										if(successor.fd != -1){
											printf("[*] Tentar via TCP\n");
											printf("[-] Enviei para o Sucessor: ");
											if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}												
										}
									}	
								}
							}else{
								printf("[X] Formato da mensagem FND errado\n");
								close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
							}

						// Caso 3 descrito anteriormente					
						}else if(!strncmp(buffer,"EFND",4)){
							if(sscanf(buffer,"%s %s", garbage, to_find) == 2 ){

								// Guardar a informação do nó que enviou o EFND 
								memcpy(&client, &addr, addrlen);
								node_setup(&intruder, host, service, to_find, socket(AF_INET, SOCK_DGRAM, 0)); 
								if(intruder.fd==-1){printf("[X] Erro na função <socket> UDP\n");close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}

								// Iniciar a pesquisa da key recebida
								if(key_check(to_find) != -1){									
									distance_calculator(local.key, successor.key, shortcut.key, to_find, distance);
									if(shortcut.fd == -1){distance[SHRC] = 10000;} // Distância enormes caso não exista shortcuts ou sucessor (caso seja one_node_ring, por exemplo) 
									if(successor.fd == -1 || (strcmp(local.key,successor.key) == 0 && strcmp(local.key,predecessor.key) == 0)){distance[SUCC] = 10000;}

									identifier = rand() % 100; // Geração de número aleatório
									
									if(distance[SUCC] < distance[SHRC] && distance[SUCC] < distance[LCL] && successor.fd != -1){// Delegar a pesquisa para o sucessor via TCP
										memcpy(&list_intruders[identifier].addr_intruders, &addr, addrlen);	
										node_cpy(&list_intruders[identifier].no,&intruder);
										list_intruders[identifier].no.fd = intruder.fd;
										printf("[-] Enviei para o Sucessor: ");
										sprintf(answer,"FND %s %d %s %s %s\n", to_find, identifier, local.key, local.ip, local.port); // Envio de um SELF para o predecessor novo conhecer o seu sucessor novo (o nó local)
										if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}	
										head = insertUnsortedLink(head, identifier, to_find,'E');
										node_setup(&intruder,"\0","\0","\0",-1);
									}else if(distance[SHRC] <= distance[SUCC] && distance[SHRC] < distance[LCL] && shortcut.fd != -1){// Delegar a pesquisa para o atalho via UDP
										memcpy(&list_intruders[identifier].addr_intruders, &addr, addrlen);	
										node_cpy(&list_intruders[identifier].no,&intruder);
										list_intruders[identifier].no.fd = intruder.fd;
										sprintf(answer,"FND %s %d %s %s %s", to_find, identifier, local.key, local.ip, local.port); 
										printf("[-] Enviei para o Atalho: %s\n", answer);
										do{
											n = sendto(shortcut.fd, answer, strlen(answer), 0, res_short->ai_addr, res_short->ai_addrlen);
											if(n==-1){
												printf("[X] Erro na função <sendto>\n");
												close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
											}//error
											counter++;
											if(counter > 1){printf("[@] Tentar de novo\n");}											
										}while(recv_ack(shortcut.fd) && counter <= MAX_TIME_OUT);
										
										if(counter == (MAX_TIME_OUT + 1)){
											printf("[X] Não foi possível enviar a mensagem UDP\n");
											if(successor.fd != -1){
												printf("[*] Tentar via TCP\n");
												printf("[-] Enviei para o Sucessor: ");
												if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}												
											}
										}	
										head = insertUnsortedLink(head, identifier, to_find,'E');
										node_setup(&intruder,"\0","\0","\0",-1);
									}else if(distance[LCL] < distance[SUCC] && distance[LCL] < distance[SHRC]){ //Nó que quer entrar tem o seu predecessor no nó em que pediu a pesquisa
										sprintf(answer,"EPRED %s %s %s",local.key, local.ip, local.port);	
										printf("[-] Enviei para o intruso: %s\n",answer);
										
										do{ //Reenvio das mensagens por UDP caso não receba um ACK (com Time Out)
											n = sendto(intruder.fd,answer, strlen(answer), 0, &client, addrlen);
											if(n==-1){
												printf("[X] Erro na função <sendto>\n");
												close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
											}//error
											counter++;
											if(counter > 1){printf("[@] Tentar de novo\n");}											
										}while(recv_ack(fd_udp) && counter <= MAX_TIME_OUT);
										if(counter == (MAX_TIME_OUT + 1)){printf("[X] Não foi possível enviar a mensagem UDP\n");}	
										else{close(intruder.fd);node_setup(&intruder,"\0","\0","\0",-1);}
									}						
								}else{
									printf("[X] Formato da mensagem EFND errado\n");
									close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
								}
							}else{
								printf("[X] Formato da mensagem EFND errado\n");
								close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
							}
						}else{
							printf("[X] Mensagem UDP não reconhecida\n");
							close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
						}
					}
					
				/* Este if vai até à linha 732 e trata de todas as ligações tcp enviadas pelo predecessor ao utilizador local, são elas: 
				1 - mensagem "PRED" para atualizar o predecessor do nó local 
				2 - mensagem "SELF" em resposta a um "pentry" local da qual podemos associar o proveniente como sendo sucessor 
					(sendo neste caso predecessor e sucessor ao mesmo tempo, anel de 2 nós) e não devendo responder
				3 - mensagem "FND" a qual devemos fazer seguir pelo caminho mais curto ou fazer seguir "RSP"
				4 - mensagem "RSP" a qual devemos fazer seguir pelo caminho mais curto ou parar o seguimento */					
				}else if(i == predecessor.fd){
					if((n=read(predecessor.fd,buffer,256))!=0 && strcmp(predecessor.key, "\0") != 0){
						if(n==-1){printf("[X] Erro na função <read>\n");close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}
						if(buffer[(strlen(buffer) - 1)] != '\n'){printf("[X] Mensagem TCP com formato incorreto\n");close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}
						printf("[+] Recebido do Predecessor: %s",buffer);
						
						// Caso 1 descrito anteriormente
						if(!strncmp(buffer,"PRED",4)){
							if(sscanf(buffer,"%s %s %s %s\n", garbage, intruder.key, intruder.ip, intruder.port) == 4){
								if((strcmp(intruder.key,local.key) == 0) && (strcmp(intruder.ip,local.ip) == 0) && (strcmp(intruder.port,local.port) == 0)){
									one_node_ring = 1;

									if((successor.fd == predecessor.fd) && (successor.fd != -1 && predecessor.fd != -1)){
										FD_CLR(predecessor.fd, &current_sockets);
										close(predecessor.fd);
										node_setup(&predecessor, "\0", "\0", "\0", -1);
										node_setup(&successor, "\0", "\0", "\0", -1);
									}else{
										if(successor.fd != -1){
											FD_CLR(successor.fd, &current_sockets);
											close(successor.fd);
											node_setup(&successor, "\0", "\0", "\0", -1);
										}
										if(predecessor.fd != -1){
											FD_CLR(predecessor.fd, &current_sockets);
											close(predecessor.fd);
											node_setup(&predecessor, "\0", "\0", "\0", -1);									
										}
									}
									if(!strcmp(predecessor.key, local.key) && !strcmp(local.key, successor.key)){
										node_setup(&predecessor, "\0", "\0", "\0", -1);	
										node_setup(&successor, "\0", "\0", "\0", -1);									
									}
									if(shortcut.fd != -1){
										close(shortcut.fd);
										node_setup(&shortcut, "\0", "\0", "\0", -1);
										freeaddrinfo(res_short);
									}
									printf("[*] É o unico nó no anel\n");
									node_cpy(&predecessor,&local);
									node_cpy(&successor,&local);
	
								}else{								
									newfd = create_socket(&hints, &res, intruder, "TCP", "CLIENT"); // Criação de um socket TCP para entrar em contacto com o nó passado pela mensagem PRED
									freeaddrinfo(res);
									if(newfd == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}
									printf("[-] Enviei para o novo Predecessor: ");
									sprintf(answer,"SELF %s %s %s\n", local.key, local.ip, local.port); // Envio de um SELF para o predecessor novo conhecer o seu sucessor novo (o nó local)
									if(send_msg(&newfd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}	
									if(predecessor.fd != successor.fd){FD_CLR(predecessor.fd, &current_sockets);close(predecessor.fd);}	
									node_cpy(&predecessor, &intruder);									
									predecessor.fd = newfd;
									FD_SET(predecessor.fd, &current_sockets); // Manter o programa atento às mensagem que cheguem neste socket
								}
							}else{
								printf("[X] Formato da mensagem PRED errado\n");
								close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
							}
							
						// Caso 2 descrito anteriormente
						}else if(!strncmp(buffer,"SELF",4)){
							if(sscanf(buffer,"%s %s %s %s\n", garbage, successor.key, successor.ip, successor.port) == 4){
								printf("[*] P/Bentry executado com sucesso\n\n");
								successor.fd = predecessor.fd;
								FD_SET(successor.fd, &current_sockets); // Esta linha poderá ser excluída por não ser necessário ouvir mensagens do nosso sucessor								
							}else{
								printf("[X] Formato da mensagem SELF errado\n");
								close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
							}

						// Caso 3 descrito anteriormente
						}else if(!strncmp(buffer,"FND",3)){
							if(sscanf(buffer,"%s %s %d %s %s %s\n", garbage, to_find, &identifier, origin.key, origin.ip, origin.port) == 6){
								if(key_check(to_find) == -1 || identifier > 99 || identifier < 0){
									printf("[X] Formato da mensagem FND errado\n");
									close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
								}
								
								distance_calculator(local.key, successor.key, shortcut.key, to_find, distance);
								if(shortcut.fd == -1){distance[SHRC] = 10000;} // Distância enormes caso não exista shortcuts ou sucessor (caso seja one_node_ring, por exemplo) 
								if(successor.fd == -1 || (strcmp(local.key,successor.key) == 0 && strcmp(local.key,predecessor.key) == 0)){distance[SUCC] = 10000;}
							
								// Avaliação dos vários cenários
								if(distance[SUCC] > distance[LCL] && distance[SHRC] > distance[LCL]){ // Cenário 1: Caso em que começam a ser enviadas mensagens "RSP" 
									distance_calculator(local.key, successor.key, shortcut.key, origin.key, distance);
									if(shortcut.fd == -1){distance[SHRC] = 10000;}
									if(successor.fd == -1 || (strcmp(local.key,successor.key) == 0 && strcmp(local.key,predecessor.key) == 0)){distance[SUCC] = 10000;}
									
									// Avaliação dos vários cenários
									if(distance[SUCC] <= distance[SHRC]){ // Cenário 1: Encaminhar a resposta para o sucessor via TCP
										printf("[-] Enviei para o Sucessor: ");
										sprintf(answer,"RSP %s %d %s %s %s\n", origin.key, identifier, local.key, local.ip, local.port); 
										if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}	
									}else if(distance[SHRC] < distance[SUCC]){ // Cenário 2: Encaminhar a resposta para o atalho via UDP
										sprintf(answer,"RSP %s %d %s %s %s", origin.key, identifier, local.key, local.ip, local.port);	
										printf("[-] Enviei para o Atalho: %s\n", answer);
										do{
											n = sendto(shortcut.fd,answer,strlen(answer),0,res_short->ai_addr,res_short->ai_addrlen);
											if(n==-1){
												printf("[X] Erro na função <sendto>\n");
												close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
											}//error
											counter++;
											if(counter > 1){printf("[@] Tentar de novo\n");}										
										}while(recv_ack(shortcut.fd) && counter <= MAX_TIME_OUT);
										
										if(counter == (MAX_TIME_OUT + 1)){
											printf("[X] Não foi possível enviar a mensagem UDP\n");
											if(successor.fd != -1){
												printf("[*] Tentar via TCP\n");
												printf("[-] Enviei para o Sucessor: ");
												if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}												
											}
										}
									}
									
								}else if(distance[SHRC] >= distance[SUCC]){ // Cenário 2: Encaminhar a procura para o sucessor via TCP
									printf("[-] Enviei para o Sucessor: ");
									sprintf(answer,"FND %s %d %s %s %s\n",  to_find, identifier, origin.key, origin.ip, origin.port); 
									if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}	
									
								}else if(distance[SHRC] < distance[SUCC]){ // Cenário 3: Encaminhar a procura para o atalho via UDP
									sprintf(answer,"FND %s %d %s %s %s",  to_find, identifier, origin.key, origin.ip, origin.port);
									printf("[-] Enviei para o Atalho: %s\n", answer);
									do{
										n = sendto(shortcut.fd,answer,strlen(answer),0,res_short->ai_addr,res_short->ai_addrlen);
										if(n==-1){
											printf("[X] Erro na função <sendto>\n");
											close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
										}//error
										counter++;
										if(counter > 1){printf("[@] Tentar de novo\n");}										
									}while(recv_ack(shortcut.fd) && counter <= MAX_TIME_OUT);
									
									if(counter == (MAX_TIME_OUT + 1)){
										printf("[X] Não foi possível enviar a mensagem UDP\n");
										if(successor.fd != -1){
											printf("[*] Tentar via TCP\n");
											printf("[-] Enviei para o Sucessor: ");
											if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}												
										}
									}
								}		
							}else{
								printf("[X] Formato da mensagem FND errado\n");
								close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
							}
							
						// Caso 4 descrito anteriormente						
						}else if(!strncmp(buffer,"RSP",3)){
							if(sscanf(buffer,"%s %s %d %s %s %s\n", garbage, to_find, &identifier, origin.key, origin.ip, origin.port) == 6){
								if(key_check(to_find) == -1 || identifier > 99 || identifier < 0){
									printf("[X] Formato da mensagem RSP errado\n");
									close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
								}
								
								distance_calculator(local.key, successor.key, shortcut.key, to_find, distance);
								if(shortcut.fd == -1){distance[SHRC] = 10000;}
								if(successor.fd == -1 || (strcmp(local.key,successor.key) == 0 && strcmp(local.key,predecessor.key) == 0)){distance[SUCC] = 10000;}

								// Avaliação dos vários cenários
								if(distance[LCL] != 0 && distance[SUCC] < distance[SHRC]){ // Cenário 1: Encaminhar a resposta para o sucessor via TCP
									printf("[-] Enviei para o Sucessor: ");
									sprintf(answer,"RSP %s %d %s %s %s\n",  to_find, identifier, origin.key, origin.ip, origin.port); 
									if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}	
									
								}else if(distance[LCL] != 0 && distance[SHRC] < distance[SUCC]){ // Cenário 2: Encaminhar a resposta para o atalho via UDP
									sprintf(answer,"RSP %s %d %s %s %s", to_find, identifier, origin.key, origin.ip, origin.port);	
									printf("[-] Enviei para o Atalho: %s\n", answer);
									do{
										n = sendto(shortcut.fd,answer,strlen(answer),0,res_short->ai_addr,res_short->ai_addrlen);
										if(n==-1){
											printf("[X] Erro na função <sendto>\n");
											close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
										}//error
										counter++;
										if(counter > 1){printf("[@] Tentar de novo\n");}										
									}while(recv_ack(shortcut.fd) && counter <= MAX_TIME_OUT);
									
									if(counter == (MAX_TIME_OUT + 1)){
										printf("[X] Não foi possível enviar a mensagem UDP\n");
										if(successor.fd != -1){
											printf("[*] Tentar via TCP\n");
											printf("[-] Enviei para o Sucessor: ");
											if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}												
										}
									}
									
								}else if(distance[LCL] == 0){ // Cenário 3: O nó local fez a procura		
									key = search_identifier(&head,identifier,&modo);
									if(key == -1){printf("[X] Procura duplicada ou já respondida\n");}
									
									// Responder de forma diferente caso o comando originador da procura seja um FIND ou um EFND
									if(key != -1 && modo == 'F'){
										printf("\n[*] Procura completa\n[*] A chave %d encontra-se no nó %s %s %s\n\n",key,origin.key, origin.ip, origin.port);

									}else if(key != -1 && modo == 'E'){
										sprintf(answer,"EPRED %s %s %s",origin.key, origin.ip, origin.port);	
										printf("[-] Enviei para o intruso: %s\n",answer);
										if(list_intruders[identifier].no.fd != -1){
											do{
												n = sendto(list_intruders[identifier].no.fd,answer, strlen(answer), 0, &list_intruders[identifier].addr_intruders, addrlen);
												if(n==-1){
													printf("[X] Erro na função <sendto>\n");
													close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
												}//error	
												counter++;
												if(counter > 1){printf("[@] Tentar de novo\n");}
											}while(recv_ack(fd_udp) && counter <= MAX_TIME_OUT);
											if(counter == (MAX_TIME_OUT + 1)){printf("[X] Não foi possível enviar a mensagem EPRED por UDP\n");}
											else{close(list_intruders[identifier].no.fd); list_intruders[identifier].no.fd = -1;}
										}else{
											printf("[X] Está algo de errado com o file descriptor do intruso\n");
										}
									}
								}								
							}else{
								printf("[X] Formato da mensagem FND errado\n");
								close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
							}
						}else{
							printf("[X] Mensagem TCP não reconhecida\n");
							close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
						}						
					}else{
						if(strcmp(predecessor.key, "\0") == 0){
							FD_CLR(predecessor.fd, &current_sockets);
							close(predecessor.fd);
							predecessor.fd = -1;
							if(successor.fd == -1 && predecessor.fd == -1 && n == 0){printf("[*] Leave executado com sucesso\n\n");}
						}else{
							printf("[X] Quebra da ligação inesperada com o Predecessor\n");
							close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
						}
					}
					
				/* Este if precisa de ser testado, para saber que tipo de importância tem no programa */
				}else if(i == successor.fd){
					if((n=read(successor.fd, buffer, 256))!=0 && strcmp(predecessor.key, "\0") != 0){
						if(n==-1){printf("[X] Erro na função <read>\n");/*error*/exit(1);}

						printf("[+] Recebido do Sucessor: %s", buffer);
					}else{
						if(strcmp(successor.key, "\0") == 0){
							FD_CLR(successor.fd, &current_sockets);
							close(successor.fd);
							successor.fd = -1;
							if(successor.fd == -1 && predecessor.fd == -1 && n == 0){printf("[*] Leave executado com sucesso\n\n");}
						}else{
							printf("[X] Quebra da ligação inesperada com o atual Sucessor\n");
							close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
						}
					}
					
				/* Este if vai até à linha 963 e trata de todas os comandos inseridos pelo utilizador local, descritos da linha 129 à linha 139 */	
				}else if (i == STDIN_FILENO){ 
					fgets(buffer, 256, stdin); // Leitura do comando inserido pelo utilizador
					
					/* Este if vai até à linha 649 e trata de os comandos que precisem de "new", "pentry" ou "bentry" antes para funcionarem */							
					if(fd_tcp != -1 && fd_udp != -1){
						// Comandos que não podem ser feitos depois de new, pentry e bentry
						if(!strncmp(buffer,"pentry",6) || !strcmp(buffer,"new\n") || !strcmp(buffer,"exit\n") || !strncmp(buffer,"p",1) || !strcmp(buffer,"n\n") || !strcmp(buffer,"e\n") || !strcmp(buffer,"b\n") || !strncmp(buffer,"bentry",6)){
							printf("[*] Para usar n/new, p/pentry, b/bentry ou e/exit deve fazer l/leave do anel atual\n");
						}else if( !strncmp(buffer,"f",1) || !strncmp(buffer,"find",4)){
							if(sscanf(buffer,"%s %s\n", garbage, to_find) == 2){
								if(key_check(to_find) != -1){
									distance_calculator(local.key, successor.key, shortcut.key, to_find, distance);
									if(shortcut.fd == -1){distance[SHRC] = 10000;}
									if(successor.fd == -1 || (strcmp(local.key,successor.key) == 0 && strcmp(local.key,predecessor.key) == 0)){distance[SUCC] = 10000;}

									identifier = rand() % 100; // Geração de número aleatório

									if(distance[SUCC] < distance[SHRC] && distance[SUCC] < distance[LCL] && successor.fd != -1){								
										printf("[-] Enviei para o Sucessor: ");
										sprintf(answer,"FND %s %d %s %s %s\n", to_find,identifier ,local.key, local.ip, local.port); // Envio de um SELF para o predecessor novo conhecer o seu sucessor novo (o nó local)
										if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}	
										head = insertUnsortedLink(head, identifier, to_find,'F');
										
									}else if(distance[SHRC] <= distance[SUCC] && distance[SHRC] < distance[LCL] && shortcut.fd != -1){										
										head = insertUnsortedLink(head, identifier, to_find,'F');
										sprintf(answer,"FND %s %d %s %s %s", to_find, identifier,local.key, local.ip, local.port); 
										printf("[-] Enviei para o Atalho: %s\n", answer);
										do{
											n = sendto(shortcut.fd,answer,strlen(answer),0,res_short->ai_addr,res_short->ai_addrlen);
											if(n==-1){
												printf("[X] Erro na função <sendto>\n");
												close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
											}//error
											counter++;
											if(counter > 1){printf("[@] Tentar de novo\n");}											
										}while(recv_ack(shortcut.fd) && counter <= MAX_TIME_OUT);
										
										if(counter == (MAX_TIME_OUT + 1)){
											printf("[X] Não foi possível enviar a mensagem UDP\n");
											if(successor.fd != -1){
												printf("[*] Tentar via TCP\n");
												printf("[-] Enviei para o Sucessor: ");
												if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}												
											}
										}	
										
									}else if(distance[LCL] < distance[SUCC] && distance[LCL] < distance[SHRC]){
										printf("[*] Os dados que procura estão nesta máquina\n");
									}							
								}else{
									printf("[*] Por favor use: f/find i\n");
								}
							}else{
								printf("[*] Por favor use: f/find i\n");
							}
						}else if(!strcmp(buffer,"s\n") || !strcmp(buffer,"show\n")){
							printf("\n[*] Info predecessor -> key: %s | Ip: %s | Port: %s |\n",predecessor.key,predecessor.ip,predecessor.port);
							printf("[*] Info local -> key: %s | Ip: %s | Port: %s |\n",local.key,local.ip,local.port);
							printf("[*] Info sucessor -> key: %s | Ip: %s | Port: %s |\n",successor.key,successor.ip,successor.port);
							printf("[*] Info atalho -> key: %s | Ip: %s | Port: %s |\n\n",shortcut.key,shortcut.ip,shortcut.port);
						}else if(!strncmp(buffer,"c",1) ||!strncmp(buffer,"chord",5)){
							if(shortcut.fd != -1){
								printf("[*] Já existe um atalho, para criar um novo faça dc/deletechord primeiro\n");
							}else{
								if(sscanf(buffer,"%s %s %s %s", garbage, shortcut.key, shortcut.ip, shortcut.port) == 4){
									if(key_check(shortcut.key) != -1 && port_check(shortcut.port) != -1){								
										// Neste comando não é suposto comunicar nada, apenas se deve fazer a conexão, o nodo que é o destino do atalho não tem que o saber
										shortcut.fd = create_socket(&hints, &res_short, shortcut, "UDP", "CLIENT");
										if(shortcut.fd == -1){freeaddrinfo(res_short);close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}	
										printf("[*] Atalho criado\n");
									}else{
										printf("[*] Por favor use: c/chord i ip port\n");
									}							
								}else{
									printf("[*] Por favor use: c/chord i ip port\n");
								}
							}
						}else if(!strcmp(buffer,"d\n") || !strcmp(buffer,"dchord\n")){
							if(shortcut.fd != -1){
								// Remover a conexão, o nodo que é o destino do atalho não tem que o saber							
								close(shortcut.fd);
								node_setup(&shortcut,"\0","\0","\0",-1);
								freeaddrinfo(res_short);
								printf("[*] Atalho removido\n");								
							}else{
								printf("[*] Não existe atalho criado\n");
							}
						}else if(!strcmp(buffer,"l\n") || !strcmp(buffer,"leave\n")){
							if((successor.fd == predecessor.fd) && (successor.fd != -1 && predecessor.fd != -1)){
								if(strcmp(local.key,successor.key) != 0 && strcmp(local.key,predecessor.key) != 0){
									sprintf(answer,"PRED %s %s %s\n", predecessor.key, predecessor.ip, predecessor.port);
									printf("[-] Enviei para o Predecessor/Sucessor: ");	
									if(send_msg(&predecessor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}
								}
								node_setup(&predecessor, "\0", "\0", "\0", predecessor.fd);
								node_setup(&successor, "\0", "\0", "\0", successor.fd);
							}else{
								if(successor.fd != -1){
									if(strcmp(local.key,successor.key) != 0){
										sprintf(answer,"PRED %s %s %s\n", predecessor.key, predecessor.ip, predecessor.port);
										printf("[-] Enviei para o Sucessor: ");
										if(send_msg(&successor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}
									}
									node_setup(&successor, "\0", "\0", "\0", successor.fd);
								}
								if(predecessor.fd != -1){
									node_setup(&predecessor, "\0", "\0", "\0", predecessor.fd);									
								}
							}
							if(shortcut.fd != -1){
								close(shortcut.fd);
								node_setup(&shortcut, "\0", "\0", "\0", -1);
								freeaddrinfo(res_short);
							}
							for(i = 0; i < 100; i++){
								if(list_intruders[i].no.fd != -1){
									close(list_intruders[i].no.fd);
									list_intruders[i].no.fd = -1;
								}
							}
							close(fd_tcp);
							FD_CLR(fd_tcp, &current_sockets);
							close(fd_udp);
							FD_CLR(fd_udp, &current_sockets);
							fd_tcp = fd_udp = -1;
							one_node_ring = 0;
							if(head != NULL){freeLink(head);}
							head = initLink();
							printf("[*] A sair do anel\n");
							if(!strcmp(predecessor.key, local.key) && !strcmp(local.key, successor.key)){
								printf("[*] Leave executado com sucesso\n\n");
								node_setup(&predecessor, "\0", "\0", "\0", -1);	
								node_setup(&successor, "\0", "\0", "\0", -1);									
							}
							
						}else{ // Print dos comandos possíveis de se fazer depois de fazer "new", "pentry" ou "bentry"
							printf("[*] Use s/show, f/find, c/chord, dc/deletechord ou l/leave\n");
						}

					}else if(!strcmp(buffer,"e\n") || !strcmp(buffer,"exit\n")){
						if((fd_tcp == -1) && (fd_udp == -1)){
							if(successor.fd != -1){FD_CLR(successor.fd, &current_sockets);}
							if(predecessor.fd != -1){FD_CLR(predecessor.fd, &current_sockets);}
							FD_CLR(STDIN_FILENO, &current_sockets);
							close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
						}else{
							printf("[*] Para usar e/exit deve fazer l/leave do anel atual\n");
						}
					}else{
						if(!strcmp(buffer,"n\n") || !strcmp(buffer,"new\n")){
							if(successor.fd != -1){FD_CLR(successor.fd, &current_sockets); close(successor.fd); successor.fd = -1;}
							if(predecessor.fd != -1){FD_CLR(predecessor.fd, &current_sockets); close(predecessor.fd); predecessor.fd = -1;}
							
							//Copiar para as estruturas o sucessor e o precessor
							one_node_ring = 1;
							node_cpy(&predecessor, &local);
							node_cpy(&successor, &local);
							
						}else if(!strncmp(buffer,"p",1) || !strncmp(buffer,"pentry",6)){
							if(sscanf(buffer,"%s %s %s %s", garbage, predecessor.key, predecessor.ip, predecessor.port) == 4){
								if(key_check(predecessor.key) != -1 && port_check(predecessor.port) != -1){
									if(successor.fd != -1){FD_CLR(successor.fd, &current_sockets); close(successor.fd); successor.fd = -1;}
									if(predecessor.fd != -1){FD_CLR(predecessor.fd, &current_sockets); close(predecessor.fd); predecessor.fd = -1;}
									
									//Criar as sockets, colocar no SELECT e estabelecer e enviar um SELF para o predecessor
									predecessor.fd = create_socket(&hints, &res, predecessor, "TCP", "CLIENT");
									freeaddrinfo(res);
									if(predecessor.fd == -1){close(3);close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}	
									sprintf(answer,"SELF %s %s %s\n", local.key, local.ip, local.port);
									printf("[-] Enviei para o Predecessor: ");
									if(send_msg(&predecessor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}
									FD_SET(predecessor.fd, &current_sockets);
								}else{
									printf("[*] Por favor use: p/pentry i ip port\n");
								}
							}else{
								printf("[*] Por favor use: p/pentry i ip port\n");
							}
						}else if(!strncmp(buffer,"b",1) || !strncmp(buffer,"bentry",6)){
							if(sscanf(buffer,"%s %s %s %s", garbage, intruder.key, intruder.ip, intruder.port) == 4){
								if(key_check(intruder.key) != -1 && port_check(intruder.port) != -1){
									if(successor.fd != -1){FD_CLR(successor.fd, &current_sockets); close(successor.fd); successor.fd = -1;}
									if(predecessor.fd != -1){FD_CLR(predecessor.fd, &current_sockets); close(predecessor.fd); predecessor.fd = -1;}	
									
									// Entrar em contacto com o Nó que foi dado
									intruder.fd = create_socket(&hints, &res, intruder, "UDP", "CLIENT");
									if(intruder.fd == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}	
									sprintf(answer,"EFND %s", local.key); 
									printf("[-] Enviei para o Atalho: %s\n", answer);
									
									do{
										n = sendto(intruder.fd,answer,strlen(answer),0, res->ai_addr, res->ai_addrlen);
										if(n==-1){
											printf("[X] Erro na função <sendto>\n");
											close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
										}//error
										counter++;
										if(counter > 1){printf("[@] Tentar de novo\n");}											
									}while(recv_ack(intruder.fd) && counter <= MAX_TIME_OUT);
									/* Procura inicializada */
									
									// Obtenção de uma resposta
									memset(&buffer, '\0', sizeof buffer);
									memset(&answer, '\0', sizeof answer);
									if(counter == (MAX_TIME_OUT + 1)){
										printf("[X] Não foi possível enviar a mensagem UDP\n");
									}else{
										while(1){ 
											n = recvfrom(intruder.fd, buffer, 256, 0, &client, &addrlen);
											if(n == -1){
												printf("[X] Erro na função <recvfrom>\n");
												freeaddrinfo(res);
												close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
											}
											if(!strncmp(buffer,"EPRED",5)){
												if(sscanf(buffer,"%s %s %s %s", garbage, predecessor.key, predecessor.ip, predecessor.port) == 4 ){
													printf("[+] Recebido por UDP: %s\n", buffer);
													sprintf(answer,"ACK");
													n = sendto(intruder.fd, answer, 3, 0, res->ai_addr, res->ai_addrlen);		
													if(n==-1){
														printf("[X] Erro na função <sendto>\n");
														freeaddrinfo(res);
														close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
													}//error
													printf("[-] Enviei ACK\n");
													freeaddrinfo(res);
													
													//Enviar "SELF"
													predecessor.fd = create_socket(&hints, &res, predecessor, "TCP", "CLIENT");
													freeaddrinfo(res);
													if(predecessor.fd == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}	
													sprintf(answer,"SELF %s %s %s\n", local.key, local.ip, local.port);
													printf("[-] Enviei para o Predecessor: ");
													if(send_msg(&predecessor.fd, answer) == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}
													FD_SET(predecessor.fd, &current_sockets);

													break;
												}else{
													printf("[X] Erro na mensagem <EPRED>\n");
													freeaddrinfo(res);
													close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);
													break;											
												}
											}else{
												printf("[X] Resposta UDP não esperada\n");
												freeaddrinfo(res);
												break;
											}
										}
									}					
									if(predecessor.fd == -1){printf("[X] Bentry executado sem sucesso\n\n");}
									close(intruder.fd);									
								}else{
									printf("[*] Por favor use: b/bentry i ip port\n");
								}	
							}else{
								printf("[*] Por favor use: b/bentry i ip port\n");
							}					
						}else{
							printf("[*] Use n/new, p/pentry ou b/bentry para iniciar, ou e/exit para fechar o programa\n"); // Mensagem recebida pelo utilizador caso não use nenhum comando correto
						}
						
						// Criação de servidor UDP e TCP
						if((one_node_ring == 1) || (predecessor.fd != -1)){ // Fez pentry ou new ou bentry com sucesso
							if(intruder.fd != -1){close(intruder.fd);node_setup(&intruder, "\0", "\0", "\0", -1);}
							fd_tcp = create_socket(&hints, &res, local, "TCP", "SERVER");
							freeaddrinfo(res);
							if(fd_tcp == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}							
							FD_SET(fd_tcp, &current_sockets);
							fd_udp = create_socket(&hints, &res, local, "UDP", "SERVER");
							freeaddrinfo(res);
							if(fd_udp == -1){close_and_exit(&local, &predecessor, &successor, &intruder, &shortcut, fd_tcp, fd_udp, newfd, &head, &res_short);}
							FD_SET(fd_udp, &current_sockets);
							if(one_node_ring == 1){printf("[*] Anel criado\n");}
						}
					}
				}
			}
		}
	}
	
	return 0;
}
