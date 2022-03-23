// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include "ComponentBase.h"
#include "Toolbar.h"
#include "resource.h"
#include <dcomp.h>
#include <functional>
#include <memory>
#include <ole2.h>
#include <string>
#include <vector>
#include <winnt.h>
#ifdef USE_WEBVIEW2_WIN10
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.ViewManagement.h>

namespace winrtComp = winrt::Windows::UI::Composition;
#endif

class SettingsComponent;

enum class WebViewCreateEntry
{
    OTHER = 0,
    EVER_FROM_CREATE_WITH_OPTION_MENU = 1,
};
class AppWindow;
struct WebViewCreateOption
{
    std::wstring profile;
    bool isInPrivate = false;
    std::wstring downloadPath;

    // This value is inherited from the operated AppWindow
    WebViewCreateEntry entry = WebViewCreateEntry::OTHER;
    WebViewCreateOption()
    {
    }

    WebViewCreateOption(const std::wstring& profile_, bool inPrivate,
        const std::wstring& downloadPath, WebViewCreateEntry entry_)
        : profile(profile_), isInPrivate(inPrivate),
          downloadPath(downloadPath), entry(entry_)
    {
    }

    WebViewCreateOption(const WebViewCreateOption& opt)
    {
        profile = opt.profile;
        isInPrivate = opt.isInPrivate;
        downloadPath = opt.downloadPath;
        entry = opt.entry;
    }

    void PopupDialog(AppWindow* app);
};

class AppWindow
{
public:
    AppWindow(
        UINT creationModeId,
        const WebViewCreateOption& opt,
        const std::wstring& initialUri = L"",
        const std::wstring& userDataFolderParam = L"",
        bool isMainWindow = false,
        std::function<void()> webviewCreatedCallback = nullptr,
        bool customWindowRect = false,
        RECT windowRect = { 0 },
        bool shouldHaveToolbar = true);

    ~AppWindow();

    ICoreWebView2Controller* GetWebViewController()
    {
        return m_controller.get();
    }
    ICoreWebView2* GetWebView()
    {
        return m_webView.get();
    }
    ICoreWebView2Environment* GetWebViewEnvironment()
    {
        return m_webViewEnvironment.get();
    }
    HWND GetMainWindow()
    {
        return m_mainWindow;
    }
    void SetDocumentTitle(PCWSTR titleText);
    std::wstring GetDocumentTitle();
    RECT GetWindowBounds();
    std::wstring GetLocalUri(std::wstring path, bool useVirtualHostName = true);
    std::function<void()> GetAcceleratorKeyFunction(UINT key);
    double GetDpiScale();
    double GetTextScale();

    void ReinitializeWebView();

    template <class ComponentType, class... Args> void NewComponent(Args&&... args);

    template <class ComponentType> ComponentType* GetComponent();

    void DeleteComponent(ComponentBase* scenario);

    // Runs a function by posting it to the event loop.  Use this to do things
    // that shouldn't be done in event handlers, like show message boxes.
    // If you use this in a component, capture a pointer to this AppWindow
    // instead of the component, because the component could get deleted before
    // the AppWindow.
    void RunAsync(std::function<void(void)> callback);

    // Calls win32 MessageBox inside RunAsync.  Always uses MB_OK.  If you need
    // to get the return value from MessageBox, you'll have to use RunAsync
    // yourself.
    void AsyncMessageBox(std::wstring message, std::wstring title);

    void InstallComplete(int return_code);

    void AddRef();
    void Release();
    void NotifyClosed();
    void EnableHandlingNewWindowRequest(bool enable);


    void SetOnAppWindowClosing(std::function<void()>&& f) {
      m_onAppWindowClosing = std::move(f);
    }

    std::wstring GetUserDataFolder()
    {
        return m_userDataFolder;
    }
    const DWORD GetCreationModeId()
    {
        return m_creationModeId;
    }

    const WebViewCreateOption& GetWebViewOption()
    {
        return m_webviewOption;
    }

private:
    static PCWSTR GetWindowClass();

    static INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK
    WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    bool HandleWindowMessage(
        HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result);

    bool ExecuteWebViewCommands(WPARAM wParam, LPARAM lParam);
    bool ExecuteAppCommands(WPARAM wParam, LPARAM lParam);

    void ResizeEverything();
    void InitializeWebView();
    HRESULT CreateControllerWithOptions();
    void SetAppIcon(bool inPrivate);

