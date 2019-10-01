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


#if defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#if defined(_WIN32)
#  pragma warning(push)
#  pragma warning(disable: 4457 4456 4005 4312)
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if defined(_WIN32)
#  pragma warning(pop)
#endif
#if defined(_WIN32)
#  if defined(APIENTRY)
#    undef APIENTRY
#  endif
#  include <windows.h>
#endif

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
struct Vertex;
struct Face;
struct W_edge {
  Vertex *start, *end;
  Face *left, *right;
  W_edge *left_prev, *left_next;
  W_edge *right_prev, *right_next;
};
struct Vertex {
  float x, y, z;
  W_edge *edge;
};
struct Face {
  W_edge *edge;
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

// load obj files
void load_obj(string file_path, nanogui::MatrixXf &newPositions, nanogui::MatrixXu &newIndices) {
    FILE * file = fopen(file_path.c_str(), "r");
    if( file == NULL ){
        printf("Impossible to open the file !\n");
        return;
    }

    int idx_v = 0;
    int idx_f = 0;
    while( 1 ){
        char lineHeader[128];
        // read the first word of the line
        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF) break; // EOF = End Of File. Quit the loop
        // todo : Check file is obj or not
        // else : parse lineHeader
        // Get the size of v and f
        if ( strcmp( lineHeader, "#" ) == 0 ){
            int num_v = 0, num_f = 0;
            bool t = fscanf(file, "%d %d\n", &num_v, &num_f );
            newPositions = MatrixXf(3, num_v);
            newIndices = MatrixXu(3, num_f);
            cout << num_v << "   " << num_f << endl;
        }
        if ( strcmp( lineHeader, "v" ) == 0 ){
            float v_x = 0.0f, v_y = 0.0f, v_z = 0.0f;
            int res = fscanf(file, "%f %f %f\n", &v_x, &v_y, &v_z);
            newPositions.col(idx_v) << v_x, v_y, v_z;
            idx_v++;
        }
        if ( strcmp( lineHeader, "f" ) == 0 ){
            unsigned int f_v1 = 0, f_v2 = 0, f_v3 = 0;
            int res = fscanf(file, "%u %u %u\n", &f_v1, &f_v2, &f_v3);
            newIndices.col(idx_f) << f_v1-1, f_v2-1, f_v3-1; // index starts from 1-> 0
            idx_f++;
        }
    }
}

void save_obj (string file_path, nanogui::MatrixXf &positions, nanogui::MatrixXu &faces)
{
  std::ofstream file;

  file.open (file_path.c_str());
  file << '#'<<' '<< positions.cols() <<' '<< faces.cols() << endl;
  for (int p = 0; p < positions.cols(); p++) {
      file << 'v' <<' '<< positions(0, p)<<' '<< positions(1, p) <<' '<< positions(2, p) << endl;
  }
  for (int f = 0; f < faces.cols(); f++) {
      file << 'f' <<' '<< faces(0, f) + 1 <<' '<< faces(1, f) + 1 <<' '<< faces(2, f) + 1 << endl;
  }
  file.close();
}


