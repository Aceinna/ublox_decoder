// ubx.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "rtklib.h"

#include <stdio.h>
#include <stdlib.h>

void decode_ubx(const char* fname)
{
	FILE* fdat = NULL;
	FILE* fimu = NULL;

	raw_t raw;
	if (!init_raw(&raw, STRFMT_UBX)) {
		return;
	}

	char fileName[255] = { 0 };
	char outfilename[255] = { 0 };

	fdat = fopen(fname, "rb"); if (fdat == NULL) return;

	const char* result = strrchr(fname, '\\');
	if (result != NULL)
		strncpy(fileName, result + 1, strlen(result));
	else
		strncpy(fileName, fname, strlen(fname));
	char* result1 = strrchr(fileName, '.');
	if (result1 != NULL) result1[0] = '\0';

	sprintf(outfilename, "%s_imu.txt", fileName); 


	fimu = fopen(outfilename, "w"); if (fimu == NULL) return;

	int type = 0;
	int wn = 0;

	while ((type=input_rawf(&raw, STRFMT_UBX, fdat))>=-1)
	{
		if (type == 1)
		{
			for (int i = 0; i < raw.obs.n; ++i)
			{
				obsd_t* pobs = raw.obs.data + i;
				wn = 0;
				double ws = time2gpst(pobs->time, &wn);
				int prn = 0;
				int sys = satsys(pobs->sat, &prn);
				if (fimu != NULL) fprintf(fimu, "1,%4i,%10.4f,%3i,%3i,%14.4f,%14.4f,%10.4f,%4i,%4i\n", wn, ws, sys, prn, pobs->P[0], pobs->L[0], pobs->D[0], pobs->SNR[0], pobs->LLI[0]);
				//printf("%4i,%10.4f,%3i,%3i,%14.4f,%14.4f,%10.4f\n", wn, ws, sys, prn, prn, pobs->P[0], pobs->L[0], pobs->D[0]);
			}
		}
		else if (type == 12)
		{
			if (fimu != NULL) fprintf(fimu, "2,%4i,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f\n"
				, wn
				, raw.data[17], raw.data[11]
				, raw.data[12], raw.data[13], raw.data[14], raw.data[15], raw.data[16]);
		}
		else if (type == 11)
		{
			if (fimu != NULL) fprintf(fimu, "3,%4i,%10.4f,%14.10f,%14.10f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f\n"
				, wn
				, raw.data[0], raw.data[1], raw.data[2], raw.data[3], raw.data[4], raw.data[5]
				, raw.data[6], raw.data[7], raw.data[8], raw.data[9], raw.data[10]);
		}
		else
		{
			//printf("%i\n", type);
		}
	}


	free_raw(&raw);

	if (fdat == NULL) fclose(fdat);
	if (fimu == NULL) fclose(fimu);

	return;
}

int main(int argc, char* argv[])
{
    //std::cout << "Hello World!\n";
	if (argc < 2)
	{
		//decode_ubx("C:\\aceinna\\tesla\\248\\m8\\ubx\\ubx_raw_log_008377_2019-09-05T15-27-42.ubx");
		//decode_ubx("C:\\aceinna\\tesla\\248\\m8\\ubx\\ubx_raw_log_008378_2019-09-05T16-32-41.ubx");
		//decode_ubx("C:\\aceinna\\tesla\\248\\m8\\ubx\\ubx_raw_log_008379_2019-09-05T17-41-08.ubx");
		//decode_ubx("C:\\aceinna\\tesla\\248\\m8\\ubx\\ubx_raw_log_008380_2019-09-05T18-28-32.ubx");
		decode_ubx("C:\\aceinna\\tesla\\248\\m8\\ubx\\ubx_raw_log_008381_2019-09-05T19-14-37.ubx");
		decode_ubx("C:\\aceinna\\tesla\\248\\m8\\ubx\\ubx_raw_log_008382_2019-09-05T20-00-31.ubx");
		decode_ubx("C:\\aceinna\\tesla\\248\\m8\\ubx\\ubx_raw_log_008383_2019-09-05T20-49-50.ubx");
		decode_ubx("C:\\aceinna\\tesla\\248\\m8\\ubx\\ubx_raw_log_008384_2019-09-05T21-35-09.ubx");
		decode_ubx("C:\\aceinna\\tesla\\248\\m8\\ubx\\ubx_raw_log_008385_2019-09-05T22-20-09.ubx");
	}
	else
	{
		decode_ubx(argv[1]);
	}
	return 1;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
