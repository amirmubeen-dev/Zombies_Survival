

#include <GL/freeglut.h>
#include <omp.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>
#include <atomic>
#include <mutex>

const int   WINDOW_W      = 960;
const int   WINDOW_H      = 720;
const int   MAP_COLS      = 8;
const int   MAP_ROWS      = 6;
const float ZONE_W        = (float)WINDOW_W / MAP_COLS;
const float ZONE_H        = (float)WINDOW_H / MAP_ROWS;
const float PI            = 3.14159265f;
const float PLAYER_SPEED  = 3.2f;
const float BULLET_SPEED  = 9.0f;
const float ZOMBIE_BASE   = 0.75f;
const int   MAX_ZOMBIES   = 220;

struct Vec2 { float x, y; };

struct Player {
    Vec2  pos;
    float radius  = 18;
    int   health  = 100;
    bool  alive   = true;
    float angle   = 0;
    float gunLen  = 22;
    float legAnim = 0;
    bool  moving  = false;
};

struct Zombie {
    Vec2  pos;
    float radius;
    bool  alive    = true;
    int   health;
    int   maxHealth;
    float speed;
    int   type;
    float angle    = 0;
    float armAnim  = 0;
    float legAnim  = 0;
    bool  flashing = false;
    int   flashTimer = 0;
};

struct Bullet {
    Vec2  pos, vel;
    bool  alive = true;
    float trail[6][2];
    int   trailLen = 0;
};

struct Particle {
    Vec2  pos, vel;
    float life, maxLife;
    float r, g, b, size;
    int   type; 
};

struct BloodPool {
    Vec2  pos;
    float radius;
    float alpha;
};

Player player;
std::vector<Zombie>    zombies;
std::vector<Bullet>    bullets;
std::vector<Particle>  particles;
std::vector<BloodPool> bloodPools;

int  score      = 0;
int  wave       = 1;
int  frameCount = 0;
int  gameState  = 0;
bool keys[256]  = {};
bool specialKeys[256] = {};
int  mouseX = WINDOW_W/2, mouseY = WINDOW_H/2;

float shakeX = 0, shakeY = 0;
int   shakeTimer = 0;
float vignetteAlpha = 0;

std::atomic<int> zoneActivity[MAP_COLS * MAP_ROWS];
int ompThreads = 1;




float dist(Vec2 a, Vec2 b) {
    float dx=a.x-b.x, dy=a.y-b.y;
    return sqrtf(dx*dx+dy*dy);
}
float randf(float lo, float hi) {
    return lo + (float)rand()/RAND_MAX*(hi-lo);
}
int getZoneIdx(float x, float y) {
    int c = std::max(0,std::min((int)(x/ZONE_W), MAP_COLS-1));
    int r = std::max(0,std::min((int)(y/ZONE_H), MAP_ROWS-1));
    return r*MAP_COLS+c;
}




void spawnBlood(Vec2 pos, int count, float intensity=1.0f) {
    for (int i=0;i<count;i++) {
        Particle p;
        p.pos  = pos;
        float spd = randf(1.5f, 5.0f) * intensity;
        float ang = randf(0, 2*PI);
        p.vel  = { cosf(ang)*spd, sinf(ang)*spd };
        p.life = p.maxLife = randf(0.4f, 1.0f);
        p.r = randf(0.5f,0.8f); p.g=0.0f; p.b=0.0f;
        p.size = randf(2,5);
        p.type = 0;
        particles.push_back(p);
    }
    BloodPool bp;
    bp.pos = pos;
    bp.radius = randf(8, 18);
    bp.alpha = 0.7f;
    bloodPools.push_back(bp);
}

void spawnSparks(Vec2 pos, int count) {
    for (int i=0;i<count;i++) {
        Particle p;
        p.pos = pos;
        float ang = randf(0,2*PI);
        float spd = randf(2,6);
        p.vel = {cosf(ang)*spd, sinf(ang)*spd};
        p.life = p.maxLife = randf(0.2f,0.5f);
        p.r=1.0f; p.g=randf(0.6f,1.0f); p.b=0.2f;
        p.size=2; p.type=1;
        particles.push_back(p);
    }
}

void spawnSmoke(Vec2 pos) {
    for (int i=0;i<2;i++) {
        Particle p;
        p.pos={pos.x+randf(-5,5), pos.y+randf(-5,5)};
        p.vel={randf(-0.3f,0.3f), randf(-0.8f,-0.2f)};
        p.life=p.maxLife=randf(0.8f,1.5f);
        p.r=p.g=p.b=randf(0.3f,0.5f);
        p.size=randf(6,12); p.type=2;
        particles.push_back(p);
    }
}




