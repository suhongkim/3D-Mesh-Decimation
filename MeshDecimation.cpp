#include "MyGlCanvas_a2.h"

using namespace std; 
using namespace nanogui; 

nanogui::Matrix4f MyGLCanvas::calcVertexQuadric(Vertex * v) {
    // check if v has history 
    if (v->isQ) {
        return v->q; 
    }
    
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

void MyGLCanvas::findAdjecents(W_edge * c_pair, vector<int> &e_olds, vector<int> &f_olds, 
                              vector<W_edge> &edges, vector<Face> &faces) {
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
            //cout << "Edge found : "<< count << endl;

            // remove collapse face 
            f_olds.pop_back(); 

            break;
        }
        else{
            // Save the adjecent edge indice except collapse edge 
            e_olds.push_back(ptr2idx(e, &edges[0])); 
            e_olds.push_back(ptr2idx(ec, &edges[0]));
            // Save the RIGHT face indice on adjecent edge escept collapse edge 
            f_olds.push_back(ptr2idx(e->right, &faces[0])); 
        }

        if(count > 36) {cout << " countover " << endl; break;} // no inf
        count ++;
    }
}

void MyGLCanvas::decimateOne(vector<Vertex> &vertices, vector<Face> &faces, vector<W_edge> &edges) {   

    // [Step1] choose minimum quadric aomng random k edges -> upto n_decimate ? (using dirty(adj))
    srand (time(NULL));  // init random seed  
    //srand (0); 
    float min_cost = numeric_limits<float>::max();
    int v_update(0); 
    Vertex v_new; 

    for (int k = 0; k < mRandomK; k++) {
        int r_idx = rand() % vertices.size();
        W_edge *r_edge = vertices[r_idx].edge;


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
        
        if (A.determinant() > 0.0001) { // check if invertable
            v_opt = A.inverse() * b;
        }
        else { // set v_opt as midpoint of v1 and v2 
            v_opt(0) = 0.5*(r_edge->start->x + r_edge->end->x); 
            v_opt(1) = 0.5*(r_edge->start->y + r_edge->end->y); 
            v_opt(2) = 0.5*(r_edge->start->z + r_edge->end->z); 
            v_opt(3) = 1.0; 
        }

        // calculate cost 
        float cost = (v_opt.transpose()*(q1+q2)*v_opt).value();

        // get min cost 
        if (cost < min_cost) {
            min_cost = cost; 
            v_update = r_idx; 
            v_new.x = v_opt(0)/v_opt(3); 
            v_new.y = v_opt(1)/v_opt(3); 
            v_new.z = v_opt(2)/v_opt(3); 
            v_new.isQ = true; 
            v_new.q = q1+q2; 

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
    cout << "v_new : " << v_new.x << " " <<  v_new.y << " " <<  v_new.z << endl; 

    //[Step 2] Edge collapse and update local new vertex (vertex normal and face normal)
    // (1) find edges and faces for updating 
    vector<int> e_adjecent; 
    vector<int> e_update; 
    vector<int> f_update; 
    findAdjecents(vertices[v_update].edge, e_update, f_update, edges, faces); 
    findAdjecents(vertices[v_update].edge->left_prev->right_next, e_update, f_update, edges, faces); 
    e_adjecent = e_update; 
    // (2) find decimate targets (Index) : vertex(1), edges(3*2), faces(2)  
    // vertex(1)
    int v_decimate = ptr2idx(vertices[v_update].edge->end, &vertices[0]); 
    // faces(2)
    vector<int> f_decimate(2);
    f_decimate[0] = ptr2idx(vertices[v_update].edge->right, &faces[0]); 
    f_decimate[1] = ptr2idx(vertices[v_update].edge->left, &faces[0]); 
    // edges(6)
    vector<int> e_decimate(3*2);
    e_decimate[0] = ptr2idx(vertices[v_update].edge, &edges[0]);  // Collapse
    e_decimate[1] = ptr2idx(edges[e_decimate[0]].left_prev->right_next, &edges[0]);  //counter
    e_decimate[2] = ptr2idx(vertices[v_update].edge->left_prev, &edges[0]); // Duplicate after Collapse
    e_decimate[3] = ptr2idx(edges[e_decimate[2]].left_prev->right_next, &edges[0]);  //counter
    e_decimate[4] = ptr2idx(vertices[v_update].edge->right_next, &edges[0]); // Duplicate after Collapse    
    e_decimate[5] = ptr2idx(edges[e_decimate[4]].left_prev->right_next, &edges[0]);  //counter
    // delete e_decimate which are in adjecents 
    for (int eu = e_update.size()-1; eu >=0 ; eu--) { // erase should be deleted from back 
        for (int ed = 0; ed < e_decimate.size(); ed++) {
            if (e_update[eu] == e_decimate[ed]) { e_update.erase(e_update.begin() + eu);}
        }
    }

    cout << "v_decimate: " << v_decimate << endl; 
    cout << "f_decimate: ";
    for (auto i: f_decimate) {std::cout << i << ' ';}
    cout << endl;
    cout << "e_decimate: ";
    for (auto i: e_decimate) {std::cout << i << ' ';}
    cout << endl;
    // cout << "e_update: ";
    // for (auto i: e_update) {std::cout << i << ' ';}

    // (3) collapse edge
    cout << "Let's collapse edge ----" << endl;
    W_edge *ed; 
    W_edge *eu; 
    vector<W_edge> newEdges; 
    vector<int> ed_list; 
    vector<int> eu_list;
    for (auto e_idx : e_decimate) {
        W_edge e_new; // new edge 
        // collapse edge skip
        if (*(edges[e_idx].start) == vertices[v_update] || 
            *(edges[e_idx].end) == vertices[v_update] ) {
            continue; 
        }
        // Right face update 
        if (*(edges[e_idx].right) == faces[f_decimate[0]] || 
            *(edges[e_idx].right) == faces[f_decimate[1]]) {
            ed = &edges[e_idx];
            // set update edge
            if(*(ed->right_prev->end) == vertices[v_decimate]){
                eu = ctrEdge(ed->right_next); 
            } else {
                eu = ctrEdge(ed->right_prev); 
            }
            // vertex matching 
            eu->start->edge = eu; // start vertex edge init
            e_new.start = eu->start; 
            e_new.end = eu->end; 
            // eu right face matching            
            e_new.right = eu->right; 
            e_new.right_next = eu->right_next; 
            e_new.right_prev = eu->right_prev; 
            // ed left face matching 
            (ed->left)->edge = ed->left_next;  // ed left face edge init   
            e_new.left = ed->left; 
            e_new.left_next = ed->left_next; 
            e_new.left_prev = ed->left_prev;
        }
        // Left face update 
        if (*(edges[e_idx].left) == faces[f_decimate[0]] || 
            *(edges[e_idx].left) == faces[f_decimate[1]]) {
            ed = &edges[e_idx];
            // set update edge
            if(*(ed->left_prev->end) == vertices[v_decimate]){
                eu = ed->left_next; 
            } else {
                eu = ed->left_prev; 
            }
            // vertex matching 
            eu->start->edge = eu; // start vertex edge init
            e_new.start = eu->start; 
            e_new.end = eu->end;
            // ed left side matching 
            e_new.left = eu->left; 
            e_new.left_next = eu->left_next; 
            e_new.left_prev = eu->left_prev; 
            // eu right side matching 
            ed->right->edge = ed->right_next; // right face edge init 
            e_new.right = ed->right; 
            e_new.right_next = ed->right_next; 
            e_new.right_prev = ed->right_prev; 

        }
        newEdges.push_back(e_new);
        ed_list.push_back(e_idx);
        eu_list.push_back(ptr2idx(eu, &edges[0]));
    }
    // update newEdges to targetEdges 
    for (int u = 0; u < eu_list.size(); u++) {
        edges[eu_list[u]] = newEdges[u]; 
    }
    
    // update neighbor connectivity
    for (int d = 0; d < ed_list.size(); d++) {
        W_edge *e_dec = &edges[ed_list[d]]; 
        W_edge *e_new = &edges[eu_list[d]]; 
        // skip if collapse edge 
        if(*(e_dec->right_next->end) == vertices[v_update] ||
           *(e_dec->right_prev->start) == vertices[v_update]) {
           continue;
        }
        vector<W_edge * > en_list; 
        en_list.push_back(e_dec->right_next);
        en_list.push_back(ctrEdge(e_dec->right_next));
        en_list.push_back(e_dec->right_prev);
        en_list.push_back(ctrEdge(e_dec->right_prev));

        for (W_edge * en : en_list) {
            // check if its vertex has v_decimate 
            if(*(en->start) == vertices[v_decimate]) en->start = &vertices[v_update];
            if(*(en->end) == vertices[v_decimate]) en->end = &vertices[v_update];
            // check if its neighbor edges has e_decimate
            if(*(en->right_prev) == *e_dec) en->right_prev = e_new;
            if(*(en->right_next) == *e_dec) en->right_next = e_new;
            if(*(en->left_prev) == *e_dec) en->left_prev = e_new;
            if(*(en->left_next) == *e_dec) en->left_next = e_new;
        }
    }

    // update vertex for e_update
    for (auto e : e_update) {
        W_edge * eptr = &edges[e];
        if(*(eptr->start) == vertices[v_decimate]) eptr->start = &vertices[v_update];
        if(*(eptr->end) == vertices[v_decimate]) eptr->end = &vertices[v_update];
    }
        
   
    // (4) Switch old vertex to new vertex
    vertices[v_update].x = v_new.x; 
    vertices[v_update].y = v_new.y; 
    vertices[v_update].z = v_new.z; 
    vertices[v_update].isQ = v_new.isQ; 
    vertices[v_update].q = v_new.q; 
    for (int e_idx : e_update) { 
        if (*(edges[e_idx].start) == vertices[v_decimate]){ 
            vertices[v_update].edge = &edges[e_idx];
            break;
        }
    }


    // (5) decimate targets <- deleting elements affect to index system.......
    cout << "\n Decimate -----------------------------------------------" << endl;

    vertices.erase(vertices.begin() + v_decimate); 
    // update vertex reference for index over v_decimate; 
    for (int e=0; e < edges.size(); e++) {
        int v_start = ptr2idx(edges[e].start, &vertices[0]); 
        if(v_start > v_decimate) {
            edges[e].start = &vertices[--v_start];
        }
        int v_end = ptr2idx(edges[e].end, &vertices[0]); 
        if(v_end > v_decimate) {
            edges[e].end = &vertices[--v_end];
       }
    }

    std::sort(f_decimate.begin(), f_decimate.end(), greater<int>()); 
    for(int f_idx: f_decimate) { 
        faces.erase(faces.begin() + f_idx);
        // update Face reference for index over f_decimate[0]; 
        for (int e=0; e < edges.size(); e++) {
            int f_right = ptr2idx(edges[e].right, &faces[0]); 
            int f_left = ptr2idx(edges[e].left, &faces[0]); 
                       
            if(f_right > f_idx) {
                edges[e].right = &faces[--f_right];
            }

            if(f_left > f_idx) {
                edges[e].left = &faces[--f_left];
            }
            
        }
    }

    std::sort(e_decimate.begin(), e_decimate.end(), greater<int>()); 
    for(int e_idx: e_decimate) { 
        // printWEdge(&edges[e_idx], &edges[0], &vertices[0], &faces[0]);
        // printWEdge(&edges[34], &edges[0], &vertices[0], &faces[0]);
        // edges.erase(edges.begin() + e_idx);
        // mEdges[e_idx].end = nullptr;
        // mEdges[e_idx].start = nullptr;
        // mEdges[e_idx].right_next = nullptr;
        // mEdges[e_idx].right_prev = nullptr;
        // mEdges[e_idx].left_next = nullptr;
        // mEdges[e_idx].left_prev= nullptr;

        // cout << "Decimate "<<e_idx << "   \n"; 
        
        // update Edge reference for index over e_idx; 
        // for (int e=0; e < edges.size(); e++) {
        //     int e_rn = ptr2idx(edges[e].right_next, &edges[0]); 
        //     if(e_rn >= e_idx) {
        //         edges[e].right_next = &edges[--e_rn];
        //     }
        //     int e_rp = ptr2idx(edges[e].right_prev, &edges[0]); 
        //     if(e_rp >= e_idx) {
        //         edges[e].right_prev = &edges[--e_rp];
        //        // cout <<  e <<":   " << e_rp << endl;
        //     }
        //     int e_ln = ptr2idx(edges[e].left_next, &edges[0]); 
        //     if(e_ln >= e_idx) {
        //         edges[e].left_next = &edges[--e_ln];
        //     }
        //     int e_lp = ptr2idx(edges[e].left_prev, &edges[0]); 
        //     if(e_lp >= e_idx) {
        //         edges[e].left_prev = &edges[--e_lp];
        //     }       
        // }
    }

    // (5) update normal info for faces and vertices
    Vector3f vnormal (0.0, 0.0, 0.0); 
    for (int fn = 0; fn < f_update.size(); fn++) {
        Vector3f v1 (faces[fn].edge->start->x, faces[fn].edge->start->y, faces[fn].edge->start->z);  
        Vector3f v2 (faces[fn].edge->end->x, faces[fn].edge->end->y, faces[fn].edge->end->z);  
        Vector3f v3 (faces[fn].edge->right_next->end->x, faces[fn].edge->right_next->end->y, faces[fn].edge->right_next->end->z);  
        faces[fn].normal = (v2-v1).cross(v3-v1).normalized();
        vnormal += faces[fn].normal; 
    }
    vertices[v_update].normal = vnormal.normalized(); 
    
    // cout << "Final Vertex : " << endl; 
    // for(int i=0; i <vertices.size(); i++) {
    //     cout << "Vertex "<< i << " :  " << vertices[i].x << ", " << vertices[i].y << ", "<< vertices[i].z
    //     << " | "<< ptr2idx(vertices[i].edge, &edges[0]) << endl;

    // }

    // for (int i=0; i<edges.size(); i++) {
    //     printWEdge(&edges[13], &edges[0], &vertices[0], &faces[0]);
    // }
    
    // for(int i=0; i <faces.size(); i++) {
    //     cout << "Face "<< i  << " : " << ptr2idx(faces[i].edge->start,  &vertices[0]) << ", " 
    //                                   << ptr2idx(faces[i].edge->end,  &vertices[0]) << ", "
    //                                   << ptr2idx(faces[i].edge->right_next->end,  &vertices[0]) << " | "
    //                                   << ptr2idx(faces[i].edge, &edges[0]) << endl;
    // }
    

    cout <<" size of vertices: " << vertices.size() << endl;
    cout <<" size of faces: " << faces.size() << endl;
    cout <<" operation is done ......................" << endl; 

}
    



void MyGLCanvas::decimateMesh(int n_decimate) {
    cout <<" operation starts ...................... " << endl; 
    cout <<" size of mVertices: " << mVertices.size() << endl;
    cout <<" size of mFaces: " << mFaces.size() << endl;
    // [Step0] Need to check target number is less then mVertices 
    if (mEdges.size() + 2 > n_decimate) {
        // // Initialize matrix with exisinting matrix 
        // // positions
        // MatrixXf posM(3, mVertices.size());
        // for (int i = 0; i < mVertices.size(); i++) {
        //     posM.col(i) << mVertices[i].x, mVertices[i].y, mVertices[i].z; 
        // }
        // // indicies 
        // MatrixXu indM(3, mFaces.size());
        // for (int i = 0; i < mFaces.size(); i++) {
        //     indM.col(i) <<  ptr2idx(mFaces[i].edge->start, &mVertices[0]), 
        //                     ptr2idx(mFaces[i].edge->end, &mVertices[0]), 
        //                     ptr2idx(mFaces[i].edge->right_next->end, &mVertices[0]);  
        // }

        // // get vectors
        // vector<Vertex> vertices; vector<Face> faces; vector<W_edge> edges; 
        // updateWingEdge(posM, indM, vertices, faces, edges); 

        // decimate edges
        for (int i = 0; i < n_decimate; i++) {
            decimateOne(mVertices, mFaces, mEdges);
        }

        // // positions
        // MatrixXf positions(3, vertices.size());
        // for (int i = 0; i < vertices.size(); i++) {
        //     positions.col(i) << vertices[i].x, vertices[i].y, vertices[i].z; 

        // }
        // // indicies 
        // MatrixXu indices(3, faces.size());
        // for (int i = 0; i < faces.size(); i++) {
        //     indices.col(i) <<  ptr2idx(faces[i].edge->start, &vertices[0]), 
        //                     ptr2idx(faces[i].edge->end, &vertices[0]), 
        //                     ptr2idx(faces[i].edge->right_next->end, &vertices[0]);  
        
        // }

        
        // cout << indices(0, faces.size()-1) <<", " << indices(1, faces.size()-1) << ". " << indices(2, faces.size()-1) << endl;
        // cout << indices.cols() << " ,,,,,,,, " << positions.cols() << endl;
        // cout << faces.size() << " ,,,,,,,, " << vertices.size() << endl;
        // cout << edges.size() << endl;
        
        // cout << " FAces 546" << endl;
        // cout << ptr2idx(&faces[546], &faces[0]) << endl; 
        // cout << faces[546].normal << endl;
        // printWEdge(faces[546].edge, &edges[0], &vertices[0], &faces[0]);
        
        
        // updateWingEdge(positions, indices, mVertices, mFaces, mEdges);
        updateMeshes(); 
        
    }else {
        cout << "--------------------------------" << endl; 
        cout << "----Not possible to decimate----" << endl; 
        cout << "--------------------------------" << endl; 
    }
}
