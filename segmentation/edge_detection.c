#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define GRAY_LEVELS          255

struct bmpfileheader{
    unsigned short  filetype;
    unsigned long   filesize;
    short  reserved1;
    short  reserved2;
    unsigned long   bitmapoffset;
};

struct bitmapheader{
    unsigned long   size;
    long   width;
    long   height;
    unsigned short  planes;
    unsigned short  bitsperpixel;
    unsigned long   compression;
    unsigned long   sizeofbitmap;
    unsigned long   horzres;
    unsigned long   vertres;
    unsigned long   colorsused;
    unsigned long   colorsimp;
};

struct ctstruct{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
};

union short_char_union{
    short s_num;
    char  s_alpha[2];
};

union int_char_union{
    int  i_num;
    char i_alpha[2];
};

union long_char_union{
    long  l_num;
    char  l_alpha[4];
};

union float_char_union{
    float f_num;
    char  f_alpha[4];
};

union ushort_char_union{
    short s_num;
    char  s_alpha[2];
};

union uint_char_union{
    int  i_num;
    char i_alpha[2];
};

union ulong_char_union{
    long  l_num;
    char  l_alpha[4];
};

short average[3][3] = {{1, 2, 1}, 
                       {2, 4, 2},
                       {1, 2, 1}};
short sharpen[3][3] = {{1, 1, 1},
                       {1, -8, 1},
                       {1, 1, 1}};
short soble1[3][3] = {{-1, -2, -1}, 
                      {0, 0, 0},
                      {1, 2, 1}};
short soble2[3][3] = {{-1, 0, 1}, 
                      {-2, 0, 2},
                      {-1, 0, 1}};
short soble3[3][3] = {{0, 1, 2}, 
                      {-1, 0, 1},
                      {-2, -1, 0}};
short soble4[3][3] = {{-2, -1, 0}, 
                      {-1, 0, 1},
                      {0, 1, 2}};    

#define ARRAY_SIZE 3

int get_image_size(const char* file_name, long* rows, long* cols);
void extract_long_from_buffer(char buffer[], int lsb, int start, long* number);
void extract_ulong_from_buffer(char buffer[], int lsb, int start,unsigned long* number);
void extract_short_from_buffer(char buffer[], int lsb, int start, short* number);
void extract_ushort_from_buffer(char buffer[], int lsb, int start, unsigned short* number);
void insert_short_into_buffer(char buffer[], int start, short number);
void insert_ushort_into_buffer(char buffer[], int start, unsigned short number);
void insert_long_into_buffer(char buffer[], int start, long number);
void insert_ulong_into_buffer(char buffer[], int start, unsigned long number);
short **allocate_image_array(long length, long width);
int free_image_array(short **the_array, long length);
double **allocate_double_array(long, long);
int free_double_array(double**, long);
void create_allocate_bmp_file(const char* file_name, struct bmpfileheader* file_header, struct bitmapheader* bmheader);
void read_bmp_file_header(const char* file_name, struct bmpfileheader* file_header);
void read_bm_header(const char* file_name, struct bitmapheader* bmheader);
int calculate_pad(long width);
void write_bmp_image(const char* file_name, short **array);
int does_not_exist(const char* file_name);
void flip_image_array(short** the_image, long rows, long cols);
void read_color_table(const char* file_name, struct ctstruct* rgb, int size);
void read_bmp_image(const char* file_name, short** array);
int is_in_array(short x, short y, short height, short width);
short convolution(short** in_array, long row, long column, long height, long width, short** filter, long filter_size);
short convolutiond(short** in_array, long row, long column, long height, long width, double** filter, long filter_size);
void laplace(short** in_array, short** out_array, long height, long width);
void edge_detection(short** in_array, short** out_array, long height, long width);
void marr(short** in_array, short** out_array, long height, long width);

// .\gray_trans.exe test.bmp out.bmp

