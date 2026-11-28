// ArchViz - generic joinery (Wzx 28/11/2026: « des menuiseries génériques, porte, fenêtre, modulaires et animables »).
// One parametric description per opening (Opening: kind, style, leaves, glazing grid, finish, hinge / swing side,
// position in the wall depth, how open) gives the pieces of its joinery: the fixed frame and the moving leaves / sashes /
// panels, each with its motion (a hinge axis and an angle, or a slide). The meshes are built once; opening and closing
// only move the objects of the leaves (no geometry rebuilt, the BLAS stay).
// Portable C++ (PC, iPad, Linux tests): no renderer.
#pragma once
#include "ArchModel.h"
#include "vp3d/MeshGen.h"
#include <vector>

namespace archviz {

// Styles per kind of opening (Opening::style)
enum WindowStyle { kWinCasement = 0, kWinFixed, kWinTiltTurn, kWinAwning, kWinSliding, kWinStyleCount };
enum DoorStyle { kDoorPanel = 0, kDoorGlazed, kDoorEntrance, kDoorFrench, kDoorStyleCount };
enum SliderStyle { kSlideBypass = 0, kSlidePocket, kSlideStyleCount };
// Finishes of the frames (Opening::finish)
enum JoineryFinish { kFinishAnthracite = 0, kFinishWhite, kFinishOak, kFinishBlack, kFinishCount };

int joineryStyleCount(int kind);
const char* joineryStyleName(int kind, int style);
const char* joineryFinishName(int finish);
int joineryLeaves(const Opening& o);           // the leaves / panels actually built (Opening::leaves 0 = automatic)
int joineryMaxLeaves(const Opening& o);        // what the style allows (1: no choice)
void joinerySyncOpen(Opening& o);              // Opening::open = the most open of its leaves (the inspector's slider)

// Materials of the pieces (the scene gives one per finish)
enum JoineryMat { kJFrame = 0, kJGlass, kJPanel, kJHandle, kJMatCount };

// A piece of joinery in the local space of its opening: x along the wall from the left jamb (0 .. width), y up from
// the sill (0 .. height), z across the wall towards -n (the wall's left normal reversed: (x, y, z) is right-handed in
// the world). Moving pieces have their meshes relative to their pivot; at open = t the piece turns by angle * t around
// axis (through the pivot) and slides by slide * t. The fixed pieces have pivot 0 and no motion.
struct JoineryPiece {
    vp3d::MeshData mesh[kJMatCount];
    vp3d::float3 pivot{0, 0, 0};
    vp3d::float3 axis{0, 1, 0};
    float angle = 0.0f;
    vp3d::float3 slide{0, 0, 0};
    bool moving = false;
    // a moving piece is a leaf / sash / panel: its index in the opening (Opening::leafOpen), the kind of its handle in
    // the views (0 an arc around the hinge, 1 an arrow along the track, 2 an arrow pushing it out: awning) and where
    // the handle sits (local space of the opening, closed). Tilt & turn: open < 0 tilts it (second motion, its arrow
    // at the top of the sash), open > 0 turns it.
    int leaf = -1;
    int handleKind = 0;
    vp3d::float3 anchor{0, 0, 0};
    bool hasTilt = false;
    vp3d::float3 pivot2{0, 0, 0}, axis2{1, 0, 0}, anchor2{0, 0, 0};
    float angle2 = 0.0f;
};

// The pieces of the joinery of an opening in a wall of that thickness; sideN: the side its leaves open to, in units of
// the wall's left normal (Building::openingSide).
void buildJoinery(const Opening& o, float wallThickness, float sideN, std::vector<JoineryPiece>& out);

} // namespace archviz
