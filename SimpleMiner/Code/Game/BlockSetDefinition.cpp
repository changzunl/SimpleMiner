#include "BlockSetDefinition.hpp"

#include "BlockDef.hpp"
#include "BlockMaterialDef.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/XmlUtils.hpp"

bool BlockSetDefinition::LoadFromXmlElement(const XmlElement& element)
{
	m_name = ParseXmlAttribute(element, "name", m_name);
	ASSERT_OR_DIE(m_name.size() != 0, "Name not assigned");

	const XmlElement* pElement = nullptr;
	pElement = element.FirstChildElement("BlockMaterialAtlas");
	ASSERT_OR_DIE(pElement, "Atlas not present");

	m_blockMatAtlas = new BlockMaterialAtlas();
	m_blockMatAtlas->LoadFromXmlElement(*pElement);

	pElement = element.FirstChildElement("BlockMaterials");
	ASSERT_OR_DIE(pElement, "Materials not present");
	for (const XmlElement* pEMat = pElement->FirstChildElement("BlockMaterial"); pEMat; pEMat = pEMat->NextSiblingElement("BlockMaterial"))
	{
		BlockMaterialDef* mat = new BlockMaterialDef();
		mat->LoadFromXmlElement(*pEMat, *m_blockMatAtlas);
		m_blockMatDefs.emplace_back(mat);
	}

	pElement = element.FirstChildElement("Blocks");
	ASSERT_OR_DIE(pElement, "Blocks not present");
	for (const XmlElement* pEBlock = pElement->FirstChildElement("Block"); pEBlock; pEBlock = pEBlock->NextSiblingElement("Block"))
	{
		BlockDef* block = new BlockDef();
		block->LoadFromXmlElement(*pEBlock);
		block->m_blockId = (unsigned char)m_blockDefs.size();
		m_blockDefs.emplace_back(block);
	}

	return true;
}

const BlockMaterialAtlas* BlockSetDefinition::GetBlockMaterialAtlas() const
{
	return m_blockMatAtlas;
}

const BlockDef* BlockSetDefinition::GetBlockDefByName(const char* name) const
{
	for (auto& definition : m_blockDefs)
	{
		if (_stricmp(definition->m_name.c_str(), name) == 0)
		{
			return definition;
		}
	}
	return nullptr;
}


BlockMaterialDef* BlockSetDefinition::GetBlockMatDefByName(const char* name) const
{
	for (auto& definition : m_blockMatDefs)
	{
		if (_stricmp(definition->m_name.c_str(), name) == 0)
		{
			return definition;
		}
	}
	return nullptr;
}

void BlockSetDefinition::InitializeDefinition(const char* path)
{

	XmlDocument document;
	document.LoadFile(path);

	const XmlElement* pElement = document.RootElement()->FirstChildElement("BlockSetDefinition");
	ASSERT_OR_DIE(pElement, "No block set present");

	delete s_definition;
	s_definition = new BlockSetDefinition();
	bool load = s_definition->LoadFromXmlElement(*pElement);
	if (!load)
	{
		DebuggerPrintf(Stringf("Failed to load set definition: %s", pElement->GetText()).c_str());
		delete s_definition;
		s_definition = nullptr;
	}
}
BlockSetDefinition* BlockSetDefinition::s_definition = nullptr;

