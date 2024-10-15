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

const char CHARS[] = {' ', '.', '\'', '`', '^', '"', ',', ':', ';', 'I', 'l', '!', 'i', '>', '<', '~', '+', '_', '-', '?', ']', '[', '}', '{', '1', ')', '(', '|', '\\', '/', 't', 'f', 'j', 'r', 'x', 'n', 'u', 'v', 'c', 'z', 'X', 'Y', 'U', 'J', 'C', 'L', 'Q', '0', 'O', 'Z', 'm', 'w', 'q', 'p', 'd', 'b', 'k', 'h', 'a', 'o', '*', '#', 'M', 'W', '&', '8', '%', 'B', '@', '$'};
const COLOR_COUNT = 70;

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

typedef struct Vertex
{
    Vec3 pos;
    float color;
} Vertex;

typedef struct Triangle
{
    Vertex vertices[3];
} Triangle;

typedef struct Matrix
{
    float m[4][4];
} Matrix;

typedef struct Screen
{
    int width;
    int height;
    int *buffer;
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

float interpolate(float start, float end, float t)
{
    return start * (1.0f - t) + end * t;
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

void drawLine(Screen s, int x0, int y0, float z0, int x1, int y1, float z1, int color0, int color1)
{
    int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i, cs, ce, c;
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
            cs = color0;
            ze = z1;
            xe = x1;
            ce = color1;
        }
        else
        {
            x = x1;
            y = y1;
            zs = z1;
            cs = color1;
            ze = z0;
            xe = x0;
            ce = color0;
        }

        drawPixel(s, x, y, zs, cs);

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
            z = interpolate(1 / zs, 1 / ze, t);
            c = interpolate(cs, ce, t);
            drawPixel(s, x, y, 1.0f / z, c);
        }
    }
    else
    {
        if (dy >= 0)
        {
            x = x0;
            y = y0;
            zs = z0;
            cs = color0;
            ze = z1;
            ye = y1;
            ce = color1;
        }
        else
        {
            x = x1;
            y = y1;
            zs = z1;
            cs = color1;
            ze = z0;
            ye = y0;
            ce = color0;
        }

        drawPixel(s, x, y, zs, cs);

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
            z = interpolate(1 / zs, 1 / ze, t);
            c = interpolate(cs, ce, t);
            drawPixel(s, x, y, 1.0f / z, c);
        }
    }
}

void drawTriangle(Screen s, Triangle t)
{
    drawLine(s, t.vertices[0].pos.x, t.vertices[0].pos.y, t.vertices[0].pos.z, t.vertices[1].pos.x, t.vertices[1].pos.y, t.vertices[1].pos.z, t.vertices[0].color, t.vertices[1].color);
    drawLine(s, t.vertices[1].pos.x, t.vertices[1].pos.y, t.vertices[1].pos.z, t.vertices[2].pos.x, t.vertices[2].pos.y, t.vertices[2].pos.z, t.vertices[1].color, t.vertices[2].color);
    drawLine(s, t.vertices[2].pos.x, t.vertices[2].pos.y, t.vertices[2].pos.z, t.vertices[0].pos.x, t.vertices[0].pos.y, t.vertices[0].pos.z, t.vertices[2].color, t.vertices[0].color);
}

