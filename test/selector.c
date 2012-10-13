/*
 * selector.c
 *
 *  Created on: Oct 13, 2012
 *      Author: w4118
 */

#include <stdio.h>
#include <ctype.h> /* for is_digit method */
#include <stdlib.h>
#include "orient_lock.h"


#define FILENAME "integer"

/*
 * Accepts a starting integer as the only argument. When run, your program must
 * take the write lock for when the device is lying face down and write the
 *  integer from the argument to a file called integer in the current working
 *   directory. When the integer has been written, the write lock must be
 *   released and and re-acquired before writing the last integer + 1 to the
same file (overwriting the content of the file). Your program should run until t
erminated using ctrl+c by the user. Before releasing the write lock, you should
 output the integer to standard out.
 *
 */


static void usage() {
	printf("Invalid arguments.\n");
	printf("Usage: selector [integer]\n");
}

static int is_number(char *string)
{
	int i = 0;

	if (string == NULL)
		return 0;

	for (; string[i] != '\0'; ++i) {
		if (!isdigit(string[i]))
			return 0;
	}
	return 1;
}


int main(int argc, char **argv) {
	int counter;
	if(argc != 2 || !is_number(argv[1])) {
		usage();
		exit(-1);
	}
	counter = atoi(argv[1]);

	struct orientation_range write_lock;
	/* Only want this to work when device is lying facedown */
	struct dev_orientation write_lock_orient;
	write_lock_orient.azimuth = 0;
	write_lock_orient.pitch = 180;
	write_lock_orient.roll = 0;

	write_lock.orient = write_lock_orient;
	write_lock.azimuth_range =  180;
	write_lock.roll_range = 10;
	write_lock.pitch_range = 10;

	const char *filename = FILENAME;

	while (1) {
		/* Take the write lock */
		orientlock_write(&write_lock);

		FILE *integer_file = fopen(filename, "w");
		fprintf(integer_file, "%d", counter);

		/* Release write lock */
		orientunlock_write(&write_lock);
		++counter;
		fclose(integer_file);
	}
}