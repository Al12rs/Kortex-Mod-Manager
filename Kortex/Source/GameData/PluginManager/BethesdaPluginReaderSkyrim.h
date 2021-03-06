#pragma once
#include "stdafx.h"
#include "IPluginReader.h"
#include "IBethesdaPluginReader.h"

namespace Kortex::PluginManager
{
	class BethesdaPluginReaderSkyrim: public IBethesdaPluginReader
	{
		private:
			BethesdaPluginData m_Data;
			bool m_IsOK = false;

		protected:
			void OnRead(const IGamePlugin& plugin) override;
			
		public:
			bool IsOK() const override
			{
				return m_IsOK;
			}
			BethesdaPluginData& GetData() override
			{
				return m_Data;
			}
	};
}
