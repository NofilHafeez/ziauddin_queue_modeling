#ifdef _WIN32
  #define _USE_MATH_DEFINES   // FIX 1: ensure M_PI is defined on MSVC
  #include <windows.h>
  #include <GL/freeglut.h>
#elif __APPLE__
  #include <GLUT/glut.h>
#else
  #include <GL/glut.h>
#endif

#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <algorithm>

#ifndef M_PI
  #define M_PI 3.14159265358979323846  // FIX 1: fallback definition
#endif

// ─── Simulation parameters ───────────────────────────────────────────────────
static const int   TOTAL_PATIENTS        = 8;
static const float SERVICE_TIME          = 5.0f;
static const float MEAN_INTER_ARRIVAL    = 10.0f;
static const float SIM_SPEED            = 0.5f;
static const float ANIM_SPEED           = 2.5f;

// ─── Window ───────────────────────────────────────────────────────────────────
static int W = 1000, H = 660;

// ─── Colors ───────────────────────────────────────────────────────────────────
struct Col { float r, g, b, a; };
static Col BG      = {0.05f, 0.07f, 0.10f, 1};
static Col PANEL   = {0.08f, 0.12f, 0.17f, 1};
static Col ACCENT  = {0.00f, 0.78f, 1.00f, 1};
static Col GREEN   = {0.00f, 1.00f, 0.60f, 1};
static Col WARN    = {1.00f, 0.42f, 0.21f, 1};
static Col MUTED   = {0.29f, 0.44f, 0.56f, 1};
static Col WHITE   = {1.00f, 1.00f, 1.00f, 1};
static Col DARK    = {0.06f, 0.09f, 0.13f, 1};

// ─── Patient ──────────────────────────────────────────────────────────────────
enum PatientState { PENDING, WALKING_IN, IN_QUEUE, WALKING_TO_DOCTOR, BEING_SERVED, DONE };

struct Patient {
    int    id;
    int    arrival_sim_time;
    int    service_start;
    int    waiting_time;
    int    finish_time;

    PatientState state      = PENDING;
    float  px               = -80.0f;
    float  py               = 0.0f;
    float  targetX          = 0.0f;
    float  targetY          = 0.0f;
    float  serviceProgress  = 0.0f;
    float  alpha            = 1.0f;
    float  scale            = 1.0f;
    bool   animDone         = false;

    Col    color;
};

// ─── Simulation state ─────────────────────────────────────────────────────────
static std::vector<Patient> patients;
static int   simClock         = 0;
static bool  simRunning       = false;
static bool  simFinished      = false;
static float realTime         = 0.0f;
static float lastFrameTime    = 0.0f;

static float QUEUE_X_START    = 420.0f;
static float QUEUE_Y          = 340.0f;
static float DOCTOR_X         = 760.0f;
static float DOCTOR_Y         = 340.0f;
static float ENTRY_X          = 80.0f;
static float ENTRY_Y          = 340.0f;
static float SLOT_W           = 56.0f;

static float rho, Lq, L, Wq, Wt, P0;

struct Particle {
    float x, y, vx, vy, life, maxLife, size;
    Col color;
};
static std::vector<Particle> particles;
static float globalTime = 0.0f;

// ─── Helpers ──────────────────────────────────────────────────────────────────
static void glCol(Col c, float alphaOverride = -1.0f) {
    glColor4f(c.r, c.g, c.b, alphaOverride >= 0 ? alphaOverride : c.a);
}

static void fillRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
      glVertex2f(x,   y);
      glVertex2f(x+w, y);
      glVertex2f(x+w, y+h);
      glVertex2f(x,   y+h);
    glEnd();
}

static void strokeRect(float x, float y, float w, float h, float lw = 1.0f) {
    glLineWidth(lw);
    glBegin(GL_LINE_LOOP);
      glVertex2f(x,   y);
      glVertex2f(x+w, y);
      glVertex2f(x+w, y+h);
      glVertex2f(x,   y+h);
    glEnd();
}

