
#include "./git-stats-source.h"

#include <obs-internal.h>
#include <obs-module.h>
#include <obs-source.h>
#include <stdlib.h>

#include "./git-diff-interface.h"
#include "./hashMap/include/hashMap.h"
#include "hashMap/lib/include/untrackedFile.h"
#include "support.h"

#define OVERLOAD_VAL 9999
#define MAX_OVERLOAD 4
#define DEFAULT_OVERLOAD_CHAR "."

// global for putting source in "max number" mode
static bool testMode = false;

// global for the initial startup
static int INIT_RUN = 1;

static void git_stats_update(void*, obs_data_t*);

static void git_stats_get_defaults(obs_data_t*);

static obs_properties_t* git_stats_properties(void*);
struct gitStatsInfo {
    obs_source_t* insertionSource;
    // pointer to the text source
    // pointer to the deletion text source
    obs_source_t* deletionSource;
    // pointer to our source!!
    obs_source_t* gitSource;
    // length of the text source
    uint32_t cx;
    // height of the text source
    uint32_t cy;
    // time passed between the updates
    float time_passed;
    // the information that we get from our git stats thingy madonker
    struct gitData* data;
};

static const char* git_stats_name(void* unused) {
    UNUSED_PARAMETER(unused);
    return (obs_module_text("Git Stats"));
}

// initializes the source
static void* git_stats_create(obs_data_t* settings, obs_source_t* source) {
    struct gitStatsInfo* info = bzalloc(sizeof(struct gitStatsInfo));
    info->time_passed = 0;
    info->gitSource = source;

    const char* text_source_id = "text_ft2_source_v2";
    info->data = bzalloc(sizeof(struct gitData));
    info->data->trackedPaths = NULL;
    info->data->numTrackedFiles = 0;
    info->data->untracked = NULL;
    info->data->added = 0;
    info->data->deleted = 0;
    info->data->insertionEnabled = false;
    info->data->deletionEnabled = false;
    // malloc 8 bytes for unicode character
    info->data->overloadChar = malloc(16);
    info->data->overloadChar[0] = '\0';
    strncpy(
        info->data->overloadChar, DEFAULT_OVERLOAD_CHAR,
        strlen(DEFAULT_OVERLOAD_CHAR) + 1);

    info->insertionSource =
        obs_source_create(text_source_id, "insertionSource", settings, NULL);
    obs_source_add_active_child(info->gitSource, info->insertionSource);
    info->deletionSource =
        obs_source_create(text_source_id, "deletionSource", NULL, NULL);
    obs_source_add_active_child(info->gitSource, info->deletionSource);

    // ensure that defaults are set AFTER the creation has completed
    git_stats_get_defaults(settings);
    git_stats_update(info, settings);

    obs_log(LOG_INFO, "Source Initialized");

    return (info);
}

// free up the source and its data
static void git_stats_destroy(void* data) {
    struct gitStatsInfo* info = data;

    obs_source_remove_active_child(info->gitSource, info->insertionSource);
    obs_source_remove_active_child(info->gitSource, info->deletionSource);

    obs_source_remove(info->insertionSource);
    obs_source_release(info->insertionSource);
    info->insertionSource = NULL;

    obs_source_remove(info->deletionSource);
    obs_source_release(info->deletionSource);
    info->deletionSource = NULL;

    free(info->data->overloadChar);
    info->data->overloadChar = NULL;

    for (int i = 0; i < info->data->numTrackedFiles; i++) {
        free(info->data->trackedPaths[i]);
    }

    if (info->data->untracked) {
        freeHM(&(info->data->untracked));
    }

    bfree(info->data);
    info->data = NULL;

    bfree(info);
    info = NULL;

    obs_log(LOG_INFO, "Source Destroyed");
}

// get the width needed for the source
static uint32_t git_stats_width(void* data) {
    struct gitStatsInfo* info = data;

    return (obs_source_get_width(info->deletionSource));
}

// get the height needed for the source
static uint32_t git_stats_height(void* data) {
    struct gitStatsInfo* info = data;
    if (info->data->insertionEnabled) {
        return (obs_source_get_height(info->insertionSource));
    }
    return (obs_source_get_height(info->deletionSource));
}

