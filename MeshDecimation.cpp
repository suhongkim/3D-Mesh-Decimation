#include "MyGlCanvas_a2.h"

using namespace std; 
using namespace nanogui; 

nanogui::Matrix4f MyGLCanvas::calcVertexQuadric(Vertex * v) {
    // Calculate vertex quadric : sum of fundamental quadric of supporting planes
    W_edge * e0 = v->edge;
    W_edge * e = e0;
    Matrix4f q = Matrix4f::Zero(4,4);  
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
        // f(x) = n.dot(x - v) = 0    // plane eq
        // f(x) = n.dot(x) - n.dot(v) = ax + by + cz + d = 0 
        //  where n = (a, b, c), d = -n.dot(v)
        // homogeneous plane p = (a, b, c, -n.dot(v)) * (x, y, z, 1)
        auto p = Vector4f(faceEdge->right->normal.x(), 
                          faceEdge->right->normal.y(), 
                          faceEdge->right->normal.z(), 
                          1.0 * faceEdge->right->normal.dot(Vector3f(v->x, v->y, v->z))
                        ); 
        // Sum of fundamental quadric for a plane
        q += Matrix4f(p*p.transpose()); 

        // Stop if it sees the initial edge
        if(*e == *e0)  break;
        if(count > 36) {cout << " countover " << endl; break;} // no inf
        count ++;
    }

    return q; 
}

void MyGLCanvas::findAdjecents(W_edge * c_pair, vector<int> &e_olds, vector<int> &f_olds) {
    // find adjecent edges without collapse edge
    W_edge * e0 = c_pair; // collapse edge
    W_edge * e = e0; 
    int count = 0;

    while(1) {        
        //Adjecent edges  
        W_edge * ec; // counter edge 
        if (*(e->end) == *(c_pair->start)) {
            e = e->right_next;  // e.end == v_collapse
            ec = e->left_prev->right_next;  //ec.start == v_collapse
        } else {
            e = e->left_next; // e.start == v_collapse 
            ec = e->left_prev->right_next;  // ec.end == v_collapse 
        }

        if(*e == *e0)  { 
            // Stop if it sees the initial edge     
            cout << "Edge found : "<< count << endl;

            // remove collapse face 
            f_olds.pop_back(); 

            break;
        }
        else{
            // Save the adjecent edge indice except collapse edge 
            e_olds.push_back(ptr2idx(e, &mEdges[0])); 
            e_olds.push_back(ptr2idx(ec, &mEdges[0]));
            // Save the RIGHT face indice on adjecent edge escept collapse edge 
            f_olds.push_back(ptr2idx(e->right, &mFaces[0])); 
        }

        if(count > 36) {cout << " countover " << endl; break;} // no inf
        count ++;
    }
}

