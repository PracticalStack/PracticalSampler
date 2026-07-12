/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once


//==============================================================================
constexpr auto* macMakeArch_Default   = "Universal";
constexpr auto* macMakeArch_Native    = "Native";
constexpr auto* macMakeArch_Intel     = "x86_64";
constexpr auto* macMakeArch_Arm       = "arm64";
constexpr auto* macMakeArch_Universal = "Universal";

//==============================================================================
class MacOSMakefileProjectExporter  : public MakefileProjectExporter
{
public:
    //==============================================================================
    class MacMakeBuildConfiguration  : public MakeBuildConfiguration
    {
    public:
        MacMakeBuildConfiguration (Project& p, const ValueTree& s, const ProjectExporter& e)
            : MakeBuildConfiguration (p, s, e),
              macOSBaseSDK          (config, Ids::macOSBaseSDK,          getUndoManager()),
              macOSDeploymentTarget (config, Ids::macOSDeploymentTarget, getUndoManager(), "11.0"),
              macOSArchitecture     (config, Ids::osxArchitecture,       getUndoManager(), macMakeArch_Default),
              stripLocalSymbolsEnabled (config, Ids::stripLocalSymbols, getUndoManager(), ! isDebug())
        {
        }

        void createConfigProperties (PropertyListBuilder& props) override
        {
            addRecommendedLinuxCompilerWarningsProperty (props);
            addGCCOptimisationProperty (props);

            props.add (new TextPropertyComponent (macOSBaseSDK, "macOS Base SDK", 8, false),
                       "Leave blank to use the Xcode default SDK, or set to a version like \"14.0\" to target a specific SDK. "
                       "When blank, the Makefile will invoke `xcrun --show-sdk-path` at build time.");

            props.add (new TextPropertyComponent (macOSDeploymentTarget, "macOS Deployment Target", 8, false),
                       "Sets the -mmacosx-version-min flag, e.g. \"11.0\".");

            props.add (new ChoicePropertyComponent (macOSArchitecture, "Architecture",
                                                    { "Native (build machine)", "Intel (x86_64)",  "Apple Silicon (arm64)", "Universal (x86_64 + arm64)" },
                                                    { macMakeArch_Native,       macMakeArch_Intel, macMakeArch_Arm,         macMakeArch_Universal }),
                       "Target architecture(s) to build for. Universal emits a fat Mach-O binary in a single clang invocation.");

            props.add (new ChoicePropertyComponent (stripLocalSymbolsEnabled, "Strip Local Symbols"),
                       "Enable to run `strip --strip-unneeded` on the final binary after linking. Defaults to on for release and off for debug configs.");

            auto isBuildingAnyPlugins = (project.shouldBuildVST3() || project.shouldBuildAU() || project.shouldBuildAAX());

            if (isBuildingAnyPlugins)
            {
                props.add (new ChoicePropertyComponent (pluginBinaryCopyStepValue, "Enable Plugin Copy Step"),
                           "Enable this to copy plugin binaries to a specified folder after building.");

                if (project.shouldBuildVST3())
                    props.add (new TextPropertyComponentWithEnablement (vst3BinaryLocation, pluginBinaryCopyStepValue, "VST3 Binary Location",
                                                                        1024, false),
                               "The folder in which the compiled VST3 bundle should be placed.");

                if (project.shouldBuildAU())
                    props.add (new TextPropertyComponentWithEnablement (auBinaryLocation, pluginBinaryCopyStepValue, "AU Binary Location",
                                                                        1024, false),
                               "The folder in which the compiled AU bundle should be placed.");

                if (project.shouldBuildAAX())
                    props.add (new TextPropertyComponentWithEnablement (aaxBinaryLocation, pluginBinaryCopyStepValue, "AAX Binary Location",
                                                                        1024, false),
                               "The folder in which the compiled AAX bundle should be placed.");
            }
        }

        String getMacOSBaseSDKString() const          { return macOSBaseSDK.get(); }
        String getMacOSDeploymentTargetString() const { return macOSDeploymentTarget.get(); }
        String getMacOSArchitectureString() const     { return macOSArchitecture.get(); }

        String getUserPrecompiledHeaderPath() const   { return config[Ids::precompiledHeaderFile].toString(); }

        String getArchFlags() const
        {
            auto arch = getMacOSArchitectureString();

            if (arch == macMakeArch_Native)        return {};
            if (arch == macMakeArch_Intel)         return "-arch x86_64";
            if (arch == macMakeArch_Arm)           return "-arch arm64";
            if (arch == macMakeArch_Universal)     return "-arch x86_64 -arch arm64";

            return {};
        }

        String getModuleLibraryArchName() const override
        {
            auto arch = getMacOSArchitectureString();

            if (arch == macMakeArch_Intel)     return "x86_64";
            if (arch == macMakeArch_Arm)       return "arm64";
            if (arch == macMakeArch_Universal) return "universal";

            return "${JUCE_ARCH_LABEL}";
        }

        String getAUBinaryLocationString()   const { return auBinaryLocation.get(); }
        String getAAXBinaryLocationString()  const { return aaxBinaryLocation.get(); }

        bool isStripLocalSymbolsEnabled() const { return stripLocalSymbolsEnabled.get(); }

    private:
        ValueWithDefault macOSBaseSDK, macOSDeploymentTarget, macOSArchitecture, stripLocalSymbolsEnabled;
        ValueWithDefault auBinaryLocation  { config, Ids::auBinaryLocation,  getUndoManager(), "$(HOME)/Library/Audio/Plug-Ins/Components" };
        ValueWithDefault aaxBinaryLocation { config, Ids::aaxBinaryLocation, getUndoManager(), "/Library/Application Support/Avid/Audio/Plug-Ins" };
    };

    BuildConfiguration::Ptr createBuildConfig (const ValueTree& tree) const override
    {
        return *new MacMakeBuildConfiguration (project, tree, *this);
    }

    //==============================================================================
    class MacMakefileTarget  : public MakefileTarget
    {
    public:
        MacMakefileTarget (build_tools::ProjectType::Target::Type t, const MakefileProjectExporter& e)
            : MakefileTarget (t, e)
        {
        }

        String getBundleExtension() const
        {
            switch (type)
            {
                case GUIApp:
                case StandalonePlugIn:     return ".app";
                case VST3PlugIn:           return ".vst3";
                case AudioUnitPlugIn:      return ".component";
                case AAXPlugIn:            return ".aaxplugin";
                default:                   return {};
            }
        }

