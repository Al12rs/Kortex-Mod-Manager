#pragma once
#include "stdafx.h"
#include "Network/Common.h"
#include "Network/IModNetwork.h"
#include <KxFramework/KxSingleton.h>
class KxCURLSession;

namespace Kortex::NetworkManager
{
	class LoversLabModNetwork:
		public KxRTTI::IExtendInterface<LoversLabModNetwork, IModNetwork>,
		public KxSingletonPtr<LoversLabModNetwork>
	{
		private:
			wxString GetAPIURL() const;

		public:
			LoversLabModNetwork();

		public:
			KImageEnum GetIcon() const override;
			wxString GetName() const override;

			wxString TranslateGameIDToNetwork(const GameID& id = {}) const override
			{
				return {};
			}
			GameID TranslateGameIDFromNetwork(const wxString& id) const override
			{
				return {};
			}

			wxString GetModPageBaseURL(const GameID& id = {}) const override;
			wxString GetModPageURL(const ModRepositoryRequest& request) override;
	};
}