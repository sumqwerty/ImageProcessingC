#include <stdio.h>
#include <stdlib.h>
#include "bmp.h"

#define MAGIC_VALUE         0x4D42
#define NUM_PLANE           1
#define COMPRESSION         0
#define NUM_COLORS          0
#define IMPORTANT_COLORS    0
#define BITS_PER_PIXEL      24
#define BITS_PER_BYTE       8

BMPImage *read_image(const char *filename, char **error);
void write_image(const char *filename, BMPImage *image, char **error);
FILE *_open_file(const char *filename, const char *mode);
void _handle_error(char **error, FILE *fp, BMPImage *image);
void _clean_up(FILE *fp, BMPImage *image, char **error);

BMPImage *read_bmp(FILE *fp, char **error);
bool write_bmp(FILE *fp, BMPImage *image, char **error);
bool check_bmp_header(BMPHeader *bmp_header, FILE *fp);
void free_bmp(BMPImage *image);
long _get_file_size(FILE *fp);
int _get_image_size_bytes(BMPHeader *bmp_header);
int _get_image_row_size_bytes(BMPHeader *bmp_header);
int _get_bytes_per_pixel(BMPHeader  *bmp_header);
int _get_padding(BMPHeader *bmp_header);
int _get_position_x_row(int x, BMPHeader *bmp_header);
bool _check(bool condition, char **error, const char *error_message);
char *_string_duplicate(const char *string);

BMPImage *read_bmp(FILE *fp, char **error)
{
    BMPImage *image = malloc(sizeof(*image));
    if (!_check(image != NULL, error, "Not enough memory"))
    {
        return NULL;
    }
    // Read header
    rewind(fp);
    int num_read = fread(&image->header, sizeof(image->header), 1, fp);
    if(!_check(num_read == 1, error, "Cannot read header"))
    {
        return NULL;
    }
    // Check header
    bool is_valid_header = check_bmp_header(&image->header, fp);
    if(!_check(is_valid_header, error, "Invalid BMP file !!!!!!!"))
    {
        return NULL;
    }
    // Allocate memory for image data
    image->data = malloc(sizeof(*image->data) * image->header.image_size_bytes);
    if (!_check(image->data != NULL, error, "Not enough memory"))
    {
        return NULL;
    }
    // Read image data
    num_read = fread(image->data, image->header.image_size_bytes, 1, fp);
    if (!_check(num_read == 1, error, "Cannot read image"))
    {
        return NULL;
    }

    return image;
}

bool write_bmp(FILE *fp, BMPImage *image, char **error)
{
    // Write header
    rewind(fp);
    int num_read = fwrite(&image->header, sizeof(image->header), 1, fp);
    if (!_check(num_read == 1, error, "Cannot write image"))
    {
        return false;
    }
    // Write image data
    num_read = fwrite(image->data, image->header.image_size_bytes, 1, fp);
    if (!_check(num_read == 1, error, "Cannot write image"))
    {
        return false;
    }

    return true;
}


bool check_bmp_header(BMPHeader* bmp_header, FILE* fp)
{

    /*printf("%d : %d : %d : %d : %d : %d : %d : %d : %d : %d \n",
           bmp_header->type,
           bmp_header->offset,
           bmp_header->dib_header_size,
           bmp_header->num_planes,
           bmp_header->compression,
           bmp_header->num_colors,
           bmp_header->important_colors,
           bmp_header->bits_per_pixel,
           bmp_header->size,
           bmp_header->image_size_bytes);*/

    return //true;
        bmp_header->type == MAGIC_VALUE
        && bmp_header->num_planes == NUM_PLANE
        && bmp_header->compression == COMPRESSION
        && bmp_header->num_colors == NUM_COLORS
        && bmp_header->important_colors == IMPORTANT_COLORS
        && bmp_header->bits_per_pixel == BITS_PER_PIXEL;
        //&& bmp_header->size == _get_file_size(fp)
        //&& bmp_header->image_size_bytes == _get_image_size_bytes(bmp_header);
}