        bool isBundled() const             { return getBundleExtension().isNotEmpty(); }

        String getTargetFileSuffix() const override
        {
            if (type == DynamicLibrary)                      return ".dylib";
            if (type == SharedCodeTarget || type == StaticLibrary) return ".a";
            return {};
        }

        StringArray getCompilerFlags() const override
        {
            StringArray result;
            result.add ("-fvisibility=hidden");
            return result;
        }

        StringArray getLinkerFlags() const override
        {
            StringArray result;

            if (type == VST3PlugIn || type == AudioUnitPlugIn || type == AAXPlugIn)
            {
                result.add ("-bundle");
            }
            else if (type == DynamicLibrary)
            {
                result.add ("-dynamiclib");
                result.add ("-install_name");
                result.add ("@rpath/$(notdir $(JUCE_TARGET_" + getTargetVarName() + "))");
            }

            return result;
        }

        StringArray getTargetSettings (const MakeBuildConfiguration& config) const override
        {
            if (type == AggregateTarget)
                return {};

            StringArray s;
            const auto var = getTargetVarName();
            const auto cppflagsVarName = "JUCE_CPPFLAGS_" + var;

            s.add (cppflagsVarName + " := " + createGCCPreprocessorFlags (getDefines (config)));

            const auto cflags = getCompilerFlags();
            if (! cflags.isEmpty())
                s.add ("JUCE_CFLAGS_" + var + " := " + cflags.joinIntoString (" "));

            const auto ldflags = getLinkerFlags();
            if (! ldflags.isEmpty())
                s.add ("JUCE_LDFLAGS_" + var + " := " + ldflags.joinIntoString (" "));

            auto binaryName = owner.replacePreprocessorTokens (config, config.getTargetBinaryNameString());

            if (owner.getProject().getProjectType().isStaticLibrary() || type == SharedCodeTarget)
            {
                auto targetName = getStaticLibbedFilename (binaryName);
                s.add ("JUCE_TARGET_" + var + " := " + escapeQuotesAndSpaces (targetName));
            }
            else if (type == DynamicLibrary)
            {
                auto targetName = binaryName.upToLastOccurrenceOf (".", false, false) + ".dylib";
                s.add ("JUCE_TARGET_" + var + " := " + escapeQuotesAndSpaces (targetName));
            }
            else if (isBundled())
            {
                auto strippedName = binaryName.upToLastOccurrenceOf (".", false, false);
                auto bundleDir    = strippedName + getBundleExtension();

                s.add ("JUCE_BUNDLEDIR_" + var + " := " + escapeQuotesAndSpaces (bundleDir));
                s.add ("JUCE_TARGET_"    + var + " := " + escapeQuotesAndSpaces (bundleDir) + "/Contents/MacOS/" + escapeQuotesAndSpaces (strippedName));
                s.add ("JUCE_PLIST_"     + var + " := " + "Info-" + escapeQuotesAndSpaces (getTargetVarName())
                                                        + "-" + escapeQuotesAndSpaces (config.getName()) + ".plist");
            }
            else
            {
                // ConsoleApp
                auto targetName = binaryName.upToLastOccurrenceOf (".", false, false);
                s.add ("JUCE_TARGET_" + var + " := " + escapeQuotesAndSpaces (targetName));
            }

            // strip step — only for linkable targets (not static libs / shared code archive).
            {
                auto* macConfig = dynamic_cast<const MacMakeBuildConfiguration*> (&config);
                jassert (macConfig != nullptr);

                const auto isStaticArchive = owner.getProject().getProjectType().isStaticLibrary()
                                          || type == SharedCodeTarget
                                          || type == StaticLibrary;

                if (! isStaticArchive && macConfig->isStripLocalSymbolsEnabled())
                    s.add ("JUCE_STRIPCMD_" + var + " := $(STRIP) --strip-unneeded $(JUCE_OUTDIR)/$(JUCE_TARGET_" + var + ")");
                else
                    s.add ("JUCE_STRIPCMD_" + var + " := true");
            }

            // copy step
            if (config.isPluginBinaryCopyStepEnabled())
            {
                auto* macConfig = dynamic_cast<const MacMakeBuildConfiguration*> (&config);
                jassert (macConfig != nullptr);

                if (type == VST3PlugIn)
                {
                    s.add ("JUCE_VST3DESTDIR := " + macConfig->getVST3BinaryLocationString());
                    s.add ("JUCE_COPYCMD_" + var + " := $(JUCE_OUTDIR)/$(JUCE_BUNDLEDIR_" + var + ") $(JUCE_VST3DESTDIR)");
                }
                else if (type == AudioUnitPlugIn)
                {
                    s.add ("JUCE_AUDESTDIR := " + macConfig->getAUBinaryLocationString());
                    s.add ("JUCE_COPYCMD_" + var + " := $(JUCE_OUTDIR)/$(JUCE_BUNDLEDIR_" + var + ") $(JUCE_AUDESTDIR)");
                }
                else if (type == AAXPlugIn)
                {
                    s.add ("JUCE_AAXDESTDIR := " + macConfig->getAAXBinaryLocationString());
                    s.add ("JUCE_COPYCMD_" + var + " := $(JUCE_OUTDIR)/$(JUCE_BUNDLEDIR_" + var + ") $(JUCE_AAXDESTDIR)");
                }
            }

            return s;
        }

