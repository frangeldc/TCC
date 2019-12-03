/*
 * The Game of Life
 *
 * a cell is born, if it has exactly three neighbours
 * a cell dies of loneliness, if it has less than two neighbours
 * a cell dies of overcrowding, if it has more than three neighbours
 * a cell survives to the next generation, if it does not die of loneliness
 * or overcrowding
 *
 * In this version, a 2D array of ints is used.  A 1 cell is on, a 0 cell is off.
 * The game plays a number of steps (given by the input), printing to the screen each time.  'x' printed
 * means on, space means off.
 *
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>	/* inclui a biblioteca time para calcular o tempo do laço for principal */
typedef unsigned char cell_t;

#define DEBUG = 1

int my_size;	/* size do MPI */
int my_rank;			/* rank do MPI */
int i_bar, i_bar_max, resto_i_bar;

cell_t ** allocate_board (int size) {
	cell_t ** board = (cell_t **) malloc(sizeof(cell_t*)*size);
	int	i;
	for (i=0; i<size; i++)
		board[i] = (cell_t *) malloc(sizeof(cell_t)*size);
	return board;
}

void free_board (cell_t ** board, int size) {
        int     i;
        for (i=0; i<size; i++)
                free(board[i]);
	free(board);
}


/* return the number of on cells adjacent to the i,j cell */
int adjacent_to (cell_t ** board, int size, int i, int j) {
	int	k, l, count=0;

	int sk = (i>0) ? i-1 : i;
	int ek = (i+1 < size) ? i+1 : i;
	int sl = (j>0) ? j-1 : j;
        int el = (j+1 < size) ? j+1 : j;

	for (k=sk; k<=ek; k++)
		for (l=sl; l<=el; l++)
			count+=board[k][l];
	count-=board[i][j];

	return count;
}

