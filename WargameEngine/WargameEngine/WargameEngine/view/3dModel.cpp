#include "3dModel.h"
#define GLEW_STATIC
#include <GL/glew.h>
#include "gl.h"

C3DModel::C3DModel(std::shared_ptr<IBounding> bounding, double scale)
{ 
	m_bounding = bounding; 
	m_scale = scale; 
}

C3DModel::C3DModel(std::vector<CVector3f> & vertices, std::vector<CVector2f> & textureCoords, std::vector<CVector3f> & normals, std::vector<unsigned int> & indexes,
				   CMaterialManager & materials, std::vector<sMesh> & meshes, std::shared_ptr<IBounding> bounding, double scale)
{
	SetModel(vertices, textureCoords, normals, indexes, materials, meshes);
	m_bounding = bounding;
	m_scale = scale;
}

void DeleteList(std::map<std::set<std::string>, unsigned int> const& list)
{
	for (auto i = list.begin(); i != list.end(); ++i)
	{
		glDeleteLists(i->second, 1);
	}
}

C3DModel::~C3DModel()
{
	if (m_vbo) glDeleteBuffersARB(1, &m_vbo);
	DeleteList(m_lists);
	DeleteList(m_vertexLists);
}

void C3DModel::SetModel(std::vector<CVector3f> & vertices, std::vector<CVector2f> & textureCoords, std::vector<CVector3f> & normals, std::vector<unsigned int> & indexes,
	CMaterialManager & materials, std::vector<sMesh> & meshes)
{
	m_vbo = NULL;
	if (NULL && GLEW_ARB_vertex_buffer_object)//will be needed for animation
	{
		glGenBuffersARB(1, &m_vbo);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbo);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, vertices.size() * 3 * sizeof(float) + normals.size() * 3 * sizeof(float) + textureCoords.size() * 2 * sizeof(float), NULL, GL_STATIC_DRAW_ARB);
		glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, vertices.size() * 3 * sizeof(float), &vertices[0]);
		glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, vertices.size() * 3 * sizeof(float), normals.size() * 3 * sizeof(float), &normals[0]);
		glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, vertices.size() * 3 * sizeof(float)+normals.size() * 3 * sizeof(float), textureCoords.size() * 2 * sizeof(float), &textureCoords[0]);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}
	m_vertices.swap(vertices);
	m_textureCoords.swap(textureCoords);
	m_normals.swap(normals);
	m_count = (indexes.empty())?vertices.size():indexes.size();
	m_indexes.swap(indexes);
	std::swap(m_materials, materials);
	m_meshes.swap(meshes);
	DeleteList(m_lists);
	DeleteList(m_vertexLists);
	m_lists.clear();
	m_vertexLists.clear();
}

void C3DModel::SetAnimation(std::vector<unsigned int> & weightCount, std::vector<unsigned int> & weightIndexes, std::vector<float> & weights, std::vector<sJoint> & skeleton, std::vector<sAnimation> & animations)
{
	m_weightsCount.swap(weightCount);
	m_weightsIndexes.swap(weightIndexes);
	m_weights.swap(weights);
	m_skeleton.swap(skeleton);
	m_animations.swap(animations);
}

void C3DModel::SetBounding(std::shared_ptr<IBounding> bounding, double scale)
{
	m_bounding = bounding;
	m_scale = scale;
}

void SetMaterial(const sMaterial * material)
{
	if(!material)
	{
		return;
	}
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT, material->ambient);
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE, material->diffuse);
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR, material->specular);
	glMaterialf(GL_FRONT,GL_SHININESS, material->shininess);
	CTextureManager * texManager = CTextureManager::GetInstance();
	texManager->SetTexture(material->texture);
}

