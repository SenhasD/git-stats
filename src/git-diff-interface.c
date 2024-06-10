#include "git-diff-interface.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <obs-internal.h>
#include <obs-module.h>
#include <obs-source.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "./hashMap/include/hashMap.h"
#include "./hashMap/lib/include/untrackedFile.h"
#include "./support.h"

bool checkInsertions(char* input) {
    if (input == NULL) {
        return (false);
    }
    errno = 0;
    char* tmpString = malloc(sizeof(char) * strlen(input) + 1);
    if (errno) {
        perror("git-diff-interface(checkInsertions)");
    }
    strncpy(tmpString, input, strlen(input) + 1);
    if (tmpString == NULL) {
        printf("TMPSTRING WAS NULL IN CHECKINSERTIONS\n");
        return (false);
    }
    char* checker = strtok(tmpString, "insertions");
    if (checker == NULL) {
        tmpString = malloc(sizeof(char) * strlen(input) + 1);
        tmpString[0] = '\0';
        strncpy(tmpString, input, strlen(input) + 1);
        checker = strtok(tmpString, "insertion");
        if (checker == NULL) {
            free(tmpString);
            return (false);
        }
    }
    free(tmpString);
    return (true);
}
bool checkDeletions(char* input) {
    if (input == NULL) {
        return (false);
    }
    errno = 0;
    char* tmpString = malloc(sizeof(char) * strlen(input) + 1);
    if (errno) {
        perror("git-diff-interface(checkDeletions)");
    }
    tmpString[0] = '\0';
    strncpy(tmpString, input, strlen(input) + 1);
    if (tmpString == NULL) {
        printf("TMPSTRING NULL IN CHECKDELETIONS\n");
        return (false);
    }
    char* checker = strtok(tmpString, "deletions");
    if (checker == NULL) {
        tmpString = malloc(sizeof(char) * strlen(input) + 1);
        tmpString[0] = '\0';
        strncpy(tmpString, input, strlen(input) + 1);
        checker = strtok(tmpString, "deletion");
        if (checker == NULL) {
            free(tmpString);
            return (false);
        }
    }
    free(tmpString);
    return (true);
}

void trailingSpaceDestroyer(char* victim) { victim[strlen(victim) - 1] = '\0'; }

void trailingNewlineDestroyer(char* victim) {
    victim[strlen(victim) - 1] = '\0';
}

long getInsertionNumber(char* diffString) {
    if (diffString == NULL) {
        return (0);
    }
    errno = 0;
    char* diffStringCopy = malloc(sizeof(char) * (strlen(diffString) + 1));
    if (errno) {
        perror("git-diff-interface(getInsertionNumber)");
    }
    diffStringCopy[0] = '\0';
    strcpy(diffStringCopy, diffString);
    if (diffStringCopy == NULL) {
        printf("DIFFSTRINGCOPY IS NULL IN GETINSERTIONNUMBER\n");
        return (0);
    }
    char* buff2 = NULL;
    char* startDelim = strstr(diffStringCopy, "insertions");
    if (!startDelim) {
        startDelim = strstr(diffStringCopy, "insertion");
    }
    if (!startDelim) {
        return (0);
    }
    char buffer[strlen(startDelim)];
    for (int i = 0; i < (int)(strlen(diffString) - strlen(startDelim)); i++) {
        buffer[i] = diffString[i];
    }
    char* amt = NULL;
    buff2 = strtok(buffer, ",");
    amt = buff2;
    while ((buff2 = strtok(NULL, ","))) {
        amt = buff2;
    }
    free(diffStringCopy);
    return (atol(amt));
}

long getDeletionNumber(char* diffString) {
    if (diffString == NULL) {
        return (0);
    }
    errno = 0;
    char* diffStringCopy = malloc(sizeof(char) * (strlen(diffString) + 1));
    if (errno) {
        perror("git-diff-interface(getDeletionNumber)");
    }
    diffStringCopy[0] = '\0';
    strcpy(diffStringCopy, diffString);
    if (diffStringCopy == NULL) {
        printf("DIFFSTRINGCOPY IS NULL IN GETDELETIONNUMBER\n");
        return (0);
    }
    char* buff2;
    char* startDelim = strstr(diffStringCopy, "deletions");
    if (!startDelim) {
        startDelim = strstr(diffStringCopy, "deletion");
    }
    if (!startDelim) {
        return (0);
    }
    char buffer[strlen(startDelim)];
    for (int i = 0; i < (int)(strlen(diffString) - strlen(startDelim)); i++) {
        buffer[i] = diffString[i];
    }
    char* amt;
    buff2 = strtok(buffer, ",");
    amt = buff2;
    while ((buff2 = strtok(NULL, ","))) {
        amt = buff2;
    }
    free(diffStringCopy);
    return (atol(amt));
}