int main(int argc, char const *argv[])
{
    char*   cc;
    long    h, w;
    short  **in_array, **out_array;
    struct  bmpfileheader      bmp_file_header;
    struct  bitmapheader       bmheader;

    if(argc < 2){
        printf("\nusage: in_bmp_file_name out_bmp_file_nam\n");
        exit(-1);
    }
    if(does_not_exist(argv[1])){
        printf("\nERROR input file %s does not exist", argv[1]);
        exit(0);
    }
    cc = strstr(argv[1], ".bmp");
    if(cc == NULL){  
        printf("\nERROR %s must be a bmp file", argv[1]);
        exit(0);
    }
    cc = strstr(argv[2], ".bmp");
    if(cc == NULL){  /* create a bmp */
        printf("\nERROR %s must be a bmp file name", argv[2]);
        exit(0);
    }

    get_image_size(argv[1], &h, &w);
    in_array = allocate_image_array(h, w);
    read_bmp_image(argv[1], in_array);
    out_array = allocate_image_array(h, w);
    bmheader.height = h;
    bmheader.width  = w;

    create_allocate_bmp_file(argv[2], &bmp_file_header, &bmheader);

    // edge_detection(in_array, out_array, h, w);
    marr(in_array, out_array, h, w);
    write_bmp_image(argv[2], out_array);

    free_image_array(in_array, h);
    free_image_array(out_array, h);

    return 0;
}

void generate_gaussian_filter(double** gaussian_filter, long sigma)
{
    double x, y;
    long filter_size = 6 * sigma + 1;

    for (int i = 0; i < filter_size; i++){
        for (int j = 0; j < filter_size; j++){
            x = i - filter_size / 2;
            y = j - filter_size / 2;
            // printf("x = %lf, y = %lf \n", x, y);
            gaussian_filter[i][j] = exp(-1.0 * ((pow(x, 2) + pow(y, 2)) / 2 * sigma * sigma));
        }
    }
}

short convolution(short** in_array, long row, long column, long height, long width, short** filter, long filter_size)
{
    short** temp;
    short r_value = 0;
    long x, y;

    temp = allocate_image_array(filter_size, filter_size);
    for (int i = 0; i < filter_size; i++){
        for (int j = 0; j < filter_size; j++){
            x = i - filter_size / 2 + row;
            y = j - filter_size / 2 + column;
            temp[i][j] = is_in_array(x, y, height, width) ? in_array[x][y] : 0;
            r_value += temp[i][j] * filter[i][j];
        }
    }
    free_image_array(temp, filter_size);

    return r_value;
}

short convolutiond(short** in_array, long row, long column, long height, long width, double** filter, long filter_size)
{
    short** temp;
    double r_value = 0.0;
    long x, y;

    temp = allocate_image_array(filter_size, filter_size);
    for (int i = 0; i < filter_size; i++){
        for (int j = 0; j < filter_size; j++){
            x = i - filter_size / 2 + row;
            y = j - filter_size / 2 + column;
            temp[i][j] = is_in_array(x, y, height, width) ? in_array[x][y] : 0;
            r_value += temp[i][j] * filter[i][j];
        }
    }
    free_image_array(temp, filter_size);

    return (short)r_value;
}

void laplace(short** in_array, short** out_array, long height, long width)
{
    short** a_sharpen;

    a_sharpen = allocate_image_array(3, 3);
    for (int i = 0; i < 3; i++){
        for (int j = 0; j < 3; j++){
            a_sharpen[i][j] = sharpen[i][j];
        }
    }
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            out_array[i][j] = convolution(in_array, i, j, height, width, a_sharpen, 3);
            // if (out_array[i][j] < 0)
            //     out_array[i][j] = 0;
            // else if (out_array[i][j] > 0xff)
            //     out_array[i][j] = 0xff;
        }
    }
    free_image_array(a_sharpen, 3);
}

