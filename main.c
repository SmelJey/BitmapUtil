#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#pragma pack(1)
typedef struct BMPHeader{
    //FileHeader
    uint16_t signature;
    uint32_t fileSize, reserved, dataOffset;

    //InfoHeader
    uint32_t size, width, height;
    uint16_t planes, bpp;
    uint32_t compression, imgSize, xppm, yppm, colorsCount, impColors;
} BMPHeader;

//uint32_t convertToCPU32(unsigned char* buffer, int *j){
//    uint32_t cpuNum = buffer[(*j)++];
//    for (int i = 1; i < 4; i++, (*j)++){
//        cpuNum = cpuNum | ((uint32_t)buffer[*j] << (8 * i));
//    }
//    return cpuNum;
//}
//
//uint16_t convertToCPU16(unsigned char* buffer, int *j){
//    return ((uint16_t)buffer[(*j)++]) | (((uint16_t)buffer[(*j)++]) << 8);
//}
//
//void readBMPHeader(BMPHeader* bmpHeader, FILE *fstream){
//    unsigned char buffer[256];
//    int bufIndex = 0;
//
//    fread(bmpHeader, sizeof(BMPHeader), 1, fstream);
//
//#if 0
//    bmpHeader->signature = convertToCPU16(buffer, &bufIndex);
//    bmpHeader->fileSize = convertToCPU32(buffer, &bufIndex);
//    bmpHeader->reserved = convertToCPU32(buffer, &bufIndex);
//    bmpHeader->dataOffset = convertToCPU32(buffer, &bufIndex);
//    bmpHeader->size = convertToCPU32(buffer, &bufIndex);
//    bmpHeader->width = convertToCPU32(buffer, &bufIndex);
//    bmpHeader->height = convertToCPU32(buffer, &bufIndex);
//    bmpHeader->planes = convertToCPU16(buffer, &bufIndex);
//    bmpHeader->bpp = convertToCPU16(buffer, &bufIndex);
//    bmpHeader->compression = convertToCPU32(buffer, &bufIndex);
//    bmpHeader->imgSize = convertToCPU32(buffer, &bufIndex);
//    bmpHeader->xppm = convertToCPU32(buffer, &bufIndex);
//    bmpHeader->yppm = convertToCPU32(buffer, &bufIndex);
//    bmpHeader->colorsCount = convertToCPU32(buffer, &bufIndex);
//    bmpHeader->impColors = convertToCPU32(buffer, &bufIndex);
//#endif
//    int j = 0;
//    if (bmpHeader->reserved != 0 || bmpHeader->signature != convertToCPU16("BM", &j) ||
//        bmpHeader->size != 40 || bmpHeader->planes != 1 || bmpHeader->compression > 2){
//
//        printf("brighten: Can't read this file\n");
//        exit(4);
//    }
//}

void printBMPHeader(BMPHeader* bmpHeader){
    printf("Header: %x\n File size: %d\n Data Offset: %x\n", bmpHeader->signature, bmpHeader->fileSize, bmpHeader->dataOffset);
    printf("InfoHeader:\n Width: %d\n Height: %d\n "
           "Bits per pixel: %d\n Compression: %x\n "
           "Size of image: %d\n XPixels per meter: %d\n YPixels per meter: %d\n "
           "Colors Count: %d\n Important Colors: %d\n",
           bmpHeader->width, bmpHeader->height, bmpHeader->bpp, bmpHeader->compression,
           bmpHeader->imgSize, bmpHeader->xppm, bmpHeader->yppm, bmpHeader->colorsCount, bmpHeader->impColors);
}

#define DEBUG 1
#if !DEBUG
#define printBMPHeader
#endif

int main(int argc, char** argv) {
    FILE *ifStream, *ofStream;
    int brightness;

    if (argc < 3){
        printf("brighten: Not enough args. Need: brightness, input file, output file\n");
        return 1;
    }

    char currentDir[256];
    strcat(getcwd(currentDir, 256), argv[2]);
    ifStream = fopen(argv[2], "rb");
    if (ifStream == NULL){
        printf("brighten: Can't open this file\n");
        return 2;
    }

    char *pEnd;
    brightness = strtol(argv[1], &pEnd, 10);
    if (brightness == 0){
        printf("brighten: Incorrect value: %d\n", brightness);
        return 3;
    }


    BMPHeader bmpHeader;
    fread(&bmpHeader, sizeof(BMPHeader), 1, ifStream);
    printBMPHeader(&bmpHeader);

    fseek(ifStream, bmpHeader.dataOffset, SEEK_SET);
    ofStream = fopen(argv[3], "wb");
    fwrite(&bmpHeader, sizeof(BMPHeader), 1, ofStream);

    if (bmpHeader.compression != 0){
        printf("brighten: Compression is not supported");
        exit(5);
    }


    printf("DEBUG: %d\n", bmpHeader.fileSize - bmpHeader.dataOffset);
    unsigned char* bmpData = (unsigned char*) malloc(bmpHeader.width * bmpHeader.height * (bmpHeader.bpp / 8));
    int d = bmpHeader.width * bmpHeader.height * (bmpHeader.bpp / 8);
    fread(bmpData, d, 1, ifStream);

    switch(bmpHeader.bpp){
        case 16:
        case 24: {
            for (int i = 0; i < bmpHeader.width * bmpHeader.height * (bmpHeader.bpp / 8);) {
                uint32_t triplet = 0;
                uint32_t newVal = 0;
                uint8_t offset = 0;
                uint8_t colorSize = bmpHeader.bpp / 3;

                for (int j = 0; j < bmpHeader.bpp / 8; j++, i++){
                    triplet = triplet + (bmpData[i] << (j * 8));
                }

                for (int k = 0; k < 3; k++, offset += colorSize){
                    uint32_t pixel = ((triplet >> offset) & ((1 << colorSize) - 1));
                    pixel = pixel + (int32_t)pixel * brightness / 100;
                    if (pixel > (1 << colorSize) - 1){
                        newVal = brightness > 0 ? newVal | (((1 << colorSize) - 1) << offset) : newVal;
                    }
                    else
                        newVal = newVal | (pixel << offset);
                }
                fwrite(&newVal, bmpHeader.bpp / 8, 1, ofStream);
            }
            break;
        }
        default:
            printf("brighten: Unsupported Bits-per-pixel value: %d", bmpHeader.bpp);
            return 5;
    }

    fclose(ofStream);
    fclose(ifStream);

    return 0;
}