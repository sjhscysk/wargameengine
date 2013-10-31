#pragma once
#include "ICommand.h"
#include "..\ObjectInterface.h"
#include <vector>
#include <memory>

class CCommandHandler
{
public:
	CCommandHandler():m_current(-1) {}
	static std::weak_ptr<CCommandHandler> GetInstance();
	static void FreeInstance();
	void AddNewCreateObject(std::shared_ptr<IObject> object);
	void AddNewDeleteObject(std::shared_ptr<IObject> object);
	void AddNewMoveObject(double deltaX, double deltaY);
	void Undo();
	void Redo();
private:
	static std::shared_ptr<CCommandHandler> m_instance;
	std::vector<ICommand *> m_commands;
	int m_current;
};