// Settings that are loaded when the default button is hit in the properties
// window
static void git_stats_get_defaults(obs_data_t* settings) {
    // repo settings
    obs_data_set_default_int(settings, "delay", 5);
    obs_data_set_default_string(settings, "overload_char", ".");
    obs_data_set_default_bool(settings, "untracked_files", false);

    // shared settings
    obs_data_set_default_bool(settings, "antialiasing", true);
    obs_data_set_default_bool(settings, "outline", false);
    obs_data_set_default_bool(settings, "drop_shadow", false);

    // deletion opts
    obs_data_set_default_int(settings, "deletion_color1", 0xFF0000FF);
    obs_data_set_default_int(settings, "deletion_color2", 0xFF0000FF);
    obs_data_set_default_bool(settings, "deletion_symbol", true);

    // insertion opts
    obs_data_set_default_int(settings, "color1", 0xFF00FF00);
    obs_data_set_default_int(settings, "color2", 0xFF00FF00);
    obs_data_set_default_bool(settings, "insertion_symbol", true);

    // make DejaVu Sans Mono the default because sans serif is not mono and
    // doesn't require nerd fonts installed
    obs_data_t* font_obj = obs_data_create();
    obs_data_set_default_string(font_obj, "face", "DejaVu Sans Mono");
    obs_data_set_default_int(font_obj, "size", 256);
    obs_data_set_default_int(font_obj, "flags", 0);
    obs_data_set_default_string(font_obj, "style", "");
    obs_data_set_default_obj(settings, "font", font_obj);

    // group settings
    obs_data_set_default_bool(settings, "insertion_properties", true);
    obs_data_set_default_bool(settings, "deletion_properties", true);
}

// update the settings data in our source
static void git_stats_update(void* data, obs_data_t* settings) {
    struct gitStatsInfo* info = data;
    UNUSED_PARAMETER(data);

    // copy settings from dummy property to the deletion text source
    obs_data_t* insertionFont = obs_data_get_obj(settings, "font");
    obs_data_set_obj(
        info->deletionSource->context.settings, "font", insertionFont);
    obs_data_release(insertionFont);
    obs_data_set_bool(
        info->deletionSource->context.settings, "antialiasing",
        obs_data_get_bool(settings, "antialiasing"));
    obs_data_set_int(
        info->deletionSource->context.settings, "color1",
        obs_data_get_int(settings, "deletion_color1"));
    obs_data_set_int(
        info->deletionSource->context.settings, "color2",
        obs_data_get_int(settings, "deletion_color2"));
    obs_data_set_bool(
        info->deletionSource->context.settings, "outline",
        obs_data_get_bool(settings, "outline"));
    obs_data_set_bool(
        info->deletionSource->context.settings, "drop_shadow",
        obs_data_get_bool(settings, "drop_shadow"));

    if (strcmp(obs_data_get_string(settings, "overload_char"), "") &&
        obs_data_get_string(settings, "overload_char")) {
        char* unicode =
            extractUnicode(obs_data_get_string(settings, "overload_char"));
        if (unicode) {
            strcpy(info->data->overloadChar, unicode);
            free(unicode);
        }
        else {
            strncpy(
                info->data->overloadChar, DEFAULT_OVERLOAD_CHAR,
                strlen(DEFAULT_OVERLOAD_CHAR) + 1);
        }
    }
    else {
        strncpy(
            info->data->overloadChar, DEFAULT_OVERLOAD_CHAR,
            strlen(DEFAULT_OVERLOAD_CHAR) + 1);
    }

    if (!obs_data_get_bool(settings, "insertion_properties")) {
        info->data->insertionEnabled = false;
    }
    else {
        info->data->insertionEnabled = true;
    }

    if (!obs_data_get_bool(settings, "deletion_properties")) {
        info->data->deletionEnabled = false;
    }
    else {
        info->data->deletionEnabled = true;
    }

    obs_data_array_t* dirArray =
        obs_data_get_array(info->gitSource->context.settings, "single_repos");

    if (!obs_data_array_count(dirArray) &&
        (obs_data_get_string(settings, "repositories_directory") == NULL ||
         !strcmp(
             obs_data_get_string(settings, "repositories_directory"), ""))) {
        obs_data_set_string(
            info->insertionSource->context.settings, "text", "\n+0");
        obs_data_set_string(
            info->deletionSource->context.settings, "text", "\n   -0");
        obs_source_update(
            info->insertionSource, info->insertionSource->context.settings);
        obs_source_update(
            info->deletionSource, info->deletionSource->context.settings);
        info->data->trackedPaths = NULL;
        info->data->numTrackedFiles = 0;
    }
    else {
        if (info->data->trackedPaths) {
            for (int i = 0; i < info->data->numTrackedFiles; i++) {
                free(info->data->trackedPaths[i]);
                info->data->trackedPaths[i] = NULL;
            }
        }
        else {
            info->data->trackedPaths = malloc(sizeof(char*) * MAXNUMPATHS);
        }
        info->data->numTrackedFiles = 0;
        for (size_t i = 0; i < obs_data_array_count(dirArray); i++) {
            obs_data_t* currItem = obs_data_array_item(dirArray, i);
            const char* currVal = obs_data_get_string(currItem, "value");
            errno = 0;
            info->data->trackedPaths[i] =
                malloc(sizeof(char) * strlen(currVal) + 1);
            if (errno) {
                obs_log(
                    LOG_ERROR, "Singular Update Failed: %s", strerror(errno));
                info->data->trackedPaths[i] = NULL;
                info->data->numTrackedFiles++;
                continue;
            }
            strncpy(info->data->trackedPaths[i], currVal, strlen(currVal) + 1);
            info->data->numTrackedFiles++;
            obs_data_release(currItem);
        }
    }
    obs_data_array_release(dirArray);
    if (strcmp(obs_data_get_string(settings, "repositories_directory"), "") &&
        (obs_data_get_string(settings, "repositories_directory") != NULL)) {
        addGitRepoDir(
            info->data,
            (char*)obs_data_get_string(settings, "repositories_directory"));
    }

    info->data->delayAmount = obs_data_get_int(settings, "delay");
    info->data->added = 0;
    info->data->deleted = 0;

    if (obs_data_get_bool(settings, "untracked_files")) {
        info->data->untracked = createHashMap();
        createUntrackedFilesHM(info->data);
    }

    if (!obs_data_get_bool(settings, "untracked_files") &&
        (info->data->untracked != NULL)) {
        info->data->untracked = createHashMap();
    }
}

