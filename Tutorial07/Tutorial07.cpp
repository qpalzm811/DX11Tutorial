//--------------------------------------------------------------------------------------
// File: Tutorial07.cpp
//
// This application demonstrates texturing
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include "resource.h"


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT2 Tex;
};

struct CBNeverChanges
{
    XMMATRIX mView;
};

struct CBChangeOnResize
{
    XMMATRIX mProjection;
};

struct CBChangesEveryFrame
{
    XMMATRIX mWorld;
    XMFLOAT4 vMeshColor;
};


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
//HWND是线程相关的，通过HWND找到该窗口所属进程和线程
//Handle 是代表系统的内核对象，如文件句柄，线程句柄，进程句柄。
//系统对内核对象以链表的形式进行管理，载入到内存中的每一个内
//核对象都有一个线性地址，同时相对系统来说，在串列中有一个索引位置，这个索引位置就是内核对象的handle。
//
//HINSTANCE的本质是模块基地址，他仅仅在同一进程中才有意义，跨进程的HINSTANCE是没有意义
//
//HMODULE 是代表应用程序载入的模块，win32系统下通常是被载入模块的线性地址。
//
//HINSTANCE 在win32下与HMODULE是相同的东西(只有在16位windows上，二者有所不同).
HINSTANCE                           g_hInst = NULL;
HWND                                g_hWnd = NULL;
D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;        
//驱动程序类型  NULL驱动程序没有渲染功能的参考驱动程序

D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
//描述Direct3D设备所针对的功能,集着色器模型5。

ID3D11Device*                       g_pd3dDevice = NULL;
ID3D11DeviceContext*                g_pImmediateContext = NULL;
IDXGISwapChain*                     g_pSwapChain = NULL;
ID3D11RenderTargetView*             g_pRenderTargetView = NULL;
ID3D11Texture2D*                    g_pDepthStencil = NULL;
ID3D11DepthStencilView*             g_pDepthStencilView = NULL;
ID3D11VertexShader*                 g_pVertexShader = NULL;
ID3D11PixelShader*                  g_pPixelShader = NULL;
ID3D11InputLayout*                  g_pVertexLayout = NULL;
ID3D11Buffer*                       g_pVertexBuffer = NULL;
ID3D11Buffer*                       g_pIndexBuffer = NULL;
ID3D11Buffer*                       g_pCBNeverChanges = NULL;
ID3D11Buffer*                       g_pCBChangeOnResize = NULL;
ID3D11Buffer*                       g_pCBChangesEveryFrame = NULL;
ID3D11ShaderResourceView*           g_pTextureRV = NULL;
ID3D11SamplerState*                 g_pSamplerLinear = NULL;
XMMATRIX                            g_World;
XMMATRIX                            g_View;
XMMATRIX                            g_Projection;
XMFLOAT4                            g_vMeshColor( 0.7f, 0.7f, 0.7f, 1.0f );

