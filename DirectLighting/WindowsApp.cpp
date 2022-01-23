#include "WindowsApp.h"

LWindow* WindowsApp::m_pWindow = nullptr;

int WindowsApp::Run(D12Core* pCore, HINSTANCE hInstance, int nCmdShow)
{
	if (m_pWindow)
		delete(m_pWindow);
	m_pWindow = new LWindow();
	m_pWindow->Init(hInstance, nCmdShow, pCore->getWidth(), pCore->getHeight(), true, WindowProc, pCore->getTitle().c_str());
	



	
	if (!pCore->onInit(m_pWindow))
	{
		return 1;
	}
	messageloop(pCore);
	
  return 0;
}

LRESULT WindowsApp::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			DestroyWindow(hWnd);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd,
		message,
		wParam,
		lParam);
}

int WindowsApp::messageloop(D12Core* pCore)
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	EngineStatus::m_status = EngineStatus::Status::sRUNNING;
	bool running = true;
	while (running)
	{
		BOOL PeekMessageL(
			LPMSG lpMsg,
			HWND hWnd,
			UINT wMsgFilterMin,
			UINT wMsgFilterMax,
			UINT wRemoveMsg
		);

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			//if (msg.message == WM_QUIT)
				//break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			// run game code  
			switch (EngineStatus::m_status)
			{
				case EngineStatus::Status::sRUNNING:
				{
					//m_jellyFish.Update();
				  //m_jellyFish.Render();
					
					pCore->onUpdate();
					pCore->onRender();
				}break;
				case EngineStatus::Status::sERRORED:
				{
					running = false;
				}break;
			}
		}
	}
	//m_jellyFish.Cleanup();
	pCore->onDestroy();
	return msg.wParam;
}
