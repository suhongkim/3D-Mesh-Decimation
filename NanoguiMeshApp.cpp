#include <nanogui/nanogui.h>
#include "NanoguiMeshApp.h"

using namespace nanogui; 
using namespace std;  

NanoguiMeshApp::NanoguiMeshApp(): nanogui::Screen(Eigen::Vector2i(900, 600), "NanoGUI Cube and Menus", false) {
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
    NanoguiMeshApp::load_obj(file_path, objPositions, objIndices);
    mCanvas->updateNewMesh(objPositions, objIndices);

    //This is how we add widgets, in this case, they are connected to the same window as the OpenGL canvas
    Widget *tools = new Widget(window);
    tools->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 5));
                                        
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

      NanoguiMeshApp::load_obj(file_path, objPositions, objIndices);
      mCanvas->updateNewMesh(objPositions, objIndices);

      cout << " update is done" << endl;
    });
    b_open->setTooltip("Open New Mes");

    // Save to File : Save obj to file
    Button *b_save = new Button(anotherWindow, "Save to File");
    b_save->setCallback([&] {
      string file_path =  file_dialog(
        { {"png", "Portable Network Graphics"}, {"txt", "Text file"}, {"obj", "Obj File"} }, true);
        MatrixXf nPositions;
        MatrixXu nIndice;
        mCanvas->getObjForm(file_path, nPositions, nIndice);
        //NanoguiMeshApp::save_obj(file_path, nPositions, nIndice);
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

    new Label(anotherWindow, "Mesh Decimation", "sans-bold");
    Widget *panel2 = new Widget(anotherWindow);
    panel2->setLayout(new BoxLayout(Orientation::Horizontal,
                                    Alignment::Middle, 0, 20));
    auto intBox = new IntBox<int>(panel2);
    intBox->setEditable(true);
    intBox->setFixedSize(Vector2i(80, 25));
    intBox->setValue(1);
    // intBox->setUnits("Mhz");
    intBox->setDefaultValue("0");
    intBox->setFontSize(16);
    intBox->setFormat("[1-9][0-9]*");
    intBox->setSpinnable(true);
    intBox->setMinValue(1);
    intBox->setValueIncrement(1);
    intBox->setCallback([&](int n) {
      NanoguiMeshApp::n_decimate = n;
    });
    Button *b_decimate = new Button(panel2, "Decimate");
    b_decimate->setCallback([&] {
      mCanvas->decimateMesh(NanoguiMeshApp::n_decimate); 
    });
    b_decimate->setTooltip("Mesh Decimate with Random K");
    b_decimate->setFixedSize(Vector2i(100,25));
    b_decimate->setFontSize(20);
    //Method to assemble the interface defined before it is called
    performLayout();
}

bool NanoguiMeshApp::mouseMotionEvent(const Eigen::Vector2i &p, const Vector2i &rel, int button, int modifiers) {
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


// load obj files
void NanoguiMeshApp::load_obj(string file_path, nanogui::MatrixXf &newPositions, nanogui::MatrixXu &newIndices) {
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

void NanoguiMeshApp::save_obj (string file_path, nanogui::MatrixXf &positions, nanogui::MatrixXu &faces)
{
  std::ofstream file;

  for (int i = 0; i < positions.cols(); i++) {
    cout << positions(0, i) << ". " << positions(1,i) << ", " << positions(2,i) << endl;
  }

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