        void writeTargetLine (OutputStream& out, const StringArray& /*packages*/) override
        {
            jassert (type != AggregateTarget);

            const auto var = getTargetVarName();

            out << getBuildProduct() << " : "
                << "$(OBJECTS_" << var << ") $(RESOURCES)";

            if (type != SharedCodeTarget && owner.shouldBuildTargetType (SharedCodeTarget))
                out << " $(JUCE_OUTDIR)/$(JUCE_TARGET_SHARED_CODE)";

            out << newLine;

            out << "\t@echo Linking \"" << owner.getProject().getProjectNameString() << " - " << getName() << "\"" << newLine
                << "\t-$(V_AT)mkdir -p $(JUCE_BINDIR)" << newLine
                << "\t-$(V_AT)mkdir -p $(JUCE_LIBDIR)" << newLine
                << "\t-$(V_AT)mkdir -p $(JUCE_OUTDIR)" << newLine;

            if (isBundled())
            {
                out << "\t-$(V_AT)mkdir -p $(JUCE_OUTDIR)/$(JUCE_BUNDLEDIR_" << var << ")/Contents/MacOS"     << newLine
                    << "\t-$(V_AT)mkdir -p $(JUCE_OUTDIR)/$(JUCE_BUNDLEDIR_" << var << ")/Contents/Resources" << newLine;
            }

            if (owner.getProject().getProjectType().isStaticLibrary() || type == SharedCodeTarget)
            {
                out << "\t$(V_AT)$(AR) -rcs " << getBuildProduct()
                    << " $(OBJECTS_" << var << ")" << newLine;
            }
            else
            {
                out << "\t$(V_AT)$(CXX) -o " << getBuildProduct()
                    << " $(OBJECTS_" << var << ") ";

                if (owner.shouldBuildTargetType (SharedCodeTarget) && type != SharedCodeTarget)
                    out << "$(JUCE_OUTDIR)/$(JUCE_TARGET_SHARED_CODE) ";

                out << "$(JUCE_LDFLAGS) $(JUCE_LDFLAGS_" << var << ") $(RESOURCES) $(TARGET_ARCH)" << newLine;
            }

            if (isBundled())
            {
                out << "\t-$(V_AT)cp $(JUCE_PLIST_" << var << ")"
                    << " $(JUCE_OUTDIR)/$(JUCE_BUNDLEDIR_" << var << ")/Contents/Info.plist" << newLine
                    << "\t-$(V_AT)printf \"" << getPkgInfoBytes() << "\""
                    << " > $(JUCE_OUTDIR)/$(JUCE_BUNDLEDIR_" << var << ")/Contents/PkgInfo" << newLine
                    << "\t-$(V_AT)[ -f Icon.icns ] && cp Icon.icns $(JUCE_OUTDIR)/$(JUCE_BUNDLEDIR_" << var << ")/Contents/Resources/ || true" << newLine;
            }

            out << "\t-$(V_AT)$(JUCE_STRIPCMD_" << var << ")" << newLine;

            if (type == VST3PlugIn || type == AudioUnitPlugIn || type == AAXPlugIn)
            {
                auto destVar = (type == VST3PlugIn ? "JUCE_VST3DESTDIR"
                                                    : (type == AudioUnitPlugIn ? "JUCE_AUDESTDIR" : "JUCE_AAXDESTDIR"));

                out << "ifneq ($(" << destVar << "),)" << newLine
                    << "\t-$(V_AT)mkdir -p $(" << destVar << ")" << newLine
                    << "\t-$(V_AT)cp -R $(JUCE_COPYCMD_" << var << ")/" << newLine
                    << "endif" << newLine;
            }

            out << newLine;
        }

        void addFiles (OutputStream& out, const Array<std::pair<File, String>>& filesToCompile) override
        {
            auto cppflagsVarName = "JUCE_CPPFLAGS_" + getTargetVarName();
            auto cflagsVarName   = "JUCE_CFLAGS_"   + getTargetVarName();

            auto& project = const_cast<Project&> (owner.getProject());
            auto& macOwner = static_cast<const MacOSMakefileProjectExporter&> (owner);

            for (auto& f : filesToCompile)
            {
                build_tools::RelativePath relativePath (f.first, owner.getTargetFolder(), build_tools::RelativePath::buildTargetFolder);

                auto item = project.getMainGroup().findItemForFile (f.first);
                bool userFlaggedSkip = (item.isValid() && (bool) item.state[Ids::skipPCH]);
                bool moduleSkipByPattern = macOwner.shouldSkipPCHForModuleFile (f.first);
                bool skipPCH = userFlaggedSkip || moduleSkipByPattern;

                String compiler;
                bool usePCH = false;

                if (relativePath.hasFileExtension ("c;s;S"))
                {
                    compiler = "$(V_AT)$(CC) $(JUCE_CFLAGS)";
                }
                else if (relativePath.hasFileExtension ("mm;m"))
                {
                    compiler = "$(V_AT)$(CXX) -x objective-c++ $(JUCE_CXXFLAGS)";
                }
                else if (skipPCH)
                {
                    compiler = "$(V_AT)$(CXX) $(JUCE_CXXFLAGS) -DJUCE_SKIP_PRECOMPILED_HEADER";
                }
                else
                {
                    compiler = "$(V_AT)$(CXX) $(JUCE_PCH_INC) $(JUCE_CXXFLAGS)";
                    usePCH = true;
                }

                out << "$(JUCE_OBJDIR)/" << escapeQuotesAndSpaces (owner.getObjectFileFor (relativePath)) << ": "
                    << escapeQuotesAndSpaces (relativePath.toUnixStyle())
                    << (usePCH ? " $(JUCE_PCH)" : "") << newLine
                    << "\t-$(V_AT)mkdir -p $(JUCE_OBJDIR)" << newLine
                    << "\t@echo \"Compiling " << relativePath.getFileName() << "\"" << newLine
                    << "\t" << compiler
                    << " $(" << cppflagsVarName << ") $(" << cflagsVarName << ")"
                    << (f.second.isNotEmpty() ? " $(" + owner.getCompilerFlagSchemeVariableName (f.second) + ")" : "")
                    << " -o \"$@\" -c \"$<\"" << newLine
                    << newLine;
            }
        }

    private:
        String getPkgInfoBytes() const
        {
            auto packageType = build_tools::getXcodePackageType (type);
            auto signature   = build_tools::getXcodeBundleSignature (type);

            if (packageType.isEmpty())  packageType = "BNDL";
            if (signature.isEmpty())    signature   = "????";

            return packageType + signature;
        }
    };

    //==============================================================================
    static String getDisplayName()        { return "macOS Makefile"; }
    static String getValueTreeTypeName()  { return "MACOSX_MAKE"; }
    static String getTargetFolderName()   { return "MacOSXMakefile"; }

    Identifier getExporterIdentifier() const override { return getValueTreeTypeName(); }

    static MacOSMakefileProjectExporter* createForSettings (Project& projectToUse, const ValueTree& settingsToUse)
    {
        if (settingsToUse.hasType (getValueTreeTypeName()))
            return new MacOSMakefileProjectExporter (projectToUse, settingsToUse);

        return nullptr;
    }

