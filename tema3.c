#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>

#define MASTER 0
#define H 1
#define C 2
#define F 3
#define SF 4

static char *in_filename;
static char out_filename[20];
static char in_filename_aux[20];
static pthread_barrier_t barrier;
static char text[600000][2000]; // retin in el textul initial procesat
static int index_line = 0; // retin nr de linii al textului procesat

// functia de thread pt citirea in paralel
void *thread_function(void *arg) {
  int thread_id = *(int *)arg;
  FILE *in_file; // fisierul de intrare
  char *line = NULL; // linia din fisier
  size_t len = 0;
  char arr[2500][1100]; // paragraful initial trimis spre procesare
  char received_arr[2500][2000]; // paragraful procesat primit
  int ok = 0, i = 0, nr_lines = 0, j;
  int nr_char_received, nr_read_char;

  // deschide fisierul de intrare
  in_file = fopen(in_filename_aux, "r");
  // sincronizez thread-urile intre ele printr-o bariera declarata anterior
  pthread_barrier_wait(&barrier);
  // se verifica daca fisierul exista
	if (in_file == NULL) {
		printf("Eroare la deschiderea fisierului de intrare!\n");
		exit(1);
	}
  // asociez thread-ul 0 cu genul HORROR
  if (thread_id == 0) {
    int line_in_file = 0; // numarul liniei din fisier
    int left_bound; // linia de la care incepe paragraful
    int eof_found = 0; // daca s-a gasit sau nu sfarsitul de fisier (0 -> nu)
    // parcurge fisierul pe linii
    while (eof_found != 1) {
      // obtin pe rand cate o linie din fisier
      ssize_t c = getline(&line, &len, in_file);
      // s-a gasit sfarsitul fisierului
      if (c == -1) {
        eof_found = 1;
      }
      // verifica daca linia contine genul horror
      if (strcmp(line, "horror\n") == 0) {
        ok = 1; // s-a gasit un paragraf horror
        left_bound = line_in_file; // salvez linia de unde incepe el
      }
      nr_read_char = strlen(line) - 1; // nr de caractere ale liniei fara '\0'
      // daca s-a terminat de citit un paragraf sau, separat, ultimul paragraf
      if (nr_read_char == 0 || eof_found == 1) {
        ok = 0; // paragraful horror poate fi trimis worker-ului corespondent
      }
      // inca se mai citeste paragraful horror din fisier
      if (ok == 1) {
        // doar daca ultimul caracter de pe linie este new line
        // il sterg din ea (adica din toate in afara de ultima linie din fisier)
        if (line[strlen(line) - 1] == '\n') {
          line[strlen(line) - 1] = '\0';
        }
        // adaug linia in vectorul de string-uri, considerat ca paragraf horror
        strcpy(arr[i++], line);
      }
      else {
        // salvez nr de linii din paragraful ce urmeaza a fi trimis
        // incluzand linia cu genul
        nr_lines = i;
        // ma asigur ca trimit worker-ului HORROR
        // un paragraf cu nr nevid de linii
        if (nr_lines > 0) {
          // trimit nr de linii worker-ului HORROR
          MPI_Send(&nr_lines, 1, MPI_INT, H, 0, MPI_COMM_WORLD);
          // trimit paragraful horror worker-ului HORROR
          MPI_Send(&arr, 2500*1100, MPI_CHAR, H, 0, MPI_COMM_WORLD);
          // primesc paragraful horror procesat de la worker-ul HORROR
          MPI_Recv(&received_arr, 2500*2000, MPI_CHAR, H, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          // adaug paragraful procesat in bucata corespunzatoare
          // ,de la linia left_bound, in vectorul de string-uri text
          // respectandu-se astfel ordinea initiala a paragrafelor
          for (j = 0; j < nr_lines; j++) {
            strcpy(text[left_bound++], received_arr[j]);
          }
          // se schimba nr de linii ale textului in functie de left_bound
          // , adica daca se itereaza la o bucata mai indepartata decat
          // cea anterioara
          if(left_bound > index_line) {
            index_line = left_bound;
          }
        }
        // golesc spatiul alocat salvarii paragrafului horror
        // pentru a adauga un alt potential paragraf din fisier
        i = 0;
        for (j = 0; j < 2500; j++) {
          strcpy(arr[j], "\0");
        }
      }
      // incrementez nr de linii parcurse din fisier
      line_in_file++;
    }
  }
  // pun bariera pentru a termina de trimis/primit paragrafele horror
  pthread_barrier_wait(&barrier);
  // asociez thread-ul 1 cu genul COMEDY
  // aceeasi logica ca la thread-ul 0 pt HORROR
  if (thread_id == 1) {
    int line_in_file = 0;
    int left_bound;
    int eof_found = 0;
    while (eof_found != 1) {
      ssize_t c = getline(&line, &len, in_file);
      if (c == -1) {
        eof_found = 1;
      }
      if (strcmp(line, "comedy\n") == 0) {
        ok = 1;
        left_bound = line_in_file;
      }
      nr_read_char = strlen(line) - 1;
      if (nr_read_char == 0 || eof_found == 1) {
        ok = 0;
      }
      if (ok == 1) {
        if (line[strlen(line) - 1] == '\n') {
          line[strlen(line) - 1] = '\0';
        }
        strcpy(arr[i++], line);
      }
      else {
        nr_lines = i;
        if (nr_lines > 0) {
          MPI_Send(&nr_lines, 1, MPI_INT, C, 0, MPI_COMM_WORLD);
          MPI_Send(&arr, 2500*1100, MPI_CHAR, C, 0, MPI_COMM_WORLD);
          MPI_Recv(&received_arr, 2500*2000, MPI_CHAR, C, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          for (j = 0; j < nr_lines; j++) {
            strcpy(text[left_bound++], received_arr[j]);
          }
          if(left_bound > index_line) {
            index_line = left_bound;
          }
        }
        i = 0;
        for (j = 0; j < 2500; j++) {
          strcpy(arr[j], "\0");
        }
      }
      line_in_file++;
    }
  }
  // pun bariera pentru a termina de trimis/primit paragrafele horror, comedy
  pthread_barrier_wait(&barrier);
  // asociez thread-ul 2 cu genul FANTASY
  // aceeasi logica ca la celelalte thread-uri
  if (thread_id == 2) {
    int line_in_file = 0;
    int left_bound;
    int eof_found = 0;
    while (eof_found != 1) {
      ssize_t c = getline(&line, &len, in_file);
      if (c == -1) {
        eof_found = 1;
      }
      if (strcmp(line, "fantasy\n") == 0) {
        ok = 1;
        left_bound = line_in_file;
      }
      nr_read_char = strlen(line) - 1;
      if (nr_read_char == 0 || eof_found == 1) {
        ok = 0;
      }
      if (ok == 1) {
        if (line[strlen(line) - 1] == '\n') {
          line[strlen(line) - 1] = '\0';
        }
        strcpy(arr[i++], line);
      }
      else {
        nr_lines = i;
        if (nr_lines > 0) {
          MPI_Send(&nr_lines, 1, MPI_INT, F, 0, MPI_COMM_WORLD);
          MPI_Send(&arr, 2500*1100, MPI_CHAR, F, 0, MPI_COMM_WORLD);
          MPI_Recv(&received_arr, 2500*2000, MPI_CHAR, F, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          for (j = 0; j < nr_lines; j++) {
            strcpy(text[left_bound++], received_arr[j]);
          }
          if(left_bound > index_line) {
            index_line = left_bound;
          }
        }
        i = 0;
        for (j = 0; j < 2500; j++) {
          strcpy(arr[j], "\0");
        }
      }
      line_in_file++;
    }
  }
  // pun bariera pentru a termina de trimis/primit celelalte paragrafe
  pthread_barrier_wait(&barrier);
  // asociez thread-ul 3 cu genul SCIENCE-FICTION
  // aceeasi logica ca la celelalte thread-uri
  if (thread_id == 3) {
    int line_in_file = 0;
    int left_bound;
    int eof_found = 0;
    while (eof_found != 1) {
      ssize_t c = getline(&line, &len, in_file);
      if (c == -1) {
        eof_found = 1;
      }
      if (strcmp(line, "science-fiction\n") == 0) {
        ok = 1;
        left_bound = line_in_file;
      }
      nr_read_char = strlen(line) - 1;
      if (nr_read_char == 0 || eof_found == 1) {
        ok = 0;
      }
      if (ok == 1) {
        if (line[strlen(line) - 1] == '\n') {
          line[strlen(line) - 1] = '\0';
        }
        strcpy(arr[i++], line);
      }
      else {
        nr_lines = i;
        if (nr_lines > 0) {
          MPI_Send(&nr_lines, 1, MPI_INT, SF, 0, MPI_COMM_WORLD);
          MPI_Send(&arr, 2500*1100, MPI_CHAR, SF, 0, MPI_COMM_WORLD);
          MPI_Recv(&received_arr, 2500*2000, MPI_CHAR, SF, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          for (j = 0; j < nr_lines; j++) {
            strcpy(text[left_bound++], received_arr[j]);
          }
          if(left_bound > index_line) {
            index_line = left_bound;
          }
        }
        i = 0;
        for (j = 0; j < 2500; j++) {
          strcpy(arr[j], "\0");
        }
      }
      line_in_file++;
    }
  }
  pthread_barrier_wait(&barrier);
  fclose(in_file);
  pthread_exit(NULL);
}
// functie folosita la inversarea cuvantului
char *strrev(char *str) {
   char *p1, *p2;
   if (! str || ! *str) {
     return str;
   }
   for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2) {
     *p1 ^= *p2;
     *p2 ^= *p1;
     *p1 ^= *p2;
   }
   return str;
}
// functia main in care se vor procesa paragrafele
int main (int argc, char *argv[])
{
    int numtasks, rank, len;
    char hostname[MPI_MAX_PROCESSOR_NAME];
    int provided;
    int h_count = 0, c_count = 0, f_count = 0, sf_count = 0;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Get_processor_name(hostname, &len);

    pthread_t tid[4]; // vectorul de thread-uri
    int thread_id[4];
    int i;
    // initializez bariera pentru 4 thread-uri
    int b = pthread_barrier_init(&barrier, NULL, 4);
	  if(b != 0) {
		  printf("Eroare la initializarea barierei");
		  exit(-1);
	  }
    // nodul MASTER
    if(rank == MASTER) {
      in_filename = argv[1]; // ia numele fisierului din linie de comanda
      // se afla numele fisierului de iesire
      strcpy(in_filename_aux, in_filename);
      in_filename[strlen(in_filename) - 3] = '\0'; // sterg extensia txt
      strcpy(out_filename, in_filename);
      strcat(out_filename, "out"); // adaug extensia out

      FILE *in_file; // fisierul de intrare
      FILE *out_file; // fisierul de iesire
      // deschid fisierul de intrare pt citirea
      in_file = fopen(in_filename_aux, "r");
      // creez si deschid fisierul de iesire pt scriere
      out_file = fopen(out_filename, "w");
      // verific daca cele 2 fisiere exista
      if (in_file == NULL) {
    		printf("Eroare la deschiderea fisierului de intrare!\n");
    		exit(1);
    	}
      if (out_file == NULL) {
    		printf("Eroare la deschiderea fisierului de iesire!\n");
    		exit(1);
    	}
      // parcurg fisierul de intrare pe linii
      // si contorizez nr de paragrafe din fiecare gen in parte
      char *line = NULL;
      size_t len = 0;
      while (getline(&line, &len, in_file) != -1) {
        if (strcmp(line, "horror\n") == 0) {
          h_count++; // nr paragrafe horror
        }
        if (strcmp(line, "comedy\n") == 0) {
          c_count++; // nr paragrafe comedy
        }
        if (strcmp(line, "fantasy\n") == 0) {
          f_count++; // nr paragrafe fantasy
        }
        if (strcmp(line, "science-fiction\n") == 0) {
          sf_count++; // nr paragrafe sf
        }
      }
      fclose(in_file);
      // trimit nr de paragrafe din fiecare gen
      // in parte worker-ului corespunzator
      MPI_Send(&h_count, 1, MPI_INT, H, 0, MPI_COMM_WORLD);
      MPI_Send(&c_count, 1, MPI_INT, C, 0, MPI_COMM_WORLD);
      MPI_Send(&f_count, 1, MPI_INT, F, 0, MPI_COMM_WORLD);
      MPI_Send(&sf_count, 1, MPI_INT, SF, 0, MPI_COMM_WORLD);
      // creez cele 4 thread-uri ale nodului MASTER, folosite la citire
	    for (i = 0; i < 4; i++) {
		      thread_id[i] = i;
		      pthread_create(&tid[i], NULL, thread_function, &thread_id[i]);
	    }
      // pornesc cele 4 thread-uri
	    for (i = 0; i < 4; i++) {
		      pthread_join(tid[i], NULL);
	    }
      // dupa ce sunt prelucrate toate paragrafele din fisier
      // sunt afisate in fisierul de iesire corespunzator
      for (int j = 0; j < index_line; j++) {
        fputs(text[j], out_file);
        fputs("\n", out_file);
      }
      fclose(out_file);
    }
    // nod WORKER care poate fi {HORROR, COMEDY, FANTASY, SF}
    else {
      int nr_lines, count;
      char arr[2500][1100]; // paragraful ce va fi primit de la MASTER
      char processed_arr[2500][2000]; // paragraful procesat
      // se primeste nr de paragrafe din genul respectiv de la MASTER
      MPI_Recv(&count, 1, MPI_INT, MASTER, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      // se primesc toate paragrafele de acel gen pe rand de la MASTER
      // din thread-ul corespondent genului, impreuna cu nr de linii pt fiecare
      for (i = 0; i < count; i++) {
        MPI_Recv(&nr_lines, 1, MPI_INT, MASTER, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&arr, 2500*1100, MPI_CHAR, MASTER, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // se proceseaza paragraful primit corespunzator
        // in functie de gen pe linii
        for (int i = 0; i < nr_lines; i++) {
          // pentru horror se dubleaza consoanele din fiecare cuvant
          if (rank == H) {
            strcpy(processed_arr[i], "\0");
            char consonants[] = "bBcCdDfFgGhHjJkKlLmMnNpPqQrRsStTvVwWxXyYzZ";
            for (int ch = 0; ch < strlen(arr[i]); ch++) {
              if(strchr(consonants, arr[i][ch]) != NULL && i != 0) {
                strncat(processed_arr[i], &arr[i][ch], 1);
                char aux = arr[i][ch];
                if (arr[i][ch] >= 65 && arr[i][ch] <= 90) {
                  aux = arr[i][ch] + 32;
                }
                strncat(processed_arr[i], &aux, 1);
              }
              else {
                strncat(processed_arr[i], &arr[i][ch], 1);
              }
            }
          }
          // pentru comedy fiecare litera de pe pozitie para din
          // fiecare cuvant va fi facuta majuscula
          else if (rank == C) {
            char *token;
            strcpy(processed_arr[i], "\0");
            token = strtok(arr[i], " ");
            while (token != NULL) {
              for (int ch = 0; ch < strlen(token); ch++) {
                if ((ch % 2 == 1) && (token[ch] >= 'a' && token[ch] <= 'z') && i != 0) {
                  token[ch] = token[ch] - 32;
                }
              }
              char space = ' ';
              strcat(processed_arr[i], token);
              strncat(processed_arr[i], &space, 1);
              token = strtok(NULL, " ");
            }
          }
          // pentru fantasy prima litera a fiecarui cuvant
          // va fi facuta majuscula
          else if (rank == F) {
            char *token;
            strcpy(processed_arr[i], "\0");
            token = strtok(arr[i], " ");
            while (token != NULL) {
              if ((token[0] >= 'a' && token[0] <= 'z') && i != 0) {
                token[0] = token[0] - 32;
              }
              char space = ' ';
              strcat(processed_arr[i], token);
              strncat(processed_arr[i], &space, 1);
              token = strtok(NULL, " ");
            }
          }
          // pentru science-fiction, fiecare al 7-lea cuvant de pe linie
          // va fi inversat (din 7 in 7)
          else {
            char *token;
            int count_words = 1;
            strcpy(processed_arr[i], "\0");
            token = strtok(arr[i], " ");
            while (token != NULL) {
              if (count_words % 7 == 0 && i != 0) {
                strcat(processed_arr[i], strrev(token));
              }
              else {
                strcat(processed_arr[i], token);
              }
              char space = ' ';
              strncat(processed_arr[i], &space, 1);
              token = strtok(NULL, " ");
              count_words++;
            }
          }
        }
        // trimite paragraful procesat catre MASTER
        MPI_Send(&processed_arr, 2500*2000, MPI_CHAR, MASTER, 0, MPI_COMM_WORLD);
      }
    }
    // se distruge bariera folosita la sincronizare
    int b2 = pthread_barrier_destroy(&barrier);
	  if(b2 != 0) {
		  printf("Eroare la dezalocarea barierei");
		  exit(-1);
	  }
    MPI_Finalize();
    return 0;
}