int is_cross(short** in_array, long row, long column)
{
    int cross_num = 0;

    if (in_array[row-1][column-1] * in_array[row+1][column+1] < 0 && 
        abs(abs(in_array[row-1][column-1]) - abs(in_array[row+1][column+1])) > 0x66)
        cross_num++;
    if (in_array[row-1][column] * in_array[row+1][column] < 0&& 
        abs(abs(in_array[row-1][column]) - abs(in_array[row+1][column])) > 0x66)
        cross_num++;
    if (in_array[row-1][column+1] * in_array[row+1][column-1] < 0&& 
        abs(abs(in_array[row-1][column+1]) - abs(in_array[row+1][column-1])) > 0x66)
        cross_num++;
    if (in_array[row][column-1] * in_array[row][column+1] < 0&& 
        abs(abs(in_array[row][column-1]) - abs(in_array[row][column+1])) > 0x66)
        cross_num++;

    if (cross_num >= 2)
        return 1;
    else
        return 0;
}

void zero_cross(short** in_array, short** out_array, long height, long width)
{
    /*
     * 不处理图像周围1像素宽度
     */
    for (int i = 1; i < height-1; i++){
        for (int j = 1; j < width-1; j++){
            if (is_cross(in_array, i, j))
                out_array[i][j] = 0xff;
            else
                out_array[i][j] = 0x00;
        }
    }
}

void marr(short** in_array, short** out_array, long height, long width)
{
    long sigma = 2;
    long filter_size = 6 * sigma + 1;
    double** gaussian_filter;
    short **gauss_array, **laplace_array;

    gaussian_filter = allocate_double_array(filter_size, filter_size);
    gauss_array = allocate_image_array(height, width);
    laplace_array = allocate_image_array(height, width);
    generate_gaussian_filter(gaussian_filter, sigma);

    // for (int i = 0; i < filter_size; i++){
    //     for (int j = 0; j < filter_size; j++)
    //         printf("%lf ", gaussian_filter[i][j]);
    //     printf("\n");
    // }
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            gauss_array[i][j] = convolutiond(in_array, i, j, height, width, gaussian_filter, filter_size);
            // if (gauss_array[i][j] < 0)
            //     gauss_array[i][j] = 0;
            // else if (gauss_array[i][j] > 0xff)
            //     gauss_array[i][j] = 0xff;
        }
    }
    printf("Gasuuian filter done\n");
    laplace(gauss_array, laplace_array, height, width);
    printf("Laplace done\n");
    zero_cross(laplace_array, out_array, height, width);
    printf("Zero cross done\n");
    
    free_double_array(gaussian_filter, filter_size);
    free_image_array(gauss_array, height);
    free_image_array(laplace_array, height);
}

int is_in_array(short x, short y, short height, short width)
{
    if (x >= 0 && x < height && y >= 0 && y < width)
        return 1;
    else
        return 0;
}

/*
 * element
 * v0  v1  v2
 * v3  v4  v5
 * v6  v7  v8
 *
 */
void edge_detection(short** in_array, short** out_array, long height, long width)
{
    short gx = 0, gy = 0;
    short** a_soble1;
    short** a_soble2;

    a_soble1 = allocate_image_array(3, 3);
    a_soble2 = allocate_image_array(3, 3);
    for (int i = 0; i < 3; i++){
        for (int j = 0; j < 3; j++){
            a_soble1[i][j] = soble1[i][j];
            a_soble2[i][j] = soble2[i][j];
        }
    }
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            /*
             * 直接使用强制类型转化传递soble数组出错
             */
            gx = convolution(in_array, i, j, height, width, a_soble1, 3);
            gy = convolution(in_array, i, j, height, width, a_soble2, 3);
            // out_array[i][j] = gx;
            // out_array[i][j] = gy;
            out_array[i][j] = gx + gy;
            if (out_array[i][j] < 0)
                out_array[i][j] = 0;
            else if (out_array[i][j] > 0xff)
                out_array[i][j] = 0xff;

            /* edge enhancement */
            // if (out_array[i][j] < 0)
            //     out_array[i][j] = 0;
            // else if (out_array[i][j] > 0xff)
            //     out_array[i][j] = 0xff;
            // else if (out_array[i][j] > 200)
            //     ;
            // else
            //     out_array[i][j] = in_array[i][j];
        }
    }
    free_image_array(a_soble1, 3);
    free_image_array(a_soble2, 3);
}

