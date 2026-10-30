#include "r_war3map.h"

/* Each original water triangle may become two triangles after clipping to
 * the actual ground surface; six vertices per tile is no longer sufficient. */
static vertex_t water_vertex_buffer[(SEGMENT_SIZE+1)*(SEGMENT_SIZE+1)*12];
static vertex_t *water_current_vertex = NULL;

// FUNCTIONS

/* Warcraft W3E can flag a corner as wet even where its authored ground is
 * higher than the interpolated water surface. A full six-vertex water quad
 * on those cells places opaque water over dry terrain (ORDR scene flood).
 * Clip the actual triangles at the ground/water intersection instead of
 * disabling water or altering W3E map data. */
typedef struct {
    vertex_t vertex;
    float submerged_depth;
} waterClipPoint_t;

static waterClipPoint_t R_WaterClipIntersection(waterClipPoint_t const *a,
                                               waterClipPoint_t const *b) {
    float denominator = a->submerged_depth - b->submerged_depth;
    float t = fabsf(denominator) > 1e-6f
        ? MAX(0.0f, MIN(1.0f, a->submerged_depth / denominator)) : 0.0f;
    waterClipPoint_t out = *a;
    out.submerged_depth = 0.0f;
    out.vertex.position.x = LerpNumber(a->vertex.position.x, b->vertex.position.x, t);
    out.vertex.position.y = LerpNumber(a->vertex.position.y, b->vertex.position.y, t);
    out.vertex.position.z = LerpNumber(a->vertex.position.z, b->vertex.position.z, t);
    out.vertex.texcoord.x = LerpNumber(a->vertex.texcoord.x, b->vertex.texcoord.x, t);
    out.vertex.texcoord.y = LerpNumber(a->vertex.texcoord.y, b->vertex.texcoord.y, t);
    out.vertex.color.r = (uint8_t)lroundf(LerpNumber(a->vertex.color.r, b->vertex.color.r, t));
    out.vertex.color.g = (uint8_t)lroundf(LerpNumber(a->vertex.color.g, b->vertex.color.g, t));
    out.vertex.color.b = (uint8_t)lroundf(LerpNumber(a->vertex.color.b, b->vertex.color.b, t));
    out.vertex.color.a = (uint8_t)lroundf(LerpNumber(a->vertex.color.a, b->vertex.color.a, t));
    return out;
}

/* Sutherland-Hodgman clipping against the signed submerged-depth plane.
 * A triangle becomes 0, 3, or 4 vertices; output can be triangulated into
 * at most two triangles while preserving its authored water texture. */
static uint32_t R_ClipWaterTriangle(waterClipPoint_t const tri[3],
                                    waterClipPoint_t out[4]) {
    uint32_t count = 0;
    FOR_LOOP(i, 3) {
        waterClipPoint_t const *a = tri + i, *b = tri + (i + 1) % 3;
        bool a_inside = a->submerged_depth > 0.0f;
        bool b_inside = b->submerged_depth > 0.0f;
        if (a_inside) out[count++] = *a;
        if (a_inside != b_inside)
            out[count++] = R_WaterClipIntersection(a, b);
    }
    return count;
}

static void R_EmitClippedWaterTriangle(waterClipPoint_t const tri[3]) {
    waterClipPoint_t polygon[4];
    uint32_t count = R_ClipWaterTriangle(tri, polygon);
    for (uint32_t i = 1; i + 1 < count; i++) {
        if (water_current_vertex + 3 >
            water_vertex_buffer + sizeof(water_vertex_buffer) / sizeof(water_vertex_buffer[0]))
            return;
        *water_current_vertex++ = polygon[0].vertex;
        *water_current_vertex++ = polygon[i].vertex;
        *water_current_vertex++ = polygon[i + 1].vertex;
    }
}

#ifdef BZ_TESTS
#include "shared/test.h"
TEST(wc3_water, submerged_water_triangles_clip_at_terrain_instead_of_flooding_land) {
    waterClipPoint_t tri[3] = {0}, out[4];
    tri[0].submerged_depth = -5.0f;
    tri[1].submerged_depth = -3.0f;
    tri[2].submerged_depth = -1.0f;
    T_EQ(R_ClipWaterTriangle(tri, out), 0u);
    tri[0].submerged_depth = 3.0f;
    T_EQ(R_ClipWaterTriangle(tri, out), 3u);
    tri[1].submerged_depth = 3.0f;
    T_EQ(R_ClipWaterTriangle(tri, out), 4u);
    tri[2].submerged_depth = 3.0f;
    T_EQ(R_ClipWaterTriangle(tri, out), 3u);
}
#endif

