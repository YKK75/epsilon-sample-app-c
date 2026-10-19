#include "eadk.h"
#include <stdint.h>
#include <stdbool.h>

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Runner";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

/* ---------- Réglages (difficulté) ---------- */
#define LARG 320
#define HAUT 240
#define SOL 190            /* ordonnée du sol */
#define PX 40              /* position x du joueur */
#define PW 14              /* largeur du joueur */
#define PH 24              /* hauteur du joueur */
#define GRAVITE 1          /* plus grand = retombe plus vite */
#define SAUT (-10)         /* force du saut */
#define VIT0 4000          /* vitesse initiale (en milli-pixels/image) */
#define ACC 4              /* accélération par image (milli-pixels) */
#define VMAX 12000         /* vitesse maximale */
#define ECART_MIN 90       /* distance min entre obstacles */
#define ECART_MAX 170      /* distance max entre obstacles */
#define PAUSE_MS 20        /* pause entre images */
#define NB_OBS 4

#define RGB(r, g, b) ((eadk_color_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define CIEL   RGB(135, 206, 235)
#define TERRE  RGB(110, 80, 50)
#define HERBE  RGB(60, 170, 60)
#define JOUEUR RGB(220, 40, 40)
#define OBST   RGB(40, 40, 40)
#define TRAIT  RGB(255, 255, 255)
#define TEXTE  RGB(0, 0, 0)

typedef struct { int x, w, h; bool actif; } Obstacle;

/* Types d'obstacles : largeur, hauteur (bas / haut / large) */
static const int TYPES[3][2] = {{14, 18}, {14, 34}, {30, 16}};

static void rect(int x, int y, int w, int h, eadk_color_t c) {
  if (x < 0) { w += x; x = 0; }
  if (x + w > LARG) { w = LARG - x; }
  if (w > 0 && h > 0) {
    eadk_display_push_rect_uniform((eadk_rect_t){(uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h}, c);
  }
}

static void texte(const char *s, int x, int y) {
  eadk_display_draw_string(s, (eadk_point_t){(uint16_t)x, (uint16_t)y}, false, TEXTE, CIEL);
}

static void entier(int n, char *buf) {
  char tmp[12];
  int i = 0, j = 0;
  if (n == 0) { tmp[i++] = '0'; }
  while (n > 0) { tmp[i++] = (char)('0' + n % 10); n /= 10; }
  while (i > 0) { buf[j++] = tmp[--i]; }
  buf[j] = 0;
}

static void decor(void) {
  rect(0, 0, LARG, SOL, CIEL);
  rect(0, SOL, LARG, 6, HERBE);
  rect(0, SOL + 6, LARG, HAUT - SOL - 6, TERRE);
}

static void affiche_score(int score) {
  char buf[16];
  rect(0, 0, 120, 20, CIEL);
  texte("Score:", 5, 5);
  entier(score, buf);
  texte(buf, 60, 5);
}

static int jeu(void) {
  Obstacle obs[NB_OBS];
  int i, k;
  int y = SOL - PH, vy = 0;
  int v = VIT0, dist = 0, score = 0, decal = 0;
  int prochain = ECART_MIN;
  for (i = 0; i < NB_OBS; i++) obs[i].actif = false;
  decor();
  affiche_score(0);

  while (1) {
    int pas = v / 1000;
    /* --- Effacer --- */
    rect(PX, y, PW, PH, CIEL);
    for (i = 0; i < NB_OBS; i++)
      if (obs[i].actif) rect(obs[i].x, SOL - obs[i].h, obs[i].w, obs[i].h, CIEL);
    for (k = 0; k < 8; k++) rect((k * 45 - decal + LARG * 2) % LARG, SOL + 14, 14, 3, TERRE);

    /* --- Saut --- */
    eadk_keyboard_state_t kb = eadk_keyboard_scan();
    if (eadk_keyboard_key_down(kb, eadk_key_up) && y >= SOL - PH) vy = SAUT;
    vy += GRAVITE;
    y += vy;
    if (y >= SOL - PH) { y = SOL - PH; vy = 0; }

    /* --- Obstacles --- */
    for (i = 0; i < NB_OBS; i++) {
      if (!obs[i].actif) continue;
      obs[i].x -= pas;
      if (obs[i].x + obs[i].w < 0) { obs[i].actif = false; score++; affiche_score(score); }
    }
    dist += pas;
    if (dist >= prochain) {
      for (i = 0; i < NB_OBS; i++) {
        if (!obs[i].actif) {
          int t = (int)(eadk_random() % 3);
          obs[i].x = LARG - 1;
          obs[i].w = TYPES[t][0];
          obs[i].h = TYPES[t][1];
          obs[i].actif = true;
          break;
        }
      }
      dist = 0;
      prochain = ECART_MIN + (int)(eadk_random() % (ECART_MAX - ECART_MIN + 1)) + (v / 1000) * 8;
    }
    decal = (decal + pas) % LARG;
    if (v < VMAX) v += ACC;

    /* --- Collision --- */
    for (i = 0; i < NB_OBS; i++) {
      if (obs[i].actif && obs[i].x < PX + PW && obs[i].x + obs[i].w > PX && y + PH > SOL - obs[i].h) {
        rect(PX, y, PW, PH, JOUEUR);
        return score;
      }
    }

    /* --- Dessin --- */
    for (k = 0; k < 8; k++) rect((k * 45 - decal + LARG * 2) % LARG, SOL + 14, 14, 3, TRAIT);
    for (i = 0; i < NB_OBS; i++)
      if (obs[i].actif) rect(obs[i].x, SOL - obs[i].h, obs[i].w, obs[i].h, OBST);
    rect(PX, y, PW, PH, JOUEUR);
    eadk_display_wait_for_vblank();
    eadk_timing_msleep(PAUSE_MS);
  }
}

static bool ok_presse(void) {
  eadk_keyboard_state_t kb = eadk_keyboard_scan();
  return eadk_keyboard_key_down(kb, eadk_key_ok) || eadk_keyboard_key_down(kb, eadk_key_exe);
}

int main(int argc, char *argv[]) {
  int best = 0;
  char buf[16];
  while (1) {
    int s = jeu();
    if (s > best) best = s;
    texte("GAME OVER", 120, 60);
    texte("Score:", 100, 85);
    entier(s, buf);
    texte(buf, 150, 85);
    texte("Best:", 100, 105);
    entier(best, buf);
    texte(buf, 150, 105);
    texte("OK = rejouer", 110, 130);
    eadk_timing_msleep(500);
    while (!ok_presse()) eadk_timing_msleep(30);
    while (ok_presse()) eadk_timing_msleep(30);
  }
  return 0;
}