// FIX 2: Corrected fillRoundRect — proper triangle fan closing the last arc
// back to the very first arc vertex, not back to an arbitrary corner point.
static void fillRoundRect(float x, float y, float w, float h, float r, int segs = 10) {
    float cx = x + w / 2.0f, cy = y + h / 2.0f;

    // Corner centres
    struct Corner { float ox, oy, startA; };
    Corner corners[4] = {
        { x + r,     y + r,     (float)M_PI          },   // bottom-left
        { x + w - r, y + r,     (float)(3*M_PI/2.0)  },   // bottom-right
        { x + w - r, y + h - r, 0.0f                 },   // top-right
        { x + r,     y + h - r, (float)(M_PI/2.0)    },   // top-left
    };

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);   // fan centre

    for (int c2 = 0; c2 < 4; c2++) {
        for (int i = 0; i <= segs; i++) {
            float a = corners[c2].startA + i * ((float)M_PI / 2.0f) / segs;
            glVertex2f(corners[c2].ox + r * cosf(a),
                       corners[c2].oy + r * sinf(a));
        }
    }
    // Close: repeat the very first arc vertex so the fan has no seam
    float closeA = corners[0].startA;
    glVertex2f(corners[0].ox + r * cosf(closeA),
               corners[0].oy + r * sinf(closeA));

    glEnd();
}

static void drawCircle(float cx, float cy, float rad, bool filled = true, int segs = 32) {
    glBegin(filled ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
    if (filled) glVertex2f(cx, cy);
    for (int i = 0; i <= segs; i++) {
        float a = 2.0f * (float)M_PI * i / segs;
        glVertex2f(cx + rad * cosf(a), cy + rad * sinf(a));
    }
    glEnd();
}

static void drawLine(float x1, float y1, float x2, float y2, float lw = 1.0f) {
    glLineWidth(lw);
    glBegin(GL_LINES);
    glVertex2f(x1, y1); glVertex2f(x2, y2);
    glEnd();
}

static void drawText(float x, float y, const std::string& s, void* font = GLUT_BITMAP_8_BY_13) {
    glRasterPos2f(x, y);
    for (char c : s) glutBitmapCharacter(font, c);
}

static std::string fmt(float v, int dec = 2) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(dec) << v;
    return ss.str();
}

// FIX 10: Guard against rho >= 1 (unstable / infinite queue)
static void computeFormulas() {
    float lambda = 1.0f / MEAN_INTER_ARRIVAL;
    float mu     = 1.0f / SERVICE_TIME;
    rho = lambda / mu;

    if (rho >= 1.0f) {
        // Queue is unstable — values are infinite; clamp for display
        Lq = Wq = L = Wt = 1e9f;
        P0 = 0.0f;
    } else {
        Lq = (rho * rho) / (1.0f - rho);
        L  = rho / (1.0f - rho);
        Wq = Lq / lambda;
        Wt = 1.0f / (mu - lambda);
        P0 = (1.0f - rho) * 100.0f;
    }
}

static void spawnBurst(float x, float y, Col c, int n = 18) {
    for (int i = 0; i < n; i++) {
        Particle p;
        p.x = x; p.y = y;
        float a   = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
        float spd = 30.0f + ((float)rand() / RAND_MAX) * 90.0f;
        p.vx = cosf(a) * spd;
        p.vy = sinf(a) * spd;
        p.maxLife = p.life = 0.5f + ((float)rand() / RAND_MAX) * 0.5f;
        p.size  = 2.0f + ((float)rand() / RAND_MAX) * 4.0f;
        p.color = c;
        particles.push_back(p);
    }
}

static Col patientColor(int id) {
    float hue = fmodf(id * 0.618f, 1.0f) * 360.0f;
    float h = hue / 60.0f;
    int   i = (int)h;
    float f = h - i, q = 1.0f - f, t2 = f;
    float r, g, b;
    switch (i % 6) {
        case 0: r=1;g=t2;b=0;  break;
        case 1: r=q;g=1;b=0;   break;
        case 2: r=0;g=1;b=t2;  break;
        case 3: r=0;g=q;b=1;   break;
        case 4: r=t2;g=0;b=1;  break;
        default:r=1;g=0;b=q;   break;
    }
    return {0.4f + r * 0.6f, 0.4f + g * 0.6f, 0.4f + b * 0.6f, 1.0f};
}