class MyGLCanvas : public nanogui::GLCanvas {
public:
    MyGLCanvas(Widget *parent) : nanogui::GLCanvas(parent) {
      using namespace nanogui;

	    mShader.initFromFiles("a_smooth_shader", "StandardShading.vertexshader", "StandardShading.fragmentshader");

      mShader.bind();
      // ViewMatrixID
	    // change your rotation to work on the camera instead of rotating the entire world with the MVP matrix
	    Matrix4f V;
	    V.setIdentity();
	    //V = lookAt(Vector3f(0,12,0), Vector3f(0,0,0), Vector3f(0,1,0));
	    mShader.setUniform("V", V);

	    //ModelMatrixID
	    Matrix4f M;
	    M.setIdentity();
	    mShader.setUniform("M", M);

	
      mShader.uploadAttrib("vertexPosition_modelspace", mPositions);
      mShader.uploadAttrib("color", mColors);
	    mShader.uploadAttrib("vertexNormal_modelspace", mNormals);

      mTransVec = Eigen::Vector3f(0,0,0);
      mScaleVec = Eigen::Vector3f(1,1,1);
      mRotatVec = Eigen::Vector3f(0,0,0);
      auto mvp = updateMVP(); 
      mShader.setUniform("MVP", mvp);

	    // This the light origin position in your environment, which is totally arbitrary
	    // however it is better if it is behind the observer
	    mShader.setUniform("LightPosition_worldspace", Vector3f(-2,6,-4));
    }

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
    virtual void drawGL() override {
        using namespace nanogui;

	    //refer to the previous explanation of mShader.bind();
        mShader.bind();

	    //this simple command updates the positions matrix. You need to do the same for color and indices matrices too
        mShader.uploadAttrib("vertexPosition_modelspace", mPositions);
        mShader.uploadAttrib("color", mColors);
        mShader.uploadAttrib("vertexNormal_modelspace", mNormals);

      //This is a way to perform a simple rotation using a 4x4 rotation matrix represented by rmat
	    //mvp stands for ModelViewProjection matrix
	auto mvp = updateMVP(); 
        mShader.setUniform("MVP", mvp);
	    // If enabled, does depth comparisons and update the depth buffer.
	    // Avoid changing if you are unsure of what this means.
        glEnable(GL_DEPTH_TEST);

        if(isFaceOn) mShader.drawArray(GL_TRIANGLES, 0, mTriangles*3);
	      if(isLineOn) mShader.drawArray(GL_LINES, mTriangles*3, mTriangles*3*2);

        glDisable(GL_DEPTH_TEST);
    }

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

Eigen::Matrix4f MyGLCanvas::updateMVP() {
  Eigen::Matrix4f translation;
  translation.setIdentity();
  translation.block(0, 3, 3, 3) = mTransVec;

  Eigen::Matrix4f scaling;
  scaling.setIdentity();
  scaling.topLeftCorner<3,3>() = mScaleVec.asDiagonal();

  Eigen::Matrix4f rotation;
  rotation.setIdentity();
  rotation.topLeftCorner<3,3>() = Eigen::Matrix3f(Eigen::AngleAxisf(mRotatVec[0], Eigen::Vector3f::UnitX()) *
                                                  Eigen::AngleAxisf(mRotatVec[1], Eigen::Vector3f::UnitY()) *
                                                  Eigen::AngleAxisf(mRotatVec[2], Eigen::Vector3f::UnitZ())) * 0.25f;

  auto mvp = translation * rotation * scaling; 
  return mvp;
  //return scaling*translation * rotation ;
}

void MyGLCanvas::updateNewMesh(MatrixXf &positions, MatrixXu &indices) {
  //< Winged Edge Structure>
  int n_vertex = positions.cols();
  int n_face = indices.cols();
  int n_edge = 3*n_face; //2*(n_vertex + n_face -2); // genus=0  V + F - E/2 = 2 - 2G

  Vertex vertices[n_vertex];
  Face faces[n_face];
  W_edge edges[n_edge]; // winged edge has two directions

  // [step0] Need to scale meshes into [-1, 1] (Centered)
  Eigen::Vector3f max = positions.rowwise().maxCoeff();
  Eigen::Vector3f min = positions.rowwise().minCoeff();

  for (int p = 0; p < positions.cols(); p++) {
      positions.col(p)[0] = 2 * (positions.col(p)[0]-min[0])/(max[0]-min[0]) -1;
      positions.col(p)[1] = 2 * (positions.col(p)[1]-min[1])/(max[1]-min[1]) -1;
      positions.col(p)[2] = 2 * (positions.col(p)[2]-min[2])/(max[2]-min[2]) -1;
  }

  // [step1] Initialize Vertex from positions
  for (int v = 0; v < positions.cols(); v++) {
      vertices[v].x = positions.col(v)[0];
      vertices[v].y = positions.col(v)[1];
      vertices[v].z = positions.col(v)[2];
  }
  // [step2] Initialize Face and Edges on Right Face
  for (int f = 0; f < n_face; f++) {
      // position(vertex) index of each face
      int v_idx[3] = {indices.col(f)[0], indices.col(f)[1], indices.col(f)[2]};
      // edge index of each face
      int e_idx[3] = {3*f + 0, 3*f + 1, 3*f + 2};
      // face
      faces[f].edge = &edges[e_idx[0]]; // first edge of each triangle

      for (int i = 0; i < 3; i++) {
          // vertices
          vertices[v_idx[i]].edge = &edges[e_idx[i]]; // edge with start
          // edges (counter clockwise) : f = Right Face for all faces, edges
          edges[e_idx[i]].start = &vertices[v_idx[i]];
          edges[e_idx[i]].end   = &vertices[v_idx[(i + 1)%3]];
          edges[e_idx[i]].right = &faces[f];
          edges[e_idx[i]].right_next = &edges[e_idx[(i + 1)%3]];
          edges[e_idx[i]].right_prev = &edges[e_idx[(i + 2)%3]];
      }
  }
  // [step3] fill up the leftover info of edges
  for (int e = 0; e < n_edge; e++) {
      for (int ce = n_edge-1; ce >= 0; ce--) {
          if(edges[e].start == edges[ce].end && edges[e].end == edges[ce].start) {
              edges[e].left       = edges[ce].right;
              edges[e].left_next  = edges[ce].right_next;
              edges[e].left_prev  = edges[ce].right_prev;
              break;
          }
      }
  }
  /********************************************************/
  // caluate Vertex Normal
  /*
      vertex v1, v2, v3, ....
      triangle tr1, tr2, tr3 // all share vertex v1
      v1.normal = normalize( tr1.normal + tr2.normal + tr3.normal )
  */
  // finding all faces that share a vertex in anticlockwise order
  MatrixXf normals = MatrixXf(positions.rows(), positions.cols());
  MatrixXf faceNormals = MatrixXf(indices.rows(), indices.cols());
  cout << "mNormals num: " << normals.cols() << endl;
  for (int vn = 0; vn < normals.cols(); vn++) {
      Vertex * v = &vertices[vn];
      W_edge * e0 = v->edge;
      W_edge * e = e0;
      nanogui::Vector3f normal(0.0f, 0.0f, 0.0f);
      int count = 0;

      while(1) {
          //find Face Edge and move e
          W_edge * faceEdge;
          if (*(e->end) == *v) {
              faceEdge = e->right->edge;
              e = (e->right_next);
          } else {
              faceEdge = e->left->edge;
              e = (e->left_next);
          }
          //claculate Face normal
          /*
              triangle ( v1, v2, v3 )
              edge1 = v2-v1
              edge2 = v3-v1
              triangle.normal = cross(edge1, edge2).normalize
          */
          nanogui::Vector3f v1(faceEdge->start->x, faceEdge->start->y, faceEdge->start->z);
          nanogui::Vector3f v2(faceEdge->end->x, faceEdge->end->y, faceEdge->end->z);
          nanogui::Vector3f v3(faceEdge->right_next->end->x, faceEdge->right_next->end->y, faceEdge->right_next->end->z);
          nanogui::Vector3f n = (v2-v1).cross(v3-v1).normalized();
          normal += n;

          if(*e == *e0)  break;
          if(count > 36) {cout << " countover " << endl; break;} // no inf
          count ++;
      }
      normals.col(vn) = normal;
      // init MVP
      mTransVec = Eigen::Vector3f(0,0,0);
      mScaleVec = Eigen::Vector3f(1,1,1);
      mRotatVec = Eigen::Vector3f(0,0,0);
      mShader.setUniform("MVP", updateMVP());
  }

  /*********************************************************/
  // update mPositions, mNormals based on Faces
  mTriangles = n_face;
  mPositions = MatrixXf(3, 3*mTriangles + mTriangles*3*2);
  mVNormals  = MatrixXf(3, 3*mTriangles + mTriangles*3*2);     // vertex normal for smooth shading
  mFNormals  = MatrixXf(3, 3*mTriangles + mTriangles*3*2);    // vertex normal for flat shading
  mColors    = MatrixXf(3, 3*mTriangles + mTriangles*3*2);
  int f_idx = 0;
  int l_idx =  3*mTriangles + 0;
  for (int face = 0; face < mTriangles; face++) {
      nanogui::Vector3f v1 = positions.col(indices.col(face)[0]);
      nanogui::Vector3f v2 = positions.col(indices.col(face)[1]);
      nanogui::Vector3f v3 = positions.col(indices.col(face)[2]);
      nanogui::Vector3f n = (v2-v1).cross(v3-v1).normalized();
      nanogui::Vector3f f_color(0, 1, 0);
      nanogui::Vector3f l_color(0, 0, 0);
      for (int f = 0; f < 3; f++) {
            mPositions.col(f_idx) = positions.col(indices.col(face)[f]);
            mVNormals.col(f_idx)   = normals.col(indices.col(face)[f]);
            mFNormals.col(f_idx)  = n;
            mColors.col(f_idx)    = f_color;
            f_idx++;
      }
      for (int l = 0; l < 3; l++) {
            mPositions.col(l_idx) = positions.col(indices.col(face)[l]) + n*0.002;
            mVNormals.col(l_idx)  = normals.col(indices.col(face)[l]);
            mFNormals.col(l_idx)  = n;
            mColors.col(l_idx)    = l_color;
            l_idx++;
            mPositions.col(l_idx) = positions.col(indices.col(face)[(l+1)%3]) + n*0.002;
            mVNormals.col(l_idx)  = normals.col(indices.col(face)[(l+1)%3]);
            mFNormals.col(l_idx)  = n;
            mColors.col(l_idx)    = l_color;
            l_idx++;
      }
  }
  mNormals = mFNormals; // default : flat shading
}

void MyGLCanvas::updateMeshForm(unsigned int meshForms){
        switch(meshForms){
            case Flat_Shaded:
                mNormals = mFNormals;
                isFaceOn = true;
                isLineOn = false;
                break;
            case Smooth_Shaded:
                mNormals = mVNormals;
                isFaceOn = true;
                isLineOn = false;
                break;
            case Wireform:
                isFaceOn = false;
                isLineOn = true;
                break;
            case Shaded_with_Wire:
                isFaceOn = true;
                isLineOn = true;
                break;
            default:
                mNormals = mFNormals;
                isFaceOn = true;
                isLineOn = false;
        }
    }


class ExampleApplication : public nanogui::Screen {
public:
    ExampleApplication() : nanogui::Screen(Eigen::Vector2i(900, 600), "NanoGUI Cube and Menus", false) {
        using namespace nanogui;

	//First, we need to create a window context in which we will render both the interface and OpenGL canvas
        Window *window = new Window(this, "GLCanvas Demo");
        window->setPosition(Vector2i(15, 15));
        window->setLayout(new GroupLayout());

	//OpenGL canvas initialization, we can control the background color and also its size
        mCanvas = new MyGLCanvas(window);
        mCanvas->setBackgroundColor({100, 100, 100, 255});
        mCanvas->setSize({400, 400});

        // Initialize with Defulat Object(cube)
        string file_path = "./cube_obj.obj";
        load_obj(file_path, objPositions, objIndices);
        mCanvas->updateNewMesh(objPositions, objIndices);

	//This is how we add widgets, in this case, they are connected to the same window as the OpenGL canvas
        Widget *tools = new Widget(window);
        tools->setLayout(new BoxLayout(Orientation::Horizontal,
                                       Alignment::Middle, 0, 5));

	//then we start adding elements one by one as shown below
        Button *b0 = new Button(tools, "Random Color");
        b0->setCallback([this]() { mCanvas->setBackgroundColor(Vector4i(rand() % 256, rand() % 256, rand() % 256, 255)); });


        Button *b1 = new Button(tools, "Random Rotation");
        b1->setCallback([this]() {
        	radians[0] = (rand() % 100) / 100.0f;
        	radians[1] = (rand() % 100) / 100.0f;
        	radians[2] = (rand() % 100) / 100.0f;
        	mCanvas->setRotation(Eigen::Vector3f(radians[0], radians[1], radians[2]));
        });

        Button *b2 = new Button(tools, "QUIT");
        b2->setCallback([this]() {
          exit(1);
        });

        //widgets demonstration
        nanogui::GLShader mShader;

	//Then, we can create another window and insert other widgets into it
	Window *anotherWindow = new Window(this, "Basic widgets");
        anotherWindow->setPosition(Vector2i(500, 15));
        anotherWindow->setLayout(new GroupLayout());

    	new Label(window, "Rotation on the first axis", "sans-bold");

        // New Mesh : Open obj file and load object
        new Label(anotherWindow, "Open / Save", "sans-bold");
        Button *b_open = new Button(anotherWindow, "New Mesh");
    		b_open->setCallback([&] {
          string file_path =  file_dialog(
            { {"png", "Portable Network Graphics"}, {"txt", "Text file"}, {"obj", "Obj File"} }, false);

          load_obj(file_path, objPositions, objIndices);
          mCanvas->updateNewMesh(objPositions, objIndices);

          cout << " update is done" << endl;
        });
        b_open->setTooltip("Open New Mes");

        // Save to File : Save obj to file
        Button *b_save = new Button(anotherWindow, "Save to File");
    		b_save->setCallback([&] {
          string file_path =  file_dialog(
            { {"png", "Portable Network Graphics"}, {"txt", "Text file"}, {"obj", "Obj File"} }, true);
            save_obj(file_path, objPositions, objIndices);
        });
        b_save->setTooltip("Save your mesh");


    // Option for Shading
    new Label(anotherWindow, "Choose Mesh Form", "sans-bold");
    ComboBox *combo = new ComboBox(anotherWindow, { "Flat Shaded", "Smooth Shaded", "In Wireframe", "Shaded with Mesh"} );
      combo->setCallback([&](int value) {
      mCanvas->updateMeshForm(value);
    });

    // Option for Zooming
    new Label(anotherWindow, "Zooming", "sans-bold");
    Widget *panel = new Widget(anotherWindow);
    panel->setLayout(new BoxLayout(Orientation::Horizontal,
                                   Alignment::Middle, 0, 20));
    Slider *slider = new Slider(panel);
    slider->setValue(0.5f);
    slider->setFixedWidth(80);
    TextBox *textBox = new TextBox(panel);
    textBox->setFixedSize(Vector2i(60, 25));
    textBox->setValue("100");
    textBox->setUnits("%");
    slider->setCallback([textBox](float value) {
        textBox->setValue(std::to_string((int) (4*value * 100)));
    });
    slider->setFinalCallback([&](float value) {
        cout << "Final slider value: " << (int) (4*value * 100) << endl;
        //100% : [-1, 1]
        mCanvas->setScaling(Eigen::Vector3f(4*value, 4*value, 4*value));
    });
    textBox->setFixedSize(Vector2i(60,25));
    textBox->setFontSize(20);
    textBox->setAlignment(TextBox::Alignment::Right);

    // Rotation slots : XYZ
    new Label(anotherWindow, "Rotation (X, Y, Z)", "sans-bold");
    Widget *panelRot = new Widget(anotherWindow);
    panelRot->setLayout(new BoxLayout(Orientation::Vertical,
                                       Alignment::Middle, 0, 0));
  	Slider *rotSlider = new Slider(panelRot);
    rotSlider->setValue(0.5f);
    rotSlider->setFixedWidth(150);
    rotSlider->setCallback([&](float value) {
  	 // the middle point should be 0 rad
  	 // then we need to multiply by 2 to make it go from -1. to 1.
  	 // then we make it go from -2*M_PI to 2*M_PI
  	radians[0] = (value - 0.5f)*2*2*M_PI;
  	 //then use this to rotate on just one axis
  	mCanvas->setRotation(Eigen::Vector3f(radians[0], radians[1],radians[2]));
  	    //when you implement the other sliders and/or the Arcball, you need to keep track
  	    //of the other rotations used for the second and third axis... It will not stay as 0.0f
    });
    rotSlider->setTooltip("X");

    Slider *rotSlider2 = new Slider(panelRot);
    rotSlider2->setValue(0.5f);
    rotSlider2->setFixedWidth(150);
    rotSlider2->setCallback([&](float value) {
  	radians[1] = (value - 0.5f)*2*2*M_PI;
  	mCanvas->setRotation(Eigen::Vector3f(radians[0], radians[1],radians[2]));
  	});
    rotSlider2->setTooltip("Y");
    Slider *rotSlider3 = new Slider(panelRot);
    rotSlider3->setValue(0.5f);
    rotSlider3->setFixedWidth(150);
    rotSlider3->setCallback([&](float value) {
  	radians[2] = (value - 0.5f)*2*2*M_PI;
  	mCanvas->setRotation(Eigen::Vector3f(radians[0], radians[1], radians[2]));
  	});
    rotSlider3->setTooltip("Z");

    // Translation slots : XYZ (abs)
    new Label(anotherWindow, "Translation (X, Y, Z)", "sans-bold");
    Widget *panelRot2 = new Widget(anotherWindow);
    panelRot2->setLayout(new BoxLayout(Orientation::Vertical,
                                       Alignment::Middle, 0, 0));
  	Slider *rotSliderX = new Slider(panelRot2);
    rotSliderX->setValue(0.5f);
    rotSliderX->setFixedWidth(150);
    rotSliderX->setCallback([&](float value) {
      positions[0] = value-0.5;
      mCanvas->setTranslation(Eigen::Vector3f(positions[0], positions[1], positions[2]));
    });
    rotSliderX->setTooltip("X");

    Slider *rotSliderY = new Slider(panelRot2);
    rotSliderY->setValue(0.5f);
    rotSliderY->setFixedWidth(150);
    rotSliderY->setCallback([&](float value) {
      positions[1] = value-0.5;
  	  mCanvas->setTranslation(Eigen::Vector3f(positions[0], positions[1], positions[2]));
  	});
    rotSliderY->setTooltip("Y");

    Slider *rotSliderZ = new Slider(panelRot2);
    rotSliderZ->setValue(0.5f);
    rotSliderZ->setFixedWidth(150);
    rotSliderZ->setCallback([&](float value) {
      positions[2] = value-0.5;
  	  mCanvas->setTranslation(Eigen::Vector3f(positions[0], positions[1], positions[2]));
  	});
    rotSliderZ->setTooltip("Z (Not possible to see)");








    new Label(anotherWindow, "Check box", "sans-bold");
    CheckBox *cb = new CheckBox(anotherWindow, "Flag 1",
        [](bool state) { cout << "Check box 1 state: " << state << endl; }
    );
    cb->setChecked(true);
    cb = new CheckBox(anotherWindow, "Flag 2",
        [](bool state) { cout << "Check box 2 state: " << state << endl; }
    );




	//Method to assemble the interface defined before it is called
        performLayout();
    }

