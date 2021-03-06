#pragma once
#include "stdafx.h"
#include "Application/Resources/ImageResourceID.h"
#include "PackageProject/ModPackageProject.h"

namespace Kortex
{
	class ModPackage;
}
namespace Kortex::Application
{
	class ActiveInstanceOption;
}
namespace Kortex::InstallWizard
{
	class WizardDialog;
	enum class WizardPageID
	{
		None = -1,
		Info,
		Requirements,
		Components,
		Installation,
		Completed,
	};
}

namespace Kortex::InstallWizard
{
	class WizardButton
	{
		private:
			wxString m_Label;
			bool m_Enabled = true;

		public:
			WizardButton() = default;
			WizardButton(const wxString& label, bool isEnabled)
				:m_Label(label), m_Enabled(isEnabled)
			{
			}

		public:
			bool HasLabel() const
			{
				return !m_Label.IsEmpty();
			}
			const wxString& GetLabel() const
			{
				return m_Label;
			}
			bool IsEnabled() const
			{
				return m_Enabled;
			}
	};
}

namespace Kortex::InstallWizard
{
	class WizardPage: public wxEvtHandler
	{
		friend class WizardDialog;

		private:
			WizardDialog& m_Wizard;

		protected:
			virtual void OnLoadUIOptions(const Application::ActiveInstanceOption& option) = 0;
			virtual void OnSaveUIOptions(Application::ActiveInstanceOption& option) const = 0;
			virtual void OnPackageLoaded() = 0;

			virtual bool OnOpenPage()
			{
				return true;
			}
			virtual bool OnClosePage()
			{
				return true;
			}

			virtual WizardButton GetCancelButton()
			{
				return {};
			}
			virtual WizardButton GetBackwardButton()
			{
				return {};
			}
			virtual WizardButton GetForwardButton()
			{
				return {};
			}

		public:
			WizardPage(WizardDialog& wizard)
				:m_Wizard(wizard)
			{
			}

		public:
			const WizardDialog& GetWizard() const
			{
				return m_Wizard;
			}
			WizardDialog& GetWizard()
			{
				return m_Wizard;
			}

			wxWindow* GetPageContainer() const;
			ModPackage& GetPackage() const;
			ModPackageProject& GetPackageConfig() const;

		public:
			virtual wxWindow* Create() = 0;
			virtual wxWindow* GetWindow() = 0;

			virtual WizardPageID GetID() const = 0;
			virtual wxString GetCaption() const = 0;
			virtual wxString GetOptionName() const = 0;
	};
}