    //==============================================================================
    MacOSMakefileProjectExporter (Project& p, const ValueTree& t)
        : MakefileProjectExporter (p, t),
          plistToMergeValue         (settings, Ids::customPList,          getUndoManager()),
          extraFrameworksValue      (settings, Ids::extraFrameworks,      getUndoManager()),
          frameworkSearchPathsValue (settings, Ids::frameworkSearchPaths, getUndoManager()),
          suppressPlistResourceUsageValue (settings, Ids::suppressPlistResourceUsage, getUndoManager(), false),
          pchSkipModulePatternsValue (settings, Identifier ("pchSkipModulePatterns"), getUndoManager(), "juce_*"),
          compileFirstPatternsValue  (settings, Identifier ("compileFirstPatterns"),  getUndoManager()),
          featureGatedModulesValue   (settings, Identifier ("featureGatedModules"),   getUndoManager())
    {
        name = getDisplayName();
        targetLocationValue.setDefault (getDefaultBuildsRootFolder() + getTargetFolderName());
    }

    //==============================================================================
    // Returns list of module IDs whose feature-gating macro is absent or "0"
    // for the given config. featureGatedModulesValue format: "moduleID:MACRO, ..."
    StringArray getGatedOffModuleIDsForConfig (const BuildConfiguration& config) const
    {
        StringArray result;
        auto text = featureGatedModulesValue.get().toString().trim();
        if (text.isEmpty())
            return result;

        auto defs = config.getAllPreprocessorDefs();

        auto pairs = StringArray::fromTokens (text, ",;", "");
        for (auto& p : pairs)
        {
            auto t = p.trim();
            if (t.isEmpty() || ! t.contains (":"))
                continue;

            auto moduleID  = t.upToFirstOccurrenceOf (":", false, false).trim();
            auto macroName = t.fromFirstOccurrenceOf (":", false, false).trim();

            auto macroVal = defs[macroName];
            // Only exclude when explicitly set to "0". Absent macro means "use module default"
            // (JUCE convention: #ifndef FOO #define FOO 1).
            if (macroVal == "0")
                result.add (moduleID);
        }

        return result;
    }

    static bool fileBelongsToGatedOffModule (const File& f, const StringArray& gatedIDs)
    {
        if (gatedIDs.isEmpty())
            return false;

        auto name = f.getFileNameWithoutExtension();
        if (! name.startsWith ("include_"))
            return false;

        auto rest = name.substring (8);
        for (auto& id : gatedIDs)
            if (rest == id || rest.startsWith (id + "_"))
                return true;

        return false;
    }

    //==============================================================================
    // Sort filesToCompile so TUs matching compileFirstPatternsValue appear first
    // (in pattern order), then the rest alphabetically. This gives `make -j` a chance
    // to start the heavy TUs immediately, avoiding end-of-build stragglers on a single core.
    void sortFilesByCompilePriority (Array<std::pair<File, String>>& files) const
    {
        auto patternText = compileFirstPatternsValue.get().toString().trim();
        if (patternText.isEmpty())
            return;

        auto patterns = StringArray::fromTokens (patternText, ",; ", "");
        patterns.removeEmptyStrings();
        patterns.trim();

        auto priorityOf = [&] (const File& f) -> int
        {
            auto name = f.getFileNameWithoutExtension();
            for (int i = 0; i < patterns.size(); ++i)
                if (name.matchesWildcard (patterns[i], true))
                    return i;
            return patterns.size(); // unmatched goes last
        };

        std::stable_sort (files.begin(), files.end(),
                          [&] (const std::pair<File, String>& a, const std::pair<File, String>& b)
                          {
                              return priorityOf (a.first) < priorityOf (b.first);
                          });
    }

    //==============================================================================
    // Returns true if the given module-proxy .cpp/.mm file should skip PCH, based on
    // comma-separated wildcard patterns in pchSkipModulePatternsValue (e.g. "juce_*,hi_*").
    // Non-module files (user code) return false.
    bool shouldSkipPCHForModuleFile (const File& f) const
    {
        auto filename = f.getFileNameWithoutExtension();
        if (! filename.startsWith ("include_"))
            return false;

        auto moduleIdPrefix = filename.substring (8);

        auto patterns = StringArray::fromTokens (pchSkipModulePatternsValue.get().toString(), ",; ", "");
        patterns.removeEmptyStrings();

        for (auto& p : patterns)
            if (moduleIdPrefix.matchesWildcard (p, true))
                return true;

        return false;
    }

    //==============================================================================
    bool usesMMFiles() const override                       { return true; }
    bool isMakefile() const override                        { return true; }
    bool isLinux() const override                           { return false; }
    bool isOSX() const override                             { return true; }
    bool supportsPrecompiledHeaders() const override        { return true; }

    bool shouldFileBeCompiledByDefault (const File& file) const override
    {
        return file.hasFileExtension (sourceFileExtensions);
    }

    bool anyConfigUsesPCH() const
    {
        for (ConstConfigIterator c (*this); c.next();)
            if (c->shouldUsePrecompiledHeaderFile())
                return true;
        return false;
    }

    bool supportsTargetType (build_tools::ProjectType::Target::Type type) const override
    {
        switch (type)
        {
            case build_tools::ProjectType::Target::GUIApp:
            case build_tools::ProjectType::Target::ConsoleApp:
            case build_tools::ProjectType::Target::StandalonePlugIn:
            case build_tools::ProjectType::Target::SharedCodeTarget:
            case build_tools::ProjectType::Target::AggregateTarget:
            case build_tools::ProjectType::Target::VST3PlugIn:
            case build_tools::ProjectType::Target::AudioUnitPlugIn:
            case build_tools::ProjectType::Target::AAXPlugIn:
            case build_tools::ProjectType::Target::DynamicLibrary:
            case build_tools::ProjectType::Target::StaticLibrary:
                return true;
            case build_tools::ProjectType::Target::VSTPlugIn:
            case build_tools::ProjectType::Target::RTASPlugIn:
            case build_tools::ProjectType::Target::AudioUnitv3PlugIn:
            case build_tools::ProjectType::Target::UnityPlugIn:
            case build_tools::ProjectType::Target::unspecified:
            default:
                break;
        }

        return false;
    }