void free_bmp(BMPImage *image)
{
    free(image->data);
    free(image);
}

///////////////////////////////////////////////////////////////////////////

int *get_pixel(BMPImage *image, int x, int y, char **error)
{
    if (!_check(x <= image->header.width_px && y <= image->header.height_px,error,
                "The point is out of the image."))
    {
        return NULL;
    }
    int position_y = y * _get_image_row_size_bytes(&image->header);
    int position_x_row = _get_position_x_row(x, &image->header);
    static int colour[3];
    colour[0] = image->data[position_y + position_x_row];
    colour[1] = image->data[position_y + position_x_row + 1];
    colour[2] = image->data[position_y + position_x_row + 2];
    //printf("%d : %d : %d\n", colour[0],colour[1],colour[2]);
    return colour;
}

void set_pixel(BMPImage *image, int x, int y, int B, int G, int R, char **error)
{
    if (!_check(x <= image->header.width_px && y <= image->header.height_px,error,
                "The point is out of the image."))
    {
        return;
    }
    int position_y = y * _get_image_row_size_bytes(&image->header);
    int position_x_row = _get_position_x_row(x, &image->header);
    image->data[position_y + position_x_row + 0] = B;
    image->data[position_y + position_x_row + 1] = G;
    image->data[position_y + position_x_row + 2] = R;
}

void set_pixel2(BMPImage *image, int x, int y, int B, int G, int R, char **error)
{
    if (!_check(x <= image->header.width_px && y <= image->header.height_px,error,
                "The point is out of the image."))
    {
        return;
    }
    int position_y = y * _get_image_row_size_bytes(&image->header);
    int position_x_row = _get_position_x_row(x, &image->header);
    image->data[position_y + position_x_row + 0] = B;
    image->data[position_y + position_x_row + 1] = G;
    image->data[position_y + position_x_row + 2] = R;

}


BMPImage *reflectImage(BMPImage *image, char flag, char **error)
{
    int x = 0;
    int y = 0;
    int w = _get_image_width_px(&image->header);
    int h = _get_image_height_px(&image->header);

    BMPImage *nw_image = malloc(sizeof(*nw_image));
    if (!_check(x + w <= image->header.width_px && y + h <= image->header.height_px,error,
                "The size of the new image should be equal or less than the size of the original"))
    {
        return NULL;
    }
    // Update new_image header
    nw_image->header = image->header;
    nw_image->header.width_px = w;
    nw_image->header.height_px = h;
    nw_image->header.image_size_bytes = _get_image_size_bytes(&nw_image->header);
    nw_image->header.size = BMP_HEADER_SIZE + nw_image->header.image_size_bytes;
    // Allocate memory for image data
    nw_image->data = malloc(sizeof(*nw_image->data) * nw_image->header.image_size_bytes);
    if(!_check(nw_image->data != NULL, error, "Not enough memory"))
    {
        return NULL;
    }
    // Iterate image's columns

    for(y=0; y<h; ++y)
    {
        for(x=0; x<w; ++x)
        {
            if(flag == 'h')set_pixel(nw_image,x,y,get_pixel(image,(w-x),y,&error)[0],get_pixel(image,(w-x),y,&error)[1],get_pixel(image,(w-x),y,&error)[2],&error);

            else set_pixel(nw_image,x,y,get_pixel(image,x,(h-y),&error)[0],get_pixel(image,x,(h-y),&error)[1],get_pixel(image,x,(h-y),&error)[2],&error);
        }
    }

    return nw_image;
}