    //This is how you capture mouse events in the screen. If you want to implement the arcball instead of using
    //sliders, then you need to map the right click drag motions to suitable rotation matrices
    virtual bool mouseMotionEvent(const Eigen::Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
        if (button == GLFW_MOUSE_BUTTON_3 ) {
	    //Get right click drag mouse event, print x and y coordinates only if right button pressed
	    	cout << p.x()/500.0f << "     " << p.y()/500.0f << "\n";
        	//radians[0] = (rand() % 100) / 100.0f;
        	//radians[1] = (rand() % 100) / 100.0f;
        	radians[0] += (p.y()/500.0f - 0.5f)*2*2*M_PI;
        	radians[1] += (p.x()/500.0f - 0.5f)*2*2*M_PI;
            return true;
        }
        return false;
    }

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


private:
    nanogui::ProgressBar *mProgress;
    MyGLCanvas *mCanvas;
    float radians[3] = {0.0f, 0.0f, 0.0f};
    float positions[3] = {0.0f, 0.0f, 0.0f};
    float scales[3] = {1.0f, 1.0f, 1.0f};
    MatrixXf objPositions;
    MatrixXu objIndices;
};

int main(int /* argc */, char ** /* argv */) {
    try {
        nanogui::init();

            /* scoped variables */ {
            nanogui::ref<ExampleApplication> app = new ExampleApplication();
            app->drawAll();
            app->setVisible(true);
            nanogui::mainloop();
        }

        nanogui::shutdown();
    } catch (const std::runtime_error &e) {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
        #if defined(_WIN32)
            MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
        #else
            std::cerr << error_msg << endl;
        #endif
        return -1;
    }

    return 0;
}
