/* modified  pocketsphinx continuous example
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/time.h>

#include <sphinxbase/err.h>
#include <sphinxbase/ad.h>
#include <sphinxbase/cont_ad.h>
#include <pocketsphinx.h>

static void
sleep_msec(int32 ms)
{
    struct timeval tmo;

    tmo.tv_sec = 0;
    tmo.tv_usec = ms * 1000;
    select(0, NULL, NULL, NULL, &tmo);

}


void send_to_mbed(char const *hyp) { 
	int length = 20;
	char toSend[2];
	if (!strcmp(hyp, "turn left") || !strcmp(hyp, "oh mighty roger turn left") ||
				!strcmp(hyp, "oh mighty roger please turn left") || 
				!strcmp(hyp, "please turn left") || !strcmp(hyp, "kindly turn left")) 
		strcpy(toSend, "l");
	else if (!strcmp(hyp, "turn right")) 
		strcpy(toSend, "r");
	else if (!strcmp(hyp, "stop"))
		strcpy(toSend, "s");
	else if (!strcmp(hyp, "move forward"))
		strcpy(toSend, "m");
	else if (!strcmp(hyp, "go back"))
		strcpy(toSend, "b");
	else if (!strcmp(hyp, "hello roger"))
		strcpy(toSend, "h");

	#ifdef CAPSMODE
	toSend[0] = toupper(toSend[0]);
	#endif

	for (int i = 0; i < length; ++i) {
		printf("%s", toSend);
		sleep_msec(10);
	}
	fflush(stdout);
}


static const arg_t cont_args_def[] = {
    POCKETSPHINX_OPTIONS,
    /* Argument file. */
    { "-argfile",
      ARG_STRING,
      NULL,
      "Argument file giving extra arguments." },
    { "-adcdev", 
      ARG_STRING, 
      NULL, 
      "Name of audio device to use for input." },
    { "-infile", 
      ARG_STRING, 
      NULL, 
      "Audio file to transcribe." },
    { "-time", 
      ARG_BOOLEAN, 
      "no", 
      "Print word times in file transcription." },
    CMDLN_EMPTY_OPTION
};

static ps_decoder_t *ps;
static cmd_ln_t *config;
static FILE* rawfd;

static int32
ad_file_read(ad_rec_t * ad, int16 * buf, int32 max)
{
    size_t nread;
    
    nread = fread(buf, sizeof(int16), max, rawfd);
    
    return (nread > 0 ? nread : -1);
}

static void
print_word_times(int32 start)
{
	ps_seg_t *iter = ps_seg_iter(ps, NULL);
	while (iter != NULL) {
		int32 sf, ef, pprob;
		float conf;
		
		ps_seg_frames (iter, &sf, &ef);
		pprob = ps_seg_prob (iter, NULL, NULL, NULL);
		conf = logmath_exp(ps_get_logmath(ps), pprob);
		printf ("%s %f %f %f\n", ps_seg_word (iter), (sf + start) / 100.0, (ef + start) / 100.0, conf);
		iter = ps_seg_next (iter);
	}
}

/*
 * Main utterance processing loop:
 *     for (;;) {
 * 	   wait for start of next utterance;
 * 	   decode utterance until silence of at least 1 sec observed;
 * 	   print utterance result;
 *     }
 */
