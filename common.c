#include <stdio.h>

#define GREEN "\x1b[92m"
#define RESET "\x1b[0m"

void progress_bar(const char *label, double current, double total, double speed)
{
    const int width = 20;

    double progress = current / total;
    if (progress < 0) progress = 0;
    if (progress > 1) progress = 1;

    int filled = (int)(progress * width);

    double speed_MB = speed / (1024.0 * 1024.0);
    double speed_Mb = speed * 8.0 / 1000000.0;

    double eta = speed > 0
        ? (total - current) / speed
        : 0;

    // build the bar into its own buffer first
    char bar[64];
    int i = 0;
    for (; i < filled; i++) bar[i] = '=';
    for (; i < width; i++)  bar[i] = ' ';
    bar[width] = '\0';

    // build the whole line in one buffer, then write it in a single call
    char line[256];
    snprintf(line, sizeof(line),
        "\r%-12s " GREEN "%s" RESET
        " | %.1f/%.1f MB | ETA: %.0f s | Speed: %.2f MB/s (%.2f Mb/s)\033[K",
        label, bar,
        current / (1024.0 * 1024.0),
        total / (1024.0 * 1024.0),
        eta, speed_MB, speed_Mb);

    fputs(line, stdout);
    fflush(stdout);

    if (current >= total)
        printf("\n");
}