///Structure:DW-2
#define private public
//Base C++ Libraries
#include <string>
#include <iostream>
#include <cmath>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <thread>
#include <ctime>
#include <vector>

//Windows Libraries
#include <windows.h>
#include <windowsx.h>

//OpenGL Libraries
#include <GL/gl.h>

//aaaa0ggmc Libraries
#include <CClock.h>
#include <Translator.h>
#include <MultiEnString.h>
#include <spdlog.h>

//Resources
#include "rc.h"

#include "vmath.hpp"

#define DATA_CONFIG "data/config.cfg"
#define DATA_PATH "data/"

using namespace std;
using namespace cck;
using namespace alib;

struct GConfig{
    int err;
    float part_x;
    float part_y;
    float run_speed;
    int frame_limit;
    int vertices_limit;
    float scale_speed;
    float move_speed;
    float storing_limi;
    float scales;
    GConfig(){
        err = 0;
        part_x = part_y = 40;
        run_speed = 10;
        frame_limit = -1;
        vertices_limit = 256;
        scale_speed = 0.01;
        move_speed = 1;
        storing_limi = 2;
        scales = 1;
    }
};


void FlashScreen();
void HelpPage(const char * lanId);
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC);
void DisableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC);
void Display();
void UpdatingPainting();
void SetMenuText(HMENU menu,int command,LPSTR str);
int MultiLineTextOut(HDC pDC,int x,int y,const char* text,size_t,int LineSpace,TEXTMETRIC&);
void ConsoleDisplay();

///Logs//
LogSaver logSaver;
LogFactory l("FT",false,&logSaver);

//Communication between flash program & main
string tr_CommS = "";
atomic<int> tr_Progess = 0;

///Translations///
Translator ts;

GConfig gc;

bool criticalFlag = false;
bool errorFlag = false;

vector<Vector> points,point1;

//Windows
///main window-OpenGL runs here
HWND hwnd;
///sub window,controls
HWND subHWND;

const char * opener = "fft.math";

#define MENUITEM(X,V) const ULONG_PTR X = V
MENUITEM(mRepaint,1);
MENUITEM(mCircles,2);
MENUITEM(mReload,3);
MENUITEM(mStore,4);
MENUITEM(mOpenFileP,5);
MENUITEM(mLines,6);
MENUITEM(mPlay,7);
MENUITEM(mNextFrame,8);
MENUITEM(mChroma,9);
MENUITEM(mDisCoord,10);
MENUITEM(mDisVectors,11);
MENUITEM(mDisFVec,12);

bool stop = false;
bool calstop = false;
bool showCircles = false;
bool showLines = true;
bool nextFrame = false;
bool chroma = true;
bool discoord = true;
bool disvec = true;
bool disfvec = true;

//下一帧时间增幅倍数
unsigned int floating_scaling = 5;

char * outputbuf = NULL;

DWORD timeDL = 0;

bool rderr = false;

HMENU menu = CreateMenu();
HDC subDC;
HDC hDC;
HDC hsubDC = NULL;

HBITMAP bmp = NULL;

HBRUSH blkBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
BOOL bQuit = FALSE;
HGLRC hRC;

TEXTMETRIC metric;

atomic<float> elapseTime;
Clock loffset(false),fpsClk(false),clocks(false);

unsigned int frameGoes = 0;

Vector pos = {0,0,0,0};
Vector pdpart = {0,0,0,0};

int preFrameC = 0;
int mxFrame = 0;

float opx = 0,opy = 0;

