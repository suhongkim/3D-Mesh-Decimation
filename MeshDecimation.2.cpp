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
            v_new.x = 0.3*(r_edge->start->x + r_edge->end->x); 
            v_new.y = 0.3*(r_edge->start->y + r_edge->end->y); 
            v_new.z = 0.3*(r_edge->start->z + r_edge->end->z); 

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
    e_decimate[1] = ptr2idx(mEdges[e_decimate[0]].left_prev->right_next, &mEdges[0]);  //counter
    e_decimate[2] = ptr2idx(mVertices[v_update].edge->left_prev, &mEdges[0]); // Duplicate after Collapse
    e_decimate[3] = ptr2idx(mEdges[e_decimate[2]].left_prev->right_next, &mEdges[0]);  //counter
    e_decimate[4] = ptr2idx(mVertices[v_update].edge->right_next, &mEdges[0]); // Duplicate after Collapse    
    e_decimate[5] = ptr2idx(mEdges[e_decimate[4]].left_prev->right_next, &mEdges[0]);  //counter

    // delete e_decimate which are in adjecents 
    for (int eu = e_update.size()-1; eu >=0 ; eu--) { // erase should be deleted from back 
        for (int ed = 0; ed < e_decimate.size(); ed++) {
            if (e_update[eu] == e_decimate[ed]) { e_update.erase(e_update.begin() + eu);}
        }
    }
    
    vector<int> f_decimate(2);
    f_decimate[0] = ptr2idx(mVertices[v_update].edge->right, &mFaces[0]); 
    f_decimate[1] = ptr2idx(mVertices[v_update].edge->left, &mFaces[0]); 
    cout << "f_decimate: ";
    for (auto i: f_decimate) {std::cout << i << ' ';}
    cout << endl; 


    

    // (3) collapse edge

    W_edge *ed; 
    W_edge *eu; 
    
    // 2nd decimate edge : (collapse edge -> left_prev) 
    ed = &mEdges[e_decimate[2]];
    // set update edge
    eu = (ed->right_prev)->left_prev->right_next;
    cout << "Before update ------------------------" << endl;
    printWEdge(ed, &mEdges[0], &mVertices[0], &mFaces[0]);
    printWEdge(eu, &mEdges[0], &mVertices[0], &mFaces[0]);
    // vertex matching 
    eu->start->edge = eu; // start vertex edge init
    ed->start = eu->start; 
    ed->end = eu->end; 
    // ed right side matching 
    ed->right = eu->right; 
    ed->right_next = eu->right_next; 
    ed->right_prev = eu->right_prev; 
    // eu left side matching 
    ed->left->edge = ed->left_next; // left face edge init 
    eu->left = ed->left; 
    eu->left_next = ed->left_next; 
    eu->left_prev = ed->left_prev; 
    // update left neighbour edge 
    ed->left_next->right_prev = (eu)->left_prev->right_next; //eu counter
    ((ed->left_next)->left_prev->right_next)->left_prev = (eu)->left_prev->right_next; //counter
    ed->left_prev->right_next = (eu)->left_prev->right_next; //eu counter
    ((ed->left_prev)->left_prev->right_next)->left_next = (eu)->left_prev->right_next;
    cout << "After update ------------------------" << endl;
    printWEdge(ed, &mEdges[0], &mVertices[0], &mFaces[0]);
    printWEdge(eu, &mEdges[0], &mVertices[0], &mFaces[0]);
    cout << "Face "<< 7  << " : " << ptr2idx(mFaces[7].edge->start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mFaces[7].edge->end,  &mVertices[0]) << ", "
                                      << ptr2idx(mFaces[7].edge->right_next->end,  &mVertices[0]) << " | "
                                      << ptr2idx(mFaces[7].edge, &mEdges[0]) << endl;

    cout << "------done---------------------------" << endl;

    // 3nd decimate edge : counter(collapse edge -> left_prev)   
    ed = &mEdges[e_decimate[3]]; 
    // set update edge
    eu = ed->left_prev; 
    cout << "Before update ------------------------" << endl;
    printWEdge(ed, &mEdges[0], &mVertices[0], &mFaces[0]);
    printWEdge(eu, &mEdges[0], &mVertices[0], &mFaces[0]);
    // vertex matching 
    eu->start->edge = eu; // start vertex edge init
    ed->start = eu->start; 
    ed->end = eu->end;
    // ed left side matching 
    ed->left = eu->left; 
    ed->left_next = eu->left_next; 
    ed->left_prev = eu->left_prev; 
    // eu right side matching 
    ed->right->edge = ed->right_next; // right face edge init 
    eu->right = ed->right; 
    eu->right_next = ed->right_next; 
    eu->right_prev = ed->right_prev; 
    // update right neighbour edge 
    ed->right_next->right_prev = eu; 
    ((ed->right_next)->left_prev->right_next)->left_prev = eu; //counter
    ed->right_prev->right_next = eu; 
    ((ed->right_prev)->left_prev->right_next)->left_next = eu; //counter
    
    cout << "After update ------------------------" << endl;
    printWEdge(ed, &mEdges[0], &mVertices[0], &mFaces[0]);
    printWEdge(eu, &mEdges[0], &mVertices[0], &mFaces[0]);
        cout << "Face "<< 7  << " : " << ptr2idx(mFaces[7].edge->start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mFaces[7].edge->end,  &mVertices[0]) << ", "
                                      << ptr2idx(mFaces[7].edge->right_next->end,  &mVertices[0]) << " | "
                                      << ptr2idx(mFaces[7].edge, &mEdges[0]) << endl;

    cout << "------done---------------------------" << endl;


    // 4th decimate edge : (collapse edge -> right_next) 
    ed = &mEdges[e_decimate[4]];
    // set update edge
    eu = (ed->right_next)->left_prev->right_next;
    cout << "Before update ------------------------" << endl;
    printWEdge(ed, &mEdges[0], &mVertices[0], &mFaces[0]);
    printWEdge(eu, &mEdges[0], &mVertices[0], &mFaces[0]);
    // vertex matching  
    eu->start->edge = eu; // start vertex edge init
    ed->start = eu->start; 
    ed->end = eu->end; 
    // ed right side matching 
    ed->right = eu->right; 
    ed->right_next = eu->right_next; 
    ed->right_prev = eu->right_prev; 
    // eu left side matching
    ed->left->edge = ed->left_next; // left face edge init  
    eu->left = ed->left; 
    eu->left_next = ed->left_next; 
    eu->left_prev = ed->left_prev; 
    // update left neighbour edge 
    ed->left_next->right_prev = (eu)->left_prev->right_next; //eu counter
    ((ed->left_next)->left_prev->right_next)->left_prev = (eu)->left_prev->right_next; //counter
    ed->left_prev->right_next = (eu)->left_prev->right_next; //eu counter
    ((ed->left_prev)->left_prev->right_next)->left_next = (eu)->left_prev->right_next;
    cout << "After update ------------------------" << endl;
    printWEdge(ed, &mEdges[0], &mVertices[0], &mFaces[0]);
    printWEdge(eu, &mEdges[0], &mVertices[0], &mFaces[0]);
        cout << "Face "<< 7  << " : " << ptr2idx(mFaces[7].edge->start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mFaces[7].edge->end,  &mVertices[0]) << ", "
                                      << ptr2idx(mFaces[7].edge->right_next->end,  &mVertices[0]) << " | "
                                      << ptr2idx(mFaces[7].edge, &mEdges[0]) << endl;

    cout << "------done---------------------------" << endl;

    // 5th decimate edge : counter(collapse edge -> right_next)   
    ed = &mEdges[e_decimate[5]]; 
    // set update edge
    eu = ed->left_next; 
    cout << "Before update ------------------------" << endl;
    printWEdge(ed, &mEdges[0], &mVertices[0], &mFaces[0]);
    printWEdge(eu, &mEdges[0], &mVertices[0], &mFaces[0]);
    // vertex matching 
    eu->start->edge = eu; // start vertex edge init
    ed->start = eu->start; 
    ed->end = eu->end; 
    // ed left side matching 
    ed->left = eu->left; 
    ed->left_next = eu->left_next; 
    ed->left_prev = eu->left_prev; 
    // eu right side matching 
    ed->right->edge = ed->right_next; // right face edge init 
    eu->right = ed->right; 
    eu->right_next = ed->right_next; 
    eu->right_prev = ed->right_prev; 
    // update right neighbour edge 
    ed->right_next->right_prev = eu; 
    ((ed->right_next)->left_prev->right_next)->left_prev = eu; //counter
    ed->right_prev->right_next = eu; 
    ((ed->right_prev)->left_prev->right_next)->left_next = eu; //counter
    cout << "After update ------------------------" << endl;
    printWEdge(ed, &mEdges[0], &mVertices[0], &mFaces[0]);
    printWEdge(eu, &mEdges[0], &mVertices[0], &mFaces[0]);
        cout << "Face "<< 7  << " : " << ptr2idx(mFaces[7].edge->start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mFaces[7].edge->end,  &mVertices[0]) << ", "
                                      << ptr2idx(mFaces[7].edge->right_next->end,  &mVertices[0]) << " | "
                                      << ptr2idx(mFaces[7].edge, &mEdges[0]) << endl;

    cout << "------done---------------------------" << endl;

    // update left-over edges 
    for(int i_eu = 0; i_eu < e_update.size(); i_eu++) {
        if(*(mEdges[e_update[i_eu]].start) == mVertices[v_decimate[0]]) {
            mEdges[e_update[i_eu]].start = &mVertices[v_update]; 
        }
        if(*(mEdges[e_update[i_eu]].end) == mVertices[v_decimate[0]]) {
            mEdges[e_update[i_eu]].end = &mVertices[v_update]; 
        }
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
    cout << " Decimate -----------------------------------------------" << endl;
    
    sort(e_decimate.begin(), e_decimate.end(), greater<int>()); 
    for(int e_idx: e_decimate) { 
        printWEdge(&mEdges[e_idx], &mEdges[0], &mVertices[0], &mFaces[0]);
        mEdges.erase(mEdges.begin() + e_idx);
        cout << "Decimate "<<e_idx << "   \n"; 

        // update Edge reference for index over e_idx; 
        for (int e=0; e < mEdges.size(); e++) {
            int e_rn = ptr2idx(mEdges[e].right_next, &mEdges[0]); 
            if(e_rn > e_idx) {
                mEdges[e].right_next = &mEdges[--e_rn];
            }
            int e_rp = ptr2idx(mEdges[e].right_prev, &mEdges[0]); 
            if(e_rp > e_idx) {
                mEdges[e].right_prev = &mEdges[--e_rp];
            }
            int e_ln = ptr2idx(mEdges[e].left_next, &mEdges[0]); 
            if(e_ln > e_idx) {
                mEdges[e].left_next = &mEdges[--e_ln];
            }
            int e_lp = ptr2idx(mEdges[e].left_prev, &mEdges[0]); 
            if(e_lp > e_idx) {
                mEdges[e].left_prev = &mEdges[--e_lp];
            }       
        }

        for(int i=0; i <mFaces.size(); i++) {
        cout << "Face "<< i  << " : " << ptr2idx(mFaces[i].edge->start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mFaces[i].edge->end,  &mVertices[0]) << ", "
                                      << ptr2idx(mFaces[i].edge->right_next->end,  &mVertices[0]) << " | "
                                      << ptr2idx(mFaces[i].edge, &mEdges[0]) << endl;
        }
    }
    
    
   
    
    mVertices.erase(mVertices.begin() + v_decimate[0]); 
    // update vertex reference for index over v_decimate[0]; 
    for (int e=0; e < mEdges.size(); e++) {
        int v_start = ptr2idx(mEdges[e].start, &mVertices[0]); 
        if(v_start > v_decimate[0]) {
            mEdges[e].start = &mVertices[--v_start];
        }
        int v_end = ptr2idx(mEdges[e].end, &mVertices[0]); 
        if(v_end > v_decimate[0]) {
            mEdges[e].end = &mVertices[--v_end];
       }
    }




    sort(f_decimate.begin(), f_decimate.end(), greater<int>()); 
    for(int f_idx: f_decimate) { 
        mFaces.erase(mFaces.begin() + f_idx);
        // update Face reference for index over f_decimate[0]; 
        for (int e=0; e < mEdges.size(); e++) {
            int f_right = ptr2idx(mEdges[e].right, &mFaces[0]); 
            if(f_right > f_idx) {
                mEdges[e].right = &mFaces[--f_right];
            }
            int f_left = ptr2idx(mEdges[e].left, &mFaces[0]); 
            if(f_left > f_idx) {
                mEdges[e].left = &mFaces[--f_left];
            }
        }
    }

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
   
     
    cout << "Final Vertex : " << endl; 
    for(int i=0; i <mVertices.size(); i++) {
        cout << "Vertex "<< i << " :  " << mVertices[i].x << ", " << mVertices[i].y << ", "<< mVertices[i].z
        << " | "<< ptr2idx(mVertices[i].edge, &mEdges[0]) << endl;

    }

    for (int i=0; i<mEdges.size(); i++) {
        printWEdge(&mEdges[13], &mEdges[0], &mVertices[0], &mFaces[0]);
    }
    
    for(int i=0; i <mFaces.size(); i++) {
        cout << "Face "<< i  << " : " << ptr2idx(mFaces[i].edge->start,  &mVertices[0]) << ", " 
                                      << ptr2idx(mFaces[i].edge->end,  &mVertices[0]) << ", "
                                      << ptr2idx(mFaces[i].edge->right_next->end,  &mVertices[0]) << " | "
                                      << ptr2idx(mFaces[i].edge, &mEdges[0]) << endl;
    }


    //[Step 3] Update Mesh Data
    updateMeshes(); 


    cout <<" size of mVertices: " << mVertices.size() << endl;
    cout <<" size of mEdges: " << mEdges.size() << endl;
    cout <<" size of mFaces: " << mFaces.size() << endl;
    cout <<" operation is done ......................" << endl; 

}
    


