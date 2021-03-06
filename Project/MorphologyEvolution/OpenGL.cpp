#include <math.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string.h>
#include <vector>
#include <GL/glut.h>
#include <png.h>
#include "Vector3D.h"
#include "OpenGL.h"
#include "CubeGenerator.h"
#include "PhysicsEngine.h"
#include "utility.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

// scaling factor
const double SCALE = 300.0;

// Window Setting
const int WindowPositionX = 200;  //Window x position
const int WindowPositionY = 200;  //Window y position
const int WindowWidth = 1200;    //Window width
const int WindowHeight = 800;    //Window height
const char WindowTitle[] = "Cubic Robot";  //Window title

// Output video
bool _Rec = false;
bool _AutoView = true;
int _Viewmode = 0;
bool _Stop = false;
bool _Friction = true;
bool _Damping = true;
bool _Breathe = false;
int nudgeID = -1;
int captured_frame = 0;
vector<double> nudgeForce = {0.0, 0.0, 0.0};
vector<double> track_x;
vector<double> track_y;

//----------------------------------------------------
// Initial Viewpoint
//----------------------------------------------------
const double InitialViewPointX = 300.0; // 300.0;
const double InitialViewPointY = -500.0; // -800.0;
const double InitialViewPointZ = 300.0; // 600.0;
const double InitialRefPointX = 0.0; // 300.0;
const double InitialRefPointY = 0.0; // 200.0;
const double InitialRefPointZ = 0.0;
const double ViewPointR = sqrt(InitialViewPointX * InitialViewPointX + InitialViewPointY * InitialViewPointY);
const double ViewPointTheta = atan2(InitialViewPointY, InitialViewPointX);

//----------------------------------------------------
// Rotation by mouse action
//----------------------------------------------------

int cx, cy;                // Drag start position
double sx = 1.0 / (double)512; // Transform coefficient from absolute position of mouse to relative position in the window
double sy = 1.0 / (double)512;
double cq[7] = { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };  // Initial orientation (Quaternion)
double tq[7];              // Orientation during dragging (Quaternion)
double rt[16];              // Rotation transform matrix
int mouse_button_on = -1;

unsigned int listNumber;

//----------------------------------------------------
// Texture
//----------------------------------------------------
struct MaterialStruct {
  GLfloat ambient[4];
  GLfloat diffuse[4];
  GLfloat specular[4];
  GLfloat shininess;
};
//jade
MaterialStruct ms_jade = {
  {0.135,     0.2225,   0.1575,   1.0},
  {0.54,      0.89,     0.63,     1.0},
  {0.316228,  0.316228, 0.316228, 1.0},
  12.8};
//ruby
MaterialStruct ms_ruby  = {
  {0.1745,   0.01175,  0.01175,   1.0},
  {0.61424,  0.04136,  0.04136,   1.0},
  {0.727811, 0.626959, 0.626959,  1.0},
  76.8};
//white plastic
MaterialStruct ms_white_plastic  = {
  {0.0,   0.0,     0.0,  1.0},
  {0.55,  0.55,    0.55, 1.0},
  {0.70,  0.70,    0.70, 1.0},
  32};
// pearl
MaterialStruct ms_pearl  = {
  {0.25,     0.20725,  0.20725,  1.0},
  {1,        0.829,    0.829,    1.0},
  {0.296648, 0.296648, 0.296648, 1.0},
  10.24};

