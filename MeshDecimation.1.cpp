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
                          -1.0 * faceEdge->right->normal.dot(Vector3f(v->x, v->y, v->z))
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

    cout <<" operation starts ...................... " << endl; 
    cout <<" size of mVertices: " << mVertices.size() << endl;
    cout <<" size of mEdges: " << mEdges.size() << endl;
    cout <<" size of mFaces: " << mFaces.size() << endl;

    // [Step0] Need to check target number is less then mVertices 


    // [Step1] choose minimum quadric aomng random k edges -> upto n_decimate ? (using dirty(adj))
    //srand (time(NULL));  // init random seed  
    srand (0); 
    float min_cost = numeric_limits<float>::max();
    int v_update(0); 
    Vertex v_new; 

    for (int k = 0; k < mRandomK; k++) {
        int r_idx = rand() % mVertices.size();
        W_edge *r_edge = mVertices[r_idx].edge;
        cout <<"Choosen Edge1: " << r_edge->start->x << r_edge->start->y <<r_edge->start->z<< endl;
        cout <<"Choosen Edge2: " << r_edge->end->x << r_edge->end->y <<r_edge->end->z<< endl;

        // calculate vertex error quadric  
        Matrix4f q1 = calcVertexQuadric(r_edge->start);
        Matrix4f q2 = calcVertexQuadric(r_edge->end);

        // find optimal contraction target v' : v'.transpose()*(Q1+Q2)*V', where(v1, v2)
        // solve the linear equation : A*v_opt = b; 
        Matrix4f A = q1 + q2; 
        A.row(3) = Vector4f(0.0, 0.0, 0.0, 1.0); 
        Vector4f b; 
        b << 0.0, 0.0, 0.0, 1.0; 
        Vector4f v_opt;
        cout << "determinant : " << A.determinant() << endl; 
        if (A.determinant() > 0.0001) { // check if invertable
            v_opt = A.inverse() * b;
            cout << "V_Optimal Point : " << v_opt << endl; 
        }
        else { // set v_opt as midpoint of v1 and v2 
            v_opt(0) = 0.5*(r_edge->start->x + r_edge->end->x); 
            v_opt(1) = 0.5*(r_edge->start->y + r_edge->end->y); 
            v_opt(2) = 0.5*(r_edge->start->z + r_edge->end->z); 
            v_opt(3) = 1.0; 
        }

        // calculate cost 
        float cost = (v_opt.transpose()*(q1+q2)*v_opt).value();
        cout << "Cost Point : " << cost << endl; 
        // get min cost 
        if (cost < min_cost) {
            min_cost = cost; 
            v_update = r_idx; 
            v_new.x = v_opt(0)/v_opt(3); 
            v_new.y = v_opt(1)/v_opt(3); 
            v_new.z = v_opt(2)/v_opt(3); 

            cout << "v_new(x) : " << v_opt(0) << endl; 
            cout << "v_new(x) : " << v_opt(1)<< endl; 
            cout << "v_new(x) : " << v_opt(2) << endl; 
            cout << "v_new(x) : " << v_opt(3) << endl; 


            // If new vertex is fail, half edge collapse
            
            if (isinf(v_new.x) || isnan(v_new.x) || 
                isinf(v_new.y) || isnan(v_new.y) || 
                isinf(v_new.z) || isnan(v_new.z)) { 
                cout << " New position of Vertex has Inf / Nan !!!!!!!!!!!!!!!!" << endl;
                v_new.x = r_edge->start->x;
                v_new.y = r_edge->start->y;
                v_new.z = r_edge->start->z;
            }
        }
    }

                //////////test
            W_edge *r_edge = mVertices[v_update].edge;
            v_new.x = r_edge->start->x;
            v_new.y = r_edge->start->y;
            v_new.z = r_edge->start->z;

            cout << "v_start : " << r_edge->start->x << " " <<  r_edge->start->y << " " <<  r_edge->start->z << endl; 
            cout << "v_end : " << r_edge->end->x << " " <<  r_edge->end->y << " " <<  r_edge->end->z << endl; 


    cout << "v_new : " << v_new.x << " " <<  v_new.y << " " <<  v_new.z << endl; 

    //[Step 2] Edge collapse and update local new vertex (vertex normal and face normal)
    // (1) find edges and faces for updating 
    vector<int> e_update; 
    vector<int> f_update; 
    findAdjecents(mVertices[v_update].edge, e_update, f_update); 
    findAdjecents(mVertices[v_update].edge->left_prev->right_next, e_update, f_update); 

    // (2) find decimate targets (Index) : vertex(1), edges(3*2), faces(2)  
    vector<int> v_decimate(1);  
    v_decimate[0] = ptr2idx(mVertices[v_update].edge->end, &mVertices[0]); 
    cout << "v_decimate: " ;
    for (auto i: v_decimate) cout << i << ' ';
    cout << endl; 

    vector<int> e_decimate(3*2);
    e_decimate[0] = ptr2idx(mVertices[v_update].edge, &mEdges[0]);  // Collapse
    e_decimate[1] = ptr2idx(mVertices[v_update].edge->left_prev, &mEdges[0]); // Duplicate after Collapse
    e_decimate[2] = ptr2idx(mVertices[v_update].edge->right_next, &mEdges[0]); // Duplicate after Collapse
    e_decimate[3] = ptr2idx(mEdges[e_decimate[0]].left_prev->right_next, &mEdges[0]);  //counter
    e_decimate[4] = ptr2idx(mEdges[e_decimate[1]].left_prev->right_next, &mEdges[0]);  //counter
    e_decimate[5] = ptr2idx(mEdges[e_decimate[2]].left_prev->right_next, &mEdges[0]);  //counter

    // delete e_decimate which are in adjecents 
    for (int eu = e_update.size()-1; eu >=0 ; eu--) { // erase should be deleted from back 
        for (int ed = 0; ed < e_decimate.size(); ed++) {
            if (e_update[eu] == e_decimate[ed]) { e_update.erase(e_update.begin() + eu);}
        }
    }
    

    for(auto i: e_decimate) {
        cout << "Edge Decimate: "<< i  << " : " << ptr2idx(mEdges[i].start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mEdges[i].end,  &mVertices[0])  << endl;
    }

    for(auto i: e_update) {
        cout << "Edge Update: "<< i  << " : " << ptr2idx(mEdges[i].start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mEdges[i].end,  &mVertices[0])  << endl;
    }

    cout << "Face update index: ";
    for (int f_idx : f_update) { cout << f_idx << ", "; }
    cout << endl;


    vector<int> f_decimate(2);
    f_decimate[0] = ptr2idx(mVertices[v_update].edge->right, &mFaces[0]); 
    f_decimate[1] = ptr2idx(mVertices[v_update].edge->left, &mFaces[0]); 
    cout << "f_decimate: ";
    for (auto i: f_decimate) {std::cout << i << ' ';}
    cout << endl; 


    // for(int i=0; i <mFaces.size(); i++) {
    //     cout << "Face "<< i  << " : " << ptr2idx(mFaces[i].edge->start,  &mVertices[0]) << ", " 
    //                                   << ptr2idx(mFaces[i].edge->end,  &mVertices[0]) << ", "
    //                                   << ptr2idx(mFaces[i].edge->right_next->end,  &mVertices[0]) << endl;
    // }
    // for(int i=0; i <mEdges.size(); i++) {
    //     cout << "Edge "<< i  << " : " << ptr2idx(mEdges[i].start,  &mVertices[0]) << ", " 
    //                                   << ptr2idx(mEdges[i].end,  &mVertices[0])  << endl;
    // }
    for(int i=0; i <mFaces.size(); i++) {
        cout << "Face "<< i  << " : " << ptr2idx(mFaces[i].edge->start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mFaces[i].edge->end,  &mVertices[0]) << ", "
                                      << ptr2idx(mFaces[i].edge->right_next->end,  &mVertices[0]) << " | "
                                      << ptr2idx(mFaces[i].edge, &mEdges[0]) << endl;
    }

    // (3) update connections for alive edges 
    for (int e_idx : e_update) { 
        // update winged edge connections for decimate vertex 
        if (*(mEdges[e_idx].start) == mVertices[v_decimate[0]]){
            mEdges[e_idx].start = &mVertices[v_update]; 
            cout << "Edge(Start)"<< e_idx  << " : " << ptr2idx(mEdges[e_idx].start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mEdges[e_idx].end,  &mVertices[0])  << endl;
        }
        if (*(mEdges[e_idx].end) == mVertices[v_decimate[0]]){
            mEdges[e_idx].end = &mVertices[v_update]; 
            cout << "Edge(End) "<< e_idx  << " : " << ptr2idx(mEdges[e_idx].start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mEdges[e_idx].end,  &mVertices[0])  << endl;
        }  
        // update winged edge connections for decimate faces
        if (*(mEdges[e_idx].right) == mFaces[f_decimate[0]] || *(mEdges[e_idx].right) == mFaces[f_decimate[1]]) {
            W_edge * er = mEdges[e_idx].right_next; 
            if (*(er->left) == mFaces[f_decimate[0]] || *(er->left) == mFaces[f_decimate[1]]) {
                mEdges[e_idx].right = er->right_next->left; 
                mEdges[e_idx].right_next = er->right_next->left_next; 
                mEdges[e_idx].right_prev = er->right_next->left_prev;
            } else {
                mEdges[e_idx].right = er->left; 
                mEdges[e_idx].right_next = er->left_next; 
                mEdges[e_idx].right_prev = er->left_prev; 
            }
            // update right face which have an decimate edge with alive one 
            mEdges[e_idx].right->edge = mEdges[e_idx].right_next;
            // update vertex which might have an decimate edge with alive one 
            mEdges[e_idx].end->edge = mEdges[e_idx].right_next;

            cout << "Edge(R) "<< e_idx  << " : " << ptr2idx(mEdges[e_idx].start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mEdges[e_idx].end,  &mVertices[0])  << endl;
            int temp_i =  ptr2idx(mEdges[e_idx].right->edge, &mEdges[0]); 
            cout << "Face: "<< ptr2idx(mEdges[e_idx].right, &mFaces[0])
                 << " with Edge "<< temp_i  << " : " << ptr2idx(mEdges[temp_i].start,  &mVertices[0]) << ", " 
                                                    << ptr2idx(mEdges[temp_i].end,  &mVertices[0])  << endl;
            temp_i =  ptr2idx(mEdges[e_idx].end->edge, &mEdges[0]); 
            cout << "Vertex: "<< ptr2idx(mEdges[e_idx].end, &mVertices[0])
                 << " with Edge "<< temp_i  << " : " << ptr2idx(mEdges[temp_i].start,  &mVertices[0]) << ", " 
                                                    << ptr2idx(mEdges[temp_i].end,  &mVertices[0])  << endl;
                                
        }
        if (*(mEdges[e_idx].left) == mFaces[f_decimate[0]] || *(mEdges[e_idx].left) == mFaces[f_decimate[1]]) {
            W_edge * er = mEdges[e_idx].left_next; 
            if (*(er->left) == mFaces[f_decimate[0]] || *(er->left) == mFaces[f_decimate[1]]) {
                mEdges[e_idx].left = er->right_next->left; 
                mEdges[e_idx].left_next = er->right_next->left_next; 
                mEdges[e_idx].left_prev = er->right_next->left_prev;
            } else {
                mEdges[e_idx].left = er->left; 
                mEdges[e_idx].left_next = er->left_next; 
                mEdges[e_idx].left_prev = er->left_prev; 
            }
            // update left face which has un decimate edge with alive one 
            mEdges[e_idx].left->edge = mEdges[e_idx].left_next;
            // update vertex which might have an decimate edge with alive one 
            mEdges[e_idx].start->edge = mEdges[e_idx].left_next;


            cout << "Edge(L) "<< e_idx  << " : " << ptr2idx(mEdges[e_idx].start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mEdges[e_idx].end,  &mVertices[0])  << endl;
            int temp_i =  ptr2idx(mEdges[e_idx].left->edge, &mEdges[0]); 
            cout << "Face: "<< ptr2idx(mEdges[e_idx].left, &mFaces[0])
                 << " with Edge "<< temp_i  << " : " << ptr2idx(mEdges[temp_i].start,  &mVertices[0]) << ", " 
                                                    << ptr2idx(mEdges[temp_i].end,  &mVertices[0])  << endl;
            temp_i =  ptr2idx(mEdges[e_idx].start->edge, &mEdges[0]); 
            cout << "Vertex: "<< ptr2idx(mEdges[e_idx].end, &mVertices[0])
                 << " with Edge "<< temp_i  << " : " << ptr2idx(mEdges[temp_i].start,  &mVertices[0]) << ", " 
                                                    << ptr2idx(mEdges[temp_i].end,  &mVertices[0])  << endl;
        }
    }
    // for (int e_idx : e_decimate) {
    //     if (*(mEdges[e_idx].start) == mVertices[v_decimate[0]]){
    //         mEdges[e_idx].start = &mVertices[v_update]; 
    //         cout << "Edge update " << e_idx <<" :    " << mEdges[e_idx].start->x <<' ' << mEdges[e_idx].start->y << ' ' << mEdges[e_idx].start->z << endl;
    //     }
    //     if (*(mEdges[e_idx].end) == mVertices[v_decimate[0]]){
    //         mEdges[e_idx].end = &mVertices[v_update]; 
    //         cout << "Edge update " << e_idx <<" :    " << mEdges[e_idx].start->x <<' ' << mEdges[e_idx].start->y << ' ' << mEdges[e_idx].start->z << endl;
    //     }  
    // }

    
    //(4) update connections for alive vertices and faces 
    for (int e_idx : e_decimate) {
        // // check if the decimate edge is refered by Vertex
        // if (mEdges[e_idx] == *(mEdges[e_idx].start->edge)) {
        //     mEdges[e_idx].start->edge = (mEdges[e_idx].right_prev)->left_prev->right_next; 
        //     cout << "Vertex update at Edge" << e_idx <<" :    " << mEdges[e_idx].start->edge->start->x <<' ' << mEdges[e_idx].start->edge->start->y << ' ' << mEdges[e_idx].start->edge->start->z << endl;
        // }

        // // check if the decimate edge is refered by face
        // if (mEdges[e_idx] == *(mEdges[e_idx].right->edge)) {
        //     W_edge *er = mEdges[e_idx].left_prev; 
        //     if(*er == mEdges[e_decimate[0]] || *er == mEdges[e_decimate[3]]){ // one of collapse edge
        //         er = mEdges[e_idx].left_next; 
        //     }
        //     mEdges[e_idx].right->edge = er;  
        //     cout << "Face(R) "<< ptr2idx(mEdges[e_idx].right, &mFaces[0])   << " : "
        //          <<" Update at Edge " << e_idx <<" :    " 
        //                               << ptr2idx(mEdges[e_idx].start,  &mVertices[0]) << ", " 
        //                               << ptr2idx(mEdges[e_idx].end,  &mVertices[0])  << endl;
        // }
        // if (mEdges[e_idx] == *(mEdges[e_idx].left->edge)) {
        //     W_edge *el = (mEdges[e_idx].right_next)->left_prev->right_next;
        //     if(*el == mEdges[e_decimate[0]] || *el == mEdges[e_decimate[3]]){ // one of collapse edge
        //         el = (mEdges[e_idx].right_prev)->left_prev->right_next;
        //     }
        //     mEdges[e_idx].left->edge = el; 
        //     cout << "Face(L) "<<  ptr2idx(mEdges[e_idx].left, &mFaces[0]) <<" Update at Edge " << e_idx <<" :    " << mEdges[e_idx].left->edge->start->x <<' ' << mEdges[e_idx].left->edge->start->y << ' ' << mEdges[e_idx].left->edge->start->z << endl;
        // }

        // // check if the decimate faces is refered by edge
        // if (*(mEdges[e_idx].right) == mFaces[f_decimate[0]] || *(mEdges[e_idx].right) == mFaces[f_decimate[1]]) {
        //     mEdges[e_idx].right = mEdges[e_idx].right_next->left; 
        // }
        // if (*(mEdges[e_idx].left) == mFaces[f_decimate[0]] || *(mEdges[e_idx].left) == mFaces[f_decimate[1]]) {
        //     mEdges[e_idx].left = mEdges[e_idx].left_prev->left; 
        // }
    }


    // (5) Switch old vertex to new vertex
    mVertices[v_update].x = v_new.x; 
    mVertices[v_update].y = v_new.y; 
    mVertices[v_update].z = v_new.z; 
    for (int e_idx : e_update) { 
        if (*(mEdges[e_idx].start) == mVertices[v_decimate[0]]){ 
            mVertices[v_update].edge = &mEdges[e_idx];
            break;
        }
    }
    for(int i=0; i <mVertices.size(); i++) {
        cout << "Vertex "<< i << " :  " << mVertices[i].x << ", " << mVertices[i].y << ", "<< mVertices[i].z
        << " | "<< ptr2idx(mVertices[i].edge, &mEdges[0]) << endl;

   }
    
    for(int i=0; i <mFaces.size(); i++) {
        cout << "Face "<< i  << " : " << ptr2idx(mFaces[i].edge->start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mFaces[i].edge->end,  &mVertices[0]) << ", "
                                      << ptr2idx(mFaces[i].edge->right_next->end,  &mVertices[0]) << " | "
                                      << ptr2idx(mFaces[i].edge, &mEdges[0]) << endl;
    }
    // (4) decimate targets <- deleting elements affect to index system.......
    mVertices.erase(mVertices.begin() + v_decimate[0]); 
    for(int i=0; i <mEdges.size(); i++) {
        cout << "Edge "<< i  << " : " << ptr2idx(mEdges[i].start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mEdges[i].end,  &mVertices[0])  << endl;
    }
    cout << "Decimate Face 11  : "<< ptr2idx(mFaces[11].edge->start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mFaces[11].edge->end,  &mVertices[0]) << ", "
                                      << ptr2idx(mFaces[11].edge->right_next->end,  &mVertices[0]) << " | "
                                      << ptr2idx(mFaces[11].edge, &mEdges[0]) << endl;
    
    sort(e_decimate.begin(), e_decimate.end(), greater<int>()); 
    for(int e_idx: e_decimate) { 
        mEdges.erase(mEdges.begin() + e_idx);
        
        cout << "Decimate "<<e_idx << "   \n"; 
        cout << "Decimate Face 11  : "<< ptr2idx(mFaces[11].edge->start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mFaces[11].edge->end,  &mVertices[0]) << ", "
                                      << ptr2idx(mFaces[11].edge->right_next->end,  &mVertices[0]) << " | "
                                      << ptr2idx(mFaces[11].edge, &mEdges[0]) << endl;
        }


    sort(f_decimate.begin(), f_decimate.end(), greater<int>()); 
    for(int f_idx: f_decimate) { mFaces.erase(mFaces.begin() + f_idx);}

    // (5) update normal info for faces and vertices
    Vector3f vnormal (0.0, 0.0, 0.0); 
    for (int fn = 0; fn < f_update.size(); fn++) {
        Vector3f v1 (mFaces[fn].edge->start->x, mFaces[fn].edge->start->y, mFaces[fn].edge->start->z);  
        Vector3f v2 (mFaces[fn].edge->end->x, mFaces[fn].edge->end->y, mFaces[fn].edge->end->z);  
        Vector3f v3 (mFaces[fn].edge->right_next->end->x, mFaces[fn].edge->right_next->end->y, mFaces[fn].edge->right_next->end->z);  
        mFaces[fn].normal = (v2-v1).cross(v3-v1).normalized();
        vnormal += mFaces[fn].normal; 
    }
    mVertices[v_update].normal = vnormal.normalized(); 
   
    //[Step 3] Update Mesh Data     
    cout << "Final Vertex : " << endl; 
    for(int i=0; i <mVertices.size(); i++) {
        cout << "Vertex "<< i << " :  " << mVertices[i].x << ", " << mVertices[i].y << ", "<< mVertices[i].z
        << " | "<< ptr2idx(mVertices[i].edge, &mEdges[0]) << endl;

   }
    for(int i=0; i <mEdges.size(); i++) {
        cout << "Edge "<< i  << " : " << ptr2idx(mEdges[i].start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mEdges[i].end,  &mVertices[0])  << endl;
    }
    for(int i=0; i <mFaces.size(); i++) {
        cout << "Face "<< i  << " : " << ptr2idx(mFaces[i].edge->start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mFaces[i].edge->end,  &mVertices[0]) << ", "
                                      << ptr2idx(mFaces[i].edge->right_next->end,  &mVertices[0]) << " | "
                                      << ptr2idx(mFaces[i].edge, &mEdges[0]) << endl;
    }


    
    updateMeshes(); 


    cout <<" size of mVertices: " << mVertices.size() << endl;
    cout <<" size of mEdges: " << mEdges.size() << endl;
    cout <<" size of mFaces: " << mFaces.size() << endl;
    cout <<" operation is done ......................" << endl; 

}
    