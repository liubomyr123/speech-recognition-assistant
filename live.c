
#include <pocketsphinx.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

static int global_done = 0;
static void catch_sig(int signum)
{
    (void)signum;
    global_done = 1;
}

#ifdef WIN32
#define popen _popen
#define pclose _pclose
#endif

static FILE *popen_sox(int sample_rate);
static void clean_up();
static int init_main();
int get_speech_frame(const int16 **speech);
int handle_result_string(const char *result_string);

static ps_decoder_t *decoder;
static ps_config_t *config;
static ps_endpointer_t *endpointer;
static FILE *sox;
static short *frame;
static size_t frame_size;

int main()
{
    if (init_main() == 0)
    {
        return 1;
    }

    while (!global_done)
    {
        const int16 *speech;
        int prev_in_speech = ps_endpointer_in_speech(endpointer);
        
        if (get_speech_frame(&speech) == 0)
        {
            break;
        }
        if (speech == NULL)
        {
            printf("⏳ Waiting on your speech...\n");
            continue;
        }

        if (prev_in_speech == 0)
        {
            fprintf(stderr, "⏳ Speech start at %.2f\n", ps_endpointer_speech_start(endpointer));
            printf("✅ Start utterance processing...\n");
            ps_start_utt(decoder);
        }

        int number_of_searched_frames = ps_process_raw(decoder, speech, frame_size, FALSE, FALSE);
        if (number_of_searched_frames < 0)
        {
            E_FATAL("❌ ps_process_raw() failed\n");
        }

        const char *hypothesis_string = ps_get_hyp(decoder, NULL);
        if (hypothesis_string != NULL)
        {
            fprintf(stderr, "✅ Partial result: %s\n", hypothesis_string);
        }

        int current_in_speech = ps_endpointer_in_speech(endpointer);
        if (current_in_speech != 0)
        {
            printf("❌ You are speaking now...\n");
            continue;
        }

        fprintf(stderr, "⏳ Speech end at %.2f\n", ps_endpointer_speech_end(endpointer));
        printf("✅ End utterance processing...\n");
        ps_end_utt(decoder);

        const char *result_string = ps_get_hyp(decoder, NULL);
        if (result_string == NULL)
        {
            printf("❌ Result string after utterance is NULL.\n");
            continue;
        }

        if (strlen(result_string) == 0)
        {
            printf("❌ Result string after utterance is empty.\n");
            continue;
        }

        printf("✅ Result string: %s\n", result_string);

        if (handle_result_string(result_string) == 0)
        {
            continue;
        }
    }

    clean_up();
    return 0;
}

int handle_result_string(const char *result_string)
{
    if (strcmp(result_string, "browser") == 0)
    {
        printf("browser command\n");

        if (fork() == 0)
        {
            execlp("firefox", "firefox", "https://www.google.com", NULL);
        }
    }
    if (strcmp(result_string, "browser exit") == 0)
    {
        printf("browser exit command\n");
        system("pkill firefox");
    }
    return 1;
}

int init_main()
{
    printf("⏳ Initializing main...\n");
    config = ps_config_init(NULL);
    ps_default_search_args(config);
    if ((decoder = ps_init(config)) == NULL)
    {
        E_FATAL("PocketSphinx decoder init failed\n");
    }

    double window = 0;
    double ratio = 0.0;
    ps_vad_mode_t mode = 0;
    int sample_rate = 0;
    double frame_length = 0;
    endpointer = ps_endpointer_init(window, ratio, mode, sample_rate, frame_length);
    if (endpointer == NULL)
    {
        E_FATAL("❌ PocketSphinx endpointer init failed\n");
    }

    sox = popen_sox(ps_endpointer_sample_rate(endpointer));
    frame_size = ps_endpointer_frame_size(endpointer);

    if ((frame = malloc(frame_size * sizeof(frame[0]))) == NULL)
    {
        E_FATAL_SYSTEM("❌ Failed to allocate frame");
    }

    if (signal(SIGINT, catch_sig) == SIG_ERR)
    {
        E_FATAL_SYSTEM("❌ Failed to set SIGINT handler");
    }
    printf("✅ Initiation done!\n");
    printf("\n");
    return 1;
}

int get_speech_frame(const int16 **speech)
{
    size_t len, end_samples;
    if ((len = fread(frame, sizeof(frame[0]), frame_size, sox)) != frame_size)
    {
        if (len > 0)
        {
            (*speech) = ps_endpointer_end_stream(endpointer, frame, frame_size, &end_samples);
        }
        else
        {
            return 0;
        }
    }
    else
    {
        (*speech) = ps_endpointer_process(endpointer, frame);
    }
    return 1;
}

void clean_up()
{
    printf("\n");
    printf("\n⏳ Cleaning up...\n");
    free(frame);
    if (pclose(sox) < 0)
    {
        E_ERROR_SYSTEM("❌ Failed to pclose(sox)");
    }
    ps_endpointer_free(endpointer);
    ps_free(decoder);
    ps_config_free(config);
    printf("✅ Successfully cleaned up!\n");
}

static FILE *popen_sox(int sample_rate)
{
#define SOXCMD "sox -q -r %d -c 1 -b 16 -e signed-integer -d -t raw -"

    int len = snprintf(NULL, 0, SOXCMD, sample_rate);
    char *soxcmd = malloc(len + 1);
    if (soxcmd == NULL)
    {
        E_FATAL_SYSTEM("❌ Failed to allocate string");
    }

    if (snprintf(soxcmd, len + 1, SOXCMD, sample_rate) != len)
    {
        E_FATAL_SYSTEM("❌ snprintf() failed");
    }

    FILE *sox = popen(soxcmd, "r");
    if (sox == NULL)
    {
        E_FATAL_SYSTEM("❌ Failed to popen(%s)", soxcmd);
    }

    free(soxcmd);
    return sox;
}
