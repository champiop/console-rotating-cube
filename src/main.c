#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define PI 3.141592653589793238462643383279502884197

// Faces colors
#define BLACK "\033[0;30m"
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define PURPLE "\033[0;35m"
#define CYAN "\033[0;36m"

const char CHARS[] = {' ', '*', '#', '@'};

#define SWAP(x, y) \
    {              \
        x = x + y; \
        y = x - y; \
        x = x - y; \
    }

typedef struct Vec3
{
    float x;
    float y;
    float z;
    float w;
} Vec3;

typedef struct Triangle
{
    Vec3 vertices[3];
    int color;
} Triangle;

typedef struct Matrix
{
    float m[4][4];
} Matrix;

typedef struct Screen
{
    int width;
    int height;
    char *buffer;
    float *z_buffer;
    float far;
} Screen;

Vec3 ApplyMatrix(Matrix m, Vec3 v)
{
    Vec3 out;
    out.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w;
    out.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w;
    out.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w;
    out.w = m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w;

    if (out.w != 0)
    {
        out.x /= out.w;
        out.y /= out.w;
        out.z /= out.w;
        out.w /= out.w;
    }

    return out;
}

Matrix createZeroMatrix()
{
    Matrix m;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            m.m[i][j] = 0.0f;
        }
    }

    return m;
}

Matrix createProjectionMatrix(float near, float far, float fov, float aspect_ratio)
{
    Matrix m = createZeroMatrix();
    float fov_rad = 1.0f / tanf(fov * 0.5 / 180.0f * PI);

    m.m[0][0] = aspect_ratio * fov_rad;
    m.m[1][1] = fov_rad;
    m.m[2][2] = far / (far - near);
    m.m[2][3] = (-far * near) / (far - near);
    m.m[3][2] = 1.0f;

    return m;
}

Matrix createRotationXMatrix(float a)
{
    Matrix m = createZeroMatrix();

    m.m[0][0] = 1.0f;
    m.m[1][1] = cosf(a);
    m.m[1][2] = -sinf(a);
    m.m[2][1] = sinf(a);
    m.m[2][2] = cosf(a);
    m.m[3][3] = 1.0f;

    return m;
}

Matrix createRotationYMatrix(float a)
{
    Matrix m = createZeroMatrix();

    m.m[1][1] = 1.0f;
    m.m[0][0] = cosf(a);
    m.m[0][2] = -sinf(a);
    m.m[2][0] = sinf(a);
    m.m[2][2] = cosf(a);
    m.m[3][3] = 1.0f;

    return m;
}

Matrix createRotationZMatrix(float a)
{
    Matrix m = createZeroMatrix();

    m.m[2][2] = 1.0f;
    m.m[0][0] = cosf(a);
    m.m[0][1] = -sinf(a);
    m.m[1][0] = sinf(a);
    m.m[1][1] = cosf(a);
    m.m[3][3] = 1.0f;

    return m;
}

float crossProductX(Vec3 v1,
                    Vec3 v2)
{
    return v1.y * v2.z - v1.z * v2.y;
}

float crossProductY(Vec3 v1,
                    Vec3 v2)
{
    return v1.z * v2.x - v1.x * v2.z;
}

float crossProductZ(Vec3 v1,
                    Vec3 v2)
{
    return v1.x * v2.y - v1.y * v2.x;
}

void drawPixel(Screen s, int x, int y, float z, int color)
{
    if (x >= 0 && x < s.width && y >= 0 && y < s.height)
    {
        if (z <= s.z_buffer[y * s.width + x])
        {
            s.z_buffer[y * s.width + x] = z;
            s.buffer[y * s.width + x] = color;
        }
    }
}