//----------------------------------------------------
// Color
//----------------------------------------------------
GLfloat red[] = { 0.8, 0.2, 0.2, 1.0 };
GLfloat green[] = { 0.2, 0.8, 0.2, 1.0 };
GLfloat blue[] = { 0.2, 0.2, 0.8, 1.0 };
GLfloat yellow[] = { 0.8, 0.8, 0.2, 1.0 };
GLfloat white[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat shininess = 30.0;
GLfloat RdBu[11][4] =
{{ 0.25674740484429065 , 0.41522491349480967 , 0.6844290657439447 },
{ 0.38985005767012687 , 0.6009227220299884 , 0.7794694348327567 },
{ 0.5648596693579392 , 0.76639753940792 , 0.867589388696655 },
{ 0.7398692810457516 , 0.8849673202614379 , 0.9333333333333332 },
{ 0.8975009611687812 , 0.9603229527104958 , 0.9374855824682815 },
{ 0.9999231064975009 , 0.9976163014225298 , 0.7454056132256824 },
{ 0.9966935793925413 , 0.8975009611687813 , 0.5936178392925799 },
{ 0.9934640522875817 , 0.7477124183006538 , 0.44183006535947733 },
{ 0.9734717416378317 , 0.5474048442906576 , 0.3181084198385237 },
{ 0.9167243367935409 , 0.3430219146482122 , 0.22399077277970011 },
{ 0.8085351787773933 , 0.15501730103806227 , 0.1522491349480969 }};

//----------------------------------------------------
// Edges, Faces, and Normal vectors.
//----------------------------------------------------

int outer_sq_face[][4] = {
  { 0, 1, 2, 3 }, /* A-B-C-D */
  { 1, 5, 6, 2 }, /* B-F-G-C */
  { 5, 4, 7, 6 }, /* F-E-H-G */
  { 4, 0, 3, 7 }, /* E-A-D-H */
  { 4, 5, 1, 0 }, /* E-F-B-A */
  { 3, 2, 6, 7 }  /* D-C-G-H */
};

int inner_sq_face[][4] = {
  { 0, 7, 6, 1 }, /* A-B-C-D */
  { 4, 3, 2, 5 }, /* B-F-G-C */
  { 0, 3, 6, 5 }, /* F-E-H-G */
  { 1, 4, 7, 2 }, /* E-A-D-H */
  { 1, 3, 7, 5 }, /* E-F-B-A */
  { 0, 4, 6, 2 }  /* D-C-G-H */
};

int inner_tri_face[][3] = {
  { 1, 4, 3}, /* A-B-C-D */
  { 0, 2, 5}, /* B-F-G-C */
  { 1, 3, 6}, /* F-E-H-G */
  { 0, 7, 2}, /* E-A-D-H */
  { 0, 5, 7}, /* E-F-B-A */
  { 1, 6, 4}, /* D-C-G-H */
  { 2, 7, 5},
  { 3, 4, 6}
};

int tetra_face[][3] = {
  { 1, 0, 2},
  { 1, 2, 3},
  { 2, 0, 3},
  { 0, 1, 3}
};

int octa_face[][3] = {
  { 2, 1, 0},
  { 4, 2, 0},
  { 5, 4, 0},
  { 1, 5, 0},
  { 1, 2, 3},
  { 5, 1, 3},
  { 4, 5, 3},
  { 4, 2, 3},
  { 1, 2, 5},
  { 1, 4, 5},
  { 2, 4, 5},
  { 2, 4, 1},
  { 0, 2, 3},
  { 0, 2, 5},
  { 0, 3, 5},
  { 2, 3, 5},
  { 0, 1, 3},
  { 0, 1, 4},
  { 0, 3, 4},
  { 1, 3, 4}
};

//----------------------------------------------------
// Initialize function
//----------------------------------------------------
void Initialize(void){

  glClearColor(1.0, 1.0, 1.0, 1.0); //Assign background color. (R, G, B, alpha)
  glEnable(GL_DEPTH_TEST);//Use depth buffer: (Assign GLUT_DEPTH using glutInitDisplayMode())

  //Set a light source --------------------------------------
  GLfloat light_position0[] = { 0.0, -300.0, 500.0, 1.0 }; //Coordinate of a light source 0
  GLfloat light_position1[] = { 0.0, -300.0, 0.0, 1.0 }; //Coordinate of a light source 0

  glLightfv(GL_LIGHT0, GL_POSITION, light_position0); //
  glLightfv(GL_LIGHT1, GL_POSITION, light_position1); //

  // Create display list
  listNumber = glGenLists(1);
  glNewList( listNumber, GL_COMPILE );
  glEndList();

  // Set a perspective projection matrix ------------------------------
  glMatrixMode(GL_PROJECTION);//Matrix mode (GL PROJECTION: Perspective projection matrix)
  glLoadIdentity();//Initialize a matrix
  gluPerspective(30.0, (double)WindowWidth/(double)WindowHeight, 0.1, 2000.0); //Apparent volume gluPerspactive(th, w/h, near, far);

  //Viewpoint
  gluLookAt(
      InitialViewPointX, InitialViewPointY, InitialViewPointZ,  // Viewpoint: x,y,z;
      InitialRefPointX,  InitialRefPointY,  InitialRefPointZ,   // Reference point: x,y,z
      0.0,        0.0,        1.0 );                            // Vector: x,y,z

  // Alpha blend
  //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //glEnable(GL_BLEND);

  // Initialize rotate matrix
  qrot(rt, cq);
}

//----------------------------------------------------
// Display function
//----------------------------------------------------
void Display(void) {

  //Clear buffer.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //Rec.
  //if(t > 0.0) _Rec = true;

  // Set a viewpoint------------------------------------------------
  if(_AutoView){
    auto_view_tracking(0);
  }

  // Set a model view matrix --------------------------
  glMatrixMode(GL_MODELVIEW);// GL_MODELVIEW: model view transformation matrix)
  glLoadIdentity(); //Initialize a matrix
  glViewport(0, 0, WindowWidth, WindowHeight);

  // Rotation -------------------------
  glMultMatrixd(rt);

  //Shadow ON-----------------------------
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0); //Use a light source 0
  glEnable(GL_LIGHT1); //Use a light source 0

  // Objects -------------------------

  for(vector<Mass> robot: robots){
    if(robot.size()!=0){
      if (_Viewmode==0){
        CubicRobotSolid(robot);
        //TetraRobotSolid(robot);
      }else{
        CubicRobot(robot);
      }
    }
  }


  // Shadow OFF -----------------------------
  glDisable(GL_LIGHTING);
  // -----------------------------------

  // Draw ground. --------------------
  Ground();
  glDrawAxisd(30);
  glPopMatrix();


  // Draw characters. -------------------------------------------------------
  string str;
  str = "time = " + std::to_string(t);
  DISPLAY_TEXT(5, 95, str);
  //str = "frame = " + std::to_string(frame);
  //DISPLAY_TEXT(5, 90, str);
  //if (_Friction){str = "Friction: On";}else{str = "Friction: Off";}
  //DISPLAY_TEXT(25, 95, str);
  //if (_Damping){str = "Damping: On";}else{str = "Damping: Off";}
  //DISPLAY_TEXT(25, 90, str);
  //if (_Breathe){str = "Breathe: On";}else{str = "Breathe: Off";}
  //DISPLAY_TEXT(45, 95, str);
  //if (_Rec){str = "Rec.: On";}else{str = "Rec.: Off";}
  //DISPLAY_TEXT(45, 90, str);


  // Output png. -------------------------------------------------------
  if(_Rec){
    string fname = "./png/" + std::to_string(10000 + captured_frame) + ".png";//Output file name.
    capture(fname);
  }

  //glutInitDisplayMode(GLUT_DOUBLE) enables "double buffering"
  glutSwapBuffers();

  if (!_Stop){
    frame++ ;
  }
}


