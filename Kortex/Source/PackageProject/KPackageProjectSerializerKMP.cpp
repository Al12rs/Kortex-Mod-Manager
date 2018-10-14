#include "stdafx.h"
#include "KPackageProjectSerializerKMP.h"
#include "KPackageProjectSerializer.h"
#include "KPackageProject.h"
#include "KPackageProjectConfig.h"
#include "KPackageProjectInfo.h"
#include "KPackageProjectInterface.h"
#include "KPackageProjectFileData.h"
#include "KPackageProjectRequirements.h"
#include "KPackageProjectComponents.h"
#include "PackageManager/KPackageManager.h"
#include "ModManager/KModManager.h"
#include "GameInstance/KGameInstance.h"
#include "KAux.h"

static void WriteKPPCFlagEntryArray(const KPPCFlagEntryArray& array, KxXMLNode& arrayNode, bool isRequired)
{
	arrayNode.ClearChildren();
	for (const KPPCFlagEntry& value: array)
	{
		KxXMLNode entryNode = arrayNode.NewElement("Flag");
		entryNode.SetValue(value.GetValue());
		entryNode.SetAttribute("Name", value.GetName());
		if (isRequired)
		{
			entryNode.SetAttribute("Operator", KPackageProjectRequirements::OperatorToString(value.GetOperator()));
		}
	}
}
static void ReadKPPCFlagEntryArray(KPPCFlagEntryArray& array, const KxXMLNode& arrayNode, bool isRequired)
{
	for (KxXMLNode node = arrayNode.GetFirstChildElement(); node.IsOK(); node = node.GetNextSiblingElement())
	{
		KPPOperator operatorType = KPackageProjectComponents::ms_DefaultFlagsOperator;
		if (isRequired)
		{
			operatorType = KPackageProjectRequirements::StringToOperator(node.GetAttribute("Operator"), false, operatorType);
		}
		array.emplace_back(KPPCFlagEntry(node.GetValue(), node.GetAttribute("Name"), operatorType));
	}
}
template<class T> static void WriteLabeledValueArray(const KLabeledValueArray& array, KxXMLNode& arrayNode, const T& Func, bool isCDATA = false)
{
	arrayNode.ClearChildren();
	for (const KLabeledValue& value: array)
	{
		KxXMLNode elementNode = arrayNode.NewElement("Entry");

		elementNode.SetValue(Func(value), isCDATA);
		if (value.HasLabel())
		{
			elementNode.SetAttribute("Name", value.GetLabel());
		}
	}
}

