#include "stdafx.h"
#include "SimpleItem.h"
#include "Utility/KAux.h"

namespace
{
	using Kortex::GameConfig::ItemSamples;

	template<class TValidator> void SetValidatorBounds(TValidator& validator, const ItemSamples& samples)
	{
		using T = typename TValidator::ValueType;

		T min = 0;
		T max = 0;
		samples.GetBoundValues(min, max);
		validator.SetMin(min);
		validator.SetMax(max);
	}
}

namespace Kortex::GameConfig
{
	bool SimpleItem::Create(const KxXMLNode& itemNode)
	{
		return IsOK();
	}

	void SimpleItem::Clear()
	{
		m_Value.MakeNull();
	}
	void SimpleItem::Read(const ISource& source)
	{
		source.ReadValue(*this, m_Value);
	}
	void SimpleItem::Write(ISource& source) const
	{
		source.WriteValue(*this, m_Value);
	}

	std::unique_ptr<wxValidator> SimpleItem::CreateValidator() const
	{
		const ItemOptions& options = GetOptions();
		const TypeID type = GetTypeID();

		if (type.IsSignedInteger())
		{
			auto validator = std::make_unique<wxIntegerValidator<int64_t>>();
			SetValidatorBounds(*validator, GetSamples());
			return validator;
		}
		else if (type.IsUnsignedInteger())
		{
			auto validator = std::make_unique<wxIntegerValidator<uint64_t>>();
			SetValidatorBounds(*validator, GetSamples());
			return validator;
		}
		else if (type.IsFloat())
		{
			auto validator = std::make_unique<wxFloatingPointValidator<double>>();
			SetValidatorBounds(*validator, GetSamples());

			if (options.HasPrecision())
			{
				validator->SetPrecision(options.GetPrecision());
			}
			return validator;
		}
		return nullptr;
	}
	std::unique_ptr<KxDataView2::Editor> SimpleItem::CreateEditor() const
	{
		std::unique_ptr<KxDataView2::Editor> editor;
		const bool isEditable = IsEditable();

		if (HasSamples())
		{
			auto comboBox = std::make_unique<KxDataView2::ComboBoxEditor>();
			comboBox->SetMaxVisibleItems(32);
			comboBox->AutoPopup(!isEditable);

			// Add samples
			comboBox->ClearItems();
			const ItemSamples& samples = GetSamples();
			if (!samples.IsEmpty())
			{
				samples.ForEachSample([this, &comboBox](const SampleValue& sample)
				{
					const ItemValue& value = sample.GetValue();
					if (sample.HasLabel())
					{
						comboBox->AddItem(KxString::Format(wxS("%1 - %2"), value.Serialize(*this), sample.GetLabel()));
					}
					else
					{
						comboBox->AddItem(value.Serialize(*this));
					}
				});
			}
			editor = std::move(comboBox);
		}
		else
		{
			editor = std::make_unique<KxDataView2::TextEditor>();
		}

		editor->SetEditable(isEditable);
		editor->SetValidator(CreateValidator());
		return editor;
	}

	SimpleItem::SimpleItem(ItemGroup& group, const KxXMLNode& itemNode)
		:IExtendInterface(group, itemNode)
	{
	}
	SimpleItem::SimpleItem(ItemGroup& group, bool isUnknown)
		:IExtendInterface(group), m_IsUnknown(isUnknown)
	{
	}

	wxString SimpleItem::GetStringRepresentation(ColumnID id) const
	{
		if (id == ColumnID::Value)
		{
			return m_Value.As<wxString>();
		}
		return Item::GetStringRepresentation(id);
	}

	wxAny SimpleItem::GetValue(const KxDataView2::Column& column) const
	{
		switch (column.GetID<ColumnID>())
		{
			case ColumnID::Path:
			{
				return GetFullPath();
			}
			case ColumnID::Name:
			{
				return GetLabel();
			}
			case ColumnID::Type:
			{
				return GetTypeID().ToString();
			}
			case ColumnID::Value:
			{
				if (!m_CachedViewData)
				{
					auto FormatValue = [this](const ItemValue& value)
					{
						wxString serializedValue = value.Serialize(*this);

						if (serializedValue.IsEmpty())
						{
							if (value.GetType().IsString())
							{
								return KAux::MakeBracketedLabel(GetManager().GetTranslator().GetString(wxS("ConfigManager.View.EmptyStringValue")));
							}
							else
							{
								return KAux::MakeNoneLabel();
							}
						}
						return serializedValue;
					};

					const SampleValue* sampleValue = GetSamples().FindSampleByValue(m_Value);
					if (sampleValue && sampleValue->HasLabel())
					{
						m_CachedViewData = KxString::Format(wxS("%1 - %2"), FormatValue(m_Value), sampleValue->GetLabel());
					}
					else
					{
						m_CachedViewData = FormatValue(m_Value);
					}
				}
				return *m_CachedViewData;
			}
		}
		return {};
	}
	wxAny SimpleItem::GetEditorValue(const KxDataView2::Column& column) const
	{
		switch (column.GetID<ColumnID>())
		{
			case ColumnID::Value:
			{
				if (IsReadOnlyComboBox())
				{
					size_t index = 0;
					GetSamples().FindSampleByValue(m_Value, &index);
					return index;
				}
				else
				{
					return m_Value.Serialize(*this);
				}
			}
		}
		return {};
	}
	bool SimpleItem::SetValue(const wxAny& value, KxDataView2::Column& column)
	{
		switch (column.GetID<ColumnID>())
		{
			case ColumnID::Value:
			{
				if (IsReadOnlyComboBox())
				{
					const SampleValue* sampleValue = GetSamples().GetSampleByIndex(value.As<size_t>());
					if (sampleValue)
					{
						m_Value.Deserialize(sampleValue->GetValue().Serialize(*this), *this);
						m_CachedViewData.reset();
						return true;
					}
				}
				else
				{
					wxString data;
					value.GetAs(&data);
					m_Value.Deserialize(data, *this);
					m_CachedViewData.reset();
					return true;
				}
				return false;
			}
		}
		return false;
	}

	KxDataView2::Renderer& SimpleItem::GetRenderer(const KxDataView2::Column& column) const
	{
		return column.GetRenderer();
	}
	KxDataView2::Editor* SimpleItem::GetEditor(const KxDataView2::Column& column) const
	{
		if (column.GetID<ColumnID>() == ColumnID::Value)
		{
			if (!m_Editor)
			{
				m_Editor = CreateEditor();
			}
			return m_Editor.get();
		}
		return nullptr;
	}
	bool SimpleItem::GetAttributes(KxDataView2::CellAttributes& attributes, const KxDataView2::CellState& cellState, const KxDataView2::Column& column) const
	{
		return false;
	}
}