//----------------------------------------------------
// Idling function
//----------------------------------------------------
void Idle(){
  //glutPostRedisplay(); //Execute glutDisplayFunc() once.
}

//----------------------------------------------------
// Keyboard function.
//----------------------------------------------------
void Keyboard(unsigned char key, int x, int y){
  switch ( key )
  {
  case 'a':
    if (_AutoView){
      _AutoView=false;
    }else{
      _AutoView=true;
    }
    break;

  case 'b':
    if (_Breathe){
      _Breathe=false;
    }else{
      _Breathe=true;
    }
    break;

  case 'd':
    if (_Damping){
      _Damping=false;
    }else{
      _Damping=true;
    }
    break;

  case 'f':
    if (_Friction){
      _Friction=false;
    }else{
      _Friction=true;
    }
    break;

  case 'n':
    nudgeID += 1;
    if (nudgeID >= N_MASS) nudgeID=-1;
    break;

  case 'r':
    if (_Rec){
      _Rec=false;
    }else{
      _Rec=true;
    }
    break;

  case 's':
    if (_Stop){
      _Stop=false;
    }else{
      _Stop=true;
    }
    break;

  case 'v':
    _Viewmode+=1;
    if (_Viewmode>=3) _Viewmode = 0;
    break;

  case 'x':
    fixed_view(0);
    break;

  case 'y':
    fixed_view(1);
    break;

  case 'z':
    fixed_view(2);
    break;

  case 'q':
    exit(0);
    break;

  default:
    break;
  }
}

void SpecialKey(int key, int x, int y){

  switch ( key )
  {
  case GLUT_KEY_LEFT:
    nudgeForce[0] = -500.0;
    break;

  case GLUT_KEY_RIGHT:
    nudgeForce[0] = 500.0;
    break;

  case GLUT_KEY_UP:
    nudgeForce[1] = -500.0;
    break;

  case GLUT_KEY_DOWN:
    nudgeForce[1] = 500.0;
    break;

  case GLUT_KEY_PAGE_DOWN:
    nudgeForce[2] = -500.0;
    break;

  case GLUT_KEY_PAGE_UP:
    nudgeForce[2] = 500.0;
    break;

  default:
    break;
  }
}
//----------------------------------------------------
// Ground function
//----------------------------------------------------

void Ground(void) {
    double ground_max_x = 3000.0;
    double ground_max_y = 3000.0;
    glColor3d(0.8, 0.8, 0.8);  // Color of the ground
    GLdouble floor_normal[3] = { 0.0, 0.0, 1.0 };
    glLineWidth(1.0d);
    double dl = 30.0;


    glColor3d(0.9, 0.9, 0.9);  // Color of the ground
    glBegin(GL_QUADS);
    for(double ly = -ground_max_y ;ly <= ground_max_y; ly+=dl){
      for(double lx = -ground_max_x ;lx <= ground_max_x; lx+=dl){
        if ((int)((lx+ly)/dl)%2 == 0){
          GLdouble floor_vertices[][3] = {{lx,   ly,    0.0},
                                         {lx+dl, ly,    0.0},
                                         {lx+dl, ly+dl, 0.0},
                                         {lx,    ly+dl, 0.0}};
          glNormal3dv(floor_normal);
          for (int k=0; k<4; k++){
            glVertex3dv(floor_vertices[k]);
          }
        }
      }
    }
    glEnd();

    glColor3d(0.5, 0.5, 0.5);  // Color of the ground
    glBegin(GL_QUADS);
    for(double ly = -ground_max_y ;ly <= ground_max_y; ly+=dl){
      for(double lx = -ground_max_x ;lx <= ground_max_x; lx+=dl){
        if ((int)((lx+ly)/dl)%2 != 0){
          GLdouble floor_vertices[][3] = {{lx,   ly,    0.0},
                                         {lx+dl, ly,    0.0},
                                         {lx+dl, ly+dl, 0.0},
                                         {lx,    ly+dl, 0.0}};
          glNormal3dv(floor_normal);
          for (int k=0; k<4; k++){
            glVertex3dv(floor_vertices[k]);
          }
        }
      }
    }
    glEnd();
}

//----------------------------------------------------
// Draw Cubic Robot
//----------------------------------------------------