// ─── Init simulation ──────────────────────────────────────────────────────────
static void initSim() {
    patients.clear();
    particles.clear();
    simClock    = 0;
    realTime    = 0.0f;
    simRunning  = false;
    simFinished = false;

    // FIX 9: use roundf so fractional SERVICE_TIME is handled correctly
    int arrTime = 0, docFree = 0;
    srand((unsigned)time(nullptr));
    for (int i = 0; i < TOTAL_PATIENTS; i++) {
        Patient p;
        p.id    = i + 1;
        p.color = patientColor(i);
        p.state = PENDING;
        p.alpha = 1.0f;
        p.scale = 1.0f;

        int inter = rand() % 10 + 1;
        arrTime += inter;
        p.arrival_sim_time = arrTime;

        int sStart = (arrTime < docFree) ? docFree : arrTime;
        p.waiting_time  = sStart - arrTime;
        p.service_start = sStart;
        p.finish_time   = sStart + (int)roundf(SERVICE_TIME);  // FIX 9
        docFree = p.finish_time;

        p.px = -80.0f;
        p.py = ENTRY_Y;
        patients.push_back(p);
    }
}

static float queueSlotX(int slot) {
    return QUEUE_X_START + slot * SLOT_W;
}

// FIX 3: reassignQueueSlots also called when a patient enters the queue
static void reassignQueueSlots() {
    int slot = 0;
    for (auto& p : patients) {
        if (p.state == IN_QUEUE) {
            p.targetX = queueSlotX(slot);
            p.targetY = QUEUE_Y;
            slot++;
        }
    }
}

// ─── Update logic ─────────────────────────────────────────────────────────────
static void update(float dt) {
    globalTime += dt;

    for (auto& p : particles) {
        p.life -= dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vy -= 60.0f * dt;   // gravity (Y-up: negative vy = downward)
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return p.life <= 0; }), particles.end());

    if (!simRunning || simFinished) return;

    realTime += dt * SIM_SPEED * 10.0f;
    simClock = (int)realTime;

    // ── Phase 1: spawn arrivals ───────────────────────────────────────────────
    for (int i = 0; i < TOTAL_PATIENTS; i++) {
        Patient& p = patients[i];
        if (p.state == PENDING && simClock >= p.arrival_sim_time) {
            p.state   = WALKING_IN;
            p.px      = -60.0f;
            p.py      = ENTRY_Y;
            p.targetX = ENTRY_X;
            p.targetY = ENTRY_Y;
            spawnBurst(p.px, p.py, p.color, 8);
        }
    }

    // ── Phase 2: move toward target ───────────────────────────────────────────
    float step = ANIM_SPEED * 90.0f * dt;
    for (auto& p : patients) {
        if (p.state == PENDING || p.animDone) continue;

        float dx   = p.targetX - p.px;
        float dy   = p.targetY - p.py;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 1.5f) {
            p.px += (dx / dist) * step;
            p.py += (dy / dist) * step;
        } else {
            p.px = p.targetX;
            p.py = p.targetY;
        }
    }

    // ── Phase 3: state transitions ────────────────────────────────────────────
    for (auto& p : patients) {
        float dx      = p.targetX - p.px;
        float dy      = p.targetY - p.py;
        float dist    = sqrtf(dx * dx + dy * dy);
        bool  atTarget = (dist < 2.0f);

        // WALKING_IN → IN_QUEUE
        if (p.state == WALKING_IN && atTarget) {
            p.state = IN_QUEUE;
            // FIX 3: reassign all queue slots so this patient gets the correct
            // next free slot without duplicating a slot held by another patient.
            reassignQueueSlots();
            spawnBurst(p.px, p.py, p.color, 10);
        }

        // IN_QUEUE → WALKING_TO_DOCTOR
        // FIX 4: patient must have reached their queue target AND be at slot 0
        if (p.state == IN_QUEUE && atTarget && simClock >= p.service_start) {
            bool doctorBusy = false;
            for (auto& q : patients)
                if (q.state == BEING_SERVED || q.state == WALKING_TO_DOCTOR)
                    { doctorBusy = true; break; }

            // Confirm this patient is actually at slot-0 position
            bool atFront = (fabsf(p.targetX - queueSlotX(0)) < 2.0f);

            if (!doctorBusy && atFront) {
                p.state           = WALKING_TO_DOCTOR;
                p.targetX         = DOCTOR_X;
                p.targetY         = DOCTOR_Y;
                p.serviceProgress = 0.0f;
                reassignQueueSlots();   // shift remaining queue forward
                spawnBurst(p.px, p.py, p.color, 6);
            }
        }

        // WALKING_TO_DOCTOR → BEING_SERVED
        if (p.state == WALKING_TO_DOCTOR && atTarget) {
            p.state           = BEING_SERVED;
            p.serviceProgress = 0.0f;
            spawnBurst(DOCTOR_X, DOCTOR_Y, GREEN, 20);
        }

        // BEING_SERVED → DONE
        if (p.state == BEING_SERVED) {
            // FIX 5: cast to float before subtraction to preserve precision
            float duration = (float)(p.finish_time) - (float)(p.service_start);
            if (duration < 1.0f) duration = 1.0f;
            p.serviceProgress += dt * SIM_SPEED * 10.0f / duration;
            if (p.serviceProgress >= 1.0f) {
                p.serviceProgress = 1.0f;
                p.state   = DONE;
                p.targetX = (float)W + 100.0f;
                p.targetY = DOCTOR_Y;
                spawnBurst(DOCTOR_X, DOCTOR_Y, ACCENT, 30);
            }
        }

        // DONE → mark finished once off-screen
        if (p.state == DONE && p.px >= (float)W + 60.0f) {
            p.animDone = true;
        }
    }

    // ── Phase 4: check completion ─────────────────────────────────────────────
    bool allDone = true;
    for (auto& p : patients)
        if (!p.animDone) { allDone = false; break; }
    if (allDone) simFinished = true;
}