    void createExporterProperties (PropertyListBuilder& props) override
    {
        props.add (new TextPropertyComponent (extraFrameworksValue, "Extra Frameworks", 8192, false),
                   "A comma-separated list of extra macOS frameworks to link against (e.g. \"Accelerate,Metal\"). "
                   "JUCE module frameworks are pulled in automatically.");

        props.add (new TextPropertyComponent (frameworkSearchPathsValue, "Framework Search Paths", 8192, true),
                   "A set of paths (one per line) to search for frameworks (emitted as -F flags).");

        props.add (new TextPropertyComponent (plistToMergeValue, "Custom PList", 8192, true),
                   "An XML snippet that will be merged into the generated Info.plist file for each bundle target.");

        props.add (new ChoicePropertyComponent (suppressPlistResourceUsageValue, "Suppress AU plist resourceUsage key"),
                   "Suppress the AudioComponent resourceUsage plist entry to allow AUs to access resources outside the sandbox.");

        props.add (new TextPropertyComponent (pchSkipModulePatternsValue, "PCH: Skip Modules Matching", 1024, false),
                   "Comma-separated wildcard patterns of module IDs whose source files should skip the precompiled header. "
                   "Default \"juce_*\" skips all JUCE modules (they pull in Carbon/Cocoa headers that clash with juce::Component when pre-imported via PCH). "
                   "Your own modules (e.g. hi_*) still benefit from PCH. Use \"*\" to disable PCH for all module code, or leave empty to force PCH everywhere.");

        props.add (new TextPropertyComponent (compileFirstPatternsValue, "Compile First (Priority Patterns)", 1024, false),
                   "Comma-separated wildcard patterns matched against object-file basenames (without extension). "
                   "Matching files are emitted at the front of the OBJECTS list so `make -j` starts them first. "
                   "Use for your slowest translation units (e.g. \"*hi_core*, *hi_scripting_01*, *hi_backend*\") to avoid end-of-build stragglers. "
                   "Leave empty for alphabetical order.");

        props.add (new TextPropertyComponent (featureGatedModulesValue, "Feature-Gated Modules", 1024, false),
                   "Comma-separated list of \"moduleID:MACRO\" pairs. If MACRO is not defined as 1 in the current config's preprocessor defs, "
                   "all source files of moduleID are excluded from the build entirely (no empty TU compile cost). "
                   "Example: \"hi_rlottie:HISE_INCLUDE_RLOTTIE, hi_loris:HISE_INCLUDE_LORIS\". "
                   "Saves ~500ms per skipped TU (clang startup + SDK scan).");
    }

    void initialiseDependencyPathValues() override
    {
        vstLegacyPathValueWrapper.init ({ settings, Ids::vstLegacyFolder, nullptr },
                                        getAppSettings().getStoredPath (Ids::vstLegacyPath, TargetOS::osx), TargetOS::osx);

        aaxPathValueWrapper.init ({ settings, Ids::aaxFolder, nullptr },
                                  getAppSettings().getStoredPath (Ids::aaxPath, TargetOS::osx), TargetOS::osx);

        rtasPathValueWrapper.init ({ settings, Ids::rtasFolder, nullptr },
                                   getAppSettings().getStoredPath (Ids::rtasPath, TargetOS::osx), TargetOS::osx);
    }

    void addPlatformSpecificSettingsForProjectType (const build_tools::ProjectType&) override
    {
        callForAllSupportedTargets ([this] (build_tools::ProjectType::Target::Type targetType)
                                    {
                                        targets.insert (targetType == build_tools::ProjectType::Target::AggregateTarget ? 0 : -1,
                                                        new MacMakefileTarget (targetType, *this));
                                    });

        jassert (targets.size() > 0);
    }

    //==============================================================================
    StringArray getOSXFrameworksList() const
    {
        StringArray result (osxFrameworks);
        result.addTokens (extraFrameworksValue.get().toString(), ",;", {});
        result.trim();
        result.removeEmptyStrings();
        result.removeDuplicates (false);
        return result;
    }

    StringArray getFrameworkSearchPathsList() const
    {
        auto paths = getSearchPathsFromString (frameworkSearchPathsValue.get().toString());
        paths.removeDuplicates (false);
        return paths;
    }

    //==============================================================================
    void create (const OwnedArray<LibraryModule>&) const override
    {
        build_tools::writeStreamToFile (getTargetFolder().getChildFile ("Makefile"), [&] (MemoryOutputStream& mo)
        {
            mo.setNewLineString (getNewLineString());
            writeMacMakefile (mo);
        });

        writeIconFile();
        writePlistFiles();
        writePCHSourceFiles();
    }

    void writePCHSourceFiles() const
    {
        for (ConstConfigIterator c (*this); c.next();)
        {
            if (! c->shouldUsePrecompiledHeaderFile())
                continue;

            auto* macCfg = dynamic_cast<const MacMakeBuildConfiguration*> (c.config.get());
            if (macCfg == nullptr)
                continue;

            auto userHeaderPath = macCfg->getUserPrecompiledHeaderPath();
            if (userHeaderPath.isEmpty())
                continue;

            auto absHeaderPath = project.getProjectFolder().getChildFile (userHeaderPath);
            if (! absHeaderPath.existsAsFile())
                continue;

            auto relFromExporter = build_tools::RelativePath (absHeaderPath,
                                                              getTargetFolder(),
                                                              build_tools::RelativePath::buildTargetFolder).toUnixStyle();

            build_tools::writeStreamToFile (getTargetFolder().getChildFile (c->getPrecompiledHeaderFilename() + ".h"),
                                            [&] (MemoryOutputStream& mo)
                                            {
                                                mo.setNewLineString (getNewLineString());
                                                mo << "// Auto-generated PCH wrapper for the " << c->getName() << " config." << newLine
                                                   << "// Includes the user-specified header so #pragma once dedupes correctly." << newLine
                                                   << "#pragma once" << newLine
                                                   << "#ifndef " << BuildConfiguration::getSkipPrecompiledHeaderDefine() << newLine
                                                   << " #include \"" << relFromExporter << "\"" << newLine
                                                   << "#endif" << newLine;
                                            });
        }
    }

    //==============================================================================
    // Public so that addLibsToExporter can fill them (similar to xcodeFrameworks on the Xcode exporter).
    StringArray osxFrameworks, osxLibs;

protected:
    //==============================================================================
    StringPairArray getDefines (const BuildConfiguration& config) const
    {
        StringPairArray result;

        if (config.isDebug())
        {
            result.set ("DEBUG", "1");
            result.set ("_DEBUG", "1");
        }
        else
        {
            result.set ("NDEBUG", "1");
        }

        result = mergePreprocessorDefs (result, getAllPreprocessorDefs (config, build_tools::ProjectType::Target::unspecified));

        return result;
    }

