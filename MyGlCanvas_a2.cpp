#include "MyGlCanvas_a2.h"

using namespace std;
using namespace nanogui; 

MyGLCanvas::MyGLCanvas(Widget *parent) : nanogui::GLCanvas(parent) {
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

    mTransVec = Vector3f(0,0,0);
    mScaleVec = Vector3f(1,1,1);
    mRotatVec = Vector3f(0,0,0);
    auto mvp = updateMVP(); 
    mShader.setUniform("MVP", mvp);

    // This the light origin position in your environment, which is totally arbitrary
    // however it is better if it is behind the observer
    mShader.setUniform("LightPosition_worldspace", Vector3f(-2,6,-4));
}

    
void MyGLCanvas::drawGL() {
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

    if(isFaceOn) mShader.drawArray(GL_TRIANGLES, 0, mFaces.size()*3); 
    if(isLineOn) mShader.drawArray(GL_LINES, mFaces.size()*3, mFaces.size()*3*2); 

    glDisable(GL_DEPTH_TEST);
}

nanogui::Matrix4f MyGLCanvas::updateMVP() {
    Matrix4f translation;
    translation.setIdentity();
    translation.block(0, 3, 3, 3) = mTransVec;

    Matrix4f scaling;
    scaling.setIdentity();
    scaling.topLeftCorner<3,3>() = mScaleVec.asDiagonal();

    Matrix4f rotation;
    rotation.setIdentity();
    rotation.topLeftCorner<3,3>() = Matrix3f(Eigen::AngleAxisf(mRotatVec[0], Eigen::Vector3f::UnitX()) *
                                             Eigen::AngleAxisf(mRotatVec[1], Eigen::Vector3f::UnitY()) *
                                             Eigen::AngleAxisf(mRotatVec[2], Eigen::Vector3f::UnitZ())) * 0.25f;

    auto mvp = translation * rotation * scaling; 
    return mvp;
}

void MyGLCanvas::updateWingEdge(MatrixXf &positions, MatrixXu &indice,
                 vector<Vertex> &vertices, vector<Face> &faces, vector<W_edge> &edges) {

    //< Winged Edge Structure>
    vertices = vector<Vertex>(positions.cols()); 
    faces = vector<Face>(indice.cols());
    edges = vector<W_edge>(3*indice.cols()); 

    // [step1] Initialize Vertex from positions
    for (int v = 0; v < vertices.size(); v++) { 
        vertices[v].x = positions.col(v)[0]; 
        vertices[v].y = positions.col(v)[1]; 
        vertices[v].z = positions.col(v)[2]; 
    }
    
    // [step2] Initialize Face and Edges on Right Face
    for (int f = 0; f < faces.size(); f++) {
        // position(vertex) index of each face
        int v_idx[3] = {indice.col(f)[0], indice.col(f)[1], indice.col(f)[2]};
        // edge index of each face
        int e_idx[3] = {3*f + 0, 3*f + 1, 3*f + 2};
        
        // face
        faces[f].edge = &edges[e_idx[0]]; // first edge of each triangle
        //claculate Face normal
        //  triangle ( v1, v2, v3 )
        //  edge1 = v2-v1
        //  edge2 = v3-v1
        //  triangle.normal = cross(edge1, edge2).normalize     
            
        Vector3f v1 = positions.col(v_idx[0]);
        Vector3f v2 = positions.col(v_idx[1]);
        Vector3f v3 = positions.col(v_idx[2]);
        
        faces[f].normal = (v2-v1).cross(v3-v1).normalized();
     
        // vertices & edges
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
    for (int e = 0; e < edges.size(); e++) {
        for (int ce = edges.size()-1; ce >= 0; ce--) {
            if(edges[e].start == edges[ce].end && edges[e].end == edges[ce].start) {
                edges[e].left       = edges[ce].right;
                edges[e].left_next  = edges[ce].right_next;
                edges[e].left_prev  = edges[ce].right_prev;
                break;
            }
        }
    }
    
    //[step4] Add Vertex normal 
    for (int vn = 0; vn < vertices.size(); vn++) {
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
            // vertex v1, v2, v3, ....
            // triangle tr1, tr2, tr3 // all share vertex v1
            // v1.normal = normalize( tr1.normal + tr2.normal + tr3.normal )
            normal += faceEdge->right->normal;
        
            // Stop if it sees the initial edge
            if(*e == *e0)  break;
            if(count > 36) {cout << " countover " << endl; break;} // no inf
            count ++;
      }
      // update vertex normal 
      v->normal = normal.normalized(); 
    }
}


