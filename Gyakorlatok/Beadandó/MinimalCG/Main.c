// fájl: main.c
#include <GL/glut.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <windows.h>
#include <stdio.h>

// gömbök száma (2 vagy 3)
int numSpheres = 2;

// gömbök adatai
typedef struct {
    float pos[3];
    float vel[3];
    float radius;
    float color[4];
} Sphere;

Sphere spheres[3];

// világítás
GLfloat lightPos[] = { 5.0f, 8.0f, 5.0f, 1.0f };
GLfloat lightAmbient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
GLfloat lightDiffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
bool   lightingOn = true;

// köd
bool fogOn = false;

// --- Free camera globals ---
float cameraYaw = -90.0f;
float cameraPitch = 0.0f;
float lastX = 400, lastY = 300;
bool  firstMouse = true;

bool paused = false;
// Globálisak a fájl tetején
float fogDensityWithLight = 0.05f;
float fogDensityWithoutLight = 0.02f;

// idő a nap–hold ciklushoz (0…1)
float timeOfDay = 0.0f;
// nap–hold körpálya sugarának nagysága
const float celestialRadius = 80.0f;
// mennyi másodperc alatt jár körbe (pl. 60 mp = 1 perc = 1 nap)
const float dayLength = 300.0f;
float timeOfDaySpeed = 0.5f;


typedef struct {
    float pos[3];
    float front[3];
    float up[3];
    float right[3];
    float worldUp[3];
    float speed;
    float sensitivity;
} Camera;

Camera cam;
bool keys[256] = { 0 };

// ablak mérete (reshape-ben frissül)
int windowWidth = 800;
int windowHeight = 600;

// warp-esemény ignorálásához
bool ignoreNextWarp = false;

float groundExtentSmall = 50.0f;
float groundExtentLarge = 100.0f;
float groundExtent = 50.0f;  // aktuális, initben is használod
bool  groundIsLarge = false;

float sphereSpeedScale = 1.0f;
const float sphereSpeedMin = 0.1f;
const float sphereSpeedMax = 5.0f;
const float sphereSpeedStep = 0.1f;


float wallHeight = 10.0f;

// időlépés
const float dt = 0.01f;

// talaj sík: y = 0
const float groundY = 0.0f;





// világítás, nézet beállítása
void initLighting() {
    if (!lightingOn) {
        // Ha a világítás ki van kapcsolva, tiltsuk le a GL_LIGHTING és a fényforrásokat
        glDisable(GL_LIGHTING);
        glDisable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        return;
    }

    // Kapcsoljuk be a globális világítást és a két fényforrást
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    // Általános fényparaméterek beállítása mindkét fényre
    GLfloat ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat specular[] = { 0.8f, 0.8f, 0.8f, 1.0f };

    // Napfény erős, meleg árnyalata (GL_LIGHT0)
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    // Holdfény halk, hideg árnyalata (GL_LIGHT1)
    glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, specular);

    // Anyagkövetés bekapcsolása: glColor4fv-ből örökölnek a tárgyak
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}
// köd beállítása
void initFog() {
    if (!fogOn) {
        // Ha ki van kapcsolva a köd, akkor tiltsuk le
        glDisable(GL_FOG);
        return;
    }

    // Kapcsoljuk be a ködöt
    glEnable(GL_FOG);

    // Egységes szürke köd szín
    GLfloat fogColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
    glFogfv(GL_FOG_COLOR, fogColor);

    // Exponenciális köd másodfokú változata
    glFogi(GL_FOG_MODE, GL_EXP2);

    // Density beállítása a globális változók alapján
    if (lightingOn) {
        // ha világítás is van, használjuk ezt az értéket
        glFogf(GL_FOG_DENSITY, fogDensityWithLight);
    }
    else {
        // csak köd (nincs világítás)
        glFogf(GL_FOG_DENSITY, fogDensityWithoutLight);
    }
}