// ─── Drawing helpers ──────────────────────────────────────────────────────────
static void drawGlowCircle(float cx, float cy, float rad, Col c, float intensity = 0.4f) {
    for (int g = 4; g >= 0; g--) {
        float r2 = rad + g * 5.0f;
        float a  = intensity * (1.0f - g / 5.0f) * 0.25f;
        glCol(c, a);
        drawCircle(cx, cy, r2, true, 32);
    }
    glCol(c, 1.0f);
    drawCircle(cx, cy, rad, true, 32);
}

static void drawPatientFigure(float cx, float cy, Col c, float sc = 1.0f, float alpha = 1.0f) {
    float headR    = 10.0f * sc;
    float bodyH    = 20.0f * sc;
    float bodyW    = 12.0f * sc;
    float legH     = 10.0f * sc;
    float bodyBottom = cy + legH;
    float bodyTop    = bodyBottom + bodyH;
    float headCY     = bodyTop + headR;

    glCol({0,0,0,0.0f}, 0.18f * alpha);
    drawCircle(cx + 2, cy - 2, headR * 0.9f, true, 12);

    glCol(c, alpha * 0.7f);
    fillRoundRect(cx - bodyW / 2,        cy,           bodyW * 0.38f, legH, 2.0f * sc);
    fillRoundRect(cx + bodyW * 0.12f,    cy,           bodyW * 0.38f, legH, 2.0f * sc);

    glCol(c, alpha * 0.85f);
    fillRoundRect(cx - bodyW / 2, bodyBottom, bodyW, bodyH, 4.0f * sc);

    glCol(c, alpha * 0.18f);
    drawCircle(cx, headCY, headR + 5.0f * sc);

    glCol(c, alpha);
    drawCircle(cx, headCY, headR);

    glCol(WHITE, alpha * 0.4f);
    drawCircle(cx - headR * 0.3f, headCY + headR * 0.3f, headR * 0.28f);
}

static void drawDoctor(float cx, float cy) {
    float pulse = 0.5f + 0.5f * sinf(globalTime * 3.0f);

    glCol(ACCENT, 0.1f + 0.08f * pulse);
    drawCircle(cx, cy, 46.0f);
    glCol(ACCENT, 0.05f);
    drawCircle(cx, cy, 56.0f);

    glCol({0.08f, 0.16f, 0.22f, 1.0f});
    drawCircle(cx, cy, 36.0f);

    glCol(ACCENT, 0.8f + 0.2f * pulse);
    drawCircle(cx, cy, 36.0f, false, 48);

    float cw = 8.0f, ch = 22.0f;
    glCol(GREEN, 0.9f);
    fillRoundRect(cx - cw / 2, cy - ch / 2, cw, ch, 3.0f);
    fillRoundRect(cx - ch / 2, cy - cw / 2, ch, cw, 3.0f);

    glCol(MUTED);
    drawText(cx - 24, cy - 52.0f, "DOCTOR", GLUT_BITMAP_8_BY_13);
}

