// Based on: https://github.com/itouhiro/bdf2bmp (LICENSE: BSD style)

/*
 * bdf2bmp  --  output all glyphs in a bdf-font to a bmp-image-file
 * version: 0.6
 * date:    Wed Jan 10 23:59:03 2001
 * author:  ITOU Hiroki (itouh@lycos.ne.jp)
 *
 * jpka, 2025: Replace unable to read today output format with raw array,
 * provide magick command to convert to any regular format;
 * Simplify some things.
 *
 */

/*
 * Copyright (c) 2000,2001 ITOU Hiroki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>  /* printf(), fopen(), fwrite() */
#include <stdlib.h> /* malloc(), EXIT_SUCCESS, strtol(), exit() */
#include <string.h> /* strcmp(), strcpy() */
#include <sys/stat.h> /* stat() */
#include <ctype.h> /* isdigit() */
#include <getopt.h>

#define LINE_CHARMAX 1000 /* number of max characters in bdf-font-file; number is without reason */
#define FILENAME_CHARMAX 256 /* number of max characters in filenames;  number is without reason */

/* macro */
#define STOREBITMAP()   if(flagBitmap){\
                            memcpy(nextP, sP, length);\
                            nextP += length;\
                        }

#define FIT(x, min, max) (x < min ? min : x > max ? max : x)

struct boundingbox{
        int w; /* width (pixel) */
        int h; /* height */
        int offx; /* offset y (pixel) */
        int offy; /* offset y */
};

/* global var */
struct boundingbox font; /* global boundingbox */
static int chars; /* total number of glyphs in a bdf file */
static int dwflag = 0; /* device width; only used for proportional-fonts */
char readFilename[FILENAME_CHARMAX] = "input.bdf";
char writeFilename[FILENAME_CHARMAX] = "out.raw";



/*
 * write to disk
 */
void dwrite(const void *varP, int n, FILE *outP){
        const unsigned char *p = varP;
        unsigned char tmp;

        /* LSB; write without arranging */
        tmp = fwrite(p, n, sizeof(unsigned char), outP);
        if(tmp != sizeof(unsigned char)){
                printf("error: cannot write an output-file\n");
                exit(EXIT_FAILURE);
        }
}



/*
 * 3. read bitmapAREA(onMemory) and write bmpFile with adding spacing
 *    BMP-file: noCompression(BI_RGB), 8bitColor, Windows-Win32 type
 */
void writeRawFile(unsigned char *bitmapP, int spacing, int colchar, FILE *bmpP){
        long bmpw; /* bmp-image width (pixel) */
        long bmph; /* bmp-image height (pixel) */
        /*  bmp-lines needs to be long alined and padded with 0 */
        unsigned char uchar;
        int x,y,g,tmp;
        int rowchar; /* number of row glyphs */
        int bx, by;

        /* bmp-image width */
        bmpw = (font.w+spacing)*colchar + spacing;

        /* bmp-image height */
        rowchar = (chars/colchar) + (chars%colchar!=0);
        bmph = (font.h+spacing)*rowchar + spacing;

        printf("  Raw Array width = %d pixels\n", (int)bmpw);
        printf("  Raw Array height = %d pixels\n", (int)bmph);
        printf("  Number of glyphs column=%d row=%d\n", colchar, rowchar);

                // + sizeof(long)*11 + sizeof(short)*5 + sizeof(char)*4*256;
        printf("  Raw Array filesize = %d bytes\n", (int)(bmpw * bmph));
        printf("\nNow please run:\n\n  magick -size %dx%d -depth 8 gray:out.raw %s.png(jpg, gif...)\n\n", (int)bmpw, (int)bmph, readFilename);

        /*
         * IMAGE DATA array
         */
        for(y=0; y<bmph; y++){
                for(x=0; x<bmpw; x++){
                        uchar = 63;
                        if( (y%(font.h+spacing)<spacing) || (x%(font.w+spacing)<spacing) ){
                                /* spacing */
                                dwrite(&uchar, sizeof(uchar), bmpP);
                        }else{
                                /* read bitmapAREA & write bmpFile */
                                g = (x/(font.w+spacing)) + (y/(font.h+spacing)*colchar);
                                bx = x - (spacing*(g%colchar)) - spacing;
                                by = y - (spacing*(g/colchar)) - spacing;
                                tmp = g*(font.h*font.w) + (by%font.h)*font.w + (bx%font.w);
                                if(tmp >= chars*font.h*font.w){
                                        /* spacing over the last glyph */
                                }else
                                        uchar = *( bitmapP + tmp);
                                dwrite(&uchar, sizeof(uchar), bmpP);
                        }
                }
        }
        return;
}




