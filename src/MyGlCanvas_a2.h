#ifndef MYGLCANVAS_H
#define MYGLCANVAS_H

#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/button.h>
#include <nanogui/toolbutton.h>
#include <nanogui/popupbutton.h>
#include <nanogui/combobox.h>
#include <nanogui/progressbar.h>
#include <nanogui/entypo.h>
#include <nanogui/messagedialog.h>
#include <nanogui/textbox.h>
#include <nanogui/slider.h>
#include <nanogui/imagepanel.h>
#include <nanogui/imageview.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/colorwheel.h>
#include <nanogui/graph.h>
#include <nanogui/tabwidget.h>
#include <nanogui/glcanvas.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <limits>

// Includes for toperation
#include <cstdint>
#include <memory>
#include <utility>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::pair;
using std::to_string;

using nanogui::Screen;
using nanogui::Window;
using nanogui::GroupLayout;
using nanogui::Button;
using nanogui::CheckBox;
using nanogui::Vector2f;
using nanogui::Vector2i;
using nanogui::MatrixXu;
using nanogui::MatrixXf;
using nanogui::Label;
using nanogui::Arcball;
using nanogui::GLShader; 
using nanogui::Vector3f;

// Modified winged Edge
struct W_edge; 
struct Vertex {
  float x, y, z;
  bool isQ = false;
  nanogui::Matrix4f q;  
  nanogui::Vector3f normal;
  W_edge *edge;
};

struct Face {
  nanogui::Vector3f normal;
  W_edge *edge;

};

struct W_edge {
  Vertex *start, *end;
  Face *left, *right;
  W_edge *left_prev, *left_next;
  W_edge *right_prev, *right_next;
};


