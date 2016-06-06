/*
 * Copyright (C) 2004
 * Jason Downer
 * jbd@cableone.net
 * http://jbd.zayda.net/enscribe/
 *
 * The fft routines at the end are borrowed / modified from the 
 * fxt library.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *                       

	------------    Changes    ------------

	2008-06-15  Still me

	- Added audio mask
	
	2004-08-13  Still me

	- changed oversampling to 4x
	
	2004-08-02  Still just me

	- fixed mono / color check
	
	2004-07-04  Your's Truly

	- added sndfile support
	- fixed PNG load
	- ate too much
	
	2004-07-01  Your's Truly

	- added -hiss
	- added experimental rgb color mode for use with baudline's -monet switch
	- added gamma for hissed or experimental rgb images
 
	2004-06-25  Jason Downer

	-	Pre-alpha version, semi-sucky
	- Bugfix: output frame size corrected

	2004-06-23  Jason Downer

	-	Pre-alpha version, extra-sucky

*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include "sndfile.h"

#include "gd.h"
#include "gdfontt.h"
#include "gdfonts.h"
#include "gdfontmb.h"
#include "gdfontl.h"
#include "gdfontg.h"

#define FRand() (random()/(float)RAND_MAX)
#define FRand2() (2.0*(random()/(float)RAND_MAX)-1.0)

#define HISSGAMMA (1.8)
#define EXPGAMMA  (2.4)

int versionmajor=0;
int versionminor=1;
int versionpatch=0;

int NUMOVER=4;

enum colormodes
	{
	GREYSCALE,
	MONOCHROME,
	REDCYAN,
	ORANGEAQUA,
	YELLOWBLUE,
	GREENPURPLE,
	WHITEBLACK,
	EXPERIMENTAL
	};
	
enum audioformats
	{
	RAWAUDIO,
	WAVAUDIO,
	AIFFAUDIO,
	AUAUDIO,
	STDOUTAUDIO
	};

enum ERRORS
	{
	NO_ERROR,
	NO_ARGS,
	BAD_ARGS,
	MEM_ERROR,
	FILE_ERROR,
	IMAGE_READ_ERROR,
	IMAGE_FORMAT_ERROR,
	FONT_ERROR,
	AUDIO_FILE_ERROR,
	AUDIO_FORMAT_ERROR,
	BAD_COLORMODE,
	AUDIO_WRITE_ERROR,
	INCOMPAT_MODE,
	maxERRORS
	};

int ldn=0;
int channels=2;
int transize=1;
int textmode=0;
int colormode=REDCYAN;
int audioformat=WAVAUDIO;
int endianformat=SF_ENDIAN_FILE;
int dataformat=SF_FORMAT_PCM_16;
int samplerate=44100;
int inverse=0;
int oversample=0;
int randphase=0;

int masking=0;

float lowfreq=0;
float highfreq=100;
float length=0;

float vgamma=1.0;

char *text=NULL;
char *inputfilename=NULL;
char *outputfilename=NULL;
char *maskfilename=NULL;
char *defaultoutputfilenamewav="output.wav";
char *defaultoutputfilenameraw="output.raw";
char *defaultoutputfilenameaiff="output.aiff";
char *defaultoutputfilenameau="output.au";

int fontsize=0;

void fft(float *fr, float *fi, int ldn, int is);

void printblurb()
	{
	printf("Enscribe %d.%d.%d      (c) 2004-2008 Jason Downer\n",versionmajor,versionminor,versionpatch);
	printf("Homepage: http://jbd.zayda.net/enscribe/\n\n");
	printf("This is ALPHA software.\n\n");
	printf("Enscribe is free software, distributed under the General Public License.\n\n");
	printf("There is NO warranty; not even for MERCHANTABILITY or FITNESS\n");
	printf("FOR A PARTICULAR PURPOSE.\n\n");
	}

void printoptions()
	{
//	printf("Usage:  enscribe [switches] [-t text] input_image output_audio\n");
	printf("Usage:  enscribe [switches] input_image output_audio\n");
	printf("Valid image formats: Jpeg, Png, WBMP, Xbm, GD, GD2\n\n");
	printf("Switches:\n");
	printf("  -inverse, -i           Inverse (negative) input image\n");
	printf("\n");
	printf("  -color=[mode]          Sets color conversion method for the input file:\n");
	printf("         g                 greyscale\n");
	printf("         m                 monochrome, ugly\n");
	printf("         rc                red and cyan (default)\n");
	printf("         oa                orange and aqua\n");
	printf("         yb                yellow and blue\n");
	printf("         gp                green and purple (for Baudline(TM) )\n");
	printf("         wb                reyscale on left, inverse on right. Weird.\n");
	printf("         monet             pseudo 3-color for \"baudline -monet\" display.\n");
	printf("\n");
	printf("  -transform-size=[size] iFFT blocksize:\n");
	printf("  -ts=[size]               0 = 512\n");
	printf("                           1 = 1024 (default)\n");
	printf("                           2 = 2048\n");
	printf("                           3 = 4096\n");
	printf("                           4 = 8192\n");
	printf("\n");
//	printf("  -t text                Print text instead of image\n");
//	printf("  -fs=[font size]        Sets text font size (0 to 4)\n");
//	printf("\n");
	printf("  -lf=[start]            Set low frequency start of image (default=0%%)\n");
	printf("  -hf=[stop]             Set high frequency stop of image (default=100%%)\n");
	printf("  -hiss, -h              Uses hissing rather than buzzing\n");
	printf("  -oversample, -o        4X oversample audio (adds hiss)\n");
	printf("\n");
	printf("  -mono, -m              Produce mono ouput audio file.\n");
	printf("  -stereo, -s            Produce stereo ouput audio file (default).\n");
	printf("  -rate=[rate]           Set audio sample rate (44100 default).\n");
	printf("\n");
	printf("  -wav                   Output wav audio (default)\n");
	printf("  -aiff                  Output aiff audio data\n");
	printf("  -au                    Output au audio data\n");
	printf("  -raw                   Output raw audio data\n");
	printf("  -stdout                Output raw audio data to <stdout>\n");
	printf("\n");
	printf("Subformats\n");
	printf("  -de                    File format default endian-ness (default)\n");
	printf("  -le                    Little Endian\n");
	printf("  -be                    Big Endian\n");
	printf("  -ce                    Your CPU's endian-ness\n");
	printf("\n");
	printf("  -pcm8                  Signed 8-bit data\n");
	printf("  -pcm16                 Signed 16-bit data (default)\n");
	printf("  -pcm24                 Signed 24-bit data\n");
	printf("  -pcm32                 Signed 32-bit data\n");
	printf("  -float                 32-bit floating point\n");
	printf("  -double                64-bit floating point\n");
	printf("  -ulaw                  U-Law\n");
	printf("  -alaw                  A-Law\n");
	printf("\n");
	printf("  -mask filename         Use sound \"filename\" as FFT mask\n");
	printf("  -length=[seconds]      Specify a length in seconds (may lose some detail!)\n");
	}

int readoptions(int argc, char** argv)
	{
	char arg[256];
	int i,j;
	float f;
	int filecount=0;

	for (i=1; i < argc; i++)
		{
		if (argv[i][0]=='-')
			{
			//number of channels
			if ( (strcmp(argv[i],"-help")==0) || (strcmp(argv[i],"--help")==0))
				{
				printoptions();
				return 1;
				}
			else if ( (strcmp(argv[i],"-m")==0) || (strcmp(argv[i],"-mono")==0) )
				{
				channels=1;
				}
			else if ( (strcmp(argv[i],"-s")==0) || (strcmp(argv[i],"-stereo")==0) )
				{
				channels=2;
				}
			//color inversion
			else if ( (strcmp(argv[i],"-i")==0) || (strcmp(argv[i],"-inverse")==0) )
				{
				inverse=1;
				}
			//oversampling and hiss
			else if ( (strcmp(argv[i],"-oversample")==0) || (strcmp(argv[i],"-o")==0) )
				{
				oversample=1;
				randphase=1;
				vgamma=(vgamma < HISSGAMMA ? HISSGAMMA : vgamma);
				}
			else if ( (strcmp(argv[i],"-hiss")==0) || (strcmp(argv[i],"-h")==0) )
				{
				randphase=1;
				vgamma=(vgamma < HISSGAMMA ? HISSGAMMA : vgamma);
				}
			//file types
			else if (strcmp(argv[i],"-stdout")==0)
				{
				audioformat=STDOUTAUDIO;
				}
			else if (strcmp(argv[i],"-raw")==0)
				{
				audioformat=RAWAUDIO;
				}
			else if (strcmp(argv[i],"-wav")==0)
				{
				audioformat=WAVAUDIO;
				}
			else if (strcmp(argv[i],"-aiff")==0)
				{
				audioformat=AIFFAUDIO;
				}
			else if (strcmp(argv[i],"-au")==0)
				{
				audioformat=AUAUDIO;
				}
			//data formats
			else if (strcmp(argv[i],"-de")==0)
				{
				endianformat=SF_ENDIAN_FILE;
				}
			else if (strcmp(argv[i],"-le")==0)
				{
				endianformat=SF_ENDIAN_LITTLE;
				}
			else if (strcmp(argv[i],"-be")==0)
				{
				endianformat=SF_ENDIAN_BIG;
				}
			else if (strcmp(argv[i],"-ce")==0)
				{
				endianformat=SF_ENDIAN_CPU;
				}
			else if (strcmp(argv[i],"-pcm8")==0)
				{
				dataformat=SF_FORMAT_PCM_S8;
				}
			else if (strcmp(argv[i],"-pcm16")==0)
				{
				dataformat=SF_FORMAT_PCM_16;
				}
			else if (strcmp(argv[i],"-pcm24")==0)
				{
				dataformat=SF_FORMAT_PCM_24;
				}
			else if (strcmp(argv[i],"-pcm32")==0)
				{
				dataformat=SF_FORMAT_PCM_32;
				}
			else if (strcmp(argv[i],"-float")==0)
				{
				dataformat=SF_FORMAT_FLOAT;
				}
			else if (strcmp(argv[i],"-double")==0)
				{
				dataformat=SF_FORMAT_DOUBLE;
				}
			else if (strcmp(argv[i],"-ulaw")==0)
				{
				dataformat=SF_FORMAT_ULAW;
				}
			else if (strcmp(argv[i],"-alaw")==0)
				{
				dataformat=SF_FORMAT_ALAW;
				}
/*			else if (strncmp(argv[i],"-fs",3)==0)
				{
				if ( (strncmp(argv[i],"-fs=",4)!=0) || (strlen(argv[i])<5) )
					{
					fprintf(stderr,"Error: You need to specify a font size such as:\n");
					fprintf(stderr,"-fs=2\n");
					exit(BAD_ARGS);
					}
				else 
					{
					strncpy(arg,&(argv[i][4]),255);
					j=atoi(arg);
					if ((j<0) || (j>4))
						{
						fprintf(stderr,"Error: You need to specify a font size between 0 and 4.\n");
						exit(BAD_ARGS);
						}
					else
						{
						fontsize=j;
						}
					}
				}//-fs
*/
			else if (strncmp(argv[i],"-lf",3)==0)
				{
				if ( (strncmp(argv[i],"-lf=",4)!=0) || (strlen(argv[i])<5) )
					{
					fprintf(stderr,"Error: You need to specify a starting frequency such as:\n");
					fprintf(stderr,"-lf=25\n");
					exit(BAD_ARGS);
					}
				else 
					{
					strncpy(arg,&(argv[i][4]),255);
					f=atof(arg);
					if ((f<0) || (f>100))
						{
						fprintf(stderr,"Error: You need to specify a starting frequency between 0 and 100.\n");
						exit(BAD_ARGS);
						}
					else
						{
						lowfreq=f;
						}
					}
				}//-lf
			else if (strncmp(argv[i],"-hf",3)==0)
				{
				if ( (strncmp(argv[i],"-hf=",4)!=0) || (strlen(argv[i])<5) )
					{
					fprintf(stderr,"Error: You need to specify a stop frequency such as:\n");
					fprintf(stderr,"-hf=25\n");
					exit(BAD_ARGS);
					}
				else 
					{
					strncpy(arg,&(argv[i][4]),255);
					f=atof(arg);
					if ((f<0) || (f>100))
						{
						fprintf(stderr,"Error: You need to specify a stop frequency between 0 and 100.\n");
						exit(BAD_ARGS);
						}
					else
						{
						highfreq=f;
						}
					}
				}//-hf
			else if (strncmp(argv[i],"-length",7)==0)
				{
				if ( (strncmp(argv[i],"-length=",8)!=0) || (strlen(argv[i])<9) )
					{
					fprintf(stderr,"Error: You need to specify a length in seconds such as:\n");
					fprintf(stderr,"-length=25.0\n");
					exit(BAD_ARGS);
					}
				else 
					{
					strncpy(arg,&(argv[i][8]),255);
					f=atof(arg);
					if ((f<1) || (f>1000))
						{
						fprintf(stderr,"Error: You need to specify a length in seconds between 1 and 1000.\n");
						exit(BAD_ARGS);
						}
					else
						{
						length=f;
						}
					}
				}//-length
			else if (strncmp(argv[i],"-mask",5)==0)
				{
				if ( i+1 >= argc )
					{
					fprintf(stderr,"Error: You need to specify a audio filename such as:\n");
					fprintf(stderr,"-mask file.wav\n");
					exit(BAD_ARGS);
					}
				else 
					{
					maskfilename=argv[i+1];
					masking=1;
					i++;
					}
				}//-mask
			else if (strncmp(argv[i],"-rate",5)==0)
				{
				if ( (strncmp(argv[i],"-rate=",6)!=0) || (strlen(argv[i])<7) )
					{
					fprintf(stderr,"Error: You need to specify a sample rate such as:\n");
					fprintf(stderr,"-rate=44100\n");
					exit(BAD_ARGS);
					}
				else 
					{
					strncpy(arg,&(argv[i][6]),255);
					j=atoi(arg);
					if ((j<1) || (j>96000))
						{
						fprintf(stderr,"Error: You need to specify a sample rate between 1 and 96000.\n");
						exit(BAD_ARGS);
						}
					else
						{
						samplerate=j;
						}
					}
				}//-rate
			else if (strncmp(argv[i],"-transform-size",15)==0)
				{
				if ( (strncmp(argv[i],"-transform-size=",6)!=0) || (strlen(argv[i])<7) )
					{
					fprintf(stderr,"Error:transform size:\n");
					fprintf(stderr,"-transform-size=3\n");
					exit(BAD_ARGS);
					}
				else 
					{
					strncpy(arg,&(argv[i][6]),255);
					j=atoi(arg);
					if ((j<0) || (j>4))
						{
						fprintf(stderr,"Error: You need to specify a transform size between 0 and 4.\n");
						exit(BAD_ARGS);
						}
					else
						{
						transize=j;
						}
					}
				}//-transform
			else if (strncmp(argv[i],"-ts",3)==0)
				{
				if ( (strncmp(argv[i],"-ts=",4)!=0) || (strlen(argv[i])<5) )
					{
					fprintf(stderr,"Error: You need to specify a transform size such as:\n");
					fprintf(stderr,"-ts=3\n");
					exit(BAD_ARGS);
					}
				else 
					{
					strncpy(arg,&(argv[i][4]),255);
					j=atol(arg);
					if ((j<0) || (j>7))
						{
						fprintf(stderr,"Error: You need to specify a transform size between 0 and 7.\n");
						exit(BAD_ARGS);
						}
					else
						{
						transize=j;
						}
					}
				}//-ts
			else if (strncmp(argv[i],"-color",6)==0)
				{
				if (strncmp(argv[i],"-color=",7)!=0)
					{
					fprintf(stderr,"Error: You need to specify a color mode such as:\n");
					fprintf(stderr,"-color=g\n");
					exit(BAD_ARGS);
					}
				else 
					{
					strncpy(arg,&(argv[i][7]),255);
					if (strcmp(arg,"g")==0)
						{
						colormode=GREYSCALE;
						}
					else if (strcmp(arg,"m")==0)
						{
						colormode=MONOCHROME;
						}
					else if (strcmp(arg,"rc")==0)
						{
						colormode=REDCYAN;
						}
					else if (strcmp(arg,"oa")==0)
						{
						colormode=ORANGEAQUA;
						}
					else if (strcmp(arg,"yb")==0)
						{
						colormode=YELLOWBLUE;
						}
					else if (strcmp(arg,"gp")==0)
						{
						colormode=GREENPURPLE;
						}
					else if (strcmp(arg,"wb")==0)
						{
						colormode=WHITEBLACK;
						}
					else if (strcmp(arg,"monet")==0)
						{
						colormode=EXPERIMENTAL;
						vgamma=(vgamma < EXPGAMMA ? EXPGAMMA : vgamma);
						}
					else
						{
						fprintf(stderr,"Error: Legal color modes are:\n");
						fprintf(stderr,"g, m, rc, oa, yb, gp, wb, and monet (experimental)\n");
						exit(BAD_ARGS);
						}
					 }
				}//-color