#define MAX_INFO_SIZE 1024
#define MAX_INFO_SIZE_S "1024"
int main(int argc,char * argv[]){
    WNDCLASSEX wcex;
    MSG msg;

    outputbuf = new char[MAX_INFO_SIZE];
    ZeroMemory(outputbuf,sizeof(char) * MAX_INFO_SIZE);

    //Init LogSavers
    logSaver.SetFileOutput("data/log.txt");
    //Flash Window
    {
        thread tr(FlashScreen);
        tr.detach();
    }
    l.info("Loading Kernels...");
    //Load Translates
    atexit([]{
        ofstream ofs(DATA_CONFIG);
        if(ofs.bad())return;
        ofs << ts.Translate(ACCESS_TOKEN,"en_us").GetUTF8() << " " << logSaver.showlg;
        ofs.close();
        if(criticalFlag){
            MessageBox(NULL,"程序发生过致命错误！请查看data/log.txt检查错误！","CriticalErrors",MB_OK | MB_ICONERROR | MB_TOPMOST);
        }else if(errorFlag){
            MessageBox(NULL,"程序发生过错误！可查看data/log.txt检查错误！","Errors",MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST);
        }
        ///释放绘制时创建的资源
        if(bmp)DeleteBitmap(bmp);
        if(hsubDC)DeleteDC(hsubDC);
        delete [] outputbuf;
    });
    ts.LoadTranslateFiles(DATA_PATH);
    {
        //read statuses
        ifstream ifs(DATA_CONFIG);
        if(!ifs.bad()){
            string s = "";
            int flag = 0;
            ifs >> s >> flag;
            if(flag == 0){
                flag = LOG_RELE;
            }
            ifs.close();
            ts.LoadTranslate(s);
            logSaver.showLogs(flag);
        }else{
            l.error("Cannot load configuration  \"" DATA_CONFIG "\",but it does little harm.");
            errorFlag = true;
        }
    }
    tr_Progess = 40;
    {
        if(argc < 2){
            l.info("Given no arguments,we choose \"fft.math\" as a default file to load.");
        }else if(!strcmp(argv[2],"-help") || !strcmp(argv[2],"-h") || !strcmp(argv[2],"-?") || !strcmp(argv[2],"?")){
            HelpPage(ts.Translate(ACCESS_TOKEN,"en_us").GetUTF8().c_str());
        }else{
            opener = argv[2];
        }
        l.info("Loading Vectors...");
        points = readVectors(opener,gc.err,gc.part_x,gc.part_y,gc.run_speed,gc.vertices_limit,gc.frame_limit,gc.scale_speed,gc.move_speed,gc.storing_limi,tr_CommS);
        if(gc.err < 0){
            l.critical(errors);
            criticalFlag = true;
        }else {
            if(errors.compare("")){
                rderr = true;
                errors = string("无法正常加载文件，因为:\n") + errors;
                l.warn(errors);
            }
            string sx = "Loaded ";
            sx += to_string(gc.err);
            sx += " vectors in total.";
            l.info(sx);
        }
    }
    tr_Progess = 90;
    tr_CommS = "注册窗口,运行OpenGL...";
    {
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_OWNDC;
        wcex.lpfnWndProc = WindowProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = NULL;
        wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = "FT";
        wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

        if (!RegisterClassEx(&wcex)){
            tr_Progess = 100;
            l.critical("Failed to register class!Try to reload this application may help!");
            criticalFlag = true;
            exit(-1);
        }

        //Loading menus
        HMENU controls = CreatePopupMenu();
        HMENU view = CreatePopupMenu();
        HMENU stores = CreatePopupMenu();

        AppendMenu(menu,MF_POPUP,(UINT_PTR)controls,"控制");
        AppendMenu(menu,MF_POPUP,(UINT_PTR)view,"显示");
        AppendMenu(menu,MF_POPUP,(UINT_PTR)stores,"存储");

        //views
        AppendMenu(view,MF_CHECKED,mLines,"显示线段(Ctrl+L)");
        AppendMenu(view,MF_CHECKED,mCircles,"显示圆圈(Ctrl+O)");
        AppendMenu(view,MF_CHECKED,mDisCoord,"显示坐标系(Ctrl+C)");
        AppendMenu(view,MF_CHECKED,mDisVectors,"显示向量(Ctrl+V)");
        AppendMenu(view,MF_CHECKED,mDisFVec,"显示最终向量(Ctrl+F)");
        AppendMenu(view,MF_SEPARATOR,0,0);
        AppendMenu(view,MF_CHECKED,mChroma,"Chroma");

        //controls
        AppendMenu(controls,MF_STRING,mPlay,"停止(Ctrl+P)");
        AppendMenu(controls,MF_STRING,mNextFrame,"下一帧(Ctrl+N)");
        AppendMenu(controls,MF_SEPARATOR,0,0);
        AppendMenu(controls,MF_STRING,mRepaint,"重绘(Ctrl+Q)");
        AppendMenu(controls,MF_STRING,mReload,"重新加载(Ctrl+W)");

        //stores
        AppendMenu(stores,MF_STRING,mOpenFileP,"打开文件(Ctrl+O)");
        AppendMenu(stores,MF_STRING,mStore,"存储数据(Ctrl+S)");


        CheckMenuItem(menu,mCircles,MF_UNCHECKED);

        hwnd = CreateWindowEx(0,"FT","Look For FT",WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,CW_USEDEFAULT,CW_USEDEFAULT,
                              600,600,NULL,NULL,NULL,NULL);
        subHWND = CreateWindowEx(0,"FT","FT's console",WS_CAPTION | WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,
                              424,424,hwnd,menu,NULL,NULL);

        ShowWindow(hwnd,SW_SHOW);
        ShowWindow(subHWND,SW_SHOW);
    }

    subDC = GetDC(subHWND);
    ///创建双缓冲DC
    hsubDC = CreateCompatibleDC(subDC);
    bmp = CreateCompatibleBitmap(subDC,424,424);
    ///获取DC字体数据（未更新字体前）
    GetTextMetrics(hsubDC,&metric);
    ///启动线程
    thread updates(UpdatingPainting);
    thread paint(Display);
    thread console(ConsoleDisplay);

    loffset.Start();
    fpsClk.Start();
    clocks.Start();

    tr_Progess = 100;
    while (!bQuit){
        /* check for messages */
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
            /* handle or dispatch messages */
            if (msg.message == WM_QUIT){
                bQuit = TRUE;
            }else{
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }



    if(console.joinable())console.join();
    DestroyMenu(menu);
    DestroyWindow(subHWND);
    if(paint.joinable())paint.join();
    DestroyWindow(hwnd);
    if(updates.joinable())updates.join();

    return 0;
}

void HelpPage(const char * s){
    string composed = DATA_PATH "help_";
    composed += s;
    composed += ".html";
    ifstream ifs(composed);
    if(ifs.bad()){
        l.critical("Failed to open help page!!");
        criticalFlag = true;
    }else{
        string data = "\n";
        while(!ifs.eof()){
            getline(ifs,composed);
            data += composed;
            data += "\n";
        }
        l.info(data);
        ifs.close();
    }
}

#define SLP_TIME 20
void FlashScreen(){
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
    char * paintStr = (char *)malloc(sizeof(char) * 64);
    SetBkColor(desktopDC,RGB(0,0,0));
    SetTextColor(desktopDC,RGB(255,255,255));
    while(tr_Progess < 100){
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
        ZeroMemory(paintStr,sizeof(char) * 64);
        sprintf(paintStr,"(%d/%d) %.36s",tr_Progess.load(),100,tr_CommS.c_str());
        DrawIconEx(GetDC(NULL),x,y,ico,w,h,DI_NORMAL,NULL,DI_NORMAL);
        TextOut(desktopDC,x+10,y+10,paintStr,32);
        Sleep(SLP_TIME);
    }
    RedrawWindow(desktopWindow,&desktopRect,NULL,RDW_ALLCHILDREN);
}

void ToggleCheckedMenu(int mi,bool stat,HMENU m = menu){
    CheckMenuItem(m,mi,stat?MF_CHECKED:MF_UNCHECKED);
}

bool DealMenu(int id){
    switch(id){
    case mChroma:
        chroma = !chroma;
        ToggleCheckedMenu(mChroma,chroma);
        break;
    case mCircles:
        showCircles = !showCircles;
        ToggleCheckedMenu(mCircles,showCircles);
        break;
    case mLines:
        showLines = !showLines;
        ToggleCheckedMenu(mLines,showLines);
        break;
    case mDisCoord:
        discoord = !discoord;
        ToggleCheckedMenu(mDisCoord,discoord);
        break;
    case mDisVectors:
        disvec = !disvec;
        ToggleCheckedMenu(mDisVectors,disvec);
        break;
    case mDisFVec:
        disfvec = !disfvec;
        ToggleCheckedMenu(mDisFVec,disfvec);
        break;
    case mPlay:
        if(stop){
            clocks.Start();
        }else{
            timeDL += clocks.Stop().all;
        }
        stop = !stop;
        SetMenuText(menu,mPlay,stop?(LPSTR)"播放(Ctrl+P)":(LPSTR)"停止(Ctrl+P)");
        break;
    case mRepaint:
        calstop = true;
        point1.clear();
        for(unsigned int xt0 = 0;xt0 < points.size();xt0++){
            points[xt0].rotation = RadToDeg(points[xt0].orot);
        }
        calstop = false;
        break;
    case mReload:
        calstop = true;
        rderr = false;
        points.clear();
        point1.clear();
        points = readVectors(opener,gc.err,gc.part_x,gc.part_y,gc.run_speed,gc.vertices_limit,gc.frame_limit,gc.scale_speed,gc.move_speed,gc.storing_limi,tr_CommS);
        if(gc.err < 0){
            string dst = "Cannot open file \"";
            dst += opener;
            dst += "\"!";
            l.critical(dst);
            criticalFlag = true;
        }else{
            string dxt = "Loaded ";
            dxt += to_string(gc.err);
            dxt += " vectors in total.";
            l.info(dxt);
        }
        opx = gc.part_x;
        opy = gc.part_y;
        if(errors.compare("")){
            rderr = true;
            errors = string("无法正常加载文件，因为:\n") + errors;
            l.error(errors);
            errorFlag = true;
        }
        gc.part_x *= gc.scales;
        gc.part_y *= gc.scales;
        clocks.Stop();
        clocks.Start();
        calstop = false;
        break;
    case mNextFrame:
        nextFrame = true;
        break;
//    case :
//         = !;
//        ToggleCheckedMenu(,);
//        break;
    default:
        return false;
    }
    return true;
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    switch (uMsg){
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    case WM_COMMAND:{
        bool ret = DealMenu(LOWORD(wParam));
        if(ret)return 0;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC){
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

void DisableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC){
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}

#define calFPSFr 15
void ConsoleDisplay(){
    SimpFpsRestr restrictFps(30);
    while(!bQuit){
        ///Draw To Console
        SelectBitmap(hsubDC,bmp);
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
        SetTextColor(hsubDC,RGB(0,255,255));

        {
            int sz = sprintf(outputbuf,
                "fps:%d\n"
                "目前最高fps:%d\n"
                "现在的时间:%.2f ms = %.2f s\n"
                "点的坐标:(%f,%f)\n"
                "向量的数量:%d\n"
                "点的数量:%d\n"
                "\n%s%s",
                preFrameC,mxFrame,clocks.GetALLTime()+timeDL,
                (clocks.GetALLTime()+timeDL)/1000.0,
                point1[point1.size()-1].x,point1[point1.size()-1].y,
                points.size(),point1.size(),rderr?"错误被存储在了 data/log.txt 中\n":"",
                "PS:命令行里输入 -help或者查看data/help_zh_cn.txt 来查看帮助 :=)\n");

            MultiLineTextOut(hsubDC,5,32,outputbuf,sz,2,metric);

            {
                ///炫彩字体
                static string author = "Created by aaaa0ggmc(RockonDreaming)";
                static int uvalue = 0;
                if(chroma){
                    int stx = 5;
                    for(unsigned int idx = 0;idx < author.length();++idx){
                        SetTextColor(hsubDC,RGB(abs(sin(uvalue) + cos(idx)) * 200 + 55,abs(cos(uvalue) - sin(idx)) * 200 + 55,abs(sin(idx))) * 200 + 55);
                        TextOut(hsubDC,stx,5,&(author[idx]),1);
                        stx += metric.tmAveCharWidth;
                    }
                    ++uvalue;
                }else{
                    TextOut(hsubDC,5,5,&(author[0]),author.length());
                }
            }
        }
        BitBlt(subDC,0,0,424,424,hsubDC,0,0,SRCCOPY);
        restrictFps.sleep();
    }
}

#define PerSpec 100
void Display(){
    EnableOpenGL(hwnd, &hDC, &hRC);
    SimpFpsRestr rest(gc.frame_limit);
    while(!bQuit){
        float tx =0,ty = 0;
        frameGoes++;

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glPushMatrix();
//        glRotatef(theta, 0.0f, 1.0f, 0.0f);

        glColor3f(1.0,1.0,1.0);
        glBegin(GL_LINES);
            //Draw the line
            if(discoord){
                glVertex2f(pdpart.x,-1);
                glVertex2f(pdpart.x,1);
                glVertex2f(-1,pdpart.y);
                glVertex2f(1,pdpart.y);
            }
            //Draw the vectors trail
            Vector def = {0,0,0,0};
            def.x = pos.x;
            def.y = pos.y;

            if(showCircles){
                glColor3f(0.5f,0.5f,0.5f);
                float tLen = 0;
                for(unsigned int i = 0;i < points.size();i++){
                    tLen = points[i].Length();
                    glVertex2f((def.x+ tLen) / gc.part_x,def.y / gc.part_y);
                    for(unsigned int p = 0;p < (unsigned int)(PerSpec * tLen /2);p++){
                        tx = (tLen * cos(2*PI/(unsigned int)(PerSpec * tLen /2) * p) + def.x) / gc.part_x;
                        ty = (tLen * sin(2*PI/(unsigned int)(PerSpec * tLen /2) * p) + def.y) / gc.part_y;
                        glVertex2f(tx,ty);
                        glVertex2f(tx,ty);
                    }
                    glVertex2f((def.x + tLen) / gc.part_x,def.y / gc.part_y);
                    def.x += points[i].x;
                    def.y += points[i].y;
                }
            }
            //Draw the vectors
            if(disvec){
                glColor3f(1.0,1.0,0.0);
                def = {0,0,0,0};//temp = {0,0,0,0};
                def.x = pos.x;
                def.y = pos.y;
                glVertex2f(def.x / gc.part_x,def.y / gc.part_y);
                for(unsigned int i = 0;i < points.size();i++){
                    def.x += points[i].x;
                    def.y += points[i].y;
                    glVertex2f(def.x / gc.part_x,def.y / gc.part_y);
                    glVertex2f(def.x / gc.part_x,def.y / gc.part_y);

                }

                glVertex2f(def.x / gc.part_x,def.y / gc.part_y);
            }
            //Draw the main texture
            if(showLines){
                glColor3f(1.0,1.0,1.0);
                glVertex2f(point1[0].x/ gc.part_x + pdpart.x,point1[0].y/ gc.part_y + pdpart.y);
                for(unsigned int i = 0;i < point1.size();i++){
                    tx = point1[i].x / gc.part_x + pdpart.x;
                    ty = point1[i].y / gc.part_y + pdpart.y;
                    glVertex2f(tx,ty);
                    glVertex2f(tx,ty);
                }
            }
            //Draw Our Moving Line
            if(disfvec){
                glColor3f(1.0,1.0,1.0);
                glVertex2f(pdpart.x,pdpart.y);
                glVertex2f(point1[point1.size()-1].x / gc.part_x + pdpart.x,point1[point1.size()-1].y / gc.part_y + pdpart.y);
            }
        glEnd();

        glPopMatrix();
        SwapBuffers(hDC);
        if(nextFrame){
            nextFrame = !nextFrame;
            if(!clocks.isStop())timeDL += clocks.Stop().all;
            else timeDL += 10 * floating_scaling;
            stop = true;
            SetMenuText(menu,mPlay,(LPSTR)"播放(Ctrl+P)");
        }
        if(gc.frame_limit > 0){
            rest.sleep();
        }
        elapseTime = loffset.GetOffset();
    }
    DisableOpenGL(hwnd, hDC, hRC);
}

void UpdatingPainting(){
    SimpFpsRestr rest(120);
    while(!bQuit){
        ///calstop : stop multiple thread causes undefine behavior
        if((!stop && !calstop) || nextFrame){
            ///Calculating Painting Data
            Vector b = {0,0,0};
            for(unsigned int i = 0;i < points.size();i++){
                //Read Infinite
                //Multiply speed
                if(points[i].secperRound != RINFINITE)points[i].Rotate(((double)(clocks.GetALLTime() + timeDL) * gc.run_speed)/(double)1000);
                b.x += points[i].x;
                b.y += points[i].y;
            }
            point1.push_back(b);
            if(point1.size() > (unsigned int)(gc.vertices_limit))
                point1.erase(point1.begin());
        }
        rest.sleep();
    }
}

void SetMenuText(HMENU m,int c,LPSTR d){
    ModifyMenuA(m,c,MF_BYCOMMAND,c,d);
}

int MultiLineTextOut(HDC pDC,int x,int y,const char* text,size_t Length,int LineSpace,TEXTMETRIC & Metric){
    LineSpace += Metric.tmHeight;
    int Lines = 0;
    int Start = 0;
    int i = 0;
    for(;i < (int)Length; i++){
        if(text[i] == '\n'){
            Lines++;
            TextOut(pDC,x,y,&(text[Start]),i-Start);
            y += LineSpace;
            Start = i + 1;
        }
    }
    TextOut(pDC,x,y,&(text[Start]),i-Start);
    return Lines;
}
