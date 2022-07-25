#include "GameCommon.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SimpleTriangleFont.hpp"

void CopyVertices(int vertNum, const Vertex_PCU* vertArrayFrom, Vertex_PCU* vertArrayTo)
{
	for (int idx = 0; idx < vertNum; idx++)
	{
		vertArrayTo[idx] = vertArrayFrom[idx];
	}
}

void CopyVertices(int vertNum, const Vertex_PCU* vertArrayFrom, std::vector<Vertex_PCU>& vertsTo)
{
	vertsTo.reserve(vertsTo.size() + vertNum);
	for (int idx = 0; idx < vertNum; idx++)
	{
		vertsTo.push_back(vertArrayFrom[idx]);
	}
}

void SetColorForVertices(int vertNum, Vertex_PCU* vertArray, Rgba8 color)
{
	for (int idx = 0; idx < vertNum; idx++)
	{
		vertArray[idx].m_color = color;
	}
}

void SetColorRForVertices(int vertNum, Vertex_PCU* vertArray, unsigned char r)
{
	for (int idx = 0; idx < vertNum; idx++)
	{
		vertArray[idx].m_color.r = r;
	}
}

void SetColorGForVertices(int vertNum, Vertex_PCU* vertArray, unsigned char g)
{
	for (int idx = 0; idx < vertNum; idx++)
	{
		vertArray[idx].m_color.g = g;
	}
}

void SetColorBForVertices(int vertNum, Vertex_PCU* vertArray, unsigned char b)
{
	for (int idx = 0; idx < vertNum; idx++)
	{
		vertArray[idx].m_color.b = b;
	}
}

void SetColorAForVertices(int vertNum, Vertex_PCU* vertArray, unsigned char a)
{
	for (int idx = 0; idx < vertNum; idx++)
	{
		vertArray[idx].m_color.a = a;
	}
}

void AddTriangleToVerts(int vertStartIdx, Vertex_PCU* verts, const Vec2& point1, const Vec2& point2, const Vec2& point3, const Rgba8& color)
{
	verts[vertStartIdx + 0].m_position = Vec3(point1.x, point1.y, 0.0f);
	verts[vertStartIdx + 0].m_color = color;
	verts[vertStartIdx + 1].m_position = Vec3(point2.x, point2.y, 0.0f);
	verts[vertStartIdx + 1].m_color = color;
	verts[vertStartIdx + 2].m_position = Vec3(point3.x, point3.y, 0.0f);
	verts[vertStartIdx + 2].m_color = color;
}

void AddRectangleToVerts(int vertStartIdx, Vertex_PCU* verts, const Vec2& origin, const Vec2& size, const Rgba8& color, Vec2 pivot)
{
	Vec2 point1 = Vec2(origin.x - pivot.x * size.x, origin.y + pivot.y * size.y);
	Vec2 point2 = Vec2(origin.x - pivot.x * size.x, origin.y - (1.0f - pivot.y) * size.y);
	Vec2 point3 = Vec2(origin.x + (1.0f - pivot.x) * size.x, origin.y + pivot.y * size.y);
	Vec2 point4 = Vec2(origin.x + (1.0f - pivot.x) * size.x, origin.y - (1.0f - pivot.y) * size.y);

	AddTriangleToVerts(vertStartIdx + 0, verts, point1, point2, point3, color);
	AddTriangleToVerts(vertStartIdx + 3, verts, point4, point3, point2, color);
}

void AddVertsForText(std::vector<Vertex_PCU>& verts_out, const std::string& text, float size, float spacing, Vec2 pivot)
{
	std::vector<Vertex_PCU> verticesText;
	AddVertsForTextTriangles2D(verticesText, text, Vec2(), 1.0f, Rgba8(), 0.56f, false, spacing);

	float textLength = GetSimpleTriangleStringWidth(text, 1.0f, 0.56f, spacing);
	int vertNum = (int)verticesText.size();
	TransformVertexArrayXY3D(vertNum, &verticesText[0], 1.0f, 0.0f, Vec2(-textLength, -1.0f) * pivot);
	TransformVertexArrayXY3D(vertNum, &verticesText[0], size, 0.0f, Vec2());
	CopyVertices(vertNum, &verticesText[0], verts_out);
}

void RenderFontVertices(int vertNum, Vertex_PCU* verts, Vec2 position, Rgba8 color, float size /*= 1.0f*/, bool shadow /*= false*/)
{
	const Vec2 shadowOffset = Vec2(0.05f, -0.05f);
	std::vector<Vertex_PCU> vertsToRender;
	CopyVertices(vertNum, verts, vertsToRender);
	if (shadow)
	{
		SetColorForVertices(vertNum, &vertsToRender[0], Rgba8((unsigned char)(((float)color.r) / 3.0f), (unsigned char)(((float)color.g) / 3.0f), (unsigned char)(((float)color.b) / 3.0f), color.a));
		TransformVertexArrayXY3D(vertNum, &vertsToRender[0], size, 0.0f, position + shadowOffset * size);
		g_theRenderer->DrawVertexArray(vertNum, &vertsToRender[0]);
		SetColorForVertices(vertNum, &vertsToRender[0], color);
		TransformVertexArrayXY3D(vertNum, &vertsToRender[0], 1.0f, 0.0f, -shadowOffset * size);
		g_theRenderer->DrawVertexArray(vertNum, &vertsToRender[0]);
	}
	else
	{
		SetColorForVertices(vertNum, &vertsToRender[0], color);
		TransformVertexArrayXY3D(vertNum, &vertsToRender[0], size, 0.0f, position);
		g_theRenderer->DrawVertexArray(vertNum, &vertsToRender[0]);
	}
}