// render out the source
static void git_stats_render(void* data, gs_effect_t* effect) {
    struct gitStatsInfo* info = data;
    obs_source_video_render(info->insertionSource);
    obs_source_video_render(info->deletionSource);
    UNUSED_PARAMETER(effect);
}

// update relevant real time data for the source (called each frame with the
// time elapsed passed in)
static void git_stats_tick(void* data, float seconds) {
    struct gitStatsInfo* info = data;
    if (!obs_source_showing(info->gitSource)) {
        return;
    }

    info->time_passed += seconds;
    if (info->time_passed > info->data->delayAmount || INIT_RUN) {
        INIT_RUN &= 0;
        info->time_passed = 0;
        if (testMode) {
            int numOverload = MAX_OVERLOAD;
            errno = 0;
            char* overloadString =
                malloc(sizeof(char) * (MB_CUR_MAX * numOverload));
            if (errno) {
                obs_log(LOG_ERROR, "Malloc Failed");
                return;
            }
            overloadString[0] = ' ';
            overloadString[1] = '\0';
            for (volatile int i = 1; i < numOverload + 1; i++) {
                strcat(overloadString, info->data->overloadChar);
            }
            char buffer[30] = "";
            char* overloadValueString = ltoa(OVERLOAD_VAL);
            snprintf(
                buffer,
                strlen(overloadValueString) + strlen(overloadString) + 3,
                "%s\n+%s", overloadString, overloadValueString);
            obs_data_set_string(
                info->insertionSource->context.settings, "text", buffer);
            obs_source_update(
                info->insertionSource, info->insertionSource->context.settings);

            char spaces[7] = "";
            int deletionSize = strlen(overloadValueString) + 2;
            for (int i = 0; i < deletionSize; i++) {
                spaces[i] = ' ';
                spaces[i + 1] = '\0';
            }
            snprintf(
                buffer,
                strlen(overloadString) + (strlen(spaces) * 2) +
                    strlen(overloadValueString) + 3,
                "%s%s\n%s-%s", spaces, overloadString, spaces,
                overloadValueString);
            obs_data_set_string(
                info->deletionSource->context.settings, "text", buffer);
            obs_source_update(
                info->deletionSource, info->deletionSource->context.settings);
            free(overloadValueString);
            free(overloadString);
            return;
        }
        if (info->data->trackedPaths == NULL) {
            if (info->data->insertionEnabled) {
                if (obs_data_get_bool(
                        info->gitSource->context.settings,
                        "insertion_symbol")) {
                    obs_data_set_string(
                        info->insertionSource->context.settings, "text",
                        "\n+0");
                    obs_source_update(
                        info->insertionSource,
                        info->insertionSource->context.settings);
                }
                else {
                    obs_data_set_string(
                        info->insertionSource->context.settings, "text",
                        "\n 0");
                    obs_source_update(
                        info->insertionSource,
                        info->insertionSource->context.settings);
                }
            }
            else {
                obs_data_set_string(
                    info->insertionSource->context.settings, "text", " ");
                obs_source_update(
                    info->insertionSource,
                    info->insertionSource->context.settings);
            }
            if (info->data->deletionEnabled) {
                if (obs_data_get_bool(
                        info->gitSource->context.settings, "deletion_symbol")) {
                    obs_data_set_string(
                        info->deletionSource->context.settings, "text",
                        "\n   -0");
                    obs_source_update(
                        info->deletionSource,
                        info->deletionSource->context.settings);
                }
                else {
                    obs_data_set_string(
                        info->deletionSource->context.settings, "text",
                        "\n    0");
                    obs_source_update(
                        info->deletionSource,
                        info->deletionSource->context.settings);
                }
            }
            else {
                obs_data_set_string(
                    info->deletionSource->context.settings, "text", "     ");
                obs_source_update(
                    info->deletionSource,
                    info->deletionSource->context.settings);
            }
            return;
        }
        else {
            info->data->deleted = 0;
            info->data->added = 0;
            updateTrackedFiles(info->data);
        }
        if (info->data->untracked != NULL) {
            updateValueHM(&(info->data->untracked));
            info->data->added += getLinesAddedHM(&(info->data->untracked));
        }
        if (info->data->insertionEnabled) {
            long value = info->data->added;
            int numOverload = value / OVERLOAD_VAL;
            value = value % OVERLOAD_VAL;
            numOverload > MAX_OVERLOAD ? numOverload = MAX_OVERLOAD
                                       : numOverload;
            char* overloadString = NULL;
            if (!numOverload) {
                errno = 0;
                overloadString = malloc(sizeof(char) * 2);

                if (errno) {
                    obs_log(LOG_ERROR, "Malloc Failed");
                    return;
                }
                overloadString[0] = '\0';
            }
            else {
                errno = 0;
                overloadString = malloc(
                    sizeof(char) *
                    (strlen(info->data->overloadChar) * numOverload));
                if (errno) {
                    obs_log(LOG_ERROR, "Malloc Failed");
                    return;
                }
                overloadString[1] = '\0';
                overloadString[0] = ' ';
            }
            for (volatile int i = 1; i < numOverload + 1; i++) {
                strcat(overloadString, info->data->overloadChar);
            }

            char outputBuffer[100] = "\0";
            char* valueString = ltoa(value);
            if (obs_data_get_bool(
                    info->gitSource->context.settings, "insertion_symbol")) {
                snprintf(
                    outputBuffer,
                    strlen(overloadString) + strlen(valueString) + 3, "%s\n+%s",
                    overloadString, valueString);
                obs_data_set_string(
                    info->insertionSource->context.settings, "text",
                    outputBuffer);
                obs_source_update(
                    info->insertionSource,
                    info->insertionSource->context.settings);
            }
            else {
                snprintf(
                    outputBuffer,
                    strlen(overloadString) + strlen(valueString) + 3, "%s\n %s",
                    overloadString, valueString);
                obs_data_set_string(
                    info->insertionSource->context.settings, "text",
                    outputBuffer);
                obs_source_update(
                    info->insertionSource,
                    info->insertionSource->context.settings);
            }
            free(valueString);
            free(overloadString);
        }
        else {
            char outputBuffer[100] = "\0";
            snprintf(outputBuffer, strlen("") + 1, "%s", "");
            obs_data_set_string(
                info->insertionSource->context.settings, "text", outputBuffer);
            obs_source_update(
                info->insertionSource, info->insertionSource->context.settings);
        }
        char outputBuffer[100] = "\0";
        if (info->data->deletionEnabled) {
            long insertionValue = info->data->added % OVERLOAD_VAL;
            long deletionValue = info->data->deleted;
            int numOverload = deletionValue / OVERLOAD_VAL;
            numOverload > MAX_OVERLOAD ? numOverload = MAX_OVERLOAD
                                       : numOverload;
            deletionValue = deletionValue % OVERLOAD_VAL;
            char* overloadString = NULL;
            if (!numOverload) {
                errno = 0;
                overloadString = malloc(sizeof(char) * 2);
                if (errno) {
                    obs_log(LOG_ERROR, "Malloc Failed");
                    return;
                }

                overloadString[0] = '\0';
            }
            else {
                errno = 0;
                overloadString = malloc(
                    sizeof(char) *
                    (strlen(info->data->overloadChar) * numOverload));
                if (errno) {
                    obs_log(LOG_ERROR, "Malloc Failed");
                    return;
                }
                overloadString[1] = '\0';
                overloadString[0] = ' ';
            }
            for (int i = 1; i < numOverload + 1; i++) {
                strcat(overloadString, info->data->overloadChar);
            }
            char spaces[7] = "";
            char* insertionValueString = ltoa(insertionValue);
            int deletionSize = strlen(insertionValueString) + 2;
            free(insertionValueString);
            for (int i = 0; i < deletionSize; i++) {
                spaces[i] = ' ';
                spaces[i + 1] = '\0';
            }
            char* deletionValueString = ltoa(deletionValue);
            if (obs_data_get_bool(
                    info->gitSource->context.settings, "deletion_symbol")) {
                snprintf(
                    outputBuffer,
                    strlen(overloadString) + (strlen(spaces) * 2) +
                        strlen(deletionValueString) + 3,
                    "%s%s\n%s-%s", spaces, overloadString, spaces,
                    deletionValueString);
                obs_data_set_string(
                    info->deletionSource->context.settings, "text",
                    outputBuffer);
                obs_source_update(
                    info->deletionSource,
                    info->deletionSource->context.settings);
            }
            else {
                snprintf(
                    outputBuffer,
                    strlen(overloadString) + (strlen(spaces) * 2) +
                        strlen(deletionValueString) + 3,
                    "%s%s\n%s %s", spaces, overloadString, spaces,
                    deletionValueString);
                obs_data_set_string(
                    info->deletionSource->context.settings, "text",
                    outputBuffer);
                obs_source_update(
                    info->deletionSource,
                    info->deletionSource->context.settings);
            }
            free(deletionValueString);
            free(overloadString);
        }
        else {
            char spaces[7] = "";
            int deletionSize = strlen(ltoa(info->data->added)) + 2;
            for (int i = 0; i < deletionSize; i++) {
                spaces[i] = ' ';
                spaces[i + 1] = '\0';
            }
            snprintf(
                outputBuffer, strlen(spaces) + strlen(" ") + 1, "%s%s", spaces,
                " ");
            obs_data_set_string(
                info->deletionSource->context.settings, "text", outputBuffer);
            obs_source_update(
                info->deletionSource, info->deletionSource->context.settings);
        }
    }
}