/* Only authored wet W3E corners contribute to a water polygon.
 * A dry vertex with a high authored waterlevel is NOT itself flooded; an
 * old any-corner water test incorrectly drew a full sea quad over land. */
static float R_WaterCornerDepth(float surface_z, float ground_z, bool authored_wet) {
    float const signed_depth = surface_z - ground_z - 0.25f;
    if (!authored_wet)
        return -MAX(TILE_SIZE * 0.5f, fabsf(signed_depth));
    return signed_depth;
}

#ifdef BZ_TESTS
TEST(wc3_water, original_w3e_dry_corner_never_floods) {
    T_ASSERT(R_WaterCornerDepth(250, 0, false) < 0);
    T_ASSERT(R_WaterCornerDepth(250, 0, true) > 0);
    T_ASSERT(R_WaterCornerDepth(0, 250, true) < 0);
}
#endif

static void R_MakeWaterTile(war3map_t const *map, uint32_t x, uint32_t y) {
    struct War3MapVertex tile[4];
    GetTileVertices(x, y, tr.world, tile);

    if (!IsTileWater(tile))
        return;

    vec2_t const pos[] = {
        { tr.world->center.x + x * TILE_SIZE, tr.world->center.y + y * TILE_SIZE },
        { tr.world->center.x + (x + 1) * TILE_SIZE, tr.world->center.y + y * TILE_SIZE },
        { tr.world->center.x + (x + 1) * TILE_SIZE, tr.world->center.y + (y + 1) * TILE_SIZE },
        { tr.world->center.x + x * TILE_SIZE, tr.world->center.y + (y + 1) * TILE_SIZE },
    };

    float const waterlevel[] = {
        GetWar3MapVertexWaterLevel(&tile[3]),
        GetWar3MapVertexWaterLevel(&tile[2]),
        GetWar3MapVertexWaterLevel(&tile[0]),
        GetWar3MapVertexWaterLevel(&tile[1]),
    };

    /* Compare water to the EXACT terrain mesh vertex height, including
     * cliff ramp offsets and runtime deformations. This matches the ground
     * triangles rendered in r_war3map_ground.c. */
    float const height[] = {
        R_GetVertexPosition(map, x,     y,     true).z,
        R_GetVertexPosition(map, x + 1, y,     true).z,
        R_GetVertexPosition(map, x + 1, y + 1, true).z,
        R_GetVertexPosition(map, x,     y + 1, true).z,
    };
    bool const authored_wet[] = {
        tile[3].water != 0, tile[2].water != 0,
        tile[0].water != 0, tile[1].water != 0,
    };

    wc3WaterStyle_t const *style = R_WaterStyle();
    struct color32 const color[] = {
        R_WaterDepthColor(style, (waterlevel[0] - height[0]) / TILE_SIZE),
        R_WaterDepthColor(style, (waterlevel[1] - height[1]) / TILE_SIZE),
        R_WaterDepthColor(style, (waterlevel[2] - height[2]) / TILE_SIZE),
        R_WaterDepthColor(style, (waterlevel[3] - height[3]) / TILE_SIZE),
    };
    
#define WATER_SCALE(x,y) (((x%3)+y)/3.0)
    
    vec2_t const tc[] = {
        { WATER_SCALE(x, 0),  WATER_SCALE(y, 0) },
        { WATER_SCALE(x, 1),  WATER_SCALE(y, 0) },
        { WATER_SCALE(x, 1),  WATER_SCALE(y, 1) },
        { WATER_SCALE(x, 0),  WATER_SCALE(y, 1) },
    };

    struct vertex geom[] = {
        {
            .position = { pos[0].x, pos[0].y, waterlevel[0] },
            .texcoord = tc[0],
            .normal = { 0, 0, 1 },
            .color = color[0],
        },
        {
            .position = { pos[1].x, pos[1].y, waterlevel[1] },
            .texcoord = tc[1],
            .normal = { 0, 0, 1 },
            .color = color[1],
        },
        {
            .position = { pos[2].x, pos[2].y, waterlevel[2] },
            .texcoord = tc[2],
            .normal = { 0, 0, 1 },
            .color = color[2],
        },
        {
            .position = { pos[0].x, pos[0].y, waterlevel[0] },
            .texcoord = tc[0],
            .normal = { 0, 0, 1 },
            .color = color[0],
        },
        {
            .position = { pos[2].x, pos[2].y, waterlevel[2] },
            .texcoord = tc[2],
            .normal = { 0, 0, 1 },
            .color = color[2],
        },
        {
            .position = { pos[3].x, pos[3].y, waterlevel[3] },
            .texcoord = tc[3],
            .normal = { 0, 0, 1 },
            .color = color[3],
        },
    };

    /* A water bit means this region may contain water, not that all its
     * vertices are below the water surface. Compare to the same deformed
     * ground heights used to draw the underlying W3E terrain. */
    float const depth[] = {
        R_WaterCornerDepth(waterlevel[0], height[0], authored_wet[0]),
        R_WaterCornerDepth(waterlevel[1], height[1], authored_wet[1]),
        R_WaterCornerDepth(waterlevel[2], height[2], authored_wet[2]),
        R_WaterCornerDepth(waterlevel[3], height[3], authored_wet[3]),
    };
    waterClipPoint_t const triangle_a[3] = {
        { geom[0], depth[0] }, { geom[1], depth[1] }, { geom[2], depth[2] },
    };
    waterClipPoint_t const triangle_b[3] = {
        { geom[3], depth[0] }, { geom[4], depth[2] }, { geom[5], depth[3] },
    };
    R_EmitClippedWaterTriangle(triangle_a);
    R_EmitClippedWaterTriangle(triangle_b);
}