static void drawQueueLane() {
    float floorY = QUEUE_Y - 2.0f;
    glCol(MUTED, 0.18f);
    glLineWidth(2.0f);
    drawLine(QUEUE_X_START - 30, floorY, DOCTOR_X - 50, floorY);

    // FIX 7: replaced glLineStipple (deprecated in core GL) with
    // short manual dashes drawn as individual GL_LINES segments
    float guideY = QUEUE_Y + 30.0f;
    glCol(MUTED, 0.12f);
    glLineWidth(1.0f);
    {
        float dashLen = 8.0f, gapLen = 6.0f;
        float x = QUEUE_X_START - 20;
        float xEnd = DOCTOR_X - 50;
        glBegin(GL_LINES);
        while (x < xEnd) {
            float x2 = std::min(x + dashLen, xEnd);
            glVertex2f(x,  guideY);
            glVertex2f(x2, guideY);
            x += dashLen + gapLen;
        }
        glEnd();
    }

    for (int i = 0; i < 6; i++) {
        float sx = queueSlotX(i);
        glCol(MUTED, 0.08f);
        glPushMatrix();
        glTranslatef(sx, QUEUE_Y, 0);
        glScalef(1.0f, 0.35f, 1.0f);
        drawCircle(0, 0, 16.0f, true, 16);
        glPopMatrix();
        glCol(MUTED, 0.22f);
        glPushMatrix();
        glTranslatef(sx, QUEUE_Y, 0);
        glScalef(1.0f, 0.35f, 1.0f);
        drawCircle(0, 0, 16.0f, false, 16);
        glPopMatrix();
    }

    glCol(WARN, 0.4f);
    drawLine(ENTRY_X + 28, QUEUE_Y, QUEUE_X_START - 24, QUEUE_Y, 1.5f);
    glBegin(GL_TRIANGLES);
    glVertex2f(QUEUE_X_START - 16, QUEUE_Y);
    glVertex2f(QUEUE_X_START - 28, QUEUE_Y - 6);
    glVertex2f(QUEUE_X_START - 28, QUEUE_Y + 6);
    glEnd();

    glCol(ACCENT, 0.3f);
    drawLine(queueSlotX(5) + 24, QUEUE_Y, DOCTOR_X - 50, QUEUE_Y, 1.5f);
    glBegin(GL_TRIANGLES);
    glVertex2f(DOCTOR_X - 42, QUEUE_Y);
    glVertex2f(DOCTOR_X - 54, QUEUE_Y - 6);
    glVertex2f(DOCTOR_X - 54, QUEUE_Y + 6);
    glEnd();

    glCol(MUTED, 0.45f);
    drawText(QUEUE_X_START - 10, QUEUE_Y - 28, "WAITING QUEUE", GLUT_BITMAP_8_BY_13);
}

static void drawEntry() {
    float pulse = 0.5f + 0.5f * sinf(globalTime * 2.5f);

    glCol(WARN, 0.10f + 0.06f * pulse);
    drawCircle(ENTRY_X, ENTRY_Y, 30.0f);
    glCol(WARN, 0.6f + 0.3f * pulse);
    drawCircle(ENTRY_X, ENTRY_Y, 22.0f, false, 24);

    float ew = 5.0f, eh = 14.0f;
    glCol(WARN, 0.8f);
    fillRoundRect(ENTRY_X - ew / 2, ENTRY_Y - eh / 2, ew, eh, 2.0f);
    fillRoundRect(ENTRY_X - eh / 2, ENTRY_Y - ew / 2, eh, ew, 2.0f);

    glCol(WARN, 0.7f);
    drawText(ENTRY_X - 20, ENTRY_Y - 38.0f, "ENTRY", GLUT_BITMAP_8_BY_13);
}

static void drawBackground() {
    glCol(BG);
    fillRect(0, 0, W, H);

    glCol(MUTED, 0.04f);
    glLineWidth(1.0f);
    int gridSz = 40;
    for (int x = 0; x < W; x += gridSz) drawLine((float)x, 0, (float)x, (float)H);
    for (int y = 0; y < H; y += gridSz) drawLine(0, (float)y, (float)W, (float)y);

    glCol(ACCENT, 0.07f);
    fillRect(0, H - 4, W, 4);
}