void drawLine(Screen s, int x0, int y0, float z0, int x1, int y1, float z1, int color)
{
    int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;
    dx = x1 - x0;
    dy = y1 - y0;
    dx1 = abs(dx);
    dy1 = abs(dy);
    px = 2 * dy1 - dx1;
    py = 2 * dx1 - dy1;

    float z, zs, ze, t;
    t = 0;

    if (dy1 <= dx1)
    {
        if (dx >= 0)
        {
            x = x0;
            y = y0;
            zs = z0;
            ze = z1;
            xe = x1;
        }
        else
        {
            x = x1;
            y = y1;
            zs = z1;
            ze = z0;
            xe = x0;
        }

        drawPixel(s, x, y, zs, color);

        for (i = 0; x < xe; i++)
        {
            x++;
            t += 1.0f / dx1;
            if (px < 0)
            {
                px = px + 2 * dy1;
            }
            else
            {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                    y++;
                else
                    y--;

                px = px + 2 * (dy1 - dx1);
            }
            z = (1.0f / zs) * (1.0f - t) + (1.0f / ze) * t;
            drawPixel(s, x, y, 1.0f / z, color);
        }
    }
    else
    {
        if (dy >= 0)
        {
            x = x0;
            y = y0;
            zs = z0;
            ze = z1;
            ye = y1;
        }
        else
        {
            x = x1;
            y = y1;
            zs = z1;
            ze = z0;
            ye = y0;
        }

        drawPixel(s, x, y, zs, color);

        for (i = 0; y < ye; i++)
        {
            y++;
            t += 1.0f / dy1;
            if (py < 0)
            {
                py = py + 2 * dx1;
            }
            else
            {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                    x++;
                else
                    x--;

                py = py + 2 * (dx1 - dy1);
            }
            z = (1.0f / zs) * (1.0f - t) + (1.0f / ze) * t;
            drawPixel(s, x, y, 1.0f / z, color);
        }
    }
}

void drawTriangle(Screen s, Triangle t)
{
    drawLine(s, t.vertices[0].x, t.vertices[0].y, t.vertices[0].z, t.vertices[1].x, t.vertices[1].y, t.vertices[1].z, t.color);
    drawLine(s, t.vertices[1].x, t.vertices[1].y, t.vertices[1].z, t.vertices[2].x, t.vertices[2].y, t.vertices[2].z, t.color);
    drawLine(s, t.vertices[2].x, t.vertices[2].y, t.vertices[2].z, t.vertices[0].x, t.vertices[0].y, t.vertices[0].z, t.color);
}