void createUntrackedFilesHM(struct gitData* data) {
    FILE* fp;
    char filename[1000];
    char entirePath[1000];

    for (int i = 0; i < data->numTrackedFiles; i++) {
        int commandLength =
            (strlen("/usr/bin/git -C ") + strlen(data->trackedPaths[i]) +
             strlen(" ls-files --others --exclude-standard") + 1);
        errno = 0;
        char* command = malloc(sizeof(char) * commandLength);
        if (errno) {
            printf("MALLOC FAILED IN CREATEUNTRACKED\n");
            continue;
        }
        command[0] = '\0';

        snprintf(
            command, commandLength + 1, "%s %s %s", "/usr/bin/git -C",
            data->trackedPaths[i], "ls-files --others --exclude-standard");

        fp = popen(command, "r");
        if (fp == NULL) {
            continue;
        }

        while (fgets(filename, sizeof(filename), fp)) {
            trailingNewlineDestroyer(filename);

            if (!(data->trackedPaths[i][strlen(data->trackedPaths[i]) - 1] ==
                  '/')) {
                snprintf(
                    entirePath,
                    (strlen(filename) + strlen(data->trackedPaths[i]) + 2),
                    "%s/%s", data->trackedPaths[i], filename);
            }
            else {
                snprintf(
                    entirePath,
                    (strlen(filename) + strlen(data->trackedPaths[i]) + 1),
                    "%s%s", data->trackedPaths[i], filename);
            }
            addElementHM(
                &(data->untracked), entirePath,
                createUntrackedFile(entirePath));
        }

        pclose(fp);
    }
}

char* formatEndPathChar(char* formatee) {
    errno = 0;
    char* output = malloc(sizeof(char) * strlen(formatee) + 3);
    if (errno) {
        perror("git-diff-interface(formatEndPathChar)");
        return (formatee);
    }
    output[0] = '\0';
    snprintf(output, (strlen(formatee) + 2), "%s%c", formatee, '/');
    return (output);
}

void expandHomeDir(char** input) {
    char expanded[100] = "\0";
    strcpy(expanded, getHomePath());
    errno = 0;
    char* inputHold = malloc(sizeof(char) * (strlen((*input)) + 1));
    if (errno) {
        perror("git-diff-interface(expandHomeDir)");
        return;
    }
    inputHold[0] = '\0';
    strcpy(inputHold, ((*input) + 1));
    errno = 0;
    (*input) = malloc(sizeof(char) * (strlen((*input)) + strlen(expanded) + 1));
    if (errno || (*input) == NULL) {
        printf("FAILED EXPANDHOMEDIR\n");
        return;
    }
    (*input)[0] = '\0';
    snprintf(
        (*input), (strlen(inputHold) + strlen(expanded) + 1), "%s%s", expanded,
        inputHold);
}

// check if path is a git dir
bool checkPath(char* path) {
    DIR* dptr;
    char* buffer;

    // check if the directory exists
    errno = 0;
    buffer = malloc(sizeof(char) * (strlen(path) + 8));
    if (errno) {
        perror("git-diff-interface(checkPath)");
        return (false);
    }
    buffer[0] = '\0';

    if (path[strlen(path) - 1] != '/') {
        path = formatEndPathChar(path);
    }
    strcpy(buffer, path);
    snprintf(buffer, (strlen(path) + 7), "%s%s", path, ".git/");
    if (buffer[0] == '~') {
        expandHomeDir(&buffer);
    }
    dptr = opendir(buffer);
    if (errno) {
        closedir(dptr);
        return (false);
    }
    closedir(dptr);
    return (true);
}

