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
    int flip = 0;          // bit 0: the leaves open to the other side of the wall, bit 1: hinged on the right (or the
                           // panels stack to the right)
    // Joinery (ArchJoinery.h): the style per kind (WindowStyle, DoorStyle, SliderStyle), the leaves / panels (0 =
    // automatic by width), the glazing bars (lights across x up), the finish of the frames, how far it is set into the
    // wall from its axis (metres, towards the side the leaves do not open to = +n), and how open it is (0..1).
    int style = 0;
    int leaves = 0;
    int cols = 1, rows = 1;
    int finish = 0;
    float inset = 0.0f;
    float open = 0.0f;          // the inspector's slider: sets every leaf
    // each leaf / sash / panel on its own (0 closed .. 1 open; a tilt & turn sash: -1 .. 0 tilted in)
    float leafOpen[4] = {0, 0, 0, 0};
    void setAllOpen(float v) { open = v; for (float& l : leafOpen) l = v; }
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

enum RoofType { kRoofNone = 0, kRoofFlat, kRoofGable, kRoofHip, kRoofShed, kRoofTypeCount };
enum RoofFinish { kRoofTilesGrey = 0, kRoofTilesRed, kRoofZinc, kRoofSlate, kRoofFinishCount };

struct Level {
    std::string name;
    float elevation = 0.0f;   // top of the floor, metres
    float height = 2.70f;     // floor to ceiling
    // the roof over this level when it is the top one (ArchRoof.h): type, slope (degrees), overhang past the walls,
    // thickness, ridge across the long side instead of along it, finish
    int roof = kRoofGable;
    float roofSlope = 35.0f;
    float roofOverhang = 0.40f;
    float roofThick = 0.25f;
    int roofRidge = 0;
    int roofFinish = kRoofTilesGrey;
};

// A flight of stairs from a level to the next (ArchStair.h): its foot (the front of the first step, middle of its
// width) and the direction it climbs, its shape, width, rise (0 = the height of its level + the slab above), risers
// (0 = automatic: Blondel 2h + g = 63 cm), handrail, open risers, finish.
enum StairType { kStairStraight = 0, kStairL, kStairU, kStairSpiral, kStairTypeCount };
enum StairFinish { kStairOak = 0, kStairConcrete, kStairWhite, kStairFinishCount };
struct Stair {
    float2 p;
    float yaw = 0;            // radians: the direction of the climb in the plan (0 = +x)
    int type = kStairStraight;
    int level = 0;
    float width = 0.90f;
    float rise = 0;
    int risers = 0;
    bool handrail = true;
    bool open = false;
    bool turnRight = false;   // L / U: the turn (spiral: clockwise)
    int finish = kStairOak;
    bool alive = true;
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

struct Item {                 // a piece of furniture / an object placed on a level (an asset of the library)
    std::string asset;        // slug of the asset (its folder in the library)
    float2 p;                 // plan position of its centre
    float yaw = 0;            // radians, counter-clockwise in the plan
    float elev = 0;           // above the floor of its level
    int level = 0;
    float2 size{1, 1};        // footprint (width x depth, metres) and height, copied from the asset when placed: the
    float height = 1;         // plan and the picking work without the asset loaded
};

struct Building {
    std::vector<Level> levels;
    std::vector<Stair> stairs;
    std::vector<Item> items;
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
    // The side (in units of the wall's left normal n: +1 / -1) the leaves of an opening open to: into the room when the
    // wall has a room on one side only, else -n; Opening::flip bit 0 turns it over.
    float openingSide(int w, const Opening& o) const;
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