// egérmozgás callback
void mouse_callback(int x, int y) {
    // Ha pause-ban vagyunk, semmi ne történjen
    if (paused) {
        return;
    }

    // Ha ez egy belső warp-pointer esemény, ignoráljuk
    if (ignoreNextWarp) {
        ignoreNextWarp = false;
        lastX = x;
        lastY = y;
        return;
    }

    // Első valós egérmozduláskor állítsuk be a középpontot
    if (firstMouse) {
        firstMouse = false;
        lastX = windowWidth / 2;
        lastY = windowHeight / 2;
    }

    // Offset számítása
    float xoffset = (x - lastX) * cam.sensitivity;
    float yoffset = (lastY - y) * cam.sensitivity;
    lastX = x;
    lastY = y;

    // Yaw / Pitch frissítése és korlátozás
    cameraYaw += xoffset;
    cameraPitch += yoffset;
    if (cameraPitch > 89.0f) cameraPitch = 89.0f;
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;

    // Új front vektor számítása
    float radYaw = cameraYaw * (float)M_PI / 180.0f;
    float radPitch = cameraPitch * (float)M_PI / 180.0f;
    cam.front[0] = cosf(radYaw) * cosf(radPitch);
    cam.front[1] = sinf(radPitch);
    cam.front[2] = sinf(radYaw) * cosf(radPitch);
    // Normalizálás
    float len = sqrtf(
        cam.front[0] * cam.front[0] +
        cam.front[1] * cam.front[1] +
        cam.front[2] * cam.front[2]
    );
    for (int i = 0; i < 3; i++) cam.front[i] /= len;

    // right = front × worldUp
    float rx = cam.front[1] * cam.worldUp[2] - cam.front[2] * cam.worldUp[1];
    float ry = cam.front[2] * cam.worldUp[0] - cam.front[0] * cam.worldUp[2];
    float rz = cam.front[0] * cam.worldUp[1] - cam.front[1] * cam.worldUp[0];
    len = sqrtf(rx * rx + ry * ry + rz * rz);
    cam.right[0] = rx / len; cam.right[1] = ry / len; cam.right[2] = rz / len;

    // up = right × front
    float ux = cam.right[1] * cam.front[2] - cam.right[2] * cam.front[1];
    float uy = cam.right[2] * cam.front[0] - cam.right[0] * cam.front[2];
    float uz = cam.right[0] * cam.front[1] - cam.right[1] * cam.front[0];
    len = sqrtf(ux * ux + uy * uy + uz * uz);
    cam.up[0] = ux / len; cam.up[1] = uy / len; cam.up[2] = uz / len;

    // Vissza warp-pointer az ablak közepére
    ignoreNextWarp = true;
    glutWarpPointer(windowWidth / 2, windowHeight / 2);
    lastX = windowWidth / 2;
    lastY = windowHeight / 2;
}

void renderText(float x, float y, const char* s) {
    glRasterPos2f(x, y);
    const unsigned char* us = (const unsigned char*)s;
    while (*us) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *us++);
    }
}


// billentyű lenyomás
void keyboard_down(unsigned char key, int x, int y) {
    keys[key] = true;
    if (key == 27)     // ESC
        exit(0);

    switch (key) {
    case 'p': case 'P':
        paused = !paused;
        if (paused) {
            // belépés pause módba
            glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
        }
        else {
            // kilépés pause-ból
            firstMouse = true;
            ignoreNextWarp = true;
            glutWarpPointer(windowWidth / 2, windowHeight / 2);
            glutSetCursor(GLUT_CURSOR_NONE);
        }
        break;

    case 'f': case 'F':
        fogOn = !fogOn;
        break;

    case 'l': case 'L':
        lightingOn = !lightingOn;
        break;

    case 'n': case 'N':
        numSpheres = (numSpheres == 2 ? 3 : 2);
        break;
        
    }
}