/* The wet flags, terrain heights, and surface intersection determine vertex
 * count. SetWaterBaseColor changes only the color attributes, so both initial
 * creation and a tint update can use the identical deterministic bake. */
static uint32_t R_BakeMapSegmentWater(war3map_t const *map, uint32_t sx, uint32_t sy) {
    water_current_vertex = water_vertex_buffer;
    for (uint32_t x = sx * SEGMENT_SIZE; x < (sx + 1) * SEGMENT_SIZE; x++) {
        for (uint32_t y = sy * SEGMENT_SIZE; y < (sy + 1) * SEGMENT_SIZE; y++) {
            R_MakeWaterTile(map, x, y);
        }
    }
    return (uint32_t)(water_current_vertex - water_vertex_buffer);
}

/* Reuse the existing VAO/VBO if only the tint changed; terrain deformations
 * use the separate segment-rebuild path because they can change clipping. */
bool R_UpdateMapSegmentWaterColors(war3map_t const *map, uint32_t sx, uint32_t sy,
                                   maplayer_t *layer) {
    if (!layer || layer->type != MAPLAYERTYPE_WATER || !layer->buffer ||
        !R_WaterStyle()->num_frames) return false;
    uint32_t const count = R_BakeMapSegmentWater(map, sx, sy);
    if (count != layer->num_vertices) return false;
    if (count) R_UpdateVertexArrayObject(layer->buffer, 0, water_vertex_buffer, count);
    layer->texture = R_WaterStyle()->frames[0];
    return true;
}

maplayer_t *R_BuildMapSegmentWater(war3map_t const *map, uint32_t sx, uint32_t sy) {
    /* R_LoadWaterStyle already reported a missing Water.slk row or texture for this tileset. */
    if (!R_WaterStyle()->num_frames) return NULL;
    uint32_t const count = R_BakeMapSegmentWater(map, sx, sy);
    /* Dry terrain contributes no water geometry. Avoid a zero-vertex buffer
     * upload for each empty segment without changing rendered water. */
    if (!count) return NULL;
    maplayer_t *mapLayer = ri.MemAlloc(sizeof(maplayer_t));
    mapLayer->type = MAPLAYERTYPE_WATER;
    mapLayer->texture = R_WaterStyle()->frames[0]; /* _W3M_DrawAlphaSurfaces selects the animated frame. */
    mapLayer->num_vertices = count;
    mapLayer->buffer = R_MakeVertexArrayObject(water_vertex_buffer, count);
    return mapLayer;
}