//--------------------------------------------------------------------------------------
// Forward declarations 前置声明
//--------------------------------------------------------------------------------------
//HRESULT 一个32位的值，用于描述错误或警告。
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    //UNREFERENCED_PARAMETER用途：hPrevInstance参数我使用过了，别报警告了
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;
      
    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop 主消息循环
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        Render();
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    //windows api
    // Register class 注册窗口类
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window 新建窗口对象。
    g_hInst = hInstance;

    // RECT其左上角和右下角的坐标定义一个矩形
    RECT rc = { 0, 0, 1024, 768 };//窗口大小，//left top 左上角坐标 right bottom 右下角坐标
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    //窗口大小指针， 窗口样式， 窗口是否有菜单

    g_hWnd = CreateWindow( L"TutorialWindowClass", L"Direct3D 11 Tutorial 7", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
                           NULL );
    //窗口的初始水平位置。窗口的初始垂直位置。窗口宽，窗口高  hInstance是窗口关联的模块实例的句柄
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( szFileName, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        if( pErrorBlob ) pErrorBlob->Release();
        return hr;
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// 1 Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    // RECT其左上角和右下角的坐标定义一个矩形
    RECT rc = { 0, 0, 1024, 768 };//窗口大小，//left top 左上角坐标 right bottom 右下角坐标

    GetClientRect( g_hWnd, &rc );
    // 检索窗口的工作区的坐标。
    
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
//#ifdef _DEBUG
//    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
//#endif
    //驱动枚举，从中选择驱动类型
    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,   //硬件驱动程序
        D3D_DRIVER_TYPE_WARP,       //WARP驱动程序，这是一种高性能的软件光栅化程序
        D3D_DRIVER_TYPE_REFERENCE,  //引用驱动器支持每种D3D功能的软件实现。为精度而不是速度而设计
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );//记录数组大小

    //描述D3D Device 所针对的功能集
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1; //描述交换链中缓冲区数量的值

    // BufferDesc 描述后缓冲 显示模式 backbuffer display mode
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  //4通道8bit整数
    sd.BufferDesc.RefreshRate.Numerator = 60;           //刷新率分子
    sd.BufferDesc.RefreshRate.Denominator = 1;          //刷新率分明
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;   //后缓冲区的表面使用情况和CPU访问选项
    sd.OutputWindow = g_hWnd;                           //输出窗口的HWND句柄
    sd.SampleDesc.Count = 1;                            //采样参数，每个像素的采样数
    sd.SampleDesc.Quality = 0.9;                        //图像质量等级。质量越高性能越低。0-9范围
    sd.Windowed = TRUE;

    //遍历driverTypes
    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;        //2D纹理
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer ); 
    //0指从0开始缓冲区索引 __uuidof返回GUID LPVOID指向任何类型的指针。
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRenderTargetView );
    //pBackBuffer 指向渲染目标的Resource的指针，null是创建一个视图，指向ID3D11RenderTargetView的指针
    pBackBuffer->Release();     //释放指针
    if( FAILED( hr ) )
        return hr;

    // Create depth stencil（模型模板） texture 描述2D纹理
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width = width;    //纹理宽度1-16384（128*128）
    descDepth.Height = height;  //纹理高度
    descDepth.MipLevels = 1;    //纹理中的最大mipmap级别数
    //纹理中的最大mipmap级别数1多重采样纹理 | 0 子纹理subtexture
    //mipmap就那个大图缩小成一大堆小图的

    descDepth.ArraySize = 1;    //纹理数量1-2048
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;   //纹理格式  
    //SampleDesc为纹理指定多重采样参数的结构
    descDepth.SampleDesc.Count = 1;     //每个像素的多次采样数
    descDepth.SampleDesc.Quality = 0;   //图像质量0<= x <1 
    descDepth.Usage = D3D11_USAGE_DEFAULT;  //标识如何读取和写入纹理的值
    /*D3D11_USAGE_DEFAULT	需要GPU进行读写访问的资源。这可能是最常见的用法选择。
      D3D11_USAGE_IMMUTABLE	只能由GPU读取的资源。它不能由GPU编写，也不能被CPU完全访问。这种类型的资源必须在创建时初始化，因为创建后无法更改。
      D3D11_USAGE_DYNAMIC	GPU（只读）和CPU（只读）均可访问的资源。对于每帧至少要由CPU更新一次的资源，动态资源是一个不错的选择。若要更新动态资源，请使用Map方法。
      D3D11_USAGE_STAGING	支持从GPU到CPU的数据传输（复制）的资源。*/
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL; //如何将资源绑定管线pipeline
    //D3D11_BIND_RENDER_TARGET	将纹理绑定为输出合并阶段的渲染目标
    descDepth.CPUAccessFlags = 0;   //指定允许的CPU访问类型的标志
    descDepth.MiscFlags = 0;        //不太常见的资源选项的标志
    hr = g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &g_pDepthStencil );
    // descDepth描述2D纹理资源的D3D11_TEXTURE2D_DESC结构的指针 ，
    // 指向创建的纹理的ID3D11Texture2D接口的指针
    if( FAILED( hr ) )
        return hr;

    // Create the depth stencil view 用于访问resource数据
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory( &descDSV, sizeof(descDSV) );
    descDSV.Format = descDepth.Format;          //资源数据格式
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;  //资源类型
    descDSV.Texture2D.MipSlice = 0;             //2D纹理mip切片        
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    // g_pDepthStencil :depth-stencil surface深度模板表面的资源的指针,
    // &descDSV 指向depth-stencil-view的指针 
    // &g_pDepthStencilView 指向ID3D11DepthStencilView的指针的地址
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );
    // Bind 一个或多个渲染目标 和 depth-stencil buffer 到 output-merger stage
    // 绑定的渲染目标的数量 ，
    // 指向ID3D11RenderTargetView数组的指针，代表要绑定到设备的渲染目标
    // 指向ID3D11DepthStencilView的指针，表示要绑定到设备的深度模板视图
    
    // Setup the viewport
    D3D11_VIEWPORT vp;          //定义视口的尺寸
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;     //最小深度，Z-buffer
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;        //视口左上的X位置
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

    // Compile the vertex shader
    ID3DBlob* pVSBlob = NULL;   //用于返回任意长度的数据
    //导入shader
    hr = CompileShaderFromFile( L"Tutorial07.fx", "VS", "vs_4_0", &pVSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

    // Create the vertex shader
    // 从已编译的着色器创建一个顶点着色器对象
    hr = g_pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), 
         pVSBlob->GetBufferSize(), NULL, &g_pVertexShader );
    if( FAILED( hr ) )
    {    
        pVSBlob->Release();
        return hr;
    } 

    // Define the input layout 用来描述输入到shader中的数据组成
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE( layout );

    // Create the input layout
    //describe the input - buffer data for the input - assembler stage
    hr = g_pd3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &g_pVertexLayout );
    pVSBlob->Release();
    if( FAILED( hr ) )
        return hr;

    // Set the input layout
    //Bind an input - layout object to the input - assembler stage.
    g_pImmediateContext->IASetInputLayout( g_pVertexLayout );

    // Compile the pixel shader
    ID3DBlob* pPSBlob = NULL;
    hr = CompileShaderFromFile( L"Tutorial07.fx", "PS", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

    // Create the pixel shader
    hr = g_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader );
    pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;

    // Create vertex buffer
    SimpleVertex vertices[] =
    {//三维位置，四维颜色，二维贴图uv
        { XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        { XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        { XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        { XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        { XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT2( 0.0f, 1.0f ) },

        { XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },
    };

    //描述缓冲区资源。
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd)); //用零填充内存块。起始地址的指针, 长度单位字节
    bd.Usage = D3D11_USAGE_DEFAULT; //如何读写缓冲区，频率是关键
    //D3D11_USAGE_DEFAULT需要GPU进行读写访问的资源最常使用
    bd.ByteWidth = sizeof(SimpleVertex) * 24;   // 缓冲区大小（以字节为单位）
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;    // 确定缓冲区将如何绑定到管道。
    bd.CPUAccessFlags = 0;                      // 不需要CPU访问，则CPU访问标志

    // 初始化子资源的数据
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;    // 初始化数据的指针,这是三角形数据
    // hr检测错误
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
    // 描述缓冲区资源 ，描述初始化数据的D3D11_SUBRESOURCE_DATA结构的指针 ， 缓冲区对象的ID3D11Buffer接口的指针
    if (FAILED(hr))
        return hr;

    // Set vertex buffer
    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );

    // Create index buffer
    // Create vertex buffer
    WORD indices[] =
    // WORD 无符号短整型
    {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * 36;
    // 36 vertices needed for 12 triangles in a triangle list1 
    //三角形列表拓扑中12个三角形有36个顶点
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER; 
    //将一个缓冲区作为索引缓冲区绑定到输入Assembler阶段。
    bd.CPUAccessFlags = 0;

    InitData.pSysMem = indices;//导入索引index
    
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pIndexBuffer );

    //描述缓冲区资源 ，描述初始化数据的D3D11_SUBRESOURCE_DATA结构的指针 ， 缓冲区对象的ID3D11Buffer接口的指针
    if( FAILED( hr ) )
        return hr;

    // Set index buffer
    g_pImmediateContext->IASetIndexBuffer( g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

    // Set primitive topology    //将顶点数据解释为三角形列表
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    //将顶点数据解释为三角形列表

    // Create the constant buffers 常量缓冲区
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(CBNeverChanges);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBNeverChanges );//ID3D11Buffer*类型
    if( FAILED( hr ) )
        return hr;
    
    bd.ByteWidth = sizeof(CBChangeOnResize);
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBChangeOnResize );
    if( FAILED( hr ) )
        return hr;
    
    bd.ByteWidth = sizeof(CBChangesEveryFrame);
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBChangesEveryFrame );
    if( FAILED( hr ) )
        return hr;

    // Load the Texture 读取纹理
    hr = D3DX11CreateShaderResourceViewFromFile( g_pd3dDevice, L"seafloor.dds", NULL, NULL, &g_pTextureRV, NULL );
    if( FAILED( hr ) )
        return hr;

    // Create the sample state
    // Describes a sampler state.
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory( &sampDesc, sizeof(sampDesc) );
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // 采样纹理时要使用的过滤方法
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;     // 解析0到1范围之外的au纹理坐标
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;     // 解析0到1范围之外的av纹理坐标
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;     // 解析0到1范围之外的w纹理坐标
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;   // 采样数据与现有采样数据进行比较的功能
    sampDesc.MinLOD = 0;                                // mipmap范围的下限以限制访问
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;                // mipmap范围的上限以限制访问
    hr = g_pd3dDevice->CreateSamplerState( &sampDesc, &g_pSamplerLinear );
    if( FAILED( hr ) )
        return hr;

    // Initialize the world matrix 初始化世界矩阵
    g_World = XMMatrixIdentity();//单位矩阵

    // Initialize the view matrix 视图矩阵
    XMVECTOR Eye = XMVectorSet( 0.0f, 3.0f, -10.0f, 0.0f );  // 相机的位置
    XMVECTOR At = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );    // 焦点的位置
    XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );    // 相机的向上方向位置
    g_View = XMMatrixLookAtLH( Eye, At, Up );
    //相机位置，向上方向 ，焦点为左手坐标系构建视图矩阵

    // ？
    CBNeverChanges cbNeverChanges;
    cbNeverChanges.mView = XMMatrixTranspose( g_View );
    g_pImmediateContext->UpdateSubresource( g_pCBNeverChanges, 0, NULL, &cbNeverChanges, 0, 0 );
    //The CPU copies data from memory to a subresource created in non-mappable memory
    // 目标资源的指针，从零索引，NULL则数据将无偏移地写入目标子资源，指向内存中源数据的指针
    
    // Initialize the projection matrix   投影矩阵
    g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV4,   //90度自上而下的视场角（以弧度
        width / (FLOAT)height, 0.01f, 100.0f );
    //视空间的宽高比X：Y ，到附近裁剪平面的距离， 到远裁剪平面的距离
    //表示我们看不见小于 0.1 或超过 110 的任何内容
    
    CBChangeOnResize cbChangesOnResize;
    cbChangesOnResize.mProjection = XMMatrixTranspose( g_Projection );
    g_pImmediateContext->UpdateSubresource( g_pCBChangeOnResize, 0, NULL, &cbChangesOnResize, 0, 0 );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();

    if( g_pSamplerLinear ) g_pSamplerLinear->Release();
    if( g_pTextureRV ) g_pTextureRV->Release();
    if( g_pCBNeverChanges ) g_pCBNeverChanges->Release();
    if( g_pCBChangeOnResize ) g_pCBChangeOnResize->Release();
    if( g_pCBChangesEveryFrame ) g_pCBChangesEveryFrame->Release();
    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
    if( g_pIndexBuffer ) g_pIndexBuffer->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
    if( g_pDepthStencil ) g_pDepthStencil->Release();
    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