// billentyű felengedés
void keyboard_up(unsigned char key, int x, int y) {
    keys[key] = false;
}

// mozgatja a kamerát a lenyomott billentyűk alapján
void do_movement() {
    float velocity = cam.speed * dt;
    if (keys['w'] || keys['W'])
        for (int i = 0;i < 3;i++) cam.pos[i] += cam.front[i] * velocity;
    if (keys['s'] || keys['S'])
        for (int i = 0;i < 3;i++) cam.pos[i] -= cam.front[i] * velocity;
    if (keys['a'] || keys['A'])
        for (int i = 0;i < 3;i++) cam.pos[i] -= cam.right[i] * velocity;
    if (keys['d'] || keys['D'])
        for (int i = 0;i < 3;i++) cam.pos[i] += cam.right[i] * velocity;
    if (keys[' '])            // Space
        cam.pos[1] += velocity;
    if (keys[VK_CONTROL])     // Ctrl
        cam.pos[1] -= velocity;
}




// gömbök inicializálása
void initSpheres() {
    for (int i = 0;i < 3;i++) {
        spheres[i].radius = 1.0f;
        spheres[i].pos[0] = -2.0f + 2.0f * i;
        spheres[i].pos[1] = 5.0f + i * 2.0f;
        spheres[i].pos[2] = 0.0f;
        spheres[i].vel[0] = (i % 2 ? -1.0f : 1.2f);
        spheres[i].vel[1] = 0.0f;
        spheres[i].vel[2] = (i % 3 ? 0.8f : -1.0f);
        spheres[i].color[0] = (i == 0 ? 1 : 0);
        spheres[i].color[1] = (i == 1 ? 1 : 0);
        spheres[i].color[2] = (i == 2 ? 1 : 0);
        spheres[i].color[3] = 1.0f;
    }
}

void initCamera() {
    cam.pos[0] = 0;   cam.pos[1] = 2;  cam.pos[2] = 12;
    cam.front[0] = 0; cam.front[1] = 0; cam.front[2] = -1;
    cam.worldUp[0] = 0; cam.worldUp[1] = 1; cam.worldUp[2] = 0;
    cam.speed = 5.0f;
    cam.sensitivity = 0.1f;
    // right = front × worldUp
    float rx = cam.front[1] * cam.worldUp[2]
        - cam.front[2] * cam.worldUp[1];
    float ry = cam.front[2] * cam.worldUp[0]
        - cam.front[0] * cam.worldUp[2];
    float rz = cam.front[0] * cam.worldUp[1]
        - cam.front[1] * cam.worldUp[0];
    float len = sqrtf(rx * rx + ry * ry + rz * rz);
    cam.right[0] = rx / len; cam.right[1] = ry / len; cam.right[2] = rz / len;
    memcpy(cam.up, cam.worldUp, sizeof(cam.up));
}

// sík talaj kirajzolása
void drawGround() {
    float e = groundExtent;        // innen veszi a méretet
    glDisable(GL_LIGHTING);
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_QUADS);
    glVertex3f(-e, groundY, -e);
    glVertex3f(-e, groundY, e);
    glVertex3f(e, groundY, e);
    glVertex3f(e, groundY, -e);
    glEnd();
    if (lightingOn) glEnable(GL_LIGHTING);
}

void drawWalls() {
    float e = groundExtent;
    float h = wallHeight;
    glDisable(GL_LIGHTING);
    glColor3f(0.3f, 0.3f, 0.3f);  // fal színe (sötétebb szürke)
    glBegin(GL_QUADS);
    // északi fal (z = +e)
    glVertex3f(-e, groundY, e);
    glVertex3f(e, groundY, e);
    glVertex3f(e, groundY + h, e);
    glVertex3f(-e, groundY + h, e);
    // déli fal (z = -e)
    glVertex3f(-e, groundY, -e);
    glVertex3f(-e, groundY + h, -e);
    glVertex3f(e, groundY + h, -e);
    glVertex3f(e, groundY, -e);
    // nyugati fal (x = -e)
    glVertex3f(-e, groundY, -e);
    glVertex3f(-e, groundY, e);
    glVertex3f(-e, groundY + h, e);
    glVertex3f(-e, groundY + h, -e);
    // keleti fal (x = +e)
    glVertex3f(e, groundY, -e);
    glVertex3f(e, groundY + h, -e);
    glVertex3f(e, groundY + h, e);
    glVertex3f(e, groundY, e);
    glEnd();
    if (lightingOn) glEnable(GL_LIGHTING);
}