void KPackageProjectSerializerKMP::ReadBase()
{
	KxXMLNode baseNode = m_XML.QueryElement("Package");
	if (baseNode.IsOK())
	{
		m_ProjectLoad->SetFormatVersion(baseNode.GetAttribute("FormatVersion"));
		m_ProjectLoad->SetModID(baseNode.GetAttribute("ID"));
		
		KxXMLNode targetProfileNode = baseNode.GetFirstChildElement("TargetProfile");
		m_ProjectLoad->SetTargetProfileID(targetProfileNode.GetAttribute("ID"));
	}
}
void KPackageProjectSerializerKMP::ReadConfig()
{
	KxXMLNode configNode = m_XML.QueryElement("Package/PackageConfig");
	if (configNode.IsOK())
	{
		KPackageProjectConfig& config = m_ProjectLoad->GetConfig();

		config.SetInstallPackageFile(configNode.GetFirstChildElement("InstallPackageFile").GetValue());
		config.SetCompressionMethod(configNode.GetFirstChildElement("CompressionMethod").GetValue());
		config.SetCompressionLevel(configNode.GetFirstChildElement("CompressionLevel").GetValueInt());
		config.SetCompressionDictionarySize(configNode.GetFirstChildElement("CompressionDictionarySize").GetValueInt());
		config.SetUseMultithreading(configNode.GetFirstChildElement("CompressionUseMultithreading").GetValueBool());
		config.SetSolidArchive(configNode.GetFirstChildElement("CompressionSolidArchive").GetValueBool());
	}
}
void KPackageProjectSerializerKMP::ReadInfo()
{
	KxXMLNode infoNode = m_XML.QueryElement("Package/Info");
	if (infoNode.IsOK())
	{
		KPackageProjectInfo& info = m_ProjectLoad->GetInfo();

		// Basic info
		info.SetName(infoNode.GetFirstChildElement("Name").GetValue());
		info.SetTranslatedName(infoNode.GetFirstChildElement("TranslatedName").GetValue());
		info.SetVersion(infoNode.GetFirstChildElement("Version").GetValue());
		info.SetAuthor(infoNode.GetFirstChildElement("Author").GetValue());
		info.SetTranslator(infoNode.GetFirstChildElement("Translator").GetValue());
		info.SetDescription(infoNode.GetFirstChildElement("Description").GetValue());

		// Custom info
		KAux::LoadLabeledValueArray(info.GetCustomFields(), infoNode.GetFirstChildElement("Custom"));

		// Web-sites
		KxXMLNode sitesNode = infoNode.GetFirstChildElement("Sites");
		info.SetWebSite(KNETWORK_PROVIDER_ID_NEXUS, sitesNode.GetAttributeInt("NexusID", -1));
		info.SetWebSite(KNETWORK_PROVIDER_ID_TESALL, sitesNode.GetAttributeInt("TESALLID", -1));
		info.SetWebSite(KNETWORK_PROVIDER_ID_LOVERSLAB, sitesNode.GetAttributeInt("LoversLabID", -1));
		KAux::LoadLabeledValueArray(info.GetWebSites(), sitesNode, "Name");

		// Documents
		KAux::LoadLabeledValueArray(info.GetDocuments(), infoNode.GetFirstChildElement("Documents"), "Name");

		// Tags
		KAux::LoadStringArray(info.GetTags(), infoNode.GetFirstChildElement("Tags"));
	}
}
void KPackageProjectSerializerKMP::ReadInterface()
{
	KxXMLNode interfaceNode = m_XML.QueryElement("Package/Interface");
	if (interfaceNode.IsOK())
	{
		KPackageProjectInterface& interfaceConfig = m_ProjectLoad->GetInterface();
		KPPITitleConfig& titleConfig = interfaceConfig.GetTitleConfig();

		// Read customization
		KxXMLNode titleConfigNode = interfaceNode.GetFirstChildElement("Caption");
		if (titleConfigNode.IsOK())
		{
			titleConfig.SetAlignment((wxAlignment)titleConfigNode.GetAttributeInt("Alignment", KPPITitleConfig::ms_InvalidAlignment));
			
			int64_t colorValue = titleConfigNode.GetAttributeInt("Color", -1);
			if (colorValue != -1)
			{
				titleConfig.SetColor(KxColor::FromRGBA(colorValue));
			}
		}

		// Read special images config
		auto ReadImageConfig = [](const KxXMLNode& node) -> KPPIImageEntry
		{
			KPPIImageEntry entry;
			if (node.IsOK())
			{
				entry.SetPath(node.GetAttribute("Path"));
				entry.SetVisible(node.GetAttributeBool("Visible", true));
				entry.SetSize(wxSize(node.GetAttributeInt("Width", wxDefaultCoord), node.GetAttributeInt("Height", wxDefaultCoord)));
				entry.SetDescription(node.GetFirstChildElement("Description").GetValue());
			}
			return entry;
		};
		interfaceConfig.SetMainImage(ReadImageConfig(interfaceNode.GetFirstChildElement("MainImage")).GetPath());
		interfaceConfig.SetHeaderImage(ReadImageConfig(interfaceNode.GetFirstChildElement("HeaderImage")).GetPath());

		// Read images list
		KxXMLNode imagesNode = interfaceNode.GetFirstChildElement("Images");
		for (KxXMLNode node = imagesNode.GetFirstChildElement(); node.IsOK(); node = node.GetNextSiblingElement())
		{
			interfaceConfig.GetImages().emplace_back(ReadImageConfig(node));
		}
	}
}
void KPackageProjectSerializerKMP::ReadFileData()
{
	KxXMLNode fileDataNode = m_XML.QueryElement("Package/Files");
	if (fileDataNode.IsOK())
	{
		KPackageProjectFileData& fileData = m_ProjectLoad->GetFileData();

		// Folder
		for (KxXMLNode folderNode = fileDataNode.GetFirstChildElement(); folderNode.IsOK(); folderNode = folderNode.GetNextSiblingElement())
		{
			KPPFFileEntry* fileEntry = NULL;
			KPPFFolderEntry* folderEntry = NULL;
			if (folderNode.GetName() == "Folder")
			{
				folderEntry = new KPPFFolderEntry();
				fileEntry = fileData.AddFolder(folderEntry);
			}
			else
			{
				fileEntry = fileData.AddFile(new KPPFFileEntry());
			}

			fileEntry->SetID(folderNode.GetAttribute("ID"));
			fileEntry->SetSource(folderNode.GetAttribute("Source"));
			fileEntry->SetDestination(folderNode.GetAttribute("Destination"));
			fileEntry->SetPriority(folderNode.GetAttributeInt("Priority", KPackageProjectFileData::ms_DefaultPriority));

			if (m_AsProject && folderEntry)
			{
				for (KxXMLNode fileNode = folderNode.GetFirstChildElement(); fileNode.IsOK(); fileNode = fileNode.GetNextSiblingElement())
				{
					KPPFFolderEntryItem& fileEntry = folderEntry->GetFiles().emplace_back(KPPFFolderEntryItem());
					fileEntry.SetDestination(fileNode.GetValue());
					fileEntry.SetSource(fileNode.GetAttribute("Source"));
				}
			}
		}
	}
}
void KPackageProjectSerializerKMP::ReadRequirements()
{
	KPackageProjectRequirements& requirements = m_ProjectLoad->GetRequirements();

	KxXMLNode requirementsNode = m_XML.QueryElement("Package/Requirements");
	if (requirementsNode.IsOK())
	{
		KAux::LoadStringArray(requirements.GetDefaultGroup(), requirementsNode.GetFirstChildElement("DefaultGroups"));

		for (KxXMLNode groupNode = requirementsNode.GetFirstChildElement("Groups").GetFirstChildElement(); groupNode.IsOK(); groupNode = groupNode.GetNextSiblingElement())
		{
			KPPRRequirementsGroup* requirementGroup = requirements.GetGroups().emplace_back(new KPPRRequirementsGroup()).get();
			requirementGroup->SetID(groupNode.GetAttribute("ID"));

			for (KxXMLNode entryNode = groupNode.GetFirstChildElement(); entryNode.IsOK(); entryNode = entryNode.GetNextSiblingElement())
			{
				KPPRTypeDescriptor type = requirements.StringToTypeDescriptor(entryNode.GetAttribute("Type"));

				KPPRRequirementEntry* entry = requirementGroup->GetEntries().emplace_back(new KPPRRequirementEntry(type)).get();
				entry->SetID(entryNode.GetAttribute("ID"));
				entry->SetOperator(requirements.StringToOperator(entryNode.GetAttribute("Operator"), requirements.ms_DefaultEntryOperator));
				entry->SetName(entryNode.GetFirstChildElement("Name").GetValue());

				// Object
				KxXMLNode objectNode = entryNode.GetFirstChildElement("Object");
				entry->SetObject(objectNode.GetValue());
				entry->SetObjectFunction(requirements.StringToObjectFunction(objectNode.GetAttribute("Function")));

				// Version
				KxXMLNode versionNode = entryNode.GetFirstChildElement("Version");
				entry->SetRequiredVersion(versionNode.GetValue());
				entry->SetRVFunction(requirements.StringToOperator(versionNode.GetAttribute("Function"), requirements.ms_DefaultVersionOperator));

				// Description
				entry->SetDescription(entryNode.GetFirstChildElement("Description").GetValue());

				// Conform
				entry->ConformToTypeDescriptor();
			}
		}
	}
}
void KPackageProjectSerializerKMP::ReadComponents()
{
	KPackageProjectComponents& components = m_ProjectLoad->GetComponents();

	KxXMLNode componentsNode = m_XML.QueryElement("Package/Components");
	if (componentsNode.IsOK())
	{
		// Read required files
		KAux::LoadStringArray(components.GetRequiredFileData(), componentsNode.GetFirstChildElement("RequiredFiles"));

		// Read steps
		for (KxXMLNode stepNode = componentsNode.GetFirstChildElement("Steps").GetFirstChildElement(); stepNode.IsOK(); stepNode = stepNode.GetNextSiblingElement())
		{
			auto& step = components.GetSteps().emplace_back(new KPPCStep());
			step->SetName(stepNode.GetAttribute("Name"));
			ReadKPPCFlagEntryArray(step->GetConditions(), stepNode.GetFirstChildElement("Conditions"), true);

			for (KxXMLNode groupNode = stepNode.GetFirstChildElement("Groups").GetFirstChildElement(); groupNode.IsOK(); groupNode = groupNode.GetNextSiblingElement())
			{
				auto& pSet = step->GetGroups().emplace_back(new KPPCGroup());
				pSet->SetName(groupNode.GetAttribute("Name"));
				pSet->SetSelectionMode(components.StringToSelectionMode(groupNode.GetAttribute("SelectionMode")));

				for (KxXMLNode entryNode = groupNode.GetFirstChildElement("Entries").GetFirstChildElement(); entryNode.IsOK(); entryNode = entryNode.GetNextSiblingElement())
				{
					auto& entry = pSet->GetEntries().emplace_back(new KPPCEntry());
					entry->SetName(entryNode.GetFirstChildElement("Name").GetValue());
					entry->SetImage(entryNode.GetFirstChildElement("Image").GetAttribute("Path"));
					entry->SetDescription(entryNode.GetFirstChildElement("Description").GetValue());

					// Type descriptor
					KxXMLNode typeDescriptorNode = entryNode.GetFirstChildElement("TypeDescriptor");
					entry->SetTDDefaultValue(components.StringToTypeDescriptor(typeDescriptorNode.GetAttribute("DefaultValue")));
					entry->SetTDConditionalValue(components.StringToTypeDescriptor(typeDescriptorNode.GetAttribute("ConditionalValue"), KPPC_DESCRIPTOR_INVALID));
					ReadKPPCFlagEntryArray(entry->GetTDConditions(), typeDescriptorNode.GetFirstChildElement("Conditions"), true);

					// If condition list is empty and type descriptor values are equal, clear 'ConditionalValue'
					if (entry->GetTDConditions().empty() && entry->GetTDDefaultValue() == entry->GetTDConditionalValue())
					{
						entry->SetTDConditionalValue(KPPC_DESCRIPTOR_INVALID);
					}

					KAux::LoadStringArray(entry->GetFileData(), entryNode.GetFirstChildElement("Files"));
					KAux::LoadStringArray(entry->GetRequirements(), entryNode.GetFirstChildElement("Requirements"));
					ReadKPPCFlagEntryArray(entry->GetAssignedFlags(), entryNode.GetFirstChildElement("AssignedFlags"), false);
				}
			}
		}

		auto ReadConditionalSteps = [&componentsNode, &components](const wxString& sRootNodeName, const wxString& sNodeName)
		{
			for (KxXMLNode stepNode = componentsNode.GetFirstChildElement(sRootNodeName).GetFirstChildElement(); stepNode.IsOK(); stepNode = stepNode.GetNextSiblingElement())
			{
				auto& step = components.GetConditionalSteps().emplace_back(new KPPCConditionalStep());
				ReadKPPCFlagEntryArray(step->GetConditions(), stepNode.GetFirstChildElement("Conditions"), true);
				KAux::LoadStringArray(step->GetEntries(), stepNode.GetFirstChildElement(sNodeName));
			}
		};
		ReadConditionalSteps("ConditionalSteps", "Files");
	}
}

