#include "MaterialManager.h"

sMaterial * CMaterialManager::GetMaterial(std::string const& name)
{
	if(m_materials.find(name) == m_materials.end())
	{
		return NULL;
	}
	return &m_materials.find(name)->second;
}

void CMaterialManager::AddMaterial (std::string const& name, sMaterial const& material)
{
	m_materials[name] = material;
}

void CMaterialManager::InsertMaterials(std::map<std::string, sMaterial> const& materials)
{
	m_materials.insert(materials.begin(), materials.end());
}