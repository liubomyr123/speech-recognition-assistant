
#include <pocketsphinx.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef WIN32
#define popen _popen
#define pclose _pclose
#endif

typedef enum
{
    END_RUNNING,
    NO_SPEECH_FRAMES,
    SPEECH_IS_NULL,
    STILL_SPEAKING,
    ERROR_START_UTTERANCE,
    ERROR_END_UTTERANCE,
} UtteranceStatus;

typedef enum
{
    RUN_SPEECH_RECOGNITION,
    END_PROGRAM,
    WRONG_COMMAND
} ProgramStatus;

static void clean_up();
static int init_main();
static void catch_sig(int signum);
static ProgramStatus ask_user_command();
static FILE *popen_sox(int sample_rate);
static int get_speech_frame(const int16 **speech);
static int handle_result_string(const char *result_string);
static UtteranceStatus process_utterance(const char **result_string, _Bool show_partial_result);

static FILE *sox_audio_stream;
static short *audio_frame_buffer;
static size_t audio_frame_buffer_size;
static ps_config_t *speech_config;
static ps_decoder_t *speech_decoder;
static ps_endpointer_t *speech_endpointer;

int main()
{
    if (init_main() == 0)
    {
        return 1;
    }

    const char *result_string = NULL;
    while (1)
    {
        ProgramStatus status = ask_user_command();
        if (status == END_PROGRAM)
        {
            break;
        }

        if (status == RUN_SPEECH_RECOGNITION)
        {
            while (1)
            {
                UtteranceStatus status = process_utterance(&result_string, 1);
                if (status == NO_SPEECH_FRAMES)
                {
                    break;
                }
                if (status == ERROR_END_UTTERANCE)
                {
                    break;
                }
                if (status == ERROR_START_UTTERANCE)
                {
                    break;
                }
                if (status == STILL_SPEAKING)
                {
                    continue;
                }
                if (status == SPEECH_IS_NULL)
                {
                    continue;
                }

                if (handle_result_string(result_string) == 1)
                {
                    break;
                }
            }
        }
    }

    return 0;
}

static UtteranceStatus process_utterance(const char **result_string, _Bool show_partial_result)
{
    static _Bool waiting_message_shown = 0;
    const int16 *speech;
    int prev_in_speech = ps_endpointer_in_speech(speech_endpointer);

    if (get_speech_frame(&speech) == 0)
    {
        printf("⏳ No more speech frames. Exiting...\n");
        return NO_SPEECH_FRAMES;
    }
    if (speech == NULL)
    {
        if (!waiting_message_shown)
        {
            printf("⏳ Waiting on your speech...\n");
            waiting_message_shown = 1;
        }
        return SPEECH_IS_NULL;
    }

    waiting_message_shown = 0;

    if (prev_in_speech == 0)
    {
        printf("\n");
        printf("⏳ Start utterance processing...\n");
        fprintf(stderr, "⏳ Speech start at %.2f\n", ps_endpointer_speech_start(speech_endpointer));
        if (ps_start_utt(speech_decoder) == 0)
        {
            printf("✅ Successfully started utterance processing.\n");
            printf("\n");
        }
        else
        {
            printf("❌ Failed to start utterance processing.\n");
            return ERROR_START_UTTERANCE;
        }
    }

    int number_of_searched_frames = ps_process_raw(speech_decoder, speech, audio_frame_buffer_size, FALSE, FALSE);
    if (number_of_searched_frames < 0)
    {
        E_FATAL("❌ ps_process_raw() failed\n");
    }

    const char *hypothesis_string = NULL;
    if (show_partial_result)
    {
        hypothesis_string = ps_get_hyp(speech_decoder, NULL);
        if (hypothesis_string != NULL)
        {
            fprintf(stderr, "✅ Partial result: %s\n", hypothesis_string);
        }
    }

    int current_in_speech = ps_endpointer_in_speech(speech_endpointer);
    if (current_in_speech != 0)
    {
        // printf("⏳ You are speaking now...\n");
        return STILL_SPEAKING;
    }

    printf("\n");
    fprintf(stderr, "⏳ Speech end at %.2f\n", ps_endpointer_speech_end(speech_endpointer));
    printf("⏳ End utterance processing...\n");
    if (ps_end_utt(speech_decoder) == 0)
    {
        printf("✅ Successfully finished utterance processing.\n");
        printf("\n");
    }
    else
    {
        printf("❌ Failed to finish utterance processing.\n");
        printf("\n");
        return ERROR_END_UTTERANCE;
    }

    (*result_string) = ps_get_hyp(speech_decoder, NULL);
    return END_RUNNING;
}

