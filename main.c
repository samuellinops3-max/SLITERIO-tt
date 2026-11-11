// Cobra simples - PS3 homebrew (tiny3d)
// D-pad esquerda/direita = virar | START = sair

#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <sysutil/sysutil.h>
#include <io/pad.h>
#include <tiny3d.h>

#define W 1280.0f
#define H 720.0f
#define MAX_PTS 200
#define SIZE 14.0f

static float px[MAX_PTS], py[MAX_PTS];
static int n = 20;
static float angle = 0.0f;
static float fx = 640, fy = 360;
static int exit_requested = 0;

static void sys_cb(uint64_t status, uint64_t param, void *userdata) {
    (void)param; (void)userdata;
    if (status == SYSUTIL_EVENT_MENU_CLOSE || status == SYSUTIL_EVENT_EXIT_GAME)
        exit_requested = 1;
}

static void square(float x, float y, float s, uint32_t c) {
    tiny3d_VertexPos(x,     y,     0); tiny3d_VertexColor(c);
    tiny3d_VertexPos(x + s, y,     0); tiny3d_VertexColor(c);
    tiny3d_VertexPos(x + s, y + s, 0); tiny3d_VertexColor(c);
    tiny3d_VertexPos(x,     y + s, 0); tiny3d_VertexColor(c);
}

int main(void) {
    tiny3d_Init(1024 * 1024);
    ioPadInit(7);
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, sys_cb, NULL);

    for (int i = 0; i < MAX_PTS; i++) { px[i] = 640; py[i] = 360; }

    while (!exit_requested) {
        sysUtilCheckCallback();
        padData pad;
        ioPadGetData(0, &pad);
        uint16_t b1 = pad.button[CELL_PAD_BTN_OFFSET_DIGITAL1];
        if (b1 & CELL_PAD_CTRL_START) break;
        if (b1 & CELL_PAD_CTRL_LEFT)  angle -= 0.06f;
        if (b1 & CELL_PAD_CTRL_RIGHT) angle += 0.06f;

        // move a cabeca e guarda o rastro
        for (int i = n - 1; i > 0; i--) { px[i] = px[i - 1]; py[i] = py[i - 1]; }
        px[0] += cosf(angle) * 4.0f;
        py[0] += sinf(angle) * 4.0f;

        // bateu na borda: volta do outro lado
        if (px[0] < 0) px[0] += W;
        if (px[0] > W) px[0] -= W;
        if (py[0] < 0) py[0] += H;
        if (py[0] > H) py[0] -= H;

        // comeu a comida: cresce e nova comida
        float dx = px[0] - fx, dy = py[0] - fy;
        if (dx * dx + dy * dy < 400.0f && n < MAX_PTS - 1) {
            n += 5;
            fx = 100 + rand() % 1000;
            fy = 100 + rand() % 500;
        }

        tiny3d_Clear(0xff000000, TINY3D_CLEAR_ALL);
        tiny3d_AlphaTest(0, 0, TINY3D_ALPHA_FUNC_ALWAYS);
        tiny3d_Project2D();

        tiny3d_SetPolygon(TINY3D_QUADS);
        square(fx - 8, fy - 8, 16, 0xffff4040);              // comida
        for (int i = 0; i < n; i++) {
            uint32_t c = (i == 0) ? 0xff40ff40 : 0xff20b020; // cabeca mais clara
            square(px[i] - SIZE / 2, py[i] - SIZE / 2, SIZE, c);
        }
        tiny3d_End();

        tiny3d_Flip();
    }

    ioPadEnd();
    return 0;
}