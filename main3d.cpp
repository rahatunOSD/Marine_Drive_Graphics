// =============================================================
//  MARINE DRIVE COX'S BAZAR  -  3D RESORT EDITION
//  ------------------------------------------------------------
//  Build: Code::Blocks + MinGW
//  Linker: freeglut, opengl32, glu32, winmm, gdi32
//
//  No external image files needed - all textures generated in
//  code. Just compile and run.
//
//  Controls:
//     W A S D ............. Move camera
//     Q / E ............... Up / Down
//     Arrow Up / Down ..... Vehicle speed +/-
//     1 / 0 ............... Animation on / off
//     n ................... Night
//     y ................... Day  (renamed from 'd' so it doesn't clash with WASD)
//     u ................... Toggle sunset
//     r / k ............... Rain on / off
//     f / h ............... Fog on / off
//     c ................... Reset camera
//     + / - ............... Fullscreen / windowed
//     x / Esc ............. Exit
// =============================================================

#include <iostream>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>

using namespace std;

#define PI 3.14159265358979323846f

// ============================================================
//  GLOBAL STATE
// ============================================================
bool  animOn       = true;
bool  night        = false;
bool  sunset       = false;
bool  rain         = false;
bool  fogOn        = false;

float vehicleSpeed = 0.08f;
float carPos[8];
float boatPos[4];
float birdPhase[6];
float waiterPhase = 0.0f;       // walking animation phase

// ---- NEW FEATURES ----
bool  thunderOn    = false;
float thunderTimer = 2.0f;
float thunderFlash = 0.0f;
float thunderX     = 0.0f, thunderZ = 30.0f;
float swimPhase[4] = { 0.0f, 1.57f, 3.14f, 4.71f };
float shopPhase    = 0.0f;
float trainX       = -900.0f;     // train start — enters from left

#define RAIN_COUNT 500
float rainX[RAIN_COUNT];
float rainY[RAIN_COUNT];
float rainZ[RAIN_COUNT];

// camera
float camX = 60.0f, camY = 18.0f, camZ = 55.0f;
float camYaw   = -150.0f;
float camPitch =  -15.0f;
int   lastMouseX = 0, lastMouseY = 0;
bool  mouseDragging = false;

// textures
GLuint texSky, texSea, texSand, texRoad, texGrass;
GLuint texMountain, texWall, texGlass, texLeaf, texCloud;
GLuint texPool, texFlower, texDome, texSidewalk;
GLuint texDirt, texLightDirt;

// ============================================================
//  UTILITIES
// ============================================================
static float frand(float a, float b) { return a + (b - a) * (rand() / (float)RAND_MAX); }

static void drawBox(float sx, float sy, float sz)
{
    glPushMatrix();
    glScalef(sx, sy, sz);
    glutSolidCube(1.0);
    glPopMatrix();
}

static void renderText(float x, float y, void *font, const char *s)
{
    glRasterPos2f(x, y);
    while (*s) glutBitmapCharacter(font, *s++);
}

// internal helper - draws stroke text at exact world position (left aligned)
// adds extra letter-spacing (in stroke units, applied per-char before drawing)
static void drawStrokeText_(const char *str, float x, float y, float z,
                            float scale, float lineWidth)
{
    const float extraSpacing = 30.0f;   // extra gap between letters (stroke units)
    glLineWidth(lineWidth);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);
    for (const char *p = str; *p; ++p) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
        glTranslatef(extraSpacing, 0.0f, 0.0f);
    }
    glPopMatrix();
}

// 3D stroke text (GLUT_STROKE_ROMAN = Times-Roman style) - thin, centered, no border
static void draw3DText(const char *str, float x, float y, float z,
                       float charHeight, float r, float g, float b)
{
    glDisable(GL_LIGHTING);
    glColor3f(r, g, b);

    const float extraSpacing = 30.0f;
    int   nChars = 0;
    float totalW = 0.0f;
    for (const char *p = str; *p; ++p) {
        totalW += glutStrokeWidth(GLUT_STROKE_ROMAN, *p);
        nChars++;
    }
    totalW += extraSpacing * (nChars > 0 ? nChars - 1 : 0);
    float scale = charHeight / 100.0f;
    float startX = x - (totalW * scale) * 0.5f;
    float baselineY = y - charHeight * 0.45f;

    drawStrokeText_(str, startX, baselineY, z, scale, 2.0f);
    glLineWidth(1.0f);

    glEnable(GL_LIGHTING);
}

static void setMat(float r, float g, float b)
{
    // Set both via glColor (tracked by GL_COLOR_MATERIAL) AND glMaterial
    // (covers both front and back faces). This guarantees the color sticks
    // regardless of which one OpenGL implementation prefers.
    glColor3f(r, g, b);
    GLfloat m[4] = { r, g, b, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, m);
}

// Simple value-noise (deterministic, integer-hash based)
static float vnoise(int x, int y)
{
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

static float smoothNoise(float x, float y)
{
    int   ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - ix, fy = y - iy;
    float a = vnoise(ix,     iy);
    float b = vnoise(ix + 1, iy);
    float c = vnoise(ix,     iy + 1);
    float d = vnoise(ix + 1, iy + 1);
    float u = fx * fx * (3 - 2 * fx);
    float v = fy * fy * (3 - 2 * fy);
    return a + (b - a) * u + (c - a) * v + (a - b - c + d) * u * v;
}

static float fbm(float x, float y, int oct)
{
    float v = 0.0f, amp = 1.0f, freq = 1.0f, tot = 0.0f;
    for (int i = 0; i < oct; ++i) {
        v   += amp * smoothNoise(x * freq, y * freq);
        tot += amp;
        amp *= 0.5f;
        freq *= 2.0f;
    }
    return v / tot;
}

static GLubyte clampu(float v) { if (v < 0) v = 0; if (v > 255) v = 255; return (GLubyte)v; }

// ============================================================
//  PROCEDURAL TEXTURES
// ============================================================
static GLuint makeTexFromBuffer(int w, int h, GLubyte *data)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return id;
}

static GLuint genSkyTex()
{
    const int W = 256, H = 128;
    GLubyte *d = new GLubyte[W * H * 4];
    for (int y = 0; y < H; ++y) {
        float t = y / (float)H;
        for (int x = 0; x < W; ++x) {
            float n = fbm(x * 0.03f, y * 0.05f, 4);
            // base gradient
            float r = 0.55f * (1 - t) + 0.85f * t;
            float g = 0.75f * (1 - t) + 0.92f * t;
            float b = 0.95f * (1 - t) + 0.98f * t;
            // clouds in upper half
            float cloud = 0;
            if (y < H * 0.7f) {
                float nn = fbm(x * 0.025f, y * 0.06f, 5);
                cloud = (nn - 0.05f) * 1.4f;
                if (cloud < 0) cloud = 0;
                if (cloud > 1) cloud = 1;
            }
            r = r * (1 - cloud) + 1.0f * cloud;
            g = g * (1 - cloud) + 1.0f * cloud;
            b = b * (1 - cloud) + 1.0f * cloud;
            int i = (y * W + x) * 4;
            d[i + 0] = clampu(r * 255);
            d[i + 1] = clampu(g * 255);
            d[i + 2] = clampu(b * 255);
            d[i + 3] = 255;
            (void)n;
        }
    }
    GLuint t = makeTexFromBuffer(W, H, d);
    delete[] d;
    return t;
}

static GLuint genSeaTex()
{
    const int N = 128;
    GLubyte *d = new GLubyte[N * N * 4];
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float n = fbm(x * 0.05f, y * 0.05f, 4);
            float r = 0.05f + n * 0.08f;
            float g = 0.45f + n * 0.20f;
            float b = 0.70f + n * 0.25f;
            int i = (y * N + x) * 4;
            d[i + 0] = clampu(r * 255);
            d[i + 1] = clampu(g * 255);
            d[i + 2] = clampu(b * 255);
            d[i + 3] = 255;
        }
    GLuint t = makeTexFromBuffer(N, N, d);
    delete[] d;
    return t;
}

static GLuint genSandTex()
{
    const int N = 128;
    GLubyte *d = new GLubyte[N * N * 4];
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float n = fbm(x * 0.25f, y * 0.25f, 3) * 0.5f + 0.5f;
            float grain = (rand() % 30) / 255.0f;
            float r = 0.92f * n + grain;
            float g = 0.82f * n + grain;
            float b = 0.62f * n + grain;
            int i = (y * N + x) * 4;
            d[i + 0] = clampu(r * 255);
            d[i + 1] = clampu(g * 255);
            d[i + 2] = clampu(b * 255);
            d[i + 3] = 255;
        }
    GLuint t = makeTexFromBuffer(N, N, d);
    delete[] d;
    return t;
}

static GLuint genRoadTex()
{
    const int N = 128;
    GLubyte *d = new GLubyte[N * N * 4];
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float n = fbm(x * 0.5f, y * 0.5f, 3);
            int v = (int)(45 + n * 40 + (rand() % 15));
            int i = (y * N + x) * 4;
            d[i + 0] = clampu(v);
            d[i + 1] = clampu(v + 2);
            d[i + 2] = clampu(v + 4);
            d[i + 3] = 255;
        }
    GLuint t = makeTexFromBuffer(N, N, d);
    delete[] d;
    return t;
}

static GLuint genGrassTex()
{
    const int N = 128;
    GLubyte *d = new GLubyte[N * N * 4];
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float n = fbm(x * 0.4f, y * 0.4f, 3);
            float r = 0.20f + n * 0.20f;
            float g = 0.55f + n * 0.30f;
            float b = 0.18f + n * 0.10f;
            int i = (y * N + x) * 4;
            d[i + 0] = clampu(r * 255);
            d[i + 1] = clampu(g * 255);
            d[i + 2] = clampu(b * 255);
            d[i + 3] = 255;
        }
    GLuint t = makeTexFromBuffer(N, N, d);
    delete[] d;
    return t;
}

static GLuint genMountainTex()
{
    const int N = 128;
    GLubyte *d = new GLubyte[N * N * 4];
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float n = fbm(x * 0.06f, y * 0.06f, 4);
            float h = y / (float)N;
            float r = 0.35f + n * 0.15f - h * 0.05f;
            float g = 0.42f + n * 0.18f - h * 0.05f;
            float b = 0.28f + n * 0.10f;
            int i = (y * N + x) * 4;
            d[i + 0] = clampu(r * 255);
            d[i + 1] = clampu(g * 255);
            d[i + 2] = clampu(b * 255);
            d[i + 3] = 255;
        }
    GLuint t = makeTexFromBuffer(N, N, d);
    delete[] d;
    return t;
}

static GLuint genWallTex()
{
    // cream wall with darker horizontal banding
    const int N = 128;
    GLubyte *d = new GLubyte[N * N * 4];
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float n = fbm(x * 0.05f, y * 0.05f, 3) * 0.05f;
            float r = 0.94f + n;
            float g = 0.90f + n;
            float b = 0.82f + n;
            int i = (y * N + x) * 4;
            d[i + 0] = clampu(r * 255);
            d[i + 1] = clampu(g * 255);
            d[i + 2] = clampu(b * 255);
            d[i + 3] = 255;
        }
    GLuint t = makeTexFromBuffer(N, N, d);
    delete[] d;
    return t;
}

static GLuint genGlassTex()
{
    // blue tinted glass with subtle horizontal banding for floors
    const int W = 64, H = 128;
    GLubyte *d = new GLubyte[W * H * 4];
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            float band = (y % 16 < 2) ? 0.4f : 1.0f;
            float div  = (x % 16 < 1) ? 0.5f : 1.0f;
            float r = 0.45f * band * div;
            float g = 0.62f * band * div;
            float b = 0.78f * band * div;
            int i = (y * W + x) * 4;
            d[i + 0] = clampu(r * 255);
            d[i + 1] = clampu(g * 255);
            d[i + 2] = clampu(b * 255);
            d[i + 3] = 255;
        }
    GLuint t = makeTexFromBuffer(W, H, d);
    delete[] d;
    return t;
}

static GLuint genLeafTex()
{
    const int N = 64;
    GLubyte *d = new GLubyte[N * N * 4];
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float n = fbm(x * 0.2f, y * 0.2f, 2);
            float r = 0.10f + n * 0.10f;
            float g = 0.55f + n * 0.25f;
            float b = 0.15f + n * 0.10f;
            int i = (y * N + x) * 4;
            d[i + 0] = clampu(r * 255);
            d[i + 1] = clampu(g * 255);
            d[i + 2] = clampu(b * 255);
            d[i + 3] = 255;
        }
    GLuint t = makeTexFromBuffer(N, N, d);
    delete[] d;
    return t;
}

static GLuint genCloudTex()
{
    const int N = 128;
    GLubyte *d = new GLubyte[N * N * 4];
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float nx = (x - N * 0.5f) / (N * 0.5f);
            float ny = (y - N * 0.5f) / (N * 0.5f);
            float r2 = nx * nx + ny * ny;
            float fall = 1.0f - r2;
            if (fall < 0) fall = 0;
            float n = fbm(x * 0.05f, y * 0.05f, 4) * 0.5f + 0.5f;
            float a = fall * n;
            int i = (y * N + x) * 4;
            d[i + 0] = 255;
            d[i + 1] = 255;
            d[i + 2] = 255;
            d[i + 3] = clampu(a * 255);
        }
    GLuint t = makeTexFromBuffer(N, N, d);
    delete[] d;
    return t;
}

static GLuint genPoolTex()
{
    const int N = 128;
    GLubyte *d = new GLubyte[N * N * 4];
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float n = fbm(x * 0.1f, y * 0.1f, 3);
            float r = 0.20f + n * 0.10f;
            float g = 0.72f + n * 0.15f;
            float b = 0.92f + n * 0.05f;
            int i = (y * N + x) * 4;
            d[i + 0] = clampu(r * 255);
            d[i + 1] = clampu(g * 255);
            d[i + 2] = clampu(b * 255);
            d[i + 3] = 255;
        }
    GLuint t = makeTexFromBuffer(N, N, d);
    delete[] d;
    return t;
}

static GLuint genFlowerTex()
{
    const int N = 64;
    GLubyte *d = new GLubyte[N * N * 4];
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float n = fbm(x * 0.4f, y * 0.4f, 3);
            float r = 0.55f + n * 0.30f;
            float g = 0.20f + n * 0.15f;
            float b = 0.70f + n * 0.25f;
            int i = (y * N + x) * 4;
            d[i + 0] = clampu(r * 255);
            d[i + 1] = clampu(g * 255);
            d[i + 2] = clampu(b * 255);
            d[i + 3] = 255;
        }
    GLuint t = makeTexFromBuffer(N, N, d);
    delete[] d;
    return t;
}

static GLuint genDomeTex()
{
    const int N = 64;
    GLubyte *d = new GLubyte[N * N * 4];
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float n = fbm(x * 0.3f, y * 0.3f, 2);
            float r = 0.10f + n * 0.10f;
            float g = 0.45f + n * 0.15f;
            float b = 0.30f + n * 0.10f;
            int i = (y * N + x) * 4;
            d[i + 0] = clampu(r * 255);
            d[i + 1] = clampu(g * 255);
            d[i + 2] = clampu(b * 255);
            d[i + 3] = 255;
        }
    GLuint t = makeTexFromBuffer(N, N, d);
    delete[] d;
    return t;
}

static GLuint genSidewalkTex()
{
    const int N = 128;
    GLubyte *d = new GLubyte[N * N * 4];
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            bool grid = (x % 32 < 2) || (y % 32 < 2);
            float n = fbm(x * 0.2f, y * 0.2f, 2) * 0.1f;
            float v = 0.78f + n;
            if (grid) v *= 0.6f;
            int i = (y * N + x) * 4;
            d[i + 0] = clampu(v * 255);
            d[i + 1] = clampu(v * 250);
            d[i + 2] = clampu(v * 240);
            d[i + 3] = 255;
        }
    GLuint t = makeTexFromBuffer(N, N, d);
    delete[] d;
    return t;
}

// brown forest-floor dirt with darker patches + grass tufts
static GLuint genDirtTex()
{
    const int N = 128;
    GLubyte *d = new GLubyte[N * N * 4];
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float n  = fbm(x * 0.12f, y * 0.12f, 4);
            float n2 = fbm(x * 0.45f, y * 0.45f, 2);
            // base brown earth
            float r = 0.42f + n * 0.18f;
            float g = 0.30f + n * 0.15f;
            float b = 0.18f + n * 0.10f;
            // dark moist patches
            if (n < -0.15f) {
                r *= 0.6f; g *= 0.55f; b *= 0.55f;
            }
            // scattered grass tufts (greenish dots)
            if (n2 > 0.30f && (rand() % 5 == 0)) {
                r = 0.25f + n2 * 0.10f;
                g = 0.45f + n2 * 0.15f;
                b = 0.18f;
            }
            // tiny pebble grain
            float grain = (rand() % 25) / 255.0f - 0.05f;
            r += grain; g += grain; b += grain;
            int i = (y * N + x) * 4;
            d[i + 0] = clampu(r * 255);
            d[i + 1] = clampu(g * 255);
            d[i + 2] = clampu(b * 255);
            d[i + 3] = 255;
        }
    GLuint t = makeTexFromBuffer(N, N, d);
    delete[] d;
    return t;
}

// light cream / pale-tan mati for paths and roadside strips
static GLuint genLightDirtTex()
{
    const int N = 128;
    GLubyte *d = new GLubyte[N * N * 4];
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float n  = fbm(x * 0.15f, y * 0.15f, 3);
            float n2 = fbm(x * 0.45f, y * 0.45f, 2);
            // light cream/tan base
            float r = 0.86f + n * 0.07f;
            float g = 0.80f + n * 0.06f;
            float b = 0.66f + n * 0.05f;
            // small darker spots
            if (n2 < -0.15f) {
                r *= 0.88f; g *= 0.85f; b *= 0.82f;
            }
            // tiny grain
            float grain = (rand() % 18) / 255.0f - 0.035f;
            r += grain; g += grain; b += grain;
            int i = (y * N + x) * 4;
            d[i + 0] = clampu(r * 255);
            d[i + 1] = clampu(g * 255);
            d[i + 2] = clampu(b * 255);
            d[i + 3] = 255;
        }
    GLuint t = makeTexFromBuffer(N, N, d);
    delete[] d;
    return t;
}

static void initTextures()
{
    texSky       = genSkyTex();
    texSea       = genSeaTex();
    texSand      = genSandTex();
    texRoad      = genRoadTex();
    texGrass     = genGrassTex();
    texMountain  = genMountainTex();
    texWall      = genWallTex();
    texGlass     = genGlassTex();
    texLeaf      = genLeafTex();
    texCloud     = genCloudTex();
    texPool      = genPoolTex();
    texFlower    = genFlowerTex();
    texDome      = genDomeTex();
    texSidewalk  = genSidewalkTex();
    texDirt      = genDirtTex();
    texLightDirt = genLightDirtTex();
}

// ============================================================
//  SKY / MOUNTAINS / CLOUDS
// ============================================================
static void drawSkyDome()
{
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texSky);

    if (night)        glColor3f(0.15f, 0.15f, 0.30f);
    else if (sunset)  glColor3f(1.00f, 0.65f, 0.45f);
    else              glColor3f(1.00f, 1.00f, 1.00f);

    glPushMatrix();
    glTranslatef(camX, 0.0f, camZ);
    const int S = 32;
    for (int i = 0; i < S; ++i) {
        float a0 = (i / (float)S) * 2 * PI;
        float a1 = ((i + 1) / (float)S) * 2 * PI;
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= 8; ++j) {
            float t = j / 8.0f;
            float y = -10 + t * 130;
            glTexCoord2f(i / (float)S * 6.0f, 1.0f - t);
            glVertex3f(cosf(a0) * 250, y, sinf(a0) * 250);
            glTexCoord2f((i + 1) / (float)S * 6.0f, 1.0f - t);
            glVertex3f(cosf(a1) * 250, y, sinf(a1) * 250);
        }
        glEnd();
    }
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
}

static void drawClouds()
{
    if (night) return;
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texCloud);

    glColor4f(1, 1, 1, sunset ? 0.5f : 0.65f);
    srand(11);
    for (int i = 0; i < 10; ++i) {
        float ax = frand(-400, 400);
        float az = frand(-400, 400);
        float ay = frand(70, 100);
        float sw = frand(12, 22);
        float sh = frand(5, 9);
        glPushMatrix();
        glTranslatef(camX + ax, ay, camZ + az);
        // billboard towards camera (rough)
        glRotatef(-camYaw - 90.0f, 0, 1, 0);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex3f(-sw, -sh, 0);
        glTexCoord2f(1, 0); glVertex3f( sw, -sh, 0);
        glTexCoord2f(1, 1); glVertex3f( sw,  sh, 0);
        glTexCoord2f(0, 1); glVertex3f(-sw,  sh, 0);
        glEnd();
        glPopMatrix();
    }

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
}

static void drawStars()
{
    if (!night) return;
    glDisable(GL_LIGHTING);
    // varied star sizes for sparkle
    glColor3f(1.0f, 1.0f, 0.95f);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    srand(7);
    for (int i = 0; i < 450; ++i) {
        float a = frand(0, 2 * PI);
        float h = frand(40, 110);
        glVertex3f(camX + cosf(a) * 220, h, camZ + sinf(a) * 220);
    }
    glEnd();
    // bigger/brighter stars
    glPointSize(3.5f);
    glColor3f(1.0f, 1.0f, 0.75f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 120; ++i) {
        float a = frand(0, 2 * PI);
        float h = frand(45, 105);
        glVertex3f(camX + cosf(a) * 218, h, camZ + sinf(a) * 218);
    }
    glEnd();
    glPointSize(1.0f);
    glEnable(GL_LIGHTING);
}

static void drawSunMoon()
{
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(camX - 100, 80, camZ - 160);
    if (night) {
        // small glowing moon at night
        glColor3f(0.98f, 0.98f, 0.88f);
        glutSolidSphere(5.5, 24, 24);
        // soft small halo
        glColor4f(0.95f, 0.95f, 0.75f, 0.20f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glutSolidSphere(7.0, 20, 20);
        glColor4f(0.95f, 0.95f, 0.75f, 0.10f);
        glutSolidSphere(9.0, 18, 18);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        // 2 small craters
        glColor3f(0.78f, 0.78f, 0.70f);
        glPushMatrix();
        glTranslatef(-1.8f, 0.8f, 5.2f);
        glutSolidSphere(0.6, 10, 8);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(1.4f, -1.4f, 5.3f);
        glutSolidSphere(0.4, 8, 6);
        glPopMatrix();
    } else if (sunset) {
        glColor3f(1.00f, 0.55f, 0.20f);
        glutSolidSphere(6, 24, 24);
    } else {
        glColor3f(1.00f, 0.95f, 0.55f);
        glutSolidSphere(6, 24, 24);
    }
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

// ---------- very distant, blurred hills (heavy atmospheric haze) ----------
static float terrainHeight(float x, float z)
{
    // hills pushed VERY far back - so they look blurred from distance
    float front = -420.0f;
    float depth = front - z;
    if (depth < 0) return 0.0f;

    // SMOOTHER profile - fewer high-freq components = softer / blurrier look
    float h = 4.0f
            + sinf(x * 0.009f) * 6.0f
            + sinf(x * 0.005f + 1.7f) * 4.5f
            + sinf(x * 0.022f + 3.1f) * 2.0f
            + smoothNoise(x * 0.015f, z * 0.015f) * 7.0f
            + smoothNoise(x * 0.045f, z * 0.045f) * 2.5f;
            // removed high-freq noise octaves for softer blur

    float ramp = depth / 60.0f;
    if (ramp > 1.0f) ramp = 1.0f;
    h *= ramp * 1.0f;

    if (h < 0.0f) h = 0.0f;
    return h;
}

static void hillVertex(float x, float z, int maxDepthIdx, int curDepthIdx)
{
    float h = terrainHeight(x, z);

    // finite-difference normal
    float hxL = terrainHeight(x - 4.0f, z);
    float hxR = terrainHeight(x + 4.0f, z);
    float hzD = terrainHeight(x, z - 4.0f);
    float hzU = terrainHeight(x, z + 4.0f);
    float nx = (hxL - hxR);
    float nz = (hzD - hzU);
    float ny = 10.0f;
    float nl = sqrtf(nx * nx + ny * ny + nz * nz);
    if (nl < 1e-5f) nl = 1.0f;
    nx /= nl; ny /= nl; nz /= nl;

    // colour: deep green close, hazy blue-grey far (atmospheric perspective)
    float depthT = curDepthIdx / (float)maxDepthIdx;   // 0..1
    // also lighter on sunny slopes (where height is greater)
    float heightT = h / 25.0f;
    if (heightT > 1) heightT = 1;

    float r, g, b;
    if (night) {
        r = 0.06f + depthT * 0.06f;
        g = 0.10f + depthT * 0.08f;
        b = 0.20f + depthT * 0.12f;
    } else if (sunset) {
        float baseR = 0.50f + heightT * 0.10f;
        float baseG = 0.42f + heightT * 0.05f;
        float baseB = 0.45f;
        float skyR = 0.95f, skyG = 0.65f, skyB = 0.55f;
        float haze = 0.40f + depthT * 0.50f;
        if (haze > 0.95f) haze = 0.95f;
        r = baseR * (1 - haze) + skyR * haze;
        g = baseG * (1 - haze) + skyG * haze;
        b = baseB * (1 - haze) + skyB * haze;
    } else {
        // sunny day - VERY blurred/hazy distant hills
        // heavy haze across whole ridge - blurred / out-of-focus look
        float baseR = 0.30f + heightT * 0.10f;
        float baseG = 0.44f + heightT * 0.10f;
        float baseB = 0.38f + heightT * 0.05f;
        float skyR  = 0.78f, skyG = 0.85f, skyB = 0.94f;
        // HEAVY haze from the start - simulates atmospheric blur
        float haze = 0.62f + depthT * 0.34f;
        if (haze > 0.97f) haze = 0.97f;
        r = baseR * (1 - haze) + skyR * haze;
        g = baseG * (1 - haze) + skyG * haze;
        b = baseB * (1 - haze) + skyB * haze;
    }

    glColor3f(r, g, b);
    glNormal3f(nx, ny, nz);
    glTexCoord2f(x * 0.025f, z * 0.025f);
    glVertex3f(x, h - 0.45f, z);
}

static void drawMountains()
{
    // proper 3D hill mesh (not flat strips) with per-vertex haze + lighting
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texGrass);

    const int NX = 220;
    const int NZ = 40;
    const float X0 = -1300.0f, X1 = 1300.0f;
    const float Z0 = -420.0f,  Z1 = -1200.0f;   // VERY far away - blurred by distance

    for (int zi = 0; zi < NZ; ++zi) {
        float z0 = Z0 + (Z1 - Z0) * (zi       / (float)NZ);
        float z1 = Z0 + (Z1 - Z0) * ((zi + 1) / (float)NZ);
        glBegin(GL_TRIANGLE_STRIP);
        for (int xi = 0; xi <= NX; ++xi) {
            float x = X0 + (X1 - X0) * (xi / (float)NX);
            hillVertex(x, z0, NZ, zi);
            hillVertex(x, z1, NZ, zi + 1);
        }
        glEnd();
    }

    // reset color to white so subsequent textured objects aren't tinted
    glColor3f(1.0f, 1.0f, 1.0f);
    glDisable(GL_TEXTURE_2D);

    // VERY subtle forest hints - only a few highly-hazed clumps,
    // since the hills are far and should look blurred
    if (!night) {
        srand(31);
        for (int i = 0; i < 35; ++i) {
            float fx = frand(-1200.0f, 1200.0f);
            float fz = frand(-500.0f, -350.0f);
            float h  = terrainHeight(fx, fz);
            if (h < 5.0f) continue;
            // very heavy haze - just barely visible darker patch
            float baseR = 0.22f, baseG = 0.32f, baseB = 0.22f;
            float skyR  = 0.78f, skyG  = 0.85f, skyB  = 0.94f;
            float haze  = 0.78f;
            float r = baseR * (1 - haze) + skyR * haze;
            float g = baseG * (1 - haze) + skyG * haze;
            float b = baseB * (1 - haze) + skyB * haze;
            setMat(r, g, b);
            glPushMatrix();
            glTranslatef(fx, h - 0.3f, fz);
            glScalef(5.0f, 1.8f, 5.0f);
            glutSolidSphere(1.0, 8, 6);
            glPopMatrix();
        }
    }
}

// ============================================================
//  SEA / BEACH / ROAD
// ============================================================
static void drawSea()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texSea);
    glColor3f(1, 1, 1);
    if (night)       setMat(0.10f, 0.20f, 0.35f);
    else if (sunset) setMat(0.85f, 0.55f, 0.45f);
    else             setMat(0.55f, 0.85f, 1.00f);

    float t = glutGet(GLUT_ELAPSED_TIME) * 0.001f;

    // ---- NEAR animated sea (detailed grid in front of camera) ----
    glBegin(GL_QUADS);
    for (int z = 0; z < 30; ++z) {
        for (int x = -200; x < 200; x += 2) {           // wider, lower density
            float fx = (float)x;
            float fz = 6.0f + z * 4.0f;
            float h0 = sinf(fx * 0.4f + t) * 0.10f;
            float h1 = sinf((fx + 2) * 0.4f + t) * 0.10f;
            glNormal3f(0, 1, 0);
            glTexCoord2f(fx * 0.3f,           fz * 0.3f);         glVertex3f(fx,        -0.6f + h0, fz);
            glTexCoord2f((fx + 2) * 0.3f,     fz * 0.3f);         glVertex3f(fx + 2.0f, -0.6f + h1, fz);
            glTexCoord2f((fx + 2) * 0.3f,     (fz + 4) * 0.3f);   glVertex3f(fx + 2.0f, -0.6f + h1, fz + 4.0f);
            glTexCoord2f(fx * 0.3f,           (fz + 4) * 0.3f);   glVertex3f(fx,        -0.6f + h0, fz + 4.0f);
        }
    }
    glEnd();

    // ---- FAR sea (one big flat textured quad to horizon) ----
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0,    0);   glVertex3f(-1200.0f, -0.60f, 126.0f);
    glTexCoord2f(240,  0);   glVertex3f( 1200.0f, -0.60f, 126.0f);
    glTexCoord2f(240, 200);  glVertex3f( 1200.0f, -0.60f, 1200.0f);
    glTexCoord2f(0,   200);  glVertex3f(-1200.0f, -0.60f, 1200.0f);

    // also extend sea sideways beyond near-grid range
    glTexCoord2f(0,   0);    glVertex3f(-1200.0f, -0.60f,    6.0f);
    glTexCoord2f(200, 0);    glVertex3f( -200.0f, -0.60f,    6.0f);
    glTexCoord2f(200, 24);   glVertex3f( -200.0f, -0.60f,  126.0f);
    glTexCoord2f(0,   24);   glVertex3f(-1200.0f, -0.60f,  126.0f);

    glTexCoord2f(0,   0);    glVertex3f(  200.0f, -0.60f,    6.0f);
    glTexCoord2f(200, 0);    glVertex3f( 1200.0f, -0.60f,    6.0f);
    glTexCoord2f(200, 24);   glVertex3f( 1200.0f, -0.60f,  126.0f);
    glTexCoord2f(0,   24);   glVertex3f(  200.0f, -0.60f,  126.0f);
    glEnd();

    // white foam strip near shore
    glDisable(GL_TEXTURE_2D);
    glColor4f(1, 1, 1, 0.6f);
    setMat(0.95f, 0.95f, 0.95f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    for (int x = -200; x < 200; ++x) {
        float fx = (float)x;
        float wob = sinf(fx * 0.6f + t * 2) * 0.4f;
        glNormal3f(0, 1, 0);
        glVertex3f(fx,        -0.55f, 5.0f);
        glVertex3f(fx + 1.0f, -0.55f, 5.0f);
        glVertex3f(fx + 1.0f, -0.55f, 6.0f + wob);
        glVertex3f(fx,        -0.55f, 6.0f + wob);
    }
    glEnd();
    glDisable(GL_BLEND);
}

static void drawBeach()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texSand);
    glColor3f(1, 1, 1);
    setMat(1.0f, 0.95f, 0.75f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0,   0);  glVertex3f(-800.0f, -0.5f,  0.0f);
    glTexCoord2f(160, 0);  glVertex3f( 800.0f, -0.5f,  0.0f);
    glTexCoord2f(160, 4);  glVertex3f( 800.0f, -0.5f,  6.0f);
    glTexCoord2f(0,   4);  glVertex3f(-800.0f, -0.5f,  6.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

static void drawRoad()
{
    const float L = 800.0f;       // road / scene half-length (horizon)

    // asphalt - WIDER (16 units wide now)
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texRoad);
    glColor3f(1, 1, 1);
    setMat(0.6f, 0.6f, 0.6f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0,  0);  glVertex3f(-L, -0.45f, -16.0f);
    glTexCoord2f(72, 0);  glVertex3f( L, -0.45f, -16.0f);
    glTexCoord2f(72, 4);  glVertex3f( L, -0.45f,   0.0f);
    glTexCoord2f(0,  4);  glVertex3f(-L, -0.45f,   0.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // double yellow center line (at middle, z = -8)
    setMat(0.95f, 0.85f, 0.20f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-L, -0.44f, -8.15f);
    glVertex3f( L, -0.44f, -8.15f);
    glVertex3f( L, -0.44f, -8.05f);
    glVertex3f(-L, -0.44f, -8.05f);

    glVertex3f(-L, -0.44f, -7.95f);
    glVertex3f( L, -0.44f, -7.95f);
    glVertex3f( L, -0.44f, -7.85f);
    glVertex3f(-L, -0.44f, -7.85f);
    glEnd();

    // white lane dashes - 4 lanes total, 2 each direction
    setMat(0.95f, 0.95f, 0.95f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    for (float x = -L + 2; x < L - 2; x += 4) {
        // sea-side outer
        glVertex3f(x,       -0.44f, -2.05f);
        glVertex3f(x + 2.0f,-0.44f, -2.05f);
        glVertex3f(x + 2.0f,-0.44f, -1.95f);
        glVertex3f(x,       -0.44f, -1.95f);
        // sea-side inner
        glVertex3f(x,       -0.44f, -5.05f);
        glVertex3f(x + 2.0f,-0.44f, -5.05f);
        glVertex3f(x + 2.0f,-0.44f, -4.95f);
        glVertex3f(x,       -0.44f, -4.95f);
        // land-side inner
        glVertex3f(x,       -0.44f, -11.05f);
        glVertex3f(x + 2.0f,-0.44f, -11.05f);
        glVertex3f(x + 2.0f,-0.44f, -10.95f);
        glVertex3f(x,       -0.44f, -10.95f);
        // land-side outer
        glVertex3f(x,       -0.44f, -14.05f);
        glVertex3f(x + 2.0f,-0.44f, -14.05f);
        glVertex3f(x + 2.0f,-0.44f, -13.95f);
        glVertex3f(x,       -0.44f, -13.95f);
    }
    glEnd();

    // sidewalks - shifted for wider road
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texSidewalk);
    glColor3f(1, 1, 1);
    setMat(1, 1, 1);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0,  0);  glVertex3f(-L, -0.40f, -0.4f);
    glTexCoord2f(72, 0);  glVertex3f( L, -0.40f, -0.4f);
    glTexCoord2f(72, 1);  glVertex3f( L, -0.40f,  0.0f);
    glTexCoord2f(0,  1);  glVertex3f(-L, -0.40f,  0.0f);

    glTexCoord2f(0,  0);  glVertex3f(-L, -0.40f, -18.5f);
    glTexCoord2f(72, 0);  glVertex3f( L, -0.40f, -18.5f);
    glTexCoord2f(72, 1);  glVertex3f( L, -0.40f, -16.0f);
    glTexCoord2f(0,  1);  glVertex3f(-L, -0.40f, -16.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // LIGHT MATI strip behind sidewalk - between road and buildings
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texLightDirt);
    setMat(1, 1, 1);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0,  0);  glVertex3f(-L, -0.42f, -50.0f);
    glTexCoord2f(60, 0);  glVertex3f( L, -0.42f, -50.0f);
    glTexCoord2f(60, 6);  glVertex3f( L, -0.42f, -18.5f);
    glTexCoord2f(0,  6);  glVertex3f(-L, -0.42f, -18.5f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

// ============================================================
//  BEACH OBJECTS: umbrellas, sunbeds, people
// ============================================================
static void drawUmbrella(float x, float z, float colR, float colG, float colB)
{
    glPushMatrix();
    glTranslatef(x, -0.5f, z);
    // pole
    setMat(0.3f, 0.2f, 0.1f);
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    GLUquadric *q = gluNewQuadric();
    gluCylinder(q, 0.04, 0.04, 2.5, 8, 1);
    gluDeleteQuadric(q);
    glPopMatrix();
    // canopy (cone-like) - 8 segments
    setMat(colR, colG, colB);
    float top  = 2.6f;
    float side = 2.2f;
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(0, top, 0);
    for (int i = 0; i <= 8; ++i) {
        float a = i * (2 * PI / 8.0f);
        float r = 1.0f;
        glVertex3f(cosf(a) * r, side, sinf(a) * r);
    }
    glEnd();
    glPopMatrix();
}

static void drawSunbed(float x, float z, float rot)
{
    glPushMatrix();
    glTranslatef(x, -0.4f, z);
    glRotatef(rot, 0, 1, 0);
    setMat(0.95f, 0.95f, 0.95f);
    // base
    glPushMatrix();
    glTranslatef(0, 0.10f, 0);
    drawBox(1.6f, 0.08f, 0.6f);
    glPopMatrix();
    // back rest
    glPushMatrix();
    glTranslatef(0.55f, 0.32f, 0);
    glRotatef(-25, 0, 0, 1);
    drawBox(0.55f, 0.08f, 0.6f);
    glPopMatrix();
    // legs
    setMat(0.5f, 0.4f, 0.3f);
    glPushMatrix(); glTranslatef( 0.65f, 0.0f,  0.25f); drawBox(0.06f, 0.20f, 0.06f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.65f, 0.0f,  0.25f); drawBox(0.06f, 0.20f, 0.06f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.65f, 0.0f, -0.25f); drawBox(0.06f, 0.20f, 0.06f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.65f, 0.0f, -0.25f); drawBox(0.06f, 0.20f, 0.06f); glPopMatrix();
    glPopMatrix();
}

static void drawPerson(float x, float z, float skin1, float skin2, float skin3,
                       float shirtR, float shirtG, float shirtB)
{
    glPushMatrix();
    glTranslatef(x, -0.5f, z);
    // legs
    setMat(0.20f, 0.20f, 0.40f);
    glPushMatrix(); glTranslatef(-0.08f, 0.30f, 0); drawBox(0.10f, 0.60f, 0.10f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.08f, 0.30f, 0); drawBox(0.10f, 0.60f, 0.10f); glPopMatrix();
    // torso
    setMat(shirtR, shirtG, shirtB);
    glPushMatrix(); glTranslatef(0, 0.85f, 0); drawBox(0.30f, 0.45f, 0.20f); glPopMatrix();
    // head
    setMat(skin1, skin2, skin3);
    glPushMatrix(); glTranslatef(0, 1.20f, 0); glutSolidSphere(0.13, 12, 12); glPopMatrix();
    // arms
    setMat(skin1, skin2, skin3);
    glPushMatrix(); glTranslatef(-0.22f, 0.90f, 0); drawBox(0.10f, 0.35f, 0.10f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.22f, 0.90f, 0); drawBox(0.10f, 0.35f, 0.10f); glPopMatrix();
    glPopMatrix();
}

static void drawBeachStuff()
{
    srand(123);
    // umbrellas, sunbeds, people scattered along the beach
    float colors[6][3] = {
        {0.85f, 0.25f, 0.25f},
        {0.20f, 0.45f, 0.85f},
        {1.00f, 0.85f, 0.30f},
        {0.30f, 0.75f, 0.40f},
        {0.95f, 0.55f, 0.25f},
        {0.65f, 0.25f, 0.75f}
    };
    for (int i = 0; i < 44; ++i) {
        float x = -160.0f + i * 7.0f + frand(-2, 2);
        float z = frand(1.5f, 5.0f);
        int c = (i + 3) % 6;
        drawUmbrella(x, z, colors[c][0], colors[c][1], colors[c][2]);
        // sunbed next to it
        drawSunbed(x + frand(-0.8f, 0.8f), z + frand(-0.3f, 0.3f), frand(-30, 30));
        if (i % 2 == 0)
            drawSunbed(x + frand(-1.5f, -0.5f), z + frand(-0.2f, 0.4f), frand(-30, 30) + 180);
    }
    // people walking
    for (int i = 0; i < 26; ++i) {
        float x = -160.0f + i * 12.0f + frand(-3, 3);
        float z = frand(0.5f, 4.5f);
        int c = (i * 5 + 1) % 6;
        drawPerson(x, z, 0.85f, 0.7f, 0.55f,
                   colors[c][0], colors[c][1], colors[c][2]);
    }
}

// ============================================================
//  COCONUT TREES
// ============================================================
static void drawCoconutTree(float x, float z, float scale = 1.0f)
{
    glPushMatrix();
    glTranslatef(x, -0.5f, z);
    glScalef(scale, scale, scale);
    setMat(0.45f, 0.28f, 0.15f);
    GLUquadric *q = gluNewQuadric();
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 0.22, 0.16, 5.5, 12, 2);
    glPopMatrix();
    gluDeleteQuadric(q);

    // fronds with leaf texture
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texLeaf);
    glColor3f(1, 1, 1);
    setMat(1, 1, 1);
    glTranslatef(0, 5.5f, 0);
    for (int i = 0; i < 8; ++i) {
        glPushMatrix();
        glRotatef(i * (360.0f / 8.0f), 0, 1, 0);
        glRotatef(30 + (i % 2) * 8, 0, 0, 1);
        glTranslatef(1.2f, 0, 0);
        // a leaf made of a flat quad
        glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);
        glTexCoord2f(0, 0); glVertex3f(-1.4f, 0, -0.4f);
        glTexCoord2f(1, 0); glVertex3f( 1.4f, 0, -0.4f);
        glTexCoord2f(1, 1); glVertex3f( 1.4f, 0,  0.4f);
        glTexCoord2f(0, 1); glVertex3f(-1.4f, 0,  0.4f);
        glEnd();
        glPopMatrix();
    }
    glDisable(GL_TEXTURE_2D);

    // coconuts
    setMat(0.30f, 0.18f, 0.08f);
    for (int i = 0; i < 3; ++i) {
        glPushMatrix();
        float a = i * 120.0f * PI / 180.0f;
        glTranslatef(cosf(a) * 0.25f, -0.2f, sinf(a) * 0.25f);
        glutSolidSphere(0.16, 10, 10);
        glPopMatrix();
    }
    glPopMatrix();
}

// ============================================================
//  STREET LAMP
// ============================================================
static void drawLampPost(float x)
{
    setMat(0.15f, 0.15f, 0.18f);
    glPushMatrix();
    glTranslatef(x, 1.5f, -17.0f);
    drawBox(0.12f, 4.0f, 0.12f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(x + 0.5f, 3.4f, -17.0f);
    drawBox(1.0f, 0.08f, 0.08f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(x + 1.0f, 3.3f, -17.0f);
    if (night || sunset) {
        GLfloat em[4] = { 1.0f, 0.95f, 0.5f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
    }
    setMat(0.95f, 0.9f, 0.55f);
    glutSolidSphere(0.20, 12, 12);
    GLfloat z[4] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, z);
    glPopMatrix();
}

// ============================================================
//  RESORT BOUNDARY WALL + BIG ENTRANCE GATE
// ============================================================
static void drawResortBoundary()
{
    // ---- Boundary aligned with restaurant's gate line, back side OPEN ----
    float bL = -32.0f, bR = 17.0f;
    float bF = -19.0f;
    float bB = -71.0f;                    // forward of forest line at z=-73
    float baseH = 0.45f;
    float barH  = 2.30f;
    float baseY = -0.50f;
    float gateGapHalf = 5.0f;

    // ---- low CONCRETE BASE walls (very short) ----
    setMat(0.92f, 0.93f, 0.94f);
    // FRONT base - left segment
    {
        float leftLen = (-gateGapHalf) - bL;
        float midX = (bL + (-gateGapHalf)) * 0.5f;
        glPushMatrix();
        glTranslatef(midX, baseY + baseH * 0.5f, bF);
        drawBox(leftLen, baseH, 0.25f);
        glPopMatrix();
    }
    // FRONT base - right segment
    {
        float rightLen = bR - gateGapHalf;
        float midX = (gateGapHalf + bR) * 0.5f;
        glPushMatrix();
        glTranslatef(midX, baseY + baseH * 0.5f, bF);
        drawBox(rightLen, baseH, 0.25f);
        glPopMatrix();
    }
    // (BACK base REMOVED - back side open to forest)
    // LEFT base
    glPushMatrix();
    glTranslatef(bL, baseY + baseH * 0.5f, (bF + bB) * 0.5f);
    drawBox(0.25f, baseH, bF - bB);
    glPopMatrix();
    // RIGHT base
    glPushMatrix();
    glTranslatef(bR, baseY + baseH * 0.5f, (bF + bB) * 0.5f);
    drawBox(0.25f, baseH, bF - bB);
    glPopMatrix();

    // ---- VERTICAL METAL BARS (see-through, you can see inside) ----
    setMat(0.78f, 0.82f, 0.88f);
    float barSpacing = 0.42f;
    float barW = 0.06f;

    // FRONT side bars - left of gate
    for (float bx = bL + 0.4f; bx < -gateGapHalf - 0.5f; bx += barSpacing) {
        glPushMatrix();
        glTranslatef(bx, baseY + baseH + barH * 0.5f, bF);
        drawBox(barW, barH, barW);
        glPopMatrix();
    }
    // FRONT side bars - right of gate
    for (float bx = gateGapHalf + 0.5f; bx < bR - 0.4f; bx += barSpacing) {
        glPushMatrix();
        glTranslatef(bx, baseY + baseH + barH * 0.5f, bF);
        drawBox(barW, barH, barW);
        glPopMatrix();
    }
    // (BACK side bars REMOVED)
    // LEFT side bars
    for (float bz = bB + 0.4f; bz < bF - 0.4f; bz += barSpacing) {
        glPushMatrix();
        glTranslatef(bL, baseY + baseH + barH * 0.5f, bz);
        drawBox(barW, barH, barW);
        glPopMatrix();
    }
    // RIGHT side bars
    for (float bz = bB + 0.4f; bz < bF - 0.4f; bz += barSpacing) {
        glPushMatrix();
        glTranslatef(bR, baseY + baseH + barH * 0.5f, bz);
        drawBox(barW, barH, barW);
        glPopMatrix();
    }

    // ---- TOP RAILS connecting the bars (thin horizontal cap) ----
    setMat(0.65f, 0.72f, 0.82f);
    // front rails (left + right of gate)
    {
        float leftLen = (-gateGapHalf) - bL;
        float midX = (bL + (-gateGapHalf)) * 0.5f;
        glPushMatrix();
        glTranslatef(midX, baseY + baseH + barH + 0.05f, bF);
        drawBox(leftLen, 0.10f, 0.18f);
        glPopMatrix();
    }
    {
        float rightLen = bR - gateGapHalf;
        float midX = (gateGapHalf + bR) * 0.5f;
        glPushMatrix();
        glTranslatef(midX, baseY + baseH + barH + 0.05f, bF);
        drawBox(rightLen, 0.10f, 0.18f);
        glPopMatrix();
    }
    // (BACK rail REMOVED)
    // left rail
    glPushMatrix();
    glTranslatef(bL, baseY + baseH + barH + 0.05f, (bF + bB) * 0.5f);
    drawBox(0.18f, 0.10f, bF - bB);
    glPopMatrix();
    // right rail
    glPushMatrix();
    glTranslatef(bR, baseY + baseH + barH + 0.05f, (bF + bB) * 0.5f);
    drawBox(0.18f, 0.10f, bF - bB);
    glPopMatrix();

    // ---- DECORATIVE WHITE PILLARS (taller, blue cap) at corners + intervals ----
    float pillarSpacing = 6.0f;
    setMat(0.95f, 0.96f, 0.97f);

    // pillar positions function (so we can do corners + intermediates)
    // FRONT pillars
    for (float px = bL; px <= bR + 0.01f; px += pillarSpacing) {
        if (fabsf(px) < gateGapHalf + 1.0f) continue;
        if (px > bR) px = bR;
        glPushMatrix();
        glTranslatef(px, baseY + 1.5f, bF);
        drawBox(0.50f, 3.0f, 0.50f);
        glPopMatrix();
        // blue cap
        setMat(0.40f, 0.70f, 0.90f);
        glPushMatrix();
        glTranslatef(px, baseY + 3.10f, bF);
        drawBox(0.65f, 0.20f, 0.65f);
        glPopMatrix();
        setMat(0.95f, 0.96f, 0.97f);
    }
    // (BACK pillars REMOVED - back is open)
    // 2 corner cap pillars at the back ends of side walls
    glPushMatrix();
    glTranslatef(bL, baseY + 1.5f, bB);
    drawBox(0.55f, 3.0f, 0.55f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(bR, baseY + 1.5f, bB);
    drawBox(0.55f, 3.0f, 0.55f);
    glPopMatrix();
    setMat(0.40f, 0.70f, 0.90f);
    glPushMatrix();
    glTranslatef(bL, baseY + 3.10f, bB);
    drawBox(0.72f, 0.22f, 0.72f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(bR, baseY + 3.10f, bB);
    drawBox(0.72f, 0.22f, 0.72f);
    glPopMatrix();
    setMat(0.95f, 0.96f, 0.97f);
    // LEFT pillars
    for (float pz = bB; pz <= bF + 0.01f; pz += pillarSpacing) {
        if (pz > bF) pz = bF;
        glPushMatrix();
        glTranslatef(bL, baseY + 1.5f, pz);
        drawBox(0.50f, 3.0f, 0.50f);
        glPopMatrix();
        setMat(0.40f, 0.70f, 0.90f);
        glPushMatrix();
        glTranslatef(bL, baseY + 3.10f, pz);
        drawBox(0.65f, 0.20f, 0.65f);
        glPopMatrix();
        setMat(0.95f, 0.96f, 0.97f);
    }
    // RIGHT pillars
    for (float pz = bB; pz <= bF + 0.01f; pz += pillarSpacing) {
        if (pz > bF) pz = bF;
        glPushMatrix();
        glTranslatef(bR, baseY + 1.5f, pz);
        drawBox(0.50f, 3.0f, 0.50f);
        glPopMatrix();
        setMat(0.40f, 0.70f, 0.90f);
        glPushMatrix();
        glTranslatef(bR, baseY + 3.10f, pz);
        drawBox(0.65f, 0.20f, 0.65f);
        glPopMatrix();
        setMat(0.95f, 0.96f, 0.97f);
    }

    // ============================================================
    //   BIG MAIN GATE at front center
    // ============================================================
    float gateCx = 0.0f;
    float gateZ  = bF;
    float pillarH = 5.5f;
    float pillarTopY = baseY + pillarH;

    // 2 BIG WHITE PILLARS
    setMat(0.96f, 0.97f, 0.97f);
    glPushMatrix();
    glTranslatef(gateCx - gateGapHalf + 0.4f, baseY + pillarH * 0.5f, gateZ);
    drawBox(0.85f, pillarH, 0.85f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(gateCx + gateGapHalf - 0.4f, baseY + pillarH * 0.5f, gateZ);
    drawBox(0.85f, pillarH, 0.85f);
    glPopMatrix();

    // Light blue caps on top of each pillar (decorative)
    setMat(0.40f, 0.70f, 0.90f);
    glPushMatrix();
    glTranslatef(gateCx - gateGapHalf + 0.4f, pillarTopY + 0.20f, gateZ);
    drawBox(1.15f, 0.40f, 1.15f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(gateCx + gateGapHalf - 0.4f, pillarTopY + 0.20f, gateZ);
    drawBox(1.15f, 0.40f, 1.15f);
    glPopMatrix();

    // White horizontal beam / arch connecting the pillars
    setMat(0.96f, 0.97f, 0.97f);
    glPushMatrix();
    glTranslatef(gateCx, pillarTopY - 0.10f, gateZ);
    drawBox(gateGapHalf * 2.0f + 1.0f, 0.70f, 0.95f);
    glPopMatrix();

    // (Seaview Resort sign panel + text removed per user request)

    // Decorative iron grille bars (gate doors, between pillars)
    setMat(0.25f, 0.28f, 0.35f);
    int nBars = 10;
    for (int i = 0; i < nBars; ++i) {
        float bx = gateCx - gateGapHalf + 0.9f + i * ((gateGapHalf * 2.0f - 1.8f) / (nBars - 1));
        glPushMatrix();
        glTranslatef(bx, baseY + 1.6f, gateZ);
        drawBox(0.07f, 3.20f, 0.07f);
        glPopMatrix();
    }
    // 2 horizontal grille rails
    glPushMatrix();
    glTranslatef(gateCx, baseY + 0.50f, gateZ);
    drawBox(gateGapHalf * 2.0f - 1.0f, 0.12f, 0.10f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(gateCx, baseY + 2.90f, gateZ);
    drawBox(gateGapHalf * 2.0f - 1.0f, 0.12f, 0.10f);
    glPopMatrix();

    // ---- INTERIOR DECOR: hedges + flower bushes near the gate path ----
    // green hedges along the inside of the front wall (each side of gate)
    setMat(0.20f, 0.55f, 0.25f);
    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < 5; ++i) {
            float hx = side * (gateGapHalf + 1.5f + i * 2.5f);
            if (hx > bR - 1.0f || hx < bL + 1.0f) continue;
            glPushMatrix();
            glTranslatef(hx, baseY + 0.30f, bF - 1.2f);
            drawBox(1.6f, 0.55f, 0.55f);
            glPopMatrix();
        }
    }
    // pink/purple flower bushes between hedges
    setMat(0.85f, 0.30f, 0.65f);
    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < 4; ++i) {
            float fx = side * (gateGapHalf + 2.8f + i * 2.5f);
            if (fx > bR - 1.0f || fx < bL + 1.0f) continue;
            glPushMatrix();
            glTranslatef(fx, baseY + 0.40f, bF - 2.5f);
            glutSolidSphere(0.40, 10, 8);
            glPopMatrix();
        }
    }
}

// ============================================================
//  BEACH RESORT (U-shape, white buildings with red tile roofs)
// ============================================================
// Single resort building block — white walls + red peaked roof + windows
static void drawResortBlock(float bx, float bz, float w, float h, float d)
{
    float gY = -0.50f;
    float roofH = 1.4f;

    // WHITE walls
    setMat(0.96f, 0.97f, 0.97f);
    glPushMatrix();
    glTranslatef(bx, gY + h * 0.5f, bz);
    drawBox(w, h, d);
    glPopMatrix();

    // RED / TERRACOTTA peaked tile roof - left slant
    setMat(0.72f, 0.28f, 0.18f);
    glBegin(GL_QUADS);
    glNormal3f(-1, 0.5f, 0);
    glVertex3f(bx - w * 0.55f, gY + h, bz - d * 0.5f);
    glVertex3f(bx - w * 0.55f, gY + h, bz + d * 0.5f);
    glVertex3f(bx,             gY + h + roofH, bz + d * 0.5f);
    glVertex3f(bx,             gY + h + roofH, bz - d * 0.5f);
    glEnd();
    // right slant
    glBegin(GL_QUADS);
    glNormal3f(1, 0.5f, 0);
    glVertex3f(bx + w * 0.55f, gY + h, bz - d * 0.5f);
    glVertex3f(bx + w * 0.55f, gY + h, bz + d * 0.5f);
    glVertex3f(bx,             gY + h + roofH, bz + d * 0.5f);
    glVertex3f(bx,             gY + h + roofH, bz - d * 0.5f);
    glEnd();
    // gable triangles (white)
    setMat(0.92f, 0.93f, 0.94f);
    glBegin(GL_TRIANGLES);
    glNormal3f(0, 0, 1);
    glVertex3f(bx - w * 0.55f, gY + h, bz + d * 0.5f);
    glVertex3f(bx + w * 0.55f, gY + h, bz + d * 0.5f);
    glVertex3f(bx,             gY + h + roofH, bz + d * 0.5f);
    glNormal3f(0, 0, -1);
    glVertex3f(bx - w * 0.55f, gY + h, bz - d * 0.5f);
    glVertex3f(bx + w * 0.55f, gY + h, bz - d * 0.5f);
    glVertex3f(bx,             gY + h + roofH, bz - d * 0.5f);
    glEnd();
    // roof ridge line (darker)
    setMat(0.55f, 0.22f, 0.12f);
    glPushMatrix();
    glTranslatef(bx, gY + h + roofH + 0.05f, bz);
    drawBox(0.18f, 0.12f, d + 0.20f);
    glPopMatrix();

    // light blue window panels on front + back sides (2 floors)
    if (night || sunset) {
        GLfloat em[4] = { 0.85f, 0.70f, 0.30f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
        setMat(0.95f, 0.85f, 0.45f);
    } else {
        setMat(0.55f, 0.78f, 0.90f);
    }
    for (int floor = 0; floor < 2; ++floor) {
        float wy = gY + 0.7f + floor * (h * 0.45f);
        for (int win = 0; win < 3; ++win) {
            float wx = bx - w * 0.30f + win * (w * 0.30f);
            // front
            glPushMatrix();
            glTranslatef(wx, wy, bz + d * 0.5f + 0.02f);
            drawBox(0.55f, 0.75f, 0.03f);
            glPopMatrix();
            // back
            glPushMatrix();
            glTranslatef(wx, wy, bz - d * 0.5f - 0.02f);
            drawBox(0.55f, 0.75f, 0.03f);
            glPopMatrix();
        }
    }
    GLfloat zero[4] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
}

// ============================================================
//  OUTDOOR BBQ LOUNGE (pergola roof, wooden deck, stone-base counter)
// ============================================================
static void drawBBQLounge(float cx, float cz)
{
    float gY = -0.50f;
    float W = 19.0f, D = 18.0f;        // longer depth
    float floorH = 0.30f;
    float postH  = 5.5f;

    // wooden deck floor
    setMat(0.55f, 0.38f, 0.22f);
    glPushMatrix();
    glTranslatef(cx, gY + floorH * 0.5f, cz);
    drawBox(W, floorH, D);
    glPopMatrix();
    // plank stripes on deck
    setMat(0.40f, 0.26f, 0.14f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    for (int i = 0; i < 12; ++i) {
        float zp = cz - D * 0.5f + i * (D / 12.0f);
        glVertex3f(cx - W * 0.5f, gY + floorH + 0.005f, zp);
        glVertex3f(cx + W * 0.5f, gY + floorH + 0.005f, zp);
        glVertex3f(cx + W * 0.5f, gY + floorH + 0.005f, zp + 0.06f);
        glVertex3f(cx - W * 0.5f, gY + floorH + 0.005f, zp + 0.06f);
    }
    glEnd();

    // 4 wooden corner posts
    setMat(0.45f, 0.30f, 0.18f);
    float postPos[4][2] = {
        { -W * 0.45f, -D * 0.45f }, {  W * 0.45f, -D * 0.45f },
        { -W * 0.45f,  D * 0.45f }, {  W * 0.45f,  D * 0.45f }
    };
    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
        glTranslatef(cx + postPos[i][0], gY + postH * 0.5f, cz + postPos[i][1]);
        drawBox(0.28f, postH, 0.28f);
        glPopMatrix();
    }

    // 2 long edge beams along Z (pergola support)
    setMat(0.45f, 0.30f, 0.18f);
    glPushMatrix();
    glTranslatef(cx - W * 0.43f, gY + postH + 0.10f, cz);
    drawBox(0.18f, 0.25f, D * 0.95f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(cx + W * 0.43f, gY + postH + 0.10f, cz);
    drawBox(0.18f, 0.25f, D * 0.95f);
    glPopMatrix();

    // SPARSE pergola cross-slats (mostly open sky look)
    setMat(0.40f, 0.27f, 0.15f);
    int nSlats = 9;
    for (int i = 0; i < nSlats; ++i) {
        float zp = cz - D * 0.42f + i * (D * 0.84f / (nSlats - 1));
        glPushMatrix();
        glTranslatef(cx, gY + postH + 0.30f, zp);
        drawBox(W * 0.95f, 0.12f, 0.12f);
        glPopMatrix();
    }

    // ============================================================
    //   BBQ MACHINE - HUGE counter-style grill spanning full back wall
    // ============================================================
    float bbqX = cx;
    float bbqZ = cz - D * 0.32f;
    float bbqY = gY;
    float bbqW = W * 0.88f;                // covers full back width
    float bbqD = 1.60f;                    // depth of grill body

    // 6 metal legs spread along the wide grill
    setMat(0.18f, 0.18f, 0.22f);
    for (int i = 0; i < 6; ++i) {
        float lx = bbqX - bbqW * 0.45f + i * (bbqW * 0.90f / 5.0f);
        for (int side = -1; side <= 1; side += 2) {
            glPushMatrix();
            glTranslatef(lx, bbqY + 0.55f, bbqZ + side * bbqD * 0.40f);
            drawBox(0.16f, 1.10f, 0.16f);
            glPopMatrix();
        }
    }

    // Main grill body (HUGE black box spanning whole back)
    setMat(0.15f, 0.15f, 0.18f);
    glPushMatrix();
    glTranslatef(bbqX, bbqY + 1.35f, bbqZ);
    drawBox(bbqW, 0.55f, bbqD);
    glPopMatrix();

    // Lid - tilted half open
    setMat(0.20f, 0.20f, 0.22f);
    glPushMatrix();
    glTranslatef(bbqX, bbqY + 1.68f, bbqZ - 0.35f);
    glRotatef(-30, 1, 0, 0);
    glTranslatef(0, 0.25f, 0);
    drawBox(bbqW, 0.60f, bbqD);
    glPopMatrix();
    // lid handles (3 across the wide lid)
    setMat(0.35f, 0.35f, 0.40f);
    for (int h = 0; h < 3; ++h) {
        float hx = bbqX - bbqW * 0.30f + h * (bbqW * 0.30f);
        glPushMatrix();
        glTranslatef(hx, bbqY + 2.20f, bbqZ + 0.15f);
        drawBox(0.55f, 0.12f, 0.14f);
        glPopMatrix();
    }

    // Grill grates - many parallel bars spanning full width
    setMat(0.55f, 0.55f, 0.60f);
    int nGrates = (int)(bbqW / 0.40f);
    for (int g = 0; g < nGrates; ++g) {
        float gx = bbqX - bbqW * 0.46f + g * (bbqW * 0.92f / (nGrates - 1));
        glPushMatrix();
        glTranslatef(gx, bbqY + 1.65f, bbqZ);
        drawBox(0.05f, 0.05f, bbqD * 0.85f);
        glPopMatrix();
    }

    // Glowing red-hot coals - full width
    if (night || sunset) {
        GLfloat em[4] = { 1.0f, 0.40f, 0.10f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
    }
    setMat(0.95f, 0.40f, 0.10f);
    glPushMatrix();
    glTranslatef(bbqX, bbqY + 1.50f, bbqZ);
    drawBox(bbqW * 0.92f, 0.12f, bbqD * 0.75f);
    glPopMatrix();
    GLfloat zeroBBQ[4] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zeroBBQ);

    // ============================================================
    //   FOOD ON THE GRILL - meat + fish
    // ============================================================
    // MEAT chunks (reddish brown) along the back row of the grill
    setMat(0.60f, 0.22f, 0.12f);
    for (int i = 0; i < 7; ++i) {
        float mx = bbqX - bbqW * 0.40f + i * (bbqW * 0.80f / 6.0f);
        glPushMatrix();
        glTranslatef(mx, bbqY + 1.72f, bbqZ - bbqD * 0.20f);
        drawBox(0.55f, 0.20f, 0.42f);
        glPopMatrix();
        // darker grill marks on top
        setMat(0.30f, 0.10f, 0.05f);
        glPushMatrix();
        glTranslatef(mx - 0.15f, bbqY + 1.83f, bbqZ - bbqD * 0.20f);
        drawBox(0.04f, 0.02f, 0.40f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(mx + 0.15f, bbqY + 1.83f, bbqZ - bbqD * 0.20f);
        drawBox(0.04f, 0.02f, 0.40f);
        glPopMatrix();
        setMat(0.60f, 0.22f, 0.12f);
    }

    // FISH (silvery oval) along the front row of the grill
    setMat(0.78f, 0.78f, 0.82f);
    for (int i = 0; i < 6; ++i) {
        float fx = bbqX - bbqW * 0.36f + i * (bbqW * 0.72f / 5.0f);
        glPushMatrix();
        glTranslatef(fx, bbqY + 1.72f, bbqZ + bbqD * 0.20f);
        glScalef(0.65f, 0.12f, 0.20f);
        glutSolidSphere(1.0, 12, 8);
        glPopMatrix();
        // dark grill marks on fish
        setMat(0.30f, 0.25f, 0.20f);
        glPushMatrix();
        glTranslatef(fx, bbqY + 1.81f, bbqZ + bbqD * 0.20f);
        drawBox(0.55f, 0.02f, 0.03f);
        glPopMatrix();
        setMat(0.78f, 0.78f, 0.82f);
    }

    // ============================================================
    //   SMOKE rising from the meat + fish
    // ============================================================
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    srand(1357);
    // smoke plumes rising from each food piece
    for (int plume = 0; plume < 13; ++plume) {
        // 7 above meat back row, 6 above fish front row
        float px, pz;
        if (plume < 7) {
            px = bbqX - bbqW * 0.40f + plume * (bbqW * 0.80f / 6.0f);
            pz = bbqZ - bbqD * 0.20f;
        } else {
            px = bbqX - bbqW * 0.36f + (plume - 7) * (bbqW * 0.72f / 5.0f);
            pz = bbqZ + bbqD * 0.20f;
        }
        // each plume has ~5 puffs rising
        for (int p = 0; p < 5; ++p) {
            float py = bbqY + 1.95f + p * 0.45f;
            float drift = sinf(plume * 1.3f + p * 0.8f) * 0.15f * p;
            float r = 0.18f + p * 0.08f;
            float alpha = 0.55f - p * 0.10f;
            if (alpha < 0.10f) alpha = 0.10f;
            glColor4f(0.88f, 0.88f, 0.90f, alpha);
            glPushMatrix();
            glTranslatef(px + drift, py, pz);
            glutSolidSphere(r, 10, 8);
            glPopMatrix();
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glColor3f(1, 1, 1);

    // ============================================================
    //   1 ANIMATED CHEF in FRONT of BBQ, cooking with cleaner look
    // ============================================================
    {
        float chefX = bbqX - bbqW * 0.28f;
        float chefZ = bbqZ + bbqD * 0.55f + 0.70f;
        float cgY   = bbqY;

        // animation - cooking motion
        float cookPh   = waiterPhase * 4.0f;
        float bodyBob  = sinf(cookPh) * 0.04f;
        float armSwing = sinf(cookPh) * 18.0f;        // degrees
        float armSwingL= sinf(cookPh + PI * 0.5f) * 12.0f;

        // ---- pants (dark) ----
        setMat(0.18f, 0.18f, 0.24f);
        glPushMatrix();
        glTranslatef(chefX, cgY + 0.60f, chefZ);
        drawBox(0.72f, 1.20f, 0.42f);
        glPopMatrix();

        // ---- shoes (black) ----
        setMat(0.08f, 0.08f, 0.08f);
        glPushMatrix();
        glTranslatef(chefX + 0.20f, cgY + 0.08f, chefZ - 0.08f);
        drawBox(0.30f, 0.16f, 0.45f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(chefX - 0.20f, cgY + 0.08f, chefZ - 0.08f);
        drawBox(0.30f, 0.16f, 0.45f);
        glPopMatrix();

        // ---- WHITE chef coat (apron) - bigger torso for clarity ----
        setMat(0.98f, 0.98f, 0.98f);
        glPushMatrix();
        glTranslatef(chefX, cgY + 1.75f + bodyBob, chefZ);
        drawBox(0.85f, 1.30f, 0.52f);
        glPopMatrix();

        // dark buttons down the chef coat (visible detail)
        setMat(0.30f, 0.30f, 0.35f);
        for (int b = 0; b < 4; ++b) {
            glPushMatrix();
            glTranslatef(chefX, cgY + 2.10f + bodyBob - b * 0.25f, chefZ - 0.27f);
            drawBox(0.08f, 0.08f, 0.04f);
            glPopMatrix();
        }

        // ---- ANIMATED RIGHT ARM with spatula (swinging) ----
        glPushMatrix();
        glTranslatef(chefX + 0.48f, cgY + 2.20f + bodyBob, chefZ);    // shoulder pivot
        glRotatef(-50 + armSwing, 1, 0, 0);                            // swing forward-back
        // white sleeve
        setMat(0.98f, 0.98f, 0.98f);
        glPushMatrix();
        glTranslatef(0, -0.20f, -0.05f);
        drawBox(0.22f, 0.60f, 0.22f);
        glPopMatrix();
        // skin hand at end
        setMat(0.92f, 0.78f, 0.62f);
        glPushMatrix();
        glTranslatef(0, -0.55f, -0.05f);
        glutSolidSphere(0.13, 12, 10);
        glPopMatrix();
        // spatula extending from hand toward grill
        setMat(0.55f, 0.38f, 0.22f);
        glPushMatrix();
        glTranslatef(0, -0.65f, -0.30f);
        drawBox(0.05f, 0.05f, 0.45f);
        glPopMatrix();
        setMat(0.50f, 0.50f, 0.55f);
        glPushMatrix();
        glTranslatef(0, -0.65f, -0.65f);
        drawBox(0.20f, 0.04f, 0.25f);
        glPopMatrix();
        glPopMatrix();

        // ---- ANIMATED LEFT ARM (subtle counter-motion) ----
        glPushMatrix();
        glTranslatef(chefX - 0.48f, cgY + 2.20f + bodyBob, chefZ);
        glRotatef(-30 + armSwingL, 1, 0, 0);
        setMat(0.98f, 0.98f, 0.98f);
        glPushMatrix();
        glTranslatef(0, -0.25f, -0.05f);
        drawBox(0.22f, 0.55f, 0.22f);
        glPopMatrix();
        setMat(0.92f, 0.78f, 0.62f);
        glPushMatrix();
        glTranslatef(0, -0.55f, -0.05f);
        glutSolidSphere(0.13, 12, 10);
        glPopMatrix();
        glPopMatrix();

        // ---- HEAD with face ----
        setMat(0.92f, 0.78f, 0.62f);
        glPushMatrix();
        glTranslatef(chefX, cgY + 2.70f + bodyBob, chefZ);
        glutSolidSphere(0.28, 16, 14);
        glPopMatrix();
        // 2 eyes (small dark dots on face)
        setMat(0.10f, 0.10f, 0.10f);
        glPushMatrix();
        glTranslatef(chefX + 0.10f, cgY + 2.74f + bodyBob, chefZ - 0.25f);
        glutSolidSphere(0.035, 8, 6);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(chefX - 0.10f, cgY + 2.74f + bodyBob, chefZ - 0.25f);
        glutSolidSphere(0.035, 8, 6);
        glPopMatrix();
        // mustache
        setMat(0.18f, 0.10f, 0.05f);
        glPushMatrix();
        glTranslatef(chefX, cgY + 2.62f + bodyBob, chefZ - 0.26f);
        drawBox(0.20f, 0.04f, 0.05f);
        glPopMatrix();
        // smile (small mouth)
        glPushMatrix();
        glTranslatef(chefX, cgY + 2.55f + bodyBob, chefZ - 0.26f);
        drawBox(0.12f, 0.02f, 0.03f);
        glPopMatrix();

        // ---- CHEF TOQUE HAT (clean iconic look) ----
        // base band (slightly darker white)
        setMat(0.95f, 0.95f, 0.95f);
        glPushMatrix();
        glTranslatef(chefX, cgY + 3.05f + bodyBob, chefZ);
        drawBox(0.52f, 0.16f, 0.52f);
        glPopMatrix();
        // big puffy round top
        setMat(0.99f, 0.99f, 0.99f);
        glPushMatrix();
        glTranslatef(chefX, cgY + 3.45f + bodyBob, chefZ);
        glScalef(1.0f, 0.90f, 1.0f);
        glutSolidSphere(0.45, 16, 14);
        glPopMatrix();
        // small puffs on the side for that classic toque shape
        glPushMatrix();
        glTranslatef(chefX + 0.30f, cgY + 3.55f + bodyBob, chefZ + 0.10f);
        glutSolidSphere(0.25, 12, 10);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(chefX - 0.30f, cgY + 3.55f + bodyBob, chefZ - 0.10f);
        glutSolidSphere(0.25, 12, 10);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(chefX + 0.10f, cgY + 3.62f + bodyBob, chefZ - 0.25f);
        glutSolidSphere(0.22, 12, 10);
        glPopMatrix();
    }

    // 2 smoke stack chimneys (on top, spread along the wide grill)
    setMat(0.20f, 0.20f, 0.22f);
    glPushMatrix();
    glTranslatef(bbqX - bbqW * 0.30f, bbqY + 2.15f, bbqZ - 0.55f);
    drawBox(0.26f, 1.00f, 0.26f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(bbqX + bbqW * 0.30f, bbqY + 2.15f, bbqZ - 0.55f);
    drawBox(0.26f, 1.00f, 0.26f);
    glPopMatrix();

    // Control knobs row at the front - many across the wide grill
    setMat(0.30f, 0.30f, 0.32f);
    int nKnobs = 9;
    for (int k = 0; k < nKnobs; ++k) {
        float kx = bbqX - bbqW * 0.40f + k * (bbqW * 0.80f / (nKnobs - 1));
        glPushMatrix();
        glTranslatef(kx, bbqY + 1.35f, bbqZ + bbqD * 0.51f);
        glutSolidSphere(0.10, 10, 8);
        glPopMatrix();
    }

    // BACK wall - vertical wood slats (NOW on the actual BACK side, away from road)
    setMat(0.40f, 0.27f, 0.15f);
    int nSlats2 = 18;
    for (int i = 0; i < nSlats2; ++i) {
        float xp = cx - W * 0.42f + i * (W * 0.84f / (nSlats2 - 1));
        glPushMatrix();
        glTranslatef(xp, gY + 1.80f, cz - D * 0.45f);    // -D*0.45 = back (away from road)
        drawBox(0.08f, 2.00f, 0.08f);
        glPopMatrix();
    }
    // horizontal supports on the back screen
    setMat(0.45f, 0.30f, 0.18f);
    glPushMatrix();
    glTranslatef(cx, gY + 1.05f, cz - D * 0.45f);
    drawBox(W * 0.85f, 0.08f, 0.10f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(cx, gY + 2.65f, cz - D * 0.45f);
    drawBox(W * 0.85f, 0.08f, 0.10f);
    glPopMatrix();

    // 5 BIGGER bar stools in front of the counter
    setMat(0.55f, 0.38f, 0.22f);
    for (int i = 0; i < 5; ++i) {
        float sx = cx - 3.6f + i * 1.8f;
        float sz = cz + 1.0f;
        // round seat - bigger
        glPushMatrix();
        glTranslatef(sx, gY + 1.20f, sz);
        glScalef(0.55f, 0.08f, 0.55f);
        glutSolidSphere(1.0, 14, 8);
        glPopMatrix();
        // metal leg - taller
        setMat(0.30f, 0.30f, 0.34f);
        glPushMatrix();
        glTranslatef(sx, gY + 0.60f, sz);
        drawBox(0.14f, 1.20f, 0.14f);
        glPopMatrix();
        // foot ring
        glPushMatrix();
        glTranslatef(sx, gY + 0.40f, sz);
        drawBox(0.40f, 0.04f, 0.04f);
        glPopMatrix();
        setMat(0.55f, 0.38f, 0.22f);
    }

    // a hanging lantern from the pergola (center)
    if (night || sunset) {
        GLfloat em[4] = { 1.0f, 0.75f, 0.30f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
    }
    setMat(1.0f, 0.85f, 0.40f);
    glPushMatrix();
    glTranslatef(cx, gY + postH - 0.20f, cz);
    drawBox(0.35f, 0.50f, 0.35f);
    glPopMatrix();
    GLfloat zero[4] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
    // cord
    setMat(0.65f, 0.50f, 0.30f);
    glPushMatrix();
    glTranslatef(cx, gY + postH + 0.18f, cz);
    drawBox(0.025f, 0.30f, 0.025f);
    glPopMatrix();

    // ============================================================
    //   Clean "BBQ" banner (black background + white text)
    // ============================================================
    {
        float bannerW = 11.0f;
        float bannerH = 2.4f;
        float bannerY = gY + postH + 0.90f;
        float bannerZ = cz + D * 0.48f;

        // 2 hanging ropes (white)
        setMat(0.95f, 0.95f, 0.95f);
        glPushMatrix();
        glTranslatef(cx - bannerW * 0.40f, bannerY + bannerH * 0.5f + 0.25f, bannerZ);
        drawBox(0.03f, 0.50f, 0.03f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(cx + bannerW * 0.40f, bannerY + bannerH * 0.5f + 0.25f, bannerZ);
        drawBox(0.03f, 0.50f, 0.03f);
        glPopMatrix();

        // BLACK banner background
        setMat(0.08f, 0.08f, 0.08f);
        glPushMatrix();
        glTranslatef(cx, bannerY, bannerZ);
        drawBox(bannerW, bannerH, 0.10f);
        glPopMatrix();

        // WHITE "BBQ" text (centered, big)
        draw3DText("BBQ",
                   cx, bannerY, bannerZ + 0.07f,
                   1.80f,
                   1.00f, 1.00f, 1.00f);
    }
}

// ============================================================
//  BBQ CROWD - group of 3 + couple in beach costumes with skewers
//  cx, cz = same center as drawBBQLounge
// ============================================================
static void drawBeachPersonSkewer(float px, float pz,
                                  float skinR, float skinG, float skinB,
                                  float topR,  float topG,  float topB,
                                  float shrtR, float shrtG, float shrtB,
                                  bool  skwerRight,
                                  bool  womanDress,
                                  float yRot = 0.0f)
{
    float gY = -0.50f;
    glPushMatrix();
    glTranslatef(px, gY, pz);
    glRotatef(yRot, 0, 1, 0);
    glScalef(2.0f, 2.0f, 2.0f);  // larger people

    // SHORTS or DRESS
    if (womanDress) {
        setMat(shrtR, shrtG, shrtB);
        glPushMatrix(); glTranslatef(0, 0.55f, 0); drawBox(0.48f, 1.10f, 0.30f); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0.12f, 0); drawBox(0.62f, 0.10f, 0.36f); glPopMatrix();
    } else {
        setMat(skinR, skinG, skinB);
        glPushMatrix(); glTranslatef(-0.10f, 0.28f, 0); drawBox(0.14f, 0.55f, 0.14f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.10f, 0.28f, 0); drawBox(0.14f, 0.55f, 0.14f); glPopMatrix();
        setMat(shrtR, shrtG, shrtB);
        glPushMatrix(); glTranslatef(0, 0.65f, 0); drawBox(0.40f, 0.45f, 0.24f); glPopMatrix();
    }

    // TANK TOP
    setMat(topR, topG, topB);
    glPushMatrix(); glTranslatef(0, 1.05f, 0); drawBox(0.36f, 0.50f, 0.24f); glPopMatrix();

    // ARMS (skin)
    setMat(skinR, skinG, skinB);
    glPushMatrix(); glTranslatef(-0.25f, 1.00f, 0); drawBox(0.13f, 0.40f, 0.14f); glPopMatrix();
    // skewer arm - raised at angle
    float sArmSide = skwerRight ? 1.0f : -1.0f;
    glPushMatrix();
    glTranslatef(sArmSide * 0.25f, 1.15f, 0);
    glRotatef(-50.0f * sArmSide, 0, 0, 1.0f);
    drawBox(0.13f, 0.45f, 0.14f);
    glPopMatrix();

    // HEAD
    setMat(skinR, skinG, skinB);
    glPushMatrix(); glTranslatef(0, 1.56f, 0); glutSolidSphere(0.21f, 14, 12); glPopMatrix();
    // hair
    setMat(0.12f, 0.07f, 0.02f);
    glPushMatrix(); glTranslatef(0, 1.70f, 0); glScalef(1.1f, 0.5f, 1.1f);
    glutSolidSphere(0.23f, 12, 10); glPopMatrix();

    // SKEWER STICK
    float skX = sArmSide * 0.52f;
    setMat(0.65f, 0.42f, 0.12f);
    glPushMatrix();
    glTranslatef(skX, 1.30f, -0.10f);
    glRotatef(sArmSide * 38.0f, 0, 0, 1.0f);
    glRotatef(-22.0f, 1, 0, 0);
    drawBox(0.05f, 0.82f, 0.05f);
    glPopMatrix();

    // meat chunks on skewer
    setMat(0.65f, 0.22f, 0.08f);
    glPushMatrix(); glTranslatef(skX + sArmSide*0.28f, 1.58f, -0.22f);
    glutSolidSphere(0.10f, 10, 8); glPopMatrix();
    glPushMatrix(); glTranslatef(skX + sArmSide*0.36f, 1.68f, -0.29f);
    glutSolidSphere(0.09f, 10, 8); glPopMatrix();

    glPopMatrix();
}

// Round BBQ table with food on top
static void drawBBQTable(float tx, float tz)
{
    float gY = -0.50f;
    float tableH = 2.20f;   // height matches 2x-scaled people waist

    // table leg (dark wood)
    setMat(0.38f, 0.24f, 0.10f);
    glPushMatrix();
    glTranslatef(tx, gY + tableH * 0.5f, tz);
    drawBox(0.22f, tableH, 0.22f);
    glPopMatrix();

    // table top (lighter wood, round-ish = octagonal box)
    setMat(0.58f, 0.40f, 0.20f);
    glPushMatrix();
    glTranslatef(tx, gY + tableH + 0.09f, tz);
    drawBox(2.60f, 0.18f, 2.60f);
    glPopMatrix();

    // food on table: BBQ meat chunks
    setMat(0.62f, 0.20f, 0.08f);
    float foods[4][2] = {{-0.6f,-0.5f},{0.6f,-0.5f},{-0.6f,0.5f},{0.6f,0.5f}};
    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
        glTranslatef(tx + foods[i][0], gY + tableH + 0.28f, tz + foods[i][1]);
        glScalef(1.0f, 0.5f, 1.0f);
        glutSolidSphere(0.22f, 10, 8);
        glPopMatrix();
    }

    // skewer sticks lying on table
    setMat(0.65f, 0.42f, 0.12f);
    for (int i = 0; i < 3; ++i) {
        glPushMatrix();
        glTranslatef(tx - 0.5f + i*0.5f, gY + tableH + 0.20f, tz);
        drawBox(0.05f, 0.05f, 1.50f);
        glPopMatrix();
    }
}

static void drawBBQCrowd(float cx, float cz)
{
    float fz = cz + 11.5f;   // open space in front of lounge

    // ============================================================
    //  GROUP OF 3 in a circle (equilateral triangle) around table
    // ============================================================
    float gx = cx - 5.0f;   // group center X
    float gz = fz - 1.0f;   // group center Z
    float R  = 3.2f;        // radius from center to each person

    // Table in the center
    drawBBQTable(gx, gz);

    // Person 1 (south, facing table = facing -z = yRot 0)
    drawBeachPersonSkewer(gx,           gz + R,
        0.88f,0.68f,0.50f,  0.90f,0.12f,0.10f,  0.15f,0.35f,0.85f,
        true,  false, 0.0f);

    // Person 2 (back-left, yRot 240 to face center)
    drawBeachPersonSkewer(gx - R*0.866f, gz - R*0.5f,
        0.82f,0.63f,0.44f,  0.12f,0.72f,0.25f,  0.95f,0.82f,0.05f,
        false, false, 240.0f);

    // Person 3 female (back-right, yRot 120 to face center)
    drawBeachPersonSkewer(gx + R*0.866f, gz - R*0.5f,
        0.90f,0.72f,0.55f,  0.95f,0.48f,0.05f,  0.95f,0.48f,0.05f,
        true,  true,  120.0f);

    // ============================================================
    //  COUPLE side by side, sharing one table
    // ============================================================
    float cpx = cx + 6.5f;   // couple center X
    float cpz = fz;

    // Shared table in front of couple
    drawBBQTable(cpx, cpz - 2.5f);

    // Woman (left): purple dress, faces table (-z = 0)
    drawBeachPersonSkewer(cpx - 1.8f, cpz,
        0.88f,0.68f,0.50f,  0.62f,0.15f,0.80f,  0.62f,0.15f,0.80f,
        false, true,  0.0f);

    // Man (right): white tank, teal shorts, faces table (-z = 0)
    drawBeachPersonSkewer(cpx + 1.8f, cpz,
        0.80f,0.62f,0.42f,  0.95f,0.95f,0.95f,  0.05f,0.58f,0.62f,
        true,  false, 0.0f);

    // Heart above couple
    {
        float hx = cpx, hy = -0.50f + 4.80f, hz = cpz - 0.5f;
        setMat(0.95f, 0.15f, 0.35f);
        glPushMatrix(); glTranslatef(hx-0.26f,hy,hz);
        glScalef(0.8f,0.8f,0.5f); glutSolidSphere(0.28f,10,8); glPopMatrix();
        glPushMatrix(); glTranslatef(hx+0.26f,hy,hz);
        glScalef(0.8f,0.8f,0.5f); glutSolidSphere(0.28f,10,8); glPopMatrix();
        glPushMatrix(); glTranslatef(hx,hy-0.28f,hz);
        glScalef(0.9f,0.7f,0.5f); glutSolidSphere(0.28f,10,8); glPopMatrix();
    }
}

// Whole beach resort - 2 rows of buildings each side, central garden, boundary + gate
static void drawBeachResort(float cx, float cz)
{
    float gY = -0.50f;

    // ---- GREEN GARDEN around the ENTIRE resort (matches the wider boundary) ----
    setMat(0.30f, 0.62f, 0.32f);
    glPushMatrix();
    glTranslatef(cx, gY + 0.01f, cz);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-23.5f, 0, -26.5f);
    glVertex3f( 23.5f, 0, -26.5f);
    glVertex3f( 23.5f, 0,  36.0f);
    glVertex3f(-23.5f, 0,  36.0f);
    glEnd();
    glPopMatrix();

    // ---- TAN SAND walkways between buildings (shifted further out) ----
    setMat(0.82f, 0.74f, 0.55f);
    // left walkway in front of left wing
    glPushMatrix();
    glTranslatef(cx, gY + 0.015f, cz);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-16.0f, 0, -22.0f);
    glVertex3f(-12.5f, 0, -22.0f);
    glVertex3f(-12.5f, 0,  22.0f);
    glVertex3f(-16.0f, 0,  22.0f);
    glEnd();
    glPopMatrix();
    // right walkway in front of right wing
    glPushMatrix();
    glTranslatef(cx, gY + 0.015f, cz);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(12.5f, 0, -22.0f);
    glVertex3f(16.0f, 0, -22.0f);
    glVertex3f(16.0f, 0,  22.0f);
    glVertex3f(12.5f, 0,  22.0f);
    glEnd();
    glPopMatrix();

    // central swimming pool - BIGGER
    setMat(0.30f, 0.70f, 0.92f);
    glPushMatrix();
    glTranslatef(cx, gY + 0.04f, cz - 4.0f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-5.0f, 0, -5.5f);
    glVertex3f( 5.0f, 0, -5.5f);
    glVertex3f( 5.0f, 0,  5.5f);
    glVertex3f(-5.0f, 0,  5.5f);
    glEnd();
    glPopMatrix();
    // pool deck - bigger
    setMat(0.94f, 0.90f, 0.78f);
    glPushMatrix();
    glTranslatef(cx, gY + 0.03f, cz - 4.0f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-6.0f, 0, -6.5f);
    glVertex3f( 6.0f, 0, -6.5f);
    glVertex3f( 6.0f, 0,  6.5f);
    glVertex3f(-6.0f, 0,  6.5f);
    glEnd();
    glPopMatrix();

    // ---- Decorative garden flowers + bushes (removed) ----
    /*for (int i = 0; i < 18; ++i) {
        float fx = cx + ((i % 3) - 1) * 4.5f;
        float fz = cz - 11.0f + (i / 3) * 4.0f;
        if (fabsf(fx - cx) < 4.5f && fz > cz - 8.0f && fz < cz - 0.5f) continue; // skip pool
        // alternating pink flower / green bush
        if (i % 2 == 0) setMat(0.85f, 0.30f, 0.65f);    // pink flowers
        else            setMat(0.22f, 0.55f, 0.28f);    // green bush
        glPushMatrix();
        glTranslatef(fx, gY + 0.25f, fz);
        glutSolidSphere(0.28, 8, 6);
        glPopMatrix();
    }*/
    // central fountain (cylinder + water dome) - moved to front for visibility
    setMat(0.95f, 0.95f, 0.95f);
    glPushMatrix();
    glTranslatef(cx, gY + 0.50f, cz + 20.0f);
    drawBox(1.4f, 1.00f, 1.4f);
    glPopMatrix();
    setMat(0.55f, 0.78f, 0.92f);
    glPushMatrix();
    glTranslatef(cx, gY + 1.20f, cz + 20.0f);
    glutSolidSphere(0.55, 14, 12);
    glPopMatrix();

    // walkway from gate to lobby (lighter sand stripe)
    setMat(0.95f, 0.90f, 0.78f);
    glPushMatrix();
    glTranslatef(cx, gY + 0.025f, cz + 26.0f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-1.5f, 0, -8.0f);
    glVertex3f( 1.5f, 0, -8.0f);
    glVertex3f( 1.5f, 0,  8.0f);
    glVertex3f(-1.5f, 0,  8.0f);
    glEnd();
    glPopMatrix();

    // ---- DECORATIVE GARDEN: trees + flowers all around the resort ----
    // pink flower beds along the front walkway (removed)
    /*for (int i = 0; i < 14; ++i) {
        float fx = cx + ((i % 2 == 0) ? -3.5f : 3.5f);
        float fz = cz + 17.0f + (i / 2) * 2.5f;
        if (i % 2 == 0) setMat(0.85f, 0.30f, 0.65f);
        else            setMat(0.22f, 0.55f, 0.28f);
        glPushMatrix();
        glTranslatef(fx, gY + 0.30f, fz);
        glutSolidSphere(0.32, 8, 6);
        glPopMatrix();
    }*/

    // small trees/bushes scattered around the perimeter
    srand(33333);
    for (int i = 0; i < 50; ++i) {
        int side = i % 4;       // 0=left, 1=right, 2=back, 3=front-corner
        float fx, fz;
        if (side == 0) {        // left side garden (between wing and wall)
            fx = cx + frand(-18.5f, -13.8f);
            fz = cz + frand(-21.0f, 32.0f);
        } else if (side == 1) { // right side garden
            fx = cx + frand(13.8f, 18.5f);
            fz = cz + frand(-21.0f, 32.0f);
        } else if (side == 2) { // back garden (behind buildings)
            fx = cx + frand(-18.0f, 18.0f);
            fz = cz + frand(-21.0f, -16.0f);
        } else {                // front corners
            fx = cx + ((i & 1) ? 1 : -1) * frand(10.0f, 18.0f);
            fz = cz + frand(24.0f, 32.0f);
        }
        // tree: small brown trunk + green canopy
        setMat(0.40f, 0.25f, 0.12f);
        glPushMatrix();
        glTranslatef(fx, gY + 0.5f, fz);
        drawBox(0.10f, 1.0f, 0.10f);
        glPopMatrix();
        setMat(0.20f + frand(0, 0.10f), 0.55f + frand(0, 0.15f), 0.22f);
        glPushMatrix();
        glTranslatef(fx, gY + 1.30f, fz);
        glutSolidSphere(0.65, 10, 8);
        glPopMatrix();
    }

    // colorful flower beds scattered around (removed)
    /*srand(44444);
    for (int i = 0; i < 40; ++i) {
        float fx = cx + frand(-18.0f, 18.0f);
        float fz = cz + frand(-22.0f, 33.0f);
        // skip inside garden strip (already decorated) and walkways area
        if (fabsf(fx - cx) < 9.0f && fz > cz - 13.0f && fz < cz + 16.0f) continue;
        // alternating pink/yellow flowers
        if (i % 3 == 0)      setMat(0.85f, 0.30f, 0.65f);
        else if (i % 3 == 1) setMat(0.95f, 0.85f, 0.20f);
        else                 setMat(0.85f, 0.45f, 0.25f);
        glPushMatrix();
        glTranslatef(fx, gY + 0.20f, fz);
        glutSolidSphere(0.25, 8, 6);
        glPopMatrix();
    }*/

    // ============================================================
    //  BUILDINGS: just 1 long outer row on each side (no inner cottages)
    // ============================================================
    // LEFT WING - 1 outer row, 6 buildings (pushed further left for wider garden)
    for (int i = 0; i < 6; ++i) {
        float bx = cx - 19.0f;
        float bz = cz - 13.0f + i * 5.5f;
        drawResortBlock(bx, bz, 5.4f, 5.5f, 4.6f);
    }
    // RIGHT WING - 1 outer row, 6 buildings (pushed further right)
    for (int i = 0; i < 6; ++i) {
        float bx = cx + 19.0f;
        float bz = cz - 13.0f + i * 5.5f;
        drawResortBlock(bx, bz, 5.4f, 5.5f, 4.6f);
    }

    // ============================================================
    //  BOUNDARY WALL with GATE in front (aligned with restaurant gate line z=-19)
    // ============================================================
    float bL = cx - 24.0f, bR = cx + 24.0f;  // wider for bigger perimeter garden
    float bF = -19.0f;                       // SAME ROW as restaurant / Seaview gates
    float bB = cz - 27.0f;                   // even more depth
    float wallH = 1.6f, baseY = gY;
    float gateGapHalf = 4.0f;

    setMat(0.95f, 0.96f, 0.97f);
    // FRONT wall - left of gate
    {
        float leftLen = (cx - gateGapHalf) - bL;
        glPushMatrix();
        glTranslatef((bL + cx - gateGapHalf) * 0.5f, baseY + wallH * 0.5f, bF);
        drawBox(leftLen, wallH, 0.25f);
        glPopMatrix();
    }
    // FRONT wall - right of gate
    {
        float rightLen = bR - (cx + gateGapHalf);
        glPushMatrix();
        glTranslatef((cx + gateGapHalf + bR) * 0.5f, baseY + wallH * 0.5f, bF);
        drawBox(rightLen, wallH, 0.25f);
        glPopMatrix();
    }
    // BACK wall
    glPushMatrix();
    glTranslatef(cx, baseY + wallH * 0.5f, bB);
    drawBox(bR - bL, wallH, 0.25f);
    glPopMatrix();
    // LEFT wall
    glPushMatrix();
    glTranslatef(bL, baseY + wallH * 0.5f, (bF + bB) * 0.5f);
    drawBox(0.25f, wallH, bF - bB);
    glPopMatrix();
    // RIGHT wall
    glPushMatrix();
    glTranslatef(bR, baseY + wallH * 0.5f, (bF + bB) * 0.5f);
    drawBox(0.25f, wallH, bF - bB);
    glPopMatrix();

    // light-blue top rail on all walls
    setMat(0.50f, 0.78f, 0.92f);
    {
        float leftLen = (cx - gateGapHalf) - bL;
        glPushMatrix();
        glTranslatef((bL + cx - gateGapHalf) * 0.5f, baseY + wallH + 0.05f, bF);
        drawBox(leftLen, 0.10f, 0.32f);
        glPopMatrix();
        float rightLen = bR - (cx + gateGapHalf);
        glPushMatrix();
        glTranslatef((cx + gateGapHalf + bR) * 0.5f, baseY + wallH + 0.05f, bF);
        drawBox(rightLen, 0.10f, 0.32f);
        glPopMatrix();
    }
    glPushMatrix();
    glTranslatef(cx, baseY + wallH + 0.05f, bB);
    drawBox(bR - bL, 0.10f, 0.32f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(bL, baseY + wallH + 0.05f, (bF + bB) * 0.5f);
    drawBox(0.32f, 0.10f, bF - bB);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(bR, baseY + wallH + 0.05f, (bF + bB) * 0.5f);
    drawBox(0.32f, 0.10f, bF - bB);
    glPopMatrix();

    // ---- GATE at the front ----
    float gateZ = bF;
    float pillarH = 4.5f;
    setMat(0.95f, 0.96f, 0.97f);
    // 2 white pillars
    glPushMatrix();
    glTranslatef(cx - gateGapHalf + 0.3f, baseY + pillarH * 0.5f, gateZ);
    drawBox(0.60f, pillarH, 0.60f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(cx + gateGapHalf - 0.3f, baseY + pillarH * 0.5f, gateZ);
    drawBox(0.60f, pillarH, 0.60f);
    glPopMatrix();
    // blue caps on each pillar
    setMat(0.40f, 0.70f, 0.90f);
    glPushMatrix();
    glTranslatef(cx - gateGapHalf + 0.3f, baseY + pillarH + 0.15f, gateZ);
    drawBox(0.78f, 0.30f, 0.78f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(cx + gateGapHalf - 0.3f, baseY + pillarH + 0.15f, gateZ);
    drawBox(0.78f, 0.30f, 0.78f);
    glPopMatrix();
    // top beam connecting pillars
    setMat(0.95f, 0.96f, 0.97f);
    glPushMatrix();
    glTranslatef(cx, baseY + pillarH - 0.05f, gateZ);
    drawBox(gateGapHalf * 2.0f + 0.6f, 0.55f, 0.80f);
    glPopMatrix();
    // (sign panel + Bay Resort text removed per user request)
    // dark vertical grille bars between pillars
    setMat(0.25f, 0.28f, 0.35f);
    int nBars = 8;
    for (int i = 0; i < nBars; ++i) {
        float bx = cx - gateGapHalf + 0.7f + i * ((gateGapHalf * 2.0f - 1.4f) / (nBars - 1));
        glPushMatrix();
        glTranslatef(bx, baseY + 1.30f, gateZ);
        drawBox(0.06f, 2.50f, 0.06f);
        glPopMatrix();
    }
}

// ============================================================
//  TRADITIONAL BAMBOO RESTAURANT (peaked thatched roof)
// ============================================================
// ---- cartoon fish ----
static void drawCartoonFish(float x, float y, float z, float scale,
                             float r, float g, float b)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);

    // body (stretched sphere)
    setMat(r, g, b);
    glPushMatrix();
    glScalef(1.6f, 0.9f, 0.5f);
    glutSolidSphere(1.0, 16, 12);
    glPopMatrix();

    // tail fin (triangle-shaped)
    glBegin(GL_TRIANGLES);
    glNormal3f(-1, 0, 0);
    glVertex3f(-1.5f,  0.0f, 0);
    glVertex3f(-2.4f,  0.8f, 0);
    glVertex3f(-2.4f, -0.8f, 0);
    glEnd();

    // top fin
    glBegin(GL_TRIANGLES);
    glNormal3f(0, 1, 0);
    glVertex3f( 0.0f, 0.85f, 0);
    glVertex3f(-0.6f, 1.5f,  0);
    glVertex3f(-0.9f, 0.85f, 0);
    glEnd();

    // eye (white sphere + black pupil)
    setMat(0.95f, 0.95f, 0.95f);
    glPushMatrix();
    glTranslatef(0.85f, 0.20f, 0.35f);
    glutSolidSphere(0.18, 10, 10);
    glPopMatrix();
    setMat(0.05f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(0.92f, 0.20f, 0.45f);
    glutSolidSphere(0.09, 8, 8);
    glPopMatrix();

    // mouth (small dark slit)
    setMat(0.20f, 0.10f, 0.10f);
    glPushMatrix();
    glTranslatef(1.30f, -0.05f, 0.30f);
    drawBox(0.15f, 0.06f, 0.10f);
    glPopMatrix();

    glPopMatrix();
}

// ---- cartoon octopus ----
static void drawCartoonOctopus(float x, float y, float z, float scale,
                                float r, float g, float b)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);

    // round head (slightly elongated)
    setMat(r, g, b);
    glPushMatrix();
    glScalef(1.1f, 1.2f, 1.1f);
    glutSolidSphere(1.0, 16, 12);
    glPopMatrix();

    // 8 tentacles hanging down (curved)
    setMat(r * 0.85f, g * 0.85f, b * 0.85f);
    for (int i = 0; i < 8; ++i) {
        float a = i * (2 * PI / 8.0f);
        float dx = cosf(a) * 0.6f;
        float dz = sinf(a) * 0.6f;
        // 3 segments per tentacle - curving outward then down
        for (int j = 0; j < 3; ++j) {
            float t = j / 3.0f;
            float seg_x = dx * (1.0f + t * 0.5f);
            float seg_z = dz * (1.0f + t * 0.5f);
            float seg_y = -0.8f - j * 0.4f;
            float seg_size = 0.30f - j * 0.07f;
            glPushMatrix();
            glTranslatef(seg_x, seg_y, seg_z);
            glutSolidSphere(seg_size, 8, 6);
            glPopMatrix();
        }
    }

    // 2 big white eyes
    setMat(0.95f, 0.95f, 0.95f);
    glPushMatrix();
    glTranslatef( 0.40f, 0.40f, 0.85f);
    glutSolidSphere(0.22, 10, 10);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-0.40f, 0.40f, 0.85f);
    glutSolidSphere(0.22, 10, 10);
    glPopMatrix();
    // pupils
    setMat(0.05f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef( 0.42f, 0.42f, 1.05f);
    glutSolidSphere(0.11, 8, 8);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-0.42f, 0.42f, 1.05f);
    glutSolidSphere(0.11, 8, 8);
    glPopMatrix();

    glPopMatrix();
}

// ---- cartoon starfish (5-arm flat star) ----
static void drawCartoonStarfish(float x, float y, float z, float scale,
                                 float r, float g, float b)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);

    setMat(r, g, b);
    // center disc
    glPushMatrix();
    glScalef(0.5f, 0.25f, 0.5f);
    glutSolidSphere(1.0, 12, 8);
    glPopMatrix();

    // 5 arms radiating
    for (int i = 0; i < 5; ++i) {
        float a = i * (2 * PI / 5.0f);
        float ax = cosf(a);
        float az = sinf(a);
        glBegin(GL_TRIANGLES);
        // top face
        glNormal3f(0, 1, 0);
        glVertex3f(0, 0.1f, 0);
        glVertex3f(ax * 1.0f, 0.1f, az * 1.0f);
        glVertex3f(ax * 0.4f - az * 0.3f, 0.1f, az * 0.4f + ax * 0.3f);
        glVertex3f(0, 0.1f, 0);
        glVertex3f(ax * 0.4f + az * 0.3f, 0.1f, az * 0.4f - ax * 0.3f);
        glVertex3f(ax * 1.0f, 0.1f, az * 1.0f);
        // bottom face
        glNormal3f(0, -1, 0);
        glVertex3f(0, -0.1f, 0);
        glVertex3f(ax * 0.4f - az * 0.3f, -0.1f, az * 0.4f + ax * 0.3f);
        glVertex3f(ax * 1.0f, -0.1f, az * 1.0f);
        glVertex3f(0, -0.1f, 0);
        glVertex3f(ax * 1.0f, -0.1f, az * 1.0f);
        glVertex3f(ax * 0.4f + az * 0.3f, -0.1f, az * 0.4f - ax * 0.3f);
        glEnd();
    }

    // bumpy texture - small spheres scattered on top
    setMat(r * 1.15f, g * 1.15f, b * 1.15f);
    for (int i = 0; i < 8; ++i) {
        float a = frand(0, 2 * PI);
        float r2 = frand(0.2f, 0.6f);
        glPushMatrix();
        glTranslatef(cosf(a) * r2, 0.18f, sinf(a) * r2);
        glutSolidSphere(0.07, 6, 6);
        glPopMatrix();
    }

    glPopMatrix();
}

// ---- cartoon dolphin (sleek body + dorsal + tail fin) ----
static void drawCartoonDolphin(float x, float y, float z, float scale,
                                float r, float g, float b)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);

    // sleek body - cone-like with rounded head
    setMat(r, g, b);
    glPushMatrix();
    glScalef(2.0f, 0.7f, 0.7f);
    glutSolidSphere(1.0, 16, 12);
    glPopMatrix();

    // dorsal fin on top
    glBegin(GL_TRIANGLES);
    glNormal3f(0, 0, 1);
    glVertex3f( 0.0f, 0.70f, 0);
    glVertex3f(-0.3f, 1.40f, 0);
    glVertex3f(-0.6f, 0.70f, 0);
    glNormal3f(0, 0, -1);
    glVertex3f( 0.0f, 0.70f, 0);
    glVertex3f(-0.6f, 0.70f, 0);
    glVertex3f(-0.3f, 1.40f, 0);
    glEnd();

    // tail fin (curved horizontal)
    glBegin(GL_TRIANGLES);
    glNormal3f(-1, 0, 0);
    glVertex3f(-1.9f,  0.0f, 0);
    glVertex3f(-2.6f,  0.5f, 0.6f);
    glVertex3f(-2.6f, -0.5f, 0.6f);
    glVertex3f(-1.9f,  0.0f, 0);
    glVertex3f(-2.6f, -0.5f,-0.6f);
    glVertex3f(-2.6f,  0.5f,-0.6f);
    glEnd();

    // belly (lighter color)
    setMat(0.95f, 0.95f, 0.98f);
    glPushMatrix();
    glTranslatef(0, -0.35f, 0);
    glScalef(1.6f, 0.30f, 0.55f);
    glutSolidSphere(1.0, 12, 10);
    glPopMatrix();

    // eye
    setMat(0.05f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(1.30f, 0.20f, 0.40f);
    glutSolidSphere(0.10, 8, 8);
    glPopMatrix();

    // smile (small dark line on side)
    setMat(0.10f, 0.10f, 0.15f);
    glPushMatrix();
    glTranslatef(1.65f, -0.05f, 0.35f);
    drawBox(0.20f, 0.04f, 0.10f);
    glPopMatrix();

    glPopMatrix();
}

// ---- cartoon crab ----
static void drawCartoonCrab(float x, float y, float z, float scale)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);

    // body (oval shell - red)
    setMat(0.85f, 0.18f, 0.15f);
    glPushMatrix();
    glScalef(1.4f, 0.8f, 1.0f);
    glutSolidSphere(1.0, 14, 10);
    glPopMatrix();

    // two big front claws
    setMat(0.95f, 0.25f, 0.20f);
    glPushMatrix();
    glTranslatef(-1.6f, 0.0f, 0.6f);
    glScalef(0.7f, 0.5f, 0.5f);
    glutSolidSphere(1.0, 10, 8);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-1.6f, 0.0f, -0.6f);
    glScalef(0.7f, 0.5f, 0.5f);
    glutSolidSphere(1.0, 10, 8);
    glPopMatrix();

    // 6 legs (3 each side)
    setMat(0.75f, 0.15f, 0.12f);
    for (int i = 0; i < 3; ++i) {
        float angle = -25.0f + i * 25.0f;
        glPushMatrix();
        glTranslatef(0.3f, 0.0f, 1.1f);
        glRotatef(angle, 0, 1, 0);
        drawBox(0.15f, 0.10f, 0.8f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.3f, 0.0f, -1.1f);
        glRotatef(-angle, 0, 1, 0);
        drawBox(0.15f, 0.10f, 0.8f);
        glPopMatrix();
    }

    // eyes on stalks (white + black)
    setMat(0.95f, 0.95f, 0.95f);
    glPushMatrix();
    glTranslatef(0.5f, 0.6f,  0.30f);
    glutSolidSphere(0.18, 8, 8);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.5f, 0.6f, -0.30f);
    glutSolidSphere(0.18, 8, 8);
    glPopMatrix();
    setMat(0.05f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(0.62f, 0.65f,  0.30f);
    glutSolidSphere(0.08, 6, 6);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.62f, 0.65f, -0.30f);
    glutSolidSphere(0.08, 6, 6);
    glPopMatrix();

    glPopMatrix();
}

// ---- cartoon shrimp ----
static void drawCartoonShrimp(float x, float y, float z, float scale)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);

    setMat(0.95f, 0.55f, 0.45f);
    // segmented body (3 spheres)
    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
        glTranslatef(-0.4f * i, 0.05f * i * i * 0.2f, 0);
        glScalef(0.55f - i * 0.06f, 0.55f - i * 0.06f, 0.55f - i * 0.06f);
        glutSolidSphere(1.0, 10, 8);
        glPopMatrix();
    }
    // tail fin
    setMat(0.90f, 0.40f, 0.30f);
    glBegin(GL_TRIANGLES);
    glNormal3f(-1, 0, 0);
    glVertex3f(-1.6f,  0.0f, 0);
    glVertex3f(-2.1f,  0.4f, 0.3f);
    glVertex3f(-2.1f,  0.4f,-0.3f);
    glVertex3f(-1.6f,  0.0f, 0);
    glVertex3f(-2.1f, -0.4f, 0.3f);
    glVertex3f(-2.1f, -0.4f,-0.3f);
    glEnd();

    // eye
    setMat(0.10f, 0.10f, 0.10f);
    glPushMatrix();
    glTranslatef(0.50f, 0.30f, 0.25f);
    glutSolidSphere(0.07, 6, 6);
    glPopMatrix();

    glPopMatrix();
}

// ---- a parked car with style variation (all small - no buses/trucks) ----
// style: 0=sedan  1=hatchback  2=compact-suv  3=convertible  4=sports-low  5=mini
static void drawParkedCar(float x, float z, float rot,
                          float r, float g, float b, int style)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(rot, 0, 1, 0);

    // pick dimensions by style
    float bL, bH, bD;   // body length, height, depth
    float cL, cH, cD;   // cabin length, height, depth
    float cX;           // cabin x offset
    bool  hasRoof = true;

    switch (style) {
        case 0: bL=2.4f; bH=0.50f; bD=1.10f; cL=1.4f; cH=0.50f; cD=1.00f; cX=-0.10f; break;  // sedan
        case 1: bL=2.0f; bH=0.55f; bD=1.05f; cL=1.3f; cH=0.55f; cD=0.95f; cX=-0.20f; break;  // hatchback
        case 2: bL=2.2f; bH=0.60f; bD=1.10f; cL=1.7f; cH=0.70f; cD=1.05f; cX=-0.05f; break;  // mini-SUV
        case 3: bL=2.4f; bH=0.50f; bD=1.10f; cL=0.0f; cH=0.0f;  cD=0.0f;  cX=0;     hasRoof=false; break; // convertible
        case 4: bL=2.6f; bH=0.40f; bD=1.05f; cL=1.0f; cH=0.35f; cD=0.95f; cX=-0.10f; break;  // sports
        default:bL=1.8f; bH=0.50f; bD=1.00f; cL=1.2f; cH=0.50f; cD=0.95f; cX=-0.10f; break;  // mini
    }

    // body
    setMat(r, g, b);
    glPushMatrix(); glTranslatef(0, bH * 0.5f, 0); drawBox(bL, bH, bD); glPopMatrix();
    // cabin (if any)
    if (hasRoof) {
        glPushMatrix();
        glTranslatef(cX, bH + cH * 0.5f, 0);
        drawBox(cL, cH, cD);
        glPopMatrix();

        // windows (dark tint)
        setMat(0.25f, 0.30f, 0.40f);
        glPushMatrix();
        glTranslatef(cX, bH + cH * 0.5f, cD * 0.5f + 0.01f);
        drawBox(cL * 0.92f, cH * 0.75f, 0.02f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(cX, bH + cH * 0.5f, -cD * 0.5f - 0.01f);
        drawBox(cL * 0.92f, cH * 0.75f, 0.02f);
        glPopMatrix();
    } else {
        // convertible - show 2 seat backs poking up
        setMat(0.20f, 0.20f, 0.25f);
        glPushMatrix(); glTranslatef(-0.10f, bH + 0.25f,  0.25f); drawBox(0.35f, 0.40f, 0.30f); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.10f, bH + 0.25f, -0.25f); drawBox(0.35f, 0.40f, 0.30f); glPopMatrix();
    }

    // headlights
    setMat(1.0f, 0.95f, 0.70f);
    glPushMatrix(); glTranslatef(bL * 0.5f + 0.01f, 0.18f,  bD * 0.32f); drawBox(0.04f, 0.14f, 0.18f); glPopMatrix();
    glPushMatrix(); glTranslatef(bL * 0.5f + 0.01f, 0.18f, -bD * 0.32f); drawBox(0.04f, 0.14f, 0.18f); glPopMatrix();

    // wheels
    setMat(0.05f, 0.05f, 0.05f);
    float wxOff = bL * 0.33f;
    float wzOff = bD * 0.50f;
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
            glPushMatrix();
            glTranslatef(sx * wxOff, -0.18f, sz * wzOff);
            glRotatef(90, 0, 1, 0);
            GLUquadric *q = gluNewQuadric();
            gluDisk(q, 0, 0.30, 12, 1);
            gluCylinder(q, 0.30, 0.30, 0.15, 12, 1);
            gluDeleteQuadric(q);
            glPopMatrix();
        }

    glPopMatrix();
}

// helper - bamboo pole (cylinder with darker nodes)
static void drawBambooPole(float x, float y, float z, float height, float radius)
{
    GLUquadric *q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);

    // VIVID bamboo body - golden yellow-green
    setMat(0.92f, 0.88f, 0.62f);
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, radius, radius, height, 12, 2);
    glPopMatrix();

    // bamboo nodes/rings - subtle darker tan
    setMat(0.55f, 0.42f, 0.20f);
    int nodes = (int)(height / 0.8f);
    for (int i = 1; i <= nodes; ++i) {
        glPushMatrix();
        glTranslatef(x, y + (i * 0.8f), z);
        glRotatef(-90, 1, 0, 0);
        gluCylinder(q, radius * 1.12f, radius * 1.12f, 0.07f, 12, 1);
        glPopMatrix();
    }
    gluDeleteQuadric(q);
}

// ============================================================
//  ICE CREAM STALL (beach snack kiosk)
// ============================================================
static void drawIceCreamStall(float cx, float cz)
{
    float gY = -0.50f;

    setMat(0.55f, 0.35f, 0.18f);
    glPushMatrix(); glTranslatef(cx, gY + 0.60f, cz); drawBox(26.0f, 1.20f, 1.80f); glPopMatrix();
    setMat(0.75f, 0.55f, 0.30f);
    glPushMatrix(); glTranslatef(cx, gY + 1.22f, cz); drawBox(26.2f, 0.12f, 2.0f); glPopMatrix();

    float flavR[] = {0.95f,0.98f,0.55f,0.90f,0.80f};
    float flavG[] = {0.40f,0.82f,0.85f,0.60f,0.40f};
    float flavB[] = {0.55f,0.85f,0.45f,0.85f,0.70f};
    for (int i = 0; i < 5; ++i) {
        float tx = cx - 10.0f + i * 5.0f;
        setMat(0.95f,0.95f,0.95f);
        glPushMatrix(); glTranslatef(tx, gY+1.40f, cz-0.20f); drawBox(0.55f,0.30f,0.55f); glPopMatrix();
        setMat(flavR[i],flavG[i],flavB[i]);
        glPushMatrix(); glTranslatef(tx, gY+1.62f, cz-0.20f); glutSolidSphere(0.22f,12,10); glPopMatrix();
    }

    setMat(0.45f,0.28f,0.14f);
    float postX[2]={cx-12.5f,cx+12.5f}, postZ[2]={cz-1.0f,cz+1.0f};
    for (int px=0;px<2;++px) for (int pz=0;pz<2;++pz) {
        glPushMatrix(); glTranslatef(postX[px],gY+2.80f,postZ[pz]); drawBox(0.16f,5.60f,0.16f); glPopMatrix();
    }

    float aY=gY+5.60f; float aW=27.0f; float aD=2.8f; int nS=12;
    for (int s=0;s<nS;++s) {
        float t0=s/(float)nS, t1=(s+1)/(float)nS;
        float x0=cx-aW*0.5f+t0*aW, x1=cx-aW*0.5f+t1*aW;
        if (s%2==0) setMat(0.92f,0.15f,0.15f); else setMat(0.98f,0.98f,0.98f);
        glBegin(GL_QUADS);
        glNormal3f(0,1,0.4f);
        glVertex3f(x0,aY,      cz+aD*0.5f); glVertex3f(x1,aY,      cz+aD*0.5f);
        glVertex3f(x1,aY-0.8f, cz-aD*0.5f); glVertex3f(x0,aY-0.8f, cz-aD*0.5f);
        glEnd();
    }
    setMat(0.92f,0.15f,0.15f);
    for (int f=0;f<10;++f) {
        float fx=cx-aW*0.5f+f*(aW/10.0f);
        glPushMatrix(); glTranslatef(fx+aW/20.0f,aY-0.75f,cz+aD*0.5f); drawBox(aW/10.0f*0.7f,0.25f,0.06f); glPopMatrix();
    }

    float coneY=aY+0.10f;
    setMat(0.92f,0.72f,0.42f);
    glBegin(GL_TRIANGLES);
    glNormal3f(0,0,1);
    glVertex3f(cx-0.50f,coneY,       cz+aD*0.5f+0.05f);
    glVertex3f(cx+0.50f,coneY,       cz+aD*0.5f+0.05f);
    glVertex3f(cx,      coneY+1.20f, cz+aD*0.5f+0.05f);
    glEnd();
    setMat(0.98f,0.75f,0.85f);
    glPushMatrix(); glTranslatef(cx,coneY+1.30f,cz+aD*0.5f+0.05f); glutSolidSphere(0.42f,14,12); glPopMatrix();

    // ── Banner — same style as BBQ sign ──────────────────────────
    {
        float bannerW = 22.0f;
        float bannerH = 2.4f;
        float bannerY = aY - 1.20f;
        float bannerZ = cz + aD * 0.5f;
        // 2 hanging ropes
        setMat(0.95f,0.95f,0.95f);
        glPushMatrix(); glTranslatef(cx-bannerW*0.40f, bannerY+bannerH*0.5f+0.25f, bannerZ); drawBox(0.03f,0.50f,0.03f); glPopMatrix();
        glPushMatrix(); glTranslatef(cx+bannerW*0.40f, bannerY+bannerH*0.5f+0.25f, bannerZ); drawBox(0.03f,0.50f,0.03f); glPopMatrix();
        // black background
        setMat(0.08f,0.08f,0.08f);
        glPushMatrix(); glTranslatef(cx, bannerY, bannerZ); drawBox(bannerW, bannerH, 0.10f); glPopMatrix();
        // white text
        draw3DText("ICE CREAM", cx, bannerY, bannerZ+0.07f, 1.80f, 1.00f,1.00f,1.00f);
    }

    float vx=cx-0.5f, vz=cz+0.60f;
    setMat(0.20f,0.20f,0.45f);
    glPushMatrix(); glTranslatef(vx-0.08f,gY+0.38f,vz); drawBox(0.14f,0.75f,0.14f); glPopMatrix();
    glPushMatrix(); glTranslatef(vx+0.08f,gY+0.38f,vz); drawBox(0.14f,0.75f,0.14f); glPopMatrix();
    setMat(0.96f,0.96f,0.96f);
    glPushMatrix(); glTranslatef(vx,gY+1.00f,vz); drawBox(0.35f,0.55f,0.22f); glPopMatrix();
    setMat(0.92f,0.78f,0.62f);
    glPushMatrix(); glTranslatef(vx,gY+1.40f,vz); glutSolidSphere(0.14f,12,10); glPopMatrix();
    setMat(0.95f,0.95f,0.95f);
    glPushMatrix(); glTranslatef(vx,gY+1.58f,vz); drawBox(0.25f,0.08f,0.25f); glPopMatrix();
    glPushMatrix(); glTranslatef(vx,gY+1.70f,vz); glutSolidSphere(0.15f,10,8); glPopMatrix();
}

// ============================================================
//  SWIMMER ANIMATION
// ============================================================
static void drawSwimmer(float x, float z, float phase)
{
    float gY=-0.45f;
    glPushMatrix();
    glTranslatef(x, gY, z);
    float bob=sinf(phase*1.5f)*0.08f;
    glTranslatef(0,bob,0);

    float hue=fmodf(phase*0.05f,1.0f);
    float sR,sG,sB;
    if (hue<0.33f)      {sR=0.20f;sG=0.50f;sB=0.90f;}
    else if (hue<0.66f) {sR=0.90f;sG=0.20f;sB=0.20f;}
    else                {sR=0.20f;sG=0.75f;sB=0.30f;}

    setMat(sR,sG,sB);
    glPushMatrix(); glScalef(1.8f,0.28f,0.55f); glutSolidSphere(1.0f,14,10); glPopMatrix();
    setMat(0.92f,0.78f,0.62f);
    glPushMatrix(); glTranslatef(1.0f,0.10f,0.0f); glutSolidSphere(0.20f,12,10); glPopMatrix();
    setMat(sR*0.7f,sG*0.7f,sB*0.7f);
    glPushMatrix(); glTranslatef(1.0f,0.18f,0.0f); glScalef(0.20f,0.12f,0.20f); glutSolidSphere(1.0f,10,8); glPopMatrix();
    setMat(0.15f,0.15f,0.15f);
    glPushMatrix(); glTranslatef(1.18f,0.12f, 0.08f); glutSolidSphere(0.055f,8,6); glPopMatrix();
    glPushMatrix(); glTranslatef(1.18f,0.12f,-0.08f); glutSolidSphere(0.055f,8,6); glPopMatrix();

    float rA=sinf(phase)*75.0f, lA=sinf(phase+PI)*75.0f;
    setMat(0.92f,0.78f,0.62f);
    glPushMatrix(); glTranslatef(0,0,-0.38f); glRotatef(rA,1,0,0); glTranslatef(0,0,-0.45f); drawBox(0.60f,0.10f,0.10f); glPopMatrix();
    glPushMatrix(); glTranslatef(0,0, 0.38f); glRotatef(lA,1,0,0); glTranslatef(0,0, 0.45f); drawBox(0.60f,0.10f,0.10f); glPopMatrix();

    float kR=sinf(phase*3.0f)*20.0f, kL=sinf(phase*3.0f+PI)*20.0f;
    setMat(sR,sG,sB);
    glPushMatrix(); glTranslatef(-0.80f,0, 0.18f); glRotatef(kR,1,0,0); drawBox(0.55f,0.14f,0.14f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.80f,0,-0.18f); glRotatef(kL,1,0,0); drawBox(0.55f,0.14f,0.14f); glPopMatrix();

    glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    float sp=(sinf(phase)>0.5f)?0.70f:0.20f;
    glColor4f(1,1,1,sp); glPointSize(4.0f);
    glBegin(GL_POINTS);
    for (int s=0;s<6;++s) { float sa=s*(2*PI/6.0f)+phase; glVertex3f(x+cosf(sa)*0.55f,gY+bob+0.05f,z+sinf(sa)*0.35f); }
    glEnd();
    glPointSize(1.0f); glDisable(GL_BLEND); glEnable(GL_LIGHTING);
    glPopMatrix();
}

// ============================================================
//  LIGHTNING / THUNDER EFFECT
// ============================================================
static void drawLightning()
{
    if (thunderFlash<0.02f) return;

    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(-1,1,-1,1);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.9f,0.9f,1.0f,thunderFlash*0.35f);
    glBegin(GL_QUADS); glVertex2f(-1,-1); glVertex2f(1,-1); glVertex2f(1,1); glVertex2f(-1,1); glEnd();
    glDisable(GL_BLEND);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);

    glDisable(GL_LIGHTING);
    glLineWidth(3.0f); glColor3f(1.0f,1.0f,0.6f*thunderFlash+0.4f);
    float bx=thunderX, bz=thunderZ, sY=60.0f, eY=-0.5f;
    glBegin(GL_LINE_STRIP);
    srand((unsigned)(thunderTimer*1000));
    glVertex3f(bx,sY,bz);
    for (int i=1;i<8;++i) {
        float t=i/8.0f, py=sY+(eY-sY)*t;
        glVertex3f(bx+frand(-3.0f,3.0f),py,bz+frand(-1.5f,1.5f));
    }
    glVertex3f(bx,eY,bz);
    glEnd();
    glLineWidth(1.5f); glColor3f(0.9f,0.9f,1.0f);
    glBegin(GL_LINE_STRIP);
    float brY=sY*0.4f; glVertex3f(bx+1.0f,brY,bz);
    for (int i=0;i<4;++i) { float py=brY+(-5.0f-brY)*(i/4.0f); glVertex3f(bx+1.0f+frand(-2,2),py,bz+frand(-1,1)); }
    glEnd();
    glLineWidth(1.0f);
    glPointSize(8.0f); glColor3f(1,1,0.5f);
    glBegin(GL_POINTS); glVertex3f(bx,eY+0.1f,bz); glEnd();
    glPointSize(1.0f); glEnable(GL_LIGHTING);
}

// ============================================================
//  DERA RESORT — Duplex hotel (cx=62, between Restaurant & Shop)
//  No overlap: restaurant right=45, shop left=78, resort 48..76
// ============================================================
static void drawParkedCar(float cx2, float cz2, float yRot,
                          float r, float g, float b)
{
    float gY = -0.50f;
    glPushMatrix();
    glTranslatef(cx2, gY, cz2);
    glRotatef(yRot, 0, 1, 0);

    // body lower
    setMat(r, g, b);
    glPushMatrix(); glTranslatef(0, 0.38f, 0); drawBox(3.60f, 0.65f, 1.65f); glPopMatrix();
    // cabin
    glPushMatrix(); glTranslatef(-0.15f, 0.95f, 0); drawBox(2.10f, 0.65f, 1.52f); glPopMatrix();
    // windshield / rear glass
    setMat(0.26f, 0.32f, 0.40f);
    glPushMatrix(); glTranslatef( 0.88f, 0.92f, 0); drawBox(0.06f, 0.55f, 1.40f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.20f, 0.92f, 0); drawBox(0.06f, 0.55f, 1.40f); glPopMatrix();
    // side windows
    glPushMatrix(); glTranslatef(-0.15f, 0.95f, 0.78f); drawBox(1.90f, 0.45f, 0.04f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.15f, 0.95f,-0.78f); drawBox(1.90f, 0.45f, 0.04f); glPopMatrix();
    // bumpers
    setMat(r*0.55f, g*0.55f, b*0.55f);
    glPushMatrix(); glTranslatef( 1.85f, 0.26f, 0); drawBox(0.12f, 0.24f, 1.50f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.85f, 0.26f, 0); drawBox(0.12f, 0.24f, 1.50f); glPopMatrix();
    // headlights
    if (night || sunset) {
        GLfloat em[4]={1.0f,0.95f,0.7f,1}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em);
    }
    setMat(1.0f, 0.95f, 0.75f);
    glPushMatrix(); glTranslatef(1.82f, 0.38f, 0.50f); drawBox(0.05f, 0.18f, 0.22f); glPopMatrix();
    glPushMatrix(); glTranslatef(1.82f, 0.38f,-0.50f); drawBox(0.05f, 0.18f, 0.22f); glPopMatrix();
    { GLfloat z0[4]={0,0,0,1}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0); }
    // tail lights
    setMat(0.70f, 0.08f, 0.08f);
    glPushMatrix(); glTranslatef(-1.82f, 0.38f, 0.50f); drawBox(0.05f, 0.16f, 0.20f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.82f, 0.38f,-0.50f); drawBox(0.05f, 0.16f, 0.20f); glPopMatrix();
    // wheels (4)
    setMat(0.12f, 0.12f, 0.12f);
    float wx[2]={1.0f,-1.0f};
    float wz[2]={0.86f,-0.86f};
    for(int wi=0;wi<2;++wi) for(int wj=0;wj<2;++wj) {
        GLUquadric*q=gluNewQuadric();
        glPushMatrix(); glTranslatef(wx[wi],0.20f,wz[wj]);
        glRotatef(90,0,1,0); gluCylinder(q,0.28f,0.28f,0.25f,14,2);
        gluDisk(q,0,0.28f,14,1); glPopMatrix(); gluDeleteQuadric(q);
        setMat(0.55f,0.55f,0.58f);
        glPushMatrix(); glTranslatef(wx[wi],0.20f,wz[wj]+0.05f*(wj==0?1:-1));
        glutSolidSphere(0.16f,10,8); glPopMatrix();
        setMat(0.12f,0.12f,0.12f);
    }
    glPopMatrix();
}

static void drawDeraResort(float cx, float cz)
{
    float gY   = -0.50f;
    float W    = 28.0f;
    float D    = 18.0f;
    float GH   =  5.50f;   // ground-floor ceiling height
    float UH   =  5.50f;   // upper-floor height
    float TH   = GH + UH;  // total = 11
    float platH = 0.35f;
    float fY   = gY + platH;

    // ── Forecourt / driveway ─────────────────────────────────
    setMat(0.76f, 0.74f, 0.70f);   // light asphalt
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(cx-W*0.50f, gY+0.003f, cz+D*0.5f);
    glVertex3f(cx+W*0.50f, gY+0.003f, cz+D*0.5f);
    glVertex3f(cx+W*0.50f, gY+0.003f, cz+D*0.5f+20.0f);
    glVertex3f(cx-W*0.50f, gY+0.003f, cz+D*0.5f+20.0f);
    glEnd();
    // driveway lane markings
    setMat(0.90f, 0.88f, 0.82f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    for(int i=0;i<6;++i){
        float zp=cz+D*0.5f+2.5f+i*2.8f;
        glVertex3f(cx-0.10f,gY+0.006f,zp); glVertex3f(cx+0.10f,gY+0.006f,zp);
        glVertex3f(cx+0.10f,gY+0.006f,zp+1.5f); glVertex3f(cx-0.10f,gY+0.006f,zp+1.5f);
    }
    glEnd();
    // parking bay lines
    setMat(0.96f, 0.94f, 0.88f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    float bayZstart = cz+D*0.5f+3.0f;
    float bayXs[5]={cx-W*0.50f, cx-W*0.24f, cx, cx+W*0.24f, cx+W*0.50f};
    for(int b=0;b<5;++b){
        // bay divider lines
        glVertex3f(bayXs[b]-0.08f,gY+0.007f,bayZstart);
        glVertex3f(bayXs[b]+0.08f,gY+0.007f,bayZstart);
        glVertex3f(bayXs[b]+0.08f,gY+0.007f,bayZstart+5.5f);
        glVertex3f(bayXs[b]-0.08f,gY+0.007f,bayZstart+5.5f);
    }
    glEnd();

    // ── Boundary wall + gate ──────────────────────────────────
    setMat(0.88f, 0.85f, 0.78f);
    // Left boundary
    glPushMatrix(); glTranslatef(cx-W*0.50f-0.15f, gY+0.75f, cz+D*0.5f+10.0f);
    drawBox(0.30f, 1.50f, 20.0f); glPopMatrix();
    // Right boundary
    glPushMatrix(); glTranslatef(cx+W*0.50f+0.15f, gY+0.75f, cz+D*0.5f+10.0f);
    drawBox(0.30f, 1.50f, 20.0f); glPopMatrix();
    // Gate pillars
    setMat(0.92f, 0.88f, 0.78f);
    glPushMatrix(); glTranslatef(cx-2.8f, gY+1.50f, cz+D*0.5f+19.5f); drawBox(0.55f, 3.0f, 0.55f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+2.8f, gY+1.50f, cz+D*0.5f+19.5f); drawBox(0.55f, 3.0f, 0.55f); glPopMatrix();
    // Gate pillar caps
    setMat(0.75f, 0.62f, 0.12f);
    glPushMatrix(); glTranslatef(cx-2.8f, gY+3.10f, cz+D*0.5f+19.5f); glScalef(1,0.4f,1); glutSolidSphere(0.40f,10,8); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+2.8f, gY+3.10f, cz+D*0.5f+19.5f); glScalef(1,0.4f,1); glutSolidSphere(0.40f,10,8); glPopMatrix();
    // "DERA RESORT" gate sign bar
    setMat(0.10f, 0.10f, 0.12f);
    glPushMatrix(); glTranslatef(cx, gY+3.60f, cz+D*0.5f+19.5f); drawBox(5.20f, 0.80f, 0.22f); glPopMatrix();
    setMat(0.78f, 0.64f, 0.12f);
    glPushMatrix(); glTranslatef(cx, gY+3.60f, cz+D*0.5f+19.6f); drawBox(5.30f, 0.90f, 0.05f); glPopMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(0.04f, 0.04f, 0.04f);   // black
    drawStrokeText_("DERA RESORT", cx-2.20f, gY+3.52f, cz+D*0.5f+19.75f, 0.0035f, 1.0f);
    glEnable(GL_LIGHTING);

    // ── Platform ──────────────────────────────────────────────
    setMat(0.90f, 0.87f, 0.80f);
    glPushMatrix(); glTranslatef(cx, gY+platH*0.5f, cz); drawBox(W, platH, D); glPopMatrix();
    // marble floor
    setMat(0.97f, 0.97f, 0.95f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(cx-W*0.5f,fY+0.002f,cz-D*0.5f); glVertex3f(cx+W*0.5f,fY+0.002f,cz-D*0.5f);
    glVertex3f(cx+W*0.5f,fY+0.002f,cz+D*0.5f); glVertex3f(cx-W*0.5f,fY+0.002f,cz+D*0.5f);
    glEnd();
    setMat(0.68f,0.68f,0.68f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    for(int i=0;i<=7;++i){float xp=cx-W*0.5f+i*(W/7.0f);
        glVertex3f(xp,fY+0.004f,cz-D*0.5f); glVertex3f(xp+0.04f,fY+0.004f,cz-D*0.5f);
        glVertex3f(xp+0.04f,fY+0.004f,cz+D*0.5f); glVertex3f(xp,fY+0.004f,cz+D*0.5f);}
    for(int i=0;i<=5;++i){float zp=cz-D*0.5f+i*(D/5.0f);
        glVertex3f(cx-W*0.5f,fY+0.004f,zp); glVertex3f(cx+W*0.5f,fY+0.004f,zp);
        glVertex3f(cx+W*0.5f,fY+0.004f,zp+0.04f); glVertex3f(cx-W*0.5f,fY+0.004f,zp+0.04f);}
    glEnd();

    // ── GROUND FLOOR walls ────────────────────────────────────
    setMat(0.95f, 0.92f, 0.86f);   // warm cream
    // back wall
    glPushMatrix(); glTranslatef(cx, fY+GH*0.5f, cz-D*0.5f); drawBox(W, GH, 0.30f); glPopMatrix();
    // left wall
    glPushMatrix(); glTranslatef(cx-W*0.5f, fY+GH*0.5f, cz); drawBox(0.30f, GH, D); glPopMatrix();
    // right wall
    glPushMatrix(); glTranslatef(cx+W*0.5f, fY+GH*0.5f, cz); drawBox(0.30f, GH, D); glPopMatrix();
    // front wall — left & right of entrance
    glPushMatrix(); glTranslatef(cx-W*0.5f+3.50f, fY+GH*0.5f, cz+D*0.5f); drawBox(7.0f, GH, 0.30f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+W*0.5f-3.50f, fY+GH*0.5f, cz+D*0.5f); drawBox(7.0f, GH, 0.30f); glPopMatrix();
    // front wall above arch
    glPushMatrix(); glTranslatef(cx, fY+GH-0.70f, cz+D*0.5f); drawBox(W-14.0f, 1.40f, 0.30f); glPopMatrix();

    // dark brown cornice band at top of ground floor
    setMat(0.35f, 0.25f, 0.15f);
    glPushMatrix(); glTranslatef(cx, fY+GH+0.12f, cz); drawBox(W+0.5f, 0.24f, D+0.5f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx, fY+GH-0.08f, cz+D*0.5f+0.26f); drawBox(W+0.5f, 0.20f, 0.10f); glPopMatrix();

    // ── Ground-floor WINDOWS (front: 2 left + 2 right of arch)
    setMat(0.28f, 0.35f, 0.44f);
    float gfWin[4]={cx-W*0.5f+1.20f, cx-W*0.5f+4.60f, cx+W*0.5f-4.60f, cx+W*0.5f-1.20f};
    for(int w=0;w<4;++w){
        glPushMatrix(); glTranslatef(gfWin[w], fY+GH*0.45f, cz+D*0.5f+0.04f);
        drawBox(1.60f, 2.0f, 0.06f); glPopMatrix();
        // window frame
        setMat(0.92f, 0.88f, 0.82f);
        glPushMatrix(); glTranslatef(gfWin[w], fY+GH*0.45f, cz+D*0.5f+0.08f); drawBox(1.72f, 2.12f, 0.06f); glPopMatrix();
        glPushMatrix(); glTranslatef(gfWin[w], fY+GH*0.45f, cz+D*0.5f+0.04f);
        drawBox(1.60f, 2.0f, 0.06f); glPopMatrix();
        setMat(0.28f,0.35f,0.44f);
    }
    // side windows ground (2 each side)
    setMat(0.28f,0.35f,0.44f);
    float sideWz[2]={cz-D*0.30f, cz+D*0.15f};
    for(int sw=0;sw<2;++sw){
        glPushMatrix(); glTranslatef(cx-W*0.5f-0.02f, fY+GH*0.45f, sideWz[sw]); drawBox(0.06f,2.0f,1.60f); glPopMatrix();
        glPushMatrix(); glTranslatef(cx+W*0.5f+0.02f, fY+GH*0.45f, sideWz[sw]); drawBox(0.06f,2.0f,1.60f); glPopMatrix();
    }

    // ── GRAND ENTRANCE ARCH ───────────────────────────────────
    // Arch frame columns
    setMat(0.88f, 0.82f, 0.70f);
    glPushMatrix(); glTranslatef(cx-3.6f, fY+2.20f, cz+D*0.5f); drawBox(0.55f, 4.40f, 0.55f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+3.6f, fY+2.20f, cz+D*0.5f); drawBox(0.55f, 4.40f, 0.55f); glPopMatrix();
    // Arch curve (approximated with wedge segments)
    setMat(0.88f, 0.82f, 0.70f);
    int nSeg=10;
    for(int s=0;s<nSeg;++s){
        float a0=s/(float)nSeg*PI, a1=(s+1)/(float)nSeg*PI;
        float r=3.60f;
        float x0=cx+cosf(PI-a0)*r, x1=cx+cosf(PI-a1)*r;
        float y0=fY+GH-0.70f+sinf(a0)*r*0.45f, y1=fY+GH-0.70f+sinf(a1)*r*0.45f;
        glBegin(GL_QUADS);
        glNormal3f(0,0,1);
        glVertex3f(x0-0.27f,y0,cz+D*0.5f+0.16f); glVertex3f(x1-0.27f,y1,cz+D*0.5f+0.16f);
        glVertex3f(x1+0.27f,y1,cz+D*0.5f+0.16f); glVertex3f(x0+0.27f,y0,cz+D*0.5f+0.16f);
        glEnd();
    }
    // Entry steps
    setMat(0.85f, 0.82f, 0.76f);
    glPushMatrix(); glTranslatef(cx, fY-0.08f, cz+D*0.5f+0.70f); drawBox(6.5f, 0.18f, 1.4f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx, fY+0.12f, cz+D*0.5f+0.25f); drawBox(6.5f, 0.18f, 0.6f); glPopMatrix();
    // Portico canopy
    setMat(0.35f, 0.25f, 0.15f);
    glPushMatrix(); glTranslatef(cx, fY+GH-0.05f, cz+D*0.5f+1.50f); drawBox(8.0f, 0.25f, 3.20f); glPopMatrix();
    // canopy support posts
    setMat(0.88f, 0.82f, 0.70f);
    glPushMatrix(); glTranslatef(cx-3.4f, fY+GH*0.5f-0.5f, cz+D*0.5f+2.8f); drawBox(0.35f, GH-1.0f, 0.35f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+3.4f, fY+GH*0.5f-0.5f, cz+D*0.5f+2.8f); drawBox(0.35f, GH-1.0f, 0.35f); glPopMatrix();

    // ── UPPER FLOOR ───────────────────────────────────────────
    setMat(0.98f, 0.95f, 0.90f);  // slightly lighter cream
    float uBase = fY + GH + 0.24f;
    // back wall
    glPushMatrix(); glTranslatef(cx, uBase+UH*0.5f, cz-D*0.5f); drawBox(W, UH, 0.30f); glPopMatrix();
    // side walls
    glPushMatrix(); glTranslatef(cx-W*0.5f, uBase+UH*0.5f, cz); drawBox(0.30f, UH, D); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+W*0.5f, uBase+UH*0.5f, cz); drawBox(0.30f, UH, D); glPopMatrix();
    // front wall upper
    glPushMatrix(); glTranslatef(cx, uBase+UH*0.5f, cz+D*0.5f); drawBox(W, UH, 0.30f); glPopMatrix();

    // BALCONY slab along front
    setMat(0.88f, 0.84f, 0.76f);
    glPushMatrix(); glTranslatef(cx, uBase+0.15f, cz+D*0.5f+0.80f); drawBox(W+0.5f, 0.22f, 1.80f); glPopMatrix();
    // Balcony railing (slender balusters)
    setMat(0.96f, 0.94f, 0.90f);
    glPushMatrix(); glTranslatef(cx, uBase+0.95f, cz+D*0.5f+1.62f); drawBox(W+0.5f, 0.08f, 0.08f); glPopMatrix();
    for(int bl=0;bl<14;++bl){
        float bx=cx-W*0.5f+0.5f+bl*(W/13.0f);
        glPushMatrix(); glTranslatef(bx, uBase+0.55f, cz+D*0.5f+1.62f); drawBox(0.08f, 0.78f, 0.08f); glPopMatrix();
    }

    // Upper floor windows (5 across front)
    setMat(0.28f, 0.35f, 0.44f);
    for(int w=0;w<5;++w){
        float wx2=cx-W*0.5f+2.0f+w*(W-4.0f)/4.0f;
        glPushMatrix(); glTranslatef(wx2, uBase+UH*0.48f, cz+D*0.5f+0.04f);
        drawBox(2.0f, 2.20f, 0.06f); glPopMatrix();
        // frame
        setMat(0.92f,0.88f,0.82f);
        glPushMatrix(); glTranslatef(wx2, uBase+UH*0.48f, cz+D*0.5f+0.08f); drawBox(2.15f,2.34f,0.05f); glPopMatrix();
        glPushMatrix(); glTranslatef(wx2, uBase+UH*0.48f, cz+D*0.5f+0.04f); drawBox(2.0f,2.20f,0.06f); glPopMatrix();
        setMat(0.28f,0.35f,0.44f);
        // small Juliet balcony bar under each window
        setMat(0.35f,0.25f,0.15f);
        glPushMatrix(); glTranslatef(wx2, uBase+UH*0.48f-1.15f, cz+D*0.5f+0.20f); drawBox(2.20f,0.10f,0.45f); glPopMatrix();
    }

    // top parapet
    setMat(0.35f, 0.25f, 0.15f);
    glPushMatrix(); glTranslatef(cx, uBase+UH+0.20f, cz); drawBox(W+0.5f, 0.40f, D+0.5f); glPopMatrix();
    // parapet crenellations
    setMat(0.92f,0.88f,0.82f);
    for(int pc=0;pc<8;++pc){
        float px2=cx-W*0.5f+1.5f+pc*(W-3.0f)/7.0f;
        glPushMatrix(); glTranslatef(px2, uBase+UH+0.55f, cz+D*0.5f+0.26f); drawBox(1.20f,0.50f,0.22f); glPopMatrix();
        glPushMatrix(); glTranslatef(px2, uBase+UH+0.55f, cz-D*0.5f-0.26f); drawBox(1.20f,0.50f,0.22f); glPopMatrix();
    }

    // ── MAIN FACADE SIGN — "DERA RESORT" ─────────────────────
    setMat(0.06f, 0.06f, 0.07f);
    glPushMatrix(); glTranslatef(cx, uBase+UH*0.80f, cz+D*0.5f+0.38f); drawBox(14.0f, 1.60f, 0.22f); glPopMatrix();
    // gold frame
    setMat(0.78f, 0.64f, 0.12f);
    glPushMatrix(); glTranslatef(cx, uBase+UH*0.80f, cz+D*0.5f+0.50f); drawBox(14.2f, 1.76f, 0.04f); glPopMatrix();
    // Emissive at night
    { GLfloat em[4]={0,0,0,1};
      if(night||sunset){em[0]=0.90f;em[1]=0.76f;em[2]=0.20f;em[3]=1.0f;}
      glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em); }
    glDisable(GL_LIGHTING);
    glColor3f(0.04f, 0.04f, 0.04f);   // black
    drawStrokeText_("DERA  RESORT",
        cx - 5.0f, uBase+UH*0.78f, cz+D*0.5f+0.58f,
        0.0075f, 1.0f);
    glEnable(GL_LIGHTING);
    // small stars / rating
    { GLfloat em2[4]={0,0,0,1};
      if(night||sunset){em2[0]=0.7f;em2[1]=0.6f;em2[2]=0.1f;em2[3]=1.0f;}
      glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em2);}
    glDisable(GL_LIGHTING);
    glColor3f(0.90f,0.75f,0.12f);
    drawStrokeText_("* * * * *",
        cx-2.20f, uBase+UH*0.68f, cz+D*0.5f+0.58f,
        0.0028f, 1.0f);
    glEnable(GL_LIGHTING);
    { GLfloat z0[4]={0,0,0,1}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0); }

    // ── OUTDOOR RECEPTION DESK ────────────────────────────────
    float recX=cx, recZ=cz+D*0.5f+5.50f;
    // Canopy over reception
    setMat(0.08f,0.08f,0.10f);
    glPushMatrix(); glTranslatef(recX-2.8f, fY+3.80f, recZ); drawBox(0.20f,3.60f,0.20f); glPopMatrix();
    glPushMatrix(); glTranslatef(recX+2.8f, fY+3.80f, recZ); drawBox(0.20f,3.60f,0.20f); glPopMatrix();
    setMat(0.20f,0.20f,0.22f);
    glPushMatrix(); glTranslatef(recX, fY+5.65f, recZ); drawBox(6.0f,0.22f,2.80f); glPopMatrix();
    // canopy underside warm strip
    if(night||sunset){GLfloat em[4]={0.55f,0.48f,0.30f,1};glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em);}
    setMat(0.96f,0.92f,0.78f);
    glPushMatrix(); glTranslatef(recX, fY+5.54f, recZ); drawBox(5.80f,0.04f,2.60f); glPopMatrix();
    {GLfloat z0[4]={0,0,0,1};glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0);}
    // Reception counter (dark, marble top)
    setMat(0.10f,0.10f,0.12f);
    glPushMatrix(); glTranslatef(recX-0.5f, fY+0.68f, recZ-0.50f); drawBox(5.20f,1.36f,1.55f); glPopMatrix();
    setMat(0.94f,0.92f,0.90f);
    glPushMatrix(); glTranslatef(recX-0.5f, fY+1.38f, recZ-0.50f); drawBox(5.38f,0.10f,1.70f); glPopMatrix();
    // gold accent strip
    setMat(0.78f,0.64f,0.12f);
    glPushMatrix(); glTranslatef(recX-0.5f, fY+1.30f, recZ+0.26f); drawBox(5.24f,0.07f,0.06f); glPopMatrix();
    // computer / monitor
    setMat(0.10f,0.10f,0.10f);
    glPushMatrix(); glTranslatef(recX-1.8f, fY+1.60f, recZ-0.65f); glRotatef(-20,1,0,0); drawBox(0.55f,0.38f,0.05f); glPopMatrix();
    setMat(0.25f,0.48f,0.82f);
    glPushMatrix(); glTranslatef(recX-1.8f, fY+1.65f, recZ-0.67f); glRotatef(-20,1,0,0); drawBox(0.42f,0.26f,0.02f); glPopMatrix();
    // "RECEPTION" sign on counter face
    {GLfloat em[4]={0,0,0,1};
     if(night||sunset){em[0]=0.6f;em[1]=0.5f;em[2]=0.1f;em[3]=1;}
     glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em);}
    glDisable(GL_LIGHTING);
    glColor3f(0.86f,0.72f,0.12f);
    drawStrokeText_("RECEPTION", recX-2.40f, fY+0.90f, recZ+0.28f, 0.0028f, 1.0f);
    glEnable(GL_LIGHTING);
    {GLfloat z0[4]={0,0,0,1};glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0);}
    // flower vase on counter
    setMat(0.20f,0.55f,0.38f);
    glPushMatrix(); glTranslatef(recX+1.5f, fY+1.55f, recZ-0.50f); glutSolidSphere(0.18f,10,8); glPopMatrix();
    setMat(0.92f,0.30f,0.40f);
    for(int fl=0;fl<5;++fl){float fa=fl*72.0f*PI/180.0f;
        glPushMatrix(); glTranslatef(recX+1.5f+cosf(fa)*0.18f, fY+1.80f, recZ-0.50f+sinf(fa)*0.18f);
        glutSolidSphere(0.06f,8,6); glPopMatrix();}

    // ── WATCHMAN ──────────────────────────────────────────────
    {
        float wkX=recX+3.0f, wkZ=recZ+1.20f;
        float wkY=fY;
        glPushMatrix(); glTranslatef(wkX,wkY,wkZ); glRotatef(200.0f,0,1,0);
        // boots
        setMat(0.15f,0.12f,0.10f);
        glPushMatrix(); glTranslatef(-0.10f,0.14f,0); drawBox(0.18f,0.28f,0.20f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.10f,0.14f,0); drawBox(0.18f,0.28f,0.20f); glPopMatrix();
        // trousers (dark navy)
        setMat(0.10f,0.12f,0.24f);
        glPushMatrix(); glTranslatef(-0.10f,0.54f,0); drawBox(0.18f,0.54f,0.18f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.10f,0.54f,0); drawBox(0.18f,0.54f,0.18f); glPopMatrix();
        // shirt (dark navy with brass buttons)
        glPushMatrix(); glTranslatef(0,1.12f,0); drawBox(0.44f,0.68f,0.28f); glPopMatrix();
        // epaulettes
        setMat(0.78f,0.64f,0.12f);
        glPushMatrix(); glTranslatef(-0.25f,1.48f,0); drawBox(0.24f,0.08f,0.28f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.25f,1.48f,0); drawBox(0.24f,0.08f,0.28f); glPopMatrix();
        // buttons
        setMat(0.78f,0.64f,0.12f);
        for(int btn=0;btn<3;++btn){
            glPushMatrix(); glTranslatef(0,1.38f-btn*0.18f,0.15f); glutSolidSphere(0.04f,6,4); glPopMatrix();
        }
        // arms (attention pose)
        setMat(0.10f,0.12f,0.24f);
        glPushMatrix(); glTranslatef(-0.30f,1.10f,0); glRotatef(8,0,0,1); drawBox(0.14f,0.60f,0.14f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.30f,1.10f,0); glRotatef(-8,0,0,1); drawBox(0.14f,0.60f,0.14f); glPopMatrix();
        // hands
        setMat(0.75f,0.60f,0.48f);
        glPushMatrix(); glTranslatef(-0.30f,0.75f,0); glutSolidSphere(0.07f,8,6); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.30f,0.75f,0); glutSolidSphere(0.07f,8,6); glPopMatrix();
        // neck
        setMat(0.75f,0.60f,0.48f);
        glPushMatrix(); glTranslatef(0,1.52f,0); drawBox(0.15f,0.16f,0.15f); glPopMatrix();
        // head
        glPushMatrix(); glTranslatef(0,1.70f,0); glutSolidSphere(0.18f,12,10); glPopMatrix();
        // cap (navy flat cap)
        setMat(0.10f,0.12f,0.24f);
        glPushMatrix(); glTranslatef(0,1.84f,0); glScalef(1,0.32f,1); glutSolidSphere(0.22f,12,8); glPopMatrix();
        setMat(0.10f,0.12f,0.24f);
        glPushMatrix(); glTranslatef(0.14f,1.80f,0.06f); drawBox(0.28f,0.05f,0.20f); glPopMatrix(); // peak
        // cap badge
        setMat(0.78f,0.64f,0.12f);
        glPushMatrix(); glTranslatef(0,1.85f,0.22f); glutSolidSphere(0.06f,8,6); glPopMatrix();
        glPopMatrix();
    }

    // ── PARKED CARS (4 cars in forecourt bays) ────────────────
    float bayZ = cz+D*0.5f+5.0f;
    // Colours: silver, white, dark-blue, red
    drawParkedCar(cx-W*0.36f, bayZ,  0.0f, 0.75f,0.76f,0.78f);  // silver  (x≈52)
    drawParkedCar(cx-W*0.12f, bayZ,  0.0f, 0.96f,0.96f,0.96f);  // white   (x≈59)
    drawParkedCar(cx+W*0.12f, bayZ, 180.0f, 0.10f,0.15f,0.45f); // dark-blue(x≈65)
    drawParkedCar(cx+W*0.36f, bayZ, 180.0f, 0.72f,0.10f,0.12f); // red     (x≈72)

    // ── Lamp posts flanking entrance & gate ───────────────────
    auto luxLamp=[&](float lx, float lz){
        setMat(0.14f,0.14f,0.16f);
        GLUquadric*q=gluNewQuadric();
        glPushMatrix(); glTranslatef(lx,gY,lz); glRotatef(-90,1,0,0);
        gluCylinder(q,0.08f,0.08f,5.5f,10,2); glPopMatrix();
        setMat(0.12f,0.12f,0.14f);
        glPushMatrix(); glTranslatef(lx,gY+5.5f,lz); drawBox(1.20f,0.10f,0.10f); glPopMatrix();
        if(night||sunset){GLfloat em[4]={1,0.95f,0.75f,1};glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em);}
        setMat(1,0.95f,0.78f);
        glPushMatrix(); glTranslatef(lx+0.60f,gY+5.40f,lz); glutSolidSphere(0.20f,10,8); glPopMatrix();
        {GLfloat z0[4]={0,0,0,1};glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0);}
        gluDeleteQuadric(q);
    };
    luxLamp(cx-W*0.5f-1.0f, cz+D*0.5f+2.0f);
    luxLamp(cx+W*0.5f+1.0f, cz+D*0.5f+2.0f);
    luxLamp(cx-W*0.5f-1.0f, cz+D*0.5f+12.0f);
    luxLamp(cx+W*0.5f+1.0f, cz+D*0.5f+12.0f);
}

// ============================================================
//  BONFIRE — large fire with crowd sitting around
// ============================================================
static void drawBonfire(float cx, float cz)
{
    float gY = -0.50f;
    const float S = 0.28f;         // scale factor — shorter than person height
    GLUquadric* q = gluNewQuadric();

    // ── Sand patch / ash circle ──────────────────────────────
    setMat(0.60f,0.55f,0.45f);
    glBegin(GL_POLYGON); glNormal3f(0,1,0);
    for(int a=0;a<24;++a){ float ang=a*2*PI/24.0f;
        glVertex3f(cx+cosf(ang)*5.0f*S*0.5f, gY+0.004f, cz+sinf(ang)*5.0f*S*0.5f);}
    glEnd();

    // ── Stone ring ───────────────────────────────────────────
    setMat(0.42f,0.38f,0.34f);
    for(int i=0;i<12;++i){
        float ang=i*2*PI/12.0f;
        glPushMatrix(); glTranslatef(cx+cosf(ang)*1.70f*S, gY+0.28f*S, cz+sinf(ang)*1.70f*S);
        glScalef(0.68f*S,0.44f*S,0.68f*S); glutSolidSphere(0.5f,8,6); glPopMatrix();
    }

    // ── 3 crossed logs ───────────────────────────────────────
    setMat(0.28f,0.16f,0.07f);
    for(int li=0;li<3;++li){
        glPushMatrix(); glTranslatef(cx,gY+0.14f*S,cz);
        glRotatef(li*60.0f,0,1,0); glRotatef(6,0,0,1);
        glTranslatef(0,0,-1.50f*S); glRotatef(90,1,0,0);
        gluCylinder(q,0.26f*S,0.22f*S,3.00f*S,10,2); glPopMatrix();
    }
    // charred ember glow at log centre
    if(night||sunset){ GLfloat em[4]={1.0f,0.28f,0.0f,1.0f};
        glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em); }
    setMat(0.85f,0.22f,0.02f);
    glPushMatrix(); glTranslatef(cx,gY+0.24f*S,cz); glScalef(1.0f,0.12f,1.0f);
    glutSolidSphere(1.10f*S,14,6); glPopMatrix();
    { GLfloat z0[4]={0,0,0,1}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0); }

    // ── Animated fire cones ───────────────────────────────────
    float fl1=0.80f+0.20f*sinf(shopPhase*4.2f);
    float fl2=0.85f+0.15f*cosf(shopPhase*3.6f+1.1f);
    float fl3=0.88f+0.12f*sinf(shopPhase*5.0f+0.7f);

    auto flame=[&](float fx,float fz,float h,float r,float g,float b,float rad){
        glDisable(GL_LIGHTING);
        glColor3f(r,g,b);
        glPushMatrix(); glTranslatef(fx,gY+0.36f*S,fz);
        glBegin(GL_TRIANGLE_FAN); glVertex3f(0,h,0);
        for(int a=0;a<=16;++a){ float an=a*2*PI/16.0f;
            glVertex3f(cosf(an)*rad,0,sinf(an)*rad);}
        glEnd(); glPopMatrix();
        glEnable(GL_LIGHTING);
    };
    // outer red-orange
    flame(cx,           cz,           6.80f*fl1, 1.0f,0.28f,0.0f, 2.40f*S);
    flame(cx+0.90f*S,   cz+0.60f*S,  5.60f*fl2, 1.0f,0.38f,0.0f, 1.80f*S);
    flame(cx-1.10f*S,   cz-0.50f*S,  6.20f*fl3, 1.0f,0.32f,0.0f, 1.90f*S);
    // middle orange
    flame(cx,           cz,           5.20f*fl2, 1.0f,0.62f,0.0f, 1.60f*S);
    flame(cx+0.40f*S,   cz-0.30f*S,  4.40f*fl1, 1.0f,0.68f,0.0f, 1.30f*S);
    // inner yellow
    flame(cx,           cz,           3.80f*fl3, 1.0f,0.90f,0.10f,1.10f*S);
    flame(cx-0.20f*S,   cz+0.20f*S,  3.00f*fl2, 1.0f,0.95f,0.25f,0.80f*S);
    // bright white-yellow core tip
    flame(cx,           cz,           7.60f*fl1, 1.0f,0.98f,0.90f,0.36f*S);

    // night glow disc on ground
    if(night||sunset){
        GLfloat em2[4]={1.0f,0.38f,0.0f,1.0f};
        glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em2);
        setMat(1.0f,0.48f,0.04f);
        glPushMatrix(); glTranslatef(cx,gY+0.01f,cz);
        glScalef(6.0f*S,0.02f,6.0f*S); glutSolidSphere(0.5f,18,4); glPopMatrix();
        GLfloat z0[4]={0,0,0,1}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0);
    }

    // ── Smoke puffs (translucent, drifting up) ────────────────
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for(int s=0;s<6;++s){
        float sy=gY+6.4f*S+s*1.8f*S;
        float sw=sinf(shopPhase*1.4f+s*1.6f)*0.8f*S;
        glColor4f(0.50f,0.48f,0.46f, 0.24f-s*0.035f);
        glPushMatrix(); glTranslatef(cx+sw,sy,cz);
        glScalef((0.70f+s*0.28f)*S,0.56f*S,(0.70f+s*0.28f)*S); glutSolidSphere(0.5f,10,6); glPopMatrix();
    }
    glDisable(GL_BLEND); glEnable(GL_LIGHTING);

    // ── 7 people sitting / standing around fire ───────────────
    float pAngDeg[7]={0,52,104,156,208,260,312};
    float pRad=3.80f*S;
    float sr[7]={0.82f,0.22f,0.18f,0.88f,0.70f,0.90f,0.28f};
    float sg[7]={0.18f,0.48f,0.62f,0.55f,0.22f,0.48f,0.72f};
    float sb[7]={0.18f,0.82f,0.28f,0.12f,0.72f,0.22f,0.72f};
    float sk[7]={0.86f,0.58f,0.74f,0.90f,0.50f,0.80f,0.64f};

    for(int p=0;p<7;++p){
        float ang=pAngDeg[p]*PI/180.0f;
        float px=cx+cosf(ang)*pRad, pz=cz+sinf(ang)*pRad;
        float psway=0.0f;
        bool sit=true; // everyone sits around the fire
        float legH=sit?0.52f*S:1.08f*S, bodyY=sit?1.0f*S:1.76f*S;
        float faceAng=atan2f(cx-px,cz-pz)*180.0f/PI;

        glPushMatrix(); glTranslatef(px,gY,pz); glRotatef(faceAng+psway,0,1,0);
        // legs
        setMat(0.18f,0.15f,0.24f);
        glPushMatrix(); glTranslatef(-0.20f*S,legH*0.5f,0); drawBox(0.36f*S,legH,0.36f*S); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.20f*S,legH*0.5f,0); drawBox(0.36f*S,legH,0.36f*S); glPopMatrix();
        // shirt
        setMat(sr[p],sg[p],sb[p]);
        glPushMatrix(); glTranslatef(0,bodyY,0); drawBox(0.76f*S,1.0f*S,0.52f*S); glPopMatrix();
        // arms
        setMat(sk[p]*0.90f,sk[p]*0.72f,sk[p]*0.52f);
        glPushMatrix(); glTranslatef(-0.56f*S,bodyY-0.12f*S,0); glRotatef(sit?-22.0f:12.0f,0,0,1);
        drawBox(0.26f*S,0.88f*S,0.26f*S); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.56f*S,bodyY-0.12f*S,0); glRotatef(sit?22.0f:-12.0f,0,0,1);
        drawBox(0.26f*S,0.88f*S,0.26f*S); glPopMatrix();
        // head
        setMat(sk[p],sk[p]*0.80f,sk[p]*0.58f);
        glPushMatrix(); glTranslatef(0,bodyY+0.76f*S,0); glutSolidSphere(0.36f*S,12,10); glPopMatrix();
        setMat(0.12f,0.08f,0.05f);
        glPushMatrix(); glTranslatef(0,bodyY+1.00f*S,0); glutSolidSphere(0.28f*S,10,8); glPopMatrix();
        // stick / cup props
        if(p%3==0){ // roasting stick
            setMat(0.32f,0.20f,0.08f);
            glPushMatrix(); glTranslatef(0.56f*S,bodyY+0.08f*S,0);
            glRotatef(-28,1,0,0); glRotatef(22,0,0,1);
            drawBox(0.08f*S,0.10f*S,2.60f*S); glPopMatrix();
        }
        if(p%3==1){ // drink cup
            setMat(0.78f,0.28f,0.14f);
            glPushMatrix(); glTranslatef(0.64f*S,bodyY+0.36f*S,0.32f*S);
            drawBox(0.20f*S,0.30f*S,0.20f*S); glPopMatrix();
        }
        glPopMatrix();
    }
    gluDeleteQuadric(q);
}

// ============================================================
//  SEAFOOD PUSH-CART — thelagari with vendor & customers
// ============================================================
static void drawSeafoodCart(float cx, float cz, float yRot)
{
    float gY=-0.50f;
    const float S=1.0f;            // scale factor
    GLUquadric* q=gluNewQuadric();

    glPushMatrix(); glTranslatef(cx,gY,cz); glRotatef(yRot,0,1,0);

    // ── Cart legs (4 wooden posts) ────────────────────────────
    setMat(0.38f,0.24f,0.10f);
    float lx[2]={-0.65f*S,0.65f*S}, lz2[2]={-0.38f*S,0.38f*S};
    for(int i=0;i<2;++i) for(int j=0;j<2;++j){
        glPushMatrix(); glTranslatef(lx[i],0.55f*S,lz2[j]);
        glRotatef(-90,1,0,0); gluCylinder(q,0.07f*S,0.07f*S,1.10f*S,8,1); glPopMatrix();
    }
    // Cart tray top
    setMat(0.52f,0.34f,0.16f);
    glPushMatrix(); glTranslatef(0,1.12f*S,0); drawBox(1.52f*S,0.14f*S,0.82f*S); glPopMatrix();
    // side boards
    setMat(0.42f,0.26f,0.10f);
    glPushMatrix(); glTranslatef(0, 0.90f*S, 0.42f*S); drawBox(1.52f*S,0.38f*S,0.10f*S); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 0.90f*S,-0.42f*S); drawBox(1.52f*S,0.38f*S,0.10f*S); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.77f*S,0.90f*S,0); drawBox(0.10f*S,0.38f*S,0.82f*S); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.77f*S,0.90f*S,0); drawBox(0.10f*S,0.38f*S,0.82f*S); glPopMatrix();

    // ── 2 large wheels ────────────────────────────────────────
    setMat(0.14f,0.12f,0.10f);
    for(int side=-1;side<=1;side+=2){
        glPushMatrix(); glTranslatef(side*0.72f*S,0.30f*S,0);
        glRotatef(side*90,0,1,0); gluCylinder(q,0.30f*S,0.30f*S,0.14f*S,18,2);
        gluDisk(q,0,0.30f*S,18,1); glPopMatrix();
    }
    // spokes
    setMat(0.45f,0.38f,0.28f);
    for(int sp=0;sp<6;++sp){ float sa=sp*60.0f*PI/180.0f;
        float ry=0.30f*S+sinf(sa)*0.22f*S, rz2=cosf(sa)*0.22f*S;
        glPushMatrix(); glTranslatef( 0.72f*S,ry,rz2); drawBox(0.12f*S,0.06f*S,0.06f*S); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.72f*S,ry,rz2); drawBox(0.12f*S,0.06f*S,0.06f*S); glPopMatrix();
    }
    // push handle bar
    setMat(0.30f,0.20f,0.08f);
    glPushMatrix(); glTranslatef(0.82f*S,1.22f*S,0); glRotatef(-90,1,0,0);
    gluCylinder(q,0.06f*S,0.06f*S,0.80f*S,8,1); glPopMatrix();
    glPushMatrix(); glTranslatef(0.82f*S,2.00f*S,0); drawBox(0.10f*S,0.08f*S,1.0f*S); glPopMatrix();

    // ── Striped umbrella ──────────────────────────────────────
    setMat(0.14f,0.14f,0.16f);
    glPushMatrix(); glTranslatef(0,1.20f*S,0); glRotatef(-90,1,0,0);
    gluCylinder(q,0.06f*S,0.06f*S,2.20f*S,8,1); glPopMatrix();
    float uc[6][3]={{0.90f,0.14f,0.14f},{1.0f,0.85f,0.04f},{0.18f,0.62f,0.22f},
                     {0.90f,0.14f,0.14f},{1.0f,0.85f,0.04f},{0.18f,0.62f,0.22f}};
    for(int i=0;i<6;++i){
        float a0=i*60.0f*PI/180.0f, a1=(i+1)*60.0f*PI/180.0f;
        setMat(uc[i][0],uc[i][1],uc[i][2]);
        glBegin(GL_TRIANGLES); glNormal3f(0,1,0);
        glVertex3f(0,3.40f*S*0.5f,0);
        glVertex3f(cosf(a0)*1.80f*S*0.5f,3.00f*S*0.5f,sinf(a0)*1.80f*S*0.5f);
        glVertex3f(cosf(a1)*1.80f*S*0.5f,3.00f*S*0.5f,sinf(a1)*1.80f*S*0.5f);
        glEnd();
    }
    setMat(0.85f,0.10f,0.10f);
    glPushMatrix(); glTranslatef(0,3.44f*S*0.5f,0); glutSolidSphere(0.14f*S,8,6); glPopMatrix();

    // ── Seafood display on tray ───────────────────────────────
    setMat(0.84f,0.92f,0.96f);  // crushed ice
    glPushMatrix(); glTranslatef(0,1.20f*S,0); drawBox(1.30f*S,0.07f*S,0.68f*S); glPopMatrix();
    // 3 fish (silver-blue oval)
    setMat(0.62f,0.70f,0.82f);
    glPushMatrix(); glTranslatef(-0.40f*S,1.30f*S,0.10f*S); glScalef(0.38f*S,0.12f*S,0.18f*S); glutSolidSphere(1,10,6); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.06f*S,1.30f*S,-0.08f*S); glScalef(0.40f*S,0.12f*S,0.16f*S); glutSolidSphere(1,10,6); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.42f*S,1.30f*S, 0.12f*S); glScalef(0.32f*S,0.12f*S,0.16f*S); glutSolidSphere(1,10,6); glPopMatrix();
    // prawns (orange-pink)
    setMat(1.0f,0.50f,0.25f);
    glPushMatrix(); glTranslatef(-0.22f*S,1.30f*S,-0.20f*S); glScalef(0.18f*S,0.10f*S,0.28f*S); glutSolidSphere(1,8,6); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.22f*S,1.30f*S,-0.22f*S); glScalef(0.16f*S,0.10f*S,0.26f*S); glutSolidSphere(1,8,6); glPopMatrix();
    // crab (red flat)
    setMat(0.85f,0.16f,0.08f);
    glPushMatrix(); glTranslatef(-0.52f*S,1.30f*S,-0.16f*S); glScalef(0.22f*S,0.08f*S,0.20f*S); glutSolidSphere(1,10,6); glPopMatrix();
    // price placard
    setMat(0.96f,0.96f,0.92f);
    glPushMatrix(); glTranslatef(0.62f*S,1.40f*S,-0.30f*S); drawBox(0.28f*S,0.36f*S,0.05f*S); glPopMatrix();
    setMat(0.10f,0.10f,0.55f);
    glPushMatrix(); glTranslatef(0.62f*S,1.50f*S,-0.28f*S); drawBox(0.18f*S,0.05f*S,0.02f*S); glPopMatrix();
    glPushMatrix(); glTranslatef(0.62f*S,1.40f*S,-0.28f*S); drawBox(0.14f*S,0.05f*S,0.02f*S); glPopMatrix();

    glPopMatrix(); // end cart transform

    // ── Vendor (behind cart) ──────────────────────────────────
    float vdx=cx-sinf(yRot*PI/180.0f)*2.30f;
    float vdz=cz-cosf(yRot*PI/180.0f)*2.30f;
    glPushMatrix(); glTranslatef(vdx,gY,vdz); glRotatef(yRot,0,1,0);
    // legs
    setMat(0.18f,0.20f,0.30f);
    glPushMatrix(); glTranslatef(-0.20f,0.60f,0); drawBox(0.36f,1.20f,0.36f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.20f,0.60f,0); drawBox(0.36f,1.20f,0.36f); glPopMatrix();
    // shirt (green)
    setMat(0.12f,0.50f,0.26f);
    glPushMatrix(); glTranslatef(0,2.0f,0); drawBox(0.84f,1.10f,0.56f); glPopMatrix();
    // apron
    setMat(0.92f,0.90f,0.88f);
    glPushMatrix(); glTranslatef(0,1.64f,0.30f); drawBox(0.76f,1.36f,0.08f); glPopMatrix();
    // arms
    setMat(0.12f,0.50f,0.26f);
    glPushMatrix(); glTranslatef(-0.60f,1.96f,0); glRotatef(18,0,0,1); glRotatef(-14,1,0,0); drawBox(0.26f,1.0f,0.26f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.60f,1.96f,0); glRotatef(-18,0,0,1); glRotatef(-14,1,0,0); drawBox(0.26f,1.0f,0.26f); glPopMatrix();
    // head
    setMat(0.70f,0.52f,0.36f);
    glPushMatrix(); glTranslatef(0,3.08f,0); glutSolidSphere(0.36f,12,10); glPopMatrix();
    // chef hat
    setMat(0.96f,0.96f,0.96f);
    glPushMatrix(); glTranslatef(0,3.40f,0); glRotatef(-90,1,0,0);
    gluCylinder(q,0.34f,0.32f,0.60f,12,2); glPopMatrix();
    glPushMatrix(); glTranslatef(0,4.00f,0); glScalef(1,0.42f,1); glutSolidSphere(0.44f,12,6); glPopMatrix();
    glPopMatrix();

    // ── 2 customers browsing ──────────────────────────────────
    for(int c=0;c<2;++c){
        float cang=(yRot+(c==0?-42.0f:38.0f))*PI/180.0f;
        float cpx=cx+cosf(cang)*3.30f, cpz=cz+sinf(cang)*3.30f;
        float cr=(c==0)?0.88f:0.24f, cg=(c==0)?0.22f:0.52f, cb=(c==0)?0.52f:0.88f;
        glPushMatrix(); glTranslatef(cpx,gY,cpz); glRotatef(yRot+180.0f,0,1,0);
        setMat(0.20f,0.18f,0.28f);
        glPushMatrix(); glTranslatef(-0.20f,0.60f,0); drawBox(0.36f,1.20f,0.36f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.20f,0.60f,0); drawBox(0.36f,1.20f,0.36f); glPopMatrix();
        setMat(cr,cg,cb);
        glPushMatrix(); glTranslatef(0,1.92f,0); drawBox(0.80f,1.04f,0.56f); glPopMatrix();
        setMat(0.80f,0.62f,0.46f);
        glPushMatrix(); glTranslatef(0,3.04f,0); glutSolidSphere(0.36f,12,10); glPopMatrix();
        setMat(0.12f,0.08f,0.06f);
        glPushMatrix(); glTranslatef(0,3.28f,0); glutSolidSphere(0.28f,10,8); glPopMatrix();
        glPopMatrix();
    }
    gluDeleteQuadric(q);
}

// ============================================================
//  WATER PARK  (right of BeachResort, cx≈218, no overlap)
// ============================================================
static void drawWaterPark(float cx, float cz)
{
    float gY=-0.50f;
    const float TH=20.0f;   // slide tower height
    GLUquadric*q=gluNewQuadric();

    // ── Tiled ground ─────────────────────────────────────────
    setMat(0.70f,0.88f,0.84f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(cx-20.0f,gY+0.01f,cz-16.0f); glVertex3f(cx+20.0f,gY+0.01f,cz-16.0f);
    glVertex3f(cx+20.0f,gY+0.01f,cz+22.0f); glVertex3f(cx-20.0f,gY+0.01f,cz+22.0f);
    glEnd();
    // tile grout
    setMat(0.55f,0.72f,0.70f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    for(int i=0;i<=8;++i){ float xp=cx-20.0f+i*5.0f;
        glVertex3f(xp,gY+0.012f,cz-16.0f); glVertex3f(xp+0.08f,gY+0.012f,cz-16.0f);
        glVertex3f(xp+0.08f,gY+0.012f,cz+22.0f); glVertex3f(xp,gY+0.012f,cz+22.0f);}
    for(int i=0;i<=7;++i){ float zp=cz-16.0f+i*5.5f;
        glVertex3f(cx-20.0f,gY+0.012f,zp); glVertex3f(cx+20.0f,gY+0.012f,zp);
        glVertex3f(cx+20.0f,gY+0.012f,zp+0.08f); glVertex3f(cx-20.0f,gY+0.012f,zp+0.08f);}
    glEnd();

    // ── Entry sign arch ───────────────────────────────────────
    setMat(0.10f,0.35f,0.75f);
    glPushMatrix(); glTranslatef(cx-10.0f,gY+3.0f,cz+21.5f); drawBox(0.60f,6.0f,0.50f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+10.0f,gY+3.0f,cz+21.5f); drawBox(0.60f,6.0f,0.50f); glPopMatrix();
    setMat(0.12f,0.55f,0.88f);
    glPushMatrix(); glTranslatef(cx,gY+6.50f,cz+21.5f); drawBox(21.0f,1.20f,0.55f); glPopMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(1.0f,0.95f,0.08f);
    drawStrokeText_("WATER  PARK", cx-6.0f, gY+6.80f, cz+21.85f, 0.0055f, 1.0f);
    glEnable(GL_LIGHTING);

    // ── Landing pool (blue) ───────────────────────────────────
    setMat(0.08f,0.40f,0.80f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(cx-8.0f,gY+0.15f,cz+6.0f); glVertex3f(cx+8.0f,gY+0.15f,cz+6.0f);
    glVertex3f(cx+8.0f,gY+0.15f,cz+18.0f);glVertex3f(cx-8.0f,gY+0.15f,cz+18.0f);
    glEnd();
    // pool walls
    setMat(0.88f,0.93f,0.96f);
    glPushMatrix(); glTranslatef(cx,gY+0.35f,cz+6.0f);  drawBox(16.0f,0.70f,0.22f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx,gY+0.35f,cz+18.0f); drawBox(16.0f,0.70f,0.22f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx-8.0f,gY+0.35f,cz+12.0f); drawBox(0.22f,0.70f,12.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+8.0f,gY+0.35f,cz+12.0f); drawBox(0.22f,0.70f,12.0f); glPopMatrix();
    // water shimmer
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.30f,0.65f,1.0f,0.35f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(cx-7.8f,gY+0.20f,cz+6.2f); glVertex3f(cx+7.8f,gY+0.20f,cz+6.2f);
    glVertex3f(cx+7.8f,gY+0.20f,cz+17.8f);glVertex3f(cx-7.8f,gY+0.20f,cz+17.8f);
    glEnd();
    glDisable(GL_BLEND); glEnable(GL_LIGHTING);

    // ── Small kids pool ───────────────────────────────────────
    setMat(0.12f,0.55f,0.88f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(cx+9.0f,gY+0.13f,cz+14.0f); glVertex3f(cx+18.5f,gY+0.13f,cz+14.0f);
    glVertex3f(cx+18.5f,gY+0.13f,cz+20.5f);glVertex3f(cx+9.0f,gY+0.13f,cz+20.5f);
    glEnd();
    setMat(0.88f,0.93f,0.96f);
    glPushMatrix(); glTranslatef(cx+13.75f,gY+0.30f,cz+14.0f);  drawBox(9.5f,0.60f,0.18f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+13.75f,gY+0.30f,cz+20.5f);  drawBox(9.5f,0.60f,0.18f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+9.0f,gY+0.30f,cz+17.25f);   drawBox(0.18f,0.60f,6.5f);  glPopMatrix();
    glPushMatrix(); glTranslatef(cx+18.5f,gY+0.30f,cz+17.25f);  drawBox(0.18f,0.60f,6.5f);  glPopMatrix();

    // ── SLIDE TOWER (steel lattice, center) ───────────────────
    float tX=cx+1.0f, tZ=cz-4.0f;
    setMat(0.65f,0.65f,0.68f);
    float legX[4]={-3.5f,3.5f,-3.5f,3.5f}, legZ[4]={-3.5f,-3.5f,3.5f,3.5f};
    for(int i=0;i<4;++i){
        glPushMatrix(); glTranslatef(tX+legX[i],gY+TH*0.5f,tZ+legZ[i]);
        drawBox(0.40f,TH,0.40f); glPopMatrix();
    }
    // horizontal brace rings at 5m intervals
    setMat(0.55f,0.55f,0.58f);
    for(float bh=4.0f;bh<TH;bh+=4.0f){
        glPushMatrix(); glTranslatef(tX,gY+bh,tZ); drawBox(7.3f,0.18f,0.18f); glPopMatrix();
        glPushMatrix(); glTranslatef(tX,gY+bh,tZ); drawBox(0.18f,0.18f,7.3f); glPopMatrix();
    }
    // Top platform
    setMat(0.90f,0.55f,0.12f);
    glPushMatrix(); glTranslatef(tX,gY+TH+0.15f,tZ); drawBox(8.0f,0.30f,8.0f); glPopMatrix();
    // Safety railings
    setMat(0.20f,0.20f,0.22f);
    for(int i=0;i<4;++i){
        float ra=i*90.0f*PI/180.0f;
        float rx=tX+cosf(ra)*3.80f, rz2=tZ+sinf(ra)*3.80f;
        glPushMatrix(); glTranslatef(rx,gY+TH+1.0f,rz2); drawBox(0.14f,2.0f,0.14f); glPopMatrix();
    }
    glPushMatrix(); glTranslatef(tX,gY+TH+2.0f,tZ); drawBox(8.2f,0.14f,0.14f); glPopMatrix();
    glPushMatrix(); glTranslatef(tX,gY+TH+2.0f,tZ); drawBox(0.14f,0.14f,8.2f); glPopMatrix();

    // ── SLIDE A: Yellow wide open drop slide ─────────────────
    for(int s=0;s<12;++s){
        float t=s/11.0f;
        float sx=tX+4.0f+t*9.0f, sy=gY+TH*(1.0f-t*t)+0.60f, sz2=tZ+t*10.0f;
        setMat(1.0f,0.85f,0.08f);
        glPushMatrix(); glTranslatef(sx,sy,sz2); glRotatef(-50.0f*t,0,0,1);
        drawBox(3.0f,0.22f,3.0f); glPopMatrix();
        // slide walls
        setMat(1.0f,0.94f,0.30f);
        glPushMatrix(); glTranslatef(sx-1.60f,sy+0.70f,sz2); glRotatef(-50.0f*t,0,0,1);
        drawBox(0.16f,1.40f,3.0f); glPopMatrix();
        glPushMatrix(); glTranslatef(sx+1.60f,sy+0.70f,sz2); glRotatef(-50.0f*t,0,0,1);
        drawBox(0.16f,1.40f,3.0f); glPopMatrix();
    }

    // ── SLIDE B: Green spiral slide ──────────────────────────
    for(int s=0;s<28;++s){
        float t=s/27.0f;
        float spirAng=t*4.0f*PI+PI*0.5f;
        float sR=5.0f, sy=gY+TH*(1.0f-t)+0.40f;
        float sx=tX+cosf(spirAng)*sR, sz2=tZ+sinf(spirAng)*sR;
        setMat(0.16f,0.70f,0.26f);
        glPushMatrix(); glTranslatef(sx,sy,sz2);
        glRotatef(spirAng*180.0f/PI,0,1,0);
        drawBox(2.80f,0.25f,1.80f);
        // outer wall
        setMat(0.10f,0.55f,0.18f);
        glPushMatrix(); glTranslatef(0,0.80f,0.95f); drawBox(2.80f,1.60f,0.16f); glPopMatrix();
        glPopMatrix();
    }

    // ── SLIDE C: Blue enclosed tube slide ────────────────────
    for(int s=0;s<14;++s){
        float t=s/13.0f;
        float sx=tX-4.0f-t*10.0f, sy=gY+TH*(0.95f-t*0.88f)+0.40f, sz2=tZ+t*5.0f;
        setMat(0.18f,0.45f,0.90f);
        glPushMatrix(); glTranslatef(sx,sy,sz2);
        glScalef(1.0f,0.62f,1.0f); glutSolidSphere(1.80f,12,8); glPopMatrix();
    }

    // ── SLIDE D: Red/orange kiddie slide (short, near kids pool) ─
    for(int s=0;s<8;++s){
        float t=s/7.0f;
        float sx=cx+13.0f, sy=gY+6.0f*(1.0f-t)+0.40f, sz2=cz+6.0f+t*8.0f;
        setMat(0.90f,0.28f,0.12f);
        glPushMatrix(); glTranslatef(sx,sy,sz2); glRotatef(-40.0f*t,0,0,1);
        drawBox(2.0f,0.20f,2.0f); glPopMatrix();
    }
    // kiddie slide mini tower
    setMat(0.65f,0.65f,0.68f);
    glPushMatrix(); glTranslatef(cx+13.0f,gY+3.5f,cz+4.5f); drawBox(0.28f,7.0f,0.28f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+15.0f,gY+3.5f,cz+4.5f); drawBox(0.28f,7.0f,0.28f); glPopMatrix();
    setMat(0.90f,0.55f,0.12f);
    glPushMatrix(); glTranslatef(cx+14.0f,gY+7.30f,cz+4.5f); drawBox(4.5f,0.25f,3.0f); glPopMatrix();

    // ── Ladder staircase to main tower ───────────────────────
    setMat(0.58f,0.58f,0.62f);
    for(int st=0;st<14;++st){
        float sh=gY+st*(TH/14.0f)+0.20f;
        glPushMatrix(); glTranslatef(tX-5.5f,sh,tZ+3.0f); drawBox(2.0f,0.14f,0.55f); glPopMatrix();
    }
    glPushMatrix(); glTranslatef(tX-6.4f,gY+TH*0.5f,tZ+3.0f); drawBox(0.14f,TH,0.14f); glPopMatrix();
    glPushMatrix(); glTranslatef(tX-4.6f,gY+TH*0.5f,tZ+3.0f); drawBox(0.14f,TH,0.14f); glPopMatrix();

    // ── Deck chairs around pool ───────────────────────────────
    float chairXs[6]={cx-12.0f,cx-12.0f,cx-12.0f,cx+10.0f,cx+10.0f,cx+10.0f};
    float chairZs[6]={cz+8.0f,cz+12.0f,cz+16.0f,cz+8.0f,cz+12.0f,cz+16.0f};
    for(int dc=0;dc<6;++dc){
        setMat(0.92f,0.78f,0.42f);
        glPushMatrix(); glTranslatef(chairXs[dc],gY+0.30f,chairZs[dc]); drawBox(2.50f,0.16f,0.90f); glPopMatrix();
        glPushMatrix(); glTranslatef(chairXs[dc],gY+0.65f,chairZs[dc]-0.55f); glRotatef(-35,1,0,0);
        drawBox(2.50f,1.20f,0.14f); glPopMatrix();
        // legs
        setMat(0.65f,0.52f,0.28f);
        glPushMatrix(); glTranslatef(chairXs[dc]-1.0f,gY+0.15f,chairZs[dc]); drawBox(0.14f,0.30f,0.90f); glPopMatrix();
        glPushMatrix(); glTranslatef(chairXs[dc]+1.0f,gY+0.15f,chairZs[dc]); drawBox(0.14f,0.30f,0.90f); glPopMatrix();
    }

    // ── People (6 in queue, 4 at pool, 3 sliding) ────────────
    float psr[10]={0.82f,0.18f,0.90f,0.12f,0.75f,0.88f,0.28f,0.55f,0.90f,0.20f};
    float psg[10]={0.18f,0.60f,0.50f,0.55f,0.18f,0.78f,0.72f,0.22f,0.28f,0.70f};
    float psb[10]={0.82f,0.88f,0.10f,0.85f,0.75f,0.12f,0.20f,0.88f,0.82f,0.80f};
    // queue at staircase
    for(int p=0;p<6;++p){
        float px=tX-5.5f, pz2=tZ+5.5f+p*2.8f;
        glPushMatrix(); glTranslatef(px,gY,pz2); glRotatef(0.0f,0,1,0);
        setMat(0.18f,0.16f,0.26f);
        glPushMatrix(); glTranslatef(-0.20f,0.60f,0); drawBox(0.36f,1.20f,0.36f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.20f,0.60f,0); drawBox(0.36f,1.20f,0.36f); glPopMatrix();
        setMat(psr[p],psg[p],psb[p]);
        glPushMatrix(); glTranslatef(0,1.92f,0); drawBox(0.76f,1.04f,0.52f); glPopMatrix();
        setMat(0.82f,0.64f,0.48f);
        glPushMatrix(); glTranslatef(0,3.04f,0); glutSolidSphere(0.36f,10,8); glPopMatrix();
        setMat(0.12f,0.08f,0.06f);
        glPushMatrix(); glTranslatef(0,3.28f,0); glutSolidSphere(0.28f,8,6); glPopMatrix();
        glPopMatrix();
    }
    // pool loungers (4 people lying)
    for(int p=0;p<4;++p){
        float px=chairXs[p], pz2=chairZs[p]+0.0f;
        glPushMatrix(); glTranslatef(px,gY+0.50f,pz2); glRotatef(90.0f,0,1,0);
        setMat(psr[p+4],psg[p+4],psb[p+4]);
        glPushMatrix(); glTranslatef(0,0.20f,0); drawBox(1.80f,0.28f,0.44f); glPopMatrix();
        setMat(0.82f,0.64f,0.48f);
        glPushMatrix(); glTranslatef(0.95f,0.20f,0); glutSolidSphere(0.24f,10,8); glPopMatrix();
        glPopMatrix();
    }
    // 3 animated sliders on slide A
    for(int s=0;s<3;++s){
        float t=fmodf(shopPhase*0.10f+s*0.33f,1.0f);
        float sx=tX+4.0f+t*9.0f, sy=gY+TH*(1.0f-t*t)+1.60f, sz2=tZ+t*10.0f;
        glPushMatrix(); glTranslatef(sx,sy,sz2); glRotatef(-50.0f*t,0,0,1);
        setMat(psr[s],psg[s],psb[s]);
        glPushMatrix(); glTranslatef(0,0.55f,0); drawBox(0.60f,0.80f,0.50f); glPopMatrix();
        setMat(0.82f,0.65f,0.48f);
        glPushMatrix(); glTranslatef(0,1.30f,0); glutSolidSphere(0.30f,10,8); glPopMatrix();
        glPopMatrix();
    }
    gluDeleteQuadric(q);
}

// ============================================================
//  ANIMATED TRAIN  (runs across x-axis at z=-130)
//  Big, colorful, smoke from engine
// ============================================================
static void drawTrain(float tx, float tz)
{
    float gY=-0.50f;
    float rH=1.20f;    // rail height above ground
    GLUquadric*q=gluNewQuadric();

    // ── Railway track (2 rails + sleepers) ───────────────────
    setMat(0.30f,0.22f,0.12f);
    for(float sx=tx-5.0f;sx<tx+80.0f;sx+=3.5f){
        glPushMatrix(); glTranslatef(sx,gY+rH-0.25f,tz); drawBox(3.0f,0.20f,5.50f); glPopMatrix();
    }
    setMat(0.45f,0.42f,0.40f);
    glPushMatrix(); glTranslatef(tx+35.0f,gY+rH,tz-2.0f); drawBox(90.0f,0.18f,0.30f); glPopMatrix();
    glPushMatrix(); glTranslatef(tx+35.0f,gY+rH,tz+2.0f); drawBox(90.0f,0.18f,0.30f); glPopMatrix();

    // ── ENGINE (locomotive) ───────────────────────────────────
    float ex=tx;
    // boiler body
    setMat(0.78f,0.12f,0.10f);   // deep red
    glPushMatrix(); glTranslatef(ex,gY+rH+3.50f,tz); drawBox(11.0f,5.50f,5.50f); glPopMatrix();
    // rounded boiler nose (front)
    setMat(0.68f,0.10f,0.08f);
    glPushMatrix(); glTranslatef(ex+5.55f,gY+rH+3.0f,tz);
    glScalef(0.60f,1.0f,1.0f); glutSolidSphere(2.75f,14,10); glPopMatrix();
    // cab (darker, at rear)
    setMat(0.58f,0.08f,0.06f);
    glPushMatrix(); glTranslatef(ex-3.0f,gY+rH+5.0f,tz); drawBox(5.0f,3.20f,5.50f); glPopMatrix();
    // cab windows
    setMat(0.65f,0.82f,0.90f);
    glPushMatrix(); glTranslatef(ex-3.0f,gY+rH+5.80f,tz+2.80f); drawBox(3.20f,1.60f,0.10f); glPopMatrix();
    glPushMatrix(); glTranslatef(ex-3.0f,gY+rH+5.80f,tz-2.80f); drawBox(3.20f,1.60f,0.10f); glPopMatrix();
    // gold trim bands
    setMat(0.85f,0.68f,0.10f);
    glPushMatrix(); glTranslatef(ex,gY+rH+1.20f,tz); drawBox(11.2f,0.28f,5.60f); glPopMatrix();
    glPushMatrix(); glTranslatef(ex,gY+rH+6.10f,tz); drawBox(11.2f,0.20f,5.60f); glPopMatrix();
    // front headlight
    setMat(1.0f,0.95f,0.70f);
    if(night||sunset){ GLfloat em[4]={1,0.95f,0.7f,1}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em); }
    glPushMatrix(); glTranslatef(ex+5.80f,gY+rH+4.20f,tz); glutSolidSphere(0.65f,12,8); glPopMatrix();
    { GLfloat z0[4]={0,0,0,1}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0); }
    // cowcatcher (front V-shape)
    setMat(0.22f,0.22f,0.24f);
    glPushMatrix(); glTranslatef(ex+6.20f,gY+rH+0.70f,tz); drawBox(0.50f,1.0f,5.50f); glPopMatrix();
    glPushMatrix(); glTranslatef(ex+6.0f,gY+rH+0.30f,tz-1.50f); glRotatef(30,0,0,1); drawBox(0.40f,0.50f,2.80f); glPopMatrix();
    glPushMatrix(); glTranslatef(ex+6.0f,gY+rH+0.30f,tz+1.50f); glRotatef(30,0,0,1); drawBox(0.40f,0.50f,2.80f); glPopMatrix();
    // smokestack
    setMat(0.18f,0.18f,0.20f);
    glPushMatrix(); glTranslatef(ex+3.5f,gY+rH+6.20f,tz); glRotatef(-90,1,0,0);
    gluCylinder(q,0.55f,0.65f,2.80f,12,2); glPopMatrix();
    // stack flare
    glPushMatrix(); glTranslatef(ex+3.5f,gY+rH+9.20f,tz); glScalef(1,0.38f,1);
    glutSolidSphere(1.0f,12,6); glPopMatrix();
    // engine wheels (3 pairs, large)
    setMat(0.14f,0.14f,0.16f);
    float wXpos[3]={ex+4.0f,ex+1.0f,ex-2.5f};
    for(int w=0;w<3;++w){
        for(int side=-1;side<=1;side+=2){
            glPushMatrix(); glTranslatef(wXpos[w],gY+rH,tz+side*3.0f);
            glRotatef(90,0,1,0); gluCylinder(q,1.0f,1.0f,0.28f,18,2);
            gluDisk(q,0,1.0f,18,1); glPopMatrix();
            // spokes
            setMat(0.40f,0.35f,0.28f);
            for(int sp=0;sp<8;++sp){ float sa=sp*45.0f*PI/180.0f;
                glPushMatrix(); glTranslatef(wXpos[w],gY+rH+sinf(sa)*0.70f,tz+side*3.0f+cosf(sa)*0.70f);
                drawBox(0.28f,0.12f,0.12f); glPopMatrix();
            }
            setMat(0.14f,0.14f,0.16f);
        }
    }
    // small front bogey wheels
    setMat(0.22f,0.22f,0.24f);
    glPushMatrix(); glTranslatef(ex+5.50f,gY+rH-0.20f,tz-2.0f);
    glRotatef(90,0,1,0); gluCylinder(q,0.48f,0.48f,0.20f,12,2); gluDisk(q,0,0.48f,12,1); glPopMatrix();
    glPushMatrix(); glTranslatef(ex+5.50f,gY+rH-0.20f,tz+2.0f);
    glRotatef(90,0,1,0); gluCylinder(q,0.48f,0.48f,0.20f,12,2); gluDisk(q,0,0.48f,12,1); glPopMatrix();

    // ── ANIMATED SMOKE PUFFS from stack ──────────────────────
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for(int s=0;s<7;++s){
        float st2=fmodf(shopPhase*0.22f+s*0.14f,1.0f);
        float smokeY=gY+rH+9.20f+st2*8.0f;
        float smokeSway=(st2*st2)*2.0f;
        float smokeAlpha=0.55f*(1.0f-st2);
        float smokeR=0.70f+st2*0.22f, smokeG=0.68f+st2*0.22f, smokeB=0.66f+st2*0.22f;
        glColor4f(smokeR,smokeG,smokeB,smokeAlpha);
        glPushMatrix(); glTranslatef(ex+3.5f+smokeSway,smokeY,tz);
        glScalef((0.45f+st2*0.55f),(0.40f+st2*0.40f),(0.45f+st2*0.55f));
        glutSolidSphere(1.80f,10,6); glPopMatrix();
    }
    glDisable(GL_BLEND); glEnable(GL_LIGHTING);

    // ── PASSENGER CARS (5 colorful cars behind engine) ────────
    float carCols[5][3]={{0.14f,0.38f,0.80f},{0.14f,0.65f,0.22f},
                          {0.88f,0.72f,0.08f},{0.75f,0.28f,0.72f},{0.90f,0.42f,0.12f}};
    float carLen=11.0f, coupling=1.5f;
    for(int c=0;c<5;++c){
        float cx2=ex-(carLen+coupling)*(c+1);
        float cr=carCols[c][0], cg=carCols[c][1], cb=carCols[c][2];
        // car body
        setMat(cr,cg,cb);
        glPushMatrix(); glTranslatef(cx2,gY+rH+3.20f,tz); drawBox(carLen,5.20f,5.20f); glPopMatrix();
        // roof (slightly darker)
        setMat(cr*0.80f,cg*0.80f,cb*0.80f);
        glPushMatrix(); glTranslatef(cx2,gY+rH+5.90f,tz); drawBox(carLen,0.45f,5.30f); glPopMatrix();
        // windows (2 rows, both sides)
        setMat(0.65f,0.85f,0.92f);
        for(int w=0;w<4;++w){
            float wox=cx2-carLen*0.38f+w*(carLen*0.25f);
            glPushMatrix(); glTranslatef(wox,gY+rH+3.80f,tz+2.65f); drawBox(1.60f,1.20f,0.10f); glPopMatrix();
            glPushMatrix(); glTranslatef(wox,gY+rH+3.80f,tz-2.65f); drawBox(1.60f,1.20f,0.10f); glPopMatrix();
        }
        // white trim stripe
        setMat(0.96f,0.96f,0.96f);
        glPushMatrix(); glTranslatef(cx2,gY+rH+1.20f,tz); drawBox(carLen,0.22f,5.28f); glPopMatrix();
        // car number (simple dark rect)
        setMat(0.15f,0.15f,0.15f);
        glPushMatrix(); glTranslatef(cx2,gY+rH+4.80f,tz+2.67f); drawBox(1.20f,0.80f,0.08f); glPopMatrix();
        // bogey wheels (2 axles per car)
        setMat(0.14f,0.14f,0.16f);
        float bwXs[2]={cx2-carLen*0.30f, cx2+carLen*0.30f};
        for(int bw=0;bw<2;++bw){
            for(int side=-1;side<=1;side+=2){
                glPushMatrix(); glTranslatef(bwXs[bw],gY+rH,tz+side*2.80f);
                glRotatef(90,0,1,0); gluCylinder(q,0.70f,0.70f,0.22f,14,2);
                gluDisk(q,0,0.70f,14,1); glPopMatrix();
            }
        }
        // coupling rod between cars
        setMat(0.28f,0.28f,0.30f);
        if(c<4) glPushMatrix(); glTranslatef(cx2-carLen*0.5f-coupling*0.5f,gY+rH+1.0f,tz);
        if(c<4) drawBox(coupling,0.25f,0.25f); if(c<4) glPopMatrix();
    }
    // engine-to-car coupling
    setMat(0.28f,0.28f,0.30f);
    glPushMatrix(); glTranslatef(ex-carLen*0.5f-coupling*0.25f,gY+rH+1.0f,tz);
    drawBox(coupling,0.25f,0.25f); glPopMatrix();

    gluDeleteQuadric(q);
}

// ============================================================
//  BEACH VIBES BOUTIQUE — Luxury showroom (Zara/Gucci style)
//  Full glass front, tall H=10, visible products from outside
// ============================================================
static void drawBeachShop(float cx, float cz)
{
    float gY=-0.50f, W=24.0f, D=17.0f, H=10.0f, platH=0.30f;
    float fY=gY+platH;

    // ── paved forecourt (concrete / stone tile) ─────────────────
    setMat(0.82f,0.80f,0.78f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(cx-W*1.1f,gY+0.005f,cz-D*0.9f);
    glVertex3f(cx+W*1.1f,gY+0.005f,cz-D*0.9f);
    glVertex3f(cx+W*1.1f,gY+0.005f,cz+D*1.1f);
    glVertex3f(cx-W*1.1f,gY+0.005f,cz+D*1.1f);
    glEnd();
    // tile grout lines
    setMat(0.60f,0.58f,0.56f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    for(int i=0;i<=10;++i){float xp=cx-W*1.1f+i*(W*2.2f/10.0f);
        glVertex3f(xp,gY+0.01f,cz-D*0.9f); glVertex3f(xp+0.06f,gY+0.01f,cz-D*0.9f);
        glVertex3f(xp+0.06f,gY+0.01f,cz+D*1.1f); glVertex3f(xp,gY+0.01f,cz+D*1.1f);}
    for(int i=0;i<=8;++i){float zp=cz-D*0.9f+i*(D*2.0f/8.0f);
        glVertex3f(cx-W*1.1f,gY+0.01f,zp); glVertex3f(cx+W*1.1f,gY+0.01f,zp);
        glVertex3f(cx+W*1.1f,gY+0.01f,zp+0.06f); glVertex3f(cx-W*1.1f,gY+0.01f,zp+0.06f);}
    glEnd();

    // ══════════════════════════════════════════════════════════
    //  STRUCTURE
    // ══════════════════════════════════════════════════════════

    // Raised platform (polished concrete look)
    setMat(0.92f,0.91f,0.90f);
    glPushMatrix(); glTranslatef(cx,gY+platH*0.5f,cz); drawBox(W,platH,D); glPopMatrix();

    // Marble floor pattern (white + light grey veins)
    setMat(0.96f,0.96f,0.96f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(cx-W*0.5f,fY+0.002f,cz-D*0.5f); glVertex3f(cx+W*0.5f,fY+0.002f,cz-D*0.5f);
    glVertex3f(cx+W*0.5f,fY+0.002f,cz+D*0.5f); glVertex3f(cx-W*0.5f,fY+0.002f,cz+D*0.5f);
    glEnd();
    // thin grout lines
    setMat(0.72f,0.72f,0.72f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    for(int i=0;i<=8;++i){float xp=cx-W*0.5f+i*(W/8.0f);
        glVertex3f(xp,fY+0.004f,cz-D*0.5f); glVertex3f(xp+0.04f,fY+0.004f,cz-D*0.5f);
        glVertex3f(xp+0.04f,fY+0.004f,cz+D*0.5f); glVertex3f(xp,fY+0.004f,cz+D*0.5f);}
    for(int i=0;i<=6;++i){float zp=cz-D*0.5f+i*(D/6.0f);
        glVertex3f(cx-W*0.5f,fY+0.004f,zp); glVertex3f(cx+W*0.5f,fY+0.004f,zp);
        glVertex3f(cx+W*0.5f,fY+0.004f,zp+0.04f); glVertex3f(cx-W*0.5f,fY+0.004f,zp+0.04f);}
    glEnd();

    // ── Side walls & back wall (crisp white) ─────────────────
    setMat(0.97f,0.97f,0.97f);
    // Back wall
    glPushMatrix(); glTranslatef(cx,fY+H*0.5f,cz-D*0.5f); drawBox(W,H,0.28f); glPopMatrix();
    // Left wall
    glPushMatrix(); glTranslatef(cx-W*0.5f,fY+H*0.5f,cz-D*0.25f); drawBox(0.28f,H,D*0.5f); glPopMatrix();
    // Right wall
    glPushMatrix(); glTranslatef(cx+W*0.5f,fY+H*0.5f,cz-D*0.25f); drawBox(0.28f,H,D*0.5f); glPopMatrix();
    // No solid flanks — full glass front corner-to-corner

    // ── Flat roof slab + thin fascia ─────────────────────────
    setMat(0.20f,0.20f,0.20f);   // dark charcoal top
    glPushMatrix(); glTranslatef(cx,fY+H+0.20f,cz); drawBox(W+0.8f,0.40f,D+0.8f); glPopMatrix();
    // Slim fascia band — front edge only (avoids floating lines above roof)
    setMat(0.15f,0.15f,0.15f);
    glPushMatrix(); glTranslatef(cx, fY+H+0.02f, cz+D*0.5f+0.08f); drawBox(W+0.8f,0.35f,0.08f); glPopMatrix();

    // ── Structural columns — 4 slim steel only (no mullions, no transom) ──
    setMat(0.12f,0.12f,0.14f);
    // 4 columns: 2 edge + 2 inner (defining 3 glass bays)
    float colXs[4]={cx-W*0.5f, cx-W*0.167f, cx+W*0.167f, cx+W*0.5f};
    for(int c=0;c<4;++c){
        glPushMatrix(); glTranslatef(colXs[c],fY+H*0.5f,cz+D*0.5f); drawBox(0.13f,H,0.13f); glPopMatrix();
        glPushMatrix(); glTranslatef(colXs[c],fY+H*0.5f,cz-D*0.5f+0.15f); drawBox(0.13f,H,0.13f); glPopMatrix();
    }
    // Side columns (mid-depth, for structural integrity)
    glPushMatrix(); glTranslatef(cx-W*0.5f,fY+H*0.5f,cz); drawBox(0.13f,H,0.13f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+W*0.5f,fY+H*0.5f,cz); drawBox(0.13f,H,0.13f); glPopMatrix();
    // No transom bars, no mullions — clean minimal glass facade

    // ── Full-height glass front (3 bays × full H) ───────────
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    // 3 bays: left / center-entrance / right — more transparent so products visible
    float bayAlpha[3]={0.20f,0.22f,0.20f};
    for(int b=0;b<3;++b){
        float x0=colXs[b]+0.07f, x1=colXs[b+1]-0.07f;
        bool isCenter=(b==1);   // center bay = entrance opening
        float yBot = isCenter ? fY+3.60f : fY;
        glColor4f(0.68f,0.88f,1.0f,bayAlpha[b]);
        glBegin(GL_QUADS); glNormal3f(0,0,1);
        glVertex3f(x0,yBot,    cz+D*0.5f+0.05f);
        glVertex3f(x1,yBot,    cz+D*0.5f+0.05f);
        glVertex3f(x1,fY+H-0.2f,cz+D*0.5f+0.05f);
        glVertex3f(x0,fY+H-0.2f,cz+D*0.5f+0.05f);
        glEnd();
    }
    // Side glass panels (half-depth)
    glColor4f(0.70f,0.88f,1.0f,0.25f);
    glBegin(GL_QUADS); glNormal3f(-1,0,0);
    glVertex3f(cx-W*0.5f-0.05f,fY,    cz+D*0.5f-0.22f);
    glVertex3f(cx-W*0.5f-0.05f,fY,    cz+0.0f);
    glVertex3f(cx-W*0.5f-0.05f,fY+H-0.2f,cz+0.0f);
    glVertex3f(cx-W*0.5f-0.05f,fY+H-0.2f,cz+D*0.5f-0.22f);
    glEnd();
    glBegin(GL_QUADS); glNormal3f(1,0,0);
    glVertex3f(cx+W*0.5f+0.05f,fY,    cz+D*0.5f-0.22f);
    glVertex3f(cx+W*0.5f+0.05f,fY,    cz+0.0f);
    glVertex3f(cx+W*0.5f+0.05f,fY+H-0.2f,cz+0.0f);
    glVertex3f(cx+W*0.5f+0.05f,fY+H-0.2f,cz+D*0.5f-0.22f);
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    // ── Entrance door header + step ──────────────────────────
    setMat(0.12f,0.12f,0.14f);
    glPushMatrix(); glTranslatef(cx,fY+3.60f,cz+D*0.5f); drawBox(W*0.334f,0.16f,0.16f); glPopMatrix();
    // Entry step
    setMat(0.85f,0.84f,0.82f);
    glPushMatrix(); glTranslatef(cx,fY-0.08f,cz+D*0.5f+0.55f); drawBox(6.0f,0.16f,1.2f); glPopMatrix();

    // ── Flat canopy overhang above entrance ─────────────────
    setMat(0.12f,0.12f,0.14f);
    glPushMatrix(); glTranslatef(cx,fY+3.80f,cz+D*0.5f+1.20f); drawBox(W*0.37f,0.18f,2.60f); glPopMatrix();
    // canopy underside (light strip - emissive at night)
    if(night||sunset){ GLfloat em[4]={0.6f,0.6f,0.55f,1.0f}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em); }
    setMat(0.95f,0.95f,0.90f);
    glPushMatrix(); glTranslatef(cx,fY+3.72f,cz+D*0.5f+1.20f); drawBox(W*0.33f,0.04f,2.40f); glPopMatrix();
    { GLfloat z0[4]={0,0,0,1}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0); }

    // ══════════════════════════════════════════════════════════
    //  SIGNAGE (Luxury style — slim black panel, gold letters)
    // ══════════════════════════════════════════════════════════
    // ── Luxury sign — wide dark panel, hairline gold frame ───
    setMat(0.06f,0.06f,0.07f);
    glPushMatrix(); glTranslatef(cx,fY+H-1.25f,cz+D*0.5f+0.36f); drawBox(16.0f,2.20f,0.20f); glPopMatrix();
    // hairline gold frame (4 thin bars)
    setMat(0.82f,0.68f,0.14f);
    glPushMatrix(); glTranslatef(cx,fY+H-0.18f,cz+D*0.5f+0.47f); drawBox(16.1f,0.06f,0.05f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx,fY+H-2.34f,cz+D*0.5f+0.47f); drawBox(16.1f,0.06f,0.05f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx-8.05f,fY+H-1.25f,cz+D*0.5f+0.47f); drawBox(0.06f,2.18f,0.05f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+8.05f,fY+H-1.25f,cz+D*0.5f+0.47f); drawBox(0.06f,2.18f,0.05f); glPopMatrix();
    // BEACH VIBES — ultra-thin stroke (lineWidth=1.0), large scale
    { GLfloat em[4]={0,0,0,1};
      if(night||sunset){em[0]=0.95f;em[1]=0.82f;em[2]=0.22f;em[3]=1.0f;}
      glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em); }
    glDisable(GL_LIGHTING);
    glColor3f(0.95f,0.82f,0.18f);
    drawStrokeText_("BEACH  VIBES",
        cx - 5.60f, fY+H-1.15f, cz+D*0.5f+0.52f,
        0.0072f, 1.0f);                       // scale=0.0072 → clean thin
    glEnable(GL_LIGHTING);
    { GLfloat z0[4]={0,0,0,1}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0); }
    // BOUTIQUE subtitle (off-white, very thin)
    glDisable(GL_LIGHTING);
    glColor3f(0.90f,0.88f,0.84f);
    drawStrokeText_("B O U T I Q U E",
        cx - 3.60f, fY+H-2.10f, cz+D*0.5f+0.52f,
        0.0040f, 1.0f);
    glEnable(GL_LIGHTING);

    // Small "est. 2024" plaque under sign
    setMat(0.08f,0.08f,0.09f);
    glPushMatrix(); glTranslatef(cx,fY+H-2.55f,cz+D*0.5f+0.38f); drawBox(3.5f,0.40f,0.12f); glPopMatrix();

    // ══════════════════════════════════════════════════════════
    //  BACK WALL LOGO / BRAND WALL
    // ══════════════════════════════════════════════════════════
    setMat(0.08f,0.08f,0.09f);
    glPushMatrix(); glTranslatef(cx,fY+H*0.55f,cz-D*0.5f+0.35f); drawBox(8.0f,4.0f,0.12f); glPopMatrix();
    { GLfloat em[4]={0,0,0,1};
      if(night||sunset){em[0]=0.65f;em[1]=0.50f;em[2]=0.10f;em[3]=1.0f;}
      glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em); }
    draw3DText("BVB", cx-1.8f, fY+H*0.50f, cz-D*0.5f+0.50f, 0.90f, 0.88f,0.70f,0.14f);
    { GLfloat z0[4]={0,0,0,1}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0); }

    // Light strip along back wall base
    if(night||sunset){ GLfloat em[4]={0.4f,0.35f,0.25f,1.0f}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em); }
    setMat(0.92f,0.85f,0.60f);
    glPushMatrix(); glTranslatef(cx,fY+0.06f,cz-D*0.5f+0.40f); drawBox(W-0.6f,0.06f,0.08f); glPopMatrix();
    { GLfloat z0[4]={0,0,0,1}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0); }

    // ══════════════════════════════════════════════════════════
    //  WINDOW DISPLAY — 3 MANNEQUINS NEAR FRONT GLASS
    //  Positioned close to front so visible from outside
    // ══════════════════════════════════════════════════════════
    // Raised display podiums (white)
    float podZ = cz+D*0.5f-1.0f;   // very close to front glass for max visibility
    float podXs[3]={cx-8.0f, cx, cx+8.0f};  // aligned to left / center / right glass bays
    float podH=0.35f;

    // helper: draw a full mannequin on a podium
    auto drawMannequin=[&](float mx,float mz,float r,float g,float b,
                           float r2,float g2,float b2, bool hasDress, bool hasSunhat)
    {
        // podium
        setMat(0.92f,0.92f,0.92f);
        glPushMatrix(); glTranslatef(mx,fY+podH*0.5f,mz); drawBox(1.30f,podH,1.30f); glPopMatrix();
        // Spotlight circle on podium
        setMat(0.98f,0.96f,0.88f);
        glBegin(GL_POLYGON); glNormal3f(0,1,0);
        for(int a=0;a<16;++a){float ang=a*2*PI/16;
            glVertex3f(mx+cosf(ang)*0.55f,fY+podH+0.002f,mz+sinf(ang)*0.55f);}
        glEnd();

        float base=fY+podH;
        // legs (skin)
        setMat(0.94f,0.80f,0.68f);
        glPushMatrix(); glTranslatef(mx-0.13f,base+0.50f,mz); drawBox(0.20f,1.0f,0.20f); glPopMatrix();
        glPushMatrix(); glTranslatef(mx+0.13f,base+0.50f,mz); drawBox(0.20f,1.0f,0.20f); glPopMatrix();

        if(!hasDress){
            // bottoms (bikini/shorts)
            setMat(r,g,b);
            glPushMatrix(); glTranslatef(mx,base+1.05f,mz); drawBox(0.48f,0.40f,0.28f); glPopMatrix();
        } else {
            // flowy dress — narrower top, wider hem
            setMat(r,g,b);
            glBegin(GL_QUADS);
            glNormal3f(0,0,1);
            glVertex3f(mx-0.62f,base+1.0f,mz+0.18f); glVertex3f(mx+0.62f,base+1.0f,mz+0.18f);
            glVertex3f(mx+0.38f,base+2.35f,mz+0.18f); glVertex3f(mx-0.38f,base+2.35f,mz+0.18f);
            glNormal3f(0,0,-1);
            glVertex3f(mx-0.62f,base+1.0f,mz-0.18f); glVertex3f(mx+0.62f,base+1.0f,mz-0.18f);
            glVertex3f(mx+0.38f,base+2.35f,mz-0.18f); glVertex3f(mx-0.38f,base+2.35f,mz-0.18f);
            glNormal3f(-1,0,0);
            glVertex3f(mx-0.62f,base+1.0f,mz+0.18f); glVertex3f(mx-0.62f,base+1.0f,mz-0.18f);
            glVertex3f(mx-0.38f,base+2.35f,mz-0.18f); glVertex3f(mx-0.38f,base+2.35f,mz+0.18f);
            glNormal3f(1,0,0);
            glVertex3f(mx+0.62f,base+1.0f,mz+0.18f); glVertex3f(mx+0.62f,base+1.0f,mz-0.18f);
            glVertex3f(mx+0.38f,base+2.35f,mz-0.18f); glVertex3f(mx+0.38f,base+2.35f,mz+0.18f);
            glEnd();
            // waistband
            setMat(r*0.75f,g*0.75f,b*0.75f);
            glPushMatrix(); glTranslatef(mx,base+2.35f,mz); glScalef(0.78f,0.10f,0.38f); glutSolidSphere(1.0f,10,6); glPopMatrix();
        }
        // midriff / torso (skin)
        setMat(0.94f,0.80f,0.68f);
        glPushMatrix(); glTranslatef(mx,base+1.60f,mz); drawBox(0.36f,hasDress?0.0f:0.28f,0.24f); glPopMatrix();
        // top / bikini
        setMat(r2,g2,b2);
        glPushMatrix(); glTranslatef(mx,base+2.00f,mz+0.15f); drawBox(0.46f,0.32f,0.08f); glPopMatrix();
        // torso upper
        setMat(0.94f,0.80f,0.68f);
        glPushMatrix(); glTranslatef(mx,base+2.20f,mz); drawBox(0.38f,0.28f,0.26f); glPopMatrix();
        // arms (elegant pose — one slightly out)
        glPushMatrix(); glTranslatef(mx-0.32f,base+2.12f,mz); drawBox(0.14f,0.60f,0.14f); glPopMatrix();
        glPushMatrix(); glTranslatef(mx+0.32f,base+2.12f,mz);
        glRotatef(-20.0f,0,0,1); drawBox(0.14f,0.60f,0.14f); glPopMatrix();
        // neck
        glPushMatrix(); glTranslatef(mx,base+2.56f,mz); drawBox(0.14f,0.18f,0.14f); glPopMatrix();
        // head
        setMat(0.94f,0.80f,0.68f);
        glPushMatrix(); glTranslatef(mx,base+2.82f,mz); glutSolidSphere(0.20f,14,12); glPopMatrix();
        // hair
        setMat(0.15f,0.10f,0.06f);
        glPushMatrix(); glTranslatef(mx,base+2.96f,mz); glutSolidSphere(0.16f,12,10); glPopMatrix();
        if(!hasSunhat){
            // long hair strands
            glPushMatrix(); glTranslatef(mx-0.10f,base+2.64f,mz-0.08f); drawBox(0.07f,0.38f,0.07f); glPopMatrix();
            glPushMatrix(); glTranslatef(mx+0.10f,base+2.64f,mz-0.08f); drawBox(0.07f,0.38f,0.07f); glPopMatrix();
        } else {
            // wide brim sun hat
            setMat(0.94f,0.87f,0.55f);
            glPushMatrix(); glTranslatef(mx,base+3.02f,mz); glScalef(1.0f,0.22f,1.0f); glutSolidSphere(0.45f,16,10); glPopMatrix();
            glPushMatrix(); glTranslatef(mx,base+2.92f,mz); glScalef(1.0f,0.22f,1.0f); glutSolidSphere(0.24f,12,8); glPopMatrix();
            setMat(r,g,b);
            glPushMatrix(); glTranslatef(mx,base+3.00f,mz); glScalef(1.0f,0.07f,1.0f); glutSolidSphere(0.26f,12,6); glPopMatrix();
        }
        // price card on podium
        setMat(0.97f,0.97f,0.95f);
        glPushMatrix(); glTranslatef(mx+0.58f,fY+podH+0.22f,mz-0.30f); drawBox(0.20f,0.35f,0.03f); glPopMatrix();
        setMat(0.12f,0.12f,0.12f);
        glPushMatrix(); glTranslatef(mx+0.58f,fY+podH+0.22f,mz-0.28f); drawBox(0.14f,0.02f,0.01f); glPopMatrix();
        glPushMatrix(); glTranslatef(mx+0.58f,fY+podH+0.16f,mz-0.28f); drawBox(0.10f,0.02f,0.01f); glPopMatrix();
    };

    // Mannequin 1 — Hot-pink bikini
    drawMannequin(podXs[0], podZ, 1.0f,0.15f,0.52f, 1.0f,0.15f,0.52f, false, false);
    // Mannequin 2 — Turquoise maxi dress
    drawMannequin(podXs[1], podZ, 0.14f,0.80f,0.86f, 0.14f,0.65f,0.80f, true,  false);
    // Mannequin 3 — Coral set + sun hat
    drawMannequin(podXs[2], podZ, 1.0f,0.42f,0.22f, 1.0f,0.42f,0.22f, false, true);

    // Slim spotlight rods above each mannequin (ceiling to podium)
    setMat(0.12f,0.12f,0.14f);
    for(int i=0;i<3;++i){
        GLUquadric*q=gluNewQuadric();
        glPushMatrix(); glTranslatef(podXs[i],fY+H-0.22f,podZ); glRotatef(-90,1,0,0);
        gluCylinder(q,0.04f,0.04f,H-0.22f,8,1); glPopMatrix(); gluDeleteQuadric(q);
        // spotlight head
        setMat(0.20f,0.20f,0.22f);
        glPushMatrix(); glTranslatef(podXs[i],fY+1.10f,podZ); glutSolidSphere(0.12f,10,8); glPopMatrix();
        // light cone (emissive warm)
        if(night||sunset){GLfloat em[4]={0.55f,0.48f,0.30f,1.0f};glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em);}
        setMat(0.98f,0.94f,0.78f);
        glPushMatrix(); glTranslatef(podXs[i],fY+0.55f,podZ);
        glScalef(0.55f,0.55f,0.55f); glutSolidSphere(0.25f,10,8); glPopMatrix();
        {GLfloat z0[4]={0,0,0,1};glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0);}
    }

    // ══════════════════════════════════════════════════════════
    //  INTERIOR DISPLAYS
    // ══════════════════════════════════════════════════════════

    // ── Central service counter (marble top, dark base) ───────
    float deskX=cx, deskZ=cz+1.5f;
    setMat(0.12f,0.12f,0.14f);
    glPushMatrix(); glTranslatef(deskX,fY+0.70f,deskZ); drawBox(5.80f,1.40f,1.60f); glPopMatrix();
    // marble counter top
    setMat(0.94f,0.94f,0.92f);
    glPushMatrix(); glTranslatef(deskX,fY+1.42f,deskZ); drawBox(6.0f,0.10f,1.78f); glPopMatrix();
    // counter top grey vein
    setMat(0.68f,0.68f,0.68f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(deskX-2.8f,fY+1.43f,deskZ-0.60f); glVertex3f(deskX+0.5f,fY+1.43f,deskZ+0.20f);
    glVertex3f(deskX+0.5f,fY+1.43f,deskZ+0.23f); glVertex3f(deskX-2.8f,fY+1.43f,deskZ-0.57f);
    glEnd();
    // gold accent strip on desk front
    setMat(0.75f,0.62f,0.12f);
    glPushMatrix(); glTranslatef(deskX,fY+1.35f,deskZ+0.81f); drawBox(5.85f,0.08f,0.06f); glPopMatrix();
    // POS screen (slim, angled)
    setMat(0.10f,0.10f,0.10f);
    glPushMatrix(); glTranslatef(deskX+1.5f,fY+1.60f,deskZ-0.50f); glRotatef(-15,1,0,0); drawBox(0.50f,0.35f,0.04f); glPopMatrix();
    setMat(0.25f,0.52f,0.85f);
    glPushMatrix(); glTranslatef(deskX+1.5f,fY+1.65f,deskZ-0.52f); glRotatef(-15,1,0,0); drawBox(0.38f,0.24f,0.02f); glPopMatrix();
    // Bag / item on counter
    setMat(0.80f,0.65f,0.12f);
    glPushMatrix(); glTranslatef(deskX-1.0f,fY+1.50f,deskZ+0.10f); drawBox(0.32f,0.38f,0.22f); glPopMatrix();
    setMat(0.65f,0.52f,0.10f);
    glPushMatrix(); glTranslatef(deskX-1.0f,fY+1.70f,deskZ+0.10f); drawBox(0.22f,0.06f,0.06f); glPopMatrix();

    // ── Girls wall display (left side, on back-left wall) ────
    {
        float wdX=cx-W*0.38f, wdZ=cz-D*0.46f;
        // Section label — slim gold bar
        setMat(0.75f,0.62f,0.12f);
        glPushMatrix(); glTranslatef(wdX,fY+H*0.70f,wdZ+0.32f); drawBox(6.5f,0.08f,0.10f); glPopMatrix();
        // Label text
        { GLfloat em[4]={0,0,0,1};
          if(night||sunset){em[0]=0.6f;em[1]=0.5f;em[2]=0.1f;em[3]=1.0f;}
          glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em); }
        draw3DText("WOMEN", wdX-1.8f, fY+H*0.68f, wdZ+0.40f, 0.30f, 0.82f,0.68f,0.12f);
        { GLfloat z0[4]={0,0,0,1}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0); }

        // Slim wall shelves — 4 levels of accessories/folded items
        float shelfColors[4][3]={{1.0f,0.18f,0.55f},{0.18f,0.80f,0.88f},{1.0f,0.88f,0.18f},{0.68f,0.32f,0.88f}};
        for(int sl=0;sl<4;++sl){
            float sy=fY+1.2f+sl*1.2f;
            // shelf board (white lacquer)
            setMat(0.96f,0.96f,0.96f);
            glPushMatrix(); glTranslatef(wdX,sy,wdZ+0.20f); drawBox(6.0f,0.06f,0.80f); glPopMatrix();
            // Support brackets (gold)
            setMat(0.72f,0.60f,0.12f);
            for(int br=0;br<3;++br){float bx=wdX-2.0f+br*2.0f;
                glPushMatrix(); glTranslatef(bx,sy-0.20f,wdZ+0.08f); drawBox(0.06f,0.40f,0.06f); glPopMatrix();}
            // Folded items on shelf
            setMat(shelfColors[sl][0],shelfColors[sl][1],shelfColors[sl][2]);
            for(int item=0;item<4;++item){
                float ix=wdX-2.2f+item*1.5f;
                glPushMatrix(); glTranslatef(ix,sy+0.12f,wdZ+0.10f); drawBox(1.0f,0.18f,0.50f); glPopMatrix();
                // fold line
                setMat(shelfColors[sl][0]*0.75f,shelfColors[sl][1]*0.75f,shelfColors[sl][2]*0.75f);
                glPushMatrix(); glTranslatef(ix,sy+0.22f,wdZ+0.10f); drawBox(1.02f,0.02f,0.52f); glPopMatrix();
            }
        }

        // Girls shoe shelf (lower left, near side wall)
        float ssX=cx-W*0.38f+1.0f, ssZ=cz+D*0.25f;
        setMat(0.12f,0.12f,0.14f);  // black shelf unit
        glPushMatrix(); glTranslatef(ssX,fY+1.20f,ssZ); drawBox(6.5f,2.40f,1.20f); glPopMatrix();
        setMat(0.96f,0.96f,0.96f);
        for(int sh=0;sh<3;++sh){
            float sy=fY+0.55f+sh*0.75f;
            glPushMatrix(); glTranslatef(ssX,sy,ssZ); drawBox(6.3f,0.06f,1.10f); glPopMatrix();}
        float gsc[9][3]={{1.0f,0.18f,0.52f},{0.18f,0.78f,0.90f},{1.0f,0.88f,0.18f},
                          {0.86f,0.22f,0.22f},{0.68f,0.32f,0.90f},{0.96f,0.96f,0.96f},
                          {1.0f,0.60f,0.28f},{0.22f,0.55f,0.35f},{0.88f,0.70f,0.90f}};
        for(int r=0;r<3;++r){ float sy=fY+0.60f+r*0.75f;
            for(int p=0;p<3;++p){ int ci=r*3+p; float sx=ssX-2.2f+p*2.2f;
                setMat(gsc[ci][0],gsc[ci][1],gsc[ci][2]);
                glPushMatrix(); glTranslatef(sx-0.20f,sy,ssZ); drawBox(0.30f,0.14f,0.58f); glPopMatrix();
                glPushMatrix(); glTranslatef(sx+0.20f,sy,ssZ); drawBox(0.30f,0.14f,0.58f); glPopMatrix();
                // heel
                if(ci%3!=2){setMat(gsc[ci][0]*0.65f,gsc[ci][1]*0.65f,gsc[ci][2]*0.65f);
                    glPushMatrix(); glTranslatef(sx-0.20f,sy-0.09f,ssZ-0.22f); drawBox(0.06f,0.18f,0.06f); glPopMatrix();
                    glPushMatrix(); glTranslatef(sx+0.20f,sy-0.09f,ssZ-0.22f); drawBox(0.06f,0.18f,0.06f); glPopMatrix();}
            }}
        // shelf label
        { GLfloat em[4]={0,0,0,1};
          if(night||sunset){em[0]=0.6f;em[1]=0.5f;em[2]=0.1f;em[3]=1.0f;}
          glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em);}
        draw3DText("SHOES", ssX-1.5f, fY+2.90f, ssZ+0.62f, 0.26f, 0.82f,0.68f,0.12f);
        {GLfloat z0[4]={0,0,0,1};glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0);}
    }

    // ── Men / Boys wall display (right side) ─────────────────
    {
        float wdX=cx+W*0.20f, wdZ=cz-D*0.46f;
        setMat(0.75f,0.62f,0.12f);
        glPushMatrix(); glTranslatef(wdX,fY+H*0.70f,wdZ+0.32f); drawBox(5.5f,0.08f,0.10f); glPopMatrix();
        { GLfloat em[4]={0,0,0,1};
          if(night||sunset){em[0]=0.6f;em[1]=0.5f;em[2]=0.1f;em[3]=1.0f;}
          glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em); }
        draw3DText("MEN", wdX-1.0f, fY+H*0.68f, wdZ+0.40f, 0.30f, 0.82f,0.68f,0.12f);
        {GLfloat z0[4]={0,0,0,1};glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0);}

        // Hanging rack for shorts (black metal)
        float rkX=cx+W*0.25f, rkZ=cz-D*0.35f;
        setMat(0.12f,0.12f,0.14f);
        glPushMatrix(); glTranslatef(rkX-2.2f,fY+2.0f,rkZ); drawBox(0.10f,4.0f,0.10f); glPopMatrix();
        glPushMatrix(); glTranslatef(rkX+2.2f,fY+2.0f,rkZ); drawBox(0.10f,4.0f,0.10f); glPopMatrix();
        glPushMatrix(); glTranslatef(rkX,fY+4.06f,rkZ);       drawBox(4.50f,0.10f,0.10f); glPopMatrix();
        // Base feet
        glPushMatrix(); glTranslatef(rkX-2.2f,fY+0.05f,rkZ); drawBox(0.80f,0.10f,0.40f); glPopMatrix();
        glPushMatrix(); glTranslatef(rkX+2.2f,fY+0.05f,rkZ); drawBox(0.80f,0.10f,0.40f); glPopMatrix();

        float bsc[7][3]={{0.10f,0.25f,0.75f},{0.0f,0.50f,0.36f},{0.88f,0.30f,0.0f},
                          {0.50f,0.0f,0.55f},{0.82f,0.10f,0.15f},{0.10f,0.10f,0.12f},{0.95f,0.95f,0.95f}};
        for(int bs=0;bs<7;++bs){
            float bsx=rkX-2.6f+bs*0.75f;
            setMat(bsc[bs][0],bsc[bs][1],bsc[bs][2]);
            glPushMatrix(); glTranslatef(bsx,fY+3.50f,rkZ+0.07f); drawBox(0.52f,0.75f,0.07f); glPopMatrix();
            setMat(fminf(bsc[bs][0]+0.20f,1),fminf(bsc[bs][1]+0.20f,1),fminf(bsc[bs][2]+0.20f,1));
            glPushMatrix(); glTranslatef(bsx-0.13f,fY+3.14f,rkZ+0.08f); drawBox(0.20f,0.28f,0.05f); glPopMatrix();
            glPushMatrix(); glTranslatef(bsx+0.13f,fY+3.14f,rkZ+0.08f); drawBox(0.20f,0.28f,0.05f); glPopMatrix();
            // waistband
            setMat(fminf(bsc[bs][0]*1.2f,1),fminf(bsc[bs][1]*1.2f,1),fminf(bsc[bs][2]*1.2f,1));
            glPushMatrix(); glTranslatef(bsx,fY+3.88f,rkZ+0.075f); drawBox(0.52f,0.12f,0.075f); glPopMatrix();
            // metal hanger
            setMat(0.65f,0.65f,0.65f);
            glPushMatrix(); glTranslatef(bsx,fY+3.96f,rkZ); drawBox(0.09f,0.24f,0.09f); glPopMatrix();
            glPushMatrix(); glTranslatef(bsx,fY+4.04f,rkZ); drawBox(0.48f,0.05f,0.05f); glPopMatrix();
        }

        // Men shoe shelf (right side)
        float msX=cx+W*0.28f, msZ=cz+D*0.25f;
        setMat(0.12f,0.12f,0.14f);
        glPushMatrix(); glTranslatef(msX,fY+1.20f,msZ); drawBox(5.5f,2.40f,1.20f); glPopMatrix();
        setMat(0.96f,0.96f,0.96f);
        for(int sh=0;sh<3;++sh){
            float sy=fY+0.55f+sh*0.75f;
            glPushMatrix(); glTranslatef(msX,sy,msZ); drawBox(5.3f,0.06f,1.10f); glPopMatrix();}
        float boySC[6][3]={{0.10f,0.28f,0.75f},{0.88f,0.46f,0.12f},{0.10f,0.10f,0.12f},
                            {0.94f,0.94f,0.94f},{0.30f,0.68f,0.30f},{0.70f,0.56f,0.38f}};
        for(int r=0;r<3;++r){ float sy=fY+0.60f+r*0.75f;
            for(int p=0;p<2;++p){ int ci=r*2+p; float sx=msX-1.6f+p*2.0f;
                setMat(boySC[ci][0],boySC[ci][1],boySC[ci][2]);
                glPushMatrix(); glTranslatef(sx-0.24f,sy,msZ); drawBox(0.35f,0.10f,0.70f); glPopMatrix();
                glPushMatrix(); glTranslatef(sx+0.24f,sy,msZ); drawBox(0.35f,0.10f,0.70f); glPopMatrix();
                setMat(boySC[ci][0]*0.72f,boySC[ci][1]*0.72f,boySC[ci][2]*0.72f);
                glPushMatrix(); glTranslatef(sx-0.24f,sy+0.06f,msZ); drawBox(0.26f,0.07f,0.18f); glPopMatrix();
                glPushMatrix(); glTranslatef(sx+0.24f,sy+0.06f,msZ); drawBox(0.26f,0.07f,0.18f); glPopMatrix();
            }}
        {GLfloat em[4]={0,0,0,1};
          if(night||sunset){em[0]=0.6f;em[1]=0.5f;em[2]=0.1f;em[3]=1.0f;}
          glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em);}
        draw3DText("SHOES", msX-1.4f, fY+2.90f, msZ+0.62f, 0.26f, 0.82f,0.68f,0.12f);
        {GLfloat z0[4]={0,0,0,1};glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0);}
    }

    // ══════════════════════════════════════════════════════════
    //  ANIMATED CHARACTERS
    // ══════════════════════════════════════════════════════════

    // helper: draw a simple character body
    auto drawChar=[&](float cx2,float cy,float cz2,float yRot,
                      float legR,float legG,float legB,
                      float topR,float topG,float topB,
                      float skinR,float skinG,float skinB,
                      float hairR,float hairG,float hairB)
    {
        glPushMatrix(); glTranslatef(cx2,cy,cz2); glRotatef(yRot,0,1,0);
        setMat(legR,legG,legB);
        glPushMatrix(); glTranslatef(-0.10f,0.45f,0); drawBox(0.16f,0.90f,0.16f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.10f,0.45f,0); drawBox(0.16f,0.90f,0.16f); glPopMatrix();
        setMat(topR,topG,topB);
        glPushMatrix(); glTranslatef(0,1.10f,0); drawBox(0.40f,0.65f,0.26f); glPopMatrix();
        setMat(skinR,skinG,skinB);
        glPushMatrix(); glTranslatef(-0.28f,1.05f,0); drawBox(0.13f,0.60f,0.13f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.28f,1.05f,0); drawBox(0.13f,0.60f,0.13f); glPopMatrix();
        glPushMatrix(); glTranslatef(0,1.62f,0); glutSolidSphere(0.17f,12,10); glPopMatrix();
        setMat(hairR,hairG,hairB);
        glPushMatrix(); glTranslatef(0,1.75f,0); glutSolidSphere(0.13f,10,8); glPopMatrix();
        glPopMatrix();
    };

    // SHOPKEEPER — black shirt, turns to assist
    {
        drawChar(deskX-0.5f, fY+1.40f, deskZ-1.0f,
                 180.0f,
                 0.12f,0.12f,0.14f,  // dark trousers
                 0.08f,0.08f,0.10f,  // black shirt
                 0.82f,0.66f,0.50f,  // skin
                 0.20f,0.14f,0.08f); // hair
    }

    // CUSTOMER 1 — girl with raised arm, browsing mannequins
    {
        float armA=38.0f;
        glPushMatrix(); glTranslatef(cx-5.5f,fY,podZ+2.2f); glRotatef(165.0f,0,1,0);
        setMat(0.90f,0.64f,0.32f); // khaki shorts
        glPushMatrix(); glTranslatef(-0.09f,0.44f,0); drawBox(0.14f,0.90f,0.14f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.09f,0.44f,0); drawBox(0.14f,0.90f,0.14f); glPopMatrix();
        setMat(1.0f,0.36f,0.58f); // pink top
        glPushMatrix(); glTranslatef(0,1.06f,0); drawBox(0.36f,0.60f,0.22f); glPopMatrix();
        setMat(0.90f,0.74f,0.60f);
        glPushMatrix(); glTranslatef(0.26f,1.10f,0); glRotatef(-armA,1,0,0); drawBox(0.13f,0.55f,0.13f); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.26f,1.02f,0); drawBox(0.13f,0.55f,0.13f); glPopMatrix();
        glPushMatrix(); glTranslatef(0,1.60f,0); glutSolidSphere(0.16f,12,10); glPopMatrix();
        setMat(0.82f,0.58f,0.18f);
        glPushMatrix(); glTranslatef(0,1.72f,0); glutSolidSphere(0.13f,10,8); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.10f,1.46f,-0.06f); drawBox(0.06f,0.30f,0.06f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.10f,1.46f,-0.06f); drawBox(0.06f,0.30f,0.06f); glPopMatrix();
        glPopMatrix();
    }

    // CUSTOMER 2 — girl with luxury shopping bag, slow turn
    {
        glPushMatrix(); glTranslatef(cx-1.5f,fY,cz+D*0.08f); glRotatef(195.0f,0,1,0);
        setMat(0.25f,0.25f,0.30f); // dark trousers
        glPushMatrix(); glTranslatef(-0.10f,0.44f,0); drawBox(0.16f,0.90f,0.16f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.10f,0.44f,0); drawBox(0.16f,0.90f,0.16f); glPopMatrix();
        setMat(0.96f,0.96f,0.96f); // white blouse
        glPushMatrix(); glTranslatef(0,1.08f,0); drawBox(0.38f,0.62f,0.24f); glPopMatrix();
        setMat(0.90f,0.74f,0.60f);
        glPushMatrix(); glTranslatef(-0.28f,1.00f,0); drawBox(0.13f,0.60f,0.13f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.28f,1.00f,0); drawBox(0.13f,0.60f,0.13f); glPopMatrix();
        // luxury paper bag (gold)
        setMat(0.80f,0.65f,0.12f);
        glPushMatrix(); glTranslatef(0.28f,0.68f,0.14f); drawBox(0.30f,0.38f,0.18f); glPopMatrix();
        setMat(0.10f,0.10f,0.10f);
        glPushMatrix(); glTranslatef(0.28f,0.88f,0.14f); drawBox(0.18f,0.08f,0.04f); glPopMatrix();
        setMat(0.90f,0.74f,0.60f);
        glPushMatrix(); glTranslatef(0,1.62f,0); glutSolidSphere(0.17f,12,10); glPopMatrix();
        setMat(0.15f,0.10f,0.08f);
        glPushMatrix(); glTranslatef(0,1.76f,0); glutSolidSphere(0.13f,10,8); glPopMatrix();
        glPopMatrix();
    }

    // CUSTOMER 3 — boy leaning to look at shoes
    {
        glPushMatrix(); glTranslatef(cx+W*0.25f,fY,cz+D*0.22f); glRotatef(355.0f,0,1,0);
        setMat(0.15f,0.18f,0.22f); // dark jeans
        glPushMatrix(); glTranslatef(-0.12f,0.48f,0); drawBox(0.18f,0.98f,0.18f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.12f,0.48f,0); drawBox(0.18f,0.98f,0.18f); glPopMatrix();
        setMat(0.30f,0.55f,0.88f); // blue shirt
        glPushMatrix(); glTranslatef(0,1.16f,0); drawBox(0.44f,0.68f,0.28f); glPopMatrix();
        setMat(0.72f,0.55f,0.40f);
        glPushMatrix(); glTranslatef(-0.32f,1.14f,0); glRotatef(20,1,0,0); drawBox(0.15f,0.66f,0.15f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.32f,1.14f,0); glRotatef(20,1,0,0); drawBox(0.15f,0.66f,0.15f); glPopMatrix();
        glPushMatrix(); glTranslatef(0,1.70f,0); glutSolidSphere(0.18f,12,10); glPopMatrix();
        setMat(0.18f,0.12f,0.08f);
        glPushMatrix(); glTranslatef(0,1.82f,0); glutSolidSphere(0.14f,10,8); glPopMatrix();
        glPopMatrix();
    }

    // ══════════════════════════════════════════════════════════
    //  AMBIENT LED CEILING STRIP LIGHTS
    // ══════════════════════════════════════════════════════════
    if(night||sunset){
        GLfloat em[4]={0.80f,0.75f,0.60f,1.0f};
        glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,em);
    }
    setMat(0.98f,0.95f,0.85f);
    // Main track along centre
    glPushMatrix(); glTranslatef(cx,fY+H-0.10f,cz); drawBox(W-0.8f,0.05f,0.25f); glPopMatrix();
    // Side tracks
    glPushMatrix(); glTranslatef(cx,fY+H-0.10f,cz-D*0.30f); drawBox(W-0.8f,0.05f,0.12f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx,fY+H-0.10f,cz+D*0.20f); drawBox(W-0.8f,0.05f,0.12f); glPopMatrix();
    { GLfloat z0[4]={0,0,0,1}; glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z0); }

    // ── Two tall potted plants flanking entrance (inside) ────
    auto luxPot=[&](float px,float pz){
        // ceramic pot (dark charcoal)
        setMat(0.12f,0.12f,0.14f);
        glPushMatrix(); glTranslatef(px,fY+0.50f,pz); glScalef(1,1.0f,1); glutSolidSphere(0.35f,12,8); glPopMatrix();
        glPushMatrix(); glTranslatef(px,fY+0.92f,pz); drawBox(0.58f,0.08f,0.58f); glPopMatrix(); // rim
        // soil
        setMat(0.28f,0.20f,0.14f);
        glPushMatrix(); glTranslatef(px,fY+0.98f,pz); glScalef(0.55f,0.18f,0.55f); glutSolidSphere(1,10,6); glPopMatrix();
        // tall stem
        setMat(0.22f,0.48f,0.22f);
        glPushMatrix(); glTranslatef(px,fY+1.80f,pz); drawBox(0.08f,1.65f,0.08f); glPopMatrix();
        // leaves
        for(int lf=0;lf<8;++lf){
            float la=lf*45.0f*PI/180.0f;
            float ly=fY+1.20f+lf*0.20f;
            glPushMatrix(); glTranslatef(px+cosf(la)*0.45f,ly,pz+sinf(la)*0.45f);
            glRotatef(lf*45.0f,0,1,0); glRotatef(-40.0f,0,0,1);
            drawBox(0.90f,0.06f,0.22f); glPopMatrix();
        }
    };
    luxPot(cx-2.8f, cz+D*0.5f-0.80f);
    luxPot(cx+2.8f, cz+D*0.5f-0.80f);
}

static void drawRestaurant(float cx, float cz)
{
    float floorY = -0.5f;
    float platH  =  0.5f;
    float postH  =  3.2f;       // smaller
    float W      = 24.0f;       // wider
    float D      = 17.0f;       // longer
    float roofH  =  5.0f;       // peaked roof

    glPushMatrix();
    glTranslatef(cx, floorY, cz);

    // ---- grass patch around restaurant ----
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texGrass);
    glColor3f(1, 1, 1);
    setMat(1, 1, 1);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0); glVertex3f(-W * 0.9f, 0.005f, -D * 0.9f);
    glTexCoord2f(6, 0); glVertex3f( W * 0.9f, 0.005f, -D * 0.9f);
    glTexCoord2f(6, 6); glVertex3f( W * 0.9f, 0.005f,  D * 0.9f);
    glTexCoord2f(0, 6); glVertex3f(-W * 0.9f, 0.005f,  D * 0.9f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);   // unbind any leftover texture

    // ---- bamboo platform / floor (rich warm brown) ----
    setMat(0.60f, 0.40f, 0.20f);
    glPushMatrix();
    glTranslatef(0, platH * 0.5f, 0);
    drawBox(W, platH, D);
    glPopMatrix();

    // bamboo plank stripes on top of platform (lighter golden)
    setMat(0.85f, 0.65f, 0.30f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    for (int i = 0; i < 14; ++i) {
        float z0 = -D * 0.5f + i * (D / 14.0f);
        glVertex3f(-W * 0.5f, platH + 0.01f, z0);
        glVertex3f( W * 0.5f, platH + 0.01f, z0);
        glVertex3f( W * 0.5f, platH + 0.01f, z0 + 0.05f);
        glVertex3f(-W * 0.5f, platH + 0.01f, z0 + 0.05f);
    }
    glEnd();

    // ---- BAMBOO POSTS (cylindrical, with nodes) ----
    float postPos[6][2] = {
        { -W * 0.45f, -D * 0.45f }, {  W * 0.45f, -D * 0.45f },
        { -W * 0.45f,  D * 0.45f }, {  W * 0.45f,  D * 0.45f },
        { -W * 0.45f,  0.0f      }, {  W * 0.45f,  0.0f      }
    };
    for (int i = 0; i < 6; ++i) {
        drawBambooPole(postPos[i][0], platH, postPos[i][1], postH, 0.18f);
    }

    // ---- BAMBOO horizontal rails (golden bamboo color) ----
    setMat(0.92f, 0.88f, 0.62f);
    glPushMatrix(); glTranslatef(0, platH + 0.4f, D * 0.45f); drawBox(W * 0.9f, 0.10f, 0.13f); glPopMatrix();
    glPushMatrix(); glTranslatef(0, platH + 1.1f, D * 0.45f); drawBox(W * 0.9f, 0.10f, 0.13f); glPopMatrix();
    glPushMatrix(); glTranslatef(0, platH + 0.4f,-D * 0.45f); drawBox(W * 0.9f, 0.10f, 0.13f); glPopMatrix();
    glPushMatrix(); glTranslatef(0, platH + 1.1f,-D * 0.45f); drawBox(W * 0.9f, 0.10f, 0.13f); glPopMatrix();
    glPushMatrix(); glTranslatef(-W * 0.45f, platH + 0.4f, 0); drawBox(0.13f, 0.10f, D * 0.9f); glPopMatrix();
    glPushMatrix(); glTranslatef(-W * 0.45f, platH + 1.1f, 0); drawBox(0.13f, 0.10f, D * 0.9f); glPopMatrix();
    glPushMatrix(); glTranslatef( W * 0.45f, platH + 0.4f, 0); drawBox(0.13f, 0.10f, D * 0.9f); glPopMatrix();
    glPushMatrix(); glTranslatef( W * 0.45f, platH + 1.1f, 0); drawBox(0.13f, 0.10f, D * 0.9f); glPopMatrix();

    // vertical bamboo spindles (golden)
    setMat(0.92f, 0.88f, 0.62f);
    for (int i = 0; i < 9; ++i) {
        float xx = -W * 0.42f + i * (W * 0.84f / 8.0f);
        glPushMatrix(); glTranslatef(xx, platH + 0.75f, -D * 0.45f); drawBox(0.07f, 0.70f, 0.07f); glPopMatrix();
        glPushMatrix(); glTranslatef(xx, platH + 0.75f,  D * 0.45f); drawBox(0.07f, 0.70f, 0.07f); glPopMatrix();
    }

    // ---- interior bamboo tables, chairs, and seated people ----
    // 3x3 grid of tables. Each table has a 4-bit occupancy mask (which chairs are taken).
    // Some tables are fully occupied, some half, some empty.
    int tableOccupancy[9] = { 0xF, 0x5, 0x0, 0x3, 0x9, 0xA, 0x6, 0x0, 0xC };
    float cushColors[4][3] = {
        {0.95f, 0.30f, 0.30f}, {0.30f, 0.55f, 0.95f},
        {0.95f, 0.85f, 0.25f}, {0.50f, 0.85f, 0.40f}
    };
    float shirtColors[8][3] = {
        {0.85f, 0.20f, 0.30f}, {0.20f, 0.55f, 0.90f},
        {0.95f, 0.70f, 0.20f}, {0.30f, 0.75f, 0.40f},
        {0.65f, 0.30f, 0.85f}, {0.95f, 0.45f, 0.15f},
        {0.20f, 0.75f, 0.85f}, {0.85f, 0.45f, 0.65f}
    };
    float chairOff = 0.85f;
    float chairPos[4][2] = { { chairOff, 0 }, { -chairOff, 0 },
                             { 0, chairOff }, { 0, -chairOff } };

    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col) {
            float tx = -W * 0.30f + col * (W * 0.30f);
            float tz = -D * 0.30f + row * (D * 0.30f);
            int occ = tableOccupancy[row * 3 + col];

            // table top (bamboo color)
            setMat(0.78f, 0.65f, 0.40f);
            glPushMatrix();
            glTranslatef(tx, platH + 0.80f, tz);
            drawBox(1.1f, 0.08f, 1.1f);
            glPopMatrix();
            // table leg (bamboo)
            setMat(0.78f, 0.72f, 0.42f);
            glPushMatrix();
            glTranslatef(tx, platH + 0.40f, tz);
            drawBox(0.12f, 0.80f, 0.12f);
            glPopMatrix();

            // small candle on every table (glows at night)
            {
                // candle holder
                setMat(0.40f, 0.30f, 0.20f);
                glPushMatrix();
                glTranslatef(tx + 0.35f, platH + 0.85f, tz + 0.35f);
                drawBox(0.10f, 0.04f, 0.10f);
                glPopMatrix();
                // candle body
                setMat(0.95f, 0.90f, 0.85f);
                glPushMatrix();
                glTranslatef(tx + 0.35f, platH + 0.97f, tz + 0.35f);
                drawBox(0.06f, 0.20f, 0.06f);
                glPopMatrix();
                // flame (emissive)
                if (night || sunset) {
                    GLfloat em[4] = { 1.0f, 0.65f, 0.20f, 1.0f };
                    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
                }
                setMat(1.0f, 0.75f, 0.25f);
                glPushMatrix();
                glTranslatef(tx + 0.35f, platH + 1.13f, tz + 0.35f);
                glScalef(0.04f, 0.10f, 0.04f);
                glutSolidSphere(1.0, 8, 6);
                glPopMatrix();
                GLfloat zero[4] = { 0, 0, 0, 1 };
                glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
            }

            // if any chair occupied, put food/plates on table
            if (occ != 0) {
                // white plate
                setMat(0.95f, 0.95f, 0.95f);
                glPushMatrix();
                glTranslatef(tx, platH + 0.86f, tz);
                glScalef(0.45f, 0.04f, 0.45f);
                glutSolidSphere(1.0, 12, 6);
                glPopMatrix();
                // small food spot (orange = fish/curry)
                setMat(0.95f, 0.45f, 0.20f);
                glPushMatrix();
                glTranslatef(tx, platH + 0.89f, tz);
                glScalef(0.15f, 0.04f, 0.15f);
                glutSolidSphere(1.0, 10, 6);
                glPopMatrix();
                // small water glass (blue)
                setMat(0.55f, 0.80f, 0.95f);
                glPushMatrix();
                glTranslatef(tx + 0.30f, platH + 0.90f, tz + 0.30f);
                drawBox(0.08f, 0.20f, 0.08f);
                glPopMatrix();
            }

            // 4 chairs + a person if that chair is occupied
            for (int c = 0; c < 4; ++c) {
                // chair cushion (colored)
                setMat(cushColors[c][0], cushColors[c][1], cushColors[c][2]);
                glPushMatrix();
                glTranslatef(tx + chairPos[c][0], platH + 0.40f, tz + chairPos[c][1]);
                drawBox(0.45f, 0.10f, 0.45f);
                glPopMatrix();

                // chair backrest (small)
                setMat(cushColors[c][0] * 0.7f, cushColors[c][1] * 0.7f, cushColors[c][2] * 0.7f);
                glPushMatrix();
                float bx = chairPos[c][0] * 1.20f;
                float bz = chairPos[c][1] * 1.20f;
                glTranslatef(tx + bx, platH + 0.70f, tz + bz);
                drawBox(0.45f, 0.45f, 0.06f);
                glPopMatrix();

                // person sitting on this chair?
                if (occ & (1 << c)) {
                    int sIdx = (row * 3 + col + c) % 8;
                    // shirt/torso
                    setMat(shirtColors[sIdx][0], shirtColors[sIdx][1], shirtColors[sIdx][2]);
                    glPushMatrix();
                    glTranslatef(tx + chairPos[c][0], platH + 0.75f, tz + chairPos[c][1]);
                    drawBox(0.32f, 0.45f, 0.28f);
                    glPopMatrix();
                    // arms (shirt color, small boxes on sides)
                    glPushMatrix();
                    glTranslatef(tx + chairPos[c][0] + 0.20f, platH + 0.70f, tz + chairPos[c][1]);
                    drawBox(0.10f, 0.32f, 0.20f);
                    glPopMatrix();
                    glPushMatrix();
                    glTranslatef(tx + chairPos[c][0] - 0.20f, platH + 0.70f, tz + chairPos[c][1]);
                    drawBox(0.10f, 0.32f, 0.20f);
                    glPopMatrix();
                    // head (skin tone)
                    setMat(0.92f, 0.78f, 0.62f);
                    glPushMatrix();
                    glTranslatef(tx + chairPos[c][0], platH + 1.10f, tz + chairPos[c][1]);
                    glutSolidSphere(0.13, 12, 10);
                    glPopMatrix();
                    // hair (dark)
                    setMat(0.15f, 0.10f, 0.05f);
                    glPushMatrix();
                    glTranslatef(tx + chairPos[c][0], platH + 1.18f, tz + chairPos[c][1]);
                    glScalef(0.14f, 0.06f, 0.14f);
                    glutSolidSphere(1.0, 10, 8);
                    glPopMatrix();
                }
            }
        }

    // ---- TALL PEAKED THATCHED ROOF - light warm tan thatch ----
    float roofBaseY = platH + postH;
    float ridgeY    = roofBaseY + roofH;
    setMat(0.90f, 0.70f, 0.45f);    // light warm tan thatch

    // Left slant panel
    glBegin(GL_QUADS);
    glNormal3f(-1, 0.5f, 0);
    glVertex3f(-W * 0.55f, roofBaseY, -D * 0.55f);
    glVertex3f(-W * 0.55f, roofBaseY,  D * 0.55f);
    glVertex3f( 0.0f,      ridgeY,     D * 0.55f);
    glVertex3f( 0.0f,      ridgeY,    -D * 0.55f);
    glEnd();
    // Right slant panel
    glBegin(GL_QUADS);
    glNormal3f(1, 0.5f, 0);
    glVertex3f( W * 0.55f, roofBaseY, -D * 0.55f);
    glVertex3f( W * 0.55f, roofBaseY,  D * 0.55f);
    glVertex3f( 0.0f,      ridgeY,     D * 0.55f);
    glVertex3f( 0.0f,      ridgeY,    -D * 0.55f);
    glEnd();

    // Front gable triangle (closes the front face)
    setMat(0.82f, 0.62f, 0.40f);    // lighter tan gable
    glBegin(GL_TRIANGLES);
    glNormal3f(0, 0, 1);
    glVertex3f(-W * 0.55f, roofBaseY,  D * 0.55f);
    glVertex3f( W * 0.55f, roofBaseY,  D * 0.55f);
    glVertex3f( 0.0f,      ridgeY,     D * 0.55f);
    // Back gable triangle
    glNormal3f(0, 0, -1);
    glVertex3f(-W * 0.55f, roofBaseY, -D * 0.55f);
    glVertex3f( W * 0.55f, roofBaseY, -D * 0.55f);
    glVertex3f( 0.0f,      ridgeY,    -D * 0.55f);
    glEnd();

    // thatch texture lines - subtle medium brown stripes for thatching detail
    setMat(0.60f, 0.42f, 0.22f);
    glBegin(GL_QUADS);
    for (int i = 1; i < 8; ++i) {
        float t = i / 8.0f;
        // left side stripe
        float x0 = -W * 0.55f * (1 - t);
        float y0 = roofBaseY + roofH * t;
        glNormal3f(-1, 0.5f, 0);
        glVertex3f(x0,         y0,         -D * 0.55f);
        glVertex3f(x0,         y0,          D * 0.55f);
        glVertex3f(x0 + 0.05f, y0 - 0.02f,  D * 0.55f);
        glVertex3f(x0 + 0.05f, y0 - 0.02f, -D * 0.55f);
        // right side stripe
        float x1 =  W * 0.55f * (1 - t);
        glNormal3f(1, 0.5f, 0);
        glVertex3f(x1,         y0,         -D * 0.55f);
        glVertex3f(x1,         y0,          D * 0.55f);
        glVertex3f(x1 - 0.05f, y0 - 0.02f,  D * 0.55f);
        glVertex3f(x1 - 0.05f, y0 - 0.02f, -D * 0.55f);
    }
    glEnd();

    // ---- entrance steps in front ----
    setMat(0.45f, 0.28f, 0.14f);
    for (int i = 0; i < 3; ++i) {
        glPushMatrix();
        glTranslatef(0, 0.1f + i * 0.13f, D * 0.5f + 0.5f + i * 0.4f);
        drawBox(3.0f - i * 0.4f, 0.13f, 0.4f);
        glPopMatrix();
    }

    // ---- decorative lantern lights (warm glow at night) ----
    if (night || sunset) {
        GLfloat em[4] = { 1.0f, 0.75f, 0.30f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
    }
    setMat(1.0f, 0.85f, 0.45f);
    for (int i = 0; i < 4; ++i) {
        float ax = -W * 0.35f + i * (W * 0.23f);
        glPushMatrix();
        glTranslatef(ax, platH + postH + 0.5f, D * 0.45f);
        glutSolidSphere(0.15, 10, 10);
        glPopMatrix();
    }
    GLfloat zero[4] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);

    // ---- HANGING LANTERNS under the roof (bright at night/sunset) ----
    {
        float lanternY = platH + postH - 0.4f;
        float lanternPos[8][2] = {
            { -W * 0.30f, -D * 0.25f }, { 0.0f,       -D * 0.25f }, { W * 0.30f, -D * 0.25f },
            { -W * 0.30f,  D * 0.25f }, { W * 0.30f,  D * 0.25f },
            { -W * 0.15f,  0.0f },      { W * 0.15f,  0.0f },
            { 0.0f,        D * 0.40f }
        };
        for (int i = 0; i < 8; ++i) {
            float lx = lanternPos[i][0];
            float lz = lanternPos[i][1];

            // light brown thin cord
            setMat(0.65f, 0.50f, 0.30f);
            glPushMatrix();
            glTranslatef(lx, lanternY + 0.30f, lz);
            drawBox(0.02f, 0.60f, 0.02f);
            glPopMatrix();

            // lantern body
            if (night || sunset) {
                GLfloat em[4] = { 1.0f, 0.75f, 0.30f, 1.0f };
                glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
            }
            setMat(1.0f, 0.85f, 0.40f);
            glPushMatrix();
            glTranslatef(lx, lanternY, lz);
            drawBox(0.30f, 0.45f, 0.30f);
            glPopMatrix();
            // bulb below
            setMat(1.0f, 0.95f, 0.55f);
            glPushMatrix();
            glTranslatef(lx, lanternY - 0.05f, lz);
            glutSolidSphere(0.18, 12, 10);
            glPopMatrix();
            GLfloat zero[4] = { 0, 0, 0, 1 };
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);

            // glow halo on floor below
            if (night || sunset) {
                glDisable(GL_LIGHTING);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);
                glColor4f(1.0f, 0.80f, 0.30f, 0.35f);
                glBegin(GL_TRIANGLE_FAN);
                glVertex3f(lx, platH + 0.02f, lz);
                for (int s = 0; s <= 12; ++s) {
                    float a = s * (2 * PI / 12.0f);
                    glVertex3f(lx + cosf(a) * 1.6f, platH + 0.02f, lz + sinf(a) * 1.6f);
                }
                glEnd();
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
                glEnable(GL_LIGHTING);
                glColor3f(1, 1, 1);
            }
        }
    }

    // "OCEAN KITCHEN" signboard placed INSIDE the gable triangle
    // Use 40% height where there's more horizontal room
    float signY = roofBaseY + roofH * 0.42f;
    float signMaxHalfW = W * 0.55f * (1.0f - 0.42f);         // half-width at this height
    float signW = signMaxHalfW * 1.65f;                      // safely inside triangle
    if (night || sunset) {
        GLfloat em2[4] = { 0.10f, 0.35f, 0.70f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em2);
    }
    setMat(0.10f, 0.25f, 0.55f);    // navy blue ocean theme
    glPushMatrix();
    glTranslatef(0, signY, D * 0.55f + 0.12f);
    drawBox(signW, 1.0f, 0.18f);
    glPopMatrix();
    GLfloat zero2[4] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero2);

    // "Seafood" name - big Times-Roman style, thin lines, fits inside triangle
    // At 42% gable height, triangle half-width = W*0.55*(1-0.42) = ~7.66 (W=24)
    // "Seafood" 7 chars * ~85 stroke = ~595 units. With charH=1.6, world width = 9.5 ✓
    draw3DText("Seafood",
               0.0f,
               signY,
               D * 0.55f + 0.24f,
               1.60f,
               1.00f, 0.95f, 0.20f);         // bright yellow

    // ---- SEAFOOD CARTOON DECORATIONS - FRONT GABLE TRIANGLE ONLY ----
    // All decorations confined to the front triangular gable face
    // (gable goes from y=roofBaseY at x=±W*0.55 up to y=ridgeY at x=0)

    // Gable surface front-most plane: z = D * 0.55f + tiny offset
    float gz = D * 0.55f + 0.30f;       // slightly in front of gable surface

    // gable interior bounds (vertical center axis):
    // at height h above roofBaseY, half-width is W*0.55 * (1 - h/roofH)

    // Blue fish mascot at top of triangle (only decoration kept)
    drawCartoonFish(0, roofBaseY + roofH * 0.80f, gz, 1.20f, 0.20f, 0.55f, 0.95f);

    glPopMatrix();

    // ---- OUTDOOR PATIO in front of restaurant (HUGE) ----
    setMat(0.86f, 0.78f, 0.62f);
    glPushMatrix();
    glTranslatef(cx, -0.43f, cz + D * 0.5f + 9.0f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-W * 0.85f, 0, -5.0f);
    glVertex3f( W * 0.85f, 0, -5.0f);
    glVertex3f( W * 0.85f, 0,  8.0f);
    glVertex3f(-W * 0.85f, 0,  8.0f);
    glEnd();
    glPopMatrix();

    // 3 wooden SQUARE tables with wooden slatted chairs (warm brown wood)
    float outdoorTablePosX[3] = { -W * 0.45f, 0.0f,  W * 0.45f };
    int   outdoorOcc[3]       = {  1,         0,     1 };

    // wood color tones (all warm brown, like the reference photo)
    float woodTop[3]  = { 0.50f, 0.32f, 0.18f };   // darker varnished top
    float woodLeg[3]  = { 0.55f, 0.38f, 0.22f };   // legs
    float woodChair[3]= { 0.50f, 0.32f, 0.18f };   // chair wood

    for (int t = 0; t < 3; ++t) {
        float tx = cx + outdoorTablePosX[t];
        float tz = cz + D * 0.5f + 10.5f;
        float tableH = 1.45f;
        float gY = -0.43f;

        // SQUARE wooden table top
        setMat(woodTop[0], woodTop[1], woodTop[2]);
        glPushMatrix();
        glTranslatef(tx, gY + tableH, tz);
        drawBox(2.20f, 0.12f, 2.20f);
        glPopMatrix();

        // 4 wooden legs (one per corner)
        setMat(woodLeg[0], woodLeg[1], woodLeg[2]);
        float legOff = 0.95f;
        for (int lx = -1; lx <= 1; lx += 2)
            for (int lz = -1; lz <= 1; lz += 2) {
                glPushMatrix();
                glTranslatef(tx + lx * legOff, gY + tableH * 0.5f, tz + lz * legOff);
                drawBox(0.14f, tableH, 0.14f);
                glPopMatrix();
            }
        // small cross-brace under table
        glPushMatrix();
        glTranslatef(tx, gY + tableH * 0.35f, tz);
        drawBox(2.00f, 0.07f, 0.10f);
        glPopMatrix();

        // food + plate on occupied tables
        if (outdoorOcc[t]) {
            setMat(0.95f, 0.95f, 0.95f);
            glPushMatrix();
            glTranslatef(tx, gY + tableH + 0.10f, tz);
            glScalef(0.55f, 0.06f, 0.55f);
            glutSolidSphere(1.0, 16, 6);
            glPopMatrix();
            setMat(0.95f, 0.50f, 0.20f);
            glPushMatrix();
            glTranslatef(tx, gY + tableH + 0.16f, tz);
            glScalef(0.25f, 0.06f, 0.25f);
            glutSolidSphere(1.0, 12, 6);
            glPopMatrix();
            // glass
            setMat(0.55f, 0.80f, 0.95f);
            glPushMatrix();
            glTranslatef(tx + 0.55f, gY + tableH + 0.30f, tz + 0.45f);
            drawBox(0.20f, 0.45f, 0.20f);
            glPopMatrix();
        }

        // 4 wooden CHAIRS (with high slat backrest) around square table
        float chairOffsets[4][2] = { { 2.10f, 0.0f }, { -2.10f, 0.0f },
                                     { 0.0f, 2.10f }, { 0.0f, -2.10f } };
        float chairRot[4]        = { 90.0f, -90.0f, 180.0f, 0.0f };

        for (int c = 0; c < 4; ++c) {
            float ox = chairOffsets[c][0];
            float oz = chairOffsets[c][1];

            glPushMatrix();
            glTranslatef(tx + ox, gY, tz + oz);
            glRotatef(chairRot[c], 0, 1, 0);

            // seat (wooden box)
            setMat(woodChair[0], woodChair[1], woodChair[2]);
            glPushMatrix();
            glTranslatef(0, 0.70f, 0);
            drawBox(0.90f, 0.12f, 0.90f);
            glPopMatrix();

            // 4 chair legs
            for (int lx = -1; lx <= 1; lx += 2)
                for (int lz = -1; lz <= 1; lz += 2) {
                    glPushMatrix();
                    glTranslatef(lx * 0.38f, 0.35f, lz * 0.38f);
                    drawBox(0.10f, 0.70f, 0.10f);
                    glPopMatrix();
                }

            // 2 back posts (extending up from the back of seat)
            setMat(woodChair[0] * 0.85f, woodChair[1] * 0.85f, woodChair[2] * 0.85f);
            glPushMatrix();
            glTranslatef(-0.38f, 1.35f, -0.40f);
            drawBox(0.10f, 1.20f, 0.10f);
            glPopMatrix();
            glPushMatrix();
            glTranslatef( 0.38f, 1.35f, -0.40f);
            drawBox(0.10f, 1.20f, 0.10f);
            glPopMatrix();

            // top rail
            glPushMatrix();
            glTranslatef(0, 1.93f, -0.40f);
            drawBox(0.90f, 0.10f, 0.10f);
            glPopMatrix();

            // 3 vertical slats between back posts
            for (int s = -1; s <= 1; ++s) {
                glPushMatrix();
                glTranslatef(s * 0.22f, 1.45f, -0.40f);
                drawBox(0.08f, 1.00f, 0.06f);
                glPopMatrix();
            }

            // middle horizontal rail (between slats)
            glPushMatrix();
            glTranslatef(0, 1.05f, -0.40f);
            drawBox(0.85f, 0.08f, 0.08f);
            glPopMatrix();

            glPopMatrix();   // end chair

            // person sitting?
            bool seatOcc = false;
            if (outdoorOcc[t]) {
                if (t == 0) seatOcc = (c != 2);          // 3 of 4
                else if (t == 2) seatOcc = true;
            }
            if (seatOcc) {
                int sIdx = (t * 4 + c) % 8;
                float shirtPalette[8][3] = {
                    {0.90f, 0.25f, 0.35f}, {0.20f, 0.55f, 0.90f},
                    {0.95f, 0.70f, 0.20f}, {0.30f, 0.75f, 0.40f},
                    {0.65f, 0.30f, 0.85f}, {0.95f, 0.45f, 0.15f},
                    {0.20f, 0.75f, 0.85f}, {0.85f, 0.45f, 0.65f}
                };
                setMat(shirtPalette[sIdx][0], shirtPalette[sIdx][1], shirtPalette[sIdx][2]);
                glPushMatrix();
                glTranslatef(tx + ox, gY + 1.30f, tz + oz);
                drawBox(0.70f, 0.85f, 0.55f);
                glPopMatrix();
                // arms
                glPushMatrix();
                glTranslatef(tx + ox + 0.42f, gY + 1.20f, tz + oz);
                drawBox(0.20f, 0.70f, 0.42f);
                glPopMatrix();
                glPushMatrix();
                glTranslatef(tx + ox - 0.42f, gY + 1.20f, tz + oz);
                drawBox(0.20f, 0.70f, 0.42f);
                glPopMatrix();
                // head
                setMat(0.92f, 0.78f, 0.62f);
                glPushMatrix();
                glTranslatef(tx + ox, gY + 2.00f, tz + oz);
                glutSolidSphere(0.28, 16, 14);
                glPopMatrix();
                // hair
                setMat(0.10f, 0.06f, 0.02f);
                glPushMatrix();
                glTranslatef(tx + ox, gY + 2.17f, tz + oz);
                glScalef(0.31f, 0.12f, 0.31f);
                glutSolidSphere(1.0, 14, 10);
                glPopMatrix();
            }
        }
    }

    // ---- STRING LIGHTS over outdoor patio (festoon bulbs) ----
    {
        float poleH    = 4.5f;
        float patioZf  = cz + D * 0.5f + 4.5f;    // pole row near restaurant
        float patioZb  = cz + D * 0.5f + 14.0f;   // pole row far (sea-side)
        float poleX[2] = { cx - W * 0.45f, cx + W * 0.45f };
        float poleZ[2] = { patioZf, patioZb };

        // 4 wooden poles at the corners of the outdoor patio
        setMat(0.45f, 0.30f, 0.18f);
        for (int px = 0; px < 2; ++px)
            for (int pz = 0; pz < 2; ++pz) {
                glPushMatrix();
                glTranslatef(poleX[px], -0.43f + poleH * 0.5f, poleZ[pz]);
                drawBox(0.18f, poleH, 0.18f);
                glPopMatrix();
            }

        // 3 string lines stretched across (top of poles)
        // string z positions: front, middle, back
        float stringZ[3] = { patioZf + 0.2f, (patioZf + patioZb) * 0.5f, patioZb - 0.2f };
        float topY       = -0.43f + poleH - 0.10f;

        for (int s = 0; s < 3; ++s) {
            float sz = stringZ[s];

            // light brown thin cord
            glDisable(GL_LIGHTING);
            glColor3f(0.70f, 0.55f, 0.35f);
            glLineWidth(1.2f);
            glBegin(GL_LINE_STRIP);
            const int N = 20;
            for (int i = 0; i <= N; ++i) {
                float t = i / (float)N;
                float lx = poleX[0] + t * (poleX[1] - poleX[0]);
                // catenary sag - lowest in the middle
                float sag = sinf(t * PI) * 0.30f;
                glVertex3f(lx, topY - sag, sz);
            }
            glEnd();
            glLineWidth(1.0f);
            glEnable(GL_LIGHTING);

            // bulbs along the string
            int nBulbs = 12;
            for (int i = 0; i < nBulbs; ++i) {
                float t = (i + 0.5f) / nBulbs;
                float bx = poleX[0] + t * (poleX[1] - poleX[0]);
                float sag = sinf(t * PI) * 0.30f;
                float by = topY - sag - 0.10f;     // bulb hangs below string

                // emissive bulb at night/sunset
                if (night || sunset) {
                    GLfloat em[4] = { 1.0f, 0.85f, 0.40f, 1.0f };
                    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
                }
                setMat(1.0f, 0.92f, 0.55f);
                glPushMatrix();
                glTranslatef(bx, by, sz);
                glutSolidSphere(0.12, 10, 8);
                glPopMatrix();
                GLfloat zero[4] = { 0, 0, 0, 1 };
                glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
            }
        }

        // soft glow patches on patio floor under each string at night
        if (night || sunset) {
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            glColor4f(1.0f, 0.80f, 0.30f, 0.18f);
            glBegin(GL_QUADS);
            glNormal3f(0, 1, 0);
            glVertex3f(cx - W * 0.55f, -0.42f, patioZf - 1.5f);
            glVertex3f(cx + W * 0.55f, -0.42f, patioZf - 1.5f);
            glVertex3f(cx + W * 0.55f, -0.42f, patioZb + 1.5f);
            glVertex3f(cx - W * 0.55f, -0.42f, patioZb + 1.5f);
            glEnd();
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            glEnable(GL_LIGHTING);
            glColor3f(1, 1, 1);
        }
    }

    // ---- BROWN WOODEN GATE (no plank path / stones) ----
    {
        float pathWidth = 4.5f;        // kept only for gate sizing reference
        float pathZgate = -19.0f;

        // ============================================================
        //   BROWN WOODEN GATE with peaked thatched roof
        // ============================================================
        float gateZ = pathZgate;
        float gateHalfW = pathWidth * 0.5f + 1.0f;
        float postH    = 5.2f;
        float postTopY = -0.43f + postH;

        // BROWN COLUMNS (2 thick wooden pillars)
        setMat(0.50f, 0.32f, 0.18f);
        glPushMatrix();
        glTranslatef(cx - gateHalfW, -0.43f + postH * 0.5f, gateZ);
        drawBox(0.65f, postH, 0.65f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(cx + gateHalfW, -0.43f + postH * 0.5f, gateZ);
        drawBox(0.65f, postH, 0.65f);
        glPopMatrix();

        // brown horizontal top beam connecting the columns
        setMat(0.55f, 0.36f, 0.20f);
        glPushMatrix();
        glTranslatef(cx, postTopY - 0.30f, gateZ);
        drawBox(gateHalfW * 2.0f + 0.65f, 0.55f, 0.75f);
        glPopMatrix();

        // PEAKED THATCHED ROOF on top of the gate
        {
            float rB = postTopY + 0.05f;            // roof base above beam
            float rH = 1.6f;                         // roof peak height
            float rW = gateHalfW * 2.0f + 2.2f;     // roof extends past columns
            float rD = 2.0f;                         // roof depth
            // slant panels (warm tan thatch)
            setMat(0.55f, 0.40f, 0.25f);
            // left slant
            glBegin(GL_QUADS);
            glNormal3f(-1, 0.5f, 0);
            glVertex3f(cx - rW * 0.5f, rB, gateZ - rD * 0.5f);
            glVertex3f(cx - rW * 0.5f, rB, gateZ + rD * 0.5f);
            glVertex3f(cx,             rB + rH, gateZ + rD * 0.5f);
            glVertex3f(cx,             rB + rH, gateZ - rD * 0.5f);
            glEnd();
            // right slant
            glBegin(GL_QUADS);
            glNormal3f(1, 0.5f, 0);
            glVertex3f(cx + rW * 0.5f, rB, gateZ - rD * 0.5f);
            glVertex3f(cx + rW * 0.5f, rB, gateZ + rD * 0.5f);
            glVertex3f(cx,             rB + rH, gateZ + rD * 0.5f);
            glVertex3f(cx,             rB + rH, gateZ - rD * 0.5f);
            glEnd();
            // front + back gable triangles
            setMat(0.48f, 0.34f, 0.18f);
            glBegin(GL_TRIANGLES);
            glNormal3f(0, 0, 1);
            glVertex3f(cx - rW * 0.5f, rB, gateZ + rD * 0.5f);
            glVertex3f(cx + rW * 0.5f, rB, gateZ + rD * 0.5f);
            glVertex3f(cx,             rB + rH, gateZ + rD * 0.5f);
            glNormal3f(0, 0, -1);
            glVertex3f(cx - rW * 0.5f, rB, gateZ - rD * 0.5f);
            glVertex3f(cx + rW * 0.5f, rB, gateZ - rD * 0.5f);
            glVertex3f(cx,             rB + rH, gateZ - rD * 0.5f);
            glEnd();
            // ridge line + eave trim
            setMat(0.35f, 0.20f, 0.10f);
            glPushMatrix();
            glTranslatef(cx, rB + rH + 0.05f, gateZ);
            drawBox(rW + 0.2f, 0.12f, rD + 0.10f);
            glPopMatrix();
        }

        // BROWN LATTICE side fences (3 horizontal + vertical bars on each side)
        setMat(0.50f, 0.32f, 0.18f);
        for (int side = -1; side <= 1; side += 2) {
            float fenceCenterX = cx + side * (gateHalfW + 1.6f);
            // 3 horizontal rails
            for (int h = 0; h < 3; ++h) {
                glPushMatrix();
                glTranslatef(fenceCenterX, -0.43f + 1.0f + h * 1.5f, gateZ);
                drawBox(1.8f, 0.10f, 0.15f);
                glPopMatrix();
            }
            // 5 vertical pickets
            for (int v = 0; v < 5; ++v) {
                float pxv = fenceCenterX - 0.7f + v * 0.35f;
                glPushMatrix();
                glTranslatef(pxv, -0.43f + 2.30f, gateZ);
                drawBox(0.08f, 3.50f, 0.10f);
                glPopMatrix();
            }
        }

        // hanging "Seafood" sign panel (between columns, navy blue)
        if (night || sunset) {
            GLfloat em[4] = { 0.20f, 0.40f, 0.75f, 1.0f };
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
        }
        setMat(0.10f, 0.25f, 0.55f);
        glPushMatrix();
        glTranslatef(cx, postTopY - 1.40f, gateZ + 0.45f);
        drawBox(2.8f, 0.85f, 0.10f);
        glPopMatrix();
        GLfloat zero[4] = { 0, 0, 0, 1 };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
        // sign hanging cords (thin)
        setMat(0.65f, 0.50f, 0.30f);
        glPushMatrix();
        glTranslatef(cx - 1.0f, postTopY - 0.90f, gateZ + 0.40f);
        drawBox(0.025f, 0.45f, 0.025f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(cx + 1.0f, postTopY - 0.90f, gateZ + 0.40f);
        drawBox(0.025f, 0.45f, 0.025f);
        glPopMatrix();
        // "Seafood" text
        draw3DText("Seafood",
                   cx, postTopY - 1.40f, gateZ + 0.51f,
                   0.55f,
                   1.00f, 0.95f, 0.20f);

        // 2 hanging lanterns on the front of the beam
        if (night || sunset) {
            GLfloat em[4] = { 1.0f, 0.75f, 0.30f, 1.0f };
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
        }
        setMat(1.0f, 0.85f, 0.40f);
        glPushMatrix();
        glTranslatef(cx - gateHalfW + 0.45f, postTopY - 0.60f, gateZ + 0.45f);
        drawBox(0.32f, 0.50f, 0.32f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(cx + gateHalfW - 0.45f, postTopY - 0.60f, gateZ + 0.45f);
        drawBox(0.32f, 0.50f, 0.32f);
        glPopMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
    }

    // ---- 2 ANIMATED WAITERS - one walking OUT (with food), one walking IN ----
    // Endpoints in world Z:
    float zInside  = cz - D * 0.10f;            // inside the restaurant platform
    float zOutside = cz + D * 0.5f + 11.0f;     // outside, by the patio tables

    for (int w = 0; w < 2; ++w) {
        // each waiter has its own phase offset so they're at different points in the cycle
        float cycleT = fmodf(waiterPhase + w * PI, 2.0f * PI) / (2.0f * PI);   // 0..1

        float wz, facing;
        if (w == 0) {
            // Waiter A: walks OUT from restaurant → patio (carrying food out)
            wz     = zInside + cycleT * (zOutside - zInside);
            facing = 0.0f;          // faces +Z (toward patio / camera)
        } else {
            // Waiter B: walks IN from patio → restaurant (returning to kitchen)
            wz     = zOutside - cycleT * (zOutside - zInside);
            facing = 180.0f;        // faces -Z (back into restaurant)
        }

        // each in their own lane to avoid colliding
        float wx = cx + (w == 0 ? -W * 0.15f : W * 0.15f);

        // walking animation
        float walkSpeed = 6.0f;
        float legSwing  = sinf(waiterPhase * walkSpeed + w * PI) * 30.0f;
        float bob       = fabsf(sinf(waiterPhase * walkSpeed + w * PI)) * 0.10f;
        float gY        = -0.43f + bob;

        glPushMatrix();
        glTranslatef(wx, 0.0f, wz);
        glRotatef(facing, 0, 1, 0);

        // white shirt / vest
        setMat(0.98f, 0.98f, 0.98f);
        glPushMatrix();
        glTranslatef(0, gY + 1.85f, 0);
        drawBox(0.85f, 1.25f, 0.62f);
        glPopMatrix();
        // black bow-tie
        setMat(0.10f, 0.10f, 0.10f);
        glPushMatrix();
        glTranslatef(0, gY + 2.30f, 0.32f);
        drawBox(0.30f, 0.12f, 0.04f);
        glPopMatrix();

        // 2 individual legs - SWINGING (animated)
        setMat(0.18f, 0.18f, 0.22f);
        // left leg
        glPushMatrix();
        glTranslatef(-0.20f, gY + 0.65f, 0);
        glRotatef(legSwing, 1, 0, 0);     // swing along Z axis
        drawBox(0.32f, 1.25f, 0.30f);
        glPopMatrix();
        // right leg (opposite swing)
        glPushMatrix();
        glTranslatef(0.20f, gY + 0.65f, 0);
        glRotatef(-legSwing, 1, 0, 0);
        drawBox(0.32f, 1.25f, 0.30f);
        glPopMatrix();
        // shoes (also swing with legs)
        setMat(0.05f, 0.05f, 0.05f);
        glPushMatrix();
        glTranslatef(-0.20f, gY + 0.65f, 0);
        glRotatef(legSwing, 1, 0, 0);
        glTranslatef(0, -0.65f, 0.12f);
        drawBox(0.32f, 0.16f, 0.45f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.20f, gY + 0.65f, 0);
        glRotatef(-legSwing, 1, 0, 0);
        glTranslatef(0, -0.65f, 0.12f);
        drawBox(0.32f, 0.16f, 0.45f);
        glPopMatrix();

        // arms - holding tray forward (slight sway in sync with legs)
        float armSway = sinf(waiterPhase * walkSpeed + w * PI) * 0.05f;
        setMat(0.98f, 0.98f, 0.98f);
        glPushMatrix();
        glTranslatef(0.54f, gY + 2.05f + armSway, 0.25f);
        drawBox(0.24f, 0.72f, 0.48f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(-0.54f, gY + 2.05f - armSway, 0.25f);
        drawBox(0.24f, 0.72f, 0.48f);
        glPopMatrix();
        // head
        setMat(0.92f, 0.78f, 0.62f);
        glPushMatrix();
        glTranslatef(0, gY + 2.85f, 0);
        glutSolidSphere(0.32, 16, 14);
        glPopMatrix();
        // hair
        setMat(0.08f, 0.05f, 0.02f);
        glPushMatrix();
        glTranslatef(0, gY + 3.05f, 0);
        glScalef(0.35f, 0.15f, 0.35f);
        glutSolidSphere(1.0, 14, 10);
        glPopMatrix();

        // tray held forward
        setMat(0.55f, 0.38f, 0.20f);
        glPushMatrix();
        glTranslatef(0, gY + 2.30f, 0.80f);
        drawBox(1.25f, 0.10f, 0.95f);
        glPopMatrix();
        // plate
        setMat(0.95f, 0.95f, 0.95f);
        glPushMatrix();
        glTranslatef(0, gY + 2.38f, 0.80f);
        glScalef(0.42f, 0.06f, 0.42f);
        glutSolidSphere(1.0, 14, 6);
        glPopMatrix();
        // orange food
        setMat(0.95f, 0.45f, 0.20f);
        glPushMatrix();
        glTranslatef(0, gY + 2.45f, 0.80f);
        glScalef(0.20f, 0.07f, 0.20f);
        glutSolidSphere(1.0, 12, 6);
        glPopMatrix();
        // glass
        setMat(0.55f, 0.80f, 0.95f);
        glPushMatrix();
        glTranslatef(-0.45f, gY + 2.55f, 0.80f);
        drawBox(0.20f, 0.40f, 0.20f);
        glPopMatrix();

        glPopMatrix();   // end waiter transform
    }
}

// ============================================================
//  OCEAN BREEZE INN — Mediterranean blue-dome style
// ============================================================
static void drawOceanBreezeInn(float cx, float cz)
{
    float gY=-0.50f, W=22.0f, D=14.0f, H=10.0f, fY=gY+0.20f;
    GLUquadric* q=gluNewQuadric();
    setMat(0.96f,0.95f,0.92f);
    glPushMatrix(); glTranslatef(cx,fY+H*0.5f,cz); drawBox(W,H,D); glPopMatrix();
    setMat(0.18f,0.40f,0.80f);
    glPushMatrix(); glTranslatef(cx,fY+H+2.2f,cz); glScalef(1,0.55f,1); glutSolidSphere(4.2f,16,12); glPopMatrix();
    setMat(0.96f,0.95f,0.92f); // drum under dome
    glPushMatrix(); glTranslatef(cx,fY+H+0.8f,cz); drawBox(5.0f,1.6f,5.0f); glPopMatrix();
    // Blue windows and shutters
    setMat(0.18f,0.40f,0.78f);
    for(int w=0;w<3;++w){
        float wx=cx-W*0.33f+w*W*0.33f;
        glPushMatrix(); glTranslatef(wx,fY+H*0.64f,cz+D*0.5f+0.06f); drawBox(2.4f,2.8f,0.14f); glPopMatrix();
        glPushMatrix(); glTranslatef(wx,fY+H*0.24f,cz+D*0.5f+0.06f); drawBox(2.4f,2.2f,0.14f); glPopMatrix();
    }
    // Entrance arch
    setMat(0.18f,0.40f,0.80f);
    glPushMatrix(); glTranslatef(cx,fY+2.2f,cz+D*0.5f+0.08f); drawBox(3.2f,4.4f,0.16f); glPopMatrix();
    setMat(0.14f,0.35f,0.76f);
    glPushMatrix(); glTranslatef(cx,fY+4.6f,cz+D*0.5f+0.08f); glScalef(1,0.5f,1); glutSolidSphere(1.62f,12,8); glPopMatrix();
    // Terrace railing
    setMat(0.18f,0.40f,0.78f);
    glPushMatrix(); glTranslatef(cx,fY+H+0.30f,cz+D*0.5f+0.22f); drawBox(W+0.6f,0.50f,0.12f); glPopMatrix();
    { float bW=15.0f,bH=1.8f,bY=fY+H-0.90f,bZ=cz+D*0.5f+0.22f;
      setMat(0.95f,0.95f,0.95f);
      glPushMatrix(); glTranslatef(cx-bW*0.40f,bY+bH*0.5f+0.22f,bZ); drawBox(0.04f,0.44f,0.04f); glPopMatrix();
      glPushMatrix(); glTranslatef(cx+bW*0.40f,bY+bH*0.5f+0.22f,bZ); drawBox(0.04f,0.44f,0.04f); glPopMatrix();
      setMat(0.12f,0.30f,0.65f);
      glPushMatrix(); glTranslatef(cx,bY,bZ); drawBox(bW,bH,0.10f); glPopMatrix();
      draw3DText("OCEAN BREEZE", cx, bY+0.25f, bZ+0.07f, 0.70f, 0.96f,0.96f,0.96f);
      draw3DText("INN", cx, bY-0.58f, bZ+0.07f, 0.50f, 0.96f,0.96f,0.96f); }
    gluDeleteQuadric(q);
}

// ============================================================
//  SEA MIST CAFE — colourful tropical restaurant/cafe
// ============================================================
static void drawSeaMistCafe(float cx, float cz)
{
    float gY=-0.50f, W=20.0f, D=13.0f, H=7.0f, fY=gY+0.20f;
    GLUquadric* q=gluNewQuadric();
    // Main body — teal green
    setMat(0.20f,0.62f,0.58f);
    glPushMatrix(); glTranslatef(cx,fY+H*0.5f,cz); drawBox(W,H,D); glPopMatrix();
    // Coral-pink trim stripe
    setMat(0.95f,0.45f,0.35f);
    glPushMatrix(); glTranslatef(cx,fY+H,cz); drawBox(W+0.4f,0.40f,D+0.4f); glPopMatrix();
    // Pitched thatch roof
    for(int s=-1;s<=1;s+=2){
        setMat(0.55f,0.38f,0.18f);
        glBegin(GL_TRIANGLES); glNormal3f(s*0.6f,1,0);
        glVertex3f(cx-s*W*0.5f,fY+H,cz-D*0.5f); glVertex3f(cx-s*W*0.5f,fY+H,cz+D*0.5f);
        glVertex3f(cx,fY+H+3.5f,cz-D*0.5f);
        glVertex3f(cx-s*W*0.5f,fY+H,cz+D*0.5f); glVertex3f(cx,fY+H+3.5f,cz+D*0.5f);
        glVertex3f(cx,fY+H+3.5f,cz-D*0.5f);
        glEnd();
    }
    // Ridge beam
    setMat(0.40f,0.26f,0.10f);
    glPushMatrix(); glTranslatef(cx,fY+H+3.5f,cz); drawBox(0.28f,0.28f,D+0.4f); glPopMatrix();
    // Large windows (3)
    setMat(0.55f,0.82f,0.80f);
    for(int w=0;w<3;++w){
        float wx=cx-W*0.33f+w*W*0.33f;
        glPushMatrix(); glTranslatef(wx,fY+H*0.52f,cz+D*0.5f+0.06f); drawBox(3.0f,3.2f,0.14f); glPopMatrix();
    }
    // Entrance with canopy
    setMat(0.95f,0.45f,0.35f);
    glPushMatrix(); glTranslatef(cx,fY+H*0.30f+1.2f,cz+D*0.5f+2.0f); drawBox(5.5f,0.20f,4.0f); glPopMatrix();
    // Canopy poles
    for(int p=-1;p<=1;p+=2){
        glPushMatrix(); glTranslatef(cx+p*2.2f,fY+H*0.30f,cz+D*0.5f+3.8f); glRotatef(-90,1,0,0);
        gluCylinder(q,0.09f,0.09f,H*0.30f+1.2f,8,1); glPopMatrix();
    }
    // Outdoor tables (3 small circular)
    setMat(0.65f,0.48f,0.22f);
    for(int t=-1;t<=1;++t){ glPushMatrix(); glTranslatef(cx+t*3.5f,gY+0.55f,cz+D*0.5f+5.0f); glScalef(1,0.18f,1); glutSolidSphere(0.80f,10,6); glPopMatrix(); }
    { float bW=13.0f,bH=1.8f,bY=fY+H+0.10f,bZ=cz+D*0.5f+0.22f;
      setMat(0.95f,0.95f,0.95f);
      glPushMatrix(); glTranslatef(cx-bW*0.40f,bY+bH*0.5f+0.22f,bZ); drawBox(0.04f,0.44f,0.04f); glPopMatrix();
      glPushMatrix(); glTranslatef(cx+bW*0.40f,bY+bH*0.5f+0.22f,bZ); drawBox(0.04f,0.44f,0.04f); glPopMatrix();
      setMat(0.14f,0.46f,0.42f);
      glPushMatrix(); glTranslatef(cx,bY,bZ); drawBox(bW,bH,0.10f); glPopMatrix();
      draw3DText("SEA MIST", cx, bY+0.25f, bZ+0.07f, 0.80f, 0.96f,0.92f,0.30f);
      draw3DText("CAFE & GRILL", cx, bY-0.60f, bZ+0.07f, 0.48f, 0.96f,0.96f,0.96f); }
    gluDeleteQuadric(q);
}

// ============================================================
//  NEPTUNE RESORT — large curved-facade resort hotel
// ============================================================
static void drawNeptuneResort(float cx, float cz)
{
    float gY=-0.50f, W=26.0f, D=16.0f, H=18.0f, fY=gY+0.25f;
    GLUquadric* q=gluNewQuadric();
    // Base podium
    setMat(0.80f,0.82f,0.84f);
    glPushMatrix(); glTranslatef(cx,gY+0.12f,cz); drawBox(W+2.0f,0.25f,D+2.0f); glPopMatrix();
    // Main tower — ocean-blue glass
    setMat(0.22f,0.52f,0.75f);
    glPushMatrix(); glTranslatef(cx,fY+H*0.5f,cz); drawBox(W,H,D); glPopMatrix();
    // White horizontal bands
    setMat(0.92f,0.94f,0.96f);
    for(int f=0;f<6;++f){
        float fy=fY+H*f/5.0f;
        glPushMatrix(); glTranslatef(cx,fy,cz+D*0.5f+0.07f); drawBox(W+0.5f,0.32f,0.14f); glPopMatrix();
    }
    // Balconies (alternate floors)
    setMat(0.88f,0.90f,0.92f);
    for(int f=1;f<5;f+=1){
        float fy=fY+H*f/5.0f+0.40f;
        glPushMatrix(); glTranslatef(cx,fy,cz+D*0.5f+0.65f); drawBox(W+0.8f,0.55f,0.12f); glPopMatrix();
    }
    // Lobby ground floor glass
    setMat(0.18f,0.48f,0.72f);
    glPushMatrix(); glTranslatef(cx,fY+2.8f,cz+D*0.5f+0.12f); drawBox(W,5.6f,0.18f); glPopMatrix();
    // Entrance portal
    setMat(0.12f,0.40f,0.68f);
    glPushMatrix(); glTranslatef(cx,fY+2.5f,cz+D*0.5f+0.22f); drawBox(5.5f,5.0f,0.14f); glPopMatrix();
    // Roof top cylinder feature
    setMat(0.18f,0.44f,0.70f);
    glPushMatrix(); glTranslatef(cx,fY+H+1.5f,cz); glRotatef(-90,1,0,0);
    gluCylinder(q,3.5f,3.5f,3.0f,20,2); glPopMatrix();
    setMat(0.14f,0.38f,0.66f);
    glPushMatrix(); glTranslatef(cx,fY+H+4.5f,cz); glScalef(1,0.35f,1); glutSolidSphere(3.5f,16,8); glPopMatrix();
    // Flagpole
    setMat(0.72f,0.72f,0.74f);
    glPushMatrix(); glTranslatef(cx,fY+H+4.5f,cz-D*0.3f); glRotatef(-90,1,0,0);
    gluCylinder(q,0.09f,0.09f,5.5f,8,1); glPopMatrix();
    setMat(0.10f,0.28f,0.80f);
    glPushMatrix(); glTranslatef(cx+1.1f,fY+H+9.8f,cz-D*0.3f); drawBox(2.2f,1.2f,0.08f); glPopMatrix();
    { float bW=18.0f,bH=2.2f,bY=fY+H*0.88f,bZ=cz+D*0.5f+0.20f;
      setMat(0.95f,0.95f,0.95f);
      glPushMatrix(); glTranslatef(cx-bW*0.40f,bY+bH*0.5f+0.22f,bZ); drawBox(0.04f,0.44f,0.04f); glPopMatrix();
      glPushMatrix(); glTranslatef(cx+bW*0.40f,bY+bH*0.5f+0.22f,bZ); drawBox(0.04f,0.44f,0.04f); glPopMatrix();
      setMat(0.08f,0.16f,0.42f);
      glPushMatrix(); glTranslatef(cx,bY,bZ); drawBox(bW,bH,0.10f); glPopMatrix();
      draw3DText("NEPTUNE", cx, bY+0.30f, bZ+0.07f, 1.10f, 0.22f,0.72f,1.00f);
      draw3DText("RESORT & SPA", cx, bY-0.76f, bZ+0.07f, 0.52f, 0.96f,0.96f,0.96f); }
    gluDeleteQuadric(q);
}

// ============================================================
//  PALM BAY HOTEL — warm tropical mid-rise
// ============================================================
static void drawPalmBayHotel(float cx, float cz)
{
    float gY=-0.50f, W=24.0f, D=15.0f, H=14.0f, fY=gY+0.22f;
    GLUquadric* q=gluNewQuadric();
    // Ground platform
    setMat(0.84f,0.80f,0.74f);
    glPushMatrix(); glTranslatef(cx,gY+0.12f,cz); drawBox(W+2.0f,0.24f,D+2.0f); glPopMatrix();
    // Main body — warm amber/gold
    setMat(0.88f,0.72f,0.30f);
    glPushMatrix(); glTranslatef(cx,fY+H*0.5f,cz); drawBox(W,H,D); glPopMatrix();
    // Dark brown horizontal trim bands
    setMat(0.30f,0.18f,0.08f);
    for(int f=1;f<5;++f){
        float fy=fY+H*f/5.0f;
        glPushMatrix(); glTranslatef(cx,fy,cz+D*0.5f+0.06f); drawBox(W+0.4f,0.30f,0.12f); glPopMatrix();
    }
    // Windows (4×3 grid)
    setMat(0.45f,0.62f,0.80f);
    for(int row=0;row<3;++row) for(int col=0;col<4;++col){
        float wx=cx-W*0.38f+col*W*0.76f/3.0f;
        float wy=fY+H*0.18f+row*H*0.68f/2.0f;
        glPushMatrix(); glTranslatef(wx,wy,cz+D*0.5f+0.05f); drawBox(2.0f,2.4f,0.14f); glPopMatrix();
    }
    // Lobby — large glass pane
    setMat(0.40f,0.60f,0.82f);
    glPushMatrix(); glTranslatef(cx,fY+2.6f,cz+D*0.5f+0.10f); drawBox(6.0f,5.2f,0.18f); glPopMatrix();
    // Tiered pagoda-style roof
    setMat(0.22f,0.14f,0.06f);
    glPushMatrix(); glTranslatef(cx,fY+H+0.20f,cz); drawBox(W+1.5f,0.50f,D+1.5f); glPopMatrix();
    setMat(0.28f,0.18f,0.08f);
    glPushMatrix(); glTranslatef(cx,fY+H+0.90f,cz); drawBox(W*0.75f,0.50f,D*0.75f); glPopMatrix();
    setMat(0.35f,0.22f,0.10f);
    glPushMatrix(); glTranslatef(cx,fY+H+1.60f,cz); drawBox(W*0.50f,0.50f,D*0.50f); glPopMatrix();
    // Spire on top
    setMat(0.80f,0.65f,0.18f);
    glPushMatrix(); glTranslatef(cx,fY+H+2.0f,cz); glRotatef(-90,1,0,0);
    gluCylinder(q,0.20f,0.02f,4.0f,10,2); glPopMatrix();
    // Entrance steps
    setMat(0.78f,0.72f,0.62f);
    for(int s=0;s<3;++s){
        glPushMatrix(); glTranslatef(cx,gY+0.08f+s*0.10f,cz+D*0.5f+0.55f+s*0.32f);
        drawBox(5.5f,0.10f,0.65f); glPopMatrix();
    }
    // 2 palm trees flanking entrance
    for(int t=-1;t<=1;t+=2){
        float tx=cx+t*4.8f, tz=cz+D*0.5f+3.0f;
        setMat(0.38f,0.24f,0.10f);
        glPushMatrix(); glTranslatef(tx,fY+2.5f,tz); glRotatef(-90,1,0,0);
        gluCylinder(q,0.20f,0.14f,5.0f,8,2); glPopMatrix();
        setMat(0.16f,0.52f,0.20f);
        for(int l=0;l<5;++l){ float la=l*72.0f*PI/180.0f;
            glPushMatrix(); glTranslatef(tx+cosf(la)*1.5f,fY+7.5f+sinf(la)*0.4f,tz+sinf(la)*1.5f);
            glRotatef(l*72.0f,0,1,0); glRotatef(-25,0,0,1); drawBox(0.18f,0.18f,2.5f); glPopMatrix(); }
    }
    { float bW=17.0f,bH=2.0f,bY=fY+H*0.86f,bZ=cz+D*0.5f+0.18f;
      setMat(0.95f,0.95f,0.95f);
      glPushMatrix(); glTranslatef(cx-bW*0.40f,bY+bH*0.5f+0.22f,bZ); drawBox(0.04f,0.44f,0.04f); glPopMatrix();
      glPushMatrix(); glTranslatef(cx+bW*0.40f,bY+bH*0.5f+0.22f,bZ); drawBox(0.04f,0.44f,0.04f); glPopMatrix();
      setMat(0.20f,0.12f,0.04f);
      glPushMatrix(); glTranslatef(cx,bY,bZ); drawBox(bW,bH,0.10f); glPopMatrix();
      draw3DText("PALM BAY", cx, bY+0.26f, bZ+0.07f, 1.00f, 0.90f,0.72f,0.22f);
      draw3DText("HOTEL", cx, bY-0.68f, bZ+0.07f, 0.62f, 0.96f,0.96f,0.96f); }
    gluDeleteQuadric(q);
}

// ============================================================
//  VOLLEYBALL COURT — beach volleyball near Baywatch
// ============================================================
static void drawVolleyballCourt(float cx, float cz)
{
    float gY=-0.50f;
    GLUquadric* q=gluNewQuadric();

    // Sandy court patch
    setMat(0.82f,0.74f,0.56f);
    glBegin(GL_QUADS); glNormal3f(0,1,0);
    glVertex3f(cx-9.0f,gY+0.02f,cz-4.5f); glVertex3f(cx+9.0f,gY+0.02f,cz-4.5f);
    glVertex3f(cx+9.0f,gY+0.02f,cz+4.5f); glVertex3f(cx-9.0f,gY+0.02f,cz+4.5f);
    glEnd();

    // White boundary lines
    glDisable(GL_LIGHTING);
    glColor3f(1,1,1); glLineWidth(2.5f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(cx-8.0f,gY+0.04f,cz-4.0f); glVertex3f(cx+8.0f,gY+0.04f,cz-4.0f);
    glVertex3f(cx+8.0f,gY+0.04f,cz+4.0f); glVertex3f(cx-8.0f,gY+0.04f,cz+4.0f);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(cx,gY+0.04f,cz-4.0f); glVertex3f(cx,gY+0.04f,cz+4.0f);
    glEnd();
    glLineWidth(1.0f); glEnable(GL_LIGHTING);

    // Net poles
    setMat(0.70f,0.70f,0.72f);
    glPushMatrix(); glTranslatef(cx,gY,cz-4.5f); glRotatef(-90,1,0,0); gluCylinder(q,0.09f,0.09f,2.60f,8,1); glPopMatrix();
    glPushMatrix(); glTranslatef(cx,gY,cz+4.5f); glRotatef(-90,1,0,0); gluCylinder(q,0.09f,0.09f,2.60f,8,1); glPopMatrix();
    // Top cable
    setMat(0.80f,0.80f,0.80f);
    glPushMatrix(); glTranslatef(cx,gY+2.56f,cz); drawBox(0.05f,0.05f,9.2f); glPopMatrix();
    // Horizontal net bands
    setMat(0.90f,0.90f,0.92f);
    for(int r=0;r<6;++r){ float ny=gY+0.85f+r*0.28f; glPushMatrix(); glTranslatef(cx,ny,cz); drawBox(0.04f,0.05f,9.0f); glPopMatrix(); }
    // Vertical net strips
    for(int c=0;c<10;++c){ float nz=cz-4.5f+c*1.0f; glPushMatrix(); glTranslatef(cx,gY+1.72f,nz); drawBox(0.02f,1.72f,0.04f); glPopMatrix(); }

    // Volleyball
    setMat(0.95f,0.88f,0.28f);
    glPushMatrix(); glTranslatef(cx+3.5f,gY+0.26f,cz+1.5f); glutSolidSphere(0.24f,14,12); glPopMatrix();

    // Side benches
    setMat(0.42f,0.26f,0.10f);
    for(int b=-1;b<=1;b+=2){
        glPushMatrix(); glTranslatef(cx+b*10.5f,gY+0.30f,cz); drawBox(0.22f,0.60f,4.0f); glPopMatrix();
        glPushMatrix(); glTranslatef(cx+b*10.5f,gY+0.62f,cz); drawBox(2.0f,0.14f,4.2f); glPopMatrix();
    }
    gluDeleteQuadric(q);
}

// ============================================================
//  THE GRAND — 5-Star Restaurant
// ============================================================
static void drawFiveStarRestaurant(float cx, float cz)
{
    float gY=-0.50f, W=30.0f, D=18.0f, H=13.0f;
    float fY=gY+0.30f;
    GLUquadric* q=gluNewQuadric();

    // Platform
    setMat(0.90f,0.88f,0.86f);
    glPushMatrix(); glTranslatef(cx,gY+0.15f,cz); drawBox(W+3.0f,0.30f,D+3.0f); glPopMatrix();

    // Main body (ivory)
    setMat(0.96f,0.94f,0.90f);
    glPushMatrix(); glTranslatef(cx,fY+H*0.5f,cz); drawBox(W,H,D); glPopMatrix();

    // 6 classical columns
    setMat(0.98f,0.97f,0.95f);
    for(int c=0;c<6;++c){
        float cx2=cx-W*0.42f+c*W*0.84f/5.0f;
        glPushMatrix(); glTranslatef(cx2,fY+H*0.5f,cz+D*0.5f+0.35f); drawBox(0.90f,H,0.90f); glPopMatrix();
        setMat(0.88f,0.72f,0.20f);
        glPushMatrix(); glTranslatef(cx2,fY+H+0.12f,cz+D*0.5f+0.35f); drawBox(1.12f,0.30f,1.12f); glPopMatrix();
        setMat(0.98f,0.97f,0.95f);
    }

    // Triangular pediment
    setMat(0.93f,0.91f,0.87f);
    glBegin(GL_TRIANGLES); glNormal3f(0,0.3f,1);
    glVertex3f(cx-W*0.5f,fY+H,      cz+D*0.5f+0.46f);
    glVertex3f(cx+W*0.5f,fY+H,      cz+D*0.5f+0.46f);
    glVertex3f(cx,        fY+H+4.5f, cz+D*0.5f+0.46f);
    glEnd();
    setMat(0.85f,0.70f,0.18f);
    glPushMatrix(); glTranslatef(cx,fY+H+0.08f,cz+D*0.5f+0.50f); drawBox(W+0.4f,0.28f,0.10f); glPopMatrix();

    // Double entrance doors
    setMat(0.22f,0.13f,0.06f);
    glPushMatrix(); glTranslatef(cx-0.70f,fY+2.0f,cz+D*0.5f+0.06f); drawBox(1.20f,4.0f,0.16f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+0.70f,fY+2.0f,cz+D*0.5f+0.06f); drawBox(1.20f,4.0f,0.16f); glPopMatrix();
    setMat(0.85f,0.70f,0.18f);
    glPushMatrix(); glTranslatef(cx-0.08f,fY+2.0f,cz+D*0.5f+0.15f); drawBox(0.10f,0.36f,0.06f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+0.08f,fY+2.0f,cz+D*0.5f+0.15f); drawBox(0.10f,0.36f,0.06f); glPopMatrix();

    // 4 elegant windows with gold frames
    float winXs[4]={cx-W*0.36f,cx-W*0.13f,cx+W*0.13f,cx+W*0.36f};
    for(int w=0;w<4;++w){
        setMat(0.85f,0.70f,0.18f);
        glPushMatrix(); glTranslatef(winXs[w],fY+H*0.57f,cz+D*0.5f+0.05f); drawBox(2.50f,3.50f,0.14f); glPopMatrix();
        setMat(0.55f,0.72f,0.88f);
        glPushMatrix(); glTranslatef(winXs[w],fY+H*0.57f,cz+D*0.5f+0.13f); drawBox(2.10f,3.10f,0.10f); glPopMatrix();
    }

    // Entrance steps
    setMat(0.86f,0.84f,0.82f);
    for(int s=0;s<4;++s){
        glPushMatrix(); glTranslatef(cx,gY+0.08f+s*0.10f,cz+D*0.5f+0.65f+s*0.35f);
        drawBox(6.0f,0.10f,0.70f); glPopMatrix();
    }

    // Banner sign (BBQ style)
    {
        float bW=22.0f, bH=2.6f, bY=fY+H+2.2f, bZ=cz+D*0.5f+0.52f;
        setMat(0.95f,0.95f,0.95f);
        glPushMatrix(); glTranslatef(cx-bW*0.40f,bY+bH*0.5f+0.28f,bZ); drawBox(0.04f,0.56f,0.04f); glPopMatrix();
        glPushMatrix(); glTranslatef(cx+bW*0.40f,bY+bH*0.5f+0.28f,bZ); drawBox(0.04f,0.56f,0.04f); glPopMatrix();
        setMat(0.08f,0.06f,0.04f);
        glPushMatrix(); glTranslatef(cx,bY,bZ); drawBox(bW,bH,0.10f); glPopMatrix();
        draw3DText("THE GRAND", cx, bY+0.42f, bZ+0.07f, 1.50f, 0.88f,0.72f,0.20f);
        draw3DText("5 STAR RESTAURANT", cx, bY-0.80f, bZ+0.07f, 0.52f, 0.96f,0.96f,0.96f);
    }

    // 2 flagpoles
    for(int f=-1;f<=1;f+=2){
        setMat(0.70f,0.70f,0.72f);
        glPushMatrix(); glTranslatef(cx+f*W*0.44f,fY+H+0.20f,cz-D*0.5f+1.5f); glRotatef(-90,1,0,0);
        gluCylinder(q,0.09f,0.09f,5.0f,8,1); glPopMatrix();
        setMat(0.10f,0.30f,0.80f);
        glPushMatrix(); glTranslatef(cx+f*(W*0.44f+0.95f),fY+H+5.0f,cz-D*0.5f+1.5f); drawBox(2.0f,1.10f,0.08f); glPopMatrix();
    }
    gluDeleteQuadric(q);
}

// ============================================================
//  CORAL BAY HOTEL — modern boutique hotel
// ============================================================
static void drawBoutiqueHotel(float cx, float cz)
{
    float gY=-0.50f, W=22.0f, D=15.0f, H=20.0f;
    float fY=gY+0.20f;
    GLUquadric* q=gluNewQuadric();

    // Lobby base
    setMat(0.82f,0.82f,0.84f);
    glPushMatrix(); glTranslatef(cx,fY+3.0f,cz); drawBox(W+2.0f,6.0f,D+2.0f); glPopMatrix();

    // Main tower (glass blue-gray)
    setMat(0.48f,0.62f,0.78f);
    glPushMatrix(); glTranslatef(cx,fY+6.0f+H*0.5f,cz); drawBox(W,H,D); glPopMatrix();

    // Horizontal floor bands
    setMat(0.76f,0.76f,0.78f);
    for(int f=0;f<7;++f){
        float fy=fY+6.0f+H*f/6.0f;
        glPushMatrix(); glTranslatef(cx,fy,cz+D*0.5f+0.06f); drawBox(W+0.6f,0.28f,0.14f); glPopMatrix();
    }

    // Lobby glass front
    setMat(0.40f,0.58f,0.78f);
    glPushMatrix(); glTranslatef(cx,fY+3.0f,cz+D*0.5f+0.12f); drawBox(W,6.0f,0.18f); glPopMatrix();

    // Lobby entrance
    setMat(0.30f,0.50f,0.75f);
    glPushMatrix(); glTranslatef(cx,fY+2.0f,cz+D*0.5f+0.22f); drawBox(4.0f,4.0f,0.10f); glPopMatrix();

    // Rooftop
    setMat(0.30f,0.32f,0.36f);
    glPushMatrix(); glTranslatef(cx,fY+6.0f+H+0.15f,cz); drawBox(W+0.8f,0.30f,D+0.8f); glPopMatrix();
    setMat(0.68f,0.68f,0.70f);
    glPushMatrix(); glTranslatef(cx,fY+6.0f+H+0.72f,cz+D*0.5f+0.40f); drawBox(W+1.0f,0.55f,0.12f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx,fY+6.0f+H+0.72f,cz-D*0.5f-0.40f); drawBox(W+1.0f,0.55f,0.12f); glPopMatrix();
    // Rooftop pool
    setMat(0.16f,0.50f,0.80f);
    glPushMatrix(); glTranslatef(cx,fY+6.0f+H+0.36f,cz); drawBox(8.0f,0.10f,5.5f); glPopMatrix();

    // Sign banner
    {
        float bW=16.0f, bH=2.2f, bY=fY+6.0f+H*0.91f, bZ=cz+D*0.5f+0.20f;
        setMat(0.95f,0.95f,0.95f);
        glPushMatrix(); glTranslatef(cx-bW*0.40f,bY+bH*0.5f+0.22f,bZ); drawBox(0.04f,0.44f,0.04f); glPopMatrix();
        glPushMatrix(); glTranslatef(cx+bW*0.40f,bY+bH*0.5f+0.22f,bZ); drawBox(0.04f,0.44f,0.04f); glPopMatrix();
        setMat(0.06f,0.06f,0.08f);
        glPushMatrix(); glTranslatef(cx,bY,bZ); drawBox(bW,bH,0.10f); glPopMatrix();
        draw3DText("CORAL BAY", cx, bY+0.30f, bZ+0.07f, 1.10f, 0.18f,0.78f,0.96f);
        draw3DText("HOTEL", cx, bY-0.78f, bZ+0.07f, 0.65f, 0.96f,0.96f,0.96f);
    }
    gluDeleteQuadric(q);
}

// ============================================================
//  BLUE WAVE BAR & LOUNGE
// ============================================================
static void drawBlueWaveBar(float cx, float cz)
{
    float gY=-0.50f, W=20.0f, D=14.0f, H=6.5f;
    float fY=gY+0.20f;
    GLUquadric* q=gluNewQuadric();

    // Main structure (warm terracotta)
    setMat(0.76f,0.40f,0.20f);
    glPushMatrix(); glTranslatef(cx,fY+H*0.5f,cz); drawBox(W,H,D); glPopMatrix();

    // Arched windows (5)
    for(int w=0;w<5;++w){
        float wx=cx-W*0.40f+w*W*0.80f/4.0f;
        setMat(0.50f,0.68f,0.88f);
        glPushMatrix(); glTranslatef(wx,fY+H*0.52f,cz+D*0.5f+0.07f); drawBox(2.2f,2.8f,0.14f); glPopMatrix();
        setMat(0.55f,0.72f,0.90f);
        glPushMatrix(); glTranslatef(wx,fY+H*0.52f+1.55f,cz+D*0.5f+0.07f); glScalef(1,0.48f,1); glutSolidSphere(1.12f,10,6); glPopMatrix();
    }

    // Flat roof + parapet
    setMat(0.62f,0.33f,0.16f);
    glPushMatrix(); glTranslatef(cx,fY+H+0.22f,cz); drawBox(W+0.8f,0.44f,D+0.8f); glPopMatrix();
    setMat(0.88f,0.86f,0.84f);
    glPushMatrix(); glTranslatef(cx,fY+H+0.78f,cz+D*0.5f+0.40f); drawBox(W+1.0f,0.52f,0.12f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx,fY+H+0.78f,cz-D*0.5f-0.40f); drawBox(W+1.0f,0.52f,0.12f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx-W*0.5f-0.40f,fY+H+0.78f,cz); drawBox(0.12f,0.52f,D+1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(cx+W*0.5f+0.40f,fY+H+0.78f,cz); drawBox(0.12f,0.52f,D+1.0f); glPopMatrix();

    // Entrance canopy
    setMat(0.14f,0.14f,0.18f);
    glPushMatrix(); glTranslatef(cx,fY+H-0.22f,cz+D*0.5f+2.0f); drawBox(7.0f,0.22f,4.0f); glPopMatrix();
    for(int p=-1;p<=1;p+=2){
        glPushMatrix(); glTranslatef(cx+p*3.0f,fY+H*0.5f,cz+D*0.5f+3.8f); glRotatef(-90,1,0,0);
        gluCylinder(q,0.09f,0.09f,H*0.5f,8,1); glPopMatrix();
    }

    // Sign banner
    {
        float bW=14.0f, bH=2.0f, bY=fY+H+0.02f, bZ=cz+D*0.5f+0.22f;
        setMat(0.95f,0.95f,0.95f);
        glPushMatrix(); glTranslatef(cx-bW*0.40f,bY+bH*0.5f+0.22f,bZ); drawBox(0.04f,0.44f,0.04f); glPopMatrix();
        glPushMatrix(); glTranslatef(cx+bW*0.40f,bY+bH*0.5f+0.22f,bZ); drawBox(0.04f,0.44f,0.04f); glPopMatrix();
        setMat(0.04f,0.06f,0.14f);
        glPushMatrix(); glTranslatef(cx,bY,bZ); drawBox(bW,bH,0.10f); glPopMatrix();
        draw3DText("BLUE WAVE", cx, bY+0.25f, bZ+0.07f, 0.88f, 0.22f,0.72f,1.00f);
        draw3DText("BAR & LOUNGE", cx, bY-0.68f, bZ+0.07f, 0.50f, 0.96f,0.96f,0.96f);
    }
    gluDeleteQuadric(q);
}

// ============================================================
//  SEA PEARL RESORT — fills left end of road
// ============================================================
static void drawSeaPearlResort(float cx, float cz)
{
    float gY=-0.50f, W=28.0f, D=16.0f, H=16.0f;
    float fY=gY+0.25f;
    GLUquadric* q=gluNewQuadric();

    // Ground platform
    setMat(0.86f,0.84f,0.80f);
    glPushMatrix(); glTranslatef(cx,gY+0.12f,cz); drawBox(W+2.5f,0.25f,D+2.5f); glPopMatrix();

    // Main building — warm sandy beige
    setMat(0.90f,0.82f,0.66f);
    glPushMatrix(); glTranslatef(cx,fY+H*0.5f,cz); drawBox(W,H,D); glPopMatrix();

    // Horizontal balcony slabs (every 3 units)
    setMat(0.82f,0.74f,0.58f);
    for(int f=1;f<5;++f){
        float fy=fY+f*H/5.0f;
        glPushMatrix(); glTranslatef(cx,fy,cz+D*0.5f+0.55f); drawBox(W+1.0f,0.22f,1.10f); glPopMatrix();
    }

    // Balcony railings
    setMat(0.96f,0.94f,0.90f);
    for(int f=1;f<5;++f){
        float fy=fY+f*H/5.0f+0.50f;
        glPushMatrix(); glTranslatef(cx,fy,cz+D*0.5f+0.55f); drawBox(W+1.0f,0.60f,0.10f); glPopMatrix();
    }

    // Window grid (4 columns × 4 rows)
    setMat(0.45f,0.65f,0.82f);
    for(int row=0;row<4;++row) for(int col=0;col<4;++col){
        float wx=cx-W*0.36f+col*W*0.72f/3.0f;
        float wy=fY+H*0.18f+row*H*0.72f/3.0f;
        glPushMatrix(); glTranslatef(wx,wy,cz+D*0.5f+0.05f); drawBox(2.2f,2.6f,0.14f); glPopMatrix();
    }

    // Arched lobby entrance
    setMat(0.38f,0.56f,0.78f);
    glPushMatrix(); glTranslatef(cx,fY+2.8f,cz+D*0.5f+0.10f); drawBox(5.0f,5.6f,0.18f); glPopMatrix();
    setMat(0.42f,0.60f,0.80f);
    glPushMatrix(); glTranslatef(cx,fY+5.8f,cz+D*0.5f+0.10f); glScalef(1,0.5f,1); glutSolidSphere(2.52f,14,8); glPopMatrix();

    // Pointed roof / gable
    setMat(0.55f,0.35f,0.18f);
    glBegin(GL_TRIANGLES); glNormal3f(0,1,0.4f);
    glVertex3f(cx-W*0.5f,fY+H,      cz+D*0.5f+0.1f);
    glVertex3f(cx+W*0.5f,fY+H,      cz+D*0.5f+0.1f);
    glVertex3f(cx,        fY+H+5.0f, cz+D*0.5f+0.1f);
    // back slope
    glVertex3f(cx-W*0.5f,fY+H,      cz-D*0.5f-0.1f);
    glVertex3f(cx+W*0.5f,fY+H,      cz-D*0.5f-0.1f);
    glVertex3f(cx,        fY+H+5.0f, cz-D*0.5f-0.1f);
    glEnd();
    // Roof ridge beam
    setMat(0.42f,0.26f,0.12f);
    glPushMatrix(); glTranslatef(cx,fY+H+5.0f,cz); drawBox(0.30f,0.30f,D+0.4f); glPopMatrix();
    // Side triangles of gable
    setMat(0.55f,0.35f,0.18f);
    for(int side=-1;side<=1;side+=2){
        glBegin(GL_TRIANGLES); glNormal3f(side,0,0);
        glVertex3f(cx+side*W*0.5f,fY+H,     cz-D*0.5f);
        glVertex3f(cx+side*W*0.5f,fY+H,     cz+D*0.5f);
        glVertex3f(cx+side*W*0.5f,fY+H+5.0f,cz);
        glEnd();
    }

    // Sign banner (BBQ style)
    {
        float bW=20.0f, bH=2.4f, bY=fY+H+1.2f, bZ=cz+D*0.5f+0.52f;
        setMat(0.95f,0.95f,0.95f);
        glPushMatrix(); glTranslatef(cx-bW*0.40f,bY+bH*0.5f+0.26f,bZ); drawBox(0.04f,0.52f,0.04f); glPopMatrix();
        glPushMatrix(); glTranslatef(cx+bW*0.40f,bY+bH*0.5f+0.26f,bZ); drawBox(0.04f,0.52f,0.04f); glPopMatrix();
        setMat(0.06f,0.08f,0.14f);
        glPushMatrix(); glTranslatef(cx,bY,bZ); drawBox(bW,bH,0.10f); glPopMatrix();
        draw3DText("SEA PEARL", cx, bY+0.38f, bZ+0.07f, 1.30f, 0.86f,0.92f,1.00f);
        draw3DText("RESORT", cx, bY-0.75f, bZ+0.07f, 0.68f, 0.96f,0.96f,0.96f);
    }

    // Garden trees (2 either side of entrance)
    for(int t=-1;t<=1;t+=2){
        float tx=cx+t*4.5f, tz=cz+D*0.5f+2.5f;
        setMat(0.32f,0.20f,0.09f);
        glPushMatrix(); glTranslatef(tx,fY+1.0f,tz); glRotatef(-90,1,0,0); gluCylinder(q,0.18f,0.12f,2.0f,8,1); glPopMatrix();
        setMat(0.18f,0.55f,0.22f);
        glPushMatrix(); glTranslatef(tx,fY+3.5f,tz); glutSolidSphere(1.10f,12,10); glPopMatrix();
    }

    gluDeleteQuadric(q);
}

// ============================================================
//  SEAVIEW RESORT (curved building with dome)
// ============================================================
static void drawSeaviewResort()
{
    // The hotel sits on the LAND side (negative Z), centered around X=0
    float cx = 0.0f;
    float cz = -55.0f;
    float baseY = -0.5f;
    float floorH = 1.55f;       // taller floors
    int   floors = 13;          // more floors
    float radius = 11.5f;       // bigger arc radius
    float arcStart = -100.0f;   // degrees
    float arcSpan  =  210.0f;   // slightly wider arc

    glPushMatrix();
    glTranslatef(cx, baseY, cz);

    // base/podium - WHITE
    setMat(0.97f, 0.98f, 0.99f);
    glPushMatrix();
    glTranslatef(0, 0.5f, 5.0f);
    drawBox(24.0f, 1.0f, 8.5f);
    glPopMatrix();

    // entrance canopy - WHITE
    setMat(0.95f, 0.96f, 0.97f);
    glPushMatrix();
    glTranslatef(0, 1.2f, 10.0f);
    drawBox(10.0f, 0.35f, 2.5f);
    glPopMatrix();

    // signage strip on canopy - WHITE
    setMat(0.92f, 0.94f, 0.95f);
    glPushMatrix();
    glTranslatef(0, 2.0f, 10.0f);
    drawBox(9.0f, 0.65f, 0.22f);
    glPopMatrix();

    // CURVED MAIN TOWER - a partial cylinder of glass-fronted floors
    int segs = 28;
    for (int f = 0; f < floors; ++f) {
        float y = 0.8f + f * floorH;
        // alternate: wall band then glass band per floor (banded look)
        // wall ring - PURE WHITE (no texture)
        glDisable(GL_TEXTURE_2D);
        setMat(0.97f, 0.98f, 0.99f);
        glBegin(GL_QUAD_STRIP);
        for (int s = 0; s <= segs; ++s) {
            float t = s / (float)segs;
            float ang = (arcStart + t * arcSpan) * PI / 180.0f;
            float xr = cosf(ang) * radius;
            float zr = sinf(ang) * radius;
            glNormal3f(cosf(ang), 0, sinf(ang));
            glVertex3f(xr, y, zr);
            glVertex3f(xr, y + 0.20f, zr);
        }
        glEnd();

        // glass band - WHITE with subtle grey window lines
        glDisable(GL_TEXTURE_2D);
        if (night) {
            GLfloat em[4] = { 0.7f, 0.55f, 0.25f, 1.0f };
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
            setMat(0.95f, 0.85f, 0.45f);
        } else {
            setMat(0.88f, 0.90f, 0.92f);
        }
        glBegin(GL_QUAD_STRIP);
        for (int s = 0; s <= segs; ++s) {
            float t = s / (float)segs;
            float ang = (arcStart + t * arcSpan) * PI / 180.0f;
            float xr = cosf(ang) * (radius + 0.05f);
            float zr = sinf(ang) * (radius + 0.05f);
            glNormal3f(cosf(ang), 0, sinf(ang));
            glVertex3f(xr, y + 0.20f, zr);
            glVertex3f(xr, y + 1.20f, zr);
        }
        glEnd();

        // ---- OPEN BALCONY for this floor ----
        float balconyExt = 0.95f;          // how far the balcony extends outward
        float railH      = 0.55f;          // railing height
        float r_in  = radius + 0.05f;
        float r_out = radius + balconyExt;

        // balcony slab (top surface)
        setMat(0.96f, 0.97f, 0.97f);
        glBegin(GL_QUAD_STRIP);
        for (int s = 0; s <= segs; ++s) {
            float t = s / (float)segs;
            float ang = (arcStart + t * arcSpan) * PI / 180.0f;
            glNormal3f(0, 1, 0);
            glVertex3f(cosf(ang) * r_in,  y + 0.22f, sinf(ang) * r_in);
            glVertex3f(cosf(ang) * r_out, y + 0.22f, sinf(ang) * r_out);
        }
        glEnd();
        // balcony underside
        setMat(0.88f, 0.90f, 0.92f);
        glBegin(GL_QUAD_STRIP);
        for (int s = 0; s <= segs; ++s) {
            float t = s / (float)segs;
            float ang = (arcStart + t * arcSpan) * PI / 180.0f;
            glNormal3f(0, -1, 0);
            glVertex3f(cosf(ang) * r_out, y + 0.10f, sinf(ang) * r_out);
            glVertex3f(cosf(ang) * r_in,  y + 0.10f, sinf(ang) * r_in);
        }
        glEnd();
        // balcony outer edge (the rim under the railing)
        setMat(0.92f, 0.93f, 0.95f);
        glBegin(GL_QUAD_STRIP);
        for (int s = 0; s <= segs; ++s) {
            float t = s / (float)segs;
            float ang = (arcStart + t * arcSpan) * PI / 180.0f;
            glNormal3f(cosf(ang), 0, sinf(ang));
            glVertex3f(cosf(ang) * r_out, y + 0.10f, sinf(ang) * r_out);
            glVertex3f(cosf(ang) * r_out, y + 0.22f, sinf(ang) * r_out);
        }
        glEnd();

        // RAILING - top rail (horizontal cap)
        setMat(0.55f, 0.65f, 0.78f);
        glBegin(GL_QUAD_STRIP);
        for (int s = 0; s <= segs; ++s) {
            float t = s / (float)segs;
            float ang = (arcStart + t * arcSpan) * PI / 180.0f;
            glNormal3f(0, 1, 0);
            glVertex3f(cosf(ang) * (r_out - 0.05f), y + 0.22f + railH, sinf(ang) * (r_out - 0.05f));
            glVertex3f(cosf(ang) * (r_out + 0.05f), y + 0.22f + railH, sinf(ang) * (r_out + 0.05f));
        }
        glEnd();

        // vertical balusters along railing
        setMat(0.85f, 0.88f, 0.92f);
        int balusterStep = 2;
        for (int s = 0; s <= segs; s += balusterStep) {
            float t = s / (float)segs;
            float ang = (arcStart + t * arcSpan) * PI / 180.0f;
            glPushMatrix();
            glTranslatef(cosf(ang) * r_out, y + 0.22f + railH * 0.5f, sinf(ang) * r_out);
            drawBox(0.05f, railH, 0.05f);
            glPopMatrix();
        }
        GLfloat zero[4] = { 0, 0, 0, 1 };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
        glDisable(GL_TEXTURE_2D);
    }

    // BACK wall (closes the arc on land side) - PURE WHITE
    glDisable(GL_TEXTURE_2D);
    setMat(0.97f, 0.98f, 0.99f);
    float topY = 0.8f + floors * floorH;
    float a0 = arcStart * PI / 180.0f;
    float a1 = (arcStart + arcSpan) * PI / 180.0f;
    glBegin(GL_QUADS);
    glNormal3f(0, 0, -1);
    glVertex3f(cosf(a0) * radius, 0.8f, sinf(a0) * radius);
    glVertex3f(cosf(a1) * radius, 0.8f, sinf(a1) * radius);
    glVertex3f(cosf(a1) * radius, topY,  sinf(a1) * radius);
    glVertex3f(cosf(a0) * radius, topY,  sinf(a0) * radius);
    glEnd();

    // ROOF TOP DISK - white
    setMat(0.96f, 0.97f, 0.98f);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(0, topY + 0.05f, 0);
    for (int s = 0; s <= segs; ++s) {
        float t = s / (float)segs;
        float ang = (arcStart + t * arcSpan) * PI / 180.0f;
        glVertex3f(cosf(ang) * radius, topY + 0.05f, sinf(ang) * radius);
    }
    glEnd();

    // GREEN DOME on the front-center top (bigger)
    glPushMatrix();
    glTranslatef(0, topY + 0.05f, -2.5f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texDome);
    glColor3f(1, 1, 1);
    setMat(1, 1, 1);
    GLUquadric *q = gluNewQuadric();
    gluQuadricTexture(q, GL_TRUE);
    gluQuadricNormals(q, GLU_SMOOTH);
    glPushMatrix();
    glScalef(2.8f, 2.0f, 2.8f);
    gluSphere(q, 1.0, 24, 16);
    glPopMatrix();
    gluDeleteQuadric(q);
    glDisable(GL_TEXTURE_2D);
    // dome cross / antenna - taller
    setMat(0.85f, 0.85f, 0.85f);
    glPushMatrix();
    glTranslatef(0, 2.6f, 0);
    drawBox(0.08f, 2.0f, 0.08f);
    glPopMatrix();
    glPopMatrix();

    // "BAYWATCH" name on top of the resort - facing FRONT (toward camera/sea)
    // White sign panel (glows at night)
    float signZ = 6.0f;     // front of the roof so visible from camera
    if (night || sunset) {
        GLfloat em[4] = { 0.95f, 0.95f, 0.95f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
    }
    setMat(0.97f, 0.98f, 0.99f);
    glPushMatrix();
    glTranslatef(0, topY + 2.5f, signZ);
    drawBox(11.0f, 2.6f, 0.35f);
    glPopMatrix();
    GLfloat zero[4] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);

    // "Baywatch" text on the sign - bright blue, facing forward
    draw3DText("Baywatch",
               0.0f, topY + 2.5f, signZ + 0.20f,
               1.40f,
               0.10f, 0.45f, 0.85f);

    // 2 small support posts under the sign
    setMat(0.85f, 0.88f, 0.90f);
    glPushMatrix();
    glTranslatef(-4.5f, topY + 1.3f, signZ);
    drawBox(0.25f, 1.5f, 0.25f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef( 4.5f, topY + 1.3f, signZ);
    drawBox(0.25f, 1.5f, 0.25f);
    glPopMatrix();

    glPopMatrix();
}

// ============================================================
//  ROW OF SMALLER BEACHFRONT HOTELS
// ============================================================
static void drawHotel(float x, float z, float w, float h, float d,
                      float r, float g, float b, bool wide_glass = true)
{
    glPushMatrix();
    glTranslatef(x, -0.5f + h * 0.5f, z);

    // wall front
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texWall);
    glColor3f(1, 1, 1);
    setMat(r, g, b);
    drawBox(w, h, d);
    glDisable(GL_TEXTURE_2D);

    // glass band on front (sea-facing side, +Z)
    int floors = (int)(h / 1.2f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texGlass);
    if (night) {
        GLfloat em[4] = { 0.85f, 0.7f, 0.3f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
        setMat(0.95f, 0.85f, 0.45f);
    } else {
        setMat(1, 1, 1);
    }
    for (int f = 1; f < floors; ++f) {
        float yy = -h * 0.5f + f * 1.2f;
        glBegin(GL_QUADS);
        glNormal3f(0, 0, 1);
        glTexCoord2f(0, 0); glVertex3f(-w * 0.45f, yy,        d * 0.5f + 0.02f);
        glTexCoord2f(4, 0); glVertex3f( w * 0.45f, yy,        d * 0.5f + 0.02f);
        glTexCoord2f(4, 1); glVertex3f( w * 0.45f, yy + 0.8f, d * 0.5f + 0.02f);
        glTexCoord2f(0, 1); glVertex3f(-w * 0.45f, yy + 0.8f, d * 0.5f + 0.02f);
        glEnd();
    }
    GLfloat zero[4] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
    glDisable(GL_TEXTURE_2D);

    // small parapet/sign on top
    setMat(r * 0.5f, g * 0.5f, b * 0.5f);
    glPushMatrix();
    glTranslatef(0, h * 0.5f + 0.4f, d * 0.5f - 0.2f);
    drawBox(w * 0.6f, 0.8f, 0.2f);
    glPopMatrix();

    glPopMatrix();
    (void)wide_glass;
}

// ============================================================
//  FOREST BELT - trees between resort/buildings and far hills
// ============================================================
static void drawForestTree(float x, float z, float scale, float darkness, int variant)
{
    // make sure no texture/state from previous draws is bleeding
    glDisable(GL_TEXTURE_2D);

    // moderate trunk
    float trunkH = 1.8f * scale;
    glColor3f(0.30f * darkness, 0.18f * darkness, 0.08f * darkness);
    setMat(0.30f * darkness, 0.18f * darkness, 0.08f * darkness);
    glPushMatrix();
    glTranslatef(x, -0.5f, z);
    glRotatef(-90, 1, 0, 0);
    GLUquadric *q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    gluCylinder(q, 0.25f * scale, 0.18f * scale, trunkH, 8, 2);
    gluDeleteQuadric(q);
    glPopMatrix();

    // canopy - VIVID GREEN, varied per tree (4 forest tones)
    float baseR, baseG, baseB;
    switch (variant % 4) {
        case 0: baseR = 0.08f; baseG = 0.40f; baseB = 0.10f; break;  // deep green
        case 1: baseR = 0.14f; baseG = 0.48f; baseB = 0.14f; break;  // medium green
        case 2: baseR = 0.22f; baseG = 0.52f; baseB = 0.12f; break;  // yellow-green
        default:baseR = 0.10f; baseG = 0.45f; baseB = 0.22f; break;  // blue-green
    }
    baseR *= darkness;
    baseG *= darkness;
    baseB *= darkness;

    // only light haze on close trees, more on distant
    float depthT = (-z - 80.0f) / 280.0f;
    if (depthT < 0) depthT = 0;
    if (depthT > 1) depthT = 1;
    float skyR = 0.65f, skyG = 0.78f, skyB = 0.85f;
    float haze = depthT * 0.20f;
    float r = baseR * (1 - haze) + skyR * haze;
    float g = baseG * (1 - haze) + skyG * haze;
    float b = baseB * (1 - haze) + skyB * haze;
    glColor3f(r, g, b);                  // also use glColor (works with COLOR_MATERIAL)
    setMat(r, g, b);

    // BIG layered canopy on top of tall trunk
    glPushMatrix();
    glTranslatef(x, -0.5f + trunkH + 0.5f * scale, z);

    glPushMatrix();
    glScalef(1.8f * scale, 1.7f * scale, 1.8f * scale);
    glutSolidSphere(1.0, 12, 10);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.9f * scale, 0.4f * scale, 0.3f * scale);
    glScalef(1.2f * scale, 1.1f * scale, 1.2f * scale);
    glutSolidSphere(1.0, 10, 8);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.8f * scale, 0.3f * scale, -0.4f * scale);
    glScalef(1.2f * scale, 1.1f * scale, 1.2f * scale);
    glutSolidSphere(1.0, 10, 8);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.1f * scale, -0.3f * scale, 0.9f * scale);
    glScalef(1.0f * scale, 0.95f * scale, 1.0f * scale);
    glutSolidSphere(1.0, 10, 8);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.3f * scale, 0.7f * scale, -0.1f * scale);
    glScalef(0.9f * scale, 0.85f * scale, 0.9f * scale);
    glutSolidSphere(1.0, 8, 8);
    glPopMatrix();

    glPopMatrix();

    // reset color for safety
    glColor3f(1, 1, 1);
}

static void drawForestFloor()
{
    // big GREEN GRASS ground plane covering the forest belt area
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texGrass);
    glColor3f(1, 1, 1);
    setMat(1, 1, 1);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0,   0);  glVertex3f(-700.0f, -0.43f,  -65.0f);
    glTexCoord2f(100, 0);  glVertex3f( 700.0f, -0.43f,  -65.0f);
    glTexCoord2f(100, 40); glVertex3f( 700.0f, -0.43f, -400.0f);
    glTexCoord2f(0,   40); glVertex3f(-700.0f, -0.43f, -400.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

// tiny bush/sapling - just a small canopy without big trunk
static void drawSmallBush(float x, float z, float size, int variant)
{
    glDisable(GL_TEXTURE_2D);
    float baseR, baseG, baseB;
    switch (variant % 4) {
        case 0: baseR = 0.10f; baseG = 0.45f; baseB = 0.12f; break;
        case 1: baseR = 0.16f; baseG = 0.55f; baseB = 0.16f; break;
        case 2: baseR = 0.25f; baseG = 0.58f; baseB = 0.14f; break;
        default:baseR = 0.12f; baseG = 0.50f; baseB = 0.22f; break;
    }
    glColor3f(baseR, baseG, baseB);
    setMat(baseR, baseG, baseB);
    glPushMatrix();
    glTranslatef(x, -0.45f + size * 0.4f, z);
    glScalef(size, size * 0.7f, size);
    glutSolidSphere(1.0, 8, 6);
    glPopMatrix();
    glColor3f(1, 1, 1);
}

static void drawForestBelt()
{
    srand(77);

    // ---- Layer 1: medium trees right behind back-row buildings ----
    for (int i = 0; i < 280; ++i) {
        float fx = frand(-500.0f, 500.0f);
        float fz = frand(-90.0f,  -73.0f);
        if (fx > -38.0f && fx < 20.0f) continue;       // resort gap
        if (fx > 125.0f && fx < 158.0f) continue;       // BBQ lounge gap
        float scale    = frand(1.3f, 2.0f);
        float darkness = frand(0.85f, 1.10f);
        int   variant  = rand() % 4;
        drawForestTree(fx, fz, scale, darkness, variant);
    }

    // ---- Layer 2: forest carpet ----
    for (int i = 0; i < 750; ++i) {
        float fx = frand(-500.0f, 500.0f);
        float fz = frand(-220.0f, -90.0f);
        float scale    = frand(1.0f, 1.7f);
        float darkness = frand(0.85f, 1.15f);
        int   variant  = rand() % 4;
        drawForestTree(fx, fz, scale, darkness, variant);
    }

    // ---- Layer 3: small trees in mid-distance ----
    for (int i = 0; i < 380; ++i) {
        float fx = frand(-550.0f, 550.0f);
        float fz = frand(-370.0f, -220.0f);
        float scale    = frand(0.8f, 1.4f);
        float darkness = frand(0.80f, 1.05f);
        int   variant  = rand() % 4;
        drawForestTree(fx, fz, scale, darkness, variant);
    }

    // ---- Layer 4: TINY BUSHES scattered across the green floor ----
    for (int i = 0; i < 900; ++i) {
        float fx = frand(-550.0f, 550.0f);
        float fz = frand(-380.0f, -70.0f);
        // skip resort grounds
        if (fx > -38.0f && fx < 20.0f && fz > -85.0f) continue;
        // skip BBQ lounge area (around x=140, z=-58)
        if (fx > 125.0f && fx < 158.0f && fz > -78.0f && fz < -42.0f) continue;
        float size    = frand(0.3f, 0.8f);
        int   variant = rand() % 4;
        drawSmallBush(fx, fz, size, variant);
    }
}

// Small 1-2 story shop with a colored signboard above
static void drawShop(float x, float z, float w, float h, float d,
                     float wallR, float wallG, float wallB,
                     float signR, float signG, float signB)
{
    glPushMatrix();
    glTranslatef(x, -0.5f + h * 0.5f, z);

    // wall
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texWall);
    glColor3f(1, 1, 1);
    setMat(wallR, wallG, wallB);
    drawBox(w, h, d);
    glDisable(GL_TEXTURE_2D);

    // big front shop window (glass on sea-facing side)
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texGlass);
    if (night) {
        GLfloat em[4] = { 0.9f, 0.75f, 0.35f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
        setMat(0.95f, 0.85f, 0.45f);
    } else setMat(1, 1, 1);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(-w * 0.40f, -h * 0.40f, d * 0.5f + 0.02f);
    glTexCoord2f(3, 0); glVertex3f( w * 0.40f, -h * 0.40f, d * 0.5f + 0.02f);
    glTexCoord2f(3, 1); glVertex3f( w * 0.40f,  h * 0.20f, d * 0.5f + 0.02f);
    glTexCoord2f(0, 1); glVertex3f(-w * 0.40f,  h * 0.20f, d * 0.5f + 0.02f);
    glEnd();
    GLfloat zero[4] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
    glDisable(GL_TEXTURE_2D);

    // signboard above shop window
    if (night || sunset) {
        GLfloat em[4] = { signR * 0.6f, signG * 0.6f, signB * 0.6f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
    }
    setMat(signR, signG, signB);
    glPushMatrix();
    glTranslatef(0, h * 0.32f, d * 0.5f + 0.08f);
    drawBox(w * 0.85f, h * 0.18f, 0.12f);
    glPopMatrix();
    GLfloat zero2[4] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero2);

    // awning (small overhanging roof)
    setMat(signR * 0.7f, signG * 0.7f, signB * 0.7f);
    glPushMatrix();
    glTranslatef(0, h * 0.20f, d * 0.5f + 0.4f);
    drawBox(w * 0.95f, 0.1f, 0.6f);
    glPopMatrix();

    glPopMatrix();
}

static void drawHotelRow()
{
    // ============ RIGHT side of Seaview - front-row hotels REMOVED ============
    // (last 3 right-side hotels removed - replaced with beach resort)

    // ============ LEFT side of Seaview - ALL REMOVED per user request ============

    // ============ back-row buildings (depth, second layer) ============
    // (left-side back-row buildings - ALL REMOVED)
    // (right-side back-row hotels REMOVED)

    // ============ small SHOPS along the road in between hotels ============
    // (RIGHT side shops REMOVED)
    // (left-side shops - ALL REMOVED)
}

// ============================================================
//  POOL + GARDEN + FLOWERS
// ============================================================
static void drawPool()
{
    // pool sits in front of the Seaview Resort, between resort and road
    // Actually we'll place it on the side / behind the resort for a garden effect
    // Place it to the LEFT side of Seaview (negative X), forming the resort grounds
    float cx = -14.0f;
    float cz = -45.0f;
    float baseY = -0.40f;
    int segs = 32;

    // garden grass patch under pool (bigger)
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texGrass);
    glColor3f(1, 1, 1);
    setMat(1, 1, 1);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0); glVertex3f(cx - 22, baseY + 0.005f, cz - 14);
    glTexCoord2f(8, 0); glVertex3f(cx +  6, baseY + 0.005f, cz - 14);
    glTexCoord2f(8, 6); glVertex3f(cx +  6, baseY + 0.005f, cz + 14);
    glTexCoord2f(0, 6); glVertex3f(cx - 22, baseY + 0.005f, cz + 14);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // pool deck (BIGGER lighter sand area around pool)
    setMat(0.95f, 0.92f, 0.85f);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(cx, baseY + 0.02f, cz);
    for (int s = 0; s <= segs; ++s) {
        float t = s / (float)segs;
        float a = t * 2 * PI;
        glVertex3f(cx + cosf(a) * 11.0f, baseY + 0.02f, cz + sinf(a) * 8.5f);
    }
    glEnd();

    // pool water (BIGGER blue pool)
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texPool);
    glColor3f(1, 1, 1);
    setMat(1, 1, 1);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0.5f, 0.5f); glVertex3f(cx, baseY + 0.025f, cz);
    for (int s = 0; s <= segs; ++s) {
        float t = s / (float)segs;
        float a = t * 2 * PI;
        float u = 0.5f + cosf(a) * 0.5f;
        float v = 0.5f + sinf(a) * 0.5f;
        glTexCoord2f(u, v);
        glVertex3f(cx + cosf(a) * 8.5f, baseY + 0.025f, cz + sinf(a) * 6.5f);
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // bridge across pool (longer to match bigger pool)
    setMat(0.55f, 0.35f, 0.20f);
    glPushMatrix();
    glTranslatef(cx, baseY + 0.30f, cz);
    drawBox(18.0f, 0.10f, 0.8f);
    glPopMatrix();

    // 4 pool-side lounge chairs (white) around the pool
    setMat(0.95f, 0.95f, 0.95f);
    for (int s = 0; s < 4; ++s) {
        float a = s * (PI * 0.5f) + PI * 0.25f;
        float px = cx + cosf(a) * 9.5f;
        float pz = cz + sinf(a) * 7.5f;
        glPushMatrix();
        glTranslatef(px, baseY + 0.10f, pz);
        glRotatef(a * 180.0f / PI + 90.0f, 0, 1, 0);
        drawBox(1.6f, 0.10f, 0.6f);
        glPopMatrix();
    }
}

static void drawGardenFlowers()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texFlower);
    glColor3f(1, 1, 1);
    setMat(1, 1, 1);
    srand(42);
    // pool-side garden (left near seaview)
    for (int i = 0; i < 50; ++i) {
        float x = frand(-30, -8);
        float z = frand(-55, -30);
        if (x > -19 && x < -8 && z > -50 && z < -40) continue;   // skip pool area
        glPushMatrix();
        glTranslatef(x, -0.4f, z);
        glutSolidSphere(0.25, 8, 8);
        glPopMatrix();
    }
    // EXTENDED left-side garden strip in front of new buildings (full length)
    for (int i = 0; i < 110; ++i) {
        float x = frand(-160, -30);
        float z = frand(-55, -42);
        glPushMatrix();
        glTranslatef(x, -0.4f, z);
        glutSolidSphere(0.22, 8, 8);
        glPopMatrix();
    }
    // right side garden flowers (full length)
    for (int i = 0; i < 110; ++i) {
        float x = frand(8, 160);
        float z = frand(-55, -42);
        glPushMatrix();
        glTranslatef(x, -0.4f, z);
        glutSolidSphere(0.22, 8, 8);
        glPopMatrix();
    }
    glDisable(GL_TEXTURE_2D);

    // low concrete barriers (along roadside) - cream/light color
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texLightDirt);
    setMat(1, 1, 1);
    for (int i = 0; i < 22; ++i) {
        float x = -170 + i * 16.0f;
        glPushMatrix();
        glTranslatef(x, -0.25f, -17.5f);
        drawBox(2.0f, 0.4f, 0.4f);
        glPopMatrix();
    }
    glDisable(GL_TEXTURE_2D);
}

// ============================================================
//  VEHICLES
// ============================================================
static void drawWheel(float x, float y, float z)
{
    setMat(0.05f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(90, 0, 1, 0);
    GLUquadric *q = gluNewQuadric();
    gluDisk(q, 0, 0.30, 14, 1);
    gluCylinder(q, 0.30, 0.30, 0.15, 14, 1);
    glTranslatef(0, 0, 0.15f);
    gluDisk(q, 0, 0.30, 14, 1);
    gluDeleteQuadric(q);
    glPopMatrix();
}

static void drawCar(float x, float r, float g, float b, bool faceLeft)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, faceLeft ? -4.0f : -12.0f);
    if (faceLeft) glRotatef(180, 0, 1, 0);

    setMat(r, g, b);
    glPushMatrix(); glTranslatef(0, 0.10f, 0); drawBox(2.4f, 0.50f, 1.10f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.1f, 0.55f, 0); drawBox(1.4f, 0.50f, 1.00f); glPopMatrix();

    // windows
    setMat(0.25f, 0.30f, 0.35f);
    glPushMatrix(); glTranslatef(-0.1f, 0.60f, 0.51f); drawBox(1.30f, 0.35f, 0.02f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.1f, 0.60f,-0.51f); drawBox(1.30f, 0.35f, 0.02f); glPopMatrix();

    // headlights
    if (night || sunset) {
        GLfloat em[4] = { 1, 0.95f, 0.7f, 1 };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
    }
    setMat(1, 0.95f, 0.7f);
    glPushMatrix(); glTranslatef(1.21f, 0.18f, 0.35f); drawBox(0.04f, 0.14f, 0.18f); glPopMatrix();
    glPushMatrix(); glTranslatef(1.21f, 0.18f,-0.35f); drawBox(0.04f, 0.14f, 0.18f); glPopMatrix();
    GLfloat zero[4] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);

    drawWheel( 0.8f, -0.20f,  0.55f);
    drawWheel(-0.8f, -0.20f,  0.55f);
    drawWheel( 0.8f, -0.20f, -0.55f);
    drawWheel(-0.8f, -0.20f, -0.55f);

    glPopMatrix();
}

static void drawBus(float x, float r, float g, float b, bool faceLeft)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, faceLeft ? -4.0f : -12.0f);
    if (faceLeft) glRotatef(180, 0, 1, 0);

    setMat(r, g, b);
    glPushMatrix(); glTranslatef(0, 0.60f, 0); drawBox(4.5f, 1.50f, 1.30f); glPopMatrix();

    setMat(0.30f, 0.35f, 0.45f);
    for (int i = 0; i < 5; ++i) {
        glPushMatrix(); glTranslatef(-1.8f + i * 0.9f, 0.90f,  0.66f); drawBox(0.7f, 0.55f, 0.02f); glPopMatrix();
        glPushMatrix(); glTranslatef(-1.8f + i * 0.9f, 0.90f, -0.66f); drawBox(0.7f, 0.55f, 0.02f); glPopMatrix();
    }

    if (night || sunset) {
        GLfloat em[4] = { 1, 0.95f, 0.7f, 1 };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
    }
    setMat(1, 0.95f, 0.7f);
    glPushMatrix(); glTranslatef(2.26f, 0.40f, 0.40f); drawBox(0.04f, 0.18f, 0.22f); glPopMatrix();
    glPushMatrix(); glTranslatef(2.26f, 0.40f,-0.40f); drawBox(0.04f, 0.18f, 0.22f); glPopMatrix();
    GLfloat zero[4] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);

    drawWheel( 1.5f, -0.20f,  0.65f);
    drawWheel(-1.5f, -0.20f,  0.65f);
    drawWheel( 1.5f, -0.20f, -0.65f);
    drawWheel(-1.5f, -0.20f, -0.65f);

    glPopMatrix();
}

// ============================================================
//  BOATS
// ============================================================
static void drawBoat(float x, float z)
{
    setMat(0.95f, 0.95f, 0.95f);
    glPushMatrix();
    glTranslatef(x, -0.35f, z);
    glScalef(2.5f, 0.5f, 0.9f);
    glutSolidCube(1.0);
    glPopMatrix();

    setMat(0.10f, 0.45f, 0.75f);
    glPushMatrix();
    glTranslatef(x, 0.10f, z);
    glScalef(1.8f, 0.3f, 0.5f);
    glutSolidCube(1.0);
    glPopMatrix();

    // mast
    setMat(0.5f, 0.3f, 0.15f);
    glPushMatrix();
    glTranslatef(x, 1.0f, z);
    drawBox(0.06f, 1.5f, 0.06f);
    glPopMatrix();
}

// ============================================================
//  FISHING BOAT — detailed wooden fishing trawler
// ============================================================
static void drawFishingBoat(float x, float z, float yRot=0.0f)
{
    float wY=-0.30f; // water surface level
    GLUquadric* q=gluNewQuadric();
    glPushMatrix(); glTranslatef(x,wY,z); glRotatef(yRot,0,1,0);

    // Hull — deep blue, tapered bow
    setMat(0.12f,0.28f,0.62f);
    glPushMatrix(); glTranslatef(0,0,0); drawBox(5.5f,0.80f,2.0f); glPopMatrix();
    // Hull sides (slightly raised)
    setMat(0.10f,0.22f,0.55f);
    glPushMatrix(); glTranslatef(0,0.50f,0); drawBox(5.5f,0.30f,2.0f); glPopMatrix();
    // Bow taper (pointed front)
    setMat(0.12f,0.28f,0.62f);
    glBegin(GL_TRIANGLES); glNormal3f(0,0,1);
    glVertex3f( 2.75f, 0.20f, -1.0f); glVertex3f( 2.75f, 0.20f,  1.0f); glVertex3f( 4.0f,  0.20f, 0.0f);
    glVertex3f( 2.75f,-0.40f, -1.0f); glVertex3f( 2.75f,-0.40f,  1.0f); glVertex3f( 4.0f, -0.40f, 0.0f);
    glEnd();
    // White hull stripe
    setMat(0.95f,0.95f,0.95f);
    glPushMatrix(); glTranslatef(0,0.68f,0); drawBox(5.6f,0.12f,2.05f); glPopMatrix();

    // Deck
    setMat(0.55f,0.38f,0.18f);
    glPushMatrix(); glTranslatef(0,0.72f,0); drawBox(5.2f,0.12f,1.80f); glPopMatrix();

    // Cabin (wheelhouse)
    setMat(0.88f,0.85f,0.80f);
    glPushMatrix(); glTranslatef(0.50f,1.30f,0); drawBox(2.2f,1.20f,1.50f); glPopMatrix();
    // Cabin windows
    setMat(0.55f,0.75f,0.90f);
    glPushMatrix(); glTranslatef(1.62f,1.38f,0); drawBox(0.10f,0.55f,1.10f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.50f,1.38f, 0.76f); drawBox(1.60f,0.55f,0.10f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.50f,1.38f,-0.76f); drawBox(1.60f,0.55f,0.10f); glPopMatrix();
    // Cabin roof
    setMat(0.70f,0.20f,0.15f);
    glPushMatrix(); glTranslatef(0.50f,1.98f,0); drawBox(2.4f,0.15f,1.65f); glPopMatrix();

    // Mast
    setMat(0.40f,0.26f,0.10f);
    glPushMatrix(); glTranslatef(0.50f,2.12f,0); glRotatef(-90,1,0,0);
    gluCylinder(q,0.07f,0.05f,3.5f,8,1); glPopMatrix();
    // Crow-nest
    setMat(0.50f,0.34f,0.14f);
    glPushMatrix(); glTranslatef(0.50f,5.50f,0); drawBox(0.50f,0.30f,0.50f); glPopMatrix();
    // Antenna on top
    setMat(0.70f,0.70f,0.72f);
    glPushMatrix(); glTranslatef(0.50f,5.65f,0); glRotatef(-90,1,0,0);
    gluCylinder(q,0.03f,0.01f,1.2f,6,1); glPopMatrix();

    // Fishing boom (side arm)
    setMat(0.35f,0.22f,0.08f);
    glPushMatrix(); glTranslatef(-1.0f,2.20f,0); glRotatef(30,0,0,1);
    drawBox(0.08f,0.08f,3.0f); glPopMatrix();

    // Net pile on deck (back)
    setMat(0.62f,0.55f,0.30f);
    glPushMatrix(); glTranslatef(-1.6f,0.85f,0); glScalef(1,0.35f,1); glutSolidSphere(0.70f,10,6); glPopMatrix();

    // Life buoy
    setMat(0.95f,0.25f,0.12f);
    glPushMatrix(); glTranslatef(1.80f,1.20f,0.90f); glRotatef(90,1,0,0);
    gluCylinder(q,0.25f,0.25f,0.12f,12,1);
    gluDisk(q,0.12f,0.25f,12,1); glPopMatrix();

    glPopMatrix();
    gluDeleteQuadric(q);
}

// ============================================================
//  LARGE SHIP — passenger / cargo vessel
// ============================================================
static void drawShip(float x, float z, float yRot=0.0f)
{
    float wY=-0.50f;
    GLUquadric* q=gluNewQuadric();
    glPushMatrix(); glTranslatef(x,wY,z); glRotatef(yRot,0,1,0);

    // Hull — dark navy
    setMat(0.08f,0.10f,0.22f);
    glPushMatrix(); glTranslatef(0,0.80f,0); drawBox(24.0f,1.60f,6.0f); glPopMatrix();
    // Hull bottom (below waterline — black)
    setMat(0.10f,0.10f,0.10f);
    glPushMatrix(); glTranslatef(0,0.0f,0); drawBox(24.0f,1.60f,6.0f); glPopMatrix();
    // Bow tapered point
    setMat(0.08f,0.10f,0.22f);
    glBegin(GL_TRIANGLES); glNormal3f(1,0,0);
    glVertex3f(12.0f,1.60f,-3.0f); glVertex3f(12.0f,1.60f,3.0f); glVertex3f(16.0f,1.20f,0);
    glVertex3f(12.0f,-0.10f,-3.0f); glVertex3f(12.0f,-0.10f,3.0f); glVertex3f(16.0f,-0.10f,0);
    glEnd();
    // Stern (flat back with small ramp)
    setMat(0.10f,0.12f,0.25f);
    glPushMatrix(); glTranslatef(-12.5f,0.80f,0); drawBox(1.0f,1.60f,6.0f); glPopMatrix();

    // White hull stripe
    setMat(0.96f,0.96f,0.96f);
    glPushMatrix(); glTranslatef(0,1.60f,0); drawBox(24.2f,0.20f,6.10f); glPopMatrix();

    // Main deck
    setMat(0.68f,0.62f,0.52f);
    glPushMatrix(); glTranslatef(0,1.80f,0); drawBox(24.0f,0.15f,6.0f); glPopMatrix();

    // Superstructure — 3 decks
    float sW=14.0f, sD=5.2f;
    // Deck 1
    setMat(0.92f,0.92f,0.90f);
    glPushMatrix(); glTranslatef(0,3.0f,0); drawBox(sW,2.4f,sD); glPopMatrix();
    // Deck 2
    setMat(0.88f,0.88f,0.86f);
    glPushMatrix(); glTranslatef(0,5.6f,0); drawBox(sW*0.80f,2.2f,sD*0.90f); glPopMatrix();
    // Deck 3 (bridge)
    setMat(0.84f,0.84f,0.82f);
    glPushMatrix(); glTranslatef(1.0f,7.9f,0); drawBox(sW*0.55f,2.0f,sD*0.75f); glPopMatrix();

    // Porthole windows — deck 1
    setMat(0.45f,0.65f,0.85f);
    for(int w=0;w<5;++w){
        float wx=-sW*0.40f+w*sW*0.80f/4.0f;
        glPushMatrix(); glTranslatef(wx,3.10f,sD*0.5f+0.06f); glutSolidSphere(0.38f,10,8); glPopMatrix();
        glPushMatrix(); glTranslatef(wx,3.10f,-sD*0.5f-0.06f); glutSolidSphere(0.38f,10,8); glPopMatrix();
    }
    // Windows — deck 2
    setMat(0.50f,0.68f,0.88f);
    for(int w=0;w<4;++w){
        float wx=-sW*0.32f+w*sW*0.64f/3.0f;
        glPushMatrix(); glTranslatef(wx,5.72f,sW*0.80f*0.5f*sD/sW+0.06f); drawBox(1.0f,0.90f,0.12f); glPopMatrix();
    }
    // Bridge windows (front)
    setMat(0.55f,0.72f,0.90f);
    for(int w=0;w<3;++w){
        float wx=-sW*0.22f+w*sW*0.44f/2.0f;
        glPushMatrix(); glTranslatef(wx,8.10f,sD*0.75f*0.5f+0.07f); drawBox(1.4f,1.0f,0.12f); glPopMatrix();
    }

    // 2 Funnels / smokestacks
    for(int f=-1;f<=1;f+=2){
        // stack body
        setMat(0.14f,0.14f,0.18f);
        glPushMatrix(); glTranslatef(f*2.5f,10.0f,0); glRotatef(-90,1,0,0);
        gluCylinder(q,0.70f,0.60f,3.5f,14,2); glPopMatrix();
        // stack rim
        setMat(0.20f,0.20f,0.24f);
        glPushMatrix(); glTranslatef(f*2.5f,13.5f,0); glRotatef(-90,1,0,0);
        gluCylinder(q,0.78f,0.70f,0.50f,14,1); glPopMatrix();
        // coloured band on funnel
        setMat(0.85f,0.15f,0.15f);
        glPushMatrix(); glTranslatef(f*2.5f,11.5f,0); glRotatef(-90,1,0,0);
        gluCylinder(q,0.72f,0.70f,1.0f,14,1); glPopMatrix();
    }

    // Forward mast
    setMat(0.55f,0.55f,0.58f);
    glPushMatrix(); glTranslatef(8.0f,1.95f,0); glRotatef(-90,1,0,0);
    gluCylinder(q,0.12f,0.08f,10.0f,10,1); glPopMatrix();
    // Radar dish on bridge
    setMat(0.72f,0.72f,0.75f);
    glPushMatrix(); glTranslatef(1.0f,10.0f,0); glScalef(1,0.25f,1); glutSolidSphere(1.0f,12,6); glPopMatrix();

    // Anchor chain holes (hull front)
    setMat(0.05f,0.05f,0.08f);
    glPushMatrix(); glTranslatef(10.5f,1.30f, 1.8f); glutSolidSphere(0.28f,8,6); glPopMatrix();
    glPushMatrix(); glTranslatef(10.5f,1.30f,-1.8f); glutSolidSphere(0.28f,8,6); glPopMatrix();

    // Deck railing
    setMat(0.80f,0.80f,0.82f);
    glPushMatrix(); glTranslatef(0,2.15f,3.10f); drawBox(24.0f,0.45f,0.08f); glPopMatrix();
    glPushMatrix(); glTranslatef(0,2.15f,-3.10f); drawBox(24.0f,0.45f,0.08f); glPopMatrix();

    glPopMatrix();
    gluDeleteQuadric(q);
}

// ============================================================
//  BIRDS
// ============================================================
static void drawBird(float x, float y, float z, float wp)
{
    setMat(0.05f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(x, y, z);
    float w = sinf(wp) * 0.6f;
    glPushMatrix(); glScalef(0.4f, 0.15f, 0.15f); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix();
    glRotatef(w * 40, 1, 0, 0);
    glTranslatef(0, 0.1f, 0.5f);
    glScalef(0.6f, 0.04f, 0.8f);
    glutSolidCube(1.0);
    glPopMatrix();
    glPushMatrix();
    glRotatef(-w * 40, 1, 0, 0);
    glTranslatef(0, 0.1f, -0.5f);
    glScalef(0.6f, 0.04f, 0.8f);
    glutSolidCube(1.0);
    glPopMatrix();
    glPopMatrix();
}

// ============================================================
//  RAIN
// ============================================================
static void initRain()
{
    for (int i = 0; i < RAIN_COUNT; ++i) {
        rainX[i] = frand(-80, 80);
        rainY[i] = frand(0, 40);
        rainZ[i] = frand(-30, 30);
    }
}

static void drawRain()
{
    if (!rain) return;
    glDisable(GL_LIGHTING);
    glColor3f(0.7f, 0.8f, 1.0f);
    glLineWidth(1.4f);
    glBegin(GL_LINES);
    for (int i = 0; i < RAIN_COUNT; ++i) {
        glVertex3f(rainX[i], rainY[i], rainZ[i]);
        glVertex3f(rainX[i], rainY[i] - 0.6f, rainZ[i]);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

// ============================================================
//  LIGHTING / FOG / HUD
// ============================================================
static void setupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat pos[4] = { -100.0f, 100.0f, 60.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, pos);

    GLfloat diff[4], amb[4];
    if (night) {
        diff[0] = 0.20f; diff[1] = 0.22f; diff[2] = 0.40f; diff[3] = 1;
        amb[0]  = 0.08f; amb[1]  = 0.08f; amb[2]  = 0.15f; amb[3]  = 1;
    } else if (sunset) {
        diff[0] = 1.00f; diff[1] = 0.55f; diff[2] = 0.30f; diff[3] = 1;
        amb[0]  = 0.30f; amb[1]  = 0.20f; amb[2]  = 0.15f; amb[3]  = 1;
    } else {
        diff[0] = 1.00f; diff[1] = 0.95f; diff[2] = 0.85f; diff[3] = 1;
        amb[0]  = 0.45f; amb[1]  = 0.45f; amb[2]  = 0.50f; amb[3]  = 1;
    }
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
}

static void setupFog()
{
    if (fogOn || rain) {
        glEnable(GL_FOG);
        GLfloat col[4] = { 0.75f, 0.80f, 0.85f, 1.0f };
        if (night) { col[0] = 0.05f; col[1] = 0.05f; col[2] = 0.12f; }
        glFogfv(GL_FOG_COLOR, col);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, 40.0f);
        glFogf(GL_FOG_END,   200.0f);
        glClearColor(col[0], col[1], col[2], 1.0f);
    } else {
        glDisable(GL_FOG);
        if (night)       glClearColor(0.03f, 0.04f, 0.10f, 1);
        else if (sunset) glClearColor(0.95f, 0.65f, 0.40f, 1);
        else             glClearColor(0.55f, 0.78f, 0.95f, 1);
    }
}

static void drawHUD(int w, int h)
{
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1, 1, 1);
    char buf[160];
    sprintf(buf, "Marine Drive 3D  |  Speed: %.2f  |  %s%s%s",
            vehicleSpeed,
            night ? "Night" : (sunset ? "Sunset" : "Day"),
            rain  ? "  Rain"  : "",
            fogOn ? "  Fog"   : "");
    renderText(12, h - 22, GLUT_BITMAP_HELVETICA_12, buf);
    renderText(12, 18, GLUT_BITMAP_HELVETICA_10,
               "WASD/QE move  |  drag LMB look  |  arrows speed  |  n/y/u mode  |  r/k rain  |  f/h fog  |  c reset");


    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

// ============================================================
//  CAMERA / DISPLAY
// ============================================================
static void applyCamera()
{
    float yawR   = camYaw   * PI / 180.0f;
    float pitchR = camPitch * PI / 180.0f;
    float fx = cosf(pitchR) * cosf(yawR);
    float fy = sinf(pitchR);
    float fz = cosf(pitchR) * sinf(yawR);
    gluLookAt(camX, camY, camZ,
              camX + fx, camY + fy, camZ + fz,
              0, 1, 0);
}

static void display()
{
    setupFog();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    applyCamera();
    setupLighting();

    drawSkyDome();
    drawStars();
    drawSunMoon();
    drawMountains();
    drawClouds();

    drawSea();
    drawBeach();
    drawRoad();

    // beach: umbrellas + people
    drawBeachStuff();

    // ALL coconut palm trees REMOVED per user request
    // (front-row, garden palms, and back-row scattered palms all disabled)

    // lamp posts (extended full road)
    for (int i = -66; i <= 66; ++i) drawLampPost(i * 12.0f);

    drawHotelRow();
    drawSeaviewResort();
    drawResortBoundary();
    // ---- garden strip between Seaview Resort and Seafood restaurant ----
    {
        // green grass patch
        setMat(0.30f, 0.62f, 0.32f);
        glPushMatrix();
        glTranslatef(0, -0.43f + 0.01f, 0);
        glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);
        glVertex3f(17.0f, 0, -19.0f);
        glVertex3f(22.0f, 0, -19.0f);
        glVertex3f(22.0f, 0, -50.0f);
        glVertex3f(17.0f, 0, -50.0f);
        glEnd();
        glPopMatrix();

        // scattered flowers + bushes in this garden (removed)
        /*srand(8888);
        for (int i = 0; i < 25; ++i) {
            float fx = frand(17.5f, 21.5f);
            float fz = frand(-49.0f, -19.5f);
            int kind = i % 3;
            if (kind == 0)      setMat(0.85f, 0.30f, 0.65f);   // pink
            else if (kind == 1) setMat(0.95f, 0.85f, 0.20f);   // yellow
            else                setMat(0.22f, 0.55f, 0.28f);   // green bush
            glPushMatrix();
            glTranslatef(fx, -0.43f + 0.22f, fz);
            glutSolidSphere(0.26, 8, 6);
            glPopMatrix();
        }*/

        // 2 small trees in the garden
        for (int i = 0; i < 2; ++i) {
            float fx = 19.5f;
            float fz = -25.0f - i * 16.0f;
            setMat(0.40f, 0.25f, 0.12f);
            glPushMatrix();
            glTranslatef(fx, -0.43f + 0.6f, fz);
            drawBox(0.12f, 1.2f, 0.12f);
            glPopMatrix();
            setMat(0.22f, 0.55f, 0.25f);
            glPushMatrix();
            glTranslatef(fx, -0.43f + 1.5f, fz);
            glutSolidSphere(0.75, 12, 10);
            glPopMatrix();
        }
    }
    drawRestaurant(33.0f, -55.0f);
    drawFiveStarRestaurant(-50.0f, -55.0f);   // The Grand — 5-star restaurant
    drawBoutiqueHotel(-88.0f, -55.0f);        // Coral Bay Hotel
    drawBlueWaveBar(-135.0f, -55.0f);         // Blue Wave Bar & Lounge
    drawSeaPearlResort(-172.0f, -55.0f);      // Sea Pearl Resort
    drawOceanBreezeInn(-218.0f, -55.0f);      // Ocean Breeze Inn
    drawNeptuneResort(-265.0f, -55.0f);       // Neptune Resort & Spa
    drawSeaMistCafe(-312.0f, -55.0f);         // Sea Mist Cafe & Grill
    drawPalmBayHotel(-358.0f, -55.0f);
    drawOceanBreezeInn(-405.0f, -55.0f);
    drawNeptuneResort(-452.0f, -55.0f);
    drawSeaMistCafe(-498.0f, -55.0f);
    drawPalmBayHotel(-545.0f, -55.0f);
    drawBoutiqueHotel(-591.0f, -55.0f);
    drawBlueWaveBar(-637.0f, -55.0f);
    drawSeaPearlResort(-683.0f, -55.0f);
    drawOceanBreezeInn(-729.0f, -55.0f);
    drawNeptuneResort(-776.0f, -55.0f);       // left road end x≈-800
    drawBeachResort(175.0f, -55.0f);  // U-shaped beach resort at far right
    drawVolleyballCourt(155.0f, 5.0f);        // Beach volleyball, Baywatch left side
    drawBoutiqueHotel(268.0f, -55.0f);        // right of water park
    drawSeaMistCafe(316.0f, -55.0f);
    drawNeptuneResort(362.0f, -55.0f);
    drawPalmBayHotel(408.0f, -55.0f);
    drawOceanBreezeInn(452.0f, -55.0f);
    drawNeptuneResort(498.0f, -55.0f);
    drawPalmBayHotel(544.0f, -55.0f);
    drawSeaMistCafe(590.0f, -55.0f);
    drawBoutiqueHotel(636.0f, -55.0f);
    drawBlueWaveBar(682.0f, -55.0f);
    drawSeaPearlResort(728.0f, -55.0f);
    drawOceanBreezeInn(774.0f, -55.0f);       // right road end x≈+800
    drawWaterPark(218.0f, -55.0f);    // Water park right of beach resort
    // train removed
    drawBBQLounge(140.0f, -58.0f);    // BBQ lounge - longer + pushed slightly back
    drawBBQCrowd(140.0f, -58.0f);     // beach crowd eating BBQ in front of lounge
    drawIceCreamStall(116.0f, -56.0f); // Ice Cream Stall — filling gap between Shop & BBQ
    drawSeafoodCart(105.0f,-42.0f,155.0f); // seafood thelagari #1
    drawSeafoodCart(117.0f,-46.0f,180.0f); // seafood thelagari #2
    drawSeafoodCart(128.0f,-42.0f,205.0f); // seafood thelagari #3
    drawBeachShop(90.0f, -55.0f);     // Beach Vibes Boutique
    drawDeraResort(62.0f, -55.0f);    // Dera Resort (duplex hotel)
    // ice cream stall removed
    if (animOn) {
        for (int i = 0; i < 4; ++i) {
            float sx = -45.0f + i * 28.0f;
            float sz = 15.0f + sinf(swimPhase[i] * 0.25f) * 6.0f;
            drawSwimmer(sx, sz, swimPhase[i]);
        }
    }
    if (thunderOn) drawLightning();
    drawPool();

    // ---- BIG GARDEN on the right side of Seaview Resort (extends to boundary) ----
    {
        float gY = -0.40f;
        // garden area: from x=3 (right of resort center) to x=16.5 (inside boundary at 17)
        //              from z=-25 (front area) to z=-72 (back, near back boundary at -78)
        float gxL = 3.0f, gxR = 16.5f;
        float gzF = -25.0f, gzB = -72.0f;
        float gxMid = (gxL + gxR) * 0.5f;
        float gzMid = (gzF + gzB) * 0.5f;

        // grass patch
        setMat(0.32f, 0.65f, 0.34f);
        glPushMatrix();
        glTranslatef(0, gY + 0.005f, 0);
        glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);
        glVertex3f(gxL, 0, gzB);
        glVertex3f(gxR, 0, gzB);
        glVertex3f(gxR, 0, gzF);
        glVertex3f(gxL, 0, gzF);
        glEnd();
        glPopMatrix();

        // central fountain
        setMat(0.95f, 0.95f, 0.95f);
        glPushMatrix();
        glTranslatef(gxMid, gY + 0.50f, gzMid);
        drawBox(1.4f, 1.00f, 1.4f);
        glPopMatrix();
        setMat(0.55f, 0.78f, 0.92f);
        glPushMatrix();
        glTranslatef(gxMid, gY + 1.15f, gzMid);
        glutSolidSphere(0.55, 14, 12);
        glPopMatrix();

        // gravel/sand walking paths in cross pattern
        setMat(0.86f, 0.80f, 0.65f);
        // horizontal path
        glPushMatrix();
        glTranslatef(0, gY + 0.015f, 0);
        glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);
        glVertex3f(gxL,  0, gzMid - 1.0f);
        glVertex3f(gxR,  0, gzMid - 1.0f);
        glVertex3f(gxR,  0, gzMid + 1.0f);
        glVertex3f(gxL,  0, gzMid + 1.0f);
        glEnd();
        glPopMatrix();
        // vertical path
        glPushMatrix();
        glTranslatef(0, gY + 0.015f, 0);
        glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);
        glVertex3f(gxMid - 1.0f, 0, gzB);
        glVertex3f(gxMid + 1.0f, 0, gzB);
        glVertex3f(gxMid + 1.0f, 0, gzF);
        glVertex3f(gxMid - 1.0f, 0, gzF);
        glEnd();
        glPopMatrix();

        // dense flower beds removed
        /*srand(99999);
        for (int i = 0; i < 130; ++i) {
            float fx = frand(gxL + 0.5f, gxR - 0.5f);
            float fz = frand(gzB + 0.5f, gzF - 0.5f);
            // skip cross paths
            if (fabsf(fz - gzMid) < 1.1f) continue;
            if (fabsf(fx - gxMid) < 1.1f) continue;
            // skip fountain area
            if (fabsf(fx - gxMid) < 1.0f && fabsf(fz - gzMid) < 1.0f) continue;
            int kind = i % 4;
            if (kind == 0)      setMat(0.85f, 0.30f, 0.65f);   // pink
            else if (kind == 1) setMat(0.95f, 0.85f, 0.20f);   // yellow
            else if (kind == 2) setMat(0.85f, 0.45f, 0.25f);   // orange
            else                setMat(0.22f, 0.55f, 0.28f);   // green bush
            glPushMatrix();
            glTranslatef(fx, gY + 0.22f, fz);
            glutSolidSphere(0.26, 8, 6);
            glPopMatrix();
        }*/

        // 10 decorative trees scattered through the garden
        srand(11111);
        for (int i = 0; i < 10; ++i) {
            float tx = frand(gxL + 1.5f, gxR - 1.5f);
            float tz = frand(gzB + 1.5f, gzF - 1.5f);
            // skip the cross paths + fountain
            if (fabsf(tz - gzMid) < 1.5f) continue;
            if (fabsf(tx - gxMid) < 1.5f) continue;
            setMat(0.40f, 0.25f, 0.12f);
            glPushMatrix();
            glTranslatef(tx, gY + 0.7f, tz);
            drawBox(0.14f, 1.4f, 0.14f);
            glPopMatrix();
            setMat(0.20f + frand(0, 0.10f), 0.55f + frand(0, 0.15f), 0.25f);
            glPushMatrix();
            glTranslatef(tx, gY + 1.7f, tz);
            glutSolidSphere(0.85, 12, 10);
            glPopMatrix();
        }

        // 4 garden benches along the cross paths
        setMat(0.50f, 0.32f, 0.18f);
        float benchPos[4][3] = {
            { gxMid + 3.5f, gzMid, 0 },
            { gxMid - 3.5f, gzMid, 0 },
            { gxMid, gzMid + 5.0f, 90 },
            { gxMid, gzMid - 5.0f, 90 }
        };
        for (int i = 0; i < 4; ++i) {
            glPushMatrix();
            glTranslatef(benchPos[i][0], gY + 0.35f, benchPos[i][1]);
            glRotatef(benchPos[i][2], 0, 1, 0);
            drawBox(1.4f, 0.10f, 0.45f);
            glPopMatrix();
            // bench backrest
            glPushMatrix();
            glTranslatef(benchPos[i][0], gY + 0.70f, benchPos[i][1]);
            glRotatef(benchPos[i][2], 0, 1, 0);
            glTranslatef(0, 0, -0.20f);
            drawBox(1.4f, 0.55f, 0.06f);
            glPopMatrix();
        }
    }

    //drawGardenFlowers();  // flowers removed
    drawForestFloor();
    drawForestBelt();

    // vehicles - two directions, two lanes
    drawCar(carPos[0], 0.95f, 0.95f, 0.95f, false);
    drawCar(carPos[1], 0.85f, 0.15f, 0.15f, true);
    drawCar(carPos[2], 0.20f, 0.55f, 0.85f, false);
    drawBus(carPos[3], 0.85f, 0.20f, 0.20f, true);   // red bus like the image
    drawCar(carPos[4], 0.90f, 0.45f, 0.20f, false);
    drawBus(carPos[5], 0.20f, 0.30f, 0.70f, true);
    drawCar(carPos[6], 0.80f, 0.80f, 0.20f, false);
    drawCar(carPos[7], 0.30f, 0.30f, 0.30f, true);

    // boats — animated small boats
    drawBoat(boatPos[0], 30.0f);
    drawBoat(boatPos[1], 55.0f);
    drawBoat(boatPos[2], 80.0f);
    drawBoat(boatPos[3], 100.0f);

    // fishing boats — static, scattered across sea
    drawFishingBoat(-35.0f,  22.0f,  15.0f);   // near shore, slight angle
    drawFishingBoat( 55.0f,  18.0f,   0.0f);   // straight ahead
    drawFishingBoat(-18.0f,  42.0f, 170.0f);   // mid-sea, facing left
    drawFishingBoat( 30.0f,  68.0f,  10.0f);   // further out
    drawFishingBoat(-60.0f,  90.0f, 190.0f);   // far left, facing away
    drawFishingBoat( 70.0f,  75.0f, 350.0f);   // far right, turned around
    drawFishingBoat(  5.0f, 110.0f,  30.0f);   // deep sea, slightly angled

    // large ship (jahaj) — distant, static
    drawShip(-20.0f, 105.0f, 160.0f);   // large ship centre-sea, angled
    drawShip( 50.0f, 118.0f, 200.0f);   // second ship further right

    // birds
    for (int i = 0; i < 6; ++i) {
        float bx = -40.0f + i * 12.0f + sinf(birdPhase[i] * 0.4f) * 6;
        float by = 15.0f + sinf(birdPhase[i]) * 1.0f;
        float bz = 25.0f + i * 4.0f;
        drawBird(bx, by, bz, birdPhase[i] * 4);
    }

    drawRain();

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    drawHUD(w, h);

    glutSwapBuffers();
}

// ============================================================
//  UPDATE
// ============================================================
static void update(int v)
{
    if (animOn) {
        carPos[0] += vehicleSpeed;       if (carPos[0] >  170.0f) carPos[0] = -170.0f;
        carPos[1] -= vehicleSpeed;       if (carPos[1] < -170.0f) carPos[1] =  170.0f;
        carPos[2] += vehicleSpeed*1.1f;  if (carPos[2] >  170.0f) carPos[2] = -170.0f;
        carPos[3] -= vehicleSpeed*0.9f;  if (carPos[3] < -170.0f) carPos[3] =  170.0f;
        carPos[4] += vehicleSpeed*0.95f; if (carPos[4] >  170.0f) carPos[4] = -170.0f;
        carPos[5] -= vehicleSpeed*1.05f; if (carPos[5] < -170.0f) carPos[5] =  170.0f;
        carPos[6] += vehicleSpeed*1.20f; if (carPos[6] >  170.0f) carPos[6] = -170.0f;
        carPos[7] -= vehicleSpeed*0.85f; if (carPos[7] < -170.0f) carPos[7] =  170.0f;

        boatPos[0] += 0.01f;  if (boatPos[0] > 60) boatPos[0] = -60;
        boatPos[1] -= 0.008f; if (boatPos[1] <-60) boatPos[1] =  60;
        boatPos[2] += 0.012f; if (boatPos[2] > 60) boatPos[2] = -60;
        boatPos[3] -= 0.009f; if (boatPos[3] <-60) boatPos[3] =  60;

        for (int i = 0; i < 6; ++i) birdPhase[i] += 0.15f;
        waiterPhase += 0.06f;     // waiter walking animation
        for (int i = 0; i < 4; ++i) swimPhase[i] += 0.05f;
        // shopPhase and trainX intentionally not updated — scene is static
        if (thunderOn) {
            if (thunderTimer > 0.0f) { thunderTimer -= 0.025f; thunderFlash *= 0.88f; }
            else { thunderFlash=frand(0.55f,1.0f); thunderX=frand(-90.0f,90.0f); thunderZ=frand(5.0f,80.0f); thunderTimer=frand(1.5f,6.0f); }
        }

        if (rain) {
            for (int i = 0; i < RAIN_COUNT; ++i) {
                rainY[i] -= 1.2f;
                if (rainY[i] < -0.5f) {
                    rainY[i] = frand(20, 40);
                    rainX[i] = frand(-80, 80);
                    rainZ[i] = frand(-30, 30);
                }
            }
        }
    }
    glutPostRedisplay();
    glutTimerFunc(25, update, 0);
}

// ============================================================
//  INPUT
// ============================================================
static void keyboard(unsigned char key, int x, int y)
{
    float step = 2.0f;
    float yawR = camYaw * PI / 180.0f;
    float fx = cosf(yawR), fz = sinf(yawR);
    switch (key) {
        case 'w': case 'W': camX += fx * step; camZ += fz * step; break;
        case 's': case 'S': camX -= fx * step; camZ -= fz * step; break;
        case 'a': case 'A': camX -= fz * step; camZ += fx * step; break;
        case 'd': case 'D': camX += fz * step; camZ -= fx * step; break;
        case 'q': case 'Q': camY += step * 0.5f; break;
        case 'e': case 'E': camY -= step * 0.5f; break;
        case '1': animOn = true;  break;
        case '0': animOn = false; break;
        case 'n': case 'N': night = true;  sunset = false; break;
        case 'y': case 'Y': night = false; sunset = false; break;
        case 'u': case 'U': sunset = !sunset; night = false; break;
        case 'r': case 'R': rain = true;  break;
        case 'k': case 'K': rain = false; break;
        case 'l': case 'L': thunderOn = !thunderOn; thunderFlash = 0; thunderTimer = 0.5f; break;
        case 'f': case 'F': fogOn = true;  break;
        case 'h': case 'H': fogOn = false; break;
        case 'c': case 'C': camX=0; camY=4; camZ=30; camYaw=0; camPitch=-8; break;
        case '+': glutFullScreen(); break;
        case '-': glutReshapeWindow(1280, 720); glutPositionWindow(50, 40); break;
        case 'x': case 'X': case 27: exit(0); break;
    }
    glutPostRedisplay();
}

static void special(int key, int x, int y)
{
    if (key == GLUT_KEY_UP)   vehicleSpeed += 0.02f;
    if (key == GLUT_KEY_DOWN) { vehicleSpeed -= 0.02f; if (vehicleSpeed < 0) vehicleSpeed = 0; }
    glutPostRedisplay();
}

static void mouseBtn(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
        mouseDragging = (state == GLUT_DOWN);
        lastMouseX = x; lastMouseY = y;
    }
}

static void mouseMove(int x, int y)
{
    if (!mouseDragging) return;
    float dx = (float)(x - lastMouseX);
    float dy = (float)(y - lastMouseY);
    camYaw   += dx * 0.25f;
    camPitch -= dy * 0.25f;
    if (camPitch >  89) camPitch =  89;
    if (camPitch < -89) camPitch = -89;
    lastMouseX = x; lastMouseY = y;
    glutPostRedisplay();
}

static void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55.0, (double)w / (double)h, 0.1, 2500.0);
    glMatrixMode(GL_MODELVIEW);
}

static void initGL()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glClearColor(0.55f, 0.78f, 0.95f, 1.0f);

    initTextures();

    carPos[0] = -80; carPos[1] =  60; carPos[2] = -30; carPos[3] =  10;
    carPos[4] =  25; carPos[5] = -50; carPos[6] = -10; carPos[7] =  70;
    boatPos[0] = -20; boatPos[1] = 10; boatPos[2] = 40; boatPos[3] = 70;
    for (int i = 0; i < 6; ++i) birdPhase[i] = i * 0.7f;
    initRain();
}

int main(int argc, char** argv)
{
    cout << "==========================================" << endl;
    cout << "   MARINE DRIVE COX'S BAZAR - RESORT 3D   " << endl;
    cout << "==========================================" << endl;
    cout << " W A S D     : move camera" << endl;
    cout << " Q / E       : up / down" << endl;
    cout << " Drag LMB    : look around" << endl;
    cout << " UP / DOWN   : vehicle speed +/-" << endl;
    cout << " 1 / 0       : animation on / off" << endl;
    cout << " n           : night mode" << endl;
    cout << " y           : day mode" << endl;
    cout << " u           : toggle sunset" << endl;
    cout << " r / k       : rain on / off" << endl;
    cout << " f / h       : fog on / off" << endl;
    cout << " c           : reset camera" << endl;
    cout << " + / -       : fullscreen / windowed" << endl;
    cout << " x / Esc     : exit" << endl;
    cout << "==========================================" << endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutInitWindowPosition(50, 40);
    glutCreateWindow("MARINE DRIVE COX BAZAR - 3D RESORT");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouseBtn);
    glutMotionFunc(mouseMove);
    glutTimerFunc(25, update, 0);

    glutMainLoop();
    return 0;
}
