#include <psl1ght.h>
#include <tiny3d.h>
#include <io/pad.h>
#include <stdlib.h>
#include <math.h>

int main(){
    tiny3d_Init(1024*1024);
    padInfo padinfo;
    padData paddata;
    ioPadInit(7);

    int state = 0; // 0=menu 1=jogando
    int snakeX = 640, snakeY = 360;
    int dirX = 8, dirY = 0;
    int foodX = 300, foodY = 200;
    int score = 0;
    int frame = 0;

    int aiX[4] = {200, 1000, 400, 900};
    int aiY[4] = {150, 500, 600, 100};
    int aiDirX[4] = {5, -5, 4, -4};
    int aiDirY[4] = {3, -3, -4, 4};
    int aiColor[4] = {0xffff00ff, 0xff00ffffff, 0xffff00ff, 0xffff8000};

    while(1){
        ioPadGetInfo(&padinfo);
        if(padinfo.status[0]){
            ioPadGetData(0, &paddata);
            if(state==0 && paddata.BTN_CROSS) state=1;
            if(state==1){
                if(paddata.BTN_CROSS) break;
                if(paddata.BTN_UP) {dirX=0; dirY=-8;}
                if(paddata.BTN_DOWN) {dirX=0; dirY=8;}
                if(paddata.BTN_LEFT) {dirX=-8; dirY=0;}
                if(paddata.BTN_RIGHT) {dirX=8; dirY=0;}
                if(paddata.BTN_START) state=0;
            }
        }

        if(state==1){
            snakeX += dirX; snakeY += dirY;
            if(snakeX<0) snakeX=1280; if(snakeX>1280) snakeX=0;
            if(snakeY<0) snakeY=720; if(snakeY>720) snakeY=0;

            for(int i=0;i<4;i++){
                if(aiX[i] < foodX) aiDirX[i]=5; else aiDirX[i]=-5;
                if(aiY[i] < foodY) aiDirY[i]=4; else aiDirY[i]=-4;
                if(abs(aiX[i]-snakeX)<80 && abs(aiY[i]-snakeY)<80){
                    aiDirX[i] = -aiDirX[i]; aiDirY[i] = -aiDirY[i];
                }
                aiX[i]+=aiDirX[i]; aiY[i]+=aiDirY[i];
                if(aiX[i]<0) aiX[i]=1280; if(aiX[i]>1280) aiX[i]=0;
                if(aiY[i]<0) aiY[i]=720; if(aiY[i]>720) aiY[i]=0;
            }

            if(snakeX>foodX-20 && snakeX<foodX+20 && snakeY>foodY-20 && snakeY<foodY+20){
                score++;
                foodX = 40 + (rand()%1200);
                foodY = 40 + (rand()%640);
            }
        }

        tiny3d_Clear(0xff101010, TINY3D_CLEAR_ALL);
        tiny3d_Project2D();

        if(state==0){
            tiny3d_SetPolygon(TINY3D_QUADS);
            for(int i=0;i<10;i++){
                int mx = 300 + i*60;
                int my = 300 + (int)(sin(frame*0.05+i)*30);
                tiny3d_VertexPos(mx, my, 0); tiny3d_VertexColor(0xff00ff00);
                tiny3d_VertexPos(mx+40, my, 0); tiny3d_VertexColor(0xff00ff00);
                tiny3d_VertexPos(mx+40, my+40, 0); tiny3d_VertexColor(0xff00ff00);
                tiny3d_VertexPos(mx, my+40, 0); tiny3d_VertexColor(0xff00ff00);
            }
            tiny3d_End();
            tiny3d_SetPolygon(TINY3D_QUADS);
            tiny3d_VertexPos(540, 500, 0); tiny3d_VertexColor(0xffffffff);
            tiny3d_VertexPos(740, 500, 0); tiny3d_VertexColor(0xffffffff);
            tiny3d_VertexPos(740, 560, 0); tiny3d_VertexColor(0xffffffff);
            tiny3d_VertexPos(540, 560, 0); tiny3d_VertexColor(0xffffffff);
            tiny3d_End();
        } else {
            tiny3d_SetPolygon(TINY3D_QUADS);
            tiny3d_VertexPos(foodX, foodY, 0); tiny3d_VertexColor(0xff0000ff);
            tiny3d_VertexPos(foodX+20, foodY, 0); tiny3d_VertexColor(0xff0000ff);
            tiny3d_VertexPos(foodX+20, foodY+20, 0); tiny3d_VertexColor(0xff0000ff);
            tiny3d_VertexPos(foodX, foodY+20, 0); tiny3d_VertexColor(0xff0000ff);
            tiny3d_End();
            tiny3d_SetPolygon(TINY3D_QUADS);
            tiny3d_VertexPos(snakeX, snakeY, 0); tiny3d_VertexColor(0xff00ff00);
            tiny3d_VertexPos(snakeX+30, snakeY, 0); tiny3d_VertexColor(0xff00ff00);
            tiny3d_VertexPos(snakeX+30, snakeY+30, 0); tiny3d_VertexColor(0xff00ff00);
            tiny3d_VertexPos(snakeX, snakeY+30, 0); tiny3d_VertexColor(0xff00ff00);
            tiny3d_End();
            for(int i=0;i<4;i++){
                tiny3d_SetPolygon(TINY3D_QUADS);
                tiny3d_VertexPos(aiX[i], aiY[i], 0); tiny3d_VertexColor(aiColor[i]);
                tiny3d_VertexPos(aiX[i]+35, aiY[i], 0); tiny3d_VertexColor(aiColor[i]);
                tiny3d_VertexPos(aiX[i]+35, aiY[i]+35, 0); tiny3d_VertexColor(aiColor[i]);
                tiny3d_VertexPos(aiX[i], aiY[i]+35, 0); tiny3d_VertexColor(aiColor[i]);
                tiny3d_End();
            }
        }
        tiny3d_Flip();
        frame++;
        sysTimerUsleep(16000);
    }
    return 0;
}