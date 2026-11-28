// ArchViz - the lights of the Light step: the fixtures (their body and their light: a rectangular area light of
// the renderer, seen by the camera, in the HQ view and the path tracer alike), the colour of a temperature, the power
// in lumens, and the sun of a clock (solar time, date, latitude).
#pragma once
#include "ArchModel.h"
#include "vp3d/MeshGen.h"
#include "vp3d/Scene.h"

namespace archviz {

const char* fixtureName(int kind);
float fixtureDefaultLumens(int kind);
// The height above the floor of the light of a fixture (Fixture::height, or the default of its kind).
float fixtureHeight(const Building& B, const Fixture& f);

// Its body (housing, shade, cord) in world space and its light (an area light facing the way it shines).
void buildFixture(const Building& B, const Fixture& f, vp3d::MeshData& body, vp3d::AreaLight& light);

// Linear RGB of a black body at kelvin, luminance 1.
vp3d::float3 kelvinColor(float kelvin);

// The radiance factor of a one-sided rectangular light of area a (m2) emitting lumens (the renderer's units: the sun
// of the sky gives about 3.2 for 100 000 lux).
float lumensToIntensity(float lumens, float area);

// The sun of a clock: direction towards it (world: x east, y up, -z north) and its elevation (radians; < 0 at night).
vp3d::float3 sunDirection(float latitudeDeg, int month, int day, float solarHour, float* elevation = nullptr);

} // namespace archviz