    HRESULT OnCreateEnvironmentCompleted(HRESULT result, ICoreWebView2Environment* environment);
    HRESULT OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller);
    HRESULT DeleteFileRecursive(std::wstring path);
    void RegisterEventHandlers();
    void ReinitializeWebViewWithNewBrowser();
    void RestartApp();
    bool CloseWebView(bool cleanupUserDataFolder = false);
    void CleanupUserDataFolder();
    void CloseAppWindow();
    void ChangeLanguage();
    void UpdateCreationModeMenu();
    void ToggleAADSSO();
    bool ClearBrowsingData(COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds);
    void UpdateAppTitle();
    void ToggleExclusiveUserDataFolderAccess();
#ifdef USE_WEBVIEW2_WIN10
    void OnTextScaleChanged(
        winrt::Windows::UI::ViewManagement::UISettings const& uiSettings,
        winrt::Windows::Foundation::IInspectable const& args);
#endif

    std::wstring GetLocalPath(std::wstring path, bool keep_exe_path);
    void DeleteAllComponents();

    template <class ComponentType> std::unique_ptr<ComponentType> MoveComponent();

    // The initial URI to which to navigate the WebView2's top level document.
    // This is either empty string in which case we will use StartPage::GetUri,
    //  or "none" to mean don't perform an initial navigate,
    //  or a valid absolute URI to which we will navigate.
    std::wstring m_initialUri;
    std::wstring m_userDataFolder;
    HWND m_mainWindow = nullptr;
    Toolbar m_toolbar;
    std::function<void()> m_onWebViewFirstInitialized;
    std::function<void()> m_onAppWindowClosing;
    DWORD m_creationModeId = 0;
    int m_refCount = 1;
    bool m_isClosed = false;

    // The following is state that belongs with the webview, and should
    // be reinitialized along with it. Everything here is undefined when
    // m_webView is null.
    wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
    wil::com_ptr<ICoreWebView2Controller> m_controller;
    wil::com_ptr<ICoreWebView2> m_webView;
    wil::com_ptr<ICoreWebView2_3> m_webView3;

    bool m_shouldHandleNewWindowRequest = true;

    EventRegistrationToken m_browserExitedEventToken = {};
    UINT32 m_newestBrowserPid = 0;

    // All components are deleted when the WebView is closed.
    std::vector<std::unique_ptr<ComponentBase>> m_components;
    // options for creation of webview controller
    WebViewCreateOption m_webviewOption;
    std::wstring m_profileDirName;

    std::unique_ptr<SettingsComponent> m_oldSettingsComponent;

    std::wstring m_language;

    // app title, initialized in constructor
    std::wstring m_appTitle;

    // document title from web page that wants to show in window title bar
    std::wstring m_documentTitle;

    bool m_AADSSOEnabled = false;
    bool m_ExclusiveUserDataFolderAccess = false;

    // Fullscreen related code
    RECT m_previousWindowRect;
    HMENU m_hMenu;
    BOOL m_containsFullscreenElement = FALSE;
    bool m_fullScreenAllowed = true;
    bool m_isPopupWindow = false;
    void EnterFullScreen();
    void ExitFullScreen();

    // Compositor creation helper methods
    HRESULT DCompositionCreateDevice2(IUnknown* renderingDevice, REFIID riid, void** ppv);
    HRESULT TryCreateDispatcherQueue();

    wil::com_ptr<IDCompositionDevice> m_dcompDevice;
#ifdef USE_WEBVIEW2_WIN10
    winrtComp::Compositor m_wincompCompositor{ nullptr };
    winrt::Windows::UI::ViewManagement::UISettings m_uiSettings{ nullptr };
#endif

    // Background Image members
    HBITMAP m_appBackgroundImageHandle;
    BITMAP m_appBackgroundImage;
    HDC m_memHdc;
    RECT m_appBackgroundImageRect;
};

template <class ComponentType, class... Args> void AppWindow::NewComponent(Args&&... args)
{
    m_components.emplace_back(new ComponentType(std::forward<Args>(args)...));
}

template <class ComponentType> ComponentType* AppWindow::GetComponent()
{
    for (auto& component : m_components)
    {
        if (auto wanted = dynamic_cast<ComponentType*>(component.get()))
        {
            return wanted;
        }
    }
    return nullptr;
}
