#include <windows.h>
#include <math.h>
#include <cmath>
#include <vector>
#include <stdio.h>
#include <time.h>
#include <gl/gl.h>
#include <gL/glext.h>
#include <gL/glu.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <windowsx.h>
#include <pthread.h>
#include <string>
#include "vmath.hpp"
#include "res.h"

#include <CClock.h>
#include <Translator.h>
#include <MultiEnString.h>
#include <spdlog.h>

#define Block(X)
#define STOP_PLAY_BTN 1234
#define HIDE_SHOW_LINES 1235
#define HIDE_SHOW_CRICLES 1236
#define SLP_TIME 20
#define PerSpec 100
#define STORE_DATA 1237
#define REPAINT_PAGE 1238
#define MENU(X) ((HMENU)(X))
#define RELOAD_PAGE 1239
#define OPEN_LOADED_FILE 1240
#define OPEN_HELP_PAGE 1241
#define GetLimMs ((DWORD)(1000/(FRAME_LIMIT)))

using namespace std;
using namespace cck;
using namespace alib;

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND, HDC, HGLRC);
void * UpdatingPainting(void *);
void * FlushScreen(void *);
void PreInit();

int floating_scaling = 1;
bool nextFrame = false;
string ssStr = "";
bool flushing = true;
int calFPSFr = 15,FRAME_LIMIT = 1000,MXP = 1024,err = 0;
bool calstop = false;
float partx = 10,party = 10,opx,opy,speed = 1,scaleSpeed = 0.05,mvSp = 0.1;
float pxmpx,pympy;///pos.x / part.x && pos.y / part.y///
float scales = 1;
bool stop = false,showLines = true,showCircles = false;
float fenv = -1;
int store_status = 0;
bool rderr = false;

Translator ts;
LogSaver saver;
LogFactory l("FFT",true);


HWND subHWND = NULL,btn_tg = NULL,btn_tghd = NULL,btn_tgcr = NULL,btn_storeData = NULL,btn_rp = NULL,btn_rl = NULL,btn_of = NULL;
HWND go4help = NULL;
HDC subDC = NULL;
DWORD timeDL = 0;
BOOL bQuit = FALSE;

