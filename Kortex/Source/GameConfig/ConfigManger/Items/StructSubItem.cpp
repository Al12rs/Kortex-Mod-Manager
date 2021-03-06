#include "stdafx.h"
#include "StructSubItem.h"
#include "StructItem.h"

namespace Kortex::GameConfig
{
	StructSubItem::StructSubItem(StructItem& structItem, const KxXMLNode& itemNode)
		:ExtendInterface(structItem.GetGroup(), itemNode), m_Struct(structItem)
	{
		GetOptions().Load(itemNode.GetFirstChildElement(wxS("Options")), GetDataType());

		uint32_t copyMask = ItemOptionsCopy::Everything & ~(ItemOptionsCopy::InputFormat|ItemOptionsCopy::OutputFormat);
		GetOptions().CopyIfNotSpecified(m_Struct.GetOptions(), {}, static_cast<ItemOptionsCopy>(copyMask));
	}

	bool StructSubItem::IsOK() const
	{
		return GetTypeID().IsDefinitiveType() && !GetName().IsEmpty();
	}
	wxString StructSubItem::GetViewString(ColumnID id) const
	{
		if (id == ColumnID::Path)
		{
			return m_Struct.GetName() + wxS("::") + GetName();
		}
		return SimpleItem::GetViewString(id);
	}
	wxString StructSubItem::GetPath() const
	{
		return m_Struct.GetPath();
	}

	bool StructSubItem::SetValue(KxDataView2::Column& column, const wxAny& value)
	{
		if (SimpleItem::SetValue(column, value))
		{
			m_Struct.ChangeNotify();
			return true;
		}
		return false;
	}
}
