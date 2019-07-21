#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <ctype.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "common/nsfsdk/nsfsdk.h"
#include "common/zlib/nez.h"
#include "kbhit.h"

#define NSFPLAY_VERSION     "0.1.3dev"

#define HAVE_SYS_IOCTL_H
#define HAVE_OSS_AUDIO

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_OSS_AUDIO
#  ifdef HAVE_MACHINE_SOUNDCARD_H
#   include <machine/soundcard.h>
#  else
#   ifdef HAVE_SOUNDCARD_H
#    include <soundcard.h>
#   else
/* #    include <linux/soundcard.h> */
#    include <sys/soundcard.h>
#   endif
#  endif
#endif

#define LEN_OUTBUF 4096

#ifndef DEFAULT_FREQ
#define DEFAULT_FREQ       44100
#endif

#ifndef DEFAULT_TIME
#define DEFAULT_TIME       300
#endif

#ifndef WAITTIME_TOSTART
#define WAITTIME_TOSTART   300000
#endif

#define SONG_STEPS 5

#define FINCODE_NORMAL    0
#define FINCODE_QUIT      1
#define FINCODE_NEXT      2
#define FINCODE_PREV      3
#define FINCODE_REWIND    4
#define FINCODE_FORWARD   5
#define FINCODE_BACKWARD  6
#define FINCODE_FORCESTOP 10

#define STOPSONG_THRESHOLD_VALUE  10
#define STOPSONG_THRESHOLD_TIMES  30

/*--------------------------------------------------------------------------*
 * nsfplay_single:
 *
 *
 *--------------------------------------------------------------------------*/
static int
nsfplay_single(int dev, HNSF hnsf, unsigned freq, unsigned time,
	       unsigned songno)
{
  signed short outbuf[4 * LEN_OUTBUF];
  int channel;
  unsigned remain;

  int ch = 0;
  int fincode = -1;
  int thresholdCount =0 ;
  int lastVal = 0;
  int isPaused = 0;

#ifdef ENABLE_BUFFERED_PCM
  signed short *pcm_buffer = NULL;
  int pcm_buffer_ptr;
  int pcm_buffer_size;

  int ptr;
#endif /* ENABLE_BUFFERD_PCM */

  /* init */
  ioctl( dev, SNDCTL_DSP_RESET, 0 );

  NSFSDK_SetSongNo(hnsf, songno);
  NSFSDK_SetFrequency(hnsf, freq);
  channel = NSFSDK_GetChannel(hnsf);
  NSFSDK_SetChannel(hnsf, channel);

#ifdef ENABLE_BUFFERED_PCM
  pcm_buffer_ptr = 0;
  ioctl( dev, SNDCTL_DSP_GETBLKSIZE, &pcm_buffer_size );
  pcm_buffer = (signed short *)malloc(pcm_buffer_size);
#endif /* ENABLE_BUFFERED_PCM */

  remain = freq * time;

  /* start */
  NSFSDK_Reset(hnsf);

  while ((remain > LEN_OUTBUF) && (fincode < 0)) {

    if (!isPaused) {
      remain -= LEN_OUTBUF;
      NSFSDK_Render(hnsf, outbuf, LEN_OUTBUF);

      /* judges whether the song already stopped */
#if 0
      if (abs(outbuf[0] < STOPSONG_THRESHOLD_VALUE)) {
	thresholdCount++;
	/* fprintf(stderr, "count: %d\n", thresholdCount); */
      } else {
	thresholdCount = 0;
      }

#else
      if (abs(lastVal - outbuf[0]) < STOPSONG_THRESHOLD_VALUE) {
	thresholdCount++;
      } else {
	thresholdCount = 0;
      }

      lastVal = outbuf[0];

#endif

      if (thresholdCount >= STOPSONG_THRESHOLD_TIMES) {
	fincode = FINCODE_FORCESTOP;
	continue;
      }

#ifdef ENABLE_BUFFERED_PCM
      for ( ptr=0 ; ptr<LEN_OUTBUF * channel ; ptr++ ) {
	if ( pcm_buffer_ptr >= pcm_buffer_size / (sizeof(signed short)) ) {
	  write( dev, pcm_buffer, pcm_buffer_size );
	  pcm_buffer_ptr = 0;
	}
	pcm_buffer[pcm_buffer_ptr++] = outbuf[ptr];
      }
#else
      write( dev, outbuf, LEN_OUTBUF * (sizeof(signed short) * channel));
#endif /* ENABLE_BUFFERED_PCM */

    } /* !isPaused */

    /* checking for key inputs */
    if (kbhit()) {
      ch = readch();
      //fprintf(stderr, "you hit \"%c\" - %d\n", ch, ch);

      switch (ch) {

      case 'q':
	fincode = FINCODE_QUIT;
	break;

      case '\n':    // LF / "OK" key for Zaurus SL-C700
      case '\r':    // CR
	//fincode = FINCODE_REWIND;
	//fincode = FINCODE_NEXT;
	isPaused = isPaused ? 0 : 1;
	if (isPaused) {
	  fprintf(stderr, "** Paused **\n");
	}
	break;

      case 0x1b:    // ESC / "Cancel" key for zaurus SL-C700
	if (kbhit()) {

	  /* For Arrow key: key stroke combination starting from ESC */
	  ch = readch();
	  //fprintf(stderr, "you hit also \"%c\" - %d\n", ch, ch);
	  if (ch == '[') {

	    if (kbhit()) {
	      ch = readch();
	      //fprintf(stderr, "and \"%c\" - %d\n", ch, ch);
	      switch (ch) {

	      case 'A':
		/* ESC + '[' + 'A' means "up arrow key" */
		//fincode = FINCODE_NEXT;
		fincode = FINCODE_PREV;
		break;

	      case 'B':
		/* ESC + '[' + 'B' means "down arrow key" */
		//fincode = FINCODE_PREV;
		fincode = FINCODE_NEXT;
		break;

	      case 'C':
		/* ESC + '[' + 'C' means "right arrow key" */
		fincode = FINCODE_FORWARD;
		break;

	      case 'D':
		/* ESC + '[' + 'D' means "left arrow key" */
		fincode = FINCODE_BACKWARD;
		break;
	      }
	    }
	  }

	} else {
	    /* "REAL" ESC key was pressed */
	    fincode = FINCODE_QUIT;
	}
	break;

      }
    }
  }

  /* ending */
  if (fincode < 0) {
    fincode = FINCODE_NORMAL;
    NSFSDK_Render(hnsf, outbuf, remain);

#ifdef ENABLE_BUFFERED_PCM
    write( dev, pcm_buffer, remain * (sizeof(signed short) * channel) );
#else
    write( dev, outbuf, remain * (sizeof(signed short) * channel) );
#endif /* ENABLE_BUFFERED_PCM */
  }

#ifdef ENABLE_BUFFERED_PCM
  if (pcm_buffer != NULL) {
    free(pcm_buffer);
  }
#endif /* ENABLE_BUFFERED_PCM */

  return fincode;
}

