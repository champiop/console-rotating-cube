#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define PI 3.141592653589793238462643383279502884197

// Background color
#define BLKB "\e[40m \e[0m"
// Faces colors
#define REDHB "\e[0;101m \e[0m"
#define GRNHB "\e[0;102m \e[0m"
#define YELHB "\e[0;103m \e[0m"
#define BLUHB "\e[0;104m \e[0m"
#define MAGHB "\e[0;105m \e[0m"
#define CYNHB "\e[0;106m \e[0m"

typedef struct Vector3f
{
    float x;
    float y;
    float z;
} Vector3f;

typedef struct Triangle
{
    Vector3f vertices[3];
    int color;
} Triangle;

typedef struct Screen
{
    int width;
    int height;
    int renderDistance;
    int *buffer;
    float *z_buffer;
} Screen;

Screen *createScreen(int width, int height)
{
    Screen *s = (Screen *)malloc(sizeof(Screen));
    s->width = width;
    s->height = height;
    s->renderDistance = 100;
    s->buffer = (int *)calloc(width * height, sizeof(int));
    s->z_buffer = (float *)calloc(width * height, sizeof(float));

    return s;
}

void freeScreen(Screen *s)
{
    free(s->buffer);
    free(s->z_buffer);
    free(s);
}

float rotateX(float A, float B, float C,
              float x, float y, float z)
{
    return ((cosf(B + C) + cosf(B - C)) / 2) * x +
           ((-sinf(B + C) + sinf(B - C)) / 2) * y +
           sinf(B) * z;
}

float rotateY(float A, float B, float C,
              float x, float y, float z)
{
    return ((-cosf(A + B + C) + cosf(A - B + C) - cosf(A + B - C) + cosf(A - B - C) + 2 * sinf(A + C) - 2 * sinf(A - C)) / 4) * x +
           ((2 * cosf(A + C) + 2 * cosf(A - C) + sinf(A + B + C) - sinf(A - B + C) - sinf(A + B - C) + sinf(A - B - C)) / 4) * y +
           ((-sinf(A + B) - sinf(A - B)) / 2) * z;
}

float rotateZ(float A, float B, float C,
              float x, float y, float z)
{
    return ((-2 * cosf(A + C) + 2 * cosf(A - C) - sinf(A + B + C) + sinf(A - B + C) - sinf(A + B - C) + sinf(A - B - C)) / 4) * x +
           ((-cosf(A + B + C) + cosf(A - B + C) + cosf(A + B - C) - cosf(A - B - C) + 2 * sinf(A + C) + 2 * sinf(A - C)) / 4) * y +
           ((cosf(A + B) + cosf(A - B)) / 2) * z;
}

void rotateTriangle(Triangle *t,
                    float A, float B, float C,
                    float center_x, float center_y, float center_z)
{
    for (int i = 0; i < 3; i++)
    {
        float x = t->vertices[i].x - center_x;
        float y = t->vertices[i].y - center_y;
        float z = t->vertices[i].z - center_z;
        t->vertices[i].x = rotateX(A, B, C, x, y, z) + center_x;
        t->vertices[i].y = rotateY(A, B, C, x, y, z) + center_y;
        t->vertices[i].z = rotateZ(A, B, C, x, y, z) + center_z;
    }
}

float crossProductX(Vector3f v1,
                    Vector3f v2)
{
    return v1.y * v2.z - v1.z * v2.y;
}

float crossProductY(Vector3f v1,
                    Vector3f v2)
{
    return v1.z * v2.x - v1.x * v2.z;
}

float crossProductZ(Vector3f v1,
                    Vector3f v2)
{
    return v1.x * v2.y - v1.y * v2.x;
}

void clearScreen(Screen *s)
{
    for (int y = 0; y < s->height; y++)
    {
        for (int x = 0; x < s->width; x++)
        {
            s->buffer[x + s->width * y] = 0;
            s->z_buffer[x + s->width * y] = -s->renderDistance;
        }
    }
}

int sameSign(float x, float y)
{
    return (x > 0 && y > 0) || (x < 0 && y < 0);
}