    StringArray getMacCFlags (const MacMakeBuildConfiguration& config) const
    {
        StringArray result;

        if (config.isDebug())
        {
            result.add ("-g");
            result.add ("-ggdb");
        }

        result.add ("-O" + config.getGCCOptimisationFlag());

        if (config.isLinkTimeOptimisationEnabled())
            result.add ("-flto=thin");

        for (auto& recommended : config.getRecommendedCompilerWarningFlags().common)
            result.add (recommended);

        auto sdk = config.getMacOSBaseSDKString().trim();
        if (sdk.isNotEmpty())
            result.add ("-isysroot $(shell xcrun --sdk macosx" + sdk + " --show-sdk-path 2>/dev/null || xcrun --sdk macosx --show-sdk-path)");
        else
            result.add ("-isysroot $(shell xcrun --sdk macosx --show-sdk-path)");

        auto dep = config.getMacOSDeploymentTargetString().trim();
        if (dep.isNotEmpty())
            result.add ("-mmacosx-version-min=" + dep);

        auto extra = replacePreprocessorTokens (config, getExtraCompilerFlagsString()).trim();
        if (extra.isNotEmpty())
            result.add (extra);

        return result;
    }

    StringArray getMacCXXFlags (const BuildConfiguration& config) const
    {
        StringArray result;

        for (auto& recommended : config.getRecommendedCompilerWarningFlags().cpp)
            result.add (recommended);

        auto cppStandard = project.getCppStandardString();

        if (cppStandard == "latest")
            cppStandard = project.getLatestNumberedCppStandardString();

        result.add ("-std=" + String (shouldUseGNUExtensions() ? "gnu++" : "c++") + cppStandard);
        result.add ("-stdlib=libc++");

        return result;
    }

    StringArray getMacHeaderSearchPaths (const BuildConfiguration& config) const
    {
        StringArray searchPaths (extraSearchPaths);
        searchPaths.addArray (config.getHeaderSearchPaths());
        searchPaths = getCleanedStringArray (searchPaths);

        StringArray result;

        for (auto& path : searchPaths)
            result.add (build_tools::unixStylePath (replacePreprocessorTokens (config, path)));

        return result;
    }

    StringArray getMacLibrarySearchPaths (const BuildConfiguration& config) const
    {
        auto result = getSearchPathsFromString (config.getLibrarySearchPathString());

        for (auto path : moduleLibSearchPaths)
            result.add (path + "/" + config.getModuleLibraryArchName());

        return result;
    }

    StringArray getMacLibraryNames (const BuildConfiguration& config) const
    {
        StringArray result (osxLibs);

        auto libraries = StringArray::fromTokens (getExternalLibrariesString(), ";", "\"'");
        libraries.removeEmptyStrings();

        for (auto& lib : libraries)
            result.add (replacePreprocessorTokens (config, lib).trim());

        result.removeDuplicates (false);
        return result;
    }

    StringArray getMacLinkerFlags (const MacMakeBuildConfiguration& config) const
    {
        StringArray result;
        result.add ("-fvisibility=hidden");
        result.add ("-stdlib=libc++");

        auto dep = config.getMacOSDeploymentTargetString().trim();
        if (dep.isNotEmpty())
            result.add ("-mmacosx-version-min=" + dep);

        if (config.isLinkTimeOptimisationEnabled())
        {
            result.add ("-flto=thin");
            result.add ("-Wl,-cache_path_lto,build/lto_cache");
        }

        auto extraFlags = getExtraLinkerFlagsString().trim();
        if (extraFlags.isNotEmpty())
            result.add (replacePreprocessorTokens (config, extraFlags));

        return result;
    }

