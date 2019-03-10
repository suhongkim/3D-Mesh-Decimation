#ifndef NANOGUIMESHAPP_H
#define NANOGUIMESHAPP_H


#include <nanogui/nanogui.h>
#include "MyGlCanvas_a2.h"

using namespace nanogui; 
using namespace std;  

class NanoguiMeshApp : public nanogui::Screen {
public:
    NanoguiMeshApp();

    //This is how you capture mouse events in the screen. If you want to implement the arcball instead of using
    //sliders, then you need to map the right click drag motions to suitable rotation matrices
    virtual bool mouseMotionEvent(const Eigen::Vector2i &p, const Vector2i &rel, int button, int modifiers) override; 

    virtual void drawContents() override {
        // ... put your rotation code here if you use dragging the mouse, updating either your model points, the mvp matrix or the V matrix, depending on the approach used
        //mCanvas->updateRotation();
    }

    virtual void draw(NVGcontext *ctx) {
	      /* Animate the scrollbar */
        //mProgress->setValue(std::fmod((float) glfwGetTime() / 10, 1.0f));

        /* Draw the user interface */
        Screen::draw(ctx);
    }

    void load_obj(string file_path, nanogui::MatrixXf &newPositions, nanogui::MatrixXu &newIndices);
    void save_obj (string file_path, nanogui::MatrixXf &positions, nanogui::MatrixXu &faces);

private:
    nanogui::ProgressBar *mProgress;
    MyGLCanvas *mCanvas;
    float radians[3] = {0.0f, 0.0f, 0.0f};
    float positions[3] = {0.0f, 0.0f, 0.0f};
    float scales[3] = {1.0f, 1.0f, 1.0f};
    MatrixXf objPositions;
    MatrixXu objIndices;
    int n_decimate = 0;
    
};


#endif