/*
 * 2. transfer bdf-font-file to bitmapAREA(onMemory) one glyph by one
 */
void assignBitmap(unsigned char *bitmapP, char *glyphP, int sizeglyphP, struct boundingbox glyph, int dw){
        static char *hex2binP[]= {
                "0000","0001","0010","0011","0100","0101","0110","0111",
                "1000","1001","1010","1011","1100","1101","1110","1111"
        };
        int d; /* decimal number translated from hexNumber */
        int hexlen; /* a line length(without newline code) */
        char binP[LINE_CHARMAX]; /* binary strings translated from decimal number */
        static int nowchar = 0; /* number of glyphs handlled until now */
        char *tmpP;
        char tmpsP[LINE_CHARMAX];
        int bitAh, bitAw; /* bitA width, height */
        int offtop, offbottom, offleft; /* glyph offset */
        unsigned char *bitAP;
        unsigned char *bitBP;
        int i,j,x,y;

        /*
         * 2.1) change hexadecimal strings to a bitmap of glyph( called bitA)
         */
        tmpP = strstr(glyphP, "\n");
        if(tmpP == NULL){
                /* if there is BITMAP\nENDCHAR in a given bdf-file */
                *glyphP = '0';
                *(glyphP+1) = '0';
                *(glyphP+2) = '\n';
                tmpP = glyphP + 2;
                sizeglyphP = 3;
        }
        hexlen = tmpP - glyphP;
        bitAw = hexlen * 4;
        bitAh = sizeglyphP / (hexlen+1);
        bitAP = malloc(bitAw * bitAh); /* address of bitA */
        if(bitAP == NULL){
                printf("error bitA malloc\n");
                exit(EXIT_FAILURE);
        }
        for(i=0,x=0,y=0; i<sizeglyphP; i++){
                if(glyphP[i] == '\n'){
                        x=0; y++;
                }else{
                        sprintf(tmpsP, "0x%c", glyphP[i]); /* get one character from hexadecimal strings */
                        d = (int)strtol(tmpsP,(char **)NULL, 16);
                        strcpy(binP, hex2binP[d]); /* change hexa strings to bin strings */
                        for(j=0; j<4; j++,x++){
                                if( bitAP+y*bitAw+x > bitAP+bitAw*bitAh ){
                                        printf("error: bitA pointer\n");
                                        exit(EXIT_FAILURE);
                                }else{
                                        *(bitAP + y*bitAw + x) = binP[j] - '0';
                                }
                        }
                }
        }

        /*
         * 2.2)make another bitmap area(called bitB)
         *      bitB is sized to FONTBOUNDINGBOX
         */
        bitBP = malloc(font.w * font.h); /* address of bitB */
        if(bitBP == NULL){
                printf("error bitB malloc\n");
                exit(EXIT_FAILURE);
        }
#if 0
// How to test it? What it does? -> Which colors to use?
        for(i=0; i<font.h; i++){
                for(j=0; j<font.w; j++){
                        if(! dwflag){
                                /* all in boundingbox: palette[0] */
                                *(bitBP + i*font.w + j) = 64;
                        }else{
                                /* show the baselines and widths of glyphs */
                                if( (j < (-1)*font.offx) || (j >= (-1)*font.offx+dw)){
                                        if(i < font.h -  (-1)*font.offy){
                                                /* over baseline: palette[3] */
                                                *(bitBP + i*font.w + j) = 96;
                                        }else{
                                                /* under baseline: palette[4] */
                                                *(bitBP + i*font.w + j) = 128;
                                        }
                                }else{
                                        /* in dwidth: palette[0] */
                                        *(bitBP + i*font.w + j) = 192;
                                }
                        }
                }
        }
#endif
        /*
         * 2.3) copy bitA inside BBX (smaller) to bitB (bigger)
         *       with offset-shifting;
         *      a scope beyond bitA is already filled with palette[0or3]
         */
        offleft = (-1)*font.offx + glyph.offx;
        /* offright = font.w - glyph.w - offleft; */

        offbottom = (-1)*font.offy + glyph.offy;
        offtop = font.h - glyph.h - offbottom;

        for(i=0; i<font.h; i++){
                if( i<offtop || i>=offtop+glyph.h )
                        ; /* do nothing */
                else
                        for(j=0; j<font.w; j++)
                                if( j<offleft || j>=offleft+glyph.w )
                                        ; /* do nothing */
                                else
                                        *(bitBP + i*font.w + j) = *(bitAP + (i-offtop)*bitAw + (j-offleft)) * 255;
        }

        /*
         * 2.4) copy bitB to bitmapAREA
         */
        for(i=0; i<font.h; i++)
                for(j=0; j<font.w; j++)
                        *(bitmapP + (nowchar*font.w*font.h) + (i*font.w) + j) = *(bitBP + i*font.w + j);

        nowchar++;
        free(bitAP);
        free(bitBP);
}