void spawnZombie() {
    if ((int)zombies.size() >= MAX_ZOMBIES) return;
    Zombie z;
    z.alive = true;
    int edge = rand()%4;
    if (edge==0) z.pos={randf(0,WINDOW_W),-25};
    else if(edge==1) z.pos={randf(0,WINDOW_W),WINDOW_H+25};
    else if(edge==2) z.pos={-25,randf(0,WINDOW_H)};
    else             z.pos={WINDOW_W+25,randf(0,WINDOW_H)};

    int r = rand()%10;
    if (r<6) {
        z.type=0; z.radius=15; z.speed=ZOMBIE_BASE+(wave*0.05f);
        z.health=z.maxHealth=1;
    } else if(r<9) {
        z.type=1; z.radius=12; z.speed=ZOMBIE_BASE*1.9f+(wave*0.07f);
        z.health=z.maxHealth=1;
    } else {
        z.type=2; z.radius=22; z.speed=ZOMBIE_BASE*0.55f;
        z.health=z.maxHealth=4+wave;
    }
    z.angle=0; z.armAnim=randf(0,PI); z.legAnim=randf(0,PI);
    zombies.push_back(z);
}




void shootBullet(float tx, float ty) {
    Bullet b;
    b.pos = player.pos;
    b.alive = true;
    float dx=tx-player.pos.x, dy=ty-player.pos.y;
    float len=sqrtf(dx*dx+dy*dy);
    if (len<1) return;
    b.vel={dx/len*BULLET_SPEED, dy/len*BULLET_SPEED};
    b.trailLen=0;
    bullets.push_back(b);
    spawnSparks(player.pos, 3);
}

void triggerShake(float intensity, int duration) {
    shakeTimer = duration;
    shakeX = randf(-intensity, intensity);
    shakeY = randf(-intensity, intensity);
}




void updateZombies_OpenMP() {
    int n = (int)zombies.size();
    #pragma omp parallel for num_threads(4) schedule(dynamic,8)
    for (int i=0;i<n;i++) {
        if (!zombies[i].alive) continue;
        float dx=player.pos.x-zombies[i].pos.x;
        float dy=player.pos.y-zombies[i].pos.y;
        float d=sqrtf(dx*dx+dy*dy);
        if (d>0.1f) {
            zombies[i].pos.x += (dx/d)*zombies[i].speed;
            zombies[i].pos.y += (dy/d)*zombies[i].speed;
            zombies[i].angle  = atan2f(dy,dx);
        }
        zombies[i].armAnim += 0.06f;
        zombies[i].legAnim += 0.09f;
        if (zombies[i].flashTimer>0) zombies[i].flashTimer--;
        else zombies[i].flashing=false;
        int zIdx=getZoneIdx(zombies[i].pos.x, zombies[i].pos.y);
        zoneActivity[zIdx]++;
    }
    ompThreads = omp_get_max_threads();
}




struct ZoneThreat { int zone,count; };
std::vector<ZoneThreat> zoneThreatReport;

void mpi_zone_analysis() {
    for (int i=0;i<MAP_COLS*MAP_ROWS;i++) zoneActivity[i]=0;
    for (auto& z:zombies) {
        if (!z.alive) continue;
        zoneActivity[getZoneIdx(z.pos.x,z.pos.y)]++;
    }
    zoneThreatReport.clear();
    for (int i=0;i<MAP_COLS*MAP_ROWS;i++)
        if (zoneActivity[i]>2)
            zoneThreatReport.push_back({i,zoneActivity[i].load()});
}




void checkCollisions() {
    int nb=(int)bullets.size(), nz=(int)zombies.size();
    #pragma omp parallel for num_threads(4)
    for (int bi=0;bi<nb;bi++) {
        if (!bullets[bi].alive) continue;
        for (int zi=0;zi<nz;zi++) {
            if (!zombies[zi].alive) continue;
            if (dist(bullets[bi].pos,zombies[zi].pos) < zombies[zi].radius+5) {
                bullets[bi].alive=false;
                zombies[zi].health--;
                zombies[zi].flashing=true;
                zombies[zi].flashTimer=5;
                if (zombies[zi].health<=0) {
                    zombies[zi].alive=false;
                    #pragma omp atomic
                    score += (zombies[zi].type==2)?30:(zombies[zi].type==1)?20:10;
                }
            }
        }
    }
    for (auto& z:zombies) {
        if (!z.alive) continue;
        if (dist(player.pos,z.pos) < player.radius+z.radius-6) {
            player.health -= (z.type==2)?2:1;
            vignetteAlpha = 0.6f;
            triggerShake(6.0f, 8);
            if (player.health<=0) { player.alive=false; gameState=2; }
        }
    }
}