/*			else if (strcmp(argv[i],"-text")==0)
				{
				if (argc < i + 2)
					{
					fprintf(stderr,"Error: no text!\n");
					exit(BAD_ARGS);
					}
				else
					{
					i++;
					text=argv[i];
					filecount++; //We don't need an input filename
					}
				}//-text
*/
			else
				{
				fprintf(stderr,"Error: Unknown option: %s\n",argv[i]);
				exit(BAD_ARGS);
				}
			
			}//switches

		else if (filecount<2)
			{
			if (filecount==0)
				{
				inputfilename=argv[i];
				filecount++;
				}
			else if (filecount==1)
				{
				outputfilename=argv[i];
				filecount++;
				}
			}//filenames
		else
			{
			fprintf(stderr,"Error: Extra filename: %s\n",argv[i]);
			exit(BAD_ARGS);
			}
		
		}//for i
	
	if (filecount==0)
		{
		fprintf(stderr,"Error: No filenames.\n");
		exit(BAD_ARGS);
		}  
	else if (filecount==1)
		{
		if (audioformat==RAWAUDIO)
			{
			fprintf(stderr,"Warning: No output filename, using %s instead\n", defaultoutputfilenameraw);
			outputfilename=defaultoutputfilenameraw;
			}
		else if (audioformat==WAVAUDIO)
			{
			fprintf(stderr,"Warning: No output filename, using %s instead\n", defaultoutputfilenamewav);
			outputfilename=defaultoutputfilenamewav;
			}
		else if (audioformat==AIFFAUDIO)
			{
			fprintf(stderr,"Warning: No output filename, using %s instead\n", defaultoutputfilenameaiff);
			outputfilename=defaultoutputfilenameaiff;
			}
		else if (audioformat==AUAUDIO)
			{
			fprintf(stderr,"Warning: No output filename, using %s instead\n", defaultoutputfilenameau);
			outputfilename=defaultoutputfilenameau;
			}
		else if (audioformat==STDOUTAUDIO)
			{
			fprintf(stderr,"Writing raw audio to <stdout>\n");
			outputfilename="<stdout>";
			}
		}  

	if (highfreq <= lowfreq)
		{
		fprintf(stderr,"Error: Bad start and stop frequencies.\n");
		}
		
	return 1;
	}