BMPImage *crop_bmp(BMPImage *image, int x, int y, int w, int h, char **error)
{
    BMPImage *new_image = malloc(sizeof(*new_image));
    if (!_check(x + w <= image->header.width_px && y + h <= image->header.height_px,error,
                "The size of the new image should be equal or less than the size of the original"))
    {
        return NULL;
    }
    // Update new_image header
    new_image->header = image->header;
    new_image->header.width_px = w;
    new_image->header.height_px = h;
    new_image->header.image_size_bytes = _get_image_size_bytes(&new_image->header);
    new_image->header.size = BMP_HEADER_SIZE + new_image->header.image_size_bytes;
    // Allocate memory for image data
    new_image->data = malloc(sizeof(*new_image->data) * new_image->header.image_size_bytes);
    if(!_check(new_image->data != NULL, error, "Not enough memory"))
    {
        return NULL;
    }
    int position_y = y * _get_image_row_size_bytes(&image->header);
    int position_x_row = _get_position_x_row(x, &image->header);
    int new_index = 0;
    // Iterate image's columns
    for (int i = 0; i < h; i++)
    {
        // Iterate image's rows
        for (int j = 0; j < w; j++)
        {
            // Iterate image's pixels
            for(int k = 0; k < 3; k++)
            {
                new_image->data[new_index] = image->data[position_y + position_x_row];
                new_index++;
                position_x_row++;
            }
        }
        // Add padding to new_image
        int padding = _get_padding(&new_image->header);
        for (int l = 0; l < padding; l++)
        {
            new_image->data[new_index] = 0x00;
            new_index++;
        }
        position_y += _get_image_row_size_bytes(&image->header);
        position_x_row = _get_position_x_row(x, &image->header);
    }

    return new_image;
}

void fill_region(BMPImage *image, int x, int y, int w, int h, int B, int G, int R, char **error)
{
    BMPImage *new_image = malloc(sizeof(*new_image));
    if (!_check(x + w <= image->header.width_px && y + h <= image->header.height_px,error,
                "The region is out of the image."))
    {
        return;
    }
    int position_y = y * _get_image_row_size_bytes(&image->header);
    int position_x_row = _get_position_x_row(x, &image->header);
    int new_index = 0;
    // Iterate image's columns
    for (int i = y; i < y+h; i++)
    {
        // Iterate image's rows
        for (int j = x; j < x+w; j++)
        {
            set_pixel(image,j,i,B,G,R,&error);
        }
    }
}

int brightness(int B, int G, int R)
{
    return ((B+G+R)/3);
}

void binaryImage(BMPImage *image, int threshHold,char **error)
{
    for(int y=0; y< _get_image_height_px(&image->header); ++y)
    {
        for(int x=0; x < _get_image_width_px(&image->header); ++x)
        {
            int grayBgr = brightness(get_pixel(image,x,y,error)[0],get_pixel(image,x,y,error)[1],get_pixel(image,x,y,error)[2]);
            if(grayBgr < threshHold)
            {
                int col = 0;
                set_pixel(image,x,y,col,col,col,&error);
            }
            else
            {
                int col = 255;
                set_pixel(image,x,y,col,col,col,&error);
            }

        }
    }
}


int avgGrayLevel(BMPImage *image, char **error)
{
    int sum = 0;
    for(int y=0; y< _get_image_height_px(&image->header); ++y)
    {
        for(int x=0; x < _get_image_width_px(&image->header); ++x)
        {
            int grayBgr = brightness(get_pixel(image,x,y,error)[0],get_pixel(image,x,y,error)[1],get_pixel(image,x,y,error)[2]);
            sum += grayBgr;
        }
    }
    sum = (sum / (_get_image_height_px(&image->header) * _get_image_width_px(&image->header)));
    return sum;
}

void imgToGrayscale(BMPImage *image, char **error)
{
    for(int y=0; y< _get_image_height_px(&image->header); ++y)
    {
        for(int x=0; x < _get_image_width_px(&image->header); ++x)
        {
            int grayBgr = brightness(get_pixel(image,x,y,error)[0],get_pixel(image,x,y,error)[1],get_pixel(image,x,y,error)[2]);
            set_pixel(image,x,y,grayBgr,grayBgr,grayBgr,&error);
        }
    }
}