double **allocate_double_array(long length, long width)
{
   int i;
   double **the_array;

   the_array = malloc(length * sizeof(double  *));
   for(i=0; i<length; i++){
      the_array[i] = malloc(width * sizeof(double ));
      if(the_array[i] == '\0'){
         printf("\n\tmalloc of the_double[%d] failed", i);
      }  /* ends if */
   }  /* ends loop over i */
   return(the_array);
}  /* ends allocate_double_array */

int free_double_array(double **the_array, long length)
{
    int i;
    for(i=0; i<length; i++)
        free(the_array[i]);
    return(1);
}  /* ends free_double_array */

void read_color_table(const char* file_name, struct ctstruct* rgb, int size)
{
   int  i;
   char buffer[10];
   FILE *fp;

   fp = fopen(file_name, "rb");

   fseek(fp, 54, SEEK_SET);

   for(i=0; i<size; i++){
      fread(buffer, 1, 1, fp);
      rgb[i].blue = buffer[0];
      fread(buffer, 1, 1, fp);
      rgb[i].green = buffer[0];
      fread(buffer, 1, 1, fp);
      rgb[i].red = buffer[0];
      fread(buffer, 1, 1, fp);
         /* fourth byte nothing */
   }  /* ends loop over i */

   fclose(fp);
}  /* ends read_color_table */

void read_bmp_image(const char* file_name, short** array)
{
    FILE   *fp;
    int    i, j;
    int    negative = 0,
           pad      = 0,
           place    = 0;
    long   colors   = 0,
           height   = 0,
           //position = 0,
           width    = 0;
    struct bmpfileheader file_header;
    struct bitmapheader  bmheader;
    struct ctstruct rgb[GRAY_LEVELS+1];
    unsigned char uc;

    read_bmp_file_header(file_name, &file_header);
    read_bm_header(file_name, &bmheader);
    if(bmheader.bitsperpixel != 8){
        printf("\nCannot read image when bits per pixel is not 8");
        exit(1);
    }

    if(bmheader.colorsused == 0)
        colors = GRAY_LEVELS + 1;
    else
        colors = bmheader.colorsused;
    read_color_table(file_name, (struct ctstruct*)&rgb, colors);

    fp = fopen(file_name, "rb");
    fseek(fp, file_header.bitmapoffset, SEEK_SET);

    width = bmheader.width;
    if(bmheader.height < 0){
        height   = bmheader.height * (-1);
        negative = 1;
    }
    else
        height = bmheader.height;

    pad = calculate_pad(width);

    for(i=0; i<height; i++){
        for(j=0; j<width; j++){
            place = fgetc(fp);
            uc = (place & 0xff);
            place = uc;
            array[i][j] = rgb[place].blue;
        }  /* ends loop over j */
        if(pad != 0){
            //position = fseek(fp, pad, SEEK_CUR);
            fseek(fp, pad, SEEK_CUR);
        }  /* ends if pad 1= 0 */
    }  /* ends loop over i */

    if(negative == 0)
        flip_image_array(array, height, width);
}  /* ends read_bmp_image */

void flip_image_array(short** the_image, long rows, long cols)
{
    int  i, j;
    long rd2;
    short **temp;

    temp = allocate_image_array(rows, cols);
    rd2  = rows/2;
    for(i=0; i<rd2; i++){
        for(j=0; j<cols; j++){
            temp[rows-1-i][j] = the_image[i][j];
        }  /* ends loop over j */
    }  /* ends loop over i */

    for(i=rd2; i<rows; i++){
        for(j=0; j<cols; j++){
            temp[rows-1-i][j] = the_image[i][j];
        }  /* ends loop over j */
    }  /* ends loop over i */

    for(i=0; i<rows; i++)
        for(j=0; j<cols; j++)
            the_image[i][j] = temp[i][j];

    free_image_array(temp, rows);
}  /* ends flip_image_array */

