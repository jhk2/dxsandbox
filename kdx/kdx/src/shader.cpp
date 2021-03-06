#include "shader.h"
#include "dxbase.h"
#include <fstream>

/*static*/ void Shader::OutputShaderErrorMessage(ID3D10Blob **errorMessage)
{
	char *errors;
	unsigned long bufferSize;
	std::ofstream fout;

	errors = (char *) (*errorMessage)->GetBufferPointer();
	bufferSize = (*errorMessage)->GetBufferSize();

	FILE *file;
	fopen_s(&file, "shader_errors.txt", "w");
	fwrite(errors, sizeof(char), bufferSize - 1, file);
	fclose(file);

	(*errorMessage)->Release();
	
	DxBase::ThrowError(L"Error Compiling Shader");
}

Shader::Shader(ID3D11Device &device, LPCWSTR filename)
{
	// can't call pure virtual methods in constructor
}

bool Shader::init(ID3D11Device &device, LPCWSTR filename)
{
	HRESULT result;
	ID3D10Blob *errorMessage = 0;

	result = compileFromFile(filename, &errorMessage);

	if (FAILED(result)) {
		if (errorMessage) {
			OutputShaderErrorMessage(&errorMessage);
		} else {
			DxBase::ThrowError(L"Missing Shader File");
		}
		return false;
	} else {
		result = initShader(device);
		if (FAILED(result)) {
			DxBase::ThrowError(L"shader init failed");
			return false;
		}
	}
	return true;
	
	// WORKNOTE: access violation when I try to release this, so probably don't need to
	//errorMessage->Release();
}

VertexShader::VertexShader(ID3D11Device &device, LPCWSTR filename) : Shader(device, filename)
{
	if (!init(device, filename)) {
		exit(1);
	}
}

VertexShader::~VertexShader()
{
	shader_->Release();
	buffer_->Release();
}

HRESULT VertexShader::compileFromFile(LPCWSTR filename, ID3D10Blob **errorMessage)
{
	HRESULT test = D3DX11CompileFromFile(filename, NULL, NULL, "VertexMain", "vs_5_0", NULL, NULL, NULL, &buffer_, errorMessage, NULL);
	return test;
}

HRESULT VertexShader::initShader(ID3D11Device &device)
{
	return device.CreateVertexShader(buffer_->GetBufferPointer(), buffer_->GetBufferSize(), NULL, &shader_);
}

ID3D11InputLayout* VertexShader::setInputLayout(ID3D11Device &device, D3D11_INPUT_ELEMENT_DESC *desc, UINT numElements)
{
	ID3D11InputLayout *pLayout;
	HRESULT result = device.CreateInputLayout(desc, numElements, buffer_->GetBufferPointer(), buffer_->GetBufferSize(), &pLayout);
	return pLayout;
}

PixelShader::PixelShader(ID3D11Device &device, LPCWSTR filename) : Shader(device, filename)
{
	if (!init(device, filename)) {
		exit(1);
	}
}

PixelShader::~PixelShader()
{
	shader_->Release();
	buffer_->Release();
}

HRESULT PixelShader::compileFromFile(LPCWSTR filename, ID3D10Blob **errorMessage)
{
	return D3DX11CompileFromFile(filename, NULL, NULL, "PixelMain", "ps_5_0", NULL, NULL, NULL, &buffer_, errorMessage, NULL);
}

HRESULT PixelShader::initShader(ID3D11Device &device)
{
	return device.CreatePixelShader(buffer_->GetBufferPointer(), buffer_->GetBufferSize(), NULL, &shader_);
}

ComputeShader::ComputeShader(ID3D11Device &device, LPCWSTR filename) : Shader(device, filename)
{
	if (!init(device, filename)) {
		exit(1);
	}
}

ComputeShader::~ComputeShader()
{
	shader_->Release();
	buffer_->Release();
}

HRESULT ComputeShader::compileFromFile(LPCWSTR filename, ID3D10Blob **errorMessage)
{
	return D3DX11CompileFromFile(filename, NULL, NULL, "ComputeMain", "cs_5_0", NULL, NULL, NULL, &buffer_, errorMessage, NULL);
}

HRESULT ComputeShader::initShader(ID3D11Device &device)
{
	return device.CreateComputeShader(buffer_->GetBufferPointer(), buffer_->GetBufferSize(), NULL, &shader_);
}