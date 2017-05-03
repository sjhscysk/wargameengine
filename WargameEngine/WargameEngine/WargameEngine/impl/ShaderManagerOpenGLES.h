#pragma once
#include "../view/IShaderManager.h"
#include <functional>
#include <map>
#include <vector>

class CShaderManagerOpenGLES : public IShaderManager
{
public:
	~CShaderManagerOpenGLES();
	std::unique_ptr<IShaderProgram> NewProgram(const Path& vertex = Path(), const Path& fragment = Path(), const Path& geometry = Path()) override;
	std::unique_ptr<IShaderProgram> NewProgramSource(std::string const& vertex = "", std::string const& fragment = "", std::string const& geometry = "") override;
	void PushProgram(IShaderProgram const& program) const override;
	void PopProgram() const override;

	virtual void SetUniformValue(std::string const& uniform, int elementSize, size_t count, const float* value) const override;
	virtual void SetUniformValue(std::string const& uniform, int elementSize, size_t count, const int* value) const override;
	virtual void SetUniformValue(std::string const& uniform, int elementSize, size_t count, const unsigned int* value) const override;

	virtual void SetVertexAttribute(std::string const& attribute, int elementSize, size_t count, const float* values, bool perInstance = false) const override;
	virtual void SetVertexAttribute(std::string const& attribute, int elementSize, size_t count, const int* values, bool perInstance = false) const override;
	virtual void SetVertexAttribute(std::string const& attribute, int elementSize, size_t count, const unsigned int* values, bool perInstance = false) const override;
	virtual void SetVertexAttribute(std::string const& attribute, IVertexAttribCache const& cache, int elementSize, size_t count, TYPE type, bool perInstance = false, size_t offset = 0) const override;

	virtual void DisableVertexAttribute(std::string const& attribute, int size, const float* defaultValue) const override;
	virtual void DisableVertexAttribute(std::string const& attribute, int size, const int* defaultValue) const override;
	virtual void DisableVertexAttribute(std::string const& attribute, int size, const unsigned int* defaultValue) const override;

	virtual std::unique_ptr<IVertexAttribCache> CreateVertexAttribCache(size_t size, const void* value) const override;

	void DoOnProgramChange(std::function<void()> const& handler);

	void SetInputAttributes(const void* vertices, const void* normals, const void* texCoords, size_t count, size_t vertexComponents);
	void SetInputAttributes(const IVertexAttribCache& cache, size_t vertexOffset, size_t normalOffset, size_t texCoordOffset, size_t stride);
	void SetMaterial(const float* ambient, const float* diffuse, const float* specular, const float shininess);

private:
	struct ShaderProgramCache
	{
		std::map<std::string, int> attribLocations;
		std::map<std::string, int> uniformLocations;
		std::map<std::string, bool> attribState;
	};

	void SetVertexAttributeImpl(std::string const& attribute, int elementSize, size_t count, const void* values, bool perInstance, unsigned int format) const;
	ShaderProgramCache& GetProgramCache() const;
	int GetUniformLocation(std::string const& uniform) const;
	void NewProgramImpl(unsigned prgm, unsigned vertexShader, unsigned framgentShader);

	mutable std::vector<const IShaderProgram*> m_programs;
	mutable unsigned int m_activeProgram;
	std::function<void()> m_onProgramChange;
	mutable std::map<std::string, unsigned> m_vertexAttribBuffers;
	mutable unsigned m_vertexInputBuffer = 0;

	mutable std::map<unsigned, ShaderProgramCache> m_shaderProgramCache;
};