#ifndef MESH_H
#define MESH_H

#include "dxbase.h"
#include <vector>

// use template specialization to get enums corresponding to index buffer types
// used for calls to IASetIndexBuffer
template<typename T> struct IndexTypeToEnum {};
template<> struct IndexTypeToEnum<UINT8> { enum { value = DXGI_FORMAT_R8_UINT }; };
template<> struct IndexTypeToEnum<UINT16> { enum { value = DXGI_FORMAT_R16_UINT }; };
template<> struct IndexTypeToEnum<UINT32> { enum { value = DXGI_FORMAT_R32_UINT }; };

template<typename IND_TYPE>
class Mesh {
public:
	Mesh(D3D11_PRIMITIVE_TOPOLOGY topology) : topology_(topology), indexcount_(0), inds_() {}
	virtual ~Mesh() { indexbuffer_->Release(); }

	Mesh& addInd(const IND_TYPE &newind)
	{
		inds_.push_back(newind);
		return *this;
	}

	Mesh& addInds(const std::vector<IND_TYPE> &newinds)
	{
		inds_.insert(inds_.end(), newinds.begin(), newinds.end());
		return *this;
	}

	void finalize(ID3D11Device &dev)
	{
		finalizeVertices(dev);
		indexcount_ = inds_.size();
		// generate the index buffer object
		D3D11_BUFFER_DESC ibufdesc;
		ZeroMemory(&ibufdesc, sizeof(D3D11_BUFFER_DESC));

		ibufdesc.Usage = D3D11_USAGE_DEFAULT;
		ibufdesc.ByteWidth = sizeof(IND_TYPE) * indexcount_;
		ibufdesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibufdesc.CPUAccessFlags = 0;
		ibufdesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA subr;
		ZeroMemory(&subr, sizeof(D3D11_SUBRESOURCE_DATA));

		subr.pSysMem = &inds_[0];
		dev.CreateBuffer(&ibufdesc, &subr, &indexbuffer_); // WORKQUESTION: better to do this with initial data in createbuffer or use map+memcp?
	}

	virtual void draw(ID3D11Device &dev, ID3D11DeviceContext &devcon)
	{
		setVertexBuffers(devcon);
		devcon.IASetIndexBuffer(indexbuffer_, (DXGI_FORMAT) IndexTypeToEnum<IND_TYPE>::value, 0);
		devcon.IASetPrimitiveTopology(topology_);
		devcon.DrawIndexed(indexcount_, 0, 0);
	}
protected:
	virtual void finalizeVertices(ID3D11Device &dev) = 0;
	inline virtual void setVertexBuffers(ID3D11DeviceContext &devcon) = 0;

	ID3D11Buffer *indexbuffer_;
	UINT indexcount_;
	D3D11_PRIMITIVE_TOPOLOGY topology_;
	std::vector<IND_TYPE> inds_;
};

template<class VERT_TYPE, typename IND_TYPE>
class InterleavedMesh : public Mesh<IND_TYPE>
{
public:
	InterleavedMesh(D3D11_PRIMITIVE_TOPOLOGY topology) : Mesh<IND_TYPE>(topology) {}
	virtual ~InterleavedMesh() { vertexbuffer_->Release(); }

	InterleavedMesh& addVert(const VERT_TYPE &newvert)
	{
		verts_.push_back(newvert);
		return *this;
	}

	InterleavedMesh& addVerts(const std::vector<VERT_TYPE> &newverts)
	{
		verts_.insert(verts_.end(), newverts.begin(), newverts.end());
		return *this;
	}

private:
	void finalizeVertices(ID3D11Device &dev)
	{
		D3D11_BUFFER_DESC vbufdesc;
		ZeroMemory(&vbufdesc, sizeof(D3D11_BUFFER_DESC));

		vbufdesc.Usage = D3D11_USAGE_DEFAULT;
		vbufdesc.ByteWidth = sizeof(VERT_TYPE) * verts_.size();
		vbufdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbufdesc.CPUAccessFlags = 0;
		vbufdesc.MiscFlags = 0;
		
		D3D11_SUBRESOURCE_DATA subr;
		ZeroMemory(&subr, sizeof(D3D11_SUBRESOURCE_DATA));

		subr.pSysMem = &verts_[0];
		dev.CreateBuffer(&vbufdesc, &subr, &vertexbuffer_);
	}

	inline void setVertexBuffers(ID3D11DeviceContext &devcon)
	{
		UINT stride = sizeof(VERT_TYPE);
		UINT offset = 0;
		devcon.IASetVertexBuffers(0, 1, &vertexbuffer_, &stride, &offset);
	}

	ID3D11Buffer *vertexbuffer_; // one vertex buffer for interleaved vertices
	std::vector<VERT_TYPE> verts_;
};

#endif // MESH_H