#ifndef VMATH_HPP_INCLUDED
#define VMATH_HPP_INCLUDED
#define PI 3.14159265358979
#include <iostream>
#include <vector>
#include <stdio.h>
#include <string>
#include <fstream>
#include <ctype.h>

using namespace std;
using namespace alib::g3;

#define RadToDeg(x) (((x)/(PI))*(180))
float degToRad = ((PI)/180);
string errors = "";

#define RINFINITE 0

struct Vector{
    float rotation;
    float orot;
    float secperRound;
    float x;
    float y;

    void Rotate(float diff);
    float Dot(Vector b);
    Vector Normalize();
    float Length();
};

float Vector::Dot(Vector b){
    return x * b.x + y * b.y;
}

Vector Vector::Normalize(){
    float len = Length();
    Vector a = *this;
    a.x = a.x / len;
    a.y = a.y / len;
    return a;
}

void Vector::Rotate(float diff){
    float len = Length();
    rotation = 2* PI * (diff/secperRound) + RadToDeg(orot);
    //cout << rotation << " " << diff << endl;
    x = len * (float)cos(rotation);
    y = len * (float)sin(rotation);
}

float Vector::Length(){
    return (float)sqrt(x*x + y*y);
}

string UppercaseString(string s){
    string rt = "";
    for(unsigned int i = 0;i < s.size();i++){
        rt += toupper(s[i]);
    }
    return rt;
}

vector<Vector> readVectors(string fname,int & err,float & px,float & py,
                           float & speed,int & MXP,int & FL,float & ss,float & mp,
                           float & fenv,string & srs){
    vector<Vector> vecs = {};
    Vector tmp;
    ifstream ifs;
    bool regging = false;
    char ctok = '\0';
    int tokens = 0;
    string tok = "";
    float t = 0;
    string tp;
    string utok;
    errors = "";
    ifs.open(fname);
    if(!ifs.good()){
        errors += "无法读取文件";
        errors += fname;
        errors += "!\n";
        err = -1;///Can't open file correctly.
        return {};
    }
    while(!ifs.eof()){
        tokens++;
        srs = "向量读取中[" + to_string(tokens) + "]";
        ///Pre init
        tok = utok = tp = "";
        ctok = '\0';
        ifs >> tok;
        utok = UppercaseString(tok);
        if(!utok.compare("AREG"))regging = !regging;
        if(regging){
            getline(ifs,tp);
            continue;
        }
        if(!utok.compare("VECTOR") || !utok.compare("V")){
            ///Creating Vector
            tmp = {0};
            for(int x = 0;x < 4;x++){
                ifs >> ctok;
                if(toupper(ctok) == 'X'){
                    ifs >> t;
                    tmp.x = t;
                }else if(toupper(ctok) == 'Y'){
                    ifs >> t;
                    tmp.y = t;
                }else if(toupper(ctok) == 'R'){
                    ifs >> t;
                    tmp.orot = t;
                }else if(toupper(ctok) == 'T'){
                    ifs >> t;
                    tmp.secperRound = t;
                }else if(!tok.compare("")){
                    continue;
                }else{
                    errors += "VECTOR(V)标记不符合语法![标记:" + tok + "][子标记:"+ ctok +"]\n";
                    continue;
                }
            }
            vecs.push_back(tmp);
        }else if(!utok.compare("REG") || !utok.compare("R") || !tok.compare("#")){
            getline(ifs,tp);
            continue;
        }else if(!utok.compare("P") || !utok.compare("PART")){
            ifs >> t;
            px = t;
            ifs >> t;
            py = t;
        }else if(!utok.compare("S") || !utok.compare("SPEED")){
            ifs >> t;
            speed = t;
        }else if(!utok.compare("VLIMIT") || !utok.compare("VL")){
            int v;
            ifs >> v;
            MXP = v;
        }else if(!utok.compare("FRAMELIMIT") || !utok.compare("FL")){
            int v;
            ifs >> v;
            FL = v;
        }else if(!utok.compare("SCALESPEED") || !utok.compare("SS")){
            ifs >> t;
            ss = t;
        }else if(!utok.compare("MOVESPEED") || !utok.compare("MP")){
            ifs >> t;
            mp = t;
        }else if(!utok.compare("FENV")){
            ifs >> t;
            fenv = t;
        }else if(!tok.compare("")){
            continue;
        }else{
            errors += "Main token doesn't match the grammar.[Token:" + tok + "]\n";
            if(utok[0] == '#' || utok[0] == 'R' || !strncmp(utok.c_str(),"REG",3)){
                errors += "Note:也许是注释标记后面没加上空格，试试看吧！(美有姬^_^)";
            }
            continue;
        }
    }
    ifs.close();
    err = tokens;
    return vecs;
}

string GetErrors(){
    return errors;
}

#endif // VMATH_HPP_INCLUDED
