#include "ExportTools.h"

void Win2PPM(int width, int height) {
    char outdir[10] = "out/"; //Must be defined!
    int i, j;
    FILE* fptr;
    static int counter = 0;
    char fname[32];
    unsigned char* image;

    /* Allocate our buffer for the image */
    image = (unsigned char*)malloc(3 * width * height * sizeof(char));
    if (image == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate memory for image\n");
    }

    /* Open the file */
    sprintf(fname, "%simage_%04d.ppm", outdir, counter);
    if ((fptr = fopen(fname, "w")) == NULL) {
        fprintf(stderr, "ERROR: Failed to open file for window capture\n");
    }

    /* Copy the image into our buffer */
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);

    /* Write the PPM file */
    fprintf(fptr, "P6\n%d %d\n255\n", width, height);
    for (j = height - 1; j >= 0; j--) {
        for (i = 0; i < width; i++) {
            fputc(image[3 * j * width + 3 * i + 0], fptr);
            fputc(image[3 * j * width + 3 * i + 1], fptr);
            fputc(image[3 * j * width + 3 * i + 2], fptr);
        }
    }

    free(image);
    fclose(fptr);
    counter++;
}