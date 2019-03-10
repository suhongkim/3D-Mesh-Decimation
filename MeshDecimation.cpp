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


void MyGLCanvas::decimateMesh(int n_decimate) {
    // [Step1] choose minimum quadric aomng random k edges 
    srand (time(NULL));  // init random seed 
    W_edge *min_edge; 
    float min_cost = numeric_limits<float>::max();

    for (int k = 0; k < randomK; k++) {
        int r_idx = rand() % n_edge;
        W_edge *r_edge = &mEdges[r_idx];

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
        v_opt << 0.0, 0.0, 0.0, 1.0; 
        if (A.determinant() > 0.0) { // check if invertable
            v_opt = A.inverse() * b;
            //v_opt << v_bar(0,0)/v_bar(0,3), v_bar(0,1)/v_bar(0,3), v_bar(0,2)/v_bar(0,3); 
        }
        else { // set v_opt as midpoint of v1 and v2 
            v_opt(0,0) = 0.5*(r_edge->start->x + r_edge->end->x); 
            v_opt(0,1) = 0.5*(r_edge->start->y + r_edge->end->y); 
            v_opt(0,2) = 0.5*(r_edge->start->z + r_edge->end->z); 
            v_opt(0,3) = 1.0; 
        }

        // calculate cost 
        float cost = (v_opt.transpose()*(q1+q2)*v_opt).value();


        cout << cost << endl;
        cout << min_cost << endl; 

        // get min cost 
        if (cost < min_cost) {
            min_cost = cost; 
            min_edge = r_edge; 
        }
    }

    //[Step 2] Edge collapse and update local new vertex (vertex normal and face normal)
    // How can remove vertex????? 

    //[Step 3] Update Mesh Data 
    //updateMeshInfo(); 
    //updateMeshes(); 


}
    