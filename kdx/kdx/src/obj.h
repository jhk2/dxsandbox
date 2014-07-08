#ifndef OBJ_H
#define OBJ_H
#include "mesh.hpp"
#include "texture.h"

// class for representing OBJ models
class Obj {
public:
	Obj(ID3D11Device &dev, ID3D11DeviceContext &devcon, LPCWSTR filename); // allowing wide characters for non-english filenames
	virtual ~Obj();

	void draw(ID3D11Device &dev, ID3D11DeviceContext &devcon);

private:
	// bounding box
	fl3 min_, max_;

	// struct for materials
	struct ObjMaterial {
		virtual ~ObjMaterial() { materialbuffer->Release(); }
		struct ObjConstantBuffer {
			FLOAT Ns; // specular coefficient
			FLOAT Ni; // index of refraction
			union {
				FLOAT d, Tr; // transparency (can have both notations)
			};
			FLOAT padding0;
			fl3 Tf; // transmission filter (allows only certain colors through)
			

			UINT32 illum; // illumination model
			// 0 means constant illumination (color = Kd)
			// 1 means lambertian model (diffuse and ambient only)
			// 2 means lambert + blinn-phong (diffuse, specular, and ambient)
			// there's more at http://en.wikipedia.org/wiki/Wavefront_.obj_file
			// but these are the basics

			// we need padding to make vectors not straddle 16 byte boundaries
			// http://msdn.microsoft.com/en-us/library/windows/desktop/bb509632%28v=vs.85%29.aspx
			fl3 Ka; // ambient color
			FLOAT padding1;
			fl3 Kd; // diffuse color
			FLOAT padding2;
			fl3 Ks; // specular color
			FLOAT padding3;
			fl3 Ke; // emissive color
			FLOAT padding4;
		} cbuffer;
		std::wstring name;

		// eventually want textures here
		Texture *map_Ka;
		Texture *map_Kd;
		Texture *map_Ks;
		// WORKNOTE: might want to redo this to make resource and sampler arrays so we can bind all at once

		// also want handles for whatever d3d structures (buffers, etc)
		ID3D11Buffer *materialbuffer;
	};

	// container for the mesh/geometry data itself
	// one Obj can have multiple meshes
	typedef Mesh<UINT32> ObjMesh;

	bool loadFile(ID3D11Device &dev, ID3D11DeviceContext &devcon, LPCWSTR filename);
	bool loadMaterials(ID3D11Device &dev, ID3D11DeviceContext &devcon, LPCWSTR filename);

	// functions to create a mesh out of the mesh-specific parts of the file
	ObjMesh* createPTNMesh(ID3D11Device &dev, ID3D11DeviceContext &devcon, FILE *file);
	ObjMesh* createPTMesh(ID3D11Device &dev, ID3D11DeviceContext &devcon, FILE *file);
	ObjMesh* createPNMesh(ID3D11Device &dev, ID3D11DeviceContext &devcon, FILE *file);
	ObjMesh* createPMesh(ID3D11Device &dev, ID3D11DeviceContext &devcon, FILE *file);
	
	// intermediate vectors for storing vertices, texcoords, normals
	std::vector<fl3> verts_;
	std::vector<fl3> texs_;
	std::vector<fl3> norms_;
	// faces/indices
	std::vector<UINT32> inds_;
	
	// temporary variables for parsing materials
	std::map<std::wstring, bool> mtlfiles_;
	// textures can be shared for multiple materials, so a map to keep track of them
	std::map<std::wstring, Texture *> textures_;
	// temporary map of which v/t/n combos have been assigned to which index
	std::map<int3, UINT32> combos_;
	UINT32 currentcombo_;
		
	// map of materials by addressable name
	std::map<std::wstring, ObjMaterial *> materials_;
	// uniform buffers for these materials
		
	// map for storing the final meshes and associated materials
	std::map<std::wstring, std::pair<ObjMesh *, ObjMaterial *>> meshes_;
};
#endif // OBJ_H