    //==============================================================================
    void writeMacConfig (OutputStream& out, const MacMakeBuildConfiguration& config, const Array<std::pair<File, String>>& filesToCompile) const
    {
        String buildDirName ("build");
        auto intermediatesDirName = buildDirName + "/intermediate/" + config.getName();
        auto outputDir = buildDirName + "/" + config.getName();

        if (config.getTargetBinaryRelativePathString().isNotEmpty())
        {
            build_tools::RelativePath binaryPath (config.getTargetBinaryRelativePathString(), build_tools::RelativePath::projectFolder);
            outputDir = binaryPath.rebased (projectFolder, getTargetFolder(), build_tools::RelativePath::buildTargetFolder).toUnixStyle();
        }

        out << "ifeq ($(CONFIG)," << escapeQuotesAndSpaces (config.getName()) << ")" << newLine
            << "  JUCE_BINDIR := " << escapeQuotesAndSpaces (buildDirName)           << newLine
            << "  JUCE_LIBDIR := " << escapeQuotesAndSpaces (buildDirName)           << newLine
            << "  JUCE_OBJDIR := " << escapeQuotesAndSpaces (intermediatesDirName)   << newLine
            << "  JUCE_OUTDIR := " << escapeQuotesAndSpaces (outputDir)              << newLine
            << newLine
            << "  ifeq ($(TARGET_ARCH),)"                                            << newLine
            << "    TARGET_ARCH := " << config.getArchFlags()                        << newLine
            << "  endif"                                                             << newLine
            << newLine;

        if (config.shouldUsePrecompiledHeaderFile())
        {
            out << "  JUCE_PCH_SRC := " << config.getPrecompiledHeaderFilename() << ".h" << newLine
                << "  JUCE_PCH     := $(JUCE_OBJDIR)/JucePrecompiledHeader.pch"          << newLine
                << "  JUCE_PCH_INC := -include-pch $(JUCE_PCH)"                          << newLine
                << "  # Clang cannot emit one PCH for fat binaries - disable PCH when multi-arch." << newLine
                << "  ifneq ($(word 2,$(filter -arch,$(TARGET_ARCH))),)"                 << newLine
                << "    JUCE_PCH     :="                                                 << newLine
                << "    JUCE_PCH_INC :="                                                 << newLine
                << "  endif"                                                             << newLine
                << newLine;
        }
        else
        {
            out << "  JUCE_PCH     :="                                                   << newLine
                << "  JUCE_PCH_INC :="                                                   << newLine
                << newLine;
        }

        // CPPFLAGS
        out << "  JUCE_CPPFLAGS := $(DEPFLAGS) ";
        out << createGCCPreprocessorFlags (mergePreprocessorDefs (getDefines (config),
                                                                  getAllPreprocessorDefs (config, build_tools::ProjectType::Target::unspecified)));

        for (auto& path : getMacHeaderSearchPaths (config))
            out << " -I" << escapeQuotesAndSpaces (path).replace ("~", "$(HOME)");

        out << " $(CPPFLAGS)" << newLine;

        // per-target settings
        for (auto* target : targets)
        {
            auto lines = target->getTargetSettings (config);

            if (lines.size() > 0)
                out << "  " << lines.joinIntoString ("\n  ") << newLine;

            out << newLine;
        }

        // CFLAGS
        out << "  JUCE_CFLAGS += $(JUCE_CPPFLAGS) $(TARGET_ARCH)";

        auto cflags = getMacCFlags (config).joinIntoString (" ");
        if (cflags.isNotEmpty())
            out << " " << cflags;

        out << " $(CFLAGS)" << newLine;

        // CXXFLAGS
        out << "  JUCE_CXXFLAGS += $(JUCE_CFLAGS)";

        auto cxxflags = getMacCXXFlags (config).joinIntoString (" ");
        if (cxxflags.isNotEmpty())
            out << " " << cxxflags;

        out << " $(CXXFLAGS)" << newLine;

        // LDFLAGS
        out << "  JUCE_LDFLAGS += $(TARGET_ARCH) -L$(JUCE_BINDIR) -L$(JUCE_LIBDIR)";

        for (auto& path : getMacLibrarySearchPaths (config))
            out << " -L" << escapeQuotesAndSpaces (path).replace ("~", "$(HOME)");

        for (auto& path : getFrameworkSearchPathsList())
            out << " -F" << escapeQuotesAndSpaces (path).replace ("~", "$(HOME)");

        auto linkerFlags = getMacLinkerFlags (config).joinIntoString (" ");
        if (linkerFlags.isNotEmpty())
            out << " " << linkerFlags;

        for (auto& lib : getMacLibraryNames (config))
            out << " -l" << lib;

        for (auto& fw : getOSXFrameworksList())
            out << " -framework " << fw;

        out << " $(LDFLAGS)" << newLine;

        out << newLine;

        // Per-config OBJECTS lists — feature-gated modules excluded based on this config's macros.
        auto gatedIDs = getGatedOffModuleIDsForConfig (config);

        for (auto* target : targets)
        {
            if (target->type == build_tools::ProjectType::Target::AggregateTarget)
                continue;

            auto var = target->getTargetVarName();
            auto targetType = (project.isAudioPluginProject() ? target->type : MakefileTarget::SharedCodeTarget);

            out << "  OBJECTS_" << var << " := \\" << newLine;

            for (auto& f : filesToCompile)
            {
                if (project.getTargetTypeFromFilePath (f.first, true) != targetType)
                    continue;

                if (fileBelongsToGatedOffModule (f.first, gatedIDs))
                    continue;

                build_tools::RelativePath rel (f.first, getTargetFolder(), build_tools::RelativePath::buildTargetFolder);
                out << "    $(JUCE_OBJDIR)/" << escapeQuotesAndSpaces (getObjectFileFor (rel)) << " \\" << newLine;
            }

            out << newLine;
        }

        out << "  CLEANCMD = rm -rf $(JUCE_OUTDIR) $(JUCE_OBJDIR)" << newLine
            << "endif" << newLine
            << newLine;
    }

    void writeMacMakefile (OutputStream& out) const
    {
        out << "# Automatically generated makefile, created by the Projucer"                                     << newLine
            << "# Don't edit this file! Your changes will be overwritten when you re-save the Projucer project!" << newLine
            << newLine;

        out << "# Cap parallel jobs to physical core count (prevents OOM on template-heavy codebases)." << newLine
            << "# Override by invoking make with an explicit -j flag, e.g. `make -j8`."                 << newLine
            << "ifeq ($(origin JUCE_JOBS_CAPPED),undefined)"                                            << newLine
            << "  export JUCE_JOBS_CAPPED := 1"                                                         << newLine
            << "  MAKEFLAGS += -j$(shell sysctl -n hw.physicalcpu) -l $(shell sysctl -n hw.physicalcpu)" << newLine
            << "endif"                                                                                  << newLine
            << newLine;

        out << "# build with \"V=1\" for verbose builds" << newLine
            << "ifeq ($(V), 1)"                          << newLine
            << "V_AT ="                                  << newLine
            << "else"                                    << newLine
            << "V_AT = @"                                << newLine
            << "endif"                                   << newLine
            << newLine;

        out << "# (disables dependency generation if multiple architectures are set)" << newLine
            << "DEPFLAGS := $(if $(word 2, $(TARGET_ARCH)), , -MMD)"                   << newLine
            << newLine;

        out << ".DEFAULT_GOAL := all" << newLine
            << newLine;

        out << "ifndef STRIP"  << newLine
            << "  STRIP=strip" << newLine
            << "endif"         << newLine
            << newLine;

        out << "ifndef AR" << newLine
            << "  AR=ar"   << newLine
            << "endif"     << newLine
            << newLine;

        out << "ifndef CONFIG"                                                       << newLine
            << "  CONFIG=" << escapeQuotesAndSpaces (getConfiguration (0)->getName()) << newLine
            << "endif"                                                               << newLine
            << newLine;

        out << "JUCE_ARCH_LABEL := $(shell uname -m)" << newLine
            << newLine;

        Array<std::pair<File, String>> filesToCompile;

        for (int i = 0; i < getAllGroups().size(); ++i)
            findAllFilesToCompile (getAllGroups().getReference (i), filesToCompile);

        sortFilesByCompilePriority (filesToCompile);

        writeCompilerFlagSchemes (out, filesToCompile);

        for (ConstConfigIterator config (*this); config.next();)
            writeMacConfig (out, dynamic_cast<const MacMakeBuildConfiguration&> (*config), filesToCompile);

        if (anyConfigUsesPCH())
        {
            out << "$(JUCE_PCH): $(JUCE_PCH_SRC)"                                                                                         << newLine
                << "\t-$(V_AT)mkdir -p $(JUCE_OBJDIR)"                                                                                    << newLine
                << "\t@echo \"Precompiling $(JUCE_PCH_SRC)\""                                                                             << newLine
                << "\t$(V_AT)$(CXX) $(JUCE_CXXFLAGS) $(JUCE_CPPFLAGS_SHARED_CODE) -x c++-header -o $(JUCE_PCH) $(JUCE_PCH_SRC)"           << newLine
                << newLine;
        }

        out << getPhonyTargetLine() << newLine << newLine;

        writeTargetLines (out);

        auto getFilesForTarget = [this] (const Array<std::pair<File, String>>& files,
                                         MakefileTarget* target) -> Array<std::pair<File, String>>
        {
            Array<std::pair<File, String>> targetFiles;
            auto targetType = (project.isAudioPluginProject() ? target->type : MakefileTarget::SharedCodeTarget);

            for (auto& f : files)
                if (project.getTargetTypeFromFilePath (f.first, true) == targetType)
                    targetFiles.add (f);

            return targetFiles;
        };

        for (auto target : targets)
            target->addFiles (out, getFilesForTarget (filesToCompile, target));

        out << "clean:"                           << newLine
            << "\t@echo Cleaning " << projectName << newLine
            << "\t$(V_AT)$(CLEANCMD)"             << newLine
            << newLine;

        out << "strip:"                                                       << newLine
            << "\t@echo Stripping " << projectName                            << newLine
            << "\t-$(V_AT)$(STRIP) --strip-unneeded $(JUCE_OUTDIR)/$(TARGET)" << newLine
            << newLine;

        out << ".PHONY: distclean"                    << newLine
            << "distclean:"                           << newLine
            << "\t@echo Nuking full build tree for " << projectName << newLine
            << "\t$(V_AT)rm -rf build"                << newLine
            << newLine;

        writeIncludeLines (out);
    }

