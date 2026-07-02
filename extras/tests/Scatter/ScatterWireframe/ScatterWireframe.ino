/**
 * ScatterWireframe.ino
 * @date 03-10-26
 *
 * ViewPoint Scatter Test: Frames — Rotating 3D Wireframe Projection
 *
 * Renders a 3D wireframe solid projected onto the scatter plot using
 * addBreak() to separate each edge into an independent line segment.
 * The object rotates in three axes, scales (zoom 0.1x to 50x), and
 * translates across the field. The shape morphs between platonic solids:
 *
 * - CUBE:         12 edges, familiar reference shape
 * - TETRAHEDRON:  6 edges, asymmetric projection
 * - OCTAHEDRON:   12 edges, diamond-like silhouette
 * - ICOSAHEDRON:  30 edges, near-spherical (complex)
 *
 * Each edge is drawn as two points (start vertex → end vertex) followed
 * by addBreak(). The renderer sees many disconnected line segments that
 * together form a recognizable solid.
 *
 * Auto-scale challenges:
 * - Projection extents change dramatically with rotation angle
 *   (edge-on cube is a line, face-on cube fills a square)
 * - Scale jumps when zoom changes by 50x
 * - Translation shifts the entire bounding box off-center
 * - Shape morphing changes vertex count and edge topology
 *
 * Hardware: Any Arduino-compatible board
 *
 * Things to try:
 * - Watch the wireframe rotate and notice silhouette changes
 * - See scale response when zoom snaps from 1x to 50x
 * - Notice how translation + rotation creates asymmetric axis ranges
 * - Compare shapes: icosahedron fills space smoothly, tetrahedron is spiky
 */

#include <ViewPoint.h>
using namespace viewpoint;

// --- Vertex and edge storage ---
#define MAX_VERTS 12
#define MAX_EDGES 30

struct Vec3 { float x, y, z; };

Vec3 verts[MAX_VERTS];
int edges[MAX_EDGES][2];
int numVerts = 0;
int numEdges = 0;

// Transform state
float rotX = 0.0, rotY = 0.0, rotZ = 0.0;
float rotSpeedX = 0.01, rotSpeedY = 0.013, rotSpeedZ = 0.007;
float scale = 10.0;
float offsetX = 0.0, offsetY = 0.0;

// Targets
float targetScale = 10.0;
float targetOffsetX = 0.0;
float targetOffsetY = 0.0;
float targetRotSpeedX = 0.01;
float targetRotSpeedY = 0.013;
float targetRotSpeedZ = 0.007;

unsigned long frameCount = 0;
unsigned long nextShapeChange = 0;
unsigned long nextTransformChange = 0;

enum Shape : uint8_t { CUBE, TETRAHEDRON, OCTAHEDRON, ICOSAHEDRON, NUM_SHAPES };
Shape currentShape = CUBE;

void buildCube();
void buildTetrahedron();
void buildOctahedron();
void buildIcosahedron();
void setShape(Shape s);
void pickTransform();
void project(Vec3 v, float& px, float& py);

void setup() {
    randomSeed(analogRead(0));
    view.begin(PlotType::Scatter, Mode::Frames);
    view.setDelay(25);

    view.addHorizontalReferenceLine(0.0);
    view.addVerticalReferenceLine(0.0);

    view.setAxisLabels("X", "Y");
    view.setPlotTitle("3D Wireframe");

    view.trace("Wire").setColor(colors::Cyan);

    setShape(CUBE);
    nextShapeChange = 20 + random(40);
    nextTransformChange = 5 + random(10);
}

