#include "obj.h"
#include <assert.h>
#include <string>

Obj::Obj(ID3D11Device &dev, ID3D11DeviceContext &devcon, LPCWSTR filename) : currentcombo_(0), min_(), max_()
{
	loadFile(dev, devcon, filename);
}

Obj::~Obj()
{
	for (std::map<std::wstring, std::pair<ObjMesh *, ObjMaterial *>>::iterator iter = meshes_.begin(); iter != meshes_.end(); iter++) {
		delete iter->second.first;
	}
	for (std::map<std::wstring, ObjMaterial *>::iterator iter = materials_.begin(); iter != materials_.end(); iter++) {
		delete iter->second;
	}
	for (std::map<std::wstring, Texture *>::iterator iter = textures_.begin(); iter != textures_.end(); iter++) {
		delete iter->second;
	}
}

void Obj::draw(ID3D11Device &dev, ID3D11DeviceContext &devcon)
{
	 for (std::map<std::wstring, std::pair<ObjMesh *, ObjMaterial *>>::const_iterator iter = meshes_.begin(); iter != meshes_.end(); iter++) {
		 ObjMaterial &curmat = *(iter->second.second);
		 // set constant buffers eventually

		 // also set textures/samplers

		 // draw the actual mesh
		 ObjMesh &curmesh = *(iter->second.first);
		 curmesh.draw(dev, devcon);
	 }
}


bool Obj::loadFile(ID3D11Device &dev, ID3D11DeviceContext &devcon, LPCWSTR filename)
{
    //printf("trying to load obj file %s\n", filename); fflush(stdout);
	FILE *pFile = 0;
	_wfopen_s(&pFile, filename, L"r");
	
	if(!pFile) {
		std::wstring message (L"failed to open obj file ");
		message += filename;
		DxBase::ThrowError(message.c_str());
		return false;
	}
	
	wchar_t line[1024] = L"";
	wchar_t buf[512] = L"";
	ObjMaterial *currentmat = 0;
	std::wstring meshname = L"";
	
	// map for storing v/t/n triples versus index buffer
	std::map<int3, unsigned int> triples;
	fpos_t lastread;
	fgetpos(pFile, &lastread);
	while (fwscanf_s(pFile, L"%s", buf, _countof(buf)) > 0) {
		if (wcscmp(buf, L"mtllib") == 0) {
			// need to load the material file if we have not already
			fwscanf_s(pFile, L"%s", buf, _countof(buf));
			std::wstring mtlfile (buf);
			if (mtlfiles_.find(mtlfile) == mtlfiles_.end()) {
				// add the path to the obj file to the name
				std::wstring fullpath (filename);
				size_t slashindex = fullpath.find_last_of(L"/");
				if (slashindex != std::wstring::npos) {
					mtlfile = fullpath.substr(0, slashindex + 1) + (mtlfile);
				}
				// insert a new material
				loadMaterials(dev, devcon, mtlfile.c_str());
			} else {
				printf("found undefined mtlfile %s, which is bad, but continuing\n", buf); fflush(stdout);
			}
		} else if (wcscmp(buf, L"usemtl") == 0) {
			fwscanf_s(pFile, L"%s", buf, _countof(buf));
			std::wstring mtlfile (buf);
			currentmat = materials_[mtlfile];
			assert(currentmat != 0);
		} else if (wcscmp(buf, L"g") == 0) {
			fwscanf_s(pFile, L"%s", buf, _countof(buf));
            //printf("loading submesh %s\n", buf); fflush(stdout);
			meshname = buf;
		} else if (wcscmp(buf, L"v") == 0) {
			fl3 v;
			fwscanf_s(pFile, L"%f %f %f", &v.x, &v.y, &v.z);
			// WORKNOTE: to account for RH mesh -> LH app, invert z axis
			v.z = -v.z;
			// update min/max
			min_.x = min(min_.x, v.x);
			min_.y = min(min_.y, v.y);
			min_.z = min(min_.z, v.z);
			max_.x = max(max_.x, v.x);
			max_.y = max(max_.y, v.y);
			max_.z = max(max_.z, v.z);
			verts_.push_back(v);
		} else if (wcscmp(buf, L"vt") == 0) {
			fl3 t;
			fwscanf_s(pFile, L"%f %f %f", &t.x, &t.y, &t.z);
			// WORKNOTE: to account for 0,0 being bottom left on GL but top left on DX, invert v texcoord
			t.y = 1.0f - t.y;
			texs_.push_back(t);
		} else if (wcscmp(buf, L"vn") == 0) {
			fl3 n;
			fwscanf_s(pFile, L"%f %f %f", &n.x, &n.y, &n.z);
			// WORKNOTE: to account for RH mesh -> LH app, invert z axis
			n.z = -n.z;
			norms_.push_back(n);
		} else if (wcscmp(buf, L"f") == 0) {
			// move it back to before the f
			fsetpos(pFile, &lastread);
			// we make the assumption that all faces for a particular mesh are consecutively located in the file
			// get the face indices
			ObjMesh *currentmesh = 0;
			if (texs_.size() == 0 && norms_.size() == 0) {
				// we only have positions
                //printf("create new PMesh\n"); fflush(stdout);
				currentmesh = createPMesh(dev, devcon, pFile);
			} else if (texs_.size() == 0) {
				// we have positions and normals
				//printf("we've read %u verts and %u normals\n", verts_.size(), norms_.size()); fflush(stdout);
                //printf("create new PNMesh\n"); fflush(stdout);
				currentmesh = createPNMesh(dev, devcon, pFile);
			} else if (norms_.size() == 0) {
				// we have positions and texcoords
                //printf("create new PTMesh\n"); fflush(stdout);
				currentmesh = createPTMesh(dev, devcon, pFile);
			} else {
				// we have all 3
                //printf("create new PTNMesh\n"); fflush(stdout);
				currentmesh = createPTNMesh(dev, devcon, pFile);
			}
			// put it in the map
			if(meshname.length() == 0) {
				meshname = L"default";
			}
			if(meshes_.find(meshname) == meshes_.end()) {
                //printf("inserting new mesh with name %s\n", meshname.c_str()); fflush(stdout);
				meshes_[meshname] = std::pair<ObjMesh*,ObjMaterial*>(currentmesh, currentmat);
			} else {
				std::wstring message (L"tried to insert mesh which already existed with name ");
				message += meshname;
				DxBase::ThrowError(message.c_str());
			}
		}
		fgetpos(pFile, &lastread);
	}
	// clear temporary stuff
	verts_.clear();
	texs_.clear();
	norms_.clear();
	inds_.clear();
	mtlfiles_.clear();
	combos_.clear();
	return true;
}