void imgToNegative(BMPImage *image, char **error)
{
    for(int y=0; y< _get_image_height_px(&image->header); ++y)
    {
        for(int x=0; x < _get_image_width_px(&image->header); ++x)
        {
            //int grayBgr = brightness(get_pixel(image,x,y,error)[0],get_pixel(image,x,y,error)[1],get_pixel(image,x,y,error)[2]);
            int invBlue = (255 - get_pixel(image,x,y,error)[0]);
            int invGreen = (255 - get_pixel(image,x,y,error)[1]);
            int invRed = (255 - get_pixel(image,x,y,error)[2]);
            set_pixel(image,x,y,invBlue,invGreen,invRed,&error);
        }
    }
}


void edgeFilter(BMPImage *image, int detail, char **error)
{
    for(int y=0; y< _get_image_height_px(&image->header); ++y)
    {
        for(int x=0; x < _get_image_width_px(&image->header); ++x)
        {
            int grayBgr1 = brightness(get_pixel(image,x,y,error)[0],get_pixel(image,x,y,error)[1],get_pixel(image,x,y,error)[2]);
            int grayBgr2 = brightness(get_pixel(image,(x+1),y,error)[0],get_pixel(image,(x+1),y,error)[1],get_pixel(image,(x+1),y,error)[2]);
            int diff = abs(grayBgr1-grayBgr2);
            if(diff > detail)set_pixel(image,x,y,255,255,255,&error);
            else set_pixel(image,x,y,0,0,0,&error);
        }
    }
}


void imageInfo(BMPImage *image)
{
    printf("Width: %d \n", _get_image_width_px(&image->header));
    printf("Height: %d \n", _get_image_height_px(&image->header));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

long _get_file_size(FILE *fp)
{
    // Get current file position
    long current_position = ftell(fp);
    if (current_position == -1)
    {
        return -1;
    }
    // Set file position to the end
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        return -2;
    }
    // Get current file position (now at the end)
    long file_size = ftell(fp);
    if (file_size == -1)
    {
        return -3;
    }
    // Restore previous file position
    if (fseek(fp, current_position, SEEK_SET) != 0)
    {
        return -4;
    }

    return file_size;
}


int _get_image_width_px(BMPHeader *bmp_header)
{
    return bmp_header->width_px;
}

int _get_image_height_px(BMPHeader *bmp_header)
{
    return bmp_header->height_px;
}

int _get_image_size_bytes(BMPHeader *bmp_header)
{
    return _get_image_row_size_bytes(bmp_header) * bmp_header->height_px;
}


int _get_image_row_size_bytes(BMPHeader *bmp_header)
{
    int bytes_per_row_without_padding = bmp_header->width_px * _get_bytes_per_pixel(bmp_header);
    return bytes_per_row_without_padding + _get_padding(bmp_header);
}

int _get_padding(BMPHeader *bmp_header)
{
    return (4 - (bmp_header->width_px * _get_bytes_per_pixel(bmp_header)) % 4) % 4;
}


int _get_bytes_per_pixel(BMPHeader  *bmp_header)
{
    return bmp_header->bits_per_pixel / BITS_PER_BYTE;
}

int _get_position_x_row(int x, BMPHeader *bmp_header)
{
    return x * _get_bytes_per_pixel(bmp_header);
}

bool _check(bool condition, char **error, const char *error_message)
{
    bool is_valid = true;
    if(!condition)
    {
        is_valid = false;
        if (*error == NULL)  // to avoid memory leaks
        {
            *error = _string_duplicate(error_message);
        }
    }
    return is_valid;
}

char *_string_duplicate(const char *string)
{
    char *copy = malloc(sizeof(*copy) * (strlen(string) + 1));
    if (copy == NULL)
    {
        return "Not enough memory for error message";
    }
    strcpy(copy, string);

    return copy;
}

BMPImage *read_image(const char *filename, char **error)
{
    FILE *input_ptr = _open_file(filename, "rb");
    BMPImage *image = read_bmp(input_ptr, error);

    if (*error != NULL)
    {
        printf("%s\n",*error);
        _handle_error(error, input_ptr, image);
    }
    fclose(input_ptr);

    return image;
}

