// Cobra Arena - jogo estilo slither.io para PS3 (homebrew, offline)
// Biblioteca grafica: tiny3d (sobre PSL1GHT). Controle: DualShock 3.
//
// Controles:
//   MENU:   X ou START = jogar | O = sair
//   JOGO:   D-pad esquerda/direita = virar | START = voltar ao menu
//   FIM:    X = jogar de novo | O = menu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include <sysutil/sysutil.h>
#include <io/pad.h>
#include <tiny3d.h>

#define SCREEN_W 1280.0f
#define SCREEN_H 720.0f

#define ARENA_MARGIN 20.0f
#define MAX_SNAKES   6          // 1 jogador + 5 IAs
#define MAX_BODY     500        // pontos guardados por cobra
#define MAX_FOOD     250
#define SEG_SPACING  5.0f       // distancia entre pontos do corpo

#define BASE_LEN     40         // pontos iniciais do corpo
#define GROW_PER_FOOD 8

typedef enum { STATE_MENU, STATE_PLAY, STATE_OVER } GameState;

typedef struct {
    float x, y;
    float r, g, b;
} Food;

typedef struct {
    float x, y;            // cabeca
    float angle;           // direcao em radianos
    float speed;
    int   alive;
    int   ai;
    int   target_len;      // comprimento alvo em pontos
    int   npts;            // pontos atuais do corpo
    float pts[MAX_BODY][2];// pontos do corpo, pts[0] = cabeca
    float r, g, b;         // cor base
    int   score;
} Snake;

static Snake snakes[MAX_SNAKES];
static Food  food[MAX_FOOD];
static GameState state = STATE_MENU;
static int exit_requested = 0;
static int menu_sel = 0;

// ---------- sistema ----------

static void sysutil_cb(uint64_t status, uint64_t param, void *userdata) {
    (void)param; (void)userdata;
    if (status == SYSUTIL_EVENT_MENU_CLOSE || status == SYSUTIL_EVENT_EXIT_GAME) {
        exit_requested = 1;
    }
}

static float frand(float a, float b) {
    return a + (b - a) * ((float)rand() / (float)RAND_MAX);
}

static float angle_diff(float a, float b) {
    float d = b - a;
    while (d >  M_PI) d -= 2.0f * (float)M_PI;
    while (d < -M_PI) d += 2.0f * (float)M_PI;
    return d;
}

// ---------- comida ----------

static void spawn_food(Food *f) {
    f->x = frand(ARENA_MARGIN + 10, SCREEN_W - ARENA_MARGIN - 10);
    f->y = frand(ARENA_MARGIN + 10, SCREEN_H - ARENA_MARGIN - 10);
    f->r = frand(0.4f, 1.0f);
    f->g = frand(0.4f, 1.0f);
    f->b = frand(0.4f, 1.0f);
}

static void init_food(void) {
    for (int i = 0; i < MAX_FOOD; i++) spawn_food(&food[i]);
}

// ---------- cobras ----------

static void reset_snake(Snake *s, int idx, int ai) {
    static const float colors[MAX_SNAKES][3] = {
        {0.20f, 0.95f, 0.40f},   // jogador: verde
        {0.95f, 0.30f, 0.30f},
        {0.30f, 0.55f, 1.00f},
        {1.00f, 0.80f, 0.20f},
        {0.85f, 0.35f, 0.95f},
        {0.25f, 0.95f, 0.95f},
    };
    s->ai = ai;
    s->alive = 1;
    s->score = 0;
    s->speed = ai ? 2.6f : 3.0f;
    s->r = colors[idx][0];
    s->g = colors[idx][1];
    s->b = colors[idx][2];
    s->target_len = BASE_LEN;

    s->x = frand(200, SCREEN_W - 200);
    s->y = frand(150, SCREEN_H - 150);
    s->angle = frand(-(float)M_PI, (float)M_PI);

    s->npts = BASE_LEN;
    for (int i = 0; i < s->npts; i++) {
        s->pts[i][0] = s->x - cosf(s->angle) * SEG_SPACING * i;
        s->pts[i][1] = s->y - sinf(s->angle) * SEG_SPACING * i;
    }
}

static void reset_game(void) {
    init_food();
    for (int i = 0; i < MAX_SNAKES; i++) {
        reset_snake(&snakes[i], i, i != 0);
    }
}

