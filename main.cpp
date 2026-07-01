/**
 * Marine Drive Cox Bazar - OpenGL Scene
 * ======================================
 * Controls:
 *   s        : Show / hide title card
 *   1 / 0    : Animation ON / OFF
 *   n / d    : Night / Day mode
 *   u        : Sunset mode toggle
 *   r / e    : Rain ON / OFF
 *   h        : Hands up / down
 *   UP / DOWN: Speed up / slow down vehicles
 *   + / -    : Full-screen / restore window
 *   x        : Exit
 */
#include <iostream>
#include <GL/gl.h>
#include <GL/glut.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <mmsystem.h>

#define PI 3.14159265358979323846

using namespace std;
int triangleAmount = 1000;
GLfloat twicePi = 2.0f * PI;

float vehicleSpeed = 0.003f;

bool  night       = false;
bool  on          = false;
bool  cover       = false;
bool  vehicleStop = false;
bool  rain        = false;
bool  waveUp      = true;
bool  handup      = false;

float _crashB2 = 0.0f;
float _crashB1 = 0.0f;

// Rain drop positions (11 x-offsets, 15 y-offsets)
float rainX[11] = {};
float rainY[15] = {};
int   rainStep  = 0;

// Vehicle scroll positions
// run[0..3]: clouds/misc  run[4]: green car  run[6]: red bus  etc.
float run[11] = {};

// Wave animation offsets (17 wave segments)
float wave[17] = {};

// === NEW FEATURE GLOBALS ===
bool sunset = false;
float _sunX = -0.5f;
float _moonX = 0.5f;
float birdX[3]  = { -1.3f, -1.5f, -1.8f };
float _birdY  = 0.72f;
float _birdWing = 0.0f;
bool  wingUp  = true;
int   wingTimer = 0;
float _starTwinkle = 1.0f;
bool  starBright = true;
int   starTimer = 0;
// Beach scene globals
int   trafficState = 0;    // 0=red,1=yellow,2=green
int   trafficTimer = 0;
float _horseX   = -0.30f; // horse+owner walking
float _sell1X   =  0.55f; // shell seller 1
float _sell2X   =  0.75f; // shell seller 2

// === NEW PEOPLE & OBJECTS GLOBALS (added) ===
float _coupleX     = 0.00f;   // couple walking offset
bool  _coupleDir   = true;    // true = moving right
float _childJump   = 0.0f;    // child jump height
bool  _childUp     = true;
int   _childTimer  = 0;
float _leafSway    = 0.0f;    // coconut leaf sway angle (degrees)
bool  _leafDir     = true;
int   _flashTimer  = 0;       // photographer flash timer
bool  _flashOn     = false;
float _netAngle    = 0.0f;    // fisherman net swing (degrees)
bool  _netDir      = true;

void resetRain()
{
    for(int k = 0; k < 10; k++) rainY[k] = 0.0f;
    rainStep = 0;
}

void clearColor(float r, float g, float b,float bb)
{
    glClearColor(r,g,b,bb);
}

void renderBitmapString(float x, float y, float z, void *font, char *string)
{
    char *c;
    glRasterPos3f(x, y,z);
    for (c=string; *c != '\0'; c++)
    {
        glutBitmapCharacter(font, *c);
    }
}

// Draw a filled circle (TRIANGLE_FAN)
void drawCircle(float cx, float cy, float r)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for(int k = 0; k <= 100; k++)
        glVertex2f(cx + r * cosf(k * twicePi / 100),
                   cy + r * sinf(k * twicePi / 100));
    glEnd();
}

// Draw one cloud puff at (cx,cy) with radius r
void drawCloudPuff(float cx, float cy, float r)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for(int k = 0; k <= 100; k++)
        glVertex2f(cx + r * cosf(k * twicePi / 100),
                   cy + r * sinf(k * twicePi / 100));
    glEnd();
}

// Set cloud colour based on current sky mode
void setCloudColor()
{
    if(night)                 glColor3f(0.50f, 0.50f, 0.50f);
    else if(rain && !night)   glColor3f(0.43f, 0.63f, 0.85f);
    else                      glColor3f(0.95f, 0.95f, 0.95f);
}

void drawSky()
{
//sky - GRADIENT SKY (upgraded).............................................

    // === GRADIENT SKY ===
    glBegin(GL_QUADS);
    if(night)
    {
        // Night: deep navy at top, dark teal at horizon
        glColor3f(0.0f, 0.01f, 0.08f);   // top-left
        glVertex2f(-1.0f, 1.0f);
        glColor3f(0.0f, 0.01f, 0.08f);   // top-right
        glVertex2f( 1.0f, 1.0f);
        glColor3f(0.05f, 0.15f, 0.25f);  // bottom-right (horizon)
        glVertex2f( 1.0f, 0.5f);
        glColor3f(0.05f, 0.15f, 0.25f);  // bottom-left (horizon)
        glVertex2f(-1.0f, 0.5f);
    }
    else if(sunset)
    {
        // Sunset: deep orange/red at horizon, purple at top
        glColor3f(0.3f, 0.0f, 0.3f);     // top - purple
        glVertex2f(-1.0f, 1.0f);
        glColor3f(0.3f, 0.0f, 0.3f);
        glVertex2f( 1.0f, 1.0f);
        glColor3f(1.0f, 0.45f, 0.0f);    // horizon - orange
        glVertex2f( 1.0f, 0.5f);
        glColor3f(1.0f, 0.45f, 0.0f);
        glVertex2f(-1.0f, 0.5f);
    }
    else if(rain)
    {
        // Rainy: grey-blue gradient
        glColor3f(0.15f, 0.25f, 0.40f);
        glVertex2f(-1.0f, 1.0f);
        glColor3f(0.15f, 0.25f, 0.40f);
        glVertex2f( 1.0f, 1.0f);
        glColor3f(0.30f, 0.45f, 0.60f);
        glVertex2f( 1.0f, 0.5f);
        glColor3f(0.30f, 0.45f, 0.60f);
        glVertex2f(-1.0f, 0.5f);
    }
    else
    {
        // Day: bright cyan at top, light sky-blue at horizon
        glColor3f(0.25f, 0.55f, 0.95f);  // top
        glVertex2f(-1.0f, 1.0f);
        glColor3f(0.25f, 0.55f, 0.95f);
        glVertex2f( 1.0f, 1.0f);
        glColor3f(0.65f, 0.88f, 1.0f);   // horizon
        glVertex2f( 1.0f, 0.5f);
        glColor3f(0.65f, 0.88f, 1.0f);
        glVertex2f(-1.0f, 0.5f);
    }
    glEnd();


}

void drawSun()
{
    // sun - MOVING SUN WITH GLOW (upgraded)
    if(!night)
    {
        GLfloat xs1 = _sunX;
        GLfloat ys1 = 0.83f;

        // Outer glow
        GLfloat glowR = sunset ? 0.07f : 0.085f;
        glColor3f( sunset ? 1.0f : 1.0f,
                   sunset ? 0.6f : 0.95f,
                   sunset ? 0.0f : 0.4f );
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(xs1, ys1);
        for(int i = 0; i <= triangleAmount; i++)
            glVertex2f(xs1 + glowR*cos(i*twicePi/triangleAmount),
                       ys1 + glowR*sin(i*twicePi/triangleAmount));
        glEnd();

        // Bright core
        GLfloat radiuss1 = 0.05f;
        glColor3f( sunset ? 1.0f : 0.98f,
                   sunset ? 0.85f : 0.97f,
                   sunset ? 0.2f  : 0.0f );
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(xs1, ys1);
        for(int i = 0; i <= triangleAmount; i++)
            glVertex2f(xs1 + radiuss1*cos(i*twicePi/triangleAmount),
                       ys1 + radiuss1*sin(i*twicePi/triangleAmount));
        glEnd();
    }


}