void updateParticles_OpenMP() {
    int n=(int)particles.size();
    #pragma omp parallel for num_threads(2)
    for (int i=0;i<n;i++) {
        particles[i].pos.x += particles[i].vel.x;
        particles[i].pos.y += particles[i].vel.y;
        particles[i].vel.x *= 0.92f;
        particles[i].vel.y *= 0.92f;
        particles[i].life  -= 0.025f;
    }
    particles.erase(std::remove_if(particles.begin(),particles.end(),
        [](const Particle& p){return p.life<=0;}),particles.end());
    for (auto& bp:bloodPools) bp.alpha -= 0.0003f;
    bloodPools.erase(std::remove_if(bloodPools.begin(),bloodPools.end(),
        [](const BloodPool& b){return b.alpha<=0;}),bloodPools.end());
}




void update() {
    if (gameState!=1) return;
    frameCount++;

    player.angle = atan2f((float)mouseY - player.pos.y,
                          (float)mouseX - player.pos.x);

    bool moving = false;
    if (keys['w']||keys['W']||specialKeys[GLUT_KEY_UP])    { player.pos.y-=PLAYER_SPEED; moving=true; }
    if (keys['s']||keys['S']||specialKeys[GLUT_KEY_DOWN])  { player.pos.y+=PLAYER_SPEED; moving=true; }
    if (keys['a']||keys['A']||specialKeys[GLUT_KEY_LEFT])  { player.pos.x-=PLAYER_SPEED; moving=true; }
    if (keys['d']||keys['D']||specialKeys[GLUT_KEY_RIGHT]) { player.pos.x+=PLAYER_SPEED; moving=true; }
    player.moving = moving;
    if (moving) player.legAnim += 0.18f;

    player.pos.x=std::max(player.radius,std::min((float)WINDOW_W-player.radius,player.pos.x));
    player.pos.y=std::max(player.radius,std::min((float)WINDOW_H-player.radius,player.pos.y));

    int spawnRate=std::max(15, 80-wave*7);
    if (frameCount%spawnRate==0) spawnZombie();
    if (score >= wave*120) wave++;

    if (frameCount%25==0) {
        Vec2 gunTip={
            player.pos.x + cosf(player.angle)*(player.gunLen+5),
            player.pos.y + sinf(player.angle)*(player.gunLen+5)
        };
        spawnSmoke(gunTip);
    }

    if (shakeTimer>0) {
        shakeTimer--;
        shakeX = randf(-4,4) * (shakeTimer/10.0f);
        shakeY = randf(-4,4) * (shakeTimer/10.0f);
    } else { shakeX=0; shakeY=0; }

    if (vignetteAlpha>0) vignetteAlpha -= 0.015f;

    updateZombies_OpenMP();
    mpi_zone_analysis();

    for (auto& b:bullets) {
        if (!b.alive) continue;
        if (b.trailLen<6) {
            b.trail[b.trailLen][0]=b.pos.x;
            b.trail[b.trailLen][1]=b.pos.y;
            b.trailLen++;
        } else {
            for (int i=0;i<5;i++) {
                b.trail[i][0]=b.trail[i+1][0];
                b.trail[i][1]=b.trail[i+1][1];
            }
            b.trail[5][0]=b.pos.x;
            b.trail[5][1]=b.pos.y;
        }
        b.pos.x+=b.vel.x;
        b.pos.y+=b.vel.y;
        if (b.pos.x<0||b.pos.x>WINDOW_W||b.pos.y<0||b.pos.y>WINDOW_H)
            b.alive=false;
    }

    checkCollisions();

    for (auto& z:zombies)
        if (!z.alive)
            spawnBlood(z.pos, (z.type==2)?20:10, (z.type==2)?1.5f:1.0f);

    zombies.erase(std::remove_if(zombies.begin(),zombies.end(),
        [](const Zombie& z){return !z.alive;}),zombies.end());
    bullets.erase(std::remove_if(bullets.begin(),bullets.end(),
        [](const Bullet& b){return !b.alive;}),bullets.end());

    updateParticles_OpenMP();
}





void drawCircleFill(float x, float y, float r,
                    float R, float G, float B, float A=1.0f, int segs=24) {
    glColor4f(R,G,B,A);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x,y);
    for (int i=0;i<=segs;i++) {
        float a=i*2*PI/segs;
        glVertex2f(x+cosf(a)*r, y+sinf(a)*r);
    }
    glEnd();
}