// at the current point the pointer in the parameter is changing its address
// before getting to the for loop
void updateTrackedFiles(struct gitData* data) {
    if (data == NULL) {
        printf("Data Struct Is NULL\n");
        return;
    }
    FILE* fp;
    char output[1000];

    long insertions = 0;
    long deletions = 0;
    for (int i = 0; i < data->numTrackedFiles; i++) {
        if (!checkPath(data->trackedPaths[i])) {
            continue;
        }
        int commandLength =
            (strlen("/usr/bin/git -C ") + strlen(data->trackedPaths[i]) +
             strlen(" diff --no-pager --shortstat") + 1);

        errno = 0;
        char* command = malloc(sizeof(char) * commandLength);
        if (errno) {
            perror("git-diff-interface(updateTrackedFiles)");
            continue;
        }
        command[0] = '\0';

        snprintf(
            command, commandLength, "%s %s %s", "/usr/bin/git -C",
            data->trackedPaths[i], "diff --shortstat");

        fp = popen(command, "r");

        if (fp == NULL) {
            continue;
        }
        // assert(fp != NULL);

        if (fgets(output, sizeof(output), fp)) {
            bool insertionExists = checkInsertions(output);
            bool deletionExists = checkDeletions(output);
            if (insertionExists) {
                insertions += getInsertionNumber(output);
            }
            if (deletionExists) {
                deletions += getDeletionNumber(output);
            }
        }
        pclose(fp);
    }
    data->added += insertions;
    data->deleted += deletions;
}

bool checkRepoExists(char** repos, int amtRepos, char* checkPath) {
    for (int i = 0; i < amtRepos; i++) {
        if (!strcmp(repos[i], checkPath)) {
            printf("TEST: %s | %s\n", repos[i], checkPath);
            return (true);
        }
    }
    return (false);
}

void addGitRepoDir(struct gitData* data, char* repoDirPath) {
    /*- grab all of the dir strings within the repo */
    /*- verify that all of these dirs are git repos */
    /*    - verify that the dirs don't already exist*/
    /*    - if is valid git repo add to tracked paths*/
    /*    - else skip this dir*/
    DIR* dptr;
    char* buffer;

    // check if the directory exists
    errno = 0;
    buffer = malloc(sizeof(char) * (strlen(repoDirPath) + 8));
    if (errno) {
        perror("git-diff-interface(addGitRepoDir)");
        return;
    }
    buffer[0] = '\0';

    if (repoDirPath[strlen(repoDirPath) - 1] != '/') {
        repoDirPath = formatEndPathChar(repoDirPath);
    }
    strcpy(buffer, repoDirPath);
    if (buffer[0] == '~') {
        expandHomeDir(&buffer);
    }
    dptr = opendir(buffer);
    if (errno) {
        closedir(dptr);
        perror("Failed To Open Directory\n");
        return;
    }
    closedir(dptr);

    // grab all directories within the repo dir
    DIR* dp = opendir(repoDirPath);
    struct dirent* dirStruct = readdir(dp);

    if (strcmp(dirStruct->d_name, ".") && strcmp(dirStruct->d_name, "..") &&
        (dirStruct != NULL)) {
        char* tmpFilePath = malloc(
            sizeof(char) *
            (strlen(dirStruct->d_name) + strlen(repoDirPath) + 2));
        tmpFilePath[0] = '\0';
        snprintf(
            tmpFilePath, (strlen(repoDirPath) + strlen(dirStruct->d_name) + 1),
            "%s%s", repoDirPath, dirStruct->d_name);
        if (checkPath(tmpFilePath)) {
            if (data->numTrackedFiles == 0) {
                data->trackedPaths = malloc(sizeof(char*) * MAXNUMPATHS);
            }
            if (!checkRepoExists(
                    data->trackedPaths, data->numTrackedFiles, tmpFilePath)) {
                data->trackedPaths[data->numTrackedFiles] =
                    malloc(sizeof(char) * (strlen(tmpFilePath) + 1));
                strncpy(
                    data->trackedPaths[data->numTrackedFiles], tmpFilePath,
                    (strlen(tmpFilePath) + 1));
                data->numTrackedFiles++;
            }
        }
    }
    while ((dirStruct = readdir(dp))) {
        if (strcmp(dirStruct->d_name, ".") && strcmp(dirStruct->d_name, "..")) {
            char* tmpFilePath = malloc(
                sizeof(char) *
                (strlen(dirStruct->d_name) + strlen(repoDirPath) + 2));
            tmpFilePath[0] = '\0';
            snprintf(
                tmpFilePath,
                (strlen(repoDirPath) + strlen(dirStruct->d_name) + 1), "%s%s",
                repoDirPath, dirStruct->d_name);
            if (checkPath(tmpFilePath)) {
                if (!checkRepoExists(
                        data->trackedPaths, data->numTrackedFiles,
                        tmpFilePath)) {
                    data->trackedPaths[data->numTrackedFiles] =
                        malloc(sizeof(char) * (strlen(tmpFilePath) + 1));
                    strncpy(
                        data->trackedPaths[data->numTrackedFiles], tmpFilePath,
                        (strlen(tmpFilePath) + 1));
                    data->numTrackedFiles++;
                }
            }
        }
    }
}