int get_image_size(const char* file_name, long* rows, long* cols)
{
    struct bitmapheader bmph;

    read_bm_header(file_name, &bmph);
    *rows = bmph.height;
    *cols = bmph.width;

    return 1;
}  /* ends get_image_size */

short **allocate_image_array(long length, long width)
{
   int i;
   short **the_array;

   the_array = malloc(length * sizeof(short  *));
   for(i=0; i<length; i++){
      the_array[i] = malloc(width * sizeof(short ));
      if(the_array[i] == '\0'){
         printf("\n\tmalloc of the_image[%d] failed", i);
      }  /* ends if */
   }  /* ends loop over i */
   return(the_array);
}  /* ends allocate_image_array */

int free_image_array(short **the_array, long length)
{
    int i;
    for(i=0; i<length; i++)
        free(the_array[i]);
    return(1);
}  /* ends free_image_array */

int calculate_pad(long width)
{
   int pad = 0;
   pad = ( (width%4) == 0) ? 0 : (4-(width%4));
   return(pad);
}  /* ends calculate_pad */

void create_allocate_bmp_file(const char *file_name, struct bmpfileheader *file_header, struct bitmapheader *bmheader)
{
    char buffer[100];
    int  i, pad = 0;
    FILE *fp;

    pad = calculate_pad(bmheader->width);

    bmheader->size         =  40;
    bmheader->planes       =   1;
    bmheader->bitsperpixel =   8;
    bmheader->compression  =   0;
    bmheader->sizeofbitmap = bmheader->height * 
                            (bmheader->width + pad);
    bmheader->horzres      = 300;
    bmheader->vertres      = 300;
    bmheader->colorsused   = 256;
    bmheader->colorsimp    = 256;

    file_header->filetype     = 0x4D42;
    file_header->reserved1    =  0;
    file_header->reserved2    =  0;
    file_header->bitmapoffset = 14 + 
                                bmheader->size +
                                bmheader->colorsused*4;
    file_header->filesize     = file_header->bitmapoffset +
                                bmheader->sizeofbitmap;

    if((fp = fopen(file_name, "wb")) == NULL){
      printf("\nERROR Could not create file %s",
             file_name);
      exit(2);
    }

    insert_ushort_into_buffer(buffer, 0, file_header->filetype);
    fwrite(buffer, 1, 2, fp);

    insert_ulong_into_buffer(buffer, 0, file_header->filesize);
    fwrite(buffer, 1, 4, fp);

    insert_short_into_buffer(buffer, 0, file_header->reserved1);
    fwrite(buffer, 1, 2, fp);

    insert_short_into_buffer(buffer, 0, file_header->reserved2);
    fwrite(buffer, 1, 2, fp);

    insert_ulong_into_buffer(buffer, 0, file_header->bitmapoffset);
    fwrite(buffer, 1, 4, fp);


      /*********************************************
      *
      *   Write the 40-byte bit map header.
      *
      *********************************************/

    insert_ulong_into_buffer(buffer, 0, bmheader->size);
    fwrite(buffer, 1, 4, fp);

    insert_long_into_buffer(buffer, 0, bmheader->width);
    fwrite(buffer, 1, 4, fp);

    insert_long_into_buffer(buffer, 0, bmheader->height);
    fwrite(buffer, 1, 4, fp);

    insert_ushort_into_buffer(buffer, 0, bmheader->planes);
    fwrite(buffer, 1, 2, fp);

    insert_ushort_into_buffer(buffer, 0, bmheader->bitsperpixel);
    fwrite(buffer, 1, 2, fp);

    insert_ulong_into_buffer(buffer, 0, bmheader->compression);
    fwrite(buffer, 1, 4, fp);

    insert_ulong_into_buffer(buffer, 0, bmheader->sizeofbitmap);
    fwrite(buffer, 1, 4, fp);

    insert_ulong_into_buffer(buffer, 0, bmheader->horzres);
    fwrite(buffer, 1, 4, fp);

    insert_ulong_into_buffer(buffer, 0, bmheader->vertres);
    fwrite(buffer, 1, 4, fp);

    insert_ulong_into_buffer(buffer, 0, bmheader->colorsused);
    fwrite(buffer, 1, 4, fp);

    insert_ulong_into_buffer(buffer, 0, bmheader->colorsimp);
    fwrite(buffer, 1, 4, fp);

      /*********************************************
      *
      *   Write a blank color table.
      *   It has 256 entries (number of colors)
      *   that are each 4 bytes.
      *
      *********************************************/

    buffer[0] = 0x00;

    for(i=0; i<(256*4); i++)
        fwrite(buffer, 1, 1, fp);

      /*********************************************
      *
      *   Write a zero image.  
      *
      *********************************************/

    buffer[0] = 0x00;

    for(i=0; i<bmheader->sizeofbitmap; i++)
        fwrite(buffer, 1, 1, fp);

    fclose(fp);
}  /* ends create_allocate_bmp_file */