KxXMLNode KPackageProjectSerializerKMP::WriteBase()
{
	KxXMLNode baseNode = m_XML.NewElement("Package");
	baseNode.SetAttribute("FormatVersion", KPackageManager::GetInstance()->GetVersion());
	baseNode.SetAttribute("ID", m_ProjectSave->GetModID());

	KxXMLNode targetProfileNode = baseNode.NewElement("TargetProfile");
	targetProfileNode.SetAttribute("ID", KApp::Get().GetCurrentGameID());

	return baseNode;
}
void KPackageProjectSerializerKMP::WriteConfig(KxXMLNode& baseNode)
{
	if (m_AsProject)
	{
		KxXMLNode configNode = baseNode.NewElement("PackageConfig");
		const KPackageProjectConfig& config = m_ProjectSave->GetConfig();

		configNode.NewElement("InstallPackageFile").SetValue(config.GetInstallPackageFile());
		configNode.NewElement("CompressionMethod").SetValue(config.GetCompressionMethod());
		configNode.NewElement("CompressionLevel").SetValue(config.GetCompressionLevel());
		configNode.NewElement("CompressionDictionarySize").SetValue(config.GetCompressionDictionarySize());
		configNode.NewElement("CompressionUseMultithreading").SetValue(config.IsMultithreadingUsed());
		configNode.NewElement("CompressionSolidArchive").SetValue(config.IsSolidArchive());
	}
}
void KPackageProjectSerializerKMP::WriteInfo(KxXMLNode& baseNode)
{
	KxXMLNode infoNode = baseNode.NewElement("Info");
	const KPackageProjectInfo& info = m_ProjectSave->GetInfo();

	// Basic info
	infoNode.NewElement("Name").SetValue(info.GetName());
	infoNode.NewElement("Version").SetValue(info.GetVersion());
	infoNode.NewElement("Author").SetValue(info.GetAuthor());

	if (!info.GetTranslator().IsEmpty())
	{
		infoNode.NewElement("Translator").SetValue(info.GetTranslator());
	}

	if (!info.GetTranslatedName().IsEmpty())
	{
		infoNode.NewElement("TranslatedName").SetValue(info.GetTranslatedName());
	}

	infoNode.NewElement("Description").SetValue(info.GetDescription(), true);

	// Custom info
	if (!info.GetCustomFields().empty())
	{
		KAux::SaveLabeledValueArray(info.GetCustomFields(), infoNode.NewElement("Custom"));
	}

	// Web-sites
	KxXMLNode sitesNode = infoNode.NewElement("Sites");
	auto WriteFixedSite = [&sitesNode, info](const char* name, KNetworkProviderID index)
	{
		if (info.HasWebSite(index))
		{
			sitesNode.SetAttribute(name, info.GetWebSiteModID(index));
		}
	};
	WriteFixedSite("NexusID", KNETWORK_PROVIDER_ID_NEXUS);
	WriteFixedSite("TESALLID", KNETWORK_PROVIDER_ID_TESALL);
	WriteFixedSite("LoversLabID", KNETWORK_PROVIDER_ID_LOVERSLAB);
	KAux::SaveLabeledValueArray(info.GetWebSites(), sitesNode, "Name");

	// Documents
	if (!info.GetDocuments().empty())
	{
		WriteLabeledValueArray(info.GetDocuments(), infoNode.NewElement("Documents"), [this](const KLabeledValue& value)
		{
			return m_AsProject ? value.GetValue() : PathNameToPackage(value.GetValue(), KPP_CONTENT_DOCUMENTS);
		});
	}

	// Tags
	if (!info.GetTags().empty())
	{
		KAux::SaveStringArray(info.GetTags(), infoNode.NewElement("Tags"));
	}
}
void KPackageProjectSerializerKMP::WriteInterface(KxXMLNode& baseNode)
{
	const KPackageProjectInterface& interfaceConfig = m_ProjectSave->GetInterface();
	const KPPITitleConfig& titleConfig = interfaceConfig.GetTitleConfig();
	KxXMLNode interfaceNode = baseNode.NewElement("Interface");

	// Write customization
	if (titleConfig.IsOK())
	{
		KxXMLNode node = interfaceNode.NewElement("Caption");
		if (titleConfig.HasAlignment())
		{
			node.SetAttribute("Alignment", (int64_t)titleConfig.GetAlignment());
		}
		if (titleConfig.HasColor())
		{
			node.SetAttribute("Color", (int64_t)titleConfig.GetColor().GetRGBA());
		}
	}

	// Write special images config
	auto WriteImageConfig = [this](KxXMLNode& rootNode, const wxString& name, const KPPIImageEntry* entry, bool isListEntry)
	{
		if (entry)
		{
			if (!isListEntry || (isListEntry && entry->HasPath()))
			{
				KxXMLNode node = rootNode.NewElement(name);
				node.SetAttribute("Path", m_AsProject ? entry->GetPath() : PathNameToPackage(entry->GetPath(), KPP_CONTENT_IMAGES));
				node.SetAttribute("Visible", entry->IsVisible());

				if (isListEntry)
				{
					if (entry->HasDescription())
					{
						node.NewElement("Description").SetValue(entry->GetDescriptionRaw(), true);
					}
				}
				else
				{
					//node.SetAttribute("Width", entry->GetSize().GetWidth());
					//node.SetAttribute("Height", entry->GetSize().GetHeight());
				}
			}
		}
	};
	WriteImageConfig(interfaceNode, "MainImage", interfaceConfig.GetMainImageEntry(), false);
	WriteImageConfig(interfaceNode, "HeaderImage", interfaceConfig.GetHeaderImageEntry(), false);

	// Write images list
	if (!interfaceConfig.GetImages().empty())
	{
		KxXMLNode imagesNode = interfaceNode.NewElement("Images");
		for (const KPPIImageEntry& entry: interfaceConfig.GetImages())
		{
			WriteImageConfig(imagesNode, "Entry", &entry, true);
		}
	}
}
void KPackageProjectSerializerKMP::WriteFileData(KxXMLNode& baseNode)
{
	KxXMLNode fileDataNode = baseNode.NewElement("Files");
	const KPackageProjectFileData& fileData = m_ProjectSave->GetFileData();

	// Folders
	if (!fileData.GetData().empty())
	{
		for (const auto& entry: fileData.GetData())
		{
			const KPPFFolderEntry* folderEntry = entry->ToFolderEntry();
			KxXMLNode entryNode = fileDataNode.NewElement(folderEntry ? "Folder" : "File");

			if (m_AsProject)
			{
				if (entry->GetID() != entry->GetSource())
				{
					entryNode.SetAttribute("ID", entry->GetID());
				}
				entryNode.SetAttribute("Source", PathNameToPackage(entry->GetSource(), KPP_CONTENT_FILEDATA));
			}
			else
			{
				entryNode.SetAttribute("Source", entry->GetID());
			}
			entryNode.SetAttribute("Destination", entry->GetDestination());

			if (!entry->IsDefaultPriority())
			{
				entryNode.SetAttribute("Priority", entry->GetPriority());
			}

			if (m_AsProject && folderEntry && !folderEntry->GetFiles().empty())
			{
				for (const KPPFFolderEntryItem& fileEntry: folderEntry->GetFiles())
				{
					KxXMLNode fileEntryNode = entryNode.NewElement("Entry");
					fileEntryNode.SetValue(fileEntry.GetDestination());
					fileEntryNode.SetAttribute("Source", fileEntry.GetSource());
				}
			}
		}
	}
}
void KPackageProjectSerializerKMP::WriteRequirements(KxXMLNode& baseNode)
{
	const KPackageProjectRequirements& requirements = m_ProjectSave->GetRequirements();
	KxXMLNode requirementsNode = baseNode.NewElement("Requirements");
	if (!requirements.IsDefaultGroupEmpty())
	{
		KAux::SaveStringArray(requirements.GetDefaultGroup(), requirementsNode.NewElement("DefaultGroups"));
	}

	if (!requirements.GetGroups().empty())
	{
		KxXMLNode groupsArrayNode = requirementsNode.NewElement("Groups");
		for (const auto& group: requirements.GetGroups())
		{
			KxXMLNode requirementsGroupNode = groupsArrayNode.NewElement("Group");
			requirementsGroupNode.SetAttribute("ID", group->GetID());

			if (!group->GetEntries().empty())
			{
				for (const auto& entry: group->GetEntries())
				{
					KxXMLNode entryNode = requirementsGroupNode.NewElement("Entry");
					if (!entry->GetID().IsEmpty())
					{
						entryNode.SetAttribute("ID", entry->GetID());
					}
					entryNode.SetAttribute("Type", requirements.TypeDescriptorToString(entry->GetTypeDescriptor()));
					entryNode.SetAttribute("Operator", requirements.OperatorToString(entry->GetOperator()));

					entryNode.NewElement("Name").SetValue(entry->GetName());

					// Object
					KxXMLNode objectNode = entryNode.NewElement("Object");
					objectNode.SetValue(entry->GetObject());
					objectNode.SetAttribute("Function", requirements.ObjectFunctionToString(entry->GetObjectFunction()));

					// Version
					KxXMLNode versionNode = entryNode.NewElement("Version");
					versionNode.SetValue(entry->GetRequiredVersion());
					versionNode.SetAttribute("Function", requirements.OperatorToString(entry->GetRVFunction()));

					// Description
					if (!entry->GetDescription().IsEmpty())
					{
						entryNode.NewElement("Description").SetValue(entry->GetDescription(), true);
					}
				}
			}
		}
	}
}
void KPackageProjectSerializerKMP::WriteComponents(KxXMLNode& baseNode)
{
	KxXMLNode componentsNode = baseNode.NewElement("Components");
	const KPackageProjectComponents& components = m_ProjectSave->GetComponents();

	// Write required files
	if (!components.GetRequiredFileData().empty())
	{
		KAux::SaveStringArray(components.GetRequiredFileData(), componentsNode.NewElement("RequiredFiles"));
	}

	// Write steps
	if (!components.GetSteps().empty())
	{
		KxXMLNode stepsArrayNode = componentsNode.NewElement("Steps");
		for (const auto& step: components.GetSteps())
		{
			/* Header */
			KxXMLNode stepNode = stepsArrayNode.NewElement("Step");
			if (!step->IsEmptyName())
			{
				stepNode.SetAttribute("Name", step->GetName());
			}

			/* Step conditions */
			if (!step->GetConditions().empty())
			{
				WriteKPPCFlagEntryArray(step->GetConditions(), stepNode.NewElement("Conditions"), true);
			}
			

			/* Groups */
			if (!step->GetGroups().empty())
			{
				KxXMLNode groupsArrayNode = stepNode.NewElement("Groups");
				for (const auto& group: step->GetGroups())
				{
					KxXMLNode groupNode = groupsArrayNode.NewElement("Group");
					if (!group->IsEmptyName())
					{
						groupNode.SetAttribute("Name", group->GetName());
					}
					groupNode.SetAttribute("SelectionMode", components.SelectionModeToString(group->GetSelectionMode()));

					// Group entries
					if (!group->GetEntries().empty())
					{
						KxXMLNode tEntriesArrayNode = groupNode.NewElement("Entries");
						for (const auto& entry: group->GetEntries())
						{
							KxXMLNode entryNode = tEntriesArrayNode.NewElement("Entry");

							// Name is required
							entryNode.NewElement("Name").SetValue(entry->GetName());

							// Image
							if (!entry->GetImage().IsEmpty())
							{
								wxString image = m_AsProject ? entry->GetImage() : PathNameToPackage(entry->GetImage(), KPP_CONTENT_IMAGES);
								entryNode.NewElement("Image").SetAttribute("Path", image);
							}

							// Description
							if (!entry->GetDescription().IsEmpty())
							{
								entryNode.NewElement("Description").SetValue(entry->GetDescription(), true);
							}

							// Type descriptor
							KxXMLNode typeDescriptorNode = entryNode.NewElement("TypeDescriptor");
							typeDescriptorNode.SetAttribute("DefaultValue", components.TypeDescriptorToString(entry->GetTDDefaultValue()));
							if (entry->GetTDConditionalValue() != KPPC_DESCRIPTOR_INVALID)
							{
								typeDescriptorNode.SetAttribute("ConditionalValue", components.TypeDescriptorToString(entry->GetTDConditionalValue()));
							}

							if (!entry->GetTDConditions().empty())
							{
								WriteKPPCFlagEntryArray(entry->GetTDConditions(), typeDescriptorNode.NewElement("Conditions"), true);
							}

							if (!entry->GetFileData().empty())
							{
								KAux::SaveStringArray(entry->GetFileData(), entryNode.NewElement("Files"));
							}

							if (!entry->GetRequirements().empty())
							{
								KAux::SaveStringArray(entry->GetRequirements(), entryNode.NewElement("Requirements"));
							}

							if (!entry->GetAssignedFlags().empty())
							{
								WriteKPPCFlagEntryArray(entry->GetAssignedFlags(), entryNode.NewElement("AssignedFlags"), false);
							}
						}
					}
				}
			}
		}
	}

	auto WriteConditionalSteps = [&componentsNode](const KPPCConditionalStepArray& steps, const wxString& sRootNodeName, const wxString& sNodeName)
	{
		if (!steps.empty())
		{
			KxXMLNode stepsArrayNode = componentsNode.NewElement(sRootNodeName);
			for (const auto& step: steps)
			{
				/* Header */
				KxXMLNode setNode = stepsArrayNode.NewElement("Step");
				if (!step->GetConditions().empty())
				{
					WriteKPPCFlagEntryArray(step->GetConditions(), setNode.NewElement("Conditions"), true);
				}

				/* Entries */
				if (!step->GetEntries().empty())
				{
					KAux::SaveStringArray(step->GetEntries(), setNode.NewElement(sNodeName));
				}
			}
		}
	};
	WriteConditionalSteps(components.GetConditionalSteps(), "ConditionalSteps", "Files");
}

void KPackageProjectSerializerKMP::Serialize(const KPackageProject* project)
{
	m_ProjectSave = project;
	m_XML.Load(wxEmptyString);

	KxXMLNode baseNode = WriteBase();
	WriteConfig(baseNode);
	WriteInfo(baseNode);
	WriteFileData(baseNode);
	WriteInterface(baseNode);
	WriteRequirements(baseNode);
	WriteComponents(baseNode);

	m_Data = m_XML.Save();
}
void KPackageProjectSerializerKMP::Structurize(KPackageProject* project)
{
	m_ProjectLoad = project;
	m_XML.Load(m_Data);

	ReadBase();
	ReadConfig();
	ReadInfo();
	ReadFileData();
	ReadInterface();
	ReadRequirements();
	ReadComponents();
}
