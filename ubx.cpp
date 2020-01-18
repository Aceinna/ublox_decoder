// ubx.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "rtklib.h"

#include <stdio.h>
#include <stdlib.h>

static void deg2dms(double deg, double* dms)
{
	double sign = deg < 0.0 ? -1.0 : 1.0, a = fabs(deg);
	dms[0] = floor(a); a = (a - dms[0]) * 60.0;
	dms[1] = floor(a); a = (a - dms[1]) * 60.0;
	dms[2] = a; dms[0] *= sign;
}

/* output solution in the form of nmea GGA sentence --------------------------*/
static int outnmea_gga1(unsigned char* buff, double time, int type, double* blh, int ns, double dop, double age)
{
	double h, ep[6], dms1[3], dms2[3];
	char* p = (char*)buff, * q, sum;

	if (type != 1 && type != 4 && type != 5) {
		p += sprintf(p, "$GPGGA,,,,,,,,,,,,,,");
		for (q = (char*)buff + 1, sum = 0; *q; q++) sum ^= *q;
		p += sprintf(p, "*%02X%c%c", sum, 0x0D, 0x0A);
		return p - (char*)buff;
	}
	time -= 18.0;
	ep[2] = floor(time / (24 * 3600));
	time -= ep[2] * 24 * 3600.0;
	ep[3] = floor(time / 3600);
	time -= ep[3] * 3600;
	ep[4] = floor(time / 60);
	time -= ep[4] * 60;
	ep[5] = time;
	h = 0.0;
	deg2dms(fabs(blh[0]) * 180.0 / PI, dms1);
	deg2dms(fabs(blh[1]) * 180.0 / PI, dms2);
	p += sprintf(p, "$GPGGA,%02.0f%02.0f%05.2f,%02.0f%010.7f,%s,%03.0f%010.7f,%s,%d,%02d,%.1f,%.3f,M,%.3f,M,%.1f,",
		ep[3], ep[4], ep[5], dms1[0], dms1[1] + dms1[2] / 60.0, blh[0] >= 0 ? "N" : "S",
		dms2[0], dms2[1] + dms2[2] / 60.0, blh[1] >= 0 ? "E" : "W", type,
		ns, dop, blh[2] - h, h, age);
	for (q = (char*)buff + 1, sum = 0; *q; q++) sum ^= *q; /* check-sum */
	p += sprintf(p, "*%02X%c%c", sum, 0x0D, 0x0A);
	return p - (char*)buff;
}


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

	strcpy(fileName, fname);
	char* result1 = strrchr(fileName, '.');
	if (result1 != NULL) result1[0] = '\0';

	sprintf(outfilename, "%s_raw", fileName); 


	fimu = fopen(outfilename, "w"); if (fimu == NULL) return;

	int type = 0;
	int wn = 0;

	while ((type=input_rawf(&raw, STRFMT_UBX, fdat))>=-1)
	{
		if (type == 1)
		{
#if 0
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
#endif
		}
		else if (type == 12)
		{
#if 0
			if (fimu != NULL) fprintf(fimu, "2,%4i,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f\n"
				, wn
				, raw.data[17], raw.data[11]
				, raw.data[12], raw.data[13], raw.data[14], raw.data[15], raw.data[16]);
#endif
		}
		else if (type == 11)
		{
			double blh[3] = { raw.data[1] * PI / 180.0, raw.data[2] * PI / 180.0, raw.data[3] };
			unsigned char buffer[255] = { 0 };
			outnmea_gga1(buffer, raw.data[0], 1, blh, 10, 1.0, 1.0);
			if (fimu != NULL) fprintf(fimu, "%s", buffer);
#if 0
			if (fimu != NULL) fprintf(fimu, "3,%4i,%10.4f,%14.10f,%14.10f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f\n"
				, wn
				, raw.data[0], raw.data[1], raw.data[2], raw.data[3], raw.data[4], raw.data[5]
				, raw.data[6], raw.data[7], raw.data[8], raw.data[9], raw.data[10]);
#endif
		}
		else if (type == 13)
		{
#if 0
			if (fimu != NULL) fprintf(fimu, "4,%4i,%10i,%14.10f,%14.10f,%10.4f\n"
				, wn
				, (int)raw.f9k_data[0], raw.f9k_data[1], raw.f9k_data[2], raw.f9k_data[3]);
#endif
		}
		else if (type == 14)
		{
#if 0
			if (fimu != NULL) fprintf(fimu, "5,%4i,%10i,%14.10f,%14.10f,%10.4f,%10.4f\n"
				, wn
				, (int)raw.f9k_data[4], raw.f9k_data[5], raw.f9k_data[6], raw.f9k_data[7], raw.f9k_data[8]);
#endif
		}
		else if (type == 15) // navPvt
		{
			
		}
		else if (type == 16) //esfRaw
		{
#if 1
			if (fimu != NULL)
			{
				for (int i = 0; i < 10; i++)
				{
					fprintf(fimu, "6,%4i,%10i,%14.10f,%14.10f,%10.4f,%10.4f,%14.10f,%14.10f,%10.4f\n"
						, wn
						, (int)raw.m8l_esfRaw[i*8], raw.m8l_esfRaw[i*8+1], raw.m8l_esfRaw[i*8+2], raw.m8l_esfRaw[i*8+3]
						, raw.m8l_esfRaw[i*8+4], raw.m8l_esfRaw[i*8+5], raw.m8l_esfRaw[i*8+6], raw.m8l_esfRaw[i*8+7]);
				}
			}
#endif
		}
		else if (type == 2)
		{
#if 0
			//printf("%i\n", type);
			int prn = 0;
			int sat = raw.ephsat;
			int sys = satsys(sat, &prn);
			if (sys == SYS_GLO)
			{
				geph_t* eph = raw.nav.geph + sat - 1;
				if (fimu != NULL) fprintf(fimu, "4,%4i\n"
					, eph->sat);
			}
			else if (sys==SYS_GPS||sys==SYS_GAL||sys==SYS_CMP)
			{
				eph_t* eph = raw.nav.eph + sat - 1;
				if (fimu != NULL) fprintf(fimu, "4,%4i\n"
					, eph->sat);
			}
#endif
		}
		else if (type==3)
		{
			//printf("%i\n", type);
		}
		else if (type == 9)
		{
			//printf("%i\n", type);
		}
		else if (type>0)
		{
			//printf("%i\n", type);
		}
	}


	free_raw(&raw);

	if (fdat != NULL) fclose(fdat);
	if (fimu != NULL) fclose(fimu);

	return;
}