void write_image(const char *filename, BMPImage *image, char **error)
{
    FILE *output_ptr = _open_file(filename, "wb");

    if (!write_bmp(output_ptr, image, error))
    {
        _handle_error(error, output_ptr, image);
    }
    fclose(output_ptr);
}

FILE *_open_file(const char *filename, const char *mode)
{
    FILE *fp = fopen(filename, mode);
    if (fp == NULL)
    {
        fprintf(stderr, "Could not open file %s", filename);
        exit(EXIT_FAILURE);
    }

    return fp;
}

void _handle_error(char **error, FILE *fp, BMPImage *image)
{
    fprintf(stderr, "ERROR: %s\n", *error);
    _clean_up(fp, image, error);

    exit(EXIT_FAILURE);
}

void _clean_up(FILE *fp, BMPImage *image, char **error)
{
    if (fp != NULL)
    {
        fclose(fp);
    }
    free_bmp(image);
    free(*error);
}


int main(void)
{
    char *error = NULL;

    //BMPImage *image1 = read_image("t1_24.bmp", &error);
    //BMPImage *image2 = read_image("3.bmp", &error);
    BMPImage *image3 = read_image("frog_1.bmp", &error);

    //imageInfo
    //imageInfo(image1);
    //imageInfo(image2);
    imageInfo(image3);


    //setting pixel
    //set_pixel2(image1,10,10,255,0,255,&error); //not visible because its only 1 pixel
    //for(int i=0; i<100; ++i) set_pixel2(image2,i,10,255,100,255,&error); //setting 100 pixels
    //fill_region(image3,0,0,100,100,255,0,0,&error);
    //write_image("set_pix.bmp", image2, &error);

    //getting pixel
    //int *pix = get_pixel(image1,0,0,&error);
    //printf("%d : %d : %d\n", pix[0],pix[1],pix[2]);

    //greayScale
    //printf("Average greayLevel: %d\n", avgGrayLevel(image1,&error));
    //printf("Average greayLevel: %d\n", avgGrayLevel(image2,&error));
    //imgToGrayscale(image1,&error);
    //imgToGrayscale(image2,&error);
    //imgToGrayscale(image3,&error);
    //write_image("grey2.bmp", image1, &error);
    //write_image("grey2.bmp", image2, &error);
    //write_image("grey.bmp", image3, &error);

    //binary image
    //binaryImage(image1,avgGrayLevel(image1,&error),&error);
    //binaryImage(image2,avgGrayLevel(image2,&error),&error);
    binaryImage(image3,avgGrayLevel(image3,&error),&error);
    //write_image("binary.bmp", image1, &error);
    //write_image("binary.bmp", image2, &error);
    write_image("binary.bmp", image3, &error);


    //cropping
    //BMPImage *crop_image = crop_bmp(image1, 0, 0, 50, 50, &error);
    //BMPImage *crop_image = crop_bmp(image2, 100, 100, 50, 50, &error);
    //write_image("crop.bmp", crop_image, &error);

    //refection
    //BMPImage *ref = reflectImage(image1, 'h', &error);
    //BMPImage *ref = reflectImage(image3, 'v', &error);
    //write_image("ref2.bmp", ref, &error);

    //Negative
    //imgToNegative(image2,&error);
    //write_image("negate.bmp",image2,&error);
    //imgToNegative(image3, &error);
    //write_image("negate_frog.bmp", image3, &error);


    //Edge Filter
    //edgeFilter(image1, 1, &error);
    //write_image("edge.bmp", image1, &error);
    //edgeFilter(image2, 10, &error);
    //write_image("edge.bmp", image2, &error);
    //edgeFilter(image3, 10, &error);
    //write_image("edge.bmp", image3, &error);


    //_clean_up(NULL, image1, &error);
    //_clean_up(NULL, image2, &error);
    _clean_up(NULL, image3, &error);
    //_clean_up(NULL, crop_image, &error);


    return EXIT_SUCCESS;
}
