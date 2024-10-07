#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "emoji_utils.h"
#include "emoji_extensions.h"

#define MAX_PATH 4096

// Define a structure to map keys (file names or extensions) to emojis
//typedef struct {
//    const char *key;
//    const char *emoji;
//} EmojiMapEntry;

/**
 * Safely duplicates a string, handling memory allocation errors.
 *
 * @param str The string to duplicate.
 * @return A newly allocated copy of the input string.
 */
static char *safe_strdup(const char *str) {
    char *dup = strdup(str);
    if (!dup) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    return dup;
}

/**
 * Determines the appropriate emoji for files in the /dev directory.
 *
 * This function uses two lookup tables:
 * 1. exact_emoji_map for exact matches
 * 2. prefix_emoji_map for prefix matches
 *
 * @param path The name of the file in the /dev directory.
 * @return A dynamically allocated string containing the emoji.
 */
char *get_dev_emoji(const char *path) {
    // Define a lookup table for exact matches
    static const EmojiMapEntry exact_emoji_map[] = {
        {"loop", "🔁"},      {"null", "🕳️"},        {"zero", "🕳️"},
        {"random", "🎲"},    {"urandom", "🎲"},     {"tty", "🖥️"},
        {"usb", "🔌"},       {"vga_arbiter", "🖼️"}, {"vhci", "🔌"},
        {"vhost-net", "🌐"}, {"vhost-vsock", "💬"}, {"mcelog", "📋"},
        {"media0", "🎬"},    {"mei0", "🧠"},        {"mem", "🗄️"},
        {"hpet", "⏱️"},       {"hwrng", "🎲"},       {"kmsg", "📜"},
        {"kvm", "🌰"},       {"zram", "🗜️"},        {"udmabuf", "🔄"},
        {"uhid", "🕹️"},      {"rfkill", "📡"},      {"ppp", "🌐"},
        {"ptmx", "🖥️"},      {"userfaultfd", "🚧"}, {"nvram", "🗄️"},
        {"port", "🔌"},      {"autofs", "🚗"},      {"btrfs-control", "🌳"},
        {"console", "🖥️"},   {"full", "🔒"},        {"fuse", "🔥"},
        {"gpiochip0", "📌"}, {"cuse", "🧩"},        {"cpu_dma_latency", "⏱️"}};

    // Check for exact matches
    for (size_t i = 0; i < sizeof(exact_emoji_map) / sizeof(exact_emoji_map[0]); i++) {
        if (strcmp(path, exact_emoji_map[i].key) == 0) {
            return safe_strdup(exact_emoji_map[i].emoji);
        }
    }

    // Define a lookup table for prefix matches
    static const EmojiMapEntry prefix_emoji_map[] = {
        {"loop", "🔁"}, {"sd", "💽"},  {"tty", "🖥️"},      {"usb", "🔌"}, {"video", "🎥"},
        {"nvme", "💽"}, {"lp", "🖨️"},  {"hidraw", "🔠"},   {"vcs", "📟"}, {"vcsa", "📟"},
        {"ptp", "🕰️"},  {"rtc", "🕰️"}, {"watchdog", "🐕"}, {"mtd", "⚡"}};

    // Check for prefix matches
    for (size_t i = 0; i < sizeof(prefix_emoji_map) / sizeof(prefix_emoji_map[0]); i++) {
        if (strncmp(path, prefix_emoji_map[i].key, strlen(prefix_emoji_map[i].key)) == 0) {
            return safe_strdup(prefix_emoji_map[i].emoji);
        }
    }

    // Default emoji for unmatched /dev files
    return safe_strdup("🔧");
}

static char* check_file_content(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) return NULL;

    char buffer[256];
    if (fgets(buffer, sizeof(buffer), file) != NULL) {
        for (size_t i = 0; i < content_map_size; i++) {
            if (strstr(buffer, content_map[i].key) != NULL) {
                fclose(file);
                return safe_strdup(content_map[i].emoji);
            }
        }
    }

    fclose(file);
    return NULL;
}


/**
 * Determines the appropriate emoji for a given file based on its characteristics.
 *
 * This function checks the file type, special cases, and file extensions
 * to assign the most appropriate emoji.
 *
 * @param path The path to the file.
 * @return A dynamically allocated string containing the emoji.
 */
char *get_emoji(const char *path) {
    struct stat path_stat;

    // Check if we can get file information
    if (lstat(path, &path_stat) != 0) {
        return safe_strdup("❓");  // Unknown file type
    }

    // Check for symbolic links
    if (S_ISLNK(path_stat.st_mode)) {
        return safe_strdup(S_ISDIR(path_stat.st_mode) ? "🔗📁" : "🔗");
    }

    // Check for directories
    if (S_ISDIR(path_stat.st_mode)) {
        return safe_strdup("📁");
    }

    // Extract the filename from the path
    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;


    char *content_emoji = check_file_content(path);
    if (content_emoji) {
      return content_emoji;
    }

    for (size_t i = 0; i < special_case_map_size; i++) {
        if (strstr(filename, special_case_map[i].key) == filename) {
            return safe_strdup(special_case_map[i].emoji);
        }
    }

    // Check file extensions
    char *extension = strrchr(path, '.');
    if (extension) {
        extension++;  // Skip the dot

        for (size_t i = 0; i < ext_map_size; i++) {
            if (strcasecmp(extension, ext_map[i].key) == 0) {
                return safe_strdup(ext_map[i].emoji);
            }
        }
    }

    // Check for hidden files
    if (path[0] == '.') {
        return safe_strdup("⚙️");
    }

    // Check for executable files
    if (is_executable(path)) {
        return safe_strdup("💾");
    }

    // Check for text files
    if (is_text_file(path)) {
        return safe_strdup("📝");
    }

    // Default emoji for unclassified files
    return safe_strdup("❓");
}

/**
 * Checks if a file is executable.
 *
 * @param path The path to the file.
 * @return 1 if the file is executable, 0 otherwise.
 */
int is_executable(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return (st.st_mode & S_IXUSR) != 0;
    }
    return 0;
}

/**
 * Checks if a file is a text file by examining its contents.
 *
 * This function reads the first 1024 bytes of the file and checks
 * if all characters are printable or whitespace.
 *
 * @param path The path to the file.
 * @return 1 if the file is likely a text file, 0 otherwise.
 */
int is_text_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return 0;
    }

    unsigned char buffer[1024];
    size_t bytesRead = fread(buffer, 1, sizeof(buffer), file);
    fclose(file);

    if (bytesRead == 0) {
        return 1;  // Empty file is considered text
    }

    for (size_t i = 0; i < bytesRead; i++) {
        if (!isprint(buffer[i]) && !isspace(buffer[i])) {
            return 0;
        }
    }
    return 1;
}