static void
recognize_from_microphone()
{
    ad_rec_t *ad;
    int16 adbuf[4096];
    int32 k, ts, rem;
	time_t tlag; // time lag calculate
    char const *hyp;
    char const *uttid;
	int32 confidence;
    cont_ad_t *cont;
    char word[256];

    if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"),
                          (int)cmd_ln_float32_r(config, "-samprate"))) == NULL)
        E_FATAL("Failed to open audio device\n");

    /* Initialize continuous listening module */
    if ((cont = cont_ad_init(ad, ad_read)) == NULL)
        E_FATAL("Failed to initialize voice activity detection\n");
    if (ad_start_rec(ad) < 0)
        E_FATAL("Failed to start recording\n");
    if (cont_ad_calib(cont) < 0)
        E_FATAL("Failed to calibrate voice activity detection\n");

    for (;;) {
        /* Indicate listening for next utterance */
        fprintf(stderr, "READY....\n");
        fflush(stdout);
        fflush(stderr);

        /* Wait data for next utterance */
        while ((k = cont_ad_read(cont, adbuf, 4096)) == 0)
            sleep_msec(100);

        if (k < 0)
            E_FATAL("Failed to read audio\n");

        /*
         * Non-zero amount of data received; start recognition of new utterance.
         * NULL argument to uttproc_begin_utt => automatic generation of utterance-id.
         */
        if (ps_start_utt(ps, NULL) < 0)
            E_FATAL("Failed to start utterance\n");
		fprintf(stderr, "\nstarted ps_process_raw\n");
        ps_process_raw(ps, adbuf, k, FALSE, FALSE);
        fprintf(stderr,"Listening...\n");
        fflush(stdout);

        /* Note timestamp for this first block of data */
        ts = cont->read_ts;
		//tlag = time(NULL); //record time when utterance starts

        /* Decode utterance until end (marked by a "long" silence, >1sec) */
        for (;;) {
            /* Read non-silence audio data, if any, from continuous listening module */
            if ((k = cont_ad_read(cont, adbuf, 4096)) < 0)
                E_FATAL("Failed to read audio\n");
            if (k == 0) {
                /*
                 * No speech data available; check current timestamp with most recent
                 * speech to see if more than 1 sec elapsed.  If so, end of utterance.
                 */
				//if (time(NULL) - tlag > 1) break; // break if 2 seconds passed
				break;
                if ((cont->read_ts - ts) > DEFAULT_SAMPLES_PER_SEC/2)
                    break;
            }
            else {
                /* New speech data received; note current timestamp */
                ts = cont->read_ts;
            }

            /*
             * Decode whatever data was read above.
             */
            rem = ps_process_raw(ps, adbuf, k, FALSE, FALSE);

            /* If no work to be done, sleep a bit */
            if ((rem == 0) && (k == 0))
                sleep_msec(20);
        }

        /*
         * Utterance ended; flush any accumulated, unprocessed A/D data and stop
         * listening until current utterance completely decoded
         */
        ad_stop_rec(ad);
        while (ad_read(ad, adbuf, 4096) >= 0);
        cont_ad_reset(cont);

        //printf("Stopped listening, please wait...\n");
        fflush(stdout);
        /* Finish decoding, obtain and print result */
        ps_end_utt(ps);
        hyp = ps_get_hyp(ps, &confidence, &uttid);
        fprintf(stderr,"%s\n",hyp);
		//fprintf(stderr, "probability :%d\n", ps_get_prob());

		send_to_mbed(hyp);

        fflush(stdout);


        /* Resume A/D recording for next utterance */
        if (ad_start_rec(ad) < 0)
            E_FATAL("Failed to start recording\n");
    }

    cont_ad_close(cont);
    ad_close(ad);
}

static jmp_buf jbuf;
static void
sighandler(int signo)
{
    longjmp(jbuf, 1);
}

int
main(int argc, char *argv[])
{
    char const *cfg;

    if (argc == 2) {
        config = cmd_ln_parse_file_r(NULL, cont_args_def, argv[1], TRUE);
    }
    else {
        config = cmd_ln_parse_r(NULL, cont_args_def, argc, argv, FALSE);
    }
    /* Handle argument file as -argfile. */
    if (config && (cfg = cmd_ln_str_r(config, "-argfile")) != NULL) {
        config = cmd_ln_parse_file_r(config, cont_args_def, cfg, FALSE);
    }
    if (config == NULL)
        return 1;

    ps = ps_init(config);
    if (ps == NULL)
        return 1;

    E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);

    if (setjmp(jbuf) == 0) {
	    recognize_from_microphone();
	}

    ps_free(ps);
    return 0;
}

