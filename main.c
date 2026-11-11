#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <io/pad.h>
#include <vitasdk.h> // Compatibilidade de tipos básicos PSL1GHT
#include <rsx/rsx.h>
#include <sysutil/sysutil.h>

#define MAX_SNAKE_LENGTH 200
#define NUM_FOODS 30
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

typedef struct {
    float x, y;
} Point;

typedef struct {
    Point body[MAX_SNAKE_LENGTH];
    int length;
    float radius;
    float speed;
    float angle;
} Snake;

typedef struct {
    Point pos;
    uint32_t color;
    int active;
} Food;

// Inicialização básica do jogo
Snake player;
Food foods[NUM_FOODS];
int gameOver = 0;

void init_game() {
    player.length = 10;
    player.radius = 15.0f;
    player.speed = 4.0f;
    player.angle = 0.0f;
    
    // Posição inicial no centro da tela
    for(int i = 0; i < player.length; i++) {
        player.body[i].x = SCREEN_WIDTH / 2.0f;
        player.body[i].y = SCREEN_HEIGHT / 2.0f + (i * 10);
    }

    // Gerar comidas iniciais aleatórias
    for(int i = 0; i < NUM_FOODS; i++) {
        foods[i].pos.x = (rand() % (SCREEN_WIDTH - 100)) + 50;
        foods[i].pos.y = (rand() % (SCREEN_HEIGHT - 100)) + 50;
        foods[i].color = 0xFF00FF00; // Verde brilhante padrao
        foods[i].active = 1;
    }
}

void update_game(padData pad) {
    if (gameOver) return;

    // Captura o analógico esquerdo para direção de 360 graus
    float stick_x = (float)pad.ANA_L_X - 128.0f;
    float stick_y = (float)pad.ANA_L_Y - 128.0f;

    // Se o analógico se mover além da zona morta, muda o ângulo
    if (fabs(stick_x) > 20.0f || fabs(stick_y) > 20.0f) {
        player.angle = atan2f(stick_y, stick_x);
    }

    // Atualiza o corpo (cada parte segue a anterior)
    for (int i = player.length - 1; i > 0; i--) {
        player.body[i] = player.body[i - 1];
    }

    // Move a cabeça na direção do ângulo
    player.body[0].x += cosf(player.angle) * player.speed;
    player.body[0].y += sinf(player.angle) * player.speed;

    // Colisão com as bordas do mapa (Looping estilo Pacman ou morte)
    if (player.body[0].x < 0) player.body[0].x = SCREEN_WIDTH;
    if (player.body[0].x > SCREEN_WIDTH) player.body[0].x = 0;
    if (player.body[0].y < 0) player.body[0].y = SCREEN_HEIGHT;
    if (player.body[0].y > SCREEN_HEIGHT) player.body[0].y = 0;

    // Checar colisão com comida
    for (int i = 0; i < NUM_FOODS; i++) {
        if (foods[i].active) {
            float dx = player.body[0].x - foods[i].pos.x;
            float dy = player.body[0].y - foods[i].pos.y;
            float distance = sqrtf(dx*dx + dy*dy);

            if (distance < (player.radius + 8.0f)) {
                foods[i].active = 0;
                if (player.length < MAX_SNAKE_LENGTH) {
                    player.length++;
                }
                // Respawn da comida em outro lugar
                foods[i].pos.x = (rand() % (SCREEN_WIDTH - 100)) + 50;
                foods[i].pos.y = (rand() % (SCREEN_HEIGHT - 100)) + 50;
                foods[i].active = 1;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    padInfo padinfo;
    padData paddata;
    
    ioPadInit(7);
    init_game();

    // Loop principal do jogo para o PS3
    while(1) {
        ioPadGetInfo(&padinfo);
        if(padinfo.status[0]) {
            ioPadGetData(0, &paddata);
            
            // Se apertar START reinicia o jogo
            if(paddata.BTN_START) {
                init_game();
            }
            
            update_game(paddata);
        }
        
        // Aguarda o sincronismo vertical da TV (60 FPS estável)
        sysUtilCheckCallback();
    }

    ioPadEnd();
    return 0;
}
