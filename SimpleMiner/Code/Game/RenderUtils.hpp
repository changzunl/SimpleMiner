#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/Vec2.hpp"
#include <string>
#include <vector>

void CopyVertices(int vertNum, const Vertex_PCU* vertArrayFrom, Vertex_PCU* vertArrayTo);
void CopyVertices(int vertNum, const Vertex_PCU* vertArrayFrom, std::vector<Vertex_PCU>& vertsTo);
void SetColorForVertices(int vertNum, Vertex_PCU* vertArrayFrom, Rgba8 color);
void SetColorRForVertices(int vertNum, Vertex_PCU* vertArray, unsigned char r);
void SetColorGForVertices(int vertNum, Vertex_PCU* vertArray, unsigned char g);
void SetColorBForVertices(int vertNum, Vertex_PCU* vertArray, unsigned char b);
void SetColorAForVertices(int vertNum, Vertex_PCU* vertArray, unsigned char a);

void AddTriangleToVerts(int vertStartIdx, Vertex_PCU* verts, const Vec2& point1, const Vec2& point2, const Vec2& point3, const Rgba8& color);
void AddRectangleToVerts(int vertStartIdx, Vertex_PCU* verts, const Vec2& origin, const Vec2& size, const Rgba8& color, Vec2 pivot = Vec2(0.5f, 0.5f));

void AddVertsForText(std::vector<Vertex_PCU>& verts_out, const std::string& text, float size = 1.0f, float spacing = 0.2f, Vec2 pivot = Vec2(0.5f, 0.5f));
void RenderFontVertices(int vertNum, Vertex_PCU* verts, Vec2 position, Rgba8 color, float size = 1.0f, bool shadow = false);