void drawMoon()
{
    // moon - CRESCENT MOON WITH GLOW (upgraded)
    if(night)
    {
        GLfloat xsm1 = _moonX;
        GLfloat ysm1 = 0.88f;
        GLfloat radiussm1 = 0.055f;

        // Soft glow behind moon
        glColor3f(0.7f, 0.7f, 0.5f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(xsm1, ysm1);
        for(int i = 0; i <= triangleAmount; i++)
            glVertex2f(xsm1 + 0.075f*cos(i*twicePi/triangleAmount),
                       ysm1 + 0.075f*sin(i*twicePi/triangleAmount));
        glEnd();

        // Full moon (white-yellow)
        glColor3f(1.0f, 0.98f, 0.85f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(xsm1, ysm1);
        for(int i = 0; i <= triangleAmount; i++)
            glVertex2f(xsm1 + radiussm1*cos(i*twicePi/triangleAmount),
                       ysm1 + radiussm1*sin(i*twicePi/triangleAmount));
        glEnd();

        // Overlay circle to create crescent
        GLfloat xsm2 = xsm1 + 0.025f;
        GLfloat ysm2 = ysm1 + 0.005f;
        glColor3f(0.02f, 0.04f, 0.12f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(xsm2, ysm2);
        for(int i = 0; i <= triangleAmount; i++)
            glVertex2f(xsm2 + radiussm1*cos(i*twicePi/triangleAmount),
                       ysm2 + radiussm1*sin(i*twicePi/triangleAmount));
        glEnd();
    }

}

void drawStars()
{
    // star - TWINKLING STARS (upgraded)
    if(night && !rain)
    {
        glPointSize(2.5f);
        glBegin(GL_POINTS);
        // Alternate bright/dim groups for twinkle effect
        glColor3f(_starTwinkle, _starTwinkle, _starTwinkle * 0.9f);
        glVertex2f(-0.9f, 0.9f);

        glVertex2f(-0.7f, 0.9f);

        glVertex2f(-0.5f, 0.9f);

        glVertex2f(-0.3f, 0.9f);

        glVertex2f(-0.1f, 0.9f);

        glVertex2f(0.1f, 0.9f);

        glVertex2f(0.3f, 0.9f);

        glVertex2f(0.5f, 0.9f);

        glVertex2f(0.7f, 0.9f);

        glVertex2f(0.9f, 0.9f);

        glVertex2f(-0.98f, 0.93f);

        glVertex2f(-0.88f, 0.94f);

        glVertex2f(-0.68f, 0.91f);

        glVertex2f(-0.55f, 0.92f);

        glVertex2f(-0.45f, 0.93f);

        glVertex2f(-0.25f, 0.94f);

        glVertex2f(0.0f, 0.9f);

        glVertex2f(0.13f, 0.91f);

        glVertex2f(0.25f, 0.92f);

        glVertex2f(0.37f, 0.93f);

        glVertex2f(0.49f, 0.92f);

        glVertex2f(-0.8f, 0.85f);

        glVertex2f(-0.6f, 0.85f);

        glVertex2f(-0.4f, 0.85f);

        glVertex2f(-0.2f, 0.85f);

        glVertex2f(0.0f, 0.85f);

        glVertex2f(0.2f, 0.85f);

        glVertex2f(0.4f, 0.85f);

        glVertex2f(0.6f, 0.85f);

        glVertex2f(0.8f, 0.85f);

        glVertex2f(1.0f, 0.85f);

        glEnd();
    }
}

void drawBirds()
{


    // === FLYING BIRDS (new animation) ===
    if(!cover)
    {
        // Wing shape: two arcs forming a V
        float wx = _birdWing;  // wing flap offset

        // Bird 1
        glColor3f(0.1f, 0.1f, 0.1f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        glVertex2f(birdX[0] - 0.04f, _birdY - wx*0.03f);
        glVertex2f(birdX[0],         _birdY + wx*0.03f);
        glVertex2f(birdX[0] + 0.04f, _birdY - wx*0.03f);
        glEnd();

        // Bird 2 (slightly lower, behind)
        glBegin(GL_LINE_STRIP);
        glVertex2f(birdX[1] - 0.035f, _birdY - 0.04f - wx*0.03f);
        glVertex2f(birdX[1],          _birdY - 0.04f + wx*0.03f);
        glVertex2f(birdX[1] + 0.035f, _birdY - 0.04f - wx*0.03f);
        glEnd();

        // Bird 3
        glBegin(GL_LINE_STRIP);
        glVertex2f(birdX[2] - 0.03f, _birdY - 0.02f - wx*0.025f);
        glVertex2f(birdX[2],         _birdY - 0.02f + wx*0.025f);
        glVertex2f(birdX[2] + 0.03f, _birdY - 0.02f - wx*0.025f);
        glEnd();
        glLineWidth(1.0f);
    }

}

void drawHills()
{
//hill3

    glPushMatrix();
    glTranslatef(0.3f,0.0f,0.0f);

    glBegin(GL_POLYGON);
    glColor3f(0.0f,0.2f,0.0f);
    glVertex2f(-1.0f, 0.5f);
    glVertex2f(-1.0f, 0.65f);


    glVertex2f(-1.0f, 0.65f);
    glVertex2f(-0.97f, 0.68f);

    glVertex2f(-0.97f, 0.68f);
    glVertex2f(-0.95f, 0.66f);

    glVertex2f(-0.95f, 0.66f);
    glVertex2f(-0.9f, 0.77f);

    glVertex2f(-0.9f, 0.77f);
    glVertex2f(-0.87f, 0.73f);

    glVertex2f(-0.87f, 0.73f);
    glVertex2f(-0.84f, 0.66f);

    glVertex2f(-0.84f, 0.66f);
    glVertex2f(-0.8f, 0.7f);

    glVertex2f(-0.8f, 0.7f);
    glVertex2f(-0.78f, 0.68f);

    glVertex2f(-0.78f, 0.68f);
    glVertex2f(-0.73f, 0.73f);

    glVertex2f(-0.73f, 0.73f);
    glVertex2f(-0.68f, 0.64f);

    glVertex2f(-0.68f, 0.64f);
    glVertex2f(-0.65f, 0.67f);

    glVertex2f(-0.65f, 0.67f);
    glVertex2f(-0.58f, 0.58f);

    glVertex2f(-0.58f, 0.58f);
    glVertex2f(-0.55f, 0.6f);

    glVertex2f(-0.55f, 0.6f);
    glVertex2f(-0.5f, 0.5f);


    glEnd();


    glPopMatrix();

//hill01
    glBegin(GL_POLYGON);
    glColor3f(0.0f,0.4f,0.0f);
    if(night)
    {
        glColor3f(0.09, 0.20, 0.09);
    }
    glVertex2f(-1.0f, 0.5f);
    glVertex2f(-1.0f, 0.65f);


    glVertex2f(-1.0f, 0.65f);
    glVertex2f(-0.97f, 0.68f);

    glVertex2f(-0.97f, 0.68f);
    glVertex2f(-0.95f, 0.66f);

    glVertex2f(-0.95f, 0.66f);
    glVertex2f(-0.9f, 0.75f);

    glVertex2f(-0.9f, 0.75f);
    glVertex2f(-0.87f, 0.73f);

    glVertex2f(-0.87f, 0.73f);
    glVertex2f(-0.84f, 0.66f);

    glVertex2f(-0.84f, 0.66f);
    glVertex2f(-0.8f, 0.7f);

    glVertex2f(-0.8f, 0.7f);
    glVertex2f(-0.78f, 0.68f);

    glVertex2f(-0.78f, 0.68f);
    glVertex2f(-0.73f, 0.73f);

    glVertex2f(-0.73f, 0.73f);
    glVertex2f(-0.68f, 0.64f);

    glVertex2f(-0.68f, 0.64f);
    glVertex2f(-0.65f, 0.67f);

    glVertex2f(-0.65f, 0.67f);
    glVertex2f(-0.58f, 0.58f);

    glVertex2f(-0.58f, 0.58f);
    glVertex2f(-0.55f, 0.6f);

    glVertex2f(-0.55f, 0.6f);
    glVertex2f(-0.5f, 0.5f);


    glEnd();


    //hill5

    glPushMatrix();
    glTranslatef(0.8f,0.0f,0.0f);

    glBegin(GL_POLYGON);
    glColor3f(0.0f,0.2f,0.0f);
    if(night)
    {
        glColor3f(0.08, 0.19, 0.08);
    }


    glVertex2f(-0.65f, 0.5f);
    glVertex2f(-0.63f, 0.55f);

    glVertex2f(-0.63f, 0.55f);
    glVertex2f(-0.61f, 0.53f);

    glVertex2f(-0.61f, 0.53f);
    glVertex2f(-0.55f, 0.65f);

    glVertex2f(-0.55f, 0.65f);
    glVertex2f(-0.52f, 0.63f);

    glVertex2f(-0.52f, 0.63f);
    glVertex2f(-0.5f, 0.68f);

    glVertex2f(-0.5f, 0.68f);
    glVertex2f(-0.47f, 0.66f);

    glVertex2f(-0.47f, 0.66f);
    glVertex2f(-0.44f, 0.73f);

    glVertex2f(-0.44f, 0.73f);
    glVertex2f(-0.4f, 0.7f);

    glVertex2f(-0.4f, 0.7f);
    glVertex2f(-0.35f, 0.62f);

    glVertex2f(-0.35f, 0.62f);
    glVertex2f(-0.3f, 0.66f);

    glVertex2f(-0.3f, 0.66f);
    glVertex2f(-0.28f, 0.63f);

    glVertex2f(-0.28f, 0.63f);
    glVertex2f(-0.24f, 0.69f);

    glVertex2f(-0.24f, 0.69f);
    glVertex2f(-0.18f, 0.58f);

    glVertex2f(-0.18f, 0.58f);
    glVertex2f(-0.15f, 0.6f);

    glVertex2f(-0.15f, 0.6f);
    glVertex2f(-0.11f, 0.55f);

    glVertex2f(-0.11f, 0.55f);
    glVertex2f(-0.1f, 0.5f);

    glEnd();

    glPopMatrix();

    //hill4

    glPushMatrix();
    glTranslatef(0.78f,0.0f,0.0f);

    glBegin(GL_POLYGON);
    glColor3f(0.0f,0.3f,0.0f);
    if(night)
    {
        glColor3f(0.12, 0.28, 0.12);
    }
    glVertex2f(-1.0f, 0.5f);
    glVertex2f(-1.0f, 0.65f);


    glVertex2f(-1.0f, 0.65f);
    glVertex2f(-0.97f, 0.68f);

    glVertex2f(-0.97f, 0.68f);
    glVertex2f(-0.95f, 0.66f);

    glVertex2f(-0.95f, 0.66f);
    glVertex2f(-0.9f, 0.75f);

    glVertex2f(-0.9f, 0.75f);
    glVertex2f(-0.87f, 0.73f);

    glVertex2f(-0.87f, 0.73f);
    glVertex2f(-0.84f, 0.66f);

    glVertex2f(-0.84f, 0.66f);
    glVertex2f(-0.8f, 0.7f);

    glVertex2f(-0.8f, 0.7f);
    glVertex2f(-0.78f, 0.68f);

    glVertex2f(-0.78f, 0.68f);
    glVertex2f(-0.73f, 0.73f);

    glVertex2f(-0.73f, 0.73f);
    glVertex2f(-0.68f, 0.64f);

    glVertex2f(-0.68f, 0.64f);
    glVertex2f(-0.65f, 0.67f);

    glVertex2f(-0.65f, 0.67f);
    glVertex2f(-0.58f, 0.58f);

    glVertex2f(-0.58f, 0.58f);
    glVertex2f(-0.55f, 0.6f);

    glVertex2f(-0.55f, 0.6f);
    glVertex2f(-0.5f, 0.5f);


    glEnd();


    glPopMatrix();

//hill6

    glPushMatrix();
    glTranslatef(1.3f,0.0f,0.0f);

    glBegin(GL_POLYGON);
    glColor3f(0.0f,0.3f,0.0f);
    if(night)
    {
        glColor3f(0.00, 0.20, 0.00);
    }

    glVertex2f(-0.65f, 0.5f);
    glVertex2f(-0.63f, 0.55f);

    glVertex2f(-0.63f, 0.55f);
    glVertex2f(-0.61f, 0.53f);

    glVertex2f(-0.61f, 0.53f);
    glVertex2f(-0.55f, 0.65f);

    glVertex2f(-0.55f, 0.65f);
    glVertex2f(-0.52f, 0.63f);

    glVertex2f(-0.52f, 0.63f);
    glVertex2f(-0.5f, 0.68f);

    glVertex2f(-0.5f, 0.68f);
    glVertex2f(-0.47f, 0.66f);

    glVertex2f(-0.47f, 0.66f);
    glVertex2f(-0.44f, 0.73f);

    glVertex2f(-0.44f, 0.73f);
    glVertex2f(-0.4f, 0.7f);

    glVertex2f(-0.4f, 0.7f);
    glVertex2f(-0.35f, 0.62f);

    glVertex2f(-0.35f, 0.62f);
    glVertex2f(-0.3f, 0.66f);

    glVertex2f(-0.3f, 0.66f);
    glVertex2f(-0.28f, 0.63f);

    glVertex2f(-0.28f, 0.63f);
    glVertex2f(-0.24f, 0.69f);

    glVertex2f(-0.24f, 0.69f);
    glVertex2f(-0.18f, 0.58f);

    glVertex2f(-0.18f, 0.58f);
    glVertex2f(-0.15f, 0.6f);

    glVertex2f(-0.15f, 0.6f);
    glVertex2f(-0.11f, 0.55f);

    glVertex2f(-0.11f, 0.55f);
    glVertex2f(-0.1f, 0.5f);

    glEnd();

    glPopMatrix();


//hill2
    glBegin(GL_POLYGON);
    glColor3f(0.0f,0.5f,0.0f);
    if(night)
    {
        glColor3f(0.13, 0.31, 0.13);
    }

    glVertex2f(-0.65f, 0.5f);
    glVertex2f(-0.63f, 0.55f);

    glVertex2f(-0.63f, 0.55f);
    glVertex2f(-0.61f, 0.53f);

    glVertex2f(-0.61f, 0.53f);
    glVertex2f(-0.55f, 0.65f);

    glVertex2f(-0.55f, 0.65f);
    glVertex2f(-0.52f, 0.63f);

    glVertex2f(-0.52f, 0.63f);
    glVertex2f(-0.5f, 0.68f);

    glVertex2f(-0.5f, 0.68f);
    glVertex2f(-0.47f, 0.66f);

    glVertex2f(-0.47f, 0.66f);
    glVertex2f(-0.44f, 0.73f);

    glVertex2f(-0.44f, 0.73f);
    glVertex2f(-0.4f, 0.7f);

    glVertex2f(-0.4f, 0.7f);
    glVertex2f(-0.35f, 0.62f);

    glVertex2f(-0.35f, 0.62f);
    glVertex2f(-0.3f, 0.66f);

    glVertex2f(-0.3f, 0.66f);
    glVertex2f(-0.28f, 0.63f);

    glVertex2f(-0.28f, 0.63f);
    glVertex2f(-0.24f, 0.69f);

    glVertex2f(-0.24f, 0.69f);
    glVertex2f(-0.18f, 0.58f);

    glVertex2f(-0.18f, 0.58f);
    glVertex2f(-0.15f, 0.6f);

    glVertex2f(-0.15f, 0.6f);
    glVertex2f(-0.11f, 0.55f);

    glVertex2f(-0.11f, 0.55f);
    glVertex2f(-0.1f, 0.5f);

    glEnd();

}

void drawClouds()
{
//cloud1........................................................................
    glPushMatrix();
    glTranslatef(run[0],0,0);

    glPushMatrix();
    glTranslatef(0.05f, 0.14f, 0.0f);
    glScalef( 0.7, 0.8,0);

    GLfloat xm1=-0.8f;
    GLfloat ym1=0.9f;
    GLfloat radiusm1 =0.08f;
    glColor3f(0.95f,0.95f,0.95);
    if(night)
    {
        glColor3f(0.5f,0.5f,0.5);
    }
    if(rain && !night)
    {
        glColor3f(0.43, 0.63, 0.85);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xm1, ym1);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xm1 + (radiusm1 * cos(i *  twicePi / triangleAmount)),
                    ym1 + (radiusm1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    GLfloat xm2=-0.72f;
    GLfloat ym2=0.9f;
    glColor3f(0.95f,0.95f,0.95);
    if(night)
    {
        glColor3f(0.5f,0.5f,0.5);
    }
    if(rain && !night)
    {
        glColor3f(0.43, 0.63, 0.85);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xm2, ym2);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xm2 + (radiusm1 * cos(i *  twicePi / triangleAmount)),
                    ym2 + (radiusm1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    GLfloat xm3=-0.77f;
    GLfloat ym3=0.82f;
    GLfloat radiusm2 =0.05f;
    glColor3f(0.95f,0.95f,0.95);
    if(night)
    {
        glColor3f(0.5f,0.5f,0.5);
    }
    if(rain && !night)
    {
        glColor3f(0.43, 0.63, 0.85);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xm3, ym3);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xm3 + (radiusm2 * cos(i *  twicePi / triangleAmount)),
                    ym3 + (radiusm2 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    GLfloat xm4=-0.85f;
    GLfloat ym4=0.82f;
    glColor3f(0.95f,0.95f,0.95);
    if(night)
    {
        glColor3f(0.5f,0.5f,0.5);
    }
    if(rain && !night)
    {
        glColor3f(0.43, 0.63, 0.85);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xm4, ym4);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xm4 + (radiusm2 * cos(i *  twicePi / triangleAmount)),
                    ym4 + (radiusm2 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


    GLfloat xm5=-0.69f;
    GLfloat ym5=0.82f;
    glColor3f(0.95f,0.95f,0.95);
    if(night)
    {
        glColor3f(0.5f,0.5f,0.5);
    }
    if(rain && !night)
    {
        glColor3f(0.43, 0.63, 0.85);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xm5, ym5);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xm5 + (radiusm2 * cos(i *  twicePi / triangleAmount)),
                    ym5 + (radiusm2 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    glPopMatrix();

    glPopMatrix();


    //cloud2....................................................................
    glPushMatrix();
    glTranslatef(run[1],0,0);

    glPushMatrix();
    glTranslatef(0.3f, 0.4f, 0.0f);
    glScalef( 0.6, 0.6,0);

    GLfloat xcl1=0.8f;
    GLfloat ycl1=0.83f;
    GLfloat radiusm3 =0.08f;
    glColor3f(0.95f,0.95f,0.95);
    if(night)
    {
        glColor3f(0.5f,0.5f,0.5);
    }
    if(rain && !night)
    {
        glColor3f(0.43, 0.63, 0.85);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xcl1, ycl1);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xcl1 + (radiusm1 * cos(i *  twicePi / triangleAmount)),
                    ycl1 + (radiusm1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    GLfloat xcl2=0.84f;
    GLfloat ycl2=0.76f;
    GLfloat radiusm4 =0.08f;
    glColor3f(0.95f,0.95f,0.95);
    if(night)
    {
        glColor3f(0.5f,0.5f,0.5);
    }
    if(rain && !night)
    {
        glColor3f(0.43, 0.63, 0.85);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xcl2, ycl2);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xcl2 + (radiusm4 * cos(i *  twicePi / triangleAmount)),
                    ycl2 + (radiusm4 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    GLfloat xcl3=0.725f;
    GLfloat ycl3=0.79f;
    GLfloat radiusm5 =0.092f;
    glColor3f(0.95f,0.95f,0.95);
    if(night)
    {
        glColor3f(0.5f,0.5f,0.5);
    }
    if(rain && !night)
    {
        glColor3f(0.43, 0.63, 0.85);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xcl3, ycl3);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xcl3 + (radiusm5 * cos(i *  twicePi / triangleAmount)),
                    ycl3 + (radiusm5* sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    GLfloat xcl4=0.72f;
    GLfloat ycl4=0.88f;
    GLfloat radiusm6 =0.06f;
    glColor3f(0.95f,0.95f,0.95);
    if(night)
    {
        glColor3f(0.5f,0.5f,0.5);
    }
    if(rain && !night)
    {
        glColor3f(0.43, 0.63, 0.85);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xcl4, ycl4);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xcl4 + (radiusm6* cos(i *  twicePi / triangleAmount)),
                    ycl4 + (radiusm6* sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    glPopMatrix();

}

void drawScene()
{
    // rail line....................................................................................................

    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);

    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.01f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.02f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.03f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.04f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.05f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.06f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.07f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.08f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.09f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.1f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.11f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.12f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.13f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.14f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.15f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.16f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.17f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.18f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.19f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.2f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.21f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.22f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.23f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.24f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.25f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.26f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.27f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.28f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.29f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.3f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.31f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.32f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.33f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.34f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.35f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.36f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.37f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.38f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.39f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.4f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.41f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.42f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.43f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.44f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.45f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.46f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.47f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.48f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.49f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.5f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.51f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.52f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.53f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.54f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.55f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.56f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.57f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.58f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.59f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.6f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.61f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.62f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.63f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.64f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.65f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.66f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.67f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.68f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.69f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.7f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.71f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.72f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.73f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.74f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.75f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.76f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.77f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.78f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.79f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.8f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.81f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.82f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.83f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.84f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.85f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.86f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.87f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.88f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.89f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.9f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.91f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.92f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.93f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.94f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.95f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.96f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.97f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.98f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.99f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    //again

    glPushMatrix();
    glTranslatef(1.0f, 0.0f, 0.0f);

    glPushMatrix();
    glTranslatef(0.01f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.02f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.03f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.04f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.05f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.06f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.07f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.08f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.09f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.1f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.11f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.12f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.13f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.14f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.15f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.16f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.17f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.18f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.19f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.2f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.21f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.22f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.23f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.24f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.25f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.26f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.27f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.28f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.29f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.3f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.31f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.32f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.33f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.34f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.35f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.36f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.37f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.38f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.39f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.4f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.41f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.42f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.43f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.44f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.45f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.46f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.47f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.48f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.49f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.5f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.51f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.52f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.53f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.54f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.55f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.56f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.57f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.58f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.59f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.6f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.61f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.62f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.63f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.64f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.65f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.66f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.67f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.68f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.69f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.7f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.71f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.72f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.73f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.74f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.75f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.76f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.77f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.78f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.79f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.8f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.81f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.82f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.83f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.84f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.85f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.86f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.87f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.88f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.89f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.9f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.91f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.92f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.93f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.94f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.95f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.96f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.97f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.98f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.99f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(-1.0f, 0.45f);
    glVertex2f(-0.995f, 0.45f);
    glVertex2f(-0.995f, 0.5f);
    glVertex2f(-1.0f, 0.5f);
    glEnd();
    glPopMatrix();

    glPopMatrix();


    glBegin(GL_QUADS);
    glColor3f(1.0f,1.0f,1.0f);
    glVertex2f(-1.0f, 0.467f);
    glVertex2f(1.0f,0.467f);
    glVertex2f(1.0f,0.46f);
    glVertex2f(-1.0f,0.46f);

    glVertex2f(-1.0f, 0.485f);
    glVertex2f(1.0f,0.485f);
    glVertex2f(1.0f, 0.49f);
    glVertex2f(-1.0f, 0.49f);

    glEnd();


    // rail line end /////////////////////////////////////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<??????????

    //rail start/////////////////////.......................


    glPushMatrix();
    glTranslatef(run[2],0,0);

    glBegin(GL_QUADS);
    glColor3f(1.0f,0.4f,0.0f);
    glVertex2f(0.02f,0.51f);
    glVertex2f(0.105f,0.51f);
    glVertex2f(0.105f,0.46f);
    glVertex2f(0.0f,0.46f);

    glColor3f(0.0f,0.3f,0.6f);
    glVertex2f(0.02f,0.505f);
    glVertex2f(0.1f,0.505f);
    glVertex2f(0.1f,0.5f);
    glVertex2f(0.02f,0.5f);

    glColor3f(1.0f,1.0f,1.0f);
    glVertex2f(0.02f,0.476f);
    glVertex2f(0.1f,0.476f);
    glVertex2f(0.1f,0.465f);
    glVertex2f(0.02f,0.465f);

    glColor3f(0.0f,0.3f,0.6f);
    glVertex2f(0.025f,0.471f);
    glVertex2f(0.1f,0.471f);
    glVertex2f(0.1f,0.465f);
    glVertex2f(0.021f,0.465f);
//..
    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.012f,0.495f);
    glVertex2f(0.019f,0.495f);
    glVertex2f(0.019f,0.48f);
    glVertex2f(0.007f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.022f,0.495f);
    glVertex2f(0.03f,0.495f);
    glVertex2f(0.03f,0.48f);
    glVertex2f(0.022f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.032f,0.495f);
    glVertex2f(0.04f,0.495f);
    glVertex2f(0.04f,0.48f);
    glVertex2f(0.032f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.042f,0.495f);
    glVertex2f(0.05f,0.495f);
    glVertex2f(0.05f,0.48f);
    glVertex2f(0.042f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.052f,0.495f);
    glVertex2f(0.06f,0.495f);
    glVertex2f(0.06f,0.48f);
    glVertex2f(0.052f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.062f,0.495f);
    glVertex2f(0.07f,0.495f);
    glVertex2f(0.07f,0.48f);
    glVertex2f(0.062f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.072f,0.495f);
    glVertex2f(0.08f,0.495f);
    glVertex2f(0.08f,0.48f);
    glVertex2f(0.072f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.082f,0.495f);
    glVertex2f(0.09f,0.495f);
    glVertex2f(0.09f,0.48f);
    glVertex2f(0.082f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.092f,0.495f);
    glVertex2f(0.099f,0.495f);
    glVertex2f(0.099f,0.48f);
    glVertex2f(0.092f,0.48f);

    glEnd();


    glBegin(GL_QUADS);
    glColor3f(1.0f,0.4f,0.0f);
    glVertex2f(0.205f,0.51f);
    glVertex2f(0.102f,0.51f);
    glVertex2f(0.102f,0.46f);
    glVertex2f(0.205f,0.46f);

    glColor3f(0.0f,0.3f,0.6f);
    glVertex2f(0.199f,0.505f);
    glVertex2f(0.102f,0.505f);
    glVertex2f(0.102f,0.5f);
    glVertex2f(0.199f,0.5f);


    glColor3f(1.0f,1.0f,1.0f);
    glVertex2f(0.11f,0.465f);
    glVertex2f(0.105f,0.465f);
    glVertex2f(0.105f,0.5f);
    glVertex2f(0.11f,0.5f);

    glColor3f(1.0f,1.0f,1.0f);
    glVertex2f(0.199f,0.476f);
    glVertex2f(0.105f,0.476f);
    glVertex2f(0.105f,0.465f);
    glVertex2f(0.199f,0.465f);

    glColor3f(0.0f,0.3f,0.6f);
    glVertex2f(0.11f,0.471f);
    glVertex2f(0.199f,0.471f);
    glVertex2f(0.199f,0.465f);
    glVertex2f(0.107f,0.465f);

//windus
    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.12f,0.495f);
    glVertex2f(0.112f,0.495f);
    glVertex2f(0.112f,0.48f);
    glVertex2f(0.12f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.13f,0.495f);
    glVertex2f(0.122f,0.495f);
    glVertex2f(0.122f,0.48f);
    glVertex2f(0.13f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.14f,0.495f);
    glVertex2f(0.132f,0.495f);
    glVertex2f(0.132f,0.48f);
    glVertex2f(0.14f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.15f,0.495f);
    glVertex2f(0.142f,0.495f);
    glVertex2f(0.142f,0.48f);
    glVertex2f(0.15f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.16f,0.495f);
    glVertex2f(0.152f,0.495f);
    glVertex2f(0.152f,0.48f);
    glVertex2f(0.16f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.17f,0.495f);
    glVertex2f(0.162f,0.495f);
    glVertex2f(0.162f,0.48f);
    glVertex2f(0.17f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.18f,0.495f);
    glVertex2f(0.172f,0.495f);
    glVertex2f(0.172f,0.48f);
    glVertex2f(0.18f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.19f,0.495f);
    glVertex2f(0.182f,0.495f);
    glVertex2f(0.182f,0.48f);
    glVertex2f(0.19f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.1985f,0.495f);
    glVertex2f(0.192f,0.495f);
    glVertex2f(0.192f,0.48f);
    glVertex2f(0.1985f,0.48f);


    glEnd();


    glBegin(GL_QUADS);
    glColor3f(1.0f,0.4f,0.0f);
    glVertex2f(0.305f,0.51f);
    glVertex2f(0.202f,0.51f);
    glVertex2f(0.202f,0.46f);
    glVertex2f(0.305f,0.46f);

    glColor3f(0.0f,0.3f,0.6f);
    glVertex2f(0.3f,0.505f);
    glVertex2f(0.202f,0.505f);
    glVertex2f(0.202f,0.5f);
    glVertex2f(0.3f,0.5f);


    glColor3f(1.0f,1.0f,1.0f);
    glVertex2f(0.21f,0.465f);
    glVertex2f(0.205f,0.465f);
    glVertex2f(0.205f,0.5f);
    glVertex2f(0.21f,0.5f);

    glColor3f(1.0f,1.0f,1.0f);
    glVertex2f(0.3f,0.476f);
    glVertex2f(0.205f,0.476f);
    glVertex2f(0.205f,0.465f);
    glVertex2f(0.3f,0.465f);

    glColor3f(0.0f,0.3f,0.6f);
    glVertex2f(0.21f,0.471f);
    glVertex2f(0.3f,0.471f);
    glVertex2f(0.3f,0.465f);
    glVertex2f(0.207f,0.465f);

    //..
    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.22f,0.495f);
    glVertex2f(0.212f,0.495f);
    glVertex2f(0.212f,0.48f);
    glVertex2f(0.22f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.23f,0.495f);
    glVertex2f(0.222f,0.495f);
    glVertex2f(0.222f,0.48f);
    glVertex2f(0.23f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.24f,0.495f);
    glVertex2f(0.232f,0.495f);
    glVertex2f(0.232f,0.48f);
    glVertex2f(0.24f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.25f,0.495f);
    glVertex2f(0.242f,0.495f);
    glVertex2f(0.242f,0.48f);
    glVertex2f(0.25f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.26f,0.495f);
    glVertex2f(0.252f,0.495f);
    glVertex2f(0.252f,0.48f);
    glVertex2f(0.26f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.27f,0.495f);
    glVertex2f(0.262f,0.495f);
    glVertex2f(0.262f,0.48f);
    glVertex2f(0.27f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.28f,0.495f);
    glVertex2f(0.272f,0.495f);
    glVertex2f(0.272f,0.48f);
    glVertex2f(0.28f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.29f,0.495f);
    glVertex2f(0.282f,0.495f);
    glVertex2f(0.282f,0.48f);
    glVertex2f(0.29f,0.48f);

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.3f,0.495f);
    glVertex2f(0.292f,0.495f);
    glVertex2f(0.292f,0.48f);
    glVertex2f(0.3f,0.48f);

    glEnd();

    glPopMatrix();

    //rail end /////////////////////.......................


    //forest<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

//////////////tree 01 copy.01..................

    glPushMatrix();
    glTranslatef(1.f, 0.2f, 0.0f);
    glScalef( 0.5, 0.5,0);
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex2f(-0.64f, 0.2f);
    glVertex2f(-0.64f, 0.3f);
    glVertex2f(-0.65f, 0.3f);
    glVertex2f(-0.65f, 0.2f);
    glEnd();


    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPushMatrix();
    glTranslatef(-0.01f, 0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.02f, 0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.01f, 0.02f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();


    GLfloat xf1=-0.645f;
    GLfloat yf1=0.3f;
    GLfloat radiusf1 =0.039f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf1, yf1);
    GLfloat twice1Pi = 1.0f * PI;
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf1 + (radiusf1 * cos(i *  twice1Pi / triangleAmount)),
                    yf1 + (radiusf1 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();


    GLfloat xf2=-0.645f;
    GLfloat yf2=0.33f;
    GLfloat radiusf2 =0.034f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf2, yf2);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf2 + (radiusf2 * cos(i *  twice1Pi / triangleAmount)),
                    yf2 + (radiusf2 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();

    GLfloat xf3=-0.645f;
    GLfloat yf3=0.36f;
    GLfloat radiusf3 =0.029f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf3, yf3);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf3 + (radiusf3 * cos(i *  twice1Pi / triangleAmount)),
                    yf3 + (radiusf3 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();

    glLoadIdentity();

    glPopMatrix();


    //////////////tree 01 copy.02..................

    glPushMatrix();
    glTranslatef(1.01f, 0.15f, 0.0f);
    glScalef( 0.5, 0.5,0);
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex2f(-0.64f, 0.2f);
    glVertex2f(-0.64f, 0.3f);
    glVertex2f(-0.65f, 0.3f);
    glVertex2f(-0.65f, 0.2f);
    glEnd();


    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPushMatrix();
    glTranslatef(-0.01f, 0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.02f, 0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.01f, 0.02f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();


    GLfloat xf4=-0.645f;
    GLfloat yf4=0.3f;
    GLfloat radiusf4 =0.039f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf4, yf4);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf4 + (radiusf4 * cos(i *  twice1Pi / triangleAmount)),
                    yf4 + (radiusf4 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();


    GLfloat xf5=-0.645f;
    GLfloat yf5=0.33f;
    GLfloat radiusf5 =0.034f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf5, yf5);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf5 + (radiusf5 * cos(i *  twice1Pi / triangleAmount)),
                    yf5 + (radiusf5 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();

    GLfloat xf6=-0.645f;
    GLfloat yf6=0.36f;
    GLfloat radiusf6 =0.029f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf6, yf6);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf6 + (radiusf6 * cos(i *  twice1Pi / triangleAmount)),
                    yf6 + (radiusf6 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();

    glLoadIdentity();

    glPopMatrix();


    //////////////tree 01 copy.03..................

    glPushMatrix();
    glTranslatef( 0.95f, 0.35f, 0.0f);
    glScalef( 0.3, 0.3,0);
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex2f(-0.64f, 0.2f);
    glVertex2f(-0.64f, 0.3f);
    glVertex2f(-0.65f, 0.3f);
    glVertex2f(-0.65f, 0.2f);
    glEnd();


    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPushMatrix();
    glTranslatef(-0.01f, 0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.02f, 0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.01f, 0.02f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();


    GLfloat xf7=-0.645f;
    GLfloat yf7=0.3f;
    GLfloat radiusf7 =0.039f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf7, yf7);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf7 + (radiusf7 * cos(i *  twice1Pi / triangleAmount)),
                    yf7 + (radiusf7 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();


    GLfloat xf8=-0.645f;
    GLfloat yf8=0.33f;
    GLfloat radiusf8 =0.034f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf8, yf8);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf8 + (radiusf8 * cos(i *  twice1Pi / triangleAmount)),
                    yf8 + (radiusf8 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();

    GLfloat xf9=-0.645f;
    GLfloat yf9=0.36f;
    GLfloat radiusf9 =0.029f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf9, yf9);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf9 + (radiusf9 * cos(i *  twice1Pi / triangleAmount)),
                    yf9 + (radiusf9 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();

    glLoadIdentity();

    glPopMatrix();


    //////////////tree 01 copy.04..................

    glPushMatrix();
    glTranslatef(1.12f, 0.35f, 0.0f);
    glScalef( 0.3, 0.3,0);
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex2f(-0.64f, 0.2f);
    glVertex2f(-0.64f, 0.3f);
    glVertex2f(-0.65f, 0.3f);
    glVertex2f(-0.65f, 0.2f);
    glEnd();


    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPushMatrix();
    glTranslatef(-0.01f, 0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.02f, 0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.01f, 0.02f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();


    GLfloat xf10=-0.645f;
    GLfloat yf10=0.3f;
    GLfloat radiusf10 =0.039f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf10, yf10);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf10 + (radiusf10* cos(i *  twice1Pi / triangleAmount)),
                    yf10 + (radiusf10 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();


    GLfloat xf11=-0.645f;
    GLfloat yf11=0.33f;
    GLfloat radiusf11 =0.034f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf11, yf11);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf11 + (radiusf11 * cos(i *  twice1Pi / triangleAmount)),
                    yf11 + (radiusf11 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();

    GLfloat xf12=-0.645f;
    GLfloat yf12=0.36f;
    GLfloat radiusf12 =0.029f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf12, yf12);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf12 + (radiusf12 * cos(i *  twice1Pi / triangleAmount)),
                    yf12 + (radiusf12 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();

    glLoadIdentity();

    glPopMatrix();


    //////////////tree 01 copy.05..................

    glPushMatrix();
    glTranslatef(1.16f, 0.35f, 0.0f);
    glScalef( 0.3, 0.3,0);
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex2f(-0.64f, 0.2f);
    glVertex2f(-0.64f, 0.3f);
    glVertex2f(-0.65f, 0.3f);
    glVertex2f(-0.65f, 0.2f);
    glEnd();


    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPushMatrix();
    glTranslatef(-0.01f, 0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.02f, 0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.01f, 0.02f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();


    GLfloat xf13=-0.645f;
    GLfloat yf13=0.3f;
    GLfloat radiusf13 =0.039f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf13, yf13);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf13 + (radiusf13* cos(i *  twice1Pi / triangleAmount)),
                    yf13 + (radiusf13 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();


    GLfloat xf14=-0.645f;
    GLfloat yf14=0.33f;
    GLfloat radiusf14 =0.034f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf14, yf14);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf14 + (radiusf14 * cos(i *  twice1Pi / triangleAmount)),
                    yf14 + (radiusf14 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();

    GLfloat xf15=-0.645f;
    GLfloat yf15=0.36f;
    GLfloat radiusf15 =0.029f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xf15, yf15);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xf15 + (radiusf15 * cos(i *  twice1Pi / triangleAmount)),
                    yf15 + (radiusf15 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();

    glLoadIdentity();

    glPopMatrix();


    // house01......................................................................................................


    glBegin(GL_QUADS);
    glColor3f(0.6f,0.6f,0.6f);

    glVertex2f(-0.86f, 0.48f);
    glVertex2f(-0.88f, 0.48f);
    glVertex2f(-0.88f, 0.3f);
    glVertex2f(-0.86f, 0.3f);

    glColor3f(0.2f,0.0f,0.0f);

    glVertex2f(-0.855f, 0.48f);
    glVertex2f(-0.885f, 0.48f);
    glVertex2f(-0.885f, 0.495f);
    glVertex2f(-0.855f, 0.495f);


    glEnd();

    glBegin(GL_POLYGON);

    glColor3f(0.6f,0.6f,0.6f);
    glVertex2f(-0.9f, 0.2f);
    glVertex2f(-0.9f, 0.4f);
    glVertex2f(-0.8f, 0.5f);
    glVertex2f(-0.7f, 0.4f);
    glVertex2f(-0.7f, 0.2f);


    glEnd();

    glBegin(GL_QUADS);

    glColor3f(0.2f,0.0f,0.0f);

    glVertex2f(-0.69f, 0.39f);
    glVertex2f(-0.8f, 0.5f);
    glVertex2f(-0.8f, 0.52f);
    glVertex2f(-0.68f, 0.4f);

    glVertex2f(-0.91f, 0.39f);
    glVertex2f(-0.8f, 0.5f);
    glVertex2f(-0.8f, 0.52f);
    glVertex2f(-0.92f, 0.4f);
    glEnd();

//windos...................................................................................................
    glBegin(GL_QUADS);
    glColor3f(1.0f,1.0f,1.0f);

    glVertex2f(-0.808f, 0.43f);
    glVertex2f(-0.792f, 0.43f);
    glVertex2f(-0.792f, 0.46f);
    glVertex2f(-0.808f, 0.46f);

    glColor3f(0.6f,0.8f,1.0f);

    glVertex2f(-0.805f, 0.435f);
    glVertex2f(-0.795f, 0.435f);
    glVertex2f(-0.795f, 0.455f);
    glVertex2f(-0.805f, 0.455f);

    glEnd();

    glBegin(GL_QUADS);

    glColor3f(1.0f,1.0f,1.0f);

    glVertex2f(-0.78f, 0.32f);
    glVertex2f(-0.82f, 0.32f);
    glVertex2f(-0.82f, 0.38f);
    glVertex2f(-0.78f, 0.38f);

    glColor3f(0.2f,0.0f,0.0f);

    glVertex2f(-0.775f, 0.39f);
    glVertex2f(-0.825f, 0.39f);
    glVertex2f(-0.825f, 0.38f);
    glVertex2f(-0.775f, 0.38f);


    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }

    glVertex2f(-0.785f, 0.325f);
    glVertex2f(-0.795f, 0.325f);
    glVertex2f(-0.795f, 0.345f);
    glVertex2f(-0.785f, 0.345f);

    glVertex2f(-0.785f, 0.35f);
    glVertex2f(-0.795f, 0.35f);
    glVertex2f(-0.795f, 0.37f);
    glVertex2f(-0.785f, 0.37f);

    glVertex2f(-0.815f, 0.325f);
    glVertex2f(-0.805f, 0.325f);
    glVertex2f(-0.805f, 0.345f);
    glVertex2f(-0.815f, 0.345f);

    glVertex2f(-0.815f, 0.35f);
    glVertex2f(-0.805f, 0.35f);
    glVertex2f(-0.805f, 0.37f);
    glVertex2f(-0.815f, 0.37f);


    glEnd();
    glLineWidth(1);
    glBegin(GL_LINES);
    glVertex2f(-0.80f, 0.32f);
    glVertex2f(-0.80f, 0.38f);
    glEnd();


    //copy windos.............................................................

    glPushMatrix();
    glTranslatef(-0.06f, 0.0f, 0.0f);

    glBegin(GL_QUADS);

    glColor3f(1.0f,1.0f,1.0f);

    glVertex2f(-0.78f, 0.32f);
    glVertex2f(-0.82f, 0.32f);
    glVertex2f(-0.82f, 0.38f);
    glVertex2f(-0.78f, 0.38f);

    glColor3f(0.2f,0.0f,0.0f);

    glVertex2f(-0.775f, 0.39f);
    glVertex2f(-0.825f, 0.39f);
    glVertex2f(-0.825f, 0.38f);
    glVertex2f(-0.775f, 0.38f);


    glColor3f(0.6f,0.8f,1.0f);

    glVertex2f(-0.785f, 0.325f);
    glVertex2f(-0.795f, 0.325f);
    glVertex2f(-0.795f, 0.345f);
    glVertex2f(-0.785f, 0.345f);

    glVertex2f(-0.785f, 0.35f);
    glVertex2f(-0.795f, 0.35f);
    glVertex2f(-0.795f, 0.37f);
    glVertex2f(-0.785f, 0.37f);

    glVertex2f(-0.815f, 0.325f);
    glVertex2f(-0.805f, 0.325f);
    glVertex2f(-0.805f, 0.345f);
    glVertex2f(-0.815f, 0.345f);

    glVertex2f(-0.815f, 0.35f);
    glVertex2f(-0.805f, 0.35f);
    glVertex2f(-0.805f, 0.37f);
    glVertex2f(-0.815f, 0.37f);


    glEnd();
    glLineWidth(1);
    glBegin(GL_LINES);
    glVertex2f(-0.80f, 0.32f);
    glVertex2f(-0.80f, 0.38f);
    glEnd();

    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.06f, 0.0f, 0.0f);

    glBegin(GL_QUADS);

    glColor3f(1.0f,1.0f,1.0f);

    glVertex2f(-0.78f, 0.32f);
    glVertex2f(-0.82f, 0.32f);
    glVertex2f(-0.82f, 0.38f);
    glVertex2f(-0.78f, 0.38f);

    glColor3f(0.2f,0.0f,0.0f);

    glVertex2f(-0.775f, 0.39f);
    glVertex2f(-0.825f, 0.39f);
    glVertex2f(-0.825f, 0.38f);
    glVertex2f(-0.775f, 0.38f);


    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }

    glVertex2f(-0.785f, 0.325f);
    glVertex2f(-0.795f, 0.325f);
    glVertex2f(-0.795f, 0.345f);
    glVertex2f(-0.785f, 0.345f);

    glVertex2f(-0.785f, 0.35f);
    glVertex2f(-0.795f, 0.35f);
    glVertex2f(-0.795f, 0.37f);
    glVertex2f(-0.785f, 0.37f);

    glVertex2f(-0.815f, 0.325f);
    glVertex2f(-0.805f, 0.325f);
    glVertex2f(-0.805f, 0.345f);
    glVertex2f(-0.815f, 0.345f);

    glVertex2f(-0.815f, 0.35f);
    glVertex2f(-0.805f, 0.35f);
    glVertex2f(-0.805f, 0.37f);
    glVertex2f(-0.815f, 0.37f);


    glEnd();
    glLineWidth(1);
    glBegin(GL_LINES);
    glVertex2f(-0.80f, 0.32f);
    glVertex2f(-0.80f, 0.38f);
    glEnd();

    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.1f, 0.0f);

    glBegin(GL_QUADS);

    glColor3f(1.0f,1.0f,1.0f);

    glVertex2f(-0.78f, 0.32f);
    glVertex2f(-0.82f, 0.32f);
    glVertex2f(-0.82f, 0.38f);
    glVertex2f(-0.78f, 0.38f);

    glColor3f(0.2f,0.0f,0.0f);

    glVertex2f(-0.775f, 0.39f);
    glVertex2f(-0.825f, 0.39f);
    glVertex2f(-0.825f, 0.38f);
    glVertex2f(-0.775f, 0.38f);


    glColor3f(0.6f,0.8f,1.0f);

    glVertex2f(-0.785f, 0.325f);
    glVertex2f(-0.795f, 0.325f);
    glVertex2f(-0.795f, 0.345f);
    glVertex2f(-0.785f, 0.345f);

    glVertex2f(-0.785f, 0.35f);
    glVertex2f(-0.795f, 0.35f);
    glVertex2f(-0.795f, 0.37f);
    glVertex2f(-0.785f, 0.37f);

    glVertex2f(-0.815f, 0.325f);
    glVertex2f(-0.805f, 0.325f);
    glVertex2f(-0.805f, 0.345f);
    glVertex2f(-0.815f, 0.345f);

    glVertex2f(-0.815f, 0.35f);
    glVertex2f(-0.805f, 0.35f);
    glVertex2f(-0.805f, 0.37f);
    glVertex2f(-0.815f, 0.37f);


    glEnd();
    glLineWidth(1);
    glBegin(GL_LINES);
    glVertex2f(-0.80f, 0.32f);
    glVertex2f(-0.80f, 0.38f);
    glEnd();

    glPopMatrix();


    glPushMatrix();
    glTranslatef(-0.06f, -0.1f, 0.0f);

    glBegin(GL_QUADS);

    glColor3f(1.0f,1.0f,1.0f);

    glVertex2f(-0.78f, 0.32f);
    glVertex2f(-0.82f, 0.32f);
    glVertex2f(-0.82f, 0.38f);
    glVertex2f(-0.78f, 0.38f);

    glColor3f(0.2f,0.0f,0.0f);

    glVertex2f(-0.775f, 0.39f);
    glVertex2f(-0.825f, 0.39f);
    glVertex2f(-0.825f, 0.38f);
    glVertex2f(-0.775f, 0.38f);


    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }

    glVertex2f(-0.785f, 0.325f);
    glVertex2f(-0.795f, 0.325f);
    glVertex2f(-0.795f, 0.345f);
    glVertex2f(-0.785f, 0.345f);

    glVertex2f(-0.785f, 0.35f);
    glVertex2f(-0.795f, 0.35f);
    glVertex2f(-0.795f, 0.37f);
    glVertex2f(-0.785f, 0.37f);

    glVertex2f(-0.815f, 0.325f);
    glVertex2f(-0.805f, 0.325f);
    glVertex2f(-0.805f, 0.345f);
    glVertex2f(-0.815f, 0.345f);

    glVertex2f(-0.815f, 0.35f);
    glVertex2f(-0.805f, 0.35f);
    glVertex2f(-0.805f, 0.37f);
    glVertex2f(-0.815f, 0.37f);


    glEnd();
    glLineWidth(1);
    glBegin(GL_LINES);
    glVertex2f(-0.80f, 0.32f);
    glVertex2f(-0.80f, 0.38f);
    glEnd();

    glPopMatrix();


//door

    glBegin(GL_QUADS);

    glColor3f(1.0f,1.0f,1.0f);

    glVertex2f(-0.76f, 0.2f);
    glVertex2f(-0.72f, 0.2f);
    glVertex2f(-0.72f, 0.28f);
    glVertex2f(-0.76f, 0.28f);


    glColor3f(0.6f,0.8f,1.0f);

    glVertex2f(-0.755f, 0.21f);
    glVertex2f(-0.742f, 0.21f);
    glVertex2f(-0.742f, 0.27f);
    glVertex2f(-0.755f, 0.27f);

    glVertex2f(-0.738f, 0.21f);
    glVertex2f(-0.725f, 0.21f);
    glVertex2f(-0.725f, 0.27f);
    glVertex2f(-0.738f, 0.27f);


    glEnd();

    glBegin(GL_QUADS);


    glColor3f(0.8f,0.8f,0.8f);

    glVertex2f(-0.76f, 0.2f);
    glVertex2f(-0.72f, 0.2f);
    glVertex2f(-0.72f, 0.19f);
    glVertex2f(-0.76f, 0.19f);

    glVertex2f(-0.755f, 0.17f);
    glVertex2f(-0.725f, 0.17f);
    glVertex2f(-0.725f, 0.19f);
    glVertex2f(-0.755f, 0.19f);
    glEnd();


    // tree2 copy.........................................//


    glPushMatrix();
    glTranslatef(0.06f, -0.01f, 0.0f);
    float run[2] = 0.0;
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex2f(-0.96f, 0.2f);
    glVertex2f(-0.96f, 0.3f);
    glVertex2f(-0.95f, 0.3f);
    glVertex2f(-0.95f, 0.2f);
    glEnd();

    GLfloat xt10=-0.955f;
    GLfloat yt10=0.3f;
    GLfloat radiust10 =0.03f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xt10, yt10);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xt10 + (radiust10* cos(i *  twicePi / triangleAmount)),
                    yt10 + (radiust10 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    GLfloat xt11=-0.955f;
    GLfloat yt11=0.33f;
    GLfloat radiust11 =0.025f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xt11, yt11);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xt11 + (radiust11 * cos(i *  twicePi / triangleAmount)),
                    yt11 + (radiust11 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    GLfloat xt12=-0.955f;
    GLfloat yt12=0.35f;
    GLfloat radiust12 =0.02f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xt12, yt12);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xt12 + (radiust12 * cos(i *  twicePi / triangleAmount)),
                    yt12 + (radiust12 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    glPopMatrix();


    glBegin(GL_QUADS);


    glColor3f(0.6f,0.28f,0.0f);

    glVertex2f(-0.93f, 0.23f);
    glVertex2f(-0.77f, 0.23f);
    glVertex2f(-0.77f, 0.19f);
    glVertex2f(-0.93f, 0.19f);

    glVertex2f(-0.71f, 0.23f);
    glVertex2f(-0.67f, 0.23f);
    glVertex2f(-0.67f, 0.19f);
    glVertex2f(-0.71f, 0.19f);


    glColor3f(0.8f,0.4f,0.0f);

    glVertex2f(-0.76f, 0.24f);
    glVertex2f(-0.77f, 0.24f);
    glVertex2f(-0.77f, 0.19f);
    glVertex2f(-0.76f, 0.19f);

    glVertex2f(-0.71f, 0.24f);
    glVertex2f(-0.72f, 0.24f);
    glVertex2f(-0.72f, 0.19f);
    glVertex2f(-0.71f, 0.19f);


    glEnd();


//house 2.............................................................................................................

    glBegin(GL_QUADS);
    glColor3f(0.05, 0.30, 0.50);
    glVertex2f(-0.47f, 0.6f);
    glVertex2f(-0.47f, 0.2f);
    glVertex2f(-0.6f, 0.2f);
    glVertex2f(-0.6f, 0.6f);
    glEnd();


    glBegin(GL_QUADS);
    glColor3f(0.00, 0.18, 0.34);
    glVertex2f(-0.47f, 0.47f);
    glVertex2f(-0.4f, 0.35f);
    glVertex2f(-0.4f, 0.2f);
    glVertex2f(-0.47f, 0.2f);
    glEnd();

    glLineWidth(6);
    glBegin(GL_LINES);
    glColor3f(0.04, 0.31, 0.52);
    glVertex2f(- 0.472, 0.47);
    glVertex2f(- 0.39, 0.34);
    glEnd();

    glLineWidth(6);
    glBegin(GL_LINES);
    glColor3f(0.05, 0.20, 0.34);
    glVertex2f(- 0.61, 0.60);
    glVertex2f(- 0.46, 0.60);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.95, 0.98, 0.98);
    glVertex2f(-0.53f, 0.56f);
    glVertex2f(-0.53f, 0.5f);
    glVertex2f(-0.57f, 0.5f);
    glVertex2f(-0.57f, 0.56f);
    glEnd();

    glLineWidth(6);
    glBegin(GL_LINES);
    glColor3f(0.00, 0.20, 0.35);
    glVertex2f(- 0.521, 0.57);
    glVertex2f(- 0.579, 0.57);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.95, 0.98, 0.98);
    glVertex2f(-0.5f, 0.56f);
    glVertex2f(-0.5f, 0.52f);
    glVertex2f(-0.485f, 0.52f);
    glVertex2f(-0.485f, 0.56f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.38, 0.68, 0.78);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.497f, 0.557f);
    glVertex2f(-0.497f, 0.523f);
    glVertex2f(-0.487f, 0.523);
    glVertex2f(-0.487f, 0.557f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.38, 0.68, 0.78);
    glVertex2f(-0.532f, 0.557f);
    glVertex2f(-0.532f, 0.503f);
    glVertex2f(-0.549f, 0.503);
    glVertex2f(-0.549f, 0.557f);

    glVertex2f(-0.552f, 0.557f);
    glVertex2f(-0.552f, 0.503f);
    glVertex2f(-0.568f, 0.503);
    glVertex2f(-0.568f, 0.557f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.62, 0.67, 0.73);
    glVertex2f(-0.52f, 0.51f);
    glVertex2f(-0.52f, 0.475f);
    glVertex2f(-0.58f, 0.475);
    glVertex2f(-0.58f, 0.51f);
    glEnd();


    glBegin(GL_QUADS);
    glColor3f(0.95, 0.98, 0.98);
    glVertex2f(-0.53f, 0.3f);
    glVertex2f(-0.53f, 0.24f);
    glVertex2f(-0.57f, 0.24f);
    glVertex2f(-0.57f, 0.3f);
    glEnd();

    glLineWidth(6);
    glBegin(GL_LINES);
    glColor3f(0.00, 0.20, 0.35);
    glVertex2f(- 0.521, 0.31);
    glVertex2f(- 0.579, 0.31);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.95, 0.98, 0.98);
    glVertex2f(-0.5f, 0.3f);
    glVertex2f(-0.5f, 0.26f);
    glVertex2f(-0.485f, 0.26f);
    glVertex2f(-0.485f, 0.3f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.38, 0.68, 0.78);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.497f, 0.297f);
    glVertex2f(-0.497f, 0.263f);
    glVertex2f(-0.487f, 0.263);
    glVertex2f(-0.487f, 0.297f);
    glEnd();


    glBegin(GL_QUADS);
    glColor3f(0.38, 0.68, 0.78);
    glVertex2f(-0.532f, 0.297f);
    glVertex2f(-0.532f, 0.243f);
    glVertex2f(-0.549f, 0.243);
    glVertex2f(-0.549f, 0.297f);

    glVertex2f(-0.552f, 0.297f);
    glVertex2f(-0.552f, 0.243f);
    glVertex2f(-0.568f, 0.243);
    glVertex2f(-0.568f, 0.297f);

    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.62, 0.67, 0.73);
    glVertex2f(-0.52f, 0.25f);
    glVertex2f(-0.52f, 0.215f);
    glVertex2f(-0.58f, 0.215);
    glVertex2f(-0.58f, 0.25f);
    glEnd();


    glBegin(GL_QUADS);
    glColor3f(0.95, 0.98, 0.98);
    glVertex2f(-0.53f, 0.43f);
    glVertex2f(-0.53f, 0.37f);
    glVertex2f(-0.57f, 0.37f);
    glVertex2f(-0.57f, 0.43f);
    glEnd();

    glLineWidth(6);
    glBegin(GL_LINES);
    glColor3f(0.00, 0.20, 0.35);
    glVertex2f(- 0.521, 0.44);
    glVertex2f(- 0.579, 0.44);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.95, 0.98, 0.98);
    glVertex2f(-0.5f, 0.43f);
    glVertex2f(-0.5f, 0.39f);
    glVertex2f(-0.485f, 0.39f);
    glVertex2f(-0.485f, 0.43f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.38, 0.68, 0.78);
    glVertex2f(-0.497f, 0.427f);
    glVertex2f(-0.497f, 0.393f);
    glVertex2f(-0.487f, 0.393);
    glVertex2f(-0.487f, 0.427f);
    glEnd();


    glBegin(GL_QUADS);
    glColor3f(0.38, 0.68, 0.78);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.532f, 0.427f);
    glVertex2f(-0.532f, 0.373f);
    glVertex2f(-0.549f, 0.373);
    glVertex2f(-0.549f, 0.427f);

    glVertex2f(-0.552f, 0.427f);
    glVertex2f(-0.552f, 0.373f);
    glVertex2f(-0.568f, 0.373);
    glVertex2f(-0.568f, 0.427f);

    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.62, 0.67, 0.73);
    glVertex2f(-0.52f, 0.38f);
    glVertex2f(-0.52f, 0.345f);
    glVertex2f(-0.58f, 0.345);
    glVertex2f(-0.58f, 0.38f);
    glEnd();


    glBegin(GL_QUADS);
    glColor3f(0.95, 0.98, 0.98);
    glVertex2f(-0.428f, 0.37f);
    glVertex2f(-0.428f, 0.32f);
    glVertex2f(-0.453f, 0.32f);
    glVertex2f(-0.453f, 0.37f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.38, 0.68, 0.78);
    glVertex2f(-0.431f, 0.365f);
    glVertex2f(-0.431f, 0.323f);
    glVertex2f(-0.450f, 0.323);
    glVertex2f(-0.450f, 0.365f);
    glEnd();


    glBegin(GL_QUADS);
    glColor3f(0.95, 0.98, 0.98);
    glVertex2f(-0.425f, 0.3);
    glVertex2f(-0.425f, 0.2f);
    glVertex2f(-0.455f, 0.2);
    glVertex2f(-0.455f, 0.3);
    glEnd();

    glLineWidth(4);
    glBegin(GL_LINES);
    glColor3f(0.62, 0.67, 0.73);
    glVertex2f(- 0.435, 0.215);
    glVertex2f(- 0.435, 0.245);
    glEnd();

    glLineWidth(4);
    glBegin(GL_LINES);
    glColor3f(0.62, 0.67, 0.73);
    glVertex2f(- 0.445, 0.215);
    glVertex2f(- 0.445, 0.245);
    glEnd();


    glLineWidth(4);
    glBegin(GL_LINES);
    glColor3f(0.62, 0.67, 0.73);
    glVertex2f(- 0.435, 0.255);
    glVertex2f(- 0.435, 0.285);
    glEnd();

    glLineWidth(4);
    glBegin(GL_LINES);
    glColor3f(0.62, 0.67, 0.73);
    glVertex2f(- 0.445, 0.255);
    glVertex2f(- 0.445, 0.285);
    glEnd();

    glLineWidth(5);
    glBegin(GL_LINES);
    glColor3f(0.80, 0.80, 0.80);
    glVertex2f(- 0.421, 0.2);
    glVertex2f(- 0.458, 0.2);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.80, 0.80, 0.80);
    glVertex2f(-0.425f, 0.2f);
    glVertex2f(-0.425f, 0.15f);
    glVertex2f(-0.455f, 0.15);
    glVertex2f(-0.455f, 0.2f);
    glEnd();


    // car 01.....................................................................


    glBegin(GL_QUADS);
    glColor3f(0.8f, 0.8f, 0.8f);
    glVertex2f(-0.34f, 0.295f);
    glVertex2f(-0.33f, 0.2f);
    glVertex2f(-0.4f, 0.2f);
    glVertex2f(-0.39f, 0.295f);

    glVertex2f(-0.325f, 0.17f);
    glVertex2f(-0.33f, 0.2f);
    glVertex2f(-0.4f, 0.2f);
    glVertex2f(-0.405f, 0.17f);

    glEnd();


    glPushMatrix();
    glTranslatef(-0.01f, 0.01f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex2f(-0.34f, 0.28f);
    glVertex2f(-0.33f, 0.25f);
    glVertex2f(-0.38f, 0.25f);
    glVertex2f(-0.37f, 0.28f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.342f, 0.277f);
    glVertex2f(-0.335f, 0.255f);
    glVertex2f(-0.375f, 0.255f);
    glVertex2f(-0.367f, 0.277f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.9f, 0.9f, 0.0f);
    glVertex2f(-0.325f, 0.24f);
    glVertex2f(-0.33f, 0.25f);
    glVertex2f(-0.38f, 0.25f);
    glVertex2f(-0.385f, 0.24f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex2f(-0.325f, 0.24f);
    glVertex2f(-0.325f, 0.21f);
    glVertex2f(-0.385f, 0.21f);
    glVertex2f(-0.385f, 0.24f);
    glEnd();

    glBegin(GL_QUADS);

    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(-0.328f, 0.234f);
    glVertex2f(-0.328f, 0.216f);
    glVertex2f(-0.339f, 0.216f);
    glVertex2f(-0.339f, 0.234f);

    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(-0.372f, 0.234f);
    glVertex2f(-0.372f, 0.216f);
    glVertex2f(-0.382f, 0.216f);
    glVertex2f(-0.382f, 0.234f);

    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.369f, 0.22f);
    glVertex2f(-0.369f, 0.215f);
    glVertex2f(-0.341f, 0.215f);
    glVertex2f(-0.341f, 0.22f);

    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.369f, 0.225f);
    glVertex2f(-0.369f, 0.23f);
    glVertex2f(-0.341f, 0.23f);
    glVertex2f(-0.341f, 0.225f);


    glEnd();


    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.375f, 0.195f);
    glVertex2f(-0.375f, 0.21f);
    glVertex2f(-0.385f, 0.21f);
    glVertex2f(-0.385f, 0.195f);

    glVertex2f(-0.325f, 0.195f);
    glVertex2f(-0.325f, 0.21f);
    glVertex2f(-0.335f, 0.21f);
    glVertex2f(-0.335f, 0.195f);
    glEnd();

    glPopMatrix();