inline bool operator==(const Vertex& lhs, const Vertex& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

inline bool operator==(const W_edge& lhs, const W_edge& rhs)
{
    return lhs.start == rhs.start && lhs.end == rhs.end;
}

inline bool operator==(const Face& lhs, const Face& rhs)
{
    return lhs.edge == rhs.edge;
}


inline float deg2rad(int degree) {
    return ((float)degree * 3.14f) / 180.0f;
}

inline int ptr2idx(Vertex * dptr, Vertex * arr0){
    return &(*dptr) - &(*arr0);   
}
inline int ptr2idx(W_edge * dptr, W_edge * arr0){
    return &(*dptr) - &(*arr0);   
}
inline int ptr2idx(Face * dptr, Face * arr0){
    return &(*dptr) - &(*arr0);   
}

inline W_edge* ctrEdge(W_edge * e) {
    return e->left_prev->right_next;
}

inline void printWEdge(W_edge * e, W_edge * e0, Vertex * v0) {
    string out; 
    out = "Edge(" +  to_string(ptr2idx(e, e0)) + ") : " 
            + to_string(ptr2idx(e->start, v0)) +  ", " 
            + to_string(ptr2idx(e->end,  v0) )+ "\n";

    cout << out;  
}
inline void printWEdge(W_edge * e, W_edge * e0, Vertex * v0, Face * f0) {
    string out; 
    out = "Edge(" +  to_string(ptr2idx(e, e0)) + ") : " 
            + to_string(ptr2idx(e->start, v0)) + "(" 
            + to_string(ptr2idx(e->start->edge, e0)) +"), " 
            + to_string(ptr2idx(e->end,  v0) )+ "(" 
            + to_string(ptr2idx(e->end->edge, e0)) +") \n";

    out += "---> Face R (" + to_string(ptr2idx(e->right, f0)) + ") : "
            + to_string(ptr2idx(e->right->edge->start, v0)) + ", "
            + to_string(ptr2idx(e->right->edge->end, v0)) + ", "
            + to_string(ptr2idx(e->right->edge->right_next->end, v0))+ " | "
            + to_string(ptr2idx(e->right->edge, e0)) +  "\n";
    out += "---> Edge R_Next (" + to_string(ptr2idx(e->right_next, e0)) + ") : "
            + to_string(ptr2idx(e->right_next->start, v0)) +  ", " 
            + to_string(ptr2idx(e->right_next->end,  v0) )+ "\n";
    out += "---> Edge R_Prev (" + to_string(ptr2idx(e->right_prev, e0)) + ") : "
            + to_string(ptr2idx(e->right_prev->start, v0)) +  ", " 
            + to_string(ptr2idx(e->right_prev->end,  v0) )+ "\n";

    out += "---> Face L (" + to_string(ptr2idx(e->left, f0)) + ") : "
            + to_string(ptr2idx(e->left->edge->start, v0)) + ", "
            + to_string(ptr2idx(e->left->edge->end, v0)) + ", "
            + to_string(ptr2idx(e->left->edge->right_next->end, v0))+ " | "
            + to_string(ptr2idx(e->left->edge, e0)) + "\n";
    out += "---> Edge L_Next (" + to_string(ptr2idx(e->left_next, e0)) + ") : "
            + to_string(ptr2idx(e->left_next->start, v0)) +  ", " 
            + to_string(ptr2idx(e->left_next->end,  v0) )+ "\n";
    out += "---> Edge L_Prev (" + to_string(ptr2idx(e->left_prev, e0)) + ") : "
            + to_string(ptr2idx(e->left_prev->start, v0)) +  ", " 
            + to_string(ptr2idx(e->left_prev->end,  v0) )+ "\n";
    cout << out;  

}

class MyGLCanvas : public nanogui::GLCanvas {
public:
    MyGLCanvas(Widget *parent); 

    //flush data on call
    ~MyGLCanvas() {
      mShader.free();
    }
    void setTranslation(nanogui::Vector3f transVec) {
        mTransVec = transVec;
    }
    void setScaling(nanogui::Vector3f scaleVec) {
        mScaleVec = scaleVec;
    }
    void setRotation(nanogui::Vector3f rotateVec) {
        mRotatVec = rotateVec; // radians
    }

    nanogui::Matrix4f updateMVP();

    void updateWingEdge(MatrixXf &positions, MatrixXu &indice,
                        vector<Vertex> &vertices, vector<Face> &faces, vector<W_edge> &edges);

    void updateMeshes(); 

    void updateNewMesh(MatrixXf &positions, MatrixXu &indices);

    void updateMeshForm(unsigned int meshForms);

    void getObjForm(string file_path,MatrixXf &positions, MatrixXu &indices);

    void updateMeshColors(MatrixXf newColors){
        mColors = newColors;
    }


    //Mutiple Choice Decimation based on Quadric 
    void decimateMesh(int n_decimate); 
    void decimateOne(vector<Vertex> &v, vector<Face> &f, vector<W_edge> &e); 
    nanogui::Matrix4f calcVertexQuadric(Vertex * v); 
    void findAdjecents(W_edge * c_pair, vector<int> &e_olds, vector<int> &f_olds, vector<W_edge> &e, vector<Face> &f);
    
    //OpenGL calls this method constantly to update the screen.
    virtual void drawGL() override; 

//Instantiation of the variables that can be acessed outside of this class to interact with the interface
//Need to be updated if a interface element is interacting with something that is inside the scope of MyGLCanvas
private:
    //for Winged Edge
    vector<Vertex> mVertices;
    vector<Face>   mFaces;
    vector<W_edge> mEdges; // winged edge has two directions
    // for Mesah Info
    MatrixXf mPositions;
    MatrixXf mNormals;
    MatrixXf mVNormals;
    MatrixXf mFNormals;
    MatrixXf mColors;
    // for shader
    GLShader mShader;
    Vector3f mTransVec;
    Vector3f mScaleVec;
    Vector3f mRotatVec;
    bool isFaceOn = true;
    bool isLineOn = false;
    enum MeshForms {Flat_Shaded, Smooth_Shaded, Wireform, Shaded_with_Wire};
    // decimation
    int mRandomK = 8; 
};

#endif