void drawCircleOutline(float x, float y, float r,
                       float R, float G, float B, float lw=1.5f, int segs=24) {
    glLineWidth(lw);
    glColor3f(R,G,B);
    glBegin(GL_LINE_LOOP);
    for (int i=0;i<segs;i++) {
        float a=i*2*PI/segs;
        glVertex2f(x+cosf(a)*r, y+sinf(a)*r);
    }
    glEnd();
}

void drawRect(float x, float y, float w, float h,
              float R, float G, float B, float A=1.0f) {
    glColor4f(R,G,B,A);
    glBegin(GL_QUADS);
    glVertex2f(x,y); glVertex2f(x+w,y);
    glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}

void drawLine(float x1,float y1,float x2,float y2,
              float R,float G,float B,float lw=2.0f,float A=1.0f) {
    glLineWidth(lw);
    glColor4f(R,G,B,A);
    glBegin(GL_LINES);
    glVertex2f(x1,y1); glVertex2f(x2,y2);
    glEnd();
}

void drawText(float x,float y,const std::string& s,
              float R=1,float G=1,float B=1,float A=1.0f) {
    glColor4f(R,G,B,A);
    glRasterPos2f(x,y);
    for (char c:s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,c);
}

void drawSmall(float x,float y,const std::string& s,
               float R=1,float G=1,float B=1) {
    glColor3f(R,G,B);
    glRasterPos2f(x,y);
    for (char c:s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12,c);
}


void drawBackground() {
    
    drawRect(0,0,WINDOW_W,WINDOW_H, 0.04f,0.02f,0.02f);

    
    glLineWidth(1.0f);
    glColor4f(0.10f, 0.03f, 0.03f, 1.0f);
    int tile = 48;
    for (int x=0;x<WINDOW_W;x+=tile) {
        glBegin(GL_LINES);
        glVertex2f((float)x,0); glVertex2f((float)x,(float)WINDOW_H);
        glEnd();
    }
    for (int y=0;y<WINDOW_H;y+=tile) {
        glBegin(GL_LINES);
        glVertex2f(0,(float)y); glVertex2f((float)WINDOW_W,(float)y);
        glEnd();
    }

    
    float cx=WINDOW_W/2.0f, cy=WINDOW_H/2.0f;
    float maxR=sqrtf(cx*cx+cy*cy)*1.1f;
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0,0,0,0); glVertex2f(cx,cy);
    for (int i=0;i<=40;i++) {
        float a=i*2*PI/40;
        glColor4f(0,0,0,0.7f);
        glVertex2f(cx+cosf(a)*maxR, cy+sinf(a)*maxR);
    }
    glEnd();

    
    for (auto& bp:bloodPools)
        drawCircleFill(bp.pos.x, bp.pos.y, bp.radius,
                       0.35f,0.0f,0.0f, bp.alpha*0.8f, 16);
}


void drawMPIZones() {
    for (int row=0;row<MAP_ROWS;row++) {
        for (int col=0;col<MAP_COLS;col++) {
            int idx=row*MAP_COLS+col;
            int cnt=zoneActivity[idx].load();
            float intensity=std::min(cnt/12.0f, 0.5f);
            if (intensity>0.05f) {
                drawRect(col*ZONE_W,row*ZONE_H,ZONE_W,ZONE_H,
                         0.8f,0.0f,0.0f, intensity*0.22f);
                glLineWidth(1.5f);
                glColor4f(0.8f,0,0, intensity*0.7f);
                glBegin(GL_LINE_LOOP);
                glVertex2f(col*ZONE_W,     row*ZONE_H);
                glVertex2f((col+1)*ZONE_W, row*ZONE_H);
                glVertex2f((col+1)*ZONE_W, (row+1)*ZONE_H);
                glVertex2f(col*ZONE_W,     (row+1)*ZONE_H);
                glEnd();
            }
        }
    }
}