//house03..............................royal////////////////////////////////////////////////////////////////////////

    glBegin(GL_QUADS);
    glColor3f(0.0f,0.4f,0.2f);
    glVertex2f(-0.3f, 0.45f);
    glVertex2f(0.0f, 0.45f);
    glVertex2f(0.0f, 0.19f);
    glVertex2f(-0.3f, 0.19f);

    glColor3f(0.4f,0.0f,0.0f);

    glVertex2f(-0.0065f, 0.45f);
    glVertex2f(0.0f, 0.45f);
    glVertex2f(0.0f, 0.19f);
    glVertex2f(-0.0065f, 0.19f);

    glColor3f(0.4f,0.0f,0.0f);
    glVertex2f(-0.3f, 0.45f);
    glVertex2f(-0.295f, 0.45f);
    glVertex2f(-0.295f, 0.19f);
    glVertex2f(-0.3f, 0.19f);


    glEnd();


    glPushMatrix();
    glTranslatef(0.0f, 0.08f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.8f, 0.4f, 0.0f);
    glVertex2f(-0.02f, 0.37f);
    glVertex2f(0.0f, 0.33f);
    glVertex2f(-0.3f, 0.33f);
    glVertex2f(-0.28f, 0.37f);

    glColor3f(1.0f, 0.4f, 0.2f);
    glVertex2f(-0.01f, 0.22f);
    glVertex2f(-0.01f, 0.33f);
    glVertex2f(-0.29f, 0.33f);
    glVertex2f(-0.29f, 0.22f);


    glColor3f(1.0f, 0.8f, 0.6f);
    glVertex2f(-0.01f, 0.325f);
    glVertex2f(-0.01f, 0.33f);
    glVertex2f(-0.29f, 0.33f);
    glVertex2f(-0.29f, 0.325f);


    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex2f(-0.01f, 0.22f);
    glVertex2f(-0.01f, 0.227f);
    glVertex2f(-0.29f, 0.227f);
    glVertex2f(-0.29f, 0.22f);


    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.8f, 0.4f, 0.0f);
    glVertex2f(-0.12f, 0.4f);
    glVertex2f(-0.09f, 0.35f);
    glVertex2f(-0.21f, 0.35f);
    glVertex2f(-0.18f, 0.4f);

    glColor3f(1.0f, 0.6f, 0.2f);
    glVertex2f(-0.1f, 0.2f);
    glVertex2f(-0.1f, 0.35f);
    glVertex2f(-0.2f, 0.35f);
    glVertex2f(-0.2f, 0.2f);


    glColor3f(1.0f, 0.8f, 0.6f);
    glVertex2f(-0.1f, 0.345f);
    glVertex2f(-0.1f, 0.35f);
    glVertex2f(-0.2f, 0.35f);
    glVertex2f(-0.2f, 0.345f);

    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex2f(-0.1f, 0.2f);
    glVertex2f(-0.1f, 0.215f);
    glVertex2f(-0.2f, 0.215f);
    glVertex2f(-0.2f, 0.2f);


    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex2f(-0.17f, 0.282f);
    glVertex2f(-0.17f, 0.215f);
    glVertex2f(-0.13f, 0.215f);
    glVertex2f(-0.13f, 0.282f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.151f, 0.277f);
    glVertex2f(-0.151f, 0.215f);
    glVertex2f(-0.165f, 0.215f);
    glVertex2f(-0.165f, 0.277f);

    glVertex2f(-0.135f, 0.277f);
    glVertex2f(-0.135f, 0.215f);
    glVertex2f(-0.149f, 0.215f);
    glVertex2f(-0.149f, 0.277f);

    glEnd();

    glBegin(GL_TRIANGLES);
    glColor3f(0.6f, 0.0f, 0.0f);
    glVertex2f(-0.111f, 0.345f);
    glVertex2f(-0.15f, 0.42f);
    glVertex2f(-0.189f, 0.345f);

    glColor3f(1.0f, 0.8f, 0.6f);
    glVertex2f(-0.117f, 0.345f);
    glVertex2f(-0.15f, 0.41f);
    glVertex2f(-0.184f, 0.345f);

    glEnd();

    glColor3f(0.0,0.0,1.0);
    renderBitmapString(-0.168f, 0.35f, 0.0f, GLUT_BITMAP_HELVETICA_10, "Royal");


// windus.......................
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.285f, 0.283f);
    glVertex2f(-0.285f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.022f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.285f, 0.283f);
    glVertex2f(-0.285f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.044f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.285f, 0.283f);
    glVertex2f(-0.285f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.066f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.285f, 0.283f);
    glVertex2f(-0.285f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();


    //
    glPushMatrix();
    glTranslatef(0.0f, -0.046f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.285f, 0.283f);
    glVertex2f(-0.285f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.022f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.285f, 0.283f);
    glVertex2f(-0.285f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.044f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.285f, 0.283f);
    glVertex2f(-0.285f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.066f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.285f, 0.283f);
    glVertex2f(-0.285f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.189f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.285f, 0.283f);
    glVertex2f(-0.285f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.022f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.285f, 0.283f);
    glVertex2f(-0.285f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.044f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.285f, 0.283f);
    glVertex2f(-0.285f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.066f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.286f, 0.283f);
    glVertex2f(-0.286f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();


    //
    glPushMatrix();
    glTranslatef(0.0f, -0.046f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.285f, 0.283f);
    glVertex2f(-0.285f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.022f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.285f, 0.283f);
    glVertex2f(-0.285f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.044f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.285f, 0.283f);
    glVertex2f(-0.285f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.066f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.286f, 0.283f);
    glVertex2f(-0.286f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();
    glPopMatrix();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.094f, 0.01f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.286f, 0.283f);
    glVertex2f(-0.286f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.022f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.286f, 0.283f);
    glVertex2f(-0.286f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.044f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.286f, 0.283f);
    glVertex2f(-0.286f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.066f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.286f, 0.283f);
    glVertex2f(-0.286f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.283f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    glPopMatrix();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.094f, -0.037f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.286f, 0.26f);
    glVertex2f(-0.286f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.26f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    //..
    glPushMatrix();
    glTranslatef(0.007f, -0.036f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.036f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.047f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.047f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    //..

    glPushMatrix();
    glTranslatef(0.066f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.286f, 0.26f);
    glVertex2f(-0.286f, 0.32f);
    glVertex2f(-0.27f, 0.32f);
    glVertex2f(-0.27f, 0.26f);

    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.007f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.012f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.024f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    //..
    glPushMatrix();
    glTranslatef(0.007f, -0.036f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.036f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.007f, -0.047f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.047f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.6f,0.8f,1.0f);
    glVertex2f(-0.284f, 0.309f);
    glVertex2f(-0.284f, 0.317f);
    glVertex2f(-0.278f, 0.317f);
    glVertex2f(-0.278f, 0.309f);
    glEnd();
    glPopMatrix();
    //..
    glPopMatrix();
    glPopMatrix();
    glPopMatrix();

    glBegin(GL_QUADS);
    glColor3f(0.6f,0.0f,0.0f);

    glVertex2f(-0.3f, 0.23f);
    glVertex2f(-0.17f, 0.23f);
    glVertex2f(-0.17f, 0.19f);
    glVertex2f(-0.3f, 0.19f);

    glVertex2f(0.0f, 0.23f);
    glVertex2f(-0.13f, 0.23f);
    glVertex2f(-0.13f, 0.19f);
    glVertex2f(0.0f, 0.19f);


    glColor3f(0.4f,0.0f,0.0f);

    glVertex2f(-0.18f, 0.24f);
    glVertex2f(-0.17f, 0.24f);
    glVertex2f(-0.17f, 0.19f);
    glVertex2f(-0.18f, 0.19f);

    glVertex2f(-0.12f, 0.24f);
    glVertex2f(-0.13f, 0.24f);
    glVertex2f(-0.13f, 0.19f);
    glVertex2f(-0.12f, 0.19f);


    glColor3f(0.8f,0.8f,0.8f);
    glVertex2f(-0.13f, 0.17f);
    glVertex2f(-0.17f, 0.17f);
    glVertex2f(-0.17f, 0.19f);
    glVertex2f(-0.13f, 0.19f);


    glEnd();

/////////////////////////.......................//////////end_royal///////////////...........................//////////////


////////////////////////////////////////////// marmad/////////////////////////////////////////////////////////////

    glPushMatrix();
    // glTranslatef(-0.03f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 0.89f, 0.8f);
    glVertex2f(0.04f, 0.31f);
    glVertex2f(0.04f, 0.3f);
    glVertex2f(0.36f, 0.3f);
    glVertex2f(0.36f, 0.31f);

    glColor3f(1.0f, 0.69f, 0.4f);
    glVertex2f(0.04f, 0.29f);
    glVertex2f(0.04f, 0.3f);
    glVertex2f(0.36f, 0.3f);
    glVertex2f(0.36f, 0.29f);

    glColor3f(0.69f, 0.4f, 1.0f);
    glVertex2f(0.05f, 0.29f);
    glVertex2f(0.05f, 0.19f);
    glVertex2f(0.35f, 0.19f);
    glVertex2f(0.35f, 0.29f);

    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(0.055f, 0.285f);
    glVertex2f(0.055f, 0.195f);
    glVertex2f(0.105f, 0.195f);
    glVertex2f(0.105f, 0.285f);

    glColor3f(0.6f, 0.8f, 1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.06f, 0.28f);
    glVertex2f(0.06f, 0.2f);
    glVertex2f(0.1f, 0.2f);
    glVertex2f(0.1f, 0.28f);


    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(0.295f, 0.285f);
    glVertex2f(0.295f, 0.195f);
    glVertex2f(0.345f, 0.195f);
    glVertex2f(0.345f, 0.285f);

    glColor3f(0.6f, 0.8f, 1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.3f, 0.28f);
    glVertex2f(0.3f, 0.2f);
    glVertex2f(0.34f, 0.2f);
    glVertex2f(0.34f, 0.28f);


    glEnd();


    glBegin(GL_QUADS);
    glColor3f(1.0f, 0.89f, 0.8f);
    glVertex2f(0.1f, 0.385f);
    glVertex2f(0.1f, 0.4f);
    glVertex2f(0.3f, 0.4f);
    glVertex2f(0.3f, 0.385f);

    glColor3f(1.0f, 0.69f, 0.4f);
    glVertex2f(0.1f, 0.385f);
    glVertex2f(0.1f, 0.37f);
    glVertex2f(0.3f, 0.37f);
    glVertex2f(0.3f, 0.385f);

    glColor3f(0.69f, 0.4f, 1.0f);
    glVertex2f(0.11f, 0.37f);
    glVertex2f(0.11f, 0.19f);
    glVertex2f(0.29f, 0.19f);
    glVertex2f(0.29f, 0.37f);

    glColor3f(1.0f, 0.5f, 0.0f);
    glVertex2f(0.11f, 0.278f);
    glVertex2f(0.11f, 0.29f);
    glVertex2f(0.29f, 0.29f);
    glVertex2f(0.29f, 0.278f);

    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(0.11f, 0.282f);
    glVertex2f(0.11f, 0.286f);
    glVertex2f(0.29f, 0.286f);
    glVertex2f(0.29f, 0.282f);


    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(0.12f, 0.36f);
    glVertex2f(0.12f, 0.3f);
    glVertex2f(0.28f, 0.3f);
    glVertex2f(0.28f, 0.36f);

    glColor3f(0.6f, 0.8f, 1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(0.125f, 0.355f);
    glVertex2f(0.125f, 0.305f);
    glVertex2f(0.275f, 0.305f);
    glVertex2f(0.275f, 0.355f);


    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(0.12f, 0.278f);
    glVertex2f(0.12f, 0.19f);
    glVertex2f(0.112f, 0.19f);
    glVertex2f(0.112f, 0.278f);

    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(0.288f, 0.278f);
    glVertex2f(0.288f, 0.19f);
    glVertex2f(0.28f, 0.19f);
    glVertex2f(0.28f, 0.278f);


    glColor3f(0.8f, 0.8f, 0.8f);
    glVertex2f(0.04f, 0.15f);
    glVertex2f(0.05f, 0.19f);
    glVertex2f(0.35f, 0.19f);
    glVertex2f(0.36f, 0.15f);


    glEnd();


// car1 copy


    glPushMatrix();
    glTranslatef(0.5f, 0.055f, 0.0f);
    glScalef( 0.7, 0.7,0);

    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.34f, 0.28f);
    glVertex2f(-0.33f, 0.25f);
    glVertex2f(-0.38f, 0.25f);
    glVertex2f(-0.37f, 0.28f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.342f, 0.277f);
    glVertex2f(-0.335f, 0.255f);
    glVertex2f(-0.375f, 0.255f);
    glVertex2f(-0.367f, 0.277f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.325f, 0.24f);
    glVertex2f(-0.33f, 0.25f);
    glVertex2f(-0.38f, 0.25f);
    glVertex2f(-0.385f, 0.24f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.325f, 0.24f);
    glVertex2f(-0.325f, 0.21f);
    glVertex2f(-0.385f, 0.21f);
    glVertex2f(-0.385f, 0.24f);
    glEnd();
    glBegin(GL_QUADS);

    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(-0.328f, 0.234f);
    glVertex2f(-0.328f, 0.216f);
    glVertex2f(-0.339f, 0.216f);
    glVertex2f(-0.339f, 0.234f);

    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(-0.372f, 0.234f);
    glVertex2f(-0.372f, 0.216f);
    glVertex2f(-0.382f, 0.216f);
    glVertex2f(-0.382f, 0.234f);

    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.369f, 0.22f);
    glVertex2f(-0.369f, 0.215f);
    glVertex2f(-0.341f, 0.215f);
    glVertex2f(-0.341f, 0.22f);

    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.369f, 0.225f);
    glVertex2f(-0.369f, 0.23f);
    glVertex2f(-0.341f, 0.23f);
    glVertex2f(-0.341f, 0.225f);


    glEnd();


    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.375f, 0.195f);
    glVertex2f(-0.375f, 0.21f);
    glVertex2f(-0.385f, 0.21f);
    glVertex2f(-0.385f, 0.195f);

    glVertex2f(-0.325f, 0.195f);
    glVertex2f(-0.325f, 0.21f);
    glVertex2f(-0.335f, 0.21f);
    glVertex2f(-0.335f, 0.195f);
    glEnd();

    glLoadIdentity();

    glPopMatrix();

    //again car1 copy

    glPushMatrix();
    glTranslatef(0.445f, 0.055f, 0.0f);
    glScalef( 0.7, 0.7,0);

    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.29f, 0.6f);
    glVertex2f(-0.34f, 0.28f);
    glVertex2f(-0.33f, 0.25f);
    glVertex2f(-0.38f, 0.25f);
    glVertex2f(-0.37f, 0.28f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.342f, 0.277f);
    glVertex2f(-0.335f, 0.255f);
    glVertex2f(-0.375f, 0.255f);
    glVertex2f(-0.367f, 0.277f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.2f, 0.4f);
    glVertex2f(-0.325f, 0.24f);
    glVertex2f(-0.33f, 0.25f);
    glVertex2f(-0.38f, 0.25f);
    glVertex2f(-0.385f, 0.24f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.29f, 0.6f);
    glVertex2f(-0.325f, 0.24f);
    glVertex2f(-0.325f, 0.21f);
    glVertex2f(-0.385f, 0.21f);
    glVertex2f(-0.385f, 0.24f);
    glEnd();
    glBegin(GL_QUADS);

    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(-0.328f, 0.234f);
    glVertex2f(-0.328f, 0.216f);
    glVertex2f(-0.339f, 0.216f);
    glVertex2f(-0.339f, 0.234f);

    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(-0.372f, 0.234f);
    glVertex2f(-0.372f, 0.216f);
    glVertex2f(-0.382f, 0.216f);
    glVertex2f(-0.382f, 0.234f);

    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.369f, 0.22f);
    glVertex2f(-0.369f, 0.215f);
    glVertex2f(-0.341f, 0.215f);
    glVertex2f(-0.341f, 0.22f);

    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(-0.369f, 0.225f);
    glVertex2f(-0.369f, 0.23f);
    glVertex2f(-0.341f, 0.23f);
    glVertex2f(-0.341f, 0.225f);


    glEnd();


    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.375f, 0.195f);
    glVertex2f(-0.375f, 0.21f);
    glVertex2f(-0.385f, 0.21f);
    glVertex2f(-0.385f, 0.195f);

    glVertex2f(-0.325f, 0.195f);
    glVertex2f(-0.325f, 0.21f);
    glVertex2f(-0.335f, 0.21f);
    glVertex2f(-0.335f, 0.195f);
    glEnd();

    glLoadIdentity();

    glPopMatrix();

    //again car1 copy

    glPushMatrix();
    glTranslatef(0.395f, 0.055f, 0.0f);
    glScalef( 0.7, 0.7,0);

    glBegin(GL_QUADS);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(-0.34f, 0.28f);
    glVertex2f(-0.33f, 0.25f);
    glVertex2f(-0.38f, 0.25f);
    glVertex2f(-0.37f, 0.28f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.342f, 0.277f);
    glVertex2f(-0.335f, 0.255f);
    glVertex2f(-0.375f, 0.255f);
    glVertex2f(-0.367f, 0.277f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(1.0f, 0.8f, 0.0f);
    glVertex2f(-0.325f, 0.24f);
    glVertex2f(-0.33f, 0.25f);
    glVertex2f(-0.38f, 0.25f);
    glVertex2f(-0.385f, 0.24f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(-0.325f, 0.24f);
    glVertex2f(-0.325f, 0.21f);
    glVertex2f(-0.385f, 0.21f);
    glVertex2f(-0.385f, 0.24f);
    glEnd();
    glBegin(GL_QUADS);

    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.328f, 0.234f);
    glVertex2f(-0.328f, 0.216f);
    glVertex2f(-0.339f, 0.216f);
    glVertex2f(-0.339f, 0.234f);

    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.372f, 0.234f);
    glVertex2f(-0.372f, 0.216f);
    glVertex2f(-0.382f, 0.216f);
    glVertex2f(-0.382f, 0.234f);

    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.369f, 0.22f);
    glVertex2f(-0.369f, 0.215f);
    glVertex2f(-0.341f, 0.215f);
    glVertex2f(-0.341f, 0.22f);

    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.369f, 0.225f);
    glVertex2f(-0.369f, 0.23f);
    glVertex2f(-0.341f, 0.23f);
    glVertex2f(-0.341f, 0.225f);


    glEnd();


    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(-0.375f, 0.195f);
    glVertex2f(-0.375f, 0.21f);
    glVertex2f(-0.385f, 0.21f);
    glVertex2f(-0.385f, 0.195f);

    glVertex2f(-0.325f, 0.195f);
    glVertex2f(-0.325f, 0.21f);
    glVertex2f(-0.335f, 0.21f);
    glVertex2f(-0.335f, 0.195f);
    glEnd();

    glLoadIdentity();

    glPopMatrix();

    glPopMatrix();


//BAYWATCH Hotel (replaced hospital)
    // === BAYWATCH 5-STAR HOTEL ===
    glPushMatrix();
    glTranslatef(0.25f, 0.04f, 0.0f);
    glScalef(0.55f,0.85f,0.0f);

    // Main building - luxury beige
    glBegin(GL_QUADS);
    glColor3f(0.96f, 0.87f, 0.70f);
    glVertex2f(0.7f, 0.6f);
    glVertex2f(1.0f, 0.6f);
    glVertex2f(1.0f, 0.1f);
    glVertex2f(0.7f, 0.1f);

    // Dark base
    glColor3f(0.55f, 0.40f, 0.25f);
    glVertex2f(0.7f, 0.12f);
    glVertex2f(1.0f, 0.12f);
    glVertex2f(1.0f, 0.1f);
    glVertex2f(0.7f, 0.1f);

    // Roof
    glColor3f(0.18f, 0.38f, 0.62f);
    glVertex2f(0.68f, 0.62f);
    glVertex2f(1.02f, 0.62f);
    glVertex2f(1.02f, 0.60f);
    glVertex2f(0.68f, 0.60f);

    // Windows row 1
    glColor3f(0.5f, 0.8f, 1.0f);
    glVertex2f(0.73f, 0.55f); glVertex2f(0.79f, 0.55f);
    glVertex2f(0.79f, 0.48f); glVertex2f(0.73f, 0.48f);

    glVertex2f(0.82f, 0.55f); glVertex2f(0.88f, 0.55f);
    glVertex2f(0.88f, 0.48f); glVertex2f(0.82f, 0.48f);

    glVertex2f(0.91f, 0.55f); glVertex2f(0.97f, 0.55f);
    glVertex2f(0.97f, 0.48f); glVertex2f(0.91f, 0.48f);

    // Windows row 2
    glVertex2f(0.73f, 0.44f); glVertex2f(0.79f, 0.44f);
    glVertex2f(0.79f, 0.37f); glVertex2f(0.73f, 0.37f);

    glVertex2f(0.82f, 0.44f); glVertex2f(0.88f, 0.44f);
    glVertex2f(0.88f, 0.37f); glVertex2f(0.82f, 0.37f);

    glVertex2f(0.91f, 0.44f); glVertex2f(0.97f, 0.44f);
    glVertex2f(0.97f, 0.37f); glVertex2f(0.91f, 0.37f);

    // Entrance door (grand)
    glColor3f(0.30f, 0.20f, 0.10f);
    glVertex2f(0.81f, 0.30f); glVertex2f(0.89f, 0.30f);
    glVertex2f(0.89f, 0.13f); glVertex2f(0.81f, 0.13f);

    // Door glass
    glColor3f(0.6f, 0.85f, 1.0f);
    glVertex2f(0.82f, 0.29f); glVertex2f(0.88f, 0.29f);
    glVertex2f(0.88f, 0.14f); glVertex2f(0.82f, 0.14f);

    // Awning above door
    glColor3f(0.0f, 0.25f, 0.55f);
    glVertex2f(0.79f, 0.33f); glVertex2f(0.91f, 0.33f);
    glVertex2f(0.91f, 0.30f); glVertex2f(0.79f, 0.30f);

    glEnd();

    // Swimming pool in front of hotel
    glBegin(GL_QUADS);
    // Pool water
    glColor3f(0.0f, 0.65f, 0.90f);
    glVertex2f(0.70f, 0.08f);
    glVertex2f(1.00f, 0.08f);
    glVertex2f(1.00f, -0.05f);
    glVertex2f(0.70f, -0.05f);

    // Pool edge
    glColor3f(0.88f, 0.88f, 0.88f);
    glVertex2f(0.69f, 0.09f);
    glVertex2f(1.01f, 0.09f);
    glVertex2f(1.01f, 0.08f);
    glVertex2f(0.69f, 0.08f);

    glVertex2f(0.69f, -0.05f);
    glVertex2f(1.01f, -0.05f);
    glVertex2f(1.01f, -0.06f);
    glVertex2f(0.69f, -0.06f);

    glEnd();

    // Pool lane lines
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex2f(0.80f, 0.08f); glVertex2f(0.80f, -0.05f);
    glVertex2f(0.90f, 0.08f); glVertex2f(0.90f, -0.05f);
    glEnd();

    // BAYWATCH sign
    glColor3f(0.0f, 0.0f, 0.0f);
    renderBitmapString(0.73f, 0.64f, 0.0f, GLUT_BITMAP_HELVETICA_12, "BAYWATCH");

    glPopMatrix();
    ////////////////////////////////////////house5_end/////////////////////////////////////////////////////


// tree01 ............................................................................................................

    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex2f(-0.64f, 0.2f);
    glVertex2f(-0.64f, 0.3f);
    glVertex2f(-0.65f, 0.3f);
    glVertex2f(-0.65f, 0.2f);
    glEnd();


    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPushMatrix();
    glTranslatef(-0.01f, 0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.02f, 0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.01f, 0.02f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.645f, 0.25f);
    glVertex2f(-0.64f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.655f, 0.26f);
    glVertex2f(-0.658f, 0.2f);


    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.645f, 0.2f);
    glVertex2f(-0.635f, 0.26f);
    glVertex2f(-0.638f, 0.2f);


    glEnd();


    glPopMatrix();


    GLfloat xt01=-0.645f;
    GLfloat yt01=0.3f;
    GLfloat radiust01 =0.039f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xt01, yt01);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xt01 + (radiust01 * cos(i *  twice1Pi / triangleAmount)),
                    yt01 + (radiust01 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();


    GLfloat xt02=-0.645f;
    GLfloat yt02=0.33f;
    GLfloat radiust02 =0.034f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xt02, yt02);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xt02 + (radiust02 * cos(i *  twice1Pi / triangleAmount)),
                    yt02 + (radiust02 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();

    GLfloat xt03=-0.645f;
    GLfloat yt03=0.36f;
    GLfloat radiust03 =0.029f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xt03, yt03);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xt03 + (radiust03 * cos(i *  twice1Pi / triangleAmount)),
                    yt03 + (radiust03 * sin(i * twice1Pi / triangleAmount)) );
    }
    glEnd();