// árnyék vetítési mátrix síkra
void shadowMatrix(const GLfloat ground[4], const GLfloat light[4], GLfloat mat[16]) {
    float dot = ground[0] * light[0] + ground[1] * light[1] + ground[2] * light[2] + ground[3] * light[3];
    for (int i = 0;i < 4;i++) {
        for (int j = 0;j < 4;j++) {
            mat[j * 4 + i] = dot * (i == j) - light[i] * ground[j];
        }
    }
}

// Equal–mass rugalmas ütközés két gömb között
void handleSphereCollision(Sphere* s1, Sphere* s2) {
    // 1) Vektor a két gömb közepe között
    float dx = s1->pos[0] - s2->pos[0];
    float dy = s1->pos[1] - s2->pos[1];
    float dz = s1->pos[2] - s2->pos[2];
    float dist2 = dx * dx + dy * dy + dz * dz;
    float rSum = s1->radius + s2->radius;

    // 2) Ha nincs átfedés, kilépünk
    if (dist2 >= rSum * rSum) return;

    float dist = sqrtf(dist2);
    // Elkerüljük a 0-val való osztást
    if (dist == 0.0f) {
        // kissé eltoljuk az egyik gömböt
        dx = rSum; dy = dz = 0.0f;
        dist = rSum;
    }

    // 3) Gömbök szétcsúsztatása az átfedés felét eltolva
    float penetration = 0.5f * (rSum - dist);
    float nx = dx / dist;
    float ny = dy / dist;
    float nz = dz / dist;
    s1->pos[0] += nx * penetration;
    s1->pos[1] += ny * penetration;
    s1->pos[2] += nz * penetration;
    s2->pos[0] -= nx * penetration;
    s2->pos[1] -= ny * penetration;
    s2->pos[2] -= nz * penetration;

    // 4) Relatív sebesség a normál mentén
    float vxRel = s1->vel[0] - s2->vel[0];
    float vyRel = s1->vel[1] - s2->vel[1];
    float vzRel = s1->vel[2] - s2->vel[2];
    float vDotN = vxRel * nx + vyRel * ny + vzRel * nz;

    // Ha már távolodnak, nem kell visszapattanni
    if (vDotN >= 0.0f) return;

    // 5) Impulzus számítása (egyenletes tömeg esetén egyszerűsítve)
    float j = -2.0f * vDotN / 2.0f;  // m1=m2, így / (m1+m2) = /2
    float ix = j * nx;
    float iy = j * ny;
    float iz = j * nz;

    // 6) Sebességek módosítása
    s1->vel[0] += ix;
    s1->vel[1] += iy;
    s1->vel[2] += iz;
    s2->vel[0] -= ix;
    s2->vel[1] -= iy;
    s2->vel[2] -= iz;
}