// Verifica se o ponto (px,py) bate no corpo de outra cobra
static int hits_body(float px, float py, int self_idx, float radius) {
    for (int j = 0; j < MAX_SNAKES; j++) {
        Snake *o = &snakes[j];
        if (!o->alive) continue;
        int start = (j == self_idx) ? 15 : 1; // a propria cabeca tem folga
        for (int k = start; k < o->npts; k++) {
            float dx = px - o->pts[k][0];
            float dy = py - o->pts[k][1];
            if (dx * dx + dy * dy < radius * radius) return 1;
        }
    }
    return 0;
}

static int outside_arena(float px, float py) {
    return px < ARENA_MARGIN || px > SCREEN_W - ARENA_MARGIN ||
           py < ARENA_MARGIN || py > SCREEN_H - ARENA_MARGIN;
}

// Quando uma cobra morre, o corpo vira comida
static void drop_body_as_food(Snake *s) {
    for (int k = 0; k < s->npts; k += 3) {
        for (int f = 0; f < MAX_FOOD; f++) {
            // substitui uma comida "livre" pelo pedaco do corpo
            if (rand() % MAX_FOOD == 0) {
                food[f].x = s->pts[k][0];
                food[f].y = s->pts[k][1];
                food[f].r = s->r;
                food[f].g = s->g;
                food[f].b = s->b;
                break;
            }
        }
    }
}

// IA: vai atras da comida mais proxima e evita paredes e corpos
static void ai_steer(Snake *s, int idx) {
    float best_d2 = 1e18f;
    float tx = s->x, ty = s->y;
    for (int i = 0; i < MAX_FOOD; i++) {
        float dx = food[i].x - s->x;
        float dy = food[i].y - s->y;
        float d2 = dx * dx + dy * dy;
        if (d2 < best_d2) { best_d2 = d2; tx = food[i].x; ty = food[i].y; }
    }
    float want = atan2f(ty - s->y, tx - s->x);

    // olha a frente em dois angulos; se houver perigo, desvia para o lado livre
    const float look = 55.0f;
    float left_a  = s->angle - 0.6f;
    float right_a = s->angle + 0.6f;
    float front_a = s->angle;
    int danger_front = outside_arena(s->x + cosf(front_a) * look, s->y + sinf(front_a) * look) ||
                       hits_body(s->x + cosf(front_a) * look, s->y + sinf(front_a) * look, idx, 10);
    int danger_left  = outside_arena(s->x + cosf(left_a) * look,  s->y + sinf(left_a) * look) ||
                       hits_body(s->x + cosf(left_a) * look,  s->y + sinf(left_a) * look, idx, 10);
    int danger_right = outside_arena(s->x + cosf(right_a) * look, s->y + sinf(right_a) * look) ||
                       hits_body(s->x + cosf(right_a) * look, s->y + sinf(right_a) * look, idx, 10);

    if (danger_front) {
        if (!danger_left && danger_right)       want = left_a;
        else if (!danger_right && danger_left)  want = right_a;
        else                                    want = s->angle + ((rand() & 1) ? 1.2f : -1.2f);
    }

    float d = angle_diff(s->angle, want);
    const float turn = 0.07f;    // limite de giro por frame (suave)
    if (d >  turn) d =  turn;
    if (d < -turn) d = -turn;
    s->angle += d;
}

static void update_snake(Snake *s, int idx, const padData *pad) {
    if (!s->alive) return;

    if (s->ai) {
        ai_steer(s, idx);
    } else {
        const float turn = 0.075f;
        if (pad->button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_LEFT)  s->angle -= turn;
        if (pad->button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_RIGHT) s->angle += turn;
    }

    s->x += cosf(s->angle) * s->speed;
    s->y += sinf(s->angle) * s->speed;

    // bateu na parede ou no corpo de alguem
    if (outside_arena(s->x, s->y) || hits_body(s->x, s->y, idx, 7.0f)) {
        s->alive = 0;
        drop_body_as_food(s);
        return;
    }

    // guarda o rastro: novo ponto na frente do corpo
    if (s->npts > 0) {
        float dx = s->x - s->pts[0][0];
        float dy = s->y - s->pts[0][1];
        if (dx * dx + dy * dy >= SEG_SPACING * SEG_SPACING) {
            int max = s->target_len < MAX_BODY ? s->target_len : MAX_BODY - 1;
            int n = s->npts;
            if (n > max) n = max;
            memmove(&s->pts[1], &s->pts[0], sizeof(float) * 2 * n);
            s->pts[0][0] = s->x;
            s->pts[0][1] = s->y;
            s->npts = n + 1;
            if (s->npts > max) s->npts = max;
        }
    }

    // come comida
    for (int i = 0; i < MAX_FOOD; i++) {
        float dx = food[i].x - s->x;
        float dy = food[i].y - s->y;
        if (dx * dx + dy * dy < 18.0f * 18.0f) {
            s->target_len += GROW_PER_FOOD;
            s->score++;
            s->r = (s->r * 0.9f) + food[i].r * 0.1f;
            s->g = (s->g * 0.9f) + food[i].g * 0.1f;
            s->b = (s->b * 0.9f) + food[i].b * 0.1f;
            spawn_food(&food[i]);
        }
    }
}