void MyGLCanvas::updateMeshes() {
    // update mPositions, mNormals based on WingedEdge data
    int n_face = mFaces.size();
    mPositions = MatrixXf(3, 3*n_face + n_face*3*2);
    mVNormals  = MatrixXf(3, 3*n_face + n_face*3*2);    // vertex normal for smooth shading
    mFNormals  = MatrixXf(3, 3*n_face + n_face*3*2);    // vertex normal for flat shading
    mColors    = MatrixXf(3, 3*n_face + n_face*3*2);
    int f_idx = 0;   // face
    int l_idx =  3*n_face + 0; // wire 
    nanogui::Vector3f f_color(0, 1, 0);
    nanogui::Vector3f l_color(0, 0, 0);

    for (int face = 0; face < n_face; face++) {
        Face *f = &mFaces[face]; 
        Vertex* v_list[3]  = {f->edge->start, f->edge->end, f->edge->right_next->end};  

        for (int i = 0; i < 3; i++) {
            mPositions.col(f_idx) =  Vector3f(v_list[i]->x, v_list[i]->y, v_list[i]->z);
            mVNormals.col(f_idx)  = v_list[i]->normal; 
            mFNormals.col(f_idx)  = f->normal;
            mColors.col(f_idx)    = f_color;
            f_idx++;
        }
        
        for (int l = 0; l < 3; l++) {
            mPositions.col(l_idx) = Vector3f(v_list[l]->x, v_list[l]->y, v_list[l]->z) + f->normal*0.002;
            mVNormals.col(l_idx)  = v_list[l]->normal; 
            mFNormals.col(l_idx)  = f->normal;
            mColors.col(l_idx)    = l_color;
            l_idx++;
            mPositions.col(l_idx) = Vector3f(v_list[(l+1)%3]->x, v_list[(l+1)%3]->y, v_list[(l+1)%3]->z) + f->normal*0.002;
            mVNormals.col(l_idx)  = v_list[(l+1)%3]->normal; 
            mFNormals.col(l_idx)  = f->normal;
            mColors.col(l_idx)    = l_color;
            l_idx++;
        }
    }
    mNormals = mFNormals; // default : flat shading
}

void MyGLCanvas::updateNewMesh(MatrixXf &positions, MatrixXu &indice) {
    // 1. Need to scale meshes into [-1, 1] (Centered)
    Vector3f max = positions.rowwise().maxCoeff();
    Vector3f min = positions.rowwise().minCoeff();

    for (int p = 0; p < positions.cols(); p++) {
        positions.col(p)[0] = 2 * (positions.col(p)[0]-min[0])/(max[0]-min[0]) -1;
        positions.col(p)[1] = 2 * (positions.col(p)[1]-min[1])/(max[1]-min[1]) -1;
        positions.col(p)[2] = 2 * (positions.col(p)[2]-min[2])/(max[2]-min[2]) -1;
    }

    // 2. Update Winged Edge DataStructure
    updateWingEdge(positions, indice, mVertices, mFaces, mEdges);
    
    // 3. Update Mesh data
    updateMeshes(); 
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

void MyGLCanvas::getObjForm(string file_path, MatrixXf &positions, MatrixXu &indices) {
    // positions
    positions = MatrixXf(3, mVertices.size());
    for (int i = 0; i < mVertices.size(); i++) {
        positions.col(i) << mVertices[i].x, mVertices[i].y, mVertices[i].z; 
    }
    // indicies 
    indices = MatrixXu(3, mFaces.size());
    for (int i = 0; i < mFaces.size(); i++) {
        indices.col(i) <<  ptr2idx(mFaces[i].edge->start, &mVertices[0]), 
                        ptr2idx(mFaces[i].edge->end, &mVertices[0]), 
                        ptr2idx(mFaces[i].edge->right_next->end, &mVertices[0]);  
    }


   std::ofstream file;

  file.open (file_path.c_str());
  file << '#'<<' '<< positions.cols() <<' '<< indices.cols() << endl;
  for (int p = 0; p < positions.cols(); p++) {
      file << 'v' <<' '<< positions(0, p)<<' '<< positions(1, p) <<' '<< positions(2, p) << endl;
  }
  for (int i = 0; i < mFaces.size(); i++) {
      file << 'f' <<' '<< ptr2idx(mFaces[i].edge->start, &mVertices[0]) + 1 <<' '<< ptr2idx(mFaces[i].edge->end, &mVertices[0]) + 1 <<' '<< ptr2idx(mFaces[i].edge->right_next->end, &mVertices[0]) + 1 << endl;
  }
  file.close();

}