void drawPlayer() {
    float x=player.pos.x, y=player.pos.y;
    float ang=player.angle;
    float legSwing = sinf(player.legAnim)*8.0f;

    glPushMatrix();
    glTranslatef(x,y,0);

    
    drawCircleFill(3,5, player.radius*0.9f, 0,0,0, 0.35f,18);

    
    float lx=cosf(ang+PI*0.5f)*7, ly=sinf(ang+PI*0.5f)*7;
    float fw=cosf(ang)*10, fwy=sinf(ang)*10;
    if (player.moving) {
        drawLine(lx,ly,  lx+fw+cosf(ang)*legSwing, ly+fwy+sinf(ang)*legSwing,
                 0.25f,0.3f,0.25f,1.0f,5.0f);
        drawLine(-lx,-ly,-lx+fw-cosf(ang)*legSwing,-ly+fwy-sinf(ang)*legSwing,
                 0.25f,0.3f,0.25f,1.0f,5.0f);
    } else {
        drawLine(lx,ly,   lx+fw, ly+fwy, 0.25f,0.3f,0.25f,1.0f,5.0f);
        drawLine(-lx,-ly,-lx+fw,-ly+fwy, 0.25f,0.3f,0.25f,1.0f,5.0f);
    }

    
    drawCircleFill(0,0, player.radius,      0.12f,0.22f,0.12f, 1.0f, 22);
    drawCircleFill(0,0, player.radius*0.65f,0.18f,0.32f,0.18f, 1.0f, 22);

    
    float sx=cosf(ang+PI*0.5f)*(player.radius*0.75f);
    float sy=sinf(ang+PI*0.5f)*(player.radius*0.75f);
    drawCircleFill( sx, sy, 7, 0.15f,0.28f,0.15f,1.0f,10);
    drawCircleFill(-sx,-sy, 7, 0.15f,0.28f,0.15f,1.0f,10);

    
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.16f,0.26f,0.16f,1);
    glVertex2f(cosf(ang)*5, sinf(ang)*5);
    for (int i=0;i<=20;i++) {
        float a=ang-PI*0.55f + i*(PI*1.1f)/20.0f;
        glColor4f(0.22f,0.36f,0.22f,1);
        glVertex2f(cosf(a)*(player.radius*0.88f), sinf(a)*(player.radius*0.88f));
    }
    glEnd();

    
    float gx=cosf(ang)*12, gy=sinf(ang)*12;
    drawLine(0,0, gx,gy, 0.2f,0.28f,0.2f, 5.0f);
    
    float gx2=cosf(ang)*player.gunLen, gy2=sinf(ang)*player.gunLen;
    drawLine(gx,gy, gx2,gy2, 0.12f,0.12f,0.12f, 5.0f);
    
    if (frameCount%6<2)
        drawCircleFill(gx2,gy2, 5, 1.0f,0.9f,0.3f, 0.5f, 8);
    else
        drawCircleFill(gx2,gy2, 3, 0.4f,0.4f,0.4f, 1.0f, 8);

    
    float vx=cosf(ang)*11, vy=sinf(ang)*11;
    drawCircleFill(vx,vy, 4.0f, 0.5f,0.9f,1.0f, 0.9f, 8);
    drawCircleFill(vx,vy, 7.0f, 0.3f,0.7f,1.0f, 0.2f, 8);

    
    drawCircleOutline(0,0, player.radius, 0.35f,0.65f,0.35f, 1.5f, 22);

    glPopMatrix();
}


