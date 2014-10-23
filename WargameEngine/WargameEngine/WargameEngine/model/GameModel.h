#pragma once
#include <memory>
#include "ObjectInterface.h"
#include <vector>
#include <string>
#include <map>

class CGameModel
{
private:
	std::vector<std::shared_ptr<IObject>> m_objects;
	std::shared_ptr<IObject> m_selectedObject;
	static std::shared_ptr<CGameModel> m_instanse;
	std::map<std::string, std::string> m_properties;

	CGameModel(void):m_selectedObject(NULL){};
	CGameModel(CGameModel const&){};
public:
	~CGameModel(void);
	static std::weak_ptr<CGameModel> GetInstance();
	static void FreeInstance();
	unsigned long GetObjectCount() const;
	void Clear();
	std::shared_ptr<const IObject> Get3DObject(unsigned long number) const;
	std::shared_ptr<IObject> Get3DObject(unsigned long number);
	std::shared_ptr<IObject> Get3DObject(IObject * obj);
	void AddObject(std::shared_ptr<IObject> pObject);
	void DeleteObjectByPtr(std::shared_ptr<IObject> pObject);
	void SelectObject(std::shared_ptr<IObject> pObject);
	std::shared_ptr<const IObject> GetSelectedObject() const;
	std::shared_ptr<IObject> GetSelectedObject();
	static bool IsGroup(IObject* object);
	void SetProperty(std::string const& key, std::string const& value);
	std::string const GetProperty(std::string const& key);
	std::map<std::string, std::string> const& GetAllProperties() const;
	std::vector<char> GetState(bool hasAdresses = false) const;
	void SetState(char* data, bool hasAdresses = false);
};