// kirajzolás
void display() {
    // 1) Háttér törlése
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2) Kamera nézet beállítása
    glLoadIdentity();
    float centerX = cam.pos[0] + cam.front[0];
    float centerY = cam.pos[1] + cam.front[1];
    float centerZ = cam.pos[2] + cam.front[2];
    gluLookAt(
        cam.pos[0], cam.pos[1], cam.pos[2],
        centerX, centerY, centerZ,
        cam.up[0], cam.up[1], cam.up[2]
    );

    // 3) Nap–hold ciklus: pozíciók és fények
    float angle = timeOfDay * 2.0f * M_PI;
    float sunX = sinf(angle) * celestialRadius;
    float sunY = cosf(angle) * celestialRadius;
    float moonX = -sunX;
    float moonY = -sunY;

    // beállítjuk GL_LIGHT0-t (nap) és GL_LIGHT1-et (hold)
    GLfloat sunPos[4] = { sunX, sunY, 0.0f, 0.0f };
    GLfloat sunColor[4] = { 1.0f, 0.95f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, sunPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunColor);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunColor);

    GLfloat moonPos[4] = { moonX, moonY, 0.0f, 0.0f };
    GLfloat moonColor[4] = { 0.6f, 0.6f, 0.7f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, moonPos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, moonColor);
    glLightfv(GL_LIGHT1, GL_SPECULAR, moonColor);

    // 4) Árnyékok a talajon (síkvetítés – csak ha felette vannak)
    {
        GLfloat groundPlane[4] = { 0.0f, 1.0f, 0.0f, -groundY };
        GLfloat shadowMat[16];
        // Kiszámoljuk a vetítési mátrixot a nap fényforrására
        shadowMatrix(groundPlane, sunPos, shadowMat);

        glDisable(GL_LIGHTING);

        // -- Gömbök árnyéka --
        glPushMatrix();
        glMultMatrixf(shadowMat);
        for (int i = 0; i < numSpheres; i++) {
            if (spheres[i].pos[1] > groundY) {
                glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
                glPushMatrix();
                glTranslatef(
                    spheres[i].pos[0],
                    spheres[i].pos[1],
                    spheres[i].pos[2]
                );
                glutSolidSphere(spheres[i].radius, 20, 20);
                glPopMatrix();
            }
        }
        glPopMatrix();

        // -- Nap árnyéka --
        if (sunY > groundY) {
            glPushMatrix();
            glMultMatrixf(shadowMat);
            glTranslatef(sunX, sunY, 0.0f);
            glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
            glutSolidSphere(5.0f, 20, 20);
            glPopMatrix();
        }

        // -- Hold árnyéka --
        if (moonY > groundY) {
            glPushMatrix();
            glMultMatrixf(shadowMat);
            glTranslatef(moonX, moonY, 0.0f);
            glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
            glutSolidSphere(4.0f, 20, 20);
            glPopMatrix();
        }

        if (lightingOn) glEnable(GL_LIGHTING);
    }

    // 5) Égitestek kirajzolása (mindig háttérben)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // Nap
    glPushMatrix();
    glTranslatef(sunX, sunY, 0.0f);
    glColor3f(1.0f, 0.9f, 0.2f);
    glutSolidSphere(5.0f, 20, 20);
    glPopMatrix();

    // Hold
    glPushMatrix();
    glTranslatef(moonX, moonY, 0.0f);
    glColor3f(0.7f, 0.7f, 0.8f);
    glutSolidSphere(4.0f, 20, 20);
    glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    if (lightingOn) glEnable(GL_LIGHTING);

    // 6) Világítás és köd a többi tárgyhoz
    initLighting();
    initFog();

    // 7) Talaj és falak
    drawGround();
    drawWalls();

    // 8) Árnyékok és gömbök (a nap–hold előtti gömb-árnyékokat az előző blokkba tettük)
    {
        GLfloat groundPlane[4] = { 0,1,0,-groundY };
        GLfloat mat[16];
        shadowMatrix(groundPlane, sunPos, mat);

        glDisable(GL_LIGHTING);
        glPushMatrix();
        glMultMatrixf(mat);
        for (int i = 0; i < numSpheres; i++) {
            glColor4f(0, 0, 0, 0.5f);
            glPushMatrix();
            glTranslatef(
                spheres[i].pos[0],
                spheres[i].pos[1],
                spheres[i].pos[2]
            );
            glutSolidSphere(spheres[i].radius, 20, 20);
            glPopMatrix();
        }
        glPopMatrix();
        if (lightingOn) glEnable(GL_LIGHTING);
    }

    // 9) Gömbök kirajzolása
    for (int i = 0; i < numSpheres; i++) {
        glColor4fv(spheres[i].color);
        glPushMatrix();
        glTranslatef(
            spheres[i].pos[0],
            spheres[i].pos[1],
            spheres[i].pos[2]
        );
        glutSolidSphere(spheres[i].radius, 32, 32);
        glPopMatrix();
    }

    // 10) PAUSE overlay és gombok
    if (paused) {
        // Depth-test és világítás kikapcsolása az overlay-hez
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);

        // Ortho 2D projekció beállítása
        GLint vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, vp[2], 0, vp[3]);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix(); glLoadIdentity();

        // Gomb- és szöveg-paraméterek
        const int BW = 100;
        const int BH = 30;
        const int MX = 20;
        const int MY = 20;
        const int SP = 15;
        const int PAD = 8;   // szöveg belső alsó margó
        const int VAL_OFF = 20;  // változó-szám kiírás Y-eltolása

        int bx = MX;
        int by = vp[3] - MY - BH;

        glColor3f(0.3f, 0.3f, 0.3f);
        // 1) Toggle Ground
        glBegin(GL_QUADS);
        glVertex2i(bx, by);
        glVertex2i(bx + BW, by);
        glVertex2i(bx + BW, by + BH);
        glVertex2i(bx, by + BH);
        glEnd();

        // 2) Fog–
        bx += BW + SP;
        glBegin(GL_QUADS);
        glVertex2i(bx, by);
        glVertex2i(bx + BW, by);
        glVertex2i(bx + BW, by + BH);
        glVertex2i(bx, by + BH);
        glEnd();

        // 3) Fog+
        bx += BW + SP;
        glBegin(GL_QUADS);
        glVertex2i(bx, by);
        glVertex2i(bx + BW, by);
        glVertex2i(bx + BW, by + BH);
        glVertex2i(bx, by + BH);
        glEnd();

        // 4) Speed– (új sor)
        bx = MX;
        by -= BH + SP;
        glBegin(GL_QUADS);
        glVertex2i(bx, by);
        glVertex2i(bx + BW, by);
        glVertex2i(bx + BW, by + BH);
        glVertex2i(bx, by + BH);
        glEnd();

        // 5) Speed+
        bx += BW + SP;
        glBegin(GL_QUADS);
        glVertex2i(bx, by);
        glVertex2i(bx + BW, by);
        glVertex2i(bx + BW, by + BH);
        glVertex2i(bx, by + BH);
        glEnd();

        // Szövegek
        glColor3f(1, 1, 1);
        int tx, ty;
        char buf[32];

        // Toggle Ground label
        {
            const char* txt = "Toggle Ground";
            int tw = glutBitmapLength(GLUT_BITMAP_HELVETICA_18,
                (const unsigned char*)txt);
            tx = MX + (BW - tw) / 2;
            ty = vp[3] - MY - BH + PAD;
            glRasterPos2i(tx, ty); renderText(tx, ty, txt);
            // érték kiírása alatta
            snprintf(buf, sizeof(buf), "%.1f", groundExtent);
            glRasterPos2i(tx, ty - VAL_OFF); renderText(tx, ty - VAL_OFF, buf);
        }

        // Fog– label
        bx = MX + (BW + SP);
        {
            const char* txt = "Fog   -";
            int tw = glutBitmapLength(GLUT_BITMAP_HELVETICA_18,
                (const unsigned char*)txt);
            tx = bx + (BW - tw) / 2;
            ty = vp[3] - MY - BH + PAD;
            glRasterPos2i(tx, ty); renderText(tx, ty, txt);
        }
        // Fog+ label
        bx += BW + SP;
        {
            const char* txt = "Fog   +";
            int tw = glutBitmapLength(GLUT_BITMAP_HELVETICA_18,
                (const unsigned char*)txt);
            tx = bx + (BW - tw) / 2;
            ty = vp[3] - MY - BH + PAD;
            glRasterPos2i(tx, ty); renderText(tx, ty, txt);
            // fog érték kiírása
            float fogVal = lightingOn ? fogDensityWithLight : fogDensityWithoutLight;
            snprintf(buf, sizeof(buf), "%.3f", fogVal);
            glRasterPos2i(tx, ty - VAL_OFF); renderText(tx, ty - VAL_OFF, buf);
        }

        // Speed– label
        bx = MX;
        {
            const char* txt = "Speed -";
            int tw = glutBitmapLength(GLUT_BITMAP_HELVETICA_18,
                (const unsigned char*)txt);
            tx = bx + (BW - tw) / 2;
            ty = by + PAD;
            glRasterPos2i(tx, ty); renderText(tx, ty, txt);
        }
        // Speed+ label
        bx += BW + SP;
        {
            const char* txt = "Speed +";
            int tw = glutBitmapLength(GLUT_BITMAP_HELVETICA_18,
                (const unsigned char*)txt);
            tx = bx + (BW - tw) / 2;
            ty = by + PAD;
            glRasterPos2i(tx, ty); renderText(tx, ty, txt);
            // speed érték kiírása
            snprintf(buf, sizeof(buf), "%.2f", sphereSpeedScale);
            glRasterPos2i(tx, ty - VAL_OFF); renderText(tx, ty - VAL_OFF, buf);
        }

        // Mátrix és állapot vissza
        glPopMatrix();
        glMatrixMode(GL_PROJECTION); glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

        glEnable(GL_DEPTH_TEST);
        if (lightingOn) glEnable(GL_LIGHTING);
    }

    // 11) Buffercsere
    glutSwapBuffers();
}