void fillTriangle(Screen s, Triangle t)
{
    int x0 = t.vertices[0].x;
    int y0 = t.vertices[0].y;
    float z0 = t.vertices[0].z;
    int x1 = t.vertices[1].x;
    int y1 = t.vertices[1].y;
    float z1 = t.vertices[1].z;
    int x2 = t.vertices[2].x;
    int y2 = t.vertices[2].y;
    float z2 = t.vertices[2].z;

    if (y0 > y1)
    {
        SWAP(y0, y1);
        SWAP(x0, x1);
        SWAP(z0, z1);
    }
    if (y0 > y2)
    {
        SWAP(y0, y2);
        SWAP(x0, x2);
        SWAP(z0, z2);
    }
    if (y1 > y2)
    {
        SWAP(y1, y2);
        SWAP(x1, x2);
        SWAP(z1, z2);
    }

    int x_1, dx_1, dy_1, dx1_1, dy1_1, px_1, py_1, dirx_1, changed_1;
    int x_2, dx_2, dy_2, dx1_2, dy1_2, px_2, py_2, dirx_2, changed_2;
    int y;

    dx_1 = x1 - x0;
    dy_1 = y1 - y0;
    dx1_1 = abs(dx_1);
    dy1_1 = abs(dy_1);
    px_1 = 2 * dy1_1 - dx1_1;
    py_1 = 2 * dx1_1 - dy1_1;
    dirx_1 = dx_1 >= 0 ? 1 : -1;
    changed_1 = 0;

    dx_2 = x2 - x0;
    dy_2 = y2 - y0;
    dx1_2 = abs(dx_2);
    dy1_2 = abs(dy_2);
    px_2 = 2 * dy1_2 - dx1_2;
    py_2 = 2 * dx1_2 - dy1_2;
    dirx_2 = dx_2 >= 0 ? 1 : -1;
    changed_2 = 0;

    x_1 = x0;
    x_2 = x0;

    float z_1, z_2, t_1, t_2;
    t_1 = 0;
    t_2 = 0;

    for (y = y0; y < y1; y++)
    {
        while (!changed_1)
        {
            if (dy1_1 <= dx1_1)
            {
                if (px_1 < 0)
                {
                    px_1 = px_1 + 2 * dy1_1;
                }
                else
                {
                    px_1 = px_1 + 2 * (dy1_1 - dx1_1);
                    changed_1 = 1;
                }
                z_1 = (1.0f / z0) * (1.0f - t_1) + (1.0f / z1) * t_1;
                t_1 += 1.0f / dx1_1;
                x_1 += dirx_1;
            }
            else
            {
                if (py_1 < 0)
                {
                    py_1 = py_1 + 2 * dx1_1;
                }
                else
                {
                    x_1 += dirx_1;

                    py_1 = py_1 + 2 * (dx1_1 - dy1_1);
                }
                z_1 = (1.0f / z0) * (1.0f - t_1) + (1.0f / z1) * t_1;
                t_1 += 1.0f / dy1_1;
                changed_1 = 1;
            }
        }

        while (!changed_2)
        {
            if (dy1_2 <= dx1_2)
            {
                if (px_2 < 0)
                {
                    px_2 = px_2 + 2 * dy1_2;
                }
                else
                {
                    px_2 = px_2 + 2 * (dy1_2 - dx1_2);
                    changed_2 = 1;
                }
                z_2 = (1.0f / z0) * (1.0f - t_2) + (1.0f / z2) * t_2;
                t_2 += 1.0f / dx1_2;
                x_2 += dirx_2;
            }
            else
            {
                if (py_2 < 0)
                {
                    py_2 = py_2 + 2 * dx1_2;
                }
                else
                {
                    x_2 += dirx_2;

                    py_2 = py_2 + 2 * (dx1_2 - dy1_2);
                }
                z_2 = (1.0f / z0) * (1.0f - t_2) + (1.0f / z2) * t_2;
                t_2 += 1.0f / dy1_2;
                changed_2 = 1;
            }
        }

        changed_1 = 0;
        changed_2 = 0;
        drawLine(s, x_1, y, 1.0f / z_1, x_2, y, 1.0f / z_2, t.color);
    }

    dx_1 = x2 - x1;
    dy_1 = y2 - y1;
    dx1_1 = abs(dx_1);
    dy1_1 = abs(dy_1);
    px_1 = 2 * dy1_1 - dx1_1;
    py_1 = 2 * dx1_1 - dy1_1;
    dirx_1 = dx_1 >= 0 ? 1 : -1;

    x_1 = x1;

    t_1 = 0;

    for (y = y1; y < y2; y++)
    {
        while (!changed_1)
        {
            if (dy1_1 <= dx1_1)
            {
                if (px_1 < 0)
                {
                    px_1 = px_1 + 2 * dy1_1;
                }
                else
                {
                    px_1 = px_1 + 2 * (dy1_1 - dx1_1);
                    changed_1 = 1;
                }
                z_1 = (1.0f / z1) * (1.0f - t_1) + (1.0f / z2) * t_1;
                t_1 += 1.0f / dx1_1;
                x_1 += dirx_1;
            }
            else
            {
                if (py_1 < 0)
                {
                    py_1 = py_1 + 2 * dx1_1;
                }
                else
                {
                    x_1 += dirx_1;

                    py_1 = py_1 + 2 * (dx1_1 - dy1_1);
                }
                z_1 = (1.0f / z1) * (1.0f - t_1) + (1.0f / z2) * t_1;
                t_1 += 1.0f / dy1_1;
                changed_1 = 1;
            }
        }

        while (!changed_2)
        {
            if (dy1_2 <= dx1_2)
            {
                if (px_2 < 0)
                {
                    px_2 = px_2 + 2 * dy1_2;
                }
                else
                {
                    px_2 = px_2 + 2 * (dy1_2 - dx1_2);
                    changed_2 = 1;
                }
                z_2 = (1.0f / z0) * (1.0f - t_2) + (1.0f / z2) * t_2;
                t_2 += 1.0f / dx1_2;
                x_2 += dirx_2;
            }
            else
            {
                if (py_2 < 0)
                {
                    py_2 = py_2 + 2 * dx1_2;
                }
                else
                {
                    x_2 += dirx_2;

                    py_2 = py_2 + 2 * (dx1_2 - dy1_2);
                }
                z_2 = (1.0f / z0) * (1.0f - t_2) + (1.0f / z2) * t_2;
                t_2 += 1.0f / dy1_2;
                changed_2 = 1;
            }
        }

        changed_1 = 0;
        changed_2 = 0;
        drawLine(s, x_1, y, 1.0f / z_1, x_2, y, 1.0f / z_2, t.color);
    }
}

