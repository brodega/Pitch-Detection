#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_complex.h>

#define REAL(z, i) ((z)[2 * (i)])
#define IMAG(z, i) ((z)[2 * (i) + 1])
#define SAMPLE_RATE 44100

typedef struct
{
	int value;
	int position;
} Node;

int get_array_size(FILE* file)
{
	char line[80];
	int num_lines = 0;

	while (strcmp(line, "%%\n") != 0) {
		fgets(line, 10, file);
	}

	while (fgets(line, 50, file) != NULL) {
		num_lines++;
	}
	//printf("num_lines =\t%d\n", num_lines);
	rewind(file);
	
	return num_lines;
}

void get_amplitudes(int argc, char *argv[], int *array, int num_lines) 
{
	FILE *file;
	file = fopen(argv[1], "r");

	char line[80];
	int number;
	
	/* Allocate some memory for our array */

	/* Skip over the header again */
	while (strcmp(line, "%%\n") != 0) {
		fgets(line, 10, file);
	}

	/* Make the array of samples */
	int i;
	for (i = 0; i < num_lines; i++) {
		fgets(line, 80, file);
		sscanf(line, "%d", &number);
		array[i] = number;
	}
	rewind(file);
}


long get_max(double *array, int array_size)
{
	int i, j, j_temp;
	long max;
	for (i = 0; i < array_size; i++) {
		if (max < array[i]) {
			max = array[i];
		}
	}

	return max;
}

int main(int argc, char* argv[])
{
	//make sure file is passed
	if (argc == 1) {
		exit(1);
	}

	//open file
	FILE *file;
	file = fopen(argv[1], "r");
	char line[80];
	fgets(line, 15, file);

	//validate RRA format
	if (strcmp(line, "RRAUDIO\n") != 0) {
		printf("Not a valid RRAUDIO file\n");
		exit(-1);
	}

	//create array for rra data
	int *array;
	//array_size = # of lines in file
	int array_size = get_array_size(file);
	//changing it to a power of 2

			//shift a window of size 1024

	double new_array_size;
	new_array_size = pow(2,floor(log2((double)array_size)));

	array = (int *) malloc(array_size * sizeof(int));

	//fill array with amplitude data from rra file
	get_amplitudes(argc, argv, array, array_size);
	fclose(file);


	//array for fft 
	double *data;
	data = (double *) malloc((array_size * 2) * sizeof(double));
	long i;
	//initialize REAL and IMAG arrays to 0
	for (i = 0; i < array_size; i++) {
		REAL(data, i) = 0.0;
		IMAG(data, i) = 0.0;
	}

	for (i = 0; i < new_array_size; i++) {
		REAL(data, i) = array[i];
	}

	//pump data through fft
	gsl_fft_complex_radix2_forward(data, 1, new_array_size);

	//array of bin frequencies to correspond with output of fft
	double *bin_array;
	bin_array = (double *) malloc(sizeof(double) * new_array_size);

	//fill bin array
	for (i = 0; i < new_array_size; i++) {
		bin_array[i] = (i * SAMPLE_RATE) / new_array_size;
	}

	//pipe to rra file
	printf("RRAUDIO\n");
	printf("samples: %d\n", (int)new_array_size);
	puts("%%");
	/*			what is this
	for (i = 0; i < new_array_size; i++) {
		printf("%f\n", REAL(data, i) / sqrt(array_size));
	}
	*/

//	get_max doesn't do anything useful
//	long max = get_max(bin_array, new_array_size);
//	printf("Max: %ld\n", max);


	//array for output
	double *out_array;
	out_array = (double *) malloc(sizeof(double) * new_array_size);

	//fill out_array with magnitudes
	for (i = 0; i < new_array_size; i++) {
		out_array[i] = sqrt(REAL(data, i) * REAL(data, i) + IMAG(data, i) * IMAG(data, i));
//		printf("%lf\n", sqrt(REAL(data, i) * REAL(data, i) + IMAG(data, i) * IMAG(data, i)));
	}
	
	//create, fill peak array with indices of peaks
	//indices correspond to which indices in out_array are peaks, and what frequencies they are in bin_array
	//need to find max peak for correct frequency hopefully
	double *peak_array;
	peak_array = (double *) malloc(sizeof(double) * new_array_size);
	int count = 0;
	for(i = 0; i < new_array_size; i++){
		if( i > 2 && i < new_array_size - 2){
			if(out_array[i] > out_array[i-1] && out_array[i] > out_array[i-2]){
				if(out_array[i] > out_array[i+1] && out_array[i] > out_array[i+2]){
					peak_array[count] = i;
					count++;
				}
			}		

		}
	}

	for(i =0; i < count; i++){
		//printf("%lf\n",out_array[ (int)peak_array[i] ] );
		printf("%lf:%lf\n", peak_array[i], bin_array[(int)peak_array[i]]);
	}

		

	

	/*			what is this
	double data[2 * 256];
	int i;
	for (i = 0; i < 256; i++) {
		REAL(data, i) = 0.0;
		IMAG(data, i) = 0.0;
	}

	for (i = 0; i < 256; i++) {
		REAL(data, i) = array[i];
	}
	gsl_fft_complex_radix2_forward(data, 1, 256);

	for (i = 0; i < (sizeof(data) / sizeof(double)); i++) {
		printf("%f\t%f\n", 
				REAL(data, i) / sqrt(256),
				IMAG(data, i) / sqrt(128));
	}
	*/

	return 0;
}
