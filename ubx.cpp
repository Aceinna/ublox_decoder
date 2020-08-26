// ubx.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "rtklib.h"

#include <stdio.h>
#include <stdlib.h>

#define HEADKML1 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
#define HEADKML2 "<kml xmlns=\"http://www.opengis.net/kml/2.2\">"
#define MARKICON "http://maps.google.com/mapfiles/kml/shapes/track.png"
#define SIZP     0.4            /* mark size of rover positions */
#define SIZR     0.8            /* mark size of reference position */

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
	char* p = (char*)buff, *q, sum;

	if (type == 0) {
		/* if type==0, output none */
		return 0;
		/*p += sprintf(p, "$GPGGA,,,,,,,,,,,,,,");
		for (q = (char*)buff + 1, sum = 0; *q; q++) sum ^= *q;
		p += sprintf(p, "*%02X%c%c", sum, 0x0D, 0x0A);
		return p - (char*)buff;*/
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
	FILE* fpvt = NULL; // u-blox own solution
	FILE* fscv = NULL; // u-blox csv solution
	FILE* fkml = NULL;

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

	memset(outfilename, 0, 255 * sizeof(char));
	sprintf(outfilename, "%s_pvt.nmea", fileName);
	fpvt = fopen(outfilename, "w"); if (fpvt == NULL) return;

	memset(outfilename, 0, 255 * sizeof(char));
	sprintf(outfilename, "%s_ins.kml", fileName);
	fkml = fopen(outfilename, "w"); if (fkml == NULL) return;

	//B-G-R white green light-yellow  red yellow cyan
	const char *color[] = {"ffffffff","ff008800","ff00aaff","ff0000ff","ff00ffff","ffff00ff"};  
	fprintf(fkml, "%s\n%s\n", HEADKML1, HEADKML2);
	fprintf(fkml, "<Document>\n");
	for (int i = 0; i < 6; i++) {
		fprintf(fkml, "<Style id=\"P%d\">\n", i);
		fprintf(fkml, "  <IconStyle>\n");
		fprintf(fkml, "    <color>%s</color>\n", color[i]);
		fprintf(fkml, "    <scale>%.1f</scale>\n", i == 0 ? SIZR : SIZP);
		fprintf(fkml, "    <Icon><href>%s</href></Icon>\n", MARKICON);
		fprintf(fkml, "  </IconStyle>\n");
		fprintf(fkml, "</Style>\n");
	}

	memset(outfilename, 0, 255 * sizeof(char));
	sprintf(outfilename, "%s_pvt.csv", fileName);
	fscv = fopen(outfilename, "w"); if (fscv == NULL) return;

	fprintf(fscv, "GPS_week,week second in ms,Error_N,Error_E,Error_U,Error_2D,Error_3D,Corrections Valid,\
Latitude,Longitude,Height_ell_m,HDOP,PDOP,Num_Sattelites,\
SDEast_m,SDNorth_m,SDHeight_m,\
VelEast,VelNorth,VelHeight,SDVelEast_m,SDVelNorth_m,SDVelHeight_m,\
Heading_deg,Heading_Acc_deg,Accuracy\n");

	int type = 0;
	int wn = 0;

	while ((type = input_rawf(&raw, STRFMT_UBX, fdat)) >= -1)
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
			// outnmea_gga1(buffer, raw.data[0], raw.data[11], blh, 10, 1.0, 1.0);
			// if (fimu != NULL) fprintf(fimu, "%s", buffer);
#if 1
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
		else if (type == 17)
		{
#if 1
			//raw.esfmeas_speed.timeTag
			//raw.esfmeas_speed.direction
			//raw.esfmeas_speed.single_tick
			if (fimu != NULL) fprintf(fimu, "6,%4i,%10i,%4i,%14d\n"
				, wn
				, raw.esfmeas_speed.timeTag, raw.esfmeas_speed.direction, raw.esfmeas_speed.single_tick);
#endif
		}
		else if (type == 15) // navPvt
		{
			double blh[3] = { raw.f9k_data[1] * PI / 180.0, raw.f9k_data[2] * PI / 180.0, raw.f9k_data[3] };
			unsigned char buffer[255] = { 0 };
			int retgga = 0;

			if (fabs(blh[0] * blh[1]) < 1e-7) continue;
			time2gpst(raw.time_pvt, &wn);
			retgga = outnmea_gga1(buffer, raw.f9k_data[0], (int)raw.f9k_data[14], blh, (int)raw.f9k_data[15], 1.0, 1.0);
			if (!retgga)
			{
				continue;
			}
			if (fpvt != NULL) fprintf(fpvt, "%s", buffer);

			double ep[6] = { 0.0 };
			char str[256] = "";
			time2epoch(raw.time_pvt, ep);
			sprintf_s(str, 255, "%04.0f-%02.0f-%02.0fT%02.0f:%02.0f:%06.3fZ",
				ep[0], ep[1], ep[2], ep[3], ep[4], ep[5]);

			if (fkml != NULL) {
				fprintf(fkml, "<Placemark>\n");
				fprintf(fkml, "<TimeStamp><when>%s</when></TimeStamp>\n", str);
				fprintf(fkml, "<description><![CDATA[\n");
				fprintf(fkml, "<TABLE border=\"1\" width=\"100%\" Align=\"center\">\n");
				fprintf(fkml, "<TR ALIGN=RIGHT>\n");
				fprintf(fkml, "<TR ALIGN = RIGHT><TD ALIGN = LEFT>Time:</TD><TD>");
				fprintf(fkml, "%4d</TD><TD>", wn);
				fprintf(fkml, "%11.4f</TD><TD>", raw.f9k_data[0]);
				fprintf(fkml, "%02.0f:%02.0f:%06.3f</TD><TD>", ep[3], ep[4], ep[5]);
				fprintf(fkml, "%2d/%2d/%3d</TD></TR>\n", (int)ep[0], (int)ep[1], (int)ep[2]);
				fprintf(fkml, "<TR ALIGN=RIGHT><TD ALIGN=LEFT>Position:</TD><TD>");
				fprintf(fkml, "%11.7f</TD><TD>", blh[0] * 180 / PI);
				fprintf(fkml, "%11.7f</TD><TD>", blh[1] * 180 / PI);
				fprintf(fkml, "%8.4f</TD>", blh[2]);
				fprintf(fkml, "<TD>(DMS,m)</TD></TR>\n");
				fprintf(fkml, "<TR ALIGN=RIGHT><TD ALIGN=LEFT>Vel(N,E,D):</TD><TD>");
				fprintf(fkml, "%8.4f</TD><TD>", raw.f9k_data[9]);
				fprintf(fkml, "%8.4f</TD><TD>", raw.f9k_data[10]);
				fprintf(fkml, "%8.4f</TD>", raw.f9k_data[11]);
				fprintf(fkml, "<TD>(m/s)</TD></TR>\n");
				fprintf(fkml, "<TR ALIGN=RIGHT><TD ALIGN=LEFT>Att(r,p,y):</TD><TD>");
				fprintf(fkml, "%8.4f</TD><TD>", 0.0);
				fprintf(fkml, "%8.4f</TD><TD>", 0.0);
				fprintf(fkml, "%8.4f</TD>", raw.f9k_data[6]);
				fprintf(fkml, "<TD>(deg)</TD></TR>\n");
				fprintf(fkml, "<TR ALIGN=RIGHT><TD ALIGN=LEFT>Misc Info:</TD><TD>");
				fprintf(fkml, "%d</TD><TD>", raw.f9k_data[14]);
				fprintf(fkml, "%d</TD><TD>", 0);
				fprintf(fkml, "%d</TD>", 0);
				fprintf(fkml, "<TD></TD></TR>\n");
				fprintf(fkml, "</TABLE>\n");
				fprintf(fkml, "]]></description>\n");

				fprintf(fkml, "<styleUrl>#P%d</styleUrl>\n", 2);
				fprintf(fkml, "<Style>\n");
				fprintf(fkml, "<IconStyle>\n");
				fprintf(fkml, "<heading>%f</heading>\n", raw.f9k_data[6]);
				fprintf(fkml, "</IconStyle>\n");
				fprintf(fkml, "</Style>\n");


				fprintf(fkml, "<Point>\n");
				fprintf(fkml, "<coordinates>%13.9f,%12.9f,%5.3f</coordinates>\n", blh[1] * R2D,
					blh[0] * R2D, blh[2]);
				fprintf(fkml, "</Point>\n");
				fprintf(fkml, "</Placemark>\n");
			}
#if 1
			if (fscv != NULL) fprintf(fscv, "%4d,%12.0f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f,%d,\
%12.7f,%12.7f,%10.4f,%6.2f,%6.2f,\
%2d,\
%10.4f,%10.4f,%10.4f,\
%10.4f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f,\
%12.7f,%12.7f,%2d\n"
				, wn, raw.f9k_data[0]*1000, 0.0, 0.0, 0.0, 0.0, 0.0, 0
				, raw.f9k_data[1], raw.f9k_data[2], raw.f9k_data[3], 0.0, raw.f9k_data[16]
				, (int)raw.f9k_data[15]
				, 0.0, raw.f9k_data[7], raw.f9k_data[8]
				, raw.f9k_data[10], raw.f9k_data[9], -raw.f9k_data[11], 0.0, 0.0, raw.f9k_data[12]
			    , raw.f9k_data[5], raw.f9k_data[13], (int)raw.f9k_data[14]);
#endif
		}
		else if (type == 16) //esfRaw
		{
#if 1
			if (fimu != NULL)
			{
				for (int i = 0; i < 10; i++)
				{
					fprintf(fimu, "7,%4i,%10i,%14.10f,%14.10f,%10.4f,%10.4f,%14.10f,%14.10f,%10.4f\n"
						, wn
						, (int)raw.m8l_esfRaw[i * 8], raw.m8l_esfRaw[i * 8 + 1], raw.m8l_esfRaw[i * 8 + 2], raw.m8l_esfRaw[i * 8 + 3]
						, raw.m8l_esfRaw[i * 8 + 4], raw.m8l_esfRaw[i * 8 + 5], raw.m8l_esfRaw[i * 8 + 6], raw.m8l_esfRaw[i * 8 + 7]);
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
			else if (sys == SYS_GPS || sys == SYS_GAL || sys == SYS_CMP)
			{
				eph_t* eph = raw.nav.eph + sat - 1;
				if (fimu != NULL) fprintf(fimu, "4,%4i\n"
					, eph->sat);
			}
#endif
		}
		else if (type == 3)
		{
			//printf("%i\n", type);
		}
		else if (type == 9)
		{
			//printf("%i\n", type);
		}
		else if (type > 0)
		{
			//printf("%i\n", type);
		}
	}


	free_raw(&raw);

	if (fdat != NULL) fclose(fdat);
	if (fimu != NULL) fclose(fimu);
	if (fkml != NULL) {
		fprintf(fkml, "</Document>\n");
		fprintf(fkml, "</kml>\n");
		fclose(fkml);
	}

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
		//decode_ubx("E:\\test\\tesla\\12.04\\ubx_native\\ubx_raw_log_010381_2019-12-04T17-41-39.ubx");
		//decode_ubx("E:\\test\\tesla\\12.04\\ubx_native\\ubx_raw_log_010382_2019-12-04T18-44-04.ubx");
		//decode_ubx("E:\\test\\tesla\\12.04\\ubx_native\\ubx_raw_log_010383_2019-12-04T19-30-46.ubx");
		//decode_ubx("E:\\test\\tesla\\12.04\\ubx_native\\ubx_raw_log_010384_2019-12-04T20-12-38.ubx");
		//decode_ubx("E:\\test\\tesla\\12.04\\ubx_native\\ubx_raw_log_010385_2019-12-04T20-55-55.ubx");
		decode_ubx("E:\\data\\239\\COM124_200826_024331.ubx");
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
