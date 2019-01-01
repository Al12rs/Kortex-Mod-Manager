#include "stdafx.h"
#include "IApplication.h"
#include "SystemApplication.h"
#include <Kortex/Application.hpp>
#include <Kortex/Events.hpp>
#include "Archive/KArchive.h"
#include <KxFramework/KxSystem.h>

namespace Kortex
{
	SystemApplication* IApplication::GetSystemApp()
	{
		return SystemApplication::GetInstance();
	}

	wxString IApplication::RethrowCatchAndGetExceptionInfo() const
	{
		return GetSystemApp()->RethrowCatchAndGetExceptionInfo();
	}

	void IApplication::InitGlobalManagers()
	{
		wxLogInfo("Initializing modules");

		IModule::ForEachModuleAndManager([](IModule& module, IManager* manager = nullptr)
		{
			if (manager)
			{
				wxLogInfo("Initializing manager: %s::%s", module.GetModuleInfo().GetID(), manager->GetManagerInfo().GetID());
				manager->OnInit();
			}
			else
			{
				if (module.GetModuleDisposition() == IModule::Disposition::Global)
				{
					wxLogInfo("Initializing module: %s", module.GetModuleInfo().GetID());
					module.OnInit();
				}
			}
		});
	}
	void IApplication::UnInitGlobalManagers()
	{
		wxLogInfo("Uninitializing modules.");

		IModule::ForEachModuleAndManager([](IModule& module, IManager* manager = nullptr)
		{
			if (manager)
			{
				wxLogInfo("Uninitializing manager: %s::%s", module.GetModuleInfo().GetID(), manager->GetManagerInfo().GetID());
				manager->OnExit();
			}
			else
			{
				if (module.GetModuleDisposition() == IModule::Disposition::Global)
				{
					wxLogInfo("Uninitializing module: %s", module.GetModuleInfo().GetID());
					module.OnExit();
				}
			}
		});
	}

	bool IApplication::Is64Bit() const
	{
		#if defined _WIN64
		return true;
		#else
		return false;
		#endif
	}
	bool IApplication::IsSystem64Bit() const
	{
		return KxSystem::Is64Bit();
	}

	bool IApplication::IsAnotherRunning() const
	{
		return GetSystemApp()->IsAnotherRunning();
	}
	bool IApplication::QueueDownloadToMainProcess(const wxString& link)
	{
		return GetSystemApp()->QueueDownloadToMainProcess(link);
	}

	void IApplication::EnableIE10Support()
	{
		GetSystemApp()->ConfigureForInternetExplorer10(true);
	}
	void IApplication::DisableIE10Support()
	{
		GetSystemApp()->ConfigureForInternetExplorer10(false);
	}

	wxString IApplication::GetID() const
	{
		return GetSystemApp()->GetAppName();
	}
	wxString IApplication::GetName() const
	{
		return GetSystemApp()->GetAppDisplayName();
	}
	wxString IApplication::GetDeveloper() const
	{
		return GetSystemApp()->GetVendorName();
	}
	KxVersion IApplication::GetVersion() const
	{
		return GetSystemApp()->GetAppVersion();
	}
	KxVersion IApplication::GetWxWidgetsVersion() const
	{
		return wxGetLibraryVersionInfo();
	}

	KxXMLDocument& IApplication::GetGlobalConfig() const
	{
		return GetSystemApp()->GetGlobalConfig();
	}

	wxCmdLineParser& IApplication::GetCmdLineParser() const
	{
		return GetSystemApp()->GetCmdLineParser();
	}
	bool IApplication::ParseCommandLine()
	{
		return GetSystemApp()->ParseCommandLine();
	}

	wxWindow* IApplication::GetActiveWindow() const
	{
		return ::wxGetActiveWindow();
	}
	wxWindow* IApplication::GetTopWindow() const
	{
		return GetSystemApp()->GetTopWindow();
	}
	void IApplication::SetTopWindow(wxWindow* window)
	{
		return GetSystemApp()->SetTopWindow(window);
	}

	void IApplication::ExitApp(int exitCode)
	{
		GetSystemApp()->ExitApp(exitCode);
	}
	wxLog& IApplication::GetLogger()
	{
		return *GetSystemApp()->GetLogger();
	}
	LoadTranslationStatus IApplication::TryLoadTranslation(KxTranslation& translation,
														   const KxTranslation::AvailableMap& availableTranslations,
														   const wxString& desiredLocale
	) const
	{
		auto LoadLang = [&translation, &availableTranslations](const wxString& locale) -> bool
		{
			auto it = availableTranslations.find(locale);
			if (it != availableTranslations.end())
			{
				wxLogInfo("Trying to load translation from file \"%s\" for \"%s\" locale", it->second, locale);
				if (translation.LoadFromFile(it->second))
				{
					translation.SetLocale(locale);
					return true;
				}
			}
			return false;
		};

		if (!availableTranslations.empty())
		{
			// Try load translation for desired locale
			if (LoadLang(desiredLocale))
			{
				return LoadTranslationStatus::Success;
			}

			// Try default locales
			if (LoadLang(KxTranslation::GetUserDefaultLocale()) ||
				LoadLang(KxTranslation::GetSystemPreferredLocale()) ||
				LoadLang(KxTranslation::GetSystemDefaultLocale()) ||
				LoadLang("en-US"))
			{
				return LoadTranslationStatus::Success;
			}

			// Try first available
			const auto& first = *availableTranslations.begin();
			if (LoadLang(first.first))
			{
				return LoadTranslationStatus::Success;
			}
			return LoadTranslationStatus::LoadingError;
		}
		return LoadTranslationStatus::NoTranslations;
	}
}