// billentyűk kezelése
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'f': // köd ki/be
    case 'F':
        fogOn = !fogOn;
        break;
    case 'l': // világítás ki/be
    case 'L':
        lightingOn = !lightingOn;
        break;
    case 'n': // gömbök száma váltása
    case 'N':
        numSpheres = (numSpheres == 2 ? 3 : 2);
        break;
    case 27:  // ESC kilép
        exit(0);
    }
}

// ablak átméretezése
void reshape(int w, int h) {
    if (h == 0) h = 1;
    windowWidth = w;
    windowHeight = h;
    float ratio = (float)w / (float)h;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, ratio, 1.0f, 200.0f);
    glMatrixMode(GL_MODELVIEW);

    // egér középre
    ignoreNextWarp = true;
    glutWarpPointer(windowWidth / 2, windowHeight / 2);
    lastX = windowWidth / 2;
    lastY = windowHeight / 2;
}

#define BW 100
#define BH 30
#define MX 20
#define MY 20
#define SP 15

void mouse_button(int button, int state, int x, int y) {
    // Csak PAUSE alatt és bal gomb lenyomásakor
    if (!paused || button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
        return;

    // GLUT y-koordináta invertálása
    int iy = windowHeight - y;

    // 1) Toggle Ground gomb
    int bx = MX;
    int by = windowHeight - MY - BH;
    if (x >= bx && x <= bx + BW &&
        iy >= by && iy <= by + BH) {
        groundIsLarge = !groundIsLarge;
        groundExtent = groundIsLarge ? groundExtentLarge : groundExtentSmall;
        return;
    }

    // 2) Fog– gomb
    bx += BW + SP;
    if (x >= bx && x <= bx + BW &&
        iy >= by && iy <= by + BH) {
        if (lightingOn)
            fogDensityWithLight = fmaxf(0.0f, fogDensityWithLight - 0.005f);
        else
            fogDensityWithoutLight = fmaxf(0.0f, fogDensityWithoutLight - 0.005f);
        return;
    }

    // 3) Fog+ gomb
    bx += BW + SP;
    if (x >= bx && x <= bx + BW &&
        iy >= by && iy <= by + BH) {
        if (lightingOn)
            fogDensityWithLight = fminf(1.0f, fogDensityWithLight + 0.005f);
        else
            fogDensityWithoutLight = fminf(1.0f, fogDensityWithoutLight + 0.005f);
        return;
    }

    // 4) Speed– gomb (második sorban)
    bx = MX;
    by -= BH + SP;
    if (x >= bx && x <= bx + BW &&
        iy >= by && iy <= by + BH) {
        sphereSpeedScale = fmaxf(sphereSpeedMin, sphereSpeedScale - sphereSpeedStep);
        return;
    }

    // 5) Speed+ gomb
    bx += BW + SP;
    if (x >= bx && x <= bx + BW &&
        iy >= by && iy <= by + BH) {
        sphereSpeedScale = fminf(sphereSpeedMax, sphereSpeedScale + sphereSpeedStep);
        return;
    }
}

void idle() {
    // 1) Free-camera mozgatása és gömb-fizika csak ha nincs pause
    if (!paused) {
        // kamera
        do_movement();

        // gömbök fizikai léptetése
        for (int i = 0; i < numSpheres; i++) {
            Sphere* s = &spheres[i];
            // gravitáció
            s->vel[1] -= 9.81f * dt;
            // pozíció frissítése
            for (int k = 0; k < 3; k++)
                s->pos[k] += s->vel[k] * dt * sphereSpeedScale;

            // talaj-ütközés
            if (s->pos[1] - s->radius < groundY) {
                s->pos[1] = groundY + s->radius;
                s->vel[1] = -s->vel[1] * 0.8f;
            }
            // fal-ütközések X és Z irányban
            if (s->pos[0] - s->radius < -groundExtent) {
                s->pos[0] = -groundExtent + s->radius;
                s->vel[0] = -s->vel[0];
            }
            if (s->pos[0] + s->radius > groundExtent) {
                s->pos[0] = groundExtent - s->radius;
                s->vel[0] = -s->vel[0];
            }
            if (s->pos[2] - s->radius < -groundExtent) {
                s->pos[2] = -groundExtent + s->radius;
                s->vel[2] = -s->vel[2];
            }
            if (s->pos[2] + s->radius > groundExtent) {
                s->pos[2] = groundExtent - s->radius;
                s->vel[2] = -s->vel[2];
            }
        }

        // gömb-gömb ütközések
        for (int i = 0; i < numSpheres; i++) {
            for (int j = i + 1; j < numSpheres; j++) {
                handleSphereCollision(&spheres[i], &spheres[j]);
            }
        }

        for (int i = 0; i < numSpheres; i++) {
            for (int j = i + 1; j < numSpheres; j++) {
                handleSphereCollision(&spheres[i], &spheres[j]);
            }
        }

        // 2) Nap–hold idő frissítése is csak itt
        timeOfDay += dt / dayLength;
        if (timeOfDay > 1.0f)
            timeOfDay -= 1.0f;
    }

    // 3) Mindig kérjük az újrarajzolást, hogy a pause-ban is lássuk a feliratokat
    glutPostRedisplay();
}






int main(int argc, char** argv) {
    // GLUT init
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Drágaságom Gollam");

    glEnable(GL_DEPTH_TEST);
    initSpheres();
    initCamera();
    initLighting();

    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutPassiveMotionFunc(mouse_callback);
    glutSetCursor(GLUT_CURSOR_NONE);  // elrejti a kurzort, FPS-szerű élmény
    glutKeyboardFunc(keyboard_down);
    glutKeyboardUpFunc(keyboard_up);
    glutPassiveMotionFunc(mouse_callback);
    glutSetCursor(GLUT_CURSOR_NONE);
    glutMouseFunc(mouse_button);



    glutMainLoop();
    return 0;
}