void write_bmp_image(const char* file_name, short **array)
{
    char   *buffer;
    FILE   *image_file;
    int    pad = 0;
    int    i, j;
    long   height = 0, width = 0;
    struct bitmapheader  bmheader;
    struct bmpfileheader file_header;
    struct ctstruct rgb[GRAY_LEVELS+1];
    union  short_char_union scu;

    read_bmp_file_header(file_name, &file_header);
    read_bm_header(file_name, &bmheader);

    height = bmheader.height;
    width  = bmheader.width;
    if(height < 0) 
        height = height*(-1);

    buffer = (char *) malloc(width * sizeof(char ));
    for(i=0; i<width; i++)
        buffer[i] = '\0';

    image_file = fopen(file_name, "rb+");

    /****************************************
    *
    *   Write the color table first.
    *
    ****************************************/

    fseek(image_file, 54, SEEK_SET);
    for(i=0; i<GRAY_LEVELS+1; i++){
        rgb[i].blue  = i;
        rgb[i].green = i;
        rgb[i].red   = i;
    }  /* ends loop over i */

    for(i=0; i<bmheader.colorsused; i++){
        buffer[0] = rgb[i].blue;
        fwrite(buffer , 1, 1, image_file);
        buffer[0] = rgb[i].green;
        fwrite(buffer , 1, 1, image_file);
        buffer[0] = rgb[i].red;
        fwrite(buffer , 1, 1, image_file);
        buffer[0] = 0x00;
        fwrite(buffer , 1, 1, image_file);
    }  /* ends loop over i */

    fseek(image_file, file_header.bitmapoffset, SEEK_SET);

    pad = calculate_pad(width);

    for(i=0; i<height; i++){
        for(j=0; j<width; j++){

            if(bmheader.bitsperpixel == 8){
                scu.s_num = 0;
                if(bmheader.height > 0)
                    scu.s_num = array[height-1-i][j];
                else
                    scu.s_num = array[i][j];
                buffer[j] = scu.s_alpha[0];
            }  /* ends if bits_per_pixel == 8 */
            else{
                printf("\nERROR bitsperpixel is not 8");
                exit(1);
            }
        }  /* ends loop over j */

        fwrite(buffer, 1, width, image_file);

        if(pad != 0){
            for(j=0; j<pad; j++)
                buffer[j] = 0x00;
            fwrite(buffer, 1, pad, image_file);
        }  /* ends if pad != 0 */

    }  /* ends loop over i */

    fclose(image_file);
    free(buffer);
}  /* ends write_bmp_image */