//tree02...............................................................................................................

    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex2f(-0.96f, 0.2f);
    glVertex2f(-0.96f, 0.3f);
    glVertex2f(-0.95f, 0.3f);
    glVertex2f(-0.95f, 0.2f);
    glEnd();


    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.95f, 0.2f);
    glVertex2f(-0.945f, 0.25f);
    glVertex2f(-0.938f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.955f, 0.26f);
    glVertex2f(-0.958f, 0.2f);

    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.935f, 0.26f);
    glVertex2f(-0.938f, 0.2f);
    glEnd();

    glPushMatrix();
    glTranslatef(-0.02f, 0.02f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.95f, 0.2f);
    glVertex2f(-0.945f, 0.25f);
    glVertex2f(-0.938f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.955f, 0.26f);
    glVertex2f(-0.958f, 0.2f);

    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.935f, 0.26f);
    glVertex2f(-0.938f, 0.2f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.03f, 0.02f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.95f, 0.2f);
    glVertex2f(-0.945f, 0.25f);
    glVertex2f(-0.938f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.955f, 0.26f);
    glVertex2f(-0.958f, 0.2f);

    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.935f, 0.26f);
    glVertex2f(-0.938f, 0.2f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.04f, 0.02f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.95f, 0.2f);
    glVertex2f(-0.945f, 0.25f);
    glVertex2f(-0.938f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.955f, 0.26f);
    glVertex2f(-0.958f, 0.2f);

    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.935f, 0.26f);
    glVertex2f(-0.938f, 0.2f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.04f, 0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.95f, 0.2f);
    glVertex2f(-0.945f, 0.25f);
    glVertex2f(-0.938f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.955f, 0.26f);
    glVertex2f(-0.958f, 0.2f);

    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.935f, 0.26f);
    glVertex2f(-0.938f, 0.2f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.02f, 0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.95f, 0.2f);
    glVertex2f(-0.945f, 0.25f);
    glVertex2f(-0.938f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.955f, 0.26f);
    glVertex2f(-0.958f, 0.2f);

    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.935f, 0.26f);
    glVertex2f(-0.938f, 0.2f);
    glEnd();
    glPopMatrix();


    GLfloat xt4=-0.955f;
    GLfloat yt4=0.3f;
    GLfloat radiust4 =0.03f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xt4, yt4);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xt4 + (radiust4 * cos(i *  twicePi / triangleAmount)),
                    yt4 + (radiust4 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    GLfloat xt5=-0.955f;
    GLfloat yt5=0.33f;
    GLfloat radiust5 =0.025f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xt5, yt5);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xt5 + (radiust5 * cos(i *  twicePi / triangleAmount)),
                    yt5 + (radiust5 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    GLfloat xt6=-0.955f;
    GLfloat yt6=0.35f;
    GLfloat radiust6 =0.02f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xt6, yt6);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xt6 + (radiust6 * cos(i *  twicePi / triangleAmount)),
                    yt6 + (radiust6 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


// tree02 copy...........................................
    glPushMatrix();
    glTranslatef(-0.02f, 0.02f, 0.0f);

    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.0f, 0.0f);
    glVertex2f(-0.96f, 0.2f);
    glVertex2f(-0.96f, 0.3f);
    glVertex2f(-0.95f, 0.3f);
    glVertex2f(-0.95f, 0.2f);
    glEnd();

    GLfloat xt7=-0.955f;
    GLfloat yt7=0.3f;
    GLfloat radiust7 =0.03f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xt7, yt7);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xt7 + (radiust7 * cos(i *  twicePi / triangleAmount)),
                    yt7 + (radiust7 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    GLfloat xt8=-0.955f;
    GLfloat yt8=0.33f;
    GLfloat radiust8 =0.025f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xt8, yt8);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xt8 + (radiust8 * cos(i *  twicePi / triangleAmount)),
                    yt8 + (radiust8 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    GLfloat xt9=-0.955f;
    GLfloat yt9=0.35f;
    GLfloat radiust9 =0.02f;
    glColor3f(0.0f,0.2f,0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xt9, yt9);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xt9 + (radiust9 * cos(i *  twicePi / triangleAmount)),
                    yt9 + (radiust9 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    glPopMatrix();


    glPushMatrix();
    glTranslatef(-0.04f, -0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.95f, 0.2f);
    glVertex2f(-0.945f, 0.25f);
    glVertex2f(-0.938f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.955f, 0.26f);
    glVertex2f(-0.958f, 0.2f);

    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.935f, 0.26f);
    glVertex2f(-0.938f, 0.2f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.02f, -0.02f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.95f, 0.2f);
    glVertex2f(-0.945f, 0.25f);
    glVertex2f(-0.938f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.955f, 0.26f);
    glVertex2f(-0.958f, 0.2f);

    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.935f, 0.26f);
    glVertex2f(-0.938f, 0.2f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.05f, -0.01f, 0.0f);
    glBegin(GL_TRIANGLES);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.95f, 0.2f);
    glVertex2f(-0.945f, 0.25f);
    glVertex2f(-0.938f, 0.2f);

    glColor3f(0.0f, 0.2f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.955f, 0.26f);
    glVertex2f(-0.958f, 0.2f);

    glColor3f(0.0f, 0.5f, 0.0f);
    glVertex2f(-0.945f, 0.2f);
    glVertex2f(-0.935f, 0.26f);
    glVertex2f(-0.938f, 0.2f);
    glEnd();
    glPopMatrix();


    //wake way................................................................................
    glBegin(GL_QUADS);
    glColor3f(0.8f,0.8f,0.8f);
    glVertex2f(-1.0f, -0.17f);
    glVertex2f(1.0f, -0.17f);
    glVertex2f(1.0f, -0.12f);
    glVertex2f(-1.0f, -0.12f);


    glColor3f(0.8f,0.8f,0.8f);
    glVertex2f(-1.0f, 0.17f);
    glVertex2f(1.0f, 0.17f);
    glVertex2f(1.0f, 0.12f);
    glVertex2f(-1.0f, 0.12f);

    glColor3f(1.0f,1.0f,1.0f);
    glVertex2f(-1.0f, -0.1f);
    glVertex2f(1.0f, -0.1f);
    glVertex2f(1.0f, -0.12f);
    glVertex2f(-1.0f, -0.12f);


    glColor3f(1.0f,1.0f,1.0f);
    glVertex2f(-1.0f, 0.1f);
    glVertex2f(1.0f, 0.1f);
    glVertex2f(1.0f, 0.12f);
    glVertex2f(-1.0f, 0.12f);

    glEnd();

//lamp
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);


    glEnd();

    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.85f, 0.12f);

        glEnd();
    }

    //..

    glPushMatrix();
    glTranslatef(0.2f,0.0f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.85f, 0.12f);

        glEnd();
    }
    glPopMatrix();


    //..

    glPushMatrix();
    glTranslatef(0.4f,0.0f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.85f, 0.12f);
        glEnd();
    }
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslatef(0.6f,0.0f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.85f, 0.12f);

        glEnd();
    }
    glPopMatrix();


    //..

    glPushMatrix();
    glTranslatef(0.8f,0.0f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.85f, 0.12f);
        glEnd();
    }
    glPopMatrix();


    //..

    glPushMatrix();
    glTranslatef(1.0f,0.0f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.85f, 0.12f);
        glEnd();
    }
    glPopMatrix();


    //..

    glPushMatrix();
    glTranslatef(1.2f,0.0f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.85f, 0.12f);

        glEnd();
    }
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslatef(1.4f,0.0f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.85f, 0.12f);

        glEnd();
    }
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslatef(1.6f,0.0f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.85f, 0.12f);

        glEnd();
    }
    glPopMatrix();


    //..

    glPushMatrix();
    glTranslatef(1.8f,0.0f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.8f,0.8f,0.8f);
        glVertex2f(-0.85f, 0.12f);
        glEnd();
    }
    glPopMatrix();


// police

    GLfloat xp=0.4f;
    GLfloat yp=0.195f;
    GLfloat radiusp =0.007f;

    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xp, yp); // center of circle
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xp + (radiusp * cos(i *  twicePi / triangleAmount)),
                    yp+ (radiusp * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    // NAVY BLUE Police Uniform
    glBegin(GL_QUADS);
    // Shirt - navy blue with white stripe
    glColor3f(0.05f,0.10f,0.40f); // navy blue
    glVertex2f(0.394f, 0.185f);
    glVertex2f(0.407f, 0.185f);
    glVertex2f(0.407f, 0.155f);
    glVertex2f(0.394f, 0.155f);

    // White stripe center
    glColor3f(1.0f,1.0f,1.0f);
    glVertex2f(0.3995f, 0.185f);
    glVertex2f(0.4015f, 0.185f);
    glVertex2f(0.4015f, 0.155f);
    glVertex2f(0.3995f, 0.155f);

    // Gold badge on chest
    glColor3f(1.0f, 0.85f, 0.0f);
    glVertex2f(0.404f, 0.182f);
    glVertex2f(0.407f, 0.182f);
    glVertex2f(0.407f, 0.177f);
    glVertex2f(0.404f, 0.177f);

    glEnd();

    // Police cap (navy blue)
    glBegin(GL_QUADS);
    glColor3f(0.05f,0.10f,0.40f);
    glVertex2f(0.391f, 0.200f);
    glVertex2f(0.410f, 0.200f);
    glVertex2f(0.409f, 0.197f);
    glVertex2f(0.392f, 0.197f);
    // Cap brim
    glColor3f(0.02f,0.05f,0.25f);
    glVertex2f(0.389f, 0.197f);
    glVertex2f(0.412f, 0.197f);
    glVertex2f(0.412f, 0.195f);
    glVertex2f(0.389f, 0.195f);
    glEnd();

    glBegin(GL_QUADS);
    // Right hand - navy
    glColor3f(0.05f,0.10f,0.40f);
    glVertex2f(0.411f, 0.185f);
    glVertex2f(0.407f, 0.185f);
    glVertex2f(0.407f, 0.145f);
    glVertex2f(0.411f, 0.145f);

    glColor3f(0.8f,0.6f,0.4f); // skin cuff
    glVertex2f(0.411f, 0.150f);
    glVertex2f(0.407f, 0.150f);
    glVertex2f(0.407f, 0.145f);
    glVertex2f(0.411f, 0.145f);

    // Left hand
    if(!handup)
    {
        glColor3f(0.05f,0.10f,0.40f);
        glVertex2f(0.394f, 0.185f);
        glVertex2f(0.39f, 0.185f);
        glVertex2f(0.39f, 0.145f);
        glVertex2f(0.394f, 0.145f);

        glColor3f(0.8f,0.6f,0.4f);
        glVertex2f(0.394f, 0.150f);
        glVertex2f(0.39f, 0.150f);
        glVertex2f(0.39f, 0.145f);
        glVertex2f(0.394f, 0.145f);
    }
    if(handup)
    {
        glColor3f(0.05f,0.10f,0.40f);
        glVertex2f(0.394f, 0.175f);
        glVertex2f(0.39f, 0.175f);
        glVertex2f(0.39f, 0.215f);
        glVertex2f(0.394f, 0.215f);

        glColor3f(0.8f,0.6f,0.4f);
        glVertex2f(0.394f, 0.210f);
        glVertex2f(0.39f, 0.210f);
        glVertex2f(0.39f, 0.215f);
        glVertex2f(0.394f, 0.215f);
    }
    // Navy trousers
    glColor3f(0.05f,0.10f,0.30f);
    glVertex2f(0.406f, 0.12f);
    glVertex2f(0.401f, 0.12f);
    glVertex2f(0.401f, 0.155f);
    glVertex2f(0.406f, 0.155f);

    glColor3f(0.05f,0.10f,0.30f);
    glVertex2f(0.395f, 0.12f);
    glVertex2f(0.4f, 0.12f);
    glVertex2f(0.4f, 0.155f);
    glVertex2f(0.395f, 0.155f);

    // Table/desk
    glColor3f(0.3f,0.2f,0.1f);
    glVertex2f(0.415f, 0.12f);
    glVertex2f(0.47f, 0.12f);
    glVertex2f(0.46f, 0.158f);
    glVertex2f(0.42f, 0.158f);

    glColor3f(0.9f,0.9f,0.0f);
    glVertex2f(0.414f, 0.12f);
    glVertex2f(0.455f, 0.12f);
    glVertex2f(0.46f, 0.158f);
    glVertex2f(0.42f, 0.158f);

    glEnd();

    // === TRAFFIC LIGHT ===
    // Pole
    glColor3f(0.3f,0.3f,0.3f);
    glBegin(GL_QUADS);
    glVertex2f(0.42f, 0.19f);
    glVertex2f(0.424f, 0.19f);
    glVertex2f(0.424f, 0.12f);
    glVertex2f(0.42f, 0.12f);
    glEnd();
    // Box
    glColor3f(0.15f,0.15f,0.15f);
    glBegin(GL_QUADS);
    glVertex2f(0.416f, 0.285f);
    glVertex2f(0.428f, 0.285f);
    glVertex2f(0.428f, 0.19f);
    glVertex2f(0.416f, 0.19f);
    glEnd();
    // Red light
    {
        GLfloat xr=0.422f, yr=0.275f, rr=0.005f;
        if(trafficState==0) glColor3f(1.0f,0.0f,0.0f);
        else glColor3f(0.4f,0.0f,0.0f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(xr,yr);
        for(int i=0;i<=30;i++) glVertex2f(xr+rr*cos(i*twicePi/30),yr+rr*sin(i*twicePi/30));
        glEnd();
    }
    // Yellow light
    {
        GLfloat xr=0.422f, yr=0.262f, rr=0.005f;
        if(trafficState==1) glColor3f(1.0f,0.85f,0.0f);
        else glColor3f(0.4f,0.3f,0.0f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(xr,yr);
        for(int i=0;i<=30;i++) glVertex2f(xr+rr*cos(i*twicePi/30),yr+rr*sin(i*twicePi/30));
        glEnd();
    }
    // Green light
    {
        GLfloat xr=0.422f, yr=0.249f, rr=0.005f;
        if(trafficState==2) glColor3f(0.0f,0.9f,0.0f);
        else glColor3f(0.0f,0.3f,0.0f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(xr,yr);
        for(int i=0;i<=30;i++) glVertex2f(xr+rr*cos(i*twicePi/30),yr+rr*sin(i*twicePi/30));
        glEnd();
    }

    // === ZEBRA CROSSING ===
    glBegin(GL_QUADS);
    for(int z=0; z<5; z++) {
        if(z%2==0) glColor3f(1.0f,1.0f,1.0f);
        else glColor3f(0.25f,0.25f,0.25f);
        float zx = 0.43f + z * 0.018f;
        glVertex2f(zx,       -0.10f);
        glVertex2f(zx+0.016f,-0.10f);
        glVertex2f(zx+0.016f,-0.12f);
        glVertex2f(zx,       -0.12f);
    }
    glEnd();


//road................................................................................................................
    glBegin(GL_QUADS);


    glColor3f(0.2f,0.2f,0.2f);
    glVertex2f(-1.0f, -0.1f);
    glVertex2f(1.0f, -0.1f);
    glVertex2f(1.0f, 0.1f);
    glVertex2f(-1.0f, 0.1f);

    glColor3f(1.0f,1.0f,1.0f);
    glVertex2f(-0.05f, 0.01f);
    glVertex2f(0.05f, 0.01f);
    glVertex2f(0.05f,-0.01f);
    glVertex2f(-0.05f,-0.01f);
    glEnd();

    glPushMatrix();
    glTranslatef(-0.2f,0.0f, 0.0f);
    glColor3f(1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    glVertex2f(-0.05f, 0.01f);
    glVertex2f(0.05f, 0.01f);
    glVertex2f(0.05f,-0.01f);
    glVertex2f(-0.05f,-0.01f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.4f,0.0f, 0.0f);
    glColor3f(1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    glVertex2f(-0.05f, 0.01f);
    glVertex2f(0.05f, 0.01f);
    glVertex2f(0.05f,-0.01f);
    glVertex2f(-0.05f,-0.01f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.6f,0.0f, 0.0f);
    glColor3f(1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    glVertex2f(-0.05f, 0.01f);
    glVertex2f(0.05f, 0.01f);
    glVertex2f(0.05f,-0.01f);
    glVertex2f(-0.05f,-0.01f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.8f,0.0f, 0.0f);
    glColor3f(1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    glVertex2f(-0.05f, 0.01f);
    glVertex2f(0.05f, 0.01f);
    glVertex2f(0.05f,-0.01f);
    glVertex2f(-0.05f,-0.01f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-1.0f,0.0f, 0.0f);
    glColor3f(1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    glVertex2f(-0.05f, 0.01f);
    glVertex2f(0.05f, 0.01f);
    glVertex2f(0.05f,-0.01f);
    glVertex2f(-0.05f,-0.01f);
    glEnd();
    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.2f,0.0f, 0.0f);
    glColor3f(1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    glVertex2f(-0.05f, 0.01f);
    glVertex2f(0.05f, 0.01f);
    glVertex2f(0.05f,-0.01f);
    glVertex2f(-0.05f,-0.01f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.4f,0.0f, 0.0f);
    glColor3f(1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    glVertex2f(-0.05f, 0.01f);
    glVertex2f(0.05f, 0.01f);
    glVertex2f(0.05f,-0.01f);
    glVertex2f(-0.05f,-0.01f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.6f,0.0f, 0.0f);
    glColor3f(1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    glVertex2f(-0.05f, 0.01f);
    glVertex2f(0.05f, 0.01f);
    glVertex2f(0.05f,-0.01f);
    glVertex2f(-0.05f,-0.01f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.8f,0.0f, 0.0f);
    glColor3f(1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    glVertex2f(-0.05f, 0.01f);
    glVertex2f(0.05f, 0.01f);
    glVertex2f(0.05f,-0.01f);
    glVertex2f(-0.05f,-0.01f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.0f,0.0f, 0.0f);
    glColor3f(1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    glVertex2f(-0.05f, 0.01f);
    glVertex2f(0.05f, 0.01f);
    glVertex2f(0.05f,-0.01f);
    glVertex2f(-0.05f,-0.01f);
    glEnd();
    glPopMatrix();


    // bus01<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>


    glPushMatrix();
    glTranslatef(run[3],0,0);
    GLfloat xb1=-0.455f;
    GLfloat yb1=0.095f;
    GLfloat radiusb1 = 0.01f;
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xb1, yb1);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xb1 + (radiusb1 * cos(i *  twicePi / triangleAmount)),
                    yb1 + (radiusb1 * sin(i * twicePi / triangleAmount)));
    }
    glEnd();

    if(night)
    {
        glBegin(GL_QUADS);
        glColor3f(1.0f,0.6f,0.2f);
        glVertex2f(-0.445f, 0.097f);
        glVertex2f(-0.455f, 0.095f);
        glColor3f(1.0f,1.0f,0.79f);
        glVertex2f(-0.435f, 0.06f);
        glVertex2f(-0.39f, 0.06f);

        glEnd();
    }

    glBegin(GL_QUADS);
    glColor3f(1.0, 0.5, 0.0);
    glVertex2f(- 0.45, 0.188);
    glVertex2f(- 0.45, 0.06);
    glColor3f(0.98, 0.88, 0.02);
    glVertex2f(- 0.6, 0.06);
    glVertex2f(- 0.6, 0.188);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.00, 0.00, 0.00);
    glVertex2f(- 0.45, 0.077);
    glVertex2f(- 0.45, 0.069);
    glVertex2f(- 0.46, 0.069);
    glVertex2f(- 0.46, 0.077);
    glEnd();

    glLineWidth(6);
    glBegin(GL_LINES);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.455f, 0.17f);
    glVertex2f(- 0.455f, 0.12f);
    glEnd();

    glLineWidth(2);
    glBegin(GL_LINES);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.59f, 0.12f);
    glVertex2f(- 0.47f, 0.12f);
    glEnd();

    glLineWidth(2);
    glBegin(GL_LINES);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.59f, 0.1f);
    glVertex2f(- 0.47f, 0.1f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.465, 0.17);
    glVertex2f(- 0.465, 0.14);
    glVertex2f(- 0.48, 0.14);
    glVertex2f(- 0.48, 0.17);
    glEnd();

    glPushMatrix();
    glTranslatef(- 0.022,0,0);
    glBegin(GL_QUADS);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.465, 0.17);
    glVertex2f(- 0.465, 0.14);
    glVertex2f(- 0.48, 0.14);
    glVertex2f(- 0.48, 0.17);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(- 0.044,0,0);
    glBegin(GL_QUADS);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.465, 0.17);
    glVertex2f(- 0.465, 0.14);
    glVertex2f(- 0.48, 0.14);
    glVertex2f(- 0.48, 0.17);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(- 0.066,0,0);
    glBegin(GL_QUADS);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.465, 0.17);
    glVertex2f(- 0.465, 0.14);
    glVertex2f(- 0.48, 0.14);
    glVertex2f(- 0.48, 0.17);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(- 0.088,0,0);
    glBegin(GL_QUADS);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.465, 0.17);
    glVertex2f(- 0.465, 0.14);
    glVertex2f(- 0.48, 0.14);
    glVertex2f(- 0.48, 0.17);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(- 0.11,0,0);
    glBegin(GL_QUADS);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.465, 0.17);
    glVertex2f(- 0.465, 0.14);
    glVertex2f(- 0.48, 0.14);
    glVertex2f(- 0.48, 0.17);
    glEnd();
    glPopMatrix();


    GLfloat xb2=-0.49f;
    GLfloat yb2=0.06f;
    GLfloat radiusb2 = 0.02f;
    glColor3f(0.10, 0.10, 0.10);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xb2, yb2);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xb2 + (radiusb2 * cos(i *  twicePi / triangleAmount)),
                    yb2 + (radiusb2 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


    GLfloat xb3=- 0.49f;
    GLfloat yb3=0.06f;
    GLfloat radiusb3 = 0.014f;
    glColor3f(0.47, 0.46, 0.46 );
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xb3, yb3);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xb3 + (radiusb3 * cos(i *  twicePi / triangleAmount)),
                    yb3 + (radiusb3 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


    GLfloat xb4=- 0.57f;
    GLfloat yb4=0.06f;
    glColor3f(0.10, 0.10, 0.10 );
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xb4, yb4);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xb4 + (radiusb2 * cos(i *  twicePi / triangleAmount)),
                    yb4 + (radiusb2 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


    GLfloat xb5=- 0.57f;
    GLfloat yb5=0.06f;
    glColor3f(0.47, 0.46, 0.46 );
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xb5, yb5);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xb5 + (radiusb3 * cos(i *  twicePi / triangleAmount)),
                    yb5 + (radiusb3 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    glPopMatrix();

    //CAR4 body<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    glPushMatrix();
    glTranslatef(run[4],0,0);
    glPushMatrix();
    glTranslatef(0.5f,0.1f,0.0f);
    glRotatef(180,0,1,0),
              glBegin(GL_QUADS);
    glColor3f(0.0f,0.5f,0.0f);
    glVertex2f(0.42f, 0.0175f);
    glVertex2f(0.42f, -0.02f);
    glVertex2f(0.32f, -0.02f);
    glVertex2f(0.33f, 0.0175f);

    glEnd();
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.5f,0.0f);
    glVertex2f(0.42f, -0.02f);
    glVertex2f(0.42f, -0.067f);
    glVertex2f(0.3f, -0.067f);
    glVertex2f(0.3f, -0.02f);

    glEnd();


//Light....................................................................
    glBegin(GL_QUADS);
    glColor3f(1.0f,0.038f,0.27f);


    glVertex2f(0.306f, -0.02f);
    glVertex2f(0.309f, -0.032f);

    glVertex2f(0.3f, -0.032f);
    glVertex2f(0.3f, -0.02f);


    glEnd();

    if(night)
    {
        glBegin(GL_QUADS);
        glColor3f(1.0f,0.6f,0.2f);
        glVertex2f(0.3f, -0.025f);
        glVertex2f(0.3f, -0.032f);
        glColor3f(1.0f,1.0f,0.79f);
        glVertex2f(0.28f, -0.067f);
        glVertex2f(0.25f, -0.067f);

        glEnd();
    }

//back light.............................................
    glBegin(GL_QUADS);
    glColor3f(1.0f,0.38f,0.27f);

    glVertex2f(0.42f, -0.04f);
    glVertex2f(0.413f, -0.04);
    glVertex2f(0.413f, -0.054f);
    glVertex2f(0.42f, -0.054f);


    glEnd();
//  //BACK SIDE WHELL.......................................................

    glBegin(GL_QUADS);
    glColor3f(0.8f,1.0f,1.0f);

    glVertex2f(0.42f, -0.018f);
    glVertex2f(0.425f, -0.018f);
    glVertex2f(0.425f, -0.05f);
    glVertex2f(0.42f, -0.053f);


    glEnd();


    //window1..............................

    glBegin(GL_QUADS);
    glColor3f(0.8f,1.0f,1.0f);


    glVertex2f(0.352f, 0.012f);
    glVertex2f(0.352f, -0.02f);
    glVertex2f(0.325f, -0.02f);
    glVertex2f(0.334f, 0.012f);
    glEnd();

    //window2..................
    glBegin(GL_QUADS);
    glColor3f(0.8f,1.0f,1.0f);

    glVertex2f(0.354f, 0.012f);
    glVertex2f(0.354f, -0.02f);
    glVertex2f(0.374f, -0.02f);
    glVertex2f(0.374f, 0.012f);


    glEnd();


    //window3.................................

    glBegin(GL_QUADS);
    glColor3f(0.8f,1.0f,1.0f);

    glVertex2f(0.41f, 0.012f);
    glVertex2f(0.41f, -0.02f);
    glVertex2f(0.376f, -0.02f);
    glVertex2f(0.376f, 0.012f);


    glEnd();
//
    //Door1..........................................


    glBegin(GL_QUADS);
    glColor3f(0.8f,1.0f,1.0f);

    glVertex2f(0.352f, -0.023f);
    glVertex2f(0.352f, -0.027f);
    glVertex2f(0.344f, -0.027f);
    glVertex2f(0.344f, -0.023f);


    glEnd();

    //Door2..........................................................


    glBegin(GL_QUADS);
    glColor3f(0.8f,1.0f,1.0f);

    glVertex2f(0.373f, -0.023f);
    glVertex2f(0.373f, -0.027f);
    glVertex2f(0.367f, -0.027f);
    glVertex2f(0.367f, -0.023f);


    glEnd();

    //wheel1...................................


    GLfloat xc=0.33f;
    GLfloat yc=-0.067f;
    GLfloat radiusc =0.015f;

    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xc, yc); // center of circle
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xc + (radiusc * cos(i *  twicePi / triangleAmount)),
                    yc+ (radiusc * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


    GLfloat xc1=0.33f;
    GLfloat yc1=-0.067f;
    GLfloat radiusc3 =0.008f;

    glColor3f(0.50f, 0.50f, 0.50f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xc1, yc1); // center of circle
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xc1 + (radiusc3 * cos(i *  twicePi / triangleAmount)),
                    yc1 + (radiusc3 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


//wheel2.................................


    GLfloat xc2=0.39f;
    GLfloat yc2=-0.067f;
    GLfloat radiusc4=0.015f;

    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xc2, yc2); // center of circle
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xc2 + (radiusc4 * cos(i *  twicePi / triangleAmount)),
                    yc2 + (radiusc4 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


    GLfloat xc3=0.39f;
    GLfloat yc3=-0.067f;
    GLfloat radiusc5=0.008f;

    glColor3f(0.50f, 0.50f, 0.50f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xc3, yc3); // center of circle
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xc3 + (radiusc5 * cos(i *  twicePi / triangleAmount)),
                    yc3 + (radiusc5 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glLoadIdentity();

    glPopMatrix();


    glPopMatrix();


    //CAR3 body<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    glPushMatrix();
    glTranslatef(run[5],0,0);
    glPushMatrix();
    glTranslatef(-0.45f,0.0f,0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,1.0f);
    glVertex2f(0.4f, 0.0175f);
    glVertex2f(0.42f, -0.02f);
    glVertex2f(0.32f, -0.02f);
    glVertex2f(0.33f, 0.0175f);

    glEnd();
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.0f,1.0f);
    glVertex2f(0.42f, -0.02f);
    glVertex2f(0.42f, -0.067f);
    glVertex2f(0.3f, -0.067f);
    glVertex2f(0.3f, -0.02f);

    glEnd();


//Light....................................................................
    glBegin(GL_QUADS);
    glColor3f(1.0f,0.038f,0.27f);


    glVertex2f(0.306f, -0.02f);
    glVertex2f(0.309f, -0.032f);

    glVertex2f(0.3f, -0.032f);
    glVertex2f(0.3f, -0.02f);


    glEnd();
    if(night)
    {
        glBegin(GL_QUADS);
        glColor3f(1.0f,0.6f,0.2f);
        glVertex2f(0.3f, -0.025f);
        glVertex2f(0.3f, -0.032f);
        glColor3f(1.0f,1.0f,0.79f);
        glVertex2f(0.28f, -0.067f);
        glVertex2f(0.25f, -0.067f);

        glEnd();
    }
//back light.............................................
    glBegin(GL_QUADS);
    glColor3f(1.0f,0.38f,0.27f);

    glVertex2f(0.42f, -0.04f);
    glVertex2f(0.413f, -0.04);
    glVertex2f(0.413f, -0.054f);
    glVertex2f(0.42f, -0.054f);


    glEnd();
//  //BACK SIDE WHELL.......................................................
//
//   glBegin(GL_QUADS);
//    glColor3f(0.8f,1.0f,1.0f);
//
//    glVertex2f(0.42f, -0.018f);
//    glVertex2f(0.425f, -0.018f);
//    glVertex2f(0.425f, -0.05f);
//    glVertex2f(0.42f, -0.053f);glEnd();


    //window1..............................

    glBegin(GL_QUADS);
    glColor3f(0.8f,1.0f,1.0f);


    glVertex2f(0.352f, 0.012f);
    glVertex2f(0.352f, -0.02f);
    glVertex2f(0.325f, -0.02f);
    glVertex2f(0.334f, 0.012f);
    glEnd();

    //window2..................
    glBegin(GL_QUADS);
    glColor3f(0.8f,1.0f,1.0f);

    glVertex2f(0.354f, 0.012f);
    glVertex2f(0.354f, -0.02f);
    glVertex2f(0.374f, -0.02f);
    glVertex2f(0.374f, 0.012f);


    glEnd();


    //window3.................................

    glBegin(GL_QUADS);
    glColor3f(0.8f,1.0f,1.0f);

    glVertex2f(0.394f, 0.012f);
    glVertex2f(0.41f, -0.02f);
    glVertex2f(0.376f, -0.02f);
    glVertex2f(0.376f, 0.012f);


    glEnd();
//
    //Door1..........................................


    glBegin(GL_QUADS);
    glColor3f(0.8f,1.0f,1.0f);

    glVertex2f(0.352f, -0.023f);
    glVertex2f(0.352f, -0.027f);
    glVertex2f(0.344f, -0.027f);
    glVertex2f(0.344f, -0.023f);


    glEnd();

    //Door2..........................................................


    glBegin(GL_QUADS);
    glColor3f(0.8f,1.0f,1.0f);

    glVertex2f(0.373f, -0.023f);
    glVertex2f(0.373f, -0.027f);
    glVertex2f(0.367f, -0.027f);
    glVertex2f(0.367f, -0.023f);


    glEnd();

    //wheel1...................................


    GLfloat xc5=0.33f;
    GLfloat yc5=-0.067f;

    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xc, yc); // center of circle
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xc5 + (radiusc * cos(i *  twicePi / triangleAmount)),
                    yc5+ (radiusc * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


    GLfloat xc6=0.33f;
    GLfloat yc6=-0.067f;

    glColor3f(0.50f, 0.50f, 0.50f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xc6, yc6); // center of circle
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xc6 + (radiusc3 * cos(i *  twicePi / triangleAmount)),
                    yc6 + (radiusc3 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


//wheel2.................................


    GLfloat xc7=0.39f;
    GLfloat yc7=-0.067f;
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xc7, yc7); // center of circle
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xc7 + (radiusc4 * cos(i *  twicePi / triangleAmount)),
                    yc7 + (radiusc4 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


    GLfloat xc8=0.39f;
    GLfloat yc8=-0.067f;
    glColor3f(0.50f, 0.50f, 0.50f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xc8, yc8); // center of circle
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xc8 + (radiusc5 * cos(i *  twicePi / triangleAmount)),
                    yc8 + (radiusc5 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    glPopMatrix();
    glPopMatrix();


//	// bus 02<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>

    glPushMatrix();

    glTranslatef(run[6],0,0);

    glPushMatrix();
    glTranslatef(0.09f,-0.125f, 0.0f);
    glRotatef(180,0,1,0);


    GLfloat xb6=-0.455f;
    GLfloat yb6=0.095f;
    GLfloat radiusb6 = 0.01f;
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xb6, yb6);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xb6 + (radiusb6 * cos(i *  twicePi / triangleAmount)),
                    yb6 + (radiusb6 * sin(i * twicePi / triangleAmount)));
    }
    glEnd();

    if(night)
    {
        glBegin(GL_QUADS);
        glColor3f(1.0f,0.6f,0.2f);
        glVertex2f(-0.445f, 0.097f);
        glVertex2f(-0.455f, 0.095f);
        glColor3f(1.0f,1.0f,0.79f);
        glVertex2f(-0.435f, 0.06f);
        glVertex2f(-0.39f, 0.06f);

        glEnd();
    }

    glBegin(GL_QUADS);
    glColor3f(0.0, 0.4, 0.0);
    glVertex2f(- 0.45, 0.188);
    glVertex2f(- 0.45, 0.06);
    glColor3f(1.0, 0.2, 0.2);
    glVertex2f(- 0.6, 0.06);
    glVertex2f(- 0.6, 0.188);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.00, 0.00, 0.00);
    glVertex2f(- 0.45, 0.077);
    glVertex2f(- 0.45, 0.069);
    glVertex2f(- 0.46, 0.069);
    glVertex2f(- 0.46, 0.077);
    glEnd();

    glLineWidth(6);
    glBegin(GL_LINES);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.455f, 0.17f);
    glVertex2f(- 0.455f, 0.12f);
    glEnd();

    glLineWidth(2);
    glBegin(GL_LINES);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.59f, 0.12f);
    glVertex2f(- 0.47f, 0.12f);
    glEnd();

    glLineWidth(2);
    glBegin(GL_LINES);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.59f, 0.1f);
    glVertex2f(- 0.47f, 0.1f);
    glEnd();

    glBegin(GL_QUADS);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.465, 0.17);
    glVertex2f(- 0.465, 0.14);
    glVertex2f(- 0.48, 0.14);
    glVertex2f(- 0.48, 0.17);
    glEnd();

    glPushMatrix();
    glTranslatef(- 0.022,0,0);
    glBegin(GL_QUADS);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.465, 0.17);
    glVertex2f(- 0.465, 0.14);
    glVertex2f(- 0.48, 0.14);
    glVertex2f(- 0.48, 0.17);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(- 0.044,0,0);
    glBegin(GL_QUADS);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.465, 0.17);
    glVertex2f(- 0.465, 0.14);
    glVertex2f(- 0.48, 0.14);
    glVertex2f(- 0.48, 0.17);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(- 0.066,0,0);
    glBegin(GL_QUADS);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.465, 0.17);
    glVertex2f(- 0.465, 0.14);
    glVertex2f(- 0.48, 0.14);
    glVertex2f(- 0.48, 0.17);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(- 0.088,0,0);
    glBegin(GL_QUADS);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.465, 0.17);
    glVertex2f(- 0.465, 0.14);
    glVertex2f(- 0.48, 0.14);
    glVertex2f(- 0.48, 0.17);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(- 0.11,0,0);
    glBegin(GL_QUADS);
    glColor3f(0.76, 0.77, 0.78);
    glVertex2f(- 0.465, 0.17);
    glVertex2f(- 0.465, 0.14);
    glVertex2f(- 0.48, 0.14);
    glVertex2f(- 0.48, 0.17);
    glEnd();
    glPopMatrix();


    GLfloat xb7=-0.49f;
    GLfloat yb7=0.06f;
    GLfloat radiusb7 = 0.02f;
    glColor3f(0.10, 0.10, 0.10);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xb7, yb7);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xb7 + (radiusb7 * cos(i *  twicePi / triangleAmount)),
                    yb7 + (radiusb7 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


    GLfloat xb8=- 0.49f;
    GLfloat yb8=0.06f;
    GLfloat radiusb8 = 0.014f;
    glColor3f(0.47, 0.46, 0.46 );
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xb8, yb8);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xb8 + (radiusb8 * cos(i *  twicePi / triangleAmount)),
                    yb8 + (radiusb8 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


    GLfloat xb9=- 0.57f;
    GLfloat yb9=0.06f;
    GLfloat radiusb9 = 0.02f;
    glColor3f(0.10, 0.10, 0.10 );
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xb9, yb9);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xb9 + (radiusb9 * cos(i *  twicePi / triangleAmount)),
                    yb9 + (radiusb9 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


    GLfloat xb10=- 0.57f;
    GLfloat yb10=0.06f;
    GLfloat radiusb10 = 0.014f;
    glColor3f(0.47, 0.46, 0.46);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xb10, yb10);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xb10 + (radiusb10 * cos(i *  twicePi / triangleAmount)),
                    yb10 + (radiusb10* sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    glPopMatrix();
    glPopMatrix();

// lamp part02<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

//..

    glPushMatrix();
    glTranslatef(-0.1f,-0.22f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.85f, 0.12f);


        glEnd();
    }
    glPopMatrix();


    //..

    glPushMatrix();
    glTranslatef(0.1f,-0.22f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.85f, 0.12f);


        glEnd();
    }
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslatef(0.3f,-0.22f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.85f, 0.12f);


        glEnd();
    }
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslatef(0.5f,-0.22f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.85f, 0.12f);


        glEnd();
    }
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslatef(0.7f,-0.22f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.85f, 0.12f);


        glEnd();
    }
    glPopMatrix();


    //..

    glPushMatrix();
    glTranslatef(0.9f,-0.22f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.85f, 0.12f);


        glEnd();
    }
    glPopMatrix();