void decode_rtcm3(const char* fname, int year, int mon, int day)
{
	FILE* fdat = NULL;
	FILE* fgps = NULL;

	rtcm_t rtcm;
	if (!init_rtcm(&rtcm)) {
		return;
	}

	char fileName[255] = { 0 };
	char outfilename[255] = { 0 };

	fdat = fopen(fname, "rb"); if (fdat == NULL) return;

	strncpy(fileName, fname, strlen(fname));
	char* result = strrchr(fileName, '.');
	if (result != NULL) result[0] = '\0';

	sprintf(outfilename, "%s_rtcm.txt", fileName);

	fgps = fopen(outfilename, "w"); if (fgps == NULL) return;

	double ep[] = { (double)year, (double)mon, (double)day, 0.0, 0.0, 0.0 };

	rtcm.time = epoch2time(ep);

	int type = 0;
	int wn = 0;
	double ws = 0.0;

	while ((type = input_rtcm3f(&rtcm, fdat)) >= -1)
	{
		if (type == 1)
		{
			for (int i = 0; i < rtcm.obs.n; ++i)
			{
				obsd_t* pobs = rtcm.obs.data + i;
				ws = time2gpst(pobs->time, &wn);
				int prn = 0;
				int sys = satsys(pobs->sat, &prn);
				if (fgps != NULL) fprintf(fgps, "1,%4i,%10.4f,%3i,%3i,%14.4f,%14.4f,%10.4f,%4i,%4i\n", wn, ws, sys, prn, pobs->P[0], pobs->L[0], pobs->D[0], pobs->SNR[0], pobs->LLI[0]);
				//printf("%4i,%10.4f,%3i,%3i,%14.4f,%14.4f,%10.4f\n", wn, ws, sys, prn, prn, pobs->P[0], pobs->L[0], pobs->D[0]);
			}
		}
		else if (type == 2)
		{
			//printf("%i\n", type);
			int prn = 0;
			int sat = rtcm.ephsat;
			int sys = satsys(sat, &prn);
			if (sys == SYS_GLO)
			{
				geph_t* eph = rtcm.nav.geph + sat - 1;
				if (fgps != NULL) fprintf(fgps, "4,%4i\n"
					, eph->sat);
			}
			else if (sys == SYS_GPS || sys == SYS_GAL || sys == SYS_CMP)
			{
				eph_t* eph = rtcm.nav.eph + sat - 1;
				if (fgps != NULL) fprintf(fgps, "4,%4i\n"
					, eph->sat);
			}
		}
		else if (type == 3)
		{
			printf("%i\n", type);
		}
		else if (type == 5)
		{
			ws = time2gpst(rtcm.time, &wn);
			if (fgps != NULL) fprintf(fgps, "5,%4i,%10.4f,%14.4f,%14.4f,%14.4f,%4i\n", wn, ws, rtcm.sta.pos[0], rtcm.sta.pos[1], rtcm.sta.pos[2], rtcm.staid);
			//printf("%4i,%10.4f,%3i,%3i,%14.4f,%14.4f,%10.4f\n", wn, ws, sys, prn, prn, pobs->P[0], pobs->L[0], pobs->D[0]);
		}
		else if (type > 0)
		{
			printf("%i\n", type);
		}
	}


	free_rtcm(&rtcm);

	if (fdat != NULL) fclose(fdat);
	if (fgps != NULL) fclose(fgps);

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
		//decode_ubx("C:\\aceinna\\tesla\\248\\m8\\ubx\\ubx_raw_log_008381_2019-09-05T19-14-37.ubx");
		//decode_ubx("C:\\aceinna\\tesla\\248\\m8\\ubx\\ubx_raw_log_008382_2019-09-05T20-00-31.ubx");
		//decode_ubx("C:\\aceinna\\tesla\\248\\m8\\ubx\\ubx_raw_log_008383_2019-09-05T20-49-50.ubx");
		//decode_ubx("C:\\aceinna\\tesla\\248\\m8\\ubx\\ubx_raw_log_008384_2019-09-05T21-35-09.ubx");
		//decode_ubx("C:\\aceinna\\tesla\\248\\m8\\ubx\\ubx_raw_log_008385_2019-09-05T22-20-09.ubx");

		//decode_rtcm3("C:\\aceinna\\tesla\\248\\sf01248d01.dat", 2019, 9, 5);
		decode_ubx("C:\\tesla\\1119\\m8l_psb_in_car_nov19\\ubx_raw_log_010207_2019-11-19T19-03-21.ubx");
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
