/*
 * =============================================================================
 * File        : read_csv.c
 * Author      : Leandro Martins dos Santos
 * Created on  : 2024-07-04
 * Last Update : 2025-11-04
 * Version     : 1.0
 * Description : CSV parsing and trajectory data preparation for Power PMAC.
 *               This module reads CSV files containing motion profiles and
 *               prepares them for execution via shared memory or buffer transfer.
 *
 * Dependencies: stdio.h, stdlib.h, string.h
 *               gplib.h (Power PMAC library interface)
 *               pp_proj.h (project-specific definitions and shared memory access)
 *
 * Compiler    : Power PMAC IDE (Visual Studio-based) or GCC (Linux)
 * Target      : Power PMAC CPU (Linux/Xenomai RT kernel)
 *
 * License     : Apache-2.0
 *
 * =============================================================================
 */

#include <gplib.h>
#include <stdlib.h>
#include <errno.h>

#define _PPScriptMode_		// for enum mode, replace this with #define _EnumMode_
#include "../../Include/pp_proj.h"
#include "read_csv.h"

int write_PVT(FILE *file, int bufferNum)
{
    // USHM PVT buffers
    int *pushm_time;
    int *pushm_user;
    double *pushm_positions[NUM_AXES];
    double *pushm_velocities[NUM_AXES];
    int line_count = 0;
    // Initialize buffers addresses
    pushm_time = (int *) pushm + USHM_INT_BASE_IDX + (bufferNum * USHM_BUFF_OFFSET_INT_IDX);
    pushm_user = (int *) pushm + USHM_INT_BASE_IDX + (bufferNum * USHM_BUFF_OFFSET_INT_IDX)+1;

    int axis;
    for (axis = 0; axis < NUM_AXES; axis++) {
        pushm_positions[axis] = (double *) pushm + (USHM_DOUBLE_BASE_IDX + axis) + (bufferNum * USHM_BUFF_OFFSET_DOUBLE_IDX);
    }
    for (axis = 0; axis < NUM_AXES; axis++) {
        pushm_velocities[axis] = (double *) pushm + (USHM_DOUBLE_BASE_IDX+NUM_AXES + axis) + (bufferNum * USHM_BUFF_OFFSET_DOUBLE_IDX);
    }

    char line[MAX_LINE_SIZE];
    // Read the first line
    if (fgets(line, sizeof(line), file) != NULL) {
        char *token = strtok(line, ",");
        while (token != NULL) {
			// Store header Data
			// ...
	        token = strtok(NULL, ",");
        }
    }

    while (fgets(line, sizeof(line), file) && line_count < USHM_BUFF_SIZE) {
        char *field = strtok(line, ",");

        *pushm_time = atoi(field);
        field = strtok(NULL, ",");
        pushm_time += USHM_LINE_OFFSET_INT_IDX;
        *pushm_user = atoi(field);
        field = strtok(NULL, ",");
        pushm_user += USHM_LINE_OFFSET_INT_IDX; // motot/axis wise
//        pushm_user += (NUM_AXES+1)*2; //point-wise

        for (axis = 0; axis < NUM_AXES; axis++) {
            *pushm_positions[axis] = atof(field);
            field = strtok(NULL, ",");
            pushm_positions[axis] += USHM_LINE_OFFSET_DOUBLE_IDX; // motor/axis wise
//            pushm_positions[axis] += (NUM_AXES+1); //point-wise

        }
        for (axis = 0; axis < NUM_AXES; axis++) {
            *pushm_velocities[axis] = atof(field);
            field = strtok(NULL, ",");
            pushm_velocities[axis] += USHM_LINE_OFFSET_DOUBLE_IDX;
        }

        line_count++;
    }
    return 0;
}

int write_positions(FILE *file, int bufferNum, int *line_count)
{
    // USHM Motor positions buffers
    int *pushm_user;
    double *pushm_positions[NUM_AXES];

    // Initialize buffers addresses
    pushm_user = (int *) pushm + USHM_INT_BASE_IDX + (bufferNum * USHM_BUFF_OFFSET_INT_IDX)+1;

    int axis;
    for (axis = 0; axis < NUM_AXES; axis++) {
        pushm_positions[axis] = (double *) pushm + (USHM_DOUBLE_BASE_IDX + axis) + (bufferNum * USHM_BUFF_OFFSET_DOUBLE_IDX);
    }

    char line[MAX_LINE_SIZE];
    // Read the first line
    if (fgets(line, sizeof(line), file) != NULL) {
        char *token = strtok(line, ",");
        while (token != NULL) {
			// Store header Data
			// ...
	        token = strtok(NULL, ",");
        }
    }

    while (fgets(line, sizeof(line), file) && (*line_count) < USHM_BUFF_SIZE) {
        char *field = strtok(line, ",");

        *pushm_user = atoi(field);
        field = strtok(NULL, ",");
//        pushm_user += USHM_LINE_OFFSET_INT_IDX; // motot/axis wise
        pushm_user += (NUM_AXES+1)*2; //point-wise

        for (axis = 0; axis < NUM_AXES; axis++) {
            *pushm_positions[axis] = atof(field);
            field = strtok(NULL, ",");
//            pushm_positions[axis] += USHM_LINE_OFFSET_DOUBLE_IDX; // motor/axis wise
            pushm_positions[axis] += (NUM_AXES+1); //point-wise
        }
        (*line_count)++;
    }
    return 0;
}

int read_csv(char *filename, int profileType, int bufferNum)
{
    FILE *file = fopen(filename, "r");
    int line_count = 0;

    if (file == NULL) {
        printf("Could not open file %s", filename);
        return 1;
    }
    if (profileType == 0) {
        write_PVT(file, bufferNum);
    }
    else if (profileType == 1) {
        write_positions(file, bufferNum,&line_count);
    }

    SetPtrVar(BufferFill_A+bufferNum, line_count);
//    printf("lines: %d\n",line_count);

    fclose(file);
    return 0;
}

int main(int argc, char *argv[])
{
	InitLibrary();  // Required for accessing Power PMAC library
//    double exec_time = GetCPUClock(); // Evaluation purposes only

    // TODO: Add argc checks, and set defaults values for profileType and bufferNum
    char *filename = argv[1];
    uint8_t profileType = atoi(argv[2]);    // 0: PVT, 1:Motors positions + User Commands
    uint8_t bufferNum = atoi(argv[3]);      // 0:BufferA, 1:BufferB, [2:BufferC]

//    int line_count;
    read_csv(filename, profileType, bufferNum);

//	exec_time = GetCPUClock()-exec_time;
//	printf("Lines number: %d\n", line_count);
//	printf("Execution time: %f us\n",exec_time);
	CloseLibrary();

	return 0;
}