// callback for the test_button property
static bool toggleTestCallback(
    obs_properties_t* properties, obs_property_t* buttonProps, void* data) {
    UNUSED_PARAMETER(properties);
    UNUSED_PARAMETER(buttonProps);
    UNUSED_PARAMETER(data);
    testMode ^= 1;
    return (true);
}

// properties that are generated in the ui of the source
static obs_properties_t* git_stats_properties(void* unused) {
    struct gitStatsInfo* info = unused;
    UNUSED_PARAMETER(unused);
    obs_properties_t* props = obs_properties_create();

    obs_properties_t* repo_props = obs_properties_create();

    obs_properties_add_path(
        repo_props, "repositories_directory", "Directory Holding Repositories",
        OBS_PATH_DIRECTORY, NULL, NULL);

    obs_properties_add_editable_list(
        repo_props, "single_repos", "Single Repositories",
        OBS_EDITABLE_LIST_TYPE_FILES, NULL, NULL);

    obs_properties_add_int(
        repo_props, "delay", "Delay Between Updates", 0, INT_MAX, 1);

    obs_properties_add_text(
        repo_props, "overload_char", "Character Shown For Overload",
        OBS_TEXT_DEFAULT);

    obs_properties_add_bool(
        repo_props, "untracked_files", "Account For Untracked Files");

    obs_properties_add_button(
        repo_props, "test_button", "Test Max Size", toggleTestCallback);

    obs_properties_add_group(
        props, "repo_properties", "Repository Settings", OBS_GROUP_NORMAL,
        repo_props);

    ///////////////

    obs_properties_t* shared_props =
        obs_source_properties(info->insertionSource);
    obs_properties_remove_by_name(shared_props, "text_file");
    obs_properties_remove_by_name(shared_props, "from_file");
    obs_properties_remove_by_name(shared_props, "log_mode");
    obs_properties_remove_by_name(shared_props, "log_lines");
    obs_properties_remove_by_name(shared_props, "word_wrap");
    obs_properties_remove_by_name(shared_props, "text");
    obs_properties_remove_by_name(shared_props, "custom_width");
    obs_properties_remove_by_name(shared_props, "color1");
    obs_properties_remove_by_name(shared_props, "color2");
    obs_properties_add_group(
        props, "shared_properties", "Shared Settings", OBS_GROUP_NORMAL,
        shared_props);

    ///////////////

    obs_properties_t* text1_props =
        obs_source_properties(info->insertionSource);
    obs_properties_remove_by_name(text1_props, "font");
    obs_properties_remove_by_name(text1_props, "text_file");
    obs_properties_remove_by_name(text1_props, "from_file");
    obs_properties_remove_by_name(text1_props, "log_mode");
    obs_properties_remove_by_name(text1_props, "log_lines");
    obs_properties_remove_by_name(text1_props, "word_wrap");
    obs_properties_remove_by_name(text1_props, "text");
    obs_properties_remove_by_name(text1_props, "custom_width");
    obs_properties_remove_by_name(text1_props, "drop_shadow");
    obs_properties_remove_by_name(text1_props, "outline");
    obs_properties_remove_by_name(text1_props, "antialiasing");
    obs_properties_add_bool(text1_props, "insertion_symbol", "+ Symbol");
    obs_data_set_default_int(
        info->insertionSource->context.settings, "color1", 0xFF00FF00);
    obs_data_set_default_int(
        info->insertionSource->context.settings, "color1", 0xFF00FF00);
    obs_properties_add_group(
        props, "insertion_properties", "Insertion Settings",
        OBS_GROUP_CHECKABLE, text1_props);

    //////////////

    obs_properties_t* text2_props = obs_properties_create();
    obs_properties_add_color_alpha(text2_props, "deletion_color1", "Color1");
    obs_properties_add_color_alpha(text2_props, "deletion_color2", "Color2");
    obs_properties_add_bool(text2_props, "deletion_symbol", "- Symbol");

    obs_properties_add_group(
        props, "deletion_properties", "Deletion Settings", OBS_GROUP_CHECKABLE,
        text2_props);

    return props;
}

// clang-format off
    struct obs_source_info git_stats_source = {
        .id           = "git-stats",
        .type         = OBS_SOURCE_TYPE_INPUT,
        .output_flags = OBS_SOURCE_VIDEO,
        .get_name     = git_stats_name,
        .create       = git_stats_create,
        .destroy      = git_stats_destroy,
        .update       = git_stats_update,
        .video_render = git_stats_render,
        .get_width    = git_stats_width,
        .get_height   = git_stats_height,
        .video_tick = git_stats_tick, 
        .get_properties = git_stats_properties, 
        .icon_type = OBS_ICON_TYPE_TEXT, 
    };
// clang-format on