static void drawTopBar() {
    glCol(PANEL);
    fillRect(0, H - 60, W, 60);
    glCol(ACCENT, 0.15f);
    drawLine(0, H - 60, W, H - 60, 1.0f);

    glCol(ACCENT);
    drawText(24, H - 36, "M/M/1 QUEUE SIMULATION", GLUT_BITMAP_HELVETICA_18);
    glCol(MUTED);
    drawText(24, H - 18, "CLINIC PATIENT FLOW  |  KENDALL NOTATION", GLUT_BITMAP_8_BY_13);

    std::string clkStr = "SIM CLOCK: " + std::to_string(simClock) + "s";
    glCol(GREEN);
    drawText(W - 200, H - 36, clkStr, GLUT_BITMAP_HELVETICA_18);

    std::string status = simFinished ? "COMPLETE" : (simRunning ? "RUNNING" : "PRESS [SPACE]");
    Col sc = simFinished ? GREEN : (simRunning ? ACCENT : WARN);
    float bp = 0.6f + 0.4f * sinf(globalTime * 4.0f);
    glCol(sc, simRunning && !simFinished ? bp : 1.0f);
    drawText(W - 200, H - 18, status, GLUT_BITMAP_8_BY_13);
}

static void drawFormulaPanel() {
    float px = 20, py = 20, pw = 200, ph = 260;
    glCol(PANEL);
    fillRoundRect(px, py, pw, ph, 8.0f);
    glCol(ACCENT, 0.3f);
    strokeRect(px, py, pw, ph, 1.0f);
    glCol(ACCENT, 0.7f);
    fillRect(px, py + ph - 4, pw, 4);

    glCol(ACCENT);
    drawText(px + 12, py + ph - 18, "FORMULA RESULTS", GLUT_BITMAP_8_BY_13);
    glCol(ACCENT, 0.3f);
    drawLine(px + 10, py + ph - 26, px + pw - 10, py + ph - 26, 1.0f);

    // FIX 10: show warning if queue is unstable
    if (rho >= 1.0f) {
        glCol(WARN);
        drawText(px + 12, py + ph - 46, "UNSTABLE (rho>=1)", GLUT_BITMAP_8_BY_13);
        return;
    }

    struct Row { std::string label; float val; std::string unit; };
    std::vector<Row> rows = {
        { "rho  traffic:",  rho, ""    },
        { "Lq   in queue:", Lq,  "pts" },
        { "L    in system:",L,   "pts" },
        { "Wq   wait time:",Wq,  "s"   },
        { "W    sys time:", Wt,  "s"   },
        { "P0   idle prob:",P0,  "%"   },
    };

    float ry = py + ph - 46;
    for (auto& row : rows) {
        glCol(MUTED);
        drawText(px + 12, ry, row.label, GLUT_BITMAP_8_BY_13);
        glCol(WHITE);
        drawText(px + 148, ry, fmt(row.val) + row.unit, GLUT_BITMAP_8_BY_13);
        ry -= 30;
    }
}

static void drawPatientLog() {
    float px = W - 220, py = 20, pw = 200, ph = (float)H - 110;
    glCol(PANEL);
    fillRoundRect(px, py, pw, ph, 8.0f);
    glCol(ACCENT, 0.2f);
    strokeRect(px, py, pw, ph, 1.0f);
    glCol(ACCENT, 0.7f);
    fillRect(px, py + ph - 4, pw, 4);

    glCol(ACCENT);
    drawText(px + 12, py + ph - 18, "PATIENT LOG", GLUT_BITMAP_8_BY_13);
    glCol(ACCENT, 0.3f);
    drawLine(px + 10, py + ph - 26, px + pw - 10, py + ph - 26, 1.0f);

    float ry = py + ph - 46;
    for (auto& p : patients) {
        if (p.state == PENDING) continue;

        glCol(p.color);
        drawCircle(px + 18, ry + 4, 5.0f);

        glCol(WHITE);
        drawText(px + 30, ry, "P" + std::to_string(p.id), GLUT_BITMAP_8_BY_13);

        std::string st;
        Col sc = MUTED;
        switch (p.state) {
            case WALKING_IN:        st = "ARRIVING"; sc = WARN;   break;
            case IN_QUEUE:          st = "WAITING";  sc = WARN;   break;
            case WALKING_TO_DOCTOR: st = "GOING IN"; sc = ACCENT; break;
            case BEING_SERVED:      st = "SERVED";   sc = GREEN;  break;
            case DONE:              st = "DONE";     sc = MUTED;  break;
            default:                st = "";                      break;
        }
        glCol(sc);
        drawText(px + 60, ry, st, GLUT_BITMAP_8_BY_13);

        if (p.state == BEING_SERVED) {
            float bw = 130, bh = 4;
            float bx = px + 14, by = ry - 12;
            glCol(DARK);
            fillRect(bx, by, bw, bh);
            glCol(GREEN, 0.8f);
            fillRect(bx, by, bw * p.serviceProgress, bh);
        }

        if (p.state == DONE || p.animDone) {
            glCol(MUTED);
            drawText(px + 130, ry, "wt:" + std::to_string(p.waiting_time) + "s",
                     GLUT_BITMAP_8_BY_13);
        }

        ry -= 32;
        if (ry < py + 10) break;
    }
}

