#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#define CANT 6

/////////////////////////////////////////////////////////////////
void *connection_handler(void *); //el hilo que hace todas las cosas

struct infoParaElHilo
{
	char ext[10];
	char path[90];
};

struct infoDesdeElHilo
{
	int cantDeArchivos;
	size_t archMasChico;
	size_t archMasGrande;
	int usr;
	int usrNum;
};

int main(int argc, char *argv[])
{

	if (argc != 2)
	{
		//verifico la cantidad de parametros
		printf(
			"uso correcto: %s directorio. \nPara mas informacion ejecute %s -?\n",
			argv[0], argv[0]);
	}
	else if (strcmp(argv[1], "-?") == 0)
	{
		//ayuda
		printf("El script analiza archivos en un directorio enviado por parametro, generando una estadistica de los siguientes valores, agrupados por cada extension de archivo distinta (txt, doc, c, jpg, etc. ).\no Cantidad de archivos totales\no Cantidad de archivos\no Tamaño del archivo más chico\no Tamaño del archivo más grande\no Usuario dueño de la mayor cantidad de archivos y cuántos son \nel analisis se paraleliza para optimizar el tiempo de ejecucion.\n");
	}
	else
	{
		//veo si hay directorio
		DIR *dir = opendir(argv[1]);
		if (dir)
		{
			/* el directorio existe */

			pthread_t hilo[CANT];					//declaro los hilos
			void *var;								//la variable donde vuelve todo
			struct infoParaElHilo *isaliente[CANT]; //la variable donde va todo

			//inicializo lo que le voy a mandar
			for (int i = 0; i < CANT; i++)
			{
				isaliente[i] = (struct infoParaElHilo *)malloc(sizeof(struct infoParaElHilo));
				strcat(isaliente[i]->path, argv[1]);
			}
			strcat(isaliente[0]->ext, "txt");
			strcat(isaliente[1]->ext, "jpg");
			strcat(isaliente[2]->ext, "mp3");
			strcat(isaliente[3]->ext, "csv");
			strcat(isaliente[4]->ext, "c");
			strcat(isaliente[5]->ext, "doc");

			//lanzo los hilos
			for (int i = 0; i < CANT; i++)
			{
				if (pthread_create(&hilo[i], NULL, connection_handler,
								   (void *)isaliente[i]) < 0)
				{
					perror("could not create thread");
					return 1;
				}
			}

			//recibe los hilos e imprime por pantalla lo que devolvieron
			for (int i = 0; i < CANT; i++)
			{
				pthread_join(hilo[i], &var);
				if ((struct infoDesdeElHilo *)var != NULL && ((struct infoDesdeElHilo *)var)->cantDeArchivos != 0)
				{
					printf("-----------------------------------------------------------------\n");
					printf("El hilo %ld \nanalizo las extensiones %s\n", hilo[i], isaliente[i]->ext);
					printf("analizo %d archivos en total\n", ((struct infoDesdeElHilo *)var)->cantDeArchivos);
					printf("el archivo mas chico que encontro pesaba %ld bytes\n", ((struct infoDesdeElHilo *)var)->archMasChico);
					printf("el archivo mas grande que encontro pesaba %ld bytes\n", ((struct infoDesdeElHilo *)var)->archMasGrande);
					printf("el usuario %d tiene %d archivos\n", ((struct infoDesdeElHilo *)var)->usr, ((struct infoDesdeElHilo *)var)->usrNum);
				}
			}

			closedir(dir);
		}
		else if (ENOENT == errno)
		{
			/* el directorio no existe */
			printf(
				"%s no es un directorio valido \nPara mas informacion ejecute %s -?\n",
				argv[1], argv[0]);
		}
		else
		{
			/* no se, ojala no caiga nunca aca, pero lo dejo por las dudas */
			return 100;
		}
	}
	return 0;
}

/*
 * Esto maneja cada extension y devuelve la estadistica
 * */
void *connection_handler(void *data)
{

	struct infoParaElHilo *datos = (struct infoParaElHilo *)data;

	DIR *dir;
	struct dirent *ent;
	struct stat finfo;
	struct infoDesdeElHilo *resumen = (struct infoDesdeElHilo *)malloc(sizeof(struct infoDesdeElHilo));
	resumen->cantDeArchivos = 0;
	resumen->archMasGrande = 0;
	resumen->usrNum = 0;
	resumen->archMasChico = 99999;

	if ((dir = opendir(datos->path)) != NULL)
	{
		/* yo se que es una solucion desprolija, y en otro lenguaje hubiese hecho un set y listo,
		pero como c es un tanto mas primitivo voy a usar un vector para contar los usuarios y cuantos archivos tiene cada uno.*/
		unsigned int usrMat[100][2];
		int ultimoUsuarioAgregado = 0;

		/* recorre todos los archivos y directorios adentro de dir */
		while ((ent = readdir(dir)) != NULL)
		{

			int len = strlen(ent->d_name);
			int len2 = 3;
			char buf[PATH_MAX + 1];
			if (len > len2 && memcmp(ent->d_name + len - len2, datos->ext, len2) == 0) // chequea la extension
			{
				//aumenta cantidad de archivos
				resumen->cantDeArchivos++;

				//concateno direccion y nombre para poder abrir el archivo desde cualquier lado
				strcpy(buf, datos->path);
				strcat(buf, "/");
				strcat(buf, ent->d_name);

				//lo abro
				int file = 0;
				if ((file = open(buf, O_RDONLY)) < -1)
					return (void *)200;
				if (-1 == file)
				{
					printf("\n open() failed with error [%s]\n", strerror(errno));
					return (void *)300;
				}

				//tiro fstat
				if (fstat(file, &finfo) > 0)
				{
					return (void *)400;
				}

				/* Aca estaba probando fstat, lo dejo comentado por las dudas*/
				//printf("File Size: \t\t%ld bytes\n", finfo.st_size);
				//printf("Number of Links: \t%ld\n", finfo.st_nlink);
				//printf("File owner: \t%d\n", finfo.st_uid);
				//printf("File inode: \t\t%ld\n", finfo.st_ino);

				//esto es lo que pide el ejercicio basicamente
				if (resumen->archMasGrande < finfo.st_size)
				{
					resumen->archMasGrande = finfo.st_size;
				}
				if (resumen->archMasChico > finfo.st_size)
				{
					resumen->archMasChico = finfo.st_size;
				}
				int entro = 0;
				for (int i = 0; i < ultimoUsuarioAgregado && i < 100; i++)
				{
					if (finfo.st_uid == usrMat[i][0])
					{
						entro = 1;
						usrMat[i][1]++; //aumento la cantidad de archivos de ese usuario
					}
				}
				if (!entro)
				{
					usrMat[ultimoUsuarioAgregado][0] = finfo.st_uid;
					usrMat[ultimoUsuarioAgregado][1] = 1;
					ultimoUsuarioAgregado++;
				}
			}
		}
		closedir(dir);

		for (int i = 0; i < ultimoUsuarioAgregado && i < 100; i++)
		{
			if (usrMat[i][1] > resumen->usrNum)
			{
				resumen->usr = usrMat[i][0];
				resumen->usrNum = usrMat[i][1];
			}
		}
	}
	else
	{
		/* could not open directory */
		perror("");
		return (void *)100;
	}

	return (void *)resumen;
}