bool Obj::loadMaterials(ID3D11Device &dev, ID3D11DeviceContext &devcon, LPCWSTR filename)
{
    //printf("trying to load mtl file %s\n", filename); fflush(stdout);
	FILE *pFile = 0;
	_wfopen_s(&pFile, filename, L"r");
	if(!pFile) {
		std::wstring message (L"couldn't open mtlfile ");
		message += filename;
		DxBase::ThrowError(message.c_str());
		return false;
	}
	
	// get the directory
	std::wstring fullpath (filename);
	size_t slashindex = fullpath.find_last_of(L"/");
	std::wstring directory = L"";
	if (slashindex != std::wstring::npos) {
		directory = fullpath.substr(0, slashindex + 1);
	}
	
	ObjMaterial *currentmat = 0;
	
	wchar_t buf[512] = L"";	

	while (fwscanf_s(pFile, L"%s", buf, _countof(buf)) > 0) {
		//printf("buf contents %s\n", buf); fflush(stdout);
		if (wcscmp(buf, L"newmtl") == 0) {
			fwscanf_s(pFile, L"%s", buf, _countof(buf));
			ObjMaterial *newmat = new ObjMaterial();
			newmat->name = std::wstring(buf);
			materials_[newmat->name] = newmat;
			currentmat = newmat;
		} else if (wcscmp(buf, L"Ns") == 0) {
			float val;
			fwscanf_s(pFile, L"%f", &val);
			currentmat->cbuffer.Ns = val;
		} else if (wcscmp(buf, L"Ni") == 0) {
			float val;
			fwscanf_s(pFile, L"%f", &val);
			currentmat->cbuffer.Ni = val;
		} else if (wcscmp(buf, L"d") == 0 || wcscmp(buf, L"Tr") == 0) {
			float val;
			fwscanf_s(pFile, L"%f", &val);
			// for d/tf, might have another value already so take the max
			currentmat->cbuffer.d = max(currentmat->cbuffer.d, val);
		} else if (wcscmp(buf, L"illum") == 0) {
			unsigned int val;
			fwscanf_s(pFile, L"%u", &val);
			currentmat->cbuffer.illum = val;
		} else if (wcscmp(buf, L"Ka") == 0) {
			fl3 val;
			fwscanf_s(pFile, L"%f %f %f", &val.x, &val.y, &val.z);
			currentmat->cbuffer.Ka = val;
		} else if (wcscmp(buf, L"Kd") == 0) {
			fl3 val;
			fwscanf_s(pFile, L"%f %f %f", &val.x, &val.y, &val.z);
			currentmat->cbuffer.Kd = val;
		} else if (wcscmp(buf, L"Ks") == 0) {
			fl3 val;
			fwscanf_s(pFile, L"%f %f %f", &val.x, &val.y, &val.z);
			currentmat->cbuffer.Ks = val;
		} else if (wcscmp(buf, L"Ke") == 0) {
			fl3 val;
			fwscanf_s(pFile, L"%f %f %f", &val.x, &val.y, &val.z);
			currentmat->cbuffer.Ke = val;
		} else if (wcscmp(buf, L"map_Ka") == 0) {
			fwscanf_s(pFile, L"%s", buf, _countof(buf));
			std::wstring texpath (buf);
			std::wstring fulltexpath = directory + texpath;
			if (textures_.find(texpath) == textures_.end()) {
				// insert a new texture
				textures_[texpath] = new Texture(dev, devcon, fulltexpath.c_str());
			}
			currentmat->map_Ka = textures_[texpath];
		} else if (wcscmp(buf, L"map_Kd") == 0) {
			fwscanf_s(pFile, L"%s", buf, _countof(buf));
			std::wstring texpath (buf);
			std::wstring fulltexpath = directory + buf;
			if (textures_.find(texpath) == textures_.end()) {
				// insert a new texture
				textures_[texpath] = new Texture(dev, devcon, fulltexpath.c_str());
			}
			currentmat->map_Kd = textures_[texpath];
		} else if (wcscmp(buf, L"map_Ks") == 0) {
			fwscanf_s(pFile, L"%s", buf, _countof(buf));
			std::wstring texpath (buf);
			std::wstring fulltexpath = directory + buf;
			if (textures_.find(texpath) == textures_.end()) {
				// insert a new texture
				textures_[texpath] = new Texture(dev, devcon, fulltexpath.c_str());
			}
			currentmat->map_Ks = textures_[texpath];
		}
	}
	
	// common buffer description
	D3D11_BUFFER_DESC bufferdesc;
	ZeroMemory(&bufferdesc, sizeof(D3D11_BUFFER_DESC));
	bufferdesc.Usage = D3D11_USAGE_DEFAULT;
	bufferdesc.ByteWidth = sizeof(ObjMaterial);
	bufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferdesc.CPUAccessFlags = 0; // WORKNOTE: if we use map+memcpy, this needs to be CPU_ACCESS_WRITE
	bufferdesc.MiscFlags = 0;
	bufferdesc.StructureByteStride = 0;

	for (std::map<std::wstring, ObjMaterial *>::iterator it = materials_.begin(); it != materials_.end(); it++) {
		ObjMaterial *mat = it->second;
		// make a constant buffer for these material properties
		D3D11_SUBRESOURCE_DATA subr;
		ZeroMemory(&subr, sizeof(D3D11_SUBRESOURCE_DATA));
		subr.pSysMem = &mat->cbuffer;

		// TODO: THIS IS FAILING CURRENTLY
		HRESULT result = dev.CreateBuffer(&bufferdesc, &subr, &mat->materialbuffer);
		if (FAILED(result)) {
			return false;
		}
	}
	return true;
}