// ---------- desenho (tiny3d, 2D em pixels) ----------

static void quad(float x, float y, float w, float h, float r, float g, float b, float a) {
    uint32_t c = ((uint32_t)(r * 255) << 24) | ((uint32_t)(g * 255) << 16) |
                 ((uint32_t)(b * 255) << 8)  | (uint32_t)(a * 255);
    tiny3d_VertexPos(x,     y,     0); tiny3d_VertexColor(c);
    tiny3d_VertexPos(x + w, y,     0); tiny3d_VertexColor(c);
    tiny3d_VertexPos(x + w, y + h, 0); tiny3d_VertexColor(c);
    tiny3d_VertexPos(x,     y + h, 0); tiny3d_VertexColor(c);
}

// circulo aproximado com 8 triangulos (cada um com 3 vertices)
static void circle_tri(float cx, float cy, float rad, float r, float g, float b, float a) {
    uint32_t c = ((uint32_t)(r * 255) << 24) | ((uint32_t)(g * 255) << 16) |
                 ((uint32_t)(b * 255) << 8)  | (uint32_t)(a * 255);
    const int seg = 8;
    for (int i = 0; i < seg; i++) {
        float a0 = (float)i * 2.0f * (float)M_PI / seg;
        float a1 = (float)(i + 1) * 2.0f * (float)M_PI / seg;
        tiny3d_VertexPos(cx, cy, 0); tiny3d_VertexColor(c);
        tiny3d_VertexPos(cx + cosf(a0) * rad, cy + sinf(a0) * rad, 0); tiny3d_VertexColor(c);
        tiny3d_VertexPos(cx + cosf(a1) * rad, cy + sinf(a1) * rad, 0); tiny3d_VertexColor(c);
    }
}

static void draw_arena(void) {
    // fundo escuro e borda
    quad(0, 0, SCREEN_W, SCREEN_H, 0.05f, 0.06f, 0.10f, 1.0f);
    quad(ARENA_MARGIN, ARENA_MARGIN, SCREEN_W - 2 * ARENA_MARGIN, SCREEN_H - 2 * ARENA_MARGIN,
         0.08f, 0.10f, 0.16f, 1.0f);
}

static void draw_food_batch(void) {
    for (int i = 0; i < MAX_FOOD; i++) {
        circle_tri(food[i].x, food[i].y, 5.0f, food[i].r, food[i].g, food[i].b, 1.0f);
    }
}

static void draw_snake_body(const Snake *s) {
    // corpo mais escuro no rabo, mais claro perto da cabeca
    for (int k = s->npts - 1; k >= 1; k -= 2) {
        float t = (float)k / (float)s->npts;     // 0 = cabeca, 1 = rabo
        float shade = 1.0f - 0.45f * t;
        float rad = 9.0f - 3.5f * t;             // corpo afila no rabo
        circle_tri(s->pts[k][0], s->pts[k][1], rad,
                   s->r * shade, s->g * shade, s->b * shade, 1.0f);
    }
}

static void draw_snake_head(const Snake *s) {
    circle_tri(s->x, s->y, 11.0f, s->r, s->g, s->b, 1.0f);
    // olhos
    float ex = cosf(s->angle), ey = sinf(s->angle);
    float px = -ey, py = ex;
    circle_tri(s->x + ex * 5 + px * 4, s->y + ey * 5 + py * 4, 3.0f, 1, 1, 1, 1);
    circle_tri(s->x + ex * 5 - px * 4, s->y + ey * 5 - py * 4, 3.0f, 1, 1, 1, 1);
    circle_tri(s->x + ex * 6 + px * 4, s->y + ey * 6 + py * 4, 1.4f, 0, 0, 0, 1);
    circle_tri(s->x + ex * 6 - px * 4, s->y + ey * 6 - py * 4, 1.4f, 0, 0, 0, 1);
}