void CubicRobot(vector<Mass> robot){

  // Masses
  for(int i=0; i<N_MASS; i++){
    if (robot[i].m==0.0) continue;

    glPushMatrix();
    if (i!=nudgeID){
      glMaterialfv(GL_FRONT, GL_AMBIENT, ms_jade.ambient);
      glMaterialfv(GL_FRONT, GL_DIFFUSE, ms_jade.diffuse);
      glMaterialfv(GL_FRONT, GL_SPECULAR, ms_jade.specular);
      glMaterialfv(GL_FRONT, GL_SHININESS, &ms_jade.shininess);
    }else{
      glMaterialfv(GL_FRONT, GL_AMBIENT, ms_ruby.ambient);
      glMaterialfv(GL_FRONT, GL_DIFFUSE, ms_ruby.diffuse);
      glMaterialfv(GL_FRONT, GL_SPECULAR, ms_ruby.specular);
      glMaterialfv(GL_FRONT, GL_SHININESS, &ms_ruby.shininess);
    }

    glTranslated(robot[i].p[0]*SCALE , robot[i].p[1]*SCALE , robot[i].p[2]*SCALE );
    glutSolidSphere(nodeRadius*SCALE, 20, 20);
    glPopMatrix();
  }

  // Springs
  GLUquadricObj *bars[N_SPRING];

  for(int i=0; i<N_SPRING; i++){
    if (robot[spring[i].masses[0]].m == 0.0 ||
        robot[spring[i].masses[1]].m == 0.0) continue;

    glPushMatrix();

    /* --- Color setting --- */

    int color_index=5;

    /* ViewMode=1: Color varies depends on how long the spring length is compared to its initial length. */
    if (_Viewmode==1){
      double dx = 0.3;
      double length = calcDistance(mass[spring[i].masses[0]].p, mass[spring[i].masses[1]].p);
      color_index = (int) ((length / springInitialRestlength[i] - (1.0 - dx)) * 5.0 / dx);
      //color_index = (int) ((spring[i].l0 / springInitialRestlength[i] - (1.0 - dx)) * 5.0 / dx);

    /* ViewMode=2: Color varies depends on how strong the spring constant K is compared to its initial value. */
    } else if(_Viewmode==2){
      double dx = 1.0;
      color_index = (int) (( 1.0/dx * log(spring[i].k / springK) + 1.0) * 5.0);
    }

    if (color_index < 0) color_index = 0;
    if (color_index > 10) color_index = 10;
    glMaterialfv(GL_FRONT, GL_DIFFUSE, RdBu[color_index]);


    /* --- Draw a string --- */
    /* Calculate the vector of the spring and length of the spring */
    vector<double> begin_pt = scaling(robot[spring[i].masses[0]].p, SCALE);
    vector<double> end_pt = scaling(robot[spring[i].masses[1]].p, SCALE);
    vector<double>vec = sub(end_pt, begin_pt);
    double vec_abs = calcNorm(vec);

    /* Draw a cylinder */
    bars[i] = gluNewQuadric();
    gluQuadricDrawStyle(bars[i], GLU_FILL);
    glTranslated(begin_pt[0], begin_pt[1], begin_pt[2]);
    glRotated(acos(vec[2]/vec_abs)*180/PI, -vec[1], vec[0], 0.0);
    gluCylinder(bars[i], 1.5, 1.5, vec_abs, 8, 8);

    glPopMatrix();
  }

}

void CubicRobotSolid(vector<Mass> robot){

  /* Draw a surface for each cube */
  for (std::array<int, 8> vertex:vertices){
    glPushMatrix();

    /* Material Color */
    //GLfloat color[] = {blue[0], blue[1], blue[2], 1.0};
    //glMaterialfv(GL_FRONT, GL_DIFFUSE, color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, ms_jade.diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ms_jade.ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ms_jade.specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &ms_jade.shininess);

    /* Draw surface */
    glBegin(GL_TRIANGLES);

    /* Surfaces of outer squares of a Cube.*/
    for (auto face: outer_sq_face) {
      vector<Mass>nodes;
      for(int i=0; i<4; ++i) nodes.push_back(robot[vertex[face[i]]]);
      drawSquareSurface(nodes);
    }

    /* Surfaces of inner squares of a Cube.*/
    for (auto face: inner_sq_face) {
      vector<Mass>nodes;
      for(int i=0; i<4; ++i) nodes.push_back(robot[vertex[face[i]]]);
      drawSquareSurface(nodes);
    }

    /* Surfaces of inner triangles of a Cube.*/
    for (auto face: inner_tri_face) {
      vector<Mass>nodes;
      for(int i=0; i<3; ++i) nodes.push_back(robot[vertex[face[i]]]);
      drawTriangleSurface(nodes);
    }

    glEnd();
    glPopMatrix();

    /*
    glBegin(GL_QUADS);
    for (int j = 0; j < 6; ++j) {
      glNormal3dv(normal[j]);

      for (int i = 0; i < 4; ++i) {
        GLdouble scaled_vertex[3] =
                {robot[vertex[face[j][i]]].p[0]*SCALE,
                 robot[vertex[face[j][i]]].p[1]*SCALE,
                 robot[vertex[face[j][i]]].p[2]*SCALE};
        glVertex3dv(scaled_vertex);
      }
    }
    glEnd();
    glPopMatrix();

    glColor3d(0.0, 0.0, 0.0);
    glBegin(GL_LINES);
    for (int i = 0; i < 12; ++i) {
      GLdouble scaled_vertex1[3] =
                {robot[vertex[edge[i][0]]].p[0]*SCALE,
                 robot[vertex[edge[i][0]]].p[1]*SCALE,
                 robot[vertex[edge[i][0]]].p[2]*SCALE};
      GLdouble scaled_vertex2[3] =
                {robot[vertex[edge[i][1]]].p[0]*SCALE,
                 robot[vertex[edge[i][1]]].p[1]*SCALE,
                 robot[vertex[edge[i][1]]].p[2]*SCALE};
      glVertex3dv(scaled_vertex1);
      glVertex3dv(scaled_vertex2);
    }
    glEnd();
    */

  }
  /* --- Draw edges --- */
  /*

  glColor3d(0.0, 0.0, 0.0);
  glBegin(GL_LINES);
  for (int j = 0; j < N_SPRING; ++j) {
    if (spring[j].k==0.0) continue;
    for (int i = 0; i < 2; ++i) {
      GLdouble scaled_vertex[3] =
              {robot[spring[j].masses[i]].p[0]*SCALE,
               robot[spring[j].masses[i]].p[1]*SCALE,
               robot[spring[j].masses[i]].p[2]*SCALE};
      glVertex3dv(scaled_vertex);
    }
  }
  glEnd();
  */
}

