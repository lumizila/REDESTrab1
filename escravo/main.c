#include "../utils.c"

int main(){
  printf("\niniciando programa escravo(servidor)!\n");
  int soquete = ConexaoRawSocket("eno1");
	//tipo de mensagem a ser enviada/recebida
	short tipo = 0;
	//numero de sequencia da mensagem a ser enviada
	short sequencia = 0;
	//numero de sequencia da mensagem recebida
	short seqRec = 0;
	//tamanho de DADOS da mensagem recebida
	short tamRec = 0;
	//mensagem recebida por completo
	unsigned char msgRec[MSG_SIZE];
	//bits de DADOS da msg recebida
	unsigned char dataRec[DATA_SIZE];
	//para guardar o input do usuario
	char *input;
	//TODO alterar para subs ter tamanho qualquer
	char *subs[3];
	//guarda o PATH do diretorio corrente
	char path[MAX_INPUT] = "./";
	//apenas para guardar comandos locais
	char localCommand[DATA_SIZE];

	//auxiliares
	int i;
	while(true){
		//recebe mensagem
		int r = read(soquete, msgRec, MSG_SIZE);
		//desempacota mensagem
		strcpy(dataRec, "");
		int status = desempacotaMsg(msgRec, dataRec, &seqRec, &tamRec, &tipo);
		dataRec[tamRec] = 0x00;
		//se retornou 0, nao houve erro, se retornou -1 inicio nao confere,
		//-2 paridade não confere.
		//se inicio nao confere soh ignora a mensagem
		//confere se paridade incorreta responder com NACK
		if(status == -2){
			//empacota NACK, SEQ = NUMERO DE SEQ DA MENSAGEM RECEBIDA
			printf("Mensagem recebida com paridade incoreta, enviando nack.\n");
			unsigned char msgEnviar[MSG_SIZE];
			empacotaMsg("", msgEnviar, NACK, seqRec, 0);
			write(soquete, msgEnviar, OVERLOAD_SIZE);
			sequencia = aumentaSeq(sequencia);
		}
		//confere o tamanho da mensagem
		//confere qual a sequencia da mensagem
		if(status == 0){
			//cria resposta adequada para cada tipo de mensagem
			switch(tipo){
				case 6: //cd
					printf("Recebi um cd: %s\n", dataRec);
					/*getting the first substring*/
					input = strtok(dataRec, " \n");
					/*walking trough the other substrings*/
					i = 0;
					while(input != NULL){
						subs[i] = input;
						input = strtok(NULL, " \n");
						i = i+1;
					}
					int error = mudaDir(subs[1]);
					//responde com ACK
					if(error == 0){
						unsigned char msgEnviar[MSG_SIZE];
						empacotaMsg("", msgEnviar, OK, seqRec, 0);
						write(soquete, msgEnviar, OVERLOAD_SIZE);
						apagaRelativos(subs[1]);
						//se nao houve erro, guardar no diretorio corrente
						//TODO para o ls, checar se da certo o ls de acordo com o cd
						strcpy(path, subs[1]);
					}
					//Se ERRO responde com o cod do erro
					else{
						printf("segfault?\n");
						unsigned char msgEnviar[MSG_SIZE];
						if(error == EACCES){
							empacotaMsg(NAO_PERMITIDO, msgEnviar, ERRO, seqRec, sizeof(NAO_PERMITIDO));
							printf("nao permitido\n");
							fflush(stdout);
							write(soquete, msgEnviar, sizeof(NAO_PERMITIDO)+OVERLOAD_SIZE);
						}
						else{
							empacotaMsg(NAO_EXISTE, msgEnviar, ERRO, seqRec, sizeof(NAO_EXISTE));
							printf("nao existe\n");
							fflush(stdout);
							write(soquete, msgEnviar, sizeof(NAO_EXISTE)+OVERLOAD_SIZE);
						}
					}
					break;
				case 7: //ls
					printf("Recebi um ls: %s\n", dataRec);
                    fflush(stdout);
                    int errorLS = 0;
                    char bufferLS[DATA_SIZE];
                    unsigned char resultadoLS[DATA_SIZE];
                    strcpy(bufferLS, "");
                    strcpy(resultadoLS, "");
                    
                    //aqui eu guardo cada opção do ls em subs[1] em diante
					input = strtok(dataRec, " \n");
					i = 0;
					while(input != NULL){
						subs[i] = input;
						input = strtok(NULL, " \n");
						i = i+1;
					}

                    //numeroOpcoes guarda a quantidade de argumentos em subs
		            int numeroOpcoes = i;

                    //aqui eu concateno o "ls" com as opções que estão guardadas em subs[1] em diante e armazeno em localCommand
                    strcpy(localCommand, "ls");
                    for(int i = 1; i < numeroOpcoes; i++){
	                    strcat(localCommand, " ");
	                    strcat(localCommand, subs[i]);
                    }
                    strcat(localCommand, "\n");

			        //executa o ls e salva o resultado em resultadoLS
			        FILE *lsofFile_p = popen(localCommand, "r");
                    if (!lsofFile_p) {
                        errorLS = 1;
                    }

                    while (!feof(lsofFile_p)) {
                        if (fgets(bufferLS, sizeof(bufferLS), lsofFile_p) != NULL) {
                            strcat(resultadoLS, bufferLS);
                        }
                    }
                    pclose(lsofFile_p);
					printf("Resultado ls: %s", resultadoLS);
                    fflush(stdout);
                    if (errorLS == 0) {
                        unsigned char msgEnviar[MSG_SIZE];
                        empacotaMsg(resultadoLS, msgEnviar, MOSTRA, seqRec, sizeof(resultadoLS));
                        write(soquete, msgEnviar, sizeof(resultadoLS)+OVERLOAD_SIZE);
                    }

                    //salvando resultadoLS em arquivo tmp
                    /*FILE *f;
                    f = fopen("tmp", "w");
                    if (f == NULL) {
                        printf("Erro ao criar arquivo tmp");
                    }
                    fprintf(f,"%s", resultadoLS);
                    fclose(f);

                    long long int tam_arquivoLS = tamArquivo("tmp");
                    //envia o arquivo com o resultado do ls
					if(errorLS == 0){
						enviaArquivo("tmp", soquete, tam_arquivoLS, &seqRec);
					}*/

					break;
				case 8: //get
					//TODO Responde com ACK/ERRO, se foi um ACK enviar o TAM do arquivo e os dados e o OK
					break;
				case 9: //put
					printf("Recebi um put: %s\n", dataRec);
					fflush(stdout);
					/*getting the first substring*/
					input = strtok(dataRec, " \n");
					/*walking trough the other substrings*/
					i = 0;
					while(input != NULL){
						subs[i] = input;
						input = strtok(NULL, " \n");
						i = i+1;
					}
					char nomeArq[FILE_NAME];
					strcpy(nomeArq, subs[1]);
					//checa se pode escrever neste diretorio
					if(access("./", W_OK) == 0){
						//Responde com OK
						unsigned char msgEnviar[MSG_SIZE];
						empacotaMsg("", msgEnviar, OK, seqRec, 0);
						printf("pode escrever neste diretorio.\n");
						fflush(stdout);
						write(soquete, msgEnviar, OVERLOAD_SIZE);
						//se foi um OK, recebe o tamanho do arquivo
						while(true){
							//recebe mensagem
							int r = read(soquete, msgRec, MSG_SIZE);
							//desempacota mensagem
							strcpy(dataRec, "");
							int status = desempacotaMsg(msgRec, dataRec, &seqRec, &tamRec, &tipo);
							//se inicio nao confere soh ignora a mensagem
							//se paridade nao confere envia nack
							if(status == -2){
								//empacota NACK, SEQ = NUMERO DE SEQ DA MENSAGEM RECEBIDA
								printf("Mensagem recebida com paridade incoreta, enviando nack.\n");
								unsigned char msgEnviar[MSG_SIZE];
								empacotaMsg("", msgEnviar, NACK, seqRec, 0);
								write(soquete, msgEnviar, OVERLOAD_SIZE);
								sequencia = aumentaSeq(sequencia);
							}
							else if(status == 0){
								if(checaMemoria("./", dataRec) == 0){
									//se tem memoria suficiente, responde com ACK
									empacotaMsg("", msgEnviar, OK, seqRec, 0);
									dataRec[tamRec] = 0x00;
									printf("tem espaco para escrever arq de tam: %s\n", dataRec);
									fflush(stdout);
									write(soquete, msgEnviar, OVERLOAD_SIZE);
									recebeArquivo(nomeArq, soquete, atoll(dataRec));
								}
								else{
									//se nao tem memoria suficiente, responde com ERRO
									empacotaMsg(NAO_ESPACO, msgEnviar, ERRO, seqRec, sizeof(NAO_ESPACO));
									printf("nao tem espaco para escrever arq de tam: %s\n", msgEnviar);
									fflush(stdout);
									write(soquete, msgEnviar, sizeof(NAO_ESPACO)+OVERLOAD_SIZE);
								}
								break;
							}
						}
					}
					//ERRO(se nao tem permissao de escrita)
					else{
						unsigned char msgEnviar[MSG_SIZE];
						empacotaMsg(NAO_PERMITIDO, msgEnviar, ERRO, seqRec, sizeof(NAO_PERMITIDO));
						printf("nao permitido: %s\n", msgEnviar);
						fflush(stdout);
						write(soquete, msgEnviar, sizeof(NAO_PERMITIDO)+OVERLOAD_SIZE);
					}
					break;
				default:
					printf("ERRO: o tipo de comando não confere\n");
			}
		}
	}
}