void drawZombie(const Zombie& z) {
    float x=z.pos.x, y=z.pos.y;
    float ang=z.angle;
    float armSwing = sinf(z.armAnim)*14.0f;
    float legSwing = sinf(z.legAnim)*9.0f;

    float br,bg,bb;
    if (z.type==0)      { br=0.18f; bg=0.52f; bb=0.12f; }
    else if (z.type==1) { br=0.62f; bg=0.18f; bb=0.08f; }
    else                { br=0.42f; bg=0.08f; bb=0.42f; }

    if (z.flashing) { br=1.0f; bg=1.0f; bb=1.0f; }

    glPushMatrix();
    glTranslatef(x,y,0);

    
    drawCircleFill(2,5, z.radius*0.85f, 0,0,0, 0.3f,14);

    float lx=cosf(ang+PI*0.5f)*z.radius*0.4f;
    float ly=sinf(ang+PI*0.5f)*z.radius*0.4f;
    float fw=cosf(ang)*z.radius*0.55f;
    float fwy=sinf(ang)*z.radius*0.55f;
    float lw = (z.type==2) ? 5.0f : 3.5f;

    
    drawLine( lx, ly,  lx+fw+cosf(ang)*legSwing,  ly+fwy+sinf(ang)*legSwing,
              br*0.7f,bg*0.7f,bb*0.7f,1.0f,lw);
    drawLine(-lx,-ly, -lx+fw-cosf(ang)*legSwing, -ly+fwy-sinf(ang)*legSwing,
              br*0.7f,bg*0.7f,bb*0.7f,1.0f,lw);

    
    drawCircleFill(0,0, z.radius,       br*0.65f,bg*0.65f,bb*0.65f, 1.0f, 20);
    drawCircleFill(0,0, z.radius*0.72f, br,bg,bb, 1.0f, 20);

    
    float ax=cosf(ang)*z.radius*0.5f, ay=sinf(ang)*z.radius*0.5f;
    drawLine( lx, ly,
              ax*2.2f+lx+cosf(ang+PI*0.5f)*armSwing*0.5f,
              ay*2.2f+ly+sinf(ang+PI*0.5f)*armSwing*0.5f,
              br,bg,bb,1.0f,lw);
    drawLine(-lx,-ly,
              ax*2.2f-lx-cosf(ang+PI*0.5f)*armSwing*0.5f,
              ay*2.2f-ly-sinf(ang+PI*0.5f)*armSwing*0.5f,
              br,bg,bb,1.0f,lw);
    
    float hx1=ax*2.2f+lx, hy1=ay*2.2f+ly;
    float hx2=ax*2.2f-lx, hy2=ay*2.2f-ly;
    for (int c=0;c<3;c++) {
        float ca=ang+PI*0.5f + (c-1)*0.5f;
        drawLine(hx1,hy1, hx1+cosf(ca)*6, hy1+sinf(ca)*6, br,bg,bb,1.0f,1.5f);
        drawLine(hx2,hy2, hx2+cosf(ca)*6, hy2+sinf(ca)*6, br,bg,bb,1.0f,1.5f);
    }

    
    float hpx=cosf(ang)*z.radius*0.52f, hpy=sinf(ang)*z.radius*0.52f;
    drawCircleFill(hpx,hpy, z.radius*0.5f, br*1.1f,bg*1.1f,bb*1.1f, 1.0f, 16);

    
    float ex=cosf(ang+PI*0.5f)*z.radius*0.18f;
    float ey=sinf(ang+PI*0.5f)*z.radius*0.18f;
    
    drawCircleFill(hpx+ex, hpy+ey, 6.0f, 1.0f,0.0f,0.0f, 0.2f, 8);
    drawCircleFill(hpx-ex, hpy-ey, 6.0f, 1.0f,0.0f,0.0f, 0.2f, 8);
    
    drawCircleFill(hpx+ex, hpy+ey, 2.8f, 1.0f,0.15f,0.0f, 1.0f, 8);
    drawCircleFill(hpx-ex, hpy-ey, 2.8f, 1.0f,0.15f,0.0f, 1.0f, 8);

    
    float mx=hpx+cosf(ang)*z.radius*0.15f;
    float my=hpy+sinf(ang)*z.radius*0.15f;
    drawLine(mx-cosf(ang+PI*0.5f)*4, my-sinf(ang+PI*0.5f)*4,
             mx+cosf(ang+PI*0.5f)*4, my+sinf(ang+PI*0.5f)*4,
             0.7f,0.0f,0.0f,1.0f,2.0f);

    
    drawCircleOutline(0,0, z.radius, 0,0,0, 1.5f, 20);

    
    if (z.type==2 && z.health>0) {
        float bw=z.radius*2.3f;
        float bx=-z.radius*1.15f, by=-z.radius-13;
        drawRect(bx,by,bw,7, 0.15f,0.0f,0.0f);
        drawRect(bx,by,bw*((float)z.health/z.maxHealth),7, 0.9f,0.1f,0.1f);
    }

    glPopMatrix();
}


void drawBullet(const Bullet& b) {
    for (int i=0;i<b.trailLen-1;i++) {
        float alpha=(float)i/b.trailLen*0.65f;
        glLineWidth(2.5f);
        glColor4f(1.0f,0.85f,0.3f,alpha);
        glBegin(GL_LINES);
        glVertex2f(b.trail[i][0],   b.trail[i][1]);
        glVertex2f(b.trail[i+1][0], b.trail[i+1][1]);
        glEnd();
    }
    drawCircleFill(b.pos.x,b.pos.y, 9.0f, 1.0f,0.8f,0.2f, 0.25f, 8);
    drawCircleFill(b.pos.x,b.pos.y, 5.0f, 1.0f,0.95f,0.5f, 1.0f, 8);
}


void drawParticles() {
    for (auto& p:particles) {
        float t=p.life/p.maxLife;
        if (p.type==0) {
            drawCircleFill(p.pos.x,p.pos.y, p.size*t, p.r,p.g,p.b, t*0.9f, 6);
        } else if (p.type==1) {
            glPointSize(p.size);
            glColor4f(p.r,p.g,p.b,t);
            glBegin(GL_POINTS); glVertex2f(p.pos.x,p.pos.y); glEnd();
        } else {
            drawCircleFill(p.pos.x,p.pos.y, p.size*(2.0f-t), p.r,p.g,p.b, t*0.22f, 10);
        }
    }
}


void drawVignette() {
    if (vignetteAlpha<=0) return;
    float cx=WINDOW_W/2.0f, cy=WINDOW_H/2.0f;
    float maxR=sqrtf(cx*cx+cy*cy);
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0,0,0,0); glVertex2f(cx,cy);
    for (int i=0;i<=40;i++) {
        float a=i*2*PI/40;
        glColor4f(0.85f,0.0f,0.0f, vignetteAlpha*0.9f);
        glVertex2f(cx+cosf(a)*maxR, cy+sinf(a)*maxR);
    }
    glEnd();
}


