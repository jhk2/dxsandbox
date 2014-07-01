#ifndef SHADER_H
#define SHADER_H
#include <Windows.h>

struct ID3D10Blob;
struct ID3D11Device;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct D3D11_INPUT_ELEMENT_DESC;

class Shader {
public:
	enum SHADER_TYPES {
		VERTEX_SHADER = 0x01,
		PIXEL_SHADER = 0x02,
		GEOMETRY_SHADER = 0x04,
		TESSELLATION_SHADER = 0x08,
		COMPUTE_SHADER = 0x10
	};
private:
	static void OutputShaderErrorMessage(ID3D10Blob **errorMessage);
public:
	Shader(ID3D11Device &device, LPCWSTR filename);
private:
	virtual HRESULT compileFromFile(LPCWSTR filename, ID3D10Blob **errorMessage) = 0;
	virtual HRESULT initShader(ID3D11Device &device) = 0;
protected:
	void init(ID3D11Device &device, LPCWSTR filename);
	ID3D10Blob *buffer_;
};

class VertexShader : public Shader {
public:
	VertexShader(ID3D11Device &device, LPCWSTR filename);
	virtual ~VertexShader();
	ID3D11VertexShader *get() { return shader_; }
	ID3D11InputLayout* setInputLayout(ID3D11Device &device, D3D11_INPUT_ELEMENT_DESC *desc, UINT numElements);
private:
	virtual HRESULT compileFromFile(LPCWSTR filename, ID3D10Blob **errorMessage);
	virtual HRESULT initShader(ID3D11Device &device);
	ID3D11VertexShader *shader_;
};

class PixelShader : public Shader {
public:
	PixelShader(ID3D11Device &device, LPCWSTR filename);
	virtual ~PixelShader();
	ID3D11PixelShader *get() { return shader_; }
private:
	virtual HRESULT compileFromFile(LPCWSTR filename, ID3D10Blob **errorMessage);
	virtual HRESULT initShader(ID3D11Device &device);
	ID3D11PixelShader *shader_;
};
#endif // SHADER_H