void read_bmp_file_header(const char *file_name, struct bmpfileheader *file_header)
{
   char  buffer[10];
   short ss;
   unsigned long  ull;
   unsigned short uss;
   FILE     *fp;

   fp = fopen(file_name, "rb");

   fread(buffer, 1, 2, fp);
   extract_ushort_from_buffer(buffer, 1, 0, &uss);
   file_header->filetype = uss;

   fread(buffer, 1, 4, fp);
   extract_ulong_from_buffer(buffer, 1, 0, &ull);
   file_header->filesize = ull;

   fread(buffer, 1, 2, fp);
   extract_short_from_buffer(buffer, 1, 0, &ss);
   file_header->reserved1 = ss;

   fread(buffer, 1, 2, fp);
   extract_short_from_buffer(buffer, 1, 0, &ss);
   file_header->reserved2 = ss;

   fread(buffer, 1, 4, fp);
   extract_ulong_from_buffer(buffer, 1, 0, &ull);
   file_header->bitmapoffset = ull;

   fclose(fp);

}  /* ends read_bmp_file_header */

void read_bm_header(const char* file_name, struct bitmapheader *bmheader)
{
    char  buffer[10];
    long  ll;
    unsigned long  ull;
    unsigned short uss;
    FILE *fp;

    fp = fopen(file_name, "rb");

    /****************************************
    *
    *   Seek past the first 14 byte header.
    *
    ****************************************/

    fseek(fp, 14, SEEK_SET);

    fread(buffer, 1, 4, fp);
    extract_ulong_from_buffer(buffer, 1, 0, &ull);
    bmheader->size = ull;

    fread(buffer, 1, 4, fp);
    extract_long_from_buffer(buffer, 1, 0, &ll);
    bmheader->width = ll;

    fread(buffer, 1, 4, fp);
    extract_long_from_buffer(buffer, 1, 0, &ll);
    bmheader->height = ll;

    fread(buffer, 1, 2, fp);
    extract_ushort_from_buffer(buffer, 1, 0, &uss);
    bmheader->planes = uss;

    fread(buffer, 1, 2, fp);
    extract_ushort_from_buffer(buffer, 1, 0, &uss);
    bmheader->bitsperpixel = uss;

    fread(buffer, 1, 4, fp);
    extract_ulong_from_buffer(buffer, 1, 0, &ull);
    bmheader->compression = ull;

    fread(buffer, 1, 4, fp);
    extract_ulong_from_buffer(buffer, 1, 0, &ull);
    bmheader->sizeofbitmap = ull;

    fread(buffer, 1, 4, fp);
    extract_ulong_from_buffer(buffer, 1, 0, &ull);
    bmheader->horzres = ull;

    fread(buffer, 1, 4, fp);
    extract_ulong_from_buffer(buffer, 1, 0, &ull);
    bmheader->vertres = ull;

    fread(buffer, 1, 4, fp);
    extract_ulong_from_buffer(buffer, 1, 0, &ull);
    bmheader->colorsused = ull;

    fread(buffer, 1, 4, fp);
    extract_ulong_from_buffer(buffer, 1, 0, &ull);
    bmheader->colorsimp = ull;

    fclose(fp);
}  /* ends read_bm_header */

void extract_long_from_buffer(char buffer[], int lsb, int start, long* number)
{
    union long_char_union lcu;

    if(lsb == 1){
        lcu.l_alpha[0] = buffer[start+0];
        lcu.l_alpha[1] = buffer[start+1];
        lcu.l_alpha[2] = buffer[start+2];
        lcu.l_alpha[3] = buffer[start+3];
    }  /* ends if lsb = 1 */

    if(lsb == 0){
        lcu.l_alpha[0] = buffer[start+3];
        lcu.l_alpha[1] = buffer[start+2];
        lcu.l_alpha[2] = buffer[start+1];
        lcu.l_alpha[3] = buffer[start+0];
    }  /* ends if lsb = 0      */

    *number = lcu.l_num;
    }  /* ends extract_long_from_buffer */