void drawHUD() {
    
    drawRect(0,0,WINDOW_W,54, 0.03f,0.01f,0.01f, 0.93f);
    drawLine(0,54,WINDOW_W,54, 0.55f,0.0f,0.0f, 2.0f);

    
    float hpRatio=std::max(0.0f,(float)player.health/100.0f);
    drawRect(12,14,154,22, 0.25f,0.0f,0.0f);
    float hR=1.0f-hpRatio*0.3f, hG=hpRatio*0.8f;
    drawRect(12,14,154*hpRatio,22, hR,hG,0.0f);
    drawText(16,31,"HP: "+std::to_string(player.health), 1,1,1);

    drawText(WINDOW_W/2-58,33,"SCORE: "+std::to_string(score), 1.0f,0.85f,0.15f);
    drawText(WINDOW_W-118,33,"WAVE: "+std::to_string(wave), 0.9f,0.3f,0.3f);

    
    drawRect(0,WINDOW_H-58,WINDOW_W,58, 0.02f,0.01f,0.01f, 0.92f);
    drawLine(0,WINDOW_H-58,WINDOW_W,WINDOW_H-58, 0.4f,0.0f,0.0f, 1.5f);

    drawSmall(10,WINDOW_H-40,
        "[OpenMP] Active threads: "+std::to_string(ompThreads)+
        "  |  Zombie AI updated in parallel: "+std::to_string((int)zombies.size()),
        0.3f,1.0f,0.3f);
    drawSmall(10,WINDOW_H-18,
        "[MPI] Hot zones: "+std::to_string((int)zoneThreatReport.size())+"/"+
        std::to_string(MAP_COLS*MAP_ROWS)+
        "  |  [OpenGL GPU] Entities on GPU: "+
        std::to_string((int)zombies.size()+(int)bullets.size()+(int)particles.size()),
        1.0f,0.6f,0.2f);

    drawSmall(WINDOW_W-178,WINDOW_H-40,
        "Zombies: "+std::to_string((int)zombies.size()), 0.9f,0.3f,0.3f);
    drawSmall(WINDOW_W-178,WINDOW_H-18,
        "Particles: "+std::to_string((int)particles.size()), 0.7f,0.5f,0.9f);
}


void drawMenu() {
    drawRect(0,0,WINDOW_W,WINDOW_H, 0.03f,0.0f,0.0f);

    
    for (int i=0;i<9;i++) {
        float bx=55.0f+i*105.0f;
        float drip=30.0f + sinf(frameCount*0.025f+i*1.3f)*10.0f;
        drawRect(bx-3,0, 6,drip, 0.5f,0.0f,0.0f, 0.8f);
        drawCircleFill(bx,drip, 8, 0.5f,0.0f,0.0f, 0.85f, 12);
    }

    
    glColor4f(0,0,0,0.85f);
    glRasterPos2f(WINDOW_W/2-178, WINDOW_H/2-110);
    std::string t1="PARALLEL ZOMBIE SURVIVAL";
    for (char c:t1) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,c);

    glColor4f(0.92f,0.08f,0.08f,1);
    glRasterPos2f(WINDOW_W/2-180, WINDOW_H/2-112);
    for (char c:t1) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,c);

    drawText(WINDOW_W/2-148,WINDOW_H/2-70,"Semester Project  —  Parallel Computing",0.65f,0.65f,0.65f);

    
    float bx=WINDOW_W/2-148.0f;
    drawRect(bx,    WINDOW_H/2-46, 90,26, 0.08f,0.38f,0.08f,0.85f);
    drawRect(bx+100,WINDOW_H/2-46, 62,26, 0.08f,0.08f,0.48f,0.85f);
    drawRect(bx+172,WINDOW_H/2-46,124,26, 0.38f,0.08f,0.08f,0.85f);
    drawSmall(bx+10,  WINDOW_H/2-27,"OpenMP",    0.3f,1.0f,0.3f);
    drawSmall(bx+110, WINDOW_H/2-27,"MPI",       0.4f,0.6f,1.0f);
    drawSmall(bx+180, WINDOW_H/2-27,"OpenGL GPU",1.0f,0.6f,0.3f);

    drawText(WINDOW_W/2-108,WINDOW_H/2+18, "WASD / Arrows  =  Move",    1,1,1);
    drawText(WINDOW_W/2-108,WINDOW_H/2+48, "Left Click  =  Shoot",      1,1,1);
    drawText(WINDOW_W/2-108,WINDOW_H/2+78, "ESC  =  Quit",              1,1,1);

    float pulse=0.55f+0.45f*sinf(frameCount*0.08f);
    glColor4f(1.0f,0.85f,0.1f,pulse);
    glRasterPos2f(WINDOW_W/2-122, WINDOW_H/2+128);
    std::string s2=">>>  PRESS ENTER TO START  <<<";
    for (char c:s2) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,c);
}