void drawSquareSurface(vector<Mass> nodes){

  vector<Mass>triangle_nodes;
  triangle_nodes = {nodes[0], nodes[1], nodes[2]};
  drawTriangleSurface(triangle_nodes);
  triangle_nodes = {nodes[0], nodes[1], nodes[3]};
  drawTriangleSurface(triangle_nodes);
  triangle_nodes = {nodes[0], nodes[2], nodes[3]};
  drawTriangleSurface(triangle_nodes);
  triangle_nodes = {nodes[1], nodes[2], nodes[3]};
  drawTriangleSurface(triangle_nodes);

}

void drawTriangleSurface(vector<Mass> nodes){

  if (nodes[0].m!=0.0 && nodes[1].m!=0.0 && nodes[2].m!=0.0){

    GLdouble normal[3];
    vector<double> v = sub(nodes[1].p, nodes[0].p);
    vector<double> w = sub(nodes[2].p, nodes[1].p);
    normal[0] = v[1]*w[2] - v[2]*w[1];
    normal[1] = v[2]*w[0] - v[0]*w[2];
    normal[2] = v[0]*w[1] - v[1]*w[0];
    double norm = sqrt(normal[0]*normal[0] + normal[1]*normal[1]+normal[2]*normal[2]);
    normal[0] = normal[0]/norm;
    normal[1] = normal[1]/norm;
    normal[2] = normal[2]/norm;

    glNormal3dv(normal);

    /* Surface */
    for (Mass node: nodes) {
      GLdouble scaled_vertex[3] = {node.p[0]*SCALE+normal[0]*0.5,
                                   node.p[1]*SCALE+normal[1]*0.5,
                                   node.p[2]*SCALE+normal[2]*0.5};
      glVertex3dv(scaled_vertex);
    }

    normal[0] = -normal[0];
    normal[1] = -normal[1];
    normal[2] = -normal[2];

    glNormal3dv(normal);

    /* Surface */
    for (Mass node: nodes) {
      GLdouble scaled_vertex[3] = {node.p[0]*SCALE+normal[0]*0.5,
                                   node.p[1]*SCALE+normal[1]*0.5,
                                   node.p[2]*SCALE+normal[2]*0.5};
      glVertex3dv(scaled_vertex);
    }
  }

}

//----------------------------------------------------
// Draw Tetra Robot
//----------------------------------------------------

void TetraRobotSolid(vector<Mass> robot){

  for (std::array<int, 4> vertex: tetra_vertices){

    /* --- Surface shading --- */

    glPushMatrix();

    /* Material Color */
    //GLfloat color[] = {green[0], green[1], green[2], 0.1};
    glMaterialfv(GL_FRONT, GL_DIFFUSE, ms_pearl.diffuse);
    glMaterialfv(GL_FRONT, GL_AMBIENT, ms_pearl.ambient);
    glMaterialfv(GL_FRONT, GL_SPECULAR, ms_pearl.specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, &ms_pearl.shininess);

    /* Draw surface */
    glBegin(GL_TRIANGLES);

    /* Surfaces of Tetrahedron */
    for (int j = 0; j < 4; ++j) {

      vector<Mass>nodes;
      for(int i=0; i<3; ++i) nodes.push_back(robot[vertex[tetra_face[j][i]]]);

      /* Check the presence of the node*/
      if (nodes[0].m==0 || nodes[1].m==0 || nodes[2].m==0) continue;

      /* Normal vector and surfaces*/
      setSurface(nodes);

    }

    glEnd();
    glPopMatrix();

  }

  for (std::array<int, 6> vertex: octa_vertices){

    /* --- Surface shading --- */

    glPushMatrix();

    /* Material Color */
    //GLfloat color[] = {green[0], green[1], green[2], 0.1};
    glMaterialfv(GL_FRONT, GL_DIFFUSE, ms_pearl.diffuse);
    glMaterialfv(GL_FRONT, GL_AMBIENT, ms_pearl.ambient);
    glMaterialfv(GL_FRONT, GL_SPECULAR, ms_pearl.specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, &ms_pearl.shininess);

    /* Draw surface */
    glBegin(GL_TRIANGLES);

      /* Surfaces of Octahedron */
    for (auto face :octa_face) {

      vector<Mass>nodes;
      for(int i=0; i<3; ++i){
        int id = vertex[face[i]];
        if (id == -1) break;
        nodes.push_back(robot[id]);
      }
      /* Check the presence of the node*/
      if (nodes.size()<3) continue;
      if (nodes[0].m==0 || nodes[1].m==0 || nodes[2].m==0) continue;

      /* Normal vector and surfaces*/
      setSurface(nodes);

    }
    glEnd();
    glPopMatrix();

  }

  /* --- Draw edges --- */
  glColor3d(0.0, 0.0, 0.0);
  glBegin(GL_LINES);
  for (int j = 0; j < N_SPRING; ++j) {
    for (int i = 0; i < 2; ++i) {
      GLdouble scaled_vertex[3] =
              {robot[spring[j].masses[i]].p[0]*SCALE,
               robot[spring[j].masses[i]].p[1]*SCALE,
               robot[spring[j].masses[i]].p[2]*SCALE};
      glVertex3dv(scaled_vertex);
    }
  }
  glEnd();

}