void play (cell_t ** board, cell_t ** newboard, int size) {
	int	i, j, a;
   MPI_Status status;

/* ---------- Troca de bordas ---------- */
  if (my_rank == 0){
		/* Envio da ultima linha para o rank de baixo */
    MPI_Send((&board[i_bar-1][0]), (size), MPI_CHAR, (my_rank+1), 0 ,MPI_COMM_WORLD);
		/* Recebimento da borda inferior */
		MPI_Recv((&board[i_bar][0]), (size), MPI_CHAR, (my_rank+1), 0, MPI_COMM_WORLD, &status);
   }
  else if ((my_rank != 0) && (my_rank != (my_size-1) ) && (i_bar == i_bar_max) ){
		/* Recebimento da borda inferior */
		MPI_Recv((&board[my_rank*i_bar-1][0]), (size), MPI_CHAR, (my_rank-1), 0, MPI_COMM_WORLD, &status);
		/* Envio da primeira linha para o rank de cima */
		MPI_Send((&board[my_rank*i_bar][0]), (size), MPI_CHAR, (my_rank-1), 0 ,MPI_COMM_WORLD);
		/* Envio da ultima linha para o rank de baixo */
		MPI_Send((&board[(my_rank+1)*i_bar-1][0]), (size), MPI_CHAR, (my_rank+1), 0 ,MPI_COMM_WORLD);
		/* Recebimento da borda superior */
		MPI_Recv((&board[(my_rank+1)*i_bar][0]), (size), MPI_CHAR, (my_rank+1), 0, MPI_COMM_WORLD, &status);
  }
  else if ((my_rank != 0) && (my_rank != (my_size-1) ) && (i_bar < i_bar_max) ){
		/* Recebimento da borda superior */
		MPI_Recv((&board[(i_bar_max*resto_i_bar-1) + ((my_rank-resto_i_bar)*i_bar)][0]), (size), MPI_CHAR, (my_rank-1), 0, MPI_COMM_WORLD, &status);
		/* Envio da primeira linha para o rank de cima */
		MPI_Send((&board[(i_bar_max*resto_i_bar) + ((my_rank-resto_i_bar)*i_bar)][0]), (size), MPI_CHAR, (my_rank-1), 0 ,MPI_COMM_WORLD);
		/* Envio da ultima linha para o rank de baixo */
		MPI_Send((&board[(i_bar_max*resto_i_bar+i_bar-1) + ((my_rank-resto_i_bar)*i_bar)][0]), (size), MPI_CHAR, (my_rank+1), 0 ,MPI_COMM_WORLD);
		/* Recebimento da borda inferior */
		MPI_Recv((&board[i_bar_max*resto_i_bar+i_bar][0]), (size), MPI_CHAR, (my_rank+1), 0, MPI_COMM_WORLD, &status);
  }
  else if (my_rank == (my_size-1) ){
		/* Recebimento da borda superior */
		MPI_Recv((&board[size-i_bar-1][0]), (size), MPI_CHAR, (my_rank-1), 0, MPI_COMM_WORLD, &status);
		/* Envio da primeira linha para o rank de cima */
    MPI_Send((&board[size-i_bar][0]), (size), MPI_CHAR, (my_rank-1), 0 ,MPI_COMM_WORLD);
  }

	/* for each cell, apply the rules of Life */
  if (i_bar == i_bar_max){
    for (int i = (my_rank * i_bar_max); ( i < ((my_rank + 1) * i_bar_max) ); i++ ){
			for (j=0; j<size; j++) {
  			a = adjacent_to (board, size, i, j);
  			if (a == 2) newboard[i][j] = board[i][j];
  			if (a == 3) newboard[i][j] = 1;
  			if (a < 2) newboard[i][j] = 0;
  			if (a > 3) newboard[i][j] = 0;
  		}
     }
  }
  else if ( (i_bar < i_bar_max) && (my_rank != (my_size-1)) ){
    for (int i = ((my_rank * i_bar)+1); ( i < ((my_rank * i_bar) + i_bar) ); i++ ){
      for (j=0; j<size; j++) {
        a = adjacent_to (board, size, i, j);
        if (a == 2) newboard[i][j] = board[i][j];
        if (a == 3) newboard[i][j] = 1;
        if (a < 2) newboard[i][j] = 0;
        if (a > 3) newboard[i][j] = 0;
      }
    }
  }
	else if (my_rank == (my_size-1) ){
    for (int i = (size-i_bar); ( i < (size) ); i++ ){
      for (j=0; j<size; j++) {
        a = adjacent_to (board, size, i, j);
        if (a == 2) newboard[i][j] = board[i][j];
        if (a == 3) newboard[i][j] = 1;
        if (a < 2) newboard[i][j] = 0;
        if (a > 3) newboard[i][j] = 0;
      }
    }
  }
}

/* print the life board */
void print (cell_t ** board, int size) {
	int	i, j;
	/* for each row */
	for (j=0; j<size; j++) {
		/* print each column position... */
		for (i=0; i<size; i++)
			printf ("%c", board[i][j] ? 'x' : ' ');
		/* followed by a carriage return */
		printf ("\n");
	}
}

/* read a file into the life board */
void read_file (FILE * f, cell_t ** board, int size) {
	int	i, j;
	char	*s = (char *) malloc(size+10);
	char c;
	for (j=0; j<size; j++) {
		/* get a string */
		fgets (s, size+10,f);
		/* copy the string to the life board */
		for (i=0; i<size; i++) {
		 	//c=fgetc(f);
			//putchar(c);
			board[i][j] = s[i] == 'x';
		}
		//fscanf(f,"\n");
	}
}