void C3DModel::NewList(unsigned int & list, const std::set<std::string> * hideMeshes, bool vertexOnly)
{
	if (m_vbo)
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbo);
		if (!m_vertices.empty())
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, 0);
		}
		if (!m_normals.empty() && !vertexOnly)
		{
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, 0, (void*)(m_vertices.size() * 3 * sizeof(float)));
		}
		if (!m_textureCoords.empty() && !vertexOnly)
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, (void*)(m_vertices.size() * 3 * sizeof(float)+m_normals.size() * 3 * sizeof(float)));
		}
	}
	else
	{
		if (!m_vertices.empty())
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, &m_vertices[0]);
		}
		if (!m_normals.empty() && !vertexOnly)
		{
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, 0, &m_normals[0]);
		}
		if (!m_textureCoords.empty() && !vertexOnly)
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, &m_textureCoords[0]);
		}
	}
	list = glGenLists(1);
	glNewList(list, GL_COMPILE);
	glPushMatrix();
	glScaled(m_scale, m_scale, m_scale);
	if (!m_indexes.empty()) //Draw by meshes;
	{
		unsigned int begin = 0;
		unsigned int end;
		for (unsigned int i = 0; i < m_meshes.size(); ++i)
		{
			if (hideMeshes && hideMeshes->find(m_meshes[i].name) != hideMeshes->end())
			{
				end = m_meshes[i].polygonIndex;
				glDrawElements(GL_TRIANGLES, end - begin, GL_UNSIGNED_INT, &m_indexes[begin]);
				SetMaterial(m_materials.GetMaterial(m_meshes[i].materialName));
				begin = (i + 1 == m_meshes.size()) ? m_count : m_meshes[i + 1].polygonIndex;
				continue;
			}
			if (vertexOnly || (i > 0 && m_meshes[i].materialName == m_meshes[i - 1].materialName))
			{
				continue;
			}
			end = m_meshes[i].polygonIndex;
			glDrawElements(GL_TRIANGLES, end - begin, GL_UNSIGNED_INT, &m_indexes[begin]);
			if (!vertexOnly) SetMaterial(m_materials.GetMaterial(m_meshes[i].materialName));
			begin = end;
		}
		end = m_count;
		if (begin != end)
		{
			glDrawElements(GL_TRIANGLES, end - begin, GL_UNSIGNED_INT, &m_indexes[begin]);
		}
	}
	else //Draw in a row
	{
		glDrawArrays(GL_TRIANGLES, 0, m_count);
	}
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	sMaterial empty;
	SetMaterial(&empty);
	glPopMatrix();
	glEndList();
	if (m_vbo) glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void MultiplyVectorToMatrix(CVector3f & vect, float * matrix)
{
	float result[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			result[i] += matrix[i * 4 + j] * ((j == 3)?1.0f:vect[j]);
		}
	}
	if (result[3] != 0.0f)
	{
		for (int i = 0; i < 3; ++i)
		{
			result[i] /= result[3];
		}
	}
	vect = CVector3f(result);
}


void MultiplyMatrices(float * a, float * b)
{
	float c[16];
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			c[i * 4 + j] = 0;
			for (int k = 0; k < 4; k++)
			{
				c[i * 4 + j] = c[i * 4 + j] + a[i * 4 + k] * b[k * 4 + j];
			}
		}
	}
	memcpy(a, c, sizeof(float) * 16);
}

void AddAllChildren(std::vector<sAnimation> const& anims, unsigned int current, std::vector<sAnimation> & set)
{
	set.push_back(anims[current]);
	for (unsigned int i = 0; i < anims[current].children.size(); ++i)
	{
		AddAllChildren(anims, anims[current].children[i], set);
	}
}

