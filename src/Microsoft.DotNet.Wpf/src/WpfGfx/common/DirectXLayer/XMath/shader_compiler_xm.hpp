// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

#include "dxlcommon.hpp"
#include "shader_compiler_t.hpp"

#include <d3d9.h>
#include <D3Dcompiler.h>
#include <memory>

#include <wrl/client.h>

#include "wgx_error.h"

namespace dxlayer
{
    // An implementation of ID3DBlob 
    // for use in shader compilation operations
    class buffer_xm : public buffer, public ID3DBlob
    {
    private:
        Microsoft::WRL::ComPtr<ID3DBlob> m_blob;

    public:
        // constructor
        inline buffer_xm(ID3DBlob* blob) : m_blob(blob) {}

        // Inherited via buffer
        virtual data get_buffer_data() override
        {
            return{ GetBufferPointer(), static_cast<int64_t>(GetBufferSize()) };
        }

#pragma region Inherited via ID3DBlob

        virtual LPVOID __stdcall GetBufferPointer() override
        {
            return m_blob->GetBufferPointer();
        }

        virtual SIZE_T __stdcall GetBufferSize() override
        {
            return m_blob->GetBufferSize();
        }

        virtual HRESULT __stdcall QueryInterface(REFIID riid, void ** ppvObject) override
        {
            return m_blob.Get()->QueryInterface(riid, ppvObject);
        }

        virtual ULONG __stdcall AddRef(void) override
        {
            return m_blob.Get()->AddRef();
        }

        virtual ULONG __stdcall Release(void) override
        {
            return m_blob.Get()->Release();
        }

#pragma endregion // Inherited via ID3DBlob
    };

    // Implements shader_t in terms of shader API's exposed 
    // by D3DCompiler_*.dll
    template<>
    class shader_t<dxapi::xmath>
    {
    public:
        // Compile a shader file using D3DCompile
        static inline HRESULT compile(
            std::string src_data,
            std::string entry_point_name,
            std::string shader_profile_target,
            unsigned long flags1, unsigned long flags2,
            std::shared_ptr<buffer>& shader,
            std::shared_ptr<buffer>& err_msgs)
        {
            ID3DBlob* pShader = nullptr;
            ID3DBlob* pErrorMsgs = nullptr;

            auto hResult =
                D3DCompile(
                    src_data.c_str(), src_data.size() * sizeof(std::string::value_type),
                    nullptr,
                    nullptr,
                    nullptr,
                    entry_point_name.c_str(),
                    shader_profile_target.c_str(),
                    flags1, flags2,
                    &pShader, &pErrorMsgs);

            if (SUCCEEDED(hResult))
            {
                shader.reset(new buffer_xm(pShader));

                err_msgs.reset();
                err_msgs = nullptr;
            }
            else
            {
                shader.reset();
                shader = nullptr;

                err_msgs.reset(new buffer_xm(pErrorMsgs));
            }

            return hResult;
        }

#pragma region get_pixel_shader_profile_name

        // Returns the name of the highest high-level shader language(HLSL) pixel-shader 
        // profile supported by a given device.
        template<typename ID3DDevice>
        static std::string get_pixel_shader_profile_name(ID3DDevice* pD3DDevice);

        // Specialization of get_pixel_shader_profile_name with 
        // ID3DDevice = D3DDeviceContext
        template<>
        inline static std::string get_pixel_shader_profile_name<D3DDeviceContext>(
            D3DDeviceContext* pD3DDevice)
        {
            // We will query D3DCAPS9 to identify the best supported
            // profile name. If that query fails, then the following 
            // default name will be used as a fallback. 
            const std::string default_profile_name = "ps_4_0_level_9_3";

            return default_profile_name;
        }

#pragma endregion

#pragma region get_vertex_shader_profile_name

        // Returns the name of the highest high-level shader language(HLSL) vertex-shader 
        // profile supported by a given device.
        template<typename ID3DDevice>
        static std::string get_vertex_shader_profile_name(ID3DDevice* pD3DDevice);

        // Specialization of get_vertex_shader_profile_name with 
        // ID3DDevice = D3DDeviceContext
        template<>
        static std::string get_vertex_shader_profile_name<D3DDeviceContext>(
            D3DDeviceContext* pD3DDevice)
        {
            const std::string default_profile_name = "vs_4_0_level_9_3";

            return default_profile_name;
        }

#pragma endregion

        // Changes an error HRESULT to the more descriptive 
        // WGXERR_SHADER_COMPILE_FAILED if appropriate, and 
        // outputs the compiler errors 
#pragma warning(push)
#pragma warning(disable: 4100) // err_msgs is unreferenced in retail/fre builds.
        static HRESULT handle_errors_and_transform_hresult(
            HRESULT hResult, 
            const std::shared_ptr<buffer>& err_msgs)
#pragma warning(pop)
        {
            HRESULT retval = hResult;

            if ((hResult == D3DERR_INVALIDCALL) ||
                (hResult == DXGI_ERROR_INVALID_CALL) ||
                (hResult == E_FAIL))
            {
                retval = WGXERR_SHADER_COMPILE_FAILED;
            }

#if (defined(DBG) || defined(DEBUG) || defined(_DEBUG)) && !defined(TESTUSE_NOTRACETAG)

            //
            // Output compiler errors
            //

            auto szErrors =
                static_cast<char const*>(err_msgs->get_buffer_data().buffer);
            TraceTag((tagError, "MIL-HW: Vertex Shader compiler errors:\n%s", szErrors));

#endif // (defined(DBG) || defined(DEBUG) || defined(_DEBUG)) && !defined(TESTUSE_NOTRACETAG)
            return hResult;
            }

        };
    }