void loop() {
    // Shape changes
    if (frameCount >= nextShapeChange) {
        setShape((Shape)random(NUM_SHAPES));
        nextShapeChange = frameCount + 15 + random(30);
    }

    // Transform parameter changes
    if (frameCount >= nextTransformChange) {
        pickTransform();
        nextTransformChange = frameCount + 5 + random(15);
    }

    // Drift toward targets
    scale += (targetScale - scale) * 0.04;
    offsetX += (targetOffsetX - offsetX) * 0.03;
    offsetY += (targetOffsetY - offsetY) * 0.03;
    rotSpeedX += (targetRotSpeedX - rotSpeedX) * 0.05;
    rotSpeedY += (targetRotSpeedY - rotSpeedY) * 0.05;
    rotSpeedZ += (targetRotSpeedZ - rotSpeedZ) * 0.05;

    // Advance rotation
    rotX += rotSpeedX;
    rotY += rotSpeedY;
    rotZ += rotSpeedZ;
    if (rotX > 2.0 * M_PI) rotX -= 2.0 * M_PI;
    if (rotY > 2.0 * M_PI) rotY -= 2.0 * M_PI;
    if (rotZ > 2.0 * M_PI) rotZ -= 2.0 * M_PI;

    // Draw each edge as a separate line segment
    for (int e = 0; e < numEdges; e++) {
        float px0, py0, px1, py1;
        project(verts[edges[e][0]], px0, py0);
        project(verts[edges[e][1]], px1, py1);

        view.addData("Wire", constrain(px0, -1e6f, 1e6f), constrain(py0, -1e6f, 1e6f));
        view.addData("Wire", constrain(px1, -1e6f, 1e6f), constrain(py1, -1e6f, 1e6f));
        view.addBreak("Wire");
    }

    view.send();
    frameCount++;
}

void project(Vec3 v, float& px, float& py) {
    // Rotate around X
    float y1 = v.y * cos(rotX) - v.z * sin(rotX);
    float z1 = v.y * sin(rotX) + v.z * cos(rotX);
    float x1 = v.x;

    // Rotate around Y
    float x2 = x1 * cos(rotY) + z1 * sin(rotY);
    float z2 = -x1 * sin(rotY) + z1 * cos(rotY);
    float y2 = y1;

    // Rotate around Z
    float x3 = x2 * cos(rotZ) - y2 * sin(rotZ);
    float y3 = x2 * sin(rotZ) + y2 * cos(rotZ);

    // Orthographic projection with scale and offset
    px = x3 * scale + offsetX;
    py = y3 * scale + offsetY;
}

void setShape(Shape s) {
    currentShape = s;
    switch (s) {
        case CUBE:          buildCube(); break;
        case TETRAHEDRON:   buildTetrahedron(); break;
        case OCTAHEDRON:    buildOctahedron(); break;
        case ICOSAHEDRON:   buildIcosahedron(); break;
    }
}

void buildCube() {
    numVerts = 8;
    numEdges = 12;
    float v = 1.0;
    Vec3 c[] = {
        {-v,-v,-v}, { v,-v,-v}, { v, v,-v}, {-v, v,-v},
        {-v,-v, v}, { v,-v, v}, { v, v, v}, {-v, v, v}
    };
    for (int i = 0; i < 8; i++) verts[i] = c[i];
    int e[][2] = {
        {0,1},{1,2},{2,3},{3,0},  // Back face
        {4,5},{5,6},{6,7},{7,4},  // Front face
        {0,4},{1,5},{2,6},{3,7}   // Connecting edges
    };
    for (int i = 0; i < 12; i++) { edges[i][0] = e[i][0]; edges[i][1] = e[i][1]; }
}

void buildTetrahedron() {
    numVerts = 4;
    numEdges = 6;
    // Regular tetrahedron inscribed in unit sphere
    verts[0] = { 1.0,  1.0,  1.0};
    verts[1] = { 1.0, -1.0, -1.0};
    verts[2] = {-1.0,  1.0, -1.0};
    verts[3] = {-1.0, -1.0,  1.0};
    // Normalize to unit sphere
    float s = 1.0 / sqrt(3.0);
    for (int i = 0; i < 4; i++) {
        verts[i].x *= s; verts[i].y *= s; verts[i].z *= s;
    }
    int e[][2] = {{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}};
    for (int i = 0; i < 6; i++) { edges[i][0] = e[i][0]; edges[i][1] = e[i][1]; }
}