vector<Vector> point0,point1;
string errs = "";
Clock clocks(false);
Vector pos = {0,0};

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow){

    PreInit();
    saver.SetFileOutput(TRANSLATE_PATH "/log.txt");

    int preFrameC = 0,frameGoes = 0,mxFrame = 0;
    float tx,ty;

    WNDCLASSEX wcex;
    HWND hwnd;
    HDC hDC;
    HGLRC hRC;
    HBRUSH blkBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    MSG msg;
    pthread_t dt = 0;

    Clock fpsClk(false);

    ssStr = "开始读取向量数据.";
    l.info(ssStr);

    point0 = readVectors("fft.math",err,partx,party,speed,MXP,FRAME_LIMIT,scaleSpeed,mvSp,fenv,ssStr);

    fenv = (int)fenv;

    if(err){
        flushing = false;
        Sleep(30);
        MessageBox(NULL,"读取fft.math错误","Error",MB_TOPMOST | MB_ICONERROR | MB_OK);
        exit(EXIT_FAILURE);
    }

    ssStr = "读取成功";

    errs = GetErrors();
    opx = partx;
    opy = party;

    ssStr = "注册class.";

    Block(WCEX){
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_OWNDC;
        wcex.lpfnWndProc = WindowProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = "FFT";
        wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    }

    if (!RegisterClassEx(&wcex)){
        flushing = false;
        Sleep(30);
        MessageBox(NULL,"无法注册class","Error",MB_TOPMOST | MB_ICONERROR | MB_OK);
        return 0;
    }

    ssStr = "初始化窗口.";

    Block(InitWindowComponets){
        hwnd = CreateWindowEx(0,"FFT","Look For FFT",WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,CW_USEDEFAULT,CW_USEDEFAULT,
                              600,600,NULL,NULL,hInstance,NULL);
        subHWND = CreateWindowEx(0,"FFT","FFT's console",WS_CAPTION | WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,
                              424,424,hwnd,NULL,hInstance,NULL);
        btn_tg = CreateWindowEx(0,"BUTTON","Stop",WS_CHILD | WS_VISIBLE,184,316,
                              40,30,subHWND,MENU(STOP_PLAY_BTN),hInstance,NULL);
        btn_tghd = CreateWindowEx(0,
                              "BUTTON","HL",WS_CHILD | WS_VISIBLE,108,316,
                              40,30,subHWND,MENU(HIDE_SHOW_LINES),hInstance,NULL);
        btn_tgcr = CreateWindowEx(0,"BUTTON","SC",WS_CHILD | WS_VISIBLE,256,316,
                              40,30,subHWND,MENU(HIDE_SHOW_CRICLES),hInstance,NULL);
        btn_storeData = CreateWindowEx(0,"BUTTON","SD",WS_CHILD | WS_VISIBLE,360,5,
                              40,30,subHWND,MENU(STORE_DATA),hInstance,NULL);
        btn_storeData = CreateWindowEx(0,"BUTTON","RP",WS_CHILD | WS_VISIBLE,360,40,
                              40,30,subHWND,MENU(REPAINT_PAGE),hInstance,NULL);
        btn_rl = CreateWindowEx(0,"BUTTON","RL",WS_CHILD | WS_VISIBLE,360,75,
                              40,30,subHWND,MENU(RELOAD_PAGE),hInstance,NULL);
        btn_of = CreateWindowEx(0,"BUTTON","OF",WS_CHILD | WS_VISIBLE,360,105,
                              40,30,subHWND,MENU(OPEN_LOADED_FILE),hInstance,NULL);

        go4help = CreateWindowEx(0,"BUTTON","HP",WS_CHILD | WS_VISIBLE,360,140,
                              40,30,subHWND,MENU(OPEN_HELP_PAGE),hInstance,NULL);

        flushing = false;

        ShowWindow(hwnd, nCmdShow);
        ShowWindow(subHWND,nCmdShow);
    }

    subDC = GetDC(subHWND);

    EnableOpenGL(hwnd, &hDC, &hRC);

    //The calculating must run before catch events,or window will be paint in very bad///
    ///nope,the calculating process must run in multiple threads
    pthread_create(&dt,NULL,UpdatingPainting,NULL);
    pthread_detach(dt);
    if(errs.compare("")){
        rderr = true;
        ofstream ofs;
        ofs.open("fft.error");
        if(!ofs.good()){
            MessageBox(NULL,"无法写错误信息入fft.error中","Error",MB_TOPMOST | MB_ICONERROR | MB_OK);
            exit(2);
        }
        ofs << errs;
        ofs.close();
    }

    clocks.Start();
    fpsClk.Start();

    while (!bQuit)
    {
        /* check for messages */
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            /* handle or dispatch messages */
            if (msg.message == WM_QUIT)
            {
                pthread_cancel(dt);
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            Block(OpenGL Drawing){
                /* OpenGL animation code goes here */
                //Update frame count
                frameGoes++;

                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                glPushMatrix();
                //glRotatef(theta, 0.0f, 0.0f, 1.0f);

                glColor3f(1.0,1.0,1.0);
                glBegin(GL_LINES);
                    //Draw the line
                    glVertex2f(pxmpx,-1);
                    glVertex2f(pxmpx,1);
                    glVertex2f(-1,pympy);
                    glVertex2f(1,pympy);
                    //Draw the vectors trail
                    Vector def = {0,0,0,0};
                    def.x = pos.x;
                    def.y = pos.y;

                    if(showCircles){
                        glColor3f(0.5f,0.5f,0.5f);
                        float tLen = 0;
                        for(unsigned int i = 0;i < point0.size();i++){
                            tLen = point0[i].Length();
                            glVertex2f((def.x+ tLen) /partx,def.y / party);
                            for(unsigned int p = 0;p < (unsigned int)(PerSpec * tLen /2);p++){
                                tx = (tLen * cos(2*PI/(unsigned int)(PerSpec * tLen /2) * p) + def.x) / partx;
                                ty = (tLen * sin(2*PI/(unsigned int)(PerSpec * tLen /2) * p) + def.y) / party;
                                glVertex2f(tx,ty);
                                glVertex2f(tx,ty);
                            }
                            glVertex2f((def.x + tLen) / partx,def.y / party);
                            def.x += point0[i].x;
                            def.y += point0[i].y;
                        }
                    }
                    //Draw the vectors
                    glColor3f(1.0,1.0,0.0);
                    def = {0,0,0,0};//temp = {0,0,0,0};
                    def.x = pos.x;
                    def.y = pos.y;
                    glVertex2f(def.x / partx,def.y / party);
                    for(unsigned int i = 0;i < point0.size();i++){
                        def.x += point0[i].x;
                        def.y += point0[i].y;
                        glVertex2f(def.x / partx,def.y / party);
                        glVertex2f(def.x / partx,def.y / party);

                    }

                    glVertex2f(def.x / partx,def.y / party);
                    //Draw the main texture
                    glColor3f(1.0,1.0,1.0);
                    if(showLines){
                        string stored = "";
                        bool store = false;
                        if(store_status == 3){
                            store_status = 1;
                            store = true;
                        }
                        if(store){
                            if(fenv <= 0){
                                stored += "(" + to_string(point1[0].x) + "," + to_string(point1[0].y) + ")\n";
                            }else{
                                string fmtStr = string("(%.") + to_string((int)fenv) +
                                 string("f,%.") + to_string((int)fenv) + string("f)\n");
                                char * strfmtbuf = (char *)malloc(sizeof(char) * 1024);
                                ZeroMemory(strfmtbuf,sizeof(char) * 1024);
                                sprintf(strfmtbuf,fmtStr.c_str(),point1[0].x,point1[0].y);
                                stored += strfmtbuf;
                            }
                        }
                        glVertex2f(point1[0].x/partx + pxmpx,point1[0].y/party + pympy);
                        for(unsigned int i = 0;i < point1.size();i++){
                            tx = point1[i].x / partx + pxmpx;
                            ty = point1[i].y / party + pympy;
                            if(store){
                                if(fenv <= 0){
                                    stored += "(" + to_string(point1[i].x) + "," + to_string(point1[i].y) + ")\n";
                                }else{
                                    string fmtStr = string("(%.") + to_string((int)fenv) +
                                    string("f,%.") + to_string((int)fenv) + string("f)\n");
                                    char * strfmtbuf = (char *)malloc(sizeof(char) * 1024);
                                    ZeroMemory(strfmtbuf,sizeof(char) * 1024);
                                    sprintf(strfmtbuf,fmtStr.c_str(),point1[i].x,point1[i].y);
                                    stored += strfmtbuf;
                                }
                            }
                            glVertex2f(tx,ty);
                            glVertex2f(tx,ty);
                        }
                        ///Do processing
                        if(store){
                            ofstream ofs0;
                            ofs0.open("fft.sd");
                            if(ofs0.good()){
                                try{
                                    ofs0 << stored;
                                    ofs0.close();
                                }catch(std::exception *){
                                    store_status = 2;
                                }
                            }else{
                                store_status = 2;
                            }
                        }
                    }
                    //Draw Our Moving Line
                    glVertex2f(pxmpx,pympy);
                    glVertex2f(point1[point1.size()-1].x / partx + pxmpx,point1[point1.size()-1].y / party + pympy);
                glEnd();

                glPopMatrix();
                SwapBuffers(hDC);
                ///Draw To Console
                HDC hsubDC = CreateCompatibleDC(subDC);
                HBITMAP bmp = CreateCompatibleBitmap(subDC,424,424);
                SelectBitmap(hsubDC,bmp);
                int xt = 5;
                string str = "fps:";
                SetBkColor(hsubDC,RGB(0,0,0));
                SetTextColor(hsubDC,RGB(255,255,255));
                SelectBrush(hsubDC,blkBrush);
                Rectangle(hsubDC,0,0,424,424);
                if(frameGoes >= calFPSFr){
                    int off = 0;
                    off = fpsClk.GetOffset();
                    if(off != 0){
                        preFrameC = (int)(1000 / off) * frameGoes;
                        if(preFrameC > mxFrame){
                            mxFrame = preFrameC;
                        }
                    }
                    frameGoes = 0;
                }
                SetBkMode(hsubDC,TRANSPARENT);
                SetTextColor(hsubDC,RGB(255,255,255));
                str += to_string(preFrameC);
                TextOut(hsubDC,5,xt,str.c_str(),str.length());
                xt += 20;
                str = "目前最高的fps:" + to_string(mxFrame);
                TextOut(hsubDC,5,xt,str.c_str(),str.length());
                xt += 20;
                str = "现在的时间:" + to_string(clocks.GetALLTime()+timeDL) + "ms/" + to_string(((float)(clocks.GetALLTime() + timeDL) /(float)1000)) + "s";
                TextOut(hsubDC,5,xt,str.c_str(),str.length());
                xt += 20;
                str = "点的坐标:[" + to_string(point1[point1.size()-1].x) + "," + to_string(point1[point1.size()-1].y) + "]";
                TextOut(hsubDC,5,xt,str.c_str(),str.length());
                xt += 20;
                str = "向量数量:" + to_string(point0.size());
                TextOut(hsubDC,5,xt,str.c_str(),str.length());
                xt += 20;
                str = "点的数量:" + to_string(point1.size());
                TextOut(hsubDC,5,xt,str.c_str(),str.length());
                xt += 20;
                if(rderr){
                    str = "错误被存储在了 [fft.error].";
                    TextOut(hsubDC,5,xt,str.c_str(),str.length());
                    xt += 20;
                }
                str = "PS:SL代表显示出点围成的图案,HL反之.:=)";
                TextOut(hsubDC,5,xt,str.c_str(),str.length());
                xt += 20;
                str = "PS:SC代表显示出向量的旋转圆圈,HC反之.:=)";
                TextOut(hsubDC,5,xt,str.c_str(),str.length());
                xt += 20;
                str = "PS:SD:存储这一轮的点数据 RP:重画 RL:重新加载";
                TextOut(hsubDC,5,xt,str.c_str(),str.length());
                xt += 20;
                str = "PS:按'+','-'缩放,wasd(或 ↑ ← ↓ →)移动 ,esc退出.";
                TextOut(hsubDC,5,xt,str.c_str(),str.length());
                xt += 20;
                if(!store_status){
                    str = "还没存数据呢！";
                }else if(store_status == 1){
                    str = "数据存储在了[fff.sd].";
                }else if(store_status == 2){
                    str = "数据存储失败！";
                }
                TextOut(hsubDC,5,xt,str.c_str(),str.length());
                xt += 40;
                str = "aaaa0ggmc(RockonDreaming) 制作";
                TextOut(hsubDC,5,xt,str.c_str(),str.length());
                BitBlt(subDC,0,0,424,424,hsubDC,0,0,SRCCOPY);
                DeleteDC(hsubDC);
                DeleteBitmap(bmp);
                if(nextFrame){
                    nextFrame = !nextFrame;
                    if(!clocks.isStop())timeDL += clocks.Stop().all;
                    else timeDL += 10 * floating_scaling;
                    SetWindowText(btn_tg,"Play");
                    stop = true;
                }
                if(GetLimMs){//If GetLimMs not eqauls 0,then we sleep,else we do nothing
                    Sleep(GetLimMs);
                    ///Why do this
                    ///Cause in windows Sleep(0) means you throw the rest of the calculating time///
                    ///So don't sleep can cause more fps///
                }
            }
        }
    }

    /* shutdown OpenGL */
    DisableOpenGL(hwnd, hDC, hRC);

    /* destroy the window explicitly */
    DestroyWindow(hwnd);

    ///保存数据
    ofstream ofs(CONFIG_PATH);
    if(!ofs.bad()){
        ofs << ts.Translate(VERIFY_TOKEN,"en_us").GetUTF8();
    }
    ofs.close();

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WORD wmId;
    switch (uMsg)
    {
        case WM_CLOSE:
            PostQuitMessage(0);
        break;
        case WM_COMMAND:
            wmId = LOWORD(wParam);
            switch(wmId){
            case STOP_PLAY_BTN:
                if(stop){
                    clocks.Start();
                    SetWindowText(btn_tg,"Stop");
                }else{
                    timeDL += clocks.Stop().all;
                    SetWindowText(btn_tg,"Play");
                }
                stop = !stop;
                break;
            case OPEN_LOADED_FILE:
                system("start fft.math");
                break;
            case OPEN_HELP_PAGE:
                system("start data\\help.html");
                break;
            case HIDE_SHOW_LINES:
                showLines = !showLines;
                if(showLines){
                    SetWindowText(btn_tghd,"HL");
                }else{
                    SetWindowText(btn_tghd,"SL");
                }
                break;
            case HIDE_SHOW_CRICLES:
                showCircles = !showCircles;
                if(showCircles){
                    SetWindowText(btn_tgcr,"HC");
                }else{
                    SetWindowText(btn_tgcr,"SC");
                }
                break;
            case STORE_DATA:
                store_status = 3;//Storing
                break;
            case REPAINT_PAGE:
                calstop = true;
                point1.clear();
                for(unsigned int xt0 = 0;xt0 < point0.size();xt0++){
                    point0[xt0].rotation = RadToDeg(point0[xt0].orot);
                }
                calstop = false;
                break;
            case RELOAD_PAGE:
                calstop = true;
                rderr = false;
                point0.clear();
                point1.clear();
                point0 = readVectors("fft.math",err,partx,party,speed,MXP,FRAME_LIMIT,scaleSpeed,mvSp,fenv,ssStr);
                if(err){
                    MessageBox(NULL,"读取fft.math错误","Error",MB_TOPMOST | MB_ICONERROR | MB_OK);
                    exit(EXIT_FAILURE);
                }
                errs = GetErrors();
                opx = partx;
                opy = party;
                if(errs.compare("")){
                    rderr = true;
                    ofstream ofs;
                    ofs.open("fft.error");
                    if(!ofs.good()){
                        MessageBox(NULL,"无法写错误信息入fft.error中","Error",MB_TOPMOST | MB_ICONERROR | MB_OK);
                        exit(2);
                    }
                    ofs << errs;
                    ofs.close();
                }
                partx *= scales;
                party *= scales;
                clocks.Stop();
                clocks.Start();
                calstop = false;
                break;
            default:
                break;
            }
            SetFocus(subHWND);
        case WM_DESTROY:
            return 0;
        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case '1':
                    floating_scaling = 1;
                    break;
                case '2':
                    floating_scaling = 2;
                    break;
                case '3':
                    floating_scaling = 3;
                    break;
                case '4':
                    floating_scaling = 4;
                    break;
                case '5':
                    floating_scaling = 5;
                    break;
                case '6':
                    floating_scaling = 6;
                    break;
                case '7':
                    floating_scaling = 7;
                    break;
                case '8':
                    floating_scaling = 8;
                    break;
                case '9':
                    floating_scaling = 9;
                    break;
                case 'N':
                    nextFrame = true;
                    break;
                case VK_SPACE:
                   if(stop){
                        clocks.Start();
                        SetWindowText(btn_tg,"Stop");
                    }else{
                        timeDL += clocks.Stop().all;
                        SetWindowText(btn_tg,"Play");
                    }
                    stop = !stop;
                    break;
                case VK_ESCAPE:
                    PostQuitMessage(0);
                    break;
                case VK_ADD:
                    ///use multiply else not be a round///
                    scales -= scaleSpeed;
                    if(scales <= 0)scales = 1;
                    partx = opx * scales;
                    party = opy * scales;
                    break;
                case VK_SUBTRACT:
                    scales += scaleSpeed;
                    if(scales <= 0)scales = 1;
                    partx = opx * scales;
                    party = opy * scales;
                    break;
                case VK_LEFT:
                case VK_HOME:
                    pos.x += mvSp;
                    break;
                case VK_UP:
                    pos.y -= mvSp;
                    break;
                case VK_DOWN:
                    pos.y += mvSp;
                    break;
                case VK_RIGHT:
                case VK_END:
                    pos.x -= mvSp;
                    break;
                case 229:
                    switch(lParam){
                        case 1074593793://long press
                        case 851969://'+'
                            scales -= scaleSpeed;
                            if(scales <= 0)scales = 1;
                            partx = opx * scales;
                            party = opy * scales;
                            break;
                        case 1074528257:
                        case 786433://'-'
                            scales += scaleSpeed;
                            if(scales <= 0)scales = 1;
                            partx = opx * scales;
                            party = opy * scales;
                            break;
                        case 1074855937://lp
                        case 1114113://w
                            pos.y -= mvSp;
                            break;
                        case 1075707905://lp
                        case 1966081://a
                            pos.x += mvSp;
                            break;
                        case 1075773441://lp
                        case 2031617://s
                            pos.y += mvSp;
                            break;
                        case 1075838977://lp
                        case 2097153://d
                            pos.x -= mvSp;
                            break;
                        default:
                            break;
                    }
                    break;
                case 187:
                    scales -= scaleSpeed;
                    if(scales <= 0)scales = 1;
                    partx = opx * scales;
                    party = opy * scales;
                    break;
                case 189:
                    scales += scaleSpeed;
                    if(scales <= 0)scales = 1;
                    partx = opx * scales;
                    party = opy * scales;
                    break;
                case 87://w
                    pos.y -= mvSp;
                    break;
                case 65://a
                    pos.x += mvSp;
                    break;
                case 83://s
                    pos.y += mvSp;
                    break;
                case 68://d
                    pos.x -= mvSp;
                    break;
            }
            pxmpx = pos.x / partx;
            pympy = pos.y / party;
        }
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC)
{
    PIXELFORMATDESCRIPTOR pfd;

    int iFormat;

    /* get the device context (DC) */
    *hDC = GetDC(hwnd);

    /* set the pixel format for the DC */
    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW |
                  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iFormat = ChoosePixelFormat(*hDC, &pfd);

    SetPixelFormat(*hDC, iFormat, &pfd);

    /* create and enable the render context (RC) */
    *hRC = wglCreateContext(*hDC);

    wglMakeCurrent(*hDC, *hRC);
}

void DisableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}

void * UpdatingPainting(void *){
    while(!bQuit){
        ///calstop : stop multiple thread causes undefine behavior
        if((!stop && !calstop) || nextFrame){
            ///Calculating Painting Data
            Vector b = {0,0,0};
            for(unsigned int i = 0;i < point0.size();i++){
                //Read Infinite
                //Multiply speed
                if(point0[i].secperRound != RINFINITE)point0[i].Rotate(((double)(clocks.GetALLTime() + timeDL) * speed)/(double)1000);
                b.x += point0[i].x;
                b.y += point0[i].y;
            }
            point1.push_back(b);
            if(point1.size() > (unsigned int)(MXP))
                point1.erase(point1.begin());
        }
        pthread_testcancel();
        Sleep(GetLimMs + 4);
    }
    return NULL;
}

void PreInit(){
    pthread_t flushT = 0;
    pthread_create(&flushT,NULL,FlushScreen,NULL);
    pthread_detach(flushT);
    ///加载翻译
    ts.LoadTranslateFiles(TRANSLATE_PATH);
    ///读取config
    ifstream config(CONFIG_PATH);
    if(!config.bad()){
        string s = "";
        config >> s;
        ts.LoadTranslate(s);
        config.close();
    }else ts.LoadTranslate("en_us");
}