/*--------------------------------------------------------------------------*
 * nsfplay_main:
 *
 *
 *--------------------------------------------------------------------------*/
static void
nsfplay_main(char *ipath, unsigned freq, unsigned time, unsigned songno,
	     int isSingleMode)
{
  FILE *ifp = NULL;
  unsigned char *nsfbuf = NULL;
  void *nezbuf;
  int nezsize;
  char *pathbuf = NULL;
  HNSF hnsf = NULL;
  setlocale(LC_ALL, "");
  do {
    size_t size, len;

    int dev;
    int arg;

    unsigned songMax;
    int result;

    fprintf(stderr, "Initializing ... ");

    if (freq == 0) freq = DEFAULT_FREQ;
    if (time == 0) time = DEFAULT_TIME;
    //		remain = freq * time;
#if 0
    size = 0;
    len = strlen(ipath);
    if (size < len) size = len;
    pathbuf = malloc(size + 8);
    if (pathbuf == NULL) {
      fprintf(stderr, "Error: Short of memory\n");
      break;
    }
    ifp = fopen(ipath, "rb");
    if (ifp == NULL) {
      strcpy(pathbuf, ipath);
      strcat(pathbuf, ".nsf");
      ifp = fopen(pathbuf, "rb");
      if (ifp == NULL) {
	fprintf(stderr, "Error: File '%s' cannot open\n", ipath);
	break;
      }
    }

    fseek(ifp, 0, SEEK_END);
    size = ftell(ifp);
    nsfbuf = malloc(size + 16);
    if (nsfbuf == NULL) {
      fprintf(stderr, "Error: Short of memory\n");
      break;
    }
    fseek(ifp, 0, SEEK_SET);
    fread(nsfbuf, 1, size, ifp);
    fclose(ifp); ifp = NULL;

    hnsf = NSFSDK_Load(nsfbuf, size);

#else
    nezsize = NEZ_extract(ipath, &nezbuf);
    if ((nezsize == 0) || (nezbuf == NULL)) {
      fprintf(stderr, "Error: Failed to load file '%s'\n", ipath);
      break;
    }

    hnsf = NSFSDK_Load(nezbuf, nezsize);
    free(nezbuf);
#endif

    if (hnsf == NULL) {
      fprintf(stderr, "Error: Fail to load NSF\n");
      break;
    }
    if (songno == 0) songno = NSFSDK_GetSongNo(hnsf);

    dev = open( "/dev/dsp", O_WRONLY );
    if ( dev < 0 ) {
      fprintf(stderr, "Error: cannot open /dev/dsp\n");
      break;
    }
		  
    /* set 16 bit mode */
    /*
      fixes to "Little endian signed 16"
      see: linux/soundcard.h, pcm8.c
    */
    arg = AFMT_S16_LE;
    if ( ioctl( dev, SNDCTL_DSP_SETFMT, &arg )<0 ||
	 arg != AFMT_S16_LE ) {
      break;
    }

    /* set surround mode */
    /* NSFSDK_GetChannel(hnsf) returns ... 1: Monaural 2: Stereo */
    arg = NSFSDK_GetChannel(hnsf) - 1;
    if ( ioctl( dev, SNDCTL_DSP_STEREO, &arg )<0 ) {
      break;
    }
		  
    /* set the sample rate */
    arg = freq;
    if ( ioctl( dev, SNDCTL_DSP_SPEED, &arg )<0 ||
	 (int)(arg/100) != (int)(freq/100) ) {
      break;
    }

    /* get the max number for current songdata */
    songMax = NSFSDK_GetSongMax(hnsf);
    if (songno > songMax) {
      songno = songMax;
    }

    /*** start ***/
    fprintf(stderr, "done.\n");
    if (!isSingleMode) {
      fprintf(stderr, "+++ Hit arrow key to change song, 'q' or 'Cancel' key to quit +++\n");
    }

    init_keyboard();

    do {
      fprintf(stderr, "SongNo. [ %3d ]\n", songno);
      usleep(WAITTIME_TOSTART);

      /* play a music specified by songno */
      result = nsfplay_single(dev, hnsf, freq, time, songno);
		  
      if (isSingleMode) {
	break;
      }

      switch (result) {
      case FINCODE_NORMAL:
      case FINCODE_NEXT:
      case FINCODE_FORCESTOP:
	songno++;
	break;

      case FINCODE_PREV:
	if (songno > 0) {
	  --songno;
	}
	break;

      case FINCODE_FORWARD:
	songno += SONG_STEPS;
	break;

      case FINCODE_BACKWARD:
	if ((int)songno - SONG_STEPS < 0) {
	  songno = 0;
	} else {
	  songno -= SONG_STEPS;
	}
	break;

      case FINCODE_REWIND:
      default:
	break;
      }

    } while ((result != FINCODE_QUIT) && (songno <= songMax));

    /*** end ***/
    close_keyboard();
		  
    fprintf(stderr, "Finished.\n");

  } while(0);

  if (hnsf != NULL) NSFSDK_Terminate(hnsf);
  if (ifp != NULL) fclose(ifp);
  if (nsfbuf != NULL) free(nsfbuf);
  if (pathbuf != NULL) free(pathbuf);
}