    void writeTargetLines (OutputStream& out) const
    {
        auto n = targets.size();

        for (int i = 0; i < n; ++i)
        {
            if (auto* target = targets.getUnchecked (i))
            {
                if (target->type == build_tools::ProjectType::Target::AggregateTarget)
                {
                    StringArray dependencies;
                    MemoryOutputStream subTargetLines;

                    for (int j = 0; j < n; ++j)
                    {
                        if (i == j) continue;

                        if (auto* dependency = targets.getUnchecked (j))
                        {
                            if (dependency->type != build_tools::ProjectType::Target::SharedCodeTarget)
                            {
                                auto phonyName = dependency->getPhonyName();
                                subTargetLines << phonyName << " : " << dependency->getBuildProduct() << newLine;
                                dependencies.add (phonyName);
                            }
                        }
                    }

                    out << "all : " << dependencies.joinIntoString (" ") << newLine << newLine;
                    out << subTargetLines.toString()                     << newLine << newLine;
                }
                else
                {
                    if (! getProject().isAudioPluginProject())
                        out << "all : " << target->getBuildProduct() << newLine << newLine;

                    target->writeTargetLine (out, StringArray());
                }
            }
        }
    }

    //==============================================================================
    void writeIconFile() const
    {
        auto icons = getIcons();

        if (! icons.big && ! icons.small)
            return;

        auto iconFile = getTargetFolder().getChildFile ("Icon.icns");
        build_tools::writeMacIcon (icons, iconFile);
    }

    void writePlistFiles() const
    {
        auto icons = getIcons();
        File iconFile;

        if (icons.big || icons.small)
            iconFile = getTargetFolder().getChildFile ("Icon.icns");

        for (ConstConfigIterator c (*this); c.next();)
        {
            for (auto* target : targets)
            {
                auto* macTarget = dynamic_cast<const MacMakefileTarget*> (target);
                if (macTarget == nullptr || ! macTarget->isBundled())
                    continue;

                writePlistForTargetAndConfig (*macTarget, *c, iconFile);
            }
        }
    }

    void writePlistForTargetAndConfig (const MacMakefileTarget& target, const BuildConfiguration& config, const File& iconFile) const
    {
        build_tools::PlistOptions options;

        auto binaryName = replacePreprocessorTokens (config, config.getTargetBinaryNameString())
                            .upToLastOccurrenceOf (".", false, false);

        options.type                    = target.type;
        options.executableName          = binaryName;
        options.bundleIdentifier        = getBundleIdentifierForTarget (target.type);
        options.plistToMerge            = plistToMergeValue.get().toString();
        options.iOS                     = false;
        options.iconFile                = iconFile;
        options.projectName             = projectName;
        options.version                 = project.getVersionString();
        options.currentProjectVersion   = project.getVersionString();
        options.companyCopyright        = project.getCompanyCopyrightString();
        options.allPreprocessorDefs     = getAllPreprocessorDefs();
        options.documentExtensions      = {};
        options.pluginName              = project.getPluginNameString();
        options.pluginManufacturer      = project.getPluginManufacturerString();
        options.pluginDescription       = project.getPluginDescriptionString();
        options.pluginAUExportPrefix    = project.getPluginAUExportPrefixString();
        options.auMainType              = project.getAUMainTypeString();
        options.isAuSandboxSafe         = project.isAUSandBoxSafe();
        options.isPluginSynth           = project.isPluginSynth();
        options.suppressResourceUsage   = suppressPlistResourceUsageValue.get();
        options.enableIAA               = project.shouldEnableIAA();
        options.IAAPluginName           = project.getIAAPluginName();
        options.pluginManufacturerCode  = project.getPluginManufacturerCodeString();
        options.IAATypeCode             = project.getIAATypeCode();
        options.pluginCode              = project.getPluginCodeString();
        options.versionAsHex            = project.getVersionAsHexInteger();

        options.write (getTargetFolder().getChildFile ("Info-" + target.getTargetVarName() + "-" + config.getName() + ".plist"));
    }

    String getBundleIdentifierForTarget (build_tools::ProjectType::Target::Type t) const
    {
        auto base = project.getBundleIdentifierString();

        switch (t)
        {
            case build_tools::ProjectType::Target::VST3PlugIn:       return base + ".vst3";
            case build_tools::ProjectType::Target::AudioUnitPlugIn:  return base + ".au";
            case build_tools::ProjectType::Target::AAXPlugIn:        return base + ".aax";
            case build_tools::ProjectType::Target::StandalonePlugIn: return base + ".standalone";
            default:                                                 return base;
        }
    }

private:
    ValueWithDefault plistToMergeValue, extraFrameworksValue, frameworkSearchPathsValue, suppressPlistResourceUsageValue,
                     pchSkipModulePatternsValue, compileFirstPatternsValue, featureGatedModulesValue;

    JUCE_DECLARE_NON_COPYABLE (MacOSMakefileProjectExporter)
};