void * FlushScreen(void *){
    HICON ico = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(MAIN_ICON));
    DWORD w = 0,h = 0;
    DWORD x = 0,y = 0;
    HDC desktopDC = GetDC(NULL);
    HWND desktopWindow = WindowFromDC(desktopDC);
    RECT desktopRect = {0,0,0,0};
    Clock clk_time;
    Clock updatePos;

    GetWindowRect(desktopWindow,&desktopRect);

    h = (desktopRect.bottom - desktopRect.top) / 5 * 3;
    w = h;

    x = ((desktopRect.right - desktopRect.left) - w)/2;
    y = ((desktopRect.bottom - desktopRect.top) - h)/2;

    bool ans = true;

    char * paintStr = (char *)malloc(sizeof(char) * 24);

    SetBkColor(desktopDC,RGB(0,0,0));

    SetTextColor(desktopDC,RGB(255,255,255));

    while(flushing){
        if(updatePos.Now().offset > 1000){
            updatePos.GetOffset();
            GetWindowRect(desktopWindow,&desktopRect);
            h = (desktopRect.bottom - desktopRect.top) / 5 * 3;
            w = h;
            x = ((desktopRect.right - desktopRect.left) - w)/2;
            y = ((desktopRect.bottom - desktopRect.top) - h)/2;
        }
        if(clk_time.GetALLTime() > 16000 && ans){
            int an = MessageBox(NULL,"向量似乎有点多是否退出","提示",MB_TOPMOST | MB_ICONQUESTION | MB_YESNO);
            if(an == IDYES){
                RedrawWindow(desktopWindow,&desktopRect,NULL,RDW_ALLCHILDREN);
                exit(-1);
            }else{
                MessageBox(NULL,"可随时按下ESC退出！","提示",MB_TOPMOST | MB_OK);
            }
            ans = false;
        }
        if(GetAsyncKeyState(VK_ESCAPE) & 0x8000){
            RedrawWindow(desktopWindow,&desktopRect,NULL,RDW_ALLCHILDREN);
            exit(-1);
        }
        ZeroMemory(paintStr,sizeof(char) * 24);
        sprintf(paintStr,"%.24s",ssStr.c_str());
        DrawIconEx(GetDC(NULL),x,y,ico,w,h,DI_NORMAL,NULL,DI_NORMAL);
        TextOut(desktopDC,x+10,y+10,paintStr,24);
        Sleep(SLP_TIME);
    }

    RedrawWindow(desktopWindow,&desktopRect,NULL,RDW_ALLCHILDREN);

    return NULL;
}
