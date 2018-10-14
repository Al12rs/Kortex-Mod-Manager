#pragma once
#include "stdafx.h"
#include "KNetworkConstants.h"
#include "KNetworkProvider.h"
#include "KNetworkProviderLoversLabModInfo.h"
#include <KxFramework/KxSingleton.h>
class KNetworkProviderLoversLab;
class KxCURLSession;

//////////////////////////////////////////////////////////////////////////
class KNetworkProviderLoversLab: public KNetworkProvider, public KxSingletonPtr<KNetworkProviderLoversLab>
{
	public:
		static constexpr KNetworkProviderID GetTypeID()
		{
			return KNETWORK_PROVIDER_ID_LOVERSLAB;
		}

	private:
		wxString GetAPIURL() const;

	protected:
		virtual bool DoAuthenticate(wxWindow* window = NULL) override;
		virtual bool DoValidateAuth(wxWindow* window = NULL) override;
		virtual bool DoSignOut(wxWindow* window = NULL) override;
		virtual bool DoIsAuthenticated() const override;

	public:
		KNetworkProviderLoversLab(KNetworkProviderID providerID);

	public:
		virtual KImageEnum GetIcon() const override;
		virtual wxString GetName() const override;
		virtual wxString GetGameID(const KGameID& id = KGameIDs::NullGameID) const override;
		virtual wxString GetModURLBasePart(const KGameID& id = KGameIDs::NullGameID) const override;
		virtual wxString GetModURL(int64_t modID, const wxString& modSignature = wxEmptyString, const KGameID& id = KGameIDs::NullGameID) override;

		virtual ModInfo GetModInfo(int64_t modID, const KGameID& id = KGameIDs::NullGameID) const override;
		virtual FileInfo GetFileInfo(int64_t modID, int64_t fileID, const KGameID& id = KGameIDs::NullGameID) const override;
		virtual FileInfo::Vector GetFilesList(int64_t modID, const KGameID& id = KGameIDs::NullGameID) const override;
		virtual DownloadInfo::Vector GetFileDownloadLinks(int64_t modID, int64_t fileID, const KGameID& id = KGameIDs::NullGameID) const override;
		virtual EndorsedInfo EndorseMod(int64_t modID, EndorsementState::Value state = EndorsementState::Endorse, const KGameID& id = KGameIDs::NullGameID) override;
};