int main (int argc, char** argv) {
/* Inicio do MPI */
	MPI_Init(&argc, &argv);
	MPI_Comm_size (MPI_COMM_WORLD, &my_size);
	MPI_Comm_rank (MPI_COMM_WORLD, &my_rank);
	MPI_Status status;
	double localbegin, localend, localspent, spent;
	int size, steps;
	FILE    *f;
	if (my_rank == 0) {
    f = fopen("judge.in", "r");
//  f = stdin;
		fscanf(f,"%d %d", &size, &steps);
	}
	/* Envio do valor de size e steps da entrada para todos os processadores */
	MPI_Bcast(&size, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&steps, 1, MPI_INT, 0, MPI_COMM_WORLD);
	cell_t ** prev = allocate_board (size);
	if (my_rank == 0) {
		read_file (f, prev,size);
		fclose(f);
	}

  cell_t ** next = allocate_board (size);
	cell_t ** tmp;
  cell_t ** local_matrix = allocate_board (size);	// criado por mim para o rcv
	int i,j;
	/* i_bar é a divisão do size da matriz pelo size de processadores */
	i_bar = size/my_size;
	/* resto_i_bar é o resto da divisão entre siza da matriz pelo size de processadores */
	int resto_i_bar = size%my_size;
	if (my_rank < resto_i_bar) {
		/* i_bar_max é o máximo de linhas por rank e i_bar é o número de linhas local do rank */
		i_bar_max = i_bar + 1;
    i_bar = i_bar_max;
	}
  else if (resto_i_bar != 0) {
    i_bar_max = i_bar + 1;
  }
	else if (resto_i_bar == 0) {
		i_bar_max = i_bar;
	}

	#ifdef DEBUG
// apenas o my_rank 0 imprime o initial
//   if (my_rank == 0) {
//		printf("Initial \n");
//		print(prev,size);
//		printf("----------\n");
//	}
	#endif

/* A matriz está na variável prev! */

/* ---------- Inicializa a local_matrix do rank 0 ---------- */
  if ( my_rank == 0 ) {
	  for (int i = 0; i < size; i++){
	    for (int j = 0; j < size; j++){
	      local_matrix[i][j] = prev[i][j];
	    }
		}
  }
	free_board(prev,size);
	/*
	if ( my_rank != 0 ) {
    for (int i = 0; i < size; i++){
    	for (int j = 0; j < size; j++){
        local_matrix[i][j] = 0;
      }
		}
	}
	*/
/* ---------- Send do rank master para os outros ranks ---------- */
	if ( (my_rank == 0) && (resto_i_bar != 0) ) {
		/* Esse for funciona para mandar para os outros 'i' ranks */
  	for (int i = 1; i < (resto_i_bar); i++) {
			/* Esse for funciona para mandar para as 'j' linhas da matriz */
			for (int j=(i*i_bar_max); j < ( (i+1)*i_bar_max ); j++){
				MPI_Send( (&local_matrix[j][0]), (size), MPI_CHAR, i, 0 ,MPI_COMM_WORLD );
      }
    }
		/* Esse for funciona para mandar para os outros 'i' ranks */
		for ( int i = resto_i_bar; i < (my_size); i++) {
		/* Esse for funciona para mandar para as 'j' linhas da matriz */
			for (int j= ( (i_bar_max*resto_i_bar) + ((i-resto_i_bar)*(i_bar_max-1)) ) ;
			j < ( (i_bar_max*resto_i_bar) + ((i-resto_i_bar+1)*(i_bar_max-1)) ); j++) {
				MPI_Send( (&local_matrix[j][0]), (size), MPI_CHAR, i, 0 ,MPI_COMM_WORLD );
      }
    }
  }
	else if ( (my_rank == 0) && (resto_i_bar == 0) ) {
		/* Esse for funciona para mandar para os outros 'i' ranks */
  	for (int i = 1; i < (my_size); i++) {
			/* Esse for funciona para mandar para as 'j' linhas da matriz */
			for (int j=(i*i_bar_max); j < ( (i+1)*i_bar_max ); j++) {
				MPI_Send( (&local_matrix[j][0]), (size), MPI_CHAR, i, 0 ,MPI_COMM_WORLD );
      }
    }
  }

// ---------- Receive dos ranks slaves ----------
  else if ( (my_rank != 0) && (i_bar == i_bar_max) ) {
		/* Esse for funciona para receber as 'i' linhas da matriz */
    for (int i = (my_rank * i_bar_max);  i < ((my_rank + 1)*i_bar_max); i++ ) {
      MPI_Recv( (&local_matrix[i][0]), (size), MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status );
    }
  }
  else if ( (my_rank != 0) && (i_bar < i_bar_max) ) {
		/* Esse for funciona para receber as 'i' linhas da matriz */
		for (int i = ( (i_bar_max*resto_i_bar) + ((my_rank-resto_i_bar)*(i_bar_max-1)) );
		( i < ((i_bar_max*resto_i_bar) + ((my_rank-resto_i_bar+1)*(i_bar))) ); i++ ) {
      MPI_Recv( (&local_matrix[i][0]), (size), MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status );
    }
  }

/* ---------- Etapa em que ocorre a série de plays ---------- */
	MPI_Barrier(MPI_COMM_WORLD);
  	localbegin = MPI_Wtime();
	for ( i=0; i<steps; i++) {
		play (local_matrix,next,size);
    #ifdef DEBUG
//		printf("%d ---------- My_rank = %d\n", i, my_rank);
//		if (my_rank == 0 ) {
//			print (next,size);
//		}
		#endif
		tmp = next;
		next = local_matrix;
		local_matrix = tmp;
	}
	localend = MPI_Wtime();
	localspent = (localbegin - localend);
	MPI_Reduce(&localspent, &spent, 1, MPI_DOUBLE,MPI_MAX, 0, MPI_COMM_WORLD);
	if (my_rank==0){
		printf("Tempo de execução do laço for principal:%.3f\n", spent);
	}

/* ---------- Send dos slaves para o rank 0 para junção da matriz final ---------- */
  if ( (my_rank != 0) && (i_bar == i_bar_max) ) {
		/* Esse for funciona para mandar para as 'i' linhas da matriz */
    for (int i = (my_rank * i_bar_max); ( i < ((my_rank + 1)*i_bar_max) ); i++ ){
			MPI_Send( (&local_matrix[i][0]), (size), MPI_CHAR, 0, 0 ,MPI_COMM_WORLD );
    }
  }
  else if ( (my_rank != 0) && (i_bar < i_bar_max) ) {
		/* Esse for funciona para mandar para as 'i' linhas da matriz */
    for (int i = ((i_bar_max*resto_i_bar) + ((my_rank-resto_i_bar)*i_bar));
		( i < ((i_bar_max*resto_i_bar+i_bar) + ((my_rank-resto_i_bar)*(i_bar))) ); i++ ) {
			MPI_Send( (&local_matrix[i][0]), (size), MPI_CHAR, 0, 0 ,MPI_COMM_WORLD );
    }
  }
/* ---------- Receive do rank 0 para junção da matriz final ---------- */
  else if ((my_rank == 0) && (resto_i_bar != 0)) {
		/* Esse for funciona para receber dos outros 'i' ranks */
	  for (int i = 1; i < (resto_i_bar); i++) {
			/* Esse for funciona para receber as 'j' linhas da matriz */
			for (int j=(i*i_bar_max); j < ( (i+1)*i_bar_max ); j++) {
				MPI_Recv( (&local_matrix[j][0]), (size), MPI_CHAR, i, 0, MPI_COMM_WORLD, &status );
			}
		}
		/* Esse for funciona para receber dos outros 'i' ranks */
		for ( int i = resto_i_bar; i < (my_size); i++) {
			/* Esse for funciona para receber as 'j' linhas da matriz */
			for (int j= ( (i_bar_max*resto_i_bar) + ((i-resto_i_bar)*(i_bar_max-1)) );
			j < ( (i_bar_max*resto_i_bar) + ((i-resto_i_bar+1)*(i_bar_max-1)) ); j++){
				MPI_Recv( (&local_matrix[j][0]), (size), MPI_CHAR, i, 0, MPI_COMM_WORLD, &status );
			}
		}
  }
	else if ((my_rank == 0) && (resto_i_bar == 0)) {
		/* Esse for funciona para receber dos outros 'i' ranks */
		for (int i = 1; i < (my_size); i++) {
			/* Esse for funciona para receber as 'j' linhas da matriz */
			for (int j=(i*i_bar_max); j < ( (i+1)*i_bar_max ); j++) {
				MPI_Recv( (&local_matrix[j][0]), (size), MPI_CHAR, i, 0, MPI_COMM_WORLD, &status );
			}
		}
	}
/*
   // apenas o my_rank 0 imprime o final
   if (my_rank == 0){
	     print (local_matrix,size);
   }
*/

	free_board(next,size);
	free_board(local_matrix,size);
	MPI_Finalize();	// finalizando o MPI
}