void checkmem(void *ptr,char* msg)
	{
	if 	(ptr==NULL)
		{
		fprintf(stderr,"ERROR: Memory allocation failed for %s\n",msg);
		exit(MEM_ERROR);
		}
	}

int main(int argc, char** argv) 
	{
	gdImagePtr im,im2;

  // gd fonts for textmode
  //gdFontPtr font;

	FILE *in;

	SF_INFO out_info;
	SF_INFO mask_info;
	SNDFILE* out;
	SNDFILE* maskfile;

	int size, halfsize, quartersize, dblsize;
	int startx,stopx;
	
	int w,h,h2;
	float scale,cr,cl,cm;
//	float dif;
	float fftscale,bytescale;
	float inv_halfsize;
	float inv_quartersize;

	int c,x,x2,y,t,i,j;
	
	float *realL;  
	float *imagL;
	float *realR;  
	float *imagR;
	float *bufR1,*bufR2,*bufR3,*bufR4;  
	float *bufL1,*bufL2,*bufL3,*bufL4;  
	float *temp;
	
	float *buftblL[4];
	float *buftblR[4];
	float *bl1,*bl2,*bl3,*bl4;
	float *br1,*br2,*br3,*br4;
	
	float *lerp;
	float *window;
	float *mask;
	
	float ph0,ph1;
	
	float gammatable[256];
	
	printblurb();
	
	if (argc==1)
		{
		printoptions();
		exit(NO_ARGS);
		}
	else readoptions(argc,argv);	
	
	//Now we have options
	if ((channels<2)&&(colormode>=REDCYAN))
		{
		fprintf(stderr,"Error: only greyscale and monochrome color modes are available in mono.\n");
		exit(INCOMPAT_MODE);
		}
	
	ldn=transize+9;
	size=1 << ldn;
	dblsize=size << 1;
	halfsize=size >> 1;
	inv_halfsize=1.0/halfsize;
	
	quartersize=size >> 2;
	inv_quartersize=1.0/quartersize;
	
	fprintf(stderr,"Encoding image %s into ",inputfilename);
	switch (audioformat)
		{
		case (WAVAUDIO):fprintf(stderr,"Microsoft WAV");break;
		case (AIFFAUDIO):fprintf(stderr,"Apple AIFF");break;
		case (AUAUDIO):fprintf(stderr,"Sun AU");break;
		case (RAWAUDIO):fprintf(stderr,"raw");break;
		}
	fprintf(stderr," audio file %s\n", outputfilename);
	fprintf(stderr,"samplerate: %d Hz  channels: %d\n",samplerate,channels);
	if (randphase)
		fprintf(stderr,"random phase hiss\n");
	if (oversample)
		fprintf(stderr,"oversampling\n");

	
	fftscale=1.0 / ((float)halfsize); //we may need to fudge this down even more to prevent clipping
	bytescale=1.0 / 255.0;
	
	for (i=0;i<256;i++)
		{
		gammatable[i]=powf((i*bytescale),vgamma);
		}
	
	realL=(float*)calloc(size,sizeof(float));
	checkmem(realL,"realL");
	imagL=(float*)calloc(size,sizeof(float));
	checkmem(imagL,"imagL");

	if (channels==2)
		{
		realR=(float*)calloc(size,sizeof(float));
		checkmem(realR,"realR");
		imagR=(float*)calloc(size,sizeof(float));
		checkmem(imagR,"imagR");
		}
	
	if (channels==1)
		{	
		temp=(float*)calloc(size,sizeof(float));
		}
	else
		{
		temp=(float*)calloc(dblsize,sizeof(float));
		}
	checkmem(temp,"temp");
	
	if (masking)
		{	
		mask=(float*)calloc(halfsize,sizeof(float));
		checkmem(mask,"mask");
		}
	
  if (oversample)
		{
		bufL1=(float*)calloc(quartersize,sizeof(float));
		bufL2=(float*)calloc(quartersize,sizeof(float));
		bufL3=(float*)calloc(quartersize,sizeof(float));
		bufL4=(float*)calloc(quartersize,sizeof(float));
		checkmem(bufL1,"bufL1");
		checkmem(bufL2,"bufL2");
		checkmem(bufL3,"bufL3");
		checkmem(bufL4,"bufL4");
		
		if (channels==2)
			{
		bufR1=(float*)calloc(quartersize,sizeof(float));
		bufR2=(float*)calloc(quartersize,sizeof(float));
		bufR3=(float*)calloc(quartersize,sizeof(float));
		bufR4=(float*)calloc(quartersize,sizeof(float));
		checkmem(bufR1,"bufR1");
		checkmem(bufR2,"bufR2");
		checkmem(bufR3,"bufR3");
		checkmem(bufR4,"bufR4");
			}

	  lerp = (float*)calloc(size, sizeof(float));
		checkmem(lerp,"lerp");
	
	  for (i=0;i<size;i++)
		  {
			float a;
			//Ok, this was a lerp (linear interpolation) for fading, 
			//then it was a cubic spline I found in povray's noise functions
			//and now is just have a sine wave cycle
  		
			//lerp[i]=(float)i/(float)halfsize;
			a=(float)i/(float)(size-1);
			//lerp[i]=((a)*(a)*(3.0-(2.0*(a))));
			lerp[i]=sin(M_PI * a);
			}
    }


	if (masking)
		{
		int mc,ms,mcount=0,mcc=0;
		float maxp=0;
		float *mbuf;
		fprintf(stderr,"Opening mask file\n");
		maskfile=sf_open(maskfilename,SFM_READ,&mask_info);
		if (maskfile==NULL)
			{
			fprintf(stderr,"Error opening mask file: %s\n",maskfilename);
			perror(NULL);
			exit(AUDIO_FILE_ERROR);
			}

		mc=mask_info.channels;
		ms=size*mc;
		mbuf=(float*)calloc(ms,sizeof(float));
		checkmem(mbuf,"mbuf");

		memset((void *)realL, 0, sizeof(float)*size);
		memset((void *)imagL, 0, sizeof(float)*size);

		do
			{
			mcount=sf_read_float(maskfile, mbuf, ms);
			if (mcount!=ms) break;
			mcc++;
			if (mc==1)
				{
				for (i=0;i<size;i++)
					realL[i]+=mbuf[i];
				}
			else if (mc==2)
				{
				for (i=0,j=0;i<size;i++,j+=2)
					realL[i]+=mbuf[j]+mbuf[j+1];
				}
			else
				{
				fprintf(stderr,"Cannot read %d channel sound %s\n",mc,maskfilename);
				exit(AUDIO_FORMAT_ERROR);
				}
			
			}
		while (mcount==ms); //redundant
		fprintf(stderr,"Read %d chunks from %d channel sound %s\n",mcc,mc,maskfilename);
		
		if (mcc==0)
			{		
			fprintf(stderr,"Could not get enough audio data from %s!\nSwitching to non-masking mode!\n",maskfilename);
			masking=0;
			}
		else
			{
			//Windowing
			for (i=0;i<size;i++)
				{
				realL[i]*=sin((M_PI*(i))/size);
				}

			fprintf(stderr,"Performing FFT...\n");
			fft(realL,imagL,ldn,+1);
		
			startx=(lowfreq/100.0)*halfsize; //redundant
			stopx=(highfreq/100.0)*halfsize;

			for (i=startx;i<stopx;i++)
				{
				mask[i]=sqrt(realL[i]*realL[i]+imagL[i]*imagL[i]);
				if (mask[i]>maxp) maxp=mask[i];
				}

			for (i=0;i<startx;i++) mask[i]=0;
			for (i=stopx;i<halfsize;i++) mask[i]=0;

			if (maxp>0)
				for (i=startx;i<stopx;i++)
					{
					mask[i]=(1<<transize)*40*mask[i]/maxp;
					};
			}
		free(mbuf);
		sf_close(maskfile);
		}

	if (!textmode)
		{
		char *lower;
		int namelength;

		in = fopen(inputfilename, "r");
		
		if (in==NULL)
			{
			fprintf(stderr,"Error opening input file: %s\n",inputfilename);
			perror(NULL);
			exit(FILE_ERROR);
			}
			
		namelength=strlen(inputfilename);
		lower = (char*)calloc(namelength,sizeof(char));
		
		for (i=0;i<namelength;i++)
			{
			lower[i]=(char)tolower((int)inputfilename[i]);
			}
		
		im=NULL;
		if (strstr(lower,".jpg")!=NULL)
			{im = gdImageCreateFromJpeg(in);}
		else if (strstr(lower,".jpeg")!=NULL)
			{im = gdImageCreateFromJpeg(in);}
		else if (strstr(lower,".jfif")!=NULL)
			{im = gdImageCreateFromJpeg(in);}
		else if (strstr(lower,".png")!=NULL)
			{im = gdImageCreateFromPng(in);}
		
		
		else if (strstr(lower,".wbmp")!=NULL)
			{im = gdImageCreateFromWBMP(in);}
		else if (strstr(lower,".xbm")!=NULL)
			{im = gdImageCreateFromXbm(in);}
		else if (strstr(lower,".gd2")!=NULL)
			{im = gdImageCreateFromGd2(in);}
		else if (strstr(lower,".gd")!=NULL)
			{im = gdImageCreateFromGd(in);}
			
		if (im==NULL)
					{
					fprintf(stderr,"Error: Cannot decipher image file %s from extension\n",inputfilename);
					exit(IMAGE_READ_ERROR);
					}

		free(lower);		
		fclose(in);

		w = gdImageSX(im);
		h = gdImageSY(im);
	
		if (w<1)
			{
			fprintf(stderr,"Error: Bad input image width: %d\n",w);
			exit(IMAGE_FORMAT_ERROR);
			}
	
		if (h<1)
			{
			fprintf(stderr,"Error: Bad input image height: %d\n",h);
			exit(IMAGE_FORMAT_ERROR);
			}
	
		scale=halfsize/((float)w);
		
		startx=(lowfreq/100.0)*halfsize;
		stopx=(highfreq/100.0)*halfsize;

		//block DC and next partial
		if (startx < 2) startx = 2;
		//Avoid getting too close to nyquist
		if (stopx > halfsize - 2) stopx = halfsize - 2;
 				
		}// input image load

/*	else //we're in textmode
		{
		switch (fontsize)
			{
			case 0:
				font=gdFontGetTiny();
				break;
			case 1:
				font=gdFontGetSmall();
				break;
			case 2:
				font=gdFontGetMediumBold();
				break;
			case 3:
				font=gdFontGetLarge();
				break;
			case 4:
				font=gdFontGetGiant();
				break;
			default:
				fprintf(stderr,"Internal Error: Bad font size: %d.\n",fontsize);
				exit(FONT_ERROR);
			}
		scale=1.0;//???
		w=1;//???
		h=stopx - startx;//???
		}
*/
	if (length>0)
		h2=(length*samplerate)/size;
	else
		{
		h2=h*scale;
		//h2=h*512/((float)w);
		//h2=h >> transize;
		}
	if (oversample)
		im2 = gdImageCreateTrueColor( halfsize, (int)(h2*4) );
	else
		im2 = gdImageCreateTrueColor( halfsize, (int)(h2) );

	fprintf(stderr,"Scaling Image\n");
	gdImageCopyResampled(im2, im, startx, 0, 0, 0, stopx-startx, im2->sy, im->sx, im->sy);

//-----------Setup Output-------------- 
	
	out_info.frames     = h2 * size;    
	out_info.samplerate = samplerate ;
	out_info.channels   = channels;
	out_info.sections   = 0;
	out_info.seekable   = 0;

	out_info.format=endianformat|dataformat;
	
	if (audioformat==WAVAUDIO)
		{
		out_info.format     |= SF_FORMAT_WAV;
		}
	else if (audioformat==RAWAUDIO)
		{
		out_info.format     |= SF_FORMAT_RAW;
		}
	else if (audioformat==AIFFAUDIO)
		{
		out_info.format     |= SF_FORMAT_AIFF;
		}
	else if (audioformat==AUAUDIO)
		{
		out_info.format     |= SF_FORMAT_AU;
		}
	else if (audioformat==STDOUTAUDIO)
		{
		out_info.format     |= SF_FORMAT_RAW;
		}

	if (! sf_format_check(&out_info))
		{
		fprintf(stderr,"Error: Bad sound file format settings!\n");
		exit(AUDIO_FORMAT_ERROR);
		} 

fprintf(stderr,"Opening output file\n");

	if (audioformat==STDOUTAUDIO)
		{
		out=sf_open_fd(STDOUT_FILENO, SFM_WRITE, &out_info, 0);
		}
	else
		{
		out=sf_open(outputfilename, SFM_WRITE, &out_info);
		}

	if (out==NULL)
		{
		fprintf(stderr,"Error: Error opening output file: %s\n",outputfilename);
		perror(NULL);
		exit(AUDIO_FILE_ERROR);
		}

	
	if (colormode==MONOCHROME)
		{
		gdImageTrueColorToPalette(im2, 1, 2);
		}

	gdImageDestroy(im);  
	
	window=(float*)calloc(im2->sx,sizeof(float));
	for (x = startx; x < stopx; x++)
		{
		float dw=stopx-startx;
		window[x]=sin(M_PI*(x-startx)/dw);
		}
		
	if (oversample)
		{
		buftblL[0]=bufL1;
		buftblL[1]=bufL2;
		buftblL[2]=bufL3;
		buftblL[3]=bufL4;
		buftblR[0]=bufR1;
		buftblR[1]=bufR2;
		buftblR[2]=bufR3;
		buftblR[3]=bufR4;
		}
	fprintf(stderr,"Building output audio file\n");

	//-----------Main loop------------//
	for (y = 0; y < im2->sy; y++)
		{
		memset(realL,0,size*sizeof(float));
		memset(imagL,0,size*sizeof(float));

		if (channels==2)
			{
			memset(realR,0,size*sizeof(float));
			memset(imagR,0,size*sizeof(float));
			}
		
		for (x = startx; x < stopx; x++)
			{
			c = gdImageGetPixel(im2,x,y);
			
			switch (colormode)
				{
				case GREYSCALE:
					cl = cr = ( 0.3 * gdImageRed(im2,c) + 0.59 * gdImageGreen(im2,c) + 0.11 * gdImageGreen(im2,c) );
					break;
				case MONOCHROME:
					cl = cr = ( 0.3 * gdImageRed(im2,c) + 0.59 * gdImageGreen(im2,c) + 0.11 * gdImageGreen(im2,c) );
					break;
				case REDCYAN:
					cl = gdImageRed(im2,c);
					cr = 0.5 * gdImageBlue(im2,c) +  0.5 * gdImageGreen(im2,c);
					break;
				case ORANGEAQUA:
					cl = ( gdImageRed(im2,c)  + ( 0.5 * gdImageGreen(im2,c)) ) / 1.5;
					cr = ( gdImageBlue(im2,c) + ( 0.5 * gdImageGreen(im2,c)) ) / 1.5;
					break;
				case YELLOWBLUE:
					cl = 0.5 * gdImageRed(im2,c) +  0.5 * gdImageGreen(im2,c);
					cr = gdImageBlue(im2,c);
					break;
				case GREENPURPLE:
					cl = gdImageGreen(im2,c);
					cr = 0.5 * gdImageBlue(im2,c) +  0.5 * gdImageRed(im2,c);
					break;
				case WHITEBLACK:
					cl = ( 0.3 * gdImageRed(im2,c) + 0.59 * gdImageGreen(im2,c) + 0.11 * gdImageGreen(im2,c) );
					cr = 255 - cl;
					break;
				case EXPERIMENTAL:
					cl = gdImageRed(im2,c);
					cr = gdImageGreen(im2,c);
					cm = gdImageBlue(im2,c);
					#define LP 30.0					
					if (cl<LP) cl=LP;
					if (cr<LP) cr=LP;
					if (cm<LP) cm=LP;
					break;
				default:  
					fprintf(stderr,"Internal Error: Bad color mode: %d.\n",colormode);
					exit(BAD_COLORMODE);
			 	}			
			
			if (inverse)
				{
				cl = 255 - cl;
				cr = 255 - cr;	
				}
			
			cl = gammatable[(unsigned char)cl];
			cl *= window[x];
			
			if (channels==2)
				{
				cr = gammatable[(unsigned char)cr];
				cr *= window[x];
				}
			
			if (randphase)			
				{ph0=1.0*FRand2()*M_PI;}
			else
				{
				ph0=M_PI/2.0;
				//ph0=M_PI*sin(2*M_PI*((y+x)/(float)im2->sy));
				}
			
			if (colormode==EXPERIMENTAL)
				{
				cm = gammatable[(unsigned char)cm];
				cm *= window[x];
				
				cm *= 2.0 - 1.0;
				if (inverse)
					{cm=-cm;}

				//dif = (cl - cr)/2.0;
				//ph1 = ph0 + acos(-cm * fabs(dif));
				
				ph1 = ph0 + acos(-cm);
				}
			else
				{ph1=ph0;}
		
			cl *= fftscale;
			if (channels==2)
				cr *= fftscale;

			imagL[x] =  cl * sin(ph1);
			realL[x] =  cl * cos(ph1);
			if (channels==2)
				{
				imagR[x] =  cr * sin(ph0);
				realR[x] =  cr * cos(ph0);
				}
			
			if (masking)
				{
				imagL[x] *=  mask[x];
				realL[x] *=  mask[x];
				if (channels==2)
					{
					imagR[x] *=  mask[x];
					realR[x] *=  mask[x];
					}
				}
			
			
			}
		
		fft(realL,imagL,ldn,-1);
		if (channels==2)
			fft(realR,imagR,ldn,-1);
		
		if (oversample)
			{	
			//yeah, it's ugly, but easier for me to understand later
			bl1=buftblL[(y)&3];bl2=buftblL[(y+1)&3];bl3=buftblL[(y+2)&3];bl4=buftblL[(y+3)&3];				
			br1=buftblR[(y)&3];br2=buftblR[(y+1)&3];br3=buftblR[(y+2)&3];br4=buftblR[(y+3)&3];				
			for (x = 0, t=0; x < quartersize; x++, t++)
				{
				x2=x;
				bl1[x] += lerp[x2] * realL[x2]; x2+=quartersize;
				bl2[x] += lerp[x2] * realL[x2]; x2+=quartersize;
				bl3[x] += lerp[x2] * realL[x2]; x2+=quartersize;
				bl4[x] += lerp[x2] * realL[x2];
				temp[t]=bl1[x];
				bl1[x]=0;
				
				if (channels==2)
					{
					x2=x;
					br1[x] += lerp[x2] * realR[x2]; x2+=quartersize;
					br2[x] += lerp[x2] * realR[x2]; x2+=quartersize;
					br3[x] += lerp[x2] * realR[x2]; x2+=quartersize;
					br4[x] += lerp[x2] * realR[x2];
					temp[++t]=br1[x];
					br1[x]=0;
					}
				}		
			
			if (sf_writef_float(out, temp, quartersize)!=quartersize)
				{
				fprintf(stderr,"ERROR: SF_WRITE Failed\n");
				exit(AUDIO_WRITE_ERROR);
				}

			if (y == im2->sy - 1)//We are at the end of the run, so flush our buffers
				{
				for (x = 0, t=0; x < quartersize; x++, t++)
					{
					temp[t]=bl2[x];
					if (channels==2) temp[++t]=br2[x];
					}
				for (x = 0;x < quartersize; x++, t++)
					{
					temp[t]=bl3[x];
					if (channels==2) temp[++t]=br3[x];
					}
				for (x = 0; x < quartersize; x++, t++)
					{
					temp[t]=bl4[x];
					if (channels==2) temp[++t]=br4[x];
					}
				if (sf_writef_float(out, temp, 3*quartersize)!=3*quartersize)
					{
					fprintf(stderr,"ERROR: SF_WRITE Failed at tail\n");
					exit(AUDIO_WRITE_ERROR);
					}

				}
			}
		else
			{
			for (x = 0, t=0; x < size; x++, t++)
				{
				temp[t]   = realL[x];
				if (channels==2)
					temp[++t] = realR[x];
				}		
			if (sf_writef_float(out, temp, size)!=size)
				{
				fprintf(stderr,"ERROR: SF_WRITE Failed\n");
				exit(AUDIO_WRITE_ERROR);
				}
			}
		}
	
	
	sf_close(out);

	gdImageDestroy(im2); 
		
	free(realL);
	free(imagL);
	if (channels==2)
		{
		free(realR);
		free(imagR);
		}
	
	free(temp);
	
	free(window);
	
  if (oversample)
		{
		free(lerp);
		free(bufL1);
		free(bufL2);
		free(bufL3);
		free(bufL4);
		
		if (channels==2)
			{
			free(bufR1);
			free(bufR2);
			free(bufR3);
			free(bufR4);
			}
		}
	
	fprintf(stderr,"Done.\n");

	return 0; 
	}