void extract_ulong_from_buffer(char buffer[], int lsb, int start,unsigned long* number)
{
    union ulong_char_union lcu;

    if(lsb == 1){
        lcu.l_alpha[0] = buffer[start+0];
        lcu.l_alpha[1] = buffer[start+1];
        lcu.l_alpha[2] = buffer[start+2];
        lcu.l_alpha[3] = buffer[start+3];
    }  /* ends if lsb = 1 */

    if(lsb == 0){
        lcu.l_alpha[0] = buffer[start+3];
        lcu.l_alpha[1] = buffer[start+2];
        lcu.l_alpha[2] = buffer[start+1];
        lcu.l_alpha[3] = buffer[start+0];
    }  /* ends if lsb = 0      */
    *number = lcu.l_num;
}  /* ends extract_ulong_from_buffer */

void extract_short_from_buffer(char buffer[], int lsb, int start, short* number)
{
    union short_char_union lcu;

    if(lsb == 1){
        lcu.s_alpha[0] = buffer[start+0];
        lcu.s_alpha[1] = buffer[start+1];
    }  /* ends if lsb = 1 */

    if(lsb == 0){
        lcu.s_alpha[0] = buffer[start+1];
        lcu.s_alpha[1] = buffer[start+0];
    }  /* ends if lsb = 0      */

    *number = lcu.s_num;
}  /* ends extract_short_from_buffer */

void extract_ushort_from_buffer(char buffer[], int lsb, int start, unsigned short* number)
{
    union ushort_char_union lcu;

    if(lsb == 1){
        lcu.s_alpha[0] = buffer[start+0];
        lcu.s_alpha[1] = buffer[start+1];
    }  /* ends if lsb = 1 */

    if(lsb == 0){
        lcu.s_alpha[0] = buffer[start+1];
        lcu.s_alpha[1] = buffer[start+0];
    }  /* ends if lsb = 0      */

    *number = lcu.s_num;
}  /* ends extract_ushort_from_buffer */

void insert_short_into_buffer(char buffer[], int start, short number)
{
    union short_char_union lsu;

    lsu.s_num       = number;
    buffer[start+0] = lsu.s_alpha[0];
    buffer[start+1] = lsu.s_alpha[1];

}  /* ends insert_short_into_buffer */

void insert_ushort_into_buffer(char buffer[], int start, unsigned short number)
{
    union ushort_char_union lsu;

    lsu.s_num       = number;
    buffer[start+0] = lsu.s_alpha[0];
    buffer[start+1] = lsu.s_alpha[1];
}  /* ends insert_short_into_buffer */

void insert_long_into_buffer(char buffer[], int start, long number)  
{
    union long_char_union lsu;

    lsu.l_num       = number;
    buffer[start+0] = lsu.l_alpha[0];
    buffer[start+1] = lsu.l_alpha[1];
    buffer[start+2] = lsu.l_alpha[2];
    buffer[start+3] = lsu.l_alpha[3];
}  /* ends insert_short_into_buffer */

void insert_ulong_into_buffer(char buffer[], int start, unsigned long number)
{
    union ulong_char_union lsu;

    lsu.l_num       = number;
    buffer[start+0] = lsu.l_alpha[0];
    buffer[start+1] = lsu.l_alpha[1];
    buffer[start+2] = lsu.l_alpha[2];
    buffer[start+3] = lsu.l_alpha[3];
}  /* ends insert_ulong_into_buffer */

int does_not_exist(const char* file_name)
{
    FILE *image_file;
    int  result;

    result = 1;
    image_file = fopen(file_name, "rb");
    if(image_file != NULL){
        result = 0;
        fclose(image_file);
    }
    return(result);
}  /* ends does_not_exist */

