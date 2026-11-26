// ArchViz - the building model: levels, a graph of walls per level (nodes joined by walls, mitred corners, T and X
// junctions split into nodes), openings hosted by the walls, rooms found as the closed faces of the graph.
// Portable C++ (PC, iPad, Linux tests): no window, no renderer. The 3D meshes are built by ArchMesh.cpp.
// Plan coordinates: metres, x to the east, y to the north. World (vp3d, Y up): (x, elevation + z, -y).
#pragma once
#include "vp3d/Math.h"
#include <string>
#include <vector>

namespace archviz {

using vp3d::float2;
using vp3d::float3;

inline float2 operator-(float2 a) { return {-a.x, -a.y}; }
inline float dot2(float2 a, float2 b) { return a.x * b.x + a.y * b.y; }
inline float cross2(float2 a, float2 b) { return a.x * b.y - a.y * b.x; }
inline float len2(float2 a) { return std::sqrt(dot2(a, a)); }
inline float2 norm2(float2 a) { const float l = len2(a); return l > 1e-9f ? a * (1.0f / l) : float2(1, 0); }
inline float2 perp(float2 a) { return {-a.y, a.x}; }   // left normal

enum OpeningKind { kWindow = 0, kDoor = 1, kSlider = 2 };
const char* openingName(int kind);

struct Opening {
    int kind = kWindow;
    float pos = 1.0f;      // centre, metres from the start node of the wall
    float width = 1.2f;
    float height = 1.35f;
    float sill = 0.9f;     // height of the bottom above the floor (0 for doors)
    int flip = 0;          // door: hinge side / swing side (plan symbol)
};

struct Node {
    float2 p;
    int level = 0;
    bool alive = true;
};

struct Wall {
    int a = -1, b = -1;
    float thickness = 0.30f;
    float height = 2.70f;
    std::vector<Opening> ops;
    bool alive = true;
};

enum FloorMat { kFloorWood = 0, kFloorTile = 1, kFloorConcrete = 2, kFloorCount };
const char* floorName(int m);

// A room name placed in the plan: the room found around the point takes it (the faces are recomputed after every edit).
struct RoomLabel {
    float2 p;
    std::string name;
    int floor = kFloorWood;
    int level = 0;
    bool alive = true;
};

struct Level {
    std::string name;
    float elevation = 0.0f;   // top of the floor, metres
    float height = 2.70f;     // floor to ceiling
};

struct Room {                 // derived (computeRooms)
    std::vector<float2> poly; // counter-clockwise, through the wall axes
    float area = 0;           // m2, inside the walls (axis polygon minus half the walls, approximately)
    int label = -1;
    int level = 0;
    float2 centre;
};

struct WallGeom {             // derived: the mitred outline of a wall
    float2 a, b, u, n;        // axis ends, direction, left normal
    float len = 0;
    float2 aL, aR, bL, bR;    // corners (L = side +n)
    float sL0, sL1, sR0, sR1; // extents of each side along the axis (metres from a)
};

struct Joint {                // derived: the piece of wall where 3 or more walls meet (fills the gap between their ends)
    std::vector<float2> poly; // counter-clockwise around the node
    int level = 0;
    float height = 0;
};

struct Building {
    std::vector<Level> levels;
    std::vector<Node> nodes;
    std::vector<Wall> walls;
    std::vector<RoomLabel> labels;
    std::vector<Room> rooms;          // derived
    std::vector<Joint> joints;        // derived
    std::vector<WallGeom> geom;       // derived, per wall

    Building() { levels.push_back({"Level 0", 0.0f, 2.70f}); }
    void clear() { *this = Building(); }

    // Adds the wall a-b on a level and keeps the graph planar: existing nodes on the way, crossings with other walls
    // and collinear overlaps become shared nodes / walls (a room drawn against the house reuses its façade).
    // Returns the number of new walls.
    int addWall(float2 a, float2 b, int level, float thickness, float height);
    // Node at p (an existing one within tol, else a new one, splitting the wall it lies on).
    int nodeAt(float2 p, int level, float tol = 0.01f);
    int splitWall(int w, float2 p);   // returns the new node
    void removeWall(int w);           // and the nodes left alone
    void removeOpening(int w, int i) { walls[w].ops.erase(walls[w].ops.begin() + i); }
    // Moves a wall along its normal by d (its two nodes: the walls around stretch).
    void moveWall(int w, float d);
    // Sets the length of a wall by moving its end node b along the axis (a node shared with other walls: they follow).
    void setWallLength(int w, float len);
    void compact();                   // drops the dead nodes / walls (indices change)
    void update();                    // geometry + rooms (call after every edit)

    int addLevel();
    float wallLength(int w) const { const WallGeom& g = geom[w]; return g.len; }
    // usable interval of the openings along a wall (between the inner corners)
    void openingRange(int w, float& lo, float& hi) const;
    void clampOpening(int w, Opening& o) const;
    int roomAt(float2 p, int level) const;
    int wallAt(float2 p, int level, float tol, float* along = nullptr) const;   // nearest wall within tol of its face

    std::string toJson() const;
    bool fromJson(const std::string& text);

    static Building demoHouse();

private:
    void computeGeometry();
    void computeRooms();
};

float polyArea(const std::vector<float2>& p);   // signed, > 0 counter-clockwise
bool pointInPoly(float2 p, const std::vector<float2>& poly);
// ear clipping of a simple polygon (CCW): triangles as index triples
void triangulate(const std::vector<float2>& poly, std::vector<int>& tris);

} // namespace archviz