static int handle_result_string(const char *result_string)
{
    if (result_string == NULL)
    {
        printf("❌ Result string after utterance is NULL.\n");
    }

    if (strlen(result_string) == 0)
    {
        printf("❌ Result string after utterance is empty.\n");
    }

    printf("✅ Result string: %s\n", result_string);

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

int check_if_directory_is_empty(const char *folder)
{
    DIR *dir_stream = opendir(folder);
    if (dir_stream == NULL)
    {
        perror("❌ Cannot open reading dir stream.\n");
        return -1;
    }
    int is_empty_flag = 1;
    struct dirent *entry = readdir(dir_stream);
    while (entry != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            is_empty_flag = 0;
            break;
        }
        entry = readdir(dir_stream);
    }

    if (closedir(dir_stream) != 0)
    {
        perror("❌ Cannot close reading dir stream.\n");
        return -1;
    }
    return is_empty_flag;
}

int check_if_file_exist(char *name, char *folder)
{
    DIR *dir_stream = opendir(folder);
    if (dir_stream == NULL)
    {
        perror("❌ Cannot open reading dir stream.\n");
        return -1;
    }
    int is_found_flag = 0;
    struct dirent *dp = readdir(dir_stream);
    while (dp != NULL)
    {
        if (strncmp(dp->d_name, name, strlen(name)) == 0)
        {
            is_found_flag = 1; // OK
            break;
        }
        dp = readdir(dir_stream);
    }
    if (closedir(dir_stream) != 0)
    {
        perror("❌ Cannot close reading dir stream.\n");
        return -1;
    }
    return is_found_flag;
}

int check_if_directory_exists_parent(char *parent_dir, char *name)
{
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", parent_dir, name);

    struct stat path_stat;
    if (stat(full_path, &path_stat) != 0)
    {
        return -1;
    }

    if (S_ISDIR(path_stat.st_mode))
    {
        return 1; // OK
    }
    else
    {
        return 0;
    }
}

int check_models_files(int *is_run_default_setup, char *model_name)
{
    int result = 1;
    if (strcmp(model_name, "MODEL_UA") != 0 &&
        strcmp(model_name, "MODEL_DE") != 0 &&
        strcmp(model_name, "MODEL_RU") != 0)
    {
        result = 0;
        return result;
    }

    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "./%s", model_name);

    if (check_if_directory_exists_parent("./", model_name) != 1)
    {
        printf("❌ %s folder was not found.\n", model_name);
        *is_run_default_setup = 1;
        result = 0;
    }
    else
    {
        *is_run_default_setup = 0;
        if (check_if_directory_is_empty(full_path) == 1)
        {
            printf("❌ %s: Folder is empty.\n", model_name);
            *is_run_default_setup = 1;
            result = 0;
            return result;
        }

        if (check_if_directory_exists_parent(full_path, "hmm") != 1)
        {
            printf("❌ %s: ./hmm folder was not found.\n", model_name);
            *is_run_default_setup = 1;
            result = 0;
        }
        else
        {
            char hmm_full_path[1024];
            snprintf(hmm_full_path, sizeof(hmm_full_path), "./%s/hmm", model_name);

            if (check_if_directory_is_empty(hmm_full_path) == 1)
            {
                printf("❌ %s: ./hmm folder is empty.\n", model_name);
                *is_run_default_setup = 1;
                result = 0;
            }
        }
        if (check_if_file_exist("dictionary.dic", full_path) != 1 && check_if_file_exist("dictionary.dic.bin", full_path) != 1)
        {
            printf("❌ %s: Dictionary file was not found.\n", model_name);
            *is_run_default_setup = 1;
            result = 0;
        }

        if (check_if_file_exist("language_model.lm", full_path) != 1 && check_if_file_exist("language_model.lm.bin", full_path) != 1)
        {
            printf("❌ %s: Language model file was not found.\n", model_name);
            *is_run_default_setup = 1;
            result = 0;
        }

        if (*is_run_default_setup == 1)
        {
            printf("❌ %s: Skipping...\n", model_name);
            result = 0;
        }
        else
        {
            printf("⏳ Running %s...\n", model_name);
            result = 1;
        }
    }
    return result;
}