/*
 * read oneline from textfile
 */
int getLine(char* lineP, int max, FILE* inputP){
        if (fgets(lineP, max, inputP) == NULL)
                return 0;
        else
                return strlen(lineP); /* fgets returns strings included '\n' */
}


/*
 * 1. read BDF-file and transfer to assignBitmap()
 */
unsigned char *readBdfFile(unsigned char *bitmapP, FILE *readP){
        int i;
        int length;
        char sP[LINE_CHARMAX]; /* one line(strings) from bdf-font-file */
        static int cnt; /* only used in debugging: counter of appeared glyphs */
        struct boundingbox glyph; /* an indivisual glyph width, height,offset x,y */
        int flagBitmap = 0; /* this line is bitmap-data?(ON) or no?(OFF) */
        char *tokP; /* top address of a token from strings */
        char *glyphP = NULL; /* bitmap-data(hexadecimal strings) */
        char* nextP = NULL; /* address of writing next in glyphP */
        int dw = 0; /* dwidth */
        static int bdfflag = 0; /* the given bdf-file is valid or not */

        while(1){
                length = getLine(sP, LINE_CHARMAX, readP);
                if((! bdfflag) && (length == 0)){
                        /* given input-file is not a bdf-file */
                        printf("error: input-file is not a bdf file\n");
                        exit(EXIT_FAILURE);
                }
                if(length == 0)
                        break; /* escape from while-loop */

                /* remove carriage-return(CR) */
                for(i=0; i<length; i++){
                        if(sP[i] == '\r'){
                                sP[i] = '\n';
                                length--;
                        }
                }

                /* classify from the top character of sP */
                switch(sP[0]){
                case 'S':
                        if(! bdfflag){
                                /* top of the bdf-file */
                                if(strncmp(sP, "STARTFONT ", 10) == 0){
                                        bdfflag = 1;
                                        printf("startfont exists %d\n", bdfflag);
                                }else{
                                        /* given input-file is not a bdf-file */
                                        printf("error: input-file is not a bdf file\n");
                                        exit(EXIT_FAILURE);
                                }
                        }
                        break;
                case 'F':
                        if(strncmp(sP, "FONTBOUNDINGBOX ", 16) == 0){
                                /*  16 means no comparing '\0' */

                                /* get font.w, font.h, font.offx, and font.offy */
                                tokP = strtok(sP, " ");/* tokP addresses next space of FONTBOUNDINGBOX */
                                tokP += (strlen(tokP)+1);/* tokP addresses top character of width in FONTBOUNDINGBOX */
                                tokP = strtok(tokP, " ");/* set NUL on space after width */
                                font.w = atoi(tokP);
                                tokP += (strlen(tokP)+1);/* height in FONTBOUNDINGBOX */
                                tokP = strtok(tokP, " ");
                                font.h = atoi(tokP);
                                tokP += (strlen(tokP)+1);
                                tokP = strtok(tokP, " ");
                                font.offx = atoi(tokP);
                                tokP += (strlen(tokP)+1);
                                tokP = strtok(tokP, "\n");
                                font.offy = atoi(tokP);
                                printf("global glyph width=%dpixels ",font.w);
                                printf("height=%dpixels\n",font.h);
                                printf("global glyph offset x=%dpixels ",font.offx);
                                printf("y=%dpixels\n",font.offy);
                        }else
                                STOREBITMAP();
                        break;
                case 'C':
                        if(strncmp(sP, "CHARS ", 6) == 0){
                                /* get chars */
                                tokP = strtok(sP, " ");
                                tokP += (strlen(tokP)+1);
                                tokP = strtok(tokP, "\n");
                                chars = atoi(tokP);
                                printf("  Total glyphs = %d\n",chars);
                                cnt=0;

                                /* allocate bitmapAREA */
                                bitmapP = (unsigned char*)malloc(chars * font.h * font.w );
                                if(bitmapP == NULL){
                                        printf("error malloc\n");
                                        exit(EXIT_FAILURE);
                                }
                        }else
                                STOREBITMAP();
                        break;
                case 'D':
                        if(strncmp(sP, "DWIDTH ", 7) == 0){
                                /* get dw */
                                tokP = strtok(sP, " ");
                                tokP += (strlen(tokP)+1);
                                tokP = strtok(tokP, " ");
                                dw = atoi(tokP);
                        }else
                                STOREBITMAP();
                        break;
                case 'B':
                        if(strncmp(sP, "BITMAP", 6) == 0){
                                /* allocate glyphP */
                                glyphP = (char*)malloc(font.w*font.h); /* allocate more room */
                                if(glyphP == NULL){
                                        printf("error malloc bdf\n");
                                        exit(EXIT_FAILURE);
                                }
                                memset(glyphP, 0, font.w*font.h); /* zero clear */
                                nextP = glyphP;
                                flagBitmap = 1;
                        }else if(strncmp(sP, "BBX ", 4) == 0){
                                /* get glyph.offx, glyph.offy, glyph.w, and glyph.h */
                                tokP = strtok(sP, " ");/* space after 'BBX' */
                                tokP += (strlen(tokP)+1);/* top of width */
                                tokP = strtok(tokP, " ");/* set NUL on space after width */
                                glyph.w = atoi(tokP);
                                tokP += (strlen(tokP)+1);/* height */
                                tokP = strtok(tokP, " ");
                                glyph.h = atoi(tokP);
                                tokP += (strlen(tokP)+1);/* offx */
                                tokP = strtok(tokP, " ");
                                glyph.offx = atoi(tokP);
                                tokP += (strlen(tokP)+1);/* offy */
                                tokP = strtok(tokP, "\n");
                                glyph.offy = atoi(tokP);
                                /* d_printf("glyph width=%dpixels ",glyph.w); */
                                /* d_printf("height=%dpixels\n",glyph.h); */
                                /* d_printf("glyph offset x=%dpixels ",glyph.offx); */
                                /* d_printf("y=%dpixels\n",glyph.offy); */
                        }else
                                STOREBITMAP();
                        break;
                case 'E':
                        if(strncmp(sP, "ENDCHAR", 7) == 0){
                                // printf("\nglyph %d\n", cnt);
                                // printf("%s\n",glyphP);
                                assignBitmap(bitmapP, glyphP, nextP - glyphP, glyph, dw);
                                flagBitmap = 0;
                                free(glyphP);
                                cnt++;
                        }else
                                STOREBITMAP();
                        break;
                default:
                        STOREBITMAP();
                        break;
                }/* switch */
        }/* while */
        /* 'break' goto here */
        return bitmapP;
}