/*This code, with minor alterations, is from the GPL'd FXT-2000 Library*/
#define sumdiff(a,b,s,d)    { s=a+b; d=a-b; }
#define sumdiff_05(a,b,s,d) { s=0.5*(a+b); d=0.5*(a-b); }
#define sumdiff2(s,d)       { float t; t=s-d; s+=d; d=t; }
#define cmult(c,s,u,v)      { float t; t=u*s+v*c; u*=c; u-=v*s; v=t; }
#define SWAP(x,y)           { float tmp=x; x=y; y=tmp; }

void revbin_permute(float *fr, long n);

void fft(float *fr, float *fi, int ldn, int is)
/*
 * radix 2 fft a la cooley tukey
 *
 */
	{
	int    n2,ldm,m,mh,j,r;
	int    t1, t2;
	float pi,phi,c,s;
	float ur,vr, ui,vi;

	n2=1<<ldn;
	pi=is*M_PI;

	revbin_permute(fr,n2);
	revbin_permute(fi,n2);

	for (ldm=1; ldm<=ldn; ++ldm)
		{
		m=(1<<ldm);            /* m=2^ldm */
		mh=(m>>1);             /* mh=m/2 */

		phi=pi/(float)(mh);

		for (j=0; j<mh; ++j)
			{
			float w=phi*(float)j;

			c=cos(w);
			s=sin(w);

			for (r=0; r<n2; r+=m)
				{
				t1=r+j;
				t2=t1+mh;

				vr=fr[t2]*c-fi[t2]*s;
				vi=fr[t2]*s+fi[t2]*c;

				ur=fr[t1];
				fr[t1]+=vr;
				fr[t2]=ur-vr;

				ui=fi[t1];
				fi[t1]+=vi;
				fi[t2]=ui-vi;
				}
			}
		}
	} 

void revbin_permute(float *fr, long n)
	{
	long m,j;
	for (m=1,j=0; m<n-1; m++)
		{
		long k;
		for(k=n>>1; (!((j^=k)&k)); k>>=1);

		if (j>m)  SWAP(fr[m],fr[j]);
		}
	}