void C3DModel::Draw(const std::set<std::string> * hideMeshes, bool vertexOnly, std::string const& animationToPlay, long time)
{
	std::vector<CVector3f> vertices;
	unsigned int k = 0;//weightIndex
	//Apply animation transormations for each vertex
	if (!m_weightsCount.empty())
	{
		//get animations that are need to be played
		std::vector<sAnimation> animsToPlay;
		for (unsigned int i = 0; i < m_animations.size(); ++i)
		{
			if (m_animations[i].id == animationToPlay)
			{
				AddAllChildren(m_animations, i, animsToPlay);
				break;
			}
		}
		//copy all matrices
		std::vector<float> jointMatrices;
		for (unsigned int i = 0; i < m_skeleton.size(); ++i)
		{
			for (unsigned int j = 0; j < 16; ++j)
			{
				jointMatrices.push_back(m_skeleton[i].matrix[j]);
			}
		}
		//replace affected joints with animation matrices
		float t = ((double)time) / 1000.0f;
		for (unsigned int i = 0; i < animsToPlay.size(); ++i)
		{
			unsigned int k;
			for (k = 0; k < animsToPlay[i].keyframes.size(); ++k)
			{
				if (t <= animsToPlay[i].keyframes[k] && (k == 0 || t > animsToPlay[i].keyframes[k - 1]))
				{
					break;
				}
			}
			if (k < animsToPlay[i].keyframes.size())
			{
				for (unsigned int j = 0; j < 16; ++j)
				{
					jointMatrices[animsToPlay[i].boneIndex * 16 + j] = animsToPlay[i].matrices[k * 16 + j];
				}
			}
		}
		//cycle through all joints and multiply them to their parents
		for (unsigned int i = 0; i < m_skeleton.size(); ++i)
		{
			if (m_skeleton[i].parentIndex != -1)
			{
				float parent[16];
				memcpy(parent, &jointMatrices[m_skeleton[i].parentIndex * 16], sizeof(float) * 16);
				MultiplyMatrices(parent, &jointMatrices[i * 16]);
				memcpy(&jointMatrices[i * 16], parent, sizeof(float) * 16);
			}
		}
		for (unsigned int i = 0; i < m_vertices.size(); ++i)
		{
			CVector3f v(0.0f, 0.0f, 0.0f);
			CVector3f old = m_vertices[i];
			//for each animation that has influence on this vertex perform an action;
			for (unsigned int j = 0; j < m_weightsCount[i]; ++j, ++k)
			{
				CVector3f cur = m_vertices[i];
				sJoint * joint = &m_skeleton[m_weightsIndexes[k]];
				MultiplyVectorToMatrix(cur, joint->bindShapeMatrix);
				MultiplyVectorToMatrix(cur, joint->invBindMatrix);
				MultiplyVectorToMatrix(cur, &jointMatrices[m_weightsIndexes[k] * 16]);
				cur *= m_weights[k];
				v += cur;
			}
			vertices.push_back(v);
		}
	}
	else
	{
		if (vertexOnly && m_vertexLists.find(*hideMeshes) == m_vertexLists.end())
		{
			NewList(m_vertexLists[*hideMeshes], hideMeshes, true);
		}
		if (!vertexOnly && m_lists.find(*hideMeshes) == m_lists.end())
		{
			NewList(m_lists[*hideMeshes], hideMeshes, false);
		}
		if (vertexOnly)
		{
			glCallList(m_vertexLists[*hideMeshes]);
		}
		else
		{
			glCallList(m_lists[*hideMeshes]);
		}
		return;
	}
	if (!vertices.empty())
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, !vertices.empty()?&vertices[0]:&m_vertices[0]);
	}
	if (!m_normals.empty() && !vertexOnly)
	{
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, &m_normals[0]);
	}
	if (!m_textureCoords.empty() && !vertexOnly)
	{
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, &m_textureCoords[0]);
	}
	glPushMatrix();
	glScaled(m_scale, m_scale, m_scale);
	if (!m_indexes.empty()) //Draw by meshes;
	{
		unsigned int begin = 0;
		unsigned int end;
		for (unsigned int i = 0; i < m_meshes.size(); ++i)
		{
			if (hideMeshes && hideMeshes->find(m_meshes[i].name) != hideMeshes->end())
			{
				end = m_meshes[i].polygonIndex;
				glDrawElements(GL_TRIANGLES, end - begin, GL_UNSIGNED_INT, &m_indexes[begin]);
				SetMaterial(m_materials.GetMaterial(m_meshes[i].materialName));
				begin = (i + 1 == m_meshes.size()) ? m_count : m_meshes[i + 1].polygonIndex;
				continue;
			}
			if (vertexOnly || (i > 0 && m_meshes[i].materialName == m_meshes[i - 1].materialName))
			{
				continue;
			}
			end = m_meshes[i].polygonIndex;
			glDrawElements(GL_TRIANGLES, end - begin, GL_UNSIGNED_INT, &m_indexes[begin]);
			if (!vertexOnly) SetMaterial(m_materials.GetMaterial(m_meshes[i].materialName));
			begin = end;
		}
		end = m_count;
		if (begin != end)
		{
			glDrawElements(GL_TRIANGLES, end - begin, GL_UNSIGNED_INT, &m_indexes[begin]);
		}
	}
	else //Draw in a row
	{
		glDrawArrays(GL_TRIANGLES, 0, m_count);
	}
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	sMaterial empty;
	SetMaterial(&empty);
	glPopMatrix();
}

void C3DModel::Preload() const
{
	CTextureManager * texManager = CTextureManager::GetInstance();
	for (unsigned int i = 0; i < m_meshes.size(); ++i)
	{
		if (!m_materials.GetMaterial(m_meshes[i].materialName)) continue;
		texManager->SetTexture(m_materials.GetMaterial(m_meshes[i].materialName)->texture);
	}
}

std::vector<std::string> C3DModel::GetAnimations() const
{
	std::vector<std::string> result;
	for (unsigned int i = 0; i < m_animations.size(); ++i)
	{
		result.push_back(m_animations[i].id);
	}
	return result;
}