/*
 *
 */
void printhelp(void){
        printf("bdf2bmp version 0.6\n");
        printf("Usage: bdf2bmp [-option] input-bdf-file output-bmp-file\n");
        printf("Option:\n");
        printf("  -sN    spacing N pixels (default N=2)\n");
        printf("             N value can range from 0 to 32\n");
        printf("  -cN    specifying N colomns in grid (default N=32)\n");
        printf("             N value can range from 1 to 1024\n");
        printf("  -w     showing the baseline and the widths of glyphs\n");
        printf("             with gray colors (not work? can't test)\n");
        printf("  -i     prompting whether to overwrite an existing file\n");
        printf("  -h     print help\n");
        exit(EXIT_FAILURE);
}



/*
 *
 */
int main(int argc, char *argv[]){
        FILE *readP;
        FILE *writeP;
        int tmp, c;
        unsigned char *bitmapP = NULL; /* address of bitmapAREA */
        int spacing = 2; /* breadth of spacing (default 2) */
        int colchar = 32; /* number of columns(horizontal) (default 32) */
        int iflag = 0;
        struct stat fileinfo;

        /*
         * deal with arguments
         */
        int opt;
        while ((opt = getopt_long_only(argc, argv, "s:c:wih", NULL, NULL)) != -1){
                unsigned long ul = (optarg == NULL) ? 0 : strtoul(optarg, NULL, 10);
                switch (opt){
                        case 's':
                                spacing = FIT(ul, 0, 32);
                                break;
                        case 'c':
                                colchar = FIT(ul, 1, 1024);
                                break;
                        case 'w':
                                dwflag = 1;
                                break;
                        case 'i':
                                iflag = 1;
                                break;
                        default:
                                printhelp();
                }
        }

        if (argc - optind > 0) {
                strcpy(readFilename, argv[optind]);
                if (argc - optind > 1) {
                        strcpy(writeFilename, argv[optind + 1]);
                        if(strcmp(readFilename, writeFilename) == 0){
                                printf("error: input-filename and output-filename are same\n");
                                exit(EXIT_FAILURE);
                        }
                }
        }

        /*
         * prepare to read&write files
         */
        readP = fopen(readFilename, "r");
        if(readP == NULL){
                printf("bdf2bmp: '%s' does not exist\n", readFilename);
                exit(EXIT_FAILURE);
        }
        /* Does writeFilename already exist? */
        if((iflag) && (stat(writeFilename, &fileinfo)==0)){
                fprintf(stderr, "bdf2bmp: overwrite '%s'? ", writeFilename);
                c = fgetc(stdin);
                if((c=='y') || (c=='Y'))
                        ; /* go next */
                else
                        /* printf("not overwrite\n"); */
                        exit(EXIT_FAILURE);
        }
        writeP=fopen(writeFilename, "wb");
        if(writeP == NULL){
                printf("error: cannot write '%s'\n", writeFilename);
                exit(EXIT_FAILURE);
        }


        /* read bdf-font-file */
        bitmapP = readBdfFile(bitmapP, readP);
        fclose(readP);

        /* write bmp-image-file */
        writeRawFile(bitmapP, spacing, colchar, writeP);
        tmp = fclose(writeP);
        if(tmp == EOF){
                printf("error: cannot write '%s'\n", writeFilename);
                free(bitmapP);
                exit(EXIT_FAILURE);
        }

        free(bitmapP);
        return EXIT_SUCCESS;
}