int main(int argc, char **argv)
{
	unsigned count = 0, freq = 0, time = 0, songno = 0;
	int i;
	int isSingleMode = 0;

	for (i = 1; i < argc; i++)
	{
		if (strncmp(argv[i],"-f",2) == 0)
		{
			char *p = argv[i] + 2;
			if (*p == '\0')
			{
				if (argv[i + 1] == NULL) break;
				p = argv[++i];
			}
			freq = atoi(p);
			continue;
		}
		if (strncmp(argv[i],"-t",2) == 0)
		{
			char *p = argv[i] + 2;
			if (*p == '\0')
			{
				if (argv[i + 1] == NULL) break;
				p = argv[++i];
			}
			time = atoi(p);
			continue;
		}
		if (strncmp(argv[i],"-n",2) == 0)
		{
			char *p = argv[i] + 2;
			if (*p == '\0')
			{
				if (argv[i + 1] == NULL) break;
				p = argv[++i];
			}
			songno = atoi(p);
			continue;
		}
		if (strcmp(argv[i],"-s") == 0)
		{
		  if (argv[i + 1] == NULL) break;
		  isSingleMode = 1;
		  continue;
		}
		nsfplay_main(argv[i], freq, time, songno, isSingleMode);
		count++;
		break;
	}
	if (count == 0)
	{
	  fprintf(stderr, "NSF Player version %s - Copyright (C) 2003 T.MITSUYOSHI\n", NSFPLAY_VERSION);
	  fprintf(stderr, "Usage: %s [options]... file\n", argv[0]);
	  fprintf(stderr, "Options:\n");
	  fprintf(stderr, "  -f<frequency>    set output frequency in Hz (default: %d)\n", DEFAULT_FREQ);
	  fprintf(stderr, "  -t<time>         set output time in sec (default: %d)\n", DEFAULT_TIME);
	  fprintf(stderr, "  -n<songno>       set output song no\n");
	  fprintf(stderr, "  -s               play single song and exit\n");
	}
	return 0;
}