void fillTriangle(Screen s, Triangle t)
{
    int x0 = t.vertices[0].pos.x;
    int y0 = t.vertices[0].pos.y;
    float z0 = t.vertices[0].pos.z;
    int c0 = t.vertices[0].color;
    int x1 = t.vertices[1].pos.x;
    int y1 = t.vertices[1].pos.y;
    float z1 = t.vertices[1].pos.z;
    int c1 = t.vertices[1].color;
    int x2 = t.vertices[2].pos.x;
    int y2 = t.vertices[2].pos.y;
    float z2 = t.vertices[2].pos.z;
    int c2 = t.vertices[2].color;

    if (y0 > y1)
    {
        SWAP(y0, y1);
        SWAP(x0, x1);
        SWAP(z0, z1);
        SWAP(c0, c1);
    }
    if (y0 > y2)
    {
        SWAP(y0, y2);
        SWAP(x0, x2);
        SWAP(z0, z2);
        SWAP(c0, c2);
    }
    if (y1 > y2)
    {
        SWAP(y1, y2);
        SWAP(x1, x2);
        SWAP(z1, z2);
        SWAP(c1, c2);
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

    float z_1, z_2, t_1, t_2, c_1, c_2;
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
                z_1 = interpolate(1 / z0, 1 / z1, t_1);
                c_1 = interpolate(c0, c1, t_1);
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
                z_1 = interpolate(1 / z0, 1 / z1, t_1);
                c_1 = interpolate(c0, c1, t_1);
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
                z_2 = interpolate(1 / z0, 1 / z2, t_2);
                c_2 = interpolate(c0, c2, t_2);
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
                z_2 = interpolate(1 / z0, 1 / z2, t_2);
                c_2 = interpolate(c0, c2, t_2);
                t_2 += 1.0f / dy1_2;
                changed_2 = 1;
            }
        }

        changed_1 = 0;
        changed_2 = 0;
        drawLine(s, x_1, y, 1.0f / z_1, x_2, y, 1.0f / z_2, c_1, c_2);
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
                z_1 = interpolate(1 / z1, 1 / z2, t_1);
                c_1 = interpolate(c1, c2, t_1);
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
                z_1 = interpolate(1 / z1, 1 / z2, t_1);
                c_1 = interpolate(c1, c2, t_1);
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
                z_2 = interpolate(1 / z0, 1 / z2, t_2);
                c_2 = interpolate(c0, c2, t_2);
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
                z_2 = interpolate(1 / z0, 1 / z2, t_2);
                c_2 = interpolate(c0, c2, t_2);
                t_2 += 1.0f / dy1_2;
                changed_2 = 1;
            }
        }

        changed_1 = 0;
        changed_2 = 0;
        drawLine(s, x_1, y, 1.0f / z_1, x_2, y, 1.0f / z_2, c_1, c_2);
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
            char c = CHARS[s.buffer[y * s.width + x]];
            printf("%c%c", c, c);
        }
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("usage: ./cube <path_to_geometry>");
        exit(1);
    }

    Screen screen;
    screen.width = 100;
    screen.height = 100;
    screen.buffer = malloc(screen.width * screen.height * sizeof(int));
    screen.z_buffer = malloc(screen.width * screen.height * sizeof(float));
    screen.far = 50.0f;
    clear(screen);

    FILE *file;
    file = fopen(argv[1], "r");

    int triangleCount;
    fscanf(file, "%d", &triangleCount);

    Triangle *geometry = malloc(triangleCount * sizeof(Triangle));
    for (int i = 0; i < triangleCount; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            fscanf(file, "%f %f %f %f", &geometry[i].vertices[j].pos.x, &geometry[i].vertices[j].pos.y, &geometry[i].vertices[j].pos.z, &geometry[i].vertices[j].color);
            geometry[i].vertices[j].pos.w = 1.0f;
        }
    }

    fclose(file);

    Matrix projection = createProjectionMatrix(0.1f, screen.far, 90.0f, (float)screen.height / (float)screen.width);

    Matrix rotationX, rotationZ;
    float a = 0.0f;
    float b = 0.0f;
    float cubeZ = 2.0f;

    while (1)
    {
        clear(screen);

        rotationX = createRotationXMatrix(a);
        rotationZ = createRotationZMatrix(b);

        Triangle rotated = {{{{0, 0, 0, 1.0f}, 0}, {{0, 0, 0, 1.0f}, 0}, {{0, 0, 0, 1.0f}, 0}}};
        Triangle projected = {{{{0, 0, 0, 1.0f}, 0}, {{0, 0, 0, 1.0f}, 0}, {{0, 0, 0, 1.0f}, 0}}};
        Triangle translated = {{{{0, 0, 0, 1.0f}, 0}, {{0, 0, 0, 1.0f}, 0}, {{0, 0, 0, 1.0f}, 0}}};

        for (int j = 0; j < triangleCount; j++)
        {
            rotated.vertices[0].color = geometry[j].vertices[0].color;
            rotated.vertices[1].color = geometry[j].vertices[1].color;
            rotated.vertices[2].color = geometry[j].vertices[2].color;
            rotated.vertices[0].pos = ApplyMatrix(rotationX, geometry[j].vertices[0].pos);
            rotated.vertices[1].pos = ApplyMatrix(rotationX, geometry[j].vertices[1].pos);
            rotated.vertices[2].pos = ApplyMatrix(rotationX, geometry[j].vertices[2].pos);
            rotated.vertices[0].pos = ApplyMatrix(rotationZ, rotated.vertices[0].pos);
            rotated.vertices[1].pos = ApplyMatrix(rotationZ, rotated.vertices[1].pos);
            rotated.vertices[2].pos = ApplyMatrix(rotationZ, rotated.vertices[2].pos);

            translated = rotated;
            translated.vertices[0].pos.z = rotated.vertices[0].pos.z + cubeZ;
            translated.vertices[1].pos.z = rotated.vertices[1].pos.z + cubeZ;
            translated.vertices[2].pos.z = rotated.vertices[2].pos.z + cubeZ;

            Vec3 normal, vec1, vec2;
            vec1.x = translated.vertices[1].pos.x - translated.vertices[0].pos.x;
            vec1.y = translated.vertices[1].pos.y - translated.vertices[0].pos.y;
            vec1.z = translated.vertices[1].pos.z - translated.vertices[0].pos.z;

            vec2.x = translated.vertices[2].pos.x - translated.vertices[0].pos.x;
            vec2.y = translated.vertices[2].pos.y - translated.vertices[0].pos.y;
            vec2.z = translated.vertices[2].pos.z - translated.vertices[0].pos.z;

            normal.x = crossProductX(vec1, vec2);
            normal.y = crossProductY(vec1, vec2);
            normal.z = crossProductZ(vec1, vec2);

            float l = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
            normal.x /= l;
            normal.y /= l;
            normal.z /= l;

            if (normal.z < 0)
            {
                Vec3 lightDirection = {.x = sqrtf(2.0f) / 2.0f, .y = 0.0f, .z = sqrtf(2.0f) / 2.0f, .w = 1.0f};
                float dot = (lightDirection.x * normal.x + lightDirection.y * normal.y + lightDirection.z * normal.z);

                for (int i = 0; i < 3; i++)
                {
                    translated.vertices[i].color *= -dot;
                    if (translated.vertices[i].color <= 0.3)
                        translated.vertices[i].color = 0.3;
                }

                projected.vertices[0].color = translated.vertices[0].color * (COLOR_COUNT - 1);
                projected.vertices[1].color = translated.vertices[1].color * (COLOR_COUNT - 1);
                projected.vertices[2].color = translated.vertices[2].color * (COLOR_COUNT - 1);
                projected.vertices[0].pos = ApplyMatrix(projection, translated.vertices[0].pos);
                projected.vertices[1].pos = ApplyMatrix(projection, translated.vertices[1].pos);
                projected.vertices[2].pos = ApplyMatrix(projection, translated.vertices[2].pos);

                projected.vertices[0].pos.x = (projected.vertices[0].pos.x + 1.0f) * 0.5 * (float)screen.width;
                projected.vertices[1].pos.x = (projected.vertices[1].pos.x + 1.0f) * 0.5 * (float)screen.width;
                projected.vertices[2].pos.x = (projected.vertices[2].pos.x + 1.0f) * 0.5 * (float)screen.width;
                projected.vertices[0].pos.y = (projected.vertices[0].pos.y + 1.0f) * 0.5 * (float)screen.height;
                projected.vertices[1].pos.y = (projected.vertices[1].pos.y + 1.0f) * 0.5 * (float)screen.height;
                projected.vertices[2].pos.y = (projected.vertices[2].pos.y + 1.0f) * 0.5 * (float)screen.height;

                fillTriangle(screen, projected);
            }

            a += 0.01f;
            b += 0.008f;
            cubeZ += 0.02f;
            if (cubeZ >= screen.far)
                cubeZ = 2.0f;
        }

        display(screen);

        usleep(100000);
    }

    free(geometry);
    free(screen.buffer);
    free(screen.z_buffer);

    return 0;
}