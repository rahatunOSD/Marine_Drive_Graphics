#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MARINE DRIVE 3D - Auto-Update Script
=====================================
এই script চালালে main3d.cpp তে ৩টি নতুন ফিচার automatically যোগ হবে:
  1. Ice Cream Stall (সমুদ্র সৈকতে)
  2. Swimmers Animation (সমুদ্রে সাঁতারু)
  3. Lightning/Thunder Effect ('l' key দিয়ে toggle)

ব্যবহার:
  1. এই script (apply_updates.py) কে main3d.cpp এর একই ফোল্ডারে রাখুন
  2. Python 3 দিয়ে চালান: python apply_updates.py
  3. main3d_updated.cpp তৈরি হবে
  4. Code::Blocks এ main3d_updated.cpp দিয়ে compile করুন

New Controls:
  l / L .... Lightning on/off (বজ্রপাত)
"""

import sys
import os

# ============================================================
# NEW CODE BLOCKS TO INSERT
# ============================================================

NEW_GLOBALS = '''
// ---- NEW FEATURES ----
// Lightning / Thunder
bool  thunderOn    = false;
float thunderTimer = 2.0f;      // countdown to next strike
float thunderFlash = 0.0f;      // current brightness 0..1
float thunderX     = 0.0f;
float thunderZ     = 30.0f;

// Swimmers (4 animated)
float swimPhase[4] = { 0.0f, 1.57f, 3.14f, 4.71f };
'''

NEW_FUNCTIONS = r'''
// ============================================================
//  ICE CREAM STALL (নতুন বিল্ডিং)
// ============================================================
static void drawIceCreamStall(float cx, float cz)
{
    float gY = -0.50f;

    // Wooden base counter
    setMat(0.55f, 0.35f, 0.18f);
    glPushMatrix();
    glTranslatef(cx, gY + 0.60f, cz);
    drawBox(4.0f, 1.20f, 1.80f);
    glPopMatrix();

    // counter top
    setMat(0.75f, 0.55f, 0.30f);
    glPushMatrix();
    glTranslatef(cx, gY + 1.22f, cz);
    drawBox(4.2f, 0.12f, 2.0f);
    glPopMatrix();

    // Ice cream tubs on counter (5 flavors)
    float flavorsR[] = { 0.95f, 0.98f, 0.55f, 0.90f, 0.80f };
    float flavorsG[] = { 0.40f, 0.82f, 0.85f, 0.60f, 0.40f };
    float flavorsB[] = { 0.55f, 0.85f, 0.45f, 0.85f, 0.70f };
    for (int i = 0; i < 5; ++i) {
        float tx = cx - 1.6f + i * 0.80f;
        setMat(0.95f, 0.95f, 0.95f);
        glPushMatrix();
        glTranslatef(tx, gY + 1.40f, cz - 0.20f);
        drawBox(0.55f, 0.30f, 0.55f);
        glPopMatrix();
        setMat(flavorsR[i], flavorsG[i], flavorsB[i]);
        glPushMatrix();
        glTranslatef(tx, gY + 1.62f, cz - 0.20f);
        glutSolidSphere(0.22f, 12, 10);
        glPopMatrix();
    }

    // 4 wooden posts
    setMat(0.45f, 0.28f, 0.14f);
    float postX[2] = { cx - 2.2f, cx + 2.2f };
    float postZ[2] = { cz - 1.0f, cz + 1.0f };
    for (int px = 0; px < 2; ++px)
        for (int pz = 0; pz < 2; ++pz) {
            glPushMatrix();
            glTranslatef(postX[px], gY + 2.80f, postZ[pz]);
            drawBox(0.16f, 5.60f, 0.16f);
            glPopMatrix();
        }

    // RED & WHITE striped awning
    float awningY = gY + 5.60f;
    float awningW = 5.0f;
    float awningD = 2.8f;
    int   nStripes = 8;
    for (int s = 0; s < nStripes; ++s) {
        float t0 = s       / (float)nStripes;
        float t1 = (s + 1) / (float)nStripes;
        float x0 = cx - awningW * 0.5f + t0 * awningW;
        float x1 = cx - awningW * 0.5f + t1 * awningW;
        if (s % 2 == 0) setMat(0.92f, 0.15f, 0.15f);
        else             setMat(0.98f, 0.98f, 0.98f);
        glBegin(GL_QUADS);
        glNormal3f(0, 1, 0.4f);
        glVertex3f(x0, awningY,         cz + awningD * 0.5f);
        glVertex3f(x1, awningY,         cz + awningD * 0.5f);
        glVertex3f(x1, awningY - 0.8f,  cz - awningD * 0.5f);
        glVertex3f(x0, awningY - 0.8f,  cz - awningD * 0.5f);
        glEnd();
    }
    // awning fringe
    setMat(0.92f, 0.15f, 0.15f);
    for (int f = 0; f < 10; ++f) {
        float fx = cx - awningW * 0.5f + f * (awningW / 10.0f);
        glPushMatrix();
        glTranslatef(fx + awningW / 20.0f, awningY - 0.75f, cz + awningD * 0.5f);
        drawBox(awningW / 10.0f * 0.7f, 0.25f, 0.06f);
        glPopMatrix();
    }

    // Giant ice cream cone sign on top
    float coneBaseY = awningY + 0.10f;
    setMat(0.92f, 0.72f, 0.42f);
    glBegin(GL_TRIANGLES);
    glNormal3f(0, 0, 1);
    glVertex3f(cx - 0.50f, coneBaseY,        cz + awningD * 0.5f + 0.05f);
    glVertex3f(cx + 0.50f, coneBaseY,        cz + awningD * 0.5f + 0.05f);
    glVertex3f(cx,         coneBaseY + 1.20f, cz + awningD * 0.5f + 0.05f);
    glEnd();
    setMat(0.98f, 0.75f, 0.85f);
    glPushMatrix();
    glTranslatef(cx, coneBaseY + 1.30f, cz + awningD * 0.5f + 0.05f);
    glutSolidSphere(0.42f, 14, 12);
    glPopMatrix();
    setMat(0.72f, 0.48f, 0.30f);
    glPushMatrix();
    glTranslatef(cx, coneBaseY + 1.78f, cz + awningD * 0.5f + 0.05f);
    glutSolidSphere(0.35f, 12, 10);
    glPopMatrix();

    // "ICE CREAM" banner
    if (night || sunset) {
        GLfloat em[4] = { 0.40f, 0.10f, 0.10f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
    }
    setMat(0.12f, 0.08f, 0.08f);
    glPushMatrix();
    glTranslatef(cx, awningY - 1.05f, cz + awningD * 0.5f + 0.10f);
    drawBox(3.2f, 0.55f, 0.12f);
    glPopMatrix();
    GLfloat zerO[4] = { 0, 0, 0, 1 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zerO);

    draw3DText("ICE CREAM",
               cx, awningY - 1.05f, cz + awningD * 0.5f + 0.18f,
               0.42f,
               0.98f, 0.85f, 0.25f);

    // Vendor behind counter
    {
        float vx = cx - 0.5f;
        float vz = cz + 0.60f;
        setMat(0.20f, 0.20f, 0.45f);
        glPushMatrix(); glTranslatef(vx - 0.08f, gY + 0.38f, vz); drawBox(0.14f, 0.75f, 0.14f); glPopMatrix();
        glPushMatrix(); glTranslatef(vx + 0.08f, gY + 0.38f, vz); drawBox(0.14f, 0.75f, 0.14f); glPopMatrix();
        setMat(0.96f, 0.96f, 0.96f);
        glPushMatrix(); glTranslatef(vx, gY + 1.00f, vz); drawBox(0.35f, 0.55f, 0.22f); glPopMatrix();
        setMat(0.92f, 0.78f, 0.62f);
        glPushMatrix(); glTranslatef(vx, gY + 1.40f, vz); glutSolidSphere(0.14f, 12, 10); glPopMatrix();
        setMat(0.95f, 0.95f, 0.95f);
        glPushMatrix(); glTranslatef(vx, gY + 1.58f, vz); drawBox(0.25f, 0.08f, 0.25f); glPopMatrix();
        glPushMatrix(); glTranslatef(vx, gY + 1.70f, vz); glutSolidSphere(0.15f, 10, 8); glPopMatrix();
    }
}

// ============================================================
//  SWIMMERS অ্যানিমেশন (নতুন ক্যারেক্টার)
// ============================================================
static void drawSwimmer(float x, float z, float phase)
{
    float gY = -0.45f;
    glPushMatrix();
    glTranslatef(x, gY, z);
    float bob = sinf(phase * 1.5f) * 0.08f;
    glTranslatef(0, bob, 0);

    float hue = fmodf(phase * 0.05f, 1.0f);
    float suitR, suitG, suitB;
    if (hue < 0.33f)      { suitR = 0.20f; suitG = 0.50f; suitB = 0.90f; }
    else if (hue < 0.66f) { suitR = 0.90f; suitG = 0.20f; suitB = 0.20f; }
    else                  { suitR = 0.20f; suitG = 0.75f; suitB = 0.30f; }

    float skinR = 0.92f, skinG = 0.78f, skinB = 0.62f;

    // body (lying flat)
    setMat(suitR, suitG, suitB);
    glPushMatrix();
    glScalef(1.8f, 0.28f, 0.55f);
    glutSolidSphere(1.0f, 14, 10);
    glPopMatrix();

    // head
    setMat(skinR, skinG, skinB);
    glPushMatrix();
    glTranslatef(1.0f, 0.10f, 0.0f);
    glutSolidSphere(0.20f, 12, 10);
    glPopMatrix();

    // swim cap
    setMat(suitR * 0.7f, suitG * 0.7f, suitB * 0.7f);
    glPushMatrix();
    glTranslatef(1.0f, 0.18f, 0.0f);
    glScalef(0.20f, 0.12f, 0.20f);
    glutSolidSphere(1.0f, 10, 8);
    glPopMatrix();

    // goggles
    setMat(0.15f, 0.15f, 0.15f);
    glPushMatrix(); glTranslatef(1.18f, 0.12f,  0.08f); glutSolidSphere(0.055f, 8, 6); glPopMatrix();
    glPushMatrix(); glTranslatef(1.18f, 0.12f, -0.08f); glutSolidSphere(0.055f, 8, 6); glPopMatrix();

    // arms (freestyle stroke)
    float rightArmAngle = sinf(phase) * 75.0f;
    float leftArmAngle  = sinf(phase + PI) * 75.0f;

    setMat(skinR, skinG, skinB);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -0.38f);
    glRotatef(rightArmAngle, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, 0.0f, -0.45f);
    drawBox(0.60f, 0.10f, 0.10f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.38f);
    glRotatef(leftArmAngle, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, 0.0f, 0.45f);
    drawBox(0.60f, 0.10f, 0.10f);
    glPopMatrix();

    // legs (flutter kick)
    float kickR = sinf(phase * 3.0f) * 20.0f;
    float kickL = sinf(phase * 3.0f + PI) * 20.0f;
    setMat(suitR, suitG, suitB);
    glPushMatrix();
    glTranslatef(-0.80f, 0.0f,  0.18f);
    glRotatef(kickR, 1.0f, 0.0f, 0.0f);
    drawBox(0.55f, 0.14f, 0.14f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-0.80f, 0.0f, -0.18f);
    glRotatef(kickL, 1.0f, 0.0f, 0.0f);
    drawBox(0.55f, 0.14f, 0.14f);
    glPopMatrix();

    // water splash
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float splashAlpha = (sinf(phase) > 0.5f) ? 0.70f : 0.20f;
    glColor4f(1.0f, 1.0f, 1.0f, splashAlpha);
    glPointSize(4.0f);
    glBegin(GL_POINTS);
    for (int s = 0; s < 6; ++s) {
        float sa = s * (2 * PI / 6.0f) + phase;
        glVertex3f(x + cosf(sa) * 0.55f, gY + bob + 0.05f, z + sinf(sa) * 0.35f);
    }
    glEnd();
    glPointSize(1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

// ============================================================
//  LIGHTNING / বজ্রপাত ইফেক্ট (নতুন আবহাওয়া)
// ============================================================
static void drawLightning()
{
    if (thunderFlash < 0.02f) return;

    // Whole-screen flash
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.9f, 0.9f, 1.0f, thunderFlash * 0.35f);
    glBegin(GL_QUADS);
    glVertex2f(-1, -1); glVertex2f(1, -1);
    glVertex2f(1,  1);  glVertex2f(-1, 1);
    glEnd();
    glDisable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    // 3D lightning bolt (zigzag)
    glDisable(GL_LIGHTING);
    glLineWidth(3.0f);
    glColor3f(1.0f, 1.0f, 0.6f * thunderFlash + 0.4f);

    float bx = thunderX;
    float bz = thunderZ;
    float startY = 60.0f;
    float endY   = -0.5f;

    glBegin(GL_LINE_STRIP);
    srand((unsigned)(thunderTimer * 1000));
    glVertex3f(bx, startY, bz);
    for (int i = 1; i < 8; ++i) {
        float t  = i / 8.0f;
        float py = startY + (endY - startY) * t;
        float ox = frand(-3.0f, 3.0f);
        float oz = frand(-1.5f, 1.5f);
        glVertex3f(bx + ox, py, bz + oz);
    }
    glVertex3f(bx, endY, bz);
    glEnd();

    // branch bolt
    glLineWidth(1.5f);
    glColor3f(0.9f, 0.9f, 1.0f);
    glBegin(GL_LINE_STRIP);
    float branchY = startY * 0.4f;
    glVertex3f(bx + 1.0f, branchY, bz);
    for (int i = 0; i < 4; ++i) {
        float py = branchY + (-5.0f - branchY) * (i / 4.0f);
        glVertex3f(bx + 1.0f + frand(-2.0f, 2.0f), py, bz + frand(-1.0f, 1.0f));
    }
    glEnd();

    glLineWidth(1.0f);

    // glow at impact
    glPointSize(8.0f);
    glColor3f(1.0f, 1.0f, 0.5f);
    glBegin(GL_POINTS);
    glVertex3f(bx, endY + 0.1f, bz);
    glEnd();
    glPointSize(1.0f);
    glEnable(GL_LIGHTING);
}

'''

# ============================================================
# MODIFICATIONS FOR EXISTING FUNCTIONS
# ============================================================

# In display(): after "drawBeachStuff();"
DISPLAY_ADDITION = '''
    // ---- NEW: Ice Cream Stall on beach ----
    drawIceCreamStall(80.0f, 3.0f);

    // ---- NEW: Swimmers in the sea ----
    if (animOn) {
        for (int i = 0; i < 4; ++i) {
            float sx = -45.0f + i * 28.0f;
            float sz = 15.0f + sinf(swimPhase[i] * 0.25f) * 6.0f;
            drawSwimmer(sx, sz, swimPhase[i]);
        }
    }

    // ---- NEW: Lightning bolt ----
    if (thunderOn) drawLightning();
'''

# In update(): after "waiterPhase += 0.06f;"
UPDATE_ADDITION = '''
        // NEW: Swimmer animation
        for (int i = 0; i < 4; ++i) swimPhase[i] += 0.05f;

        // NEW: Thunder/Lightning animation
        if (thunderOn) {
            if (thunderTimer > 0.0f) {
                thunderTimer -= 0.025f;
                thunderFlash *= 0.88f;
            } else {
                thunderFlash = frand(0.55f, 1.0f);
                thunderX     = frand(-90.0f, 90.0f);
                thunderZ     = frand(5.0f, 80.0f);
                thunderTimer = frand(1.5f, 6.0f);
            }
        }
'''

# ============================================================
# APPLY ALL CHANGES
# ============================================================

def apply_updates(input_path, output_path):
    print(f"Reading: {input_path}")
    with open(input_path, 'r', encoding='utf-8') as f:
        content = f.read()

    original_len = len(content)
    changes_made = []

    # --- CHANGE 1: Add new globals after waiterPhase ---
    marker1 = 'float waiterPhase = 0.0f;       // walking animation phase'
    if marker1 in content:
        content = content.replace(marker1, marker1 + '\n' + NEW_GLOBALS, 1)
        changes_made.append("✓ New globals added (thunder + swimmer phases)")
    else:
        print("WARNING: Could not find waiterPhase marker!")

    # --- CHANGE 2: Insert new functions before drawRain ---
    # Find the drawRain function and insert our new functions before it
    marker2 = '// ============================================================\n//  RAIN\n// ============================================================'
    if marker2 in content:
        content = content.replace(marker2, NEW_FUNCTIONS + '\n' + marker2, 1)
        changes_made.append("✓ New functions added (drawIceCreamStall, drawSwimmer, drawLightning)")
    else:
        print("WARNING: Could not find RAIN section marker!")

    # --- CHANGE 3: Update display() - add after drawBeachStuff() ---
    marker3 = '    // beach: umbrellas + people\n    drawBeachStuff();'
    if marker3 in content:
        content = content.replace(marker3, marker3 + '\n' + DISPLAY_ADDITION, 1)
        changes_made.append("✓ display() updated with new draw calls")
    else:
        # Try alternative
        marker3b = '    drawBeachStuff();'
        if marker3b in content:
            content = content.replace(marker3b, marker3b + '\n' + DISPLAY_ADDITION, 1)
            changes_made.append("✓ display() updated with new draw calls (alt)")
        else:
            print("WARNING: Could not find drawBeachStuff() in display!")

    # --- CHANGE 4: Update update() - add after waiterPhase update ---
    marker4 = '        waiterPhase += 0.06f;     // waiter walking animation'
    if marker4 in content:
        content = content.replace(marker4, marker4 + '\n' + UPDATE_ADDITION, 1)
        changes_made.append("✓ update() updated with swimmer + thunder animation")
    else:
        marker4b = '        waiterPhase += 0.06f;'
        if marker4b in content:
            content = content.replace(marker4b, marker4b + '\n' + UPDATE_ADDITION, 1)
            changes_made.append("✓ update() updated (alt)")
        else:
            print("WARNING: Could not find waiterPhase update in update()!")

    # --- CHANGE 5: Add 'l' key to keyboard() ---
    marker5 = "        case 'k': case 'K': rain = false; break;"
    new_key  = "        case 'l': case 'L': thunderOn = !thunderOn; thunderFlash = 0; thunderTimer = 0.5f; break;"
    if marker5 in content:
        content = content.replace(marker5, marker5 + '\n' + new_key, 1)
        changes_made.append("✓ keyboard(): 'l' key added for lightning toggle")
    else:
        print("WARNING: Could not find rain 'k' key in keyboard()!")

    # --- CHANGE 6: Update HUD sprintf ---
    old_hud = '''    sprintf(buf, "Marine Drive 3D  |  Speed: %.2f  |  %s%s%s",
            vehicleSpeed,
            night ? "Night" : (sunset ? "Sunset" : "Day"),
            rain  ? "  Rain"  : "",
            fogOn ? "  Fog"   : "");'''
    new_hud = '''    sprintf(buf, "Marine Drive 3D  |  Speed: %.2f  |  %s%s%s%s",
            vehicleSpeed,
            night ? "Night" : (sunset ? "Sunset" : "Day"),
            rain      ? "  Rain"      : "",
            thunderOn ? "  Lightning" : "",
            fogOn     ? "  Fog"       : "");'''
    if old_hud in content:
        content = content.replace(old_hud, new_hud, 1)
        changes_made.append("✓ HUD updated to show lightning status")
    else:
        print("WARNING: Could not find HUD sprintf!")

    # --- CHANGE 7: Update controls text ---
    old_controls = '"WASD/QE move  |  drag LMB look  |  arrows speed  |  n/y/u mode  |  r/k rain  |  f fog  |  g lights  |  c reset"'
    new_controls = '"WASD/QE move  |  drag LMB look  |  n/y/u mode  |  r/k rain  |  l lightning  |  f fog  |  g lights  |  c reset"'
    if old_controls in content:
        content = content.replace(old_controls, new_controls, 1)
        changes_made.append("✓ Controls help text updated")
    else:
        print("NOTE: Controls text not found (minor)")

    # --- CHANGE 8: Add comment for 'l' key in top header ---
    old_header_ctrl = '//     x / Esc ............. Exit'
    new_header_ctrl = '//     l ................... Lightning on / off\n//     x / Esc ............. Exit'
    if old_header_ctrl in content:
        content = content.replace(old_header_ctrl, new_header_ctrl, 1)
        changes_made.append("✓ Header controls comment updated")

    # Write output
    print(f"\nWriting: {output_path}")
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(content)

    print(f"\n{'='*50}")
    print("CHANGES APPLIED:")
    for c in changes_made:
        print(f"  {c}")
    print(f"\nOriginal size: {original_len:,} chars")
    print(f"Updated size:  {len(content):,} chars")
    print(f"Added:         {len(content)-original_len:,} chars")
    print(f"{'='*50}")
    print(f"\nSUCCESS! Output: {output_path}")
    print("Now open main3d_updated.cpp in Code::Blocks and Build!")


if __name__ == '__main__':
    # Find main3d.cpp
    script_dir = os.path.dirname(os.path.abspath(__file__))

    # Check same directory
    input_file = os.path.join(script_dir, 'main3d.cpp')
    if not os.path.exists(input_file):
        # Try parent directory
        input_file = os.path.join(os.path.dirname(script_dir), 'main3d.cpp')

    if not os.path.exists(input_file):
        print("ERROR: main3d.cpp not found!")
        print(f"Script directory: {script_dir}")
        print("Please put apply_updates.py in the same folder as main3d.cpp")
        sys.exit(1)

    output_file = os.path.join(os.path.dirname(input_file), 'main3d_updated.cpp')

    print("MARINE DRIVE 3D - Update Script")
    print("="*40)
    apply_updates(input_file, output_file)