void setSurface(vector<Mass> nodes){
  GLdouble normal[3];

  vector<double> v = sub(nodes[1].p, nodes[0].p);
  vector<double> w = sub(nodes[2].p, nodes[1].p);
  normal[0] = v[1]*w[2] - v[2]*w[1];
  normal[1] = v[2]*w[0] - v[0]*w[2];
  normal[2] = v[0]*w[1] - v[1]*w[0];
  double norm = sqrt(normal[0]*normal[0] + normal[1]*normal[1]+normal[2]*normal[2]);
  normal[0] = normal[0]/norm;
  normal[1] = normal[1]/norm;
  normal[2] = normal[2]/norm;

  glNormal3dv(normal);

  /* Surface */
  for (Mass node: nodes) {
    GLdouble scaled_vertex[3] = {node.p[0]*SCALE, node.p[1]*SCALE, node.p[2]*SCALE};
    glVertex3dv(scaled_vertex);
  }

}

//----------------------------------------------------
// Draw Characters.
//----------------------------------------------------

void DISPLAY_TEXT(int x, int y, string str){
  static int list=0;
  glDisable(GL_LIGHTING);
  glDisable(GL_LIGHT0);

  glPushAttrib(GL_ENABLE_BIT);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, 100, 0, 100);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glColor3b(0.0, 0.0, 0.0);
  glCallList(list);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glPopAttrib();
  glMatrixMode(GL_MODELVIEW);
  list=glGenLists(1);
  glNewList(list,GL_COMPILE);

  DRAW_STRING(x, y, str , GLUT_BITMAP_TIMES_ROMAN_24);
  glEndList();

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
}

void DRAW_STRING(int x, int y, string str, void *font){
  int len, i;
  glRasterPos2f(x, y);
  len = str.size();
  char t_char[len];
  str.copy(t_char, len);
  for (i = 0; i < len; i++){
    glutBitmapCharacter(font, t_char[i]);
  }
}

// -----------------------------------------------------------------------
// Frame capture -> png file
// -----------------------------------------------------------------------