void buildOctahedron() {
    numVerts = 6;
    numEdges = 12;
    verts[0] = { 1, 0, 0}; verts[1] = {-1, 0, 0};
    verts[2] = { 0, 1, 0}; verts[3] = { 0,-1, 0};
    verts[4] = { 0, 0, 1}; verts[5] = { 0, 0,-1};
    int e[][2] = {
        {0,2},{0,3},{0,4},{0,5},
        {1,2},{1,3},{1,4},{1,5},
        {2,4},{2,5},{3,4},{3,5}
    };
    for (int i = 0; i < 12; i++) { edges[i][0] = e[i][0]; edges[i][1] = e[i][1]; }
}

void buildIcosahedron() {
    numVerts = 12;
    numEdges = 30;
    // Golden ratio vertices
    float phi = (1.0 + sqrt(5.0)) / 2.0;
    float s = 1.0 / sqrt(1.0 + phi * phi);  // Normalize to unit sphere
    float a = s;
    float b = phi * s;
    Vec3 iv[] = {
        {-a, b, 0}, { a, b, 0}, {-a,-b, 0}, { a,-b, 0},
        { 0,-a, b}, { 0, a, b}, { 0,-a,-b}, { 0, a,-b},
        { b, 0,-a}, { b, 0, a}, {-b, 0,-a}, {-b, 0, a}
    };
    for (int i = 0; i < 12; i++) verts[i] = iv[i];
    int ie[][2] = {
        {0,1},{0,5},{0,7},{0,10},{0,11},
        {1,5},{1,7},{1,8},{1,9},
        {2,3},{2,4},{2,6},{2,10},{2,11},
        {3,4},{3,6},{3,8},{3,9},
        {4,5},{4,9},{4,11},
        {5,9},{5,11},
        {6,7},{6,8},{6,10},
        {7,8},{7,10},
        {8,9},{10,11}
    };
    for (int i = 0; i < 30; i++) { edges[i][0] = ie[i][0]; edges[i][1] = ie[i][1]; }
}

void pickTransform() {
    int change = random(6);
    switch (change) {
        case 0:
            // Zoom in
            targetScale = 20.0 + random(3000) / 100.0;
            break;
        case 1:
            // Zoom out (tiny)
            targetScale = 0.1 + random(200) / 100.0;
            break;
        case 2:
            // Translate to random position
            targetOffsetX = (random(2) == 0 ? 1 : -1) * (10.0 + random(20000) / 100.0);
            targetOffsetY = (random(2) == 0 ? 1 : -1) * (10.0 + random(20000) / 100.0);
            break;
        case 3:
            // Return to center, moderate scale
            targetOffsetX = (random(200) - 100) / 50.0;
            targetOffsetY = (random(200) - 100) / 50.0;
            targetScale = 5.0 + random(2000) / 100.0;
            break;
        case 4:
            // Change rotation speed
            targetRotSpeedX = (random(2) == 0 ? 1 : -1) * (0.005 + random(50) / 1000.0);
            targetRotSpeedY = (random(2) == 0 ? 1 : -1) * (0.005 + random(50) / 1000.0);
            targetRotSpeedZ = (random(2) == 0 ? 1 : -1) * (0.005 + random(50) / 1000.0);
            break;
        case 5:
            // Extreme zoom + offset (scale buster)
            targetScale = 100.0 + random(40000) / 100.0;
            targetOffsetX = (random(2) == 0 ? 1 : -1) * (50.0 + random(50000) / 100.0);
            targetOffsetY = (random(2) == 0 ? 1 : -1) * (50.0 + random(50000) / 100.0);
            // 30% chance of instant snap
            if (random(100) < 30) {
                scale = targetScale;
                offsetX = targetOffsetX;
                offsetY = targetOffsetY;
            }
            break;
    }
}