//HWND是线程相关的，通过HWND找到该窗口所属进程和线程
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}


//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
    // Update our time
    static float t = 0.0f;
    if( g_driverType == D3D_DRIVER_TYPE_REFERENCE )//驱动程序，它是支持每种Direct3D功能的软件实现
    {
        t += ( float )XM_PI * 0.0125f;
    }
    else
    {
        static DWORD dwTimeStart = 0;   //32位无符号整数
        DWORD dwTimeCur = GetTickCount64();   //检索自系统启动以来经过的毫秒数，GetTickCount()最长为49.7天。
        if( dwTimeStart == 0 )
            dwTimeStart = dwTimeCur;
        t = ( dwTimeCur - dwTimeStart ) / 1000.0f;
    }

    //
    // Animate the cube 立方体动画
    //
    g_World = XMMatrixRotationY(t); //绕Y周旋转的矩阵，t是角度，修改可换角度看

    // Modify修改 the color
    g_vMeshColor.x = ( sinf( t * 1.0f ) + 1.0f ) * 0.5f;
    g_vMeshColor.y = ( cosf( t * 3.0f ) + 1.0f ) * 0.5f;
    g_vMeshColor.z = ( sinf( t * 5.0f ) + 1.0f ) * 0.5f;

    //
    // Clear the back buffer 清理后置缓冲区
    //
    float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red, green, blue, alpha
    g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, ClearColor );

    //
    // Clear the depth buffer to 1.0 (max depth)
    //
    g_pImmediateContext->ClearDepthStencilView( g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );
    // 1.0f:Clear the depth buffer with this value.
    // 0:   Clear the stencil buffer with this value.
    
    // Update variables that change once per frame
    // 每一帧更改变量
    CBChangesEveryFrame cb;
    cb.mWorld = XMMatrixTranspose( g_World );   //转置
    cb.vMeshColor = g_vMeshColor;               //更替颜色
    g_pImmediateContext->UpdateSubresource( g_pCBChangesEveryFrame, 0, NULL, &cb, 0, 0 );
    // 将指针传递给以与着色器常量缓冲区相同的顺序存储的矩阵
    /*为了做到这一点，我们将创建一个与着色器中的常量缓冲区具有相同布局的结构。
    另外，由于矩阵在C++和HLSL中的内存排列方式不同，我们必须在更新之前转置矩阵*/

    //
    // Render the cube
    //
    g_pImmediateContext->VSSetShader( g_pVertexShader, NULL, 0 );
    g_pImmediateContext->VSSetConstantBuffers( 0, 1, &g_pCBNeverChanges );
    g_pImmediateContext->VSSetConstantBuffers( 1, 1, &g_pCBChangeOnResize );
    g_pImmediateContext->VSSetConstantBuffers( 2, 1, &g_pCBChangesEveryFrame );
    g_pImmediateContext->PSSetShader( g_pPixelShader, NULL, 0 );
    g_pImmediateContext->PSSetConstantBuffers( 2, 1, &g_pCBChangesEveryFrame );
    g_pImmediateContext->PSSetShaderResources( 0, 1, &g_pTextureRV );
    g_pImmediateContext->PSSetSamplers( 0, 1, &g_pSamplerLinear );
    g_pImmediateContext->DrawIndexed( 36, 0, 0 );

    //
    // Present our back buffer to our front buffer
    //
    g_pSwapChain->Present( 0, 0 );
}