static void drawSimArea() {
    float areaX = 230, areaY = 140;
    float areaW = W - 460, areaH = H - 220;

    glCol(DARK);
    fillRoundRect(areaX, areaY, areaW, areaH, 10.0f);
    glCol(ACCENT, 0.1f);
    strokeRect(areaX, areaY, areaW, areaH, 1.0f);

    drawQueueLane();
    drawEntry();
    drawDoctor(DOCTOR_X, DOCTOR_Y);

    for (auto& p : patients) {
        if (p.state == PENDING || p.animDone) continue;
        float sc = p.scale;
        if (p.state == BEING_SERVED) {
            float pulse = 0.95f + 0.05f * sinf(globalTime * 5.0f);
            sc *= pulse;
        }
        drawPatientFigure(p.px, p.py, p.color, sc, p.alpha);

        float figH = (10.0f + 20.0f + 20.0f) * sc;
        glCol(p.color, p.alpha);
        drawText(p.px - 6, p.py + figH + 4.0f,
                 "P" + std::to_string(p.id), GLUT_BITMAP_8_BY_13);

        if (p.state == IN_QUEUE && p.waiting_time > 0) {
            glCol(WARN, 0.9f);
            drawText(p.px - 10, p.py - 16,
                     "wt:" + std::to_string(p.waiting_time) + "s", GLUT_BITMAP_8_BY_13);
        }
    }

    for (auto& pt : particles) {
        float t = pt.life / pt.maxLife;
        glCol(pt.color, t * 0.85f);
        drawCircle(pt.x, pt.y, pt.size * t, true, 8);
    }

    if (!simRunning && !simFinished) {
        float pulse = 0.4f + 0.6f * fabsf(sinf(globalTime * 2.0f));
        glCol(ACCENT, pulse);
        drawText(areaX + areaW / 2 - 80, areaY + areaH / 2,
                 "PRESS [SPACE] TO START", GLUT_BITMAP_HELVETICA_18);
    }

    if (simFinished) {
        float pulse = 0.7f + 0.3f * sinf(globalTime * 3.0f);
        glCol(GREEN, pulse);
        drawText(areaX + areaW / 2 - 80, areaY + areaH / 2,
                 "SIMULATION COMPLETE", GLUT_BITMAP_HELVETICA_18);
        glCol(MUTED);
        drawText(areaX + areaW / 2 - 60, areaY + areaH / 2 - 24,
                 "PRESS [R] TO RESTART", GLUT_BITMAP_8_BY_13);
    }
}

// ─── GLUT callbacks ───────────────────────────────────────────────────────────
static void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawBackground();
    drawTopBar();
    drawFormulaPanel();
    drawSimArea();
    drawPatientLog();

    glutSwapBuffers();
}

static void reshape(int w, int h) {
    W = w; H = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);   // Y-up: (0,0) = bottom-left
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    ENTRY_X       = w * 0.22f;
    ENTRY_Y       = h * 0.48f;
    QUEUE_X_START = w * 0.38f;
    QUEUE_Y       = h * 0.48f;
    DOCTOR_X      = w * 0.72f;
    DOCTOR_Y      = h * 0.48f;
    SLOT_W        = 52.0f;
}

static void timer(int) {
    float now = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float dt  = now - lastFrameTime;
    if (dt > 0.1f) dt = 0.1f;
    lastFrameTime = now;

    update(dt);
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

static void keyboard(unsigned char key, int, int) {
    switch (key) {
        case ' ':
            if (!simFinished) simRunning = !simRunning;
            break;
        case 'r': case 'R':
            initSim();
            simRunning = false;
            break;
        case 27:
            exit(0);
    }
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    computeFormulas();
    initSim();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(W, H);
    glutInitWindowPosition(100, 80);
    glutCreateWindow("M/M/1 Queue Simulation — Clinic");

    glClearColor(BG.r, BG.g, BG.b, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    lastFrameTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}