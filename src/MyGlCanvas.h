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

// Includes for the GLTexture class.
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


// winged Edge
struct W_edge; 
struct Vertex {
  float x, y, z;
  W_edge *edge;
};
struct Face {
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

inline float deg2rad(int degree) {
    return ((float)degree * 3.14f) / 180.0f;
}

class MyGLCanvas : public nanogui::GLCanvas {
public:
    MyGLCanvas(Widget *parent); 

    //flush data on call
    ~MyGLCanvas() {
      mShader.free();
    }
    void setTranslation(Eigen::Vector3f transVec) {
        mTransVec = transVec;
    }
    void setScaling(Eigen::Vector3f scaleVec) {
        mScaleVec = scaleVec;
    }
    void setRotation(Eigen::Vector3f rotateVec) {
        mRotatVec = rotateVec; // radians
    }

    Eigen::Matrix4f updateMVP();

    void updateNewMesh(MatrixXf &positions, MatrixXu &indices);

    void updateMeshForm(unsigned int meshForms);

    void updateMeshColors(MatrixXf newColors){
        mColors = newColors;
    }

    //OpenGL calls this method constantly to update the screen.
    virtual void drawGL() override; 

//Instantiation of the variables that can be acessed outside of this class to interact with the interface
//Need to be updated if a interface element is interacting with something that is inside the scope of MyGLCanvas
private:
    MatrixXf mPositions;
    MatrixXf mNormals;
    MatrixXf mVNormals;
    MatrixXf mFNormals;
    MatrixXf mColors;
    nanogui::GLShader mShader;
    Eigen::Vector3f mTransVec;
    Eigen::Vector3f mScaleVec;
    Eigen::Vector3f mRotatVec;
    int mTriangles = 12;
    bool isFaceOn = true;
    bool isLineOn = false;
    enum MeshForms {Flat_Shaded, Smooth_Shaded, Wireform, Shaded_with_Wire};
};

#endif