static void load_modals()
{
    if (speech_config == NULL)
    {
        return;
    }

    int is_run_default_setup = 1;

#ifdef MODEL_UA
    printf("\n");
    printf("✅ MODEL_UA flag is defined.\n");
    if (check_models_files(&is_run_default_setup, "MODEL_UA") == 1)
    {
        printf("Loading...\n");
    }
    printf("\n");
#endif

#ifdef MODEL_DE
    printf("\n");
    printf("✅ MODEL_DE flag is defined.\n");
    if (check_models_files(&is_run_default_setup, "MODEL_DE") == 1)
    {
        printf("Loading...\n");
    }
    printf("\n");
#endif

#ifdef MODEL_RU
    printf("\n");
    printf("✅ MODEL_RU flag is defined.\n");
    if (check_models_files(&is_run_default_setup, "MODEL_RU") == 1)
    {
        printf("Loading...\n");
    }
    printf("\n");
#endif

    if (is_run_default_setup == 1)
    {
        printf("   ├─ ⏳ Running default acoustic and language models...\n");
        ps_default_search_args(speech_config);
    }
}

static int init_main()
{
    printf("⏳ Initializing main...\n");
    atexit(clean_up);

    speech_config = ps_config_init(NULL);
    if (speech_config == NULL)
    {
        E_FATAL("❌ PocketSphinx config init failed\n");
    }
    else
    {
        printf("   ├─ ✅ PocketSphinx config successfully initialized.\n");
    }

    load_modals();

    speech_decoder = ps_init(speech_config);
    if (speech_decoder == NULL)
    {
        E_FATAL("❌ PocketSphinx decoder init failed\n");
    }
    else
    {
        printf("   ├─ ✅ PocketSphinx decoder successfully initialized.\n");
    }

    double window = 0;
    double ratio = 0.0;
    ps_vad_mode_t mode = 0;
    int sample_rate = 0;
    double frame_length = 0;

    speech_endpointer = ps_endpointer_init(window, ratio, mode, sample_rate, frame_length);
    if (speech_endpointer == NULL)
    {
        E_FATAL("❌ PocketSphinx endpointer init failed\n");
    }
    else
    {
        printf("   ├─ ✅ PocketSphinx endpointer successfully initialized.\n");
    }

    sox_audio_stream = popen_sox(ps_endpointer_sample_rate(speech_endpointer));
    audio_frame_buffer_size = ps_endpointer_frame_size(speech_endpointer);

    if ((audio_frame_buffer = malloc(audio_frame_buffer_size * sizeof(audio_frame_buffer[0]))) == NULL)
    {
        E_FATAL_SYSTEM("❌ Failed to allocate frame");
    }
    else
    {
        printf("   ├─ ✅ Successfully allocated frame.\n");
    }

    if (signal(SIGINT, catch_sig) == SIG_ERR)
    {
        E_FATAL_SYSTEM("❌ Failed to set SIGINT handler");
    }
    else
    {
        printf("   ├─ ✅ Successfully set SIGINT handler.\n");
    }

    printf("   └─ ✅ Initiation done!\n");
    printf("\n");
    return 1;
}