Obj::ObjMesh* Obj::createPTNMesh(ID3D11Device &dev, ID3D11DeviceContext &devcon, FILE *file)
{
	//~ printf("creating ptn mesh\n"); fflush(stdout);
	unsigned int read = 0;
	InterleavedMesh<PTNvert, UINT32> *mesh = new InterleavedMesh<PTNvert, UINT32>(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	currentcombo_ = 0;
	combos_.clear();
	// 3 vertex attributes with 3 floats each
	fpos_t lastpos;
	wchar_t buf[512] = L"";
	fgetpos(file, &lastpos);
	while (fwscanf_s(file, L"%s", buf, _countof(buf)) > 0) {
		if (wcscmp(buf, L"f") != 0) {
			// we hit something other than "f"
			//~ printf("hit a non f character %s after %u faces\n", buf, read); fflush(stdout);
			fsetpos(file, &lastpos);
			break;
		} else {
			// read the face
			int3 v[3];
			fwscanf_s(file, L"%u/%u/%u %u/%u/%u %u/%u/%u", &v[0].x, &v[0].y, &v[0].z, &v[1].x, &v[1].y, &v[1].z, &v[2].x, &v[2].y, &v[2].z);
			// check if each of them is contained
			PTNvert vert;
			for (int i = 0; i < 3; i++) {
				int3 &cur = v[i];
				if (combos_.find(cur) == combos_.end()) {
					//~ printf("making new vertex entry for combo %u/%u/%u index %u\n", cur.x, cur.y, cur.z, currentcombo_); fflush(stdout);
					combos_[cur] = currentcombo_;
					//inds_.push_back(currentcombo_);
					mesh->addInd(currentcombo_);
					currentcombo_++;
					vert.pos_ = verts_[cur.x - 1];
					vert.tex_ = texs_[cur.y - 1];
					vert.norm_ = norms_[cur.z - 1];
					//~ printf("adding new vertex combo index %u pos %f,%f,%f (%u) tex %f,%f,%f (%u)\n", currentcombo_, vert.pos_.x, vert.pos_.y, vert.pos_.z, cur.x-1, vert.tex_.x, vert.tex_.y, vert.tex_.z, cur.y-1); fflush(stdout);
					mesh->addVert(vert);
				} else {
					//~ printf("found existing entry for combo %u/%u/%u index %u\n", cur.x, cur.y, cur.z, combos_[cur]); fflush(stdout);
					//inds_.push_back(combos_[cur]);
					mesh->addInd(combos_[cur]);
				}
			}
			read++;
		}
		fgetpos(file, &lastpos);
	}
	mesh->finalize(dev);
	// we've read all the faces, so make the mesh
	return mesh;
}

Obj::ObjMesh* Obj::createPTMesh(ID3D11Device &dev, ID3D11DeviceContext &devcon, FILE *file)
{
	unsigned int read = 0;
	currentcombo_ = 0;
	combos_.clear();
	InterleavedMesh<PTvert, UINT32> *mesh = new InterleavedMesh<PTvert, UINT32>(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 2 vertex attributes with 3 floats each
	fpos_t lastpos;
	wchar_t buf[512] = L"";
	fgetpos(file, &lastpos);
	while (fwscanf_s(file, L"%s", buf, _countof(buf)) > 0) {
		//~ printf("buf contents: %s\n", buf); fflush(stdout);
		if (wcscmp(buf, L"f") != 0) {
			// we hit something other than "f"
			//~ printf("hit something other than f: %s\n", buf); fflush(stdout);
			fsetpos(file, &lastpos);
			break;
		} else {
			// read the face
			int3 v[3];
			unsigned int scanned = fwscanf_s(file, L"%u/%u %u/%u %u/%u", &v[0].x, &v[0].y, &v[1].x, &v[1].y, &v[2].x, &v[2].y);
			//~ if(scanned != 6) {
				//~ printf("scanned less than 6 items: %u/%u %u/%u %u/%u\n", v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y);
				//~ fflush(stdout);
			//~ }
			// check if each of them is contained
			PTvert vert;
			for (int i = 0; i < 3; i++) {
				int3 &cur = v[i];
				if (combos_.find(cur) == combos_.end()) {
					combos_[cur] = currentcombo_;
					//inds_.push_back(currentcombo_);
					mesh->addInd(currentcombo_);
					currentcombo_++;
					vert.pos_ = verts_[cur.x - 1];
					vert.tex_ = texs_[cur.y - 1];
					mesh->addVert(vert);
				} else {
					//inds_.push_back(combos_[cur]);
					mesh->addInd(combos_[cur]);
				}
			}
			read++;
		}
		fgetpos(file, &lastpos);
	}
	//~ printf("read %u faces\n", read); fflush(stdout);
	mesh->finalize(dev);
	// we've read all the faces, so make the mesh
	return mesh;
}

Obj::ObjMesh* Obj::createPNMesh(ID3D11Device &dev, ID3D11DeviceContext &devcon, FILE *file)
{
	currentcombo_ = 0;
	combos_.clear();
    //printf("creating pn mesh\n"); fflush(stdout);
	InterleavedMesh<PNvert, UINT32> *mesh = new InterleavedMesh<PNvert, UINT32>(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 2 vertex attributes with 3 floats each
	fpos_t lastpos;
	wchar_t buf[512] = L"";
	fgetpos(file, &lastpos);
	unsigned int faces = 0;
	while (fwscanf_s(file, L"%s", buf, _countof(buf)) > 0) {
		if (wcscmp(buf, L"f") != 0) {
			// we hit something other than "f"
            //printf("hit something other than f, ending mesh\n"); fflush(stdout);
			fsetpos(file, &lastpos);
			break;
		} else {
			// read the face
			int3 v[3];
			fwscanf_s(file, L"%u//%u %u//%u %u//%u", &v[0].x, &v[0].y, &v[1].x, &v[1].y, &v[2].x, &v[2].y);
			// check if each of them is contained
			PNvert vert;
			for (int i = 0; i < 3; i++) {
				int3 &cur = v[i];
				if (combos_.find(cur) == combos_.end()) {
					combos_[cur] = currentcombo_;
					//inds_.push_back(currentcombo_);
					mesh->addInd(currentcombo_);
					currentcombo_++;
					vert.pos_ = verts_[cur.x - 1];
					vert.norm_ = norms_[cur.y - 1];
					mesh->addVert(vert);
				} else {
					//inds_.push_back(combos_[cur]);
					mesh->addInd(combos_[cur]);
				}
			}
			faces++;
		}
		fgetpos(file, &lastpos);
	}
    //printf("read %u faces\n", faces); fflush(stdout);
	mesh->finalize(dev);
	// we've read all the faces, so make the mesh
	return mesh;
}

Obj::ObjMesh* Obj::createPMesh(ID3D11Device &dev, ID3D11DeviceContext &devcon, FILE *file)
{
	currentcombo_ = 0;
	combos_.clear();
	InterleavedMesh<fl3, UINT32> *mesh = new InterleavedMesh<fl3, UINT32>(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 2 vertex attributes with 3 floats each
	fpos_t lastpos;
	wchar_t buf[512] = L"";
	fgetpos(file, &lastpos);
	while (fwscanf_s(file, L"%s", buf, _countof(buf)) > 0) {
		if (wcscmp(buf, L"f") != 0) {
			// we hit something other than "f"
			fsetpos(file, &lastpos);
			break;
		} else {
			// read the face
			int3 v[3];
			fwscanf_s(file, L"%u/%u %u/%u %u/%u", &v[0].x, &v[1].x, &v[2].x);
			// check if each of them is contained
			fl3 vert;
			for (int i = 0; i < 3; i++) {
				int3 &cur = v[i];
				if (combos_.find(cur) == combos_.end()) {
					combos_[cur] = currentcombo_;
					//inds_.push_back(currentcombo_);
					mesh->addInd(currentcombo_);
					currentcombo_++;
					vert = verts_[cur.x - 1];
					mesh->addVert(vert);
				} else {
					//inds_.push_back(combos_[cur]);
					mesh->addInd(combos_[cur]);
				}
			}
		}
		fgetpos(file, &lastpos);
	}
	mesh->finalize(dev);
	// we've read all the faces, so make the mesh
	return mesh;
}