void clear(Screen s)
{
    for (int y = 0; y < s.height; y++)
    {
        for (int x = 0; x < s.width; x++)
        {
            s.buffer[y * s.width + x] = 0;
            s.z_buffer[y * s.width + x] = s.far;
        }
    }
}

void display(Screen s)
{
    system("clear");
    for (int y = 0; y < s.height; y++)
    {
        for (int x = 0; x < s.width; x++)
        {
            char c = ']';
            switch (s.buffer[y * s.width + x])
            {
            case 0:
                printf("  ");
                break;

            case 1:
                printf("%s%c%c", RED, c, c);
                break;

            case 2:
                printf("%s%c%c", GREEN, c, c);
                break;

            case 3:
                printf("%s%c%c", BLUE, c, c);
                break;

            case 4:
                printf("%s%c%c", YELLOW, c, c);
                break;

            case 5:
                printf("%s%c%c", PURPLE, c, c);
                break;

            case 6:
                printf("%s%c%c", CYAN, c, c);
                break;

            default:
                break;
            }
        }
        printf("\n");
    }
}

int main()
{
    Screen screen;
    screen.width = 100;
    screen.height = 100;
    screen.buffer = malloc(screen.width * screen.height * sizeof(char));
    screen.z_buffer = malloc(screen.width * screen.height * sizeof(float));
    screen.far = 100.0f;
    clear(screen);

    Triangle cube[12] = {
        {{{-0.8f, -0.8f, -0.8f, 1.0f}, {-0.8f, 0.8f, -0.8f, 1.0f}, {0.8, 0.8, -0.8f, 1.0f}}, 1},
        {{{-0.8f, -0.8f, -0.8f, 1.0f}, {0.8, 0.8, -0.8f, 1.0f}, {0.8, -0.8f, -0.8f, 1.0f}}, 1},

        {{{0.8, -0.8f, -0.8f, 1.0f}, {0.8, 0.8, -0.8f, 1.0f}, {0.8, 0.8, 0.8, 1.0f}}, 2},
        {{{0.8, -0.8f, -0.8f, 1.0f}, {0.8, 0.8, 0.8, 1.0f}, {0.8, -0.8f, 0.8, 1.0f}}, 2},

        {{{0.8, -0.8f, 0.8, 1.0f}, {0.8, 0.8, 0.8, 1.0f}, {-0.8f, 0.8, 0.8, 1.0f}}, 3},
        {{{0.8, -0.8f, 0.8, 1.0f}, {-0.8f, 0.8, 0.8, 1.0f}, {-0.8f, -0.8f, 0.8, 1.0f}}, 3},

        {{{-0.8f, -0.8f, 0.8, 1.0f}, {-0.8f, 0.8, 0.8, 1.0f}, {-0.8f, 0.8, -0.8f, 1.0f}}, 4},
        {{{-0.8f, -0.8f, 0.8, 1.0f}, {-0.8f, 0.8, -0.8f, 1.0f}, {-0.8f, -0.8f, -0.8f, 1.0f}}, 4},

        {{{-0.8f, 0.8, -0.8f, 1.0f}, {-0.8f, 0.8, 0.8, 1.0f}, {0.8, 0.8, 0.8, 1.0f}}, 5},
        {{{-0.8f, 0.8, -0.8f, 1.0f}, {0.8, 0.8, 0.8, 1.0f}, {0.8, 0.8, -0.8f, 1.0f}}, 5},

        {{{0.8, -0.8f, 0.8, 1.0f}, {-0.8f, -0.8f, 0.8, 1.0f}, {-0.8f, -0.8f, -0.8f, 1.0f}}, 6},
        {{{0.8, -0.8f, 0.8, 1.0f}, {-0.8f, -0.8f, -0.8f, 1.0f}, {0.8, -0.8f, -0.8f, 1.0f}}, 6},
    };

    Matrix projection = createProjectionMatrix(0.1f, screen.far, 90.0f, (float)screen.height / (float)screen.width);

    Matrix rotationX, rotationZ;
    float a = 0.0f;
    float b = 0.0f;

    while (1)
    {
        clear(screen);

        rotationX = createRotationXMatrix(a);
        rotationZ = createRotationZMatrix(b);

        Triangle rotated = {{{0, 0, 0, 1.0f}, {0, 0, 0, 1.0f}, {0, 0, 0, 1.0f}}, 0};
        Triangle projected = {{{0, 0, 0, 1.0f}, {0, 0, 0, 1.0f}, {0, 0, 0, 1.0f}}, 0};
        Triangle translated = {{{0, 0, 0, 1.0f}, {0, 0, 0, 1.0f}, {0, 0, 0, 1.0f}}, 0};

        for (int j = 0; j < 12; j++)
        {
            rotated.color = cube[j].color;
            rotated.vertices[0] = ApplyMatrix(rotationX, cube[j].vertices[0]);
            rotated.vertices[1] = ApplyMatrix(rotationX, cube[j].vertices[1]);
            rotated.vertices[2] = ApplyMatrix(rotationX, cube[j].vertices[2]);
            rotated.vertices[0] = ApplyMatrix(rotationZ, rotated.vertices[0]);
            rotated.vertices[1] = ApplyMatrix(rotationZ, rotated.vertices[1]);
            rotated.vertices[2] = ApplyMatrix(rotationZ, rotated.vertices[2]);

            translated = rotated;
            translated.vertices[0].z = rotated.vertices[0].z + 4.0f;
            translated.vertices[1].z = rotated.vertices[1].z + 4.0f;
            translated.vertices[2].z = rotated.vertices[2].z + 4.0f;

            Vec3 normal, vec1, vec2;
            vec1.x = translated.vertices[1].x - translated.vertices[0].x;
            vec1.y = translated.vertices[1].y - translated.vertices[0].y;
            vec1.z = translated.vertices[1].z - translated.vertices[0].z;

            vec2.x = translated.vertices[2].x - translated.vertices[0].x;
            vec2.y = translated.vertices[2].y - translated.vertices[0].y;
            vec2.z = translated.vertices[2].z - translated.vertices[0].z;

            normal.x = crossProductX(vec1, vec2);
            normal.y = crossProductY(vec1, vec2);
            normal.z = crossProductZ(vec1, vec2);

            float l = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
            normal.x /= l;
            normal.y /= l;
            normal.z /= l;

            if (normal.z < 0)
            {
                projected.color = translated.color;
                projected.vertices[0] = ApplyMatrix(projection, translated.vertices[0]);
                projected.vertices[1] = ApplyMatrix(projection, translated.vertices[1]);
                projected.vertices[2] = ApplyMatrix(projection, translated.vertices[2]);

                projected.vertices[0].x = (projected.vertices[0].x + 1.0f) * 0.5 * (float)screen.width;
                projected.vertices[1].x = (projected.vertices[1].x + 1.0f) * 0.5 * (float)screen.width;
                projected.vertices[2].x = (projected.vertices[2].x + 1.0f) * 0.5 * (float)screen.width;
                projected.vertices[0].y = (projected.vertices[0].y + 1.0f) * 0.5 * (float)screen.height;
                projected.vertices[1].y = (projected.vertices[1].y + 1.0f) * 0.5 * (float)screen.height;
                projected.vertices[2].y = (projected.vertices[2].y + 1.0f) * 0.5 * (float)screen.height;

                fillTriangle(screen, projected);
            }

            a += 0.005f;
            b += 0.05f;
        }

        display(screen);

        sleep(1);
    }

    free(screen.buffer);
    free(screen.z_buffer);

    return 0;
}