static int get_speech_frame(const int16 **speech)
{
    size_t end_samples;
    // printf("⏳ Reading speech frame...\n");

    size_t len = fread(audio_frame_buffer, sizeof(audio_frame_buffer[0]), audio_frame_buffer_size, sox_audio_stream);
    // printf("📏 Read %zu samples (expected %zu)\n", len, audio_frame_buffer_size);

    if (len != audio_frame_buffer_size)
    {
        if (len > 0)
        {
            printf("🔚 Stream ending, processing remaining samples...\n");
            (*speech) = ps_endpointer_end_stream(speech_endpointer, audio_frame_buffer, audio_frame_buffer_size, &end_samples);
            // printf("✅ Processed %zu end samples\n", end_samples);
        }
        else
        {
            printf("❌ No more data to read, exiting...\n");
            return 0;
        }
    }
    else
    {
        // printf("✅ Processing normal speech frame...\n");
        (*speech) = ps_endpointer_process(speech_endpointer, audio_frame_buffer);
    }
    return 1;
}

static void clean_up()
{
    printf("\n");
    printf("\n⏳ Cleaning up...\n");
    if (audio_frame_buffer != NULL)
    {
        free(audio_frame_buffer);
        printf("   ├─ ✅ Frame cleaned up\n");
    }
    if (sox_audio_stream != NULL)
    {
        if (pclose(sox_audio_stream) < 0)
        {
            E_ERROR_SYSTEM("❌ Failed to pclose(sox)");
        }
        printf("   ├─ ✅ Sox cleaned up\n");
    }
    if (speech_endpointer != NULL)
    {
        ps_endpointer_free(speech_endpointer);
        printf("   ├─ ✅ Endpointer cleaned up\n");
    }
    if (speech_decoder != NULL)
    {
        ps_free(speech_decoder);
        printf("   ├─ ✅ Decoder cleaned up\n");
    }
    if (speech_config != NULL)
    {
        ps_config_free(speech_config);
        printf("   ├─ ✅ Config cleaned up\n");
    }
    printf("   └─ ✅ Successfully cleaned up!\n");
}

static FILE *popen_sox(int sample_rate)
{
#define SOXCMD "sox -q -r %d -c 1 -b 16 -e signed-integer -d -t raw -"

    int len = snprintf(NULL, 0, SOXCMD, sample_rate);
    if (len < 0)
    {
        E_FATAL_SYSTEM("❌ snprintf() failed to calculate buffer size");
    }
    else
    {
        printf("   ├─ ✅ Successfully calculated buffer size: %d\n", len);
    }

    char *soxcmd = malloc(len + 1);
    if (soxcmd == NULL)
    {
        E_FATAL_SYSTEM("❌ Failed to allocate string");
    }
    else
    {
        printf("   ├─ ✅ Successfully allocated sox command string.\n");
    }

    if (snprintf(soxcmd, len + 1, SOXCMD, sample_rate) != len)
    {
        E_FATAL_SYSTEM("❌ snprintf() failed: unexpected string length\n");
    }
    else
    {
        printf("   ├─ ✅ Successfully formatted soz command: %s\n", soxcmd);
    }

    FILE *sox = popen(soxcmd, "r");
    if (sox == NULL)
    {
        E_FATAL_SYSTEM("❌ Failed to execute sox command: %s", soxcmd);
    }
    else
    {
        printf("   ├─ ✅ Successfully started soz process with command: %s\n", soxcmd);
    }

    free(soxcmd);
    return sox;
}

static ProgramStatus ask_user_command()
{
    ProgramStatus status = WRONG_COMMAND;
    int command = 1;
    int result;
    while (1)
    {
        printf("\n");
        printf("🔚 Select 1 for run recognition and 0 to stop the program: \n");
        result = scanf("%d", &command);

        if (result != 1)
        {
            while (getchar() != '\n')
            {
            };
            printf("❌ Invalid format! Please enter a number.\n");
        }
        else if (command != 0 && command != 1)
        {
            printf("❌ Invalid command! Please enter 1 or 0\n");
        }
        else
        {
            break;
        }
    }

    switch (command)
    {
    case 1:
    {
        printf("✅ Perfect! Running speech recognition...\n");
        status = RUN_SPEECH_RECOGNITION;
        break;
    }
    case 0:
    {
        printf("✅ Perfect! Stopping program...\n");
        status = END_PROGRAM;
        break;
    }
    }

    return status;
}

static void catch_sig(int signum)
{
    (void)signum;
    printf("\n📶 Perfect! Interrupt signal detected.\n");
    printf("✅ Stopping program...\n");
    exit(0);
}