void MyGLCanvas::decimateMesh(int n_decimate) {
    // check v_new step by step 
    // check connection 


    cout <<" operation starts ...................... " << endl; 
    cout <<" size of mVertice: " << mVertice.size() << endl;
    cout <<" size of mEdges: " << mEdges.size() << endl;
    cout <<" size of mFaces: " << mFaces.size() << endl;
    
    // [Step1] choose minimum quadric aomng random k edges -> upto n_decimate ? (using dirty(adj))
    srand (time(NULL));  // init random seed  
    float min_cost = numeric_limits<float>::max();
    int i_old(0); 
    Vertex v_new; 

    for (int k = 0; k < mRandomK; k++) {
        int r_idx = rand() % mVertice.size();
        W_edge *r_edge = mVertice[r_idx].edge;

        // calculate vertex error quadric  
        Matrix4f q1 = calcVertexQuadric(r_edge->start);
        Matrix4f q2 = calcVertexQuadric(r_edge->end);
        
        // find optimal contraction target v' : v'.transpose()*(Q1+Q2)*V', where(v1, v2)
        // solve the linear equation : A*v_opt = b; 
        Matrix4f A = q1 + q2; 
        A.row(3) = Vector4f(0.0, 0.0, 0.0, 1.0); 
        MatrixXf b (4, 1); 
        b << 0.0, 0.0, 0.0, 1.0; 
        MatrixXf v_opt (4, 1);
        cout << "determinant : " << A.determinant() << endl; 
        if (A.determinant() > 0.0001) { // check if invertable
            v_opt = A.inverse() * b;
        }
        else { // set v_opt as midpoint of v1 and v2 
            v_opt(0,0) = 0.5*(r_edge->start->x + r_edge->end->x); 
            v_opt(0,1) = 0.5*(r_edge->start->y + r_edge->end->y); 
            v_opt(0,2) = 0.5*(r_edge->start->z + r_edge->end->z); 
            v_opt(0,3) = 1.0; 
        }

        // calculate cost 
        float cost = (v_opt.transpose()*(q1+q2)*v_opt).value();
    
        // get min cost 
        if ((k == 0) || (cost < min_cost)) {
            min_cost = cost; 
            i_old = r_idx; 
            v_new.x = v_opt(0,0)/v_opt(0,3); 
            v_new.y = v_opt(0,1)/v_opt(0,3); 
            v_new.z = v_opt(0,2)/v_opt(0,3); 
        }
    }
    cout << "v_new : " << v_new.x << " " <<  v_new.y << " " <<  v_new.z << endl; 

    //[Step 2] Edge collapse and update local new vertex (vertex normal and face normal)
    // (1) find decimate targets (Index) : vertex(2), edges(3*2), faces(2)  
    vector<int> v_decimate(2);  
    v_decimate[0] = i_old; 
    v_decimate[1] = ptr2idx(mVertice[i_old].edge->end, &mVertice[0]); 
    
    vector<int> e_decimate(3*2);
    e_decimate[0] = ptr2idx(mVertice[i_old].edge, &mEdges[0]);  // Collapse
    e_decimate[1] = ptr2idx(mVertice[i_old].edge->left_prev, &mEdges[0]); // Duplicate after Collapse
    e_decimate[2] = ptr2idx(mVertice[i_old].edge->right_prev, &mEdges[0]); // Duplicate after Collapse
    e_decimate[3] = ptr2idx(mEdges[e_decimate[0]].left_prev->right_next, &mEdges[0]);  //counter
    e_decimate[4] = ptr2idx(mEdges[e_decimate[1]].left_prev->right_next, &mEdges[0]);  //counter
    e_decimate[5] = ptr2idx(mEdges[e_decimate[2]].left_prev->right_next, &mEdges[0]);  //counter

    vector<int> f_decimate(2);
    f_decimate[0] = ptr2idx(mVertice[i_old].edge->right, &mFaces[0]); 
    f_decimate[1] = ptr2idx(mVertice[i_old].edge->left, &mFaces[0]); 

    // (2) find old edges and faces for updating 
    vector<int> e_update; 
    vector<int> f_update; 
    findAdjecents(mVertice[i_old].edge, e_update, f_update); 
    findAdjecents(mVertice[i_old].edge->left_prev->right_next, e_update, f_update); 
    
    // (3) add new vertex and update Edges with new vertex ==> need more connection ?????? 
    mVertice.push_back(v_new);
    for (int e_idx : e_update) { 
        if (*(mEdges[e_idx].start) == mVertice[i_old]){
            mEdges[e_idx].start = &mVertice.back(); 
        }
        if (*(mEdges[e_idx].end) == mVertice[i_old]){
            mEdges[e_idx].end = &mVertice.back(); 
        }  
    }

    // (4) decimate targets
    for (int v_idx : v_decimate) { mVertice.erase(mVertice.begin() + v_idx); }
    for (int e_idx : e_decimate) { mEdges.erase(mEdges.begin() + e_idx); }
    for (int f_idx : f_decimate) { mFaces.erase(mFaces.begin() + f_idx); }

    // (5) update normal info for faces and vertice
    Vector3f vnormal (0.0, 0.0, 0.0); 
    for (int fn = 0; fn < f_update.size(); fn++) {
        Vector3f v1 (mFaces[fn].edge->start->x, mFaces[fn].edge->start->y, mFaces[fn].edge->start->z);  
        Vector3f v2 (mFaces[fn].edge->end->x, mFaces[fn].edge->end->y, mFaces[fn].edge->end->z);  
        Vector3f v3 (mFaces[fn].edge->right_next->end->x, mFaces[fn].edge->right_next->end->y, mFaces[fn].edge->right_next->end->z);  
        mFaces[fn].normal = (v2-v1).cross(v3-v1).normalized();
        vnormal += mFaces[fn].normal; 
    }
    mVertice.back().normal = vnormal.normalized(); 
   
    //[Step 3] Update Mesh Data     
    updateMeshes(); 


    cout <<" size of mVertice: " << mVertice.size() << endl;
    cout <<" size of mEdges: " << mEdges.size() << endl;
    cout <<" size of mFaces: " << mFaces.size() << endl;
    cout <<" operation is done ......................" << endl; 

}
    