// Menu sem fonte: barras coloridas (texto virá em versao futura)
static void draw_menu(void) {
    quad(0, 0, SCREEN_W, SCREEN_H, 0.05f, 0.06f, 0.10f, 1.0f);
    // titulo: 6 bolinhas verdes representando a cobra
    for (int i = 0; i < 6; i++) {
        circle_tri(440 + i * 60.0f, 170 + sinf(i * 0.8f) * 18.0f, 22.0f, 0.2f, 0.95f, 0.4f, 1.0f);
    }
    // botoes: JOGAR (verde) e SAIR (vermelho)
    quad(480, 360, 320, 70, menu_sel == 0 ? 0.2f : 0.12f, menu_sel == 0 ? 0.9f : 0.3f, 0.3f, 1.0f);
    quad(480, 460, 320, 70, menu_sel == 1 ? 0.9f : 0.5f, 0.2f, 0.2f, 1.0f);
}

static void draw_over(void) {
    quad(0, 0, SCREEN_W, SCREEN_H, 0.10f, 0.02f, 0.02f, 1.0f);
    quad(480, 360, 320, 70, 0.2f, 0.9f, 0.3f, 1.0f);   // X: jogar de novo
    quad(480, 460, 320, 70, 0.5f, 0.2f, 0.2f, 1.0f);   // O: menu
}

// ---------- entrada ----------

static void handle_menu_input(const padData *pad) {
    uint16_t btn  = pad->button[CELL_PAD_BTN_OFFSET_DIGITAL1];
    uint16_t btn2 = pad->button[CELL_PAD_BTN_OFFSET_DIGITAL2];
    if (btn & CELL_PAD_CTRL_UP)   menu_sel = 0;
    if (btn & CELL_PAD_CTRL_DOWN) menu_sel = 1;
    if (btn2 & CELL_PAD_CTRL_CROSS) {
        if (menu_sel == 0) { reset_game(); state = STATE_PLAY; }
        else exit_requested = 1;
    }
}

// ---------- main ----------

int main(void) {
    tiny3d_Init(1024 * 1024);
    ioPadInit(7);
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, sysutil_cb, NULL);
    srand(42);

    while (!exit_requested) {
        sysUtilCheckCallback();

        padInfo pinfo;
        padData pad;
        ioPadGetInfo(&pinfo);
        ioPadGetData(0, &pad);

        tiny3d_Clear(0xff000000, TINY3D_CLEAR_ALL);
        tiny3d_AlphaTest(1, 0x10, TINY3D_ALPHA_FUNC_GEQUAL);
        tiny3d_BlendFunc(1, TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
                         TINY3D_BLEND_FUNC_SRC_RGB_ONE_MINUS_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_ONE_MINUS_SRC_ALPHA,
                         TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);
        tiny3d_Project2D();

        switch (state) {
        case STATE_MENU:
            handle_menu_input(&pad);
            tiny3d_SetPolygon(TINY3D_QUADS);
            draw_menu();
            tiny3d_End();
            break;

        case STATE_PLAY: {
            uint16_t b1 = pad.button[CELL_PAD_BTN_OFFSET_DIGITAL1];
            if (b1 & CELL_PAD_CTRL_START) { state = STATE_MENU; break; }

            for (int i = 0; i < MAX_SNAKES; i++) update_snake(&snakes[i], i, &pad);

            if (!snakes[0].alive) state = STATE_OVER;

            // desenha tudo em um unico lote por tipo de primitiva
            tiny3d_SetPolygon(TINY3D_QUADS);
            draw_arena();
            tiny3d_End();

            tiny3d_SetPolygon(TINY3D_TRIANGLES);
            draw_food_batch();
            for (int i = 0; i < MAX_SNAKES; i++)
                if (snakes[i].alive) draw_snake_body(&snakes[i]);
            for (int i = 0; i < MAX_SNAKES; i++)
                if (snakes[i].alive) draw_snake_head(&snakes[i]);
            tiny3d_End();
            break;
        }

        case STATE_OVER:
            if (pad.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CROSS) {
                reset_game();
                state = STATE_PLAY;
            } else if (pad.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE) {
                state = STATE_MENU;
            }
            tiny3d_SetPolygon(TINY3D_QUADS);
            draw_over();
            tiny3d_End();
            break;
        }

        tiny3d_Flip();
    }

    ioPadEnd();
    return 0;
}