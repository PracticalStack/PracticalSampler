#include <JuceHeader.h>

using namespace juce;

ValueTree buildApiValueTree(const var& jsonData)
{
	ValueTree root("Api");

	auto* classesObj = jsonData.getProperty("classes", {}).getDynamicObject();
	if (classesObj == nullptr)
		return root;

	for (auto& classPair : classesObj->getProperties())
	{
		auto className = classPair.name.toString();
		auto* classData = classPair.value.getDynamicObject();
		if (classData == nullptr) continue;

		ValueTree classNode(className);

		auto* methodsObj = classData->getProperty("methods").getDynamicObject();
		if (methodsObj == nullptr)
		{
			root.addChild(classNode, -1, nullptr);
			continue;
		}

		for (auto& methodPair : methodsObj->getProperties())
		{
			auto* methodData = methodPair.value.getDynamicObject();
			if (methodData == nullptr) continue;

			ValueTree methodNode("method");

			// Copy all properties from the JSON method object into the ValueTree.
			// The Python filter-binary stage already curates exactly which fields
			// belong in the blob, so no filtering is needed here.
			for (auto& prop : methodData->getProperties())
				methodNode.setProperty(prop.name, prop.value, nullptr);

			classNode.addChild(methodNode, -1, nullptr);
		}

		root.addChild(classNode, -1, nullptr);
	}

	return root;
}

void writeBinaryHeader(const File& outputFile, const String& namespaceName,
                       const String& varName, int dataSize)
{
	String guard = "BINARY_" + namespaceName.toUpperCase() + "_H";

	String h;
	h << "/* (Auto-generated binary data file). */\n\n";
	h << "#ifndef " << guard << "\n";
	h << "#define " << guard << "\n\n";
	h << "namespace " << namespaceName << "\n{\n";
	h << "    extern const char*  " << varName << ";\n";
	h << "    const int           " << varName << "Size = " << dataSize << ";\n";
	h << "}\n\n";
	h << "#endif\n";

	outputFile.replaceWithText(h);
}

void writeBinarySource(const File& outputFile, const String& namespaceName,
                       const String& headerName, const String& varName,
                       const void* data, int dataSize)
{
	MemoryOutputStream cpp;
	cpp << "/* (Auto-generated binary data file). */\n\n";
	cpp << "#include \"" << headerName << "\"\n\n";
	cpp << "static const unsigned char temp1[] = {";

	auto* bytes = static_cast<const unsigned char*>(data);

	for (int i = 0; i < dataSize; i++)
	{
		if (i > 0) cpp << ",";
		if (i % 40 == 0) cpp << "\n  ";
		cpp << (int)bytes[i];
	}

	cpp << "};\n";
	cpp << "const char* " << namespaceName << "::" << varName
	    << " = (const char*) temp1;\n";

	outputFile.replaceWithText(cpp.toString());
}

int main(int argc, char* argv[])
{
	if (argc < 4)
	{
		std::cout << "Usage: ApiValueTreeBuilder <input.json> <output_dir> <namespace>"
		          << std::endl;
		std::cout << "Example: ApiValueTreeBuilder filtered_api.json "
		          << "\"../../hi_scripting/scripting/api\" XmlApi" << std::endl;
		return 1;
	}

	File inputFile(argv[1]);
	File outputDir(argv[2]);
	String namespaceName(argv[3]);

	if (!inputFile.existsAsFile())
	{
		std::cerr << "ERROR: Input file not found: "
		          << inputFile.getFullPathName() << std::endl;
		return 1;
	}

	if (!outputDir.isDirectory())
	{
		std::cerr << "ERROR: Output directory not found: "
		          << outputDir.getFullPathName() << std::endl;
		return 1;
	}

	// Parse JSON
	auto jsonText = inputFile.loadFileAsString();
	auto jsonData = JSON::parse(jsonText);

	if (!jsonData.isObject())
	{
		std::cerr << "ERROR: Failed to parse JSON from "
		          << inputFile.getFullPathName() << std::endl;
		return 1;
	}

	// Build ValueTree
	auto tree = buildApiValueTree(jsonData);

	// Count stats
	int classCount = tree.getNumChildren();
	int methodCount = 0;
	int callScopeCount = 0;
	int deprecatedCount = 0;

	for (int i = 0; i < classCount; i++)
	{
		auto classNode = tree.getChild(i);
		for (int j = 0; j < classNode.getNumChildren(); j++)
		{
			methodCount++;
			if (classNode.getChild(j).hasProperty("callScope"))
				callScopeCount++;
			if (classNode.getChild(j).hasProperty("deprecated"))
				deprecatedCount++;
		}
	}

	// Serialize to binary
	MemoryOutputStream mos;
	tree.writeToStream(mos);

	int dataSize = (int)mos.getDataSize();
	String varName = "apivaluetree_dat";

	// Write output files
	File headerFile = outputDir.getChildFile(namespaceName + ".h");
	File sourceFile = outputDir.getChildFile(namespaceName + ".cpp");

	writeBinaryHeader(headerFile, namespaceName, varName, dataSize);
	writeBinarySource(sourceFile, namespaceName, namespaceName + ".h",
	                  varName, mos.getData(), dataSize);

	std::cout << "ApiValueTreeBuilder complete:" << std::endl;
	std::cout << "  Classes: " << classCount << std::endl;
	std::cout << "  Methods: " << methodCount << std::endl;
	std::cout << "  With callScope: " << callScopeCount << std::endl;
	std::cout << "  Deprecated: " << deprecatedCount << std::endl;
	std::cout << "  Binary size: " << dataSize << " bytes" << std::endl;
	std::cout << "  Header: " << headerFile.getFullPathName() << std::endl;
	std::cout << "  Source: " << sourceFile.getFullPathName() << std::endl;

	return 0;
}