void capture(const string fname)
{

    int len = fname.size();
    char filepath[len];
    fname.copy(filepath, len);

    png_bytep raw1D;
    png_bytepp raw2D;
    int i;
    int width = glutGet(GLUT_WINDOW_WIDTH);
    int height = glutGet(GLUT_WINDOW_HEIGHT);

    // Create structure.
    FILE *fp = fopen(filepath, "wb");
    png_structp pp = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop ip = png_create_info_struct(pp);

    // Pre-process
    png_init_io(pp, fp);
    png_set_IHDR(pp, ip, width, height,
        8, // 8bit
        PNG_COLOR_TYPE_RGBA, // RGBA
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // Pixel area
    raw1D = (png_bytep)malloc(height * png_get_rowbytes(pp, ip));
    raw2D = (png_bytepp)malloc(height * sizeof(png_bytep));
    for (i = 0; i < height; i++)
        raw2D[i] = &raw1D[i*png_get_rowbytes(pp, ip)];

    // Capture.
    glReadBuffer(GL_FRONT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Initial value = 4.
    glReadPixels(0, 0, width, height,
            GL_RGBA, // RGBA
            GL_UNSIGNED_BYTE, // 8bit
            (void*)raw1D);

    // Flip.
    for (i = 0; i < height/ 2; i++){
        png_bytep swp = raw2D[i];
        raw2D[i] = raw2D[height - i - 1];
        raw2D[height - i - 1] = swp;
    }

    // Write.
    png_write_info(pp, ip);
    png_write_image(pp, raw2D);
    png_write_end(pp, ip);

    // Close.
    png_destroy_write_struct(&pp, &ip);
    fclose(fp);
    free(raw1D);
    free(raw2D);

    captured_frame += 1;
    printf("write out screen capture to '%s'\n", filepath);
}

//------------------------------------------------------------------------
// Viewpoint rotation by mouse drag
//------------------------------------------------------------------------

// --- Drag mouse action
void mouse_motion(int x, int y){
  double dx, dy, distance;

  // Displacement of the mouse pointer from the drag start point.
  dx = (x - cx) * sx;
  dy = (y - cy) * sy;
  distance = sqrt(dx * dx + dy * dy);

  if( distance != 0.0 && mouse_button_on == 0 )
  {
    // Rotation by mouse drag (Quaternion dq).
    double ar = distance * 2.0 * PI * 0.5;
    double as = sin(ar) / distance;
    double dq[4] = { cos(ar), dy * as, dx * as, 0.0 };

    // Initial orientation cq * quarternion dq
    qmul(tq, dq, cq);

    // Quaternion -> Rotation matrix
    qrot(rt, tq);
  }else if( distance != 0.0 && mouse_button_on == 2  ){
    tq[4] = cq[4] + dx * 100.0;
    tq[6] = cq[6] - dy * 100.0;
    rt[12] = tq[4];
    rt[14] = tq[6];
  }

}

// --- On mouse action
void mouse_on(int button, int state, int x, int y)
{
  switch (button) {
  case 0: // 	MOUSE_LEFT_BUTTON = 0,
    switch (state) {
    case 0:
      // Drag starting point
      cx = x;
      cy = y;
      mouse_button_on = 0;
      break;
    case 1:
      // Save initial orientation.
      cq[0] = tq[0];
      cq[1] = tq[1];
      cq[2] = tq[2];
      cq[3] = tq[3];
      mouse_button_on = -1;
      break;
    default:
      break;
    }
    break;

  //case 1: //MOUSE_MIDDLE_BUTTON = 1
  case 2: // MOUSE_RIGHT_BUTTON = 2,
     switch (state) {
     case 0:
       // Drag starting point
       cx = x;
       cy = y;
       mouse_button_on = 2;
       break;
     case 1:
       // Save initial orientation.
       cq[4] = tq[4];
       cq[5] = tq[5];
       cq[6] = tq[6];
       mouse_button_on = -1;
       break;
    default:
      break;
    }
    break;

  case 3: // MOUSE_SCROLL_UP = 3,
    if (state == GLUT_DOWN){
      rt[13] += 20;
    }
  case 4: // MOUSE_SCROLL_DOWN = 4
    if (state == GLUT_DOWN){
      rt[13] -= 10;
    }

  default:
    break;
  }
  //cout << x << " " << y<<endl;
}

// --- multiplication of quaternions r <- p x q
void qmul(double r[], const double p[], const double q[])
{
  r[0] = p[0] * q[0] - p[1] * q[1] - p[2] * q[2] - p[3] * q[3];
  r[1] = p[0] * q[1] + p[1] * q[0] + p[2] * q[3] - p[3] * q[2];
  r[2] = p[0] * q[2] - p[1] * q[3] + p[2] * q[0] + p[3] * q[1];
  r[3] = p[0] * q[3] + p[1] * q[2] - p[2] * q[1] + p[3] * q[0];
}

// --- transform matrix r <- quaternion q
void qrot(double r[], double q[]){
  double x2 = q[1] * q[1] * 2.0;
  double y2 = q[2] * q[2] * 2.0;
  double z2 = q[3] * q[3] * 2.0;
  double xy = q[1] * q[2] * 2.0;
  double yz = q[2] * q[3] * 2.0;
  double zx = q[3] * q[1] * 2.0;
  double xw = q[1] * q[0] * 2.0;
  double yw = q[2] * q[0] * 2.0;
  double zw = q[3] * q[0] * 2.0;

  r[ 0] = 1.0 - y2 - z2;
  r[ 1] = xy + zw;
  r[ 2] = zx - yw;
  r[ 4] = xy - zw;
  r[ 5] = 1.0 - z2 - x2;
  r[ 6] = yz + xw;
  r[ 8] = zx + yw;
  r[ 9] = yz - xw;
  r[10] = 1.0 - x2 - y2;
  r[ 3] = r[ 7] = r[11];// = r[12] = r[13] = r[14] = 0.0;
  r[12] = q[4];
  r[13] = q[5];
  r[14] = q[6];
  r[15] = 1.0;
}

void fixed_view(int type){
    /*
    // Set a model view matrix --------------------------
    glMatrixMode(GL_MODELVIEW);// GL_MODELVIEW: model view transformation matrix)
    glLoadIdentity(); //Initialize a matrix
    glViewport(0, 0, WindowWidth, WindowHeight);

    rt[0]=1.0; rt[1]=0.0; rt[2]=0.0; rt[3]=0.0;
    rt[4]=0.0; rt[5]=1.0; rt[6]=0.0; rt[7]=0.0;
    rt[8]=0.0; rt[9]=0.0; rt[10]=1.0; rt[11]=0.0;
    rt[12]=0.0; rt[13]=0.0; rt[14]=0.0; rt[15]=1.0;
    cq[0] = 1.0; cq[1] = 0.0; cq[2] = 0.0; cq[3] = 0.0;
    cq[4] = 0.0; cq[5] = 0.0; cq[6] = 0.0;
    tq[0] = 1.0; tq[1] = 0.0; tq[2] = 0.0; tq[3] = 0.0;
    tq[4] = 0.0; tq[5] = 0.0; tq[6] = 0.0;
    */
    // Set a perspective projection matrix ------------------------------
    glMatrixMode(GL_PROJECTION);//Matrix mode (GL PROJECTION: Perspective projection matrix)
    glLoadIdentity();//Initialize a matrix
    gluPerspective(30.0, (double)WindowWidth/(double)WindowHeight, 0.1, 2000.0); //Apparent volume gluPerspactive(th, w/h, near, far);

    vector<double> centroid = get_centroid(robots[0]);

    double ViewPointX = centroid[0];
    double ViewPointY = centroid[1];
    double ViewPointZ = centroid[2];

    if (type == 0){
        ViewPointX -= 500.0;
        ViewPointZ += 300.0;
    }else if(type == 1){
        ViewPointY -= 500.0;
        ViewPointZ += 300.0;
    }else if(type == 2){
        ViewPointZ += 500;
    }

    gluLookAt(
      ViewPointX,  ViewPointY,  ViewPointZ,             // Viewpoint: x,y,z;
      centroid[0], centroid[1], centroid[2],        // Reference point: x,y,z
      0.0,        0.0,        1.0 );                            //Vector: x,y,z

}

void glDrawAxisd(double length)
{
	GLUquadricObj *arrows[3];

	// Draw X-axis
	glColor3ub(255, 0, 0);
    glLineWidth(5.0);
	glBegin(GL_LINES);
		glVertex3d(0, 0, 0);
		glVertex3d( length, 0, 0);
	glEnd();
	glPushMatrix();
		arrows[0] = gluNewQuadric();
		gluQuadricDrawStyle(arrows[0], GLU_FILL);
		glTranslated(length, 0.0f, 0.0f);
		glRotated(90.0f, 0,1,0);
		gluCylinder(arrows[0], length/20, 0.0f, length/5, 8, 8);
	glPopMatrix();

	// Draw Y-axis
	glColor3ub(  0,255, 0);
    glLineWidth(5.0);
	glBegin(GL_LINES);
		glVertex3d( 0, 0, 0);
		glVertex3d( 0, length, 0);
	glEnd();
	glPushMatrix();
		arrows[1] = gluNewQuadric();
		gluQuadricDrawStyle(arrows[1], GLU_FILL);
		glTranslated(0.0f, length, 0.0f);
		glRotated(-90.0f, 1,0,0);
		gluCylinder(arrows[1], length/20, 0.0f, length/5, 8, 8);
	glPopMatrix();

	// Draw Z-axis
	glColor3ub(  0, 0,255);
    glLineWidth(5.0);
	glBegin(GL_LINES);
		glVertex3d( 0, 0, 0);
		glVertex3d( 0, 0, length);
	glEnd();
	glPushMatrix();
		arrows[2] = gluNewQuadric();
		gluQuadricDrawStyle(arrows[2], GLU_FILL);
		glTranslated(0.0f, 0.0f, length);
		gluCylinder(arrows[2], length/20, 0.0f, length/5, 8, 8);
	glPopMatrix();

}

void getMatrix(void){
    GLfloat m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    printf("m[0]:% 7.5f m[4]:% 7.5f m[8] :% 7.5f m[12]:% 7.5f\n", m[0], m[4], m[8],  m[12]);
    printf("m[1]:% 7.5f m[5]:% 7.5f m[9] :% 7.5f m[13]:% 7.5f\n", m[1], m[5], m[9],  m[13]);
    printf("m[2]:% 7.5f m[6]:% 7.5f m[10]:% 7.5f m[14]:% 7.5f\n", m[2], m[6], m[10], m[14]);
    printf("m[3]:% 7.5f m[7]:% 7.5f m[11]:% 7.5f m[16]:% 7.5f\n", m[3], m[7], m[11], m[15]);
}

void auto_view_tracking(int type){

    // Set a perspective projection matrix ------------------------------
    glMatrixMode(GL_PROJECTION); //Matrix mode (GL PROJECTION: Perspective projection matrix)
    glLoadIdentity();            //Initialize a matrix
    gluPerspective(30.0, (double)WindowWidth/(double)WindowHeight, 0.1, 2000.0); //Apparent volume gluPerspactive(th, w/h, near, far);

    double ViewPointX = InitialViewPointX;
    double ViewPointY = InitialViewPointY;
    double ViewPointZ = InitialViewPointZ;
    double RefPointX = InitialRefPointX;
    double RefPointY = InitialRefPointY;
    double RefPointZ = InitialRefPointZ;

    vector<double> center_point = get_centroid(robots[0]);

    track_x.push_back(center_point[0]);
    track_y.push_back(center_point[1]);
    if (track_x.size()==101) track_x.erase(track_x.begin());
    if (track_y.size()==101) track_y.erase(track_y.begin());
    double ave_x = accumulate(track_x.begin(), track_x.end(), 0.0) / track_x.size();
    double ave_y = accumulate(track_y.begin(), track_y.end(), 0.0) / track_y.size();

    // Set a perspective projection matrix ------------------------------

    if (type==0){
      double omega = 2.0 * PI * 0.0;
      ViewPointX = ViewPointR * cos( omega * t + ViewPointTheta) + ave_x;
      ViewPointY = ViewPointR * sin( omega * t + ViewPointTheta) + ave_y;
      ViewPointZ = InitialViewPointZ;
      RefPointX  = ave_x;
      RefPointY  = ave_y;
      RefPointZ  = 0.0;
    }

    else if(type==1){
      double t0 = 4.5;
      double shift_x = 200;
      double shift_y = 0.0;
      double shift_z = 0.0;
      if(t > t0){;
        ViewPointX += shift_x * (t-t0);
        ViewPointY += shift_y * (t-t0);
        ViewPointZ += shift_z * (t-t0);
        RefPointX  += shift_x * (t-t0);
        RefPointY  += shift_y * (t-t0);
        RefPointX  += shift_z * (t-t0);
      }
    }

    gluLookAt(
      ViewPointX,  ViewPointY,  ViewPointZ,      // Viewpoint: x,y,z;
      RefPointX,   RefPointY,   RefPointZ,       // Reference point: x,y,z
      0.0,         0.0,         1.0 );           //　Vector: x,y,z

    GLfloat light_position0[] = { (GLfloat) ViewPointX, (GLfloat) ViewPointY, (GLfloat) ViewPointZ, 1.0 }; //Coordinate of a light source 0
    glLightfv(GL_LIGHT0, GL_POSITION, light_position0);

}

vector<double> get_centroid(vector<Mass> robot){

  vector<double> center_point = {0.0, 0.0, 0.0};

  int num_of_mass = 0.0;
  for (int i=0; i<N_MASS; i++){
    if (robot[i].m==0) continue;
    center_point = add(center_point, robot[i].p);
    num_of_mass += 1;
  }
  center_point = scaling(center_point, SCALE/num_of_mass);

  return center_point;
}