//..

    glPushMatrix();
    glTranslatef(1.1f,-0.22f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.85f, 0.12f);


        glEnd();
    }
    glPopMatrix();


    //..

    glPushMatrix();
    glTranslatef(1.3f,-0.22f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.85f, 0.12f);


        glEnd();
    }
    glPopMatrix();


    //..

    glPushMatrix();
    glTranslatef(1.5f,-0.22f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.85f, 0.12f);


        glEnd();
    }
    glPopMatrix();


    //..

    glPushMatrix();
    glTranslatef(1.7f,-0.22f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);
    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.85f, 0.12f);

        glEnd();
    }
    glPopMatrix();


    //..

    glPushMatrix();
    glTranslatef(1.9f,-0.22f,0.0f);
    glBegin(GL_QUADS);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.9025f, 0.21f);
    glVertex2f(-0.8975f, 0.21f);
    glVertex2f(-0.8975f, 0.11f);
    glVertex2f(-0.9025f, 0.11f);

    glColor3f(0.8f,0.9f,1.0f);
    glVertex2f(-0.915f, 0.21f);
    glVertex2f(-0.885f, 0.21f);
    glVertex2f(-0.885f, 0.216f);
    glVertex2f(-0.915f, 0.216f);

    glEnd();

    glPointSize(4);
    glBegin(GL_POINTS);

    glColor3f(0.0f,0.0f,0.0f);
    if(night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glVertex2f(-0.915f, 0.213f);

    glVertex2f(-0.885f, 0.213f);
    glEnd();
    if(night)
    {
        //night light
        glBegin(GL_TRIANGLES);
        glBegin(GL_TRIANGLES);
        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.915f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.905f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.95f, 0.12f);

        glColor3f(0.9f,0.9f,0.0f);
        glVertex2f(-0.885f, 0.213f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.895f, 0.12f);
        glColor3f(0.2f,0.2f,0.2f);
        glVertex2f(-0.85f, 0.12f);

        glEnd();
    }
    glPopMatrix();

    //bech.......................................................................................
    glBegin(GL_QUADS);
    glColor3f(1.0f,0.71f,0.43f);
    if(night)
    {
        glColor3f(0.8, 0.51, 0.23);
    }

    glVertex2f(-1.0f, -0.45f);
    glVertex2f(1.0f, -0.45f);
    glVertex2f(1.0f, -0.17f);
    glVertex2f(-1.0f, -0.17f);
    glEnd();

    // ============================================================
    // BEACH PEOPLE SCENE
    // ============================================================
    if(!cover)
    {
        // =============================================
        // BEACH PEOPLE - large, clearly visible figures
        // Beach y range: -0.17 (top) to -0.45 (bottom)
        // =============================================

        // ---- LOUNGER 1 (left side, bright orange frame) ----
        glBegin(GL_QUADS);
        glColor3f(0.75f, 0.38f, 0.08f); // wood dark
        glVertex2f(-0.82f, -0.195f);
        glVertex2f(-0.55f, -0.195f);
        glVertex2f(-0.55f, -0.210f);
        glVertex2f(-0.82f, -0.210f);
        // Mattress on lounger
        glColor3f(0.95f, 0.90f, 0.50f); // yellow mat
        glVertex2f(-0.81f, -0.190f);
        glVertex2f(-0.56f, -0.190f);
        glVertex2f(-0.56f, -0.200f);
        glVertex2f(-0.81f, -0.200f);
        // Legs
        glColor3f(0.50f, 0.25f, 0.05f);
        glVertex2f(-0.80f, -0.210f); glVertex2f(-0.78f, -0.210f);
        glVertex2f(-0.78f, -0.225f); glVertex2f(-0.80f, -0.225f);
        glVertex2f(-0.59f, -0.210f); glVertex2f(-0.57f, -0.210f);
        glVertex2f(-0.57f, -0.225f); glVertex2f(-0.59f, -0.225f);
        // Person lying on lounger 1 - bright red swimsuit body
        glColor3f(0.95f, 0.10f, 0.10f);
        glVertex2f(-0.79f, -0.178f);
        glVertex2f(-0.57f, -0.178f);
        glVertex2f(-0.57f, -0.193f);
        glVertex2f(-0.79f, -0.193f);
        glEnd();
        // Person head
        {
            GLfloat hx=-0.545f, hy=-0.183f, hr=0.022f;
            glColor3f(0.88f,0.65f,0.42f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(hx,hy);
            for(int i=0;i<=24;i++) glVertex2f(hx+hr*cos(i*twicePi/24),hy+hr*sin(i*twicePi/24));
            glEnd();
        }
        // Sunglasses
        glColor3f(0.08f,0.08f,0.08f);
        glBegin(GL_QUADS);
        glVertex2f(-0.567f,-0.178f); glVertex2f(-0.527f,-0.178f);
        glVertex2f(-0.527f,-0.186f); glVertex2f(-0.567f,-0.186f);
        glEnd();

        // ---- LOUNGER 2 (center, bright blue frame) ----
        glBegin(GL_QUADS);
        glColor3f(0.10f, 0.28f, 0.82f);
        glVertex2f(-0.15f, -0.197f);
        glVertex2f(0.15f,  -0.197f);
        glVertex2f(0.15f,  -0.212f);
        glVertex2f(-0.15f, -0.212f);
        // Blue mat
        glColor3f(0.40f, 0.70f, 1.00f);
        glVertex2f(-0.14f, -0.192f);
        glVertex2f(0.14f,  -0.192f);
        glVertex2f(0.14f,  -0.200f);
        glVertex2f(-0.14f, -0.200f);
        // Legs
        glColor3f(0.05f, 0.15f, 0.55f);
        glVertex2f(-0.13f,-0.212f); glVertex2f(-0.11f,-0.212f);
        glVertex2f(-0.11f,-0.225f); glVertex2f(-0.13f,-0.225f);
        glVertex2f(0.11f, -0.212f); glVertex2f(0.13f, -0.212f);
        glVertex2f(0.13f, -0.225f); glVertex2f(0.11f, -0.225f);
        // Person lying on lounger 2 - bright green swimsuit
        glColor3f(0.10f, 0.80f, 0.20f);
        glVertex2f(-0.13f, -0.178f);
        glVertex2f(0.13f,  -0.178f);
        glVertex2f(0.13f,  -0.194f);
        glVertex2f(-0.13f, -0.194f);
        glEnd();
        // Head
        {
            GLfloat hx=0.153f, hy=-0.183f, hr=0.022f;
            glColor3f(0.88f,0.65f,0.42f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(hx,hy);
            for(int i=0;i<=24;i++) glVertex2f(hx+hr*cos(i*twicePi/24),hy+hr*sin(i*twicePi/24));
            glEnd();
        }

        // ---- SITTING PERSON 1 (right side, blue shirt) ----
        glBegin(GL_QUADS);
        // Torso
        glColor3f(0.10f, 0.28f, 0.90f);
        glVertex2f(0.310f, -0.195f);
        glVertex2f(0.350f, -0.195f);
        glVertex2f(0.350f, -0.250f);
        glVertex2f(0.310f, -0.250f);
        // Bent legs (knees up)
        glColor3f(0.85f,0.66f,0.43f); // skin
        glVertex2f(0.305f, -0.250f);
        glVertex2f(0.355f, -0.250f);
        glVertex2f(0.355f, -0.275f);
        glVertex2f(0.305f, -0.275f);
        // Arms spread
        glColor3f(0.10f, 0.28f, 0.90f);
        glVertex2f(0.280f, -0.205f);
        glVertex2f(0.310f, -0.205f);
        glVertex2f(0.310f, -0.220f);
        glVertex2f(0.280f, -0.220f);
        glVertex2f(0.350f, -0.205f);
        glVertex2f(0.378f, -0.205f);
        glVertex2f(0.378f, -0.220f);
        glVertex2f(0.350f, -0.220f);
        glEnd();
        // Head
        {
            GLfloat hx=0.330f, hy=-0.180f, hr=0.022f;
            glColor3f(0.88f,0.65f,0.42f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(hx,hy);
            for(int i=0;i<=24;i++) glVertex2f(hx+hr*cos(i*twicePi/24),hy+hr*sin(i*twicePi/24));
            glEnd();
        }

        // ---- SITTING PERSON 2 (left-center, pink top) ----
        glBegin(GL_QUADS);
        // Torso
        glColor3f(0.92f, 0.30f, 0.55f);
        glVertex2f(-0.410f, -0.195f);
        glVertex2f(-0.370f, -0.195f);
        glVertex2f(-0.370f, -0.248f);
        glVertex2f(-0.410f, -0.248f);
        // Bent legs
        glColor3f(0.85f,0.66f,0.43f);
        glVertex2f(-0.415f, -0.248f);
        glVertex2f(-0.365f, -0.248f);
        glVertex2f(-0.365f, -0.272f);
        glVertex2f(-0.415f, -0.272f);
        // Long hair
        glColor3f(0.15f, 0.07f, 0.01f);
        glVertex2f(-0.415f, -0.195f);
        glVertex2f(-0.368f, -0.195f);
        glVertex2f(-0.368f, -0.220f);
        glVertex2f(-0.415f, -0.220f);
        glEnd();
        {
            GLfloat hx=-0.390f, hy=-0.178f, hr=0.022f;
            glColor3f(0.88f,0.65f,0.42f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(hx,hy);
            for(int i=0;i<=24;i++) glVertex2f(hx+hr*cos(i*twicePi/24),hy+hr*sin(i*twicePi/24));
            glEnd();
        }

        // ---- SHELL SELLER 1 (brown shirt, basket, animated _sell1X) ----
        glPushMatrix();
        glTranslatef(_sell1X, 0.0f, 0.0f);
        glBegin(GL_QUADS);
        // Shirt
        glColor3f(0.72f, 0.35f, 0.08f);
        glVertex2f(-0.018f, -0.192f);
        glVertex2f( 0.018f, -0.192f);
        glVertex2f( 0.018f, -0.240f);
        glVertex2f(-0.018f, -0.240f);
        // Trousers dark blue
        glColor3f(0.12f, 0.12f, 0.45f);
        glVertex2f(-0.016f, -0.240f); glVertex2f(-0.003f, -0.240f);
        glVertex2f(-0.003f, -0.272f); glVertex2f(-0.016f, -0.272f);
        glVertex2f( 0.003f, -0.240f); glVertex2f( 0.016f, -0.240f);
        glVertex2f( 0.016f, -0.272f); glVertex2f( 0.003f, -0.272f);
        // Basket extended right
        glColor3f(0.58f, 0.35f, 0.08f);
        glVertex2f( 0.018f, -0.200f); glVertex2f( 0.042f, -0.200f);
        glVertex2f( 0.042f, -0.220f); glVertex2f( 0.018f, -0.220f);
        // Basket handle
        glColor3f(0.38f, 0.20f, 0.04f);
        glVertex2f( 0.022f, -0.195f); glVertex2f( 0.038f, -0.195f);
        glVertex2f( 0.038f, -0.200f); glVertex2f( 0.022f, -0.200f);
        // Hat brim
        glColor3f(0.48f, 0.28f, 0.06f);
        glVertex2f(-0.028f, -0.172f); glVertex2f( 0.028f, -0.172f);
        glVertex2f( 0.028f, -0.177f); glVertex2f(-0.028f, -0.177f);
        glEnd();
        // Head
        {
            GLfloat hx=0.0f, hy=-0.175f, hr=0.020f;
            glColor3f(0.72f,0.50f,0.30f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(hx,hy);
            for(int i=0;i<=24;i++) glVertex2f(hx+hr*cos(i*twicePi/24),hy+hr*sin(i*twicePi/24));
            glEnd();
        }
        // Arm reaching to basket
        glColor3f(0.72f, 0.35f, 0.08f);
        glBegin(GL_QUADS);
        glVertex2f( 0.018f, -0.198f); glVertex2f( 0.034f, -0.198f);
        glVertex2f( 0.034f, -0.208f); glVertex2f( 0.018f, -0.208f);
        glEnd();
        glPopMatrix();

        // ---- SHELL SELLER 2 (green shirt, tray on head, animated _sell2X) ----
        glPushMatrix();
        glTranslatef(_sell2X, 0.0f, 0.0f);
        glBegin(GL_QUADS);
        // Shirt
        glColor3f(0.15f, 0.60f, 0.22f);
        glVertex2f(-0.016f, -0.194f);
        glVertex2f( 0.016f, -0.194f);
        glVertex2f( 0.016f, -0.240f);
        glVertex2f(-0.016f, -0.240f);
        // Trousers
        glColor3f(0.35f, 0.18f, 0.06f);
        glVertex2f(-0.015f,-0.240f); glVertex2f(-0.002f,-0.240f);
        glVertex2f(-0.002f,-0.270f); glVertex2f(-0.015f,-0.270f);
        glVertex2f( 0.002f,-0.240f); glVertex2f( 0.015f,-0.240f);
        glVertex2f( 0.015f,-0.270f); glVertex2f( 0.002f,-0.270f);
        // Arms up to hold tray
        glColor3f(0.15f, 0.60f, 0.22f);
        glVertex2f(-0.016f,-0.200f); glVertex2f(-0.028f,-0.200f);
        glVertex2f(-0.028f,-0.182f); glVertex2f(-0.016f,-0.182f);
        glVertex2f( 0.016f,-0.200f); glVertex2f( 0.028f,-0.200f);
        glVertex2f( 0.028f,-0.182f); glVertex2f( 0.016f,-0.182f);
        // Tray (wide flat)
        glColor3f(0.85f, 0.82f, 0.48f);
        glVertex2f(-0.034f,-0.178f); glVertex2f( 0.034f,-0.178f);
        glVertex2f( 0.034f,-0.183f); glVertex2f(-0.034f,-0.183f);
        glEnd();
        // Head (under tray)
        {
            GLfloat hx=0.0f, hy=-0.186f, hr=0.018f;
            glColor3f(0.72f,0.50f,0.30f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(hx,hy);
            for(int i=0;i<=24;i++) glVertex2f(hx+hr*cos(i*twicePi/24),hy+hr*sin(i*twicePi/24));
            glEnd();
        }
        glPopMatrix();

        // ---- HORSE + PERSON WALKING (animated _horseX) ----
        glPushMatrix();
        glTranslatef(_horseX, 0.0f, 0.0f);

        // Horse body (brown)
        glColor3f(0.55f, 0.30f, 0.10f);
        glBegin(GL_QUADS);
        glVertex2f(-0.065f,-0.215f); glVertex2f( 0.065f,-0.215f);
        glVertex2f( 0.065f,-0.260f); glVertex2f(-0.065f,-0.260f);
        glEnd();
        // Neck
        glColor3f(0.55f, 0.30f, 0.10f);
        glBegin(GL_QUADS);
        glVertex2f( 0.042f,-0.193f); glVertex2f( 0.065f,-0.193f);
        glVertex2f( 0.065f,-0.215f); glVertex2f( 0.042f,-0.215f);
        glEnd();
        // Head
        {
            GLfloat hx=0.070f, hy=-0.188f, hr=0.022f;
            glColor3f(0.55f,0.30f,0.10f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(hx,hy);
            for(int i=0;i<=24;i++) glVertex2f(hx+hr*cos(i*twicePi/24),hy+hr*sin(i*twicePi/24));
            glEnd();
        }
        // Legs (4)
        glColor3f(0.38f, 0.18f, 0.06f);
        glBegin(GL_QUADS);
        glVertex2f(-0.058f,-0.260f); glVertex2f(-0.044f,-0.260f);
        glVertex2f(-0.044f,-0.290f); glVertex2f(-0.058f,-0.290f);
        glVertex2f(-0.022f,-0.260f); glVertex2f(-0.008f,-0.260f);
        glVertex2f(-0.008f,-0.290f); glVertex2f(-0.022f,-0.290f);
        glVertex2f( 0.008f,-0.260f); glVertex2f( 0.022f,-0.260f);
        glVertex2f( 0.022f,-0.290f); glVertex2f( 0.008f,-0.290f);
        glVertex2f( 0.044f,-0.260f); glVertex2f( 0.058f,-0.260f);
        glVertex2f( 0.058f,-0.290f); glVertex2f( 0.044f,-0.290f);
        // Mane (dark)
        glColor3f(0.18f, 0.08f, 0.02f);
        glVertex2f( 0.044f,-0.193f); glVertex2f( 0.065f,-0.193f);
        glVertex2f( 0.065f,-0.210f); glVertex2f( 0.044f,-0.210f);
        // Tail
        glVertex2f(-0.068f,-0.220f); glVertex2f(-0.050f,-0.220f);
        glVertex2f(-0.046f,-0.250f); glVertex2f(-0.072f,-0.250f);
        glEnd();

        // Person walking horse (blue shirt, holding rope)
        glBegin(GL_QUADS);
        // Shirt
        glColor3f(0.15f, 0.42f, 0.85f);
        glVertex2f(-0.105f,-0.198f); glVertex2f(-0.078f,-0.198f);
        glVertex2f(-0.078f,-0.248f); glVertex2f(-0.105f,-0.248f);
        // Trousers
        glColor3f(0.15f, 0.15f, 0.38f);
        glVertex2f(-0.104f,-0.248f); glVertex2f(-0.091f,-0.248f);
        glVertex2f(-0.091f,-0.278f); glVertex2f(-0.104f,-0.278f);
        glVertex2f(-0.091f,-0.248f); glVertex2f(-0.078f,-0.248f);
        glVertex2f(-0.078f,-0.278f); glVertex2f(-0.091f,-0.278f);
        // Arm extending rope toward horse
        glColor3f(0.15f,0.42f,0.85f);
        glVertex2f(-0.078f,-0.210f); glVertex2f(-0.058f,-0.210f);
        glVertex2f(-0.058f,-0.224f); glVertex2f(-0.078f,-0.224f);
        glEnd();
        // Rope (thick line)
        glColor3f(0.50f, 0.32f, 0.08f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(-0.058f,-0.217f);
        glVertex2f( 0.040f,-0.217f);
        glEnd();
        glLineWidth(1.0f);
        // Head of horse walker
        {
            GLfloat hx=-0.092f, hy=-0.185f, hr=0.020f;
            glColor3f(0.88f,0.65f,0.42f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(hx,hy);
            for(int i=0;i<=24;i++) glVertex2f(hx+hr*cos(i*twicePi/24),hy+hr*sin(i*twicePi/24));
            glEnd();
        }
        glPopMatrix();

        // ============================================================
        // ===== NEW ADDED FEATURES (people + objects) ================
        // ============================================================

        // ---- COCONUT TREE LEFT (narikel gach) ----
        {
            float tx = -0.95f, ty = -0.20f;
            // Trunk
            glColor3f(0.45f, 0.27f, 0.10f);
            glBegin(GL_QUADS);
            glVertex2f(tx-0.020f, ty);
            glVertex2f(tx+0.020f, ty);
            glVertex2f(tx+0.014f, ty+0.40f);
            glVertex2f(tx-0.014f, ty+0.40f);
            glEnd();
            // Trunk ring details
            glColor3f(0.30f, 0.18f, 0.05f);
            glLineWidth(1.5f);
            glBegin(GL_LINES);
            for(int k=1;k<=7;k++){
                float yy = ty + k*0.055f;
                glVertex2f(tx-0.018f, yy);
                glVertex2f(tx+0.018f, yy);
            }
            glEnd();
            glLineWidth(1.0f);
            // Leaves (animated sway)
            glPushMatrix();
            glTranslatef(tx, ty+0.40f, 0.0f);
            glRotatef(_leafSway, 0.0f, 0.0f, 1.0f);
            glColor3f(0.10f, 0.55f, 0.15f);
            for(int k=0;k<7;k++){
                float a = (k * 360.0f / 7.0f) * PI / 180.0f;
                float len = 0.13f;
                float ex = cos(a)*len;
                float ey = sin(a)*len*0.6f + 0.02f;
                glBegin(GL_TRIANGLES);
                glVertex2f(0.0f, 0.0f);
                glVertex2f(ex, ey);
                glVertex2f(ex*0.5f - 0.02f, ey + 0.025f);
                glEnd();
                glBegin(GL_TRIANGLES);
                glVertex2f(0.0f, 0.0f);
                glVertex2f(ex, ey);
                glVertex2f(ex*0.5f + 0.02f, ey - 0.025f);
                glEnd();
            }
            // Coconuts cluster
            glColor3f(0.30f, 0.18f, 0.05f);
            for(int k=0;k<3;k++){
                float ccx = -0.012f + k*0.012f;
                float ccy = -0.010f;
                float ccr = 0.008f;
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(ccx,ccy);
                for(int i=0;i<=12;i++) glVertex2f(ccx+ccr*cos(i*twicePi/12),ccy+ccr*sin(i*twicePi/12));
                glEnd();
            }
            glPopMatrix();
        }

        // ---- COCONUT TREE RIGHT (narikel gach) ----
        {
            float tx = 0.93f, ty = -0.20f;
            glColor3f(0.45f, 0.27f, 0.10f);
            glBegin(GL_QUADS);
            glVertex2f(tx-0.020f, ty);
            glVertex2f(tx+0.020f, ty);
            glVertex2f(tx+0.014f, ty+0.36f);
            glVertex2f(tx-0.014f, ty+0.36f);
            glEnd();
            glColor3f(0.30f, 0.18f, 0.05f);
            glLineWidth(1.5f);
            glBegin(GL_LINES);
            for(int k=1;k<=6;k++){
                float yy = ty + k*0.057f;
                glVertex2f(tx-0.018f, yy);
                glVertex2f(tx+0.018f, yy);
            }
            glEnd();
            glLineWidth(1.0f);
            glPushMatrix();
            glTranslatef(tx, ty+0.36f, 0.0f);
            glRotatef(-_leafSway, 0.0f, 0.0f, 1.0f);
            glColor3f(0.08f, 0.50f, 0.14f);
            for(int k=0;k<7;k++){
                float a = (k * 360.0f / 7.0f) * PI / 180.0f;
                float len = 0.12f;
                float ex = cos(a)*len;
                float ey = sin(a)*len*0.6f + 0.02f;
                glBegin(GL_TRIANGLES);
                glVertex2f(0.0f, 0.0f);
                glVertex2f(ex, ey);
                glVertex2f(ex*0.5f - 0.02f, ey + 0.025f);
                glEnd();
                glBegin(GL_TRIANGLES);
                glVertex2f(0.0f, 0.0f);
                glVertex2f(ex, ey);
                glVertex2f(ex*0.5f + 0.02f, ey - 0.025f);
                glEnd();
            }
            glColor3f(0.30f, 0.18f, 0.05f);
            for(int k=0;k<3;k++){
                float ccx = -0.012f + k*0.012f;
                float ccy = -0.010f;
                float ccr = 0.008f;
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(ccx,ccy);
                for(int i=0;i<=12;i++) glVertex2f(ccx+ccr*cos(i*twicePi/12),ccy+ccr*sin(i*twicePi/12));
                glEnd();
            }
            glPopMatrix();
        }

        // ---- BEACH UMBRELLA + CHAIR ----
        {
            float ux = 0.46f;
            // Chair frame (wood) - seat and tilted backrest
            glColor3f(0.55f, 0.32f, 0.10f);
            glBegin(GL_QUADS);
            glVertex2f(ux-0.040f, -0.232f);
            glVertex2f(ux+0.040f, -0.232f);
            glVertex2f(ux+0.040f, -0.242f);
            glVertex2f(ux-0.040f, -0.242f);
            // backrest
            glVertex2f(ux+0.030f, -0.232f);
            glVertex2f(ux+0.040f, -0.232f);
            glVertex2f(ux+0.062f, -0.188f);
            glVertex2f(ux+0.052f, -0.188f);
            glEnd();
            // legs
            glColor3f(0.40f, 0.22f, 0.06f);
            glBegin(GL_QUADS);
            glVertex2f(ux-0.038f,-0.242f); glVertex2f(ux-0.028f,-0.242f);
            glVertex2f(ux-0.028f,-0.270f); glVertex2f(ux-0.038f,-0.270f);
            glVertex2f(ux+0.028f,-0.242f); glVertex2f(ux+0.038f,-0.242f);
            glVertex2f(ux+0.038f,-0.270f); glVertex2f(ux+0.028f,-0.270f);
            glEnd();
            // Chair striped fabric (blue + white)
            glBegin(GL_QUADS);
            for(int k=0;k<4;k++){
                if(k%2==0) glColor3f(0.20f, 0.45f, 0.92f);
                else       glColor3f(0.98f, 0.98f, 0.98f);
                float y1 = -0.232f - k*0.0025f;
                float y2 = y1 - 0.0025f;
                glVertex2f(ux-0.038f, y1); glVertex2f(ux+0.038f, y1);
                glVertex2f(ux+0.038f, y2); glVertex2f(ux-0.038f, y2);
            }
            glEnd();
            // Umbrella pole
            glColor3f(0.35f, 0.20f, 0.05f);
            glBegin(GL_QUADS);
            glVertex2f(ux-0.005f, -0.225f);
            glVertex2f(ux+0.005f, -0.225f);
            glVertex2f(ux+0.005f, -0.075f);
            glVertex2f(ux-0.005f, -0.075f);
            glEnd();
            // Umbrella canopy (striped, arched)
            int segs = 8;
            float Rx = 0.13f, Ry = 0.045f;
            float uy_top = -0.078f;
            for(int k=0;k<segs;k++){
                if(k%2==0) glColor3f(0.95f, 0.18f, 0.18f);
                else       glColor3f(0.98f, 0.98f, 0.92f);
                float t1 = (k    /(float)segs);
                float t2 = ((k+1)/(float)segs);
                float x1 = ux - Rx + t1*2.0f*Rx;
                float x2 = ux - Rx + t2*2.0f*Rx;
                float y1 = uy_top - 0.005f + Ry * (1.0f - (t1*2.0f-1.0f)*(t1*2.0f-1.0f));
                float y2 = uy_top - 0.005f + Ry * (1.0f - (t2*2.0f-1.0f)*(t2*2.0f-1.0f));
                glBegin(GL_TRIANGLES);
                glVertex2f(ux, uy_top + Ry);
                glVertex2f(x1, y1);
                glVertex2f(x2, y2);
                glEnd();
            }
            // Tip on top of umbrella
            glColor3f(0.35f, 0.20f, 0.05f);
            glBegin(GL_QUADS);
            glVertex2f(ux-0.003f, -0.030f);
            glVertex2f(ux+0.003f, -0.030f);
            glVertex2f(ux+0.003f, -0.040f);
            glVertex2f(ux-0.003f, -0.040f);
            glEnd();
        }

        // ---- CHILDREN PLAYING + SANDCASTLE ----
        {
            float cx = 0.66f;
            // Sandcastle - 3 towers
            glColor3f(0.85f, 0.72f, 0.45f);
            glBegin(GL_QUADS);
            glVertex2f(cx-0.025f, -0.310f);
            glVertex2f(cx+0.025f, -0.310f);
            glVertex2f(cx+0.025f, -0.265f);
            glVertex2f(cx-0.025f, -0.265f);
            glVertex2f(cx-0.060f, -0.300f);
            glVertex2f(cx-0.030f, -0.300f);
            glVertex2f(cx-0.030f, -0.275f);
            glVertex2f(cx-0.060f, -0.275f);
            glVertex2f(cx+0.030f, -0.300f);
            glVertex2f(cx+0.060f, -0.300f);
            glVertex2f(cx+0.060f, -0.275f);
            glVertex2f(cx+0.030f, -0.275f);
            glEnd();
            // Tower tops (triangles)
            glColor3f(0.70f, 0.55f, 0.30f);
            glBegin(GL_TRIANGLES);
            glVertex2f(cx-0.025f, -0.265f); glVertex2f(cx+0.025f, -0.265f); glVertex2f(cx, -0.240f);
            glVertex2f(cx-0.060f, -0.275f); glVertex2f(cx-0.030f, -0.275f); glVertex2f(cx-0.045f, -0.255f);
            glVertex2f(cx+0.030f, -0.275f); glVertex2f(cx+0.060f, -0.275f); glVertex2f(cx+0.045f, -0.255f);
            glEnd();
            // Tiny flag
            glColor3f(0.30f, 0.18f, 0.05f);
            glLineWidth(1.5f);
            glBegin(GL_LINES);
            glVertex2f(cx, -0.240f); glVertex2f(cx, -0.222f);
            glEnd();
            glLineWidth(1.0f);
            glColor3f(0.95f, 0.10f, 0.10f);
            glBegin(GL_TRIANGLES);
            glVertex2f(cx, -0.222f); glVertex2f(cx+0.018f, -0.227f); glVertex2f(cx, -0.232f);
            glEnd();

            // Child 1 (jumping, pink shirt, pigtails) -- LEFT of castle
            float ch1x = cx - 0.095f;
            float ch1y = -0.245f + _childJump;
            glColor3f(0.95f, 0.30f, 0.55f);
            glBegin(GL_QUADS);
            glVertex2f(ch1x-0.013f, ch1y);
            glVertex2f(ch1x+0.013f, ch1y);
            glVertex2f(ch1x+0.013f, ch1y-0.026f);
            glVertex2f(ch1x-0.013f, ch1y-0.026f);
            // shorts
            glColor3f(0.20f, 0.40f, 0.85f);
            glVertex2f(ch1x-0.013f, ch1y-0.026f);
            glVertex2f(ch1x+0.013f, ch1y-0.026f);
            glVertex2f(ch1x+0.013f, ch1y-0.040f);
            glVertex2f(ch1x-0.013f, ch1y-0.040f);
            // legs
            glColor3f(0.85f, 0.62f, 0.40f);
            glVertex2f(ch1x-0.011f, ch1y-0.040f); glVertex2f(ch1x-0.002f, ch1y-0.040f);
            glVertex2f(ch1x-0.002f, ch1y-0.058f); glVertex2f(ch1x-0.011f, ch1y-0.058f);
            glVertex2f(ch1x+0.002f, ch1y-0.040f); glVertex2f(ch1x+0.011f, ch1y-0.040f);
            glVertex2f(ch1x+0.011f, ch1y-0.058f); glVertex2f(ch1x+0.002f, ch1y-0.058f);
            // raised arms
            glColor3f(0.95f, 0.30f, 0.55f);
            glVertex2f(ch1x-0.026f, ch1y+0.008f); glVertex2f(ch1x-0.013f, ch1y+0.008f);
            glVertex2f(ch1x-0.013f, ch1y-0.005f); glVertex2f(ch1x-0.026f, ch1y-0.005f);
            glVertex2f(ch1x+0.013f, ch1y+0.008f); glVertex2f(ch1x+0.026f, ch1y+0.008f);
            glVertex2f(ch1x+0.026f, ch1y-0.005f); glVertex2f(ch1x+0.013f, ch1y-0.005f);
            glEnd();
            // Head
            {
                GLfloat hx=ch1x, hy=ch1y+0.018f, hr=0.015f;
                glColor3f(0.88f,0.65f,0.42f);
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(hx,hy);
                for(int i=0;i<=24;i++) glVertex2f(hx+hr*cos(i*twicePi/24),hy+hr*sin(i*twicePi/24));
                glEnd();
            }
            // Pigtails
            glColor3f(0.18f, 0.08f, 0.02f);
            glBegin(GL_QUADS);
            glVertex2f(ch1x-0.020f, ch1y+0.027f); glVertex2f(ch1x-0.012f, ch1y+0.027f);
            glVertex2f(ch1x-0.012f, ch1y+0.010f); glVertex2f(ch1x-0.020f, ch1y+0.010f);
            glVertex2f(ch1x+0.012f, ch1y+0.027f); glVertex2f(ch1x+0.020f, ch1y+0.027f);
            glVertex2f(ch1x+0.020f, ch1y+0.010f); glVertex2f(ch1x+0.012f, ch1y+0.010f);
            glEnd();

            // Child 2 (squatting, green shirt) -- RIGHT of castle
            float ch2x = cx + 0.090f;
            float ch2y = -0.260f;
            glColor3f(0.20f, 0.80f, 0.30f);
            glBegin(GL_QUADS);
            glVertex2f(ch2x-0.013f, ch2y);
            glVertex2f(ch2x+0.013f, ch2y);
            glVertex2f(ch2x+0.013f, ch2y-0.024f);
            glVertex2f(ch2x-0.013f, ch2y-0.024f);
            // shorts
            glColor3f(0.85f, 0.25f, 0.10f);
            glVertex2f(ch2x-0.013f, ch2y-0.024f);
            glVertex2f(ch2x+0.013f, ch2y-0.024f);
            glVertex2f(ch2x+0.013f, ch2y-0.038f);
            glVertex2f(ch2x-0.013f, ch2y-0.038f);
            // squat legs
            glColor3f(0.85f, 0.62f, 0.40f);
            glVertex2f(ch2x-0.013f, ch2y-0.038f); glVertex2f(ch2x+0.013f, ch2y-0.038f);
            glVertex2f(ch2x+0.013f, ch2y-0.046f); glVertex2f(ch2x-0.013f, ch2y-0.046f);
            // arm reaching to castle (left)
            glColor3f(0.20f, 0.80f, 0.30f);
            glVertex2f(ch2x-0.030f, ch2y-0.005f); glVertex2f(ch2x-0.013f, ch2y-0.005f);
            glVertex2f(ch2x-0.013f, ch2y-0.016f); glVertex2f(ch2x-0.030f, ch2y-0.016f);
            glEnd();
            // Head
            {
                GLfloat hx=ch2x, hy=ch2y+0.018f, hr=0.015f;
                glColor3f(0.88f,0.65f,0.42f);
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(hx,hy);
                for(int i=0;i<=24;i++) glVertex2f(hx+hr*cos(i*twicePi/24),hy+hr*sin(i*twicePi/24));
                glEnd();
            }
            // Short hair
            glColor3f(0.10f, 0.06f, 0.02f);
            glBegin(GL_QUADS);
            glVertex2f(ch2x-0.015f, ch2y+0.030f); glVertex2f(ch2x+0.015f, ch2y+0.030f);
            glVertex2f(ch2x+0.015f, ch2y+0.022f); glVertex2f(ch2x-0.015f, ch2y+0.022f);
            glEnd();
        }

        // ---- COUPLE HOLDING HANDS (animated _coupleX) ----
        glPushMatrix();
        glTranslatef(_coupleX, 0.0f, 0.0f);
        {
            // Person A (left, blue dress, long hair)
            float ax = -0.220f, ay = -0.205f;
            glColor3f(0.20f, 0.40f, 0.85f);
            glBegin(GL_QUADS);
            glVertex2f(ax-0.014f, ay);
            glVertex2f(ax+0.014f, ay);
            glVertex2f(ax+0.022f, ay-0.060f);
            glVertex2f(ax-0.022f, ay-0.060f);
            // legs
            glColor3f(0.88f, 0.65f, 0.42f);
            glVertex2f(ax-0.012f, ay-0.060f); glVertex2f(ax-0.002f, ay-0.060f);
            glVertex2f(ax-0.002f, ay-0.078f); glVertex2f(ax-0.012f, ay-0.078f);
            glVertex2f(ax+0.002f, ay-0.060f); glVertex2f(ax+0.012f, ay-0.060f);
            glVertex2f(ax+0.012f, ay-0.078f); glVertex2f(ax+0.002f, ay-0.078f);
            // arm to right (holding hand)
            glColor3f(0.20f, 0.40f, 0.85f);
            glVertex2f(ax+0.014f, ay-0.005f); glVertex2f(ax+0.030f, ay-0.005f);
            glVertex2f(ax+0.030f, ay-0.018f); glVertex2f(ax+0.014f, ay-0.018f);
            glEnd();
            // Head A
            {
                GLfloat hx=ax, hy=ay+0.020f, hr=0.020f;
                glColor3f(0.88f,0.65f,0.42f);
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(hx,hy);
                for(int i=0;i<=24;i++) glVertex2f(hx+hr*cos(i*twicePi/24),hy+hr*sin(i*twicePi/24));
                glEnd();
            }
            // Long hair
            glColor3f(0.12f, 0.06f, 0.02f);
            glBegin(GL_QUADS);
            glVertex2f(ax-0.022f, ay+0.034f); glVertex2f(ax+0.022f, ay+0.034f);
            glVertex2f(ax+0.020f, ay-0.005f); glVertex2f(ax-0.020f, ay-0.005f);
            glEnd();

            // Person B (right, white shirt, dark trousers)
            float bx = -0.140f, by = -0.205f;
            glColor3f(0.95f, 0.95f, 0.95f);
            glBegin(GL_QUADS);
            glVertex2f(bx-0.014f, by);
            glVertex2f(bx+0.014f, by);
            glVertex2f(bx+0.014f, by-0.042f);
            glVertex2f(bx-0.014f, by-0.042f);
            // trousers
            glColor3f(0.18f, 0.18f, 0.30f);
            glVertex2f(bx-0.014f, by-0.042f); glVertex2f(bx-0.001f, by-0.042f);
            glVertex2f(bx-0.001f, by-0.078f); glVertex2f(bx-0.014f, by-0.078f);
            glVertex2f(bx+0.001f, by-0.042f); glVertex2f(bx+0.014f, by-0.042f);
            glVertex2f(bx+0.014f, by-0.078f); glVertex2f(bx+0.001f, by-0.078f);
            // arm to left (holding hand)
            glColor3f(0.95f, 0.95f, 0.95f);
            glVertex2f(bx-0.030f, by-0.005f); glVertex2f(bx-0.014f, by-0.005f);
            glVertex2f(bx-0.014f, by-0.018f); glVertex2f(bx-0.030f, by-0.018f);
            glEnd();
            // Head B
            {
                GLfloat hx=bx, hy=by+0.020f, hr=0.020f;
                glColor3f(0.88f,0.65f,0.42f);
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(hx,hy);
                for(int i=0;i<=24;i++) glVertex2f(hx+hr*cos(i*twicePi/24),hy+hr*sin(i*twicePi/24));
                glEnd();
            }
            // Short dark hair
            glColor3f(0.10f, 0.06f, 0.02f);
            glBegin(GL_QUADS);
            glVertex2f(bx-0.020f, by+0.034f); glVertex2f(bx+0.020f, by+0.034f);
            glVertex2f(bx+0.020f, by+0.024f); glVertex2f(bx-0.020f, by+0.024f);
            glEnd();

            // Tiny heart above couple
            float hxx = (ax + bx) / 2.0f;
            float hyy = ay + 0.085f;
            glColor3f(0.95f, 0.10f, 0.30f);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(hxx-0.005f, hyy+0.002f);
            for(int i=0;i<=12;i++) glVertex2f((hxx-0.005f)+0.006f*cos(i*twicePi/12), (hyy+0.002f)+0.005f*sin(i*twicePi/12));
            glEnd();
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(hxx+0.005f, hyy+0.002f);
            for(int i=0;i<=12;i++) glVertex2f((hxx+0.005f)+0.006f*cos(i*twicePi/12), (hyy+0.002f)+0.005f*sin(i*twicePi/12));
            glEnd();
            glBegin(GL_TRIANGLES);
            glVertex2f(hxx-0.010f, hyy);
            glVertex2f(hxx+0.010f, hyy);
            glVertex2f(hxx, hyy-0.012f);
            glEnd();
        }
        glPopMatrix();

        // ---- PHOTOGRAPHER (with tripod camera, animated flash) ----
        {
            float px = -0.480f;
            // Tripod legs
            glColor3f(0.20f, 0.20f, 0.22f);
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glVertex2f(px, -0.205f); glVertex2f(px-0.020f, -0.262f);
            glVertex2f(px, -0.205f); glVertex2f(px+0.000f, -0.262f);
            glVertex2f(px, -0.205f); glVertex2f(px+0.020f, -0.262f);
            glEnd();
            glLineWidth(1.0f);
            // Camera body
            glColor3f(0.10f, 0.10f, 0.10f);
            glBegin(GL_QUADS);
            glVertex2f(px-0.022f, -0.193f);
            glVertex2f(px+0.022f, -0.193f);
            glVertex2f(px+0.022f, -0.215f);
            glVertex2f(px-0.022f, -0.215f);
            glEnd();
            // Lens
            glColor3f(0.30f, 0.30f, 0.32f);
            glBegin(GL_QUADS);
            glVertex2f(px+0.020f, -0.198f);
            glVertex2f(px+0.038f, -0.198f);
            glVertex2f(px+0.038f, -0.212f);
            glVertex2f(px+0.020f, -0.212f);
            glEnd();
            // Flash bulb on top
            glColor3f(0.85f, 0.85f, 0.85f);
            glBegin(GL_QUADS);
            glVertex2f(px-0.005f, -0.187f); glVertex2f(px+0.005f, -0.187f);
            glVertex2f(px+0.005f, -0.193f); glVertex2f(px-0.005f, -0.193f);
            glEnd();
            // Flash burst when active
            if(_flashOn){
                glColor3f(1.0f, 1.0f, 0.85f);
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(px+0.045f, -0.205f);
                for(int i=0;i<=24;i++) glVertex2f(px+0.045f+0.040f*cos(i*twicePi/24), -0.205f+0.040f*sin(i*twicePi/24));
                glEnd();
                // flash rays
                glColor3f(1.0f, 1.0f, 0.95f);
                glLineWidth(2.0f);
                glBegin(GL_LINES);
                for(int k=0;k<8;k++){
                    float aa = k * twicePi / 8.0f;
                    glVertex2f(px+0.045f, -0.205f);
                    glVertex2f(px+0.045f + 0.065f*cos(aa), -0.205f + 0.065f*sin(aa));
                }
                glEnd();
                glLineWidth(1.0f);
            }

            // Photographer body (behind the tripod, slight left)
            float pgx = px - 0.050f;
            glColor3f(0.55f, 0.20f, 0.55f);
            glBegin(GL_QUADS);
            glVertex2f(pgx-0.015f, -0.198f);
            glVertex2f(pgx+0.015f, -0.198f);
            glVertex2f(pgx+0.015f, -0.245f);
            glVertex2f(pgx-0.015f, -0.245f);
            // trousers
            glColor3f(0.15f, 0.15f, 0.20f);
            glVertex2f(pgx-0.014f,-0.245f); glVertex2f(pgx-0.001f,-0.245f);
            glVertex2f(pgx-0.001f,-0.275f); glVertex2f(pgx-0.014f,-0.275f);
            glVertex2f(pgx+0.001f,-0.245f); glVertex2f(pgx+0.014f,-0.245f);
            glVertex2f(pgx+0.014f,-0.275f); glVertex2f(pgx+0.001f,-0.275f);
            // Arm reaching toward camera
            glColor3f(0.55f, 0.20f, 0.55f);
            glVertex2f(pgx+0.015f, -0.200f); glVertex2f(pgx+0.042f, -0.200f);
            glVertex2f(pgx+0.042f, -0.213f); glVertex2f(pgx+0.015f, -0.213f);
            glEnd();
            // Head
            {
                GLfloat hx=pgx, hy=-0.180f, hr=0.020f;
                glColor3f(0.88f,0.65f,0.42f);
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(hx,hy);
                for(int i=0;i<=24;i++) glVertex2f(hx+hr*cos(i*twicePi/24),hy+hr*sin(i*twicePi/24));
                glEnd();
            }
            // Cap
            glColor3f(0.30f, 0.10f, 0.30f);
            glBegin(GL_QUADS);
            glVertex2f(pgx-0.022f, -0.160f); glVertex2f(pgx+0.022f, -0.160f);
            glVertex2f(pgx+0.022f, -0.170f); glVertex2f(pgx-0.022f, -0.170f);
            // Cap visor
            glVertex2f(pgx+0.016f, -0.170f); glVertex2f(pgx+0.034f, -0.170f);
            glVertex2f(pgx+0.034f, -0.174f); glVertex2f(pgx+0.016f, -0.174f);
            glEnd();
        }

        // ---- FISHERMAN WITH NET (animated swing) ----
        {
            float fx = 0.82f;
            float fy = -0.200f;
            // Body (yellow shirt)
            glColor3f(0.95f, 0.85f, 0.10f);
            glBegin(GL_QUADS);
            glVertex2f(fx-0.016f, fy);
            glVertex2f(fx+0.016f, fy);
            glVertex2f(fx+0.016f, fy-0.045f);
            glVertex2f(fx-0.016f, fy-0.045f);
            // dark trousers rolled up
            glColor3f(0.18f, 0.10f, 0.05f);
            glVertex2f(fx-0.015f, fy-0.045f); glVertex2f(fx-0.001f, fy-0.045f);
            glVertex2f(fx-0.001f, fy-0.072f); glVertex2f(fx-0.015f, fy-0.072f);
            glVertex2f(fx+0.001f, fy-0.045f); glVertex2f(fx+0.015f, fy-0.045f);
            glVertex2f(fx+0.015f, fy-0.072f); glVertex2f(fx+0.001f, fy-0.072f);
            glEnd();
            // Head
            {
                GLfloat hx=fx, hy=fy+0.020f, hr=0.020f;
                glColor3f(0.78f,0.55f,0.32f);
                glBegin(GL_TRIANGLE_FAN);
                glVertex2f(hx,hy);
                for(int i=0;i<=24;i++) glVertex2f(hx+hr*cos(i*twicePi/24),hy+hr*sin(i*twicePi/24));
                glEnd();
            }
            // Conical hat
            glColor3f(0.78f, 0.62f, 0.30f);
            glBegin(GL_TRIANGLES);
            glVertex2f(fx-0.026f, -0.165f);
            glVertex2f(fx+0.026f, -0.165f);
            glVertex2f(fx,         -0.135f);
            glEnd();
            // Brim line
            glColor3f(0.55f, 0.40f, 0.18f);
            glLineWidth(1.5f);
            glBegin(GL_LINES);
            glVertex2f(fx-0.026f, -0.165f);
            glVertex2f(fx+0.026f, -0.165f);
            glEnd();
            glLineWidth(1.0f);

            // Net + arm: rotate from shoulder by _netAngle
            glPushMatrix();
            glTranslatef(fx-0.014f, fy-0.005f, 0.0f);
            glRotatef(_netAngle, 0.0f, 0.0f, 1.0f);
            // Arm (yellow shirt sleeve continues then skin)
            glColor3f(0.95f, 0.85f, 0.10f);
            glBegin(GL_QUADS);
            glVertex2f(0.000f, -0.000f);
            glVertex2f(-0.005f, -0.010f);
            glVertex2f(-0.045f, -0.005f);
            glVertex2f(-0.045f,  0.005f);
            glEnd();
            // Net pole (stick)
            glColor3f(0.40f, 0.22f, 0.06f);
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glVertex2f(-0.045f, 0.000f);
            glVertex2f(-0.110f, 0.000f);
            glEnd();
            glLineWidth(1.0f);
            // Net bag (V shape)
            glColor3f(0.85f, 0.85f, 0.90f);
            glLineWidth(1.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(-0.110f,  0.020f);
            glVertex2f(-0.110f, -0.020f);
            glVertex2f(-0.150f, -0.012f);
            glVertex2f(-0.150f,  0.012f);
            glEnd();
            // Net mesh lines
            glBegin(GL_LINES);
            for(int k=0;k<5;k++){
                float t = k/4.0f;
                glVertex2f(-0.110f, -0.020f + t*0.040f);
                glVertex2f(-0.150f, -0.012f + t*0.024f);
            }
            for(int k=0;k<4;k++){
                float t = k/3.0f;
                glVertex2f(-0.110f + t*-0.040f,  0.020f - t*0.008f);
                glVertex2f(-0.110f + t*-0.040f, -0.020f + t*0.008f);
            }
            glEnd();
            // A small fish caught in net (drawn if net is down-ish)
            glColor3f(0.85f, 0.55f, 0.10f);
            glBegin(GL_TRIANGLES);
            glVertex2f(-0.135f, 0.000f);
            glVertex2f(-0.125f, 0.006f);
            glVertex2f(-0.125f,-0.006f);
            glVertex2f(-0.125f, 0.005f);
            glVertex2f(-0.115f, 0.000f);
            glVertex2f(-0.125f,-0.005f);
            glEnd();
            glPopMatrix();
        }

    }
    // END BEACH PEOPLE SCENE


    //Beach bench <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


    ///bench 1

    glPushMatrix();
    glTranslated(0.0f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();


    //..
    glPushMatrix();
    glTranslated(-0.03f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslated(0.03f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..
    glPushMatrix();
    glTranslated(0.06f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

//..
    glPushMatrix();
    glTranslated(0.09f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslated(0.12f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();


    ////.////

    glPushMatrix();
    glTranslated(0.3f,0.0f,0.0f);

    glPushMatrix();
    glTranslated(0.0f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();


    //..
    glPushMatrix();
    glTranslated(-0.03f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslated(0.03f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..
    glPushMatrix();
    glTranslated(0.06f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

//..
    glPushMatrix();
    glTranslated(0.09f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslated(0.12f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();
    glPopMatrix();


    ////.////

    glPushMatrix();
    glTranslated(-0.3f,0.0f,0.0f);

    glPushMatrix();
    glTranslated(0.0f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();


    //..
    glPushMatrix();
    glTranslated(-0.03f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslated(0.03f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..
    glPushMatrix();
    glTranslated(0.06f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

//..
    glPushMatrix();
    glTranslated(0.09f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslated(0.12f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();
    glPopMatrix();


    ////.////

    glPushMatrix();
    glTranslated(-0.45f,0.0f,0.0f);

    glPushMatrix();
    glTranslated(0.0f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();


    //..
    glPushMatrix();
    glTranslated(-0.03f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslated(0.03f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..
    glPushMatrix();
    glTranslated(0.06f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

//..
    glPushMatrix();
    glTranslated(0.09f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslated(0.12f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();
    glPopMatrix();


    ////.////

    glPushMatrix();
    glTranslated(0.45f,0.0f,0.0f);

    glPushMatrix();
    glTranslated(0.0f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();


    //..
    glPushMatrix();
    glTranslated(-0.03f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslated(0.03f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..
    glPushMatrix();
    glTranslated(0.06f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

//..
    glPushMatrix();
    glTranslated(0.09f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslated(0.12f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();
    glPopMatrix();


////.////

    glPushMatrix();
    glTranslated(-0.85f,0.0f,0.0f);

    glPushMatrix();
    glTranslated(0.0f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();


    //..
    glPushMatrix();
    glTranslated(-0.03f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslated(0.03f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..
    glPushMatrix();
    glTranslated(0.06f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

//..
    glPushMatrix();
    glTranslated(0.09f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();

    //..

    glPushMatrix();
    glTranslated(0.12f,0.01f,0.0f);
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(0.33, 0.19, 0.19);

    glVertex2f( 0.112,- 0.238);
    glVertex2f( 0.105,- 0.245);

    glVertex2f( 0.112,- 0.28);
    glVertex2f( 0.105,- 0.29);

    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(0.80, 0.35, 0.01);
    glVertex2f( 0.13,- 0.23);
    glVertex2f( 0.13,- 0.3);
    glVertex2f( 0.11,- 0.3);
    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.115,- 0.22);
    glVertex2f( 0.135,- 0.22);
    glEnd();


    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0.85, 0.83, 0.80);
    glVertex2f( 0.11,- 0.29);
    glVertex2f( 0.131,- 0.29);

    glVertex2f( 0.11,- 0.28);
    glVertex2f( 0.131,- 0.28);

    glVertex2f( 0.11,- 0.27);
    glVertex2f( 0.131,- 0.27);

    glVertex2f( 0.11,- 0.26);
    glVertex2f( 0.131,- 0.26);

    glVertex2f( 0.11,- 0.25);
    glVertex2f( 0.131,- 0.25);

    glVertex2f( 0.11,- 0.24);
    glVertex2f( 0.131,- 0.24);

    glVertex2f( 0.11,- 0.23);
    glVertex2f( 0.131,- 0.23);

    glEnd();
    glPopMatrix();
    glPopMatrix();


    // bus 02<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>
//
//    glPushMatrix();
////    if(crash){
////        glTranslatef(run[6],_crashB2,0);
////    }
////    else{
////       glTranslatef(run[6],0,0);
////    }
//
//    glTranslatef(run[6],_crashB2,0);
//


}

// =====================================================================
// BBQ STORE WITH BEACH CROWD
// Store on right side of scene, open space in front (road-side),
// beach-costume people eating BBQ: one group of 3 + one couple
// =====================================================================
void drawBBQStore()
{
    // ---- Store Building ----
    float sx1 = 0.50f, sx2 = 0.96f;
    float sy1 = 0.26f, sy2 = 0.46f;

    // Building wall (warm brick orange)
    glColor3f(0.82f, 0.38f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(sx1, sy1); glVertex2f(sx2, sy1);
    glVertex2f(sx2, sy2); glVertex2f(sx1, sy2);
    glEnd();

    // Roof (dark red triangle)
    glColor3f(0.50f, 0.12f, 0.04f);
    glBegin(GL_TRIANGLES);
    glVertex2f(sx1 - 0.02f, sy2);
    glVertex2f(sx2 + 0.02f, sy2);
    glVertex2f((sx1+sx2)*0.5f, sy2 + 0.09f);
    glEnd();

    // Sign board (dark background)
    glColor3f(0.12f, 0.08f, 0.04f);
    glBegin(GL_QUADS);
    glVertex2f(sx1+0.05f, sy2-0.03f); glVertex2f(sx2-0.05f, sy2-0.03f);
    glVertex2f(sx2-0.05f, sy2-0.11f); glVertex2f(sx1+0.05f, sy2-0.11f);
    glEnd();
    // Sign text
    glColor3f(1.0f, 0.85f, 0.0f);
    char bbqLabel[] = "BBQ CORNER";
    renderBitmapString(sx1+0.09f, sy2-0.090f, 0.0f, GLUT_BITMAP_HELVETICA_12, bbqLabel);

    // Store windows
    glColor3f(0.95f, 0.88f, 0.65f);
    glBegin(GL_QUADS);
    glVertex2f(sx1+0.03f, sy1+0.03f); glVertex2f(sx1+0.18f, sy1+0.03f);
    glVertex2f(sx1+0.18f, sy1+0.14f); glVertex2f(sx1+0.03f, sy1+0.14f);
    glVertex2f(sx1+0.22f, sy1+0.03f); glVertex2f(sx1+0.37f, sy1+0.03f);
    glVertex2f(sx1+0.37f, sy1+0.14f); glVertex2f(sx1+0.22f, sy1+0.14f);
    glEnd();

    // Store door
    glColor3f(0.35f, 0.20f, 0.08f);
    glBegin(GL_QUADS);
    glVertex2f(sx2-0.16f, sy1); glVertex2f(sx2-0.06f, sy1);
    glVertex2f(sx2-0.06f, sy1+0.18f); glVertex2f(sx2-0.16f, sy1+0.18f);
    glEnd();

    // BBQ grill/counter at store front
    glColor3f(0.28f, 0.18f, 0.08f);
    glBegin(GL_QUADS);
    glVertex2f(sx1,        sy1);       glVertex2f(sx1+0.22f, sy1);
    glVertex2f(sx1+0.22f, sy1-0.05f); glVertex2f(sx1,       sy1-0.05f);
    glEnd();
    // Grill top (dark iron)
    glColor3f(0.18f, 0.18f, 0.18f);
    glBegin(GL_QUADS);
    glVertex2f(sx1+0.01f, sy1-0.008f); glVertex2f(sx1+0.21f, sy1-0.008f);
    glVertex2f(sx1+0.21f, sy1-0.030f); glVertex2f(sx1+0.01f, sy1-0.030f);
    glEnd();
    // Grill smoke lines
    glColor3f(0.75f, 0.75f, 0.75f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(sx1+0.06f, sy1-0.008f);
    glVertex2f(sx1+0.05f, sy1+0.018f);
    glVertex2f(sx1+0.07f, sy1+0.038f);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex2f(sx1+0.13f, sy1-0.008f);
    glVertex2f(sx1+0.14f, sy1+0.018f);
    glVertex2f(sx1+0.12f, sy1+0.038f);
    glEnd();
    glLineWidth(1.0f);

    // Sidewalk / open space between store and road (light concrete)
    glColor3f(0.80f, 0.78f, 0.74f);
    glBegin(GL_QUADS);
    glVertex2f(sx1, 0.10f); glVertex2f(sx2, 0.10f);
    glVertex2f(sx2, sy1);   glVertex2f(sx1, sy1);
    glEnd();

    // ===== PEOPLE IN BEACH COSTUMES =====
    // py = body-top Y; head at py+0.018, feet at ~py-0.068
    float py = 0.205f;

    // ---- Group of 3 ----

    // Person 1: red tank top, blue shorts
    {
        float px = 0.53f;
        // Legs (skin)
        glColor3f(0.85f, 0.65f, 0.45f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.010f, py-0.042f); glVertex2f(px-0.001f, py-0.042f);
        glVertex2f(px-0.001f, py-0.068f); glVertex2f(px-0.010f, py-0.068f);
        glVertex2f(px+0.001f, py-0.042f); glVertex2f(px+0.010f, py-0.042f);
        glVertex2f(px+0.010f, py-0.068f); glVertex2f(px+0.001f, py-0.068f);
        glEnd();
        // Blue shorts
        glColor3f(0.15f, 0.35f, 0.85f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.013f, py-0.012f); glVertex2f(px+0.013f, py-0.012f);
        glVertex2f(px+0.013f, py-0.046f); glVertex2f(px-0.013f, py-0.046f);
        glEnd();
        // Red tank top
        glColor3f(0.90f, 0.12f, 0.08f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.012f, py+0.000f); glVertex2f(px+0.012f, py+0.000f);
        glVertex2f(px+0.012f, py-0.015f); glVertex2f(px-0.012f, py-0.015f);
        glEnd();
        // Head
        glColor3f(0.85f, 0.65f, 0.45f);
        drawCircle(px, py+0.018f, 0.018f);
        // Hair
        glColor3f(0.10f, 0.05f, 0.01f);
        drawCircle(px, py+0.028f, 0.012f);
        // Right arm
        glColor3f(0.85f, 0.65f, 0.45f);
        glBegin(GL_QUADS);
        glVertex2f(px+0.012f, py-0.002f); glVertex2f(px+0.026f, py-0.002f);
        glVertex2f(px+0.026f, py-0.014f); glVertex2f(px+0.012f, py-0.014f);
        glEnd();
        // Skewer stick
        glColor3f(0.60f, 0.38f, 0.08f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(px+0.026f, py-0.008f);
        glVertex2f(px+0.055f, py+0.018f);
        glEnd();
        // Meat on skewer
        glColor3f(0.68f, 0.22f, 0.05f);
        drawCircle(px+0.038f, py+0.004f, 0.006f);
        drawCircle(px+0.047f, py+0.012f, 0.005f);
        glLineWidth(1.0f);
    }

    // Person 2: green tank top, yellow shorts
    {
        float px = 0.62f;
        glColor3f(0.82f, 0.62f, 0.42f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.010f, py-0.042f); glVertex2f(px-0.001f, py-0.042f);
        glVertex2f(px-0.001f, py-0.068f); glVertex2f(px-0.010f, py-0.068f);
        glVertex2f(px+0.001f, py-0.042f); glVertex2f(px+0.010f, py-0.042f);
        glVertex2f(px+0.010f, py-0.068f); glVertex2f(px+0.001f, py-0.068f);
        glEnd();
        // Yellow shorts
        glColor3f(0.95f, 0.82f, 0.05f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.013f, py-0.012f); glVertex2f(px+0.013f, py-0.012f);
        glVertex2f(px+0.013f, py-0.046f); glVertex2f(px-0.013f, py-0.046f);
        glEnd();
        // Green tank top
        glColor3f(0.10f, 0.72f, 0.22f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.012f, py+0.000f); glVertex2f(px+0.012f, py+0.000f);
        glVertex2f(px+0.012f, py-0.015f); glVertex2f(px-0.012f, py-0.015f);
        glEnd();
        glColor3f(0.82f, 0.62f, 0.42f);
        drawCircle(px, py+0.018f, 0.018f);
        glColor3f(0.10f, 0.05f, 0.01f);
        drawCircle(px, py+0.028f, 0.012f);
        // Left arm
        glColor3f(0.82f, 0.62f, 0.42f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.026f, py-0.002f); glVertex2f(px-0.012f, py-0.002f);
        glVertex2f(px-0.012f, py-0.014f); glVertex2f(px-0.026f, py-0.014f);
        glEnd();
        glColor3f(0.60f, 0.38f, 0.08f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(px-0.026f, py-0.008f);
        glVertex2f(px-0.055f, py+0.018f);
        glEnd();
        glColor3f(0.68f, 0.22f, 0.05f);
        drawCircle(px-0.038f, py+0.004f, 0.006f);
        drawCircle(px-0.047f, py+0.012f, 0.005f);
        glLineWidth(1.0f);
    }

    // Person 3 (female): orange sundress
    {
        float px = 0.71f;
        glColor3f(0.88f, 0.68f, 0.50f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.010f, py-0.040f); glVertex2f(px-0.001f, py-0.040f);
        glVertex2f(px-0.001f, py-0.068f); glVertex2f(px-0.010f, py-0.068f);
        glVertex2f(px+0.001f, py-0.040f); glVertex2f(px+0.010f, py-0.040f);
        glVertex2f(px+0.010f, py-0.068f); glVertex2f(px+0.001f, py-0.068f);
        glEnd();
        // Orange sundress (flared)
        glColor3f(0.95f, 0.48f, 0.05f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.014f, py+0.000f); glVertex2f(px+0.014f, py+0.000f);
        glVertex2f(px+0.022f, py-0.044f); glVertex2f(px-0.022f, py-0.044f);
        glEnd();
        glColor3f(0.88f, 0.68f, 0.50f);
        drawCircle(px, py+0.020f, 0.019f);
        // Long hair
        glColor3f(0.18f, 0.09f, 0.02f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.022f, py+0.034f); glVertex2f(px+0.022f, py+0.034f);
        glVertex2f(px+0.018f, py-0.004f); glVertex2f(px-0.018f, py-0.004f);
        glEnd();
        // Right arm
        glColor3f(0.88f, 0.68f, 0.50f);
        glBegin(GL_QUADS);
        glVertex2f(px+0.014f, py-0.001f); glVertex2f(px+0.028f, py-0.001f);
        glVertex2f(px+0.028f, py-0.012f); glVertex2f(px+0.014f, py-0.012f);
        glEnd();
        glColor3f(0.60f, 0.38f, 0.08f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(px+0.028f, py-0.006f);
        glVertex2f(px+0.055f, py+0.018f);
        glEnd();
        glColor3f(0.68f, 0.22f, 0.05f);
        drawCircle(px+0.042f, py+0.008f, 0.006f);
        glLineWidth(1.0f);
    }

    // ---- Couple ----

    // Couple Woman: purple beach dress
    {
        float px = 0.79f;
        glColor3f(0.85f, 0.66f, 0.46f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.010f, py-0.040f); glVertex2f(px-0.001f, py-0.040f);
        glVertex2f(px-0.001f, py-0.068f); glVertex2f(px-0.010f, py-0.068f);
        glVertex2f(px+0.001f, py-0.040f); glVertex2f(px+0.010f, py-0.040f);
        glVertex2f(px+0.010f, py-0.068f); glVertex2f(px+0.001f, py-0.068f);
        glEnd();
        // Purple beach dress
        glColor3f(0.62f, 0.15f, 0.78f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.015f, py+0.000f); glVertex2f(px+0.015f, py+0.000f);
        glVertex2f(px+0.022f, py-0.044f); glVertex2f(px-0.022f, py-0.044f);
        glEnd();
        glColor3f(0.85f, 0.66f, 0.46f);
        drawCircle(px, py+0.020f, 0.019f);
        // Long dark hair
        glColor3f(0.15f, 0.08f, 0.02f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.022f, py+0.034f); glVertex2f(px+0.022f, py+0.034f);
        glVertex2f(px+0.018f, py-0.004f); glVertex2f(px-0.018f, py-0.004f);
        glEnd();
        // Right arm reaching toward partner
        glColor3f(0.85f, 0.66f, 0.46f);
        glBegin(GL_QUADS);
        glVertex2f(px+0.015f, py-0.004f); glVertex2f(px+0.032f, py-0.004f);
        glVertex2f(px+0.032f, py-0.016f); glVertex2f(px+0.015f, py-0.016f);
        glEnd();
        // Left arm + skewer
        glBegin(GL_QUADS);
        glVertex2f(px-0.028f, py-0.002f); glVertex2f(px-0.015f, py-0.002f);
        glVertex2f(px-0.015f, py-0.013f); glVertex2f(px-0.028f, py-0.013f);
        glEnd();
        glColor3f(0.60f, 0.38f, 0.08f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(px-0.028f, py-0.007f);
        glVertex2f(px-0.055f, py+0.020f);
        glEnd();
        glColor3f(0.68f, 0.22f, 0.05f);
        drawCircle(px-0.042f, py+0.007f, 0.006f);
        glLineWidth(1.0f);
    }

    // Couple Man: teal swim shorts, white tank top
    {
        float px = 0.88f;
        glColor3f(0.80f, 0.62f, 0.42f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.010f, py-0.038f); glVertex2f(px-0.001f, py-0.038f);
        glVertex2f(px-0.001f, py-0.068f); glVertex2f(px-0.010f, py-0.068f);
        glVertex2f(px+0.001f, py-0.038f); glVertex2f(px+0.010f, py-0.038f);
        glVertex2f(px+0.010f, py-0.068f); glVertex2f(px+0.001f, py-0.068f);
        glEnd();
        // Teal swim shorts
        glColor3f(0.05f, 0.58f, 0.62f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.014f, py-0.010f); glVertex2f(px+0.014f, py-0.010f);
        glVertex2f(px+0.014f, py-0.042f); glVertex2f(px-0.014f, py-0.042f);
        glEnd();
        // White tank top
        glColor3f(0.95f, 0.95f, 0.95f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.013f, py+0.000f); glVertex2f(px+0.013f, py+0.000f);
        glVertex2f(px+0.013f, py-0.013f); glVertex2f(px-0.013f, py-0.013f);
        glEnd();
        glColor3f(0.80f, 0.62f, 0.42f);
        drawCircle(px, py+0.018f, 0.019f);
        // Short hair
        glColor3f(0.10f, 0.06f, 0.02f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.020f, py+0.032f); glVertex2f(px+0.020f, py+0.032f);
        glVertex2f(px+0.020f, py+0.024f); glVertex2f(px-0.020f, py+0.024f);
        glEnd();
        // Left arm reaching toward woman (holding hands)
        glColor3f(0.80f, 0.62f, 0.42f);
        glBegin(GL_QUADS);
        glVertex2f(px-0.032f, py-0.004f); glVertex2f(px-0.013f, py-0.004f);
        glVertex2f(px-0.013f, py-0.016f); glVertex2f(px-0.032f, py-0.016f);
        glEnd();
        // Right arm + skewer
        glBegin(GL_QUADS);
        glVertex2f(px+0.013f, py-0.002f); glVertex2f(px+0.026f, py-0.002f);
        glVertex2f(px+0.026f, py-0.013f); glVertex2f(px+0.013f, py-0.013f);
        glEnd();
        glColor3f(0.60f, 0.38f, 0.08f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(px+0.026f, py-0.007f);
        glVertex2f(px+0.052f, py+0.018f);
        glEnd();
        glColor3f(0.68f, 0.22f, 0.05f);
        drawCircle(px+0.040f, py+0.007f, 0.006f);
        glLineWidth(1.0f);
    }

    // Small heart above couple
    {
        float hxx = 0.835f, hyy = py + 0.065f;
        glColor3f(0.95f, 0.10f, 0.30f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(hxx-0.004f, hyy+0.002f);
        for(int i=0;i<=12;i++)
            glVertex2f((hxx-0.004f)+0.005f*cosf(i*twicePi/12),
                       (hyy+0.002f)+0.004f*sinf(i*twicePi/12));
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(hxx+0.004f, hyy+0.002f);
        for(int i=0;i<=12;i++)
            glVertex2f((hxx+0.004f)+0.005f*cosf(i*twicePi/12),
                       (hyy+0.002f)+0.004f*sinf(i*twicePi/12));
        glEnd();
        glBegin(GL_TRIANGLES);
        glVertex2f(hxx-0.008f, hyy);
        glVertex2f(hxx+0.008f, hyy);
        glVertex2f(hxx,        hyy-0.009f);
        glEnd();
    }
}

void drawSea()
{
    //sea............................................................................................

    glPushMatrix();
    glTranslatef(0,- 0.1,0);
    glBegin(GL_QUADS);
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }

    glVertex2f(-1.0f, -0.35f);
    glVertex2f(1.0f, -0.35f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(-1.0f, -1.0f);
    glEnd();

    //wave[0]..............................
    glPushMatrix();
    glTranslatef(0,wave[0],0);
    GLfloat xw1=-0.9f;
    GLfloat yw1=-0.61f;
    GLfloat radiusw1 =0.3f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw1, yw1);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw1 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw1 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    //wave[1]..............................
    glPushMatrix();
    glTranslatef(0,wave[1],0);
    GLfloat xw2=0.3f;
    GLfloat yw2=-0.61f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw2, yw2);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw2 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw2 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();


    //wave[2]..............................
    glPushMatrix();
    glTranslatef(0,wave[2],0);
    GLfloat xw3=0.2f;
    GLfloat yw3=-0.62f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw3, yw3);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw3 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw3 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    //wave[3]..............................

    glPushMatrix();
    glTranslatef(0,wave[3],0);
    GLfloat xw4=0.1f;
    GLfloat yw4=-0.63f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw4, yw4);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw4 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw4 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();


    //wave[4]..............................
    glPushMatrix();
    glTranslatef(0,wave[4],0);
    GLfloat xw5=-0.2f;
    GLfloat yw5=-0.62f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw5, yw5);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw5 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw5 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    //wave[5]..............................
    glPushMatrix();
    glTranslatef(0,wave[5],0);
    GLfloat xw6=-0.8f;
    GLfloat yw6=-0.62f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw6, yw6);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw6 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw6 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    //wave[6]..............................
    glPushMatrix();
    glTranslatef(0,wave[6],0);
    GLfloat xw7=-0.7f;
    GLfloat yw7=-0.63f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw7, yw7);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw7 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw7 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    //wave[7]..............................
    glPushMatrix();
    glTranslatef(0,wave[7],0);
    GLfloat xw8=-0.6f;
    GLfloat yw8=-0.62f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw8, yw8);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw8 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw8 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    //wave[8]..............................
    glPushMatrix();
    glTranslatef(0,wave[8],0);
    GLfloat xw9=-0.5f;
    GLfloat yw9=-0.61f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw9, yw9);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw9 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw9 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    //wave[9]..............................
    glPushMatrix();
    glTranslatef(0,wave[9],0);
    GLfloat xw10=-0.4f;
    GLfloat yw10=-0.62f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw10, yw10);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw10 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw10 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    //wave[10]..............................
    glPushMatrix();
    glTranslatef(0,wave[10],0);
    GLfloat xw11=-0.05f;
    GLfloat yw11=-0.63f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw11, yw11);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw11 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw11 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    //wave[11]..............................
    glPushMatrix();
    glTranslatef(0,wave[11],0);
    GLfloat xw12=0.4f;
    GLfloat yw12=-0.61f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw12, yw12);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw12 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw12 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    //wave[12]..............................
    glPushMatrix();
    glTranslatef(0,wave[12],0);
    GLfloat xw13=0.5f;
    GLfloat yw13=-0.62f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw13, yw13);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw13 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw13 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    //wave[13]..............................
    glPushMatrix();
    glTranslatef(0,wave[13],0);
    GLfloat xw14=0.6f;
    GLfloat yw14=-0.63f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw14, yw14);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw14 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw14 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    //wave[14]..............................
    glPushMatrix();
    glTranslatef(0,wave[14],0);
    GLfloat xw15=0.7f;
    GLfloat yw15=-0.61f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw15, yw15);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw15 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw15 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();


    //wave[15]..............................
    glPushMatrix();
    glTranslatef(0,wave[15],0);
    GLfloat xw16=0.8f;
    GLfloat yw16=-0.62f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw16, yw16);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw16 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw16 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    //wave[16]..............................
    glPushMatrix();
    glTranslatef(0,wave[16],0);
    GLfloat xw17=0.9f;
    GLfloat yw17=-0.63f;
    glColor3f(0.0f,0.29f,0.6f);
    if(night)
    {
        glColor3f(0.0f, 0.09f, 0.4f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xw17, yw17);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xw17 + (radiusw1 * cos(i *  twicePi / triangleAmount)),
                    yw17 + (radiusw1 * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();
    glPopMatrix();

    glPopMatrix();


    // small.boat...................................................................................
    glPushMatrix();
    glTranslatef(0,- 0.1,0);
    if(!night && ! rain)
    {
        glPushMatrix();
        glTranslatef(run[7],0,0);

        glPushMatrix();
        glTranslatef(0.01f, -0.2f, 0.0f);
        glScalef( 0.5, 0.5,0);


        glBegin(GL_POLYGON);

        glColor3f(0.4f,0.0f,0.0f);

        glVertex2f(-0.75f, -0.47f);
        glVertex2f(-0.65f, -0.47f);

        glVertex2f(-0.6f, -0.4f);
        glVertex2f(-0.65f, -0.47f);

        glVertex2f(-0.8f, -0.4f);
        glVertex2f(-0.75f, -0.47f);

        glVertex2f(-0.6f, -0.4f);
        glVertex2f(-0.65f, -0.44f);

        glVertex2f(-0.75f, -0.44f);
        glVertex2f(-0.8f, -0.4f);

        glVertex2f(-0.65f, -0.44f);
        glVertex2f(-0.75f, -0.44f);

        glEnd();

        glBegin(GL_QUADS);

        glColor3f(0.0f,0.6f,0.0f);

        glVertex2f(-0.72f, -0.39f);
        glVertex2f(-0.67f, -0.39f);
        glVertex2f(-0.65f, -0.435f);
        glVertex2f(-0.7f, -0.435f);


        glEnd();

        glBegin(GL_TRIANGLES);
        glColor3f(0.0f,0.0f,0.0f);

        glVertex2f(-0.72f, -0.39f);
        glVertex2f(-0.7f, -0.43f);
        glVertex2f(-0.75f, -0.418f);


        glEnd();
        glLoadIdentity();
        glPopMatrix();

        glPopMatrix();
    }
    glPopMatrix();

    //smallboat night
    if(night || rain)
    {
        glPushMatrix();
        glTranslatef(1.1,- 0.05,0);

        glPushMatrix();
        glTranslatef(0.01f, -0.2f, 0.0f);
        glScalef( 0.5, 0.5,0);


        glBegin(GL_POLYGON);

        glColor3f(0.4f,0.0f,0.0f);

        glVertex2f(-0.75f, -0.47f);
        glVertex2f(-0.65f, -0.47f);

        glVertex2f(-0.6f, -0.4f);
        glVertex2f(-0.65f, -0.47f);

        glVertex2f(-0.8f, -0.4f);
        glVertex2f(-0.75f, -0.47f);

        glVertex2f(-0.6f, -0.4f);
        glVertex2f(-0.65f, -0.44f);

        glVertex2f(-0.75f, -0.44f);
        glVertex2f(-0.8f, -0.4f);

        glVertex2f(-0.65f, -0.44f);
        glVertex2f(-0.75f, -0.44f);

        glEnd();

        glBegin(GL_QUADS);

        glColor3f(0.0f,0.6f,0.0f);

        glVertex2f(-0.72f, -0.39f);
        glVertex2f(-0.67f, -0.39f);
        glVertex2f(-0.65f, -0.435f);
        glVertex2f(-0.7f, -0.435f);


        glEnd();

        glBegin(GL_TRIANGLES);
        glColor3f(0.0f,0.0f,0.0f);

        glVertex2f(-0.72f, -0.39f);
        glVertex2f(-0.7f, -0.43f);
        glVertex2f(-0.75f, -0.418f);


        glEnd();
        glLoadIdentity();
        glPopMatrix();

        glPopMatrix();
    }

    //small boat copy...........................................................
    glPushMatrix();
    glTranslatef(0,- 0.1,0);
    if(!night && !rain)
    {
        glPushMatrix();
        glTranslatef(run[8],0,0);

        glPushMatrix();
        glTranslatef(0.09f,-0.3f, 0.0f);
        glRotatef(180,0,1,0);

        glScalef( 0.5, 0.5,0);


        glBegin(GL_POLYGON);

        glColor3f(0.2f,0.1f,0.0f);

        glVertex2f(-0.75f, -0.47f);
        glVertex2f(-0.65f, -0.47f);

        glVertex2f(-0.6f, -0.4f);
        glVertex2f(-0.65f, -0.47f);

        glVertex2f(-0.8f, -0.4f);
        glVertex2f(-0.75f, -0.47f);

        glVertex2f(-0.6f, -0.4f);
        glVertex2f(-0.65f, -0.44f);

        glVertex2f(-0.75f, -0.44f);
        glVertex2f(-0.8f, -0.4f);

        glVertex2f(-0.65f, -0.44f);
        glVertex2f(-0.75f, -0.44f);

        glEnd();

        glBegin(GL_QUADS);

        glColor3f(0.9f,0.5f,0.0f);

        glVertex2f(-0.72f, -0.39f);
        glVertex2f(-0.67f, -0.39f);
        glVertex2f(-0.65f, -0.435f);
        glVertex2f(-0.7f, -0.435f);


        glEnd();

        glBegin(GL_TRIANGLES);
        glColor3f(0.0f,0.0f,0.0f);

        glVertex2f(-0.72f, -0.39f);
        glVertex2f(-0.7f, -0.43f);
        glVertex2f(-0.75f, -0.418f);

        glEnd();
        glLoadIdentity();
        glPopMatrix();

        glPopMatrix();

    }
    glPopMatrix();
}


void drawNightOverlay()
{
///night
    if(night || rain)
    {
        glPushMatrix();
        glTranslatef( 0.17, 0.05,0);

        glPushMatrix();
        glTranslatef(0.09f,-0.3f, 0.0f);
        glRotatef(180,0,1,0);

        glScalef( 0.5, 0.5,0);


        glBegin(GL_POLYGON);

        glColor3f(0.2f,0.1f,0.0f);

        glVertex2f(-0.75f, -0.47f);
        glVertex2f(-0.65f, -0.47f);

        glVertex2f(-0.6f, -0.4f);
        glVertex2f(-0.65f, -0.47f);

        glVertex2f(-0.8f, -0.4f);
        glVertex2f(-0.75f, -0.47f);

        glVertex2f(-0.6f, -0.4f);
        glVertex2f(-0.65f, -0.44f);

        glVertex2f(-0.75f, -0.44f);
        glVertex2f(-0.8f, -0.4f);

        glVertex2f(-0.65f, -0.44f);
        glVertex2f(-0.75f, -0.44f);

        glEnd();

        glBegin(GL_QUADS);

        glColor3f(0.9f,0.5f,0.0f);

        glVertex2f(-0.72f, -0.39f);
        glVertex2f(-0.67f, -0.39f);
        glVertex2f(-0.65f, -0.435f);
        glVertex2f(-0.7f, -0.435f);


        glEnd();

        glBegin(GL_TRIANGLES);
        glColor3f(0.0f,0.0f,0.0f);

        glVertex2f(-0.72f, -0.39f);
        glVertex2f(-0.7f, -0.43f);
        glVertex2f(-0.75f, -0.418f);

        glEnd();
        glLoadIdentity();
        glPopMatrix();

        glPopMatrix();

    }
    //big_boat...............................................................................................

    glPushMatrix();
    glTranslatef(run[9],0,0);

    glPushMatrix();
    glTranslatef(0.01f,-0.3f, 0.0f);
    glScalef( 0.7f,0.6f,0.0f);

    glBegin(GL_QUADS);
    glColor3f(0.6f,0.29f,0.0f);

    glVertex2f(0.1f, -0.8f);
    glVertex2f(0.5f, -0.8f);
    glVertex2f(0.4f, -0.9f);
    glVertex2f(0.2f, -0.9f);

    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(0.6f, -0.8f);
    glVertex2f(0.5f, -0.8f);
    glVertex2f(0.4f, -0.9f);
    glVertex2f(0.5f, -0.9f);

    glColor3f(1.0f,1.0f,1.0f);
    glVertex2f(0.18f, -0.88f);
    glVertex2f(0.2f, -0.9f);
    glColor3f(0.0f,0.0f,0.0f);
    glVertex2f(0.4f, -0.9f);
    glVertex2f(0.42f, -0.88f);

    glColor3f(0.5f,0.5f,0.5f);
    glVertex2f(0.15f, -0.75f);
    glVertex2f(0.45f, -0.75f);
    glVertex2f(0.45f, -0.8f);
    glVertex2f(0.15f, -0.8f);

    glColor3f(1.0f,1.0f,1.0f);
    glVertex2f(0.55f, -0.75f);
    glVertex2f(0.45f, -0.75f);
    glVertex2f(0.45f, -0.8f);
    glVertex2f(0.55f, -0.8f);

    glColor3f(0.8f,0.6f,1.0f);
    glVertex2f(0.15f, -0.75f);
    glVertex2f(0.45f, -0.75f);
    glVertex2f(0.4f, -0.7f);
    glVertex2f(0.15f, -0.7f);

    glColor3f(0.6f,0.6f,1.0f);
    glVertex2f(0.55f, -0.75f);
    glVertex2f(0.45f, -0.75f);
    glVertex2f(0.4f, -0.7f);
    glVertex2f(0.5f, -0.7f);
    glEnd();
    /**.................................... big boat-Pillar .......................................**/
    glColor3f(0.0f,0.0f,0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.18f, -0.65f);
    glVertex2f(0.21f, -0.65f);
    glVertex2f(0.21f, -0.7f);
    glVertex2f(0.18f, -0.7f);
    glEnd();

    glColor3f(0.6f,0.29f,0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.18f, -0.65f);
    glVertex2f(0.21f, -0.65f);
    glVertex2f(0.21f, -0.63f);
    glVertex2f(0.18f, -0.63f);
    glEnd();

    glColor3f(0.0f,0.0f,0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.28f, -0.65f);
    glVertex2f(0.31f, -0.65f);
    glVertex2f(0.31f, -0.7f);
    glVertex2f(0.28f, -0.7f);
    glEnd();

    glColor3f(0.6f,0.29f,0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.28f, -0.65f);
    glVertex2f(0.31f, -0.65f);
    glVertex2f(0.31f, -0.63f);
    glVertex2f(0.28f, -0.63f);
    glEnd();

    glColor3f(0.0f,0.0f,0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.38f, -0.65f);
    glVertex2f(0.41f, -0.65f);
    glVertex2f(0.41f, -0.7f);
    glVertex2f(0.38f, -0.7f);
    glEnd();

    glColor3f(0.6f,0.29f,0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.38f, -0.65f);
    glVertex2f(0.41f, -0.65f);
    glVertex2f(0.41f, -0.63f);
    glVertex2f(0.38f, -0.63f);
    glEnd();


    /**.................................... big boat-design .......................................**/

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glBegin(GL_QUADS);
    glVertex2f(0.16f, -0.79f);
    glVertex2f(0.19f, -0.79f);
    glVertex2f(0.19f, -0.76f);
    glVertex2f(0.16f, -0.76f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.05f, 0.0f, 0.0f);
    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glBegin(GL_QUADS);
    glVertex2f(0.16f, -0.79f);
    glVertex2f(0.19f, -0.79f);
    glVertex2f(0.19f, -0.76f);
    glVertex2f(0.16f, -0.76f);
    glEnd();

    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.1f, 0.0f, 0.0f);
    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glBegin(GL_QUADS);
    glVertex2f(0.16f, -0.79f);
    glVertex2f(0.19f, -0.79f);
    glVertex2f(0.19f, -0.76f);
    glVertex2f(0.16f, -0.76f);
    glEnd();

    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.15f, 0.0f, 0.0f);
    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glBegin(GL_QUADS);
    glVertex2f(0.16f, -0.79f);
    glVertex2f(0.19f, -0.79f);
    glVertex2f(0.19f, -0.76f);
    glVertex2f(0.16f, -0.76f);
    glEnd();

    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.2f, 0.0f, 0.0f);
    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glBegin(GL_QUADS);
    glVertex2f(0.16f, -0.79f);
    glVertex2f(0.19f, -0.79f);
    glVertex2f(0.19f, -0.76f);
    glVertex2f(0.16f, -0.76f);
    glEnd();

    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.25f, 0.0f, 0.0f);
    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glBegin(GL_QUADS);
    glVertex2f(0.16f, -0.79f);
    glVertex2f(0.19f, -0.79f);
    glVertex2f(0.19f, -0.76f);
    glVertex2f(0.16f, -0.76f);
    glEnd();

    glPopMatrix();

    glColor3f(0.6f,0.8f,1.0f);
    if (night)
    {
        glColor3f(0.9f,0.9f,0.0f);
    }
    glBegin(GL_QUADS);
    glVertex2f(0.46f, -0.79f);
    glVertex2f(0.54f, -0.79f);
    glVertex2f(0.54f, -0.76f);
    glVertex2f(0.46f, -0.76f);
    glEnd();


    GLfloat xboat1=0.175f;
    GLfloat yboat1=-0.84f;
    GLfloat radiusb = 0.015f;

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xboat1, yboat1);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xboat1 + (radiusb * cos(i *  twicePi / triangleAmount)),
                    yboat1 + (radiusb * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();


    glPushMatrix();
    glTranslatef(0.045f, 0.0f, 0.0f);
    GLfloat xboat2=0.18f;
    GLfloat yboat2=-0.84f;
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xboat2, yboat2);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xboat2 + (radiusb * cos(i *  twicePi / triangleAmount)),
                    yboat2 + (radiusb * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.095f, 0.0f, 0.0f);
    GLfloat xboat3=0.18f;
    GLfloat yboat3=-0.84f;
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xboat3, yboat3);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xboat3 + (radiusb * cos(i *  twicePi / triangleAmount)),
                    yboat3 + (radiusb * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    glPopMatrix();


    glPushMatrix();
    glTranslatef(0.145f, 0.0f, 0.0f);
    GLfloat xboat4=0.18f;
    GLfloat yboat4=-0.84f;
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xboat4, yboat4);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xboat4 + (radiusb * cos(i *  twicePi / triangleAmount)),
                    yboat4 + (radiusb * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.195f, 0.0f, 0.0f);
    GLfloat xboat5=0.18f;
    GLfloat yboat5=-0.84f;
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xboat5, yboat5);
    for(int i = 0; i <= triangleAmount; i++)
    {
        glVertex2f( xboat5 + (radiusb * cos(i * twicePi / triangleAmount)),
                    yboat5 + (radiusb * sin(i * twicePi / triangleAmount)) );
    }
    glEnd();

    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.245f, 0.0f, 0.0f);
    GLfloat xboat6 = 0.18f;
    GLfloat yboat6 = -0.84f;
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xboat6, yboat6);
    for(int j = 0; j <= triangleAmount; j++)
    {
        glVertex2f( xboat6 + (radiusb * cos(j * twicePi / triangleAmount)),
                    yboat6 + (radiusb * sin(j * twicePi / triangleAmount)) );
    }
    glEnd();

    glPopMatrix();
    glPopMatrix();
    glPopMatrix();
}

void drawRain()
{
    // Rain animation: 10 staggered layers of drops
    if(rain)
    {
        // Inner y-offsets create depth/phase staggering between layers
        static const float innerOffY[10] = { 0.0f, -0.2f, -0.4f, -0.6f, -0.8f,
                                             -1.0f, -1.2f, -1.4f, -1.6f, -1.8f };
        static const float innerOffX[10] = { 0.0f, -0.02f, 0.0f, -0.03f, -0.03f,
                                             -0.03f, -0.03f, 0.0f, -0.03f, -0.03f };
        for(int k = 0; k < 10; k++)
        {
            glPushMatrix();
            glTranslatef(rainX[k], rainY[k], 0);
            glPushMatrix();
            glTranslatef(innerOffX[k], innerOffY[k], 0);

            // Two passes: second pass offset 0.05 units left
            for(int pass = 0; pass < 2; pass++)
            {
                if(pass == 1) { glPushMatrix(); glTranslatef(-0.05f, 0, 0); }
                glBegin(GL_LINES);
                glColor3f(1, 1, 1);
                for(float dx = -0.95f; dx <= 1.36f; dx += 0.1f)
                {
                    glVertex2f(dx,        0.9f);
                    glVertex2f(dx - 0.005f, 0.85f);
                }
                glEnd();
                if(pass == 1) glPopMatrix();
            }

            glPopMatrix();
            glPopMatrix();
        }
    }

    glColor3f(0.0,0.0,0.0);
    // (BAYWATCH label shown on hotel building)

    glColor3f(0.0,0.0,0.0);
    renderBitmapString(0.811f, 0.3f, 0.0f, GLUT_BITMAP_HELVETICA_10, "Inani");


    glColor3f(0.6,0.0,0.0);
    renderBitmapString(-0.85f, 0.4f, 0.0f, GLUT_BITMAP_HELVETICA_10, "La Bella Resort");

    glColor3f(1.0,1.0,1.0);
    renderBitmapString(-0.59f, 0.61f, 0.0f, GLUT_BITMAP_HELVETICA_10, "Neeshorgo Hotel");

    glColor3f(0.0,0.2,0.0);
    renderBitmapString(0.11f, 0.375f, 0.0f, GLUT_BITMAP_HELVETICA_10, "Mermaid Cafe Marine Drive");


}

void drawTitleCard()
{
    ////ENDS Here
  if(!cover){
    glBegin(GL_QUADS);
    glColor3f(0.0, 0.36, 0.0);
    glVertex2f(-1.0f,-1.0f);
    glVertex2f(1.0f, -1.0f);
    glColor3f(0.18, 0.36, 0.56);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();
    glColor3f(1.0,1.0,1.0);

     renderBitmapString(-0.3f, 0.85f, 0.0f, GLUT_BITMAP_HELVETICA_18, " 01054 - COMPUTER GRAPHICS [M]");


    glPushMatrix();
    //glTranslatef(run[10],0,0);
    renderBitmapString(-0.24f, 0.75f, 0.0f, GLUT_BITMAP_HELVETICA_18, "MARINE DRIVE COX BAZAR");
    glPopMatrix();

    renderBitmapString(-0.28f, 0.65f, 0.0f, GLUT_BITMAP_HELVETICA_18, "Submitted by Group[L] Code hunter");
    renderBitmapString(-0.4f, 0.5f, 0.0f, GLUT_BITMAP_HELVETICA_18, "ID                                 Name                                         Contribution ");
    renderBitmapString(-0.4f, 0.4f, 0.0f, GLUT_BITMAP_HELVETICA_18, "C201242              Farida Nusrat                     60% ");
    renderBitmapString(-0.4f, 0.3f, 0.0f, GLUT_BITMAP_HELVETICA_18, "C201224          Sabrina Mostary                            40% ");

    renderBitmapString(-0.11f, -0.5f, 0.0f, GLUT_BITMAP_HELVETICA_18, "Submitted to");
    renderBitmapString(-0.18f, -0.6f, 0.0f, GLUT_BITMAP_HELVETICA_18, "Mrs. Israt Binteh Habib");
    renderBitmapString(-0.22f, -0.65f, 0.0f, GLUT_BITMAP_HELVETICA_12, " Computer Science and Engineering (CSE)");
    renderBitmapString(-0.2f, -0.7f, 0.0f, GLUT_BITMAP_HELVETICA_10, "International Islamic University Chittagong-Bangladesh");
  }
//vertex01
//    glLineWidth(1);
//
//    glBegin(GL_LINES);            // Each set of 4 vertices form a quad
//    glColor3f(0.0f, 1.0f, 0.0f); // green
//
//    glVertex2f(0.0f, 0.0f);    // x, y
//    glVertex2f(1.0f, 0.0f);    // x, y
//
//    glVertex2f(0.0f, 0.0f);    // x, y
//    glVertex2f(0.0f, 1.0f);    // x, y
//
//    glVertex2f(0.0f, 0.0f);    // x, y
//    glVertex2f(-1.0f, 0.0f);    // x, y
//
//    glVertex2f(0.0f, 0.0f);    // x, y
//    glVertex2f(0.0f, -1.0f);
//    glEnd();
//
//    glBegin(GL_LINES);
//    glColor3f(0.0f,0.0f,0.0f);
//
//    glVertex2f(-1.0f, 0.1f);    // x, y
//    glVertex2f(1.0f, 0.1f);
//    glVertex2f(-1.0f, 0.2f);    // x, y
//    glVertex2f(1.0f, 0.2f);
//    glVertex2f(-1.0f, 0.3f);    // x, y
//    glVertex2f(1.0f, 0.3f);
//    glVertex2f(-1.0f, 0.4f);    // x, y
//    glVertex2f(1.0f, 0.4f);
//    glVertex2f(-1.0f, 0.5f);    // x, y
//    glVertex2f(1.0f, 0.5f);
//    glVertex2f(-1.0f, 0.6f);    // x, y
//    glVertex2f(1.0f, 0.6f);
//    glVertex2f(-1.0f, 0.7f);    // x, y
//    glVertex2f(1.0f, 0.7f);
//    glVertex2f(-1.0f, 0.8f);    // x, y
//    glVertex2f(1.0f, 0.8f);
//    glVertex2f(-1.0f, 0.9f);    // x, y
//    glVertex2f(1.0f, 0.9f);
//
//    glVertex2f(-1.0f, -0.1f);    // x, y
//    glVertex2f(1.0f, -0.1f);
//    glVertex2f(-1.0f, -0.2f);    // x, y
//    glVertex2f(1.0f, -0.2f);
//    glVertex2f(-1.0f, -0.3f);    // x, y
//    glVertex2f(1.0f, -0.3f);
//    glVertex2f(-1.0f, -0.4f);    // x, y
//    glVertex2f(1.0f, -0.4f);
//    glVertex2f(-1.0f, -0.5f);    // x, y
//    glVertex2f(1.0f, -0.5f);
//    glVertex2f(-1.0f, -0.6f);    // x, y
//    glVertex2f(1.0f, -0.6f);
//    glVertex2f(-1.0f, -0.7f);    // x, y
//    glVertex2f(1.0f, -0.7f);
//    glVertex2f(-1.0f, -0.8f);    // x, y
//    glVertex2f(1.0f, -0.8f);
//    glVertex2f(-1.0f, -0.9f);    // x, y
//    glVertex2f(1.0f, -0.9f);
//
//    glVertex2f(0.1f, 1.0f);    // x, y
//    glVertex2f(0.1f, -1.0f);
//    glVertex2f(0.2f, 1.0f);    // x, y
//    glVertex2f(0.2f, -1.0f);
//    glVertex2f(0.3f, 1.0f);    // x, y
//    glVertex2f(0.3f, -1.0f);
//    glVertex2f(0.4f, 1.0f);    // x, y
//    glVertex2f(0.4f, -1.0f);

} // end drawTitleCard

// ============================================================
//  DISPLAY
// ============================================================
void display()
{
    if(night)        glClearColor(0.05f, 0.15f, 0.25f, 1.0f);
    else if(sunset)  glClearColor(1.0f,  0.45f, 0.0f,  1.0f);
    else if(rain)    glClearColor(0.30f, 0.45f, 0.60f, 1.0f);
    else             glClearColor(0.65f, 0.88f, 1.0f,  1.0f);

    glClear(GL_COLOR_BUFFER_BIT);

    drawSky();
    drawSun();
    drawMoon();
    drawStars();
    drawBirds();
    drawHills();
    drawClouds();
    drawScene();
    drawBBQStore();
    drawSea();
    drawNightOverlay();
    drawRain();
    drawTitleCard();

    glFlush();
}

// ============================================================
//  ANIMATION UPDATE
// ============================================================
void update(int value)
{
    if(!cover)
    {
        run[10] += 0.003f;
        if(run[10] > 1.25f) run[10] = -1.0f;
    }

    if(on)
    {
        if(!vehicleStop)
        {
            run[6] -= vehicleSpeed;
            if(!handup) run[3] += vehicleSpeed;
            run[4] += (vehicleSpeed + 0.001f);
            run[5] -= (vehicleSpeed + 0.001f);
            run[2] -= (vehicleSpeed + 0.002f);
        }
        run[0] += 0.0003f;
        run[1] -= 0.0002f;
        run[9] += 0.0009f;

        // Wrap vehicles
        if(run[3]  >  1.5f) run[3]  = -1.5f;
        if(run[4]  >  1.5f) run[4]  = -1.5f;
        if(run[5]  < -1.5f) run[5]  =  1.5f;
        if(run[6]  < -1.5f) run[6]  =  1.5f;
        if(run[2]  < -1.5f) run[2]  =  1.5f;
        if(run[0]  >  1.5f) run[0]  = -1.5f;
        if(run[1]  < -1.5f) run[1]  =  1.5f;
        if(run[9]  >  1.5f) run[9]  = -1.5f;

        // Rain
        if(rain)
        {
            for(int i = 0; i < 11; i++)
            {
                rainY[i] -= 0.05f;
                if(rainY[i] < -1.0f) rainY[i] = 1.0f;
            }
        }

        // Sun / moon drift
        _sunX  += 0.0002f; if(_sunX  >  1.2f) _sunX  = -1.2f;
        _moonX += 0.0002f; if(_moonX >  1.2f) _moonX = -1.2f;

        // Bird wing flap
        wingTimer++;
        if(wingTimer > 8)
        {
            wingTimer = 0;
            if(wingUp) { _birdWing += 0.5f; if(_birdWing >= 1.0f) wingUp = false; }
            else       { _birdWing -= 0.5f; if(_birdWing <= 0.0f) wingUp = true;  }
        }
        for(int i = 0; i < 3; i++) { birdX[i] += 0.003f; if(birdX[i] > 1.5f) birdX[i] = -1.5f; }

        // Star twinkle
        starTimer++;
        if(starTimer > 20)
        {
            starTimer = 0;
            if(starBright) { _starTwinkle -= 0.2f; if(_starTwinkle <= 0.5f) starBright = false; }
            else           { _starTwinkle += 0.2f; if(_starTwinkle >= 1.0f) starBright = true;  }
        }

        // Waves
        if(waveUp)
        {
            for(int i = 0; i < 17; i++) wave[i] += 0.002f;
            if(wave[0] >= 0.02f) waveUp = false;
        }
        else
        {
            for(int i = 0; i < 17; i++) wave[i] -= 0.002f;
            if(wave[0] <= -0.02f) waveUp = true;
        }

        // Traffic light
        trafficTimer++;
        if(trafficTimer > 100) { trafficTimer = 0; trafficState = (trafficState + 1) % 3; }

        // Horse walking
        _horseX += 0.0004f; if(_horseX > 0.6f) _horseX = -0.6f;

        // === SHELL SELLERS WALKING (loop left across beach) ===
        _sell1X -= 0.0003f; if(_sell1X < -1.2f) _sell1X = 1.2f;
        _sell2X -= 0.0003f; if(_sell2X < -1.2f) _sell2X = 1.2f;

        // Couple walking
        if(_coupleDir) {
            _coupleX += 0.0006f;
            if(_coupleX > 0.55f) _coupleDir = false;
        } else {
            _coupleX -= 0.0006f;
            if(_coupleX < -0.25f) _coupleDir = true;
        }

        // Child jump
        _childTimer++;
        if(_childTimer > 10)
        {
            _childTimer = 0;
            if(_childUp) { _childJump += 0.005f; if(_childJump >= 0.03f) _childUp = false; }
            else         { _childJump -= 0.005f; if(_childJump <= 0.0f)  _childUp = true;  }
        }

        // Coconut leaf sway
        if(_leafDir) { _leafSway += 0.3f; if(_leafSway >= 12.0f) _leafDir = false; }
        else         { _leafSway -= 0.3f; if(_leafSway <= -12.0f) _leafDir = true; }

        // Photographer flash
        _flashTimer++;
        if(_flashTimer > 60) { _flashOn = true; _flashTimer = 0; }
        if(_flashOn && _flashTimer > 5) { _flashOn = false; }

        // Fisherman net
        if(_netDir) { _netAngle += 0.5f; if(_netAngle >= 20.0f) _netDir = false; }
        else        { _netAngle -= 0.5f; if(_netAngle <= -20.0f) _netDir = true; }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// ============================================================
//  KEYBOARD
// ============================================================
void keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
        case 's': case 'S': cover = !cover;          break;
        case '1': on = true;                         break;
        case '0': on = false;                        break;
        case 'n': case 'N': night = true;  sunset = false; break;
        case 'd': case 'D': night = false; sunset = false; break;
        case 'u': case 'U': sunset = !sunset; night = false; break;
        case 'r': case 'R': rain = true;             break;
        case 'e': case 'E': rain = false;            break;
        case 'h': case 'H': handup = !handup;        break;
        case '+': glutFullScreen();                  break;
        case '-': glutReshapeWindow(900, 600); glutPositionWindow(100, 50); break;
        case 'x': case 'X': exit(0);                break;
    }
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y)
{
    if(key == GLUT_KEY_UP)
    {
        vehicleSpeed += 0.001f;
    }
    if(key == GLUT_KEY_DOWN)
    {
        vehicleSpeed -= 0.001f;
        if(vehicleSpeed < 0.0f) vehicleSpeed = 0.0f;
    }
    glutPostRedisplay();
}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================
//  MAIN
// ============================================================
int main(int argc, char** argv)
{
    cout << "==========================================" << endl;
    cout << "   MARINE DRIVE COX BAZAR - 2D SCENE     " << endl;
    cout << "==========================================" << endl;
    cout << "Press s    : Show / Hide Title Card"  << endl;
    cout << "Press 1    : Animation ON"            << endl;
    cout << "Press 0    : Animation OFF"           << endl;
    cout << "Press n    : Night Mode"              << endl;
    cout << "Press d    : Day Mode"                << endl;
    cout << "Press u    : Sunset Mode"             << endl;
    cout << "Press r    : Rain ON"                 << endl;
    cout << "Press e    : Rain OFF"                << endl;
    cout << "Press h    : Hands Up/Down"           << endl;
    cout << "Press UP   : Speed Up Vehicles"       << endl;
    cout << "Press DOWN : Slow Down Vehicles"      << endl;
    cout << "Press +    : Full Screen"             << endl;
    cout << "Press -    : Restore Screen"          << endl;
    cout << "Press x    : Exit"                    << endl;
    cout << "==========================================" << endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(900, 600);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("MARINE DRIVE COX BAZAR");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(16, update, 0);

    PlaySound(TEXT(""), NULL, SND_ASYNC | SND_LOOP | SND_NODEFAULT);

    glutMainLoop();
    return 0;
}
//    glVertex2f(0.5f, 1.0f);    // x, y
//    glVertex2f(0.5f, -1.0f);
//    glVertex2f(0.6f, 1.0f);    // x, y
//    glVertex2f(0.6f, -1.0f);
//    glVertex2f(0.7f, 1.0f);    // x, y
//    glVertex2f(0.7f, -1.0f);
//    glVertex2f(0.8f, 1.0f);    // x, y
//    glVertex2f(0.8f, -1.0f);
//    glVertex2f(0.9f, 1.0f);    // x, y
//    glVertex2f(0.9f, -1.0f);
//
//    glVertex2f(-0.1f, 1.0f);    // x, y
//    glVertex2f(-0.1f, -1.0f);
//    glVertex2f(-0.2f, 1.0f);    // x, y
//    glVertex2f(-0.2f, -1.0f);
//    glVertex2f(-0.3f, 1.0f);    // x, y
//    glVertex2f(-0.3f, -1.0f);
//    glVertex2f(-0.4f, 1.0f);    // x, y
//    glVertex2f(-0.4f, -1.0f);
//    glVertex2f(-0.5f, 1.0f);    // x, y
//    glVertex2f(-0.5f, -1.0f);
//    glVertex2f(-0.6f, 1.0f);    // x, y
//    glVertex2f(-0.6f, -1.0f);
//    glVertex2f(-0.7f, 1.0f);    // x, y
//    glVertex2f(-0.7f, -1.0f);
//    glVertex2f(-0.8f, 1.0f);    // x, y
//    glVertex2f(-0.8f, -1.0f);
//    glVertex2f(-0.9f, 1.0f);    // x, y
//    glVertex2f(-0.9f, -1.0f);
//    glEnd();


}

void display()
{
    // Set background clear color
    if(night)        clearColor(0.05f, 0.15f, 0.25f, 1.0f);
    else if(sunset)  clearColor(1.0f,  0.45f, 0.0f,  1.0f);
    else if(rain)    clearColor(0.30f, 0.45f, 0.60f, 1.0f);
    else             clearColor(0.65f, 0.88f, 1.0f,  1.0f);

    glClear(GL_COLOR_BUFFER_BIT);

    drawSky();
    drawSun();
    drawMoon();
    drawStars();
    drawBirds();
    drawHills();
    drawClouds();
    drawScene();
    drawBBQStore();
    drawSea();
    drawNightOverlay();
    drawRain();
    drawTitleCard();

    glFlush();
}


void update(int value)
{
    if(!cover)
    {
        run[10] += 0.003f;

        if (run[10] > 1.25)
    {
        run[10] = -1;
    }

    }


    if(on)
    {
        if(!vehicleStop)
        {
            run[6] -= vehicleSpeed;
            if(!handup)
            {
                run[3] += vehicleSpeed;
            }
            run[4] += (vehicleSpeed+ 0.001);
            run[5] -= (vehicleSpeed+ 0.001);
            run[2] -= (vehicleSpeed+ 0.002);
        }
        run[0] += 0.0003f;
        run[1] -= 0.0002f;
        run[9] += 0.0009f;

        if(rain)
        {
            rainY[0] -= 0.05;
            rainY[1] -= 0.05;
            rainY[2] -= 0.05;
            rainY[3] -= 0.05;
            rainY[4] -= 0.05;
            rainY[5] -= 0.05;
            rainY[6] -= 0.05;
            rainY[7] -= 0.05;
            rainY[8] -= 0.05;
            rainY[9] -= 0.05;
            i += 1;
            rainX[0] -= 0.0055;
            rainX[1] -= 0.0055;
            rainX[2] -= 0.0055;
            rainX[3] -= 0.0055;
            rainX[4] -= 0.0055;
            rainX[5] -= 0.0055;
            rainX[6] -= 0.0055;
            rainX[7] -= 0.0055;
            rainX[8] -= 0.0055;
            rainX[9] -= 0.0055;
        }

        if(waveUp)
        {
            for(int k = 0; k < 17; k++) wave[k] += 0.00042f;
        }
        else
        {
            for(int k = 0; k < 17; k++) wave[k] -= 0.00032f;
        }


        if(!night && !rain)
        {
            run[7] -= 0.0005f;
            run[8] += 0.0005f;
            //run[9] += 0.0009f;
        }
    }

    if (run[6] < -1.7)
    {
        run[6] = 0.5;//float rainY[1] = 0.0;
    }
    if (run[4] > 1)
    {
        run[4] = -1.2;
    }
    if (run[5] < -1)
    {
        run[5] = 1.2;
    }
    if (run[3] > 1.7)
    {
        run[3] = - 0.6;
    }
    if (run[2] < -1.7)
    {
        run[2] = 1.3;
    }
    if (run[0] > 1.7)
    {
        run[0]= - 0.6;
    }
    if (run[1] < -1.9)
    {
        run[1]= 0.35;
    }
    if (run[7] < - 0.8)
    {
        run[7]= 1.5;
    }
    if (run[8] > .8)
    {
        run[8]= -1.5;
    }
    if (run[9] > .9)
    {
        run[9]= -1.5;
    }
    if(rainY[0] < -1.9)
    {
        rainY[0] = 0.1;
        rainX[0] = 0;
    }
    if(rainY[1] < -1.7)
    {
        rainY[1] = 0.3;
        rainX[1] = 0;
    }
    if(rainY[2] < -1.5)
    {
        rainY[2] = 0.5;
        rainX[2] = 0;
    }
    if(rainY[3] < -1.3)
    {
        rainY[3] = 0.7;
        rainX[3] = 0;
    }
    if(rainY[4] < -1.1)
    {
        rainY[4] = 0.9;
        rainX[4] = 0;
    }
    if(rainY[5] < - 0.9)
    {
        rainY[5] = 1.1;
        rainX[5] = 0;
    }
    if(rainY[6] < - 0.7)
    {
        rainY[6] = 1.3;
        rainX[6] = 0;
    }
    if(rainY[7] < - 0.5)
    {
        rainY[7] = 1.5;
        rainX[7] = 0;
    }
    if(rainY[8] < - 0.3)
    {
        rainY[8] = 1.7;
        rainX[8] = 0;
    }
    if(rainY[9] < - 0.1)
    {
        rainY[9] = 1.9;
        rainX[9] = 0;
    }

    if(rainStep == 120)
    {
        resetRain();
    }

    if (wave[0] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[0] < 0.0)
    {
        waveUp = true;
    }
    if (wave[1] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[1] < 0.0)
    {
        waveUp = true;
    }
    if (wave[2] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[2] < 0.0)
    {
        waveUp = true;
    }
    if (wave[3] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[3] < 0.0)
    {
        waveUp = true;
    }
    if (wave[4] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[4] < 0.0)
    {
        waveUp = true;
    }

    if (wave[5] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[5] < 0.0)
    {
        waveUp = true;
    }
    if (wave[6] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[6] < 0.0)
    {
        waveUp = true;
    }
    if (wave[7] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[7] < 0.0)
    {
        waveUp = true;
    }
    if (wave[8] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[8] < 0.0)
    {
        waveUp = true;
    }
    if (wave[9] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[9] < 0.0)
    {
        waveUp = true;
    }

    if (wave[10] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[10] < 0.0)
    {
        waveUp = true;
    }
    if (wave[11] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[11] < 0.0)
    {
        waveUp = true;
    }
    if (wave[12] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[12] < 0.0)
    {
        waveUp = true;
    }
    if (wave[13] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[13] < 0.0)
    {
        waveUp = true;
    }
    if (wave[14] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[14] < 0.0)
    {
        waveUp = true;
    }
    if (wave[15] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[15] < 0.0)
    {
        waveUp = true;
    }
    if (wave[16] >= 0.08)
    {
        waveUp = false;
    }
    if (wave[16] < 0.0)
    {
        waveUp = true;
    }

    // === BIRD ANIMATION (new) ===
    if(!cover)
    {
        birdX[0] += 0.0035f;
        birdX[1] += 0.003f;
        birdX[2] += 0.0028f;
        if(birdX[0] > 1.3f)  birdX[0] = -1.3f;
        if(birdX[1] > 1.3f)  birdX[1] = -1.5f;
        if(birdX[2] > 1.3f)  birdX[2] = -1.8f;

        // Wing flap
        wingTimer++;
        if(wingTimer > 8) {
            wingTimer = 0;
            wingUp = !wingUp;
        }
        if(wingUp)  _birdWing += 0.15f;
        else        _birdWing -= 0.15f;
        if(_birdWing >  1.0f) _birdWing =  1.0f;
        if(_birdWing < -1.0f) _birdWing = -1.0f;
    }

    // === MOVING SUN (new) ===
    if(!night && !cover)
    {
        _sunX += 0.00015f;
        if(_sunX > 0.9f) _sunX = -0.9f;
    }

    // === MOVING MOON (new) ===
    if(night && !cover)
    {
        _moonX -= 0.00012f;
        if(_moonX < -0.9f) _moonX = 0.9f;
    }

    // === STAR TWINKLE (new) ===
    if(night) {
        starTimer++;
        if(starTimer > 15) {
            starTimer = 0;
            starBright = !starBright;
        }
        _starTwinkle = starBright ? 1.0f : 0.55f;
    }

    // === TRAFFIC LIGHT CYCLE ===
    trafficTimer++;
    if(trafficTimer > 100) {
        trafficTimer = 0;
        trafficState = (trafficState + 1) % 3;
    }

    // === SHELL SELLERS WALKING (loop left across beach) ===
    if(!cover) {
        _sell1X -= 0.0008f;
        if(_sell1X < -1.1f) _sell1X = 1.1f;
        _sell2X -= 0.0006f;
        if(_sell2X < -1.1f) _sell2X = 1.1f;
    }

    // === HORSE + OWNER WALKING (loop right) ===
    if(!cover) {
        _horseX += 0.0005f;
        if(_horseX > 1.2f) _horseX = -1.2f;
    }

    // === NEW: COUPLE WALKING BACK & FORTH ===
    if(!cover) {
        if(_coupleDir) {
            _coupleX += 0.0006f;
            if(_coupleX > 0.55f) _coupleDir = false;
        } else {
            _coupleX -= 0.0006f;
            if(_coupleX < -0.25f) _coupleDir = true;
        }
    }

    // === NEW: CHILD JUMP ANIMATION ===
    if(!cover) {
        _childTimer++;
        if(_childTimer > 4) {
            _childTimer = 0;
            if(_childUp) {
                _childJump += 0.006f;
                if(_childJump >= 0.030f) _childUp = false;
            } else {
                _childJump -= 0.006f;
                if(_childJump <= 0.0f) {
                    _childJump = 0.0f;
                    _childUp = true;
                }
            }
        }
    }

    // === NEW: COCONUT LEAF SWAY ===
    if(!cover) {
        if(_leafDir) {
            _leafSway += 0.15f;
            if(_leafSway > 4.0f) _leafDir = false;
        } else {
            _leafSway -= 0.15f;
            if(_leafSway < -4.0f) _leafDir = true;
        }
    }

    // === NEW: PHOTOGRAPHER FLASH ===
    if(!cover) {
        _flashTimer++;
        if(_flashOn) {
            if(_flashTimer > 4) {
                _flashOn = false;
                _flashTimer = 0;
            }
        } else {
            if(_flashTimer > 80) {
                _flashOn = true;
                _flashTimer = 0;
            }
        }
    }

    // === NEW: FISHERMAN NET SWING ===
    if(!cover) {
        if(_netDir) {
            _netAngle += 0.6f;
            if(_netAngle > 25.0f) _netDir = false;
        } else {
            _netAngle -= 0.6f;
            if(_netAngle < -10.0f) _netDir = true;
        }
    }

    glutPostRedisplay(); //Tell GLUT that the display has changed
    glutTimerFunc(25, update, 0);
}

void SpecialInput(int key, int x, int y)
{

    switch(key)
    {
    case GLUT_KEY_UP:
        if(vehicleSpeed >= 0)
        {
            vehicleStop = false;
        }
        vehicleSpeed = vehicleSpeed + 0.001;
        break;

    case GLUT_KEY_DOWN:
        //if(vehicleSpeed >= 0){
        if(vehicleSpeed > 0)
        {
            vehicleSpeed = vehicleSpeed - 0.001;
        }
        if(vehicleSpeed <= 0)
        {
            vehicleStop = true;
        }
        break;
    }
    glutPostRedisplay();
}

void Input(unsigned char key, int x, int y)
{

    switch(key)
    {
    case 'n':
        night = true;

        sndPlaySound("music\\night.wav", SND_ASYNC);

        break;
    case 'd':
        night = false;
        sndPlaySound("music\\day.wav", SND_ASYNC);
        break;

    case '0':
        on= false;
        break;

    case '1':
        on = true;
        break;

        case 's':
                if(cover)
        {
            cover = false;
        }
        else
        {
            cover = true;
        }
        break;

    case '+':
        glutFullScreen();
        break;

    case '-':
        glutReshapeWindow(1024,576);
        glutInitWindowPosition(50,40);
        break;

    case 'x':
        //glutHideWindow();
        exit(0);
        break;

    case 'r':
        rain = true;
        sndPlaySound("music\\rain.wav", SND_ASYNC);
        break;

    case 'e':
        rain = false;
        break;

    case 'u':
        // Sunset mode toggle
        if(sunset) {
            sunset = false;
        } else {
            sunset = true;
            night  = false;
        }
        break;

    case 'h':
        if(handup)
        {
            handup = false;
        }
        else
        {
            handup = true;
        }
        break;

    }
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    cout << "========================================" << endl;
    cout << "   MARINE DRIVE COX BAZAR - CONTROLS   " << endl;
    cout << "========================================" << endl;
    cout << "Press s : Start / Show Scene" << endl;
    cout << "Press 1 : Animation ON" << endl;
    cout << "Press 0 : Animation OFF" << endl;
    cout << "Press h : Hands up/down" << endl;
    cout << "Press n : Night mode" << endl;
    cout << "Press d : Day mode" << endl;
    cout << "Press u : Sunset mode (NEW)" << endl;
    cout << "Press r : Rain start" << endl;
    cout << "Press e : Rain stop" << endl;
    cout << "Press UP   : Speed up vehicles" << endl;
    cout << "Press DOWN : Speed down vehicles" << endl;
    cout << "Press +    : Full Screen" << endl;
    cout << "Press -    : Restore Screen" << endl;
    cout << "Press x    : Exit" << endl;
    cout << "----------------------------------------" << endl;
    cout << "NEW: Couple, Children, Photographer," << endl;
    cout << "     Fisherman, Coconut tree, Umbrella" << endl;
    cout << "========================================" << endl;

    glutInitWindowSize(1024,576);
    glutInit(&argc, argv);
    glutCreateWindow("MARINE DRIVE COX BAZAR");
    glutInitWindowPosition(50,40);
    glutDisplayFunc(display);
    glutSpecialFunc(SpecialInput);
    glutKeyboardFunc(Input);
    glutTimerFunc(25,update,0);

    glutMainLoop();

    return 0;
}


// ============================================================
//  DISPLAY
// ============================================================
void display()
{
    if(night)        glClearColor(0.05f, 0.15f, 0.25f, 1.0f);
    else if(sunset)  glClearColor(1.0f,  0.45f, 0.0f,  1.0f);
    else if(rain)    glClearColor(0.30f, 0.45f, 0.60f, 1.0f);
    else             glClearColor(0.65f, 0.88f, 1.0f,  1.0f);

    glClear(GL_COLOR_BUFFER_BIT);

    drawSky();
    drawSun();
    drawMoon();
    drawStars();
    drawBirds();
    drawHills();
    drawClouds();
    drawScene();
    drawBBQStore();
    drawSea();
    drawNightOverlay();
    drawRain();
    drawTitleCard();

    glFlush();
}

// ============================================================
//  ANIMATION UPDATE
// ============================================================
void update(int value)
{
    if(!cover)
    {
        run[10] += 0.003f;
        if(run[10] > 1.25f) run[10] = -1.0f;
    }

    if(on)
    {
        if(!vehicleStop)
        {
            run[6] -= vehicleSpeed;
            if(!handup) run[3] += vehicleSpeed;
            run[4] += (vehicleSpeed + 0.001f);
            run[5] -= (vehicleSpeed + 0.001f);
            run[2] -= (vehicleSpeed + 0.002f);
        }
        run[0] += 0.0003f;
        run[1] -= 0.0002f;
        run[9] += 0.0009f;

        if(run[3]  >  1.5f) run[3]  = -1.5f;
        if(run[4]  >  1.5f) run[4]  = -1.5f;
        if(run[5]  < -1.5f) run[5]  =  1.5f;
        if(run[6]  < -1.5f) run[6]  =  1.5f;
        if(run[2]  < -1.5f) run[2]  =  1.5f;
        if(run[0]  >  1.5f) run[0]  = -1.5f;
        if(run[1]  < -1.5f) run[1]  =  1.5f;
        if(run[9]  >  1.5f) run[9]  = -1.5f;

        if(rain)
        {
            for(int i = 0; i < 11; i++)
            {
                rainY[i] -= 0.05f;
                if(rainY[i] < -1.0f) rainY[i] = 1.0f;
            }
        }

        _sunX  += 0.0002f; if(_sunX  >  1.2f) _sunX  = -1.2f;
        _moonX += 0.0002f; if(_moonX >  1.2f) _moonX = -1.2f;

        wingTimer++;
        if(wingTimer > 8)
        {
            wingTimer = 0;
            if(wingUp) { _birdWing += 0.5f; if(_birdWing >= 1.0f) wingUp = false; }
            else       { _birdWing -= 0.5f; if(_birdWing <= 0.0f) wingUp = true;  }
        }
        for(int i = 0; i < 3; i++) { birdX[i] += 0.003f; if(birdX[i] > 1.5f) birdX[i] = -1.5f; }

        starTimer++;
        if(starTimer > 20)
        {
            starTimer = 0;
            if(starBright) { _starTwinkle -= 0.2f; if(_starTwinkle <= 0.5f) starBright = false; }
            else           { _starTwinkle += 0.2f; if(_starTwinkle >= 1.0f) starBright = true;  }
        }

        if(waveUp)
        {
            for(int i = 0; i < 17; i++) wave[i] += 0.002f;
            if(wave[0] >= 0.02f) waveUp = false;
        }
        else
        {
            for(int i = 0; i < 17; i++) wave[i] -= 0.002f;
            if(wave[0] <= -0.02f) waveUp = true;
        }

        trafficTimer++;
        if(trafficTimer > 100) { trafficTimer = 0; trafficState = (trafficState + 1) % 3; }

        _horseX += 0.0004f; if(_horseX > 0.6f) _horseX = -0.6f;

        // === SHELL SELLERS WALKING (loop left across beach) ===
        _sell1X -= 0.0003f; if(_sell1X < -1.2f) _sell1X = 1.2f;
        _sell2X -= 0.0003f; if(_sell2X < -1.2f) _sell2X = 1.2f;

        if(_coupleDir) {
            _coupleX += 0.0006f;
            if(_coupleX > 0.55f) _coupleDir = false;
        } else {
            _coupleX -= 0.0006f;
            if(_coupleX < -0.25f) _coupleDir = true;
        }

        _childTimer++;
        if(_childTimer > 10)
        {
            _childTimer = 0;
            if(_childUp) { _childJump += 0.005f; if(_childJump >= 0.03f) _childUp = false; }
            else         { _childJump -= 0.005f; if(_childJump <= 0.0f)  _childUp = true;  }
        }

        if(_leafDir) { _leafSway += 0.3f; if(_leafSway >= 12.0f) _leafDir = false; }
        else         { _leafSway -= 0.3f; if(_leafSway <= -12.0f) _leafDir = true; }

        _flashTimer++;
        if(_flashTimer > 60) { _flashOn = true; _flashTimer = 0; }
        if(_flashOn && _flashTimer > 5) { _flashOn = false; }

        if(_netDir) { _netAngle += 0.5f; if(_netAngle >= 20.0f) _netDir = false; }
        else        { _netAngle -= 0.5f; if(_netAngle <= -20.0f) _netDir = true; }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// ============================================================
//  INPUT
// ============================================================
void keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
        case 's': case 'S': cover = !cover;               break;
        case '1': on = true;                              break;
        case '0': on = false;                             break;
        case 'n': case 'N': night = true;  sunset = false; break;
        case 'd': case 'D': night = false; sunset = false; break;
        case 'u': case 'U': sunset = !sunset; night = false; break;
        case 'r': case 'R': rain = true;                  break;
        case 'e': case 'E': rain = false;                 break;
        case 'h': case 'H': handup = !handup;             break;
        case '+': glutFullScreen();                       break;
        case '-': glutReshapeWindow(900, 600); glutPositionWindow(100, 50); break;
        case 'x': case 'X': exit(0);                     break;
    }
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y)
{
    if(key == GLUT_KEY_UP)   vehicleSpeed += 0.001f;
    if(key == GLUT_KEY_DOWN) { vehicleSpeed -= 0.001f; if(vehicleSpeed < 0.0f) vehicleSpeed = 0.0f; }
    glutPostRedisplay();
}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

// ============================================================
//  MAIN
// ============================================================
int main(int argc, char** argv)
{
    cout << "==========================================" << endl;
    cout << "   MARINE DRIVE COX BAZAR - 2D SCENE     " << endl;
    cout << "==========================================" << endl;
    cout << "Press s    : Show / Hide Title Card"       << endl;
    cout << "Press 1    : Animation ON"                 << endl;
    cout << "Press 0    : Animation OFF"                << endl;
    cout << "Press n    : Night Mode"                   << endl;
    cout << "Press d    : Day Mode"                     << endl;
    cout << "Press u    : Sunset Mode"                  << endl;
    cout << "Press r    : Rain ON"                      << endl;
    cout << "Press e    : Rain OFF"                     << endl;
    cout << "Press h    : Hands Up/Down"                << endl;
    cout << "Press UP   : Speed Up Vehicles"            << endl;
    cout << "Press DOWN : Slow Down Vehicles"           << endl;
    cout << "Press +    : Full Screen"                  << endl;
    cout << "Press -    : Restore Screen"               << endl;
    cout << "Press x    : Exit"                         << endl;
    cout << "==========================================" << endl;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(900, 600);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("MARINE DRIVE COX BAZAR");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(16, update, 0);

    PlaySound(TEXT(""), NULL, SND_ASYNC | SND_LOOP | SND_NODEFAULT);

    glutMainLoop();
    return 0;
}

    glutCreateWindow("MARINE DRIVE COX BAZAR");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(16, update, 0);

    PlaySound(TEXT(""), NULL, SND_ASYNC | SND_LOOP | SND_NODEFAULT);

    glutMainLoop();
    return 0;
}
