///Structure:SEH
//Base C++ Libraries
#include <iostream>
#include <filesystem>
#include <cmath>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <thread>
#include <ctime>
#include <vector>
#include <string>

#include <stdlib.h>

//Windows Libraries
#include <windows.h>
#include <windowsx.h>

//OpenGL Libraries
#include <GL/gl.h>

//aaaa0ggmc Libraries
#include <alib-g3/aclock.h>
#include <alib-g3/atranslator.h>
#include <alib-g3/alogger.h>
#include <alib-g3/adata.h>

#include "vmath.hpp"

#define DATA_CONFIG "data/config.toml"
#define DATA_DISPLAY "data/display.toml"
#define DATA_PATH "data/"
#define DATA_TRANSLATIONS "data/translations"
#define DATA_OUTPUT "data/out.txt"

using namespace std;
using namespace alib::g3;

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
    float oneeqw;
    GConfig(){
        err = 0;
        part_x = part_y = 40;
        run_speed = 10;
        frame_limit = -1;
        vertices_limit = 256;
        scale_speed = 3;
        move_speed = 4;
        storing_limi = 2;
        scales = 1;
        oneeqw = 1;
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
bool DealMenu(int id);
int OpenFileS();
void loadDisplayConfig();

///Logs//
Logger logSaver;
LogFactory l("FT",logSaver);

//Communication between flash program & main
string tr_CommS = "";
atomic<int> tr_Progess = 0;

///Translations///
Translator ts;

GConfig gc;

bool criticalFlag = false;
bool errorFlag = false;

vector<Vector> points,point1;

string lastDir;
string consoleStr = "";
string errStr = "";

//Windows
///main window-OpenGL runs here
HWND hwnd;
///sub window,controls
HWND subHWND;

string opener = "fft.math";

#define MENUITEM(X,V) const ULONG_PTR X = V
MENUITEM(mRepaint,1);
MENUITEM(mCircles,2);
MENUITEM(mReload,3);
MENUITEM(mStore,4);
MENUITEM(mOpenFileP,5);
MENUITEM(mLines,6);
MENUITEM(mPlay,7);
MENUITEM(mChroma,9);
MENUITEM(mDisCoord,10);
MENUITEM(mDisVectors,11);
MENUITEM(mDisFVec,12);
MENUITEM(mNew,13);
MENUITEM(mHelp,14);
MENUITEM(mNeon,15);
MENUITEM(mFollow,16);
MENUITEM(mBackO,17);
MENUITEM(mDConfig,18);
MENUITEM(mAutoCfg,19);
MENUITEM(mAutoDis,20);

bool stop = false;
bool calstop = false;
bool showCircles = false;
bool showLines = true;
bool nextFrame = false;
bool chroma = true;
bool discoord = true;
bool disvec = true;
bool disfvec = true;
bool neon = false;
bool follow = false;
bool threed = false;
bool focus;
bool autoCfg = false;
bool autoDis = true;

//下一帧时间增幅倍数
unsigned int floating_scaling = 5;

char * outputbuf = NULL;

bool rderr = false;

HMENU menu = CreateMenu();
HDC subDC;
HDC hDC;
HDC hsubDC = NULL;

HBITMAP bmp = NULL;

HBRUSH blkBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
BOOL bQuit = FALSE;
HGLRC hRC;

//Loading menus
HMENU controls = CreatePopupMenu();
HMENU view = CreatePopupMenu();
HMENU stores = CreatePopupMenu();

TEXTMETRIC metric;

atomic<float> elapseTime;
Clock loffset(false),fpsClk(false);

unsigned int frameGoes = 0;

Vector pos = {0,0,0,0};
Vector pdpart = {0,0,0,0};

int preFrameC = 0;
int mxFrame = 0;

float opx = 0,opy = 0;
float timeCur;

filesystem::file_time_type lw_display,lw_config;

Clock cneon;

void ToggleCheckedMenu(int mi,bool stat,HMENU m = menu);
void updateConfigLastWrite();

#define MAX_INFO_SIZE 1024
#define MAX_INFO_SIZE_S "1024"
int main(int argc,char * argv[]){
    WNDCLASSEX wcex;
    MSG msg;

    srand(time(0));

    auto tconsole = std::make_shared<lot::Console>();
    auto tfile = std::make_shared<lot::SingleFile>();

    outputbuf = new char[MAX_INFO_SIZE];
    ZeroMemory(outputbuf,sizeof(char) * MAX_INFO_SIZE);

    //Init LogSavers
    logSaver.appendLogOutputTarget("console",tconsole);
    logSaver.appendLogOutputTarget("file",tfile);
    //Flash Window
    {
        thread tr(FlashScreen);
        tr.detach();
    }
    l.info("Loading Kernels...");
    //Load Translates
    atexit([]{
        #define bool2str(x) ((x)?"true":"false")
        ofstream ofs(DATA_CONFIG);
        if(ofs.bad())return;
        ofs << "language_id = \"" << ts.translate_def(ALIB_DEF_ACCESS,"en_us") << "\"\n";
        ofs << "log_level =" << logSaver.getLogVisibilities() << "\n";
        ofs << "follow = " << bool2str(follow) << "\n";
        ofs << "neon = " << bool2str(neon) << "\n";
        ofs << "chroma = " << bool2str(chroma) << "\n";
        ofs << "lastDir = \"" << lastDir << "\"\n";
        ofs << "threeDimension = " << bool2str(threed) << "\n";
        ofs << "autoLoadConfig = " << bool2str(autoCfg) << "\n";
        ofs << "autoLoadDisplayConfig = " << bool2str(autoDis) << "\n";
        ofs << "[display]" << "\n";
        ofs << "finalVector = " << bool2str(disfvec) << "\n";
        ofs << "vectors = " << bool2str(disvec) << "\n";
        ofs << "coordinates = " << bool2str(discoord) << "\n";
        ofs << "rotatingCircles = " << bool2str(showCircles) << "\n";
        ofs << "track = " << bool2str(showLines);
        ofs.close();
        if(criticalFlag){
            MessageBox(NULL,ts.translate_def("critical_error","程序发生过致命错误！请查看data/log.txt检查错误！").c_str(),"CriticalError",MB_OK | MB_ICONERROR | MB_TOPMOST);
        }else if(errorFlag){
            MessageBox(NULL,ts.translate_def("error","程序发生过错误！可查看data/log.txt检查错误！").c_str(),"Error",MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST);
        }
        ///释放绘制时创建的资源
        if(bmp)DeleteBitmap(bmp);
        if(hsubDC)DeleteDC(hsubDC);
        delete [] outputbuf;
    });

    ts.readTranslationFiles(DATA_TRANSLATIONS);
    l(LOG_INFO) << "Loaded translates,count:" << (int)ts.translations.size() << endlog;
    {
        GDoc doc;
        int ret = doc.read_parseFileTOML(DATA_CONFIG);
        if(ret){
            l.error("Cannot load configuration  \"" DATA_CONFIG "\",but it does little harm.");
            errorFlag = true;
        }else{
            #define boolCheck(s) ((!(s))?0:(strcmp(*s,"1")?0:1))
            #define value_unfold(v,def,METHOD) ((!(v))?(def):(METHOD(*v)))
            auto value = doc.get("language_id");
            ts.loadTranslation(value_unfold(value,"en_us",));
            value = doc.get("log_level");
            logSaver.setLogVisibilities(value_unfold(value,LOG_FULL,atoi));
            value = doc.get("lastDir");
            lastDir = value_unfold(value,"",);
            follow = boolCheck(doc.get("follow"));
            chroma = boolCheck(doc.get("chroma"));
            neon = boolCheck(doc.get("neon"));
            disvec = boolCheck(doc.get("display.vectors"));
            discoord = boolCheck(doc.get("display.coordinates"));
            showCircles = boolCheck(doc.get("display.rotatingCircles"));
            showLines = boolCheck(doc.get("display.track"));
            disfvec = boolCheck(doc.get("display.finalVector"));
            threed = boolCheck(doc.get("threeDimension"));
            autoDis = boolCheck(doc.get("autoLoadDisplayConfig"));
            autoCfg = boolCheck(doc.get("autoLoadConfig"));
        }
        loadDisplayConfig();
        gc.oneeqw = 1/gc.scales;
        opx = gc.part_x;
        opy = gc.part_y;
    }
    //toml++ auto unescapes
    consoleStr = ts.translate_def("consoleStr",
                            "fps:%d\n"
                            "目前最高fps:%d\n"
                            "现在的时间:%.2f ms = %.2f s\n"
                            "点的坐标:(%f,%f)\n"
                            "向量的数量:%llu\n"
                            "点的数量:%llu\n"
                            "\n%s");
    errStr = ts.translate_def("errStr","错误被存储在了 data/log.txt 中\n");
    tr_Progess = 40;
    l.info("Loading files...");
    {
        if(argc < 2){
            l.info("Given no arguments,we choose \"fft.math\" as a default file to load.");
        }else if(!strcmp(argv[1],"-help") || !strcmp(argv[1],"-h") || !strcmp(argv[1],"-?") || !strcmp(argv[1],"?")){
            HelpPage(ts.translate_def(ALIB_DEF_ACCESS,"en_us").c_str());
        }else{
            opener = argv[1];
        }
        l.info("Loading Vectors...");
        points = readVectors(opener,gc.err,gc.part_x,gc.part_y,gc.run_speed,gc.vertices_limit,gc.frame_limit,gc.scale_speed,gc.move_speed,gc.storing_limi,tr_CommS);
        if(gc.err < 0){
            l.critical(errors);
            criticalFlag = true;
        }else {
            if(errors.compare("")){
                rderr = true;
                errors = string("Failed to load files normally,because:\n") + errors;
                l.warn(errors);
            }
            l() << "Loaded " << gc.err << " vectors in total." << endlog;
        }
        gc.oneeqw = 1/gc.scales;
        opx = gc.part_x;
        opy = gc.part_y;
    }
    tr_Progess = 90;
    tr_CommS = ts.translate_def("load0","注册窗口,运行OpenGL......");
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

        AppendMenu(menu,MF_POPUP,(UINT_PTR)controls,ts.translate_def("menu.control","控制").c_str());
        AppendMenu(menu,MF_POPUP,(UINT_PTR)view,ts.translate_def("menu.display","显示").c_str());
        AppendMenu(menu,MF_POPUP,(UINT_PTR)stores,ts.translate_def("menu.storage","存储").c_str());
        AppendMenu(menu,MF_STRING,mHelp,ts.translate_def("menu.help","帮助").c_str());

        //views
        AppendMenu(view,MF_CHECKED,mLines,ts.translate_def("menu.displayLines","轨迹(L)").c_str());
        ToggleCheckedMenu(mLines,showLines);
        AppendMenu(view,MF_CHECKED,mCircles,ts.translate_def("menu.displayCircles","圆圈(O)").c_str());
        ToggleCheckedMenu(mCircles,showCircles);
        AppendMenu(view,MF_CHECKED,mDisCoord,ts.translate_def("menu.displayCoordinates","坐标系(C)").c_str());
        ToggleCheckedMenu(mDisCoord,discoord);
        AppendMenu(view,MF_CHECKED,mDisVectors,ts.translate_def("menu.displayVectors","向量(V)").c_str());
        ToggleCheckedMenu(mDisVectors,disvec);
        AppendMenu(view,MF_CHECKED,mDisFVec,ts.translate_def("menu.displayFinalVector","最终向量(F)").c_str());
        ToggleCheckedMenu(mDisFVec,disfvec);
        AppendMenu(view,MF_SEPARATOR,0,0);
        AppendMenu(view,MF_CHECKED,mChroma,ts.translate_def("menu.chroma","炫彩文字(M)").c_str());
        AppendMenu(view,MF_CHECKED,mNeon,ts.translate_def("menu.neon","炫彩线段").c_str());
        AppendMenu(view,MF_CHECKED,mFollow,ts.translate_def("menu.follow","跟随").c_str());
        ToggleCheckedMenu(mChroma,chroma);
        ToggleCheckedMenu(mNeon,neon);
        ToggleCheckedMenu(mFollow,follow);

        //controls
        AppendMenu(controls,MF_STRING,mPlay,ts.translate_def("menu.controlPause","暂停(空格)").c_str());
        AppendMenu(controls,MF_SEPARATOR,0,0);
        AppendMenu(controls,MF_STRING,mRepaint,ts.translate_def("menu.controlRepaint","重绘(Q)").c_str());
        AppendMenu(controls,MF_STRING,mReload,ts.translate_def("menu.controlReload","重新加载").c_str());
        AppendMenu(controls,MF_STRING,mDConfig,ts.translate_def("menu.controlConfig","重新加载显示配置(R)").c_str());
        AppendMenu(controls,MF_SEPARATOR,0,0);
        AppendMenu(controls,MF_CHECKED,mAutoCfg,ts.translate_def("menu.autoCfg","自动加载配置").c_str());
        AppendMenu(controls,MF_CHECKED,mAutoDis,ts.translate_def("menu.autoDis","自动加载显示配置").c_str());
        ToggleCheckedMenu(mAutoCfg,autoCfg);
        ToggleCheckedMenu(mAutoDis,autoDis);
        AppendMenu(controls,MF_SEPARATOR,0,0);
        AppendMenu(controls,MF_STRING,mBackO,ts.translate_def("menu.controlOrigin","返回原点").c_str());

        //stores
        AppendMenu(stores,MF_STRING,mOpenFileP,ts.translate_def("menu.jmpToFile","转到文件(J)").c_str());
        AppendMenu(stores,MF_STRING,mNew,ts.translate_def("menu.openNewFile","打开新文件(O)").c_str());
        AppendMenu(stores,MF_STRING,mStore,ts.translate_def("menu.store","存储数据(S)").c_str());

        hwnd = CreateWindowEx(0,"FT",ts.translate_def("title.lookForFT","观察傅里叶变换").c_str(),WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,CW_USEDEFAULT,CW_USEDEFAULT,
                              600,600,NULL,NULL,NULL,NULL);
        subHWND = CreateWindowEx(0,"FT",ts.translate_def("title.console","傅里叶变换控制台").c_str(),WS_CAPTION | WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,
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

    loffset.start();
    fpsClk.start();

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
        if(updatePos.now().offset > 1000){
            updatePos.getOffset();
            updatePos.clearOffset();
            GetWindowRect(desktopWindow,&desktopRect);
            h = (desktopRect.bottom - desktopRect.top) / 5 * 3;
            w = h;
            x = ((desktopRect.right - desktopRect.left) - w)/2;
            y = ((desktopRect.bottom - desktopRect.top) - h)/2;
        }
        if(clk_time.getAllTime() > 16000 && ans){
            int an = MessageBox(NULL,ts.translate_def("tooManyVecs","向量似乎有点多是否退出").c_str(),ts.translate_def("info","提示").c_str(),MB_TOPMOST | MB_ICONQUESTION | MB_YESNO);
            if(an == IDYES){
                RedrawWindow(desktopWindow,&desktopRect,NULL,RDW_ALLCHILDREN);
                exit(-1);
            }else{
                MessageBox(NULL,ts.translate_def("escInfo","可随时按下ESC退出！").c_str(),ts.translate_def("info","提示").c_str(),MB_TOPMOST | MB_OK);
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

void ToggleCheckedMenu(int mi,bool stat,HMENU m){
    CheckMenuItem(m,mi,stat?MF_CHECKED:MF_UNCHECKED);
}

bool DealMenu(int id){
    switch(id){
    case mChroma:
        chroma = !chroma;
        ToggleCheckedMenu(mChroma,chroma);
        break;
    case mNeon:
        neon = !neon;
        ToggleCheckedMenu(mNeon,neon);
        break;
    case mDConfig:
        loadDisplayConfig();
        break;
    case mFollow:
        follow = !follow;
        ToggleCheckedMenu(mFollow,follow);
        break;
    case mCircles:
        showCircles = !showCircles;
        ToggleCheckedMenu(mCircles,showCircles);
        break;
    case mLines:
        showLines = !showLines;
        ToggleCheckedMenu(mLines,showLines);
        break;
    case mAutoCfg:
        autoCfg = !autoCfg;
        ToggleCheckedMenu(mAutoCfg,autoCfg);
        break;
    case mAutoDis:
        autoDis = !autoDis;
        ToggleCheckedMenu(mAutoDis,autoDis);
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
        stop = !stop;
        SetMenuText(menu,mPlay,stop?(LPSTR)ts.translate_def("menu.controlPlay","播放(空格)").c_str():(LPSTR)ts.translate_def("menu.controlPause","暂停(空格)").c_str());
        break;
    case mRepaint:
        calstop = true;
        point1.clear();
        for(unsigned int xt0 = 0;xt0 < points.size();xt0++){
            points[xt0].rotation = RadToDeg(points[xt0].orot);
        }
        calstop = false;
        break;
    case mOpenFileP:{
        string command = "start notepad \"";
        command += opener;
        command += "\"";
        system(command.c_str());
        break;
    }
    case mStore:{
        ofstream kofs(DATA_OUTPUT);
        if(kofs.bad()){
            l.error("Cannot open file \"" DATA_OUTPUT "\" to write down the data!");
            errorFlag = true;
        }else{
            kofs << point1.size() << " " << "-32 32 " << timeCur <<endl;
            for(Vector & vx : point1){
                kofs << vx.x << " " << vx.y << "\n";
            }
            l.info("Data is written to \"" DATA_OUTPUT "\" successfully!");
            kofs.close();
        }
        break;
    }
    case mNew:
        OpenFileS();
        break;
    case mHelp:{
        system("start notepad " ".\\data\\help_zh_cn.txt");
        break;
    }
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
            errors = string("Failed to load files normally,because:\n") + errors;
            l.error(errors);
            errorFlag = true;
        }
        gc.part_x *= gc.scales;
        gc.part_y *= gc.scales;
        calstop = false;
        break;
    case mBackO:
        pos.x = pos.y = 0;
        pdpart.x = pos.x / gc.part_x;
        pdpart.y = pos.y / gc.part_y;
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

void scale(bool up){
    if(up){
        ///use multiply else not be a round///
        gc.oneeqw *= 1.1;
        gc.scales = 1/gc.oneeqw;
        if(gc.scales <= 0.001)gc.scales = 0.001;
    }else{
        gc.oneeqw /= 1.1;
        gc.scales = 1/gc.oneeqw;
        if(gc.scales <= 0.001)gc.scales = 0.001;
    }
    gc.part_x = opx * gc.scales;
    gc.part_y = opy * gc.scales;
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
    case WM_SETFOCUS:
        focus = true;
        break;
    case WM_KILLFOCUS:
        focus = false;
        break;
    case WM_KEYDOWN:{
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
            case 'L':
                DealMenu(mLines);
                break;
            case 'O':
                DealMenu(mCircles);
                break;
            case 'R':
                DealMenu(mDConfig);
                break;
            case 'C':
                DealMenu(mDisCoord);
                break;
            case 'V':
                DealMenu(mDisVectors);
                break;
            case 'F':
                DealMenu(mDisFVec);
                break;
            case 'M':
                DealMenu(mChroma);
                break;
            case VK_SPACE:
               DealMenu(mPlay);
                break;
            case VK_ADD:
                scale(true);
                break;
            case VK_SUBTRACT:
                scale(false);
                break;
            case 229:
                switch(lParam){
                    case 1074593793://long press
                    case 851969://'+'
                        scale(true);
                        break;
                    case 1074528257:
                    case 786433://'-'
                        scale(false);
                        break;
                    default:
                        break;
                }
                break;
            case 187:
                scale(true);
                break;
            case 189:
                scale(false);
                break;
        }
        pdpart.x = pos.x / gc.part_x;
        pdpart.y = pos.y / gc.part_y;
    }
    break;
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
    RateLimiter restrictFps(30);
    while(!bQuit){
        ///Draw To Console
        SelectBitmap(hsubDC,bmp);
        SetBkColor(hsubDC,RGB(0,0,0));
        SetTextColor(hsubDC,RGB(255,255,255));
        SelectBrush(hsubDC,blkBrush);
        Rectangle(hsubDC,0,0,424,424);
        if(frameGoes >= calFPSFr){
            int off = 0;
            off = fpsClk.getOffset();
            fpsClk.clearOffset();
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
            ///更新配置文件
            auto p0 = lw_config;
            auto p1 = lw_display;
            updateConfigLastWrite();
            if(autoCfg && p0 != lw_config){
                DealMenu(mReload);
            }
            if(autoDis && p1 != lw_display){
                DealMenu(mDConfig);
            }
            int sz = sprintf(outputbuf,
                consoleStr.c_str(),
                preFrameC,mxFrame,timeCur * 1000,
                timeCur,
                point1[point1.size()-1].x,point1[point1.size()-1].y,
                points.size(),point1.size(),rderr?errStr.c_str():"");

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
        restrictFps.wait();
    }
}

#define PerSpec 100
void Display(){
    EnableOpenGL(hwnd, &hDC, &hRC);
    RateLimiter rest(gc.frame_limit);
    Clock movec;
    Trigger tmove(movec,10);

    while(!bQuit){

        if(follow){
            pos.x = -point1[point1.size()-1].x;
            pos.y = -point1[point1.size()-1].y;
            pdpart.x = pos.x / gc.part_x;
            pdpart.y = pos.y / gc.part_y;
        }else if(focus && tmove.test()){
            if(GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState(VK_UP) & 0x8000){
                pos.y -= gc.move_speed * gc.scales * 0.1;
            }else if(GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState(VK_DOWN) & 0x8000){
                pos.y += gc.move_speed * gc.scales * 0.1;
            }
            if(GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000){
                pos.x += gc.move_speed * gc.scales * 0.1;
            }else if(GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000){
                pos.x -= gc.move_speed * gc.scales * 0.1;
            }
            pdpart.x = pos.x / gc.part_x;
            pdpart.y = pos.y / gc.part_y;
        }

        float tx =0,ty = 0;
        frameGoes++;

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glPushMatrix();
//        glRotatef(theta, 0.0f, 1.0f, 0.0f);

        glBegin(GL_LINES);
            //Draw the line
            if(discoord){
                glColor3f(1.0,0,0);
                glVertex2f(pdpart.x,-1);
                glVertex2f(pdpart.x,1);
                glColor3f(0,1.0,0);
                glVertex2f(-1,pdpart.y);
                glVertex2f(1,pdpart.y);
                if(threed){
                    glColor3f(0,0,1.0);
                    glVertex3f(pdpart.x,pdpart.y,-1);
                    glVertex3f(pdpart.x,pdpart.y,1);
                }
            }
            glColor3f(1.0,1.0,1.0);
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
                    unsigned int iterx = max(16.f,PerSpec * tLen /2);
                    for(unsigned int p = 0;p < iterx;p++){
                        tx = (tLen * cos(2*PI/iterx * p) + def.x) / gc.part_x;
                        ty = (tLen * sin(2*PI/iterx * p) + def.y) / gc.part_y;
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
                if(neon)glColor3f(
                    0.3*sin(cneon.getAllTime()/500.0) + 0.7,
                    0.3*cos(cneon.getAllTime()/500.0) + 0.7,
                    0.3*sin(2*cneon.getAllTime()/400.0) + 0.7
                );
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
        if(gc.frame_limit > 0){
            rest.wait();
        }
        elapseTime = loffset.getOffset();
        loffset.clearOffset();
    }
    DisableOpenGL(hwnd, hDC, hRC);
}

void UpdatingPainting(){
    RateLimiter rest(100);
    while(!bQuit){
        ///calstop : stop multiple thread causes undefine behavior
        if((!stop && !calstop) || nextFrame){
            nextFrame = false;
            ///Calculating Painting Data
            Vector b = {0,0,0};
            for(unsigned int i = 0;i < points.size();i++){
                //Read Infinite
                //Multiply speed
                if(points[i].secperRound != RINFINITE)points[i].Rotate(timeCur);
                b.x += points[i].x;
                b.y += points[i].y;
            }
            point1.push_back(b);
            if(gc.vertices_limit < 0 && point1.size() > (unsigned int)(gc.vertices_limit))
                point1.erase(point1.begin());
            timeCur += 0.01 * gc.run_speed;
        }
        rest.wait();
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

int OpenFileS(){
    BOOL bSel;
    OPENFILENAME file = {0};
    string filePath = "";
    filePath.resize(MAX_PATH);
    ZeroMemory((void*)filePath.c_str(),sizeof(TCHAR) * MAX_PATH);
    file.hwndOwner = subHWND;
    file.lStructSize = sizeof(file);
    file.lpstrFilter = TEXT("*.*(*.*)\0*.*\0");//要选择的文件后缀
    file.lpstrInitialDir = lastDir.c_str();//默认的文件路径
    file.lpstrFile = (LPSTR)filePath.c_str();//存放文件的缓冲区
    file.nMaxFile = MAX_PATH;
    file.nFilterIndex = 0;
    file.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;//标志如果是多选要加上OFN_ALLOWMULTISELECT

    bSel = GetOpenFileName(&file);
    if(bSel){
        opener = filePath;
        DealMenu(mReload);
    }
    return 0;
}

void loadDisplayConfig(){
    GDoc status;
    if(!status.read_parseFileTOML(DATA_DISPLAY)){
        gc.frame_limit = value_unfold(status.get("frameLimit"),180,atof);
        gc.move_speed = value_unfold(status.get("moveSpeed"),1,atof);
        gc.scale_speed = value_unfold(status.get("scaleSpeed"),1,atof);
        gc.storing_limi = value_unfold(status.get("fenv"),5,atof);
        gc.vertices_limit = value_unfold(status.get("verticesLimit"),4096,atof);
        gc.part_x = value_unfold(status.get("partx"),8,atof);
        gc.part_y = value_unfold(status.get("party"),8,atof);
        gc.run_speed = value_unfold(status.get("calcSpeed"),1,atof);
    }else{
        l(LOG_ERROR) << "Failed to load display config." << endlog;
    }
}

void updateConfigLastWrite(){
    lw_display = filesystem::last_write_time(DATA_DISPLAY);
    lw_config = filesystem::last_write_time(DATA_CONFIG);
}
