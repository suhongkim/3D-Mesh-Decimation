#include "MyGlCanvas.h"

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


MyGLCanvas::MyGLCanvas(Widget *parent) : nanogui::GLCanvas(parent) {
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

    
void MyGLCanvas::drawGL() {
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