void drawGameOver() {
    drawRect(0,0,WINDOW_W,WINDOW_H, 0,0,0, 0.78f);

    glColor4f(0.9f,0.05f,0.05f,1);
    glRasterPos2f(WINDOW_W/2-98,WINDOW_H/2-82);
    std::string s="GAME  OVER";
    for (char c:s) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,c);

    drawText(WINDOW_W/2-92,WINDOW_H/2-32,
             "Final Score: "+std::to_string(score),1.0f,0.85f,0.15f);
    drawText(WINDOW_W/2-92,WINDOW_H/2+5,
             "Wave Reached: "+std::to_string(wave),0.8f,0.3f,0.3f);
    drawText(WINDOW_W/2-92,WINDOW_H/2+42,
             "OpenMP Threads Used: "+std::to_string(ompThreads),0.3f,0.9f,0.3f);

    float pulse=0.55f+0.45f*sinf(frameCount*0.09f);
    drawText(WINDOW_W/2-122,WINDOW_H/2+90,
             "Press ENTER to Play Again",1,1,1,pulse);
}




void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(shakeX, shakeY, 0);

    if (gameState==0) {
        frameCount++;
        drawMenu();
    } else if (gameState==1) {
        drawBackground();
        drawMPIZones();
        drawParticles();
        for (auto& z:zombies) if (z.alive) drawZombie(z);
        for (auto& b:bullets) if (b.alive) drawBullet(b);
        drawPlayer();
        drawVignette();
        drawHUD();
    } else {
        drawBackground();
        for (auto& z:zombies) if (z.alive) drawZombie(z);
        drawGameOver();
    }

    glutSwapBuffers();
}

void initGame() {
    player.pos={WINDOW_W/2.0f,WINDOW_H/2.0f};
    player.health=100; player.alive=true;
    player.angle=0; player.legAnim=0; player.moving=false;
    zombies.clear(); bullets.clear();
    particles.clear(); bloodPools.clear();
    score=0; wave=1; frameCount=0;
    vignetteAlpha=0; shakeTimer=0; shakeX=0; shakeY=0;
    for (int i=0;i<MAP_COLS*MAP_ROWS;i++) zoneActivity[i]=0;
    gameState=1;
}

void keyboard(unsigned char key,int,int) {
    keys[key]=true;
    if (key==27) exit(0);
    if (key==13 && (gameState==0||gameState==2)) initGame();
}
void keyboardUp(unsigned char key,int,int) { keys[key]=false; }
void specialKey(int key,int,int)   { if(key<256) specialKeys[key]=true;  }
void specialKeyUp(int key,int,int) { if(key<256) specialKeys[key]=false; }
void mouseMove(int x,int y)    { mouseX=x; mouseY=y; }
void mousePassive(int x,int y) { mouseX=x; mouseY=y; }
void mouse(int button,int state,int x,int y) {
    mouseX=x; mouseY=y;
    if (gameState==1 && button==GLUT_LEFT_BUTTON && state==GLUT_DOWN)
        shootBullet((float)x,(float)y);
}

void timer(int) { update(); glutPostRedisplay(); glutTimerFunc(16,timer,0); }

void reshape(int w,int h) {
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0,w,h,0);
    glMatrixMode(GL_MODELVIEW);
}




int main(int argc,char** argv) {
    srand((unsigned)time(nullptr));
    omp_set_num_threads(4);
    std::cout<<"[OpenMP] Max threads: "<<omp_get_max_threads()<<"\n";
    std::cout<<"[MPI Sim] Zones: "<<MAP_COLS*MAP_ROWS<<" ("<<MAP_COLS<<"x"<<MAP_ROWS<<")\n";
    std::cout<<"[OpenGL] GPU renderer starting...\n";

    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(WINDOW_W,WINDOW_H);
    glutCreateWindow("Parallel Zombie Survival - Horror Edition | OpenMP + MPI + OpenGL");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.03f,0.01f,0.01f,1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKey);
    glutSpecialUpFunc(specialKeyUp);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMove);
    glutPassiveMotionFunc(mousePassive);
    glutTimerFunc(16,timer,0);

    std::cout<<"[GAME] Ready! Press ENTER to begin.\n";
    glutMainLoop();
    return 0;
}