// Rasterization
void renderTriangle(Screen *s, Triangle t)
{
    for (int y = -s->height / 2; y < s->height / 2; y++)
    {
        for (int x = -s->width / 2; x < s->width / 2; x++)
        {
            Vector3f v0 = (Vector3f){t.vertices[1].x - t.vertices[0].x,
                                     t.vertices[1].y - t.vertices[0].y,
                                     0};
            Vector3f p0 = (Vector3f){x - t.vertices[0].x,
                                     y - t.vertices[0].y,
                                     0};

            Vector3f v1 = (Vector3f){t.vertices[2].x - t.vertices[1].x,
                                     t.vertices[2].y - t.vertices[1].y,
                                     0};
            Vector3f p1 = (Vector3f){x - t.vertices[1].x,
                                     y - t.vertices[1].y,
                                     0};

            Vector3f v2 = (Vector3f){t.vertices[0].x - t.vertices[2].x,
                                     t.vertices[0].y - t.vertices[2].y,
                                     0};
            Vector3f p2 = (Vector3f){x - t.vertices[2].x,
                                     y - t.vertices[2].y,
                                     0};

            if (sameSign(crossProductZ(v0, p0), crossProductZ(v1, p1)) &&
                sameSign(crossProductZ(v1, p1), crossProductZ(v2, p2)))
            {
                v0 = (Vector3f){t.vertices[1].x - t.vertices[0].x,
                                t.vertices[1].y - t.vertices[0].y,
                                t.vertices[1].z - t.vertices[0].z};
                v1 = (Vector3f){t.vertices[2].x - t.vertices[1].x,
                                t.vertices[2].y - t.vertices[1].y,
                                t.vertices[2].z - t.vertices[1].z};

                float z = -((crossProductX(v0, v1) * p0.x + crossProductY(v0, v1) * p0.y) / crossProductZ(v0, v1)) + t.vertices[0].z;

                if (z > s->z_buffer[x + s->width / 2 + s->width * (y + s->height / 2)])
                {
                    s->z_buffer[x + s->width / 2 + s->width * (y + s->height / 2)] = z;
                    s->buffer[x + s->width / 2 + s->width * (y + s->height / 2)] = t.color;
                }
            }
        }
    }
}

void displayScreen(Screen *s)
{
    system("clear");
    char *c;
    for (int y = 0; y < s->height; y++)
    {
        for (int x = 0; x < s->width; x++)
        {
            switch (s->buffer[x + s->width * y])
            {
            case 1:
                c = REDHB;
                break;

            case 2:
                c = CYNHB;
                break;

            case 3:
                c = GRNHB;
                break;

            case 4:
                c = YELHB;
                break;

            case 5:
                c = MAGHB;
                break;

            case 6:
                c = BLUHB;
                break;

            default:
                c = BLKB;
                break;
            }
            printf("%s%s", c, c);
        }
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
        exit(1);

    FILE *file = fopen(argv[1], "r");

    int width, height, count;
    fscanf(file, "%d", &width);
    fscanf(file, "%d", &height);
    fscanf(file, "%d", &count);

    Screen *screen = createScreen(width, height);
    Triangle triangles[count];

    // Reads vertices positions
    float x, y, z;
    int color;
    int i = 0;
    int j = 0;
    while (i < count)
    {
        fscanf(file, "%f", &x);
        fscanf(file, "%f", &y);
        fscanf(file, "%f", &z);
        fscanf(file, "%d", &color);

        triangles[i].vertices[j] = (Vector3f){x, y, z};

        j++;
        if (j % 3 == 0)
        {
            triangles[i].color = color;
            i++;
            j = 0;
        }
    }
    fclose(file);

    // Rotates and displays the cube
    float A = PI / 30;
    float B = PI / 40;
    float C = 0;
    for (int t = 0; t < 60; t++)
    {
        clearScreen(screen);
        for (i = 0; i < count; i++)
        {
            renderTriangle(screen, triangles[i]);
        }

        displayScreen(screen);

        for (i = 0; i < count; i++)
        {
            rotateTriangle(&triangles[i], A, B, C, 0, 0, 0);
        }

        sleep(1);
    }

    freeScreen(screen);
    return 0;
}